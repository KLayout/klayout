
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include "dbCell.h"
#include "dbLayout.h"
#include "dbManager.h"
#include "dbBox.h"
#include "dbPCellVariant.h"
#include "dbLayoutUtils.h"
#include "dbLayerMapping.h"
#include "dbCellMapping.h"

#include <limits>

namespace db
{

struct CellOp
  : public db::Op
{
  CellOp () { }
  virtual ~CellOp () { }

  virtual void redo (db::Cell *) const = 0;
  virtual void undo (db::Cell *) const = 0;
};

class SwapLayerOp 
  : public CellOp
{
public:
  SwapLayerOp (unsigned int a, unsigned int b)
    : m_a (a), m_b (b)
  { }

  virtual void redo (db::Cell *cell) const
  {
    cell->swap (m_a, m_b);
  }

  virtual void undo (db::Cell *cell) const
  {
    cell->swap (m_a, m_b);
  }

private:
  unsigned int m_a, m_b;
};

struct SetCellPropId
  : public CellOp
{
  SetCellPropId (db::properties_id_type f, db::properties_id_type t)
    : m_from (f), m_to (t)
  { }

  virtual void redo (db::Cell *cell) const
  {
    cell->prop_id (m_to);
  }

  virtual void undo (db::Cell *cell) const
  {
    cell->prop_id (m_from);
  }

private:
  db::properties_id_type m_from, m_to;
};

Cell::box_type Cell::ms_empty_box = Cell::box_type ();

Cell::Cell (cell_index_type ci, db::Layout &l) 
  : db::Object (l.manager ()), 
    m_cell_index (ci), mp_layout (&l), m_instances (this), m_prop_id (0), m_hier_levels (0),
    m_bbox_needs_update (false), m_ghost_cell (false),
    mp_last (0), mp_next (0)
{
  //  .. nothing yet 
}

Cell::Cell (const Cell &d)
  : db::Object (d), 
    gsi::ObjectBase (),
    mp_layout (d.mp_layout), m_instances (this), m_prop_id (d.m_prop_id), m_hier_levels (d.m_hier_levels),
    mp_last (0), mp_next (0)
{
  m_cell_index = d.m_cell_index;
  operator= (d);
}

Cell &
Cell::operator= (const Cell &d)
{
  if (this != &d) {

    //  Note: the cell index is part of the cell's identity - hence we do not change it here. It's copied in 
    //  the copy ctor however.

    invalidate_hier ();

    clear_shapes_no_invalidate ();
    for (shapes_map::const_iterator s = d.m_shapes_map.begin (); s != d.m_shapes_map.end (); ++s) {
      shapes (s->first) = s->second;
    }

    m_ghost_cell = d.m_ghost_cell;
    m_instances = d.m_instances;
    m_bbox = d.m_bbox;
    m_bboxes = d.m_bboxes;
    m_hier_levels = d.m_hier_levels;
    m_prop_id = d.m_prop_id;
    m_bbox_needs_update = d.m_bbox_needs_update;

  }
  return *this;
}

Cell::~Cell ()
{
  clear_shapes ();
}

Cell *
Cell::clone (db::Layout &layout) const
{
  Cell *new_cell = new Cell (cell_index (), layout);
  *new_cell = *this;
  return new_cell;
}

unsigned int
Cell::layers () const
{
  if (m_shapes_map.empty ()) {
    return 0;
  } else {
    shapes_map::const_iterator s = m_shapes_map.end ();
    --s;
    return s->first + 1;
  }
}

bool
Cell::empty () const
{
  if (! m_instances.empty ()) {
    return false;
  }

  for (shapes_map::const_iterator s = m_shapes_map.begin (); s != m_shapes_map.end (); ++s) {
    if (! s->second.empty ()) {
      return false;
    }
  }

  return true;
}

void 
Cell::clear (unsigned int index)
{
  shapes_map::iterator s = m_shapes_map.find(index);
  if (s != m_shapes_map.end() && ! s->second.empty ()) {
    mp_layout->invalidate_bboxes (index);  //  HINT: must come before the change is done!
    s->second.clear ();
    m_bbox_needs_update = true;
  }
}

void
Cell::clear (unsigned int index, unsigned int types)
{
  shapes_map::iterator s = m_shapes_map.find(index);
  if (s != m_shapes_map.end() && ! s->second.empty ()) {
    mp_layout->invalidate_bboxes (index);  //  HINT: must come before the change is done!
    s->second.clear (types);
    m_bbox_needs_update = true;
  }
}

Cell::shapes_type &
Cell::shapes (unsigned int index) 
{
  shapes_map::iterator s = m_shapes_map.find(index);
  if (s == m_shapes_map.end()) {
    s = m_shapes_map.insert (std::make_pair(index, shapes_type (0, this, mp_layout ? mp_layout->is_editable () : true))).first;
    s->second.manager (manager ());
  }
  return s->second;
}

const Cell::shapes_type &
Cell::shapes (unsigned int index) const
{
  shapes_map::const_iterator s = m_shapes_map.find(index);
  if (s != m_shapes_map.end()) {
    return s->second;
  } else {
    //  Because of a gcc bug it seems to be not possible
    //  to instantiate a simple static object here:
    static const shapes_type *empty_shapes = 0;
    if (! empty_shapes) {
      empty_shapes = new shapes_type ();
    }
    return *empty_shapes;
  }
}

unsigned int
Cell::index_of_shapes (const Cell::shapes_type *shapes) const
{
  for (shapes_map::const_iterator s = m_shapes_map.begin (); s != m_shapes_map.end (); ++s) {
    if (&s->second == shapes) {
      return s->first;
    }
  }
  return std::numeric_limits<unsigned int>::max ();
}

void
Cell::clear_shapes ()
{
  mp_layout->invalidate_bboxes (std::numeric_limits<unsigned int>::max ());  //  HINT: must come before the change is done!
  clear_shapes_no_invalidate ();
}

void 
Cell::update_relations ()
{
  m_instances.update_relations (mp_layout, cell_index ());
}

bool 
Cell::is_shape_bbox_dirty () const
{
  if (m_bbox_needs_update) {
    return true;
  }
  for (shapes_map::const_iterator s = m_shapes_map.begin (); s != m_shapes_map.end (); ++s) {
    if (s->second.is_bbox_dirty ()) {
      return true;
    }
  }
  return false;
}

bool 
Cell::update_bbox (unsigned int layers)
{
  unsigned int l;

  //  determine the bounding box
  box_type org_bbox = m_bbox;
  m_bbox = box_type ();

  //  save the original boxes for simple compare
  box_map org_bboxes;
  org_bboxes.swap (m_bboxes);
  
  //  compute the per-layer bboxes of the cell instances
  //  exploit the fact that these are sorted by instance,
  //  rotation and magnification.
  for (instances_type::sorted_inst_iterator o = m_instances.begin_sorted_insts (); o != m_instances.end_sorted_insts (); ) {

    const cell_inst_array_type *o1_inst = *o;

    instances_type::sorted_inst_iterator oo = o;
    while (++oo != m_instances.end_sorted_insts () && (*oo)->raw_equal (*o1_inst)) 
      ;

    box_type raw_box;
    while (o != oo) {
      raw_box += (*o)->raw_bbox ();
      ++o;
    }

    for (l = 0; l < layers; ++l) {

      //  the per-layer bounding boxes
      db::box_convert <cell_inst_type> bc (*mp_layout, l);
      box_type lbox = o1_inst->bbox_from_raw_bbox (raw_box, bc);

      if (! lbox.empty ()) {
        m_bbox += lbox;
        box_map::iterator b = m_bboxes.find (l);
        if (b == m_bboxes.end ()) {
            m_bboxes.insert (std::make_pair (l, lbox));
        } else {
            b->second += lbox;
        }
      }

    }
    
  }

  //  update the bboxes of the shapes lists
  for (shapes_map::iterator s = m_shapes_map.begin (); s != m_shapes_map.end (); ++s) {

    s->second.reset_bbox_dirty ();

    box_type sbox (s->second.bbox ());

    if (! sbox.empty ()) {
      m_bbox += sbox;
      box_map::iterator b = m_bboxes.find (s->first);
      if (b == m_bboxes.end ()) {
         m_bboxes.insert (std::make_pair (s->first, sbox));
      } else {
         b->second += sbox;
      }
    }
   
  }

  //  reset "dirty child instances" flag
  m_bbox_needs_update = false;

  //  return true, if anything has changed with the box 
  return (org_bbox != m_bbox || org_bboxes != m_bboxes);

}

void
Cell::copy (unsigned int src, unsigned int dest)
{
  if (src != dest) {
    shapes (dest).insert (shapes (src));
  } else {
    //  When duplicating the layer, first create a copy to avoid problems with non-stable containers
    //  Hint: using the assignment and not the copy ctor does not copy the db::Manager association.
    db::Shapes shape_copy;
    shape_copy = shapes (src);
    shapes (dest).insert (shape_copy);
  }
}

void
Cell::copy (unsigned int src, unsigned int dest, unsigned int types)
{
  if (src != dest) {
    shapes (dest).insert (shapes (src), types);
  } else {
    //  When duplicating the layer, first create a copy to avoid problems with non-stable containers
    //  Hint: using the assignment and not the copy ctor does not copy the db::Manager association.
    db::Shapes shape_copy;
    shape_copy.insert (shapes (src), types);
    shapes (dest).insert (shape_copy);
  }
}

void
Cell::move (unsigned int src, unsigned int dest)
{
  if (src != dest) {
    copy (src, dest);
    clear (src);
  }
}

void
Cell::move (unsigned int src, unsigned int dest, unsigned int types)
{
  if (src != dest) {
    copy (src, dest, types);
    clear (src, types);
  }
}

void
Cell::swap (unsigned int i1, unsigned int i2)
{
  if (i1 != i2) {

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new SwapLayerOp (i1, i2));
    }

    shapes (i1).swap (shapes (i2));
    m_bbox_needs_update = true;
  }
}

void 
Cell::sort_shapes ()
{
  for (shapes_map::iterator s = m_shapes_map.begin (); s != m_shapes_map.end (); ++s) {
    s->second.sort ();
  }
}

void
Cell::prop_id (db::properties_id_type id) 
{
  if (m_prop_id != id) {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new SetCellPropId (m_prop_id, id));
    }
    m_prop_id = id;
  }
}

const Cell::box_type &
Cell::bbox () const
{
  mp_layout->update ();
  return m_bbox;
}

const Cell::box_type &
Cell::bbox (unsigned int l) const
{
  mp_layout->update ();
  box_map::const_iterator b = m_bboxes.find (l);
  if (b != m_bboxes.end ()) {
    return b->second;
  } else {
    return ms_empty_box;
  }
}

Cell::const_iterator
Cell::begin () const
{
  mp_layout->update ();
  return m_instances.begin ();
}

Cell::overlapping_iterator 
Cell::begin_overlapping (const box_type &b) const
{
  mp_layout->update ();
  return m_instances.begin_overlapping (b, mp_layout);
}

Cell::touching_iterator 
Cell::begin_touching (const box_type &b) const
{
  mp_layout->update ();
  return m_instances.begin_touching (b, mp_layout);
}

Cell::parent_inst_iterator 
Cell::begin_parent_insts () const
{
  mp_layout->update ();
  return m_instances.begin_parent_insts (mp_layout);
}

Cell::child_cell_iterator 
Cell::begin_child_cells () const
{
  mp_layout->update ();
  return m_instances.begin_child_cells ();
}

size_t 
Cell::child_cells () const
{
  mp_layout->update ();
  return m_instances.child_cells ();
}

size_t 
Cell::parent_cells () const
{
  mp_layout->update ();
  return m_instances.parent_cells ();
}

Cell::parent_cell_iterator 
Cell::begin_parent_cells () const
{
  mp_layout->update ();
  return m_instances.begin_parent_cells ();
}

Cell::parent_cell_iterator 
Cell::end_parent_cells () const
{
  mp_layout->update ();
  return m_instances.end_parent_cells ();
}

bool 
Cell::is_top () const
{
  mp_layout->update ();
  return m_instances.is_top ();
}

bool 
Cell::is_leaf () const
{
  return m_instances.empty ();
}

unsigned int 
Cell::hierarchy_levels () const
{
  mp_layout->update ();
  return m_hier_levels;
}

static bool
has_shapes_touching_impl (const db::Cell &cell, unsigned int layer, const db::Box &box)
{
  if (! cell.shapes (layer).begin_touching (box, db::ShapeIterator::All).at_end ()) {
    return true;
  }

  for (db::Cell::touching_iterator i = cell.begin_touching (box); ! i.at_end (); ++i) {

    for (db::CellInstArray::iterator ia = i->cell_inst ().begin_touching (box, db::box_convert<db::CellInst> (*cell.layout (), layer)); ! ia.at_end (); ++ia) {

      db::Box cbox;
      if (i->is_complex ()) {
        cbox = i->complex_trans (*ia).inverted () * box;
      } else {
        cbox = (*ia).inverted () * box;
      }

      if (has_shapes_touching_impl (cell.layout ()->cell (i->cell_index ()), layer, cbox)) {
        return true;
      }

    }

  }

  return false;
}

bool
Cell::has_shapes_touching (unsigned int layer, const db::Box &box) const
{
  return has_shapes_touching_impl (*this, layer, box);
}

void 
Cell::collect_caller_cells (std::set<cell_index_type> &callers) const
{
  collect_caller_cells (callers, -1);
}

void 
Cell::collect_caller_cells (std::set<cell_index_type> &callers, const std::set<cell_index_type> &cone, int levels) const
{
  if (levels != 0) {
    for (parent_cell_iterator cc = begin_parent_cells (); cc != end_parent_cells (); ++cc) {
      if (cone.find (*cc) != cone.end () && callers.find (*cc) == callers.end () && mp_layout->is_valid_cell_index (*cc)) {
        callers.insert (*cc);
        mp_layout->cell (*cc).collect_caller_cells (callers, levels < 0 ? levels : levels - 1);
      }
    }
  }
}

void 
Cell::collect_caller_cells (std::set<cell_index_type> &callers, int levels) const
{
  if (levels != 0) {
    for (parent_cell_iterator cc = begin_parent_cells (); cc != end_parent_cells (); ++cc) {
      if (callers.find (*cc) == callers.end () && mp_layout->is_valid_cell_index (*cc)) {
        callers.insert (*cc);
        mp_layout->cell (*cc).collect_caller_cells (callers, levels < 0 ? levels : levels - 1);
      }
    }
  }
}

void 
Cell::collect_called_cells (std::set<cell_index_type> &called) const
{
  collect_called_cells (called, -1);
}

void 
Cell::collect_called_cells (std::set<cell_index_type> &called, int levels) const
{
  if (levels != 0) {
    for (child_cell_iterator cc = begin_child_cells (); ! cc.at_end (); ++cc) {
      if (called.find (*cc) == called.end () && mp_layout->is_valid_cell_index (*cc)) {
        called.insert (*cc);
        mp_layout->cell (*cc).collect_called_cells (called, levels < 0 ? levels : levels - 1);
      }
    }
  }
}

void 
Cell::invalidate_insts ()
{
  mp_layout->invalidate_hier ();  //  HINT: must come before the change is done!
  mp_layout->invalidate_bboxes (std::numeric_limits<unsigned int>::max ());
  m_bbox_needs_update = true;
}

void 
Cell::invalidate_hier ()
{
  mp_layout->invalidate_hier ();  //  HINT: must come before the change is done!
}

void 
Cell::redo (db::Op *op)
{
  db::CellOp *cell_op = dynamic_cast<db::CellOp *> (op);
  if (cell_op) {
    //  redo operation
    cell_op->redo (this);
  } else {
    //  other actions are only queued by the instance list - this is should be 
    //  responsible for the handling of the latter.
    //  HACK: this is not really a nice concept, but it saves us a pointer to the manager.
    m_instances.redo (op);
  }
}

void 
Cell::undo (db::Op *op) 
{
  db::CellOp *cell_op = dynamic_cast<db::CellOp *> (op);
  if (cell_op) {
    //  undo operation
    cell_op->undo (this);
  } else {
    //  other actions are only queued by the instance list - this is should be 
    //  responsible for the handling of the latter.
    //  HACK: this is not really a nice concept, but it saves us a pointer to the manager.
    m_instances.undo (op);
  }
}

void
Cell::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (! no_self) {
    stat->add (typeid (Cell), (void *) this, sizeof (Cell), sizeof (Cell), parent, purpose, cat);
  }
  db::mem_stat (stat, purpose, cat, m_bboxes, true, (void *) this);
  db::mem_stat (stat, MemStatistics::Instances, cat, m_instances, true, (void *) this);
  db::mem_stat (stat, MemStatistics::ShapesInfo, cat, m_shapes_map, true, (void *) this);
}

void 
Cell::clear_shapes_no_invalidate ()
{
  //  Hint: we can't simply clear the map because of the undo stack
  for (shapes_map::iterator s = m_shapes_map.begin (); s != m_shapes_map.end (); ++s) {
    s->second.clear ();
  }
  m_bbox_needs_update = true;
}

unsigned int 
Cell::count_hier_levels () const
{
  unsigned int l = 0;

  for (const_iterator c = begin (); !c.at_end (); ++c) {
    l = std::max (l, (unsigned int) mp_layout->cell (c->cell_index ()).m_hier_levels + 1);
  }

  return l;
}

void 
Cell::count_parent_insts (std::vector <size_t> &count) const
{
  m_instances.count_parent_insts (count);
}

void 
Cell::clear_parent_insts (size_t sz)
{
  m_instances.clear_parent_insts (sz);
}

void 
Cell::sort_child_insts ()
{
  m_instances.sort_child_insts (false);
}

std::pair<bool, db::pcell_id_type> 
Cell::is_pcell_instance (const instance_type &ref) const
{
  return mp_layout->is_pcell_instance (ref.cell_index ());
}

std::map<std::string, tl::Variant> 
Cell::get_named_pcell_parameters (const instance_type &ref) const
{
  return mp_layout->get_named_pcell_parameters (ref.cell_index ());
}

tl::Variant
Cell::get_pcell_parameter (const instance_type &ref, const std::string &name) const
{
  return mp_layout->get_pcell_parameter (ref.cell_index (), name);
}

const std::vector<tl::Variant> &
Cell::get_pcell_parameters (const instance_type &ref) const
{
  return mp_layout->get_pcell_parameters (ref.cell_index ());
}

Cell::instance_type 
Cell::change_pcell_parameters (const instance_type &ref, const std::vector<tl::Variant> &new_parameters)
{
  cell_index_type new_cell_index = mp_layout->get_pcell_variant_cell (ref.cell_index (), new_parameters);
  if (new_cell_index != ref.cell_index ()) {

    CellInstArray new_cell_inst (ref.cell_inst ());
    new_cell_inst.object () = db::CellInst (new_cell_index);

    return m_instances.replace (ref, new_cell_inst);

  } else {
    return ref;
  }
}

void 
Cell::sort_inst_tree (bool force)
{
  m_instances.sort_inst_tree (mp_layout, force);

  //  update the number of hierarchy levels
  m_hier_levels = count_hier_levels ();
}

std::string 
Cell::get_basic_name () const
{
  tl_assert (layout () != 0);
  return layout ()->cell_name (cell_index ());
}

std::string
Cell::get_qualified_name () const
{
  return get_basic_name ();
}

std::string 
Cell::get_display_name () const
{
  tl_assert (layout () != 0);
  if (is_ghost_cell () && empty ()) {
    return std::string ("(") + layout ()->cell_name (cell_index ()) + std::string (")");
  } else {
    return layout ()->cell_name (cell_index ());
  }
}

void
Cell::set_name (const std::string &name)
{
  tl_assert (layout () != 0);
  layout ()->rename_cell (cell_index (), name.c_str ());
}

void
Cell::copy_shapes (const db::Cell &source_cell, const db::LayerMapping &layer_mapping)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same cell")));
  }

  db::Layout *target_layout = layout ();
  if (! target_layout) {
    throw tl::Exception (tl::to_string (tr ("Cell does not reside in a layout")));
  }
  const db::Layout *source_layout = source_cell.layout ();
  if (! source_layout) {
    throw tl::Exception (tl::to_string (tr ("Source cell does not reside in a layout")));
  }

  if (target_layout != source_layout) {
    db::PropertyMapper pm (target_layout, source_layout);
    db::ICplxTrans trans (source_layout->dbu () / target_layout->dbu ());
    for (std::map<unsigned int, unsigned int>::const_iterator lm = layer_mapping.begin (); lm != layer_mapping.end (); ++lm) {
      shapes (lm->second).insert_transformed (source_cell.shapes (lm->first), trans, pm);
    }
  } else {
    for (std::map<unsigned int, unsigned int>::const_iterator lm = layer_mapping.begin (); lm != layer_mapping.end (); ++lm) {
      shapes (lm->second).insert (source_cell.shapes (lm->first));
    }
  }
}

void
Cell::copy_shapes (const db::Cell &source_cell)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same cell")));
  }
  db::Layout *target_layout = layout ();
  if (! target_layout) {
    throw tl::Exception (tl::to_string (tr ("Cell does not reside in a layout")));
  }

  if (target_layout != source_cell.layout ()) {
    if (! source_cell.layout ()) {
      throw tl::Exception (tl::to_string (tr ("Source cell does not reside in a layout")));
    }
    db::LayerMapping lm;
    lm.create_full (*target_layout, *source_cell.layout ());
    this->copy_shapes (source_cell, lm);
  } else {
    for (db::Layout::layer_iterator l = target_layout->begin_layers (); l != target_layout->end_layers (); ++l) {
      shapes ((*l).first).insert (source_cell.shapes ((*l).first));
    }
  }
}

void
Cell::copy_instances (const db::Cell &source_cell)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy instances within the same cell")));
  }
  if (layout () != source_cell.layout ()) {
    throw tl::Exception (tl::to_string (tr ("Cells do not reside in the same layout")));
  }

  for (db::Cell::const_iterator i = source_cell.begin (); ! i.at_end (); ++i) {
    insert (*i);
  }
}

std::vector<db::cell_index_type>
Cell::copy_tree (const db::Cell &source_cell)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same cell")));
  }

  db::Layout *target_layout = layout ();
  if (! target_layout) {
    throw tl::Exception (tl::to_string (tr ("Cell does not reside in a layout")));
  }
  const db::Layout *source_layout = source_cell.layout ();
  if (! source_layout) {
    throw tl::Exception (tl::to_string (tr ("Source cell does not reside in a layout")));
  }

  db::ICplxTrans trans (source_layout->dbu () / target_layout->dbu ());

  db::CellMapping cm;
  std::vector <db::cell_index_type> new_cells = cm.create_single_mapping_full (*target_layout, cell_index (), *source_layout, source_cell.cell_index ());

  db::LayerMapping lm;
  lm.create_full (*target_layout, *source_cell.layout ());

  std::vector <db::cell_index_type> source_cells;
  source_cells.push_back (source_cell.cell_index ());
  db::copy_shapes (*target_layout, *source_layout, trans, source_cells, cm.table (), lm.table ());

  return new_cells;
}

void
Cell::copy_tree_shapes (const db::Cell &source_cell, const db::CellMapping &cm)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same cell")));
  }

  db::Layout *target_layout = layout ();
  if (! target_layout) {
    throw tl::Exception (tl::to_string (tr ("Cell does not reside in a layout")));
  }
  const db::Layout *source_layout = source_cell.layout ();
  if (! source_layout) {
    throw tl::Exception (tl::to_string (tr ("Source cell does not reside in a layout")));
  }

  db::ICplxTrans trans (source_layout->dbu () / target_layout->dbu ());

  db::LayerMapping lm;
  lm.create_full (*target_layout, *source_cell.layout ());

  std::vector <db::cell_index_type> source_cells;
  source_cells.push_back (source_cell.cell_index ());
  db::copy_shapes (*target_layout, *source_layout, trans, source_cells, cm.table (), lm.table ());
}

void
Cell::copy_tree_shapes (const db::Cell &source_cell, const db::CellMapping &cm, const db::LayerMapping &lm)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same cell")));
  }

  db::Layout *target_layout = layout ();
  if (! target_layout) {
    throw tl::Exception (tl::to_string (tr ("Cell does not reside in a layout")));
  }
  const db::Layout *source_layout = source_cell.layout ();
  if (! source_layout) {
    throw tl::Exception (tl::to_string (tr ("Source cell does not reside in a layout")));
  }

  db::ICplxTrans trans (source_layout->dbu () / target_layout->dbu ());

  std::vector <db::cell_index_type> source_cells;
  source_cells.push_back (source_cell.cell_index ());
  db::copy_shapes (*target_layout, *source_layout, trans, source_cells, cm.table (), lm.table ());
}

void
Cell::move_shapes (db::Cell &source_cell, const db::LayerMapping &layer_mapping)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot move shapes within the same cell")));
  }

  db::Layout *target_layout = layout ();
  if (! target_layout) {
    throw tl::Exception (tl::to_string (tr ("Cell does not reside in a layout")));
  }
  db::Layout *source_layout = source_cell.layout ();
  if (! source_layout) {
    throw tl::Exception (tl::to_string (tr ("Source cell does not reside in a layout")));
  }

  if (target_layout != source_layout) {
    db::PropertyMapper pm (target_layout, source_layout);
    db::ICplxTrans trans (source_layout->dbu () / target_layout->dbu ());
    for (std::map<unsigned int, unsigned int>::const_iterator lm = layer_mapping.begin (); lm != layer_mapping.end (); ++lm) {
      shapes (lm->second).insert_transformed (source_cell.shapes (lm->first), trans, pm);
      source_cell.shapes (lm->first).clear ();
    }
  } else {
    for (std::map<unsigned int, unsigned int>::const_iterator lm = layer_mapping.begin (); lm != layer_mapping.end (); ++lm) {
      shapes (lm->second).insert (source_cell.shapes (lm->first));
      source_cell.shapes (lm->first).clear ();
    }
  }
}

void
Cell::move_shapes (db::Cell &source_cell)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot move shapes within the same cell")));
  }
  db::Layout *target_layout = layout ();
  if (! target_layout) {
    throw tl::Exception (tl::to_string (tr ("Cell does not reside in a layout")));
  }

  if (target_layout != source_cell.layout ()) {
    if (! source_cell.layout ()) {
      throw tl::Exception (tl::to_string (tr ("Source cell does not reside in a layout")));
    }
    db::LayerMapping lm;
    lm.create_full (*target_layout, *source_cell.layout ());
    move_shapes (source_cell, lm);
  } else {
    for (db::Layout::layer_iterator l = target_layout->begin_layers (); l != target_layout->end_layers (); ++l) {
      shapes ((*l).first).insert (source_cell.shapes ((*l).first));
      source_cell.shapes ((*l).first).clear ();
    }
  }
}

void
Cell::move_instances (db::Cell &source_cell)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot move instances within the same cell")));
  }
  if (layout () != source_cell.layout ()) {
    throw tl::Exception (tl::to_string (tr ("Cells do not reside in the same layout")));
  }

  for (db::Cell::const_iterator i = source_cell.begin (); ! i.at_end (); ++i) {
    insert (*i);
  }

  source_cell.clear_insts ();
}

std::vector<db::cell_index_type>
Cell::move_tree (db::Cell &source_cell)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot move shapes within the same cell")));
  }

  db::Layout *target_layout = layout ();
  if (! target_layout) {
    throw tl::Exception (tl::to_string (tr ("Cell does not reside in a layout")));
  }
  db::Layout *source_layout = source_cell.layout ();
  if (! source_layout) {
    throw tl::Exception (tl::to_string (tr ("Source cell does not reside in a layout")));
  }

  db::PropertyMapper pm (target_layout, source_layout);
  db::ICplxTrans trans (source_layout->dbu () / target_layout->dbu ());

  db::CellMapping cm;
  std::vector <db::cell_index_type> new_cells = cm.create_single_mapping_full (*target_layout, cell_index (), *source_layout, source_cell.cell_index ());

  db::LayerMapping lm;
  lm.create_full (*target_layout, *source_cell.layout ());

  std::vector <db::cell_index_type> source_cells;
  source_cells.push_back (source_cell.cell_index ());
  db::move_shapes (*target_layout, *source_layout, trans, source_cells, cm.table (), lm.table ());

  source_layout->prune_subcells (source_cell.cell_index ());

  return new_cells;
}

void
Cell::move_tree_shapes (db::Cell &source_cell, const db::CellMapping &cm)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot move shapes within the same cell")));
  }

  db::Layout *target_layout = layout ();
  if (! target_layout) {
    throw tl::Exception (tl::to_string (tr ("Cell does not reside in a layout")));
  }
  db::Layout *source_layout = source_cell.layout ();
  if (! source_layout) {
    throw tl::Exception (tl::to_string (tr ("Source cell does not reside in a layout")));
  }

  db::PropertyMapper pm (target_layout, source_layout);
  db::ICplxTrans trans (source_layout->dbu () / target_layout->dbu ());

  db::LayerMapping lm;
  lm.create_full (*target_layout, *source_cell.layout ());

  std::vector <db::cell_index_type> source_cells;
  source_cells.push_back (source_cell.cell_index ());
  db::move_shapes (*target_layout, *source_layout, trans, source_cells, cm.table (), lm.table ());
}

void
Cell::move_tree_shapes (db::Cell &source_cell, const db::CellMapping &cm, const db::LayerMapping &lm)
{
  if (this == &source_cell) {
    throw tl::Exception (tl::to_string (tr ("Cannot move shapes within the same cell")));
  }

  db::Layout *target_layout = layout ();
  if (! target_layout) {
    throw tl::Exception (tl::to_string (tr ("Cell does not reside in a layout")));
  }
  db::Layout *source_layout = source_cell.layout ();
  if (! source_layout) {
    throw tl::Exception (tl::to_string (tr ("Source cell does not reside in a layout")));
  }

  db::PropertyMapper pm (target_layout, source_layout);
  db::ICplxTrans trans (source_layout->dbu () / target_layout->dbu ());

  std::vector <db::cell_index_type> source_cells;
  source_cells.push_back (source_cell.cell_index ());
  db::move_shapes (*target_layout, *source_layout, trans, source_cells, cm.table (), lm.table ());
}

}


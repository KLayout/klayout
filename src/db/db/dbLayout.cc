
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#include "dbLayout.h"
#include "dbMemStatistics.h"
#include "dbTrans.h"
#include "dbShapeRepository.h"
#include "dbPCellHeader.h"
#include "dbPCellVariant.h"
#include "dbPCellDeclaration.h"
#include "dbLibraryProxy.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "dbRegion.h"
#include "tlTimer.h"
#include "tlLog.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlAssert.h"


namespace db
{

static const int layout_base_verbosity = 30;

// -----------------------------------------------------------------
//  The undo/redo operations

struct LayoutOp
  : public db::Op
{
  LayoutOp () { }
  virtual ~LayoutOp () { }

  virtual void redo (db::Layout *) const = 0;
  virtual void undo (db::Layout *) const = 0;
};

struct SetLayoutPropId
  : public LayoutOp
{
  SetLayoutPropId (db::properties_id_type f, db::properties_id_type t)
    : m_from (f), m_to (t)
  { }

  virtual void redo (db::Layout *layout) const
  {
    layout->prop_id (m_to);
  }

  virtual void undo (db::Layout *layout) const
  {
    layout->prop_id (m_from);
  }

private:
  db::properties_id_type m_from, m_to;
};

struct SetLayoutDBU
  : public LayoutOp
{
  SetLayoutDBU (double f, double t)
    : m_from (f), m_to (t)
  { }

  virtual void redo (db::Layout *layout) const
  {
    layout->dbu (m_to);
  }

  virtual void undo (db::Layout *layout) const
  {
    layout->dbu (m_from);
  }

private:
  double m_from, m_to;
};

struct RenameCellOp
  : public LayoutOp
{
  RenameCellOp (db::cell_index_type i, const std::string &f, const std::string &t)
    : m_cell_index (i), m_from (f), m_to (t)
  { }

  virtual void redo (db::Layout *layout) const
  {
    layout->rename_cell (m_cell_index, m_to.c_str ());
  }

  virtual void undo (db::Layout *layout) const
  {
    layout->rename_cell (m_cell_index, m_from.c_str ());
  }

private:
  db::cell_index_type m_cell_index;
  std::string m_from, m_to;
};

struct NewRemoveCellOp
  : public LayoutOp
{
  NewRemoveCellOp (db::cell_index_type i, const std::string &name, bool remove, db::Cell *cell)
    : m_cell_index (i), m_name (name), m_remove (remove), mp_cell (cell)
  { }

  ~NewRemoveCellOp ()
  {
    if (mp_cell) {
      delete mp_cell;
    }
  }

  virtual void redo (db::Layout *layout) const
  {
    if (m_remove) {
      remove_cell (layout);
    } else {
      new_cell (layout);
    }
  }

  virtual void undo (db::Layout *layout) const
  {
    if (m_remove) {
      new_cell (layout);
    } else {
      remove_cell (layout);
    }
  }

private:
  db::cell_index_type m_cell_index;
  std::string m_name;
  bool m_remove;
  mutable db::Cell *mp_cell;

  virtual void new_cell (db::Layout *layout) const
  {
    tl_assert (mp_cell != 0);
    layout->insert_cell (m_cell_index, m_name, mp_cell);
    mp_cell = 0; // now it belongs to the layout
  }

  virtual void remove_cell (db::Layout *layout) const
  {
    tl_assert (mp_cell == 0);
    mp_cell = layout->take_cell (m_cell_index);
  }
};

struct SetLayerPropertiesOp
  : public LayoutOp
{
  SetLayerPropertiesOp (unsigned int l, const db::LayerProperties &new_props, const db::LayerProperties &old_props)
    : m_layer_index (l), m_new_props (new_props), m_old_props (old_props)
  { }

  virtual void redo (db::Layout *layout) const
  {
    layout->set_properties (m_layer_index, m_new_props);
  }

  virtual void undo (db::Layout *layout) const
  {
    layout->set_properties (m_layer_index, m_old_props);
  }

private:
  unsigned int m_layer_index;
  db::LayerProperties m_new_props, m_old_props;
};

struct InsertRemoveLayerOp
  : public LayoutOp
{
  InsertRemoveLayerOp (unsigned int l, const db::LayerProperties &props, bool insert)
    : m_layer_index (l), m_props (props), m_insert (insert)
  { }

  virtual void redo (db::Layout *layout) const
  {
    if (m_insert) {
      layout->insert_layer (m_layer_index, m_props);
    } else {
      layout->delete_layer (m_layer_index);
    }
  }

  virtual void undo (db::Layout *layout) const
  {
    if (! m_insert) {
      layout->insert_layer (m_layer_index, m_props);
    } else {
      layout->delete_layer (m_layer_index);
    }
  }

private:
  unsigned int m_layer_index;
  db::LayerProperties m_props;
  bool m_insert;
};

// -----------------------------------------------------------------
//  Implementation of the LayerIterator class

LayerIterator::LayerIterator (unsigned int layer_index, const db::Layout &layout)
  : m_layer_index (layer_index), m_layout (layout)
{
  while (m_layer_index < m_layout.layers () && ! m_layout.is_valid_layer (m_layer_index)) {
    ++m_layer_index;
  }
}

LayerIterator &
LayerIterator::operator++() 
{
  do {
    ++m_layer_index;
  } while (m_layer_index < m_layout.layers () && ! m_layout.is_valid_layer (m_layer_index));

  return *this;
}

std::pair<unsigned int, const db::LayerProperties *> 
LayerIterator::operator*() const
{
  return std::pair<unsigned int, const db::LayerProperties *> (m_layer_index, &m_layout.get_properties (m_layer_index));
}

// -----------------------------------------------------------------
//  Implementation of the Layout class

Layout::Layout (db::Manager *manager)
  : db::Object (manager),
    m_cells_size (0),
    m_invalid (0),
    m_top_cells (0),
    m_dbu (0.001),
    m_prop_id (0),
    m_properties_repository (this),
    m_guiding_shape_layer (-1),
    m_waste_layer (-1),
    m_editable (db::default_editable_mode ())
{
  // .. nothing yet ..
}

Layout::Layout (bool editable, db::Manager *manager)
  : db::Object (manager),
    m_cells_size (0),
    m_invalid (0),
    m_top_cells (0),
    m_dbu (0.001),
    m_prop_id (0),
    m_properties_repository (this),
    m_guiding_shape_layer (-1),
    m_waste_layer (-1),
    m_editable (editable)
{
  // .. nothing yet ..
}

Layout::Layout (const db::Layout &layout)
  : db::Object (layout),
    db::LayoutStateModel (),
    gsi::ObjectBase (),
    m_cells_size (0),
    m_invalid (0),
    m_top_cells (0),
    m_dbu (0.001),
    m_prop_id (0),
    m_properties_repository (this),
    m_guiding_shape_layer (-1),
    m_waste_layer (-1),
    m_editable (layout.m_editable)
{
  *this = layout;
}

Layout::~Layout ()
{
  //  since it the cell graph (or the derived layout) might produce some transactions that refer to 
  //  this object, we need to clear the manager's transaction list before the cell graph is deleted.
  if (manager ()) {
    manager ()->clear ();
  }

  clear ();
}

void
Layout::dbu (double d)
{
  if (fabs (d - m_dbu)) {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new SetLayoutDBU (m_dbu, d));
    }
    m_dbu = d;
    dbu_changed ();
  }
}

void
Layout::clear ()
{
  invalidate_hier ();

  m_free_cell_indices.clear ();
  m_cells.clear ();
  m_cells_size = 0;
  m_cell_ptrs.clear ();

  m_top_down_list.clear ();

  m_free_indices.clear ();
  m_layer_states.clear ();

  for (std::vector<const char *>::const_iterator p = m_cell_names.begin (); p != m_cell_names.end (); ++p) {
    if (*p) {
      delete [] *p;
    }
  }
  m_cell_names.clear ();
  m_cell_map.clear ();

  m_shape_repository = db::GenericRepository ();
  db::PropertiesRepository empty_pr (this);
  m_properties_repository = empty_pr;
  m_array_repository = db::ArrayRepository ();

  for (std::vector<pcell_header_type *>::const_iterator pc = m_pcells.begin (); pc != m_pcells.end (); ++pc) {
    delete *pc;
  }
  m_pcells.clear ();
  m_pcell_ids.clear ();

  m_guiding_shape_layer = -1;
  m_waste_layer = -1;

  m_lib_proxy_map.clear ();
  m_meta_info.clear ();
}

Layout &
Layout::operator= (const Layout &d)
{
  if (&d != this) {

    db::LayoutStateModel::operator= (d);

    clear ();

    m_guiding_shape_layer = d.m_guiding_shape_layer;
    m_waste_layer = d.m_waste_layer;
    m_editable = d.m_editable;

    m_pcell_ids = d.m_pcell_ids;
    m_pcells.reserve (d.m_pcells.size ());

    for (std::vector<pcell_header_type *>::const_iterator pc = d.m_pcells.begin (); pc != d.m_pcells.end (); ++pc) {
      if (*pc) {
        m_pcells.push_back (new pcell_header_type (**pc));
      } else {
        m_pcells.push_back (0);
      }
    }

    m_lib_proxy_map = d.m_lib_proxy_map;

    m_cell_ptrs.resize (d.m_cell_ptrs.size (), 0);

    for (const_iterator c = d.begin (); c != d.end (); ++c) {
      cell_type *new_cell = (*c).clone (*this);
      m_cells.push_back_ptr (new_cell);
      ++m_cells_size;
      m_cell_ptrs [new_cell->cell_index ()] = new_cell;
    }

    m_properties_repository = d.m_properties_repository; // because the cell assign operator does not map property ID's ..
    m_free_indices = d.m_free_indices;
    m_layer_states = d.m_layer_states;
    m_layer_props = d.m_layer_props;
    m_top_down_list = d.m_top_down_list;
    m_top_cells = d.m_top_cells;

    m_cell_names.reserve (d.m_cell_names.size ());

    cell_index_type i = 0;
    for (std::vector<const char *>::const_iterator p = d.m_cell_names.begin (); p != d.m_cell_names.end (); ++p) {
      if (*p) {
        char *pp = new char [strlen (*p) + 1];
        strcpy (pp, *p);
        m_cell_names.push_back (pp);
        m_cell_map.insert (std::make_pair (pp, i));
      } else {
        m_cell_names.push_back (0);
      }
      ++i;
    }

    m_dbu = d.m_dbu;
    m_meta_info = d.m_meta_info;

  }
  return *this;
}

void
Layout::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (!no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }

  db::mem_stat (stat, purpose, cat, m_cell_ptrs, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_free_cell_indices, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_top_down_list, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_free_indices, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_layer_states, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_cell_names, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_cell_map, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_layer_props, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_pcells, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_pcell_ids, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_lib_proxy_map, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_meta_info, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_string_repository, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_shape_repository, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_properties_repository, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_array_repository, true, (void *) this);

  for (std::vector<const char *>::const_iterator i = m_cell_names.begin (); i != m_cell_names.end (); ++i) {
    stat->add (typeid (char []), (void *) *i, strlen (*i) + 1, strlen (*i) + 1, (void *) this, purpose, cat);
  }
  for (cell_list::const_iterator i = m_cells.begin (); i != m_cells.end (); ++i) {
    db::mem_stat (stat, MemStatistics::CellInfo, int (i->id ()), *i, false, (void *) this);
  }
  for (std::vector<pcell_header_type *>::const_iterator i = m_pcells.begin (); i != m_pcells.end (); ++i) {
    db::mem_stat (stat, MemStatistics::CellInfo, 0, **i, false, (void *) this);
  }
}

void
Layout::prop_id (db::properties_id_type id) 
{
  if (m_prop_id != id) {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new SetLayoutPropId (m_prop_id, id));
    }
    m_prop_id = id;
  }
}

bool 
Layout::has_cell (const char *name)
{
  return m_cell_map.find (name) != m_cell_map.end ();
}

std::pair<bool, cell_index_type> 
Layout::cell_by_name (const char *name) const
{
  cell_map_type::const_iterator c = m_cell_map.find (name);
  if (c != m_cell_map.end ()) {
    return std::make_pair (true, c->second);
  } else {
    return std::make_pair (false, 0);
  }
}

const char *
Layout::cell_name (cell_index_type index) const
{
  tl_assert (index < m_cell_names.size ());
  return m_cell_names [index];
}

void 
Layout::delete_cells (const std::set<cell_index_type> &cells_to_delete)
{
  //  Collect parent cells
  std::set <cell_index_type> pcs;
  for (std::set<cell_index_type>::const_iterator c = cells_to_delete.begin (); c != cells_to_delete.end (); ++c) {
    const db::Cell &cref = cell (*c);
    for (db::Cell::parent_cell_iterator pc = cref.begin_parent_cells (); pc != cref.end_parent_cells (); ++pc) {
      pcs.insert (*pc);
    }
  }

  //  Clear all instances
  for (std::set<cell_index_type>::const_iterator c = cells_to_delete.begin (); c != cells_to_delete.end (); ++c) {

    db::Cell &cref = cell (*c);

    cref.clear_insts ();

    //  If transacting, do not use clear_shapes here. This will delete all the shapes containers will
    //  will disable us saving undo data with reference to them.
    if (manager () && manager ()->transacting ()) {
      for (unsigned int i = 0; i < layers (); ++i) {
        if (is_valid_layer (i)) {
          cref.clear (i);
        }
      }
    } else {
      cref.clear_shapes ();
    }

  }

  //  delete all instances of this cell

  std::vector <db::Instance> insts_to_delete;
  for (std::set <cell_index_type>::const_iterator pc = pcs.begin (); pc != pcs.end (); ++pc) {

    db::Cell &parent_cref = cell (*pc);

    insts_to_delete.clear ();
    for (db::Cell::const_iterator pci = parent_cref.begin (); ! pci.at_end (); ++pci) {
      if (cells_to_delete.find (pci->cell_index ()) != cells_to_delete.end ()) {
        insts_to_delete.push_back (*pci);
      }
    }

    std::sort (insts_to_delete.begin (), insts_to_delete.end ());

    parent_cref.erase_insts (insts_to_delete);

  }

  //  erase the cells themselves
  //  If transacting, the cell is not deleted yet. Instead, the transaction object acts as
  //  a backup container for the cell. This is necessary since the ID's within manager are given to
  //  cell child objects that must remain.
  for (std::set<cell_index_type>::const_iterator c = cells_to_delete.begin (); c != cells_to_delete.end (); ++c) {

    if (manager () && manager ()->transacting ()) {
       
      //  note the "take" method - this takes out the cell
      manager ()->queue (this, new NewRemoveCellOp (*c, cell_name (*c), true /*remove*/, take_cell (*c)));

    } else {

      //  remove the cell - we use take_cell and delete to avoid recursion issues
      delete take_cell (*c);

    }

  }
}

void 
Layout::delete_cell (cell_index_type id)
{
  db::Cell &cref = cell (id);

  std::vector <cell_index_type> pcs;
  for (db::Cell::parent_cell_iterator pc = cref.begin_parent_cells (); pc != cref.end_parent_cells (); ++pc) {
    pcs.push_back (*pc);
  }

  cref.clear_insts ();

  //  If transacting, do not use clear_shapes here. This will delete all the shapes containers will
  //  will disable us saving undo data with reference to them.
  if (manager () && manager ()->transacting ()) {
    for (unsigned int i = 0; i < layers (); ++i) {
      if (is_valid_layer (i)) {
        cref.clear (i);
      }
    }
  } else {
    cref.clear_shapes ();
  }

  //  delete all instances of this cell

  std::vector <db::Instance> insts_to_delete;
  for (std::vector <cell_index_type>::const_iterator pc = pcs.begin (); pc != pcs.end (); ++pc) {

    if (is_valid_cell_index (*pc)) {

      db::Cell &parent_cref = cell (*pc);

      insts_to_delete.clear ();
      for (db::Cell::const_iterator pci = parent_cref.begin (); ! pci.at_end (); ++pci) {
        if (pci->cell_index () == id) {
          insts_to_delete.push_back (*pci);
        }
      }

      std::sort (insts_to_delete.begin (), insts_to_delete.end ());

      parent_cref.erase_insts (insts_to_delete);

    }

  }

  //  erase the cell itself
  //  If transacting, the cell is not deleted yet. Instead, the transaction object acts as
  //  a backup container for the cell. This is necessary since the ID's within manager are given to
  //  cell child objects that must remain.

  if (manager () && manager ()->transacting ()) {
     
    //  not the "take" method - this takes out the cell
    manager ()->queue (this, new NewRemoveCellOp (id, cell_name (id), true /*remove*/, take_cell (id)));

  } else {

    //  remove the cell - we use take_cell and delete to avoid recursion issues
    delete take_cell (id);

  }
}

void
Layout::insert (db::cell_index_type cell, int layer, const db::Region &region)
{
  region.insert_into (this, cell, layer);
}

void
Layout::insert (db::cell_index_type cell, int layer, const db::Edges &edges)
{
  edges.insert_into (this, cell, layer);
}

void
Layout::insert (db::cell_index_type cell, int layer, const db::EdgePairs &edge_pairs)
{
  edge_pairs.insert_into (this, cell, layer);
}

void
Layout::flatten (const db::Cell &source_cell, db::Cell &target_cell, const db::ICplxTrans &t, int levels)
{
  db::ICplxTrans tt = t;

  if (&source_cell != &target_cell) {

    unsigned int nlayers = layers ();
    for (unsigned int l = 0; l < nlayers; ++l) {

      if (is_valid_layer (l)) {

        db::Shapes &target_shapes = target_cell.shapes (l);
        const db::Shapes &source_shapes = source_cell.shapes (l);

        tl::ident_map<db::Layout::properties_id_type> pm1;
        for (db::Shapes::shape_iterator sh = source_shapes.begin (db::ShapeIterator::All); ! sh.at_end (); ++sh) {
          target_shapes.insert (*sh, tt, pm1);
        }

      }

    }

  }

  if (levels == 0) {

    if (&source_cell != &target_cell) {
      for (db::Cell::const_iterator inst = source_cell.begin (); ! inst.at_end (); ++inst) {
        db::Instance new_inst = target_cell.insert (*inst);
        target_cell.transform (new_inst, tt);
      }
    }

  } else if (&target_cell == &source_cell) {

    update ();

    try {

      //  Note: suppressing the update speeds up the flatten process considerably since 
      //  even an iteration of the instances requires an update.
      start_changes ();

      db::Instances old_instances (0);
      old_instances = target_cell.instances ();
      target_cell.clear_insts ();

      for (db::Cell::const_iterator inst = old_instances.begin (); ! inst.at_end (); ++inst) {

        db::CellInstArray cell_inst = inst->cell_inst ();

        for (db::CellInstArray::iterator a = cell_inst.begin (); ! a.at_end (); ++a) {
          db::ICplxTrans tinst = t * cell_inst.complex_trans (*a);
          flatten (cell (cell_inst.object ().cell_index ()), target_cell, tinst, levels < 0 ? levels : levels - 1);
        }

      }

      end_changes ();

    } catch (...) {
      end_changes ();
      throw;
    }

  } else {
    
    try {

      //  Note: suppressing the update speeds up the flatten process considerably since 
      //  even an iteration of the instances requires an update.
      start_changes ();

      for (db::Cell::const_iterator inst = source_cell.begin (); ! inst.at_end (); ++inst) {

        db::CellInstArray cell_inst = inst->cell_inst ();

        for (db::CellInstArray::iterator a = cell_inst.begin (); ! a.at_end (); ++a) {
          db::ICplxTrans tinst = t * cell_inst.complex_trans (*a);
          flatten (cell (cell_inst.object ().cell_index ()), target_cell, tinst, levels < 0 ? levels : levels - 1);
        }

      }

      end_changes ();

    } catch (...) {
      end_changes ();
      throw;
    }

  }

}

void 
Layout::flatten (db::Cell &cell_to_flatten, int levels, bool prune) 
{
  std::set<db::cell_index_type> direct_children;
  if (prune) {
    //  save direct children
    cell_to_flatten.collect_called_cells (direct_children, 1);
  }

  flatten (cell_to_flatten, cell_to_flatten, db::ICplxTrans (), levels);

  if (prune) {

    //  determine all direct children that are orphans now.
    for (std::set<db::cell_index_type>::iterator dc = direct_children.begin (); dc != direct_children.end (); ) {
      std::set<db::cell_index_type>::iterator dc_next = dc;
      ++dc_next;
      if (cell (*dc).parent_cells () != 0) {
        direct_children.erase (dc);
      }
      dc = dc_next;
    }

    //  and prune them
    prune_cells (direct_children.begin (), direct_children.end (), levels - 1);

  }
}

void 
Layout::prune_cell (cell_index_type id, int levels)
{
  do_prune_cell_or_subcell (id, levels, false);
}

void 
Layout::prune_subcells (cell_index_type id, int levels)
{
  do_prune_cell_or_subcell (id, levels, true);
}

void 
Layout::do_prune_cell_or_subcell (cell_index_type id, int levels, bool subcells)
{
  db::Cell &cref = cell (id);

  //  collect the called cells
  std::set <cell_index_type> called;
  cref.collect_called_cells (called, levels);
  called.insert (id);

  //  From these cells erase all cells that have parents outside the subtree of our cell.
  //  Make sure this is done recursively by doing this top-down.
  for (top_down_iterator c = begin_top_down (); c != end_top_down (); ++c) {
    if (called.find (*c) != called.end () && *c != id) {
      db::Cell &ccref = cell (*c);
      for (db::Cell::parent_cell_iterator pc = ccref.begin_parent_cells (); pc != ccref.end_parent_cells (); ++pc) {
        if (called.find (*pc) == called.end ()) {
          //  we have a parent outside the subset considered currently (either the cell was never in or
          //  it was removed itself already): remove this cell from the set of valid subcells.
          called.erase (*c);
          break;
        }
      }
    }
  }

  //  order the called cells bottom-up 
  std::vector <cell_index_type> cells_to_delete;
  cells_to_delete.reserve (called.size ());
  for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
    if (called.find (*c) != called.end () && (!subcells || *c != id)) {
      cells_to_delete.push_back (*c);
    }
  }

  //  and delete these cells
  delete_cells (cells_to_delete.begin (), cells_to_delete.end ());

  //  erase all instances in the subcells case (because, by definition we don't have any more instances)
  if (subcells) {
    cref.clear_insts ();
  }
}

void 
Layout::prune_cells (const std::set<cell_index_type> &ids, int levels)
{
  do_prune_cells_or_subcells (ids, levels, false);
}

void 
Layout::prune_subcells (const std::set<cell_index_type> &ids, int levels)
{
  do_prune_cells_or_subcells (ids, levels, true);
}

void 
Layout::do_prune_cells_or_subcells (const std::set<cell_index_type> &ids, int levels, bool subcells)
{
  //  collect the called cells
  std::set <cell_index_type> called;
  for (std::set<cell_index_type>::const_iterator id = ids.begin (); id != ids.end (); ++id) {
    db::Cell &cref = cell (*id);
    cref.collect_called_cells (called, levels);
  }
  called.insert (ids.begin (), ids.end ());

  //  From these cells erase all cells that have parents outside the subtree of our cell.
  //  Make sure this is done recursively by doing this top-down.
  for (top_down_iterator c = begin_top_down (); c != end_top_down (); ++c) {
    if (called.find (*c) != called.end () && ids.find (*c) == ids.end ()) {
      db::Cell &ccref = cell (*c);
      for (db::Cell::parent_cell_iterator pc = ccref.begin_parent_cells (); pc != ccref.end_parent_cells (); ++pc) {
        if (called.find (*pc) == called.end ()) {
          //  we have a parent outside the subset considered currently (either the cell was never in or
          //  it was removed itself already): remove this cell from the set of valid subcells.
          called.erase (*c);
          break;
        }
      }
    }
  }

  //  order the called cells bottom-up 
  std::vector <cell_index_type> cells_to_delete;
  cells_to_delete.reserve (called.size ());
  for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
    if (called.find (*c) != called.end () && (!subcells || ids.find (*c) == ids.end ())) {
      cells_to_delete.push_back (*c);
    }
  }

  //  and delete these cells
  delete_cells (cells_to_delete.begin (), cells_to_delete.end ());

  //  erase all instances in the subcells case (because, by definition we don't have any more instances)
  if (subcells) {
    for (std::set<cell_index_type>::const_iterator id = ids.begin (); id != ids.end (); ++id) {
      db::Cell &cref = cell (*id);
      cref.clear_insts ();
    }
  }
}

void 
Layout::delete_cell_rec (cell_index_type id)
{
  db::Cell &cref = cell (id);

  //  collect the called cells
  std::set <cell_index_type> called;
  cref.collect_called_cells (called);
  called.insert (id);

  //  order the called cells bottom-up
  std::vector <cell_index_type> cells_to_delete;
  cells_to_delete.reserve (called.size ());
  for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
    if (called.find (*c) != called.end ()) {
      cells_to_delete.push_back (*c);
    }
  }

  //  and delete these cells
  delete_cells (cells_to_delete.begin (), cells_to_delete.end ());
}

void
Layout::insert_cell (cell_index_type ci, const std::string &name, db::Cell *cell)
{
  //  this method is supposed to restore a cell deleted before
  tl_assert (m_cell_names.size () > ci);
  tl_assert (m_cell_names [ci] == 0);

  char *cp = new char [name.size () + 1];
  m_cell_names [ci] = cp;
  strcpy (cp, name.c_str ());

  invalidate_hier ();

  m_cells.push_back_ptr (cell);
  m_cell_ptrs [ci] = cell;
  m_cell_map.insert (std::make_pair (cp, ci));

  cell->reregister ();
  ++m_cells_size;
}

db::Cell *
Layout::take_cell (cell_index_type ci)
{
  tl_assert (m_cell_ptrs [ci] != 0);

  invalidate_hier ();

  cell_type *cell = m_cells.take (iterator (m_cell_ptrs [ci]));
  cell->unregister ();
  --m_cells_size;

  m_cell_ptrs [ci] = 0;

  //  Using free cell indices does have one significant drawback:
  //  The cellview references cannot be uniquely classified as being invalid - because the
  //  ID might be reused. This causes problems, when a cell is being deleted and subsequently a
  //  cell is being created - a crash occures. Therefore the free index feature is disabled.
  //  If this causes memory consumption problems, it should be considered to use a map and
  //  an arbitrary ID.
  // m_free_cell_indices.push_back (ci);

  if (m_cell_names [ci] != 0) {

    cell_map_type::iterator cm = m_cell_map.find (m_cell_names [ci]);
    if (cm != m_cell_map.end ()) {
      m_cell_map.erase (cm);
    }

    delete [] m_cell_names [ci];
    m_cell_names [ci] = 0;

  }

  return cell;
}

std::string 
Layout::uniquify_cell_name (const char *name) const
{
  if (name != 0 && m_cell_map.find (name) == m_cell_map.end ()) {
    return std::string (name);
  } else {

    std::string b;

    //  if the cell does not have a valid name yet, create a unique one.
    unsigned int j = 0;
    for (unsigned int m = 0x40000000; m > 0; m >>= 1) {
      j += m;
      b = std::string (name ? name : "") + "$" + tl::to_string (j);
      if (m_cell_map.find (b.c_str ()) == m_cell_map.end ()) {
        j -= m;
      }
    } 

    b = std::string (name ? name : "") + "$" + tl::to_string (j + 1);
    return b;

  }
}

cell_index_type 
Layout::add_cell (const char *name)
{
  std::string b;

  if (name == 0) {

    //  0 name means: create a new one
    b = uniquify_cell_name (0);
    name = b.c_str ();

  } else {

    cell_map_type::const_iterator cm = m_cell_map.find (name);
    if (cm != m_cell_map.end ()) {

      const db::Cell &c= cell (cm->second);
      if (c.is_ghost_cell () && c.empty ()) {
        //  ghost cells are available as new cells - the idea is to 
        //  treat them as non-existing.
        return cm->second;
      } else {
        //  create a unique name
        b = uniquify_cell_name (name);
        name = b.c_str ();
      }

    }

  }

  //  create a new cell 
  cell_index_type new_index = allocate_new_cell ();

  cell_type *new_cell = new cell_type (new_index, *this);
  m_cells.push_back_ptr (new_cell);
  m_cell_ptrs [new_index] = new_cell;

  //  enter it's index and cell_name
  register_cell_name (name, new_index);

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new NewRemoveCellOp (new_index, m_cell_names [new_index], false /*new*/, 0));
  }

  return new_index;
}

void 
Layout::register_cell_name (const char *name, cell_index_type ci)
{
  //  enter it's index and cell_name
  char *cp;

  cp = new char [strlen (name) + 1];
  strcpy (cp, name);

  while (m_cell_names.size () < ci) {
    char *e = new char [1];
    *e = 0;
    m_cell_names.push_back (e);
  }

  if (m_cell_names.size () > ci) {
    delete [] m_cell_names [ci];
    m_cell_names [ci] = cp;
  } else {
    m_cell_names.push_back (cp);
  }

  m_cell_map.insert (std::make_pair (cp, ci));
}

void
Layout::rename_cell (cell_index_type id, const char *name)
{
  tl_assert (id < m_cell_names.size ());

  if (strcmp (m_cell_names [id], name) != 0) {

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new RenameCellOp (id, m_cell_names [id], name));
    }

    m_cell_map.erase (m_cell_names [id]);

    char *cp = new char [strlen (name) + 1];
    strcpy (cp, name);

    delete [] m_cell_names [id];
    m_cell_names [id] = cp;

    m_cell_map.insert (std::make_pair (cp, id));

    //  to enforce a redraw and a rebuild
    cell_name_changed ();

  }
}

bool 
Layout::topological_sort ()
{
  m_top_cells = 0;
  m_top_down_list.clear ();
  m_top_down_list.reserve (m_cells_size);

  std::vector<size_t> num_parents (m_cell_ptrs.size (), 0);

  //  while there are cells to treat ..
  while (m_top_down_list.size () != m_cells_size) {

    size_t n_top_down_cells = m_top_down_list.size ();

    //  Treat all cells that do not have all parents reported.
    //  For all such a cells, disable the parent counting, 
    //  add the cell's index to the top-down sorted list and
    //  increment the reported parent instance count in all the
    //  child cells.

    for (const_iterator c = begin (); c != end (); ++c) {
      if (c->parent_cells () == num_parents [c->cell_index ()]) {
        m_top_down_list.push_back (c->cell_index ());
        num_parents [c->cell_index ()] = std::numeric_limits<cell_index_type>::max ();
      }
    }

    //  For all these a cells, increment the reported parent instance 
    //  count in all the child cells.
    for (cell_index_vector::const_iterator ii = m_top_down_list.begin () + n_top_down_cells; ii != m_top_down_list.end (); ++ii) {
      for (cell_type::child_cell_iterator cc = cell (*ii).begin_child_cells (); ! cc.at_end (); ++cc) {
        tl_assert (num_parents [*cc] != std::numeric_limits<cell_index_type>::max ());
        num_parents [*cc] += 1;
      }
    }

    //  If no new cells have been reported this is basically a 
    //  sign of recursion in the graph.
    if (n_top_down_cells == m_top_down_list.size ()) {
      return false;
    }
 
  }

  //  Determine the number of top cells
  for (top_down_iterator e = m_top_down_list.begin (); e != m_top_down_list.end () && cell (*e).is_top (); ++e) {
    ++m_top_cells;
  }

  //  The cell graph is fine.
  return true;

}

bool 
Layout::is_valid_cell_index (cell_index_type ci) const
{
  return ci < m_cell_ptrs.size () && m_cell_ptrs [ci] != 0;
}

cell_index_type
Layout::allocate_new_cell ()
{
  invalidate_hier ();

  cell_index_type new_index;
  if (m_free_cell_indices.empty ()) {
    new_index = cell_index_type (m_cell_ptrs.size ());
    m_cell_ptrs.push_back (0);
  } else {
    new_index = m_free_cell_indices.back ();
    m_free_cell_indices.pop_back ();
  }

  ++m_cells_size;

  return new_index;
}

void
Layout::cleanup ()
{
  //  deleting cells may create new top cells which need to be deleted as well, hence we iterate
  //  until there are no more cells to delete
  while (true) {

    //  delete all cells that are top cells and are proxies. Those cells are proxies no longer required.
    std::set<cell_index_type> cells_to_delete;
    for (top_down_iterator c = begin_top_down (); c != end_top_cells (); ++c) {
      if (cell (*c).is_proxy ()) {
        cells_to_delete.insert (*c);
      }
    }

    if (cells_to_delete.empty ()) {
      break;
    }

    delete_cells (cells_to_delete);

  }
}

void 
Layout::update_relations ()
{
  for (iterator c = begin (); c != end (); ++c) {
    c->sort_child_insts ();
  }

  std::vector <size_t> parent_insts (cells (), 0);
  for (const_iterator c = begin (); c != end (); ++c) {
    c->count_parent_insts (parent_insts);
  }
  std::vector <size_t>::const_iterator n = parent_insts.begin ();
  for (iterator c = begin (); c != end (); ++c, ++n) {
    c->clear_parent_insts (*n);
  }
  for (iterator c = begin (); c != end (); ++c) {
    c->update_relations ();
  }
}

Layout::top_down_iterator 
Layout::end_top_cells () 
{
  update ();
  return m_top_down_list.begin () + m_top_cells;
}

Layout::top_down_const_iterator 
Layout::end_top_cells () const
{
  update ();
  return m_top_down_list.begin () + m_top_cells;
}

void 
Layout::force_update () 
{
  if (hier_dirty () || bboxes_dirty ()) {

    unsigned int invalid = m_invalid;

    try {

      m_invalid = std::numeric_limits<unsigned int>::max ();   //  prevent recursion

      db::LayoutStateModel *state_model = const_cast<db::LayoutStateModel *> ((const db::LayoutStateModel *) this);
      state_model->update ();

      m_invalid = invalid;

    } catch (...) {
      m_invalid = invalid;
      throw;
    }

  }
}

void 
Layout::update () const
{
  if (! under_construction () && (hier_dirty () || bboxes_dirty ())) {

    try {

      m_invalid = std::numeric_limits<unsigned int>::max ();   //  prevent recursion

      db::LayoutStateModel *state_model = const_cast<db::LayoutStateModel *> ((const db::LayoutStateModel *) this);
      state_model->update ();

      m_invalid = 0;

    } catch (...) {
      m_invalid = 0;
      throw;
    }

  }
}

void 
Layout::do_update ()
{
  tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity, tl::to_string (tr ("Sorting")));

  //  establish a progress report since this operation can take some time.
  //  HINT: because of some gcc bug, automatic destruction of the tl::Progress
  //  object does not work. We overcome this problem by creating the object with new 
  //  and catching exceptions.
  tl::RelativeProgress *pr = new tl::RelativeProgress (tl::to_string (tr ("Sorting layout")), m_cells_size, 1000);
  pr->set_desc ("");

  try {

    //  if the hierarchy has been changed so far, update
    //  the hierarchy management information
    if (hier_dirty ()) {
      {
        tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity + 10, "Updating relations");
        pr->set_desc (tl::to_string (tr ("Updating relations")));
        update_relations ();
      } 
      {
        tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity + 10, "Topological sort");
        pr->set_desc (tl::to_string (tr ("Topological sorting")));
        tl_assert (topological_sort ());
      }
    }

    //  KLUDGE: a boolean vector (with size as determined by number of cells)
    //  would probably be much faster!
    std::set<cell_index_type> dirty_parents;

    //  if something on the bboxes (either on shape level or on 
    //  cell bbox level - i.e. by child instances) has been changed,
    //  update the bbox information. In addition sort the shapes
    //  lists of region queries, since they might have changed once
    //  the bboxes are dirty.
    if (bboxes_dirty ()) {

      {
        tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity + 10, "Updating bounding boxes");
        unsigned int layers = 0;
        pr->set (0);
        pr->set_desc (tl::to_string (tr ("Updating bounding boxes")));
        for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
          ++*pr;
          cell_type &cp (cell (*c));
          if (cp.is_shape_bbox_dirty () || dirty_parents.find (*c) != dirty_parents.end ()) {
            if (cp.update_bbox (layers)) {
              //  the bounding box has changed - need to insert parents into "dirty parents" list
              for (cell_type::parent_cell_iterator p = cp.begin_parent_cells (); p != cp.end_parent_cells (); ++p) {
                dirty_parents.insert (*p);
              }
            } 
          }
          if (cp.layers () > layers) {
            layers = cp.layers ();
          }
        }
      }

      {
        tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity + 10, "Sorting shapes");
        pr->set (0);
        pr->set_desc (tl::to_string (tr ("Sorting shapes")));
        for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
          ++*pr;
          cell_type &cp (cell (*c));
          cp.sort_shapes ();
        }
      }
    }

    //  sort the instance trees now, since we have computed the bboxes
    if (hier_dirty () || ! dirty_parents.empty ()) {
      tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity + 10, "Sorting instances");
      size_t layers = 0;
      pr->set (0);
      pr->set_desc (tl::to_string (tr ("Sorting instances")));
      for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
        ++*pr;
        cell_type &cp (cell (*c));
        if (hier_dirty () || dirty_parents.find (*c) != dirty_parents.end ()) {
          cp.sort_inst_tree ();
        }
        if (cp.layers () > layers) {
          layers = cp.layers ();
        }
      }
    }

  } catch (...) {
    delete pr;
    throw;
  }

  delete pr;
}

void
Layout::clear_meta ()
{
  m_meta_info.clear ();
}

void
Layout::add_meta_info (const MetaInfo &i)
{
  for (meta_info::iterator m = m_meta_info.begin (); m != m_meta_info.end (); ++m) {
    if (m->name == i.name) {
      *m = i;
      return;
    }
  }
  m_meta_info.push_back (i);
}

void
Layout::remove_meta_info (const std::string &name)
{
  for (meta_info::iterator m = m_meta_info.begin (); m != m_meta_info.end (); ++m) {
    if (m->name == name) {
      m_meta_info.erase (m);
      return;
    }
  }
}

const std::string &
Layout::meta_info_value (const std::string &name) const
{
  for (meta_info::const_iterator m = m_meta_info.begin (); m != m_meta_info.end (); ++m) {
    if (m->name == name) {
      return m->value;
    }
  }

  static const std::string s_empty;
  return s_empty;
}

void 
Layout::swap_layers (unsigned int a, unsigned int b)
{
  tl_assert (a < layers () && m_layer_states [a] != Free);
  tl_assert (b < layers () && m_layer_states [b] != Free);

  //  clear the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->swap (a, b);
  }
}

void 
Layout::move_layer (unsigned int src, unsigned int dest)
{
  tl_assert (src < layers () && m_layer_states [src] != Free);
  tl_assert (dest < layers () && m_layer_states [dest] != Free);

  //  move the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->move (src, dest);
  }
}

void 
Layout::copy_layer (unsigned int src, unsigned int dest)
{
  tl_assert (src < layers () && m_layer_states [src] != Free);
  tl_assert (dest < layers () && m_layer_states [dest] != Free);

  //  copy the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->copy (src, dest);
  }
}

void 
Layout::clear_layer (unsigned int n)
{
  tl_assert (n < layers () && m_layer_states [n] != Free);

  //  clear the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->clear (n);
  }
}

void 
Layout::delete_layer (unsigned int n)
{
  tl_assert (n < layers () && m_layer_states [n] != Free);

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new InsertRemoveLayerOp (n, m_layer_props [n], false /*delete*/));
  }

  m_free_indices.push_back (n);
  m_layer_states [n] = Free;

  //  clear the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->clear (n);
  }

  layer_properties_changed ();
}

unsigned int 
Layout::insert_layer (const LayerProperties &props)
{
  unsigned int i = do_insert_layer ();
  while (m_layer_props.size () <= i) {
    m_layer_props.push_back (LayerProperties ());
  }
  m_layer_props [i] = props;

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new InsertRemoveLayerOp (i, props, true/*insert*/));
  }

  layer_properties_changed ();

  return i;
}

void 
Layout::insert_layer (unsigned int index, const LayerProperties &props)
{
  do_insert_layer (index);
  while (m_layer_props.size () <= index) {
    m_layer_props.push_back (LayerProperties ());
  }
  m_layer_props [index] = props;

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new InsertRemoveLayerOp (index, props, true/*insert*/));
  }

  layer_properties_changed ();
}

unsigned int
Layout::get_layer (const db::LayerProperties &lp)
{
  if (lp.is_null ()) {
    //  for a null layer info always create a layer
    return insert_layer ();
  } else {
    //  if we have a layer with the requested properties already, return this.
    for (db::Layout::layer_iterator li = begin_layers (); li != end_layers (); ++li) {
      if ((*li).second->log_equal (lp)) {
        return (*li).first;
      }
    }
    //  otherwise create a new layer
    return insert_layer (lp);
  }
}

unsigned int
Layout::waste_layer () const
{
  if (m_waste_layer < 0) {
    //  create the waste layer (since that layer is cached we can do
    //  this in a "const" fashion.
    db::Layout *self = const_cast<db::Layout *> (this);
    self->m_waste_layer = (int) self->insert_special_layer (db::LayerProperties ("WASTE"));
  }

  return (unsigned int) m_waste_layer;
}

unsigned int
Layout::guiding_shape_layer () const
{
  if (m_guiding_shape_layer < 0) {
    //  create the guiding shape layer (since that layer is cached we can do
    //  this in a "const" fashion.
    db::Layout *self = const_cast<db::Layout *> (this);
    self->m_guiding_shape_layer = (int) self->insert_special_layer (db::LayerProperties ("GUIDING_SHAPES"));
  }

  return (unsigned int) m_guiding_shape_layer;
}

unsigned int 
Layout::insert_special_layer (const LayerProperties &props)
{
  unsigned int i = do_insert_layer (true /*special*/);
  while (m_layer_props.size () <= i) {
    m_layer_props.push_back (LayerProperties ());
  }
  m_layer_props [i] = props;

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new InsertRemoveLayerOp (i, props, true/*insert*/));
  }

  return i;
}

void 
Layout::set_properties (unsigned int i, const LayerProperties &props)
{
  if (m_layer_props [i] != props) {

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new SetLayerPropertiesOp (i, props, m_layer_props [i]));
    }

    m_layer_props [i] = props;

    layer_properties_changed ();

  }
}

void 
Layout::insert_special_layer (unsigned int index, const LayerProperties &props)
{
  do_insert_layer (index, true /*special*/);
  while (m_layer_props.size () <= index) {
    m_layer_props.push_back (LayerProperties ());
  }
  m_layer_props [index] = props;

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new InsertRemoveLayerOp (index, props, true/*insert*/));
  }
}

unsigned int 
Layout::do_insert_layer (bool special) 
{
  if (m_free_indices.size () > 0) {
    unsigned int i = m_free_indices.back ();
    m_free_indices.pop_back ();
    m_layer_states [i] = special ? Special : Normal;
    return i;
  } else {
    m_layer_states.push_back (special ? Special : Normal);
    unsigned int i = layers () - 1;
    return i;
  }
}

void 
Layout::do_insert_layer (unsigned int index, bool special) 
{
  if (index >= layers ()) {

    //  add layer to the end of the list.
    //  add as may freelist entries as required.
    while (index > layers ()) {
      m_free_indices.push_back (layers ());
      m_layer_states.push_back (Free);
    }
    m_layer_states.push_back (special ? Special : Normal);

  } else {

    tl_assert (m_layer_states [index] == Free);
    m_layer_states [index] = special ? Special : Normal;
  
  }

}

void 
Layout::reserve_layers (unsigned int n)
{
  m_layer_states.reserve (n);
}

static const std::vector<tl::Variant> &gauge_parameters (const std::vector<tl::Variant> &p, const db::PCellDeclaration *pcell_decl, std::vector<tl::Variant> &buffer)
{
  const std::vector<db::PCellParameterDeclaration> &pcp = pcell_decl->parameter_declarations ();

  if (pcp.size () > p.size ()) {

    buffer.clear ();
    buffer.resize (pcp.size ());
    buffer = p;
    for (std::vector<PCellParameterDeclaration>::const_iterator i = pcp.begin () + p.size (); i != pcp.end (); ++i) {
      buffer.push_back (i->get_default ());
    }
    return buffer;

  } else if (pcp.size () < p.size ()) {

    buffer.clear ();
    buffer.insert (buffer.end (), p.begin (), p.begin () + pcp.size ());
    return buffer;

  } else {
    return p;
  }
}

void 
Layout::get_pcell_variant_as (pcell_id_type pcell_id, const std::vector<tl::Variant> &p, cell_index_type target_cell_index, ImportLayerMapping *layer_mapping)
{
  pcell_header_type *header = pcell_header (pcell_id);
  tl_assert (header != 0);

  std::vector<tl::Variant> buffer;
  const std::vector<tl::Variant> &parameters = gauge_parameters (p, header->declaration (), buffer);

  //  this variant must not exist yet for "get as" semantics
  tl_assert (header->get_variant (*this, parameters) == 0);

  tl_assert (! (manager () && manager ()->transacting ()));
  tl_assert (m_cell_ptrs [target_cell_index] != 0);
 
  invalidate_hier ();

  m_cells.erase (iterator (m_cell_ptrs [target_cell_index]));

  pcell_variant_type *variant = new pcell_variant_type (target_cell_index, *this, pcell_id, parameters);
  m_cells.push_back_ptr (variant);
  m_cell_ptrs [target_cell_index] = variant;

  // produce the layout
  variant->update (layer_mapping);
}

cell_index_type 
Layout::get_pcell_variant_dict (pcell_id_type pcell_id, const std::map<std::string, tl::Variant> &p)
{
  pcell_header_type *header = pcell_header (pcell_id);
  tl_assert (header != 0);

  std::vector<tl::Variant> parameters;
  const std::vector<db::PCellParameterDeclaration> &pcp = header->declaration ()->parameter_declarations ();
  parameters.reserve (pcp.size ());
  for (std::vector<db::PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end(); ++pd) {
    std::map<std::string, tl::Variant>::const_iterator pp = p.find (pd->get_name ());
    if (pp == p.end ()) {
      parameters.push_back (pd->get_default ());
    } else {
      parameters.push_back (pp->second);
    }
  }

  pcell_variant_type *variant = header->get_variant (*this, parameters);
  if (! variant) {

    std::string b (header->get_name ());
    if (m_cell_map.find (b.c_str ()) != m_cell_map.end ()) {
      b = uniquify_cell_name (b.c_str ());
    }

    //  create a new cell 
    cell_index_type new_index = allocate_new_cell ();

    variant = new pcell_variant_type (new_index, *this, pcell_id, parameters);
    m_cells.push_back_ptr (variant);
    m_cell_ptrs [new_index] = variant;

    //  enter it's index and cell_name
    register_cell_name (b.c_str (), new_index);

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new NewRemoveCellOp (new_index, m_cell_names [new_index], false /*new*/, 0));
    }

    // produce the layout
    variant->update ();

  }

  return variant->cell_index ();
}

cell_index_type 
Layout::get_pcell_variant (pcell_id_type pcell_id, const std::vector<tl::Variant> &p)
{
  pcell_header_type *header = pcell_header (pcell_id);
  tl_assert (header != 0);

  std::vector<tl::Variant> buffer;
  const std::vector<tl::Variant> &parameters = gauge_parameters (p, header->declaration (), buffer);

  pcell_variant_type *variant = header->get_variant (*this, parameters);
  if (! variant) {

    std::string b (header->get_name ());
    if (m_cell_map.find (b.c_str ()) != m_cell_map.end ()) {
      b = uniquify_cell_name (b.c_str ());
    }

    //  create a new cell 
    cell_index_type new_index = allocate_new_cell ();

    variant = new pcell_variant_type (new_index, *this, pcell_id, parameters);
    m_cells.push_back_ptr (variant);
    m_cell_ptrs [new_index] = variant;

    //  enter it's index and cell_name
    register_cell_name (b.c_str (), new_index);

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new NewRemoveCellOp (new_index, m_cell_names [new_index], false /*new*/, 0));
    }

    // produce the layout
    variant->update ();

  }

  return variant->cell_index ();
}

const Layout::pcell_header_type *
Layout::pcell_header (pcell_id_type pcell_id) const
{
  return (const_cast<db::Layout *> (this))->pcell_header (pcell_id);
}

Layout::pcell_header_type *
Layout::pcell_header (pcell_id_type pcell_id)
{
  if (pcell_id >= m_pcells.size ()) {
    return 0;
  } else {
    return m_pcells [pcell_id];
  }
}

std::pair<bool, pcell_id_type>
Layout::pcell_by_name (const char *name) const
{
  std::map<std::string, pcell_id_type>::const_iterator pcid = m_pcell_ids.find (std::string (name));
  if (pcid != m_pcell_ids.end ()) {
    return std::make_pair (true, pcid->second);
  } else {
    return std::make_pair (false, pcell_id_type (0));
  }
}

pcell_id_type
Layout::register_pcell (const std::string &name, pcell_declaration_type *declaration)
{
  //  No undo/redo support for PCell registration. The interactions with PCell variants
  //  (for which undo/redo support is available) is too complex ...
  tl_assert (!manager () || !manager ()->transacting ());

  pcell_id_type id;

  pcell_name_map::const_iterator pcid = m_pcell_ids.find (name);
  if (pcid != m_pcell_ids.end ()) {

    //  replace any existing PCell declaration with that name.
    id = pcid->second;
    if (m_pcells [id]) {
      delete m_pcells [id];
    }

    m_pcells [id] = new pcell_header_type (id, name, declaration);

  } else {

    id = m_pcells.size ();
    m_pcells.push_back (new pcell_header_type (id, name, declaration));
    m_pcell_ids.insert (std::make_pair (std::string (name), id));

  }

  declaration->m_id = id;
  declaration->m_name = name;

  //  marks this object being held by the layout
  declaration->keep ();

  return id;
}

const Layout::pcell_declaration_type *
Layout::pcell_declaration (pcell_id_type pcell_id) const
{
  const pcell_header_type *header = pcell_header (pcell_id);
  return header ? header->declaration () : 0;
}

db::cell_index_type 
Layout::convert_cell_to_static (db::cell_index_type ci)
{
  tl_assert (is_valid_cell_index (ci));
  db::cell_index_type ret_ci = ci;

  if (dynamic_cast<const LibraryProxy *> (m_cell_ptrs [ci]) || dynamic_cast<const PCellVariant *> (m_cell_ptrs [ci])) {

    invalidate_hier ();

    const cell_type &org_cell = cell (ci);

    //  Note: convert to static cell by explicitly cloning to the db::Cell class
    ret_ci = add_cell (org_cell.get_basic_name ().c_str ());
    cell_type &new_cell = cell (ret_ci);
    new_cell = org_cell;
    new_cell.set_cell_index (ret_ci);

    //  remove guiding shapes.
    if (m_guiding_shape_layer >= 0) {
      new_cell.shapes (m_guiding_shape_layer).clear ();
    }

  }

  return ret_ci;
}

std::pair<bool, db::pcell_id_type> 
Layout::is_pcell_instance (cell_index_type cell_index) const
{
  const Cell *child_cell = &cell (cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
  if (lib_proxy) {
    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    tl_assert (lib != 0);
    return lib->layout ().is_pcell_instance (lib_proxy->library_cell_index ());
  }

  const PCellVariant *pcell_variant = dynamic_cast<const PCellVariant *> (child_cell);
  if (pcell_variant) {
    return std::make_pair (true, pcell_variant->pcell_id ());
  } else {
    return std::make_pair (false, db::pcell_id_type(0));
  }
}

const Layout::pcell_declaration_type *
Layout::pcell_declaration_for_pcell_variant (cell_index_type variant_cell_index) const
{
  const Cell *variant_cell = &cell (variant_cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (variant_cell);
  if (lib_proxy) {
    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    tl_assert (lib != 0);
    return lib->layout ().pcell_declaration_for_pcell_variant (lib_proxy->library_cell_index ());
  }

  const PCellVariant *pcell_variant = dynamic_cast<const PCellVariant *> (variant_cell);
  if (pcell_variant) {
    return pcell_declaration (pcell_variant->pcell_id ());
  } else {
    return 0;
  }
}

std::pair<db::Library *, db::cell_index_type>
Layout::defining_library (cell_index_type cell_index) const
{
  const db::Layout *layout = this;
  db::Library *lib = 0;

  while (true) {

    const Cell *child_cell = &layout->cell (cell_index);
    const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
    if (lib_proxy) {

      lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
      tl_assert (lib != 0);
      cell_index = lib_proxy->library_cell_index ();
      layout = &lib->layout ();

    } else {
      return std::pair<db::Library *, db::cell_index_type> (lib, cell_index);
    }

  }
}

tl::Variant
Layout::get_pcell_parameter (cell_index_type cell_index, const std::string &name) const
{
  const Cell *child_cell = &cell (cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
  if (lib_proxy) {
    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    tl_assert (lib != 0);
    return lib->layout ().get_pcell_parameter (lib_proxy->library_cell_index (), name);
  }

  const PCellVariant *pcell_variant = dynamic_cast<const PCellVariant *> (child_cell);
  if (pcell_variant) {
    return pcell_variant->parameter_by_name (name);
  } else {
    static std::map<std::string, tl::Variant> empty;
    return empty;
  }
}

std::map<std::string, tl::Variant>
Layout::get_named_pcell_parameters (cell_index_type cell_index) const
{
  const Cell *child_cell = &cell (cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
  if (lib_proxy) {
    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    tl_assert (lib != 0);
    return lib->layout ().get_named_pcell_parameters (lib_proxy->library_cell_index ());
  }

  const PCellVariant *pcell_variant = dynamic_cast<const PCellVariant *> (child_cell);
  if (pcell_variant) {
    return pcell_variant->parameters_by_name ();
  } else {
    static std::map<std::string, tl::Variant> empty;
    return empty;
  }
}

const std::vector<tl::Variant> &
Layout::get_pcell_parameters (cell_index_type cell_index) const
{
  const Cell *child_cell = &cell (cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
  if (lib_proxy) {
    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    tl_assert (lib != 0);
    return lib->layout ().get_pcell_parameters (lib_proxy->library_cell_index ());
  }

  const PCellVariant *pcell_variant = dynamic_cast<const PCellVariant *> (child_cell);
  if (pcell_variant) {
    return pcell_variant->parameters ();
  } else {
    static std::vector<tl::Variant> empty;
    return empty;
  }
}

cell_index_type
Layout::get_pcell_variant_cell (cell_index_type cell_index, const std::vector<tl::Variant> &new_parameters)
{
  Cell *child_cell = &cell (cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
  if (lib_proxy) {

    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    cell_index_type new_lib_cell_index = lib->layout ().get_pcell_variant_cell (lib_proxy->library_cell_index (), new_parameters);

    if (new_lib_cell_index != lib_proxy->library_cell_index ()) {
      return get_lib_proxy (lib, new_lib_cell_index);
    }

  } else {

    PCellVariant *pcell_variant = dynamic_cast<PCellVariant *> (child_cell);
    if (pcell_variant) {
      return get_pcell_variant (pcell_variant->pcell_id (), new_parameters);
    } 

  }

  return cell_index;

}

bool 
Layout::get_context_info (cell_index_type cell_index, std::vector <std::string> &context_info) const
{
  const db::Cell *cptr = &cell (cell_index);
  const db::Layout *ly = this;

  const db::LibraryProxy *lib_proxy;
  while (ly != 0 && (lib_proxy = dynamic_cast <const db::LibraryProxy *> (cptr)) != 0) {

    const db::Library *lib = db::LibraryManager::instance ().lib (lib_proxy->lib_id ());
    if (! lib) {
      return false; //  abort
    } else {

      //  one level of library indirection
      ly = &lib->layout ();
      cptr = &ly->cell (lib_proxy->library_cell_index ());
      context_info.push_back ("LIB=" + lib->get_name ());

    }

  }

  const db::PCellVariant *pcell_variant = dynamic_cast <const db::PCellVariant *> (cptr);
  if (pcell_variant) {
    
    const db::PCellDeclaration *pcell_decl = ly->pcell_declaration (pcell_variant->pcell_id ());

    const std::vector<db::PCellParameterDeclaration> &pcp = pcell_decl->parameter_declarations ();
    std::vector<db::PCellParameterDeclaration>::const_iterator pd = pcp.begin ();
    for (std::vector<tl::Variant>::const_iterator p = pcell_variant->parameters ().begin (); p != pcell_variant->parameters ().end () && pd != pcp.end (); ++p, ++pd) {
      context_info.push_back ("P(" + tl::to_word_or_quoted_string (pd->get_name ()) + ")=" + p->to_parsable_string ());
    }

    const db::PCellHeader *header = ly->pcell_header (pcell_variant->pcell_id ());
    context_info.push_back ("PCELL=" + header->get_name ());

  } else {
    context_info.push_back ("CELL=" + std::string (ly->cell_name (cptr->cell_index ())));
  }

  return true;
}

bool
Layout::recover_proxy_as (cell_index_type cell_index, std::vector <std::string>::const_iterator from, std::vector <std::string>::const_iterator to, ImportLayerMapping *layer_mapping)
{
  if (from == to) {
    return false;
  }

  tl::Extractor ex (from->c_str ());

  if (ex.test ("LIB=")) {

    std::string lib_name = ex.skip ();
    Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (lib_name);
    if (! lib) {
      return false;
    }

    db::Cell *lib_cell = lib->layout ().recover_proxy (from + 1, to);
    if (lib_cell) {
      get_lib_proxy_as (lib, lib_cell->cell_index (), cell_index, layer_mapping);
      return true;
    }

  } else {

    std::map<std::string, tl::Variant> parameters;

    while (from != to && (ex = tl::Extractor (from->c_str ())).test ("P(")) {

      std::string name;
      ex.read_word_or_quoted (name);
      ex.test (")");
      ex.test ("=");

      ex.read (parameters.insert (std::make_pair (name, tl::Variant ())).first->second);

      ++from;

    }

    if (ex.test ("PCELL=")) {

      std::pair<bool, pcell_id_type> pc = pcell_by_name (ex.skip ());
      if (pc.first) {
        get_pcell_variant_as (pc.second, pcell_declaration (pc.second)->map_parameters (parameters), cell_index, layer_mapping);
        return true;
      }

    } else if (ex.test ("CELL=")) {

      //  This should not happen. A cell (given by the cell index) cannot be proxy to another cell in the same layout.
      tl_assert (false);

    } 

  }

  return false;
}

db::Cell *
Layout::recover_proxy (std::vector <std::string>::const_iterator from, std::vector <std::string>::const_iterator to)
{
  if (from == to) {
    return 0;
  }

  tl::Extractor ex (from->c_str ());

  if (ex.test ("LIB=")) {

    std::string lib_name = ex.skip ();
    Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (lib_name);
    if (! lib) {
      return 0;
    }

    db::Cell *lib_cell = lib->layout ().recover_proxy (from + 1, to);
    if (lib_cell) {
      cell_index_type cell_index = get_lib_proxy (lib, lib_cell->cell_index ());
      return &cell (cell_index);
    }

  } else {

    std::map<std::string, tl::Variant> parameters;

    while (from != to && (ex = tl::Extractor (from->c_str ())).test ("P(")) {

      std::string name;
      ex.read_word_or_quoted (name);
      ex.test (")");
      ex.test ("=");

      ex.read (parameters.insert (std::make_pair (name, tl::Variant ())).first->second);

      ++from;

    }

    if (ex.test ("PCELL=")) {

      std::pair<bool, pcell_id_type> pc = pcell_by_name (ex.skip ());
      if (pc.first) {
        cell_index_type cell_index = get_pcell_variant (pc.second, pcell_declaration (pc.second)->map_parameters (parameters));
        return &cell (cell_index);
      }

    } else if (ex.test ("CELL=")) {

      std::pair<bool, cell_index_type> cc = cell_by_name (ex.skip ());
      if (cc.first) {
        return &cell (cc.second);
      }

    } 

  }

  return 0;
}

std::string 
Layout::display_name (cell_index_type cell_index) const
{
  return cell (cell_index).get_display_name ();
}

std::string 
Layout::basic_name (cell_index_type cell_index) const
{
  return cell (cell_index).get_basic_name ();
}

void
Layout::register_lib_proxy (db::LibraryProxy *lib_proxy)
{
  m_lib_proxy_map.insert (std::make_pair (std::make_pair (lib_proxy->lib_id (), lib_proxy->library_cell_index ()), lib_proxy->Cell::cell_index ()));
}

void
Layout::unregister_lib_proxy (db::LibraryProxy *lib_proxy)
{
  m_lib_proxy_map.erase (std::make_pair (lib_proxy->lib_id (), lib_proxy->library_cell_index ()));
}

void
Layout::get_lib_proxy_as (Library *lib, cell_index_type cell_index, cell_index_type target_cell_index, ImportLayerMapping *layer_mapping)
{
  tl_assert (! (manager () && manager ()->transacting ()));
  tl_assert (m_cell_ptrs [target_cell_index] != 0);
 
  invalidate_hier ();

  m_cells.erase (iterator (m_cell_ptrs [target_cell_index]));

  LibraryProxy *proxy = new LibraryProxy (target_cell_index, *this, lib->get_id (), cell_index);
  m_cells.push_back_ptr (proxy);
  m_cell_ptrs [target_cell_index] = proxy;

  // produce the layout
  proxy->update (layer_mapping);
}

cell_index_type
Layout::get_lib_proxy (Library *lib, cell_index_type cell_index)
{
  lib_proxy_map::const_iterator lp = m_lib_proxy_map.find (std::make_pair (lib->get_id (), cell_index));
  if (lp != m_lib_proxy_map.end ()) {
    return lp->second;
  } else {

    //  create a new unique name
    std::string b (lib->layout ().basic_name (cell_index));
    if (m_cell_map.find (b.c_str ()) != m_cell_map.end ()) {
      b = uniquify_cell_name (b.c_str ());
    }

    //  create a new cell (a LibraryProxy)
    cell_index_type new_index = allocate_new_cell ();

    LibraryProxy *proxy = new LibraryProxy (new_index, *this, lib->get_id (), cell_index);
    m_cells.push_back_ptr (proxy);
    m_cell_ptrs [new_index] = proxy;

    //  enter it's index and cell_name
    register_cell_name (b.c_str (), new_index);

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new NewRemoveCellOp (new_index, m_cell_names [new_index], false /*new*/, 0));
    }

    // produce the layout
    proxy->update ();

    return new_index;

  }
}

void
Layout::redo (db::Op *op)
{
  const LayoutOp *layout_op = dynamic_cast <const LayoutOp *> (op);
  if (layout_op) {
    layout_op->redo (this);
  }
}

void 
Layout::undo (db::Op *op)
{
  const LayoutOp *layout_op = dynamic_cast <const LayoutOp *> (op);
  if (layout_op) {
    layout_op->undo (this);
  }
}

}


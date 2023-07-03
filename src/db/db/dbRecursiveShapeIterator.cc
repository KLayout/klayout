
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

#include "dbRecursiveShapeIterator.h"
#include "dbRegion.h"
#include "dbEdgeProcessor.h"
#include "tlProgress.h"

namespace db
{

// ------------------------------------------------------------------------------------
//  Recursive shape iterator implementation

RecursiveShapeIterator::RecursiveShapeIterator (const RecursiveShapeIterator &d)
{
  operator= (d);
}

RecursiveShapeIterator &RecursiveShapeIterator::operator= (const RecursiveShapeIterator &d)
{
  if (&d != this) {

    m_layers = d.m_layers;
    m_has_layers = d.m_has_layers;
    m_max_depth = d.m_max_depth;
    m_min_depth = d.m_min_depth;
    m_shape_flags = d.m_shape_flags;
    mp_shape_prop_sel = d.mp_shape_prop_sel;
    m_shape_inv_prop_sel = d.m_shape_inv_prop_sel;
    m_overlapping = d.m_overlapping;
    m_start = d.m_start;
    m_stop = d.m_stop;

    mp_layout = d.mp_layout;
    mp_top_cell = d.mp_top_cell;
    mp_shapes = d.mp_shapes;

    m_region = d.m_region;
    if (d.mp_complex_region.get () != 0) {
      mp_complex_region.reset (new region_type (*d.mp_complex_region.get ()));
    } else {
      mp_complex_region.reset (0);
    }

    m_box_convert = d.m_box_convert;

    m_inst = d.m_inst;
    m_inst_array = d.m_inst_array;
    m_empty_cells_cache = d.m_empty_cells_cache;
    m_layer = d.m_layer;
    mp_cell = d.mp_cell;
    m_current_layer = d.m_current_layer;
    m_shape = d.m_shape;
    m_trans = d.m_trans;
    m_global_trans = d.m_global_trans;
    m_property_translator = d.m_property_translator;
    m_trans_stack = d.m_trans_stack;
    m_inst_iterators = d.m_inst_iterators;
    m_inst_array_iterators = d.m_inst_array_iterators;
    m_cells = d.m_cells;
    m_local_complex_region_stack = d.m_local_complex_region_stack;
    m_local_region_stack = d.m_local_region_stack;
    m_needs_reinit = d.m_needs_reinit;
    m_inst_quad_id = d.m_inst_quad_id;
    m_inst_quad_id_stack = d.m_inst_quad_id_stack;
    m_shape_quad_id = d.m_shape_quad_id;

  }
  return *this;
}

RecursiveShapeIterator::RecursiveShapeIterator ()
{
  //  anything. Not necessary reasonable.
  m_layer = 0;
  m_has_layers = false;
  mp_shapes = 0;
  mp_top_cell = 0;
  mp_cell = 0;
  m_current_layer = 0;
  m_overlapping = false;
  m_max_depth = std::numeric_limits<int>::max (); // all
  m_min_depth = 0;
  m_shape_flags = shape_iterator::All;
  mp_shape_prop_sel = 0;
  m_shape_inv_prop_sel = false;
  m_needs_reinit = false;
  m_inst_quad_id = 0;
  m_shape_quad_id = 0;
}

RecursiveShapeIterator::RecursiveShapeIterator (const shapes_type &shapes)
{
  m_layer = 0;
  m_has_layers = false;
  mp_shapes = &shapes;
  mp_top_cell = 0;
  m_overlapping = false;
  init ();
  init_region (box_type::world ());
}

RecursiveShapeIterator::RecursiveShapeIterator (const shapes_type &shapes, const box_type &region, bool overlapping)
{
  m_layer = 0;
  m_has_layers = false;
  mp_shapes = &shapes;
  mp_top_cell = 0;
  m_overlapping = overlapping;
  init ();
  init_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const shapes_type &shapes, const region_type &region, bool overlapping)
{
  m_layer = 0;
  m_has_layers = false;
  mp_shapes = &shapes;
  mp_top_cell = 0;
  m_overlapping = overlapping;
  init ();
  init_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, unsigned int layer, const box_type &region, bool overlapping)
  : m_box_convert (layout, layer)
{
  m_layer = layer;
  m_has_layers = false;
  mp_layout.reset (const_cast<db::Layout *> (&layout));
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, unsigned int layer, const region_type &region, bool overlapping)
  : m_box_convert (layout, layer)
{
  m_layer = layer;
  m_has_layers = false;
  mp_layout.reset (const_cast<db::Layout *> (&layout));
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, unsigned int layer)
  : m_box_convert (layout, layer)
{
  m_layer = layer;
  m_has_layers = false;
  mp_layout.reset (const_cast<db::Layout *> (&layout));
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = false;
  init ();
  init_region (box_type::world ());
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers, const box_type &region, bool overlapping)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers = layers;
  m_has_layers = true;
  mp_layout.reset (const_cast<db::Layout *> (&layout));
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers, const region_type &region, bool overlapping)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers = layers;
  m_has_layers = true;
  mp_layout.reset (const_cast<db::Layout *> (&layout));
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers = layers;
  m_has_layers = true;
  mp_layout.reset (const_cast<db::Layout *> (&layout));
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = false;
  init ();
  init_region (box_type::world ());
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers, const box_type &region, bool overlapping)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers.insert (m_layers.end (), layers.begin (), layers.end ());
  m_has_layers = true;
  mp_layout.reset (const_cast<db::Layout *> (&layout));
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers, const region_type &region, bool overlapping)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers.insert (m_layers.end (), layers.begin (), layers.end ());
  m_has_layers = true;
  mp_layout.reset (const_cast<db::Layout *> (&layout));
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers.insert (m_layers.end (), layers.begin (), layers.end ());
  m_has_layers = true;
  mp_layout.reset (const_cast<db::Layout *> (&layout));
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = false;
  init ();
  init_region (box_type::world ());
}

RecursiveShapeIterator::~RecursiveShapeIterator ()
{
    //  .. nothing yet ..
}


void 
RecursiveShapeIterator::init ()
{
  m_needs_reinit = true;
  m_max_depth = std::numeric_limits<int>::max (); // all
  m_min_depth = 0; // from the beginning
  m_shape_flags = shape_iterator::All;
  mp_shape_prop_sel = 0;
  m_shape_inv_prop_sel = false;
  m_inst_quad_id = 0;
  m_shape_quad_id = 0;
  mp_cell = 0;
  m_current_layer = 0;
  m_global_trans = cplx_trans_type ();
  m_property_translator = db::PropertiesTranslator ();
}

void
RecursiveShapeIterator::init_region (const RecursiveShapeIterator::box_type &region)
{
  m_region = region;
  mp_complex_region.reset (0);
}

void
RecursiveShapeIterator::init_region (const RecursiveShapeIterator::region_type &region)
{
  if (region.empty ()) {

    m_region = box_type ();
    mp_complex_region.reset (0);

  } else if (region.is_box ()) {

    m_region = region.bbox ();
    mp_complex_region.reset (0);

  } else {

    mp_complex_region.reset (new region_type (region));
    m_region = region.bbox ();
    //  A small optimization. We can do this since we merge and translate to trapezoids anyway.
    mp_complex_region->set_strict_handling (false);

  }
}

void
RecursiveShapeIterator::set_global_trans (const cplx_trans_type &tr)
{
  if (m_global_trans != tr) {
    m_global_trans = tr;
    m_needs_reinit = true;
  }
}

const db::RecursiveShapeIterator::cplx_trans_type &
RecursiveShapeIterator::always_apply () const
{
  if (m_trans_stack.empty ()) {
    return m_global_trans;
  } else {
    static cplx_trans_type unity;
    return unity;
  }
}

void
RecursiveShapeIterator::set_region (const box_type &region)
{
  if (m_region != region || mp_complex_region.get () != 0) {
    init_region (region);
    m_needs_reinit = true;
  }
}

void
RecursiveShapeIterator::set_region (const region_type &region)
{
  init_region (region);
  m_needs_reinit = true;
}

void
RecursiveShapeIterator::confine_region (const box_type &region)
{
  if (m_region.empty ()) {
    //  no more confinement
  } else if (mp_complex_region.get ()) {
    init_region (*mp_complex_region & region_type (region));
  } else {
    init_region (m_region & region);
  }
  m_needs_reinit = true;
}

void
RecursiveShapeIterator::confine_region (const region_type &region)
{
  if (m_region.empty ()) {
    //  no more confinement
  } else if (mp_complex_region.get ()) {
    init_region (*mp_complex_region & region);
  } else {
    init_region (region & region_type (m_region));
  }
  m_needs_reinit = true;
}

void
RecursiveShapeIterator::set_layer (unsigned int layer)
{
  if (m_has_layers || m_layer != layer) {
    m_has_layers = false;
    m_layers.clear ();
    m_layer = layer;
    m_needs_reinit = true;
  }
}

void
RecursiveShapeIterator::set_layers (const std::vector<unsigned int> &layers)
{
  if (! m_has_layers || m_layers != layers) {
    m_has_layers = true;
    m_layers = layers;
    m_layer = 0;
    m_needs_reinit = true;
  }
}

namespace {

struct BoxTreePusher
  : public db::SimplePolygonSink
{
  BoxTreePusher (RecursiveShapeIterator::box_tree_type *bt)
    : mp_bt (bt)
  {
    //  .. nothing yet ..
  }

  void put (const db::SimplePolygon &sp)
  {
    mp_bt->insert (sp.box ());
  }

private:
  RecursiveShapeIterator::box_tree_type *mp_bt;
};

}

void
RecursiveShapeIterator::validate (RecursiveShapeReceiver *receiver) const
{
  if (! m_needs_reinit) {
    return;
  }

  m_needs_reinit = false;

  //  re-initialize
  mp_cell = mp_top_cell;
  m_trans_stack.clear ();
  m_inst_iterators.clear ();
  m_inst_quad_id_stack.clear ();
  m_inst_array_iterators.clear ();
  m_cells.clear ();
  m_trans = m_global_trans;
  m_current_layer = 0;
  m_shape = shape_iterator ();
  m_shape_quad_id = 0;

  m_local_region_stack.clear ();
  m_local_region_stack.push_back (m_global_trans.inverted () * m_region);

  m_local_complex_region_stack.clear ();
  if (mp_complex_region.get ()) {

    //  prepare a local complex region
    m_local_complex_region_stack.push_back (box_tree_type ());

    //  Use a merge and the trapezoid generator to produce a decomposition that goes into the complex region

    db::EdgeProcessor ep;
    size_t n = 0;
    for (region_type::const_iterator p = mp_complex_region->begin (); !p.at_end (); ++p, ++n) {
      ep.insert (*p, n);
    }

    BoxTreePusher btp (&m_local_complex_region_stack.back ());
    db::TrapezoidGenerator tg (btp);

    db::MergeOp op (0);
    ep.process (tg, op);

    m_local_complex_region_stack.back ().sort (db::box_convert <db::Box> ());

  }

  if (mp_shapes) {
    //  Ensures the trees are built properly - this is important in MT contexts (i.e. TilingProcessor)
    //  TODO: get rid of that const cast
    (const_cast <db::Shapes *> (mp_shapes))->update ();
    start_shapes ();
  } else if (mp_layout && (! m_has_layers || m_current_layer < m_layers.size ())) {
    //  Ensures the trees are built properly - this is important in MT contexts (i.e. TilingProcessor)
    mp_layout->update ();
    new_cell (receiver);
    next_shape (receiver);
  }
}

void 
RecursiveShapeIterator::reset_selection ()
{
  if (mp_layout) {

    m_start.clear ();
    m_stop.clear ();

    m_needs_reinit = true;

  }
}

void 
RecursiveShapeIterator::unselect_cells (const std::set<db::cell_index_type> &cells)
{
  if (mp_layout) {

    for (std::set<db::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {
      m_stop.insert (*c);
      m_start.erase (*c);
    }

    m_needs_reinit = true;

  }
}

void 
RecursiveShapeIterator::unselect_all_cells ()
{
  if (mp_layout) {

    m_start.clear ();
    for (db::Layout::const_iterator c = mp_layout->begin (); c != mp_layout->end (); ++c) {
      m_stop.insert (c->cell_index ());
    }

    m_needs_reinit = true;

  }
}

void 
RecursiveShapeIterator::select_cells (const std::set<db::cell_index_type> &cells)
{
  if (mp_layout) {

    for (std::set<db::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {
      m_start.insert (*c);
      m_stop.erase (*c);
    }

    m_needs_reinit = true;

  }
}

void 
RecursiveShapeIterator::select_all_cells ()
{
  if (mp_layout) {

    m_stop.clear ();
    for (db::Layout::const_iterator c = mp_layout->begin (); c != mp_layout->end (); ++c) {
      m_start.insert (c->cell_index ());
    }

    m_needs_reinit = true;

  }
}

bool
RecursiveShapeIterator::at_end () const
{
  validate (0);
  return m_shape.at_end () || is_inactive ();
}

std::vector<db::InstElement>
RecursiveShapeIterator::path () const
{
  std::vector<db::InstElement> elements;
  for (size_t i = 0; i < m_inst_array_iterators.size () && i < m_inst_iterators.size (); ++i) {
    elements.push_back (db::InstElement (*m_inst_iterators [i], m_inst_array_iterators [i]));
  }
  return elements;
}

RecursiveShapeIterator::box_type
RecursiveShapeIterator::bbox () const
{
  box_type box;
  if (mp_shapes) {
    box = mp_shapes->bbox ();
  } else if (mp_top_cell) {
    if (m_has_layers) {
      for (std::vector<unsigned int>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
        box += mp_top_cell->bbox (*l);
      }
    } else {
      box += mp_top_cell->bbox (m_layer);
    }
  }

  box = box.transformed (m_global_trans);

  if (m_region != box_type::world ()) {
    box &= m_region;
  }

  return box;
}

void
RecursiveShapeIterator::skip_shape_iter_for_complex_region () const
{
  while (! m_shape.at_end ()) {

    //  skip shape quad if possible
    while (! m_shape.at_end ()) {
      if (is_outside_complex_region (m_shape.quad_box ())) {
        m_shape.skip_quad ();
      } else {
        m_shape_quad_id = m_shape.quad_id ();
        break;
      }
    }

    //  skip shapes outside the complex region
    if (! m_shape.at_end ()) {
      if (! is_outside_complex_region (m_shape->bbox ())) {
        break;
      } else {
        ++m_shape;
      }
    }

  }
}

void
RecursiveShapeIterator::skip_inst_iter_for_complex_region () const
{
  while (! m_inst.at_end ()) {

    //  skip inst quad if possible
    while (! m_inst.at_end ()) {
      if (is_outside_complex_region (m_inst.quad_box ())) {
        m_inst.skip_quad ();
      } else {
        m_inst_quad_id = m_inst.quad_id ();
        break;
      }
    }

    //  skip insts outside the complex region
    if (! m_inst.at_end ()) {
      if (! is_outside_complex_region (m_inst->bbox (m_box_convert))) {
        break;
      } else {
        ++m_inst;
      }
    }

  }
}

void
RecursiveShapeIterator::next (RecursiveShapeReceiver *receiver)
{
  if (! at_end ()) {

    ++m_shape;

    if (! m_local_complex_region_stack.empty ()) {
      skip_shape_iter_for_complex_region ();
    }

    if (! mp_shapes && m_shape.at_end ()) {
      next_shape (receiver);
    }

  }
}

void
RecursiveShapeIterator::next_shape (RecursiveShapeReceiver *receiver) const
{
  while (at_end ()) {

    if (m_has_layers && m_current_layer < m_layers.size () && ! is_inactive ()) {

      //  open a new layer
      if (++m_current_layer < m_layers.size ()) {
        m_layer = m_layers [m_current_layer];
        new_layer ();
      }

    } else {
      
      if (! m_inst.at_end () && int (m_inst_iterators.size ()) < m_max_depth) {

        tl_assert (mp_layout);

        //  determine whether the cell is empty with respect to the layers specified
        bool is_empty = false;
        if (receiver && receiver->wants_all_cells ()) {

          //  don't skip empty cells in that case

        } else if (! m_has_layers) {

          is_empty = mp_layout->cell (m_inst->cell_index ()).bbox (m_layer).empty ();

        } else {

          std::map<db::cell_index_type, bool>::const_iterator ec = m_empty_cells_cache.find (m_inst->cell_index ());
          if (ec != m_empty_cells_cache.end ()) {

            is_empty = ec->second;

          } else {

            is_empty = true;
            for (std::vector<unsigned int>::const_iterator l = m_layers.begin (); l != m_layers.end () && is_empty; ++l) {
              is_empty = mp_layout->cell (m_inst->cell_index ()).bbox (*l).empty ();
            }

            m_empty_cells_cache.insert (std::make_pair (m_inst->cell_index (), is_empty));

          }

        }

        if (is_empty) {

          ++m_inst;
          new_inst (receiver);

        } else {
          down (receiver);
        }

      } else {

        if (m_inst_iterators.empty ()) {
          //  nothing left:
          return;
        }

        //  no more instances: up and next instance
        up (receiver);

        ++m_inst_array;
        new_inst_member (receiver);

        if (m_inst_array.at_end ()) {
          ++m_inst;
          new_inst (receiver);
        }

      }

    }

  }
}

void
RecursiveShapeIterator::down (RecursiveShapeReceiver *receiver) const
{
  tl_assert (mp_layout);

  m_trans_stack.push_back (m_trans);
  m_cells.push_back (mp_cell);

  m_inst_iterators.push_back (m_inst);
  m_inst_array_iterators.push_back (m_inst_array);
  m_inst_quad_id_stack.push_back (m_inst_quad_id);

  bool ia = is_inactive ();
  bool aoi = is_all_of_instance ();
  mp_cell = &mp_layout->cell (m_inst->cell_index ());
  set_inactive (ia);
  set_all_of_instance (aoi);

  m_trans = m_trans * m_inst->complex_trans (*m_inst_array);

  //  don't transform the world region, since transformation of that region might not work properly
  box_type new_region = box_type::world ();

  //  compute the region inside the new cell
  if (new_region != m_region) {
    new_region = m_trans.inverted () * m_region;
    new_region &= cell_bbox (cell_index ());
  }
  m_local_region_stack.push_back (new_region);

  if (! m_local_complex_region_stack.empty ()) {

    m_local_complex_region_stack.push_back (box_tree_type ());
    const box_tree_type &pcl = m_local_complex_region_stack.end ()[-2];

    if (! new_region.empty ()) {

      //  compute a new, reduced complex region for use inside the new cell

      db::CellInstArray::complex_trans_type tinst = m_inst->complex_trans (*m_inst_array);
      db::CellInstArray::complex_trans_type tinst_inv = tinst.inverted ();

      db::Box bb;

      for (box_tree_type::touching_iterator b = pcl.begin_touching (new_region.transformed (tinst), db::box_convert<db::Box> ()); ! b.at_end (); ++b) {
        db::Box lb = (b->transformed (tinst_inv) & new_region);
        if (! lb.empty ()) {
          m_local_complex_region_stack.back ().insert (lb);
          bb += lb;
        }
      }

      m_local_complex_region_stack.back ().sort (db::box_convert<db::Box> ());

      //  re-adjust the new local region, so we take into account additional clipping by the complex region.
      //  in the extreme case, this box is empty:
      m_local_region_stack.back () = bb;

    }

  }

  if (receiver) {
    receiver->enter_cell (this, cell (), m_local_region_stack.back (), m_local_complex_region_stack.empty () ? 0 : &m_local_complex_region_stack.back ());
  }

  new_cell (receiver);
}

void
RecursiveShapeIterator::up (RecursiveShapeReceiver *receiver) const
{
  if (receiver) {
    receiver->leave_cell (this, cell ());
  }

  m_shape = shape_iterator ();
  m_shape_quad_id = 0;

  m_inst = m_inst_iterators.back ();
  m_inst_array = m_inst_array_iterators.back ();
  m_inst_quad_id = m_inst_quad_id_stack.back ();
  m_inst_iterators.pop_back ();
  m_inst_array_iterators.pop_back ();
  m_inst_quad_id_stack.pop_back ();

  m_trans = m_trans_stack.back ();
  m_trans_stack.pop_back ();
  mp_cell = m_cells.back ();
  m_cells.pop_back ();
  m_local_region_stack.pop_back ();
  if (! m_local_complex_region_stack.empty ()) {
    m_local_complex_region_stack.pop_back ();
  }
}

void
RecursiveShapeIterator::start_shapes () const
{
  if (! m_overlapping) {
    m_shape = mp_shapes->begin_touching (m_local_region_stack.back (), m_shape_flags, mp_shape_prop_sel, m_shape_inv_prop_sel);
  } else {
    m_shape = mp_shapes->begin_overlapping (m_local_region_stack.back (), m_shape_flags, mp_shape_prop_sel, m_shape_inv_prop_sel);
  }

  m_shape_quad_id = 0;

  if (! m_local_complex_region_stack.empty ()) {
    skip_shape_iter_for_complex_region ();
  }
}

void
RecursiveShapeIterator::new_layer () const
{
  if (int (m_trans_stack.size ()) < m_min_depth || int (m_trans_stack.size ()) > m_max_depth) {
    m_shape = shape_iterator ();
  } else if (! m_overlapping) {
    m_shape = cell ()->shapes (m_layer).begin_touching (m_local_region_stack.back (), m_shape_flags, mp_shape_prop_sel, m_shape_inv_prop_sel);
  } else {
    m_shape = cell ()->shapes (m_layer).begin_overlapping (m_local_region_stack.back (), m_shape_flags, mp_shape_prop_sel, m_shape_inv_prop_sel);
  }

  m_shape_quad_id = 0;

  //  skip instance quad if possible
  if (! m_local_complex_region_stack.empty ()) {
    skip_shape_iter_for_complex_region ();
  }
}

void 
RecursiveShapeIterator::new_cell (RecursiveShapeReceiver *receiver) const
{
  if (m_has_layers) {
    m_current_layer = 0;
    m_layer = m_layers.front ();
  }

  bool new_cell_inactive = is_child_inactive (cell_index ());
  if (is_inactive () != new_cell_inactive) {
    set_inactive (new_cell_inactive);
  }

  new_layer ();

  m_inst = cell ()->begin_touching (m_local_region_stack.back ());

  m_inst_quad_id = 0;

  //  skip instance quad if possible
  if (! m_local_complex_region_stack.empty ()) {
    skip_inst_iter_for_complex_region ();
  }

  new_inst (receiver);
}

void 
RecursiveShapeIterator::new_inst (RecursiveShapeReceiver *receiver) const
{
  //  look for the next instance with a non-empty array iterator. The latter can be 
  //  empty because we use a per-layer box converter for that case what we don't for the
  //  touching instance iterator.
  while (! m_inst.at_end ()) {

    //  skip instance quad if possible
    if (! m_local_complex_region_stack.empty ()) {
      skip_inst_iter_for_complex_region ();
      if (m_inst.at_end ()) {
        break;
      }
    }

    bool all_of_instance = false;
    bool with_region = false;

    if (m_local_region_stack.back () != box_type::world () && ! m_inst->cell_inst ().bbox (m_box_convert).inside (m_local_region_stack.back ())) {
      with_region = true;
    } else {
      //  TODO: optimization potential: only report all_of_instance == false, if not entirely within the complex region
      all_of_instance = m_local_complex_region_stack.empty ();
    }

    RecursiveShapeReceiver::new_inst_mode ni = RecursiveShapeReceiver::NI_all;
    if (receiver) {
      ni = receiver->new_inst (this, m_inst->cell_inst (), always_apply (), m_local_region_stack.back (), m_local_complex_region_stack.empty () ? 0 : &m_local_complex_region_stack.back (), all_of_instance);
    }

    if (ni == RecursiveShapeReceiver::NI_skip) {
      m_inst_array = inst_array_iterator ();
    } else if (ni == RecursiveShapeReceiver::NI_single) {
      //  a singular iterator
      m_inst_array = db::CellInstArray::iterator (m_inst->cell_inst ().front (), false);
    } else if (with_region) {
      m_inst_array = m_inst->cell_inst ().begin_touching (m_local_region_stack.back (), m_box_convert);
    } else {
      m_inst_array = m_inst->cell_inst ().begin ();
    }

    set_all_of_instance (all_of_instance);

    new_inst_member (receiver);

    if (! m_inst_array.at_end ()) {
      break;
    } else {
      ++m_inst;
    }

  }
}

void
RecursiveShapeIterator::new_inst_member (RecursiveShapeReceiver *receiver) const
{
  if (! m_local_complex_region_stack.empty ()) {

    //  skip instance array members not part of the complex region
    while (! m_inst_array.at_end ()) {
      db::Box ia_box = m_inst->complex_trans (*m_inst_array) * cell_bbox (m_inst->cell_index ());
      if (! is_outside_complex_region (ia_box)) {
        break;
      } else {
        ++m_inst_array;
      }
    }

  }

  while (! m_inst_array.at_end () && receiver) {
    if (receiver->new_inst_member (this, m_inst->cell_inst (), always_apply (), m_inst->complex_trans (*m_inst_array), m_local_region_stack.back (), m_local_complex_region_stack.empty () ? 0 : &m_local_complex_region_stack.back (), is_all_of_instance ())) {
      break;
    } else {
      ++m_inst_array;
    }
  }
}

bool
RecursiveShapeIterator::is_outside_complex_region (const db::Box &box) const
{
  if (m_overlapping) {
    return m_local_complex_region_stack.back ().begin_overlapping (box, db::box_convert<db::Box> ()).at_end ();
  } else {
    return m_local_complex_region_stack.back ().begin_touching (box, db::box_convert<db::Box> ()).at_end ();
  }
}

bool
RecursiveShapeIterator::is_child_inactive (db::cell_index_type new_child) const
{
  bool inactive = is_inactive ();
  if (! m_start.empty () && m_start.find (new_child) != m_start.end ()) {
    inactive = false;
  } else if (! m_stop.empty () && m_stop.find (new_child) != m_stop.end ()) {
    inactive = true;
  }
  return inactive;
}

void
RecursiveShapeIterator::push (RecursiveShapeReceiver *receiver)
{
  //  force reset so we can validate with a receiver
  reset ();

  receiver->begin (this);

  try {

    validate (receiver);

    while (! at_end ()) {
      receiver->shape (this, *m_shape, always_apply (), m_trans, m_local_region_stack.back (), m_local_complex_region_stack.empty () ? 0 : &m_local_complex_region_stack.back ());
      next (receiver);
    }

    receiver->end (this);

  } catch (...) {

    receiver->end (this);
    throw;

  }
}

}


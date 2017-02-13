
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

RecursiveShapeIterator::RecursiveShapeIterator ()
{
  //  anything. Not necessary reasonable.
  m_layer = 0;
  m_has_layers = false;
  mp_layout = 0;
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
}

RecursiveShapeIterator::RecursiveShapeIterator (const shapes_type &shapes)
{
  m_layer = 0;
  m_has_layers = false;
  m_region = box_type::world ();
  mp_layout = 0;
  mp_shapes = &shapes;
  mp_top_cell = 0;
  m_overlapping = false;
  init ();
}

RecursiveShapeIterator::RecursiveShapeIterator (const shapes_type &shapes, const box_type &region, bool overlapping)
{
  m_layer = 0;
  m_has_layers = false;
  m_region = region;
  mp_layout = 0;
  mp_shapes = &shapes;
  mp_top_cell = 0;
  m_overlapping = overlapping;
  init ();
}

RecursiveShapeIterator::RecursiveShapeIterator (const shapes_type &shapes, const region_type &region, bool overlapping)
{
  m_layer = 0;
  m_has_layers = false;
  m_region = region.bbox ();
  mp_layout = 0;
  mp_shapes = &shapes;
  mp_top_cell = 0;
  m_overlapping = overlapping;
  init ();
  init_complex_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const shapes_type &shapes, const box_type &region, const region_type &excl_region, bool overlapping)
{
  m_layer = 0;
  m_has_layers = false;
  m_region = region;
  mp_layout = 0;
  mp_shapes = &shapes;
  mp_top_cell = 0;
  m_overlapping = overlapping;
  init ();
  init_complex_region (region, excl_region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, unsigned int layer, const box_type &region, bool overlapping)
  : m_box_convert (layout, layer)
{
  m_layer = layer;
  m_has_layers = false;
  m_region = region;
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, unsigned int layer, const region_type &region, bool overlapping)
  : m_box_convert (layout, layer)
{
  m_layer = layer;
  m_has_layers = false;
  m_region = region.bbox ();
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_complex_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, unsigned int layer, const box_type &region, const region_type &excl_region, bool overlapping)
  : m_box_convert (layout, layer)
{
  m_layer = layer;
  m_has_layers = false;
  m_region = region;
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_complex_region (region, excl_region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, unsigned int layer)
  : m_box_convert (layout, layer)
{
  m_layer = layer;
  m_has_layers = false;
  m_region = box_type::world ();
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = false;
  init ();
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers, const box_type &region, bool overlapping)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers = layers;
  m_has_layers = true;
  m_region = region;
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers, const region_type &region, bool overlapping)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers = layers;
  m_has_layers = true;
  m_region = region.bbox ();
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_complex_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers, const box_type &region, const region_type &excl_region, bool overlapping)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers = layers;
  m_has_layers = true;
  m_region = region;
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_complex_region (region, excl_region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::vector<unsigned int> &layers)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers = layers;
  m_has_layers = true;
  m_region = box_type::world ();
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = false;
  init ();
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers, const box_type &region, bool overlapping)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers.insert (m_layers.end (), layers.begin (), layers.end ());
  m_has_layers = true;
  m_region = region;
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers, const region_type &region, bool overlapping)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers.insert (m_layers.end (), layers.begin (), layers.end ());
  m_has_layers = true;
  m_region = region.bbox ();
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_complex_region (region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers, const box_type &region, const region_type &excl_region, bool overlapping)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers.insert (m_layers.end (), layers.begin (), layers.end ());
  m_has_layers = true;
  m_region = region;
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_complex_region (region, excl_region);
}

RecursiveShapeIterator::RecursiveShapeIterator (const layout_type &layout, const cell_type &cell, const std::set<unsigned int> &layers)
  : m_box_convert (layout)
{
  m_layer = 0;
  m_layers.insert (m_layers.end (), layers.begin (), layers.end ());
  m_has_layers = true;
  m_region = box_type::world ();
  mp_layout = &layout;
  mp_shapes = 0;
  mp_top_cell = &cell;
  m_overlapping = false;
  init ();
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
RecursiveShapeIterator::init_complex_region (const RecursiveShapeIterator::box_type &box, const RecursiveShapeIterator::region_type &excl_region)
{
  //  Use a boolean NOT and the trapezoid generator to produce a decomposition that goes into the complex region

  db::EdgeProcessor ep;
  ep.insert (db::Polygon (box), 0);
  size_t n = 1;
  for (region_type::const_iterator p = excl_region.begin (); !p.at_end (); ++p) {
    ep.insert (*p, n);
    n += 2;
  }

  BoxTreePusher btp (&m_complex_region);
  db::TrapezoidGenerator tg (btp);

  db::BooleanOp op (BooleanOp::ANotB);
  ep.process (tg, op);

  m_complex_region.sort (db::box_convert <db::Box> ());
}

void
RecursiveShapeIterator::init_complex_region (const RecursiveShapeIterator::region_type &region)
{
  //  Use a merge and the trapezoid generator to produce a decomposition that goes into the complex region

  db::EdgeProcessor ep;
  size_t n = 0;
  for (region_type::const_iterator p = region.begin (); !p.at_end (); ++p, ++n) {
    ep.insert (*p, n);
  }

  BoxTreePusher btp (&m_complex_region);
  db::TrapezoidGenerator tg (btp);

  db::MergeOp op (0);
  ep.process (tg, op);

  m_complex_region.sort (db::box_convert <db::Box> ());
}

void
RecursiveShapeIterator::validate () const
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
  m_trans = cplx_trans_type ();
  m_current_layer = 0;
  m_shape = shape_iterator ();
  m_local_region_stack.push_back (m_region);
  if (!m_complex_region.empty ()) {
    m_local_complex_region_stack.push_back (m_complex_region);
  }

  if (mp_shapes) {
    //  Ensures the trees are built properly - this is important in MT contexts (i.e. TilingProcessor)
    //  TODO: get rid of that const cast
    (const_cast <db::Shapes *> (mp_shapes))->update ();
    start_shapes ();
  } else if (! m_has_layers || m_current_layer < m_layers.size ()) {
    //  Ensures the trees are built properly - this is important in MT contexts (i.e. TilingProcessor)
    mp_layout->update ();
    new_cell ();
    next_shape ();
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
  validate ();
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
      if (! is_outside_complex_region (m_inst->bbox ())) {
        break;
      } else {
        ++m_inst;
      }
    }

  }
}

void
RecursiveShapeIterator::next ()
{
  if (! at_end ()) {

    ++m_shape;

    if (! m_local_complex_region_stack.empty ()) {
      skip_shape_iter_for_complex_region ();
    }

    if (! mp_shapes && m_shape.at_end ()) {
      next_shape ();
    }

  }
}

void
RecursiveShapeIterator::next_shape () const
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

        //  determine whether the cell is empty with respect to the layers specified
        bool is_empty = false;
        if (! m_has_layers) {

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
          new_inst ();

        } else {
          down ();
        }

      } else {

        if (m_inst_iterators.empty ()) {
          //  nothing left:
          return;
        }

        //  no more instances: up and next instance
        up ();

        ++m_inst_array;
        if (m_inst_array.at_end ()) {
          ++m_inst;
          new_inst ();
        }

      }

    }

  }
}

void
RecursiveShapeIterator::down () const
{
  m_trans_stack.push_back (m_trans);
  m_cells.push_back (mp_cell);

  m_inst_iterators.push_back (m_inst);
  m_inst_array_iterators.push_back (m_inst_array);
  m_inst_quad_id_stack.push_back (m_inst_quad_id);

  bool ia = is_inactive ();
  mp_cell = &mp_layout->cell (m_inst->cell_index ());
  set_inactive (ia);

  m_trans = m_trans * m_inst->complex_trans (*m_inst_array);

  //  don't transform the world region, since transformation of that region might not work properly
  box_type new_region = box_type::world ();

  //  compute the region inside the new cell
  if (new_region != m_local_region_stack.front ()) {
    new_region = m_trans.inverted () * m_local_region_stack.front ();
    new_region &= cell ()->bbox ();
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

  new_cell ();
}

void
RecursiveShapeIterator::up () const
{
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
RecursiveShapeIterator::new_cell () const
{
  if (m_has_layers) {
    m_current_layer = 0;
    m_layer = m_layers.front ();
  }

  if (! m_start.empty () && m_start.find (cell_index ()) != m_start.end ()) {
    set_inactive (false);
  } else if (! m_stop.empty () && m_stop.find (cell_index ()) != m_stop.end ()) {
    set_inactive (true);
  }

  new_layer ();

  m_inst = cell ()->begin_touching (m_local_region_stack.back ());

  m_inst_quad_id = 0;

  //  skip instance quad if possible
  if (! m_local_complex_region_stack.empty ()) {
    skip_inst_iter_for_complex_region ();
  }

  new_inst ();
}

void 
RecursiveShapeIterator::new_inst () const
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

    if (m_local_region_stack.back () != box_type::world ()) {
      m_inst_array = m_inst->cell_inst ().begin_touching (m_local_region_stack.back (), m_box_convert);
    } else {
      m_inst_array = m_inst->cell_inst ().begin (); 
    }

    if (! m_inst_array.at_end ()) {
      break;
    } else {
      ++m_inst;
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

}


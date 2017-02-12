
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
  m_inst_array_iterators.clear ();
  m_cells.clear ();
  m_trans = cplx_trans_type ();
  m_current_layer = 0;
  m_shape = shape_iterator ();

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

        //  no more instances: up and next instance
        if (m_inst_iterators.empty ()) {
          //  nothing left:
          return;
        }

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

  bool ia = is_inactive ();
  mp_cell = &mp_layout->cell (m_inst->cell_index ());
  set_inactive (ia);

  m_trans = m_trans * m_inst->complex_trans (*m_inst_array);

  new_cell ();
}

void
RecursiveShapeIterator::up () const
{
  m_shape = shape_iterator ();

  m_inst = m_inst_iterators.back ();
  m_inst_array = m_inst_array_iterators.back ();
  m_inst_iterators.pop_back ();
  m_inst_array_iterators.pop_back ();

  m_trans = m_trans_stack.back ();
  m_trans_stack.pop_back ();
  mp_cell = m_cells.back ();
  m_cells.pop_back ();

  m_local_region = box_type::world ();
  if (m_region != m_local_region) {
    m_local_region = trans ().inverted () * m_region;
  }
}

void
RecursiveShapeIterator::start_shapes () const
{
  if (! m_overlapping) {
    m_shape = mp_shapes->begin_touching (m_region, m_shape_flags, mp_shape_prop_sel, m_shape_inv_prop_sel); 
  } else {
    m_shape = mp_shapes->begin_overlapping (m_region, m_shape_flags, mp_shape_prop_sel, m_shape_inv_prop_sel); 
  }
}

void
RecursiveShapeIterator::new_layer () const
{
  if (int (m_trans_stack.size ()) < m_min_depth || int (m_trans_stack.size ()) > m_max_depth) {
    m_shape = shape_iterator ();
  } else if (! m_overlapping) {
    m_shape = cell ()->shapes (m_layer).begin_touching (m_local_region, m_shape_flags, mp_shape_prop_sel, m_shape_inv_prop_sel); 
  } else {
    m_shape = cell ()->shapes (m_layer).begin_overlapping (m_local_region, m_shape_flags, mp_shape_prop_sel, m_shape_inv_prop_sel); 
  }
}

void 
RecursiveShapeIterator::new_cell () const
{
  //  don't transform the world region, since transformation of that region might not work properly
  m_local_region = box_type::world ();
  if (m_region != m_local_region) {
    m_local_region = trans ().inverted () * m_region;
  }

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

  m_inst = cell ()->begin_touching (m_local_region);
  new_inst ();
}

void 
RecursiveShapeIterator::new_inst () const
{
  //  look for the next instance with a non-empty array iterator. The latter can be 
  //  empty because we use a per-layer box converter for that case what we don't for the
  //  touching instance iterator.
  while (! m_inst.at_end ()) {

    if (m_local_region != box_type::world ()) {
      m_inst_array = m_inst->cell_inst ().begin_touching (m_local_region, m_box_convert); 
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

}


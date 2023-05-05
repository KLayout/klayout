
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

#include "dbRecursiveInstanceIterator.h"
#include "dbRegion.h"
#include "dbEdgeProcessor.h"
#include "tlProgress.h"

namespace db
{

// ------------------------------------------------------------------------------------
//  Recursive shape iterator implementation

RecursiveInstanceIterator::RecursiveInstanceIterator (const RecursiveInstanceIterator &d)
{
  operator= (d);
}

RecursiveInstanceIterator &RecursiveInstanceIterator::operator= (const RecursiveInstanceIterator &d)
{
  if (&d != this) {

    m_all_targets = d.m_all_targets;
    m_targets = d.m_targets;

    m_max_depth = d.m_max_depth;
    m_min_depth = d.m_min_depth;
    m_overlapping = d.m_overlapping;
    m_start = d.m_start;
    m_stop = d.m_stop;

    mp_layout = d.mp_layout;
    mp_top_cell = d.mp_top_cell;

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
    mp_cell = d.mp_cell;
    m_trans = d.m_trans;
    m_trans_stack = d.m_trans_stack;
    m_inst_iterators = d.m_inst_iterators;
    m_inst_array_iterators = d.m_inst_array_iterators;
    m_cells = d.m_cells;
    m_local_complex_region_stack = d.m_local_complex_region_stack;
    m_local_region_stack = d.m_local_region_stack;
    m_needs_reinit = d.m_needs_reinit;
    m_inst_quad_id = d.m_inst_quad_id;
    m_inst_quad_id_stack = d.m_inst_quad_id_stack;

  }
  return *this;
}

RecursiveInstanceIterator::RecursiveInstanceIterator ()
{
  //  anything. Not necessary reasonable.
  mp_top_cell = 0;
  mp_cell = 0;
  m_overlapping = false;
  m_max_depth = std::numeric_limits<int>::max (); // all
  m_min_depth = 0;
  m_needs_reinit = false;
  m_inst_quad_id = 0;
  m_all_targets = true;
}

RecursiveInstanceIterator::RecursiveInstanceIterator (const layout_type &layout, const cell_type &cell, const box_type &region, bool overlapping)
  : m_box_convert (layout)
{
  mp_layout = const_cast<db::Layout *> (&layout);
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_region (region);
}

RecursiveInstanceIterator::RecursiveInstanceIterator (const layout_type &layout, const cell_type &cell, const region_type &region, bool overlapping)
  : m_box_convert (layout)
{
  mp_layout = const_cast<db::Layout *> (&layout);
  mp_top_cell = &cell;
  m_overlapping = overlapping;
  init ();
  init_region (region);
}

RecursiveInstanceIterator::RecursiveInstanceIterator (const layout_type &layout, const cell_type &cell)
  : m_box_convert (layout)
{
  mp_layout = const_cast<db::Layout *> (&layout);
  mp_top_cell = &cell;
  m_overlapping = false;
  init ();
  init_region (box_type::world ());
}

RecursiveInstanceIterator::~RecursiveInstanceIterator ()
{
    //  .. nothing yet ..
}


void 
RecursiveInstanceIterator::init ()
{
  m_needs_reinit = true;
  m_max_depth = std::numeric_limits<int>::max (); // all
  m_min_depth = 0; // from the beginning
  m_inst_quad_id = 0;
  mp_cell = 0;
  m_all_targets = true;
}

void
RecursiveInstanceIterator::init_region (const RecursiveInstanceIterator::box_type &region)
{
  m_region = region;
  mp_complex_region.reset (0);
}

void
RecursiveInstanceIterator::init_region (const RecursiveInstanceIterator::region_type &region)
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
RecursiveInstanceIterator::set_region (const box_type &region)
{
  if (m_region != region || mp_complex_region.get () != 0) {
    init_region (region);
    m_needs_reinit = true;
  }
}

void
RecursiveInstanceIterator::set_region (const region_type &region)
{
  init_region (region);
  m_needs_reinit = true;
}

void
RecursiveInstanceIterator::confine_region (const box_type &region)
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
RecursiveInstanceIterator::confine_region (const region_type &region)
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
RecursiveInstanceIterator::enable_all_targets ()
{
  if (! m_all_targets) {
    m_all_targets = true;
    m_targets.clear ();
    m_needs_reinit = true;
  }
}

void
RecursiveInstanceIterator::set_targets (const std::set<db::cell_index_type> &tgt)
{
  if (m_all_targets || m_targets != tgt) {
    m_targets = tgt;
    m_all_targets = false;
    m_needs_reinit = true;
  }
}

namespace {

struct BoxTreePusher
  : public db::SimplePolygonSink
{
  BoxTreePusher (RecursiveInstanceIterator::box_tree_type *bt)
    : mp_bt (bt)
  {
    //  .. nothing yet ..
  }

  void put (const db::SimplePolygon &sp)
  {
    mp_bt->insert (sp.box ());
  }

private:
  RecursiveInstanceIterator::box_tree_type *mp_bt;
};

}

void
RecursiveInstanceIterator::validate (RecursiveInstanceReceiver *receiver) const
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
  m_target_tree.clear ();

  m_local_region_stack.clear ();
  m_local_region_stack.push_back (m_region);

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

  if (mp_top_cell && mp_layout) {

    if (! m_all_targets) {
      mp_top_cell->collect_called_cells (m_target_tree);
    }

    new_cell (receiver);
    next_instance (receiver);

  }
}

void 
RecursiveInstanceIterator::reset_selection ()
{
  if (mp_layout) {

    m_start.clear ();
    m_stop.clear ();

    m_needs_reinit = true;

  }
}

void 
RecursiveInstanceIterator::unselect_cells (const std::set<db::cell_index_type> &cells)
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
RecursiveInstanceIterator::unselect_all_cells ()
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
RecursiveInstanceIterator::select_cells (const std::set<db::cell_index_type> &cells)
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
RecursiveInstanceIterator::select_all_cells ()
{
  if (mp_layout) {

    m_stop.clear ();
    for (db::Layout::const_iterator c = mp_layout->begin (); c != mp_layout->end (); ++c) {
      m_start.insert (c->cell_index ());
    }

    m_needs_reinit = true;

  }
}

const RecursiveInstanceIterator::instance_element_type *
RecursiveInstanceIterator::operator-> () const
{
  validate (0);
  m_combined_instance = db::InstElement (*m_inst, m_inst_array);
  return &m_combined_instance;
}

bool
RecursiveInstanceIterator::at_end () const
{
  validate (0);
  return m_inst.at_end ();
}

std::vector<db::InstElement>
RecursiveInstanceIterator::path () const
{
  std::vector<db::InstElement> elements;
  for (size_t i = 0; i < m_inst_array_iterators.size () && i < m_inst_iterators.size (); ++i) {
    elements.push_back (db::InstElement (*m_inst_iterators [i], m_inst_array_iterators [i]));
  }
  return elements;
}

void
RecursiveInstanceIterator::skip_inst_iter_for_complex_region () const
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
RecursiveInstanceIterator::next (RecursiveInstanceReceiver *receiver)
{
  if (! at_end ()) {
    ++m_inst_array;
    if (! m_inst_array.at_end ()) {
      new_inst_member (receiver);
    } else {
      ++m_inst;
      new_inst (receiver);
    }
    next_instance (receiver);
  }
}

bool
RecursiveInstanceIterator::needs_visit () const
{
  return int (m_inst_iterators.size ()) >= m_min_depth && ! is_inactive () && (m_all_targets || m_targets.find (m_inst->cell_index ()) != m_targets.end ());
}

void
RecursiveInstanceIterator::next_instance (RecursiveInstanceReceiver *receiver) const
{
  while (true) {

    while (true) {

      if (! m_inst.at_end ()) {

        if (int (m_inst_iterators.size ()) < m_max_depth && (m_all_targets || m_target_tree.find (m_inst->cell_index ()) != m_target_tree.end ())) {
          down (receiver);
        } else {
          break;
        }

      } else {

        if (! m_inst_iterators.empty ()) {
          //  no more instances: up and next instance
          up (receiver);
        }
        break;

      }

    }

    if (m_inst.at_end ()) {
      break;
    }

    if (! needs_visit ()) {
      ++m_inst_array;
      if (! m_inst_array.at_end ()) {
        new_inst_member (receiver);
      } else {
        ++m_inst;
        new_inst (receiver);
      }
    } else {
      break;
    }

  }
}

void
RecursiveInstanceIterator::down (RecursiveInstanceReceiver *receiver) const
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

      for (box_tree_type::touching_iterator b = pcl.begin_touching (correct_box_overlapping (new_region.transformed (tinst)), db::box_convert<db::Box> ()); ! b.at_end (); ++b) {
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
RecursiveInstanceIterator::up (RecursiveInstanceReceiver *receiver) const
{
  if (receiver) {
    receiver->leave_cell (this, cell ());
  }

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
RecursiveInstanceIterator::new_cell (RecursiveInstanceReceiver *receiver) const
{
  bool new_cell_inactive = is_child_inactive (cell_index ());
  if (is_inactive () != new_cell_inactive) {
    set_inactive (new_cell_inactive);
  }

  m_inst = cell ()->begin_touching (correct_box_overlapping (m_local_region_stack.back ()));

  m_inst_quad_id = 0;

  //  skip instance quad if possible
  if (! m_local_complex_region_stack.empty ()) {
    skip_inst_iter_for_complex_region ();
  }

  new_inst (receiver);
}

void 
RecursiveInstanceIterator::new_inst (RecursiveInstanceReceiver *receiver) const
{
  //  look for the next instance with a non-empty array iterator. The array iterator can be empty because we
  //  use a lookup region.
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

    RecursiveInstanceReceiver::new_inst_mode ni = RecursiveInstanceReceiver::NI_all;
    if (receiver) {
      ni = receiver->new_inst (this, m_inst->cell_inst (), m_local_region_stack.back (), m_local_complex_region_stack.empty () ? 0 : &m_local_complex_region_stack.back (), all_of_instance);
    }

    if (ni == RecursiveInstanceReceiver::NI_skip) {
      m_inst_array = inst_array_iterator ();
    } else if (ni == RecursiveInstanceReceiver::NI_single) {
      //  a singular iterator
      m_inst_array = db::CellInstArray::iterator (m_inst->cell_inst ().front (), false);
    } else if (with_region) {
      m_inst_array = m_inst->cell_inst ().begin_touching (correct_box_overlapping (m_local_region_stack.back ()), m_box_convert);
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
RecursiveInstanceIterator::new_inst_member (RecursiveInstanceReceiver *receiver) const
{
  if (! m_local_complex_region_stack.empty ()) {

    //  skip instance array members not part of the complex region
    while (! m_inst_array.at_end ()) {
      db::Box ia_box = m_inst->complex_trans (*m_inst_array) * m_box_convert (m_inst->cell_inst ().object ());
      if (! is_outside_complex_region (ia_box)) {
        break;
      } else {
        ++m_inst_array;
      }
    }

  }

  while (! m_inst_array.at_end () && receiver) {
    if (receiver->new_inst_member (this, m_inst->cell_inst (), m_inst->complex_trans (*m_inst_array), m_local_region_stack.back (), m_local_complex_region_stack.empty () ? 0 : &m_local_complex_region_stack.back (), is_all_of_instance ())) {
      break;
    } else {
      ++m_inst_array;
    }
  }
}

RecursiveInstanceIterator::box_type
RecursiveInstanceIterator::correct_box_overlapping (const box_type &box) const
{
  if (! m_overlapping) {
    return box;
  } else if (box.empty () || box == box_type::world ()) {
    return box;
  } else if (box.width () < 2 || box.height () < 2) {
    return box;
  } else {
    return box.enlarged (box_type::vector_type (-1, -1));
  }
}

bool
RecursiveInstanceIterator::is_outside_complex_region (const box_type &box) const
{
  if (m_overlapping) {
    return m_local_complex_region_stack.back ().begin_overlapping (box, db::box_convert<box_type> ()).at_end ();
  } else {
    return m_local_complex_region_stack.back ().begin_touching (box, db::box_convert<box_type> ()).at_end ();
  }
}

bool
RecursiveInstanceIterator::is_child_inactive (db::cell_index_type new_child) const
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
RecursiveInstanceIterator::push (RecursiveInstanceReceiver *receiver)
{
  //  force reset so we can validate with a receiver
  reset ();

  receiver->begin (this);

  try {

    validate (receiver);

    while (! at_end ()) {
      next (receiver);
    }

    receiver->end (this);

  } catch (...) {

    receiver->end (this);
    throw;

  }
}

}


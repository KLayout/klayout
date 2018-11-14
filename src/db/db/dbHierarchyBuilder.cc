
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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
#include "dbHierarchyBuilder.h"
#include "dbClip.h"
#include "dbRegion.h"

namespace db
{

// -------------------------------------------------------------------------------------------

int
compare_iterators_with_respect_to_target_hierarchy (const db::RecursiveShapeIterator &iter1, const db::RecursiveShapeIterator &iter2)
{
  //  basic source (layout, top_cell) needs to be the same of course
  if (iter1.layout () != iter2.layout ()) {
    //  NOTE: pointer compare :-(
    return iter1.layout () < iter2.layout () ? -1 : 1;
  }
  if (iter1.top_cell ()->cell_index () != iter2.top_cell ()->cell_index ()) {
    return iter1.top_cell ()->cell_index () < iter2.top_cell ()->cell_index () ? -1 : 1;
  }

  //  max depth controls the main hierarchical appearance
  if (iter1.max_depth () != iter2.max_depth ()) {
    return iter1.max_depth () < iter2.max_depth () ? -1 : 1;
  }

  //  if a region is set, the hierarchical appearance is the same only if the layers and
  //  complex region are indentical
  if ((iter1.region () == db::Box::world ()) != (iter2.region () == db::Box::world ())) {
    return (iter1.region () == db::Box::world ()) < (iter2.region () == db::Box::world ()) ? -1 : 1;
  }
  if (iter1.region () != db::Box::world ()) {
    if (iter1.has_complex_region () != iter2.has_complex_region ()) {
      return iter1.has_complex_region () < iter2.has_complex_region () ? -1 : 1;
    }
    if (iter1.has_complex_region () && iter1.complex_region () != iter2.complex_region ()) {
      return iter1.complex_region () < iter2.complex_region () ? -1 : 1;
    }
    if (iter1.multiple_layers () != iter2.multiple_layers ()) {
      return iter1.multiple_layers () < iter2.multiple_layers () ? -1 : 1;
    }
    if (iter1.multiple_layers ()) {
      if (iter1.layers () != iter2.layers ()) {
        return iter1.layers () < iter2.layers ();
      }
    } else {
      if (iter1.layer () != iter2.layer ()) {
        return iter1.layer () < iter2.layer ();
      }
    }
  }

  return 0;
}

// -------------------------------------------------------------------------------------------

static bool is_outside (const db::Box &obj, const std::set<db::Box> &region)
{
  if (region.empty ()) {
    return false;
  }

  for (std::set<db::Box>::const_iterator b = region.begin (); b != region.end (); ++b) {
    if (obj.touches (*b)) {
      return false;
    }
  }

  return true;
}

static bool is_inside (const db::Box &obj, const std::set<db::Box> &region)
{
  if (region.empty ()) {
    return true;
  }

  for (std::set<db::Box>::const_iterator b = region.begin (); b != region.end (); ++b) {
    if (obj.inside (*b)) {
      return true;
    }
  }

  if (is_outside (obj, region)) {
    return false;
  } else {

    //  TODO: basically a detailed analysis is required here
    return false;

  }
}

/**
 *  @brief Computes the clip variant (a box set) from a cell bbox, a region and a complex region (optional)
 */
static std::set<db::Box> compute_clip_variant (const db::Box &cell_bbox, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region)
{
  std::set<db::Box> clip_variant;
  if (region != db::Box::world () && ! cell_bbox.inside (region)) {

    db::Box rect_box = region & cell_bbox;

    if (complex_region) {

      for (db::RecursiveShapeReceiver::box_tree_type::overlapping_iterator cr = complex_region->begin_overlapping (rect_box, db::box_convert<db::Box> ()); ! cr.at_end (); ++cr) {
        if (rect_box.overlaps (*cr)) {
          clip_variant.insert (rect_box * *cr);
        }
      }

      if (clip_variant.empty ()) {
        //  an empty clip variant should not happen, but who knows
        clip_variant.insert (db::Box ());
      }

    } else {
      clip_variant.insert (rect_box);
    }

  }

  return clip_variant;
}

static void insert_clipped (const db::Box &box, const std::set<db::Box> &clip, db::Shapes &shapes)
{
  for (std::set<db::Box>::const_iterator b = clip.begin (); b != clip.end (); ++b) {
    db::Box bb = *b & box;
    if (! bb.empty ()) {
      shapes.insert (bb);
    }
  }
}

static void insert_clipped (const db::Polygon &poly, const std::set<db::Box> &clip, db::Shapes &shapes)
{
  //  TODO: is this good way to clip a polygon at a complex boundary?
  for (std::set<db::Box>::const_iterator b = clip.begin (); b != clip.end (); ++b) {
    std::vector<db::Polygon> clipped_poly;
    db::clip_poly (poly, *b, clipped_poly);
    for (std::vector<db::Polygon>::const_iterator p = clipped_poly.begin (); p != clipped_poly.end (); ++p) {
      shapes.insert (*p);
    }
  }
}


HierarchyBuilder::HierarchyBuilder (db::Layout *target, unsigned int target_layer, bool clip_shapes)
  : mp_target (target), m_clip_shapes (clip_shapes), m_initial_pass (true), m_target_layer (target_layer)
{
  //  .. nothing yet ..
}

void
HierarchyBuilder::begin (const RecursiveShapeIterator *iter)
{
  if (m_initial_pass) {
    m_ref_iter = *iter;
  } else {
    tl_assert (compare_iterators_with_respect_to_target_hierarchy (m_ref_iter, *iter) == 0);
  }

  m_cells_seen.clear ();
  m_cm_entry = cell_map_type::const_iterator ();

  m_cell_stack.clear ();

  db::Cell &new_top = mp_target->cell (mp_target->add_cell (iter->layout ()->cell_name (iter->top_cell ()->cell_index ())));
  m_cell_stack.push_back (&new_top);
}

void
HierarchyBuilder::end (const RecursiveShapeIterator * /*iter*/)
{
  m_initial_pass = false;
  m_cells_seen.clear ();
  m_cell_stack.pop_back ();
}

void
HierarchyBuilder::enter_cell (const RecursiveShapeIterator * /*iter*/, const db::Cell * /*cell*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/)
{
  tl_assert (m_cm_entry != m_cell_map.end () && m_cm_entry != cell_map_type::const_iterator ());
  m_cells_seen.insert (m_cm_entry->first);

  m_cell_stack.push_back (&mp_target->cell (m_cm_entry->second));
}

void
HierarchyBuilder::leave_cell (const RecursiveShapeIterator * /*iter*/, const db::Cell * /*cell*/)
{
  m_cell_stack.pop_back ();
}

bool
HierarchyBuilder::new_inst (const RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::Box & /*region*/, const box_tree_type * /*complex_region*/, bool all)
{
  if (all) {

    std::pair<db::cell_index_type, std::set<db::Box> > key (inst.object ().cell_index (), std::set<db::Box> ());
    m_cm_entry = m_cell_map.find (key);

    if (m_initial_pass) {

      if (m_cm_entry == m_cell_map.end ()) {
        db::cell_index_type new_cell = mp_target->add_cell (iter->layout ()->cell_name (inst.object ().cell_index ()));
        m_cm_entry = m_cell_map.insert (std::make_pair (key, new_cell)).first;
      }

      db::CellInstArray new_inst = inst;
      new_inst.object () = db::CellInst (m_cm_entry->second);
      m_cell_stack.back ()->insert (new_inst);

    }

    return (m_cells_seen.find (key) == m_cells_seen.end ());

  } else {
    //  iterate by instance array members
    return true;
  }
}

bool
HierarchyBuilder::new_inst_member (const RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::ICplxTrans &trans, const db::Box &region, const box_tree_type *complex_region)
{
  db::Box cell_bbox = iter->layout ()->cell (inst.object ().cell_index ()).bbox ();
  std::set<db::Box> clip_variant = compute_clip_variant (cell_bbox, region, complex_region);

  std::pair<db::cell_index_type, std::set<db::Box> > key (inst.object ().cell_index (), clip_variant);
  m_cm_entry = m_cell_map.find (key);

  if (m_initial_pass) {

    if (m_cm_entry == m_cell_map.end ()) {
      std::string suffix;
      if (! key.second.empty ()) {
        suffix = "$CLIP_VAR";
      }
      db::cell_index_type new_cell = mp_target->add_cell ((std::string (iter->layout ()->cell_name (inst.object ().cell_index ())) + suffix).c_str ());
      m_cm_entry = m_cell_map.insert (std::make_pair (key, new_cell)).first;
    }

    db::CellInstArray new_inst (db::CellInst (m_cm_entry->second), trans);
    m_cell_stack.back ()->insert (new_inst);

  }

  return (m_cells_seen.find (key) == m_cells_seen.end ());
}

void
HierarchyBuilder::shape (const RecursiveShapeIterator * /*iter*/, const db::Shape &shape, const db::ICplxTrans & /*trans*/)
{
  tl_assert (m_cm_entry != m_cell_map.end () && m_cm_entry != cell_map_type::const_iterator ());

  //  TODO: property mapping?

  db::Shapes &shapes = m_cell_stack.back ()->shapes (m_target_layer);

  if (m_cm_entry->first.second.empty () || is_inside (shape.bbox (), m_cm_entry->first.second)) {

    shapes.insert (shape);

  } else if (! is_outside (shape.bbox (), m_cm_entry->first.second)) {

    //  clip the shape if required
    if (! m_clip_shapes || shape.is_text () || shape.is_edge () || shape.is_edge_pair ()) {
      shapes.insert (shape);
    } else if (shape.is_box ()) {
      insert_clipped (shape.box (), m_cm_entry->first.second, shapes);
    } else if (shape.is_polygon () || shape.is_simple_polygon () || shape.is_path ()) {
      insert_clipped (shape.polygon (), m_cm_entry->first.second, shapes);
    }

  }
}

}

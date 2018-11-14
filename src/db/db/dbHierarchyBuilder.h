
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

#ifndef HDR_dbHierarchyBuilder
#define HDR_dbHierarchyBuilder

#include "dbCommon.h"

#include "dbRecursiveShapeIterator.h"
#include "dbLayout.h"

#include <map>
#include <vector>
#include <set>

namespace db
{

/**
 *  @brief A helper function comparing two recursive shape iterators for compatibility with respect to hierarchy building
 *
 *  This function will return -1, 0 or 1 depending on whether the two iterators
 *  can be used with the same builder (0) or whether they are less (-1) or greater (1).
 */
int compare_iterators_with_respect_to_target_hierarchy (const db::RecursiveShapeIterator &iter1, const db::RecursiveShapeIterator &iter2);

/**
 *  @brief A class building a hierarchy from a recursive shape iterator in push mode
 *
 *  This class is a RecursiveShapeReceiver which acts on the hierarchy events and
 *  uses them to rebuild a hierarchy in the target layout. In can be used multiple
 *  times and will reuse the hierarchy as far as possible.
 *
 *  The hierarchy builder can form clip variants for cells and clip the shapes
 *  according to the selected region.
 *
 *  NOTE: the hierarchy build should not be used in multiple passes with regions
 *  as the hierarchy is sampled in the first pass and the hierarchy builder will
 *  rely on precisely the same hierarchy arrangement. This is not given with
 *  region selections.
 */
class HierarchyBuilder
  : public db::RecursiveShapeReceiver
{
public:

  typedef std::map<std::pair<db::cell_index_type, std::set<db::Box> >, db::cell_index_type> cell_map_type;

  HierarchyBuilder (db::Layout *target, unsigned int target_layer, bool clip_shapes);

  virtual void begin (const RecursiveShapeIterator *iter);
  virtual void end (const RecursiveShapeIterator * /*iter*/);
  virtual void enter_cell (const RecursiveShapeIterator * /*iter*/, const db::Cell * /*cell*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/);
  virtual void leave_cell (const RecursiveShapeIterator * /*iter*/, const db::Cell * /*cell*/);
  virtual bool new_inst (const RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::Box &region, const box_tree_type *complex_region, bool all);
  virtual bool new_inst_member (const RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::ICplxTrans &trans, const db::Box &region, const box_tree_type *complex_region);
  virtual void shape (const RecursiveShapeIterator *iter, const db::Shape &shape, const db::ICplxTrans &trans);

private:
  tl::weak_ptr<db::Layout> mp_target;
  bool m_clip_shapes;
  bool m_initial_pass;
  db::RecursiveShapeIterator m_ref_iter;
  cell_map_type m_cell_map;
  std::set<cell_map_type::key_type> m_cells_seen;
  cell_map_type::const_iterator m_cm_entry;
  unsigned int m_target_layer;
  std::vector<db::Cell *> m_cell_stack;
};

}

#endif

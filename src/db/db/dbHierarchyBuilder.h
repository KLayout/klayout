
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
int DB_PUBLIC compare_iterators_with_respect_to_target_hierarchy (const db::RecursiveShapeIterator &iter1, const db::RecursiveShapeIterator &iter2);

/**
 *  @brief A class to receive shapes from the hierarchy builder
 *
 *  This class can be reimplemented to implement clipping and/or
 *  simplification.
 */
class DB_PUBLIC HierarchyBuilderShapeReceiver
{
public:
  HierarchyBuilderShapeReceiver () { }
  virtual ~HierarchyBuilderShapeReceiver () { }

  virtual void push (const db::Shape &shape, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target) = 0;
  virtual void push (const db::Box &shape, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target) = 0;
  virtual void push (const db::Polygon &shape, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target) = 0;
};

/**
 *  @brief A shape receiver that simply pushes into the target
 */
class DB_PUBLIC HierarchyBuilderShapeInserter
  : public HierarchyBuilderShapeReceiver
{
public:
  HierarchyBuilderShapeInserter () { }

  virtual void push (const db::Shape &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target)
  {
    target->insert (shape);
  }

  virtual void push (const db::Box &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target)
  {
    target->insert (shape);
  }

  virtual void push (const db::Polygon &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target)
  {
    target->insert (shape);
  }
};

/**
 *  @brief A clipping shape receiver that forwards to another one
 */
class DB_PUBLIC ClippingHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  ClippingHierarchyBuilderShapeReceiver (HierarchyBuilderShapeReceiver *pipe = 0);

  virtual void push (const db::Shape &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Box &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Polygon &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);

private:
  void insert_clipped (const db::Box &box, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target);
  void insert_clipped (const db::Polygon &poly, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target);
  static bool is_inside (const db::Box &box, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region);
  static bool is_outside (const db::Box &box, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region);

  HierarchyBuilderShapeReceiver *mp_pipe;
};

/**
 *  @brief A polygon reducing shape receiver that forwards to another one
 */
class DB_PUBLIC ReducingHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  ReducingHierarchyBuilderShapeReceiver (HierarchyBuilderShapeReceiver *pipe = 0, double area_ratio = 3.0, size_t max_vertex_count = 16);

  virtual void push (const db::Shape &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Box &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Polygon &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);

private:
  void reduce (const db::Polygon &poly, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target);

  HierarchyBuilderShapeReceiver *mp_pipe;
  double m_area_ratio;
  size_t m_max_vertex_count;
};

/**
 *  @brief A polygon reference generating shape receiver that feeds a shapes array after turning the shapes into PolygonRefs
 */
class DB_PUBLIC PolygonReferenceHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  PolygonReferenceHierarchyBuilderShapeReceiver (db::Layout *layout);

  virtual void push (const db::Shape &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Box &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Polygon &shape, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);

private:
  db::Layout *mp_layout;
};

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
class DB_PUBLIC HierarchyBuilder
  : public db::RecursiveShapeReceiver
{
public:

  typedef std::map<std::pair<db::cell_index_type, std::set<db::Box> >, db::cell_index_type> cell_map_type;

  HierarchyBuilder (db::Layout *target, unsigned int target_layer, HierarchyBuilderShapeReceiver *pipe = 0);
  HierarchyBuilder (db::Layout *target, HierarchyBuilderShapeReceiver *pipe = 0);
  virtual ~HierarchyBuilder ();

  /**
   *  @brief Installs a custom shape receiver
   *  The hierarchy builder will *NOT* take ownership of this object.
   */
  void set_shape_receiver (HierarchyBuilderShapeReceiver *pipe);

  virtual void begin (const RecursiveShapeIterator *iter);
  virtual void end (const RecursiveShapeIterator *iter);
  virtual void enter_cell (const RecursiveShapeIterator *iter, const db::Cell *cell, const db::Box &region, const box_tree_type *complex_region);
  virtual void leave_cell (const RecursiveShapeIterator *iter, const db::Cell *cell);
  virtual new_inst_mode new_inst (const RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::Box &region, const box_tree_type *complex_region, bool all);
  virtual bool new_inst_member (const RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::ICplxTrans &trans, const db::Box &region, const box_tree_type *complex_region, bool all);
  virtual void shape (const RecursiveShapeIterator *iter, const db::Shape &shape, const db::ICplxTrans &trans, const db::Box &region, const box_tree_type *complex_region);

  /**
   *  @brief Sets the target layer - shapes will be put there
   */
  void set_target_layer (unsigned int target_layer)
  {
    m_target_layer = target_layer;
  }

  /**
   *  @brief Reset the builder - performs a new initial pass
   */
  void reset ();

  /**
   *  @brief Gets the initial cell the builder produced in the target layout
   */
  db::Cell *initial_cell ()
  {
    return mp_initial_cell;
  }

  /**
   *  @brief Gets the target layout
   */
  db::Layout *target ()
  {
    return mp_target.get ();
  }

  /**
   *  @brief Gets the recursive shape iterator the data was taken from
   */
  const db::RecursiveShapeIterator &source () const
  {
    return m_source;
  }

  /**
   *  @brief Gets the iterator for the cell map
   */
  cell_map_type::const_iterator begin_cell_map () const
  {
    return m_cell_map.begin ();
  }

  /**
   *  @brief Gets the iterator for the cell map (end)
   */
  cell_map_type::const_iterator end_cell_map () const
  {
    return m_cell_map.end ();
  }

private:
  tl::weak_ptr<db::Layout> mp_target;
  HierarchyBuilderShapeReceiver *mp_pipe;
  bool m_initial_pass;
  db::RecursiveShapeIterator m_source;
  cell_map_type m_cell_map;
  std::set<cell_map_type::key_type> m_cells_seen;
  cell_map_type::const_iterator m_cm_entry;
  unsigned int m_target_layer;
  std::vector<db::Cell *> m_cell_stack;
  db::Cell *mp_initial_cell;
};

}

#endif

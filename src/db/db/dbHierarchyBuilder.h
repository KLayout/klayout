
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

#ifndef HDR_dbHierarchyBuilder
#define HDR_dbHierarchyBuilder

#include "dbCommon.h"

#include "dbRecursiveShapeIterator.h"
#include "dbLayout.h"
#include "dbLayoutUtils.h"

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

  virtual void push (const db::Shape &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target) = 0;
  virtual void push (const db::Box &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target) = 0;
  virtual void push (const db::Polygon &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target) = 0;
};

/**
 *  @brief A shape receiver that simply pushes into the target
 */
class DB_PUBLIC HierarchyBuilderShapeInserter
  : public HierarchyBuilderShapeReceiver
{
public:
  HierarchyBuilderShapeInserter () { }

  virtual void push (const db::Shape &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target)
  {
    tl::const_map<db::Layout::properties_id_type> pm (prop_id);
    target->insert (shape, trans, pm);
  }

  virtual void push (const db::Box &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target)
  {
    if (trans.is_ortho ()) {
      if (prop_id != 0) {
        target->insert (db::BoxWithProperties (shape.transformed (trans), prop_id));
      } else {
        target->insert (shape.transformed (trans));
      }
    } else {
      if (prop_id != 0) {
        db::PolygonWithProperties poly (db::Polygon (shape), prop_id);
        target->insert (poly.transformed (trans));
      } else {
        db::Polygon poly (shape);
        target->insert (poly.transformed (trans));
      }
    }
  }

  virtual void push (const db::Polygon &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target)
  {
    if (trans.is_unity ()) {
      if (prop_id != 0) {
        target->insert (db::PolygonWithProperties (shape, prop_id));
      } else {
        target->insert (shape);
      }
    } else {
      if (prop_id != 0) {
        target->insert (db::PolygonWithProperties (shape.transformed (trans), prop_id));
      } else {
        target->insert (shape.transformed (trans));
      }
    }
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

  virtual void push (const db::Shape &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Box &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Polygon &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);

private:
  void insert_clipped (const db::Box &box, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target);
  void insert_clipped (const db::Polygon &poly, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target);
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
  ReducingHierarchyBuilderShapeReceiver (HierarchyBuilderShapeReceiver *pipe = 0, double area_ratio = 3.0, size_t max_vertex_count = 16, bool reject_odd_polygons = false);

  virtual void push (const db::Shape &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Box &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Polygon &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);

private:
  void reduce (const db::Polygon &poly, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target, bool check = true);

  HierarchyBuilderShapeReceiver *mp_pipe;
  double m_area_ratio;
  size_t m_max_vertex_count;
  bool m_reject_odd_polygons;
};

/**
 *  @brief A polygon reference generating shape receiver that feeds a shapes array after turning the shapes into PolygonRefs
 */
class DB_PUBLIC PolygonReferenceHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  PolygonReferenceHierarchyBuilderShapeReceiver (db::Layout *layout, const Layout *source_layout, int text_enlargement = -1, const tl::Variant &text_prop_name = tl::Variant ());

  virtual void push (const db::Shape &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Box &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Polygon &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);

private:
  void make_pref (db::Shapes *target, const db::Polygon &polygon, db::properties_id_type prop_id);

  db::Layout *mp_layout;
  int m_text_enlargement;
  bool m_make_text_prop;
  db::property_names_id_type m_text_prop_id;
  db::PropertyMapper m_pm;
};

/**
 *  @brief An edge-generating shape receiver that feeds a shapes array after turning the shapes into edges
 */
class DB_PUBLIC EdgeBuildingHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  EdgeBuildingHierarchyBuilderShapeReceiver (db::Layout *layout, const Layout *source_layout, bool as_edges);

  virtual void push (const db::Shape &shape, properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Box &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Polygon &shape, db::properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);

private:
  bool m_as_edges;
  db::PropertyMapper m_pm;
};

/**
 *  @brief An edge pair-generating shape receiver that feeds a shapes array after turning the shapes into edges
 */
class DB_PUBLIC EdgePairBuildingHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  EdgePairBuildingHierarchyBuilderShapeReceiver (db::Layout *layout, const Layout *source_layout);

  virtual void push (const db::Shape &shape, properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Box &, db::properties_id_type, const db::ICplxTrans &, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *) { }
  virtual void push (const db::Polygon &, db::properties_id_type, const db::ICplxTrans &, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *) { }

private:
  db::PropertyMapper m_pm;
};

/**
 *  @brief An text-generating shape receiver that feeds a shapes array after turning the shapes into texts
 */
class DB_PUBLIC TextBuildingHierarchyBuilderShapeReceiver
  : public HierarchyBuilderShapeReceiver
{
public:
  TextBuildingHierarchyBuilderShapeReceiver (db::Layout *layout, const Layout *source_layout);

  virtual void push (const db::Shape &shape, properties_id_type prop_id, const db::ICplxTrans &trans, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *target);
  virtual void push (const db::Box &, db::properties_id_type, const db::ICplxTrans &, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *) { }
  virtual void push (const db::Polygon &, db::properties_id_type, const db::ICplxTrans &, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *) { }

private:
  db::Layout *mp_layout;
  db::PropertyMapper m_pm;
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
  struct CellMapKey
  {
    CellMapKey ()
      : original_cell (0), inactive (false)
    { }

    CellMapKey (db::cell_index_type _original_cell, bool _inactive, const std::set<db::Box> &_clip_region)
      : original_cell (_original_cell), inactive (_inactive), clip_region (_clip_region)
    { }

    bool operator== (const CellMapKey &other) const
    {
      return original_cell == other.original_cell && inactive == other.inactive && clip_region == other.clip_region;
    }

    bool operator< (const CellMapKey &other) const
    {
      if (original_cell != other.original_cell) { return original_cell < other.original_cell; }
      if (inactive != other.inactive) { return inactive < other.inactive; }
      if (clip_region != other.clip_region) { return clip_region < other.clip_region; }
      return false;
    }

    db::cell_index_type original_cell;
    bool inactive;
    std::set<db::Box> clip_region;
  };


  typedef std::map<CellMapKey, db::cell_index_type> cell_map_type;
  typedef std::map<db::cell_index_type, std::vector<db::cell_index_type> > original_target_to_variants_map_type;
  typedef std::map<db::cell_index_type, db::cell_index_type> variant_to_original_target_map_type;

  HierarchyBuilder (db::Layout *target, unsigned int target_layer, const db::ICplxTrans &trans = db::ICplxTrans (), HierarchyBuilderShapeReceiver *pipe = 0);
  HierarchyBuilder (db::Layout *target, const db::ICplxTrans &trans = db::ICplxTrans (), HierarchyBuilderShapeReceiver *pipe = 0);
  virtual ~HierarchyBuilder ();

  /**
   *  @brief Installs a custom shape receiver
   *  The hierarchy builder will *NOT* take ownership of this object.
   */
  void set_shape_receiver (HierarchyBuilderShapeReceiver *pipe);

  virtual bool wants_all_cells () const { return m_wants_all_cells; }
  virtual void begin (const RecursiveShapeIterator *iter);
  virtual void end (const RecursiveShapeIterator *iter);
  virtual void enter_cell (const RecursiveShapeIterator *iter, const db::Cell *cell, const db::Box &region, const box_tree_type *complex_region);
  virtual void leave_cell (const RecursiveShapeIterator *iter, const db::Cell *cell);
  virtual new_inst_mode new_inst (const RecursiveShapeIterator *iter, const db::CellInstArray &inst, const ICplxTrans &always_apply, const db::Box &region, const box_tree_type *complex_region, bool all);
  virtual bool new_inst_member (const RecursiveShapeIterator *iter, const db::CellInstArray &inst, const ICplxTrans &always_apply, const db::ICplxTrans &trans, const db::Box &region, const box_tree_type *complex_region, bool all);
  virtual void shape (const RecursiveShapeIterator *iter, const db::Shape &shape, const db::ICplxTrans &always_apply, const db::ICplxTrans &trans, const db::Box &region, const box_tree_type *complex_region);

  /**
   *  @brief Sets the target layer - shapes will be put there
   */
  void set_target_layer (unsigned int target_layer)
  {
    m_target_layer = target_layer;
  }

  /**
   *  @brief Sets the target layer - shapes will be put there
   */
  void set_wants_all_cells (bool f)
  {
    m_wants_all_cells = f;
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

  /**
   *  @brief Unmaps an original cell/clip box version from the original-to-working copy cell map
   *
   *  An unmapped cell is never again considered.
   */
  void unmap (const cell_map_type::key_type &k)
  {
    m_cell_map.erase (k);
  }

  /**
   *  @brief Maps an original cell/clip box version to a original-to-working copy cell
   */
  void map (const cell_map_type::key_type &k, db::cell_index_type ci)
  {
    m_cell_map [k] = ci;
  }

  /**
   *  @brief Marks a cell as a variant of another
   *
   *  The first cell is either the original, non-variant target cell or itself a variant.
   *  The second cell will be registered as a variant of the first one.
   */
  void register_variant (db::cell_index_type non_var, db::cell_index_type var);

  /**
   *  @brief Unregisters a cell as a variant
   */
  void unregister_variant (db::cell_index_type var);

  /**
   *  @brief Gets a value indicating whether the given cell is a variant cell
   */
  bool is_variant (db::cell_index_type ci) const
  {
    return m_variants_to_original_target_map.find (ci) != m_variants_to_original_target_map.end ();
  }

  /**
   *  @brief Gets the original target for a variant cell
   */
  db::cell_index_type original_target_for_variant (db::cell_index_type ci) const;

private:
  db::cell_index_type make_cell_variant (const HierarchyBuilder::CellMapKey &key, const std::string &cell_name);

  tl::weak_ptr<db::Layout> mp_target;
  HierarchyBuilderShapeReceiver *mp_pipe;
  bool m_initial_pass;
  db::RecursiveShapeIterator m_source;
  cell_map_type m_cell_map;
  original_target_to_variants_map_type m_original_targets_to_variants_map;
  variant_to_original_target_map_type m_variants_to_original_target_map;

  std::set<cell_map_type::key_type> m_cells_seen;
  std::set<db::cell_index_type> m_cells_to_be_filled;
  cell_map_type::const_iterator m_cm_entry;
  bool m_cm_new_entry;
  unsigned int m_target_layer;
  bool m_wants_all_cells;
  std::vector<std::pair<bool, std::vector<db::Cell *> > > m_cell_stack;
  db::Cell *mp_initial_cell;

  db::ICplxTrans m_trans;
};

}

#endif

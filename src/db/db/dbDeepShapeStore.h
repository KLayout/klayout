
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


#ifndef HDR_dbDeepShapeStore
#define HDR_dbDeepShapeStore

#include "dbCommon.h"

#include "tlObject.h"
#include "tlStableVector.h"
#include "dbLayout.h"
#include "dbRecursiveShapeIterator.h"
#include "dbHierarchyBuilder.h"

#include <set>
#include <map>

namespace db {

class DeepShapeStore;

/**
 *  @brief Represents a shape collection from the deep shape store
 *
 *  This is a lightweight class pointing into the deep shape store.
 *  DeepLayer objects are issued by the DeepShapeStore class.
 */
class DB_PUBLIC DeepLayer
{
public:
  /**
   *  @brief Default constructor
   */
  DeepLayer ();

  /**
   *  @brief Destructor
   */
  ~DeepLayer ();

  /**
   *  @brief Copy constructor
   */
  DeepLayer (const DeepLayer &other);

  /**
   *  @brief Assignment
   */
  DeepLayer &operator= (const DeepLayer &other);

  /**
   *  @brief Gets the layout object
   *  The return value is guaranteed to be non-null.
   */
  db::Layout *layout ();

  /**
   *  @brief Gets the layout object (const version)
   */
  const db::Layout *layout () const;

  /**
   *  @brief Gets the layer
   */
  unsigned int layer () const
  {
    return m_layer;
  }

private:
  friend class DeepShapeStore;

  /**
   *  @brief The constructor
   */
  DeepLayer (DeepShapeStore *store, unsigned int layout, unsigned int layer);

  tl::weak_ptr<DeepShapeStore> mp_store;
  unsigned int m_layout;
  unsigned int m_layer;
};

struct DB_PUBLIC RecursiveShapeIteratorCompareForTargetHierarchy
{
  bool operator () (const db::RecursiveShapeIterator &a, const db::RecursiveShapeIterator &b) const
  {
    return db::compare_iterators_with_respect_to_target_hierarchy (a, b) < 0;
  }
};

/**
 *  @brief The "deep shape store" is a working model for the hierarchical ("deep") processor
 *
 *  The deep shape store keeps temporary data for the deep shape processor.
 *  It mainly consists of layout objects holding the hierarchy trees and layers
 *  for the actual shapes.
 *
 *  The deep shape store provides the basis for working with deep regions. On preparation,
 *  shapes are copied into the deep shape store. After fininishing, the shapes are copied
 *  back into the original layout. The deep shape store provides the methods and
 *  algorithms for doing the preparation and transfer.
 */
class DB_PUBLIC DeepShapeStore
  : public tl::Object
{
public:
  /**
   *  @brief The default constructor
   */
  DeepShapeStore ();

  /**
   *  @brief The destructor
   */
  ~DeepShapeStore ();

  /**
   *  @brief Inserts a polygon layer into the deep shape store
   *
   *  This method will create a new layer inside the deep shape store as a
   *  working copy of the original layer. Preparation involves re-shaping
   *  the polygons so their bounding box is a better approximation and the
   *  polygon complexity is reduced. For this, the polygons are split
   *  into parts satisfying the area ratio (bounding box vs. polygon area)
   *  and maximum vertex count constraints.
   */
  DeepLayer create_polygon_layer (const db::RecursiveShapeIterator &si, double max_area_ratio = 3.0, size_t max_vertex_count = 16);

private:
  typedef std::map<db::RecursiveShapeIterator, unsigned int, RecursiveShapeIteratorCompareForTargetHierarchy> layout_map_type;

  //  no copying
  DeepShapeStore (const DeepShapeStore &);
  DeepShapeStore &operator= (const DeepShapeStore &);

  tl::stable_vector<db::Layout> m_layouts;
  tl::stable_vector<db::HierarchyBuilder> m_builders;
  layout_map_type m_layout_map;
};

}

#endif


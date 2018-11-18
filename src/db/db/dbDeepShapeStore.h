
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
#include "gsiObject.h"

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
   *  @brief Gets the layout object
   *  The return value is guaranteed to be non-null.
   */
  db::Cell *initial_cell ();

  /**
   *  @brief Gets the initial cell object (const version)
   */
  const db::Cell *initial_cell () const;

  /**
   *  @brief Gets the layer
   */
  unsigned int layer () const
  {
    return m_layer;
  }

  /**
   *  @brief Gets the layout index
   */
  unsigned int layout_index () const
  {
    return m_layout;
  }

  /**
   *  @brief Inserts the layer into the given layout, starting from the given cell and into the given layer
   */
  void insert_into (Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer) const;

  /**
   *  @brief Creates a derived new deep layer
   *  Derived layers use the same layout and context, but are initially
   *  empty layers for use as output layers on the same hierarchy.
   */
  DeepLayer derived () const;

  /**
   *  @brief Creates a copy of this layer
   */
  DeepLayer copy () const;

  /**
   *  @brief Gets the shape store object
   *  This is a pure const version to prevent manipulation of the store.
   *  This method is intended to fetch configuration options from the store.
   */
  const DeepShapeStore *store () const
  {
    check_dss ();
    return mp_store.get ();
  }

private:
  friend class DeepShapeStore;

  /**
   *  @brief The constructor
   */
  DeepLayer (DeepShapeStore *store, unsigned int layout, unsigned int layer);

  void check_dss () const;

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
  : public tl::Object, public gsi::ObjectBase
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

  /**
   *  @brief Inserts the deep layer's into some target layout
   */
  void insert (const DeepLayer &layer, db::Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer);

  /**
   *  @brief For testing
   */
  static size_t instance_count ();

  /**
   *  @brief The deep shape store also keeps the number of threads to allocate for the hierarchical processor
   *
   *  This is a kind of hack, but it's convenient.
   */
  void set_threads (int n);

  /**
   *  @brief Gets the number of threads
   */
  int threads () const
  {
    return m_threads;
  }

private:
  friend class DeepLayer;

  db::Layout *layout (unsigned int n)
  {
    return &m_layouts [n];
  }

  typedef std::map<db::RecursiveShapeIterator, unsigned int, RecursiveShapeIteratorCompareForTargetHierarchy> layout_map_type;

  //  no copying
  DeepShapeStore (const DeepShapeStore &);
  DeepShapeStore &operator= (const DeepShapeStore &);

  tl::stable_vector<db::Layout> m_layouts;
  tl::stable_vector<db::HierarchyBuilder> m_builders;
  layout_map_type m_layout_map;
  int m_threads;

  struct DeliveryMappingCacheKey
  {
    //  NOTE: we shouldn't keep pointers here as the layouts may get deleted and recreated with the same address.
    //  But as we don't access these objects that's fairly safe.
    DeliveryMappingCacheKey (unsigned int _from_index, db::Layout *_into_layout, db::cell_index_type _into_cell)
      : from_index (_from_index), into_layout (_into_layout), into_cell (_into_cell)
    {
      //  .. nothing yet ..
    }

    bool operator< (const DeliveryMappingCacheKey &other) const
    {
      if (from_index != other.from_index) {
        return from_index < other.from_index;
      }
      if (into_layout != other.into_layout) {
        return into_layout < other.into_layout;
      }
      return into_cell <other.into_cell;
    }

    unsigned int from_index;
    db::Layout *into_layout;
    db::cell_index_type into_cell;
  };

  std::map<DeliveryMappingCacheKey, db::CellMapping> m_delivery_mapping_cache;
};

}

namespace tl
{

  //  disable copying of the deep shape store object
  template <> struct type_traits <db::DeepShapeStore> : public type_traits<void> {
    typedef tl::false_tag has_copy_constructor;
  };

}

#endif


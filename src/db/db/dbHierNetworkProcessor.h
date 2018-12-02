
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


#ifndef HDR_dbHierNetworkProcessor
#define HDR_dbHierNetworkProcessor

#include "dbCommon.h"
#include "dbTrans.h"
#include "dbBoxConvert.h"
#include "dbBoxTree.h"
#include "dbCell.h"

#include <map>
#include <set>

namespace db {

/**
 *  @brief Defines the connectivity
 *
 *  Connectivity is defined in terms of layers. Certain layer pairs
 *  are connected when shapes on their layers interact.
 *  Connectivity includes intra-layer connectivity - i.e.
 *  shapes on a layer are not connected by default. They need to
 *  be connected explicitly using "connect(layer)".
 */
class DB_PUBLIC Connectivity
{
public:
  typedef std::set<unsigned int> layers_type;
  typedef layers_type::const_iterator layer_iterator;

  /**
   *  @brief Creates a connectivity object without any connections
   */
  Connectivity ();

  /**
   *  @brief Adds inter-layer connectivity
   */
  void connect (unsigned int la, unsigned int lb);

  /**
   *  @brief Adds intra-layer connectivity for layer l
   */
  void connect (unsigned int l);

  /**
   *  @brief Begin iterator for the layers involved
   */
  layer_iterator begin_layers () const;

  /**
   *  @brief End iterator for the layers involved
   */
  layer_iterator end_layers () const;

  /**
   *  @brief Begin iterator for the layers connected to a specific layer
   */
  layer_iterator begin_connected (unsigned int layer) const;

  /**
   *  @brief End iterator for the layers connected to a specific layer
   */
  layer_iterator end_connected (unsigned int layer) const;

  /**
   *  @brief Returns true, if the given shapes on the given layers interact
   *
   *  This method accepts a transformation. This transformation is applied
   *  to the b shape before checking against a.
   */
  template <class T, class Trans>
  bool interacts (const T &a, unsigned int la, const T &b, unsigned int lb, const Trans &trans) const;

  /**
   *  @brief Returns true, if the given shapes on the given layers interact
   */
  template <class T>
  bool interacts (const T &a, unsigned int la, const T &b, unsigned int lb) const
  {
    return interacts (a, la, b, lb, UnitTrans ());
  }

private:
  layers_type m_all_layers;
  std::map<unsigned int, layers_type> m_connected;
};

/**
 *  @brief Represents a cluster of shapes
 *
 *  A cluster of shapes is a set of shapes of type T which are connected in terms
 *  of a given connectivity. The shapes will still be organised in layers.
 */
template <class T>
class DB_PUBLIC local_cluster
{
public:
  typedef size_t id_type;
  typedef typename T::box_type box_type;
  typedef db::unstable_box_tree<box_type, T, db::box_convert<T> > tree_type;
  typedef typename tree_type::flat_iterator shape_iterator;

  /**
   *  @brief Creates an empty cluster
   */
  local_cluster ();

  /**
   *  @brief Clears the cluster
   */
  void clear ();

  /**
   *  @brief Adds a shape with the given layer to the cluster
   */
  void add (const T &s, unsigned int la);

  /**
   *  @brief Joins this cluster with the other one
   *
   *  This will copy all shapes from the other cluster into ourself.
   */
  void join_with (const local_cluster<T> &other);

  /**
   *  @brief Gets the cluster's ID
   *
   *  The ID is a unique identifier for the cluster. An ID value of 0 is reserved for
   *  "no cluster".
   */
  id_type id () const
  {
    return m_id;
  }

  /**
   *  @brief Tests whether this cluster interacts with another cluster
   *
   *  "trans" is the transformation which is applied to the other cluster before
   *  the test.
   */
  bool interacts (const local_cluster<T> &other, const db::ICplxTrans &trans, const Connectivity &conn) const;

  /**
   *  @brief Gets the bounding box of this cluster
   */
  const box_type &bbox () const
  {
    const_cast<local_cluster<T> *> (this)->ensure_sorted ();  //  also updates bbox
    return m_bbox;
  }

  /**
   *  @brief Gets the shape iterator for a given layer
   */
  shape_iterator begin (unsigned int l) const;

private:
  template <typename> friend class local_clusters;
  template <typename> friend class interaction_receiver;

  void set_id (id_type id)
  {
    m_id = id;
  }

  const T &shape (unsigned int l, size_t index) const
  {
    typename std::map<unsigned int, tree_type>::const_iterator s = m_shapes.find (l);
    tl_assert (s != m_shapes.end ());
    return s->second.objects () [index];
  }

  void ensure_sorted ();

  id_type m_id;
  bool m_needs_update;
  std::map<unsigned int, tree_type> m_shapes;
  box_type m_bbox;
};

/**
 *  @brief A box converter for the local_cluster class
 */
template <class T>
struct DB_PUBLIC local_cluster_box_convert
{
  typedef typename local_cluster<T>::box_type box_type;
  typedef typename db::simple_bbox_tag complexity;

  box_type operator() (const local_cluster<T> &c) const
  {
    return c.bbox ();
  }
};

/**
 *  @brief A collection of clusters
 *
 *  Clusters are identified by their ID. This collection
 *  supports cluster lookup by a box region and building
 *  the clusters from a cell's shapes.
 */
template <class T>
class DB_PUBLIC local_clusters
{
public:
  typedef typename local_cluster<T>::box_type box_type;
  typedef db::box_tree<box_type, local_cluster<T>, local_cluster_box_convert<T> > tree_type;
  typedef typename tree_type::touching_iterator touching_iterator;
  typedef typename tree_type::const_iterator const_iterator;

  /**
   *  @brief Creates an empty collection
   */
  local_clusters ();

  /**
   *  @brief Gets the cluster by ID
   */
  const local_cluster<T> &cluster_by_id (typename local_cluster<T>::id_type id) const;

  /**
   *  @brief Clears the clusters
   */
  void clear ();

  /**
   *  @brief Removes a cluster with the given ID
   */
  void remove_cluster (typename local_cluster<T>::id_type id);

  /**
   *  @brief Gets the bounding box of the clusters
   */
  box_type bbox () const
  {
    const_cast<local_clusters<T> *> (this)->ensure_sorted ();
    return m_bbox;
  }

  /**
   *  @brief Gets the clusters (begin iterator)
   */
  const_iterator begin () const
  {
    return m_clusters.begin ();
  }

  /**
   *  @brief Gets the clusters (end iterator)
   */
  const_iterator end () const
  {
    return m_clusters.end ();
  }

  /**
   *  @brief Gets the clusters touching a given region
   */
  touching_iterator begin_touching (const box_type &box) const
  {
    const_cast<local_clusters<T> *> (this)->ensure_sorted ();
    return m_clusters.begin_touching (box, local_cluster_box_convert<T> ());
  }

  /**
   *  @brief Builds this collection from a cell and the given connectivity
   *
   *  This method will only build the local clusters. Child cells
   *  are not taken into account. Only the shape types listed in
   *  shape_flags are taken.
   */
  void build_clusters (const db::Cell &cell, db::ShapeIterator::flags_type shape_flags, const db::Connectivity &conn);

  /**
   *  @brief Creates and inserts a new clusters
   *
   *  NOTE: the object should not be modified after sorting has taken place.
   */
  local_cluster<T> *insert ();

private:
  void ensure_sorted ();

  bool m_needs_update;
  box_type m_bbox;
  tree_type m_clusters;
};

}

#endif

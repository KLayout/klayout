
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


#include "dbHierNetworkProcessor.h"
#include "dbShape.h"
#include "dbShapes.h"
#include "dbInstElement.h"
#include "dbPolygon.h"
#include "dbPolygonTools.h"
#include "dbBoxConvert.h"

#include <vector>
#include <map>

namespace db
{

// ------------------------------------------------------------------------------
//  Connectivity implementation

Connectivity::Connectivity ()
{
  //  .. nothing yet ..
}

void
Connectivity::connect (unsigned int la, unsigned int lb)
{
  m_connected [la].insert (lb);
  m_connected [lb].insert (la);
  m_all_layers.insert (la);
  m_all_layers.insert (lb);
}

void
Connectivity::connect (unsigned int l)
{
  m_connected [l].insert (l);
  m_all_layers.insert (l);
}

Connectivity::layer_iterator
Connectivity::begin_layers ()
{
  return m_all_layers.begin ();
}

Connectivity::layer_iterator
Connectivity::end_layers ()
{
  return m_all_layers.end ();
}

Connectivity::layers_type s_empty_layers;

Connectivity::layer_iterator
Connectivity::begin_connected (unsigned int layer)
{
  std::map<unsigned int, layers_type>::const_iterator i = m_connected.find (layer);
  if (i == m_connected.end ()) {
    return s_empty_layers.begin ();
  } else {
    return i->second.begin ();
  }
}

Connectivity::layer_iterator
Connectivity::end_connected (unsigned int layer)
{
  std::map<unsigned int, layers_type>::const_iterator i = m_connected.find (layer);
  if (i == m_connected.end ()) {
    return s_empty_layers.end ();
  } else {
    return i->second.end ();
  }
}

static bool
interaction_test (const db::PolygonRef &a, const db::PolygonRef &b)
{
  //  TODO: this could be part of db::interact (including transformation)
  if (a.obj ().is_box () && b.obj ().is_box ()) {
    return db::interact (a.obj ().box ().transformed (b.trans ().inverted () * a.trans ()), b.obj ().box ());
  } else {
    return db::interact (a.obj ().transformed (b.trans ().inverted () * a.trans ()), b.obj ());
  }
}

template <class T>
bool Connectivity::interacts (const T &a, unsigned int la, T &b, unsigned int lb) const
{
  std::map<unsigned int, layers_type>::const_iterator i = m_connected.find (la);
  if (i == m_connected.end () || i->second.find (lb) == i->second.end ()) {
    return false;
  } else {
    return interaction_test (a, b);
  }
}

//  explicit instantiations
template DB_PUBLIC bool Connectivity::interacts<db::PolygonRef> (const db::PolygonRef &a, unsigned int la, db::PolygonRef &b, unsigned int lb) const;

// ------------------------------------------------------------------------------
//  ...


#if 0
/**
 *  @brief Represents a cluster of shapes
 *
 *  A cluster of shapes is a set of shapes which are connected in terms
 *  of a given connectivity.
 */
class LocalCluster
{
public:
  typedef size_t id_type;

  LocalCluster ();

  void connect (const db::Shape &a, unsigned int la, const db::Shape &b, unsigned int lb);
  id_type id () const;

  // @@@ Trans is the transformation of the cluster to the shape (instance transformation)
  void interacts (const db::Shape &s, unsigned int ls, const db::ICplxTrans &trans, const Connectivity &conn);
  const db::Box &bbox () const;

private:
  friend class LocalClusters;

  void set_id (id_type id);
};

/**
 *  @brief Represents a collection of clusters in a cell
 *
 *  After all shapes in a cell are connected, the cluster collection is filled if
 *  disconnected clusters.
 */
class LocalClusters
{
public:
  LocalClusters ();

  //  @@@ needs to be fast(!)
  LocalCluster::id_type cluster_id_for_shape (const db::Shape &s, unsigned int ls) const;
  const LocalCluster &cluster (LocalCluster::id_type id) const;
  LocalCluster &create_cluster ();
  void remove_cluster (LocalCluster::id_type id);
  cluster_iterator find (const db::Box &region);

  // @@@ Trans is the transformation of the clusters to the shape (instance transformation)
  std::vector<unsigned int> interacting_clusters (const db::Shape &s, unsigned int ls, const db::ICplxTrans &trans, const Connectivity &conn) const;
  // @@@ Trans is the transformation of the clusters to the clusters looked up (instance transformation)
  std::vector<unsigned int> interacting_clusters (const LocalClusters &c, const db::ICplxTrans &trans, const Connectivity &conn) const;
};

/**
 *  @brief Represents all clusters in a cell and their connections to child cells
 *
 *  Connections to indirect children are made through empty dummy clusters.
 *  Also, connections between two clusters of different children without
 *  mediating cluster on cell level are made through empty dummy clusters.
 *  Hence, there is always a cluster in the parent cell which connects to
 *  clusters from child cells.
 */
class HierarchicalClusters
{
public:
  HierarchicalClusters ();

  LocalClusters &local ();

  //  build local clusters
  //  determine local to cell cluster interactions -> top-down connection, maybe across dummy
  //  identify cell overlaps
  //  determine cell-to-cell cluster interactions -> make dummy cluster to connect both, connect to each child
  //    maybe across dummy cluster
  //  shall be called bottom-up
  void build_clusters (const db::Cell &cell, const Connectivity &conn);

  //  @@@ trans is the transformation from child to this (instance transformation)
  //  used by child clusters to verify whether they are connected somewhere in the parent
  LocalCluster::id_type is_connected_to (LocalCluster::id_type id, const db::InstElement &trans) const;

  //  connections to subcells (== pins?)
  const std::vector<std::pair<LocalCluster::id_type, db::InstElement> > &top_down_connections (LocalCluster::id_type) const;

  //  propagate all connected clusters to their parents
  //  -> creates new local clusters, removes them from their local set
  //  used by the merge step to form local-only clusters
  //  can be called bottom-up
  void propagate_connected ();
};
#endif

}

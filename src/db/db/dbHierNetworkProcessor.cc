
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
#include "dbBoxScanner.h"

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

template <class Trans>
static bool
interaction_test (const db::PolygonRef &a, const db::PolygonRef &b, const Trans &trans)
{
  //  TODO: this could be part of db::interact (including transformation)
  if (a.obj ().is_box () && b.obj ().is_box ()) {
    return db::interact (a.obj ().box ().transformed (b.trans ().inverted () * a.trans ()), b.obj ().box ().transformed (trans));
  } else {
    return db::interact (a.obj ().transformed (b.trans ().inverted () * a.trans ()), b.obj ().transformed (trans));
  }
}

template <class T, class Trans>
bool Connectivity::interacts (const T &a, unsigned int la, const T &b, unsigned int lb, const Trans &trans) const
{
  std::map<unsigned int, layers_type>::const_iterator i = m_connected.find (la);
  if (i == m_connected.end () || i->second.find (lb) == i->second.end ()) {
    return false;
  } else {
    return interaction_test (a, b, trans);
  }
}

//  explicit instantiations
template DB_PUBLIC bool Connectivity::interacts<db::PolygonRef> (const db::PolygonRef &a, unsigned int la, const db::PolygonRef &b, unsigned int lb, const db::UnitTrans &trans) const;
template DB_PUBLIC bool Connectivity::interacts<db::PolygonRef> (const db::PolygonRef &a, unsigned int la, const db::PolygonRef &b, unsigned int lb, const db::ICplxTrans &trans) const;

// ------------------------------------------------------------------------------
//  local_cluster implementation

template <class T>
local_cluster<T>::local_cluster ()
  : m_id (0), m_needs_update (false)
{
  //  .. nothing yet ..
}

template <class T>
void
local_cluster<T>::clear ()
{
  m_shapes.clear ();
  m_needs_update = false;
  m_bbox = box_type ();
}

template <class T>
void
local_cluster<T>::add (const T &s, unsigned int la)
{
  m_shapes[la].insert (s);
  m_needs_update = true;
}

template <class T>
void
local_cluster<T>::join_with (const local_cluster<T> &other)
{
  for (typename std::map<unsigned int, tree_type>::const_iterator s = other.m_shapes.begin (); s != other.m_shapes.end (); ++s) {
    tree_type &other_tree = m_shapes[s->first];
    other_tree.insert (s->second.begin (), s->second.end ());
  }

  m_needs_update = true;
}

template <class T>
void
local_cluster<T>::ensure_sorted ()
{
  if (! m_needs_update) {
    return;
  }

  //  sort the shape trees
  for (typename std::map<unsigned int, tree_type>::iterator s = m_shapes.begin (); s != m_shapes.end (); ++s) {
    s->second.sort (db::box_convert<T> ());
  }

  //  recompute bounding box
  m_bbox = box_type ();
  db::box_convert<T> bc;
  for (typename std::map<unsigned int, tree_type>::const_iterator s = m_shapes.begin (); s != m_shapes.end (); ++s) {
    for (typename tree_type::const_iterator i = s->second.begin (); i != s->second.end (); ++i) {
      m_bbox += bc (*i);
    }
  }

  m_needs_update = false;
}

namespace
{

template <class T>
struct interaction_receiver
  : public box_scanner_receiver2<T, unsigned int, T, unsigned int>
{
public:
  typedef typename local_cluster<T>::box_type box_type;

  interaction_receiver (const Connectivity &conn, const db::ICplxTrans &trans)
    : mp_conn (&conn), m_any (false), m_trans (trans)
  {
    //  .. nothing yet ..
  }

  void add (const T *s1, unsigned int l1, const T *s2, unsigned int l2)
  {
    if (mp_conn->interacts (*s1, l1, *s2, l2, m_trans)) {
      m_any = true;
    }
  }

  bool stop () const
  {
    return m_any;
  }

private:
  const Connectivity *mp_conn;
  bool m_any;
  db::ICplxTrans m_trans;
};

template <class T, class Trans>
struct transformed_box
{
  typedef db::box_convert<T> base_bc;
  typedef typename T::box_type box_type;

  transformed_box (const Trans &trans)
    : m_trans (trans)
  {
    //  .. nothing yet ..
  }

  box_type operator() (const T &t) const
  {
    return m_bc (t).transformed (m_trans);
  }

private:
  base_bc m_bc;
  Trans m_trans;
};

}

template <class T>
bool
local_cluster<T>::interacts (const local_cluster<T> &other, const db::ICplxTrans &trans, const Connectivity &conn) const
{
  const_cast<local_cluster<T> *> (this)->ensure_sorted ();

  if (! other.bbox ().touches (bbox ())) {
    return false;
  }

  box_type common = other.bbox () & bbox ();

  db::box_scanner2<T, unsigned int, T, unsigned int> scanner;
  transformed_box <T, db::ICplxTrans> bc_t (trans);
  db::box_convert<T> bc;

  bool any = false;
  for (typename std::map<unsigned int, tree_type>::const_iterator s = m_shapes.begin (); s != m_shapes.end (); ++s) {
    for (typename tree_type::touching_iterator i = s->second.begin_touching (common, bc); ! i.at_end (); ++i) {
      scanner.insert1 (i.operator-> (), s->first);
      any = true;
    }
  }

  if (! any) {
    return false;
  }

  for (typename std::map<unsigned int, tree_type>::const_iterator s = other.m_shapes.begin (); s != other.m_shapes.end (); ++s) {
    for (typename tree_type::touching_iterator i = s->second.begin_touching (common.transformed (trans.inverted ()), bc); ! i.at_end (); ++i) {
      scanner.insert2 (i.operator-> (), s->first);
    }
  }

  interaction_receiver<T> rec (conn, trans);
  return ! scanner.process (rec, 1 /*==touching*/, bc, bc_t);
}

//  explicit instantiations
template class DB_PUBLIC local_cluster<db::PolygonRef>;


// ------------------------------------------------------------------------------
//  local_cluster implementation


#if 0
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

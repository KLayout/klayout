
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
Connectivity::begin_layers () const
{
  return m_all_layers.begin ();
}

Connectivity::layer_iterator
Connectivity::end_layers () const
{
  return m_all_layers.end ();
}

Connectivity::layers_type s_empty_layers;

Connectivity::layer_iterator
Connectivity::begin_connected (unsigned int layer) const
{
  std::map<unsigned int, layers_type>::const_iterator i = m_connected.find (layer);
  if (i == m_connected.end ()) {
    return s_empty_layers.begin ();
  } else {
    return i->second.begin ();
  }
}

Connectivity::layer_iterator
Connectivity::end_connected (unsigned int layer) const
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
typename local_cluster<T>::shape_iterator local_cluster<T>::begin (unsigned int l) const
{
  static tree_type s_empty_tree;

  typename std::map<unsigned int, tree_type>::const_iterator i = m_shapes.find (l);
  if (i == m_shapes.end ()) {
    return s_empty_tree.begin_flat ();
  } else {
    return i->second.begin_flat ();
  }
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
//  local_clusters implementation

template <class T>
local_clusters<T>::local_clusters ()
  : m_needs_update (false)
{
  //  .. nothing yet ..
}

template <class T>
void local_clusters<T>::clear ()
{
  m_needs_update = false;
  m_clusters.clear ();
  m_bbox = box_type ();
}

template <class T>
const local_cluster<T> &
local_clusters<T>::cluster_by_id (typename local_cluster<T>::id_type id) const
{
  //  by convention the ID is the index + 1 so 0 can be used as "nil"
  tl_assert (id > 0 && id <= m_clusters.size ());
  return m_clusters.objects ().item (id - 1);
}

template <class T>
void
local_clusters<T>::remove_cluster (typename local_cluster<T>::id_type id)
{
  tl_assert (id > 0 && id <= m_clusters.size ());
  //  TODO: get rid of this const_cast by providing "delete by index"
  //  m_clusters.erase (id - 1)
  local_cluster<T> *to_delete = const_cast<local_cluster<T> *> (& m_clusters.objects ().item (id - 1));
  m_clusters.erase (m_clusters.iterator_from_pointer (to_delete));
  m_needs_update = true;
}

template <class T>
local_cluster<T> *
local_clusters<T>::insert ()
{
  typename tree_type::iterator i = m_clusters.insert (local_cluster<T> ());
  i->set_id (i.index () + 1);
  m_needs_update = true;
  return i.operator-> ();
}

template <class T>
void
local_clusters<T>::ensure_sorted ()
{
  if (! m_needs_update) {
    return;
  }

  //  sort the shape trees
  m_clusters.sort (local_cluster_box_convert<T> ());

  //  recompute bounding box
  m_bbox = box_type ();
  for (typename tree_type::const_iterator i = m_clusters.begin (); i != m_clusters.end (); ++i) {
    m_bbox += i->bbox ();
  }

  m_needs_update = false;
}

namespace
{

template <class T, class BoxTree>
struct cluster_building_receiver
  : public db::box_scanner_receiver<T, unsigned int>
{
  typedef typename local_cluster<T>::id_type id_type;

  cluster_building_receiver (local_clusters<T> &clusters, const db::Connectivity &conn)
    : mp_clusters (&clusters), mp_conn (&conn)
  {
    //  .. nothing yet..
  }

  void add (const T *s1, unsigned int l1, const T *s2, unsigned int l2)
  {
    if (! mp_conn->interacts (*s1, l1, *s2, l2)) {
      return;
    }

    typename std::map<const T *, id_type>::const_iterator id1 = m_shape_to_cluster_id.find (s1);
    typename std::map<const T *, id_type>::const_iterator id2 = m_shape_to_cluster_id.find (s2);

    if (id1 == m_shape_to_cluster_id.end ()) {

      if (id2 == m_shape_to_cluster_id.end ()) {

        local_cluster<T> *cluster = mp_clusters->insert ();
        cluster->add (*s1, l1);
        cluster->add (*s2, l2);

        m_shape_to_cluster_id.insert (std::make_pair (s1, cluster->id ()));
        m_shape_to_cluster_id.insert (std::make_pair (s2, cluster->id ()));

      } else {

        //  NOTE: const_cast is in order - we know what we're doing.
        const_cast<local_cluster<T> &> (mp_clusters->cluster_by_id (id2->second)).add (*s1, l1);
        m_shape_to_cluster_id.insert (std::make_pair (s1, id2->second));

      }

    } else if (id2 == m_shape_to_cluster_id.end ()) {

      //  NOTE: const_cast is in order - we know what we're doing.
      const_cast<local_cluster<T> &> (mp_clusters->cluster_by_id (id1->second)).add (*s2, l2);
      m_shape_to_cluster_id.insert (std::make_pair (s2, id1->second));

    } else if (id1->second != id2->second) {

      //  this shape connects two clusters: join them
      //  NOTE: const_cast is in order - we know what we're doing.
      const_cast<local_cluster<T> &> (mp_clusters->cluster_by_id (id1->second)).join_with (mp_clusters->cluster_by_id (id2->second));
      mp_clusters->remove_cluster (id2->second);

    }
  }

  void finish (const T *s, unsigned int l)
  {
    //  if the shape has not been handled yet, insert a single cluster with only this shape
    typename std::map<const T *, id_type>::const_iterator id = m_shape_to_cluster_id.find (s);
    if (id == m_shape_to_cluster_id.end ()) {
      local_cluster<T> *cluster = mp_clusters->insert ();
      cluster->add (*s, l);
    }
  }

private:
  local_clusters<T> *mp_clusters;
  const db::Connectivity *mp_conn;
  std::map<const T *, id_type> m_shape_to_cluster_id;
};

}

template <class T>
void
local_clusters<T>::build_clusters (const db::Cell &cell, db::ShapeIterator::flags_type shape_flags, const db::Connectivity &conn)
{
  db::box_scanner<T, unsigned int> bs;
  typename T::tag object_tag;
  db::box_convert<T> bc;

  for (db::Connectivity::layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {
    const db::Shapes &shapes = cell.shapes (*l);
    for (db::Shapes::shape_iterator s = shapes.begin (shape_flags); ! s.at_end (); ++s) {
      bs.insert (s->basic_ptr (object_tag), *l);
    }
  }

  cluster_building_receiver<T, box_type> rec (*this, conn);
  bs.process (rec, 1 /*==touching*/, bc);
}

//  explicit instantiations
template class DB_PUBLIC local_clusters<db::PolygonRef>;

// ------------------------------------------------------------------------------
//  hier_clusters implementation

/**
 *  @brief A connection to a cluster in a child instance
 */
class DB_PUBLIC ClusterInstance
{
public:
  typedef std::vector<db::InstElement> inst_path_type;

  ClusterInstance (size_t id, const inst_path_type &inst_path)
    : m_id (id), m_inst_path (inst_path)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the cluster ID
   */
  size_t id () const
  {
    return m_id;
  }

  /**
   *  @brief Gets the instance path
   */
  const inst_path_type &inst () const
  {
    return m_inst_path;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const ClusterInstance &other) const
  {
    return m_id == other.m_id && m_inst_path == other.m_inst_path;
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const ClusterInstance &other) const
  {
    if (m_id != other.m_id) {
      return m_id < other.m_id;
    }
    return m_inst_path < other.m_inst_path;
  }

private:
  size_t m_id;
  inst_path_type m_inst_path;
};

template <class T> class hier_clusters;

/**
 *  @brief Local clusters with connections to clusters from child cells
 */
template <class T>
class DB_PUBLIC connected_clusters
  : public local_clusters<T>
{
public:
  typedef std::list<ClusterInstance> connections_type;
  typedef typename local_clusters<T>::box_type box_type;

  /**
   *  @brief Constructor
   */
  connected_clusters ()
    : local_clusters<T> ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the connections for a given cluster ID
   */
  const connections_type &connections_for_cluster (typename local_cluster<T>::id_type id);

  /**
   *  @brief Adds a connection between a local cluster and one from a child instance
   */
  void add_connection (typename local_cluster<T>::id_type, const ClusterInstance &inst);

  /**
   *  @brief Joins all connections of with_id to id
   */
  void join_connected_clusters (typename local_cluster<T>::id_type id, typename local_cluster<T>::id_type with_id);

  /**
   *  @brief Reverse "connections_for_cluster"
   *
   *  Finds the cluster which has a connection given by inst. "strip" elements
   *  are stipped from the front of the instantiation path.
   *  This method returns 0 if no cluster can be found.
   */
  typename local_cluster<T>::id_type find_cluster_with_connection (const ClusterInstance &inst, size_t strip) const;

private:
  std::map<typename local_cluster<T>::id_type, connections_type> m_connections;
  box_type m_full_bbox;
};

template <class T>
const typename connected_clusters<T>::connections_type &
connected_clusters<T>::connections_for_cluster (typename local_cluster<T>::id_type id)
{
  typename std::map<typename local_cluster<T>::id_type, connections_type>::const_iterator c = m_connections.find (id);
  if (c == m_connections.end ()) {
    static connections_type empty_connections;
    return empty_connections;
  } else {
    return c->second;
  }
}

template <class T>
void
connected_clusters<T>::add_connection (typename local_cluster<T>::id_type id, const ClusterInstance &inst)
{
  m_connections [id].push_back (inst);
}

template <class T>
void
connected_clusters<T>::join_connected_clusters (typename local_cluster<T>::id_type id, typename local_cluster<T>::id_type with_id)
{
  if (id == with_id) {
    return;
  }

  const connections_type &to_join = connections_for_cluster (with_id);
  connections_type &target = m_connections [id];
  target.insert (target.end (), to_join.begin (), to_join.end ());
  m_connections.erase (with_id);
}

template <class T>
typename local_cluster<T>::id_type
connected_clusters<T>::find_cluster_with_connection (const ClusterInstance &ci, size_t strip) const
{
  for (typename std::map<typename local_cluster<T>::id_type, connections_type>::const_iterator i = m_connections.begin (); i != m_connections.end (); ++i) {

    for (connections_type::const_iterator j = i->second.begin (); j != i->second.end (); ++j) {

      const ClusterInstance::inst_path_type &path = j->inst ();
      if (j->id () == ci.id () && path.size () == ci.inst ().size () - strip) {

        bool match = true;
        for (size_t k = 0; k < path.size () && match; ++k) {
          match = (path [k] == ci.inst () [k + strip]);
        }
        if (match) {
          return i->first;
        }

      }

    }

  }

  return 0;
}

template <typename> class cell_box_converter;

/**
 *  @brief A hierarchical representation of clusters
 *
 *  Hierarchical clusters
 */
template <class T>
class DB_PUBLIC hier_clusters
{
public:
  typedef typename local_cluster<T>::box_type box_type;

  /**
   *  @brief Creates an empty set of clusters
   */
  hier_clusters ();

  /**
   *  @brief Builds a hierarchy of clusters from a cell hierarchy and given connectivity
   */
  void build (const db::Layout &layout, const db::Cell &cell, db::ShapeIterator::flags_type shape_flags, const db::Connectivity &conn);

  /**
   *  @brief Gets the connected clusters for a given cell
   */
  const connected_clusters<T> &clusters_per_cell (db::cell_index_type cell_index) const;

  /**
   *  @brief Clears this collection
   */
  void clear ();

private:
  void do_build (cell_box_converter<T> &cbc, const db::Layout &layout, const db::Cell &cell, db::ShapeIterator::flags_type shape_flags, const db::Connectivity &conn);

  std::map<db::cell_index_type, connected_clusters<T> > m_per_cell_clusters;
};

template <class T>
class DB_PUBLIC cell_box_converter
{
public:
  typedef db::simple_bbox_tag complexity;
  typedef typename hier_clusters<T>::box_type box_type;

  cell_box_converter (const db::Layout &layout, const hier_clusters<T> &tree)
    : mp_layout (&layout), mp_tree (&tree)
  {
    //  .. nothing yet ..
  }

  const box_type &operator() (db::cell_index_type cell_index) const
  {
    typename std::map<db::cell_index_type, box_type>::const_iterator b = m_cache.find (cell_index);
    if (b != m_cache.end ()) {

      return b->second;

    } else {

      const connected_clusters<T> &clusters = mp_tree->clusters_per_cell (cell_index);
      box_type box = clusters.bbox ();

      const db::Cell &cell = mp_layout->cell (cell_index);
      for (db::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
        const db::CellInstArray &inst_array = inst->cell_inst ();
        box += inst_array.raw_bbox () * (*this) (inst_array.object ().cell_index ());
      }

      return m_cache.insert (std::make_pair (cell_index, box)).first->second;

    }
  }

private:
  mutable std::map<db::cell_index_type, box_type> m_cache;
  const db::Layout *mp_layout;
  const hier_clusters<T> *mp_tree;
};

template <class T>
hier_clusters<T>::hier_clusters ()
{
  //  .. nothing yet ..
}

template <class T>
void hier_clusters<T>::clear ()
{
  m_per_cell_clusters.clear ();
}

template <class T>
void
hier_clusters<T>::build (const db::Layout &layout, const db::Cell &cell, db::ShapeIterator::flags_type shape_flags, const db::Connectivity &conn)
{
  clear ();
  cell_box_converter<T> cbc (layout, *this);
  do_build (cbc, layout, cell, shape_flags, conn);
}

namespace
{

template <class T>
struct hc_receiver
  : public db::box_scanner_receiver<db::Instance, unsigned int>,
    public db::box_scanner_receiver2<local_cluster<T>, unsigned int, db::Instance, unsigned int>
{
public:
  typedef typename hier_clusters<T>::box_type box_type;
  typedef typename local_cluster<T>::id_type id_type;
  typedef std::map<ClusterInstance, local_cluster<T> *> connector_map;

  hc_receiver (const db::Layout &layout, db::connected_clusters<T> &cell_clusters, hier_clusters<T> &tree, const cell_box_converter<T> &cbc, const db::Connectivity &conn)
    : mp_layout (&layout), mp_tree (&tree), mp_cbc (&cbc), mp_conn (&conn)
  {
    mp_cell_clusters = &cell_clusters;
  }

  void add (const db::Instance *i1, unsigned int /*p1*/, const db::Instance *i2, unsigned int /*p2*/)
  {
    std::vector<db::InstElement> p;
    db::ICplxTrans t;
    add_pair (*i1, p, t, *i2, p, t);
  }

  void add (const local_cluster<T> *c1, unsigned int /*p1*/, const db::Instance *i2, unsigned int /*p2*/)
  {
    std::vector<db::InstElement> p;
    db::ICplxTrans t;
    add_pair (*c1, *i2, p, t);
  }

  bool stop () const
  {
    return false;
  }

private:
  const db::Layout *mp_layout;
  db::connected_clusters<T> *mp_cell_clusters;
  hier_clusters<T> *mp_tree;
  const cell_box_converter<T> *mp_cbc;
  const db::Connectivity *mp_conn;
  connector_map m_connectors;
  mutable std::map<ClusterInstance, ClusterInstance> m_reduction_cache;

  void add_pair (const db::Instance &i1, const std::vector<db::InstElement> &p1, const db::ICplxTrans &t1, const db::Instance &i2, const std::vector<db::InstElement> &p2, const db::ICplxTrans &t2)
  {
    box_type bb1 = (*mp_cbc) (i1.cell_index ());
    box_type b1 = (i1.cell_inst ().raw_bbox () * bb1).transformed (t1);

    box_type bb2 = (*mp_cbc) (i2.cell_index ());
    box_type b2 = (i2.cell_inst ().raw_bbox () * bb2).transformed (t2);

    if (! b1.touches (b2)) {
      return;
    }

    db::ICplxTrans t2i = t2.inverted ();
    db::ICplxTrans t12 = t2i * t1;

    box_type common = b1 & b2;

    for (db::CellInstArray::iterator ii1 = i1.begin_touching (common, mp_layout); ! ii1.at_end (); ++ii1) {

      db::ICplxTrans tt1 = t1 * i1.complex_trans (*ii1);
      box_type ib1 = bb1.transformed (tt1);
      box_type ib12 = ib1.transformed (t12);

      ClusterInstance::inst_path_type pp1;
      pp1.reserve (p1.size () + 1);
      pp1.insert (pp1.end (), p1.begin (), p1.end ());
      pp1.push_back (db::InstElement (i1, ii1));

      for (db::CellInstArray::iterator ii2 = i2.begin_touching (ib12, mp_layout); ! ii2.at_end (); ++ii2) {

        db::ICplxTrans tt2 = t2 * i2.complex_trans (*ii2);
        box_type ib2 = bb2.transformed (tt2);

        if (ib1.touches (ib2)) {

          ClusterInstance::inst_path_type pp2;
          pp2.reserve (p2.size () + 1);
          pp2.insert (pp2.end (), p2.begin (), p2.end ());
          pp2.push_back (db::InstElement (i2, ii2));

          add_single_pair (common, i1.cell_index (), pp1, tt1, i2.cell_index (), pp2, tt2);

          //  dive into cell of ii2
          const db::Cell &cell2 = mp_layout->cell (i2.cell_index ());
          for (db::Cell::touching_iterator jj2 = cell2.begin_touching (common.transformed (tt2.inverted ())); ! jj2.at_end (); ++jj2) {
            add_pair (i1, p1, t1, *jj2, pp2, tt2);
          }

        }

      }

      //  dive into cell of ii1
      const db::Cell &cell1 = mp_layout->cell (i1.cell_index ());
      for (db::Cell::touching_iterator jj1 = cell1.begin_touching (common.transformed (tt1.inverted ())); ! jj1.at_end (); ++jj1) {
        add_pair (*jj1, pp1, tt1, i2, p2, t2);
      }

    }
  }

  void add_single_pair (const box_type &common,
                        db::cell_index_type ci1, const std::vector<db::InstElement> &p1, const db::ICplxTrans &t1,
                        db::cell_index_type ci2, const std::vector<db::InstElement> &p2, const db::ICplxTrans &t2)
  {
    const db::local_clusters<T> &cl1 = mp_tree->clusters_per_cell (ci1);
    const db::local_clusters<T> &cl2 = mp_tree->clusters_per_cell (ci2);

    db::ICplxTrans t1i = t1.inverted ();
    db::ICplxTrans t2i = t2.inverted ();
    db::ICplxTrans t21 = t1i * t2;

    for (typename db::local_clusters<T>::touching_iterator i = cl1.begin_touching (common.transformed (t1i)); ! i.at_end (); ++i) {

      box_type bc1 = common & i->bbox ().transformed (t1);
      for (typename db::local_clusters<T>::touching_iterator j = cl2.begin_touching (bc1.transformed (t2i)); ! j.at_end (); ++j) {

        if (i->interacts (*j, t21, *mp_conn)) {

          ClusterInstance k1 (i->id (), p1);
          reduce (k1);

          ClusterInstance k2 (j->id (), p2);
          reduce (k2);

          typename connector_map::iterator x1 = m_connectors.find (k1);
          typename connector_map::iterator x2 = m_connectors.find (k2);

          if (x1 == m_connectors.end ()) {

            if (x2 == m_connectors.end ()) {

              db::local_cluster<T> *connector = mp_cell_clusters->insert ();
              m_connectors [k1] = connector;
              mp_cell_clusters->add_connection (connector->id (), k1);
              mp_cell_clusters->add_connection (connector->id (), k2);

            } else {
              mp_cell_clusters->add_connection (x2->second->id (), k1);
            }

          } else if (x2 == m_connectors.end ()) {

            mp_cell_clusters->add_connection (x1->second->id (), k2);

          } else if (x1->second != x2->second) {

            mp_cell_clusters->join_connected_clusters (x1->second->id (), x2->second->id ());
            mp_cell_clusters->remove_cluster (x2->second->id ());
            x2->second = x1->second;

          }

        }

      }

    }

  }

  void add_pair (const local_cluster<T> &c1, const db::Instance &i2, const std::vector<db::InstElement> &p2, const db::ICplxTrans &t2)
  {
    box_type b1 = c1.bbox ();

    box_type bb2 = (*mp_cbc) (i2.cell_index ());
    box_type b2 = (i2.cell_inst ().raw_bbox () * bb2).transformed (t2);

    if (! b1.touches (b2)) {
      return;
    }

    box_type common = b1 & b2;

    for (db::CellInstArray::iterator ii2 = i2.begin_touching (common.transformed (t2.inverted ()), mp_layout); ! ii2.at_end (); ++ii2) {

      db::ICplxTrans tt2 = t2 * i2.complex_trans (*ii2);
      box_type ib2 = bb2.transformed (tt2);

      if (b1.touches (ib2)) {

        ClusterInstance::inst_path_type pp2;
        pp2.reserve (p2.size () + 1);
        pp2.insert (pp2.end (), p2.begin (), p2.end ());
        pp2.push_back (db::InstElement (i2, ii2));

        add_single_pair (c1, i2.cell_index (), pp2, tt2);

        //  dive into cell of ii2
        const db::Cell &cell2 = mp_layout->cell (i2.cell_index ());
        for (db::Cell::touching_iterator jj2 = cell2.begin_touching (common.transformed (tt2.inverted ())); ! jj2.at_end (); ++jj2) {
          add_pair (c1, *jj2, pp2, tt2);
        }

      }

    }
  }

  void add_single_pair (const local_cluster<T> &c1,
                        db::cell_index_type ci2, const std::vector<db::InstElement> &p2, const db::ICplxTrans &t2)
  {
    const db::local_clusters<T> &cl2 = mp_tree->clusters_per_cell (ci2);

    for (typename db::local_clusters<T>::touching_iterator j = cl2.begin_touching (c1.bbox ().transformed (t2.inverted ())); ! j.at_end (); ++j) {

      if (c1.interacts (*j, t2, *mp_conn)) {

        ClusterInstance k2 (j->id (), p2);
        reduce (k2);

        mp_cell_clusters->add_connection (c1.id (), k2);

      }

    }
  }

  /**
   *  @brief Reduce the connection path to the connector highest in the hierarchy
   *
   *  This feature shall prevent a situation where a connection is made to a sub-cluster of
   *  another hierarchical cluster. We always attach to the highest possible connector.
   */
  void reduce (ClusterInstance &k) const
  {
    std::map<ClusterInstance, ClusterInstance>::const_iterator kr = m_reduction_cache.find (k);
    if (kr != m_reduction_cache.end ()) {
      k = kr->second;
      return;
    }

    for (size_t rn = 1; rn + 1 < k.inst ().size (); ++rn) {

      const db::connected_clusters<T> &clusters = mp_tree->clusters_per_cell (k.inst () [rn - 1].inst_ptr.cell_index ());
      id_type red_id = clusters.find_cluster_with_connection (k, rn);
      if (red_id > 0) {

        //  reduction possible
        ClusterInstance::inst_path_type new_path;
        new_path.insert (new_path.end (), k.inst ().begin (), k.inst ().begin () + rn);
        k = ClusterInstance (red_id, new_path);
        return;

      }

    }

  }
};

template <class T>
struct cell_inst_clusters_box_converter
{
  typedef typename local_cluster<T>::box_type box_type;
  typedef db::simple_bbox_tag complexity;

  cell_inst_clusters_box_converter (const cell_box_converter<T> &cbc)
    : mp_cbc (&cbc)
  {
    //  .. nothing yet ..
  }

  box_type operator() (const db::Instance &inst) const
  {
    return inst.cell_inst ().raw_bbox () * (*mp_cbc) (inst.cell_index ());
  }

private:
  const cell_box_converter<T> *mp_cbc;
};

}

template <class T>
void
hier_clusters<T>::do_build (cell_box_converter<T> &cbc, const db::Layout &layout, const db::Cell &cell, db::ShapeIterator::flags_type shape_flags, const db::Connectivity &conn)
{
  //  already done - don't do again
  if (m_per_cell_clusters.find (cell.cell_index ()) != m_per_cell_clusters.end ()) {
    return;
  }

  //  Build local clusters

  connected_clusters<T> &local = m_per_cell_clusters [cell.cell_index ()];
  local.build_clusters (cell, shape_flags, conn);

  //  handle connections inside child cells in a bottom-up fashion

  for (db::Cell::child_cell_iterator cc = cell.begin_child_cells (); ! cc.at_end (); ++cc) {
    do_build (cbc, layout, layout.cell (*cc), shape_flags, conn);
  }

  //  NOTE: this is a receiver for both the child-to-child and
  //  local to child interactions.
  hc_receiver<T> rec (layout, local, *this, cbc, conn);
  cell_inst_clusters_box_converter<T> cibc (cbc);

  //  handle instance to instance connections

  {
    db::box_scanner<db::Instance, unsigned int> bs;

    for (db::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
      bs.insert (inst.operator-> (), 0);
    }

    bs.process (rec, 1 /*touching*/, cibc);
  }

  //  handle local to instance connections

  {
    db::box_scanner2<db::local_cluster<T>, unsigned int, db::Instance, unsigned int> bs2;

    for (typename connected_clusters<T>::const_iterator c = local.begin (); c != local.end (); ++c) {
      bs2.insert1 (c.operator-> (), 0);
    }
    for (db::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
      bs2.insert2 (inst.operator-> (), 0);
    }

    bs2.process (rec, 1 /*touching*/, local_cluster_box_convert<T> (), cibc);
  }
}

template <class T>
const connected_clusters<T> &
hier_clusters<T>::clusters_per_cell (db::cell_index_type cell_index) const
{
  typename std::map<db::cell_index_type, connected_clusters<T> >::const_iterator c = m_per_cell_clusters.find (cell_index);
  if (c == m_per_cell_clusters.end ()) {
    static connected_clusters<T> empty;
    return empty;
  } else {
    return c->second;
  }
}

//  explicit instantiations
template class DB_PUBLIC hier_clusters<db::PolygonRef>;

}

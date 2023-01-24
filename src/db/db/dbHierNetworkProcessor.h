
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


#ifndef HDR_dbHierNetworkProcessor
#define HDR_dbHierNetworkProcessor

#include "dbCommon.h"
#include "dbTrans.h"
#include "dbBoxConvert.h"
#include "dbBoxTree.h"
#include "dbCell.h"
#include "dbInstElement.h"
#include "tlEquivalenceClusters.h"
#include "tlAssert.h"
#include "tlSList.h"

#include <map>
#include <list>
#include <vector>
#include <set>
#include <limits>

namespace tl {
  class RelativeProgress;
}

namespace db {

class DeepLayer;

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
  typedef std::set<size_t> global_nets_type;
  typedef global_nets_type::const_iterator global_nets_iterator;

  /**
   *  @brief Specifies the edge connectivity mode
   */
  enum edge_connectivity_type
  {
    /**
     *  @brief Edges connect if they are collinear
     */
    EdgesConnectCollinear,

    /**
     *  @brief Edges connect if the end point of one edge is the start point of the other edge
     */
    EdgesConnectByPoints
  };

  /**
   *  @brief Creates a connectivity object without any connections
   */
  Connectivity ();

  /**
   *  @brief Creates a connectivity object without connections and the given edge connectivity mode
   */
  Connectivity (edge_connectivity_type ec);

  /**
   *  @brief Adds intra-layer connectivity for layer l
   */
  void connect (unsigned int l);

  /**
   *  @brief Adds inter-layer connectivity
   */
  void connect (unsigned int la, unsigned int lb);

  /**
   *  @brief Adds a connection to a global net
   */
  size_t connect_global (unsigned int l, const std::string &gn);

  /**
   *  @brief Adds intra-layer connectivity for layer l
   *  This is a convenience method that takes a db::DeepLayer object.
   *  It is assumed that all those layers originate from the same deep shape store.
   */
  void connect (const db::DeepLayer &l);

  /**
   *  @brief Adds inter-layer connectivity
   *  This is a convenience method that takes a db::DeepLayer object.
   *  It is assumed that all those layers originate from the same deep shape store.
   */
  void connect (const db::DeepLayer &la, const db::DeepLayer &lb);

  /**
   *  @brief Adds a connection to a global net
   */
  size_t connect_global (const db::DeepLayer &la, const std::string &gn);

  /**
   *  @brief Gets the global net name per ID
   */
  const std::string &global_net_name (size_t id) const;

  /**
   *  @brief Gets the global net ID for the given name
   */
  size_t global_net_id (const std::string &gn);

  /**
   *  @brief Gets the number of global nets (it's also the max ID + 1)
   */
  size_t global_nets () const;

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
   *  @brief Begin iterator for the global connections for a specific layer
   */
  global_nets_iterator begin_global_connections (unsigned int layer) const;

  /**
   *  @brief End iterator for the layers connected to a specific layer
   */
  global_nets_iterator end_global_connections (unsigned int layer) const;

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

  /**
   *  @brief Returns true, if two sets of layers interact
   */
  bool interacts (const std::set<unsigned int> &la, const std::set<unsigned int> &lb) const;

  /**
   *  @brief Returns true, if two cells basically (without considering transformation) interact
   */
  bool interact (const db::Cell &a, const db::Cell &b) const;

  /**
   *  @brief Returns true, if two cells with the given transformations interact
   */
  template <class T>
  bool interact (const db::Cell &a, const T &ta, const db::Cell &b, const T &tb) const;

private:
  layers_type m_all_layers;
  std::map<unsigned int, layers_type> m_connected;
  std::vector<std::string> m_global_net_names;
  std::map<unsigned int, global_nets_type> m_global_connections;
  edge_connectivity_type m_ec;
};

/**
 *  @brief Represents a cluster of shapes
 *
 *  A cluster of shapes is a set of shapes of type T which are connected in terms
 *  of a given connectivity. The shapes will still be organized in layers.
 */
template <class T>
class DB_PUBLIC_TEMPLATE local_cluster
{
public:
  typedef size_t id_type;
  typedef typename T::box_type box_type;
  typedef db::unstable_box_tree<box_type, T, db::box_convert<T> > tree_type;
  typedef typename tree_type::flat_iterator shape_iterator;
  typedef size_t attr_id;
  typedef std::set<attr_id> attr_set;
  typedef attr_set::const_iterator attr_iterator;
  typedef size_t global_net_id;
  typedef std::set<global_net_id> global_nets;
  typedef global_nets::const_iterator global_nets_iterator;

  /**
   *  @brief Creates an empty cluster
   */
  local_cluster (size_t id = 0);

  /**
   *  @brief Clears the cluster
   */
  void clear ();

  /**
   *  @brief Returns true if the cluster is empty
   */
  bool empty () const;

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
   *  @brief Tests whether this cluster interacts with the given cell
   *
   *  "trans" is the transformation which is applied to the cell before
   *  the test.
   */
  bool interacts (const db::Cell &cell, const db::ICplxTrans &trans, const Connectivity &conn) const;

  /**
   *  @brief Gets the bounding box of this cluster
   */
  const box_type &bbox () const
  {
    const_cast<local_cluster<T> *> (this)->ensure_sorted ();  //  also updates bbox
    return m_bbox;
  }

  /**
   *  @brief Computes the "area ratio" of the cluster - this is a rough approximation of the area covered
   *  The algorithm used assumes no overlap between the polygons of the cluster.
   */
  double area_ratio () const;

  /**
   *  @brief Splits the cluster into multiple other clusters until the desired area ratio is achieved.
   *  The result is sent to the output iterator. The return value is the number of clusters produced.
   *  If the area ratio of the cluster is below the limit, no splitting happens and 0 is returned.
   */
  template <class Iter>
  size_t split (double max_area_ratio, Iter &output) const;

  /**
   *  @brief Gets a vector of layers inside the cluster
   */
  std::vector<unsigned int> layers () const;

  /**
   *  @brief Gets the total number of shapes in this cluster
   */
  size_t size () const
  {
    return m_size;
  }

  /**
   *  @brief Gets the shape iterator for a given layer
   */
  shape_iterator begin (unsigned int l) const;

  /**
   *  @brief Adds the given attribute to the attribute set
   *
   *  Attributes are arbitrary IDs attached to clusters.
   *  The attribute value 0 is reserved for "no attribute" and is not
   *  put into the set.
   */
  void add_attr (attr_id a);

  /**
   *  @brief Gets a value indicating whether the attributes of the clusters are identical
   */
  bool same_attrs (const local_cluster<T> &other) const
  {
    return m_attrs == other.m_attrs;
  }

  /**
   *  @brief Gets the attribute iterator (begin)
   */
  attr_iterator begin_attr () const
  {
    return m_attrs.begin ();
  }

  /**
   *  @brief Gets the attribute iterator (end)
   */
  attr_iterator end_attr () const
  {
    return m_attrs.end ();
  }

  /**
   *  @brief Gets the global net IDs (begin)
   */
  global_nets_iterator begin_global_nets () const
  {
    return m_global_nets.begin ();
  }

  /**
   *  @brief Gets the global net IDs (end)
   */
  global_nets_iterator end_global_nets () const
  {
    return m_global_nets.end ();
  }

  /**
   *  @brief Gets the global nets set
   */
  const global_nets &get_global_nets () const
  {
    return m_global_nets;
  }

  /**
   *  @brief Sets the global nets
   */
  void set_global_nets (const global_nets &gn);

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const;

private:
  template <typename> friend class local_clusters;
  template <typename> friend class hnp_interaction_receiver;

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
  attr_set m_attrs;
  global_nets m_global_nets;
  size_t m_size;
};

/**
 *  @brief Memory statistics for local_cluster<T>
 */
template <class T>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const local_cluster<T> &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief A box converter for the local_cluster class
 */
template <class T>
struct DB_PUBLIC_TEMPLATE local_cluster_box_convert
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
class DB_PUBLIC_TEMPLATE local_clusters
{
public:
  typedef typename local_cluster<T>::id_type id_type;
  typedef typename local_cluster<T>::box_type box_type;
  typedef typename local_cluster<T>::attr_id attr_id;
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
   *  @brief Joins a cluster with another one
   *
   *  This will also remove the other cluster.
   */
  void join_cluster_with (typename local_cluster<T>::id_type id, typename local_cluster<T>::id_type with_id);

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
   *  @brief Gets a value indicating whether the cluster set is empty
   */
  bool empty () const
  {
    return m_clusters.empty ();
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
   *
   *  If attr_equivalence is non-null, all clusters with attributes
   *  listed as equivalent in this object are joined. Additional
   *  cluster joining may happen in this case, because multi-attribute
   *  assignment might create connections too.
   */
  void build_clusters (const db::Cell &cell, const db::Connectivity &conn, const tl::equivalence_clusters<size_t> *attr_equivalence = 0, bool report_progress = false, bool separate_attributes = false);

  /**
   *  @brief Creates and inserts a new clusters
   *
   *  NOTE: the object should not be modified after sorting has taken place.
   */
  local_cluster<T> *insert ();

  /**
   *  @brief Allocates a new ID for dummy clusters
   *
   *  Dummy cluster ID's will deliver empty clusters. Used for connectors.
   */
  typename local_cluster<T>::id_type insert_dummy ()
  {
    return --m_next_dummy_id;
  }

  /**
   *  @brief Gets a value indicating whether the given ID is a dummy ID
   */
  bool is_dummy (typename local_cluster<T>::id_type id) const
  {
    return id > m_clusters.size ();
  }

  /**
   *  @brief Gets the number of clusters
   */
  size_t size () const
  {
    return m_clusters.size ();
  }

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const;

private:
  void ensure_sorted ();

  bool m_needs_update;
  box_type m_bbox;
  tree_type m_clusters;
  size_t m_next_dummy_id;

  void apply_attr_equivalences (const tl::equivalence_clusters<size_t> &attr_equivalence);
};

/**
 *  @brief Memory statistics for local_clusters<T>
 */
template <class T>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const local_clusters<T> &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief The instance information for a cluster
 */
class DB_PUBLIC ClusterInstElement
{
public:
  ClusterInstElement (const db::InstElement &ie)
  {
    if (ie.array_inst.at_end ()) {

      m_inst_cell_index = std::numeric_limits<db::cell_index_type>::max ();
      m_inst_trans = db::ICplxTrans ();
      m_inst_prop_id = 0;

    } else {

      m_inst_cell_index = ie.inst_ptr.cell_index ();
      m_inst_trans = ie.complex_trans ();
      m_inst_prop_id = ie.inst_ptr.prop_id ();

    }
  }

  ClusterInstElement (db::cell_index_type inst_cell_index, const db::ICplxTrans &inst_trans, db::properties_id_type inst_prop_id)
    : m_inst_cell_index (inst_cell_index), m_inst_trans (inst_trans), m_inst_prop_id (inst_prop_id)
  {
    //  .. nothing yet ..
  }

  ClusterInstElement ()
    : m_inst_cell_index (std::numeric_limits<db::cell_index_type>::max ()), m_inst_trans (), m_inst_prop_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true, if the cluster does not have an instance
   */
  bool has_instance () const
  {
    return m_inst_cell_index != std::numeric_limits<db::cell_index_type>::max ();
  }

  /**
   *  @brief Gets the cell index of the cell which is instantiated
   */
  db::cell_index_type inst_cell_index () const
  {
    return m_inst_cell_index;
  }

  /**
   *  @brief Gets the instance transformation
   */
  const db::ICplxTrans &inst_trans () const
  {
    return m_inst_trans;
  }

  /**
   *  @brief Gets the instance properties id
   */
  db::properties_id_type inst_prop_id () const
  {
    return m_inst_prop_id;
  }

  /**
   *  @brief Sets the instance properties id
   */
  void set_inst_prop_id (db::properties_id_type pid)
  {
    m_inst_prop_id = pid;
  }

  /**
   *  @brief Transform with the given transformation
   */
  void transform (const db::ICplxTrans &tr)
  {
    m_inst_trans = tr * m_inst_trans;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const ClusterInstElement &other) const
  {
    return m_inst_cell_index == other.m_inst_cell_index && m_inst_trans.equal (other.m_inst_trans) && m_inst_prop_id == other.m_inst_prop_id;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const ClusterInstElement &other) const
  {
    return ! operator== (other);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const ClusterInstElement &other) const
  {
    if (m_inst_cell_index != other.m_inst_cell_index) {
      return m_inst_cell_index < other.m_inst_cell_index;
    }
    if (! m_inst_trans.equal (other.m_inst_trans)) {
      return m_inst_trans.less (other.m_inst_trans);
    }
    return m_inst_prop_id < other.m_inst_prop_id;
  }

private:
  db::cell_index_type m_inst_cell_index;
  db::ICplxTrans m_inst_trans;
  db::properties_id_type m_inst_prop_id;
};

/**
 *  @brief A connection to a cluster in a child instance
 */
class DB_PUBLIC ClusterInstance
  : public ClusterInstElement
{
public:
  ClusterInstance (size_t id, db::cell_index_type inst_cell_index, const db::ICplxTrans &inst_trans, db::properties_id_type inst_prop_id)
    : ClusterInstElement (inst_cell_index, inst_trans, inst_prop_id), m_id (id)
  {
    //  .. nothing yet ..
  }

  ClusterInstance (size_t id, const db::InstElement &inst_element)
    : ClusterInstElement (inst_element), m_id (id)
  {
    //  .. nothing yet ..
  }

  ClusterInstance (size_t id)
    : ClusterInstElement (), m_id (id)
  {
    //  .. nothing yet ..
  }

  ClusterInstance ()
    : ClusterInstElement (), m_id (0)
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
   *  @brief Equality
   */
  bool operator== (const ClusterInstance &other) const
  {
    return m_id == other.m_id && ClusterInstElement::operator== (other);
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const ClusterInstance &other) const
  {
    return ! operator== (other);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const ClusterInstance &other) const
  {
    if (m_id != other.m_id) {
      return m_id < other.m_id;
    }
    return ClusterInstElement::operator< (other);
  }

private:
  size_t m_id;
};

typedef std::list<std::pair<ClusterInstance, ClusterInstance> > cluster_instance_pair_list_type;

inline bool equal_array_delegates (const db::ArrayBase *a, const db::ArrayBase *b)
{
  if ((a == 0) != (b == 0)) {
    return false;
  } else if (a) {
    return ! db::array_base_ptr_cmp_f () (a, b) && ! db::array_base_ptr_cmp_f () (b, a);
  } else {
    return true;
  }
}

inline bool less_array_delegates (const db::ArrayBase *a, const db::ArrayBase *b)
{
  if ((a == 0) != (b == 0)) {
    return (a == 0) < (b == 0);
  } else if (a) {
    return db::array_base_ptr_cmp_f () (a, b);
  } else {
    return false;
  }
}

/**
 *  @brief A helper struct to describe a pair of cell instances with a specific relative transformation
 */
struct DB_PUBLIC InstanceToInstanceInteraction
{
  InstanceToInstanceInteraction (const db::ArrayBase *_array1, const db::ArrayBase *_array2, const db::ICplxTrans &_tn, const db::ICplxTrans &_t21)
    : array1 (0), array2 (0), t21 (_t21)
  {
    if (_array1) {
      array1 = _array1->basic_clone ();
      static_cast<db::basic_array<db::Coord> *> (array1)->transform (_tn);
    }

    if (_array2) {
      array2 = _array2->basic_clone ();
      static_cast<db::basic_array<db::Coord> *> (array2)->transform (_tn);
    }
  }

  InstanceToInstanceInteraction ()
    : array1 (0), array2 (0)
  {
    //  .. nothing yet ..
  }

  InstanceToInstanceInteraction (const InstanceToInstanceInteraction &other)
    : array1 (other.array1 ? other.array1->basic_clone () : 0),
      array2 (other.array2 ? other.array2->basic_clone () : 0),
      t21 (other.t21)
  {
    //  .. nothing yet ..
  }

  InstanceToInstanceInteraction &operator= (const InstanceToInstanceInteraction &other)
  {
    if (this != &other) {

      if (array1) {
        delete array1;
      }
      array1 = other.array1 ? other.array1->basic_clone () : 0;

      if (array2) {
        delete array2;
      }
      array2 = other.array2 ? other.array2->basic_clone () : 0;

      t21 = other.t21;

    }

    return *this;
  }

  ~InstanceToInstanceInteraction ()
  {
    if (array1) {
      delete array1;
    }
    array1 = 0;

    if (array2) {
      delete array2;
    }
    array2 = 0;
  }

  bool operator== (const InstanceToInstanceInteraction &other) const
  {
    return t21.equal (other.t21) &&
            equal_array_delegates (array1, other.array1) &&
            equal_array_delegates (array2, other.array2);
  }

  bool operator< (const InstanceToInstanceInteraction &other) const
  {
    if (! t21.equal (other.t21)) {
      return t21.less (other.t21);
    }
    if (less_array_delegates (array1, other.array1)) {
      return true;
    } else if (less_array_delegates (other.array1, array1)) {
      return false;
    }
    return less_array_delegates (array2, other.array2);
  }

  db::ArrayBase *array1, *array2;
  db::ICplxTrans t21;
};

template <class Box>
struct DB_PUBLIC_TEMPLATE interaction_key_for_clusters
  : public InstanceToInstanceInteraction
{
  interaction_key_for_clusters (const db::ICplxTrans &_t1, const db::ICplxTrans &_t21, const Box &_box)
    : InstanceToInstanceInteraction (0, 0, _t1, _t21), box (_box)
  { }

  bool operator== (const interaction_key_for_clusters &other) const
  {
    return InstanceToInstanceInteraction::operator== (other) && box == other.box;
  }

  bool operator< (const interaction_key_for_clusters &other) const
  {
    if (! InstanceToInstanceInteraction::operator== (other)) {
      return InstanceToInstanceInteraction::operator< (other);
    }
    if (box != other.box) {
      return box < other.box;
    }
    return false;
  }

  Box box;
};

/**
 *  @brief An object representing the instance interaction cache
 */
template <class Key, class Value>
class DB_PUBLIC_TEMPLATE instance_interaction_cache
{
public:
  instance_interaction_cache ()
    : m_hits (0), m_misses (0)
  { }

  size_t size () const
  {
    MemStatisticsSimple ms;
    ms << m_map;
    return ms.used ();
  }

  size_t hits () const
  {
    return m_hits;
  }

  size_t misses () const
  {
    return m_misses;
  }

  const Value *find (db::cell_index_type ci1, db::cell_index_type ci2, const Key &key) const
  {
    typename std::map <std::pair<db::cell_index_type, db::cell_index_type>, std::list <std::pair<Key, Value> > >::iterator i1 = m_map.find (std::make_pair (ci1, ci2));
    if (i1 == m_map.end ()) {
      ++m_misses;
      return 0;
    }

    //  NOTE: the number of entries is low, so we can afford a linear search
    typename std::list <std::pair<Key, Value> >::iterator i = i1->second.begin ();
    while (i != i1->second.end () && ! (i->first == key)) {
      ++i;
    }

    if (i == i1->second.end ()) {
      ++m_misses;
      return 0;
    } else {
      //  move the element to the front so the most frequently used ones are at the front
      if (i != i1->second.begin ()) {
        i1->second.splice (i1->second.begin(), i1->second, i, std::next (i));
      }
      ++m_hits;
      return &i->second;
    }
  }

  Value &insert (db::cell_index_type ci1, db::cell_index_type ci2, const Key &key)
  {
    const size_t instance_cache_variant_threshold = 20;

    std::list <std::pair<Key, Value> > &m = m_map [std::make_pair (ci1, ci2)];
    if (m.size () >= instance_cache_variant_threshold) {
      m.pop_back ();
    }

    m.push_front (std::make_pair (key, Value ()));
    return m.front ().second;
  }

private:
  mutable size_t m_hits, m_misses;
  mutable std::map <std::pair<db::cell_index_type, db::cell_index_type>, std::list <std::pair<Key, Value> > > m_map;
};

template <class T> class hier_clusters;
template <class T> class connected_clusters;

/**
 *  @brief An iterator delivering all clusters of a connected_clusters set
 */
template <class T>
class DB_PUBLIC_TEMPLATE connected_clusters_iterator
{
public:
  typedef typename local_cluster<T>::id_type value_type;

  connected_clusters_iterator (const connected_clusters<T> &c);

  connected_clusters_iterator &operator++ ()
  {
    if (! m_lc_iter.at_end ()) {
      ++m_lc_iter;
    } else if (m_x_iter != m_x_iter_end) {
      ++m_x_iter;
    }
    return *this;
  }

  bool at_end () const
  {
    return m_lc_iter.at_end () && m_x_iter == m_x_iter_end;
  }

  value_type operator* () const
  {
    if (m_lc_iter.at_end ()) {
      return m_x_iter->first;
    } else {
      return m_lc_iter->id ();
    }
  }

private:
  typename local_clusters<T>::const_iterator m_lc_iter;
  typedef tl::slist<ClusterInstance> connections_type;
  typename std::map<typename local_cluster<T>::id_type, connections_type>::const_iterator m_x_iter, m_x_iter_end;
};

/**
 *  @brief Local clusters with connections to clusters from child cells
 *
 *  Clusters can get connected. There are incoming connections (from above the hierarchy)
 *  and outgoing connections (down to a child cell).
 *
 *  "root" clusters are some that don't have incoming connections. There are only
 *  root clusters or clusters which are connected from every parent cell. There are no
 *  "half connected" clusters.
 */
template <class T>
class DB_PUBLIC_TEMPLATE connected_clusters
  : public local_clusters<T>
{
public:
  typedef typename local_clusters<T>::id_type id_type;
  typedef tl::slist<ClusterInstance> connections_type;
  typedef typename local_clusters<T>::box_type box_type;
  typedef connected_clusters_iterator<T> all_iterator;
  typedef typename std::map<typename local_cluster<T>::id_type, connections_type>::const_iterator connections_iterator;

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
  const connections_type &connections_for_cluster (typename local_cluster<T>::id_type id) const;

  /**
   *  @brief Reverse "connections_for_cluster"
   *  Finds the cluster which has a connection given by inst.
   *  Returns 0 if the given connection does not exist.
   */
  typename local_cluster<T>::id_type find_cluster_with_connection (const ClusterInstance &inst) const;

  /**
   *  @brief Adds a connection between a local cluster and one from a child instance
   */
  void add_connection (typename local_cluster<T>::id_type, const ClusterInstance &inst);

  /**
   *  @brief Joins the cluster id with the cluster with_id
   *
   *  The "with_id" cluster is removed. All connections of "with_id" are transferred to the
   *  first one. All shapes of "with_id" are transferred to "id".
   */
  void join_cluster_with(typename local_cluster<T>::id_type id, typename local_cluster<T>::id_type with_id);

  /**
   *  @brief An iterator delivering all clusters (even the connectors)
   *
   *  This iterator will deliver ID's rather than cluster objects.
   */
  all_iterator begin_all () const
  {
    return connected_clusters_iterator<T> (*this);
  }

  /**
   *  @brief Begin iterator for the connections
   *
   *  The iterated object is a pair or (cluster id, connections_type).
   */
  connections_iterator begin_connections () const
  {
    return m_connections.begin ();
  }

  /**
   *  @brief Begin iterator for the connections
   */
  connections_iterator end_connections () const
  {
    return m_connections.end ();
  }

  /**
   *  @brief Gets a value indicating whether the cluster set is empty
   */
  bool empty () const
  {
    return local_clusters<T>::empty () && m_connections.empty ();
  }

  /**
   *  @brief Returns true, if the given cluster ID is a root cluster
   */
  bool is_root (id_type id) const
  {
    return m_connected_clusters.find (id) == m_connected_clusters.end ();
  }

  /**
   *  @brief Resets the root status of a cluster
   *  CAUTION: don't call this method unless you know what you're doing.
   */
  void reset_root (id_type id)
  {
    m_connected_clusters.insert (id);
  }

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const;

private:
  template<typename> friend class connected_clusters_iterator;

  std::map<id_type, connections_type> m_connections;
  std::map<ClusterInstance, typename local_cluster<T>::id_type> m_rev_connections;
  std::set<id_type> m_connected_clusters;
};

/**
 *  @brief Memory statistics for connected_clusters<T>
 */
template <class T>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const connected_clusters<T> &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

template <typename> class cell_clusters_box_converter;

/**
 *  @brief A hierarchical representation of clusters
 *
 *  Hierarchical clusters
 */
template <class T>
class DB_PUBLIC_TEMPLATE hier_clusters
  : public tl::Object
{
public:
  typedef typename local_cluster<T>::box_type box_type;

  typedef instance_interaction_cache<InstanceToInstanceInteraction, cluster_instance_pair_list_type> instance_interaction_cache_type;

  /**
   *  @brief Creates an empty set of clusters
   */
  hier_clusters ();

  /**
   *  @brief Sets the base verbosity
   *
   *  The default value is 30. Basic timing will be reported for > base_verbosity, detailed timing
   *  for > base_verbosity + 10.
   */
  void set_base_verbosity (int bv);

  /**
   *  @brief A constant indicating the top cell for the equivalence cluster key
   */
  static const db::cell_index_type top_cell_index;

  /**
   *  @brief Builds a hierarchy of clusters from a cell hierarchy and given connectivity
   */
  void build (const db::Layout &layout, const db::Cell &cell, const db::Connectivity &conn, const std::map<db::cell_index_type, tl::equivalence_clusters<size_t> > *attr_equivalence = 0, const std::set<cell_index_type> *breakout_cells = 0, bool separate_attributes = false);

  /**
   *  @brief Gets the connected clusters for a given cell
   */
  const connected_clusters<T> &clusters_per_cell (db::cell_index_type cell_index) const;

  /**
   *  @brief Gets the connected clusters for a given cell (non-const version)
   */
  connected_clusters<T> &clusters_per_cell (db::cell_index_type cell_index);

  /**
   *  @brief Writes the net shapes back to the original hierarchy
   *
   *  The layout object is supposed to be the original layout or one with identical cell indexes.
   *  "lm" is a layer mapping table from the connection layer indexes to the target layer
   *  indexes.
   *
   *  The backannotation process usually involves propagation of shapes up in the hierarchy
   *  to resolve variants.
   */
  void return_to_hierarchy (db::Layout &layout, const std::map<unsigned int, unsigned int> &lm) const;

  /**
   *  @brief Clears this collection
   */
  void clear ();

  /**
   *  @brief Ensures a cluster instance is connected from all parents of the instantiated cell
   *
   *  If "with_self" is true, the specified instance "ci" is included in the connections. Otherwise
   *  there is not connection made for this instance.
   */
  size_t propagate_cluster_inst (const db::Layout &layout, const Cell &cell, const ClusterInstance &ci, db::cell_index_type parent_ci, bool with_self);

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const;

private:
  void build_local_cluster (const db::Layout &layout, const db::Cell &cell, const db::Connectivity &conn, const tl::equivalence_clusters<size_t> *attr_equivalence, bool separate_attributes);
  void build_hier_connections (cell_clusters_box_converter<T> &cbc, const db::Layout &layout, const db::Cell &cell, const db::Connectivity &conn, const std::set<cell_index_type> *breakout_cells, instance_interaction_cache_type &instance_interaction_cache, bool separate_attributes);
  void build_hier_connections_for_cells (cell_clusters_box_converter<T> &cbc, const db::Layout &layout, const std::vector<db::cell_index_type> &cells, const db::Connectivity &conn, const std::set<cell_index_type> *breakout_cells, tl::RelativeProgress &progress, instance_interaction_cache_type &instance_interaction_cache, bool separate_attributes);
  void do_build (cell_clusters_box_converter<T> &cbc, const db::Layout &layout, const db::Cell &cell, const db::Connectivity &conn, const std::map<cell_index_type, tl::equivalence_clusters<size_t> > *attr_equivalence, const std::set<cell_index_type> *breakout_cells, bool separate_attributes);

  std::map<db::cell_index_type, connected_clusters<T> > m_per_cell_clusters;
  int m_base_verbosity;
};

/**
 *  @brief A callback function for the recursive cluster shape and cluster iterator selecting cells/circuits
 *
 *  Reimplement the "new_circuit" method to receive a callback on a new cell or circuit.
 */
class DB_PUBLIC CircuitCallback
{
public:
  CircuitCallback () { }

  /**
   *  @brief This method is called whenever a circuit is entered when descending.
   *  Return false to skip this circuit and all of it's children. This method is called before the
   *  new cell is entered.
   *  @param new_ci The cell index of the cell to enter
   */
  virtual bool new_cell (db::cell_index_type /*new_ci*/) const { return true; }
};

/**
 *  @brief A recursive shape iterator for the shapes of a cluster
 *
 *  This iterator will deliver the shapes of a cluster including the shapes for the
 *  connected child clusters.
 *
 *  This iterator applies to one layer.
 */
template <class T>
class DB_PUBLIC_TEMPLATE recursive_cluster_shape_iterator
{
public:
  typedef T value_type;
  typedef const T &reference;
  typedef const T *pointer;

  /**
   *  @brief Constructor
   */
  recursive_cluster_shape_iterator (const hier_clusters<T> &hc, unsigned int layer, db::cell_index_type ci, typename local_cluster<T>::id_type id, const CircuitCallback *callback = 0);

  /**
   *  @brief Returns a value indicating whether there are any more shapes
   */
  bool at_end () const
  {
    return m_shape_iter.at_end ();
  }

  /**
   *  @brief Returns the shape (untransformed)
   */
  reference operator* () const
  {
    return *m_shape_iter;
  }

  /**
   *  @brief Returns the shape pointer (untransformed)
   */
  pointer operator-> () const
  {
    return m_shape_iter.operator-> ();
  }

  /**
   *  @brief Returns the instantiation path of the current cluster
   *
   *  The call path's root is the initial cell
   */
  std::vector<ClusterInstance> inst_path () const;

  /**
   *  @brief Returns the transformation applicable for transforming the shape to the root cluster
   */
  const db::ICplxTrans &trans () const
  {
    return m_trans_stack.back ();
  }

  /**
   *  @brief Returns the cell index the shape lives in
   */
  db::cell_index_type cell_index () const
  {
    return m_cell_index_stack.back ();
  }

  /**
   *  @brief Returns the id of the current cluster
   */
  typename db::local_cluster<T>::id_type cluster_id () const
  {
    if (m_conn_iter_stack.size () <= 1) {
      return m_id;
    } else {
      return m_conn_iter_stack [m_conn_iter_stack.size () - 2].first->id ();
    }
  }

  /**
   *  @brief Increment operator
   */
  recursive_cluster_shape_iterator &operator++ ();

  /**
   *  @brief Skips the current cell and advances to the next cell and shape
   */
  void skip_cell ();

private:
  typedef typename db::connected_clusters<T>::connections_type connections_type;

  const hier_clusters<T> *mp_hc;
  std::vector<db::ICplxTrans> m_trans_stack;
  std::vector<db::cell_index_type> m_cell_index_stack;
  std::vector<std::pair<typename connections_type::const_iterator, typename connections_type::const_iterator> > m_conn_iter_stack;
  typename db::local_cluster<T>::shape_iterator m_shape_iter;
  unsigned int m_layer;
  typename db::local_cluster<T>::id_type m_id;
  const CircuitCallback *mp_callback;

  void next_conn ();
  void up ();
  void down (db::cell_index_type ci, typename db::local_cluster<T>::id_type id, const db::ICplxTrans &t);
};

/**
 *  @brief A recursive cluster iterator for the clusters itself
 *
 *  This iterator will deliver the child clusters of a specific cluster.
 */
template <class T>
class DB_PUBLIC_TEMPLATE recursive_cluster_iterator
{
public:
  /**
   *  @brief Constructor
   */
  recursive_cluster_iterator (const hier_clusters<T> &hc, db::cell_index_type ci, typename local_cluster<T>::id_type id);

  /**
   *  @brief Returns a value indicating whether there are any more shapes
   */
  bool at_end () const
  {
    return m_cell_index_stack.empty ();
  }

  /**
   *  @brief Returns the cell index the shape lives in
   */
  db::cell_index_type cell_index () const
  {
    return m_cell_index_stack.back ();
  }

  /**
   *  @brief Returns the id of the current cluster
   */
  typename db::local_cluster<T>::id_type cluster_id () const
  {
    if (m_conn_iter_stack.size () <= 1) {
      return m_id;
    } else {
      return m_conn_iter_stack [m_conn_iter_stack.size () - 2].first->id ();
    }
  }

  /**
   *  @brief Returns the instantiation path of the current cluster
   *
   *  The call path's root is the initial cell
   */
  std::vector<ClusterInstance> inst_path () const;

  /**
   *  @brief Increment operator
   */
  recursive_cluster_iterator &operator++ ();

private:
  typedef typename db::connected_clusters<T>::connections_type connections_type;

  const hier_clusters<T> *mp_hc;
  std::vector<db::cell_index_type> m_cell_index_stack;
  std::vector<std::pair<typename connections_type::const_iterator, typename connections_type::const_iterator> > m_conn_iter_stack;
  typename db::local_cluster<T>::id_type m_id;

  void next_conn ();
  void up ();
  void down (db::cell_index_type ci, typename db::local_cluster<T>::id_type id);
};

/**
 *  @brief A connection to a cluster from a parent cluster
 */
class DB_PUBLIC IncomingClusterInstance
{
public:
  IncomingClusterInstance (db::cell_index_type pc, size_t parent_cluster_id, const ClusterInstance &inst)
    : m_parent_cell (pc), m_parent_cluster_id (parent_cluster_id), m_inst (inst)
  {
    //  .. nothing yet ..
  }

  IncomingClusterInstance ()
    : m_parent_cell (0), m_parent_cluster_id (0), m_inst ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the cell index of the parent cell
   */
  size_t parent_cell () const
  {
    return m_parent_cell;
  }

  /**
   *  @brief Gets the cluster ID from which the cluster is connected to
   *  The parent cluster lives in the parent cell
   */
  size_t parent_cluster_id () const
  {
    return m_parent_cluster_id;
  }

  /**
   *  @brief Gets the instance path
   */
  const ClusterInstance &inst () const
  {
    return m_inst;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const IncomingClusterInstance &other) const
  {
    return m_parent_cluster_id == other.m_parent_cluster_id && m_parent_cell == other.m_parent_cell && m_inst == other.m_inst;
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const IncomingClusterInstance &other) const
  {
    if (m_parent_cluster_id != other.m_parent_cluster_id) {
      return m_parent_cluster_id < other.m_parent_cluster_id;
    }
    if (m_parent_cell != other.m_parent_cell) {
      return m_parent_cell < other.m_parent_cell;
    }
    return m_inst < other.m_inst;
  }

private:
  db::cell_index_type m_parent_cell;
  size_t m_parent_cluster_id;
  ClusterInstance m_inst;
};

/**
 *  @brief A class holding the parent relationships for clusters of cells
 *
 *  This class can be used to quickly identify the connections made to a specific cluster from a parent cluster.
 */
template <class T>
class incoming_cluster_connections
{
public:
  typedef std::list<IncomingClusterInstance> incoming_connections;

  incoming_cluster_connections (const db::Layout &layout, const db::Cell &cell, const hier_clusters<T> &hc);

  bool has_incoming (db::cell_index_type ci, size_t cluster_id) const;
  const incoming_connections &incoming (db::cell_index_type ci, size_t cluster_id) const;

private:
  mutable std::set<db::cell_index_type> m_called_cells;
  mutable std::map<db::cell_index_type, std::map<size_t, incoming_connections> > m_incoming;
  tl::weak_ptr<db::Layout> mp_layout;
  tl::weak_ptr<hier_clusters<T> > mp_hc;

  void ensure_computed (db::cell_index_type ci) const;
  void ensure_computed_parent (db::cell_index_type ci) const;
};

/**
 *  @brief A helper function generating an attribute ID from a property ID
 *  This function is used to provide a generic attribute wrapping a property ID, text ID and global net ID
 */
inline size_t prop_id_to_attr (db::properties_id_type id)
{
  return size_t (id) * 4;
}

/**
 *  @brief Gets a value indicating whether the attribute is a property ID
 */
inline bool is_prop_id_attr (size_t attr)
{
  return (attr & 3) == 0;
}

/**
 *  @brief Gets the property ID from an attribute
 */
inline db::properties_id_type prop_id_from_attr (size_t attr)
{
  return attr / 4;
}

/**
 *  @brief A helper function generating an attribute ID from a global net ID
 *  This function is used to provide a generic attribute wrapping a property ID, text ID and global net ID
 */
inline size_t global_net_id_to_attr (size_t id)
{
  return size_t (id) * 4 + 2;
}

/**
 *  @brief Gets a value indicating whether the attribute is a global net ID
 */
inline bool is_global_net_id_attr (size_t attr)
{
  return (attr & 3) == 2;
}

/**
 *  @brief Gets the global net ID from an attribute
 */
inline size_t global_net_id_from_attr (size_t attr)
{
  return attr / 4;
}

/**
 *  @brief A helper function generating an attribute from a StringRef
 */
inline size_t text_ref_to_attr (const db::Text *tr)
{
  //  NOTE: pointers are 32bit aligned, hence the lower two bits are 0
  return size_t (tr) + 1;
}

/**
 *  @brief Gets a value indicating whether the attribute is a StringRef
 */
inline bool is_text_ref_attr (size_t attr)
{
  return (attr & 3) == 1;
}

/**
 *  @brief Gets the text value from an attribute ID
 */
inline const char *text_from_attr (size_t attr)
{
  tl_assert ((attr & 1) != 0);
  const db::Text *t = reinterpret_cast<const db::Text *> (attr - 1);
  return t->string ();
}

}

#endif

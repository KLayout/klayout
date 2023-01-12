
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

#ifndef _HDR_dbNetlistCompareGraph
#define _HDR_dbNetlistCompareGraph

#include "dbCommon.h"
#include "dbNetlistCompareUtils.h"

#include <string>
#include <limits>
#include <vector>
#include <algorithm>
#include <map>

namespace db
{

// --------------------------------------------------------------------------------------------------------------------
//  A generic triplet of object category and two IDs
//  Used as a key for device terminal edges and subcircuit edges

class DB_PUBLIC CatAndIds
{
public:
  CatAndIds (size_t cat, size_t id1, size_t id2)
    : m_cat (cat), m_id1 (id1), m_id2 (id2)
  { }

  bool operator== (const CatAndIds &other) const
  {
    return m_cat == other.m_cat && m_id1 == other.m_id1 && m_id2 == other.m_id2;
  }

  bool operator< (const CatAndIds &other) const
  {
    if (m_cat != other.m_cat) {
      return m_cat < other.m_cat;
    }
    if (m_id1 != other.m_id1) {
      return m_id1 < other.m_id1;
    }
    if (m_id2 != other.m_id2) {
      return m_id2 < other.m_id2;
    }
    return false;
  }

private:
  size_t m_cat, m_id1, m_id2;
};

// --------------------------------------------------------------------------------------------------------------------
//  NetGraphNode definition and implementation

/**
 *  @brief Represents one transition within a net graph edge
 *
 *  Each transition connects two pins of subcircuits or terminals of devices.
 *  An edge is basically a collection of transitions.
 */
class DB_PUBLIC Transition
{
public:
  Transition (const db::Device *device, size_t device_category, size_t terminal1_id, size_t terminal2_id);
  Transition (const db::SubCircuit *subcircuit, size_t subcircuit_category, size_t pin1_id, size_t pin2_id);

  static size_t first_unique_pin_id ();
  CatAndIds make_key () const;

  bool operator< (const Transition &other) const;
  bool operator== (const Transition &other) const;

  std::string to_string () const;

  inline bool is_for_subcircuit () const
  {
    return m_id1 > std::numeric_limits<size_t>::max () / 2;
  }

  const db::Device *device () const
  {
    return (const db::Device *) m_ptr;
  }

  const db::SubCircuit *subcircuit () const
  {
    return (const db::SubCircuit *) m_ptr;
  }

  size_t cat () const
  {
    return m_cat;
  }

  size_t id1 () const
  {
    return m_id1;
  }

  size_t id2 () const
  {
    return m_id2;
  }

private:
  void *m_ptr;
  size_t m_cat;
  size_t m_id1, m_id2;
};

/**
 *  @brief A node within the net graph
 *
 *  This class represents a node and the edges leading from this node to
 *  other nodes.
 *
 *  A graph edge is a collection of transitions, connecting terminals of
 *  devices or pins of subcircuits plus the index of node at the other end
 *  of the edge.
 *
 *  Transitions are sorted within the edge.
 */
class DB_PUBLIC NetGraphNode
{
public:
  typedef std::pair<std::vector<Transition>, std::pair<size_t, const db::Net *> > edge_type;

  static void swap_edges (edge_type &e1, edge_type &e2)
  {
    e1.first.swap (e2.first);
    std::swap (e1.second, e2.second);
  }

  struct EdgeToEdgeOnlyCompare
  {
    bool operator() (const edge_type &a, const std::vector<Transition> &b) const
    {
      return a.first < b;
    }
  };

  typedef std::vector<edge_type>::const_iterator edge_iterator;

  NetGraphNode ()
    : mp_net (0), m_other_net_index (invalid_id)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Builds a node for a net
   */
  NetGraphNode (const db::Net *net, DeviceCategorizer &device_categorizer, CircuitCategorizer &circuit_categorizer, const DeviceFilter &device_filter, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinCategorizer *pin_map, size_t *unique_pin_id);

  /**
   *  @brief Builds a virtual node for a subcircuit
   */
  NetGraphNode (const db::SubCircuit *sc, CircuitCategorizer &circuit_categorizer, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinCategorizer *pin_map, size_t *unique_pin_id);

  void expand_subcircuit_nodes (NetGraph *graph);

  std::string to_string () const;

  const db::Net *net () const
  {
    return mp_net;
  }

  bool has_other () const
  {
    return m_other_net_index != invalid_id && m_other_net_index != unknown_id;
  }

  bool has_any_other () const
  {
    return m_other_net_index != invalid_id;
  }

  bool has_unknown_other () const
  {
    return m_other_net_index == unknown_id;
  }

  size_t other_net_index () const
  {
    return (m_other_net_index == invalid_id || m_other_net_index == unknown_id) ? m_other_net_index : m_other_net_index / 2;
  }

  bool exact_match () const
  {
    return (m_other_net_index == invalid_id || m_other_net_index == unknown_id) ? false : (m_other_net_index & 1) != 0;
  }

  void set_other_net (size_t index, bool exact_match)
  {
    if (index == invalid_id || index == unknown_id) {
      m_other_net_index = index;
    } else {
      m_other_net_index = (index * 2) + size_t (exact_match ? 1 : 0);
    }
  }

  void unset_other_net ()
  {
    m_other_net_index = invalid_id;
  }

  bool empty () const
  {
    return m_edges.empty ();
  }

  void apply_net_index (const std::map<const db::Net *, size_t> &ni);

  bool less (const NetGraphNode &node, bool with_name) const;
  bool equal (const NetGraphNode &node, bool with_name) const;

  bool operator== (const NetGraphNode &node) const
  {
    return equal (node, false);
  }

  bool operator< (const NetGraphNode &node) const
  {
    return less (node, false);
  }

  void swap (NetGraphNode &other)
  {
    std::swap (m_other_net_index, other.m_other_net_index);
    std::swap (mp_net, other.mp_net);
    m_edges.swap (other.m_edges);
  }

  edge_iterator begin () const
  {
    return m_edges.begin ();
  }

  edge_iterator end () const
  {
    return m_edges.end ();
  }

  edge_iterator find_edge (const std::vector<Transition> &edge) const
  {
    edge_iterator res = std::lower_bound (begin (), end (), edge, EdgeToEdgeOnlyCompare ());
    if (res == end () || res->first != edge) {
      return end ();
    } else {
      return res;
    }
  }

  std::vector<edge_type> &edges ()
  {
    return m_edges;
  }

private:
  const db::Net *mp_net;
  size_t m_other_net_index;
  std::vector<edge_type> m_edges;

  /**
   *  @brief Compares edges as "less"
   *  Edge comparison is based on the pins attached (name of the first pin).
   */
  static bool net_less (const db::Net *a, const db::Net *b, bool with_name);

  /**
   *  @brief Compares edges as "equal"
   *  See edge_less for the comparison details.
   */
  static bool net_equal (const db::Net *a, const db::Net *b, bool with_name);
};

/**
 *  @brief A combination of a node and an edge reference
 */
struct NodeEdgePair
{
public:
  NodeEdgePair (const NetGraphNode *_node, NetGraphNode::edge_iterator _edge)
    : node (_node), edge (_edge)
  { }

public:
  const NetGraphNode *node;
  NetGraphNode::edge_iterator edge;
};

/**
 *  @brief A comparator comparing the first node pointer from a node/edge pair
 */
struct CompareNodeEdgePair
{
  bool operator() (const NodeEdgePair &a, const NodeEdgePair &b) const
  {
    return a.node->less (*b.node, true);
  }
};

/**
 *  @brief A comparator comparing two node pointers
 */
struct CompareNodePtr
{
  bool operator() (const NetGraphNode *a, const NetGraphNode *b) const
  {
    return a->less (*b, true);
  }
};

}

namespace std
{
  inline void swap (db::NetGraphNode &a, db::NetGraphNode &b)
  {
    a.swap (b);
  }
}

namespace db
{

// --------------------------------------------------------------------------------------------------------------------
//  NetGraph definition and implementation

/**
 *  @brief The net graph for the compare algorithm
 */
class DB_PUBLIC NetGraph
{
public:
  typedef std::vector<NetGraphNode>::const_iterator node_iterator;

  NetGraph ();

  /**
   *  @brief Builds the net graph
   */
  void build (const db::Circuit *c, DeviceCategorizer &device_categorizer, CircuitCategorizer &circuit_categorizer, const db::DeviceFilter &device_filter, const std::map<const db::Circuit *, CircuitMapper> *circuit_and_pin_mapping, const CircuitPinCategorizer *circuit_pin_mapper, size_t *unique_pin_id);

  /**
   *  @brief Gets the node index for the given net
   */
  size_t node_index_for_net (const db::Net *net) const
  {
    std::map<const db::Net *, size_t>::const_iterator j = m_net_index.find (net);
    tl_assert (j != m_net_index.end ());
    return j->second;
  }

  /**
   *  @brief Gets a value indicating whether there is a node for the given net
   */
  bool has_node_index_for_net (const db::Net *net) const
  {
    return m_net_index.find (net) != m_net_index.end ();
  }

  /**
   *  @brief Gets the node for a given node index
   */
  const db::NetGraphNode &node (size_t net_index) const
  {
    return m_nodes [net_index];
  }

  /**
   *  @brief Gets the node for a given node index (non-const version)
   */
  db::NetGraphNode &node (size_t net_index)
  {
    return m_nodes [net_index];
  }

  /**
   *  @brief Gets the subcircuit virtual node per subcircuit
   *  These nodes are a concept provided to reduce the effort for
   *  subcircuit transitions. Instead of a transition from every pin
   *  to every other pin the virtual node provides edges to
   *  all pins of the subcircuit, but no front end.
   */
  const db::NetGraphNode &virtual_node (const db::SubCircuit *sc) const
  {
    std::map<const db::SubCircuit *, db::NetGraphNode>::const_iterator j = m_virtual_nodes.find (sc);
    tl_assert (j != m_virtual_nodes.end ());
    return j->second;
  }

  /**
   *  @brief Gets the subcircuit virtual node per subcircuit
   */
  db::NetGraphNode &virtual_node (const db::SubCircuit *sc)
  {
    return const_cast<db::NetGraphNode &> (((const NetGraph *) this)->virtual_node (sc));
  }

  /**
   * @brief Creates a new node representing two joined nodes
   */
  NetGraphNode joined (const NetGraphNode &a, const NetGraphNode &b) const;

  /**
   *  @brief Gets the net for a given node index
   */
  const db::Net *net_by_node_index (size_t net_index) const
  {
    return m_nodes [net_index].net ();
  }

  /**
   *  @brief Establishes an equivalence between two nodes of netlist A (this) and B (other)
   */
  void identify (size_t net_index, size_t other_net_index, bool exact_match = true)
  {
    m_nodes [net_index].set_other_net (other_net_index, exact_match);
  }

  /**
   *  @brief Removes the equivalence from the node with the given index
   */
  void unidentify (size_t net_index)
  {
    m_nodes [net_index].unset_other_net ();
  }

  /**
   *  @brief Iterator over the nodes in this graph (begin)
   */
  node_iterator begin () const
  {
    return m_nodes.begin ();
  }

  /**
   *  @brief Iterator over the nodes in this graph (end)
   */
  node_iterator end () const
  {
    return m_nodes.end ();
  }

  /**
   *  @brief The circuit this graph is associated with
   */
  const db::Circuit *circuit () const
  {
    return mp_circuit;
  }

private:
  std::vector<NetGraphNode> m_nodes;
  std::map<const db::SubCircuit *, NetGraphNode> m_virtual_nodes;
  std::map<const db::Net *, size_t> m_net_index;
  const db::Circuit *mp_circuit;
};

}

#endif

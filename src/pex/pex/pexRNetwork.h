
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#ifndef HDR_pexRNetwork
#define HDR_pexRNetwork

#include "pexCommon.h"

#include "dbPolygon.h"
#include "dbPLC.h"
#include "tlList.h"

#include <string>
#include <list>
#include <limits>

namespace pex
{

class RElement;
class RNode;
class RNetwork;

/**
 *  @brief Represents a node in the R graph
 *
 *  A node connects to multiple elements (resistors).
 *  Every element has two nodes. The nodes and elements form
 *  a graph.
 *
 *  RNode object cannot be created directly. Use "create_node"
 *  from RNetwork.
 */
class PEX_PUBLIC RNode
  : public tl::list_node<RNode>
{
public:
  /**
   *  @brief The type of the node
   */
  enum node_type {
    Internal,      //  an internal node, not related to a port
    VertexPort,    //  a node related to a vertex port
    PolygonPort    //  a node related to a polygon port
  };

  /**
   *  @brief The node type
   */
  node_type type;

  /**
   *  @brief The location + extension of the node
   */
  db::DBox location;

  /**
   *  @brief An index locating the node in the vertex or polygon port lists
   *
   *  For internal nodes, the index is a unique numbers.
   */
  unsigned int port_index;

  /**
   *  @brief An index locating the node in a layer
   *
   *  For internal nodes, the layer is 0.
   */
  unsigned int layer;

  /**
   *  @brief Gets the R elements connected to this node
   */
  const std::list<const RElement *> &elements () const
  {
    return m_elements;
  }

  /**
   *  @brief Returns a string representation of the node
   */
  std::string to_string (bool with_coords = false) const;

  /**
   *  @brief Gets the network the node lives in
   */
  RNetwork *graph () const
  {
    return mp_network;
  }

protected:
  friend class RNetwork;
  friend class RElement;
  friend class tl::list_impl<RNode, false>;

  RNode (RNetwork *network, node_type _type, const db::DBox &_location, unsigned int _port_index, unsigned int _layer)
    : type (_type), location (_location), port_index (_port_index), layer (_layer), mp_network (network)
  { }

  ~RNode () { }

private:
  RNode (const RNode &other);
  RNode &operator= (const RNode &other);

  RNetwork *mp_network;
  mutable std::list<const RElement *> m_elements;
};

/**
 *  @brief Represents an R element in the graph (an edge)
 *
 *  An element has two nodes that form the ends of the edge and
 *  a conductance value (given in Siemens).
 *
 *  The value can be RElement::short_value() indicating
 *  "infinite" conductance (a short).
 *
 *  RElement objects cannot be created directly. Use "create_element"
 *  from RNetwork.
 */
class PEX_PUBLIC RElement
  : public tl::list_node<RElement>
{
public:
  /**
   *  @brief The conductance value
   */
  double conductance;

  /**
   *  @brief The nodes the resistor connects
   */
  const RNode *a () const { return mp_a; }
  const RNode *b () const { return mp_b; }

  /**
   *  @brief Gets the other node for n
   */
  const RNode *other (const RNode *n) const
  {
    if (mp_a == n) {
      return mp_b;
    } else if (mp_b == n) {
      return mp_a;
    }
    tl_assert (false);
  }

  /**
   *  @brief Represents the conductance value for a short
   */
  static double short_value ()
  {
    return std::numeric_limits<double>::infinity ();
  }

  /**
   *  @brief Gets the resistance value
   *
   *  The resistance value is the inverse of the conducance.
   */
  double resistance () const
  {
    return conductance == short_value () ? 0.0 : 1.0 / conductance;
  }

  /**
   *  @brief Returns a string representation of the element
   */
  std::string to_string (bool with_coords = false) const;

  /**
   *  @brief Gets the network the node lives in
   */
  RNetwork *graph () const
  {
    return mp_network;
  }

protected:
  friend class RNetwork;
  friend class tl::list_impl<RElement, false>;

  RElement (RNetwork *network, double _conductivity, const RNode *a, const RNode *b)
    : conductance (_conductivity), mp_network (network), mp_a (a), mp_b (b)
  { }

  ~RElement ()
  {
    if (mp_a) {
      mp_a->m_elements.erase (m_ia);
    }
    if (mp_b) {
      mp_b->m_elements.erase (m_ib);
    }
    mp_a = mp_b = 0;
  }

  std::list<const RElement *>::iterator m_ia, m_ib;
  RNetwork *mp_network;
  const RNode *mp_a, *mp_b;

private:
  RElement (const RElement &other);
  RElement &operator= (const RElement &other);
};

/**
 *  @brief Represents a R network (a graph of RNode and RElement)
 */
class PEX_PUBLIC RNetwork
  : public tl::Object
{
public:
  typedef tl::list<RNode, false> node_list;
  typedef node_list::const_iterator node_iterator;
  typedef tl::list<RElement, false> element_list;
  typedef element_list::const_iterator element_iterator;

  /**
   *  @brief Constructor
   */
  RNetwork ();

  /**
   *  @brief Destructor
   */
  ~RNetwork ();

  /**
   *  @brief Creates a node with the given type and port index
   *
   *  If the node type is Internal, a new node is created always.
   *  If the node type is VertexPort or PolygonPort, an existing
   *  node is returned if one way created with the same type
   *  or port index already. This avoids creating duplicates
   *  for the same port.
   */
  RNode *create_node (RNode::node_type type, unsigned int port_index, unsigned int layer);

  /**
   *  @brief Creates a new element between the given nodes
   *
   *  If an element already exists between the specified nodes, the
   *  given value is added to the existing element and the existing
   *  object is returned.
   */
  RElement *create_element (double conductance, RNode *a, RNode *b);

  /**
   *  @brief Removes the given element
   *
   *  Removing the element will also remove any orphan nodes
   *  at the ends if they are of type Internal.
   */
  void remove_element (RElement *element);

  /**
   *  @brief Removes the node and the attached elements.
   *
   *  Only nodes of type Internal can be removed.
   */
  void remove_node (RNode *node);

  /**
   *  @brief Clears the network
   */
  void clear ();

  /**
   *  @brief Simplifies the network
   *
   *  This will:
   *  - Join serial resistors if connected by an internal node
   *  - Remove shorts and join the nodes, if one of them is
   *    an internal node. The non-internal node will persist.
   *  - Remove "dangling" resistors if the dangling node is
   *    an internal one
   */
  void simplify ();

  /**
   *  @brief Iterate the nodes (begin)
   */
  node_iterator begin_nodes () const
  {
    return m_nodes.begin ();
  }

  /**
   *  @brief Iterate the nodes (end)
   */
  node_iterator end_nodes () const
  {
    return m_nodes.end ();
  }

  /**
   *  @brief Gets the number of nodes
   */
  size_t num_nodes () const
  {
    return m_nodes.size ();
  }

  /**
   *  @brief Gets the number of internal nodes
   */
  size_t num_internal_nodes () const
  {
    size_t count = 0;
    for (auto n = m_nodes.begin (); n != m_nodes.end (); ++n) {
      if (n->type == pex::RNode::Internal) {
        ++count;
      }
    }
    return count;
  }

  /**
   *  @brief Iterate the elements (begin)
   */
  element_iterator begin_elements () const
  {
    return m_elements.begin ();
  }

  /**
   *  @brief Iterate the elements (end)
   */
  element_iterator end_elements () const
  {
    return m_elements.end ();
  }

  /**
   *  @brief Gets the number of elements
   */
  size_t num_elements () const
  {
    return m_elements.size ();
  }

  /**
   *  @brief Returns a string representation of the graph
   */
  std::string to_string (bool with_coords = false) const;

private:
  node_list m_nodes;
  element_list m_elements;
  std::map<std::pair<RNode *, RNode *>, RElement *> m_elements_by_nodes;
  std::map<std::pair<RNode::node_type, std::pair<unsigned int, unsigned int> >, RNode *> m_nodes_by_type;

  RNetwork (const RNetwork &);
  RNetwork &operator= (const RNetwork &);

  void join_nodes (RNode *a, RNode *b);
};

}

#endif


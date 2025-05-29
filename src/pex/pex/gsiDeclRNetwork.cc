
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


#include "gsiDecl.h"
#include "pexRExtractor.h"
#include "pexSquareCountingRExtractor.h"
#include "pexTriangulationRExtractor.h"
#include "gsiEnums.h"

namespace gsi
{

class RNode
{
public:
  ~RNode () { }

  pex::RNode::node_type type () const { return checked_pointer ()->type; }
  db::DBox location () const { return checked_pointer ()->location; }
  unsigned int port_index () const { return checked_pointer ()->port_index; }
  unsigned int layer () const { return checked_pointer ()->layer; }
  std::string to_string (bool with_coords = false) const { return checked_pointer ()->to_string (with_coords); }

  size_t obj_id () const
  {
    return size_t (mp_ptr);
  }

  static RNode *make_node_object (const pex::RNode *node)
  {
    return new RNode (node);
  }

  const pex::RNode *checked_pointer () const
  {
    if (! mp_graph.get ()) {
      throw tl::Exception (tl::to_string (tr ("Network graph has been destroyed - RNode object no longer is valid")));
    }
    return mp_ptr;
  }

  pex::RNode *checked_pointer ()
  {
    if (! mp_graph.get ()) {
      throw tl::Exception (tl::to_string (tr ("Network graph has been destroyed - RNode object no longer is valid")));
    }
    return const_cast<pex::RNode *> (mp_ptr);
  }

private:
  tl::weak_ptr<pex::RNetwork> mp_graph;
  const pex::RNode *mp_ptr;

  RNode (const pex::RNode *node)
    : mp_graph (node->graph ()),
      mp_ptr (node)
  {
    //  .. nothing yet ..
  }
};

class RElement
{
public:
  ~RElement () { }

  double conductance () const { return checked_pointer ()->conductance; }
  double resistance () const { return checked_pointer ()->resistance (); }

  RNode *a () const { return RNode::make_node_object (checked_pointer ()->a ()); }
  RNode *b () const { return RNode::make_node_object (checked_pointer ()->b ()); }

  std::string to_string (bool with_coords = false) const { return checked_pointer ()->to_string (with_coords); }

  size_t obj_id () const
  {
    return size_t (mp_ptr);
  }

  static RElement *make_element_object (const pex::RElement *element)
  {
    return new RElement (element);
  }

  const pex::RElement *checked_pointer () const
  {
    if (! mp_graph.get ()) {
      throw tl::Exception (tl::to_string (tr ("Network graph has been destroyed - RElement object no longer is valid")));
    }
    return mp_ptr;
  }

  pex::RElement *checked_pointer ()
  {
    if (! mp_graph.get ()) {
      throw tl::Exception (tl::to_string (tr ("Network graph has been destroyed - RElement object no longer is valid")));
    }
    return const_cast<pex::RElement *> (mp_ptr);
  }

private:
  tl::weak_ptr<pex::RNetwork> mp_graph;
  const pex::RElement *mp_ptr;

  RElement (const pex::RElement *node)
    : mp_graph (node->graph ()),
      mp_ptr (node)
  {
    //  .. nothing yet ..
  }
};

class RElementIterator
{
public:
  typedef std::list<const pex::RElement *>::const_iterator basic_iter;
  typedef std::forward_iterator_tag iterator_category;
  typedef RElement *value_type;
  typedef RElement *reference;
  typedef void pointer;
  typedef void difference_type;

  RElementIterator (basic_iter it)
    : m_it (it)
  { }

  bool operator== (const RElementIterator &it) const { return m_it == it.m_it; }
  void operator++ () { ++m_it; }

  RElement *operator* () const
  {
    return RElement::make_element_object (*m_it);
  }

private:
  basic_iter m_it;
};

static RElementIterator begin_node_elements (RNode *node)
{
  return RElementIterator (node->checked_pointer ()->elements ().begin ());
}

static RElementIterator end_network_elements (RNode *node)
{
  return RElementIterator (node->checked_pointer ()->elements ().end ());
}

gsi::Enum<pex::RNode::node_type> decl_NodeType ("pex", "RNodeType",
  gsi::enum_const ("Internal", pex::RNode::Internal,
    "@brief Specifies an internal node in a R network\n"
    "Internal nodes are generated during the R extraction process. "
    "The port index of such a node is an arbitrary index."
  ) +
  gsi::enum_const ("VertexPort", pex::RNode::VertexPort,
    "@brief Specifies a vertex port node in a R network\n"
    "Vertex port nodes are generated for vertex ports in \\RExtractor#extract, see 'vertex_ports' argument. "
    "The port index of such a node refers to the position in that list."
  ) +
  gsi::enum_const ("PolygonPort", pex::RNode::PolygonPort,
    "@brief Specifies a polygon port node in a R network\n"
    "Polygon port nodes are generated for polygon ports in \\RExtractor#extract, see 'polygon_ports' argument. "
    "The port index of such a node refers to the position in that list."
  ),
  "@brief This class represents the node type for RNode.\n"
  "\n"
  "This class has been introduced in version 0.30.2"
);

Class<RNode> decl_RNode ("pex", "RNode",
  gsi::method ("object_id", &RNode::obj_id,
    "@brief Returns an ID representing the actual object\n"
    "For every call, a new instance of this object is created, while multiple "
    "ones may represent the same internal object. The 'object_id' is a ID that "
    "indicates the internal object. Same object_id means same node."
  ) +
  gsi::method ("to_s", &RNode::to_string, gsi::arg ("with_coords", false),
    "@brief Returns a string representation of this object\n"
    "Nodes are printed with coordinates with 'with_coords' is true."
  ) +
  gsi::iterator_ext ("each_element", gsi::return_new_object (), &begin_node_elements, &end_network_elements,
    "@brief Iterates the \\RElement objects attached to the node\n"
  ) +
  gsi::method ("type", &RNode::type,
    "@brief Gets the type attribute of the node\n"
  ) +
  gsi::method ("location", &RNode::location,
    "@brief Gets the location attribute of the node\n"
    "The location defined the original position of the node"
  ) +
  gsi::method ("port_index", &RNode::port_index,
    "@brief Gets the port index of the node\n"
    "The port index associates a node with a original port definition."
  ) +
  gsi::method ("layer", &RNode::layer,
    "@brief Gets the Layer ID of the node\n"
    "The port index associates a node with a original port definition layer-wise."
  ),
  "@brief Represents a node in a R network graph\n"
  "See \\RNetwork for a description of this object\n"
  "\n"
  "This class has been introduced in version 0.30.2"
);

//  Inject the RNode::node_type declarations into RNode
gsi::ClassExt<RNode> inject_NodeType_in_RNode (decl_NodeType.defs ());

Class<RElement> decl_RElement ("pex", "RElement",
  gsi::method ("object_id", &RElement::obj_id,
    "@brief Returns an ID representing the actual object\n"
    "For every call, a new instance of this object is created, while multiple "
    "ones may represent the same internal object. The 'object_id' is a ID that "
    "indicates the internal object. Same object_id means same element."
  ) +
  gsi::method ("to_s", &RElement::to_string, gsi::arg ("with_coords", false),
    "@brief Returns a string representation of this object\n"
    "Nodes are printed with coordinates with 'with_coords' is true."
  ) +
  gsi::method ("resistance", &RElement::resistance,
    "@brief Gets the resistance value of the object\n"
  ) +
  gsi::factory ("a", &RElement::a,
    "@brief Gets the first node the element connects\n"
  ) +
  gsi::factory ("b", &RElement::b,
    "@brief Gets the second node the element connects\n"
  ),
  "@brief Represents an edge (also called element) in a R network graph\n"
  "See \\RNetwork for a description of this object"
  "\n"
  "This class has been introduced in version 0.30.2"
);

static RNode *create_node (pex::RNetwork *network, pex::RNode::node_type type, unsigned int port_index, unsigned int layer)
{
  return RNode::make_node_object (network->create_node (type, port_index, layer));
}

static RElement *create_element (pex::RNetwork *network, double r, RNode *a, RNode *b)
{
  double s = fabs (r) < 1e-10 ? pex::RElement::short_value () : 1.0 / r;
  return RElement::make_element_object (network->create_element (s, a->checked_pointer (), b->checked_pointer ()));
}

static void remove_element (pex::RNetwork *network, RElement *element)
{
  network->remove_element (element->checked_pointer ());
}

static void remove_node (pex::RNetwork *network, RNode *node)
{
  network->remove_node (node->checked_pointer ());
}

class NetworkElementIterator
{
public:
  typedef pex::RNetwork::element_iterator basic_iter;
  typedef std::forward_iterator_tag iterator_category;
  typedef RElement *value_type;
  typedef RElement *reference;
  typedef void pointer;
  typedef void difference_type;

  NetworkElementIterator (basic_iter it)
    : m_it (it)
  { }

  bool operator== (const NetworkElementIterator &it) const { return m_it == it.m_it; }
  void operator++ () { ++m_it; }

  RElement *operator* () const
  {
    return RElement::make_element_object (m_it.operator-> ());
  }

private:
  basic_iter m_it;
};

static NetworkElementIterator begin_network_elements (pex::RNetwork *network)
{
  return NetworkElementIterator (network->begin_elements ());
}

static NetworkElementIterator end_network_elements (pex::RNetwork *network)
{
  return NetworkElementIterator (network->end_elements ());
}

class NetworkNodeIterator
{
public:
  typedef pex::RNetwork::node_iterator basic_iter;
  typedef std::forward_iterator_tag iterator_category;
  typedef RNode *value_type;
  typedef RNode *reference;
  typedef void pointer;
  typedef void difference_type;

  NetworkNodeIterator (basic_iter it)
    : m_it (it)
  { }

  bool operator== (const NetworkNodeIterator &it) const { return m_it == it.m_it; }
  void operator++ () { ++m_it; }

  RNode *operator* () const
  {
    return RNode::make_node_object (m_it.operator-> ());
  }

private:
  basic_iter m_it;
};

static NetworkNodeIterator begin_network_nodes (pex::RNetwork *network)
{
  return NetworkNodeIterator (network->begin_nodes ());
}

static NetworkNodeIterator end_network_nodes (pex::RNetwork *network)
{
  return NetworkNodeIterator (network->end_nodes ());
}

Class<pex::RNetwork> decl_RNetwork ("pex", "RNetwork",
  gsi::factory_ext ("create_node", &create_node, gsi::arg ("type"), gsi::arg ("port_index"), gsi::arg ("layer", (unsigned int) 0),
    "@brief Creates a new node with the given type and index'.\n"
    "@return A reference to the new nbode object."
  ) +
  gsi::factory_ext ("create_element", &create_element, gsi::arg ("resistance"), gsi::arg ("a"), gsi::arg ("b"),
    "@brief Creates a new element between the nodes given by 'a' abd 'b'.\n"
    "If a resistor already exists between the two nodes, both resistors are combined into one.\n"
    "@return A reference to the new resistor object."
  ) +
  gsi::method_ext ("remove_element", &remove_element, gsi::arg ("element"),
    "@brief Removes the given element\n"
    "If removing the element renders an internal node orphan (i.e. without elements), this "
    "node is removed too."
  ) +
  gsi::method_ext ("remove_node", &remove_node, gsi::arg ("node"),
    "@brief Removes the given node\n"
    "Only internal nodes can be removed. Removing a node will also remove the "
    "elements attached to this node."
  ) +
  gsi::method ("clear", &pex::RNetwork::clear,
    "@brief Clears the network\n"
  ) +
  gsi::method ("simplify", &pex::RNetwork::simplify,
    "@brief Simplifies the network\n"
    "\n"
    "This will:\n"
    "@ul\n"
    "@li Join serial resistors if connected by an internal node @/li\n"
    "@li Remove shorts and join the nodes, if one of them is\n"
    "    an internal node. The non-internal node will persist @/li\n"
    "@li Remove \"dangling\" resistors if the dangling node is\n"
    "    an internal one @/li\n"
    "@/ul\n"
  ) +
  gsi::iterator_ext ("each_element", gsi::return_new_object (), &begin_network_elements, &end_network_elements,
    "@brief Iterates the \\RElement objects inside the network\n"
  ) +
  gsi::iterator_ext ("each_node", gsi::return_new_object (), &begin_network_nodes, &end_network_nodes,
    "@brief Iterates the \\RNode objects inside the network\n"
  ) +
  gsi::method ("num_nodes", &pex::RNetwork::num_nodes,
    "@brief Gets the total number of nodes in the network\n"
  ) +
  gsi::method ("num_internal_nodes", &pex::RNetwork::num_internal_nodes,
    "@brief Gets the number of internal nodes in the network\n"
  ) +
  gsi::method ("num_elements", &pex::RNetwork::num_elements,
    "@brief Gets the number of elements in the network\n"
  ) +
  gsi::method ("to_s", &pex::RNetwork::to_string, gsi::arg ("with_coords", false),
    "@brief Returns a string representation of the network\n"
    "Nodes are printed with coordinates with 'with_coords' is true."
  ),
  "@brief Represents a network of resistors\n"
  "\n"
  "The network is basically a graph with nodes and edges (the resistors). "
  "The resistors are called 'elements' and are represented by \\RElement objects. "
  "The nodes are represented by \\RNode objects. "
  "The network is created by \\RExtractor#extract, which turns a polygon into a resistor network.\n"
  "\n"
  "This class has been introduced in version 0.30.2\n"
);

}


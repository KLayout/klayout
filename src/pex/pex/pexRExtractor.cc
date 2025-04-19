
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


#include "pexRExtractor.h"

namespace pex
{

// -----------------------------------------------------------------------------

std::string
RNode::to_string () const
{
  std::string res;
  switch (type) {
  default:
    res += "$" + tl::to_string (port_index);
    break;
  case VertexPort:
    res += "V" + tl::to_string (port_index);
    break;
  case PolygonPort:
    res += "P" + tl::to_string (port_index);
    break;
  }
  return res;
}

// -----------------------------------------------------------------------------

std::string
RElement::to_string () const
{
  std::string res = "R ";
  if (a ()) {
    res += a ()->to_string ();
  } else {
    res += "(nil)";
  }
  res += " ";
  if (b ()) {
    res += b ()->to_string ();
  } else {
    res += "(nil)";
  }
  res += " ";
  res += tl::sprintf ("%.6g", resistance ());
  return res;
}

// -----------------------------------------------------------------------------

RNetwork::RNetwork ()
{
  //  .. nothing yet ..
}

RNetwork::~RNetwork ()
{
  clear ();
}

std::string
RNetwork::to_string () const
{
  std::string res;
  for (auto e = m_elements.begin (); e != m_elements.end (); ++e) {
    if (! res.empty ()) {
      res += "\n";
    }
    res += e->to_string ();
  }
  return res;
}

void
RNetwork::clear ()
{
  m_elements.clear ();   //  must happen before m_nodes
  m_nodes.clear ();
  m_elements_by_nodes.clear ();
  m_nodes_by_type.clear ();
}

std::map<std::pair<RNode *, RNode *>, RElement *> m_elements_by_nodes;
std::map<std::pair<RNode::node_type, unsigned int>, RNode *> m_nodes;

RNode *
RNetwork::create_node (RNode::node_type type, unsigned int port_index)
{
  if (type != RNode::Internal) {

    auto i = m_nodes_by_type.find (std::make_pair (type, port_index));
    if (i != m_nodes_by_type.end ()) {

      return i->second;

    } else {

      RNode *new_node = new RNode (this, type, db::DBox (), port_index);
      m_nodes.push_back (new_node);
      m_nodes_by_type.insert (std::make_pair (std::make_pair (type, port_index), new_node));

      return new_node;
    }

  } else {

    RNode *new_node = new RNode (this, type, db::DBox (), port_index);
    m_nodes.push_back (new_node);
    return new_node;

  }
}

RElement *
RNetwork::create_element (double conductivity, RNode *a, RNode *b)
{
  auto i = m_elements_by_nodes.find (std::make_pair (a, b));
  if (i != m_elements_by_nodes.end ()) {

    i->second->conductivity += conductivity;
    return i->second;

  } else {

    RElement *element = new RElement (this, conductivity, a, b);
    m_elements.push_back (element);
    m_elements_by_nodes.insert (std::make_pair (std::make_pair (a, b), element));

    a->m_elements.push_back (element);
    element->m_ia = --a->m_elements.end ();
    b->m_elements.push_back (element);
    element->m_ib = --b->m_elements.end ();

    return element;

  }
}

void
RNetwork::remove_node (RNode *node)
{
  tl_assert (node->type == RNode::Internal);
  while (! node->m_elements.empty ()) {
    delete const_cast<RElement *> (node->m_elements.front ());
  }
  delete node;
}

void
RNetwork::remove_element (RElement *element)
{
  RNode *a = const_cast<RNode *> (element->a ());
  RNode *b = const_cast<RNode *> (element->b ());

  delete element;

  if (a && a->type == RNode::Internal && a->m_elements.empty ()) {
    delete a;
  }
  if (b && b->type == RNode::Internal && b->m_elements.empty ()) {
    delete b;
  }
}

// -----------------------------------------------------------------------------

RExtractor::RExtractor ()
{
  //  .. nothing yet ..
}

RExtractor::~RExtractor ()
{
  //  .. nothing yet ..
}

}

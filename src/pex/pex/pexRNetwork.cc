
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


#include "pexRNetwork.h"
#include "tlEquivalenceClusters.h"

namespace pex
{

// -----------------------------------------------------------------------------

std::string
RNode::to_string (bool with_coords) const
{
  std::string res;
  switch (type) {
  default:
    res += "$";
    break;
  case VertexPort:
    res += "V";
    break;
  case PolygonPort:
    res += "P";
    break;
  }

  res += tl::to_string (port_index);
  if (layer > 0) {
    res += ".";
    res += tl::to_string (layer);
  }

  if (with_coords) {
    res += location.to_string ();
  }

  return res;
}

// -----------------------------------------------------------------------------

std::string
RElement::to_string (bool with_coords) const
{
  std::string na;
  if (a ()) {
    na = a ()->to_string (with_coords);
  } else {
    na = "(nil)";
  }

  std::string nb;
  if (b ()) {
    nb = b ()->to_string (with_coords);
  } else {
    nb = "(nil)";
  }

  if (nb < na) {
    std::swap (na, nb);
  }

  std::string res = "R " + na + " " + nb + " ";
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
RNetwork::to_string (bool with_coords) const
{
  std::string res;
  for (auto e = m_elements.begin (); e != m_elements.end (); ++e) {
    if (! res.empty ()) {
      res += "\n";
    }
    res += e->to_string (with_coords);
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

RNode *
RNetwork::create_node (RNode::node_type type, unsigned int port_index, unsigned int layer)
{
  if (type != RNode::Internal) {

    auto i = m_nodes_by_type.find (std::make_pair (type, std::make_pair (port_index, layer)));
    if (i != m_nodes_by_type.end ()) {

      return i->second;

    } else {

      RNode *new_node = new RNode (this, type, db::DBox (), port_index, layer);
      m_nodes.push_back (new_node);
      m_nodes_by_type.insert (std::make_pair (std::make_pair (type, std::make_pair (port_index, layer)), new_node));

      return new_node;
    }

  } else {

    RNode *new_node = new RNode (this, type, db::DBox (), port_index, layer);
    m_nodes.push_back (new_node);
    return new_node;

  }
}

RElement *
RNetwork::create_element (double conductance, RNode *a, RNode *b)
{
  std::pair<RNode *, RNode *> key (a, b);
  if (size_t (b) < size_t (a)) {
    std::swap (key.first, key.second);
  }

  auto i = m_elements_by_nodes.find (key);
  if (i != m_elements_by_nodes.end ()) {

    if (conductance == pex::RElement::short_value () || i->second->conductance == pex::RElement::short_value ()) {
      i->second->conductance = pex::RElement::short_value ();
    } else {
      i->second->conductance += conductance;
    }

    return i->second;

  } else {

    RElement *element = new RElement (this, conductance, a, b);
    m_elements.push_back (element);
    m_elements_by_nodes.insert (std::make_pair (key, element));

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

void
RNetwork::join_nodes (RNode *a, RNode *b)
{
  for (auto e = b->elements ().begin (); e != b->elements ().end (); ++e) {
    RNode *on = const_cast<RNode *> ((*e)->other (b));
    if (on != a) {
      create_element ((*e)->conductance, on, a);
    }
  }

  a->location += b->location;
  remove_node (b);
}

void
RNetwork::simplify ()
{
  bool any_change = true;

  while (any_change) {

    any_change = false;

    //  join shorted clusters - we take care to remove internal nodes only

    tl::equivalence_clusters<const RNode *> clusters;
    for (auto e = m_elements.begin (); e != m_elements.end (); ++e) {
      if (e->conductance == pex::RElement::short_value () && (e->a ()->type == pex::RNode::Internal || e->b ()->type == pex::RNode::Internal)) {
        clusters.same (e->a (), e->b ());
      }
    }

    for (size_t ic = 1; ic <= clusters.size (); ++ic) {

      RNode *remaining = 0;
      RNode *first_node = 0;
      for (auto c = clusters.begin_cluster (ic); c != clusters.end_cluster (ic); ++c) {
        RNode *n = const_cast<RNode *> ((*c)->first);
        if (! first_node) {
          first_node = n;
        }
        if (n->type != pex::RNode::Internal) {
          remaining = n;
          break;
        }
      }

      if (! remaining) {
        //  Only internal nodes
        remaining = first_node;
      }

      for (auto c = clusters.begin_cluster (ic); c != clusters.end_cluster (ic); ++c) {
        RNode *n = const_cast<RNode *> ((*c)->first);
        if (n != remaining && n->type == pex::RNode::Internal) {
          any_change = true;
          join_nodes (remaining, n);
        }
      }

    }

    //  combine serial resistors if connected through an internal node

    std::vector<RNode *> nodes_to_remove;

    for (auto n = m_nodes.begin (); n != m_nodes.end (); ++n) {

      size_t nres = n->elements ().size ();

      if (n->type == pex::RNode::Internal && nres <= 2) {

        any_change = true;

        if (nres == 2) {

          auto e = n->elements ().begin ();

          RNode *n1 = const_cast<RNode *> ((*e)->other (n.operator-> ()));
          double r1 = (*e)->resistance ();

          ++e;
          RNode *n2 = const_cast<RNode *> ((*e)->other (n.operator-> ()));
          double r2 = (*e)->resistance ();

          double r = r1 + r2;
          if (r == 0.0) {
            create_element (pex::RElement::short_value (), n1, n2);
          } else {
            create_element (1.0 / r, n1, n2);
          }

        }

        nodes_to_remove.push_back (n.operator-> ());

      }

    }

    for (auto n = nodes_to_remove.begin (); n != nodes_to_remove.end (); ++n) {
      remove_node (*n);
    }

  }

}

}


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

RNetwork::RNetwork ()
{
  //  .. nothing yet ..
}

RNetwork::~RNetwork ()
{
  clear ();
}

void
RNetwork::clear ()
{
  m_elements.clear ();   //  must come before m_nodes
  m_nodes.clear ();
  m_elements_by_nodes.clear ();
  m_nodes_by_type.clear ();
}

std::map<std::pair<RNode *, RNode *>, RElement *> m_elements_by_nodes;
std::map<std::pair<RNode::node_type, unsigned int>, RNode *> m_nodes;

RNode *
RNetwork::create_node (RNode::node_type type, unsigned int port_index)
{
  if (type != RNode::INTERNAL) {

    auto i = m_nodes_by_type.find (std::make_pair (type, port_index));
    if (i != m_nodes_by_type.end ()) {

      return i->second;

    } else {

      RNode *new_node = new RNode (type, db::DBox (), port_index);
      m_nodes.push_back (new_node);
      m_nodes_by_type.insert (std::make_pair (std::make_pair (type, port_index), new_node));

      return new_node;
    }

  } else {

    RNode *new_node = new RNode (type, db::DBox (), port_index);
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

    RElement *element = new RElement (conductivity, a, b);
    a->elements.push_back (element);
    element->m_ia = --a->elements.end ();
    b->elements.push_back (element);
    element->m_ia = --b->elements.end ();

    m_elements_by_nodes.insert (std::make_pair (std::make_pair (a, b), element));
    return element;

  }
}

void
RNetwork::remove_node (RNode *node)
{
  tl_assert (node->type == RNode::INTERNAL);
  while (! node->elements.empty ()) {
    remove_element (const_cast<RElement *> (node->elements.front ()));
  }
  delete node;
}

void
RNetwork::remove_element (RElement *element)
{
  RNode *a = const_cast<RNode *> (element->a);
  RNode *b = const_cast<RNode *> (element->b);

  delete element;

  if (a && a->type == RNode::INTERNAL && a->elements.empty ()) {
    delete a;
  }
  if (b && b->type == RNode::INTERNAL && b->elements.empty ()) {
    delete b;
  }
}


RExtractor::RExtractor ()
{
  //  .. nothing yet ..
}

RExtractor::~RExtractor ()
{
  //  .. nothing yet ..
}


SquareCountingRExtractor::SquareCountingRExtractor (double dbu)
{
  m_dbu = dbu;

  m_decomp_param.split_edges = true;
  m_decomp_param.with_segments = false;
}

void
SquareCountingRExtractor::extract (const db::Polygon &polygon, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports, RNetwork &rnetwork)
{
  db::CplxTrans trans = db::CplxTrans (m_dbu) * db::ICplxTrans (db::Trans (db::Point () - polygon.box ().center ()));
  auto inv_trans = trans.inverted ();

  db::plc::Graph plc;

  db::plc::ConvexDecomposition decomp (&plc);
  decomp.decompose (polygon, vertex_ports, m_decomp_param, trans);

  std::vector<std::pair<db::Polygon, db::plc::Polygon *> > decomp_polygons;
  for (auto p = plc.begin (); p != plc.end (); ++p) {
    // @@@decomp_polygons.push_back (db::Polygon ());
    // @@@decomp_polygons.back ().first = inv_trans * p->polygon ();
  }

  //  @@@ use box_scanner to find interactions between polygon_ports and decomp_polygons

}

}



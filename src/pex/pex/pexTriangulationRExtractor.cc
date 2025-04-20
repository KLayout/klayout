
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


#include "pexTriangulationRExtractor.h"
#include "dbBoxScanner.h"
#include "dbPolygonTools.h"
#include "tlIntervalMap.h"

namespace pex
{

TriangulationRExtractor::TriangulationRExtractor (double dbu)
{
  m_dbu = dbu;

  m_tri_param.min_b = 0.3;
  m_tri_param.max_area = 0.0;
}

void
TriangulationRExtractor::extract (const db::Polygon &polygon, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports, pex::RNetwork &rnetwork)
{
  rnetwork.clear ();

  db::CplxTrans trans = db::CplxTrans (m_dbu) * db::ICplxTrans (db::Trans (db::Point () - polygon.box ().center ()));
  auto inv_trans = trans.inverted ();

  //  NOTE: currently we treat polygon ports and points where the location is the center of the bounding box
  std::vector<db::Point> vp = vertex_ports;
  vp.reserve (vertex_ports.size () + polygon_ports.size ());
  for (auto pp = polygon_ports.begin (); pp != polygon_ports.end (); ++pp) {
    vp.push_back (pp->box ().center ());
  }


  db::plc::Graph plc;
  db::plc::Triangulation tri (&plc);

  tri.triangulate (polygon, vp, m_tri_param, trans);

  //  create a network node for each triangle node

  std::unordered_map<const db::plc::Vertex *, pex::RNode *> vertex2node;

  size_t internal_node_id = 0;

  for (auto p = plc.begin (); p != plc.end (); ++p) {

    for (size_t iv = 0; iv < p->size (); ++iv) {

      const db::plc::Vertex *vertex = p->vertex (iv);
      if (vertex2node.find (vertex) != vertex2node.end ()) {
        continue;
      }

      pex::RNode::node_type type = pex::RNode::Internal;
      size_t port_index = 0;

      if (vertex->is_precious ()) {
        size_t idx = vertex->id ();
        if (idx >= vertex_ports.size ()) {
          type = pex::RNode::PolygonPort;
          port_index = size_t (idx) - vertex_ports.size ();
        } else {
          type = pex::RNode::VertexPort;
          port_index = size_t (idx);
        }
      } else {
        port_index = internal_node_id++;
      }

      pex::RNode *n = rnetwork.create_node (type, port_index);
      db::DPoint loc = *vertex;
      n->location = db::DBox (loc, loc);

      vertex2node.insert (std::make_pair (vertex, n));

    }

  }

  //  produce the conductances for each triangle

  for (auto p = plc.begin (); p != plc.end (); ++p) {
    create_conductances (*p, vertex2node, rnetwork);
  }

  //  eliminate internal nodes

  eliminate_all (rnetwork);
}

void
TriangulationRExtractor::create_conductances (const db::plc::Polygon &tri, const std::unordered_map<const db::plc::Vertex *, pex::RNode *> &vertex2node, RNetwork &rnetwork)
{
  tl_assert (tri.size () == 3);

  for (int i = 0; i < 3; ++i) {

    const db::plc::Vertex *pm1 = tri.vertex (i);
    const db::plc::Vertex *p0 = tri.vertex (i + 1);
    const db::plc::Vertex *p1 = tri.vertex (i + 2);

    double a = fabs (db::vprod (*pm1 - *p0, *p1 - *p0) * 0.5);

    double lm1 = (*p0 - *pm1).sq_length ();
    double l0 = (*p1 - *p0).sq_length ();
    double l1 = (*pm1 - *p1).sq_length ();

    double s = (l0 + l1 - lm1) / (8.0 * a);

    auto i0 = vertex2node.find (p0);
    auto im1 = vertex2node.find (pm1);
    rnetwork.create_element (s, i0->second, im1->second);

  }
}

void
TriangulationRExtractor::eliminate_all (RNetwork &rnetwork)
{
  if (tl::verbosity () >= m_tri_param.base_verbosity + 10) {
    tl::info << "Staring elimination with " << rnetwork.num_internal_nodes () << " internal nodes and " << rnetwork.num_elements () << " resistors";
  }

  unsigned int niter = 0;
  std::vector<pex::RNode *> to_eliminate;

  size_t nmax = 3;
  while (nmax > 0) {

    bool another_loop = true;
    while (another_loop) {

      size_t nmax_next = 0;
      to_eliminate.clear ();

      for (auto n = rnetwork.begin_nodes (); n != rnetwork.end_nodes (); ++n) {
        if (n->type == pex::RNode::Internal) {
          size_t nn = n->elements ().size ();
          if (nn <= nmax) {
            to_eliminate.push_back (const_cast<pex::RNode *> (n.operator-> ()));
          } else if (nmax_next == 0 or nn < nmax_next) {
            nmax_next = nn;
          }
        }
      }

      if (to_eliminate.empty ()) {

        another_loop = false;
        nmax = nmax_next;

        if (tl::verbosity () >= m_tri_param.base_verbosity + 10) {
          tl::info << "Nothing left to eliminate with nmax=" << nmax;
        }

      } else {

        for (auto n = to_eliminate.begin (); n != to_eliminate.end (); ++n) {
          eliminate_node (*n, rnetwork);
        }

        niter += 1;

        if (tl::verbosity () >= m_tri_param.base_verbosity + 10) {
          tl::info << "Nodes left after iteration " << niter << " with nmax=" << nmax << ": " << rnetwork.num_internal_nodes () << " with " << rnetwork.num_elements () << " edges.";
        }

      }

    }

  }
}

void
TriangulationRExtractor::eliminate_node (pex::RNode *node, RNetwork &rnetwork)
{
  double s_sum = 0.0;
  for (auto e = node->elements ().begin (); e != node->elements ().end (); ++e) {
    s_sum += (*e)->conductivity;
  }

  if (fabs (s_sum) > 1e-10) {
    for (auto e = node->elements ().begin (); e != node->elements ().end (); ++e) {
      auto ee = e;
      ++ee;
      for ( ; ee != node->elements ().end (); ++ee) {
        pex::RNode *n1 = const_cast <pex::RNode *> ((*e)->other (node));
        pex::RNode *n2 = const_cast <pex::RNode *> ((*ee)->other (node));
        double c = (*e)->conductivity * (*ee)->conductivity / s_sum;
        rnetwork.create_element (c, n1, n2);
      }
    }
  }

  rnetwork.remove_node (node);
}

}


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
  m_skip_reduction = false;

  m_tri_param.min_b = 0.3;
  m_tri_param.max_area = 0.0;
}

void
TriangulationRExtractor::extract (const db::Polygon &polygon, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports, pex::RNetwork &rnetwork)
{
  rnetwork.clear ();

  tl::SelfTimer timer (tl::verbosity () >= m_tri_param.base_verbosity + 1, "Extracting resistor network from polygon (TriangulationRExtractor)");

  db::CplxTrans trans = db::CplxTrans (m_dbu) * db::ICplxTrans (db::Trans (db::Point () - polygon.box ().center ()));
  db::CplxTrans dbu_trans = db::CplxTrans (m_dbu);
  db::DCplxTrans v2loc_trans = dbu_trans * trans.inverted ();  // vertex to node location

  db::plc::Graph plc;
  db::plc::Triangulation tri (&plc);

  std::unordered_map <const db::plc::Vertex *, size_t> pp_vertexes;

  if (polygon_ports.empty ()) {

    tri.triangulate (polygon, vertex_ports, m_tri_param, trans);

  } else {

    tl::SelfTimer timer_tri (tl::verbosity () >= m_tri_param.base_verbosity + 11, "Triangulation step");

    //  Subtract the polygon ports from the original polygon and compute the intersection.
    //  Hence we have coincident edges that we can use to identify the nodes that are
    //  connected for the polygon ports

    db::Region org (polygon);
    db::Region pp (polygon_ports.begin (), polygon_ports.end ());

    db::Region residual_poly = org - pp;

    //  We must not remove outside triangles yet, as we need them for "find_vertexes_along_line"
    db::plc::TriangulationParameters param = m_tri_param;
    param.remove_outside_triangles = false;

    tri.clear ();

    std::vector<std::vector<db::plc::Vertex *> > edge_contours;

    //  first step of the triangulation

    for (auto p = residual_poly.begin_merged (); ! p.at_end (); ++p) {
      tri.make_contours (*p, trans, edge_contours);
    }

    unsigned int id = 0;
    for (auto v = vertex_ports.begin (); v != vertex_ports.end (); ++v) {
      tri.insert_point (trans * *v)->set_is_precious (true, id++);
    }

    for (auto p = polygon_ports.begin (); p != polygon_ports.end (); ++p) {
      //  create vertexes for the port polygon vertexes - this ensures we will find vertexes
      //  on the edges of the polygons - yet, they may be outside of the original polygon.
      //  In that case they will not be considered
      for (auto e = p->begin_edge (); !e.at_end (); ++e) {
        tri.insert_point (trans * (*e).p1 ())->set_is_precious (true, id);
      }
    }

    //  constrain and refine the triangulation

    tri.constrain (edge_contours);
    tri.refine (param);

    //  identify the vertexes present for the polygon port -> store them inside pp_vertexes

    for (auto p = polygon_ports.begin (); p != polygon_ports.end (); ++p) {
      for (auto e = p->begin_edge (); !e.at_end (); ++e) {
        //  NOTE: this currently only works if one of the end points is an actual
        //  vertex.
        auto vport = tri.find_vertexes_along_line (trans * (*e).p1 (), trans * (*e).p2 ());
        for (auto v = vport.begin (); v != vport.end (); ++v) {
          pp_vertexes.insert (std::make_pair (*v, p - polygon_ports.begin ()));
        }
      }
    }

    tri.remove_outside_triangles ();

  }

  //  Create a network node for each triangle node.

  std::unordered_map<const db::plc::Vertex *, pex::RNode *> vertex2node;
  std::unordered_set<size_t> vports_present;
  std::map<size_t, pex::RNode *> pport_nodes;

  size_t internal_node_id = 0;

  for (auto p = plc.begin (); p != plc.end (); ++p) {

    for (size_t iv = 0; iv < p->size (); ++iv) {

      const db::plc::Vertex *vertex = p->vertex (int (iv));
      if (vertex2node.find (vertex) != vertex2node.end ()) {
        continue;
      }

      pex::RNode *n = 0;

      auto ipp = pp_vertexes.find (vertex);
      if (ipp != pp_vertexes.end ()) {

        size_t port_index = ipp->second;
        auto pn = pport_nodes.find (port_index);
        if (pn != pport_nodes.end ()) {
          n = pn->second;
        } else {
          n = rnetwork.create_node (pex::RNode::PolygonPort, (unsigned int) port_index, 0);
          pport_nodes.insert (std::make_pair (port_index, n));
          n->location = dbu_trans * polygon_ports [port_index].box ();
        }

      } else if (vertex->is_precious ()) {

        for (auto pi = vertex->ids ().begin (); pi != vertex->ids ().end (); ++pi) {
          size_t port_index = size_t (*pi);
          if (port_index < vertex_ports.size ()) {
            RNode *nn = rnetwork.create_node (pex::RNode::VertexPort, (unsigned int) port_index, 0);
            nn->location = v2loc_trans * db::DBox (*vertex, *vertex);
            if (n) {
              //  in case of multiple vertexes on the same spot, short them
              rnetwork.create_element (RElement::short_value (), n, nn);
            } else {
              n = nn;
            }
            vports_present.insert (port_index);
          }
        }

      } else {

        n = rnetwork.create_node (pex::RNode::Internal, (unsigned int) internal_node_id++, 0);
        n->location = v2loc_trans * db::DBox (*vertex, *vertex);

      }

      if (n) {
        vertex2node.insert (std::make_pair (vertex, n));
      }

    }

  }

  //  check for vertex ports not assigned to a node
  //  -> this may be an indication for a vertex port inside a polygon port

  for (size_t iv = 0; iv < vertex_ports.size (); ++iv) {

    if (vports_present.find (iv) != vports_present.end ()) {
      continue;
    }

    db::Point vp = vertex_ports [iv];

    for (auto p = polygon_ports.begin (); p != polygon_ports.end (); ++p) {

      if (p->box ().contains (vp) && db::inside_poly_test<db::Polygon> (*p) (vp) >= 0) {

        auto ip = pport_nodes.find (p - polygon_ports.begin ());
        if (ip != pport_nodes.end ()) {

          //  create a new vertex port and short it to the polygon port
          auto n = rnetwork.create_node (pex::RNode::VertexPort, (unsigned int) iv, 0);
          n->location = dbu_trans * db::Box (vp, vp);
          rnetwork.create_element (pex::RElement::short_value (), n, ip->second);

        }

      }
    }

  }

  //  produce the conductances for each triangle

  for (auto p = plc.begin (); p != plc.end (); ++p) {
    create_conductances (*p, vertex2node, rnetwork);
  }

  //  eliminate internal nodes

  if (! m_skip_reduction) {
    eliminate_all (rnetwork);
  }
}

void
TriangulationRExtractor::create_conductances (const db::plc::Polygon &tri, const std::unordered_map<const db::plc::Vertex *, pex::RNode *> &vertex2node, RNetwork &rnetwork)
{
  tl_assert (tri.size () == 3);

  for (int i = 0; i < 3; ++i) {

    const db::plc::Vertex *pm1 = tri.vertex (i);
    const db::plc::Vertex *p0 = tri.vertex (i + 1);
    const db::plc::Vertex *p1 = tri.vertex (i + 2);

    auto i0 = vertex2node.find (p0);
    auto im1 = vertex2node.find (pm1);

    if (i0->second != im1->second) {

      double a = fabs (db::vprod (*pm1 - *p0, *p1 - *p0) * 0.5);

      double lm1 = (*p0 - *pm1).sq_length ();
      double l0 = (*p1 - *p0).sq_length ();
      double l1 = (*pm1 - *p1).sq_length ();

      double s = (l0 + l1 - lm1) / (8.0 * a);

      rnetwork.create_element (s, i0->second, im1->second);

    }

  }
}

void
TriangulationRExtractor::eliminate_all (RNetwork &rnetwork)
{
  if (tl::verbosity () >= m_tri_param.base_verbosity + 10) {
    tl::info << "Starting elimination with " << rnetwork.num_internal_nodes () << " internal nodes and " << rnetwork.num_elements () << " resistors";
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
          } else if (nmax_next == 0 || nn < nmax_next) {
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
    s_sum += (*e)->conductance;
  }

  if (fabs (s_sum) > 1e-10) {
    for (auto e = node->elements ().begin (); e != node->elements ().end (); ++e) {
      auto ee = e;
      ++ee;
      for ( ; ee != node->elements ().end (); ++ee) {
        pex::RNode *n1 = const_cast <pex::RNode *> ((*e)->other (node));
        pex::RNode *n2 = const_cast <pex::RNode *> ((*ee)->other (node));
        double c = (*e)->conductance * (*ee)->conductance / s_sum;
        rnetwork.create_element (c, n1, n2);
      }
    }
  }

  rnetwork.remove_node (node);
}

}

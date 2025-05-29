
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


#include "dbPLCConvexDecomposition.h"
#include "dbPLCTriangulation.h"
#include "tlLog.h"
#include "tlTimer.h"

#include <set>
#include <memory>
#include <vector>
#include <map>

namespace db
{

namespace plc
{

ConvexDecomposition::ConvexDecomposition (Graph *graph)
{
  mp_graph = graph;
  clear ();
}

void
ConvexDecomposition::clear ()
{
  mp_graph->clear ();
}

struct SortAngleAndEdgesByEdgeLength
{
  typedef std::list<std::pair<double, const Edge *> > angle_and_edges_list;

  bool operator() (const angle_and_edges_list::iterator &a, const angle_and_edges_list::iterator &b) const
  {
    double la = a->second->edge ().double_sq_length ();
    double lb = b->second->edge ().double_sq_length ();
    if (fabs (la - lb) > db::epsilon) {
      return la < lb;
    } else {
      return a->second->edge ().less (b->second->edge ());
    }
  }
};

//  TODO: move to some generic header
template <class T>
struct less_compare_func
{
  bool operator() (const T &a, const T &b) const
  {
    return a.less (b);
  }
};

//  TODO: move to some generic header
template <class T>
struct equal_compare_func
{
  bool operator() (const T &a, const T &b) const
  {
    return a.equal (b);
  }
};

Edge *find_outgoing_segment (Vertex *vertex, Edge *incoming, int &vp_max_sign)
{
  Vertex *vfrom = incoming->other (vertex);
  db::DEdge e1 (*vfrom, *vertex);

  double vp_max = 0.0;
  vp_max_sign = 0;
  Edge *outgoing = 0;

  //  Look for the outgoing edge. We pick the one which bends "most", favoring
  //  convex corners. Multiple edges per vertex are possible is corner cases such as the
  //  "hourglass" configuration.

  for (auto e = vertex->begin_edges (); e != vertex->end_edges (); ++e) {

    Edge *en = *e;
    if (en != incoming && en->is_segment ()) {

      Vertex *v = en->other (vertex);
      db::DEdge e2 (*vertex, *v);
      double vp = double (db::vprod (e1, e2)) / (e1.double_length () * e2.double_length ());

      //  vp > 0: concave, vp < 0: convex

      if (! outgoing || vp > vp_max) {
        vp_max_sign = db::vprod_sign (e1, e2);
        vp_max = vp;
        outgoing = en;
      }

    }

  }

  tl_assert (outgoing != 0);
  return outgoing;
}

void
ConvexDecomposition::collect_concave_vertexes (std::vector<ConcaveCorner> &concave_vertexes)
{
  concave_vertexes.clear ();

  std::unordered_set<Edge *> left;

  for (auto e = mp_graph->edges ().begin (); e != mp_graph->edges ().end (); ++e) {
    if (e->is_segment () && (e->left () != 0 || e->right () != 0)) {
      left.insert (e.operator-> ());
    }
  }

  while (! left.empty ()) {

    //  First segment for a new loop
    Edge *segment = *left.begin ();

    //  walk along the segments in clockwise direction. Find concave
    //  vertexes and create new vertexes perpendicular to the incoming
    //  and outgoing edge.

    Edge *start_segment = segment;
    Vertex *vto = (segment->right () && ! segment->right ()->is_outside ()) ? segment->v2 () : segment->v1 ();

    do {

      left.erase (segment);

      Edge *prev_segment = segment;

      int vp_sign = 0;
      segment = find_outgoing_segment (vto, prev_segment, vp_sign);

      if (vp_sign > 0) {
        concave_vertexes.push_back (ConcaveCorner (vto, prev_segment, segment));
      }

      vto = segment->other (vto);

    } while (segment != start_segment);

  }
}

std::pair<bool, db::DPoint>
ConvexDecomposition::search_crossing_with_next_segment (const Vertex *v0, const db::DVector &direction)
{
  auto vtri = v0->polygons ();  //  TODO: slow?
  std::vector<const Vertex *> nvv, nvv_next;

  for (auto it = vtri.begin (); it != vtri.end (); ++it) {

    //  Search for a segment in the direction perpendicular to the edge
    nvv.clear ();
    nvv.push_back (v0);
    const Polygon *t = *it;

    while (! nvv.empty ()) {

      nvv_next.clear ();

      for (auto iv = nvv.begin (); iv != nvv.end (); ++iv) {

        const Vertex *v = *iv;
        const Edge *oe = t->opposite (v);
        const Polygon *tt = oe->other (t);
        const Vertex *v1 = oe->v1 ();
        const Vertex *v2 = oe->v2 ();

        if (db::sprod_sign (*v2 - *v, direction) >= 0 && db::sprod_sign (*v1 - *v, direction) >= 0 &&
            db::vprod_sign (*v2 - *v, direction) * db::vprod_sign (*v1 - *v, direction) < 0) {

          //  this triangle covers the normal vector of e1 -> stop here or continue searching in that direction
          if (oe->is_segment ()) {
            auto cp = oe->edge ().cut_point (db::DEdge (*v0, *v0 + direction));
            if (cp.first) {
              return std::make_pair (true, cp.second);
            }
          } else {
            //  continue searching in that direction
            nvv_next.push_back (v1);
            nvv_next.push_back (v2);
            t = tt;
          }

          break;

        }

      }

      nvv.swap (nvv_next);

    }

  }

  return std::make_pair (false, db::DPoint ());
}

void
ConvexDecomposition::hertel_mehlhorn_decomposition (Triangulation &tris, const ConvexDecompositionParameters &param)
{
  bool with_segments = param.with_segments;
  bool split_edges = param.split_edges;

  std::vector<ConcaveCorner> concave_vertexes;
  collect_concave_vertexes (concave_vertexes);

  std::vector<db::DPoint> new_points;

  if (with_segments) {

    //  Create internal segments cutting off pieces orthogonal to the edges
    //  connecting the concave vertex.

    for (auto cc = concave_vertexes.begin (); cc != concave_vertexes.end (); ++cc) {

      for (unsigned int ei = 0; ei < 2; ++ei) {

        db::DEdge ee;
        const Vertex *v0 = cc->corner;
        if (ei == 0) {
          ee = db::DEdge (*cc->incoming->other (v0), *v0);
        } else {
          ee = db::DEdge (*v0, *cc->outgoing->other (v0));
        }

        auto cp = search_crossing_with_next_segment (v0, db::DVector (ee.dy (), -ee.dx ()));
        if (cp.first) {
          new_points.push_back (cp.second);
        }

      }

    }

  }

  //  eliminate duplicates and put the new points in some order

  if (! new_points.empty ()) {

    std::sort (new_points.begin (), new_points.end (), less_compare_func<db::DPoint> ());
    new_points.erase (std::unique (new_points.begin (), new_points.end (), equal_compare_func<db::DPoint> ()), new_points.end ());

    //  Insert the new points and make connections
    for (auto p = new_points.begin (); p != new_points.end (); ++p) {
      tris.insert_point (*p);
    }

    //  As the insertion invalidates the edges, we need to collect the concave vertexes again
    collect_concave_vertexes (concave_vertexes);

  }

  //  Collect essential edges
  //  Every concave vertex can have up to two essential edges.
  //  Other then suggested by Hertel-Mehlhorn we don't pick
  //  them one-by-one, but using them in length order, from the

  std::unordered_set<const Edge *> essential_edges;

  typedef std::list<std::pair<double, const Edge *> > angles_and_edges_list;
  angles_and_edges_list angles_and_edges;
  std::vector<angles_and_edges_list::iterator> sorted_edges;

  for (auto cc = concave_vertexes.begin (); cc != concave_vertexes.end (); ++cc) {

    angles_and_edges.clear ();
    const Vertex *v0 = cc->corner;

    const Edge *e = cc->incoming;
    while (e) {

      const Polygon *t = e->v2 () == v0 ? e->right () : e->left ();
      tl_assert (t != 0);

      const Edge *en = t->next_edge (e, v0);
      tl_assert (en != 0);

      db::DVector v1 = e->edge ().d () * (e->v1 () == v0 ? 1.0 : -1.0);
      db::DVector v2 = en->edge ().d () * (en->v1 () == v0 ? 1.0 : -1.0);

      double angle = atan2 (db::vprod (v1, v2), db::sprod (v1, v2));

      e = (en == cc->outgoing) ? 0 : en;
      angles_and_edges.push_back (std::make_pair (angle, e));

    }

    sorted_edges.clear ();

    for (auto i = angles_and_edges.begin (); i != angles_and_edges.end (); ++i) {
      if (i->second) {
        sorted_edges.push_back (i);
      }
    }

    std::sort (sorted_edges.begin (), sorted_edges.end (), SortAngleAndEdgesByEdgeLength ());

    for (auto i = sorted_edges.end (); i != sorted_edges.begin (); ) {
      --i;
      angles_and_edges_list::iterator ii = *i;
      angles_and_edges_list::iterator iin = ii;
      ++iin;
      if (ii->first + iin->first < (split_edges ? M_PI + db::epsilon : M_PI - db::epsilon)) {
        //  not an essential edge -> remove
        iin->first += ii->first;
        angles_and_edges.erase (ii);
      }
    }

    for (auto i = angles_and_edges.begin (); i != angles_and_edges.end (); ++i) {
      if (i->second) {
        essential_edges.insert (i->second);
      }
    }

  }

  //  Combine triangles, but don't cross essential edges

  std::unordered_set<const Polygon *> left_triangles;
  for (auto it = mp_graph->begin (); it != mp_graph->end (); ++it) {
    if (! it->is_outside ()) {
      left_triangles.insert (it.operator-> ());
    }
  }

  std::list<std::unordered_set<Edge *> > polygons;
  std::list<std::unordered_set<Vertex *> > internal_vertexes;

  while (! left_triangles.empty ()) {

    polygons.push_back (std::unordered_set<Edge *> ());
    std::unordered_set<Edge *> &edges = polygons.back ();

    internal_vertexes.push_back (std::unordered_set<Vertex *> ());
    std::unordered_set<Vertex *> &ivs = internal_vertexes.back ();

    const Polygon *tri = *left_triangles.begin ();
    std::vector<const Polygon *> queue, next_queue;
    queue.push_back (tri);

    while (! queue.empty ()) {

      next_queue.clear ();

      for (auto q = queue.begin (); q != queue.end (); ++q) {

        left_triangles.erase (*q);

        for (unsigned int i = 0; i < 3; ++i) {

          const Edge *e = (*q)->edge (i);
          const Polygon *qq = e->other (*q);

          if (e->v1 ()->is_precious ()) {
            ivs.insert (e->v1 ());
          }
          if (e->v2 ()->is_precious ()) {
            ivs.insert (e->v2 ());
          }

          if (! qq || qq->is_outside () || essential_edges.find (e) != essential_edges.end ()) {
            edges.insert (const_cast<Edge *> (e));  //  TODO: ugly const_cast
          } else if (left_triangles.find (qq) != left_triangles.end ()) {
            next_queue.push_back (qq);
          }
        }

      }

      queue.swap (next_queue);

    }

  }

  //  remove the triangles

  while (mp_graph->begin () != mp_graph->end ()) {
    delete mp_graph->begin ().operator-> ();
  }

  //  create the polygons

  auto iv = internal_vertexes.begin ();
  for (auto p = polygons.begin (); p != polygons.end (); ++p) {
    Polygon *poly = mp_graph->create_polygon (p->begin (), p->end ());
    poly->reserve_internal_vertexes (iv->size ());
    for (auto i = iv->begin (); i != iv->end (); ++i) {
      poly->add_internal_vertex (*i);
    }
    ++iv;
  }
}

void
ConvexDecomposition::decompose (const db::Polygon &poly, const ConvexDecompositionParameters &parameters, double dbu)
{
  decompose (poly, parameters, db::CplxTrans (dbu));
}

void
ConvexDecomposition::decompose (const db::Polygon &poly, const std::vector<db::Point> &vertexes, const ConvexDecompositionParameters &parameters, double dbu)
{
  decompose (poly, vertexes, parameters, db::CplxTrans (dbu));
}

void
ConvexDecomposition::decompose (const db::Polygon &poly, const ConvexDecompositionParameters &parameters, const db::CplxTrans &trans)
{
  Triangulation tri (mp_graph);
  tri.triangulate (poly, parameters.tri_param, trans);

  hertel_mehlhorn_decomposition (tri, parameters);
}

void
ConvexDecomposition::decompose (const db::Polygon &poly, const std::vector<db::Point> &vertexes, const ConvexDecompositionParameters &parameters, const db::CplxTrans &trans)
{
  Triangulation tri (mp_graph);
  tri.triangulate (poly, vertexes, parameters.tri_param, trans);

  hertel_mehlhorn_decomposition (tri, parameters);
}

void
ConvexDecomposition::decompose (const db::DPolygon &poly, const ConvexDecompositionParameters &parameters, const db::DCplxTrans &trans)
{
  Triangulation tri (mp_graph);
  tri.triangulate (poly, parameters.tri_param, trans);

  hertel_mehlhorn_decomposition (tri, parameters);
}

void
ConvexDecomposition::decompose (const db::DPolygon &poly, const std::vector<db::DPoint> &vertexes, const ConvexDecompositionParameters &parameters, const db::DCplxTrans &trans)
{
  Triangulation tri (mp_graph);
  tri.triangulate (poly, vertexes, parameters.tri_param, trans);

  hertel_mehlhorn_decomposition (tri, parameters);
}

void
ConvexDecomposition::decompose (const db::Region &region, const ConvexDecompositionParameters &parameters, double dbu)
{
  decompose (region, parameters, db::CplxTrans (dbu));
}

void
ConvexDecomposition::decompose (const db::Region &region, const ConvexDecompositionParameters &parameters, const db::CplxTrans &trans)
{
  Triangulation tri (mp_graph);
  tri.triangulate (region, parameters.tri_param, trans);

  hertel_mehlhorn_decomposition (tri, parameters);
}

}  //  namespace plc

}  //  namespace db

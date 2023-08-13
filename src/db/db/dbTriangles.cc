
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


#include "dbTriangles.h"
#include "dbLayout.h"
#include "dbWriter.h"
#include "tlStream.h"
#include "tlLog.h"

#include <set>

namespace db
{

Triangles::Triangles ()
  : m_is_constrained (false), m_level (0)
{
  //  .. nothing yet ..
}

Triangles::~Triangles ()
{
  while (! mp_triangles.empty ()) {
    remove (mp_triangles.front ());
  }

  tl_assert (mp_edges.empty ());
}

db::Vertex *
Triangles::create_vertex (double x, double y)
{
  m_vertex_heap.push_back (db::Vertex (x, y));
  return &m_vertex_heap.back ();
}

db::Vertex *
Triangles::create_vertex (const db::DPoint &pt)
{
  m_vertex_heap.push_back (pt);
  return &m_vertex_heap.back ();
}

db::TriangleEdge *
Triangles::create_edge (db::Vertex *v1, db::Vertex *v2)
{
  db::TriangleEdge *res = new db::TriangleEdge (v1, v2);
  mp_edges.push_back (res);
  return res;
}

db::Triangle *
Triangles::create_triangle (TriangleEdge *e1, TriangleEdge *e2, TriangleEdge *e3)
{
  db::Triangle *res = new db::Triangle (e1, e2, e3);
  mp_triangles.push_back (res);
  return res;
}

void
Triangles::remove (db::Triangle *tri)
{
  db::TriangleEdge *edges [3];
  for (int i = 0; i < 3; ++i) {
    edges [i] = tri->edge (i);
  }

  delete tri;

  //  clean up edges we do no longer need
  for (int i = 0; i < 3; ++i) {
    if (edges [i] && edges [i]->left () == 0 && edges [i]->right () == 0) {
      delete edges [i];
    }
  }
}

void
Triangles::init_box (const db::DBox &box)
{
  double xmin = box.left (), xmax = box.right ();
  double ymin = box.bottom (), ymax = box.top ();

  db::Vertex *vbl = create_vertex (xmin, ymin);
  db::Vertex *vtl = create_vertex (xmin, ymax);
  db::Vertex *vbr = create_vertex (xmax, ymin);
  db::Vertex *vtr = create_vertex (xmax, ymax);

  db::TriangleEdge *sl = create_edge (vbl, vtl);
  db::TriangleEdge *sd = create_edge (vtl, vbr);
  db::TriangleEdge *sb = create_edge (vbr, vbl);

  db::TriangleEdge *sr = create_edge (vbr, vtr);
  db::TriangleEdge *st = create_edge (vtr, vtl);

  create_triangle (sl, sd, sb);
  create_triangle (sd, sr, st);
}

std::string
Triangles::to_string ()
{
  std::string res;
  for (auto t = mp_triangles.begin (); t != mp_triangles.end (); ++t) {
    if (! res.empty ()) {
      res += ", ";
    }
    res += t->to_string ();
  }
  return res;
}

db::DBox
Triangles::bbox () const
{
  db::DBox box;
  for (auto t = mp_triangles.begin (); t != mp_triangles.end (); ++t) {
    for (int i = 0; i < 3; ++i) {
      box += *t->vertex (i);
    }
  }
  return box;
}

bool
Triangles::check (bool check_delaunay) const
{
  bool res = true;

  if (check_delaunay) {
    for (auto t = mp_triangles.begin (); t != mp_triangles.end (); ++t) {
      auto cp = t->circumcircle ();
      auto vi = find_inside_circle (cp.first, cp.second);
      if (! vi.empty ()) {
        res = false;
        tl::error << "(check error) triangle does not meet Delaunay criterion: " << t->to_string ();
        for (auto v = vi.begin (); v != vi.end (); ++v) {
          tl::error << "  vertex inside circumcircle: " << (*v)->to_string (true);
        }
      }
    }
  }

  for (auto t = mp_triangles.begin (); t != mp_triangles.end (); ++t) {
    for (int i = 0; i < 3; ++i) {
      if (! t->edge (i)->has_triangle (t.operator-> ())) {
        tl::error << "(check error) edges " << t->edge (i)->to_string (true)
                  << " attached to triangle " << t->to_string (true) << " does not refer to this triangle";
        res = false;
      }
    }
  }

  for (auto e = mp_edges.begin (); e != mp_edges.end (); ++e) {

    if (e->left () && e->right ()) {
      if (e->left ()->is_outside () != e->right ()->is_outside () && ! e->is_segment ()) {
        tl::error << "(check error) edge " << e->to_string (true) << " splits an outside and inside triangle, but is not a segment";
        res = false;
      }
    }

    for (auto t = e->begin_triangles (); t != e->end_triangles (); ++t) {
      if (! t->has_edge (e.operator-> ())) {
        tl::error << "(check error) edge " << e->to_string (true) << " not found in adjacent triangle " << t->to_string (true);
        res = false;
      }
      if (! t->has_vertex (e->v1 ())) {
        tl::error << "(check error) edges " << e->to_string (true) << " vertex 1 not found in adjacent triangle " << t->to_string (true);
        res = false;
      }
      if (! t->has_vertex (e->v2 ())) {
        tl::error << "(check error) edges " << e->to_string (true) << " vertex 2 not found in adjacent triangle " << t->to_string (true);
        res = false;
      }
      db::Vertex *vopp = t->opposite (e.operator-> ());
      double sgn = (e->left () == t.operator-> ()) ? 1.0 : -1.0;
      double vp = db::vprod (e->d(), *vopp - *e->v1 ());  //  positive if on left side
      if (vp * sgn <= 0.0) {
        const char * side_str = sgn > 0.0 ? "left" : "right";
        tl::error << "(check error) external point " << vopp->to_string (true) << " not on " << side_str << " side of edge " << e->to_string (true);
        res = false;
      }
    }

    if (! e->v1 ()->has_edge (e.operator-> ())) {
      tl::error << "(check error) edge " << e->to_string (true) << " vertex 1 does not list this edge";
      res = false;
    }
    if (! e->v2 ()->has_edge (e.operator-> ())) {
      tl::error << "(check error) edge " << e->to_string (true) << " vertex 2 does not list this edge";
      res = false;
    }

  }

  for (auto v = m_vertex_heap.begin (); v != m_vertex_heap.end (); ++v) {
    unsigned int num_outside_edges = 0;
    for (auto e = v->begin_edges (); e != v->end_edges (); ++e) {
      if (e->is_outside ()) {
        ++num_outside_edges;
      }
    }
    if (num_outside_edges > 0 && num_outside_edges != 2) {
      tl::error << "(check error) vertex " << v->to_string (true) << " has " << num_outside_edges << " outside edges (can only be 2)";
      res = false;
      for (auto e = v->begin_edges (); e != v->end_edges (); ++e) {
        if (e->is_outside ()) {
          tl::error << "  Outside edge is " << e->to_string (true);
        }
      }
    }
  }

  return res;
}

db::Layout *
Triangles::to_layout () const
{
  db::Layout *layout = new db::Layout ();
  layout->dbu (0.001);

  auto dbu_trans = db::CplxTrans (layout->dbu ()).inverted ();

  db::Cell &top = layout->cell (layout->add_cell ("DUMP"));
  unsigned int l1 = layout->insert_layer (db::LayerProperties (1, 0));
  unsigned int l2 = layout->insert_layer (db::LayerProperties (2, 0));
  unsigned int l10 = layout->insert_layer (db::LayerProperties (10, 0));

  for (auto t = mp_triangles.begin (); t != mp_triangles.end (); ++t) {
    db::DPoint pts[3];
    for (int i = 0; i < 3; ++i) {
      pts[i] = *t->vertex (i);
    }
    db::DPolygon poly;
    poly.assign_hull (pts + 0, pts + 3);
    top.shapes (t->is_outside () ? l2 : l1).insert (dbu_trans * poly);
  }

  for (auto e = mp_edges.begin (); e != mp_edges.end (); ++e) {
    top.shapes (l10).insert (dbu_trans * e->edge ());
  }

  return layout;
}

void
Triangles::dump (const std::string &path) const
{
  std::unique_ptr<db::Layout> ly (to_layout ());

  tl::OutputStream stream (path);

  db::SaveLayoutOptions opt;
  db::Writer writer (opt);
  writer.write (*ly, stream);

  tl::info << "Triangles written to " << path;
}

std::vector<db::Vertex *>
Triangles::find_points_around (db::Vertex *vertex, double radius)
{
  std::set<const db::Vertex *> seen;
  seen.insert (vertex);

  std::vector<db::Vertex *> res;
  std::vector<db::Vertex *> new_vertexes, next_vertexes;
  new_vertexes.push_back (vertex);

  while (! new_vertexes.empty ()) {
    next_vertexes.clear ();
    for (auto v = new_vertexes.begin (); v != new_vertexes.end (); ++v) {
      for (auto e = (*v)->begin_edges (); e != (*v)->end_edges (); ++e) {
        db::Vertex *ov = e->other (*v);
        if (ov->in_circle (*vertex, radius) == 1 && seen.insert (*v).second) {
          next_vertexes.push_back (ov);
          res.push_back (ov);
        }
      }
    }
    new_vertexes.swap (next_vertexes);
  }

  return res;
}

db::Vertex *
Triangles::insert_point (const db::DPoint &point, std::vector<db::Triangle *> *new_triangles)
{
  return insert (create_vertex (point), new_triangles);
}

db::Vertex *
Triangles::insert (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles)
{
  std::vector<db::Triangle *> tris = find_triangle_for_point (*vertex);

  //  the new vertex is outside the domain
  if (tris.empty ()) {
    tl_assert (! m_is_constrained);
    insert_new_vertex (vertex, new_triangles);
    return vertex;
  }

  //  the new vertex is on the edge between two triangles
  std::vector<db::TriangleEdge *> on_edges;
  for (int i = 0; i < 3; ++i) {
    db::TriangleEdge *e = tris.front ()->edge (i);
    if (e->side_of (*vertex) == 0) {
      on_edges.push_back (e);
    }
  }

  if (! on_edges.empty ()) {
    if (on_edges.size () == size_t (1)) {
      split_triangles_on_edge (tris, vertex, on_edges.front (), new_triangles);
      return vertex;
    } else {
      //  the vertex is already present
      tl_assert (on_edges.size () == size_t (2));
      return on_edges.front ()->common_vertex (on_edges [1]);
    }
  } else if (tris.size () == size_t (1)) {
    //  the new vertex is inside one triangle
    split_triangle (tris.front (), vertex, new_triangles);
    return vertex;
  }

  tl_assert (false);
}

std::vector<db::Triangle *>
Triangles::find_triangle_for_point (const db::DPoint &point)
{
  db::TriangleEdge *edge = find_closest_edge (point);

  std::vector<db::Triangle *> res;
  if (edge) {
    for (auto t = edge->begin_triangles (); t != edge->end_triangles (); ++t) {
      if (t->contains (point) >= 0) {
        res.push_back (t.operator-> ());
      }
    }
  }
  return res;
}

db::TriangleEdge *
Triangles::find_closest_edge (const db::DPoint &p, db::Vertex *vstart, bool inside_only)
{
  if (!vstart) {
    if (! mp_triangles.empty ()) {
      vstart = mp_triangles.front ()->vertex (0);
    } else {
      return 0;
    }
  }

  db::DEdge line (*vstart, p);

  double d = -1.0;
  db::TriangleEdge *edge = 0;
  db::Vertex *v = vstart;

  while (v) {

    db::Vertex *vnext = 0;

    for (auto e = v->begin_edges (); e != v->end_edges (); ++e) {

      if (inside_only) {
        //  NOTE: in inside mode we stay on the line of sight as we don't
        //  want to walk around outside pockets.
        if (! e->is_segment () && e->is_for_outside_triangles ()) {
          continue;
        }
        if (! e->crosses_including (line)) {
          continue;
        }
      }

      double ds = e->distance (p);

      if (d < 0.0 || ds < d) {

        d = ds;
        edge = const_cast<db::TriangleEdge *> (e.operator-> ());
        vnext = edge->other (v);

      } else if (fabs (ds - d) < std::max (1.0, fabs (ds) + fabs (d)) * db::epsilon) {

        //  this differentiation selects the edge which bends further towards
        //  the target point if both edges share a common point and that
        //  is the one the determines the distance.
        db::Vertex *cv = edge->common_vertex (e.operator-> ());
        if (cv) {
          db::DVector edge_d = *edge->other (cv) - *cv;
          db::DVector e_d = *e->other(cv) - *cv;
          db::DVector r = p - *cv;
          double edge_sp = db::sprod (r, edge_d) / edge_d.length ();
          double s_sp = db::sprod (r, e_d) / e_d.length ();
          if (s_sp > edge_sp) {
            edge = const_cast<db::TriangleEdge *> (e.operator-> ());
            vnext = edge->other (v);
          }
        }

      }

    }

    v = vnext;

  }

  return edge;
}

void
Triangles::insert_new_vertex (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles_out)
{
  if (mp_triangles.empty ()) {

    if (m_vertex_heap.size () == 3) {

      std::vector<db::Vertex *> vv;
      for (auto v = m_vertex_heap.begin (); v != m_vertex_heap.end (); ++v) {
        vv.push_back (v.operator-> ());
      }

      //  form the first triangle
      db::TriangleEdge *s1 = create_edge (vv[0], vv[1]);
      db::TriangleEdge *s2 = create_edge (vv[1], vv[2]);
      db::TriangleEdge *s3 = create_edge (vv[2], vv[0]);

      if (db::vprod_sign (s1->d (), s2->d ()) == 0) {
        //  avoid degenerate Triangles to happen here
        tl_assert (false);
      } else {
        db::Triangle *t = create_triangle (s1, s2, s3);
        if (new_triangles_out) {
          new_triangles_out->push_back (t);
        }
      }

    }

    return;

  }

  std::vector<db::Triangle *> new_triangles;

  //  Find closest edge
  db::TriangleEdge *closest_edge = find_closest_edge (*vertex);
  tl_assert (closest_edge != 0);

  db::TriangleEdge *s1 = create_edge (vertex, closest_edge->v1 ());
  db::TriangleEdge *s2 = create_edge (vertex, closest_edge->v2 ());

  db::Triangle *t = create_triangle (s1, closest_edge, s2);
  new_triangles.push_back (t);

  add_more_triangles (new_triangles, closest_edge, closest_edge->v1 (), vertex, s1);
  add_more_triangles (new_triangles, closest_edge, closest_edge->v2 (), vertex, s2);

  if (new_triangles_out) {
    new_triangles_out->insert (new_triangles_out->end (), new_triangles.begin (), new_triangles.end ());
  }

  fix_triangles (new_triangles, std::vector<db::TriangleEdge *> (), new_triangles_out);
}

void
Triangles::add_more_triangles (std::vector<db::Triangle *> &new_triangles,
                               db::TriangleEdge *incoming_edge,
                               db::Vertex *from_vertex, db::Vertex *to_vertex,
                               db::TriangleEdge *conn_edge)
{
  while (true) {

    db::TriangleEdge *next_edge = 0;

    for (auto e = from_vertex->begin_edges (); e != from_vertex->end_edges (); ++e) {
      if (! e->has_vertex (to_vertex) && e->is_outside ()) {
        //  TODO: remove and break
        tl_assert (next_edge == 0);
        next_edge = const_cast<db::TriangleEdge *> (e.operator-> ());
      }
    }

    tl_assert (next_edge != 0);
    db::Vertex *next_vertex = next_edge->other (from_vertex);

    db::DVector d_from_to = *to_vertex - *from_vertex;
    db::Vertex *incoming_vertex = incoming_edge->other (from_vertex);
    if (db::vprod_sign(*from_vertex - *incoming_vertex, d_from_to) * db::vprod_sign(*from_vertex - *next_vertex, d_from_to) >= 0) {
      return;
    }

    db::TriangleEdge *next_conn_edge = create_edge (next_vertex, to_vertex);
    db::Triangle *t = create_triangle (next_conn_edge, next_edge, conn_edge);
    new_triangles.push_back (t);

    incoming_edge = next_edge;
    conn_edge = next_conn_edge;
    from_vertex = next_vertex;

  }
}

void
Triangles::split_triangle (db::Triangle *t, db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles_out)
{
  t->unlink ();

  std::map<db::Vertex *, TriangleEdge *> v2new_edges;
  std::vector<TriangleEdge *> new_edges;
  for (int i = 0; i < 3; ++i) {
    db::Vertex *v = t->vertex (i);
    db::TriangleEdge *e = create_edge (v, vertex);
    v2new_edges[v] = e;
    new_edges.push_back (e);
  }

  std::vector<db::Triangle *> new_triangles;
  for (int i = 0; i < 3; ++i) {
    db::TriangleEdge *e = t->edge (i);
    db::Triangle *new_triangle = create_triangle (e, v2new_edges[e->v1 ()], v2new_edges[e->v2 ()]);
    if (new_triangles_out) {
      new_triangles_out->push_back (new_triangle);
    }
    new_triangle->set_outside (t->is_outside ());
    new_triangles.push_back (new_triangle);
  }

  remove (t);

  fix_triangles (new_triangles, new_edges, new_triangles_out);
}

void
Triangles::split_triangles_on_edge (const std::vector<db::Triangle *> &tris, db::Vertex *vertex, db::TriangleEdge *split_edge, std::vector<db::Triangle *> *new_triangles_out)
{
  TriangleEdge *s1 = create_edge (split_edge->v1 (), vertex);
  TriangleEdge *s2 = create_edge (split_edge->v2 (), vertex);
  s1->set_is_segment (split_edge->is_segment ());
  s2->set_is_segment (split_edge->is_segment ());

  std::vector<db::Triangle *> new_triangles;

  for (auto t = tris.begin (); t != tris.end (); ++t) {

    (*t)->unlink ();

    db::Vertex *ext_vertex = (*t)->opposite (split_edge);
    TriangleEdge *new_edge = create_edge (ext_vertex, vertex);

    for (int i = 0; i < 3; ++i) {

      db::TriangleEdge *e = (*t)->edge (i);
      if (e->has_vertex (ext_vertex)) {

        TriangleEdge *partial = e->has_vertex (split_edge->v1 ()) ? s1 : s2;
        db::Triangle *new_triangle = create_triangle (new_edge, partial, e);

        if (new_triangles_out) {
          new_triangles_out->push_back (new_triangle);
        }
        new_triangle->set_outside ((*t)->is_outside ());
        new_triangles.push_back (new_triangle);

      }

    }

  }

  for (auto t = tris.begin (); t != tris.end (); ++t) {
    remove (*t);
  }

  std::vector<db::TriangleEdge *> fixed_edges;
  fixed_edges.push_back (s1);
  fixed_edges.push_back (s2);
  fix_triangles (new_triangles, fixed_edges, new_triangles_out);
}

std::vector<db::Vertex *>
Triangles::find_touching (const db::DBox &box) const
{
  //  NOTE: this is a naive, slow implementation for test purposes
  std::vector<db::Vertex *> res;
  for (auto v = m_vertex_heap.begin (); v != m_vertex_heap.end (); ++v) {
    if (v->begin_edges () != v->end_edges ()) {
      if (box.contains (*v)) {
        res.push_back (const_cast<db::Vertex *> (v.operator-> ()));
      }
    }
  }
  return res;
}

std::vector<db::Vertex *>
Triangles::find_inside_circle (const db::DPoint &center, double radius) const
{
  //  NOTE: this is a naive, slow implementation for test purposes
  std::vector<db::Vertex *> res;
  for (auto v = m_vertex_heap.begin (); v != m_vertex_heap.end (); ++v) {
    if (v->begin_edges () != v->end_edges ()) {
      if (v->in_circle (center, radius) == 1) {
        res.push_back (const_cast<db::Vertex *> (v.operator-> ()));
      }
    }
  }
  return res;
}

void
Triangles::remove (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles)
{
  if (vertex->is_outside ()) {
    remove_outside_vertex (vertex, new_triangles);
  } else {
    remove_inside_vertex (vertex, new_triangles);
  }
}

void
Triangles::remove_outside_vertex (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles_out)
{
  auto to_remove = vertex->triangles ();

  std::vector<db::TriangleEdge *> outer_edges;
  for (auto t = to_remove.begin (); t != to_remove.end (); ++t) {
    outer_edges.push_back ((*t)->opposite (vertex));
  }

  for (auto t = to_remove.begin (); t != to_remove.end (); ++t) {
    remove (*t);
  }

  auto new_triangles = fill_concave_corners (outer_edges);
  fix_triangles (new_triangles, std::vector<db::TriangleEdge *> (), new_triangles_out);
}

void
Triangles::remove_inside_vertex (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles_out)
{
  std::vector<db::Triangle * > triangles_to_fix;
  std::set<db::Triangle * > triangles_to_fix_set;

  bool make_new_triangle = true;

  while (vertex->num_edges () > 3) {

    db::TriangleEdge *to_flip = 0;
    for (auto e = vertex->begin_edges (); e != vertex->end_edges () && to_flip == 0; ++e) {
      if (e->can_flip ()) {
        to_flip = const_cast<db::TriangleEdge *> (e.operator-> ());
      }
    }
    if (! to_flip) {
      break;
    }

    //  NOTE: in the "can_join" case zero-area triangles are created which we will sort out later
    triangles_to_fix_set.erase (to_flip->left ());
    triangles_to_fix_set.erase (to_flip->right ());

    auto pp = flip (to_flip);
    triangles_to_fix.push_back (pp.first.first);
    triangles_to_fix_set.insert (pp.first.first);
    triangles_to_fix.push_back (pp.first.second);
    triangles_to_fix_set.insert (pp.first.second);

  }

  while (vertex->num_edges () > 3) {

    tl_assert (vertex->num_edges () == 4);

    //  This case can happen if two edges attached to the vertex are collinear
    //  in this case choose the "join" strategy
    db::TriangleEdge *jseg = 0;
    for (auto e = vertex->begin_edges (); e != vertex->end_edges () && !jseg; ++e) {
      if (e->can_join_via (vertex)) {
        jseg = const_cast <db::TriangleEdge *> (e.operator-> ());
      }
    }
    tl_assert (jseg != 0);

    db::Vertex *v1 = jseg->left ()->opposite (jseg);
    db::TriangleEdge *s1 = jseg->left ()->opposite (vertex);
    db::Vertex *v2 = jseg->right ()->opposite (jseg);
    db::TriangleEdge *s2 = jseg->right ()->opposite (vertex);

    db::TriangleEdge *jseg_opp = 0;
    for (auto e = vertex->begin_edges (); e != vertex->end_edges () && !jseg_opp; ++e) {
      if (!e->has_triangle (jseg->left ()) && !e->has_triangle (jseg->right ())) {
        jseg_opp = const_cast <db::TriangleEdge *> (e.operator-> ());
      }
    }

    db::TriangleEdge *s1opp = jseg_opp->left ()->opposite (vertex);
    db::TriangleEdge *s2opp = jseg_opp->right ()->opposite (vertex);

    db::TriangleEdge *new_edge = create_edge (v1, v2);
    db::Triangle *t1 = create_triangle (s1, s2, new_edge);
    db::Triangle *t2 = create_triangle (s1opp, s2opp, new_edge);

    triangles_to_fix.push_back (t1);
    triangles_to_fix_set.insert (t1);
    triangles_to_fix.push_back (t2);
    triangles_to_fix_set.insert (t2);

    make_new_triangle = false;

  }

  auto to_remove = vertex->triangles ();

  std::vector<db::TriangleEdge *> outer_edges;
  for (auto t = to_remove.begin (); t != to_remove.end (); ++t) {
    outer_edges.push_back ((*t)->opposite (vertex));
  }

  for (auto t = to_remove.begin (); t != to_remove.end (); ++t) {
    triangles_to_fix_set.erase (*t);
    remove (*t);
  }

  if (make_new_triangle) {

    tl_assert (outer_edges.size () == size_t (3));

    db::Triangle *nt = create_triangle (outer_edges[0], outer_edges[1], outer_edges[2]);
    triangles_to_fix.push_back (nt);
    triangles_to_fix_set.insert (nt);

  }

  std::vector<Triangle *>::iterator wp = triangles_to_fix.begin ();
  for (auto t = triangles_to_fix.begin (); t != triangles_to_fix.end (); ++t) {
    if (triangles_to_fix_set.find (*t) != triangles_to_fix_set.end ()) {
      *wp++ = *t;
      if (new_triangles_out) {
        new_triangles_out->push_back (*t);
      }
    }
  }
  triangles_to_fix.erase (wp, triangles_to_fix.end ());

  fix_triangles (triangles_to_fix, std::vector<db::TriangleEdge *> (), new_triangles_out);
}

int
Triangles::fix_triangles (const std::vector<db::Triangle *> &tris, const std::vector<db::TriangleEdge *> &fixed_edges, std::vector<db::Triangle *> *new_triangles)
{
  int flips = 0;

  m_level += 1;
  for (auto e = fixed_edges.begin (); e != fixed_edges.end (); ++e) {
    (*e)->set_level (m_level);
  }

  std::vector<db::TriangleEdge *> queue, todo;

  for (auto t = tris.begin (); t != tris.end (); ++t) {
    for (int i = 0; i < 3; ++i) {
      db::TriangleEdge *e = (*t)->edge (i);
      if (e->level () < m_level && ! e->is_segment ()) {
        queue.push_back (e);
      }
    }
  }

  while (! queue.empty ()) {

    todo.clear ();
    todo.swap (queue);
    std::set<db::TriangleEdge *> queued;

    //  NOTE: we cannot be sure that already treated edges will not become
    //  illegal by neighbor edges flipping ..
    //    for s in todo:
    //      s.level = self.level

    for (auto e = todo.begin (); e != todo.end (); ++e) {

      if (is_illegal_edge (*e)) {

        queued.erase (*e);

        auto pp = flip (*e);
        auto t1 = pp.first.first;
        auto t2 = pp.first.second;
        auto s12 = pp.second;

        if (new_triangles) {
          new_triangles->push_back (t1);
          new_triangles->push_back (t2);
        }

        ++flips;
        tl_assert (! is_illegal_edge (s12)); // @@@ remove later!

        for (int i = 0; i < 3; ++i) {
          db::TriangleEdge *s1 = t1->edge (i);
          if (s1->level () < m_level && ! s1->is_segment () && queued.find (s1) == queued.end ()) {
            queue.push_back (s1);
            queued.insert (s1);
          }
        }

        for (int i = 0; i < 3; ++i) {
          db::TriangleEdge *s2 = t2->edge (i);
          if (s2->level () < m_level && ! s2->is_segment () && queued.find (s2) == queued.end ()) {
            queue.push_back (s2);
            queued.insert (s2);
          }
        }

      }

    }

    std::vector<db::TriangleEdge *>::iterator wp = queue.begin ();
    for (auto e = queue.begin (); e != queue.end (); ++e) {
      if (queued.find (*e) != queued.end ()) {
        *wp++ = *e;
      }
    }
    queue.erase (wp, queue.end ());

  }

  return flips;
}

bool
Triangles::is_illegal_edge (db::TriangleEdge *edge)
{
  db::Triangle *left = edge->left ();
  db::Triangle *right = edge->right ();
  if (!left || !right) {
    return false;
  }

  auto lr = left->circumcircle ();
  if (right->opposite (edge)->in_circle (lr.first, lr.second) > 0) {
    return true;
  }

  auto rr = right->circumcircle();
  if (left->opposite (edge)->in_circle (rr.first, rr.second) > 0) {
    return true;
  }

  return false;
}

std::pair<std::pair<Triangle *, Triangle *>, TriangleEdge *>
Triangles::flip (TriangleEdge *edge)
{
  db::Triangle *t1 = edge->left ();
  db::Triangle *t2 = edge->right ();

  bool outside = t1->is_outside ();
  tl_assert (t1->is_outside () == outside);

  //  prepare for the new triangle to replace this one
  t1->unlink ();
  t2->unlink ();

  db::Vertex *t1_vext = t1->opposite (edge);
  db::TriangleEdge *t1_sext1 = t1->find_edge_with (t1_vext, edge->v1 ());
  db::TriangleEdge *t1_sext2 = t1->find_edge_with (t1_vext, edge->v2 ());

  db::Vertex *t2_vext = t2->opposite (edge);
  db::TriangleEdge *t2_sext1 = t2->find_edge_with (t2_vext, edge->v1 ());
  db::TriangleEdge *t2_sext2 = t2->find_edge_with (t2_vext, edge->v2 ());

  db::TriangleEdge *s_new = create_edge (t1_vext, t2_vext);

  db::Triangle *t1_new = create_triangle (s_new, t1_sext1, t2_sext1);
  t1_new->set_outside (outside);
  db::Triangle *t2_new = create_triangle (s_new, t1_sext2, t2_sext2);
  t2_new->set_outside (outside);

  remove (t1);
  remove (t2);

  return std::make_pair (std::make_pair (t1_new, t2_new), s_new);
}

std::vector<db::Triangle *>
Triangles::fill_concave_corners (const std::vector<db::TriangleEdge *> &edges)
{
  std::vector<db::Triangle *> res;
  std::vector<db::Vertex *> points, terminals;

  std::map<db::Vertex *, std::vector<db::TriangleEdge *> > vertex2edge;
  for (auto e = edges.begin (); e != edges.end (); ++e) {

    auto i = vertex2edge.insert (std::make_pair ((*e)->v1 (), std::vector<db::TriangleEdge *> ()));
    if (i.second) {
      points.push_back ((*e)->v1 ());
    }
    i.first->second.push_back (*e);

    i = vertex2edge.insert (std::make_pair ((*e)->v2 (), std::vector<db::TriangleEdge *> ()));
    if (i.second) {
      points.push_back ((*e)->v2 ());
    }
    i.first->second.push_back (*e);

  }

  while (points.size () > size_t (2)) {

    terminals.clear ();
    for (auto p = points.begin (); p != points.end (); ++p) {
      if (vertex2edge [*p].size () == 1) {
        terminals.push_back (*p);
      }
    }
    tl_assert (terminals.size () == size_t (2));
    db::Vertex *v = terminals[0];

    bool any_connected = false;
    db::Vertex *vp = 0;

    std::set<db::Vertex *> to_remove;

    while (vertex2edge [v].size () >= size_t (2) || ! vp) {

      db::TriangleEdge *seg = 0;
      std::vector<db::TriangleEdge *> &ee = vertex2edge [v];
      for (auto e = ee.begin (); e != ee.end (); ++e) {
        if (! (*e)->has_vertex (vp)) {
          seg = (*e);
          break;
        }
      }

      tl_assert (seg != 0);
      db::Triangle *tri = seg->left () ? seg->left () : seg->right ();
      db::Vertex *vn = seg->other (v);

      std::vector<db::TriangleEdge *> &een = vertex2edge [vn];
      if (een.size () < size_t (2)) {
        break;
      }
      tl_assert (een.size () == size_t (2));

      db::TriangleEdge *segn = 0;
      for (auto e = een.begin (); e != een.end (); ++e) {
        if (! (*e)->has_vertex (v)) {
          segn = (*e);
          break;
        }
      }

      tl_assert (segn != 0);
      db::Vertex *vnn = segn->other (vn);
      std::vector<db::TriangleEdge *> &eenn = vertex2edge [vnn];

      //  NOTE: tri can be None in case a lonely edge stays after removing
      //  attached triangles
      if (! tri || seg->side_of (*vnn) * seg->side_of (*tri->opposite (seg)) < 0) {

        //  is concave
        db::TriangleEdge *new_edge = create_edge (v, vnn);
        for (auto e = ee.begin (); e != ee.end (); ++e) {
          if (*e == seg) {
            ee.erase (e);
            break;
          }
        }
        ee.push_back (new_edge);

        for (auto e = eenn.begin (); e != eenn.end (); ++e) {
          if (*e == segn) {
            eenn.erase (e);
            break;
          }
        }
        eenn.push_back (new_edge);

        vertex2edge.erase (vn);
        to_remove.insert (vn);

        db::Triangle *new_triangle = create_triangle (seg, segn, new_edge);
        res.push_back (new_triangle);
        any_connected = true;

      } else {

        vp = v;
        v = vn;

      }

    }

    if (not any_connected) {
      break;
    }

    std::vector<db::Vertex *>::iterator wp = points.begin ();
    for (auto p = points.begin (); p != points.end (); ++p) {
      if (to_remove.find (*p) == to_remove.end ()) {
        *wp++ = *p;
      }
    }
    points.erase (wp, points.end ());

  }

  return res;
}

}

#if 0
import math
import sys
from .triangle import *

class Triangles(object):

  def find_edge_for_points(self, p1: Point, p2: Point) -> TriangleEdge:

    v = self.find_vertex_for_point(p1)
    if v is None:
      return None
    for s in v.edges:
      if equals(s.other_vertex(v), p2):
        return s
    return None

  def search_edges_crossing(self, edge: TriangleEdge) -> set:
    """
    Finds all edges that cross the given one for a convex triangulation

    Requirements:
    * self must be a convex triangulation
    * edge must not contain another vertex from the triangulation except p1 and p2
    """

    v = edge.p1
    vv = edge.p2

    current_triangle = None
    next_edge = None

    result = []

    for s in v.edges:
      for t in [s.left, s.right]:
        if t is not None:
          os = t.opposite_edge(v)
          if os.has_vertex(vv):
            return result
          if os.crosses(edge):
            result.append(os)
            current_triangle = t
            next_edge = os
            break
      if next_edge is not None:
        break

    assert (current_triangle is not None)

    while True:

      current_triangle = next_edge.other(current_triangle)

      # Note that we're convex, so there has to be a path across triangles
      assert (current_triangle is not None)

      cs = next_edge
      next_edge = None
      for s in current_triangle.edges():
        if s != cs:
          if s.has_vertex(vv):
            return result
          if s.crosses(edge):
            result.append(s)
            next_edge = s
            break

      assert (next_edge is not None)

  def _ensure_edge_inner(self, edge: Edge) -> [TriangleEdge]:

    crossed_edges = self.search_edges_crossing(edge)

    if len(crossed_edges) == 0:

      # no crossing edge - there should be a edge already
      result = self.find_edge_for_points(edge.p1, edge.p2)
      assert (result is not None)
      result = [result]

    elif len(crossed_edges) == 1:

      # can be solved by flipping
      _, _, result = self.flip(crossed_edges[0])
      assert (result.has_vertex(edge.p1) and result.has_vertex(edge.p2))
      result = [result]

    else:

      # split edge close to center
      split_point = None
      d = None
      l_half = 0.25 * square(edge.d())
      for s in crossed_edges:
        p = s.intersection_point(edge)
        dp = abs(square(sub(p, edge.p1)) - l_half)
        if d is None or dp < d:
          dp = d
          split_point = p

      split_vertex = self.insert(Vertex(split_point.x, split_point.y))

      e1 = Edge(edge.p1, split_vertex)
      e2 = Edge(split_vertex, edge.p2)

      result = self._ensure_edge_inner(e1) + self._ensure_edge_inner(e2)

    return result

  def ensure_edge(self, edge: Edge) -> [TriangleEdge]:

    # NOTE: this should not be required if the original outer edges are non-overlapping
    # TODO: this is inefficient
    for v in self.vertexes:
      if edge.point_on(v):
        return self.ensure_edge(Edge(edge.p1, v)) + self.ensure_edge(Edge(v, edge.p2))

    edges = self._ensure_edge_inner(edge)
    for s in edges:
      # mark the edges as fixed "forever" so we don't modify them when we ensure other edges
      s.level = sys.maxsize
    return edges

  def _join_edges(self, edges) -> [TriangleEdge]:

    # edges are supposed to be ordered
    final_edges = []
    i = 1
    while i < len(edges):
      s1 = edges[i - 1]
      s2 = edges[i]
      assert(s1.is_segment == s2.is_segment)
      cp = s1.common_vertex(s2)
      assert (cp is not None)
      join_edges = []
      for s in cp.edges:
        if s != s1 and s != s2:
          if s.can_join_via(cp):
            join_edges.append(s)
          else:
            join_edges = []
            break
      if len(join_edges) > 0:
        assert(len(join_edges) <= 2)
        new_edge = TriangleEdge(s1.other_vertex(cp), s2.other_vertex(cp))
        new_edge.is_segment = s1.is_segment
        for js in join_edges:
          t1 = js.left
          t2 = js.right
          tedge1 = t1.opposite_edge(cp)
          tedge2 = t2.opposite_edge(cp)
          t1.unlink()
          self.triangles.remove(t1)
          t2.unlink()
          self.triangles.remove(t2)
          tri = Triangle(tedge1, tedge2, new_edge)
          tri.is_outside = t1.is_outside
          self.triangles.append(tri)
          js.unlink()
        self.vertexes.remove(cp)
        s1.unlink()
        s2.unlink()
        edges[i - 1] = new_edge
        del edges[i]
      else:
        i += 1


  def constrain(self, contours: [[Edge]]) -> object:

    """
    Given a set of contours with edges, mark outer triangles

    The edges must be made from existing vertexes. Edge orientation is
    clockwise.
    """

    assert(not self.is_constrained)

    resolved_edges = []

    for c in contours:
      for edge in c:
        edges = self.ensure_edge(edge)
        resolved_edges.append( (edge, edges) )

    for tri in self.triangles:
      tri.is_outside = False
      for s in tri.edges():
        s.is_segment = False

    new_tri = set()

    for re in resolved_edges:
      edge, edges = re
      for s in edges:
        s.is_segment = True
        outer_tri = None
        d = sprod_sign(edge.d(), s.d())
        if d > 0:
          outer_tri = s.left
        if d < 0:
          outer_tri = s.right
        if outer_tri is not None:
          assert (outer_tri in self.triangles)
          new_tri.add(outer_tri)
          outer_tri.is_outside = True

    while len(new_tri) > 0:

      next_tris = set()

      for tri in new_tri:
        for s in tri.edges():
          if not s.is_segment:
            ot = s.other(tri)
            if ot is not None and not ot.is_outside:
              next_tris.add(ot)
              ot.is_outside = True

      new_tri = next_tris

    # join edges where possible
    for re in resolved_edges:
      _, edges = re
      self._join_edges(edges)

    self.is_constrained = True

  def remove_outside_triangles(self):

    assert self.is_constrained

    for tri in self.triangles:
      for s in tri.edges():
        s.level = 0

    to_remove = [tri for tri in self.triangles if tri.is_outside]
    for tri in to_remove:
      for s in tri.edges():
        if not s.is_segment and s.level == 0:
          s.level = 1
          s.unlink()
      tri.unlink()
      self.triangles.remove(tri)

    to_remove = [v for v in self.vertexes if len(v.edges) == 0]
    for v in to_remove:
      self.vertexes.remove(v)

  def create_constrained_delaunay(self, contours: [[Edge]]):

    edge_contours = []

    for c in contours:

      if len(c) < 3:
        continue

      edges = []
      edge_contours.append(edges)

      vl = None
      vfirst = None
      for pt in c:
        v = self.insert(Vertex(pt.x, pt.y))
        if vfirst is None:
          vfirst = v
        else:
          edges.append(Edge(vl, v))
        vl = v
      edges.append(Edge(vl, vfirst))

    self.constrain(edge_contours)


#endif


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


#include "dbTriangles.h"
#include "dbLayout.h"
#include "dbWriter.h"
#include "tlStream.h"
#include "tlLog.h"
#include "tlTimer.h"

#include <set>
#include <memory>
#include <vector>
#include <map>

namespace db
{

Triangles::Triangles ()
  : m_is_constrained (false), m_level (0), m_id (0), m_flips (0), m_hops (0)
{
  //  .. nothing yet ..
}

Triangles::~Triangles ()
{
  while (! mp_triangles.empty ()) {
    remove_triangle (mp_triangles.begin ().operator-> ());
  }
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
  db::TriangleEdge *edge = 0;

  if (! m_returned_edges.empty ()) {
    edge = m_returned_edges.back ();
    m_returned_edges.pop_back ();
    *edge = db::TriangleEdge (v1, v2);
  } else {
    m_edges_heap.push_back (db::TriangleEdge (v1, v2));
    edge = &m_edges_heap.back ();
  }

  edge->link ();
  edge->set_id (++m_id);
  return edge;
}

db::Triangle *
Triangles::create_triangle (TriangleEdge *e1, TriangleEdge *e2, TriangleEdge *e3)
{
  db::Triangle *res = new db::Triangle (e1, e2, e3);
  res->set_id (++m_id);
  mp_triangles.push_back (res);
  return res;
}

void
Triangles::remove_triangle (db::Triangle *tri)
{
  db::TriangleEdge *edges [3];
  for (int i = 0; i < 3; ++i) {
    edges [i] = tri->edge (i);
  }

  delete tri;

  //  clean up edges we do no longer need
  for (int i = 0; i < 3; ++i) {
    db::TriangleEdge *e = edges [i];
    if (e && e->left () == 0 && e->right () == 0 && e->v1 ()) {
      e->unlink ();
      m_returned_edges.push_back (e);
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
    box += t->bbox ();
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

  for (auto e = m_edges_heap.begin (); e != m_edges_heap.end (); ++e) {

    if (!e->left () && !e->right ()) {
      continue;
    }

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
      if ((*e)->is_outside ()) {
        ++num_outside_edges;
      }
    }
    if (num_outside_edges > 0 && num_outside_edges != 2) {
      tl::error << "(check error) vertex " << v->to_string (true) << " has " << num_outside_edges << " outside edges (can only be 2)";
      res = false;
      for (auto e = v->begin_edges (); e != v->end_edges (); ++e) {
        if ((*e)->is_outside ()) {
          tl::error << "  Outside edge is " << (*e)->to_string (true);
        }
      }
    }
  }

  return res;
}

db::Layout *
Triangles::to_layout (bool decompose_by_id) const
{
  db::Layout *layout = new db::Layout ();
  layout->dbu (0.001);

  auto dbu_trans = db::CplxTrans (layout->dbu ()).inverted ();

  db::Cell &top = layout->cell (layout->add_cell ("DUMP"));
  unsigned int l1 = layout->insert_layer (db::LayerProperties (1, 0));
  unsigned int l2 = layout->insert_layer (db::LayerProperties (2, 0));
  unsigned int l10 = layout->insert_layer (db::LayerProperties (10, 0));
  unsigned int l20 = layout->insert_layer (db::LayerProperties (20, 0));
  unsigned int l21 = layout->insert_layer (db::LayerProperties (21, 0));
  unsigned int l22 = layout->insert_layer (db::LayerProperties (22, 0));

  for (auto t = mp_triangles.begin (); t != mp_triangles.end (); ++t) {
    db::DPoint pts[3];
    for (int i = 0; i < 3; ++i) {
      pts[i] = *t->vertex (i);
    }
    db::DPolygon poly;
    poly.assign_hull (pts + 0, pts + 3);
    top.shapes (t->is_outside () ? l2 : l1).insert (dbu_trans * poly);
    if (decompose_by_id) {
      if ((t->id () & 1) != 0) {
        top.shapes (l20).insert (dbu_trans * poly);
      }
      if ((t->id () & 2) != 0) {
        top.shapes (l21).insert (dbu_trans * poly);
      }
      if ((t->id () & 4) != 0) {
        top.shapes (l22).insert (dbu_trans * poly);
      }
    }
  }

  for (auto e = m_edges_heap.begin (); e != m_edges_heap.end (); ++e) {
    if ((e->left () || e->right ()) && e->is_segment ()) {
      top.shapes (l10).insert (dbu_trans * e->edge ());
    }
  }

  return layout;
}

void
Triangles::dump (const std::string &path, bool decompose_by_id) const
{
  std::unique_ptr<db::Layout> ly (to_layout (decompose_by_id));

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
        db::Vertex *ov = (*e)->other (*v);
        if (ov->in_circle (*vertex, radius) == 1 && seen.insert (ov).second) {
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
Triangles::insert_point (const db::DPoint &point, std::list<tl::weak_ptr<db::Triangle> > *new_triangles)
{
  return insert (create_vertex (point), new_triangles);
}

db::Vertex *
Triangles::insert_point (db::DCoord x, db::DCoord y, std::list<tl::weak_ptr<db::Triangle> > *new_triangles)
{
  return insert (create_vertex (x, y), new_triangles);
}

db::Vertex *
Triangles::insert (db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles)
{
  std::vector<db::Triangle *> tris = find_triangle_for_point (*vertex);

  //  the new vertex is outside the domain
  if (tris.empty ()) {
    tl_assert (! m_is_constrained);
    insert_new_vertex (vertex, new_triangles);
    return vertex;
  }

  //  check, if the new vertex is on an edge (may be edge between triangles or edge on outside)
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

      unsigned int ls = 0;
      size_t n = m_vertex_heap.size ();
      size_t m = n;

      //  A simple heuristics that takes a sqrt(N) sample from the
      //  vertexes to find a good starting point

      vstart = mp_triangles.begin ()->vertex (0);
      double dmin = vstart->distance (p);

      while (ls * ls < m) {
        m /= 2;
        for (size_t i = m / 2; i < n; i += m) {
          ++ls;
          //  NOTE: this assumes the heap is not too loaded with orphan vertexes
          db::Vertex *v = (m_vertex_heap.begin () + i).operator-> ();
          if (v->begin_edges () != v->end_edges ()) {
            double d = v->distance (p);
            if (d < dmin) {
              vstart = v;
              dmin = d;
            }
          }
        }
      }

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
        if (! (*e)->is_segment () && (*e)->is_for_outside_triangles ()) {
          continue;
        }
        if (! (*e)->crosses_including (line)) {
          continue;
        }
      }

      double ds = (*e)->distance (p);

      if (d < 0.0) {

        d = ds;
        edge = *e;
        vnext = edge->other (v);

      } else if (fabs (ds - d) < std::max (1.0, fabs (ds) + fabs (d)) * db::epsilon) {

        //  this differentiation selects the edge which bends further towards
        //  the target point if both edges share a common point and that
        //  is the one the determines the distance.
        db::Vertex *cv = edge->common_vertex (*e);
        if (cv) {
          db::DVector edge_d = *edge->other (cv) - *cv;
          db::DVector e_d = *(*e)->other(cv) - *cv;
          db::DVector r = p - *cv;
          double edge_sp = db::sprod (r, edge_d) / edge_d.length ();
          double s_sp = db::sprod (r, e_d) / e_d.length ();
          if (s_sp > edge_sp + db::epsilon) {
            edge = *e;
            vnext = edge->other (v);
          }
        }

      } else if (ds < d) {

        d = ds;
        edge = *e;
        vnext = edge->other (v);

      }

    }

    ++m_hops;

    v = vnext;

  }

  return edge;
}

void
Triangles::insert_new_vertex (db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles_out)
{
  if (mp_triangles.empty ()) {

    tl_assert (m_vertex_heap.size () <= size_t (3));  //  fails if vertexes were created but not inserted.

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
      if (! (*e)->has_vertex (to_vertex) && (*e)->is_outside ()) {
        //  TODO: remove and break
        tl_assert (next_edge == 0);
        next_edge = *e;
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
Triangles::split_triangle (db::Triangle *t, db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles_out)
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

  remove_triangle (t);

  fix_triangles (new_triangles, new_edges, new_triangles_out);
}

void
Triangles::split_triangles_on_edge (const std::vector<db::Triangle *> &tris, db::Vertex *vertex, db::TriangleEdge *split_edge, std::list<tl::weak_ptr<db::Triangle> > *new_triangles_out)
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
    remove_triangle (*t);
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
Triangles::remove (db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles)
{
  if (vertex->begin_edges () == vertex->end_edges ()) {
    //  removing an orphan vertex -> ignore
  } else if (vertex->is_outside ()) {
    remove_outside_vertex (vertex, new_triangles);
  } else {
    remove_inside_vertex (vertex, new_triangles);
  }
}

void
Triangles::remove_outside_vertex (db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles_out)
{
  auto to_remove = vertex->triangles ();

  std::vector<db::TriangleEdge *> outer_edges;
  for (auto t = to_remove.begin (); t != to_remove.end (); ++t) {
    outer_edges.push_back ((*t)->opposite (vertex));
  }

  for (auto t = to_remove.begin (); t != to_remove.end (); ++t) {
    (*t)->unlink ();
  }

  auto new_triangles = fill_concave_corners (outer_edges);

  for (auto t = to_remove.begin (); t != to_remove.end (); ++t) {
    remove_triangle (*t);
  }

  fix_triangles (new_triangles, std::vector<db::TriangleEdge *> (), new_triangles_out);
}

void
Triangles::remove_inside_vertex (db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles_out)
{
  std::set<db::Triangle *, TriangleLessFunc> triangles_to_fix;

  bool make_new_triangle = true;

  while (vertex->num_edges (4) > 3) {

    db::TriangleEdge *to_flip = 0;
    for (auto e = vertex->begin_edges (); e != vertex->end_edges () && to_flip == 0; ++e) {
      if ((*e)->can_flip ()) {
        to_flip = *e;
      }
    }
    if (! to_flip) {
      break;
    }

    //  NOTE: in the "can_join" case zero-area triangles are created which we will sort out later
    triangles_to_fix.erase (to_flip->left ());
    triangles_to_fix.erase (to_flip->right ());

    auto pp = flip (to_flip);
    triangles_to_fix.insert (pp.first.first);
    triangles_to_fix.insert (pp.first.second);

  }

  if (vertex->num_edges (4) > 3) {

    tl_assert (vertex->num_edges (5) == 4);

    //  This case can happen if two edges attached to the vertex are collinear
    //  in this case choose the "join" strategy
    db::TriangleEdge *jseg = 0;
    for (auto e = vertex->begin_edges (); e != vertex->end_edges () && !jseg; ++e) {
      if ((*e)->can_join_via (vertex)) {
        jseg = *e;
      }
    }
    tl_assert (jseg != 0);

    db::Vertex *v1 = jseg->left ()->opposite (jseg);
    db::TriangleEdge *s1 = jseg->left ()->opposite (vertex);
    db::Vertex *v2 = jseg->right ()->opposite (jseg);
    db::TriangleEdge *s2 = jseg->right ()->opposite (vertex);

    db::TriangleEdge *jseg_opp = 0;
    for (auto e = vertex->begin_edges (); e != vertex->end_edges () && !jseg_opp; ++e) {
      if (!(*e)->has_triangle (jseg->left ()) && !(*e)->has_triangle (jseg->right ())) {
        jseg_opp = *e;
      }
    }

    db::TriangleEdge *s1opp = jseg_opp->left ()->opposite (vertex);
    db::TriangleEdge *s2opp = jseg_opp->right ()->opposite (vertex);

    db::TriangleEdge *new_edge = create_edge (v1, v2);
    db::Triangle *t1 = create_triangle (s1, s2, new_edge);
    db::Triangle *t2 = create_triangle (s1opp, s2opp, new_edge);

    triangles_to_fix.insert (t1);
    triangles_to_fix.insert (t2);

    make_new_triangle = false;

  }

  auto to_remove = vertex->triangles ();

  std::vector<db::TriangleEdge *> outer_edges;
  for (auto t = to_remove.begin (); t != to_remove.end (); ++t) {
    outer_edges.push_back ((*t)->opposite (vertex));
  }

  if (make_new_triangle) {

    tl_assert (outer_edges.size () == size_t (3));

    db::Triangle *nt = create_triangle (outer_edges[0], outer_edges[1], outer_edges[2]);
    triangles_to_fix.insert (nt);

  }

  for (auto t = to_remove.begin (); t != to_remove.end (); ++t) {
    triangles_to_fix.erase (*t);
    remove_triangle (*t);
  }

  if (new_triangles_out) {
    for (auto t = triangles_to_fix.begin (); t != triangles_to_fix.end (); ++t) {
      new_triangles_out->push_back (*t);
    }
  }

  std::vector<db::Triangle *> to_fix_a (triangles_to_fix.begin (), triangles_to_fix.end ());
  fix_triangles (to_fix_a, std::vector<db::TriangleEdge *> (), new_triangles_out);
}

void
Triangles::fix_triangles (const std::vector<db::Triangle *> &tris, const std::vector<db::TriangleEdge *> &fixed_edges, std::list<tl::weak_ptr<db::Triangle> > *new_triangles)
{
  m_level += 1;
  for (auto e = fixed_edges.begin (); e != fixed_edges.end (); ++e) {
    (*e)->set_level (m_level);
  }

  std::set<db::TriangleEdge *, TriangleEdgeLessFunc> queue, todo;

  for (auto t = tris.begin (); t != tris.end (); ++t) {
    for (int i = 0; i < 3; ++i) {
      db::TriangleEdge *e = (*t)->edge (i);
      if (e->level () < m_level && ! e->is_segment ()) {
        queue.insert (e);
      }
    }
  }

  while (! queue.empty ()) {

    todo.clear ();
    todo.swap (queue);

    //  NOTE: we cannot be sure that already treated edges will not become
    //  illegal by neighbor edges flipping ..
    //    for s in todo:
    //      s.level = self.level

    for (auto e = todo.begin (); e != todo.end (); ++e) {

      if (is_illegal_edge (*e)) {

        queue.erase (*e);

        auto pp = flip (*e);
        auto t1 = pp.first.first;
        auto t2 = pp.first.second;
        auto s12 = pp.second;

        if (new_triangles) {
          new_triangles->push_back (t1);
          new_triangles->push_back (t2);
        }

        ++m_flips;
        tl_assert (! is_illegal_edge (s12)); // TODO: remove later!

        for (int i = 0; i < 3; ++i) {
          db::TriangleEdge *s1 = t1->edge (i);
          if (s1->level () < m_level && ! s1->is_segment ()) {
            queue.insert (s1);
          }
        }

        for (int i = 0; i < 3; ++i) {
          db::TriangleEdge *s2 = t2->edge (i);
          if (s2->level () < m_level && ! s2->is_segment ()) {
            queue.insert (s2);
          }
        }

      }

    }

  }
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

  remove_triangle (t1);
  remove_triangle (t2);

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

    if (! any_connected) {
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

std::vector<db::TriangleEdge *>
Triangles::search_edges_crossing (Vertex *from, Vertex *to)
{
  db::Vertex *v = from;
  db::Vertex *vv = to;
  db::DEdge edge (*from, *to);

  db::Triangle *current_triangle = 0;
  db::TriangleEdge *next_edge = 0;

  std::vector<db::TriangleEdge *> result;

  for (auto e = v->begin_edges (); e != v->end_edges () && ! next_edge; ++e) {
    for (auto t = (*e)->begin_triangles (); t != (*e)->end_triangles (); ++t) {
      db::TriangleEdge *os = t->opposite (v);
      if (os->has_vertex (vv)) {
        return result;
      }
      if (os->crosses (edge)) {
        result.push_back (os);
        current_triangle = t.operator-> ();
        next_edge = os;
        break;
      }
    }
  }

  tl_assert (current_triangle != 0);

  while (true) {

    current_triangle = next_edge->other (current_triangle);

    //  Note that we're convex, so there has to be a path across triangles
    tl_assert (current_triangle != 0);

    db::TriangleEdge *cs = next_edge;
    next_edge = 0;
    for (int i = 0; i < 3; ++i) {
      db::TriangleEdge *e = current_triangle->edge (i);
      if (e != cs) {
        if (e->has_vertex (vv)) {
          return result;
        }
        if (e->crosses (edge)) {
          result.push_back (e);
          next_edge = e;
          break;
        }
      }
    }

    tl_assert (next_edge != 0);

  }
}

db::Vertex *
Triangles::find_vertex_for_point (const db::DPoint &pt)
{
  db::TriangleEdge *edge = find_closest_edge (pt);
  if (!edge) {
    return 0;
  }
  db::Vertex *v = 0;
  if (edge->v1 ()->equal (pt)) {
    v = edge->v1 ();
  } else if (edge->v2 ()->equal (pt)) {
    v = edge->v2 ();
  }
  return v;
}

db::TriangleEdge *
Triangles::find_edge_for_points (const db::DPoint &p1, const db::DPoint &p2)
{
  db::Vertex *v = find_vertex_for_point (p1);
  if (!v) {
    return 0;
  }
  for (auto e = v->begin_edges (); e != v->end_edges (); ++e) {
    if ((*e)->other (v)->equal (p2)) {
      return *e;
    }
  }
  return 0;
}

std::vector<db::TriangleEdge *>
Triangles::ensure_edge_inner (db::Vertex *from, db::Vertex *to)
{
  auto crossed_edges = search_edges_crossing (from, to);
  std::vector<db::TriangleEdge *> result;

  if (crossed_edges.empty ()) {

    //  no crossing edge - there should be a edge already
    db::TriangleEdge *res = find_edge_for_points (*from, *to);
    tl_assert (res != 0);
    result.push_back (res);

  } else if (crossed_edges.size () == 1) {

    //  can be solved by flipping
    auto pp = flip (crossed_edges.front ());
    db::TriangleEdge *res = pp.second;
    tl_assert (res->has_vertex (from) && res->has_vertex (to));
    result.push_back (res);

  } else {

    //  split edge close to center
    db::DPoint split_point;
    double d = -1.0;
    double l_half = 0.25 * (*to - *from).sq_length ();
    for (auto e = crossed_edges.begin (); e != crossed_edges.end (); ++e) {
      db::DPoint p = (*e)->intersection_point (db::DEdge (*from, *to));
      double dp = fabs ((p - *from).sq_length () - l_half);
      if (d < 0.0 || dp < d) {
        dp = d;
        split_point = p;
      }
    }

    db::Vertex *split_vertex = insert_point (split_point);

    result = ensure_edge_inner (from, split_vertex);

    auto result2 = ensure_edge_inner (split_vertex, to);
    result.insert (result.end (), result2.begin (), result2.end ());

  }

  return result;
}

std::vector<db::TriangleEdge *>
Triangles::ensure_edge (db::Vertex *from, db::Vertex *to)
{
#if 0
  //  NOTE: this should not be required if the original segments are non-overlapping
  //  TODO: this is inefficient
  for v in self.vertexes:
    if edge.point_on(v):
      return self.ensure_edge(Edge(edge.p1, v)) + self.ensure_edge(Edge(v, edge.p2))
#endif

  auto edges = ensure_edge_inner (from, to);
  for (auto e = edges.begin (); e != edges.end (); ++e) {
    //  mark the edges as fixed "forever" so we don't modify them when we ensure other edges
    (*e)->set_level (std::numeric_limits<size_t>::max ());
  }
  return edges;
}

void
Triangles::join_edges (std::vector<db::TriangleEdge *> &edges)
{
  //  edges are supposed to be ordered
  for (size_t i = 1; i < edges.size (); ) {

    db::TriangleEdge *s1 = edges [i - 1];
    db::TriangleEdge *s2 = edges [i];
    tl_assert (s1->is_segment () == s2->is_segment ());
    db::Vertex *cp = s1->common_vertex (s2);
    tl_assert (cp != 0);

    std::vector<db::TriangleEdge *> join_edges;
    for (auto e = cp->begin_edges (); e != cp->end_edges (); ++e) {
      if (*e != s1 && *e != s2) {
        if ((*e)->can_join_via (cp)) {
          join_edges.push_back (*e);
        } else {
          join_edges.clear ();
          break;
        }
      }
    }

    if (! join_edges.empty ()) {

      tl_assert (join_edges.size () <= 2);

      TriangleEdge *new_edge = create_edge (s1->other (cp), s2->other (cp));
      new_edge->set_is_segment (s1->is_segment ());

      for (auto js = join_edges.begin (); js != join_edges.end (); ++js) {

        db::Triangle *t1 = (*js)->left ();
        db::Triangle *t2 = (*js)->right ();
        db::TriangleEdge *tedge1 = t1->opposite (cp);
        db::TriangleEdge *tedge2 = t2->opposite (cp);
        t1->unlink ();
        t2->unlink ();
        db::Triangle *tri = create_triangle (tedge1, tedge2, new_edge);
        tri->set_outside (t1->is_outside ());
        remove_triangle (t1);
        remove_triangle (t2);
      }

      edges [i - 1] = new_edge;
      edges.erase (edges.begin () + i);

    } else {
      ++i;
    }

  }
}

void
Triangles::constrain (const std::vector<std::vector<db::Vertex *> > &contours)
{
  tl_assert (! m_is_constrained);

  std::vector<std::pair<db::DEdge, std::vector<db::TriangleEdge *> > > resolved_edges;

  for (auto c = contours.begin (); c != contours.end (); ++c) {
    for (auto v = c->begin (); v != c->end (); ++v) {
      auto vv = v;
      ++vv;
      if (vv == c->end ()) {
        vv = c->begin ();
      }
      resolved_edges.push_back (std::make_pair (db::DEdge (**v, **vv), std::vector<db::TriangleEdge *> ()));
      resolved_edges.back ().second = ensure_edge (*v, *vv);
    }
  }

  for (auto tri = mp_triangles.begin (); tri != mp_triangles.end (); ++tri) {
    tri->set_outside (false);
    for (int i = 0; i < 3; ++i) {
      tri->edge (i)->set_is_segment (false);
    }
  }

  std::set<db::Triangle *, TriangleLessFunc> new_tri;

  for (auto re = resolved_edges.begin (); re != resolved_edges.end (); ++re) {
    auto edge = re->first;
    auto edges = re->second;
    for (auto e = edges.begin (); e != edges.end (); ++e) {
      (*e)->set_is_segment (true);
      db::Triangle *outer_tri = 0;
      int d = db::sprod_sign (edge.d (), (*e)->d ());
      if (d > 0) {
        outer_tri = (*e)->left ();
      }
      if (d < 0) {
        outer_tri = (*e)->right ();
      }
      if (outer_tri) {
        new_tri.insert (outer_tri);
        outer_tri->set_outside (true);
      }
    }
  }

  while (! new_tri.empty ()) {

    std::set<db::Triangle *, TriangleLessFunc> next_tris;

    for (auto tri = new_tri.begin (); tri != new_tri.end (); ++tri) {
      for (int i = 0; i < 3; ++i) {
        auto e = (*tri)->edge (i);
        if (! e->is_segment ()) {
          auto ot = e->other (*tri);
          if (ot && ! ot->is_outside ()) {
            next_tris.insert (ot);
            ot->set_outside (true);
          }
        }
      }
    }

    new_tri.swap (next_tris);

  }

  //  join edges where possible
  for (auto re = resolved_edges.begin (); re != resolved_edges.end (); ++re) {
    auto edges = re->second;
    join_edges (edges);
  }

  m_is_constrained = true;
}

void
Triangles::remove_outside_triangles ()
{
  tl_assert (m_is_constrained);

  //  NOTE: don't remove while iterating
  std::vector<db::Triangle *> to_remove;
  for (auto tri = begin (); tri != end (); ++tri) {
    if (tri->is_outside ()) {
      to_remove.push_back (const_cast<db::Triangle *> (tri.operator-> ()));
    }
  }

  for (auto t = to_remove.begin (); t != to_remove.end (); ++t) {
    remove_triangle (*t);
  }
}

void
Triangles::clear ()
{
  mp_triangles.clear ();
  m_edges_heap.clear ();
  m_vertex_heap.clear ();
  m_returned_edges.clear ();
  m_is_constrained = false;
  m_level = 0;
  m_id = 0;
}

template<class Poly, class Trans>
void
Triangles::make_contours (const Poly &poly, const Trans &trans, std::vector<std::vector<db::Vertex *> > &edge_contours)
{
  edge_contours.push_back (std::vector<db::Vertex *> ());
  for (auto pt = poly.begin_hull (); pt != poly.end_hull (); ++pt) {
    edge_contours.back ().push_back (insert_point (trans * *pt));
  }

  for (unsigned int h = 0; h < poly.holes (); ++h) {
    edge_contours.push_back (std::vector<db::Vertex *> ());
    for (auto pt = poly.begin_hole (h); pt != poly.end_hole (h); ++pt) {
      edge_contours.back ().push_back (insert_point (trans * *pt));
    }
  }
}

void
Triangles::create_constrained_delaunay (const db::Region &region, const CplxTrans &trans)
{
  clear ();

  std::vector<std::vector<db::Vertex *> > edge_contours;

  for (auto p = region.begin_merged (); ! p.at_end (); ++p) {
    make_contours (*p, trans, edge_contours);
  }

  constrain (edge_contours);
}

void
Triangles::create_constrained_delaunay (const db::Polygon &p, const CplxTrans &trans)
{
  clear ();

  std::vector<std::vector<db::Vertex *> > edge_contours;
  make_contours (p, trans, edge_contours);

  constrain (edge_contours);
}

void
Triangles::create_constrained_delaunay (const db::DPolygon &p)
{
  clear ();

  std::vector<std::vector<db::Vertex *> > edge_contours;
  make_contours (p, db::DUnitTrans (), edge_contours);

  constrain (edge_contours);
}

static bool is_skinny (const db::Triangle *tri, const Triangles::TriangulateParameters &param)
{
  if (param.min_b < db::epsilon) {
    return false;
  } else {
    double b = tri->b ();
    double delta = (b + param.min_b) * db::epsilon;
    return b < param.min_b - delta;
  }
}

static bool is_invalid (const db::Triangle *tri, const Triangles::TriangulateParameters &param)
{
  if (is_skinny (tri, param)) {
    return true;
  }

  double amax = param.max_area;
  if (param.max_area_border > db::epsilon) {
    if (tri->has_segment ()) {
      amax = param.max_area_border;
    }
  }

  if (amax > db::epsilon) {
    double a = tri->area ();
    double delta = (a + amax) * db::epsilon;
    return tri->area () > amax + delta;
  } else {
    return false;
  }
}

void
Triangles::triangulate (const db::Region &region, const TriangulateParameters &parameters, double dbu)
{
  tl::SelfTimer timer (tl::verbosity () > parameters.base_verbosity, "Triangles::triangulate");

  create_constrained_delaunay (region, db::CplxTrans (dbu));
  refine (parameters);
}

void
Triangles::triangulate (const db::Region &region, const TriangulateParameters &parameters, const db::CplxTrans &trans)
{
  tl::SelfTimer timer (tl::verbosity () > parameters.base_verbosity, "Triangles::triangulate");

  create_constrained_delaunay (region, trans);
  refine (parameters);
}

void
Triangles::triangulate (const db::Polygon &poly, const TriangulateParameters &parameters, double dbu)
{
  tl::SelfTimer timer (tl::verbosity () > parameters.base_verbosity, "Triangles::triangulate");

  create_constrained_delaunay (poly, db::CplxTrans (dbu));
  refine (parameters);
}

void
Triangles::triangulate (const db::Polygon &poly, const TriangulateParameters &parameters, const db::CplxTrans &trans)
{
  tl::SelfTimer timer (tl::verbosity () > parameters.base_verbosity, "Triangles::triangulate");

  create_constrained_delaunay (poly, trans);
  refine (parameters);
}

void
Triangles::triangulate (const db::DPolygon &poly, const TriangulateParameters &parameters)
{
  tl::SelfTimer timer (tl::verbosity () > parameters.base_verbosity, "Triangles::triangulate");

  create_constrained_delaunay (poly);
  refine (parameters);
}

void
Triangles::refine (const TriangulateParameters &parameters)
{
  if (parameters.min_b < db::epsilon && parameters.max_area < db::epsilon && parameters.max_area_border < db::epsilon) {

    //  no refinement requested - we're done.
    remove_outside_triangles ();
    return;

  }

  unsigned int nloop = 0;
  std::list<tl::weak_ptr<db::Triangle> > new_triangles;
  for (auto t = mp_triangles.begin (); t != mp_triangles.end (); ++t) {
    new_triangles.push_back (t.operator-> ());
  }

  //  TODO: break if iteration gets stuck
  while (nloop < parameters.max_iterations) {

    ++nloop;
    if (tl::verbosity () >= parameters.base_verbosity + 10) {
      tl::info << "Iteration " << nloop << " ..";
    }

    std::list<tl::weak_ptr<db::Triangle> > to_consider;
    for (auto t = new_triangles.begin (); t != new_triangles.end (); ++t) {
      if (t->get () && ! (*t)->is_outside () && is_invalid (t->get (), parameters)) {
        to_consider.push_back (*t);
      }
    }

    if (to_consider.empty ()) {
      break;
    }

    if (tl::verbosity () >= parameters.base_verbosity + 10) {
      tl::info << to_consider.size() << " triangles to consider";
    }

    new_triangles.clear ();

    for (auto t = to_consider.begin (); t != to_consider.end (); ++t) {

      if (! t->get ()) {
        //  triangle got removed during loop
        continue;
      }

      auto cr = (*t)->circumcircle();
      auto center = cr.first;

      if ((*t)->contains (center) >= 0) {

        if (tl::verbosity () >= parameters.base_verbosity + 20) {
          tl::info << "Inserting in-triangle center " << center.to_string () << " of " << (*t)->to_string (true);
        }
        insert_point (center, &new_triangles);

      } else {

        db::Vertex *vstart = 0;
        for (int i = 0; i < 3; ++i) {
          db::TriangleEdge *edge = (*t)->edge (i);
          vstart = (*t)->opposite (edge);
          if (edge->side_of (*vstart) * edge->side_of (center) < 0) {
            break;
          }
        }

        db::TriangleEdge *edge = find_closest_edge (center, vstart, true /*inside only*/);
        tl_assert (edge != 0);

        if (! edge->is_segment () || edge->side_of (*vstart) * edge->side_of (center) >= 0) {

          if (tl::verbosity () >= parameters.base_verbosity + 20) {
            tl::info << "Inserting out-of-triangle center " << center << " of " << (*t)->to_string (true);
          }
          insert_point (center, &new_triangles);

        } else {

          double sr = edge->d ().length () * 0.5;
          if (sr >= parameters.min_length) {

            db::DPoint pnew = *edge->v1 () + edge->d () * 0.5;

            if (tl::verbosity () >= parameters.base_verbosity + 20) {
              tl::info << "Split edge " << edge->to_string (true) << " at " << pnew.to_string ();
            }
            db::Vertex *vnew = insert_point (pnew, &new_triangles);
            auto vertexes_in_diametral_circle = find_points_around (vnew, sr);

            std::vector<db::Vertex *> to_delete;
            for (auto v = vertexes_in_diametral_circle.begin (); v != vertexes_in_diametral_circle.end (); ++v) {
              bool has_segment = false;
              for (auto e = (*v)->begin_edges (); e != (*v)->end_edges () && ! has_segment; ++e) {
                has_segment = (*e)->is_segment ();
              }
              if (! has_segment) {
                to_delete.push_back (*v);
              }
            }

            if (tl::verbosity () >= parameters.base_verbosity + 20) {
              tl::info << "  -> found " << to_delete.size () << " vertexes to remove";
            }
            for (auto v = to_delete.begin (); v != to_delete.end (); ++v) {
              remove (*v, &new_triangles);
            }

          }

        }

      }

    }

  }

  if (tl::verbosity () >= parameters.base_verbosity + 20) {
    tl::info << "Finishing ..";
  }

  if (parameters.mark_triangles) {

    for (auto t = begin (); t != end (); ++t) {

      size_t id = 0;

      if (! t->is_outside ()) {

        if (is_skinny (t.operator-> (), parameters)) {
          id |= 1;
        }
        if (is_invalid (t.operator-> (), parameters)) {
          id |= 2;
        }
        auto cp = t->circumcircle ();
        auto vi = find_inside_circle (cp.first, cp.second);
        if (! vi.empty ()) {
          id |= 4;
        }

      }

      (const_cast<db::Triangle *> (t.operator->()))->set_id (id);

    }

  }

  remove_outside_triangles ();
}

}

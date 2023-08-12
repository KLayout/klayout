
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
  : m_is_constrained (false)
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
    if (edges [i]->left () == 0 && edges [i]->right () == 0) {
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

  // @@@

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

  // @@@

}

db::Vertex *
Triangles::insert (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles)
{

  // @@@

}

void
Triangles::remove (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles)
{

  // @@@

}

std::vector<db::Vertex *>
Triangles::find_touching (const db::DBox &box)
{
  //  NOTE: this is a naive, slow implementation for test purposes
  std::vector<db::Vertex *> res;
  for (auto v = m_vertex_heap.begin (); v != m_vertex_heap.end (); ++v) {
    if (v->begin_edges () != v->end_edges ()) {
      if (box.contains (*v)) {
        res.push_back (v.operator-> ());
      }
    }
  }
  return res;
}

std::vector<db::Vertex *>
Triangles::find_inside_circle (const db::DPoint &center, double radius)
{
  //  NOTE: this is a naive, slow implementation for test purposes
  std::vector<db::Vertex *> res;
  for (auto v = m_vertex_heap.begin (); v != m_vertex_heap.end (); ++v) {
    if (v->begin_edges () != v->end_edges ()) {
      if (v->in_circle (center, radius) == 1) {
        res.push_back (v.operator-> ());
      }
    }
  }
  return res;
}


void
Triangles::remove_outside_vertex (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles)
{

  // @@@

}

void
Triangles::remove_inside_vertex (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles)
{

  // @@@

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

  def insert_point(self, point, new_triangles: [Vertex] = None):
    return self.insert(Vertex(point.x, point.y), new_triangles)

  def insert(self, vertex, new_triangles: [Vertex] = None):

    self.flips = 0

    tris = self.find_triangle_for_vertex(vertex)
    if len(tris) == 0:
      assert(not self.is_constrained)
      self.vertexes.append(vertex)
      self.flips += self._insert_new_vertex(vertex, new_triangles)
      return vertex

    # the new vertex is on the edge between two triangles
    on_edges = [s for s in tris[0].edges() if s.side_of(vertex) == 0]

    if len(on_edges) > 0:
      if len(on_edges) == 1:
        self.vertexes.append(vertex)
        self.flips += self._split_triangles_on_edge(tris, vertex, on_edges[0], new_triangles)
        return vertex
      else:
        # the vertex is already present
        assert (len(on_edges) == 2)
        return on_edges[0].common_vertex(on_edges[1])
    elif len(tris) == 1:
      # the new vertex is inside one triangle
      self.vertexes.append(vertex)
      self.flips += self._split_triangle(tris[0], vertex, new_triangles)
      return vertex

    assert(False)

  def remove_vertex(self, vertex: Vertex, new_triangles: [Vertex] = None):
    if vertex.is_outside():
      self._remove_outside_vertex(vertex, new_triangles)
    else:
      self._remove_inside_vertex(vertex, new_triangles)

  def _remove_outside_vertex(self, vertex: Vertex, new_triangles_out: [Vertex] = None):

    to_remove = vertex.triangles()

    outer_edges = [ t.opposite_edge(vertex) for t in to_remove ]

    for t in to_remove:
      self.triangles.remove(t)
      t.unlink()

    self.vertexes.remove(vertex)
    vertex.unlink()

    new_triangles = self._fill_concave_corners(outer_edges)

    for nt in new_triangles:
      self.triangles.append(nt)

    self._fix_triangles(new_triangles, [], new_triangles_out)

  def _remove_inside_vertex(self, vertex: Vertex, new_triangles: [Vertex] = None):

    triangles_to_fix = []

    make_new_triangle = True

    while len(vertex.edges) > 3:

      to_flip = next((s for s in vertex.edges if s.can_flip()), None)
      if to_flip is None:
        break

      # NOTE: in the "can_join" case zero-area triangles are created which we will sort out later
      any_flipped = True
      if to_flip.left in triangles_to_fix:
        triangles_to_fix.remove(to_flip.left)
      if to_flip.right in triangles_to_fix:
        triangles_to_fix.remove(to_flip.right)
      t1, t2, _ = self.flip(to_flip)
      triangles_to_fix.append(t1)
      triangles_to_fix.append(t2)

    if len(vertex.edges) > 3:

      assert(len(vertex.edges) == 4)

      # This case can happen if two edges attached to the vertex are collinear
      # in this case choose the "join" strategy
      jseg = next((s for s in vertex.edges if s.can_join_via(vertex)), None)
      assert(jseg is not None)

      v1 = jseg.left.ext_vertex(jseg)
      s1 = jseg.left.opposite_edge(vertex)
      v2 = jseg.right.ext_vertex(jseg)
      s2 = jseg.right.opposite_edge(vertex)

      jseg_opp = next((s for s in vertex.edges if not s.has_triangle(jseg.left) and not s.has_triangle(jseg.right)), None)
      assert(jseg_opp is not None)

      s1opp = jseg_opp.left.opposite_edge(vertex)
      s2opp = jseg_opp.right.opposite_edge(vertex)

      new_edge = TriangleEdge(v1, v2)
      t1 = Triangle(s1, s2, new_edge)
      self.triangles.append(t1)
      t2 = Triangle(s1opp, s2opp, new_edge)
      self.triangles.append(t2)
      triangles_to_fix.append(t1)
      triangles_to_fix.append(t2)

      make_new_triangle = False

    to_remove = vertex.triangles()

    outer_edges = [ t.opposite_edge(vertex) for t in to_remove ]

    for t in to_remove:
      self.triangles.remove(t)
      t.unlink()
      if t in triangles_to_fix:
        triangles_to_fix.remove(t)

    self.vertexes.remove(vertex)
    vertex.unlink()

    if make_new_triangle:

      assert(len(outer_edges) == 3)

      nt = Triangle(*outer_edges)
      self.triangles.append(nt)
      triangles_to_fix.append(nt)

    if new_triangles is not None:
      new_triangles += triangles_to_fix

    self._fix_triangles(triangles_to_fix, [], new_triangles)

  def _fill_concave_corners(self, edges: [TriangleEdge]) -> [Triangle]:


  def find_triangle_for_vertex(self, p: Point) -> [Triangle]:

    edge = self.find_closest_edge(p)
    if edge is not None:
      return [ t for t in [ edge.left, edge.right ] if t is not None and t.contains(p) >= 0 ]
    else:
      return []

  def find_vertex_for_point(self, p: Point) -> TriangleEdge:

    edge = self.find_closest_edge(p)
    if edge is None:
      return None
    v = None
    if equals(edge.p1, p):
      v = edge.p1
    elif equals(edge.p2, p):
      v = edge.p2
    return v

  def find_edge_for_points(self, p1: Point, p2: Point) -> TriangleEdge:

    v = self.find_vertex_for_point(p1)
    if v is None:
      return None
    for s in v.edges:
      if equals(s.other_vertex(v), p2):
        return s
    return None

  def find_closest_edge(self, p: Point, nstart: int = None, vstart: Vertex = None, inside_only = False) -> TriangleEdge:

    if nstart is None and vstart is None:
      if len(self.vertexes) > 0:
        vstart = self.vertexes[0]
      else:
        return None
    elif nstart is not None:
      if len(self.vertexes) > nstart:
        vstart = self.vertexes[nstart]
      else:
        return None
    elif vstart is None:
      return None

    line = Edge(vstart, p)

    d = None
    edge = None

    v = vstart

    while v is not None:

      vnext = None

      for s in v.edges:
        if inside_only:
          # NOTE: in inside mode we stay on the line of sight as we don't
          # want to walk around outside pockets.
          if not s.is_segment and s.is_for_outside_triangles():
            continue
          if not s.crosses_including(line):
            continue
        ds = s.distance(p)
        if d is None or ds < d:
          d = ds
          edge = s
          vnext = edge.other_vertex(v)
        elif abs(ds - d) < max(1, abs(ds) + abs(d)) * 1e-10:
          # this differentiation selects the edge which bends further towards
          # the target point if both edges share a common point and that
          # is the one the determines the distance.
          cv = edge.common_vertex(s)
          if cv is not None:
            edge_d = sub(edge.other_vertex(cv), cv)
            s_d = sub(s.other_vertex(cv), cv)
            r = sub(p, cv)
            edge_sp = sprod(r, edge_d) / math.sqrt(square(edge_d))
            s_sp = sprod(r, s_d) / math.sqrt(square(s_d))
            if s_sp > edge_sp:
              edge = s
              vnext = edge.other_vertex(v)

      v = vnext

    return edge

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

  def _insert_new_vertex(self, vertex, new_triangles_out: [Vertex] = None):

    if len(self.vertexes) <= 3:
      if len(self.vertexes) == 3:
        # form the first triangle
        s1 = TriangleEdge(self.vertexes[0], self.vertexes[1])
        s2 = TriangleEdge(self.vertexes[1], self.vertexes[2])
        s3 = TriangleEdge(self.vertexes[2], self.vertexes[0])
        # avoid degenerate Triangles to happen here @@@
        if vprod_sign(s1.d(), s2.d()) == 0:
          self.vertexes = []   # reject some points?
        else:
          t = Triangle(s1, s2, s3)
          if new_triangles_out is not None:
            new_triangles_out.append(t)
          self.triangles.append(t)
      return 0

    new_triangles = []

    # Find closest edge
    closest_edge = self.find_closest_edge(vertex)
    assert(closest_edge is not None)

    s1 = TriangleEdge(vertex, closest_edge.p1)
    s2 = TriangleEdge(vertex, closest_edge.p2)

    t = Triangle(s1, closest_edge, s2)
    self.triangles.append(t)
    new_triangles.append(t)

    self._add_more_triangles(new_triangles, closest_edge, closest_edge.p1, vertex, s1)
    self._add_more_triangles(new_triangles, closest_edge, closest_edge.p2, vertex, s2)

    if new_triangles_out is not None:
      new_triangles_out += new_triangles

    return self._fix_triangles(new_triangles, [], new_triangles_out)

  def _add_more_triangles(self, new_triangles, incoming_edge: TriangleEdge, from_vertex: Vertex, to_vertex: Vertex, conn_edge: TriangleEdge):

    while True:

      next_edge = None
      for s in from_vertex.edges:
        if not s.has_vertex(to_vertex) and s.is_outside():
          # TODO: remove and break
          assert(next_edge is None)
          next_edge = s

      assert (next_edge is not None)
      next_vertex = next_edge.other_vertex(from_vertex)

      d_from_to = sub(to_vertex, from_vertex)
      incoming_vertex = incoming_edge.other_vertex(from_vertex)
      if vprod_sign(sub(from_vertex, incoming_vertex), d_from_to) * vprod_sign(sub(from_vertex, next_vertex), d_from_to) >= 0:
        return

      next_conn_edge = TriangleEdge(next_vertex, to_vertex)
      t = Triangle(next_conn_edge, next_edge, conn_edge)
      self.triangles.append(t)
      new_triangles.append(t)

      incoming_edge = next_edge
      conn_edge = next_conn_edge
      from_vertex = next_vertex


  def _split_triangle(self, t, vertex, new_triangles_out: [Vertex] = None):

    # TODO: this is not quite efficient
    self.triangles.remove(t)
    t.unlink()

    new_edges = {}
    for v in t.vertexes:
      new_edges[v] = TriangleEdge(v, vertex)

    new_triangles = []
    for s in t.edges():
      new_triangle = Triangle(s, new_edges[s.p1], new_edges[s.p2])
      if new_triangles_out is not None:
        new_triangles_out.append(new_triangle)
      new_triangle.is_outside = t.is_outside
      new_triangles.append(new_triangle)
      self.triangles.append(new_triangle)

    return self._fix_triangles(new_triangles, new_edges.values(), new_triangles_out)

  def _split_triangles_on_edge(self, tris, vertex: Vertex, split_edge: TriangleEdge, new_triangles_out: [Vertex] = None):

    split_edge.unlink()

    s1 = TriangleEdge(split_edge.p1, vertex)
    s2 = TriangleEdge(split_edge.p2, vertex)
    s1.is_segment = split_edge.is_segment
    s2.is_segment = split_edge.is_segment

    new_triangles = []

    for t in tris:

      self.triangles.remove(t)

      ext_vertex = t.ext_vertex(split_edge)
      new_edge = TriangleEdge(ext_vertex, vertex)

      for s in t.edges():
        if s.has_vertex(ext_vertex):
          partial = s1 if s.has_vertex(split_edge.p1) else s2
          new_triangle = Triangle(new_edge, partial, s)
          if new_triangles_out is not None:
            new_triangles_out.append(new_triangle)
          new_triangle.is_outside = t.is_outside
          new_triangles.append(new_triangle)
          self.triangles.append(new_triangle)

    return self._fix_triangles(new_triangles, [s1, s2], new_triangles_out)

  def _is_illegal_edge(self, edge):

    left = edge.left
    right = edge.right
    if left is None or right is None:
      return False

    center, radius = left.circumcircle()
    if right.ext_vertex(edge).in_circle(center, radius) > 0:
      return True

    center, radius = right.circumcircle()
    if left.ext_vertex(edge).in_circle(center, radius) > 0:
      return True

    return False

  def _fix_triangles(self, tris: [Triangle], fixed_edges: [TriangleEdge], new_triangles: [Triangle] = None):

    flips = 0

    self.level += 1
    for s in fixed_edges:
      s.level = self.level

    queue = []

    for t in tris:
      for s in t.edges():
        if s.level < self.level and not s.is_segment:
          if s not in queue:
            queue.append(s)

    while not len(queue) == 0:

      todo = queue
      queue = []

      # NOTE: we cannot be sure that already treated edges will not become
      # illegal by neighbor edges flipping ..
      #  for s in todo:
      #    s.level = self.level

      for s in todo:
        if self._is_illegal_edge(s):
          if s in queue:
            queue.remove(s)
          t1, t2, s12 = self.flip(s)
          if new_triangles is not None:
            new_triangles.append(t1)
            new_triangles.append(t2)
          flips += 1
          assert(not self._is_illegal_edge(s12))  # @@@ TODO: remove later
          for s1 in t1.edges():
            if s1.level < self.level and not s1.is_segment and s1 not in queue:
              queue.append(s1)
          for s2 in t2.edges():
            if s2.level < self.level and not s2.is_segment and s2 not in queue:
              queue.append(s2)

    return flips

  def flipped_edge(self, s: TriangleEdge) -> Edge:

    return Edge(s.left.ext_vertex(s), s.right.ext_vertex(s))

  def flip(self, s: TriangleEdge) -> (Triangle, Triangle, TriangleEdge):

    t1 = s.left
    t2 = s.right

    assert t1.is_outside == t2.is_outside

    s.unlink()
    self.triangles.remove(t1)
    self.triangles.remove(t2)

    t1_vext = t1.ext_vertex(s)
    t1_sext1 = t1.find_edge_with(t1_vext, s.p1)
    t1_sext2 = t1.find_edge_with(t1_vext, s.p2)
    t2_vext = t2.ext_vertex(s)
    t2_sext1 = t2.find_edge_with(t2_vext, s.p1)
    t2_sext2 = t2.find_edge_with(t2_vext, s.p2)
    s_new = TriangleEdge(t1_vext, t2_vext)
    t1_new = Triangle(s_new, t1_sext1, t2_sext1)
    t1_new.is_outside = t1.is_outside
    t2_new = Triangle(s_new, t1_sext2, t2_sext2)
    t2_new.is_outside = t2.is_outside

    self.triangles.append(t1_new)
    self.triangles.append(t2_new)

    return t1_new, t2_new, s_new

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


  def check(self, check_delaunay: bool = True) -> bool:

    res = True

    if check_delaunay:
      for t in self.triangles:
        center, radius = t.circumcircle()
        vi = self.find_inside_circle(center, radius)
        if len(vi) > 0:
          res = False
          print(f"(check error) triangle does not meet Delaunay criterion: {repr(t)}")
          for v in vi:
            print(f"  vertex inside circumcircle: {repr(v)}")

    edges = set()
    for t in self.triangles:
      edges.add(t.s1)
      edges.add(t.s2)
      edges.add(t.s3)
      for s in t.edges():
        if not s.has_triangle(t):
          print(f"(check error) edges {repr(s)} attached to triangle {repr(t)} does not refer to this triangle")

    for s in edges:

      if s.left and s.right:
        if s.left.is_outside != s.right.is_outside:
          if not s.is_segment:
            print(f"(check error) edge {repr(s)} splits an outside and inside triangle, but is not a segment")

      for t in [ s.left, s.right ]:
        if t is not None:
          if t.s1 != s and t.s2 != s and t.s3 != s:
            print(f"(check error) edge {repr(s)} not found in adjacent triangle {repr(t)}")
            res = False
          if t.p1() != s.p1 and t.p2() != s.p1 and t.p3() != s.p1:
            print(f"(check error) edge's {repr(s)} p1 not found in adjacent triangle {repr(t)}")
            res = False
          if t.p1() != s.p2 and t.p2() != s.p2 and t.p3() != s.p2:
            print(f"(check error) edge's {repr(s)} p2 not found in adjacent triangle {repr(t)}")
            res = False
          pext = [ p for p in t.vertexes if p != s.p1 and p != s.p2 ]
          if len(pext) != 1:
            print(f"(check error) adjacent triangle {repr(t)} has none or more than one point not in edge {repr(s)}")
            res = False
          else:
            sgn = 1.0 if t == s.left else -1.0
            vp = vprod(s.d(), sub(pext[0], s.p1))  # positive if on left side
            if vp * sgn <= 0.0:
              side_str = "left" if t == s.left else "right"
              print(f"(check error) external point {repr(pext[0])} not on {side_str} side of edge {repr(s)}")
              res = False

    for v in self.vertexes:
      for s in v.edges:
        if s not in edges:
          print(f"(check error) vertex {repr(v)} has orphan edge {repr(s)}")
          res = False

    for v in self.vertexes:
      num_outside_edges = 0
      for s in v.edges:
        if s.is_outside():
          num_outside_edges += 1
      if num_outside_edges > 0 and num_outside_edges != 2:
        print(f"(check error) vertex {repr(v)} has {num_outside_edges} outside edges (can only be 2)")
        res = False
        for s in v.edges:
          if s.is_outside():
            print(f"  Outside edge is {repr(s)}")

    vertexes = set()
    for v in self.vertexes:
      vertexes.add(v)

    for s in edges:
      if s.p1 not in vertexes:
        print(f"(check error) edge's {str(s)} p1 not found in vertex list")
        res = False
      if s not in s.p1.edges:
        print(f"(check error) edge {str(s)} not found in p1's edge list")
        res = False
      if s.p2 not in vertexes:
        print(f"(check error) edge's {str(s)} p2 not found in vertex list")
        res = False
      if s not in s.p2.edges:
        print(f"(check error) edge {str(s)} not found in p2's edge list")
        res = False

    return res

  def __str__(self):
    return ", ".join([ str(t) for t in self.triangles ])

  def dump(self):
    print(str(self))

#endif

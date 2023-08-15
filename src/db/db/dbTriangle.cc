
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


#include "dbTriangle.h"

#include <set>

namespace db
{

// -------------------------------------------------------------------------------------
//  Vertex implementation

Vertex::Vertex ()
  : DPoint (), m_level (0)
{
  //  .. nothing yet ..
}

Vertex::Vertex (const db::DPoint &p)
  : DPoint (p), m_level (0)
{
  //  .. nothing yet ..
}

Vertex::Vertex (const Vertex &v)
  : DPoint (), m_level (0)
{
  operator= (v);
}

Vertex &Vertex::operator= (const Vertex &v)
{
  if (this != &v) {
    //  NOTE: edges are not copied!
    db::DPoint::operator= (v);
    m_level = v.m_level;
  }
  return *this;
}

Vertex::Vertex (db::DCoord x, db::DCoord y)
  : DPoint (x, y), m_level (0)
{
  //  .. nothing yet ..
}

bool
Vertex::is_outside () const
{
  for (auto e = m_edges.begin (); e != m_edges.end (); ++e) {
    if (e->is_outside ()) {
      return true;
    }
  }
  return false;
}

std::vector<db::Triangle *>
Vertex::triangles () const
{
  std::set<db::Triangle *> seen;
  std::vector<db::Triangle *> res;
  for (auto e = m_edges.begin (); e != m_edges.end (); ++e) {
    for (auto t = e->begin_triangles (); t != e->end_triangles (); ++t) {
      if (seen.insert (t.operator-> ()).second) {
        res.push_back (t.operator-> ());
      }
    }
  }
  return res;
}

bool
Vertex::has_edge (const TriangleEdge *edge) const
{
  for (auto e = m_edges.begin (); e != m_edges.end (); ++e) {
    if (e.operator-> () == edge) {
      return true;
    }
  }
  return false;
}

std::string
Vertex::to_string (bool with_id) const
{
  std::string res = tl::sprintf ("(%.12g, %.12g)", x (), y());
  if (with_id) {
    res += tl::sprintf ("[%p]", (void *)this);
  }
  return res;
}

int
Vertex::in_circle (const DPoint &point, const DPoint &center, double radius)
{
  double dx = point.x () - center.x ();
  double dy = point.y () - center.y ();
  double d2 = dx * dx + dy * dy;
  double r2 = radius * radius;
  double delta = fabs (d2 + r2) * db::epsilon;
  if (d2 < r2 - delta) {
    return 1;
  } else if (d2 < r2 + delta) {
    return 0;
  } else {
    return -1;
  }
}

// -------------------------------------------------------------------------------------
//  TriangleEdge implementation

TriangleEdge::TriangleEdge ()
  : mp_v1 (0), mp_v2 (0), mp_left (), mp_right (), m_level (0), m_id (0), m_is_segment (false)
{
  // .. nothing yet ..
}

TriangleEdge::TriangleEdge (Vertex *v1, Vertex *v2)
  : mp_v1 (v1), mp_v2 (v2), mp_left (), mp_right (), m_level (0), m_id (0), m_is_segment (false)
{
  v1->m_edges.push_back (this);
  v2->m_edges.push_back (this);
}

void
TriangleEdge::set_left  (Triangle *t)
{
  mp_left = t;
}

void
TriangleEdge::set_right (Triangle *t)
{
  mp_right = t;
}

Triangle *
TriangleEdge::other (const Triangle *t) const
{
  if (t == mp_left.get ()) {
    return const_cast<Triangle *> (mp_right.get ());
  }
  if (t == mp_right.get ()) {
    return const_cast<Triangle *> (mp_left.get ());
  }
  tl_assert (false);
  return 0;
}

Vertex *
TriangleEdge::other (const Vertex *t) const
{
  if (t == mp_v1) {
    return mp_v2;
  }
  if (t == mp_v2) {
    return mp_v1;
  }
  tl_assert (false);
  return 0;
}

bool
TriangleEdge::has_vertex (const Vertex *v) const
{
  return mp_v1 == v || mp_v2 == v;
}

Vertex *
TriangleEdge::common_vertex (const TriangleEdge *other) const
{
  if (has_vertex (other->v1 ())) {
    return (other->v1 ());
  }
  if (has_vertex (other->v2 ())) {
    return (other->v2 ());
  }
  return 0;
}

std::string
TriangleEdge::to_string (bool with_id) const
{
  std::string res = std::string ("(") + mp_v1->to_string (with_id) + ", " + mp_v2->to_string (with_id) + ")";
  if (with_id) {
    res += tl::sprintf ("[%p]", (void *)this);
  }
  return res;
}

double
TriangleEdge::distance (const db::DEdge &e, const db::DPoint &p)
{
  double l = db::sprod (p - e.p1 (), e.d ()) / e.d ().sq_length ();
  db::DPoint pp;
  if (l <= 0.0) {
    pp = e.p1 ();
  } else if (l >= 1.0) {
    pp = e.p2 ();
  } else {
    pp = e.p1 () + e.d () * l;
  }
  return (p - pp).length ();
}

bool
TriangleEdge::crosses (const db::DEdge &e, const db::DEdge &other)
{
  return e.side_of (other.p1 ()) * e.side_of (other.p2 ()) < 0 &&
         other.side_of (e.p1 ()) * other.side_of (e.p2 ()) < 0;
}

bool
TriangleEdge::crosses_including (const db::DEdge &e, const db::DEdge &other)
{
  return e.side_of (other.p1 ()) * e.side_of (other.p2 ()) <= 0 &&
         other.side_of (e.p1 ()) * other.side_of (e.p2 ()) <= 0;
}

db::DPoint
TriangleEdge::intersection_point (const db::DEdge &e, const db::DEdge &other)
{
  return e.intersect_point (other).second;
}

bool
TriangleEdge::point_on (const db::DEdge &edge, const db::DPoint &point)
{
  if (edge.side_of (point) != 0) {
    return false;
  } else {
    return db::sprod_sign (point - edge.p1 (), edge.d ()) * db::sprod_sign(point - edge.p2 (), edge.d ()) < 0;
  }
}

bool
TriangleEdge::can_flip () const
{
  if (! left () || ! right ()) {
    return false;
  }

  const db::Vertex *v1 = left ()->opposite (this);
  const db::Vertex *v2 = right ()->opposite (this);
  return crosses (db::DEdge (*v1, *v2));
}

bool
TriangleEdge::can_join_via (const Vertex *vertex) const
{
  if (! left () || ! right ()) {
    return false;
  }

  tl_assert (has_vertex (vertex));
  const db::Vertex *v1 = left ()->opposite (this);
  const db::Vertex *v2 = right ()->opposite (this);
  return db::DEdge (*v1, *v2).side_of (*vertex) == 0;
}

bool
TriangleEdge::is_outside () const
{
  return left () == 0 || right () == 0;
}

bool
TriangleEdge::is_for_outside_triangles () const
{
  return (left () && left ()->is_outside ()) || (right () && right ()->is_outside ());
}

bool
TriangleEdge::has_triangle (const Triangle *t) const
{
  return t != 0 && (left () == t || right () == t);
}

// -------------------------------------------------------------------------------------
//  Triangle implementation

Triangle::Triangle ()
  : m_is_outside (false), mp_v1 (0), mp_v2 (0), mp_v3 (0), m_id (0)
{
  //  .. nothing yet ..
}

Triangle::Triangle (TriangleEdge *e1, TriangleEdge *e2, TriangleEdge *e3)
  : m_is_outside (false), mp_e1 (e1), mp_e2 (e2), mp_e3 (e3), m_id (0)
{
  mp_v1 = e1->v1 ();
  mp_v2 = e1->other (mp_v1);

  if (e2->has_vertex (mp_v2)) {
    mp_v3 = e2->other (mp_v2);
    tl_assert (e3->other (mp_v3) == mp_v1);
  } else {
    mp_v3 = e3->other (mp_v2);
    tl_assert (e2->other (mp_v3) == mp_v1);
  }

  //  enforce clockwise orientation
  if (db::vprod_sign (*mp_v3 - *mp_v1, *mp_v2 - *mp_v1) < 0) {
    std::swap (mp_v3, mp_v2);
  }

  //  establish link to edges
  for (int i = 0; i < 3; ++i) {
    TriangleEdge *e = edge (i);
    int side_of = 0;
    for (int j = 0; j < 3; ++j) {
      side_of += e->side_of (*vertex (j));
    }
    //  NOTE: in the degenerated case, the triangle is not attached to an edge!
    if (side_of < 0) {
      e->set_left (this);
    } else if (side_of > 0) {
      e->set_right (this);
    }
  }
}

void
Triangle::unlink ()
{
  for (int i = 0; i != 3; ++i) {
    db::TriangleEdge *e = edge (i);
    if (e->left () == this) {
      e->set_left (0);
    }
    if (e->right () == this) {
      e->set_right (0);
    }
  }
}

std::string
Triangle::to_string (bool with_id) const
{
  std::string res = "(";
  for (int i = 0; i < 3; ++i) {
    if (i > 0) {
      res += ", ";
    }
    if (vertex (i)) {
      res += vertex (i)->to_string (with_id);
    } else {
      res += "(null)";
    }
  }
  res += ")";
  return res;
}

Vertex *
Triangle::vertex (int n) const
{
  tl_assert (mp_e1 && mp_e2 && mp_e3);
  n = (n + 3) % 3;
  if (n == 0) {
    return mp_v1;
  } else if (n == 1) {
    return mp_v2;
  } else {
    return mp_v3;
  }
}

TriangleEdge *
Triangle::edge (int n) const
{
  n = (n + 3) % 3;
  if (n == 0) {
    return const_cast<TriangleEdge *> (mp_e1.get ());
  } else if (n == 1) {
    return const_cast<TriangleEdge *> (mp_e2.get ());
  } else {
    return const_cast<TriangleEdge *> (mp_e3.get ());
  }
}

double
Triangle::area () const
{
  return fabs (db::vprod (mp_e1->d (), mp_e2->d ())) * 0.5;
}

db::DBox
Triangle::bbox () const
{
  db::DBox box;
  for (int i = 0; i < 3; ++i) {
    box += *vertex (i);
  }
  return box;
}


std::pair<db::DPoint, double>
Triangle::circumcircle () const
{
  db::DVector v1 = *vertex(0) - *vertex(1);
  db::DVector v2 = *vertex(0) - *vertex(2);
  db::DVector n1 = db::DVector (v1.y (), -v1.x ());
  db::DVector n2 = db::DVector (v2.y (), -v2.x ());

  double p1s = v1.sq_length ();
  double p2s = v2.sq_length ();

  double s = db::vprod (v1, v2);
  tl_assert (fabs (s) > db::epsilon);

  db::DVector r = (n1 * p2s - n2 * p1s) * (0.5 / s);
  db::DPoint center = *vertex (0) + r;
  double radius = r.length ();

  return std::make_pair (center, radius);
}

Vertex *
Triangle::opposite (const TriangleEdge *edge) const
{
  for (int i = 0; i < 3; ++i) {
    Vertex *v = vertex (i);
    if (! edge->has_vertex (v)) {
      return v;
    }
  }
  tl_assert (false);
}

TriangleEdge *
Triangle::opposite (const Vertex *vertex) const
{
  for (int i = 0; i < 3; ++i) {
    TriangleEdge *e = edge (i);
    if (! e->has_vertex (vertex)) {
      return e;
    }
  }
  tl_assert (false);
}

TriangleEdge *
Triangle::find_edge_with (const Vertex *v1, const Vertex *v2) const
{
  for (int i = 0; i < 3; ++i) {
    TriangleEdge *e = edge (i);
    if (e->has_vertex (v1) && e->has_vertex (v2)) {
      return e;
    }
  }
  tl_assert (false);
}

int
Triangle::contains (const db::DPoint &point) const
{
  int res = 1;
  const Vertex *vl = vertex (-1);
  for (int i = 0; i < 3; ++i) {
    const Vertex *v = vertex (i);
    int s = db::DEdge (*vl, *v).side_of (point);
    if (s == 0) {
      res = 0;
    } else if (s > 0) {
      return -1;
    }
    vl = v;
  }
  return res;
}

}


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


#include "dbTriangle.h"

#include <set>

namespace db
{

// -------------------------------------------------------------------------------------
//  Vertex implementation

Vertex::Vertex ()
  : DPoint (), m_is_precious (false)
{
  //  .. nothing yet ..
}

Vertex::Vertex (const db::DPoint &p)
  : DPoint (p), m_is_precious (false)
{
  //  .. nothing yet ..
}

Vertex::Vertex (const Vertex &v)
  : DPoint (), m_is_precious (false)
{
  operator= (v);
}

Vertex &Vertex::operator= (const Vertex &v)
{
  if (this != &v) {
    //  NOTE: edges are not copied!
    db::DPoint::operator= (v);
    m_is_precious = v.m_is_precious;
  }
  return *this;
}

Vertex::Vertex (db::DCoord x, db::DCoord y)
  : DPoint (x, y), m_is_precious (false)
{
  //  .. nothing yet ..
}

bool
Vertex::is_outside () const
{
  for (auto e = mp_edges.begin (); e != mp_edges.end (); ++e) {
    if ((*e)->is_outside ()) {
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
  for (auto e = mp_edges.begin (); e != mp_edges.end (); ++e) {
    for (auto t = (*e)->begin_triangles (); t != (*e)->end_triangles (); ++t) {
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
  for (auto e = mp_edges.begin (); e != mp_edges.end (); ++e) {
    if (*e == edge) {
      return true;
    }
  }
  return false;
}

size_t
Vertex::num_edges (int max_count) const
{
  if (max_count < 0) {
    //  NOTE: this can be slow for a std::list, so we have max_count to limit this effort
    return mp_edges.size ();
  } else {
    size_t n = 0;
    for (auto i = mp_edges.begin (); i != mp_edges.end () && --max_count >= 0; ++i) {
      ++n;
    }
    return n;
  }
}

std::string
Vertex::to_string (bool with_id) const
{
  std::string res = tl::sprintf ("(%.12g, %.12g)", x (), y());
  if (with_id) {
    res += tl::sprintf ("[%x]", (size_t)this);
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
  //  .. nothing yet ..
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

void
TriangleEdge::link ()
{
  mp_v1->mp_edges.push_back (this);
  m_ec_v1 = --mp_v1->mp_edges.end ();

  mp_v2->mp_edges.push_back (this);
  m_ec_v2 = --mp_v2->mp_edges.end ();
}

void
TriangleEdge::unlink ()
{
  if (mp_v1) {
    mp_v1->remove_edge (m_ec_v1);
  }
  if (mp_v2) {
    mp_v2->remove_edge (m_ec_v2);
  }
  mp_v1 = mp_v2 = 0;
}

Triangle *
TriangleEdge::other (const Triangle *t) const
{
  if (t == mp_left) {
    return mp_right;
  }
  if (t == mp_right) {
    return mp_left;
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
    res += tl::sprintf ("[%x]", (size_t)this);
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
  : m_is_outside (false), m_id (0)
{
  for (int i = 0; i < 3; ++i) {
    mp_v[i] = 0;
    mp_e[i] = 0;
  }
}

Triangle::Triangle (TriangleEdge *e1, TriangleEdge *e2, TriangleEdge *e3)
  : m_is_outside (false), m_id (0)
{
  mp_e[0] = e1;
  mp_v[0] = e1->v1 ();
  mp_v[1] = e1->v2 ();

  if (e2->has_vertex (mp_v[1])) {
    mp_e[1] = e2;
    mp_e[2] = e3;
  } else {
    mp_e[1] = e3;
    mp_e[2] = e2;
  }
  mp_v[2] = mp_e[1]->other (mp_v[1]);

  //  enforce clockwise orientation
  int s = db::vprod_sign (*mp_v[2] - *mp_v[0], *mp_v[1] - *mp_v[0]);
  if (s < 0) {
    std::swap (mp_v[2], mp_v[1]);
  } else if (s == 0) {
    //  Triangle is not orientable
    tl_assert (false);
  }

  //  establish link to edges
  for (int i = 0; i < 3; ++i) {

    TriangleEdge *e = mp_e[i];

    unsigned int i1 = 0;
    for ( ; e->v1 () != mp_v[i1] && i1 < 3; ++i1)
      ;
    unsigned int i2 = 0;
    for ( ; e->v2 () != mp_v[i2] && i2 < 3; ++i2)
      ;

    if ((i1 + 1) % 3 == i2) {
      e->set_right (this);
    } else {
      e->set_left (this);
    }

  }
}

Triangle::~Triangle ()
{
  unlink ();
}

void
Triangle::unlink ()
{
  for (int i = 0; i != 3; ++i) {
    db::TriangleEdge *e = mp_e[i];
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

double
Triangle::area () const
{
  return fabs (db::vprod (mp_e[0]->d (), mp_e[1]->d ())) * 0.5;
}

db::DBox
Triangle::bbox () const
{
  db::DBox box;
  for (int i = 0; i < 3; ++i) {
    box += *mp_v[i];
  }
  return box;
}


std::pair<db::DPoint, double>
Triangle::circumcircle (bool *ok) const
{
  //  see https://en.wikipedia.org/wiki/Circumcircle
  //  we set A=(0,0), so the formulas simplify

  if (ok) {
    *ok = true;
  }

  db::DVector b = *mp_v[1] - *mp_v[0];
  db::DVector c = *mp_v[2] - *mp_v[0];

  double b2 = b.sq_length ();
  double c2 = c.sq_length ();

  double sx = 0.5 * (b2 * c.y () - c2 * b.y ());
  double sy = 0.5 * (b.x () * c2 - c.x() * b2);

  double a1 = b.x() * c.y();
  double a2 = c.x() * b.y();
  double a = a1 - a2;
  double a_abs = std::abs (a);

  if (a_abs < (std::abs (a1) + std::abs (a2)) * db::epsilon) {
    if (ok) {
      *ok = false;
      return std::make_pair (db::DPoint (), 0.0);
    } else {
      tl_assert (false);
    }
  }

  double radius = sqrt (sx * sx + sy * sy) / a_abs;
  db::DPoint center = *mp_v[0] + db::DVector (sx / a, sy / a);

  return std::make_pair (center, radius);
}

Vertex *
Triangle::opposite (const TriangleEdge *edge) const
{
  for (int i = 0; i < 3; ++i) {
    Vertex *v = mp_v[i];
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
    TriangleEdge *e = mp_e[i];
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
    TriangleEdge *e = mp_e[i];
    if (e->has_vertex (v1) && e->has_vertex (v2)) {
      return e;
    }
  }
  tl_assert (false);
}

TriangleEdge *
Triangle::common_edge (const Triangle *other) const
{
  for (int i = 0; i < 3; ++i) {
    TriangleEdge *e = mp_e[i];;
    if (e->other (this) == other) {
      return e;
    }
  }
  return 0;
}

int
Triangle::contains (const db::DPoint &point) const
{
  auto c = *mp_v[2] - *mp_v[0];
  auto b = *mp_v[1] - *mp_v[0];

  int vps = db::vprod_sign (c, b);
  if (vps == 0) {
    return db::vprod_sign (point - *mp_v[0], b) == 0 && db::vprod_sign (point - *mp_v[0], c) == 0 ? 0 : -1;
  }

  int res = 1;

  const Vertex *vl = mp_v[2];
  for (int i = 0; i < 3; ++i) {
    const Vertex *v = mp_v[i];
    int n = db::vprod_sign (point - *vl, *v - *vl) * vps;
    if (n < 0) {
      return -1;
    } else if (n == 0) {
      res = 0;
    }
    vl = v;
  }

  return res;
}

double
Triangle::min_edge_length () const
{
  double lmin = mp_e[0]->d ().length ();
  for (int i = 1; i < 3; ++i) {
    lmin = std::min (lmin, mp_e[i]->d ().length ());
  }
  return lmin;
}

double
Triangle::b () const
{
  double lmin = min_edge_length ();
  bool ok = false;
  auto cr = circumcircle (&ok);
  return ok ? lmin / cr.second : 0.0;
}

bool
Triangle::has_segment () const
{
  for (int i = 0; i < 3; ++i) {
    if (mp_e[i]->is_segment ()) {
      return true;
    }
  }
  return false;
}

unsigned int
Triangle::num_segments () const
{
  unsigned int n = 0;
  for (int i = 0; i < 3; ++i) {
    if (mp_e[i]->is_segment ()) {
      ++n;
    }
  }
  return n;
}

}

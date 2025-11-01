
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


#include "dbPLC.h"
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

namespace plc
{

// -------------------------------------------------------------------------------------
//  Vertex implementation

Vertex::Vertex (Graph *graph)
  : DPoint (), mp_graph (graph), mp_ids (0)
{
  //  .. nothing yet ..
}

Vertex::Vertex (Graph *graph, const db::DPoint &p)
  : DPoint (p), mp_graph (graph), mp_ids (0)
{
  //  .. nothing yet ..
}

Vertex::Vertex (Graph *graph, const Vertex &v)
  : DPoint (), mp_graph (graph), mp_ids (0)
{
  operator= (v);
}

Vertex::Vertex (Graph *graph, db::DCoord x, db::DCoord y)
  : DPoint (x, y), mp_graph (graph), mp_ids (0)
{
  //  .. nothing yet ..
}

Vertex::Vertex (const Vertex &v)
  : DPoint (), mp_graph (v.mp_graph), mp_ids (0)
{
  operator= (v);
}

Vertex::~Vertex ()
{
  if (mp_ids) {
    delete mp_ids;
    mp_ids = 0;
  }
}

Vertex &Vertex::operator= (const Vertex &v)
{
  if (this != &v) {

    //  NOTE: edges are not copied!
    db::DPoint::operator= (v);

    if (mp_ids) {
      delete mp_ids;
      mp_ids = 0;
    }
    if (v.mp_ids) {
      mp_ids = new std::set<unsigned int> (*v.mp_ids);
    }

  }
  return *this;
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

bool
Vertex::is_on_outline () const
{
  for (auto e = mp_edges.begin (); e != mp_edges.end (); ++e) {
    if ((*e)->is_segment ()) {
      return true;
    }
  }
  return false;
}

void
Vertex::set_is_precious (bool f, unsigned int id)
{
  if (f) {
    if (! mp_ids) {
      mp_ids = new std::set<unsigned int> ();
    }
    mp_ids->insert (id);
  } else {
    if (mp_ids) {
      delete mp_ids;
      mp_ids = 0;
    }
  }
}

bool
Vertex::is_precious () const
{
  return mp_ids != 0;
}

const std::set<unsigned int> &
Vertex::ids () const
{
  if (mp_ids != 0) {
    return *mp_ids;
  } else {
    static std::set<unsigned int> empty;
    return empty;
  }
}

std::vector<Polygon *>
Vertex::polygons () const
{
  std::set<Polygon *> seen;
  std::vector<Polygon *> res;
  for (auto e = mp_edges.begin (); e != mp_edges.end (); ++e) {
    for (auto t = (*e)->begin_polygons (); t != (*e)->end_polygons (); ++t) {
      if (seen.insert (t.operator-> ()).second) {
        res.push_back (t.operator-> ());
      }
    }
  }
  return res;
}

bool
Vertex::has_edge (const Edge *edge) const
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
//  Edge implementation

Edge::Edge (Graph *graph)
  : mp_graph (graph), mp_v1 (0), mp_v2 (0), mp_left (), mp_right (), m_level (0), m_id (0), m_is_segment (false)
{
  // .. nothing yet ..
}

Edge::Edge (Graph *graph, Vertex *v1, Vertex *v2)
  : mp_graph (graph), mp_v1 (v1), mp_v2 (v2), mp_left (), mp_right (), m_level (0), m_id (0), m_is_segment (false)
{
  //  .. nothing yet ..
}

Edge::~Edge ()
{
  //  .. nothing yet ..
}

void
Edge::set_left  (Polygon *t)
{
  mp_left = t;
}

void
Edge::set_right (Polygon *t)
{
  mp_right = t;
}

void
Edge::link ()
{
  mp_v1->mp_edges.push_back (this);
  m_ec_v1 = --mp_v1->mp_edges.end ();

  mp_v2->mp_edges.push_back (this);
  m_ec_v2 = --mp_v2->mp_edges.end ();
}

void
Edge::unlink ()
{
  if (mp_v1) {
    mp_v1->remove_edge (m_ec_v1);
  }
  if (mp_v2) {
    mp_v2->remove_edge (m_ec_v2);
  }
  mp_v1 = mp_v2 = 0;
}

Polygon *
Edge::other (const Polygon *t) const
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
Edge::other (const Vertex *t) const
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
Edge::has_vertex (const Vertex *v) const
{
  return mp_v1 == v || mp_v2 == v;
}

Vertex *
Edge::common_vertex (const Edge *other) const
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
Edge::to_string (bool with_id) const
{
  std::string res = std::string ("(") + mp_v1->to_string (with_id) + ", " + mp_v2->to_string (with_id) + ")";
  if (with_id) {
    res += tl::sprintf ("[%x]", (size_t)this);
  }
  return res;
}

double
Edge::distance (const db::DEdge &e, const db::DPoint &p)
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
Edge::crosses (const db::DEdge &e, const db::DEdge &other)
{
  return e.side_of (other.p1 ()) * e.side_of (other.p2 ()) < 0 &&
         other.side_of (e.p1 ()) * other.side_of (e.p2 ()) < 0;
}

bool
Edge::crosses_including (const db::DEdge &e, const db::DEdge &other)
{
  int sa = e.side_of (other.p1 ());
  int sb = e.side_of (other.p2 ());
  int s1 = sa * sb;

  int s2 = other.side_of (e.p1 ()) * other.side_of (e.p2 ());

  //  e can end on other and so can other end on e, but both may not be coincident
  return s1 <= 0 && s2 <= 0 && ! (sa == 0 && sb == 0);
}

db::DPoint
Edge::intersection_point (const db::DEdge &e, const db::DEdge &other)
{
  return e.intersect_point (other).second;
}

bool
Edge::point_on (const db::DEdge &edge, const db::DPoint &point)
{
  if (edge.side_of (point) != 0) {
    return false;
  } else {
    return db::sprod_sign (point - edge.p1 (), edge.d ()) * db::sprod_sign(point - edge.p2 (), edge.d ()) < 0;
  }
}

bool
Edge::can_flip () const
{
  if (! left () || ! right ()) {
    return false;
  }

  const Vertex *v1 = left ()->opposite (this);
  const Vertex *v2 = right ()->opposite (this);
  return crosses (db::DEdge (*v1, *v2));
}

bool
Edge::can_join_via (const Vertex *vertex) const
{
  if (! left () || ! right ()) {
    return false;
  }

  tl_assert (has_vertex (vertex));
  const Vertex *v1 = left ()->opposite (this);
  const Vertex *v2 = right ()->opposite (this);
  return db::DEdge (*v1, *v2).side_of (*vertex) == 0;
}

bool
Edge::is_outside () const
{
  return left () == 0 || right () == 0;
}

bool
Edge::is_for_outside_triangles () const
{
  return (left () && left ()->is_outside ()) || (right () && right ()->is_outside ());
}

bool
Edge::has_polygon (const Polygon *t) const
{
  return t != 0 && (left () == t || right () == t);
}

// -------------------------------------------------------------------------------------
//  Polygon implementation

Polygon::Polygon (Graph *graph)
  : mp_graph (graph), m_is_outside (false), m_id (0)
{
  //  .. nothing yet ..
}

void
Polygon::init ()
{
  m_id = 0;
  m_is_outside = false;

  if (mp_e.empty ()) {
    return;
  }

  std::vector<Edge *> e;
  e.swap (mp_e);

  std::multimap<Vertex *, Edge *> v2e;

  for (auto i = e.begin (); i != e.end (); ++i) {
    if (i != e.begin ()) {
      v2e.insert (std::make_pair ((*i)->v1 (), *i));
      v2e.insert (std::make_pair ((*i)->v2 (), *i));
    }
  }

  mp_e.reserve (e.size ());
  mp_e.push_back (e.front ());

  mp_v.reserve (e.size ());
  mp_v.push_back (mp_e.back ()->v1 ());

  auto v = mp_e.back ()->v2 ();

  //  join the edges in the order of the polygon
  while (! v2e.empty ()) {

    mp_v.push_back (v);

    auto i = v2e.find (v);
    tl_assert (i != v2e.end () && i->first == v && i->second != mp_e.back ());
    mp_e.push_back (i->second);
    v = i->second->other (v);

    v2e.erase (i);

    i = v2e.find (v);
    while (i != v2e.end () && i->first == v) {
      if (i->second == mp_e.back ()) {
        v2e.erase (i);
        break;
      }
      ++i;
    }

  }

  //  establish clockwise order of the vertexes

  double area = 0.0;
  const Vertex *vm1 = vertex (-1), *v0;
  for (auto i = mp_v.begin (); i != mp_v.end (); ++i) {
    v0 = *i;
    area += db::vprod (*vm1 - db::DPoint (), *v0 - *vm1);
    vm1 = v0;
  }

  if (area > db::epsilon) {
    std::reverse (mp_v.begin (), mp_v.end ());
    std::reverse (mp_e.begin (), mp_e.end ());
    std::rotate (mp_e.begin (), ++mp_e.begin (), mp_e.end ());
  }

  //  link the polygon to the edges

  for (size_t i = 0; i < size (); ++i) {
    Vertex *v = mp_v[i];
    Edge *e = mp_e[i];
    if (e->v1 () == v) {
      e->set_right (this);
    } else {
      e->set_left (this);
    }
  }
}

Polygon::Polygon (Graph *graph, Edge *e1, Edge *e2, Edge *e3)
  : mp_graph (graph), m_is_outside (false), m_id (0)
{
  mp_e.resize (3, 0);
  mp_v.resize (3, 0);

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

    Edge *e = mp_e[i];

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

Polygon::~Polygon ()
{
  unlink ();
}

void
Polygon::unlink ()
{
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    if ((*e)->left () == this) {
      (*e)->set_left (0);
    }
    if ((*e)->right () == this) {
      (*e)->set_right (0);
    }
  }
}

std::string
Polygon::to_string (bool with_id) const
{
  std::string res = "(";
  for (int i = 0; i < int (size ()); ++i) {
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
Polygon::area () const
{
  return fabs (db::vprod (mp_e[0]->d (), mp_e[1]->d ())) * 0.5;
}

db::DBox
Polygon::bbox () const
{
  db::DBox box;
  for (auto i = mp_v.begin (); i != mp_v.end (); ++i) {
    box += **i;
  }
  return box;
}

db::DPolygon
Polygon::polygon () const
{
  std::vector<db::DPoint> pts;
  for (int i = 0; i < int (size ()); ++i) {
    pts.push_back (*vertex (i));
  }

  db::DPolygon poly;
  poly.assign_hull (pts.begin (), pts.end (), false);
  return poly;
}

std::pair<db::DPoint, double>
Polygon::circumcircle (bool *ok) const
{
  tl_assert (mp_v.size () == 3);

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
Polygon::opposite (const Edge *edge) const
{
  tl_assert (mp_v.size () == 3);

  for (int i = 0; i < 3; ++i) {
    Vertex *v = mp_v[i];
    if (! edge->has_vertex (v)) {
      return v;
    }
  }
  tl_assert (false);
}

Edge *
Polygon::opposite (const Vertex *vertex) const
{
  tl_assert (mp_v.size () == 3);

  for (int i = 0; i < 3; ++i) {
    Edge *e = mp_e[i];
    if (! e->has_vertex (vertex)) {
      return e;
    }
  }
  tl_assert (false);
}

Edge *
Polygon::find_edge_with (const Vertex *v1, const Vertex *v2) const
{
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    if ((*e)->has_vertex (v1) && (*e)->has_vertex (v2)) {
      return *e;
    }
  }
  tl_assert (false);
}

Edge *
Polygon::common_edge (const Polygon *other) const
{
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    if ((*e)->other (this) == other) {
      return *e;
    }
  }
  return 0;
}

int
Polygon::contains (const db::DPoint &point) const
{
  tl_assert (mp_v.size () == 3);

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

Edge *
Polygon::next_edge (const Edge *edge, const Vertex *vertex) const
{
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    if (*e != edge && ((*e)->v1 () == vertex || (*e)->v2 () == vertex)) {
      return *e;
    }
  }
  return 0;
}

double
Polygon::min_edge_length () const
{
  double lmin = mp_e[0]->d ().length ();
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    lmin = std::min (lmin, (*e)->d ().length ());
  }
  return lmin;
}

double
Polygon::b () const
{
  double lmin = min_edge_length ();
  bool ok = false;
  auto cr = circumcircle (&ok);
  return ok ? lmin / cr.second : 0.0;
}

bool
Polygon::has_segment () const
{
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    if ((*e)->is_segment ()) {
      return true;
    }
  }
  return false;
}

unsigned int
Polygon::num_segments () const
{
  unsigned int n = 0;
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    if ((*e)->is_segment ()) {
      ++n;
    }
  }
  return n;
}

// -----------------------------------------------------------------------------------

Graph::Graph ()
  : m_id (0)
{
  //  .. nothing yet ..
}

Graph::~Graph ()
{
  clear ();
}

Vertex *
Graph::create_vertex (double x, double y)
{
  m_vertex_heap.push_back (Vertex (this, x, y));
  return &m_vertex_heap.back ();
}

Vertex *
Graph::create_vertex (const db::DPoint &pt)
{
  m_vertex_heap.push_back (Vertex (this, pt));
  return &m_vertex_heap.back ();
}

Edge *
Graph::create_edge (Vertex *v1, Vertex *v2)
{
  Edge *edge = 0;

  if (! m_returned_edges.empty ()) {
    edge = m_returned_edges.back ();
    m_returned_edges.pop_back ();
    *edge = Edge (this, v1, v2);
  } else {
    m_edges_heap.push_back (Edge (this, v1, v2));
    edge = &m_edges_heap.back ();
  }

  edge->link ();
  edge->set_id (++m_id);
  return edge;
}

Polygon *
Graph::create_triangle (Edge *e1, Edge *e2, Edge *e3)
{
  Polygon *res = new Polygon (this, e1, e2, e3);
  res->set_id (++m_id);
  mp_polygons.push_back (res);

  return res;
}

void
Graph::remove_polygon (Polygon *poly)
{
  std::vector<Edge *> edges;
  edges.resize (poly->size (), 0);
  for (int i = 0; i < int (poly->size ()); ++i) {
    edges [i] = poly->edge (i);
  }

  delete poly;

  //  clean up edges we do no longer need
  for (auto e = edges.begin (); e != edges.end (); ++e) {
    if ((*e) && (*e)->left () == 0 && (*e)->right () == 0 && (*e)->v1 ()) {
      (*e)->unlink ();
      m_returned_edges.push_back (*e);
    }
  }
}

std::string
Graph::to_string ()
{
  std::string res;
  for (auto t = mp_polygons.begin (); t != mp_polygons.end (); ++t) {
    if (! res.empty ()) {
      res += ", ";
    }
    res += t->to_string ();
  }
  return res;
}

db::DBox
Graph::bbox () const
{
  db::DBox box;
  for (auto t = mp_polygons.begin (); t != mp_polygons.end (); ++t) {
    box += t->bbox ();
  }
  return box;
}

db::Layout *
Graph::to_layout (bool decompose_by_id) const
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

  std::vector<db::DPoint> pts;
  for (auto t = mp_polygons.begin (); t != mp_polygons.end (); ++t) {
    pts.clear ();
    for (int i = 0; i < int (t->size ()); ++i) {
      pts.push_back (*t->vertex (i));
    }
    db::DPolygon poly;
    poly.assign_hull (pts.begin (), pts.end ());
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
Graph::dump (const std::string &path, bool decompose_by_id) const
{
  std::unique_ptr<db::Layout> ly (to_layout (decompose_by_id));

  tl::OutputStream stream (path);

  db::SaveLayoutOptions opt;
  db::Writer writer (opt);
  writer.write (*ly, stream);

  tl::info << "Graph written to " << path;
}

void
Graph::clear ()
{
  mp_polygons.clear ();
  m_edges_heap.clear ();
  m_vertex_heap.clear ();
  m_returned_edges.clear ();
  m_id = 0;
}

}  // namespace plc

}  // namespace db

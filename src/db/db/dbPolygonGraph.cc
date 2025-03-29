
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


#include "dbPolygonGraph.h"
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

// -------------------------------------------------------------------------------------
//  GVertex implementation

GVertex::GVertex ()
  : DPoint (), m_is_precious (false)
{
  //  .. nothing yet ..
}

GVertex::GVertex (const db::DPoint &p)
  : DPoint (p), m_is_precious (false)
{
  //  .. nothing yet ..
}

GVertex::GVertex (const GVertex &v)
  : DPoint (), m_is_precious (false)
{
  operator= (v);
}

GVertex &GVertex::operator= (const GVertex &v)
{
  if (this != &v) {
    //  NOTE: edges are not copied!
    db::DPoint::operator= (v);
    m_is_precious = v.m_is_precious;
  }
  return *this;
}

GVertex::GVertex (db::DCoord x, db::DCoord y)
  : DPoint (x, y), m_is_precious (false)
{
  //  .. nothing yet ..
}

#if 0 // @@@
bool
GVertex::is_outside () const
{
  for (auto e = mp_edges.begin (); e != mp_edges.end (); ++e) {
    if ((*e)->is_outside ()) {
      return true;
    }
  }
  return false;
}
#endif

std::vector<db::GPolygon *>
GVertex::polygons () const
{
  std::set<db::GPolygon *> seen;
  std::vector<db::GPolygon *> res;
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
GVertex::has_edge (const GPolygonEdge *edge) const
{
  for (auto e = mp_edges.begin (); e != mp_edges.end (); ++e) {
    if (*e == edge) {
      return true;
    }
  }
  return false;
}

size_t
GVertex::num_edges (int max_count) const
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
GVertex::to_string (bool with_id) const
{
  std::string res = tl::sprintf ("(%.12g, %.12g)", x (), y());
  if (with_id) {
    res += tl::sprintf ("[%x]", (size_t)this);
  }
  return res;
}

int
GVertex::in_circle (const DPoint &point, const DPoint &center, double radius)
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
//  GPolygonEdge implementation

GPolygonEdge::GPolygonEdge ()
  : mp_v1 (0), mp_v2 (0), mp_left (), mp_right (), m_level (0), m_id (0), m_is_segment (false)
{
  // .. nothing yet ..
}

GPolygonEdge::GPolygonEdge (GVertex *v1, GVertex *v2)
  : mp_v1 (v1), mp_v2 (v2), mp_left (), mp_right (), m_level (0), m_id (0), m_is_segment (false)
{
  //  .. nothing yet ..
}

void
GPolygonEdge::set_left  (GPolygon *t)
{
  mp_left = t;
}

void
GPolygonEdge::set_right (GPolygon *t)
{
  mp_right = t;
}

void
GPolygonEdge::link ()
{
  mp_v1->mp_edges.push_back (this);
  m_ec_v1 = --mp_v1->mp_edges.end ();

  mp_v2->mp_edges.push_back (this);
  m_ec_v2 = --mp_v2->mp_edges.end ();
}

void
GPolygonEdge::unlink ()
{
  if (mp_v1) {
    mp_v1->remove_edge (m_ec_v1);
  }
  if (mp_v2) {
    mp_v2->remove_edge (m_ec_v2);
  }
  mp_v1 = mp_v2 = 0;
}

GPolygon *
GPolygonEdge::other (const GPolygon *t) const
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

GVertex *
GPolygonEdge::other (const GVertex *t) const
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
GPolygonEdge::has_vertex (const GVertex *v) const
{
  return mp_v1 == v || mp_v2 == v;
}

GVertex *
GPolygonEdge::common_vertex (const GPolygonEdge *other) const
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
GPolygonEdge::to_string (bool with_id) const
{
  std::string res = std::string ("(") + mp_v1->to_string (with_id) + ", " + mp_v2->to_string (with_id) + ")";
  if (with_id) {
    res += tl::sprintf ("[%x]", (size_t)this);
  }
  return res;
}

double
GPolygonEdge::distance (const db::DEdge &e, const db::DPoint &p)
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
GPolygonEdge::crosses (const db::DEdge &e, const db::DEdge &other)
{
  return e.side_of (other.p1 ()) * e.side_of (other.p2 ()) < 0 &&
         other.side_of (e.p1 ()) * other.side_of (e.p2 ()) < 0;
}

bool
GPolygonEdge::crosses_including (const db::DEdge &e, const db::DEdge &other)
{
  return e.side_of (other.p1 ()) * e.side_of (other.p2 ()) <= 0 &&
         other.side_of (e.p1 ()) * other.side_of (e.p2 ()) <= 0;
}

db::DPoint
GPolygonEdge::intersection_point (const db::DEdge &e, const db::DEdge &other)
{
  return e.intersect_point (other).second;
}

bool
GPolygonEdge::point_on (const db::DEdge &edge, const db::DPoint &point)
{
  if (edge.side_of (point) != 0) {
    return false;
  } else {
    return db::sprod_sign (point - edge.p1 (), edge.d ()) * db::sprod_sign(point - edge.p2 (), edge.d ()) < 0;
  }
}

#if 0 // @@@
bool
GPolygonEdge::is_for_outside_polygons () const
{
  return (left () && left ()->is_outside ()) || (right () && right ()->is_outside ());
}
#endif

bool
GPolygonEdge::has_polygon (const GPolygon *t) const
{
  return t != 0 && (left () == t || right () == t);
}

// -------------------------------------------------------------------------------------
//  GPolygon implementation

GPolygon::GPolygon ()
  : m_id (0)
{
  //  .. nothing yet ..
}

void
GPolygon::init ()
{
  m_id = 0;

  if (mp_e.empty ()) {
    return;
  }

  std::vector<GPolygonEdge *> e;
  e.swap (mp_e);

  std::multimap<db::GVertex *, GPolygonEdge *> v2e;

  for (auto i = e.begin (); i != e.end (); ++i) {
    if (i != e.begin ()) {
      v2e.insert (std::make_pair ((*i)->v1 (), *i));
      v2e.insert (std::make_pair ((*i)->v2 (), *i));
    }
  }

  mp_e.reserve (e.size ());
  mp_e.push_back (e.front ());
  //  NOTE: we assume the edges follow the clockwise orientation
  mp_e.back ()->set_right (this);

  mp_v.reserve (e.size ());
  mp_v.push_back (mp_e.back ()->v1 ());

  auto v = mp_e.back ()->v2 ();

  //  join the edges in the order of the polygon
  while (! v2e.empty ()) {

    mp_v.push_back (v);

    auto i = v2e.find (v);
    tl_assert (i != v2e.end () && i->first == v && i->second != mp_e.back ());
    v2e.erase (i);
    mp_e.push_back (i->second);
    //  NOTE: we assume the edges follow the clockwise orientation
    mp_e.back ()->set_right (this);

    v = i->second->other (v);
    i = v2e.find (v);
    while (i != v2e.end () && i->first == v) {
      if (i->second == mp_e.back ()) {
        v2e.erase (i);
        break;
      }
      ++i;
    }

  }
}

GPolygon::~GPolygon ()
{
  unlink ();
}

void
GPolygon::unlink ()
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
GPolygon::to_string (bool with_id) const
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
GPolygon::area () const
{
  return fabs (db::vprod (mp_e[0]->d (), mp_e[1]->d ())) * 0.5;
}

db::DBox
GPolygon::bbox () const
{
  db::DBox box;
  for (auto i = mp_v.begin (); i != mp_v.end (); ++i) {
    box += **i;
  }
  return box;
}

GPolygonEdge *
GPolygon::find_edge_with (const GVertex *v1, const GVertex *v2) const
{
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    if ((*e)->has_vertex (v1) && (*e)->has_vertex (v2)) {
      return *e;
    }
  }
  tl_assert (false);
}

GPolygonEdge *
GPolygon::common_edge (const GPolygon *other) const
{
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    if ((*e)->other (this) == other) {
      return *e;
    }
  }
  return 0;
}

#if 0 // @@@
int
GPolygon::contains (const db::DPoint &point) const
{
  auto c = *mp_v[2] - *mp_v[0];
  auto b = *mp_v[1] - *mp_v[0];

  int vps = db::vprod_sign (c, b);
  if (vps == 0) {
    return db::vprod_sign (point - *mp_v[0], b) == 0 && db::vprod_sign (point - *mp_v[0], c) == 0 ? 0 : -1;
  }

  int res = 1;

  const GVertex *vl = mp_v[2];
  for (int i = 0; i < 3; ++i) {
    const GVertex *v = mp_v[i];
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
#endif

double
GPolygon::min_edge_length () const
{
  double lmin = mp_e[0]->d ().length ();
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    lmin = std::min (lmin, (*e)->d ().length ());
  }
  return lmin;
}

#if 0 // @@@
double
GPolygon::b () const
{
  double lmin = min_edge_length ();
  bool ok = false;
  auto cr = circumcircle (&ok);
  return ok ? lmin / cr.second : 0.0;
}
#endif

bool
GPolygon::has_segment () const
{
  for (auto e = mp_e.begin (); e != mp_e.end (); ++e) {
    if ((*e)->is_segment ()) {
      return true;
    }
  }
  return false;
}

unsigned int
GPolygon::num_segments () const
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

static inline bool is_equal (const db::DPoint &a, const db::DPoint &b)
{
  return std::abs (a.x () - b.x ()) < std::max (1.0, (std::abs (a.x ()) + std::abs (b.x ()))) * db::epsilon &&
         std::abs (a.y () - b.y ()) < std::max (1.0, (std::abs (a.y ()) + std::abs (b.y ()))) * db::epsilon;
}

PolygonGraph::PolygonGraph ()
  : m_id (0)
  // @@@: m_is_constrained (false), m_level (0), m_id (0), m_flips (0), m_hops (0)
{
  //  .. nothing yet ..
}

PolygonGraph::~PolygonGraph ()
{
  clear ();
}

db::GVertex *
PolygonGraph::create_vertex (double x, double y)
{
  m_vertex_heap.push_back (db::GVertex (x, y));
  return &m_vertex_heap.back ();
}

db::GVertex *
PolygonGraph::create_vertex (const db::DPoint &pt)
{
  m_vertex_heap.push_back (pt);
  return &m_vertex_heap.back ();
}

db::GPolygonEdge *
PolygonGraph::create_edge (db::GVertex *v1, db::GVertex *v2)
{
  db::GPolygonEdge *edge = 0;

  if (! m_returned_edges.empty ()) {
    edge = m_returned_edges.back ();
    m_returned_edges.pop_back ();
    *edge = db::GPolygonEdge (v1, v2);
  } else {
    m_edges_heap.push_back (db::GPolygonEdge (v1, v2));
    edge = &m_edges_heap.back ();
  }

  edge->link ();
  edge->set_id (++m_id);
  return edge;
}

void
PolygonGraph::remove_polygon (db::GPolygon *poly)
{
  std::vector<db::GPolygonEdge *> edges;
  edges.reserve (poly->size ());
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

void
PolygonGraph::insert_polygon (const db::DPolygon &polygon)
{
  if (polygon.begin_edge ().at_end ()) {
    return;
  }

  std::vector<db::GPolygonEdge *> edges;

  for (unsigned int c = 0; c < polygon.holes () + 1; ++c) {
    const db::DPolygon::contour_type &ctr = polygon.contour (c);
    db::GVertex *v0 = 0, *vv, *v;
    for (auto p = ctr.begin (); p != ctr.end (); ++p) {
      v = create_vertex ((*p).x (), (*p).y ());
      if (! v0) {
        v0 = v;
      } else {
        edges.push_back (create_edge (vv, v));
      }
      vv = v;
    }
    if (v0 && v0 != v) {
      edges.push_back (create_edge (v, v0));
    }
  }

  create_polygon (edges.begin (), edges.end ());
}

std::string
PolygonGraph::to_string ()
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
PolygonGraph::bbox () const
{
  db::DBox box;
  for (auto t = mp_polygons.begin (); t != mp_polygons.end (); ++t) {
    box += t->bbox ();
  }
  return box;
}

#if 0 // @@@
bool
PolygonGraph::check (bool check_delaunay) const
{
  bool res = true;

  if (check_delaunay) {
    for (auto t = mp_polygons.begin (); t != mp_polygons.end (); ++t) {
      auto cp = t->circumcircle ();
      auto vi = find_inside_circle (cp.first, cp.second);
      if (! vi.empty ()) {
        res = false;
        tl::error << "(check error) polygon does not meet Delaunay criterion: " << t->to_string ();
        for (auto v = vi.begin (); v != vi.end (); ++v) {
          tl::error << "  vertex inside circumcircle: " << (*v)->to_string (true);
        }
      }
    }
  }

  for (auto t = mp_polygons.begin (); t != mp_polygons.end (); ++t) {
    for (int i = 0; i < 3; ++i) {
      if (! t->edge (i)->has_polygon (t.operator-> ())) {
        tl::error << "(check error) edges " << t->edge (i)->to_string (true)
                  << " attached to polygon " << t->to_string (true) << " does not refer to this polygon";
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
        tl::error << "(check error) edge " << e->to_string (true) << " splits an outside and inside polygon, but is not a segment";
        res = false;
      }
    }

    for (auto t = e->begin_polygons (); t != e->end_polygons (); ++t) {
      if (! t->has_edge (e.operator-> ())) {
        tl::error << "(check error) edge " << e->to_string (true) << " not found in adjacent polygon " << t->to_string (true);
        res = false;
      }
      if (! t->has_vertex (e->v1 ())) {
        tl::error << "(check error) edges " << e->to_string (true) << " vertex 1 not found in adjacent polygon " << t->to_string (true);
        res = false;
      }
      if (! t->has_vertex (e->v2 ())) {
        tl::error << "(check error) edges " << e->to_string (true) << " vertex 2 not found in adjacent polygon " << t->to_string (true);
        res = false;
      }
      db::GVertex *vopp = t->opposite (e.operator-> ());
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
#endif

db::Layout *
PolygonGraph::to_layout (bool decompose_by_id) const
{
  db::Layout *layout = new db::Layout ();
  layout->dbu (0.001);

  auto dbu_trans = db::CplxTrans (layout->dbu ()).inverted ();

  db::Cell &top = layout->cell (layout->add_cell ("DUMP"));
  unsigned int l1 = layout->insert_layer (db::LayerProperties (1, 0));
  // @@@ unsigned int l2 = layout->insert_layer (db::LayerProperties (2, 0));
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
    top.shapes (/*@@@t->is_outside () ? l2 :*/ l1).insert (dbu_trans * poly);
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
PolygonGraph::dump (const std::string &path, bool decompose_by_id) const
{
  std::unique_ptr<db::Layout> ly (to_layout (decompose_by_id));

  tl::OutputStream stream (path);

  db::SaveLayoutOptions opt;
  db::Writer writer (opt);
  writer.write (*ly, stream);

  tl::info << "PolygonGraph written to " << path;
}

void
PolygonGraph::clear ()
{
  mp_polygons.clear ();
  m_edges_heap.clear ();
  m_vertex_heap.clear ();
  m_returned_edges.clear ();
  // @@@m_is_constrained = false;
  // @@@m_level = 0;
  m_id = 0;
}

template<class Poly, class Trans>
void
PolygonGraph::make_contours (const Poly &poly, const Trans &trans, std::vector<std::vector<db::GVertex *> > &edge_contours)
{
  edge_contours.push_back (std::vector<db::GVertex *> ());
  for (auto pt = poly.begin_hull (); pt != poly.end_hull (); ++pt) {
    edge_contours.back ().push_back (insert_point (trans * *pt));
  }

  for (unsigned int h = 0; h < poly.holes (); ++h) {
    edge_contours.push_back (std::vector<db::GVertex *> ());
    for (auto pt = poly.begin_hole (h); pt != poly.end_hole (h); ++pt) {
      edge_contours.back ().push_back (insert_point (trans * *pt));
    }
  }
}

}

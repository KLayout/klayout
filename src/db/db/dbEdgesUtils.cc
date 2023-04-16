
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

#include "dbEdgesUtils.h"
#include "dbRegion.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  JoinEdgesCluster implementation

JoinEdgesCluster::JoinEdgesCluster (db::PolygonSink *output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i)
  : mp_output (output), m_ext_b (ext_b), m_ext_e (ext_e), m_ext_o (ext_o), m_ext_i (ext_i)
{
  //  .. nothing yet ..
}

void
JoinEdgesCluster::finish ()
{
  std::multimap<db::Point, iterator> objects_by_p1;
  std::multimap<db::Point, iterator> objects_by_p2;
  for (iterator o = begin (); o != end (); ++o) {
    if (o->first->p1 () != o->first->p2 ()) {
      objects_by_p1.insert (std::make_pair (o->first->p1 (), o));
      objects_by_p2.insert (std::make_pair (o->first->p2 (), o));
    }
  }

  while (! objects_by_p2.empty ()) {

    tl_assert (! objects_by_p1.empty ());

    //  Find the beginning of a new sequence
    std::multimap<db::Point, iterator>::iterator j0 = objects_by_p1.begin ();
    std::multimap<db::Point, iterator>::iterator j = j0;
    do {
      std::multimap<db::Point, iterator>::iterator jj = objects_by_p2.find (j->first);
      if (jj == objects_by_p2.end ()) {
        break;
      } else {
        j = objects_by_p1.find (jj->second->first->p1 ());
        tl_assert (j != objects_by_p1.end ());
      }
    } while (j != j0);

    iterator i = j->second;

    //  determine a sequence
    //  TODO: this chooses any solution in case of forks. Choose a specific one?
    std::vector<db::Point> pts;
    pts.push_back (i->first->p1 ());

    do {

      //  record the next point
      pts.push_back (i->first->p2 ());

      //  remove the edge as it's taken
      std::multimap<db::Point, iterator>::iterator jj;
      for (jj = objects_by_p2.find (i->first->p2 ()); jj != objects_by_p2.end () && jj->first == i->first->p2 (); ++jj) {
        if (jj->second == i) {
          break;
        }
      }
      tl_assert (jj != objects_by_p2.end () && jj->second == i);
      objects_by_p2.erase (jj);
      objects_by_p1.erase (j);

      //  process along the edge to the next one
      //  TODO: this chooses any solution in case of forks. Choose a specific one?
      j = objects_by_p1.find (i->first->p2 ());
      if (j != objects_by_p1.end ()) {
        i = j->second;
      } else {
        break;
      }

    } while (true);

    bool cyclic = (pts.back () == pts.front ());

    if (! cyclic) {

      //  non-cyclic sequence
      db::Path path (pts.begin (), pts.end (), 0, m_ext_b, m_ext_e, false);
      std::vector<db::Point> hull;
      path.hull (hull, m_ext_o, m_ext_i);
      db::Polygon poly;
      poly.assign_hull (hull.begin (), hull.end ());
      mp_output->put (poly);

    } else {

      //  we have a loop: form a contour by using the polygon size functions and a "Not" to form the hole
      db::Polygon poly;
      poly.assign_hull (pts.begin (), pts.end ());

      db::EdgeProcessor ep;
      db::PolygonGenerator pg (*mp_output, false, true);

      int mode_a = -1, mode_b = -1;

      if (m_ext_o == 0) {
        ep.insert (poly, 0);
      } else {
        db::Polygon sized_poly (poly);
        sized_poly.size (m_ext_o, m_ext_o, 2 /*sizing mode*/);
        ep.insert (sized_poly, 0);
        mode_a = 1;
      }

      if (m_ext_i == 0) {
        ep.insert (poly, 1);
      } else {
        db::Polygon sized_poly (poly);
        sized_poly.size (-m_ext_i, -m_ext_i, 2 /*sizing mode*/);
        ep.insert (sized_poly, 1);
        mode_b = 1;
      }

      db::BooleanOp2 op (db::BooleanOp::ANotB, mode_a, mode_b);
      ep.process (pg, op);

    }

  }
}

// -------------------------------------------------------------------------------------------------------------
//  extended_edge implementation

db::Polygon
extended_edge (const db::Edge &edge, db::Coord ext_b, db::Coord ext_e, db::Coord ext_o, db::Coord ext_i)
{
  db::DVector d;
  if (edge.is_degenerate ()) {
    d = db::DVector (1.0, 0.0);
  } else {
    d = db::DVector (edge.d ()) * (1.0 / edge.double_length ());
  }

  db::DVector n (-d.y (), d.x ());

  db::Point pts[4] = {
    db::Point (db::DPoint (edge.p1 ()) - d * double (ext_b) + n * double (ext_o)),
    db::Point (db::DPoint (edge.p2 ()) + d * double (ext_e) + n * double (ext_o)),
    db::Point (db::DPoint (edge.p2 ()) + d * double (ext_e) - n * double (ext_i)),
    db::Point (db::DPoint (edge.p1 ()) - d * double (ext_b) - n * double (ext_i)),
  };

  db::Polygon poly;
  poly.assign_hull (pts + 0, pts + 4);
  return poly;
}

// -------------------------------------------------------------------------------------------------------------
//  EdgeSegmentSelector processor

EdgeSegmentSelector::EdgeSegmentSelector (int mode, Edge::distance_type length, double fraction)
  : m_mode (mode), m_length (length), m_fraction (fraction)
{ }

EdgeSegmentSelector::~EdgeSegmentSelector ()
{ }

void
EdgeSegmentSelector::process (const db::Edge &edge, std::vector<db::Edge> &res) const
{
  double l = std::max (edge.double_length () * m_fraction, double (m_length));

  if (m_mode < 0) {

    res.push_back (db::Edge (edge.p1 (), db::Point (db::DPoint (edge.p1 ()) + db::DVector (edge.d ()) * (l / edge.double_length ()))));

  } else if (m_mode > 0) {

    res.push_back (db::Edge (db::Point (db::DPoint (edge.p2 ()) - db::DVector (edge.d ()) * (l / edge.double_length ())), edge.p2 ()));

  } else {

    db::DVector dl = db::DVector (edge.d ()) * (0.5 * l / edge.double_length ());
    db::DPoint center = db::DPoint (edge.p1 ()) + db::DVector (edge.p2 () - edge.p1 ()) * 0.5;

    res.push_back (db::Edge (db::Point (center - dl), db::Point (center + dl)));

  }
}

// -------------------------------------------------------------------------------------------------------------
//  EdgeAngleChecker implementation

EdgeAngleChecker::EdgeAngleChecker (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end)
{
  m_t_start = db::CplxTrans(1.0, angle_start, false, db::DVector ());
  m_t_end = db::CplxTrans(1.0, angle_end, false, db::DVector ());

  m_include_start = include_angle_start;
  m_include_end = include_angle_end;

  m_big_angle = (angle_end - angle_start + db::epsilon) > 180.0;
  m_all = (angle_end - angle_start - db::epsilon) > 360.0;
}

bool
EdgeAngleChecker::check (const db::Vector &a, const db::Vector &b) const
{
  db::DVector vout (b);

  db::DVector v1 = m_t_start * a;
  db::DVector v2 = m_t_end * a;

  int vps1 = db::vprod_sign (v1, vout);
  int vps2 = db::vprod_sign (v2, vout);
  bool opp1 = vps1 == 0 && (db::sprod_sign (v1, vout) < 0);
  bool opp2 = vps2 == 0 && (db::sprod_sign (v2, vout) < 0);

  bool vp1 = !opp1 && (m_include_start ? (db::vprod_sign (v1, vout) >= 0) : (db::vprod_sign (v1, vout) > 0));
  bool vp2 = !opp2 && (m_include_end ? (db::vprod_sign (v2, vout) <= 0) : (db::vprod_sign (v2, vout) < 0));

  if (m_big_angle && (vp1 || vp2)) {
    return true;
  } else if (! m_big_angle && vp1 && vp2) {
    return true;
  } else {
    return false;
  }
}

// -------------------------------------------------------------------------------------------------------------
//  EdgeOrientationFilter implementation

EdgeOrientationFilter::EdgeOrientationFilter (double amin, bool include_amin, double amax, bool include_amax, bool inverse)
  : m_inverse (inverse), m_checker (amin, include_amin, amax, include_amax)
{
  //  .. nothing yet ..
}

EdgeOrientationFilter::EdgeOrientationFilter (double a, bool inverse)
  : m_inverse (inverse), m_checker (a, true, a, true)
{
  //  .. nothing yet ..
}

bool
EdgeOrientationFilter::selected (const db::Edge &edge) const
{
  //  NOTE: this edge normalization confines the angle to a range between (-90 .. 90] (-90 excluded).
  //  A horizontal edge has 0 degree, a vertical one has 90 degree.
  if (edge.dx () < 0 || (edge.dx () == 0 && edge.dy () < 0)) {
    return m_checker (db::Vector (edge.ortho_length (), 0), -edge.d ()) != m_inverse;
  } else {
    return m_checker (db::Vector (edge.ortho_length (), 0), edge.d ()) != m_inverse;
  }
}

// -------------------------------------------------------------------------------------------------------------
//  SpecialEdgeOrientationFilter implementation

SpecialEdgeOrientationFilter::SpecialEdgeOrientationFilter (FilterType type, bool inverse)
  : m_type (type), m_inverse (inverse)
{
  //  .. nothing yet ..
}

static EdgeAngleChecker s_ortho_checkers [] = {
  EdgeAngleChecker (0.0, true, 0.0, true),
  EdgeAngleChecker (90.0, true, 90.0, true)
};

static EdgeAngleChecker s_diagonal_checkers [] = {
  EdgeAngleChecker (-45.0, true, -45.0, true),
  EdgeAngleChecker (45.0, true, 45.0, true)
};

static EdgeAngleChecker s_orthodiagonal_checkers [] = {
  EdgeAngleChecker (-45.0, true, -45.0, true),
  EdgeAngleChecker (0.0, true, 0.0, true),
  EdgeAngleChecker (45.0, true, 45.0, true),
  EdgeAngleChecker (90.0, true, 90.0, true)
};

bool
SpecialEdgeOrientationFilter::selected (const db::Edge &edge) const
{
  const EdgeAngleChecker *eb, *ee;

  switch (m_type) {
  case Ortho:
    eb = s_ortho_checkers;
    ee = s_ortho_checkers + sizeof (s_ortho_checkers) / sizeof (s_ortho_checkers [0]);
    break;
  case Diagonal:
    eb = s_diagonal_checkers;
    ee = s_diagonal_checkers + sizeof (s_diagonal_checkers) / sizeof (s_diagonal_checkers [0]);
    break;
  case OrthoDiagonal:
  default:
    eb = s_orthodiagonal_checkers;
    ee = s_orthodiagonal_checkers + sizeof (s_orthodiagonal_checkers) / sizeof (s_orthodiagonal_checkers [0]);
    break;
  }

  db::Vector en, ev;
  en = db::Vector (edge.ortho_length (), 0);

  //  NOTE: this edge normalization confines the angle to a range between (-90 .. 90] (-90 excluded).
  //  A horizontal edge has 0 degree, a vertical one has 90 degree.
  if (edge.dx () < 0 || (edge.dx () == 0 && edge.dy () < 0)) {
    ev = -edge.d ();
  } else {
    ev = edge.d ();
  }

  for (auto ec = eb; ec != ee; ++ec) {
    if ((*ec) (en, ev)) {
      return ! m_inverse;
    }
  }

  return m_inverse;
}

// -------------------------------------------------------------------------------------------------------------
//  Edge to Edge relation implementation

bool edge_interacts (const db::Edge &a, const db::Edge &b)
{
  return a.intersect (b);
}

bool edge_is_inside (const db::Edge &a, const db::Edge &b)
{
  return b.contains (a.p1 ()) && b.contains (a.p2 ());
}

bool edge_is_outside (const db::Edge &a, const db::Edge &b)
{
  if (a.parallel (b)) {
    return ! a.coincident (b);
  } else {
    auto pt = a.intersect_point (b);
    if (! pt.first) {
      //  no intersection -> outside
      return true;
    }
    return ! b.contains_excl (pt.second) || ! a.contains_excl (pt.second);
  }
}

// -------------------------------------------------------------------------------------------------------------
//  Edge to Polygon relation implementation

bool edge_interacts (const db::Edge &a, const db::Polygon &b)
{
  return db::interact (b, a);
}

namespace {

struct DetectTagEdgeSink
  : public db::EdgeSink
{
  DetectTagEdgeSink (int tag)
    : fail_tag (tag), result (true) { }

  virtual void put (const db::Edge &) { }

  virtual void put (const db::Edge &, int tag)
  {
    if (tag == fail_tag) {
      result = false;
      request_stop ();
    }
  }

  int fail_tag;
  bool result;
};

}

static bool
edge_is_inside_or_outside (bool outside, const db::Edge &a, const db::Polygon &b)
{
  db::EdgeProcessor ep;
  ep.insert (b, 0);

  ep.insert (a, 1);

  DetectTagEdgeSink es (outside ? 1 : 2);   //  2 is the "outside" tag in "Both" mode -> this makes inside fail
  db::EdgePolygonOp op (db::EdgePolygonOp::Both, true /*include borders*/);
  ep.process (es, op);

  return es.result;
}

bool edge_is_inside (const db::Edge &a, const db::Polygon &b)
{
  //  shortcuts
  if (!a.bbox ().inside (b.box ())) {
    return false;
  }

  return edge_is_inside_or_outside (false, a, b);
}

bool edge_is_outside (const db::Edge &a, const db::Polygon &b)
{
  //  shortcuts
  if (! a.bbox ().overlaps (b.box ())) {
    return true;
  }

  return edge_is_inside_or_outside (true, a, b);
}

}

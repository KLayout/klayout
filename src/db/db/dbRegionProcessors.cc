
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "dbRegionProcessors.h"
#include "dbPolygon.h"
#include "dbPolygonGenerators.h"

namespace db
{

// -----------------------------------------------------------------------------------
//  CornerDetectorCore implementation

CornerDetectorCore::CornerDetectorCore (double angle_start, bool include_angle_start, double angle_end, bool include_angle_end)
  : m_checker (angle_start, include_angle_start, angle_end, include_angle_end)
{
  //  .. nothing yet ..
}

void CornerDetectorCore::detect_corners (const db::Polygon &poly, const CornerPointDelivery &delivery) const
{
  size_t n = poly.holes () + 1;
  for (size_t i = 0; i < n; ++i) {

    const db::Polygon::contour_type &ctr = poly.contour (int (i));
    size_t nn = ctr.size ();
    if (nn > 2) {

      db::Point pp = ctr [nn - 2];
      db::Point pt = ctr [nn - 1];
      for (size_t j = 0; j < nn; ++j) {

        db::Point pn = ctr [j];

        if (m_checker (pt - pp, pn - pt)) {
          delivery.make_point (pt, db::Edge (pp, pt), db::Edge (pt, pn));
        }

        pp = pt;
        pt = pn;

      }

    }

  }
}

// -----------------------------------------------------------------------------------
//  Extents implementation

void Extents::process (const db::Polygon &poly, std::vector<db::Polygon> &result) const
{
  db::Box b = poly.box ();
  if (! b.empty ()) {
    result.push_back (db::Polygon (b));
  }
}

// -----------------------------------------------------------------------------------
//  RelativeExtents implementation

void RelativeExtents::process (const db::Polygon &poly, std::vector<db::Polygon> &result) const
{
  db::Box b = poly.box ();
  db::Point p1 (b.left () + db::coord_traits<db::Coord>::rounded (m_fx1 * b.width ()),
                b.bottom () + db::coord_traits<db::Coord>::rounded (m_fy1 * b.height ()));
  db::Point p2 (b.left () + db::coord_traits<db::Coord>::rounded (m_fx2 * b.width ()),
                b.bottom () + db::coord_traits<db::Coord>::rounded (m_fy2 * b.height ()));
  db::Box box = db::Box (p1, p2).enlarged (db::Vector (m_dx, m_dy));
  if (! box.empty ()) {
    result.push_back (db::Polygon (box));
  }
}

const TransformationReducer *RelativeExtents::vars () const
{
  if (m_dx == 0 && m_dy == 0 && fabs (m_fx1) < db::epsilon && fabs (m_fy1) < db::epsilon && fabs (1.0 - m_fx2) < db::epsilon && fabs (1.0 - m_fy2) < db::epsilon) {
    return 0;
  } else if (m_dx == m_dy && fabs (m_fx1 - m_fy1) < db::epsilon && fabs (1.0 - (m_fx1 + m_fx2)) < db::epsilon  && fabs (m_fx2 - m_fy2) < db::epsilon && fabs (1.0 - (m_fy1 + m_fy2)) < db::epsilon) {
    return & m_isotropic_reducer;
  } else {
    return & m_anisotropic_reducer;
  }
}

// -----------------------------------------------------------------------------------
//  RelativeExtentsAsEdges implementation

void RelativeExtentsAsEdges::process (const db::Polygon &poly, std::vector<db::Edge> &result) const
{
  db::Box b = poly.box ();
  db::Point p1 (b.left () + db::coord_traits<db::Coord>::rounded (m_fx1 * b.width ()),
                b.bottom () + db::coord_traits<db::Coord>::rounded (m_fy1 * b.height ()));
  db::Point p2 (b.left () + db::coord_traits<db::Coord>::rounded (m_fx2 * b.width ()),
                b.bottom () + db::coord_traits<db::Coord>::rounded (m_fy2 * b.height ()));
  result.push_back (db::Edge (p1, p2));
}

const TransformationReducer *RelativeExtentsAsEdges::vars () const
{
  if (fabs (m_fx1) < db::epsilon && fabs (m_fy1) < db::epsilon && fabs (1.0 - m_fx2) < db::epsilon && fabs (1.0 - m_fy2) < db::epsilon) {
    return 0;
  } else if (fabs (m_fx1 - m_fy1) < db::epsilon && fabs (1.0 - (m_fx1 + m_fx2)) < db::epsilon  && fabs (m_fx2 - m_fy2) < db::epsilon && fabs (1.0 - (m_fy1 + m_fy2)) < db::epsilon) {
    return & m_isotropic_reducer;
  } else {
    return & m_anisotropic_reducer;
  }
}

bool RelativeExtentsAsEdges::result_must_not_be_merged () const
{
  //  don't merge if the results will just be points
  return (fabs (m_fx1 - m_fx2) < db::epsilon && fabs (m_fy1 - m_fy2) < db::epsilon);
}

// -----------------------------------------------------------------------------------
//  PolygonToEdgeProcessor implementation

PolygonToEdgeProcessor::PolygonToEdgeProcessor (PolygonToEdgeProcessor::EdgeMode mode)
  : m_mode (mode)
{
  //  .. nothing yet ..
}

inline void
next (db::Polygon::contour_type::simple_iterator &iter, const db::Polygon::contour_type &contour)
{
  if (++iter == contour.end ()) {
    iter = contour.begin ();
  }
}

static void
contour_to_edges (const db::Polygon::contour_type &contour, PolygonToEdgeProcessor::EdgeMode mode, std::vector<db::Edge> &result)
{
  if (contour.size () < 3) {
    return;
  }

  db::Polygon::contour_type::simple_iterator pm1 = contour.begin ();
  db::Polygon::contour_type::simple_iterator p0 = pm1;
  next (p0, contour);
  db::Polygon::contour_type::simple_iterator p1 = p0;
  next (p1, contour);
  db::Polygon::contour_type::simple_iterator p2 = p1;
  next (p2, contour);

  while (pm1 != contour.end ()) {

    int s1 = db::vprod_sign (*p0 - *pm1, *p1 - *p0);
    int s2 = db::vprod_sign (*p1 - *p0, *p2 - *p1);

    bool take = true;

    switch (mode) {
    case PolygonToEdgeProcessor::All:
    default:
      break;
    case PolygonToEdgeProcessor::Convex:
      take = s1 < 0 && s2 < 0;
      break;
    case PolygonToEdgeProcessor::NotConvex:
      take = ! (s1 < 0 && s2 < 0);
      break;
    case PolygonToEdgeProcessor::Concave:
      take = s1 > 0 && s2 > 0;
      break;
    case PolygonToEdgeProcessor::NotConcave:
      take = ! (s1 > 0 && s2 > 0);
      break;
    case PolygonToEdgeProcessor::StepOut:
      take = s1 > 0 && s2 < 0;
      break;
    case PolygonToEdgeProcessor::NotStepOut:
      take = ! (s1 > 0 && s2 < 0);
      break;
    case PolygonToEdgeProcessor::StepIn:
      take = s1 < 0 && s2 > 0;
      break;
    case PolygonToEdgeProcessor::NotStepIn:
      take = ! (s1 < 0 && s2 > 0);
      break;
    case PolygonToEdgeProcessor::Step:
      take = s1 * s2 < 0;
      break;
    case PolygonToEdgeProcessor::NotStep:
      take = ! (s1 * s2 < 0);
      break;
    }

    if (take) {
      result.push_back (db::Edge (*p0, *p1));
    }

    ++pm1;
    next (p0, contour);
    next (p1, contour);
    next (p2, contour);

  }
}

void PolygonToEdgeProcessor::process (const db::Polygon &poly, std::vector<db::Edge> &result) const
{
  if (m_mode == All) {

    for (db::Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
      result.push_back (*e);
    }

  } else {

    for (unsigned int i = 0; i < poly.holes () + 1; ++i) {
      contour_to_edges (poly.contour (i), m_mode, result);
    }

  }
}

// -----------------------------------------------------------------------------------
//  ConvexDecomposition implementation

void ConvexDecomposition::process (const db::Polygon &poly, std::vector<db::Polygon> &result) const
{
  db::SimplePolygonContainer sp;
  db::decompose_convex (poly, m_mode, sp);
  for (std::vector <db::SimplePolygon>::const_iterator i = sp.polygons ().begin (); i != sp.polygons ().end (); ++i) {
    result.push_back (db::simple_polygon_to_polygon (*i));
  }
}

// -----------------------------------------------------------------------------------
//  TrapezoidDecomposition implementation

void TrapezoidDecomposition::process (const db::Polygon &poly, std::vector<db::Polygon> &result) const
{
  db::SimplePolygonContainer sp;
  db::decompose_trapezoids (poly, m_mode, sp);
  for (std::vector <db::SimplePolygon>::const_iterator i = sp.polygons ().begin (); i != sp.polygons ().end (); ++i) {
    result.push_back (db::simple_polygon_to_polygon (*i));
  }
}

// -----------------------------------------------------------------------------------
//  PolygonBreaker implementation

void PolygonBreaker::process (const db::Polygon &poly, std::vector<db::Polygon> &result) const
{
  if ((m_max_vertex_count > 0 && poly.vertices () > m_max_vertex_count) ||
      (m_max_area_ratio > 0 && poly.area_ratio () > m_max_area_ratio)) {

    std::vector<db::Polygon> split_polygons;
    db::split_polygon (poly, split_polygons);
    for (std::vector<db::Polygon>::const_iterator p = split_polygons.begin (); p != split_polygons.end (); ++p) {
      process (*p, result);
    }

  } else {
    result.push_back (poly);
  }
}

// -----------------------------------------------------------------------------------
//  PolygonSizer implementation

PolygonSizer::PolygonSizer (db::Coord dx, db::Coord dy, unsigned int mode)
  : m_dx (dx), m_dy (dy), m_mode (mode)
{
  if (dx == dy) {
    m_vars = new db::MagnificationReducer ();
  } else {
    m_vars = new db::XYAnisotropyAndMagnificationReducer ();
  }
}

PolygonSizer::~PolygonSizer ()
{
  delete m_vars;
}

void PolygonSizer::process (const db::Polygon &poly, std::vector<db::Polygon> &result) const
{
  db::PolygonContainer pr (result);
  db::PolygonGenerator pg2 (pr, false /*don't resolve holes*/, true /*min. coherence*/);
  db::SizingPolygonFilter siz (pg2, m_dx, m_dy, m_mode);
  siz.put (poly);
}

bool PolygonSizer::result_is_merged () const
{
  return (m_dx < 0 && m_dy < 0);
}

// -----------------------------------------------------------------------------------
//  TriangulationProcessor implementation

//  some typical value to translate the values into "order of 1"
const double triangulation_dbu = 0.001;

TriangulationProcessor::TriangulationProcessor (double max_area, double min_b)
{
  m_param.max_area = max_area * triangulation_dbu * triangulation_dbu;
  m_param.base_verbosity = 40;
  m_param.min_length = 2 * triangulation_dbu;
  m_param.min_b = min_b;
}

void
TriangulationProcessor::process (const db::Polygon &poly, std::vector<db::Polygon> &result) const
{
  //  NOTE: we center the polygon for better numerical stability
  db::CplxTrans trans = db::CplxTrans (triangulation_dbu) * db::ICplxTrans (db::Trans (db::Point () - poly.box ().center ()));

  db::Triangles tri;
  tri.triangulate (poly, m_param, trans);

  db::Point pts [3];
  auto trans_inv = trans.inverted ();

  for (auto t = tri.begin (); t != tri.end (); ++t) {
    for (int i = 0; i < 3; ++i) {
      pts [i] = trans_inv * *t->vertex (i);
    }
    result.push_back (db::Polygon ());
    result.back ().assign_hull (pts + 0, pts + 3);
  }
}

// -----------------------------------------------------------------------------------
//  DRCHullProcessor implementation

DRCHullProcessor::DRCHullProcessor (db::Coord d, db::metrics_type metrics, size_t n_circle)
  : m_d (d), m_metrics (metrics), m_n_circle (n_circle)
{
  //  .. nothing yet ..
}

static void create_edge_segment_euclidian (std::vector<db::Point> &points, const db::Edge &e, const db::Edge &ee, db::Coord dist, size_t n_circle)
{
  db::Vector d (e.d ());
  db::Vector n (-d.y (), d.x ());

  db::Vector dd (ee.d ());
  db::Vector nn (-dd.y (), dd.x ());

  if ((d.x () == 0 && d.y () == 0) || (dd.x () == 0 && dd.y () == 0)) {
    //  should not happen
    return;
  }

  double f = dist / n.double_length ();
  double ff = dist / nn.double_length ();

  points.push_back (e.p1 () + db::Vector (n * f));
  points.push_back (e.p2 () + db::Vector (n * f));

  if (db::vprod_sign (nn, n) < 0) {

    //  concave corner
    points.push_back (e.p2 ());
    points.push_back (e.p2 () + db::Vector (nn * ff));

  } else {

    double amax;
    if (db::vprod_sign (nn, n) == 0) {
      amax = db::sprod_sign (nn, n) < 0 ? M_PI : 0.0;
    } else {
      amax = atan2 (db::vprod (nn, n), db::sprod (nn, n));
    }

    double da = M_PI * 2.0 / n_circle;
    double f2 = f / cos (0.5 * da);

    int na = int (floor (amax / da + db::epsilon));
    double a0 = 0.5 * (amax - da * (na - 1));

    for (int i = 0; i < na; ++i) {
      double a = i * da + a0;
      points.push_back (e.p2 () + db::Vector (d * (f2 * sin (a)) + n * (f2 * cos (a))));
    }

  }
}

static void create_edge_segment_square (std::vector<db::Point> &points, const db::Edge &e, db::Coord dist)
{
  db::Vector d (e.d ());
  db::Vector n (-d.y (), d.x ());

  if (d.x () == 0 && d.y () == 0) {
    return;
  }

  double f = dist / n.double_length ();

  points.push_back (e.p1 ());
  points.push_back (e.p1 () + db::Vector (d * -f));
  points.push_back (e.p1 () + db::Vector (d * -f + n * f));
  points.push_back (e.p2 () + db::Vector (d * f + n * f));
  points.push_back (e.p2 () + db::Vector (d * f));
}

static void create_edge_segment_projection (std::vector<db::Point> &points, const db::Edge &e, db::Coord dist)
{
  db::Vector d (e.d ());
  db::Vector n (-d.y (), d.x ());

  if (d.x () == 0 && d.y () == 0) {
    return;
  }

  double f = dist / n.double_length ();

  points.push_back (e.p1 ());
  points.push_back (e.p1 () + db::Vector (n * f));
  points.push_back (e.p2 () + db::Vector (n * f));
}

static void create_edge_segment (std::vector<db::Point> &points, db::metrics_type metrics, const db::Edge &e, const db::Edge &ee, db::Coord d, size_t n_circle)
{
  if (metrics == db::Euclidian) {
    create_edge_segment_euclidian (points, e, ee, d, n_circle);
  } else if (metrics == db::Square) {
    create_edge_segment_square (points, e, d);
  } else if (metrics == db::Projection) {
    create_edge_segment_projection (points, e, d);
  }
}

void
DRCHullProcessor::process (const db::Polygon &poly, std::vector<db::Polygon> &result) const
{
  db::EdgeProcessor ep;
  std::vector<db::Point> points;

  for (unsigned int i = 0; i < poly.holes () + 1; ++i) {

    points.clear ();

    auto c = poly.contour (i);
    if (c.size () < 2) {
      continue;
    }

    for (auto p = c.begin (); p != c.end (); ++p) {

      auto pp = p;
      if (++pp == c.end ()) {
        pp = c.begin ();
      }

      auto ppp = pp;
      if (++ppp == c.end ()) {
        ppp = c.begin ();
      }

      create_edge_segment (points, m_metrics, db::Edge (*p, *pp), db::Edge (*pp, *ppp), m_d, m_n_circle);

    }

    for (auto p = points.begin (); p != points.end (); ++p) {

      auto pp = p;
      if (++ pp == points.end ()) {
        pp = points.begin ();
      }

      ep.insert (db::Edge (*p, *pp));

    }

  }

  db::SimpleMerge op;
  db::PolygonContainer psink (result);
  db::PolygonGenerator pg (psink, false);
  ep.process (pg, op);
}

}

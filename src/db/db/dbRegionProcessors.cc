
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

void PolygonToEdgeProcessor::process (const db::Polygon &poly, std::vector<db::Edge> &result) const
{
  for (db::Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
    result.push_back (*e);
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

}

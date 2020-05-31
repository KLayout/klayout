
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

CornerDetectorCore::CornerDetectorCore (double angle_start, double angle_end)
{
  m_t_start = db::CplxTrans(1.0, angle_start, false, db::DVector ());
  m_t_end = db::CplxTrans(1.0, angle_end, false, db::DVector ());

  m_big_angle = (angle_end - angle_start + db::epsilon) > 180.0;
  m_all = (angle_end - angle_start - db::epsilon) > 360.0;
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

        if (m_all) {
          delivery.make_point (pt);
        } else {

          db::Vector vin (pt - pp);
          db::DVector vout (pn - pt);

          db::DVector v1 = m_t_start * vin;
          db::DVector v2 = m_t_end * vin;

          bool vp1 = db::vprod_sign (v1, vout) >= 0;
          bool vp2 = db::vprod_sign (v2, vout) <= 0;

          if (m_big_angle && (vp1 || vp2)) {
            delivery.make_point (pt);
          } else if (! m_big_angle && vp1 && vp2) {
            if (db::sprod_sign (v1, vout) > 0 && db::sprod_sign (v2, vout) > 0) {
              delivery.make_point (pt);
            }
          }

        }

        pp = pt;
        pt = pn;

      }

    }

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
  if (m_dx == 0 && m_dy == 0 && fabs (m_fx1) < db::epsilon && fabs (m_fy1) < db::epsilon && fabs (m_fx2) < db::epsilon && fabs (m_fy2) < db::epsilon) {
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
  return & m_anisotropic_reducer;
}

bool RelativeExtentsAsEdges::result_must_not_be_merged () const
{
  //  don't merge if the results will just be points
  return (fabs (m_fx1 - m_fx2) < db::epsilon && fabs (m_fy1 - m_fy2) < db::epsilon);
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

}

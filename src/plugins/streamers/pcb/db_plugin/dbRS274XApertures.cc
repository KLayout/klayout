
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


#include "dbRS274XApertures.h"
#include "dbRS274XReader.h"
#include "dbPolygonTools.h"

namespace db
{

// -----------------------------------------------------------------------------
//  RS274ApertureBase implementation

RS274XApertureBase::RS274XApertureBase ()
  : mp_ep (0), mp_reader (0), m_needs_update (true)
{ 
  // .. nothing yet ..
}

void
RS274XApertureBase::produce_flash (const db::DCplxTrans &d, RS274XReader &reader, db::EdgeProcessor &ep, bool clear)
{
  if (m_needs_update) {

    mp_reader = &reader;
    mp_ep = &ep;

    m_lines.clear ();
    m_polygons.clear ();
    m_clear_polygons.clear ();

    do_produce_flash ();

    if (! m_clear_polygons.empty ()) {
      std::vector<db::Polygon> input;
      m_polygons.swap (input);
      ep.boolean (input, m_clear_polygons, m_polygons, db::BooleanOp::ANotB, false, true);
      m_clear_polygons.clear ();
    }

    m_needs_update = false;

    mp_reader = 0;
    mp_ep = 0;

  }

  db::CplxTrans trans = d * db::CplxTrans (reader.dbu ());

  for (std::vector <db::Polygon>::const_iterator p = m_polygons.begin (); p != m_polygons.end (); ++p) {
    reader.produce_polygon (p->transformed (trans), clear);
  }
  for (std::vector <db::Path>::const_iterator p = m_lines.begin (); p != m_lines.end (); ++p) {
    reader.produce_line (p->transformed (trans), clear);
  }
}

void 
RS274XApertureBase::produce_linear (const db::DCplxTrans &d, const db::DVector &dist, RS274XReader &reader, db::EdgeProcessor &ep, bool clear)
{
  mp_reader = &reader;
  mp_ep = &ep;

  std::vector<db::Polygon> p, cp;
  std::vector<db::Path> l;

  l.swap (m_lines);
  p.swap (m_polygons);
  cp.swap (m_clear_polygons);

  db::DPoint from;
  db::DPoint to = from + d.inverted () * dist;

  if (! do_produce_linear (from, to)) {

    // fallback: produce flash and employ a Minkowski sum to generate the resulting structure
    do_produce_flash ();

    double dbu = mp_reader->dbu ();
    db::Point ifrom (db::coord_traits<db::Coord>::rounded (from.x () / dbu), db::coord_traits<db::Coord>::rounded (from.y () / dbu));
    db::Point ito (db::coord_traits<db::Coord>::rounded (to.x () / dbu), db::coord_traits<db::Coord>::rounded (to.y () / dbu));

    std::vector<db::Polygon> p;

    if (! m_clear_polygons.empty ()) {
      ep.boolean (m_polygons, m_clear_polygons, p, db::BooleanOp::ANotB, false, true);
      m_clear_polygons.clear ();
      m_polygons.clear ();
    } else {
      m_polygons.swap (p);
    }

    for (std::vector<db::Polygon>::const_iterator f = p.begin (); f != p.end (); ++f) {
      m_polygons.push_back (db::minkowski_sum (*f, db::Edge (ifrom, ito), true /*resolve holes*/));
    }
    
  }

  if (! m_clear_polygons.empty ()) {
    std::vector<db::Polygon> input;
    m_polygons.swap (input);
    ep.boolean (input, m_clear_polygons, m_polygons, db::BooleanOp::ANotB, false, true);
    m_clear_polygons.clear ();
  }

  db::CplxTrans trans = d * db::CplxTrans (mp_reader->dbu ());

  for (std::vector <db::Polygon>::const_iterator p = m_polygons.begin (); p != m_polygons.end (); ++p) {
    mp_reader->produce_polygon (p->transformed (trans), clear);
  }
  for (std::vector <db::Path>::const_iterator p = m_lines.begin (); p != m_lines.end (); ++p) {
    mp_reader->produce_line (p->transformed (trans), clear);
  }

  mp_reader = 0;
  mp_ep = 0;

  l.swap (m_lines);
  p.swap (m_polygons);
  cp.swap (m_clear_polygons);
}

void 
RS274XApertureBase::clear_points ()
{
  m_points.clear ();
}

void 
RS274XApertureBase::add_point (double x, double y)
{
  double dbu = mp_reader->dbu ();
  m_points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (x / dbu), db::coord_traits<db::Coord>::rounded (y / dbu)));
}

void 
RS274XApertureBase::add_point (const db::DPoint &d)
{
  double dbu = mp_reader->dbu ();
  m_points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (d.x () / dbu), db::coord_traits<db::Coord>::rounded (d.y () / dbu)));
}

void
RS274XApertureBase::add_point (const db::Point &p)
{
  m_points.push_back (p);
}

void
RS274XApertureBase::produce_circle (double cx, double cy, double r, bool clear)
{
  clear_points ();

  int n_circle = mp_reader->get_circle_points ();

  //  adjust the radius so we get a outer approximation of the circle:
  //  r *= 1.0 / cos (M_PI / double (n_circle));

  for (int i = 0; i < n_circle; ++i) {
    double a = M_PI * 2.0 * ((double (i) + 0.5) / double (n_circle));
    add_point (cx + r * cos (a), cy + r * sin (a));
  }

  produce_polygon (clear);
}

void 
RS274XApertureBase::produce_line ()
{
  m_lines.push_back (db::Path (m_points.begin (), m_points.end (), 0));
}

void 
RS274XApertureBase::produce_polygon (bool clear)
{
  if (clear) {

    m_clear_polygons.push_back (db::Polygon ());
    m_clear_polygons.back ().assign_hull (m_points.begin (), m_points.end ());

  } else {

    if (! m_clear_polygons.empty ()) {
      std::vector<db::Polygon> input;
      m_polygons.swap (input);
      mp_ep->boolean (input, m_clear_polygons, m_polygons, db::BooleanOp::ANotB, false, true);
      m_clear_polygons.clear ();
    }

    m_polygons.push_back (db::Polygon ());
    m_polygons.back ().assign_hull (m_points.begin (), m_points.end ());

  }
}

// -----------------------------------------------------------------------------
//  RS274XCircleAperture implementation

RS274XCircleAperture::RS274XCircleAperture (RS274XReader &reader, tl::Extractor &ex)
  : m_d (0.0), m_dx (0.0), m_dy (0.0)
{
  ex.expect(",");
  ex.read (m_d);
  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_dx);
  }
  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_dy);
  }
  ex.expect_end ();

  m_d = reader.um (m_d);
  m_dx = reader.um (m_dx);
  m_dy = reader.um (m_dy);
}

void 
RS274XCircleAperture::do_produce_flash () 
{
  // produce outer circle
  produce_circle (0.0, 0.0, m_d * 0.5, false);
  
  if (m_dx > 0.0 && m_dy == 0.0) {

    // produce circle hole
    produce_circle (0.0, 0.0, m_dx * 0.5, true);

  } else if (m_dx > 0.0 && m_dy > 0.0) {

    // produce square hole
    clear_points ();
    add_point (db::DPoint (-m_dx * 0.5, -m_dy * 0.5));
    add_point (db::DPoint (m_dx * 0.5, -m_dy * 0.5));
    add_point (db::DPoint (m_dx * 0.5, m_dy * 0.5));
    add_point (db::DPoint (-m_dx * 0.5, m_dy * 0.5));
    produce_polygon (true);

  }
}

bool 
RS274XCircleAperture::do_produce_linear (const db::DPoint &from, const db::DPoint &to) 
{
  if (m_dx > 0.0 || m_dy > 0.0) {
    return false;
  }

  if (m_d < 1e-10) {

    //  zero radius: draw a line rather than a aperture 
    clear_points ();
    add_point (from);
    add_point (to);
    produce_line ();

  } else {

    db::DVector p (to - from);
    if (p.sq_length () < 1e-10) {

      produce_circle (from.x (), from.y (), m_d * 0.5, false);

    } else {

      clear_points ();

      p.transform (db::DFTrans (db::DFTrans::r270));

      int n_circle = reader ().get_circle_points ();

      //  adjust the radius so we get an outer approximation of the circle
      double r = 0.5 * m_d / cos (M_PI / double (n_circle));
      p *= r / p.length ();

      double a = -2.0 * M_PI / double (n_circle);

      p = db::DVector (p.x() * cos(a * 0.5) - p.y() * sin(a * 0.5), p.x() * sin(a * 0.5) + p.y() * cos(a * 0.5));

      for (int i = 0; i < n_circle / 2; ++i) {
        add_point (from + p);
        p = db::DVector (p.x() * cos(a) - p.y() * sin(a), p.x() * sin(a) + p.y() * cos(a));
      }

      for (int i = 0; i < n_circle / 2; ++i) {
        add_point (to + p);
        p = db::DVector (p.x() * cos(a) - p.y() * sin(a), p.x() * sin(a) + p.y() * cos(a));
      }

      produce_polygon (false);

    }

  }

  return true;
}

// -----------------------------------------------------------------------------
//  RS274XRectAperture implementation

RS274XRectAperture::RS274XRectAperture (db::RS274XReader &reader, tl::Extractor &ex)
  : m_dx (0.0), m_dy (0.0), m_hx (0.0), m_hy (0.0)
{
  ex.expect(",");
  ex.read (m_dx);
  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_dy);
  }
  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_hx);
  }
  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_hy);
  }
  ex.expect_end ();

  m_dx = reader.um (m_dx);
  m_dy = reader.um (m_dy);
  m_hx = reader.um (m_hx);
  m_hy = reader.um (m_hy);
}

void 
RS274XRectAperture::do_produce_flash ()
{
  // produce outer box
  clear_points ();
  add_point (db::DPoint (-m_dx * 0.5, -m_dy * 0.5));
  add_point (db::DPoint (m_dx * 0.5, -m_dy * 0.5));
  add_point (db::DPoint (m_dx * 0.5, m_dy * 0.5));
  add_point (db::DPoint (-m_dx * 0.5, m_dy * 0.5));
  produce_polygon (false);
  
  if (m_hx > 0.0 && m_hy > 0.0) {

    // produce square hole
    clear_points ();
    add_point (db::DPoint (-m_hx * 0.5, -m_hy * 0.5));
    add_point (db::DPoint (m_hx * 0.5, -m_hy * 0.5));
    add_point (db::DPoint (m_hx * 0.5, m_hy * 0.5));
    add_point (db::DPoint (-m_hx * 0.5, m_hy * 0.5));
    produce_polygon (true);

  }
}

bool  
RS274XRectAperture::do_produce_linear (const db::DPoint & /*from*/, const db::DPoint & /*to*/) 
{
  return false;
}

// -----------------------------------------------------------------------------
//  RS274XOvalAperture implementation

RS274XOvalAperture::RS274XOvalAperture (db::RS274XReader &reader, tl::Extractor &ex)
  : m_dx (0.0), m_dy (0.0), m_hx (0.0), m_hy (0.0)
{
  ex.expect(",");
  ex.read (m_dx);
  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_dy);
  }
  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_hx);
  }
  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_hy);
  }
  ex.expect_end ();

  m_dx = reader.um (m_dx);
  m_dy = reader.um (m_dy);
  m_hx = reader.um (m_hx);
  m_hy = reader.um (m_hy);
}

void 
RS274XOvalAperture::do_produce_flash ()
{
  int n_circle = reader ().get_circle_points ();

  // produce outer oval
  clear_points ();

  if (m_dx - m_dy > 1e-4) {

    double r = m_dy / cos (M_PI / double (n_circle));

    for (int i = 0; i < n_circle / 2; ++i) {
      double a = -M_PI * 0.5 - M_PI * 2.0 * ((double (i) + 0.5) / double (n_circle));
      add_point (0.5 * (-m_dx + m_dy + r * cos (a)), 0.5 * r * sin (a));
    }

    for (int i = 0; i < n_circle / 2; ++i) {
      double a = M_PI * 0.5 - M_PI * 2.0 * ((double (i) + 0.5) / double (n_circle));
      add_point (0.5 * (m_dx - m_dy + r * cos (a)), 0.5 * r * sin (a));
    }

  } else if (m_dy - m_dx > 1e-4) {

    double r = m_dx / cos (M_PI / double (n_circle));

    for (int i = 0; i < n_circle / 2; ++i) {
      double a = -M_PI * 2.0 * ((double (i) + 0.5) / double (n_circle));
      add_point (0.5 * r * cos (a), 0.5 * (-m_dy + m_dx + r * sin (a)));
    }

    for (int i = 0; i < n_circle / 2; ++i) {
      double a = M_PI - M_PI * 2.0 * ((double (i) + 0.5) / double (n_circle));
      add_point (0.5 * r * cos (a), 0.5 * (m_dy - m_dx + r * sin (a)));
    }

  } else {

    //  intentionally create a polygon confined within (!) the circle (in the other cases, 
    //  this must not be the case to maintain the width, here this is not necessary)
    for (int i = 0; i < n_circle; ++i) {
      double a = -M_PI * 2.0 * ((double (i) + 0.5) / double (n_circle));
      add_point (0.5 * m_dx * cos (a), 0.5 * m_dx * sin (a));
    }

  }

  produce_polygon (false);
  
  if (m_hx > 0.0 && m_hy == 0.0) {

    // produce circle hole
    produce_circle (0.0, 0.0, m_hx * 0.5, true);

  } else if (m_hx > 0.0 && m_hy > 0.0) {

    // produce square hole
    clear_points ();
    add_point (db::DPoint (-m_hx * 0.5, -m_hy * 0.5));
    add_point (db::DPoint (m_hx * 0.5, -m_hy * 0.5));
    add_point (db::DPoint (m_hx * 0.5, m_hy * 0.5));
    add_point (db::DPoint (-m_hx * 0.5, m_hy * 0.5));
    produce_polygon (true);

  }
}

bool 
RS274XOvalAperture::do_produce_linear (const db::DPoint & /*from*/, const db::DPoint & /*to*/) 
{
  return false;
}

// -----------------------------------------------------------------------------
//  RS274XRegularAperture implementation

RS274XRegularAperture::RS274XRegularAperture (db::RS274XReader &reader, tl::Extractor &ex)
  : m_d (0.0), m_a (0.0), m_nsides (0), m_hx (0.0), m_hy (0.0)
{
  ex.expect(",");
  ex.read (m_d);
  ex.test(",");
  ex.expect ("X");
  ex.read (m_nsides);

  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_a);
  }
  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_hx);
  }
  ex.test(",");
  if (ex.test ("X")) {
    ex.read (m_hy);
  }

  ex.expect_end ();

  m_d = reader.um (m_d);
  m_hx = reader.um (m_hx);
  m_hy = reader.um (m_hy);
}

void 
RS274XRegularAperture::do_produce_flash ()
{
  // produce outer regular polygon
  clear_points ();

  for (int i = 0; i < m_nsides; ++i) {
    double a = M_PI * 2.0 * double (i) / double (m_nsides) + m_a * (M_PI / 180.0);
    add_point (0.5 * m_d * cos (a), 0.5 * m_d * sin (a));
  }

  produce_polygon (false);
  
  if (m_hx > 0.0 && m_hy > 0.0) {

    // produce square hole
    clear_points ();
    add_point (db::DPoint (-m_hx * 0.5, -m_hy * 0.5));
    add_point (db::DPoint (m_hx * 0.5, -m_hy * 0.5));
    add_point (db::DPoint (m_hx * 0.5, m_hy * 0.5));
    add_point (db::DPoint (-m_hx * 0.5, m_hy * 0.5));
    produce_polygon (true);

  }
}

bool 
RS274XRegularAperture::do_produce_linear (const db::DPoint & /*from*/, const db::DPoint & /*to*/) 
{
  return false;
}

// -----------------------------------------------------------------------------
//  RS274XRegionAperture implementation

RS274XRegionAperture::RS274XRegionAperture (const db::Region &region)
  : m_region (region)
{
  //  .. nothing yet ..
}

void RS274XRegionAperture::do_produce_flash ()
{
  for (db::Region::const_iterator p = m_region.begin (); ! p.at_end (); ++p) {

    db::Polygon poly = *p;

    clear_points ();
    for (db::Polygon::polygon_contour_iterator pt = poly.begin_hull (); pt != poly.end_hull (); ++pt) {
      add_point (*pt);
    }
    produce_polygon (false);

    for (size_t h = 0; h < poly.holes (); ++h) {
      clear_points ();
      for (db::Polygon::polygon_contour_iterator pt = poly.begin_hole (int (h)); pt != poly.end_hole (int (h)); ++pt) {
        add_point (*pt);
      }
      produce_polygon (true);
    }

  }
}

bool
RS274XRegionAperture::do_produce_linear (const db::DPoint & /*from*/, const db::DPoint & /*to*/)
{
  return false;
}

// -----------------------------------------------------------------------------
//  RS274XMacroAperture implementation

RS274XMacroAperture::RS274XMacroAperture (db::RS274XReader &reader, const std::string &name, const std::string &def, tl::Extractor &ex)
  : m_name (name), m_def (def), m_unit (1.0)
{ 
  //  use the unit at definition time, not at execution time
  m_unit = reader.unit ();

  while (! ex.at_end ()) {
    double p;
    if (! ex.test (",") && ! ex.test ("X")) {
      ex.expect_end ();
    }
    ex.read (p);
    m_parameters.push_back (p);
  }
}

void 
RS274XMacroAperture::do_produce_flash ()
{
  try {
    do_produce_flash_internal ();
  } catch (tl::Exception &ex) {
    throw tl::Exception (ex.msg () + " (" + tl::to_string (tr ("expanding macro")) + " " + m_name + ")");
  }
}

void
RS274XMacroAperture::read_exposure (tl::Extractor &ex, bool &clear, bool &clear_set)
{
  int pol = int (floor (read_expr (ex) + 0.5));

  if (pol == 0) {
    clear = true;
  } else if (pol == 1) {
    clear = false;
  } else if (pol == 2) {
    clear = !clear_set || !clear;
  } else {
    throw tl::Exception (tl::to_string (tr ("Invalid exposure code '%d'")), pol);
  }

  clear_set = true;
}

void 
RS274XMacroAperture::do_produce_flash_internal ()
{
  tl::Extractor ex (m_def.c_str ());
  bool clear = false;
  bool clear_set = false;

  while (! ex.at_end ()) {

    if (ex.test ("$")) {

      int nvar = 0;
      ex.read (nvar);
      nvar -= 1;

      ex.expect ("=");

      double value = read_expr (ex);

      if (nvar >= 0) {
        while (int (m_parameters.size ()) <= nvar) {
          m_parameters.push_back (0.0);
        }
        m_parameters[nvar] = value;
      }

    } else if (! ex.test ("*")) {

      int code = 0;
      ex.read (code);

      if (code == 1) {

        ex.expect (",");
        read_exposure (ex, clear, clear_set);
        ex.expect (",");
        double d = read_expr (ex, true);
        ex.expect (",");
        double cx = read_expr (ex, true);
        ex.expect (",");
        double cy = read_expr (ex, true);

        double a = 0.0;
        if (ex.test (",")) {
          a = read_expr (ex);
        }

        db::DVector c = db::DCplxTrans (1.0, a, false, db::DVector ()) * db::DVector (cx, cy);
        produce_circle (c.x (), c.y (), d * 0.5, clear);

      } else if (code == 2 || code == 20) {

        ex.expect (",");
        read_exposure (ex, clear, clear_set);
        ex.expect (",");
        double w = read_expr (ex, true);
        ex.expect (",");
        double x1 = read_expr (ex, true);
        ex.expect (",");
        double y1 = read_expr (ex, true);
        ex.expect (",");
        double x2 = read_expr (ex, true);
        ex.expect (",");
        double y2 = read_expr (ex, true);
        ex.expect (",");
        double a = read_expr (ex);

        db::DCplxTrans t = db::DCplxTrans (1.0, a, false, db::DVector ());
        db::DPoint from = db::DPoint (x1, y1);
        db::DPoint to   = db::DPoint (x2, y2);

        db::DVector p (to - from);
        if (p.sq_length () < 1e-10) {
          throw tl::Exception (tl::to_string (tr ("Identical start and end point in type 2 or 20 aperture macro primitive")));
        }

        clear_points ();

        p = db::DVector (p.y(), -p.x()) * (0.5 * w / p.length ());

        for (int i = 0; i < 2; ++i) {
          add_point (t * (from + p));
          p = db::DVector (-p.x(), -p.y ());
        }

        for (int i = 0; i < 2; ++i) {
          p = db::DVector (-p.x(), -p.y ());
          add_point (t * (to + p));
        }

        produce_polygon (clear);

      } else if (code == 21 || code == 22) {

        ex.expect (",");
        read_exposure (ex, clear, clear_set);
        ex.expect (",");
        double w = read_expr (ex, true);
        ex.expect (",");
        double h = read_expr (ex, true);
        ex.expect (",");
        double x = read_expr (ex, true);
        ex.expect (",");
        double y = read_expr (ex, true);
        ex.expect (",");
        double a = read_expr (ex);

        if (code == 22) {
          //  TODO: clarify: how is rotation defined?
          x += 0.5 * w;
          y += 0.5 * h;
        }

        db::DCplxTrans t = db::DCplxTrans (1.0, a, false, db::DVector ());

        clear_points ();

        add_point (t * db::DPoint (x - w * 0.5, y - h * 0.5));
        add_point (t * db::DPoint (x - w * 0.5, y + h * 0.5));
        add_point (t * db::DPoint (x + w * 0.5, y + h * 0.5));
        add_point (t * db::DPoint (x + w * 0.5, y - h * 0.5));

        produce_polygon (clear);

      } else if (code == 4) {

        ex.expect (",");
        read_exposure (ex, clear, clear_set);
        ex.expect (",");
        int n = int (read_expr (ex) + 0.5);
        if (n < 1) {
          throw tl::Exception (tl::to_string (tr ("Invalid point count in outline element in aperture macro")));
        }

        std::vector<db::DPoint> points;

        for (int i = 0; i < n + 1; ++i) {

          ex.expect (",");
          double x = read_expr (ex, true);

          ex.expect (",");
          double y = read_expr (ex, true);

          points.push_back (db::DPoint (x, y));

        }

        ex.expect (",");
        double a = read_expr (ex);

        db::DCplxTrans t = db::DCplxTrans (1.0, a, false, db::DVector ());

        if (points.size () > 2 && points.front ().sq_distance (points.back ()) < 1e-10) {

          //  closed outline - fill solid 

          clear_points ();

          for (std::vector<db::DPoint>::const_iterator o = points.begin () + 1; o != points.end (); ++o) {
            add_point (t * *o);
          }

          produce_polygon (clear);

        } else if (! points.empty ()) {

          //  open outline - create thin path

          double w = 2.0; // make this variable?

          for (std::vector<db::DPoint>::const_iterator o = points.begin () + 1; o != points.end (); ++o) {

            db::DPoint from = t * o[-1];
            db::DPoint to   = t * o[0];

            db::DVector p (to - from);
            if (p.sq_length () > 1e-10) {

              clear_points ();

              p = db::DVector (p.y(), -p.x()) * (0.5 * w / p.length ());

              for (int i = 0; i < 2; ++i) {
                add_point (from + p);
                p = db::DVector (-p.x(), -p.y ());
              }

              for (int i = 0; i < 2; ++i) {
                p = db::DVector (-p.x(), -p.y ());
                add_point (to + p);
              }

              produce_polygon (clear);

            }

          }

        }

      } else if (code == 5) {

        ex.expect (",");
        read_exposure (ex, clear, clear_set);
        ex.expect (",");
        int n = int (read_expr (ex) + 0.5);
        if (n < 3) {
          throw tl::Exception (tl::to_string (tr ("Invalid point count in polygon element in aperture macro")));
        }

        ex.expect (",");
        double x = read_expr (ex, true);
        ex.expect (",");
        double y = read_expr (ex, true);
        ex.expect (",");
        double d = read_expr (ex, true);
        ex.expect (",");
        double a0 = read_expr (ex);

        clear_points ();

        for (int i = 0; i < n; ++i) {
          double a = M_PI * 2.0 * double (i) / double (n) + a0 * (M_PI / 180.0);
          add_point (x + 0.5 * d * cos (a), y + 0.5 * d * sin (a));
        }

        produce_polygon (clear);

      } else if (code == 6) {

        ex.expect (",");
        double x = read_expr (ex, true);
        ex.expect (",");
        double y = read_expr (ex, true);
        ex.expect (",");
        double d = read_expr (ex, true);
        ex.expect (",");
        double t = read_expr (ex, true);
        ex.expect (",");
        double g = read_expr (ex, true);
        ex.expect (",");
        int n = int (read_expr (ex) + 0.5);
        ex.expect (",");
        double ct = read_expr (ex, true);
        ex.expect (",");
        double cl = read_expr (ex, true);
        ex.expect (",");
        double a = read_expr (ex);

        db::DCplxTrans tr = db::DCplxTrans (db::DVector (x, y)) * db::DCplxTrans (1.0, a, false, db::DVector ()) * db::DCplxTrans (db::DVector (-x, -y));

        for (int i = 0; i < n; ++i) {
          produce_circle (x, y, d * 0.5 - i * (g + t), false);
          produce_circle (x, y, d * 0.5 - i * (g + t) - t, true);
        }

        clear_points ();
        add_point (tr * db::DPoint (-0.5 * cl, -0.5 * ct));
        add_point (tr * db::DPoint (-0.5 * cl, 0.5 * ct));
        add_point (tr * db::DPoint (0.5 * cl, 0.5 * ct));
        add_point (tr * db::DPoint (0.5 * cl, -0.5 * ct));
        produce_polygon (false);

        clear_points ();
        add_point (tr * db::DPoint (-0.5 * ct, -0.5 * cl));
        add_point (tr * db::DPoint (-0.5 * ct, 0.5 * cl));
        add_point (tr * db::DPoint (0.5 * ct, 0.5 * cl));
        add_point (tr * db::DPoint (0.5 * ct, -0.5 * cl));
        produce_polygon (false);

      } else if (code == 7) {

        ex.expect (",");
        double x = read_expr (ex, true);
        ex.expect (",");
        double y = read_expr (ex, true);
        ex.expect (",");
        double d = read_expr (ex, true);
        ex.expect (",");
        double di = read_expr (ex, true);
        ex.expect (",");
        double ct = read_expr (ex, true);
        ex.expect (",");
        double a = read_expr (ex);

        db::DCplxTrans t = db::DCplxTrans (db::DVector (x, y)) * db::DCplxTrans (1.0, a, false, db::DVector ()) * db::DCplxTrans (db::DVector (-x, -y));

        produce_circle (x, y, d * 0.5, false);
        produce_circle (x, y, di * 0.5, true);

        clear_points ();
        add_point (t * db::DPoint (-0.5 * d, -0.5 * ct));
        add_point (t * db::DPoint (-0.5 * d, 0.5 * ct));
        add_point (t * db::DPoint (0.5 * d, 0.5 * ct));
        add_point (t * db::DPoint (0.5 * d, -0.5 * ct));
        produce_polygon (true);

        clear_points ();
        add_point (t * db::DPoint (-0.5 * ct, -0.5 * d));
        add_point (t * db::DPoint (-0.5 * ct, 0.5 * d));
        add_point (t * db::DPoint (0.5 * ct, 0.5 * d));
        add_point (t * db::DPoint (0.5 * ct, -0.5 * d));
        produce_polygon (true);

      } else if (code == 3) {
        //  end of file - ignore
      } else if (code == 0) {

        //  comment
        while (! ex.at_end () && *ex != '*') {
          ++ex;
        }

      }

      ex.test ("*");

    } 

  }
}

bool 
RS274XMacroAperture::do_produce_linear (const db::DPoint & /*from*/, const db::DPoint & /*to*/) 
{
  return false;
}

double 
RS274XMacroAperture::read_atom (tl::Extractor &ex)
{
  double fac = 1.0;
  if (ex.test ("-")) {
    fac = -1.0;
  }

  double d = 0.0;

  if (ex.test ("$")) {
    int nvar = 0;
    ex.read (nvar);
    nvar -= 1;
    if (nvar >= 0 && nvar < int (m_parameters.size ())) {
      d = m_parameters[nvar];
    }
  } else if (ex.test ("(")) {
    d = read_expr (ex);
    ex.expect (")");
  } else {
    ex.read (d);
  }

  return fac * d;
}

double 
RS274XMacroAperture::read_dot_expr (tl::Extractor &ex)
{
  double d = read_atom (ex);
  while (! ex.at_end ()) {
    if (ex.test ("x") || ex.test ("X")) {
      d *= read_atom (ex);
    } else if (ex.test ("/")) {
      d /= read_atom (ex);
    } else {
      break;
    }
  }

  return d;
}

double 
RS274XMacroAperture::read_expr (tl::Extractor &ex, bool length)
{
  double d = read_dot_expr (ex);
  while (! ex.at_end ()) {
    if (ex.test ("+")) {
      d += read_dot_expr (ex);
    } else if (ex.test ("-")) {
      d -= read_dot_expr (ex);
    } else {
      break;
    }
  }

  if (length) {
    d *= m_unit;
  }

  return d;
}

}


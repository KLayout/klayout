
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


#include "libBasicPie.h"

#include <cmath>

namespace lib
{

// --------------------------------------------------------------------------
//  Implementation

static const size_t p_layer = 0;
static const size_t p_radius = 1;
static const size_t p_start_angle = 2;
static const size_t p_end_angle = 3;
static const size_t p_handle1 = 4;
static const size_t p_handle2 = 5;
static const size_t p_npoints = 6;
static const size_t p_actual_radius = 7;
static const size_t p_actual_start_angle = 8;
static const size_t p_actual_end_angle = 9;
static const size_t p_actual_handle1 = 10;
static const size_t p_actual_handle2 = 11;
static const size_t p_total = 12;

BasicPie::BasicPie ()
{
  //  .. nothing yet ..
}

std::vector<db::PCellLayerDeclaration> 
BasicPie::get_layer_declarations (const db::pcell_parameters_type &parameters) const
{
  std::vector<db::PCellLayerDeclaration> layers;
  if (parameters.size () > p_layer && parameters [p_layer].is_user<db::LayerProperties> ()) {
    db::LayerProperties lp = parameters [p_layer].to_user<db::LayerProperties> ();
    if (lp != db::LayerProperties ()) {
      layers.push_back (lp);
    }
  }
  return layers;
}

void 
BasicPie::coerce_parameters (const db::Layout & /*layout*/, db::pcell_parameters_type &parameters) const
{
  if (parameters.size () < p_total) {
    return;
  }

  double ru = parameters [p_radius].to_double ();
  double r = parameters [p_actual_radius].to_double ();
  double rs = ru;
  if (parameters [p_actual_handle1].is_user <db::DPoint> ()) {
    rs = parameters [p_actual_handle1].to_user <db::DPoint> ().distance ();
    if (parameters [p_actual_handle2].is_user <db::DPoint> ()) {
      rs = std::max (rs, parameters [p_actual_handle2].to_user <db::DPoint> ().distance ());
    }
  } else if (parameters [p_actual_handle2].is_user <db::DPoint> ()) {
    rs = parameters [p_actual_handle2].to_user <db::DPoint> ().distance ();
  } 

  double a1u = parameters [p_start_angle].to_double ();
  double a1 = parameters [p_actual_start_angle].to_double ();
  db::DPoint h1u;
  if (parameters [p_handle1].is_user<db::DPoint> ()) {
    h1u = parameters [p_handle1].to_user<db::DPoint> ();
  }
  db::DPoint h1;
  if (parameters [p_actual_handle1].is_user<db::DPoint> ()) {
    h1 = parameters [p_actual_handle1].to_user<db::DPoint> ();
  }

  double a2u = parameters [p_end_angle].to_double ();
  double a2 = parameters [p_actual_end_angle].to_double ();
  db::DPoint h2u;
  if (parameters [p_handle2].is_user<db::DPoint> ()) {
    h2u = parameters [p_handle2].to_user<db::DPoint> ();
  }
  db::DPoint h2;
  if (parameters [p_actual_handle2].is_user<db::DPoint> ()) {
    h2 = parameters [p_actual_handle2].to_user<db::DPoint> ();
  }

  if (fabs (ru - r) > 1e-6 || fabs (a1u - a1) > 1e-6 || fabs (a2u - a2) > 1e-6) {

    //  the explicit parameters have changed: use it
    ru = r;
    a1u = a1;
    a2u = a2;
    h1u = db::DPoint (r * cos (a1 / 180.0 * M_PI), r * sin (a1 / 180.0 * M_PI));
    h2u = db::DPoint (r * cos (a2 / 180.0 * M_PI), r * sin (a2 / 180.0 * M_PI));

    parameters [p_actual_handle1] = h1u;
    parameters [p_actual_handle2] = h2u;

  } else if (h1u.distance (h1) > 1e-6 || h2u.distance (h2) > 1e-6) {

    //  the handle has changed: use this
    
    double a1s = 180.0 * atan2 (h1.y (), h1.x ()) / M_PI;
    double a2s = 180.0 * atan2 (h2.y (), h2.x ()) / M_PI;

    ru = rs;
    a1u = a1s;
    a2u = a2s;
    h1u = h1;
    h2u = h2;

    parameters [p_actual_radius] = ru;
    parameters [p_actual_start_angle] = a1u;
    parameters [p_actual_end_angle] = a2u;

  }

  //  set the hidden used radius parameter
  parameters [p_radius] = ru;
  parameters [p_start_angle] = a1u;
  parameters [p_end_angle] = a2u;
  parameters [p_handle1] = h1u;
  parameters [p_handle2] = h2u;
}

void 
BasicPie::produce (const db::Layout &layout, const std::vector<unsigned int> &layer_ids, const db::pcell_parameters_type &parameters, db::Cell &cell) const
{
  if (parameters.size () < p_total || layer_ids.size () < 1) {
    return;
  }

  double r = parameters [p_radius].to_double () / layout.dbu ();
  double a1 = parameters [p_start_angle].to_double ();
  double a2 = parameters [p_end_angle].to_double ();
  if (a2 < a1 - 1e-6) {
    a2 += 360 * ceil ((a1 - a2) / 360.0 + 1e-6);
  }
  if (a2 > a1 + 360.0 - 1e-6) {
    a2 = a1 + 360.0;
  }
  int n = std::max (2, int (floor (0.5 + std::max (8, parameters [p_npoints].to_int ()) * (a2 - a1) / 360.0)));

  std::vector <db::Point> points;
  points.reserve (n + 3);

  //  Produce an outer circle approximation. This 
  //  one looks slightly better in the case of few points. 
  double rr = r / cos (M_PI * (a2 - a1) / (360.0 * n));
  double da = M_PI * (a2 - a1) / (180.0 * n);
  for (int i = 0; i < n; ++i) {
    double a = (i + 0.5) * da + M_PI * a1 / 180.0;
    points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (rr * cos (a)), db::coord_traits<db::Coord>::rounded (rr * sin (a))));
  }

  points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (r * cos (a2 * M_PI / 180.0)), db::coord_traits<db::Coord>::rounded (r * sin (a2 * M_PI / 180.0))));
  points.push_back (db::Point (0, 0));
  points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (r * cos (a1 * M_PI / 180.0)), db::coord_traits<db::Coord>::rounded (r * sin (a1 * M_PI / 180.0))));

  //  Produce the shape
  db::SimplePolygon poly;
  poly.assign_hull (points.begin (), points.end ());
  cell.shapes (layer_ids [p_layer]).insert (poly);
}

std::string 
BasicPie::get_display_name (const db::pcell_parameters_type &parameters) const
{
  return "PIE(l=" + std::string (parameters [p_layer].to_string ()) +
            ",r=" + tl::to_string (parameters [p_radius].to_double ()) +
            ",a=" + tl::to_string (parameters [p_start_angle].to_double (), 6) +
             ".." + tl::to_string (parameters [p_end_angle].to_double (), 6) +
            ",n=" + tl::to_string (parameters [p_npoints].to_int ()) +
              ")";
}

std::vector<db::PCellParameterDeclaration> 
BasicPie::get_parameter_declarations () const
{
  std::vector<db::PCellParameterDeclaration> parameters;

  //  parameter #0: layer 
  tl_assert (parameters.size () == p_layer);
  parameters.push_back (db::PCellParameterDeclaration ("layer"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_layer);
  parameters.back ().set_description (tl::to_string (tr ("Layer")));

  //  parameter #1: radius 
  //  This parameter is updated by "coerce_parameters" from "actual_radius" or the handles,
  //  whichever changed.
  tl_assert (parameters.size () == p_radius);
  parameters.push_back (db::PCellParameterDeclaration ("radius"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_hidden (true);

  //  parameter #2: start angle 
  //  This is a shadow parameter to keep the final start angle
  tl_assert (parameters.size () == p_start_angle);
  parameters.push_back (db::PCellParameterDeclaration ("a1"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_hidden (true);

  //  parameter #3: end angle 
  //  This is a shadow parameter to keep the final end angle
  tl_assert (parameters.size () == p_end_angle);
  parameters.push_back (db::PCellParameterDeclaration ("a2"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_hidden (true);

  //  parameter #4: handle 1
  tl_assert (parameters.size () == p_handle1);
  //  This is a shadow parameter to keep the current handle position and to determine
  //  whether the handle changed
  parameters.push_back (db::PCellParameterDeclaration ("handle1"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  parameters.back ().set_hidden (true);

  //  parameter #5: handle 1
  //  This is a shadow parameter to keep the current handle position and to determine
  //  whether the handle changed
  tl_assert (parameters.size () == p_handle2);
  parameters.push_back (db::PCellParameterDeclaration ("handle2"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  parameters.back ().set_hidden (true);

  //  parameter #6: number of points 
  tl_assert (parameters.size () == p_npoints);
  parameters.push_back (db::PCellParameterDeclaration ("npoints"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_int);
  parameters.back ().set_description (tl::to_string (tr ("Number of points")));
  parameters.back ().set_default (64);

  //  parameter #7: used radius
  tl_assert (parameters.size () == p_actual_radius);
  parameters.push_back (db::PCellParameterDeclaration ("actual_radius"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Radius")));
  parameters.back ().set_unit (tl::to_string (tr ("micron")));
  parameters.back ().set_default (1.0);

  //  parameter #8: used start angle
  tl_assert (parameters.size () == p_actual_start_angle);
  parameters.push_back (db::PCellParameterDeclaration ("actual_start_angle"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Start angle")));
  parameters.back ().set_unit (tl::to_string (tr ("degree")));
  parameters.back ().set_default (0.0);

  //  parameter #9: used end angle
  tl_assert (parameters.size () == p_actual_end_angle);
  parameters.push_back (db::PCellParameterDeclaration ("actual_end_angle"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("End angle")));
  parameters.back ().set_unit (tl::to_string (tr ("degree")));
  parameters.back ().set_default (90.0);

  //  parameter #10: used handle 1
  tl_assert (parameters.size () == p_actual_handle1);
  parameters.push_back (db::PCellParameterDeclaration ("actual_handle1"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  parameters.back ().set_description (tl::to_string (tr ("S")));
  parameters.back ().set_default (db::DPoint (-1.0, 0.0));

  //  parameter #11: used handle 2
  tl_assert (parameters.size () == p_actual_handle2);
  parameters.push_back (db::PCellParameterDeclaration ("actual_handle2"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  parameters.back ().set_description (tl::to_string (tr ("E")));
  parameters.back ().set_default (db::DPoint (0.0, 1.0));

  tl_assert (parameters.size () == p_total);
  return parameters;
}

}



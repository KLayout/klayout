
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


#include "libBasicEllipse.h"

#include <cmath>

namespace lib
{

// --------------------------------------------------------------------------
//  Implementation

static const size_t p_layer = 0;
static const size_t p_radius_x = 1;
static const size_t p_radius_y = 2;
static const size_t p_handle_x = 3;
static const size_t p_handle_y = 4;
static const size_t p_npoints = 5;
static const size_t p_actual_radius_x = 6;
static const size_t p_actual_radius_y = 7;
static const size_t p_total = 8;

BasicEllipse::BasicEllipse ()
{
  //  .. nothing yet ..
}

bool 
BasicEllipse::can_create_from_shape (const db::Layout & /*layout*/, const db::Shape &shape, unsigned int /*layer*/) const
{
  return (shape.is_polygon () || shape.is_box () || shape.is_path ());
}

db::Trans
BasicEllipse::transformation_from_shape (const db::Layout & /*layout*/, const db::Shape &shape, unsigned int /*layer*/) const
{
  //  use the displacement to define the center of the circle
  return db::Trans (shape.bbox ().center () - db::Point ());
}

db::pcell_parameters_type
BasicEllipse::parameters_from_shape (const db::Layout &layout, const db::Shape &shape, unsigned int layer) const
{
  db::DBox dbox = db::CplxTrans (layout.dbu ()) * shape.bbox ();

  //  use map_parameters to create defaults for the other parameters
  std::map<size_t, tl::Variant> nm;
  nm.insert (std::make_pair (p_layer, tl::Variant (layout.get_properties (layer))));
  nm.insert (std::make_pair (p_actual_radius_x, tl::Variant (0.5 * dbox.width ())));
  nm.insert (std::make_pair (p_actual_radius_y, tl::Variant (0.5 * dbox.height ())));
  return map_parameters (nm);
}

std::vector<db::PCellLayerDeclaration> 
BasicEllipse::get_layer_declarations (const db::pcell_parameters_type &parameters) const
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
BasicEllipse::coerce_parameters (const db::Layout & /*layout*/, db::pcell_parameters_type &parameters) const
{
  if (parameters.size () < p_total) {
    return;
  }

  double ru_x = parameters [p_radius_x].to_double ();
  double r_x = parameters [p_actual_radius_x].to_double ();

  double rs_x = ru_x;
  if (parameters [p_handle_x].is_user <db::DPoint> ()) {
    rs_x = fabs(parameters [p_handle_x].to_user <db::DPoint> ().x ());
  } 

  if (fabs (ru_x - r_x) > 1e-6) {
    //  the explicit radius has changed: use it
    ru_x = r_x;
    parameters [p_handle_x] = db::DPoint (-r_x, 0);
  } else {
    //  the handle has changed: use this
    ru_x = rs_x;
    r_x = rs_x;
    parameters [p_actual_radius_x] = r_x;
    parameters [p_handle_x] = db::DPoint (-r_x, 0);
  }

  //  set the hidden used radius parameter
  parameters [p_radius_x] = ru_x;

  //  do the same for the y radius
  double ru_y = parameters [p_radius_y].to_double ();
  double r_y = parameters [p_actual_radius_y].to_double ();

  double rs_y = ru_y;
  if (parameters [p_handle_y].is_user <db::DPoint> ()) {
    rs_y = fabs (parameters [p_handle_y].to_user <db::DPoint> ().y ());
  } 

  if (fabs (ru_y - r_y) > 1e-6) {
    //  the explicit radius has changed: use it
    ru_y = r_y;
    parameters [p_handle_y] = db::DPoint (0, r_y);
  } else {
    //  the handle has changed: use this
    ru_y = rs_y;
    r_y = rs_y;
    parameters [p_actual_radius_y] = r_y;
    parameters [p_handle_y] = db::DPoint (0, r_y);
  }

  //  set the hidden used radius parameter
  parameters [p_radius_y] = ru_y;
}

void 
BasicEllipse::produce (const db::Layout &layout, const std::vector<unsigned int> &layer_ids, const db::pcell_parameters_type &parameters, db::Cell &cell) const
{
  if (parameters.size () < p_total || layer_ids.size () < 1) {
    return;
  }

  double r_x = parameters [p_radius_x].to_double () / layout.dbu ();
  double r_y = parameters [p_radius_y].to_double () / layout.dbu ();
  int n = std::max (3, parameters [p_npoints].to_int ());

  std::vector <db::Point> points;
  points.reserve (n);

  //  Produce an outer circle approximation. This 
  //  one looks slightly better in the case of few points. 
  double rr_x = r_x / cos (M_PI / n);
  double rr_y = r_y / cos (M_PI / n);
  double da = 2.0 * M_PI / n;
  for (int i = 0; i < n; ++i) {
    double a = (i + 0.5) * da;
    points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (-rr_x * cos (a)), db::coord_traits<db::Coord>::rounded (rr_y * sin (a))));
  }

  //  Produce the shape
  db::SimplePolygon poly;
  poly.assign_hull (points.begin (), points.end ());
  cell.shapes (layer_ids [p_layer]).insert (poly);
}

std::string 
BasicEllipse::get_display_name (const db::pcell_parameters_type &parameters) const
{
  return "ELLIPSE(l=" + std::string (parameters [p_layer].to_string ()) +
               ",rx=" + tl::to_string (parameters [p_radius_x].to_double ()) +
               ",ry=" + tl::to_string (parameters [p_radius_y].to_double ()) +
                ",n=" + tl::to_string (parameters [p_npoints].to_int ()) +
                  ")";
}

std::vector<db::PCellParameterDeclaration> 
BasicEllipse::get_parameter_declarations () const
{
  std::vector<db::PCellParameterDeclaration> parameters;

  //  parameter #0: layer 
  tl_assert (parameters.size () == p_layer);
  parameters.push_back (db::PCellParameterDeclaration ("layer"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_layer);
  parameters.back ().set_description (tl::to_string (tr ("Layer")));

  //  parameter #1: x radius 
  tl_assert (parameters.size () == p_radius_x);
  parameters.push_back (db::PCellParameterDeclaration ("radius_x"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_hidden (true);

  //  parameter #2: y radius 
  //  This is a shadow parameter to receive the used y radius
  tl_assert (parameters.size () == p_radius_y);
  parameters.push_back (db::PCellParameterDeclaration ("radius_y"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_hidden (true);

  //  parameter #3: x handle 
  //  This is a shadow parameter to receive the used x radius
  tl_assert (parameters.size () == p_handle_x);
  parameters.push_back (db::PCellParameterDeclaration ("handle_x"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  parameters.back ().set_default (db::DPoint (-1.0, 0));
  parameters.back ().set_description (tl::to_string (tr ("Rx")));

  //  parameter #4: x handle 
  tl_assert (parameters.size () == p_handle_y);
  parameters.push_back (db::PCellParameterDeclaration ("handle_y"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  parameters.back ().set_default (db::DPoint (0, 0.5));
  parameters.back ().set_description (tl::to_string (tr ("Ry")));

  //  parameter #5: number of points 
  tl_assert (parameters.size () == p_npoints);
  parameters.push_back (db::PCellParameterDeclaration ("npoints"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_int);
  parameters.back ().set_description (tl::to_string (tr ("Number of points")));
  parameters.back ().set_default (64);

  //  parameter #6: used x radius
  tl_assert (parameters.size () == p_actual_radius_x);
  parameters.push_back (db::PCellParameterDeclaration ("actual_radius_x"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Radius (x)")));
  parameters.back ().set_unit (tl::to_string (tr ("micron")));
  parameters.back ().set_default (1.0);

  //  parameter #6: used y radius
  tl_assert (parameters.size () == p_actual_radius_y);
  parameters.push_back (db::PCellParameterDeclaration ("actual_radius_y"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Radius (y)")));
  parameters.back ().set_unit (tl::to_string (tr ("micron")));
  parameters.back ().set_default (0.5);

  return parameters;
}

}



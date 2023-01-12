
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


#include "libBasicDonut.h"

#include <cmath>

namespace lib
{

// --------------------------------------------------------------------------
//  Implementation

static const size_t p_layer = 0;
static const size_t p_radius1 = 1;
static const size_t p_radius2 = 2;
static const size_t p_handle1 = 3;
static const size_t p_handle2 = 4;
static const size_t p_npoints = 5;
static const size_t p_actual_radius1 = 6;
static const size_t p_actual_radius2 = 7;
static const size_t p_total = 8;

BasicDonut::BasicDonut ()
{
  //  .. nothing yet ..
}

bool 
BasicDonut::can_create_from_shape (const db::Layout & /*layout*/, const db::Shape &shape, unsigned int /*layer*/) const
{
  return (shape.is_polygon () || shape.is_box () || shape.is_path ());
}

db::Trans
BasicDonut::transformation_from_shape (const db::Layout & /*layout*/, const db::Shape &shape, unsigned int /*layer*/) const
{
  //  use the displacement to define the center of the circle
  return db::Trans (shape.bbox ().center () - db::Point ());
}

db::pcell_parameters_type
BasicDonut::parameters_from_shape (const db::Layout &layout, const db::Shape &shape, unsigned int layer) const
{
  db::DBox dbox = db::CplxTrans (layout.dbu ()) * shape.bbox ();

  //  use map_parameters to create defaults for the other parameters
  std::map<size_t, tl::Variant> nm;
  nm.insert (std::make_pair (p_layer, tl::Variant (layout.get_properties (layer))));
  nm.insert (std::make_pair (p_actual_radius1, tl::Variant (0.5 * std::min (dbox.width (), dbox.height ()))));
  nm.insert (std::make_pair (p_actual_radius2, tl::Variant (0.25 * std::min (dbox.width (), dbox.height ()))));
  return map_parameters (nm);
}

std::vector<db::PCellLayerDeclaration> 
BasicDonut::get_layer_declarations (const db::pcell_parameters_type &parameters) const
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
BasicDonut::coerce_parameters (const db::Layout & /*layout*/, db::pcell_parameters_type &parameters) const
{
  if (parameters.size () < p_total) {
    return;
  }

  double ru1 = parameters [p_radius1].to_double ();
  double r1 = parameters [p_actual_radius1].to_double ();
  double rs1 = ru1;
  if (parameters [p_handle1].is_user <db::DPoint> ()) {
    rs1 = parameters [p_handle1].to_user <db::DPoint> ().distance ();
  } 

  double ru2 = parameters [p_radius2].to_double ();
  double r2 = parameters [p_actual_radius2].to_double ();
  double rs2 = ru2;
  if (parameters [p_handle2].is_user <db::DPoint> ()) {
    rs2 = parameters [p_handle2].to_user <db::DPoint> ().distance ();
  } 

  if (fabs (ru1 - r1) > 1e-6 || fabs (ru2 - r2) > 1e-6) {
    //  the explicit radius has changed: use it
    ru1 = r1;
    ru2 = r2;
    parameters [p_handle1] = db::DPoint (-r1, 0);
    parameters [p_handle2] = db::DPoint (-r2, 0);
  } else {
    //  the handle has changed: use this
    ru1 = rs1;
    r1 = rs1;
    ru2 = rs2;
    r2 = rs2;
    parameters [p_actual_radius1] = r1;
    parameters [p_actual_radius2] = r2;
  }

  //  set the hidden used radius parameter
  parameters [p_radius1] = ru1;
  parameters [p_radius2] = ru2;
}

void 
BasicDonut::produce (const db::Layout &layout, const std::vector<unsigned int> &layer_ids, const db::pcell_parameters_type &parameters, db::Cell &cell) const
{
  if (parameters.size () < p_total || layer_ids.size () < 1) {
    return;
  }

  double r1 = parameters [p_radius1].to_double () / layout.dbu ();
  double r2 = parameters [p_radius2].to_double () / layout.dbu ();
  int n = std::max (3, parameters [p_npoints].to_int ());

  std::vector <db::Point> points;
  points.reserve (n * 2 + 6);

  //  Produce an outer circle approximation. This 
  //  one looks slightly better in the case of few points. 
  double da = 2.0 * M_PI / n;
  double rr1 = r1 / cos (M_PI / n);
  points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (-r1), db::coord_traits<db::Coord>::rounded (0.0)));
  for (int i = 0; i < n; ++i) {
    double a = (i + 0.5) * da;
    points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (-rr1 * cos (a)), db::coord_traits<db::Coord>::rounded (rr1 * sin (a))));
  }
  points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (-r1), db::coord_traits<db::Coord>::rounded (0.0)));

  double rr2 = r2 / cos (M_PI / n);
  points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (-r2), db::coord_traits<db::Coord>::rounded (0.0)));
  for (int i = 0; i < n; ++i) {
    double a = (n - 1 - i + 0.5) * da;
    points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (-rr2 * cos (a)), db::coord_traits<db::Coord>::rounded (rr2 * sin (a))));
  }
  points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (-r2), db::coord_traits<db::Coord>::rounded (0.0)));

  //  Produce the shape
  db::SimplePolygon poly;
  poly.assign_hull (points.begin (), points.end ());
  cell.shapes (layer_ids [p_layer]).insert (poly);
}

std::string 
BasicDonut::get_display_name (const db::pcell_parameters_type &parameters) const
{
  return "DONUT(l=" + std::string (parameters [p_layer].to_string ()) +
              ",r=" + tl::to_string (parameters [p_radius1].to_double ()) +
               ".." + tl::to_string (parameters [p_radius2].to_double ()) +
              ",n=" + tl::to_string (parameters [p_npoints].to_int ()) +
                ")";
}

std::vector<db::PCellParameterDeclaration> 
BasicDonut::get_parameter_declarations () const
{
  std::vector<db::PCellParameterDeclaration> parameters;

  //  parameter #0: layer 
  tl_assert (parameters.size () == p_layer);
  parameters.push_back (db::PCellParameterDeclaration ("layer"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_layer);
  parameters.back ().set_description (tl::to_string (tr ("Layer")));

  //  parameter #1: radius1 
  //  This is a shadow parameter to receive the used first radius
  tl_assert (parameters.size () == p_radius1);
  parameters.push_back (db::PCellParameterDeclaration ("radius1"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_hidden (true);

  //  parameter #2: radius2 
  //  This is a shadow parameter to receive the used second radius
  tl_assert (parameters.size () == p_radius2);
  parameters.push_back (db::PCellParameterDeclaration ("radius2"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_hidden (true);

  //  parameter #3: handle 1
  tl_assert (parameters.size () == p_handle1);
  parameters.push_back (db::PCellParameterDeclaration ("handle1"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  parameters.back ().set_description (tl::to_string (tr ("R1")));

  //  parameter #4: handle 2
  tl_assert (parameters.size () == p_handle2);
  parameters.push_back (db::PCellParameterDeclaration ("handle2"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  parameters.back ().set_description (tl::to_string (tr ("R2")));

  //  parameter #5: number of points 
  tl_assert (parameters.size () == p_npoints);
  parameters.push_back (db::PCellParameterDeclaration ("npoints"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_int);
  parameters.back ().set_description (tl::to_string (tr ("Number of points")));
  parameters.back ().set_default (64);

  //  parameter #6: used radius 1
  tl_assert (parameters.size () == p_actual_radius1);
  parameters.push_back (db::PCellParameterDeclaration ("actual_radius1"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Radius 1")));
  parameters.back ().set_unit (tl::to_string (tr ("micron")));
  parameters.back ().set_default (0.5);

  //  parameter #7: used radius 2
  tl_assert (parameters.size () == p_actual_radius2);
  parameters.push_back (db::PCellParameterDeclaration ("actual_radius2"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Radius 2")));
  parameters.back ().set_unit (tl::to_string (tr ("micron")));
  parameters.back ().set_default (1.0);

  return parameters;
}

}



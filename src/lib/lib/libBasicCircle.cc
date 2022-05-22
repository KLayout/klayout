
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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


#include "libBasicCircle.h"

#include <cmath>

namespace lib
{

// --------------------------------------------------------------------------
//  Implementation

static const size_t p_layer = 0;
static const size_t p_radius = 1;
static const size_t p_handle = 2;
static const size_t p_npoints = 3;
static const size_t p_actual_radius = 4;
static const size_t p_total = 5;

BasicCircle::BasicCircle ()
{
  //  .. nothing yet ..
}

bool 
BasicCircle::can_create_from_shape (const db::Layout & /*layout*/, const db::Shape &shape, unsigned int /*layer*/) const
{
  return (shape.is_polygon () || shape.is_box () || shape.is_path ());
}

db::Trans
BasicCircle::transformation_from_shape (const db::Layout & /*layout*/, const db::Shape &shape, unsigned int /*layer*/) const
{
  //  use the displacement to define the center of the circle
  return db::Trans (shape.bbox ().center () - db::Point ());
}

db::pcell_parameters_type
BasicCircle::parameters_from_shape (const db::Layout &layout, const db::Shape &shape, unsigned int layer) const
{
  db::DBox dbox = db::CplxTrans (layout.dbu ()) * shape.bbox ();

  //  use map_parameters to create defaults for the other parameters
  std::map<size_t, tl::Variant> nm;
  nm.insert (std::make_pair (p_layer, tl::Variant (layout.get_properties (layer))));
  nm.insert (std::make_pair (p_actual_radius, tl::Variant (0.5 * std::min (dbox.width (), dbox.height ()))));
  return map_parameters (nm);
}

std::vector<db::PCellLayerDeclaration> 
BasicCircle::get_layer_declarations (const db::pcell_parameters_type &parameters) const
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
BasicCircle::coerce_parameters (const db::Layout & /*layout*/, db::pcell_parameters_type &parameters) const
{
  if (parameters.size () < p_total) {
    return;
  }

  double ru = parameters [p_radius].to_double ();
  double r = parameters [p_actual_radius].to_double ();

  double rs = ru;
  if (parameters [p_handle].is_user <db::DPoint> ()) {
    rs = parameters [p_handle].to_user <db::DPoint> ().distance ();
  } 

  if (fabs (ru - r) > 1e-6) {
    //  the explicit radius has changed: use it
    ru = r;
    parameters [p_handle] = db::DPoint (-r, 0);
  } else {
    //  the handle has changed: use this
    ru = rs;
    r = rs;
    parameters [p_actual_radius] = r;
  }

  //  set the hidden used radius parameter
  parameters [p_radius] = ru;
}

void 
BasicCircle::produce (const db::Layout &layout, const std::vector<unsigned int> &layer_ids, const db::pcell_parameters_type &parameters, db::Cell &cell) const
{
  if (parameters.size () < p_total || layer_ids.size () < 1) {
    return;
  }

  double r = parameters [p_radius].to_double () / layout.dbu ();
  int n = std::max (3, parameters [p_npoints].to_int ());

  std::vector <db::Point> points;
  points.reserve (n);

  //  Produce an outer circle approximation. This 
  //  one looks slightly better in the case of few points. 
  double rr = r / cos (M_PI / n);
  double da = 2.0 * M_PI / n;
  for (int i = 0; i < n; ++i) {
    double a = (i + 0.5) * da;
    points.push_back (db::Point (db::coord_traits<db::Coord>::rounded (-rr * cos (a)), db::coord_traits<db::Coord>::rounded (rr * sin (a))));
  }

  //  Produce the shape
  db::SimplePolygon poly;
  poly.assign_hull (points.begin (), points.end ());
  cell.shapes (layer_ids [p_layer]).insert (poly);
}

std::string 
BasicCircle::get_display_name (const db::pcell_parameters_type &parameters) const
{
  return "CIRCLE(l=" + std::string (parameters [p_layer].to_string ()) +
               ",r=" + tl::to_string (parameters [p_radius].to_double ()) +
               ",n=" + tl::to_string (parameters [p_npoints].to_int ()) +
                 ")";
}

std::vector<db::PCellParameterDeclaration> 
BasicCircle::get_parameter_declarations () const
{
  std::vector<db::PCellParameterDeclaration> parameters;

  //  parameter #0: layer 
  tl_assert (parameters.size () == p_layer);
  parameters.push_back (db::PCellParameterDeclaration ("layer"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_layer);
  parameters.back ().set_description (tl::to_string (tr ("Layer")));

  //  parameter #1: radius (shadow, hidden)
  //  This parameter is updated by "coerce_parameters" from "actual_radius" or "handle",
  //  whichever changed.
  tl_assert (parameters.size () == p_radius);
  parameters.push_back (db::PCellParameterDeclaration ("radius"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_hidden (true);

  //  parameter #2: handle 
  tl_assert (parameters.size () == p_handle);
  parameters.push_back (db::PCellParameterDeclaration ("handle"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  parameters.back ().set_default (db::DPoint (-1.0, 0));
  parameters.back ().set_description (tl::to_string (tr ("R")));

  //  parameter #3: number of points 
  tl_assert (parameters.size () == p_npoints);
  parameters.push_back (db::PCellParameterDeclaration ("npoints"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_int);
  parameters.back ().set_description (tl::to_string (tr ("Number of points")));
  parameters.back ().set_default (64);

  //  parameter #4: radius (entry field)
  tl_assert (parameters.size () == p_actual_radius);
  parameters.push_back (db::PCellParameterDeclaration ("actual_radius"));
  parameters.back ().set_description (tl::to_string (tr ("Radius")));
  parameters.back ().set_unit (tl::to_string (tr ("micron")));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_default (1.0);

  return parameters;
}

}



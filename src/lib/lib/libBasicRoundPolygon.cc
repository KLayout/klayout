
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


#include "libBasicRoundPolygon.h"

#include "dbPolygonTools.h"
#include "dbEdgeProcessor.h"

#include <cmath>

namespace lib
{

// --------------------------------------------------------------------------
//  Implementation

static const size_t p_layer = 0;
static const size_t p_radius = 1;
static const size_t p_polygon = 2;
static const size_t p_npoints = 3;
static const size_t p_total = 4;

BasicRoundPolygon::BasicRoundPolygon ()
{
  //  .. nothing yet ..
}

bool 
BasicRoundPolygon::can_create_from_shape (const db::Layout & /*layout*/, const db::Shape &shape, unsigned int /*layer*/) const
{
  return (shape.is_polygon () || shape.is_box () || shape.is_path ());
}

db::pcell_parameters_type
BasicRoundPolygon::parameters_from_shape (const db::Layout &layout, const db::Shape &shape, unsigned int layer) const
{
  db::Polygon poly;
  shape.polygon (poly);

  //  use map_parameters to create defaults for the other parameters
  std::map<size_t, tl::Variant> nm;
  nm.insert (std::make_pair (p_layer, tl::Variant (layout.get_properties (layer))));
  nm.insert (std::make_pair (p_polygon, tl::Variant (db::CplxTrans (layout.dbu ()) * poly)));
  //  use 1/10 of the minimum bbox dimension as a rough initialisation of the radius
  nm.insert (std::make_pair (p_radius, tl::Variant (layout.dbu () * (std::min (poly.box ().width (), poly.box ().height ()) / 10))));
  return map_parameters (nm);
}

std::vector<db::PCellLayerDeclaration> 
BasicRoundPolygon::get_layer_declarations (const db::pcell_parameters_type &parameters) const
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
BasicRoundPolygon::produce (const db::Layout &layout, const std::vector<unsigned int> &layer_ids, const db::pcell_parameters_type &parameters, db::Cell &cell) const
{
  if (parameters.size () < p_total || layer_ids.size () < 1) {
    return;
  }

  double r = parameters [p_radius].to_double () / layout.dbu ();
  int n = std::max (3, parameters [p_npoints].to_int ());

  if (! parameters [p_polygon].is_user<db::DPolygon> ()) {
    return;
  }

  //  Produce the shape
  std::vector<db::Polygon> poly;
  poly.push_back (db::complex_trans<db::DCoord, db::Coord> (1.0 / layout.dbu ()) * parameters [p_polygon].to_user<db::DPolygon> ());

  //  Merge the polygon
  db::EdgeProcessor ep;
  std::vector<db::Polygon> merged;
  ep.simple_merge (poly, merged, false);

  //  And compute the rounded polygon
  for (std::vector<db::Polygon>::const_iterator p = merged.begin (); p != merged.end (); ++p) {
    db::Polygon pr = db::compute_rounded (*p, r, r, n);
    cell.shapes (layer_ids [p_layer]).insert (pr);
  }
}

std::string 
BasicRoundPolygon::get_display_name (const db::pcell_parameters_type &parameters) const
{
  return "ROUND_POLYGON(l=" + std::string (parameters [p_layer].to_string ()) +
                      ",r=" + tl::to_string (parameters [p_radius].to_double ()) +
                      ",n=" + tl::to_string (parameters [p_npoints].to_int ()) +
                        ")";
}

std::vector<db::PCellParameterDeclaration> 
BasicRoundPolygon::get_parameter_declarations () const
{
  std::vector<db::PCellParameterDeclaration> parameters;

  //  parameter #0: layer 
  tl_assert (parameters.size () == p_layer);
  parameters.push_back (db::PCellParameterDeclaration ("layer"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_layer);
  parameters.back ().set_description (tl::to_string (tr ("Layer")));

  //  parameter #1: radius 
  tl_assert (parameters.size () == p_radius);
  parameters.push_back (db::PCellParameterDeclaration ("radius"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Radius")));
  parameters.back ().set_default (0.1);
  parameters.back ().set_unit (tl::to_string (tr ("micron")));

  //  parameter #2: handle 
  tl_assert (parameters.size () == p_polygon);
  parameters.push_back (db::PCellParameterDeclaration ("polygon"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  db::DPolygon p;
  db::DPoint pts[] = { db::DPoint(-0.2, -0.2), db::DPoint(0.2, -0.2), db::DPoint (0.2, 0.2), db::DPoint (-0.2, 0.2) };
  p.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
  parameters.back ().set_default (p);

  //  parameter #3: number of points 
  tl_assert (parameters.size () == p_npoints);
  parameters.push_back (db::PCellParameterDeclaration ("npoints"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_int);
  parameters.back ().set_description (tl::to_string (tr ("Number of points / full circle.")));
  parameters.back ().set_default (64);

  return parameters;
}

}




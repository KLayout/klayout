
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


#include "libBasicStrokedPolygon.h"
#include "dbPolygonTools.h"
#include "dbEdgeProcessor.h"

#include <cmath>

namespace lib
{

// --------------------------------------------------------------------------
//  Implementation

static const size_t p_layer = 0;
static const size_t p_radius = 1;
static const size_t p_width = 2;
static const size_t p_shape = 3;
static const size_t p_npoints = 4;
static const size_t p_total = 5;

BasicStrokedPolygon::BasicStrokedPolygon (bool box)
  : m_box (box)
{
  //  .. nothing yet ..
}

bool 
BasicStrokedPolygon::can_create_from_shape (const db::Layout & /*layout*/, const db::Shape &shape, unsigned int /*layer*/) const
{
  return (shape.is_polygon () || shape.is_box () || shape.is_path ());
}

db::pcell_parameters_type
BasicStrokedPolygon::parameters_from_shape (const db::Layout &layout, const db::Shape &shape, unsigned int layer) const
{
  db::Polygon poly;
  shape.polygon (poly);

  //  use map_parameters to create defaults for the other parameters
  std::map<size_t, tl::Variant> nm;
  nm.insert (std::make_pair (p_layer, tl::Variant (layout.get_properties (layer))));
  if (m_box) {
    nm.insert (std::make_pair (p_shape, tl::Variant (db::CplxTrans (layout.dbu ()) * poly.box ())));
  } else {
    nm.insert (std::make_pair (p_shape, tl::Variant (db::CplxTrans (layout.dbu ()) * poly)));
  }
  //  use 1/10 of the minimum bbox dimension as a rough initialisation of the width
  nm.insert (std::make_pair (p_width, tl::Variant (layout.dbu () * (std::min (poly.box ().width (), poly.box ().height ()) / 10))));
  nm.insert (std::make_pair (p_radius, tl::Variant (0.0)));
  return map_parameters (nm);
}

std::vector<db::PCellLayerDeclaration> 
BasicStrokedPolygon::get_layer_declarations (const db::pcell_parameters_type &parameters) const
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
BasicStrokedPolygon::produce (const db::Layout &layout, const std::vector<unsigned int> &layer_ids, const db::pcell_parameters_type &parameters, db::Cell &cell) const
{
  if (parameters.size () < p_total || layer_ids.size () < 1) {
    return;
  }

  double r = parameters [p_radius].to_double () / layout.dbu ();
  double w = parameters [p_width].to_double () / layout.dbu ();
  int n = std::max (3, parameters [p_npoints].to_int ());

  std::vector <db::Polygon> shapes;
  db::EdgeProcessor ep;

  //  fetch the input
  if (parameters [p_shape].is_user<db::DPolygon> ()) {
    shapes.push_back (db::Polygon ());
    shapes.back () = db::Polygon (db::DCplxTrans (1.0 / layout.dbu ()) * parameters [p_shape].to_user<db::DPolygon> ());
  } else if (parameters [p_shape].is_user<db::DBox> ()) {
    shapes.push_back (db::Polygon ());
    shapes.back () = db::Polygon (db::Box (parameters [p_shape].to_user<db::DBox> () * (1.0 / layout.dbu ())));
  }

  //  create the outer contour
  std::vector <db::Polygon> outer;
  ep.size (shapes, db::coord_traits<db::Coord>::rounded (w * 0.5), db::coord_traits<db::Coord>::rounded (w * 0.5), outer, 4, false); 
  if (r > 0.5) {
    for (std::vector <db::Polygon>::iterator p = outer.begin (); p != outer.end (); ++p) {
      *p = compute_rounded (*p, std::max (0.0, r - w * 0.5), r + w * 0.5, n);
    }
  }

  //  create the inner contour
  std::vector <db::Polygon> inner;
  ep.size (outer, -db::coord_traits<db::Coord>::rounded (w), -db::coord_traits<db::Coord>::rounded (w), inner, 4, false); 

  //  subtract inner from outer
  shapes.clear ();
  ep.boolean (outer, inner, shapes, db::BooleanOp::ANotB, true /*resolve holes*/);

  //  Produce the shapes
  for (std::vector <db::Polygon>::const_iterator p = shapes.begin (); p != shapes.end (); ++p) {
    cell.shapes (layer_ids [p_layer]).insert (*p);
  }
}

std::string 
BasicStrokedPolygon::get_display_name (const db::pcell_parameters_type &parameters) const
{
  return std::string(m_box ? "STROKED_BOX" : "STROKED_POLYGON") +
                      "(l=" + std::string (parameters [p_layer].to_string ()) +
                      ",w=" + tl::to_string (parameters [p_width].to_double ()) +
                      ",r=" + tl::to_string (parameters [p_radius].to_double ()) +
                      ",n=" + tl::to_string (parameters [p_npoints].to_int ()) +
                        ")";
}

std::vector<db::PCellParameterDeclaration> 
BasicStrokedPolygon::get_parameter_declarations () const
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
  parameters.back ().set_default (0.0);
  parameters.back ().set_unit (tl::to_string (tr ("micron")));

  //  parameter #2: width 
  tl_assert (parameters.size () == p_width);
  parameters.push_back (db::PCellParameterDeclaration ("width"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
  parameters.back ().set_description (tl::to_string (tr ("Width")));
  parameters.back ().set_default (0.1);
  parameters.back ().set_unit (tl::to_string (tr ("micron")));

  //  parameter #3: handle 
  tl_assert (parameters.size () == p_shape);
  parameters.push_back (db::PCellParameterDeclaration ("shape"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_shape);
  if (m_box) {
    parameters.back ().set_default (db::DBox (db::DPoint (-0.2, -0.2), db::DPoint (0.2, 0.2)));
  } else {
    db::DPolygon p;
    db::DPoint pts[] = { db::DPoint(-0.2, -0.2), db::DPoint(0.2, -0.2), db::DPoint (0.2, 0.2), db::DPoint (-0.2, 0.2) };
    p.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
    parameters.back ().set_default (p);
  }

  //  parameter #4: number of points 
  tl_assert (parameters.size () == p_npoints);
  parameters.push_back (db::PCellParameterDeclaration ("npoints"));
  parameters.back ().set_type (db::PCellParameterDeclaration::t_int);
  parameters.back ().set_description (tl::to_string (tr ("Number of points / full circle.")));
  parameters.back ().set_default (64);

  return parameters;
}

}




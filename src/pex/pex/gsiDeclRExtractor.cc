
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "gsiDecl.h"
#include "pexRExtractor.h"
#include "pexSquareCountingRExtractor.h"
#include "pexTriangulationRExtractor.h"
#include "gsiEnums.h"

namespace gsi
{

static pex::RExtractor *new_sqc_rextractor (double dbu, bool skip_simplify)
{
  auto res = new pex::SquareCountingRExtractor (dbu);
  res->set_skip_simplfy (skip_simplify);
  return res;
}

static pex::RExtractor *new_tesselation_rextractor (double dbu, double min_b, double max_area, bool skip_reduction)
{
  auto res = new pex::TriangulationRExtractor (dbu);
  res->triangulation_parameters ().min_b = min_b;
  res->triangulation_parameters ().max_area = max_area;
  res->set_skip_reduction (skip_reduction);
  return res;
}

static pex::RNetwork *extract_ipolygon (pex::RExtractor *rex, const db::Polygon &poly, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports)
{
  std::unique_ptr<pex::RNetwork> p_network (new pex::RNetwork ());
  rex->extract (poly, vertex_ports, polygon_ports, *p_network);
  return p_network.release ();
}

Class<pex::RExtractor> decl_RExtractor ("pex", "RExtractor",
  gsi::constructor ("square_counting_extractor", &new_sqc_rextractor, gsi::arg ("dbu"), gsi::arg ("skip_simplify", false),
    "@brief Creates a square counting R extractor\n"
    "The square counting extractor extracts resistances from a polygon with ports using the following approach:\n"
    "\n"
    "@ul\n"
    "@li Split the original polygon into convex parts using a Hertel-Mehlhorn decomposition @/li\n"
    "@li Create internal nodes at the locations where the parts touch @/li\n"
    "@li For each part, extract the resistance along the horizonal or vertical axis, whichever is longer @/li"
    "@/ul\n"
    "\n"
    "The square counting extractor assumes the parts are 'thin' - i.e. the long axis is much longer than the short "
    "axis - and the parts are either oriented horizontally or vertically. The current flow is assumed to be linear and "
    "homogenous along the long axis. Ports define probe points for the voltages along the long long axis. "
    "Polygon ports are considered points located at the center of the polygon's bounding box.\n"
    "\n"
    "The results of the extraction is normalized to a sheet resistance of 1 Ohm/square - i.e. to obtain the actual resistor "
    "values, multiply the element resistance values by the sheet resistance.\n"
    "\n"
    "@param dbu The database unit of the polygons the extractor will work on\n"
    "@param skip_simplify If true, the final step to simplify the netlist will be skipped. This feature is for testing mainly.\n"
    "@return A new \\RExtractor object that implements the square counting extractor\n"
  ) +
  gsi::constructor ("tesselation_extractor", &new_tesselation_rextractor, gsi::arg ("dbu"), gsi::arg ("min_b", 0.3), gsi::arg ("max_area", 0.0), gsi::arg ("skip_reduction", false),
    "@brief Creates a tesselation R extractor\n"
    "The tesselation extractor starts with a triangulation of the original polygon. The triangulation is "
    "turned into a resistor network and simplified.\n"
    "\n"
    "The tesselation extractor is well suited for homogeneous geometries, but does not properly consider "
    "the boundary conditions at the borders of the region. It is good for extracting resistance networks of "
    "substrate or large sheet layers.\n"
    "\n"
    "The square counting extractor assumes the parts are 'thin' - i.e. the long axis is much longer than the short "
    "axis - and the parts are either oriented horizontally or vertically. The current flow is assumed to be linear and "
    "homogenous along the long axis. Ports define probe points for the voltages along the long long axis. "
    "Polygon ports are considered points located at the center of the polygon's bounding box.\n"
    "\n"
    "The tesselation extractor delivers a full matrix of resistors - there is a resistor between every pair of ports.\n"
    "\n"
    "The results of the extraction is normalized to a sheet resistance of 1 Ohm/square - i.e. to obtain the actual resistor "
    "values, multiply the element resistance values by the sheet resistance.\n"
    "\n"
    "@param dbu The database unit of the polygons the extractor will work on\n"
    "@param min_b Defines the min 'b' value of the refined Delaunay triangulation (see \\Polygon#delaunay)\n"
    "@param max_area Defines maximum area value of the refined Delaunay triangulation (see \\Polygon#delaunay). The value is given in square micrometer units.\n"
    "@param skip_reduction If true, the reduction step for the netlist will be skipped. This feature is for testing mainly. The resulting R graph will contain all the original triangles and the internal nodes representing the vertexes.\n"
    "@return A new \\RExtractor object that implements the square counting extractor\n"
  ) +
  gsi::factory_ext ("extract", &extract_ipolygon, gsi::arg ("polygon"), gsi::arg ("vertex_ports", std::vector<db::Point> (), "[]"), gsi::arg ("polygon_ports", std::vector<db::Polygon> (), "[]"),
    "@brief Runs the extraction on the given polygon\n"
    "This method will create a new \\RNetwork object from the given polygon.\n"
    "\n"
    "'vertex_ports' is an array of points that define point-like ports. A port will create a \\RNode object in the "
    "resistor graph. This node object carries the type \\VertexPort and the index of the vertex in this array.\n"
    "\n"
    "'polygon_ports' is an array of polygons that define distributed ports. The polygons should be inside the resistor polygon "
    "and convex. A port will create a \\RNode object in the resistor graph. "
    "For polygon ports, this node object carries the type \\PolygonPort and the index of the polygon in this array.\n"
  ),
  "@brief The basic R extractor class\n"
  "\n"
  "Use \\tesselation_extractor and \\square_counting_extractor to create an actual extractor object.\n"
  "To use the extractor, call the \\extract method on a given polygon with ports that define the network attachment points.\n"
  "\n"
  "This class has been introduced in version 0.30.2\n"
);

}


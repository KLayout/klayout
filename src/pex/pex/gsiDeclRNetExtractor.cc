
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
#include "pexRExtractorTech.h"
#include "pexRNetExtractor.h"
#include "pexRNetwork.h"
#include "gsiEnums.h"

namespace gsi
{

static unsigned int via_get_bottom_conductor (const pex::RExtractorTechVia *via)
{
  return via->bottom_conductor;
}

static void via_set_bottom_conductor (pex::RExtractorTechVia *via, unsigned int l)
{
  via->bottom_conductor = l;
}

static unsigned int via_get_cut_layer (const pex::RExtractorTechVia *via)
{
  return via->cut_layer;
}

static void via_set_cut_layer (pex::RExtractorTechVia *via, unsigned int l)
{
  via->cut_layer = l;
}

static unsigned int via_get_top_conductor (const pex::RExtractorTechVia *via)
{
  return via->top_conductor;
}

static void via_set_top_conductor (pex::RExtractorTechVia *via, unsigned int l)
{
  via->top_conductor = l;
}

static double via_get_resistance (const pex::RExtractorTechVia *via)
{
  return via->resistance;
}

static void via_set_resistance (pex::RExtractorTechVia *via, double r)
{
  via->resistance = r;
}

static double via_get_merge_distance (const pex::RExtractorTechVia *via)
{
  return via->merge_distance;
}

static void via_set_merge_distance (pex::RExtractorTechVia *via, double dist)
{
  via->merge_distance = dist;
}

Class<pex::RExtractorTechVia> decl_RExtractorTechVia ("pex", "RExtractorTechVia",
  gsi::method ("to_s", &pex::RExtractorTechVia::to_string,
    "@brief Returns a string describing this object"
  ) +
  gsi::method_ext ("merge_distance", &via_get_merge_distance,
    "@brief Gets the merge distance\n"
    "If this value is not zero, it specifies the distance below (or equal) which "
    "vias are merged into bigger blocks. This is an optimization to reduce the "
    "complexity of the via extraction. The value is given in micrometers."
  ) +
  gsi::method_ext ("merge_distance=", &via_set_merge_distance, gsi::arg ("d"),
    "@brief Sets the merge distance\n"
    "See \\merge_distance for a description of this attribute."
  ) +
  gsi::method_ext ("resistance", &via_get_resistance,
    "@brief Gets the area resistance value of the vias\n"
    "This value specifies the via resistance in Ohm * square micrometers. "
    "The actual resistance is obtained by dividing this value by the via area."
  ) +
  gsi::method_ext ("resistance=", &via_set_resistance, gsi::arg ("d"),
    "@brief Sets the via area resistance value\n"
    "See \\resistance for a description of this attribute."
  ) +
  gsi::method_ext ("bottom_conductor", &via_get_bottom_conductor,
    "@brief Gets the bottom conductor layer index\n"
    "The layer index is a generic identifier for the layer. It is the value used as key in the "
    "geometry and port arguments of \\RNetExtractor#extract."
  ) +
  gsi::method_ext ("bottom_conductor=", &via_set_bottom_conductor, gsi::arg ("l"),
    "@brief Sets the via bottom conductor layer index\n"
    "See \\bottom_conductor for a description of this attribute."
  ) +
  gsi::method_ext ("cut_layer", &via_get_cut_layer,
    "@brief Gets the cut layer index\n"
    "The layer index is a generic identifier for the layer. It is the value used as key in the "
    "geometry and port arguments of \\RNetExtractor#extract. "
    "The cut layer is the layer where the via exists."
  ) +
  gsi::method_ext ("cut_layer=", &via_set_cut_layer, gsi::arg ("l"),
    "@brief Sets the cut layer index\n"
    "See \\cut_layer for a description of this attribute."
  ) +
  gsi::method_ext ("top_conductor", &via_get_top_conductor,
    "@brief Gets the top conductor layer index\n"
    "The layer index is a generic identifier for the layer. It is the value used as key in the "
    "geometry and port arguments of \\RNetExtractor#extract."
  ) +
  gsi::method_ext ("top_conductor=", &via_set_top_conductor, gsi::arg ("l"),
    "@brief Sets the via top conductor layer index\n"
    "See \\top_conductor for a description of this attribute."
  ),
  "@brief Describes a via for the network extraction.\n"
  "This class is used to describe a via type in the context of "
  "the \\RExtractorTech class.\n"
  "\n"
  "This class has been introduced in version 0.30.2."
);

static pex::RExtractorTechConductor::Algorithm cond_get_algorithm (const pex::RExtractorTechConductor *cond)
{
  return cond->algorithm;
}

static void cond_set_algorithm (pex::RExtractorTechConductor *cond, pex::RExtractorTechConductor::Algorithm a)
{
  cond->algorithm = a;
}

static unsigned int cond_get_layer (const pex::RExtractorTechConductor *cond)
{
  return cond->layer;
}

static void cond_set_layer (pex::RExtractorTechConductor *cond, unsigned int l)
{
  cond->layer = l;
}

static double cond_get_resistance (const pex::RExtractorTechConductor *cond)
{
  return cond->resistance;
}

static void cond_set_resistance (pex::RExtractorTechConductor *cond, double r)
{
  cond->resistance = r;
}

static double cond_get_triangulation_min_b (const pex::RExtractorTechConductor *cond)
{
  return cond->triangulation_min_b;
}

static void cond_set_triangulation_min_b (pex::RExtractorTechConductor *cond, double min_b)
{
  cond->triangulation_min_b = min_b;
}

static double cond_get_triangulation_max_area (const pex::RExtractorTechConductor *cond)
{
  return cond->triangulation_max_area;
}

static void cond_set_triangulation_max_area (pex::RExtractorTechConductor *cond, double max_area)
{
  cond->triangulation_max_area = max_area;
}

Class<pex::RExtractorTechConductor> decl_RExtractorTechConductor ("pex", "RExtractorTechConductor",
  gsi::method ("to_s", &pex::RExtractorTechConductor::to_string,
    "@brief Returns a string describing this object"
  ) +
  gsi::method_ext ("algorithm", &cond_get_algorithm,
    "@brief Gets the algorithm to use\n"
    "Specifies the algorithm to use. The default algorithm is 'SquareCounting'."
  ) +
  gsi::method_ext ("algorithm=", &cond_set_algorithm, gsi::arg ("d"),
    "@brief Sets the algorithm to use\n"
    "See \\algorithm for a description of this attribute."
  ) +
  gsi::method_ext ("resistance", &cond_get_resistance,
    "@brief Gets the sheet resistance value of the conductor layer\n"
    "This value specifies the cond resistance in Ohm per square. "
    "The actual resistance is obtained by multiplying this value with the number of squares."
  ) +
  gsi::method_ext ("resistance=", &cond_set_resistance, gsi::arg ("r"),
    "@brief Sets the sheet resistance value of the conductor layer\n"
    "See \\resistance for a description of this attribute."
  ) +
  gsi::method_ext ("layer", &cond_get_layer,
    "@brief Gets the layer index\n"
    "The layer index is a generic identifier for the layer. It is the value used as key in the "
    "geometry and port arguments of \\RNetExtractor#extract. "
    "This attribute specifies the layer the conductor is on."
  ) +
  gsi::method_ext ("layer=", &cond_set_layer, gsi::arg ("l"),
    "@brief Sets the layer index\n"
    "See \\layer for a description of this attribute."
  ) +
  gsi::method_ext ("triangulation_min_b", &cond_get_triangulation_min_b,
    "@brief Gets the triangulation 'min_b' parameter\n"
    "This parameter is used for the 'Tesselation' algorithm and specifies the shortest edge to circle radius ratio of "
    "the Delaunay triangulation. "
  ) +
  gsi::method_ext ("triangulation_min_b=", &cond_set_triangulation_min_b, gsi::arg ("min_b"),
    "@brief Sets the triangulation 'min_b' parameter\n"
    "See \\triangulation_min_b for a description of this attribute."
  ) +
  gsi::method_ext ("triangulation_max_area", &cond_get_triangulation_max_area,
    "@brief Gets the triangulation 'max_area' parameter\n"
    "This parameter is used for the 'Tesselation' algorithm and specifies the maximum area of "
    "the triangles in square micrometers."
  ) +
  gsi::method_ext ("triangulation_max_area=", &cond_set_triangulation_max_area, gsi::arg ("max_area"),
    "@brief Sets the triangulation 'max_area' parameter\n"
    "See \\triangulation_max_area for a description of this attribute."
  ),
  "@brief Describes a conductor layer for the network extraction.\n"
  "This class is used to describe a conductor layer in the context of "
  "the \\RExtractorTech class.\n"
  "\n"
  "This class has been introduced in version 0.30.2."
);

gsi::Enum<pex::RExtractorTechConductor::Algorithm> decl_RExtractorTechConductor_Algorithm ("pex", "Algorithm",
  gsi::enum_const ("SquareCounting", pex::RExtractorTechConductor::SquareCounting,
    "@brief Specifies the square counting algorithm for \\RExtractorTechConductor#algorithm.\n"
    "See \\RExtractor#square_counting_extractor for more details."
  ) +
  gsi::enum_const ("Tesselation", pex::RExtractorTechConductor::Tesselation,
    "@brief Specifies the square counting algorithm for \\RExtractorTechConductor#algorithm.\n"
    "See \\RExtractor#tesselation_extractor for more details."
  ),
  "@brief This enum represents the extraction algorithm for \\RExtractorTechConductor.\n"
  "\n"
  "This enum has been introduced in version 0.30.2."
);

gsi::ClassExt<pex::RExtractorTechConductor> inject_RExtractorTechConductor_in_parent (decl_RExtractorTechConductor_Algorithm.defs ());

static bool tech_get_skip_simplify (const pex::RExtractorTech *tech)
{
  return tech->skip_simplify;
}

static void tech_set_skip_simplify (pex::RExtractorTech *tech, bool f)
{
  tech->skip_simplify = f;
}

static std::list<pex::RExtractorTechVia>::const_iterator tech_begin_vias (const pex::RExtractorTech *tech)
{
  return tech->vias.begin ();
}

static std::list<pex::RExtractorTechVia>::const_iterator tech_end_vias (const pex::RExtractorTech *tech)
{
  return tech->vias.end ();
}

static void tech_clear_vias (pex::RExtractorTech *tech)
{
  tech->vias.clear ();
}

static void tech_add_via (pex::RExtractorTech *tech, const pex::RExtractorTechVia &via)
{
  tech->vias.push_back (via);
}

static std::list<pex::RExtractorTechConductor>::const_iterator tech_begin_conductors (const pex::RExtractorTech *tech)
{
  return tech->conductors.begin ();
}

static std::list<pex::RExtractorTechConductor>::const_iterator tech_end_conductors (const pex::RExtractorTech *tech)
{
  return tech->conductors.end ();
}

static void tech_clear_conductors (pex::RExtractorTech *tech)
{
  tech->conductors.clear ();
}

static void tech_add_conductor (pex::RExtractorTech *tech, const pex::RExtractorTechConductor &conductor)
{
  tech->conductors.push_back (conductor);
}

Class<pex::RExtractorTech> decl_RExtractorTech ("pex", "RExtractorTech",
  gsi::method ("to_s", &pex::RExtractorTech::to_string,
    "@brief Returns a string describing this object"
  ) +
  gsi::method_ext ("skip_simplify", &tech_get_skip_simplify,
    "@brief Gets a value indicating whether to skip the simplify step\n"
    "This values specifies to skip the simplify step of the network after the extraction has "
    "been done. By default, the network is simplified - i.e. serial resistors are joined etc. "
    "By setting this attribute to 'false', this step is skipped."
  ) +
  gsi::method_ext ("skip_simplify=", &tech_set_skip_simplify, gsi::arg ("f"),
    "@brief Sets a value indicating whether to skip the simplify step\n"
    "See \\skip_simplify for a description of this attribute."
  ) +
  gsi::iterator_ext ("each_via", &tech_begin_vias, &tech_end_vias,
    "@brief Iterates the list of via definitions\n"
  ) +
  gsi::method_ext ("clear_vias", &tech_clear_vias,
    "@brief Clears the list of via definitions\n"
  ) +
  gsi::method_ext ("add_via", &tech_add_via, gsi::arg ("via"),
    "@brief Adds the given via definition to the list of vias\n"
  ) +
  gsi::iterator_ext ("each_conductor", &tech_begin_conductors, &tech_end_conductors,
    "@brief Iterates the list of conductor definitions\n"
  ) +
  gsi::method_ext ("clear_conductors", &tech_clear_conductors,
    "@brief Clears the list of conductor definitions\n"
  ) +
  gsi::method_ext ("add_conductor", &tech_add_conductor, gsi::arg ("conductor"),
    "@brief Adds the given conductor definition to the list of conductors\n"
  ),
  "@brief Specifies the tech stack for the R extraction.\n"
  "The tech stack is a collection of via and conductor definitions and some other attributes. "
  "It is used for the \\RNetExtractor#extract method.\n"
  "\n"
  "This enum has been introduced in version 0.30.2."
);

static pex::RNetExtractor *new_net_rextractor (double dbu)
{
  return new pex::RNetExtractor (dbu);
}

static pex::RNetwork *rex_extract (pex::RNetExtractor *rex,
                                   const pex::RExtractorTech *tech,
                                   const std::map<unsigned int, db::Region> *geo,
                                   const std::map<unsigned int, std::vector<db::Point> > *vertex_ports,
                                   const std::map<unsigned int, std::vector<db::Polygon> > *polygon_ports)
{
  std::unique_ptr<pex::RNetwork> network (new pex::RNetwork ());
  std::map<unsigned int, db::Region> empty_geo;
  std::map<unsigned int, std::vector<db::Point> > empty_vertex_ports;
  std::map<unsigned int, std::vector<db::Polygon> > empty_polygon_ports;
  rex->extract (*tech, geo ? *geo : empty_geo, vertex_ports ? *vertex_ports : empty_vertex_ports, polygon_ports ? *polygon_ports : empty_polygon_ports, *network);
  return network.release ();
}

Class<pex::RNetExtractor> decl_RNetExtractor ("pex", "RNetExtractor",
  gsi::constructor ("new", &new_net_rextractor, gsi::arg ("dbu"),
    "@brief Creates a network R extractor\n"
    "\n"
    "@param dbu The database unit of the polygons the extractor will work on\n"
    "@param skip_simplify If true, the final step to simplify the netlist will be skipped. This feature is for testing mainly.\n"
    "@return A new \\RNetExtractor object that implements the net extractor\n"
  ) +
  gsi::factory_ext ("extract", &rex_extract, gsi::arg ("tech_stack"), gsi::arg ("geo"), gsi::arg ("vertex_ports"), gsi::arg ("polygon_ports"),
    "@brief Runs the extraction on the given multi-layer geometry\n"
    "See the description of the class for more details."
  ),
  "@brief The network R extractor class\n"
  "\n"
  "This class provides the algorithms for extracting a R network from a multi-layer arrangement of conductors and vias.\n"
  "The main feature is the \\extract method. It takes a multi-layer geometry, a tech stack and a number of port definitions\n"
  "and returns a R network. The nodes in that network are annotated, so the corresponding port can be deduced from a node of\n"
  "VertexPort or PolygonPort type.\n"
  "\n"
  "Layers are given by layer indexes - those are generic IDs. Every layer has to be given a unique ID, which must be used throughout "
  "the different specifications (geometry, vias, conductors, ports).\n"
  "\n"
  "Two kind of ports are provided: point-like vertex ports and polygon ports. Polygons for polygon ports should be convex and sit inside "
  "the geometry they mark. Ports become nodes in the network. Beside ports, the network can have internal nodes. Nodes are annotated with "
  "a type (vertex, polygon, internal) and an index and layer. The layer is the layer ID, the index specifies the position of the "
  "corresponding port in the 'vertex_ports' or 'polygon_ports' list of the \\extract call.\n"
  "\n"
  "This class has been introduced in version 0.30.2\n"
);

}


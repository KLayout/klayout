
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


#include "dbNetTracer.h"
#include "dbNetTracerIO.h"

#include "gsiDecl.h"

namespace gsi
{

// -----------------------------------------------------------------------------------
//  GSI binding

static void def_connection2 (db::NetTracerConnectivity *tech, const std::string &la, const std::string &lb)
{
  db::NetTracerLayerExpressionInfo la_info = db::NetTracerLayerExpressionInfo::compile (la);
  db::NetTracerLayerExpressionInfo lb_info = db::NetTracerLayerExpressionInfo::compile (lb);
  tech->add (db::NetTracerConnectionInfo (la_info, lb_info));
}

static void def_connection3 (db::NetTracerConnectivity *tech, const std::string &la, const std::string &via, const std::string &lb)
{
  db::NetTracerLayerExpressionInfo la_info = db::NetTracerLayerExpressionInfo::compile (la);
  db::NetTracerLayerExpressionInfo via_info = db::NetTracerLayerExpressionInfo::compile (via);
  db::NetTracerLayerExpressionInfo lb_info = db::NetTracerLayerExpressionInfo::compile (lb);
  tech->add (db::NetTracerConnectionInfo (la_info, via_info, lb_info));
}

static void def_symbol (db::NetTracerConnectivity *tech, const std::string &name, const std::string &expr)
{
  tech->add_symbol (db::NetTracerSymbolInfo (db::LayerProperties (name), expr));
}

static std::string get_layer_a (const db::NetTracerConnectionInfo *info)
{
  return info->layer_a ().to_string ();
}

static std::string get_via_layer (const db::NetTracerConnectionInfo *info)
{
  return info->via_layer ().to_string ();
}

static std::string get_layer_b (const db::NetTracerConnectionInfo *info)
{
  return info->layer_b ().to_string ();
}

gsi::Class<db::NetTracerConnectionInfo> decl_NetTracerConnectionInfo ("db", "NetTracerConnectionInfo",
  gsi::method_ext ("layer_a", &get_layer_a, "@brief Gets the expression for the A layer") +
  gsi::method_ext ("via_layer", &get_via_layer, "@brief Gets the expression for the Via layer") +
  gsi::method_ext ("layer_b", &get_layer_b, "@brief Gets the expression for the B layer"),
  "@brief Represents a single connection info line for the net tracer technology definition\n"
  "This class has been introduced in version 0.28.3."
);

static std::string get_symbol (const db::NetTracerSymbolInfo *info)
{
  return info->symbol ().to_string ();
}

gsi::Class<db::NetTracerSymbolInfo> decl_NetTracerSymbolInfo ("db", "NetTracerSymbolInfo",
  gsi::method_ext ("symbol", &get_symbol, "@brief Gets the symbol") +
  gsi::method ("expression", &db::NetTracerSymbolInfo::expression, "@brief Gets the expression"),
  "@brief Represents a single symbol info line for the net tracer technology definition\n"
  "This class has been introduced in version 0.28.3."
);

gsi::Class<db::NetTracerConnectivity> decl_NetTracerConnectivity ("db", "NetTracerConnectivity",
  gsi::method ("name", &db::NetTracerConnectivity::name,
    "@brief Gets the name of the connectivty definition\n"
    "The name is an optional string defining the formal name for this definition.\n"
  ) +
  gsi::method ("name=", &db::NetTracerConnectivity::set_name, gsi::arg ("n"),
    "@brief Sets the name of the connectivty definition\n"
  ) +
  gsi::method ("description", &db::NetTracerConnectivity::description,
    "@brief Gets the description text of the connectivty definition\n"
    "The description is an optional string giving a human-readable description for this definition."
  ) +
  gsi::method ("description=", &db::NetTracerConnectivity::set_description, gsi::arg ("d"),
    "@brief Sets the description of the connectivty definition\n"
  ) +
  gsi::method_ext ("connection", &def_connection2, gsi::arg("a"), gsi::arg("b"),
    "@brief Defines a connection between two materials\n"
    "See the class description for details about this method."
  ) +
  gsi::method_ext ("connection", &def_connection3, gsi::arg("a"), gsi::arg("via"), gsi::arg("b"),
    "@brief Defines a connection between materials through a via\n"
    "See the class description for details about this method."
  ) +
  gsi::method_ext ("symbol", &def_symbol, gsi::arg("name"), gsi::arg("expr"),
    "@brief Defines a symbol for use in the material expressions.\n"
    "Defines a sub-expression to be used in further symbols or material expressions. "
    "For the detailed notation of the expression see the description of the net tracer feature."
  ) +
  gsi::iterator ("each_connection", static_cast<db::NetTracerConnectivity::const_iterator (db::NetTracerConnectivity::*) () const> (&db::NetTracerConnectivity::begin), static_cast<db::NetTracerConnectivity::const_iterator (db::NetTracerConnectivity::*) () const> (&db::NetTracerConnectivity::end),
    "@brief Gets the connection information.\n"
    "This iterator method has been introduced in version 0.28.3.\n"
  ) +
  gsi::iterator ("each_symbol", static_cast<db::NetTracerConnectivity::const_symbol_iterator (db::NetTracerConnectivity::*) () const> (&db::NetTracerConnectivity::begin_symbols), static_cast<db::NetTracerConnectivity::const_symbol_iterator (db::NetTracerConnectivity::*) () const> (&db::NetTracerConnectivity::end_symbols),
    "@brief Gets the symbol information.\n"
    "This iterator method has been introduced in version 0.28.3.\n"
  ),
  "@brief A connectivity description for the net tracer\n"
  "\n"
  "This object represents the technology description for the net tracer (represented by the \\NetTracer class).\n"
  "A technology description basically consists of connection declarations.\n"
  "A connection is given by either two or three expressions describing two conductive materials.\n"
  "With two expressions, the connection describes a transition from one material to another one.\n"
  "With three expressions, the connection describes a transition from one material to another through a "
  "connection (a \"via\").\n"
  "\n"
  "The conductive material is derived from original layers either directly or through "
  "boolean expressions. These expressions can include symbols which are defined through the "
  "\\symbol method.\n"
  "\n"
  "For details about the expressions see the description of the net tracer feature.\n"
  "\n"
  "This class has been introduced in version 0.28 and replaces the 'NetTracerTechnology' class which "
  "has been generalized.\n"
);

DB_PUBLIC gsi::Class<db::TechnologyComponent> &decl_dbTechnologyComponent ();

gsi::Class<db::NetTracerTechnologyComponent> decl_NetTracerTechnologyComponent (decl_dbTechnologyComponent (), "db", "NetTracerTechnologyComponent",
  gsi::iterator ("each", static_cast<db::NetTracerTechnologyComponent::const_iterator (db::NetTracerTechnologyComponent::*) () const> (&db::NetTracerTechnologyComponent::begin), static_cast<db::NetTracerTechnologyComponent::const_iterator (db::NetTracerTechnologyComponent::*) () const> (&db::NetTracerTechnologyComponent::end),
    "@brief Gets the connectivity definitions from the net tracer technology component.\n"
  ) +
  gsi::method ("clear", &db::NetTracerTechnologyComponent::clear,
    "@brief Removes all connectivity definitions.\n"
    "This method has been introduced in version 0.28.7"
  ) +
  gsi::method ("add", &db::NetTracerTechnologyComponent::push_back, gsi::arg ("connection"),
    "@brief Adds a connectivity definition.\n"
    "This method has been introduced in version 0.28.7"
  ),
  "@brief Represents the technology information for the net tracer.\n"
  "This class has been redefined in version 0.28 and re-introduced in version 0.28.3. Since version 0.28, "
  "multiple stacks are supported and the individual stack definition is provided through a list of stacks. Use \\each "
  "to iterate the stacks."
);

static void trace1 (db::NetTracer *net_tracer, const db::NetTracerConnectivity &tech, const db::Layout &layout, const db::Cell &cell, const db::Point &start_point, unsigned int start_layer)
{
  db::NetTracerData tracer_data = tech.get_tracer_data (layout);
  net_tracer->trace (layout, cell, start_point, start_layer, tracer_data);
}

static void trace2 (db::NetTracer *net_tracer, const db::NetTracerConnectivity &tech, const db::Layout &layout, const db::Cell &cell, const db::Point &start_point, unsigned int start_layer, const db::Point &stop_point, unsigned int stop_layer)
{
  db::NetTracerData tracer_data = tech.get_tracer_data (layout);
  net_tracer->trace (layout, cell, start_point, start_layer, stop_point, stop_layer, tracer_data);
}

static db::NetTracerData get_tracer_data_from_tech (const std::string &tech_name, const db::Layout &layout)
{
  const db::Technology *tech = db::Technologies::instance ()->technology_by_name (tech_name);
  tl_assert (tech != 0);

  const db::NetTracerTechnologyComponent *tech_component = dynamic_cast <const db::NetTracerTechnologyComponent *> (tech->component_by_name (db::net_tracer_component_name ()));
  tl_assert (tech_component != 0);

  if (tech_component->size () < 1) {
    throw tl::Exception (tl::to_string (tr ("No connectivity setup exists for technology '%s'")), tech_name);
  }
  if (tech_component->size () > 1) {
    throw tl::Exception (tl::to_string (tr ("Multiple connectivity setups exist for technology '%s' - specify a name")), tech_name);
  }

  return tech_component->begin ()->get_tracer_data (layout);
}

static db::NetTracerData get_tracer_data_from_tech (const std::string &tech_name, const std::string &name, const db::Layout &layout)
{
  const db::Technology *tech = db::Technologies::instance ()->technology_by_name (tech_name);
  tl_assert (tech != 0);

  const db::NetTracerTechnologyComponent *tech_component = dynamic_cast <const db::NetTracerTechnologyComponent *> (tech->component_by_name (db::net_tracer_component_name ()));
  tl_assert (tech_component != 0);

  for (auto t = tech_component->begin (); t != tech_component->end (); ++t) {
    if (t->name () == name) {
      return t->get_tracer_data (layout);
    }
  }

  throw tl::Exception (tl::to_string (tr ("No connectivity setup exists with name '%s' for technology '%s'")), name, tech_name);
}

static void trace1_tn (db::NetTracer *net_tracer, const std::string &tech, const db::Layout &layout, const db::Cell &cell, const db::Point &start_point, unsigned int start_layer)
{
  db::NetTracerData tracer_data = get_tracer_data_from_tech (tech, layout);
  net_tracer->trace (layout, cell, start_point, start_layer, tracer_data);
}

static void trace1_tn2 (db::NetTracer *net_tracer, const std::string &tech, const std::string &name, const db::Layout &layout, const db::Cell &cell, const db::Point &start_point, unsigned int start_layer)
{
  db::NetTracerData tracer_data = get_tracer_data_from_tech (tech, name, layout);
  net_tracer->trace (layout, cell, start_point, start_layer, tracer_data);
}

static void trace2_tn (db::NetTracer *net_tracer, const std::string &tech, const db::Layout &layout, const db::Cell &cell, const db::Point &start_point, unsigned int start_layer, const db::Point &stop_point, unsigned int stop_layer)
{
  db::NetTracerData tracer_data = get_tracer_data_from_tech (tech, layout);
  net_tracer->trace (layout, cell, start_point, start_layer, stop_point, stop_layer, tracer_data);
}

static void trace2_tn2 (db::NetTracer *net_tracer, const std::string &tech, const std::string &name, const db::Layout &layout, const db::Cell &cell, const db::Point &start_point, unsigned int start_layer, const db::Point &stop_point, unsigned int stop_layer)
{
  db::NetTracerData tracer_data = get_tracer_data_from_tech (tech, name, layout);
  net_tracer->trace (layout, cell, start_point, start_layer, stop_point, stop_layer, tracer_data);
}

gsi::Class<db::NetTracerShape> decl_NetElement ("db", "NetElement",
  gsi::method ("trans", &db::NetTracerShape::trans,
    "@brief Gets the transformation to apply for rendering the shape in the original top cell\n"
    "See the class description for more details about this attribute."
  ) +
  gsi::method ("shape", (const db::Shape &(db::NetTracerShape::*) () const) &db::NetTracerShape::shape,
    "@brief Gets the shape that makes up this net element\n"
    "See the class description for more details about this attribute."
  ) +
#if 0
  gsi::method ("is_valid?", &db::NetTracerShape::is_valid,
    "@brief Gets a value indicating whether the shape is valid\n"
    "Currently this flag is not used."
  ) +
  gsi::method ("is_pseudo?", &db::NetTracerShape::is_pseudo,
    "@brief Gets a value indicating whether the shape is a pseudo shape\n"
    "Currently this flag is not used."
  ) +
#endif
  gsi::method ("cell_index", &db::NetTracerShape::cell_index,
    "@brief Gets the index of the cell the shape is inside"
  ) +
  gsi::method ("layer", &db::NetTracerShape::layer,
    "@brief Gets the index of the layer the shape is on"
  ) +
  gsi::method ("bbox", &db::NetTracerShape::bbox,
    "@brief Delivers the bounding box of the shape as seen from the original top cell"
  ),
  "@brief A net element for the NetTracer net tracing facility\n"
  "\n"
  "This object represents a piece of a net extracted by the net tracer. "
  "See the description of \\NetTracer for more details about the net tracer feature.\n"
  "\n"
  "The NetTracer object represents one shape of the net. The shape can be an original shape or a shape derived in a boolean operation. "
  "In the first case, the shape refers to a shape within a cell or a subcell of the original top cell. In the latter case, the shape "
  "is a synthesized one and outside the original layout hierarchy.\n"
  "\n"
  "In any case, the \\shape method will deliver the shape and \\trans the transformation of the shape into the original top cell. "
  "To obtain a flat representation of the net, the shapes need to be transformed by this transformation.\n"
  "\n"
  "\\layer will give the layer the shape is located at, \\cell_index will denote the cell that contains the shape.\n"
  "\n"
  "This class has been introduced in version 0.25.\n"
);

gsi::Class<db::NetTracer> decl_NetTracer ("db", "NetTracer",
  gsi::method_ext ("trace", &trace1, gsi::arg ("tech"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("start_point"), gsi::arg ("start_layer"),
    "@brief Runs a net extraction\n"
    "\n"
    "This method runs an extraction with the given parameters.\n"
    "To make the extraction successful, a shape must be present at the given start point on the start layer. "
    "The start layer must be a valid layer mentioned within the technology specification.\n"
    "\n"
    "This version runs a single extraction - i.e. it will extract all elements connected to the given seed point. "
    "A path extraction version is provided as well which will extract one (the presumably shortest) path between two "
    "points.\n"
    "\n"
    "@param tech The connectivity definition\n"
    "@param layout The layout on which to run the extraction\n"
    "@param cell The cell on which to run the extraction (child cells will be included)\n"
    "@param start_point The start point from which to start extraction of the net\n"
    "@param start_layer The layer from which to start extraction\n"
  ) +
  gsi::method_ext ("trace", &trace2, gsi::arg ("tech"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("start_point"), gsi::arg ("start_layer"), gsi::arg ("stop_point"), gsi::arg ("stop_layer"),
    "@brief Runs a path extraction\n"
    "\n"
    "This method runs an path extraction with the given parameters.\n"
    "To make the extraction successful, a shape must be present at the given start point on the start layer and "
    "at the given stop point at the given stop layer. "
    "The start and stop layers must be a valid layers mentioned within the technology specification.\n"
    "\n"
    "This version runs a path extraction and will deliver elements forming one path leading from the start to the end point.\n"
    "\n"
    "@param tech The connectivity definition\n"
    "@param layout The layout on which to run the extraction\n"
    "@param cell The cell on which to run the extraction (child cells will be included)\n"
    "@param start_point The start point from which to start extraction of the net\n"
    "@param start_layer The layer from which to start extraction\n"
    "@param stop_point The stop point at which to stop extraction of the net\n"
    "@param stop_layer The layer at which to stop extraction\n"
  ) +
  gsi::method_ext ("trace", &trace1_tn, gsi::arg ("tech"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("start_point"), gsi::arg ("start_layer"),
    "@brief Runs a net extraction taking a predefined technology\n"
    "This method behaves identical as the version with a technology object, except that it will look for a technology "
    "with the given name to obtain the extraction setup.\n"
    "The technology is looked up by technology name. A version of this method exists where it is possible "
    "to specify the name of the particular connectivity to use in case there are multiple definitions available."
  ) +
  gsi::method_ext ("trace", &trace1_tn2, gsi::arg ("tech"), gsi::arg ("connectivity_name"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("start_point"), gsi::arg ("start_layer"),
    "@brief Runs a net extraction taking a predefined technology\n"
    "This method behaves identical as the version with a technology object, except that it will look for a technology "
    "with the given name to obtain the extraction setup. "
    "This version allows specifying the name of the connecvitiy setup.\n"
    "\n"
    "This method variant has been introduced in version 0.28."
  ) +
  gsi::method_ext ("trace", &trace2_tn, gsi::arg ("tech"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("start_point"), gsi::arg ("start_layer"), gsi::arg ("stop_point"), gsi::arg ("stop_layer"),
    "@brief Runs a path extraction taking a predefined technology\n"
    "This method behaves identical as the version with a technology object, except that it will look for a technology "
    "with the given name to obtain the extraction setup."
  ) +
  gsi::method_ext ("trace", &trace2_tn2, gsi::arg ("tech"), gsi::arg ("connectivity_name"), gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("start_point"), gsi::arg ("start_layer"), gsi::arg ("stop_point"), gsi::arg ("stop_layer"),
    "@brief Runs a path extraction taking a predefined technology\n"
    "This method behaves identical as the version with a technology object, except that it will look for a technology "
    "with the given name to obtain the extraction setup."
    "This version allows specifying the name of the connecvitiy setup.\n"
    "\n"
    "This method variant has been introduced in version 0.28."
  ) +
  gsi::iterator ("each_element", &db::NetTracer::begin, &db::NetTracer::end,
    "@brief Iterates over the elements found during extraction\n"
    "The elements are available only after the extraction has been performed."
  ) +
  gsi::method ("num_elements", &db::NetTracer::size,
    "@brief Returns the number of elements found during extraction\n"
    "This attribute is useful only after the extraction has been performed."
  ) +
  gsi::method ("clear", &db::NetTracer::clear,
    "@brief Clears the data from the last extraction\n"
  ) +
  gsi::method ("name", &db::NetTracer::name,
    "@brief Returns the name of the net found during extraction\n"
    "The net name is extracted from labels found during the extraction. "
    "This attribute is useful only after the extraction has been performed."
  ) +
  gsi::method ("trace_depth=", &db::NetTracer::set_trace_depth, gsi::arg ("n"),
    "@brief Sets the trace depth (shape limit)\n"
    "Set this value to limit the maximum number of shapes delivered. Upon reaching this count, "
    "the tracer will stop and report the net as 'incomplete' (see \\incomplete?).\n"
    "Setting a trace depth if 0 is equivalent to 'unlimited'.\n"
    "The actual number of shapes delivered may be a little less than the depth because of "
    "internal marker shapes which are taken into account, but are not delivered.\n"
    "\n"
    "This method has been introduced in version 0.26.4.\n"
  ) +
  gsi::method ("trace_depth", &db::NetTracer::trace_depth,
    "@brief gets the trace depth\n"
    "See \\trace_depth= for a description of this property.\n"
    "\n"
    "This method has been introduced in version 0.26.4.\n"
  ) +
  gsi::method ("incomplete?", &db::NetTracer::incomplete,
    "@brief Returns a value indicating whether the net is incomplete\n"
    "A net may be incomplete if the extraction has been stopped by the user for example. "
    "This attribute is useful only after the extraction has been performed."
  ),
  "@brief The net tracer feature\n"
  "\n"
  "The net tracer class provides an interface to the net tracer feature. It is accompanied by the \\NetElement and \\NetTracerTechnology classes. "
  "The latter will provide the technology definition for the net tracer while the \\NetElement objects represent a piece of the net "
  "after it has been extracted.\n"
  "\n"
  "The technology definition is optional. The net tracer can be used with a predefined technology as well. The basic "
  "scheme of using the net tracer is to instantiate a net tracer object and run the extraction through the \\NetTracer#trace "
  "method. After this method was executed successfully, the resulting net can be obtained from the net tracer object by "
  "iterating over the \\NetElement objects of the net tracer.\n"
  "\n"
  "Here is some sample code:\n"
  "\n"
  "@code\n"
  "ly = RBA::CellView::active.layout\n"
  "\n"
  "tracer = RBA::NetTracer::new\n"
  "\n"
  "tech = RBA::NetTracerConnectivity::new\n"
  "tech.connection(\"1/0\", \"2/0\", \"3/0\")\n"
  "\n"
  "tracer.trace(tech, ly, ly.top_cell, RBA::Point::new(7000, 1500), ly.find_layer(1, 0))\n"
  "\n"
  "tracer.each_element do |e|\n"
  "  puts e.shape.polygon.transformed(e.trans)\n"
  "end\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.25. With version 0.28, the \\NetTracerConnectivity class replaces the 'NetTracerTechnology' class."
);

}

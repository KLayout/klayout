
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
#include "dbLayoutToNetlist.h"
#include "dbLayoutToNetlistWriter.h"
#include "tlStream.h"

namespace gsi
{

static db::LayoutToNetlist *make_l2n (const db::RecursiveShapeIterator &iter)
{
  return new db::LayoutToNetlist (iter);
}

static db::Layout *l2n_internal_layout (db::LayoutToNetlist *l2n)
{
  //  although this isn't very clean, we dare to do so as const references are pretty useless in script languages.
  return const_cast<db::Layout *> (l2n->internal_layout ());
}

static db::Cell *l2n_internal_top_cell (db::LayoutToNetlist *l2n)
{
  //  although this isn't very clean, we dare to do so as const references are pretty useless in script languages.
  return const_cast<db::Cell *> (l2n->internal_top_cell ());
}

static void build_net (const db::LayoutToNetlist *l2n, const db::Net &net, db::Layout &target, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const tl::Variant &circuit_cell_name_prefix, const tl::Variant &device_cell_name_prefix)
{
  std::string p = circuit_cell_name_prefix.to_string ();
  std::string dp = device_cell_name_prefix.to_string ();
  l2n->build_net (net, target, target_cell, lmap, circuit_cell_name_prefix.is_nil () ? 0 : p.c_str (), device_cell_name_prefix.is_nil () ? 0 : dp.c_str ());
}

static void build_all_nets (const db::LayoutToNetlist *l2n, const db::CellMapping &cmap, db::Layout &target, const std::map<unsigned int, const db::Region *> &lmap, const tl::Variant &net_cell_name_prefix, const tl::Variant &circuit_cell_name_prefix, const tl::Variant &device_cell_name_prefix)
{
  std::string cp = circuit_cell_name_prefix.to_string ();
  std::string np = net_cell_name_prefix.to_string ();
  std::string dp = device_cell_name_prefix.to_string ();
  l2n->build_all_nets (cmap, target, lmap, net_cell_name_prefix.is_nil () ? 0 : np.c_str (), circuit_cell_name_prefix.is_nil () ? 0 : cp.c_str (), device_cell_name_prefix.is_nil () ? 0 : dp.c_str ());
}

static void write_l2n (const db::LayoutToNetlist *l2n, const std::string &path)
{
  tl::OutputStream stream (path);
  db::LayoutToNetlistStandardWriter writer (stream);
  writer.write (l2n);
}

Class<db::LayoutToNetlist> decl_dbLayoutToNetlist ("db", "LayoutToNetlist",
  gsi::constructor ("new", &make_l2n, gsi::arg ("iter"),
    "@brief The constructor\n"
    "See the class description for details.\n"
  ) +
  gsi::method ("threads=", &db::LayoutToNetlist::set_threads, gsi::arg ("n"),
    "@brief Sets the number of threads to use for operations which support multiple threads\n"
  ) +
  gsi::method ("threads", &db::LayoutToNetlist::threads,
    "@brief Gets the number of threads to use for operations which support multiple threads\n"
  ) +
  gsi::method ("area_ratio=", &db::LayoutToNetlist::set_area_ratio, gsi::arg ("r"),
    "@brief Sets the area_ratio parameter for the hierarchical network processor\n"
    "This parameter controls splitting of large polygons in order to reduce the\n"
    "error made by the bounding box approximation.\n"
  ) +
  gsi::method ("area_ratio", &db::LayoutToNetlist::area_ratio,
    "@brief Gets the area_ratio parameter for the hierarchical network processor\n"
    "See \\area_ratio= for details about this attribute."
  ) +
  gsi::method ("max_vertex_count=", &db::LayoutToNetlist::set_max_vertex_count, gsi::arg ("n"),
    "@brief Sets the max_vertex_count parameter for the hierarchical network processor\n"
    "This parameter controls splitting of large polygons in order to enhance performance\n"
    "for very big polygons.\n"
  ) +
  gsi::method ("max_vertex_count", &db::LayoutToNetlist::max_vertex_count,
    "See \\max_vertex_count= for details about this attribute."
  ) +
  gsi::method ("make_layer", &db::LayoutToNetlist::make_layer, gsi::arg ("layer_index"),
    "@brief Creates a new region representing an original layer\n"
    "'layer_index'' is the layer index of the desired layer in the original layout.\n"
    "The Region object returned is a new object and must be deleted by the caller.\n"
    "This variant produces polygons and takes texts for net name annotation.\n"
    "A variant not taking texts is \\make_polygon_layer. A Variant only taking\n"
    "texts is \\make_text_layer.\n"""
  ) +
  gsi::method ("make_text_layer", &db::LayoutToNetlist::make_text_layer, gsi::arg ("layer_index"),
    "@brief Creates a new region representing an original layer taking texts only\n"
    "See \\make_layer for details.\n"
  ) +
  gsi::method ("make_polygon_layer", &db::LayoutToNetlist::make_polygon_layer, gsi::arg ("layer_index"),
    "@brief Creates a new region representing an original layer taking polygons and texts\n"
    "See \\make_layer for details.\n"
  ) +
  gsi::method ("extract_devices", &db::LayoutToNetlist::extract_devices, gsi::arg ("extractor"), gsi::arg ("layers"),
    "@brief Extracts devices\n"
    "See the class description for more details.\n"
    "This method will run device extraction for the given extractor. The layer map is specific\n"
    "for the extractor and uses the region objects derived with \\make_layer and it's variants.\n"
    "\n"
    "In addition, derived regions can be passed too. Certain limitations apply. It's safe to use\n"
    "boolean operations for deriving layers. Other operations are applicable as long as they are\n"
    "capable of delivering hierarchical layers.\n"
    "\n"
    "If errors occur, the device extractor will contain theses errors.\n"
  ) +
  gsi::method ("connect", (void (db::LayoutToNetlist::*) (const db::Region &)) &db::LayoutToNetlist::connect, gsi::arg ("l"),
    "@brief Defines an intra-layer connection for the given layer.\n"
    "The layer is either an original layer created with \\make_layer and it's variants or\n"
    "a derived layer. Certain limitations apply. It's safe to use\n"
    "boolean operations for deriving layers. Other operations are applicable as long as they are\n"
    "capable of delivering hierarchical layers.\n"
  ) +
  gsi::method ("connect", (void (db::LayoutToNetlist::*) (const db::Region &, const db::Region &)) &db::LayoutToNetlist::connect, gsi::arg ("a"), gsi::arg ("b"),
    "@brief Defines an inter-layer connection for the given layers.\n"
    "The conditions mentioned with intra-layer \\connect apply for this method too.\n"
  ) +
  gsi::method ("connect_global", (void (db::LayoutToNetlist::*) (const db::Region &, const std::string &)) &db::LayoutToNetlist::connect_global, gsi::arg ("l"), gsi::arg ("global_net_name"),
    "@brief Defines a connection of the given layer with a global net.\n"
    "This method returns the ID of the global net. Use \\global_net_name to get "
    "the name back from the ID."
  ) +
  gsi::method ("global_net_name", &db::LayoutToNetlist::global_net_name, gsi::arg ("global_net_id"),
    "@brief Gets the global net name for the given global net ID."
  ) +
  gsi::method ("extract_netlist", &db::LayoutToNetlist::extract_netlist,
    "@brief Runs the netlist extraction\n"
    "See the class description for more details.\n"
  ) +
  gsi::method_ext ("internal_layout", &l2n_internal_layout,
    "@brief Gets the internal layout\n"
    "Usually it should not be required to obtain the internal layout. If you need to do so, make sure not to modify the layout as\n"
    "the functionality of the netlist extractor depends on it."
  ) +
  gsi::method_ext ("internal_top_cell", &l2n_internal_top_cell,
    "@brief Gets the internal top cell\n"
    "Usually it should not be required to obtain the internal cell. If you need to do so, make sure not to modify the cell as\n"
    "the functionality of the netlist extractor depends on it."
  ) +
  gsi::method ("layer_of", &db::LayoutToNetlist::layer_of, gsi::arg ("l"),
    "@brief Gets the internal layer for a given extraction layer\n"
    "This method is required to derive the internal layer index - for example for\n"
    "investigating the cluster tree.\n"
  ) +
  gsi::method ("cell_mapping_into", &db::LayoutToNetlist::cell_mapping_into, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("with_device_cells", false),
    "@brief Creates a cell mapping for copying shapes from the internal layout to the given target layout.\n"
    "If 'with_device_cells' is true, cells will be produced for devices. These are cells not corresponding to circuits, so they are disabled normally.\n"
    "Use this option, if you want to access device terminal shapes per device.\n"
    "CAUTION: this function may create new cells in 'layout'.\n"
  ) +
  gsi::method ("const_cell_mapping_into", &db::LayoutToNetlist::const_cell_mapping_into, gsi::arg ("layout"), gsi::arg ("cell"),
    "@brief Creates a cell mapping for copying shapes from the internal layout to the given target layout.\n"
    "This version will not create new cells in the target layout.\n"
    "If the required cells do not exist there yet, flatting will happen.\n"
  ) +
  gsi::method ("netlist", &db::LayoutToNetlist::netlist,
    "@brief gets the netlist extracted (0 if no extraction happened yet)\n"
  ) +
  gsi::factory ("shapes_of_net", (db::Region *(db::LayoutToNetlist::*) (const db::Net &, const db::Region &, bool) const) &db::LayoutToNetlist::shapes_of_net, gsi::arg ("net"), gsi::arg ("of_layer"), gsi::arg ("recursive"),
    "@brief Returns all shapes of a specific net and layer.\n"
    "If 'recursive'' is true, the returned region will contain the shapes of\n"
    "all subcircuits too.\n"
  ) +
  gsi::method ("shapes_of_net", (void (db::LayoutToNetlist::*) (const db::Net &, const db::Region &, bool, db::Shapes &) const) &db::LayoutToNetlist::shapes_of_net, gsi::arg ("net"), gsi::arg ("of_layer"), gsi::arg ("recursive"), gsi::arg ("to"),
    "@brief Sends all shapes of a specific net and layer to the given Shapes container.\n"
    "If 'recursive'' is true, the returned region will contain the shapes of\n"
    "all subcircuits too.\n"
  ) +
  gsi::method_ext ("build_net", &build_net, gsi::arg ("net"), gsi::arg ("target"), gsi::arg ("target_cell"), gsi::arg ("lmap"), gsi::arg ("circuit_cell_name_prefix", tl::Variant (), "nil"), gsi::arg ("device_cell_name_prefix", tl::Variant (), "nil"),
    "@brief Builds a net representation in the given layout and cell\n"
    "\n"
    "This method has two modes: recursive and top-level mode. In recursive mode,\n"
    "it will create a proper hierarchy below the given target cell to hold all subcircuits the\n"
    "net connects to. It will copy the net's parts from this subcircuits into these cells.\n"
    "\n"
    "In top-level mode, only the shapes from the net inside it's circuit are copied to\n"
    "the given target cell. No other cells are created.\n"
    "\n"
    "Recursive mode is picked when a circuit cell name prefix is given. The new cells will be\n"
    "named like circuit_cell_name_prefix + circuit name.\n"
    "\n"
    "If a device cell name prefix is given, device shapes will be output on device cells named\n"
    "like device_cell_name_prefix + device name.\n"
    "\n"
    "@param target The target layout\n"
    "@param target_cell The target cell\n"
    "@param lmap Target layer indexes (keys) and net regions (values)\n"
    "@param circuit_cell_name_prefix Chooses recursive mode if non-nil\n"
    "@param device_cell_name_prefix If given, devices will be output as separate cells\n"
  ) +
  gsi::method_ext ("build_all_nets", &build_all_nets, gsi::arg ("cmap"), gsi::arg ("target"), gsi::arg ("lmap"), gsi::arg ("net_cell_name_prefix", tl::Variant (), "nil"), gsi::arg ("circuit_cell_name_prefix", tl::Variant (), "nil"), gsi::arg ("device_cell_name_prefix", tl::Variant (), "nil"),
    "@brief Builds a full hierarchical representation of the nets\n"
    "\n"
    "This method copies all nets into cells corresponding to the circuits. It uses the cmap\n"
    "object to determine the target cell (create them with \\cell_mapping_into or \\const_cell_mapping_into.\n"
    "If no mapping is requested, the specific circuit it skipped.\n"
    "\n"
    "The method has two net annotation modes:\n"
    "\n"
    "@ul\n"
    "@li 'No annotation'' (net_cell_name_prefix == 0): the shapes will be put into the target cell simply @/li\n"
    "@li Individual subcells per net (net_cell_name_prefix != 0): for each net, a subcell is created\n"
    "    and the net shapes will be put there (name of the subcell = net_cell_name_prefix + net name). @/li\n"
    "@/ul\n"
    "\n"
    "In addition, net hierarchy is covered in two ways:\n"
    "\n"
    "@ul\n"
    "@li No connection indicated (circuit_cell_name_prefix == 0: the net shapes are simply put into their\n"
    "   respective circuits. The connections are not indicated. @/li\n"
    "@li Subnet hierarchy (circuit_cell_name_prefix != 0): for each root net, a full hierarchy is built\n"
    "   to accomodate the subnets (see build_net in recursive mode). @/li\n"
    "@/ul\n"
    "\n"
    "If a device name prefix is given, device shapes will be output on device cells named\n"
    "like device_name_prefix + device name.\n"
    "\n"
    "@param cmap The mapping of internal layout to target layout for the circuit mapping\n"
    "@param target The target layout\n"
    "@param lmap Target layer indexes (keys) and net regions (values)\n"
    "@param circuit_cell_name_prefix See method description\n"
    "@param net_cell_name_prefix See method description\n"
    "@param device_cell_name_prefix If given, devices will be output as separate cells\n"
  ) +
  gsi::method ("probe_net", (db::Net *(db::LayoutToNetlist::*) (const db::Region &, const db::DPoint &)) &db::LayoutToNetlist::probe_net, gsi::arg ("of_layer"), gsi::arg ("point"),
    "@brief Finds the net by probing a specific location on the given layer\n"
    "\n"
    "This method will find a net looking at the given layer at the specific position.\n"
    "It will traverse the hierarchy below if no shape in the requested layer is found\n"
    "in the specified location. The function will report the topmost net from far above the\n"
    "hierarchy of circuits as possible.\n"
    "\n"
    "If no net is found at all, 0 is returned.\n"
    "\n"
    "It is recommended to use \\probe on the netlist right after extraction.\n"
    "Optimization functions such as \\Netlist#purge will remove parts of the net which means\n"
    "shape to net probing may no longer work for these nets.\n"
    "\n"
    "This variant accepts a micrometer-unit location. The location is given in the\n"
    "coordinate space of the initial cell.\n"
  ) +
  gsi::method ("probe_net", (db::Net *(db::LayoutToNetlist::*) (const db::Region &, const db::Point &)) &db::LayoutToNetlist::probe_net, gsi::arg ("of_layer"), gsi::arg ("point"),
    "@brief Finds the net by probing a specific location on the given layer\n"
    "See the description of the other \\probe_net variant.\n"
    "This variant accepts a database-unit location. The location is given in the\n"
    "coordinate space of the initial cell.\n"
  ) +
  gsi::method_ext ("write", &write_l2n, gsi::arg ("path"),
    "@brief Writes the extracted netlist to a file.\n"
    "This method employs the native format of KLayout.\n"
  ),
  "@brief A generic framework for extracting netlists from layouts\n"
  "\n"
  "This class wraps various concepts from db::NetlistExtractor and db::NetlistDeviceExtractor\n"
  "and more. It is supposed to provide a framework for extracting a netlist from a layout.\n"
  "\n"
  "The use model of this class consists of five steps which need to be executed in this order.\n"
  "\n"
  "@ul\n"
  "@li Configuration: in this step, the LayoutToNetlist object is created and\n"
  "    if required, configured. Methods to be used in this step are \\threads=,\n"
  "    \\area_ratio= or \\max_vertex_count=. The constructor for the LayoutToNetlist\n"
  "    object receives a \\RecursiveShapeIterator object which basically supplies the\n"
  "    hierarchy and the layout taken as input.\n"
  "@/li\n"
  "@li Preparation\n"
  "    In this step, the device recognitions and extraction layers are drawn from\n"
  "    the framework. Derived can now be computed using boolean operations.\n"
  "    Methods to use in this step are \\make_layer and it's variants.\n"
  "    Layer preparation is not necessarily required to happen before all\n"
  "    other steps. Layers can be computed shortly before they are required.\n"
  "@/li\n"
  "@li Following the preparation, the devices can be extracted using \\extract_devices.\n"
  "    This method needs to be called for each device extractor required. Each time,\n"
  "    a device extractor needs to be given plus a map of device layers. The device\n"
  "    layers are device extractor specific. Either original or derived layers\n"
  "    may be specified here. Layer preparation may happen between calls to \\extract_devices.\n"
  "@/li\n"
  "@li Once the devices are derived, the netlist connectivity can be defined and the\n"
  "    netlist extracted. The connectivity is defined with \\connect and it's\n"
  "    flavours. The actual netlist extraction happens with \\extract_netlist.\n"
  "@/li\n"
  "@li After netlist extraction, the information is ready to be retrieved.\n"
  "    The produced netlist is available with \\netlist. The Shapes of a\n"
  "    specific net are available with \\shapes_of_net. \\probe_net allows\n"
  "    finding a net by probing a specific location.\n"
  "@li\n"
  "\n"
  "This class has been introduced in version 0.26."
);

}

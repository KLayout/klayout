
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


#include "gsiDecl.h"

#include "dbLEFImporter.h"
#include "dbDEFImporter.h"
#include "dbLEFDEFImporter.h"

namespace gsi
{

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static db::LEFDEFReaderOptions &get_lefdef_config (db::LoadLayoutOptions *options)
{
  return options->get_options<db::LEFDEFReaderOptions> ();
}

static void set_lefdef_config (db::LoadLayoutOptions *options, const db::LEFDEFReaderOptions &config)
{
  options->set_options (config);
}

//  extend lay::LoadLayoutOptions with the GDS2 options
static
gsi::ClassExt<db::LoadLayoutOptions> decl_ext_lefdef_reader_options (
  gsi::method_ext ("lefdef_config", &get_lefdef_config,
    "@brief Gets a copy of the LEF/DEF reader configuration\n"
    "The LEF/DEF reader configuration is wrapped in a separate object of class \\LEFDEFReaderConfiguration. See there for details.\n"
    "This method will return a copy of the reader configuration. To modify the configuration, modify the copy and set the modified "
    "configuration with \\lefdef_config=.\n"
    "\n"
    "\nThis method has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("lefdef_config=", &set_lefdef_config, gsi::arg ("config"),
    "@brief Sets the LEF/DEF reader configuration\n"
    "\n"
    "\nThis method has been added in version 0.25.\n"
  )
);

static tl::Variant get_net_property_name (const db::LEFDEFReaderOptions *config)
{
  if (config->produce_net_names ()) {
    return config->net_property_name ();
  } else {
    return tl::Variant ();
  }
}

static void set_net_property_name (db::LEFDEFReaderOptions *config, const tl::Variant &name)
{
  config->set_produce_net_names (! name.is_nil ());
  config->set_net_property_name (name);
}

static tl::Variant get_instance_property_name (const db::LEFDEFReaderOptions *config)
{
  if (config->produce_inst_names ()) {
    return config->inst_property_name ();
  } else {
    return tl::Variant ();
  }
}

static void set_instance_property_name (db::LEFDEFReaderOptions *config, const tl::Variant &name)
{
  config->set_produce_inst_names (! name.is_nil ());
  config->set_inst_property_name (name);
}

static tl::Variant get_pin_property_name (const db::LEFDEFReaderOptions *config)
{
  if (config->produce_pin_names ()) {
    return config->pin_property_name ();
  } else {
    return tl::Variant ();
  }
}

static void set_pin_property_name (db::LEFDEFReaderOptions *config, const tl::Variant &name)
{
  config->set_produce_pin_names (! name.is_nil ());
  config->set_pin_property_name (name);
}

static
gsi::Class<db::LEFDEFReaderOptions> decl_lefdef_config ("db", "LEFDEFReaderConfiguration",
  gsi::method ("paths_relative_to_cwd=", &db::LEFDEFReaderOptions::set_paths_relative_to_cwd, gsi::arg ("f"),
    "@brief Sets a value indicating whether to use paths relative to cwd (true) or DEF file (false) for map or LEF files\n"
    "This write-only attribute has been introduced in version 0.27.9."
  ) +
  gsi::method ("layer_map", (db::LayerMap &(db::LEFDEFReaderOptions::*) ()) &db::LEFDEFReaderOptions::layer_map,
    "@brief Gets the layer map to be used for the LEF/DEF reader\n"
    "@return A reference to the layer map\n"
    "Because LEF/DEF layer mapping is substantially different than for normal layout files, the LEF/DEF reader "
    "employs a separate layer mapping table. The LEF/DEF specific layer mapping is stored within the "
    "LEF/DEF reader's configuration and can be accessed with this attribute. The layer mapping table of "
    "\\LoadLayoutOptions will be ignored for the LEF/DEF reader.\n"
    "\n"
    "The setter is \\layer_map=. \\create_other_layers= is available to control whether layers "
    "not specified in the layer mapping table shall be created automatically."
  ) +
  gsi::method ("layer_map=", &db::LEFDEFReaderOptions::set_layer_map, gsi::arg ("m"),
    "@brief Sets the layer map to be used for the LEF/DEF reader\n"
    "See \\layer_map for details."
  ) +
  gsi::method ("create_other_layers", &db::LEFDEFReaderOptions::read_all_layers,
    "@brief Gets a value indicating whether layers not mapped in the layer map shall be created too\n"
    "See \\layer_map for details."
  ) +
  gsi::method ("create_other_layers=", &db::LEFDEFReaderOptions::set_read_all_layers, gsi::arg ("f"),
    "@brief Sets a value indicating whether layers not mapped in the layer map shall be created too\n"
    "See \\layer_map for details."
  ) +
  gsi::method ("dbu", &db::LEFDEFReaderOptions::dbu,
    "@brief Gets the database unit to use for producing the layout.\n"
    "This value specifies the database to be used for the layout that is read. When a DEF file is specified with "
    "a different database unit, the layout is translated into this database unit.\n"
  ) +
  gsi::method ("dbu=", &db::LEFDEFReaderOptions::set_dbu, gsi::arg ("dbu"),
    "@brief Sets the database unit to use for producing the layout.\n"
    "See \\dbu for details."
  ) +
  gsi::method_ext ("net_property_name", &get_net_property_name,
    "@brief Gets a value indicating whether and how to produce net names as properties.\n"
    "If set to a value not nil, net names will be attached to the net shapes generated as user properties.\n"
    "This attribute then specifies the user property name to be used for attaching the net names.\n"
    "If set to nil, no net names will be produced.\n"
    "\n"
    "The corresponding setter is \\net_property_name=."
  ) +
  gsi::method_ext ("net_property_name=", &set_net_property_name, gsi::arg ("name"),
    "@brief Sets a value indicating whether and how to produce net names as properties.\n"
    "See \\net_property_name for details."
  ) +
  gsi::method_ext ("pin_property_name", &get_pin_property_name,
    "@brief Gets a value indicating whether and how to produce pin names as properties.\n"
    "If set to a value not nil, pin names will be attached to the pin shapes generated as user properties.\n"
    "This attribute then specifies the user property name to be used for attaching the pin names.\n"
    "If set to nil, no pin names will be produced.\n"
    "\n"
    "The corresponding setter is \\pin_property_name=.\n"
    "\n"
    "This method has been introduced in version 0.26.4."
  ) +
  gsi::method_ext ("pin_property_name=", &set_pin_property_name, gsi::arg ("name"),
    "@brief Sets a value indicating whether and how to produce pin names as properties.\n"
    "See \\pin_property_name for details.\n"
    "\n"
    "This method has been introduced in version 0.26.4."
  ) +
  gsi::method_ext ("instance_property_name", &get_instance_property_name,
    "@brief Gets a value indicating whether and how to produce instance names as properties.\n"
    "If set to a value not nil, instance names will be attached to the instances generated as user properties.\n"
    "This attribute then specifies the user property name to be used for attaching the instance names.\n"
    "If set to nil, no instance names will be produced.\n"
    "\n"
    "The corresponding setter is \\instance_property_name=.\n"
    "\n"
    "This method has been introduced in version 0.26.4."
  ) +
  gsi::method_ext ("instance_property_name=", &set_instance_property_name, gsi::arg ("name"),
    "@brief Sets a value indicating whether and how to produce instance names as properties.\n"
    "See \\instance_property_name for details.\n"
    "\n"
    "This method has been introduced in version 0.26.4."
  ) +
  gsi::method ("produce_cell_outlines", &db::LEFDEFReaderOptions::produce_cell_outlines,
    "@brief Gets a value indicating whether to produce cell outlines (diearea).\n"
    "If set to true, cell outlines will be produced on the layer given by \\cell_outline_layer. "
  ) +
  gsi::method ("produce_cell_outlines=", &db::LEFDEFReaderOptions::set_produce_cell_outlines, gsi::arg ("produce"),
    "@brief Sets a value indicating whether to produce cell outlines (diearea).\n"
    "See \\produce_cell_outlines for details.\n"
  ) +
  gsi::method ("cell_outline_layer", &db::LEFDEFReaderOptions::cell_outline_layer,
    "@brief Gets the layer on which to produce the cell outline (diearea).\n"
    "This attribute is a string corresponding to the string representation of \\LayerInfo. "
    "This string can be either a layer number, a layer/datatype pair, a name or a combination of both. See \\LayerInfo for details.\n"
    "The setter for this attribute is \\cell_outline_layer=. See also \\produce_cell_outlines."
  ) +
  gsi::method ("cell_outline_layer=", &db::LEFDEFReaderOptions::set_cell_outline_layer, gsi::arg ("spec"),
    "@brief Sets the layer on which to produce the cell outline (diearea).\n"
    "See \\cell_outline_layer for details.\n"
  ) +
  gsi::method ("produce_placement_blockages", &db::LEFDEFReaderOptions::produce_placement_blockages,
    "@brief Gets a value indicating whether to produce placement blockage regions.\n"
    "If set to true, polygons will be produced representing the placement blockage region on the layer given by \\placement_blockage_layer. "
  ) +
  gsi::method ("produce_placement_blockages=", &db::LEFDEFReaderOptions::set_produce_placement_blockages, gsi::arg ("produce"),
    "@brief Sets a value indicating whether to produce placement blockage regions.\n"
    "See \\produce_placement_blockages for details.\n"
  ) +
  gsi::method ("placement_blockage_layer", &db::LEFDEFReaderOptions::placement_blockage_layer,
    "@brief Gets the layer on which to produce the placement blockage.\n"
    "This attribute is a string corresponding to the string representation of \\LayerInfo. "
    "This string can be either a layer number, a layer/datatype pair, a name or a combination of both. See \\LayerInfo for details."
    "The setter for this attribute is \\placement_blockage_layer=. See also \\produce_placement_blockages."
  ) +
  gsi::method ("placement_blockage_layer=", &db::LEFDEFReaderOptions::set_placement_blockage_layer, gsi::arg ("layer"),
    "@brief Sets the layer on which to produce the placement blockage.\n"
    "See \\placement_blockage_layer for details.\n"
  ) +
  gsi::method ("produce_regions", &db::LEFDEFReaderOptions::produce_regions,
    "@brief Gets a value indicating whether to produce regions.\n"
    "If set to true, polygons will be produced representing the regions on the layer given by \\region_layer.\n"
    "\n"
    "The attribute has been introduced in version 0.27."
  ) +
  gsi::method ("produce_regions=", &db::LEFDEFReaderOptions::set_produce_regions, gsi::arg ("produce"),
    "@brief Sets a value indicating whether to produce regions.\n"
    "See \\produce_regions for details.\n"
    "\n"
    "The attribute has been introduced in version 0.27."
  ) +
  gsi::method ("region_layer", &db::LEFDEFReaderOptions::region_layer,
    "@brief Gets the layer on which to produce the regions.\n"
    "This attribute is a string corresponding to the string representation of \\LayerInfo. "
    "This string can be either a layer number, a layer/datatype pair, a name or a combination of both. See \\LayerInfo for details."
    "The setter for this attribute is \\region_layer=. See also \\produce_regions.\n"
    "\n"
    "The attribute has been introduced in version 0.27."
  ) +
  gsi::method ("region_layer=", &db::LEFDEFReaderOptions::set_region_layer, gsi::arg ("layer"),
    "@brief Sets the layer on which to produce the regions.\n"
    "See \\region_layer for details.\n"
    "\n"
    "The attribute has been introduced in version 0.27."
  ) +
  gsi::method ("produce_via_geometry", &db::LEFDEFReaderOptions::produce_via_geometry,
    "@brief Sets a value indicating whether via geometries shall be produced.\n"
    "\n"
    "If set to true, shapes will be produced for each via. The layer to be produced will be determined from the "
    "via layer's name using the suffix provided by \\via_geometry_suffix. If there is a specific mapping in the "
    "layer mapping table for the via layer including the suffix, the layer/datatype will be taken from the layer "
    "mapping table. If there is a mapping to the undecorated via layer, the datatype will be substituted with "
    "the \\via_geometry_datatype value. If no mapping is defined, a unique number will be assigned to the layer "
    "number and the datatype will be taken from the \\via_geometry_datatype value.\n"
    "\n"
    "For example: the via layer is 'V1', \\via_geometry_suffix is 'GEO' and \\via_geometry_datatype is 1. Then:\n"
    "\n"
    "@ul\n"
    "@li If there is a mapping for 'V1.GEO', the layer and datatype will be taken from there. @/li\n"
    "@li If there is a mapping for 'V1', the layer will be taken from there and the datatype will be taken from \\via_geometry_datatype. "
    "    The name of the produced layer will be 'V1.GEO'. @/li\n"
    "@li If there is no mapping for both, the layer number will be a unique value, the datatype will be taken from \\via_geometry_datatype "
    "    and the layer name will be 'V1.GEO'. @/li"
    "@/ul\n"
  ) +
  gsi::method ("produce_via_geometry=", &db::LEFDEFReaderOptions::set_produce_via_geometry, gsi::arg ("produce"),
    "@brief Sets a value indicating whether via geometries shall be produced.\n"
    "See \\produce_via_geometry for details.\n"
  ) +
  gsi::method ("via_geometry_suffix", &db::LEFDEFReaderOptions::via_geometry_suffix,
    "@brief Gets the via geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about this property.\n"
  ) +
  gsi::method ("via_geometry_suffix=", &db::LEFDEFReaderOptions::set_via_geometry_suffix, gsi::arg ("suffix"),
    "@brief Sets the via geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about this property.\n"
  ) +
  gsi::method ("via_geometry_datatype", &db::LEFDEFReaderOptions::via_geometry_datatype,
    "@brief Gets the via geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about this property.\n"
  ) +
  gsi::method ("via_geometry_datatype=", &db::LEFDEFReaderOptions::set_via_geometry_datatype, gsi::arg ("datatype"),
    "@brief Sets the via geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about this property.\n"
  ) +
  gsi::method ("clear_via_geometry_suffixes_per_mask", &db::LEFDEFReaderOptions::clear_via_geometry_suffixes_per_mask,
    "@brief Clears the via geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("clear_via_geometry_datatypes_per_mask", &db::LEFDEFReaderOptions::clear_via_geometry_datatypes_per_mask,
    "@brief Clears the via geometry layer datatypes per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("via_geometry_suffix_per_mask", &db::LEFDEFReaderOptions::via_geometry_suffix_per_mask, gsi::arg ("mask"),
    "@brief Gets the via geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_via_geometry_suffix_per_mask", &db::LEFDEFReaderOptions::set_via_geometry_suffix_per_mask, gsi::arg ("mask"), gsi::arg ("suffix"),
    "@brief Sets the via geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("via_geometry_datatype", &db::LEFDEFReaderOptions::via_geometry_datatype_per_mask, gsi::arg ("mask"),
    "@brief Gets the via geometry layer datatype value per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_via_geometry_datatype_per_mask", &db::LEFDEFReaderOptions::set_via_geometry_datatype_per_mask, gsi::arg ("mask"), gsi::arg ("datatype"),
    "@brief Sets the via geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("via_geometry_suffix_str", &db::LEFDEFReaderOptions::via_geometry_suffix_str,
    "@hide"
  ) +
  gsi::method ("via_geometry_suffix_str=", &db::LEFDEFReaderOptions::set_via_geometry_suffix_str, gsi::arg ("suffix"),
    "@hide"
  ) +
  gsi::method ("via_geometry_datatype_str", &db::LEFDEFReaderOptions::via_geometry_datatype_str,
    "@hide"
  ) +
  gsi::method ("via_geometry_datatype_str=", &db::LEFDEFReaderOptions::set_via_geometry_datatype_str, gsi::arg ("datatype"),
    "@hide"
  ) +
  gsi::method ("via_cellname_prefix", &db::LEFDEFReaderOptions::via_cellname_prefix,
    "@brief Gets the via cellname prefix.\n"
    "Vias are represented by cells. The cell name is formed by combining the via cell name prefix and the via name.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("via_cellname_prefix=", &db::LEFDEFReaderOptions::set_via_cellname_prefix, gsi::arg ("prefix"),
    "@brief Sets the via cellname prefix.\n"
    "See \\via_cellname_prefix for details about this property.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("produce_pins", &db::LEFDEFReaderOptions::produce_pins,
    "@brief Gets a value indicating whether pin geometries shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_pins=", &db::LEFDEFReaderOptions::set_produce_pins, gsi::arg ("produce"),
    "@brief Sets a value indicating whether pin geometries shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("pins_suffix", &db::LEFDEFReaderOptions::pins_suffix,
    "@brief Gets the pin geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("pins_suffix=", &db::LEFDEFReaderOptions::set_pins_suffix, gsi::arg ("suffix"),
    "@brief Sets the pin geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("pins_datatype", &db::LEFDEFReaderOptions::pins_datatype,
    "@brief Gets the pin geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("pins_datatype=", &db::LEFDEFReaderOptions::set_pins_datatype, gsi::arg ("datatype"),
    "@brief Sets the pin geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("clear_pins_suffixes_per_mask", &db::LEFDEFReaderOptions::clear_pins_suffixes_per_mask,
    "@brief Clears the pin layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("clear_pin_datatypes_per_mask", &db::LEFDEFReaderOptions::clear_pins_datatypes_per_mask,
    "@brief Clears the pin layer datatypes per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("pins_suffix_per_mask", &db::LEFDEFReaderOptions::pins_suffix_per_mask, gsi::arg ("mask"),
    "@brief Gets the pin geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_pins_suffix_per_mask", &db::LEFDEFReaderOptions::set_pins_suffix_per_mask, gsi::arg ("mask"), gsi::arg ("suffix"),
    "@brief Sets the pin geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("pins_datatype", &db::LEFDEFReaderOptions::pins_datatype_per_mask, gsi::arg ("mask"),
    "@brief Gets the pin geometry layer datatype value per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_pins_datatype_per_mask", &db::LEFDEFReaderOptions::set_pins_datatype_per_mask, gsi::arg ("mask"), gsi::arg ("datatype"),
    "@brief Sets the pin geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("pins_suffix_str", &db::LEFDEFReaderOptions::pins_suffix_str,
    "@hide"
  ) +
  gsi::method ("pins_suffix_str=", &db::LEFDEFReaderOptions::set_pins_suffix_str, gsi::arg ("suffix"),
    "@hide"
  ) +
  gsi::method ("pins_datatype_str", &db::LEFDEFReaderOptions::pins_datatype_str,
    "@hide"
  ) +
  gsi::method ("pins_datatype_str=", &db::LEFDEFReaderOptions::set_pins_datatype_str, gsi::arg ("datatype"),
    "@hide"
  ) +
  gsi::method ("produce_lef_pins", &db::LEFDEFReaderOptions::produce_lef_pins,
    "@brief Gets a value indicating whether LEF pin geometries shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_lef_pins=", &db::LEFDEFReaderOptions::set_produce_lef_pins, gsi::arg ("produce"),
    "@brief Sets a value indicating whether LEF pin geometries shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("lef_pins_suffix", &db::LEFDEFReaderOptions::lef_pins_suffix,
    "@brief Gets the LEF pin geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("lef_pins_suffix=", &db::LEFDEFReaderOptions::set_lef_pins_suffix, gsi::arg ("suffix"),
    "@brief Sets the LEF pin geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("lef_pins_datatype", &db::LEFDEFReaderOptions::lef_pins_datatype,
    "@brief Gets the LEF pin geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("lef_pins_datatype=", &db::LEFDEFReaderOptions::set_lef_pins_datatype, gsi::arg ("datatype"),
    "@brief Sets the LEF pin geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("clear_lef_pins_suffixes_per_mask", &db::LEFDEFReaderOptions::clear_lef_pins_suffixes_per_mask,
    "@brief Clears the LEF pin layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("clear_lef_pins_datatypes_per_mask", &db::LEFDEFReaderOptions::clear_lef_pins_datatypes_per_mask,
    "@brief Clears the LEF pin layer datatypes per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("lef_pins_suffix_per_mask", &db::LEFDEFReaderOptions::lef_pins_suffix_per_mask, gsi::arg ("mask"),
    "@brief Gets the LEF pin geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_lef_pins_suffix_per_mask", &db::LEFDEFReaderOptions::set_lef_pins_suffix_per_mask, gsi::arg ("mask"), gsi::arg ("suffix"),
    "@brief Sets the LEF pin geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("lef_pins_datatype", &db::LEFDEFReaderOptions::lef_pins_datatype_per_mask, gsi::arg ("mask"),
    "@brief Gets the LEF pin geometry layer datatype value per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_lef_pins_datatype_per_mask", &db::LEFDEFReaderOptions::set_lef_pins_datatype_per_mask, gsi::arg ("mask"), gsi::arg ("datatype"),
    "@brief Sets the LEF pin geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("lef_pins_suffix_str", &db::LEFDEFReaderOptions::lef_pins_suffix_str,
    "@hide"
  ) +
  gsi::method ("lef_pins_suffix_str=", &db::LEFDEFReaderOptions::set_lef_pins_suffix_str, gsi::arg ("suffix"),
    "@hide"
  ) +
  gsi::method ("lef_pins_datatype_str", &db::LEFDEFReaderOptions::lef_pins_datatype_str,
    "@hide"
  ) +
  gsi::method ("lef_pins_datatype_str=", &db::LEFDEFReaderOptions::set_lef_pins_datatype_str, gsi::arg ("datatype"),
    "@hide"
  ) +
  gsi::method ("produce_fills", &db::LEFDEFReaderOptions::produce_fills,
    "@brief Gets a value indicating whether fill geometries shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n\n"
    "Fill support has been introduced in version 0.27."
  ) +
  gsi::method ("produce_fills=", &db::LEFDEFReaderOptions::set_produce_fills, gsi::arg ("produce"),
    "@brief Sets a value indicating whether fill geometries shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n\n"
    "Fill support has been introduced in version 0.27."
  ) +
  gsi::method ("fills_suffix", &db::LEFDEFReaderOptions::fills_suffix,
    "@brief Gets the fill geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n\n"
    "Fill support has been introduced in version 0.27."
  ) +
  gsi::method ("fills_suffix=", &db::LEFDEFReaderOptions::set_fills_suffix, gsi::arg ("suffix"),
    "@brief Sets the fill geometry layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n\n"
    "Fill support has been introduced in version 0.27."
  ) +
  gsi::method ("fills_datatype", &db::LEFDEFReaderOptions::fills_datatype,
    "@brief Gets the fill geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n\n"
    "Fill support has been introduced in version 0.27."
  ) +
  gsi::method ("fills_datatype=", &db::LEFDEFReaderOptions::set_fills_datatype, gsi::arg ("datatype"),
    "@brief Sets the fill geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n\n"
    "Fill support has been introduced in version 0.27."
  ) +
  gsi::method ("clear_fills_suffixes_per_mask", &db::LEFDEFReaderOptions::clear_fills_suffixes_per_mask,
    "@brief Clears the fill layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("clear_fill_datatypes_per_mask", &db::LEFDEFReaderOptions::clear_fills_datatypes_per_mask,
    "@brief Clears the fill layer datatypes per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("fills_suffix_per_mask", &db::LEFDEFReaderOptions::fills_suffix_per_mask, gsi::arg ("mask"),
    "@brief Gets the fill geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_fills_suffix_per_mask", &db::LEFDEFReaderOptions::set_fills_suffix_per_mask, gsi::arg ("mask"), gsi::arg ("suffix"),
    "@brief Sets the fill geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("fills_datatype", &db::LEFDEFReaderOptions::fills_datatype_per_mask, gsi::arg ("mask"),
    "@brief Gets the fill geometry layer datatype value per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_fills_datatype_per_mask", &db::LEFDEFReaderOptions::set_fills_datatype_per_mask, gsi::arg ("mask"), gsi::arg ("datatype"),
    "@brief Sets the fill geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("fills_suffix_str", &db::LEFDEFReaderOptions::fills_suffix_str,
    "@hide"
  ) +
  gsi::method ("fills_suffix_str=", &db::LEFDEFReaderOptions::set_fills_suffix_str, gsi::arg ("suffix"),
    "@hide"
  ) +
  gsi::method ("fills_datatype_str", &db::LEFDEFReaderOptions::fills_datatype_str,
    "@hide"
  ) +
  gsi::method ("fills_datatype_str=", &db::LEFDEFReaderOptions::set_fills_datatype_str, gsi::arg ("datatype"),
    "@hide"
  ) +
  gsi::method ("produce_obstructions", &db::LEFDEFReaderOptions::produce_obstructions,
    "@brief Gets a value indicating whether obstruction markers shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_obstructions=", &db::LEFDEFReaderOptions::set_produce_obstructions, gsi::arg ("produce"),
    "@brief Sets a value indicating whether obstruction markers shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("obstructions_suffix", &db::LEFDEFReaderOptions::obstructions_suffix,
    "@brief Gets the obstruction marker layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("obstructions_suffix=", &db::LEFDEFReaderOptions::set_obstructions_suffix, gsi::arg ("suffix"),
    "@brief Sets the obstruction marker layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("obstructions_datatype", &db::LEFDEFReaderOptions::obstructions_datatype,
    "@brief Gets the obstruction marker layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("obstructions_datatype=", &db::LEFDEFReaderOptions::set_obstructions_datatype, gsi::arg ("datatype"),
    "@brief Sets the obstruction marker layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_blockages", &db::LEFDEFReaderOptions::produce_blockages,
    "@brief Gets a value indicating whether routing blockage markers shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_blockages=", &db::LEFDEFReaderOptions::set_produce_blockages, gsi::arg ("produce"),
    "@brief Sets a value indicating whether routing blockage markers shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("blockages_suffix", &db::LEFDEFReaderOptions::blockages_suffix,
    "@brief Gets the blockage marker layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("blockages_suffix=", &db::LEFDEFReaderOptions::set_blockages_suffix, gsi::arg ("suffix"),
    "@brief Sets the blockage marker layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("blockages_datatype", &db::LEFDEFReaderOptions::blockages_datatype,
    "@brief Gets the blockage marker layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("blockages_datatype=", &db::LEFDEFReaderOptions::set_blockages_datatype, gsi::arg ("datatype"),
    "@brief Sets the blockage marker layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_labels", &db::LEFDEFReaderOptions::produce_labels,
    "@brief Gets a value indicating whether labels shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_labels=", &db::LEFDEFReaderOptions::set_produce_labels, gsi::arg ("produce"),
    "@brief Sets a value indicating whether labels shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("labels_suffix", &db::LEFDEFReaderOptions::labels_suffix,
    "@brief Gets the label layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("labels_suffix=", &db::LEFDEFReaderOptions::set_labels_suffix, gsi::arg ("suffix"),
    "@brief Sets the label layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("labels_datatype", &db::LEFDEFReaderOptions::labels_datatype,
    "@brief Gets the labels layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("labels_datatype=", &db::LEFDEFReaderOptions::set_labels_datatype, gsi::arg ("datatype"),
    "@brief Sets the labels layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_lef_labels", &db::LEFDEFReaderOptions::produce_lef_labels,
    "@brief Gets a value indicating whether lef_labels shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules.\n"
    "\n"
    "This method has been introduced in version 0.27.2\n"
  ) +
  gsi::method ("produce_lef_labels=", &db::LEFDEFReaderOptions::set_produce_lef_labels, gsi::arg ("produce"),
    "@brief Sets a value indicating whether lef_labels shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules.\n"
    "\n"
    "This method has been introduced in version 0.27.2\n"
  ) +
  gsi::method ("lef_labels_suffix", &db::LEFDEFReaderOptions::lef_labels_suffix,
    "@brief Gets the label layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules.\n"
    "\n"
    "This method has been introduced in version 0.27.2\n"
  ) +
  gsi::method ("lef_labels_suffix=", &db::LEFDEFReaderOptions::set_lef_labels_suffix, gsi::arg ("suffix"),
    "@brief Sets the label layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules.\n"
    "\n"
    "This method has been introduced in version 0.27.2\n"
  ) +
  gsi::method ("lef_labels_datatype", &db::LEFDEFReaderOptions::lef_labels_datatype,
    "@brief Gets the lef_labels layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules.\n"
    "\n"
    "This method has been introduced in version 0.27.2\n"
  ) +
  gsi::method ("lef_labels_datatype=", &db::LEFDEFReaderOptions::set_lef_labels_datatype, gsi::arg ("datatype"),
    "@brief Sets the lef_labels layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules.\n"
    "\n"
    "This method has been introduced in version 0.27.2\n"
  ) +
  gsi::method ("produce_routing", &db::LEFDEFReaderOptions::produce_routing,
    "@brief Gets a value indicating whether routing geometry shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("produce_routing=", &db::LEFDEFReaderOptions::set_produce_routing, gsi::arg ("produce"),
    "@brief Sets a value indicating whether routing geometry shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("routing_suffix", &db::LEFDEFReaderOptions::routing_suffix,
    "@brief Gets the routing layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("routing_suffix=", &db::LEFDEFReaderOptions::set_routing_suffix, gsi::arg ("suffix"),
    "@brief Sets the routing layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("routing_datatype", &db::LEFDEFReaderOptions::routing_datatype,
    "@brief Gets the routing layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("routing_datatype=", &db::LEFDEFReaderOptions::set_routing_datatype, gsi::arg ("datatype"),
    "@brief Sets the routing layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
  ) +
  gsi::method ("clear_routing_suffixes_per_mask", &db::LEFDEFReaderOptions::clear_routing_suffixes_per_mask,
    "@brief Clears the routing layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("clear_routing_datatypes_per_mask", &db::LEFDEFReaderOptions::clear_routing_datatypes_per_mask,
    "@brief Clears the routing layer datatypes per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("routing_suffix_per_mask", &db::LEFDEFReaderOptions::routing_suffix_per_mask, gsi::arg ("mask"),
    "@brief Gets the routing geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_routing_suffix_per_mask", &db::LEFDEFReaderOptions::set_routing_suffix_per_mask, gsi::arg ("mask"), gsi::arg ("suffix"),
    "@brief Sets the routing geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("routing_datatype", &db::LEFDEFReaderOptions::routing_datatype_per_mask, gsi::arg ("mask"),
    "@brief Gets the routing geometry layer datatype value per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_routing_datatype_per_mask", &db::LEFDEFReaderOptions::set_routing_datatype_per_mask, gsi::arg ("mask"), gsi::arg ("datatype"),
    "@brief Sets the routing geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("routing_suffix_str", &db::LEFDEFReaderOptions::routing_suffix_str,
    "@hide"
  ) +
  gsi::method ("routing_suffix_str=", &db::LEFDEFReaderOptions::set_routing_suffix_str, gsi::arg ("suffix"),
    "@hide"
  ) +
  gsi::method ("routing_datatype_str", &db::LEFDEFReaderOptions::routing_datatype_str,
    "@hide"
  ) +
  gsi::method ("routing_datatype_str=", &db::LEFDEFReaderOptions::set_routing_datatype_str, gsi::arg ("datatype"),
    "@hide"
  ) +
  gsi::method ("produce_special_routing", &db::LEFDEFReaderOptions::produce_special_routing,
    "@brief Gets a value indicating whether special routing geometry shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules.\n"
    "\n"
    "The differentiation between special and normal routing has been introduced in version 0.27."
  ) +
  gsi::method ("produce_special_routing=", &db::LEFDEFReaderOptions::set_produce_special_routing, gsi::arg ("produce"),
    "@brief Sets a value indicating whether special routing geometry shall be produced.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n"
    "The differentiation between special and normal routing has been introduced in version 0.27."
  ) +
  gsi::method ("special_routing_suffix", &db::LEFDEFReaderOptions::special_routing_suffix,
    "@brief Gets the special routing layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n"
    "The differentiation between special and normal routing has been introduced in version 0.27."
  ) +
  gsi::method ("special_routing_suffix=", &db::LEFDEFReaderOptions::set_special_routing_suffix, gsi::arg ("suffix"),
    "@brief Sets the special routing layer name suffix.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n"
    "The differentiation between special and normal routing has been introduced in version 0.27."
  ) +
  gsi::method ("special_routing_datatype", &db::LEFDEFReaderOptions::special_routing_datatype,
    "@brief Gets the special routing layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n"
    "The differentiation between special and normal routing has been introduced in version 0.27."
  ) +
  gsi::method ("special_routing_datatype=", &db::LEFDEFReaderOptions::set_special_routing_datatype, gsi::arg ("datatype"),
    "@brief Sets the special routing layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "\n"
    "The differentiation between special and normal routing has been introduced in version 0.27."
  ) +
  gsi::method ("clear_special_routing_suffixes_per_mask", &db::LEFDEFReaderOptions::clear_special_routing_suffixes_per_mask,
    "@brief Clears the special routing layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("clear_special_routing_datatypes_per_mask", &db::LEFDEFReaderOptions::clear_special_routing_datatypes_per_mask,
    "@brief Clears the special routing layer datatypes per mask.\n"
    "See \\produce_via_geometry for details about this property.\n"
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("special_routing_suffix_per_mask", &db::LEFDEFReaderOptions::special_routing_suffix_per_mask, gsi::arg ("mask"),
    "@brief Gets the special routing geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_special_routing_suffix_per_mask", &db::LEFDEFReaderOptions::set_special_routing_suffix_per_mask, gsi::arg ("mask"), gsi::arg ("suffix"),
    "@brief Sets the special routing geometry layer name suffix per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("special_routing_datatype", &db::LEFDEFReaderOptions::special_routing_datatype_per_mask, gsi::arg ("mask"),
    "@brief Gets the special routing geometry layer datatype value per mask.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("set_special_routing_datatype_per_mask", &db::LEFDEFReaderOptions::set_special_routing_datatype_per_mask, gsi::arg ("mask"), gsi::arg ("datatype"),
    "@brief Sets the special routing geometry layer datatype value.\n"
    "See \\produce_via_geometry for details about the layer production rules."
    "The mask number is a zero-based mask index (0: MASK 1, 1: MASK 2 ...)."
    "\n\n"
    "Mask specific rules have been introduced in version 0.27."
  ) +
  gsi::method ("special_routing_suffix_str", &db::LEFDEFReaderOptions::special_routing_suffix_str,
    "@hide"
  ) +
  gsi::method ("special_routing_suffix_str=", &db::LEFDEFReaderOptions::set_special_routing_suffix_str, gsi::arg ("suffix"),
    "@hide"
  ) +
  gsi::method ("special_routing_datatype_str", &db::LEFDEFReaderOptions::special_routing_datatype_str,
    "@hide"
  ) +
  gsi::method ("special_routing_datatype_str=", &db::LEFDEFReaderOptions::set_special_routing_datatype_str, gsi::arg ("datatype"),
    "@hide"
  ) +
  gsi::method ("separate_groups", &db::LEFDEFReaderOptions::separate_groups,
    "@brief Gets a value indicating whether to create separate parent cells for individual groups.\n"
    "If this property is set to true, instances belonging to different groups are separated by putting them into "
    "individual parent cells. These parent cells are named after the groups and are put into the master top cell.\n"
    "If this property is set to false (the default), no such group parents will be formed."
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("separate_groups=", &db::LEFDEFReaderOptions::set_separate_groups, gsi::arg ("flag"),
    "@brief Sets a value indicating whether to create separate parent cells for individual groups.\n"
    "See \\separate_groups for details about this property.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("joined_paths", &db::LEFDEFReaderOptions::joined_paths,
    "@brief Gets a value indicating whether to create joined paths for wires.\n"
    "If this property is set to true, wires are represented by multi-segment paths as far as possible "
    "(this will fail for 45 degree path segments for example). By defauls, wires are represented "
    "by multiple straight segments.\n"
    "\n"
    "This property has been added in version 0.28.8.\n"
  ) +
  gsi::method ("joined_paths=", &db::LEFDEFReaderOptions::set_joined_paths, gsi::arg ("flag"),
    "@brief Sets a value indicating whether to create joined paths for wires.\n"
    "See \\joined_paths for details about this property.\n"
    "\n"
    "This property has been added in version 0.28.8.\n"
  ) +
  gsi::method ("map_file", &db::LEFDEFReaderOptions::map_file,
    "@brief Gets the layer map file to use.\n"
    "If a layer map file is given, the reader will pull the layer mapping from this file. The layer mapping rules "
    "specified in the reader options are ignored in this case. These are the name suffix rules for vias, blockages, routing, "
    "special routing, pins etc. and the corresponding datatype rules. The \\layer_map attribute will also be ignored. "
    "\n"
    "The layer map file path will be resolved relative to the technology base path if the LEF/DEF reader options are "
    "used in the context of a technology.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("map_file=", &db::LEFDEFReaderOptions::set_map_file, gsi::arg ("file"),
    "@brief Sets the layer map file to use.\n"
    "See \\map_file for details about this property.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("macro_resolution_mode", &db::LEFDEFReaderOptions::macro_resolution_mode,
    "@brief Gets the macro resolution mode (LEF macros into DEF).\n"
    "This property describes the way LEF macros are turned into layout cells when reading DEF. There "
    "are three modes available:\n"
    "\n"
    "@ul\n"
    "  @li 0: produce LEF geometry unless a FOREIGN cell is specified @/li\n"
    "  @li 1: produce LEF geometry always and ignore FOREIGN @/li\n"
    "  @li 2: Never produce LEF geometry and assume FOREIGN always @/li\n"
    "@/ul\n"
    "\n"
    "If substitution layouts are specified with \\macro_layouts, these are used to provide "
    "macro layouts in case no LEF geometry is taken.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("macro_resolution_mode=", &db::LEFDEFReaderOptions::set_macro_resolution_mode, gsi::arg ("mode"),
    "@brief Sets the macro resolution mode (LEF macros into DEF).\n"
    "See \\macro_resolution_mode for details about this property.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("macro_layouts", &db::LEFDEFReaderOptions::macro_layouts,
    "@brief Gets the layout objects used for resolving LEF macros in the DEF reader.\n"
    "The DEF reader can either use LEF geometry or use a separate source of layouts for the "
    "LEF macros. The \\macro_resolution_mode controls whether to use LEF geometry. If LEF geometry is not "
    "used, the DEF reader will look up macro cells from the \\macro_layouts and pull cell layouts from there.\n"
    "\n"
    "The LEF cells are looked up as cells by name from the macro layouts in the order these are given in this array.\n"
    "\n"
    "\\macro_layout_files is another way of specifying such substitution layouts. This method accepts file names instead of layout objects.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("macro_layouts=", &db::LEFDEFReaderOptions::set_macro_layouts, gsi::arg ("layouts"),
    "@brief Sets the layout objects used for resolving LEF macros in the DEF reader.\n"
    "See \\macro_layouts for more details about this property.\n"
    "\n"
    "Layout objects specified in the array for this property are not owned by the \\LEFDEFReaderConfiguration object. "
    "Be sure to keep some other reference to these Layout objects if you are storing away the LEF/DEF reader configuration object.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("macro_layout_files", &db::LEFDEFReaderOptions::macro_layout_files,
    "@brief Gets the list of layout files to read for substituting macros in DEF\n"
    "These files play the same role than the macro layouts (see \\macro_layouts), except that this property specifies a list of file names. "
    "The given files are loaded automatically to resolve macro layouts instead of LEF geometry. See \\macro_resolution_mode for details when this happens. "
    "Relative paths are resolved relative to the DEF file to read or relative to the technology base path.\n"
    "Macros in need for substitution are looked up in the layout files by searching for cells with the same name. "
    "The files are scanned in the order they are given in the file list.\n"
    "The files from \\macro_layout_files are scanned after the layout objects specified with \\macro_layouts.\n"
    "\n"
    "The setter for this property is \\macro_layout_files=.\n"
    "\n"
    "This property has been added in version 0.27.1.\n"
  ) +
  gsi::method ("macro_layout_files=", &db::LEFDEFReaderOptions::set_macro_layout_files, gsi::arg ("file_paths"),
    "@brief Sets the list of layout files to read for substituting macros in DEF\n"
    "See \\macro_layout_files for details.\n"
    "\n"
    "This property has been added in version 0.27.1.\n"
  ) +
  gsi::method ("lef_files", &db::LEFDEFReaderOptions::lef_files,
    "@brief Gets the list technology LEF files to additionally import\n"
    "Returns a list of path names for technology LEF files to read in addition to the primary file. "
    "Relative paths are resolved relative to the file to read or relative to the technology base path.\n"
    "\n"
    "The setter for this property is \\lef_files=."
  ) +
  gsi::method ("lef_files=", &db::LEFDEFReaderOptions::set_lef_files, gsi::arg ("lef_file_paths"),
    "@brief Sets the list technology LEF files to additionally import\n"
    "See \\lef_files for details."
  ) +
  gsi::method ("read_lef_with_def", &db::LEFDEFReaderOptions::read_lef_with_def,
    "@brief Gets a value indicating whether to read all LEF files in the same directory than the DEF file.\n"
    "If this property is set to true (the default), the DEF reader will automatically consume all LEF files "
    "next to the DEF file in addition to the LEF files specified with \\lef_files. If set to false, only the "
    "LEF files specified with \\lef_files will be read.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ) +
  gsi::method ("read_lef_with_def=", &db::LEFDEFReaderOptions::set_read_lef_with_def, gsi::arg ("flag"),
    "@brief Sets a value indicating whether to read all LEF files in the same directory than the DEF file.\n"
    "See \\read_lef_with_def for details about this property.\n"
    "\n"
    "This property has been added in version 0.27.\n"
  ),
  "@brief Detailed LEF/DEF reader options\n"
  "This class is a aggregate belonging to the \\LoadLayoutOptions class. It provides options for the LEF/DEF reader. "
  "These options have been placed into a separate class to account for their complexity."
  "\n"
  "This class specifically handles layer mapping. This is the process of generating layer names or GDS layer/datatypes "
  "from LEF/DEF layers and purpose combinations. There are basically two ways: to use a map file or to use pattern-based production rules.\n"
  "\n"
  "To use a layer map file, set the \\map_file attribute to the name of the layer map file. The layer map "
  "file lists the GDS layer and datatype numbers to generate for the geometry.\n"
  "\n"
  "The pattern-based approach will use the layer name and attach a purpose-dependent suffix to it. "
  "Use the ..._suffix attributes to specify this suffix. For routing, the corresponding attribute is \\routing_suffix for example. "
  "A purpose can also be mapped to a specific GDS datatype using the corresponding ..._datatype attributes.\n"
  "The decorated or undecorated names are looked up in a layer mapping table in the next step. The layer mapping table "
  "is specified using the \\layer_map attribute. This table can be used to map layer names to specific GDS layers "
  "by using entries of the form 'NAME: layer-number'.\n"
  "\n"
  "If a layer map file is present, the pattern-based attributes are ignored.\n"
);

}


/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#include "bdReaderOptions.h"
#include "dbLoadLayoutOptions.h"
#include "tlCommandLineParser.h"

namespace bd
{

GenericReaderOptions::GenericReaderOptions ()
  : m_prefix ("i"), m_group_prefix ("Input"), m_layer_map (), m_create_other_layers (true),
    m_dbu (0.001), m_keep_layer_names (false)
{
  //  initialize from the default settings

  db::LoadLayoutOptions load_options;

  m_common_enable_text_objects = load_options.get_option_by_name ("text_enabled").to_bool ();
  m_common_enable_properties = load_options.get_option_by_name ("properties_enabled").to_bool ();

  m_gds2_box_mode = load_options.get_option_by_name ("gds2_box_mode").to_uint ();
  m_gds2_allow_big_records = load_options.get_option_by_name ("gds2_allow_big_records").to_bool ();
  m_gds2_allow_multi_xy_records = load_options.get_option_by_name ("gds2_allow_multi_xy_records").to_bool ();

  m_oasis_read_all_properties = load_options.get_option_by_name ("oasis_read_all_properties").to_bool ();
  m_oasis_expect_strict_mode = (load_options.get_option_by_name ("oasis_expect_strict_mode").to_int () > 0);

  m_create_other_layers = load_options.get_option_by_name ("cif_create_other_layers").to_bool ();
  m_cif_wire_mode = load_options.get_option_by_name ("cif_wire_mode").to_uint ();

  m_dxf_unit = load_options.get_option_by_name ("dxf_unit").to_double ();
  m_dxf_text_scaling = load_options.get_option_by_name ("dxf_text_scaling").to_double ();
  m_dxf_polyline_mode = load_options.get_option_by_name ("dxf_polyline_mode").to_int ();
  m_dxf_circle_points = load_options.get_option_by_name ("dxf_circle_points").to_int ();
  m_dxf_circle_accuracy = load_options.get_option_by_name ("dxf_circle_accuracy").to_double ();
  m_dxf_contour_accuracy = load_options.get_option_by_name ("dxf_contour_accuracy").to_double ();
  m_dxf_render_texts_as_polygons = load_options.get_option_by_name ("dxf_render_texts_as_polygons").to_bool ();
  m_dxf_keep_other_cells = load_options.get_option_by_name ("dxf_keep_other_cells").to_bool ();

  m_magic_lambda = load_options.get_option_by_name ("mag_lambda").to_double ();
  m_magic_merge = load_options.get_option_by_name ("mag_merge").to_bool ();

  tl::Variant mag_library_paths = load_options.get_option_by_name ("mag_library_paths");
  for (tl::Variant::const_iterator i = mag_library_paths.begin (); i != mag_library_paths.end (); ++i) {
    m_magic_lib_path.push_back (i->to_string ());
  }

  tl::Variant v;

  v = load_options.get_option_by_name ("lefdef_config.net_property_name");
  m_lefdef_produce_net_names = ! v.is_nil ();
  m_lefdef_net_property_name = v .to_parsable_string ();

  v = load_options.get_option_by_name ("lefdef_config.instance_property_name");
  m_lefdef_produce_inst_names = ! v.is_nil ();
  m_lefdef_inst_property_name = v .to_parsable_string ();

  v = load_options.get_option_by_name ("lefdef_config.pin_property_name");
  m_lefdef_produce_pin_names = ! v.is_nil ();
  m_lefdef_pin_property_name = v .to_parsable_string ();

  m_lefdef_produce_cell_outlines = load_options.get_option_by_name ("lefdef_config.produce_cell_outlines").to_bool ();
  m_lefdef_cell_outline_layer = load_options.get_option_by_name ("lefdef_config.cell_outline_layer").to_string ();
  m_lefdef_produce_placement_blockages = load_options.get_option_by_name ("lefdef_config.produce_placement_blockages").to_bool ();
  m_lefdef_placement_blockage_layer = load_options.get_option_by_name ("lefdef_config.placement_blockage_layer").to_string ();
  m_lefdef_produce_regions = load_options.get_option_by_name ("lefdef_config.produce_regions").to_bool ();
  m_lefdef_region_layer = load_options.get_option_by_name ("lefdef_config.region_layer").to_string ();
  m_lefdef_produce_via_geometry = load_options.get_option_by_name ("lefdef_config.produce_via_geometry").to_bool ();
  m_lefdef_via_geometry_suffix = load_options.get_option_by_name ("lefdef_config.via_geometry_suffix_str").to_string ();
  m_lefdef_via_geometry_datatype = load_options.get_option_by_name ("lefdef_config.via_geometry_datatype_str").to_string ();
  m_lefdef_via_cellname_prefix = load_options.get_option_by_name ("lefdef_config.via_cellname_prefix").to_string ();
  m_lefdef_produce_pins = load_options.get_option_by_name ("lefdef_config.produce_pins").to_bool ();
  m_lefdef_pins_suffix = load_options.get_option_by_name ("lefdef_config.pins_suffix_str").to_string ();
  m_lefdef_pins_datatype = load_options.get_option_by_name ("lefdef_config.pins_datatype_str").to_string ();
  m_lefdef_produce_lef_pins = load_options.get_option_by_name ("lefdef_config.produce_lef_pins").to_bool ();
  m_lefdef_lef_pins_suffix = load_options.get_option_by_name ("lefdef_config.lef_pins_suffix_str").to_string ();
  m_lefdef_lef_pins_datatype = load_options.get_option_by_name ("lefdef_config.lef_pins_datatype_str").to_string ();
  m_lefdef_produce_obstructions = load_options.get_option_by_name ("lefdef_config.produce_obstructions").to_bool ();
  m_lefdef_obstruction_suffix = load_options.get_option_by_name ("lefdef_config.obstructions_suffix").to_string ();
  m_lefdef_obstruction_datatype = load_options.get_option_by_name ("lefdef_config.obstructions_datatype").to_int ();
  m_lefdef_produce_blockages = load_options.get_option_by_name ("lefdef_config.produce_blockages").to_bool ();
  m_lefdef_blockage_suffix = load_options.get_option_by_name ("lefdef_config.blockages_suffix").to_string ();
  m_lefdef_blockage_datatype = load_options.get_option_by_name ("lefdef_config.blockages_datatype").to_int ();
  m_lefdef_produce_labels = load_options.get_option_by_name ("lefdef_config.produce_labels").to_bool ();
  m_lefdef_label_suffix = load_options.get_option_by_name ("lefdef_config.labels_suffix").to_string ();
  m_lefdef_label_datatype = load_options.get_option_by_name ("lefdef_config.labels_datatype").to_int ();
  m_lefdef_produce_routing = load_options.get_option_by_name ("lefdef_config.produce_routing").to_bool ();
  m_lefdef_routing_suffix = load_options.get_option_by_name ("lefdef_config.routing_suffix_str").to_string ();
  m_lefdef_routing_datatype = load_options.get_option_by_name ("lefdef_config.routing_datatype_str").to_string ();
  m_lefdef_produce_special_routing = load_options.get_option_by_name ("lefdef_config.produce_special_routing").to_bool ();
  m_lefdef_special_routing_suffix = load_options.get_option_by_name ("lefdef_config.special_routing_suffix_str").to_string ();
  m_lefdef_special_routing_datatype = load_options.get_option_by_name ("lefdef_config.special_routing_datatype_str").to_string ();

  tl::Variant lef_files = load_options.get_option_by_name ("lefdef_config.lef_files");
  for (tl::Variant::const_iterator i = lef_files.begin (); i != lef_files.end (); ++i) {
    m_lefdef_lef_files.push_back (i->to_string ());
  }

  m_lefdef_read_lef_with_def = load_options.get_option_by_name ("lefdef_config.read_lef_with_def").to_bool ();
  m_lefdef_separate_groups = load_options.get_option_by_name ("lefdef_config.separate_groups").to_bool ();
  m_lefdef_map_file = load_options.get_option_by_name ("lefdef_config.map_file").to_string ();
  m_lefdef_produce_lef_macros = (load_options.get_option_by_name ("lefdef_config.macro_resolution_mode").to_int () == 0);
}

void
GenericReaderOptions::add_options (tl::CommandLineOptions &cmd)
{
  {
    std::string group ("[" + m_group_prefix + " options - General]");

    cmd << tl::arg (group +
                    "!-" + m_prefix + "s|--" + m_long_prefix + "skip-unknown-layers", &m_create_other_layers, "Skips unknown layers",
                    "This option is effective with the the --layer-map option. If combined with "
                    "--skip-unknown-layers, layers not listed in the layer map will not be read. "
                    "By default, corresponding entries are created also for unknown layers."
                   )
        << tl::arg (group +
                    "-" + m_prefix + "m|--" + m_long_prefix + "layer-map=map", this, &GenericReaderOptions::set_layer_map, "Specifies the layer mapping for the input",
                    "This option specifies a layer selection or mapping. The selection or mapping is a sequence of source and optional "
                    "target specifications. The specifications are separated by blanks or double-slash sequences (//).\n"
                    "\n"
                    "A source specification can apply to a single or many source layers. If many source layers are "
                    "selected, they are combined into a single target layer. A source specification is:\n"
                    "\n"
                    "* A list of source specs, separated by semicolon characters (;)\n"
                    "* A layer name (in double or single quotes if necessary)\n"
                    "* A layer/datatype pair or range separated with a slash\n"
                    "* Layer and datatype can be simple positive integer numbers\n"
                    "* Layer and datatype numbers can be enumerated (numbers separated with a comma)\n"
                    "* Layer and datatype numbers can be ranges formed with a dash separator\n"
                    "\n"
                    "Target specifications are added to source specifications with a colon (:). If a target "
                    "layer is specified, all source layers addressed with the source specification are "
                    "combined into this target layer.\n"
                    "\n"
                    "Examples:\n"
                    "\n"
                    "* 1/0 2/0 3/0-255:17/0\n"
                    "  Selects 1/0, 2/0 and maps layer 3, datatype 0 to 255 to layer 17, datatype 0\n"
                    "\n"
                    "* A:1/0 B:2/0\n"
                    "  Maps named layer A to 1/0 and named layer B to 2/0"
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - GDS2 and OASIS specific]");

    cmd << tl::arg (group +
                    "#!--" + m_long_prefix + "no-texts", &m_common_enable_text_objects, "Skips text objects",
                    "With this option set, text objects won't be read."
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "no-properties", &m_common_enable_properties, "Skips properties",
                    "With this option set, properties won't be read."
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - GDS2 specific]");

    cmd << tl::arg (group +
                    "#!--" + m_long_prefix + "no-multi-xy-records", &m_gds2_allow_multi_xy_records, "Gives an error on multi-XY records",
                    "This option disables an advanced interpretation of GDS2 which allows unlimited polygon and path "
                    "complexity. For compatibility with other readers, this option restores the standard behavior and "
                    "disables this feature."
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "no-big-records", &m_gds2_allow_big_records, "Gives an error on big (>32767 bytes) records",
                    "The GDS2 specification claims the record length to be a signed 16 bit value. So a record "
                    "can be 32767 bytes max. To allow bigger records (i.e. bigger polygons), the usual approach "
                    "is to take the length as a unsigned 16 bit value, so the length is up to 65535 bytes. "
                    "This option restores the original behavior and reports big (>32767 bytes) records are errors."
                   )
        << tl::arg (group +
                    "-" + m_prefix + "b|--" + m_long_prefix + "box-mode=mode", &m_gds2_box_mode, "Specifies how BOX records are read",
                    "This an option provided for compatibility with other readers. The mode value specifies how "
                    "BOX records are read:\n"
                    "\n"
                    "* 0: ignore BOX records\n"
                    "* 1: treat as rectangles (the default)\n"
                    "* 2: treat as boundaries\n"
                    "* 3: treat as errors"
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - OASIS specific]");

    cmd << tl::arg (group +
                    "#--" + m_long_prefix + "expect-strict-mode=mode", &m_oasis_expect_strict_mode, "Makes the reader expect strict or non-strict mode",
                    "With this option, the OASIS reader will expect strict mode (mode is 1) or expect non-strict mode "
                    "(mode is 0). By default, both modes are allowed. This is a diagnostic feature and does not "
                    "have any other effect than checking the mode."
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - CIF and DXF specific]");

    cmd << tl::arg (group +
                    "-" + m_prefix + "d|--" + m_long_prefix + "dbu-in=dbu", this, &GenericReaderOptions::set_dbu, "Specifies the database unit to use",
                    "This option specifies the database unit the resulting layer will have. "
                    "The value is given in micrometer units. The default value is 1nm (0.001)."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "keep-layer-names", this, &GenericReaderOptions::set_read_named_layers, "Keeps layer names",
                    "If this option is used, layers names are kept as pure names and no attempt is made to\n"
                    "translate them into GDS layer/datatypes."
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - CIF specific]");

    cmd << tl::arg (group +
                    "-" + m_prefix + "w|--" + m_long_prefix + "wire-mode=mode", &m_cif_wire_mode, "Specifies how wires (W) are read",
                    "This option specifies how wire objects (W) are read:\n"
                    "\n"
                    "* 0: as square ended paths (the default)\n"
                    "* 1: as flush ended paths\n"
                    "* 2: as round paths"
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - DXF specific]");

    cmd << tl::arg (group +
                    "-" + m_prefix + "u|--" + m_long_prefix + "dxf-unit=unit", &m_dxf_unit, "Specifies the DXF drawing units",
                    "Since DXF is unitless, this value needs to be given to specify the drawing units. "
                    "By default, a drawing unit of micrometers is assumed."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-text-scaling=factor", &m_dxf_text_scaling, "Specifies text scaling",
                    "This value specifies text scaling in percent. A value of 100 roughly means that the letter "
                    "pitch of the font will be 92% of the specified text height. That value applies for ROMANS fonts. "
                    "When generating GDS texts, a value of 100 generates TEXT objects with "
                    "the specified size. Smaller values generate smaller sizes."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-polyline-mode=mode", &m_dxf_polyline_mode, "Specifies how POLYLINE records are handled",
                    "This value specifies how POLYLINE records are handled:\n"
                    "\n"
                    "* 0: automatic mode (default)\n"
                    "* 1: keep lines\n"
                    "* 2: create polygons from closed POLYLINE/LWPOLYLINE with width == 0\n"
                    "* 3: merge all lines (width width 0)\n"
                    "* 4: as 3 and auto-close contours"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-circle-points=points", &m_dxf_circle_points, "Specifies the number of points for a full circle for arc interpolation",
                    "See --" + m_long_prefix + "dxf-circle-accuracy for another way of specifying the number of points per circle."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-circle-accuracy=value", &m_dxf_circle_accuracy, "Specifies the accuracy of circle approximation",
                    "This value specifies the approximation accuracy of the circle and other\n"
                    "\"round\" structures. If this value is a positive number bigger than the\n"
                    "database unit (see dbu), it will control the number of points the\n"
                    "circle is resolved into. The number of points will be chosen such that\n"
                    "the deviation from the ideal curve is less than this value.\n"
                    "\n"
                    "The actual number of points used for the circle approximation is\n"
                    "not larger than circle_points.\n"
                    "\n"
                    "The value is given in the units of the DXF file."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-contour-accuracy=value", &m_dxf_contour_accuracy, "Specifies the point accuracy for contour closing",
                    "This value specifies the distance (in units of the DXF file) by which points can be separated and still\n"
                    "be considered to be connected. This value is effective in polyline mode 3 and 4.\n"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-render-texts-as-polygons", &m_dxf_render_texts_as_polygons, "Renders texts as polygons",
                    "If this option is used, texts are converted to polygons instead of being converted to labels."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-keep-other-cells", &m_dxf_keep_other_cells, "Keeps cells which are not instantiated by the top cell",
                    "With this option, all cells not found to be instantiated are kept as additional top cells. "
                    "By default, such cells are removed."
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - MAG (Magic) specific]");

    cmd << tl::arg (group +
                    "--" + m_long_prefix + "magic-lambda=lambda", &m_magic_lambda, "Specifies the lambda value",
                    "The lambda value is used as a scaling factor to turn the dimensionless Magic drawings into "
                    "physical layout."
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "magic-dont-merge", &m_magic_merge, "Disables polygon merging",
                    "With this option, the rectangles and triangles of the Magic file are not merged into polygons."
                   )
        << tl::arg (group +
                    "--" + m_long_prefix + "magic-lib-path=path", &m_magic_lib_path, "Specifies the library search path for Magic file loading",
                    "The library search path gives the locations where the reader looks up files for child cells. "
                    "This option either specifies a comma-separated list of paths to search or it can be present multiple times "
                    "for multiple search locations."
                   )
      ;
  }


  {
    std::string group ("[" + m_group_prefix + " options - LEF/DEF specific]");

    cmd << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-net-names", &m_lefdef_produce_net_names, "Disables producing net names as shape properties",
                    "If this option is present, net names will not be emitted as shape properties."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-net-property-name=spec", &m_lefdef_net_property_name, "Specifies which property name to use for net names",
                    "This option gives the name of the shape property used to annotate net names. For 'spec' use:\n"
                    "\n"
                    "* \"#n\" for property number \"n\" (compatible with GDS2)\n"
                    "* A plain word for a named property (not compatible with GDS2)\n"
                    "\n"
                    "Producing net name annotation properties can be turned off with '--" + m_long_prefix + "lefdef-dont-produce-net-names'."
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-instance-names", &m_lefdef_produce_inst_names, "Disables producing DEF macro instance names as instance properties",
                    "If this option is present, DEF macro instance names will not be emitted as instance properties."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-instance-property-name=spec", &m_lefdef_inst_property_name, "Specifies which property name to use for DEF macro instance names",
                    "This option gives the name of the instance property used to annotate DEF macro instance names. "
                    "For the 'spec' format see '--" + m_long_prefix + "lefdef-net-property-name'."
                    "\n"
                    "Producing instance name annotation properties can be turned off with '--" + m_long_prefix + "lefdef-dont-produce-instance-names'."
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-pin-names", &m_lefdef_produce_pin_names, "Disables producing pin names as shape or instance properties",
                    "If this option is present, Pin names will not be emitted as shape or instance properties."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-pin-property-name=spec", &m_lefdef_pin_property_name, "Specifies which property name to use for pin names",
                    "This option gives the name of the shape or instance property used to annotate pin names. "
                    "For the 'spec' format see '--" + m_long_prefix + "lefdef-net-property-name'."
                    "\n"
                    "Producing pin name annotation properties can be turned off with '--" + m_long_prefix + "lefdef-dont-produce-pin-names'."
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-cell-outlines", &m_lefdef_produce_cell_outlines, "Disables producing cell outlines",
                    "If this option is present, cell outlines will be skipped. Otherwise the cell outlines will be written to a layer given with '--" + m_long_prefix + "lefdef-cell-outline-layer'."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-cell-outline-layer=spec", &m_lefdef_cell_outline_layer, "Specifies which layer to use for the cell outlines",
                    "This option specifies the layer to use for the cell outline polygons. For 'spec' use:\n"
                    "\n"
                    "* \"l\" or \"l/d\" for a numerical layer or layer/datatype combination.\n"
                    "* A plain word for a named layer\n"
                    "* A name followed by a layer or layer/datatype combination in round brackets for a combined specification\n"
                    "\n"
                    "Producing cell outline markers can be turned off with '--" + m_long_prefix + "lefdef-dont-produce-cell-outlines'."
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-placement-blockages", &m_lefdef_produce_placement_blockages, "Disables producing blockage markers",
                    "If this option is present, blockages will be skipped. Otherwise the blockage markers will be written to a layer given with '--" + m_long_prefix + "lefdef-placement-blockage-layer'."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-placement-blockage-layer=spec", &m_lefdef_placement_blockage_layer, "Specifies which layer to use for the placement blockage markers",
                    "For the 'spec' format see '--" + m_long_prefix + "lefdef-cell-outline-layer'.\n"
                    "\n"
                    "Producing cell placement blockage markers can be turned off with '--" + m_long_prefix + "lefdef-dont-produce-placement-blockages'."
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-regions", &m_lefdef_produce_regions, "Disables producing regions",
                    "If this option is present, regions will be skipped. Otherwise the regions will be written to a layer given with '--" + m_long_prefix + "lefdef-region-layer'."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-region-layer=spec", &m_lefdef_region_layer, "Specifies which layer to use for the regions",
                    "For the 'spec' format see '--" + m_long_prefix + "lefdef-cell-outline-layer'.\n"
                    "\n"
                    "Producing regions can be turned off with '--" + m_long_prefix + "lefdef-dont-produce-regions'."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-separate-groups", &m_lefdef_separate_groups, "Specifies to separate groups of regions into a hierarchy",
                    "This option is used together with '--" + m_long_prefix + "lefdef-produce-regions'. If given, the region polygons will be put "
                    "into a cell hierarchy where the cells indicate the region groups.\n"
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-via-geometry", &m_lefdef_produce_via_geometry, "Skips vias when producing geometry",
                    "If this option is given, no via geometry will be produced."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-via-geometry-suffix", &m_lefdef_via_geometry_suffix, "Specifies the via geometry layer suffix in pattern-based mode",
                    "Use '" + m_long_prefix + "lefdef-via-geometry-suffix' and '--" + m_long_prefix + "lefdef-via-geometry-datatype' together with "
                    "a layer map (see '-" + m_prefix + "m') to customize where the via geometry will be put.\n"
                    "\n"
                    "This option is part of the 'pattern-based' LEF/DEF layer mapping scheme.\n"
                    "\n"
                    "The mechanism is this: from the geometry's layer name and the suffix an effective layer name is produced. For example if the "
                    "geometry is on layer 'M1' and the suffix is '_VIA', the effective layer name will be 'M1_VIA'. This layer is looked up in the "
                    "layer map. If no such layer is found, the geometry layer name without suffix is looked up. If this layer is found, the datatype "
                    "is substituted by the datatype specified with the '--" + m_long_prefix + "lefdef-via-geometry-datatype'. So eventually it's "
                    "possible to use a detailed mapping by layer name + suffix or a generic mapping by layer name + datatype.\n"
                    "\n"
                    "Suffix and datatype can be made MASK specific by giving a list of values in the form: \"<generic>,1:<for-mask1>,2:<for-mask2>...\". "
                    "For example, a datatype specification of \"6,1:61,2:62\" will use datatype 6 for via geometry without a mask assignment, "
                    "datatype 61 for via geometry assigned to MASK 1 and datatype 62 for via geometry assigned to MASK 2.\n"
                    "\n"
                    "An alternative way to provide a layer mapping is through a map file (see '--" + m_long_prefix + "lefdef-map-file')."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-via-geometry-datatype", &m_lefdef_via_geometry_datatype, "Specifies the via geometry layer datatype in pattern-based mode",
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of this option.\n"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-via-cell-prefix", &m_lefdef_via_cellname_prefix, "Specifies the prefix for the cell names generated for vias",
                    "Vias will be put into their own cells by the LEF/DEF reader. This option gives a prefix that is used to form the name of "
                    "these cells. The name is built from the prefix plus the via name.\n"
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-pins", &m_lefdef_produce_pins, "Skips pins when producing geometry",
                    "If this option is given, no pin geometry will be produced."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-pins-suffix", &m_lefdef_pins_suffix, "Specifies the pin geometry layer suffix in pattern-based mode",
                    "The pin geometry generation and layer mapping is designed in the same way than via geometry mapping. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-pins-datatype", &m_lefdef_pins_datatype, "Specifies the pin geometry layer datatype in pattern-based mode",
                    "The pin geometry generation and layer mapping is designed in the same way than via geometry mapping. "
                    "See '--" + m_long_prefix + "lefdef-produce-via-geometry' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-lef-pins", &m_lefdef_produce_lef_pins, "Skips LEF pins when producing geometry",
                    "If this option is given, no LEF pin geometry will be produced."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-lef-pins-suffix", &m_lefdef_lef_pins_suffix, "Specifies the LEF pin geometry layer suffix in pattern-based mode",
                    "The LEF pin geometry generation and layer mapping is designed in the same way than via geometry mapping. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-lef-pins-datatype", &m_lefdef_lef_pins_datatype, "Specifies the LEF pin geometry layer datatype in pattern-based mode",
                    "The LEF pin geometry generation and layer mapping is designed in the same way than via geometry mapping. "
                    "See '--" + m_long_prefix + "lefdef-produce-via-geometry' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-routing", &m_lefdef_produce_routing, "Skips routing when producing geometry",
                    "If this option is given, no routing geometry will be produced."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-routing-suffix", &m_lefdef_routing_suffix, "Specifies the routing geometry layer suffix in pattern-based mode",
                    "The routing geometry generation and layer mapping is designed in the same way than via geometry mapping. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-routing-datatype", &m_lefdef_routing_datatype, "Specifies the routing geometry layer datatype in pattern-based mode",
                    "The routing geometry generation and layer mapping is designed in the same way than via geometry mapping. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-special-routing", &m_lefdef_produce_special_routing, "Skips special routing when producing geometry",
                    "If this option is given, no special routing geometry will be produced."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-special-routing-suffix", &m_lefdef_special_routing_suffix, "Specifies the special routing geometry layer suffix in pattern-based mode",
                    "The special routing geometry generation and layer mapping is designed in the same way than via geometry mapping. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-special-routing-datatype", &m_lefdef_special_routing_datatype, "Specifies the special routing geometry layer datatype in pattern-based mode",
                    "The special routing geometry generation and layer mapping is designed in the same way than via geometry mapping. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-obstructions", &m_lefdef_produce_obstructions, "Skips obstructions when producing geometry",
                    "If this option is given, no obstruction marker geometry will be produced."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-obstruction-suffix", &m_lefdef_obstruction_suffix, "Specifies the obstruction markers layer suffix in pattern-based mode",
                    "The obstruction marker generation and layer mapping is designed in the same way than via geometry mapping, except the option to use mask specific target layers. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-obstruction-datatype", &m_lefdef_obstruction_datatype, "Specifies the obstruction markers layer datatype in pattern-based mode",
                    "The obstruction marker generation and layer mapping is designed in the same way than via geometry mapping, except the option to use mask specific target layers. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-blockages", &m_lefdef_produce_blockages, "Skips blockages when producing geometry",
                    "If this option is given, no blockage geometry will be produced."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-blockage-suffix", &m_lefdef_blockage_suffix, "Specifies the blockage markers layer suffix in pattern-based mode",
                    "The blockage marker generation and layer mapping is designed in the same way than via geometry mapping, except the option to use mask specific target layers. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-blockage-datatype", &m_lefdef_blockage_datatype, "Specifies the blockage markers layer datatype in pattern-based mode",
                    "The blockage marker generation and layer mapping is designed in the same way than via geometry mapping, except the option to use mask specific target layers. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "lefdef-dont-produce-labels", &m_lefdef_produce_labels, "Skips label when producing geometry",
                    "If this option is given, no blockage geometry will be produced."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-label-suffix", &m_lefdef_label_suffix, "Specifies the label markers layer suffix in pattern-based mode",
                    "The label marker generation and layer mapping is designed in the same way than via geometry mapping, except the option to use mask specific target layers. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "lefdef-label-datatype", &m_lefdef_label_datatype, "Specifies the label markers layer datatype in pattern-based mode",
                    "The label marker generation and layer mapping is designed in the same way than via geometry mapping, except the option to use mask specific target layers. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the mapping scheme.\n"
                   )
        << tl::arg (group +
                    "--" + m_long_prefix + "lefdef-map", &m_lefdef_map_file, "Specifies to use a layer map file",
                    "Use this option to turn off pattern-based layer mapping and to use an explicit mapping file instead. "
                    "See '--" + m_long_prefix + "lefdef-via-geometry-suffix' for a description of the pattern-based mapping scheme.\n"
                    "\n"
                    "Using a map file is an alternative way to specify layer mapping. With a layer mapping file, the individual target "
                    "layers need to specified individually for different layer/purpose combinations.\n"
                    "\n"
                    "The mapping file is one layer mapping entry per line. Each line is a layer name, followed by a list of purposes (VIA, PIN ...) "
                    "and a layer and datatype number. In addition, 'DIEAREA' can be used to map the design outline to a layer. 'NAME' in place of the "
                    "layer name and using layer/purpose in the purpose column allows mapping labels to specific layers.\n"
                    "\n"
                    "This is an example for a layer map file:\n"
                    "\n"
                    "DIEAREA ALL                       100      0\n"
                    "M1      LEFPIN                    12       0\n"
                    "M1      PIN                       12       2\n"
                    "M1      NET                       12       3\n"
                    "M1      SPNET                     12       4\n"
                    "M1      VIA                       12       5\n"
                    "M1      BLOCKAGE                  12       10\n"
                    "NAME    M1/PIN                    12       10\n"
                    "VIA1    LEFPIN,VIA,PIN,NET,SPNET  13       0\n"
                    "M2      LEFPIN,PIN,NET,SPNET,VIA  14       0\n"
                    "\n"
                    "If a map file is used, only the layers present in the map file are generated. No other layers are produced."
                   )
        << tl::arg (group +
                    "!--" + m_long_prefix + "lefdef-no-lef-macros", &m_lefdef_produce_lef_macros, "Don't produce LEF macro geometry",
                    "This option applies when reading DEF files.\n"
                    "\n"
                    "If this option is present, no geometry will be produced for LEF macros. Instead, a ghost cell with the name of the LEF "
                    "macro will be produced. If this option is not given, the LEF macros will be inserted in to these cells "
                    "unless a FOREIGN specification is present in the LEF macro. If a FOREIGN specification is given, LEF geometry is never "
                    "inserted into a DEF file. Instead ghost cells are created."
                   )
        << tl::arg (group +
                    "!--" + m_long_prefix + "lefdef-no-implicit-lef", &m_lefdef_read_lef_with_def, "Disables reading all LEF files together with DEF files",
                    "This option applies when reading DEF files.\n"
                    "\n"
                    "If this option is given, only the LEF files specified with '--" + m_long_prefix + "lefdef-lef-files' will be read."
                    "\n"
                    "If this option is not present, the DEF reader will look for all files with 'LEF' or related extensions "
                    "in the same place than the DEF file and read these files before the DEF file is read. In addition, it will read the "
                    "LEF files specified with '--" + m_long_prefix + "lefdef-lef-files'."
                   )
        << tl::arg (group +
                    "--" + m_long_prefix + "lefdef-lefs", &m_lefdef_lef_files, "Specifies which additional LEF files to read",
                    "This option applies when reading DEF files.\n"
                    "\n"
                    "Use a comma-separated list of file names here to specify which LEF files to read. "
                    "See also '--" + m_long_prefix + "lefdef-read-lef-with-def' for an option to implicitly read all LEF files in the same "
                    "place than the DEF file.\n"
                    "\n"
                    "Relative paths are resolved based on the location of the DEF file which is read."
                   )
      ;

  }
}

void GenericReaderOptions::set_layer_map (const std::string &lm)
{
  tl::Extractor ex (lm.c_str ());

  int l = 0;
  while (! ex.at_end ()) {
    m_layer_map.map_expr (ex, l);
    ex.test ("//");
    ++l;
  }
}

void GenericReaderOptions::set_read_named_layers (bool f)
{
  m_keep_layer_names = f;
}

void GenericReaderOptions::set_dbu (double dbu)
{
  m_dbu = dbu;
}

void
GenericReaderOptions::configure (db::LoadLayoutOptions &load_options) const
{
  load_options.set_option_by_name ("layer_map", tl::Variant::make_variant (m_layer_map));
  load_options.set_option_by_name ("create_other_layers", m_create_other_layers);
  load_options.set_option_by_name ("text_enabled", m_common_enable_text_objects);
  load_options.set_option_by_name ("properties_enabled", m_common_enable_properties);

  load_options.set_option_by_name ("gds2_box_mode", m_gds2_box_mode);
  load_options.set_option_by_name ("gds2_allow_big_records", m_gds2_allow_big_records);
  load_options.set_option_by_name ("gds2_allow_multi_xy_records", m_gds2_allow_multi_xy_records);

  load_options.set_option_by_name ("oasis_read_all_properties", m_oasis_read_all_properties);
  load_options.set_option_by_name ("oasis_expect_strict_mode", m_oasis_expect_strict_mode ? 1 : 0);

  load_options.set_option_by_name ("cif_layer_map", tl::Variant::make_variant (m_layer_map));
  load_options.set_option_by_name ("cif_create_other_layers", m_create_other_layers);
  load_options.set_option_by_name ("cif_dbu", m_dbu);
  load_options.set_option_by_name ("cif_wire_mode", m_cif_wire_mode);
  load_options.set_option_by_name ("cif_keep_layer_names", m_keep_layer_names);

  load_options.set_option_by_name ("dxf_layer_map", tl::Variant::make_variant (m_layer_map));
  load_options.set_option_by_name ("dxf_create_other_layers", m_create_other_layers);
  load_options.set_option_by_name ("dxf_dbu", m_dbu);
  load_options.set_option_by_name ("dxf_unit", m_dxf_unit);
  load_options.set_option_by_name ("dxf_text_scaling", m_dxf_text_scaling);
  load_options.set_option_by_name ("dxf_polyline_mode", m_dxf_polyline_mode);
  load_options.set_option_by_name ("dxf_circle_points", m_dxf_circle_points);
  load_options.set_option_by_name ("dxf_circle_accuracy", m_dxf_circle_accuracy);
  load_options.set_option_by_name ("dxf_contour_accuracy", m_dxf_contour_accuracy);
  load_options.set_option_by_name ("dxf_render_texts_as_polygons", m_dxf_render_texts_as_polygons);
  load_options.set_option_by_name ("dxf_keep_layer_names", m_keep_layer_names);
  load_options.set_option_by_name ("dxf_keep_other_cells", m_dxf_keep_other_cells);

  load_options.set_option_by_name ("mag_layer_map", tl::Variant::make_variant (m_layer_map));
  load_options.set_option_by_name ("mag_create_other_layers", m_create_other_layers);
  load_options.set_option_by_name ("mag_dbu", m_dbu);
  load_options.set_option_by_name ("mag_lambda", m_magic_lambda);
  load_options.set_option_by_name ("mag_merge", m_magic_merge);
  load_options.set_option_by_name ("mag_keep_layer_names", m_keep_layer_names);
  load_options.set_option_by_name ("mag_library_paths", tl::Variant (m_magic_lib_path.begin (), m_magic_lib_path.end ()));

  load_options.set_option_by_name ("lefdef_config.layer_map", tl::Variant::make_variant (m_layer_map));
  load_options.set_option_by_name ("lefdef_config.create_other_layers", m_create_other_layers);
  load_options.set_option_by_name ("lefdef_config.dbu", m_dbu);
  load_options.set_option_by_name ("lefdef_config.net_property_name", m_lefdef_produce_net_names ? tl::Variant (m_lefdef_net_property_name) : tl::Variant ());
  load_options.set_option_by_name ("lefdef_config.instance_property_name", m_lefdef_produce_inst_names ? tl::Variant (m_lefdef_inst_property_name) : tl::Variant ());
  load_options.set_option_by_name ("lefdef_config.pin_property_name", m_lefdef_produce_pin_names ? tl::Variant (m_lefdef_pin_property_name) : tl::Variant ());
  load_options.set_option_by_name ("lefdef_config.produce_cell_outlines", m_lefdef_produce_cell_outlines);
  load_options.set_option_by_name ("lefdef_config.cell_outline_layer", m_lefdef_cell_outline_layer);
  load_options.set_option_by_name ("lefdef_config.produce_placement_blockages", m_lefdef_produce_placement_blockages);
  load_options.set_option_by_name ("lefdef_config.placement_blockage_layer", m_lefdef_placement_blockage_layer);
  load_options.set_option_by_name ("lefdef_config.produce_regions", m_lefdef_produce_regions);
  load_options.set_option_by_name ("lefdef_config.region_layer", m_lefdef_region_layer);
  load_options.set_option_by_name ("lefdef_config.produce_via_geometry", m_lefdef_produce_via_geometry);
  load_options.set_option_by_name ("lefdef_config.via_geometry_suffix_str", m_lefdef_via_geometry_suffix);
  load_options.set_option_by_name ("lefdef_config.via_geometry_datatype_str", m_lefdef_via_geometry_datatype);
  load_options.set_option_by_name ("lefdef_config.via_cellname_prefix", m_lefdef_via_cellname_prefix);
  load_options.set_option_by_name ("lefdef_config.produce_pins", m_lefdef_produce_pins);
  load_options.set_option_by_name ("lefdef_config.pins_suffix_str", m_lefdef_pins_suffix);
  load_options.set_option_by_name ("lefdef_config.pins_datatype_str", m_lefdef_pins_datatype);
  load_options.set_option_by_name ("lefdef_config.produce_lef_pins", m_lefdef_produce_lef_pins);
  load_options.set_option_by_name ("lefdef_config.lef_pins_suffix_str", m_lefdef_lef_pins_suffix);
  load_options.set_option_by_name ("lefdef_config.lef_pins_datatype_str", m_lefdef_lef_pins_datatype);
  load_options.set_option_by_name ("lefdef_config.produce_obstructions", m_lefdef_produce_obstructions);
  load_options.set_option_by_name ("lefdef_config.obstructions_suffix", m_lefdef_obstruction_suffix);
  load_options.set_option_by_name ("lefdef_config.obstructions_datatype", m_lefdef_obstruction_datatype);
  load_options.set_option_by_name ("lefdef_config.produce_blockages", m_lefdef_produce_blockages);
  load_options.set_option_by_name ("lefdef_config.blockages_suffix", m_lefdef_blockage_suffix);
  load_options.set_option_by_name ("lefdef_config.blockages_datatype", m_lefdef_blockage_datatype);
  load_options.set_option_by_name ("lefdef_config.produce_labels", m_lefdef_produce_labels);
  load_options.set_option_by_name ("lefdef_config.labels_suffix", m_lefdef_label_suffix);
  load_options.set_option_by_name ("lefdef_config.labels_datatype", m_lefdef_label_datatype);
  load_options.set_option_by_name ("lefdef_config.produce_routing", m_lefdef_produce_routing);
  load_options.set_option_by_name ("lefdef_config.routing_suffix_str", m_lefdef_routing_suffix);
  load_options.set_option_by_name ("lefdef_config.routing_datatype_str", m_lefdef_routing_datatype);
  load_options.set_option_by_name ("lefdef_config.produce_special_routing", m_lefdef_produce_special_routing);
  load_options.set_option_by_name ("lefdef_config.special_routing_suffix_str", m_lefdef_special_routing_suffix);
  load_options.set_option_by_name ("lefdef_config.special_routing_datatype_str", m_lefdef_special_routing_datatype);
  load_options.set_option_by_name ("lefdef_config.lef_files", tl::Variant (m_lefdef_lef_files.begin (), m_lefdef_lef_files.end ()));
  load_options.set_option_by_name ("lefdef_config.read_lef_with_def", m_lefdef_read_lef_with_def);
  load_options.set_option_by_name ("lefdef_config.separate_groups", m_lefdef_separate_groups);
  load_options.set_option_by_name ("lefdef_config.map_file", m_lefdef_map_file);
  load_options.set_option_by_name ("lefdef_config.macro_resolution_mode", m_lefdef_produce_lef_macros ? 0 : 2);
}

}

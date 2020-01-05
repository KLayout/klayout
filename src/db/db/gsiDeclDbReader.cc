
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


#include "gsiDecl.h"
#include "dbReader.h"
#include "dbLoadLayoutOptions.h"

namespace gsi
{
  static db::LDPair 
  ldpair_from_lp (const db::LayerProperties &lp)
  {
    return db::LDPair (lp.layer, lp.datatype);
  }

  static bool
  lm_is_mapped (const db::LayerMap *layer_map, const db::LayerProperties &lp)
  {
    return layer_map->logical (lp).first;
  }

  static int 
  lm_logical (const db::LayerMap *layer_map, const db::LayerProperties &lp)
  {
    std::pair<bool, unsigned int> lm = layer_map->logical (lp);
    return lm.first ? int (lm.second) : -1;
  }

  static db::LayerProperties 
  lm_mapping (const db::LayerMap *layer_map, unsigned int l)
  {
    return layer_map->mapping (l);
  }

  static void
  lm_map (db::LayerMap *layer_map, const db::LayerProperties &lp, unsigned int l)
  {
    layer_map->map (lp, l);
  }

  static void
  lm_map_with_target (db::LayerMap *layer_map, const db::LayerProperties &lp, unsigned int l, const db::LayerProperties &t)
  {
    layer_map->map (lp, l, t);
  }

  static void
  lm_map_interval (db::LayerMap *layer_map, const db::LayerProperties &lp1, const db::LayerProperties &lp2, unsigned int l)
  {
    layer_map->map (ldpair_from_lp (lp1), ldpair_from_lp (lp2), l);
  }

  static void
  lm_map_interval_with_target (db::LayerMap *layer_map, const db::LayerProperties &lp1, const db::LayerProperties &lp2, unsigned int l, const db::LayerProperties &t)
  {
    layer_map->map (ldpair_from_lp (lp1), ldpair_from_lp (lp2), l, t);
  }

  static void
  lm_map_string (db::LayerMap *layer_map, std::string &s, unsigned int l)
  {
    layer_map->map_expr (s, l);
  }

  Class<db::LayerMap> decl_LayerMap ("db", "LayerMap",
    gsi::method_ext ("is_mapped?", &lm_is_mapped, 
      "@brief Check, if a given physical layer is mapped\n"
      "@args layer\n"
      "@param layer The physical layer specified with an \\LayerInfo object.\n"
      "@return True, if the layer is mapped."
    ) +
    gsi::method_ext ("logical", &lm_logical, 
      "@brief Returns the logical layer (the layer index in the layout object) for a given physical layer.n"
      "@args layer\n"
      "@param layer The physical layer specified with an \\LayerInfo object.\n"
      "@return The logical layer index or -1 if the layer is not mapped."
    ) +
    gsi::method ("mapping_str", &db::LayerMap::mapping_str, 
      "@brief Returns the mapping string for a given logical layer\n"
      "@args log_layer\n"
      "@param log_layer The logical layer for which the mapping is requested.\n"
      "@return A string describing the mapping."
      "\n"
      "The mapping string is compatible with the string that the \"map\" method accepts.\n"
    ) +
    gsi::method_ext ("mapping", &lm_mapping, 
      "@brief Returns the mapped physical (or target if one is specified) layer for a given logical layer\n"
      "@args log_layer\n"
      "@param log_layer The logical layer for which the mapping is requested.\n"
      "@return A \\LayerInfo object which is the physical layer mapped to the logical layer."
      "\n"
      "In general, there may be more than one physical layer mapped\n"
      "to one logical layer. This method will return a single one of\n"
      "them. It will return the one with the lowest layer and datatype.\n"
    ) +
    gsi::method_ext ("map", &lm_map, 
      "@brief Maps a physical layer to a logical one\n"
      "@args phys_layer,log_layer\n"
      "@param phys_layer The physical layer (a \\LayerInfo object).\n"
      "@param log_layer The logical layer to which the physical layer is mapped.\n"
      "\n"
      "In general, there may be more than one physical layer mapped\n"
      "to one logical layer. This method will add the given physical layer to the mapping for the logical layer.\n"
    ) +
    gsi::method_ext ("map", &lm_map_with_target, 
      "@brief Maps a physical layer to a logical one with a target layer\n"
      "@args phys_layer,log_layer,target_layer\n"
      "@param phys_layer The physical layer (a \\LayerInfo object).\n"
      "@param log_layer The logical layer to which the physical layer is mapped.\n"
      "@param target_layer The properties of the layer that will be created unless it already exists.\n"
      "\n"
      "In general, there may be more than one physical layer mapped\n"
      "to one logical layer. This method will add the given physical layer to the mapping for the logical layer.\n"
      "\n"
      "This method has been added in version 0.20.\n"
    ) +
    gsi::method_ext ("map", &lm_map_interval, 
      "@brief Maps a physical layer interval to a logical one\n"
      "@args pl_start,pl_stop,log_layer\n"
      "@param pl_start The first physical layer (a \\LayerInfo object).\n"
      "@param pl_stop The last physical layer (a \\LayerInfo object).\n"
      "@param log_layer The logical layer to which the physical layers are mapped.\n"
      "\n"
      "This method maps an interval of layers l1..l2 and datatypes d1..d2 to the mapping for the "
      "given logical layer. l1 and d1 are given by the pl_start argument, while l2 and d2 are given by "
      "the pl_stop argument."
    ) +
    gsi::method_ext ("map", &lm_map_interval_with_target, 
      "@brief Maps a physical layer interval to a logical one with a target layer\n"
      "@args pl_start,pl_stop,log_layer\n"
      "@param pl_start The first physical layer (a \\LayerInfo object).\n"
      "@param pl_stop The last physical layer (a \\LayerInfo object).\n"
      "@param log_layer The logical layer to which the physical layers are mapped.\n"
      "@param target_layer The properties of the layer that will be created unless it already exists.\n"
      "\n"
      "This method maps an interval of layers l1..l2 and datatypes d1..d2 to the mapping for the "
      "given logical layer. l1 and d1 are given by the pl_start argument, while l2 and d2 are given by "
      "the pl_stop argument."
      "\n"
      "This method has been added in version 0.20.\n"
    ) +
    gsi::method_ext ("map", &lm_map_string, 
      "@brief Maps a physical layer given by a string to a logical one\n"
      "@args map_expr,log_layer\n"
      "@param map_expr The string describing the physical layer to map.\n"
      "@param log_layer The logical layer to which the physical layers are mapped.\n"
      "\n"
      "The string expression is constructed using the syntax: \n"
      "\"list[/list][;..]\" for layer/datatype pairs. \"list\" is a \n"
      "sequence of numbers, separated by comma values or a range \n"
      "separated by a hyphen. Examples are: \"1/2\", \"1-5/0\", \"1,2,5/0\",\n"
      "\"1/5;5/6\".\n"
      "\n"
      "A target layer can be specified with the \":<target>\" notation where "
      "the target is a valid layer specification string (i.e. \"1/0\").\n"
      "\n"
      "Target mapping has been added in version 0.20.\n"
    ) +
    gsi::method ("clear", &db::LayerMap::clear, 
      "@brief Clears the map\n"
    ) +
    gsi::method ("from_string", &db::LayerMap::from_string_file_format,
      "@brief Creates a layer map from the given string\n"
      "The format of the string is that used in layer mapping files: one mapping entry "
      "per line, comments are allowed using '#' or '//'. The format of each line is that "
      "used in the 'map(string, index)' method.\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
    ) +
    gsi::method ("to_string", &db::LayerMap::to_string_file_format,
      "@brief Converts a layer mapping object to a string\n"
      "This method is the inverse of the \\from_string method.\n"
      "\n"
      "This method has been introduced in version 0.23.\n"
    ),
    "@brief An object representing an arbitrary mapping of physical layers to logical layers\n"
    "\n"
    "\"Physical\" layers are stream layers or other separated layers in a CAD file. \"Logical\" layers "
    "are the layers present in a \\Layout object. Logical layers are represented by an integer index while "
    "physical layers are given by a layer and datatype number or name. A logical layer is created automatically "
    "in the layout on reading if it does not exist yet.\n"
    "\n"
    "The mapping describes an association of a set of physical layers to a set of logical ones, where multiple\n"
    "physical layers can be mapped to a single logical one, which effectively merges the layers.\n"
    "\n"
    "For each logical layer, a target layer can be specified. A target layer is the layer/datatype/name combination\n"
    "as which the logical layer appears in the layout. By using a target layer different from the source layer\n"
    "renaming a layer can be achieved while loading a layout. Another use case for that feature is to assign\n"
    "layer names to GDS layer/datatype combinations which are numerical only.\n"
    "\n"
    "LayerMap objects are used in two ways: as input for the reader (inside a \\LoadLayoutOptions class) and\n"
    "as output from the reader (i.e. Layout::read method). For layer map objects used as input, the layer indexes\n"
    "(logical layers) can be consecutive numbers. They do not need to correspond with real layer indexes from\n"
    "a layout object. When used as output, the layer map's logical layers correspond to the layer indexes inside\n"
    "the layout that the layer map was used upon.\n"
    "\n"
    "This is a sample how to use the LayerMap object. It maps all datatypes of layers 1, 2 and 3 to datatype 0 and\n"
    "assigns the names 'ONE', 'TWO' and 'THREE' to these layout layers:\n"
    "\n"
    "@code"
    "lm = RBA::LayerMap::new\n"
    "lm.map(\"1/0-255 : ONE (1/0)\", 0)\n"
    "lm.map(\"2/0-255 : TWO (2/0)\", 1)\n"
    "lm.map(\"3/0-255 : THREE (3/0)\", 2)\n"
    "\n"
    "# read the layout using the layer map\n"
    "lo = RBA::LoadLayoutOptions::new\n"
    "lo.layer_map.assign(lm)\n"
    "ly = RBA::Layout::new\n"
    "ly.read(\"input.gds\", lo)\n"
    "@/code\n"
    "\n"
    "The LayerMap class has been introduced in version 0.18."
  );

  //  NOTE: the contribution comes from format specific extensions.
  Class<db::LoadLayoutOptions> decl_LoadLayoutOptions ("db", "LoadLayoutOptions",
    gsi::Methods (),
    "@brief Layout reader options\n"
    "\n"
    "This object describes various layer reader options used for loading layouts.\n"
    "\n"
    "This class has been introduced in version 0.18.\n"
  );

  static db::LayerMap
  load_without_options (db::Layout *layout, const std::string &filename)
  {
    tl::InputStream stream (filename);
    db::Reader reader (stream);
    return reader.read (*layout);
  }

  static db::LayerMap
  load_with_options (db::Layout *layout, const std::string &filename, const db::LoadLayoutOptions &options)
  {
    tl::InputStream stream (filename);
    db::Reader reader (stream);
    return reader.read (*layout, options);
  }

  //  extend the layout class by two reader methods
  static
  gsi::ClassExt<db::Layout> layout_reader_decl (
    gsi::method_ext ("read", &load_without_options,
      "@brief Load the layout from the given file\n"
      "@args filename\n"
      "The format of the file is determined automatically and automatic unzipping is provided. "
      "No particular options can be specified.\n"
      "@param filename The name of the file to load.\n"
      "@return A layer map that contains the mapping used by the reader including the layers that have been created."
      "\n"
      "This method has been added in version 0.18."
    ) +
    gsi::method_ext ("read", &load_with_options,
      "@brief Load the layout from the given file with options\n"
      "@args filename,options\n"
      "The format of the file is determined automatically and automatic unzipping is provided. "
      "In this version, some reader options can be specified. "
      "@param filename The name of the file to load.\n"
      "@param options The options object specifying further options for the reader.\n"
      "@return A layer map that contains the mapping used by the reader including the layers that have been created."
      "\n"
      "This method has been added in version 0.18."
    ),
    ""
  );

}


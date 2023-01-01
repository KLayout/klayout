
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
    return layer_map->is_mapped (lp);
  }

  static int 
  lm_logical (const db::LayerMap *layer_map, const db::LayerProperties &lp)
  {
    std::set<unsigned int> lm = layer_map->logical (lp);
    return lm.empty () ? -1 : int (*lm.begin ());
  }

  static std::set<unsigned int>
  lm_logicals (const db::LayerMap *layer_map, const db::LayerProperties &lp)
  {
    return layer_map->logical (lp);
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
  lm_map_string (db::LayerMap *layer_map, std::string &s, int l)
  {
    if (l < 0) {
      layer_map->map_expr (s, layer_map->next_index ());
    } else {
      layer_map->map_expr (s, (unsigned int) l);
    }
  }

  static void
  lm_mmap (db::LayerMap *layer_map, const db::LayerProperties &lp, unsigned int l)
  {
    layer_map->mmap (lp, l);
  }

  static void
  lm_mmap_with_target (db::LayerMap *layer_map, const db::LayerProperties &lp, unsigned int l, const db::LayerProperties &t)
  {
    layer_map->mmap (lp, l, t);
  }

  static void
  lm_mmap_interval (db::LayerMap *layer_map, const db::LayerProperties &lp1, const db::LayerProperties &lp2, unsigned int l)
  {
    layer_map->mmap (ldpair_from_lp (lp1), ldpair_from_lp (lp2), l);
  }

  static void
  lm_mmap_interval_with_target (db::LayerMap *layer_map, const db::LayerProperties &lp1, const db::LayerProperties &lp2, unsigned int l, const db::LayerProperties &t)
  {
    layer_map->mmap (ldpair_from_lp (lp1), ldpair_from_lp (lp2), l, t);
  }

  static void
  lm_mmap_string (db::LayerMap *layer_map, std::string &s, int l)
  {
    if (l < 0) {
      layer_map->mmap_expr (s, layer_map->next_index ());
    } else {
      layer_map->mmap_expr (s, (unsigned int) l);
    }
  }

  static void
  lm_unmap (db::LayerMap *layer_map, const db::LayerProperties &lp)
  {
    layer_map->unmap (lp);
  }

  static void
  lm_unmap_interval (db::LayerMap *layer_map, const db::LayerProperties &lp1, const db::LayerProperties &lp2)
  {
    layer_map->unmap (ldpair_from_lp (lp1), ldpair_from_lp (lp2));
  }

  static void
  lm_unmap_string (db::LayerMap *layer_map, std::string &s)
  {
    layer_map->unmap_expr (s);
  }

  Class<db::LayerMap> decl_LayerMap ("db", "LayerMap",
    gsi::method_ext ("is_mapped?", &lm_is_mapped, gsi::arg ("layer"),
      "@brief Check, if a given physical layer is mapped\n"
      "@param layer The physical layer specified with an \\LayerInfo object.\n"
      "@return True, if the layer is mapped."
    ) +
    gsi::method_ext ("#logical", &lm_logical, gsi::arg ("layer"),
      "@brief Returns the logical layer (the layer index in the layout object) for a given physical layer.n"
      "@param layer The physical layer specified with an \\LayerInfo object.\n"
      "@return The logical layer index or -1 if the layer is not mapped."
      "\n"
      "This method is deprecated with version 0.27 as in this version, layers can be mapped to multiple targets which "
      "this method can't capture. Use \\logicals instead.\n"
    ) +
    gsi::method_ext ("logicals", &lm_logicals, gsi::arg ("layer"),
      "@brief Returns the logical layers for a given physical layer.n"
      "@param layer The physical layer specified with an \\LayerInfo object.\n"
      "@return This list of logical layers this physical layer as mapped to or empty if there is no mapping."
      "\n"
      "This method has been introduced in version 0.27.\n"
    ) +
    gsi::method ("mapping_str", &db::LayerMap::mapping_str, gsi::arg ("log_layer"),
      "@brief Returns the mapping string for a given logical layer\n"
      "@param log_layer The logical layer for which the mapping is requested.\n"
      "@return A string describing the mapping."
      "\n"
      "The mapping string is compatible with the string that the \"map\" method accepts.\n"
    ) +
    gsi::method_ext ("mapping", &lm_mapping, gsi::arg ("log_layer"),
      "@brief Returns the mapped physical (or target if one is specified) layer for a given logical layer\n"
      "@param log_layer The logical layer for which the mapping is requested.\n"
      "@return A \\LayerInfo object which is the physical layer mapped to the logical layer."
      "\n"
      "In general, there may be more than one physical layer mapped\n"
      "to one logical layer. This method will return a single one of\n"
      "them. It will return the one with the lowest layer and datatype.\n"
    ) +
    gsi::method_ext ("map", &lm_map, gsi::arg ("phys_layer"), gsi::arg ("log_layer"),
      "@brief Maps a physical layer to a logical one\n"
      "@param phys_layer The physical layer (a \\LayerInfo object).\n"
      "@param log_layer The logical layer to which the physical layer is mapped.\n"
      "\n"
      "In general, there may be more than one physical layer mapped\n"
      "to one logical layer. This method will add the given physical layer to the mapping for the logical layer.\n"
    ) +
    gsi::method_ext ("map", &lm_map_with_target, gsi::arg ("phys_layer"), gsi::arg ("log_layer"), gsi::arg ("target_layer"),
      "@brief Maps a physical layer to a logical one with a target layer\n"
      "@param phys_layer The physical layer (a \\LayerInfo object).\n"
      "@param log_layer The logical layer to which the physical layer is mapped.\n"
      "@param target_layer The properties of the layer that will be created unless it already exists.\n"
      "\n"
      "In general, there may be more than one physical layer mapped\n"
      "to one logical layer. This method will add the given physical layer to the mapping for the logical layer.\n"
      "\n"
      "This method has been added in version 0.20.\n"
    ) +
    gsi::method_ext ("map", &lm_map_interval, gsi::arg ("pl_start"), gsi::arg ("pl_stop"), gsi::arg ("log_layer"),
      "@brief Maps a physical layer interval to a logical one\n"
      "@param pl_start The first physical layer (a \\LayerInfo object).\n"
      "@param pl_stop The last physical layer (a \\LayerInfo object).\n"
      "@param log_layer The logical layer to which the physical layers are mapped.\n"
      "\n"
      "This method maps an interval of layers l1..l2 and datatypes d1..d2 to the mapping for the "
      "given logical layer. l1 and d1 are given by the pl_start argument, while l2 and d2 are given by "
      "the pl_stop argument."
    ) +
    gsi::method_ext ("map", &lm_map_interval_with_target, gsi::arg ("pl_start"), gsi::arg ("pl_stop"), gsi::arg ("log_layer"), gsi::arg ("layer_properties"),
      "@brief Maps a physical layer interval to a logical one with a target layer\n"
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
    gsi::method_ext ("map", &lm_map_string, gsi::arg ("map_expr"), gsi::arg ("log_layer", -1),
      "@brief Maps a physical layer given by a string to a logical one\n"
      "@param map_expr The string describing the physical layer to map.\n"
      "@param log_layer The logical layer to which the physical layers are mapped.\n"
      "\n"
      "The string expression is constructed using the syntax: \n"
      "\"list[/list][;..]\" for layer/datatype pairs. \"list\" is a \n"
      "sequence of numbers, separated by comma values or a range \n"
      "separated by a hyphen. Examples are: \"1/2\", \"1-5/0\", \"1,2,5/0\",\n"
      "\"1/5;5/6\".\n"
      "\n"
      "layer/datatype wildcards can be specified with \"*\". When \"*\" is used\n"
      "for the upper limit, it is equivalent to \"all layer above\". When used\n"
      "alone, it is equivalent to \"all layers\". Examples: \"1 / *\", \"* / 10-*\"\n"
      "\n"
      "Named layers are specified simply by specifying the name, if\n"
      "necessary in single or double quotes (if the name begins with a digit or\n"
      "contains non-word characters). layer/datatype and name descriptions can\n"
      "be mixed, i.e. \"AA;1/5\" (meaning: name \"AA\" or layer 1/datatype 5).\n"
      "\n"
      "A target layer can be specified with the \":<target>\" notation, where\n"
      "target is a valid string for a LayerProperties() object.\n"
      "\n"
      "A target can include relative layer/datatype specifications and wildcards.\n"
      "For example, \"1-10/0: *+1/0\" will add 1 to the original layer number.\n"
      "\"1-10/0-50: * / *\" will use the original layers.\n"
      "\n"
      "If the logical layer is negative or omitted, the method will select the next available one.\n"
      "\n"
      "Target mapping has been added in version 0.20. The logical layer is optional since version 0.28.\n"
    ) +
    gsi::method_ext ("mmap", &lm_mmap, gsi::arg ("phys_layer"), gsi::arg ("log_layer"),
      "@brief Maps a physical layer to a logical one and adds to existing mappings\n"
      "\n"
      "This method acts like the corresponding 'map' method, but adds the logical layer to the receivers of the "
      "given physical one. Hence this method implements 1:n mapping capabilities.\n"
      "For backward compatibility, 'map' still substitutes mapping.\n"
      "\n"
      "Multi-mapping has been added in version 0.27.\n"
    ) +
    gsi::method_ext ("mmap", &lm_mmap_with_target, gsi::arg ("phys_layer"), gsi::arg ("log_layer"), gsi::arg ("target_layer"),
      "@brief Maps a physical layer to a logical one, adds to existing mappings and specifies a target layer\n"
      "\n"
      "This method acts like the corresponding 'map' method, but adds the logical layer to the receivers of the "
      "given physical one. Hence this method implements 1:n mapping capabilities.\n"
      "For backward compatibility, 'map' still substitutes mapping.\n"
      "\n"
      "Multi-mapping has been added in version 0.27.\n"
    ) +
    gsi::method_ext ("mmap", &lm_mmap_interval, gsi::arg ("pl_start"), gsi::arg ("pl_stop"), gsi::arg ("log_layer"),
      "@brief Maps a physical layer from the given interval to a logical one and adds to existing mappings\n"
      "\n"
      "This method acts like the corresponding 'map' method, but adds the logical layer to the receivers of the "
      "given physical one. Hence this method implements 1:n mapping capabilities.\n"
      "For backward compatibility, 'map' still substitutes mapping.\n"
      "\n"
      "Multi-mapping has been added in version 0.27.\n"
    ) +
    gsi::method_ext ("mmap", &lm_mmap_interval_with_target, gsi::arg ("pl_start"), gsi::arg ("pl_stop"), gsi::arg ("log_layer"), gsi::arg ("layer_properties"),
      "@brief Maps a physical layer from the given interval to a logical one, adds to existing mappings and specifies a target layer\n"
      "\n"
      "This method acts like the corresponding 'map' method, but adds the logical layer to the receivers of the "
      "given physical one. Hence this method implements 1:n mapping capabilities.\n"
      "For backward compatibility, 'map' still substitutes mapping.\n"
      "\n"
      "Multi-mapping has been added in version 0.27.\n"
    ) +
    gsi::method_ext ("mmap", &lm_mmap_string, gsi::arg ("map_expr"), gsi::arg ("log_layer", -1),
      "@brief Maps a physical layer given by an expression to a logical one and adds to existing mappings\n"
      "\n"
      "This method acts like the corresponding 'map' method, but adds the logical layer to the receivers of the "
      "given physical one. Hence this method implements 1:n mapping capabilities.\n"
      "For backward compatibility, 'map' still substitutes mapping.\n"
      "\n"
      "If the logical layer is negative or omitted, the method will select the next available one.\n"
      "\n"
      "Multi-mapping has been added in version 0.27. The logical layer is optional since version 0.28.\n"
    ) +
    gsi::method_ext ("unmap", &lm_unmap, gsi::arg ("phys_layer"),
      "@brief Unmaps the given layer\n"
      "Unmapping will remove the specific layer from the mapping. This method allows generating "
      "'mapping holes' by first mapping a range and then unmapping parts of it.\n"
      "\n"
      "This method has been introduced in version 0.27."
    ) +
    gsi::method_ext ("unmap", &lm_unmap_interval, gsi::arg ("pl_start"), gsi::arg ("pl_stop"),
      "@brief Unmaps the layers from the given interval\n"
      "This method has been introduced in version 0.27."
    ) +
    gsi::method_ext ("unmap", &lm_unmap_string, gsi::arg ("expr"),
      "@brief Unmaps the layers from the given expression\n"
      "This method has been introduced in version 0.27."
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
    "lm.map(\"1/0-255 : ONE (1/0)\")\n"
    "lm.map(\"2/0-255 : TWO (2/0)\")\n"
    "lm.map(\"3/0-255 : THREE (3/0)\")\n"
    "\n"
    "# read the layout using the layer map\n"
    "lo = RBA::LoadLayoutOptions::new\n"
    "lo.layer_map.assign(lm)\n"
    "ly = RBA::Layout::new\n"
    "ly.read(\"input.gds\", lo)\n"
    "@/code\n"
    "\n"
    "1:n mapping is supported: a physical layer can be mapped to multiple logical layers using 'mmap' instead of 'map'. When using this variant, "
    "mapping acts additive.\n"
    "The following example will map layer 1, datatypes 0 to 255 to logical layer 0, and layer 1, datatype 17 to logical layers "
    "0 plus 1:"
    "\n"
    "@code"
    "lm = RBA::LayerMap::new\n"
    "lm.map(\"1/0-255\", 0)   # (can be 'mmap' too)\n"
    "lm.mmap(\"1/17\", 1)\n"
    "@/code\n"
    "\n"
    "'unmapping' allows removing a mapping. This allows creating 'holes' in mapping ranges. The following example maps "
    "layer 1, datatypes 0 to 16 and 18 to 255 to logical layer 0:"
    "\n"
    "@code"
    "lm = RBA::LayerMap::new\n"
    "lm.map(\"1/0-255\", 0)\n"
    "lm.unmap(\"1/17\")\n"
    "@/code\n"
    "\n"
    "The LayerMap class has been introduced in version 0.18. Target layer have been introduced in version 0.20. "
    "1:n mapping and unmapping has been introduced in version 0.27."
  );

  //  NOTE: the contribution comes from format specific extensions.
  Class<db::LoadLayoutOptions> decl_LoadLayoutOptions ("db", "LoadLayoutOptions",
    gsi::method ("warn_level=", &db::LoadLayoutOptions::set_warn_level, gsi::arg ("level"),
      "@brief Sets the warning level.\n"
      "The warning level is a reader-specific setting which enables or disables warnings\n"
      "on specific levels. Level 0 is always \"warnings off\". The default level is 1\n"
      "which means \"reasonable warnings emitted\".\n"
      "\n"
      "This attribute has been added in version 0.28."
    ) +
    gsi::method ("warn_level", &db::LoadLayoutOptions::warn_level,
      "@brief Sets the warning level.\n"
      "See \\warn_level= for details about this attribute.\n"
      "\n"
      "This attribute has been added in version 0.28."
    ),
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
    gsi::method_ext ("read", &load_without_options, gsi::arg ("filename"),
      "@brief Load the layout from the given file\n"
      "The format of the file is determined automatically and automatic unzipping is provided. "
      "No particular options can be specified.\n"
      "@param filename The name of the file to load.\n"
      "@return A layer map that contains the mapping used by the reader including the layers that have been created."
      "\n"
      "This method has been added in version 0.18."
    ) +
    gsi::method_ext ("read", &load_with_options, gsi::arg ("filename"), gsi::arg ("options"),
      "@brief Load the layout from the given file with options\n"
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


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
#include "dbLayerMapping.h"
#include "dbLayout.h"

#include <memory>

namespace gsi
{

Class<db::LayerMapping> decl_LayerMapping ("db", "LayerMapping",
  gsi::method ("create", &db::LayerMapping::create, gsi::arg ("layout_a"), gsi::arg ("layout_b"),
    "@brief Initialize the layer mapping from two layouts\n"
    "\n"
    "@param layout_a The target layout\n"
    "@param layout_b The source layout\n"
    "\n"
    "The layer mapping is created by looking up each layer of layout_b in layout_a. "
    "All layers with matching specifications (\\LayerInfo) are mapped. Layouts without a layer/datatype/name specification "
    "will not be mapped.\n"
    "\\create_full is a version of this method which creates new layers in layout_a if no corresponding layer is found.\n"
  ) +
  gsi::method ("create_full", &db::LayerMapping::create_full, gsi::arg ("layout_a"), gsi::arg ("layout_b"),
    "@brief Initialize the layer mapping from two layouts\n"
    "\n"
    "@param layout_a The target layout\n"
    "@param layout_b The source layout\n"
    "@return A list of layers created\n"
    "\n"
    "The layer mapping is created by looking up each layer of layout_b in layout_a. "
    "All layers with matching specifications (\\LayerInfo) are mapped. Layouts without a layer/datatype/name specification "
    "will not be mapped.\n"
    "Layers with a valid specification which are not found in layout_a are created there.\n"
  ) +
  gsi::method ("clear", &db::LayerMapping::clear, 
    "@brief Clears the mapping.\n"
  ) +
  gsi::method ("map", &db::LayerMapping::map, gsi::arg ("layer_index_b"), gsi::arg ("layer_index_a"),
    "@brief Explicitly specify a mapping.\n"
    "\n"
    "\n"
    "@param layer_index_b The index of the layer in layout B (the \"source\")\n"
    "@param layer_index_a The index of the layer in layout A (the \"target\")\n"
    "\n"
    "Beside using the mapping generator algorithms provided through \\create and \\create_full, "
    "it is possible to explicitly specify layer mappings using this method.\n"
  ) +
  gsi::method ("table", &db::LayerMapping::table,
    "@brief Returns the mapping table.\n"
    "\n"
    "The mapping table is a dictionary where the keys are source layout layer indexes and the values are the target layout layer indexes.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("has_mapping?", &db::LayerMapping::has_mapping, gsi::arg ("layer_index_b"),
    "@brief Determine if a layer in layout_b has a mapping to a layout_a layer.\n"
    "\n"
    "\n"
    "@param layer_index_b The index of the layer in layout_b whose mapping is requested.\n"
    "@return true, if the layer has a mapping\n"
  ) +
  gsi::method ("layer_mapping", &db::LayerMapping::layer_mapping, gsi::arg ("layer_index_b"),
    "@brief Determine layer mapping of a layout_b layer to the corresponding layout_a layer.\n"
    "\n"
    "\n"
    "@param layer_index_b The index of the layer in layout_b whose mapping is requested.\n"
    "@return The corresponding layer in layout_a.\n"
  ),
  "@brief A layer mapping (source to target layout)\n"
  "\n"
  "A layer mapping is an association of layers in two layouts forming pairs of layers, i.e. "
  "one layer corresponds to another layer in the other layout. The LayerMapping object describes "
  "the mapping of layers of a source layout A to a target layout B.\n"
  "\n"
  "A layer mapping can be set up manually or using the methods \\create or \\create_full.\n"
  "\n"
  "@code\n"
  "lm = RBA::LayerMapping::new\n"
  "# explicit:\n"
  "lm.map(2, 1)  # map layer index 2 of source to 1 of target\n"
  "lm.map(7, 3)  # map layer index 7 of source to 3 of target\n"
  "...\n"
  "# or employing the specification identity:\n"
  "lm.create(target_layout, source_layout)\n"
  "# plus creating layers which don't exist in the target layout yet:\n"
  "new_layers = lm.create_full(target_layout, source_layout)\n"
  "@/code\n"
  "\n"
  "A layer might not be mapped to another layer which basically means that there is no corresponding layer.\n"
  "Such layers will be ignored in operations using the layer mapping. Use \\create_full to ensure all layers\n"
  "of the source layout are mapped.\n"
  "\n"
  "LayerMapping objects play a role mainly in the hierarchical copy or move operations of \\Layout. "
  "However, use is not restricted to these applications.\n"
  "\n"
  "This class has been introduced in version 0.23."
);

}


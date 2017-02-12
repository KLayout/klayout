
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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
#include "dbCellMapping.h"
#include "dbLayout.h"

#include <memory>

namespace gsi
{

Class<db::CellMapping> decl_CellMapping ("CellMapping", 
  gsi::method ("for_single_cell", &db::CellMapping::create_single_mapping, 
    "@brief Initialize the cell mapping for top-level identity\n"
    "\n"
    "@args layout_a, cell_index_a, layout_b, cell_index_b\n"
    "@param layout_a The target layout.\n"
    "@param cell_index_a The index of the target cell.\n"
    "@param layout_b The source layout.\n"
    "@param cell_index_b The index of the source cell.\n"
    "\n"
    "The cell mapping is created for cell_b to cell_a in the respective layouts. "
    "This method clears the mapping and creates one for the single cell pair. "
    "In addition, this method completes the mapping by adding all the child cells "
    "of cell_b to layout_a and creating the proper instances. "
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("for_single_cell_full", &db::CellMapping::create_single_mapping_full, 
    "@brief Initialize the cell mapping for top-level identity\n"
    "\n"
    "@args layout_a, cell_index_a, layout_b, cell_index_b\n"
    "@param layout_a The target layout.\n"
    "@param cell_index_a The index of the target cell.\n"
    "@param layout_b The source layout.\n"
    "@param cell_index_b The index of the source cell.\n"
    "\n"
    "The cell mapping is created for cell_b to cell_a in the respective layouts. "
    "This method clears the mapping and creates one for the single cell pair. "
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("from_geometry_full", &db::CellMapping::create_from_geometry_full, 
    "@brief Initialize the cell mapping using the geometrical identity in full mapping mode\n"
    "\n"
    "@args layout_a, cell_index_a, layout_b, cell_index_b\n"
    "@param layout_a The target layout.\n"
    "@param cell_index_a The index of the target starting cell.\n"
    "@param layout_b The source layout.\n"
    "@param cell_index_b The index of the source starting cell.\n"
    "@return A list of indexes of cells created.\n"
    "\n"
    "The cell mapping is created for cells below cell_a and cell_b in the respective layouts. "
    "This method employs geometrical identity to derive mappings for "
    "the child cells of the starting cell in layout A and B.\n"
    "If the geometrical identity is ambiguous, the algorithm will make an arbitrary choice.\n"
    "\n"
    "Full mapping means that cells which are not found in the target layout A are created there plus their "
    "corresponding instances are created as well. "
    "The returned list will contain the indexes of all cells created for that reason.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("from_geometry", &db::CellMapping::create_from_geometry, 
    "@brief Initialize the cell mapping using the geometrical identity\n"
    "\n"
    "@args layout_a, cell_index_a, layout_b, cell_index_b\n"
    "@param layout_a The target layout.\n"
    "@param cell_index_a The index of the target starting cell.\n"
    "@param layout_b The source layout.\n"
    "@param cell_index_b The index of the source starting cell.\n"
    "\n"
    "The cell mapping is created for cells below cell_a and cell_b in the respective layouts. "
    "This method employs geometrical identity to derive mappings for "
    "the child cells of the starting cell in layout A and B.\n"
    "If the geometrical identity is ambiguous, the algorithm will make an arbitrary choice.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("from_names", &db::CellMapping::create_from_names, 
    "@brief Initialize the cell mapping using the name identity\n"
    "\n"
    "@args layout_a, cell_index_a, layout_b, cell_index_b\n"
    "@param layout_a The target layout.\n"
    "@param cell_index_a The index of the target starting cell.\n"
    "@param layout_b The source layout.\n"
    "@param cell_index_b The index of the source starting cell.\n"
    "\n"
    "The cell mapping is created for cells below cell_a and cell_b in the respective layouts.\n"
    "This method employs name identity to derive mappings for "
    "the child cells of the starting cell in layout A and B.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("from_names_full", &db::CellMapping::create_from_names_full, 
    "@brief Initialize the cell mapping using the name identity in full mapping mode\n"
    "\n"
    "@args layout_a, cell_index_a, layout_b, cell_index_b\n"
    "@param layout_a The target layout.\n"
    "@param cell_index_a The index of the target starting cell.\n"
    "@param layout_b The source layout.\n"
    "@param cell_index_b The index of the source starting cell.\n"
    "@return A list of indexes of cells created.\n"
    "\n"
    "The cell mapping is created for cells below cell_a and cell_b in the respective layouts.\n"
    "This method employs name identity to derive mappings for "
    "the child cells of the starting cell in layout A and B.\n"
    "\n"
    "Full mapping means that cells which are not found in the target layout A are created there plus their "
    "corresponding instances are created as well. "
    "The returned list will contain the indexes of all cells created for that reason.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("clear", &db::CellMapping::clear, 
    "@brief Clears the mapping.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("map", &db::CellMapping::map, 
    "@brief Explicitly specify a mapping.\n"
    "\n"
    "@args cell_index_b, cell_index_a\n"
    "\n"
    "@param cell_index_b The index of the cell in layout B (the \"source\")\n"
    "@param cell_index_a The index of the cell in layout A (the \"target\")\n"
    "\n"
    "Beside using the mapping generator algorithms provided through \\from_names and \\from_geometry, "
    "it is possible to explicitly specify cell mappings using this method.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("has_mapping?", &db::CellMapping::has_mapping, 
    "@brief Determine if a cell of layout_b has a mapping to a layout_a cell.\n"
    "\n"
    "@args cell_index_b\n"
    "\n"
    "@param cell_index_b The index of the cell in layout_b whose mapping is requested.\n"
    "@return true, if the cell has a mapping\n"
  ) +
  gsi::method ("cell_mapping", &db::CellMapping::cell_mapping, 
    "@brief Determine cell mapping of a layout_b cell to the corresponding layout_a cell.\n"
    "\n"
    "@args cell_index_b\n"
    "\n"
    "@param cell_index_b The index of the cell in layout_b whose mapping is requested.\n"
    "@return The cell index in layout_a.\n"
  ),
  "@brief A cell mapping (source to target layout)\n"
  "\n"
  "A cell mapping is an association of cells in two layouts forming pairs of cells, i.e. "
  "one cell corresponds to another cell in the other layout. The CellMapping object describes "
  "the mapping of cells of a source layout B to a target layout A. The cell mapping object "
  "is basically a table associating a cell in layout B with a cell in layout A.\n"
  "\n"
  "The mapping object is used to create and hold that table. There are three basic modes in which "
  "a table can be generated:\n"
  "\n"
  "@ul\n"
  "  @li Top-level identity @/li\n"
  "  @li Geometrical identity @/li\n"
  "  @li Name identity @/li\n"
  "@/ul\n"
  "\n"
  "Top-level identity means that only one cell (the top cell) is regarded identical. All child cells are "
  "not considered identical. In full mode (see below), this will create a new, identical cell tree "
  "below the top cell in layout A.\n"
  "\n"
  "Geometrical identity is defined by the "
  "exact identity of the set of expanded instances in each starting cell. Therefore, when "
  "a cell is mapped to another cell, shapes can be transferred from one cell to another "
  "while effectively rendering the same flat geometry (in the context of the given starting cells). "
  "Location identity is basically the safest way to map cells from one hierarchy into another, because it preserves the flat shape "
  "geometry. However in some cases the algorithm may find multiple mapping candidates. In that "
  "case it will make a guess about what mapping to choose.\n"
  "\n"
  "Name identity means that cells are identified by their names - for a source cell in layer B, a target "
  "cell with the same name is looked up in the target layout A and a mapping is created if a cell with the same name "
  "is found. However, name identity does not mean that the cells are actually equivalent "
  "because they may be placed differently. Hence, cell mapping by name is not a good choice when "
  "it is important to preserve the shape geometry of a layer.\n"
  "\n"
  "A cell might not be mapped to another cell which basically means that there is no corresponding cell. "
  "In this case, flattening to the next mapped cell is an option to transfer geometries despite the "
  "missing mapping. You can enforce a mapping by using the mapping generator methods in 'full' mode, i.e. "
  "\\from_names_full or \\from_geometry_full. These versions will create new cells and their corresponding "
  "instances in the target layout if no suitable target cell is found.\n"
  "\n"
  "CellMapping objects play a role mainly in the hierarchical copy or move operations of \\Layout. "
  "However, use is not restricted to these applications.\n"
);

}


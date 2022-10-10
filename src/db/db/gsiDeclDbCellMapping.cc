
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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
#include "dbLayoutUtils.h"
#include "dbLayout.h"

#include <memory>

namespace gsi
{

static db::cell_index_type drop_cell_const ()
{
  return db::DropCell;
}

Class<db::CellMapping> decl_CellMapping ("db", "CellMapping",
  gsi::method ("DropCell", &drop_cell_const,
    "@brief A constant indicating the reques to drop a cell\n"
    "\n"
    "If used as a pseudo-target for the cell mapping, this index indicates "
    "that the cell shall be dropped rather than created on the target side "
    "or skipped by flattening. Instead, all shapes of this cell are discarded "
    "and it's children are not translated unless explicitly requested or "
    "if required are children for other cells.\n"
    "\n"
    "This constant has been introduced in version 0.25."
  ) +
  gsi::method ("for_single_cell", &db::CellMapping::create_single_mapping, gsi::arg ("layout_a"), gsi::arg ("cell_index_a"), gsi::arg ("layout_b"), gsi::arg ("cell_index_b"),
    "@brief Initializes the cell mapping for top-level identity\n"
    "\n"
    "@param layout_a The target layout.\n"
    "@param cell_index_a The index of the target cell.\n"
    "@param layout_b The source layout.\n"
    "@param cell_index_b The index of the source cell.\n"
    "\n"
    "The cell mapping is created for cell_b to cell_a in the respective layouts. "
    "This method clears the mapping and creates one for the single cell pair. "
    "If used for \\Cell#copy_tree or \\Cell#move_tree, this cell mapping will essentially "
    "flatten the cell.\n"
    "\n"
    "This method is equivalent to \\clear, followed by \\map(cell_index_a, cell_index_b).\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("for_single_cell_full", &db::CellMapping::create_single_mapping_full, gsi::arg ("layout_a"), gsi::arg ("cell_index_a"), gsi::arg ("layout_b"), gsi::arg ("cell_index_b"),
    "@brief Initializes the cell mapping for top-level identity\n"
    "\n"
    "@param layout_a The target layout.\n"
    "@param cell_index_a The index of the target cell.\n"
    "@param layout_b The source layout.\n"
    "@param cell_index_b The index of the source cell.\n"
    "\n"
    "The cell mapping is created for cell_b to cell_a in the respective layouts. "
    "This method clears the mapping and creates one for the single cell pair. "
    "In addition and in contrast to \\for_single_cell, this method completes the mapping by adding all the child cells "
    "of cell_b to layout_a and creating the proper instances.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("for_multi_cells", &db::CellMapping::create_multi_mapping, gsi::arg ("layout_a"), gsi::arg ("cell_indexes_a"), gsi::arg ("layout_b"), gsi::arg ("cell_indexes_b"),
    "@brief Initializes the cell mapping for top-level identity\n"
    "\n"
    "@param layout_a The target layout.\n"
    "@param cell_indexes_a A list of cell indexes for the target cells.\n"
    "@param layout_b The source layout.\n"
    "@param cell_indexes_b A list of cell indexes for the source cells (same number of indexes than \\cell_indexes_a).\n"
    "\n"
    "The cell mapping is created for cells from cell_indexes_b to cell from cell_indexes_a in the respective layouts. "
    "This method clears the mapping and creates one for each cell pair from cell_indexes_b vs. cell_indexes_a. "
    "If used for \\Layout#copy_tree_shapes or \\Layout#move_tree_shapes, this cell mapping will essentially "
    "flatten the source cells in the target layout.\n"
    "\n"
    "This method is equivalent to \\clear, followed by \\map(cell_index_a, cell_index_b) for each cell pair.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::method ("for_multi_cells_full", &db::CellMapping::create_multi_mapping_full, gsi::arg ("layout_a"), gsi::arg ("cell_indexes_a"), gsi::arg ("layout_b"), gsi::arg ("cell_indexes_b"),
    "@brief Initializes the cell mapping for top-level identity\n"
    "\n"
    "@param layout_a The target layout.\n"
    "@param cell_indexes_a A list of cell indexes for the target cells.\n"
    "@param layout_b The source layout.\n"
    "@param cell_indexes_b A list of cell indexes for the source cells (same number of indexes than \\cell_indexes_a).\n"
    "\n"
    "The cell mapping is created for cells from cell_indexes_b to cell from cell_indexes_a in the respective layouts. "
    "This method clears the mapping and creates one for each cell pair from cell_indexes_b vs. cell_indexes_a. "
    "In addition and in contrast to \\for_multi_cells, this method completes the mapping by adding all the child cells "
    "of all cells in cell_indexes_b to layout_a and creating the proper instances.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::method ("from_geometry_full", &db::CellMapping::create_from_geometry_full, gsi::arg ("layout_a"), gsi::arg ("cell_index_a"), gsi::arg ("layout_b"), gsi::arg ("cell_index_b"),
    "@brief Initializes the cell mapping using the geometrical identity in full mapping mode\n"
    "\n"
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
  gsi::method ("from_geometry", &db::CellMapping::create_from_geometry, gsi::arg ("layout_a"), gsi::arg ("cell_index_a"), gsi::arg ("layout_b"), gsi::arg ("cell_index_b"),
    "@brief Initializes the cell mapping using the geometrical identity\n"
    "\n"
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
  gsi::method ("from_names", &db::CellMapping::create_from_names, gsi::arg ("layout_a"), gsi::arg ("cell_index_a"), gsi::arg ("layout_b"), gsi::arg ("cell_index_b"),
    "@brief Initializes the cell mapping using the name identity\n"
    "\n"
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
  gsi::method ("from_names_full", &db::CellMapping::create_from_names_full, gsi::arg ("layout_a"), gsi::arg ("cell_index_a"), gsi::arg ("layout_b"), gsi::arg ("cell_index_b"),
    "@brief Initializes the cell mapping using the name identity in full mapping mode\n"
    "\n"
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
  gsi::method ("table", &db::CellMapping::table,
    "@brief Returns the mapping table.\n"
    "\n"
    "The mapping table is a dictionary where the keys are source layout cell indexes and the values are the target layout cell indexes.\n"
    "Note that the target cell index can be \\DropCell to indicate that a cell is supposed to be dropped.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("map", &db::CellMapping::map, gsi::arg ("cell_index_b"), gsi::arg ("cell_index_a"),
    "@brief Explicitly specifies a mapping.\n"
    "\n"
    "\n"
    "@param cell_index_b The index of the cell in layout B (the \"source\")\n"
    "@param cell_index_a The index of the cell in layout A (the \"target\") - this index can be \\DropCell\n"
    "\n"
    "Beside using the mapping generator algorithms provided through \\from_names and \\from_geometry, "
    "it is possible to explicitly specify cell mappings using this method.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method ("has_mapping?", &db::CellMapping::has_mapping, gsi::arg ("cell_index_b"),
    "@brief Returns as value indicating whether a cell of layout_b has a mapping to a layout_a cell.\n"
    "\n"
    "\n"
    "@param cell_index_b The index of the cell in layout_b whose mapping is requested.\n"
    "@return true, if the cell has a mapping\n"
    "\n"
    "Note that if the cell is supposed to be dropped (see \\DropCell), the respective "
    "source cell will also be regarded \"mapped\", so has_mapping? will return true in this case.\n"
  ) +
  gsi::method ("cell_mapping", &db::CellMapping::cell_mapping, gsi::arg ("cell_index_b"),
    "@brief Determines cell mapping of a layout_b cell to the corresponding layout_a cell.\n"
    "\n"
    "\n"
    "@param cell_index_b The index of the cell in layout_b whose mapping is requested.\n"
    "@return The cell index in layout_a.\n"
    "\n"
    "Note that the returned index can be \\DropCell to indicate the cell shall be dropped."
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
  "\n"
  "Here is one example for using \\CellMapping. It extracts cells 'A', 'B' and 'C' from one layout "
  "and copies them to another. It will also copy all shapes and all child cells. Child cells which are "
  "shared between the three initial cells will be shared in the target layout too.\n"
  "\n"
  "@code\n"
  "cell_names = [ \"A\", \"B\", \"C\" ]\n"
  "\n"
  "source = RBA::Layout::new\n"
  "source.read(\"input.gds\")\n"
  "\n"
  "target = RBA::Layout::new\n"
  "\n"
  "source_cells = cell_names.collect { |n| source.cell_by_name(n).cell_index }\n"
  "target_cells = cell_names.collect { |n| target.create_cell(n).cell_index }\n"
  "\n"
  "cm = RBA::CellMapping::new\n"
  "cm.for_multi_cells_full(target, target_cells, source, source_cells)\n"
  "target.copy_tree_shapes(source, cm)\n"
  "@/code\n"
);

}


/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
#include "dbSaveLayoutOptions.h"

namespace gsi
{

static db::SaveLayoutOptions *new_v ()
{
  return new db::SaveLayoutOptions ();
}

static std::string set_format_from_filename (db::SaveLayoutOptions *opt, const std::string &fn)
{
  auto ff = opt->set_format_from_filename (fn);
  if (! ff.first) {
    throw tl::Exception (tl::to_string (tr ("Cannot determine format from filename")));
  }
  return ff.second;
}

Class<db::SaveLayoutOptions> decl_SaveLayoutOptions ("db", "SaveLayoutOptions",
  gsi::constructor ("new", &new_v,
    "@brief Default constructor\n"
    "\n"
    "This will initialize the scale factor to 1.0, the database unit is set to\n"
    "\"same as original\" and all layers are selected as well as all cells.\n"
    "The default format is GDS2."
  ) +
  gsi::method_ext ("set_format_from_filename", &set_format_from_filename, gsi::arg ("filename"),
    "@brief Select a format from the given file name\n"
    "\n"
    "This method will set the format according to the file's extension.\n"
    "\n"
    "This method has been introduced in version 0.22. "
    "Beginning with version 0.23, this method always returns true, since the "
    "only consumer for the return value, Layout#write, now ignores that "
    "parameter and automatically determines the compression mode from the file name.\n"
    "\n"
    "Starting with version 0.30.8, this method allows specifying the desired format's extension in square brackets "
    "after the file name (e.g. 'file.txt[def]'). This allows writing files with non-standard extensions. "
    "The return value of this function now is the actual file name used without the square brackets ('file.txt' in the example case)."
  ) +
  gsi::method ("format=", &db::SaveLayoutOptions::set_format, gsi::arg ("format"),
    "@brief Select a format\n"
    "The format string can be either \"GDS2\", \"OASIS\", \"CIF\" or \"DXF\". Other formats may be available if\n"
    "a suitable plugin is installed."
  ) +
  gsi::method ("format", &db::SaveLayoutOptions::format,
    "@brief Gets the format name\n"
    "\n"
    "See \\format= for a description of that method.\n"
  ) + 
  gsi::method ("add_layer", &db::SaveLayoutOptions::add_layer, gsi::arg ("layer_index"), gsi::arg ("properties"),
    "@brief Add a layer to be saved \n"
    "\n"
    "\n"
    "Adds the layer with the given index to the layer list that will be written.\n"
    "If all layers have been selected previously, all layers will \n"
    "be unselected first and only the new layer remains.\n"
    "\n"
    "The 'properties' argument can be used to assign different layer properties than the ones\n"
    "present in the layout. Pass a default \\LayerInfo object to this argument to use the\n"
    "properties from the layout object. Construct a valid \\LayerInfo object with explicit layer,\n"
    "datatype and possibly a name to override the properties stored in the layout.\n"
  ) + 
  gsi::method ("select_all_layers", &db::SaveLayoutOptions::select_all_layers,
    "@brief Select all layers to be saved\n"
    "\n"
    "This method will clear all layers selected with \\add_layer so far and set the 'select all layers' flag.\n"
    "This is the default.\n"
  ) + 
  gsi::method ("deselect_all_layers", &db::SaveLayoutOptions::deselect_all_layers,
    "@brief Unselect all layers: no layer will be saved\n"
    "\n"
    "This method will clear all layers selected with \\add_layer so far and clear the 'select all layers' flag.\n"
    "Using this method is the only way to save a layout without any layers."
  ) + 
  gsi::method ("select_cell", &db::SaveLayoutOptions::select_cell, gsi::arg ("cell_index"),
    "@brief Selects a cell to be saved (plus hierarchy below)\n"
    "\n"
    "\n"
    "This method is basically a convenience method that combines \\clear_cells and \\add_cell.\n"
    "This method clears the 'select all cells' flag.\n"
    "\n"
    "This method has been added in version 0.22.\n"
  ) + 
  gsi::method ("select_this_cell", &db::SaveLayoutOptions::select_this_cell, gsi::arg ("cell_index"),
    "@brief Selects a cell to be saved\n"
    "\n"
    "\n"
    "This method is basically a convenience method that combines \\clear_cells and \\add_this_cell.\n"
    "This method clears the 'select all cells' flag.\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) + 
  gsi::method ("clear_cells", &db::SaveLayoutOptions::clear_cells,
    "@brief Clears all cells to be saved\n"
    "\n"
    "This method can be used to ensure that no cell is selected before \\add_cell is called to specify a cell.\n"
    "This method clears the 'select all cells' flag.\n"
    "\n"
    "This method has been added in version 0.22.\n"
  ) + 
  gsi::method ("add_this_cell", &db::SaveLayoutOptions::add_this_cell, gsi::arg ("cell_index"),
    "@brief Adds a cell to be saved\n"
    "\n"
    "\n"
    "The index of the cell must be a valid index in the context of the layout that will be saved.\n"
    "This method clears the 'select all cells' flag.\n"
    "Unlike \\add_cell, this method does not implicitly add all children of that cell.\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) + 
  gsi::method ("add_cell", &db::SaveLayoutOptions::add_cell, gsi::arg ("cell_index"),
    "@brief Add a cell (plus hierarchy) to be saved\n"
    "\n"
    "\n"
    "The index of the cell must be a valid index in the context of the layout that will be saved.\n"
    "This method clears the 'select all cells' flag.\n"
    "\n"
    "This method also implicitly adds the children of that cell. A method that does not add the "
    "children in \\add_this_cell.\n"
  ) + 
  gsi::method ("select_all_cells", &db::SaveLayoutOptions::select_all_cells,
    "@brief Select all cells to save\n"
    "\n"
    "This method will clear all cells specified with \\add_cells so far and set the 'select all cells' flag.\n"
    "This is the default.\n"
  ) + 
  gsi::method ("write_context_info=", &db::SaveLayoutOptions::set_write_context_info, gsi::arg ("flag"),
    "@brief Enables or disables context information\n"
    "\n"
    "If this flag is set to false, no context information for PCell or library cell instances is written. "
    "Those cells will be converted to plain cells and KLayout will not be able to restore the identity of "
    "those cells. Use this option to enforce compatibility with other tools that don't understand the "
    "context information of KLayout.\n"
    "\n"
    "The default value is true (context information is stored). Not all formats support context information, hence "
    "that flag has no effect for formats like CIF or DXF.\n"
    "\n"
    "This method was introduced in version 0.23.\n"
  ) +
  gsi::method ("write_context_info?", &db::SaveLayoutOptions::write_context_info,
    "@brief Gets a flag indicating whether context information will be stored\n"
    "\n"
    "See \\write_context_info= for details about this flag.\n"
    "\n"
    "This method was introduced in version 0.23.\n"
  ) +
  gsi::method ("keep_instances=", &db::SaveLayoutOptions::set_keep_instances, gsi::arg ("flag"),
    "@brief Enables or disables instances for dropped cells\n"
    "\n"
    "If this flag is set to true, instances for cells will be written, even if the cell is dropped. "
    "That may happen, if cells are selected with \\select_this_cell or \\add_this_cell or \\no_empty_cells is used. "
    "Even if cells called by such cells are not selected, instances will be written for that "
    "cell if \"keep_instances\" is true. That feature is supported by the GDS format currently and "
    "results in \"ghost cells\" which have instances but no cell definition.\n"
    "\n"
    "The default value is false (instances of dropped cells are not written).\n"
    "\n"
    "This method was introduced in version 0.23.\n"
  ) +
  gsi::method ("keep_instances?", &db::SaveLayoutOptions::keep_instances,
    "@brief Gets a flag indicating whether instances will be kept even if the target cell is dropped\n"
    "\n"
    "See \\keep_instances= for details about this flag.\n"
    "\n"
    "This method was introduced in version 0.23.\n"
  ) +
  gsi::method ("dbu=", &db::SaveLayoutOptions::set_dbu, gsi::arg ("dbu"),
    "@brief Sets the database unit to be used in the stream file\n"
    "\n"
    "By default, the database unit of the layout is used. This method allows one to explicitly use a different\n"
    "database unit. A scale factor is introduced automatically which scales all layout objects accordingly so their physical dimensions remain the same. "
    "When scaling to a larger database unit or one that is not an integer fraction of the original one, rounding errors may occur and the "
    "layout may become slightly distorted."
  ) + 
  gsi::method ("dbu", &db::SaveLayoutOptions::dbu,
    "@brief Gets the explicit database unit if one is set\n"
    "\n"
    "See \\dbu= for a description of that attribute.\n"
  ) + 
  gsi::method ("no_empty_cells=", &db::SaveLayoutOptions::set_dont_write_empty_cells, gsi::arg ("flag"),
    "@brief Don't write empty cells if this flag is set\n"
    "\n"
    "By default, all cells are written (no_empty_cells is false).\n"
    "This applies to empty cells which do not contain shapes for the specified layers "
    "as well as cells which are empty because they reference empty cells only.\n"
  ) + 
  gsi::method ("no_empty_cells?", &db::SaveLayoutOptions::dont_write_empty_cells,
    "@brief Returns a flag indicating whether empty cells are not written.\n"
  ) + 
  gsi::method ("libname=|#gds2_libname=", &db::SaveLayoutOptions::set_libname, gsi::arg ("libname"),
    "@brief Sets the library name\n"
    "\n"
    "The library name is an attribute and specifies a formal name for a library, if the layout files is to be used as one.\n"
    "Currently, this attribute is only supported by the GDS2 format. Hence the alias.\n"
    "\n"
    "By default or if the libname is an empty string, the current library name of the layout or 'LIB' is used.\n"
    "\n"
    "The 'libname' alias has been introduced in version 0.30.5. The original name \\gds2_libname= is still available."
  ) +
  gsi::method ("libname|#gds2_libname", &db::SaveLayoutOptions::libname,
    "@brief Gets the library name\n"
    "\n"
    "See \\libname= for details.\n"
    "The 'libname' alias has been introduced in version 0.30.5. The original name \\gds2_libname is still available."
  ) +
  gsi::method ("scale_factor=", &db::SaveLayoutOptions::set_scale_factor, gsi::arg ("scale_factor"),
    "@brief Sets the scaling factor for the saving \n"
    "\n"
    "Using a scaling factor will scale all objects accordingly. "
    "This scale factor adds to a potential scaling implied by using an explicit database unit.\n"
    "\n"
    "Be aware that rounding effects may occur if fractional scaling factors are used.\n"
    "\n"
    "By default, no scaling is applied."
  ) + 
  gsi::method ("scale_factor", &db::SaveLayoutOptions::scale_factor,
    "@brief Gets the scaling factor currently set\n"
  ),
  "@brief Options for saving layouts\n"
  "\n"
  "This class describes the various options for saving a layout to a stream file (GDS2, OASIS and others).\n"
  "There are: layers to be saved, cell or cells to be saved, scale factor, format, database unit\n"
  "and format specific options.\n"
  "\n"
  "Usually the default constructor provides a suitable object. Please note, that the format written is \"GDS2\" by default. Either explicitly set a "
  "format using \\format= or derive the format from the file name using \\set_format_from_filename.\n"
  "\n"
  "The layers are specified by either selecting all layers or by defining layer by layer using the\n"
  "\\add_layer method. \\select_all_layers will explicitly select all layers for saving, \\deselect_all_layers will explicitly clear the list of layers.\n"
  "\n"
  "Cells are selected in a similar fashion: by default, all cells are selected. Using \\add_cell, specific\n"
  "cells can be selected for saving. All these cells plus their hierarchy will then be written to the stream file.\n"
  "\n"
);

}

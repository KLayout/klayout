
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



#include "dbCommonReader.h"
#include "dbLoadLayoutOptions.h"
#include "gsiDecl.h"
#include "gsiEnums.h"

namespace dn
{

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_layer_map (db::LoadLayoutOptions *options, const db::LayerMap &lm, bool f)
{
  options->get_options<db::CommonReaderOptions> ().layer_map = lm;
  options->get_options<db::CommonReaderOptions> ().create_other_layers = f;
}

static void set_layer_map1 (db::LoadLayoutOptions *options, const db::LayerMap &lm)
{
  options->get_options<db::CommonReaderOptions> ().layer_map = lm;
}

static db::LayerMap &get_layer_map (db::LoadLayoutOptions *options)
{
  return options->get_options<db::CommonReaderOptions> ().layer_map;
}

static void select_all_layers (db::LoadLayoutOptions *options)
{
  options->get_options<db::CommonReaderOptions> ().layer_map = db::LayerMap ();
  options->get_options<db::CommonReaderOptions> ().create_other_layers = true;
}

static bool create_other_layers (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CommonReaderOptions> ().create_other_layers;
}

static void set_create_other_layers (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::CommonReaderOptions> ().create_other_layers = l;
}

static bool get_text_enabled (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CommonReaderOptions> ().enable_text_objects;
}

static void set_text_enabled (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::CommonReaderOptions> ().enable_text_objects = l;
}

static bool get_properties_enabled (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CommonReaderOptions> ().enable_properties;
}

static void set_properties_enabled (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::CommonReaderOptions> ().enable_properties = l;
}

static db::CellConflictResolution get_cell_conflict_resolution (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CommonReaderOptions> ().cell_conflict_resolution;
}

static void set_cell_conflict_resolution (db::LoadLayoutOptions *options, db::CellConflictResolution cc)
{
  options->get_options<db::CommonReaderOptions> ().cell_conflict_resolution = cc;
}

//  extend lay::LoadLayoutOptions with the Common options
static
gsi::ClassExt<db::LoadLayoutOptions> common_reader_options (
  gsi::method_ext ("set_layer_map", &set_layer_map, gsi::arg ("map"), gsi::arg ("create_other_layers"),
    "@brief Sets the layer map\n"
    "This sets a layer mapping for the reader. The layer map allows selection and translation of the original layers, for example to add a layer name.\n"
    "@param map The layer map to set."
    "@param create_other_layers The flag telling whether other layer should be created as well. Set to false if just the layers in the mapping table should be read.\n"
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("layer_map=", &set_layer_map1, gsi::arg ("map"),
    "@brief Sets the layer map, but does not affect the \"create_other_layers\" flag.\n"
    "Use \\create_other_layers? to enable or disable other layers not listed in the layer map.\n"
    "@param map The layer map to set."
    "\n"
    "This convenience method has been introduced with version 0.26."
  ) +
  gsi::method_ext ("select_all_layers", &select_all_layers,
    "@brief Selects all layers and disables the layer map\n"
    "\n"
    "This disables any layer map and enables reading of all layers.\n"
    "New layers will be created when required.\n"
    "\n"
    "Starting with version 0.25 this method only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("layer_map", &get_layer_map,
    "@brief Gets the layer map\n"
    "@return A reference to the layer map\n"
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
    "\n"
    "Python note: this method has been turned into a property in version 0.26."
  ) +
  gsi::method_ext ("create_other_layers?", &create_other_layers,
    "@brief Gets a value indicating whether other layers shall be created\n"
    "@return True, if other layers should be created.\n"
    "This attribute acts together with a layer map (see \\layer_map=). Layers not listed in this map are created as well when "
    "\\create_other_layers? is true. Otherwise they are ignored.\n"
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("create_other_layers=", &set_create_other_layers, gsi::arg ("create"),
    "@brief Specifies whether other layers shall be created\n"
    "@param create True, if other layers should be created.\n"
    "See \\create_other_layers? for a description of this attribute.\n"
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("text_enabled?|#is_text_enabled?", &get_text_enabled,
    "@brief Gets a value indicating whether text objects shall be read\n"
    "@return True, if text objects should be read."
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("text_enabled=", &set_text_enabled, gsi::arg ("enabled"),
    "@brief Specifies whether text objects shall be read\n"
    "@param enabled True, if text objects should be read."
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("properties_enabled?|#is_properties_enabled?", &get_properties_enabled,
    "@brief Gets a value indicating whether properties shall be read\n"
    "@return True, if properties should be read."
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("properties_enabled=", &set_properties_enabled, gsi::arg ("enabled"),
    "@brief Specifies whether properties should be read\n"
    "@param enabled True, if properties should be read."
    "\n"
    "Starting with version 0.25 this option only applies to GDS2 and OASIS format. Other formats provide their own configuration."
  ) +
  gsi::method_ext ("cell_conflict_resolution", &get_cell_conflict_resolution,
    "@brief Gets the cell conflict resolution mode\n"
    "\n"
    "Multiple layout files can be collected into a single Layout object by reading file after file into the Layout object. "
    "Cells with same names are considered a conflict. This mode indicates how such conflicts are resolved. See \\LoadLayoutOptions::CellConflictResolution "
    "for the values allowed. The default mode is \\LoadLayoutOptions::CellConflictResolution#AddToCell.\n"
    "\n"
    "This option has been introduced in version 0.27."
  ) +
  gsi::method_ext ("cell_conflict_resolution=", &set_cell_conflict_resolution, gsi::arg ("mode"),
    "@brief Sets the cell conflict resolution mode\n"
    "\n"
    "See \\cell_conflict_resolution for details about this option.\n"
    "\n"
    "This option has been introduced in version 0.27."
  ),
  ""
);


gsi::EnumIn<db::LoadLayoutOptions, db::CellConflictResolution> decl_dbCommonReader_CellConflictResolution ("db", "CellConflictResolution",
  gsi::enum_const ("AddToCell", db::AddToCell,
    "@brief Add content to existing cell\n"
    "This is the mode use in before version 0.27. Content of new cells is simply added to existing cells with the same name."
  ) +
  gsi::enum_const ("OverwriteCell", db::OverwriteCell,
    "@brief The old cell is overwritten entirely (including child cells which are not used otherwise)\n"
  ) +
  gsi::enum_const ("SkipNewCell", db::SkipNewCell,
    "@brief The new cell is skipped entirely (including child cells which are not used otherwise)\n"
  ) +
  gsi::enum_const ("RenameCell", db::RenameCell,
    "@brief The new cell will be renamed to become unique\n"
  ),
  "@brief This enum specifies how cell conflicts are handled if a layout read into another layout and a cell name conflict arises.\n"
  "Until version 0.26.8 and before, the mode was always 'AddToCell'. On reading, a cell was 'reopened' when encountering a cell name "
  "which already existed. This mode is still the default. The other modes are made available to support other ways of merging layouts.\n"
  "\n"
  "Proxy cells are never modified in the existing layout. Proxy cells are always local to their layout file. So if the existing cell is "
  "a proxy cell, the new cell will be renamed.\n"
  "\n"
  "If the new or existing cell is a ghost cell, both cells are merged always.\n"
  "\n"
  "This enum was introduced in version 0.27.\n"
);

//  Inject the NetlistCrossReference::Status declarations into NetlistCrossReference:
gsi::ClassExt<db::LoadLayoutOptions> inject_CellConflictResolution_in_parent (decl_dbCommonReader_CellConflictResolution.defs ());

}





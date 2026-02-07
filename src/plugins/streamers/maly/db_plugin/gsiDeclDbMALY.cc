
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

#include "dbMALY.h"
#include "dbMALYReader.h"
#include "dbLoadLayoutOptions.h"
#include "dbSaveLayoutOptions.h"
#include "gsiDecl.h"

namespace gsi
{

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_maly_dbu (db::LoadLayoutOptions *options, double dbu)
{
  options->get_options<db::MALYReaderOptions> ().dbu = dbu;
}

static double get_maly_dbu (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::MALYReaderOptions> ().dbu;
}

static void set_layer_map (db::LoadLayoutOptions *options, const db::LayerMap &lm, bool f)
{
  options->get_options<db::MALYReaderOptions> ().layer_map = lm;
  options->get_options<db::MALYReaderOptions> ().create_other_layers = f;
}

static void set_layer_map1 (db::LoadLayoutOptions *options, const db::LayerMap &lm)
{
  options->get_options<db::MALYReaderOptions> ().layer_map = lm;
}

static db::LayerMap &get_layer_map (db::LoadLayoutOptions *options)
{
  return options->get_options<db::MALYReaderOptions> ().layer_map;
}

static void select_all_layers (db::LoadLayoutOptions *options)
{
  options->get_options<db::MALYReaderOptions> ().layer_map = db::LayerMap ();
  options->get_options<db::MALYReaderOptions> ().create_other_layers = true;
}

static bool create_other_layers (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::MALYReaderOptions> ().create_other_layers;
}

static void set_create_other_layers (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::MALYReaderOptions> ().create_other_layers = l;
}

//  extend lay::LoadLayoutOptions with the MALY options
static
gsi::ClassExt<db::LoadLayoutOptions> maly_reader_options (
  gsi::method_ext ("maly_set_layer_map", &set_layer_map, gsi::arg ("map"), gsi::arg ("create_other_layers"),
    "@brief Sets the layer map\n"
    "This sets a layer mapping for the reader. The layer map allows selection and translation of the original layers, for example to assign layer/datatype numbers to the named layers.\n"
    "@param map The layer map to set.\n"
    "@param create_other_layers The flag indicating whether other layers will be created as well. Set to false to read only the layers in the layer map.\n"
    "\n"
    "Layer maps can also be used to map the named MALY mask layers to GDS layer/datatypes.\n"
    "\n"
    "This method has been added in version 0.30.2."
  ) +
  gsi::method_ext ("maly_layer_map=", &set_layer_map1, gsi::arg ("map"),
    "@brief Sets the layer map\n"
    "This sets a layer mapping for the reader. Unlike \\maly_set_layer_map, the 'create_other_layers' flag is not changed.\n"
    "@param map The layer map to set.\n"
    "\n"
    "Layer maps can also be used to map the named MALY mask layers to GDS layer/datatypes.\n"
    "\n"
    "This method has been added in version 0.30.2."
  ) +
  gsi::method_ext ("maly_select_all_layers", &select_all_layers,
    "@brief Selects all layers and disables the layer map\n"
    "\n"
    "This disables any layer map and enables reading of all layers.\n"
    "New layers will be created when required.\n"
    "\n"
    "This method has been added in version 0.30.2."
  ) +
  gsi::method_ext ("maly_layer_map", &get_layer_map,
    "@brief Gets the layer map\n"
    "@return A reference to the layer map\n"
    "\n"
    "This method has been added in version 0.30.2."
  ) +
  gsi::method_ext ("maly_create_other_layers?", &create_other_layers,
    "@brief Gets a value indicating whether other layers shall be created\n"
    "@return True, if other layers will be created.\n"
    "This attribute acts together with a layer map (see \\maly_layer_map=). Layers not listed in this map are created as well when "
    "\\maly_create_other_layers? is true. Otherwise they are ignored.\n"
    "\n"
    "This method has been added in version 0.30.2."
  ) +
  gsi::method_ext ("maly_create_other_layers=", &set_create_other_layers, gsi::arg ("create"),
    "@brief Specifies whether other layers shall be created\n"
    "@param create True, if other layers will be created.\n"
    "See \\maly_create_other_layers? for a description of this attribute.\n"
    "\n"
    "This method has been added in version 0.30.2."
  ) +
  gsi::method_ext ("maly_dbu=", &set_maly_dbu, gsi::arg ("dbu"),
    "@brief Specifies the database unit which the reader uses and produces\n"
    "The database unit is the final resolution of the produced layout. This physical resolution is usually "
    "defined by the layout system - GDS for example typically uses 1nm (maly_dbu=0.001).\n"
    "All geometry in the MALY pattern files is brought to the database unit by scaling.\n"
    "\n"
    "This method has been added in version 0.30.2."
  ) +
  gsi::method_ext ("maly_dbu", &get_maly_dbu,
    "@brief Specifies the database unit which the reader uses and produces\n"
    "See \\maly_dbu= method for a description of this property.\n"
    "\n"
    "This method has been added in version 0.30.2."
  ),
  ""
);

}


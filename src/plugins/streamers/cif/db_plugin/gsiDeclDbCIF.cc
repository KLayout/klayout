
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

#include "dbCIF.h"
#include "dbCIFReader.h"
#include "dbCIFWriter.h"
#include "dbLoadLayoutOptions.h"
#include "dbSaveLayoutOptions.h"
#include "gsiDecl.h"

namespace gsi
{

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_cif_wire_mode (db::LoadLayoutOptions *options, unsigned int n)
{
  options->get_options<db::CIFReaderOptions> ().wire_mode = n;
}

static unsigned int get_cif_wire_mode (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CIFReaderOptions> ().wire_mode;
}

static void set_cif_dbu (db::LoadLayoutOptions *options, double dbu)
{
  options->get_options<db::CIFReaderOptions> ().dbu = dbu;
}

static double get_cif_dbu (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CIFReaderOptions> ().dbu;
}

static void set_layer_map (db::LoadLayoutOptions *options, const db::LayerMap &lm, bool f)
{
  options->get_options<db::CIFReaderOptions> ().layer_map = lm;
  options->get_options<db::CIFReaderOptions> ().create_other_layers = f;
}

static void set_layer_map1 (db::LoadLayoutOptions *options, const db::LayerMap &lm)
{
  options->get_options<db::CIFReaderOptions> ().layer_map = lm;
}

static db::LayerMap &get_layer_map (db::LoadLayoutOptions *options)
{
  return options->get_options<db::CIFReaderOptions> ().layer_map;
}

static void select_all_layers (db::LoadLayoutOptions *options)
{
  options->get_options<db::CIFReaderOptions> ().layer_map = db::LayerMap ();
  options->get_options<db::CIFReaderOptions> ().create_other_layers = true;
}

static bool create_other_layers (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CIFReaderOptions> ().create_other_layers;
}

static void set_create_other_layers (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::CIFReaderOptions> ().create_other_layers = l;
}

static bool keep_layer_names (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::CIFReaderOptions> ().keep_layer_names;
}

static void set_keep_layer_names (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::CIFReaderOptions> ().keep_layer_names = l;
}

//  extend lay::LoadLayoutOptions with the CIF options
static
gsi::ClassExt<db::LoadLayoutOptions> cif_reader_options (
    gsi::method_ext ("cif_set_layer_map", &set_layer_map, gsi::arg ("map"), gsi::arg ("create_other_layers"),
      "@brief Sets the layer map\n"
      "This sets a layer mapping for the reader. The layer map allows selection and translation of the original layers, for example to assign layer/datatype numbers to the named layers.\n"
      "@param map The layer map to set.\n"
      "@param create_other_layers The flag indicating whether other layers will be created as well. Set to false to read only the layers in the layer map.\n"
      "\n"
      "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
      "in a format-specific fashion."
    ) +
    gsi::method_ext ("cif_layer_map=", &set_layer_map1, gsi::arg ("map"),
      "@brief Sets the layer map\n"
      "This sets a layer mapping for the reader. Unlike \\cif_set_layer_map, the 'create_other_layers' flag is not changed.\n"
      "@param map The layer map to set.\n"
      "\n"
      "This convenience method has been added in version 0.26."
    ) +
    gsi::method_ext ("cif_select_all_layers", &select_all_layers,
      "@brief Selects all layers and disables the layer map\n"
      "\n"
      "This disables any layer map and enables reading of all layers.\n"
      "New layers will be created when required.\n"
      "\n"
      "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
      "in a format-specific fashion."
    ) +
    gsi::method_ext ("cif_layer_map", &get_layer_map,
      "@brief Gets the layer map\n"
      "@return A reference to the layer map\n"
      "\n"
      "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
      "in a format-specific fashion.\n"
      "\n"
      "Python note: this method has been turned into a property in version 0.26."
    ) +
    gsi::method_ext ("cif_create_other_layers?", &create_other_layers,
      "@brief Gets a value indicating whether other layers shall be created\n"
      "@return True, if other layers will be created.\n"
      "This attribute acts together with a layer map (see \\cif_layer_map=). Layers not listed in this map are created as well when "
      "\\cif_create_other_layers? is true. Otherwise they are ignored.\n"
      "\n"
      "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
      "in a format-specific fashion."
    ) +
    gsi::method_ext ("cif_create_other_layers=", &set_create_other_layers, gsi::arg ("create"),
      "@brief Specifies whether other layers shall be created\n"
      "@param create True, if other layers will be created.\n"
      "See \\cif_create_other_layers? for a description of this attribute.\n"
      "\n"
      "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
      "in a format-specific fashion."
    ) +
    gsi::method_ext ("cif_keep_layer_names?", &keep_layer_names,
      "@brief Gets a value indicating whether layer names are kept\n"
      "@return True, if layer names are kept.\n"
      "\n"
      "When set to true, no attempt is made to translate "
      "layer names to GDS layer/datatype numbers. If set to false (the default), a layer named \"L2D15\" will be translated "
      "to GDS layer 2, datatype 15.\n"
      "\n"
      "This method has been added in version 0.25.3."
    ) +
    gsi::method_ext ("cif_keep_layer_names=", &set_keep_layer_names, gsi::arg ("keep"),
      "@brief Gets a value indicating whether layer names are kept\n"
      "@param keep True, if layer names are to be kept.\n"
      "\n"
      "See \\cif_keep_layer_names? for a description of this property.\n"
      "\n"
      "This method has been added in version 0.25.3."
    ) +
    gsi::method_ext ("cif_wire_mode=", &set_cif_wire_mode,
    "@brief How to read 'W' objects\n"
    "\n"
    "This property specifies how to read 'W' (wire) objects.\n"
    "Allowed values are 0 (as square ended paths), 1 (as flush ended paths), 2 (as round paths)\n"
    "\nThis property has been added in version 0.21.\n"
  ) +
  gsi::method_ext ("cif_wire_mode", &get_cif_wire_mode,
    "@brief Specifies how to read 'W' objects\n"
    "See \\cif_wire_mode= method for a description of this mode."
    "\nThis property has been added in version 0.21 and was renamed to cif_wire_mode in 0.25.\n"
  ) +
  gsi::method_ext ("cif_dbu=", &set_cif_dbu,
    "@brief Specifies the database unit which the reader uses and produces\n"
    "\nThis property has been added in version 0.21.\n"
  ) +
  gsi::method_ext ("cif_dbu", &get_cif_dbu,
    "@brief Specifies the database unit which the reader uses and produces\n"
    "See \\cif_dbu= method for a description of this property."
    "\nThis property has been added in version 0.21.\n"
  ),
  ""
);

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_cif_dummy_calls (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::CIFWriterOptions> ().dummy_calls = f;
}

static bool get_cif_dummy_calls (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::CIFWriterOptions> ().dummy_calls;
}

static void set_cif_blank_separator (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::CIFWriterOptions> ().blank_separator = f;
}

static bool get_cif_blank_separator (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::CIFWriterOptions> ().blank_separator;
}

//  extend lay::SaveLayoutOptions with the CIF options
static
gsi::ClassExt<db::SaveLayoutOptions> cif_writer_options (
  gsi::method_ext ("cif_dummy_calls=", &set_cif_dummy_calls,
    "@brief Sets a flag indicating whether dummy calls shall be written\n"
    "If this property is set to true, dummy calls will be written in the top level entity "
    "of the CIF file calling every top cell.\n"
    "This option is useful for enhanced compatibility with other tools.\n"
    "\nThis property has been added in version 0.23.10.\n"
  ) +
  gsi::method_ext ("cif_dummy_calls?|#cif_dummy_calls", &get_cif_dummy_calls,
    "@brief Gets a flag indicating whether dummy calls shall be written\n"
    "See \\cif_dummy_calls= method for a description of that property."
    "\nThis property has been added in version 0.23.10.\n"
    "\nThe predicate version (cif_blank_separator?) has been added in version 0.25.1.\n"
  ) +
  gsi::method_ext ("cif_blank_separator=", &set_cif_blank_separator,
    "@brief Sets a flag indicating whether blanks shall be used as x/y separator characters\n"
    "If this property is set to true, the x and y coordinates are separated with blank characters "
    "rather than comma characters."
    "\nThis property has been added in version 0.23.10.\n"
  ) +
  gsi::method_ext ("cif_blank_separator?|#cif_blank_separator", &get_cif_blank_separator,
    "@brief Gets a flag indicating whether blanks shall be used as x/y separator characters\n"
    "See \\cif_blank_separator= method for a description of that property."
    "\nThis property has been added in version 0.23.10.\n"
    "\nThe predicate version (cif_blank_separator?) has been added in version 0.25.1.\n"
  ),
  ""
);

}


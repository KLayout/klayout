
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "dbMAG.h"
#include "dbMAGReader.h"
#include "dbMAGWriter.h"
#include "dbLoadLayoutOptions.h"
#include "dbSaveLayoutOptions.h"
#include "gsiDecl.h"

namespace gsi
{

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_mag_dbu (db::LoadLayoutOptions *options, double dbu)
{
  options->get_options<db::MAGReaderOptions> ().dbu = dbu;
}

static double get_mag_dbu (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::MAGReaderOptions> ().dbu;
}

static void set_mag_lambda (db::LoadLayoutOptions *options, double lambda)
{
  options->get_options<db::MAGReaderOptions> ().lambda = lambda;
}

static double get_mag_lambda (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::MAGReaderOptions> ().lambda;
}

static void set_mag_library_paths (db::LoadLayoutOptions *options, const std::vector<std::string> &lib_paths)
{
  options->get_options<db::MAGReaderOptions> ().lib_paths = lib_paths;
}

static std::vector<std::string> get_mag_library_paths (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::MAGReaderOptions> ().lib_paths;
}

static void set_layer_map (db::LoadLayoutOptions *options, const db::LayerMap &lm, bool f)
{
  options->get_options<db::MAGReaderOptions> ().layer_map = lm;
  options->get_options<db::MAGReaderOptions> ().create_other_layers = f;
}

static void set_layer_map1 (db::LoadLayoutOptions *options, const db::LayerMap &lm)
{
  options->get_options<db::MAGReaderOptions> ().layer_map = lm;
}

static db::LayerMap &get_layer_map (db::LoadLayoutOptions *options)
{
  return options->get_options<db::MAGReaderOptions> ().layer_map;
}

static void select_all_layers (db::LoadLayoutOptions *options)
{
  options->get_options<db::MAGReaderOptions> ().layer_map = db::LayerMap ();
  options->get_options<db::MAGReaderOptions> ().create_other_layers = true;
}

static bool create_other_layers (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::MAGReaderOptions> ().create_other_layers;
}

static void set_create_other_layers (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::MAGReaderOptions> ().create_other_layers = l;
}

static bool keep_layer_names (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::MAGReaderOptions> ().keep_layer_names;
}

static void set_keep_layer_names (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::MAGReaderOptions> ().keep_layer_names = l;
}

//  extend lay::LoadLayoutOptions with the MAG options
static
gsi::ClassExt<db::LoadLayoutOptions> mag_reader_options (
  gsi::method_ext ("mag_set_layer_map", &set_layer_map, gsi::arg ("map"), gsi::arg ("create_other_layers"),
    "@brief Sets the layer map\n"
    "This sets a layer mapping for the reader. The \"create_other_layers\" specifies whether to create layers that are not "
    "in the mapping and automatically assign layers to them.\n"
    "@param map The layer map to set."
    "@param create_other_layers The flag telling whether other layer should be created also. Set to false if just the layers in the mapping table should be read.\n"
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_layer_map=", &set_layer_map1, gsi::arg ("map"),
    "@brief Sets the layer map\n"
    "This sets a layer mapping for the reader. Unlike \\mag_set_layer_map, the 'create_other_layers' flag is not changed.\n"
    "@param map The layer map to set."
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_select_all_layers", &select_all_layers,
    "@brief Selects all layers and disables the layer map\n"
    "\n"
    "This disables any layer map and enables reading of all layers.\n"
    "New layers will be created when required.\n"
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_layer_map", &get_layer_map,
    "@brief Gets the layer map\n"
    "@return A reference to the layer map\n"
    "\n"
    "This method has been added in version 0.25 and replaces the respective global option in \\LoadLayoutOptions "
    "in a format-specific fashion.\n"
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_create_other_layers?", &create_other_layers,
    "@brief Gets a value indicating whether other layers shall be created\n"
    "@return True, if other layers should be created.\n"
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_create_other_layers=", &set_create_other_layers, gsi::arg ("create"),
    "@brief Specifies whether other layers shall be created\n"
    "@param create True, if other layers should be created.\n"
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_keep_layer_names?", &keep_layer_names,
    "@brief Gets a value indicating whether layer names are kept\n"
    "@return True, if layer names are kept.\n"
    "\n"
    "When set to true, no attempt is made to translate "
    "layer names to GDS layer/datatype numbers. If set to false (the default), a layer named \"L2D15\" will be translated "
    "to GDS layer 2, datatype 15.\n"
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_keep_layer_names=", &set_keep_layer_names, gsi::arg ("keep"),
    "@brief Gets a value indicating whether layer names are kept\n"
    "@param keep True, if layer names are to be kept.\n"
    "\n"
    "See \\mag_keep_layer_names? for a description of this property.\n"
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_library_paths=", &set_mag_library_paths, gsi::arg ("lib_paths"),
    "@brief Specifies the locations where to look up libraries (in this order)\n"
    "\n"
    "The reader will look up library reference in these paths when it can't find them locally.\n"
    "Relative paths in this collection are resolved relative to the initial file's path.\n"
    "Expression interpolation is supported in the path strings.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_library_paths", &get_mag_library_paths,
    "@brief Get the locations where to look up libraries (in this order)\n"
    "See \\mag_library_paths= method for a description of this attribute."
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_lambda=", &set_mag_lambda, gsi::arg ("lambda"),
    "@brief Specifies the lambda value to used for reading\n"
    "\n"
    "The lamdba value is the basic unit of the layout.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_lambda", &get_mag_lambda,
    "@brief Get the lambda value\n"
    "See \\mag_lambda= method for a description of this attribute."
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_dbu=", &set_mag_dbu, gsi::arg ("dbu"),
    "@brief Specifies the database unit which the reader uses and produces\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_dbu", &get_mag_dbu,
    "@brief Specifies the database unit which the reader uses and produces\n"
    "See \\mag_dbu= method for a description of this property."
    "\nThis property has been added in version 0.26.2.\n"
  ),
  ""
);

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_mag_dummy_calls (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::MAGWriterOptions> ().dummy_calls = f;
}

static bool get_mag_dummy_calls (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::MAGWriterOptions> ().dummy_calls;
}

static void set_mag_blank_separator (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::MAGWriterOptions> ().blank_separator = f;
}

static bool get_mag_blank_separator (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::MAGWriterOptions> ().blank_separator;
}

//  extend lay::SaveLayoutOptions with the GDS2 options
static
gsi::ClassExt<db::SaveLayoutOptions> mag_writer_options (
  gsi::method_ext ("mag_dummy_calls=", &set_mag_dummy_calls,
    "@brief Sets a flag indicating whether dummy calls shall be written\n"
    "If this property is set to true, dummy calls will be written in the top level entity "
    "of the MAG file calling every top cell.\n"
    "This option is useful for enhanced compatibility with other tools.\n"
    "\nThis property has been added in version 0.23.10.\n"
  ) +
  gsi::method_ext ("mag_dummy_calls?|#mag_dummy_calls", &get_mag_dummy_calls,
    "@brief Gets a flag indicating whether dummy calls shall be written\n"
    "See \\mag_dummy_calls= method for a description of that property."
    "\nThis property has been added in version 0.23.10.\n"
    "\nThe predicate version (mag_blank_separator?) has been added in version 0.25.1.\n"
  ) +
  gsi::method_ext ("mag_blank_separator=", &set_mag_blank_separator,
    "@brief Sets a flag indicating whether blanks shall be used as x/y separator characters\n"
    "If this property is set to true, the x and y coordinates are separated with blank characters "
    "rather than comma characters."
    "\nThis property has been added in version 0.23.10.\n"
  ) +
  gsi::method_ext ("mag_blank_separator?|#mag_blank_separator", &get_mag_blank_separator,
    "@brief Gets a flag indicating whether blanks shall be used as x/y separator characters\n"
    "See \\mag_blank_separator= method for a description of that property."
    "\nThis property has been added in version 0.23.10.\n"
    "\nThe predicate version (mag_blank_separator?) has been added in version 0.25.1.\n"
  ),
  ""
);

}


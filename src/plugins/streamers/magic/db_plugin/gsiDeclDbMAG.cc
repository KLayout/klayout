
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

static bool merge (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::MAGReaderOptions> ().merge;
}

static void set_merge (db::LoadLayoutOptions *options, bool l)
{
  options->get_options<db::MAGReaderOptions> ().merge = l;
}

//  extend lay::LoadLayoutOptions with the MAG options
static
gsi::ClassExt<db::LoadLayoutOptions> mag_reader_options (
  gsi::method_ext ("mag_set_layer_map", &set_layer_map, gsi::arg ("map"), gsi::arg ("create_other_layers"),
    "@brief Sets the layer map\n"
    "This sets a layer mapping for the reader. The layer map allows selection and translation of the original layers, for example to assign layer/datatype numbers to the named layers.\n"
    "@param map The layer map to set.\n"
    "@param create_other_layers The flag indicating whether other layers will be created as well. Set to false to read only the layers in the layer map.\n"
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_layer_map=", &set_layer_map1, gsi::arg ("map"),
    "@brief Sets the layer map\n"
    "This sets a layer mapping for the reader. Unlike \\mag_set_layer_map, the 'create_other_layers' flag is not changed.\n"
    "@param map The layer map to set.\n"
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
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_create_other_layers?", &create_other_layers,
    "@brief Gets a value indicating whether other layers shall be created\n"
    "@return True, if other layers will be created.\n"
    "This attribute acts together with a layer map (see \\mag_layer_map=). Layers not listed in this map are created as well when "
    "\\mag_create_other_layers? is true. Otherwise they are ignored.\n"
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_create_other_layers=", &set_create_other_layers, gsi::arg ("create"),
    "@brief Specifies whether other layers shall be created\n"
    "@param create True, if other layers will be created.\n"
    "See \\mag_create_other_layers? for a description of this attribute.\n"
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
  gsi::method_ext ("mag_merge?", &merge,
    "@brief Gets a value indicating whether boxes are merged into polygons\n"
    "@return True, if boxes are merged.\n"
    "\n"
    "When set to true, the boxes and triangles of the Magic layout files are merged into polygons where possible.\n"
    "\n"
    "This method has been added in version 0.26.2."
  ) +
  gsi::method_ext ("mag_merge=", &set_merge, gsi::arg ("merge"),
    "@brief Sets a value indicating whether boxes are merged into polygons\n"
    "@param merge True, if boxes and triangles will be merged into polygons.\n"
    "\n"
    "See \\mag_merge? for a description of this property.\n"
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
    "@brief Gets the locations where to look up libraries (in this order)\n"
    "See \\mag_library_paths= method for a description of this attribute.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_lambda=", &set_mag_lambda, gsi::arg ("lambda"),
    "@brief Specifies the lambda value to used for reading\n"
    "\n"
    "The lambda value is the basic unit of the layout. Magic draws layout as multiples of this basic unit. "
    "The layout read by the MAG reader will use the database unit specified by \\mag_dbu, but the physical layout "
    "coordinates will be multiples of \\mag_lambda.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_lambda", &get_mag_lambda,
    "@brief Gets the lambda value\n"
    "See \\mag_lambda= method for a description of this attribute.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_dbu=", &set_mag_dbu, gsi::arg ("dbu"),
    "@brief Specifies the database unit which the reader uses and produces\n"
    "The database unit is the final resolution of the produced layout. This physical resolution is usually "
    "defined by the layout system - GDS for example typically uses 1nm (mag_dbu=0.001).\n"
    "All geometry in the MAG file will first be scaled to \\mag_lambda and is then brought to the database unit.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_dbu", &get_mag_dbu,
    "@brief Specifies the database unit which the reader uses and produces\n"
    "See \\mag_dbu= method for a description of this property.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ),
  ""
);

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_mag_lambda_w (db::SaveLayoutOptions *options, double f)
{
  options->get_options<db::MAGWriterOptions> ().lambda = f;
}

static double get_mag_lambda_w (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::MAGWriterOptions> ().lambda;
}

static void set_mag_write_timestamp (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::MAGWriterOptions> ().write_timestamp = f;
}

static bool get_mag_write_timestamp (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::MAGWriterOptions> ().write_timestamp;
}

static void set_mag_tech_w (db::SaveLayoutOptions *options, const std::string &t)
{
  options->get_options<db::MAGWriterOptions> ().tech = t;
}

static const std::string &get_mag_tech_w (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::MAGWriterOptions> ().tech;
}

//  extend lay::SaveLayoutOptions with the MAG options
static
gsi::ClassExt<db::SaveLayoutOptions> mag_writer_options (
  gsi::method_ext ("mag_lambda=", &set_mag_lambda_w, gsi::arg ("lambda"),
    "@brief Specifies the lambda value to used for writing\n"
    "\n"
    "The lambda value is the basic unit of the layout.\n"
    "The layout is brought to units of this value. If the layout is not on-grid on this unit, snapping will happen. "
    "If the value is less or equal to zero, KLayout will use the lambda value stored inside the layout set by a previous read operation "
    "of a MAGIC file. The lambda value is stored in the Layout object as the \"lambda\" metadata attribute.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_lambda", &get_mag_lambda_w,
    "@brief Gets the lambda value\n"
    "See \\mag_lambda= method for a description of this attribute."
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_write_timestamp=", &set_mag_write_timestamp, gsi::arg ("f"),
    "@brief Specifies whether to write a timestamp\n"
    "\n"
    "If this attribute is set to false, the timestamp written is 0. This is not permitted in the strict sense, but simplifies comparison of Magic files.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_write_timestamp?", &get_mag_write_timestamp,
    "@brief Gets a value indicating whether to write a timestamp\n"
    "See \\write_timestamp= method for a description of this attribute.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_tech=", &set_mag_tech_w, gsi::arg ("tech"),
    "@brief Specifies the technology string used for writing\n"
    "\n"
    "If this string is empty, the writer will try to obtain the technology from the \"technology\" metadata attribute of the layout.\n"
    "\nThis property has been added in version 0.26.2.\n"
  ) +
  gsi::method_ext ("mag_tech", &get_mag_tech_w,
    "@brief Gets the technology string used for writing\n"
    "See \\mag_tech= method for a description of this attribute."
    "\nThis property has been added in version 0.26.2.\n"
  ),
  ""
);

}


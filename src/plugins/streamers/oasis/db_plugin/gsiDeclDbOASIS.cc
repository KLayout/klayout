
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

#include "dbOASIS.h"
#include "dbOASISReader.h"
#include "dbOASISWriter.h"
#include "dbLoadLayoutOptions.h"
#include "dbSaveLayoutOptions.h"
#include "gsiDecl.h"

namespace gsi
{

// ---------------------------------------------------------------
//  gsi Implementation of specific methods of LoadLayoutOptions

static void set_oasis_read_all_properties (db::LoadLayoutOptions *options, bool f)
{
  options->get_options<db::OASISReaderOptions> ().read_all_properties = f;
}

static int get_oasis_read_all_properties (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::OASISReaderOptions> ().read_all_properties;
}

static void set_oasis_expect_strict_mode (db::LoadLayoutOptions *options, int f)
{
  options->get_options<db::OASISReaderOptions> ().expect_strict_mode = f;
}

static int get_oasis_expect_strict_mode (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::OASISReaderOptions> ().expect_strict_mode;
}

//  extend lay::LoadLayoutOptions with the OASIS options
static
gsi::ClassExt<db::LoadLayoutOptions> oasis_reader_options (
  gsi::method_ext ("oasis_read_all_properties=", &set_oasis_read_all_properties,
    //  this method is mainly provided as access point for the generic interface
    "@hide"
  ) +
  gsi::method_ext ("oasis_read_all_properties?", &get_oasis_read_all_properties,
    //  this method is mainly provided as access point for the generic interface
    "@hide"
  ) +
  gsi::method_ext ("oasis_expect_strict_mode=", &set_oasis_expect_strict_mode,
    //  this method is mainly provided as access point for the generic interface
    "@hide"
  ) +
  gsi::method_ext ("oasis_expect_strict_mode?", &get_oasis_expect_strict_mode,
    //  this method is mainly provided as access point for the generic interface
    "@hide"
  ),
  ""
);

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_oasis_compression (db::SaveLayoutOptions *options, int comp)
{
  options->get_options<db::OASISWriterOptions> ().compression_level = comp;
}

static int get_oasis_compression (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::OASISWriterOptions> ().compression_level;
}

static void set_oasis_recompress (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::OASISWriterOptions> ().recompress = f;
}

static bool get_oasis_recompress (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::OASISWriterOptions> ().recompress;
}

static void set_oasis_permissive (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::OASISWriterOptions> ().permissive = f;
}

static bool get_oasis_permissive (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::OASISWriterOptions> ().permissive;
}

static void set_oasis_write_std_properties (db::SaveLayoutOptions *options, bool f)
{
  db::OASISWriterOptions &oasis_options = options->get_options<db::OASISWriterOptions> ();
  if (f && oasis_options.write_std_properties == 0) {
    oasis_options.write_std_properties = 1;
  } else if (!f && oasis_options.write_std_properties != 0) {
    oasis_options.write_std_properties = 0;
  }
}

static bool get_oasis_write_std_properties (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::OASISWriterOptions> ().write_std_properties != 0;
}

static void set_oasis_write_std_properties_ext (db::SaveLayoutOptions *options, int f)
{
  options->get_options<db::OASISWriterOptions> ().write_std_properties = f;
}

static int get_oasis_write_std_properties_ext (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::OASISWriterOptions> ().write_std_properties;
}

static void set_oasis_write_cell_bounding_boxes (db::SaveLayoutOptions *options, bool f)
{
  db::OASISWriterOptions &oasis_options = options->get_options<db::OASISWriterOptions> ();
  if (f && oasis_options.write_std_properties < 2) {
    oasis_options.write_std_properties = 2;
  } else if (!f && oasis_options.write_std_properties >= 2) {
    oasis_options.write_std_properties = 1;
  }
}

static bool get_oasis_write_cell_bounding_boxes (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::OASISWriterOptions> ().write_std_properties >= 2;
}

static void set_oasis_write_cblocks (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::OASISWriterOptions> ().write_cblocks = f;
}

static bool get_oasis_write_cblocks (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::OASISWriterOptions> ().write_cblocks;
}

static void set_oasis_strict_mode (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<db::OASISWriterOptions> ().strict_mode = f;
}

static bool get_oasis_strict_mode (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::OASISWriterOptions> ().strict_mode;
}

static void set_oasis_subst_char (db::SaveLayoutOptions *options, const std::string &sc)
{
  options->get_options<db::OASISWriterOptions> ().subst_char = sc;
}

static std::string get_oasis_subst_char (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::OASISWriterOptions> ().subst_char;
}

//  extend lay::SaveLayoutOptions with the OASIS options
static
gsi::ClassExt<db::SaveLayoutOptions> oasis_writer_options (
  gsi::method_ext ("oasis_write_cblocks=", &set_oasis_write_cblocks, gsi::arg ("flag"),
    "@brief Sets a value indicating whether to write compressed CBLOCKS per cell\n"
    "Setting this property clears all format specific options for other formats such as GDS.\n"
  ) +
  gsi::method_ext ("oasis_write_cblocks?", &get_oasis_write_cblocks,
    "@brief Gets a value indicating whether to write compressed CBLOCKS per cell\n"
  ) +
  gsi::method_ext ("oasis_strict_mode=", &set_oasis_strict_mode, gsi::arg ("flag"),
    "@brief Sets a value indicating whether to write strict-mode OASIS files\n"
    "Setting this property clears all format specific options for other formats such as GDS.\n"
  ) +
  gsi::method_ext ("oasis_strict_mode?", &get_oasis_strict_mode,
    "@brief Gets a value indicating whether to write strict-mode OASIS files\n"
  ) +
  gsi::method_ext ("oasis_substitution_char=", &set_oasis_subst_char, gsi::arg ("char"),
    "@brief Sets the substitution character for a-strings and n-strings\n"
    "The substitution character is used in place of invalid characters. The value of this "
    "attribute is a string which is either empty or a single character. If the string is "
    "empty, no substitution is made at the risk of producing invalid OASIS files.\n"
    "\n"
    "This attribute has been introduce in version 0.23.\n"
  ) +
  gsi::method_ext ("oasis_substitution_char", &get_oasis_subst_char,
    "@brief Gets the substitution character\n"
    "\n"
    "See \\oasis_substitution_char for details. This attribute has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("oasis_recompress=", &set_oasis_recompress, gsi::arg ("flag"),
    "@brief Sets OASIS recompression mode\n"
    "If this flag is true, shape arrays already existing will be resolved and compression is applied "
    "to the individual shapes again. If this flag is false (the default), shape arrays already existing "
    "will be written as such.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("oasis_recompress?", &get_oasis_recompress,
    "@brief Gets the OASIS recompression mode\n"
    "See \\oasis_recompress= method for a description of this predicate."
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("oasis_permissive=", &set_oasis_permissive, gsi::arg ("flag"),
    "@brief Sets OASIS permissive mode\n"
    "If this flag is true, certain shapes which cannot be written to OASIS are reported as warnings, "
    "not as errors. For example, paths with odd width (are rounded) or polygons with less than three points (are skipped).\n"
    "\n"
    "This method has been introduced in version 0.25.1."
  ) +
  gsi::method_ext ("oasis_permissive?", &get_oasis_permissive,
    "@brief Gets the OASIS permissive mode\n"
    "See \\oasis_permissive= method for a description of this predicate."
    "\n"
    "This method has been introduced in version 0.25.1."
  ) +
  gsi::method_ext ("oasis_write_cell_bounding_boxes=", &set_oasis_write_cell_bounding_boxes, gsi::arg ("flag"),
    "@brief Sets a value indicating whether cell bounding boxes are written\n"
    "If this value is set to true, cell bounding boxes are written (S_BOUNDING_BOX). "
    "The S_BOUNDING_BOX properties will be attached to the CELLNAME records.\n"
    "\n"
    "Setting this value to true will also enable writing of other standard properties like "
    "S_TOP_CELL (see \\oasis_write_std_properties=).\n"
    "By default, cell bounding boxes are not written, but standard properties are.\n"
    "\n"
    "This method has been introduced in version 0.24.3."
  ) +
  gsi::method_ext ("oasis_write_cell_bounding_boxes?", &get_oasis_write_cell_bounding_boxes,
    "@brief Gets a value indicating whether cell bounding boxes are written\n"
    "See \\oasis_write_cell_bounding_boxes= method for a description of this flag."
    "\n"
    "This method has been introduced in version 0.24.3."
  ) +
  gsi::method_ext ("oasis_write_std_properties=", &set_oasis_write_std_properties, gsi::arg ("flag"),
    "@brief Sets a value indicating whether standard properties will be written\n"
    "If this value is false, no standard properties are written. If true, S_TOP_CELL and some other global "
    "standard properties are written. In addition, \\oasis_write_cell_bounding_boxes= can be used to "
    "write cell bounding boxes using S_BOUNDING_BOX.\n"
    "\n"
    "By default, this flag is true and standard properties are written.\n"
    "\n"
    "Setting this property to false clears the oasis_write_cell_bounding_boxes flag too.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::method_ext ("oasis_write_std_properties?", &get_oasis_write_std_properties,
    "@brief Gets a value indicating whether standard properties will be written\n"
    "See \\oasis_write_std_properties= method for a description of this flag."
    "\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::method_ext ("oasis_write_std_properties_ext=", &set_oasis_write_std_properties_ext,
    //  this method is mainly provided as access point for the generic interface
    "@hide"
  ) +
  gsi::method_ext ("oasis_write_std_properties_ext", &get_oasis_write_std_properties_ext,
    //  this method is mainly provided as access point for the generic interface
    "@hide"
  ) +
  gsi::method_ext ("oasis_compression_level=", &set_oasis_compression, gsi::arg ("level"),
    "@brief Set the OASIS compression level\n"
    "The OASIS compression level is an integer number between 0 and 10. 0 basically is no compression, "
    "1 produces shape arrays in a simple fashion. 2 and higher compression levels will use a more elaborate "
    "algorithm to find shape arrays which uses 2nd and further neighbor distances. The higher the level, the "
    "higher the memory requirements and run times.\n"
  ) +
  gsi::method_ext ("oasis_compression_level", &get_oasis_compression,
    "@brief Get the OASIS compression level\n"
    "See \\oasis_compression_level= method for a description of the OASIS compression level."
  ),
  ""
);

}


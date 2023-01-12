
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


#include "dbGDS2.h"
#include "dbGDS2Writer.h"
#include "dbGDS2Reader.h"
#include "dbSaveLayoutOptions.h"
#include "dbLoadLayoutOptions.h"

#include "gsiDecl.h"

namespace gsi
{

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_gds2_max_vertex_count (db::SaveLayoutOptions *options, unsigned int n)
{
  options->get_options<db::GDS2WriterOptions> ().max_vertex_count = n;
}

static unsigned int get_gds2_max_vertex_count (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::GDS2WriterOptions> ().max_vertex_count;
}

static void set_gds2_max_cellname_length (db::SaveLayoutOptions *options, unsigned int n)
{
  options->get_options<db::GDS2WriterOptions> ().max_cellname_length = n;
}

static unsigned int get_gds2_max_cellname_length (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::GDS2WriterOptions> ().max_cellname_length;
}

static void set_gds2_multi_xy_records (db::SaveLayoutOptions *options, bool n)
{
  options->get_options<db::GDS2WriterOptions> ().multi_xy_records = n;
}

static bool get_gds2_multi_xy_records (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::GDS2WriterOptions> ().multi_xy_records;
}

static void set_gds2_resolve_skew_arrays (db::SaveLayoutOptions *options, bool n)
{
  options->get_options<db::GDS2WriterOptions> ().resolve_skew_arrays = n;
}

static bool get_gds2_resolve_skew_arrays (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::GDS2WriterOptions> ().resolve_skew_arrays;
}

static void set_gds2_write_file_properties (db::SaveLayoutOptions *options, bool n)
{
  options->get_options<db::GDS2WriterOptions> ().write_file_properties = n;
}

static bool get_gds2_write_file_properties (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::GDS2WriterOptions> ().write_file_properties;
}

static void set_gds2_write_cell_properties (db::SaveLayoutOptions *options, bool n)
{
  options->get_options<db::GDS2WriterOptions> ().write_cell_properties = n;
}

static bool get_gds2_write_cell_properties (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::GDS2WriterOptions> ().write_cell_properties;
}

static void set_gds2_no_zero_length_paths (db::SaveLayoutOptions *options, bool n)
{
  options->get_options<db::GDS2WriterOptions> ().no_zero_length_paths = n;
}

static bool get_gds2_no_zero_length_paths (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::GDS2WriterOptions> ().no_zero_length_paths;
}

static void set_gds2_write_timestamps (db::SaveLayoutOptions *options, bool n)
{
  options->get_options<db::GDS2WriterOptions> ().write_timestamps = n;
}

static bool get_gds2_write_timestamps (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::GDS2WriterOptions> ().write_timestamps;
}

static void set_gds2_libname (db::SaveLayoutOptions *options, const std::string &n)
{
  options->get_options<db::GDS2WriterOptions> ().libname = n;
}

static std::string get_gds2_libname (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::GDS2WriterOptions> ().libname;
}

static void set_gds2_user_units (db::SaveLayoutOptions *options, double n)
{
  options->get_options<db::GDS2WriterOptions> ().user_units = n;
}

static double get_gds2_user_units (const db::SaveLayoutOptions *options)
{
  return options->get_options<db::GDS2WriterOptions> ().user_units;
}

//  extend lay::SaveLayoutOptions with the GDS2 options 
static
gsi::ClassExt<db::SaveLayoutOptions> gds2_writer_options (
  gsi::method_ext ("gds2_max_vertex_count=", &set_gds2_max_vertex_count, gsi::arg ("count"),
    "@brief Sets the maximum number of vertices for polygons to write\n"
    "This property describes the maximum number of point for polygons in GDS2 files.\n"
    "Polygons with more points will be split.\n"
    "The minimum value for this property is 4. The maximum allowed value is about 4000 or 8000, depending on the\n"
    "GDS2 interpretation. If \\gds2_multi_xy_records is true, this\n"
    "property is not used. Instead, the number of points is unlimited.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_max_vertex_count", &get_gds2_max_vertex_count,
    "@brief Gets the maximum number of vertices for polygons to write\n"
    "See \\gds2_max_vertex_count= method for a description of the maximum vertex count."
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_multi_xy_records=", &set_gds2_multi_xy_records, gsi::arg ("flag"),
    "@brief Uses multiple XY records in BOUNDARY elements for unlimited large polygons\n"
    "\n"
    "Setting this property to true allows producing polygons with an unlimited number of points \n"
    "at the cost of incompatible formats. Setting it to true disables the \\gds2_max_vertex_count setting.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_multi_xy_records?", &get_gds2_multi_xy_records,
    "@brief Gets the property enabling multiple XY records for BOUNDARY elements\n"
    "See \\gds2_multi_xy_records= method for a description of this property."
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_resolve_skew_arrays=", &set_gds2_resolve_skew_arrays, gsi::arg ("flag"),
    "@brief Resolves skew arrays into single instances\n"
    "\n"
    "Setting this property to true will make skew (non-orthogonal) arrays being resolved into single instances.\n"
    "Skew arrays happen if either the row or column vector isn't parallel to x or y axis. Such arrays can cause problems with "
    "some legacy software and can be disabled with this option.\n"
    "\nThis property has been added in version 0.27.1.\n"
  ) +
  gsi::method_ext ("gds2_resolve_skew_arrays?", &get_gds2_resolve_skew_arrays,
    "@brief Gets a value indicating whether to resolve skew arrays into single instances\n"
    "See \\gds2_resolve_skew_arrays= method for a description of this property."
    "\nThis property has been added in version 0.27.1.\n"
  ) +
  gsi::method_ext ("gds2_write_timestamps=", &set_gds2_write_timestamps, gsi::arg ("flag"),
    "@brief Writes the current time into the GDS2 timestamps if set to true\n"
    "\n"
    "If this property is set to false, the time fields will all be zero. This somewhat simplifies compare and diff "
    "applications.\n"
    "\n"
    "\nThis property has been added in version 0.21.16.\n"
  ) +
  gsi::method_ext ("gds2_write_timestamps?", &get_gds2_write_timestamps,
    "@brief Gets a value indicating whether the current time is written into the GDS2 timestamp fields\n"
    "\nThis property has been added in version 0.21.16.\n"
  ) +
  gsi::method_ext ("gds2_no_zero_length_paths=", &set_gds2_no_zero_length_paths, gsi::arg ("flag"),
    "@brief Eliminates zero-length paths if true\n"
    "\n"
    "If this property is set to true, paths with zero length will be converted to BOUNDARY objects.\n"
    "\n"
    "\nThis property has been added in version 0.23.\n"
  ) +
  gsi::method_ext ("gds2_no_zero_length_paths?|#gds2_no_zero_length_paths", &get_gds2_no_zero_length_paths,
    "@brief Gets a value indicating whether zero-length paths are eliminated\n"
    "\nThis property has been added in version 0.23.\n"
  ) +
  gsi::method_ext ("gds2_write_cell_properties=", &set_gds2_write_cell_properties, gsi::arg ("flag"),
    "@brief Enables writing of cell properties if set to true\n"
    "\n"
    "If this property is set to true, cell properties will be written as PROPATTR/PROPVALUE records immediately "
    "following the BGNSTR records. This is a non-standard extension and is therefore disabled by default.\n"
    "\n"
    "\nThis property has been added in version 0.23.\n"
  ) +
  gsi::method_ext ("gds2_write_cell_properties?|#gds2_write_cell_properties", &get_gds2_write_cell_properties,
    "@brief Gets a value indicating whether cell properties are written\n"
    "\nThis property has been added in version 0.23.\n"
  ) +
  gsi::method_ext ("gds2_write_file_properties=", &set_gds2_write_file_properties, gsi::arg ("flag"),
    "@brief Enables writing of file properties if set to true\n"
    "\n"
    "If this property is set to true, layout properties will be written as PROPATTR/PROPVALUE records immediately "
    "following the BGNLIB records. This is a non-standard extension and is therefore disabled by default.\n"
    "\n"
    "\nThis property has been added in version 0.24.\n"
  ) +
  gsi::method_ext ("gds2_write_file_properties?|#gds2_write_file_properties", &get_gds2_write_file_properties,
    "@brief Gets a value indicating whether layout properties are written\n"
    "\nThis property has been added in version 0.24.\n"
  ) +
  gsi::method_ext ("gds2_max_cellname_length=", &set_gds2_max_cellname_length, gsi::arg ("length"),
    "@brief Maximum length of cell names\n"
    "\n"
    "This property describes the maximum number of characters for cell names. \n"
    "Longer cell names will be shortened.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_max_cellname_length", &get_gds2_max_cellname_length,
    "@brief Get the maximum length of cell names\n"
    "See \\gds2_max_cellname_length= method for a description of the maximum cell name length."
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_libname=", &set_gds2_libname, gsi::arg ("libname"),
    "@brief Set the library name\n"
    "\n"
    "The library name is the string written into the LIBNAME records of the GDS file.\n"
    "The library name should not be an empty string and is subject to certain limitations in the character choice.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_libname", &get_gds2_libname,
    "@brief Get the library name\n"
    "See \\gds2_libname= method for a description of the library name."
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_user_units=", &set_gds2_user_units, gsi::arg ("uu"),
    "@brief Set the users units to write into the GDS file\n"
    "\n"
    "The user units of a GDS file are rarely used and usually are set to 1 (micron).\n"
    "The intention of the user units is to specify the display units. KLayout ignores the user unit and uses microns as the display unit.\n"
    "The user unit must be larger than zero.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_user_units", &get_gds2_user_units,
    "@brief Get the user units\n"
    "See \\gds2_user_units= method for a description of the user units."
    "\nThis property has been added in version 0.18.\n"
  ),
  ""
);

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_gds2_box_mode (db::LoadLayoutOptions *options, unsigned int n)
{
  options->get_options<db::GDS2ReaderOptions> ().box_mode = n;
}

static unsigned int get_gds2_box_mode (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::GDS2ReaderOptions> ().box_mode;
}

static void set_gds2_allow_multi_xy_records (db::LoadLayoutOptions *options, bool n)
{
  options->get_options<db::GDS2ReaderOptions> ().allow_multi_xy_records = n;
}

static bool get_gds2_allow_multi_xy_records (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::GDS2ReaderOptions> ().allow_multi_xy_records;
}

static void set_gds2_allow_big_records (db::LoadLayoutOptions *options, bool n)
{
  options->get_options<db::GDS2ReaderOptions> ().allow_big_records = n;
}

static bool get_gds2_allow_big_records (const db::LoadLayoutOptions *options)
{
  return options->get_options<db::GDS2ReaderOptions> ().allow_big_records;
}

//  extend lay::LoadLayoutOptions with the GDS2 options 
static
gsi::ClassExt<db::LoadLayoutOptions> gds2_reader_options (
  gsi::method_ext ("gds2_box_mode=", &set_gds2_box_mode,
    "@brief Sets a value specifying how to treat BOX records\n"
    "This property specifies how BOX records are treated.\n"
    "Allowed values are 0 (ignore), 1 (treat as rectangles), 2 (treat as boundaries) or 3 (treat as errors). The default is 1.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_box_mode", &get_gds2_box_mode,
    "@brief Gets a value specifying how to treat BOX records\n"
    "See \\gds2_box_mode= method for a description of this mode."
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_allow_multi_xy_records=", &set_gds2_allow_multi_xy_records,
    "@brief Allows the use of multiple XY records in BOUNDARY elements for unlimited large polygons\n"
    "\n"
    "Setting this property to true allows big polygons that span over multiple XY records.\n"
    "For strict compatibility with the standard, this property should be set to false. The default is true.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_allow_multi_xy_records?|#gds2_allow_multi_xy_records", &get_gds2_allow_multi_xy_records,
    "@brief Gets a value specifying whether to allow big polygons with multiple XY records.\n"
    "See \\gds2_allow_multi_xy_records= method for a description of this property."
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_allow_big_records=", &set_gds2_allow_big_records,
    "@brief Allows big records with more than 32767 bytes\n"
    "\n"
    "Setting this property to true allows larger records by treating the record length as unsigned short, which for example "
    "allows larger polygons (~8000 points rather than ~4000 points) without using multiple XY records.\n"
    "For strict compatibility with the standard, this property should be set to false. The default is true.\n"
    "\nThis property has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("gds2_allow_big_records?|#gds2_allow_big_records", &get_gds2_allow_big_records,
    "@brief Gets a value specifying whether to allow big records with a length of 32768 to 65535 bytes.\n"
    "See \\gds2_allow_big_records= method for a description of this property."
    "\nThis property has been added in version 0.18.\n"
  ),
  ""
);

}

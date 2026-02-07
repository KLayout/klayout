
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

#include "lstrFormat.h"
#include "dbLoadLayoutOptions.h"
#include "dbSaveLayoutOptions.h"
#include "gsiDecl.h"

namespace lstr
{

// ---------------------------------------------------------------
//  gsi Implementation of specific methods of LoadLayoutOptions

static void set_lstream_bbox_meta_info_key (db::LoadLayoutOptions *options, const std::string &key)
{
  options->get_options<lstr::ReaderOptions> ().bbox_meta_info_key = key;
}

static const std::string &get_lstream_bbox_meta_info_key (const db::LoadLayoutOptions *options)
{
  return options->get_options<lstr::ReaderOptions> ().bbox_meta_info_key;
}

//  extend lay::LoadLayoutOptions with the OASIS options
static
gsi::ClassExt<db::LoadLayoutOptions> lstream_reader_options (
  gsi::method_ext ("lstream_bbox_meta_info_key=", &set_lstream_bbox_meta_info_key, gsi::arg ("key"),
    "@brief If not an empty string, this attribute specifies the key under which the cell bounding box information is stored"
  ) +
  gsi::method_ext ("lstream_bbox_meta_info_key", &get_lstream_bbox_meta_info_key,
    "@brief If not an empty string, this attribute specifies the key under which the cell bounding box information is stored"
  ),
  ""
);

// ---------------------------------------------------------------
//  gsi Implementation of specific methods

static void set_lstream_compression (db::SaveLayoutOptions *options, int comp)
{
  options->get_options<lstr::WriterOptions> ().compression_level = comp;
}

static int get_lstream_compression (const db::SaveLayoutOptions *options)
{
  return options->get_options<lstr::WriterOptions> ().compression_level;
}

static void set_lstream_recompress (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<lstr::WriterOptions> ().recompress = f;
}

static bool get_lstream_recompress (const db::SaveLayoutOptions *options)
{
  return options->get_options<lstr::WriterOptions> ().recompress;
}

static void set_lstream_permissive (db::SaveLayoutOptions *options, bool f)
{
  options->get_options<lstr::WriterOptions> ().permissive = f;
}

static bool get_lstream_permissive (const db::SaveLayoutOptions *options)
{
  return options->get_options<lstr::WriterOptions> ().permissive;
}

//  extend lay::SaveLayoutOptions with the OASIS options
static
gsi::ClassExt<db::SaveLayoutOptions> lstream_writer_options (
  gsi::method_ext ("lstream_recompress=", &set_lstream_recompress, gsi::arg ("flag"),
    "@brief Sets LStream recompression mode\n"
    "If this flag is true, shape arrays already existing will be resolved and compression is applied "
    "to the individual shapes again. If this flag is false (the default), shape arrays already existing "
    "will be written as such.\n"
  ) +
  gsi::method_ext ("lstream_recompress?", &get_lstream_recompress,
    "@brief Gets the LStream recompression mode\n"
    "See \\oasis_recompress= method for a description of this predicate."
  ) +
  gsi::method_ext ("lstream_permissive=", &set_lstream_permissive, gsi::arg ("flag"),
    "@brief Sets LStream permissive mode\n"
    "If this flag is true, certain shapes which cannot be written to LStream are reported as warnings, "
    "not as errors. For example, paths with odd width (are rounded).\n"
  ) +
  gsi::method_ext ("lstream_permissive?", &get_lstream_permissive,
    "@brief Gets the LStream permissive mode\n"
    "See \\oasis_permissive= method for a description of this predicate."
  ) +
  gsi::method_ext ("lstream_compression_level=", &set_lstream_compression, gsi::arg ("level"),
    "@brief Set the LStream compression level\n"
    "The LStream compression level is an integer number between 0 and 10. 0 basically is no compression, "
    "1 produces shape arrays in a simple fashion. 2 and higher compression levels will use a more elaborate "
    "algorithm to find shape arrays which uses 2nd and further neighbor distances. The higher the level, the "
    "higher the memory requirements and run times.\n"
  ) +
  gsi::method_ext ("lstream_compression_level", &get_lstream_compression,
    "@brief Get the LStream compression level\n"
    "See \\oasis_compression_level= method for a description of the LStream compression level."
  ),
  ""
);

}


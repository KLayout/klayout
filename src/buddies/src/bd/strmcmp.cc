
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "bdReaderOptions.h"
#include "dbLayout.h"
#include "dbLayoutDiff.h"
#include "dbReader.h"
#include "tlCommandLineParser.h"
#include "tlTimer.h"

BD_PUBLIC int strmcmp (int argc, char *argv[])
{
  bd::GenericReaderOptions generic_reader_options_a;
  generic_reader_options_a.set_prefix ("a");
  generic_reader_options_a.set_long_prefix ("a-");
  generic_reader_options_a.set_group_prefix ("Input A");

  bd::GenericReaderOptions generic_reader_options_b;
  generic_reader_options_b.set_prefix ("b");
  generic_reader_options_b.set_long_prefix ("b-");
  generic_reader_options_b.set_group_prefix ("Input B");

  std::string infile_a, infile_b;
  std::string top_a, top_b;
  bool silent = false;
  bool ignore_duplicates = false;
  bool no_text_orientation = true;
  bool no_text_details = true;
  bool no_properties = false;
  bool no_layer_names = false;
  bool verbose = true;
  bool as_polygons = false;
  bool boxes_as_polygons = false;
  bool flatten_array_insts = false;
  bool smart_cell_mapping = false;
  bool paths_as_polygons = false;
  bool dont_summarize_missing_layers = false;
  double tolerance = 0.0;
  int max_count = 0;
  bool print_properties = false;

  tl::CommandLineOptions cmd;
  generic_reader_options_a.add_options (cmd);
  generic_reader_options_b.add_options (cmd);

  cmd << tl::arg ("input_a",                   &infile_a,   "The first input file (any format, may be gzip compressed)")
      << tl::arg ("input_b",                   &infile_b,   "The second input file (any format, may be gzip compressed)")
      << tl::arg ("-ta|--top-a=name",          &top_a,      "Specifies the cell to take as top cell from the first layout",
                  "Use this option to take a specific cell as the top cell from the first layout. All "
                  "cells not called directly or indirectly from this cell are ignored. If you use this option, "
                  "--top-b must be specified too and can be different from the first layout's top cell."
                 )
      << tl::arg ("-tb|--top-b=name",          &top_b,      "Specifies the cell to take as top cell from the second layout",
                  "See --top-a for details."
                 )
      << tl::arg ("-s|--silent",               &silent,     "Enables silent mode",
                  "In silent mode, no differences are printed, but the exit code indicates whether "
                  "the layouts are the same (0) or differences exist (> 0)."
                 )
      << tl::arg ("#!--with-text-orientation", &no_text_orientation, "Compares orientations for texts",
                  "With this option, text orientation is compared too. The position of the "
                  "text is always compared, but the rotation angle is compared only when this option "
                  "is present."
                 )
      << tl::arg ("#!--with-text-details",     &no_text_details, "Compares font and alignment for texts",
                  "With this option, text font and alignment is compared too."
                 )
      << tl::arg ("-np|--without-properties",  &no_properties, "Ignores properties",
                  "With this option, shape, cell and file properties are not compared."
                 )
      << tl::arg ("-nl|--without-layer-names", &no_layer_names, "Ignores layer names",
                  "With this option, layer names are not compared."
                 )
      << tl::arg ("!-u|--terse",               &verbose,    "Skips too many details",
                  "With this option, no details about differences are printed."
                 )
      << tl::arg ("-r|--print-properties",     &print_properties, "Prints shape properties too",
                  "This option, shape properties are printed too."
                 )
      << tl::arg ("-p|--as-polygons",          &as_polygons, "Compares shapes are polygons",
                  "This option is equivalent to using --boxes-as-polygons and --paths-as-polygons."
                 )
      << tl::arg ("--boxes-as-polygons",       &boxes_as_polygons, "Turns boxes into polygons before compare",
                  "With this option, boxes and equivalent polygons are treated identical."
                 )
      << tl::arg ("--paths-as-polygons",       &paths_as_polygons, "Turns paths into polygons before compare",
                  "With this option, paths and equivalent polygons are treated identical."
                 )
      << tl::arg ("--expand-arrays",           &flatten_array_insts, "Expands array instances before compare",
                  "With this option, arrays are equivalent single instances are treated identical."
                 )
      << tl::arg ("-1|--ignore-duplicates",    &ignore_duplicates, "Ignore duplicate instances and shapes",
                  "With this option, duplicate instances or shapes are ignored and duplication "
                  "does not count as a difference."
                 )
      << tl::arg ("-l|--layer-details",        &dont_summarize_missing_layers, "Prints details about differences for missing layers",
                  "With this option, missing layers are treated as \"empty\" and details about differences to "
                  "other, non-empty layers are printed. Essentially the content of the non-empty counterpart "
                  "is printed. Without this option, missing layers are treated as a single difference of type "
                  "\"missing layer\"."
                 )
      << tl::arg ("-c|--cell-mapping",         &smart_cell_mapping, "Attempts to identify cells by their properties",
                  "If this option is given, the algorithm will try to identify identical cells by their "
                  "geometrical properties (placement, size etc.) instead of their name. This way, cell renaming can "
                  "be detected"
                 )
      << tl::arg ("-t|--tolerance=value",      &tolerance, "Specifies a tolerance for geometry compare",
                  "If this value is given, shape comparison allows for this tolerance when comparing "
                  "coordinates. The tolerance value is given in micrometer units."
                 )
      << tl::arg ("-m|--max-count=value",      &max_count, "Specifies the maximum number of differences to report",
                  "If the value is 1, only a warning saying that the log has been abbreviated is printed. "
                  "If the value is >1, max-count-1 differences plus one warning about abbreviation is printed. "
                  "A value of 0 means \"no limitation\". To suppress all output, use --silent."
                 )
    ;

  cmd.brief ("This program will compare two layout files on a per-object basis");

  cmd.parse (argc, argv);

  if (top_a.empty () != top_b.empty ()) {
    throw tl::Exception ("Both -ta|--top-a and -tb|--top-b top cells must be given");
  }

  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Total")));

  db::Layout layout_a;
  db::Layout layout_b;

  {
    db::LoadLayoutOptions load_options;
    generic_reader_options_a.configure (load_options);
    bd::read_files (layout_a, infile_a, load_options);
  }

  {
    db::LoadLayoutOptions load_options;
    generic_reader_options_b.configure (load_options);
    bd::read_files (layout_b, infile_b, load_options);
  }

  unsigned int flags = 0;
  if (silent) {
    flags |= db::layout_diff::f_silent;
  }
  if (ignore_duplicates) {
    flags |= db::layout_diff::f_ignore_duplicates;
  }
  if (no_text_orientation) {
    flags |= db::layout_diff::f_no_text_orientation;
  }
  if (no_text_details) {
    flags |= db::layout_diff::f_no_text_details;
  }
  if (no_properties) {
    flags |= db::layout_diff::f_no_properties;
  }
  if (no_layer_names) {
    flags |= db::layout_diff::f_no_layer_names;
  }
  if (verbose) {
    flags |= db::layout_diff::f_verbose;
  }
  if (as_polygons || boxes_as_polygons) {
    flags |= db::layout_diff::f_boxes_as_polygons;
  }
  if (as_polygons || paths_as_polygons) {
    flags |= db::layout_diff::f_paths_as_polygons;
  }
  if (flatten_array_insts) {
    flags |= db::layout_diff::f_flatten_array_insts;
  }
  if (smart_cell_mapping) {
    flags |= db::layout_diff::f_smart_cell_mapping;
  }
  if (dont_summarize_missing_layers) {
    flags |= db::layout_diff::f_dont_summarize_missing_layers;
  }

  db::Coord tolerance_dbu = db::coord_traits<db::Coord>::rounded (tolerance / std::min (layout_a.dbu (), layout_b.dbu ()));
  bool result = false;

  if (smart_cell_mapping && top_a.empty ()) {

    db::Layout::top_down_const_iterator t;

    t = layout_a.begin_top_down ();
    if (t != layout_a.end_top_cells ()) {
      top_a = layout_a.cell_name (*t);
      ++t;
      if (t != layout_a.end_top_cells ()) {
        throw tl::Exception ("Top cell of first layout is not unique which is required for -c|--cell-mapping");
      }
    }

    t = layout_b.begin_top_down ();
    if (t != layout_b.end_top_cells ()) {
      top_b = layout_b.cell_name (*t);
      ++t;
      if (t != layout_b.end_top_cells ()) {
        throw tl::Exception ("Top cell of second layout is not unique which is required for -c|--cell-mapping");
      }
    }

  }

  if (! top_a.empty ()) {

    std::pair<bool, db::cell_index_type> index_a = layout_a.cell_by_name (top_a.c_str ());
    std::pair<bool, db::cell_index_type> index_b = layout_b.cell_by_name (top_b.c_str ());

    if (! index_a.first) {
      throw tl::Exception ("'" + top_a + "' is not a valid cell name in first layout");
    }
    if (! index_b.first) {
      throw tl::Exception ("'" + top_b + "' is not a valid cell name in second layout");
    }

    result = db::compare_layouts (layout_a, index_a.second, layout_b, index_b.second, flags, tolerance_dbu, max_count, print_properties);

  } else {
    result = db::compare_layouts (layout_a, layout_b, flags, tolerance_dbu, max_count, print_properties);
  }

  if (! result && ! silent) {
    tl::error << "Layouts differ";
  }

  return result ? 0 : 1;
}

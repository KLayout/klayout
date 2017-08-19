
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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
#include "dbTilingProcessor.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbSaveLayoutOptions.h"
#include "tlCommandLineParser.h"

BD_PUBLIC int strmxor (int argc, char *argv[])
{
  bd::GenericReaderOptions generic_reader_options_a;
  generic_reader_options_a.set_prefix ("a");
  generic_reader_options_a.set_long_prefix ("a-");
  generic_reader_options_a.set_group_prefix ("Input A");

  bd::GenericReaderOptions generic_reader_options_b;
  generic_reader_options_b.set_prefix ("b");
  generic_reader_options_b.set_long_prefix ("b-");
  generic_reader_options_b.set_group_prefix ("Input B");

  std::string infile_a, infile_b, output;
  std::string top_a, top_b;
  bool dont_summarize_missing_layers = false;
  bool silent = false;
  std::vector<double> tolerances;
  int tolerance_bump = 10000;
  int threads = 1;
  double tile_size = 0.0;

  tl::CommandLineOptions cmd;
  generic_reader_options_a.add_options (cmd);
  generic_reader_options_b.add_options (cmd);

  cmd << tl::arg ("input_a",                   &infile_a,   "The first input file (any format, may be gzip compressed)")
      << tl::arg ("input_b",                   &infile_b,   "The second input file (any format, may be gzip compressed)")
      << tl::arg ("?output",                   &output,     "The output file to which the XOR differences are written",
                  "This argument is optional. If not given, the exit status along indicates whether the layouts "
                  "are identical or not."
                 )
      << tl::arg ("-ta|--top-a=name",          &top_a,      "Specifies the cell to take as top cell from the first layout",
                  "Use this option to take a specific cell as the top cell from the first layout. All "
                  "cells not called directly or indirectly from this cell are ignored. If you use this option, "
                  "--top-b must be specified too and can be different from the first layout's top cell."
                 )
      << tl::arg ("-tb|--top-b=name",          &top_b,      "Specifies the cell to take as top cell from the second layout",
                  "See --top-a for details."
                 )
      << tl::arg ("-s|--silent",               &silent,     "Enables silent mode",
                  "In silent mode, no summary is printed, but the exit code indicates whether "
                  "the layouts are the same (0) or differences exist (> 0)."
                 )
      << tl::arg ("-l|--layer-details",        &dont_summarize_missing_layers, "Output details about differences for missing layers",
                  "With this option, missing layers are treated as \"empty\" and the whole layer of the other "
                  "layout is output. Without this option, a message is printed for missing layers instead."
                 )
      << tl::arg ("-t|--tolerance=values",     &tolerances, "Specifies tolerances for the geometry compare",
                  "This option can take multiple tolerance values. The values are given in micrometer units and "
                  "are separated by a comma. If a tolerance is given, XOR differences are "
                  "only reported when they are larger than the tolerance value. Tolerance values must be given in "
                  "ascending order."
                 )
      << tl::arg ("-n|--threads=threads",      &threads,   "Specifies the number of threads to use",
                  "If given, multiple threads are used for the XOR computation. This way, multiple cores can "
                  "be utilized."
                 )
      << tl::arg ("-p|--tiles=size",           &tile_size, "Specifies tiling mode",
                  "In tiling mode, the layout is divided into tiles of the given size. Each tile is computed "
                  "individually. Multiple tiles can be processed in parallel on multiple cores."
                 )
      << tl::arg ("-b|--layer-bump=offset",    &tolerance_bump, "Specifies the layer number offset to add for every tolerance",
                  "This value is the number added to the original layer number to form a layer set for each tolerance "
                  "value. If this value is set to 1000, the first tolerance value will produce XOR results on the "
                  "original layers. A second tolerance value will produce XOR results on the original layers + 1000. "
                  "A third tolerance value will produce XOR results on the original layers + 2000."
                 )
    ;

  cmd.brief ("This program will compare two layout files with a geometrical XOR operation");

  cmd.parse (argc, argv);

  if (top_a.empty () != top_b.empty ()) {
    throw tl::Exception ("Both -ta|--top-a and -tb|--top-b top cells must be given");
  }

  if (tolerances.empty ()) {
    tolerances.push_back (0.0);
  } else {
    for (std::vector<double>::const_iterator t = tolerances.begin () + 1; t != tolerances.end (); ++t) {
      if (*(t - 1) > *t - db::epsilon) {
        throw tl::Exception ("Tolerance values (-t|--tolerances) must be given in ascending order");
      }
    }
  }

  db::Layout layout_a;
  db::Layout layout_b;

  {
    db::LoadLayoutOptions load_options;
    generic_reader_options_a.configure (load_options);

    tl::InputStream stream (infile_a);
    db::Reader reader (stream);
    reader.read (layout_a, load_options);
  }

  {
    db::LoadLayoutOptions load_options;
    generic_reader_options_b.configure (load_options);

    tl::InputStream stream (infile_b);
    db::Reader reader (stream);
    reader.read (layout_b, load_options);
  }

  if (top_a.empty ()) {

    db::Layout::top_down_const_iterator t;

    t = layout_a.begin_top_down ();
    if (t != layout_a.end_top_cells ()) {
      top_a = layout_a.cell_name (*t);
      ++t;
      if (t != layout_a.end_top_cells ()) {
        throw tl::Exception ("Top cell of first layout is not unique and cannot be determined automatically");
      }
    }

    t = layout_b.begin_top_down ();
    if (t != layout_b.end_top_cells ()) {
      top_b = layout_b.cell_name (*t);
      ++t;
      if (t != layout_b.end_top_cells ()) {
        throw tl::Exception ("Top cell of second layout is not unique and cannot be determined automatically");
      }
    }

  }

  std::pair<bool, db::cell_index_type> index_a = layout_a.cell_by_name (top_a.c_str ());
  std::pair<bool, db::cell_index_type> index_b = layout_b.cell_by_name (top_b.c_str ());

  if (! index_a.first) {
    throw tl::Exception ("'" + top_a + "' is not a valid cell name in first layout");
  }
  if (! index_b.first) {
    throw tl::Exception ("'" + top_b + "' is not a valid cell name in second layout");
  }

  std::map<db::LayerProperties, std::pair<int, int> > l2l_map;

  for (db::Layout::layer_iterator l = layout_a.begin_layers (); l != layout_a.end_layers (); ++l) {
    l2l_map.insert (std::make_pair (*(*l).second, std::make_pair (-1, -1))).first->second.first = (*l).first;
  }
  for (db::Layout::layer_iterator l = layout_b.begin_layers (); l != layout_b.end_layers (); ++l) {
    l2l_map.insert (std::make_pair (*(*l).second, std::make_pair (-1, -1))).first->second.second = (*l).first;
  }

  bool result = true;

  db::TilingProcessor proc;
  proc.set_dbu (std::min (layout_a.dbu (), layout_b.dbu ()));
  proc.set_threads (std::max (1, threads));
  if (tile_size > db::epsilon) {
    proc.tile_size (tile_size, tile_size);
  }
  proc.tile_border (tolerances.back () * 2.0, tolerances.back () * 2.0);

  db::Layout output_layout;
  output_layout.dbu (proc.dbu ());

  db::cell_index_type output_top = output_layout.add_cell ("XOR");

  std::vector<unsigned int> output_layers;

  int index = 1;

  for (std::map<db::LayerProperties, std::pair<int, int> >::const_iterator ll = l2l_map.begin (); ll != l2l_map.end (); ++ll) {

    if (ll->second.first < 0 && ! dont_summarize_missing_layers) {

      tl::warn << "Layer " << ll->first.to_string () << " is not present in first layout, but in second";
      result = false;

    } else if (ll->second.second < 0 && ! dont_summarize_missing_layers) {

      tl::warn << "Layer " << ll->first.to_string () << " is not present in second layout, but in first";
      result = false;

    } else {

      std::string in_a = "a" + tl::to_string (index);
      std::string in_b = "b" + tl::to_string (index);

      if (ll->second.first < 0) {
        proc.input (in_a, db::RecursiveShapeIterator ());
      } else {
        proc.input (in_a, db::RecursiveShapeIterator (layout_a, layout_a.cell (index_a.second), ll->second.first));
      }

      if (ll->second.second < 0) {
        proc.input (in_b, db::RecursiveShapeIterator ());
      } else {
        proc.input (in_b, db::RecursiveShapeIterator (layout_b, layout_b.cell (index_b.second), ll->second.second));
      }

      std::string expr = "var x=" + in_a + "^" + in_b + "; ";

      int tol_index = 1;
      for (std::vector<double>::const_iterator t = tolerances.begin (); t != tolerances.end (); ++t) {

        std::string out = "o" + tl::to_string (index) + "_" + tl::to_string (tol_index);

        db::LayerProperties lp = ll->first;
        if (lp.layer >= 0) {
          lp.layer += (tol_index - 1) * tolerance_bump;
        }

        unsigned int output_layer = output_layout.insert_layer (lp);
        output_layers.push_back (output_layer);

        // @@@ TODO: silent mode
        proc.output (out, output_layout, output_top, output_layer);

        if (*t > db::epsilon) {
          expr += "x=x.sized(-int(" + tl::to_string (*t) + "/_dbu)/2).sized(int(" + tl::to_string (*t) + "/_dbu)/2); ";
        }
        expr += "_output(" + out + ",x); ";

      }

      proc.queue (expr);

    }

    ++index;

  }

  //  Runs the processor

  proc.execute ("Running XOR");

  //  Write the output layout

  if (! output.empty()) {

    db::SaveLayoutOptions save_options;
    save_options.set_format_from_filename (output);

    tl::OutputStream stream (output);
    db::Writer writer (save_options);
    writer.write (output_layout, stream);

  }

  //  Determine the output status based on the output top cell's emptyness

  for (std::vector<unsigned int>::const_iterator l = output_layers.begin (); l != output_layers.end () && result; ++l) {
    result = output_layout.cell (output_top).bbox (*l).empty ();
  }

  //  @@@ TODO: print a nice summary unless "silent" is set

  return result ? 0 : 1;
}


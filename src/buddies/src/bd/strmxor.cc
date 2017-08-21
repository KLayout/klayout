
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
#include "gsiExpression.h"
#include "tlCommandLineParser.h"

class CountingInserter
{
public:
  CountingInserter ()
    : m_count (0)
  {
    //  .. nothing yet ..
  }

  template <class T>
  void operator() (const T & /*t*/)
  {
    m_count += 1;
  }

  size_t count () const
  {
    return m_count;
  }

private:
  size_t m_count;
};

class CountingReceiver
  : public db::TileOutputReceiver
{
public:
  CountingReceiver ()
    : m_count (0)
  {
    //  .. nothing yet ..
  }

  virtual void put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans & /*trans*/, bool clip)
  {
    CountingInserter inserter;
    db::insert_var (inserter, obj, tile, clip);
    m_count += inserter.count ();
  }

  size_t count () const
  {
    return m_count;
  }

private:
  size_t m_count;
};

struct ResultDescriptor
{
  ResultDescriptor ()
    : layer_a (-1), layer_b (-1), layer_output (-1), layout (0), top_cell (0)
  {
    //  .. nothing yet ..
  }

  tl::shared_ptr<CountingReceiver> counter;
  int layer_a;
  int layer_b;
  int layer_output;
  db::Layout *layout;
  db::cell_index_type top_cell;

  size_t count () const
  {
    if (layout && layer_output >= 0) {
      //  NOTE: this assumes the output is flat
      tl_assert (layout->cells () == 1);
      return layout->cell (top_cell).shapes (layer_output).size ();
    } else if (counter) {
      return counter->count ();
    } else {
      return 0;
    }
  }

  bool is_empty () const
  {
    if (layout && layer_output >= 0) {
      //  NOTE: this assumes the output is flat
      tl_assert (layout->cells () == 1);
      return layout->cell (top_cell).shapes (layer_output).empty ();
    } else if (counter) {
      return counter->count () == 0;
    } else {
      return true;
    }
  }
};

BD_PUBLIC int strmxor (int argc, char *argv[])
{
  gsi::initialize_expressions ();

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
  bool no_summary = false;
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
                  "This argument is optional. If not given, the exit status alone will indicate whether the layouts "
                  "are identical or not."
                 )
      << tl::arg ("-ta|--top-a=name",          &top_a,      "Specifies the top cell for the first layout",
                  "Use this option to take a specific cell as the top cell from the first layout. All "
                  "cells not called directly or indirectly from this cell are ignored. If you use this option, "
                  "--top-b must be specified too and can be different from the first layout's top cell."
                 )
      << tl::arg ("-tb|--top-b=name",          &top_b,      "Specifies the top cell for the second layout",
                  "See --top-a for details."
                 )
      << tl::arg ("-s|--silent",               &silent,     "Silent mode",
                  "In silent mode, no summary is printed, but the exit code indicates whether "
                  "the layouts are the same (0) or differences exist (> 0)."
                 )
      << tl::arg ("#--no-summary",             &no_summary, "Don't print a summary")
      << tl::arg ("-l|--layer-details",        &dont_summarize_missing_layers, "Treats missing layers as empty",
                  "With this option, missing layers are treated as \"empty\" and the whole layer of the other "
                  "layout is output. Without this option, a message is printed for missing layers instead and the "
                  "layer from the other layout is ignored."
                 )
      << tl::arg ("-t|--tolerances=values",     &tolerances, "Specifies tolerances for the geometry compare",
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

  db::TilingProcessor proc;
  proc.set_dbu (std::min (layout_a.dbu (), layout_b.dbu ()));
  proc.set_threads (std::max (1, threads));
  if (tile_size > db::epsilon) {
    if (tl::verbosity () >= 20) {
      tl::log << "Tile size: " << tile_size;
    }
    proc.tile_size (tile_size, tile_size);
  }

  proc.tile_border (tolerances.back () * 2.0, tolerances.back () * 2.0);
  if (tl::verbosity () >= 20) {
    tl::log << "Tile border: " << tolerances.back () * 2.0;
  }

  if (tl::verbosity () >= 20) {
    tl::log << "Database unit: " << proc.dbu ();
    tl::log << "Threads: " << threads;
    tl::log << "Layer bump for tolerance: " << tolerance_bump;
  }

  std::auto_ptr<db::Layout> output_layout;
  db::cell_index_type output_top = 0;

  if (! output.empty ()) {
    output_layout.reset (new db::Layout ());
    output_layout->dbu (proc.dbu ());
    output_top = output_layout->add_cell ("XOR");
  }

  std::map<std::pair<int, db::LayerProperties>, ResultDescriptor> results;

  bool result = true;

  int index = 1;

  for (std::map<db::LayerProperties, std::pair<int, int> >::const_iterator ll = l2l_map.begin (); ll != l2l_map.end (); ++ll) {

    if ((ll->second.first < 0 || ll->second.second < 0) && ! dont_summarize_missing_layers) {

      if (ll->second.first < 0) {
        (silent ? tl::log : tl::warn) << "Layer " << ll->first.to_string () << " is not present in first layout, but in second";
      } else {
        (silent ? tl::log : tl::warn) << "Layer " << ll->first.to_string () << " is not present in second layout, but in first";
      }

      result = false;

      int tol_index = 0;
      for (std::vector<double>::const_iterator t = tolerances.begin (); t != tolerances.end (); ++t) {

        ResultDescriptor &result = results.insert (std::make_pair (std::make_pair (tol_index, ll->first), ResultDescriptor ())).first->second;
        result.layer_a = ll->second.first;
        result.layer_b = ll->second.second;
        result.layout = output_layout.get ();
        result.top_cell = output_top;

        ++tol_index;

      }

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

      int tol_index = 0;
      for (std::vector<double>::const_iterator t = tolerances.begin (); t != tolerances.end (); ++t) {

        std::string out = "o" + tl::to_string (index) + "_" + tl::to_string (tol_index + 1);

        db::LayerProperties lp = ll->first;
        if (lp.layer >= 0) {
          lp.layer += tol_index * tolerance_bump;
        }

        ResultDescriptor &result = results.insert (std::make_pair (std::make_pair (tol_index, ll->first), ResultDescriptor ())).first->second;
        result.layer_a = ll->second.first;
        result.layer_b = ll->second.second;
        result.layout = output_layout.get ();
        result.top_cell = output_top;

        if (result.layout) {
          result.layer_output = result.layout->insert_layer (lp);
          proc.output (out, *result.layout, result.top_cell, result.layer_output);
        } else {
          CountingReceiver *counter = new CountingReceiver ();
          result.counter = counter;
          proc.output (out, 0, counter, db::ICplxTrans ());
        }

        if (*t > db::epsilon) {
          expr += "x=x.sized(-round(" + tl::to_string (*t) + "/_dbu)/2).sized(round(" + tl::to_string (*t) + "/_dbu)/2); ";
        }
        expr += "_output(" + out + ",x); ";

        ++tol_index;

      }

      if (tl::verbosity () >= 20) {
        tl::log << "Running expression: '" << expr << "' for layer " << ll->first;
      }
      proc.queue (expr);

    }

    ++index;

  }

  //  Runs the processor

  if ((! silent && ! no_summary) || result || output_layout.get ()) {
    proc.execute ("Running XOR");
  }

  //  Writes the output layout

  if (output_layout.get ()) {

    db::SaveLayoutOptions save_options;
    save_options.set_format_from_filename (output);

    tl::OutputStream stream (output);
    db::Writer writer (save_options);
    writer.write (*output_layout, stream);

  }

  //  Determines the output status
  for (std::map<std::pair<int, db::LayerProperties>, ResultDescriptor>::const_iterator r = results.begin (); r != results.end () && result; ++r) {
    result = r->second.is_empty ();
  }

  if (! silent && ! no_summary) {

    if (result) {
      tl::info << "No differences found";
    } else {

      const char *line_format = "  %-10s %s";
      const char *sep = "  -------------------------------------------------------";

      tl::info << "Result summary (layers without differences are not shown):" << tl::endl;
      tl::info << tl::sprintf (line_format, "Layer", "Differences (shape count)") << tl::endl << sep;

      int ti = -1;
      for (std::map<std::pair<int, db::LayerProperties>, ResultDescriptor>::const_iterator r = results.begin (); r != results.end (); ++r) {

        if (r->first.first != ti) {
          ti = r->first.first;
          if (tolerances[ti] > db::epsilon) {
            tl::info << tl::endl << "Tolerance " << tl::micron_to_string (tolerances[ti]) << ":" << tl::endl;
            tl::info << tl::sprintf (line_format, "Layer", "Differences (shape count)") << tl::endl << sep;
          }
        }

        if (r->second.layer_a < 0 && ! dont_summarize_missing_layers) {
          tl::info << tl::sprintf (line_format, r->first.second.to_string (), "(no such layer in first layout)");
        } else if (r->second.layer_b < 0 && ! dont_summarize_missing_layers) {
          tl::info << tl::sprintf (line_format, r->first.second.to_string (), "(no such layer in second layout)");
        } else if (! r->second.is_empty ()) {
          tl::info << tl::sprintf (line_format, r->first.second.to_string (), tl::to_string (r->second.count ()));
        }

      }

      tl::info << "";

    }

  }

  return result ? 0 : 1;
}


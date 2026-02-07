
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
#include "bdWriterOptions.h"
#include "dbLayout.h"
#include "dbTilingProcessor.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbSaveLayoutOptions.h"
#include "dbRegion.h"
#include "dbDeepShapeStore.h"
#include "dbCellGraphUtils.h"
#include "gsiExpression.h"
#include "tlCommandLineParser.h"
#include "tlThreads.h"
#include "tlThreadedWorkers.h"
#include "tlTimer.h"
#include "tlOptional.h"

namespace {

// ---------------------------------------------------------------------

class HealingCountingReceiver
  : public db::TileOutputReceiver
{
public:
  HealingCountingReceiver (size_t *count, bool healing);

  virtual void put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans & /*trans*/, bool clip);
  virtual void finish (bool);

  void keep_for_healing (const db::Polygon &poly);
  void keep_for_healing (const db::Box &box);

private:
  size_t *mp_count;
  db::Region m_for_healing;
  bool m_healing;
};

class HealingCountingInserter
{
public:
  HealingCountingInserter (const db::Box &tile, bool healing, HealingCountingReceiver *rec)
    : m_count (0), mp_tile (&tile), m_healing (healing), mp_receiver (rec)
  {
    //  .. nothing yet ..
  }

  template <class T>
  void operator() (const T & /*t*/)
  {
    m_count += 1;
  }

  void operator() (const db::Polygon &poly)
  {
    if (m_healing && ! poly.box ().inside (mp_tile->enlarged (db::Vector (-1, -1)))) {
      mp_receiver->keep_for_healing (poly);
    } else {
      m_count += 1;
    }
  }

  void operator() (const db::Box &box)
  {
    if (m_healing && ! box.inside (mp_tile->enlarged (db::Vector (-1, -1)))) {
      mp_receiver->keep_for_healing (box);
    } else {
      m_count += 1;
    }
  }

  size_t count () const
  {
    return m_count;
  }

private:
  size_t m_count;
  const db::Box *mp_tile;
  bool m_healing;
  HealingCountingReceiver *mp_receiver;
};

HealingCountingReceiver::HealingCountingReceiver (size_t *count, bool healing)
  : mp_count (count), m_healing (healing)
{
  //  .. nothing yet ..
}

void
HealingCountingReceiver::put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans & /*trans*/, bool clip)
{
  HealingCountingInserter inserter (tile, m_healing, this);
  db::insert_var (inserter, obj, tile, clip);
  *mp_count += inserter.count ();
}

void
HealingCountingReceiver::keep_for_healing (const db::Polygon &poly)
{
  m_for_healing.insert (poly);
}

void
HealingCountingReceiver::keep_for_healing (const db::Box &box)
{
  m_for_healing.insert (box);
}

void
HealingCountingReceiver::finish (bool)
{
  if (m_healing) {
    *mp_count += m_for_healing.merged ().count ();
  }
}

// ---------------------------------------------------------------------

class HealingTileLayoutOutputReceiver
  : public db::TileOutputReceiver
{
public:
  HealingTileLayoutOutputReceiver (db::Layout *layout, db::Cell *cell, unsigned int layer, bool healing);

  void put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double dbu, const db::ICplxTrans &trans, bool clip);

  void begin (size_t /*nx*/, size_t /*ny*/, const db::DPoint & /*p0*/, double /*dx*/, double /*dy*/, const db::DBox & /*frame*/);
  void finish (bool /*success*/);

  void keep_for_healing (const db::Polygon &poly);
  void keep_for_healing (const db::Box &box);
  void output (const db::Polygon &poly);
  void output (const db::Box &poly);

private:
  db::Layout *mp_layout;
  db::Cell *mp_cell;
  unsigned int m_layer;
  db::Region m_for_healing;
  bool m_healing;
  tl::Mutex m_mutex;
};

class HealingTileLayoutOutputInserter
{
public:
  HealingTileLayoutOutputInserter (const db::Box &tile, bool healing, const db::ICplxTrans &trans, HealingTileLayoutOutputReceiver *rec)
    : mp_tile (&tile), m_healing (healing), mp_trans (&trans), mp_receiver (rec)
  {
    //  .. nothing yet ..
  }

  template <class T>
  void operator() (const T & /*t*/)
  {
    //  .. ignore other shapes
  }

  void operator() (const db::Polygon &poly)
  {
    if (m_healing && ! poly.box ().inside (mp_tile->enlarged (db::Vector (-1, -1)))) {
      mp_receiver->keep_for_healing (*mp_trans * poly);
    } else {
      mp_receiver->output (*mp_trans * poly);
    }
  }

  void operator() (const db::Box &box)
  {
    if (m_healing && ! box.inside (mp_tile->enlarged (db::Vector (-1, -1)))) {
      if (mp_trans->is_complex ()) {
        mp_receiver->keep_for_healing (*mp_trans * db::Polygon (box));
      } else {
        mp_receiver->keep_for_healing (*mp_trans * box);
      }
    } else {
      if (mp_trans->is_complex ()) {
        mp_receiver->output (*mp_trans * db::Polygon (box));
      } else {
        mp_receiver->output (*mp_trans * box);
      }
    }
  }

private:
  const db::Box *mp_tile;
  bool m_healing;
  const db::ICplxTrans *mp_trans;
  HealingTileLayoutOutputReceiver *mp_receiver;
};

HealingTileLayoutOutputReceiver::HealingTileLayoutOutputReceiver (db::Layout *layout, db::Cell *cell, unsigned int layer, bool healing)
  : mp_layout (layout), mp_cell (cell), m_layer (layer), m_healing (healing)
{
  //  .. nothing yet ..
}

void
HealingTileLayoutOutputReceiver::put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double dbu, const db::ICplxTrans &trans, bool clip)
{
  db::ICplxTrans tr (db::ICplxTrans (dbu / mp_layout->dbu ()) * trans);
  HealingTileLayoutOutputInserter inserter (tile, m_healing, tr, this);
  db::insert_var (inserter, obj, tile, clip);
}

void
HealingTileLayoutOutputReceiver::begin (size_t /*nx*/, size_t /*ny*/, const db::DPoint & /*p0*/, double /*dx*/, double /*dy*/, const db::DBox & /*frame*/)
{
  mp_layout->start_changes ();
}

void
HealingTileLayoutOutputReceiver::finish (bool /*success*/)
{
  //  heal the polygons
  m_for_healing.merge ();
  m_for_healing.insert_into (mp_layout, mp_cell->cell_index (), m_layer);
  m_for_healing.clear ();

  mp_layout->end_changes ();
}

void
HealingTileLayoutOutputReceiver::keep_for_healing (const db::Polygon &poly)
{
  m_for_healing.insert (poly);
}

void
HealingTileLayoutOutputReceiver::keep_for_healing (const db::Box &box)
{
  m_for_healing.insert (box);
}

void
HealingTileLayoutOutputReceiver::output (const db::Polygon &poly)
{
  mp_cell->shapes (m_layer).insert (poly);
}

void
HealingTileLayoutOutputReceiver::output (const db::Box &box)
{
  mp_cell->shapes (m_layer).insert (box);
}

// ---------------------------------------------------------------------

struct ResultDescriptor
{
  ResultDescriptor ()
    : shape_count (0), flat_shape_count (0), layer_a (-1), layer_b (-1), layer_output (-1), layout (0), top_cell (0)
  {
    //  .. nothing yet ..
  }

  size_t shape_count;
  size_t flat_shape_count;
  int layer_a;
  int layer_b;
  int layer_output;
  db::Layout *layout;
  db::cell_index_type top_cell;
  tl::optional<db::Region> results;

  size_t count () const
  {
    if (layout && layer_output >= 0) {
      size_t res = 0;
      for (db::Layout::const_iterator c = layout->begin (); c != layout->end (); ++c) {
        res += c->shapes (layer_output).size ();
      }
      return res;
    } else {
      return shape_count;
    }
  }

  size_t flat_count () const
  {
    if (layout && layer_output >= 0) {
      size_t res = 0;
      db::CellCounter counter (layout, top_cell);
      for (db::Layout::const_iterator c = layout->begin (); c != layout->end (); ++c) {
        res += c->shapes (layer_output).size () * counter.weight (c->cell_index ());
      }
      return res;
    } else {
      return flat_shape_count;
    }
  }

  bool is_empty () const
  {
    if (layout && layer_output >= 0) {
      for (db::Layout::const_iterator c = layout->begin (); c != layout->end (); ++c) {
        if (! c->shapes (layer_output).empty ()) {
          return false;
        }
      }
      return true;
    } else {
      return shape_count == 0;
    }
  }
};

// ---------------------------------------------------------------------

struct XORData
{
  XORData ()
    : layout_a (0), layout_b (0), cell_a (0), cell_b (0),
      tolerance_bump (0),
      dont_summarize_missing_layers (false), silent (false), no_summary (false),
      threads (0),
      tile_size (0.0), heal_results (false),
      output_layout (0), output_cell (0),
      layers_missing (0)
  { }

  db::Layout *layout_a, *layout_b;
  db::cell_index_type cell_a, cell_b;
  std::vector<double> tolerances;
  int tolerance_bump;
  bool dont_summarize_missing_layers;
  bool silent;
  bool no_summary;
  int threads;
  double tile_size;
  bool heal_results;
  db::Layout *output_layout;
  db::cell_index_type output_cell;
  std::map<db::LayerProperties, std::pair<int, int>, db::LPLogicalLessFunc> l2l_map;
  std::map<std::pair<int, db::LayerProperties>, ResultDescriptor> *results;
  mutable int layers_missing;
  mutable tl::Mutex lock;
};

}

// ---------------------------------------------------------------------

static bool run_tiled_xor (const XORData &xor_data);
static bool run_deep_xor (const XORData &xor_data);

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
  std::string top_a, top_b, top_output;
  bool dont_summarize_missing_layers = false;
  bool silent = false;
  bool no_summary = false;
  bool deep = false;
  std::vector<double> tolerances;
  int tolerance_bump = 10000;
  int threads = 1;
  double tile_size = 0.0;
  bool heal_results = false;

  tl::CommandLineOptions cmd;
  generic_reader_options_a.add_options (cmd);
  generic_reader_options_b.add_options (cmd);

  db::SaveLayoutOptions def_writer_options;
  def_writer_options.set_dont_write_empty_cells (true);
  bd::GenericWriterOptions writer_options (def_writer_options);
  writer_options.add_options (cmd);

  cmd << tl::arg ("input_a",                   &infile_a,   "The first input file (any format, may be gzip compressed)")
      << tl::arg ("input_b",                   &infile_b,   "The second input file (any format, may be gzip compressed)")
      << tl::arg ("?output",                   &output,     "The output file to which the XOR differences are written",
                  "This argument is optional. If not given, the exit status alone will indicate whether the layouts "
                  "are identical or not. The output is a layout file. The format of the file is derived "
                  "from the file name's suffix (.oas[.gz] for (gzipped) OASIS, .gds[.gz] for (gzipped) GDS2 etc.)."
                 )
      << tl::arg ("-ta|--top-a=name",          &top_a,      "Specifies the top cell for the first layout",
                  "Use this option to take a specific cell as the top cell from the first layout. All "
                  "cells not called directly or indirectly from this cell are ignored. If you use this option, "
                  "--top-b must be specified too and can be different from the first layout's top cell."
                 )
      << tl::arg ("-tb|--top-b=name",          &top_b,      "Specifies the top cell for the second layout",
                  "See --top-a for details."
                 )
      << tl::arg ("-to|--top-output=name",     &top_output, "Specifies the top cell for the output layout",
                  "This option is only used if an output layout is given. It will specify the name of top cell to use there. "
                  "If not specified, KLayout uses the top cell name of the first layout or the one given with --top-a."
                 )
      << tl::arg ("-u|--deep",                 &deep,       "Deep (hierarchical mode)",
                  "Enables hierarchical XOR (experimental). In this mode, tiling is not supported "
                  "and the tiling arguments are ignored."
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
      << tl::arg ("-m|--heal",                 &heal_results, "Heal results in tiling mode",
                  "This options runs a post-XOR merge to remove cuts implied by the tile formation. The resulting "
                  "feature count is closer to the real number of differences."
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
    throw tl::Exception ("Both -ta|--top-a and -tb|--top-b top cells must be given, not just one of them");
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

  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Total")));

  db::Layout layout_a;
  db::Layout layout_b;

  {
    tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Loading file (A): ")) + infile_a);

    db::LoadLayoutOptions load_options;
    generic_reader_options_a.configure (load_options);
    bd::read_files (layout_a, infile_a, load_options);
  }

  {
    tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Loading file (B): ")) + infile_b);

    db::LoadLayoutOptions load_options;
    generic_reader_options_b.configure (load_options);
    bd::read_files (layout_b, infile_b, load_options);
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

  std::map<db::LayerProperties, std::pair<int, int>, db::LPLogicalLessFunc> l2l_map;

  for (db::Layout::layer_iterator l = layout_a.begin_layers (); l != layout_a.end_layers (); ++l) {
    l2l_map.insert (std::make_pair (*(*l).second, std::make_pair (-1, -1))).first->second.first = (*l).first;
  }
  for (db::Layout::layer_iterator l = layout_b.begin_layers (); l != layout_b.end_layers (); ++l) {
    l2l_map.insert (std::make_pair (*(*l).second, std::make_pair (-1, -1))).first->second.second = (*l).first;
  }

  std::unique_ptr<db::Layout> output_layout;
  db::cell_index_type output_top = 0;

  if (! output.empty ()) {
    output_layout.reset (new db::Layout ());
    output_top = output_layout->add_cell (top_output.empty () ? top_a.c_str () : top_output.c_str ());
  }

  std::map<std::pair<int, db::LayerProperties>, ResultDescriptor> results;

  XORData xor_data;
  xor_data.layout_a = &layout_a;
  xor_data.cell_a = index_a.second;
  xor_data.layout_b = &layout_b;
  xor_data.cell_b = index_b.second;
  xor_data.tolerances = tolerances;
  xor_data.tolerance_bump = tolerance_bump;
  xor_data.dont_summarize_missing_layers = dont_summarize_missing_layers;
  xor_data.silent = silent;
  xor_data.no_summary = no_summary;
  xor_data.threads = threads;
  xor_data.tile_size = tile_size;
  xor_data.heal_results = heal_results;
  xor_data.output_layout = output_layout.get ();
  xor_data.output_cell = output_top;
  xor_data.l2l_map = l2l_map;
  xor_data.results = &results;

  //  Runs the XOR

  bool result;

  if (deep) {
    result = run_deep_xor (xor_data);
  } else {
    result = run_tiled_xor (xor_data);
  }

  //  Writes the output layout

  if (output_layout.get ()) {

    db::SaveLayoutOptions save_options;
    save_options.set_format_from_filename (output);
    writer_options.configure (save_options, *output_layout);

    tl::OutputStream stream (output);
    db::Writer writer (save_options);
    writer.write (*output_layout, stream);

  }

  if (! silent && ! no_summary) {

    if (result) {
      tl::info << tl::to_string (tr ("No differences found"));
    } else {

      const char *line_format = "  %-10s %-12s %s";

      std::string headline = tl::sprintf (line_format, tl::to_string (tr ("Layer")), tl::to_string (tr ("Output")),
                                                       deep ? tl::to_string (tr ("Differences (hierarchical/flat count)")) : tl::to_string (tr ("Differences (shape count)")));

      const char *sep = "  ----------------------------------------------------------------";

      tl::info << tl::to_string (tr ("Result summary (layers without differences are not shown):")) << tl::endl;
      tl::info << headline << tl::endl << sep;

      int ti = -1;
      for (std::map<std::pair<int, db::LayerProperties>, ResultDescriptor>::const_iterator r = results.begin (); r != results.end (); ++r) {

        if (r->first.first != ti) {
          ti = r->first.first;
          if (tolerances[ti] > db::epsilon) {
            tl::info << tl::endl << tl::to_string (tr ("Tolerance ")) << tl::micron_to_string (tolerances[ti]) << ":" << tl::endl;
            tl::info << headline << tl::endl << sep;
          }
        }

        std::string out ("-");
        std::string value;
        if (r->second.layer_a < 0 && ! dont_summarize_missing_layers) {
          value = tl::to_string (tr ("(no such layer in first layout)"));
        } else if (r->second.layer_b < 0 && ! dont_summarize_missing_layers) {
          value = tl::to_string (tr ("(no such layer in second layout)"));
        } else if (! r->second.is_empty ()) {
          if (r->second.layer_output >= 0 && r->second.layout) {
            out = r->second.layout->get_properties (r->second.layer_output).to_string ();
          }
          if (deep) {
            value = tl::sprintf (tl::to_string (tr ("%-6lu / %-6lu")), r->second.count (), r->second.flat_count ());
          } else {
            value = tl::to_string (r->second.count ());
          }
        }
        if (! value.empty ()) {
          tl::info << tl::sprintf (line_format, r->first.second.to_string (), out, value);
        }

      }

      tl::info << "";

    }

  }

  return result ? 0 : 1;
}

bool run_tiled_xor (const XORData &xor_data)
{
  db::TilingProcessor proc;
  proc.set_dbu (std::min (xor_data.layout_a->dbu (), xor_data.layout_b->dbu ()));
  proc.set_threads (std::max (1, xor_data.threads));
  if (xor_data.tile_size > db::epsilon) {
    if (tl::verbosity () >= 20) {
      tl::log << "Tile size: " << xor_data.tile_size;
      tl::log << "Healing: " << (xor_data.heal_results ? "on" : "off");
    }
    proc.tile_size (xor_data.tile_size, xor_data.tile_size);
  }

  proc.tile_border (xor_data.tolerances.back () * 2.0, xor_data.tolerances.back () * 2.0);
  if (tl::verbosity () >= 20) {
    tl::log << "Tile border: " << xor_data.tolerances.back () * 2.0;
  }

  if (tl::verbosity () >= 20) {
    tl::log << "Database unit: " << proc.dbu ();
    tl::log << "Threads: " << xor_data.threads;
    tl::log << "Layer bump for tolerance: " << xor_data.tolerance_bump;
  }

  if (xor_data.output_layout) {
    xor_data.output_layout->dbu (proc.dbu ());
  }

  bool result = true;

  int index = 1;

  for (std::map<db::LayerProperties, std::pair<int, int> >::const_iterator ll = xor_data.l2l_map.begin (); ll != xor_data.l2l_map.end (); ++ll) {

    if ((ll->second.first < 0 || ll->second.second < 0) && ! xor_data.dont_summarize_missing_layers) {

      if (ll->second.first < 0) {
        (xor_data.silent ? tl::log : tl::warn) << "Layer " << ll->first.to_string () << " is not present in first layout, but in second";
      } else {
        (xor_data.silent ? tl::log : tl::warn) << "Layer " << ll->first.to_string () << " is not present in second layout, but in first";
      }

      result = false;

      int tol_index = 0;
      for (std::vector<double>::const_iterator t = xor_data.tolerances.begin (); t != xor_data.tolerances.end (); ++t) {

        ResultDescriptor &result = xor_data.results->insert (std::make_pair (std::make_pair (tol_index, ll->first), ResultDescriptor ())).first->second;
        result.layer_a = ll->second.first;
        result.layer_b = ll->second.second;
        result.layout = xor_data.output_layout;
        result.top_cell = xor_data.output_cell;

        ++tol_index;

      }

    } else {

      std::string in_a = "a" + tl::to_string (index);
      std::string in_b = "b" + tl::to_string (index);

      if (ll->second.first < 0) {
        proc.input (in_a, db::RecursiveShapeIterator ());
      } else {
        db::RecursiveShapeIterator si (*xor_data.layout_a, xor_data.layout_a->cell (xor_data.cell_a), ll->second.first);
        si.set_for_merged_input (true);
        proc.input (in_a, si);
      }

      if (ll->second.second < 0) {
        proc.input (in_b, db::RecursiveShapeIterator ());
      } else {
        db::RecursiveShapeIterator si (*xor_data.layout_b, xor_data.layout_b->cell (xor_data.cell_b), ll->second.second);
        si.set_for_merged_input (true);
        proc.input (in_b, si);
      }

      std::string expr = "var x=" + in_a + "^" + in_b + "; ";

      int tol_index = 0;
      for (std::vector<double>::const_iterator t = xor_data.tolerances.begin (); t != xor_data.tolerances.end (); ++t) {

        std::string out = "o" + tl::to_string (index) + "_" + tl::to_string (tol_index + 1);

        db::LayerProperties lp = ll->first;
        if (lp.layer >= 0) {
          lp.layer += tol_index * xor_data.tolerance_bump;
        }

        ResultDescriptor &result = xor_data.results->insert (std::make_pair (std::make_pair (tol_index, ll->first), ResultDescriptor ())).first->second;
        result.layer_a = ll->second.first;
        result.layer_b = ll->second.second;
        result.layout = xor_data.output_layout;
        result.top_cell = xor_data.output_cell;

        if (result.layout) {
          result.layer_output = result.layout->insert_layer (lp);
          HealingTileLayoutOutputReceiver *receiver = new HealingTileLayoutOutputReceiver (result.layout, &result.layout->cell (result.top_cell), result.layer_output, xor_data.heal_results);
          proc.output (out, 0, receiver, db::ICplxTrans ());
        } else {
          HealingCountingReceiver *counter = new HealingCountingReceiver (&result.shape_count, xor_data.heal_results);
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

  if ((! xor_data.silent && ! xor_data.no_summary) || result || xor_data.output_layout) {
    proc.execute ("Running XOR");
  }

  //  no stored results currently
  for (std::map<std::pair<int, db::LayerProperties>, ResultDescriptor>::const_iterator r = xor_data.results->begin (); r != xor_data.results->end (); ++r) {
    tl_assert (! r->second.results.has_value ());
  }

  //  Determines the output status
  for (std::map<std::pair<int, db::LayerProperties>, ResultDescriptor>::const_iterator r = xor_data.results->begin (); r != xor_data.results->end () && result; ++r) {
    //  no stored results currently
    tl_assert (! r->second.results.has_value ());
    result = r->second.is_empty ();
  }

  return result;
}


class XORJob
  : public tl::JobBase
{
public:
  XORJob (int nworkers)
    : tl::JobBase (nworkers)
  {
  }

  virtual tl::Worker *create_worker ();
};

class XORWorker
  : public tl::Worker
{
public:
  XORWorker (XORJob *job);
  void perform_task (tl::Task *task);

  db::DeepShapeStore &dss ()
  {
    return m_dss;
  }

private:
  XORJob *mp_job;
  db::DeepShapeStore m_dss;
};

class XORTask
  : public tl::Task
{
public:
  XORTask (const XORData *xor_data, const db::LayerProperties &layer_props, int la, int lb, double dbu)
    : mp_xor_data (xor_data), m_layer_props (layer_props), m_la (la), m_lb (lb), m_dbu (dbu)
  {
    //  .. nothing yet ..
  }

  void run (XORWorker *worker) const
  {
    if ((m_la < 0 || m_lb < 0) && ! mp_xor_data->dont_summarize_missing_layers) {

      if (m_la < 0) {
        (mp_xor_data->silent ? tl::log : tl::warn) << "Layer " << m_layer_props.to_string () << " is not present in first layout, but in second";
      } else {
        (mp_xor_data->silent ? tl::log : tl::warn) << "Layer " << m_layer_props.to_string () << " is not present in second layout, but in first";
      }

      tl::MutexLocker locker (&mp_xor_data->lock);

      mp_xor_data->layers_missing += 1;

      int tol_index = 0;
      for (std::vector<double>::const_iterator t = mp_xor_data->tolerances.begin (); t != mp_xor_data->tolerances.end (); ++t) {

        ResultDescriptor &result = mp_xor_data->results->insert (std::make_pair (std::make_pair (tol_index, m_layer_props), ResultDescriptor ())).first->second;
        result.layer_a = m_la;
        result.layer_b = m_lb;
        result.layout = mp_xor_data->output_layout;
        result.top_cell = mp_xor_data->output_cell;

        ++tol_index;

      }

    } else {

      tl::SelfTimer timer (tl::verbosity () >= 11, "XOR on layer " + m_layer_props.to_string ());

      db::Region xor_res;

      if (m_la < 0) {

        tl_assert (m_lb >= 0);

        db::RecursiveShapeIterator ri_b (*mp_xor_data->layout_b, mp_xor_data->layout_b->cell (mp_xor_data->cell_b), m_lb);
        xor_res = db::Region (ri_b, worker->dss (), db::ICplxTrans (mp_xor_data->layout_b->dbu () / m_dbu));

      } else if (m_lb < 0) {

        db::RecursiveShapeIterator ri_a (*mp_xor_data->layout_a, mp_xor_data->layout_a->cell (mp_xor_data->cell_a), m_la);
        xor_res = db::Region (ri_a, worker->dss (), db::ICplxTrans (mp_xor_data->layout_a->dbu () / m_dbu));

      } else {

        db::RecursiveShapeIterator ri_a (*mp_xor_data->layout_a, mp_xor_data->layout_a->cell (mp_xor_data->cell_a), m_la);
        db::RecursiveShapeIterator ri_b (*mp_xor_data->layout_b, mp_xor_data->layout_b->cell (mp_xor_data->cell_b), m_lb);

        db::Region in_a (ri_a, worker->dss (), db::ICplxTrans (mp_xor_data->layout_a->dbu () / m_dbu));
        db::Region in_b (ri_b, worker->dss (), db::ICplxTrans (mp_xor_data->layout_b->dbu () / m_dbu));

        bool a_empty = in_a.empty ();
        bool b_empty = in_b.empty ();

        if (a_empty && ! b_empty) {
          xor_res = in_b;
        } else if (! a_empty && b_empty) {
          xor_res = in_a;
        } else if (! a_empty && ! b_empty) {
          tl::SelfTimer timer (tl::verbosity () >= 21, "Basic XOR on layer " + m_layer_props.to_string ());
          xor_res = in_a ^ in_b;
        }

      }

      int tol_index = 0;
      for (std::vector<double>::const_iterator t = mp_xor_data->tolerances.begin (); t != mp_xor_data->tolerances.end (); ++t) {

        db::LayerProperties lp = m_layer_props;
        if (lp.layer >= 0) {
          lp.layer += tol_index * mp_xor_data->tolerance_bump;
        }

        if (*t > db::epsilon) {
          tl::SelfTimer timer (tl::verbosity () >= 21, "Tolerance " + tl::to_string (*t) + " on layer " + m_layer_props.to_string ());
          xor_res.size (-db::coord_traits<db::Coord>::rounded (0.5 * *t / m_dbu));
          xor_res.size (db::coord_traits<db::Coord>::rounded (0.5 * *t / m_dbu));
        }

        {
          tl::MutexLocker locker (&mp_xor_data->lock);

          ResultDescriptor &result = mp_xor_data->results->insert (std::make_pair (std::make_pair (tol_index, m_layer_props), ResultDescriptor ())).first->second;
          result.layer_a = m_la;
          result.layer_b = m_lb;
          result.layout = mp_xor_data->output_layout;
          result.top_cell = mp_xor_data->output_cell;

          if (mp_xor_data->output_layout) {
            result.layer_output = result.layout->insert_layer (lp);
            if (! xor_res.empty ()) {
              result.results = xor_res;
            }
          } else {
            result.shape_count = xor_res.hier_count ();
            result.flat_shape_count = xor_res.count ();
          }
        }

        ++tol_index;

      }

    }
  }

private:
  const XORData *mp_xor_data;
  const db::LayerProperties &m_layer_props;
  int m_la;
  int m_lb;
  double m_dbu;
};

XORWorker::XORWorker (XORJob *job)
  : tl::Worker (), mp_job (job)
{
  //  TODO: this conflicts with the "set_for_merged_input" optimization below.
  //  It seems not to be very effective then. Why?
  m_dss.set_wants_all_cells (true);  //  saves time for less cell mapping operations
}

void
XORWorker::perform_task (tl::Task *task)
{
  XORTask *xor_task = dynamic_cast <XORTask *> (task);
  if (xor_task) {
    xor_task->run (this);
  }
}

tl::Worker *
XORJob::create_worker ()
{
  return new XORWorker (this);
}


bool run_deep_xor (const XORData &xor_data)
{
  double dbu = std::min (xor_data.layout_a->dbu (), xor_data.layout_b->dbu ());

  if (tl::verbosity () >= 20) {
    tl::log << "Database unit: " << dbu;
    tl::log << "Threads: " << xor_data.threads;
    tl::log << "Layer bump for tolerance: " << xor_data.tolerance_bump;
  }

  if (xor_data.output_layout) {
    xor_data.output_layout->dbu (dbu);
  }

  XORJob job (xor_data.threads);

  for (std::map<db::LayerProperties, std::pair<int, int> >::const_iterator ll = xor_data.l2l_map.begin (); ll != xor_data.l2l_map.end (); ++ll) {
    job.schedule (new XORTask (&xor_data, ll->first, ll->second.first, ll->second.second, dbu));
  }

  job.start ();
  job.wait ();

  //  Deliver the outputs
  //  NOTE: this is done single-threaded and in a delayed fashion as it is not efficient during
  //  computation and shifting hierarchy of the working layout

  if (xor_data.output_layout) {

    tl::SelfTimer timer (tl::verbosity () >= 11, "Result delivery");

    for (std::map<std::pair<int, db::LayerProperties>, ResultDescriptor>::const_iterator r = xor_data.results->begin (); r != xor_data.results->end (); ++r) {
      if (r->second.results.has_value ()) {
        r->second.results.value ().insert_into (xor_data.output_layout, xor_data.output_cell, r->second.layer_output);
      }
    }

  }

  //  Determine the output status

  bool result = (xor_data.layers_missing == 0);
  for (std::map<std::pair<int, db::LayerProperties>, ResultDescriptor>::const_iterator r = xor_data.results->begin (); r != xor_data.results->end () && result; ++r) {
    result = r->second.is_empty ();
  }

  return result;
}

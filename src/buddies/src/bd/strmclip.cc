
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
#include "dbClip.h"
#include "dbLayout.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbSaveLayoutOptions.h"
#include "tlLog.h"
#include "tlCommandLineParser.h"
#include "tlTimer.h"


struct ClipData
{
  ClipData () 
    : file_in (), file_out (), clip_layer ()
  { }

  bd::GenericReaderOptions reader_options;
  bd::GenericWriterOptions writer_options;
  std::string file_in;
  std::string file_out;
  db::LayerProperties clip_layer;
  std::vector <db::DBox> clip_boxes;
  std::string result;
  std::string top;

  void add_box (const std::string &spec)
  {
    tl::Extractor ex (spec.c_str ());
    double l = 0.0, b = 0.0, r = 0.0, t = 0.0;
    ex >> l >> "," >> b >> "," >> r >> "," >> t >> tl::Extractor::end ();
    clip_boxes.push_back (db::DBox (l, b, r, t));
  }

  void set_clip_layer (const std::string &spec)
  {
    tl::Extractor ex (spec.c_str ());
    clip_layer = db::LayerProperties ();
    clip_layer.read (ex);
  }
};


void clip (ClipData &data)
{
  db::Layout layout;
  db::Layout target_layout;

  {
    db::LoadLayoutOptions load_options;
    data.reader_options.configure (load_options);
    bd::read_files (layout, data.file_in, load_options);
  }

  //  create the layers in the target layout as well
  for (unsigned int i = 0; i < layout.layers (); ++i) {
    if (layout.is_valid_layer (i)) {
      target_layout.insert_layer (i, layout.get_properties (i));
    }
  }

  //  copy the properties repository in order to have the same ID mapping
  target_layout.dbu (layout.dbu ());

  //  look for the clip layer
  int clip_layer_index = -1;
  for (unsigned int i = 0; i < layout.layers (); ++i) {
    if (layout.is_valid_layer (i) && data.clip_layer.log_equal (layout.get_properties (i))) {
      clip_layer_index = int (i);
      break;
    }
  }

  tl::log << "Clip layer index is " << clip_layer_index;

  //  get top cells
  std::vector <db::cell_index_type> top_cells;
  if (data.top.empty ()) {
    top_cells.assign (layout.begin_top_down (), layout.end_top_cells ());
  } else {
    std::pair<bool, db::cell_index_type> tc = layout.cell_by_name (data.top.c_str ());
    if (! tc.first) {
      throw tl::Exception ("Cell %s is not a valid cell in the input layout", data.top);
    }
    top_cells.push_back (tc.second);
  }

  //  go through the top cells
  for (std::vector <db::cell_index_type>::const_iterator tc = top_cells.begin (); tc != top_cells.end (); ++tc) {

    std::vector <db::Box> clip_boxes;

    //  add the explicit boxes first
    for (std::vector <db::DBox>::const_iterator b = data.clip_boxes.begin (); b != data.clip_boxes.end (); ++b) {
      clip_boxes.push_back (db::VCplxTrans (1.0 / layout.dbu ()) * *b);
    }

    //  fetch the boxes of the clip shapes
    if (clip_layer_index >= 0) {
      collect_clip_boxes (layout, *tc, clip_layer_index, clip_boxes);
    }

    //  sort our duplicate boxes
    std::sort (clip_boxes.begin (), clip_boxes.end ());
    clip_boxes.erase (std::unique (clip_boxes.begin (), clip_boxes.end ()), clip_boxes.end ());

    tl::log << "Clip boxes are:";
    for (std::vector <db::Box>::const_iterator cbx = clip_boxes.begin (); cbx != clip_boxes.end (); ++cbx) {
      tl::log << "  " << cbx->to_string ();
    }

    std::vector<db::cell_index_type> new_cells = db::clip_layout (layout, target_layout, *tc, clip_boxes, true /*stable*/);

    //  create "very top" cells to put the result cells into
    std::string result_top;
    if (! data.result.empty ()) {
      result_top = data.result;
    } else {
      result_top = std::string ("CLIPPED_") + layout.cell_name (*tc);
    }
    db::cell_index_type clip_top = target_layout.add_cell (result_top.c_str ());
    db::Cell &clip_top_cell = target_layout.cell (clip_top);

    for (std::vector <db::cell_index_type>::const_iterator cc = new_cells.begin (); cc != new_cells.end (); ++cc) {
      clip_top_cell.insert (db::CellInstArray (db::CellInst (*cc), db::Trans ()));
    }

  }

  //  write the layout

  db::SaveLayoutOptions save_options;
  save_options.set_format_from_filename (data.file_out);
  data.writer_options.configure (save_options, target_layout);

  tl::OutputStream stream (data.file_out);
  db::Writer writer (save_options);
  writer.write (target_layout, stream);
}

BD_PUBLIC int strmclip (int argc, char *argv[])
{
  ClipData data;

  tl::CommandLineOptions cmd;
  data.reader_options.add_options (cmd);
  data.writer_options.add_options (cmd);

  cmd << tl::arg ("input",                     &data.file_in, "The input file",
                  "The input file can be any supported format. It can be gzip compressed and will "
                  "be uncompressed automatically in this case."
                 )
      << tl::arg ("output",                    &data.file_out, "The output file",
                  "The output format is determined from the suffix of the file. If the suffix indicates "
                  "gzip compression, the file will be compressed on output. Examples for recognized suffixes are "
                  "\".oas\", \".gds.gz\", \".dxf\" or \".gds2\"."
                 )
      << tl::arg ("-l|--clip-layer=spec",      &data, &ClipData::set_clip_layer, "Specifies a layer to take the clip regions from",
                  "If this option is given, the clip rectangles are taken from the given layer."
                  "The layer specification is of the \"layer/datatype\" form or a plain layer name if named layers "
                  "are available."
                 )
      << tl::arg ("-t|--top-in=cellname",      &data.top, "Specifies the top cell for input",
                  "If this option is given, it specifies the cell to use as top cell from the input."
                 )
      << tl::arg ("-x|--top-out=cellname",     &data.result, "Specifies the top cell for output",
                  "If given, this name will be used as the top cell name in the output file. "
                  "By default the output's top cell will be \"CLIPPED_\" plus the input's top cell name."
                 )
      << tl::arg ("*-r|--rect=\"l,b,r,t\"",    &data, &ClipData::add_box, "Specifies a clip box",
                  "This option specifies the box to clip in micrometer units. The box is given "
                  "by left, bottom, right and top coordinates. This option can be used multiple times "
                  "to produce a clip covering more than one rectangle."
                 )
    ;

  cmd.brief ("This program will produce clips from an input layout and writes them to another layout");

  cmd.parse (argc, argv);

  tl::SelfTimer timer (tl::verbosity () >= 11, tl::to_string (tr ("Total")));

  clip (data);

  return 0;
}

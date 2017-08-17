
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

#include "dbClip.h"
#include "dbLayout.h"
#include "dbGDS2Writer.h"
#include "dbOASISWriter.h"
#include "dbReader.h"
#include "tlLog.h"


struct ClipData
{
  ClipData () 
    : file_in (), file_out (), clip_layer (), 
      oasis (false), gzip (false)
  { }

  std::string file_in;
  std::string file_out;
  db::LayerProperties clip_layer;
  bool oasis;
  bool gzip;
  std::vector <db::DBox> clip_boxes;
  std::string result;
  std::string top;
};


void clip (const ClipData &data)
{
  db::Manager m;
  db::Layout layout (&m);
  db::Layout target_layout (&m);

  {
    tl::InputStream stream (data.file_in);
    db::Reader reader (stream);
    reader.read (layout);
  }

  //  create the layers in the target layout as well
  for (unsigned int i = 0; i < layout.layers (); ++i) {
    if (layout.is_valid_layer (i)) {
      target_layout.insert_layer (i, layout.get_properties (i));
    }
  }

  //  copy the properties repository in order to have the same ID mapping
  target_layout.properties_repository () = layout.properties_repository ();
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
  tl::OutputStreamBase *out_file = 0;
  try {

    tl::OutputStream stream (data.file_out, data.gzip ? tl::OutputStream::OM_Zlib : tl::OutputStream::OM_Plain);

    if (data.oasis) {
      db::OASISWriter writer;
      writer.write (target_layout, stream, db::SaveLayoutOptions ());
    } else {
      db::GDS2Writer writer;
      writer.write (target_layout, stream, db::SaveLayoutOptions ());
    }

    delete out_file;

  } catch (...) {
    if (out_file) {
      delete out_file;
    }
    throw;
  }
}

void print_syntax ()
{
  printf ("Syntax: strmclip [<options>] <infile> <outfile>\n");
  printf ("\n");
  printf ("Options are:\n");
  printf ("  -l 'l/d'      take clip regions from layer l, datatype d\n");
  printf ("  -o            produce oasis output\n");
  printf ("  -g            produce gds output\n");
  printf ("  -z            gzip output\n");
  printf ("  -t 'cell'     use this cell from input (default: determine top cell automatically)\n");
  printf ("  -x 'name'     use this cell as top cell in output\n");
  printf ("  -r 'l,b,r,t'  explicitly specify a clip rectangle (can be present multiple times)\n");
}

int 
main (int argc, char *argv [])
{
  try {

    ClipData data;

    for (int n = 1; n < argc; ++n) {

      if (std::string (argv [n]) == "-h") {
        print_syntax ();
        return 0;
      } else if (std::string (argv [n]) == "-o") {
        data.oasis = true;
      } else if (std::string (argv [n]) == "-g") {
        data.oasis = false;
      } else if (std::string (argv [n]) == "-z") {
        data.gzip = true;
      } else if (std::string (argv [n]) == "-x") {
        if (n < argc + 1) {
          ++n;
          data.result = argv [n];
        } 
      } else if (std::string (argv [n]) == "-t") {
        if (n < argc + 1) {
          ++n;
          data.top = argv [n];
        } 
      } else if (std::string (argv [n]) == "-r") {
        if (n < argc + 1) {
          ++n;
          tl::Extractor ex (argv [n]);
          double l = 0.0, b = 0.0, r = 0.0, t = 0.0;
          ex.read (l); ex.expect (",");
          ex.read (b); ex.expect (",");
          ex.read (r); ex.expect (",");
          ex.read (t); ex.expect_end ();
          data.clip_boxes.push_back (db::DBox (l, b, r, t));
        } 
      } else if (std::string (argv [n]) == "-l") {
        if (n < argc + 1) {
          ++n;
          tl::Extractor ex (argv[n]);
          db::LayerProperties lp;
          lp.read (ex);
          data.clip_layer = lp;
        } 
      } else if (argv [n][0] == '-') {
        print_syntax ();
        throw tl::Exception ("Unknown option: " + std::string (argv [n]));
      } else if (data.file_in.empty ()) {
        data.file_in = argv [n];
      } else if (data.file_out.empty ()) {
        data.file_out = argv [n];
      } else {
        print_syntax ();
        throw tl::Exception ("Superfluous command element: " + std::string (argv [n]));
      }

    }

    if (data.file_in.empty () || data.file_out.empty ()) {
      print_syntax ();
      throw tl::Exception ("Input or output file name missing");
    }

    clip (data);

  } catch (std::exception &ex) {
    tl::error << ex.what ();
    return 1;
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    return 1;
  } catch (...) {
    tl::error << "unspecific error";
    return 1;
  }

  return 0;
}



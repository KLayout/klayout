
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

#include "dbLayout.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbShapeProcessor.h"
#include "tlString.h"

void
syntax ()
{
  printf ("Syntax: strmxor [-u <undersize>] [-topa <topcell-a>] [-topb <topcell-b>] [-oasis|-oas] [-gds2|-gds] <infile-a> <infile-b> [<outfile>]\n");
}

int 
main (int argc, char *argv [])
{
  std::string topcell_a;
  std::string topcell_b;
  std::string infile_a;
  std::string infile_b;
  std::string outfile;
  double undersize = 0.0;
  bool format_set = false;
  std::string format;

  int ret = 0;

  try {

    for (int i = 1; i < argc; ++i) {
      std::string o (argv[i]);
      if (o == "-u") {
        if (i < argc - 1) {
          ++i;
          tl::from_string (argv[i], undersize);
        }
      } else if (o == "-topa") {
        if (i < argc - 1) {
          ++i;
          topcell_a = argv[i];
        }
      } else if (o == "-topb") {
        if (i < argc - 1) {
          ++i;
          topcell_b = argv[i];
        }
      } else if (o == "-oasis" || o == "-oas") {
        format_set = true;
        format = "OASIS";
      } else if (o == "-gds2" || o == "-gds") {
        format_set = true;
        format = "GDS2";
      } else if (o == "-h" || o == "-help" || o == "--help") {
        syntax ();
        return 0;
      } else if (argv[i][0] == '-') {
        throw tl::Exception("Unknown option: %s - use '-h' for help", (const char *) argv[i]);
      } else if (infile_a.empty ()) {
        infile_a = argv[i];
      } else if (infile_b.empty ()) {
        infile_b = argv[i];
      } else if (outfile.empty ()) {
        outfile = argv[i];
      } else {
        throw tl::Exception("Superfluous argument: %s - use '-h' for help", (const char *) argv[i]);
      }
    }

    if (infile_a.empty () || infile_b.empty ()) {
      throw tl::Exception("Both input files must be specified");
    }

    db::Manager m;
    db::Layout layout_a (&m);
    db::Layout layout_b (&m);

    {
      tl::InputStream stream (infile_a);
      db::Reader reader (stream);
      reader.read (layout_a);
    }

    {
      tl::InputStream stream (infile_b);
      db::Reader reader (stream);
      reader.read (layout_b);
    }

    db::cell_index_type top_a;
    if (topcell_a.empty ()) {
      db::Layout::top_down_iterator t = layout_a.begin_top_down ();
      if (t == layout_a.end_top_cells ()) {
        throw tl::Exception ("Layout A (%s) does not have a top cell", infile_a);
      }
      top_a = *t++;
      if (t != layout_a.end_top_cells ()) {
        throw tl::Exception ("Layout A (%s) has multiple top cells", infile_a);
      }
    } else {
      std::pair<bool, db::cell_index_type> cn = layout_a.cell_by_name (topcell_a.c_str ());
      if (! cn.first) {
        throw tl::Exception ("Layout A (%s) does not have a topcell called '%s'", infile_a, topcell_a);
      }
      top_a = cn.second;
    }

    db::cell_index_type top_b;
    if (topcell_b.empty ()) {
      db::Layout::top_down_iterator t = layout_b.begin_top_down ();
      if (t == layout_b.end_top_cells ()) {
        throw tl::Exception ("Layout B (%s) does not have a top cell", infile_b);
      }
      top_b = *t++;
      if (t != layout_b.end_top_cells ()) {
        throw tl::Exception ("Layout B (%s) has multiple top cells", infile_b);
      }
    } else {
      std::pair<bool, db::cell_index_type> cn = layout_b.cell_by_name (topcell_b.c_str ());
      if (! cn.first) {
        throw tl::Exception ("Layout B (%s) does not have a topcell called '%s'", infile_b, topcell_b);
      }
      top_b = cn.second;
    }

    if (fabs (layout_a.dbu () - layout_b.dbu ()) > 1e-6) {
      throw tl::Exception("Input file database units differ (A:%g vs. B:%g)", layout_a.dbu (), layout_b.dbu ());
    }

    std::map<db::LayerProperties, std::pair<int, int> > all_layers;
    for (unsigned int i = 0; i < layout_a.layers (); ++i) {
      if (layout_a.is_valid_layer (i)) {
        all_layers.insert (std::make_pair(layout_a.get_properties (i), std::make_pair(-1, -1))).first->second.first = int (i);
      }
    }
    for (unsigned int i = 0; i < layout_b.layers (); ++i) {
      if (layout_b.is_valid_layer (i)) {
        all_layers.insert (std::make_pair(layout_b.get_properties (i), std::make_pair(-1, -1))).first->second.second = int (i);
      }
    }

    db::Layout output;
    output.dbu (layout_a.dbu ());
    db::cell_index_type top_id = output.add_cell (layout_a.cell_name (top_a));

    db::Coord us = db::coord_traits<db::Coord>::rounded (undersize / layout_a.dbu ());

    db::ShapeProcessor sp;

    size_t ndiff = 0;

    for (std::map<db::LayerProperties, std::pair<int, int> >::const_iterator l = all_layers.begin (); l != all_layers.end (); ++l) {

      int layer_id = output.insert_layer (l->first);

      if (l->second.first >= 0 && l->second.second >= 0) {
        
        sp.boolean (layout_a, layout_a.cell (top_a), l->second.first, layout_b, layout_b.cell (top_b), l->second.second, 
                    output.cell (top_id).shapes (layer_id), db::BooleanOp::Xor, true /*recursive*/);

        sp.size (output, output.cell (top_id), layer_id, output.cell (top_id).shapes (layer_id), -us, (unsigned int) 2, true /*recursive*/);

      } else if (l->second.first >= 0) {

        sp.size (layout_a, layout_a.cell (top_a), l->second.first, output.cell (top_id).shapes (layer_id), -us, (unsigned int) 2, true /*recursive*/);

      } else if (l->second.second >= 0) {

        sp.size (layout_b, layout_b.cell (top_b), l->second.second, output.cell (top_id).shapes (layer_id), -us, (unsigned int) 2, true /*recursive*/);

      }

      size_t n = output.cell (top_id).shapes (layer_id).size ();
      // if (n > 0) {
        ndiff += n;
        tl::info << "  " << l->first.to_string () << ": " << n;
      // }

    }

    if (ndiff > 0) {
      tl::info << "----------------------------------------------------";
      tl::info << "  Total differences: " << ndiff;
      ret = 1;
    }

    if (! outfile.empty ()) {

      db::SaveLayoutOptions options;
      options.set_format_from_filename (outfile);
      if (format_set) {
        options.set_format (format);
      }

      db::Writer writer (options);
      tl::OutputStream file (outfile, tl::OutputStream::OM_Auto);
      writer.write (output, file);

    }

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

  return ret;
}



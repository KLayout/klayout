
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


#include "tlStream.h"
#include "tlAssert.h"
#include "tlException.h"
#include "dbLayout.h"
#include "dbShape.h"
#include "dbPolygonTools.h"
#include "dbGDS2Writer.h"
#include "dbGDS2.h"
#include "dbGDS2Format.h"
#include "dbClip.h"
#include "dbSaveLayoutOptions.h"
#include "dbPolygonGenerators.h"

#include <stdio.h>
#include <errno.h>
#include <time.h>

#include <limits>

namespace db
{

// ------------------------------------------------------------------
//  GDS2WriterBase implementation

GDS2WriterBase::GDS2WriterBase ()
{
  // .. nothing yet ..
}

static int safe_scale (double sf, int value)
{
  double i = floor (sf * value + 0.5);
  if (i < double (std::numeric_limits<int>::min ())) {
    throw tl::Exception ("Scaling failed: coordinate underflow");
  }
  if (i > double (std::numeric_limits<int>::max ())) {
    throw tl::Exception ("Scaling failed: coordinate overflow");
  }
  return int (i);
}

inline int scale (double sf, int value)
{
  if (sf == 1.0) {
    return value;
  } else {
    return safe_scale (sf, value);
  }
}

void
GDS2WriterBase::write_context_cell (db::Layout &layout, const short *time_data, const std::vector<db::cell_index_type> &cells)
{
  write_record_size (4 + 12 * 2);
  write_record (sBGNSTR);
  write_time (time_data);
  write_time (time_data);

  write_string_record (sSTRNAME, "$$$CONTEXT_INFO$$$");

  std::vector <std::string> context_prop_strings;

  if (layout.has_context_info ()) {

    //  Use a dummy BOUNDARY element to attach the global context

    write_record_size (4);
    write_record (sBOUNDARY);

    write_record_size (6);
    write_record (sLAYER);
    write_short (0);

    write_record_size (6);
    write_record (sDATATYPE);
    write_short (0);

    write_record_size (4 + 5 * 2 * 4);
    write_record (sXY);
    for (unsigned int i = 0; i < 10; ++i) {
      write_int (0);
    }

    context_prop_strings.clear ();

    if (layout.get_context_info (context_prop_strings)) {

      //  Hint: write in the reverse order since this way, the reader is more efficient (it knows how many strings
      //  will arrive)
      for (std::vector <std::string>::const_iterator s = context_prop_strings.end (); s != context_prop_strings.begin (); ) {

        --s;

        write_record_size (6);
        write_record (sPROPATTR);
        write_short (short (std::distance (std::vector <std::string>::const_iterator (context_prop_strings.begin ()), s)));  //  = user string

        write_string_record (sPROPVALUE, *s);

      }

    }

    write_record_size (4);
    write_record (sENDEL);

  }

  for (std::vector<db::cell_index_type>::const_iterator cell = cells.begin (); cell != cells.end (); ++cell) {

    if (layout.has_context_info (*cell)) {

      write_record_size (4);
      write_record (sSREF);

      write_string_record (sSNAME, m_cell_name_map.cell_name (*cell));

      write_record_size (12);
      write_record (sXY);
      write_int (0);
      write_int (0);

      context_prop_strings.clear ();

      if (layout.get_context_info (*cell, context_prop_strings)) {

        //  Hint: write in the reverse order since this way, the reader is more efficient (it knows how many strings
        //  will arrive)
        for (std::vector <std::string>::const_iterator s = context_prop_strings.end (); s != context_prop_strings.begin (); ) {

          --s;

          write_record_size (6);
          write_record (sPROPATTR);
          write_short (short (std::distance (std::vector <std::string>::const_iterator (context_prop_strings.begin ()), s)));  //  = user string

          write_string_record (sPROPVALUE, *s);

        }

      }

      write_record_size (4);
      write_record (sENDEL);

    }

  }

  write_record_size (4);
  write_record (sENDSTR);
}

void
GDS2WriterBase::write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options)
{
  set_stream (stream);

  double dbu = (options.dbu () == 0.0) ? layout.dbu () : options.dbu ();
  double sf = options.scale_factor () * (layout.dbu () / dbu);
  if (fabs (sf - 1.0) < 1e-9) {
    //  to avoid rounding problems, set to 1.0 exactly if possible.
    sf = 1.0;
  }

  db::GDS2WriterOptions gds2_options = options.get_options<db::GDS2WriterOptions> ();

  layout.add_meta_info ("dbuu", MetaInfo (tl::to_string (tr ("Database unit in user units")), tl::to_string (dbu / std::max (1e-9, gds2_options.user_units))));
  layout.add_meta_info ("dbum", MetaInfo (tl::to_string (tr ("Database unit in meter")), tl::to_string (dbu * 1e-6)));
  layout.add_meta_info ("libname", MetaInfo (tl::to_string (tr ("Library name")), gds2_options.libname));

  std::vector <std::pair <unsigned int, db::LayerProperties> > layers;
  options.get_valid_layers (layout, layers, db::SaveLayoutOptions::LP_AssignNumber);

  std::set <db::cell_index_type> cell_set;
  options.get_cells (layout, cell_set, layers);

  //  create a cell index vector sorted bottom-up
  std::vector <db::cell_index_type> cells;
  cells.reserve (cell_set.size ());

  for (db::Layout::bottom_up_const_iterator cell = layout.begin_bottom_up (); cell != layout.end_bottom_up (); ++cell) {
    if (cell_set.find (*cell) != cell_set.end ()) {
      cells.push_back (*cell);
    }
  }

  //  get current time
  short time_data [6] = { 0, 0, 0, 0, 0, 0 };
  if (gds2_options.write_timestamps) {
    time_t ti = 0;
    time (&ti);
    const struct tm *t = localtime (&ti);
    if (t) {
      time_data[0] = t->tm_year + 1900;
      time_data[1] = t->tm_mon + 1;
      time_data[2] = t->tm_mday;
      time_data[3] = t->tm_hour;
      time_data[4] = t->tm_min;
      time_data[5] = t->tm_sec;
    }
  }

  std::string str_time = tl::sprintf ("%d/%d/%d %d:%02d:%02d", time_data[1], time_data[2], time_data[0], time_data[3], time_data[4], time_data[5]); 
  layout.add_meta_info ("mod_time", MetaInfo (tl::to_string (tr ("Modification Time")), str_time));
  layout.add_meta_info ("access_time", MetaInfo (tl::to_string (tr ("Access Time")), str_time));

  bool multi_xy = gds2_options.multi_xy_records;
  size_t max_cellname_length = std::max (gds2_options.max_cellname_length, (unsigned int)8);
  size_t max_vertex_count = std::max (gds2_options.max_vertex_count, (unsigned int)4);
  bool no_zero_length_paths = gds2_options.no_zero_length_paths;
  bool resolve_skew_arrays = gds2_options.resolve_skew_arrays;

  m_cell_name_map = db::WriterCellNameMap (max_cellname_length);
  m_cell_name_map.replacement ('$');
  m_cell_name_map.disallow_all ();
  //  TODO: restrict character set, i.e allow_standard and "$"
  m_cell_name_map.allow_all_printing ();

  //  For keep instances we need to map all cells since all can be present as instances.
  //  We use top-down assignment to make "upper cells less modified".
  if (options.keep_instances ()) {
    for (db::Layout::bottom_up_const_iterator cell = layout.end_bottom_up (); cell != layout.begin_bottom_up (); ) {
      --cell;
      m_cell_name_map.insert(*cell, layout.cell_name (*cell));
    }
  } else {
    for (std::vector<db::cell_index_type>::const_iterator cell = cells.end (); cell != cells.begin (); ) {
      --cell;
      m_cell_name_map.insert(*cell, layout.cell_name (*cell));
    }
  }

  //  write header

  write_record_size (6);
  write_record (sHEADER);
  write_short (600);

  write_record_size (4 + 12 * 2);
  write_record (sBGNLIB);
  write_time (time_data);
  write_time (time_data);

  write_string_record (sLIBNAME, gds2_options.libname);

  write_record_size (4 + 8 * 2);
  write_record (sUNITS);
  write_double (dbu / std::max (1e-9, gds2_options.user_units));
  write_double (dbu * 1e-6);

  //  layout properties 

  if (gds2_options.write_file_properties && layout.prop_id () != 0) {
    write_properties (layout, layout.prop_id ());
  }

  //  write context info
  
  bool has_context = false;

  if (options.write_context_info ()) {
    has_context = layout.has_context_info ();
    for (std::vector<db::cell_index_type>::const_iterator cell = cells.begin (); cell != cells.end () && !has_context; ++cell) {
      has_context = layout.has_context_info (*cell);
    }
  }

  if (has_context) {
    write_context_cell (layout, time_data, cells);
  }

  //  body

  for (std::vector<db::cell_index_type>::const_iterator cell = cells.begin (); cell != cells.end (); ++cell) {

    progress_checkpoint ();

    const db::Cell &cref (layout.cell (*cell));

    //  don't write ghost cells unless they are not empty (any more)
    //  also don't write proxy cells which are not employed
    if ((! cref.is_ghost_cell () || ! cref.empty ()) && (! cref.is_proxy () || ! cref.is_top ())) {

      //  cell header 

      write_record_size (4 + 12 * 2);
      write_record (sBGNSTR);
      write_time (time_data);
      write_time (time_data);

      write_string_record (sSTRNAME, m_cell_name_map.cell_name (*cell));

      //  cell body 

      if (gds2_options.write_cell_properties && cref.prop_id () != 0) {
        write_properties (layout, cref.prop_id ());
      }

      //  instances
      
      for (db::Cell::const_iterator inst = cref.begin (); ! inst.at_end (); ++inst) {

        //  write only instances to selected cells
        if (options.keep_instances () || cell_set.find (inst->cell_index ()) != cell_set.end ()) {

          progress_checkpoint ();
          write_inst (sf, *inst, true /*normalize*/, resolve_skew_arrays, layout, inst->prop_id ());

        }

      }

      //  shapes

      for (std::vector <std::pair <unsigned int, db::LayerProperties> >::const_iterator l = layers.begin (); l != layers.end (); ++l) {
   
        if (layout.is_valid_layer (l->first)) {

          int layer = l->second.layer;
          int datatype = l->second.datatype;

          db::ShapeIterator shape (cref.shapes (l->first).begin (db::ShapeIterator::Boxes | db::ShapeIterator::Polygons | db::ShapeIterator::Edges | db::ShapeIterator::EdgePairs | db::ShapeIterator::Paths | db::ShapeIterator::Texts));
          while (! shape.at_end ()) {

            progress_checkpoint ();

            if (shape->is_text ()) {
              write_text (layer, datatype, sf, dbu, *shape, layout, shape->prop_id ());
            } else if (shape->is_polygon ()) {
              write_polygon (layer, datatype, sf, *shape, multi_xy, max_vertex_count, layout, shape->prop_id ());
            } else if (shape->is_edge ()) {
              write_edge (layer, datatype, sf, *shape, layout, shape->prop_id ());
            } else if (shape->is_edge_pair ()) {
              write_edge (layer, datatype, sf, shape->edge_pair ().first (), layout, shape->prop_id ());
              write_edge (layer, datatype, sf, shape->edge_pair ().second (), layout, shape->prop_id ());
            } else if (shape->is_path ()) {
              if (no_zero_length_paths && (shape->path_length () - shape->path_extensions ().first - shape->path_extensions ().second) == 0) {
                //  eliminate the zero-width path
                db::Polygon poly;
                shape->polygon (poly);
                write_polygon (layer, datatype, sf, poly, multi_xy, max_vertex_count, layout, shape->prop_id (), false);
              } else {
                write_path (layer, datatype, sf, *shape, multi_xy, layout, shape->prop_id ());
              }
            } else if (shape->is_box ()) {
              write_box (layer, datatype, sf, *shape, layout, shape->prop_id ());
            }

            ++shape;

          }

        }

      }

      //  end of cell

      write_record_size (4);
      write_record (sENDSTR);

    }

  }

  write_record_size (4);
  write_record (sENDLIB);

  progress_checkpoint ();
}

static bool is_orthogonal (const db::Vector &rv, const db::Vector &cv)
{
  return (rv.x () == 0 && cv.y () == 0) || (rv.y () == 0 && cv.x () == 0);
}

void
GDS2WriterBase::write_inst (double sf, const db::Instance &instance, bool normalize, bool resolve_skew_arrays, const db::Layout &layout, db::properties_id_type prop_id)
{
  db::Vector a, b;
  unsigned long amax, bmax;

  bool is_reg = instance.is_regular_array (a, b, amax, bmax);

  //  skew arrays are resolved if required
  if (is_reg && ! is_orthogonal (a, b) != 0 && resolve_skew_arrays) {
    is_reg = false;
  }

  for (db::CellInstArray::iterator ii = instance.begin (); ! ii.at_end (); ++ii) {

    db::Trans t = *ii;

    if (normalize) {

      //  try to normalize orthogonal arrays into "Cadence notation", that is
      //  column and row vectors are positive in the coordinate system of the
      //  rotated array.

      if (is_reg) {

        if (amax < 2) {
          a = db::Vector ();
        }
        if (bmax < 2) {
          b = db::Vector ();
        }

        //  normalisation only works for orthogonal vectors, parallel to x or y axis, which are not parallel
        if ((a.x () == 0 || a.y () == 0) && (b.x () == 0 || b.y () == 0) && !((a.x () != 0 && b.x () != 0) || (a.y () != 0 && b.y () != 0))) {

          db::FTrans fp = db::FTrans(t.rot ()).inverted ();

          a.transform (fp);
          b.transform (fp);

          db::Vector p;
          for (int i = 0; i < 2; ++i) {

            db::Vector   *q = (i == 0) ? &a : &b;
            unsigned long n = (i == 0) ? amax : bmax;

            if (n == 0) {
              *q = db::Vector ();
            } else {
              if (q->x () < 0) {
                p += db::Vector ((n - 1) * q->x (), 0);
                q->set_x (-q->x ());
              }
              if (q->y () < 0) {
                p += db::Vector (0, (n - 1) * q->y ());
                q->set_y (-q->y ());
              }
            }

          }

          if (a.x () != 0 || b.y () != 0) {
            std::swap (a, b);
            std::swap (amax, bmax);
          }

          fp = db::FTrans (t.rot ());
          a.transform (fp);
          b.transform (fp);

          t = t * db::Trans (p);

        }

      }

    }

    write_record_size (4);
    write_record (is_reg ? sAREF : sSREF);

    write_string_record (sSNAME, m_cell_name_map.cell_name (instance.cell_index ()));

    if (t.rot () != 0 || instance.is_complex ()) {

      write_record_size (6);
      write_record (sSTRANS);
      write_short (t.is_mirror () ? 0x8000 : 0);

      if (instance.is_complex ()) {
        db::CellInstArray::complex_trans_type ct = instance.complex_trans (t);
        write_record_size (4 + 8);
        write_record (sMAG);
        write_double (ct.mag ());
        write_record_size (4 + 8);
        write_record (sANGLE);
        write_double (ct.angle ());
      } else {
        if ((t.rot () % 4) != 0) {
          write_record_size (4 + 8);
          write_record (sANGLE);
          write_double ((t.rot () % 4) * 90.0);
        }
      }

    }

    if (is_reg) {
      write_record_size (4 + 2 * 2);
      write_record (sCOLROW);
      if (amax > 32767 || bmax > 32767) {
        throw tl::Exception (tl::to_string (tr ("Cannot write array references with more than 32767 columns or rows to GDS2 streams")));
      }
      write_short (std::max ((unsigned long) 1, bmax));
      write_short (std::max ((unsigned long) 1, amax));
    }

    write_record_size (4 + (is_reg ? 3 : 1) * 2 * 4);
    write_record (sXY);
    write_int (scale (sf, t.disp ().x ()));
    write_int (scale (sf, t.disp ().y ()));

    if (is_reg) {
      write_int (scale (sf, t.disp ().x () + b.x () * bmax));
      write_int (scale (sf, t.disp ().y () + b.y () * bmax));
      write_int (scale (sf, t.disp ().x () + a.x () * amax));
      write_int (scale (sf, t.disp ().y () + a.y () * amax));
    }

    finish (layout, prop_id);

    if (is_reg) {
      //  we have already written all instances
      break;
    }

  }
}

void
GDS2WriterBase::write_box (int layer, int datatype, double sf, const db::Shape &shape, const db::Layout &layout, db::properties_id_type prop_id)
{
  db::Box box (shape.box ());

  write_record_size (4);
  write_record (sBOUNDARY);

  write_record_size (4 + 2);
  write_record (sLAYER);
  write_short (layer);

  write_record_size (4 + 2);
  write_record (sDATATYPE);
  write_short (datatype);

  write_record_size (4 + 5 * 2 * 4);
  write_record (sXY);
  write_int (scale (sf, box.left ()));
  write_int (scale (sf, box.bottom ()));
  write_int (scale (sf, box.left ()));
  write_int (scale (sf, box.top ()));
  write_int (scale (sf, box.right ()));
  write_int (scale (sf, box.top ()));
  write_int (scale (sf, box.right ()));
  write_int (scale (sf, box.bottom ()));
  write_int (scale (sf, box.left ()));
  write_int (scale (sf, box.bottom ()));

  finish (layout, prop_id);
}

void
GDS2WriterBase::write_path (int layer, int datatype, double sf, const db::Shape &shape, bool multi_xy, const db::Layout &layout, db::properties_id_type prop_id)
{
  //  instantiate the path and draw
  db::Path path;
  shape.path (path);

  write_record_size (4);
  write_record (sPATH);

  write_record_size (4 + 2);
  write_record (sLAYER);
  write_short (layer);

  write_record_size (4 + 2);
  write_record (sDATATYPE);
  write_short (datatype);

  short type = 0;
  db::Coord w = path.width ();
  db::Coord se = path.extensions ().first;
  db::Coord ee = path.extensions ().second;

  if (se == w / 2 && ee == w / 2) {
    type = path.round () ? 1 : 2;
  } else if (se == 0 && ee == 0) {
    type = 0;
  } else {
    type = 4;
  }

  write_record_size (4 + 2);
  write_record (sPATHTYPE);
  write_short (type);

  write_record_size (4 + 4);
  write_record (sWIDTH);
  write_int (scale (sf, w));

  if (type == 4) {

    write_record_size (4 + 4);
    write_record (sBGNEXTN);
    write_int (scale (sf, se));

    write_record_size (4 + 4);
    write_record (sENDEXTN);
    write_int (scale (sf, ee));

  }

  size_t n = path.points ();

  db::Path::iterator p = path.begin ();
  while (n > 0) {

    //  determine number of points to write (all or slice for multi XY mode)
    size_t nxy = n;
    if (n > 8100 && multi_xy) {
      nxy = 8000;
    }

    write_record_size (4 + int16_t (nxy) * 2 * 4);
    write_record (sXY);

    //  write path ..
    for ( ; p != path.end () && nxy > 0; ++p) {
      write_int (scale (sf, (*p).x ()));
      write_int (scale (sf, (*p).y ()));
      --nxy;
      --n;
    }

  }

  finish (layout, prop_id);
}

void
GDS2WriterBase::write_edge (int layer, int datatype, double sf, const db::Shape &shape, const db::Layout &layout, db::properties_id_type prop_id)
{
  write_edge (layer, datatype, sf, shape.edge (), layout, prop_id);
}

void
GDS2WriterBase::write_edge (int layer, int datatype, double sf, const db::Edge &e, const db::Layout &layout, db::properties_id_type prop_id)
{
  write_record_size (4);
  write_record (sPATH);

  write_record_size (4 + 2);
  write_record (sLAYER);
  write_short (layer);

  write_record_size (4 + 2);
  write_record (sDATATYPE);
  write_short (datatype);

  write_record_size (4 + 2);
  write_record (sPATHTYPE);
  write_short (0);

  write_record_size (4 + 4);
  write_record (sWIDTH);
  write_int (0);

  write_record_size (4 + 2 * 2 * 4);
  write_record (sXY);
  write_int (scale (sf, e.p1 ().x ()));
  write_int (scale (sf, e.p1 ().y ()));
  write_int (scale (sf, e.p2 ().x ()));
  write_int (scale (sf, e.p2 ().y ()));

  finish (layout, prop_id);
}

void
GDS2WriterBase::write_text (int layer, int datatype, double sf, double dbu, const db::Shape &shape, const db::Layout &layout, db::properties_id_type prop_id)
{
  db::Trans trans = shape.text_trans ();

  write_record_size (4);
  write_record (sTEXT);

  write_record_size (4 + 2);
  write_record (sLAYER);
  write_short (layer);

  write_record_size (4 + 2);
  write_record (sTEXTTYPE);
  write_short (datatype);

  if (shape.text_halign () != db::NoHAlign || shape.text_valign () != db::NoVAlign || shape.text_font () != db::NoFont) {
    short ha = short (shape.text_halign () == db::NoHAlign ? db::HAlignLeft : shape.text_halign ());
    short va = short (shape.text_valign () == db::NoVAlign ? db::VAlignBottom : shape.text_valign ());
    // HINT: currently we don't write the font since the font is not well standardized
    // short f = (shape.text_font () == db::NoFont ? 0 : (short (shape.text_font ()) & 0xfff));
    short f = 0;
    write_record_size (4 + 2);
    write_record (sPRESENTATION);
    write_short (ha + va * 4 + f * 16);
  }

  if (trans.rot () != 0 || shape.text_size () != 0) {

    write_record_size (6);
    write_record (sSTRANS);
    write_short (trans.is_mirror () ? 0x8000 : 0);

    if (shape.text_size () != 0) {
      write_record_size (4 + 8);
      write_record (sMAG);
      write_double (shape.text_size () * sf * dbu);
    }

    if ((trans.rot () % 4) != 0) {
      write_record_size (4 + 8);
      write_record (sANGLE);
      write_double ((trans.rot () % 4) * 90.0);
    }

  }

  write_record_size (4 + 2 * 4);
  write_record (sXY);
  write_int (scale (sf, trans.disp ().x ()));
  write_int (scale (sf, trans.disp ().y ()));

  write_string_record (sSTRING, shape.text_string ());

  finish (layout, prop_id);
}

void
GDS2WriterBase::write_polygon (int layer, int datatype, double sf, const db::Polygon &polygon, bool multi_xy, size_t max_vertex, const db::Layout &layout, db::properties_id_type prop_id, bool merged)
{
  bool needs_split = (polygon.vertices () > 4 && polygon.vertices () > max_vertex && !multi_xy);

  if (polygon.holes () > 0 || (! merged && needs_split)) {

    //  resolve holes or merge polygon as a preparation step for split_polygon which only works properly
    //  on merged polygons ...
    std::vector<db::Polygon> polygons;

    db::EdgeProcessor ep;
    ep.insert_sequence (polygon.begin_edge ());
    db::PolygonContainer pc (polygons);
    db::PolygonGenerator out (pc, true /*resolve holes*/, needs_split /*min coherence for splitting*/);
    db::SimpleMerge op;
    ep.process (out, op);

    for (std::vector<db::Polygon>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
      write_polygon (layer, datatype, sf, *p, multi_xy, max_vertex, layout, prop_id, true);
    }

  } else if (needs_split) {

    std::vector <db::Polygon> polygons;
    db::split_polygon (polygon, polygons);

    for (std::vector<db::Polygon>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
      write_polygon (layer, datatype, sf, *p, multi_xy, max_vertex, layout, prop_id, true /*parts need not to be merged again*/);
    }

  } else if (polygon.vertices () > 0) {

    write_record_size (4);
    write_record (sBOUNDARY);

    write_record_size (4 + 2);
    write_record (sLAYER);
    write_short (layer);

    write_record_size (4 + 2);
    write_record (sDATATYPE);
    write_short (datatype);

    size_t n = polygon.vertices ();

    db::Polygon::polygon_contour_iterator e = polygon.begin_hull ();
    while (n > 0) {

      //  determine number of points to write (all or slice for multi XY mode)
      size_t nxy = n + 1;
      if (n > 8100 && multi_xy) {
        nxy = 8000;
      }

      write_record_size (4 + int16_t (nxy) * 2 * 4);
      write_record (sXY);

      //  write polygon ..
      for ( ; e != polygon.end_hull () && nxy > 0; ++e) {
        write_int (scale (sf, (*e).x ()));
        write_int (scale (sf, (*e).y ()));
        --nxy;
        --n;
      }

      //  .. and closing point
      if (nxy > 0) {
        e = polygon.begin_hull ();
        write_int (scale (sf, (*e).x ()));
        write_int (scale (sf, (*e).y ()));
        tl_assert (n == 0);
      }

    }

    finish (layout, prop_id);

  }
}

void
GDS2WriterBase::write_polygon (int layer, int datatype, double sf, const db::Shape &shape, bool multi_xy, size_t max_vertex, const db::Layout &layout, db::properties_id_type prop_id)
{
  if (shape.holes () > 0) {

    db::Polygon polygon;
    shape.polygon (polygon);
    write_polygon (layer, datatype, sf, polygon, multi_xy, max_vertex, layout, prop_id, false);

  } else {

    //  There is no other way to determine the actual number of points of a generic shape 
    //  without instantiating a polygon:
    size_t n = 0;
    for (db::Shape::point_iterator e = shape.begin_hull (); e != shape.end_hull (); ++e) {
      ++n;
    }

    if (n > 4 && n > max_vertex && !multi_xy) {

      //  split polygons ...
      db::Polygon polygon;
      shape.polygon (polygon);
      write_polygon (layer, datatype, sf, polygon, multi_xy, max_vertex, layout, prop_id, false);

    } else if (n > 0) {

      write_record_size (4);
      write_record (sBOUNDARY);

      write_record_size (4 + 2);
      write_record (sLAYER);
      write_short (layer);

      write_record_size (4 + 2);
      write_record (sDATATYPE);
      write_short (datatype);

      db::Shape::point_iterator e (shape.begin_hull ());
      while (n > 0) {

        //  determine number of points to write (all or slice for multi XY mode)
        size_t nxy = n + 1;
        if (n > 8100 && multi_xy) {
          nxy = 8000;
        }

        write_record_size (4 + int16_t (nxy) * 2 * 4);
        write_record (sXY);

        //  write polygon ..
        for ( ; e != shape.end_hull () && nxy > 0; ++e) {
          write_int (scale (sf, (*e).x ()));
          write_int (scale (sf, (*e).y ()));
          --nxy;
          --n;
        }

        //  .. and closing point
        if (nxy > 0) {
          e = shape.begin_hull ();
          write_int (scale (sf, (*e).x ()));
          write_int (scale (sf, (*e).y ()));
          tl_assert (n == 0);
        }

      }

      finish (layout, prop_id);

    }
  }
}

void 
GDS2WriterBase::write_properties (const db::Layout &layout, db::properties_id_type prop_id)
{
  const db::PropertiesRepository::properties_set &props = layout.properties_repository ().properties (prop_id);
  for (db::PropertiesRepository::properties_set::const_iterator p = props.begin (); p != props.end (); ++p) {

    const tl::Variant &name = layout.properties_repository ().prop_name (p->first);

    long attr = -1;
    if (name.can_convert_to_long ()) {
      attr = name.to_long ();
    }

    if (attr >= 0 && attr < 65535) {

      write_record_size (6);
      write_record (sPROPATTR);
      write_short (attr);

      write_string_record (sPROPVALUE, p->second.to_string ());

    }

  }
}

void
GDS2WriterBase::finish (const db::Layout &layout, db::properties_id_type prop_id)
{
  if (prop_id != 0) {
    write_properties (layout, prop_id);
  }

  write_record_size (4);
  write_record (sENDEL);
}

void 
GDS2WriterBase::write_string_record (short record, const std::string &t)
{
  write_record_size (4 + (int16_t (t.size () + 1) / 2) * 2);
  write_record (record);
  write_string (t);
}

} // namespace db


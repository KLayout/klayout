
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
//  Limit checking conversion functions

static int32_t safe_convert_to_int32 (int32_t value)
{
  return value;
}

static int32_t safe_convert_to_int32 (uint32_t value)
{
  if (value > std::numeric_limits<int32_t>::max ()) {
    throw tl::Exception (tl::to_string (tr ("Coordinate overflow")));
  }
  return int32_t (value);
}

static int32_t safe_convert_to_int32 (int64_t value)
{
  if (value < std::numeric_limits<int32_t>::min ()) {
    throw tl::Exception (tl::to_string (tr ("Coordinate underflow")));
  }
  if (value > std::numeric_limits<int32_t>::max ()) {
    throw tl::Exception (tl::to_string (tr ("Coordinate overflow")));
  }
  return int32_t (value);
}

static int32_t safe_convert_to_int32 (uint64_t value)
{
  if (value > std::numeric_limits<int32_t>::max ()) {
    throw tl::Exception (tl::to_string (tr ("Coordinate overflow")));
  }
  return int32_t (value);
}

template <class T>
static int32_t safe_scale (double sf, T value)
{
  double i = floor (sf * value + 0.5);
  if (i < double (std::numeric_limits<int32_t>::min ())) {
    throw tl::Exception (tl::to_string (tr ("Scaling failed: coordinate underflow")));
  }
  if (i > double (std::numeric_limits<int32_t>::max ())) {
    throw tl::Exception (tl::to_string (tr ("Scaling failed: coordinate overflow")));
  }
  return int32_t (i);
}

template <class T>
inline int32_t scale (double sf, T value)
{
  if (sf == 1.0) {
    return safe_convert_to_int32 (value);
  } else {
    return safe_scale (sf, value);
  }
}

static uint16_t safe_convert_to_uint16 (int16_t value)
{
  //  we accept this as GDS is not well defined here ...
  return value;
}

static uint16_t safe_convert_to_uint16 (uint16_t value)
{
  return value;
}

static uint16_t safe_convert_to_uint16 (int32_t value)
{
  if (value < 0) {
    throw tl::Exception (tl::to_string (tr ("Short unsigned integer underflow")));
  }
  if (value > std::numeric_limits<uint16_t>::max ()) {
    throw tl::Exception (tl::to_string (tr ("Short unsigned integer overflow")));
  }
  return uint16_t (value);
}

static uint16_t safe_convert_to_uint16 (uint32_t value)
{
  if (value > std::numeric_limits<uint16_t>::max ()) {
    throw tl::Exception (tl::to_string (tr ("Short unsigned integer overflow")));
  }
  return uint16_t (value);
}

static uint16_t safe_convert_to_uint16 (int64_t value)
{
  if (value < 0) {
    throw tl::Exception (tl::to_string (tr ("Short unsigned integer underflow")));
  }
  if (value > std::numeric_limits<uint16_t>::max ()) {
    throw tl::Exception (tl::to_string (tr ("Short unsigned integer overflow")));
  }
  return uint16_t (value);
}

static uint16_t safe_convert_to_uint16 (uint64_t value)
{
  if (value > std::numeric_limits<uint16_t>::max ()) {
    throw tl::Exception (tl::to_string (tr ("Short unsigned integer overflow")));
  }
  return uint16_t (value);
}

// ------------------------------------------------------------------
//  GDS2WriterBase implementation

GDS2WriterBase::GDS2WriterBase ()
  : m_dbu (0.0), m_resolve_skew_arrays (false), m_multi_xy (false), m_no_zero_length_paths (false),
    m_max_vertex_count (0), m_write_cell_properties (false), m_keep_instances (false)
{
  //  .. nothing yet ..
}

void
GDS2WriterBase::write_context_string (size_t n, const std::string &s)
{
  //  max. size for GDS strings used as payload carrier
  size_t chunk_size = 32000;
  short max_short = std::numeric_limits<int16_t>::max ();

  if (n > size_t (max_short) || s.size () > chunk_size) {

    //  Split strings and use a separate notation: "#<n>,<+p>:..." for the partial
    //  strings. n is the string index and p the part index (zero based).
    //  The property number is not defined in that case. There may be properties with
    //  the same number. See issue #1794.

    size_t nchunks = (s.size () + (chunk_size - 1)) / chunk_size;
    while (nchunks > 0) {

      --nchunks;

      std::string partial;
      partial.reserve (chunk_size + 100); // approx.
      partial += "#";
      partial += tl::to_string (n);
      partial += ",";
      partial += tl::to_string (nchunks);
      partial += ":";
      size_t pos = nchunks * chunk_size;
      partial += std::string (s, pos, std::min (s.size (), pos + chunk_size) - pos);

      write_record_size (6);
      write_record (sPROPATTR);
      write_short (n <= size_t (max_short) ? short (n) : max_short);

      write_string_record (sPROPVALUE, partial);

    }

  } else {

    write_record_size (6);
    write_record (sPROPATTR);
    write_short (int16_t (n));

    write_string_record (sPROPVALUE, s);

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
        size_t n = std::distance (std::vector <std::string>::const_iterator (context_prop_strings.begin ()), s);
        write_context_string (n, *s);
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
          size_t n = std::distance (std::vector <std::string>::const_iterator (context_prop_strings.begin ()), s);
          write_context_string (n, *s);
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
GDS2WriterBase::write_shape (const db::Layout &layout, int layer, int datatype, const db::Shape &shape, double sf)
{
  if (shape.is_text ()) {

    write_text (layer, datatype, sf, m_dbu, shape, layout, shape.prop_id ());

  } else if (shape.is_polygon ()) {

    write_polygon (layer, datatype, sf, shape, m_multi_xy, m_max_vertex_count, layout, shape.prop_id ());

  } else if (shape.is_edge ()) {

    write_edge (layer, datatype, sf, shape, layout, shape.prop_id ());

  } else if (shape.is_edge_pair ()) {

    write_edge (layer, datatype, sf, shape.edge_pair ().first (), layout, shape.prop_id ());
    write_edge (layer, datatype, sf, shape.edge_pair ().second (), layout, shape.prop_id ());

  } else if (shape.is_path ()) {

    if (m_no_zero_length_paths && (shape.path_length () - shape.path_extensions ().first - shape.path_extensions ().second) == 0) {
      //  eliminate the zero-width path
      db::Polygon poly;
      shape.polygon (poly);
      write_polygon (layer, datatype, sf, poly, m_multi_xy, m_max_vertex_count, layout, shape.prop_id (), false);
    } else {
      write_path (layer, datatype, sf, shape, m_multi_xy, layout, shape.prop_id ());
    }

  } else if (shape.is_box ()) {

    write_box (layer, datatype, sf, shape, layout, shape.prop_id ());

  }
}

void
GDS2WriterBase::write_cell (db::Layout &layout, const db::Cell &cref, const std::vector <std::pair <unsigned int, db::LayerProperties> > &layers, const std::set<db::cell_index_type> &cell_set, double sf, short *time_data)
{
  //  cell header

  write_record_size (4 + 12 * 2);
  write_record (sBGNSTR);
  write_time (time_data);
  write_time (time_data);

  try {
    write_string_record (sSTRNAME, m_cell_name_map.cell_name (cref.cell_index ()));
  } catch (tl::Exception &ex) {
    throw tl::Exception (ex.msg () + tl::to_string (tr (", writing cell name")));
  }

  //  cell body

  if (m_write_cell_properties && cref.prop_id () != 0) {
    try {
      write_properties (layout, cref.prop_id ());
    } catch (tl::Exception &ex) {
      throw tl::Exception (ex.msg () + tl::to_string (tr (", writing layout properties")));
    }
  }

  //  instances

  for (db::Cell::const_iterator inst = cref.begin (); ! inst.at_end (); ++inst) {

    //  write only instances to selected cells
    if (m_keep_instances || cell_set.find (inst->cell_index ()) != cell_set.end ()) {

      progress_checkpoint ();
      try {
        write_inst (sf, *inst, true /*normalize*/, m_resolve_skew_arrays, layout, inst->prop_id ());
      } catch (tl::Exception &ex) {
        throw tl::Exception (ex.msg () + tl::to_string (tr (", writing instances")));
      }

    }

  }

  //  shapes

  for (std::vector <std::pair <unsigned int, db::LayerProperties> >::const_iterator l = layers.begin (); l != layers.end (); ++l) {

    if (layout.is_valid_layer (l->first) && l->second.layer >= 0 && l->second.datatype >= 0) {

      int layer = l->second.layer;
      if (layer > std::numeric_limits<uint16_t>::max ()) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Cannot write layer numbers larger than %d to GDS2 streams")), int (std::numeric_limits<uint16_t>::max ())));
      }
      int datatype = l->second.datatype;
      if (datatype > std::numeric_limits<uint16_t>::max ()) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Cannot write datatype numbers larger than %d to GDS2 streams")), int (std::numeric_limits<uint16_t>::max ())));
      }

      db::ShapeIterator shape (cref.shapes (l->first).begin (db::ShapeIterator::Boxes | db::ShapeIterator::Polygons | db::ShapeIterator::Edges | db::ShapeIterator::EdgePairs | db::ShapeIterator::Paths | db::ShapeIterator::Texts));
      while (! shape.at_end ()) {

        progress_checkpoint ();

        try {
          write_shape (layout, layer, datatype, *shape, sf);
        } catch (tl::Exception &ex) {
          throw tl::Exception (ex.msg () + tl::sprintf (tl::to_string (tr (", writing layer %d/%d")), layer, datatype));
        }

        ++shape;

      }

    }

  }

  //  end of cell

  write_record_size (4);
  write_record (sENDSTR);
}

void
GDS2WriterBase::write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options)
{
  set_stream (stream);

  m_dbu = (options.dbu () == 0.0) ? layout.dbu () : options.dbu ();
  double sf = options.scale_factor () * (layout.dbu () / m_dbu);
  if (fabs (sf - 1.0) < 1e-9) {
    //  to avoid rounding problems, set to 1.0 exactly if possible.
    sf = 1.0;
  }

  db::GDS2WriterOptions gds2_options = options.get_options<db::GDS2WriterOptions> ();

  layout.add_meta_info ("dbuu", MetaInfo (tl::to_string (tr ("Database unit in user units")), tl::to_string (m_dbu / std::max (1e-9, gds2_options.user_units))));
  layout.add_meta_info ("dbum", MetaInfo (tl::to_string (tr ("Database unit in meter")), tl::to_string (m_dbu * 1e-6)));
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

  m_keep_instances = options.keep_instances ();
  m_multi_xy = gds2_options.multi_xy_records;
  m_max_vertex_count = std::max (gds2_options.max_vertex_count, (unsigned int)4);
  m_no_zero_length_paths = gds2_options.no_zero_length_paths;
  m_resolve_skew_arrays = gds2_options.resolve_skew_arrays;
  m_write_cell_properties = gds2_options.write_cell_properties;

  size_t max_cellname_length = std::max (gds2_options.max_cellname_length, (unsigned int)8);

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

  try {
    write_string_record (sLIBNAME, gds2_options.libname);
  } catch (tl::Exception &ex) {
    throw tl::Exception (ex.msg () + tl::to_string (tr (", writing LIBNAME")));
  }

  write_record_size (4 + 8 * 2);
  write_record (sUNITS);
  write_double (m_dbu / std::max (1e-9, gds2_options.user_units));
  write_double (m_dbu * 1e-6);

  //  layout properties 

  if (gds2_options.write_file_properties && layout.prop_id () != 0) {
    try {
      write_properties (layout, layout.prop_id ());
    } catch (tl::Exception &ex) {
      throw tl::Exception (ex.msg () + tl::to_string (tr (", writing layout properties")));
    }
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
    try {
      write_context_cell (layout, time_data, cells);
    } catch (tl::Exception &ex) {
      throw tl::Exception (ex.msg () + tl::to_string (tr (", writing context cell")));
    }
  }

  //  body

  for (std::vector<db::cell_index_type>::const_iterator cell = cells.begin (); cell != cells.end (); ++cell) {

    progress_checkpoint ();

    const db::Cell &cref (layout.cell (*cell));

    //  don't write ghost cells unless they are not empty (any more)
    //  also don't write proxy cells which are not employed
    if ((! cref.is_ghost_cell () || ! cref.empty ()) && (! cref.is_proxy () || ! cref.is_top ())) {

      try {
        write_cell (layout, cref, layers, cell_set, sf, time_data);
      } catch (tl::Exception &ex) {
        throw tl::Exception (ex.msg () + tl::sprintf (tl::to_string (tr (", writing cell '%s'")), layout.cell_name (*cell)));
      }

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
      if (amax > std::numeric_limits<int16_t>::max () || bmax > std::numeric_limits<int16_t>::max ()) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Cannot write array references with more than %d columns or rows to GDS2 streams")), int (std::numeric_limits<int16_t>::max ())));
      }
      write_short (std::max ((unsigned long) 1, bmax));
      write_short (std::max ((unsigned long) 1, amax));
    }

    write_record_size (4 + (is_reg ? 3 : 1) * 2 * 4);
    write_record (sXY);
    write_int (scale (sf, t.disp ().x ()));
    write_int (scale (sf, t.disp ().y ()));

    if (is_reg) {
      write_int (scale (sf, t.disp ().x () + b.x () * (int64_t) bmax));
      write_int (scale (sf, t.disp ().y () + b.y () * (int64_t) bmax));
      write_int (scale (sf, t.disp ().x () + a.x () * (int64_t) amax));
      write_int (scale (sf, t.disp ().y () + a.y () * (int64_t) amax));
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
  write_short (safe_convert_to_uint16 (layer));

  write_record_size (4 + 2);
  write_record (sDATATYPE);
  write_short (safe_convert_to_uint16 (datatype));

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
  write_short (safe_convert_to_uint16 (layer));

  write_record_size (4 + 2);
  write_record (sDATATYPE);
  write_short (safe_convert_to_uint16 (datatype));

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
  write_short (safe_convert_to_uint16 (layer));

  write_record_size (4 + 2);
  write_record (sDATATYPE);
  write_short (safe_convert_to_uint16 (datatype));

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
  write_short (safe_convert_to_uint16 (layer));

  write_record_size (4 + 2);
  write_record (sTEXTTYPE);
  write_short (safe_convert_to_uint16 (datatype));

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
    write_short (safe_convert_to_uint16 (layer));

    write_record_size (4 + 2);
    write_record (sDATATYPE);
    write_short (safe_convert_to_uint16 (datatype));

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
      write_short (safe_convert_to_uint16 (layer));

      write_record_size (4 + 2);
      write_record (sDATATYPE);
      write_short (safe_convert_to_uint16 (datatype));

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

    if (attr >= 0 && attr <= std::numeric_limits<uint16_t>::max ()) {

      write_record_size (6);
      write_record (sPROPATTR);
      write_short ((int16_t) attr);

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
  size_t rs = 4 + ((t.size () + 1) / 2) * 2;
  if (rs > std::numeric_limits<uint16_t>::max ()) {
    throw tl::Exception (tl::to_string (tr ("String max. length overflow")));
  }
  write_record_size (uint16_t (rs));
  write_record (record);
  write_string (t);
}

} // namespace db


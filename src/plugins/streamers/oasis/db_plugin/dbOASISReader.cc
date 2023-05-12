
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



#include "dbOASISReader.h"
#include "dbCommonReader.h"
#include "dbStream.h"
#include "dbObjectWithProperties.h"
#include "dbArray.h"
#include "dbStatic.h"

#include "tlException.h"
#include "tlString.h"
#include "tlClassRegistry.h"

namespace db
{

// ---------------------------------------------------------------
//  OASISReader

OASISReader::OASISReader (tl::InputStream &s)
  : m_stream (s),
    m_progress (tl::to_string (tr ("Reading OASIS file")), 10000),
    m_dbu (0.001),
    m_expect_strict_mode (-1),
    mm_repetition (this, "repetition"),
    mm_placement_cell (this, "placement-cell"),
    mm_placement_x (this, "playcement-x"),
    mm_placement_y (this, "playcement-y"),
    mm_layer (this, "layer"),
    mm_datatype (this, "datatype"),
    mm_textlayer (this, "textlayer"),
    mm_texttype (this, "texttype"),
    mm_text_x (this, "text-x"),
    mm_text_y (this, "text-y"),
    mm_text_string (this, "text-string"),
    mm_text_string_id (this, "text-string-id"),
    mm_geometry_x (this, "geometry-x"),
    mm_geometry_y (this, "geometry-y"),
    mm_geometry_w (this, "geometry-w"),
    mm_geometry_h (this, "geometry-h"),
    mm_polygon_point_list (this, "polygon-point-list"),
    mm_path_halfwidth (this, "path-halfwidth"),
    mm_path_start_extension (this, "path-start-extension"),
    mm_path_end_extension (this, "path-end-extension"),
    mm_path_point_list (this, "path-point-list"),
    mm_ctrapezoid_type (this, "ctrapezoid-type"),
    mm_circle_radius (this, "circle-radius"),
    mm_last_property_name (this, "last-property-name"),
    mm_last_property_is_sprop (this, "last-property-is-stdprop"),
    mm_last_value_list(this, "last-value-list"),
    m_read_texts (true),
    m_read_properties (true),
    m_read_all_properties (false),
    m_s_gds_property_name_id (0),
    m_klayout_context_property_name_id (0)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
  m_first_cellname = 0;
  m_first_propname = 0;
  m_first_propstring = 0;
  m_first_textstring = 0;
  m_first_layername = 0;
  m_in_table = NotInTable;
  m_table_cellname = 0;
  m_table_propname = 0;
  m_table_propstring = 0;
  m_table_textstring = 0;
  m_table_layername = 0;
  m_table_start = 0;
}

OASISReader::~OASISReader ()
{
  //  .. nothing yet ..
}

void
OASISReader::init (const db::LoadLayoutOptions &options)
{
  CommonReader::init (options);

  db::CommonReaderOptions common_options = options.get_options<db::CommonReaderOptions> ();
  m_read_texts = common_options.enable_text_objects;
  m_read_properties = common_options.enable_properties;

  db::OASISReaderOptions oasis_options = options.get_options<db::OASISReaderOptions> ();
  m_read_all_properties = oasis_options.read_all_properties;
  m_expect_strict_mode = oasis_options.expect_strict_mode;
}

inline long long
OASISReader::get_long_long ()
{
  unsigned long long u = get_ulong_long ();
  if ((u & 1) != 0) {
    return -(long long) (u >> 1);
  } else {
    return (long long) (u >> 1);
  }
}

inline unsigned long long
OASISReader::get_ulong_long ()
{
  unsigned long long v = 0;
  unsigned long long vm = 1;
  char c;

  do {
    unsigned char *b = (unsigned char *) m_stream.get (1);
    if (! b) {
      error (tl::to_string (tr ("Unexpected end-of-file")));
      return 0;
    }
    c = *b;
    if (vm > std::numeric_limits <unsigned long long>::max () / 128 &&
        (unsigned long long) (c & 0x7f) > (std::numeric_limits <unsigned long long>::max () / vm)) {
      error (tl::to_string (tr ("Unsigned long value overflow")));
    }
    v += (unsigned long long) (c & 0x7f) * vm;
    vm <<= 7;
  } while ((c & 0x80) != 0);

  return v;
}

inline long
OASISReader::get_long ()
{
  unsigned long u = get_ulong ();
  if ((u & 1) != 0) {
    return -long (u >> 1);
  } else {
    return long (u >> 1);
  }
}

inline unsigned long
OASISReader::get_ulong_for_divider ()
{
  unsigned long l = get_ulong ();
  if (l == 0) {
    error (tl::to_string (tr ("Divider must not be zero")));
  }
  return l;
}

inline unsigned long
OASISReader::get_ulong ()
{
  unsigned long v = 0;
  unsigned long vm = 1;
  char c;

  do {
    unsigned char *b = (unsigned char *) m_stream.get (1);
    if (! b) {
      error (tl::to_string (tr ("Unexpected end-of-file")));
      return 0;
    }
    c = *b;
    if (vm > std::numeric_limits <unsigned long>::max () / 128 &&
        (unsigned long) (c & 0x7f) > (std::numeric_limits <unsigned long>::max () / vm)) {
      error (tl::to_string (tr ("Unsigned long value overflow")));
    }
    v += (unsigned long) (c & 0x7f) * vm;
    vm <<= 7;
  } while ((c & 0x80) != 0);

  return v;
}

inline int
OASISReader::get_int ()
{
  unsigned int u = get_uint ();
  if ((u & 1) != 0) {
    return -int (u >> 1);
  } else {
    return int (u >> 1);
  }
}

inline unsigned int
OASISReader::get_uint ()
{
  unsigned int v = 0;
  unsigned int vm = 1;
  char c;

  do {
    unsigned char *b = (unsigned char *) m_stream.get (1);
    if (! b) {
      error (tl::to_string (tr ("Unexpected end-of-file")));
      return 0;
    }
    c = *b;
    if (vm > std::numeric_limits <unsigned int>::max () / 128 &&
        (unsigned int) (c & 0x7f) > (std::numeric_limits <unsigned int>::max () / vm)) {
      error (tl::to_string (tr ("Unsigned integer value overflow")));
    }
    v += (unsigned int) (c & 0x7f) * vm;
    vm <<= 7;
  } while ((c & 0x80) != 0);

  return v;
}

std::string
OASISReader::get_str ()
{
  std::string s;
  get_str (s);
  return s;
}

void
OASISReader::get_str (std::string &s)
{
  size_t l = 0;
  get (l);

  char *b = (char *) m_stream.get (l);
  if (b) {
    s.assign (b, l);
  } else {
    s = std::string ();
  }
}

double
OASISReader::get_real ()
{
  unsigned int t = get_uint ();

  if (t == 0) {

    return double (get_ulong ());

  } else if (t == 1) {

    return -double (get_ulong ());

  } else if (t == 2) {

    return 1.0 / double (get_ulong_for_divider ());

  } else if (t == 3) {

    return -1.0 / double (get_ulong_for_divider ());

  } else if (t == 4) {

    double d = double (get_ulong ());
    return d / double (get_ulong_for_divider ());

  } else if (t == 5) {

    double d = double (get_ulong ());
    return -d / double (get_ulong_for_divider ());

  } else if (t == 6) {

    union {
      float f;
      uint32_t i;
    } i2f;

    unsigned char *b = (unsigned char *) m_stream.get (sizeof (i2f.i));
    if (! b) {
      error (tl::to_string (tr ("Unexpected end-of-file")));
    }
    i2f.i = 0;
    b += sizeof (i2f.i);
    for (unsigned int i = 0; i < sizeof (i2f.i); ++i) {
      i2f.i = (i2f.i << 8) + uint32_t (*--b);
    }

    return double (i2f.f);

  } else if (t == 7) {

    union {
      double d;
      uint64_t i;
    } i2f;

    unsigned char *b = (unsigned char *) m_stream.get (sizeof (i2f.i));
    if (! b) {
      error (tl::to_string (tr ("Unexpected end-of-file")));
    }
    i2f.i = 0;
    b += sizeof (i2f.i);
    for (unsigned int i = 0; i < sizeof (i2f.i); ++i) {
      i2f.i = (i2f.i << 8) + uint64_t (*--b);
    }

    return double (i2f.d);

  } else {
    error (tl::sprintf (tl::to_string (tr ("Invalid real type %d")), t));
    return 0.0;
  }
}

db::Coord
OASISReader::get_ucoord (unsigned long grid)
{
  unsigned long long lx = 0;
  get (lx);
  lx *= grid;
  if (lx > (unsigned long long) (std::numeric_limits <db::Coord>::max ())) {
    error (tl::to_string (tr ("Coordinate value overflow")));
  }
  return db::Coord (lx);
}

OASISReader::distance_type
OASISReader::get_ucoord_as_distance (unsigned long grid)
{
  unsigned long long lx = 0;
  get (lx);
  lx *= grid;
  if (lx > (unsigned long long) (std::numeric_limits <distance_type>::max ())) {
    error (tl::to_string (tr ("Coordinate value overflow")));
  }
  return distance_type (lx);
}

db::Coord
OASISReader::get_coord (long grid)
{
  long long lx = 0;
  get (lx);
  lx *= grid;
  if (lx < (long long) (std::numeric_limits <db::Coord>::min ()) ||
      lx > (long long) (std::numeric_limits <db::Coord>::max ())) {
    error (tl::to_string (tr ("Coordinate value overflow")));
  }
  return db::Coord (lx);
}

db::Vector
OASISReader::get_2delta (long grid)
{
  unsigned long long l1 = 0;
  get (l1);

  long long lx = l1 >> 2;
  lx *= grid;
  if (lx > (long long) (std::numeric_limits <db::Coord>::max ())) {
    error (tl::to_string (tr ("Coordinate value overflow")));
  }
  db::Coord x = lx;

  switch (l1 & 3) {
  case 0:
    return db::Vector (x, 0);
  case 1:
    return db::Vector (0, x);
  case 2:
    return db::Vector (-x, 0);
  case 3:
  default:
    return db::Vector (0, -x);
  }
}

db::Vector
OASISReader::get_3delta (long grid)
{
  unsigned long long l1 = 0;
  get (l1);

  long long lx = l1 >> 3;
  lx *= grid;
  if (lx > (long long) (std::numeric_limits <db::Coord>::max ())) {
    error (tl::to_string (tr ("Coordinate value overflow")));
  }
  db::Coord x = lx;

  switch (l1 & 7) {
  case 0:
    return db::Vector (x, 0);
  case 1:
    return db::Vector (0, x);
  case 2:
    return db::Vector (-x, 0);
  case 3:
    return db::Vector (0, -x);
  case 4:
    return db::Vector (x, x);
  case 5:
    return db::Vector (-x, x);
  case 6:
    return db::Vector (-x, -x);
  case 7:
  default:
    return db::Vector (x, -x);
  }
}

db::Vector
OASISReader::get_gdelta (long grid)
{
  unsigned long long l1 = 0;
  get (l1);

  if ((l1 & 1) != 0) {

    long long lx = ((l1 & 2) == 0 ? (long long) (l1 >> 2) : -(long long) (l1 >> 2));
    lx *= grid;
    if (lx < (long long) (std::numeric_limits <db::Coord>::min ()) ||
        lx > (long long) (std::numeric_limits <db::Coord>::max ())) {
      error (tl::to_string (tr ("Coordinate value overflow")));
    }

    long long ly;
    get (ly);
    ly *= grid;
    if (ly < (long long) (std::numeric_limits <db::Coord>::min ()) ||
        ly > (long long) (std::numeric_limits <db::Coord>::max ())) {
      error (tl::to_string (tr ("Coordinate value overflow")));
    }

    return db::Vector (db::Coord (lx), db::Coord (ly));

  } else {

    long long lx = l1 >> 4;
    lx *= grid;
    if (lx > (long long) (std::numeric_limits <db::Coord>::max ())) {
      error (tl::to_string (tr ("Coordinate value overflow")));
    }
    db::Coord x = lx;

    switch ((l1 >> 1) & 7) {
    case 0:
      return db::Vector (x, 0);
    case 1:
      return db::Vector (0, x);
    case 2:
      return db::Vector (-x, 0);
    case 3:
      return db::Vector (0, -x);
    case 4:
      return db::Vector (x, x);
    case 5:
      return db::Vector (-x, x);
    case 6:
      return db::Vector (-x, -x);
    case 7:
    default:
      return db::Vector (x, -x);
    }

  }
}

void
OASISReader::error (const std::string &msg)
{
  throw OASISReaderException (msg, m_stream.pos (), m_cellname.c_str ());
}

void
OASISReader::warn (const std::string &msg, int wl)
{
  if (warn_level () < wl) {
    return;
  }

  if (warnings_as_errors ()) {
    error (msg);
  } else {
    // TODO: compress
    tl::warn << msg
             << tl::to_string (tr (" (position=")) << m_stream.pos ()
             << tl::to_string (tr (", cell=")) << m_cellname
             << ")";
  }
}

/**
 *  @brief A helper class to join two datatype layer name map members
 */
struct LNameJoinOp1
{
  void operator() (std::string &a, const std::string &b)
  {
    join_layer_names (a, b);
  }
};

/**
 *  @brief A helper class to join two layer map members
 *  This implementation basically merged the datatype maps.
 */
struct LNameJoinOp2
{
  void operator() (tl::interval_map<db::ld_type, std::string> &a, const tl::interval_map<db::ld_type, std::string> &b)
  {
    LNameJoinOp1 op1;
    a.add (b.begin (), b.end (), op1);
  }
};

/**
 *  @brief Marks the beginning of a new table
 *
 *  This method will update m_table_start which is the location used as
 *  the start position of a strict mode table. Every record except CBLOCK
 *  will update this position to point after the record. Hence m_table_start
 *  points to the beginning of a table when PROPNAME, CELLNAME or any
 *  other table-contained record is encountered.
 *  Since CBLOCK does not update this record, the position of the table will
 *  be the location of CBLOCK rather than that of the name record itself.
 *  PAD records will also call this method, so the beginning of a table
 *  is right after any preceding PAD records and exactly at the location
 *  of the first name record after PADs.
 */
void
OASISReader::mark_start_table ()
{
  //  we need to this this to really finish a CBLOCK - this is a flaw
  //  in the inflating reader, but it's hard to fix.
  get_byte ();
  m_stream.unget (1);

  //  now we can fetch the position
  m_table_start = m_stream.pos ();
}

void
OASISReader::read_offset_table ()
{
  unsigned int of = 0;

  of = get_uint ();
  get (m_table_cellname);
  if (m_table_cellname != 0 && m_expect_strict_mode >= 0 && ((of == 0) != (m_expect_strict_mode == 0))) {
    warn (tl::to_string (tr ("CELLNAME offset table has unexpected strict mode")));
  }

  of = get_uint ();
  get (m_table_textstring);
  if (m_table_textstring != 0 && m_expect_strict_mode >= 0 && ((of == 0) != (m_expect_strict_mode == 0))) {
    warn (tl::to_string (tr ("TEXTSTRING offset table has unexpected strict mode")));
  }

  of = get_uint ();
  get (m_table_propname);
  if (m_table_propname != 0 && m_expect_strict_mode >= 0 && ((of == 0) != (m_expect_strict_mode == 0))) {
    warn (tl::to_string (tr ("PROPNAME offset table has unexpected strict mode")));
  }

  of = get_uint ();
  get (m_table_propstring);
  if (m_table_propstring != 0 && m_expect_strict_mode >= 0 && ((of == 0) != (m_expect_strict_mode == 0))) {
    warn (tl::to_string (tr ("PROPSTRING offset table has unexpected strict mode")));
  }

  of = get_uint ();
  get (m_table_layername);
  if (m_table_layername != 0 && m_expect_strict_mode >= 0 && ((of == 0) != (m_expect_strict_mode == 0))) {
    warn (tl::to_string (tr ("LAYERNAME offset table has unexpected strict mode")));
  }

  //  XNAME table ignored currently
  get_uint ();
  size_t dummy = 0;
  get (dummy);
}

static const char magic_bytes[] = { "%SEMI-OASIS\015\012" };

static const char *klayout_context_propname = "KLAYOUT_CONTEXT";
static const char *s_gds_property_propname = "S_GDS_PROPERTY";

static LayoutOrCellContextInfo
make_context_info (const std::vector<tl::Variant> &context_properties)
{
  std::vector<std::string> context_strings;
  context_strings.reserve (context_properties.size ());
  for (auto s = context_properties.begin (); s != context_properties.end (); ++s) {
    context_strings.push_back (s->to_string ());
  }
  return LayoutOrCellContextInfo::deserialize (context_strings.begin (), context_strings.end ());
}

void
OASISReader::do_read (db::Layout &layout)
{
  unsigned char r;
  char *mb;

  //  prepare
  m_s_gds_property_name_id = layout.properties_repository ().prop_name_id (s_gds_property_propname);
  m_klayout_context_property_name_id = layout.properties_repository ().prop_name_id (klayout_context_propname);

  //  read magic bytes
  mb = (char *) m_stream.get (sizeof (magic_bytes) - 1);
  if (! mb) {
    error (tl::to_string (tr ("File too short")));
    return;
  }
  if (strncmp (mb, magic_bytes, sizeof (magic_bytes) - 1) != 0) {
    error (tl::to_string (tr ("Format error (missing magic bytes)")));
  }

  //  read first record
  r = get_byte ();
  if (r != 1 /*START*/) {
    error (tl::to_string (tr ("Format error (START record expected)")));
  }

  std::string v = get_str ();
  if (v != "1.0") {
    error (tl::sprintf (tl::to_string (tr ("Format error (only version 1.0 is supported, file has version %s)")), v));
  }

  double res = get_real ();
  if (res < 1e-6) {
    error (tl::sprintf (tl::to_string (tr ("Invalid resolution of %g")), res));
  }

  //  compute database unit in pixel per meter
  m_dbu = 1.0e-6 / res;
  layout.dbu (m_dbu * 1e6);

  //  read over table offsets if required
  bool table_offsets_at_end = get_uint ();
  if (! table_offsets_at_end) {
    read_offset_table ();
  }

  //  reset the strict mode checking locations
  m_first_cellname = 0;
  m_first_propname = 0;
  m_first_propstring = 0;
  m_first_textstring = 0;
  m_first_layername = 0;
  m_in_table = NotInTable;
  m_table_cellname = 0;
  m_table_propname = 0;
  m_table_propstring = 0;
  m_table_textstring = 0;
  m_table_layername = 0;

  //  define the name id counters
  unsigned long cellname_id = 0;
  unsigned long textstring_id = 0;
  unsigned long propstring_id = 0;
  unsigned long propname_id = 0;

  //  id mode (explicit or implicit)
  enum id_mode { any, expl, impl };
  id_mode cellname_id_mode = any;
  id_mode textstring_id_mode = any;
  id_mode propstring_id_mode = any;
  id_mode propname_id_mode = any;

  m_cellname_properties.clear ();
  m_textstrings.clear ();
  m_propstrings.clear ();
  m_propnames.clear ();

  m_context_strings_per_cell.clear ();

  m_instances.clear ();
  m_instances_with_props.clear ();

  db::PropertiesRepository::properties_set layout_properties;
  std::vector <tl::Variant> context_properties;

  mark_start_table ();

  //  read next record
  while (true) {

    r = get_byte ();

    if (r == 0 /*PAD*/) {

      //  simply skip.
      mark_start_table ();

    } else if (r == 2 /*END*/) {

      //  done
      break;

    } else if (r == 3 || r == 4 /*CELLNAME*/) {

      if (m_first_cellname == 0) {
        m_first_cellname = m_table_start;
      } else if (m_expect_strict_mode == 1 && m_in_table != InCELLNAME && m_first_cellname != 0) {
        warn (tl::to_string (tr ("CELLNAME outside table in strict mode")));
      }
      m_in_table = InCELLNAME;

      //  there cannot be more file level properties .. store what we have
      if (! layout_properties.empty ()) {
        layout.prop_id (layout.properties_repository ().properties_id (layout_properties));
        layout_properties.clear ();
      }

      //  read a cell name
      std::string name = get_str ();

      //  and the associated id
      unsigned long id = cellname_id;
      if (r == 3) {
        if (cellname_id_mode == expl) {
          error (tl::to_string (tr ("Explicit and implicit CELLNAME modes cannot be mixed")));
        }
        cellname_id_mode = impl;
        ++cellname_id;
      } else {
        if (cellname_id_mode == impl) {
          error (tl::to_string (tr ("Explicit and implicit CELLNAME modes cannot be mixed")));
        }
        cellname_id_mode = expl;
        get (id);
      }

      rename_cell (layout, id, name);

      reset_modal_variables ();

      std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), true);
      if (pp.first) {
        m_cellname_properties.insert (std::make_pair (id, pp.second));
      }

    } else if (r == 5 || r == 6 /*TEXTSTRING*/) {

      if (m_first_textstring == 0) {
        m_first_textstring = m_table_start;
      } else if (m_expect_strict_mode == 1 && m_in_table != InTEXTSTRING && m_first_textstring != 0) {
        warn (tl::to_string (tr ("TEXTSTRING outside table in strict mode")));
      }
      m_in_table = InTEXTSTRING;


      //  there cannot be more file level properties .. store what we have
      if (! layout_properties.empty ()) {
        layout.prop_id (layout.properties_repository ().properties_id (layout_properties));
        layout_properties.clear ();
      }

      //  read a text string
      std::string name = get_str ();

      //  and the associated id
      unsigned long id = textstring_id;
      if (r == 5) {
        if (textstring_id_mode == expl) {
          error (tl::to_string (tr ("Explicit and implicit TEXTSTRING modes cannot be mixed")));
        }
        textstring_id_mode = impl;
        ++textstring_id;
      } else {
        if (textstring_id_mode == impl) {
          error (tl::to_string (tr ("Explicit and implicit TEXTSTRING modes cannot be mixed")));
        }
        textstring_id_mode = expl;
        get (id);
      }

      if (! m_textstrings.insert (std::make_pair (id, name)).second) {
        error (tl::sprintf (tl::to_string (tr ("A TEXTSTRING with id %ld is present already")), id));
      }

      reset_modal_variables ();

      //  ignore properties attached to this name item
      read_element_properties (layout.properties_repository (), true);

    } else if (r == 7 || r == 8 /*PROPNAME*/) {

      if (m_first_propname == 0) {
        m_first_propname = m_table_start;
      } else if (m_expect_strict_mode == 1 && m_in_table != InPROPNAME && m_first_propname != 0) {
        warn (tl::to_string (tr ("PROPNAME outside table in strict mode")));
      }
      m_in_table = InPROPNAME;

      //  there cannot be more file level properties .. store what we have
      if (! layout_properties.empty ()) {
        layout.prop_id (layout.properties_repository ().properties_id (layout_properties));
        layout_properties.clear ();
      }

      //  read a property name
      std::string name = get_str ();

      //  and the associated id
      unsigned long id = propname_id;
      if (r == 7) {
        if (propname_id_mode == expl) {
          error (tl::to_string (tr ("Explicit and implicit PROPNAME modes cannot be mixed")));
        }
        propname_id_mode = impl;
        ++propname_id;
      } else {
        if (propname_id_mode == impl) {
          error (tl::to_string (tr ("Explicit and implicit PROPNAME modes cannot be mixed")));
        }
        propname_id_mode = expl;
        get (id);
      }

      if (! m_propnames.insert (std::make_pair (id, name)).second) {
        error (tl::sprintf (tl::to_string (tr ("A PROPNAME with id %ld is present already")), id));
      }

      //  resolve forward references to property names
      std::map <unsigned long, db::property_names_id_type>::iterator pf = m_propname_forward_references.find (id);
      if (pf != m_propname_forward_references.end ()) {

        bool is_s_gds_property = false;
        bool is_klayout_context_property = false;

        if (name == s_gds_property_propname) {
          is_s_gds_property = true;
        } else if (name == klayout_context_propname) {
          is_klayout_context_property = true;
        }

        //  handle special case of forward references to S_GDS_PROPERTY and KLAYOUT_CONTEXT
        if (is_s_gds_property || is_klayout_context_property) {

          db::PropertiesRepository &rep = layout.properties_repository ();

          //  exchange properties in layout_properties
          db::PropertiesRepository::properties_set new_set;

          for (db::PropertiesRepository::properties_set::const_iterator s = layout_properties.begin (); s != layout_properties.end (); ++s) {
            if (s->first == pf->second && is_s_gds_property) {

              //  S_GDS_PROPERTY translation
              if (!s->second.is_list () || s->second.get_list ().size () != 2) {
                error (tl::to_string (tr ("S_GDS_PROPERTY must have a value list with exactly two elements")));
              }

              new_set.insert (std::make_pair (rep.prop_name_id (s->second.get_list () [0]), s->second.get_list () [1]));

            } else if (s->first == pf->second && is_klayout_context_property) {

              //  feed context strings from klayout context property
              if (s->second.is_list ()) {
                for (auto l = s->second.begin (); l != s->second.end (); ++l) {
                  context_properties.push_back (*l);
                }
              } else {
                context_properties.push_back (s->second);
              }

            } else {
              new_set.insert (*s);
            }
          }

          new_set.swap (layout_properties);

          //  exchange the properties in the repository

          //  first locate all property sets that are affected
          std::map<db::properties_id_type, std::vector<db::cell_index_type> > cells_by_pid;
          for (auto p = rep.begin (); p != rep.end (); ++p) {
            if (p->second.find (pf->second) != p->second.end ()) {
              cells_by_pid.insert (std::make_pair (p->first, std::vector<db::cell_index_type> ()));
            }
          }

          //  find cells using a specific pid
          for (auto i = layout.begin (); i != layout.end (); ++i) {
            auto cc = cells_by_pid.find (i->prop_id ());
            if (cc != cells_by_pid.end ()) {
              cc->second.push_back (i->cell_index ());
            }
          }

          //  create new property sets for the ones we found
          for (auto pid = cells_by_pid.begin (); pid != cells_by_pid.end (); ++pid) {

            const db::PropertiesRepository::properties_set &old_set = rep.properties (pid->first);
            db::PropertiesRepository::properties_set new_set;

            for (auto s = old_set.begin (); s != old_set.end (); ++s) {
              if (s->first == pf->second && is_s_gds_property) {

                //  S_GDS_PROPERTY translation
                if (!s->second.is_list () || s->second.get_list ().size () != 2) {
                  error (tl::to_string (tr ("S_GDS_PROPERTY must have a value list with exactly two elements")));
                }

                new_set.insert (std::make_pair (rep.prop_name_id (s->second.get_list () [0]), s->second.get_list () [1]));

              } else if (s->first == pf->second && is_klayout_context_property) {

                auto pid2c = cells_by_pid.find (pid->first);

                if (pid->first == layout.prop_id ()) {
                  //  feed context strings from klayout context property
                  if (s->second.is_list ()) {
                    for (auto l = s->second.begin (); l != s->second.end (); ++l) {
                      context_properties.push_back (*l);
                    }
                  } else {
                    context_properties.push_back (s->second);
                  }
                }

                //  feed cell-specific context strings from klayout context property
                for (auto c = pid2c->second.begin (); c != pid2c->second.end (); ++c) {
                  std::vector<tl::Variant> &vl = m_context_strings_per_cell [*c];
                  if (s->second.is_list ()) {
                    for (auto l = s->second.begin (); l != s->second.end (); ++l) {
                      vl.push_back (*l);
                    }
                  } else {
                    vl.push_back (s->second);
                  }
                }

              } else {
                new_set.insert (*s);
              }
            }

            rep.change_properties (pid->first, new_set);

          }

        }

        layout.properties_repository ().change_name (pf->second, tl::Variant (name));
        m_propname_forward_references.erase (pf);

      }

      reset_modal_variables ();

      //  ignore properties attached to this name item
      read_element_properties (layout.properties_repository (), true);

    } else if (r == 9 || r == 10 /*PROPSTRING*/) {

      if (m_first_propstring == 0) {
        m_first_propstring = m_table_start;
      } else if (m_expect_strict_mode == 1 && m_in_table != InPROPSTRING && m_first_propstring != 0) {
        warn (tl::to_string (tr ("PROPSTRING outside table in strict mode")));
      }
      m_in_table = InPROPSTRING;

      //  there cannot be more file level properties .. store what we have
      if (! layout_properties.empty ()) {
        layout.prop_id (layout.properties_repository ().properties_id (layout_properties));
        layout_properties.clear ();
      }

      //  read a property string
      std::string name = get_str ();

      //  and the associated id
      unsigned long id = propstring_id;
      if (r == 9) {
        if (propstring_id_mode == expl) {
          error (tl::to_string (tr ("Explicit and implicit PROPSTRING modes cannot be mixed")));
        }
        propstring_id_mode = impl;
        ++propstring_id;
      } else {
        if (propstring_id_mode == impl) {
          error (tl::to_string (tr ("Explicit and implicit PROPSTRING modes cannot be mixed")));
        }
        propstring_id_mode = expl;
        get (id);
      }

      if (! m_propstrings.insert (std::make_pair (id, name)).second) {
        error (tl::sprintf (tl::to_string (tr ("A PROPSTRING with id %ld is present already")), id));
      }

      std::map<unsigned long, std::string>::iterator fw = m_propvalue_forward_references.find (id);
      if (fw != m_propvalue_forward_references.end ()) {
        fw->second = name;
      }

      reset_modal_variables ();

      //  ignore properties attached to this name item
      read_element_properties (layout.properties_repository (), true);

    } else if (r == 11 || r == 12 /*LAYERNAME*/) {

      if (m_first_layername == 0) {
        m_first_layername = m_table_start;
      } else if (m_expect_strict_mode == 1 && m_in_table != InLAYERNAME && m_first_layername != 0) {
        warn (tl::to_string (tr ("LAYERNAME outside table in strict mode")));
      }
      m_in_table = InLAYERNAME;

      //  there cannot be more file level properties .. store what we have
      if (! layout_properties.empty ()) {
        layout.prop_id (layout.properties_repository ().properties_id (layout_properties));
        layout_properties.clear ();
      }

      //  read a layer name
      std::string name = get_str ();

      db::ld_type dt1 = 0, dt2 = std::numeric_limits<db::ld_type>::max () - 1;
      db::ld_type l1 = 0, l2 = std::numeric_limits<db::ld_type>::max () - 1;
      unsigned int it;

      it = get_uint ();
      if (it == 0) {
        //  keep limits
      } else if (it == 1) {
        l2 = get_uint ();
      } else if (it == 2) {
        l1 = get_uint ();
      } else if (it == 3) {
        l1 = get_uint ();
        l2 = l1;
      } else if (it == 4) {
        l1 = get_uint ();
        l2 = get_uint ();
      } else {
        error (tl::to_string (tr ("Invalid LAYERNAME interval mode (layer)")));
      }

      it = get_uint ();
      if (it == 0) {
        //  keep limits
      } else if (it == 1) {
        dt2 = get_uint ();
      } else if (it == 2) {
        dt1 = get_uint ();
      } else if (it == 3) {
        dt1 = get_uint ();
        dt2 = dt1;
      } else if (it == 4) {
        dt1 = get_uint ();
        dt2 = get_uint ();
      } else {
        error (tl::to_string (tr ("Invalid LAYERNAME interval mode (datatype)")));
      }

      //  add to the layer name map
      tl::interval_map <db::ld_type, std::string> dt_map;
      LNameJoinOp1 op1;
      dt_map.add (dt1, dt2 + 1, name, op1);
      LNameJoinOp2 op2;
      layer_names ().add (l1, l2 + 1, dt_map, op2);

      reset_modal_variables ();

      //  ignore properties attached to this name item
      read_element_properties (layout.properties_repository (), true);

    } else if (r == 28 || r == 29 /*PROPERTY*/) {

      //  unrecognized property: store in layout properties
      if (r == 28) {
        read_properties (layout.properties_repository ());
      }

      if (! mm_last_property_is_sprop.get () && mm_last_property_name.get () == m_klayout_context_property_name_id) {
        context_properties.insert (context_properties.end (), mm_last_value_list.get ().begin (), mm_last_value_list.get ().end ());
      } else {
        //  store cell properties
        store_last_properties (layout.properties_repository (), layout_properties, true);
      }

      mark_start_table ();

    } else if (r == 30 || r == 31 /*XNAME*/) {

      //  there cannot be more file level properties .. store what we have
      if (! layout_properties.empty ()) {
        layout.prop_id (layout.properties_repository ().properties_id (layout_properties));
        layout_properties.clear ();
      }

      //  read a XNAME: it is simply ignored
      get_ulong ();
      get_str ();
      if (r == 31) {
        get_ulong ();
      }

      reset_modal_variables ();

      //  ignore properties attached to this name item
      read_element_properties (layout.properties_repository (), true);

    } else if (r == 13 || r == 14 /*CELL*/) {

      m_in_table = NotInTable;

      //  there cannot be more file level properties .. store what we have
      if (! layout_properties.empty ()) {
        layout.prop_id (layout.properties_repository ().properties_id (layout_properties));
        layout_properties.clear ();
      }

      db::cell_index_type cell_index = 0;

      //  read a cell
      if (r == 13) {

        unsigned long id = 0;
        get (id);

        std::pair<bool, db::cell_index_type> cc = cell_by_id (id);
        if (cc.first && ! layout.cell (cc.second).is_ghost_cell ()) {
          error (tl::sprintf (tl::to_string (tr ("A cell with id %ld is defined already")), id));
        }

        cell_index = make_cell (layout, id);

        m_cellname = name_for_id (id);
        if (m_cellname.empty ()) {
          m_cellname = std::string ("#") + tl::to_string (id);
        }

      } else {

        if (m_expect_strict_mode == 1) {
          warn (tl::to_string (tr ("CELL names must be references to CELLNAME ids in strict mode")));
        }

        std::string name = get_str ();

        std::pair<bool, db::cell_index_type> cc = cell_by_name (name);
        if (cc.first && ! layout.cell (cc.second).is_ghost_cell ()) {
          error (tl::sprintf (tl::to_string (tr ("A cell with name %s is defined already")), name.c_str ()));
        }

        cell_index = make_cell (layout, name);

        m_cellname = name;

      }

      reset_modal_variables ();
      mark_start_table ();

      do_read_cell (cell_index, layout);

    } else if (r == 34 /*CBLOCK*/) {

      unsigned int type = get_uint ();
      if (type != 0) {
        error (tl::sprintf (tl::to_string (tr ("Invalid CBLOCK compression type %d")), type));
      }

      size_t dummy = 0;
      get (dummy);  // uncomp-byte-count - not needed
      get (dummy);  // comp-byte-count - not needed

      //  put the stream into deflating mode
      m_stream.inflate ();

    } else {
      error (tl::sprintf (tl::to_string (tr ("Invalid record type on global level %d")), int (r)));
    }

  }

  if (! layout_properties.empty ()) {
    layout.prop_id (layout.properties_repository ().properties_id (layout_properties));
    layout_properties.clear ();
  }

  size_t pt = m_stream.pos ();

  if (table_offsets_at_end) {
    read_offset_table ();
  }

  //  read over tail and discard
  mb = (char *) m_stream.get (pt + 254 - m_stream.pos ());
  if (! mb) {
    error (tl::to_string (tr ("Format error (too few bytes after END record)")));
  }

  //  check if there are no more bytes
  mb = (char *) m_stream.get (254);
  if (mb) {
    error (tl::to_string (tr ("Format error (too many bytes after END record)")));
  }

  for (std::map <unsigned long, const db::StringRef *>::const_iterator fw = m_text_forward_references.begin (); fw != m_text_forward_references.end (); ++fw) {
    std::map <unsigned long, std::string>::const_iterator ts = m_textstrings.find (fw->first);
    if (ts == m_textstrings.end ()) {
      error (tl::sprintf (tl::to_string (tr ("No text string defined for text string id %ld")), fw->first));
    } else {
      layout.string_repository ().change_string_ref (fw->second, ts->second);
    }
  }

  //  all forward references to property names must be resolved
  for (std::map <unsigned long, db::property_names_id_type>::const_iterator fw = m_propname_forward_references.begin (); fw != m_propname_forward_references.end (); ++fw) {
    error (tl::sprintf (tl::to_string (tr ("No property name defined for property name id %ld")), fw->first));
  }

  //  resolve all propvalue forward referenced
  if (! m_propvalue_forward_references.empty ()) {

    for (auto i = context_properties.begin (); i != context_properties.end (); ++i) {
      replace_forward_references_in_variant (*i);
    }
    for (auto c = m_context_strings_per_cell.begin (); c != m_context_strings_per_cell.end (); ++c) {
      for (auto i = c->second.begin (); i != c->second.end (); ++i) {
        replace_forward_references_in_variant (*i);
      }
    }

    for (db::PropertiesRepository::non_const_iterator pi = layout.properties_repository ().begin_non_const (); pi != layout.properties_repository ().end_non_const (); ++pi) {
      for (db::PropertiesRepository::properties_set::iterator ps = pi->second.begin (); ps != pi->second.end (); ++ps) {
        replace_forward_references_in_variant (ps->second);
      }
    }

    m_propvalue_forward_references.clear ();

  }

  //  attach the properties found in CELLNAME to the cells (which may have other properties)
  for (std::map<unsigned long, db::properties_id_type>::const_iterator p = m_cellname_properties.begin (); p != m_cellname_properties.end (); ++p) {

    std::pair<bool, db::cell_index_type> c = cell_by_id (p->first);
    if (c.first) {

      db::PropertiesRepository::properties_set cnp = layout.properties_repository ().properties (p->second);

      //  Merge existing properties with the ones from CELLNAME
      db::Cell &cell = layout.cell (c.second);
      if (cell.prop_id () != 0) {
        db::PropertiesRepository::properties_set cp = layout.properties_repository ().properties (cell.prop_id ());
        cnp.insert (cp.begin (), cp.end ());
      }

      cell.prop_id (layout.properties_repository ().properties_id (cnp));

    }

  }

  //  Restore layout meta info
  if (! context_properties.empty ()) {
    LayoutOrCellContextInfo info = make_context_info (context_properties);
    layout.fill_meta_info_from_context (info);
  }

  //  Restore proxy cell (link to PCell or Library) and cell meta info
  if (! m_context_strings_per_cell.empty ()) {
    CommonReaderLayerMapping layer_mapping (this, &layout);
    for (auto cc = m_context_strings_per_cell.begin (); cc != m_context_strings_per_cell.end (); ++cc) {
      LayoutOrCellContextInfo info = make_context_info (cc->second);
      if (info.has_proxy_info ()) {
        layout.recover_proxy_as (cc->first, info, &layer_mapping);
      }
      layout.fill_meta_info_from_context (cc->first, info);
    }
  }

  //  Check the table offsets vs. real occurrence
  if (m_first_cellname != 0 && m_first_cellname != m_table_cellname && m_expect_strict_mode == 1) {
    warn (tl::sprintf (tl::to_string (tr ("CELLNAME table offset does not match first occurrence of CELLNAME in strict mode - %s vs. %s")), m_table_cellname, m_first_cellname));
  }
  if (m_first_propname != 0 && m_first_propname != m_table_propname && m_expect_strict_mode == 1) {
    warn (tl::sprintf (tl::to_string (tr ("PROPNAME table offset does not match first occurrence of PROPNAME in strict mode - %s vs. %s")), m_table_propname, m_first_propname));
  }
  if (m_first_propstring != 0 && m_first_propstring != m_table_propstring && m_expect_strict_mode == 1) {
    warn (tl::sprintf (tl::to_string (tr ("PROPSTRING table offset does not match first occurrence of PROPSTRING in strict mode - %s vs. %s")), m_table_propstring, m_first_propstring));
  }
  if (m_first_layername != 0 && m_first_layername != m_table_layername && m_expect_strict_mode == 1) {
    warn (tl::sprintf (tl::to_string (tr ("LAYERNAME table offset does not match first occurrence of LAYERNAME in strict mode - %s vs. %s")), m_table_layername, m_first_layername));
  }
  if (m_first_textstring != 0 && m_first_textstring != m_table_textstring && m_expect_strict_mode == 1) {
    warn (tl::sprintf (tl::to_string (tr ("TEXTSTRING table offset does not match first occurrence of TEXTSTRING in strict mode - %s vs. %s")), m_table_textstring, m_first_textstring));
  }
}

void
OASISReader::replace_forward_references_in_variant (tl::Variant &v)
{
  if (v.is_id ()) {

    unsigned long id = (unsigned long) v.to_id ();
    std::map <unsigned long, std::string>::const_iterator fw = m_propvalue_forward_references.find (id);
    if (fw != m_propvalue_forward_references.end ()) {
      v = tl::Variant (fw->second);
    } else {
      error (tl::sprintf (tl::to_string (tr ("No property value defined for property value id %ld")), id));
    }

  } else if (v.is_list ()) {

    //  Replace list elements as well
    //  TODO: Q: can there be a list of lists? would need recursive replacement -> make that a method of tl::Variant

    const std::vector<tl::Variant> &l = v.get_list ();
    bool needs_replacement = false;
    for (std::vector<tl::Variant>::const_iterator ll = l.begin (); ll != l.end () && ! needs_replacement; ++ll) {
      needs_replacement = ll->is_id ();
    }

    if (needs_replacement) {

      std::vector<tl::Variant> new_list (l);
      for (std::vector<tl::Variant>::iterator ll = new_list.begin (); ll != new_list.end (); ++ll) {
        if (ll->is_id ()) {
          unsigned long id = (unsigned long) ll->to_id ();
          std::map <unsigned long, std::string>::const_iterator fw = m_propvalue_forward_references.find (id);
          if (fw != m_propvalue_forward_references.end ()) {
            *ll = tl::Variant (fw->second);
          } else {
            error (tl::sprintf (tl::to_string (tr ("No property value defined for property value id %ld")), id));
          }
        }
      }

      v = tl::Variant (new_list.begin (), new_list.end ());

    }

  }
}

void
OASISReader::store_last_properties (db::PropertiesRepository &rep, db::PropertiesRepository::properties_set &properties, bool ignore_special)
{
  if (! m_read_properties) {

    //  All properties are ignored

  } else if (mm_last_property_is_sprop.get () && mm_last_property_name.get () == m_s_gds_property_name_id) {

    if (mm_last_value_list.get ().size () != 2) {
      error (tl::to_string (tr ("S_GDS_PROPERTY must have a value list with exactly two elements")));
    }

    properties.insert (std::make_pair (rep.prop_name_id (mm_last_value_list.get () [0]), mm_last_value_list.get () [1]));

  } else if (ignore_special && ! m_read_all_properties && mm_last_property_is_sprop.get ()) {

    //  Special properties are not turned into user properties except S_GDS_PROPERTY.
    //  This is mode is used for cells and layouts so the standard properties do not appear as user properties.
    //  For shapes we need to keep the special ones since they may be forward-references S_GDS_PROPERTY names.

  } else if (mm_last_value_list.get ().size () == 0) {
    properties.insert (std::make_pair (mm_last_property_name.get (), tl::Variant ()));
  } else if (mm_last_value_list.get ().size () == 1) {
    properties.insert (std::make_pair (mm_last_property_name.get (), tl::Variant (mm_last_value_list.get () [0])));
  } else if (mm_last_value_list.get ().size () > 1) {
    properties.insert (std::make_pair (mm_last_property_name.get (), tl::Variant (mm_last_value_list.get ().begin (), mm_last_value_list.get ().end ())));
  }
}

std::pair <bool, db::properties_id_type>
OASISReader::read_element_properties (db::PropertiesRepository &rep, bool ignore_special)
{
  db::PropertiesRepository::properties_set properties;

  mark_start_table ();

  while (true) {

    unsigned char m = get_byte ();

    if (m == 0 /*PAD*/) {

      //  skip PAD.
      mark_start_table ();

    } else if (m == 34 /*CBLOCK*/) {

      unsigned int type = get_uint ();
      if (type != 0) {
        error (tl::sprintf (tl::to_string (tr ("Invalid CBLOCK compression type %d")), type));
      }

      size_t dummy = 0;
      get (dummy);  // uncomp-byte-count - not needed
      get (dummy);  // comp-byte-count - not needed

      //  put the stream into deflating mode
      m_stream.inflate ();

    } else if (m == 28 /*PROPERTY*/) {

      read_properties (rep);
      store_last_properties (rep, properties, ignore_special);

      mark_start_table ();

    } else if (m == 29 /*PROPERTY*/) {

      store_last_properties (rep, properties, ignore_special);

      mark_start_table ();

    } else {

      m_stream.unget (1);
      break;

    }

  }

  if (! properties.empty ()) {
    return std::make_pair (true, rep.properties_id (properties));
  } else {
    return std::make_pair (false, 0);
  }
}

void
OASISReader::read_properties (db::PropertiesRepository &rep)
{
  unsigned char m = get_byte ();

  if (m & 0x04) {
    if (m & 0x02) {

      unsigned long id;
      get (id);

      std::map <unsigned long, std::string>::const_iterator cid = m_propnames.find (id);
      if (cid == m_propnames.end ()) {
        mm_last_property_name = rep.prop_name_id (tl::Variant (id, true /*dummy for id type*/));
        m_propname_forward_references.insert (std::make_pair (id, mm_last_property_name.get ()));
      } else {
        mm_last_property_name = rep.prop_name_id (tl::Variant (cid->second));
      }

    } else {

      if (m_expect_strict_mode == 1) {
        warn (tl::to_string (tr ("PROPERTY names must be references to PROPNAME ids in strict mode")));
      }

      mm_last_property_name = rep.prop_name_id (tl::Variant (get_str ()));

    }
  }

  mm_last_property_is_sprop = ((m & 0x01) != 0);

  if (! (m & 0x08)) {

    unsigned long n = ((unsigned long) (m >> 4)) & 0x0f;
    if (n == 15) {
      get (n);
    }

    mm_last_value_list.get_non_const ().clear ();
    mm_last_value_list.get_non_const ().reserve (n);

    while (n > 0) {

      unsigned char t = get_byte ();
      if (t < 8) {

        m_stream.unget (1);
        double v = get_real ();
        if (m_read_properties) {
          mm_last_value_list.get_non_const ().push_back (tl::Variant (v));
        }

      } else if (t == 8) {

        unsigned long l;
        get (l);
        if (m_read_properties) {
          mm_last_value_list.get_non_const ().push_back (tl::Variant (long (l)));
        }

      } else if (t == 9) {

        long l;
        get (l);
        if (m_read_properties) {
          mm_last_value_list.get_non_const ().push_back (tl::Variant (l));
        }

      } else if (t == 10 || t == 11 || t == 12) {

        if (m_expect_strict_mode == 1) {
          warn (tl::to_string (tr ("PROPERTY strings must be references to PROPSTRING ids in strict mode")));
        }

        if (m_read_properties) {
          mm_last_value_list.get_non_const ().push_back (tl::Variant (get_str ()));
        } else {
          get_str ();
        }

      } else if (t == 13 || t == 14 || t == 15) {

        unsigned long id;
        get (id);
        if (m_read_properties) {
          std::map <unsigned long, std::string>::const_iterator sid = m_propstrings.find (id);
          if (sid == m_propstrings.end ()) {
            m_propvalue_forward_references.insert (std::make_pair (id, std::string ()));
            mm_last_value_list.get_non_const ().push_back (tl::Variant (id, true /*dummy for id type*/));
          } else {
            mm_last_value_list.get_non_const ().push_back (tl::Variant (sid->second));
          }
        }

      } else {
        error (tl::sprintf (tl::to_string (tr ("Invalid property value type %d")), int (t)));
      }

      --n;

    }

    mm_last_value_list.set_initialized ();

  }
}



void
OASISReader::read_pointlist (modal_variable <std::vector <db::Point> > &pointlist, bool for_polygon)
{
  unsigned int type = get_uint ();

  unsigned long n = 0;
  get (n);
  if (n == 0) {
    error (tl::to_string (tr ("Invalid point list: length is zero")).c_str ());
  }

  pointlist.get_non_const ().clear ();
  if ((type == 0 || type == 1) && for_polygon) {
    //  because for polygons, the pointlist will be closed implicitly
    pointlist.get_non_const ().reserve (n + 2);
  } else {
    pointlist.get_non_const ().reserve (n + 1);
  }

  pointlist.get_non_const ().push_back (db::Point ());

  if (type == 0 || type == 1) {

    bool h = (type == 0);

    db::Point pos;
    for (unsigned long i = 0; i < n; ++i) {
      db::Coord d = get_coord ();
      if (h) {
        pos += db::Vector (d, 0);
      } else {
        pos += db::Vector (0, d);
      }
      h = ! h;
      pointlist.get_non_const ().push_back (pos);
    }

    //  synthesize the last point for polygons
    if (for_polygon) {
      if ((n % 2) != 0) {
        warn (tl::to_string (tr ("Type 0 or 1 point list with odd number of points is illegal")));
      }
      if (h) {
        pointlist.get_non_const ().push_back (db::Point (0, pos.y ()));
      } else {
        pointlist.get_non_const ().push_back (db::Point (pos.x (), 0));
      }
    }

  } else if (type == 2) {

    db::Point pos;
    for (unsigned long i = 0; i < n; ++i) {
      pos += get_2delta ();
      pointlist.get_non_const ().push_back (pos);
    }

  } else if (type == 3) {

    db::Point pos;
    for (unsigned long i = 0; i < n; ++i) {
      pos += get_3delta ();
      pointlist.get_non_const ().push_back (pos);
    }

  } else if (type == 4) {

    db::Point pos;
    for (unsigned long i = 0; i < n; ++i) {
      pos += get_gdelta ();
      pointlist.get_non_const ().push_back (pos);
    }

  } else if (type == 5) {

    db::Point pos;
    db::Vector delta;
    for (unsigned long i = 0; i < n; ++i) {
      delta += get_gdelta ();
      pos += delta;
      pointlist.get_non_const ().push_back (pos);
    }

  } else {
    error (tl::sprintf (tl::to_string (tr ("Invalid point list type %d")), type));
  }

  pointlist.set_initialized ();
}

bool
OASISReader::read_repetition ()
{
  unsigned int type = get_uint ();
  if (type == 0) {

    //  reuse modal variable

  } else if (type == 1) {

    unsigned long nx = 0, ny = 0;
    get (nx);
    get (ny);

    db::Coord dx = get_ucoord ();
    db::Coord dy = get_ucoord ();

    mm_repetition = new RegularRepetition (db::Vector (dx, 0), db::Vector (0, dy), dx == 0 ? 1 : nx + 2, dy == 0 ? 1 : ny + 2);

  } else if (type == 2) {

    unsigned long nx = 0;
    get (nx);

    db::Coord dx = get_ucoord ();

    mm_repetition = new RegularRepetition (db::Vector (dx, 0), db::Vector (0, 0), dx == 0 ? 1 : nx + 2, 1);

  } else if (type == 3) {

    unsigned long ny = 0;
    get (ny);

    db::Coord dy = get_ucoord ();

    mm_repetition = new RegularRepetition (db::Vector (0, 0), db::Vector (0, dy), 1, dy == 0 ? 1 : ny + 2);

  } else if (type == 4 || type == 5) {

    IrregularRepetition *rep = new IrregularRepetition ();
    mm_repetition = rep;

    unsigned long n = 0;
    get (n);

    unsigned long lgrid = 1;
    if (type == 5) {
      get (lgrid);
    }

    rep->reserve (n + 1);

    db::Coord x = 0;
    for (unsigned long i = 0; i <= n; ++i) {
      m_progress.set (m_stream.pos ());
      db::Coord d = get_ucoord (lgrid);
      if (d != 0) {
        x += d;
        rep->push_back (db::Vector (x, 0));
      }
    }

  } else if (type == 6 || type == 7) {

    IrregularRepetition *rep = new IrregularRepetition ();
    mm_repetition = rep;

    unsigned long n = 0;
    get (n);

    unsigned long lgrid = 1;
    if (type == 7) {
      get (lgrid);
    }

    rep->reserve (n + 1);

    db::Coord y = 0;
    for (unsigned long i = 0; i <= n; ++i) {
      m_progress.set (m_stream.pos ());
      db::Coord d = get_ucoord (lgrid);
      if (d != 0) {
        y += d;
        rep->push_back (db::Vector (0, y));
      }
    }

  } else if (type == 8) {

    unsigned long n = 0, m = 0;

    get (n);
    get (m);
    db::Vector dn = get_gdelta ();
    db::Vector dm = get_gdelta ();

    mm_repetition = new RegularRepetition (dn, dm, dn == db::Vector () ? 1 : n + 2, dm == db::Vector () ? 1 : m + 2);

  } else if (type == 9) {

    unsigned long n = 0;
    get (n);
    db::Vector dn = get_gdelta ();

    mm_repetition = new RegularRepetition (dn, db::Vector (0, 0), dn == db::Vector () ? 1 : n + 2, 1);

  } else if (type == 10 || type == 11) {

    IrregularRepetition *rep = new IrregularRepetition ();
    mm_repetition = rep;

    unsigned long n = 0;
    get (n);

    unsigned long grid = 1;
    if (type == 11) {
      get (grid);
    }

    rep->reserve (n + 1);

    db::Vector p;
    for (unsigned long i = 0; i <= n; ++i) {
      m_progress.set (m_stream.pos ());
      db::Vector d = get_gdelta (grid);
      if (d != db::Vector ()) {
        p += d;
        rep->push_back (p);
      }
    }

  } else {
    error (tl::sprintf (tl::to_string (tr ("Invalid repetition type %d")), type));
  }

  return mm_repetition.get ().size () > 1;
}

void
OASISReader::do_read_placement (unsigned char r,
                                bool xy_absolute,
                                db::Layout &layout,
                                tl::vector<db::CellInstArray> &instances,
                                tl::vector<db::CellInstArrayWithProperties> &instances_with_props)
{
  unsigned char m = get_byte ();

  //  locate cell
  if (m & 0x80) {

    if (m & 0x40) {

      //  cell by id
      unsigned long id;
      get (id);

      mm_placement_cell = cell_for_instance (layout, id);

    } else {

      //  cell by name
      std::string name;
      get_str (name);

      mm_placement_cell = cell_for_instance (layout, name);

    }

  }

  double mag = 1.0;
  bool mag_set = false;

  double angle_deg = 0.0; // only meaningful if angle < 0
  int angle = 0;
  bool mirror = false;

  if (r == 18) {

    if (m & 0x04) {
      mag = get_real ();
      if (fabs (mag - 1.0) > 1e-6) {
        mag_set = true;
      }
    }

    if (m & 0x02) {
      angle_deg = get_real ();
      double a = angle_deg / 90.0;
      if (a < -4 || a > 4) {
        warn (tl::sprintf (tl::to_string (tr ("Invalid rotation angle (%g is less than -360 or larger than 360)")), angle_deg));
      }
      angle = int (a < 0 ? (a - 0.5) : (a + 0.5));
      if (fabs (double (angle) - a) > 1e-6) {
        angle = -1; // indicates arbitrary orientation. Take angle_deg instead
      } else {
        if (angle < 0) {
          angle += ((4 - 1) - angle) & ~(4 - 1);
        }
        angle = angle % 4;
      }
    }

  } else {
    angle = ((m & 0x06) >> 1);
  }

  mirror = (m & 0x01) != 0;

  if (m & 0x20) {
    db::Coord x;
    get (x);
    if (xy_absolute) {
      mm_placement_x = x;
    } else {
      mm_placement_x = x + mm_placement_x.get ();
    }
  }

  if (m & 0x10) {
    db::Coord y;
    get (y);
    if (xy_absolute) {
      mm_placement_y = y;
    } else {
      mm_placement_y = y + mm_placement_y.get ();
    }
  }

  db::Vector pos (mm_placement_x.get (), mm_placement_y.get ());

  const std::vector<db::Vector> *points = 0;

  if ((m & 0x8) && read_repetition ()) {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    db::Vector a, b;
    size_t na, nb;
    if (mm_repetition.get ().is_regular (a, b, na, nb)) {

      db::CellInstArray inst;

      if (mag_set || angle < 0) {
        inst = db::CellInstArray (db::CellInst (mm_placement_cell.get ()),
                                  db::ICplxTrans (mag, angle_deg, mirror, pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb);
      } else {
        inst = db::CellInstArray (db::CellInst (mm_placement_cell.get ()),
                                  db::Trans (angle, mirror, pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb);
      }

      if (pp.first) {
        instances_with_props.push_back (db::CellInstArrayWithProperties (inst, pp.second));
      } else {
        instances.push_back (inst);
      }

    } else if (! layout.is_editable () && (points = mm_repetition.get ().is_iterated ()) != 0) {

      db::CellInstArray inst;

      if (mag_set || angle < 0) {

        db::ICplxTrans ct (mag, angle_deg, mirror, pos);

        db::CellInstArray::iterated_complex_array_type array (ct.rcos (), ct.mag ());
        array.reserve (points->size () + 1);
        array.insert (db::Vector ());
        array.insert (points->begin (), points->end ());
        array.sort ();

        inst = db::CellInstArray (db::CellInst (mm_placement_cell.get ()),
                                  db::Trans (ct), layout.array_repository ().insert (array));

      } else {

        db::CellInstArray::iterated_array_type array;
        array.reserve (points->size () + 1);
        array.insert (db::Vector ());
        array.insert (points->begin (), points->end ());
        array.sort ();

        inst = db::CellInstArray (db::CellInst (mm_placement_cell.get ()),
                                  db::Trans (angle, mirror, pos), layout.array_repository ().insert (array));

      }

      if (pp.first) {
        instances_with_props.push_back (db::CellInstArrayWithProperties (inst, pp.second));
      } else {
        instances.push_back (inst);
      }

    } else {

      RepetitionIterator p = mm_repetition.get ().begin ();
      while (! p.at_end ()) {

        db::CellInstArray inst;

        if (mag_set || angle < 0) {
          inst = db::CellInstArray (db::CellInst (mm_placement_cell.get ()), db::ICplxTrans (mag, angle_deg, mirror, pos + *p));
        } else {
          inst = db::CellInstArray (db::CellInst (mm_placement_cell.get ()), db::Trans (angle, mirror, pos + *p));
        }

        if (pp.first) {
          instances_with_props.push_back (db::CellInstArrayWithProperties (inst, pp.second));
        } else {
          instances.push_back (inst);
        }

        ++p;

      }

    }

  } else {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    db::CellInstArray inst;

    if (mag_set || angle < 0) {
      inst = db::CellInstArray (db::CellInst (mm_placement_cell.get ()), db::ICplxTrans (mag, angle_deg, mirror, pos));
    } else {
      inst = db::CellInstArray (db::CellInst (mm_placement_cell.get ()), db::Trans (angle, mirror, pos));
    }

    if (pp.first) {
      instances_with_props.push_back (db::CellInstArrayWithProperties (inst, pp.second));
    } else {
      instances.push_back (inst);
    }

  }
}

void
OASISReader::do_read_text (bool xy_absolute,
                           db::cell_index_type cell_index,
                           db::Layout &layout)
{
  unsigned char m = get_byte ();

  if (m & 0x40) {
    if (m & 0x20) {

      unsigned long id;
      get (id);

      if (m_text_forward_references.find (id) != m_text_forward_references.end ()) {

        mm_text_string.reset ();
        mm_text_string_id = id;

      } else {

        std::map <unsigned long, std::string>::const_iterator tid = m_textstrings.find (id);
        if (tid == m_textstrings.end ()) {

          mm_text_string.reset ();
          mm_text_string_id = id;

          const db::StringRef *string_ref = layout.string_repository ().create_string_ref ();
          m_text_forward_references.insert (std::make_pair (id, string_ref));

        } else {

          mm_text_string = tid->second;

        }

      }

    } else {

      if (m_expect_strict_mode == 1) {
        warn (tl::to_string (tr ("TEXT strings must be references to TEXTSTRING ids in strict mode")));
      }

      mm_text_string = get_str ();

    }
  }

  if (m & 0x1) {
    mm_textlayer = get_uint ();
  }

  if (m & 0x2) {
    mm_texttype = get_uint ();
  }

  if (m & 0x10) {
    db::Coord x;
    get (x);
    if (xy_absolute) {
      mm_text_x = x;
    } else {
      mm_text_x = x + mm_text_x.get ();
    }
  }

  if (m & 0x8) {
    db::Coord y;
    get (y);
    if (xy_absolute) {
      mm_text_y = y;
    } else {
      mm_text_y = y + mm_text_y.get ();
    }
  }

  db::Vector pos (mm_text_x.get (), mm_text_y.get ());

  std::pair<bool, unsigned int> ll (false, 0);
  if (m_read_texts) {
    ll = open_dl (layout, LDPair (mm_textlayer.get (), mm_texttype.get ()));
  }

  if ((m & 0x4) && read_repetition ()) {

    //  TODO: should not read properties if layer is not enabled!
    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      db::Text text;
      if (mm_text_string_id.is_set ()) {
        text = db::Text (m_text_forward_references.find (mm_text_string_id.get ())->second, db::Trans ());
      } else {
        text = db::Text (mm_text_string.get (), db::Trans ());
      }

      db::Cell &cell = layout.cell (cell_index);

      const std::vector<db::Vector> *points = 0;

      //  If the repetition is a regular one, convert the repetition into
      //  a shape array
      db::Vector a, b;
      size_t na, nb;
      if (! layout.is_editable () && mm_repetition.get ().is_regular (a, b, na, nb)) {

        db::TextPtr text_ptr (text, layout.shape_repository ());

        if (pp.first) {
          cell.shapes (ll.second).insert (db::object_with_properties<db::Shape::text_ptr_array_type> (db::Shape::text_ptr_array_type (text_ptr, db::Disp (pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb), pp.second));
        } else {
          cell.shapes (ll.second).insert (db::Shape::text_ptr_array_type (text_ptr, db::Disp (pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb));
        }

      } else if (! layout.is_editable () && (points = mm_repetition.get ().is_iterated ()) != 0) {

        db::TextPtr text_ptr (text, layout.shape_repository ());

        //  Create an iterated text array
        db::Shape::text_ptr_array_type::iterated_array_type array;
        array.reserve (points->size () + 1);
        array.insert (db::Vector ());
        array.insert (points->begin (), points->end ());
        array.sort ();

        if (pp.first) {
          cell.shapes (ll.second).insert (db::object_with_properties<db::Shape::text_ptr_array_type> (db::Shape::text_ptr_array_type (text_ptr, db::Disp (pos), layout.array_repository ().insert (array)), pp.second));
        } else {
          cell.shapes (ll.second).insert (db::Shape::text_ptr_array_type (text_ptr, db::Disp (pos), layout.array_repository ().insert (array)));
        }

      } else {

        RepetitionIterator p = mm_repetition.get ().begin ();
        db::TextRef text_ref (text, layout.shape_repository ());
        while (! p.at_end ()) {
          if (pp.first) {
            cell.shapes (ll.second).insert (db::TextRefWithProperties (text_ref.transformed (db::Disp (pos + *p)), pp.second));
          } else {
            cell.shapes (ll.second).insert (text_ref.transformed (db::Disp (pos + *p)));
          }
          ++p;
        }

      }

    }

  } else {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      db::Text text;
      if (mm_text_string_id.is_set ()) {
        text = db::Text (m_text_forward_references.find (mm_text_string_id.get ())->second, db::Trans (pos));
      } else {
        text = db::Text (mm_text_string.get (), db::Trans (pos));
      }

      if (pp.first) {
        layout.cell (cell_index).shapes (ll.second).insert (db::TextRefWithProperties (db::TextRef (text, layout.shape_repository ()), pp.second));
      } else {
        layout.cell (cell_index).shapes (ll.second).insert (db::TextRef (text, layout.shape_repository ()));
      }

    }

  }
}

void
OASISReader::do_read_rectangle (bool xy_absolute,
                                db::cell_index_type cell_index,
                                db::Layout &layout)
{
  unsigned char m = get_byte ();

  if (m & 0x1) {
    mm_layer = get_uint ();
  }

  if (m & 0x2) {
    mm_datatype = get_uint ();
  }

  if (m & 0x40) {
    mm_geometry_w = get_ucoord_as_distance ();
  }
  if (m & 0x80) {
    mm_geometry_h = mm_geometry_w; // TODO: really?
  } else {
    if (m & 0x20) {
      mm_geometry_h = get_ucoord_as_distance ();
    }
  }

  if (m & 0x10) {
    db::Coord x;
    get (x);
    if (xy_absolute) {
      mm_geometry_x = x;
    } else {
      mm_geometry_x = x + mm_geometry_x.get ();
    }
  }

  if (m & 0x8) {
    db::Coord y;
    get (y);
    if (xy_absolute) {
      mm_geometry_y = y;
    } else {
      mm_geometry_y = y + mm_geometry_y.get ();
    }
  }

  db::Box box (db::Point (mm_geometry_x.get (), mm_geometry_y.get ()),
               db::Point (mm_geometry_x.get () + mm_geometry_w.get (), mm_geometry_y.get () + mm_geometry_h.get ()));

  std::pair<bool, unsigned int> ll = open_dl (layout, LDPair (mm_layer.get (), mm_datatype.get ()));

  if ((m & 0x4) && read_repetition ()) {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      db::Cell &cell = layout.cell (cell_index);

      const std::vector<db::Vector> *points = 0;

      //  If the repetition is a regular one, convert the repetition into
      //  a box array
      db::Vector a, b;
      size_t na, nb;
      if (! layout.is_editable () && mm_repetition.get ().is_regular (a, b, na, nb)) {

        //  Create a box array
        if (pp.first) {
          cell.shapes (ll.second).insert (db::object_with_properties<db::Shape::box_array_type> (db::Shape::box_array_type (box, db::UnitTrans (), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb), pp.second));
        } else {
          cell.shapes (ll.second).insert (db::Shape::box_array_type (box, db::UnitTrans (), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb));
        }

      } else if (! layout.is_editable () && (points = mm_repetition.get ().is_iterated ()) != 0) {

        //  Create an iterated box array
        db::Shape::box_array_type::iterated_array_type array;
        array.reserve (points->size () + 1);
        array.insert (db::Vector ());
        array.insert (points->begin (), points->end ());
        array.sort ();

        if (pp.first) {
          cell.shapes (ll.second).insert (db::object_with_properties<db::Shape::box_array_type> (db::Shape::box_array_type (box, db::UnitTrans (), layout.array_repository ().insert (array)), pp.second));
        } else {
          cell.shapes (ll.second).insert (db::Shape::box_array_type (box, db::UnitTrans (), layout.array_repository ().insert (array)));
        }

      } else {

        //  convert the OASIS record into the rectangle one by one.
        RepetitionIterator p = mm_repetition.get ().begin ();
        while (! p.at_end ()) {
          if (pp.first) {
            cell.shapes (ll.second).insert (db::BoxWithProperties (box.moved (*p), pp.second));
          } else {
            cell.shapes (ll.second).insert (box.moved (*p));
          }
          ++p;
        }

      }

    }

  } else {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      db::Cell &cell = layout.cell (cell_index);
      if (pp.first) {
        cell.shapes (ll.second).insert (db::BoxWithProperties (box, pp.second));
      } else {
        cell.shapes (ll.second).insert (box);
      }

    }

  }
}

void
OASISReader::do_read_polygon (bool xy_absolute, db::cell_index_type cell_index, db::Layout &layout)
{
  unsigned char m = get_byte ();

  if (m & 0x1) {
    mm_layer = get_uint ();
  }

  if (m & 0x2) {
    mm_datatype = get_uint ();
  }

  if (m & 0x20) {
    read_pointlist (mm_polygon_point_list, true);
  }

  if (m & 0x10) {
    db::Coord x;
    get (x);
    if (xy_absolute) {
      mm_geometry_x = x;
    } else {
      mm_geometry_x = x + mm_geometry_x.get ();
    }
  }

  if (m & 0x8) {
    db::Coord y;
    get (y);
    if (xy_absolute) {
      mm_geometry_y = y;
    } else {
      mm_geometry_y = y + mm_geometry_y.get ();
    }
  }

  db::Vector pos (mm_geometry_x.get (), mm_geometry_y.get ());

  std::pair<bool, unsigned int> ll = open_dl (layout, LDPair (mm_layer.get (), mm_datatype.get ()));

  if ((m & 0x4) && read_repetition ()) {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      db::Cell &cell = layout.cell (cell_index);

      if (mm_polygon_point_list.get ().size () < 3) {
        warn (tl::to_string (tr ("POLYGON with less than 3 points ignored")));
      } else {

        //  convert the OASIS record into the polygon.
        db::SimplePolygon poly;
        poly.assign_hull (mm_polygon_point_list.get ().begin (), mm_polygon_point_list.get ().end (), false /*no compression*/);

        const std::vector<db::Vector> *points = 0;

        //  If the repetition is a regular one, convert the repetition into
        //  a shape array
        db::Vector a, b;
        size_t na, nb;
        if (! layout.is_editable () && mm_repetition.get ().is_regular (a, b, na, nb)) {

          //  creating a SimplePolygonPtr is most efficient with a normalized polygon because no displacement is provided
          db::Vector d (poly.box ().lower_left () - db::Point ());
          poly.move (-d);
          db::SimplePolygonPtr poly_ptr (poly, layout.shape_repository ());

          if (pp.first) {
            cell.shapes (ll.second).insert (db::object_with_properties<db::array<db::SimplePolygonPtr, db::Disp> > (db::array<db::SimplePolygonPtr, db::Disp> (poly_ptr, db::Disp (d + pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb), pp.second));
          } else {
            cell.shapes (ll.second).insert (db::array<db::SimplePolygonPtr, db::Disp> (poly_ptr, db::Disp (d + pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb));
          }

        } else if (! layout.is_editable () && (points = mm_repetition.get ().is_iterated ()) != 0) {

          db::Vector d (poly.box ().lower_left () - db::Point ());
          poly.move (-d);
          db::SimplePolygonPtr poly_ptr (poly, layout.shape_repository ());

          //  Create an iterated simple polygon array
          db::Shape::simple_polygon_ptr_array_type::iterated_array_type array;
          array.reserve (points->size () + 1);
          array.insert (db::Vector ());
          array.insert (points->begin (), points->end ());
          array.sort ();

          if (pp.first) {
            cell.shapes (ll.second).insert (db::object_with_properties<db::Shape::simple_polygon_ptr_array_type> (db::Shape::simple_polygon_ptr_array_type (poly_ptr, db::Disp (d + pos), layout.array_repository ().insert (array)), pp.second));
          } else {
            cell.shapes (ll.second).insert (db::Shape::simple_polygon_ptr_array_type (poly_ptr, db::Disp (d + pos), layout.array_repository ().insert (array)));
          }

        } else {

          db::SimplePolygonRef poly_ref (poly, layout.shape_repository ());

          RepetitionIterator p = mm_repetition.get ().begin ();
          while (! p.at_end ()) {
            if (pp.first) {
              cell.shapes (ll.second).insert (db::SimplePolygonRefWithProperties (poly_ref.transformed (db::Disp (pos + *p)), pp.second));
            } else {
              cell.shapes (ll.second).insert (poly_ref.transformed (db::Disp (pos + *p)));
            }
            ++p;
          }

        }

      }

    }

  } else {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      if (mm_polygon_point_list.get ().size () < 3) {
        warn (tl::to_string (tr ("POLYGON with less than 3 points ignored")));
      } else {

        //  convert the OASIS record into the polygon.
        db::SimplePolygon poly;
        poly.assign_hull (mm_polygon_point_list.get ().begin (), mm_polygon_point_list.get ().end (), false /*no compression*/);
        db::SimplePolygonRef poly_ref (poly, layout.shape_repository ());

        if (pp.first) {
          layout.cell (cell_index).shapes (ll.second).insert (db::SimplePolygonRefWithProperties (poly_ref.transformed (db::Disp (pos)), pp.second));
        } else {
          layout.cell (cell_index).shapes (ll.second).insert (poly_ref.transformed (db::Disp (pos)));
        }

      }

    }

  }
}

void
OASISReader::do_read_path (bool xy_absolute, db::cell_index_type cell_index, db::Layout &layout)
{
  unsigned char m = get_byte ();

  if (m & 0x1) {
    mm_layer = get_uint ();
  }

  if (m & 0x2) {
    mm_datatype = get_uint ();
  }

  if (m & 0x40) {
    mm_path_halfwidth = get_ucoord_as_distance ();
  }

  if (m & 0x80) {

    unsigned int e = get_uint ();
    if ((e & 0x0c) == 0x0c) {
      mm_path_start_extension = get_coord ();
    } else if ((e & 0x0c) == 0x04) {
      mm_path_start_extension = 0; //  TODO: is setting the start extension modal variable correct here?
    } else if ((e & 0x0c) == 0x08) {
      mm_path_start_extension = mm_path_halfwidth.get (); //  TODO: is setting the start extension modal variable correct here?
    }
    if ((e & 0x03) == 0x03) {
      mm_path_end_extension = get_coord ();
    } else if ((e & 0x03) == 0x01) {
      mm_path_end_extension = 0; //  TODO: is setting the start extension modal variable correct here?
    } else if ((e & 0x03) == 0x02) {
      mm_path_end_extension = mm_path_halfwidth.get (); //  TODO: is setting the start extension modal variable correct here?
    }

  }

  if (m & 0x20) {
    read_pointlist (mm_path_point_list, false);
  }

  if (m & 0x10) {
    db::Coord x;
    get (x);
    if (xy_absolute) {
      mm_geometry_x = x;
    } else {
      mm_geometry_x = x + mm_geometry_x.get ();
    }
  }

  if (m & 0x8) {
    db::Coord y;
    get (y);
    if (xy_absolute) {
      mm_geometry_y = y;
    } else {
      mm_geometry_y = y + mm_geometry_y.get ();
    }
  }

  db::Vector pos (mm_geometry_x.get (), mm_geometry_y.get ());

  std::pair<bool, unsigned int> ll = open_dl (layout, LDPair (mm_layer.get (), mm_datatype.get ()));

  if ((m & 0x4) && read_repetition ()) {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      if (mm_path_point_list.get ().size () < 2) {
        warn (tl::to_string (tr ("POLYGON with less than 2 points ignored")));
      } else {

        //  convert the OASIS record into the path.
        db::Path path;
        path.width (2 * mm_path_halfwidth.get ());
        path.extensions (mm_path_start_extension.get (), mm_path_end_extension.get ());
        path.assign (mm_path_point_list.get ().begin (), mm_path_point_list.get ().end ());

        db::Cell &cell = layout.cell (cell_index);

        const std::vector<db::Vector> *points = 0;

        //  If the repetition is a regular one, convert the repetition into
        //  a shape array
        db::Vector a, b;
        size_t na, nb;
        if (! layout.is_editable () && mm_repetition.get ().is_regular (a, b, na, nb)) {

          //  creating a PathPtr is most efficient with a normalized path because no displacement is provided
          db::Vector d (*path.begin ());
          path.move (-d);
          db::PathPtr path_ptr (path, layout.shape_repository ());

          if (pp.first) {
            cell.shapes (ll.second).insert (db::object_with_properties<db::array<db::PathPtr, db::Disp> > (db::array<db::PathPtr, db::Disp> (path_ptr, db::Disp (d + pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb), pp.second));
          } else {
            cell.shapes (ll.second).insert (db::array<db::PathPtr, db::Disp> (path_ptr, db::Disp (d + pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb));
          }

        } else if (! layout.is_editable () && (points = mm_repetition.get ().is_iterated ()) != 0) {

          db::Vector d (*path.begin () - db::Point ());
          path.move (-d);
          db::PathPtr path_ptr (path, layout.shape_repository ());

          //  Create an iterated simple polygon array
          db::Shape::path_ptr_array_type::iterated_array_type array;
          array.reserve (points->size () + 1);
          array.insert (db::Vector ());
          array.insert (points->begin (), points->end ());
          array.sort ();

          if (pp.first) {
            cell.shapes (ll.second).insert (db::object_with_properties<db::Shape::path_ptr_array_type> (db::Shape::path_ptr_array_type (path_ptr, db::Disp (d + pos), layout.array_repository ().insert (array)), pp.second));
          } else {
            cell.shapes (ll.second).insert (db::Shape::path_ptr_array_type (path_ptr, db::Disp (d + pos), layout.array_repository ().insert (array)));
          }

        } else {

          db::PathRef path_ref (path, layout.shape_repository ());

          RepetitionIterator p = mm_repetition.get ().begin ();
          while (! p.at_end ()) {
            if (pp.first) {
              cell.shapes (ll.second).insert (db::PathRefWithProperties (path_ref.transformed (db::Disp (pos + *p)), pp.second));
            } else {
              cell.shapes (ll.second).insert (path_ref.transformed (db::Disp (pos + *p)));
            }
            ++p;
          }

        }

      }

    }

  } else {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      if (mm_path_point_list.get ().size () < 2) {
        warn (tl::to_string (tr ("PATH with less than 2 points ignored")));
      } else {

        //  convert the OASIS record into the path.
        db::Path path;
        path.width (2 * mm_path_halfwidth.get ());
        path.extensions (mm_path_start_extension.get (), mm_path_end_extension.get ());
        path.assign (mm_path_point_list.get ().begin (), mm_path_point_list.get ().end ());
        db::PathRef path_ref (path, layout.shape_repository ());

        if (pp.first) {
          layout.cell (cell_index).shapes (ll.second).insert (db::PathRefWithProperties (path_ref.transformed (db::Disp (pos)), pp.second));
        } else {
          layout.cell (cell_index).shapes (ll.second).insert (path_ref.transformed (db::Disp (pos)));
        }

      }

    }

  }
}

void
OASISReader::do_read_trapezoid (unsigned char r, bool xy_absolute,db::cell_index_type cell_index, db::Layout &layout)
{
  unsigned char m = get_byte ();

  if (m & 0x1) {
    mm_layer = get_uint ();
  }

  if (m & 0x2) {
    mm_datatype = get_uint ();
  }

  if (m & 0x40) {
    mm_geometry_w = get_ucoord_as_distance ();
  }

  if (m & 0x20) {
    mm_geometry_h = get_ucoord_as_distance ();
  }

  db::Coord delta_a = 0, delta_b = 0;
  if (r == 23 || r == 24) {
    delta_a = get_coord ();
  }
  if (r == 23 || r == 25) {
    delta_b = get_coord ();
  }

  if (m & 0x10) {
    db::Coord x;
    get (x);
    if (xy_absolute) {
      mm_geometry_x = x;
    } else {
      mm_geometry_x = x + mm_geometry_x.get ();
    }
  }

  if (m & 0x8) {
    db::Coord y;
    get (y);
    if (xy_absolute) {
      mm_geometry_y = y;
    } else {
      mm_geometry_y = y + mm_geometry_y.get ();
    }
  }

  db::Vector pos (mm_geometry_x.get (), mm_geometry_y.get ());

  std::pair<bool, unsigned int> ll = open_dl (layout, LDPair (mm_layer.get (), mm_datatype.get ()));

  db::Point pts [4];

  if (m & 0x80) {
    //  vertically
    pts [0] = db::Point (0, std::max (delta_a, db::Coord (0)));
    pts [1] = db::Point (0, mm_geometry_h.get () + std::min (delta_b, db::Coord (0)));
    pts [2] = db::Point (mm_geometry_w.get (), mm_geometry_h.get () - std::max (delta_b, db::Coord (0)));
    pts [3] = db::Point (mm_geometry_w.get (), -std::min (delta_a, db::Coord (0)));
  } else {
    //  horizontally
    pts [0] = db::Point (std::max (delta_a, db::Coord (0)), mm_geometry_h.get ());
    pts [1] = db::Point (mm_geometry_w.get () + std::min (delta_b, db::Coord (0)), mm_geometry_h.get ());
    pts [2] = db::Point (mm_geometry_w.get () - std::max (delta_b, db::Coord (0)), db::Coord (0));
    pts [3] = db::Point (-std::min (delta_a, db::Coord (0)), db::Coord (0));
  }

  if ((m & 0x4) && read_repetition ()) {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      //  convert the OASIS record into the polygon.
      db::SimplePolygon poly;
      poly.assign_hull (pts, pts + 4, false /*no compression*/);

      db::Cell &cell = layout.cell (cell_index);

      const std::vector<db::Vector> *points = 0;

      //  If the repetition is a regular one, convert the repetition into
      //  a shape array
      db::Vector a, b;
      size_t na, nb;
      if (! layout.is_editable () && mm_repetition.get ().is_regular (a, b, na, nb)) {

        //  creating a SimplePolygonPtr is most efficient with a normalized polygon because no displacement is provided
        db::Vector d (poly.box ().lower_left ());
        poly.move (-d);
        db::SimplePolygonPtr poly_ptr (poly, layout.shape_repository ());

        if (pp.first) {
          cell.shapes (ll.second).insert (db::object_with_properties<db::array<db::SimplePolygonPtr, db::Disp> > (db::array<db::SimplePolygonPtr, db::Disp> (poly_ptr, db::Disp (d + pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb), pp.second));
        } else {
          cell.shapes (ll.second).insert (db::array<db::SimplePolygonPtr, db::Disp> (poly_ptr, db::Disp (d + pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb));
        }

      } else if (! layout.is_editable () && (points = mm_repetition.get ().is_iterated ()) != 0) {

        db::Vector d (poly.box ().lower_left () - db::Point ());
        poly.move (-d);
        db::SimplePolygonPtr poly_ptr (poly, layout.shape_repository ());

        //  Create an iterated simple polygon array
        db::Shape::simple_polygon_ptr_array_type::iterated_array_type array;
        array.reserve (points->size () + 1);
        array.insert (db::Vector ());
        array.insert (points->begin (), points->end ());
        array.sort ();

        if (pp.first) {
          cell.shapes (ll.second).insert (db::object_with_properties<db::Shape::simple_polygon_ptr_array_type> (db::Shape::simple_polygon_ptr_array_type (poly_ptr, db::Disp (d + pos), layout.array_repository ().insert (array)), pp.second));
        } else {
          cell.shapes (ll.second).insert (db::Shape::simple_polygon_ptr_array_type (poly_ptr, db::Disp (d + pos), layout.array_repository ().insert (array)));
        }

      } else {

        db::SimplePolygonRef poly_ref (poly, layout.shape_repository ());

        RepetitionIterator p = mm_repetition.get ().begin ();
        while (! p.at_end ()) {
          if (pp.first) {
            cell.shapes (ll.second).insert (db::SimplePolygonRefWithProperties (poly_ref.transformed (db::Disp (pos + *p)), pp.second));
          } else {
            cell.shapes (ll.second).insert (poly_ref.transformed (db::Disp (pos + *p)));
          }
          ++p;
        }

      }

    }

  } else {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      //  convert the OASIS record into the polygon.
      db::SimplePolygon poly;
      poly.assign_hull (pts, pts + 4, false /*no compression*/);
      db::SimplePolygonRef poly_ref (poly, layout.shape_repository ());

      if (pp.first) {
        layout.cell (cell_index).shapes (ll.second).insert (SimplePolygonRefWithProperties (poly_ref.transformed (db::Disp (pos)), pp.second));
      } else {
        layout.cell (cell_index).shapes (ll.second).insert (poly_ref.transformed (db::Disp (pos)));
      }

    }

  }
}

void
OASISReader::do_read_ctrapezoid (bool xy_absolute,db::cell_index_type cell_index, db::Layout &layout)
{
  unsigned char m = get_byte ();

  if (m & 0x1) {
    mm_layer = get_uint ();
  }

  if (m & 0x2) {
    mm_datatype = get_uint ();
  }

  if (m & 0x80) {
    mm_ctrapezoid_type = get_uint ();
  }

  if (m & 0x40) {
    mm_geometry_w = get_ucoord_as_distance ();
  }

  if (m & 0x20) {
    mm_geometry_h = get_ucoord_as_distance ();
  }

  if (m & 0x10) {
    db::Coord x;
    get (x);
    if (xy_absolute) {
      mm_geometry_x = x;
    } else {
      mm_geometry_x = x + mm_geometry_x.get ();
    }
  }

  if (m & 0x8) {
    db::Coord y;
    get (y);
    if (xy_absolute) {
      mm_geometry_y = y;
    } else {
      mm_geometry_y = y + mm_geometry_y.get ();
    }
  }

  db::Vector pos (mm_geometry_x.get (), mm_geometry_y.get ());

  std::pair<bool, unsigned int> ll = open_dl (layout, LDPair (mm_layer.get (), mm_datatype.get ()));

  db::Point pts [4];

  static db::Coord ctraps_table[][4][4] = {
    //  type 0
    {
      { 0, 0, 0, 0 },  // x=0*w+0*h, y=0*w+0*h ...
      { 0, 0, 0, 1 },
      { 1, -1, 0, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 1
    {
      { 0, 0, 0, 0 },
      { 0, 0, 0, 1 },
      { 1, 0, 0, 1 },
      { 1, -1, 0, 0 }
    },
    //  type 2
    {
      { 0, 0, 0, 0 },
      { 0, 1, 0, 1 },
      { 1, 0, 0, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 3
    {
      { 0, 1, 0, 0 },
      { 0, 0, 0, 1 },
      { 1, 0, 0, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 4
    {
      { 0, 0, 0, 0 },
      { 0, 1, 0, 1 },
      { 1, -1, 0, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 5
    {
      { 0, 1, 0, 0 },
      { 0, 0, 0, 1 },
      { 1, 0, 0, 1 },
      { 1, -1, 0, 0 }
    },
    //  type 6
    {
      { 0, 0, 0, 0 },
      { 0, 1, 0, 1 },
      { 1, 0, 0, 1 },
      { 1, -1, 0, 0 }
    },
    //  type 7
    {
      { 0, 1, 0, 0 },
      { 0, 0, 0, 1 },
      { 1, -1, 0, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 8
    {
      { 0, 0, 0, 0 },
      { 0, 0, 0, 1 },
      { 1, 0, -1, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 9
    {
      { 0, 0, 0, 0 },
      { 0, 0, -1, 1 },
      { 1, 0, 0, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 10
    {
      { 0, 0, 0, 0 },
      { 0, 0, 0, 1 },
      { 1, 0, 0, 1 },
      { 1, 0, 1, 0 }
    },
    //  type 11
    {
      { 0, 0, 1, 0 },
      { 0, 0, 0, 1 },
      { 1, 0, 0, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 12
    {
      { 0, 0, 0, 0 },
      { 0, 0, 0, 1 },
      { 1, 0, -1, 1 },
      { 1, 0, 1, 0 }
    },
    //  type 13
    {
      { 0, 0, 1, 0 },
      { 0, 0, -1, 1 },
      { 1, 0, 0, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 14
    {
      { 0, 0, 0, 0 },
      { 0, 0, -1, 1 },
      { 1, 0, 0, 1 },
      { 1, 0, 1, 0 }
    },
    //  type 15
    {
      { 0, 0, 1, 0 },
      { 0, 0, 0, 1 },
      { 1, 0, -1, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 16
    {
      { 0, 0, 0, 0 },
      { 0, 0, 1, 0 },
      { 1, 0, 0, 0 },
      { 0, 0, 0, 0 }
    },
    //  type 17
    {
      { 0, 0, 0, 0 },
      { 0, 0, 1, 0 },
      { 1, 0, 1, 0 },
      { 0, 0, 0, 0 }
    },
    //  type 18
    {
      { 0, 0, 0, 0 },
      { 1, 0, 1, 0 },
      { 1, 0, 0, 0 },
      { 0, 0, 0, 0 }
    },
    //  type 19
    {
      { 0, 0, 1, 0 },
      { 1, 0, 1, 0 },
      { 1, 0, 0, 0 },
      { 0, 0, 1, 0 }
    },
    //  type 20
    {
      { 0, 0, 0, 0 },
      { 0, 1, 0, 1 },
      { 0, 2, 0, 0 },
      { 0, 0, 0, 0 }
    },
    //  type 21
    {
      { 0, 0, 0, 1 },
      { 0, 2, 0, 1 },
      { 0, 1, 0, 0 },
      { 0, 0, 0, 1 }
    },
    //  type 22
    {
      { 0, 0, 0, 0 },
      { 0, 0, 2, 0 },
      { 1, 0, 1, 0 },
      { 0, 0, 0, 0 }
    },
    //  type 23
    {
      { 1, 0, 0, 0 },
      { 0, 0, 1, 0 },
      { 1, 0, 2, 0 },
      { 1, 0, 0, 0 }
    },
    //  type 24
    {
      { 0, 0, 0, 0 },
      { 0, 0, 0, 1 },
      { 1, 0, 0, 1 },
      { 1, 0, 0, 0 }
    },
    //  type 25
    {
      { 0, 0, 0, 0 },
      { 0, 0, 1, 0 },
      { 1, 0, 1, 0 },
      { 1, 0, 0, 0 }
    },
  };

  if (mm_ctrapezoid_type.get () >= sizeof (ctraps_table) / sizeof (ctraps_table [0])) {
    error (tl::sprintf (tl::to_string (tr ("Invalid CTRAPEZOID type %d")), int (mm_ctrapezoid_type.get ())));
  }

  db::Coord w = 0, h = 0;

  for (unsigned i = 0; i < 4; ++i) {

    db::Coord *m = ctraps_table [mm_ctrapezoid_type.get ()][i];

    db::Coord x = 0;
    if (m[0] != 0) x += m[0] * mm_geometry_w.get ();
    if (m[1] != 0) x += m[1] * mm_geometry_h.get ();

    db::Coord y = 0;
    if (m[2] != 0) y += m[2] * mm_geometry_w.get ();
    if (m[3] != 0) y += m[3] * mm_geometry_h.get ();

    pts [i] = db::Point (x, y);

    if (x > w) w = x;
    if (y > h) h = y;

  }

  //  set modal variables to the bbox of the shape
  mm_geometry_w = w;
  mm_geometry_h = h;

  unsigned int npts = 4;
  if (pts [npts - 1] == pts [0]) {
    --npts;
  }

  if ((m & 0x4) && read_repetition ()) {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      //  convert the OASIS record into the polygon.
      db::SimplePolygon poly;
      poly.assign_hull (pts, pts + npts, false /*no compression*/);

      db::Cell &cell = layout.cell (cell_index);

      const std::vector<db::Vector> *points = 0;

      //  If the repetition is a regular one, convert the repetition into
      //  a shape array
      db::Vector a, b;
      size_t na, nb;
      if (! layout.is_editable () && mm_repetition.get ().is_regular (a, b, na, nb)) {

        db::Vector d (poly.box ().lower_left () - db::Point ());
        poly.move (-d);
        db::SimplePolygonPtr poly_ptr (poly, layout.shape_repository ());

        if (pp.first) {
          cell.shapes (ll.second).insert (db::object_with_properties<db::array<db::SimplePolygonPtr, db::Disp> > (db::array<db::SimplePolygonPtr, db::Disp> (poly_ptr, db::Disp (d + pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb), pp.second));
        } else {
          cell.shapes (ll.second).insert (db::array<db::SimplePolygonPtr, db::Disp> (poly_ptr, db::Disp (d + pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb));
        }

      } else if (! layout.is_editable () && (points = mm_repetition.get ().is_iterated ()) != 0) {

        db::Vector d (poly.box ().lower_left () - db::Point ());
        poly.move (-d);
        db::SimplePolygonPtr poly_ptr (poly, layout.shape_repository ());

        //  Create an iterated simple polygon array
        db::Shape::simple_polygon_ptr_array_type::iterated_array_type array;
        array.reserve (points->size () + 1);
        array.insert (db::Vector ());
        array.insert (points->begin (), points->end ());
        array.sort ();

        if (pp.first) {
          cell.shapes (ll.second).insert (db::object_with_properties<db::Shape::simple_polygon_ptr_array_type> (db::Shape::simple_polygon_ptr_array_type (poly_ptr, db::Disp (d + pos), layout.array_repository ().insert (array)), pp.second));
        } else {
          cell.shapes (ll.second).insert (db::Shape::simple_polygon_ptr_array_type (poly_ptr, db::Disp (d + pos), layout.array_repository ().insert (array)));
        }

      } else {

        db::SimplePolygonRef poly_ref (poly, layout.shape_repository ());

        RepetitionIterator p = mm_repetition.get ().begin ();
        while (! p.at_end ()) {
          if (pp.first) {
            cell.shapes (ll.second).insert (db::SimplePolygonRefWithProperties (poly_ref.transformed (db::Disp (pos + *p)), pp.second));
          } else {
            cell.shapes (ll.second).insert (poly_ref.transformed (db::Disp (pos + *p)));
          }
          ++p;
        }

      }

    }

  } else {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      //  convert the OASIS record into the polygon.
      db::SimplePolygon poly;
      poly.assign_hull (pts, pts + npts, false /*no compression*/);
      db::SimplePolygonRef poly_ref (poly, layout.shape_repository ());

      if (pp.first) {
        layout.cell (cell_index).shapes (ll.second).insert (db::SimplePolygonRefWithProperties (poly_ref.transformed (db::Disp (pos)), pp.second));
      } else {
        layout.cell (cell_index).shapes (ll.second).insert (poly_ref.transformed (db::Disp (pos)));
      }

    }

  }
}

void
OASISReader::do_read_circle (bool xy_absolute, db::cell_index_type cell_index, db::Layout &layout)
{
  unsigned char m = get_byte ();

  if (m & 0x1) {
    mm_layer = get_uint ();
  }

  if (m & 0x2) {
    mm_datatype = get_uint ();
  }

  if (m & 0x20) {
    mm_circle_radius = get_ucoord_as_distance ();
  }

  if (m & 0x10) {
    db::Coord x;
    get (x);
    if (xy_absolute) {
      mm_geometry_x = x;
    } else {
      mm_geometry_x = x + mm_geometry_x.get ();
    }
  }

  if (m & 0x8) {
    db::Coord y;
    get (y);
    if (xy_absolute) {
      mm_geometry_y = y;
    } else {
      mm_geometry_y = y + mm_geometry_y.get ();
    }
  }

  db::Vector pos (mm_geometry_x.get (), mm_geometry_y.get ());

  std::pair<bool, unsigned int> ll = open_dl (layout, LDPair (mm_layer.get (), mm_datatype.get ()));

  //  ignore this circle if the radius is zero
  if (mm_circle_radius.get () <= 0) {
    ll.first = false;
  }

  if ((m & 0x4) && read_repetition ()) {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      //  convert the OASIS circle into a single-point path.
      db::Path path;
      path.width (2 * mm_circle_radius.get ());
      path.extensions (mm_circle_radius.get (), mm_circle_radius.get ());
      path.round (true);
      db::Point p0 (0, 0);
      path.assign (&p0, &p0 + 1);

      db::Cell &cell = layout.cell (cell_index);

      const std::vector<db::Vector> *points = 0;

      //  If the repetition is a regular one, convert the repetition into
      //  a shape array
      db::Vector a, b;
      size_t na, nb;
      if (! layout.is_editable () && mm_repetition.get ().is_regular (a, b, na, nb)) {

        //  creating a PathPtr is most efficient with a normalized path because no displacement is provided
        db::PathPtr path_ptr (path, layout.shape_repository ());

        if (pp.first) {
          cell.shapes (ll.second).insert (db::object_with_properties<db::array<db::PathPtr, db::Disp> > (db::array<db::PathPtr, db::Disp> (path_ptr, db::Disp (pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb), pp.second));
        } else {
          cell.shapes (ll.second).insert (db::array<db::PathPtr, db::Disp> (path_ptr, db::Disp (pos), layout.array_repository (), a, b, (unsigned long) na, (unsigned long) nb));
        }

      } else if (! layout.is_editable () && (points = mm_repetition.get ().is_iterated ()) != 0) {

        db::PathPtr path_ptr (path, layout.shape_repository ());

        //  Create an iterated simple polygon array
        db::Shape::path_ptr_array_type::iterated_array_type array;
        array.reserve (points->size () + 1);
        array.insert (db::Vector ());
        array.insert (points->begin (), points->end ());
        array.sort ();

        if (pp.first) {
          cell.shapes (ll.second).insert (db::object_with_properties<db::Shape::path_ptr_array_type> (db::Shape::path_ptr_array_type (path_ptr, db::Disp (pos), layout.array_repository ().insert (array)), pp.second));
        } else {
          cell.shapes (ll.second).insert (db::Shape::path_ptr_array_type (path_ptr, db::Disp (pos), layout.array_repository ().insert (array)));
        }

      } else {

        db::PathRef path_ref (path, layout.shape_repository ());

        RepetitionIterator p = mm_repetition.get ().begin ();
        while (! p.at_end ()) {
          if (pp.first) {
            cell.shapes (ll.second).insert (db::PathRefWithProperties (path_ref.transformed (db::Disp (pos + *p)), pp.second));
          } else {
            cell.shapes (ll.second).insert (path_ref.transformed (db::Disp (pos + *p)));
          }
          ++p;
        }

      }

    }

  } else {

    std::pair<bool, db::properties_id_type> pp = read_element_properties (layout.properties_repository (), false);

    if (ll.first) {

      //  convert the OASIS circle into a single-point path.
      db::Path path;
      path.width (2 * mm_circle_radius.get ());
      path.extensions (mm_circle_radius.get (), mm_circle_radius.get ());
      path.round (true);
      db::Point p0 (0, 0);
      path.assign (&p0, &p0 + 1);
      db::PathRef path_ref (path, layout.shape_repository ());

      if (pp.first) {
        layout.cell (cell_index).shapes (ll.second).insert (db::PathRefWithProperties (path_ref.transformed (db::Disp (pos)), pp.second));
      } else {
        layout.cell (cell_index).shapes (ll.second).insert (path_ref.transformed (db::Disp (pos)));
      }

    }

  }
}

void
OASISReader::reset_modal_variables ()
{
  //  reset modal variables
  mm_repetition.reset ();
  mm_placement_cell.reset ();
  mm_placement_x = 0;
  mm_placement_y = 0;
  mm_layer.reset ();
  mm_datatype.reset ();
  mm_textlayer.reset ();
  mm_texttype.reset ();
  mm_text_x = 0;
  mm_text_y = 0;
  mm_text_string.reset ();
  mm_text_string_id.reset ();
  mm_geometry_x = 0;
  mm_geometry_y = 0;
  mm_geometry_w.reset ();
  mm_geometry_h.reset ();
  mm_polygon_point_list.reset ();
  mm_path_halfwidth.reset ();
  mm_path_start_extension.reset ();
  mm_path_end_extension.reset ();
  mm_path_point_list.reset ();
  mm_ctrapezoid_type.reset ();
  mm_circle_radius.reset ();
  mm_last_property_name.reset ();
  mm_last_property_is_sprop.reset ();
  mm_last_value_list.reset ();
}

void
OASISReader::do_read_cell (db::cell_index_type cell_index, db::Layout &layout)
{
  //  clears current instance list
  m_instances.clear ();
  m_instances_with_props.clear ();

  m_progress.set (m_stream.pos ());

  bool xy_absolute = true;

  bool has_context = false;
  std::vector <tl::Variant> context_strings;
  db::PropertiesRepository::properties_set cell_properties;

  //  read next record
  while (true) {

    m_progress.set (m_stream.pos ());

    unsigned char r = get_byte ();

    if (r == 0 /*PAD*/) {

      //  simply skip.
      mark_start_table ();

    } else if (r == 15 /*XYABSOLUTE*/) {

      //  switch to absolute mode
      xy_absolute = true;

      mark_start_table ();

    } else if (r == 16 /*XYRELATIVE*/) {

      //  switch to relative mode
      xy_absolute = false;

      mark_start_table ();

    } else if (r == 17 || r == 18 /*PLACEMENT*/) {

      do_read_placement (r, xy_absolute, layout, m_instances, m_instances_with_props);

    } else if (r == 19 /*TEXT*/) {

      do_read_text (xy_absolute, cell_index, layout);

    } else if (r == 20 /*RECTANGLE*/) {

      do_read_rectangle (xy_absolute, cell_index, layout);

    } else if (r == 21 /*POLYGON*/) {

      do_read_polygon (xy_absolute, cell_index, layout);

    } else if (r == 22 /*PATH*/) {

      do_read_path (xy_absolute, cell_index, layout);

    } else if (r == 23 || r == 24 || r == 25 /*TRAPEZOID*/) {

      do_read_trapezoid (r, xy_absolute, cell_index, layout);

    } else if (r == 26 /*CTRAPEZOID*/) {

      do_read_ctrapezoid (xy_absolute, cell_index, layout);

    } else if (r == 27 /*CIRCLE*/) {

      do_read_circle (xy_absolute, cell_index, layout);

    } else if (r == 28 || r == 29 /*PROPERTY*/) {

      if (r == 28 /*PROPERTY*/) {
        read_properties (layout.properties_repository ());
      }

      if (! mm_last_property_is_sprop.get () && mm_last_property_name.get () == m_klayout_context_property_name_id) {
        has_context = true;
        context_strings.reserve (mm_last_value_list.get ().size ());
        for (std::vector<tl::Variant>::const_iterator v = mm_last_value_list.get ().begin (); v != mm_last_value_list.get ().end (); ++v) {
          context_strings.push_back (*v);
        }
      } else {
        //  store layout properties
        store_last_properties (layout.properties_repository (), cell_properties, true);
      }

      mark_start_table ();

    } else if (r == 32 /*XELEMENT*/) {

      //  read over
      get_ulong ();
      get_str ();

      read_element_properties (layout.properties_repository (), true);

    } else if (r == 33 /*XGEOMETRY*/) {

      //  read over.

      unsigned char m = get_byte ();
      get_ulong ();

      if (m & 0x1) {
        mm_layer = get_uint ();
      }

      if (m & 0x2) {
        mm_datatype = get_uint ();
      }

      //  data payload:
      get_str ();

      if (m & 0x10) {
        db::Coord x;
        get (x);
        if (xy_absolute) {
          mm_geometry_x = x;
        } else {
          mm_geometry_x = x + mm_geometry_x.get ();
        }
      }

      if (m & 0x8) {
        db::Coord y;
        get (y);
        if (xy_absolute) {
          mm_geometry_y = y;
        } else {
          mm_geometry_y = y + mm_geometry_y.get ();
        }
      }

      if ((m & 0x4) && read_repetition ()) {
        //  later: handle XGEOMETRY with repetition
      }

      read_element_properties (layout.properties_repository (), true);

    } else if (r == 34 /*CBLOCK*/) {

      unsigned int type = get_uint ();
      if (type != 0) {
        error (tl::sprintf (tl::to_string (tr ("Invalid CBLOCK compression type %d")), type));
      }

      size_t dummy = 0;
      get (dummy);  // uncomp-byte-count - not needed
      get (dummy);  // comp-byte-count - not needed

      //  put the stream into deflating mode
      m_stream.inflate ();

    } else {
      //  put the byte back into the stream
      m_stream.unget (1);
      break;
    }

  }

  if (! cell_properties.empty ()) {
    layout.cell (cell_index).prop_id (layout.properties_repository ().properties_id (cell_properties));
  }

  //  insert all instances collected (inserting them once is
  //  more effective than doing this every time)
  if (! m_instances.empty ()) {
    layout.cell (cell_index).insert (m_instances.begin (), m_instances.end ());
    //  clear immediately, because if the cell is cleared before the instances are deleted, the
    //  array pointers (living in the repository) may no longer be valid
    m_instances.clear ();
  }
  if (! m_instances_with_props.empty ()) {
    layout.cell (cell_index).insert (m_instances_with_props.begin (), m_instances_with_props.end ());
    //  see above.
    m_instances_with_props.clear ();
  }

  //  store the context strings for later
  if (has_context) {
    m_context_strings_per_cell [cell_index].swap (context_strings);
  }

  m_cellname = "";
}

}


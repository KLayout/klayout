
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



#include "dbCIFReader.h"
#include "dbStream.h"
#include "dbObjectWithProperties.h"
#include "dbArray.h"
#include "dbStatic.h"

#include "tlException.h"
#include "tlString.h"
#include "tlClassRegistry.h"

#include <cctype>

namespace db
{

// ---------------------------------------------------------------
//  CIFReader


CIFReader::CIFReader (tl::InputStream &s)
  : m_stream (s),
    m_progress (tl::to_string (tr ("Reading CIF file")), 1000),
    m_dbu (0.001), m_wire_mode (0)
{
  m_progress.set_format (tl::to_string (tr ("%.0fk lines")));
  m_progress.set_format_unit (1000.0);
  m_progress.set_unit (100000.0);
}

CIFReader::~CIFReader ()
{
  //  .. nothing yet ..
}

const LayerMap &
CIFReader::read (db::Layout &layout, const db::LoadLayoutOptions &options)
{
  init (options);

  const db::CIFReaderOptions &specific_options = options.get_options<db::CIFReaderOptions> ();
  m_wire_mode = specific_options.wire_mode;
  m_dbu = specific_options.dbu;

  set_layer_map (specific_options.layer_map);
  set_create_layers (specific_options.create_other_layers);
  set_keep_layer_names (specific_options.keep_layer_names);

  prepare_layers (layout);

  do_read (layout);

  finish_layers (layout);
  return layer_map_out ();
}

const LayerMap &
CIFReader::read (db::Layout &layout)
{
  return read (layout, db::LoadLayoutOptions ());
}

void 
CIFReader::error (const std::string &msg)
{
  throw CIFReaderException (msg, m_stream.line_number (), m_cellname);
}

void 
CIFReader::warn (const std::string &msg, int wl)
{
  if (warn_level () < wl) {
    return;
  }

  // TODO: compress
  tl::warn << msg 
           << tl::to_string (tr (" (line=")) << m_stream.line_number ()
           << tl::to_string (tr (", cell=")) << m_cellname
           << ")";
}

/**
 *  @brief Skip blanks in the sense of CIF
 *  A blank in CIF is "any ASCII character except digit, upperChar, '-', '(', ')', or ';'"
 */
void 
CIFReader::skip_blanks()
{
  while (! m_stream.at_end ()) {
    char c = m_stream.peek_char ();
    if (isupper (c) || isdigit (c) || c == '-' || c == '(' || c == ')' || c == ';') {
      return;
    }
    m_stream.get_char ();
  }
}

/**
 *  @brief Skips separators
 */
void 
CIFReader::skip_sep ()
{
  while (! m_stream.at_end ()) {
    char c = m_stream.peek_char ();
    if (isdigit (c) || c == '-' || c == '(' || c == ')' || c == ';') {
      return;
    }
    m_stream.get_char ();
  }
}

/**
 *  @brief Skip comments
 *  This assumes that the reader is after the first '(' and it will stop
 *  after the final ')'.
 */
void 
CIFReader::skip_comment ()
{
  char c;
  int bl = 0;
  while (! m_stream.at_end () && ((c = m_stream.get_char ()) != ')' || bl > 0)) {
    // check for nested comments (bl is the nesting level)
    if (c == '(') {
      ++bl;
    } else if (c == ')') {
      --bl;
    }
  }
}

/**
 *  @brief Gets a character and issues an error if the stream is at the end
 */
char 
CIFReader::get_char ()
{
  if (m_stream.at_end ()) {
    error ("Unexpected end of file");
    return 0;
  } else {
    m_progress.set (m_stream.line_number ());
    return m_stream.get_char ();
  }
}

/**
 *  @brief Tests whether the next character is a semicolon (after blanks)
 */
bool 
CIFReader::test_semi ()
{
  skip_blanks ();
  if (! m_stream.at_end () && m_stream.peek_char () == ';') {
    return true;
  } else {
    return false;
  }
}

/**
 *  @brief Tests whether a semicolon follows and issue an error if not
 */
void
CIFReader::expect_semi ()
{
  if (! test_semi ()) {
    error ("Expected ';' command terminator");
  } else {
    get_char ();
  }
}

/**
 *  @brief Skips all until the next semicolon
 */
void
CIFReader::skip_to_end ()
{
  while (! m_stream.at_end () && m_stream.get_char () != ';') {
    ;
  }
}

/**
 *  @brief Fetches an integer
 */
int 
CIFReader::read_integer_digits ()
{
  if (m_stream.at_end () || ! isdigit (m_stream.peek_char ())) {
    error ("Digit expected");
  }

  int i = 0;
  while (! m_stream.at_end () && isdigit (m_stream.peek_char ())) {

    if (i > std::numeric_limits<int>::max () / 10) {

      error ("Integer overflow");
      while (! m_stream.at_end () && isdigit (m_stream.peek_char ())) {
        m_stream.get_char ();
      }

      return 0;

    }

    char c = m_stream.get_char ();
    i = i * 10 + int (c - '0');

  }

  return i;
}

/**
 *  @brief Fetches an integer
 */
int 
CIFReader::read_integer ()
{
  skip_sep ();
  return read_integer_digits ();
}

/**
 *  @brief Fetches a signed integer
 */
int 
CIFReader::read_sinteger ()
{
  skip_sep ();

  bool neg = false;
  if (m_stream.peek_char () == '-') {
    m_stream.get_char ();
    neg = true;
  }

  int i = read_integer_digits ();
  return neg ? -i : i;
}

/**
 *  @brief Fetches a string (layer name)
 */
const std::string &
CIFReader::read_name ()
{
  skip_blanks ();

  m_cmd_buffer.clear ();
  if (m_stream.at_end ()) {
    return m_cmd_buffer;
  }

  //  Note: officially only upper and digits are allowed in names. But we allow lower case and "_" too ...
  while (! m_stream.at_end () && (isupper (m_stream.peek_char ()) || islower (m_stream.peek_char ()) || m_stream.peek_char () == '_' || isdigit (m_stream.peek_char ()))) {
    m_cmd_buffer += m_stream.get_char ();
  }

  return m_cmd_buffer;
}

/**
 *  @brief Fetches a string (in labels, texts)
 */
const std::string &
CIFReader::read_string ()
{
  m_stream.skip ();

  m_cmd_buffer.clear ();
  if (m_stream.at_end ()) {
    return m_cmd_buffer;
  }

  char q = m_stream.peek_char ();
  if (q == '"' || q == '\'') {

    get_char ();

    //  read a quoted string (KLayout extension)
    while (! m_stream.at_end () && m_stream.peek_char () != q) {
      char c = m_stream.get_char ();
      if (c == '\\' && ! m_stream.at_end ()) {
        c = m_stream.get_char ();
      }
      m_cmd_buffer += c;
    }

    if (! m_stream.at_end ()) {
      get_char ();
    }

  } else {

    while (! m_stream.at_end () && !isspace (m_stream.peek_char ()) && m_stream.peek_char () != ';') {
      m_cmd_buffer += m_stream.get_char ();
    }

  }

  return m_cmd_buffer;
}

/**
 *  @brief Reads a double value (extension)
 */
double
CIFReader::read_double ()
{
  m_stream.skip ();

  //  read a quoted string (KLayout extension)
  m_cmd_buffer.clear ();
  while (! m_stream.at_end () && (isdigit (m_stream.peek_char ()) || m_stream.peek_char () == '.' || m_stream.peek_char () == '-' || m_stream.peek_char () == 'e' || m_stream.peek_char () == 'E')) {
    m_cmd_buffer += m_stream.get_char ();
  }

  double v = 0.0;
  tl::from_string (m_cmd_buffer, v);
  return v;
}

bool
CIFReader::read_cell (db::Layout &layout, db::Cell &cell, double sf, int level)
{
  if (fabs (sf - floor (sf + 0.5)) > 1e-6) {
    warn ("Scaling factor is not an integer - snapping errors may occur in cell '" + m_cellname + "'");
  }

  int nx = 0, ny = 0, dx = 0, dy = 0;
  int layer = -2; // no layer defined yet.
  int path_mode = -1;
  size_t insts = 0;
  size_t shapes = 0;
  size_t layer_specs = 0;
  std::vector <db::Point> poly_pts;

  while (true) {

    skip_blanks ();

    char c = get_char ();
    if (c == ';') {

      //  empty command

    } else if (c == '(') {

      skip_comment ();

    } else if (c == 'E') {

      if (level > 0) {
        error ("'E' command must be outside a cell specification");
      } 

      skip_blanks ();
      break;

    } else if (c == 'D') {

      skip_blanks ();

      c = get_char ();
      if (c == 'S') {

        //  DS command:
        //  "D" blank* "S" integer (sep integer sep integer)?

        unsigned int n = 0, denom = 1, divider = 1;
        n = read_integer ();
        if (! test_semi ()) {
          denom = read_integer ();
          divider = read_integer ();
          if (divider == 0) {
            error ("'DS' command: divider cannot be zero");
          }
        }

        expect_semi ();

        std::string outer_cell = "C" + tl::to_string (n);
        std::swap (m_cellname, outer_cell);

        std::map <unsigned int, db::cell_index_type>::const_iterator c = m_cells_by_id.find (n);
        db::cell_index_type ci;
        if (c == m_cells_by_id.end ()) {
          ci = layout.add_cell (m_cellname.c_str ());
          m_cells_by_id.insert (std::make_pair (n, ci));
        } else {
          ci = c->second;
        } 

        db::Cell &cell = layout.cell (ci);

        read_cell (layout, cell, sf * double (denom) / double (divider), level + 1);

        std::swap (m_cellname, outer_cell);

      } else if (c == 'F') {

        // DF command:
        // "D" blank* "F"
        if (level == 0) {
          error ("'DF' command must be inside a cell specification");
        } 

        //  skip the rest of the command
        skip_to_end ();

        break;

      } else if (c == 'D') {

        //  DD command:
        //  "D" blank* "D" integer

        read_integer ();
        warn ("'DD' command ignored");
        skip_to_end ();

      } else {

        error ("Invalid 'D' sub-command");
        skip_to_end ();

      }

    } else if (c == 'C') {

      //  C command:
      //  "C" integer transformation
      //  transformation := (blank* ("T" point |"M" blank* "X" |"M" blank* "Y" |"R" point)*)*

      ++insts;

      unsigned int n = read_integer ();
      std::map <unsigned int, db::cell_index_type>::const_iterator c = m_cells_by_id.find (n);
      if (c == m_cells_by_id.end ()) {
        std::string cn = "C" + tl::to_string (n);
        c = m_cells_by_id.insert (std::make_pair (n, layout.add_cell (cn.c_str ()))).first;
      } 

      db::DCplxTrans trans;

      while (! test_semi ()) {

        skip_blanks ();

        char ct = get_char ();
        if (ct == 'M') {

          skip_blanks ();

          char ct2 = get_char ();
          if (ct2 == 'X') {
            trans = db::DCplxTrans (db::FTrans::m90) * trans;
          } else if (ct2 == 'Y') {
            trans = db::DCplxTrans (db::FTrans::m0) * trans;
          } else {
            error ("Invalid 'M' transformation specification");
            //  skip everything to avoid more errors here
            while (! test_semi ()) {
              get_char ();
            }
          }

        } else if (ct == 'T') {

          int x = read_sinteger ();
          int y = read_sinteger ();
          trans = db::DCplxTrans (db::DVector (x * sf, y * sf)) * trans;

        } else if (ct == 'R') {

          int x = read_sinteger ();
          int y = read_sinteger ();

          if (y != 0 || x != 0) {
            double a = atan2 (double (y), double (x)) * 180.0 / M_PI;
            trans = db::DCplxTrans (1.0, a, false, db::DVector ()) * trans;
          }

        } else {
          error ("Invalid transformation specification");
          //  skip everything to avoid more errors here
          while (! test_semi ()) {
            get_char ();
          }
        }

      }

      if (nx > 0 || ny > 0) {
        if (trans.is_ortho () && ! trans.is_mag ()) {
          cell.insert (db::CellInstArray (db::CellInst (c->second), db::Trans (db::ICplxTrans (trans)), db::Vector (dx * sf, 0.0), db::Vector (0.0, dy * sf), std::max (1, nx), std::max (1, ny)));
        } else {
          cell.insert (db::CellInstArray (db::CellInst (c->second), db::ICplxTrans (trans), db::Vector (dx * sf, 0.0), db::Vector (0.0, dy * sf), std::max (1, nx), std::max (1, ny)));
        }
      } else {
        if (trans.is_ortho () && ! trans.is_mag ()) {
          cell.insert (db::CellInstArray (db::CellInst (c->second), db::Trans (db::ICplxTrans (trans))));
        } else {
          cell.insert (db::CellInstArray (db::CellInst (c->second), db::ICplxTrans (trans)));
        }
      }

      nx = ny = 0;
      dx = dy = 0;

      expect_semi ();

    } else if (c == 'L') {

      ++layer_specs;

      std::string name = read_name ();
      if (name.empty ()) {
        error ("Missing layer name in 'L' command");
      }

      std::pair<bool, unsigned int> ll = open_layer (layout, name);
      if (! ll.first) {
        layer = -1; // ignore geometric objects on this layer
      } else {
        layer = int (ll.second);
      }

      expect_semi ();

    } else if (c == 'B') {

      ++shapes;

      if (layer < 0) {

        if (layer < -1) {
          warn ("'B' command ignored since no layer was selected");
        }
        skip_to_end ();

      } else {

        unsigned int w = 0, h = 0;
        int x = 0, y = 0;

        w = read_integer ();
        h = read_integer ();
        x = read_sinteger ();
        y = read_sinteger ();

        int rx = 0, ry = 0;
        if (! test_semi ()) {
          rx = read_sinteger ();
          ry = read_sinteger ();
        }

        if (rx >= 0 && ry == 0) {

          cell.shapes ((unsigned int) layer).insert (db::Box (db::Point (sf * (x - 0.5 * w), sf * (y - 0.5 * h)), db::Point (sf * (x + 0.5 * w), sf * (y + 0.5 * h))));

        } else {

          double n = 1.0 / sqrt (double (rx) * double (rx) + double (ry) * double (ry));

          double xw = sf * w * 0.5 * rx * n, yw = sf * w * 0.5 * ry * n;
          double xh = -sf * h * 0.5 * ry * n, yh = sf * h * 0.5 * rx * n;

          db::Point c (sf * x, sf * y);

          db::Point points [4];
          points [0] = c + db::Vector (-xw - xh, -yw - yh);
          points [1] = c + db::Vector (-xw + xh, -yw + yh);
          points [2] = c + db::Vector (xw + xh, yw + yh);
          points [3] = c + db::Vector (xw - xh, yw - yh);

          db::Polygon p;
          p.assign_hull (points, points + 4);
          cell.shapes ((unsigned int) layer).insert (p);

        }

        expect_semi ();

      }

    } else if (c == 'P') {

      ++shapes;

      if (layer < 0) {

        if (layer < -1) {
          warn ("'P' command ignored since no layer was selected");
        }
        skip_to_end ();

      } else {

        poly_pts.clear ();

        while (! test_semi ()) {

          int rx = read_sinteger ();
          int ry = read_sinteger ();

          poly_pts.push_back (db::Point (sf * rx, sf * ry));

        }

        db::Polygon p;
        p.assign_hull (poly_pts.begin (), poly_pts.end ());
        cell.shapes ((unsigned int) layer).insert (p);

        expect_semi ();

      }

    } else if (c == 'R') {

      ++shapes;

      if (layer < 0) {

        if (layer < -1) {
          warn ("'R' command ignored since no layer was selected");
        }
        skip_to_end ();

      } else {

        int w = read_integer ();

        poly_pts.clear ();

        int rx = read_sinteger ();
        int ry = read_sinteger ();

        poly_pts.push_back (db::Point (sf * rx, sf * ry));

        db::Path p (poly_pts.begin (), poly_pts.end (), db::coord_traits <db::Coord>::rounded (sf * w), db::coord_traits <db::Coord>::rounded (sf * w / 2), db::coord_traits <db::Coord>::rounded (sf * w / 2), true);
        cell.shapes ((unsigned int) layer).insert (p);

        expect_semi ();

      }

    } else if (c == 'W') {

      ++shapes;

      if (layer < 0) {

        if (layer < -1) {
          warn ("'W' command ignored since no layer was selected");
        }

        skip_to_end ();

      } else {

        int w = read_integer ();

        poly_pts.clear ();

        while (! test_semi ()) {

          int rx = read_sinteger ();
          int ry = read_sinteger ();

          poly_pts.push_back (db::Point (sf * rx, sf * ry));

        }

        if (path_mode == 0 || (path_mode < 0 && m_wire_mode == 1)) {
          //  Flush-ended paths 
          db::Path p (poly_pts.begin (), poly_pts.end (), db::coord_traits <db::Coord>::rounded (sf * w), 0, 0, false);
          cell.shapes ((unsigned int) layer).insert (p);
        } else if (path_mode == 1 || (path_mode < 0 && m_wire_mode == 2)) {
          //  Round-ended paths
          db::Path p (poly_pts.begin (), poly_pts.end (), db::coord_traits <db::Coord>::rounded (sf * w), db::coord_traits <db::Coord>::rounded (sf * w / 2), db::coord_traits <db::Coord>::rounded (sf * w / 2), true);
          cell.shapes ((unsigned int) layer).insert (p);
        } else {
          //  Square-ended paths
          db::Path p (poly_pts.begin (), poly_pts.end (), db::coord_traits <db::Coord>::rounded (sf * w), db::coord_traits <db::Coord>::rounded (sf * w / 2), db::coord_traits <db::Coord>::rounded (sf * w / 2), false);
          cell.shapes ((unsigned int) layer).insert (p);
        }

        expect_semi ();

      }

    } else if (isdigit (c)) {

      char cc = m_stream.peek_char ();
      if (c == '9' && cc == '3') {

        get_char ();

        nx = read_sinteger ();
        dx = read_sinteger ();
        ny = read_sinteger ();
        dy = read_sinteger ();

      } else if (c == '9' && cc == '4') {

        get_char ();

        // label at location 
        ++shapes;

        if (layer < 0) {
          if (layer < -1) {
            warn ("'94' command ignored since no layer was selected");
          }
        } else {

          std::string text = read_string ();

          int rx = read_sinteger ();
          int ry = read_sinteger ();

          double h = 0.0;
          if (! test_semi ()) {
            h = read_double ();
          }

          db::Text t (text.c_str (), db::Trans (db::Vector (sf * rx, sf * ry)), db::coord_traits <db::Coord>::rounded (h / m_dbu));
          cell.shapes ((unsigned int) layer).insert (t);

        }

      } else if (c == '9' && cc == '5') {

        get_char ();

        // label in box 
        ++shapes;

        if (layer < 0) {
          if (layer < -1) {
            warn ("'95' command ignored since no layer was selected");
          }
        } else {

          std::string text = read_string ();

          //  TODO: box dimensions are ignored currently.
          read_sinteger ();
          read_sinteger ();

          int rx = read_sinteger ();
          int ry = read_sinteger ();

          db::Text t (text.c_str (), db::Trans (db::Vector (sf * rx, sf * ry)));
          cell.shapes ((unsigned int) layer).insert (t);

        }

      } else if (c == '9' && cc == '8') {

        get_char ();

        // Path type (0: flush, 1: round, 2: square)
        path_mode = read_integer ();

      } else if (c == '9' && ! isdigit (cc)) {

        m_cellname = read_string ();
        m_cellname = layout.uniquify_cell_name (m_cellname.c_str ());
        layout.rename_cell (cell.cell_index (), m_cellname.c_str ());

      } else {
        //  ignore the command 
      }

      skip_to_end ();

    } else {

      //  ignore the command 
      warn ("Unknown command ignored");
      skip_to_end ();

    }
    
  }

  //  The cell is considered non-empty if it contains more than one instance, at least one shape or
  //  has at least one "L" command.
  return insts > 1 || shapes > 0 || layer_specs > 0;
}

void 
CIFReader::do_read (db::Layout &layout)
{
  try {

    db::LayoutLocker locker (&layout);
  
    double sf = 0.01 / m_dbu;
    layout.dbu (m_dbu);

    m_cellname = "{CIF top level}";

    db::Cell &cell = layout.cell (layout.add_cell ());

    if (! read_cell (layout, cell, sf, 0)) {
      // The top cell is empty or contains a single instance: discard it.
      layout.delete_cell (cell.cell_index ());
    } else {
      layout.rename_cell (cell.cell_index (), layout.uniquify_cell_name ("CIF_TOP").c_str ());
    }

    m_cellname = std::string ();

    skip_blanks ();

    if (! m_stream.at_end ()) {
      warn ("E command is followed by more text");
    }

  } catch (db::CIFReaderException &ex) {
    throw ex;
  } catch (tl::Exception &ex) {
    error (ex.msg ());
  }
}

}


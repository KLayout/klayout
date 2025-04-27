
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



#include "dbMALYReader.h"
#include "dbStream.h"
#include "dbObjectWithProperties.h"
#include "dbArray.h"
#include "dbStatic.h"
#include "dbShapeProcessor.h"
#include "dbTechnology.h"

#include "tlException.h"
#include "tlString.h"
#include "tlClassRegistry.h"
#include "tlFileUtils.h"
#include "tlUri.h"

#include <cctype>
#include <string>

namespace db
{

// ---------------------------------------------------------------
//  MALYReader


MALYReader::MALYReader (tl::InputStream &s)
  : m_stream (s),
    m_progress (tl::to_string (tr ("Reading MALY file")), 1000),
    m_dbu (0.001)
{
  m_progress.set_format (tl::to_string (tr ("%.0fk lines")));
  m_progress.set_format_unit (1000.0);
  m_progress.set_unit (100000.0);
}

MALYReader::~MALYReader ()
{
  //  .. nothing yet ..
}

bool
MALYReader::test ()
{
  return true; // @@@
  try {

    std::string rec = read_record ();

    tl::Extractor ex (rec.c_str ());
    return ex.test ("BEGIN") && ex.test ("MALY");

  } catch (...) {
    return false;
  }
}

const LayerMap &
MALYReader::read (db::Layout &layout)
{
  return read (layout, db::LoadLayoutOptions ());
}

const LayerMap &
MALYReader::read (db::Layout &layout, const db::LoadLayoutOptions &options)
{
  init (options);

  prepare_layers (layout);

  const db::MALYReaderOptions &specific_options = options.get_options<db::MALYReaderOptions> ();
  m_dbu = specific_options.dbu;

  set_layer_map (specific_options.layer_map);
  set_create_layers (specific_options.create_other_layers);
  // @@@ set_keep_layer_names (specific_options.keep_layer_names);
  set_keep_layer_names (true);

  MALYData data = read_maly_file ();

  // @@@

  finish_layers (layout);
  return layer_map_out ();
}

std::string
MALYReader::read_record ()
{
  while (! m_stream.at_end ()) {
    std::string r = read_record_internal ();
    tl::Extractor ex (r.c_str ());
    if (ex.test ("+")) {
      error (tl::to_string (tr ("'+' character past first column - did you mean to continue a line?")));
    } else if (! ex.at_end ()) {
      return r;
    }
  }

  return std::string ();
}

std::string
MALYReader::read_record_internal ()
{
  std::string rec;

  while (! m_stream.at_end ()) {

    char c = m_stream.get_char ();

    //  skip comments
    if (c == '/') {
      char cc = m_stream.peek_char ();
      if (cc == '/') {
        while (! m_stream.at_end () && (c = m_stream.get_char ()) != '\n')
          ;
        if (m_stream.at_end ()) {
          break;
        }
      } else if (cc == '*') {
        m_stream.get_char ();  //  eat leading "*"
        while (! m_stream.at_end () && (m_stream.get_char () != '*' || m_stream.peek_char () != '/'))
          ;
        if (m_stream.at_end ()) {
          error (tl::to_string (tr ("/*...*/ comment not closed")));
        }
        m_stream.get_char ();  //  eat trailing "/"
        if (m_stream.at_end ()) {
          break;
        }
        c = m_stream.get_char ();
      }
    }

    if (c == '\n') {
      if (m_stream.peek_char () == '+') {
        //  continuation line
        m_stream.get_char ();  //  eat "+"
        if (m_stream.at_end ()) {
          break;
        }
        c = m_stream.get_char ();
      } else {
        break;
      }
    }

    if (c == '"' || c == '\'') {

      rec += c;

      //  skip quoted string
      char quote = c;
      while (! m_stream.at_end ()) {
        c = m_stream.get_char ();
        rec += c;
        if (c == quote) {
          quote = 0;
          break;
        } else if (c == '\\') {
          if (m_stream.at_end ()) {
            error (tl::to_string (tr ("Unexpected end of file inside quotee string")));
          }
          c = m_stream.get_char ();
          rec += c;
        } else if (c == '\n') {
          error (tl::to_string (tr ("Line break inside quoted string")));
        }
      }

      if (quote) {
        error (tl::to_string (tr ("Unexpected end of file inside quotee string")));
      }

    } else {
      rec += c;
    }

  }

  return rec;
}

MALYData
MALYReader::read_maly_file ()
{
  // @@@
  std::cout << "@@@ BEGIN_MALY" << std::endl;
  std::string rec;
  while (! (rec = read_record ()).empty ()) {
    std::cout << rec << std::endl;
  }
  std::cout << "@@@ END_MALY" << std::endl;
  // @@@

  return MALYData (); // @@@
}

void 
MALYReader::error (const std::string &msg)
{
  throw MALYReaderException (msg, m_stream.line_number (), m_stream.source ());
}

void 
MALYReader::warn (const std::string &msg, int wl)
{
  if (warn_level () < wl) {
    return;
  }

  if (first_warning ()) {
    tl::warn << tl::sprintf (tl::to_string (tr ("In file %s:")), m_stream.source ());
  }

  int ws = compress_warning (msg);
  if (ws < 0) {
    tl::warn << msg
             << tl::to_string (tr (" (line=")) << m_stream.line_number ()
             << tl::to_string (tr (", file=")) << m_stream.source ()
             << ")";
  } else if (ws == 0) {
    tl::warn << tl::to_string (tr ("... further warnings of this kind are not shown"));
  }
}

std::string
MALYReader::resolve_path (const std::string &path)
{
  tl::URI path_uri (path);

  if (tl::is_absolute (path_uri.path ())) {

    return path_uri.to_string ();

  } else {

    tl::URI source_uri (m_stream.source ());
    source_uri.set_path (tl::dirname (source_uri.path ()));
    return source_uri.resolved (tl::URI (path)).to_string ();

  }
}

void
MALYReader::do_read (db::Layout &layout, db::cell_index_type cell_index, tl::TextInputStream &stream)
{
  try {

    // @@@

  } catch (tl::Exception &ex) {
    error (ex.msg ());
  }
}

}


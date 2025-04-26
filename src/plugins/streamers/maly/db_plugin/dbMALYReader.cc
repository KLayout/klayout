
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

  // @@@

  finish_layers (layout);
  return layer_map_out ();
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


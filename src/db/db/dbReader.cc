
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


#include "dbReader.h"
#include "dbStream.h"
#include "tlClassRegistry.h"
#include "tlTimer.h"
#include "tlLog.h"

namespace db
{

// ---------------------------------------------------------------

DB_PUBLIC void
join_layer_names (std::string &s, const std::string &n)
{
  if (s == n) {
    return;
  }

  if (! s.empty ()) {

    size_t i = s.find (n);
    if (i != std::string::npos && (i == 0 || s.c_str ()[i - 1] == ';')) {
      char after = s.c_str ()[i + n.size ()];
      if (after == 0 || after == ';') {
        //  n is already contained in s
        return;
      }
    }

    s += ";";

  }

  s += n;
}

// ---------------------------------------------------------------
//  ReaderBase implementation

ReaderBase::ReaderBase () 
  : m_warnings_as_errors (false), m_warn_level (1)
{ 
}

ReaderBase::~ReaderBase () 
{ 
}

void
ReaderBase::set_warnings_as_errors (bool f)
{
  m_warnings_as_errors = f;
}

void
ReaderBase::init (const db::LoadLayoutOptions &options)
{
  m_warn_level = options.warn_level ();
}

// ---------------------------------------------------------------
//  Reader implementation

Reader::Reader (tl::InputStream &stream)
  : mp_actual_reader (0), m_stream (stream)
{
  //  Detect the format by asking all reader declarations
  for (tl::Registrar<db::StreamFormatDeclaration>::iterator rdr = tl::Registrar<db::StreamFormatDeclaration>::begin (); rdr != tl::Registrar<db::StreamFormatDeclaration>::end () && ! mp_actual_reader; ++rdr) {
    m_stream.reset ();
    if (rdr->detect (m_stream)) {
      m_stream.reset ();
      mp_actual_reader = rdr->create_reader (m_stream);
    }
  }

  if (! mp_actual_reader) {

    m_stream.reset ();
    std::string head = m_stream.read_all (4000);
    bool has_more (m_stream.get (1) != 0);

    throw db::ReaderUnknownFormatException (tl::to_string (tr ("Stream has unknown format: ")) + stream.source (), head, has_more);

  }
}

Reader::~Reader ()
{
  if (mp_actual_reader) {
    delete mp_actual_reader;
    mp_actual_reader = 0;
  }
}

const db::LayerMap &
Reader::read (db::Layout &layout, const db::LoadLayoutOptions &options)
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Reading file: ")) + m_stream.source ());

  return mp_actual_reader->read (layout, options);
}

const db::LayerMap &
Reader::read (db::Layout &layout)
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Reading file: ")) + m_stream.source ());

  return mp_actual_reader->read (layout);
}

}


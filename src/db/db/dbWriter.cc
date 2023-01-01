
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


#include "dbWriter.h"
#include "dbStream.h"
#include "tlClassRegistry.h"
#include "tlAssert.h"
#include "tlStream.h"
#include "tlTimer.h"
#include "tlLog.h"

namespace db
{

Writer::Writer (const db::SaveLayoutOptions &options)
  : mp_writer (0), m_options (options)
{
  for (tl::Registrar<db::StreamFormatDeclaration>::iterator fmt = tl::Registrar<db::StreamFormatDeclaration>::begin (); fmt != tl::Registrar<db::StreamFormatDeclaration>::end () && ! mp_writer; ++fmt) {
    if (m_options.format () == fmt->format_name ()) {
      mp_writer = fmt->create_writer ();
    }
  }
  if (! mp_writer) {
    throw tl::Exception (tl::to_string (tr ("Unknown stream format: %s")), m_options.format ());
  }
}

Writer::~Writer ()
{
  if (mp_writer) {
    delete mp_writer;
  }
  mp_writer = 0;
}

void 
Writer::write (db::Layout &layout, tl::OutputStream &stream)
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Writing file: ")) + stream.path ());

  tl_assert (mp_writer != 0);
  mp_writer->write (layout, stream, m_options);
}

}


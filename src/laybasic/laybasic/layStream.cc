
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

#if defined(HAVE_QT)

#include "layStream.h"
#include "layPlugin.h"
#include "laybasicConfig.h"
#include "dbStream.h"
#include "dbLoadLayoutOptions.h"
#include "dbSaveLayoutOptions.h"
#include "tlAssert.h"
#include "tlString.h"

namespace lay
{

// ------------------------------------------------------------------
//  StreamPluginDeclarationBase implementation

db::StreamFormatDeclaration &
StreamPluginDeclarationBase::stream_fmt () 
{
  if (! mp_stream_fmt) {
    for (tl::Registrar<db::StreamFormatDeclaration>::iterator fmt = tl::Registrar<db::StreamFormatDeclaration>::begin (); fmt != tl::Registrar<db::StreamFormatDeclaration>::end (); ++fmt) {
      if (fmt->format_name () == m_format_name) {
        mp_stream_fmt = const_cast <db::StreamFormatDeclaration *> (&*fmt);
        break;
      }
    }
  }

  tl_assert (mp_stream_fmt);

  return *mp_stream_fmt;
}

// ------------------------------------------------------------------
//  StreamReaderPluginDeclaration implementation

const StreamReaderPluginDeclaration *StreamReaderPluginDeclaration::plugin_for_format (const std::string &format_name)
{
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    const StreamReaderPluginDeclaration *decl = dynamic_cast <const StreamReaderPluginDeclaration *> (&*cls);
    if (decl && decl->format_name () == format_name) {
      return decl;
    }
  }
  return 0;
}

// ------------------------------------------------------------------
//  StreamWriterPluginDeclaration implementation

const StreamWriterPluginDeclaration *StreamWriterPluginDeclaration::plugin_for_format (const std::string &format_name)
{
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    const StreamWriterPluginDeclaration *decl = dynamic_cast <const StreamWriterPluginDeclaration *> (&*cls);
    if (decl && decl->format_name () == format_name) {
      return decl;
    }
  }
  return 0;
}

}

#endif

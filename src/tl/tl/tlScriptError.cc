
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#include "tlScriptError.h"
#include "tlString.h"

namespace tl
{

// -------------------------------------------------------------------
//  BacktraceElement implementation

BacktraceElement::BacktraceElement (const std::string &_file, int _line)
  : file (_file), line (_line)
{
  // .. nothing yet ..
}

BacktraceElement::BacktraceElement (const std::string &_file, int _line, const std::string _more_info)
  : file (_file), line (_line), more_info (_more_info)
{
  // .. nothing yet ..
}

BacktraceElement::BacktraceElement ()
  : line (0)
{
  // .. nothing yet ..
}

std::string 
BacktraceElement::to_string() const
{
  if (line > 0) {
    if (! more_info.empty ()) {
      return file + ":" + tl::to_string (line) + ":" + more_info;
    } else {
      return file + ":" + tl::to_string (line);
    }
  } else {
    return more_info;
  }
}

// -------------------------------------------------------------------
//  ScriptError implementation

std::string
ScriptError::basic_msg () const
{
  return tl::Exception::msg ();
}

std::string 
ScriptError::msg () const
{
  std::string m = basic_msg ();

  if (! m_context.empty ()) {
    m += tl::to_string (tr (" in ")) + m_context;
  }

  for (std::vector<BacktraceElement>::const_iterator bt = backtrace ().begin (); bt != backtrace ().end (); ++bt) {
    m += "\n  ";
    m += bt->to_string ();
  }

  return m;
}

}




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

#include "tlScriptError.h"
#include "tlString.h"
#include "tlInclude.h"

namespace tl
{

// -------------------------------------------------------------------
//  BacktraceElement implementation

BacktraceElement::BacktraceElement (const std::string &_file, int _line)
  : file (_file), line (_line)
{
  translate_includes ();
}

BacktraceElement::BacktraceElement (const std::string &_file, int _line, const std::string _more_info)
  : file (_file), line (_line), more_info (_more_info)
{
  translate_includes ();
}

BacktraceElement::BacktraceElement ()
  : line (0)
{
  // .. nothing yet ..
}

void
BacktraceElement::translate_includes ()
{
  if (line < 1) {
    return;
  }

  std::pair<std::string, int> fl = tl::IncludeExpander::translate_to_original (file, line);
  if (fl.second > 0) {
    file = fl.first;
    line = fl.second;
  }
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

static std::string make_basic_msg (const char *text, const char *cls)
{
  std::string msg;
  if (*cls) {
    msg = cls;
  }
  if (*cls && *text) {
    msg += ": ";
  }
  if (*text) {
    msg += text;
  }
  return msg;
}


ScriptError::ScriptError (const char *msg, const char *cls, const std::vector<BacktraceElement> &backtrace)
  : tl::Exception (make_basic_msg (msg, cls)), m_line (-1), m_cls (cls), m_backtrace (backtrace)
{
  //  .. nothing yet ..
}

ScriptError::ScriptError (const char *msg, const char *sourcefile, int line, const char *cls, const std::vector<BacktraceElement> &backtrace)
  : tl::Exception (make_basic_msg (msg, cls)), m_sourcefile (sourcefile), m_line (line), m_cls (cls), m_backtrace (backtrace)
{
  translate_includes ();
}

ScriptError::ScriptError (const ScriptError &d)
  : tl::Exception (d), m_sourcefile (d.m_sourcefile), m_line (d.m_line), m_cls (d.m_cls), m_context (d.m_context), m_backtrace (d.m_backtrace)
{
  //  .. nothing yet ..
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

void
ScriptError::translate_includes ()
{
  if (m_line < 1) {
    return;
  }

  std::pair<std::string, int> fl = tl::IncludeExpander::translate_to_original (m_sourcefile, m_line);
  if (fl.second > 0) {
    m_sourcefile = fl.first;
    m_line = fl.second;
  }
}

}




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


#include "lymMacroInterpreter.h"
#include "lymMacro.h"

#include "tlInternational.h"
#include "tlException.h"
#include "tlClassRegistry.h"

namespace lym
{

void 
MacroInterpreter::execute (const lym::Macro *) const
{
  throw tl::Exception (tl::to_string (QObject::tr ("execute() implementation missing for DSL interpreter")));
}

bool 
MacroInterpreter::can_run (const lym::Macro *macro)
{
  for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
    if (cls.current_name () == macro->dsl_interpreter ()) {
      return true;
    }
  }
  return false;
}

void 
MacroInterpreter::execute_macro (const lym::Macro *macro)
{
  for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
    if (cls.current_name () == macro->dsl_interpreter ()) {
      cls->execute (macro);
      return;
    }
  }

  throw tl::Exception (tl::to_string (QObject::tr ("No interpreter registered for DSL type '")) + macro->dsl_interpreter () + "'");
}

std::string 
MacroInterpreter::syntax_scheme (const std::string &dsl_name)
{
  for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
    if (cls.current_name () == dsl_name) {
      return cls->syntax_scheme ();
    }
  }

  return std::string ();
}

Macro::Format
MacroInterpreter::storage_scheme (const std::string &dsl_name)
{
  for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
    if (cls.current_name () == dsl_name) {
      return cls->storage_scheme ();
    }
  }

  return Macro::PlainTextFormat;
}

Macro::Interpreter
MacroInterpreter::debugger_scheme (const std::string &dsl_name)
{
  for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
    if (cls.current_name () == dsl_name) {
      return cls->debugger_scheme ();
    }
  }

  return Macro::Ruby;
}

std::string 
MacroInterpreter::description (const std::string &dsl_name)
{
  for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
    if (cls.current_name () == dsl_name) {
      return cls->description ();
    }
  }

  return std::string ();
}

std::string 
MacroInterpreter::suffix (const std::string &dsl_name)
{
  for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
    if (cls.current_name () == dsl_name) {
      return cls->suffix ();
    }
  }

  return std::string ();
}

}


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


#include "lymMacroInterpreter.h"
#include "lymMacro.h"

#include "tlInternational.h"
#include "tlException.h"
#include "tlClassRegistry.h"
#include "tlInclude.h"

#include <cstring>

namespace lym
{

tl::Executable *
MacroInterpreter::executable (const lym::Macro *) const
{
  throw tl::Exception (tl::to_string (tr ("executable() implementation missing for DSL interpreter")));
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

namespace
{

class MacroIncludeFileResolver
  : public tl::IncludeFileResolver
{
public:
  MacroIncludeFileResolver () { }

  std::string get_text (const std::string &path) const
  {
    //  Use lym::Macro to resolve texts - this strips the XML envelope.
    //  Intentionally not compatibility check is made to allow using any
    //  type of input and specifically any extension.
    lym::Macro macro;
    macro.load_from (path);
    return macro.text ();
  }
};

}

std::pair<std::string, std::string>
MacroInterpreter::include_expansion (const lym::Macro *macro)
{
  MacroIncludeFileResolver include_file_resolver;

  std::pair<std::string, std::string> res;
  res.first = tl::IncludeExpander::expand (macro->path (), macro->text (), res.second, &include_file_resolver).to_string ();

  if (res.first != macro->path ()) {

    //  Fix the macro's text such that include expansion does not spoil __FILE__ or __LINE__ variables
    //  NOTE: this will modify the column for syntax errors. Let's hope this tiny error is acceptable.
    //  TODO: this substitution may be somewhat naive ...

    Macro::Interpreter ip = macro->interpreter ();
    if (macro->interpreter () == Macro::DSLInterpreter) {
      if (syntax_scheme () == "ruby") {
        ip = Macro::Ruby;
      } else if (syntax_scheme () == "python") {
        ip = Macro::Python;
      }
    }

    if (ip == Macro::Ruby) {

      std::string subst;
      const std::string file_const ("__FILE__");
      const std::string line_const ("__LINE__");

      for (const char *cp = res.second.c_str (); *cp; ) {
        if (strncmp (cp, file_const.c_str (), file_const.size ()) == 0 && !isalnum (cp[file_const.size ()]) && cp[file_const.size ()] != '_') {
          subst += "RBA::Macro::real_path(__FILE__, __LINE__)";
          cp += file_const.size ();
        } else if (strncmp (cp, line_const.c_str (), line_const.size ()) == 0 && !isalnum (cp[line_const.size ()]) && cp[line_const.size ()] != '_') {
          subst += "RBA::Macro::real_line(__FILE__, __LINE__)";
          cp += line_const.size ();
        } else {
          subst += *cp++;
        }
      }

      res.second = subst;

    }

  }

  return res;
}

void 
MacroInterpreter::execute_macro (const lym::Macro *macro)
{
  for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {

    if (cls.current_name () == macro->dsl_interpreter ()) {

      std::unique_ptr<tl::Executable> eo (cls->executable (macro));
      if (eo.get ()) {
        eo->do_execute ();
      }

      return;

    }

  }

  throw tl::Exception (tl::to_string (tr ("No interpreter registered for DSL type '")) + macro->dsl_interpreter () + "'");
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

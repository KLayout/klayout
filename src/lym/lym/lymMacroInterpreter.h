
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


#ifndef HDR_lymMacroInterpreter
#define HDR_lymMacroInterpreter

#include "lymCommon.h"

#include "tlRecipe.h"
#include "gsiObject.h"
#include "tlClassRegistry.h"

#include "lymMacro.h"

#include <string>
#include <vector>

namespace lym
{

/**
 *  @brief A base class for a DSL (domain specific language) interpreter
 *
 *  DSL interpreters can be registered inside the macro execution engine
 *  and are employed to run macros of the interpreter type "DSLInterpreter".
 *  DSL interpreters are identified by name and are implemented through
 *  a method "execute" which receives the text of the DSL script that this
 *  interpreter is understanding.
 *
 *  An interpreter is registered using the class registration mechanism of 
 *  tl::RegisteredClass.
 */

class LYM_PUBLIC MacroInterpreter
  : public gsi::ObjectBase
{
public:
  /**
   *  @brief The constructor
   */
  MacroInterpreter ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates the executable for a macro
   *
   *  The caller will delete the returned object.
   */
  virtual tl::Executable *executable (const lym::Macro *macro) const;

  /**
   *  @brief Returns the storage scheme
   *
   *  The storage scheme is used to determine how the macro's text shall be stored.
   *  The scheme can be MacroFormat for the macro XML format or PlainTextFormat for plain text.
   */
  virtual lym::Macro::Format storage_scheme () const
  {
    return Macro::PlainTextFormat;
  }

  /**
   *  @brief Returns the syntax scheme
   *
   *  The return value specifies the syntax highlighter scheme for this 
   *  DSL. Return an empty string for no highlighting and "ruby" to use 
   *  Ruby highlighting.
   */
  virtual std::string syntax_scheme () const
  {
    return std::string ();
  }

  /**
   *  @brief Returns the debugging scheme
   *
   *  The return value specifies the debugger used for this 
   *  DSL. The value DSLInterpreter does not make much sense and is ignored.
   */
  virtual Macro::Interpreter debugger_scheme () const
  {
    return Macro::None;
  }

  /**
   *  @brief Returns the description string 
   *
   *  The description string is used in the file selection dialog for example.
   *  If the suffix is empty, no description needs to be provided.
   */
  virtual std::string description () const
  {
    return std::string ();
  }

  /**
   *  @brief Returns the file suffix for files of this kind
   *
   *  If the file suffix is empty, the file will be masked as a .lym file.
   *  In that case, only the XML header inside the .lym file will tell whether
   *  it is a DSL or normal .lym file.
   */
  virtual std::string suffix () const
  {
    return std::string ();
  }

  /**
   *  @brief Returns the templates provided by this DSL interpreter
   *
   *  The templates are required by the macro editor in order to allow creation of
   *  new macros. The template objects must be new'd and added to the templates vector by the
   *  implementation of this method. These objects then will be deleted by the caller.
   */
  virtual void get_templates (std::vector<lym::Macro *> & /*templates*/) const
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Provides generic include file expansion
   *
   *  This method takes a given macro and substitutes include statements of the form '# %include ...' by the
   *  content of the respective file. Recursive include is supported.
   *  The return value of this method is a two-element array with two strings: the first one is a path string which
   *  holds the encoded information for translating back path/line number information into the original paths and
   *  line numbers. This first string needs to be passed to the actual script interpreter as the 'file path'. The
   *  second component of the returned array is the text of the macro with the include files substituted.
   */
  virtual std::pair<std::string, std::string> include_expansion (const lym::Macro *macro);

  /**
   *  @brief Runs the script for the DSL interpreter with the given name
   *
   *  This method locates the DSL interpreter with the given name and 
   *  runs the script on it.
   */
  static void execute_macro (const lym::Macro *macro);

  /**
   *  @brief Returns true, if a DSL interpreter is registered for the given macro
   */
  static bool can_run (const lym::Macro *macro);

  /**
   *  @brief Gets the syntax scheme for the given DSL name
   */
  static std::string syntax_scheme (const std::string &dsl_name);

  /**
   *  @brief Gets the storage scheme for the given DSL name
   */
  static Macro::Format storage_scheme (const std::string &dsl_name);

  /**
   *  @brief Gets the debugger scheme for the given DSL name
   */
  static Macro::Interpreter debugger_scheme (const std::string &dsl_name);

  /**
   *  @brief Gets the description for the given DSL name
   */
  static std::string description (const std::string &dsl_name);

  /**
   *  @brief Gets the suffix for the given DSL name
   */
  static std::string suffix (const std::string &dsl_name);
};

}

#endif



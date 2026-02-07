
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef _HDR_rba
#define _HDR_rba

#include "gsi.h"
#include "gsiInterpreter.h"
#include "tlScriptError.h"
#include "rbaCommon.h"

namespace rba
{

struct RubyInterpreterPrivateData;

/**
 *  @brief The ruby interpreter wrapper class
 */
class RBA_PUBLIC RubyInterpreter
  : public gsi::Interpreter
{
public:
  /**
   *  @brief Constructor
   *
   *  Creates and initializes the Interpreter. The static "initialize" method must have been called before.
   */
  RubyInterpreter ();

  /**
   *  @brief Destructor
   */
  ~RubyInterpreter ();

  /**
   *  @brief Add the given path to the search path ($: in ruby)
   */
  void add_path (const std::string &path, bool prepend = false);

  /**
   *  @brief Adds a package location to this interpreter
   */
  void add_package_location (const std::string &package_path);

  /**
   *  @brief Removes a package location from this interpreter
   */
  void remove_package_location (const std::string &package_path);

  /**
   *  @brief Requires the given module (ruby "require")
   */
  void require (const std::string &filename);

  /**
   *  @brief Set the given debugger scope
   *
   *  The debugger scope is the element to which the back trace will be reduced.
   *  Specifically this suppresses calls from inner functions called from that file.
   *  This is useful for DSL implementations.
   */
  void set_debugger_scope (const std::string &filename);

  /**
   *  @brief Removes the debugger scope
   */
  void remove_debugger_scope ();

  /**
   *  @brief Gets the current debugger scope
   */
  const std::string &debugger_scope () const;

  /**
   *  @brief Ignores the next exception
   *
   *  This is useful for suppressing re-raised exceptions in the debugger.
   */
  void ignore_next_exception ();

  /**
   *  @brief Load the given file (ruby "load")
   */
  void load_file (const std::string &filename);

  /**
   *  @brief Implementation of gsi::Interpreter::eval_string
   */
  void eval_string (const char *string, const char *filename = 0, int line = 1, int context = -1);

  /**
   *  @brief Implementation of gsi::Interpreter::eval_expr
   */
  tl::Variant eval_expr (const char *string, const char *filename = 0, int line = 1, int context = -1);
 
  /**
   *  @brief Implementation of gsi::Interpreter::eval_string_and_print
   */ 
  void eval_string_and_print (const char *string, const char *filename = 0, int line = 1, int context = -1);

  /**
   *  @brief Returns an inspector for the given context
   */
  virtual gsi::Inspector *inspector (int context = -1);

  /**
   *  @brief Defines a global variable with the given name and value 
   */
  void define_variable (const std::string &name, const tl::Variant &value);

  /**
   *  @brief Gets a value indicating whether the interpreter is available
   */
  bool available () const;

  /**
   *  @brief Installs the given console for output
   */
  void push_console (gsi::Console *console);

  /**
   *  @brief Removes the given console
   */
  void remove_console (gsi::Console *console);

  /**
   *  @brief Gets the current console
   */
  gsi::Console *current_console ();

  /**
   *  @brief Installs the given execution handler
   *
   *  The execution handler is informed when the interpreter enters code execution (also
   *  from the outside, i.e. a method reimplementing a C++ method or a event handler).
   *  During execution, the handler receives trace events which allow him to intercept
   *  execution.
   */
  void push_exec_handler (gsi::ExecutionHandler *exec_handler);

  /**
   *  @brief Removes the given execution handler
   */
  void remove_exec_handler (gsi::ExecutionHandler *exec_handler);

  /**
   *  @brief Indicates the start of a callback section
   *  This method will be called when C++ triggers Ruby execution.
   *  (see BEGIN_EXEC/END_EXEC macros)
   */
  void begin_exec ();

  /**
   *  @brief Indicates the end of a callback section
   *  This method will be called when Ruby returns into C++ code.
   *  (see BEGIN_EXEC/END_EXEC macros)
   */
  void end_exec ();

  /**
   *  @brief Gets a flag indicating whether exceptions are blocked from being seen in the debugger
   */
  bool exceptions_blocked ();

  /**
   *  @brief Sets a flag indicating whether exceptions are blocked from being seen in the debugger
   */
  void block_exceptions (bool f);

  /**
   *  @brief Fetch the version string
   *
   *  Returns an empty string when no Ruby interpreter is installed.
   */
  std::string version () const;

  /**
   *  @brief Provide a first (basic) initialization
   *
   *  This method must be called with a continuation main function since the Ruby
   *  interpreter want's to install itself atop of other functions on the stack:
   *
   *  @code
   *  int main_cont (int, char **)
   *  {
   *    // real main code ...
   *  }
   *
   *  int main (int argc, char **argv)
   *  {
   *    return RubyInterpreter::initialize (argc, argv, &main_cont);
   *  }
   *  @endcode
   */
  static int initialize (int &argc, char **argv, int (*main_func) (int &, char **));

  /**
   *  @brief The instance of the Ruby interpreter of 0 if there is none.
   */
  static RubyInterpreter *instance ();

  /**
   *  @brief Stores internal data - do not use this!
   */
  RubyInterpreterPrivateData *d;
};

class RBA_PUBLIC RubyStackTraceProvider
  : public gsi::StackTraceProvider
{
public:
  RubyStackTraceProvider (const std::string &scope);

  virtual std::vector<tl::BacktraceElement> stack_trace () const;
  virtual size_t scope_index () const;
  virtual int stack_depth () const;

  static size_t scope_index (const std::vector<tl::BacktraceElement> &bt, const std::string &scope);

  //  we could use this for ruby >= 1.9.3
#if 0
  static int count_stack_levels(void *arg, VALUE file, int line, VALUE method);
  extern "C" int rb_backtrace_each (int (*iter)(void *arg, VALUE file, int line, VALUE method), void *arg);
  virtual int stack_depth ();
#endif

private:
  const std::string &m_scope;
};

}

#endif


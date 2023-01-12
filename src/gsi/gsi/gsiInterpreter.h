
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

#ifndef _HDR_gsiInterpreter
#define _HDR_gsiInterpreter

#include "tlScriptError.h"
#include "tlClassRegistry.h"
#include "tlVariant.h"
#include "gsiCommon.h"

namespace gsi
{

class Interpreter;
class Inspector;

/**
 *  @brief The console interface for output
 *
 *  Using this interface, the interpreter can output text on stdout or stderr streams.
 *  This interface is implemented by ConsoleImpl.
 */
class GSI_PUBLIC Console
{
public:
  /**
   *  @brief The output stream designator
   *  Usually the console will not send the output to different streams but use
   *  the stream information to format the output properly.
   *  OS_stdout is normal output, OS_stderr is error output, 
   *  OS_echo is user feedback, specifically used for echoing any input.
   */
  enum output_stream { OS_none, OS_stdout, OS_stderr, OS_echo };

  /**
   *  @brief Constructor
   */
  Console () { }

  /**
   *  @brief Destructor
   */
  virtual ~Console () { }

  /**
   *  @brief Writes the given string to the console using the given stream
   *  The stream specifies how the output will be formatted (warning, error, ...).
   */
  virtual void write_str (const char *text, output_stream os) = 0;

  /**
   *  @brief Flushes the output (prepares new output)
   *  If a line is pending that line is terminated and a new line is started.
   */
  virtual void flush () = 0;

  /**
   *  @brief Returns true, if the console is a TTY (will probably enable coloring)
   */
  virtual bool is_tty () = 0;

  /**
   *  @brief Returns the number of columns for the terminal or 0 if unspecified
   */
  virtual int columns () = 0;

  /**
   *  @brief Returns the number of rows for the terminal or 0 if unspecified
   */
  virtual int rows () = 0;
};

/**
 *  @brief An interface delivering a stack trace for the ExecutionHandler's trace function
 */
class GSI_PUBLIC StackTraceProvider
{
public:
  StackTraceProvider () { }
  virtual ~StackTraceProvider () { }
  virtual std::vector<tl::BacktraceElement> stack_trace () const = 0;
  virtual int stack_depth () const = 0;
  virtual size_t scope_index () const = 0;
};

/**
 *  @brief An interface controlling the execution of a script
 *
 *  This interface controls the execution of a script. Basically when a script
 *  is executed, the handler allows intercepting the execution and implement breakpoints
 *  for example
 */
class GSI_PUBLIC ExecutionHandler
{
public:
  /**
   *  @brief The constructor
   */
  ExecutionHandler () { }

  /**
   *  @brief The destructor 
   */
  virtual ~ExecutionHandler () { }

  /**
   *  @brief Indicates the start of the execution of a block of code
   *
   *  This method is called when the execution begins. It can be used to clear any caches for example.
   */
  virtual void start_exec (Interpreter * /*interpreter*/) { }

  /**
   *  @brief Indicates then end of the execution of a block of code
   *
   *  This method is called after the execution has finished.
   */
  virtual void end_exec (Interpreter * /*interpreter*/) { }

  /**
   *  @brief Indicates that we descend into a call
   */
  virtual void push_call_stack (Interpreter * /*interpreter*/) { }

  /**
   *  @brief Indicates that we ascend into a call
   */
  virtual void pop_call_stack (Interpreter * /*interpreter*/) { }

  /**
   *  @brief Indicates that an exception is thrown
   */
  virtual void exception_thrown (Interpreter * /*interpreter*/, size_t /*file_id*/, int /*line*/, const std::string & /*eclass*/, const std::string & /*emsg*/, const StackTraceProvider * /*stack_trace_provider*/) { }

  /**
   *  @brief This method is called during execution
   */
  virtual void trace (Interpreter * /*interpreter*/, size_t /*file_id*/, int /*line*/, const StackTraceProvider * /*stack_trace_provider*/) { }

  /**
   *  @brief Associate a file path with an ID
   *
   *  This method is supposed to deliver an arbitrary integer ID (which can hold a pointer for
   *  example) for a given file path string. When the trace method is called, this ID is used to
   *  identify the file instead of the heavy file path.
   */
  virtual size_t id_for_path (Interpreter * /*interpreter*/, const std::string & /*path*/) { return 0; }
};

/**
 *  @brief A generic interpreter interface
 */
class GSI_PUBLIC Interpreter
  : public tl::RegisteredClass<Interpreter>
{
public:
  /**
   *  @brief Constructor
   */
  Interpreter (int position = 0, const char *name = "");

  /**
   *  @brief Destructor
   */
  virtual ~Interpreter ();

  /**
   *  @brief Add the given path to the search path ($: in ruby)
   */
  virtual void add_path (const std::string &path) = 0;

  /**
   *  @brief Requires the given module (ruby "require")
   */
  virtual void require (const std::string &filename) = 0;

  /**
   *  @brief Set the given debugger scope
   *
   *  The debugger scope is the element to which the back trace will be reduced.
   *  Specifically this suppresses calls from inner functions called from that file.
   *  This is useful for DSL implementations.
   */
  virtual void set_debugger_scope (const std::string &filename) = 0;

  /**
   *  @brief Removes the debugger scope
   */
  virtual void remove_debugger_scope () = 0;

  /**
   *  @brief Ignores the next exception
   *
   *  This is useful for suppressing re-raised exceptions in the debugger.
   */
  virtual void ignore_next_exception () = 0;

  /**
   *  @brief Load the given file (ruby "load")
   */
  virtual void load_file (const std::string &filename) = 0;

  /**
   *  @brief Evaluates the given string 
   *
   *  The filename and line gives the location at which the evaluation should begin.
   *  This location is indicated in the stack trace and error messages
   *
   *  "context" is the evaluation context index: -1 is the global context, 0 is the first
   *  context on the call stack, 1 the second and so on.
   */
  virtual void eval_string (const char *string, const char *filename = 0, int line = 1, int context = -1) = 0;

  /**
   *  @brief Evaluates the given expression string and returns the results as a variant
   *
   *  The filename and line gives the location at which the evaluation should begin.
   *  This location is indicated in the stack trace and error messages
   *  The result is converted to a variant and returned. When that is not possible, the 
   *  result is converted to a string and then put into the variant.
   *
   *  "context" is the evaluation context index: -1 is the global context, 0 is the first
   *  context on the call stack, 1 the second and so on.
   */
  virtual tl::Variant eval_expr (const char *string, const char *filename = 0, int line = 1, int context = -1) = 0;
 
  /**
   *  @brief Evaluates the given string and prints the result to stdout
   *
   *  The filename and line gives the location at which the evaluation should begin.
   *  This location is indicated in the stack trace and error messages
   *
   *  "context" is the evaluation context index: -1 is the global context, 0 is the first
   *  context on the call stack, 1 the second and so on.
   */ 
  virtual void eval_string_and_print (const char *string, const char *filename = 0, int line = 1, int context = -1) = 0;

  /**
   *  @brief Gets an inspector object for the given context
   *
   *  If context is -1, an inspector is returned for the global variables.
   *  Otherwise, 0 is the first context on the stack, 1 the second and so on.
   *  If no inspector can be provided, 0 is returned.
   *  The returned object must be deleted by the caller.
   */
  virtual Inspector *inspector (int context = -1) = 0;

  /**
   *  @brief Defines a global variable with the given name and value 
   */
  virtual void define_variable (const std::string &name, const tl::Variant &value) = 0;

  /**
   *  @brief Installs the given console for output
   */
  virtual void push_console (Console *console) = 0;

  /**
   *  @brief Removes the given console
   */
  virtual void remove_console (Console *console) = 0;

  /**
   *  @brief Gets a value indicating whether the interpreter is available
   */
  virtual bool available () const = 0;

  /**
   *  @brief Gets a string indicating the interpreter version
   */
  virtual std::string version () const = 0;

  /**
   *  @brief Installs the given execution handler
   *
   *  The execution handler is informed when the interpreter enters code execution (also
   *  from the outside, i.e. a method reimplementing a C++ method or a event handler).
   *  During execution, the handler receives trace events which allow him to intercept
   *  execution.
   */
  virtual void push_exec_handler (ExecutionHandler *exec_handler) = 0;

  /**
   *  @brief Removes the given execution handler
   */
  virtual void remove_exec_handler (ExecutionHandler *exec_handler) = 0;

  /**
   *  @brief Adds a package location to this interpreter
   *
   *  Interpreters may look for their packages here or in a subfolder
   *  of this path. For example, the Python interpreter will add
   *  <package location>/python to the sys.path search path.
   *  If this path is already registered, the interpreter shall ignore
   *  this request.
   */
  virtual void add_package_location (const std::string &package_path) = 0;

  /**
   *  @brief Removes a package location from this interpreter
   *
   *  This is the inverse of "add_package_location".
   */
  virtual void remove_package_location (const std::string &package_path) = 0;
};

/**
 *  @brief The interpreter registry
 */
extern GSI_PUBLIC tl::Registrar<Interpreter> interpreters;

}

#endif


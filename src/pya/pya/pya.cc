
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

#include <Python.h>
#include <frameobject.h>   //  Python - for traceback

#include "pya.h"
#include "pyaConvert.h"
#include "pyaInspector.h"
#include "pyaUtils.h"
#include "pyaHelpers.h"
#include "pyaModule.h"
#include "pyaCommon.h"

#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "tlLog.h"
#include "tlEnv.h"
#include "tlStream.h"
#include "tlTimer.h"
#include "tlFileUtils.h"
#include "tlString.h"
#include "tlInternational.h"

#if defined(HAVE_QT)
#  include <QCoreApplication>
#endif

//  For the installation path
#ifdef _WIN32
#  include <windows.h>
#endif

namespace pya
{

// --------------------------------------------------------------------------
//  PythonError implementation

PythonError::PythonError (const char *msg, const char *cls, const std::vector <tl::BacktraceElement> &backtrace)
  : tl::ScriptError (msg, cls, backtrace)
{ }

PythonError::PythonError (const char *msg, const char *sourcefile, int line, const char *cls, const std::vector <tl::BacktraceElement> &backtrace)
  : tl::ScriptError (msg, sourcefile, line, cls, backtrace)
{ }

PythonError::PythonError (const PythonError &d)
  : tl::ScriptError (d)
{ }

// --------------------------------------------------------------------------

/**
 *  @brief The python interpreter instance
 */
PythonInterpreter *sp_interpreter = 0;

// -------------------------------------------------------------------

/**
 *  @brief Normalizes the file path
 *  This function normalizes the file path so it only contains one
 *  kind of slashes on Windows.
 */
static
std::string normalize_path (const std::string &p)
{
#if defined(__WIN32)
  std::string np;
  np.reserve (p.size ());
  for (const char *c = p.c_str (); *c; ++c) {
    np += (*c == '\\' ? '/' : *c);
  }
  return np;
#else
  return p;
#endif
}

// -------------------------------------------------------------------
//  PythonStackTraceProvider definition and implementation

class PythonStackTraceProvider
  : public gsi::StackTraceProvider
{
public:
  PythonStackTraceProvider (PyFrameObject *frame, const std::string &scope)
    : m_scope (scope)
  {
    while (frame != NULL) {

#if PY_VERSION_HEX >= 0x030A0000
      int line = PyFrame_GetLineNumber(frame);
#else
      int line = frame->f_lineno;
#endif
      std::string fn;
#if PY_VERSION_HEX >= 0x030A0000
      if (test_type<std::string> (PyFrame_GetCode(frame)->co_filename, true)) {
        fn = normalize_path (python2c<std::string> (PyFrame_GetCode(frame)->co_filename));
#else
      if (test_type<std::string> (frame->f_code->co_filename, true)) {
        fn = normalize_path (python2c<std::string> (frame->f_code->co_filename));
#endif
      }
      m_stack_trace.push_back (tl::BacktraceElement (fn, line));

#if PY_VERSION_HEX >= 0x030A0000
      frame = PyFrame_GetBack(frame);
#else
      frame = frame->f_back;
#endif

    }
  }

  virtual std::vector<tl::BacktraceElement> stack_trace () const
  {
    return m_stack_trace;
  }

  virtual size_t scope_index () const
  {
    static int consider_scope = -1;

    //  disable scoped debugging (e.g. DRC script lines) if $KLAYOUT_PYA_DEBUG_SCOPE is set.
    if (consider_scope < 0) {
      consider_scope = tl::app_flag ("pya-debug-scope") ? 0 : 1;
    }
    if (! consider_scope) {
      return 0;
    }

    if (! m_scope.empty ()) {
      for (size_t i = 0; i < m_stack_trace.size (); ++i) {
        if (m_stack_trace [i].file == m_scope) {
          return i;
        }
      }
    }
    return 0;
  }

  virtual int stack_depth () const
  {
    return int (m_stack_trace.size ());
  }

private:
  std::string m_scope;
  std::vector<tl::BacktraceElement> m_stack_trace;
};

// --------------------------------------------------------------------------
//  The interpreter implementation

static const char *pya_module_name = "pya";

#if PY_MAJOR_VERSION < 3

static PyObject *
init_pya_module ()
{
  static PyMethodDef module_methods[] = {
    {NULL}  // Sentinel
  };
  return Py_InitModule3 (pya_module_name, module_methods, "KLayout Python API.");
}

#else

static PyObject *
init_pya_module ()
{
  static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    pya_module_name,        // m_name
    "KLayout Python API.",  // m_doc
    -1,                     // m_size
    NULL,                   // m_methods
    NULL,                   // m_reload
    NULL,                   // m_traverse
    NULL,                   // m_clear
    NULL,                   // m_free
  };
  return PyModule_Create (&moduledef);
}

#endif

static void reset_interpreter ()
{
  delete sp_interpreter;
  tl_assert (sp_interpreter == 0);
}

PythonInterpreter::PythonInterpreter (bool embedded)
  : gsi::Interpreter (0, "pya"),
    mp_current_console (0), mp_current_exec_handler (0), m_current_exec_level (0),
    m_in_trace (false), m_block_exceptions (false), m_ignore_next_exception (false),
    mp_current_frame (NULL), mp_py3_app_name (0), m_embedded (embedded)
{
  //  Don't attempt any additional initialization in the standalone module case
  if (! embedded) {

    sp_interpreter = this;

    //  this monitor whether Python shuts down and deletes the interpreter's
    //  instance.
    //  NOTE: this assumes, the interpreter was created with new(!)
    Py_AtExit (&reset_interpreter);

    return;

  }

  tl::SelfTimer timer (tl::verbosity () >= 21, "Initializing Python");

  std::string app_path = tl::get_app_path ();

#if PY_MAJOR_VERSION >= 3

  //  if set, use $KLAYOUT_PYTHONPATH to initialize the path
# if defined(_WIN32)

  tl_assert (sizeof (wchar_t) == 2);

  Py_SetPythonHome ((wchar_t *) L"");  //  really ignore $PYTHONHOME + without this, we get dummy error message about lacking path for libraries

  const wchar_t *python_path = _wgetenv (L"KLAYOUT_PYTHONPATH");
  if (python_path) {

    Py_SetPath (python_path);

  } else {

    //  If present, read the paths from a file in INST_PATH/.python-paths.txt.
    //  The content of this file is evaluated as an expression and the result
    //  is placed inside the Python path.

    try {

      std::string path;

      wchar_t buffer[MAX_PATH];
      int len;
      if ((len = GetModuleFileNameW (NULL, buffer, MAX_PATH)) > 0) {

        std::string inst_dir = tl::absolute_path (tl::to_string (std::wstring (buffer, len)));
        std::string path_file = tl::combine_path (inst_dir, ".python-paths.txt");
        if (tl::file_exists (path_file)) {

          tl::log << tl::to_string (tr ("Reading Python path from ")) << path_file;

          tl::InputStream path_file_stream (path_file);
          std::string path_file_text = path_file_stream.read_all ();

          tl::Eval eval;
          eval.set_global_var ("inst_path", tl::Variant (inst_dir));
          tl::Expression ex;
          eval.parse (ex, path_file_text.c_str ());
          tl::Variant v = ex.execute ();

          if (v.is_list ()) {
            for (tl::Variant::iterator i = v.begin (); i != v.end (); ++i) {
              if (! path.empty ()) {
                path += ";";
              }
              path += i->to_string ();
            }
          }

        }

      }

      Py_SetPath (tl::to_wstring (path).c_str ());

    } catch (tl::Exception &ex) {
      tl::error << tl::to_string (tr ("Evaluation of Python path expression failed")) << ": " << ex.msg ();
    } catch (...) {
      tl::error << tl::to_string (tr ("Evaluation of Python path expression failed"));
    }

  }

# else

  const char *python_path = getenv ("KLAYOUT_PYTHONPATH");
  if (python_path) {

    std::wstring path = tl::to_wstring (tl::to_string_from_local (python_path));
    Py_SetPath (path.c_str ());

  }

# endif

#endif

#if PY_MAJOR_VERSION < 3

  Py_SetProgramName (make_string (app_path));

  Py_InitializeEx (0 /*don't set signals*/);

  //  Set dummy argv[]
  //  TODO: more?
  char *argv[1] = { make_string (app_path) };
#if PY_MINOR_VERSION >= 7
  PySys_SetArgvEx (1, argv, 0);
#else
  PySys_SetArgv (1, argv);
#endif

  PyObject *module = init_pya_module ();
  if (module == NULL) {
    check_error ();
    return;
  }

  PyImport_ImportModule (pya_module_name);

#else

  //  Python 3 requires a unicode string for the application name
  PyObject *an = c2python (app_path);
  tl_assert (an != NULL);
  mp_py3_app_name = PyUnicode_AsWideCharString (an, NULL);
  tl_assert (mp_py3_app_name != NULL);
  Py_DECREF (an);
  Py_SetProgramName (mp_py3_app_name);

  PyImport_AppendInittab (pya_module_name, &init_pya_module);
  Py_InitializeEx (0 /*don't set signals*/);

  //  Set dummy argv[]
  //  TODO: more?
  wchar_t *argv[1] = { mp_py3_app_name };
  PySys_SetArgvEx (1, argv, 0);

  //  Import the module
  PyObject *module = PyImport_ImportModule (pya_module_name);
  if (module == NULL) {
    check_error ();
    return;
  }

#endif

  //  Build two objects that provide a way to redirect stdout, stderr
  //  and instantiate them two times for stdout and stderr.
  PYAChannelObject::make_class (module);
  m_stdout_channel = PythonRef (PYAChannelObject::create (gsi::Console::OS_stdout));
  m_stdout = PythonPtr (m_stdout_channel.get ());
  m_stderr_channel = PythonRef (PYAChannelObject::create (gsi::Console::OS_stderr));
  m_stderr = PythonPtr (m_stderr_channel.get ());

  sp_interpreter = this;

  m_pya_module.reset (new pya::PythonModule ());
  m_pya_module->init (pya_module_name, module);
  m_pya_module->make_classes ();
}

PythonInterpreter::~PythonInterpreter ()
{
  m_stdout_channel = PythonRef ();
  m_stderr_channel = PythonRef ();
  m_stdout = PythonPtr ();
  m_stderr = PythonPtr ();

  if (m_embedded) {

    Py_Finalize ();

    if (mp_py3_app_name) {
      PyMem_Free (mp_py3_app_name);
      mp_py3_app_name = 0;
    }

  }

  sp_interpreter = 0;
}

char *
PythonInterpreter::make_string (const std::string &s)
{
  m_string_heap.push_back (s);
  return const_cast<char *> (m_string_heap.back ().c_str ());
}

void
PythonInterpreter::add_path (const std::string &p)
{
  PyObject *path = PySys_GetObject ((char *) "path");
  if (path != NULL && PyList_Check (path)) {
    PyList_Append (path, c2python (p));
  }
}

void
PythonInterpreter::add_package_location (const std::string &package_path)
{
  std::string path = tl::combine_path (tl::absolute_file_path (package_path), "python");
  if (tl::file_exists (path) && m_package_paths.find (path) == m_package_paths.end ()) {
    m_package_paths.insert (path);
    add_path (path);
  }
}

void
PythonInterpreter::remove_package_location (const std::string & /*package_path*/)
{
  //  Currently, we do not really remove the location. Python might get screwed up this way.
}

void
PythonInterpreter::require (const std::string & /*filename*/)
{
  //  TODO: is there a way to implement that?
  throw tl::Exception (tl::to_string (tr ("'require' not implemented for Python interpreter")));
}

void
PythonInterpreter::set_debugger_scope (const std::string &filename)
{
  m_debugger_scope = filename;
}

void
PythonInterpreter::remove_debugger_scope ()
{
  m_debugger_scope.clear ();
}

void
PythonInterpreter::ignore_next_exception ()
{
  if (mp_current_exec_handler) {
    m_ignore_next_exception = true;
  }
}

void
PythonInterpreter::load_file (const std::string &filename)
{
  tl::InputStream stream (filename);
  eval_string (stream.read_all ().c_str (), filename.c_str (), 1);
}

/**
 *  @brief Gets the global and local variable lists for a given context index
 */
void
PythonInterpreter::get_context (int context, PythonRef &globals, PythonRef &locals, const char *file)
{
  globals = PythonRef ();
  locals = PythonRef ();

  PyFrameObject *f = mp_current_frame;
  while (f && context > 0) {
#if PY_VERSION_HEX >= 0x030B0000
    f = PyFrame_GetBack(f);
#else
    f = f->f_back;
#endif
    --context;
  }

  if (f) {

    //  merge "fast" (arguments etc.) to locals:
    //  (see PyFrame_GetLocals implementation)
    PyFrame_FastToLocals (f);

#if PY_VERSION_HEX >= 0x030B0000
    globals = PythonRef (PyObject_GetAttrString((PyObject*)f, "f_globals"));
    locals = PythonRef (PyObject_GetAttrString((PyObject*)f, "f_locals"), false);
#else
    globals = PythonRef (f->f_globals, false);
    locals = PythonRef (f->f_locals, false);
#endif

  } else {

    //  TODO: create a private namespace here? Or let the macros litter the global namespace?
    PythonPtr main_module (PyImport_AddModule ("__main__"));
    tl_assert (main_module);
    PythonPtr dict (PyModule_GetDict (main_module.get ()));
    tl_assert (dict);

    globals = dict;
    locals = dict;

    if (file) {

      PythonRef fn (c2python (file));
      PyDict_SetItemString (locals.get (), "__file__", fn.get ());

    }

  }
}

void
PythonInterpreter::eval_string (const char *expr, const char *file, int /*line*/, int context)
{
  PYTHON_BEGIN_EXEC

    //  TODO: what to do with "line"?
    PythonRef code (Py_CompileString(expr, file ? file : "(eval)", Py_file_input));
    if (! code) {
      check_error ();
      return;
    }

    PythonRef globals, locals;
    get_context (context, globals, locals, file);

#if PY_MAJOR_VERSION < 3
    PythonRef result (PyEval_EvalCode ((PyCodeObject *)code.get (), globals.get (), locals.get ()));
#else
    PythonRef result (PyEval_EvalCode (code.get (), globals.get (), locals.get ()));
#endif
    if (! result) {
      check_error ();
    }

  PYTHON_END_EXEC
}

/**
 *  @brief Evaluates the given expression or executes the given statement
 *  The expression is given int "string". If "eval_expr" is true, the string is evaluated as expression
 *  and the result is returned in the variant. If "eval_expr" is false, the string is evaluated, the
 *  result is printed to the currently active console and a nil variant is returned.
 */
tl::Variant
PythonInterpreter::eval_int (const char *expr, const char *file, int /*line*/, bool eval_expr, int context)
{
  tl::Variant ret;

  PYTHON_BEGIN_EXEC

    //  TODO: what to do with "line"?
    PythonRef code (Py_CompileString (expr, file ? file : "(eval)", eval_expr ? Py_eval_input : Py_single_input));
    if (! code) {
      check_error ();
      return ret;
    }

    PythonRef globals, locals;
    get_context (context, globals, locals, file);

#if PY_MAJOR_VERSION < 3
    PythonRef result (PyEval_EvalCode ((PyCodeObject*) code.get (), globals.get (), locals.get ()));
#else
    PythonRef result (PyEval_EvalCode (code.get (), globals.get (), locals.get ()));
#endif
    if (! result) {
      check_error ();
      return ret;
    }

    if (eval_expr) {
      ret = python2c<tl::Variant> (result.get ());
    } else {
      //  eval_expr == false will print the output -> terminate stream if required
      if (mp_current_console) {
        mp_current_console->flush ();
      }
    }

  PYTHON_END_EXEC

  return ret;
}

void
PythonInterpreter::eval_string_and_print (const char *expr, const char *file, int line, int context)
{
  eval_int (expr, file, line, false, context);
}

tl::Variant
PythonInterpreter::eval_expr (const char *expr, const char *file, int line, int context)
{
  return eval_int (expr, file, line, true, context);
}

gsi::Inspector *
PythonInterpreter::inspector (int context)
{
  PythonRef globals, locals;
  get_context (context, globals, locals, 0);
  return create_inspector (locals.get (), true /*symbolic*/);
}

void
PythonInterpreter::define_variable (const std::string &name, const tl::Variant &value)
{
  PythonPtr main_module (PyImport_AddModule ("__main__"));
  PythonPtr dict (PyModule_GetDict (main_module.get ()));
  if (dict) {
    PythonRef v (c2python (value));
    PyDict_SetItemString (dict.get (), name.c_str (), v.get ());
  }
}

bool
PythonInterpreter::available () const
{
  return true;
}

void
PythonInterpreter::initialize ()
{
  // .. no implementation required ..
}

size_t
PythonInterpreter::prepare_trace (PyObject *fn_object)
{
  std::map<PyObject *, size_t>::const_iterator f = m_file_id_map.find (fn_object);
  if (f == m_file_id_map.end ()) {
    f = m_file_id_map.insert (std::make_pair (fn_object, mp_current_exec_handler->id_for_path (this, normalize_path (python2c<std::string> (fn_object))))).first;
  }

  return f->second;
}

//  TODO: make the Python object the interpreter and don't use singleton instances (multi-threading support)
static
int pya_trace_func (PyObject * /*obj*/, PyFrameObject *frame, int event, PyObject *arg)
{
  if (PythonInterpreter::instance ()) {
    return PythonInterpreter::instance ()->trace_func (frame, event, arg);
  } else {
    return 0;
  }
}

int
PythonInterpreter::trace_func (PyFrameObject *frame, int event, PyObject *arg)
{
  if (! mp_current_exec_handler || m_in_trace) {
    return 0;
  }

  PYA_TRY

    mp_current_frame = frame;
    m_in_trace = true;

    if (event == PyTrace_LINE) {

      //  see below for a description of m_block_exceptions
      m_block_exceptions = false;

#if PY_VERSION_HEX >= 0x030B0000
      int line = PyFrame_GetLineNumber(frame);
      size_t file_id = prepare_trace (PyFrame_GetCode(frame)->co_filename);
#else
      int line = frame->f_lineno;
      size_t file_id = prepare_trace (frame->f_code->co_filename);
#endif

      PythonStackTraceProvider st_provider (frame, m_debugger_scope);
      mp_current_exec_handler->trace (this, file_id, line, &st_provider);

    } else if (event == PyTrace_CALL) {

      mp_current_exec_handler->push_call_stack (this);

    } else if (event == PyTrace_RETURN) {

      mp_current_exec_handler->pop_call_stack (this);

    } else if (event == PyTrace_EXCEPTION && ! m_block_exceptions) {

      PythonPtr exc_type, exc_value;

      if (PyTuple_Check (arg) && PyTuple_Size (arg) == 3) {
        exc_type = PythonPtr (PyTuple_GetItem (arg, 0));
        exc_value = PythonPtr (PyTuple_GetItem (arg, 1));
      }

      if (exc_type && exc_type.get () != PyExc_StopIteration) {

        //  If the next exception shall be ignored, do so
        if (m_ignore_next_exception) {

          m_ignore_next_exception = false;

        } else {

#if PY_VERSION_HEX >= 0x030B0000
          int line = PyFrame_GetLineNumber(frame);
          size_t file_id = prepare_trace (PyFrame_GetCode(frame)->co_filename);
#else
          int line = frame->f_lineno;
          size_t file_id = prepare_trace (frame->f_code->co_filename);
#endif

          std::string emsg = "<unknown>";
          if (exc_value) {
            PythonRef msg_str (PyObject_Str (exc_value.get ()));
            if (msg_str && test_type<std::string> (msg_str.get (), true)) {
              emsg = python2c<std::string> (msg_str.get ());
            }
          }

          std::string eclass = "<unknown>";
          if (exc_type) {
            const char *c = ((PyTypeObject *) exc_type.get ())->tp_name;
            if (c) {
              eclass = c;
            }
          }

          PythonStackTraceProvider st_provider (frame, m_debugger_scope);
          mp_current_exec_handler->exception_thrown (this, file_id, line, eclass, emsg, &st_provider);

        }

        //  TODO: really needed?
        //  Ruby tends to call this callback twice - once from rb_f_raise and then
        //  from rb_exc_raise. We use the m_block_exceptions flag to suppress the
        //  second one
        m_block_exceptions = true;

      }

    }

    mp_current_frame = 0;
    m_in_trace = false;
    return 0;

  PYA_CATCH("trace function")

  m_in_trace = false;
  return -1;
}

void
PythonInterpreter::push_exec_handler (gsi::ExecutionHandler *exec_handler)
{
  if (mp_current_exec_handler) {
    m_exec_handlers.push_back (mp_current_exec_handler);
  } else {
    PyEval_SetTrace (pya_trace_func, NULL);
  }

  mp_current_exec_handler = exec_handler;
  m_file_id_map.clear ();

  //  if we happen to push the exec handler inside the execution,
  //  signal start of execution
  if (m_current_exec_level > 0) {
    mp_current_exec_handler->start_exec (this);
  }
}

void
PythonInterpreter::remove_exec_handler (gsi::ExecutionHandler *exec_handler)
{
  if (mp_current_exec_handler == exec_handler) {

    //  if we happen to remove the exec handler inside the execution,
    //  signal end of execution
    if (m_current_exec_level > 0) {
      mp_current_exec_handler->end_exec (this);
    }

    if (m_exec_handlers.empty ()) {
      mp_current_exec_handler = 0;
      PyEval_SetProfile (NULL, NULL);
    } else {
      mp_current_exec_handler = m_exec_handlers.back ();
      m_exec_handlers.pop_back ();
    }

  } else {

    for (std::vector<gsi::ExecutionHandler *>::iterator eh = m_exec_handlers.begin (); eh != m_exec_handlers.end (); ++eh) {
      if (*eh == exec_handler) {
        m_exec_handlers.erase (eh);
        break;
      }
    }

  }
}

void
PythonInterpreter::push_console (gsi::Console *console)
{
  if (! mp_current_console) {

    PythonPtr current_stdout (PySys_GetObject ((char *) "stdout"));
    std::swap (current_stdout, m_stdout);
    if (current_stdout) {
      PySys_SetObject ((char *) "stdout", current_stdout.get ());
    }

    PythonPtr current_stderr (PySys_GetObject ((char *) "stderr"));
    std::swap (current_stderr, m_stderr);
    if (current_stderr) {
      PySys_SetObject ((char *) "stderr", current_stderr.get ());
    }

  } else {
    m_consoles.push_back (mp_current_console);
  }

  mp_current_console = console;
}

void
PythonInterpreter::remove_console (gsi::Console *console)
{
  if (mp_current_console == console) {

    if (m_consoles.empty ()) {

      mp_current_console = 0;

      PythonPtr current_stdout (PySys_GetObject ((char *) "stdout"));
      std::swap (current_stdout, m_stdout);
      if (current_stdout) {
        PySys_SetObject ((char *) "stdout", current_stdout.get ());
      }

      PythonPtr current_stderr (PySys_GetObject ((char *) "stderr"));
      std::swap (current_stderr, m_stderr);
      if (current_stderr) {
        PySys_SetObject ((char *) "stderr", current_stderr.get ());
      }

    } else {
      mp_current_console = m_consoles.back ();
      m_consoles.pop_back ();
    }

  } else {

    for (std::vector<gsi::Console *>::iterator c = m_consoles.begin (); c != m_consoles.end (); ++c) {
      if (*c == console) {
        m_consoles.erase (c);
        break;
      }
    }

  }
}

std::string
PythonInterpreter::version () const
{
  PyObject *version = PySys_GetObject ((char *) "version");
  if (version != NULL) {
    return python2c<std::string> (version);
  } else {
    return std::string ();
  }
}

gsi::Console *PythonInterpreter::current_console () const
{
  return mp_current_console;
}

void PythonInterpreter::begin_execution ()
{
  m_block_exceptions = false;
  if (m_current_exec_level++ == 0) {
    m_file_id_map.clear ();
    if (mp_current_exec_handler) {
      mp_current_exec_handler->start_exec (this);
    }
  }
}

void PythonInterpreter::end_execution ()
{
  if (m_current_exec_level > 0 && --m_current_exec_level == 0 && mp_current_exec_handler) {
    mp_current_exec_handler->end_exec (this);
  }
}

std::string
PythonInterpreter::python_doc (const gsi::MethodBase *m)
{
  return pya::PythonModule::python_doc (m);
}

PythonInterpreter *PythonInterpreter::instance ()
{
  return sp_interpreter;
}

}

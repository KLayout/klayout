
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


#include <Python.h>
#include <frameobject.h>   //  Python - for traceback

#include "tlString.h"

#include "pyaUtils.h"
#include "pyaConvert.h"
#include "pya.h"

#include <string>
#include <vector>
#include <algorithm>

namespace pya
{

/**
 *  @brief Turn Python errors into C++ exceptions
 */
void check_error ()
{
  PyObject *py_exc_type = NULL, *py_exc_value = NULL, *py_exc_traceback = NULL;
  PyErr_Fetch (&py_exc_type, &py_exc_value, &py_exc_traceback);
  if (py_exc_type != NULL) {

    PyErr_NormalizeException (&py_exc_type, &py_exc_value, &py_exc_traceback);

    PythonRef exc_type (py_exc_type);
    PythonRef exc_value (py_exc_value);
    PythonRef exc_traceback (py_exc_traceback);

    std::string exc_cls ("unknown");
    const char *c = ((PyTypeObject *) exc_type.get ())->tp_name;
    if (c) {
      exc_cls = c;
    }

    //  fetch traceback
    //  TODO: really decref the stack trace? how about the other objects in the stack trace?
    std::vector <tl::BacktraceElement> backtrace;
    if (exc_traceback) {
      PyTracebackObject *traceback = (PyTracebackObject*) exc_traceback.get ();
      for (PyTracebackObject *t = traceback; t; t = t->tb_next) {
        int lineno = t->tb_lineno;
#if PY_VERSION_HEX >= 0x030B0000
        //  since version 3.11.7, lineno may be -1 and indicates that the frame has to be inspected
        if (lineno < 0) {
          lineno = PyFrame_GetLineNumber(t->tb_frame);
        }
        backtrace.push_back (tl::BacktraceElement (python2c<std::string> (PyFrame_GetCode(t->tb_frame)->co_filename), lineno));
#else
        backtrace.push_back (tl::BacktraceElement (python2c<std::string> (t->tb_frame->f_code->co_filename), lineno));
#endif
      }
      std::reverse (backtrace.begin (), backtrace.end ());
    }

    if (PyErr_GivenExceptionMatches (exc_type.get (), PyExc_SyntaxError) && PyTuple_Check (exc_value.get ()) && PyTuple_Size (exc_value.get ()) >= 2) {

      const char *sourcefile = 0;
      std::string sourcefile_arg;
      int line = 0;
      std::string msg = "syntax error (could not parse exception)";

      try {

        if (exc_value && PyTuple_Check (exc_value.get ()) && PyTuple_Size (exc_value.get ()) >= 2) {

          std::string msg_arg = python2c<std::string> (PyTuple_GetItem (exc_value.get (), 0));

          PyObject *args = PyTuple_GetItem (exc_value.get (), 1);
          if (PyTuple_Check (args) && PyTuple_Size (args) >= 3) {
            sourcefile_arg = python2c<std::string> (PyTuple_GetItem (args, 0));
            sourcefile = sourcefile_arg.c_str ();
            line = python2c<int> (PyTuple_GetItem (args, 1));
            //  Not used: column_arg = python2c<int> (PyTuple_GetItem (args, 2);
          }

          //  build a Ruby-like message
          msg = sourcefile_arg;
          msg += ":";
          msg += tl::to_string (line);
          msg += ": ";
          msg += msg_arg;

        }

      } catch (...) {
        //  ignore exceptions here
      }

      if (! backtrace.empty () && ! sourcefile) {
        sourcefile = backtrace.front ().file.c_str ();
        line = backtrace.front ().line;
      }

      throw PythonError (msg.empty () ? exc_cls.c_str () : msg.c_str (), sourcefile ? sourcefile : "unknown", line, exc_cls.c_str (), backtrace);

    } else if (PyErr_GivenExceptionMatches (exc_type.get (), PyExc_SystemExit)) {

      int status = 0;
      if (exc_value) {
        tl::Variant st = python2c<tl::Variant> (exc_value.get ());
        if (st.can_convert_to_int ()) {
          status = st.to_int ();
        }
      }

      throw tl::ExitException (status);

    } else {

      std::string msg;
      if (exc_value) {
        PythonRef msg_str (PyObject_Str (exc_value.get ()));
        if (msg_str && test_type<std::string> (msg_str.get (), true)) {
          msg = python2c<std::string> (msg_str.get ());
        }
      }

      const char *sourcefile = 0;
      int line = 0;

      if (! backtrace.empty ()) {
        sourcefile = backtrace.front ().file.c_str ();
        line = backtrace.front ().line;
      }

      throw PythonError (msg.empty () ? exc_cls.c_str () : msg.c_str (), sourcefile ? sourcefile : "unknown", line, exc_cls.c_str (), backtrace);

    }

  }
}

}


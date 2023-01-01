
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

#include "pyaSignalHandler.h"
#include "pya.h"
#include "pyaMarshal.h"
#include "pyaConvert.h"
#include "pyaUtils.h"

namespace pya
{

// --------------------------------------------------------------------------
//  Implementation of CallbackFunction

CallbackFunction::CallbackFunction (PythonRef pym, const gsi::MethodBase *m)
  : mp_method (m)
{
  //  We have a problem here with cyclic references. Bound instances methods can
  //  create reference cycles if their target objects somehow points back to us
  //  (or worse, to some parent of us, i.e. inside a QWidget hierarchy).
  //  A solution is to take a bound instance method apart and store a weak
  //  reference to self plus a real reference to the function.

  if (pym && PyMethod_Check (pym.get ()) && PyMethod_Self (pym.get ()) != NULL) {

    m_weak_self = PythonRef (PyWeakref_NewRef (PyMethod_Self (pym.get ()), NULL));
    m_callable = PythonRef (PyMethod_Function (pym.get ()), false /* borrowed ref */);
#if PY_MAJOR_VERSION < 3
    m_class = PythonRef (PyMethod_Class (pym.get ()), false /* borrowed ref */);
#endif

  } else {
    m_callable = pym;
  }
}

const gsi::MethodBase *CallbackFunction::method () const
{
  return mp_method;
}

PythonRef CallbackFunction::callable () const
{
  if (m_callable && m_weak_self) {

    PyObject *self = PyWeakref_GetObject (m_weak_self.get ());
    if (self == Py_None) {
      //  object expired - no callback possible
      return PythonRef ();
    }

#if PY_MAJOR_VERSION < 3
    return PythonRef (PyMethod_New (m_callable.get (), self, m_class.get ()));
#else
    return PythonRef (PyMethod_New (m_callable.get (), self));
#endif

  } else {
    return m_callable;
  }
}

bool CallbackFunction::is_instance_method () const
{
  return m_callable && m_weak_self;
}

PyObject *CallbackFunction::self_ref () const
{
  return PyWeakref_GetObject (m_weak_self.get ());
}

PyObject *CallbackFunction::callable_ref () const
{
  return m_callable.get ();
}

bool CallbackFunction::operator== (const CallbackFunction &other) const
{
  if (is_instance_method () != other.is_instance_method ()) {
    return false;
  }
  if (m_weak_self) {
    if (self_ref () != other.self_ref ()) {
      return false;
    }
  }
  return callable_ref () == other.callable_ref ();
}

// --------------------------------------------------------------------------
//  Implementation of SignalHandler

SignalHandler::SignalHandler ()
{
  //  .. nothing yet ..
}

SignalHandler::~SignalHandler ()
{
  clear ();
}

void SignalHandler::call (const gsi::MethodBase *meth, gsi::SerialArgs &args, gsi::SerialArgs &ret) const
{
  PYTHON_BEGIN_EXEC

    tl::Heap heap;

    int args_avail = int (std::distance (meth->begin_arguments (), meth->end_arguments ()));
    PythonRef argv (PyTuple_New (args_avail));
    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); args && a != meth->end_arguments (); ++a) {
      PyTuple_SetItem (argv.get (), int (a - meth->begin_arguments ()), pop_arg (*a, args, 0, heap).release ());
    }

    //  NOTE: in case one event handler deletes the object, it's safer to first collect the handlers and
    //  then call them.
    std::vector<PythonRef> callables;
    callables.reserve (m_cbfuncs.size ());
    for (std::vector<CallbackFunction>::const_iterator c = m_cbfuncs.begin (); c != m_cbfuncs.end (); ++c) {
      PythonRef callable = c->callable ();
      if (callable) {
        callables.push_back (c->callable ());
      }
    }

    PythonRef result;

    for (std::vector<PythonRef>::const_iterator c = callables.begin (); c != callables.end (); ++c) {

      //  determine the number of arguments required
      int arg_count = args_avail;
      if (args_avail > 0) {

        PythonRef fc (PyObject_GetAttrString (c->get (), "__code__"));
        if (fc) {
          PythonRef ac (PyObject_GetAttrString (fc.get (), "co_argcount"));
          if (ac) {
            arg_count = python2c<int> (ac.get ());
            if (PyObject_HasAttrString (c->get (), "__self__")) {
              arg_count -= 1;
            }
          }
        }

      }

      //  use less arguments if applicable
      if (arg_count == 0) {
        result = PythonRef (PyObject_CallObject (c->get (), NULL));
      } else if (arg_count < args_avail) {
        PythonRef argv_less (PyTuple_GetSlice (argv.get (), 0, arg_count));
        result = PythonRef (PyObject_CallObject (c->get (), argv_less.get ()));
      } else {
        result = PythonRef (PyObject_CallObject (c->get (), argv.get ()));
      }

      if (! result) {
        check_error ();
      }

    }

    push_arg (meth->ret_type (), ret, result.get (), heap);

    //  a Python callback must not leave temporary objects
    tl_assert (heap.empty ());

  PYTHON_END_EXEC
}

void SignalHandler::add (PyObject *callable)
{
  remove (callable);
  m_cbfuncs.push_back (CallbackFunction (PythonPtr (callable), 0));
}

void SignalHandler::remove (PyObject *callable)
{
  //  To avoid cyclic references, the CallbackFunction holder is employed. However, the
  //  "true" callable no longer is the original one. Hence, we need to do a strict compare
  //  against the effective one.
  CallbackFunction cbref (PythonPtr (callable), 0);
  for (std::vector<CallbackFunction>::iterator c = m_cbfuncs.begin (); c != m_cbfuncs.end (); ++c) {
    if (*c == cbref) {
      m_cbfuncs.erase (c);
      break;
    }
  }
}

void SignalHandler::clear ()
{
  m_cbfuncs.clear ();
}

void SignalHandler::assign (const SignalHandler *other)
{
  m_cbfuncs = other->m_cbfuncs;
}

}

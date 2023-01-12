
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


#ifndef _HDR_pyaSignalHandler
#define _HDR_pyaSignalHandler

#include <Python.h>

#include "pyaRefs.h"

#include "gsiSignals.h"

namespace pya
{

/**
 *  @brief A storage object for a function to callback
 */
struct CallbackFunction
{
  CallbackFunction (PythonRef pym, const gsi::MethodBase *m);

  PythonRef callable () const;
  const gsi::MethodBase *method () const;
  bool operator== (const CallbackFunction &other) const;

private:
  PythonRef m_callable;
  PythonRef m_weak_self;
  PythonRef m_class;
  const gsi::MethodBase *mp_method;

  PyObject *self_ref () const;
  PyObject *callable_ref () const;
  bool is_instance_method () const;
};

/**
 *  @brief The signal handler abstraction
 *
 *  This class implements the signal handler that interfaces to GSI's signal system
 */
class SignalHandler
  : public gsi::SignalHandler
{
public:
  /**
   *  @brief Constructor
   */
  SignalHandler ();

  /**
   *  @brief Destructor
   */
  ~SignalHandler ();

  /**
   *  @brief Implementation of the callback interface
   */
  virtual void call (const gsi::MethodBase *method, gsi::SerialArgs &args, gsi::SerialArgs &ret) const;

  /**
   *  @brief Adds a callable to the list of targets
   */
  void add (PyObject *callable);

  /**
   *  @brief Removes a callable from the list of targets
   */
  void remove (PyObject *callable);

  /**
   *  @brief Clears the list of callables
   */
  void clear ();

  /**
   *  @brief Assign another handler to this
   */
  void assign (const SignalHandler *other);

private:
  std::vector<CallbackFunction> m_cbfuncs;
};

}

#endif

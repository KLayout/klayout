
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "pyaRefs.h"

namespace pya
{

// --------------------------------------------------------------------------
//  PythonRef implementation

PythonRef::PythonRef ()
  : mp_obj (NULL), m_owns_pointer (true)
{ 
  //  .. nothing yet ..
}

PythonRef::PythonRef (const PythonPtr &ptr)
  : mp_obj (ptr.get ()), m_owns_pointer (true)
{ 
  Py_XINCREF (mp_obj);
}

PythonRef::PythonRef (PyObject *obj, bool new_ref)
  : mp_obj (obj), m_owns_pointer (true)
{ 
  if (! new_ref) {
    Py_XINCREF (mp_obj);
  }
}

PythonRef &PythonRef::operator= (PyObject *obj)
{
  if (m_owns_pointer) {
    Py_XDECREF (mp_obj);
  }
  mp_obj = obj;
  m_owns_pointer = true;
  return *this;
}

PythonRef &PythonRef::operator= (const PythonPtr &ptr)
{
  if (m_owns_pointer) {
    Py_XDECREF (mp_obj);
  }
  mp_obj = ptr.get ();
  Py_XINCREF (mp_obj);
  m_owns_pointer = true;
  return *this;
}

PythonRef &PythonRef::operator= (const PythonRef &other)
{
  if (this != &other && mp_obj != other.mp_obj) {
    if (m_owns_pointer) {
      Py_XDECREF (mp_obj);
    }
    mp_obj = other.mp_obj;
    m_owns_pointer = true;
    Py_XINCREF (mp_obj);
  }
  return *this;
}

PythonRef::PythonRef (const PythonRef &other)
  : mp_obj (other.mp_obj), m_owns_pointer (true)
{
  Py_XINCREF (mp_obj);
}

PythonRef::~PythonRef ()
{
  if (m_owns_pointer) {
    Py_XDECREF (mp_obj);
  }
}

PythonRef::operator bool () const
{
  return mp_obj != NULL;
}

PyObject *PythonRef::operator-> () const
{
  return mp_obj;
}

PyObject *PythonRef::get () const
{
  return mp_obj;
}

PyObject *PythonRef::release ()
{
  PyObject *o = mp_obj;
  mp_obj = NULL;
  return o;
}

PyObject *PythonRef::release_const () const
{
  m_owns_pointer = false;
  return mp_obj;
}

// --------------------------------------------------------------------------
//  PythonPtr implementation

PythonPtr::PythonPtr ()
  : mp_obj (NULL)
{
  //  .. nothing yet ..
}

PythonPtr::PythonPtr (PyObject *obj)
  : mp_obj (obj)
{
  Py_XINCREF (obj);
}

PythonPtr &PythonPtr::operator= (const PythonPtr &other)
{
  if (this != &other && mp_obj != other.mp_obj) {
    Py_XDECREF (mp_obj);
    mp_obj = other.mp_obj;
    Py_XINCREF (mp_obj);
  }
  return *this;
}

PythonPtr::PythonPtr (const PythonRef &other)
  : mp_obj (other.get ())
{
  Py_XINCREF (mp_obj);
}

PythonPtr::PythonPtr (const PythonPtr &other)
  : mp_obj (other.mp_obj)
{
  Py_XINCREF (mp_obj);
}

PythonPtr::~PythonPtr ()
{
  Py_XDECREF (mp_obj);
}

PyObject *PythonPtr::release ()
{
  PyObject *obj = mp_obj;
  mp_obj = NULL;
  return obj;
}

PythonPtr::operator bool () const
{
  return mp_obj != NULL;
}

PyObject *PythonPtr::operator-> () const
{
  return mp_obj;
}

PyObject *PythonPtr::get () const
{
  return mp_obj;
}

}


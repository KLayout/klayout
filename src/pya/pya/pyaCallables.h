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

#ifndef _HDR_pyaCallables
#define _HDR_pyaCallables

#include <Python.h>

#include "pyaCommon.h"

namespace pya
{

void pya_object_deallocate (PyObject *self);
int pya_object_init (PyObject * /*self*/, PyObject *args, PyObject *kwds);
PyObject *pya_object_new (PyTypeObject *type, PyObject * /*args*/, PyObject * /*kwds*/);

PyObject *object_default_ne_impl (PyObject *self, PyObject *args);
PyObject *object_default_ge_impl (PyObject *self, PyObject *args);
PyObject *object_default_le_impl (PyObject *self, PyObject *args);
PyObject *object_default_gt_impl (PyObject *self, PyObject *args);
PyObject *object_default_deepcopy_impl (PyObject *self, PyObject *args);

typedef PyObject *(*py_func_ptr_t) (PyObject *, PyObject *);

py_func_ptr_t get_method_adaptor (int n);
py_func_ptr_t get_property_getter_adaptor (int n);
py_func_ptr_t get_property_setter_adaptor (int n);
py_func_ptr_t get_method_init_adaptor (int n);

inline void *make_closure (int mid_getter, int mid_setter)
{
  size_t g = mid_getter < 0 ? (size_t) 0 : (size_t) mid_getter;
  size_t s = mid_setter < 0 ? (size_t) 0 : (size_t) mid_setter;
  return (void *) ((s << 16) | g);
}

inline unsigned int getter_from_closure (void *closure)
{
  return (unsigned int) (size_t (closure) & 0xffff);
}

inline unsigned int setter_from_closure (void *closure)
{
  return (unsigned int) (size_t (closure) >> 16);
}

PyObject *property_getter_func (PyObject *self, void *closure);
int property_setter_func (PyObject *self, PyObject *value, void *closure);

}

#endif

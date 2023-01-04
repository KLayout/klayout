
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

#include "pyaInspector.h"
#include "pyaRefs.h"
#include "pyaUtils.h"
#include "pyaConvert.h"

#include "tlString.h"
#include "tlLog.h"

namespace pya
{

// -------------------------------------------------------------------

/**
 *  @brief A helper function for determining whether a Python object is a plain type
 *
 *  Plain types are int, string, etc.
 */
static bool is_plain_type (PyObject *obj)
{
  if (obj == NULL || obj == Py_None) {
    return true;
  }
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check (obj) || PyString_Check (obj)) {
    return true;
  }
#else
  if (PyBytes_Check (obj)) {
    return true;
  }
#endif
  if (PyBool_Check (obj) || PyFloat_Check (obj) || PyLong_Check (obj) || PyUnicode_Check (obj) || PyByteArray_Check (obj)) {
    return true;
  }

  return false;
}

/**
 *  @brief Gets the type string from an object
 */
std::string type_str (PyObject *obj)
{
  if (obj == NULL) {
    return std::string ();
  } 
  PyTypeObject *type = Py_TYPE (obj);
  if (type == NULL) {
    return std::string ();
  } 
  return type->tp_name;
}

/**
 *  @brief Gets the visibility from an object and a name
 */
gsi::Inspector::Visibility visibility_flag (PyObject *obj, PyObject *key)
{
  //  By default, classes and modules are not shown
  if (PyType_Check (obj) || PyModule_Check (obj)) {
    return gsi::Inspector::IfRequested;
  } 

  //  Callable objects are not shown
  if (PyCallable_Check (obj)) {
    return gsi::Inspector::Never;
  }

  //  Plus: items named "_..." are not shown too
  //  TODO: performance?
  if (test_type<std::string> (key, false)) {
    std::string k = python2c<std::string> (key);
    if (k.empty () || k[0] == '_') {
      return gsi::Inspector::IfRequested;
    }
  }

  return gsi::Inspector::Always;
}

/**
 *  @brief An inspector subclass delivering the elements of a Python dict
 */
class DictInspector
  : public gsi::Inspector
{
public:
  DictInspector (PyObject *dict, bool symbolic)
    : m_dict (dict), m_symbolic (symbolic)
  {
    try {
      m_keys = PythonRef (PyDict_Keys (dict));
      if (! m_keys) {
        check_error ();
      }
      m_values = PythonRef (PyDict_Values (dict));
      if (! m_values) {
        check_error ();
      }
    } catch (tl::Exception &ex) {
      tl::warn << "DictInspector::constructor: " << ex.msg ();
    } catch (...) {
      tl::warn << "DictInspector::constructor: unspecific error";
    }
  }

  std::string description () const
  {
    return std::string ("...");
  }

  bool equiv (const gsi::Inspector *o) const
  {
    const DictInspector *other = dynamic_cast<const DictInspector *> (o);
    return other && other->m_dict.get () == m_dict.get ();
  }

  tl::Variant keyv (size_t index) const
  {
    if (m_keys && PyList_Check (m_keys.get ()) && Py_ssize_t (index) < PyList_GET_SIZE (m_keys.get ())) {
      return python2c<tl::Variant> (PyList_GET_ITEM (m_keys.get (), index));
    } else {
      return tl::Variant ();
    }
  }

  std::string key (size_t index) const
  {
    if (! m_symbolic) {
      return std::string ();
    }

    try {
      if (m_keys && PyList_Check (m_keys.get ()) && Py_ssize_t (index) < PyList_GET_SIZE (m_keys.get ())) {
        return python2c<std::string> (PyList_GET_ITEM (m_keys.get (), index));
      } else {
        return std::string ();
      }
    } catch (...) {
      return std::string ("(error)");
    }
  }
  
  tl::Variant value (size_t index) const 
  {
    if (m_values && PyList_Check (m_values.get ()) && Py_ssize_t (index) < PyList_Size (m_values.get ())) {
      return python2c<tl::Variant> (PyList_GET_ITEM (m_values.get (), index));
    } else {
      return tl::Variant ();
    }
  }

  std::string type (size_t index) const 
  {
    if (m_values && PyList_Check (m_values.get ()) && Py_ssize_t (index) < PyList_Size (m_values.get ())) {
      return type_str (PyList_GET_ITEM (m_values.get (), index));
    } else {
      return std::string ();
    }
  }

  gsi::Inspector::Visibility visibility (size_t index) const 
  {
    if (m_symbolic && m_keys && m_values && PyList_Check (m_values.get ()) && Py_ssize_t (index) < PyList_Size (m_values.get ()) && PyList_Check (m_keys.get ()) && Py_ssize_t (index) < PyList_Size (m_keys.get ())) {
      return visibility_flag (PyList_GET_ITEM (m_values.get (), index), PyList_GET_ITEM (m_keys.get (), index));
    } else {
      return gsi::Inspector::Always;
    }
  }

  size_t count () const 
  {
    if (m_keys && PyList_Check (m_keys.get ())) {
      return size_t (PyList_GET_SIZE (m_keys.get ()));
    } else {
      return 0;
    }
  }

  virtual bool has_children (size_t index) const
  {
    if (m_values && PyList_Check (m_values.get ()) && Py_ssize_t (index) < PyList_Size (m_values.get ())) {
      return !is_plain_type (PyList_GET_ITEM (m_values.get (), index));
    } else {
      return false;
    }
  }

  virtual Inspector *child_inspector (size_t index) const
  {
    if (m_values && PyList_Check (m_values.get ()) && Py_ssize_t (index) < PyList_Size (m_values.get ())) {
      return create_inspector (PyList_GET_ITEM (m_values.get (), index));
    } else {
      return 0;
    }
  }

private:
  PythonPtr m_dict;
  PythonRef m_keys, m_values;
  bool m_symbolic;
};

/**
 *  @brief An inspector subclass delivering the elements of a Python list
 */
class ListInspector
  : public gsi::Inspector
{
public:
  ListInspector (PyObject *list)
    : m_values (list)
  {
  }

  std::string description () const
  {
    return std::string ("...");
  }

  bool equiv (const gsi::Inspector *o) const
  {
    const ListInspector *other = dynamic_cast<const ListInspector *> (o);
    return other && other->m_values.get () == m_values.get ();
  }

  bool has_keys () const
  {
    return false;
  }
  
  std::string type (size_t index) const 
  {
    if (m_values && PyList_Check (m_values.get ()) && Py_ssize_t (index) < PyList_Size (m_values.get ())) {
      return type_str (PyList_GET_ITEM (m_values.get (), index));
    } else {
      return std::string ();
    }
  }

  tl::Variant value (size_t index) const 
  {
    if (m_values && PyList_Check (m_values.get ()) && Py_ssize_t (index) < PyList_Size (m_values.get ())) {
      return python2c<tl::Variant> (PyList_GET_ITEM (m_values.get (), index));
    } else {
      return tl::Variant ();
    }
  }

  gsi::Inspector::Visibility visibility (size_t /*index*/) const 
  {
    return gsi::Inspector::Always;
  }

  size_t count () const 
  {
    if (m_values && PyList_Check (m_values.get ())) {
      return size_t (PyList_GET_SIZE (m_values.get ()));
    } else {
      return 0;
    }
  }

  virtual bool has_children (size_t index) const
  {
    if (m_values && PyList_Check (m_values.get ()) && Py_ssize_t (index) < PyList_Size (m_values.get ())) {
      return !is_plain_type (PyList_GET_ITEM (m_values.get (), index));
    } else {
      return false;
    }
  }

  virtual Inspector *child_inspector (size_t index) const
  {
    if (m_values && PyList_Check (m_values.get ()) && Py_ssize_t (index) < PyList_Size (m_values.get ())) {
      return create_inspector (PyList_GET_ITEM (m_values.get (), index));
    } else {
      return 0;
    }
  }

private:
  PythonPtr m_values;
};

/**
 *  @brief An inspector subclass delivering the elements of a Python tuple
 */
class TupleInspector
  : public gsi::Inspector
{
public:
  TupleInspector (PyObject *tuple)
    : m_values (tuple)
  {
  }

  std::string description () const
  {
    return std::string ("...");
  }

  bool equiv (const gsi::Inspector *o) const
  {
    const TupleInspector *other = dynamic_cast<const TupleInspector *> (o);
    return other && other->m_values.get () == m_values.get ();
  }

  bool has_keys () const
  {
    return false;
  }

  std::string type (size_t index) const 
  {
    if (m_values && PyTuple_Check (m_values.get ()) && Py_ssize_t (index) < PyTuple_Size (m_values.get ())) {
      return type_str (PyTuple_GET_ITEM (m_values.get (), index));
    } else {
      return std::string ();
    }
  }

  tl::Variant value (size_t index) const 
  {
    if (m_values && PyTuple_Check (m_values.get ()) && Py_ssize_t (index) < PyTuple_Size (m_values.get ())) {
      return python2c<tl::Variant> (PyTuple_GET_ITEM (m_values.get (), index));
    } else {
      return tl::Variant ();
    }
  }

  gsi::Inspector::Visibility visibility (size_t /*index*/) const 
  {
    return gsi::Inspector::Always;
  }

  size_t count () const 
  {
    if (m_values && PyTuple_Check (m_values.get ())) {
      return size_t (PyTuple_GET_SIZE (m_values.get ()));
    } else {
      return 0;
    }
  }

  virtual bool has_children (size_t index) const
  {
    if (m_values && PyTuple_Check (m_values.get ()) && Py_ssize_t (index) < PyTuple_Size (m_values.get ())) {
      return !is_plain_type (PyTuple_GET_ITEM (m_values.get (), index));
    } else {
      return false;
    }
  }

  virtual Inspector *child_inspector (size_t index) const
  {
    if (m_values && PyTuple_Check (m_values.get ()) && Py_ssize_t (index) < PyTuple_Size (m_values.get ())) {
      return create_inspector (PyTuple_GET_ITEM (m_values.get (), index));
    } else {
      return 0;
    }
  }

private:
  PythonPtr m_values;
};

/**
 *  @brief An inspector subclass delivering the elements of an object
 */
class ObjectInspector
  : public gsi::Inspector
{
public:
  ObjectInspector (PyObject *obj)
    : m_obj (obj)
  {
    //  PyObject_Dir is sensitive to pending errors
    PyErr_Clear ();
    m_keys = PythonRef (PyObject_Dir (obj));
    if (! m_keys) {
      try {
        check_error ();
      } catch (tl::Exception &ex) {
        tl::warn << "ObjectInspector::constructor: " << ex.msg ();
      } catch (...) {
        tl::warn << "ObjectInspector::constructor: unspecific error";
      }
    }
  }

  std::string description () const
  {
    PythonRef rep (PyObject_Repr (m_obj.get ()));
    if (! rep) {
      check_error ();
      return std::string ("...");
    } else {
      return python2c<std::string> (rep.get ());
    }
  }

  bool equiv (const gsi::Inspector *o) const
  {
    const ObjectInspector *other = dynamic_cast<const ObjectInspector *> (o);
    return other && other->m_obj.get () == m_obj.get ();
  }

  std::string key (size_t index) const 
  {
    try {
      if (m_keys && PyList_Check (m_keys.get ()) && Py_ssize_t (index) < PyList_GET_SIZE (m_keys.get ())) {
        return python2c<std::string> (PyList_GET_ITEM (m_keys.get (), index));
      } else {
        return std::string ();
      }
    } catch (...) {
      return std::string ("(error)");
    }
  }
  
  tl::Variant value (size_t index) const 
  {
    if (m_keys && PyList_Check (m_keys.get ()) && Py_ssize_t (index) < PyList_Size (m_keys.get ())) {
      PythonRef value (PyObject_GetAttr (m_obj.get (), PyList_GET_ITEM (m_keys.get (), index)));
      if (! value) {
        check_error ();
      }
      return python2c<tl::Variant> (value.get ());
    } else {
      return tl::Variant ();
    }
  }

  std::string type (size_t index) const 
  {
    if (m_keys && PyList_Check (m_keys.get ()) && Py_ssize_t (index) < PyList_Size (m_keys.get ())) {
      PythonRef value (PyObject_GetAttr (m_obj.get (), PyList_GET_ITEM (m_keys.get (), index)));
      if (! value) {
        PyErr_Clear ();
      } else {
        return type_str (value.get ());
      }
    } 
    return std::string ();
  }

  gsi::Inspector::Visibility visibility (size_t index) const 
  {
    if (m_keys && PyList_Check (m_keys.get ()) && Py_ssize_t (index) < PyList_Size (m_keys.get ())) {
      PythonRef value (PyObject_GetAttr (m_obj.get (), PyList_GET_ITEM (m_keys.get (), index)));
      if (! value) {
        PyErr_Clear ();
      } else {
        return visibility_flag (value.get (), PyList_GET_ITEM (m_keys.get (), index));
      }
    } 
    return gsi::Inspector::Always;
  }

  size_t count () const 
  {
    if (m_keys && PyList_Check (m_keys.get ())) {
      return size_t (PyList_GET_SIZE (m_keys.get ()));
    } else {
      return 0;
    }
  }

  virtual bool has_children (size_t index) const
  {
    if (m_keys && PyList_Check (m_keys.get ()) && Py_ssize_t (index) < PyList_Size (m_keys.get ())) {
      PythonRef value (PyObject_GetAttr (m_obj.get (), PyList_GET_ITEM (m_keys.get (), index)));
      if (! value) {
        PyErr_Clear ();
      }
      return !is_plain_type (value.get ());
    } else {
      return false;
    }
  }

  virtual Inspector *child_inspector (size_t index) const
  {
    if (m_keys && PyList_Check (m_keys.get ()) && Py_ssize_t (index) < PyList_Size (m_keys.get ())) {
      PythonRef value (PyObject_GetAttr (m_obj.get (), PyList_GET_ITEM (m_keys.get (), index)));
      if (! value) {
        PyErr_Clear ();
      }
      return create_inspector (value.get ());
    } else {
      return 0;
    }
  }

private:
  PythonPtr m_obj;
  PythonRef m_keys;
};

gsi::Inspector *
create_inspector (PyObject *obj, bool symbolic)
{
  if (PyDict_Check (obj)) {
    return new DictInspector (obj, symbolic);
  } else if (PyList_Check (obj)) {
    return new ListInspector (obj);
  } else if (PyTuple_Check (obj)) {
    return new TupleInspector (obj);
  } else if (obj != NULL) {
    return new ObjectInspector (obj);
  } else {
    return 0;
  }
}

}


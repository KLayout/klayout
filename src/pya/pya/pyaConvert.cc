
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


#include "pyaConvert.h"
#include "pyaObject.h"
#include "pyaModule.h"
#include "pyaStatusChangedListener.h"
#include "pyaUtils.h"

#include "gsiClassBase.h"

#include <string>

namespace pya
{

bool is_derived_from (const gsi::ClassBase *cls, const std::type_info &ti)
{
  return cls->is_derived_from (gsi::class_by_typeinfo_no_assert (ti));
}

template <>
long python2c_func<long>::operator() (PyObject *rval)
{
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check (rval)) {
    return PyInt_AsLong (rval);
  } else
#endif
  if (PyLong_Check (rval)) {
    return PyLong_AsLong (rval);
  } else if (PyFloat_Check (rval)) {
    return (long) (PyFloat_AsDouble (rval));
  } else {
    throw tl::TypeError (tl::to_string (tr ("Value cannot be converted to an integer")));
  }
}

template <>
bool python2c_func<bool>::operator() (PyObject *rval)
{
  return PyObject_IsTrue (rval);
}

template <>
char python2c_func<char>::operator() (PyObject *rval)
{
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check (rval)) {
    return char (PyInt_AsLong (rval));
  } else
#endif
  if (PyLong_Check (rval)) {
    return char (PyLong_AsLong (rval));
  } else if (PyFloat_Check (rval)) {
    return char (PyFloat_AsDouble (rval));
  } else {
    throw tl::TypeError (tl::to_string (tr ("Value cannot be converted to a character")));
  }
}

template <>
unsigned long python2c_func<unsigned long>::operator() (PyObject *rval)
{
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check (rval)) {
    return PyInt_AsUnsignedLongMask (rval);
  } else
#endif
  if (PyLong_Check (rval)) {
    return PyLong_AsUnsignedLongMask (rval);
  } else if (PyFloat_Check (rval)) {
    return (unsigned long) (PyFloat_AsDouble (rval));
  } else {
    throw tl::TypeError (tl::to_string (tr ("Value cannot be converted to an integer")));
  }
}

template <>
long long python2c_func<long long>::operator() (PyObject *rval)
{
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check (rval)) {
    return PyInt_AsLong (rval);
  } else
#endif
  if (PyLong_Check (rval)) {
    return PyLong_AsLongLong (rval);
  } else if (PyFloat_Check (rval)) {
    return (long long) (PyFloat_AsDouble (rval));
  } else {
    throw tl::TypeError (tl::to_string (tr ("Value cannot be converted to an integer")));
  }
}

template <>
unsigned long long python2c_func<unsigned long long>::operator() (PyObject *rval)
{
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check (rval)) {
    return PyInt_AsUnsignedLongMask (rval);
  } else
#endif
  if (PyLong_Check (rval)) {
    return PyLong_AsUnsignedLongLongMask (rval);
  } else if (PyFloat_Check (rval)) {
    return (unsigned long long) (PyFloat_AsDouble (rval));
  } else {
    throw tl::TypeError (tl::to_string (tr ("Value cannot be converted to an integer")));
  }
}

#if defined(HAVE_64BIT_COORD)
template <>
__int128 python2c_func<__int128>::operator() (PyObject *rval)
{
  // TODO: this is pretty simplistic
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check (rval)) {
    return PyInt_AsLong (rval);
  } else
#endif
  if (PyLong_Check (rval)) {
    return PyLong_AsLongLong (rval);
  } else if (PyFloat_Check (rval)) {
    return PyFloat_AsDouble (rval);
  } else {
    throw tl::TypeError (tl::to_string (tr ("Value cannot be converted to an integer")));
  }
}
#endif

template <>
double python2c_func<double>::operator() (PyObject *rval)
{
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check (rval)) {
    return PyInt_AsLong (rval);
  } else
#endif
  if (PyLong_Check (rval)) {
    return PyLong_AsLongLong (rval);
  } else if (PyFloat_Check (rval)) {
    return PyFloat_AsDouble (rval);
  } else {
    throw tl::TypeError (tl::to_string (tr ("Value cannot be converted to a floating-point value")));
  }
}

template <>
std::string python2c_func<std::string>::operator() (PyObject *rval)
{
#if PY_MAJOR_VERSION < 3
  if (PyString_Check (rval)) {
    return std::string (PyString_AsString (rval), PyString_Size (rval));
  } else
#else
  if (PyBytes_Check (rval)) {
    return std::string (PyBytes_AsString (rval), PyBytes_Size (rval));
  } else
#endif
  if (PyUnicode_Check (rval)) {
    PythonRef ba (PyUnicode_AsUTF8String (rval));
    if (! ba) {
      check_error ();
    }
    return std::string (PyBytes_AsString (ba.get ()), PyBytes_Size (ba.get ()));
  } else if (PyByteArray_Check (rval)) {
    return std::string (PyByteArray_AsString (rval), PyByteArray_Size (rval));
  } else {
    throw tl::TypeError (tl::to_string (tr ("Value cannot be converted to a string")));
  }
}

template <>
std::vector<char> python2c_func<std::vector<char> >::operator() (PyObject *rval)
{
#if PY_MAJOR_VERSION < 3
  if (PyString_Check (rval)) {
    const char *cp = PyString_AsString (rval);
    return std::vector<char> (cp, cp + PyString_Size (rval));
  } else
#else
  if (PyBytes_Check (rval)) {
    char *cp = 0;
    Py_ssize_t sz = 0;
    PyBytes_AsStringAndSize (rval, &cp, &sz);
    tl_assert (cp != 0);
    return std::vector<char> (cp, cp + sz);
  } else
#endif
  if (PyUnicode_Check (rval)) {
    PythonRef ba (PyUnicode_AsUTF8String (rval));
    if (! ba) {
      check_error ();
    }
    char *cp = 0;
    Py_ssize_t sz = 0;
    PyBytes_AsStringAndSize (ba.get (), &cp, &sz);
    tl_assert (cp != 0);
    return std::vector<char> (cp, cp + sz);
  } else if (PyByteArray_Check (rval)) {
    char *cp = PyByteArray_AsString (rval);
    Py_ssize_t sz = PyByteArray_Size (rval);
    return std::vector<char> (cp, cp + sz);
  } else {
    throw tl::TypeError (tl::to_string (tr ("Value cannot be converted to a byte array")));
  }
}

#if defined(HAVE_QT)
template <>
QByteArray python2c_func<QByteArray>::operator() (PyObject *rval)
{
#if PY_MAJOR_VERSION < 3
  if (PyString_Check (rval)) {
    return QByteArray (PyString_AsString (rval), PyString_Size (rval));
  } else
#else
  if (PyBytes_Check (rval)) {
    return QByteArray (PyBytes_AsString (rval), PyBytes_Size (rval));
  } else
#endif
  if (PyUnicode_Check (rval)) {
    PythonRef ba (PyUnicode_AsUTF8String (rval));
    if (! ba) {
      check_error ();
    }
    return QByteArray (PyBytes_AsString (ba.get ()), PyBytes_Size (ba.get ()));
  } else if (PyByteArray_Check (rval)) {
    return QByteArray (PyByteArray_AsString (rval), PyByteArray_Size (rval));
  } else {
    throw tl::TypeError (tl::to_string (tr ("Value cannot be converted to a byte array")));
  }
}

template <>
QString python2c_func<QString>::operator() (PyObject *rval)
{
  //  TODO: directly convert Unicode strings to QString if possible
  return tl::to_qstring (python2c<std::string> (rval));
}
#endif

template <>
tl::Variant python2c_func<tl::Variant>::operator() (PyObject *rval)
{
  if (rval == NULL || rval == Py_None) {
    return tl::Variant ();
  } else if (PyBool_Check (rval)) {
    return tl::Variant (python2c<bool> (rval));
  } else if (PyLong_Check (rval)) {
    return tl::Variant (python2c<long long> (rval));
#if PY_MAJOR_VERSION < 3
  } else if (PyInt_Check (rval)) {
    return tl::Variant (python2c<int> (rval));
#endif
  } else if (PyFloat_Check (rval)) {
    return tl::Variant (python2c<double> (rval));
#if PY_MAJOR_VERSION < 3
  } else if (PyString_Check (rval)) {
    return tl::Variant (python2c<std::string> (rval));
#else
  } else if (PyBytes_Check (rval)) {
    return tl::Variant (python2c<std::vector<char> > (rval));
#endif
  } else if (PyUnicode_Check (rval)) {
    return tl::Variant (python2c<std::string> (rval));
  } else if (PyByteArray_Check (rval)) {
    return tl::Variant (python2c<std::vector<char> > (rval));
  } else if (PyList_Check (rval)) {

    size_t len = PyList_Size (rval);

    static std::vector<tl::Variant> empty;
    tl::Variant r (empty.begin (), empty.end ());
    r.get_list ().reserve (len);
    for (size_t i = 0; i < len; ++i) {
      r.get_list ().push_back (python2c<tl::Variant> (PyList_GetItem (rval, i)));
    }
    return r;

  } else if (PyTuple_Check (rval)) {

    size_t len = PyTuple_Size (rval);

    static std::vector<tl::Variant> empty;
    tl::Variant r (empty.begin (), empty.end ());
    r.get_list ().reserve (len);
    for (size_t i = 0; i < len; ++i) {
      r.get_list ().push_back (python2c<tl::Variant> (PyTuple_GetItem (rval, i)));
    }
    return r;

  } else if (PyDict_Check (rval)) {

    tl::Variant r;
    r.set_array ();
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_Next (rval, &pos, &key, &value)) {
      r.insert (python2c<tl::Variant> (key), python2c<tl::Variant> (value));
    }

    return r;

  } else {

    const gsi::ClassBase *cls = PythonModule::cls_for_type (Py_TYPE (rval));
    if (cls) {

      PYAObjectBase *p = PYAObjectBase::from_pyobject (rval);

      //  employ the tl::Variant binding capabilities of the Expression binding to derive the
      //  variant value.

      void *obj = p->obj ();
      if (! obj) {
        return tl::Variant ();
      }

      if (cls->is_managed ()) {

        const tl::VariantUserClassBase *var_cls = cls->var_cls (p->const_ref ());
        tl_assert (var_cls != 0);

        gsi::Proxy *gsi_proxy = cls->gsi_object (obj)->find_client<gsi::Proxy> ();
        if (!gsi_proxy) {
          //  establish a new proxy
          gsi_proxy = new gsi::Proxy (cls);
          gsi_proxy->set (obj, false, p->const_ref (), false);
        }

        tl::Variant out;
        out.set_user_ref (gsi_proxy, var_cls, false);
        return out;

      } else {
        //  No reference management available: deep copy mode.
        return tl::Variant (cls->clone (obj), cls->var_cls (false), true);
      }

    } else {

      //  last resort: convert to string
      std::string m ("<unknown type>");
      PythonRef msg_str (PyObject_Str (rval));
      if (msg_str) {
        m = python2c<std::string> (msg_str.get ());
      }
      return tl::Variant (m);

    }

  }
}

PyObject *
object_to_python (void *obj, PYAObjectBase *self, const gsi::ArgType &atype)
{
  const gsi::ClassBase *cls = atype.cls()->subclass_decl (obj);

  bool is_direct   = !(atype.is_ptr () || atype.is_ref () || atype.is_cptr () || atype.is_cref ());
  bool pass_obj    = atype.pass_obj () || is_direct;
  bool is_const    = atype.is_cptr () || atype.is_cref ();
  bool prefer_copy = atype.prefer_copy ();
  bool can_destroy = prefer_copy || atype.is_ptr ();

  return object_to_python (obj, self, cls, pass_obj, is_const, prefer_copy, can_destroy);
}

/**
 *  @brief Correct constness if a reference is const and a non-const reference is required
 *  HINT: this is a workaround for the fact that unlike C++, Python does not have const or non-const
 *  references. Since a reference is identical with the object it points to, there are only const or non-const
 *  objects. We deliver const objects first, but if a non-const version is requested, the
 *  object turns into a non-const one. This may be confusing but provides a certain level
 *  of "constness", at least until there is another non-const reference for that object.
 */
void correct_constness (PYAObjectBase *p, bool const_required)
{
  if (p->const_ref () && ! const_required) {
    //  promote to non-const object
    p->set_const_ref (false);
  }
}

PyObject *
object_to_python (void *obj, PYAObjectBase *self, const gsi::ClassBase *cls, bool pass_obj, bool is_const, bool prefer_copy, bool can_destroy)
{
  if (! obj || ! cls) {
    Py_RETURN_NONE;
  }

  const gsi::ClassBase *clsact = cls->subclass_decl (obj);
  if (! clsact) {
    Py_RETURN_NONE;
  }

  PYAObjectBase *pya_object = 0;

  if (self && self->obj () == obj) {

    //  reuse self if the object to be converted is self
    pya_object = self;

  } else if (! clsact->adapted_type_info () && clsact->is_managed ()) {

    StatusChangedListener *client = clsact->gsi_object (obj)->find_client<StatusChangedListener> ();
    if (client) {
      pya_object = client->pya_object();
    }

  } else if (clsact->adapted_type_info ()) {

    //  create an adaptor from an adapted type
    if (pass_obj) {
      obj = clsact->create_from_adapted_consume (obj);
    } else {
      obj = clsact->create_from_adapted (obj);
    }

    //  we wil own the new object
    pass_obj = true;

  }

  if (! pass_obj && prefer_copy && ! clsact->adapted_type_info () && ! clsact->is_managed () && clsact->can_copy () && clsact->can_default_create ()) {

    //  We copy objects passed by const reference if they are not managed
    //  (derived from gsi::ObjectBase, in that case, client_data is not 0).
    //  Such objects are often exposed internals. First we can't
    //  guarantee the const correctness of references. Second, we
    //  can't guarantee the lifetime of the container will exceed that
    //  of the exposed property. Hence copying is safer.

    PyTypeObject *type = PythonModule::type_for_cls (clsact);
    if (!type) {
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("Requested type %s.%s is not bound to a Python class (did you load the '%s' module?)")), clsact->module (), clsact->name (), clsact->module ()));
    }

    //  create a instance and copy the value
    PyObject *new_pyobject = type->tp_alloc (type, 0);
    PYAObjectBase *new_object = PYAObjectBase::from_pyobject_unsafe (new_pyobject);
    new (new_object) PYAObjectBase (clsact, new_pyobject);
    clsact->assign (new_object->obj (), obj);
    return new_pyobject;

  } else if (pya_object) {

    //  we have a that is located in C++ space but is supposed to get attached
    //  a Python object. If it already has, we simply return a reference to this
    Py_INCREF (pya_object->py_object ());

    correct_constness (pya_object, is_const);

    return pya_object->py_object ();

  } else {

    PyTypeObject *type = PythonModule::type_for_cls (clsact);
    if (!type) {
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("Requested type %s.%s is not bound to a Python class (did you load the '%s' module?)")), clsact->module (), clsact->name (), clsact->module ()));
    }

    //  create a instance and copy the value
    PyObject *new_pyobject = type->tp_alloc (type, 0);
    PYAObjectBase *new_object = PYAObjectBase::from_pyobject_unsafe (new_pyobject);
    new (new_object) PYAObjectBase (clsact, new_pyobject);
    new_object->set (obj, pass_obj, is_const, can_destroy);
    return new_pyobject;

  }
}

template <>
PyObject *c2python_func<const tl::Variant &>::operator() (const tl::Variant &c)
{
  if (c.is_double ()) {
    return c2python (c.to_double ());
  } else if (c.is_bool ()) {
    return c2python (c.to_bool ());
  } else if (c.is_a_string ()) {
    return c2python (c.to_string ());
  } else if (c.is_a_bytearray ()) {
    return c2python (c.to_bytearray ());
  } else if (c.is_long ()) {
    return c2python (c.to_long ());
  } else if (c.is_ulong ()) {
    return c2python (c.to_ulong ());
  } else if (c.is_longlong ()) {
    return c2python (c.to_longlong ());
  } else if (c.is_ulonglong ()) {
    return c2python (c.to_ulonglong ());
  } else if (c.is_array ()) {

    PyObject *ret = PyDict_New ();
    for (tl::Variant::const_array_iterator i = c.begin_array (); i != c.end_array (); ++i) {
      PyDict_SetItem (ret, c2python (i->first), c2python (i->second));
    }
    return ret;

  } else if (c.is_list ()) {

    PyObject *ret = PyList_New (c.get_list ().size ());
    size_t index = 0;
    for (tl::Variant::const_iterator i = c.begin (); i != c.end (); ++i, ++index) {
      PyList_SetItem (ret, index, c2python (*i));
    }
    return ret;

  } else if (c.is_user ()) {

    const gsi::ClassBase *cls = c.gsi_cls ();
    if (cls) {
      void *obj = const_cast<void *> (c.to_user ());
      return object_to_python (obj, 0, c.user_cls ()->gsi_cls (), false, false, true, false);
    } else {
      //  not a known type -> return nil
      Py_RETURN_NONE;
    }

  } else {
    Py_RETURN_NONE;
  }
}

template <>
PyObject *c2python_func<const std::string &>::operator() (const std::string &c)
{
#if PY_MAJOR_VERSION < 3
  return PyString_FromStringAndSize (c.c_str (), Py_ssize_t (c.size ()));
#else
  PyObject *ret = PyUnicode_DecodeUTF8 (c.c_str (), Py_ssize_t (c.size ()), NULL);
  if (ret == NULL) {
    check_error ();
  }
  return ret;
#endif
}

template <>
PyObject *c2python_func<const std::vector<char> &>::operator() (const std::vector<char> &c)
{
#if PY_MAJOR_VERSION < 3
  return PyByteArray_FromStringAndSize (&c.front (), Py_ssize_t (c.size ()));
#else
  return PyBytes_FromStringAndSize (&c.front (), Py_ssize_t (c.size ()));
#endif
}

template <>
PyObject *c2python_func<const char *>::operator() (const char *p)
{
  const char *s = p;
  if (! s) {
    s = "(null)";
  }

#if PY_MAJOR_VERSION < 3
  return PyString_FromString (s);
#else
  PyObject *ret = PyUnicode_DecodeUTF8 (s, strlen (s), NULL);
  if (ret == NULL) {
    check_error ();
  }
  return ret;
#endif
}

#if defined(HAVE_QT)
template <>
PyObject *c2python_func<const QByteArray &>::operator() (const QByteArray &qba)
{
  if (qba.isNull ()) {
    Py_RETURN_NONE;
  } else {
#if PY_MAJOR_VERSION < 3
    return PyByteArray_FromStringAndSize (qba.constData (), Py_ssize_t (qba.size ()));
#else
    return PyBytes_FromStringAndSize (qba.constData (), Py_ssize_t (qba.size ()));
#endif
  }
}
#endif

#if defined(HAVE_QT)
template <>
PyObject *c2python_func<const QString &>::operator() (const QString &qs)
{
  if (qs.isNull ()) {
    Py_RETURN_NONE;
  } else {
    //  TODO: can be done more efficiently
    std::string c (tl::to_string (qs));
    return c2python (c);
  }
}
#endif

}


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


#ifndef _HDR_pyaConvert
#define _HDR_pyaConvert

#include <Python.h>

#include "pyaCommon.h"
#include "pyaModule.h"
#include "pyaObject.h"

#include "tlVariant.h"
#include "tlException.h"
#include "tlHeap.h"

#include <string>
#if defined(HAVE_QT)
# include <QString>
# include <QByteArray>
#endif

#include <typeinfo>

namespace gsi
{
  class ClassBase;
  class ArgType;

  GSI_PUBLIC const ClassBase *class_by_typeinfo_no_assert (const std::type_info &ti);
}

namespace pya
{

class PYAObjectBase;

//  Forward declarations to reduce the dependency on gsi
PYA_PUBLIC bool is_derived_from (const gsi::ClassBase *cls, const std::type_info &ti);

// -------------------------------------------------------------------
//  Conversion of a generic object to a Python object

/**
 *  @brief Translates an object to a Python object (PyObject)
 *  @param obj The generic object pointer
 *  @param self If there is an object where the returned object may be a member of or 0 if there isn't
 *  @param cls The class of the object
 *  @param pass_obj If true, the Python object will own the original object which gets destroyed when the Ruby object is finalized
 *  @param is_const If true, the Python object will be a const one unless the original object is already bound in a non-const way
 *  @param prefer_copy If true, the Python object will be copied unless there is a way to use shared references (for managed objects)
 *  @param can_destroy If true, the Python object can be destroyed explicitly
 *  @return The Python object
 */
PYA_PUBLIC PyObject *
object_to_python (void *obj, PYAObjectBase *self, const gsi::ClassBase *cls, bool pass_obj, bool is_const, bool prefer_copy, bool can_destroy);

/**
 *  @brief Translates an object to a Python object (PyObject)
 *  This version takes it's flags from the atype given.
 */
PYA_PUBLIC PyObject *
object_to_python (void *obj, PYAObjectBase *self, const gsi::ArgType &atype);

// -------------------------------------------------------------------
//  Type checks 

template <class T>
struct test_type_func
{
  bool operator() (PyObject * /*rval*/, bool /*loose*/)
  {
    return false;
  }
};

template <>
struct test_type_func<bool>
{
  bool operator() (PyObject *rval, bool loose)
  {
    if (loose) {
      return true;  // everything can be converted to bool
    } else {
      return PyBool_Check (rval) || rval == Py_None;
    }
  }
};

//  used for other integer types as well:
template <>
struct test_type_func<int>
{
  bool operator() (PyObject *rval, bool loose)
  {
    //  bool values don't count as int in strict mode
    if (!loose && PyBool_Check (rval)) {
      return false;
    }
#if PY_MAJOR_VERSION < 3
   return PyInt_Check (rval) || PyLong_Check (rval) || (PyFloat_Check (rval) && loose);
#else
   return PyLong_Check (rval) || (PyFloat_Check (rval) && loose);
#endif
  }
};

template <> struct test_type_func<unsigned int> : public test_type_func<int> { };
template <> struct test_type_func<char> : public test_type_func<int> { };
template <> struct test_type_func<signed char> : public test_type_func<int> { };
template <> struct test_type_func<unsigned char> : public test_type_func<int> { };
template <> struct test_type_func<short> : public test_type_func<int> { };
template <> struct test_type_func<unsigned short> : public test_type_func<int> { };
template <> struct test_type_func<long> : public test_type_func<int> { };
template <> struct test_type_func<unsigned long> : public test_type_func<int> { };
template <> struct test_type_func<long long> : public test_type_func<int> { };
template <> struct test_type_func<unsigned long long> : public test_type_func<int> { };

#if defined(HAVE_64BIT_COORD)
template <>
struct test_type_func<__int128>
{
  bool operator() (PyObject *rval, bool loose)
  {
    return test_type_func<int> () (rval, loose);
  }
};
#endif

template <>
struct test_type_func<double>
{
  bool operator() (PyObject *rval, bool loose)
  {
    //  bool values don't count as int in strict mode
    if (!loose && PyBool_Check (rval)) {
      return false;
    }
#if PY_MAJOR_VERSION < 3
    return PyFloat_Check (rval) || (PyInt_Check (rval) && loose) || (PyLong_Check (rval) && loose);
#else
    return PyFloat_Check (rval) || (PyLong_Check (rval) && loose);
#endif
  }
};

template <>
struct test_type_func<float> : public test_type_func<double> { };

template <>
struct test_type_func<void *> : public test_type_func<size_t> { };

//  used for strings in general:
template <>
struct test_type_func<const char *>
{
  bool operator() (PyObject *rval, bool /*loose*/)
  {
#if PY_MAJOR_VERSION < 3
    return PyString_Check (rval) || PyUnicode_Check (rval) || PyByteArray_Check (rval);
#else
    return PyBytes_Check (rval) || PyUnicode_Check (rval) || PyByteArray_Check (rval);
#endif
  }
};

template <> struct test_type_func<std::string> : public test_type_func<const char *> { };
template <> struct test_type_func<std::vector<char> > : public test_type_func<const char *> { };
#if defined(HAVE_QT)
template <> struct test_type_func<QString> : public test_type_func<const char *> { };
template <> struct test_type_func<QByteArray> : public test_type_func<const char *> { };
#endif

template <>
struct test_type_func<tl::Variant>
{
  bool operator() (PyObject * /*rval*/, bool /*loose*/)
  {
    return true;
  }
};

template <class T>
struct test_type_func<T &>
{
  bool operator() (PyObject *rval, bool /*loose*/)
  {
    //  TODO: we currently don't check for non-constness
    const gsi::ClassBase *cls_decl = pya::PythonModule::cls_for_type (Py_TYPE (rval));
    return cls_decl && is_derived_from (cls_decl, typeid (T));
  }
};

template <class T>
struct test_type_func<const T &>
{
  bool operator() (PyObject *rval, bool loose)
  {
    return test_type_func<T &> () (rval, loose);
  }
};

template <class T>
struct test_type_func<const T *>
{
  bool operator() (PyObject *rval, bool loose)
  {
    //  for the pointer types, None is an allowed value
    return rval == Py_None || test_type_func<T &> () (rval, loose);
  }
};

template <class T>
struct test_type_func<T *>
{
  bool operator() (PyObject *rval, bool loose)
  {
    //  for the pointer types, None is an allowed value
    return rval == Py_None || test_type_func<T &> () (rval, loose);
  }
};


/**
 *  @brief Checks whether the Python object is compatible with the given type
 *
 *  The type checks are somewhat more picky than the python2c functions.
 *  The type checks are used to resolve overridden methods, so being picky
 *  might be more appropriate.
 *
 *  @param loose If true, the type is checked more loosely. Use for second-pass matching.
 */
template <class T>
inline bool test_type (PyObject * rval, bool loose = false)
{
  return test_type_func<T> () (rval, loose);
}

/**
 *  @brief Test a PyObject *for compatibility with a vector of the given type R
 */
template <class R>
inline bool test_vector (PyObject *arr, bool loose = false)
{
  if (PyList_Check (arr)) {

    size_t len = PyList_Size (arr);
    for (size_t i = 0; i < len; ++i) {
      if (! test_type_func<R> () (PyList_GetItem (arr, i), loose)) {
        return false;
      }
    }

    return true;

  } else if (PyTuple_Check (arr)) {

    size_t len = PyTuple_Size (arr);
    for (size_t i = 0; i < len; ++i) {
      if (! test_type_func<R> () (PyTuple_GetItem (arr, i), loose)) {
        return false;
      }
    }

    return true;

  } else {
    return false;
  }
}

// -------------------------------------------------------------------
//  Python to C conversion

template <class T>
struct python2c_func
{
  T operator() (PyObject * /*rval*/)
  {
    tl_assert (false);  //  type not bound
  }
};

template <> PYA_PUBLIC long python2c_func<long>::operator() (PyObject *rval);
template <> PYA_PUBLIC unsigned long python2c_func<unsigned long>::operator() (PyObject *rval);
template <> PYA_PUBLIC bool python2c_func<bool>::operator() (PyObject *rval);
template <> PYA_PUBLIC char python2c_func<char>::operator() (PyObject *rval);

template <class D, class C>
struct python2c_func_cast
  : public python2c_func<C>
{
  D operator() (PyObject *rval)
  {
    return (D) (python2c_func<C>::operator() (rval));
  }
};

template <> struct python2c_func<signed char> : public python2c_func_cast<signed char, char> { };
template <> struct python2c_func<unsigned char> : public python2c_func_cast<unsigned char, char> { };
template <> struct python2c_func<short> : public python2c_func_cast<short, long> { };
template <> struct python2c_func<unsigned short> : public python2c_func_cast<unsigned short, long> { };
template <> struct python2c_func<int> : public python2c_func_cast<int, long> { };
template <> struct python2c_func<unsigned int> : public python2c_func_cast<unsigned int, unsigned long> { };

template <> PYA_PUBLIC long long python2c_func<long long>::operator() (PyObject *rval);
template <> PYA_PUBLIC unsigned long long python2c_func<unsigned long long>::operator() (PyObject *rval);

#if defined(HAVE_64BIT_COORD)
template <> __int128 python2c_func<__int128>::operator() (PyObject *rval);
#endif

template <> PYA_PUBLIC double python2c_func<double>::operator() (PyObject *rval);
template <> struct python2c_func<float> : public python2c_func_cast<float, double> { };

template <> PYA_PUBLIC std::string python2c_func<std::string>::operator() (PyObject *rval);
template <> PYA_PUBLIC std::vector<char> python2c_func<std::vector<char> >::operator() (PyObject *rval);
#if defined(HAVE_QT)
template <> PYA_PUBLIC QByteArray python2c_func<QByteArray>::operator() (PyObject *rval);
template <> PYA_PUBLIC QString python2c_func<QString>::operator() (PyObject *rval);
#endif

template <> struct python2c_func<void *> : public python2c_func_cast<void *, size_t> { };

template <> PYA_PUBLIC tl::Variant python2c_func<tl::Variant>::operator() (PyObject *rval);

template <class T> struct python2c_func<T &>
{
  T &operator() (PyObject *rval)
  {
    tl_assert (rval != Py_None);

    const gsi::ClassBase *cls_decl = PythonModule::cls_for_type (Py_TYPE (rval));
    tl_assert (cls_decl != 0);
    tl_assert (is_derived_from (cls_decl, typeid (T)));

    PYAObjectBase *p = PYAObjectBase::from_pyobject (rval);
    return *((T *)p->obj ());
  }
};

template <class T> struct python2c_func<const T &>
{
  const T &operator() (PyObject *rval)
  {
    return python2c_func<T &>() (rval);
  }
};

template <class T> struct python2c_func<T *>
{
  T *operator() (PyObject *rval)
  {
    tl_assert (rval != Py_None);

    if (rval == Py_None) {
      return 0;
    } else {
      return &python2c_func<T &>() (rval);
    }
  }
};

template <class T> struct python2c_func<const T *>
{
  const T *operator() (PyObject *rval)
  {
    return python2c_func<T *> (rval);
  }
};

/**
 *  @brief Converts the Python object to the given type
 */
template <class T>
inline T python2c (PyObject *rval)
{
  return python2c_func<T> () (rval);
}

// -------------------------------------------------------------------
//  C to Python conversion

template <class T>
struct c2python_func
{
  PyObject *operator() (T t);
};

/**
 *  @brief Converts a C++ object reference to a Python object
 *  T must be a registered type.
 */
template <class T>
struct c2python_func<T &>
{
  PyObject *operator() (T &p)
  {
    return object_to_python ((void *) &p, 0, gsi::class_by_typeinfo_no_assert (typeid (T)), false /*==don't pass*/, false /*==non-const*/, false, false /*==can't destroy*/);
  }
};

/**
 *  @brief Converts a const C++ object reference to a Python object
 *  T must be a registered type.
 */
template <class T>
struct c2python_func<const T &>
{
  PyObject *operator() (const T &p)
  {
    return object_to_python ((void *) &p, 0, gsi::class_by_typeinfo_no_assert (typeid (T)), false /*==don't pass*/, true /*==const*/, false, false /*==can't destroy*/);
  }
};

/**
 *  @brief Converts a C++ object pointer to a Python object
 *  This version does not transfer ownership over the object to Python. The pointer will
 *  still be held by the caller. To transer the ownership to Python, use python2c_new(p).
 *  T must be a registered type.
 */
template <class T>
struct c2python_func<T *>
{
  PyObject *operator() (T *p)
  {
    if (! p) {
      Py_RETURN_NONE;
    } else {
      return object_to_python ((void *) p, 0, gsi::class_by_typeinfo_no_assert (typeid (T)), false /*==don't pass*/, false /*==non-const*/, false, false /*==can't destroy*/);
    }
  }
};

/**
 *  @brief Converts a C++ object pointer to a Python object
 *  This version will not transfer the ownership over the pointer as the pointer is const.
 *  T must be a registered type.
 */
template <class T>
struct c2python_func<const T *>
{
  PyObject *operator() (const T *p)
  {
    if (! p) {
      Py_RETURN_NONE;
    } else {
      return object_to_python ((void *) p, 0, gsi::class_by_typeinfo_no_assert (typeid (T)), false /*==don't pass*/, true /*==const*/, false, false /*==can't destroy*/);
    }
  }
};

template <>
struct c2python_func<bool>
{
  PyObject *operator() (bool c)
  {
    if (c) {
      Py_RETURN_TRUE;
    } else {
      Py_RETURN_FALSE;
    }
  }
};

template <>
struct c2python_func<char>
{
  PyObject *operator() (char c)
  {
    return PyLong_FromLong (long (c));
  }
};

template <>
struct c2python_func<signed char>
{
  PyObject *operator() (signed char c)
  {
    return PyLong_FromLong (long (c));
  }
};

template <>
struct c2python_func<unsigned char>
{
  PyObject *operator() (unsigned char c)
  {
    return PyLong_FromLong (long (c));
  }
};

template <>
struct c2python_func<short>
{
  PyObject *operator() (short c)
  {
    return PyLong_FromLong (long (c));
  }
};

template <>
struct c2python_func<unsigned short>
{
  PyObject *operator() (unsigned short c)
  {
    return PyLong_FromLong (long (c));
  }
};

template <>
struct c2python_func<int>
{
  PyObject *operator() (int c)
  {
    return PyLong_FromLong (long (c));
  }
};

template <>
struct c2python_func<unsigned int>
{
  PyObject *operator() (unsigned int c)
  {
    return PyLong_FromUnsignedLong ((unsigned long) (c));
  }
};

template <>
struct c2python_func<long>
{
  PyObject *operator() (long c)
  {
    return PyLong_FromLong (c);
  }
};

template <>
struct c2python_func<unsigned long>
{
  PyObject *operator() (unsigned long c)
  {
    return PyLong_FromUnsignedLong (c);
  }
};

template <>
struct c2python_func<long long>
{
  PyObject *operator() (long long c)
  {
    return PyLong_FromLongLong (c);
  }
};

template <>
struct c2python_func<unsigned long long>
{
  PyObject *operator() (unsigned long long c)
  {
    return PyLong_FromUnsignedLongLong (c);
  }
};

#if defined(HAVE_64BIT_COORD)
template <>
struct c2python_func<__int128>
{
  PyObject *operator() (const __int128 &c)
  {
    return PyLong_FromUnsignedLongLong (c);
  }
};
#endif

template <>
struct c2python_func<double>
{
  PyObject *operator() (double c)
  {
    return PyFloat_FromDouble (double (c));
  }
};

template <>
struct c2python_func<float>
{
  PyObject *operator() (float c)
  {
    return PyFloat_FromDouble (double (c));
  }
};

template <> PYA_PUBLIC PyObject *c2python_func<const char *>::operator() (const char *);

#if defined(HAVE_QT)
template <> PYA_PUBLIC PyObject *c2python_func<const QString &>::operator() (const QString &c);
template <> struct c2python_func<QString> : public c2python_func<const QString &> { };
template <> PYA_PUBLIC PyObject *c2python_func<const QByteArray &>::operator() (const QByteArray &c);
template <> struct c2python_func<QByteArray> : public c2python_func<const QByteArray &> { };
#endif

template <> PYA_PUBLIC PyObject *c2python_func<const std::string &>::operator() (const std::string &c);
template <> struct c2python_func<std::string> : public c2python_func<const std::string &> { };
template <> struct c2python_func<std::vector<char> > : public c2python_func<const std::vector<char> &> { };

template <>
struct c2python_func<void *>
  : public c2python_func<size_t>
{
  PyObject *operator() (void *p)
  {
    return c2python_func<size_t>::operator () (size_t (p));
  }
};

template <> PYA_PUBLIC PyObject *c2python_func<const tl::Variant &>::operator() (const tl::Variant &c);
template <> struct c2python_func<tl::Variant> : public c2python_func<const tl::Variant &> { };

/**
 *  @brief Converts the Python object to the given type
 */
template <class T>
inline PyObject *c2python (T t)
{
  return c2python_func<T> () (t);
}

/**
 *  @brief Converts a C++ object pointer to a Python object
 *  This version transfers ownership over the object to Python. The pointer will
 *  be help by Python and the object will be destroyed when Python no longer needs the
 *  object. To keep the ownership, use python2c(p).
 *  T must be a registered type.
 */
template <class T>
inline PyObject *c2python_new (T *p)
{
  if (! p) {
    Py_RETURN_NONE;
  } else {
    return object_to_python ((void *) p, 0, gsi::class_by_typeinfo_no_assert (typeid (T)), true /*==pass*/, false /*==non-const*/, false, true /*==can destroy*/);
  }
}

}

#endif

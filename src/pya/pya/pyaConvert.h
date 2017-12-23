
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "Python.h"

#include "tlVariant.h"
#include "tlException.h"
#include "tlHeap.h"

#include <string>
#include <QString>
#include <QByteArray>

namespace gsi
{
  class ClassBase;
  class ArgType;
}

namespace pya
{

class PYAObjectBase;

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
PyObject *
object_to_python (void *obj, PYAObjectBase *self, const gsi::ClassBase *cls, bool pass_obj, bool is_const, bool prefer_copy, bool can_destroy);

/**
 *  @brief Translates an object to a Python object (PyObject)
 *  This version takes it's flags from the atype given.
 */
PyObject *
object_to_python (void *obj, PYAObjectBase *self, const gsi::ArgType &atype);

// -------------------------------------------------------------------
//  Type checks 

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
bool test_type (PyObject * /*rval*/, bool /*loose*/)
{
  return false;
}

template <>
inline bool test_type<bool> (PyObject *rval, bool loose)
{
  if (loose) {
    return true;  // everything can be converted to bool
  } else {
    return PyBool_Check (rval) || rval == Py_None;
  }
}

//  used for other integer types as well:
template <>
inline bool test_type<int> (PyObject *rval, bool loose)
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

template <>
inline bool test_type<char> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<signed char> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<unsigned char> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<short> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<unsigned short> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<unsigned int> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<long> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<unsigned long> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<long long> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<unsigned long long> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}

#if defined(HAVE_64BIT_COORD)
template <>
inline bool test_type<__int128> (PyObject *rval, bool loose)
{
  return test_type<int> (rval, loose);
}
#endif

template <>
inline bool test_type<double> (PyObject *rval, bool loose)
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

template <>
inline bool test_type<float> (PyObject *rval, bool loose)
{
  return test_type<double> (rval, loose);
}

template <>
inline bool test_type<void *> (PyObject *rval, bool loose)
{
  return test_type<size_t> (rval, loose);
}

//  used for strings in general:
template <>
inline bool test_type<const char *> (PyObject *rval, bool /*loose*/)
{
#if PY_MAJOR_VERSION < 3
  return PyString_Check (rval) || PyUnicode_Check (rval) || PyByteArray_Check (rval);
#else
  return PyBytes_Check (rval) || PyUnicode_Check (rval) || PyByteArray_Check (rval);
#endif
}

template <>
inline bool test_type<std::string> (PyObject *rval, bool loose)
{
  return test_type<const char *> (rval, loose);
}

template <>
inline bool test_type<QString> (PyObject *rval, bool loose)
{
  return test_type<const char *> (rval, loose);
}

template <>
inline bool test_type<QByteArray> (PyObject *rval, bool loose)
{
  return test_type<const char *> (rval, loose);
}

template <>
inline bool test_type<tl::Variant> (PyObject * /*rval*/, bool /*loose*/)
{
  return true;
}

/**
 *  @brief Test a PyObject *for compatibility with a vector of the given type R
 */
template <class R>
inline bool test_vector (PyObject *arr, bool loose)
{
  if (PyList_Check (arr)) {

    size_t len = PyList_Size (arr);
    for (size_t i = 0; i < len; ++i) {
      if (! test_type<R> (PyList_GetItem (arr, i), loose)) {
        return false;
      }
    }

    return true;

  } else if (PyTuple_Check (arr)) {

    size_t len = PyTuple_Size (arr);
    for (size_t i = 0; i < len; ++i) {
      if (! test_type<R> (PyTuple_GetItem (arr, i), loose)) {
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

/**
 *  @brief Converts the Python object to the given type
 */
template <class T>
inline T python2c (PyObject *, tl::Heap * = 0)
{
  tl_assert (false);
  return T ();
}

template <> long python2c<long> (PyObject *rval, tl::Heap *heap);
template <> bool python2c<bool> (PyObject *rval, tl::Heap *heap);
template <> char python2c<char> (PyObject *rval, tl::Heap *heap);

template <>
inline signed char python2c<signed char> (PyObject *rval, tl::Heap *)
{
  return (signed char) python2c<char> (rval);
}

template <>
inline unsigned char python2c<unsigned char> (PyObject *rval, tl::Heap *)
{
  return (unsigned char) python2c<char> (rval);
}

template <>
inline short python2c<short> (PyObject *rval, tl::Heap *)
{
  return (short) python2c<long> (rval);
}

template <>
inline unsigned short python2c<unsigned short> (PyObject *rval, tl::Heap *)
{
  return (unsigned short) python2c<long> (rval);
}

template <>
inline int python2c<int> (PyObject *rval, tl::Heap *)
{
  return (int) python2c<long> (rval);
}

template <>
inline unsigned int python2c<unsigned int> (PyObject *rval, tl::Heap *)
{
  return (unsigned int) python2c<long> (rval);
}

template <> unsigned long python2c<unsigned long> (PyObject *rval, tl::Heap *heap);
template <> long long python2c<long long> (PyObject *rval, tl::Heap *heap);
template <> unsigned long long python2c<unsigned long long> (PyObject *rval, tl::Heap *heap);

#if defined(HAVE_64BIT_COORD)
template <> __int128 python2c<__int128> (PyObject *rval, tl::Heap *heap);
#endif

template <> double python2c<double> (PyObject *rval, tl::Heap *heap);

template <>
inline float python2c<float> (PyObject *rval, tl::Heap *)
{
  return (float) python2c<double> (rval);
}

template <> std::string python2c<std::string> (PyObject *rval, tl::Heap *heap);
template <> QByteArray python2c<QByteArray> (PyObject *rval, tl::Heap *heap);
template <> QString python2c<QString> (PyObject *rval, tl::Heap *heap);

template <>
inline void *python2c<void *> (PyObject *rval, tl::Heap *)
{
  return (void *) python2c<size_t> (rval);
}

template <> const char *python2c<const char *> (PyObject *rval, tl::Heap *heap);
template <> tl::Variant python2c<tl::Variant> (PyObject *rval, tl::Heap *heap);

// -------------------------------------------------------------------
//  C to Python conversion

/**
 *  @brief Converts the given type to a Python object
 */
template <class T>
PyObject *c2python (const T &);

template <>
inline PyObject *c2python<bool> (const bool &c)
{
  if (c) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

template <>
inline PyObject *c2python<char> (const char &c)
{
  return PyLong_FromLong (long (c));
}

template <>
inline PyObject *c2python<signed char> (const signed char &c)
{
  return PyLong_FromLong (long (c));
}

template <>
inline PyObject *c2python<unsigned char> (const unsigned char &c)
{
  return PyLong_FromLong (long (c));
}

template <>
inline PyObject *c2python<short> (const short &c)
{
  return PyLong_FromLong (long (c));
}

template <>
inline PyObject *c2python<unsigned short> (const unsigned short &c)
{
  return PyLong_FromLong (long (c));
}

template <>
inline PyObject *c2python<int> (const int &c)
{
  return PyLong_FromLong (long (c));
}

template <>
inline PyObject *c2python<unsigned int> (const unsigned int &c)
{
  return PyLong_FromLong (long (c));
}

template <>
inline PyObject *c2python<long> (const long &c)
{
  return PyLong_FromLong (c);
}

template <>
inline PyObject *c2python<unsigned long> (const unsigned long &c)
{
  return PyLong_FromUnsignedLong (c);
}

template <>
inline PyObject *c2python<long long> (const long long &c)
{
  return PyLong_FromLongLong (c);
}

template <>
inline PyObject *c2python<unsigned long long> (const unsigned long long &c)
{
  return PyLong_FromUnsignedLongLong (c);
}

#if defined(HAVE_64BIT_COORD)
template <>
inline PyObject *c2python<__int128> (const __int128 &c)
{
  return PyLong_FromUnsignedLongLong (c);
}
#endif

template <>
inline PyObject *c2python<double> (const double &c)
{
  return PyFloat_FromDouble (c);
}

template <>
inline PyObject *c2python<float> (const float &c)
{
  return PyFloat_FromDouble (double (c));
}

template <> PyObject *c2python<std::string> (const std::string &c);
template <> PyObject *c2python<QByteArray> (const QByteArray &qba);
template <> PyObject *c2python<QString> (const QString &qs);

template <>
inline PyObject *c2python<void *> (void * const &s)
{
  return c2python<size_t> (size_t (s));
}

template <>
PyObject *c2python<tl::Variant> (const tl::Variant &c);

template <> PyObject *c2python<const char *> (const char * const &p);

}

#endif


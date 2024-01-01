
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#ifndef _HDR_rbaConvert
#define _HDR_rbaConvert

#ifdef HAVE_RUBY

#include "gsiTypes.h"

#include "rbaUtils.h"
#include <ruby.h>

namespace rba
{

class Proxy;

// -------------------------------------------------------------------

/**
 *  @brief Translates an object to a Ruby object (VALUE)
 *  @param obj The generic object pointer
 *  @param self The object which the object is derived from (self in a method call) or 0 if there is no such object
 *  @param cls The class of the object
 *  @param pass_obj If true, the Ruby object will own the original object which gets destroyed when the Ruby object is finalized
 *  @param is_const If true, the Ruby object will be a const one unless the original object is already bound in a non-const way
 *  @param prefer_copy If true, the Ruby object will be copied unless there is a way to use shared references (for managed objects)
 *  @param can_destroy If true, the Ruby object can be destroyed explicitly
 *  @return The Ruby object
 */
VALUE object_to_ruby (void *obj, Proxy *self, const gsi::ClassBase *cls, bool pass_obj, bool is_const, bool prefer_copy, bool can_destroy);

/**
 *  @brief Translates an object to a Ruby object (VALUE)
 *  This version takes it's flags from the atype given.
 */
VALUE object_to_ruby (void *obj, Proxy *self, const gsi::ArgType &atype);

// -------------------------------------------------------------------
//  Type checks
//
//  The type checks are somewhat more picky than the ruby2c functions.
//  The type checks are used to resolve overridden methods, so being picky
//  might be more appropriate.

template <class T>
inline bool test_type (VALUE /*rval*/, bool /*loose*/)
{
  return false;
}

template <>
inline bool test_type<bool> (VALUE rval, bool loose)
{
  if (loose) {
    return true;  // everything can be converted to bool
  } else {
    unsigned int t = TYPE (rval);
    return (t == T_FALSE || t == T_TRUE || t == T_NIL);
  }
}

//  used for other integer types as well:
template <>
inline bool test_type<int> (VALUE rval, bool loose)
{
  unsigned int t = TYPE (rval);
  return (t == T_FIXNUM || t == T_BIGNUM || (loose && t == T_FLOAT));
}

template <>
inline bool test_type<char> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<signed char> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<unsigned char> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<short> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<unsigned short> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<unsigned int> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<long> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<unsigned long> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<long long> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}

template <>
inline bool test_type<unsigned long long> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}

#if defined(HAVE_64BIT_COORD)
template <>
inline bool test_type<__int128> (VALUE rval, bool loose)
{
  return test_type<int> (rval, loose);
}
#endif

template <>
inline bool test_type<double> (VALUE rval, bool loose)
{
  unsigned int t = TYPE (rval);
  return (t == T_FLOAT || (loose && (t == T_FIXNUM || t == T_BIGNUM)));
}

template <>
inline bool test_type<float> (VALUE rval, bool loose)
{
  return test_type<double> (rval, loose);
}

template <>
inline bool test_type<void *> (VALUE rval, bool loose)
{
  return test_type<size_t> (rval, loose);
}

template <>
inline bool test_type<gsi::StringType> (VALUE rval, bool /*loose*/)
{
  return TYPE (rval) == T_STRING;
}

template <>
inline bool test_type<gsi::ByteArrayType> (VALUE rval, bool /*loose*/)
{
  return TYPE (rval) == T_STRING;
}

template <>
inline bool test_type<gsi::VariantType> (VALUE /*rval*/, bool /*loose*/)
{
  //  assume we can translate everything into a variant
  return true;
}

template <>
inline bool test_type<gsi::ObjectType> (VALUE rval, bool)
{
  return TYPE (rval) == T_DATA;
}

template <>
inline bool test_type<gsi::VectorType> (VALUE rval, bool)
{
  return TYPE (rval) == T_ARRAY;
}

template <>
inline bool test_type<gsi::MapType> (VALUE rval, bool)
{
  return TYPE (rval) == T_HASH;
}

// -------------------------------------------------------------------
//  Ruby to C conversion

template <class T>
T ruby2c (VALUE v);

template <>
inline bool ruby2c<bool> (VALUE rval)
{
  return bool (RTEST (rval));
}

template <>
inline char ruby2c<char> (VALUE rval)
{
  return rba_safe_num2int (rval);
}

template <>
inline signed char ruby2c<signed char> (VALUE rval)
{
  return rba_safe_num2int (rval);
}

template <>
inline unsigned char ruby2c<unsigned char> (VALUE rval)
{
  return rba_safe_num2uint (rval);
}

template <>
inline short ruby2c<short> (VALUE rval)
{
  return rba_safe_num2int (rval);
}

template <>
inline unsigned short ruby2c<unsigned short> (VALUE rval)
{
  return rba_safe_num2uint (rval);
}

template <>
inline int ruby2c<int> (VALUE rval)
{
  return rba_safe_num2int (rval);
}

template <>
inline unsigned int ruby2c<unsigned int> (VALUE rval)
{
  return rba_safe_num2uint (rval);
}

template <>
inline long ruby2c<long> (VALUE rval)
{
  return rba_safe_num2long (rval);
}

template <>
inline unsigned long ruby2c<unsigned long> (VALUE rval)
{
  return rba_safe_num2ulong (rval);
}

template <>
inline long long ruby2c<long long> (VALUE rval)
{
  return rba_safe_num2ll (rval);
}

template <>
inline unsigned long long ruby2c<unsigned long long> (VALUE rval)
{
  return rba_safe_num2ull (rval);
}

#if defined(HAVE_64BIT_COORD)
template <>
inline __int128 ruby2c<__int128> (VALUE rval)
{
  // TODO: this is pretty simplistic
  return rba_safe_num2dbl (rval);
}
#endif

template <>
inline double ruby2c<double> (VALUE rval)
{
  return rba_safe_num2dbl (rval);
}

template <>
inline float ruby2c<float> (VALUE rval)
{
  return float (rba_safe_num2dbl (rval));
}

template <>
inline std::string ruby2c<std::string> (VALUE rval)
{
  VALUE str = rba_safe_string_value (rval);
  return std::string (RSTRING_PTR(str), RSTRING_LEN(str));
}

template <>
inline std::vector<char> ruby2c<std::vector<char> > (VALUE rval)
{
  VALUE str = rba_safe_string_value (rval);
  char *cp = RSTRING_PTR(str);
  size_t sz = RSTRING_LEN(str);
  return std::vector<char> (cp, cp + sz);
}

#if defined(HAVE_QT)
template <>
inline QByteArray ruby2c<QByteArray> (VALUE rval)
{
  VALUE str = rba_safe_string_value (rval);
  return QByteArray (RSTRING_PTR(str), RSTRING_LEN(str));
}

template <>
inline QString ruby2c<QString> (VALUE rval)
{
  VALUE str = rba_safe_string_value (rval);
  return tl::to_qstring (std::string (RSTRING_PTR(str), RSTRING_LEN(str)));
}
#endif

template <>
inline void *ruby2c<void *> (VALUE rval)
{
  return (void *) ruby2c<size_t> (rval);
}

template <>
inline const char *ruby2c<const char *> (VALUE rval)
{
  VALUE str = rba_safe_string_value (rval);
  return RSTRING_PTR(str);
}

template <>
tl::Variant ruby2c<tl::Variant> (VALUE rval);

// -------------------------------------------------------------------
//  C to Ruby conversion

template <class R>
VALUE c2ruby (const R &r);

template <>
inline VALUE c2ruby<bool> (const bool &c)
{
  return c ? Qtrue : Qfalse;
}

template <>
inline VALUE c2ruby<char> (const char &c)
{
  return INT2NUM (c);
}

template <>
inline VALUE c2ruby<signed char> (const signed char &c)
{
  return UINT2NUM (c);
}

template <>
inline VALUE c2ruby<unsigned char> (const unsigned char &c)
{
  return UINT2NUM (c);
}

template <>
inline VALUE c2ruby<short> (const short &c)
{
  return INT2NUM (c);
}

template <>
inline VALUE c2ruby<unsigned short> (const unsigned short &c)
{
  return UINT2NUM (c);
}

template <>
inline VALUE c2ruby<int> (const int &c)
{
  return INT2NUM (c);
}

template <>
inline VALUE c2ruby<unsigned int> (const unsigned int &c)
{
  return UINT2NUM (c);
}

template <>
inline VALUE c2ruby<long> (const long &c)
{
  return LONG2NUM (c);
}

template <>
inline VALUE c2ruby<unsigned long> (const unsigned long &c)
{
  return ULONG2NUM (c);
}

template <>
inline VALUE c2ruby<long long> (const long long &c)
{
  return LL2NUM (c);
}

template <>
inline VALUE c2ruby<unsigned long long> (const unsigned long long &c)
{
  return ULL2NUM (c);
}

#if defined(HAVE_64BIT_COORD)
template <>
inline VALUE c2ruby<__int128> (const __int128 &c)
{
  // TODO: this is pretty simplistic
  return rb_float_new (double (c));
}
#endif

template <>
inline VALUE c2ruby<double> (const double &c)
{
  return rb_float_new (c);
}

template <>
inline VALUE c2ruby<float> (const float &c)
{
  return rb_float_new (c);
}

template <>
inline VALUE c2ruby<std::string> (const std::string &c)
{
  return rb_str_new (c.c_str (), long (c.size ()));
}

template <>
inline VALUE c2ruby<std::vector<char> > (const std::vector<char> &c)
{
  return rb_str_new (&c.front (), c.size ());
}

#if defined(HAVE_QT)
template <>
inline VALUE c2ruby<QByteArray> (const QByteArray &qba)
{
  if (qba.isNull ()) {
    return Qnil;
  } else {
    return rb_str_new (qba.constData (), qba.size ());
  }
}

template <>
inline VALUE c2ruby<QString> (const QString &qs)
{
  if (qs.isNull ()) {
    return Qnil;
  } else {
    std::string c (tl::to_string (qs));
    return rb_str_new (c.c_str (), long (c.size ()));
  }
}
#endif

template <>
inline VALUE c2ruby<void *> (void * const &s)
{
  return c2ruby<size_t> (size_t (s));
}

template <>
VALUE c2ruby<tl::Variant> (const tl::Variant &c);

template <>
inline VALUE c2ruby<const char *> (const char * const & s)
{
  if (! s) {
    static const char null_string[] = "(null)";
    return rb_str_new (null_string, sizeof (null_string) - 1);
  } else {
    return rb_str_new (s, long (strlen (s)));
  }
}

}

#endif

#endif

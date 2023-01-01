
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


#ifndef _HDR_tlVariantUserClasses
#define _HDR_tlVariantUserClasses

#include "tlVariant.h"
#include "tlTypeTraits.h"
#include "tlString.h"
#include "tlAssert.h"

namespace tl
{

class EvalClass;

/**
 *  @brief A helper function to implement clone as efficiently as possible
 */
template<class T, bool> struct _var_user_clone_impl;

template<class T>
struct _var_user_clone_impl<T, true>
{
  static T *call (const T *a) { return new T (*a); }
};

template<class T>
struct _var_user_clone_impl<T, false>
{
  static T *call (const T *) { tl_assert (false); }
};

/**
 *  @brief A helper function to implement assign as efficiently as possible
 */
template<class T, bool> struct _var_user_assign_impl;

template<class T>
struct _var_user_assign_impl<T, true>
{
  static void call (T *a, const T *b) { *a = *b; }
};

template<class T>
struct _var_user_assign_impl<T, false>
{
  static void call (T *, const T *) { tl_assert (false); }
};

/**
 *  @brief A helper function to implement equal as efficiently as possible
 */
template<class T, bool> struct _var_user_equal_impl;

template<class T>
struct _var_user_equal_impl<T, true>
{
  static bool call (const T *a, const T *b) { return *a == *b; }
};

template<class T>
struct _var_user_equal_impl<T, false>
{
  static bool call (const T *, const T *) { tl_assert (false); }
};

/**
 *  @brief A helper function to implement equal as efficiently as possible
 */
template<class T, bool> struct _var_user_less_impl;

template<class T>
struct _var_user_less_impl<T, true>
{
  static bool call (const T *a, const T *b) { return *a < *b; }
};

template<class T>
struct _var_user_less_impl<T, false>
{
  static bool call (const T *, const T *) { tl_assert (false); }
};

/**
 *  @brief A helper function to implement equal as efficiently as possible
 */
template<class T, bool> struct _var_user_to_string_impl;

template<class T>
struct _var_user_to_string_impl<T, true>
{
  static std::string call (const T *a) { return a->to_string (); }
};

template<class T>
struct _var_user_to_string_impl<T, false>
{
  static std::string call (const T *) { tl_assert (false); }
};

/**
 *  @brief A helper function to implement equal as efficiently as possible
 */
template<class T, bool> struct _var_user_to_int_impl;

template<class T>
struct _var_user_to_int_impl<T, true>
{
  static int call (const T *a) { return a->to_int (); }
};

template<class T>
struct _var_user_to_int_impl<T, false>
{
  static int call (const T *) { tl_assert (false); }
};

/**
 *  @brief A helper function to implement equal as efficiently as possible
 */
template<class T, bool> struct _var_user_to_double_impl;

template<class T>
struct _var_user_to_double_impl<T, true>
{
  static double call (const T *a) { return a->to_double (); }
};

template<class T>
struct _var_user_to_double_impl<T, false>
{
  static double call (const T *) { tl_assert (false); }
};

/**
 *  @brief A helper function to implement equal as efficiently as possible
 */
template<class T, bool> struct _var_user_to_variant_impl;

template<class T>
struct _var_user_to_variant_impl<T, true>
{
  static tl::Variant call (const T *a) { return a->to_variant (); }
};

template<class T>
struct _var_user_to_variant_impl<T, false>
{
  static tl::Variant call (const T *) { tl_assert (false); }
};

/**
 *  @brief A utility implementation of tl::VariantUserClass using type traits for the implementation
 */
template <class T>
class VariantUserClassImpl 
  : public tl::VariantUserClass<T>
{
public:
  virtual void *create () const 
  { 
    return new T (); 
  }

  virtual void destroy (void *a) const 
  {
    delete (T *)a; 
  }

  virtual bool equal (const void *a, const void *b) const
  { 
    return _var_user_equal_impl<T, tl::has_equal_operator<T>::value>::call ((const T *) a, (const T *) b);
  }

  virtual bool less (const void *a, const void *b) const
  { 
    return _var_user_less_impl<T, tl::has_less_operator<T>::value>::call ((const T *) a, (const T *) b);
  }

  virtual void *clone (const void *a) const
  { 
    return _var_user_clone_impl<T, std::is_copy_constructible<T>::value>::call ((const T *) a);
  }

  virtual void assign (void *a, const void *b) const
  {
    _var_user_assign_impl<T, std::is_copy_assignable<T>::value>::call ((T *) a, (const T *)b);
  }

  virtual std::string to_string (const void *a) const
  { 
    return _var_user_to_string_impl<T, tl::has_to_string<T>::value>::call ((const T *) a);
  }

  virtual int to_int (const void *a) const
  {
    return _var_user_to_int_impl<T, tl::has_to_int<T>::value>::call ((const T *) a);
  }

  virtual double to_double (const void *a) const
  {
    return _var_user_to_double_impl<T, tl::has_to_double<T>::value>::call ((const T *) a);
  }

  virtual void to_variant (const void *a, tl::Variant &v) const
  {
    v = _var_user_to_variant_impl<T, tl::has_to_variant<T>::value>::call ((const T *) a);
  }

  virtual void read (void *a, tl::Extractor &ex) const
  { 
    ex.read (*(T *)a);
  }

  virtual const char *name () const 
  { 
    return ""; 
  }

  virtual bool is_const () const 
  { 
    return false; 
  }

  virtual bool is_ref () const
  {
    return false;
  }

  virtual void *deref_proxy (tl::Object *obj) const
  {
    //  By default, the tl::Object is considered to be the first base class of the actual object
    return obj;
  }

  virtual const gsi::ClassBase *gsi_cls () const
  { 
    return 0; 
  }

  virtual const tl::EvalClass *eval_cls () const 
  { 
    return 0; 
  }
};

}

#endif





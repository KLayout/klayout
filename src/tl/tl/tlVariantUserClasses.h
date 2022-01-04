
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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
 *  @brief A helper function to implement equal as efficiently as possible
 */
template<class T, class I>
bool _var_user_equal_impl (const T *a, const T *b, I);

template<class T>
bool _var_user_equal_impl (const T *a, const T *b, tl::true_tag)
{
  return *a == *b;
}

template<class T>
bool _var_user_equal_impl (const T * /*a*/, const T * /*b*/, tl::false_tag)
{
  tl_assert (false);
}

/**
 *  @brief A helper function to implement clone as efficiently as possible
 */
template<class T, class I>
T *_var_user_clone_impl (const T *a, I);

template<class T>
T *_var_user_clone_impl (const T *a, tl::true_tag)
{
  return new T (*a);
}

template<class T>
T *_var_user_clone_impl (const T * /*a*/, tl::false_tag)
{
  tl_assert (false);
}

/**
 *  @brief A helper function to implement assignment as efficiently as possible
 */
template<class T, class I>
void _var_user_assign_impl (T *a, const T *b, I);

template<class T>
void _var_user_assign_impl (T *a, const T *b, tl::true_tag)
{
  *a = *b;
}

template<class T>
void _var_user_assign_impl (T * /*a*/, const T * /*b*/, tl::false_tag)
{
  tl_assert (false);
}

/**
 *  @brief A helper function to implement less as efficiently as possible
 */
template<class T, class I>
bool _var_user_less_impl (const T *a, const T *b, I);

template<class T>
bool _var_user_less_impl (const T *a, const T *b, tl::true_tag)
{
  return *a < *b;
}

template<class T>
bool _var_user_less_impl (const T *, const T *, tl::false_tag)
{
  tl_assert (false);
}

/**
 *  @brief A helper function to implement to_string as efficiently as possible
 */
template<class T, class I>
std::string _var_user_to_string_impl (const T *a, I);

template<class T>
std::string _var_user_to_string_impl (const T *a, tl::true_tag)
{
  return a->to_string ();
}

template<class T>
std::string _var_user_to_string_impl (const T *, tl::false_tag)
{
  tl_assert (false);
}

/**
 *  @brief A helper function to implement read as efficiently as possible
 */
template<class T, class I>
void _var_user_read_impl (T *a, tl::Extractor &ex, I);

template<class T>
void _var_user_read_impl (T *a, tl::Extractor &ex, tl::true_tag)
{
  ex.read (*a);
}

template<class T>
void _var_user_read_impl (T *, tl::Extractor &, tl::false_tag)
{
  tl_assert (false);
}

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
    typename tl::type_traits<T>::has_equal_operator f;
    return _var_user_equal_impl ((T *) a, (T *) b, f);
  }

  virtual bool less (const void *a, const void *b) const
  { 
    typename tl::type_traits<T>::has_less_operator f;
    return _var_user_less_impl ((T *) a, (T *) b, f);
  }

  virtual void *clone (const void *a) const
  { 
    typename tl::type_traits<T>::has_copy_constructor f;
    return _var_user_clone_impl ((const T *) a, f);
  }

  virtual void assign (void *a, const void *b) const
  {
    //  TODO: we assume (for now) that objects with a copy constructor do have an assignment operator too
    typename tl::type_traits<T>::has_copy_constructor f;
    _var_user_assign_impl ((T *) a, (const T *) b, f);
  }

  virtual std::string to_string (const void *a) const
  { 
    typename tl::type_traits<T>::supports_to_string f;
    return _var_user_to_string_impl ((const T *) a, f);
  }

  virtual void read (void *a, tl::Extractor &ex) const 
  { 
    typename tl::type_traits<T>::supports_extractor f;
    _var_user_read_impl ((T *) a, ex, f);
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





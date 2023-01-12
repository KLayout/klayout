
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


#ifndef _HDR_gsiClass
#define _HDR_gsiClass

#include "gsiClassBase.h"
#include "gsiObject.h"

#include "tlTypeTraits.h"
#include "tlHeap.h"
#include "tlUtils.h"
#include "tlVariant.h"

#include <memory>

//  For a comprehensive documentation see gsi.h

namespace gsi
{

// -----------------------------------------------------------------------------
//  Variant binding

/**
 *  @brief A helper function to implement equal as efficiently as possible
 */
template<class T, bool> struct _var_user_equal_impl;

template<class T>
struct _var_user_equal_impl<T, true>
{
  static bool call (const T *a, const T *b, const VariantUserClassImpl * /*delegate*/) { return *a == *b; }
};

template<class T>
struct _var_user_equal_impl<T, false>
{
  static bool call (const T *a, const T *b, const VariantUserClassImpl *delegate) { return delegate->equal_impl ((void *) a, (void *) b); }
};

/**
 *  @brief A helper function to implement equal as efficiently as possible
 */
template<class T, bool> struct _var_user_less_impl;

template<class T>
struct _var_user_less_impl<T, true>
{
  static bool call (const T *a, const T *b, const VariantUserClassImpl * /*delegate*/) { return *a < *b; }
};

template<class T>
struct _var_user_less_impl<T, false>
{
  static bool call (const T *a, const T *b, const VariantUserClassImpl *delegate) { return delegate->less_impl ((void *) a, (void *) b); }
};

/**
 *  @brief A helper function to implement to_string as efficiently as possible
 */
template<class T, bool> struct _var_user_to_string_impl;

template<class T>
struct _var_user_to_string_impl<T, true>
{
  static std::string call (const T *a, const VariantUserClassImpl * /*delegate*/) { return a->to_string (); }
};

template<class T>
struct _var_user_to_string_impl<T, false>
{
  static std::string call (const T *a, const VariantUserClassImpl *delegate) { return delegate->to_string_impl ((void *) a); }
};

/**
 *  @brief A helper function to implement to_variant as efficiently as possible
 */
template<class T, bool> struct _var_user_to_variant_impl;

template<class T>
struct _var_user_to_variant_impl<T, true>
{
  static tl::Variant call (const T *a, const VariantUserClassImpl * /*delegate*/) { return a->to_variant (); }
};

template<class T>
struct _var_user_to_variant_impl<T, false>
{
  static tl::Variant call (const T *a, const VariantUserClassImpl *delegate) { return delegate->to_variant_impl ((void *) a); }
};

/**
 *  @brief A helper function to implement to_int as efficiently as possible
 */
template<class T, bool> struct _var_user_to_int_impl;

template<class T>
struct _var_user_to_int_impl<T, true>
{
  static int call (const T *a, const VariantUserClassImpl * /*delegate*/) { return a->to_int (); }
};

template<class T>
struct _var_user_to_int_impl<T, false>
{
  static int call (const T *a, const VariantUserClassImpl *delegate) { return delegate->to_int_impl ((void *) a); }
};

/**
 *  @brief A helper function to implement to_double as efficiently as possible
 */
template<class T, bool> struct _var_user_to_double_impl;

template<class T>
struct _var_user_to_double_impl<T, true>
{
  static double call (const T *a, const VariantUserClassImpl * /*delegate*/) { return a->to_double (); }
};

template<class T>
struct _var_user_to_double_impl<T, false>
{
  static double call (const T *a, const VariantUserClassImpl *delegate) { return delegate->to_double_impl ((void *) a); }
};

/**
 *  @brief A VariantUserClassBase specialization that links GSI classes and Variant classes
 */
template <class T>
class GSI_PUBLIC_TEMPLATE VariantUserClass
  : public tl::VariantUserClass<T>, private VariantUserClassImpl
{
public:
  VariantUserClass ()
    : VariantUserClassImpl (),
      mp_cls (0), mp_object_cls (0), m_is_const (false)
  {
    //  .. nothing yet ..
  }

  ~VariantUserClass ()
  {
    mp_cls = 0;
    tl::VariantUserClass<T>::unregister_instance (this, m_is_const);
  }

  void initialize (const gsi::ClassBase *cls, const tl::VariantUserClassBase *object_cls, bool is_const)
  {
    mp_cls = cls; 
    mp_object_cls = object_cls;
    m_is_const = is_const;

    VariantUserClassImpl::initialize (cls, this, object_cls, is_const);

    if (! object_cls) {
      //  Register the const/non-const classes for the tl::Variant payload.
      tl::VariantUserClass<T>::register_instance (this, m_is_const);
    }
  }

  const tl::EvalClass *eval_cls () const 
  {
    return this;
  }

  void *deref_proxy (tl::Object *proxy) const
  {
    Proxy *p = dynamic_cast<Proxy *> (proxy);
    if (p) {
      return p->obj ();
    } else {
      return 0;
    }
  }

  virtual bool equal (const void *a, const void *b) const
  {
    return gsi::_var_user_equal_impl<T, tl::has_equal_operator<T>::value>::call ((const T *) a, (const T *) b, this);
  }

  virtual bool less (const void *a, const void *b) const
  {
    return gsi::_var_user_less_impl<T, tl::has_less_operator<T>::value>::call ((const T *) a, (const T *) b, this);
  }

  virtual void to_variant (const void *a, tl::Variant &var) const
  {
    var = gsi::_var_user_to_variant_impl<T, tl::has_to_variant<T>::value>::call ((const T *) a, this);
  }

  virtual std::string to_string (const void *a) const
  {
    return gsi::_var_user_to_string_impl<T, tl::has_to_string<T>::value>::call ((const T *) a, this);
  }

  virtual int to_int (const void *a) const
  {
    return gsi::_var_user_to_int_impl<T, tl::has_to_int<T>::value>::call ((const T *) a, this);
  }

  virtual double to_double (const void *a) const
  {
    return gsi::_var_user_to_double_impl<T, tl::has_to_double<T>::value>::call ((const T *) a, this);
  }

  void *clone (const void *obj) const
  {
    void *new_obj = mp_cls->create ();
    mp_cls->assign (new_obj, obj);
    return new_obj;
  }

  void assign (void *self, const void *other) const
  {
    mp_cls->assign (self, other);
  }

  void *create () const
  {
    return mp_cls->create ();
  }

  void destroy (void *obj) const 
  {
    if (obj) {
      mp_cls->destroy (obj);
    }
  }

  const char *name () const
  {
    return mp_cls ? mp_cls->name ().c_str() : 0;
  }

  void read (void *a, tl::Extractor &ex) const
  {
    T *t = (T *) a;
    ex.read (*t);
  }

  const gsi::ClassBase *gsi_cls () const
  {
    return mp_cls;
  }

  bool is_class () const 
  { 
    return mp_object_cls != 0; 
  }

  bool is_const () const
  { 
    return m_is_const; 
  }

private:
  const gsi::ClassBase *mp_cls;
  const tl::VariantUserClassBase *mp_object_cls;
  bool m_is_const;

  //  no copying currently
  VariantUserClass &operator= (const VariantUserClass &);
  VariantUserClass (const VariantUserClass &);
};

// -----------------------------------------------------------------------------
//  GSI implementation

template <class X, bool> struct _destroy;
template <class X> struct _destroy<X, false> { static void call (X *) { tl_assert (false); } };
template <class X> struct _destroy<X, true> { static void call (X *x) { delete x; } };

template <class X, bool> struct _create;
template <class X> struct _create<X, false> { static void *call () { throw tl::Exception (tl::to_string (tr ("Object cannot be created here"))); } };
template <class X> struct _create<X, true> { static void *call () { return new X (); } };

template <class X, bool> struct _clone;
template <class X> struct _clone<X, false> { static void *call (const void *) { throw tl::Exception (tl::to_string (tr ("Object cannot be copied here"))); } };
template <class X> struct _clone<X, true> { static void *call (const void *other) { return new X (*(const X *)other); } };

template <class X, bool> struct _assign;
template <class X> struct _assign<X, false> { static void call (void *, const void *) { throw tl::Exception (tl::to_string (tr ("Object cannot be copied here"))); } };
template <class X> struct _assign<X, true> { static void call (void *dest, const void *src) { *(X *)dest = *(const X *)src; } };

/**
 *  @brief A helper class which tests whether a given object can be upcast 
 */
class SubClassTesterBase
{
public:
  SubClassTesterBase () { }
  virtual ~SubClassTesterBase () { }

  virtual bool can_upcast (const void *p) const = 0;
};

/**
 *  @brief A specific implementation of the upcast tester
 *  
 *  The can_upcast method will return true, if the object (which has at least to 
 *  be of type B) can be upcast to X.
 */
template <class X, class B, bool B_IS_POLYMORPHIC>
class SubClassTester;

/**
 *  @brief Specialization for polymorphic types - we can use dynamic_cast to tests whether B object can be cast to X
 */
template <class X, class B>
class SubClassTester<X, B, true>
  : public SubClassTesterBase 
{
public:
  virtual bool can_upcast (const void *p) const 
  {
    return dynamic_cast<const X *>((const B *)p) != 0;
  }
};

/**
 *  @brief Specialization for non-polymorphic types
 */
template <class X, class B>
class SubClassTester<X, B, false>
  : public SubClassTesterBase 
{
public:
  virtual bool can_upcast (const void *) const 
  {
    //  Non-polymorphic classes can't be upcast, hence we always return false here
    return false;
  }
};

/**
 *  @brief An extension declaration
 *
 *  Instantiating an object of this kind will extend the class X with 
 *  the given methods.
 */
template <class X>
class GSI_PUBLIC_TEMPLATE ClassExt
  : public ClassBase
{
public:
  ClassExt (const Methods &mm, const std::string &doc = std::string ())
    : ClassBase (doc, mm), mp_declaration (0)
  {
    //  .. nothing yet ..
  }

  ClassExt (const std::string &doc = std::string ())
    : ClassBase (doc, Methods ()), mp_declaration (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Imports a class into this class
   *  This constructor will pull in another class into this one as a kind of link using
   *  a different name.
   *  This feature is not quite useful usually and is reserved for special use cases
   *  such as including enums into a declaration namespace.
   */
  ClassExt (const ClassBase &import, const std::string &name = std::string (), const std::string &doc = std::string ())
    : ClassBase (doc, Methods ()), mp_declaration (&import)
  {
    set_name (name);
  }

  virtual bool binds () const
  {
    return false;
  }

  virtual const std::type_info &type () const
  {
    return typeid (X);
  }

  /**
   *  @brief Gets the real (main) declaration object
   *  The main declaration object is 0 initially indicating that the classes
   *  have not been merged.
   */
  virtual const ClassBase *declaration () const 
  {
    return mp_declaration;
  }

  virtual bool consolidate () const
  {
    //  gets the "real" declaration by the type info
    //  TODO: ugly const_cast hack
    ClassBase *non_const_decl = const_cast<ClassBase *> (cls_decl<X> ());

    //  transfer the methods
    for (gsi::ClassBase::method_iterator m = begin_methods (); m != end_methods (); ++m) {
      non_const_decl->add_method ((*m)->clone ());
    }

    //  Treat class imports (extensions with a base class): import the class as 
    //  a child class plus import constants into the class (intended for enum import).
    if (declaration ()) {
      non_const_decl->add_child_class (this);
    }

    //  No longer required
    return false;
  }

private:
  const ClassBase *mp_declaration;
};

/**
 *  @brief A tag indicating "no adaptor"
 */
struct NoAdaptorTag { };

/**
 *  @brief Delivers the typeid for the given class 
 */
template <class X, class Adapted>
struct adaptor_type_info
{
  static const std::type_info *type_info () 
  {
    return &typeid (Adapted);
  }

  static X *create (const Adapted *a)
  {
    return new X (*a);
  }

  static X *create_consume (Adapted *a)
  {
    X *x = new X (*a);
    delete a;
    return x;
  }

  static const Adapted *get (const X *x)
  {
    return &x->value ();
  }

  static Adapted *create_adapted (const X *x)
  {
    return new Adapted (x->value ());
  }
};

template <class X>
struct adaptor_type_info<X, NoAdaptorTag>
{
  static const std::type_info *type_info () 
  {
    return 0;
  }

  static X *create (const NoAdaptorTag *)
  {
    tl_assert (false);
    return 0;
  }

  static X *create_consume (NoAdaptorTag *)
  {
    tl_assert (false);
    return 0;
  }

  static const NoAdaptorTag *get (const X *)
  {
    tl_assert (false);
    return 0;
  }

  static NoAdaptorTag *create_adapted (const X *)
  {
    tl_assert (false);
    return 0;
  }
};

/**
 *  @brief The declaration of a specific class
 *
 *  This class declares all methods that are required to instantiate or copy an object
 *  or to call it's methods in some generic way.
 */
template <class X, class Adapted = NoAdaptorTag>
class GSI_PUBLIC_TEMPLATE Class
  : public ClassBase
{
public:
  Class (const std::string &module, const std::string &name, const Methods &mm, const std::string &doc = std::string (), bool do_register = true)
    : ClassBase (doc, mm, do_register)
  {
    set_module (module);
    set_name (name);
  }

  template <class B>
  Class (const Class<B> &base, const std::string &module, const std::string &name, const Methods &mm, const std::string &doc = std::string (), bool do_register = true)
    : ClassBase (doc, mm, do_register), m_subclass_tester (new SubClassTester<X, B, std::is_polymorphic<B>::value> ())
  {
    set_module (module);
    set_name (name);
    set_base (&base);
  }

  Class (const std::string &module, const std::string &name, const std::string &doc = std::string (), bool do_register = true)
    : ClassBase (doc, Methods (), do_register)
  {
    set_module (module);
    set_name (name);
  }

  template <class B>
  Class (const Class<B> &base, const std::string &module, const std::string &name, const std::string &doc = std::string (), bool do_register = true)
    : ClassBase (doc, Methods (), do_register), m_subclass_tester (new SubClassTester<X, B, std::is_polymorphic<B>::value> ())
  {
    set_module (module);
    set_name (name);
    set_base (&base);
  }

  virtual const std::type_info *adapted_type_info () const 
  {
    return adaptor_type_info<X, Adapted>::type_info ();
  }

  virtual const ClassBase *declaration () const
  {
    return this;
  }

  virtual bool consolidate () const
  {
    return true;
  }

  void initialize ()
  {
    ClassBase::initialize ();
    m_var_cls.initialize (this, 0, false);
    m_var_cls_c.initialize (this, 0, true);
    m_var_cls_cls.initialize (this, &m_var_cls, false);
  }

  bool is_managed () const
  {
    return tl::is_derived<gsi::ObjectBase, X> ();
  }

  gsi::ObjectBase *gsi_object (void *p, bool /*required*/) const
  {
    return tl::try_static_cast<gsi::ObjectBase, X> ((X *) p);
  }

  virtual void destroy (void *p) const
  {
    X *x = (X *)p;
    _destroy<X, std::is_destructible<X>::value>::call (x);
  }

  virtual void *create () const
  {
    return _create<X, std::is_default_constructible<X>::value>::call ();
  }

  virtual void *create_from_adapted (const void *x) const
  {
    return adaptor_type_info<X, Adapted>::create ((const Adapted *) x);
  }

  virtual void *create_from_adapted_consume (void *x) const
  {
    return adaptor_type_info<X, Adapted>::create_consume ((Adapted *) x);
  }

  virtual const void *adapted_from_obj (const void *obj) const
  {
    return adaptor_type_info<X, Adapted>::get ((const X *) obj);
  }

  virtual void *create_adapted_from_obj (const void *obj) const
  {
    return adaptor_type_info<X, Adapted>::create_adapted ((const X *) obj);
  }

  virtual void *clone (const void *other) const
  {
    return _clone<X, std::is_copy_constructible<X>::value>::call (other);
  }

  virtual void assign (void *dest, const void *src) const
  {
    _assign<X, std::is_copy_assignable<X>::value>::call (dest, src);
  }

  virtual bool can_destroy () const
  {
    return std::is_destructible<X>::value;
  }

  virtual bool can_copy () const
  {
    return std::is_copy_constructible<X>::value;
  }

  virtual bool can_default_create () const
  {
    return std::is_default_constructible<X>::value;
  }

  virtual const ClassBase *subclass_decl (const void *p) const 
  {
    if (p) {
      for (tl::weak_collection<ClassBase>::const_iterator s = subclasses ().begin (); s != subclasses ().end (); ++s) {
        if (s->can_upcast (p)) {
          return s->subclass_decl (p);
        }
      }
    }

    return this;
  }

  virtual bool can_upcast (const void *p) const 
  {
    return m_subclass_tester.get () && m_subclass_tester->can_upcast (p);
  }

  virtual bool binds () const
  {
    return true;
  }

  virtual const std::type_info &type () const
  {
    return typeid (X);
  }

  virtual const tl::VariantUserClassBase *var_cls_cls () const
  {
    return &m_var_cls_cls;
  }

  virtual const tl::VariantUserClassBase *var_cls (bool is_const) const
  {
    if (is_const) {
      return &m_var_cls_c;
    } else {
      return &m_var_cls;
    }
  }

private:
  gsi::VariantUserClass<X> m_var_cls;
  gsi::VariantUserClass<X> m_var_cls_c;
  gsi::VariantUserClass<X> m_var_cls_cls;
  std::unique_ptr<SubClassTesterBase> m_subclass_tester;
};

/**
 *  @brief The declaration of a class with a base class
 *
 *  This is an alternative to using the Class constructor with a base class reference. It
 *  employs a template parameter rather than a gsi::Class object. This way it's easier to 
 *  use if the declaration object of the base class is not available.
 */
template <class X, class B, class Adapted = NoAdaptorTag>
class SubClass
  : public Class<X, Adapted>
{
public:
  SubClass (const std::string &module, const std::string &name, const Methods &mm, const std::string &doc = std::string ())
    : Class<X, Adapted> (module, name, mm, doc)
  {
    //  .. nothing yet ..
  }

  SubClass (const std::string &module, const std::string &name, const std::string &doc = std::string ())
    : Class<X, Adapted> (module, name, doc)
  {
    //  .. nothing yet ..
  }

  virtual bool consolidate () const
  {
    //  TODO: ugly const_cast
    SubClass<X, B, Adapted> *non_const_this = const_cast<SubClass<X, B, Adapted> *> (this);
    non_const_this->set_base (cls_decl<B> ());
    return Class<X, Adapted>::consolidate ();
  }
};

/**
 *  @brief The declaration of a child class
 *
 *  This declaration is similar to the gsi::Class declaration but makes the given class
 *  a subclass of the parent.
 */
template <class P, class X, class Adapted = NoAdaptorTag>
class GSI_PUBLIC_TEMPLATE ChildClass
  : public Class<X, Adapted>
{
public:
  ChildClass (const std::string &module, const std::string &name, const Methods &mm, const std::string &doc = std::string ())
    : Class<X, Adapted> (module, name, mm, doc)
  {
    //  .. nothing yet ..
  }

  ChildClass (const std::string &module, const std::string &name, const std::string &doc = std::string ())
    : Class<X, Adapted> (module, name, doc)
  {
    //  .. nothing yet ..
  }

  virtual bool consolidate () const
  {
    //  TODO: ugly const cast
    ClassBase *non_const_pcls = const_cast<ClassBase *> (cls_decl<P> ());
    non_const_pcls->add_child_class (this);

    //  no longer required as it is a child now.
    return false;
  }
};

/**
 *  @brief The declaration of a subclass with a base class
 *
 *  This declaration is similar to the gsi::Class declaration but makes the given class
 *  a subclass of the parent.
 */
template <class P, class X, class B, class Adapted = NoAdaptorTag>
class GSI_PUBLIC_TEMPLATE ChildSubClass
  : public SubClass<X, B, Adapted>
{
public:
  ChildSubClass (const std::string &module, const std::string &name, const Methods &mm, const std::string &doc = std::string ())
    : SubClass<X, B, Adapted> (module, name, mm, doc)
  {
    //  .. nothing yet ..
  }

  ChildSubClass (const std::string &module, const std::string &name, const std::string &doc = std::string ())
    : SubClass<X, B, Adapted> (module, name, doc)
  {
    //  .. nothing yet ..
  }

  virtual bool consolidate () const
  {
    //  TODO: ugly const cast
    ClassBase *non_const_pcls = const_cast<ClassBase *> (cls_decl<P> ());
    non_const_pcls->add_child_class (this);

    return SubClass<X, B, Adapted>::consolidate ();
  }
};

/**
 *  @brief Produce a fallback class declaration
 *
 *  The main intention of this function is to provide a warning message
 *  for development.
 */
GSI_PUBLIC const ClassBase *fallback_cls_decl (const std::type_info &ti);

/**
 *  @brief Obtain the class declaration for a given class
 * 
 *  This method looks up the declaration object for a given type.
 *  It does so dynamically, since declarations may be located in different
 *  libraries. However, for performance reasons, the definitions are cached. 
 */
template <class X>
const ClassBase *cls_decl ()
{
  //  TODO: needs thread safety? It's rather unlikely that two threads enter this
  //  piece of code at the same time and they interfere when storing the results.
  static const ClassBase *cd = 0;
  if (! cd) {
    cd = class_by_typeinfo_no_assert (typeid (X));
    if (!cd) {
      cd = fallback_cls_decl (typeid (X));
    }
  }
  return cd;
}

}

#endif

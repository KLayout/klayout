
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


#ifndef _HDR_tlVariant
#define _HDR_tlVariant

#include "tlCommon.h"

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <typeinfo>

#include "tlInternational.h"
#include "tlAssert.h"
#include "tlObject.h"

#if defined(HAVE_QT)
# include <QString>
# include <QByteArray>
# include <QVariant>
#endif

namespace gsi
{
  class GSI_PUBLIC ClassBase;
  struct NoAdaptorTag;
  template <class T, class A> class GSI_PUBLIC_TEMPLATE Class;
  template <class X> const ClassBase *cls_decl ();
}

namespace tl
{

class Variant;
class Extractor;
class EvalClass;
template <class T> void extractor_impl (tl::Extractor &, T &);
template <class T> bool test_extractor_impl (tl::Extractor &, T &);

/**
 *  @brief A base class which describes a class, i.e. an object capable of converting and handling void *
 *
 *  There must be one particular class object per class. In particular the equality of the 
 *  class object pointer's must indicate that two object's share the same class.
 */
class TL_PUBLIC VariantUserClassBase
{
public:
  VariantUserClassBase () { }
  virtual ~VariantUserClassBase () { }

  virtual void *create () const = 0;
  virtual void destroy (void *) const = 0;
  virtual bool equal (const void *, const void *) const = 0;
  virtual bool less (const void *, const void *) const = 0;
  virtual void *clone (const void *) const = 0;
  virtual std::string to_string (const void *) const = 0;
  virtual int to_int (const void *) const = 0;
  virtual double to_double (const void *) const = 0;
  virtual void to_variant (const void *, tl::Variant &var) const = 0;
  virtual void read (void *, tl::Extractor &ex) const = 0;
  virtual const char *name () const = 0;
  virtual bool is_const () const = 0;
  virtual void assign (void *self, const void *other) const = 0;
  virtual const gsi::ClassBase *gsi_cls () const = 0;
  virtual const tl::EvalClass *eval_cls () const = 0;
  virtual void *deref_proxy (tl::Object *proxy) const = 0;

  const void *deref_proxy_const (const tl::Object *proxy) const
  {
    return deref_proxy (const_cast<tl::Object *> (proxy));
  }

  static std::string translate_class_name (const std::string &lc_clsname);
  static void clear_class_table ();
  static void register_user_class (const std::string &name, const VariantUserClassBase *cls);
  static const VariantUserClassBase *find_cls_by_name (const std::string &name);

protected:
  static const tl::VariantUserClassBase *instance (const std::type_info &type, bool is_const);
  static void register_instance (const tl::VariantUserClassBase *inst, const std::type_info &type, bool is_const);
  static void unregister_instance (const tl::VariantUserClassBase *inst, const std::type_info &type, bool is_const);
};

/**
 *  @brief A derived class encapsulating a certain user type 
 *
 *  We will employ RTTI to identify a type through that base class.
 */
template <class T>
class TL_PUBLIC_TEMPLATE VariantUserClass
  : public VariantUserClassBase
{
public:
  VariantUserClass () { }

  T *get (void *ptr) const { return reinterpret_cast<T *> (ptr); }
  const T *get (const void *ptr) const { return reinterpret_cast<const T *> (ptr); }

  static const tl::VariantUserClassBase *instance (bool is_const)
  {
    return VariantUserClassBase::instance (typeid (T), is_const);
  }

private:
  static const tl::VariantUserClassBase *ms_instances[4];

protected:
  void register_instance (const tl::VariantUserClassBase *inst, bool is_const)
  {
    VariantUserClassBase::register_instance (inst, typeid (T), is_const);
  }

  void unregister_instance (const tl::VariantUserClassBase *inst, bool is_const)
  {
    VariantUserClassBase::unregister_instance (inst, typeid (T), is_const);
  }
};

/**
 *   @brief A basic variant type
 *
 *   This variant is capable of storing long, double, std::string, void (nil) and lists
 *   of other variants.
 */
class TL_PUBLIC Variant
{
public:
  enum type { 
    t_nil, 
    t_bool, 
    t_char, 
    t_schar, 
    t_uchar, 
    t_short, 
    t_ushort, 
    t_int, 
    t_uint, 
    t_long, 
    t_ulong, 
    t_longlong, 
    t_ulonglong, 
#if defined(HAVE_64BIT_COORD)
    t_int128, 
#endif
    t_id, 
    t_float, 
    t_double, 
    t_string, 
    t_stdstring, 
    t_bytearray,
#if defined(HAVE_QT)
    t_qstring,
    t_qbytearray, 
#endif
    t_list, 
    t_array, 
    t_user,
    t_user_ref
  };

  typedef std::vector<tl::Variant>::const_iterator const_iterator;
  typedef std::vector<tl::Variant>::iterator iterator;
  typedef std::map<tl::Variant, tl::Variant> array_type;
  typedef array_type::const_iterator const_array_iterator;
  typedef array_type::iterator array_iterator;

  /**
   *  @brief Initialize the Variant with "nil"
   */
  Variant ();

  /**
   *  @brief Copy ctor
   */
  Variant (const tl::Variant &d);

  /**
   *  @brief Initialize the Variant with a std::vector<char>
   */
  Variant (const std::vector<char> &s);

#if defined(HAVE_QT)
  /**
   *  @brief Initialize the Variant with a QByteArray
   */
  Variant (const QByteArray &s);

  /**
   *  @brief Initialize the Variant with a QString
   */
  Variant (const QString &s);

  /**
   *  @brief Create from a QVariant
   *
   *  This constructor will convert a QVariant into a tl::Variant as far as possible.
   */
  explicit Variant (const QVariant &v);
#endif

  /**
   *  @brief Initialize the Variant with "string"
   */
  Variant (const std::string &s);

  /**
   *  @brief Initialize the Variant with "string"
   */
  Variant (const char *s);

  /**
   *  @brief Initialize the Variant with "double"
   */
  Variant (double d);

  /**
   *  @brief Initialize the Variant with "float"
   */
  Variant (float d);

  /**
   *  @brief Initialize the Variant with "char"
   */
  Variant (char c);

  /**
   *  @brief Initialize the Variant with "signed char"
   */
  Variant (signed char c);

  /**
   *  @brief Initialize the Variant with "unsigned char"
   */
  Variant (unsigned char c);

  /**
   *  @brief Initialize the Variant with "short"
   */
  Variant (short s);

  /**
   *  @brief Initialize the Variant with "unsigned short"
   */
  Variant (unsigned short s);

  /**
   *  @brief Initialize the Variant with "bool"
   */
  Variant (bool l);

  /**
   *  @brief Initialize the Variant with "int" (actually "long")
   */
  Variant (int l);

  /**
   *  @brief Initialize the Variant with "unsigned int" (actually "unsigned long")
   */
  Variant (unsigned int l);

  /**
   *  @brief Initialize the Variant with "long"
   */
  Variant (long l);

  /**
   *  @brief Initialize the Variant with "unsigned long"
   */
  Variant (unsigned long l);
 
  /**
   *  @brief Initialize the Variant with "long long"
   */
  Variant (long long l);

  /**
   *  @brief Initialize the Variant with "unsigned long long"
   */
  Variant (unsigned long long l);
 
#if defined(HAVE_64BIT_COORD)
  /**
   *  @brief Initialize the Variant with "__int128"
   */
  Variant (__int128 l);
#endif

  /**
   *  @brief Initialize the Variant with an "id" 
   *
   *  The "id" type is basically a size_t, but is supposed to be used as a representative for another value.
   *  One application for that type is a placeholder for an OASIS name until it is associated with a real value.
   */
  Variant (size_t l, bool /*dummy*/);

  /**
   *  @brief Initialize with a user type based on void *
   *
   *  The Variant will take over the ownership over the user object.
   */
  Variant (void *object, const VariantUserClassBase *cls, bool shared)
    : m_type (t_user), m_string (0)
  {
    m_var.mp_user.object = object;
    m_var.mp_user.shared = shared;
    m_var.mp_user.cls = cls;
  }

  /**
   *  @brief Initialize with a user type based on tl::Object
   *
   *  If shared is true, the variant will use a shared pointer to manage the ownership
   *  of the object (i.e. if the object is new'd it will be deleted by the variant). If
   *  shared is false, a weak pointer will be employed that watches the object.
   */
  Variant (tl::Object *object, const VariantUserClassBase *cls, bool shared)
    : m_type (t_user_ref), m_string (0)
  {
    new (m_var.mp_user_ref.ptr) WeakOrSharedPtr (object, shared);
    m_var.mp_user_ref.cls = cls;
  }

  /**
   *  @brief Initialize with a user type (will always create a deep copy)
   */
  template <class T>
  Variant (const T &obj)
    : m_type (t_user), m_string (0)
  {
    const tl::VariantUserClassBase *c = tl::VariantUserClass<T>::instance (false);
    tl_assert (c != 0);
    m_var.mp_user.object = new T (obj);
    m_var.mp_user.shared = true;
    m_var.mp_user.cls = c;
  }

  /**
   *  @brief Initialize the Variant with an explicit vector or variants
   */
  Variant (const std::vector<tl::Variant> &list)
    : m_type (t_list), m_string (0)
  {
    m_var.m_list = new std::vector<tl::Variant> (list);
  }

  /**
   *  @brief Initialize the Variant with a list
   */
  template <class Iter>
  Variant (Iter from, Iter to)
    : m_type (t_list), m_string (0)
  {
    m_var.m_list = new std::vector<tl::Variant> (from, to);
  }

  /**
   *  @brief Destructor
   */
  ~Variant ();

  /**
   *  @brief Utility: initialize a variant from GSI type reference
   */
  template <class T>
  static tl::Variant make_variant_ref (T *t)
  {
    const tl::VariantUserClassBase *c = gsi::cls_decl<T> ()->var_cls (false);
    tl_assert (c != 0);
    return tl::Variant ((void *) t, c, false);
  }

  /**
   *  @brief Utility: initialize a variant from GSI type reference
   */
  template <class T>
  static tl::Variant make_variant_ref (const T *t)
  {
    const tl::VariantUserClassBase *c = gsi::cls_decl<T> ()->var_cls (true);
    tl_assert (c != 0);
    return tl::Variant ((void *) t, c, false);
  }

  /**
   *  @brief Utility: initialize a variant from GSI type (take over ownership)
   */
  template <class T>
  static tl::Variant make_variant (T *t)
  {
    const tl::VariantUserClassBase *c = gsi::cls_decl<T> ()->var_cls (false);
    tl_assert (c != 0);
    return tl::Variant ((void *) t, c, true);
  }

  /**
   *  @brief Utility: initialize a variant from GSI type (deep copy)
   */
  template <class T>
  static tl::Variant make_variant (const T &t, bool is_const = false)
  {
    const tl::VariantUserClassBase *c = gsi::cls_decl <T> ()->var_cls (is_const);
    tl_assert (c != 0);
    return tl::Variant ((void *) new T(t), c, true);
  }

#if defined(HAVE_QT)
  /**
   *  @brief Convert to a QVariant
   */
  QVariant to_qvariant () const;
#endif

  /**
   *  @brief Assignment
   */
  Variant &operator= (const Variant &v);

  /**
   *  @brief Assignment of a string
   */
  Variant &operator= (const char *v);

#if defined(HAVE_QT)
  /**
   *  @brief Assignment of a QByteArray
   */
  Variant &operator= (const QByteArray &v);

  /**
   *  @brief Assignment of a QString
   */
  Variant &operator= (const QString &v);
#endif

  /**
   *  @brief Assignment of a string
   */
  Variant &operator= (const std::string &v);

  /**
   *  @brief Assignment of a STL byte array
   */
  Variant &operator= (const std::vector<char> &v);

  /**
   *  @brief Assignment of a double
   */
  Variant &operator= (double d);

  /**
   *  @brief Assignment of a float
   */
  Variant &operator= (float d);

  /**
   *  @brief Assignment of a bool
   */
  Variant &operator= (bool l);

  /**
   *  @brief Assignment of a char
   */
  Variant &operator= (char l);

  /**
   *  @brief Assignment of an unsigned char
   */
  Variant &operator= (unsigned char c);

  /**
   *  @brief Assignment of a signed char
   */
  Variant &operator= (signed char c);

  /**
   *  @brief Assignment of a short
   */
  Variant &operator= (short s);

  /**
   *  @brief Assignment of an unsigned char
   */
  Variant &operator= (unsigned short s);

  /**
   *  @brief Assignment of a int
   */
  Variant &operator= (int l);

  /**
   *  @brief Assignment of a unsigned int
   */
  Variant &operator= (unsigned int l);

  /**
   *  @brief Assignment of a long
   */
  Variant &operator= (long l);

  /**
   *  @brief Assignment of a unsigned long
   */
  Variant &operator= (unsigned long l);

  /**
   *  @brief Assignment of a long long
   */
  Variant &operator= (long long l);

  /**
   *  @brief Assignment of a unsigned long long
   */
  Variant &operator= (unsigned long long l);

#if defined(HAVE_64BIT_COORD)
  /**
   *  @brief Assignment of a int128
   */
  Variant &operator= (__int128 l);
#endif

  /**
   *  @brief Reset to nil
   */
  void reset ();

  /**
   *  @brief Initialize with a user type
   */
  void set_user (void *object, const VariantUserClassBase *cls, bool shared)
  {
    reset ();
    m_type = t_user;
    m_var.mp_user.object = object;
    m_var.mp_user.shared = shared;
    m_var.mp_user.cls = cls;
  }

  /**
   *  @brief Initialize with a user type
   */
  void set_user_ref (tl::Object *obj, const VariantUserClassBase *cls, bool shared)
  {
    reset ();
    m_type = t_user_ref;
    new (m_var.mp_user_ref.ptr) tl::WeakOrSharedPtr (obj, shared);
    m_var.mp_user_ref.cls = cls;
  }

  /**
   *  @brief Initialize with an empty list with the given reserve
   */
  void set_list (size_t reserve = 0)
  {
    reset ();
    m_type = t_list;
    m_var.m_list = new std::vector<tl::Variant> ();
    if (reserve > 0) {
      m_var.m_list->reserve (reserve);
    }
  }

  /**
   *  @brief Initialize with an empty array
   */
  void set_array ()
  {
    reset ();
    m_type = t_array;
    m_var.m_array = new std::map<tl::Variant, tl::Variant> ();
  }

  /**  
   *  @brief Equality
   *
   *  For user types, this is not implemented yet.
   */
  bool operator== (const Variant &d) const;

  /**  
   *  @brief Inequality
   *
   *  For user types, this is not implemented yet.
   */
  bool operator!= (const Variant &d) const
  {
    return !operator== (d);
  }

  /**  
   *  @brief Comparison
   *
   *  For user types, this is not implemented yet.
   */
  bool operator< (const Variant &d) const;

  /**
   *  @brief Conversion to a string
   *
   *  This performs the conversion to a string as far as possible.
   *  No conversion is provided to user types currently.
   */
  const char *to_string () const;

#if defined(HAVE_QT)
  /**
   *  @brief Conversion to a QByteArray
   *
   *  This performs the conversion to a QByteArray as far as possible.
   *  No conversion is provided to user types currently.
   */
  QByteArray to_qbytearray () const;

  /**
   *  @brief Conversion to a QString
   *
   *  This performs the conversion to a QString as far as possible.
   *  No conversion is provided to user types currently.
   */
  QString to_qstring () const;
#endif

  /**
   *  @brief Conversion to a STL byte array
   *
   *  This performs the conversion to a std::vector<char> as far as possible.
   *  No conversion is provided to user types currently.
   */
  std::vector<char> to_bytearray () const;

  /**
   *  @brief Conversion to a std::string
   *
   *  This performs the conversion to a QString as far as possible.
   *  No conversion is provided to user types currently.
   */
  std::string to_stdstring () const;

  /**
   *  @brief Conversion to a unsigned long long
   *
   *  This performs the conversion to a unsigned long long as far as possible.
   *  No conversion is provided to user types currently.
   */
  unsigned long long to_ulonglong () const;

  /**
   *  @brief Conversion to a long long
   *
   *  This performs the conversion to a long long as far as possible.
   *  No conversion is provided to user types currently.
   */
  long long to_longlong () const;

#if defined(HAVE_64BIT_COORD)
  /**
   *  @brief Conversion to an int128
   */
  __int128 to_int128 () const;
#endif

  /**
   *  @brief Conversion to a unsigned int
   *
   *  This performs the conversion to a unsigned int as far as possible.
   *  No conversion is provided to user types currently.
   */
  unsigned int to_uint () const;

  /**
   *  @brief Conversion to a int
   *
   *  This performs the conversion to a int as far as possible.
   *  No conversion is provided to user types currently.
   */
  int to_int () const;

  /**
   *  @brief Conversion to a unsigned long
   *
   *  This performs the conversion to a unsigned long as far as possible.
   *  No conversion is provided to user types currently.
   */
  unsigned long to_ulong () const;

  /**
   *  @brief Conversion to a long
   *
   *  This performs the conversion to a long as far as possible.
   *  No conversion is provided to user types currently.
   */
  long to_long () const;

  /**
   *  @brief Conversion to a unsigned long
   *
   *  This performs the conversion to a unsigned short as far as possible.
   *  No conversion is provided to user types currently.
   */
  unsigned short to_ushort () const;

  /**
   *  @brief Conversion to a short
   *
   *  This performs the conversion to a short as far as possible.
   *  No conversion is provided to user types currently.
   */
  short to_short () const;

  /**
   *  @brief Conversion to a signed char
   *
   *  This performs the conversion to a signed char as far as possible.
   *  No conversion is provided to user types currently.
   */
  signed char to_schar () const;

  /**
   *  @brief Conversion to a unsigned char
   *
   *  This performs the conversion to an unsigned char as far as possible.
   *  No conversion is provided to user types currently.
   */
  unsigned char to_uchar () const;

  /**
   *  @brief Conversion to a char
   *
   *  This performs the conversion to a char as far as possible.
   *  No conversion is provided to user types currently.
   */
  char to_char () const;

  /**
   *  @brief Conversion to an id
   *
   *  This gets the id value if the variant is an id.
   *  No conversion is provided to user types currently.
   */
  size_t to_id () const;

  /**
   *  @brief Conversion to a bool
   *
   *  This performs the conversion to a bool as far as possible.
   *  No conversion is provided to user types currently.
   */
  bool to_bool () const;

  /**
   *  @brief Conversion to a double
   *
   *  This performs the conversion to a double value as far as possible.
   *  No conversion is provided to user types currently.
   */
  double to_double () const;

  /**
   *  @brief Conversion to a float
   *
   *  This performs the conversion to a float value as far as possible.
   *  No conversion is provided to user types currently.
   */
  float to_float () const;

  /**
   *  @brief conversion to a standard type 
   *
   *  This is a templatized version of the various to_... methods. This
   *  does also not include conversion to a user type.
   *  This is the generic version. Specializations follow.
   */
  template <class T>
  T to () const
  {
    tl_assert (false);
  }

  /**
   *  @brief Converts to the user object (const)
   */
  const void *to_user () const
  {
    if (m_type == t_user) {
      return m_var.mp_user.object;
    } else if (m_type == t_user_ref) {
      return m_var.mp_user_ref.cls->deref_proxy_const (reinterpret_cast<const WeakOrSharedPtr *> (m_var.mp_user_ref.ptr)->get ());
    } else {
      return 0;
    }
  }

  /**
   *  @brief Converts to the user object
   */
  void *to_user ()
  {
    if (m_type == t_user) {
      return m_var.mp_user.object;
    } else if (m_type == t_user_ref) {
      return m_var.mp_user_ref.cls->deref_proxy (reinterpret_cast<WeakOrSharedPtr *> (m_var.mp_user_ref.ptr)->get ());
    } else {
      return 0;
    }
  }

  /**
   *  @brief Converts to a tl::Object (const)
   */
  const tl::Object *to_object () const
  {
    if (m_type == t_user_ref) {
      return reinterpret_cast<const WeakOrSharedPtr *> (m_var.mp_user_ref.ptr)->get ();
    } else {
      return 0;
    }
  }

  /**
   *  @brief Converts to the user object
   */
  tl::Object *to_object ()
  {
    return const_cast<tl::Object *> (((const tl::Variant *) this)->to_object ());
  }

  /**
   *  @brief Gets the user object's class
   */
  const VariantUserClassBase *user_cls () const
  {
    if (m_type == t_user) {
      return m_var.mp_user.cls;
    } else if (m_type == t_user_ref) {
      return m_var.mp_user_ref.cls;
    } else {
      return 0;
    }
  }

  /**
   *  @brief Gets the GSI class if the variant is a user object
   */
  const gsi::ClassBase *gsi_cls () const
  {
    return user_cls () ? user_cls ()->gsi_cls () : 0;
  }

  /**
   *  @brief Returns a value indicating whether the user object is a const reference or object
   */
  bool user_is_const () const;

  /**
   *  @brief Returns a value indicating whether the user object is a reference
   *  References do not own the object and upon destruction of the tl::Variant the object
   *  is not deleted.
   */
  bool user_is_ref () const;

  /**
   *  @brief Deletes the user object
   *  An object can only be deleted if it is owned by the variant, i.e. user_is_ref is false.
   */
  void user_destroy ();

  /**
   *  @brief Takes the user object and releases it from the variant
   */
  void *user_take ();

  /**
   *  @brief Assigns the object stored in other to self
   *
   *  "other" needs to be a user object and the class of "other" needs to be same as self.
   */
  void user_assign (const tl::Variant &other);

  /**
   *  @brief Creates a clone of the current object
   */
  tl::Variant user_dup () const;

  /** 
   *  @brief Convert to the given user type (const version)
   */
  template <class T> 
  const T &to_user () const
  {
    if (is_user()) {
      const VariantUserClass<T> *tcls = dynamic_cast<const VariantUserClass<T> *> (user_cls ());
      tl_assert (tcls != 0);
      const T *t = tcls->get (to_user ());
      tl_assert (t);
      return *t;
    } else {
      tl_assert (false);
    }
  }

  /**
   *  @brief Convert to the given user type
   */
  template <class T>
  T &to_user ()
  {
    return const_cast<T &> (((const Variant *) this)->to_user<T> ());
  }

  /**
   *  @brief Morph to the given type
   *
   *  After morphing the variant, the variant will use the given type internally.
   *  The native pointer can be used to access the value then.
   *  A nil value is not morphed and remains nil. In that case, the native pointer will be 0.
   */
  template<class T>
  tl::Variant &morph ()
  {
    if (! is_nil ()) {
      *this = to<T> ();
    }
    return *this;
  }

  /**
   *  @brief Cast to the given type
   *
   *  This creates a new variant which uses the given type internally
   */
  template <class T>
  Variant cast () const
  {
    return Variant (to<T> ());
  }

  /**
   *  @brief Access the native (internal) object 
   *
   *  For nil, 0 is returned.
   */
  void *native_ptr ()
  {
    //  saves one implementation ...
    return const_cast<void *> (((const tl::Variant *) this)->native_ptr ());
  }

  /**
   *  @brief Access the native (internal) object 
   *
   *  For nil, 0 is returned.
   */
  const void *native_ptr () const;

  /**
   *  @brief Get the list iterators, if it is one
   */
  const_iterator begin () const
  {
    tl_assert (m_type == t_list);
    return m_var.m_list->begin ();
  }

  /**
   *  @brief Get the list iterators, if it is one
   */
  const_iterator end () const
  {
    tl_assert (m_type == t_list);
    return m_var.m_list->end ();
  }

  /**
   *  @brief Get the list iterators, if it is one
   */
  iterator begin () 
  {
    tl_assert (m_type == t_list);
    return m_var.m_list->begin ();
  }

  /**
   *  @brief Get the list iterators, if it is one
   */
  iterator end () 
  {
    tl_assert (m_type == t_list);
    return m_var.m_list->end ();
  }

  /**
   *  @brief Reserve some length for a list
   */ 
  void reserve (size_t n) 
  {
    tl_assert (m_type == t_list);
    m_var.m_list->reserve (n);
  }

  /**
   *  @brief Get the length of the list if there is one, otherwise 0
   */ 
  size_t size () const
  {
    return m_type == t_list ? m_var.m_list->size () : 0;
  }

  /**
   *  @brief Add a element to the list
   */
  void push (const tl::Variant &v)
  {
    tl_assert (m_type == t_list);
    m_var.m_list->push_back (v);
  }

  /**
   *  @brief Get the back element of the list
   */
  tl::Variant &back ()
  {
    tl_assert (m_type == t_list);
    return m_var.m_list->back ();
  }

  /**
   *  @brief Get the back element of the list (const)
   */
  const tl::Variant &back () const
  {
    tl_assert (m_type == t_list);
    return m_var.m_list->back ();
  }

  /**
   *  @brief Get the front element of the list
   */
  tl::Variant &front ()
  {
    tl_assert (m_type == t_list);
    return m_var.m_list->front ();
  }

  /**
   *  @brief Get the front element of the list (const)
   */
  const tl::Variant &front () const
  {
    tl_assert (m_type == t_list);
    return m_var.m_list->front ();
  }

  /**
   *  @brief Get the list, if it is one
   */
  std::vector<tl::Variant> &get_list ()
  {
    tl_assert (m_type == t_list);
    return *m_var.m_list;
  }

  /**
   *  @brief Get the list, if it is one (const)
   */
  const std::vector<tl::Variant> &get_list () const
  {
    tl_assert (m_type == t_list);
    return *m_var.m_list;
  }

  /**
   *  @brief Get the array iterators, if it is one
   */
  const_array_iterator begin_array () const
  {
    tl_assert (m_type == t_array);
    return m_var.m_array->begin ();
  }

  /**
   *  @brief Get the array iterators, if it is one
   */
  const_array_iterator end_array () const
  {
    tl_assert (m_type == t_array);
    return m_var.m_array->end ();
  }

  /**
   *  @brief Get the array iterators, if it is one
   */
  array_iterator begin_array () 
  {
    tl_assert (m_type == t_array);
    return m_var.m_array->begin ();
  }

  /**
   *  @brief Get the array iterators, if it is one
   */
  array_iterator end_array () 
  {
    tl_assert (m_type == t_array);
    return m_var.m_array->end ();
  }

  /**
   *  @brief Get the length of the array if there is one, otherwise 0
   */ 
  size_t array_size () const
  {
    return m_type == t_array ? m_var.m_array->size () : 0;
  }

  /**
   *  @brief Insert an element into the array
   */
  void insert (const tl::Variant &k, const tl::Variant &v)
  {
    tl_assert (m_type == t_array);
    m_var.m_array->insert (std::make_pair (k, v));
  }

  /**
   *  @brief Returns the value for the given key or 0 if the variant is not an array or does not contain the key
   */
  tl::Variant *find (const tl::Variant &k);

  /**
   *  @brief Returns the value for the given key or 0 if the variant is not an array or does not contain the key
   */
  const tl::Variant *find (const tl::Variant &k) const;

  /**
   *  @brief Get the list, if it is one
   */
  array_type &get_array ()
  {
    tl_assert (m_type == t_array);
    return *m_var.m_array;
  }

  /**
   *  @brief Get the list, if it is one (const)
   */
  const array_type &get_array () const
  {
    tl_assert (m_type == t_array);
    return *m_var.m_array;
  }

  /**
   *  @brief Test, if it can convert to a double
   *
   *  All numeric types can convert to double. That is double and the integer types.
   */
  bool can_convert_to_double () const;

  /**
   *  @brief Test, if it can convert to a float
   *
   *  All numeric types can convert to float. That is double and the integer types unless the double value is outside the float range.
   */
  bool can_convert_to_float () const;

  /**
   *  @brief Test, if it can convert to a char
   *
   *  All numeric types can convert to char unless the value is outside the allowed range.
   */
  bool can_convert_to_char () const;

  /**
   *  @brief Test, if it can convert to an signed char
   *
   *  All numeric types can convert to signed char unless the value is outside the allowed range.
   */
  bool can_convert_to_schar () const;

  /**
   *  @brief Test, if it can convert to an unsigned char
   *
   *  All numeric types can convert to unsigned char unless the value is outside the allowed range.
   */
  bool can_convert_to_uchar () const;

  /**
   *  @brief Test, if it can convert to a short
   *
   *  All numeric types can convert to short unless the value is outside the allowed range.
   */
  bool can_convert_to_short () const;

  /**
   *  @brief Test, if it can convert to an unsigned short
   *
   *  All numeric types can convert to unsigned short unless the value is outside the allowed range.
   */
  bool can_convert_to_ushort () const;

  /**
   *  @brief Test, if it can convert to an int
   *
   *  All numeric types can convert to int unless the value is outside the allowed range.
   */
  bool can_convert_to_int () const;

  /**
   *  @brief Test, if it can convert to an unsigned int
   *
   *  All numeric types can convert to unsigned int unless the value is outside the allowed range.
   */
  bool can_convert_to_uint () const;

  /**
   *  @brief Test, if it can convert to a long
   *
   *  All numeric types can convert to long unless the value is outside the allowed range.
   */
  bool can_convert_to_long () const;

  /**
   *  @brief Test, if it can convert to an unsigned long
   *
   *  All numeric types can convert to unsigned long unless the value is outside the allowed range.
   */
  bool can_convert_to_ulong () const;

  /**
   *  @brief Test, if it can convert to a long long
   *
   *  All numeric types can convert to long unless the value is outside the allowed range.
   */
  bool can_convert_to_longlong () const;

  /**
   *  @brief Test, if it can convert to an unsigned long long
   *
   *  All numeric types can convert to unsigned long unless the value is outside the allowed range.
   */
  bool can_convert_to_ulonglong () const;

#if defined(HAVE_64BIT_COORD)
  /**
   *  @brief Test, if it can convert to an int128
   */
  bool can_convert_to_int128 () const;
#endif

  /**
   *  @brief Returns true if the conversion to the given type is possible
   *
   *  This is a templatized version of the various can_convert_to_... methods. This
   *  does not include conversion to a user type, arrays or lists.
   *  This is the generic version. Specializations follow.
   */
  template <class T>
  bool can_convert_to () const
  {
    return false;
  }

  /**
   *  @brief Test, if it is a double or can be converted to a double
   */
  bool is_double () const
  {
    return m_type == t_double || m_type == t_float;
  }

  /**
   *  @brief Test, if it is a char 
   */
  bool is_char () const
  {
    return m_type == t_char;
  }

  /**
   *  @brief Test, if it is a long or can be converted to a long
   */
  bool is_long () const
  {
    return m_type == t_long || m_type == t_int || m_type == t_short || m_type == t_schar;
  }

  /**
   *  @brief Test, if it is an unsigned long or can be converted into one
   */
  bool is_ulong () const
  {
    return m_type == t_ulong || m_type == t_uint || m_type == t_ushort || m_type == t_uchar;
  }

  /**
   *  @brief Test, if it is a long long
   */
  bool is_longlong () const
  {
    return m_type == t_longlong;
  }

  /**
   *  @brief Test, if it is a unsigned long long
   */
  bool is_ulonglong () const
  {
    return m_type == t_ulonglong;
  }

#if defined(HAVE_64BIT_COORD)
  /**
   *  @brief Test, if it is an int128
   */
  bool is_int128 () const
  {
    return m_type == t_int128;
  }
#endif

  /**
   *  @brief Test, if it is a bool
   */
  bool is_bool () const
  {
    return m_type == t_bool;
  }

  /**
   *  @brief Test, if it is a id
   */
  bool is_id () const
  {
    return m_type == t_id;
  }

  /**
   *  @brief Test, if it is a std::vector<char> byte array
   */
  bool is_bytearray () const
  {
    return m_type == t_bytearray;
  }

#if defined(HAVE_QT)

  /**
   *  @brief Test, if it is a QByteArray
   */
  bool is_qbytearray () const
  {
    return m_type == t_qbytearray;
  }

  /**
   *  @brief Test, if it is a QString
   */
  bool is_qstring () const
  {
    return m_type == t_qstring;
  }

#endif

  /**
   *  @brief Test, if it is a std::string
   */
  bool is_stdstring () const
  {
    return m_type == t_stdstring;
  }

  /**
   *  @brief Test, if it is a "C" string
   */
  bool is_cstring () const
  {
    return m_type == t_string;
  }

  /**
   *  @brief Test, if it is any string
   */
  bool is_a_string () const
  {
#if defined(HAVE_QT)
    return m_type == t_string || m_type == t_stdstring || m_type == t_qstring;
#else
    return m_type == t_string || m_type == t_stdstring;
#endif
  }

  /**
   *  @brief Test, if it is a byte array
   */
  bool is_a_bytearray () const
  {
#if defined(HAVE_QT)
    return m_type == t_bytearray || m_type == t_qbytearray;
#else
    return m_type == t_bytearray;
#endif
  }

  /**
   *  @brief Returns true if the variant is of the given type internally
   *
   *  This is a templatized version of the various can_convert_to_... methods. This
   *  does not include conversion to a user type, arrays or lists.
   *  This is the generic version. Specializations follow.
   */
  template <class T>
  bool is () const
  {
    return false;
  }

  /**
   *  @brief Test, if it is nil
   */
  bool is_nil () const
  {
    return m_type == t_nil;
  }

  /**
   *  @brief Test, if it is an array
   */
  bool is_array () const
  {
    return m_type == t_array;
  }

  /**
   *  @brief Test, if it is a list
   */
  bool is_list () const
  {
    return m_type == t_list;
  }

  /**
   *  @brief Get the type code
   */
  type type_code () const
  {
    return m_type;
  }

  /**
   *  @brief Test, if this is a user type
   */
  bool is_user () const
  {
    return m_type == t_user || m_type == t_user_ref;
  }

  /** 
   *  @brief Test, if this is a user type and can convert to the given type
   */
  template <class T>
  bool is_user () const
  {
    if (m_type == t_user) {
      const VariantUserClass<T> *tcls = dynamic_cast<const VariantUserClass<T> *> (m_var.mp_user.cls);
      return tcls != 0;
    } else if (m_type == t_user_ref) {
      const VariantUserClass<T> *tcls = dynamic_cast<const VariantUserClass<T> *> (m_var.mp_user_ref.cls);
      return tcls != 0;
    } else {
      return false;
    }
  }

  /**
   *  @brief Test, if this is a user type of tl::Object class
   */
  bool is_object () const
  {
    return m_type == t_user_ref;
  }

  /**
   *  @brief Test, if this is a user type and can convert to the given type
   */
  template <class T>
  bool is_object () const
  {
    if (m_type == t_user_ref) {
      const VariantUserClass<T> *tcls = dynamic_cast<const VariantUserClass<T> *> (m_var.mp_user_ref.cls);
      return tcls != 0;
    } else {
      return false;
    }
  }

  /**
   *  @brief Swap contents with another instance
   */
  void swap (tl::Variant &other);

  /**
   *  @brief A method to deliver an empty-list variant
   */
  static tl::Variant empty_list ();

  /**
   *  @brief A method to deliver an empty-array variant
   */
  static tl::Variant empty_array ();

  /**
   *  @brief Convert the Variant to a string that can be parsed with the Extractor
   *
   *  No conversion is provided for user types and "nil" currently.
   */
  std::string to_parsable_string () const;

private:
  type m_type;

  union ValueHolder {
    std::vector<tl::Variant> *m_list;
    std::map<tl::Variant, tl::Variant> *m_array;
    double m_double;
    float m_float;
    char m_char;
    unsigned char m_uchar;
    signed char m_schar;
    short m_short;
    unsigned short m_ushort;
    int m_int;
    unsigned int m_uint;
    long m_long;
    unsigned long m_ulong;
    long long m_longlong;
    unsigned long long m_ulonglong;
#if defined(HAVE_64BIT_COORD)
    __int128 m_int128;
#endif
    bool m_bool;
    size_t m_id;
    struct {
      void *object;
      bool shared;
      const VariantUserClassBase *cls;
    } mp_user;
    struct {
      char ptr [sizeof (WeakOrSharedPtr)];
      const VariantUserClassBase *cls;
    } mp_user_ref;
#if defined(HAVE_QT)
    QString *m_qstring;
    QByteArray *m_qbytearray;
#endif
    std::vector<char> *m_bytearray;
    std::string *m_stdstring;
  } m_var;

  //  this will hold the string if it is valid
  mutable char *m_string;

  void set_user_object (void *obj, bool shared);
};

//  specializations of the to ... methods
template<> inline bool Variant::to<bool> () const                                 { return to_bool (); }
template<> inline char Variant::to<char> () const                                 { return to_char (); }
template<> inline unsigned char Variant::to<unsigned char> () const               { return to_uchar (); }
template<> inline signed char Variant::to<signed char> () const                   { return to_schar (); }
template<> inline short Variant::to<short> () const                               { return to_short (); }
template<> inline unsigned short Variant::to<unsigned short> () const             { return to_ushort (); }
template<> inline int Variant::to<int> () const                                   { return to_int (); }
template<> inline unsigned int Variant::to<unsigned int> () const                 { return to_uint (); }
template<> inline long Variant::to<long> () const                                 { return to_long (); }
template<> inline unsigned long Variant::to<unsigned long> () const               { return to_ulong (); }
template<> inline long long Variant::to<long long> () const                       { return to_longlong (); }
template<> inline unsigned long long Variant::to<unsigned long long> () const     { return to_ulonglong (); }
#if defined(HAVE_64BIT_COORD)
template<> inline __int128 Variant::to<__int128> () const                         { return to_int128 (); }
#endif
template<> inline double Variant::to<double> () const                             { return to_double (); }
template<> inline float Variant::to<float> () const                               { return to_float (); }
template<> inline std::string Variant::to<std::string> () const                   { return to_stdstring (); }
template<> inline std::vector<char> Variant::to<std::vector<char> > () const      { return to_bytearray (); }
#if defined(HAVE_QT)
template<> inline QString Variant::to<QString> () const                           { return to_qstring (); }
template<> inline QByteArray Variant::to<QByteArray> () const                     { return to_qbytearray (); }
#endif
template<> inline const char *Variant::to<const char *> () const                  { return to_string (); }

//  specializations if the is.. methods
template<> inline bool Variant::is<bool> () const                 { return m_type == t_bool; }
template<> inline bool Variant::is<char> () const                 { return m_type == t_char; }
template<> inline bool Variant::is<unsigned char> () const        { return m_type == t_uchar; }
template<> inline bool Variant::is<signed char> () const          { return m_type == t_schar; }
template<> inline bool Variant::is<short> () const                { return m_type == t_short; }
template<> inline bool Variant::is<unsigned short> () const       { return m_type == t_ushort; }
template<> inline bool Variant::is<int> () const                  { return m_type == t_int; }
template<> inline bool Variant::is<unsigned int> () const         { return m_type == t_uint; }
template<> inline bool Variant::is<long> () const                 { return m_type == t_long; }
template<> inline bool Variant::is<unsigned long> () const        { return m_type == t_ulong; }
template<> inline bool Variant::is<long long> () const            { return m_type == t_longlong; }
template<> inline bool Variant::is<unsigned long long> () const   { return m_type == t_ulonglong; }
#if defined(HAVE_64BIT_COORD)
template<> inline bool Variant::is<__int128> () const             { return m_type == t_int128; }
#endif
template<> inline bool Variant::is<double> () const               { return m_type == t_double; }
template<> inline bool Variant::is<float> () const                { return m_type == t_float; }
template<> inline bool Variant::is<std::string> () const          { return m_type == t_stdstring; }
template<> inline bool Variant::is<std::vector<char> > () const   { return m_type == t_bytearray; }
#if defined(HAVE_QT)
template<> inline bool Variant::is<QString> () const              { return m_type == t_qstring; }
template<> inline bool Variant::is<QByteArray> () const           { return m_type == t_qbytearray; }
#endif
template<> inline bool Variant::is<const char *> () const         { return m_type == t_string; }

//  specializations of the can_convert.. methods
template<> inline bool Variant::can_convert_to<bool> () const                 { return true; }
template<> inline bool Variant::can_convert_to<char> () const                 { return can_convert_to_char (); }
template<> inline bool Variant::can_convert_to<unsigned char> () const        { return can_convert_to_uchar (); }
template<> inline bool Variant::can_convert_to<signed char> () const          { return can_convert_to_schar (); }
template<> inline bool Variant::can_convert_to<short> () const                { return can_convert_to_short (); }
template<> inline bool Variant::can_convert_to<unsigned short> () const       { return can_convert_to_ushort (); }
template<> inline bool Variant::can_convert_to<int> () const                  { return can_convert_to_int (); }
template<> inline bool Variant::can_convert_to<unsigned int> () const         { return can_convert_to_uint (); }
template<> inline bool Variant::can_convert_to<long> () const                 { return can_convert_to_long (); }
template<> inline bool Variant::can_convert_to<unsigned long> () const        { return can_convert_to_ulong (); }
template<> inline bool Variant::can_convert_to<long long> () const            { return can_convert_to_longlong (); }
template<> inline bool Variant::can_convert_to<unsigned long long> () const   { return can_convert_to_ulonglong (); }
#if defined(HAVE_64BIT_COORD)
template<> inline bool Variant::can_convert_to<__int128> () const             { return can_convert_to_int128 (); }
#endif
template<> inline bool Variant::can_convert_to<double> () const               { return can_convert_to_double (); }
template<> inline bool Variant::can_convert_to<float> () const                { return can_convert_to_float (); }
template<> inline bool Variant::can_convert_to<std::string> () const          { return true; }
template<> inline bool Variant::can_convert_to<std::vector<char> > () const   { return true; }
#if defined(HAVE_QT)
template<> inline bool Variant::can_convert_to<QString> () const              { return true; }
template<> inline bool Variant::can_convert_to<QByteArray> () const           { return true; }
#endif
template<> inline bool Variant::can_convert_to<const char *> () const         { return true; }

/**
 *  @brief Initialize the class table (must be called once)
 */
void initialize_variant_class_table ();

/**
 *  @brief Special extractors for Variant
 */
template<> TL_PUBLIC void extractor_impl<Variant> (tl::Extractor &ex, Variant &v);
template<> TL_PUBLIC bool test_extractor_impl<Variant> (tl::Extractor &ex, Variant &v);

} // namespace tl

#endif


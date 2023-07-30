
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


#include "gsiDecl.h"
#include "gsiExpression.h"
#include "gsiObjectHolder.h"

#include "tlExpression.h"
#include "tlLog.h"

#include <set>
#include <map>
#include <list>
#include <cstdio>
#include <algorithm>

namespace gsi
{

// -------------------------------------------------------------------
//  Method table implementation

/**
 *  @brief A single entry in the method table
 *  This class provides an entry for one name. It provides flags
 *  (ctor) for the method and a list of implementations
 *  (gsi::MethodBase objects)
 */
class ExpressionMethodTableEntry
{
public:
  typedef std::vector<const gsi::MethodBase *>::const_iterator method_iterator;

  ExpressionMethodTableEntry (const std::string &name)
    : m_name (name)
  { }

  const std::string &name () const
  {
    return m_name;
  }

  void add (const gsi::MethodBase *m)
  {
    m_methods.push_back (m);
  }

  void finish ()
  {
    //  remove duplicate entries in the method list
    std::vector<const gsi::MethodBase *> m = m_methods;
    std::sort(m.begin (), m.end ());
    m_methods.assign (m.begin (), std::unique (m.begin (), m.end ()));
  }

  method_iterator begin () const
  {
    return m_methods.begin ();
  }

  method_iterator end () const
  {
    return m_methods.end ();
  }

private:
  std::string m_name;
  std::vector<const gsi::MethodBase *> m_methods;
};

/**
 *  @brief The method table for a class
 *  The method table will provide the methods associated with a native method, i.e.
 *  a certain name. It only provides the methods, not a overload resolution strategy.
 */
class ExpressionMethodTable
  : public gsi::PerClassClientSpecificData
{
public:
  /**
   *  @brief Find a method by name and static flag
   *  This method will return a pair of true and the method ID if a method with 
   *  the static attribute and the name is found. Otherwise the first value of
   *  the returned pair will be false.
   */
  std::pair<bool, size_t> find (bool st, const std::string &name) const
  {
    std::map<std::pair<bool, std::string>, size_t>::const_iterator t = m_name_map.find (std::make_pair (st, name));
    if (! st && t == m_name_map.end ()) {
      //  can also use static methods for instances
      t = m_name_map.find (std::make_pair (true, name));
    }
    if (t != m_name_map.end ()) {
      return std::make_pair (true, t->second);
    } else {
      return std::make_pair (false, 0);
    }
  }

  /**
   *  @brief Returns the name of the method with ID mid
   */
  const std::string &name (size_t mid) const
  {
    return m_table [mid].name ();
  }

  /**
   *  @brief Begin iterator for the overloaded methods for method ID mid
   */
  ExpressionMethodTableEntry::method_iterator begin (size_t mid) const
  {
    return m_table[mid].begin ();
  }

  /**
   *  @brief End iterator for the overloaded methods for method ID mid
   */
  ExpressionMethodTableEntry::method_iterator end (size_t mid) const
  {
    return m_table[mid].end ();
  }

  static const ExpressionMethodTable *method_table_by_class (const gsi::ClassBase *cls_decl)
  {
    const ExpressionMethodTable *mt = dynamic_cast<const ExpressionMethodTable *>(cls_decl->gsi_data ());
    tl_assert (mt != 0);
    return mt;
  }

  static void initialize_class (const gsi::ClassBase *cls_decl)
  {
    ExpressionMethodTable *mtnc = new ExpressionMethodTable (cls_decl);
    cls_decl->set_gsi_data (mtnc);
  }

private:
  const gsi::ClassBase *mp_cls_decl;
  std::map<std::pair<bool, std::string>, size_t> m_name_map;
  std::vector<ExpressionMethodTableEntry> m_table;

  /**
   *  @brief Adds the given method with the given name to the list of methods registered under that name
   */
  void add_method (const std::string &name, const gsi::MethodBase *mb) 
  {
    bool st = mb->is_static ();

    std::map<std::pair<bool, std::string>, size_t>::iterator n = m_name_map.find (std::make_pair (st, name));
    if (n == m_name_map.end ()) {

      m_name_map.insert (std::make_pair (std::make_pair(st, name), m_table.size ()));
      m_table.push_back (ExpressionMethodTableEntry (name));
      m_table.back ().add (mb);

    } else {

      m_table [n->second].add (mb);

    }
  }

  /**
   *  @brief Private ctor - no construction from the outside
   */
  ExpressionMethodTable ();

  /**
   *  @brief Private ctor - no construction from the outside
   *  This constructor will create the method table for the given class.
   */
  ExpressionMethodTable (const gsi::ClassBase *cls_decl)
    : mp_cls_decl (cls_decl)
  { 
    for (gsi::ClassBase::method_iterator m = cls_decl->begin_methods (); m != cls_decl->end_methods (); ++m) {

      if (! (*m)->is_callback ()) {

        for (gsi::MethodBase::synonym_iterator syn = (*m)->begin_synonyms (); syn != (*m)->end_synonyms (); ++syn) {
          if (syn->is_setter) {
            add_method (syn->name + "=", *m);
          } else if (syn->name == "*!") {
            //  non-commutative multiplication
            add_method ("*", *m);
          } else {
            add_method (syn->name, *m);
          }
        }

      }

    }

    //  do some cleanup
    for (std::vector<ExpressionMethodTableEntry>::iterator m = m_table.begin (); m != m_table.end (); ++m) {
      m->finish ();
    }
  }
};

// -------------------------------------------------------------------

/**
 *  @brief Fetches the final object pointer from a tl::Variant
 */
inline void *get_object (tl::Variant &var)
{
  return var.to_user ();
}

/**
 *  @brief Fetches the object pointer
 *  In contrast to get_object, this function will fetch the pointer
 *  without trying to create the object and without checking whether
 *  it is destroyed already.
 */
void *get_object_raw (tl::Variant &var)
{
  void *obj = 0;
  if (var.type_code () == tl::Variant::t_user) {

    obj = var.native_ptr ();

  } else if (var.type_code () == tl::Variant::t_user_ref) {

    Proxy *p = dynamic_cast<Proxy *> (var.to_object ());
    if (p) {
      obj = p->raw_obj ();
    }

  }
  return obj;
}

// -------------------------------------------------------------------
//  Test if an argument can be converted to the given type

bool test_arg (const gsi::ArgType &atype, const tl::Variant &arg, bool loose);

template <class R>
struct test_arg_func
{
  void operator () (bool *ret, const tl::Variant &arg, const gsi::ArgType & /*atype*/, bool /*loose*/)
  {
    *ret = arg.can_convert_to<R> ();
  }
};

template <>
struct test_arg_func<gsi::VoidType>
{
  void operator () (bool *ret, const tl::Variant & /*arg*/, const gsi::ArgType & /*atype*/, bool /*loose*/)
  {
    *ret = true;
  }
};

template <>
struct test_arg_func<gsi::ObjectType>
{
  void operator () (bool *ret, const tl::Variant &arg, const gsi::ArgType &atype, bool loose)
  {
    //  allow nil of pointers
    if ((atype.is_ptr () || atype.is_cptr ()) && arg.is_nil ()) {
      *ret = true;
      return;
    }

    if (! arg.is_user ()) {
      *ret = false;
      return;
    }

    const tl::VariantUserClassBase *cls = arg.user_cls ();
    if (! cls) {
      *ret = false;
    } else if (! cls->gsi_cls ()->is_derived_from (atype.cls ()) && (! loose || ! cls->gsi_cls ()->can_convert_to(atype.cls ()))) {
      *ret = false;
    } else if ((atype.is_ref () || atype.is_ptr ()) && cls->is_const ()) {
      *ret = false;
    } else {
      *ret = true;
    }
  }
};

template <>
struct test_arg_func<gsi::VectorType>
{
  void operator () (bool *ret, const tl::Variant &arg, const gsi::ArgType &atype, bool loose)
  {
    if (! arg.is_list ()) {
      *ret = false;
      return;
    }

    tl_assert (atype.inner () != 0);
    const ArgType &ainner = *atype.inner ();

    *ret = true;
    for (tl::Variant::const_iterator v = arg.begin (); v != arg.end () && *ret; ++v) { 
      if (! test_arg (ainner, *v, loose)) {
        *ret = false;
      }
    }
  }
};

template <>
struct test_arg_func<gsi::MapType>
{
  void operator () (bool *ret, const tl::Variant &arg, const gsi::ArgType &atype, bool loose)
  {
    //  Note: delegating that to the function avoids "injected class name used as template template expression" warning
    if (! arg.is_array ()) {
      *ret = false;
      return;
    }

    tl_assert (atype.inner () != 0);
    tl_assert (atype.inner_k () != 0);
    const ArgType &ainner = *atype.inner ();
    const ArgType &ainner_k = *atype.inner_k ();

    if (! arg.is_list ()) {
      *ret = false;
      return;
    }

    *ret = true;
    for (tl::Variant::const_array_iterator a = arg.begin_array (); a != arg.end_array () && *ret; ++a) { 
      if (! test_arg (ainner_k, a->first, loose)) {
        *ret = false;
      } else if (! test_arg (ainner, a->second, loose)) {
        *ret = false;
      }
    }
  }
};

bool
test_arg (const gsi::ArgType &atype, const tl::Variant &arg, bool loose)
{
  //  for const X * or X *, nil is an allowed value
  if ((atype.is_cptr () || atype.is_ptr ()) && arg.is_nil ()) {
    return true;
  }

  bool ret = false;
  gsi::do_on_type<test_arg_func> () (atype.type (), &ret, arg, atype, loose);
  return ret;
}
      
// -------------------------------------------------------------------
//  Variant to C conversion

template <class R>
struct var2c
{
  static R get (const tl::Variant &rval)
  {
    return rval.to<R> ();
  }
};

template <>
struct var2c<tl::Variant>
{
  static const tl::Variant &get (const tl::Variant &rval)
  {
    return rval;
  }
};

// ---------------------------------------------------------------------
//  Serialization helpers

/**
 *  @brief An adaptor for a vector which uses the tl::Variant's list perspective
 */
class VariantBasedVectorAdaptorIterator
  : public gsi::VectorAdaptorIterator
{
public:
  VariantBasedVectorAdaptorIterator (tl::Variant::iterator b, tl::Variant::iterator e, const gsi::ArgType *ainner);

  virtual void get (SerialArgs &w, tl::Heap &heap) const;
  virtual bool at_end () const;
  virtual void inc ();

private:
  tl::Variant::iterator m_b, m_e;
  const gsi::ArgType *mp_ainner;
};

/**
 *  @brief An adaptor for a vector which uses the tl::Variant's list perspective
 */
class VariantBasedVectorAdaptor   
  : public gsi::VectorAdaptor
{
public:
  VariantBasedVectorAdaptor (tl::Variant *var, const gsi::ArgType *ainner);

  virtual VectorAdaptorIterator *create_iterator () const;
  virtual void push (SerialArgs &r, tl::Heap &heap);
  virtual void clear ();
  virtual size_t size () const;
  virtual size_t serial_size () const;

private:
  const gsi::ArgType *mp_ainner;
  tl::Variant *mp_var;
};

/**
 *  @brief An adaptor for a map which uses the tl::Variant's array perspective
 */
class VariantBasedMapAdaptorIterator
  : public gsi::MapAdaptorIterator
{
public:
  VariantBasedMapAdaptorIterator (tl::Variant::array_iterator b, tl::Variant::array_iterator e, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k);

  virtual void get (SerialArgs &w, tl::Heap &heap) const; 
  virtual bool at_end () const;
  virtual void inc ();

private:
  tl::Variant::array_iterator m_b, m_e;
  const gsi::ArgType *mp_ainner, *mp_ainner_k;
};

/**
 *  @brief An adaptor for a vector which uses the tl::Variant's list perspective
 */
class VariantBasedMapAdaptor   
  : public gsi::MapAdaptor
{
public:
  VariantBasedMapAdaptor (tl::Variant *var, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k);

  virtual MapAdaptorIterator *create_iterator () const;
  virtual void insert (SerialArgs &r, tl::Heap &heap);
  virtual void clear ();
  virtual size_t size () const;
  virtual size_t serial_size () const;

private:
  const gsi::ArgType *mp_ainner, *mp_ainner_k;
  tl::Variant *mp_var;
};

// ---------------------------------------------------------------------
//  Writer function for serialization

/**
 *  @brief Serialization of POD types
 */
template <class R>
struct writer
{
  void operator() (gsi::SerialArgs *aa, tl::Variant *arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    if (arg->is_nil () && atype.type () != gsi::T_var) {

      if (! (atype.is_ptr () || atype.is_cptr ())) {
        throw tl::Exception (tl::to_string (tr ("Arguments of reference or direct type cannot be passed nil")));
      } else if (atype.is_ptr ()) {
        aa->write<R *> ((R *)0);
      } else {
        aa->write<const R *> ((const R *)0);
      }

    } else {

      if (atype.is_ref () || atype.is_ptr ()) {

        // TODO: morph the variant to the requested type and pass its pointer (requires a non-const reference for arg)
        // -> we would have a reference that can modify the argument (out parameter).
        R *v = new R (var2c<R>::get (*arg));
        heap->push (v);

        aa->write<void *> (v);

      } else if (atype.is_cref ()) {
        //  Note: POD's are written as copies for const refs, so we can pass a temporary here:
        //  (avoids having to create a temp object)
        aa->write<const R &> (var2c<R>::get (*arg));
      } else if (atype.is_cptr ()) {
        //  Note: POD's are written as copies for const ptrs, so we can pass a temporary here:
        //  (avoids having to create a temp object)
        R r = var2c<R>::get (*arg);
        aa->write<const R *> (&r);
      } else {
        aa->write<R> (var2c<R>::get (*arg));
      }

    }
  }
};

/** 
 *  @brief Serialization for strings 
 */
template <>
struct writer<StringType>
{
  void operator() (gsi::SerialArgs *aa, tl::Variant *arg, const gsi::ArgType &atype, tl::Heap *)
  {
    //  Cannot pass ownership currently
    tl_assert (!atype.pass_obj ());

    if (arg->is_nil ()) {

      if (! (atype.is_ptr () || atype.is_cptr ())) {
        //  nil is treated as an empty string for references
        aa->write<void *> ((void *)new StringAdaptorImpl<std::string> (std::string ()));
      } else {
        aa->write<void *> ((void *)0);
      }

    } else {

      // TODO: morph the variant to the requested type and pass its pointer (requires a non-const reference for arg)
      // -> we would have a reference that can modify the argument (out parameter).
      // NOTE: by convention we pass the ownership to the receiver for adaptors.
      aa->write<void *> ((void *)new StringAdaptorImpl<std::string> (arg->to_string ()));

    }
  }
};

/**
 *  @brief Specialization for Variant
 */
template <>
struct writer<VariantType>
{
  void operator() (gsi::SerialArgs *aa, tl::Variant *arg, const gsi::ArgType &, tl::Heap *)
  {
    //  TODO: clarify: is nil a zero-pointer to a variant or a pointer to a "nil" variant?
    // NOTE: by convention we pass the ownership to the receiver for adaptors.
    aa->write<void *> ((void *)new VariantAdaptorImpl<tl::Variant> (arg));
  }
};

/**
 *  @brief Specialization for Vectors
 */
template <>
struct writer<VectorType>
{
  void operator() (gsi::SerialArgs *aa, tl::Variant *arg, const gsi::ArgType &atype, tl::Heap *)
  {
    if (arg->is_nil ()) {
      if (! (atype.is_ptr () || atype.is_cptr ())) {
        throw tl::Exception (tl::to_string (tr ("Arguments of reference or direct type cannot be passed nil")));
      } else {
        aa->write<void *> ((void *)0);
      }
    } else {
      tl_assert (atype.inner () != 0);
      aa->write<void *> ((void *)new VariantBasedVectorAdaptor (arg, atype.inner ()));
    }
  }
};

/**
 *  @brief Specialization for Maps
 */
template <>
struct writer<MapType>
{
  void operator() (gsi::SerialArgs *aa, tl::Variant *arg, const gsi::ArgType &atype, tl::Heap *)
  {
    if (arg->is_nil ()) {
      if (! (atype.is_ptr () || atype.is_cptr ())) {
        throw tl::Exception (tl::to_string (tr ("Arguments of reference or direct type cannot be passed nil")));
      } else {
        aa->write<void *> ((void *)0);
      }
    } else {
      tl_assert (atype.inner () != 0);
      tl_assert (atype.inner_k () != 0);
      aa->write<void *> ((void *)new VariantBasedMapAdaptor (arg, atype.inner (), atype.inner_k ()));
    }
  }
};

/**
 *  @brief Specialization for void
 */
template <>
struct writer<gsi::VoidType>
{
  void operator() (gsi::SerialArgs *, tl::Variant *, const gsi::ArgType &, tl::Heap *)
  {
    //  nothing - void type won't be serialized
  }
};

/**
 *  @brief Specialization for void
 */
template <>
struct writer<gsi::ObjectType>
{
  void operator() (gsi::SerialArgs *aa, tl::Variant *arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    if (atype.is_ref () || atype.is_cref () || atype.is_ptr () || atype.is_cptr ()) {

      if (arg->is_nil ()) {

        if (atype.is_ref () || atype.is_cref ()) {
          throw tl::Exception (tl::to_string (tr ("Cannot pass nil to reference parameters")));
        }

        aa->write<void *> ((void *) 0);

      } else {

        if (! arg->is_user ()) {
          throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s)")), atype.cls ()->name ()));
        }

        const tl::VariantUserClassBase *cls = arg->user_cls ();
        if (!cls) {
          throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s)")), atype.cls ()->name ()));
        }
        if (cls->is_const () && (atype.is_ref () || atype.is_ptr ())) {
          throw tl::Exception (tl::sprintf (tl::to_string (tr ("Cannot pass a const reference of class %s to a non-const reference or pointer parameter")), atype.cls ()->name ()));
        }

        if (cls->gsi_cls ()->is_derived_from (atype.cls ())) {

          if (cls->gsi_cls ()->adapted_type_info ()) {
            //  resolved adapted type
            aa->write<void *> ((void *) cls->gsi_cls ()->adapted_from_obj (get_object (*arg)));
          } else {
            aa->write<void *> (get_object (*arg));
          }

        } else if ((atype.is_cref () || atype.is_cptr ()) && cls->gsi_cls ()->can_convert_to (atype.cls ())) {

          //  We can convert objects for cref and cptr, but ownership over these objects is not transferred.
          //  Hence we have to keep them on the heap.
          void *new_obj = atype.cls ()->create_obj_from (cls->gsi_cls (), get_object (*arg));
          heap->push (new gsi::ObjectHolder (atype.cls (), new_obj));
          aa->write<void *> (new_obj);

        } else {
          throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s)")), atype.cls ()->name ()));
        }

      }

    } else {

      if (! arg->is_user ()) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s)")), atype.cls ()->name ()));
      }

      const tl::VariantUserClassBase *cls = arg->user_cls ();
      if (!cls) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s)")), atype.cls ()->name ()));
      }
      if (cls->is_const () && (atype.is_ref () || atype.is_ptr ())) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Cannot pass a const reference of class %s to a non-const reference or pointer parameter")), atype.cls ()->name ()));
      }

      if (cls->gsi_cls ()->is_derived_from (atype.cls ())) {

        if (cls->gsi_cls ()->adapted_type_info ()) {
          aa->write<void *> (cls->gsi_cls ()->create_adapted_from_obj (get_object (*arg)));
        } else {
          aa->write<void *> ((void *) cls->gsi_cls ()->clone (get_object (*arg)));
        }

      } else if (cls->gsi_cls ()->can_convert_to (atype.cls ())) {

        aa->write<void *> (atype.cls ()->create_obj_from (cls->gsi_cls (), get_object (*arg)));

      } else {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s)")), atype.cls ()->name ()));
      }

    }

  }
};

// ---------------------------------------------------------------------
//  Reader function for serialization

/**
 *  @brief A reader function 
 */
template <class R> 
struct reader
{
  void
  operator() (tl::Variant *out, gsi::SerialArgs *rr, const gsi::ArgType &atype, tl::Heap *heap)
  {
    if (atype.is_ref ()) {
      *out = rr->template read<R &> (*heap);
    } else if (atype.is_cref ()) {
      *out = rr->template read<const R &> (*heap);
    } else if (atype.is_ptr ()) {
      R *p = rr->template read<R *> (*heap);
      if (p == 0) {
        *out = tl::Variant ();
      } else {
        *out = *p;
      }
    } else if (atype.is_cptr ()) {
      const R *p = rr->template read<const R *> (*heap);
      if (p == 0) {
        *out = tl::Variant ();
      } else {
        *out = *p;
      }
    } else {
      *out = rr->template read<R> (*heap);
    }
  }
};

/**
 *  @brief A reader specialization for void * 
 */
template <> 
struct reader<void *>
{
  void
  operator() (tl::Variant *out, gsi::SerialArgs *rr, const gsi::ArgType &atype, tl::Heap *heap)
  {
    tl_assert (!atype.is_ref ());
    tl_assert (!atype.is_cref ());
    tl_assert (!atype.is_ptr ());
    tl_assert (!atype.is_cptr ());
    *out = size_t (rr->read<void *> (*heap));
  }
};

/**
 *  @brief A reader specialization for strings
 */
template <> 
struct reader<gsi::StringType>
{
  void
  operator() (tl::Variant *out, gsi::SerialArgs *rr, const gsi::ArgType &, tl::Heap *heap)
  {
    std::unique_ptr<StringAdaptor> a ((StringAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *out = tl::Variant ();
    } else {
      *out = tl::Variant (std::string (a->c_str (), a->size ()));
    }
  }
};

/**
 *  @brief A reader specialization for variants
 */
template <> 
struct reader<gsi::VariantType>
{
  void
  operator() (tl::Variant *out, gsi::SerialArgs *rr, const gsi::ArgType &, tl::Heap *heap)
  {
    std::unique_ptr<VariantAdaptor> a ((VariantAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *out = tl::Variant ();
    } else {
      *out = a->var ();
    }
  }
};

/**
 *  @brief A reader specialization for maps
 */
template <> 
struct reader<MapType>
{
  void
  operator() (tl::Variant *out, gsi::SerialArgs *rr, const gsi::ArgType &atype, tl::Heap *heap)
  {
    std::unique_ptr<MapAdaptor> a ((MapAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *out = tl::Variant ();
    } else {
      tl_assert (atype.inner () != 0);
      tl_assert (atype.inner_k () != 0);
      VariantBasedMapAdaptor t (out, atype.inner (), atype.inner_k ());
      a->copy_to (&t, *heap); 
    }
  }
};

/**
 *  @brief A reader specialization for const char * 
 */
template <> 
struct reader<VectorType>
{
  void
  operator() (tl::Variant *out, gsi::SerialArgs *rr, const gsi::ArgType &atype, tl::Heap *heap)
  {
    std::unique_ptr<VectorAdaptor> a ((VectorAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *out = tl::Variant ();
    } else {
      tl_assert (atype.inner () != 0);
      VariantBasedVectorAdaptor t (out, atype.inner ());
      a->copy_to (&t, *heap); 
    }
  }
};

/**
 *  @brief A reader specialization for objects
 */
template <> 
struct reader<ObjectType>
{
  void
  operator() (tl::Variant *out, gsi::SerialArgs *rr, const gsi::ArgType &atype, tl::Heap *heap)
  {
    void *obj = rr->read<void *> (*heap);

    bool is_const = atype.is_cptr () || atype.is_cref ();
    bool owner = true;
    if (atype.is_ptr () || atype.is_cptr () || atype.is_ref () || atype.is_cref ()) {
      owner = atype.pass_obj ();
    }
    bool can_destroy = atype.is_ptr () || owner;

    const gsi::ClassBase *clsact = atype.cls ()->subclass_decl (obj);
    tl_assert (clsact != 0);

    if (obj == 0) {

      *out = tl::Variant ();

    } else if (!clsact->adapted_type_info () && clsact->is_managed ()) {

      //  gsi::ObjectBase-based objects can be managed by reference since they
      //  provide a tl::Object through the proxy.

      *out = tl::Variant ();

      const tl::VariantUserClassBase *cls = clsact->var_cls (atype.is_cref () || atype.is_cptr ());
      tl_assert (cls != 0);

      Proxy *proxy = clsact->gsi_object (obj)->find_client<Proxy> ();
      if (proxy) {

        out->set_user_ref (proxy, cls, false);

      } else {

        //  establish a new proxy
        proxy = new Proxy (clsact);
        proxy->set (obj, owner, is_const, can_destroy);

        out->set_user_ref (proxy, cls, owner);

      }

    } else {

      const tl::VariantUserClassBase *cls = 0;

      if (clsact->adapted_type_info ()) {
        //  create an adaptor from an adapted type
        if (owner) {
          obj = clsact->create_from_adapted_consume (obj);
        } else {
          obj = clsact->create_from_adapted (obj);
        }
        cls = clsact->var_cls (false);
      } else {
        cls = clsact->var_cls (is_const);
      }

      tl_assert (cls != 0);
      *out = tl::Variant ();

      //  consider prefer_copy
      if (! owner && atype.prefer_copy () && !clsact->is_managed () && clsact->can_copy ()) {
        obj = clsact->clone (obj);
        owner = true;
      }

      out->set_user (obj, cls, owner);

    }
  }
};

/**
 *  @brief A reader specialization for new objects
 */
template <> 
struct reader<VoidType>
{
  void
  operator() (tl::Variant *, gsi::SerialArgs *, const gsi::ArgType &, tl::Heap *)
  {
    //  nothing - void type won't be serialized
  }
};

// ---------------------------------------------------------------------
//  VariantBasedVectorAdaptorIterator implementation

VariantBasedVectorAdaptorIterator::VariantBasedVectorAdaptorIterator (tl::Variant::iterator b, tl::Variant::iterator e, const gsi::ArgType *ainner)
  : m_b (b), m_e (e), mp_ainner (ainner)
{
  //  .. nothing yet ..
}

void VariantBasedVectorAdaptorIterator::get (SerialArgs &w, tl::Heap &heap) const 
{
  gsi::do_on_type<writer> () (mp_ainner->type (), &w, &*m_b, *mp_ainner, &heap);
}

bool VariantBasedVectorAdaptorIterator::at_end () const 
{
  return m_b == m_e;
}

void VariantBasedVectorAdaptorIterator::inc () 
{
  ++m_b;
}

// ---------------------------------------------------------------------
//  VariantBasedVectorAdaptor implementation

VariantBasedVectorAdaptor::VariantBasedVectorAdaptor (tl::Variant *var, const gsi::ArgType *ainner)
  : mp_ainner (ainner), mp_var (var)
{
}

VectorAdaptorIterator *VariantBasedVectorAdaptor::create_iterator () const
{
  return new VariantBasedVectorAdaptorIterator (mp_var->begin (), mp_var->end (), mp_ainner);
}

void VariantBasedVectorAdaptor::push (SerialArgs &r, tl::Heap &heap) 
{
  tl::Variant member;
  gsi::do_on_type<reader> () (mp_ainner->type (), &member, &r, *mp_ainner, &heap);
  mp_var->push (member);
}

void VariantBasedVectorAdaptor::clear () 
{
  mp_var->set_list ();
}

size_t VariantBasedVectorAdaptor::size () const
{
  return mp_var->size ();
}

size_t VariantBasedVectorAdaptor::serial_size () const 
{
  return mp_ainner->size ();
}

// ---------------------------------------------------------------------
//  VariantBasedMapAdaptorIterator implementation

VariantBasedMapAdaptorIterator::VariantBasedMapAdaptorIterator (tl::Variant::array_iterator b, tl::Variant::array_iterator e, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k)
  : m_b (b), m_e (e), mp_ainner (ainner), mp_ainner_k (ainner_k)
{
  //  .. nothing yet ..
}

void VariantBasedMapAdaptorIterator::get (SerialArgs &w, tl::Heap &heap) const 
{
  //  Note: the const_cast is ugly but in this context we won't modify the variant given as the key.
  //  And it lets us keep the interface tidy.
  gsi::do_on_type<writer> () (mp_ainner_k->type (), &w, const_cast<tl::Variant *> (&m_b->first), *mp_ainner_k, &heap);
  gsi::do_on_type<writer> () (mp_ainner->type (), &w, &m_b->second, *mp_ainner, &heap);
}

bool VariantBasedMapAdaptorIterator::at_end () const 
{
  return m_b == m_e;
}

void VariantBasedMapAdaptorIterator::inc () 
{
  ++m_b;
}

// ---------------------------------------------------------------------
//  VariantBasedMapAdaptor implementation

VariantBasedMapAdaptor::VariantBasedMapAdaptor (tl::Variant *var, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k)
  : mp_ainner (ainner), mp_ainner_k (ainner_k), mp_var (var)
{
}

MapAdaptorIterator *VariantBasedMapAdaptor::create_iterator () const
{
  return new VariantBasedMapAdaptorIterator (mp_var->begin_array (), mp_var->end_array (), mp_ainner, mp_ainner_k);
}

void VariantBasedMapAdaptor::insert (SerialArgs &r, tl::Heap &heap) 
{
  tl::Variant k, v;
  gsi::do_on_type<reader> () (mp_ainner_k->type (), &k, &r, *mp_ainner_k, &heap);
  gsi::do_on_type<reader> () (mp_ainner->type (), &v, &r, *mp_ainner, &heap);
  mp_var->insert (k, v);
}

void VariantBasedMapAdaptor::clear () 
{
  mp_var->set_array ();
}

size_t VariantBasedMapAdaptor::size () const
{
  return mp_var->array_size ();
}

size_t VariantBasedMapAdaptor::serial_size () const 
{
  return mp_ainner_k->size () + mp_ainner->size ();
}

// ---------------------------------------------------------------------
//  Implementation of initialize_expressions

class EvalClassFunction
  : public tl::EvalFunction
{
public:
  EvalClassFunction (const tl::VariantUserClassBase *var_cls)
    : mp_var_cls (var_cls)
  {
    //  .. nothing yet ..
  }

  void execute (const tl::ExpressionParserContext & /*context*/, tl::Variant &out, const std::vector<tl::Variant> &args) const 
  {
    if (! args.empty ()) {
      throw tl::Exception (tl::to_string (tr ("Class '%s' is not a function - use 'new' to create a new object")), mp_var_cls->name ());
    }
    out = tl::Variant ((void *) 0, mp_var_cls, false);
  }

private:
  const tl::VariantUserClassBase *mp_var_cls;
};

void GSI_PUBLIC 
initialize_expressions ()
{
  //  just in case this did not happen yet ...
  gsi::initialize ();

  //  Go through all classes (maybe again)
  std::list<const gsi::ClassBase *> classes = gsi::ClassBase::classes_in_definition_order ();
  for (std::list<const gsi::ClassBase *>::const_iterator c = classes.begin (); c != classes.end (); ++c) {

    if ((*c)->is_external ()) {
      //  skip external classes
      continue;
    } else if ((*c)->declaration () != *c) {
      tl_assert ((*c)->parent () != 0);  //  top-level classes should be merged
      continue;
    }

    //  install the method table:
    ExpressionMethodTable::initialize_class (*c);

    //  register a function that creates a class object (use a function to avoid issues with
    //  late destruction of global variables which the class object is already gone)
    const tl::VariantUserClassBase *cc = (*c)->var_cls_cls ();
    if (cc) {
      tl::Eval::define_global_function ((*c)->name (), new EvalClassFunction (cc));
    }

  }
}

// -------------------------------------------------------------------------
//  VariantUserClassImpl implementation

VariantUserClassImpl::VariantUserClassImpl () 
  : mp_cls (0), mp_self (0), mp_object_cls (0), m_is_const (false)
{ 
  //  .. nothing yet ..
}

void 
VariantUserClassImpl::initialize (const gsi::ClassBase *cls, const tl::VariantUserClassBase *self, const tl::VariantUserClassBase *object_cls, bool is_const)
{
  mp_cls = cls;
  mp_self = self;
  mp_object_cls = object_cls;
  m_is_const = is_const;
}

VariantUserClassImpl::~VariantUserClassImpl () 
{ 
  mp_cls = 0;
}

bool
VariantUserClassImpl::has_method (const std::string &method) const
{
  const gsi::ClassBase *cls = mp_cls;

  while (cls) {
    if (ExpressionMethodTable::method_table_by_class (cls)->find (false, method).first) {
      return true;
    }
    cls = cls->base ();
  }

  return false;
}

bool 
VariantUserClassImpl::equal_impl (void *obj, void *other) const 
{
  if (obj) {

    if (! has_method ("==")) {

      //  No == method - use object identity
      return (void *) this == other;

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;
      vv.resize (1, tl::Variant ());
      vv[0].set_user (other, mp_object_cls, false);

      execute_gsi (context, out, object, "==", vv);

      return out.to_bool ();

    }

  } else {
    return false; 
  }
}

bool 
VariantUserClassImpl::less_impl (void *obj, void *other) const 
{
  if (obj) {

    if (! has_method ("<")) {

      //  No < method - use object pointers
      return (void *) this < other;

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;
      vv.resize (1, tl::Variant ());
      vv[0].set_user (other, mp_object_cls, false);

      execute_gsi (context, out, object, "<", vv);

      return out.to_bool ();

    }

  } else {
    return false; 
  }
}

std::string 
VariantUserClassImpl::to_string_impl (void *obj) const 
{
  if (obj) {

    if (! has_method ("to_s")) {

      //  no method to convert the object to a string
      return std::string ();

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;

      execute_gsi (context, out, object, "to_s", vv);

      return out.to_string ();

    }

  } else {
    return std::string (); 
  }
}

tl::Variant
VariantUserClassImpl::to_variant_impl (void *obj) const
{
  if (obj) {

    if (! has_method ("to_v")) {

      //  no method to convert the object to a string
      return tl::Variant ();

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;

      execute_gsi (context, out, object, "to_v", vv);

      return out;

    }

  } else {
    return tl::Variant ();
  }
}

int
VariantUserClassImpl::to_int_impl (void *obj) const
{
  if (obj) {

    if (! has_method ("to_i")) {

      //  no method to convert the object to an integer
      return 0;

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;

      execute_gsi (context, out, object, "to_i", vv);

      return out.to_int ();

    }

  } else {
    return 0;
  }
}

double
VariantUserClassImpl::to_double_impl (void *obj) const
{
  if (obj) {

    if (! has_method ("to_f")) {

      //  no method to convert the object to a double value
      return 0.0;

    } else {

      tl::ExpressionParserContext context;

      tl::Variant out;

      tl::Variant object (obj, mp_object_cls, false);
      std::vector<tl::Variant> vv;

      execute_gsi (context, out, object, "to_f", vv);

      return out.to_double ();

    }

  } else {
    return 0.0;
  }
}

void
VariantUserClassImpl::execute (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const
{
  if (mp_object_cls == 0 && method == "is_a") {

    if (args.size () != 1) {
      throw tl::EvalError (tl::to_string (tr ("'is_a' method requires exactly one argument")), context);
    }

    bool ret = false;
    if (args [0].is_user ()) {
      const tl::VariantUserClassBase *ub = args [0].user_cls ();
      if (ub && ub->gsi_cls () == mp_cls) {
        ret = true;
      }
    }

    out = ret;

  } else if (mp_object_cls != 0 && method == "new" && args.size () == 0) {

    void *obj = mp_cls->create ();
    if (obj) {

      if (mp_cls->is_managed ()) {

        Proxy *proxy = new Proxy (mp_cls);
        proxy->set (obj, true, false, true);

        //  gsi::Object based objects are managed through a Proxy and
        //  shared pointers within tl::Variant. That means: copy by reference.
        out.set_user_ref (proxy, mp_object_cls, true);

      } else {
        out.set_user (obj, mp_object_cls, true);
      }

    } else {
      out.reset ();
    }

  } else if (mp_object_cls == 0 && method == "dup") {

    if (args.size () != 0) {
      throw tl::EvalError (tl::to_string (tr ("'dup' method does not allow arguments")), context);
    }

    void *obj = mp_cls->create ();
    if (obj) {

      mp_cls->assign (obj, get_object (object));

      if (mp_cls->is_managed ()) {

        Proxy *proxy = new Proxy (mp_cls);
        proxy->set (obj, true, false, true);

        //  gsi::Object based objects are managed through a Proxy and
        //  shared pointers within tl::Variant. That mean: copy by reference.
        out.set_user_ref (proxy, mp_cls->var_cls (false), true);

      } else {
        out.set_user (obj, mp_cls->var_cls (false), true);
      }

    } else {
      out.reset ();
    }

  } else {
    try {
      execute_gsi (context, out, object, method, args);
    } catch (tl::EvalError &) {
      throw;
    } catch (tl::Exception &ex) {
      throw tl::EvalError (ex.msg (), context);
    }
  }
}

static tl::Variant
special_method_impl (gsi::MethodBase::special_method_type smt, tl::Variant &self, const std::vector<tl::Variant> &args)
{
  if (smt == gsi::MethodBase::Destroy) {
    self.user_destroy ();
  } else if (smt == gsi::MethodBase::Keep) {
    //  nothing to do here for GSI objects
  } else if (smt == gsi::MethodBase::Release) {
    //  nothing to do here for GSI objects
  } else if (smt == gsi::MethodBase::Create) {
    //  nothing to do here for GSI objects
  } else if (smt == gsi::MethodBase::IsConst) {
    return tl::Variant (self.user_is_const ());
  } else if (smt == gsi::MethodBase::Destroyed) {

    if (self.type_code () == tl::Variant::t_user) {
      return self.to_user () == 0;
    } else if (self.type_code () == tl::Variant::t_user_ref) {
      Proxy *proxy = dynamic_cast<Proxy *> (self.to_object ());
      if (proxy) {
        return proxy->destroyed ();
      }
    }

    return true;

  } else if (smt == gsi::MethodBase::Assign) {
    tl_assert (args.size () == 1);
    if (!args.front ().is_user () || self.user_cls () != args.front ().user_cls ()) {
      throw tl::Exception (tl::to_string (tr ("Source and target object must be of the same type for assignment")));
    }
    self.user_assign (args.front ());
  } else if (smt == gsi::MethodBase::Dup) {
    return self.user_dup ();
  }

  return tl::Variant ();
}

static std::pair<const ExpressionMethodTable *, size_t> find_method (const gsi::ClassBase *cls, bool as_static, const std::string &method)
{
  const ExpressionMethodTable *mt = 0;
  size_t mid = 0;

  while (cls) {

    mt = ExpressionMethodTable::method_table_by_class (cls);
    std::pair<bool, size_t> t = mt->find (as_static, method);
    if (t.first) {
      mid = t.second;
      return std::make_pair (mt, mid);
    }

    //  try unnamed child classes as static
    for (auto cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
      if (cc->name ().empty ()) {
        std::pair<const ExpressionMethodTable *, size_t> m = find_method (cc->declaration (), true, method);
        if (m.first) {
          return m;
        }
      }
    }

    cls = cls->base ();

  }

  return std::make_pair ((const ExpressionMethodTable *) 0, size_t (0));
}

static const gsi::ClassBase *find_class_scope (const gsi::ClassBase *cls, const std::string &name)
{
  while (cls) {

    //  try named child classes
    for (auto cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
      if (cc->name () == name) {
        return cc->declaration ();
      }
    }

    //  try unnamed child classes as additional bases
    for (auto cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
      if (cc->name ().empty ()) {
        const gsi::ClassBase *scope = find_class_scope (cc->declaration (), name);
        if (scope) {
          return scope;
        }
      }
    }

    cls = cls->base ();

  }

  return 0;
}

void
VariantUserClassImpl::execute_gsi (const tl::ExpressionParserContext & /*context*/, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const
{
  tl_assert (object.is_user ());

  const gsi::ClassBase *clsact = mp_cls;
  if (clsact) {
    //  determine the real class of the object (it may be a subclass)
    void *obj = get_object_raw (object);
    if (obj) {
      clsact = clsact->subclass_decl (obj);
    }
  }

  auto m = find_method (clsact, mp_object_cls != 0 /*static*/, method);

  const ExpressionMethodTable *mt = m.first;
  size_t mid = m.second;

  if (! mt) {

    //  try class scope
    const gsi::ClassBase *scope = find_class_scope (clsact, method);
    if (scope) {

      if (! args.empty ()) {
        throw tl::Exception (tl::to_string (tr ("'%s' is not a function and cannot have parameters")), method);
      }

      //  we found a class scope: return a reference to that
      const tl::VariantUserClassBase *scope_var_cls = scope->var_cls_cls ();
      if (scope_var_cls) {
        out = tl::Variant ((void *) 0, scope_var_cls, false);
      } else {
        out = tl::Variant ();
      }
      return;

    } else {
      throw tl::Exception (tl::to_string (tr ("Unknown method '%s' of class '%s'")), method, clsact->name ());
    }

  }

  const gsi::MethodBase *meth = 0;
  int candidates = 0;

  for (ExpressionMethodTableEntry::method_iterator m = mt->begin (mid); m != mt->end (mid); ++m) {

    if ((*m)->is_signal()) {
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("Signals are not supported inside expressions (event %s)")), method.c_str ()));
    } else if ((*m)->is_callback()) {
      //  ignore callbacks
    } else if ((*m)->compatible_with_num_args ((unsigned int) args.size ())) {
      ++candidates;
      meth = *m;
    }

  }

  //  no candidate -> error
  if (! meth) {

    std::set<unsigned int> nargs;
    for (ExpressionMethodTableEntry::method_iterator m = mt->begin (mid); m != mt->end (mid); ++m) {
      nargs.insert (std::distance ((*m)->begin_arguments (), (*m)->end_arguments ()));
    }
    std::string nargs_s;
    for (std::set<unsigned int>::const_iterator na = nargs.begin (); na != nargs.end (); ++na) {
      if (na != nargs.begin ()) {
        nargs_s += "/";
      }
      nargs_s += tl::to_string (*na);
    }

    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Invalid number of arguments for method %s, class %s (got %d, expected %s)")), method.c_str (), mp_cls->name (), int (args.size ()), nargs_s));
  }

  //  more than one candidate -> refine by checking the arguments
  if (candidates > 1) {

    meth = 0;
    candidates = 0;
    int score = 0;
    bool const_matching = true;

    for (ExpressionMethodTableEntry::method_iterator m = mt->begin (mid); m != mt->end (mid); ++m) {

      if (! (*m)->is_callback () && ! (*m)->is_signal ()) {

        //  check arguments (count and type)
        bool is_valid = (*m)->compatible_with_num_args ((unsigned int) args.size ());
        int sc = 0;
        int i = 0;
        for (gsi::MethodBase::argument_iterator a = (*m)->begin_arguments (); is_valid && i < int (args.size ()) && a != (*m)->end_arguments (); ++a, ++i) {
          if (test_arg (*a, args [i], false /*strict*/)) {
            ++sc;
          } else if (test_arg (*a, args [i], true /*loose*/)) {
            //  non-scoring match
          } else {
            is_valid = false;
          }
        }

        if (is_valid) {

          //  constness matching candidates have precedence
          if ((*m)->is_const () != m_is_const) {
            if (const_matching && candidates > 0) {
              is_valid = false;
            } else {
              const_matching = false;
            }
          } else if (! const_matching) {
            const_matching = true;
            candidates = 0;
          }

        }

        if (is_valid) {

          //  otherwise take the candidate with the better score
          if (candidates > 0 && sc > score) {
            candidates = 1;
            meth = *m;
            score = sc;
          } else if (candidates == 0 || sc == score) {
            ++candidates;
            meth = *m;
            score = sc;
          }

        }

      }

    }

  }

  if (! meth) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("No method with matching arguments for method %s, class %s")), method.c_str (), mp_cls->name ()));
  }

  if (candidates > 1) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Ambiguous overload variants for method %s, class %s - multiple method declarations match arguments")), method.c_str (), mp_cls->name ()));
  }

  if (m_is_const && ! meth->is_const ()) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Cannot call non-const method %s, class %s on a const reference")), method.c_str (), mp_cls->name ()));
  }

  if (meth->is_signal ()) {

    //  TODO: events not supported yet
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Signals not supported yet (method %s, class %s)")), method.c_str (), mp_cls->name ()));

  } else if (meth->smt () != gsi::MethodBase::None) {

    out = special_method_impl (meth->smt (), object, args);

  } else {

    gsi::SerialArgs arglist (meth->argsize ());
    tl::Heap heap;
    size_t narg = 0;
    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); a != meth->end_arguments () && narg < args.size (); ++a, ++narg) {
      try {
        //  Note: this const_cast is ugly, but it will basically enable "out" parameters
        //  TODO: clean this up.
        gsi::do_on_type<writer> () (a->type (), &arglist, const_cast<tl::Variant *> (&args [narg]), *a, &heap);
      } catch (tl::Exception &ex) {
        std::string msg = ex.msg () + tl::sprintf (tl::to_string (tr (" (argument '%s')")), a->spec ()->name ());
        throw tl::Exception (msg);
      }
    }

    SerialArgs retlist (meth->retsize ());

    meth->call (get_object (object), arglist, retlist);

    if (meth->ret_type ().is_iter ()) {
      //  TODO: iterators not supported yet
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("Iterators not supported yet (method %s, class %s)")), method.c_str (), mp_cls->name ()));
    } else {
      out = tl::Variant ();
      try {
        gsi::do_on_type<reader> () (meth->ret_type ().type (), &out, &retlist, meth->ret_type (), &heap);
      } catch (tl::Exception &ex) {
        std::string msg = ex.msg () + tl::to_string (tr (" (return value)"));
        throw tl::Exception (msg);
      }
    }

  }
}

}



/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "gsiVariantArgs.h"
#include "gsiTypes.h"
#include "gsiSerialisation.h"
#include "gsiClassBase.h"
#include "gsiObjectHolder.h"

#include "tlVariant.h"
#include "tlHeap.h"

namespace gsi
{

// -------------------------------------------------------------------

/**
 *  @brief Fetches the final object pointer from a tl::Variant
 */
inline void *get_object (tl::Variant &var)
{
  return var.to_user ();
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

    if (arg.is_list ()) {

      //  we may implicitly convert an array into a constructor call of a target object -
      //  for now we only check whether the number of arguments is compatible with the array given.

      int n = int (arg.size ());

      *ret = false;
      for (gsi::ClassBase::method_iterator c = atype.cls ()->begin_constructors (); c != atype.cls ()->end_constructors (); ++c) {
        if ((*c)->compatible_with_num_args (n)) {
          *ret = true;
          break;
        }
      }

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

void push_args (gsi::SerialArgs &arglist, const tl::Variant &args, const gsi::MethodBase *meth, tl::Heap *heap);

/**
 *  @brief Specialization for void
 */
template <>
struct writer<gsi::ObjectType>
{
  void operator() (gsi::SerialArgs *aa, tl::Variant *arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    if (arg->is_nil ()) {

      if (atype.is_ref () || atype.is_cref ()) {
        throw tl::Exception (tl::to_string (tr ("Cannot pass nil to reference parameters")));
      } else if (! atype.is_cptr () && ! atype.is_ptr ()) {
        throw tl::Exception (tl::to_string (tr ("Cannot pass nil to direct parameters")));
      }

      aa->write<void *> ((void *) 0);

    } else if (arg->is_list ()) {

      //  we may implicitly convert an array into a constructor call of a target object -
      //  for now we only check whether the number of arguments is compatible with the array given.

      int n = int (arg->size ());
      const gsi::MethodBase *meth = 0;
      for (gsi::ClassBase::method_iterator c = atype.cls ()->begin_constructors (); c != atype.cls ()->end_constructors (); ++c) {
        if ((*c)->compatible_with_num_args (n)) {
          meth = *c;
          break;
        }
      }

      if (!meth) {
        throw tl::Exception (tl::to_string (tr ("No constructor of %s available that takes %d arguments (implicit call from tuple)")), atype.cls ()->name (), n);
      }

      //  implicit call of constructor
      gsi::SerialArgs retlist (meth->retsize ());
      gsi::SerialArgs arglist (meth->argsize ());

      push_args (arglist, *arg, meth, heap);

      meth->call (0, arglist, retlist);

      void *new_obj = retlist.read<void *> (*heap);
      if (new_obj && (atype.is_ptr () || atype.is_cptr () || atype.is_ref () || atype.is_cref ())) {
        //  For pointers or refs, ownership over these objects is not transferred.
        //  Hence we have to keep them on the heap.
        //  TODO: what if the called method takes ownership using keep()?
        heap->push (new gsi::ObjectHolder (atype.cls (), new_obj));
      }

      aa->write<void *> (new_obj);

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

      if (atype.is_ref () || atype.is_cref () || atype.is_ptr () || atype.is_cptr ()) {

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

      } else {

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

  }
};

void push_arg (gsi::SerialArgs &arglist, const ArgType &atype, tl::Variant &arg, tl::Heap *heap)
{
  gsi::do_on_type<writer> () (atype.type (), &arglist, &arg, atype, heap);
}

void push_args (gsi::SerialArgs &arglist, const tl::Variant &args, const gsi::MethodBase *meth, tl::Heap *heap)
{
  int n = int (args.size ());
  int narg = 0;

  for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); a != meth->end_arguments () && narg < n; ++a, ++narg) {
    try {
      //  Note: this const_cast is ugly, but it will basically enable "out" parameters
      //  TODO: clean this up.
      gsi::do_on_type<writer> () (a->type (), &arglist, const_cast<tl::Variant *> ((args.get_list ().begin () + narg).operator-> ()), *a, heap);
    } catch (tl::Exception &ex) {
      std::string msg = ex.msg () + tl::sprintf (tl::to_string (tr (" (argument '%s')")), a->spec ()->name ());
      throw tl::Exception (msg);
    }
  }
}

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
//  pop_arg implementation

void
pull_arg (gsi::SerialArgs &retlist, const gsi::ArgType &atype, tl::Variant &arg_out, tl::Heap *heap)
{
  gsi::do_on_type<reader> () (atype.type (), &arg_out, &retlist, atype, heap);
}

}

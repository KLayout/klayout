
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

#if defined(HAVE_RUBY)

#include "rba.h"
#include "rbaUtils.h"
#include "rbaMarshal.h"
#include "rbaConvert.h"
#include "rbaInternal.h"

#include "gsiDeclBasic.h"
#include "gsiObjectHolder.h"

namespace rba
{

void push_args (gsi::SerialArgs &arglist, const gsi::MethodBase *meth, VALUE *argv, int argc, tl::Heap &heap);

// -------------------------------------------------------------------
//  Serialization adaptors for strings, variants, vectors and maps

/**
 *  @brief An adaptor for a string from ruby objects
 */
class RubyBasedStringAdaptor
  : public gsi::StringAdaptor
{
public:
  RubyBasedStringAdaptor (VALUE value)
  {
    m_string = rba_safe_obj_as_string (value);
    gc_lock_object (m_string);
  }

  ~RubyBasedStringAdaptor ()
  {
    gc_unlock_object (m_string);
  }

  virtual const char *c_str () const
  {
    return RSTRING_PTR (m_string);
  }

  virtual size_t size () const
  {
    return RSTRING_LEN (m_string);
  }

  virtual void set (const char * /*c_str*/, size_t /*s*/, tl::Heap & /*heap*/)
  {
    //  TODO: is there a setter for a string?
    //  -> so far, string OUT parameters are not supported
  }

private:
  VALUE m_string;
};

/**
 *  @brief An adaptor for a byte array from ruby objects
 */
class RubyBasedByteArrayAdaptor
  : public gsi::ByteArrayAdaptor
{
public:
  RubyBasedByteArrayAdaptor (VALUE value)
  {
    m_bytes = rba_safe_string_value (value);
    gc_lock_object (m_bytes);
  }

  ~RubyBasedByteArrayAdaptor ()
  {
    gc_unlock_object (m_bytes);
  }

  virtual const char *c_str () const
  {
    return RSTRING_PTR (m_bytes);
  }

  virtual size_t size () const
  {
    return RSTRING_LEN (m_bytes);
  }

  virtual void set (const char * /*c_str*/, size_t /*s*/, tl::Heap & /*heap*/)
  {
    //  TODO: is there a setter for a string?
    //  -> so far, byte array OUT parameters are not supported
  }

private:
  VALUE m_bytes;
};

/**
 *  @brief An adaptor for a variant from ruby objects
 */
class RubyBasedVariantAdaptor
  : public gsi::VariantAdaptor
{
public:
  RubyBasedVariantAdaptor (VALUE var);
  ~RubyBasedVariantAdaptor ();

  virtual tl::Variant var () const;
  virtual void set (const tl::Variant &v, tl::Heap &heap);
  VALUE value () const { return m_var; }

private:
  VALUE m_var;
};

/**
 *  @brief An adaptor for a vector iterator from Ruby objects
 */
class RubyBasedVectorAdaptorIterator
  : public gsi::VectorAdaptorIterator
{
public:
  RubyBasedVectorAdaptorIterator (VALUE array, const gsi::ArgType *ainner);

  virtual void get (gsi::SerialArgs &w, tl::Heap &heap) const;
  virtual bool at_end () const;
  virtual void inc ();

private:
  VALUE m_array;
  size_t m_i, m_len;
  const gsi::ArgType *mp_ainner;
};

/**
 *  @brief An adaptor for a vector from Ruby objects
 */
class RubyBasedVectorAdaptor
  : public gsi::VectorAdaptor
{
public:
  RubyBasedVectorAdaptor (VALUE array, const gsi::ArgType *ainner);
  ~RubyBasedVectorAdaptor ();

  virtual gsi::VectorAdaptorIterator *create_iterator () const;
  virtual void push (gsi::SerialArgs &r, tl::Heap &heap);
  virtual void clear ();
  virtual size_t size () const;
  virtual size_t serial_size () const;

private:
  const gsi::ArgType *mp_ainner;
  VALUE m_array;
};

/**
 *  @brief An adaptor for a map iterator from Ruby objects
 */
class RubyBasedMapAdaptorIterator
  : public gsi::MapAdaptorIterator
{
public:
  RubyBasedMapAdaptorIterator (VALUE hash, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k);

  virtual void get (gsi::SerialArgs &w, tl::Heap &heap) const;
  virtual bool at_end () const;
  virtual void inc ();

private:
  std::vector<std::pair<VALUE, VALUE> > m_kv;
  std::vector<std::pair<VALUE, VALUE> >::const_iterator m_b, m_e;
  const gsi::ArgType *mp_ainner, *mp_ainner_k;
};

/**
 *  @brief An adaptor for a map from Ruby objects
 */
class RubyBasedMapAdaptor
  : public gsi::MapAdaptor
{
public:
  RubyBasedMapAdaptor (VALUE hash, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k);
  ~RubyBasedMapAdaptor ();

  virtual gsi::MapAdaptorIterator *create_iterator () const;
  virtual void insert (gsi::SerialArgs &r, tl::Heap &heap);
  virtual void clear ();
  virtual size_t size () const;
  virtual size_t serial_size () const;

private:
  const gsi::ArgType *mp_ainner, *mp_ainner_k;
  VALUE m_hash;
};

// -------------------------------------------------------------------
//  Return the boxed value pointer for a given basic type from the reference

template <class R>
struct get_boxed_value_func
{
  void operator() (void **value, VALUE arg, tl::Heap *heap)
  {
    if (TYPE (arg) != T_DATA) {

      R *v = new R (ruby2c<R> (arg));
      heap->push (v);
      *value = v;

    } else {

      const gsi::ClassBase *bt = gsi::cls_decl <gsi::Value> ();

      Proxy *p = 0;
      Data_Get_Struct (arg, Proxy, p);
      if (!p->cls_decl ()->is_derived_from (bt)) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Passing an object to pointer or reference requires a boxed type (RBA::%s)")), bt->name ()));
      }

      gsi::Value *bo = reinterpret_cast<gsi::Value *> (p->obj ());
      if (bo) {
        *value = bo->value ().template morph<R> ().native_ptr ();
      }

    }
  }
};

struct get_boxed_value_func_error
{
  void operator() (void ** /*value*/, VALUE /*arg*/, tl::Heap * /*heap*/)
  {
    tl_assert (false);
  }
};

template <> struct get_boxed_value_func<gsi::VariantType> : get_boxed_value_func_error { };
template <> struct get_boxed_value_func<gsi::StringType> : get_boxed_value_func_error { };
template <> struct get_boxed_value_func<gsi::ByteArrayType> : get_boxed_value_func_error { };
template <> struct get_boxed_value_func<gsi::ObjectType> : get_boxed_value_func_error { };
template <> struct get_boxed_value_func<gsi::VectorType> : get_boxed_value_func_error { };
template <> struct get_boxed_value_func<gsi::MapType> : get_boxed_value_func_error { };
template <> struct get_boxed_value_func<gsi::VoidType> : get_boxed_value_func_error { };

void *boxed_value_ptr (gsi::BasicType type, VALUE arg, tl::Heap &heap)
{
  void *ret = 0;
  gsi::do_on_type<get_boxed_value_func> () (type, &ret, arg, &heap);
  return ret;
}

// -------------------------------------------------------------------

/**
 *  @brief A serialization wrapper (write mode)
 *
 *  The generic class is for POD objects.
 */
template <class R>
struct writer
{
  void operator() (gsi::SerialArgs *aa, VALUE arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    if (arg == Qnil) {

      if (atype.is_ref () || atype.is_cref ()) {
        throw tl::Exception (tl::to_string (tr ("Arguments or return values of reference type cannot be passed nil")));
      } else if (atype.is_ptr ()) {
        aa->write<R *> ((R *)0);
      } else if (atype.is_cptr ()) {
        aa->write<const R *> ((const R *)0);
      } else {
        aa->write<R> ((R)0);
      }

    } else {

      if (atype.is_ref () || atype.is_ptr ()) {
        // references or pointers require a boxed object. Pointers also allow nil.
        void *vc = boxed_value_ptr (atype.type (), arg, *heap);
        if (! vc && atype.is_ref ()) {
          throw tl::Exception (tl::to_string (tr ("Arguments or return values of reference or direct type cannot be passed nil or an empty boxed value object")));
        }
        aa->write<void *> (vc);
      } else if (atype.is_cref ()) {
        aa->write<const R &> (ruby2c<R> (arg));
      } else if (atype.is_cptr ()) {
        R r = ruby2c<R> (arg);
        aa->write<const R *> (&r);
      } else {
        aa->write<R> (ruby2c<R> (arg));
      }

    }
  }
};

/**
 *  @brief Serialization for strings
 */
template <>
struct writer<gsi::StringType>
{
  void operator() (gsi::SerialArgs *aa, VALUE arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    //  Cannot pass ownership currently
    tl_assert (!atype.pass_obj ());

    if (arg == Qnil) {

      if (! (atype.is_ptr () || atype.is_cptr ())) {
        //  nil is treated as an empty string for references
        aa->write<void *> ((void *)new gsi::StringAdaptorImpl<std::string> (std::string ()));
      } else {
        aa->write<void *> ((void *)0);
      }

    } else {

      if (atype.is_ref () || atype.is_ptr ()) {

        // references or pointers require a boxed object. Pointers also allow nil.
        void *vc = 0;
        get_boxed_value_func<std::string> () (&vc, arg, heap);
        if (! vc && atype.is_ref ()) {
          throw tl::Exception (tl::to_string (tr ("Arguments or return values of reference or direct type cannot be passed nil or an empty boxed value object")));
        }

        //  NOTE: by convention we pass the ownership to the receiver for adaptors.
        if (! vc) {
          aa->write<void *> (0);
        } else {
          aa->write<void *> ((void *)new gsi::StringAdaptorImpl<std::string> ((std::string *) vc));
        }

      } else {

        //  NOTE: by convention we pass the ownership to the receiver for adaptors.
        aa->write<void *> ((void *)new RubyBasedStringAdaptor (arg));

      }

    }
  }
};

/**
 *  @brief Serialization for strings
 */
template <>
struct writer<gsi::ByteArrayType>
{
  void operator() (gsi::SerialArgs *aa, VALUE arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    //  Cannot pass ownership currently
    tl_assert (!atype.pass_obj ());

    if (arg == Qnil) {

      if (! (atype.is_ptr () || atype.is_cptr ())) {
        //  nil is treated as an empty string for references
        aa->write<void *> ((void *)new gsi::ByteArrayAdaptorImpl<std::vector<char> > (std::vector<char> ()));
      } else {
        aa->write<void *> ((void *)0);
      }

    } else {

      if (atype.is_ref () || atype.is_ptr ()) {

        // references or pointers require a boxed object. Pointers also allow nil.
        void *vc = 0;
        get_boxed_value_func<std::vector<char> > () (&vc, arg, heap);
        if (! vc && atype.is_ref ()) {
          throw tl::Exception (tl::to_string (tr ("Arguments or return values of reference or direct type cannot be passed nil or an empty boxed value object")));
        }

        //  NOTE: by convention we pass the ownership to the receiver for adaptors.
        if (! vc) {
          aa->write<void *> (0);
        } else {
          aa->write<void *> ((void *)new gsi::ByteArrayAdaptorImpl<std::vector<char> > ((std::vector<char> *) vc));
        }

      } else {

        //  NOTE: by convention we pass the ownership to the receiver for adaptors.
        aa->write<void *> ((void *)new RubyBasedByteArrayAdaptor (arg));

      }

    }
  }
};

/**
 *  @brief Specialization for Variant
 */
template <>
struct writer<gsi::VariantType>
{
  void operator() (gsi::SerialArgs *aa, VALUE arg, const gsi::ArgType &, tl::Heap *)
  {
    //  TODO: clarify: is nil a zero-pointer to a variant or a pointer to a "nil" variant?
    // NOTE: by convention we pass the ownership to the receiver for adaptors.
    aa->write<void *> ((void *)new RubyBasedVariantAdaptor (arg));
  }
};

/**
 *  @brief Specialization for Vectors
 */
template <>
struct writer<gsi::VectorType>
{
  void operator() (gsi::SerialArgs *aa, VALUE arg, const gsi::ArgType &atype, tl::Heap *)
  {
    if (arg == Qnil) {
      if (! (atype.is_ptr () || atype.is_cptr ())) {
        throw tl::Exception (tl::to_string (tr ("Arguments of reference or direct type cannot be passed nil")));
      } else {
        aa->write<void *> ((void *)0);
      }
    } else {

      if (TYPE (arg) != T_ARRAY) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected array, got %s)")), rba_class_name (arg).c_str ()));
      }

      tl_assert (atype.inner () != 0);
      aa->write<void *> ((void *)new RubyBasedVectorAdaptor (arg, atype.inner ()));

    }
  }
};

/**
 *  @brief Specialization for Maps
 */
template <>
struct writer<gsi::MapType>
{
  void operator() (gsi::SerialArgs *aa, VALUE arg, const gsi::ArgType &atype, tl::Heap *)
  {
    if (arg == Qnil) {
      if (! (atype.is_ptr () || atype.is_cptr ())) {
        throw tl::Exception (tl::to_string (tr ("Arguments of reference or direct type cannot be passed nil")));
      } else {
        aa->write<void *> ((void *)0);
      }

    } else {

      if (TYPE (arg) != T_HASH) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected hash, got %s)")), rba_class_name (arg).c_str ()));
      }

      tl_assert (atype.inner () != 0);
      tl_assert (atype.inner_k () != 0);
      aa->write<void *> ((void *)new RubyBasedMapAdaptor (arg, atype.inner (), atype.inner_k ()));

    }
  }
};

/**
 *  @brief A specialization of the write function for vector types
 */
template <>
struct writer <gsi::ObjectType>
{
  void operator() (gsi::SerialArgs *aa, VALUE arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    if (arg == Qnil) {

      if (! (atype.is_ptr () || atype.is_cptr ())) {
        throw tl::Exception (tl::to_string (tr ("Arguments of reference or direct type cannot be passed nil")));
      } else {
        aa->write<void *> ((void *) 0);
      }

    } else if (TYPE (arg) == T_ARRAY) {

      //  we may implicitly convert an array into a constructor call of a target object -
      //  for now we only check whether the number of arguments is compatible with the array given.

      int n = RARRAY_LEN (arg);
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

      push_args (arglist, meth, RARRAY_PTR (arg), n, *heap);

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

      if (TYPE (arg) != T_DATA) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s, got %s)")), atype.cls ()->name (), rba_class_name (arg).c_str ()));
      }

      Proxy *p = 0;
      Data_Get_Struct (arg, Proxy, p);

      if (atype.is_ptr () || atype.is_cptr () || atype.is_ref () || atype.is_cref ()) {

        if (p->cls_decl ()->is_derived_from (atype.cls ())) {

          if (p->cls_decl ()->adapted_type_info ()) {
            //  resolved adapted type
            aa->write<void *> ((void *) p->cls_decl ()->adapted_from_obj (p->obj ()));
          } else {
            aa->write<void *> (p->obj ());
          }

        } else if ((atype.is_cref () || atype.is_cptr ()) && p->cls_decl ()->can_convert_to (atype.cls ())) {

          //  We can convert objects for cref and cptr, but ownership over these objects is not transferred.
          //  Hence we have to keep them on the heap.
          void *new_obj = atype.cls ()->create_obj_from (p->cls_decl (), p->obj ());
          heap->push (new gsi::ObjectHolder (atype.cls (), new_obj));
          aa->write<void *> (new_obj);

        } else {
          throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s, got %s)")), atype.cls ()->name (), rba_class_name (arg).c_str ()));
        }

      } else {

        if (p->cls_decl ()->is_derived_from (atype.cls ())) {

          if (p->cls_decl ()->adapted_type_info ()) {
            //  resolved adapted type
            aa->write<void *> (p->cls_decl ()->create_adapted_from_obj (p->obj ()));
          } else {
            aa->write<void *> (atype.cls ()->clone (p->obj ()));
          }

        } else if (p->cls_decl ()->can_convert_to (atype.cls ())) {

          aa->write<void *> (atype.cls ()->create_obj_from (p->cls_decl (), p->obj ()));

        } else {
          throw tl::Exception (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s, got %s)")), atype.cls ()->name (), rba_class_name (arg).c_str ()));
        }

      }

    }

  }
};

/**
 *  @brief T_void is ignored on writing
 */
template <>
struct writer <gsi::VoidType>
{
  void operator() (gsi::SerialArgs * /*aa*/, VALUE /*arg*/, const gsi::ArgType & /*atype*/, tl::Heap * /*heap*/)
  {
    //  .. nothing: void is not serialized
  }
};

// -------------------------------------------------------------------
//  Push an argument on the call or return stack

void
push_arg (const gsi::ArgType &atype, gsi::SerialArgs &aserial, VALUE arg, tl::Heap &heap)
{
  gsi::do_on_type<writer> () (atype.type (), &aserial, arg, atype, &heap);
}

/**
 *  @brief Deserialization wrapper
 *
 *  The default implementation is for POD types, strings and variants
 */
template <class R>
struct reader
{
  void operator() (gsi::SerialArgs *rr, VALUE *ret, Proxy * /*self*/, const gsi::ArgType &arg, tl::Heap *heap)
  {
    if (arg.is_ref ()) {
      *ret = c2ruby<R> (rr->template read<R &> (*heap));
    } else if (arg.is_cref ()) {
      *ret = c2ruby<R> (rr->template read<const R &> (*heap));
    } else if (arg.is_ptr ()) {
      R *p = rr->template read<R *> (*heap);
      if (p) {
        *ret = c2ruby<R> (*p);
      } else {
        *ret = Qnil;
      }
    } else if (arg.is_cptr ()) {
      const R *p = rr->template read<const R *> (*heap);
      if (p) {
        *ret = c2ruby<R> (*p);
      } else {
        *ret = Qnil;
      }
    } else {
      *ret = c2ruby<R> (rr->template read<R> (*heap));
    }
  }
};

/**
 *  @brief Deserialization wrapper: specialization for void *
 *
 *  Without that would would have to handle void *&, void * const &, ...
 *  TODO: right now these types are not supported.
 */
template <>
struct reader<void *>
{
  void operator() (gsi::SerialArgs *rr, VALUE *ret, Proxy * /*self*/, const gsi::ArgType &arg, tl::Heap *heap)
  {
    tl_assert (! arg.is_cref ());
    tl_assert (! arg.is_ref ());
    tl_assert (! arg.is_cptr ());
    tl_assert (! arg.is_ptr ());
    *ret = c2ruby<void *> (rr->read<void *> (*heap));
  }
};

/**
 *  @brief Deserialization wrapper: specialization for strings
 */
template <>
struct reader<gsi::StringType>
{
  void operator() (gsi::SerialArgs *rr, VALUE *ret, Proxy * /*self*/, const gsi::ArgType &, tl::Heap *heap)
  {
    std::unique_ptr<gsi::StringAdaptor> a ((gsi::StringAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *ret = Qnil;
    } else {
      *ret = rb_str_new (a->c_str (), long (a->size ()));
    }
  }
};

/**
 *  @brief Deserialization wrapper: specialization for strings
 */
template <>
struct reader<gsi::ByteArrayType>
{
  void operator() (gsi::SerialArgs *rr, VALUE *ret, Proxy * /*self*/, const gsi::ArgType &, tl::Heap *heap)
  {
    std::unique_ptr<gsi::ByteArrayAdaptor> a ((gsi::ByteArrayAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *ret = Qnil;
    } else {
      *ret = rb_str_new (a->c_str (), long (a->size ()));
    }
  }
};

static VALUE object_from_variant (tl::Variant &var, Proxy *self, const gsi::ArgType &atype, bool transfer = false)
{
  if (var.is_user()) {

    bool is_direct = (! atype.is_cptr() && ! atype.is_ptr () && ! atype.is_cref () && ! atype.is_ref ());
    bool pass_obj = atype.pass_obj() || is_direct;
    bool is_const = atype.is_cptr() || atype.is_cref();
    bool prefer_copy = false;
    bool can_destroy = false;

    //  TODO: ugly const_cast, but there is no "const shared reference" ...
    gsi::Proxy *holder = dynamic_cast<gsi::Proxy *>(const_cast<tl::Object *>(var.to_object ()));

    void *obj = var.to_user ();
    const gsi::ClassBase *cls = var.user_cls ()->gsi_cls ();

    if (pass_obj || transfer) {

      if (holder) {

        //  transfer ownership of object: when the transfer mode indicates a transfer of ownership (pass_obj == true)
        //  and the holder is owning the object, we transfer ownership (case 2). If the variant itself is a reference,
        //  this indicates a variable reference within gsi::Expressions or another case, where the object (through the
        //  holder and another variant) is actually held otherwise. In that case, we leave the ownership where it is
        //  (case 1, pass by reference).
        if (var.user_is_ref ()) {
          pass_obj = false;   // case 1
        } else if (holder->owned ()) {
          holder->keep ();   // case 2
          can_destroy = true;
        }

      } else {

        //  If the object was not owned before, it is not owned after (bears risk of invalid
        //  pointers, but it's probably rarely the case. Non-managed objects are usually copied
        //  between the ownership spaces.
        //  If the variant holds the user object, we can take it from it and claim ownership.
        if (var.user_is_ref ()) {
          prefer_copy = false;   // unsafe
          pass_obj = false;
        } else {
          obj = var.user_take ();
          can_destroy = true;
        }

      }

    } else {

      //  This is the case for return values that prefer to be copied (e.g. from const &)
      prefer_copy = atype.prefer_copy ();

    }

    return object_to_ruby (obj, self, cls, pass_obj, is_const, prefer_copy, can_destroy);

  } else {
    return c2ruby<tl::Variant> (var);
  }
}

/**
 *  @brief Deserialization wrapper: specialization for variants
 */
template <>
struct reader<gsi::VariantType>
{
  void operator() (gsi::SerialArgs *rr, VALUE *ret, Proxy *self, const gsi::ArgType &atype, tl::Heap *heap)
  {
    std::unique_ptr<gsi::VariantAdaptor> a ((gsi::VariantAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *ret = Qnil;
    } else {
      gsi::VariantAdaptorImpl<tl::Variant> *aa = dynamic_cast<gsi::VariantAdaptorImpl<tl::Variant> *> (a.get ());
      RubyBasedVariantAdaptor *pa = dynamic_cast<RubyBasedVariantAdaptor *> (a.get ());
      if (aa) {
        //  A small optimization that saves one variant copy
        *ret = object_from_variant (aa->var_ref_nc (), self, atype);
      } else if (pa) {
        //  Optimization for Ruby to Ruby transfer
        *ret = pa->value ();
      } else {
        tl::Variant v = a->var ();
        //  NOTE: as v may hold the object, we need to transfer ownership
        *ret = object_from_variant (v, self, atype, true);
      }
    }
  }
};

/**
 *  @brief Deserialization wrapper: specialization for vectors
 */
template <>
struct reader<gsi::VectorType>
{
  void operator() (gsi::SerialArgs *rr, VALUE *ret, Proxy * /*self*/, const gsi::ArgType &atype, tl::Heap *heap)
  {
    std::unique_ptr<gsi::VectorAdaptor> a ((gsi::VectorAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *ret = Qnil;
    } else {
      *ret = rb_ary_new ();
      tl_assert (atype.inner () != 0);
      RubyBasedVectorAdaptor t (*ret, atype.inner ());
      a->copy_to (&t, *heap);
    }
  }
};

/**
 *  @brief Deserialization wrapper: specialization for maps
 */
template <>
struct reader<gsi::MapType>
{
  void operator() (gsi::SerialArgs *rr, VALUE *ret, Proxy * /*self*/, const gsi::ArgType &atype, tl::Heap *heap)
  {
    std::unique_ptr<gsi::MapAdaptor> a ((gsi::MapAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *ret = Qnil;
    } else {
      *ret = rb_hash_new ();
      tl_assert (atype.inner () != 0);
      tl_assert (atype.inner_k () != 0);
      RubyBasedMapAdaptor t (*ret, atype.inner (), atype.inner_k ());
      a->copy_to (&t, *heap);
    }
  }
};

/**
 *  @brief Deserialization wrapper: specialization for objects
 */
template <>
struct reader<gsi::ObjectType>
{
  void operator() (gsi::SerialArgs *rr, VALUE *ret, Proxy *self, const gsi::ArgType &atype, tl::Heap *heap)
  {
    void *obj = rr->read<void *> (*heap);
    if (! obj) {
      *ret = Qnil;
    } else {
      *ret = object_to_ruby (obj, self, atype);
    }
  }
};

template <>
struct reader<gsi::VoidType>
{
  void operator() (gsi::SerialArgs * /*rr*/, VALUE * /*ret*/, Proxy * /*self*/, const gsi::ArgType & /*atype*/, tl::Heap * /*heap*/)
  {
    //  .. nothing: void is not serialized
  }
};

// ---------------------------------------------------------------------
//  RubyBasedVariantAdaptor implementation

RubyBasedVariantAdaptor::RubyBasedVariantAdaptor (VALUE var)
  : m_var (var)
{
  gc_lock_object (m_var);
}

RubyBasedVariantAdaptor::~RubyBasedVariantAdaptor ()
{
  gc_unlock_object (m_var);
}

tl::Variant RubyBasedVariantAdaptor::var () const
{
  return ruby2c<tl::Variant> (m_var);
}

void RubyBasedVariantAdaptor::set (const tl::Variant & /*v*/, tl::Heap & /*heap*/)
{
  //  TODO: is there a setter for a string?
}

// ---------------------------------------------------------------------
//  RubyBasedVectorAdaptorIterator implementation

RubyBasedVectorAdaptorIterator::RubyBasedVectorAdaptorIterator (VALUE array, const gsi::ArgType *ainner)
  : m_array (array), m_i (0), m_len (RARRAY_LEN (array)), mp_ainner (ainner)
{
  //  .. nothing yet ..
}

void RubyBasedVectorAdaptorIterator::get (gsi::SerialArgs &w, tl::Heap &heap) const
{
  gsi::do_on_type<writer> () (mp_ainner->type (), &w, rb_ary_entry (m_array, long (m_i)), *mp_ainner, &heap);
}

bool RubyBasedVectorAdaptorIterator::at_end () const
{
  return m_i == m_len;
}

void RubyBasedVectorAdaptorIterator::inc ()
{
  ++m_i;
}

// ---------------------------------------------------------------------
//  RubyBasedVectorAdaptor implementation

RubyBasedVectorAdaptor::RubyBasedVectorAdaptor (VALUE array, const gsi::ArgType *ainner)
  : mp_ainner (ainner), m_array (array)
{
  gc_lock_object (m_array);
}

RubyBasedVectorAdaptor::~RubyBasedVectorAdaptor ()
{
  gc_unlock_object (m_array);
}

gsi::VectorAdaptorIterator *RubyBasedVectorAdaptor::create_iterator () const
{
  return new RubyBasedVectorAdaptorIterator (m_array, mp_ainner);
}

void RubyBasedVectorAdaptor::push (gsi::SerialArgs &r, tl::Heap &heap)
{
  VALUE member;
  gsi::do_on_type<reader> () (mp_ainner->type (), &r, &member, (Proxy *) 0, *mp_ainner, &heap);
  rb_ary_push (m_array, member);
}

void RubyBasedVectorAdaptor::clear ()
{
  rb_ary_clear (m_array);
}

size_t RubyBasedVectorAdaptor::size () const
{
  return RARRAY_LEN (m_array);
}

size_t RubyBasedVectorAdaptor::serial_size () const
{
  return mp_ainner->size ();
}

// ---------------------------------------------------------------------
//  RubyBasedMapAdaptorIterator implementation

static int push_map_i (VALUE key, VALUE value, VALUE arg)
{
  std::vector<std::pair<VALUE, VALUE> > *v = (std::vector<std::pair<VALUE, VALUE> > *) arg;
  v->push_back (std::make_pair (key, value));
  return ST_CONTINUE;
}

RubyBasedMapAdaptorIterator::RubyBasedMapAdaptorIterator (VALUE hash, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k)
  : mp_ainner (ainner), mp_ainner_k (ainner_k)
{
  m_kv.reserve (RHASH_SIZE (hash));
  rb_hash_foreach (hash, (int (*)(...)) &push_map_i, (VALUE) &m_kv);
  m_b = m_kv.begin ();
  m_e = m_kv.end ();
}

void RubyBasedMapAdaptorIterator::get (gsi::SerialArgs &w, tl::Heap &heap) const
{
  gsi::do_on_type<writer> () (mp_ainner_k->type (), &w, m_b->first, *mp_ainner_k, &heap);
  gsi::do_on_type<writer> () (mp_ainner->type (), &w, m_b->second, *mp_ainner, &heap);
}

bool RubyBasedMapAdaptorIterator::at_end () const
{
  return m_b == m_e;
}

void RubyBasedMapAdaptorIterator::inc ()
{
  ++m_b;
}

// ---------------------------------------------------------------------
//  RubyBasedMapAdaptor implementation

RubyBasedMapAdaptor::RubyBasedMapAdaptor (VALUE hash, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k)
  : mp_ainner (ainner), mp_ainner_k (ainner_k), m_hash (hash)
{
  gc_lock_object (m_hash);
}

RubyBasedMapAdaptor::~RubyBasedMapAdaptor()
{
  gc_unlock_object (m_hash);
}

gsi::MapAdaptorIterator *RubyBasedMapAdaptor::create_iterator () const
{
  return new RubyBasedMapAdaptorIterator (m_hash, mp_ainner, mp_ainner_k);
}

void RubyBasedMapAdaptor::insert (gsi::SerialArgs &r, tl::Heap &heap)
{
  VALUE k, v;
  gsi::do_on_type<reader> () (mp_ainner_k->type (), &r, &k, (Proxy *) 0, *mp_ainner_k, &heap);
  gsi::do_on_type<reader> () (mp_ainner->type (), &r, &v, (Proxy *) 0, *mp_ainner, &heap);
  rb_hash_aset (m_hash, k, v);
}

void RubyBasedMapAdaptor::clear ()
{
  rb_hash_clear (m_hash);
}

size_t RubyBasedMapAdaptor::size () const
{
  return RHASH_SIZE (m_hash);
}

size_t RubyBasedMapAdaptor::serial_size () const
{
  return mp_ainner_k->size () + mp_ainner->size ();
}

// -------------------------------------------------------------------
//  Pops an argument from the call or return stack

VALUE
pop_arg (const gsi::ArgType &atype, Proxy *self, gsi::SerialArgs &aserial, tl::Heap &heap)
{
  VALUE ret = Qnil;
  gsi::do_on_type<reader> () (atype.type (), &aserial, &ret, self, atype, &heap);
  return ret;
}

// -------------------------------------------------------------------
//  Tests if an argument can be converted to the given type

//  if atype is a vector:
//      argument must be an array of the given type
//  if atype is a ref:
//      argument must be a boxed type of the required type or an object of the requested class
//  if atype is a ptr:
//      argument must be a boxed type of the required type or an object of the requested class or nil
//  if atype is a cptr:
//      argument must be of requested type or nil
//  otherwise:
//      argument must be of the requested type

template <class R>
struct test_arg_func
{
  void operator () (bool *ret, VALUE arg, const gsi::ArgType &atype, bool loose)
  {
    if ((atype.is_cptr () || atype.is_ptr ()) && arg == Qnil) {

      //  for const X * or X *, nil is an allowed value
      *ret = true;

    } else {

      *ret = false;

      if (atype.is_ptr () || atype.is_ref ()) {

        //  check if we have a boxed type
        if (TYPE (arg) == T_DATA) {
          const gsi::ClassBase *bc = gsi::cls_decl <gsi::Value> ();
          Proxy *p = 0;
          Data_Get_Struct (arg, Proxy, p);
          if (p->cls_decl ()->is_derived_from (bc)) {
            *ret = true;
          }
        }

      }

      if (! *ret) {
        //  otherwise try a normal match and let the serializer sort out the wrong arguments with
        //  a good error message.
        *ret = test_type<R> (arg, loose);
      }

    }
  }
};

/**
 *  @brief Test a VALUE for compatibility with a vector of the given type R
 */
template <class R>
struct test_vector
{
  void operator() (bool *ret, VALUE arr, const gsi::ArgType &ainner, bool loose)
  {
    *ret = true;

    unsigned int len = RARRAY_LEN(arr);
    VALUE *el = RARRAY_PTR(arr);
    while (len-- > 0) {
      if (! test_arg (ainner, *el++, loose)) {
        *ret = false;
        break;
      }
    }
  }
};

template <>
struct test_arg_func<gsi::VectorType>
{
  void operator () (bool *ret, VALUE arg, const gsi::ArgType &atype, bool loose)
  {
    if ((atype.is_cptr () || atype.is_ptr ()) && arg == Qnil) {
      //  for pointers to vectors, nil is a valid value
      *ret = true;
    } else if (TYPE (arg) != T_ARRAY) {
      *ret = false;
    } else {

      tl_assert (atype.inner () != 0);
      const gsi::ArgType &ainner = *atype.inner ();

      gsi::do_on_type<test_vector> () (ainner.type (), ret, arg, ainner, loose);

    }
  }
};

struct HashTestKeyValueData
{
  const gsi::ArgType *ainner_k;
  const gsi::ArgType *ainner;
  bool *ret;
  bool loose;
};

static int hash_test_value_key (VALUE key, VALUE value, VALUE a)
{
  HashTestKeyValueData *args = (HashTestKeyValueData *)a;
  if (! test_arg (*args->ainner_k, key, args->loose)) {
    *(args->ret) = false;
    return ST_STOP;
  }
  if (! test_arg (*args->ainner, value, args->loose)) {
    *(args->ret) = false;
    return ST_STOP;
  }
  return ST_CONTINUE;
}

template <>
struct test_arg_func<gsi::MapType>
{
  void operator () (bool *ret, VALUE arg, const gsi::ArgType &atype, bool loose)
  {
    if ((atype.is_cptr () || atype.is_ptr ()) && arg == Qnil) {
      //  for pointers to maps, nil is a valid value
      *ret = true;
    } else if (TYPE (arg) != T_HASH) {
      *ret = false;
    } else {

      tl_assert (atype.inner () != 0);
      tl_assert (atype.inner_k () != 0);

      HashTestKeyValueData args;
      args.ainner_k = atype.inner_k ();
      args.ainner = atype.inner ();
      args.ret = ret;
      args.loose = loose;

      *ret = true;
      rb_hash_foreach (arg, (int (*)(...)) &hash_test_value_key, (VALUE) &args);

    }
  }
};

template <>
struct test_arg_func<gsi::ObjectType>
{
  void operator () (bool *ret, VALUE arg, const gsi::ArgType &atype, bool loose)
  {
    if ((atype.is_cptr () || atype.is_ptr ()) && arg == Qnil) {

      //  for const X * or X *, nil is an allowed value
      *ret = true;

    } else if (loose && TYPE (arg) == T_ARRAY) {

      //  we may implicitly convert an array into a constructor call of a target object -
      //  for now we only check whether the number of arguments is compatible with the array given.

      int n = RARRAY_LEN (arg);

      *ret = false;
      for (gsi::ClassBase::method_iterator c = atype.cls ()->begin_constructors (); c != atype.cls ()->end_constructors (); ++c) {
        if ((*c)->compatible_with_num_args (n)) {
          *ret = true;
          break;
        }
      }

    } else {

      *ret = (TYPE (arg) == T_DATA);

      if (*ret) {

        //  additionally check, whether the object matches the class type
        Proxy *p = 0;
        Data_Get_Struct (arg, Proxy, p);

        //  in loose mode (second pass) try to match the types via implicit constructors,
        //  in strict mode (first pass) require direct type match
        if (p->cls_decl () == atype.cls () || (loose && (p->cls_decl ()->is_derived_from (atype.cls ()) || p->cls_decl ()->can_convert_to (atype.cls ())))) {
          //  type matches: check constness
          if ((atype.is_ref () || atype.is_ptr ()) && p->const_ref ()) {
            *ret = false;
          }
        } else {
          *ret = false;
        }

      }

    }

  }
};

bool
test_arg (const gsi::ArgType &atype, VALUE arg, bool loose)
{
  bool ret = false;
  gsi::do_on_type<test_arg_func> () (atype.type (), &ret, arg, atype, loose);
  return ret;
}

}

#endif


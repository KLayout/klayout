
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


#include "pyaMarshal.h"
#include "pyaObject.h"
#include "pyaConvert.h"
#include "pyaModule.h"

#include "gsiTypes.h"
#include "gsiObjectHolder.h"

namespace pya
{

// -------------------------------------------------------------------
//  Serialization adaptors for strings, variants, vectors and maps

/**
 *  @brief An adaptor for a string from ruby objects
 */
class PythonBasedStringAdaptor
  : public gsi::StringAdaptor
{
public:
  PythonBasedStringAdaptor (const PythonPtr &string)
    : m_stdstr (python2c<std::string> (string.get ())), m_string (string)
  {
    //  .. nothing yet ..
  }

  virtual const char *c_str () const
  {
    return m_stdstr.c_str ();
  }

  virtual size_t size () const
  {
    return m_stdstr.size ();
  }

  virtual void set (const char * /*c_str*/, size_t /*s*/, tl::Heap & /*heap*/)
  {
    //  TODO: is there a setter for a string?
    //  So far it's not possible to have string OUT parameter
  }

private:
  std::string m_stdstr;
  PythonPtr m_string;
};

/**
 *  @brief An adaptor for a byte array from ruby objects
 */
class PythonBasedByteArrayAdaptor
  : public gsi::ByteArrayAdaptor
{
public:
  PythonBasedByteArrayAdaptor (const PythonPtr &ba)
    : m_bytearray (python2c<std::vector<char> > (ba.get ())), m_bytes (ba)
  {
    //  .. nothing yet ..
  }

  virtual const char *c_str () const
  {
    return &m_bytearray.front ();
  }

  virtual size_t size () const
  {
    return m_bytearray.size ();
  }

  virtual void set (const char * /*c_str*/, size_t /*s*/, tl::Heap & /*heap*/)
  {
    //  TODO: is there a setter for a byte array?
    //  So far it's not possible to have bytes OUT parameter
  }

private:
  std::vector<char> m_bytearray;
  PythonPtr m_bytes;
};

/**
 *  @brief An adaptor for a variant from ruby objects
 */
class PythonBasedVariantAdaptor
  : public gsi::VariantAdaptor
{
public:
  PythonBasedVariantAdaptor (const PythonPtr &var);

  virtual tl::Variant var () const;
  virtual void set (const tl::Variant &v, tl::Heap & /*heap*/);
  const PythonPtr &ptr () const { return m_var; }

private:
  PythonPtr m_var;
};

/**
 *  @brief An adaptor for a vector iterator from Python objects
 */
class PythonBasedVectorAdaptorIterator
  : public gsi::VectorAdaptorIterator
{
public:
  PythonBasedVectorAdaptorIterator (const PythonPtr &array, size_t len, const gsi::ArgType *ainner);

  virtual void get (gsi::SerialArgs &w, tl::Heap &heap) const;
  virtual bool at_end () const;
  virtual void inc ();

private:
  PythonPtr m_array;
  size_t m_i, m_len;
  const gsi::ArgType *mp_ainner;
};

/**
 *  @brief An adaptor for a vector from Python objects
 */
class PythonBasedVectorAdaptor
  : public gsi::VectorAdaptor
{
public:
  PythonBasedVectorAdaptor (const PythonPtr &array, const gsi::ArgType *ainner);

  virtual gsi::VectorAdaptorIterator *create_iterator () const;
  virtual void push (gsi::SerialArgs &r, tl::Heap &heap);
  virtual void clear ();
  virtual size_t size () const;
  virtual size_t serial_size () const;

private:
  const gsi::ArgType *mp_ainner;
  PythonPtr m_array;
};

/**
 *  @brief An adaptor for a map iterator from Python objects
 */
class PythonBasedMapAdaptorIterator
  : public gsi::MapAdaptorIterator
{
public:
  PythonBasedMapAdaptorIterator (const PythonPtr &hash, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k);

  virtual void get (gsi::SerialArgs &w, tl::Heap &heap) const;
  virtual bool at_end () const;
  virtual void inc ();

private:
  const gsi::ArgType *mp_ainner, *mp_ainner_k;
  Py_ssize_t m_pos;
  PythonPtr m_hash;
  PyObject *m_key, *m_value;
  bool m_has_items;
};

/**
 *  @brief An adaptor for a map from Python objects
 */
class PythonBasedMapAdaptor
  : public gsi::MapAdaptor
{
public:
  PythonBasedMapAdaptor (const PythonPtr &hash, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k);

  virtual gsi::MapAdaptorIterator *create_iterator () const;
  virtual void insert (gsi::SerialArgs &r, tl::Heap &heap);
  virtual void clear ();
  virtual size_t size () const;
  virtual size_t serial_size () const;

private:
  const gsi::ArgType *mp_ainner, *mp_ainner_k;
  PythonPtr m_hash;
};

// -------------------------------------------------------------------
//  Return the boxed value pointer for a given basic type from the reference

template <class R>
struct get_boxed_value_func
{
  void operator() (void **ret, PyObject *arg, tl::Heap *heap)
  {
    const gsi::ClassBase *cls_decl = PythonModule::cls_for_type (Py_TYPE (arg));
    if (! cls_decl) {

      R *v = new R (python2c<R> (arg));
      heap->push (v);
      *ret = v;

    } else {

      const gsi::ClassBase *bt = gsi::cls_decl <gsi::Value> ();

      if (!cls_decl->is_derived_from (bt)) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Passing an object to pointer or reference requires a boxed type (pya.%s)")), bt->name ()));
      }

      PYAObjectBase *p = PYAObjectBase::from_pyobject (arg);
      gsi::Value *bo = reinterpret_cast<gsi::Value *> (p->obj ());
      if (bo) {
        *ret = bo->value ().template morph<R> ().native_ptr ();
      }

    }
  }
};

void *boxed_value_ptr (gsi::BasicType type, PyObject *arg, tl::Heap &heap)
{
  void *value = 0;
  gsi::do_on_type<get_boxed_value_func> () (type, &value, arg, &heap);
  return value;
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
  void operator() (gsi::SerialArgs *aa, PyObject *arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    if (arg == Py_None || arg == NULL) {

      if (atype.is_ref () || atype.is_cref ()) {
        throw tl::Exception (tl::to_string (tr ("Arguments or return values of reference type cannot be passed None")));
      } else if (atype.is_ptr ()) {
        aa->write<R *> ((R *)0);
      } else if (atype.is_cptr ()) {
        aa->write<const R *> ((const R *)0);
      } else {
        aa->write<R> ((R)0);
      }

    } else {

      if (atype.is_ref () || atype.is_ptr ()) {
        // references or pointers require a boxed object. Pointers also allow None.
        void *vc = boxed_value_ptr (atype.type (), arg, *heap);
        if (! vc && atype.is_ref ()) {
          throw tl::Exception (tl::to_string (tr ("Arguments or return values of reference or direct type cannot be passed None or an empty boxed value object")));
        }
        aa->write<void *> (vc);
      } else if (atype.is_cref ()) {
        //  Note: POD's are written as copies for const refs, so we can pass a temporary here:
        //  (avoids having to create a temp object)
        aa->write<const R &> (python2c<R> (arg));
      } else if (atype.is_cptr ()) {
        //  Note: POD's are written as copies for const ptrs, so we can pass a temporary here:
        //  (avoids having to create a temp object)
        R r = python2c<R> (arg);
        aa->write<const R *> (&r);
      } else {
        aa->write<R> (python2c<R> (arg));
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
  void operator() (gsi::SerialArgs *aa, PyObject *arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    //  Cannot pass ownership currently
    tl_assert (!atype.pass_obj ());

    if (arg == Py_None || arg == NULL) {

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
        aa->write<void *> ((void *)new PythonBasedStringAdaptor (arg));

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
  void operator() (gsi::SerialArgs *aa, PyObject *arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    //  Cannot pass ownership currently
    tl_assert (!atype.pass_obj ());

    if (arg == Py_None || arg == NULL) {

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
        aa->write<void *> ((void *)new PythonBasedByteArrayAdaptor (arg));

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
  void operator() (gsi::SerialArgs *aa, PyObject *arg, const gsi::ArgType &, tl::Heap *)
  {
    //  TODO: clarify: is nil a zero-pointer to a variant or a pointer to a "nil" variant?
    // NOTE: by convention we pass the ownership to the receiver for adaptors.
    aa->write<void *> ((void *)new PythonBasedVariantAdaptor (arg));
  }
};

/**
 *  @brief Specialization for Vectors
 */
template <>
struct writer<gsi::VectorType>
{
  void operator() (gsi::SerialArgs *aa, PyObject *arg, const gsi::ArgType &atype, tl::Heap *)
  {
    if (arg == Py_None || arg == NULL) {
      if (! (atype.is_ptr () || atype.is_cptr ())) {
        throw tl::Exception (tl::to_string (tr ("Arguments of reference or direct type cannot be passed nil")));
      } else {
        aa->write<void *> ((void *)0);
      }
    } else {
      tl_assert (atype.inner () != 0);
      aa->write<void *> ((void *)new PythonBasedVectorAdaptor (arg, atype.inner ()));
    }
  }
};

/**
 *  @brief Specialization for Maps
 */
template <>
struct writer<gsi::MapType>
{
  void operator() (gsi::SerialArgs *aa, PyObject *arg, const gsi::ArgType &atype, tl::Heap *)
  {
    if (arg == Py_None || arg == NULL) {
      if (! (atype.is_ptr () || atype.is_cptr ())) {
        throw tl::Exception (tl::to_string (tr ("Arguments of reference or direct type cannot be passed nil")));
      } else {
        aa->write<void *> ((void *)0);
      }
    } else {
      tl_assert (atype.inner () != 0);
      tl_assert (atype.inner_k () != 0);
      aa->write<void *> ((void *)new PythonBasedMapAdaptor (arg, atype.inner (), atype.inner_k ()));
    }
  }
};

/**
 *  @brief A serialization wrapper (write mode)
 *  Specialization for objects
 */
template <>
struct writer<gsi::ObjectType>
{
  void operator() (gsi::SerialArgs *aa, PyObject *arg, const gsi::ArgType &atype, tl::Heap *heap)
  {
    if (arg == Py_None || arg == NULL) {

      if (! (atype.is_ptr () || atype.is_cptr ())) {
        throw tl::Exception (tl::to_string (tr ("Arguments of reference or direct type cannot be passed null")));
      } else {
        aa->write<void *> ((void *) 0);
        return;
      }

    }

    if (atype.is_ptr () || atype.is_cptr () || atype.is_ref () || atype.is_cref ()) {

      const gsi::ClassBase *cls_decl = PythonModule::cls_for_type (Py_TYPE (arg));
      if (! cls_decl) {
        throw tl::TypeError (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s, got %s)")), atype.cls ()->name (), Py_TYPE (arg)->tp_name));
      }

      if (cls_decl->is_derived_from (atype.cls ())) {

        PYAObjectBase *p = PYAObjectBase::from_pyobject (arg);

        if (cls_decl->adapted_type_info ()) {
          //  resolved adapted type
          aa->write<void *> ((void *)cls_decl->adapted_from_obj (p->obj ()));
        } else {
          aa->write<void *> (p->obj ());
        }

      } else if (cls_decl->can_convert_to (atype.cls ())) {

        PYAObjectBase *p = PYAObjectBase::from_pyobject (arg);

        //  We can convert objects for cref and cptr, but ownership over these objects is not transferred.
        //  Hence we have to keep them on the heap.
        void *new_obj = atype.cls ()->create_obj_from (p->cls_decl (), p->obj ());
        heap->push (new gsi::ObjectHolder (atype.cls (), new_obj));
        aa->write<void *> (new_obj);

      } else {
        throw tl::TypeError (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s, got %s)")), atype.cls ()->name (), cls_decl->name ()));
      }

    } else {

      const gsi::ClassBase *cls_decl = PythonModule::cls_for_type (Py_TYPE (arg));
      if (! cls_decl) {
        throw tl::TypeError (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s, got %s)")), atype.cls ()->name (), Py_TYPE (arg)->tp_name));
      }

      if (cls_decl->is_derived_from (atype.cls ())) {

        PYAObjectBase *p = PYAObjectBase::from_pyobject (arg);

        if (cls_decl->adapted_type_info ()) {
          //  resolved adapted type
          aa->write<void *> (cls_decl->create_adapted_from_obj (p->obj ()));
        } else {
          aa->write<void *> (atype.cls ()->clone (p->obj ()));
        }

      } else if (cls_decl->can_convert_to (atype.cls ())) {

        PYAObjectBase *p = PYAObjectBase::from_pyobject (arg);
        aa->write<void *> (atype.cls ()->create_obj_from (cls_decl, p->obj ()));

      } else {
        throw tl::TypeError (tl::sprintf (tl::to_string (tr ("Unexpected object type (expected argument of class %s, got %s)")), atype.cls ()->name (), cls_decl->name ()));
      }

    }

  }
};

/**
 *  @brief A serialization wrapper (write mode)
 *  Specialization for void
 */
template <>
struct writer<gsi::VoidType>
{
  void operator() (gsi::SerialArgs *, PyObject *, const gsi::ArgType &, tl::Heap *)
  {
    //  ignore void
  }
};

void
push_arg (const gsi::ArgType &atype, gsi::SerialArgs &aserial, PyObject *arg, tl::Heap &heap)
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
  void operator() (gsi::SerialArgs *rr, PythonRef *ret, PYAObjectBase * /*self*/, const gsi::ArgType &arg, tl::Heap *heap)
  {
    if (arg.is_ref ()) {
      *ret = c2python (rr->template read<R &> (*heap));
    } else if (arg.is_cref ()) {
      *ret = c2python (rr->template read<const R &> (*heap));
    } else if (arg.is_ptr ()) {
      R *p = rr->template read<R *> (*heap);
      if (p) {
        *ret = c2python (*p);
      } else {
        *ret = PythonRef (Py_None, false /*borrowed*/);
      }
    } else if (arg.is_cptr ()) {
      const R *p = rr->template read<const R *> (*heap);
      if (p) {
        *ret = c2python (*p);
      } else {
        *ret = PythonRef (Py_None, false /*borrowed*/);
      }
    } else {
      *ret = c2python (rr->template read<R> (*heap));
    }
  }
};

/**
 *  @brief Deserialization wrapper: specialization for const char *
 *
 *  Without that would would have to handle void *&, void * const &, ...
 *  TODO: right now these types are not supported.
 */
template <>
struct reader<void *>
{
  void operator() (gsi::SerialArgs *rr, PythonRef *ret, PYAObjectBase * /*self*/, const gsi::ArgType &arg, tl::Heap *heap)
  {
    tl_assert (! arg.is_cref ());
    tl_assert (! arg.is_ref ());
    tl_assert (! arg.is_cptr ());
    tl_assert (! arg.is_ptr ());
    *ret = c2python (rr->read<void *> (*heap));
  }
};

/**
 *  @brief Deserialization wrapper: specialization for strings
 */
template <>
struct reader<gsi::StringType>
{
  void operator() (gsi::SerialArgs *rr, PythonRef *ret, PYAObjectBase * /*self*/, const gsi::ArgType &, tl::Heap *heap)
  {
    std::unique_ptr<gsi::StringAdaptor> a ((gsi::StringAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *ret = PythonRef (Py_None, false /*borrowed*/);
    } else {
      *ret = c2python (std::string (a->c_str (), a->size ()));
    }
  }
};

/**
 *  @brief Deserialization wrapper: specialization for byte arrays
 */
template <>
struct reader<gsi::ByteArrayType>
{
  void operator() (gsi::SerialArgs *rr, PythonRef *ret, PYAObjectBase * /*self*/, const gsi::ArgType &, tl::Heap *heap)
  {
    std::unique_ptr<gsi::ByteArrayAdaptor> a ((gsi::ByteArrayAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *ret = PythonRef (Py_None, false /*borrowed*/);
    } else {
      const char *cp = a->c_str ();
      size_t sz = a->size ();
#if PY_MAJOR_VERSION < 3
      *ret = PyByteArray_FromStringAndSize (cp, sz);
#else
      *ret = PyBytes_FromStringAndSize (cp, sz);
#endif
    }
  }
};

static
PyObject *object_from_variant (tl::Variant &var, PYAObjectBase *self, const gsi::ArgType &atype, bool transfer = false)
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

    return object_to_python (obj, self, cls, pass_obj, is_const, prefer_copy, can_destroy);

  } else {
    return c2python (var);
  }
}

/**
 *  @brief Deserialization wrapper: specialization for variants
 */
template <>
struct reader<gsi::VariantType>
{
  void operator() (gsi::SerialArgs *rr, PythonRef *ret, PYAObjectBase *self, const gsi::ArgType &atype, tl::Heap *heap)
  {
    std::unique_ptr<gsi::VariantAdaptor> a ((gsi::VariantAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *ret = PythonRef (Py_None, false /*borrowed*/);
    } else {
      gsi::VariantAdaptorImpl<tl::Variant> *aa = dynamic_cast<gsi::VariantAdaptorImpl<tl::Variant> *> (a.get ());
      PythonBasedVariantAdaptor *pa = dynamic_cast<PythonBasedVariantAdaptor *> (a.get ());
      if (aa) {
        //  A small optimization that saves one variant copy
        *ret = object_from_variant (aa->var_ref_nc (), self, atype);
      } else if (pa) {
        //  Optimization for Python to Python transfer
        *ret = pa->ptr ();
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
  void operator() (gsi::SerialArgs *rr, PythonRef *ret, PYAObjectBase * /*self*/, const gsi::ArgType &atype, tl::Heap *heap)
  {
    std::unique_ptr<gsi::VectorAdaptor> a ((gsi::VectorAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *ret = PythonRef (Py_None, false /*borrowed*/);
    } else {
      *ret = PyList_New (0);
      tl_assert (atype.inner () != 0);
      PythonBasedVectorAdaptor t (*ret, atype.inner ());
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
  void operator() (gsi::SerialArgs *rr, PythonRef *ret, PYAObjectBase * /*self*/, const gsi::ArgType &atype, tl::Heap *heap)
  {
    std::unique_ptr<gsi::MapAdaptor> a ((gsi::MapAdaptor *) rr->read<void *>(*heap));
    if (!a.get ()) {
      *ret = PythonRef (Py_None, false /*borrowed*/);
    } else {
      *ret = PyDict_New ();
      tl_assert (atype.inner () != 0);
      tl_assert (atype.inner_k () != 0);
      PythonBasedMapAdaptor t (*ret, atype.inner (), atype.inner_k ());
      a->copy_to (&t, *heap);
    }
  }
};

/**
 *  @brief Deserialization wrapper: specialization for object
 */
template <>
struct reader<gsi::ObjectType>
{
  void operator() (gsi::SerialArgs *rr, PythonRef *ret, PYAObjectBase *self, const gsi::ArgType &atype, tl::Heap *heap)
  {
    void *obj = rr->read<void *> (*heap);
    if (! obj) {
      *ret = PythonRef (Py_None, false /*borrowed*/);
    } else {
      *ret = object_to_python (obj, self, atype);
    }
  }
};

/**
 *  @brief Deserialization wrapper: specialization for void
 */
template <>
struct reader<gsi::VoidType>
{
  void operator() (gsi::SerialArgs *, PythonRef *, PYAObjectBase *, const gsi::ArgType &, tl::Heap *)
  {
    //  .. nothing: void is not serialized
  }
};

PythonRef
pop_arg (const gsi::ArgType &atype, gsi::SerialArgs &aserial, PYAObjectBase *self, tl::Heap &heap)
{
  PythonRef ret;
  gsi::do_on_type<reader> () (atype.type (), &aserial, &ret, self, atype, &heap);
  return ret;
}

// ------------------------------------------------------------------
//  PythonBasedVariantAdaptor implementation

PythonBasedVariantAdaptor::PythonBasedVariantAdaptor (const PythonPtr &var)
  : m_var (var)
{
  //  .. nothing yet ..
}

tl::Variant PythonBasedVariantAdaptor::var () const
{
  return python2c<tl::Variant> (m_var.get ());
}

void PythonBasedVariantAdaptor::set (const tl::Variant & /*v*/, tl::Heap & /*heap*/)
{
  //  TODO: is there a setter for a string?
}

// ---------------------------------------------------------------------
//  PythonBasedVectorAdaptorIterator implementation

PythonBasedVectorAdaptorIterator::PythonBasedVectorAdaptorIterator (const PythonPtr &array, size_t len, const gsi::ArgType *ainner)
  : m_array (array), m_i (0), m_len (len), mp_ainner (ainner)
{
  //  .. nothing yet ..
}

void PythonBasedVectorAdaptorIterator::get (gsi::SerialArgs &w, tl::Heap &heap) const
{
  PyObject *member = NULL;
  if (PyTuple_Check (m_array.get ())) {
    member = PyTuple_GetItem (m_array.get (), m_i);
  } else if (PyList_Check (m_array.get ())) {
    member = PyList_GetItem (m_array.get (), m_i);
  }
  gsi::do_on_type<writer> () (mp_ainner->type (), &w, member, *mp_ainner, &heap);
}

bool PythonBasedVectorAdaptorIterator::at_end () const
{
  return m_i == m_len;
}

void PythonBasedVectorAdaptorIterator::inc ()
{
  ++m_i;
}

// ---------------------------------------------------------------------
//  PythonBasedVectorAdaptor implementation

PythonBasedVectorAdaptor::PythonBasedVectorAdaptor (const PythonPtr &array, const gsi::ArgType *ainner)
  : mp_ainner (ainner), m_array (array)
{
  //  .. nothing yet ..
}

gsi::VectorAdaptorIterator *PythonBasedVectorAdaptor::create_iterator () const
{
  return new PythonBasedVectorAdaptorIterator (m_array, size (), mp_ainner);
}

void PythonBasedVectorAdaptor::push (gsi::SerialArgs &r, tl::Heap &heap)
{
  if (PyList_Check (m_array.get ())) {
    PythonRef member;
    gsi::do_on_type<reader> () (mp_ainner->type (), &r, &member, (PYAObjectBase *) 0, *mp_ainner, &heap);
    PyList_Append (m_array.get (), member.get ());
  } else if (PyTuple_Check (m_array.get ())) {
    throw tl::Exception (tl::to_string (tr ("Tuples cannot be modified and cannot be used as out parameters")));
  }
}

void PythonBasedVectorAdaptor::clear ()
{
  if (PySequence_Check (m_array.get ())) {
    PySequence_DelSlice (m_array.get (), 0, PySequence_Length (m_array.get ()));
  }
}

size_t PythonBasedVectorAdaptor::size () const
{
  if (PySequence_Check (m_array.get ())) {
    return PySequence_Length (m_array.get ());
  } else {
    return 0;
  }
}

size_t PythonBasedVectorAdaptor::serial_size () const
{
  return mp_ainner->size ();
}

// ---------------------------------------------------------------------
//  PythonBasedMapAdaptorIterator implementation

PythonBasedMapAdaptorIterator::PythonBasedMapAdaptorIterator (const PythonPtr &hash, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k)
  : mp_ainner (ainner), mp_ainner_k (ainner_k)
{
  m_pos = 0;
  m_hash = hash;
  inc ();
}

void PythonBasedMapAdaptorIterator::get (gsi::SerialArgs &w, tl::Heap &heap) const
{
  gsi::do_on_type<writer> () (mp_ainner_k->type (), &w, m_key, *mp_ainner_k, &heap);
  gsi::do_on_type<writer> () (mp_ainner->type (), &w, m_value, *mp_ainner, &heap);
}

bool PythonBasedMapAdaptorIterator::at_end () const
{
  return ! m_has_items;
}

void PythonBasedMapAdaptorIterator::inc ()
{
  m_has_items = PyDict_Next(m_hash.get (), &m_pos, &m_key, &m_value);
}

// ---------------------------------------------------------------------
//  PythonBasedMapAdaptor implementation

PythonBasedMapAdaptor::PythonBasedMapAdaptor (const PythonPtr &hash, const gsi::ArgType *ainner, const gsi::ArgType *ainner_k)
  : mp_ainner (ainner), mp_ainner_k (ainner_k), m_hash (hash)
{
}

gsi::MapAdaptorIterator *PythonBasedMapAdaptor::create_iterator () const
{
  return new PythonBasedMapAdaptorIterator (m_hash, mp_ainner, mp_ainner_k);
}

void PythonBasedMapAdaptor::insert (gsi::SerialArgs &r, tl::Heap &heap)
{
  PythonRef k, v;
  gsi::do_on_type<reader> () (mp_ainner_k->type (), &r, &k, (PYAObjectBase *) 0, *mp_ainner_k, &heap);
  gsi::do_on_type<reader> () (mp_ainner->type (), &r, &v, (PYAObjectBase *) 0, *mp_ainner, &heap);
  PyDict_SetItem (m_hash.get (), k.get (), v.get ());
}

void PythonBasedMapAdaptor::clear ()
{
  PyDict_Clear (m_hash.get ());
}

size_t PythonBasedMapAdaptor::size () const
{
  return PyDict_Size (m_hash.get ());
}

size_t PythonBasedMapAdaptor::serial_size () const
{
  return mp_ainner_k->size () + mp_ainner->size ();
}

// -------------------------------------------------------------------
//  Test if an argument can be converted to the given type

//  if atype is a vector:
//      argument must be an array of the given type
//  if atype is a ref:
//      argument must be a boxed type of the required type or an object of the requested class
//  if atype is a ptr:
//      argument must be a boxed type of the required type or an object of the requested class or null
//  if atype is a cptr:
//      argument must be of requested type or null
//  otherwise:
//      argument must be of the requested type

template <class R>
struct test_arg_func
{
  void operator() (bool *ret, PyObject *arg, const gsi::ArgType &atype, bool loose)
  {
    if ((atype.is_cptr () || atype.is_ptr ()) && arg == Py_None) {

      //  for const X * or X *, null is an allowed value
      *ret = true;

    } else {

      *ret = false;

      if (atype.is_ptr () || atype.is_ref ()) {

        //  check if we have a boxed type
        const gsi::ClassBase *cls_decl = PythonModule::cls_for_type (Py_TYPE (arg));
        if (cls_decl) {
          const gsi::ClassBase *bc = gsi::cls_decl <gsi::Value> ();
          if (cls_decl->is_derived_from (bc)) {
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

template <>
struct test_arg_func<gsi::VariantType>
{
  void operator() (bool *ret, PyObject *, const gsi::ArgType &, bool)
  {
    //  we assume we can convert everything into a variant
    *ret = true;
  }
};

template <>
struct test_arg_func<gsi::StringType>
{
  void operator() (bool *ret, PyObject *arg, const gsi::ArgType &, bool)
  {
#if PY_MAJOR_VERSION < 3
    if (PyString_Check (arg)) {
      *ret = true;
    } else
#else
    if (PyBytes_Check (arg)) {
      *ret = true;
    } else
#endif
    if (PyUnicode_Check (arg)) {
      *ret = true;
    } else if (PyByteArray_Check (arg)) {
      *ret = true;
    } else {
      *ret = false;
    }
  }
};

template <>
struct test_arg_func<gsi::VectorType>
{
  void operator() (bool *ret, PyObject *arg, const gsi::ArgType &atype, bool loose)
  {
    if ((atype.is_cptr () || atype.is_ptr ()) && arg == Py_None) {
      //  for ptr or cptr, null is an allowed value
      *ret = true;
      return;
    }

    if (! PyTuple_Check (arg) && ! PyList_Check (arg)) {
      *ret = false;
      return;
    }

    tl_assert (atype.inner () != 0);
    const gsi::ArgType &ainner = *atype.inner ();

    *ret = true;
    if (PyTuple_Check (arg)) {

      size_t n = PyTuple_Size (arg);
      for (size_t i = 0; i < n && *ret; ++i) {
        if (! test_arg (ainner, PyTuple_GetItem (arg, i), loose)) {
          *ret = false;
        }
      }

    } else {

      size_t n = PyList_Size (arg);
      for (size_t i = 0; i < n && *ret; ++i) {
        if (! test_arg (ainner, PyList_GetItem (arg, i), loose)) {
          *ret = false;
        }
      }

    }
  }
};

template <>
struct test_arg_func<gsi::MapType>
{
  void operator () (bool *ret, PyObject *arg, const gsi::ArgType &atype, bool loose)
  {
    if ((atype.is_cptr () || atype.is_ptr ()) && arg == Py_None) {
      //  for ptr or cptr, null is an allowed value
      *ret = true;
      return;
    }

    if (! PyDict_Check (arg)) {
      *ret = false;
      return;
    }

    tl_assert (atype.inner () != 0);
    tl_assert (atype.inner_k () != 0);
    const gsi::ArgType &ainner = *atype.inner ();
    const gsi::ArgType &ainner_k = *atype.inner ();

    //  Note: we test key and value separately. That way we don't need to
    //  instantiate a 2d template with do_on_type2.
    *ret = true;

    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(arg, &pos, &key, &value)) {
      if (! test_arg (ainner_k, key, loose)) {
        *ret = false;
        break;
      }
      if (! test_arg (ainner, value, loose)) {
        *ret = false;
        break;
      }
    }
  }
};

template <>
struct test_arg_func<gsi::ObjectType>
{
  void operator() (bool *ret, PyObject *arg, const gsi::ArgType &atype, bool loose)
  {
    //  for const X * or X *, null is an allowed value
    if ((atype.is_cptr () || atype.is_ptr ()) && arg == Py_None) {
      *ret = true;
      return;
    }

    const gsi::ClassBase *cls_decl = PythonModule::cls_for_type (Py_TYPE (arg));
    if (! cls_decl) {
      *ret = false;
      return;
    }

    if (! (cls_decl == atype.cls () || (loose && (cls_decl->is_derived_from (atype.cls ()) || cls_decl->can_convert_to(atype.cls ()))))) {
      *ret = false;
      return;
    }
    if ((atype.is_ref () || atype.is_ptr ()) && ((PYAObjectBase *) arg)->const_ref ()) {
      *ret = false;
      return;
    }

    *ret = true;

  }
};

bool
test_arg (const gsi::ArgType &atype, PyObject *arg, bool loose)
{
  bool ret = false;
  gsi::do_on_type<test_arg_func> () (atype.type (), &ret, arg, atype, loose);
  return ret;
}

}

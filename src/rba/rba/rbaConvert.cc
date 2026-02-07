
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
#include "rbaConvert.h"
#include "rbaInternal.h"

#include "gsiDecl.h"

#include <ruby/encoding.h>

namespace rba
{

static int push_map_i (VALUE key, VALUE value, VALUE arg)
{
  std::vector<std::pair<VALUE, VALUE> > *v = (std::vector<std::pair<VALUE, VALUE> > *) arg;
  v->push_back (std::make_pair (key, value));
  return ST_CONTINUE;
}

template <>
tl::Variant ruby2c<tl::Variant> (VALUE rval)
{
  if (FIXNUM_P (rval)) {
    return tl::Variant (ruby2c<long> (rval));
  } else if (rval == Qnil) {
    return tl::Variant ();
  } else if (rval == Qfalse) {
    return tl::Variant (false);
  } else if (rval == Qtrue) {
    return tl::Variant (true);
  } else if (TYPE (rval) == T_BIGNUM) {
    return tl::Variant (ruby2c<long long> (rval));
  } else if (TYPE (rval) == T_FLOAT) {
    return tl::Variant (ruby2c<double> (rval));
  } else if (TYPE (rval) == T_HASH) {

    std::vector<std::pair<VALUE, VALUE> > kv;
    kv.reserve (RHASH_SIZE (rval));
    //  Note: we use this scheme rather than directly inserting into the variant because
    //  the rb_hash_foreach is not exception-safe.
    rb_hash_foreach (rval, (int (*)(...)) &push_map_i, (VALUE) &kv);

    tl::Variant r;
    r.set_array ();
    for (std::vector<std::pair<VALUE, VALUE> >::const_iterator i = kv.begin (); i != kv.end (); ++i) {
      r.insert (ruby2c<tl::Variant> (i->first), ruby2c<tl::Variant> (i->second));
    }
    return r;

  } else if (TYPE (rval) == T_ARRAY) {

    unsigned int len = RARRAY_LEN(rval);
    VALUE *el = RARRAY_PTR(rval);

    static std::vector<tl::Variant> empty;
    tl::Variant r (empty.begin (), empty.end ());
    r.get_list ().reserve (len);
    while (len-- > 0) {
      r.get_list ().push_back (ruby2c<tl::Variant> (*el++));
    }
    return r;

  } else if (TYPE (rval) == T_DATA) {

    //  some types are supported through "complex" tl::Variant's
    Proxy *p = 0;
    Data_Get_Struct (rval, Proxy, p);

    //  employ the tl::Variant binding capabilities of the Expression binding to derive the
    //  variant value.

    const gsi::ClassBase *cls = p->cls_decl ();

    void *obj = p->obj ();
    if (! obj) {
      return tl::Variant ();
    }

    if (cls->is_managed ()) {

      const tl::VariantUserClassBase *var_cls = cls->var_cls (p->const_ref ());
      tl_assert (var_cls != 0);

      gsi::Proxy *gsi_proxy = cls->gsi_object (obj)->find_client<gsi::Proxy> ();
      if (!gsi_proxy) {
        //  establish a new proxy
        gsi_proxy = new gsi::Proxy (cls);
        gsi_proxy->set (obj, false, p->const_ref (), false);
      }

      tl::Variant out;
      out.set_user_ref (gsi_proxy, var_cls, false);
      return out;

    } else {
      //  No reference management available: deep copy mode.
      return tl::Variant (cls->clone (obj), cls->var_cls (false), true);
    }

  } else if (TYPE (rval) == T_STRING) {

    //  UTF-8 encoded strings are taken to be string, others are byte strings
    //  At least this ensures consistency for a full Ruby-C++ turnaround cycle.
    if (rb_enc_from_index (rb_enc_get_index (rval)) == rb_utf8_encoding ()) {
      return tl::Variant (ruby2c<std::string> (rval));
    } else {
      return tl::Variant (ruby2c<std::vector<char> > (rval));
    }

  } else {
    return tl::Variant (ruby2c<std::string> (rba_safe_obj_as_string (rval)));
  }
}

VALUE object_to_ruby (void *obj, Proxy *self, const gsi::ArgType &atype)
{
  const gsi::ClassBase *cls = atype.cls()->subclass_decl (obj);

  bool is_direct   = !(atype.is_ptr () || atype.is_ref () || atype.is_cptr () || atype.is_cref ());
  bool pass_obj    = atype.pass_obj () || is_direct;
  bool is_const    = atype.is_cptr () || atype.is_cref ();
  bool prefer_copy = atype.prefer_copy ();
  bool can_destroy = prefer_copy || atype.is_ptr ();

  return object_to_ruby (obj, self, cls, pass_obj, is_const, prefer_copy, can_destroy);
}

/**
 *  @brief Correct constness if a reference is const and a non-const reference is required
 *  HINT: this is a workaround for the fact that unlike C++, Ruby does not have const or non-const
 *  references. Since a reference is identical with the object it points to, there are only const or non-const
 *  objects. We deliver const objects first, but if a non-const version is requested, the
 *  object turns into a non-const one. This may be confusing but provides a certain level
 *  of "constness", at least until there is another non-const reference for that object.
 */
static void correct_constness (Proxy *p, bool const_required)
{
  if (p && p->const_ref () && ! const_required) {
    //  promote to non-const object
    p->set_const_ref (false);
  }
}

VALUE
object_to_ruby (void *obj, Proxy *self, const gsi::ClassBase *cls, bool pass_obj, bool is_const, bool prefer_copy, bool can_destroy)
{
  VALUE ret = Qnil;
  if (! obj || ! cls) {
    return ret;
  }

  const gsi::ClassBase *clsact = cls->subclass_decl (obj);
  if (! clsact) {
    return ret;
  }

  //  Derive a Proxy reference if the object is already bound
  Proxy *rba_data = 0;
  if (self && self->obj () == obj) {

    //  reuse "self" if the object to convert is self.
    rba_data = self;

  } else if (! clsact->adapted_type_info () && clsact->is_managed ()) {

    rba_data = clsact->gsi_object (obj)->find_client<Proxy> ();
    if (rba_data) {

      //  Don't use objects that are T_ZOMBIE or otherwise unusable
      if (BUILTIN_TYPE (rba_data->self ()) != T_DATA) {
        rba_data->detach ();
        rba_data = 0;
        //  must be the last and only Proxy for this object
        tl_assert (clsact->gsi_object (obj)->find_client<Proxy> () == 0);
      }

    }

  } else if (clsact->adapted_type_info ()) {

    //  create an adaptor from an adapted type
    if (pass_obj) {
      obj = clsact->create_from_adapted_consume (obj);
    } else {
      obj = clsact->create_from_adapted (obj);
    }

    //  we wil own the new object
    pass_obj = true;

  }

  if (! pass_obj && prefer_copy && ! clsact->adapted_type_info () && ! clsact->is_managed () && clsact->can_copy () && clsact->can_default_create ()) {

    //  We copy objects passed by const reference if they are not managed.
    //  Such objects are often exposed internals. First we can't
    //  guarantee the const correctness of references. Second, we
    //  can't guarantee the lifetime of the container will exceed that
    //  of the exposed property. Hence copying is safer.

    //  create a instance and copy the value
    ret = rb_obj_alloc (ruby_cls (clsact, false));
    Proxy *p = 0;
    Data_Get_Struct (ret, Proxy, p);
    clsact->assign (p->obj (), obj);

  } else if (rba_data && rba_data->self () != Qnil) {

    //  we have a that is located in C++ space but is supposed to get attached
    //  a Ruby object. If it already has, we simply return a reference to this.
    ret = rba_data->self ();

#if HAVE_RUBY_VERSION_CODE >= 20200 && HAVE_RUBY_VERSION_CODE < 30000
    //  Mark the returned object - the original one may have been already
    //  scheduled for sweeping. This happens at least for Ruby 2.3 which
    //  has a two-phase GC (mark and sweep in separate steps). If by chance
    //  the marking has been done already, we must not return this object
    //  unmarked. This happens in GC_LAZY_SWEEP mode which became popular
    //  with Ruby 2.2.
    rb_gc_mark_maybe (ret);
#endif

    //  correct constness if the object is not supposed to be const
    correct_constness (rba_data, is_const);

  } else {

    //  create an instance
    //  TODO: we will create a fresh object here, delete it again and link the
    //  reference to the existing object to the Ruby object. This is not quite
    //  efficient - we should avoid creating and deleting a dummy object first.
    ret = rb_obj_alloc (ruby_cls (clsact, false));
    Proxy *p = 0;
    Data_Get_Struct (ret, Proxy, p);
    p->set (obj, pass_obj, is_const /*const*/, can_destroy /*can_destroy*/, ret);

  }

  return ret;
}

template <>
VALUE c2ruby<tl::Variant> (const tl::Variant &c)
{
  if (c.is_double ()) {
    return c2ruby<double> (c.to_double ());
  } else if (c.is_bool ()) {
    return c2ruby<bool> (c.to_bool ());
  } else if (c.is_a_string ()) {
    return c2ruby<std::string> (c.to_stdstring ());
  } else if (c.is_a_bytearray ()) {
    return c2ruby<std::vector<char> > (c.to_bytearray ());
  } else if (c.is_long () || c.is_char ()) {
    return c2ruby<long> (c.to_long ());
  } else if (c.is_ulong ()) {
    return c2ruby<unsigned long> (c.to_ulong ());
  } else if (c.is_longlong ()) {
    return c2ruby<long long> (c.to_longlong ());
  } else if (c.is_ulonglong ()) {
    return c2ruby<unsigned long long> (c.to_ulonglong ());
  } else if (c.is_array ()) {
    VALUE ret = rb_hash_new ();
    for (tl::Variant::const_array_iterator i = c.begin_array (); i != c.end_array (); ++i) {
      rb_hash_aset (ret, c2ruby<tl::Variant> (i->first), c2ruby<tl::Variant> (i->second));
    }
    return ret;
  } else if (c.is_list ()) {
    VALUE ret = rb_ary_new ();
    for (tl::Variant::const_iterator i = c.begin (); i != c.end (); ++i) {
      rb_ary_push (ret, c2ruby<tl::Variant> (*i));
    }
    return ret;
  } else if (c.is_user ()) {
    const gsi::ClassBase *cls = c.gsi_cls ();
    if (cls) {
      if (! c.user_is_ref () && cls->is_managed ()) {
        void *obj = c.user_unshare ();
        return object_to_ruby (obj, 0, c.user_cls ()->gsi_cls (), true, c.user_is_const (), false, false);
      } else {
        void *obj = const_cast<void *> (c.to_user ());
        return object_to_ruby (obj, 0, c.user_cls ()->gsi_cls (), false, false, true, false);
      }
    } else {
      //  not a known type -> return nil
      return Qnil;
    }
  } else {
    return Qnil;
  }
}

}

#endif

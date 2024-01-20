
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


#include "rba.h"
#include "rbaInspector.h"
#include "rbaConvert.h"
#include "rbaInternal.h"
#include "rbaMarshal.h"

#include "gsiInspector.h"
#include "gsiDecl.h"

//  No inspector support for Ruby < 2.0

#if defined(HAVE_RUBY) && HAVE_RUBY_VERSION_CODE >= 20000

#include <ruby.h>
#include <signal.h>

#include <vector>
#include <algorithm>
#include <string>

namespace rba
{

static gsi::Inspector *create_inspector_for_object (VALUE value);
static bool has_inspector (VALUE value);

class RBAArrayInspector
  : public gsi::Inspector
{
public:
  RBAArrayInspector (VALUE array)
    : m_array (array)
  {
    rb_gc_register_address (&m_array);
  }

  ~RBAArrayInspector ()
  {
    rb_gc_unregister_address (&m_array);
    m_array = Qnil;
  }

  std::string description () const
  {
    return std::string ("...");
  }

  bool has_keys () const
  {
    return false;
  }

  VALUE rb_value (size_t index) const
  {
    return rb_ary_entry (m_array, long (index));
  }

  virtual std::string type (size_t index) const
  {
    return ruby2c<std::string> (rba_safe_obj_as_string (rb_class_of (rb_value (index))));
  }

  virtual Visibility visibility (size_t /*index*/) const
  {
    return gsi::Inspector::Always;
  }

  virtual tl::Variant value (size_t index) const
  {
    return ruby2c<tl::Variant> (rb_value (index));
  }

  virtual size_t count () const
  {
    return TYPE (m_array) == T_ARRAY ? RARRAY_LEN (m_array) : 0;
  }

  virtual bool has_children (size_t index) const
  {
    return has_inspector (rb_value (index));
  }

  virtual gsi::Inspector *child_inspector (size_t index) const
  {
    return create_inspector_for_object (rb_value (index));
  }

  virtual bool equiv (const gsi::Inspector *other) const
  {
    const RBAArrayInspector *o = dynamic_cast<const RBAArrayInspector *> (other);
    return o && o->m_array == m_array;
  }

private:
  VALUE m_array;
};

static int push_key_to_ary_i (VALUE key, VALUE /*value*/, VALUE a)
{
  rb_ary_push (a, key);
  return ST_CONTINUE;
}

class RBAHashInspector
  : public gsi::Inspector
{
public:
  RBAHashInspector (VALUE hash)
    : m_hash (hash)
  {
    rb_gc_register_address (&m_hash);
    m_keys = rb_ary_new2 (long (RHASH_SIZE (m_hash)));
    rb_gc_register_address (&m_keys);
    rb_hash_foreach (m_hash, (int (*)(...)) &push_key_to_ary_i, m_keys);
  }

  ~RBAHashInspector ()
  {
    rb_gc_unregister_address (&m_hash);
    rb_gc_unregister_address (&m_keys);
    m_hash = Qnil;
    m_keys = Qnil;
  }

  std::string description () const
  {
    return std::string ("...");
  }

  VALUE rb_key (size_t index) const
  {
    return rb_ary_entry (m_keys, long (index));
  }

  VALUE rb_value (size_t index) const
  {
    return rb_hash_fetch (m_hash, rb_key (index));
  }

  virtual tl::Variant keyv (size_t index) const
  {
    return ruby2c<tl::Variant> (rb_key (index));
  }

  virtual std::string type (size_t index) const
  {
    return ruby2c<std::string> (rba_safe_obj_as_string (rb_class_of (rb_value (index))));
  }

  virtual Visibility visibility (size_t /*index*/) const
  {
    return gsi::Inspector::Always;
  }

  virtual tl::Variant value (size_t index) const
  {
    return ruby2c<tl::Variant> (rb_value (index));
  }

  virtual size_t count () const
  {
    return RARRAY_LEN (m_keys);
  }

  virtual bool has_children (size_t index) const
  {
    return has_inspector (rb_value (index));
  }

  virtual gsi::Inspector *child_inspector (size_t index) const
  {
    return create_inspector_for_object (rb_value (index));
  }

  virtual bool equiv (const gsi::Inspector *other) const
  {
    const RBAHashInspector *o = dynamic_cast<const RBAHashInspector *> (other);
    return o && o->m_hash == m_hash;
  }

private:
  VALUE m_hash;
  VALUE m_keys;
};

class RBAObjectInspector
  : public gsi::Inspector
{
public:
  RBAObjectInspector (VALUE obj)
    : m_obj (obj), m_members (Qnil)
  {
    rb_gc_register_address (&m_obj);
    m_members = rb_obj_instance_variables (m_obj);
    rb_gc_register_address (&m_members);
  }

  ~RBAObjectInspector ()
  {
    rb_gc_unregister_address (&m_obj);
    m_obj = Qnil;
    rb_gc_unregister_address (&m_members);
    m_members = Qnil;
  }

  std::string description () const
  {
    return ruby2c<std::string> (rba_safe_inspect (m_obj));
  }

  VALUE rb_key (size_t index) const
  {
    if (index == 0) {
      return rb_class_of (m_obj);
    }
    --index;

    return rb_ary_entry (m_members, long (index));
  }

  VALUE rb_value (size_t index) const
  {
    if (index == 0) {
      return rb_class_of (m_obj);
    }
    --index;

    VALUE var = rb_ary_entry (m_members, long (index));
    if (TYPE (var) == T_SYMBOL) {
      return rb_ivar_get (m_obj, SYM2ID (var));
    } else {
      return Qnil;
    }
  }

  virtual std::string key (size_t index) const
  {
    return ruby2c<std::string> (rba_safe_obj_as_string (rb_key (index)));
  }

  virtual std::string type (size_t index) const
  {
    return ruby2c<std::string> (rba_safe_obj_as_string (rb_class_of (rb_value (index))));
  }

  virtual Visibility visibility (size_t /*index*/) const
  {
    return gsi::Inspector::Always;
  }

  virtual tl::Variant value (size_t index) const
  {
    return ruby2c<tl::Variant> (rb_value (index));
  }

  virtual size_t count () const
  {
    return 1 + RARRAY_LEN (m_members);
  }

  virtual bool has_children (size_t index) const
  {
    return has_inspector (rb_value (index));
  }

  virtual gsi::Inspector *child_inspector (size_t index) const
  {
    return create_inspector_for_object (rb_value (index));
  }

  virtual bool equiv (const gsi::Inspector *other) const
  {
    const RBAObjectInspector *o = dynamic_cast<const RBAObjectInspector *> (other);
    return o && o->m_obj == m_obj;
  }

private:
  VALUE m_obj, m_members;
};

static void collect_getters (const gsi::ClassBase *cls, std::vector<std::pair<std::string, const gsi::MethodBase *> > &getters)
{
  if (cls->base ()) {
    collect_getters (cls->base (), getters);
  }

  //  properties are identified by a setter - the corresponding non-setter method is the getter then.
  std::set<std::string> setter_names;
  for (gsi::ClassBase::method_iterator m = cls->begin_methods (); m != cls->end_methods (); ++m) {
    if (! (*m)->is_callback ()) {
      for (gsi::MethodBase::synonym_iterator s = (*m)->begin_synonyms (); s != (*m)->end_synonyms (); ++s) {
        if (s->is_setter) {
          setter_names.insert (s->name);
        }
      }
    }
  }

  for (gsi::ClassBase::method_iterator m = cls->begin_methods (); m != cls->end_methods (); ++m) {
    if (! (*m)->is_callback ()) {
      for (gsi::MethodBase::synonym_iterator s = (*m)->begin_synonyms (); s != (*m)->end_synonyms (); ++s) {
        if (s->is_getter || (! s->is_setter && setter_names.find (s->name) != setter_names.end ())) {
          getters.push_back (std::make_pair (s->name, *m));
        }
      }
    }
  }
}

class RBADataInspector
  : public gsi::Inspector
{
public:
  RBADataInspector (VALUE obj)
    : m_obj (obj), mp_cls (0), m_members (Qnil)
  {
    rb_gc_register_address (&m_obj);
    mp_cls = find_cclass_maybe_null (rb_class_of (m_obj));
    m_members = rb_obj_instance_variables (m_obj);
    rb_gc_register_address (&m_members);

    if (mp_cls) {
      collect_getters (mp_cls, m_getters);
    }
  }

  ~RBADataInspector ()
  {
    rb_gc_unregister_address (&m_obj);
    m_obj = Qnil;
    rb_gc_unregister_address (&m_members);
    m_members = Qnil;
  }

  std::string description () const
  {
    return ruby2c<std::string> (rba_safe_inspect (m_obj));
  }

  virtual std::string key (size_t index) const
  {
    if (index == 0) {
      return rba_class_name (m_obj);
    }
    --index;

    if (index < size_t (RARRAY_LEN (m_members))) {
      VALUE var = rb_ary_entry (m_members, long (index));
      return ruby2c<std::string> (rba_safe_obj_as_string (var));
    }
    index -= RARRAY_LEN (m_members);

    if (index < m_getters.size ()) {
      return m_getters [index].first;
    } else {
      return std::string ();
    }
  }

  VALUE rb_value (size_t index) const
  {
    if (index == 0) {
      return rb_class_of (m_obj);
    }
    --index;

    if (index < size_t (RARRAY_LEN (m_members))) {
      VALUE var = rb_ary_entry (m_members, long (index));
      if (TYPE (var) == T_SYMBOL) {
        return rb_ivar_get (m_obj, SYM2ID (var));
      } else {
        return Qnil;
      }
    }
    index -= RARRAY_LEN (m_members);

    if (index < m_getters.size ()) {

      const gsi::MethodBase *meth = m_getters[index].second;

      Proxy *p = 0;
      Data_Get_Struct (m_obj, Proxy, p);
      void *obj = 0;
      if (p) {
        //  Hint: this potentially instantiates the object
        obj = p->obj ();
      }

      if (obj) {

        gsi::SerialArgs retlist (meth->retsize ());
        gsi::SerialArgs arglist (meth->argsize ());
        meth->call (obj, arglist, retlist);

        tl::Heap heap;
        return pull_arg (meth->ret_type (), p, retlist, heap);

      }

    }

    return Qnil;
  }

  virtual std::string type (size_t index) const
  {
    return ruby2c<std::string> (rba_safe_obj_as_string (rb_class_of (rb_value (index))));
  }

  virtual Visibility visibility (size_t /*index*/) const
  {
    return gsi::Inspector::Always;
  }

  virtual tl::Variant value (size_t index) const
  {
    return ruby2c<tl::Variant> (rb_value (index));
  }

  virtual size_t count () const
  {
    return 1 + RARRAY_LEN (m_members) + m_getters.size ();
  }

  virtual bool has_children (size_t index) const
  {
    return has_inspector (rb_value (index));
  }

  virtual gsi::Inspector *child_inspector (size_t index) const
  {
    return create_inspector_for_object (rb_value (index));
  }

  virtual bool equiv (const gsi::Inspector *other) const
  {
    const RBADataInspector *o = dynamic_cast<const RBADataInspector *> (other);
    return o && o->m_obj == m_obj;
  }

private:
  VALUE m_obj;
  const gsi::ClassBase *mp_cls;
  VALUE m_members;
  std::vector<std::pair<std::string, const gsi::MethodBase *> > m_getters;
};

class RBAClassInspector
  : public gsi::Inspector
{
public:
  RBAClassInspector (VALUE cls)
    : m_class (cls), m_members (Qnil)
  {
    rb_gc_register_address (&m_class);
    m_members = rb_mod_class_variables (0, NULL, m_class);
    rb_gc_register_address (&m_members);
  }

  ~RBAClassInspector ()
  {
    rb_gc_unregister_address (&m_class);
    m_class = Qnil;
    rb_gc_unregister_address (&m_members);
    m_members = Qnil;
  }

  std::string description () const
  {
    return ruby2c<std::string> (rba_safe_obj_as_string (m_class));
  }

  VALUE rb_key (size_t index) const
  {
    return rb_ary_entry (m_members, long (index));
  }

  VALUE rb_value (size_t index) const
  {
    VALUE var = rb_ary_entry (m_members, long (index));
    if (TYPE (var) == T_SYMBOL) {
      return rb_ivar_get (m_class, SYM2ID (var));
    } else {
      return Qnil;
    }
  }

  virtual std::string key (size_t index) const
  {
    return ruby2c<std::string> (rba_safe_obj_as_string (rb_key (index)));
  }

  virtual std::string type (size_t index) const
  {
    return ruby2c<std::string> (rba_safe_obj_as_string (rb_class_of (rb_value (index))));
  }

  virtual Visibility visibility (size_t /*index*/) const
  {
    return gsi::Inspector::Always;
  }

  virtual tl::Variant value (size_t index) const
  {
    return ruby2c<tl::Variant> (rb_value (index));
  }

  virtual size_t count () const
  {
    return RARRAY_LEN (m_members);
  }

  virtual bool has_children (size_t index) const
  {
    return has_inspector (rb_value (index));
  }

  virtual gsi::Inspector *child_inspector (size_t index) const
  {
    return create_inspector_for_object (rb_value (index));
  }

  virtual bool equiv (const gsi::Inspector *other) const
  {
    const RBAClassInspector *o = dynamic_cast<const RBAClassInspector *> (other);
    return o && o->m_class == m_class;
  }

private:
  VALUE m_class, m_members;
};

class RBABindingInspector
  : public gsi::Inspector
{
public:
  RBABindingInspector (int context)
    : m_context (context)
  {
    m_local_variables = rba_eval_string_in_context ("local_variables", 0, 0, m_context);
    rb_gc_register_address (&m_local_variables);
  }

  ~RBABindingInspector ()
  {
    rb_gc_unregister_address (&m_local_variables);
    m_local_variables = Qnil;
  }

  std::string description () const
  {
    return std::string ();
  }

  virtual std::string key (size_t index) const
  {
    return ruby2c<std::string> (rba_safe_obj_as_string (rb_ary_entry (m_local_variables, long (index))));
  }

  VALUE rb_value (size_t index) const
  {
    return rba_eval_string_in_context (key (index).c_str (), 0, 0, m_context);
  }

  virtual std::string type (size_t index) const
  {
    return ruby2c<std::string> (rba_safe_obj_as_string (rb_class_of (rb_value (index))));
  }

  virtual Visibility visibility (size_t /*index*/) const
  {
    return gsi::Inspector::Always;
  }

  virtual tl::Variant value (size_t index) const
  {
    return ruby2c<tl::Variant> (rb_value (index));
  }

  virtual size_t count () const
  {
    return TYPE (m_local_variables) == T_ARRAY ? RARRAY_LEN (m_local_variables) : 0;
  }

  virtual bool has_children (size_t index) const
  {
    return has_inspector (rb_value (index));
  }

  virtual gsi::Inspector *child_inspector (size_t index) const
  {
    return create_inspector_for_object (rb_value (index));
  }

  virtual bool equiv (const gsi::Inspector *other) const
  {
    const RBABindingInspector *o = dynamic_cast<const RBABindingInspector *> (other);
    return o && o->m_context == m_context;
  }

private:
  int m_context;
  VALUE m_local_variables;
};

static bool has_inspector (VALUE value)
{
  return (TYPE (value) == T_ARRAY || TYPE (value) == T_HASH || TYPE (value) == T_DATA || TYPE (value) == T_OBJECT || TYPE (value) == T_CLASS);
}

static gsi::Inspector *create_inspector_for_object (VALUE value)
{
  if (TYPE (value) == T_ARRAY) {
    return new RBAArrayInspector (value);
  } else if (TYPE (value) == T_HASH) {
    return new RBAHashInspector (value);
  } else if (TYPE (value) == T_DATA) {
    return new RBADataInspector (value);
  } else if (TYPE (value) == T_OBJECT) {
    return new RBAObjectInspector (value);
  } else if (TYPE (value) == T_CLASS) {
    return new RBAClassInspector (value);
  } else {
    return 0;
  }
}

gsi::Inspector *create_inspector (int context)
{
  return new RBABindingInspector (context);
}

}

#else

namespace rba
{

//  No inspector support
gsi::Inspector *create_inspector (int)
{
  return 0;
}

}

#endif

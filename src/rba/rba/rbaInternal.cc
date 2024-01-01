
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


#if defined(HAVE_RUBY)

#include "rba.h"
#include "rbaInternal.h"
#include "rbaUtils.h"
#include "rbaMarshal.h"

#include "tlLog.h"

namespace rba
{

// --------------------------------------------------------------------------
//  Implementation of the locked object vault

class LockedObjectVault
{
public:
  static void init (VALUE module, const char *name);

  LockedObjectVault ();
  ~LockedObjectVault ();

  static LockedObjectVault *instance ()
  {
    return mp_instance;
  }

  void add (VALUE object);
  void remove (VALUE object);
  void mark_this ();

private:
  std::map<VALUE, size_t> m_objects;

  static VALUE m_klass;
  static VALUE m_instance;
  static LockedObjectVault *mp_instance;

  static void free (void *p);
  static void mark (void *p);
  static VALUE alloc (VALUE klass);
};

VALUE LockedObjectVault::m_klass = Qnil;
VALUE LockedObjectVault::m_instance = Qnil;
LockedObjectVault *LockedObjectVault::mp_instance = 0;

LockedObjectVault::LockedObjectVault ()
{
  mp_instance = this;
}

LockedObjectVault::~LockedObjectVault ()
{
  if (mp_instance == this) {
    mp_instance = 0;
  }
}

void
LockedObjectVault::add (VALUE object)
{
  auto i = m_objects.find (object);
  if (i != m_objects.end ()) {
    i->second += 1;
  } else {
    m_objects.insert (std::make_pair (object, size_t (1)));
  }
}

void
LockedObjectVault::remove (VALUE object)
{
  auto i = m_objects.find (object);
  if (i != m_objects.end ()) {
    i->second -= 1;
    if (i->second == 0) {
      m_objects.erase (i);
    }
  }
}

void
LockedObjectVault::mark_this ()
{
  for (auto o = m_objects.begin (); o != m_objects.end (); ++o) {
    rb_gc_mark (o->first);
  }
}

void
LockedObjectVault::mark (void *p)
{
  ((LockedObjectVault *) p)->mark_this ();
}

void
LockedObjectVault::free (void *p)
{
  delete ((LockedObjectVault *) p);
}

VALUE
LockedObjectVault::alloc (VALUE klass)
{
  tl_assert (TYPE (klass) == T_CLASS);

  LockedObjectVault *vault = new LockedObjectVault ();
  return Data_Wrap_Struct (klass, &LockedObjectVault::mark, &LockedObjectVault::free, vault);
}

void
LockedObjectVault::init (VALUE module, const char *name)
{
  if (m_instance != Qnil) {
    return;
  }

  m_klass = rb_define_class_under (module, name, rb_cObject);
  rb_define_alloc_func (m_klass, LockedObjectVault::alloc);

  m_instance = rba_class_new_instance_checked (0, 0, m_klass);
  rb_gc_register_address (&m_instance);
}

void
make_locked_object_vault (VALUE module)
{
  LockedObjectVault::init (module, "RBALockedObjectVault");
}

void
gc_lock_object (VALUE value)
{
  if (LockedObjectVault::instance ()) {
    LockedObjectVault::instance ()->add (value);
  }
}

void
gc_unlock_object (VALUE value)
{
  if (LockedObjectVault::instance ()) {
    LockedObjectVault::instance ()->remove (value);
  }
}

// --------------------------------------------------------------------------
//  Proxy implementation

Proxy::Proxy (const gsi::ClassBase *_cls_decl)
  : m_cls_decl (_cls_decl),
    m_obj (0),
    m_owned (false),
    m_const_ref (false),
    m_destroyed (false),
    m_can_destroy (false),
    m_self (Qnil)
{
  //  .. nothing yet ..
}

Proxy::~Proxy ()
{
  try {
    reset ();
  } catch (std::exception &ex) {
    tl::warn << "Caught exception in object destructor: " << ex.what ();
  } catch (tl::Exception &ex) {
    tl::warn << "Caught exception in object destructor: " << ex.msg ();
  } catch (...) {
    tl::warn << "Caught unspecified exception in object destructor";
  }
  m_destroyed = true;
}

void
Proxy::mark ()
{
  for (std::map <const gsi::MethodBase *, VALUE>::const_iterator pt = m_signal_handlers.begin (); pt != m_signal_handlers.end (); ++pt) {
    rb_gc_mark (pt->second);
  }
}

VALUE
Proxy::signal_handler (const gsi::MethodBase *meth)
{
  std::map <const gsi::MethodBase *, VALUE>::iterator pt = m_signal_handlers.find (meth);
  if (pt != m_signal_handlers.end ()) {
    return pt->second;
  }

  //  end register the new one
  VALUE args[] = { m_self };
  VALUE sh = rba_class_new_instance_checked (1, args, SignalHandler::klass);

  m_signal_handlers.insert (std::make_pair (meth, sh));

  //  install the handler
  SignalHandler *sig_handler = 0;
  Data_Get_Struct (sh, SignalHandler, sig_handler);
  meth->add_handler (obj (), sig_handler);

  return sh;
}

bool Proxy::can_call () const
{
  return rba::RubyInterpreter::instance () != 0;
}

void
Proxy::call (int id, gsi::SerialArgs &args, gsi::SerialArgs &ret) const
{
  tl_assert (id < int (m_cbfuncs.size ()) && id >= 0);

  const gsi::MethodBase *meth = m_cbfuncs [id].method;
  ID mid  = m_cbfuncs [id].method_id;

  try {

    VALUE argv = rb_ary_new2 (long (std::distance (meth->begin_arguments (), meth->end_arguments ())));
    RB_GC_GUARD (argv);

    tl::Heap heap;

    //  TODO: callbacks with default arguments?
    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); args && a != meth->end_arguments (); ++a) {
      rb_ary_push (argv, pull_arg (*a, 0, args, heap));
    }

    VALUE rb_ret = rba_funcall2_checked (m_self, mid, RARRAY_LEN (argv), RARRAY_PTR (argv));

    push_arg (meth->ret_type (), ret, rb_ret, heap);

    if (meth->ret_type ().pass_obj ()) {
      //  In factory callbacks, make sure the returned object is not deleted by
      //  anyone except the caller.
      Proxy *p = 0;
      Data_Get_Struct (rb_ret, Proxy, p);
      p->keep ();
    }

    //  a Ruby callback must not leave temporary objects
    tl_assert (heap.empty ());

  } catch (rba::RubyError &err) {
    rba::RubyError err_with_context (err);
    err_with_context.set_context (m_cls_decl->name () + "::" + meth->names ());
    throw err_with_context;
  } catch (tl::ExitException & /*ex*/) {
    throw;
  } catch (tl::Exception &ex) {
    throw tl::Exception (tl::to_string (tr ("Error calling method")) + " '" + m_cls_decl->name () + "::" + meth->names () + "': " + ex.msg ());
  } catch (...) {
    throw;
  }
}

void
Proxy::destroy ()
{
  if (! m_cls_decl) {
    m_obj = 0;
    return;
  }

  if (! (m_owned || m_can_destroy) && m_obj) {
    throw tl::Exception (tl::to_string (tr ("Object cannot be destroyed explicitly")));
  }

  //  first create the object if it was not created yet and check if it has not been
  //  destroyed already (the former is to ensure that the object is created at least)
  if (! m_obj) {
    if (m_destroyed) {
      throw tl::Exception (tl::to_string (tr ("Object has been destroyed already")));
    } else {
      m_obj = m_cls_decl->create ();
      m_owned = true;
    }
  }

  void *o = 0;
  if (m_owned || m_can_destroy) {
    o = m_obj;
  }
  detach ();
  if (o) {
    m_cls_decl->destroy (o);
  }
}

void
Proxy::detach ()
{
  if (! m_destroyed && m_cls_decl && m_cls_decl->is_managed ()) {
    gsi::ObjectBase *gsi_object = m_cls_decl->gsi_object (m_obj, false);
    if (gsi_object) {
      gsi_object->status_changed_event ().remove (this, &Proxy::object_status_changed);
    }
    if (!m_owned && m_self != Qnil) {
      gc_unlock_object (m_self);
    }
  }

  //  NOTE: m_owned = false might mean the C++ object is already destroyed. We must not
  //  modify in this case and without is_managed() there is no way of knowing the state.
  if (m_owned) {
    clear_callbacks ();
  }

  m_self = Qnil;
  m_obj = 0;
  m_destroyed = true;
  m_const_ref = false;
  m_owned = false;
  m_can_destroy = false;
}

Proxy::callbacks_cache Proxy::s_callbacks_cache;

void
Proxy::initialize_callbacks ()
{
#if 0

  callbacks_cache::iterator cb = s_callbacks_cache.find (CLASS_OF (m_self));
  if (cb == s_callbacks_cache.end ()) {

    cb = s_callbacks_cache.insert (std::make_pair (CLASS_OF (m_self), callback_methods_type ())).first;

    const gsi::ClassBase *cls = m_cls_decl;

    // we got a new object - hence we have to attach event handlers.
    // We don't need to install virtual function callbacks because in that case, no overload is possible
    // (the object has been created on C++ side).
    while (cls) {

      for (gsi::ClassBase::method_iterator m = cls->begin_methods (); m != cls->end_methods (); ++m) {

        if ((*m)->is_callback ()) {

          //  HINT: all callback may not have aliases nor overloads
          const char *nstr = (*m)->primary_name ().c_str ();

          //  There is no place in the ruby API to determine whether a method is defined.
          //  Instead we explicitly call "method_defined?" to check, whether the given method
          //  is defined.
          VALUE name = rb_str_new (nstr, strlen (nstr));

          for (int prot = 0; prot < 2; ++prot) {

            VALUE rb_ret;
            if (prot) {
              rb_ret = rb_funcall2_checked (rb_class_of (self), rb_intern ("protected_method_defined?"), 1, &name);
            } else {
              rb_ret = rb_funcall2_checked (rb_class_of (self), rb_intern ("method_defined?"), 1, &name);
            }
            if (RTEST (rb_ret)) {
              //  Only if the class defines that method we can link the virtual method call to the
              //  Ruby method
              cb->second.push_back (*m);
              break;
            }

          }

        }

      }

      //  consider base classes as well.
      cls = cls->base ();

    }

  }

  for (callback_methods_type::const_iterator m = cb->second.begin (); m != cb->second.end (); ++m) {

    bool prot = false;
    const char *nstr = extract_protected ((*m)->name ().c_str (), prot);

    ID method_id = rb_intern (nstr);

    int id = add_callback (Proxy::CallbackFunction (method_id, m_self, *m));
    (*m)->set_callback (m_obj, gsi::Callback (id, this, (*m)->argsize (), (*m)->retsize ()));

  }

#else

  const gsi::ClassBase *cls = m_cls_decl;

  // we got a new object - hence we have to attach callback handlers.
  // We don't need to install virtual function callbacks because in that case, no overload is possible
  // (the object has been created on C++ side).
  while (cls) {

    for (gsi::ClassBase::method_iterator m = cls->begin_callbacks (); m != cls->end_callbacks (); ++m) {

      //  HINT: all callback may not have aliases nor overloads
      const char *nstr = (*m)->primary_name ().c_str ();

      //  There is no place in the ruby API to determine whether a method is defined.
      //  Instead we explicitly call "method_defined?" to check, whether the given method
      //  is defined.
      VALUE name = rb_str_new (nstr, long (strlen (nstr)));
      RB_GC_GUARD (name);

      for (int prot = 0; prot < 2; ++prot) {

        VALUE rb_ret;
        if (prot) {
          rb_ret = rba_funcall2_checked (rb_class_of (m_self), rb_intern ("protected_method_defined?"), 1, &name);
        } else {
          rb_ret = rba_funcall2_checked (rb_class_of (m_self), rb_intern ("method_defined?"), 1, &name);
        }
        if (RTEST (rb_ret)) {

          //  Only if the class defines that method we can link the virtual method call to the
          //  Ruby method
          int id = add_callback (Proxy::CallbackFunction (rb_intern (nstr), *m));
          (*m)->set_callback (m_obj, gsi::Callback (id, this, (*m)->argsize (), (*m)->retsize ()));

          break;

        }

      }

    }

    //  consider base classes as well.
    cls = cls->base ();

  }

#endif
}

void
Proxy::clear_callbacks ()
{
  m_cbfuncs.clear ();

  if (! m_obj) {
    return;
  }

  const gsi::ClassBase *cls = m_cls_decl;
  while (cls) {

    //  reset all callbacks
    for (gsi::ClassBase::method_iterator m = cls->begin_callbacks (); m != cls->end_callbacks (); ++m) {
      (*m)->set_callback (m_obj, gsi::Callback ());
    }

    //  consider base classes as well.
    cls = cls->base ();

  }
}

void
Proxy::release ()
{
  //  If the object is managed we first reset the ownership of all other clients
  //  and then make us the owner
  const gsi::ClassBase *cls = cls_decl ();
  if (cls) {

    if (cls->is_managed ()) {
      void *o = obj ();
      if (o) {
        cls->gsi_object (o)->keep ();
      }
    }

    if (!m_owned) {
      if (cls->is_managed () && m_self != Qnil) {
        gc_unlock_object (m_self);
      }
      m_owned = true;
    }

  }
}

void
Proxy::keep ()
{
  const gsi::ClassBase *cls = cls_decl ();
  if (cls) {
    void *o = obj ();
    if (o) {
      if (cls->is_managed ()) {
        cls->gsi_object (o)->keep ();
      } else {
        keep_internal ();
      }
    }
  }
}

void
Proxy::keep_internal ()
{
  if (m_owned) {
    //  Fallback: the object is not gsi-enabled: we use the ownership flag in this
    //  case to keep it alive. This will not reset the ownership flag for all clients.
    m_owned = false;
    tl_assert (m_self != Qnil);
    if (m_cls_decl->is_managed ()) {
      gc_lock_object (m_self);
    }
  }
}

void
Proxy::set (void *obj, bool owned, bool const_ref, bool can_destroy, VALUE self)
{
  const gsi::ClassBase *cls = cls_decl ();
  tl_assert (cls);

  if (obj != m_obj) {

    //  cleanup
    if (m_obj) {

      bool prev_owned = m_owned;
      void *prev_obj = m_obj;

      detach ();

      //  Destroy the object if we are owner. We don't destroy the object if it was locked
      //  (either because we are not owner or from C++ side using keep())
      if (prev_owned) {
        cls->destroy (prev_obj);
      }

    }

    m_obj = obj;
    m_self = self;
    m_owned = owned;
    m_can_destroy = can_destroy;
    m_const_ref = const_ref;

    if (m_obj) {

      if (cls->is_managed ()) {

        gsi::ObjectBase *gsi_object = cls->gsi_object (m_obj);
        //  Consider the case of "keep inside constructor"
        if (m_owned && gsi_object->already_kept ()) {
          m_owned = false;
        }
        gsi_object->status_changed_event ().add (this, &Proxy::object_status_changed);

        if (! m_owned) {
          gc_lock_object (m_self);
        }

      }

      initialize_callbacks ();

    }

  } else {

    //  m_owned = owned;  - thou shalt not change the ownership status
    m_can_destroy = can_destroy;
    m_const_ref = const_ref;

  }

  //  now we have a valid object (or nil) - we can reset "destroyed" state. Note: this has to be done
  //  here because before detach might be called on *this which resets m_destroyed.
  m_destroyed = false;
}

void *
Proxy::obj ()
{
  if (! m_obj) {
    if (m_destroyed) {
      throw tl::Exception (tl::to_string (tr ("Object has been destroyed already")));
    } else if (cls_decl ()->can_default_create()) {
      //  delayed creation of a detached C++ object ..
      set (cls_decl ()->create (), true, false, true, m_self);
    } else {
      throw tl::Exception (tl::to_string (tr ("Object cannot be default-created (missing arguments to 'new'?)")));
    }
  }

  return m_obj;
}

void
Proxy::object_status_changed (gsi::ObjectBase::StatusEventType type)
{
  if (type == gsi::ObjectBase::ObjectDestroyed) {
    m_destroyed = true;  //  NOTE: must be set before detach and indicates that the object was destroyed externally.
    detach ();
  } else if (type == gsi::ObjectBase::ObjectKeep) {
    keep_internal ();
  } else if (type == gsi::ObjectBase::ObjectRelease) {
    release ();
  }
}

// --------------------------------------------------------------------------
//  SignalHandler implementation

VALUE SignalHandler::klass = Qnil;

SignalHandler::SignalHandler ()
  : m_obj (Qnil)
{
}

SignalHandler::~SignalHandler ()
{
  clear_procs ();
}

void
SignalHandler::initialize (VALUE obj)
{
  m_obj = obj;
}

void
SignalHandler::assign (VALUE proc)
{
  clear_procs ();
  add (proc);
}

void
SignalHandler::clear ()
{
  clear_procs ();
}

void
SignalHandler::add (VALUE proc)
{
  remove (proc);
  m_procs.push_back (proc);
}

void
SignalHandler::remove (VALUE proc)
{
  for (std::list<VALUE>::iterator p = m_procs.begin (); p != m_procs.end (); ++p) {
    if (*p == proc) {
      m_procs.erase (p);
      break;
    }
  }
}

void
SignalHandler::mark_this ()
{
  if (m_obj != Qnil) {
    rb_gc_mark (m_obj);
  }
  for (std::list<VALUE>::iterator p = m_procs.begin (); p != m_procs.end (); ++p) {
    rb_gc_mark (*p);
  }
}

void
SignalHandler::clear_procs ()
{
  m_procs.clear ();
}

void
SignalHandler::free (void *p)
{
  delete ((SignalHandler *) p);
}

void
SignalHandler::mark (void *p)
{
  ((SignalHandler *) p)->mark_this ();
}

VALUE
SignalHandler::alloc (VALUE klass)
{
  tl_assert (TYPE (klass) == T_CLASS);

  SignalHandler *signal = new SignalHandler ();
  return Data_Wrap_Struct (klass, &SignalHandler::mark, &SignalHandler::free, signal);
}

VALUE
SignalHandler::static_initialize (VALUE self, VALUE obj)
{
  SignalHandler *p = 0;
  Data_Get_Struct (self, SignalHandler, p);
  if (p) {
    p->initialize (obj);
  }
  return Qnil;
}

VALUE
SignalHandler::static_assign (VALUE self, VALUE proc)
{
  //  NOTE: self-assignment happens on "signal+=proc" which is resolved to
  //  "signal=(signal+proc)" and "signal+proc" returns the signal itself.
  //    and:
  if (proc != self) {

    if (TYPE (proc) != T_DATA || rb_obj_is_kind_of (proc, rb_cProc) != Qtrue) {
      std::string msg = tl::to_string (tr ("Single argument to signal must be a Proc object"));
      VALUE args [1];
      args [0] = rb_str_new2 (msg.c_str ());
      rb_exc_raise (rb_class_new_instance(1, args, rb_eRuntimeError));
    }

    SignalHandler *p = 0;
    Data_Get_Struct (self, SignalHandler, p);
    if (p) {
      p->assign (proc);
    }

  }

  return Qnil;
}

VALUE
SignalHandler::static_add (VALUE self, VALUE proc)
{
  if (TYPE (proc) != T_DATA || rb_obj_is_kind_of (proc, rb_cProc) != Qtrue) {
    std::string msg = tl::to_string (tr ("Single argument to signal's add method must be a Proc object"));
    VALUE args [1];
    args [0] = rb_str_new2 (msg.c_str ());
    rb_exc_raise (rb_class_new_instance(1, args, rb_eRuntimeError));
  }

  SignalHandler *p = 0;
  Data_Get_Struct (self, SignalHandler, p);
  if (p) {
    p->add (proc);
  }
  return self;
}

VALUE
SignalHandler::static_clear (VALUE self)
{
  SignalHandler *p = 0;
  Data_Get_Struct (self, SignalHandler, p);
  if (p) {
    p->clear ();
  }
  return self;
}

VALUE
SignalHandler::static_remove (VALUE self, VALUE proc)
{
  SignalHandler *p = 0;
  Data_Get_Struct (self, SignalHandler, p);
  if (p) {
    p->remove (proc);
  }
  return self;
}

void
SignalHandler::define_class (VALUE module, const char *name)
{
  klass = rb_define_class_under (module, name, rb_cObject);
  rb_define_alloc_func (klass, SignalHandler::alloc);
  rb_define_method (klass, "initialize", (ruby_func) &SignalHandler::static_initialize, 1);
  rb_define_method (klass, "set", (ruby_func) &SignalHandler::static_assign, 1);
  rb_define_method (klass, "clear", (ruby_func) &SignalHandler::static_clear, 0);
  rb_define_method (klass, "+", (ruby_func) &SignalHandler::static_add, 1);
  rb_define_method (klass, "add", (ruby_func) &SignalHandler::static_add, 1);
  rb_define_method (klass, "connect", (ruby_func) &SignalHandler::static_add, 1);
  rb_define_method (klass, "-", (ruby_func) &SignalHandler::static_remove, 1);
  rb_define_method (klass, "remove", (ruby_func) &SignalHandler::static_remove, 1);
  rb_define_method (klass, "disconnect", (ruby_func) &SignalHandler::static_remove, 1);
}

void SignalHandler::call (const gsi::MethodBase *meth, gsi::SerialArgs &args, gsi::SerialArgs &ret) const
{
  VALUE argv = rb_ary_new2 (long (std::distance (meth->begin_arguments (), meth->end_arguments ())));
  RB_GC_GUARD (argv);

  tl::Heap heap;

  //  TODO: signals with default arguments?
  for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); args && a != meth->end_arguments (); ++a) {
    rb_ary_push (argv, pull_arg (*a, 0, args, heap));
  }

  //  call the signal handlers ... the last one will deliver the return value
  VALUE rb_ret = Qnil;
  for (std::list<VALUE>::const_iterator p = m_procs.begin (); p != m_procs.end (); ++p) {
    rb_ret = rba_funcall2_checked (*p, rb_intern ("call"), RARRAY_LEN (argv), RARRAY_PTR (argv));
  }

  push_arg (meth->ret_type (), ret, rb_ret, heap);

  //  a Ruby callback must not leave temporary objects
  tl_assert (heap.empty ());
}

// --------------------------------------------------------------------------
//  Class map management

static std::map <VALUE, const gsi::ClassBase *> cls_map;
static std::map <std::pair<const gsi::ClassBase *, bool>, VALUE> rev_cls_map;

void register_class (VALUE ruby_cls, const gsi::ClassBase *gsi_cls, bool as_static)
{
  cls_map.insert (std::make_pair (ruby_cls, gsi_cls));
  rev_cls_map.insert (std::make_pair (std::make_pair (gsi_cls, as_static), ruby_cls));
}

VALUE ruby_cls (const gsi::ClassBase *cls, bool as_static)
{
  auto c = rev_cls_map.find (std::make_pair (cls, as_static));
  tl_assert (c != rev_cls_map.end ());
  return c->second;
}

bool is_registered (const gsi::ClassBase *cls, bool as_static)
{
  return rev_cls_map.find (std::make_pair (cls, as_static)) != rev_cls_map.end ();
}

const gsi::ClassBase *find_cclass (VALUE k)
{
  const gsi::ClassBase *cls = find_cclass_maybe_null (k);
  tl_assert (cls != 0);
  return cls;
}

const gsi::ClassBase *find_cclass_maybe_null (VALUE k)
{
  std::map <VALUE, const gsi::ClassBase *>::const_iterator cls;

  //  find the class that is bound to C++ (maybe a super class)
  while (k != rb_cObject) {
    cls = cls_map.find (k);
    if (cls == cls_map.end ()) {
      //  if not found advance to super class
      k = RCLASS_SUPER (k);
    } else {
      break;
    }
  }

  return cls != cls_map.end () ? cls->second : 0;
}

}

#endif


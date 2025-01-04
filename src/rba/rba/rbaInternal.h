
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

#ifndef _HDR_rbaInternal
#define _HDR_rbaInternal

#ifdef HAVE_RUBY

#include "gsiDecl.h"
#include "gsiCallback.h"
#include "gsiMethods.h"
#include "gsiSignals.h"
#include "tlObject.h"
#include "tlScriptError.h"

#include <ruby.h>

namespace rba
{

/**
 *  @brief A class encapsulating a ruby exception
 */
class RubyError
  : public tl::ScriptError
{
public:
  RubyError (VALUE exc, const char *msg, const char *cls, const std::vector <tl::BacktraceElement> &backtrace)
    : tl::ScriptError (msg, cls, backtrace), m_exc (exc)
  { }

  RubyError (VALUE exc, const char *msg, const char *sourcefile, int line, const char *cls, const std::vector <tl::BacktraceElement> &backtrace)
    : tl::ScriptError (msg, sourcefile, line, cls, backtrace), m_exc (exc)
  { }

  RubyError (const RubyError &d)
    : tl::ScriptError (d), m_exc (d.m_exc)
  { }

  VALUE exc () const
  {
    return m_exc;
  }

private:
  VALUE m_exc;
};

/**
 *  @brief A class responsible for forwarding exceptions raised from "break", "return" and other flow control functions
 */
class RubyContinueException
  : public tl::CancelException
{
public:
  RubyContinueException (int state)
    : tl::CancelException (), m_state (state)
  { }

  int state () const
  {
    return m_state;
  }

private:
  int m_state;
};

/**
 *  @brief The proxy object that represents the C++ object on the Ruby side
 */
class Proxy
  : public gsi::Callee
{
public:
  struct CallbackFunction
  {
    CallbackFunction (ID id, const gsi::MethodBase *m)
      : method_id (id), method (m)
    {
      //  .. nothing yet ..
    }

    ID method_id;
    const gsi::MethodBase *method;
  };

  Proxy (const gsi::ClassBase *_cls_decl);
  ~Proxy ();

  void keep ();
  void release ();
  void destroy ();
  void set (void *obj, bool owned, bool const_ref, bool can_destroy, VALUE self);
  void detach ();
  void mark ();

  void reset ()
  {
    set (0, false, false, false, Qnil);
  }

  const gsi::ClassBase *cls_decl () const
  {
    return m_cls_decl;
  }

  VALUE self () const
  {
    return m_self;
  }

  void set_self (VALUE self)
  {
    m_self = self;
  }

  bool destroyed () const
  {
    return m_destroyed;
  }

  bool const_ref () const
  {
    return m_const_ref;
  }

  void set_const_ref (bool c)
  {
    m_const_ref = c;
  }

  void *obj ();

  bool owned () const
  {
    return m_owned;
  }

  int add_callback (const CallbackFunction &vf)
  {
    m_cbfuncs.push_back (vf);
    return int (m_cbfuncs.size () - 1);
  }

  void clear_callbacks ();

  VALUE signal_handler (const gsi::MethodBase *meth);

  virtual void call (int id, gsi::SerialArgs &args, gsi::SerialArgs &ret) const;
  virtual bool can_call () const;

private:
  const gsi::ClassBase *m_cls_decl;
  void *m_obj;
  bool m_owned : 1;
  bool m_const_ref : 1;
  bool m_destroyed : 1;
  bool m_can_destroy : 1;
  VALUE m_self;

  std::vector<CallbackFunction> m_cbfuncs;
  std::map<const gsi::MethodBase *, VALUE> m_signal_handlers;

  typedef std::vector<const gsi::MethodBase *> callback_methods_type;
  typedef std::map<VALUE, callback_methods_type> callbacks_cache;
  static callbacks_cache s_callbacks_cache;

  void initialize_callbacks ();

  void object_status_changed (gsi::ObjectBase::StatusEventType type);
  void keep_internal ();
};

/**
 *  @brief The SignalHandler implementation
 */
class SignalHandler
  : public gsi::SignalHandler
{
public:
  static VALUE klass;

  static void define_class (VALUE module, const char *name);

  SignalHandler ();
  ~SignalHandler ();

  void initialize (VALUE obj);
  void assign (VALUE proc);
  void clear ();
  void add (VALUE proc);
  void remove (VALUE proc);
  virtual void call (const gsi::MethodBase *meth, gsi::SerialArgs &args, gsi::SerialArgs &ret) const;
  void mark_this ();

private:
  VALUE m_obj;
  std::list<VALUE> m_procs;

  void clear_procs ();
  static void free (void *p);
  static void mark (void *p);
  static VALUE alloc (VALUE klass);
  static VALUE static_initialize (VALUE self, VALUE obj);
  static VALUE static_assign (VALUE self, VALUE proc);
  static VALUE static_add (VALUE self, VALUE proc);
  static VALUE static_clear (VALUE self);
  static VALUE static_remove (VALUE self, VALUE proc);
};

/**
 *  @brief Registers a Ruby class for a gsi class
 */
void register_class (VALUE ruby_cls, const gsi::ClassBase *gsi_cls, bool as_static);

/**
 *  @brief Find the class declaration from the Ruby object
 */
const gsi::ClassBase *find_cclass (VALUE k);

/**
 *  @brief Find the class declaration from the Ruby object
 */
const gsi::ClassBase *find_cclass_maybe_null (VALUE k);

/**
 *  @brief Finds the Ruby class for a gsi class
 */
VALUE ruby_cls (const gsi::ClassBase *cls, bool as_static);

/**
 *  @brief Gets a value indicating whether a Ruby class is registered for a GSI class
 */
bool is_registered (const gsi::ClassBase *gsi_cls, bool as_static);

/**
 *  @brief Locks the Ruby object against destruction by the GC
 *
 *  After calling this function, the object given by value is no longer
 *  managed by the GC. This is equivalent to rb_gc_register_address, but
 *  faster.
 */
void gc_lock_object (VALUE value);

/**
 *  @brief Unlocks the Ruby object against destruction by the GC
 *
 *  After calling this function, the object given by value is no longer
 *  managed by the GC. This is equivalent to rb_gc_unregister_address, but
 *  faster.
 */
void gc_unlock_object (VALUE value);

/**
 *  @brief A Locker for the object based on the RIIA pattern
 */
class GCLocker
{
public:
  GCLocker (VALUE value)
    : m_value (value)
  {
    gc_lock_object (m_value);
  }

  ~GCLocker ()
  {
    gc_unlock_object (m_value);
  }

private:
  GCLocker ();
  GCLocker (const GCLocker &other);
  GCLocker &operator= (const GCLocker &other);

  VALUE m_value;
};

/**
 *  @brief Makes the locked object vault required for gc_lock_object and gc_unlock_object
 *
 *  This function needs to be called by the interpreter initially.
 */
void make_locked_object_vault (VALUE module);

}

#endif

#endif


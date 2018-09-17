
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#ifndef _HDR_pyaObject
#define _HDR_pyaObject

#include "Python.h"

#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "gsiSignals.h"
#include "tlObject.h"

#include "pyaRefs.h"

namespace pya
{

class PYAObjectBase;

/**
 *  @brief A storage object for a function to callback
 */
struct CallbackFunction
{
  CallbackFunction (PythonRef pym, const gsi::MethodBase *m);

  PythonRef callable () const;
  const gsi::MethodBase *method () const;
  bool operator== (const CallbackFunction &other) const;

private:
  PythonRef m_callable;
  PythonRef m_weak_self;
  PythonRef m_class;
  const gsi::MethodBase *mp_method;

  PyObject *self_ref () const;
  PyObject *callable_ref () const;
  bool is_instance_method () const;
};

/**
 *  @brief An adaptor class for the callback mechanism
 */
class Callee
  : public gsi::Callee
{
public:
  /**
   *  @brief Constructor for a Callee object pointing the to given Python object
   */
  Callee (PYAObjectBase *obj);
  
  /**
   *  @brief Destructor
   */
  ~Callee ();

  /**
   *  @brief Adds a callback (given by the CallbackFunction)
   *  This method returns a callback ID which can be used to register the callback
   *  at an GSI object.
   */
  int add_callback (const CallbackFunction &vf);

  /**
   *  @brief Clears all callbacks registered 
   */
  void clear_callbacks ();

  /**
   *  @brief Implementation of the Callee interface
   */
  virtual void call (int id, gsi::SerialArgs &args, gsi::SerialArgs &ret) const;

  /**
   *  @brief Implementation of the Callee interface
   */
  virtual bool can_call () const;

private:
  PYAObjectBase *mp_obj;
  std::vector<CallbackFunction> m_cbfuncs; 
};

/**
 *  @brief The signal handler abstraction
 *
 *  This class implements the signal handler that interfaces to GSI's signal system
 */
class SignalHandler
  : public gsi::SignalHandler
{
public:
  /**
   *  @brief Constructor
   */
  SignalHandler ();

  /**
   *  @brief Destructor
   */
  ~SignalHandler ();

  /**
   *  @brief Implementation of the callback interface
   */
  virtual void call (const gsi::MethodBase *method, gsi::SerialArgs &args, gsi::SerialArgs &ret) const;

  /**
   *  @brief Adds a callable to the list of targets
   */
  void add (PyObject *callable);

  /**
   *  @brief Removes a callable from the list of targets
   */
  void remove (PyObject *callable);

  /**
   *  @brief Clears the list of callables
   */
  void clear ();

  /**
   *  @brief Assign another handler to this
   */
  void assign (const SignalHandler *other);

private:
  std::vector<CallbackFunction> m_cbfuncs;
};

/**
 *  @brief A helper object to forward status changed events to a Python object
 *  This object is used to connect the events to the Python object. Unfortunately,
 *  PYAObjectBase cannot be derived from tl::Object directly since in that case,
 *  tl::Object will be placed before PyObject in the memory layout.
 */
class StatusChangedListener
  : public tl::Object
{
public:
  StatusChangedListener (PYAObjectBase *pya_object);

  void object_status_changed (gsi::ObjectBase::StatusEventType type);

  PYAObjectBase *pya_object () const
  {
    return mp_pya_object;
  }

private:
  PYAObjectBase *mp_pya_object;
};

/**
 *  @brief The Python object representing a GSI object
 *
 *  Note: the PYAObjectBase must be directly derived from PyObject so that
 *  a PyObject pointer can be cast to a PYAObjectBase pointer.
 */
class PYAObjectBase
  : public PyObject
{
public:
  /**
   *  @brief Constructor - creates a new object for the given GSI class
   */
  PYAObjectBase (const gsi::ClassBase *_cls_decl);

  /**
   *  @brief Destructor
   */
  ~PYAObjectBase ();

  /**
   *  @brief Indicates that a C++ object is present
   */
  bool is_attached () const
  {
    return m_obj != 0;
  }

  /**
   *  @brief Explicitly destroy the C++ object
   *  If the C++ object is owned by the Python object, this method will delete the C++
   *  object and the \destroyed attribute will become true.
   *  The reference is no longer valid.
   */
  void destroy ();

  /**
   *  @brief Links the Python object with a C++ object
   *  The "owned" attribute indicates that the reference will be owned by the Python object.
   *  That means that the C++ object is being destroyed when the Python object expires. 
   *  If "const_ref" is true, the Python object is said to be a const object which means
   *  only const methods can be called on it. That is a somewhat weak emulation for the 
   *  constness concept in C++ since there is only one Python object representing multiple
   *  references. If one of these references goes to non-const, the Python object will accept
   *  non-const method calls.
   *  "can_destroy" indicates that the C++ object can be destroyed (has a destructor).
   */
  void set (void *obj, bool owned, bool const_ref, bool can_destroy);

  /**
   *  @brief Unlinks the C++ object from the Python object
   *  This method can be called to make the Python object cut the link to the C++ object.
   *  After that operation, the \destroyed attribute will be come true, even though the 
   *  C++ object may not actually be destroyed.
   *  The reference will become invalid.
   */
  void detach ();

  /**
   *  @brief Gets the GSI class object
   */
  const gsi::ClassBase *cls_decl () const 
  {
    return m_cls_decl;
  }

  /**
   *  @brief Gets a flag indicating that the corresponding C++ object expired
   *  If the Python object acts as a weak reference to a foreign object (owned = false),
   *  the foreign object may expire before the Python object is deleted.
   *  In that case, destroyed becomes true.
   */
  bool destroyed () const 
  {
    return m_destroyed;
  }

  /**
   *  @brief Returns a flag indicating that this Python object is a const reference to a C++ object
   *  See \set for a description of that flag
   */
  bool const_ref () const 
  {
    return m_const_ref;
  }

  /**
   *  @brief Sets a flag indicating that this Python object is a const reference to the C++ object
   *  See \set for a description of that flag.
   */
  void set_const_ref (bool c) 
  {
    m_const_ref = c;
  }

  /**
   *  @brief Returns the C++ object reference
   */
  void *obj ();

  /**
   *  @brief Puts this object under C++ management (release from script management)
   */
  void keep ();

  /**
   *  @brief Puts this object under script management again
   */
  void release ();

  /**
   *  @brief Returns true, if the C++ object is owned by the Python object
   *  See \set for details about this flag
   */
  bool owned () const 
  {
    return m_owned;
  }

  /** 
   *  @brief The callee interface
   */
  Callee &callee () 
  {
    return m_callee;
  }

  /** 
   *  @brief The callee interface (const pointer)
   */
  const Callee &callee () const 
  {
    return m_callee;
  }

  /**
   *  @brief Returns the signal handler for the signal given by "meth"
   *  If a signal handler was already present, the existing object is returned.
   */
  pya::SignalHandler *signal_handler (const gsi::MethodBase *meth);

  /**
   *  @brief Clears the callbacks cache
   */
  static void clear_callbacks_cache ();

private:
  friend class StatusChangedListener;

  typedef std::vector<const gsi::MethodBase *> callback_methods_type;
  typedef std::map<PythonRef, callback_methods_type> callbacks_cache;
  static callbacks_cache s_callbacks_cache;

  void detach_callbacks ();
  void initialize_callbacks ();

  StatusChangedListener m_listener;
  Callee m_callee;
  const gsi::ClassBase *m_cls_decl;
  void *m_obj;
  bool m_owned : 1;
  bool m_const_ref : 1;
  bool m_destroyed : 1;
  bool m_can_destroy : 1;
  std::map <const gsi::MethodBase *, pya::SignalHandler> m_signal_table;

  void object_status_changed (gsi::ObjectBase::StatusEventType type);
  void keep_internal ();
};

}

#endif


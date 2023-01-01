
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


#include "pyaObject.h"
#include "pyaMarshal.h"
#include "pyaUtils.h"
#include "pyaConvert.h"
#include "pyaSignalHandler.h"
#include "pyaStatusChangedListener.h"
#include "pya.h"

#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "gsiSignals.h"
#include "tlObject.h"

#include "tlLog.h"

namespace pya
{

// --------------------------------------------------------------------------
//  Private classes

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

// --------------------------------------------------------------------------
//  Implementation of Callee

Callee::Callee (PYAObjectBase *obj)
  : mp_obj (obj)
{
  //  .. nothing yet ..
}

Callee::~Callee ()
{
  //  .. nothing yet ..
}

int 
Callee::add_callback (const CallbackFunction &vf)
{
  m_cbfuncs.push_back (vf);
  return int (m_cbfuncs.size () - 1);
}

void 
Callee::clear_callbacks ()
{
  m_cbfuncs.clear ();
}

bool
Callee::can_call () const
{
  return pya::PythonInterpreter::instance () != 0;
}

void 
Callee::call (int id, gsi::SerialArgs &args, gsi::SerialArgs &ret) const
{
  const gsi::MethodBase *meth = m_cbfuncs [id].method ();

  try {

    PythonRef callable (m_cbfuncs [id].callable ());

    tl::Heap heap;

    if (callable) {

      PYTHON_BEGIN_EXEC

        size_t arg4self = 1;

        //  One argument for "self"
        PythonRef argv (PyTuple_New (arg4self + std::distance (meth->begin_arguments (), meth->end_arguments ())));

        //  Put self into first argument
        PyTuple_SetItem (argv.get (), 0, mp_obj->py_object ());
        Py_INCREF (mp_obj->py_object ());

        //  TODO: callbacks with default arguments?
        for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); args && a != meth->end_arguments (); ++a) {
          PyTuple_SetItem (argv.get (), arg4self + (a - meth->begin_arguments ()), pop_arg (*a, args, 0, heap).release ());
        }

        PythonRef result (PyObject_CallObject (callable.get (), argv.get ()));
        if (! result) {
          check_error ();
        }

        tl::Heap heap;
        push_arg (meth->ret_type (), ret, meth->ret_type().pass_obj() ? result.release() : result.get (), heap);

        
        //  a Python callback must not leave temporary objects
        tl_assert (heap.empty ());

      PYTHON_END_EXEC

    }

  } catch (PythonError &err) {
    PythonError err_with_context (err);
    err_with_context.set_context (mp_obj->cls_decl ()->name () + "." + meth->names ());
    throw err_with_context;
  } catch (tl::ExitException &) {
    throw;
  } catch (tl::Exception &ex) {
    throw tl::Exception (tl::to_string (tr ("Error calling method")) + " '" + mp_obj->cls_decl ()->name () + "." + meth->names () + "': " + ex.msg ());
  } catch (...) {
    throw;
  }
}

// --------------------------------------------------------------------------
//  Implementation of PYAObjectBase

PYAObjectBase::PYAObjectBase(const gsi::ClassBase *_cls_decl, PyObject *py_object)
  : mp_py_object (py_object),
    mp_listener (new pya::StatusChangedListener (this)),
    mp_callee (new pya::Callee (this)),
    m_cls_decl (_cls_decl),
    m_obj (0),
    m_owned (false),
    m_const_ref (false),
    m_destroyed (false),
    m_can_destroy (false)
{
  //  .. nothing yet ..
}

PYAObjectBase::~PYAObjectBase ()
{
  try {

    bool prev_owned = m_owned;
    void *prev_obj = m_obj;

    detach ();

    //  Destroy the object if we are owner. We don't destroy the object if it was locked
    //  (either because we are not owner or from C++ side using keep())
    if (m_cls_decl && prev_obj && prev_owned) {
      m_cls_decl->destroy (prev_obj);
    }

  } catch (std::exception &ex) {
    tl::warn << "Caught exception in object destructor: " << ex.what ();
  } catch (tl::Exception &ex) {
    tl::warn << "Caught exception in object destructor: " << ex.msg ();
  } catch (...) {
    tl::warn << "Caught unspecified exception in object destructor";
  }

  delete mp_listener;
  mp_listener = 0;
  delete mp_callee;
  mp_callee = 0;
  m_destroyed = true;
}

void
PYAObjectBase::object_destroyed ()
{
  //  This may happen outside the Python interpreter, so we safeguard ourselves against this.
  //  In this case, we may encounter a memory leak, but there is little we can do
  //  against this and it will happen in the application teardown anyway.
  if (PythonInterpreter::instance ()) {

    bool prev_owner = m_owned;

    m_destroyed = true;  // NOTE: must be set before detach!

    detach ();

    //  NOTE: this may delete "this"!
    if (!prev_owner) {
      Py_DECREF (py_object ());
    }

  }
}

void 
PYAObjectBase::release ()
{
  //  If the object is managed we first reset the ownership of all other clients
  //  and then make us the owner
  const gsi::ClassBase *cls = cls_decl ();
  if (cls && cls->is_managed ()) {
    void *o = obj ();
    if (o) {
      cls->gsi_object (o)->keep ();
    }
  }

  //  NOTE: this is fairly dangerous
  if (!m_owned) {
    m_owned = true;
    //  NOTE: this may delete "this"! TODO: this should not happen. Can we assert that somehow?
    Py_DECREF (py_object ());
  }
}

void
PYAObjectBase::keep_internal ()
{
  if (m_owned) {
    Py_INCREF (py_object ());
    m_owned = false;
  }
}

void 
PYAObjectBase::keep ()
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
PYAObjectBase::detach ()
{
  if (m_obj) {

    const gsi::ClassBase *cls = cls_decl ();

    if (! m_destroyed && cls && cls->is_managed ()) {
      gsi::ObjectBase *gsi_object = cls->gsi_object (m_obj, false);
      if (gsi_object) {
        gsi_object->status_changed_event ().remove (mp_listener, &StatusChangedListener::object_status_changed);
      }
    }

    //  NOTE: m_owned = false might mean the C++ object is already destroyed. We must not
    //  modify in this case and without is_managed() there is no way of knowing the state.
    if (m_owned) {
      detach_callbacks ();
    }

    m_obj = 0;
    m_const_ref = false;
    m_owned = false;
    m_can_destroy = false;

  }
}

void 
PYAObjectBase::set (void *obj, bool owned, bool const_ref, bool can_destroy) 
{
  const gsi::ClassBase *cls = cls_decl ();
  if (!cls) {
    return;
  }

  tl_assert (! m_obj);
  tl_assert (obj);

  m_obj = obj;
  m_owned = owned;
  m_can_destroy = can_destroy;
  m_const_ref = const_ref;

  //  initialize the callbacks according to the methods which need some
  initialize_callbacks ();

  if (cls->is_managed ()) {
    gsi::ObjectBase *gsi_object = cls->gsi_object (m_obj);
    //  Consider the case of "keep inside constructor"
    if (gsi_object->already_kept ()) {
      keep_internal ();
    }
    gsi_object->status_changed_event ().add (mp_listener, &StatusChangedListener::object_status_changed);
  }

  if (!m_owned) {
    Py_INCREF (py_object ());
  }
}

//  TODO: a static (singleton) instance is not thread-safe
PYAObjectBase::callbacks_cache PYAObjectBase::s_callbacks_cache;

pya::SignalHandler *
PYAObjectBase::signal_handler (const gsi::MethodBase *meth)
{
  std::map <const gsi::MethodBase *, pya::SignalHandler>::iterator st = m_signal_table.find (meth);
  if (st == m_signal_table.end ()) {
    st = m_signal_table.insert (std::make_pair (meth, pya::SignalHandler ())).first;
    meth->add_handler (obj (), &st->second);
  }
  return &st->second;
}

void
PYAObjectBase::initialize_callbacks ()
{
//  1 to enable caching, 0 to disable it.
//  TODO: caching appears to create some leaks ...
#if 1

  PythonRef type_ref ((PyObject *) Py_TYPE (py_object ()), false /*borrowed*/);

  //  Locate the callback-enabled methods set by Python type object (pointer)
  //  NOTE: I'm not quite sure whether the type object pointer is a good key
  //  for the cache. It may change since class objects may expire too if
  //  classes are put on the heap. Hence we have to keep a reference which is
  //  a pity, but hard to avoid.
  callbacks_cache::iterator cb = s_callbacks_cache.find (type_ref);
  if (cb == s_callbacks_cache.end ()) {

    cb = s_callbacks_cache.insert (std::make_pair (type_ref, callback_methods_type ())).first;
    
    const gsi::ClassBase *cls = cls_decl ();

    //  TODO: cache this .. this is taking too much time if done on every instance
    //  we got a new object - hence we have to attach event handlers.
    //  We don't need to install virtual function callbacks because in that case, no overload is possible
    //  (the object has been created on C++ side).
    while (cls) {

      for (gsi::ClassBase::method_iterator m = cls->begin_callbacks (); m != cls->end_callbacks (); ++m) {

        if (m_owned) {

          //  NOTE: only Python-implemented classes can reimplement methods. Since we
          //  take the attribute from the class object, only Python instances can overwrite 
          //  the methods and owned indicates that. owned == true indicates that.

          //  NOTE: a callback may not have aliases nor overloads
          const char *nstr = (*m)->primary_name ().c_str ();

          //  NOTE: we just take attributes from the class object. That implies that it's not
          //  possible to reimplement a method through instance attributes (rare case, I hope).
          //  In addition, if we'd use instance attributes we create circular references 
          //  (self/callback to method, method to self).
          //  TODO: That may happen too often, i.e. if the Python class does not reimplement the virtual
          //  method, but the C++ class defines a method hook that the reimplementation can call. 
          //  We don't want to produce a lot of overhead for the Qt classes here.
          PythonRef py_attr = PyObject_GetAttrString ((PyObject *) Py_TYPE (py_object ()), nstr);
          if (! py_attr) {

            //  because PyObject_GetAttrString left an error
            PyErr_Clear ();

          } else {

            //  Only if a Python-level class defines that method we can link the virtual method call to the 
            //  Python method. We should not create callbacks which we refer to C class implementations because that
            //  may create issues with callbacks during destruction (i.e. QWidget-destroyed signal)
            if (! PyCFunction_Check (py_attr.get ())) {
              cb->second.push_back (*m);
            }

          }

        }

      }

      //  consider base classes as well.
      cls = cls->base ();

    }

  }

  for (callback_methods_type::const_iterator m = cb->second.begin (); m != cb->second.end (); ++m) {

    PythonRef py_attr;
    const char *nstr = (*m)->primary_name ().c_str ();
    py_attr = PyObject_GetAttrString ((PyObject *) Py_TYPE (py_object ()), nstr);

    int id = mp_callee->add_callback (CallbackFunction (py_attr, *m));
    (*m)->set_callback (m_obj, gsi::Callback (id, mp_callee, (*m)->argsize (), (*m)->retsize ()));

  }

#else

  const gsi::ClassBase *cls = cls_decl ();

  //  TODO: cache this .. this is taking too much time if done on every instance
  //  we got a new object - hence we have to attach event handlers.
  //  We don't need to install virtual function callbacks because in that case, no overload is possible
  //  (the object has been created on C++ side).
  while (cls) {

    for (gsi::ClassBase::method_iterator m = cls->begin_methods (); m != cls->end_methods (); ++m) {

      if ((*m)->is_callback () && m_owned) {

        //  NOTE: only Python-implemented classes can reimplement methods. Since we
        //  take the attribute from the class object, only Python instances can overwrite 
        //  the methods and owned indicates that. owned == true indicates that.

        //  NOTE: a callback may not have aliases nor overloads
        const char *nstr = (*m)->primary_name ().c_str ();

        //  NOTE: we just take attributes from the class object. That implies that it's not
        //  possible to reimplement a method through instance attributes (rare case, I hope).
        //  In addition, if we'd use instance attributes we create circular references 
        //  (self/callback to method, method to self).
        //  TODO: That may happen too often, i.e. if the Python class does not reimplement the virtual
        //  method, but the C++ class defines a method hook that the reimplementation can call. 
        //  We don't want to produce a lot of overhead for the Qt classes here.
        PythonRef py_attr = PyObject_GetAttrString ((PyObject *) Py_TYPE (py_object ()), nstr);
        if (! py_attr) {

          //  because PyObject_GetAttrString left an error
          PyErr_Clear ();

        } else {

          //  Only if a Python-level class defines that method we can link the virtual method call to the 
          //  Python method. We should not create callbacks which we refer to C class implementations because that
          //  may create issues with callbacks during destruction (i.e. QWidget-destroyed signal)
          if (! PyCFunction_Check (py_attr.get ())) {

            PyObject *py_attr = PyObject_GetAttrString ((PyObject *) Py_TYPE (py_object ()), nstr);
            tl_assert (py_attr != NULL);
            int id = mp_callee->add_callback (CallbackFunction (py_attr, *m));
            (*m)->set_callback (m_obj, gsi::Callback (id, mp_callee, (*m)->argsize (), (*m)->retsize ()));

          }

        }

      }

    }

    //  consider base classes as well.
    cls = cls->base ();

  }

#endif
}

void 
PYAObjectBase::clear_callbacks_cache ()
{
  s_callbacks_cache.clear ();
}

void
PYAObjectBase::detach_callbacks ()
{
  PythonRef type_ref ((PyObject *) Py_TYPE (py_object ()), false /*borrowed*/);

  callbacks_cache::iterator cb = s_callbacks_cache.find (type_ref);
  if (cb != s_callbacks_cache.end ()) {
    for (callback_methods_type::const_iterator m = cb->second.begin (); m != cb->second.end (); ++m) {
      (*m)->set_callback (m_obj, gsi::Callback ());
    }
  }

  mp_callee->clear_callbacks ();
}

void 
PYAObjectBase::destroy ()
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

  m_destroyed = true;
}

void *
PYAObjectBase::obj () 
{
  if (! m_obj) {
    if (m_destroyed) {
      throw tl::Exception (tl::to_string (tr ("Object has been destroyed already")));
    } else {
      //  delayed creation of a detached C++ object ..
      set(cls_decl ()->create (), true, false, true);
    }
  }

  return m_obj;
}

PYAObjectBase *
PYAObjectBase::from_pyobject (PyObject *py_object)
{
  if (Py_TYPE (py_object)->tp_init == NULL) {
    throw tl::Exception (tl::to_string (tr ("Extension classes do not support instance methods or properties")));
  }

  PYAObjectBase *pya_object = from_pyobject_unsafe (py_object);
  tl_assert (pya_object->py_object () == py_object);
  return pya_object;
}

}


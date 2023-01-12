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

#include "pyaCallables.h"
#include "pyaObject.h"
#include "pyaModule.h"
#include "pyaInternal.h"
#include "pyaHelpers.h"
#include "pyaMarshal.h"
#include "pyaConvert.h"
#include "pyaUtils.h"
#include "gsiMethods.h"

namespace pya
{

// --------------------------------------------------------------------------
//  Methods for PYAObjectBase Python binding

/**
 *  @brief Destructor for the base class (the implementation object)
 */
void
pya_object_deallocate (PyObject *self)
{
  //  This avoids an assertion in debug builds (Python, gcmodule.c - update_refs).
  //  In short, the GC expects not to see objects with refcount 0 and asserts.
  //  However, due to triggering of signals or similar, the destructor call below
  //  may trigger a GC (https://github.com/KLayout/klayout/issues/1054).
  //  According to the comments this may be turned into a release mode assertion, so
  //  we better work around it.
  ++self->ob_refcnt;

  PYAObjectBase *p = PYAObjectBase::from_pyobject (self);
  p->~PYAObjectBase ();
  Py_TYPE (self)->tp_free (self);
}

/**
 *  @brief Constructor for the base class (the implementation object)
 */
int
pya_object_init (PyObject * /*self*/, PyObject *args, PyObject *kwds)
{
  //  no particular initialization
  static char *kwlist[] = {NULL};
  if (! PyArg_ParseTupleAndKeywords (args, kwds, "", kwlist)) {
    return -1;
  } else {
    return 0;
  }
}

/**
 *  @brief Factory for a base class object
 */
PyObject *
pya_object_new (PyTypeObject *type, PyObject * /*args*/, PyObject * /*kwds*/)
{
  //  create the object
  PyObject *self_pyobject = type->tp_alloc (type, 0);
  PYAObjectBase *self = PYAObjectBase::from_pyobject_unsafe (self_pyobject);
  new (self) PYAObjectBase (PythonModule::cls_for_type (type), self_pyobject);
  return self_pyobject;
}

// --------------------------------------------------------------------------
//  Method binding guts

/**
 *  @brief Gets the method name from a method id
 */
std::string
method_name_from_id (int mid, PyObject *self)
{
  const gsi::ClassBase *cls_decl = 0;

  if (! PyType_Check (self)) {
    PYAObjectBase *p = PYAObjectBase::from_pyobject (self);
    cls_decl = p->cls_decl ();
  } else {
    cls_decl = PythonModule::cls_for_type ((PyTypeObject *) self);
  }

  tl_assert (cls_decl != 0);

  const MethodTable *mt = MethodTable::method_table_by_class (cls_decl);
  tl_assert (mt);

  //  locate the method in the base classes method table if necessary
  while (mid < int (mt->bottom_mid ())) {

    tl_assert (cls_decl->base ());
    cls_decl = cls_decl->base ();
    mt = MethodTable::method_table_by_class (cls_decl);
    tl_assert (mt);

  }

  return cls_decl->name () + "." + mt->name (mid);
}

/**
 *  @brief Gets the method name from a method id
 */
std::string
property_name_from_id (int mid, PyObject *self)
{
  const gsi::ClassBase *cls_decl = 0;

  if (! PyType_Check (self)) {
    PYAObjectBase *p = PYAObjectBase::from_pyobject (self);
    cls_decl = p->cls_decl ();
  } else {
    cls_decl = PythonModule::cls_for_type ((PyTypeObject *) self);
  }

  tl_assert (cls_decl != 0);

  const MethodTable *mt = MethodTable::method_table_by_class (cls_decl);
  tl_assert (mt);

  //  locate the method in the base classes method table if necessary
  while (mid < int (mt->bottom_property_mid ())) {

    tl_assert (cls_decl->base ());
    cls_decl = cls_decl->base ();
    mt = MethodTable::method_table_by_class (cls_decl);
    tl_assert (mt);

  }

  return cls_decl->name () + "." + mt->property_name (mid);
}

static gsi::ArgType create_void_type ()
{
  gsi::ArgType at;
  at.init<void> ();
  return at;
}

static gsi::ArgType s_void_type = create_void_type ();

static PyObject *
get_return_value (PYAObjectBase *self, gsi::SerialArgs &retlist, const gsi::MethodBase *meth, tl::Heap &heap)
{
  PyObject *ret = NULL;

  if (meth->ret_type ().is_iter ()) {

    gsi::IterAdaptorAbstractBase *iter = (gsi::IterAdaptorAbstractBase *) retlist.read<gsi::IterAdaptorAbstractBase *> (heap);
    ret = (PyObject *) PYAIteratorObject::create (self ? self->py_object () : 0, iter, &meth->ret_type ());

  } else if (meth->ret_type () == s_void_type && self != 0) {

    //  simple, yet magical :)
    ret = self->py_object ();
    Py_INCREF (ret);

  } else {

    ret = pop_arg (meth->ret_type (), retlist, self, heap).release ();

  }

  return ret;
}

static const gsi::MethodBase *
match_method (int mid, PyObject *self, PyObject *args, bool strict)
{
  const gsi::ClassBase *cls_decl = 0;

  PYAObjectBase *p = 0;
  if (! PyType_Check (self)) {
    p = PYAObjectBase::from_pyobject (self);
    cls_decl = p->cls_decl ();
  } else {
    cls_decl = PythonModule::cls_for_type ((PyTypeObject *) self);
  }

  tl_assert (cls_decl != 0);

  int argc = args == NULL ? 0 : int (PyTuple_Size (args));

  //  get number of candidates by argument count
  const gsi::MethodBase *meth = 0;
  unsigned int candidates = 0;

  const MethodTable *mt = MethodTable::method_table_by_class (cls_decl);
  tl_assert (mt);

  //  locate the method in the base classes method table if necessary
  while (mid < int (mt->bottom_mid ())) {

    tl_assert (cls_decl->base ());
    cls_decl = cls_decl->base ();
    mt = MethodTable::method_table_by_class (cls_decl);
    tl_assert (mt);

  }

  for (MethodTableEntry::method_iterator m = mt->begin (mid); m != mt->end (mid); ++m) {

    if ((*m)->is_callback()) {

      //  ignore callbacks

    } else if ((*m)->compatible_with_num_args (argc)) {

      ++candidates;
      meth = *m;

    }

  }

  //  no candidate -> error
  if (! meth) {

    if (! strict) {
      return 0;
    }

    std::set<unsigned int> nargs;
    for (MethodTableEntry::method_iterator m = mt->begin (mid); m != mt->end (mid); ++m) {
      if (! (*m)->is_callback ()) {
        nargs.insert (std::distance ((*m)->begin_arguments (), (*m)->end_arguments ()));
      }
    }

    std::string nargs_s;
    for (std::set<unsigned int>::const_iterator na = nargs.begin (); na != nargs.end (); ++na) {
      if (na != nargs.begin ()) {
        nargs_s += "/";
      }
      nargs_s += tl::to_string (*na);
    }

    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Invalid number of arguments (got %d, expected %s)")), argc, nargs_s));

  }

  //  more than one candidate -> refine by checking the arguments
  if (candidates > 1) {

    meth = 0;
    candidates = 0;
    int score = 0;
    bool const_matching = true;

    for (MethodTableEntry::method_iterator m = mt->begin (mid); m != mt->end (mid); ++m) {

      if (! (*m)->is_callback ()) {

        //  check arguments (count and type)
        bool is_valid = (*m)->compatible_with_num_args (argc);
        int sc = 0;
        int i = 0;
        for (gsi::MethodBase::argument_iterator a = (*m)->begin_arguments (); is_valid && i < argc && a != (*m)->end_arguments (); ++a, ++i) {
          if (test_arg (*a, PyTuple_GetItem (args, i), false /*strict*/)) {
            ++sc;
          } else if (test_arg (*a, PyTuple_GetItem (args, i), true /*loose*/)) {
            //  non-scoring match
          } else {
            is_valid = false;
          }
        }

        if (is_valid && p) {

          //  constness matching candidates have precedence
          if ((*m)->is_const () != p->const_ref ()) {
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

  } else if (meth && mt->fallback_not_implemented (mid)) {

    //  one candidate, but needs checking whether compatibility is given - this avoid having to route NotImplemented over TypeError exceptions later
    int i = 0;
    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); i < argc && a != meth->end_arguments (); ++a, ++i) {
      if (! test_arg (*a, PyTuple_GetItem (args, i), true /*loose*/)) {
        return 0;
      }
    }

  }

  if (! meth) {
    if (! strict || mt->fallback_not_implemented (mid)) {
      return 0;
    } else {
      throw tl::TypeError (tl::to_string (tr ("No overload with matching arguments")));
    }
  }

  if (candidates > 1) {
    if (! strict || mt->fallback_not_implemented (mid)) {
      return 0;
    } else {
      throw tl::TypeError (tl::to_string (tr ("Ambiguous overload variants - multiple method declarations match arguments")));
    }
  }

  return meth;
}

/**
 *  @brief Implements dup
 */
static PyObject *
object_dup (PyObject *self, PyObject *args)
{
  const gsi::ClassBase *cls_decl_self = PythonModule::cls_for_type (Py_TYPE (self));
  tl_assert (cls_decl_self != 0);

  if (! PyArg_ParseTuple (args, "")) {
    return NULL;
  }

  if (! cls_decl_self->can_copy ()) {
    throw tl::Exception (tl::to_string (tr ("No copy constructor provided for class '%s'")), cls_decl_self->name ());
  }

  PyObject *new_object = Py_TYPE (self)->tp_alloc (Py_TYPE (self), 0);
  PythonRef obj (new_object);
  PYAObjectBase *new_pya_base = PYAObjectBase::from_pyobject_unsafe (new_object);
  new (new_pya_base) PYAObjectBase (cls_decl_self, new_object);
  new_pya_base->set (cls_decl_self->clone (PYAObjectBase::from_pyobject (self)->obj ()), true, false, false);

  return obj.release ();
}

/**
 *  @brief Implements assign
 */
static PyObject *
object_assign (PyObject *self, PyObject *args)
{
  const gsi::ClassBase *cls_decl_self = PythonModule::cls_for_type (Py_TYPE (self));
  tl_assert (cls_decl_self != 0);

  PyObject *src = NULL;
  if (! PyArg_ParseTuple (args, "O", &src)) {
    return NULL;
  }

  const gsi::ClassBase *cls_decl_src = PythonModule::cls_for_type (Py_TYPE (src));
  tl_assert (cls_decl_src != 0);

  if (cls_decl_src != cls_decl_self) {
    throw tl::Exception (tl::to_string (tr ("Type is not identical on assign")));
  }
  if (! cls_decl_self->can_copy ()) {
    throw tl::Exception (tl::to_string (tr ("No assignment provided for class '%s'")), cls_decl_self->name ());
  }

  cls_decl_self->assign ((PYAObjectBase::from_pyobject (self))->obj (), (PYAObjectBase::from_pyobject (src))->obj ());

  Py_INCREF (self);
  return self;
}

/**
 *  @brief Default implementation of "__deepcopy__"
 */
PyObject *
object_default_deepcopy_impl (PyObject *self, PyObject * /*args*/)
{
  PyObject *copy_method = PyObject_GetAttrString (self, "__copy__");
  tl_assert (copy_method != NULL);

  PythonRef empty_args (PyTuple_New (0));
  return PyObject_Call (copy_method, empty_args.get (), NULL);
}

/**
 *  @brief Default implementation of "__ne__"
 */
PyObject *
object_default_ne_impl (PyObject *self, PyObject *args)
{
  PyObject *eq_method = PyObject_GetAttrString (self, "__eq__");
  tl_assert (eq_method != NULL);

  PythonRef res (PyObject_Call (eq_method, args, NULL));
  if (! res) {
    return NULL;
  } else {
    return c2python (! python2c<bool> (res.get ()));
  }
}

/**
 *  @brief Default implementation of "__ge__"
 */
PyObject *
object_default_ge_impl (PyObject *self, PyObject *args)
{
  PyObject *eq_method = PyObject_GetAttrString (self, "__lt__");
  tl_assert (eq_method != NULL);

  PythonRef res (PyObject_Call (eq_method, args, NULL));
  if (! res) {
    return NULL;
  } else {
    return c2python (! python2c<bool> (res.get ()));
  }
}

/**
 *  @brief Default implementation of "__le__"
 */
PyObject *
object_default_le_impl (PyObject *self, PyObject *args)
{
  PyObject *eq_method = PyObject_GetAttrString (self, "__eq__");
  tl_assert (eq_method != NULL);

  PyObject *lt_method = PyObject_GetAttrString (self, "__lt__");
  tl_assert (lt_method != NULL);

  PythonRef eq_res (PyObject_Call (eq_method, args, NULL));
  if (! eq_res) {
    return NULL;
  }
  PythonRef lt_res (PyObject_Call (lt_method, args, NULL));
  if (! lt_res) {
    return NULL;
  }
  return c2python (python2c<bool> (eq_res.get ()) || python2c<bool> (lt_res.get ()));
}

/**
 *  @brief Default implementation of "__gt__"
 */
PyObject *
object_default_gt_impl (PyObject *self, PyObject *args)
{
  PyObject *eq_method = PyObject_GetAttrString (self, "__eq__");
  tl_assert (eq_method != NULL);

  PyObject *lt_method = PyObject_GetAttrString (self, "__lt__");
  tl_assert (lt_method != NULL);

  PythonRef eq_res (PyObject_Call (eq_method, args, NULL));
  if (! eq_res) {
    return NULL;
  }
  PythonRef lt_res (PyObject_Call (lt_method, args, NULL));
  if (! lt_res) {
    return NULL;
  }
  return c2python (! (python2c<bool> (eq_res.get ()) || python2c<bool> (lt_res.get ())));
}

/**
 *  @brief Implements create
 */
static PyObject *
object_create (PyObject *self, PyObject *args)
{
  if (! PyArg_ParseTuple (args, "")) {
    return NULL;
  }

  (PYAObjectBase::from_pyobject (self))->obj ();
  Py_RETURN_NONE;
}

/**
 *  @brief Implements release
 */
static PyObject *
object_release (PyObject *self, PyObject *args)
{
  if (! PyArg_ParseTuple (args, "")) {
    return NULL;
  }

  (PYAObjectBase::from_pyobject (self))->release ();
  Py_RETURN_NONE;
}

/**
 *  @brief Implements keep
 */
static PyObject *
object_keep (PyObject *self, PyObject *args)
{
  if (! PyArg_ParseTuple (args, "")) {
    return NULL;
  }

  (PYAObjectBase::from_pyobject (self))->keep ();
  Py_RETURN_NONE;
}

/**
 *  @brief Implements destroy
 */
static PyObject *
object_destroy (PyObject *self, PyObject *args)
{
  if (! PyArg_ParseTuple (args, "")) {
    return NULL;
  }

  (PYAObjectBase::from_pyobject (self))->destroy ();
  Py_RETURN_NONE;
}

/**
 *  @brief Implements destroyed
 */
static PyObject *
object_destroyed (PyObject *self, PyObject *args)
{
  if (! PyArg_ParseTuple (args, "")) {
    return NULL;
  }

  return c2python (PYAObjectBase::from_pyobject (self)->destroyed ());
}

/**
 *  @brief Implements is_const
 */
static PyObject *
object_is_const (PyObject *self, PyObject *args)
{
  if (! PyArg_ParseTuple (args, "")) {
    return NULL;
  }

  return c2python (PYAObjectBase::from_pyobject (self)->const_ref ());
}

static PyObject *
special_method_impl (gsi::MethodBase::special_method_type smt, PyObject *self, PyObject *args)
{
  if (smt == gsi::MethodBase::Destroy) {
    return object_destroy (self, args);
  } else if (smt == gsi::MethodBase::Keep) {
    return object_keep (self, args);
  } else if (smt == gsi::MethodBase::Release) {
    return object_release (self, args);
  } else if (smt == gsi::MethodBase::Create) {
    return object_create (self, args);
  } else if (smt == gsi::MethodBase::IsConst) {
    return object_is_const (self, args);
  } else if (smt == gsi::MethodBase::Destroyed) {
    return object_destroyed (self, args);
  } else if (smt == gsi::MethodBase::Assign) {
    return object_assign (self, args);
  } else if (smt == gsi::MethodBase::Dup) {
    return object_dup (self, args);
  } else {
    Py_RETURN_NONE;
  }
}

static void
push_args (gsi::SerialArgs &arglist, const gsi::MethodBase *meth, PyObject *args, tl::Heap &heap)
{
  int i = 0;
  int argc = args == NULL ? 0 : int (PyTuple_Size (args));

  try {

    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); i < argc && a != meth->end_arguments (); ++a, ++i) {
      push_arg (*a, arglist, PyTuple_GetItem (args, i), heap);
    }

  } catch (tl::Exception &ex) {

    //  In case of an error upon write, pop the arguments to clean them up.
    //  Without this, there is a risk to keep dead objects on the stack.
    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); a != meth->end_arguments () && arglist; ++a) {
      pop_arg (*a, arglist, 0, heap);
    }

    std::string msg;
    const gsi::ArgSpecBase *arg_spec = meth->begin_arguments () [i].spec ();

    if (arg_spec && ! arg_spec->name ().empty ()) {
      msg = tl::sprintf (tl::to_string (tr ("%s for argument #%d ('%s')")), ex.basic_msg (), i + 1, arg_spec->name ());
    } else {
      msg = tl::sprintf (tl::to_string (tr ("%s for argument #%d")), ex.basic_msg (), i + 1);
    }

    ex.set_basic_msg (msg);
    throw;

  } catch (...) {

    //  In case of an error upon write, pop the arguments to clean them up.
    //  Without this, there is a risk to keep dead objects on the stack.
    for (gsi::MethodBase::argument_iterator a = meth->begin_arguments (); a != meth->end_arguments () && arglist; ++a) {
      pop_arg (*a, arglist, 0, heap);
    }

    throw;

  }
}

static PyObject *
method_adaptor (int mid, PyObject *self, PyObject *args)
{
  PyObject *ret = NULL;

  PYA_TRY

    const gsi::MethodBase *meth = match_method (mid, self, args, true);

    //  method is not implemented
    if (! meth) {
#if PY_MAJOR_VERSION < 3
      PyErr_SetNone(PyExc_NotImplementedError);
      return NULL;
#else
      Py_RETURN_NOTIMPLEMENTED;
#endif
    }

    //  handle special methods
    if (meth->smt () != gsi::MethodBase::None) {

      ret = special_method_impl (meth->smt (), self, args);

    } else {

      PYAObjectBase *p = 0;
      if (! PyType_Check (self)) {
        //  non-static method
        p = PYAObjectBase::from_pyobject (self);
      }

      tl::Heap heap;

      if (p && p->const_ref () && ! meth->is_const ()) {
        throw tl::Exception (tl::to_string (tr ("Cannot call non-const method on a const reference")));
      }

      void *obj = 0;
      if (p) {
        //  Hint: this potentially instantiates the object
        obj = p->obj ();
      }

      gsi::SerialArgs retlist (meth->retsize ());
      gsi::SerialArgs arglist (meth->argsize ());

      push_args (arglist, meth, args, heap);

      meth->call (obj, arglist, retlist);

      ret = get_return_value (p, retlist, meth, heap);

      if (ret == NULL) {
        Py_INCREF (Py_None);
        ret = Py_None;
      }

    }

  PYA_CATCH(method_name_from_id (mid, self))

  return ret;
}

static PyObject *property_getter_impl (int mid, PyObject *self);

static PyObject *
property_getter_adaptor (int mid, PyObject *self, PyObject *args)
{
  PyObject *ret = NULL;

  PYA_TRY

    int argc = args == NULL ? 0 : int (PyTuple_Size (args));
    if (argc != 0) {
      throw tl::Exception (tl::to_string (tr ("Property getters must not have an argument")));
    }

    ret = property_getter_impl (mid, self);

  PYA_CATCH(property_name_from_id (mid, self))

  return ret;
}

static PyObject *property_setter_impl (int mid, PyObject *self, PyObject *value);

static PyObject *
property_setter_adaptor (int mid, PyObject *self, PyObject *args)
{
  PyObject *ret = NULL;

  PYA_TRY

    int argc = args == NULL ? 0 : int (PyTuple_Size (args));
    if (argc != 1) {
      throw tl::Exception (tl::to_string (tr ("Property setter needs exactly one argument")));
    }

    PyObject *value = PyTuple_GetItem (args, 0);
    if (value) {
      ret = property_setter_impl (mid, self, value);
    }

  PYA_CATCH(property_name_from_id (mid, self))

  return ret;
}

/**
 *  @brief __init__ implementation (bound to method ith id 'mid')
 */
static PyObject *
method_init_adaptor (int mid, PyObject *self, PyObject *args)
{
  PYA_TRY

    PYAObjectBase *p = PYAObjectBase::from_pyobject (self);

    //  delete any object which we may have already
    if (p->is_attached ()) {
      p->destroy ();
    }

    const gsi::MethodBase *meth = match_method (mid, self, args, PyTuple_Size (args) > 0 || ! p->cls_decl ()->can_default_create ());

    if (meth && meth->smt () == gsi::MethodBase::None) {

      tl::Heap heap;

      gsi::SerialArgs retlist (meth->retsize ());
      gsi::SerialArgs arglist (meth->argsize ());

      push_args (arglist, meth, args, heap);

      meth->call (0, arglist, retlist);

      void *obj = retlist.read<void *> (heap);
      if (obj) {
        p->set (obj, true, false, true);
      }

    } else {

      //  No action required - the object is default-created later once it is really required.
      if (! PyArg_ParseTuple (args, "")) {
        return NULL;
      }

    }

    Py_RETURN_NONE;

  PYA_CATCH(method_name_from_id (mid, self))

  return NULL;
}


static PyObject *
property_getter_impl (int mid, PyObject *self)
{
  const gsi::ClassBase *cls_decl;

  PYAObjectBase *p = 0;
  if (! PyType_Check (self)) {
    p = PYAObjectBase::from_pyobject (self);
    cls_decl = p->cls_decl ();
  } else {
    cls_decl = PythonModule::cls_for_type ((PyTypeObject *) self);
  }

  const MethodTable *mt = MethodTable::method_table_by_class (cls_decl);
  tl_assert (mt);

  //  locate the method in the base classes method table if necessary
  while (mid < int (mt->bottom_property_mid ())) {

    tl_assert (cls_decl->base ());
    cls_decl = cls_decl->base ();
    mt = MethodTable::method_table_by_class (cls_decl);
    tl_assert (mt);

  }

  //  fetch the (only) getter method
  const gsi::MethodBase *meth = 0;
  if (mt->begin_getters (mid) != mt->end_getters (mid)) {
    meth = *mt->begin_getters (mid);
  } else {
    throw tl::Exception (tl::to_string (tr ("Internal error: cannot locate getter method")));
  }

  if (meth->is_signal ()) {

    //  a signal getter is implemented as returning a proxy object for the signal which allows manipulation
    //  of the signal
    tl_assert (p != 0);  //  no static signals
    return PYASignal::create (self, p->signal_handler (meth));

  } else {

    //  getter must not have arguments
    if (meth->argsize () > 0) {
      throw tl::Exception (tl::to_string (tr ("Internal error: getters must not have arguments")));
    }

    void *obj = 0;
    if (p) {
      //  Hint: this potentially instantiates the object
      obj = p->obj ();
    }

    tl::Heap heap;

    gsi::SerialArgs retlist (meth->retsize ());
    gsi::SerialArgs arglist (0);
    meth->call (obj, arglist, retlist);

    PyObject *ret = get_return_value (p, retlist, meth, heap);

    if (ret == NULL) {
      Py_INCREF (Py_None);
      ret = Py_None;
    }

    return ret;

  }
}

PyObject *
property_getter_func (PyObject *self, void *closure)
{
  PyObject *ret = NULL;
  PYA_TRY
    ret = property_getter_impl (getter_from_closure (closure), self);
  PYA_CATCH(property_name_from_id (getter_from_closure (closure), self))
  return ret;
}

static PyObject *
property_setter_impl (int mid, PyObject *self, PyObject *value)
{
  const gsi::ClassBase *cls_decl;

  PYAObjectBase *p = 0;
  if (! PyType_Check (self)) {
    p = PYAObjectBase::from_pyobject (self);
    cls_decl = p->cls_decl ();
  } else {
    cls_decl = PythonModule::cls_for_type ((PyTypeObject *) self);
  }

  if (p && p->const_ref ()) {
    throw tl::Exception (tl::to_string (tr ("Cannot call a setter on a const reference")));
  }

  const MethodTable *mt = MethodTable::method_table_by_class (cls_decl);
  tl_assert (mt);

  //  locate the method in the base classes method table if necessary
  while (mid < int (mt->bottom_property_mid ())) {

    tl_assert (cls_decl->base ());
    cls_decl = cls_decl->base ();
    mt = MethodTable::method_table_by_class (cls_decl);
    tl_assert (mt);

  }

  if (mt->begin_setters (mid) == mt->end_setters (mid)) {
    throw tl::Exception (tl::to_string (tr ("Internal error: cannot locate setter method")));
  }

  const gsi::MethodBase *meth = 0;
  int candidates = 0;

  //  Find the setter among the methods
  for (MethodTableEntry::method_iterator m = mt->begin_setters (mid); m != mt->end_setters (mid); ++m) {

    if ((*m)->is_signal ()) {

      candidates = 1;
      meth = *m;
      break;

    } else if ((*m)->compatible_with_num_args (1)) {

      ++candidates;
      meth = *m;

    }

  }

  //  no candidate -> error
  if (! meth) {
    throw tl::Exception (tl::to_string (tr ("Internal error: no setter compatible with one argument")));
  }

  //  more than one candidate -> refine by checking the arguments
  if (candidates > 1) {

    //  two passes where the second is with loose checking
    int pass = 0;

    do {

      meth = 0;
      candidates = 0;

      for (MethodTableEntry::method_iterator m = mt->begin_setters (mid); m != mt->end_setters (mid); ++m) {

        //  check arguments (count and type)
        bool is_valid = (*m)->compatible_with_num_args (1);
        if (is_valid && ! test_arg (*(*m)->begin_arguments (), value, pass != 0 /*loose in the second pass*/)) {
          is_valid = false;
        }

        if (is_valid) {
          ++candidates;
          meth = *m;
        }

      }

      ++pass;

    } while (! meth && pass < 2);

  }

  if (! meth) {
    throw tl::Exception (tl::to_string (tr ("No setter overload with matching arguments")));
  } else if (candidates > 1) {
    throw tl::Exception (tl::to_string (tr ("Ambiguous overload variants - multiple setter declarations match arguments")));
  }

  void *obj = 0;
  if (p) {
    //  Hint: this potentially instantiates the object
    obj = p->obj ();
  }

  if (meth->is_signal ()) {

    if (!p) {

      //  TODO: Static signals?

    } else if (PyObject_IsInstance (value, (PyObject *) PYASignal::cls)) {

      //  assigning a signal to a signal works if it applies to the same handler -
      //  this simplifies the implementation of += and -=.
      if (p->signal_handler (meth) != ((PYASignal *) value)->handler.get ()) {
        throw tl::Exception (tl::to_string (tr ("Invalid assignment of signal to signal")));
      }

    } else if (value == Py_None) {

      //  assigning None means "clear"
      p->signal_handler (meth)->clear ();

    } else if (! PyCallable_Check (value)) {
      throw tl::Exception (tl::to_string (tr ("A signal needs to be assigned a callable object")));
    } else {

      //  assigning a callable
      pya::SignalHandler *handler = p->signal_handler (meth);
      handler->clear ();
      handler->add (value);

    }

    Py_RETURN_NONE;

  } else {

    gsi::SerialArgs retlist (meth->retsize ());
    gsi::SerialArgs arglist (meth->argsize ());

    tl::Heap heap;
    gsi::MethodBase::argument_iterator a = meth->begin_arguments ();
    push_arg (*a, arglist, value, heap);

    meth->call (obj, arglist, retlist);

    return get_return_value (p, retlist, meth, heap);

  }
}

int
property_setter_func (PyObject *self, PyObject *value, void *closure)
{
  int res = -1;

  PYA_TRY

    PyObject *ret = property_setter_impl (setter_from_closure (closure), self, value);

    //  ignore the result
    if (ret != NULL) {
      Py_DECREF (ret);
    }

    res = 0;

  PYA_CATCH(property_name_from_id (setter_from_closure (closure), self))

  return res;
}

//  Adaptor arrays

template <int N>
PyObject *method_adaptor (PyObject *self, PyObject *args)
{
  return method_adaptor (N, self, args);
}

static py_func_ptr_t method_adaptors [] =
{
  &method_adaptor<0x000>, &method_adaptor<0x001>, &method_adaptor<0x002>, &method_adaptor<0x003>, &method_adaptor<0x004>, &method_adaptor<0x005>, &method_adaptor<0x006>, &method_adaptor<0x007>,
  &method_adaptor<0x008>, &method_adaptor<0x009>, &method_adaptor<0x00a>, &method_adaptor<0x00b>, &method_adaptor<0x00c>, &method_adaptor<0x00d>, &method_adaptor<0x00e>, &method_adaptor<0x00f>,
  &method_adaptor<0x010>, &method_adaptor<0x011>, &method_adaptor<0x012>, &method_adaptor<0x013>, &method_adaptor<0x014>, &method_adaptor<0x015>, &method_adaptor<0x016>, &method_adaptor<0x017>,
  &method_adaptor<0x018>, &method_adaptor<0x019>, &method_adaptor<0x01a>, &method_adaptor<0x01b>, &method_adaptor<0x01c>, &method_adaptor<0x01d>, &method_adaptor<0x01e>, &method_adaptor<0x01f>,
  &method_adaptor<0x020>, &method_adaptor<0x021>, &method_adaptor<0x022>, &method_adaptor<0x023>, &method_adaptor<0x024>, &method_adaptor<0x025>, &method_adaptor<0x026>, &method_adaptor<0x027>,
  &method_adaptor<0x028>, &method_adaptor<0x029>, &method_adaptor<0x02a>, &method_adaptor<0x02b>, &method_adaptor<0x02c>, &method_adaptor<0x02d>, &method_adaptor<0x02e>, &method_adaptor<0x02f>,
  &method_adaptor<0x030>, &method_adaptor<0x031>, &method_adaptor<0x032>, &method_adaptor<0x033>, &method_adaptor<0x034>, &method_adaptor<0x035>, &method_adaptor<0x036>, &method_adaptor<0x037>,
  &method_adaptor<0x038>, &method_adaptor<0x039>, &method_adaptor<0x03a>, &method_adaptor<0x03b>, &method_adaptor<0x03c>, &method_adaptor<0x03d>, &method_adaptor<0x03e>, &method_adaptor<0x03f>,
  &method_adaptor<0x040>, &method_adaptor<0x041>, &method_adaptor<0x042>, &method_adaptor<0x043>, &method_adaptor<0x044>, &method_adaptor<0x045>, &method_adaptor<0x046>, &method_adaptor<0x047>,
  &method_adaptor<0x048>, &method_adaptor<0x049>, &method_adaptor<0x04a>, &method_adaptor<0x04b>, &method_adaptor<0x04c>, &method_adaptor<0x04d>, &method_adaptor<0x04e>, &method_adaptor<0x04f>,
  &method_adaptor<0x050>, &method_adaptor<0x051>, &method_adaptor<0x052>, &method_adaptor<0x053>, &method_adaptor<0x054>, &method_adaptor<0x055>, &method_adaptor<0x056>, &method_adaptor<0x057>,
  &method_adaptor<0x058>, &method_adaptor<0x059>, &method_adaptor<0x05a>, &method_adaptor<0x05b>, &method_adaptor<0x05c>, &method_adaptor<0x05d>, &method_adaptor<0x05e>, &method_adaptor<0x05f>,
  &method_adaptor<0x060>, &method_adaptor<0x061>, &method_adaptor<0x062>, &method_adaptor<0x063>, &method_adaptor<0x064>, &method_adaptor<0x065>, &method_adaptor<0x066>, &method_adaptor<0x067>,
  &method_adaptor<0x068>, &method_adaptor<0x069>, &method_adaptor<0x06a>, &method_adaptor<0x06b>, &method_adaptor<0x06c>, &method_adaptor<0x06d>, &method_adaptor<0x06e>, &method_adaptor<0x06f>,
  &method_adaptor<0x070>, &method_adaptor<0x071>, &method_adaptor<0x072>, &method_adaptor<0x073>, &method_adaptor<0x074>, &method_adaptor<0x075>, &method_adaptor<0x076>, &method_adaptor<0x077>,
  &method_adaptor<0x078>, &method_adaptor<0x079>, &method_adaptor<0x07a>, &method_adaptor<0x07b>, &method_adaptor<0x07c>, &method_adaptor<0x07d>, &method_adaptor<0x07e>, &method_adaptor<0x07f>,
  &method_adaptor<0x080>, &method_adaptor<0x081>, &method_adaptor<0x082>, &method_adaptor<0x083>, &method_adaptor<0x084>, &method_adaptor<0x085>, &method_adaptor<0x086>, &method_adaptor<0x087>,
  &method_adaptor<0x088>, &method_adaptor<0x089>, &method_adaptor<0x08a>, &method_adaptor<0x08b>, &method_adaptor<0x08c>, &method_adaptor<0x08d>, &method_adaptor<0x08e>, &method_adaptor<0x08f>,
  &method_adaptor<0x090>, &method_adaptor<0x091>, &method_adaptor<0x092>, &method_adaptor<0x093>, &method_adaptor<0x094>, &method_adaptor<0x095>, &method_adaptor<0x096>, &method_adaptor<0x097>,
  &method_adaptor<0x098>, &method_adaptor<0x099>, &method_adaptor<0x09a>, &method_adaptor<0x09b>, &method_adaptor<0x09c>, &method_adaptor<0x09d>, &method_adaptor<0x09e>, &method_adaptor<0x09f>,
  &method_adaptor<0x0a0>, &method_adaptor<0x0a1>, &method_adaptor<0x0a2>, &method_adaptor<0x0a3>, &method_adaptor<0x0a4>, &method_adaptor<0x0a5>, &method_adaptor<0x0a6>, &method_adaptor<0x0a7>,
  &method_adaptor<0x0a8>, &method_adaptor<0x0a9>, &method_adaptor<0x0aa>, &method_adaptor<0x0ab>, &method_adaptor<0x0ac>, &method_adaptor<0x0ad>, &method_adaptor<0x0ae>, &method_adaptor<0x0af>,
  &method_adaptor<0x0b0>, &method_adaptor<0x0b1>, &method_adaptor<0x0b2>, &method_adaptor<0x0b3>, &method_adaptor<0x0b4>, &method_adaptor<0x0b5>, &method_adaptor<0x0b6>, &method_adaptor<0x0b7>,
  &method_adaptor<0x0b8>, &method_adaptor<0x0b9>, &method_adaptor<0x0ba>, &method_adaptor<0x0bb>, &method_adaptor<0x0bc>, &method_adaptor<0x0bd>, &method_adaptor<0x0be>, &method_adaptor<0x0bf>,
  &method_adaptor<0x0c0>, &method_adaptor<0x0c1>, &method_adaptor<0x0c2>, &method_adaptor<0x0c3>, &method_adaptor<0x0c4>, &method_adaptor<0x0c5>, &method_adaptor<0x0c6>, &method_adaptor<0x0c7>,
  &method_adaptor<0x0c8>, &method_adaptor<0x0c9>, &method_adaptor<0x0ca>, &method_adaptor<0x0cb>, &method_adaptor<0x0cc>, &method_adaptor<0x0cd>, &method_adaptor<0x0ce>, &method_adaptor<0x0cf>,
  &method_adaptor<0x0d0>, &method_adaptor<0x0d1>, &method_adaptor<0x0d2>, &method_adaptor<0x0d3>, &method_adaptor<0x0d4>, &method_adaptor<0x0d5>, &method_adaptor<0x0d6>, &method_adaptor<0x0d7>,
  &method_adaptor<0x0d8>, &method_adaptor<0x0d9>, &method_adaptor<0x0da>, &method_adaptor<0x0db>, &method_adaptor<0x0dc>, &method_adaptor<0x0dd>, &method_adaptor<0x0de>, &method_adaptor<0x0df>,
  &method_adaptor<0x0e0>, &method_adaptor<0x0e1>, &method_adaptor<0x0e2>, &method_adaptor<0x0e3>, &method_adaptor<0x0e4>, &method_adaptor<0x0e5>, &method_adaptor<0x0e6>, &method_adaptor<0x0e7>,
  &method_adaptor<0x0e8>, &method_adaptor<0x0e9>, &method_adaptor<0x0ea>, &method_adaptor<0x0eb>, &method_adaptor<0x0ec>, &method_adaptor<0x0ed>, &method_adaptor<0x0ee>, &method_adaptor<0x0ef>,
  &method_adaptor<0x0f0>, &method_adaptor<0x0f1>, &method_adaptor<0x0f2>, &method_adaptor<0x0f3>, &method_adaptor<0x0f4>, &method_adaptor<0x0f5>, &method_adaptor<0x0f6>, &method_adaptor<0x0f7>,
  &method_adaptor<0x0f8>, &method_adaptor<0x0f9>, &method_adaptor<0x0fa>, &method_adaptor<0x0fb>, &method_adaptor<0x0fc>, &method_adaptor<0x0fd>, &method_adaptor<0x0fe>, &method_adaptor<0x0ff>,
  &method_adaptor<0x100>, &method_adaptor<0x101>, &method_adaptor<0x102>, &method_adaptor<0x103>, &method_adaptor<0x104>, &method_adaptor<0x105>, &method_adaptor<0x106>, &method_adaptor<0x107>,
  &method_adaptor<0x108>, &method_adaptor<0x109>, &method_adaptor<0x10a>, &method_adaptor<0x10b>, &method_adaptor<0x10c>, &method_adaptor<0x10d>, &method_adaptor<0x10e>, &method_adaptor<0x10f>,
  &method_adaptor<0x110>, &method_adaptor<0x111>, &method_adaptor<0x112>, &method_adaptor<0x113>, &method_adaptor<0x114>, &method_adaptor<0x115>, &method_adaptor<0x116>, &method_adaptor<0x117>,
  &method_adaptor<0x118>, &method_adaptor<0x119>, &method_adaptor<0x11a>, &method_adaptor<0x11b>, &method_adaptor<0x11c>, &method_adaptor<0x11d>, &method_adaptor<0x11e>, &method_adaptor<0x11f>,
  &method_adaptor<0x120>, &method_adaptor<0x121>, &method_adaptor<0x122>, &method_adaptor<0x123>, &method_adaptor<0x124>, &method_adaptor<0x125>, &method_adaptor<0x126>, &method_adaptor<0x127>,
  &method_adaptor<0x128>, &method_adaptor<0x129>, &method_adaptor<0x12a>, &method_adaptor<0x12b>, &method_adaptor<0x12c>, &method_adaptor<0x12d>, &method_adaptor<0x12e>, &method_adaptor<0x12f>,
  &method_adaptor<0x130>, &method_adaptor<0x131>, &method_adaptor<0x132>, &method_adaptor<0x133>, &method_adaptor<0x134>, &method_adaptor<0x135>, &method_adaptor<0x136>, &method_adaptor<0x137>,
  &method_adaptor<0x138>, &method_adaptor<0x139>, &method_adaptor<0x13a>, &method_adaptor<0x13b>, &method_adaptor<0x13c>, &method_adaptor<0x13d>, &method_adaptor<0x13e>, &method_adaptor<0x13f>,
  &method_adaptor<0x140>, &method_adaptor<0x141>, &method_adaptor<0x142>, &method_adaptor<0x143>, &method_adaptor<0x144>, &method_adaptor<0x145>, &method_adaptor<0x146>, &method_adaptor<0x147>,
  &method_adaptor<0x148>, &method_adaptor<0x149>, &method_adaptor<0x14a>, &method_adaptor<0x14b>, &method_adaptor<0x14c>, &method_adaptor<0x14d>, &method_adaptor<0x14e>, &method_adaptor<0x14f>,
  &method_adaptor<0x150>, &method_adaptor<0x151>, &method_adaptor<0x152>, &method_adaptor<0x153>, &method_adaptor<0x154>, &method_adaptor<0x155>, &method_adaptor<0x156>, &method_adaptor<0x157>,
  &method_adaptor<0x158>, &method_adaptor<0x159>, &method_adaptor<0x15a>, &method_adaptor<0x15b>, &method_adaptor<0x15c>, &method_adaptor<0x15d>, &method_adaptor<0x15e>, &method_adaptor<0x15f>,
  &method_adaptor<0x160>, &method_adaptor<0x161>, &method_adaptor<0x162>, &method_adaptor<0x163>, &method_adaptor<0x164>, &method_adaptor<0x165>, &method_adaptor<0x166>, &method_adaptor<0x167>,
  &method_adaptor<0x168>, &method_adaptor<0x169>, &method_adaptor<0x16a>, &method_adaptor<0x16b>, &method_adaptor<0x16c>, &method_adaptor<0x16d>, &method_adaptor<0x16e>, &method_adaptor<0x16f>,
  &method_adaptor<0x170>, &method_adaptor<0x171>, &method_adaptor<0x172>, &method_adaptor<0x173>, &method_adaptor<0x174>, &method_adaptor<0x175>, &method_adaptor<0x176>, &method_adaptor<0x177>,
  &method_adaptor<0x178>, &method_adaptor<0x179>, &method_adaptor<0x17a>, &method_adaptor<0x17b>, &method_adaptor<0x17c>, &method_adaptor<0x17d>, &method_adaptor<0x17e>, &method_adaptor<0x17f>,
  &method_adaptor<0x180>, &method_adaptor<0x181>, &method_adaptor<0x182>, &method_adaptor<0x183>, &method_adaptor<0x184>, &method_adaptor<0x185>, &method_adaptor<0x186>, &method_adaptor<0x187>,
  &method_adaptor<0x188>, &method_adaptor<0x189>, &method_adaptor<0x18a>, &method_adaptor<0x18b>, &method_adaptor<0x18c>, &method_adaptor<0x18d>, &method_adaptor<0x18e>, &method_adaptor<0x18f>,
  &method_adaptor<0x190>, &method_adaptor<0x191>, &method_adaptor<0x192>, &method_adaptor<0x193>, &method_adaptor<0x194>, &method_adaptor<0x195>, &method_adaptor<0x196>, &method_adaptor<0x197>,
  &method_adaptor<0x198>, &method_adaptor<0x199>, &method_adaptor<0x19a>, &method_adaptor<0x19b>, &method_adaptor<0x19c>, &method_adaptor<0x19d>, &method_adaptor<0x19e>, &method_adaptor<0x19f>,
  &method_adaptor<0x1a0>, &method_adaptor<0x1a1>, &method_adaptor<0x1a2>, &method_adaptor<0x1a3>, &method_adaptor<0x1a4>, &method_adaptor<0x1a5>, &method_adaptor<0x1a6>, &method_adaptor<0x1a7>,
  &method_adaptor<0x1a8>, &method_adaptor<0x1a9>, &method_adaptor<0x1aa>, &method_adaptor<0x1ab>, &method_adaptor<0x1ac>, &method_adaptor<0x1ad>, &method_adaptor<0x1ae>, &method_adaptor<0x1af>,
  &method_adaptor<0x1b0>, &method_adaptor<0x1b1>, &method_adaptor<0x1b2>, &method_adaptor<0x1b3>, &method_adaptor<0x1b4>, &method_adaptor<0x1b5>, &method_adaptor<0x1b6>, &method_adaptor<0x1b7>,
  &method_adaptor<0x1b8>, &method_adaptor<0x1b9>, &method_adaptor<0x1ba>, &method_adaptor<0x1bb>, &method_adaptor<0x1bc>, &method_adaptor<0x1bd>, &method_adaptor<0x1be>, &method_adaptor<0x1bf>,
  &method_adaptor<0x1c0>, &method_adaptor<0x1c1>, &method_adaptor<0x1c2>, &method_adaptor<0x1c3>, &method_adaptor<0x1c4>, &method_adaptor<0x1c5>, &method_adaptor<0x1c6>, &method_adaptor<0x1c7>,
  &method_adaptor<0x1c8>, &method_adaptor<0x1c9>, &method_adaptor<0x1ca>, &method_adaptor<0x1cb>, &method_adaptor<0x1cc>, &method_adaptor<0x1cd>, &method_adaptor<0x1ce>, &method_adaptor<0x1cf>,
  &method_adaptor<0x1d0>, &method_adaptor<0x1d1>, &method_adaptor<0x1d2>, &method_adaptor<0x1d3>, &method_adaptor<0x1d4>, &method_adaptor<0x1d5>, &method_adaptor<0x1d6>, &method_adaptor<0x1d7>,
  &method_adaptor<0x1d8>, &method_adaptor<0x1d9>, &method_adaptor<0x1da>, &method_adaptor<0x1db>, &method_adaptor<0x1dc>, &method_adaptor<0x1dd>, &method_adaptor<0x1de>, &method_adaptor<0x1df>,
  &method_adaptor<0x1e0>, &method_adaptor<0x1e1>, &method_adaptor<0x1e2>, &method_adaptor<0x1e3>, &method_adaptor<0x1e4>, &method_adaptor<0x1e5>, &method_adaptor<0x1e6>, &method_adaptor<0x1e7>,
  &method_adaptor<0x1e8>, &method_adaptor<0x1e9>, &method_adaptor<0x1ea>, &method_adaptor<0x1eb>, &method_adaptor<0x1ec>, &method_adaptor<0x1ed>, &method_adaptor<0x1ee>, &method_adaptor<0x1ef>,
  &method_adaptor<0x1f0>, &method_adaptor<0x1f1>, &method_adaptor<0x1f2>, &method_adaptor<0x1f3>, &method_adaptor<0x1f4>, &method_adaptor<0x1f5>, &method_adaptor<0x1f6>, &method_adaptor<0x1f7>,
  &method_adaptor<0x1f8>, &method_adaptor<0x1f9>, &method_adaptor<0x1fa>, &method_adaptor<0x1fb>, &method_adaptor<0x1fc>, &method_adaptor<0x1fd>, &method_adaptor<0x1fe>, &method_adaptor<0x1ff>,
  &method_adaptor<0x200>, &method_adaptor<0x201>, &method_adaptor<0x202>, &method_adaptor<0x203>, &method_adaptor<0x204>, &method_adaptor<0x205>, &method_adaptor<0x206>, &method_adaptor<0x207>,
  &method_adaptor<0x208>, &method_adaptor<0x209>, &method_adaptor<0x20a>, &method_adaptor<0x20b>, &method_adaptor<0x20c>, &method_adaptor<0x20d>, &method_adaptor<0x20e>, &method_adaptor<0x20f>,
  &method_adaptor<0x210>, &method_adaptor<0x211>, &method_adaptor<0x212>, &method_adaptor<0x213>, &method_adaptor<0x214>, &method_adaptor<0x215>, &method_adaptor<0x216>, &method_adaptor<0x217>,
  &method_adaptor<0x218>, &method_adaptor<0x219>, &method_adaptor<0x21a>, &method_adaptor<0x21b>, &method_adaptor<0x21c>, &method_adaptor<0x21d>, &method_adaptor<0x21e>, &method_adaptor<0x21f>,
  &method_adaptor<0x220>, &method_adaptor<0x221>, &method_adaptor<0x222>, &method_adaptor<0x223>, &method_adaptor<0x224>, &method_adaptor<0x225>, &method_adaptor<0x226>, &method_adaptor<0x227>,
  &method_adaptor<0x228>, &method_adaptor<0x229>, &method_adaptor<0x22a>, &method_adaptor<0x22b>, &method_adaptor<0x22c>, &method_adaptor<0x22d>, &method_adaptor<0x22e>, &method_adaptor<0x22f>,
  &method_adaptor<0x230>, &method_adaptor<0x231>, &method_adaptor<0x232>, &method_adaptor<0x233>, &method_adaptor<0x234>, &method_adaptor<0x235>, &method_adaptor<0x236>, &method_adaptor<0x237>,
  &method_adaptor<0x238>, &method_adaptor<0x239>, &method_adaptor<0x23a>, &method_adaptor<0x23b>, &method_adaptor<0x23c>, &method_adaptor<0x23d>, &method_adaptor<0x23e>, &method_adaptor<0x23f>,
  &method_adaptor<0x240>, &method_adaptor<0x241>, &method_adaptor<0x242>, &method_adaptor<0x243>, &method_adaptor<0x244>, &method_adaptor<0x245>, &method_adaptor<0x246>, &method_adaptor<0x247>,
  &method_adaptor<0x248>, &method_adaptor<0x249>, &method_adaptor<0x24a>, &method_adaptor<0x24b>, &method_adaptor<0x24c>, &method_adaptor<0x24d>, &method_adaptor<0x24e>, &method_adaptor<0x24f>,
  &method_adaptor<0x250>, &method_adaptor<0x251>, &method_adaptor<0x252>, &method_adaptor<0x253>, &method_adaptor<0x254>, &method_adaptor<0x255>, &method_adaptor<0x256>, &method_adaptor<0x257>,
  &method_adaptor<0x258>, &method_adaptor<0x259>, &method_adaptor<0x25a>, &method_adaptor<0x25b>, &method_adaptor<0x25c>, &method_adaptor<0x25d>, &method_adaptor<0x25e>, &method_adaptor<0x25f>,
  &method_adaptor<0x260>, &method_adaptor<0x261>, &method_adaptor<0x262>, &method_adaptor<0x263>, &method_adaptor<0x264>, &method_adaptor<0x265>, &method_adaptor<0x266>, &method_adaptor<0x267>,
  &method_adaptor<0x268>, &method_adaptor<0x269>, &method_adaptor<0x26a>, &method_adaptor<0x26b>, &method_adaptor<0x26c>, &method_adaptor<0x26d>, &method_adaptor<0x26e>, &method_adaptor<0x26f>,
  &method_adaptor<0x270>, &method_adaptor<0x271>, &method_adaptor<0x272>, &method_adaptor<0x273>, &method_adaptor<0x274>, &method_adaptor<0x275>, &method_adaptor<0x276>, &method_adaptor<0x277>,
  &method_adaptor<0x278>, &method_adaptor<0x279>, &method_adaptor<0x27a>, &method_adaptor<0x27b>, &method_adaptor<0x27c>, &method_adaptor<0x27d>, &method_adaptor<0x27e>, &method_adaptor<0x27f>,
  &method_adaptor<0x280>, &method_adaptor<0x281>, &method_adaptor<0x282>, &method_adaptor<0x283>, &method_adaptor<0x284>, &method_adaptor<0x285>, &method_adaptor<0x286>, &method_adaptor<0x287>,
  &method_adaptor<0x288>, &method_adaptor<0x289>, &method_adaptor<0x28a>, &method_adaptor<0x28b>, &method_adaptor<0x28c>, &method_adaptor<0x28d>, &method_adaptor<0x28e>, &method_adaptor<0x28f>,
  &method_adaptor<0x290>, &method_adaptor<0x291>, &method_adaptor<0x292>, &method_adaptor<0x293>, &method_adaptor<0x294>, &method_adaptor<0x295>, &method_adaptor<0x296>, &method_adaptor<0x297>,
  &method_adaptor<0x298>, &method_adaptor<0x299>, &method_adaptor<0x29a>, &method_adaptor<0x29b>, &method_adaptor<0x29c>, &method_adaptor<0x29d>, &method_adaptor<0x29e>, &method_adaptor<0x29f>,
  &method_adaptor<0x2a0>, &method_adaptor<0x2a1>, &method_adaptor<0x2a2>, &method_adaptor<0x2a3>, &method_adaptor<0x2a4>, &method_adaptor<0x2a5>, &method_adaptor<0x2a6>, &method_adaptor<0x2a7>,
  &method_adaptor<0x2a8>, &method_adaptor<0x2a9>, &method_adaptor<0x2aa>, &method_adaptor<0x2ab>, &method_adaptor<0x2ac>, &method_adaptor<0x2ad>, &method_adaptor<0x2ae>, &method_adaptor<0x2af>,
  &method_adaptor<0x2b0>, &method_adaptor<0x2b1>, &method_adaptor<0x2b2>, &method_adaptor<0x2b3>, &method_adaptor<0x2b4>, &method_adaptor<0x2b5>, &method_adaptor<0x2b6>, &method_adaptor<0x2b7>,
  &method_adaptor<0x2b8>, &method_adaptor<0x2b9>, &method_adaptor<0x2ba>, &method_adaptor<0x2bb>, &method_adaptor<0x2bc>, &method_adaptor<0x2bd>, &method_adaptor<0x2be>, &method_adaptor<0x2bf>,
  &method_adaptor<0x2c0>, &method_adaptor<0x2c1>, &method_adaptor<0x2c2>, &method_adaptor<0x2c3>, &method_adaptor<0x2c4>, &method_adaptor<0x2c5>, &method_adaptor<0x2c6>, &method_adaptor<0x2c7>,
  &method_adaptor<0x2c8>, &method_adaptor<0x2c9>, &method_adaptor<0x2ca>, &method_adaptor<0x2cb>, &method_adaptor<0x2cc>, &method_adaptor<0x2cd>, &method_adaptor<0x2ce>, &method_adaptor<0x2cf>,
  &method_adaptor<0x2d0>, &method_adaptor<0x2d1>, &method_adaptor<0x2d2>, &method_adaptor<0x2d3>, &method_adaptor<0x2d4>, &method_adaptor<0x2d5>, &method_adaptor<0x2d6>, &method_adaptor<0x2d7>,
  &method_adaptor<0x2d8>, &method_adaptor<0x2d9>, &method_adaptor<0x2da>, &method_adaptor<0x2db>, &method_adaptor<0x2dc>, &method_adaptor<0x2dd>, &method_adaptor<0x2de>, &method_adaptor<0x2df>,
  &method_adaptor<0x2e0>, &method_adaptor<0x2e1>, &method_adaptor<0x2e2>, &method_adaptor<0x2e3>, &method_adaptor<0x2e4>, &method_adaptor<0x2e5>, &method_adaptor<0x2e6>, &method_adaptor<0x2e7>,
  &method_adaptor<0x2e8>, &method_adaptor<0x2e9>, &method_adaptor<0x2ea>, &method_adaptor<0x2eb>, &method_adaptor<0x2ec>, &method_adaptor<0x2ed>, &method_adaptor<0x2ee>, &method_adaptor<0x2ef>,
  &method_adaptor<0x2f0>, &method_adaptor<0x2f1>, &method_adaptor<0x2f2>, &method_adaptor<0x2f3>, &method_adaptor<0x2f4>, &method_adaptor<0x2f5>, &method_adaptor<0x2f6>, &method_adaptor<0x2f7>,
  &method_adaptor<0x2f8>, &method_adaptor<0x2f9>, &method_adaptor<0x2fa>, &method_adaptor<0x2fb>, &method_adaptor<0x2fc>, &method_adaptor<0x2fd>, &method_adaptor<0x2fe>, &method_adaptor<0x2ff>,
  &method_adaptor<0x300>, &method_adaptor<0x301>, &method_adaptor<0x302>, &method_adaptor<0x303>, &method_adaptor<0x304>, &method_adaptor<0x305>, &method_adaptor<0x306>, &method_adaptor<0x307>,
  &method_adaptor<0x308>, &method_adaptor<0x309>, &method_adaptor<0x30a>, &method_adaptor<0x30b>, &method_adaptor<0x30c>, &method_adaptor<0x30d>, &method_adaptor<0x30e>, &method_adaptor<0x30f>,
  &method_adaptor<0x310>, &method_adaptor<0x311>, &method_adaptor<0x312>, &method_adaptor<0x313>, &method_adaptor<0x314>, &method_adaptor<0x315>, &method_adaptor<0x316>, &method_adaptor<0x317>,
  &method_adaptor<0x318>, &method_adaptor<0x319>, &method_adaptor<0x31a>, &method_adaptor<0x31b>, &method_adaptor<0x31c>, &method_adaptor<0x31d>, &method_adaptor<0x31e>, &method_adaptor<0x31f>,
  &method_adaptor<0x320>, &method_adaptor<0x321>, &method_adaptor<0x322>, &method_adaptor<0x323>, &method_adaptor<0x324>, &method_adaptor<0x325>, &method_adaptor<0x326>, &method_adaptor<0x327>,
  &method_adaptor<0x328>, &method_adaptor<0x329>, &method_adaptor<0x32a>, &method_adaptor<0x32b>, &method_adaptor<0x32c>, &method_adaptor<0x32d>, &method_adaptor<0x32e>, &method_adaptor<0x32f>,
  &method_adaptor<0x330>, &method_adaptor<0x331>, &method_adaptor<0x332>, &method_adaptor<0x333>, &method_adaptor<0x334>, &method_adaptor<0x335>, &method_adaptor<0x336>, &method_adaptor<0x337>,
  &method_adaptor<0x338>, &method_adaptor<0x339>, &method_adaptor<0x33a>, &method_adaptor<0x33b>, &method_adaptor<0x33c>, &method_adaptor<0x33d>, &method_adaptor<0x33e>, &method_adaptor<0x33f>,
  &method_adaptor<0x340>, &method_adaptor<0x341>, &method_adaptor<0x342>, &method_adaptor<0x343>, &method_adaptor<0x344>, &method_adaptor<0x345>, &method_adaptor<0x346>, &method_adaptor<0x347>,
  &method_adaptor<0x348>, &method_adaptor<0x349>, &method_adaptor<0x34a>, &method_adaptor<0x34b>, &method_adaptor<0x34c>, &method_adaptor<0x34d>, &method_adaptor<0x34e>, &method_adaptor<0x34f>,
  &method_adaptor<0x350>, &method_adaptor<0x351>, &method_adaptor<0x352>, &method_adaptor<0x353>, &method_adaptor<0x354>, &method_adaptor<0x355>, &method_adaptor<0x356>, &method_adaptor<0x357>,
  &method_adaptor<0x358>, &method_adaptor<0x359>, &method_adaptor<0x35a>, &method_adaptor<0x35b>, &method_adaptor<0x35c>, &method_adaptor<0x35d>, &method_adaptor<0x35e>, &method_adaptor<0x35f>,
  &method_adaptor<0x360>, &method_adaptor<0x361>, &method_adaptor<0x362>, &method_adaptor<0x363>, &method_adaptor<0x364>, &method_adaptor<0x365>, &method_adaptor<0x366>, &method_adaptor<0x367>,
  &method_adaptor<0x368>, &method_adaptor<0x369>, &method_adaptor<0x36a>, &method_adaptor<0x36b>, &method_adaptor<0x36c>, &method_adaptor<0x36d>, &method_adaptor<0x36e>, &method_adaptor<0x36f>,
  &method_adaptor<0x370>, &method_adaptor<0x371>, &method_adaptor<0x372>, &method_adaptor<0x373>, &method_adaptor<0x374>, &method_adaptor<0x375>, &method_adaptor<0x376>, &method_adaptor<0x377>,
  &method_adaptor<0x378>, &method_adaptor<0x379>, &method_adaptor<0x37a>, &method_adaptor<0x37b>, &method_adaptor<0x37c>, &method_adaptor<0x37d>, &method_adaptor<0x37e>, &method_adaptor<0x37f>,
  &method_adaptor<0x380>, &method_adaptor<0x381>, &method_adaptor<0x382>, &method_adaptor<0x383>, &method_adaptor<0x384>, &method_adaptor<0x385>, &method_adaptor<0x386>, &method_adaptor<0x387>,
  &method_adaptor<0x388>, &method_adaptor<0x389>, &method_adaptor<0x38a>, &method_adaptor<0x38b>, &method_adaptor<0x38c>, &method_adaptor<0x38d>, &method_adaptor<0x38e>, &method_adaptor<0x38f>,
  &method_adaptor<0x390>, &method_adaptor<0x391>, &method_adaptor<0x392>, &method_adaptor<0x393>, &method_adaptor<0x394>, &method_adaptor<0x395>, &method_adaptor<0x396>, &method_adaptor<0x397>,
  &method_adaptor<0x398>, &method_adaptor<0x399>, &method_adaptor<0x39a>, &method_adaptor<0x39b>, &method_adaptor<0x39c>, &method_adaptor<0x39d>, &method_adaptor<0x39e>, &method_adaptor<0x39f>,
  &method_adaptor<0x3a0>, &method_adaptor<0x3a1>, &method_adaptor<0x3a2>, &method_adaptor<0x3a3>, &method_adaptor<0x3a4>, &method_adaptor<0x3a5>, &method_adaptor<0x3a6>, &method_adaptor<0x3a7>,
  &method_adaptor<0x3a8>, &method_adaptor<0x3a9>, &method_adaptor<0x3aa>, &method_adaptor<0x3ab>, &method_adaptor<0x3ac>, &method_adaptor<0x3ad>, &method_adaptor<0x3ae>, &method_adaptor<0x3af>,
  &method_adaptor<0x3b0>, &method_adaptor<0x3b1>, &method_adaptor<0x3b2>, &method_adaptor<0x3b3>, &method_adaptor<0x3b4>, &method_adaptor<0x3b5>, &method_adaptor<0x3b6>, &method_adaptor<0x3b7>,
  &method_adaptor<0x3b8>, &method_adaptor<0x3b9>, &method_adaptor<0x3ba>, &method_adaptor<0x3bb>, &method_adaptor<0x3bc>, &method_adaptor<0x3bd>, &method_adaptor<0x3be>, &method_adaptor<0x3bf>,
  &method_adaptor<0x3c0>, &method_adaptor<0x3c1>, &method_adaptor<0x3c2>, &method_adaptor<0x3c3>, &method_adaptor<0x3c4>, &method_adaptor<0x3c5>, &method_adaptor<0x3c6>, &method_adaptor<0x3c7>,
  &method_adaptor<0x3c8>, &method_adaptor<0x3c9>, &method_adaptor<0x3ca>, &method_adaptor<0x3cb>, &method_adaptor<0x3cc>, &method_adaptor<0x3cd>, &method_adaptor<0x3ce>, &method_adaptor<0x3cf>,
  &method_adaptor<0x3d0>, &method_adaptor<0x3d1>, &method_adaptor<0x3d2>, &method_adaptor<0x3d3>, &method_adaptor<0x3d4>, &method_adaptor<0x3d5>, &method_adaptor<0x3d6>, &method_adaptor<0x3d7>,
  &method_adaptor<0x3d8>, &method_adaptor<0x3d9>, &method_adaptor<0x3da>, &method_adaptor<0x3db>, &method_adaptor<0x3dc>, &method_adaptor<0x3dd>, &method_adaptor<0x3de>, &method_adaptor<0x3df>,
  &method_adaptor<0x3e0>, &method_adaptor<0x3e1>, &method_adaptor<0x3e2>, &method_adaptor<0x3e3>, &method_adaptor<0x3e4>, &method_adaptor<0x3e5>, &method_adaptor<0x3e6>, &method_adaptor<0x3e7>,
  &method_adaptor<0x3e8>, &method_adaptor<0x3e9>, &method_adaptor<0x3ea>, &method_adaptor<0x3eb>, &method_adaptor<0x3ec>, &method_adaptor<0x3ed>, &method_adaptor<0x3ee>, &method_adaptor<0x3ef>,
  &method_adaptor<0x3f0>, &method_adaptor<0x3f1>, &method_adaptor<0x3f2>, &method_adaptor<0x3f3>, &method_adaptor<0x3f4>, &method_adaptor<0x3f5>, &method_adaptor<0x3f6>, &method_adaptor<0x3f7>,
  &method_adaptor<0x3f8>, &method_adaptor<0x3f9>, &method_adaptor<0x3fa>, &method_adaptor<0x3fb>, &method_adaptor<0x3fc>, &method_adaptor<0x3fd>, &method_adaptor<0x3fe>, &method_adaptor<0x3ff>,
  &method_adaptor<0x400>, &method_adaptor<0x401>, &method_adaptor<0x402>, &method_adaptor<0x403>, &method_adaptor<0x404>, &method_adaptor<0x405>, &method_adaptor<0x406>, &method_adaptor<0x407>,
  &method_adaptor<0x408>, &method_adaptor<0x409>, &method_adaptor<0x40a>, &method_adaptor<0x40b>, &method_adaptor<0x40c>, &method_adaptor<0x40d>, &method_adaptor<0x40e>, &method_adaptor<0x40f>,
  &method_adaptor<0x410>, &method_adaptor<0x411>, &method_adaptor<0x412>, &method_adaptor<0x413>, &method_adaptor<0x414>, &method_adaptor<0x415>, &method_adaptor<0x416>, &method_adaptor<0x417>,
  &method_adaptor<0x418>, &method_adaptor<0x419>, &method_adaptor<0x41a>, &method_adaptor<0x41b>, &method_adaptor<0x41c>, &method_adaptor<0x41d>, &method_adaptor<0x41e>, &method_adaptor<0x41f>,
  &method_adaptor<0x420>, &method_adaptor<0x421>, &method_adaptor<0x422>, &method_adaptor<0x423>, &method_adaptor<0x424>, &method_adaptor<0x425>, &method_adaptor<0x426>, &method_adaptor<0x427>,
  &method_adaptor<0x428>, &method_adaptor<0x429>, &method_adaptor<0x42a>, &method_adaptor<0x42b>, &method_adaptor<0x42c>, &method_adaptor<0x42d>, &method_adaptor<0x42e>, &method_adaptor<0x42f>,
  &method_adaptor<0x430>, &method_adaptor<0x431>, &method_adaptor<0x432>, &method_adaptor<0x433>, &method_adaptor<0x434>, &method_adaptor<0x435>, &method_adaptor<0x436>, &method_adaptor<0x437>,
  &method_adaptor<0x438>, &method_adaptor<0x439>, &method_adaptor<0x43a>, &method_adaptor<0x43b>, &method_adaptor<0x43c>, &method_adaptor<0x43d>, &method_adaptor<0x43e>, &method_adaptor<0x43f>,
  &method_adaptor<0x440>, &method_adaptor<0x441>, &method_adaptor<0x442>, &method_adaptor<0x443>, &method_adaptor<0x444>, &method_adaptor<0x445>, &method_adaptor<0x446>, &method_adaptor<0x447>,
  &method_adaptor<0x448>, &method_adaptor<0x449>, &method_adaptor<0x44a>, &method_adaptor<0x44b>, &method_adaptor<0x44c>, &method_adaptor<0x44d>, &method_adaptor<0x44e>, &method_adaptor<0x44f>,
  &method_adaptor<0x450>, &method_adaptor<0x451>, &method_adaptor<0x452>, &method_adaptor<0x453>, &method_adaptor<0x454>, &method_adaptor<0x455>, &method_adaptor<0x456>, &method_adaptor<0x457>,
  &method_adaptor<0x458>, &method_adaptor<0x459>, &method_adaptor<0x45a>, &method_adaptor<0x45b>, &method_adaptor<0x45c>, &method_adaptor<0x45d>, &method_adaptor<0x45e>, &method_adaptor<0x45f>,
  &method_adaptor<0x460>, &method_adaptor<0x461>, &method_adaptor<0x462>, &method_adaptor<0x463>, &method_adaptor<0x464>, &method_adaptor<0x465>, &method_adaptor<0x466>, &method_adaptor<0x467>,
  &method_adaptor<0x468>, &method_adaptor<0x469>, &method_adaptor<0x46a>, &method_adaptor<0x46b>, &method_adaptor<0x46c>, &method_adaptor<0x46d>, &method_adaptor<0x46e>, &method_adaptor<0x46f>,
  &method_adaptor<0x470>, &method_adaptor<0x471>, &method_adaptor<0x472>, &method_adaptor<0x473>, &method_adaptor<0x474>, &method_adaptor<0x475>, &method_adaptor<0x476>, &method_adaptor<0x477>,
  &method_adaptor<0x478>, &method_adaptor<0x479>, &method_adaptor<0x47a>, &method_adaptor<0x47b>, &method_adaptor<0x47c>, &method_adaptor<0x47d>, &method_adaptor<0x47e>, &method_adaptor<0x47f>,
  &method_adaptor<0x480>, &method_adaptor<0x481>, &method_adaptor<0x482>, &method_adaptor<0x483>, &method_adaptor<0x484>, &method_adaptor<0x485>, &method_adaptor<0x486>, &method_adaptor<0x487>,
  &method_adaptor<0x488>, &method_adaptor<0x489>, &method_adaptor<0x48a>, &method_adaptor<0x48b>, &method_adaptor<0x48c>, &method_adaptor<0x48d>, &method_adaptor<0x48e>, &method_adaptor<0x48f>,
  &method_adaptor<0x490>, &method_adaptor<0x491>, &method_adaptor<0x492>, &method_adaptor<0x493>, &method_adaptor<0x494>, &method_adaptor<0x495>, &method_adaptor<0x496>, &method_adaptor<0x497>,
  &method_adaptor<0x498>, &method_adaptor<0x499>, &method_adaptor<0x49a>, &method_adaptor<0x49b>, &method_adaptor<0x49c>, &method_adaptor<0x49d>, &method_adaptor<0x49e>, &method_adaptor<0x49f>,
  &method_adaptor<0x4a0>, &method_adaptor<0x4a1>, &method_adaptor<0x4a2>, &method_adaptor<0x4a3>, &method_adaptor<0x4a4>, &method_adaptor<0x4a5>, &method_adaptor<0x4a6>, &method_adaptor<0x4a7>,
  &method_adaptor<0x4a8>, &method_adaptor<0x4a9>, &method_adaptor<0x4aa>, &method_adaptor<0x4ab>, &method_adaptor<0x4ac>, &method_adaptor<0x4ad>, &method_adaptor<0x4ae>, &method_adaptor<0x4af>,
  &method_adaptor<0x4b0>, &method_adaptor<0x4b1>, &method_adaptor<0x4b2>, &method_adaptor<0x4b3>, &method_adaptor<0x4b4>, &method_adaptor<0x4b5>, &method_adaptor<0x4b6>, &method_adaptor<0x4b7>,
  &method_adaptor<0x4b8>, &method_adaptor<0x4b9>, &method_adaptor<0x4ba>, &method_adaptor<0x4bb>, &method_adaptor<0x4bc>, &method_adaptor<0x4bd>, &method_adaptor<0x4be>, &method_adaptor<0x4bf>,
  &method_adaptor<0x4c0>, &method_adaptor<0x4c1>, &method_adaptor<0x4c2>, &method_adaptor<0x4c3>, &method_adaptor<0x4c4>, &method_adaptor<0x4c5>, &method_adaptor<0x4c6>, &method_adaptor<0x4c7>,
  &method_adaptor<0x4c8>, &method_adaptor<0x4c9>, &method_adaptor<0x4ca>, &method_adaptor<0x4cb>, &method_adaptor<0x4cc>, &method_adaptor<0x4cd>, &method_adaptor<0x4ce>, &method_adaptor<0x4cf>,
  &method_adaptor<0x4d0>, &method_adaptor<0x4d1>, &method_adaptor<0x4d2>, &method_adaptor<0x4d3>, &method_adaptor<0x4d4>, &method_adaptor<0x4d5>, &method_adaptor<0x4d6>, &method_adaptor<0x4d7>,
  &method_adaptor<0x4d8>, &method_adaptor<0x4d9>, &method_adaptor<0x4da>, &method_adaptor<0x4db>, &method_adaptor<0x4dc>, &method_adaptor<0x4dd>, &method_adaptor<0x4de>, &method_adaptor<0x4df>,
  &method_adaptor<0x4e0>, &method_adaptor<0x4e1>, &method_adaptor<0x4e2>, &method_adaptor<0x4e3>, &method_adaptor<0x4e4>, &method_adaptor<0x4e5>, &method_adaptor<0x4e6>, &method_adaptor<0x4e7>,
  &method_adaptor<0x4e8>, &method_adaptor<0x4e9>, &method_adaptor<0x4ea>, &method_adaptor<0x4eb>, &method_adaptor<0x4ec>, &method_adaptor<0x4ed>, &method_adaptor<0x4ee>, &method_adaptor<0x4ef>,
  &method_adaptor<0x4f0>, &method_adaptor<0x4f1>, &method_adaptor<0x4f2>, &method_adaptor<0x4f3>, &method_adaptor<0x4f4>, &method_adaptor<0x4f5>, &method_adaptor<0x4f6>, &method_adaptor<0x4f7>,
  &method_adaptor<0x4f8>, &method_adaptor<0x4f9>, &method_adaptor<0x4fa>, &method_adaptor<0x4fb>, &method_adaptor<0x4fc>, &method_adaptor<0x4fd>, &method_adaptor<0x4fe>, &method_adaptor<0x4ff>,
};

py_func_ptr_t get_method_adaptor (int n)
{
  tl_assert (n >= 0 && n < int (sizeof (method_adaptors) / sizeof (method_adaptors [0])));
  return method_adaptors [n];
}

template <int N>
PyObject *property_getter_adaptor (PyObject *self, PyObject *args)
{
  return property_getter_adaptor (N, self, args);
}

static py_func_ptr_t property_getter_adaptors [] =
{
  &property_getter_adaptor<0x000>, &property_getter_adaptor<0x001>, &property_getter_adaptor<0x002>, &property_getter_adaptor<0x003>, &property_getter_adaptor<0x004>, &property_getter_adaptor<0x005>, &property_getter_adaptor<0x006>, &property_getter_adaptor<0x007>,
  &property_getter_adaptor<0x008>, &property_getter_adaptor<0x009>, &property_getter_adaptor<0x00a>, &property_getter_adaptor<0x00b>, &property_getter_adaptor<0x00c>, &property_getter_adaptor<0x00d>, &property_getter_adaptor<0x00e>, &property_getter_adaptor<0x00f>,
  &property_getter_adaptor<0x010>, &property_getter_adaptor<0x011>, &property_getter_adaptor<0x012>, &property_getter_adaptor<0x013>, &property_getter_adaptor<0x014>, &property_getter_adaptor<0x015>, &property_getter_adaptor<0x016>, &property_getter_adaptor<0x017>,
  &property_getter_adaptor<0x018>, &property_getter_adaptor<0x019>, &property_getter_adaptor<0x01a>, &property_getter_adaptor<0x01b>, &property_getter_adaptor<0x01c>, &property_getter_adaptor<0x01d>, &property_getter_adaptor<0x01e>, &property_getter_adaptor<0x01f>,
  &property_getter_adaptor<0x020>, &property_getter_adaptor<0x021>, &property_getter_adaptor<0x022>, &property_getter_adaptor<0x023>, &property_getter_adaptor<0x024>, &property_getter_adaptor<0x025>, &property_getter_adaptor<0x026>, &property_getter_adaptor<0x027>,
  &property_getter_adaptor<0x028>, &property_getter_adaptor<0x029>, &property_getter_adaptor<0x02a>, &property_getter_adaptor<0x02b>, &property_getter_adaptor<0x02c>, &property_getter_adaptor<0x02d>, &property_getter_adaptor<0x02e>, &property_getter_adaptor<0x02f>,
  &property_getter_adaptor<0x030>, &property_getter_adaptor<0x031>, &property_getter_adaptor<0x032>, &property_getter_adaptor<0x033>, &property_getter_adaptor<0x034>, &property_getter_adaptor<0x035>, &property_getter_adaptor<0x036>, &property_getter_adaptor<0x037>,
  &property_getter_adaptor<0x038>, &property_getter_adaptor<0x039>, &property_getter_adaptor<0x03a>, &property_getter_adaptor<0x03b>, &property_getter_adaptor<0x03c>, &property_getter_adaptor<0x03d>, &property_getter_adaptor<0x03e>, &property_getter_adaptor<0x03f>,
  &property_getter_adaptor<0x040>, &property_getter_adaptor<0x041>, &property_getter_adaptor<0x042>, &property_getter_adaptor<0x043>, &property_getter_adaptor<0x044>, &property_getter_adaptor<0x045>, &property_getter_adaptor<0x046>, &property_getter_adaptor<0x047>,
  &property_getter_adaptor<0x048>, &property_getter_adaptor<0x049>, &property_getter_adaptor<0x04a>, &property_getter_adaptor<0x04b>, &property_getter_adaptor<0x04c>, &property_getter_adaptor<0x04d>, &property_getter_adaptor<0x04e>, &property_getter_adaptor<0x04f>,
  &property_getter_adaptor<0x050>, &property_getter_adaptor<0x051>, &property_getter_adaptor<0x052>, &property_getter_adaptor<0x053>, &property_getter_adaptor<0x054>, &property_getter_adaptor<0x055>, &property_getter_adaptor<0x056>, &property_getter_adaptor<0x057>,
  &property_getter_adaptor<0x058>, &property_getter_adaptor<0x059>, &property_getter_adaptor<0x05a>, &property_getter_adaptor<0x05b>, &property_getter_adaptor<0x05c>, &property_getter_adaptor<0x05d>, &property_getter_adaptor<0x05e>, &property_getter_adaptor<0x05f>,
  &property_getter_adaptor<0x060>, &property_getter_adaptor<0x061>, &property_getter_adaptor<0x062>, &property_getter_adaptor<0x063>, &property_getter_adaptor<0x064>, &property_getter_adaptor<0x065>, &property_getter_adaptor<0x066>, &property_getter_adaptor<0x067>,
  &property_getter_adaptor<0x068>, &property_getter_adaptor<0x069>, &property_getter_adaptor<0x06a>, &property_getter_adaptor<0x06b>, &property_getter_adaptor<0x06c>, &property_getter_adaptor<0x06d>, &property_getter_adaptor<0x06e>, &property_getter_adaptor<0x06f>,
  &property_getter_adaptor<0x070>, &property_getter_adaptor<0x071>, &property_getter_adaptor<0x072>, &property_getter_adaptor<0x073>, &property_getter_adaptor<0x074>, &property_getter_adaptor<0x075>, &property_getter_adaptor<0x076>, &property_getter_adaptor<0x077>,
  &property_getter_adaptor<0x078>, &property_getter_adaptor<0x079>, &property_getter_adaptor<0x07a>, &property_getter_adaptor<0x07b>, &property_getter_adaptor<0x07c>, &property_getter_adaptor<0x07d>, &property_getter_adaptor<0x07e>, &property_getter_adaptor<0x07f>,
  &property_getter_adaptor<0x080>, &property_getter_adaptor<0x081>, &property_getter_adaptor<0x082>, &property_getter_adaptor<0x083>, &property_getter_adaptor<0x084>, &property_getter_adaptor<0x085>, &property_getter_adaptor<0x086>, &property_getter_adaptor<0x087>,
  &property_getter_adaptor<0x088>, &property_getter_adaptor<0x089>, &property_getter_adaptor<0x08a>, &property_getter_adaptor<0x08b>, &property_getter_adaptor<0x08c>, &property_getter_adaptor<0x08d>, &property_getter_adaptor<0x08e>, &property_getter_adaptor<0x08f>,
  &property_getter_adaptor<0x090>, &property_getter_adaptor<0x091>, &property_getter_adaptor<0x092>, &property_getter_adaptor<0x093>, &property_getter_adaptor<0x094>, &property_getter_adaptor<0x095>, &property_getter_adaptor<0x096>, &property_getter_adaptor<0x097>,
  &property_getter_adaptor<0x098>, &property_getter_adaptor<0x099>, &property_getter_adaptor<0x09a>, &property_getter_adaptor<0x09b>, &property_getter_adaptor<0x09c>, &property_getter_adaptor<0x09d>, &property_getter_adaptor<0x09e>, &property_getter_adaptor<0x09f>,
  &property_getter_adaptor<0x0a0>, &property_getter_adaptor<0x0a1>, &property_getter_adaptor<0x0a2>, &property_getter_adaptor<0x0a3>, &property_getter_adaptor<0x0a4>, &property_getter_adaptor<0x0a5>, &property_getter_adaptor<0x0a6>, &property_getter_adaptor<0x0a7>,
  &property_getter_adaptor<0x0a8>, &property_getter_adaptor<0x0a9>, &property_getter_adaptor<0x0aa>, &property_getter_adaptor<0x0ab>, &property_getter_adaptor<0x0ac>, &property_getter_adaptor<0x0ad>, &property_getter_adaptor<0x0ae>, &property_getter_adaptor<0x0af>,
  &property_getter_adaptor<0x0b0>, &property_getter_adaptor<0x0b1>, &property_getter_adaptor<0x0b2>, &property_getter_adaptor<0x0b3>, &property_getter_adaptor<0x0b4>, &property_getter_adaptor<0x0b5>, &property_getter_adaptor<0x0b6>, &property_getter_adaptor<0x0b7>,
  &property_getter_adaptor<0x0b8>, &property_getter_adaptor<0x0b9>, &property_getter_adaptor<0x0ba>, &property_getter_adaptor<0x0bb>, &property_getter_adaptor<0x0bc>, &property_getter_adaptor<0x0bd>, &property_getter_adaptor<0x0be>, &property_getter_adaptor<0x0bf>,
  &property_getter_adaptor<0x0c0>, &property_getter_adaptor<0x0c1>, &property_getter_adaptor<0x0c2>, &property_getter_adaptor<0x0c3>, &property_getter_adaptor<0x0c4>, &property_getter_adaptor<0x0c5>, &property_getter_adaptor<0x0c6>, &property_getter_adaptor<0x0c7>,
  &property_getter_adaptor<0x0c8>, &property_getter_adaptor<0x0c9>, &property_getter_adaptor<0x0ca>, &property_getter_adaptor<0x0cb>, &property_getter_adaptor<0x0cc>, &property_getter_adaptor<0x0cd>, &property_getter_adaptor<0x0ce>, &property_getter_adaptor<0x0cf>,
  &property_getter_adaptor<0x0d0>, &property_getter_adaptor<0x0d1>, &property_getter_adaptor<0x0d2>, &property_getter_adaptor<0x0d3>, &property_getter_adaptor<0x0d4>, &property_getter_adaptor<0x0d5>, &property_getter_adaptor<0x0d6>, &property_getter_adaptor<0x0d7>,
  &property_getter_adaptor<0x0d8>, &property_getter_adaptor<0x0d9>, &property_getter_adaptor<0x0da>, &property_getter_adaptor<0x0db>, &property_getter_adaptor<0x0dc>, &property_getter_adaptor<0x0dd>, &property_getter_adaptor<0x0de>, &property_getter_adaptor<0x0df>,
  &property_getter_adaptor<0x0e0>, &property_getter_adaptor<0x0e1>, &property_getter_adaptor<0x0e2>, &property_getter_adaptor<0x0e3>, &property_getter_adaptor<0x0e4>, &property_getter_adaptor<0x0e5>, &property_getter_adaptor<0x0e6>, &property_getter_adaptor<0x0e7>,
  &property_getter_adaptor<0x0e8>, &property_getter_adaptor<0x0e9>, &property_getter_adaptor<0x0ea>, &property_getter_adaptor<0x0eb>, &property_getter_adaptor<0x0ec>, &property_getter_adaptor<0x0ed>, &property_getter_adaptor<0x0ee>, &property_getter_adaptor<0x0ef>,
  &property_getter_adaptor<0x0f0>, &property_getter_adaptor<0x0f1>, &property_getter_adaptor<0x0f2>, &property_getter_adaptor<0x0f3>, &property_getter_adaptor<0x0f4>, &property_getter_adaptor<0x0f5>, &property_getter_adaptor<0x0f6>, &property_getter_adaptor<0x0f7>,
  &property_getter_adaptor<0x0f8>, &property_getter_adaptor<0x0f9>, &property_getter_adaptor<0x0fa>, &property_getter_adaptor<0x0fb>, &property_getter_adaptor<0x0fc>, &property_getter_adaptor<0x0fd>, &property_getter_adaptor<0x0fe>, &property_getter_adaptor<0x0ff>,
  &property_getter_adaptor<0x100>, &property_getter_adaptor<0x101>, &property_getter_adaptor<0x102>, &property_getter_adaptor<0x103>, &property_getter_adaptor<0x104>, &property_getter_adaptor<0x105>, &property_getter_adaptor<0x106>, &property_getter_adaptor<0x107>,
  &property_getter_adaptor<0x108>, &property_getter_adaptor<0x109>, &property_getter_adaptor<0x10a>, &property_getter_adaptor<0x10b>, &property_getter_adaptor<0x10c>, &property_getter_adaptor<0x10d>, &property_getter_adaptor<0x10e>, &property_getter_adaptor<0x10f>,
  &property_getter_adaptor<0x110>, &property_getter_adaptor<0x111>, &property_getter_adaptor<0x112>, &property_getter_adaptor<0x113>, &property_getter_adaptor<0x114>, &property_getter_adaptor<0x115>, &property_getter_adaptor<0x116>, &property_getter_adaptor<0x117>,
  &property_getter_adaptor<0x118>, &property_getter_adaptor<0x119>, &property_getter_adaptor<0x11a>, &property_getter_adaptor<0x11b>, &property_getter_adaptor<0x11c>, &property_getter_adaptor<0x11d>, &property_getter_adaptor<0x11e>, &property_getter_adaptor<0x11f>,
  &property_getter_adaptor<0x120>, &property_getter_adaptor<0x121>, &property_getter_adaptor<0x122>, &property_getter_adaptor<0x123>, &property_getter_adaptor<0x124>, &property_getter_adaptor<0x125>, &property_getter_adaptor<0x126>, &property_getter_adaptor<0x127>,
  &property_getter_adaptor<0x128>, &property_getter_adaptor<0x129>, &property_getter_adaptor<0x12a>, &property_getter_adaptor<0x12b>, &property_getter_adaptor<0x12c>, &property_getter_adaptor<0x12d>, &property_getter_adaptor<0x12e>, &property_getter_adaptor<0x12f>,
  &property_getter_adaptor<0x130>, &property_getter_adaptor<0x131>, &property_getter_adaptor<0x132>, &property_getter_adaptor<0x133>, &property_getter_adaptor<0x134>, &property_getter_adaptor<0x135>, &property_getter_adaptor<0x136>, &property_getter_adaptor<0x137>,
  &property_getter_adaptor<0x138>, &property_getter_adaptor<0x139>, &property_getter_adaptor<0x13a>, &property_getter_adaptor<0x13b>, &property_getter_adaptor<0x13c>, &property_getter_adaptor<0x13d>, &property_getter_adaptor<0x13e>, &property_getter_adaptor<0x13f>,
  &property_getter_adaptor<0x140>, &property_getter_adaptor<0x141>, &property_getter_adaptor<0x142>, &property_getter_adaptor<0x143>, &property_getter_adaptor<0x144>, &property_getter_adaptor<0x145>, &property_getter_adaptor<0x146>, &property_getter_adaptor<0x147>,
  &property_getter_adaptor<0x148>, &property_getter_adaptor<0x149>, &property_getter_adaptor<0x14a>, &property_getter_adaptor<0x14b>, &property_getter_adaptor<0x14c>, &property_getter_adaptor<0x14d>, &property_getter_adaptor<0x14e>, &property_getter_adaptor<0x14f>,
  &property_getter_adaptor<0x150>, &property_getter_adaptor<0x151>, &property_getter_adaptor<0x152>, &property_getter_adaptor<0x153>, &property_getter_adaptor<0x154>, &property_getter_adaptor<0x155>, &property_getter_adaptor<0x156>, &property_getter_adaptor<0x157>,
  &property_getter_adaptor<0x158>, &property_getter_adaptor<0x159>, &property_getter_adaptor<0x15a>, &property_getter_adaptor<0x15b>, &property_getter_adaptor<0x15c>, &property_getter_adaptor<0x15d>, &property_getter_adaptor<0x15e>, &property_getter_adaptor<0x15f>,
  &property_getter_adaptor<0x160>, &property_getter_adaptor<0x161>, &property_getter_adaptor<0x162>, &property_getter_adaptor<0x163>, &property_getter_adaptor<0x164>, &property_getter_adaptor<0x165>, &property_getter_adaptor<0x166>, &property_getter_adaptor<0x167>,
  &property_getter_adaptor<0x168>, &property_getter_adaptor<0x169>, &property_getter_adaptor<0x16a>, &property_getter_adaptor<0x16b>, &property_getter_adaptor<0x16c>, &property_getter_adaptor<0x16d>, &property_getter_adaptor<0x16e>, &property_getter_adaptor<0x16f>,
  &property_getter_adaptor<0x170>, &property_getter_adaptor<0x171>, &property_getter_adaptor<0x172>, &property_getter_adaptor<0x173>, &property_getter_adaptor<0x174>, &property_getter_adaptor<0x175>, &property_getter_adaptor<0x176>, &property_getter_adaptor<0x177>,
  &property_getter_adaptor<0x178>, &property_getter_adaptor<0x179>, &property_getter_adaptor<0x17a>, &property_getter_adaptor<0x17b>, &property_getter_adaptor<0x17c>, &property_getter_adaptor<0x17d>, &property_getter_adaptor<0x17e>, &property_getter_adaptor<0x17f>,
  &property_getter_adaptor<0x180>, &property_getter_adaptor<0x181>, &property_getter_adaptor<0x182>, &property_getter_adaptor<0x183>, &property_getter_adaptor<0x184>, &property_getter_adaptor<0x185>, &property_getter_adaptor<0x186>, &property_getter_adaptor<0x187>,
  &property_getter_adaptor<0x188>, &property_getter_adaptor<0x189>, &property_getter_adaptor<0x18a>, &property_getter_adaptor<0x18b>, &property_getter_adaptor<0x18c>, &property_getter_adaptor<0x18d>, &property_getter_adaptor<0x18e>, &property_getter_adaptor<0x18f>,
  &property_getter_adaptor<0x190>, &property_getter_adaptor<0x191>, &property_getter_adaptor<0x192>, &property_getter_adaptor<0x193>, &property_getter_adaptor<0x194>, &property_getter_adaptor<0x195>, &property_getter_adaptor<0x196>, &property_getter_adaptor<0x197>,
  &property_getter_adaptor<0x198>, &property_getter_adaptor<0x199>, &property_getter_adaptor<0x19a>, &property_getter_adaptor<0x19b>, &property_getter_adaptor<0x19c>, &property_getter_adaptor<0x19d>, &property_getter_adaptor<0x19e>, &property_getter_adaptor<0x19f>,
  &property_getter_adaptor<0x1a0>, &property_getter_adaptor<0x1a1>, &property_getter_adaptor<0x1a2>, &property_getter_adaptor<0x1a3>, &property_getter_adaptor<0x1a4>, &property_getter_adaptor<0x1a5>, &property_getter_adaptor<0x1a6>, &property_getter_adaptor<0x1a7>,
  &property_getter_adaptor<0x1a8>, &property_getter_adaptor<0x1a9>, &property_getter_adaptor<0x1aa>, &property_getter_adaptor<0x1ab>, &property_getter_adaptor<0x1ac>, &property_getter_adaptor<0x1ad>, &property_getter_adaptor<0x1ae>, &property_getter_adaptor<0x1af>,
  &property_getter_adaptor<0x1b0>, &property_getter_adaptor<0x1b1>, &property_getter_adaptor<0x1b2>, &property_getter_adaptor<0x1b3>, &property_getter_adaptor<0x1b4>, &property_getter_adaptor<0x1b5>, &property_getter_adaptor<0x1b6>, &property_getter_adaptor<0x1b7>,
  &property_getter_adaptor<0x1b8>, &property_getter_adaptor<0x1b9>, &property_getter_adaptor<0x1ba>, &property_getter_adaptor<0x1bb>, &property_getter_adaptor<0x1bc>, &property_getter_adaptor<0x1bd>, &property_getter_adaptor<0x1be>, &property_getter_adaptor<0x1bf>,
  &property_getter_adaptor<0x1c0>, &property_getter_adaptor<0x1c1>, &property_getter_adaptor<0x1c2>, &property_getter_adaptor<0x1c3>, &property_getter_adaptor<0x1c4>, &property_getter_adaptor<0x1c5>, &property_getter_adaptor<0x1c6>, &property_getter_adaptor<0x1c7>,
  &property_getter_adaptor<0x1c8>, &property_getter_adaptor<0x1c9>, &property_getter_adaptor<0x1ca>, &property_getter_adaptor<0x1cb>, &property_getter_adaptor<0x1cc>, &property_getter_adaptor<0x1cd>, &property_getter_adaptor<0x1ce>, &property_getter_adaptor<0x1cf>,
  &property_getter_adaptor<0x1d0>, &property_getter_adaptor<0x1d1>, &property_getter_adaptor<0x1d2>, &property_getter_adaptor<0x1d3>, &property_getter_adaptor<0x1d4>, &property_getter_adaptor<0x1d5>, &property_getter_adaptor<0x1d6>, &property_getter_adaptor<0x1d7>,
  &property_getter_adaptor<0x1d8>, &property_getter_adaptor<0x1d9>, &property_getter_adaptor<0x1da>, &property_getter_adaptor<0x1db>, &property_getter_adaptor<0x1dc>, &property_getter_adaptor<0x1dd>, &property_getter_adaptor<0x1de>, &property_getter_adaptor<0x1df>,
  &property_getter_adaptor<0x1e0>, &property_getter_adaptor<0x1e1>, &property_getter_adaptor<0x1e2>, &property_getter_adaptor<0x1e3>, &property_getter_adaptor<0x1e4>, &property_getter_adaptor<0x1e5>, &property_getter_adaptor<0x1e6>, &property_getter_adaptor<0x1e7>,
  &property_getter_adaptor<0x1e8>, &property_getter_adaptor<0x1e9>, &property_getter_adaptor<0x1ea>, &property_getter_adaptor<0x1eb>, &property_getter_adaptor<0x1ec>, &property_getter_adaptor<0x1ed>, &property_getter_adaptor<0x1ee>, &property_getter_adaptor<0x1ef>,
  &property_getter_adaptor<0x1f0>, &property_getter_adaptor<0x1f1>, &property_getter_adaptor<0x1f2>, &property_getter_adaptor<0x1f3>, &property_getter_adaptor<0x1f4>, &property_getter_adaptor<0x1f5>, &property_getter_adaptor<0x1f6>, &property_getter_adaptor<0x1f7>,
  &property_getter_adaptor<0x1f8>, &property_getter_adaptor<0x1f9>, &property_getter_adaptor<0x1fa>, &property_getter_adaptor<0x1fb>, &property_getter_adaptor<0x1fc>, &property_getter_adaptor<0x1fd>, &property_getter_adaptor<0x1fe>, &property_getter_adaptor<0x1ff>,
  &property_getter_adaptor<0x200>, &property_getter_adaptor<0x201>, &property_getter_adaptor<0x202>, &property_getter_adaptor<0x203>, &property_getter_adaptor<0x204>, &property_getter_adaptor<0x205>, &property_getter_adaptor<0x206>, &property_getter_adaptor<0x207>,
  &property_getter_adaptor<0x208>, &property_getter_adaptor<0x209>, &property_getter_adaptor<0x20a>, &property_getter_adaptor<0x20b>, &property_getter_adaptor<0x20c>, &property_getter_adaptor<0x20d>, &property_getter_adaptor<0x20e>, &property_getter_adaptor<0x20f>,
  &property_getter_adaptor<0x210>, &property_getter_adaptor<0x211>, &property_getter_adaptor<0x212>, &property_getter_adaptor<0x213>, &property_getter_adaptor<0x214>, &property_getter_adaptor<0x215>, &property_getter_adaptor<0x216>, &property_getter_adaptor<0x217>,
  &property_getter_adaptor<0x218>, &property_getter_adaptor<0x219>, &property_getter_adaptor<0x21a>, &property_getter_adaptor<0x21b>, &property_getter_adaptor<0x21c>, &property_getter_adaptor<0x21d>, &property_getter_adaptor<0x21e>, &property_getter_adaptor<0x21f>,
  &property_getter_adaptor<0x220>, &property_getter_adaptor<0x221>, &property_getter_adaptor<0x222>, &property_getter_adaptor<0x223>, &property_getter_adaptor<0x224>, &property_getter_adaptor<0x225>, &property_getter_adaptor<0x226>, &property_getter_adaptor<0x227>,
  &property_getter_adaptor<0x228>, &property_getter_adaptor<0x229>, &property_getter_adaptor<0x22a>, &property_getter_adaptor<0x22b>, &property_getter_adaptor<0x22c>, &property_getter_adaptor<0x22d>, &property_getter_adaptor<0x22e>, &property_getter_adaptor<0x22f>,
  &property_getter_adaptor<0x230>, &property_getter_adaptor<0x231>, &property_getter_adaptor<0x232>, &property_getter_adaptor<0x233>, &property_getter_adaptor<0x234>, &property_getter_adaptor<0x235>, &property_getter_adaptor<0x236>, &property_getter_adaptor<0x237>,
  &property_getter_adaptor<0x238>, &property_getter_adaptor<0x239>, &property_getter_adaptor<0x23a>, &property_getter_adaptor<0x23b>, &property_getter_adaptor<0x23c>, &property_getter_adaptor<0x23d>, &property_getter_adaptor<0x23e>, &property_getter_adaptor<0x23f>,
  &property_getter_adaptor<0x240>, &property_getter_adaptor<0x241>, &property_getter_adaptor<0x242>, &property_getter_adaptor<0x243>, &property_getter_adaptor<0x244>, &property_getter_adaptor<0x245>, &property_getter_adaptor<0x246>, &property_getter_adaptor<0x247>,
  &property_getter_adaptor<0x248>, &property_getter_adaptor<0x249>, &property_getter_adaptor<0x24a>, &property_getter_adaptor<0x24b>, &property_getter_adaptor<0x24c>, &property_getter_adaptor<0x24d>, &property_getter_adaptor<0x24e>, &property_getter_adaptor<0x24f>,
  &property_getter_adaptor<0x250>, &property_getter_adaptor<0x251>, &property_getter_adaptor<0x252>, &property_getter_adaptor<0x253>, &property_getter_adaptor<0x254>, &property_getter_adaptor<0x255>, &property_getter_adaptor<0x256>, &property_getter_adaptor<0x257>,
  &property_getter_adaptor<0x258>, &property_getter_adaptor<0x259>, &property_getter_adaptor<0x25a>, &property_getter_adaptor<0x25b>, &property_getter_adaptor<0x25c>, &property_getter_adaptor<0x25d>, &property_getter_adaptor<0x25e>, &property_getter_adaptor<0x25f>,
  &property_getter_adaptor<0x260>, &property_getter_adaptor<0x261>, &property_getter_adaptor<0x262>, &property_getter_adaptor<0x263>, &property_getter_adaptor<0x264>, &property_getter_adaptor<0x265>, &property_getter_adaptor<0x266>, &property_getter_adaptor<0x267>,
  &property_getter_adaptor<0x268>, &property_getter_adaptor<0x269>, &property_getter_adaptor<0x26a>, &property_getter_adaptor<0x26b>, &property_getter_adaptor<0x26c>, &property_getter_adaptor<0x26d>, &property_getter_adaptor<0x26e>, &property_getter_adaptor<0x26f>,
  &property_getter_adaptor<0x270>, &property_getter_adaptor<0x271>, &property_getter_adaptor<0x272>, &property_getter_adaptor<0x273>, &property_getter_adaptor<0x274>, &property_getter_adaptor<0x275>, &property_getter_adaptor<0x276>, &property_getter_adaptor<0x277>,
  &property_getter_adaptor<0x278>, &property_getter_adaptor<0x279>, &property_getter_adaptor<0x27a>, &property_getter_adaptor<0x27b>, &property_getter_adaptor<0x27c>, &property_getter_adaptor<0x27d>, &property_getter_adaptor<0x27e>, &property_getter_adaptor<0x27f>,
  &property_getter_adaptor<0x280>, &property_getter_adaptor<0x281>, &property_getter_adaptor<0x282>, &property_getter_adaptor<0x283>, &property_getter_adaptor<0x284>, &property_getter_adaptor<0x285>, &property_getter_adaptor<0x286>, &property_getter_adaptor<0x287>,
  &property_getter_adaptor<0x288>, &property_getter_adaptor<0x289>, &property_getter_adaptor<0x28a>, &property_getter_adaptor<0x28b>, &property_getter_adaptor<0x28c>, &property_getter_adaptor<0x28d>, &property_getter_adaptor<0x28e>, &property_getter_adaptor<0x28f>,
  &property_getter_adaptor<0x290>, &property_getter_adaptor<0x291>, &property_getter_adaptor<0x292>, &property_getter_adaptor<0x293>, &property_getter_adaptor<0x294>, &property_getter_adaptor<0x295>, &property_getter_adaptor<0x296>, &property_getter_adaptor<0x297>,
  &property_getter_adaptor<0x298>, &property_getter_adaptor<0x299>, &property_getter_adaptor<0x29a>, &property_getter_adaptor<0x29b>, &property_getter_adaptor<0x29c>, &property_getter_adaptor<0x29d>, &property_getter_adaptor<0x29e>, &property_getter_adaptor<0x29f>,
  &property_getter_adaptor<0x2a0>, &property_getter_adaptor<0x2a1>, &property_getter_adaptor<0x2a2>, &property_getter_adaptor<0x2a3>, &property_getter_adaptor<0x2a4>, &property_getter_adaptor<0x2a5>, &property_getter_adaptor<0x2a6>, &property_getter_adaptor<0x2a7>,
  &property_getter_adaptor<0x2a8>, &property_getter_adaptor<0x2a9>, &property_getter_adaptor<0x2aa>, &property_getter_adaptor<0x2ab>, &property_getter_adaptor<0x2ac>, &property_getter_adaptor<0x2ad>, &property_getter_adaptor<0x2ae>, &property_getter_adaptor<0x2af>,
  &property_getter_adaptor<0x2b0>, &property_getter_adaptor<0x2b1>, &property_getter_adaptor<0x2b2>, &property_getter_adaptor<0x2b3>, &property_getter_adaptor<0x2b4>, &property_getter_adaptor<0x2b5>, &property_getter_adaptor<0x2b6>, &property_getter_adaptor<0x2b7>,
  &property_getter_adaptor<0x2b8>, &property_getter_adaptor<0x2b9>, &property_getter_adaptor<0x2ba>, &property_getter_adaptor<0x2bb>, &property_getter_adaptor<0x2bc>, &property_getter_adaptor<0x2bd>, &property_getter_adaptor<0x2be>, &property_getter_adaptor<0x2bf>,
  &property_getter_adaptor<0x2c0>, &property_getter_adaptor<0x2c1>, &property_getter_adaptor<0x2c2>, &property_getter_adaptor<0x2c3>, &property_getter_adaptor<0x2c4>, &property_getter_adaptor<0x2c5>, &property_getter_adaptor<0x2c6>, &property_getter_adaptor<0x2c7>,
  &property_getter_adaptor<0x2c8>, &property_getter_adaptor<0x2c9>, &property_getter_adaptor<0x2ca>, &property_getter_adaptor<0x2cb>, &property_getter_adaptor<0x2cc>, &property_getter_adaptor<0x2cd>, &property_getter_adaptor<0x2ce>, &property_getter_adaptor<0x2cf>,
  &property_getter_adaptor<0x2d0>, &property_getter_adaptor<0x2d1>, &property_getter_adaptor<0x2d2>, &property_getter_adaptor<0x2d3>, &property_getter_adaptor<0x2d4>, &property_getter_adaptor<0x2d5>, &property_getter_adaptor<0x2d6>, &property_getter_adaptor<0x2d7>,
  &property_getter_adaptor<0x2d8>, &property_getter_adaptor<0x2d9>, &property_getter_adaptor<0x2da>, &property_getter_adaptor<0x2db>, &property_getter_adaptor<0x2dc>, &property_getter_adaptor<0x2dd>, &property_getter_adaptor<0x2de>, &property_getter_adaptor<0x2df>,
  &property_getter_adaptor<0x2e0>, &property_getter_adaptor<0x2e1>, &property_getter_adaptor<0x2e2>, &property_getter_adaptor<0x2e3>, &property_getter_adaptor<0x2e4>, &property_getter_adaptor<0x2e5>, &property_getter_adaptor<0x2e6>, &property_getter_adaptor<0x2e7>,
  &property_getter_adaptor<0x2e8>, &property_getter_adaptor<0x2e9>, &property_getter_adaptor<0x2ea>, &property_getter_adaptor<0x2eb>, &property_getter_adaptor<0x2ec>, &property_getter_adaptor<0x2ed>, &property_getter_adaptor<0x2ee>, &property_getter_adaptor<0x2ef>,
  &property_getter_adaptor<0x2f0>, &property_getter_adaptor<0x2f1>, &property_getter_adaptor<0x2f2>, &property_getter_adaptor<0x2f3>, &property_getter_adaptor<0x2f4>, &property_getter_adaptor<0x2f5>, &property_getter_adaptor<0x2f6>, &property_getter_adaptor<0x2f7>,
  &property_getter_adaptor<0x2f8>, &property_getter_adaptor<0x2f9>, &property_getter_adaptor<0x2fa>, &property_getter_adaptor<0x2fb>, &property_getter_adaptor<0x2fc>, &property_getter_adaptor<0x2fd>, &property_getter_adaptor<0x2fe>, &property_getter_adaptor<0x2ff>,
  &property_getter_adaptor<0x300>, &property_getter_adaptor<0x301>, &property_getter_adaptor<0x302>, &property_getter_adaptor<0x303>, &property_getter_adaptor<0x304>, &property_getter_adaptor<0x305>, &property_getter_adaptor<0x306>, &property_getter_adaptor<0x307>,
  &property_getter_adaptor<0x308>, &property_getter_adaptor<0x309>, &property_getter_adaptor<0x30a>, &property_getter_adaptor<0x30b>, &property_getter_adaptor<0x30c>, &property_getter_adaptor<0x30d>, &property_getter_adaptor<0x30e>, &property_getter_adaptor<0x30f>,
  &property_getter_adaptor<0x310>, &property_getter_adaptor<0x311>, &property_getter_adaptor<0x312>, &property_getter_adaptor<0x313>, &property_getter_adaptor<0x314>, &property_getter_adaptor<0x315>, &property_getter_adaptor<0x316>, &property_getter_adaptor<0x317>,
  &property_getter_adaptor<0x318>, &property_getter_adaptor<0x319>, &property_getter_adaptor<0x31a>, &property_getter_adaptor<0x31b>, &property_getter_adaptor<0x31c>, &property_getter_adaptor<0x31d>, &property_getter_adaptor<0x31e>, &property_getter_adaptor<0x31f>,
  &property_getter_adaptor<0x320>, &property_getter_adaptor<0x321>, &property_getter_adaptor<0x322>, &property_getter_adaptor<0x323>, &property_getter_adaptor<0x324>, &property_getter_adaptor<0x325>, &property_getter_adaptor<0x326>, &property_getter_adaptor<0x327>,
  &property_getter_adaptor<0x328>, &property_getter_adaptor<0x329>, &property_getter_adaptor<0x32a>, &property_getter_adaptor<0x32b>, &property_getter_adaptor<0x32c>, &property_getter_adaptor<0x32d>, &property_getter_adaptor<0x32e>, &property_getter_adaptor<0x32f>,
  &property_getter_adaptor<0x330>, &property_getter_adaptor<0x331>, &property_getter_adaptor<0x332>, &property_getter_adaptor<0x333>, &property_getter_adaptor<0x334>, &property_getter_adaptor<0x335>, &property_getter_adaptor<0x336>, &property_getter_adaptor<0x337>,
  &property_getter_adaptor<0x338>, &property_getter_adaptor<0x339>, &property_getter_adaptor<0x33a>, &property_getter_adaptor<0x33b>, &property_getter_adaptor<0x33c>, &property_getter_adaptor<0x33d>, &property_getter_adaptor<0x33e>, &property_getter_adaptor<0x33f>,
  &property_getter_adaptor<0x340>, &property_getter_adaptor<0x341>, &property_getter_adaptor<0x342>, &property_getter_adaptor<0x343>, &property_getter_adaptor<0x344>, &property_getter_adaptor<0x345>, &property_getter_adaptor<0x346>, &property_getter_adaptor<0x347>,
  &property_getter_adaptor<0x348>, &property_getter_adaptor<0x349>, &property_getter_adaptor<0x34a>, &property_getter_adaptor<0x34b>, &property_getter_adaptor<0x34c>, &property_getter_adaptor<0x34d>, &property_getter_adaptor<0x34e>, &property_getter_adaptor<0x34f>,
  &property_getter_adaptor<0x350>, &property_getter_adaptor<0x351>, &property_getter_adaptor<0x352>, &property_getter_adaptor<0x353>, &property_getter_adaptor<0x354>, &property_getter_adaptor<0x355>, &property_getter_adaptor<0x356>, &property_getter_adaptor<0x357>,
  &property_getter_adaptor<0x358>, &property_getter_adaptor<0x359>, &property_getter_adaptor<0x35a>, &property_getter_adaptor<0x35b>, &property_getter_adaptor<0x35c>, &property_getter_adaptor<0x35d>, &property_getter_adaptor<0x35e>, &property_getter_adaptor<0x35f>,
  &property_getter_adaptor<0x360>, &property_getter_adaptor<0x361>, &property_getter_adaptor<0x362>, &property_getter_adaptor<0x363>, &property_getter_adaptor<0x364>, &property_getter_adaptor<0x365>, &property_getter_adaptor<0x366>, &property_getter_adaptor<0x367>,
  &property_getter_adaptor<0x368>, &property_getter_adaptor<0x369>, &property_getter_adaptor<0x36a>, &property_getter_adaptor<0x36b>, &property_getter_adaptor<0x36c>, &property_getter_adaptor<0x36d>, &property_getter_adaptor<0x36e>, &property_getter_adaptor<0x36f>,
  &property_getter_adaptor<0x370>, &property_getter_adaptor<0x371>, &property_getter_adaptor<0x372>, &property_getter_adaptor<0x373>, &property_getter_adaptor<0x374>, &property_getter_adaptor<0x375>, &property_getter_adaptor<0x376>, &property_getter_adaptor<0x377>,
  &property_getter_adaptor<0x378>, &property_getter_adaptor<0x379>, &property_getter_adaptor<0x37a>, &property_getter_adaptor<0x37b>, &property_getter_adaptor<0x37c>, &property_getter_adaptor<0x37d>, &property_getter_adaptor<0x37e>, &property_getter_adaptor<0x37f>,
  &property_getter_adaptor<0x380>, &property_getter_adaptor<0x381>, &property_getter_adaptor<0x382>, &property_getter_adaptor<0x383>, &property_getter_adaptor<0x384>, &property_getter_adaptor<0x385>, &property_getter_adaptor<0x386>, &property_getter_adaptor<0x387>,
  &property_getter_adaptor<0x388>, &property_getter_adaptor<0x389>, &property_getter_adaptor<0x38a>, &property_getter_adaptor<0x38b>, &property_getter_adaptor<0x38c>, &property_getter_adaptor<0x38d>, &property_getter_adaptor<0x38e>, &property_getter_adaptor<0x38f>,
  &property_getter_adaptor<0x390>, &property_getter_adaptor<0x391>, &property_getter_adaptor<0x392>, &property_getter_adaptor<0x393>, &property_getter_adaptor<0x394>, &property_getter_adaptor<0x395>, &property_getter_adaptor<0x396>, &property_getter_adaptor<0x397>,
  &property_getter_adaptor<0x398>, &property_getter_adaptor<0x399>, &property_getter_adaptor<0x39a>, &property_getter_adaptor<0x39b>, &property_getter_adaptor<0x39c>, &property_getter_adaptor<0x39d>, &property_getter_adaptor<0x39e>, &property_getter_adaptor<0x39f>,
  &property_getter_adaptor<0x3a0>, &property_getter_adaptor<0x3a1>, &property_getter_adaptor<0x3a2>, &property_getter_adaptor<0x3a3>, &property_getter_adaptor<0x3a4>, &property_getter_adaptor<0x3a5>, &property_getter_adaptor<0x3a6>, &property_getter_adaptor<0x3a7>,
  &property_getter_adaptor<0x3a8>, &property_getter_adaptor<0x3a9>, &property_getter_adaptor<0x3aa>, &property_getter_adaptor<0x3ab>, &property_getter_adaptor<0x3ac>, &property_getter_adaptor<0x3ad>, &property_getter_adaptor<0x3ae>, &property_getter_adaptor<0x3af>,
  &property_getter_adaptor<0x3b0>, &property_getter_adaptor<0x3b1>, &property_getter_adaptor<0x3b2>, &property_getter_adaptor<0x3b3>, &property_getter_adaptor<0x3b4>, &property_getter_adaptor<0x3b5>, &property_getter_adaptor<0x3b6>, &property_getter_adaptor<0x3b7>,
  &property_getter_adaptor<0x3b8>, &property_getter_adaptor<0x3b9>, &property_getter_adaptor<0x3ba>, &property_getter_adaptor<0x3bb>, &property_getter_adaptor<0x3bc>, &property_getter_adaptor<0x3bd>, &property_getter_adaptor<0x3be>, &property_getter_adaptor<0x3bf>,
  &property_getter_adaptor<0x3c0>, &property_getter_adaptor<0x3c1>, &property_getter_adaptor<0x3c2>, &property_getter_adaptor<0x3c3>, &property_getter_adaptor<0x3c4>, &property_getter_adaptor<0x3c5>, &property_getter_adaptor<0x3c6>, &property_getter_adaptor<0x3c7>,
  &property_getter_adaptor<0x3c8>, &property_getter_adaptor<0x3c9>, &property_getter_adaptor<0x3ca>, &property_getter_adaptor<0x3cb>, &property_getter_adaptor<0x3cc>, &property_getter_adaptor<0x3cd>, &property_getter_adaptor<0x3ce>, &property_getter_adaptor<0x3cf>,
  &property_getter_adaptor<0x3d0>, &property_getter_adaptor<0x3d1>, &property_getter_adaptor<0x3d2>, &property_getter_adaptor<0x3d3>, &property_getter_adaptor<0x3d4>, &property_getter_adaptor<0x3d5>, &property_getter_adaptor<0x3d6>, &property_getter_adaptor<0x3d7>,
  &property_getter_adaptor<0x3d8>, &property_getter_adaptor<0x3d9>, &property_getter_adaptor<0x3da>, &property_getter_adaptor<0x3db>, &property_getter_adaptor<0x3dc>, &property_getter_adaptor<0x3dd>, &property_getter_adaptor<0x3de>, &property_getter_adaptor<0x3df>,
  &property_getter_adaptor<0x3e0>, &property_getter_adaptor<0x3e1>, &property_getter_adaptor<0x3e2>, &property_getter_adaptor<0x3e3>, &property_getter_adaptor<0x3e4>, &property_getter_adaptor<0x3e5>, &property_getter_adaptor<0x3e6>, &property_getter_adaptor<0x3e7>,
  &property_getter_adaptor<0x3e8>, &property_getter_adaptor<0x3e9>, &property_getter_adaptor<0x3ea>, &property_getter_adaptor<0x3eb>, &property_getter_adaptor<0x3ec>, &property_getter_adaptor<0x3ed>, &property_getter_adaptor<0x3ee>, &property_getter_adaptor<0x3ef>,
  &property_getter_adaptor<0x3f0>, &property_getter_adaptor<0x3f1>, &property_getter_adaptor<0x3f2>, &property_getter_adaptor<0x3f3>, &property_getter_adaptor<0x3f4>, &property_getter_adaptor<0x3f5>, &property_getter_adaptor<0x3f6>, &property_getter_adaptor<0x3f7>,
  &property_getter_adaptor<0x3f8>, &property_getter_adaptor<0x3f9>, &property_getter_adaptor<0x3fa>, &property_getter_adaptor<0x3fb>, &property_getter_adaptor<0x3fc>, &property_getter_adaptor<0x3fd>, &property_getter_adaptor<0x3fe>, &property_getter_adaptor<0x3ff>,
  &property_getter_adaptor<0x400>, &property_getter_adaptor<0x401>, &property_getter_adaptor<0x402>, &property_getter_adaptor<0x403>, &property_getter_adaptor<0x404>, &property_getter_adaptor<0x405>, &property_getter_adaptor<0x406>, &property_getter_adaptor<0x407>,
  &property_getter_adaptor<0x408>, &property_getter_adaptor<0x409>, &property_getter_adaptor<0x40a>, &property_getter_adaptor<0x40b>, &property_getter_adaptor<0x40c>, &property_getter_adaptor<0x40d>, &property_getter_adaptor<0x40e>, &property_getter_adaptor<0x40f>,
  &property_getter_adaptor<0x410>, &property_getter_adaptor<0x411>, &property_getter_adaptor<0x412>, &property_getter_adaptor<0x413>, &property_getter_adaptor<0x414>, &property_getter_adaptor<0x415>, &property_getter_adaptor<0x416>, &property_getter_adaptor<0x417>,
  &property_getter_adaptor<0x418>, &property_getter_adaptor<0x419>, &property_getter_adaptor<0x41a>, &property_getter_adaptor<0x41b>, &property_getter_adaptor<0x41c>, &property_getter_adaptor<0x41d>, &property_getter_adaptor<0x41e>, &property_getter_adaptor<0x41f>,
  &property_getter_adaptor<0x420>, &property_getter_adaptor<0x421>, &property_getter_adaptor<0x422>, &property_getter_adaptor<0x423>, &property_getter_adaptor<0x424>, &property_getter_adaptor<0x425>, &property_getter_adaptor<0x426>, &property_getter_adaptor<0x427>,
  &property_getter_adaptor<0x428>, &property_getter_adaptor<0x429>, &property_getter_adaptor<0x42a>, &property_getter_adaptor<0x42b>, &property_getter_adaptor<0x42c>, &property_getter_adaptor<0x42d>, &property_getter_adaptor<0x42e>, &property_getter_adaptor<0x42f>,
  &property_getter_adaptor<0x430>, &property_getter_adaptor<0x431>, &property_getter_adaptor<0x432>, &property_getter_adaptor<0x433>, &property_getter_adaptor<0x434>, &property_getter_adaptor<0x435>, &property_getter_adaptor<0x436>, &property_getter_adaptor<0x437>,
  &property_getter_adaptor<0x438>, &property_getter_adaptor<0x439>, &property_getter_adaptor<0x43a>, &property_getter_adaptor<0x43b>, &property_getter_adaptor<0x43c>, &property_getter_adaptor<0x43d>, &property_getter_adaptor<0x43e>, &property_getter_adaptor<0x43f>,
  &property_getter_adaptor<0x440>, &property_getter_adaptor<0x441>, &property_getter_adaptor<0x442>, &property_getter_adaptor<0x443>, &property_getter_adaptor<0x444>, &property_getter_adaptor<0x445>, &property_getter_adaptor<0x446>, &property_getter_adaptor<0x447>,
  &property_getter_adaptor<0x448>, &property_getter_adaptor<0x449>, &property_getter_adaptor<0x44a>, &property_getter_adaptor<0x44b>, &property_getter_adaptor<0x44c>, &property_getter_adaptor<0x44d>, &property_getter_adaptor<0x44e>, &property_getter_adaptor<0x44f>,
  &property_getter_adaptor<0x450>, &property_getter_adaptor<0x451>, &property_getter_adaptor<0x452>, &property_getter_adaptor<0x453>, &property_getter_adaptor<0x454>, &property_getter_adaptor<0x455>, &property_getter_adaptor<0x456>, &property_getter_adaptor<0x457>,
  &property_getter_adaptor<0x458>, &property_getter_adaptor<0x459>, &property_getter_adaptor<0x45a>, &property_getter_adaptor<0x45b>, &property_getter_adaptor<0x45c>, &property_getter_adaptor<0x45d>, &property_getter_adaptor<0x45e>, &property_getter_adaptor<0x45f>,
  &property_getter_adaptor<0x460>, &property_getter_adaptor<0x461>, &property_getter_adaptor<0x462>, &property_getter_adaptor<0x463>, &property_getter_adaptor<0x464>, &property_getter_adaptor<0x465>, &property_getter_adaptor<0x466>, &property_getter_adaptor<0x467>,
  &property_getter_adaptor<0x468>, &property_getter_adaptor<0x469>, &property_getter_adaptor<0x46a>, &property_getter_adaptor<0x46b>, &property_getter_adaptor<0x46c>, &property_getter_adaptor<0x46d>, &property_getter_adaptor<0x46e>, &property_getter_adaptor<0x46f>,
  &property_getter_adaptor<0x470>, &property_getter_adaptor<0x471>, &property_getter_adaptor<0x472>, &property_getter_adaptor<0x473>, &property_getter_adaptor<0x474>, &property_getter_adaptor<0x475>, &property_getter_adaptor<0x476>, &property_getter_adaptor<0x477>,
  &property_getter_adaptor<0x478>, &property_getter_adaptor<0x479>, &property_getter_adaptor<0x47a>, &property_getter_adaptor<0x47b>, &property_getter_adaptor<0x47c>, &property_getter_adaptor<0x47d>, &property_getter_adaptor<0x47e>, &property_getter_adaptor<0x47f>,
  &property_getter_adaptor<0x480>, &property_getter_adaptor<0x481>, &property_getter_adaptor<0x482>, &property_getter_adaptor<0x483>, &property_getter_adaptor<0x484>, &property_getter_adaptor<0x485>, &property_getter_adaptor<0x486>, &property_getter_adaptor<0x487>,
  &property_getter_adaptor<0x488>, &property_getter_adaptor<0x489>, &property_getter_adaptor<0x48a>, &property_getter_adaptor<0x48b>, &property_getter_adaptor<0x48c>, &property_getter_adaptor<0x48d>, &property_getter_adaptor<0x48e>, &property_getter_adaptor<0x48f>,
  &property_getter_adaptor<0x490>, &property_getter_adaptor<0x491>, &property_getter_adaptor<0x492>, &property_getter_adaptor<0x493>, &property_getter_adaptor<0x494>, &property_getter_adaptor<0x495>, &property_getter_adaptor<0x496>, &property_getter_adaptor<0x497>,
  &property_getter_adaptor<0x498>, &property_getter_adaptor<0x499>, &property_getter_adaptor<0x49a>, &property_getter_adaptor<0x49b>, &property_getter_adaptor<0x49c>, &property_getter_adaptor<0x49d>, &property_getter_adaptor<0x49e>, &property_getter_adaptor<0x49f>,
  &property_getter_adaptor<0x4a0>, &property_getter_adaptor<0x4a1>, &property_getter_adaptor<0x4a2>, &property_getter_adaptor<0x4a3>, &property_getter_adaptor<0x4a4>, &property_getter_adaptor<0x4a5>, &property_getter_adaptor<0x4a6>, &property_getter_adaptor<0x4a7>,
  &property_getter_adaptor<0x4a8>, &property_getter_adaptor<0x4a9>, &property_getter_adaptor<0x4aa>, &property_getter_adaptor<0x4ab>, &property_getter_adaptor<0x4ac>, &property_getter_adaptor<0x4ad>, &property_getter_adaptor<0x4ae>, &property_getter_adaptor<0x4af>,
  &property_getter_adaptor<0x4b0>, &property_getter_adaptor<0x4b1>, &property_getter_adaptor<0x4b2>, &property_getter_adaptor<0x4b3>, &property_getter_adaptor<0x4b4>, &property_getter_adaptor<0x4b5>, &property_getter_adaptor<0x4b6>, &property_getter_adaptor<0x4b7>,
  &property_getter_adaptor<0x4b8>, &property_getter_adaptor<0x4b9>, &property_getter_adaptor<0x4ba>, &property_getter_adaptor<0x4bb>, &property_getter_adaptor<0x4bc>, &property_getter_adaptor<0x4bd>, &property_getter_adaptor<0x4be>, &property_getter_adaptor<0x4bf>,
  &property_getter_adaptor<0x4c0>, &property_getter_adaptor<0x4c1>, &property_getter_adaptor<0x4c2>, &property_getter_adaptor<0x4c3>, &property_getter_adaptor<0x4c4>, &property_getter_adaptor<0x4c5>, &property_getter_adaptor<0x4c6>, &property_getter_adaptor<0x4c7>,
  &property_getter_adaptor<0x4c8>, &property_getter_adaptor<0x4c9>, &property_getter_adaptor<0x4ca>, &property_getter_adaptor<0x4cb>, &property_getter_adaptor<0x4cc>, &property_getter_adaptor<0x4cd>, &property_getter_adaptor<0x4ce>, &property_getter_adaptor<0x4cf>,
  &property_getter_adaptor<0x4d0>, &property_getter_adaptor<0x4d1>, &property_getter_adaptor<0x4d2>, &property_getter_adaptor<0x4d3>, &property_getter_adaptor<0x4d4>, &property_getter_adaptor<0x4d5>, &property_getter_adaptor<0x4d6>, &property_getter_adaptor<0x4d7>,
  &property_getter_adaptor<0x4d8>, &property_getter_adaptor<0x4d9>, &property_getter_adaptor<0x4da>, &property_getter_adaptor<0x4db>, &property_getter_adaptor<0x4dc>, &property_getter_adaptor<0x4dd>, &property_getter_adaptor<0x4de>, &property_getter_adaptor<0x4df>,
  &property_getter_adaptor<0x4e0>, &property_getter_adaptor<0x4e1>, &property_getter_adaptor<0x4e2>, &property_getter_adaptor<0x4e3>, &property_getter_adaptor<0x4e4>, &property_getter_adaptor<0x4e5>, &property_getter_adaptor<0x4e6>, &property_getter_adaptor<0x4e7>,
  &property_getter_adaptor<0x4e8>, &property_getter_adaptor<0x4e9>, &property_getter_adaptor<0x4ea>, &property_getter_adaptor<0x4eb>, &property_getter_adaptor<0x4ec>, &property_getter_adaptor<0x4ed>, &property_getter_adaptor<0x4ee>, &property_getter_adaptor<0x4ef>,
  &property_getter_adaptor<0x4f0>, &property_getter_adaptor<0x4f1>, &property_getter_adaptor<0x4f2>, &property_getter_adaptor<0x4f3>, &property_getter_adaptor<0x4f4>, &property_getter_adaptor<0x4f5>, &property_getter_adaptor<0x4f6>, &property_getter_adaptor<0x4f7>,
  &property_getter_adaptor<0x4f8>, &property_getter_adaptor<0x4f9>, &property_getter_adaptor<0x4fa>, &property_getter_adaptor<0x4fb>, &property_getter_adaptor<0x4fc>, &property_getter_adaptor<0x4fd>, &property_getter_adaptor<0x4fe>, &property_getter_adaptor<0x4ff>,
};

py_func_ptr_t get_property_getter_adaptor (int n)
{
  tl_assert (n >= 0 && n < int (sizeof (property_getter_adaptors) / sizeof (property_getter_adaptors [0])));
  return property_getter_adaptors [n];
}

template <int N>
PyObject *property_setter_adaptor (PyObject *self, PyObject *args)
{
  return property_setter_adaptor (N, self, args);
}

static py_func_ptr_t property_setter_adaptors [] =
{
  &property_setter_adaptor<0x000>, &property_setter_adaptor<0x001>, &property_setter_adaptor<0x002>, &property_setter_adaptor<0x003>, &property_setter_adaptor<0x004>, &property_setter_adaptor<0x005>, &property_setter_adaptor<0x006>, &property_setter_adaptor<0x007>,
  &property_setter_adaptor<0x008>, &property_setter_adaptor<0x009>, &property_setter_adaptor<0x00a>, &property_setter_adaptor<0x00b>, &property_setter_adaptor<0x00c>, &property_setter_adaptor<0x00d>, &property_setter_adaptor<0x00e>, &property_setter_adaptor<0x00f>,
  &property_setter_adaptor<0x010>, &property_setter_adaptor<0x011>, &property_setter_adaptor<0x012>, &property_setter_adaptor<0x013>, &property_setter_adaptor<0x014>, &property_setter_adaptor<0x015>, &property_setter_adaptor<0x016>, &property_setter_adaptor<0x017>,
  &property_setter_adaptor<0x018>, &property_setter_adaptor<0x019>, &property_setter_adaptor<0x01a>, &property_setter_adaptor<0x01b>, &property_setter_adaptor<0x01c>, &property_setter_adaptor<0x01d>, &property_setter_adaptor<0x01e>, &property_setter_adaptor<0x01f>,
  &property_setter_adaptor<0x020>, &property_setter_adaptor<0x021>, &property_setter_adaptor<0x022>, &property_setter_adaptor<0x023>, &property_setter_adaptor<0x024>, &property_setter_adaptor<0x025>, &property_setter_adaptor<0x026>, &property_setter_adaptor<0x027>,
  &property_setter_adaptor<0x028>, &property_setter_adaptor<0x029>, &property_setter_adaptor<0x02a>, &property_setter_adaptor<0x02b>, &property_setter_adaptor<0x02c>, &property_setter_adaptor<0x02d>, &property_setter_adaptor<0x02e>, &property_setter_adaptor<0x02f>,
  &property_setter_adaptor<0x030>, &property_setter_adaptor<0x031>, &property_setter_adaptor<0x032>, &property_setter_adaptor<0x033>, &property_setter_adaptor<0x034>, &property_setter_adaptor<0x035>, &property_setter_adaptor<0x036>, &property_setter_adaptor<0x037>,
  &property_setter_adaptor<0x038>, &property_setter_adaptor<0x039>, &property_setter_adaptor<0x03a>, &property_setter_adaptor<0x03b>, &property_setter_adaptor<0x03c>, &property_setter_adaptor<0x03d>, &property_setter_adaptor<0x03e>, &property_setter_adaptor<0x03f>,
  &property_setter_adaptor<0x040>, &property_setter_adaptor<0x041>, &property_setter_adaptor<0x042>, &property_setter_adaptor<0x043>, &property_setter_adaptor<0x044>, &property_setter_adaptor<0x045>, &property_setter_adaptor<0x046>, &property_setter_adaptor<0x047>,
  &property_setter_adaptor<0x048>, &property_setter_adaptor<0x049>, &property_setter_adaptor<0x04a>, &property_setter_adaptor<0x04b>, &property_setter_adaptor<0x04c>, &property_setter_adaptor<0x04d>, &property_setter_adaptor<0x04e>, &property_setter_adaptor<0x04f>,
  &property_setter_adaptor<0x050>, &property_setter_adaptor<0x051>, &property_setter_adaptor<0x052>, &property_setter_adaptor<0x053>, &property_setter_adaptor<0x054>, &property_setter_adaptor<0x055>, &property_setter_adaptor<0x056>, &property_setter_adaptor<0x057>,
  &property_setter_adaptor<0x058>, &property_setter_adaptor<0x059>, &property_setter_adaptor<0x05a>, &property_setter_adaptor<0x05b>, &property_setter_adaptor<0x05c>, &property_setter_adaptor<0x05d>, &property_setter_adaptor<0x05e>, &property_setter_adaptor<0x05f>,
  &property_setter_adaptor<0x060>, &property_setter_adaptor<0x061>, &property_setter_adaptor<0x062>, &property_setter_adaptor<0x063>, &property_setter_adaptor<0x064>, &property_setter_adaptor<0x065>, &property_setter_adaptor<0x066>, &property_setter_adaptor<0x067>,
  &property_setter_adaptor<0x068>, &property_setter_adaptor<0x069>, &property_setter_adaptor<0x06a>, &property_setter_adaptor<0x06b>, &property_setter_adaptor<0x06c>, &property_setter_adaptor<0x06d>, &property_setter_adaptor<0x06e>, &property_setter_adaptor<0x06f>,
  &property_setter_adaptor<0x070>, &property_setter_adaptor<0x071>, &property_setter_adaptor<0x072>, &property_setter_adaptor<0x073>, &property_setter_adaptor<0x074>, &property_setter_adaptor<0x075>, &property_setter_adaptor<0x076>, &property_setter_adaptor<0x077>,
  &property_setter_adaptor<0x078>, &property_setter_adaptor<0x079>, &property_setter_adaptor<0x07a>, &property_setter_adaptor<0x07b>, &property_setter_adaptor<0x07c>, &property_setter_adaptor<0x07d>, &property_setter_adaptor<0x07e>, &property_setter_adaptor<0x07f>,
  &property_setter_adaptor<0x080>, &property_setter_adaptor<0x081>, &property_setter_adaptor<0x082>, &property_setter_adaptor<0x083>, &property_setter_adaptor<0x084>, &property_setter_adaptor<0x085>, &property_setter_adaptor<0x086>, &property_setter_adaptor<0x087>,
  &property_setter_adaptor<0x088>, &property_setter_adaptor<0x089>, &property_setter_adaptor<0x08a>, &property_setter_adaptor<0x08b>, &property_setter_adaptor<0x08c>, &property_setter_adaptor<0x08d>, &property_setter_adaptor<0x08e>, &property_setter_adaptor<0x08f>,
  &property_setter_adaptor<0x090>, &property_setter_adaptor<0x091>, &property_setter_adaptor<0x092>, &property_setter_adaptor<0x093>, &property_setter_adaptor<0x094>, &property_setter_adaptor<0x095>, &property_setter_adaptor<0x096>, &property_setter_adaptor<0x097>,
  &property_setter_adaptor<0x098>, &property_setter_adaptor<0x099>, &property_setter_adaptor<0x09a>, &property_setter_adaptor<0x09b>, &property_setter_adaptor<0x09c>, &property_setter_adaptor<0x09d>, &property_setter_adaptor<0x09e>, &property_setter_adaptor<0x09f>,
  &property_setter_adaptor<0x0a0>, &property_setter_adaptor<0x0a1>, &property_setter_adaptor<0x0a2>, &property_setter_adaptor<0x0a3>, &property_setter_adaptor<0x0a4>, &property_setter_adaptor<0x0a5>, &property_setter_adaptor<0x0a6>, &property_setter_adaptor<0x0a7>,
  &property_setter_adaptor<0x0a8>, &property_setter_adaptor<0x0a9>, &property_setter_adaptor<0x0aa>, &property_setter_adaptor<0x0ab>, &property_setter_adaptor<0x0ac>, &property_setter_adaptor<0x0ad>, &property_setter_adaptor<0x0ae>, &property_setter_adaptor<0x0af>,
  &property_setter_adaptor<0x0b0>, &property_setter_adaptor<0x0b1>, &property_setter_adaptor<0x0b2>, &property_setter_adaptor<0x0b3>, &property_setter_adaptor<0x0b4>, &property_setter_adaptor<0x0b5>, &property_setter_adaptor<0x0b6>, &property_setter_adaptor<0x0b7>,
  &property_setter_adaptor<0x0b8>, &property_setter_adaptor<0x0b9>, &property_setter_adaptor<0x0ba>, &property_setter_adaptor<0x0bb>, &property_setter_adaptor<0x0bc>, &property_setter_adaptor<0x0bd>, &property_setter_adaptor<0x0be>, &property_setter_adaptor<0x0bf>,
  &property_setter_adaptor<0x0c0>, &property_setter_adaptor<0x0c1>, &property_setter_adaptor<0x0c2>, &property_setter_adaptor<0x0c3>, &property_setter_adaptor<0x0c4>, &property_setter_adaptor<0x0c5>, &property_setter_adaptor<0x0c6>, &property_setter_adaptor<0x0c7>,
  &property_setter_adaptor<0x0c8>, &property_setter_adaptor<0x0c9>, &property_setter_adaptor<0x0ca>, &property_setter_adaptor<0x0cb>, &property_setter_adaptor<0x0cc>, &property_setter_adaptor<0x0cd>, &property_setter_adaptor<0x0ce>, &property_setter_adaptor<0x0cf>,
  &property_setter_adaptor<0x0d0>, &property_setter_adaptor<0x0d1>, &property_setter_adaptor<0x0d2>, &property_setter_adaptor<0x0d3>, &property_setter_adaptor<0x0d4>, &property_setter_adaptor<0x0d5>, &property_setter_adaptor<0x0d6>, &property_setter_adaptor<0x0d7>,
  &property_setter_adaptor<0x0d8>, &property_setter_adaptor<0x0d9>, &property_setter_adaptor<0x0da>, &property_setter_adaptor<0x0db>, &property_setter_adaptor<0x0dc>, &property_setter_adaptor<0x0dd>, &property_setter_adaptor<0x0de>, &property_setter_adaptor<0x0df>,
  &property_setter_adaptor<0x0e0>, &property_setter_adaptor<0x0e1>, &property_setter_adaptor<0x0e2>, &property_setter_adaptor<0x0e3>, &property_setter_adaptor<0x0e4>, &property_setter_adaptor<0x0e5>, &property_setter_adaptor<0x0e6>, &property_setter_adaptor<0x0e7>,
  &property_setter_adaptor<0x0e8>, &property_setter_adaptor<0x0e9>, &property_setter_adaptor<0x0ea>, &property_setter_adaptor<0x0eb>, &property_setter_adaptor<0x0ec>, &property_setter_adaptor<0x0ed>, &property_setter_adaptor<0x0ee>, &property_setter_adaptor<0x0ef>,
  &property_setter_adaptor<0x0f0>, &property_setter_adaptor<0x0f1>, &property_setter_adaptor<0x0f2>, &property_setter_adaptor<0x0f3>, &property_setter_adaptor<0x0f4>, &property_setter_adaptor<0x0f5>, &property_setter_adaptor<0x0f6>, &property_setter_adaptor<0x0f7>,
  &property_setter_adaptor<0x0f8>, &property_setter_adaptor<0x0f9>, &property_setter_adaptor<0x0fa>, &property_setter_adaptor<0x0fb>, &property_setter_adaptor<0x0fc>, &property_setter_adaptor<0x0fd>, &property_setter_adaptor<0x0fe>, &property_setter_adaptor<0x0ff>,
  &property_setter_adaptor<0x100>, &property_setter_adaptor<0x101>, &property_setter_adaptor<0x102>, &property_setter_adaptor<0x103>, &property_setter_adaptor<0x104>, &property_setter_adaptor<0x105>, &property_setter_adaptor<0x106>, &property_setter_adaptor<0x107>,
  &property_setter_adaptor<0x108>, &property_setter_adaptor<0x109>, &property_setter_adaptor<0x10a>, &property_setter_adaptor<0x10b>, &property_setter_adaptor<0x10c>, &property_setter_adaptor<0x10d>, &property_setter_adaptor<0x10e>, &property_setter_adaptor<0x10f>,
  &property_setter_adaptor<0x110>, &property_setter_adaptor<0x111>, &property_setter_adaptor<0x112>, &property_setter_adaptor<0x113>, &property_setter_adaptor<0x114>, &property_setter_adaptor<0x115>, &property_setter_adaptor<0x116>, &property_setter_adaptor<0x117>,
  &property_setter_adaptor<0x118>, &property_setter_adaptor<0x119>, &property_setter_adaptor<0x11a>, &property_setter_adaptor<0x11b>, &property_setter_adaptor<0x11c>, &property_setter_adaptor<0x11d>, &property_setter_adaptor<0x11e>, &property_setter_adaptor<0x11f>,
  &property_setter_adaptor<0x120>, &property_setter_adaptor<0x121>, &property_setter_adaptor<0x122>, &property_setter_adaptor<0x123>, &property_setter_adaptor<0x124>, &property_setter_adaptor<0x125>, &property_setter_adaptor<0x126>, &property_setter_adaptor<0x127>,
  &property_setter_adaptor<0x128>, &property_setter_adaptor<0x129>, &property_setter_adaptor<0x12a>, &property_setter_adaptor<0x12b>, &property_setter_adaptor<0x12c>, &property_setter_adaptor<0x12d>, &property_setter_adaptor<0x12e>, &property_setter_adaptor<0x12f>,
  &property_setter_adaptor<0x130>, &property_setter_adaptor<0x131>, &property_setter_adaptor<0x132>, &property_setter_adaptor<0x133>, &property_setter_adaptor<0x134>, &property_setter_adaptor<0x135>, &property_setter_adaptor<0x136>, &property_setter_adaptor<0x137>,
  &property_setter_adaptor<0x138>, &property_setter_adaptor<0x139>, &property_setter_adaptor<0x13a>, &property_setter_adaptor<0x13b>, &property_setter_adaptor<0x13c>, &property_setter_adaptor<0x13d>, &property_setter_adaptor<0x13e>, &property_setter_adaptor<0x13f>,
  &property_setter_adaptor<0x140>, &property_setter_adaptor<0x141>, &property_setter_adaptor<0x142>, &property_setter_adaptor<0x143>, &property_setter_adaptor<0x144>, &property_setter_adaptor<0x145>, &property_setter_adaptor<0x146>, &property_setter_adaptor<0x147>,
  &property_setter_adaptor<0x148>, &property_setter_adaptor<0x149>, &property_setter_adaptor<0x14a>, &property_setter_adaptor<0x14b>, &property_setter_adaptor<0x14c>, &property_setter_adaptor<0x14d>, &property_setter_adaptor<0x14e>, &property_setter_adaptor<0x14f>,
  &property_setter_adaptor<0x150>, &property_setter_adaptor<0x151>, &property_setter_adaptor<0x152>, &property_setter_adaptor<0x153>, &property_setter_adaptor<0x154>, &property_setter_adaptor<0x155>, &property_setter_adaptor<0x156>, &property_setter_adaptor<0x157>,
  &property_setter_adaptor<0x158>, &property_setter_adaptor<0x159>, &property_setter_adaptor<0x15a>, &property_setter_adaptor<0x15b>, &property_setter_adaptor<0x15c>, &property_setter_adaptor<0x15d>, &property_setter_adaptor<0x15e>, &property_setter_adaptor<0x15f>,
  &property_setter_adaptor<0x160>, &property_setter_adaptor<0x161>, &property_setter_adaptor<0x162>, &property_setter_adaptor<0x163>, &property_setter_adaptor<0x164>, &property_setter_adaptor<0x165>, &property_setter_adaptor<0x166>, &property_setter_adaptor<0x167>,
  &property_setter_adaptor<0x168>, &property_setter_adaptor<0x169>, &property_setter_adaptor<0x16a>, &property_setter_adaptor<0x16b>, &property_setter_adaptor<0x16c>, &property_setter_adaptor<0x16d>, &property_setter_adaptor<0x16e>, &property_setter_adaptor<0x16f>,
  &property_setter_adaptor<0x170>, &property_setter_adaptor<0x171>, &property_setter_adaptor<0x172>, &property_setter_adaptor<0x173>, &property_setter_adaptor<0x174>, &property_setter_adaptor<0x175>, &property_setter_adaptor<0x176>, &property_setter_adaptor<0x177>,
  &property_setter_adaptor<0x178>, &property_setter_adaptor<0x179>, &property_setter_adaptor<0x17a>, &property_setter_adaptor<0x17b>, &property_setter_adaptor<0x17c>, &property_setter_adaptor<0x17d>, &property_setter_adaptor<0x17e>, &property_setter_adaptor<0x17f>,
  &property_setter_adaptor<0x180>, &property_setter_adaptor<0x181>, &property_setter_adaptor<0x182>, &property_setter_adaptor<0x183>, &property_setter_adaptor<0x184>, &property_setter_adaptor<0x185>, &property_setter_adaptor<0x186>, &property_setter_adaptor<0x187>,
  &property_setter_adaptor<0x188>, &property_setter_adaptor<0x189>, &property_setter_adaptor<0x18a>, &property_setter_adaptor<0x18b>, &property_setter_adaptor<0x18c>, &property_setter_adaptor<0x18d>, &property_setter_adaptor<0x18e>, &property_setter_adaptor<0x18f>,
  &property_setter_adaptor<0x190>, &property_setter_adaptor<0x191>, &property_setter_adaptor<0x192>, &property_setter_adaptor<0x193>, &property_setter_adaptor<0x194>, &property_setter_adaptor<0x195>, &property_setter_adaptor<0x196>, &property_setter_adaptor<0x197>,
  &property_setter_adaptor<0x198>, &property_setter_adaptor<0x199>, &property_setter_adaptor<0x19a>, &property_setter_adaptor<0x19b>, &property_setter_adaptor<0x19c>, &property_setter_adaptor<0x19d>, &property_setter_adaptor<0x19e>, &property_setter_adaptor<0x19f>,
  &property_setter_adaptor<0x1a0>, &property_setter_adaptor<0x1a1>, &property_setter_adaptor<0x1a2>, &property_setter_adaptor<0x1a3>, &property_setter_adaptor<0x1a4>, &property_setter_adaptor<0x1a5>, &property_setter_adaptor<0x1a6>, &property_setter_adaptor<0x1a7>,
  &property_setter_adaptor<0x1a8>, &property_setter_adaptor<0x1a9>, &property_setter_adaptor<0x1aa>, &property_setter_adaptor<0x1ab>, &property_setter_adaptor<0x1ac>, &property_setter_adaptor<0x1ad>, &property_setter_adaptor<0x1ae>, &property_setter_adaptor<0x1af>,
  &property_setter_adaptor<0x1b0>, &property_setter_adaptor<0x1b1>, &property_setter_adaptor<0x1b2>, &property_setter_adaptor<0x1b3>, &property_setter_adaptor<0x1b4>, &property_setter_adaptor<0x1b5>, &property_setter_adaptor<0x1b6>, &property_setter_adaptor<0x1b7>,
  &property_setter_adaptor<0x1b8>, &property_setter_adaptor<0x1b9>, &property_setter_adaptor<0x1ba>, &property_setter_adaptor<0x1bb>, &property_setter_adaptor<0x1bc>, &property_setter_adaptor<0x1bd>, &property_setter_adaptor<0x1be>, &property_setter_adaptor<0x1bf>,
  &property_setter_adaptor<0x1c0>, &property_setter_adaptor<0x1c1>, &property_setter_adaptor<0x1c2>, &property_setter_adaptor<0x1c3>, &property_setter_adaptor<0x1c4>, &property_setter_adaptor<0x1c5>, &property_setter_adaptor<0x1c6>, &property_setter_adaptor<0x1c7>,
  &property_setter_adaptor<0x1c8>, &property_setter_adaptor<0x1c9>, &property_setter_adaptor<0x1ca>, &property_setter_adaptor<0x1cb>, &property_setter_adaptor<0x1cc>, &property_setter_adaptor<0x1cd>, &property_setter_adaptor<0x1ce>, &property_setter_adaptor<0x1cf>,
  &property_setter_adaptor<0x1d0>, &property_setter_adaptor<0x1d1>, &property_setter_adaptor<0x1d2>, &property_setter_adaptor<0x1d3>, &property_setter_adaptor<0x1d4>, &property_setter_adaptor<0x1d5>, &property_setter_adaptor<0x1d6>, &property_setter_adaptor<0x1d7>,
  &property_setter_adaptor<0x1d8>, &property_setter_adaptor<0x1d9>, &property_setter_adaptor<0x1da>, &property_setter_adaptor<0x1db>, &property_setter_adaptor<0x1dc>, &property_setter_adaptor<0x1dd>, &property_setter_adaptor<0x1de>, &property_setter_adaptor<0x1df>,
  &property_setter_adaptor<0x1e0>, &property_setter_adaptor<0x1e1>, &property_setter_adaptor<0x1e2>, &property_setter_adaptor<0x1e3>, &property_setter_adaptor<0x1e4>, &property_setter_adaptor<0x1e5>, &property_setter_adaptor<0x1e6>, &property_setter_adaptor<0x1e7>,
  &property_setter_adaptor<0x1e8>, &property_setter_adaptor<0x1e9>, &property_setter_adaptor<0x1ea>, &property_setter_adaptor<0x1eb>, &property_setter_adaptor<0x1ec>, &property_setter_adaptor<0x1ed>, &property_setter_adaptor<0x1ee>, &property_setter_adaptor<0x1ef>,
  &property_setter_adaptor<0x1f0>, &property_setter_adaptor<0x1f1>, &property_setter_adaptor<0x1f2>, &property_setter_adaptor<0x1f3>, &property_setter_adaptor<0x1f4>, &property_setter_adaptor<0x1f5>, &property_setter_adaptor<0x1f6>, &property_setter_adaptor<0x1f7>,
  &property_setter_adaptor<0x1f8>, &property_setter_adaptor<0x1f9>, &property_setter_adaptor<0x1fa>, &property_setter_adaptor<0x1fb>, &property_setter_adaptor<0x1fc>, &property_setter_adaptor<0x1fd>, &property_setter_adaptor<0x1fe>, &property_setter_adaptor<0x1ff>,
  &property_setter_adaptor<0x200>, &property_setter_adaptor<0x201>, &property_setter_adaptor<0x202>, &property_setter_adaptor<0x203>, &property_setter_adaptor<0x204>, &property_setter_adaptor<0x205>, &property_setter_adaptor<0x206>, &property_setter_adaptor<0x207>,
  &property_setter_adaptor<0x208>, &property_setter_adaptor<0x209>, &property_setter_adaptor<0x20a>, &property_setter_adaptor<0x20b>, &property_setter_adaptor<0x20c>, &property_setter_adaptor<0x20d>, &property_setter_adaptor<0x20e>, &property_setter_adaptor<0x20f>,
  &property_setter_adaptor<0x210>, &property_setter_adaptor<0x211>, &property_setter_adaptor<0x212>, &property_setter_adaptor<0x213>, &property_setter_adaptor<0x214>, &property_setter_adaptor<0x215>, &property_setter_adaptor<0x216>, &property_setter_adaptor<0x217>,
  &property_setter_adaptor<0x218>, &property_setter_adaptor<0x219>, &property_setter_adaptor<0x21a>, &property_setter_adaptor<0x21b>, &property_setter_adaptor<0x21c>, &property_setter_adaptor<0x21d>, &property_setter_adaptor<0x21e>, &property_setter_adaptor<0x21f>,
  &property_setter_adaptor<0x220>, &property_setter_adaptor<0x221>, &property_setter_adaptor<0x222>, &property_setter_adaptor<0x223>, &property_setter_adaptor<0x224>, &property_setter_adaptor<0x225>, &property_setter_adaptor<0x226>, &property_setter_adaptor<0x227>,
  &property_setter_adaptor<0x228>, &property_setter_adaptor<0x229>, &property_setter_adaptor<0x22a>, &property_setter_adaptor<0x22b>, &property_setter_adaptor<0x22c>, &property_setter_adaptor<0x22d>, &property_setter_adaptor<0x22e>, &property_setter_adaptor<0x22f>,
  &property_setter_adaptor<0x230>, &property_setter_adaptor<0x231>, &property_setter_adaptor<0x232>, &property_setter_adaptor<0x233>, &property_setter_adaptor<0x234>, &property_setter_adaptor<0x235>, &property_setter_adaptor<0x236>, &property_setter_adaptor<0x237>,
  &property_setter_adaptor<0x238>, &property_setter_adaptor<0x239>, &property_setter_adaptor<0x23a>, &property_setter_adaptor<0x23b>, &property_setter_adaptor<0x23c>, &property_setter_adaptor<0x23d>, &property_setter_adaptor<0x23e>, &property_setter_adaptor<0x23f>,
  &property_setter_adaptor<0x240>, &property_setter_adaptor<0x241>, &property_setter_adaptor<0x242>, &property_setter_adaptor<0x243>, &property_setter_adaptor<0x244>, &property_setter_adaptor<0x245>, &property_setter_adaptor<0x246>, &property_setter_adaptor<0x247>,
  &property_setter_adaptor<0x248>, &property_setter_adaptor<0x249>, &property_setter_adaptor<0x24a>, &property_setter_adaptor<0x24b>, &property_setter_adaptor<0x24c>, &property_setter_adaptor<0x24d>, &property_setter_adaptor<0x24e>, &property_setter_adaptor<0x24f>,
  &property_setter_adaptor<0x250>, &property_setter_adaptor<0x251>, &property_setter_adaptor<0x252>, &property_setter_adaptor<0x253>, &property_setter_adaptor<0x254>, &property_setter_adaptor<0x255>, &property_setter_adaptor<0x256>, &property_setter_adaptor<0x257>,
  &property_setter_adaptor<0x258>, &property_setter_adaptor<0x259>, &property_setter_adaptor<0x25a>, &property_setter_adaptor<0x25b>, &property_setter_adaptor<0x25c>, &property_setter_adaptor<0x25d>, &property_setter_adaptor<0x25e>, &property_setter_adaptor<0x25f>,
  &property_setter_adaptor<0x260>, &property_setter_adaptor<0x261>, &property_setter_adaptor<0x262>, &property_setter_adaptor<0x263>, &property_setter_adaptor<0x264>, &property_setter_adaptor<0x265>, &property_setter_adaptor<0x266>, &property_setter_adaptor<0x267>,
  &property_setter_adaptor<0x268>, &property_setter_adaptor<0x269>, &property_setter_adaptor<0x26a>, &property_setter_adaptor<0x26b>, &property_setter_adaptor<0x26c>, &property_setter_adaptor<0x26d>, &property_setter_adaptor<0x26e>, &property_setter_adaptor<0x26f>,
  &property_setter_adaptor<0x270>, &property_setter_adaptor<0x271>, &property_setter_adaptor<0x272>, &property_setter_adaptor<0x273>, &property_setter_adaptor<0x274>, &property_setter_adaptor<0x275>, &property_setter_adaptor<0x276>, &property_setter_adaptor<0x277>,
  &property_setter_adaptor<0x278>, &property_setter_adaptor<0x279>, &property_setter_adaptor<0x27a>, &property_setter_adaptor<0x27b>, &property_setter_adaptor<0x27c>, &property_setter_adaptor<0x27d>, &property_setter_adaptor<0x27e>, &property_setter_adaptor<0x27f>,
  &property_setter_adaptor<0x280>, &property_setter_adaptor<0x281>, &property_setter_adaptor<0x282>, &property_setter_adaptor<0x283>, &property_setter_adaptor<0x284>, &property_setter_adaptor<0x285>, &property_setter_adaptor<0x286>, &property_setter_adaptor<0x287>,
  &property_setter_adaptor<0x288>, &property_setter_adaptor<0x289>, &property_setter_adaptor<0x28a>, &property_setter_adaptor<0x28b>, &property_setter_adaptor<0x28c>, &property_setter_adaptor<0x28d>, &property_setter_adaptor<0x28e>, &property_setter_adaptor<0x28f>,
  &property_setter_adaptor<0x290>, &property_setter_adaptor<0x291>, &property_setter_adaptor<0x292>, &property_setter_adaptor<0x293>, &property_setter_adaptor<0x294>, &property_setter_adaptor<0x295>, &property_setter_adaptor<0x296>, &property_setter_adaptor<0x297>,
  &property_setter_adaptor<0x298>, &property_setter_adaptor<0x299>, &property_setter_adaptor<0x29a>, &property_setter_adaptor<0x29b>, &property_setter_adaptor<0x29c>, &property_setter_adaptor<0x29d>, &property_setter_adaptor<0x29e>, &property_setter_adaptor<0x29f>,
  &property_setter_adaptor<0x2a0>, &property_setter_adaptor<0x2a1>, &property_setter_adaptor<0x2a2>, &property_setter_adaptor<0x2a3>, &property_setter_adaptor<0x2a4>, &property_setter_adaptor<0x2a5>, &property_setter_adaptor<0x2a6>, &property_setter_adaptor<0x2a7>,
  &property_setter_adaptor<0x2a8>, &property_setter_adaptor<0x2a9>, &property_setter_adaptor<0x2aa>, &property_setter_adaptor<0x2ab>, &property_setter_adaptor<0x2ac>, &property_setter_adaptor<0x2ad>, &property_setter_adaptor<0x2ae>, &property_setter_adaptor<0x2af>,
  &property_setter_adaptor<0x2b0>, &property_setter_adaptor<0x2b1>, &property_setter_adaptor<0x2b2>, &property_setter_adaptor<0x2b3>, &property_setter_adaptor<0x2b4>, &property_setter_adaptor<0x2b5>, &property_setter_adaptor<0x2b6>, &property_setter_adaptor<0x2b7>,
  &property_setter_adaptor<0x2b8>, &property_setter_adaptor<0x2b9>, &property_setter_adaptor<0x2ba>, &property_setter_adaptor<0x2bb>, &property_setter_adaptor<0x2bc>, &property_setter_adaptor<0x2bd>, &property_setter_adaptor<0x2be>, &property_setter_adaptor<0x2bf>,
  &property_setter_adaptor<0x2c0>, &property_setter_adaptor<0x2c1>, &property_setter_adaptor<0x2c2>, &property_setter_adaptor<0x2c3>, &property_setter_adaptor<0x2c4>, &property_setter_adaptor<0x2c5>, &property_setter_adaptor<0x2c6>, &property_setter_adaptor<0x2c7>,
  &property_setter_adaptor<0x2c8>, &property_setter_adaptor<0x2c9>, &property_setter_adaptor<0x2ca>, &property_setter_adaptor<0x2cb>, &property_setter_adaptor<0x2cc>, &property_setter_adaptor<0x2cd>, &property_setter_adaptor<0x2ce>, &property_setter_adaptor<0x2cf>,
  &property_setter_adaptor<0x2d0>, &property_setter_adaptor<0x2d1>, &property_setter_adaptor<0x2d2>, &property_setter_adaptor<0x2d3>, &property_setter_adaptor<0x2d4>, &property_setter_adaptor<0x2d5>, &property_setter_adaptor<0x2d6>, &property_setter_adaptor<0x2d7>,
  &property_setter_adaptor<0x2d8>, &property_setter_adaptor<0x2d9>, &property_setter_adaptor<0x2da>, &property_setter_adaptor<0x2db>, &property_setter_adaptor<0x2dc>, &property_setter_adaptor<0x2dd>, &property_setter_adaptor<0x2de>, &property_setter_adaptor<0x2df>,
  &property_setter_adaptor<0x2e0>, &property_setter_adaptor<0x2e1>, &property_setter_adaptor<0x2e2>, &property_setter_adaptor<0x2e3>, &property_setter_adaptor<0x2e4>, &property_setter_adaptor<0x2e5>, &property_setter_adaptor<0x2e6>, &property_setter_adaptor<0x2e7>,
  &property_setter_adaptor<0x2e8>, &property_setter_adaptor<0x2e9>, &property_setter_adaptor<0x2ea>, &property_setter_adaptor<0x2eb>, &property_setter_adaptor<0x2ec>, &property_setter_adaptor<0x2ed>, &property_setter_adaptor<0x2ee>, &property_setter_adaptor<0x2ef>,
  &property_setter_adaptor<0x2f0>, &property_setter_adaptor<0x2f1>, &property_setter_adaptor<0x2f2>, &property_setter_adaptor<0x2f3>, &property_setter_adaptor<0x2f4>, &property_setter_adaptor<0x2f5>, &property_setter_adaptor<0x2f6>, &property_setter_adaptor<0x2f7>,
  &property_setter_adaptor<0x2f8>, &property_setter_adaptor<0x2f9>, &property_setter_adaptor<0x2fa>, &property_setter_adaptor<0x2fb>, &property_setter_adaptor<0x2fc>, &property_setter_adaptor<0x2fd>, &property_setter_adaptor<0x2fe>, &property_setter_adaptor<0x2ff>,
  &property_setter_adaptor<0x300>, &property_setter_adaptor<0x301>, &property_setter_adaptor<0x302>, &property_setter_adaptor<0x303>, &property_setter_adaptor<0x304>, &property_setter_adaptor<0x305>, &property_setter_adaptor<0x306>, &property_setter_adaptor<0x307>,
  &property_setter_adaptor<0x308>, &property_setter_adaptor<0x309>, &property_setter_adaptor<0x30a>, &property_setter_adaptor<0x30b>, &property_setter_adaptor<0x30c>, &property_setter_adaptor<0x30d>, &property_setter_adaptor<0x30e>, &property_setter_adaptor<0x30f>,
  &property_setter_adaptor<0x310>, &property_setter_adaptor<0x311>, &property_setter_adaptor<0x312>, &property_setter_adaptor<0x313>, &property_setter_adaptor<0x314>, &property_setter_adaptor<0x315>, &property_setter_adaptor<0x316>, &property_setter_adaptor<0x317>,
  &property_setter_adaptor<0x318>, &property_setter_adaptor<0x319>, &property_setter_adaptor<0x31a>, &property_setter_adaptor<0x31b>, &property_setter_adaptor<0x31c>, &property_setter_adaptor<0x31d>, &property_setter_adaptor<0x31e>, &property_setter_adaptor<0x31f>,
  &property_setter_adaptor<0x320>, &property_setter_adaptor<0x321>, &property_setter_adaptor<0x322>, &property_setter_adaptor<0x323>, &property_setter_adaptor<0x324>, &property_setter_adaptor<0x325>, &property_setter_adaptor<0x326>, &property_setter_adaptor<0x327>,
  &property_setter_adaptor<0x328>, &property_setter_adaptor<0x329>, &property_setter_adaptor<0x32a>, &property_setter_adaptor<0x32b>, &property_setter_adaptor<0x32c>, &property_setter_adaptor<0x32d>, &property_setter_adaptor<0x32e>, &property_setter_adaptor<0x32f>,
  &property_setter_adaptor<0x330>, &property_setter_adaptor<0x331>, &property_setter_adaptor<0x332>, &property_setter_adaptor<0x333>, &property_setter_adaptor<0x334>, &property_setter_adaptor<0x335>, &property_setter_adaptor<0x336>, &property_setter_adaptor<0x337>,
  &property_setter_adaptor<0x338>, &property_setter_adaptor<0x339>, &property_setter_adaptor<0x33a>, &property_setter_adaptor<0x33b>, &property_setter_adaptor<0x33c>, &property_setter_adaptor<0x33d>, &property_setter_adaptor<0x33e>, &property_setter_adaptor<0x33f>,
  &property_setter_adaptor<0x340>, &property_setter_adaptor<0x341>, &property_setter_adaptor<0x342>, &property_setter_adaptor<0x343>, &property_setter_adaptor<0x344>, &property_setter_adaptor<0x345>, &property_setter_adaptor<0x346>, &property_setter_adaptor<0x347>,
  &property_setter_adaptor<0x348>, &property_setter_adaptor<0x349>, &property_setter_adaptor<0x34a>, &property_setter_adaptor<0x34b>, &property_setter_adaptor<0x34c>, &property_setter_adaptor<0x34d>, &property_setter_adaptor<0x34e>, &property_setter_adaptor<0x34f>,
  &property_setter_adaptor<0x350>, &property_setter_adaptor<0x351>, &property_setter_adaptor<0x352>, &property_setter_adaptor<0x353>, &property_setter_adaptor<0x354>, &property_setter_adaptor<0x355>, &property_setter_adaptor<0x356>, &property_setter_adaptor<0x357>,
  &property_setter_adaptor<0x358>, &property_setter_adaptor<0x359>, &property_setter_adaptor<0x35a>, &property_setter_adaptor<0x35b>, &property_setter_adaptor<0x35c>, &property_setter_adaptor<0x35d>, &property_setter_adaptor<0x35e>, &property_setter_adaptor<0x35f>,
  &property_setter_adaptor<0x360>, &property_setter_adaptor<0x361>, &property_setter_adaptor<0x362>, &property_setter_adaptor<0x363>, &property_setter_adaptor<0x364>, &property_setter_adaptor<0x365>, &property_setter_adaptor<0x366>, &property_setter_adaptor<0x367>,
  &property_setter_adaptor<0x368>, &property_setter_adaptor<0x369>, &property_setter_adaptor<0x36a>, &property_setter_adaptor<0x36b>, &property_setter_adaptor<0x36c>, &property_setter_adaptor<0x36d>, &property_setter_adaptor<0x36e>, &property_setter_adaptor<0x36f>,
  &property_setter_adaptor<0x370>, &property_setter_adaptor<0x371>, &property_setter_adaptor<0x372>, &property_setter_adaptor<0x373>, &property_setter_adaptor<0x374>, &property_setter_adaptor<0x375>, &property_setter_adaptor<0x376>, &property_setter_adaptor<0x377>,
  &property_setter_adaptor<0x378>, &property_setter_adaptor<0x379>, &property_setter_adaptor<0x37a>, &property_setter_adaptor<0x37b>, &property_setter_adaptor<0x37c>, &property_setter_adaptor<0x37d>, &property_setter_adaptor<0x37e>, &property_setter_adaptor<0x37f>,
  &property_setter_adaptor<0x380>, &property_setter_adaptor<0x381>, &property_setter_adaptor<0x382>, &property_setter_adaptor<0x383>, &property_setter_adaptor<0x384>, &property_setter_adaptor<0x385>, &property_setter_adaptor<0x386>, &property_setter_adaptor<0x387>,
  &property_setter_adaptor<0x388>, &property_setter_adaptor<0x389>, &property_setter_adaptor<0x38a>, &property_setter_adaptor<0x38b>, &property_setter_adaptor<0x38c>, &property_setter_adaptor<0x38d>, &property_setter_adaptor<0x38e>, &property_setter_adaptor<0x38f>,
  &property_setter_adaptor<0x390>, &property_setter_adaptor<0x391>, &property_setter_adaptor<0x392>, &property_setter_adaptor<0x393>, &property_setter_adaptor<0x394>, &property_setter_adaptor<0x395>, &property_setter_adaptor<0x396>, &property_setter_adaptor<0x397>,
  &property_setter_adaptor<0x398>, &property_setter_adaptor<0x399>, &property_setter_adaptor<0x39a>, &property_setter_adaptor<0x39b>, &property_setter_adaptor<0x39c>, &property_setter_adaptor<0x39d>, &property_setter_adaptor<0x39e>, &property_setter_adaptor<0x39f>,
  &property_setter_adaptor<0x3a0>, &property_setter_adaptor<0x3a1>, &property_setter_adaptor<0x3a2>, &property_setter_adaptor<0x3a3>, &property_setter_adaptor<0x3a4>, &property_setter_adaptor<0x3a5>, &property_setter_adaptor<0x3a6>, &property_setter_adaptor<0x3a7>,
  &property_setter_adaptor<0x3a8>, &property_setter_adaptor<0x3a9>, &property_setter_adaptor<0x3aa>, &property_setter_adaptor<0x3ab>, &property_setter_adaptor<0x3ac>, &property_setter_adaptor<0x3ad>, &property_setter_adaptor<0x3ae>, &property_setter_adaptor<0x3af>,
  &property_setter_adaptor<0x3b0>, &property_setter_adaptor<0x3b1>, &property_setter_adaptor<0x3b2>, &property_setter_adaptor<0x3b3>, &property_setter_adaptor<0x3b4>, &property_setter_adaptor<0x3b5>, &property_setter_adaptor<0x3b6>, &property_setter_adaptor<0x3b7>,
  &property_setter_adaptor<0x3b8>, &property_setter_adaptor<0x3b9>, &property_setter_adaptor<0x3ba>, &property_setter_adaptor<0x3bb>, &property_setter_adaptor<0x3bc>, &property_setter_adaptor<0x3bd>, &property_setter_adaptor<0x3be>, &property_setter_adaptor<0x3bf>,
  &property_setter_adaptor<0x3c0>, &property_setter_adaptor<0x3c1>, &property_setter_adaptor<0x3c2>, &property_setter_adaptor<0x3c3>, &property_setter_adaptor<0x3c4>, &property_setter_adaptor<0x3c5>, &property_setter_adaptor<0x3c6>, &property_setter_adaptor<0x3c7>,
  &property_setter_adaptor<0x3c8>, &property_setter_adaptor<0x3c9>, &property_setter_adaptor<0x3ca>, &property_setter_adaptor<0x3cb>, &property_setter_adaptor<0x3cc>, &property_setter_adaptor<0x3cd>, &property_setter_adaptor<0x3ce>, &property_setter_adaptor<0x3cf>,
  &property_setter_adaptor<0x3d0>, &property_setter_adaptor<0x3d1>, &property_setter_adaptor<0x3d2>, &property_setter_adaptor<0x3d3>, &property_setter_adaptor<0x3d4>, &property_setter_adaptor<0x3d5>, &property_setter_adaptor<0x3d6>, &property_setter_adaptor<0x3d7>,
  &property_setter_adaptor<0x3d8>, &property_setter_adaptor<0x3d9>, &property_setter_adaptor<0x3da>, &property_setter_adaptor<0x3db>, &property_setter_adaptor<0x3dc>, &property_setter_adaptor<0x3dd>, &property_setter_adaptor<0x3de>, &property_setter_adaptor<0x3df>,
  &property_setter_adaptor<0x3e0>, &property_setter_adaptor<0x3e1>, &property_setter_adaptor<0x3e2>, &property_setter_adaptor<0x3e3>, &property_setter_adaptor<0x3e4>, &property_setter_adaptor<0x3e5>, &property_setter_adaptor<0x3e6>, &property_setter_adaptor<0x3e7>,
  &property_setter_adaptor<0x3e8>, &property_setter_adaptor<0x3e9>, &property_setter_adaptor<0x3ea>, &property_setter_adaptor<0x3eb>, &property_setter_adaptor<0x3ec>, &property_setter_adaptor<0x3ed>, &property_setter_adaptor<0x3ee>, &property_setter_adaptor<0x3ef>,
  &property_setter_adaptor<0x3f0>, &property_setter_adaptor<0x3f1>, &property_setter_adaptor<0x3f2>, &property_setter_adaptor<0x3f3>, &property_setter_adaptor<0x3f4>, &property_setter_adaptor<0x3f5>, &property_setter_adaptor<0x3f6>, &property_setter_adaptor<0x3f7>,
  &property_setter_adaptor<0x3f8>, &property_setter_adaptor<0x3f9>, &property_setter_adaptor<0x3fa>, &property_setter_adaptor<0x3fb>, &property_setter_adaptor<0x3fc>, &property_setter_adaptor<0x3fd>, &property_setter_adaptor<0x3fe>, &property_setter_adaptor<0x3ff>,
  &property_setter_adaptor<0x400>, &property_setter_adaptor<0x401>, &property_setter_adaptor<0x402>, &property_setter_adaptor<0x403>, &property_setter_adaptor<0x404>, &property_setter_adaptor<0x405>, &property_setter_adaptor<0x406>, &property_setter_adaptor<0x407>,
  &property_setter_adaptor<0x408>, &property_setter_adaptor<0x409>, &property_setter_adaptor<0x40a>, &property_setter_adaptor<0x40b>, &property_setter_adaptor<0x40c>, &property_setter_adaptor<0x40d>, &property_setter_adaptor<0x40e>, &property_setter_adaptor<0x40f>,
  &property_setter_adaptor<0x410>, &property_setter_adaptor<0x411>, &property_setter_adaptor<0x412>, &property_setter_adaptor<0x413>, &property_setter_adaptor<0x414>, &property_setter_adaptor<0x415>, &property_setter_adaptor<0x416>, &property_setter_adaptor<0x417>,
  &property_setter_adaptor<0x418>, &property_setter_adaptor<0x419>, &property_setter_adaptor<0x41a>, &property_setter_adaptor<0x41b>, &property_setter_adaptor<0x41c>, &property_setter_adaptor<0x41d>, &property_setter_adaptor<0x41e>, &property_setter_adaptor<0x41f>,
  &property_setter_adaptor<0x420>, &property_setter_adaptor<0x421>, &property_setter_adaptor<0x422>, &property_setter_adaptor<0x423>, &property_setter_adaptor<0x424>, &property_setter_adaptor<0x425>, &property_setter_adaptor<0x426>, &property_setter_adaptor<0x427>,
  &property_setter_adaptor<0x428>, &property_setter_adaptor<0x429>, &property_setter_adaptor<0x42a>, &property_setter_adaptor<0x42b>, &property_setter_adaptor<0x42c>, &property_setter_adaptor<0x42d>, &property_setter_adaptor<0x42e>, &property_setter_adaptor<0x42f>,
  &property_setter_adaptor<0x430>, &property_setter_adaptor<0x431>, &property_setter_adaptor<0x432>, &property_setter_adaptor<0x433>, &property_setter_adaptor<0x434>, &property_setter_adaptor<0x435>, &property_setter_adaptor<0x436>, &property_setter_adaptor<0x437>,
  &property_setter_adaptor<0x438>, &property_setter_adaptor<0x439>, &property_setter_adaptor<0x43a>, &property_setter_adaptor<0x43b>, &property_setter_adaptor<0x43c>, &property_setter_adaptor<0x43d>, &property_setter_adaptor<0x43e>, &property_setter_adaptor<0x43f>,
  &property_setter_adaptor<0x440>, &property_setter_adaptor<0x441>, &property_setter_adaptor<0x442>, &property_setter_adaptor<0x443>, &property_setter_adaptor<0x444>, &property_setter_adaptor<0x445>, &property_setter_adaptor<0x446>, &property_setter_adaptor<0x447>,
  &property_setter_adaptor<0x448>, &property_setter_adaptor<0x449>, &property_setter_adaptor<0x44a>, &property_setter_adaptor<0x44b>, &property_setter_adaptor<0x44c>, &property_setter_adaptor<0x44d>, &property_setter_adaptor<0x44e>, &property_setter_adaptor<0x44f>,
  &property_setter_adaptor<0x450>, &property_setter_adaptor<0x451>, &property_setter_adaptor<0x452>, &property_setter_adaptor<0x453>, &property_setter_adaptor<0x454>, &property_setter_adaptor<0x455>, &property_setter_adaptor<0x456>, &property_setter_adaptor<0x457>,
  &property_setter_adaptor<0x458>, &property_setter_adaptor<0x459>, &property_setter_adaptor<0x45a>, &property_setter_adaptor<0x45b>, &property_setter_adaptor<0x45c>, &property_setter_adaptor<0x45d>, &property_setter_adaptor<0x45e>, &property_setter_adaptor<0x45f>,
  &property_setter_adaptor<0x460>, &property_setter_adaptor<0x461>, &property_setter_adaptor<0x462>, &property_setter_adaptor<0x463>, &property_setter_adaptor<0x464>, &property_setter_adaptor<0x465>, &property_setter_adaptor<0x466>, &property_setter_adaptor<0x467>,
  &property_setter_adaptor<0x468>, &property_setter_adaptor<0x469>, &property_setter_adaptor<0x46a>, &property_setter_adaptor<0x46b>, &property_setter_adaptor<0x46c>, &property_setter_adaptor<0x46d>, &property_setter_adaptor<0x46e>, &property_setter_adaptor<0x46f>,
  &property_setter_adaptor<0x470>, &property_setter_adaptor<0x471>, &property_setter_adaptor<0x472>, &property_setter_adaptor<0x473>, &property_setter_adaptor<0x474>, &property_setter_adaptor<0x475>, &property_setter_adaptor<0x476>, &property_setter_adaptor<0x477>,
  &property_setter_adaptor<0x478>, &property_setter_adaptor<0x479>, &property_setter_adaptor<0x47a>, &property_setter_adaptor<0x47b>, &property_setter_adaptor<0x47c>, &property_setter_adaptor<0x47d>, &property_setter_adaptor<0x47e>, &property_setter_adaptor<0x47f>,
  &property_setter_adaptor<0x480>, &property_setter_adaptor<0x481>, &property_setter_adaptor<0x482>, &property_setter_adaptor<0x483>, &property_setter_adaptor<0x484>, &property_setter_adaptor<0x485>, &property_setter_adaptor<0x486>, &property_setter_adaptor<0x487>,
  &property_setter_adaptor<0x488>, &property_setter_adaptor<0x489>, &property_setter_adaptor<0x48a>, &property_setter_adaptor<0x48b>, &property_setter_adaptor<0x48c>, &property_setter_adaptor<0x48d>, &property_setter_adaptor<0x48e>, &property_setter_adaptor<0x48f>,
  &property_setter_adaptor<0x490>, &property_setter_adaptor<0x491>, &property_setter_adaptor<0x492>, &property_setter_adaptor<0x493>, &property_setter_adaptor<0x494>, &property_setter_adaptor<0x495>, &property_setter_adaptor<0x496>, &property_setter_adaptor<0x497>,
  &property_setter_adaptor<0x498>, &property_setter_adaptor<0x499>, &property_setter_adaptor<0x49a>, &property_setter_adaptor<0x49b>, &property_setter_adaptor<0x49c>, &property_setter_adaptor<0x49d>, &property_setter_adaptor<0x49e>, &property_setter_adaptor<0x49f>,
  &property_setter_adaptor<0x4a0>, &property_setter_adaptor<0x4a1>, &property_setter_adaptor<0x4a2>, &property_setter_adaptor<0x4a3>, &property_setter_adaptor<0x4a4>, &property_setter_adaptor<0x4a5>, &property_setter_adaptor<0x4a6>, &property_setter_adaptor<0x4a7>,
  &property_setter_adaptor<0x4a8>, &property_setter_adaptor<0x4a9>, &property_setter_adaptor<0x4aa>, &property_setter_adaptor<0x4ab>, &property_setter_adaptor<0x4ac>, &property_setter_adaptor<0x4ad>, &property_setter_adaptor<0x4ae>, &property_setter_adaptor<0x4af>,
  &property_setter_adaptor<0x4b0>, &property_setter_adaptor<0x4b1>, &property_setter_adaptor<0x4b2>, &property_setter_adaptor<0x4b3>, &property_setter_adaptor<0x4b4>, &property_setter_adaptor<0x4b5>, &property_setter_adaptor<0x4b6>, &property_setter_adaptor<0x4b7>,
  &property_setter_adaptor<0x4b8>, &property_setter_adaptor<0x4b9>, &property_setter_adaptor<0x4ba>, &property_setter_adaptor<0x4bb>, &property_setter_adaptor<0x4bc>, &property_setter_adaptor<0x4bd>, &property_setter_adaptor<0x4be>, &property_setter_adaptor<0x4bf>,
  &property_setter_adaptor<0x4c0>, &property_setter_adaptor<0x4c1>, &property_setter_adaptor<0x4c2>, &property_setter_adaptor<0x4c3>, &property_setter_adaptor<0x4c4>, &property_setter_adaptor<0x4c5>, &property_setter_adaptor<0x4c6>, &property_setter_adaptor<0x4c7>,
  &property_setter_adaptor<0x4c8>, &property_setter_adaptor<0x4c9>, &property_setter_adaptor<0x4ca>, &property_setter_adaptor<0x4cb>, &property_setter_adaptor<0x4cc>, &property_setter_adaptor<0x4cd>, &property_setter_adaptor<0x4ce>, &property_setter_adaptor<0x4cf>,
  &property_setter_adaptor<0x4d0>, &property_setter_adaptor<0x4d1>, &property_setter_adaptor<0x4d2>, &property_setter_adaptor<0x4d3>, &property_setter_adaptor<0x4d4>, &property_setter_adaptor<0x4d5>, &property_setter_adaptor<0x4d6>, &property_setter_adaptor<0x4d7>,
  &property_setter_adaptor<0x4d8>, &property_setter_adaptor<0x4d9>, &property_setter_adaptor<0x4da>, &property_setter_adaptor<0x4db>, &property_setter_adaptor<0x4dc>, &property_setter_adaptor<0x4dd>, &property_setter_adaptor<0x4de>, &property_setter_adaptor<0x4df>,
  &property_setter_adaptor<0x4e0>, &property_setter_adaptor<0x4e1>, &property_setter_adaptor<0x4e2>, &property_setter_adaptor<0x4e3>, &property_setter_adaptor<0x4e4>, &property_setter_adaptor<0x4e5>, &property_setter_adaptor<0x4e6>, &property_setter_adaptor<0x4e7>,
  &property_setter_adaptor<0x4e8>, &property_setter_adaptor<0x4e9>, &property_setter_adaptor<0x4ea>, &property_setter_adaptor<0x4eb>, &property_setter_adaptor<0x4ec>, &property_setter_adaptor<0x4ed>, &property_setter_adaptor<0x4ee>, &property_setter_adaptor<0x4ef>,
  &property_setter_adaptor<0x4f0>, &property_setter_adaptor<0x4f1>, &property_setter_adaptor<0x4f2>, &property_setter_adaptor<0x4f3>, &property_setter_adaptor<0x4f4>, &property_setter_adaptor<0x4f5>, &property_setter_adaptor<0x4f6>, &property_setter_adaptor<0x4f7>,
  &property_setter_adaptor<0x4f8>, &property_setter_adaptor<0x4f9>, &property_setter_adaptor<0x4fa>, &property_setter_adaptor<0x4fb>, &property_setter_adaptor<0x4fc>, &property_setter_adaptor<0x4fd>, &property_setter_adaptor<0x4fe>, &property_setter_adaptor<0x4ff>,
};

py_func_ptr_t get_property_setter_adaptor (int n)
{
  tl_assert (n >= 0 && n < int (sizeof (property_setter_adaptors) / sizeof (property_setter_adaptors [0])));
  return property_setter_adaptors [n];
}

template <int N>
PyObject *method_init_adaptor (PyObject *self, PyObject *args)
{
  return method_init_adaptor (N, self, args);
}

static py_func_ptr_t method_init_adaptors [] =
{
  &method_init_adaptor<0x000>, &method_init_adaptor<0x001>, &method_init_adaptor<0x002>, &method_init_adaptor<0x003>, &method_init_adaptor<0x004>, &method_init_adaptor<0x005>, &method_init_adaptor<0x006>, &method_init_adaptor<0x007>,
  &method_init_adaptor<0x008>, &method_init_adaptor<0x009>, &method_init_adaptor<0x00a>, &method_init_adaptor<0x00b>, &method_init_adaptor<0x00c>, &method_init_adaptor<0x00d>, &method_init_adaptor<0x00e>, &method_init_adaptor<0x00f>,
  &method_init_adaptor<0x010>, &method_init_adaptor<0x011>, &method_init_adaptor<0x012>, &method_init_adaptor<0x013>, &method_init_adaptor<0x014>, &method_init_adaptor<0x015>, &method_init_adaptor<0x016>, &method_init_adaptor<0x017>,
  &method_init_adaptor<0x018>, &method_init_adaptor<0x019>, &method_init_adaptor<0x01a>, &method_init_adaptor<0x01b>, &method_init_adaptor<0x01c>, &method_init_adaptor<0x01d>, &method_init_adaptor<0x01e>, &method_init_adaptor<0x01f>,
  &method_init_adaptor<0x020>, &method_init_adaptor<0x021>, &method_init_adaptor<0x022>, &method_init_adaptor<0x023>, &method_init_adaptor<0x024>, &method_init_adaptor<0x025>, &method_init_adaptor<0x026>, &method_init_adaptor<0x027>,
  &method_init_adaptor<0x028>, &method_init_adaptor<0x029>, &method_init_adaptor<0x02a>, &method_init_adaptor<0x02b>, &method_init_adaptor<0x02c>, &method_init_adaptor<0x02d>, &method_init_adaptor<0x02e>, &method_init_adaptor<0x02f>,
  &method_init_adaptor<0x030>, &method_init_adaptor<0x031>, &method_init_adaptor<0x032>, &method_init_adaptor<0x033>, &method_init_adaptor<0x034>, &method_init_adaptor<0x035>, &method_init_adaptor<0x036>, &method_init_adaptor<0x037>,
  &method_init_adaptor<0x038>, &method_init_adaptor<0x039>, &method_init_adaptor<0x03a>, &method_init_adaptor<0x03b>, &method_init_adaptor<0x03c>, &method_init_adaptor<0x03d>, &method_init_adaptor<0x03e>, &method_init_adaptor<0x03f>,
  &method_init_adaptor<0x040>, &method_init_adaptor<0x041>, &method_init_adaptor<0x042>, &method_init_adaptor<0x043>, &method_init_adaptor<0x044>, &method_init_adaptor<0x045>, &method_init_adaptor<0x046>, &method_init_adaptor<0x047>,
  &method_init_adaptor<0x048>, &method_init_adaptor<0x049>, &method_init_adaptor<0x04a>, &method_init_adaptor<0x04b>, &method_init_adaptor<0x04c>, &method_init_adaptor<0x04d>, &method_init_adaptor<0x04e>, &method_init_adaptor<0x04f>,
  &method_init_adaptor<0x050>, &method_init_adaptor<0x051>, &method_init_adaptor<0x052>, &method_init_adaptor<0x053>, &method_init_adaptor<0x054>, &method_init_adaptor<0x055>, &method_init_adaptor<0x056>, &method_init_adaptor<0x057>,
  &method_init_adaptor<0x058>, &method_init_adaptor<0x059>, &method_init_adaptor<0x05a>, &method_init_adaptor<0x05b>, &method_init_adaptor<0x05c>, &method_init_adaptor<0x05d>, &method_init_adaptor<0x05e>, &method_init_adaptor<0x05f>,
  &method_init_adaptor<0x060>, &method_init_adaptor<0x061>, &method_init_adaptor<0x062>, &method_init_adaptor<0x063>, &method_init_adaptor<0x064>, &method_init_adaptor<0x065>, &method_init_adaptor<0x066>, &method_init_adaptor<0x067>,
  &method_init_adaptor<0x068>, &method_init_adaptor<0x069>, &method_init_adaptor<0x06a>, &method_init_adaptor<0x06b>, &method_init_adaptor<0x06c>, &method_init_adaptor<0x06d>, &method_init_adaptor<0x06e>, &method_init_adaptor<0x06f>,
  &method_init_adaptor<0x070>, &method_init_adaptor<0x071>, &method_init_adaptor<0x072>, &method_init_adaptor<0x073>, &method_init_adaptor<0x074>, &method_init_adaptor<0x075>, &method_init_adaptor<0x076>, &method_init_adaptor<0x077>,
  &method_init_adaptor<0x078>, &method_init_adaptor<0x079>, &method_init_adaptor<0x07a>, &method_init_adaptor<0x07b>, &method_init_adaptor<0x07c>, &method_init_adaptor<0x07d>, &method_init_adaptor<0x07e>, &method_init_adaptor<0x07f>,
  &method_init_adaptor<0x080>, &method_init_adaptor<0x081>, &method_init_adaptor<0x082>, &method_init_adaptor<0x083>, &method_init_adaptor<0x084>, &method_init_adaptor<0x085>, &method_init_adaptor<0x086>, &method_init_adaptor<0x087>,
  &method_init_adaptor<0x088>, &method_init_adaptor<0x089>, &method_init_adaptor<0x08a>, &method_init_adaptor<0x08b>, &method_init_adaptor<0x08c>, &method_init_adaptor<0x08d>, &method_init_adaptor<0x08e>, &method_init_adaptor<0x08f>,
  &method_init_adaptor<0x090>, &method_init_adaptor<0x091>, &method_init_adaptor<0x092>, &method_init_adaptor<0x093>, &method_init_adaptor<0x094>, &method_init_adaptor<0x095>, &method_init_adaptor<0x096>, &method_init_adaptor<0x097>,
  &method_init_adaptor<0x098>, &method_init_adaptor<0x099>, &method_init_adaptor<0x09a>, &method_init_adaptor<0x09b>, &method_init_adaptor<0x09c>, &method_init_adaptor<0x09d>, &method_init_adaptor<0x09e>, &method_init_adaptor<0x09f>,
  &method_init_adaptor<0x0a0>, &method_init_adaptor<0x0a1>, &method_init_adaptor<0x0a2>, &method_init_adaptor<0x0a3>, &method_init_adaptor<0x0a4>, &method_init_adaptor<0x0a5>, &method_init_adaptor<0x0a6>, &method_init_adaptor<0x0a7>,
  &method_init_adaptor<0x0a8>, &method_init_adaptor<0x0a9>, &method_init_adaptor<0x0aa>, &method_init_adaptor<0x0ab>, &method_init_adaptor<0x0ac>, &method_init_adaptor<0x0ad>, &method_init_adaptor<0x0ae>, &method_init_adaptor<0x0af>,
  &method_init_adaptor<0x0b0>, &method_init_adaptor<0x0b1>, &method_init_adaptor<0x0b2>, &method_init_adaptor<0x0b3>, &method_init_adaptor<0x0b4>, &method_init_adaptor<0x0b5>, &method_init_adaptor<0x0b6>, &method_init_adaptor<0x0b7>,
  &method_init_adaptor<0x0b8>, &method_init_adaptor<0x0b9>, &method_init_adaptor<0x0ba>, &method_init_adaptor<0x0bb>, &method_init_adaptor<0x0bc>, &method_init_adaptor<0x0bd>, &method_init_adaptor<0x0be>, &method_init_adaptor<0x0bf>,
  &method_init_adaptor<0x0c0>, &method_init_adaptor<0x0c1>, &method_init_adaptor<0x0c2>, &method_init_adaptor<0x0c3>, &method_init_adaptor<0x0c4>, &method_init_adaptor<0x0c5>, &method_init_adaptor<0x0c6>, &method_init_adaptor<0x0c7>,
  &method_init_adaptor<0x0c8>, &method_init_adaptor<0x0c9>, &method_init_adaptor<0x0ca>, &method_init_adaptor<0x0cb>, &method_init_adaptor<0x0cc>, &method_init_adaptor<0x0cd>, &method_init_adaptor<0x0ce>, &method_init_adaptor<0x0cf>,
  &method_init_adaptor<0x0d0>, &method_init_adaptor<0x0d1>, &method_init_adaptor<0x0d2>, &method_init_adaptor<0x0d3>, &method_init_adaptor<0x0d4>, &method_init_adaptor<0x0d5>, &method_init_adaptor<0x0d6>, &method_init_adaptor<0x0d7>,
  &method_init_adaptor<0x0d8>, &method_init_adaptor<0x0d9>, &method_init_adaptor<0x0da>, &method_init_adaptor<0x0db>, &method_init_adaptor<0x0dc>, &method_init_adaptor<0x0dd>, &method_init_adaptor<0x0de>, &method_init_adaptor<0x0df>,
  &method_init_adaptor<0x0e0>, &method_init_adaptor<0x0e1>, &method_init_adaptor<0x0e2>, &method_init_adaptor<0x0e3>, &method_init_adaptor<0x0e4>, &method_init_adaptor<0x0e5>, &method_init_adaptor<0x0e6>, &method_init_adaptor<0x0e7>,
  &method_init_adaptor<0x0e8>, &method_init_adaptor<0x0e9>, &method_init_adaptor<0x0ea>, &method_init_adaptor<0x0eb>, &method_init_adaptor<0x0ec>, &method_init_adaptor<0x0ed>, &method_init_adaptor<0x0ee>, &method_init_adaptor<0x0ef>,
  &method_init_adaptor<0x0f0>, &method_init_adaptor<0x0f1>, &method_init_adaptor<0x0f2>, &method_init_adaptor<0x0f3>, &method_init_adaptor<0x0f4>, &method_init_adaptor<0x0f5>, &method_init_adaptor<0x0f6>, &method_init_adaptor<0x0f7>,
  &method_init_adaptor<0x0f8>, &method_init_adaptor<0x0f9>, &method_init_adaptor<0x0fa>, &method_init_adaptor<0x0fb>, &method_init_adaptor<0x0fc>, &method_init_adaptor<0x0fd>, &method_init_adaptor<0x0fe>, &method_init_adaptor<0x0ff>,
  &method_init_adaptor<0x100>, &method_init_adaptor<0x101>, &method_init_adaptor<0x102>, &method_init_adaptor<0x103>, &method_init_adaptor<0x104>, &method_init_adaptor<0x105>, &method_init_adaptor<0x106>, &method_init_adaptor<0x107>,
  &method_init_adaptor<0x108>, &method_init_adaptor<0x109>, &method_init_adaptor<0x10a>, &method_init_adaptor<0x10b>, &method_init_adaptor<0x10c>, &method_init_adaptor<0x10d>, &method_init_adaptor<0x10e>, &method_init_adaptor<0x10f>,
  &method_init_adaptor<0x110>, &method_init_adaptor<0x111>, &method_init_adaptor<0x112>, &method_init_adaptor<0x113>, &method_init_adaptor<0x114>, &method_init_adaptor<0x115>, &method_init_adaptor<0x116>, &method_init_adaptor<0x117>,
  &method_init_adaptor<0x118>, &method_init_adaptor<0x119>, &method_init_adaptor<0x11a>, &method_init_adaptor<0x11b>, &method_init_adaptor<0x11c>, &method_init_adaptor<0x11d>, &method_init_adaptor<0x11e>, &method_init_adaptor<0x11f>,
  &method_init_adaptor<0x120>, &method_init_adaptor<0x121>, &method_init_adaptor<0x122>, &method_init_adaptor<0x123>, &method_init_adaptor<0x124>, &method_init_adaptor<0x125>, &method_init_adaptor<0x126>, &method_init_adaptor<0x127>,
  &method_init_adaptor<0x128>, &method_init_adaptor<0x129>, &method_init_adaptor<0x12a>, &method_init_adaptor<0x12b>, &method_init_adaptor<0x12c>, &method_init_adaptor<0x12d>, &method_init_adaptor<0x12e>, &method_init_adaptor<0x12f>,
  &method_init_adaptor<0x130>, &method_init_adaptor<0x131>, &method_init_adaptor<0x132>, &method_init_adaptor<0x133>, &method_init_adaptor<0x134>, &method_init_adaptor<0x135>, &method_init_adaptor<0x136>, &method_init_adaptor<0x137>,
  &method_init_adaptor<0x138>, &method_init_adaptor<0x139>, &method_init_adaptor<0x13a>, &method_init_adaptor<0x13b>, &method_init_adaptor<0x13c>, &method_init_adaptor<0x13d>, &method_init_adaptor<0x13e>, &method_init_adaptor<0x13f>,
  &method_init_adaptor<0x140>, &method_init_adaptor<0x141>, &method_init_adaptor<0x142>, &method_init_adaptor<0x143>, &method_init_adaptor<0x144>, &method_init_adaptor<0x145>, &method_init_adaptor<0x146>, &method_init_adaptor<0x147>,
  &method_init_adaptor<0x148>, &method_init_adaptor<0x149>, &method_init_adaptor<0x14a>, &method_init_adaptor<0x14b>, &method_init_adaptor<0x14c>, &method_init_adaptor<0x14d>, &method_init_adaptor<0x14e>, &method_init_adaptor<0x14f>,
  &method_init_adaptor<0x150>, &method_init_adaptor<0x151>, &method_init_adaptor<0x152>, &method_init_adaptor<0x153>, &method_init_adaptor<0x154>, &method_init_adaptor<0x155>, &method_init_adaptor<0x156>, &method_init_adaptor<0x157>,
  &method_init_adaptor<0x158>, &method_init_adaptor<0x159>, &method_init_adaptor<0x15a>, &method_init_adaptor<0x15b>, &method_init_adaptor<0x15c>, &method_init_adaptor<0x15d>, &method_init_adaptor<0x15e>, &method_init_adaptor<0x15f>,
  &method_init_adaptor<0x160>, &method_init_adaptor<0x161>, &method_init_adaptor<0x162>, &method_init_adaptor<0x163>, &method_init_adaptor<0x164>, &method_init_adaptor<0x165>, &method_init_adaptor<0x166>, &method_init_adaptor<0x167>,
  &method_init_adaptor<0x168>, &method_init_adaptor<0x169>, &method_init_adaptor<0x16a>, &method_init_adaptor<0x16b>, &method_init_adaptor<0x16c>, &method_init_adaptor<0x16d>, &method_init_adaptor<0x16e>, &method_init_adaptor<0x16f>,
  &method_init_adaptor<0x170>, &method_init_adaptor<0x171>, &method_init_adaptor<0x172>, &method_init_adaptor<0x173>, &method_init_adaptor<0x174>, &method_init_adaptor<0x175>, &method_init_adaptor<0x176>, &method_init_adaptor<0x177>,
  &method_init_adaptor<0x178>, &method_init_adaptor<0x179>, &method_init_adaptor<0x17a>, &method_init_adaptor<0x17b>, &method_init_adaptor<0x17c>, &method_init_adaptor<0x17d>, &method_init_adaptor<0x17e>, &method_init_adaptor<0x17f>,
  &method_init_adaptor<0x180>, &method_init_adaptor<0x181>, &method_init_adaptor<0x182>, &method_init_adaptor<0x183>, &method_init_adaptor<0x184>, &method_init_adaptor<0x185>, &method_init_adaptor<0x186>, &method_init_adaptor<0x187>,
  &method_init_adaptor<0x188>, &method_init_adaptor<0x189>, &method_init_adaptor<0x18a>, &method_init_adaptor<0x18b>, &method_init_adaptor<0x18c>, &method_init_adaptor<0x18d>, &method_init_adaptor<0x18e>, &method_init_adaptor<0x18f>,
  &method_init_adaptor<0x190>, &method_init_adaptor<0x191>, &method_init_adaptor<0x192>, &method_init_adaptor<0x193>, &method_init_adaptor<0x194>, &method_init_adaptor<0x195>, &method_init_adaptor<0x196>, &method_init_adaptor<0x197>,
  &method_init_adaptor<0x198>, &method_init_adaptor<0x199>, &method_init_adaptor<0x19a>, &method_init_adaptor<0x19b>, &method_init_adaptor<0x19c>, &method_init_adaptor<0x19d>, &method_init_adaptor<0x19e>, &method_init_adaptor<0x19f>,
  &method_init_adaptor<0x1a0>, &method_init_adaptor<0x1a1>, &method_init_adaptor<0x1a2>, &method_init_adaptor<0x1a3>, &method_init_adaptor<0x1a4>, &method_init_adaptor<0x1a5>, &method_init_adaptor<0x1a6>, &method_init_adaptor<0x1a7>,
  &method_init_adaptor<0x1a8>, &method_init_adaptor<0x1a9>, &method_init_adaptor<0x1aa>, &method_init_adaptor<0x1ab>, &method_init_adaptor<0x1ac>, &method_init_adaptor<0x1ad>, &method_init_adaptor<0x1ae>, &method_init_adaptor<0x1af>,
  &method_init_adaptor<0x1b0>, &method_init_adaptor<0x1b1>, &method_init_adaptor<0x1b2>, &method_init_adaptor<0x1b3>, &method_init_adaptor<0x1b4>, &method_init_adaptor<0x1b5>, &method_init_adaptor<0x1b6>, &method_init_adaptor<0x1b7>,
  &method_init_adaptor<0x1b8>, &method_init_adaptor<0x1b9>, &method_init_adaptor<0x1ba>, &method_init_adaptor<0x1bb>, &method_init_adaptor<0x1bc>, &method_init_adaptor<0x1bd>, &method_init_adaptor<0x1be>, &method_init_adaptor<0x1bf>,
  &method_init_adaptor<0x1c0>, &method_init_adaptor<0x1c1>, &method_init_adaptor<0x1c2>, &method_init_adaptor<0x1c3>, &method_init_adaptor<0x1c4>, &method_init_adaptor<0x1c5>, &method_init_adaptor<0x1c6>, &method_init_adaptor<0x1c7>,
  &method_init_adaptor<0x1c8>, &method_init_adaptor<0x1c9>, &method_init_adaptor<0x1ca>, &method_init_adaptor<0x1cb>, &method_init_adaptor<0x1cc>, &method_init_adaptor<0x1cd>, &method_init_adaptor<0x1ce>, &method_init_adaptor<0x1cf>,
  &method_init_adaptor<0x1d0>, &method_init_adaptor<0x1d1>, &method_init_adaptor<0x1d2>, &method_init_adaptor<0x1d3>, &method_init_adaptor<0x1d4>, &method_init_adaptor<0x1d5>, &method_init_adaptor<0x1d6>, &method_init_adaptor<0x1d7>,
  &method_init_adaptor<0x1d8>, &method_init_adaptor<0x1d9>, &method_init_adaptor<0x1da>, &method_init_adaptor<0x1db>, &method_init_adaptor<0x1dc>, &method_init_adaptor<0x1dd>, &method_init_adaptor<0x1de>, &method_init_adaptor<0x1df>,
  &method_init_adaptor<0x1e0>, &method_init_adaptor<0x1e1>, &method_init_adaptor<0x1e2>, &method_init_adaptor<0x1e3>, &method_init_adaptor<0x1e4>, &method_init_adaptor<0x1e5>, &method_init_adaptor<0x1e6>, &method_init_adaptor<0x1e7>,
  &method_init_adaptor<0x1e8>, &method_init_adaptor<0x1e9>, &method_init_adaptor<0x1ea>, &method_init_adaptor<0x1eb>, &method_init_adaptor<0x1ec>, &method_init_adaptor<0x1ed>, &method_init_adaptor<0x1ee>, &method_init_adaptor<0x1ef>,
  &method_init_adaptor<0x1f0>, &method_init_adaptor<0x1f1>, &method_init_adaptor<0x1f2>, &method_init_adaptor<0x1f3>, &method_init_adaptor<0x1f4>, &method_init_adaptor<0x1f5>, &method_init_adaptor<0x1f6>, &method_init_adaptor<0x1f7>,
  &method_init_adaptor<0x1f8>, &method_init_adaptor<0x1f9>, &method_init_adaptor<0x1fa>, &method_init_adaptor<0x1fb>, &method_init_adaptor<0x1fc>, &method_init_adaptor<0x1fd>, &method_init_adaptor<0x1fe>, &method_init_adaptor<0x1ff>,
  &method_init_adaptor<0x200>, &method_init_adaptor<0x201>, &method_init_adaptor<0x202>, &method_init_adaptor<0x203>, &method_init_adaptor<0x204>, &method_init_adaptor<0x205>, &method_init_adaptor<0x206>, &method_init_adaptor<0x207>,
  &method_init_adaptor<0x208>, &method_init_adaptor<0x209>, &method_init_adaptor<0x20a>, &method_init_adaptor<0x20b>, &method_init_adaptor<0x20c>, &method_init_adaptor<0x20d>, &method_init_adaptor<0x20e>, &method_init_adaptor<0x20f>,
  &method_init_adaptor<0x210>, &method_init_adaptor<0x211>, &method_init_adaptor<0x212>, &method_init_adaptor<0x213>, &method_init_adaptor<0x214>, &method_init_adaptor<0x215>, &method_init_adaptor<0x216>, &method_init_adaptor<0x217>,
  &method_init_adaptor<0x218>, &method_init_adaptor<0x219>, &method_init_adaptor<0x21a>, &method_init_adaptor<0x21b>, &method_init_adaptor<0x21c>, &method_init_adaptor<0x21d>, &method_init_adaptor<0x21e>, &method_init_adaptor<0x21f>,
  &method_init_adaptor<0x220>, &method_init_adaptor<0x221>, &method_init_adaptor<0x222>, &method_init_adaptor<0x223>, &method_init_adaptor<0x224>, &method_init_adaptor<0x225>, &method_init_adaptor<0x226>, &method_init_adaptor<0x227>,
  &method_init_adaptor<0x228>, &method_init_adaptor<0x229>, &method_init_adaptor<0x22a>, &method_init_adaptor<0x22b>, &method_init_adaptor<0x22c>, &method_init_adaptor<0x22d>, &method_init_adaptor<0x22e>, &method_init_adaptor<0x22f>,
  &method_init_adaptor<0x230>, &method_init_adaptor<0x231>, &method_init_adaptor<0x232>, &method_init_adaptor<0x233>, &method_init_adaptor<0x234>, &method_init_adaptor<0x235>, &method_init_adaptor<0x236>, &method_init_adaptor<0x237>,
  &method_init_adaptor<0x238>, &method_init_adaptor<0x239>, &method_init_adaptor<0x23a>, &method_init_adaptor<0x23b>, &method_init_adaptor<0x23c>, &method_init_adaptor<0x23d>, &method_init_adaptor<0x23e>, &method_init_adaptor<0x23f>,
  &method_init_adaptor<0x240>, &method_init_adaptor<0x241>, &method_init_adaptor<0x242>, &method_init_adaptor<0x243>, &method_init_adaptor<0x244>, &method_init_adaptor<0x245>, &method_init_adaptor<0x246>, &method_init_adaptor<0x247>,
  &method_init_adaptor<0x248>, &method_init_adaptor<0x249>, &method_init_adaptor<0x24a>, &method_init_adaptor<0x24b>, &method_init_adaptor<0x24c>, &method_init_adaptor<0x24d>, &method_init_adaptor<0x24e>, &method_init_adaptor<0x24f>,
  &method_init_adaptor<0x250>, &method_init_adaptor<0x251>, &method_init_adaptor<0x252>, &method_init_adaptor<0x253>, &method_init_adaptor<0x254>, &method_init_adaptor<0x255>, &method_init_adaptor<0x256>, &method_init_adaptor<0x257>,
  &method_init_adaptor<0x258>, &method_init_adaptor<0x259>, &method_init_adaptor<0x25a>, &method_init_adaptor<0x25b>, &method_init_adaptor<0x25c>, &method_init_adaptor<0x25d>, &method_init_adaptor<0x25e>, &method_init_adaptor<0x25f>,
  &method_init_adaptor<0x260>, &method_init_adaptor<0x261>, &method_init_adaptor<0x262>, &method_init_adaptor<0x263>, &method_init_adaptor<0x264>, &method_init_adaptor<0x265>, &method_init_adaptor<0x266>, &method_init_adaptor<0x267>,
  &method_init_adaptor<0x268>, &method_init_adaptor<0x269>, &method_init_adaptor<0x26a>, &method_init_adaptor<0x26b>, &method_init_adaptor<0x26c>, &method_init_adaptor<0x26d>, &method_init_adaptor<0x26e>, &method_init_adaptor<0x26f>,
  &method_init_adaptor<0x270>, &method_init_adaptor<0x271>, &method_init_adaptor<0x272>, &method_init_adaptor<0x273>, &method_init_adaptor<0x274>, &method_init_adaptor<0x275>, &method_init_adaptor<0x276>, &method_init_adaptor<0x277>,
  &method_init_adaptor<0x278>, &method_init_adaptor<0x279>, &method_init_adaptor<0x27a>, &method_init_adaptor<0x27b>, &method_init_adaptor<0x27c>, &method_init_adaptor<0x27d>, &method_init_adaptor<0x27e>, &method_init_adaptor<0x27f>,
  &method_init_adaptor<0x280>, &method_init_adaptor<0x281>, &method_init_adaptor<0x282>, &method_init_adaptor<0x283>, &method_init_adaptor<0x284>, &method_init_adaptor<0x285>, &method_init_adaptor<0x286>, &method_init_adaptor<0x287>,
  &method_init_adaptor<0x288>, &method_init_adaptor<0x289>, &method_init_adaptor<0x28a>, &method_init_adaptor<0x28b>, &method_init_adaptor<0x28c>, &method_init_adaptor<0x28d>, &method_init_adaptor<0x28e>, &method_init_adaptor<0x28f>,
  &method_init_adaptor<0x290>, &method_init_adaptor<0x291>, &method_init_adaptor<0x292>, &method_init_adaptor<0x293>, &method_init_adaptor<0x294>, &method_init_adaptor<0x295>, &method_init_adaptor<0x296>, &method_init_adaptor<0x297>,
  &method_init_adaptor<0x298>, &method_init_adaptor<0x299>, &method_init_adaptor<0x29a>, &method_init_adaptor<0x29b>, &method_init_adaptor<0x29c>, &method_init_adaptor<0x29d>, &method_init_adaptor<0x29e>, &method_init_adaptor<0x29f>,
  &method_init_adaptor<0x2a0>, &method_init_adaptor<0x2a1>, &method_init_adaptor<0x2a2>, &method_init_adaptor<0x2a3>, &method_init_adaptor<0x2a4>, &method_init_adaptor<0x2a5>, &method_init_adaptor<0x2a6>, &method_init_adaptor<0x2a7>,
  &method_init_adaptor<0x2a8>, &method_init_adaptor<0x2a9>, &method_init_adaptor<0x2aa>, &method_init_adaptor<0x2ab>, &method_init_adaptor<0x2ac>, &method_init_adaptor<0x2ad>, &method_init_adaptor<0x2ae>, &method_init_adaptor<0x2af>,
  &method_init_adaptor<0x2b0>, &method_init_adaptor<0x2b1>, &method_init_adaptor<0x2b2>, &method_init_adaptor<0x2b3>, &method_init_adaptor<0x2b4>, &method_init_adaptor<0x2b5>, &method_init_adaptor<0x2b6>, &method_init_adaptor<0x2b7>,
  &method_init_adaptor<0x2b8>, &method_init_adaptor<0x2b9>, &method_init_adaptor<0x2ba>, &method_init_adaptor<0x2bb>, &method_init_adaptor<0x2bc>, &method_init_adaptor<0x2bd>, &method_init_adaptor<0x2be>, &method_init_adaptor<0x2bf>,
  &method_init_adaptor<0x2c0>, &method_init_adaptor<0x2c1>, &method_init_adaptor<0x2c2>, &method_init_adaptor<0x2c3>, &method_init_adaptor<0x2c4>, &method_init_adaptor<0x2c5>, &method_init_adaptor<0x2c6>, &method_init_adaptor<0x2c7>,
  &method_init_adaptor<0x2c8>, &method_init_adaptor<0x2c9>, &method_init_adaptor<0x2ca>, &method_init_adaptor<0x2cb>, &method_init_adaptor<0x2cc>, &method_init_adaptor<0x2cd>, &method_init_adaptor<0x2ce>, &method_init_adaptor<0x2cf>,
  &method_init_adaptor<0x2d0>, &method_init_adaptor<0x2d1>, &method_init_adaptor<0x2d2>, &method_init_adaptor<0x2d3>, &method_init_adaptor<0x2d4>, &method_init_adaptor<0x2d5>, &method_init_adaptor<0x2d6>, &method_init_adaptor<0x2d7>,
  &method_init_adaptor<0x2d8>, &method_init_adaptor<0x2d9>, &method_init_adaptor<0x2da>, &method_init_adaptor<0x2db>, &method_init_adaptor<0x2dc>, &method_init_adaptor<0x2dd>, &method_init_adaptor<0x2de>, &method_init_adaptor<0x2df>,
  &method_init_adaptor<0x2e0>, &method_init_adaptor<0x2e1>, &method_init_adaptor<0x2e2>, &method_init_adaptor<0x2e3>, &method_init_adaptor<0x2e4>, &method_init_adaptor<0x2e5>, &method_init_adaptor<0x2e6>, &method_init_adaptor<0x2e7>,
  &method_init_adaptor<0x2e8>, &method_init_adaptor<0x2e9>, &method_init_adaptor<0x2ea>, &method_init_adaptor<0x2eb>, &method_init_adaptor<0x2ec>, &method_init_adaptor<0x2ed>, &method_init_adaptor<0x2ee>, &method_init_adaptor<0x2ef>,
  &method_init_adaptor<0x2f0>, &method_init_adaptor<0x2f1>, &method_init_adaptor<0x2f2>, &method_init_adaptor<0x2f3>, &method_init_adaptor<0x2f4>, &method_init_adaptor<0x2f5>, &method_init_adaptor<0x2f6>, &method_init_adaptor<0x2f7>,
  &method_init_adaptor<0x2f8>, &method_init_adaptor<0x2f9>, &method_init_adaptor<0x2fa>, &method_init_adaptor<0x2fb>, &method_init_adaptor<0x2fc>, &method_init_adaptor<0x2fd>, &method_init_adaptor<0x2fe>, &method_init_adaptor<0x2ff>,
  &method_init_adaptor<0x300>, &method_init_adaptor<0x301>, &method_init_adaptor<0x302>, &method_init_adaptor<0x303>, &method_init_adaptor<0x304>, &method_init_adaptor<0x305>, &method_init_adaptor<0x306>, &method_init_adaptor<0x307>,
  &method_init_adaptor<0x308>, &method_init_adaptor<0x309>, &method_init_adaptor<0x30a>, &method_init_adaptor<0x30b>, &method_init_adaptor<0x30c>, &method_init_adaptor<0x30d>, &method_init_adaptor<0x30e>, &method_init_adaptor<0x30f>,
  &method_init_adaptor<0x310>, &method_init_adaptor<0x311>, &method_init_adaptor<0x312>, &method_init_adaptor<0x313>, &method_init_adaptor<0x314>, &method_init_adaptor<0x315>, &method_init_adaptor<0x316>, &method_init_adaptor<0x317>,
  &method_init_adaptor<0x318>, &method_init_adaptor<0x319>, &method_init_adaptor<0x31a>, &method_init_adaptor<0x31b>, &method_init_adaptor<0x31c>, &method_init_adaptor<0x31d>, &method_init_adaptor<0x31e>, &method_init_adaptor<0x31f>,
  &method_init_adaptor<0x320>, &method_init_adaptor<0x321>, &method_init_adaptor<0x322>, &method_init_adaptor<0x323>, &method_init_adaptor<0x324>, &method_init_adaptor<0x325>, &method_init_adaptor<0x326>, &method_init_adaptor<0x327>,
  &method_init_adaptor<0x328>, &method_init_adaptor<0x329>, &method_init_adaptor<0x32a>, &method_init_adaptor<0x32b>, &method_init_adaptor<0x32c>, &method_init_adaptor<0x32d>, &method_init_adaptor<0x32e>, &method_init_adaptor<0x32f>,
  &method_init_adaptor<0x330>, &method_init_adaptor<0x331>, &method_init_adaptor<0x332>, &method_init_adaptor<0x333>, &method_init_adaptor<0x334>, &method_init_adaptor<0x335>, &method_init_adaptor<0x336>, &method_init_adaptor<0x337>,
  &method_init_adaptor<0x338>, &method_init_adaptor<0x339>, &method_init_adaptor<0x33a>, &method_init_adaptor<0x33b>, &method_init_adaptor<0x33c>, &method_init_adaptor<0x33d>, &method_init_adaptor<0x33e>, &method_init_adaptor<0x33f>,
  &method_init_adaptor<0x340>, &method_init_adaptor<0x341>, &method_init_adaptor<0x342>, &method_init_adaptor<0x343>, &method_init_adaptor<0x344>, &method_init_adaptor<0x345>, &method_init_adaptor<0x346>, &method_init_adaptor<0x347>,
  &method_init_adaptor<0x348>, &method_init_adaptor<0x349>, &method_init_adaptor<0x34a>, &method_init_adaptor<0x34b>, &method_init_adaptor<0x34c>, &method_init_adaptor<0x34d>, &method_init_adaptor<0x34e>, &method_init_adaptor<0x34f>,
  &method_init_adaptor<0x350>, &method_init_adaptor<0x351>, &method_init_adaptor<0x352>, &method_init_adaptor<0x353>, &method_init_adaptor<0x354>, &method_init_adaptor<0x355>, &method_init_adaptor<0x356>, &method_init_adaptor<0x357>,
  &method_init_adaptor<0x358>, &method_init_adaptor<0x359>, &method_init_adaptor<0x35a>, &method_init_adaptor<0x35b>, &method_init_adaptor<0x35c>, &method_init_adaptor<0x35d>, &method_init_adaptor<0x35e>, &method_init_adaptor<0x35f>,
  &method_init_adaptor<0x360>, &method_init_adaptor<0x361>, &method_init_adaptor<0x362>, &method_init_adaptor<0x363>, &method_init_adaptor<0x364>, &method_init_adaptor<0x365>, &method_init_adaptor<0x366>, &method_init_adaptor<0x367>,
  &method_init_adaptor<0x368>, &method_init_adaptor<0x369>, &method_init_adaptor<0x36a>, &method_init_adaptor<0x36b>, &method_init_adaptor<0x36c>, &method_init_adaptor<0x36d>, &method_init_adaptor<0x36e>, &method_init_adaptor<0x36f>,
  &method_init_adaptor<0x370>, &method_init_adaptor<0x371>, &method_init_adaptor<0x372>, &method_init_adaptor<0x373>, &method_init_adaptor<0x374>, &method_init_adaptor<0x375>, &method_init_adaptor<0x376>, &method_init_adaptor<0x377>,
  &method_init_adaptor<0x378>, &method_init_adaptor<0x379>, &method_init_adaptor<0x37a>, &method_init_adaptor<0x37b>, &method_init_adaptor<0x37c>, &method_init_adaptor<0x37d>, &method_init_adaptor<0x37e>, &method_init_adaptor<0x37f>,
  &method_init_adaptor<0x380>, &method_init_adaptor<0x381>, &method_init_adaptor<0x382>, &method_init_adaptor<0x383>, &method_init_adaptor<0x384>, &method_init_adaptor<0x385>, &method_init_adaptor<0x386>, &method_init_adaptor<0x387>,
  &method_init_adaptor<0x388>, &method_init_adaptor<0x389>, &method_init_adaptor<0x38a>, &method_init_adaptor<0x38b>, &method_init_adaptor<0x38c>, &method_init_adaptor<0x38d>, &method_init_adaptor<0x38e>, &method_init_adaptor<0x38f>,
  &method_init_adaptor<0x390>, &method_init_adaptor<0x391>, &method_init_adaptor<0x392>, &method_init_adaptor<0x393>, &method_init_adaptor<0x394>, &method_init_adaptor<0x395>, &method_init_adaptor<0x396>, &method_init_adaptor<0x397>,
  &method_init_adaptor<0x398>, &method_init_adaptor<0x399>, &method_init_adaptor<0x39a>, &method_init_adaptor<0x39b>, &method_init_adaptor<0x39c>, &method_init_adaptor<0x39d>, &method_init_adaptor<0x39e>, &method_init_adaptor<0x39f>,
  &method_init_adaptor<0x3a0>, &method_init_adaptor<0x3a1>, &method_init_adaptor<0x3a2>, &method_init_adaptor<0x3a3>, &method_init_adaptor<0x3a4>, &method_init_adaptor<0x3a5>, &method_init_adaptor<0x3a6>, &method_init_adaptor<0x3a7>,
  &method_init_adaptor<0x3a8>, &method_init_adaptor<0x3a9>, &method_init_adaptor<0x3aa>, &method_init_adaptor<0x3ab>, &method_init_adaptor<0x3ac>, &method_init_adaptor<0x3ad>, &method_init_adaptor<0x3ae>, &method_init_adaptor<0x3af>,
  &method_init_adaptor<0x3b0>, &method_init_adaptor<0x3b1>, &method_init_adaptor<0x3b2>, &method_init_adaptor<0x3b3>, &method_init_adaptor<0x3b4>, &method_init_adaptor<0x3b5>, &method_init_adaptor<0x3b6>, &method_init_adaptor<0x3b7>,
  &method_init_adaptor<0x3b8>, &method_init_adaptor<0x3b9>, &method_init_adaptor<0x3ba>, &method_init_adaptor<0x3bb>, &method_init_adaptor<0x3bc>, &method_init_adaptor<0x3bd>, &method_init_adaptor<0x3be>, &method_init_adaptor<0x3bf>,
  &method_init_adaptor<0x3c0>, &method_init_adaptor<0x3c1>, &method_init_adaptor<0x3c2>, &method_init_adaptor<0x3c3>, &method_init_adaptor<0x3c4>, &method_init_adaptor<0x3c5>, &method_init_adaptor<0x3c6>, &method_init_adaptor<0x3c7>,
  &method_init_adaptor<0x3c8>, &method_init_adaptor<0x3c9>, &method_init_adaptor<0x3ca>, &method_init_adaptor<0x3cb>, &method_init_adaptor<0x3cc>, &method_init_adaptor<0x3cd>, &method_init_adaptor<0x3ce>, &method_init_adaptor<0x3cf>,
  &method_init_adaptor<0x3d0>, &method_init_adaptor<0x3d1>, &method_init_adaptor<0x3d2>, &method_init_adaptor<0x3d3>, &method_init_adaptor<0x3d4>, &method_init_adaptor<0x3d5>, &method_init_adaptor<0x3d6>, &method_init_adaptor<0x3d7>,
  &method_init_adaptor<0x3d8>, &method_init_adaptor<0x3d9>, &method_init_adaptor<0x3da>, &method_init_adaptor<0x3db>, &method_init_adaptor<0x3dc>, &method_init_adaptor<0x3dd>, &method_init_adaptor<0x3de>, &method_init_adaptor<0x3df>,
  &method_init_adaptor<0x3e0>, &method_init_adaptor<0x3e1>, &method_init_adaptor<0x3e2>, &method_init_adaptor<0x3e3>, &method_init_adaptor<0x3e4>, &method_init_adaptor<0x3e5>, &method_init_adaptor<0x3e6>, &method_init_adaptor<0x3e7>,
  &method_init_adaptor<0x3e8>, &method_init_adaptor<0x3e9>, &method_init_adaptor<0x3ea>, &method_init_adaptor<0x3eb>, &method_init_adaptor<0x3ec>, &method_init_adaptor<0x3ed>, &method_init_adaptor<0x3ee>, &method_init_adaptor<0x3ef>,
  &method_init_adaptor<0x3f0>, &method_init_adaptor<0x3f1>, &method_init_adaptor<0x3f2>, &method_init_adaptor<0x3f3>, &method_init_adaptor<0x3f4>, &method_init_adaptor<0x3f5>, &method_init_adaptor<0x3f6>, &method_init_adaptor<0x3f7>,
  &method_init_adaptor<0x3f8>, &method_init_adaptor<0x3f9>, &method_init_adaptor<0x3fa>, &method_init_adaptor<0x3fb>, &method_init_adaptor<0x3fc>, &method_init_adaptor<0x3fd>, &method_init_adaptor<0x3fe>, &method_init_adaptor<0x3ff>,
};

py_func_ptr_t get_method_init_adaptor (int n)
{
  tl_assert (n >= 0 && n < int (sizeof (method_init_adaptors) / sizeof (method_init_adaptors [0])));
  return method_init_adaptors [n];
}

}

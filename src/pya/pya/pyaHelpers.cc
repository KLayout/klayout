
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


#include "pyaHelpers.h"
#include "pyaUtils.h"
#include "pyaMarshal.h"
#include "pyaObject.h"
#include "pyaConvert.h"
#include "pyaSignalHandler.h"
#include "pya.h"

namespace pya
{

// -------------------------------------------------------------------
//  Helper objects: PYAChannelObject

PyTypeObject *PYAChannelObject::cls = 0;

/**
 *  @brief Implementation of the write method of the channel object
 */
static PyObject *
pya_channel_write (PyObject *self, PyObject *args)
{
  const char *msg = 0;
  if (! PyArg_ParseTuple (args, "s", &msg)) {
    return NULL;
  }

  PYAChannelObject *channel = (PYAChannelObject *) self;
  if (PythonInterpreter::instance () && PythonInterpreter::instance ()->current_console ()) {
    PythonInterpreter::instance ()->current_console ()->write_str (msg, channel->channel);
  }

  Py_RETURN_NONE;
}

/**
 *  @brief Implementation of the flush method of the channel object
 */
static PyObject *
pya_channel_flush (PyObject * /*self*/, PyObject *args)
{
  if (! PyArg_ParseTuple (args, "")) {
    return NULL;
  }

  if (PythonInterpreter::instance () && PythonInterpreter::instance ()->current_console ()) {
    PythonInterpreter::instance ()->current_console ()->flush ();
  }

  Py_RETURN_NONE;
}

/**
 *  @brief Implementation of the isatty method of the channel object
 */
static PyObject *
pya_channel_isatty (PyObject * /*self*/, PyObject *args)
{
  if (! PyArg_ParseTuple (args, "")) {
    return NULL;
  }

  if (PythonInterpreter::instance () && PythonInterpreter::instance ()->current_console () && PythonInterpreter::instance ()->current_console ()->is_tty ()) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

/**
 *  @brief Initialization of the channel object
 */
static int
pya_channel_init (PyObject *self, PyObject *, PyObject *)
{
  PYAChannelObject *channel = (PYAChannelObject *) self;
  channel->channel = gsi::Console::OS_none;
  return 0;
}

void
PYAChannelObject::make_class (PyObject *module)
{
  static PyTypeObject channel_type = {
    PyVarObject_HEAD_INIT (&PyType_Type, 0)
    "pya._Channel",             // tp_name
    sizeof (PYAChannelObject)   // tp_size
  };

  static PyMethodDef channel_methods[] = {
      {"write", (PyCFunction) &pya_channel_write, METH_VARARGS, "internal stdout/stderr redirection object: write method" },
      {"flush", (PyCFunction) &pya_channel_flush, METH_VARARGS, "internal stdout/stderr redirection object: flush method" },
      {"isatty", (PyCFunction) &pya_channel_isatty, METH_VARARGS, "internal stdout/stderr redirection object: isatty method" },
      {NULL,  NULL},
  };

  channel_type.tp_flags = Py_TPFLAGS_DEFAULT;
  channel_type.tp_methods = channel_methods;
  channel_type.tp_init = pya_channel_init;

  PyType_Ready (&channel_type);
  Py_INCREF (&channel_type);

  PyModule_AddObject (module, "_Channel", (PyObject *) &channel_type);

  cls = &channel_type;
}

PYAChannelObject *
PYAChannelObject::create (gsi::Console::output_stream chn)
{
  tl_assert (cls != 0);
  PYAChannelObject *channel = (PYAChannelObject *) cls->tp_alloc (cls, 0);
  if (channel == NULL) {
    check_error ();
  } else {
    channel->channel = chn;
  }
  return channel;
}

// -------------------------------------------------------------------
//  Helper objects: PYAStaticAttributeDescriptorObject

PyTypeObject *PYAStaticAttributeDescriptorObject::cls = 0;

/**
 *  @brief Implementation of the static attribute getter
 */
static PyObject *
pya_static_attribute_descriptor_get (PyObject *self, PyObject * /*obj*/, PyObject * /*type*/)
{
  PYAStaticAttributeDescriptorObject *attr = (PYAStaticAttributeDescriptorObject *) self;
  if (attr->getter) {
    return (*(attr->getter)) ((PyObject *) attr->type, NULL);
  } else {
    std::string msg;
    msg += tl::to_string (tr ("Attribute not readable"));
    msg += ": ";
    msg += attr->type->tp_name;
    msg += ".";
    msg += attr->name;
    PyErr_SetString (PyExc_AttributeError, msg.c_str ());
    return NULL;
  }
}

/**
 *  @brief Implementation of the static attribute setter
 */
static int
pya_static_attribute_descriptor_set (PyObject *self, PyObject * /*obj*/, PyObject *value)
{
  PYAStaticAttributeDescriptorObject *attr = (PYAStaticAttributeDescriptorObject *) self;
  if (attr->setter) {
    PythonRef args (PyTuple_Pack (1, value));
    PyObject *ret = (*(attr->setter)) ((PyObject *) attr->type, args.get ());
    if (ret) {
      Py_DECREF(ret);
      return 0;
    } else {
      return -1;
    }
  } else {
    std::string msg;
    msg += tl::to_string (tr ("Attribute cannot be changed"));
    msg += ": ";
    msg += attr->type->tp_name;
    msg += ".";
    msg += attr->name;
    PyErr_SetString (PyExc_AttributeError, msg.c_str ());
    return -1;
  }
}

/**
 *  @brief Initialization of the static attribute object
 */
static int
pya_static_attribute_descriptor_init (PyObject *self, PyObject *, PyObject *)
{
  PYAStaticAttributeDescriptorObject *attr = (PYAStaticAttributeDescriptorObject *) self;
  attr->getter = 0;
  attr->setter = 0;
  attr->name = 0;
  attr->type = 0;
  return 0;
}

void
PYAStaticAttributeDescriptorObject::make_class (PyObject *module)
{
  static PyTypeObject static_attribute_descriptor_type = {
    PyVarObject_HEAD_INIT (&PyType_Type, 0)
    "pya._StaticAttribute",                       // tp_name
    sizeof (PYAStaticAttributeDescriptorObject)   // tp_size
  };

  static_attribute_descriptor_type.tp_flags = Py_TPFLAGS_DEFAULT;
  static_attribute_descriptor_type.tp_init = pya_static_attribute_descriptor_init;
  static_attribute_descriptor_type.tp_descr_get = pya_static_attribute_descriptor_get;
  static_attribute_descriptor_type.tp_descr_set = pya_static_attribute_descriptor_set;
  static_attribute_descriptor_type.tp_setattro = PyObject_GenericSetAttr;
  static_attribute_descriptor_type.tp_getattro = PyObject_GenericGetAttr;

  PyType_Ready (&static_attribute_descriptor_type);
  Py_INCREF (&static_attribute_descriptor_type);

  PyModule_AddObject (module, "_StaticAttribute", (PyObject *) &static_attribute_descriptor_type);

  cls = &static_attribute_descriptor_type;
}

PYAStaticAttributeDescriptorObject *
PYAStaticAttributeDescriptorObject::create (const char *n)
{
  tl_assert (cls != 0);
  PYAStaticAttributeDescriptorObject *desc = (PYAStaticAttributeDescriptorObject *) cls->tp_alloc (cls, 0);
  if (desc == NULL) {
    check_error ();
  } else {
    desc->name = n;
  }
  return desc;
}

// -------------------------------------------------------------------
//  Helper objects: PYAAmbiguousMethodDispatcher

PyTypeObject *PYAAmbiguousMethodDispatcher::cls = 0;

static PyObject *
pya_ambiguous_method_dispatcher_get (PyObject *self, PyObject *obj, PyObject *type)
{
  PYAAmbiguousMethodDispatcher *attr = (PYAAmbiguousMethodDispatcher *) self;
  PyObject *descr;
  if (obj == NULL || obj == Py_None) {
    descr = attr->attr_class;
  } else {
    descr = attr->attr_inst;
  }

  //  taken from object.c, PyObject_GenericGetAttrWithDict
#if PY_MAJOR_VERSION < 3
  tl_assert (PyType_HasFeature (Py_TYPE (descr), Py_TPFLAGS_HAVE_CLASS)); 
#endif
  descrgetfunc f = Py_TYPE (descr)->tp_descr_get;
  if (f == NULL) {
    Py_INCREF (descr);
    return descr;
  } else {
    return (*f) (descr, obj, type);
  }
}

static int
pya_ambiguous_method_dispatcher_set (PyObject * /*self*/, PyObject * /*obj*/, PyObject * /*value*/)
{
  PyErr_SetNone (PyExc_AttributeError);
  return -1;
}

static void
pya_ambiguous_method_dispatcher_deallocate (PyObject *self)
{
  PYAAmbiguousMethodDispatcher *attr = (PYAAmbiguousMethodDispatcher *) self;
  Py_XDECREF (attr->attr_inst);
  Py_XDECREF (attr->attr_class);
  Py_TYPE (self)->tp_free ((PyObject *) self);
}

void
PYAAmbiguousMethodDispatcher::make_class (PyObject *module)
{
  static PyTypeObject static_ambiguous_method_dispatcher_type = {
    PyVarObject_HEAD_INIT (&PyType_Type, 0)
    "pya._AmbiguousMethodDispatcher",       // tp_name
    sizeof (PYAAmbiguousMethodDispatcher)   // tp_size
  };

  static_ambiguous_method_dispatcher_type.tp_flags = Py_TPFLAGS_DEFAULT;
  static_ambiguous_method_dispatcher_type.tp_dealloc = pya_ambiguous_method_dispatcher_deallocate;
  static_ambiguous_method_dispatcher_type.tp_descr_get = pya_ambiguous_method_dispatcher_get;
  static_ambiguous_method_dispatcher_type.tp_descr_set = pya_ambiguous_method_dispatcher_set;
  static_ambiguous_method_dispatcher_type.tp_setattro = PyObject_GenericSetAttr;
  static_ambiguous_method_dispatcher_type.tp_getattro = PyObject_GenericGetAttr;

  PyType_Ready (&static_ambiguous_method_dispatcher_type);
  Py_INCREF (&static_ambiguous_method_dispatcher_type);

  PyModule_AddObject (module, "_AmbiguousMethodDispatcher", (PyObject *) &static_ambiguous_method_dispatcher_type);

  cls = &static_ambiguous_method_dispatcher_type;
}

PYAAmbiguousMethodDispatcher *
PYAAmbiguousMethodDispatcher::create (PyObject *ai, PyObject *ac)
{
  tl_assert (cls != 0);
  PYAAmbiguousMethodDispatcher* desc = (PYAAmbiguousMethodDispatcher *) cls->tp_alloc (cls, 0);
  if (desc == NULL) {
    Py_XDECREF (ai);
    Py_XDECREF (ac);
    check_error ();
  } else {
    desc->attr_inst = ai;
    desc->attr_class = ac;
  }
  return desc;
}


// -------------------------------------------------------------------
//  Helper objects: PYAIteratorObject

PyTypeObject *PYAIteratorObject::cls = 0;

/**
 *  @brief Gets the iterator object (reflective)
 */
static PyObject *
pya_plain_iterator_iter (PyObject *self)
{
  //  we have to return a new reference -> when using self, we have to increment out count
  Py_INCREF (self);
  return self;
}

/**
 *  @brief Increments the iterator
 */
static PyObject *
pya_plain_iterator_next (PyObject *self)
{
  PYAIteratorObject *iter = (PYAIteratorObject *) self;

  if (! iter->iter) {
    PyErr_SetNone (PyExc_StopIteration);
    return NULL;
  }

  //  increment except on first visit
  if (! iter->first) {
    iter->iter->inc ();
  }
  iter->first = false;

  if (iter->iter->at_end ()) {
    PyErr_SetNone (PyExc_StopIteration);
    return NULL;
  }

  //  TODO: what to do with the heap here?
  tl::Heap heap;

  gsi::SerialArgs args (iter->iter->serial_size ());
  iter->iter->get (args);
  PythonRef obj = pop_arg (*iter->value_type, args, 0, heap);

  return obj.release ();
}

static void
pya_plain_iterator_deallocate (PyObject *self)
{
  PYAIteratorObject *p = (PYAIteratorObject *) self;
  if (p->origin) {
    Py_DECREF (p->origin);
    p->origin = 0;
  }
  if (p->iter) {
    delete p->iter;
    p->iter = 0;
  }
  Py_TYPE (self)->tp_free ((PyObject *) self);
}

void
PYAIteratorObject::make_class (PyObject *module)
{
  static PyTypeObject iterator_type = {
    PyVarObject_HEAD_INIT (&PyType_Type, 0)
    "pya._Iterator",            // tp_name
    sizeof (PYAIteratorObject)  // tp_size
  };

  iterator_type.tp_flags = Py_TPFLAGS_DEFAULT;
  iterator_type.tp_dealloc = pya_plain_iterator_deallocate;
  iterator_type.tp_iter = pya_plain_iterator_iter;
  iterator_type.tp_iternext = pya_plain_iterator_next;

  PyType_Ready (&iterator_type);
  Py_INCREF (&iterator_type);

  PyModule_AddObject (module, "_Iterator", (PyObject *) &iterator_type);

  cls = &iterator_type;
}

PYAIteratorObject *
PYAIteratorObject::create (PyObject *origin, gsi::IterAdaptorAbstractBase *iter, const gsi::ArgType *value_type)
{
  tl_assert (cls != 0);
  PYAIteratorObject *iter_obj = (PYAIteratorObject *) cls->tp_alloc (cls, 0);
  if (iter_obj == NULL) {
    check_error ();
  } else {
    if (origin) {
      //  The iterator will keep a reference to the origin object of the iterator
      Py_INCREF (origin);
    }
    iter_obj->origin = origin;
    iter_obj->iter = iter;
    iter_obj->value_type = value_type;
    iter_obj->first = true;
  }
  return iter_obj;
}

// -------------------------------------------------------------------
//  Helper objects: PYASignal

PyTypeObject *PYASignal::cls = 0;

/**
 *  @brief Adds a callable to the signal
 */
static PyObject *
pya_signal_add (PyObject *self, PyObject *args)
{
  PyObject *callable = 0;
  if (! PyArg_ParseTuple (args, "O", &callable)) {
    return NULL;
  }

  if (! PyCallable_Check (callable)) {
    std::string msg;
    msg += tl::to_string (tr ("Signal's += operator needs a callable object"));
    PyErr_SetString (PyExc_AttributeError, msg.c_str ());
    return NULL;
  }

  PYASignal *signal = (PYASignal *) self;
  if (signal->handler) {
    signal->handler->add (callable);
  }

  Py_RETURN_NONE;
}

static PyObject *
pya_signal_inplace_add (PyObject *self, PyObject *callable)
{
  if (! PyCallable_Check (callable)) {
    std::string msg;
    msg += tl::to_string (tr ("Signal's += operator needs a callable object"));
    PyErr_SetString (PyExc_AttributeError, msg.c_str ());
    return NULL;
  }

  PYASignal *signal = (PYASignal *) self;
  if (signal->handler) {
    signal->handler->add (callable);
  }

  //  inplace_add requires to create a copy of the object
  return PYASignal::create (signal->origin, signal->handler.get ());
}

/**
 *  @brief Removes a callable from the signal
 */
static PyObject *
pya_signal_remove (PyObject *self, PyObject *args)
{
  PyObject *callable = 0;
  if (! PyArg_ParseTuple (args, "O", &callable)) {
    return NULL;
  }

  if (! PyCallable_Check (callable)) {
    std::string msg;
    msg += tl::to_string (tr ("Signal's -= operator needs a callable object"));
    PyErr_SetString (PyExc_AttributeError, msg.c_str ());
    return NULL;
  }

  PYASignal *signal = (PYASignal *) self;
  if (signal->handler) {
    signal->handler->remove (callable);
  }

  Py_RETURN_NONE;
}

/**
 *  @brief Removes a callable from the signal
 */
static PyObject *
pya_signal_inplace_remove (PyObject *self, PyObject *callable)
{
  if (! PyCallable_Check (callable)) {
    std::string msg;
    msg += tl::to_string (tr ("Signal's -= operator needs a callable object"));
    PyErr_SetString (PyExc_AttributeError, msg.c_str ());
    return NULL;
  }

  PYASignal *signal = (PYASignal *) self;
  if (signal->handler) {
    signal->handler->remove (callable);
  }

  //  inplace_subtract requires to create a copy of the object
  return PYASignal::create (signal->origin, signal->handler.get ());
}

/**
 *  @brief Makes the callable the only receiver of the signal
 */
static PyObject *
pya_signal_set (PyObject *self, PyObject *args)
{
  PyObject *callable = 0;
  if (! PyArg_ParseTuple (args, "O", &callable)) {
    return NULL;
  }

  if (! PyCallable_Check (callable)) {
    std::string msg;
    msg += tl::to_string (tr ("Signal's 'set' method needs a callable object"));
    PyErr_SetString (PyExc_AttributeError, msg.c_str ());
    return NULL;
  }

  PYASignal *signal = (PYASignal *) self;
  if (signal->handler) {
    signal->handler->clear ();
    signal->handler->add (callable);
  }

  Py_RETURN_NONE;
}

/**
 *  @brief __call__ implementation
 *  Calling a signal is equivalent to setting it (for backward compatibility:
 *  for establishing a signal handler, adding it is the better solution).
 */
static PyObject *
pya_signal_call (PyObject *self, PyObject *args, PyObject * /*kw*/)
{
  return pya_signal_set (self, args);
}

/**
 *  @brief Clears the list of callables
 */
static PyObject *
pya_signal_clear (PyObject *self, PyObject * /*args*/)
{
  PYASignal *signal = (PYASignal *) self;
  if (signal->handler) {
    signal->handler->clear ();
  }

  Py_RETURN_NONE;
}

static void
pya_signal_deallocate (PyObject *self)
{
  PYASignal *p = (PYASignal *) self;
  p->~PYASignal ();
  Py_TYPE (self)->tp_free ((PyObject *) self);
}

PYASignal::PYASignal (PyObject *_origin, pya::SignalHandler *_handler)
{
  if (_origin) {
    //  The iterator will keep a reference to the origin object of the iterator
    Py_INCREF (_origin);
  }
  origin = _origin;
  handler.reset (_handler);
}

PYASignal::~PYASignal ()
{
  if (origin) {
    Py_DECREF (origin);
    origin = 0;
  }
}

void
PYASignal::make_class (PyObject *module)
{
  static PyTypeObject signal_type = {
    PyVarObject_HEAD_INIT (&PyType_Type, 0)
    "pya._Signal",      // tp_name
    sizeof (PYASignal)  // tp_size
  };

  static PyMethodDef signal_methods[] = {
      {"add", (PyCFunction) &pya_signal_add, METH_VARARGS, "internal signal proxy object: += operator" },
      {"remove", (PyCFunction) &pya_signal_remove, METH_VARARGS, "internal signal proxy object: -= operator" },
      {"set", (PyCFunction) &pya_signal_set, METH_VARARGS, "internal signal proxy object: assignment" },
      {"clear", (PyCFunction) &pya_signal_clear, METH_NOARGS, "internal signal proxy object: clears all receivers" },
      {NULL,  NULL},
  };

  static PyNumberMethods nm = { };
  nm.nb_inplace_add = &pya_signal_inplace_add;
  nm.nb_inplace_subtract = &pya_signal_inplace_remove;

#if PY_MAJOR_VERSION < 3
  signal_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
#else
  signal_type.tp_flags = Py_TPFLAGS_DEFAULT;
#endif
  signal_type.tp_methods = signal_methods;
  signal_type.tp_as_number = &nm;
  signal_type.tp_dealloc = &pya_signal_deallocate;
  signal_type.tp_call = &pya_signal_call;

  PyType_Ready (&signal_type);
  Py_INCREF (&signal_type);

  PyModule_AddObject (module, "_Signal", (PyObject *) &signal_type);

  cls = &signal_type;
}

PYASignal *
PYASignal::create (PyObject *origin, pya::SignalHandler *handler)
{
  tl_assert (cls != 0);
  PYASignal *signal_obj = (PYASignal *) cls->tp_alloc (cls, 0);
  if (signal_obj == NULL) {
    check_error ();
  } else {
    new (signal_obj) PYASignal (origin, handler);
  }
  return signal_obj;
}

}


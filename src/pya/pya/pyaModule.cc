
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


#include <Python.h>

#include "pyaModule.h"
#include "pya.h"
#include "pyaObject.h"
#include "pyaConvert.h"
#include "pyaHelpers.h"
#include "pyaMarshal.h"
#include "pyaSignalHandler.h"
#include "pyaUtils.h"
#include "pyaInternal.h"
#include "pyaCallables.h"

#include "version.h"

#include <map>

namespace pya
{

// --------------------------------------------------------------------------
//  Some utilities

static void
set_type_attr (PyTypeObject *type, const std::string &name, PythonRef &attr)
{
  tl_assert (attr.get () != NULL);
  if (type->tp_dict != NULL && PyDict_GetItemString ((PyObject *) type, name.c_str ()) != NULL) {
    tl::warn << "Ambiguous attribute name " << name << " in class " << type->tp_name;
  } else {
    PyObject_SetAttrString ((PyObject *) type, name.c_str (), attr.get ());
  }
}

// --------------------------------------------------------------------------
//  The PythonModule implementation

std::map<const gsi::MethodBase *, std::string> PythonModule::m_python_doc;
std::vector<const gsi::ClassBase *> PythonModule::m_classes;

const std::string pymod_name ("klayout");

PythonModule::PythonModule ()
  : mp_mod_def (0)
{
  //  .. nothing yet ..
}

PythonModule::~PythonModule ()
{
  PYAObjectBase::clear_callbacks_cache ();

  //  the Python objects were probably deleted by Python itself as it exited -
  //  don't try to delete them again.
  mp_module.release ();

  while (!m_methods_heap.empty ()) {
    delete m_methods_heap.back ();
    m_methods_heap.pop_back ();
  }

  while (!m_getseters_heap.empty ()) {
    delete m_getseters_heap.back ();
    m_getseters_heap.pop_back ();
  }

  if (mp_mod_def) {
    delete[] mp_mod_def;
    mp_mod_def = 0;
  }
}

PyObject *
PythonModule::module ()
{
  return mp_module.get ();
}

PyObject *
PythonModule::take_module ()
{
  return mp_module.release ();
}

void
PythonModule::init (const char *mod_name, const char *description)
{
  //  create a (standalone) Python interpreter if we don't have one yet
  //  NOTE: Python itself will take care to remove this instance in this case.
  if (! pya::PythonInterpreter::instance ()) {
    new pya::PythonInterpreter (false);
  }

  //  do some checks before we create the module
  tl_assert (mod_name != 0);
  tl_assert (mp_module.get () == 0);

  m_mod_name = pymod_name + "." + mod_name;
  m_mod_description = description;

  PyObject *module = 0;

#if PY_MAJOR_VERSION < 3

  static PyMethodDef module_methods[] = {
    {NULL}  // Sentinel
  };

  module = Py_InitModule3 (m_mod_name.c_str (), module_methods, m_mod_description.c_str ());

#else

  struct PyModuleDef mod_def = {
     PyModuleDef_HEAD_INIT,
     m_mod_name.c_str (),
     NULL,     // module documentation
     -1,       // size of per-interpreter state of the module,
               // if the module keeps state in global variables.
     NULL
  };

  tl_assert (! mp_mod_def);

  //  prepare a persistent structure with the module definition
  //  and pass this one to PyModule_Create
  mp_mod_def = new char[sizeof (PyModuleDef)];
  memcpy ((void *) mp_mod_def, (const void *) &mod_def, sizeof (PyModuleDef));

  module = PyModule_Create ((PyModuleDef *) mp_mod_def);
  
#endif

  mp_module = PythonRef (module);
}

void
PythonModule::init (const char *mod_name, PyObject *module)
{
  //  do some checks before we create the module
  tl_assert (mp_module.get () == 0);

  m_mod_name = mod_name;
  mp_module = PythonRef (module);
}

PyMethodDef *
PythonModule::make_method_def ()
{
  static PyMethodDef md = { };
  m_methods_heap.push_back (new PyMethodDef (md));
  return m_methods_heap.back ();
}

PyGetSetDef *
PythonModule::make_getset_def ()
{
  static PyGetSetDef gsd = { };
  m_getseters_heap.push_back (new PyGetSetDef (gsd));
  return m_getseters_heap.back ();
}

char *
PythonModule::make_string (const std::string &s)
{
  m_string_heap.push_back (s);
  return const_cast<char *> (m_string_heap.back ().c_str ());
}

void
PythonModule::add_python_doc (const gsi::ClassBase & /*cls*/, const MethodTable *mt, int mid, const std::string &doc)
{
  for (MethodTableEntry::method_iterator m = mt->begin (mid); m != mt->end (mid); ++m) {
    add_python_doc (*m, doc);
  }
}

void
PythonModule::add_python_doc (const gsi::MethodBase *m, const std::string &doc)
{
  std::string &doc_string = m_python_doc [m];
  doc_string += doc;
  doc_string += ".\n\n";
}

std::string
PythonModule::python_doc (const gsi::MethodBase *method)
{
  std::map<const gsi::MethodBase *, std::string>::const_iterator d = m_python_doc.find (method);
  if (d != m_python_doc.end ()) {
    return d->second;
  } else {
    return std::string ();
  }
}

namespace
{

class PythonClassGenerator
{
public:
  PythonClassGenerator (PythonModule *module, PyObject *all_list)
    : mp_module (module), m_all_list (all_list)
  {
    //  .. nothing yet ..
  }

  //  needs to be called before for each extension before the classes are made
  void register_extension (const gsi::ClassBase *cls)
  {
    if (cls->name ().empty ()) {
      //  got an extension
      tl_assert (cls->parent ());
      m_extensions_for [cls->parent ()->declaration ()].push_back (cls->declaration ());
    }
  }

  PyTypeObject *make_class (const gsi::ClassBase *cls, bool as_static)
  {
    //  drop non-standard names
    if (tl::verbosity () >= 40) {
      tl::info << tl::sprintf (tl::to_string (tr ("Creating class %s.%s")), PyModule_GetName (mp_module->module ()), cls->name ());
    }

    //  NOTE: with as_static = true, this method produces a mixin. This is a class entirely consisting
    //  of static constants and child classes only. It can be mixed into an existing class for emulation
    //  additional base classes.
    //  Everything else, like properties and methods will not work as the method enumeration scheme is
    //  not capable of handling more than a single base class.

    PyTypeObject *pt = PythonClassClientData::py_type (*cls, as_static);
    if (pt != 0) {
      return pt;
    }

    PythonRef bases;

    //  mix-in unnamed extensions and get the base classes

    int n_bases = (cls->base () != 0 ? 1 : 0);
    auto exts = m_extensions_for.find (cls);
    if (exts != m_extensions_for.end ()) {
      n_bases += int (exts->second.size ());
    }

    bases = PythonRef (PyTuple_New (n_bases));

    int ibase = 0;
    if (cls->base () != 0) {
      PyTypeObject *pt = make_class (cls->base (), as_static);
      PyObject *base = (PyObject *) pt;
      Py_INCREF (base);
      PyTuple_SetItem (bases.get (), ibase++, base);
    }

    if (exts != m_extensions_for.end ()) {
      for (auto ie = exts->second.begin (); ie != exts->second.end (); ++ie) {
        PyObject *base = (PyObject *) make_class (*ie, true);
        Py_INCREF (base);
        PyTuple_SetItem (bases.get (), ibase++, base);
      }
    }

    //  creates the type object

    PythonRef dict (PyDict_New ());
    PyDict_SetItemString (dict.get (), "__module__", PythonRef (c2python (mp_module->mod_name ())).get ());
    PyDict_SetItemString (dict.get (), "__doc__", PythonRef (c2python (cls->doc ())).get ());
    PyDict_SetItemString (dict.get (), "__gsi_id__", PythonRef (c2python (mp_module->next_class_id ())).get ());

    PythonRef args (PyTuple_New (3));
    if (! as_static) {
      PyTuple_SetItem (args.get (), 0, c2python (cls->name ()));
    } else {
      PyTuple_SetItem (args.get (), 0, c2python (cls->name () + "_Mixin"));
    }
    PyTuple_SetItem (args.get (), 1, bases.release ());
    PyTuple_SetItem (args.get (), 2, dict.release ());

    PyTypeObject *type = (PyTypeObject *) PyObject_Call ((PyObject *) &PyType_Type, args.get (), NULL);
    if (type == NULL) {
      try {
      	check_error ();
      } catch (tl::Exception &ex) {
	tl::error << ex.msg ();
      }
      tl_assert (false);
    }

    //  Customize
    if (! as_static) {
      type->tp_basicsize += sizeof (PYAObjectBase);
      type->tp_init = &pya_object_init;
      type->tp_new = &pya_object_new;
      type->tp_dealloc = (destructor) &pya_object_deallocate;
      type->tp_setattro = PyObject_GenericSetAttr;
      type->tp_getattro = PyObject_GenericGetAttr;
    }

    PythonClassClientData::initialize (*cls, type, as_static, mp_module);

    mp_module->register_class (cls);

    tl_assert (mp_module->cls_for_type (type) == cls);

    //  add to the parent class as child class or add to module

    if (! cls->parent ()) {
      PyList_Append (m_all_list, PythonRef (c2python (cls->name ())).get ());
      PyModule_AddObject (mp_module->module (), cls->name ().c_str (), (PyObject *) type);
    }

    //  produce the child classes

    for (auto cc = cls->begin_child_classes (); cc != cls->end_child_classes (); ++cc) {
      if (! cc->name ().empty ()) {
        PyTypeObject *child_class = make_class (cc->declaration (), as_static);
        PythonRef attr ((PyObject *) child_class, false /*borrowed*/);
        set_type_attr (type, cc->name ().c_str (), attr);
      }
    }

    //  Build the attributes now ...

    MethodTable *mt = MethodTable::method_table_by_class (cls);

    //  produce the properties

    for (size_t mid = mt->bottom_property_mid (); mid < mt->top_property_mid (); ++mid) {

      MethodTableEntry::method_iterator begin_setters = mt->begin_setters (mid);
      MethodTableEntry::method_iterator end_setters = mt->end_setters (mid);
      MethodTableEntry::method_iterator begin_getters = mt->begin_getters (mid);
      MethodTableEntry::method_iterator end_getters = mt->end_getters (mid);
      int setter_mid = begin_setters != end_setters ? int (mid) : -1;
      int getter_mid = begin_getters != end_getters ? int (mid) : -1;

      bool is_static = false;
      if (begin_setters != end_setters) {
        is_static = (*begin_setters)->is_static ();
      } else if (begin_getters != end_getters) {
        is_static = (*begin_getters)->is_static ();
      }

      if (! as_static || is_static) {

        const std::string &name = mt->property_name (mid);

        //  look for the real getter and setter, also look in the base classes
        const gsi::ClassBase *icls = cls;
        while ((icls = icls->base ()) != 0 && (begin_setters == end_setters || begin_getters == end_getters)) {

          const MethodTable *mt_base = MethodTable::method_table_by_class (icls);
          tl_assert (mt_base);
          std::pair<bool, size_t> t = mt_base->find_property (is_static, name);
          if (t.first) {
            if (begin_setters == end_setters && mt_base->begin_setters (t.second) != mt_base->end_setters (t.second)) {
              setter_mid = int (t.second);
              begin_setters = mt_base->begin_setters (t.second);
              end_setters = mt_base->end_setters (t.second);
            }
            if (begin_getters == end_getters && mt_base->begin_getters (t.second) != mt_base->end_getters (t.second)) {
              getter_mid = int (t.second);
              begin_getters = mt_base->begin_getters (t.second);
              end_getters = mt_base->end_getters (t.second);
            }
          }

        }

        std::string doc;

        //  add getter and setter documentation, create specific Python documentation

        for (MethodTableEntry::method_iterator m = begin_getters; m != end_getters; ++m) {
          if (! doc.empty ()) {
            doc += "\n\n";
          }
          doc += (*m)->doc ();
          mp_module->add_python_doc (*m, tl::sprintf (tl::to_string (tr ("The object exposes a readable attribute '%s'. This is the getter")), name));
        }

        for (MethodTableEntry::method_iterator m = begin_setters; m != end_setters; ++m) {
          if (! doc.empty ()) {
            doc += "\n\n";
          }
          doc += (*m)->doc ();
          mp_module->add_python_doc (*m, tl::sprintf (tl::to_string (tr ("The object exposes a writable attribute '%s'. This is the setter")), name));
        }

        PythonRef attr;

        if (! is_static) {

          //  non-static attribute getters/setters
          PyGetSetDef *getset = mp_module->make_getset_def ();
          getset->name = mp_module->make_string (name);
          getset->get = begin_getters != end_getters ? &property_getter_func : NULL;
          getset->set = begin_setters != end_setters ? &property_setter_func : NULL;
          getset->doc = mp_module->make_string (doc);
          getset->closure = make_closure (getter_mid, setter_mid);

          attr = PythonRef (PyDescr_NewGetSet (type, getset));

        } else {

          PYAStaticAttributeDescriptorObject *desc = PYAStaticAttributeDescriptorObject::create (mp_module->make_string (name));

          desc->type = type;
          desc->getter = begin_getters != end_getters ? get_property_getter_adaptor (getter_mid) : NULL;
          desc->setter = begin_setters != end_setters ? get_property_setter_adaptor (setter_mid) : NULL;
          attr = PythonRef (desc);

        }

        set_type_attr (type, name, attr);

      }

    }

    if (! as_static) {

      //  collect the names which have been disambiguated static/non-static wise
      std::vector<std::string> disambiguated_names;

      //  produce the methods now
      for (size_t mid = mt->bottom_mid (); mid < mt->top_mid (); ++mid) {

        if (! mt->is_enabled (mid)) {
          continue;
        }

        std::string name = mt->name (mid);

        //  needs static/non-static disambiguation?
        std::pair<bool, size_t> t = mt->find_method (! mt->is_static (mid), name);
        if (t.first) {

          disambiguated_names.push_back (name);
          if (mt->is_static (mid)) {
            name = "_class_" + name;
            mp_module->add_python_doc (*cls, mt, int (mid), tl::sprintf (tl::to_string (tr ("This class method is available as '%s' in Python")), name));
          } else {
            name = "_inst_" + name;
            mp_module->add_python_doc (*cls, mt, int (mid), tl::sprintf (tl::to_string (tr ("This instance method is available as '%s' in Python")), name));
          }

        } else {

          //  does this method hide a property? -> append "_" in that case
          t = mt->find_property (mt->is_static (mid), name);
          if (t.first) {
            name += "_";
            mp_module->add_python_doc (*cls, mt, int (mid), tl::sprintf (tl::to_string (tr ("This method is available as '%s' in Python to distiguish it from the property with the same name")), name));
          }

        }

        //  create documentation
        std::string doc;
        for (MethodTableEntry::method_iterator m = mt->begin (mid); m != mt->end (mid); ++m) {
          if (! doc.empty ()) {
            doc = "\n\n";
          }
          doc += (*m)->doc ();
        }

        if (! mt->is_static (mid)) {  //  Bound methods

          PyMethodDef *method = mp_module->make_method_def ();
          method->ml_name = mp_module->make_string (name);
          if (name == "__deepcopy__") {
            //  Special handling needed as the memo argument needs to be ignored
            method->ml_meth = &object_default_deepcopy_impl;
          } else if (mt->is_init (mid)) {
            method->ml_meth = (PyCFunction) get_method_init_adaptor (mid);
          } else {
            method->ml_meth = (PyCFunction) get_method_adaptor (mid);
          }
          method->ml_doc = mp_module->make_string (doc);
          method->ml_flags = METH_VARARGS;

          PythonRef attr = PythonRef (PyDescr_NewMethod (type, method));
          set_type_attr (type, name, attr);

        } else {  //  Class methods

          PyMethodDef *method = mp_module->make_method_def ();
          method->ml_name = mp_module->make_string (name);
          method->ml_meth = (PyCFunction) get_method_adaptor (mid);
          method->ml_doc = mp_module->make_string (doc);
          method->ml_flags = METH_VARARGS | METH_CLASS;

          PythonRef attr = PythonRef (PyDescr_NewClassMethod (type, method));
          set_type_attr (type, name, attr);

        }

      }

      //  Complete the comparison operators if necessary.
      //  Unlike Ruby, Python does not automatically implement != from == for example.
      //  We assume that "==" and "<" are the minimum requirements for full comparison
      //  and "==" is the minimum requirement for equality. Hence:
      //    * If "==" is given, but no "!=", synthesize
      //        "a != b" by "!a == b"
      //    * If "==" and "<" are given, synthesize if required
      //        "a <= b" by "a < b || a == b"
      //        "a > b" by "!(a < b || a == b)"  (could be b < a, but this avoids having to switch arguments)
      //        "a >= b" by "!a < b"

      bool has_eq = mt->find_method (false, "==").first;
      bool has_ne = mt->find_method (false, "!=").first;
      bool has_lt = mt->find_method (false, "<").first;
      bool has_le = mt->find_method (false, "<=").first;
      bool has_gt = mt->find_method (false, ">").first;
      bool has_ge = mt->find_method (false, ">=").first;
      bool has_cmp = mt->find_method (false, "<=>").first;

      if (! has_cmp && has_eq) {

        if (! has_ne) {

          //  Add a definition for "__ne__"
          PyMethodDef *method = mp_module->make_method_def ();
          method->ml_name = "__ne__";
          method->ml_meth = &object_default_ne_impl;
          method->ml_flags = METH_VARARGS;

          PythonRef attr = PythonRef (PyDescr_NewMethod (type, method));
          set_type_attr (type, method->ml_name, attr);

        }

        if (has_lt && ! has_le) {

          //  Add a definition for "__le__"
          PyMethodDef *method = mp_module->make_method_def ();
          method->ml_name = "__le__";
          method->ml_meth = &object_default_le_impl;
          method->ml_flags = METH_VARARGS;

          PythonRef attr = PythonRef (PyDescr_NewMethod (type, method));
          set_type_attr (type, method->ml_name, attr);

        }

        if (has_lt && ! has_gt) {

          //  Add a definition for "__gt__"
          PyMethodDef *method = mp_module->make_method_def ();
          method->ml_name = "__gt__";
          method->ml_meth = &object_default_gt_impl;
          method->ml_flags = METH_VARARGS;

          PythonRef attr = PythonRef (PyDescr_NewMethod (type, method));
          set_type_attr (type, method->ml_name, attr);

        }

        if (has_lt && ! has_ge) {

          //  Add a definition for "__ge__"
          PyMethodDef *method = mp_module->make_method_def ();
          method->ml_name = "__ge__";
          method->ml_meth = &object_default_ge_impl;
          method->ml_flags = METH_VARARGS;

          PythonRef attr = PythonRef (PyDescr_NewMethod (type, method));
          set_type_attr (type, method->ml_name, attr);

        }

      }

      //  install the static/non-static dispatcher descriptor

      std::sort (disambiguated_names.begin (), disambiguated_names.end ());
      disambiguated_names.erase (std::unique (disambiguated_names.begin (), disambiguated_names.end ()), disambiguated_names.end ());

      for (std::vector<std::string>::const_iterator a = disambiguated_names.begin (); a != disambiguated_names.end (); ++a) {

        std::pair<bool, size_t> pa;
        pa = mt->find_method (true, *a);
        if (pa.first) {
          mt->alias (pa.second, "_class_" + *a);
        }
        pa = mt->find_method (false, *a);
        if (pa.first) {
          mt->alias (pa.second, "_inst_" + *a);
        }

        PyObject *attr_inst = PyObject_GetAttrString ((PyObject *) type, ("_inst_" + *a).c_str ());
        PyObject *attr_class = PyObject_GetAttrString ((PyObject *) type, ("_class_" + *a).c_str ());
        if (attr_inst == NULL || attr_class == NULL) {

          //  some error -> don't install the disambiguator
          Py_XDECREF (attr_inst);
          Py_XDECREF (attr_class);
          PyErr_Clear ();

          if (tl::verbosity () >= 20) {
            tl::warn << "Unable to install a static/non-static disambiguator for " << *a << " in class " << cls->name ();
          }

        } else {

          PyObject *desc = PYAAmbiguousMethodDispatcher::create (attr_inst, attr_class);
          PythonRef name (c2python (*a));
          //  Note: we use GenericSetAttr since that one allows us setting attributes on built-in types
          PyObject_GenericSetAttr ((PyObject *) type, name.get (), desc);

        }

      }

    }

    mt->finish ();

    return type;
  }

private:
  PythonModule *mp_module;
  PyObject *m_all_list;
  std::map<const gsi::ClassBase *, std::vector<const gsi::ClassBase *> > m_extensions_for;
};

}

void
PythonModule::make_classes (const char *mod_name)
{
  PyObject *module = mp_module.get ();

  //  Prepare an __all__ index for the module

  PythonRef all_list;
  if (! PyObject_HasAttrString (module, "__all__")) {
    all_list = PythonRef (PyList_New (0));
    PyObject_SetAttrString (module, "__all__", all_list.get ());
  } else {
    all_list = PythonRef (PyObject_GetAttrString (module, "__all__"));
  }

  //  Establish __doc__
  PyObject_SetAttrString (module, "__doc__", PythonRef (c2python (m_mod_description)).get ());
  PyList_Append (all_list.get (), PythonRef (c2python ("__doc__")).get ());

  //  Establish __version__
  PyObject_SetAttrString (module, "__version__", PythonRef (c2python (prg_version)).get ());
  PyList_Append (all_list.get (), PythonRef (c2python ("__version__")).get ());

  //  Build a class for descriptors for static attributes
  PYAStaticAttributeDescriptorObject::make_class (module);

  //  Build a class for static/non-static dispatching descriptors
  PYAAmbiguousMethodDispatcher::make_class (module);

  //  Build a class for iterators
  PYAIteratorObject::make_class (module);

  //  Build a class for signals
  PYASignal::make_class (module);

  std::list<const gsi::ClassBase *> sorted_classes = gsi::ClassBase::classes_in_definition_order (mod_name);

  PythonClassGenerator gen (this, all_list.get ());

  if (mod_name) {
    for (std::list<const gsi::ClassBase *>::const_iterator c = sorted_classes.begin (); c != sorted_classes.end (); ++c) {
      if ((*c)->module () != mod_name) {
        //  don't handle classes outside this module, but require them to be present
        if (! PythonClassClientData::py_type (**c, false)) {
          throw tl::Exception (tl::sprintf ("class %s.%s required from outside the module %s, but that module is not loaded", (*c)->module (), (*c)->name (), mod_name));
        }
      }
    }
  }

  //  first pass: register the extensions using all available classes
  for (std::list<const gsi::ClassBase *>::const_iterator c = sorted_classes.begin (); c != sorted_classes.end (); ++c) {
    if ((*c)->declaration () != *c) {
      gen.register_extension (*c);
    }
  }

  //  second pass: make the classes
  for (auto c = sorted_classes.begin (); c != sorted_classes.end (); ++c) {
    if ((*c)->declaration () == *c && (! mod_name || (*c)->module () == mod_name)) {
      gen.make_class (*c, false);
    }
  }
}

const gsi::ClassBase *PythonModule::cls_for_type (PyTypeObject *type)
{
  //  GSI classes store their class index inside the __gsi_id__ attribute
  if (PyObject_HasAttrString ((PyObject *) type, "__gsi_id__")) {

    PyObject *cls_id = PyObject_GetAttrString ((PyObject *) type, "__gsi_id__");
    if (cls_id != NULL && pya::test_type<size_t> (cls_id)) {
      size_t i = pya::python2c<size_t> (cls_id);
      if (i < m_classes.size ()) {
        return m_classes [i];
      }
    }

  }

  return 0;
}

PyTypeObject *PythonModule::type_for_cls (const gsi::ClassBase *cls)
{
  return PythonClassClientData::py_type (*cls, false);
}

}

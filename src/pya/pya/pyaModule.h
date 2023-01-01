
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


#ifndef _HDR_pyaModule
#define _HDR_pyaModule

#include <Python.h>

#include "pyaCommon.h"
#include "pyaRefs.h"

#include <map>
#include <list>
#include <vector>
#include <string>

namespace gsi
{
  class ClassBase;
  class MethodBase;
}

namespace pya
{

class MethodTable;

/**
 *  @brief A representative for a Python module
 *  Instantiate this object to represent a python module.
 *  If used externally (for a library), call init with the module name.
 *  Then call make_classes to create the individual classes.
 */
class PYA_PUBLIC PythonModule
{
public:
  /**
   *  @brief Constructor
   */
  PythonModule ();

  /**
   *  @brief Destructor
   */
  ~PythonModule ();

  /**
   *  @brief Initializes the module
   *  This entry point is for external use where the module has not been created yet
   */
  void init (const char *mod_name, const char *description);

  /**
   *  @brief Initializes the module
   *  This entry point is for internal use where the module is created by the interpreter
   */
  void init (const char *mod_name, PyObject *module);

  /**
   *  @brief Creates the classes after init has been called
   */
  void make_classes (const char *mod_name = 0);

  /**
   *  @brief Gets the GSI class for a Python class
   */
  static const gsi::ClassBase *cls_for_type (PyTypeObject *type);

  /**
   *  @brief The reverse: gets a Python class for a GSI class or NULL if there is no binding
   */
  static PyTypeObject *type_for_cls (const gsi::ClassBase *cls);

  /**
   *  @brief Returns additional Python-specific documentation for the given method
   *  If no specific documentation exists, an empty string is returned.
   */
  static std::string python_doc (const gsi::MethodBase *method);

  /**
   *  @brief Gets the PyModule object
   */
  PyObject *module ();

  /**
   *  @brief Gets the PyModule object
   *  This method will release the ownership over the PyObject
   */
  PyObject *take_module ();

  /**
   *  @brief Gets the module name
   */
  const std::string &mod_name () const
  {
    return m_mod_name;
  }

  /**
   *  @brief Adds the given documentation for the given class and method
   */
  void add_python_doc (const gsi::ClassBase &cls, const MethodTable *mt, int mid, const std::string &doc);

  /**
   *  @brief Adds the given documentation for the given class
   */
  void add_python_doc (const gsi::MethodBase *m, const std::string &doc);

  /**
   *  @brief Creates a method definition
   */
  PyMethodDef *make_method_def ();

  /**
   *  @brief Creates a property getter/setter pair
   */
  PyGetSetDef *make_getset_def ();

  /**
   *  @brief Creates a string with backup storage
   */
  char *make_string (const std::string &s);

  /**
   *  @brief Gets the next available class ID
   */
  size_t next_class_id () const
  {
    return m_classes.size ();
  }

  /**
   *  @brief Register the class
   */
  void register_class (const gsi::ClassBase *cls)
  {
    m_classes.push_back (cls);
  }

private:
  std::list<std::string> m_string_heap;
  std::vector<PyMethodDef *> m_methods_heap;
  std::vector<PyGetSetDef *> m_getseters_heap;

  std::string m_mod_name, m_mod_description;
  PythonRef mp_module;
  char *mp_mod_def;

  static std::map<const gsi::MethodBase *, std::string> m_python_doc;
  static std::vector<const gsi::ClassBase *> m_classes;
};

}

#endif

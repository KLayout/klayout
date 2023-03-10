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


#ifndef _HDR_pyaInternal
#define _HDR_pyaInternal

#include <Python.h>

#include "pyaCommon.h"
#include "pyaRefs.h"

#include "gsiClassBase.h"

#include <map>
#include <list>
#include <vector>
#include <string>

namespace gsi
{
  class MethodBase;
}

namespace pya
{

class PythonModule;

// -------------------------------------------------------------------
//  The lookup table for the method overload resolution

/**
 *  @brief A single entry in the method table
 *  This class provides an entry for one name. It provides flags
 *  (ctor, static, protected) for the method and a list of implementations
 *  (gsi::MethodBase objects)
 */
class MethodTableEntry
{
public:
  typedef std::vector<const gsi::MethodBase *>::const_iterator method_iterator;

  MethodTableEntry (const std::string &name, bool st, bool prot);

  const std::string &name () const;
  void set_name (const std::string &n);

  void set_enabled (bool en);
  bool is_enabled () const;

  void set_fallback_not_implemented (bool en);
  bool fallback_not_implemented () const;

  void set_init(bool f);
  bool is_init () const;

  bool is_static () const;
  bool is_protected () const;

  void add (const gsi::MethodBase *m);

  void finish ();

  method_iterator begin () const;
  method_iterator end () const;

  const std::vector<const gsi::MethodBase *> &methods () const
  {
    return m_methods;
  }

private:
  std::string m_name;
  bool m_is_static : 1;
  bool m_is_protected : 1;
  bool m_is_enabled : 1;
  bool m_is_init : 1;
  bool m_fallback_not_implemented : 1;
  std::vector<const gsi::MethodBase *> m_methods;
};

/**
 *  @brief The method table for a class
 *  The method table will provide the methods associated with a native method, i.e.
 *  a certain name. It only provides the methods, not a overload resolution strategy.
 */
class MethodTable
{
public:
  /**
   *  @brief Constructor
   *  This constructor will create a method table for the given class and register
   *  this table under this class.
   */
  MethodTable (const gsi::ClassBase *cls_decl, PythonModule *module);
  /**
   *  @brief Returns the lowest method ID within the space of this table
   *  Method IDs below this one are reserved for base class methods
   */
  size_t bottom_mid () const;

  /**
   *  @brief Returns the topmost + 1 method ID.
   */
  size_t top_mid () const;

  /**
   *  @brief Returns the lowest property method ID within the space of this table
   *  Method IDs below this one are reserved for base class methods
   */
  size_t bottom_property_mid () const;

  /**
   *  @brief Returns the topmost + 1 property method ID.
   */
  size_t top_property_mid () const;

  /**
   *  @brief Find a method with the given name and static flag
   *  Returns true or false in the first part (true, if found) and
   *  the MID in the second part.
   */
  std::pair<bool, size_t> find_method (bool st, const std::string &name) const;

  /**
   *  @brief Find a property with the given name and static flag
   *  Returns true or false in the first part (true, if found) and
   *  the MID in the second part.
   */
  std::pair<bool, size_t> find_property (bool st, const std::string &name) const;

  /**
   *  @brief Adds a method to the table
   */
  void add_method (const std::string &name, const gsi::MethodBase *mb);

  /**
   *  @brief Adds a setter with the given name
   */
  void add_setter (const std::string &name, const gsi::MethodBase *setter);

  /**
   *  @brief Adds a getter with the given name
   */
  void add_getter (const std::string &name, const gsi::MethodBase *getter);

  /**
   *  @brief Returns true if the method is enabled
   */
  bool is_enabled (size_t mid) const;

  /**
   *  @brief Enables or disables a method
   */
  void set_enabled (size_t mid, bool en);

  /**
   *  @brief Returns true if the method has a NotImplemented fallback
   */
  bool fallback_not_implemented (size_t mid) const;

  /**
   *  @brief Sets a value indicating that the method has a fallback to NotImplemented for non-matching arguments
   */
  void set_fallback_not_implemented (size_t mid, bool f);

  /**
   *  @brief Returns true if the method is an initializer
   */
  bool is_init (size_t mid) const;

  /**
   *  @brief Sets initializer
   */
  void set_init (size_t mid, bool f);

  /**
   *  @brief Returns true if the method with the given ID is static
   */
  bool is_static (size_t mid) const;

  /**
   *  @brief Returns true if the method with the given ID is protected
   */
  bool is_protected (size_t mid) const;

  /**
   *  @brief Creates an alias for the given method
   */
  void alias (size_t mid, const std::string &new_name);

  /**
   *  @brief Renames a method
   */
  void rename (size_t mid, const std::string &new_name);

  /**
   *  @brief Returns the name of the method with the given ID
   */
  const std::string &name (size_t mid) const;

  /**
   *  @brief Returns the name of the property with the given ID
   */
  const std::string &property_name (size_t mid) const;

  /**
   *  @brief Begins iteration of the overload variants for setter of property ID mid
   */
  MethodTableEntry::method_iterator begin_setters (size_t mid) const;

  /**
   *  @brief Ends iteration of the overload variants for setter of property ID mid
   */
  MethodTableEntry::method_iterator end_setters (size_t mid) const;

  /**
   *  @brief Begins iteration of the overload variants for getter of property ID mid
   */
  MethodTableEntry::method_iterator begin_getters (size_t mid) const;

  /**
   *  @brief Ends iteration of the overload variants for getter of property ID mid
   */
  MethodTableEntry::method_iterator end_getters (size_t mid) const;

  /**
   *  @brief Begins iteration of the overload variants for method ID mid
   */
  MethodTableEntry::method_iterator begin (size_t mid) const;

  /**
   *  @brief Ends iteration of the overload variants for method ID mid
   */
  MethodTableEntry::method_iterator end (size_t mid) const;

  /**
   *  @brief Finishes construction of the table
   *  This method must be called after the add_method calls have been used
   *  to fill the table. It will remove duplicate entries and clean up memory.
   */
  void finish ();

  /**
   *  @brief Obtain a method table for a given class
   */
  static MethodTable *method_table_by_class (const gsi::ClassBase *cls_decl);

  /**
   *  @brief Gets the method table
   */
  const std::vector<MethodTableEntry> &method_table () const
  {
    return m_table;
  }

  /**
   *  @brief Gets the property table
   */
  const std::vector<std::pair<MethodTableEntry, MethodTableEntry> > &property_table () const
  {
    return m_property_table;
  }

private:
  size_t m_method_offset;
  size_t m_property_offset;
  const gsi::ClassBase *mp_cls_decl;
  std::map<std::pair<bool, std::string>, size_t> m_name_map;
  std::map<std::pair<bool, std::string>, size_t> m_property_name_map;
  std::vector<MethodTableEntry> m_table;
  std::vector<std::pair<MethodTableEntry, MethodTableEntry> > m_property_table;
  PythonModule *mp_module;

  void add_method_basic (const std::string &name, const gsi::MethodBase *mb, bool enabled = true, bool init = false, bool fallback_not_implemented = false);
  void add_setter_basic (const std::string &name, const gsi::MethodBase *setter);
  void add_getter_basic (const std::string &name, const gsi::MethodBase *getter);
  bool is_property_setter (bool st, const std::string &name);
  bool is_property_getter (bool st, const std::string &name);
};

struct PythonClassClientData
  : public gsi::PerClassClientSpecificData
{
  PythonClassClientData (const gsi::ClassBase *_cls, PyTypeObject *_py_type, PyTypeObject *_py_type_static, PythonModule *module);
  ~PythonClassClientData ();

  PythonPtr py_type_object;
  PythonPtr py_type_object_static;
  MethodTable method_table;

  static PyTypeObject *py_type (const gsi::ClassBase &cls_decl, bool as_static);
  static void initialize (const gsi::ClassBase &cls_decl, PyTypeObject *py_type, bool as_static, PythonModule *module);
};

}

#endif

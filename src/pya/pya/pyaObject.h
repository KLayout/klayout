
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


#ifndef _HDR_pyaObject
#define _HDR_pyaObject

#include <Python.h>

#include "pyaRefs.h"
#include "pyaCommon.h"
#include "pyaSignalHandler.h"

#include "tlAssert.h"

#include <vector>
#include <map>
#include <memory>

namespace gsi
{
  class ClassBase;
  class MethodBase;
}

namespace pya
{

class Callee;
class StatusChangedListener;

/**
 *  @brief The Python object representing a GSI object
 *
 *  NOTE: this memory block is attached to the actual structure
 *  and obtained by taking the last sizeof(PYAObjectBase) bytes.
 *  It's basically a connector between GSI objects and the Python
 *  objects.
 */
class PYA_PUBLIC PYAObjectBase
{
public:
  /**
   *  @brief Constructor - creates a new object for the given GSI class
   */
  PYAObjectBase (const gsi::ClassBase *_cls_decl, PyObject *py_object);

  /**
   *  @brief Destructor
   */
  ~PYAObjectBase ();

  /**
   *  @brief Gets the PYAObjectBase pointer from a PyObject pointer
   *  This version doesn't check anything.
   */
  static PYAObjectBase *from_pyobject_unsafe (PyObject *py_object)
  {
    //  the objects must not be a pure static extension
    return (PYAObjectBase *)((char *) py_object + Py_TYPE (py_object)->tp_basicsize - sizeof (PYAObjectBase));
  }

  /**
   *  @brief Gets the PYAObjectBase pointer from a PyObject pointer
   */
  static PYAObjectBase *from_pyobject (PyObject *py_object);

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
   *  @brief Gets the Python object for this bridge object
   */
  PyObject *py_object () const
  {
    return mp_py_object;
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
  void object_destroyed ();
  void keep_internal ();

  PyObject *mp_py_object;
  StatusChangedListener *mp_listener;
  Callee *mp_callee;
  const gsi::ClassBase *m_cls_decl;
  void *m_obj;
  bool m_owned : 1;
  bool m_const_ref : 1;
  bool m_destroyed : 1;
  bool m_can_destroy : 1;
  std::map <const gsi::MethodBase *, pya::SignalHandler> m_signal_table;
};

}

#endif


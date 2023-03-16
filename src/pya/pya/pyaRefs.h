
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


#ifndef _HDR_pyaRefs
#define _HDR_pyaRefs

#include "pyaCommon.h"

struct _object;
typedef _object PyObject;

namespace pya
{

class PythonPtr;

/**
 *  @brief A reference-counted Python object reference
 *
 *  This reference represents stolen references. Upon destruction, this
 *  object will automatically decrement the reference count.
 *  The constructor has a parameter which allows using that class for
 *  borrowed references too. 
 *  PythonRef will basically become the owner of the referred object.
 */
class PYA_PUBLIC PythonRef
{
public:
  /**
   *  @brief Creates a void (NULL) reference
   */
  PythonRef ();

  /**
   *  @brief Creates a reference from a pointer
   *
   *  The reference will not take over the ownership over the object,
   *  preserving the semantics of the pointer.
   */
  PythonRef (const PythonPtr &ptr);

  /** 
   *  @brief Creates a reference for the given object
   *  If new_ref is false, the reference is regarded a borrowed reference and
   *  the reference count is incremented initially to compensate for the 
   *  decrement in the destructor
   */
  PythonRef (PyObject *obj, bool new_ref = true);

  /**
   *  @brief Assignment: refers to the same object than the other reference
   */
  PythonRef (const PythonRef &other);

  /**
   *  @brief Destructor: releases the reference
   */
  ~PythonRef ();

  /**
   *  @brief Assigns the given new reference to this
   *  Any existing reference is released.
   */
  PythonRef &operator= (PyObject *obj);

  /**
   *  @brief Assigns the given borrowed reference to this
   *  Any existing reference is released.
   */
  PythonRef &operator= (const PythonPtr &obj);

  /**
   *  @brief Assignment of another reference to this
   */
  PythonRef &operator= (const PythonRef &other);

  /**
   *  @brief Returns true unless the reference is a NULL reference
   */
  operator bool () const;

  /**
   *  @brief Dereferencing operator
   */
  PyObject *operator-> () const;

  /**
   *  @brief Gets the pointer to the referred object
   */
  PyObject *get () const;

  /**
   *  @brief Takes the pointer
   *  After that operation, the PythonRef object is no longer the owner 
   *  of the referred object.
   */
  PyObject *release ();

  /**
   *  @brief Comparison operator
   */
  bool operator== (const PythonRef &other) const
  {
    return mp_obj == other.mp_obj;
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const PythonRef &other) const
  {
    return mp_obj < other.mp_obj;
  }

private:
  PyObject *mp_obj;
};

/**
 *  @brief A reference-counted Python borrowed object reference
 *
 *  This reference represents borrowed references. Upon construction and destruction, this
 *  object will automatically increment and decrement the reference count.
 */
class PYA_PUBLIC PythonPtr
{
public:
  /**
   *  @brief Constructor
   *  The default constructor will create a NULL pointer.
   */
  PythonPtr ();

  /**
   *  @brief Constructor from a borrowed reference
   *  The PythonPtr will keep a reference to that object and release it when it is destroyed.
   */
  PythonPtr (PyObject *obj);

  /**
   *  @brief Conversion constructor
   *  The PythonPtr will become another holder of the reference
   */
  PythonPtr (const PythonRef &other);

  /**
   *  @brief Copy constructor
   *  The PythonPtr will become another holder of the reference
   */
  PythonPtr (const PythonPtr &other);

  /**
   *  @brief Destructor
   *  The destructor will release one reference.
   */
  ~PythonPtr ();

  /**
   *  @brief assigns the other reference to this
   */
  PythonPtr &operator= (const PythonPtr &other);

  /**
   *  @brief Returns true unless the reference is a NULL reference
   */
  operator bool () const;

  /**
   *  @brief Dereferencing operator
   */
  PyObject *operator-> () const;

  /**
   *  @brief Gets the pointer to the referred object
   */
  PyObject *get () const;

  /**
   *  @brief Comparison operator
   */
  bool operator== (const PythonPtr &other) const
  {
    return mp_obj == other.mp_obj;
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const PythonPtr &other) const
  {
    return mp_obj < other.mp_obj;
  }

  /**
   *  @brief Releases the object
   *
   *  This method returns and resets the raw pointer without changing the
   *  reference count.
   */
  PyObject *release ();

private:
  PyObject *mp_obj;
};

}

#endif 


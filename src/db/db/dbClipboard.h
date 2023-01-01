
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



#ifndef HDR_dbClipboard
#define HDR_dbClipboard

#include "dbCommon.h"

#include <vector>

namespace db
{

class Object;

/**
 *  @brief The clipboard object
 *
 *  Each object stored in the clipboard must be derived from this
 *  base class. 
 */
class DB_PUBLIC ClipboardObject
{
public:
  /**
   *  @brief The ctor
   */
  ClipboardObject ()
  {
    //  nothing yet.
  }

  /**
   *  @brief The dtor is virtual to exploit RTTI
   */
  virtual ~ClipboardObject ()
  {
    //  nothing yet.
  }
};

/**
 *  @brief A basic clipboard object with a "type"
 *
 *  This object encapsulates any type into a clipboard 
 *  object for retrieval from the clipboard.
 */
template <class Value>
class ClipboardValue 
  : public ClipboardObject
{
public:
  /**
   *  @brief Create a clipboard object that can be stored using the default constructor of the value
   */
  ClipboardValue ()
    : ClipboardObject (), m_value ()
  {
    //  nothing yet.
  }

  /**
   *  @brief Create a clipboard object that can be stored
   *  
   *  @param value The value to store
   */
  ClipboardValue (const Value &value)
    : ClipboardObject (), m_value (value)
  {
    //  nothing yet.
  }

  /**
   *  @brief Accessor the the value
   */
  const Value &get () const
  {
    return m_value;
  }

  /**
   *  @brief Accessor the the value (non-const)
   */
  Value &get () 
  {
    return m_value;
  }

private:
  Value m_value;
};

/**
 *  @brief The clipboard class
 *
 *  The clipboard allows one to store objects from the ClipboardObject
 *  class. These objects are owned by the clipboard class and must
 *  be passed after have being newed to the += operator.
 *  There is a static instance of the clipboard that should be used
 *  by the applications.
 */
class DB_PUBLIC Clipboard
{
public:
  typedef std::vector <const ClipboardObject *>::const_iterator iterator;

  /**
   *  @brief The singleton instance
   */
  static Clipboard &instance ()
  {
    return m_instance;
  }

  /**
   *  @brief The constructor
   */
  Clipboard ();

  /**
   *  @brief The destructor
   */
  ~Clipboard ();

  /**
   *  @brief Add a new object
   */
  Clipboard &operator+= (ClipboardObject *object);

  /**
   *  @brief Clear the clipboard
   */
  void clear ();

  /**
   *  @brief Access to the objects: start iterator
   */
  iterator begin () 
  {
    return m_objects.begin ();
  }

  /**
   *  @brief Access to the objects: end iterator
   */
  iterator end () 
  {
    return m_objects.end ();
  }

  /**
   *  @brief Tell if the clipboard has any data
   */
  bool empty () const
  {
    return m_objects.empty ();
  }

  /**
   *  @brief Swap the objects in the clipboard object with another one
   */
  void swap (Clipboard &other)
  {
    m_objects.swap (other.m_objects);
  }

private:
  std::vector <const ClipboardObject *> m_objects;
  static Clipboard m_instance;

  //  no copying
  Clipboard (const Clipboard &);
  Clipboard &operator=(const Clipboard &);
};

} // namespace db

#endif


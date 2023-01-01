
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

#ifndef HDR_layBookmarkList
#define HDR_layBookmarkList

#include "laybasicCommon.h"

#include "layDisplayState.h"
#include "tlObject.h"

#include <vector>
#include <string>

namespace tl
{
  class XMLElementList;
}

namespace lay
{

/**
 *  @brief Extend a DisplayState object by a name and some accessors
 */
class LAYBASIC_PUBLIC BookmarkListElement
  : public DisplayState
{
public:
  BookmarkListElement () 
  {
    //  .. nothing yet ..
  }

  BookmarkListElement (const DisplayState &state)
    : DisplayState (state)
  { 
    //  .. nothing yet ..
  }

  BookmarkListElement (const std::string &n, const DisplayState &state)
    : DisplayState (state), m_name (n)
  { 
    //  .. nothing yet ..
  }

  BookmarkListElement &operator= (const DisplayState &state)
  {
    DisplayState::operator= (state);
    return *this;
  }

  const std::string &name () const
  {
    return m_name;
  }

  void set_name (const std::string &n) 
  {
    m_name = n;
  }

  static const tl::XMLElementList *xml_format ();

public:
  std::string m_name;
};

/**
 *  @brief The list of bookmarks
 */
class LAYBASIC_PUBLIC BookmarkList
  : public tl::Object
{
public:
  typedef std::vector<BookmarkListElement> bookmark_list_type;
  typedef bookmark_list_type::const_iterator const_iterator;

  /**
   *  @brief The begin iterator of all bookmark elements
   */
  const_iterator begin () const 
  {
    return m_list.begin ();
  }

  /**
   *  @brief The end iterator of all bookmark elements
   */
  const_iterator end () const 
  {
    return m_list.end ();
  }

  /**
   *  @brief Add a native bookmark list element
   */
  void add (const BookmarkListElement &e)
  {
    m_list.push_back (e);
  }

  /**
   *  @brief Add a bookmark 
   */
  void add (const std::string &name, const DisplayState &state)
  {
    m_list.push_back (BookmarkListElement (name, state));
  }

  /**
   *  @brief Reserve a certain number of elements
   */
  void reserve (size_t n)
  {
    m_list.reserve (n);
  }

  /**
   *  @brief Clear the bookmark list
   */
  void clear ()
  {
    m_list.clear ();
  }
  
  /**
   *  @brief Obtain the number of bookmarks
   */
  size_t size () const
  {
    return m_list.size ();
  }

  /**
   *  @brief Rename the element with the given index
   */
  void rename (size_t index, const std::string &name)
  {
    m_list [index].set_name (name);
  }
  
  /**
   *  @brief Obtain the name of the element with the given index
   */
  const std::string &name (size_t index) const
  {
    return m_list [index].name ();
  }
  
  /**
   *  @brief Set the state of the element with the given index
   */
  void state (size_t index, const DisplayState &state)
  {
    m_list [index] = state;
  }
  
  /**
   *  @brief Obtain the name of the element with the given index
   */
  const DisplayState &state (size_t index) const
  {
    return m_list [index];
  }

  /**
   *  @brief Propose a new bookmark name
   */
  std::string propose_new_bookmark_name () const;

  /**
   *  @brief Save the list 
   */
  void save (const std::string &fn) const;

  /**
   *  @brief Load the list
   */
  void load (const std::string &fn);

private:
  bookmark_list_type m_list;
};


} // namespace lay

#endif

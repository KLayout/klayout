
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


#ifndef HDR_dbLibraryManager
#define HDR_dbLibraryManager

#include "dbCommon.h"

#include "dbTypes.h"
#include "tlClassRegistry.h"
#include "tlEvents.h"
#include "tlThreads.h"

#include <map>
#include <vector>
#include <string>
#include <set>

namespace db
{

class Library;

/**
 *  @brief The library manager
 *
 *  The library manager manages a collection of libraries.
 *
 *  The LibraryManager implements tl::Observed and delivers a signal when the library collection
 *  changes.
 */
class DB_PUBLIC LibraryManager
{
public:
  typedef std::multimap <std::string, lib_id_type> lib_name_map;
  typedef lib_name_map::const_iterator iterator;

  /** 
   *  @brief The singleton instance
   */
  static LibraryManager &instance ();

  /**
   *  @brief Returns true, if the manager is initialized
   */
  static bool initialized ();

  /**
   *  @brief Destructor
   */
  ~LibraryManager ();

  /**
   *  @brief An event indicating that libraries have changed
   */
  tl::Event changed_event;

  /**
   *  @brief Begin iterator for the name/id pairs of the libraries registered.
   */
  iterator begin () const
  {
    return m_lib_by_name.begin ();
  }

  /**
   *  @brief End iterator for the name/id pairs of the libraries registered.
   */
  iterator end () const
  {
    return m_lib_by_name.end ();
  }

  /**
   *  @brief Get the library by name which is valid for all given technologies
   *
   *  This method looks up a library which is valid for all technologies listed in "for_technologies". It may be
   *  responsible for more than that too.
   *
   *  @return A pair, the boolean is true, if the name is valid. The second member is the library id.
   */
  std::pair<bool, lib_id_type> lib_by_name (const std::string &name, const std::set<std::string> &for_technologies) const;

  /**
   *  @brief Get the library by name which is valid for the given technology
   *
   *  This method looks up a library which is valid for the given technology. It may be
   *  responsible for more than that too.
   *
   *  @return A pair, the boolean is true, if the name is valid. The second member is the library id.
   */
  std::pair<bool, lib_id_type> lib_by_name (const std::string &name, const std::string &for_technology) const
  {
    std::set<std::string> techs;
    if (! for_technology.empty ()) {
      techs.insert (for_technology);
    }
    return lib_by_name (name, techs);
  }

  /**
   *  @brief Get the library by name for any technology
   *
   *  @return A pair, the boolean is true, if the name is valid. The second member is the library id.
   */
  std::pair<bool, lib_id_type> lib_by_name (const std::string &name) const
  {
    return lib_by_name (name, std::set<std::string> ());
  }

  /**
   *  @brief Get the library by name
   *
   *  @return The pointer to the library or 0, if there is no library with that name.
   */
  Library *lib_ptr_by_name (const std::string &name) const
  {
    std::pair<bool, lib_id_type> ll = lib_by_name (name);
    if (ll.first) {
      return lib (ll.second);
    } else {
      return 0;
    }
  }

  /**
   *  @brief Get the library by name and technology
   *
   *  @return The pointer to the library or 0, if there is no library with that name.
   */
  Library *lib_ptr_by_name (const std::string &name, const std::string &for_technology) const
  {
    std::pair<bool, lib_id_type> ll = lib_by_name (name, for_technology);
    if (ll.first) {
      return lib (ll.second);
    } else {
      return 0;
    }
  }

  /**
   *  @brief Get the library by name and technology
   *
   *  @return The pointer to the library or 0, if there is no library with that name.
   */
  Library *lib_ptr_by_name (const std::string &name, const std::set<std::string> &for_technologies) const
  {
    std::pair<bool, lib_id_type> ll = lib_by_name (name, for_technologies);
    if (ll.first) {
      return lib (ll.second);
    } else {
      return 0;
    }
  }

  /**
   *  @brief Register a library under the given name and associate a id 
   *
   *  The library will then be owned by the library manager.
   *  This method is use the name property of the library (which must not be
   *  changed later) and set the id of the library.
   *  If a library with that name already exists, the existing library will still 
   *  be available by id, but not by name.
   *
   *  @param library The library to register
   */
  lib_id_type register_lib (Library *library);

  /**
   *  @brief Unregisters a library
   *
   *  This will release the library from the manager's control and lifetime management.
   */
  void unregister_lib (Library *library);

  /**
   *  @brief Deletes a library 
   */
  void delete_lib (Library *library);

  /**
   *  @brief Get A library from an id
   *
   *  Returns 0, if the library id is not valid.
   */
  Library *lib (lib_id_type id) const;

  /**
   *  @brief Clear all libraries
   *
   *  This method is mainly provided for test purposes.
   */
  void clear ();

private:
  std::vector<Library *> m_libs;
  lib_name_map m_lib_by_name;
  mutable tl::Mutex m_lock;

  LibraryManager ();
  Library *lib_internal (lib_id_type id) const;
};

}


#endif


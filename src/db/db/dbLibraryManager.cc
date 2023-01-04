
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


#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "dbCommon.h"
#include "dbColdProxy.h"

#include "tlAssert.h"
#include "tlStaticObjects.h"
#include "tlClassRegistry.h"
#include "tlThreads.h"

#include <memory>

namespace db
{

static LibraryManager *sp_instance (0);

LibraryManager &
LibraryManager::instance ()
{
  if (sp_instance == 0) {
    sp_instance = new LibraryManager ();
    tl::StaticObjects::reg (&sp_instance);
  }
  return *sp_instance;
}

bool LibraryManager::initialized () 
{
  return sp_instance != 0;
}

LibraryManager::LibraryManager ()
{
  // handle auto-registered libraries:
  for (tl::Registrar<db::Library>::iterator l = tl::Registrar<db::Library>::begin (); l != tl::Registrar<db::Library>::end (); ++l) {
    // take library - no other one will need it
    register_lib (l.take ());
  }
}

LibraryManager::~LibraryManager ()
{
  clear ();
}

std::pair<bool, lib_id_type> 
LibraryManager::lib_by_name (const std::string &name, const std::set<std::string> &for_technologies) const
{
  tl::MutexLocker locker (&m_lock);

  iterator l;

  if (! for_technologies.empty ()) {

    l = m_lib_by_name.find (name);
    while (l != m_lib_by_name.end () && l->first == name) {
      const db::Library *lptr = lib_internal (l->second);
      bool found = lptr->for_technologies ();
      for (std::set<std::string>::const_iterator t = for_technologies.begin (); t != for_technologies.end () && found; ++t) {
        if (! lptr->is_for_technology (*t)) {
          found = false;
        }
      }
      if (found) {
        return std::make_pair (true, l->second);
      }
      ++l;
    }

  }

  //  fallback: technology-unspecific libs
  l = m_lib_by_name.find (name);
  while (l != m_lib_by_name.end () && l->first == name) {
    if (! lib_internal (l->second)->for_technologies ()) {
      return std::make_pair (true, l->second);
    }
    ++l;
  }

  return std::make_pair (false, lib_id_type (0));
}

void
LibraryManager::unregister_lib (Library *library)
{
  if (! library) {
    return;
  }

  {
    tl::MutexLocker locker (&m_lock);

    for (lib_id_type id = 0; id < m_libs.size (); ++id) {
      if (m_libs [id] == library) {
        m_lib_by_name.erase (library->get_name ());
        m_libs [id] = 0;
        break;
      }
    }
  }

  library->remap_to (0);
  library->set_id (std::numeric_limits<lib_id_type>::max ());
}

void
LibraryManager::delete_lib (Library *library)
{
  if (library) {
    unregister_lib (library);
    delete library;
  }
}

lib_id_type 
LibraryManager::register_lib (Library *library)
{
  lib_id_type id = std::numeric_limits<size_t>::max ();
  Library *old_lib = 0;

  {
    tl::MutexLocker locker (&m_lock);

    if (library->get_id () < m_libs.size ()) {
      //  ignore re-registration attempts (they crash)
      tl_assert (m_libs [library->get_id ()] == library);
      return library->get_id ();
    }

    library->keep (); //  marks the library owned by the C++ side of GSI

    for (id = 0; id < m_libs.size (); ++id) {
      if (m_libs [id] == 0) {
        break;
      }
    }

    if (id == m_libs.size ()) {
      m_libs.push_back (library);
    } else {
      m_libs [id] = library;
    }

    library->set_id (id);

    //  if the new library replaces the old one, remap existing library proxies before deleting the library
    //  (replacement is done only when all technologies are substituted)
    lib_name_map::iterator l = m_lib_by_name.find (library->get_name ());
    bool found = false;
    while (l != m_lib_by_name.end () && l->first == library->get_name ()) {
      if (m_libs [l->second] && m_libs [l->second]->get_technologies () == library->get_technologies ()) {
        found = true;
        break;
      }
      ++l;
    }

    if (found) {
      //  substitute
      old_lib = m_libs [l->second];
      m_lib_by_name.erase (l);
    }

    //  insert new lib as first of this name
    l = m_lib_by_name.find (library->get_name ());
    m_lib_by_name.insert (l, std::make_pair (library->get_name (), id));
  }

  if (old_lib) {

    old_lib->remap_to (library);

    {
      //  reset the library pointer only after "remap_to" -> this function may need lib_by_id.
      tl::MutexLocker locker (&m_lock);
      m_libs [old_lib->get_id ()] = 0;
    }

    old_lib->set_id (std::numeric_limits<lib_id_type>::max ());
    delete old_lib;
    old_lib = 0;

  }

  //  take care of cold referrers - these may not get valid
  //  NOTE: this will try to substitute the cold proxies we may have generated during "remap_to" above, but
  //  "restore_proxies" takes care not to re-substitute cold proxies.

  const tl::weak_collection<db::ColdProxy> &cold_proxies = db::ColdProxy::cold_proxies_per_lib_name (library->get_name ());
  std::set<db::Layout *> to_refresh;
  for (tl::weak_collection<db::ColdProxy>::const_iterator p = cold_proxies.begin (); p != cold_proxies.end (); ++p) {
    to_refresh.insert (const_cast<db::Layout *> (p->layout ()));
  }

  for (std::set<db::Layout *>::const_iterator l = to_refresh.begin (); l != to_refresh.end (); ++l) {
    (*l)->restore_proxies (0);
  }

  //  issue the change notification
  changed_event ();

  return id;
}

Library *
LibraryManager::lib (lib_id_type id) const
{
  tl::MutexLocker locker (&m_lock);
  return lib_internal (id);
}

Library *
LibraryManager::lib_internal (lib_id_type id) const
{
  if (id >= m_libs.size ()) {
    return 0;
  } else {
    return m_libs [id];
  }
}

void
LibraryManager::clear ()
{
  std::vector<Library *> libs;

  {
    tl::MutexLocker locker (&m_lock);

    if (m_libs.empty ()) {
      return;
    }

    //  empty the library table before we delete them - this avoid accesses to invalid libraries while doing so.
    libs.swap (m_libs);
    m_lib_by_name.clear ();
  }

  for (std::vector<Library *>::iterator l = libs.begin (); l != libs.end (); ++l) {
    if (*l) {
      (*l)->remap_to (0);
      (*l)->set_id (std::numeric_limits<lib_id_type>::max ());
      delete *l;
    }
  }

  changed_event ();
}

}


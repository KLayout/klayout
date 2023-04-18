
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


#include "dbColdProxy.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "dbLayout.h"
#include "dbLayoutUtils.h"

#include "tlThreads.h"

namespace db
{

static tl::Mutex s_map_mutex;
static std::map<std::string, tl::weak_collection<ColdProxy> *> s_proxies_per_library_name;

const tl::weak_collection<ColdProxy> &
ColdProxy::cold_proxies_per_lib_name (const std::string &libname)
{
  tl::MutexLocker locker (&s_map_mutex);

  std::map<std::string, tl::weak_collection<ColdProxy> *>::const_iterator i = s_proxies_per_library_name.find (libname);
  if (i != s_proxies_per_library_name.end ()) {
    return *i->second;
  } else {
    static tl::weak_collection<ColdProxy> s_empty;
    return s_empty;
  }
}

ColdProxy::ColdProxy (db::cell_index_type ci, db::Layout &layout, const LayoutOrCellContextInfo &info)
  : Cell (ci, layout), mp_context_info (new LayoutOrCellContextInfo (info))
{
  if (! info.lib_name.empty ()) {
    tl::MutexLocker locker (&s_map_mutex);
    std::map<std::string, tl::weak_collection<ColdProxy> *>::iterator i = s_proxies_per_library_name.find (info.lib_name);
    if (i == s_proxies_per_library_name.end ()) {
      i = s_proxies_per_library_name.insert (std::make_pair (info.lib_name, new tl::weak_collection<ColdProxy> ())).first;
    }
    i->second->push_back (this);
  }
}

ColdProxy::~ColdProxy ()
{
  delete mp_context_info;
  mp_context_info = 0;
}

Cell *
ColdProxy::clone (Layout &layout) const
{
  Cell *cell = new ColdProxy (db::Cell::cell_index (), layout, *mp_context_info);
  //  copy the cell content
  *cell = *this;
  return cell;
}

std::string 
ColdProxy::get_basic_name () const
{
  if (! mp_context_info->pcell_name.empty ()) {
    return mp_context_info->pcell_name;
  } else if (! mp_context_info->cell_name.empty ()) {
    return mp_context_info->cell_name;
  } else {
    return Cell::get_basic_name ();
  }
}

std::string 
ColdProxy::get_display_name () const
{
  if (! mp_context_info->lib_name.empty ()) {
    std::string stem = "<defunct>" + mp_context_info->lib_name + ".";
    if (! mp_context_info->pcell_name.empty ()) {
      return stem + mp_context_info->pcell_name;
    } else if (! mp_context_info->cell_name.empty ()) {
      return stem + mp_context_info->cell_name;
    } else {
      return stem + "<unknown>";
    }
  } else {
    return Cell::get_display_name ();
  }
}

std::string
ColdProxy::get_qualified_name () const
{
  if (! mp_context_info->lib_name.empty ()) {
    std::string stem = "<defunct>" + mp_context_info->lib_name + ".";
    if (! mp_context_info->pcell_name.empty ()) {
      if (mp_context_info->pcell_parameters.empty ()) {
        return stem + mp_context_info->pcell_name;
      } else {
        //  TODO: list parameters? Might be long.
        return stem + mp_context_info->pcell_name + "(...)";
      }
    } else if (! mp_context_info->cell_name.empty ()) {
      return stem + mp_context_info->cell_name;
    } else {
      return stem + "<unknown>";
    }
  } else {
    return Cell::get_qualified_name ();
  }
}

}


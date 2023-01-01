
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


#ifndef HDR_dbLibraryDescriptor
#define HDR_dbLibraryDescriptor

#include "dbCommon.h"
#include "gsiObject.h"
#include "dbLayout.h"
#include "tlTypeTraits.h"
#include "tlObject.h"

#include <string>
#include <set>

namespace db
{

class Layout;

/**
 *  @brief A library
 *
 *  A library is basically a wrapper around a layout object.
 *  A library is additionally associated with an id, a name and a description.
 *  A library must provide a layout. This class does not specify how the layout 
 *  is provided. To do so, this class must be reimplemented.
 */
class DB_PUBLIC Library
  : public gsi::ObjectBase, public tl::Object
{
public:
  /**
   *  @brief The constructor
   */
  Library();

  /**
   *  @brief Copy constructor
   */
  Library(const Library &);

  /**
   *  @brief The destructor
   */
  virtual ~Library ();

  /**
   *  @brief The layout object
   *
   *  This method must be reimplemented by some derived class to actually provide the
   *  layout or the derived class fills the layout.
   */
  virtual db::Layout &layout () 
  {
    return m_layout;
  }

  /**
   *  @brief Get the const layout
   *
   *  This version uses the non-const, virtual implementation to provide a const
   *  layout accessor.
   */
  const db::Layout &layout () const
  {
    return (const_cast<Library *> (this))->layout ();
  }

  /**
   *  @brief Getter for the name property
   */
  const std::string &get_name () const
  {
    return m_name;
  }

  /**
   *  @brief Setter for the name property
   */
  void set_name (const std::string &name) 
  {
    m_name = name;
  }

  /**
   *  @brief Gets the technology name this library is associated with
   *
   *  If this attribute is non-empty, the library is selected only when the given technology is
   *  used for the layout.
   */
  const std::set<std::string> &get_technologies () const
  {
    return m_technologies;
  }

  /**
   *  @brief Gets a value indicating whether this library is associated with the given technology
   */
  bool is_for_technology (const std::string &name) const;

  /**
   *  @brief Gets a value indicating whether the library is associated with any technology
   */
  bool for_technologies () const;

  /**
   *  @brief Sets the technology name this library is associated with
   *
   *  This will reset the list of technologies to this one.
   *  If the given technology string is empty, the list of technologies will be cleared.
   */
  void set_technology (const std::string &t);

  /**
   *  @brief Clears the list of technologies this library is associated with
   */
  void clear_technologies ();

  /**
   *  @brief Additionally associate the library with the given technology
   */
  void add_technology (const std::string &tech);

  /**
   *  @brief Getter for the description property
   */
  const std::string &get_description () const
  {
    return m_description;
  }

  /**
   *  @brief Setter for the description property
   */
  void set_description (const std::string &description) 
  {
    m_description = description;
  }

  /**
   *  @brief Getter for the library Id property
   */
  lib_id_type get_id () const
  {
    return m_id;
  }

  /**
   *  @brief Setter for the library Id property
   */
  void set_id (lib_id_type id) 
  {
    m_id = id;
  }

  /**
   *  @brief Register a LibraryProxy in the given layout
   */
  void register_proxy (db::LibraryProxy *lib_proxy, db::Layout *layout);
  
  /**
   *  @brief Unregister the Library proxy
   */
  void unregister_proxy (db::LibraryProxy *lib_proxy, db::Layout *layout);

  /**
   *  @brief Retires a LibraryProxy in the given layout
   *
   *  A proxy becomes entirely retired if the refcount is equal to the
   *  retired count. This feature is used to decide whether a proxy
   *  is actually used or only present as a shadow object for the transaction
   *  management.
   */
  void retire_proxy (db::LibraryProxy *lib_proxy);

  /**
   *  @brief Unretires the Library proxy
   */
  void unretire_proxy (db::LibraryProxy *lib_proxy);

  /**
   *  @brief Gets a value indicating whether a proxy is entirely retired
   */
  bool is_retired (const cell_index_type library_cell_index) const;

  /**
   *  @brief Refreshes the library on all clients
   *
   *  This will refresh PCells, retire cells (turn them into "cold proxies") and reload layouts.
   */
  void refresh ();

  /**
   *  @brief Remap the library proxies to a different library
   *
   *  After remapping, "other" can replace "this".
   */
  void remap_to (db::Library *other);

  /**
   *  @brief This event is fired if proxies get retired on unretired
   */
  tl::Event retired_state_changed_event;

private:
  std::string m_name;
  std::string m_description;
  std::set<std::string> m_technologies;
  lib_id_type m_id;
  db::Layout m_layout;
  std::map<db::Layout *, int> m_referrers;
  std::map<db::cell_index_type, int> m_refcount, m_retired_count;

  // no copying.
  Library &operator=(const Library &);
};

}

#endif




/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#ifndef HDR_laySalt
#define HDR_laySalt

#include "layCommon.h"
#include "laySaltGrain.h"
#include "laySaltGrains.h"

#include <QObject>

namespace lay
{

/**
 *  @brief The global salt (package manager) object
 *  This object can be configured to represent a couple of locations.
 *  It will provide a collection of grains for these locations.
 */
class LAY_PUBLIC Salt
  : public QObject
{
Q_OBJECT

public:
  typedef SaltGrains::collection_iterator iterator;
  typedef std::vector<SaltGrain *>::const_iterator flat_iterator;

  /**
   *  @brief Default constructor
   */
  Salt ();

  /**
   *  @brief Copy constructor
   */
  Salt (const Salt &other);

  /**
   *  @brief assignment
   */
  Salt &operator= (const Salt &other);

  /**
   *  @brief Adds the given location to the ones the package manager uses
   *  Adding a location will scan the folder and make the contents available
   *  as a new collection.
   */
  void add_location (const std::string &path);

  /**
   *  @brief Removes a given location
   *  This will remove the collection from the package locations.
   */
  void remove_location (const std::string &path);

  /**
   *  @brief Refreshes the collections
   *  This method rescans all registered locations.
   */
  void refresh ();

  /**
   *  @brief Iterates the collections (begin)
   */
  iterator begin () const
  {
    return m_root.begin_collections ();
  }

  /**
   *  @brief Iterates the collections (end)
   */
  iterator end () const
  {
    return m_root.end_collections ();
  }

  /**
   *  @brief A flat iterator of (sorted) grains (begin)
   */
  flat_iterator begin_flat ()
  {
    ensure_flat_present ();
    return mp_flat_grains.begin ();
  }

  /**
   *  @brief A flat iterator of (sorted) grains (end)
   */
  flat_iterator end_flat ()
  {
    ensure_flat_present ();
    return mp_flat_grains.end ();
  }

signals:
  /**
   *  @brief A signal triggered when one of the collections changed
   */
  void collections_changed ();

private:
  SaltGrains m_root;
  std::vector<SaltGrain *> mp_flat_grains;

  void ensure_flat_present ();
  void add_collection_to_flat (lay::SaltGrains &gg);
};

}

#endif

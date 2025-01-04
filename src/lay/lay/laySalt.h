
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include <map>

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
   *  @brief Returns a value indicating whether the collection is empty
   */
  bool is_empty () const
  {
    return m_root.is_empty ();
  }

  /**
   *  @brief A flat iterator of (sorted) grains (begin)
   */
  flat_iterator begin_flat ();

  /**
   *  @brief A flat iterator of (sorted) grains (end)
   */
  flat_iterator end_flat ();

  /**
   *  @brief Gets the grain with the given name
   */
  SaltGrain *grain_by_name (const std::string &name);

  /**
   *  @brief Gets the grain with the given name (const version)
   */
  const SaltGrain *grain_by_name (const std::string &name) const
  {
    return const_cast<Salt *> (this)->grain_by_name (name);
  }

  /**
   *  @brief Loads the salt from a "salt mine" file
   */
  void load (const std::string &p)
  {
    m_root.load (p);
  }

  /**
   *  @brief Loads the salt from a "salt mine" stream
   */
  void load (const std::string &p, tl::InputStream &s)
  {
    m_root.load (p, s);
  }

  /**
   *  @brief Saves the salt to a "salt mine" file
   *  This feature is provided for debugging purposes mainly.
   */
  void save (const std::string &p)
  {
    m_root.save (p);
  }

  /**
   *  @brief Removes a grain from the salt
   *
   *  This operation will remove the grain with the given name from the salt and delete all files and directories related to it.
   *  If multiple grains with the same name exist, they will all be removed.
   *
   *  Returns true, if the package could be removed successfully.
   */
  bool remove_grain (const SaltGrain &grain);

  /**
   *  @brief Creates a new grain from a template
   *
   *  This method will create a folder for a grain with the given path and download or copy
   *  all files related to this grain. It will copy the download URL from the template into the
   *  new grain, so updates will come from the original location.
   *
   *  If the target's name is not set, it will be taken from the template.
   *  If the target's path is not set and a grain with the given name already exists in
   *  the package, the path is taken from that grain.
   *  If no target path is set and no grain with this name exists yet, a new path will
   *  be constructed using the first location in the salt.
   *
   *  The target grain will be updated with the installation information. If the target grain
   *  contains an installation path prior to the installation, this path will be used for the
   *  installation of the grain files.
   *
   *  Returns true, if the package could be created successfully.
   */
  bool create_grain (const SaltGrain &templ, SaltGrain &target, double timeout = 60.0, tl::InputHttpStreamCallback *callback = 0);

  /**
   *  @brief Removes redundant entries with same names
   *
   *  This method will keep the first entry or the one with the higher version.
   */
  void consolidate ();

  /**
   *  @brief Gets the root collection
   *
   *  This method is provided for test purposes mainly.
   */
  SaltGrains &root ();

  /**
   *  @brief Gets a value indicating whether the collection wants package information to be downloaded always
   */
  bool download_package_information () const;

signals:
  /**
   *  @brief A signal triggered before one of the collections changed
   */
  void collections_about_to_change ();

  /**
   *  @brief A signal triggered when one of the collections changed
   */
  void collections_changed ();

private:
  SaltGrains m_root;
  std::vector<SaltGrain *> mp_flat_grains;
  std::map<std::string, SaltGrain *> m_grains_by_name;

  void validate ();
  void invalidate ();
  void add_collection_to_flat (lay::SaltGrains &gg);
};

}

#endif

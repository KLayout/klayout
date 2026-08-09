
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef HDR_dbFileBasedLibrary
#define HDR_dbFileBasedLibrary

#include "dbLibrary.h"
#include "dbCommon.h"

#include <string>
#include <list>

namespace db
{

/**
 *  @brief A Library specialization that ties a library to a file
 *
 *  This object supports loading a library from multiple files and merging them into
 *  a single library (e.g. for loading a directory of cell files).
 */
class DB_PUBLIC FileBasedLibrary
  : public db::Library
{
public:
  /**
   *  @brief Creates a file-based library object
   *
   *  @param path The file path
   *  @param name The library name
   *
   *  If the library name is an empty string, the library name is taken from
   *  a GDS LIBNAME or from the file name in the path.
   *
   *  Note that you need to call "load" in order to actually load the file.
   */
  FileBasedLibrary (const std::string &path, const std::string &name = std::string ());

  /**
   *  @brief Merges another file into this library
   *
   *  If the library was not loaded already, the merge requests are postponed
   *  until "load" is called.
   */
  void merge_with_other_layout (const std::string &path);

  /**
   *  @brief Loads the files
   *
   *  If the files are already loaded, this method does nothing.
   *  It returns the name of the library derived from the first file
   *  or the name given in the constructor. The constructor name has
   *  priority.
   */
  std::string load ();

  /**
   *  @brief Implements the reload feature
   */
  virtual std::string reload ();

  /**
   *  @brief Set the paths
   *  This method is provided for test purposes only.
   */
  void set_paths (const std::string &path, const std::list<std::string> &other_paths = std::list<std::string> ())
  {
    m_path = path;
    m_other_paths = other_paths;
  }

  /**
   *  @brief Gets a value indicating whether the library is for the given path
   */
  bool is_for_path (const std::string &path);

  /**
   *  @brief Gets a value indicating whether the library is for the given path
   */
  bool is_for_paths (const std::vector<std::string> &paths);

private:
  std::string m_name;
  std::string m_path;
  std::list<std::string> m_other_paths;
  bool m_is_loaded;

  void merge_impl (const std::string &path);
};

}

#endif



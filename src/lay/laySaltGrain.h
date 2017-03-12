
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

#ifndef HDR_laySaltGrain
#define HDR_laySaltGrain

#include "layCommon.h"
#include "tlObject.h"

namespace lay
{

/**
 *  @brief This class represents on grain of salt
 *  "One grain of salt" is one package.
 */
class LAY_PUBLIC SaltGrain
  : public tl::Object
{
public:
  /**
   *  @brief A descriptor for one dependency
   *  A dependency can be specified either through a name (see name property)
   *  or a download URL. If download URL are specified, they have precedence
   *  over names.
   *  The version is the minimum required version. If empty, any version is
   *  allowed to resolve this dependency.
   */
  struct Dependency
  {
    std::string name;
    std::string url;
    std::string version;

    bool operator== (const Dependency &other) const
    {
      return name == other.name && url == other.url && version == other.version;
    }
  };

  /**
   *  @brief Constructor
   */
  SaltGrain ();

  /**
   *  @brief Equality
   */
  bool operator== (const SaltGrain &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const SaltGrain &other) const
  {
    return !operator== (other);
  }

  /**
   *  @brief Gets the name of the grain
   *
   *  The name is either a plain name (a word) or a path into a collection.
   *  Name paths are formed using the "/" separator. "mypackage" is a plain name,
   *  while "mycollection/mypackage" is a package within a collection. Collections
   *  can be used to group packages. Names are case sensitive in general, but
   *  names differing only in case should be avoided.
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the name of the grain
   */
  void set_name (const std::string &p);

  /**
   *  @brief Gets the title of the grain
   *
   *  The title is a brief description that is shown in the title of the
   *  package manager.
   */
  const std::string &title () const
  {
    return m_title;
  }

  /**
   *  @brief Sets the title of the grain
   */
  void set_title (const std::string &t);

  /**
   *  @brief Gets the documentation text of the grain
   *
   *  The documentation text is an XML document using
   *  KLayout's doc format.
   */
  const std::string &doc () const
  {
    return m_doc;
  }

  /**
   *  @brief Sets the documentation text of the grain
   */
  void set_doc (const std::string &t);

  /**
   *  @brief Gets the version of the grain
   *
   *  A version string is of the form "x.y..." where x, y and other version
   *  components are integer numbers.
   */
  const std::string &version () const
  {
    return m_version;
  }

  /**
   *  @brief Sets the version of the grain
   */
  void set_version (const std::string &v);

  /**
   *  @brief Gets the absolute file path of the installed grain
   *  This is the file path to the grain folder.
   */
  const std::string &path () const
  {
    return m_path;
  }

  /**
   *  @brief Sets the absolute file path of the installed grain
   */
  void set_path (const std::string &p);

  /**
   *  @brief Gets the download URL
   *  The download URL is the place from which the grain was installed originally.
   */
  const std::string &url () const
  {
    return m_url;
  }

  /**
   *  @brief Sets the download URL
   */
  void set_url (const std::string &u);

  /**
   *  @brief Gets the dependencies of the grain
   *  Grains this grain depends on are installed automatically when the grain
   *  is installed.
   */
  const std::vector<Dependency> &dependencies () const
  {
    return m_dependencies;
  }

  /**
   *  @brief Gets the dependencies of the grain (non-const)
   */
  std::vector<Dependency> &dependencies ()
  {
    return m_dependencies;
  }

  /**
   *  @brief Dependency iterator (begin)
   */
  std::vector<Dependency>::const_iterator begin_dependencies () const
  {
    return m_dependencies.begin ();
  }

  /**
   *  @brief Dependency iterator (end)
   */
  std::vector<Dependency>::const_iterator end_dependencies () const
  {
    return m_dependencies.end ();
  }

  /**
   *  @brief Adds a dependency
   */
  void add_dependency (const Dependency &dep)
  {
    m_dependencies.push_back (dep);
  }

  /**
   *  @brief Loads the data from a given file
   *  This method will *not* set the path.
   */
  void load (const std::string &file_path);

  /**
   *  @brief Saves the data to the path inside the grain folder given by the "path" property
   */
  void save () const;

  /**
   *  @brief Saves the data to the given file
   */
  void save (const std::string &file_path) const;

  /**
   *  @brief Compares two version strings
   *  Returns -1 if v1 < v2, 0 if v1 == v2 and 1 if v1 > v2.
   *  Malformed versions are read gracefully. Letters and non-digits are skipped.
   *  Missing numbers are read as 0. Hence "1.0 == 1" for example.
   */
  static int compare_versions (const std::string &v1, const std::string &v2);

  /**
   *  @brief Detects a grain from the given directory
   *  This method will return a grain constructed from the given directory.
   *  The data is read from "path/grain.xml". This method will throw an
   *  exception if an error occurs during reading.
   */
  static SaltGrain from_path (const std::string &path);

  /**
   *  @brief Returns a value indicating whether the given path represents is a grain
   */
  static bool is_grain (const std::string &path);

private:
  std::string m_name;
  std::string m_version;
  std::string m_path;
  std::string m_url;
  std::string m_title;
  std::string m_doc;
  std::vector<Dependency> m_dependencies;
};

}

#endif

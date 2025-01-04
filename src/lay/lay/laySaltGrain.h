
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

#ifndef HDR_laySaltGrain
#define HDR_laySaltGrain

#include "layCommon.h"
#include "tlObject.h"
#include "tlStream.h"
#include "tlXMLParser.h"

#include <QTime>
#include <QImage>

namespace tl
{
  class InputHttpStreamCallback;
}

namespace lay
{

/**
 *  @brief A descriptor for one dependency
 *  A dependency can be specified either through a name (see name property)
 *  or a download URL. If download URL are specified, they have precedence
 *  over names.
 *  The version is the minimum required version. If empty, any version is
 *  allowed to resolve this dependency.
 */
struct SaltGrainDependency
{
  SaltGrainDependency ()
  { }

  std::string name;
  std::string url;
  std::string version;

  bool operator== (const SaltGrainDependency &other) const
  {
    return name == other.name && url == other.url && version == other.version;
  }
};

/**
 *  @brief This class represents on grain of salt
 *  "One grain of salt" is one package.
 */
class LAY_PUBLIC SaltGrain
  : public tl::Object
{
public:
  /**
   *  @brief Constructor
   */
  SaltGrain ();

  /**
   *  @brief Destructor
   */
  virtual ~SaltGrain () { }

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
   *  @brief Gets the token of the grain
   *
   *  The grain's token is a unique identifier by which external systems can
   *  identify the package - for example for package voting.
   *  A token is not part of the package's identity. It's intended as a
   *  package identification safe against forgery.
   */
  const std::string &token () const
  {
    return m_token;
  }

  /**
   *  @brief Sets the token of the grain
   */
  void set_token (const std::string &t);

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
   *  The documentation text is a brief description.
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
   *  @brief Gets the documentation URL of the grain
   *
   *  The documentation URL provides a detailed documentation.
   */
  const std::string &doc_url () const
  {
    return m_doc_url;
  }

  /**
   *  @brief Sets the documentation URL of the grain
   */
  void set_doc_url (const std::string &u);

  /**
   *  @brief Gets the effective documentation URL
   *
   *  The effective documentation URL is formed from the installation path
   *  and the documentation URL if the latter is a relative one.
   */
  std::string eff_doc_url () const;

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
   *  @brief Gets the API version of the grain
   *
   *  The API version is the KLayout version required to run the grain's macros.
   *  A version string is of the form "x.y..." where x, y and other version
   *  components are integer numbers.
   *
   *  The version string can also list other features such as ruby version etc.
   *  The components are separated with a semicolon.
   *  For example, "0.26; ruby; python 3.0" means: requires KLayout API 0.26,
   *  ruby (any version) any python (>= 3.0).
   */
  const std::string &api_version () const
  {
    return m_api_version;
  }

  /**
   *  @brief Sets the API version of the grain
   */
  void set_api_version (const std::string &v);

  /**
   *  @brief Gets the author of the grain
   */
  const std::string &author () const
  {
    return m_author;
  }

  /**
   *  @brief Sets the author of the grain
   */
  void set_author (const std::string &a);

  /**
   *  @brief Gets the author's contact
   */
  const std::string &author_contact () const
  {
    return m_author_contact;
  }

  /**
   *  @brief Sets the author's contact
   */
  void set_author_contact (const std::string &a);

  /**
   *  @brief Gets the license of the grain
   */
  const std::string &license () const
  {
    return m_license;
  }

  /**
   *  @brief Sets the license of the grain
   */
  void set_license (const std::string &l);

  /**
   *  @brief Gets the release date and/or time of the grain
   */
  const QDateTime &authored_time () const
  {
    return m_authored_time;
  }

  /**
   *  @brief Sets the release date and/or time
   */
  void set_authored_time (const QDateTime &t);

  /**
   *  @brief Gets the installation date and/or time of the grain
   */
  const QDateTime &installed_time () const
  {
    return m_installed_time;
  }

  /**
   *  @brief Sets the installation date and/or time
   */
  void set_installed_time (const QDateTime &t);

  /**
   *  @brief Gets the icon image for the grain.
   *  The preferred image size is 64x64 pixels.
   *  The image may be null image. In this case, a default image is used.
   */
  const QImage &icon () const
  {
    return m_icon;
  }

  /**
   *  @brief Sets icon image
   */
  void set_icon (const QImage &i);

  /**
   *  @brief Gets a screenshot image for documentation.
   *  The image may be null image. In this case, no screenshot is shown.
   */
  const QImage &screenshot () const
  {
    return m_screenshot;
  }

  /**
   *  @brief Sets screenshot image
   */
  void set_screenshot (const QImage &i);

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
   *  @brief Gets a value indicating whether the grain is hidden
   *  A grain can be hidden (in Salt.Mine) if it's a pure dependency package
   *  which is only there because others need it. Such packages are listed
   *  as dependencies, but they are not shown by default.
   */
  bool is_hidden () const
  {
    return m_hidden;
  }

  /**
   *  @brief Sets a value indicating whether the grain is hidden
   */
  void set_hidden (bool f);

  /**
   *  @brief Gets the dependencies of the grain
   *  Grains this grain depends on are installed automatically when the grain
   *  is installed.
   */
  const std::vector<SaltGrainDependency> &dependencies () const
  {
    return m_dependencies;
  }

  /**
   *  @brief Gets the dependencies of the grain (non-const)
   */
  std::vector<SaltGrainDependency> &dependencies ()
  {
    return m_dependencies;
  }

  /**
   *  @brief Dependency iterator (begin)
   */
  std::vector<SaltGrainDependency>::const_iterator begin_dependencies () const
  {
    return m_dependencies.begin ();
  }

  /**
   *  @brief Dependency iterator (end)
   */
  std::vector<SaltGrainDependency>::const_iterator end_dependencies () const
  {
    return m_dependencies.end ();
  }

  /**
   *  @brief Adds a dependency
   */
  void add_dependency (const SaltGrainDependency &dep)
  {
    m_dependencies.push_back (dep);
  }

  /**
   *  @brief Returns true, if the collection is read-only
   */
  bool is_readonly () const;

  /**
   *  @brief Loads the data from a given file
   *  This method will *not* set the path.
   */
  void load (const std::string &file_path);

  /**
   *  @brief Loads the data from a given stream
   */
  void load (tl::InputStream &stream);

  /**
   *  @brief Saves the data to the path inside the grain folder given by the "path" property
   */
  void save () const;

  /**
   *  @brief Saves the data to the given file
   */
  void save (const std::string &file_path) const;

  /**
   *  @brief Gets the XML structure representing a grain
   */
  static tl::XMLElementList &xml_elements ();

  /**
   *  @brief Compares two version strings
   *  Returns -1 if v1 < v2, 0 if v1 == v2 and 1 if v1 > v2.
   *  Malformed versions are read gracefully. Letters and non-digits are skipped.
   *  Missing numbers are read as 0. Hence "1.0 == 1" for example.
   */
  static int compare_versions (const std::string &v1, const std::string &v2);

  /**
   *  @brief Gets a value indicating whether the given version string is a valid version
   */
  static bool valid_version (const std::string &v);

  /**
   *  @brief Gets a value indicating whether the given version string is a valid API version string
   */
  static bool valid_api_version (const std::string &v);

  /**
   *  @brief Checks whether the given string is a valid name
   */
  static bool valid_name (const std::string &n);

  /**
   *  @brief Detects a grain from the given directory
   *  This method will return a grain constructed from the given directory.
   *  The data is read from "path/grain.xml". This method will throw an
   *  exception if an error occurs during reading.
   */
  static SaltGrain from_path (const std::string &path);

  /**
   *  @brief Loads the grain from the given URL
   *  This method will return a grain constructed from the downloaded data.
   *  The data is read from "URL/grain.xml". This method will throw an
   *  exception if an error occurs during reading.
   *
   *  CAUTION: with GIT protocol and large repositories, this function may be very expensive.
   */
  static SaltGrain from_url (const std::string &url, double timeout = 60.0, tl::InputHttpStreamCallback *callback = 0);

  /**
   *  @brief Returns a stream prepared for downloading the grain
   *  The stream is a new'd object and needs to be deleted by the caller.
   *  "url" is the download URL on input and gets modified to match the
   *  actual URL if it is a relative one.
   *
   *  CAUTION: with GIT protocol and large repositories, this function may be very expensive.
   */
  static tl::InputStream *stream_from_url (std::string &url, double timeout = 60.0, tl::InputHttpStreamCallback *callback = 0);

  /**
   *  @brief Gets the name of the spec file ("grain.xml")
   */
  static const std::string &spec_file ();

  /**
   *  @brief Returns a value indicating whether the given path represents is a grain
   */
  static bool is_grain (const std::string &path);

private:
  std::string m_name;
  std::string m_token;
  std::string m_version;
  std::string m_api_version;
  std::string m_path;
  std::string m_url;
  std::string m_title;
  std::string m_doc, m_doc_url;
  std::string m_author;
  std::string m_author_contact;
  std::string m_license;
  bool m_hidden;
  QDateTime m_authored_time, m_installed_time;
  QImage m_icon, m_screenshot;
  std::vector<SaltGrainDependency> m_dependencies;
};

}

#endif

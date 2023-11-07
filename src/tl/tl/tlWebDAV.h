
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


#ifndef HDR_tlWebDAV
#define HDR_tlWebDAV

#include "tlCommon.h"
#include "tlStream.h"

#include <string>
#include <vector>

#if !defined(HAVE_CURL) && !defined(HAVE_QT)
#  error "tlWebDAV.h can only be used with either curl or Qt enabled"
#endif

namespace tl
{

class InputHttpStreamCallback;

/**
 *  @brief Represents an item in a WebDAV collection
 */
class TL_PUBLIC WebDAVItem
{
public:
  /**
   *  @brief Default constructor
   */
  WebDAVItem ()
    : m_is_collection (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructor
   */
  WebDAVItem (bool is_collection, const std::string &url, const std::string &name)
    : m_is_collection (is_collection), m_url (url), m_name (name)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets a value indicating whether this item is a collection
   *  If false, it's a file.
   */
  bool is_collection () const
  {
    return m_is_collection;
  }

  /**
   *  @brief Gets the URL of this item
   */
  const std::string &url () const
  {
    return m_url;
  }

  /**
   *  @brief Gets the name of this item
   *  The name is only valid for sub-items.
   */
  const std::string &name () const
  {
    return m_name;
  }

protected:
  bool m_is_collection;
  std::string m_url;
  std::string m_name;
};

/**
 *  @brief Represents an object from a WebDAV URL
 *  This object can be a file or collection
 */
class TL_PUBLIC WebDAVObject
  : public WebDAVItem
{
public:
  typedef std::vector<WebDAVItem> container;
  typedef container::const_iterator iterator;

  /**
   *  @brief Open a stream with the given URL
   */
  WebDAVObject ();

  /**
   *  @brief Populates the collection from the given URL
   *  The depth value can be 0 (self only) or 1 (self + collection members).
   */
  void read (const std::string &url, int depth, double timeout = 60.0, tl::InputHttpStreamCallback *callback = 0);

  /**
   *  @brief Gets the items of this collection (begin iterator)
   */
  iterator begin () const
  {
    return m_items.begin ();
  }

  /**
   *  @brief Gets the items of this collection (begin iterator)
   */
  iterator end () const
  {
    return m_items.end ();
  }

  /**
   *  @brief Downloads the collection or file with the given URL
   *
   *  This method will download the WebDAV object from url to the file path
   *  given in "target".
   *
   *  For file download, the target must be the path of the target file.
   *  For collection download, the target must be a directory path. In this
   *  case, the target directory must exist already.
   *
   *  Sub-directories are created if required.
   *
   *  This method returns false if the directory structure could
   *  not be obtained or downloading of one file failed.
   */
  static bool download (const std::string &url, const std::string &target, double timeout = 60.0, tl::InputHttpStreamCallback *callback = 0);

  /**
   *  @brief Gets a stream object for downloading the single item of the given URL
   *
   *  The stream object returned needs to be deleted by the caller.
   */
  static tl::InputStream *download_item (const std::string &url, double timeout = 60.0, tl::InputHttpStreamCallback *callback = 0);

private:
  container m_items;
};

}

#endif



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


#include "tlWebDAV.h"
#include "tlXMLParser.h"
#include "tlHttpStream.h"
#include "tlStream.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlLog.h"
#include "tlUri.h"
#include "tlFileUtils.h"

#include <memory>

namespace tl
{

// ---------------------------------------------------------------
//  WebDAVCollection implementation

WebDAVObject::WebDAVObject ()
{
  //  .. nothing yet ..
}

namespace
{

/**
 *  @brief A dummy "DOM" for the WebDAV reply
 */
struct ResourceType
{
  ResourceType () : is_collection (false) { }

  const std::string &collection () const
  {
    static std::string empty;
    return empty;
  }

  void set_collection (const std::string &)
  {
    is_collection = true;
  }

  bool is_collection;
};

/**
 *  @brief A dummy "DOM" for the WebDAV reply
 */
struct Prop
{
  ResourceType resourcetype;
};

/**
 *  @brief A dummy "DOM" for the WebDAV reply
 */
struct PropStat
{
  std::string status;
  Prop prop;
};

/**
 *  @brief A dummy "DOM" for the WebDAV reply
 */
struct Response
{
  std::string href;
  PropStat propstat;
};

/**
 *  @brief A dummy "DOM" for the WebDAV reply
 */
struct MultiStatus
{
  typedef std::list<Response> container;
  typedef container::const_iterator iterator;

  iterator begin () const { return responses.begin (); }
  iterator end () const { return responses.end (); }
  void add (const Response &r) { responses.push_back (r); }

  container responses;
};

}

tl::XMLStruct<MultiStatus> xml_struct ("multistatus",
  tl::make_element (&MultiStatus::begin, &MultiStatus::end, &MultiStatus::add, "response",
    tl::make_member (&Response::href, "href") +
    tl::make_element (&Response::propstat, "propstat",
      tl::make_member (&PropStat::status, "status") +
      tl::make_element (&PropStat::prop, "prop",
        tl::make_element (&Prop::resourcetype, "resourcetype",
          tl::make_member (&ResourceType::collection, &ResourceType::set_collection, "collection")
        )
      )
    )
  )
);

static std::string item_name (const std::string &path1, const std::string &path2)
{
  std::vector <std::string> sl1 = tl::split (path1, "/");
  if (! sl1.empty () && sl1.back ().empty ()) {
    sl1.pop_back ();
  }

  std::vector <std::string> sl2 = tl::split (path2, "/");
  if (! sl2.empty () && sl2.back ().empty ()) {
    sl2.pop_back ();
  }

  if (sl1 == sl2) {
    //  This is the top-level item (echoed in the PROPFIND response)
    return std::string ();
  } else if (! sl2.empty ()) {
    return sl2.back ();
  } else {
    throw tl::Exception (tl::to_string (tr ("Invalid WebDAV response: %s is not a collection sub-item of %s")), path2, path1);
  }
}

void
WebDAVObject::read (const std::string &url, int depth, double timeout, tl::InputHttpStreamCallback *callback)
{
  tl::URI base_uri (url);

  tl::InputHttpStream http (url);
  http.set_timeout (timeout);
  http.set_callback (callback);
  http.add_header ("User-Agent", "SVN");
  http.add_header ("Depth", tl::to_string (depth));
  http.set_request ("PROPFIND");
  http.set_data ("<?xml version=\"1.0\" encoding=\"utf-8\"?><propfind xmlns=\"DAV:\"><prop><resourcetype xmlns=\"DAV:\"/></prop></propfind>");

  MultiStatus multistatus;
  tl::InputStream stream (http);
  tl::XMLStreamSource source (stream);
  xml_struct.parse (source, multistatus);

  //  TODO: check status ..

  m_items.clear ();
  for (MultiStatus::iterator r = multistatus.begin (); r != multistatus.end (); ++r) {

    bool is_collection = r->propstat.prop.resourcetype.is_collection;
    tl::URI item_url = base_uri.resolved (tl::URI (r->href));

    std::string n = item_name (base_uri.path (), item_url.path ());
    std::string item_url_string = item_url.to_string ();

    if (! n.empty ()) {
      m_items.push_back (WebDAVItem (is_collection, item_url_string, n));
    } else {
      m_is_collection = is_collection;
      m_url = item_url_string;
    }

  }
}

namespace
{

struct DownloadItem
{
  DownloadItem (const std::string &u, const std::string &p)
  {
    url = u;
    path = p;
  }

  std::string url;
  std::string path;
};

}

static
void fetch_download_items (const std::string &url, const std::string &target, std::list<DownloadItem> &items, tl::AbsoluteProgress &progress, double timeout, tl::InputHttpStreamCallback *callback)
{
  ++progress;

  WebDAVObject object;
  object.read (url, 1, timeout, callback);

  if (object.is_collection ()) {

    if (! tl::file_exists (target)) {
      throw tl::Exception (tl::to_string (tr ("Download failed: target directory '%s' does not exists")), target);
    }

    for (WebDAVObject::iterator i = object.begin (); i != object.end (); ++i) {

      std::string item_path = tl::absolute_file_path (tl::combine_path (target, i->name ()));

      if (i->is_collection ()) {

        if (! tl::file_exists (item_path)) {
          if (! tl::mkpath (item_path)) {
            throw tl::Exception (tl::to_string (tr ("Download failed: unable to create subdirectory '%s' in '%s'")), i->name (), target);
          }
        } else if (! tl::is_dir (item_path)) {
          throw tl::Exception (tl::to_string (tr ("Download failed: unable to create subdirectory '%s' in '%s' - is already a file")), i->name (), target);
        } else if (! tl::is_writable (item_path)) {
          throw tl::Exception (tl::to_string (tr ("Download failed: unable to create subdirectory '%s' in '%s' - no write permissions")), i->name (), target);
        }

        fetch_download_items (i->url (), item_path, items, progress, timeout, callback);

      } else {

        if (tl::file_exists (item_path) && ! tl::is_writable (item_path)) {
          throw tl::Exception (tl::to_string (tr ("Download failed: file is '%s' in '%s' - already exists, but no write permissions")), i->name (), target);
        }

        items.push_back (DownloadItem (i->url (), item_path));

      }
    }

  } else {
    items.push_back (DownloadItem (url, target));
  }
}

tl::InputStream *
WebDAVObject::download_item (const std::string &url, double timeout, tl::InputHttpStreamCallback *callback)
{
  tl::InputHttpStream *http = new tl::InputHttpStream (url);
  http->set_timeout (timeout);
  http->set_callback (callback);
  //  This trick allows accessing GitHub repos through their SVN API
  http->add_header ("User-Agent", "SVN");
  return new tl::InputStream (http);
}

bool
WebDAVObject::download (const std::string &url, const std::string &target, double timeout, tl::InputHttpStreamCallback *callback)
{
  std::list<DownloadItem> items;

  try {

    tl::info << tr ("Fetching file structure from ") << url;
    tl::AbsoluteProgress progress (tl::sprintf (tl::to_string (tr ("Fetching directory structure from %s")), url));
    fetch_download_items (url, target, items, progress, timeout, callback);

  } catch (tl::Exception &ex) {
    tl::error << tr ("Error downloading file structure from '") << url << "':" << tl::endl << ex.msg ();
    return false;
  }

  bool has_errors = false;

  {
    tl::info << tl::sprintf (tl::to_string (tr ("Downloading %d file(s) now ..")), items.size ());

    tl::RelativeProgress progress (tl::sprintf (tl::to_string (tr ("Downloading file(s) from %s")), url), items.size (), 1);

    for (std::list<DownloadItem>::const_iterator i = items.begin (); i != items.end (); ++i) {

      tl::info << tl::sprintf (tl::to_string (tr ("Downloading '%s' to '%s' ..")), i->url, i->path);

      try {

        tl::OutputStream os (i->path);
        std::unique_ptr<tl::InputStream> is (download_item (i->url, timeout, callback));
        is->copy_to (os);

        ++progress;

      } catch (tl::BreakException &ex) {
        tl::info << tr ("Download was cancelled") << tl::endl << ex.msg ();
        has_errors = true;
        break;
      } catch (tl::CancelException &ex) {
        tl::info << tr ("Download was cancelled") << tl::endl << ex.msg ();
        has_errors = true;
        break;
      } catch (tl::Exception &ex) {
        tl::error << tr ("Error downloading file from '") << i->url << "':" << tl::endl << ex.msg ();
        has_errors = true;
      }

    }
  }

  return ! has_errors;
}

}

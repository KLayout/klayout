
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


#include "tlWebDAV.h"
#include "tlXMLParser.h"
#include "tlHttpStream.h"
#include "tlStream.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlLog.h"

#include <QUrl>
#include <QDir>

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

static std::string item_name (const QString &path1, const QString &path2)
{
  QStringList sl1 = path1.split (QChar ('/'));
  if (! sl1.empty () && sl1.back ().isEmpty ()) {
    sl1.pop_back ();
  }

  QStringList sl2 = path2.split (QChar ('/'));
  if (! sl2.empty () && sl2.back ().isEmpty ()) {
    sl2.pop_back ();
  }

  int i = 0;
  for ( ; i < sl1.length () && i < sl2.length (); ++i) {
    if (sl1 [i] != sl2 [i]) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid WebDAV response: %1 is not a collection item of %2").arg (path2).arg (path1)));
    }
  }
  if (i == sl2.length ()) {
    return std::string ();
  } else if (i + 1 == sl2.length ()) {
    return tl::to_string (sl2[i]);
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid WebDAV response: %1 is not a collection sub-item of %2").arg (path2).arg (path1)));
  }
}

void
WebDAVObject::read (const std::string &url, int depth)
{
  QUrl base_url = QUrl (tl::to_qstring (url));

  tl::InputHttpStream http (url);
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
    QUrl item_url = base_url.resolved (QUrl (tl::to_qstring (r->href)));

    std::string n = item_name (base_url.path (), item_url.path ());
    std::string item_url_string = tl::to_string (item_url.toString ());

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
void fetch_download_items (const std::string &url, const std::string &target, std::list<DownloadItem> &items, tl::AbsoluteProgress &progress)
{
  ++progress;

  WebDAVObject object;
  object.read (url, 1);

  if (object.is_collection ()) {

    QDir dir (tl::to_qstring (target));
    if (! dir.exists ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Download failed: target directory '%1' does not exists").arg (dir.path ())));
    }

    for (WebDAVObject::iterator i = object.begin (); i != object.end (); ++i) {

      QFileInfo new_item (dir.absoluteFilePath (tl::to_qstring (i->name ())));

      if (i->is_collection ()) {

        if (! new_item.exists ()) {
          if (! dir.mkdir (tl::to_qstring (i->name ()))) {
            throw tl::Exception (tl::to_string (QObject::tr ("Download failed: unable to create subdirectory '%2' in '%1'").arg (dir.path ()).arg (tl::to_qstring (i->name ()))));
          }
        } else if (! new_item.isDir ()) {
          throw tl::Exception (tl::to_string (QObject::tr ("Download failed: unable to create subdirectory '%2' in '%1' - is already a file").arg (dir.path ()).arg (tl::to_qstring (i->name ()))));
        } else if (! new_item.isWritable ()) {
          throw tl::Exception (tl::to_string (QObject::tr ("Download failed: unable to create subdirectory '%2' in '%1' - no write permissions").arg (dir.path ()).arg (tl::to_qstring (i->name ()))));
        }

        fetch_download_items (i->url (), tl::to_string (new_item.filePath ()), items, progress);

      } else {

        if (new_item.exists () && ! new_item.isWritable ()) {
          throw tl::Exception (tl::to_string (QObject::tr ("Download failed: file is '%2' in '%1' - already exists, but no write permissions").arg (dir.path ()).arg (tl::to_qstring (i->name ()))));
        }

        items.push_back (DownloadItem (i->url (), tl::to_string (dir.absoluteFilePath (tl::to_qstring (i->name ())))));

      }
    }

  } else {
    items.push_back (DownloadItem (url, target));
  }
}

bool
WebDAVObject::download (const std::string &url, const std::string &target)
{
  std::list<DownloadItem> items;

  try {

    tl::info << QObject::tr ("Fetching file structure from ") << url;
    tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Fetching directory structure from %1").arg (tl::to_qstring (url))));
    fetch_download_items (url, target, items, progress);

  } catch (tl::Exception &ex) {
    tl::error << QObject::tr ("Error downloading file structure from '") << url << "':" << tl::endl << ex.msg ();
    return false;
  }

  bool has_errors = false;

  {
    tl::info << tl::to_string (QObject::tr ("Downloading %1 file(s) now ..").arg (items.size ()));

    tl::RelativeProgress progress (tl::to_string (QObject::tr ("Downloading file(s) from %1").arg (tl::to_qstring (url))), items.size (), 1);

    for (std::list<DownloadItem>::const_iterator i = items.begin (); i != items.end (); ++i) {

      tl::info << QObject::tr ("Downloading '%1' to '%2' ..").arg (tl::to_qstring  (i->url)).arg (tl::to_qstring (i->path));

      try {

        tl::InputHttpStream http (i->url);

        QFile file (tl::to_qstring (i->path));
        if (! file.open (QIODevice::WriteOnly)) {
          has_errors = true;
          tl::error << QObject::tr ("Unable to open file '%1' for writing").arg (tl::to_qstring (i->path));
        }

        const size_t chunk = 65536;
        char b[chunk];
        size_t read;
        while ((read = http.read (b, sizeof (b))) > 0) {
          if (! file.write (b, read)) {
            tl::error << QObject::tr ("Unable to write %2 bytes file '%1'").arg (tl::to_qstring (i->path)).arg (int (read));
            has_errors = true;
            break;
          }
        }

        file.close ();

      } catch (tl::Exception &ex) {
        tl::error << QObject::tr ("Error downloading file from '") << i->url << "':" << tl::endl << ex.msg ();
        has_errors = true;
      }

      ++progress;

    }
  }

  return ! has_errors;
}

}

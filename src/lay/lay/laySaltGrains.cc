
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

#include "laySaltGrains.h"
#include "tlString.h"
#include "tlFileUtils.h"
#include "tlLog.h"

#include <QDir>
#include <QFileInfo>
#include <QResource>
#include <QUrl>

namespace lay
{

SaltGrains::SaltGrains ()
{
  //  .. nothing yet ..
}

bool
SaltGrains::operator== (const SaltGrains &other) const
{
  return m_name == other.m_name &&
         m_path == other.m_path &&
         m_title == other.m_title &&
         m_collections == other.m_collections &&
         m_grains == other.m_grains;
}

void
SaltGrains::set_name (const std::string &n)
{
  m_name = n;
}

void
SaltGrains::set_title (const std::string &t)
{
  m_title = t;
}

void
SaltGrains::set_path (const std::string &p)
{
  m_path = p;
}

void
SaltGrains::add_collection (const SaltGrains &collection)
{
  m_collections.push_back (collection);
}

bool
SaltGrains::remove_collection (collection_iterator iter, bool with_files)
{
  //  NOTE: this is kind of inefficient, but in order to maintain the const iterator semantics this approach is required
  for (collections_type::iterator i = m_collections.begin (); i != m_collections.end (); ++i) {
    if (i == iter) {
      if (with_files && !tl::rm_dir_recursive (tl::to_qstring (i->path ()))) {
        return false;
      }
      m_collections.erase (i);
      return true;
    }
  }

  return false;
}

void
SaltGrains::add_grain (const SaltGrain &grain)
{
  m_grains.push_back (grain);
}

bool
SaltGrains::remove_grain (grain_iterator iter, bool with_files)
{
  //  NOTE: this is kind of inefficient, but in order to maintain the const iterator semantics this approach is required
  for (grains_type::iterator i = m_grains.begin (); i != m_grains.end (); ++i) {
    if (i == iter) {
      if (with_files && !tl::rm_dir_recursive (tl::to_qstring (i->path ()))) {
        return false;
      }
      m_grains.erase (i);
      return true;
    }
  }

  return false;
}

bool
SaltGrains::is_empty () const
{
  if (! m_grains.empty ()) {
    return false;
  }
  for (collections_type::const_iterator i = m_collections.begin (); i != m_collections.end (); ++i) {
    if (!i->is_empty ()) {
      return false;
    }
  }
  return true;
}

bool
SaltGrains::is_readonly () const
{
  return QFileInfo (tl::to_qstring (path ())).isWritable ();
}

namespace
{

/**
 *  @brief A helper class required because directory traversal is not supported by QResource directly
 */
class OpenResource
  : public QResource
{
public:
  using QResource::isDir;
  using QResource::isFile;
  using QResource::children;

  OpenResource (const QString &path)
    : QResource (path)
  {
    //  .. nothing yet ..
  }
};

}

SaltGrains
SaltGrains::from_path (const std::string &path, const std::string &prefix)
{
  tl_assert (! path.empty ());

  SaltGrains grains;
  grains.set_path (path);

  if (path[0] != ':') {

    QDir dir (tl::to_qstring (path));
    QStringList entries = dir.entryList (QDir::NoDotAndDotDot | QDir::Dirs, QDir::Name);
    for (QStringList::const_iterator e = entries.begin (); e != entries.end (); ++e) {

      std::string new_prefix = prefix;
      if (! new_prefix.empty ()) {
        new_prefix += "/";
      }
      new_prefix += tl::to_string (*e);

      std::string epath = tl::to_string (dir.absoluteFilePath (*e));
      if (SaltGrain::is_grain (epath)) {
        try {
          SaltGrain g (SaltGrain::from_path (epath));
          g.set_name (new_prefix);
          grains.add_grain (g);
        } catch (...) {
          //  ignore errors (TODO: what to do here?)
        }
      } else if (QFileInfo (tl::to_qstring (epath)).isDir ()) {
        SaltGrains c = SaltGrains::from_path (epath, new_prefix);
        c.set_name (new_prefix);
        if (! c.is_empty ()) {
          grains.add_collection (c);
        }
      }

    }

  } else {

    OpenResource resource (tl::to_qstring (path));
    if (resource.isDir ()) {

      QStringList templ_dir = resource.children ();
      for (QStringList::const_iterator t = templ_dir.begin (); t != templ_dir.end (); ++t) {

        std::string new_prefix = prefix;
        if (! new_prefix.empty ()) {
          new_prefix += "/";
        }
        new_prefix += tl::to_string (*t);

        std::string epath = path + "/" + tl::to_string (*t);

        if (SaltGrain::is_grain (epath)) {
          try {
            SaltGrain g (SaltGrain::from_path (epath));
            g.set_name (new_prefix);
            grains.add_grain (g);
          } catch (...) {
            //  ignore errors (TODO: what to do here?)
          }
        } else if (OpenResource (tl::to_qstring (epath)).isDir ()) {
          SaltGrains c = SaltGrains::from_path (epath, new_prefix);
          c.set_name (new_prefix);
          if (! c.is_empty ()) {
            grains.add_collection (c);
          }
        }

      }

    }

  }

  return grains;
}

static tl::XMLElementList s_group_struct =
  tl::make_member (&SaltGrains::name, &SaltGrains::set_name, "name") +
  tl::make_member (&SaltGrains::include, "include") +
  tl::make_element (&SaltGrains::begin_collections, &SaltGrains::end_collections, &SaltGrains::add_collection, "group", &s_group_struct) +
  tl::make_element (&SaltGrains::begin_grains, &SaltGrains::end_grains, &SaltGrains::add_grain, "salt-grain", SaltGrain::xml_elements ());

static tl::XMLStruct<lay::SaltGrains> s_xml_struct ("salt-mine", s_group_struct);

void
SaltGrains::load (const std::string &p)
{
  m_url = p;

  tl::XMLFileSource source (p);
  s_xml_struct.parse (source, *this);
}

void
SaltGrains::load (tl::InputStream &p)
{
  m_url.clear ();

  tl::XMLStreamSource source (p);
  s_xml_struct.parse (source, *this);
}

void
SaltGrains::include (const std::string &src_in)
{
  if (! src_in.empty ()) {

    std::string src = src_in;

    //  base relative URL's on the parent URL
    if (!m_url.empty () && src.find ("http:") != 0 && src.find ("https:") != 0 && src.find ("file:") != 0 && !src.empty() && src[0] != '/' && src[0] != '\\') {

      //  replace the last component ("repository.xml") by the given path
      QUrl url (tl::to_qstring (m_url));
      QStringList path_comp = url.path ().split (QString::fromUtf8 ("/"));
      if (!path_comp.isEmpty ()) {
        path_comp.back () = tl::to_qstring (src);
      }
      url.setPath (path_comp.join (QString::fromUtf8 ("/")));

      src = tl::to_string (url.toString ());

    }

    if (tl::verbosity () >= 20) {
      tl::log << "Including package index from " << src;
    }

    lay::SaltGrains g;
    g.load (src);
    m_collections.splice (m_collections.end (), g.m_collections);
    m_grains.splice (m_grains.end (), g.m_grains);

  }
}

void
SaltGrains::save (const std::string &p) const
{
  tl::OutputStream os (p, tl::OutputStream::OM_Plain);
  s_xml_struct.write (os, *this);
}

}

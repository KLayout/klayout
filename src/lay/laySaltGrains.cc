
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

#include <QDir>
#include <QFileInfo>

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

void
SaltGrains::remove_collection (collection_iterator iter)
{
  //  NOTE: this is kind of inefficient, but in order to maintain the const iterator semantics this approach is required
  for (collections_type::iterator i = m_collections.begin (); i != m_collections.end (); ++i) {
    if (i == iter) {
      m_collections.erase (i);
      break;
    }
  }
}

void
SaltGrains::add_grain (const SaltGrain &grain)
{
  m_grains.push_back (grain);
}

void
SaltGrains::remove_grain (grain_iterator iter)
{
  //  NOTE: this is kind of inefficient, but in order to maintain the const iterator semantics this approach is required
  for (grains_type::iterator i = m_grains.begin (); i != m_grains.end (); ++i) {
    if (i == iter) {
      m_grains.erase (i);
      break;
    }
  }
}

bool
SaltGrains::is_empty () const
{
  return m_collections.empty () && m_grains.empty ();
}

SaltGrains
SaltGrains::from_path (const std::string &path, const std::string &prefix)
{
  SaltGrains grains;
  grains.set_path (path);

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

  return grains;
}

}

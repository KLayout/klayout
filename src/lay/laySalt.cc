
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

#include "laySalt.h"
#include "tlString.h"

#include <QFileInfo>

namespace lay
{

Salt::Salt ()
{
  //  .. nothing yet ..
}

Salt::Salt (const Salt &other)
  : QObject ()
{
  operator= (other);
}

Salt &Salt::operator= (const Salt &other)
{
  if (this != &other) {
    m_root = other.m_root;
  }
  return *this;
}

void
Salt::add_location (const std::string &path)
{
  //  do nothing if the collection is already there
  QFileInfo fi (tl::to_qstring (path));
  for (lay::SaltGrains::collection_iterator g = m_root.begin_collections (); g != m_root.end_collections (); ++g) {
    if (QFileInfo (tl::to_qstring (g->path ())) == fi) {
      return;
    }
  }

  lay::SaltGrains gg = lay::SaltGrains::from_path (path);
  m_root.add_collection (gg);
  mp_flat_grains.clear ();
  emit collections_changed ();
}

void
Salt::remove_location (const std::string &path)
{
  QFileInfo fi (tl::to_qstring (path));
  for (lay::SaltGrains::collection_iterator g = m_root.begin_collections (); g != m_root.end_collections (); ++g) {
    if (QFileInfo (tl::to_qstring (g->path ())) == fi) {
      m_root.remove_collection (g, false);
      mp_flat_grains.clear ();
      emit collections_changed ();
      return;
    }
  }
}

void
Salt::refresh ()
{
  lay::SaltGrains new_root;
  for (lay::SaltGrains::collection_iterator g = m_root.begin_collections (); g != m_root.end_collections (); ++g) {
    new_root.add_collection (lay::SaltGrains::from_path (g->path ()));
  }
  if (new_root != m_root) {
    m_root = new_root;
    mp_flat_grains.clear ();
    emit collections_changed ();
  }
}

void
Salt::add_collection_to_flat (SaltGrains &gg)
{
  for (lay::SaltGrains::grain_iterator g = gg.begin_grains (); g != gg.end_grains (); ++g) {
    //  TODO: get rid of the const cast - would require a non-const grain iterator
    mp_flat_grains.push_back (const_cast <SaltGrain *> (g.operator-> ()));
  }
  for (lay::SaltGrains::collection_iterator g = gg.begin_collections (); g != gg.end_collections (); ++g) {
    //  TODO: get rid of the const cast - would require a non-const grain collection iterator
    add_collection_to_flat (const_cast <SaltGrains &> (*g));
  }
}

namespace {

struct NameCompare
{
  bool operator () (lay::SaltGrain *a, lay::SaltGrain *b) const
  {
    //  TODO: UTF-8 support?
    return a->name () < b->name ();
  }
};

}

void
Salt::ensure_flat_present ()
{
  if (mp_flat_grains.empty ()) {
    add_collection_to_flat (m_root);
    std::sort (mp_flat_grains.begin (), mp_flat_grains.end (), NameCompare ());
  }
}

}

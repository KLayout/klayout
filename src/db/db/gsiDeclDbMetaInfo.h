
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

#ifndef _HDR_gsiDeclDbMetaInfo
#define _HDR_gsiDeclDbMetaInfo

#include "dbLayout.h"
#include "tlVariant.h"
#include "tlObject.h"

#include <string>
#include <iterator>

namespace gsi
{

struct MetaInfo
{
  MetaInfo (const std::string &n, const std::string &d, const tl::Variant &v, bool p)
    : name (n), description (d), value (v), persisted (p)
  { }

  MetaInfo (const std::string &n, const db::MetaInfo &mi)
    : name (n), description (mi.description), value (mi.value), persisted (mi.persisted)
  { }

  MetaInfo ()
    : name (), description (), value (), persisted (false)
  { }

  std::string name;
  std::string description;
  tl::Variant value;
  bool persisted;
};

struct MetaInfoIterator
{
  typedef std::forward_iterator_tag iterator_category;
  typedef MetaInfo value_type;
  typedef void difference_type;
  typedef MetaInfo reference;
  typedef void pointer;

  MetaInfoIterator ()
    : mp_layout (), m_b (), m_e ()
  { }

  MetaInfoIterator (const db::Layout *layout, db::Layout::meta_info_iterator b, db::Layout::meta_info_iterator e)
    : mp_layout (const_cast<db::Layout *> (layout)), m_b (b), m_e (e)
  { }

  bool at_end () const
  {
    return !mp_layout || m_b == m_e;
  }

  void operator++ ()
  {
    if (mp_layout) {
      ++m_b;
    }
  }

  MetaInfo operator* () const
  {
    if (mp_layout) {
      return MetaInfo (mp_layout->meta_info_name (m_b->first), m_b->second);
    } else {
      return MetaInfo ();
    }
  }

private:
  tl::weak_ptr<db::Layout> mp_layout;
  db::Layout::meta_info_iterator m_b, m_e;
};

}

#endif

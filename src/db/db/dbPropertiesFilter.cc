
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

#include "dbPropertiesFilter.h"

namespace db
{

PropertiesFilter::PropertiesFilter (const tl::Variant &name, const tl::Variant &value, bool inverse)
  : m_name_id (db::property_names_id (name)), m_value_from (value), m_exact (true), m_glob (false), m_inverse (inverse)
{
  //  .. nothing yet ..
}

PropertiesFilter::PropertiesFilter (const tl::Variant &name, const tl::Variant &from, const tl::Variant &to, bool inverse)
  : m_name_id (db::property_names_id (name)), m_value_from (from), m_value_to (to), m_exact (false), m_glob (false), m_inverse (inverse)
{
  //  .. nothing yet ..
}

PropertiesFilter::PropertiesFilter (const tl::Variant &name, const tl::GlobPattern &pattern, bool inverse)
  : m_name_id (db::property_names_id (name)), m_pattern (pattern), m_exact (true), m_glob (true), m_inverse (inverse)
{
  //  .. nothing yet ..
}

bool
PropertiesFilter::prop_selected (db::properties_id_type prop_id) const
{
  tl::MutexLocker locker (&m_lock);

  auto c = m_cache.find (prop_id);
  if (c != m_cache.end ()) {
    return c->second;
  }

  bool res = prop_selected_impl (prop_id);
  m_cache.insert (std::make_pair (prop_id, res));
  return res;
}

bool
PropertiesFilter::prop_selected_impl (db::properties_id_type prop_id) const
{
  const db::PropertiesSet &ps = db::properties (prop_id);
  if (ps.has_value (m_name_id)) {

    const tl::Variant &value = ps.value (m_name_id);

    if (m_glob) {
      return m_pattern.match (value.to_string ()) != m_inverse;
    } else if (m_exact) {
      return (value == m_value_from) != m_inverse;
    } else {
      return ((m_value_from.is_nil () || ! (value < m_value_from)) && (m_value_to.is_nil () || value < m_value_to)) != m_inverse;
    }

  } else {
    return m_inverse;
  }
}

}

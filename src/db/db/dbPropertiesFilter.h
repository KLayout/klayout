
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef HDR_dbPropertiesFilter
#define HDR_dbPropertiesFilter

#include "dbCommon.h"
#include "dbPropertiesRepository.h"
#include "dbPolygon.h"
#include "tlVariant.h"
#include "tlGlobPattern.h"
#include "tlThreads.h"

#include <map>

namespace db
{

/**
 *  @brief A properties filter
 *
 *  This is a base class for PolygonFilters, EdgeFilters etc. for selecting
 *  Polygons from Regions by property.
 */
class DB_PUBLIC PropertiesFilter
{
public:
  PropertiesFilter (const tl::Variant &name, const tl::Variant &value, bool inverse);
  PropertiesFilter (const tl::Variant &name, const tl::Variant &from, const tl::Variant &to, bool inverse);
  PropertiesFilter (const tl::Variant &name, const tl::GlobPattern &pattern, bool inverse);

  bool prop_selected (db::properties_id_type prop_id) const;

private:
  bool prop_selected_impl (db::properties_id_type prop_id) const;

  mutable std::map<db::properties_id_type, bool> m_cache;
  db::property_names_id_type m_name_id;
  tl::Variant m_value_from, m_value_to;
  tl::GlobPattern m_pattern;
  bool m_exact;
  bool m_glob;
  bool m_inverse;
  mutable tl::Mutex m_lock;
};

template <class PolygonFilter>
class polygon_properties_filter
  : public PolygonFilter, public PropertiesFilter
{
public:
  polygon_properties_filter<PolygonFilter> (const tl::Variant &name, const tl::GlobPattern &pattern, bool inverse)
    : PropertiesFilter (name, pattern, inverse)
  {
    //  .. nothing yet ..
  }

  polygon_properties_filter<PolygonFilter> (const tl::Variant &name, const tl::Variant &value, bool inverse)
    : PropertiesFilter (name, value, inverse)
  {
    //  .. nothing yet ..
  }

  polygon_properties_filter<PolygonFilter> (const tl::Variant &name, const tl::Variant &from, const tl::Variant &to, bool inverse)
    : PropertiesFilter (name, from, to, inverse)
  {
    //  .. nothing yet ..
  }

  bool selected (const db::Polygon &, db::properties_id_type prop_id) const
  {
    return PropertiesFilter::prop_selected (prop_id);
  }

  bool selected (const db::PolygonRef &, db::properties_id_type prop_id) const
  {
    return PropertiesFilter::prop_selected (prop_id);
  }
};

template <class BasicFilter, class ShapeType>
class generic_properties_filter
  : public BasicFilter, public PropertiesFilter
{
public:
  generic_properties_filter<BasicFilter, ShapeType> (const tl::Variant &name, const tl::GlobPattern &pattern, bool inverse)
    : PropertiesFilter (name, pattern, inverse)
  {
    //  .. nothing yet ..
  }

  generic_properties_filter<BasicFilter, ShapeType> (const tl::Variant &name, const tl::Variant &value, bool inverse)
    : PropertiesFilter (name, value, inverse)
  {
    //  .. nothing yet ..
  }

  generic_properties_filter<BasicFilter, ShapeType> (const tl::Variant &name, const tl::Variant &from, const tl::Variant &to, bool inverse)
    : PropertiesFilter (name, from, to, inverse)
  {
    //  .. nothing yet ..
  }

  bool selected (const ShapeType &, db::properties_id_type prop_id) const
  {
    return PropertiesFilter::prop_selected (prop_id);
  }
};

}

#endif

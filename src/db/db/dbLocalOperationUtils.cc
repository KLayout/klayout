
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


#include "dbLocalOperationUtils.h"
#include "dbPolygonTools.h"

namespace db
{

// -----------------------------------------------------------------------------------------------
//  class PolygonRefGenerator

PolygonRefToShapesGenerator::PolygonRefToShapesGenerator (db::Layout *layout, db::Shapes *shapes, db::properties_id_type prop_id)
  : PolygonSink (), mp_layout (layout), mp_shapes (shapes), m_prop_id (prop_id)
{
  //  .. nothing yet ..
}

void PolygonRefToShapesGenerator::put (const db::Polygon &polygon)
{
  tl::MutexLocker locker (&mp_layout->lock ());
  if (m_prop_id != 0) {
    mp_shapes->insert (db::PolygonRefWithProperties (db::PolygonRef (polygon, mp_layout->shape_repository ()), m_prop_id));
  } else {
    mp_shapes->insert (db::PolygonRef (polygon, mp_layout->shape_repository ()));
  }
}

// -----------------------------------------------------------------------------------------------
//  class PolygonSplitter

PolygonSplitter::PolygonSplitter (PolygonSink &sink, double max_area_ratio, size_t max_vertex_count)
  : mp_sink (&sink), m_max_area_ratio (max_area_ratio), m_max_vertex_count (max_vertex_count)
{
  //  .. nothing yet ..
}

void
PolygonSplitter::put (const db::Polygon &poly)
{
  if ((m_max_vertex_count > 0 && poly.vertices () > m_max_vertex_count) || (m_max_area_ratio > 0.0 && poly.area_ratio () > m_max_area_ratio)) {

    std::vector <db::Polygon> split_polygons;
    db::split_polygon (poly, split_polygons);
    for (std::vector <db::Polygon>::const_iterator sp = split_polygons.begin (); sp != split_polygons.end (); ++sp) {
      put (*sp);
    }

  } else {
    mp_sink->put (poly);
  }
}

}

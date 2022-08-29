
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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
//  class EdgeToEdgeSetGenerator

EdgeToEdgeSetGenerator::EdgeToEdgeSetGenerator (std::unordered_set<db::Edge> &edges, int tag, EdgeToEdgeSetGenerator *chained)
  : mp_edges (&edges), m_tag (tag), mp_chained (chained)
{
  //  .. nothing yet ..
}

void EdgeToEdgeSetGenerator::put (const db::Edge &edge)
{
  mp_edges->insert (edge);
  if (mp_chained) {
    mp_chained->put (edge);
  }
}

void EdgeToEdgeSetGenerator::put (const db::Edge &edge, int tag)
{
  if (m_tag == 0 || m_tag == tag) {
    mp_edges->insert (edge);
  }
  if (mp_chained) {
    mp_chained->put (edge, tag);
  }
}

// -----------------------------------------------------------------------------------------------
//  class PolygonRefGenerator

PolygonRefToShapesGenerator::PolygonRefToShapesGenerator (db::Layout *layout, db::Shapes *shapes)
  : PolygonSink (), mp_layout (layout), mp_shapes (shapes)
{
  //  .. nothing yet ..
}

void PolygonRefToShapesGenerator::put (const db::Polygon &polygon)
{
  tl::MutexLocker locker (&mp_layout->lock ());
  mp_shapes->insert (db::PolygonRef (polygon, mp_layout->shape_repository ()));
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


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


#include "dbMutableEdges.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  MutableEdges implementation

MutableEdges::MutableEdges ()
  : AsIfFlatEdges ()
{
  //  .. nothing yet ..
}

MutableEdges::MutableEdges (const MutableEdges &other)
  : AsIfFlatEdges (other)
{
  //  .. nothing yet ..
}

MutableEdges::~MutableEdges ()
{
  //  .. nothing yet ..
}

void
MutableEdges::insert (const db::Box &box)
{
  if (! box.empty () && box.width () > 0 && box.height () > 0) {
    do_insert (db::Edge (box.lower_left (), box.upper_left ()), 0);
    do_insert (db::Edge (box.upper_left (), box.upper_right ()), 0);
    do_insert (db::Edge (box.upper_right (), box.lower_right ()), 0);
    do_insert (db::Edge (box.lower_right (), box.lower_left ()), 0);
  }
}

void
MutableEdges::insert (const db::BoxWithProperties &box)
{
  if (! box.empty () && box.width () > 0 && box.height () > 0) {
    do_insert (db::Edge (box.lower_left (), box.upper_left ()), box.properties_id ());
    do_insert (db::Edge (box.upper_left (), box.upper_right ()), box.properties_id ());
    do_insert (db::Edge (box.upper_right (), box.lower_right ()), box.properties_id ());
    do_insert (db::Edge (box.lower_right (), box.lower_left ()), box.properties_id ());
  }
}

void
MutableEdges::insert (const db::Path &path)
{
  if (path.points () > 0) {
    insert (path.polygon ());
  }
}

void
MutableEdges::insert (const db::PathWithProperties &path)
{
  if (path.points () > 0) {
    insert (db::PolygonWithProperties (path.polygon (), path.properties_id ()));
  }
}

void
MutableEdges::insert (const db::Polygon &polygon)
{
  if (polygon.holes () > 0 || polygon.vertices () > 0) {
    for (db::Polygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
      do_insert (*e, 0);
    }
  }
}

void
MutableEdges::insert (const db::PolygonWithProperties &polygon)
{
  if (polygon.holes () > 0 || polygon.vertices () > 0) {
    for (db::Polygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
      do_insert (*e, polygon.properties_id ());
    }
  }
}

void
MutableEdges::insert (const db::SimplePolygon &polygon)
{
  if (polygon.vertices () > 0) {
    for (db::SimplePolygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
      do_insert (*e, 0);
    }
  }
}

void
MutableEdges::insert (const db::SimplePolygonWithProperties &polygon)
{
  if (polygon.vertices () > 0) {
    for (db::SimplePolygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
      do_insert (*e, polygon.properties_id ());
    }
  }
}

void
MutableEdges::insert (const db::Shape &shape)
{
  db::properties_id_type prop_id = shape.prop_id ();

  if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {

    db::Polygon poly;
    shape.polygon (poly);
    for (auto e = poly.begin_edge (); ! e.at_end (); ++e) {
      do_insert (*e, prop_id);
    }

  } else if (shape.is_edge ()) {

    db::Edge edge;
    shape.edge (edge);
    do_insert (edge, prop_id);

  }
}

}


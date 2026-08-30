
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


#include "dbMutableRegion.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  FlatRegion implementation

MutableRegion::MutableRegion ()
  : AsIfFlatRegion ()
{
  //  .. nothing yet ..
}

MutableRegion::MutableRegion (const MutableRegion &other)
  : AsIfFlatRegion (other)
{
  //  .. nothing yet ..
}

MutableRegion::~MutableRegion ()
{
  //  .. nothing yet ..
}

void
MutableRegion::insert (const db::Box &box, db::properties_id_type prop_id)
{
  if (! box.empty () && box.width () > 0 && box.height () > 0) {
    do_insert (db::Polygon (box), prop_id);
  }
}

void
MutableRegion::insert (const db::Path &path, db::properties_id_type prop_id)
{
  if (path.points () > 0) {
    do_insert (path.polygon (), prop_id);
  }
}

void
MutableRegion::insert (const db::SimplePolygon &polygon, db::properties_id_type prop_id)
{
  if (polygon.vertices () > 0) {
    db::Polygon poly;
    poly.assign_hull (polygon.hull ());
    do_insert (poly, prop_id);
  }
}

void
MutableRegion::insert (const db::Shape &shape)
{
  if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
    db::Polygon poly;
    shape.polygon (poly);
    do_insert (poly, shape.prop_id ());
  }
}

}

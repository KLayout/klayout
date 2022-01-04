
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
MutableRegion::insert (const db::Box &box)
{
  if (! box.empty () && box.width () > 0 && box.height () > 0) {
    do_insert (db::Polygon (box));
  }
}

void
MutableRegion::insert (const db::Path &path)
{
  if (path.points () > 0) {
    do_insert (path.polygon ());
  }
}

void
MutableRegion::insert (const db::SimplePolygon &polygon)
{
  if (polygon.vertices () > 0) {
    db::Polygon poly;
    poly.assign_hull (polygon.begin_hull (), polygon.end_hull ());
    do_insert (poly);
  }
}

void
MutableRegion::insert (const db::Shape &shape)
{
  if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
    db::Polygon poly;
    shape.polygon (poly);
    insert (poly);
  } else if (shape.is_path ()) {
    insert (shape.path ());
  } else if (shape.is_box ()) {
    insert (shape.box ());
  }
}

}


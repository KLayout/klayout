
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


#ifndef HDR_dbMutableRegion
#define HDR_dbMutableRegion

#include "dbCommon.h"

#include "dbAsIfFlatRegion.h"

#include <set>

namespace db {

/**
 *  @brief An interface representing mutable regions
 *
 *  Mutable regions offer insert, transform, flatten and other manipulation functions.
 */
class DB_PUBLIC MutableRegion
  : public AsIfFlatRegion
{
public:
  MutableRegion ();
  MutableRegion (const MutableRegion &other);
  virtual ~MutableRegion ();

  virtual void do_insert (const db::Polygon &polygon, db::properties_id_type prop_id) = 0;

  void transform (const db::UnitTrans &) { }
  void transform (const db::Disp &t) { do_transform (db::Trans (t)); }
  void transform (const db::Trans &t) { do_transform (t); }
  void transform (const db::ICplxTrans &t) { do_transform (t); }
  void transform (const db::IMatrix2d &t) { do_transform (t); }
  void transform (const db::IMatrix3d &t) { do_transform (t); }

  virtual void do_transform (const db::Trans &t) = 0;
  virtual void do_transform (const db::ICplxTrans &t) = 0;
  virtual void do_transform (const db::IMatrix2d &t) = 0;
  virtual void do_transform (const db::IMatrix3d &t) = 0;

  virtual void flatten () = 0;

  virtual void reserve (size_t n) = 0;

  void insert (const db::Polygon &polygon) { do_insert (polygon, 0); }
  void insert (const db::PolygonWithProperties &polygon) { do_insert (polygon, polygon.properties_id ()); }
  void insert (const db::Box &box);
  void insert (const db::BoxWithProperties &box);
  void insert (const db::Path &path);
  void insert (const db::PathWithProperties &path);
  void insert (const db::SimplePolygon &polygon);
  void insert (const db::SimplePolygonWithProperties &polygon);

  void insert (const db::Shape &shape);

  template <class T>
  void insert (const db::Shape &shape, const T &trans)
  {
    if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
      db::Polygon poly;
      shape.polygon (poly);
      poly.transform (trans);
      do_insert (poly, shape.prop_id ());
    }
  }

  template <class Iter>
  void insert (const Iter &b, const Iter &e)
  {
    reserve (count () + (e - b));
    for (Iter i = b; i != e; ++i) {
      insert (*i);
    }
  }

  template <class Iter>
  void insert_seq (const Iter &seq)
  {
    for (Iter i = seq; ! i.at_end (); ++i) {
      insert (*i);
    }
  }
};

}

#endif


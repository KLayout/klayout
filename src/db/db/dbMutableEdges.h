
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


#ifndef HDR_dbMutableEdges
#define HDR_dbMutableEdges

#include "dbCommon.h"

#include "dbAsIfFlatEdges.h"

#include <set>

namespace db {

/**
 *  @brief An interface representing mutable edge collections
 *
 *  Mutable edge collections offer insert, transform, flatten and other manipulation functions.
 */
class DB_PUBLIC MutableEdges
  : public AsIfFlatEdges
{
public:
  MutableEdges ();
  MutableEdges (const MutableEdges &other);
  virtual ~MutableEdges ();

  virtual void do_transform (const db::Trans &t) = 0;
  virtual void do_transform (const db::ICplxTrans &t) = 0;
  virtual void do_transform (const db::IMatrix2d &t) = 0;
  virtual void do_transform (const db::IMatrix3d &t) = 0;

  virtual void flatten () = 0;

  virtual void reserve (size_t n) = 0;

  virtual void do_insert (const db::Edge &edge, db::properties_id_type prop_id) = 0;

  void transform (const db::UnitTrans &) { }
  void transform (const db::Disp &t) { do_transform (db::Trans (t)); }
  void transform (const db::Trans &t) { do_transform (t); }
  void transform (const db::ICplxTrans &t) { do_transform (t); }
  void transform (const db::IMatrix2d &t) { do_transform (t); }
  void transform (const db::IMatrix3d &t) { do_transform (t); }

  void insert (const db::Edge &edge) { do_insert (edge, 0); }
  void insert (const db::EdgeWithProperties &edge) { do_insert (edge, edge.properties_id ()); }
  void insert (const db::Box &box);
  void insert (const db::BoxWithProperties &box);
  void insert (const db::Path &path);
  void insert (const db::PathWithProperties &path);
  void insert (const db::SimplePolygon &polygon);
  void insert (const db::SimplePolygonWithProperties &polygon);
  void insert (const db::Polygon &polygon);
  void insert (const db::PolygonWithProperties &polygon);
  void insert (const db::Shape &shape);

  template <class T>
  void insert (const db::Shape &shape, const T &trans)
  {
    db::properties_id_type prop_id = shape.prop_id ();

    if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {

      db::Polygon poly;
      shape.polygon (poly);
      for (auto e = poly.begin_edge (); ! e.at_end (); ++e) {
        do_insert ((*e).transformed (trans), prop_id);
      }

    } else if (shape.is_edge ()) {

      db::Edge edge;
      shape.edge (edge);
      edge.transform (trans);
      do_insert (edge, prop_id);

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


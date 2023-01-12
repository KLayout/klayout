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

#include "dbNetShape.h"
#include "dbShapes.h"
#include "dbPolygonTools.h"

namespace db {

NetShape::NetShape ()
  : m_ptr (0), m_dx (0), m_dy (0)
{ }

NetShape::NetShape (const db::PolygonRef &pr)
{
  m_ptr = size_t (&pr.obj ()) + 1;
  m_dx = pr.trans ().disp ().x ();
  m_dy = pr.trans ().disp ().y ();
}

NetShape::NetShape (const db::Polygon &poly, db::GenericRepository &repo)
{
  db::PolygonRef pr (poly, repo);
  m_ptr = size_t (&pr.obj ()) + 1;
  m_dx = pr.trans ().disp ().x ();
  m_dy = pr.trans ().disp ().y ();
}

NetShape::NetShape (const db::TextRef &tr)
{
  m_ptr = size_t (&tr.obj ());
  m_dx = tr.trans ().disp ().x ();
  m_dy = tr.trans ().disp ().y ();
}

NetShape::NetShape (const db::Text &text, db::GenericRepository &repo)
{
  db::TextRef tr (text, repo);
  m_ptr = size_t (&tr.obj ());
  m_dx = tr.trans ().disp ().x ();
  m_dy = tr.trans ().disp ().y ();
}

NetShape::shape_type NetShape::type () const
{
  if (m_ptr == 0) {
    return None;
  } else if ((m_ptr & 1) != 0) {
    return Polygon;
  } else {
    return Text;
  }
}

db::PolygonRef NetShape::polygon_ref () const
{
  if ((size_t (m_ptr) & 1) != 0) {
    return db::PolygonRef (reinterpret_cast<db::Polygon *> (m_ptr - 1), db::Disp (db::Vector (m_dx, m_dy)));
  }
  tl_assert (false);
}

db::TextRef NetShape::text_ref () const
{
  if ((size_t (m_ptr) & 1) == 0) {
    return db::TextRef (reinterpret_cast<db::Text *> (m_ptr), db::Disp (db::Vector (m_dx, m_dy)));
  }
  tl_assert (false);
}

void NetShape::transform (const db::Disp &tr)
{
  m_dx += tr.disp ().x ();
  m_dy += tr.disp ().y ();
}

NetShape::box_type NetShape::bbox () const
{
  if ((m_ptr & 1) != 0) {
    return polygon_ref ().box ();
  } else if (m_ptr != 0) {
    return text_ref ().box ();
  } else {
    return box_type ();
  }
}

void NetShape::insert_into (db::Shapes &shapes) const
{
  if ((m_ptr & 1) != 0) {
    shapes.insert (polygon_ref ());
  } else if (m_ptr != 0) {
    shapes.insert (text_ref ());
  }
}

void NetShape::insert_into (db::Shapes &shapes, db::properties_id_type pi) const
{
  if ((m_ptr & 1) != 0) {
    shapes.insert (db::PolygonRefWithProperties (polygon_ref (), pi));
  } else if (m_ptr != 0) {
    shapes.insert (db::TextRefWithProperties (text_ref (), pi));
  }
}

bool NetShape::interacts_with (const db::NetShape &other) const
{
  if (m_ptr == 0 || other.m_ptr == 0 || ! bbox ().touches (other.bbox ())) {
    return false;
  }

  if ((m_ptr & 1) != 0) {

    if ((other.m_ptr & 1) != 0) {

      //  Polygon vs. polygon
      db::PolygonRef pr_other = other.polygon_ref ();
      db::PolygonRef pr = polygon_ref ();
      db::Polygon p = pr_other.obj ().transformed (pr.trans ().inverted () * pr_other.trans (), false);
      return db::interact_pp (pr.obj (), p);

    } else {

      //  NOTE: we assume that the text ref's target is at 0,0
      db::PolygonRef pr = polygon_ref ();
      db::Point pt = db::Point (other.m_dx, other.m_dy) - pr.trans ().disp ();
      return db::inside_poly (pr.obj ().begin_edge (), pt) >= 0;

    }

  } else {

    if ((other.m_ptr & 1) == 0) {

      //  Text vs. text
      return m_dx == other.m_dx && m_dy == other.m_dy;

    } else {

      //  NOTE: we assume that the text ref's target is at 0,0
      db::PolygonRef pr_other = other.polygon_ref ();
      db::Point pt = db::Point (m_dx, m_dy) - pr_other.trans ().disp ();
      return db::inside_poly (pr_other.obj ().begin_edge (), pt) >= 0;

    }

  }
}

template <class Tr>
bool NetShape::interacts_with_transformed (const db::NetShape &other, const Tr &trans) const
{
  if (m_ptr == 0 || other.m_ptr == 0 || ! bbox ().touches (other.bbox ().transformed (trans))) {
    return false;
  }

  if ((m_ptr & 1) != 0) {

    if ((other.m_ptr & 1) != 0) {

      //  Polygon vs. polygon
      db::PolygonRef pr_other = other.polygon_ref ();
      db::PolygonRef pr = polygon_ref ();
      db::Polygon p = pr_other.obj ().transformed (Tr (pr.trans ().inverted ()) * trans * Tr (pr_other.trans ()));
      return db::interact_pp (pr.obj (), p);

    } else {

      //  NOTE: we assume that the text ref's target is at 0,0
      db::PolygonRef pr = polygon_ref ();
      db::Point pt = trans * db::Point (other.m_dx, other.m_dy) - pr.trans ().disp ();
      return db::inside_poly (pr.obj ().begin_edge (), pt) >= 0;

    }

  } else {

    if ((other.m_ptr & 1) == 0) {

      //  Text vs. text
      db::Point pt = trans * db::Point (other.m_dx, other.m_dy);
      return db::Point (m_dx, m_dy) == pt;

    } else {

      //  NOTE: we assume that the text ref's target is at 0,0
      db::PolygonRef pr_other = other.polygon_ref ();
      db::Point pt = trans.inverted () * db::Point (m_dx, m_dy) - pr_other.trans ().disp ();
      return db::inside_poly (pr_other.obj ().begin_edge (), pt) >= 0;

    }

  }
}

//  explicit instantiations
template DB_PUBLIC bool NetShape::interacts_with_transformed<db::ICplxTrans> (const db::NetShape &other, const db::ICplxTrans &trans) const;
template DB_PUBLIC bool NetShape::interacts_with_transformed<db::Trans> (const db::NetShape &other, const db::Trans &trans) const;

}

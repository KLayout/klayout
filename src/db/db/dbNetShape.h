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


#ifndef HDR_dbNetShape
#define HDR_dbNetShape

#include "dbPolygon.h"
#include "dbText.h"
#include "dbShapeRepository.h"
#include "dbBoxConvert.h"
#include "dbShape.h"
#include "dbShapeFlags.h" //  for addressable_object_from_shape
#include "tlSList.h"

namespace db {

class Shapes;

/**
 *  @brief Provides a union of a PolygonRef and a TextRef
 *
 *  This object is used in the netlist extractor and represents either a polygon or a text.
 *  The TextRef shall utilize a StringRef to represent the string.
 */
class DB_PUBLIC NetShape
{
public:
  enum shape_type { None, Text, Polygon };

  typedef db::Point point_type;
  typedef db::Box box_type;
  typedef db::Coord coord_type;
  typedef db::Disp trans_type;

  /**
   *  @brief Default constructor
   */
  NetShape ();

  /**
   *  @brief A NetShape object representing a PolygonRef
   */
  NetShape (const db::PolygonRef &pr);

  /**
   *  @brief A NetShape object representing a Polygon from the given shape repository
   */
  NetShape (const db::Polygon &poly, db::GenericRepository &repo);

  /**
   *  @brief A NetShape object representing a TextRef
   */
  NetShape (const db::TextRef &tr);

  /**
   *  @brief A NetShape object representing a Text from the given shape repository
   */
  NetShape (const db::Text &text, db::GenericRepository &repo);

  /**
   *  @brief Gets a code indicating the type of object stored herein
   */
  shape_type type () const;

  /**
   *  @brief Gets the PolygonRef object
   *  Asserts if the object stored is not a polygon.
   */
  db::PolygonRef polygon_ref () const;

  /**
   *  @brief Gets the TextRef object
   *  Asserts if the object stored is not a text.
   */
  db::TextRef text_ref () const;

  /**
   *  @brief In-place transformation
   */
  void transform (const db::Disp &tr);

  /**
   *  @brief Equality
   */
  bool operator== (const NetShape &net_shape) const
  {
    return m_ptr == net_shape.m_ptr && m_dx == net_shape.m_dx && m_dy == net_shape.m_dy;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const NetShape &net_shape) const
  {
    return ! operator== (net_shape);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const NetShape &net_shape) const
  {
    if (m_ptr != net_shape.m_ptr) {
      return m_ptr < net_shape.m_ptr;
    }
    if (m_dx != net_shape.m_dx) {
      return m_dx < net_shape.m_dx;
    }
    return m_dy < net_shape.m_dy;
  }

  /**
   *  @brief Gets the bounding box of the object
   */
  box_type bbox () const;

  /**
   *  @brief Inserts the object into a Shapes collection
   */
  void insert_into (db::Shapes &shapes) const;

  /**
   *  @brief Inserts the object into a Shapes collection with the given properties ID
   */
  void insert_into (db::Shapes &shapes, db::properties_id_type pi) const;

  /**
   *  @brief Returns true if the object interacts with another NetShape object
   */
  bool interacts_with (const db::NetShape &other) const;

  /**
   *  @brief Returns true if the object interacts with another NetShape object after transforming it
   */
  template <class Tr>
  bool interacts_with_transformed (const db::NetShape &other, const Tr &trans) const;

public:
  size_t m_ptr;
  coord_type m_dx, m_dy;
};

/**
 *  @brief A box converter implementation for NetShape
 */
template <>
struct box_convert<db::NetShape>
{
  typedef db::NetShape::box_type box_type;
  typedef db::NetShape::coord_type coord_type;
  typedef db::complex_bbox_tag complexity;

  box_type operator() (const db::NetShape &net_shape) const
  {
    return net_shape.bbox ();
  }
};

template <>
struct addressable_object_from_shape<db::NetShape>
{
  typedef db::NetShape value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.type () == db::Shape::TextRef) {
      m_heap.push_back (db::NetShape (shape.text_ref ()));
      return &m_heap.back ();
    } else if (shape.type () == db::Shape::PolygonRef) {
      m_heap.push_back (db::NetShape (shape.polygon_ref ()));
      return &m_heap.back ();
    } else {
      tl_assert (false);
    }
  }

private:
  tl::slist<NetShape> m_heap;
};

}

#endif


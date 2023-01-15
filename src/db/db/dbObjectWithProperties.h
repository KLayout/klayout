
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


#ifndef HDR_dbObjectWithProperties
#define HDR_dbObjectWithProperties

#include "tlException.h"
#include "dbTypes.h"
#include "dbPolygon.h"
#include "dbPath.h"
#include "dbEdge.h"
#include "dbEdgePair.h"
#include "dbText.h"
#include "dbBox.h"
#include "dbArray.h"
#include "dbCellInst.h"
#include "dbPropertiesRepository.h"

namespace db
{

class ArrayRepository;

/**
 *  @brief A object with properties template
 *
 *  This template provides some kind of "enhanced shape". A shape can be supplied
 *  with additional properties. For performance reasons, the properties are stored
 *  as an index within a lookup table. Each index refers to a set of properties.
 *  The "PropertiesRepository" class manages the properties available and associates
 *  a property set with an index. 
 *  The shape-with-properties template adds the properties repository index to a 
 *  shape and inherits all of the shape's methods.
 *  This template is not confined to be used with shapes. It can be used for
 *  instances as well.
 */

template <class Obj>
class object_with_properties
  : public Obj
{
public:
  typedef Obj object_type;

  typedef typename Obj::box_type box_type;
  typedef typename Obj::coord_type coord_type;
  typedef typename Obj::point_type point_type;

  typedef db::object_tag< object_with_properties<Obj> > tag;

  /**
   *  @brief The default constructor
   */
  object_with_properties ()
    : Obj (), m_id (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Create myself from a object and an id
   */
  object_with_properties (const Obj &obj, properties_id_type id)
    : Obj (obj), m_id (id)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The copy constructor
   */
  object_with_properties (const object_with_properties<Obj> &d)
    : Obj (d), m_id (d.m_id)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Assignment
   */
  object_with_properties &operator= (const object_with_properties<Obj> &d)
  {
    if (this != &d) {
      Obj::operator= (d);
      m_id = d.m_id;
    }
    return *this;
  }

  /**
   *  @brief Translation from a different repository space in a generic sense
   *
   *  This is required since the translation basically acts as an assignment operator
   *  whose sementics we have to provide here.
   */
  template <class Rep>
  void translate (const object_with_properties<Obj> &d, Rep &rep, db::ArrayRepository &array_rep)
  {
    Obj::translate (d, rep, array_rep);
    m_id = d.m_id;
  }

  /**
   *  @brief Translation with transformation from a different repository space in a generic sense
   *
   *  This is required since the translation basically acts as an assignment operator
   *  whose sementics we have to provide here.
   */
  template <class Rep, class Trans>
  void translate (const object_with_properties<Obj> &d, const Trans &t, Rep &rep, db::ArrayRepository &array_rep)
  {
    Obj::translate (d, t, rep, array_rep);
    m_id = d.m_id;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const object_with_properties<Obj> &d) const
  {
    return Obj::operator== (d) && m_id == d.m_id;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const object_with_properties<Obj> &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief Comparison
   */
  bool operator< (const object_with_properties<Obj> &d) const
  {
    if (! Obj::operator== (d)) {
      return Obj::operator< (d);
    } 
    return m_id < d.m_id;
  }

  /**
   *  @brief Properties Id read accessor
   */
  properties_id_type properties_id () const
  {
    return m_id;
  }

  /**
   *  @brief Properties Id write accessor
   */
  void properties_id (properties_id_type id) 
  {
    m_id = id;
  }

  /**
   *  @brief Returns the transformed object
   */
  template <class Trans>
  object_with_properties<Obj> transformed (const Trans &tr) const
  {
    return object_with_properties<Obj> (Obj::transformed (tr), m_id);
  }

private:
  properties_id_type m_id;
};

typedef object_with_properties<Polygon> PolygonWithProperties;
typedef object_with_properties<DPolygon> DPolygonWithProperties;
typedef object_with_properties<SimplePolygon> SimplePolygonWithProperties;
typedef object_with_properties<DSimplePolygon> DSimplePolygonWithProperties;
typedef object_with_properties<PolygonRef> PolygonRefWithProperties;
typedef object_with_properties<DPolygonRef> DPolygonRefWithProperties;
typedef object_with_properties<SimplePolygonRef> SimplePolygonRefWithProperties;
typedef object_with_properties<DSimplePolygonRef> DSimplePolygonRefWithProperties;

typedef object_with_properties<Path> PathWithProperties;
typedef object_with_properties<DPath> DPathWithProperties;
typedef object_with_properties<PathRef> PathRefWithProperties;
typedef object_with_properties<DPathRef> DPathRefWithProperties;

typedef object_with_properties<Point> PointWithProperties;
typedef object_with_properties<DPoint> DPointWithProperties;

typedef object_with_properties<Edge> EdgeWithProperties;
typedef object_with_properties<DEdge> DEdgeWithProperties;

typedef object_with_properties<EdgePair> EdgePairWithProperties;
typedef object_with_properties<DEdgePair> DEdgePairWithProperties;

typedef object_with_properties<Text> TextWithProperties;
typedef object_with_properties<DText> DTextWithProperties;
typedef object_with_properties<TextRef> TextRefWithProperties;
typedef object_with_properties<DTextRef> DTextRefWithProperties;

typedef object_with_properties<Box> BoxWithProperties;
typedef object_with_properties<DBox> DBoxWithProperties;

typedef object_with_properties<db::array<db::CellInst, db::Trans> > CellInstArrayWithProperties;
typedef object_with_properties<db::array<db::CellInst, db::DTrans> > DCellInstArrayWithProperties;

} // namespace db

#endif


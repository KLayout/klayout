
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


#ifndef HDR_dbShapeFlags
#define HDR_dbShapeFlags

#include "dbCommon.h"
#include "dbShapes.h"
#include "tlSList.h"

namespace db
{

template <class T>
struct shape_flags_traits
{
  static unsigned int generic () { return 0; }
  static unsigned int pure ()    { return 0; }
  static bool with_props ()      { return false; }
};

template <>
struct shape_flags_traits<db::PolygonRef>
  : public shape_flags_traits<void>
{
  static unsigned int generic () { return 1 << db::ShapeIterator::PolygonRef; }
  static unsigned int pure ()    { return 1 << db::ShapeIterator::PolygonRef; }
};

template <>
struct shape_flags_traits<db::TextRef>
  : public shape_flags_traits<void>
{
  static unsigned int generic () { return 1 << db::ShapeIterator::TextRef; }
  static unsigned int pure ()    { return 1 << db::ShapeIterator::TextRef; }
};

template <>
struct shape_flags_traits<db::Box>
  : public shape_flags_traits<void>
{
  static unsigned int generic () { return db::ShapeIterator::Boxes; }
  static unsigned int pure ()    { return 1 << db::ShapeIterator::Box; }
};

template <>
struct shape_flags_traits<db::Path>
  : public shape_flags_traits<void>
{
  static unsigned int generic () { return db::ShapeIterator::Paths; }
  static unsigned int pure ()    { return 1 << db::ShapeIterator::Path; }
};

template <>
struct shape_flags_traits<db::Polygon>
  : public shape_flags_traits<void>
{
  static unsigned int generic () { return db::ShapeIterator::Regions; }
  static unsigned int pure ()    { return 1 << db::ShapeIterator::Polygon; }
};

template <>
struct shape_flags_traits<db::SimplePolygon>
  : public shape_flags_traits<void>
{
  static unsigned int generic () { return db::ShapeIterator::Regions; }
  static unsigned int pure ()    { return 1 << db::ShapeIterator::SimplePolygon; }
};

template <>
struct shape_flags_traits<db::Edge>
  : public shape_flags_traits<void>
{
  static unsigned int generic () { return db::ShapeIterator::Edges; }
  static unsigned int pure ()    { return 1 << db::ShapeIterator::Edge; }
};

template <>
struct shape_flags_traits<db::EdgePair>
  : public shape_flags_traits<void>
{
  static unsigned int generic () { return db::ShapeIterator::EdgePairs; }
  static unsigned int pure ()    { return 1 << db::ShapeIterator::EdgePair; }
};

template <>
struct shape_flags_traits<db::Text>
  : public shape_flags_traits<void>
{
  static unsigned int generic () { return db::ShapeIterator::Texts; }
  static unsigned int pure ()    { return 1 << db::ShapeIterator::Text; }
};

template <class T>
struct shape_flags_traits<db::object_with_properties<T> >
  : public shape_flags_traits<T>
{
  static bool with_props ()      { return true; }
};

template <class T> unsigned int shape_flags ()      { return shape_flags_traits<T>::generic (); }
template <class T> unsigned int shape_flags_pure () { return shape_flags_traits<T>::pure (); }
template <class T> bool shape_flags_with_props ()   { return shape_flags_traits<T>::with_props (); }

/**
 *  @brief Converter helpers for changing a shape to an object of a specific type
 *
 *  These converters a volatile. The pointer delivered is not valid after the next object has
 *  been retrieved.
 */

template <class T>
struct DB_PUBLIC shape_to_object_impl
{
  typedef T value_type;

  void set (const db::Shape &) { }
  const value_type *get (const db::Shape &s) const { return s.basic_ptr (typename T::tag ()); }
};

template <class T>
struct DB_PUBLIC shape_to_object_impl<db::object_with_properties<T> >
{
  typedef db::object_with_properties<T> value_type;

  void set (const db::Shape &s)
  {
    if (! s.has_prop_id ()) {
      m_shape = value_type (*s.basic_ptr (typename T::tag ()), 0);
    }
  }

  const value_type *get (const db::Shape &s) const
  {
    if (! s.has_prop_id ()) {
      return &m_shape;
    } else {
      return s.basic_ptr (typename value_type::tag ());
    }
  }

private:
  value_type m_shape;
};

template <>
struct DB_PUBLIC shape_to_object_impl<db::Polygon>
{
  typedef db::Polygon value_type;

  void set (const db::Shape &s) { s.polygon (m_shape); }
  const value_type *get (const db::Shape &) const { return &m_shape; }

private:
  value_type m_shape;
};

template <>
struct DB_PUBLIC shape_to_object_impl<db::PolygonWithProperties>
{
  typedef db::PolygonWithProperties value_type;

  void set (const db::Shape &s)
  {
    s.polygon (m_shape);
    m_shape.properties_id (s.prop_id ());
  }

  const value_type *get (const db::Shape &) const
  {
    return &m_shape;
  }

private:
  value_type m_shape;
};

template <>
struct DB_PUBLIC shape_to_object_impl<db::SimplePolygon>
{
  typedef db::SimplePolygon value_type;

  void set (const db::Shape &s) { s.simple_polygon (m_shape); }
  const value_type *get (const db::Shape &) const { return &m_shape; }

private:
  value_type m_shape;
};

template <>
struct DB_PUBLIC shape_to_object_impl<db::SimplePolygonWithProperties>
{
  typedef db::SimplePolygonWithProperties value_type;

  void set (const db::Shape &s)
  {
    s.simple_polygon (m_shape);
    m_shape.properties_id (s.prop_id ());
  }

  const value_type *get (const db::Shape &) const
  {
    return &m_shape;
  }

private:
  value_type m_shape;
};

template <>
struct DB_PUBLIC shape_to_object_impl<db::Path>
{
  typedef db::Path value_type;

  void set (const db::Shape &s) { s.path (m_shape); }
  const value_type *get (const db::Shape &) const { return &m_shape; }

private:
  value_type m_shape;
};

template <>
struct DB_PUBLIC shape_to_object_impl<db::PathWithProperties>
{
  typedef db::PathWithProperties value_type;

  void set (const db::Shape &s)
  {
    s.path (m_shape);
    m_shape.properties_id (s.prop_id ());
  }

  const value_type *get (const db::Shape &) const
  {
    return &m_shape;
  }

private:
  value_type m_shape;
};

template <>
struct DB_PUBLIC shape_to_object_impl<db::Text>
{
  typedef db::Text value_type;

  void set (const db::Shape &s) { s.text (m_shape); }
  const value_type *get (const db::Shape &) const { return &m_shape; }

private:
  value_type m_shape;
};

template <>
struct DB_PUBLIC shape_to_object_impl<db::TextWithProperties>
{
  typedef db::TextWithProperties value_type;

  void set (const db::Shape &s)
  {
    s.text (m_shape);
    m_shape.properties_id (s.prop_id ());
  }

  const value_type *get (const db::Shape &) const
  {
    return &m_shape;
  }

private:
  value_type m_shape;
};

template <>
struct DB_PUBLIC shape_to_object_impl<db::Box>
{
  typedef db::Box value_type;

  void set (const db::Shape &s) { s.box (m_shape); }
  const value_type *get (const db::Shape *) const { return &m_shape; }

private:
  value_type m_shape;
};

template <>
struct DB_PUBLIC shape_to_object_impl<db::BoxWithProperties>
{
  typedef db::BoxWithProperties value_type;

  void set (const db::Shape &s)
  {
    s.box (m_shape);
    m_shape.properties_id (s.prop_id ());
  }

  const value_type *get (const db::Shape &) const
  {
    return &m_shape;
  }

private:
  value_type m_shape;
};

template <class T>
struct DB_PUBLIC shape_to_object
  : public shape_to_object_impl<T>
{
  const typename shape_to_object_impl<T>::value_type &operator() (const db::Shape &s)
  {
    shape_to_object_impl<T>::set (s);
    return *shape_to_object_impl<T>::get (s);
  }
};

/**
 *  @brief Implements an addressable object heap
 *
 *  This object can deliver addressable objects from shapes. It will keep temporary objects
 *  internally if required.
 */

template <class T>
struct addressable_object_from_shape
{
  typedef T value_type;

  const T *operator () (const db::Shape &shape)
  {
    typename T::tag object_tag;
    return shape.basic_ptr (object_tag);
  }
};

template <class T>
struct addressable_object_from_shape<db::object_with_properties<T> >
{
  typedef db::object_with_properties<T> value_type;

  const db::object_with_properties<T> *operator () (const db::Shape &shape)
  {
    if (shape.has_prop_id ()) {
      typename db::object_with_properties<T>::tag object_tag;
      return shape.basic_ptr (object_tag);
    } else {
      typename T::tag object_tag;
      m_heap.push_back (db::object_with_properties<T> (*shape.basic_ptr (object_tag), 0));
      return &m_heap.back ();
    }
  }

private:
  tl::slist<db::object_with_properties<T> > m_heap;
};

template <>
struct addressable_object_from_shape<db::Box>
{
  typedef db::Box value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.type () == db::Shape::Box) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.box (m_heap.front ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::BoxWithProperties>
{
  typedef db::BoxWithProperties value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.has_prop_id () && shape.type () == db::Shape::Box) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.box (m_heap.front ());
      m_heap.front ().properties_id (shape.prop_id ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::Polygon>
{
  typedef db::Polygon value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.type () == db::Shape::Polygon) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.polygon (m_heap.front ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::PolygonWithProperties>
{
  typedef db::PolygonWithProperties value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.has_prop_id () && shape.type () == db::Shape::Polygon) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.polygon (m_heap.front ());
      m_heap.front ().properties_id (shape.prop_id ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::SimplePolygon>
{
  typedef db::SimplePolygon value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.type () == db::Shape::SimplePolygon) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.simple_polygon (m_heap.front ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::SimplePolygonWithProperties>
{
  typedef db::SimplePolygonWithProperties value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.has_prop_id () && shape.type () == db::Shape::SimplePolygon) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.simple_polygon (m_heap.front ());
      m_heap.front ().properties_id (shape.prop_id ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::Path>
{
  typedef db::Path value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.type () == db::Shape::Path) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.path (m_heap.front ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::PathWithProperties>
{
  typedef db::PathWithProperties value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.has_prop_id () && shape.type () == db::Shape::Path) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.path (m_heap.front ());
      m_heap.front ().properties_id (shape.prop_id ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::Edge>
{
  typedef db::Edge value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.type () == db::Shape::Edge) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.edge (m_heap.front ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::EdgeWithProperties>
{
  typedef db::EdgeWithProperties value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.has_prop_id () && shape.type () == db::Shape::Edge) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.edge (m_heap.front ());
      m_heap.front ().properties_id (shape.prop_id ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::EdgePair>
{
  typedef db::EdgePair value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.type () == db::Shape::EdgePair) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.edge_pair (m_heap.front ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::EdgePairWithProperties>
{
  typedef db::EdgePairWithProperties value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.has_prop_id () && shape.type () == db::Shape::EdgePair) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.edge_pair (m_heap.front ());
      m_heap.front ().properties_id (shape.prop_id ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::Text>
{
  typedef db::Text value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.type () == db::Shape::Text) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.text (m_heap.front ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

template <>
struct addressable_object_from_shape<db::TextWithProperties>
{
  typedef db::TextWithProperties value_type;

  const value_type *operator () (const db::Shape &shape)
  {
    if (shape.has_prop_id () && shape.type () == db::Shape::Text) {
      return shape.basic_ptr (value_type::tag ());
    } else {
      m_heap.push_front (value_type ());
      shape.text (m_heap.front ());
      m_heap.front ().properties_id (shape.prop_id ());
      return &m_heap.front ();
    }
  }

private:
  tl::slist<value_type> m_heap;
};

}

#endif

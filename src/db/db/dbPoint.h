
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



#ifndef HDR_dbPoint
#define HDR_dbPoint

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbObjectTag.h"
#include "tlString.h"
#include "tlTypeTraits.h"
#include "tlVector.h"

#include <string>

namespace db {

template <class C> class vector;
template <class C, class R = C> struct box;
template <class C> class generic_repository;
class ArrayRepository;

/**
 *  @brief A point class
 */

template <class C>
class DB_PUBLIC_TEMPLATE point
{
public:
  typedef C coord_type;
  typedef db::coord_traits<C> coord_traits;
  typedef db::vector<C> vector_type;
  typedef typename coord_traits::distance_type distance_type; 
  typedef typename coord_traits::area_type area_type; 
  typedef db::object_tag< point<C> > tag;
  typedef db::box<C> box_type;
  typedef db::point<C> point_type;

  /** 
   *  @brief Default constructor
   *
   *  Creates a point at 0,0
   */
  point () : m_x (0), m_y (0) { }

  /**
   *  @brief Standard constructor
   *
   *  @param x The x coordinate
   *  @param y The y coordinate
   */
  point (C x, C y) : m_x (x), m_y (y) { }

  /**
   *  @brief Standard constructor from a different type
   *
   *  @param x The x coordinate
   *  @param y The y coordinate
   */
  template <class D>
  point (D x, D y) : m_x (coord_traits::rounded (x)), m_y (coord_traits::rounded (y)) { }

  /**
   *  @brief The copy constructor 
   *
   *  @param d The source from which to copy
   */
  point (const point<C> &d) : m_x (d.x ()), m_y (d.y ()) { }

  /**
   *  @brief Assignment
   *
   *  @param d The source from which to take the data
   */
  point &operator= (const point<C> &d) 
  {
    m_x = d.x ();
    m_y = d.y ();
    return *this;
  }

  /**
   *  @brief The copy constructor that also converts
   *
   *  The copy constructor allows one to convert between different
   *  coordinate types, if possible.
   *
   *  @param d The source from which to copy
   */
  template <class D>
  explicit point (const point<D> &d) : m_x (coord_traits::rounded (d.x ())), m_y (coord_traits::rounded (d.y ())) { }

  /**
   *  @brief Assignment which also converts
   *
   *  This assignment operator will convert the coordinate types if possible
   *
   *  @param d The source from which to take the data
   */
  template <class D>
  point &operator= (const point<C> &d) 
  {
    m_x = coord_traits::rounded (d.x ());
    m_y = coord_traits::rounded (d.y ());
    return *this;
  }

  /**
   *  @brief Add to operation
   */
  point<C> &operator+= (const vector<C> &v);

  /**
   *  @brief method version of operator+ (mainly for automation purposes)
   */
  point<C> add (const vector<C> &v) const;

  /**
   *  @brief Subtract from operation
   */
  point<C> &operator-= (const vector<C> &v);
  
  /**
   *  @brief method version of operator- (mainly for automation purposes)
   */
  point<C> subtract (const vector<C> &v) const;

  /**
   *  @brief method version of operator- (mainly for automation purposes)
   */
  vector<C> subtract (const point<C> &p) const;

  /**
   *  @brief "less" comparison operator
   *
   *  This operator is provided to establish a sorting
   *  order
   */
  bool operator< (const point<C> &p) const;

  /**
   *  @brief Equality test operator
   */
  bool operator== (const point<C> &p) const;

  /**
   *  @brief Inequality test operator
   */
  bool operator!= (const point<C> &p) const;

  /**
   *  @brief Const transform
   *
   *  Transforms the point with the given transformation
   *  without modifying the point.
   *
   *  @param t The transformation to apply
   *  @return The transformed point
   */
  template <class Tr>
  point<typename Tr::target_coord_type> transformed (const Tr &t) const;

  /**
   *  @brief In-place transformation
   *
   *  Transforms the point with the given transformation
   *  and writes the result back to the point.
   *
   *  @param t The transformation to apply
   *  @return The transformed point
   */
  template <class Tr>
  point &transform (const Tr &t);

  /**
   *  @brief Accessor to the x coordinate
   */
  C x () const;

  /**
   *  @brief Accessor to the y coordinate
   */
  C y () const;

  /**
   *  @brief Write accessor to the x coordinate
   */
  void set_x (C _x);

  /**
   *  @brief Write accessor to the y coordinate
   */
  void set_y (C _y);

  /**
   *  @brief Scaling self by some factor
   *
   *  Scaling involves rounding which in our case is simply handled
   *  with the coord_traits scheme.
   */
  point<C> &operator*= (double s);

  /**
   *  @brief Scaling self by some integer factor
   */
  point<C> &operator*= (long s);

  /**
   *  @brief Division by some divisor.
   *
   *  Scaling involves rounding which in our case is simply handled
   *  with the coord_traits scheme.
   */

  point<C> &operator/= (double s);

  /**
   *  @brief Dividing self by some integer divisor
   */
  point<C> &operator/= (long s);

  /**
   *  @brief The euclidian distance to another point
   *
   *  @param d The other to compute the distance to.
   */
  distance_type distance (const point<C> &p) const;

  /**
   *  @brief The euclidian distance of the point to (0,0)
   */
  distance_type distance () const;

  /**
   *  @brief The euclidian distance to another point as double value
   *
   *  @param d The other to compute the distance to.
   */
  double double_distance (const point<C> &p) const;

  /**
   *  @brief The euclidian distance of the point to (0,0) as double value
   */
  double double_distance () const;

  /**
   *  @brief The square euclidian distance to another point
   *
   *  @param d The other to compute the distance to.
   */
  area_type sq_distance (const point<C> &p) const;

  /**
   *  @brief The square euclidian distance to point (0,0)
   *
   *  @param d The other to compute the distance to.
   */
  area_type sq_distance () const;

  /**
   *  @brief The square of the euclidian distance to another point as double value
   *
   *  @param d The other to compute the distance to.
   */
  double sq_double_distance (const point<C> &p) const;

  /**
   *  @brief The square of the euclidian distance of the point to (0,0) as double value
   */
  double sq_double_distance () const;

  /**
   *  @brief String conversion
   *
   *  If dbu is set, it determines the factor by which the coordinates are multiplied to render
   *  micron units. In addition, a micron format is chosen for output of these coordinates.
   */
  std::string
  to_string (double dbu = 0.0) const
  {
    if (dbu == 1.0) {
      return tl::db_to_string (m_x) + "," + tl::db_to_string (m_y);
    } else if (dbu > 0.0) {
      return tl::micron_to_string (dbu * m_x) + "," + tl::micron_to_string (dbu * m_y);
    } else {
      return tl::to_string (m_x) + "," + tl::to_string (m_y);
    }
  }

  /**
   *  @brief Fuzzy comparison of points
   */
  bool equal (const point<C> &p) const;

  /**
   *  @brief Fuzzy comparison of points for inequality
   */
  bool not_equal (const point<C> &p) const
  {
    return ! equal (p);
  }

  /**
   *  @brief Fuzzy "less" comparison of points
   */
  bool less (const point<C> &p) const;

  /**
   *  @brief The (dummy) translation operator
   */
  void translate (const point<C> &d, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
  }

  /**
   *  @brief The (dummy) translation operator
   */
  template <class T>
  void translate (const point<C> &d, const T &t, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
    transform (t);
  }

private:
  C m_x, m_y;
};

template <class C>
inline point<C> &
point<C>::operator+= (const vector<C> &v)
{
  m_x += v.x ();
  m_y += v.y ();
  return *this;
}

template <class C>
inline point<C> 
point<C>::add (const vector<C> &v) const
{
  point<C> r (*this);
  r += v;
  return r;
}

template <class C>
inline point<C> &
point<C>::operator-= (const vector<C> &v)
{
  m_x -= v.x ();
  m_y -= v.y ();
  return *this;
}

template <class C>
inline point<C> 
point<C>::subtract (const vector<C> &v) const
{
  return *this - v;
}

template <class C>
inline vector<C>
point<C>::subtract (const point<C> &p) const
{
  return *this - p;
}

template <class C>
inline bool 
point<C>::operator< (const point<C> &p) const
{
  return m_y < p.m_y || (m_y == p.m_y && m_x < p.m_x);
}

template <class C>
inline bool 
point<C>::less (const point<C> &p) const
{
  if (! coord_traits::equal (y (), p.y ())) {
    return y () < p.y ();
  }
  if (! coord_traits::equal (x (), p.x ())) {
    return x () < p.x ();
  }
  return false;
}

template <class C>
inline bool 
point<C>::operator== (const point<C> &p) const
{
  return m_x == p.m_x && m_y == p.m_y;
}

template <class C>
inline bool 
point<C>::equal (const point<C> &p) const
{
  return coord_traits::equal (x (), p.x ()) && coord_traits::equal (y (), p.y ());
}

template <class C>
inline bool 
point<C>::operator!= (const point<C> &p) const
{
  return !operator== (p);
}

template <class C> template <class Tr>
inline point<typename Tr::target_coord_type> 
point<C>::transformed (const Tr &t) const
{
  return t (*this);
}

template <class C> template <class Tr>
inline point<C> &
point<C>::transform (const Tr &t)
{
  *this = t (*this);
  return *this;
}

template <class C>
inline C 
point<C>::x () const
{
  return m_x;
}

template <class C>
inline C 
point<C>::y () const
{
  return m_y;
}

template <class C>
inline void 
point<C>::set_x (C _x) 
{
  m_x = _x;
}

template <class C>
inline void 
point<C>::set_y (C _y) 
{
  m_y = _y;
}

template <class C>
inline point<double> 
operator* (const db::point<C> &p, double s) 
{
  return point<double> (p.x () * s, p.y () * s);
}

template <class C>
inline point<C> 
operator* (const db::point<C> &p, long s) 
{
  return point<C> (p.x () * s, p.y () * s);
}

template <class C>
inline point<C> 
operator* (const db::point<C> &p, unsigned long s) 
{
  return point<C> (p.x () * s, p.y () * s);
}

template <class C>
inline point<C> 
operator* (const db::point<C> &p, int s) 
{
  return point<C> (p.x () * s, p.y () * s);
}

template <class C>
inline point<C> 
operator* (const db::point<C> &p, unsigned int s) 
{
  return point<C> (p.x () * s, p.y () * s);
}

template <class C, typename Number>
inline point<C>
operator/ (const db::point<C> &p, Number s)
{
  double mult = 1.0 / static_cast<double>(s);
  return point<C> (p.x () * mult, p.y () * mult);
}

template <class C>
inline point<C> &
point<C>::operator/= (double s)
{
  double mult = 1.0 / static_cast<double>(s);
  *this *= mult;
  return *this;
}

template <class C>
inline point<C> &
point<C>::operator/= (long s)
{
  double mult = 1.0 / static_cast<double>(s);
  *this *= mult;
  return *this;
}

template <class C>
inline point<C> &
point<C>::operator*= (double s) 
{
  m_x = coord_traits::rounded (m_x * s);
  m_y = coord_traits::rounded (m_y * s);
  return *this;
}

template <class C>
inline point<C> &
point<C>::operator*= (long s) 
{
  m_x = coord_traits::rounded (m_x * s);
  m_y = coord_traits::rounded (m_y * s);
  return *this;
}

template <class C>
inline typename point<C>::distance_type 
point<C>::distance (const point<C> &p) const
{
  double ddx (p.x ());
  double ddy (p.y ());
  ddx -= double (x ());
  ddy -= double (y ());
  return coord_traits::rounded_distance (sqrt (ddx * ddx + ddy * ddy));
}

template <class C>
inline typename point<C>::distance_type 
point<C>::distance () const
{
  double ddx (x ());
  double ddy (y ());
  return coord_traits::rounded_distance (sqrt (ddx * ddx + ddy * ddy));
}

template <class C>
inline double 
point<C>::double_distance (const point<C> &p) const
{
  double ddx (p.x ());
  double ddy (p.y ());
  ddx -= double (x ());
  ddy -= double (y ());
  return sqrt (ddx * ddx + ddy * ddy);
}

template <class C>
inline double 
point<C>::double_distance () const
{
  double ddx (x ());
  double ddy (y ());
  return sqrt (ddx * ddx + ddy * ddy);
}

template <class C>
inline typename point<C>::area_type 
point<C>::sq_distance (const point<C> &p) const
{
  return coord_traits::sq_length (p.x (), p.y (), x (), y ());
}

template <class C>
inline typename point<C>::area_type 
point<C>::sq_distance () const
{
  return coord_traits::sq_length (0, 0, x (), y ());
}

template <class C>
inline double 
point<C>::sq_double_distance (const point<C> &p) const
{
  double ddx (p.x ());
  double ddy (p.y ());
  ddx -= double (x ());
  ddy -= double (y ());
  return ddx * ddx + ddy * ddy;
}

template <class C>
inline double 
point <C>::sq_double_distance () const
{
  double ddx (x ());
  double ddy (y ());
  return ddx * ddx + ddy * ddy;
}

/**
 *  @brief The binary + operator (addition point and vector)
 *
 *  @param p The first point
 *  @param v The second point
 *  @return p + v
 */
template <class C>
inline point<C>
operator+ (point<C> p, const vector<C> &v)
{
  p += v;
  return p;
}

/**
 *  @brief The binary - operator (addition of points)
 *
 *  @param p1 The first point
 *  @param p2 The second point
 *  @return p1 - p2
 */
template <class C>
inline point<C>
operator- (const point<C> &p, const vector<C> &v)
{
  return point<C> (p.x () - v.x (), p.y () - v.y ());
}

/**
 *  @brief The binary - operator (addition of points)
 *
 *  @param p1 The first point
 *  @param p2 The second point
 *  @return p1 - p2
 */
template <class C>
inline vector<C>
operator- (const point<C> &p1, const point<C> &p2)
{
  return vector<C> (p1.x () - p2.x (), p1.y () - p2.y ());
}

/**
 *  @brief The unary - operator 
 *
 *  @param p The point 
 *  @return -p = (-p.x, -p.y)
 */
template <class C>
inline point<C> 
operator- (const point<C> &p)
{
  return point<C> (-p.x (), -p.y ());
}

/**
 *  @brief The stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const point<C> &p)
{
  return (os << p.to_string ());
}

/**
 *  @brief The short integer point
 */
typedef point <short> ShortPoint;

/**
 *  @brief The standard point
 */
typedef point <db::Coord> Point;

/**
 *  @brief The standard double coordinate point
 */
typedef point <db::DCoord> DPoint;

/**
 *  @brief A generic conversion operator from double point to any type
 */
template <class D, class C>
struct point_coord_converter
{
  db::point<D> operator() (const db::point<C> &dp) const
  {
    return db::point<D> (dp);
  }
};

/**
 *  A fuzzy "less" operator for point lists
 */
template <class C>
inline bool less (const tl::vector<point<C> > &a, const tl::vector<point<C> > &b)
{
  if (a.size () != b.size ()) {
    return a.size () < b.size ();
  }

  for (typename tl::vector<point<C> >::const_iterator i = a.begin (), j = b.begin (); i != a.end (); ++i, ++j) {
    if (! i->equal (*j)) {
      return i->less (*j);
    }
  }

  return false;
}

/**
 *  A fuzzy "equal" operator for point lists
 */
template <class C>
inline bool equal (const tl::vector<point<C> > &a, const tl::vector<point<C> > &b)
{
  if (a.size () != b.size ()) {
    return false;
  }

  for (typename tl::vector<point<C> >::const_iterator i = a.begin (), j = b.begin (); i != a.end (); ++i, ++j) {
    if (! i->equal (*j)) {
      return false;
    }
  }

  return true;
}

}

/**
 *  @brief Special extractors for the points
 */

namespace tl 
{
  template <> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Point &p);
  template <> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DPoint &p);

  template <> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Point &p);
  template <> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DPoint &p);

} // namespace tl

#endif


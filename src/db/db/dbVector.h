
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



#ifndef HDR_dbVector
#define HDR_dbVector

#include "dbCommon.h"

#include "dbTypes.h"
#include "tlString.h"
#include "tlTypeTraits.h"

#include <string>

namespace db {

template <class C> class point;

/**
 *  @brief A vector class
 *
 *  A vector basically describes the relation of two points. q = p + v where v is a vector.
 *  Hence, a vector does not describe an absolute but a relative position is space.
 */

template <class C>
class DB_PUBLIC_TEMPLATE vector
{
public:
  typedef C coord_type;
  typedef db::coord_traits<C> coord_traits;
  typedef db::point<C> point_type;
  typedef typename coord_traits::distance_type distance_type; 
  typedef typename coord_traits::area_type area_type; 

  /** 
   *  @brief Default constructor
   *
   *  Creates a vector at 0,0
   */
  vector ()
    : m_x (0), m_y (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Standard constructor
   *
   *  @param x The x coordinate
   *  @param y The y coordinate
   */
  vector (C x, C y)
    : m_x (x), m_y (y)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Standard constructor
   *
   *  @param x The x coordinate
   *  @param y The y coordinate
   */
  template <class D>
  vector (D x, D y)
    : m_x (coord_traits::rounded (x)), m_y (coord_traits::rounded (y))
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Standard constructor from a point
   *
   *  @param p The point from which to take the coordinates
   *
   *  HINT: this is a hack. It does not really make sense
   */
  explicit vector (const point_type &p)
    : m_x (p.x ()), m_y (p.y ())
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Standard constructor as a difference between two points
   *
   *  @param p1 The point to take as starting point
   *  @param p2 The point to take as end point
   */
  vector (const point_type &p1, const point_type &p2)
    : m_x (p2.x () - p1.x ()), m_y (p2.y () - p1.y ())
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The copy constructor 
   *
   *  @param d The source from which to copy
   */
  vector (const vector<C> &d)
    : m_x (d.x ()), m_y (d.y ())
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Assignment
   *
   *  @param d The source from which to take the data
   */
  vector &operator= (const vector<C> &d) 
  {
    m_x = d.x ();
    m_y = d.y ();
    return *this;
  }

  /**
   *  @brief The copy constructor that converts also
   *
   *  The copy constructor allows converting between different
   *  coordinate types, if possible.
   *
   *  @param d The source from which to copy
   */
  template <class D>
  explicit vector (const vector<D> &d)
    : m_x (coord_traits::rounded (d.x ())), m_y (coord_traits::rounded (d.y ()))
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Assignment which also converts
   *
   *  This assignment operator will convert the coordinate types if possible
   *
   *  @param d The source from which to take the data
   */
  template <class D>
  vector &operator= (const point<C> &d) 
  {
    m_x = coord_traits::rounded (d.x ());
    m_y = coord_traits::rounded (d.y ());
    return *this;
  }

  /**
   *  @brief Add to operation
   */
  vector<C> &operator+= (const vector<C> &p);

  /**
   *  @brief method version of operator+ (mainly for automation purposes)
   */
  vector<C> add (const vector<C> &p) const;

  /**
   *  @brief Subtract from operation
   */
  vector<C> &operator-= (const vector<C> &p);
  
  /**
   *  @brief method version of operator- (mainly for automation purposes)
   */
  vector<C> subtract (const vector<C> &p) const;

  /**
   *  @brief "less" comparison operator
   *
   *  This operator is provided to establish a sorting
   *  order
   */
  bool operator< (const vector<C> &p) const;

  /**
   *  @brief Equality test operator
   */
  bool operator== (const vector<C> &p) const;

  /**
   *  @brief Inequality test operator
   */
  bool operator!= (const vector<C> &p) const;

  /**
   *  @brief Const transform
   *
   *  Transforms the vector with the given transformation
   *  without modifying the vector.
   *  When a vector is transformed, it is not displaced.
   *
   *  @param t The transformation to apply
   *  @return The transformed vector
   */
  template <class Tr>
  vector<typename Tr::target_coord_type> transformed (const Tr &t) const;

  /**
   *  @brief In-place transformation
   *
   *  Transforms the vector with the given transformation
   *  and writes the result back to the vector.
   *
   *  @param t The transformation to apply
   *  @return The transformed vector
   */
  template <class Tr>
  vector &transform (const Tr &t);

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
   *  @brief Fuzzy comparison of vectors
   */
  bool equal (const vector<C> &p) const;

  /**
   *  @brief Fuzzy comparison of vectors for inequality
   */
  bool not_equal (const vector<C> &p) const
  {
    return !equal (p);
  }

  /**
   *  @brief Fuzzy "less" comparison of vectors
   */
  bool less (const vector<C> &p) const;

  /**
   *  @brief Scaling by some factor
   *
   *  To avoid round effects, the result vector is of double coordinate type.
   */
  vector<double> operator* (double s) const;

  /**
   *  @brief Scaling by some factor
   */
  vector<C> operator* (long s) const;

  /**
   *  @brief Scaling self by some factor
   *
   *  Scaling by a double value in general involves rounding when the coordinate type is integer.
   */
  vector<C> operator*= (double s);

  /**
   *  @brief Scaling self by some integer factor
   */
  vector<C> operator*= (long s);

  /**
   *  @brief Division by some divisor.
   *
   *  Scaling involves rounding which in our case is simply handled
   *  with the coord_traits scheme.
   */

  vector<C> &operator/= (double s);

  /**
   *  @brief Dividing self by some integer divisor
   */
  vector<C> &operator/= (long s);

  /**
   *  @brief The euclidian length 
   */
  distance_type length () const;

  /**
   *  @brief The euclidian length of the vector
   */
  double double_length () const;

  /**
   *  @brief The square euclidian length of the vector
   */
  area_type sq_length () const;

  /**
   *  @brief The square of the euclidian length of the vector
   */
  double sq_double_length () const;

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

private:
  C m_x, m_y;
};

template <class C>
inline vector<C> &
vector<C>::operator+= (const vector<C> &p)
{
  m_x += p.x ();
  m_y += p.y ();
  return *this;
}

template <class C>
inline vector<C> 
vector<C>::add (const vector<C> &p) const
{
  vector<C> r (*this);
  r += p;
  return r;
}

template <class C>
inline vector<C> &
vector<C>::operator-= (const vector<C> &p)
{
  m_x -= p.x ();
  m_y -= p.y ();
  return *this;
}

template <class C>
inline vector<C> 
vector<C>::subtract (const vector<C> &p) const
{
  vector<C> r (*this);
  r -= p;
  return r;
}

template <class C>
inline bool
vector<C>::operator< (const vector<C> &p) const
{
  return m_y < p.m_y || (m_y == p.m_y && m_x < p.m_x);
}

template <class C>
inline bool
vector<C>::less (const vector<C> &p) const
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
vector<C>::operator== (const vector<C> &p) const
{
  return m_x == p.m_x && m_y == p.m_y;
}

template <class C>
inline bool
vector<C>::equal (const vector<C> &p) const
{
  return coord_traits::equal (x (), p.x ()) && coord_traits::equal (y (), p.y ());
}

template <class C>
inline bool
vector<C>::operator!= (const vector<C> &p) const
{
  return !operator== (p);
}

template <class C> template <class Tr>
inline vector<typename Tr::target_coord_type> 
vector<C>::transformed (const Tr &t) const
{
  return t (vector<typename Tr::target_coord_type> (*this));
}

template <class C> template <class Tr>
inline vector<C> &
vector<C>::transform (const Tr &t)
{
  *this = vector<C> (t (*this));
  return *this;
}

template <class C>
inline C 
vector<C>::x () const
{
  return m_x;
}

template <class C>
inline C 
vector<C>::y () const
{
  return m_y;
}

template <class C>
inline void 
vector<C>::set_x (C _x) 
{
  m_x = _x;
}

template <class C>
inline void 
vector<C>::set_y (C _y) 
{
  m_y = _y;
}

template <class C>
inline vector<double>
vector<C>::operator* (double s) const
{
  return vector<double> (m_x * s, m_y * s);
}

template <class C>
inline vector<C>
vector<C>::operator* (long s) const
{
  return vector<C> (m_x * s, m_y * s);
}

template <class C, typename Number>
inline vector<C>
operator/ (const db::vector<C> &p, Number s)
{
  double mult = 1.0 / static_cast<double>(s);
  return vector<C> (p.x () * mult, p.y () * mult);
}

template <class C>
inline vector<C> &
vector<C>::operator/= (double s)
{
  double mult = 1.0 / static_cast<double>(s);
  *this *= mult;
  return *this;
}

template <class C>
inline vector<C> &
vector<C>::operator/= (long s)
{
  double mult = 1.0 / static_cast<double>(s);
  *this *= mult;
  return *this;
}

template <class C>
inline vector<C> 
vector<C>::operator*= (double s) 
{
  m_x = coord_traits::rounded (m_x * s);
  m_y = coord_traits::rounded (m_y * s);
  return *this;
}

template <class C>
inline vector<C>
vector<C>::operator*= (long s)
{
  m_x *= s;
  m_y *= s;
  return *this;
}

template <class C>
inline typename vector<C>::distance_type 
vector<C>::length () const
{
  double ddx (x ());
  double ddy (y ());
  return coord_traits::rounded_distance (sqrt (ddx * ddx + ddy * ddy));
}

template <class C>
inline double 
vector<C>::double_length () const
{
  double ddx (x ());
  double ddy (y ());
  return sqrt (ddx * ddx + ddy * ddy);
}

template <class C>
inline typename vector<C>::area_type 
vector<C>::sq_length () const
{
  return coord_traits::sq_length (0, 0, x (), y ());
}

template <class C>
inline double 
vector<C>::sq_double_length () const
{
  double ddx (x ());
  double ddy (y ());
  return ddx * ddx + ddy * ddy;
}

/**
 *  @brief The binary + operator (addition of vectors)
 *
 *  @param p1 The first vector
 *  @param p2 The second vector
 *  @return p1 + p2
 */
template <class C>
inline vector<C>
operator+ (const vector<C> &p1, const vector<C> &p2)
{
  vector<C> p (p1);
  p += p2;
  return p;
}

/**
 *  @brief The binary - operator (addition of vectors)
 *
 *  @param p1 The first vector
 *  @param p2 The second vector
 *  @return p1 - p2
 */
template <class C>
inline vector<C>
operator- (const vector<C> &p1, const vector<C> &p2)
{
  vector<C> p (p1);
  p -= p2;
  return p;
}

/**
 *  @brief The unary - operator 
 *
 *  @param p The vector 
 *  @return -p = (-p.x, -p.y)
 */
template <class C>
inline vector<C> 
operator- (const vector<C> &p)
{
  return vector<C> (-p.x (), -p.y ());
}

/**
 *  @brief The stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const vector<C> &p)
{
  return (os << p.to_string ());
}

/**
 *  @brief The short integer vector
 */
typedef vector <short> ShortVector;

/**
 *  @brief The standard vector
 */
typedef vector <db::Coord> Vector;

/**
 *  @brief The standard double coordinate vector
 */
typedef vector <db::DCoord> DVector;

/**
 *  @brief Convenience wrappers for coord_traits functions: vector product: 0->p x 0->q
 */
template <class C>
typename db::coord_traits<C>::area_type vprod (const db::vector<C> &p, const db::vector<C> &q)
{
  return db::coord_traits<C>::vprod (p.x (), p.y (), q.x (), q.y (), 0, 0);
}

/**
 *  @brief Convenience wrappers for coord_traits functions: vector product sign: sign(0->p x 0->q)
 */
template <class C>
int vprod_sign (const db::vector<C> &p, const db::vector<C> &q)
{
  return db::coord_traits<C>::vprod_sign (p.x (), p.y (), q.x (), q.y (), 0, 0);
}

/**
 *  @brief Convenience wrappers for coord_traits functions: vector product with sign
 */
template <class C>
std::pair<typename db::coord_traits<C>::area_type, int> vprod_with_sign (const db::vector<C> &p, const db::vector<C> &q)
{
  return db::coord_traits<C>::vprod_with_sign (p.x (), p.y (), q.x (), q.y (), 0, 0);
}

/**
 *  @brief Convenience wrappers for coord_traits functions: scalar product: 0->p x 0->q
 */
template <class C>
typename db::coord_traits<C>::area_type sprod (const db::vector<C> &p, const db::vector<C> &q)
{
  return db::coord_traits<C>::sprod (p.x (), p.y (), q.x (), q.y (), 0, 0);
}

/**
 *  @brief Convenience wrappers for coord_traits functions: scalar product sign: sign(0->p x 0->q)
 */
template <class C>
int sprod_sign (const db::vector<C> &p, const db::vector<C> &q)
{
  return db::coord_traits<C>::sprod_sign (p.x (), p.y (), q.x (), q.y (), 0, 0);
}

/**
 *  @brief Convenience wrappers for coord_traits functions: scalar product with sign
 */
template <class C>
std::pair<typename db::coord_traits<C>::area_type, int> sprod_with_sign (const db::vector<C> &p, const db::vector<C> &q)
{
  return db::coord_traits<C>::sprod_with_sign (p.x (), p.y (), q.x (), q.y (), 0, 0);
}

/**
 *  @brief A generic conversion operator from double vector to any type
 */
template <class C>
struct from_double_vector 
{
  db::vector<C> operator() (const DVector &dp) const
  {
    return db::vector<C> (dp);
  }
};

}

/**
 *  @brief Special extractors for the vectors
 */

namespace tl 
{
  template <> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Vector &p);
  template <> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DVector &p);

  template <> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Vector &p);
  template <> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DVector &p);

} // namespace tl

#endif


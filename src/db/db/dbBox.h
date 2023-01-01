
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


#ifndef HDR_dbBox
#define HDR_dbBox

#include "dbCommon.h"

#include "dbPoint.h"
#include "dbTrans.h"
#include "dbObjectTag.h"
#include "tlTypeTraits.h"

#include <limits>
#include <string>
#include <functional>

namespace db {

template <class Coord> class generic_repository;
class ArrayRepository;

/**
 *  @brief A box class
 *
 *  This object represents a box (a rectangular shape).
 *  Notation is: p1 is the lower left point, p2 the 
 *  upper right one. 
 *  A box can be empty. An empty box represents no area
 *  (not even a point). A box can be a point or a single
 *  line. In this case, the area is zero but the box still
 *  can overlap other boxes. 
 *  The template parameter C is the type to use for coordinate
 *  values. "R" is the type actually used for representing the
 *  coordinates internally (i.e. R=short, C=int for a 16bit 
 *  coordinates box).
 */

template <class C, class R>
struct DB_PUBLIC_TEMPLATE box
{
  typedef C coord_type;
  typedef box<C, R> box_type;
  typedef point<C> point_type;
  typedef vector<C> vector_type;
  typedef typename coord_traits<C>::area_type area_type;
  typedef typename coord_traits<C>::distance_type distance_type;
  typedef typename coord_traits<C>::perimeter_type perimeter_type;
  typedef object_tag< box<C, R> > tag;

  /**
   *  @brief Empty box constructor
   */
  box ()
    : m_p1 (1, 1), m_p2 (-1, -1)
  {
    //  .. nothing else ..
  }

  /**
   *  @brief Standard constructor with four coordinates
   *  
   *  Creates a box from four coordinates (left, bottom,
   *  right, top). The coordinates are sorted, so left and
   *  right can be swapped as well as top and bottom.
   *
   *  @param x1 The first x coordinate 
   *  @param y1 The first y coordinate
   *  @param x2 The second x coordinate
   *  @param y2 The second y coordinate
   */
  box (C x1, C y1, C x2, C y2)
    : m_p1 (x1 < x2 ? x1 : x2, y1 < y2 ? y1 : y2), 
      m_p2 (x2 > x1 ? x2 : x1, y2 > y1 ? y2 : y1)
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The standard constructor taking two point objects
   *
   *  As the four coordinate constructor but accepting two
   *  point objects. The coordinates are sorted so the points
   *  do not necessarily need to be lower/left or upper/right.
   *
   *  @param p1 The first point
   *  @param p2 The second point
   */
  box (const point<C> &p1, const point<C> &p2)
    : m_p1 (p1.x () < p2.x () ? p1.x () : p2.x (), p1.y () < p2.y () ? p1.y () : p2.y ()), 
      m_p2 (p2.x () > p1.x () ? p2.x () : p1.x (), p2.y () > p1.y () ? p2.y () : p1.y ())
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The copy constructor that also does type conversions
   *
   *  The implementation relies on the ability of the point constructor
   *  to convert between types. It assumes that the conversion is 
   *  maintaining the order of the coordinates and the emptyness condition.
   */
  template <class D, class DR>
  explicit box (const box<D, DR> &b)
    : m_p1 (b.p1 ()), m_p2 (b.p2 ())
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The world box (maximum dimensions)
   *
   *  Hint: this box is likely to be somewhat misfunctional. It cannot be transformed well for example.
   */
  static box world () 
  {
    return box (std::numeric_limits<C>::min (), std::numeric_limits<C>::min (), std::numeric_limits<C>::max (), std::numeric_limits<C>::max ());
  }

  /**
   *  @brief The (dummy) translation operator
   */
  void translate (const box<C, R> &d, db::generic_repository<C> &, db::ArrayRepository &);

  /**
   *  @brief The (dummy) translation operator
   */
  template <class T>
  void translate (const box<C, R> &d, const T &t, db::generic_repository<C> &, db::ArrayRepository &);

  /**
   *  @brief The "less" operator to establish a sorting order.
   */
  bool operator< (const box<C, R> &b) const;

  /**
   *  @brief Equality test
   */
  bool operator== (const box<C, R> &b) const;

  /**
   *  @brief Inequality test
   */
  bool operator!= (const box<C, R> &b) const;

  /**
   *  @brief Fuzzy comparison of boxes
   */
  bool equal (const box_type &b) const;

  /**
   *  @brief Fuzzy comparison of boxes
   */
  bool not_equal (const box_type &b) const
  {
    return !equal (b);
  }

  /**
   *  @brief Fuzzy "less" comparison of points
   */
  bool less (const box_type &p) const;

  /**
   *  @brief A method version for scaling operator* (mainly for automation purposes)
   */
  box<C, R> scaled (double s) const;

  /**
   *  @brief Convolve boxes.
   *
   *  The *= operator convolves the box with the one given as 
   *  the argument. The box resulting from "convolution" is the
   *  outer boundary of the union set formed by placing 
   *  the second box at every point of the first. In other words,
   *  the returned box of (p1,p2)*(q1,q2) is (p1+q1,p2+q2).
   * 
   *  @param b The box to convolve with *this.
   *
   *  @return The convolved box.
   */
  box<C, R> &operator*= (const box<C, R> &b);

  /**
   *  @brief A method version for operator* (mainly for automation purposes)
   */
  box<C, R> convolved (const box<C, R> &b) const;

  /**
   *  @brief Joining of boxes.
   *
   *  The += operator joins the box with the one given as 
   *  the argument. Joining constructs a box that encloses
   *  both boxes given. Empty boxes are neutral: they do not
   *  change another box when joining. Overwrites *this
   *  with the result.
   * 
   *  @param b The box to join with *this.
   *
   *  @return The joined box.
   */
  box<C, R> &operator+= (const box<C, R> &b);

  /**
   *  @brief A method version for operator+ (mainly for automation purposes)
   */
  box<C, R> joined (const box<C, R> &b) const;

  /**
   *  @brief Joining of a box with a point.
   *
   *  The += operator joins the box with a point such that
   *  the new box encloses the point and the old box.
   *  Overwrites *this with the result.
   * 
   *  @param p The point to join with *this.
   *
   *  @return The joined box.
   */
  box<C, R> &operator+= (const point<C> &p);

  /**
   *  @brief Intersection of boxes.
   *
   *  The intersection of two boxes is the largest
   *  box common to both boxes. The intersection may be 
   *  empty if both boxes to not touch. If the boxes do
   *  not overlap but touch the result may be a single
   *  line or point with an area of zero. Overwrites *this
   *  with the result.
   *
   *  @param c The box to take the intersection with
   *
   *  @return The intersection box.
   */

  box<C, R> &operator&= (const box<C, R> &b);

  /**
   *  @brief A method version for operator& (mainly for automation purposes)
   */
  box<C, R> intersection (const box<C, R> &b) const;

  /**
   *  @brief Returns the box moved by a certain distance
   *
   *  Moves the box by a given offset and returns the moved
   *  box. Does not modify *this. Does not check for coordinate
   *  overflows.
   *
   *  @param p The offset to move the box.
   *
   *  @return The moved box.
   */
  box<C, R> moved (const vector<C> &p) const;

  /**
   *  @brief Enlarges the box by a certain amount.
   *
   *  Enlarges the box by x and y value specified in the vector
   *  passed. Positive values with grow the box, negative ones
   *  will shrink the box. The result may be an empty box if the
   *  box disappears. The amount specifies the grow or shrink
   *  per edge. The width and height will change by twice the
   *  amount.
   *  Does not modify *this. Does not check for coordinate
   *  overflows.
   *
   *  @param p The grow or shrink amount in x and y direction
   *
   *  @return The enlarged box.
   */
  box<C, R> enlarged (const vector<C> &p) const;

  /**
   *  @brief Transformation of the box
   * 
   *  Transforms the box with a given transformation and
   *  writes the result to *this. If the transformation is non-orthogonal,
   *  the result will still be a box (which is not correct strictly spoken)
   *  which will be the enclosing box of the rotated box.
   *
   *  @param t The transformation to apply.
   *
   *  @return The transformed box.
   */
  template <class Tr>
  box<C, R> &transform (const Tr &t);

  /**
   *  @brief Returns the transformed box.
   *
   *  Transforms the box and returns the result without changing
   *  the box. If the transformation is non-orthogonal,
   *  the result will still be a box (which is not correct strictly spoken)
   *  which will be the enclosing box of the rotated box.
   *
   *  @param t The transformation to apply.
   *
   *  @return The transformed box.
   */
  template <class Tr>
  box<typename Tr::target_coord_type> transformed (const Tr &t) const;

  /**
   *  @brief Moves the box
   *
   *  Like %moved but modifies the box so it becomes the moved
   *  box.
   *
   *  @param p The distance to move
   *
   *  @return The moved box.
   */
  box<C, R> &move (const vector<C> &p);

  /**
   *  @brief Reduce the box
   *
   *  This method is mainly provided for template argument substitution
   *  of path and polygon objects by boxes. It basically moves the box.
   *
   *  @param tr Receives the transformation that must be applied to render the original box
   */
  void reduce (simple_trans<coord_type> &tr)
  {
    vector_type d (p1 () - point_type ());
    move (-d);
    tr = simple_trans<coord_type> (simple_trans<coord_type>::r0, d);
  }

  /**
   *  @brief Reduce the box
   *
   *  This method is mainly provided for template argument substitution
   *  of path and polygon objects by boxes. It basically moves the box.
   *
   *  @param tr Receives the transformation that must be applied to render the original box
   */
  void reduce (disp_trans<coord_type> &tr)
  {
    vector_type d (p1 () - point_type ());
    move (-d);
    tr = disp_trans<coord_type> (d);
  }

  /**
   *  @brief Reduce the box
   *
   *  This method is mainly provided for template argument substitution
   *  of path and polygon objects by boxes. It basically does nothing (like the same methods in path etc.)
   */
  void reduce (unit_trans<coord_type> &)
  {
    //  .. nothing ..
  }

  /**
   *  @brief Enlarges the box
   *
   *  Like %enlarged but modifies the box so it becomes the enlarged
   *  box.
   *
   *  @param p The grow/shrink to apply.
   *
   *  @return The enlarged box.
   */
  box<C, R> &enlarge (const vector<C> &p);

  /**
   *  @brief Accessor to the lower left point.
   *
   *  @return The lower left point
   */
  const point<R> &p1 () const;

  /**
   *  @brief Accessor to the upper right point.
   *
   *  @return The upper right point
   */
  const point<R> &p2 () const;

  /**
   *  @brief Accessor to the lower left point. This is synonym for p1.
   *
   *  @return The lower left point
   */
  const point<R> &lower_left () const;

  /**
   *  @brief Accessor to the upper right point. This a synonym for p2.
   *
   *  @return The upper right point
   */
  const point<R> &upper_right () const;

  /**
   *  @brief Accessor to the upper left point.
   *
   *  @return The upper left point
   */
  point<R> upper_left () const;

  /**
   *  @brief Accessor to the lower right point.
   *
   *  @return The lower right point
   */
  point<R> lower_right () const;

  /**
   *  @brief is_point predicate
   *
   *  A box has this predicate if it consists of exactly one point
   */
  bool is_point () const;

  /**
   *  @brief Write accessor to the lower left point.
   *
   *  The box will still remain a valid box even if the new lower left point
   *  is not the actual lower left point. In this case, the points will be
   *  ordered properly.
   */
  void set_p1 (const point<C> &_p1);

  /**
   *  @brief Accessor to the upper right point.
   *
   *  The box will still remain a valid box even if the new upper right point
   *  is not the actual upper right point. In this case, the points will be
   *  ordered properly.
   */
  void set_p2 (const point<C> &_p2);

  /**
   *  @brief Accessor to the center point
   *
   *  Due to rounding in integer space, the center coordinate may not
   *  be the exact center unless double coordinates as used.
   *  For very large boxes, width and height might overflow and the
   *  result may not be correct.
   *
   *  @return The center
   */
  point<C> center () const;

  /**
   *  @brief The width of the box.
   *
   *  @return The width of the box.
   */
  distance_type width () const;

  /**
   *  @brief The height of the box.
   *
   *  @return The height of the box.
   */
  distance_type height () const;

  /**
   *  @brief The left boundary of the box.
   */
  C left () const;

  /**
   *  @brief The right boundary of the box.
   */
  C right () const;

  /**
   *  @brief The top boundary of the box.
   */
  C top () const;

  /**
   *  @brief The bottom boundary of the box.
   */
  C bottom () const;

  /**
   *  @brief Set the left margin 
   *
   *  If the left margin gets larger that the right one, the value given will become the 
   *  right margin
   */
  void set_left (C l);

  /**
   *  @brief Set the right margin 
   *
   *  If the right margin gets less that the left one, the value given will become the 
   *  left margin
   */
  void set_right (C r);

  /**
   *  @brief Set the bottom margin 
   *
   *  If the bottom margin gets larger that the top one, the value given will become the 
   *  top margin
   */
  void set_bottom (C b);

  /**
   *  @brief Set the top margin 
   *
   *  If the top margin gets less that the bottom one, the value given will become the 
   *  bottom margin
   */
  void set_top (C t);

  /**
   *  @brief Empty test of the box.
   *
   *  @return True if the box is empty (not if the area is zero)
   */
  bool empty () const;

  /**
   *  @brief Contains test.
   *
   *  Tests whether a point is inside the box.
   *  This includes if the point is on the box contour.
   *
   *  @param p The point to test against.
   *
   *  @return true if the point is inside p.
   */
  bool contains (const point<C> &p) const;

  /**
   *  @brief Inside test.
   *
   *  Tests whether the box is inside the given one. Returns false
   *  if either the box of the test box is empty.
   *  "Inside" also includes the case when the edges of the box coincide.
   *
   *  @param b The box to test against.
   *
   *  @return true if the box is inside b.
   */
  bool inside (const box<C, R> &b) const;

  /**
   *  @brief Touching test.
   *
   *  Tests whether the box is touching the given one. Returns false
   *  if either the box of the test box is empty.
   *  Touching also includes the case of overlap.
   *
   *  @param b The box to test against.
   *
   *  @return true if the box is touching b (has as least one common
   *  point with b)
   */
  bool touches (const box<C, R> &b) const;

  /**
   *  @brief Overlap test.
   *
   *  Tests whether the box is overlapping the given one. Returns false
   *  if either the box of the test box is empty.
   *
   *  @param b The box to test against.
   *
   *  @return true if the box is overlapping b (the area of the 
   *  intersection box is non-empty)
   */
  bool overlaps (const box<C, R> &b) const;

  /**
   *  @brief Computation of the perimeter of a box
   *
   *  @return The perimeter of the box. 0 if empty.
   */
  perimeter_type perimeter () const;
  
  /**
   *  @brief Computation of the area of a box
   *
   *  @return The area of the box. 0 if empty.
   */
  area_type area () const;
  
  /**
   *  @brief Computation of the area of a box in double 
   *
   *  @return The area of the box. 0 if empty.
   */
  double double_area () const;
  
  /** 
   *  @brief Conversion to string
   *
   *  If dbu is set, it determines the factor by which the coordinates are multiplied to render
   *  micron units. In addition, a micron format is chosen for output of these coordinates.
   */
  std::string to_string (double dbu = 0.0) const
  {
    if (empty ()) {
      return "()";
    } else {
      return "(" + m_p1.to_string (dbu) + ";" + m_p2.to_string (dbu) + ")";
    }
  }


private:
  point<R> m_p1, m_p2;
};

template <class C, class R>
inline void 
box<C, R>::translate (const box<C, R> &d, db::generic_repository<C> &, db::ArrayRepository &)
{
  *this = d;
}

template <class C, class R> template <class T>
inline void 
box<C, R>::translate (const box<C, R> &d, const T &t, db::generic_repository<C> &, db::ArrayRepository &)
{
  *this = d;
  transform (t);
}

template <class C, class R>
inline bool 
box<C, R>::operator< (const box<C, R> &b) const
{
  return m_p1 < b.m_p1 || (m_p1 == b.m_p1 && m_p2 < b.m_p2);
}

template <class C, class R>
inline bool 
box<C, R>::less (const box<C, R> &b) const
{
  if (! m_p1.equal (b.p1 ())) {
    return m_p1.less (b.p1 ());
  }
  if (! m_p2.equal (b.p2 ())) {
    return m_p2.less (b.p2 ());
  }
  return false;
}

template <class C, class R>
inline bool 
box<C, R>::operator== (const box<C, R> &b) const
{
  if (empty () && b.empty ()) {
    return true;
  } else if (! empty () && ! b.empty ()) {
    return m_p1 == b.m_p1 && m_p2 == b.m_p2;
  } else {
    return false;
  }
}

template <class C, class R>
inline bool 
box<C, R>::operator!= (const box<C, R> &b) const
{
  return !operator== (b);
}

template <class C, class R>
inline bool 
box<C, R>::equal (const box<C, R> &b) const
{
  return m_p1.equal (b.p1 ()) && m_p2.equal (b.p2 ());
}

template <class C, class R>
inline box<C, R> &
box<C, R>::operator*= (const box<C, R> &b)
{
  if (! b.empty () && ! empty ()) {
    m_p1 += vector<R> (b.m_p1);
    m_p2 += vector<R> (b.m_p2);
  } else {
    *this = box<C, R> ();
  }
  return *this;
}

template <class C, class R>
inline box<C, R> 
box<C, R>::scaled (double s) const
{
  return box<C, R> (*this * s);
}

template <class C, class R>
inline box<C, R> 
box<C, R>::convolved (const box<C, R> &b) const
{
  box<C, R> r (*this);
  r *= b;
  return r;
}

template <class C, class R>
inline box<C, R> &
box<C, R>::operator+= (const box<C, R> &b)
{
  if (! b.empty ()) {
    if (empty ()) {
      *this = b;
    } else {
      db::point<C> p1 (m_p1.x () < b.m_p1.x () ? m_p1.x () : b.m_p1.x (),
                       m_p1.y () < b.m_p1.y () ? m_p1.y () : b.m_p1.y ());
      db::point<C> p2 (m_p2.x () > b.m_p2.x () ? m_p2.x () : b.m_p2.x (),
                       m_p2.y () > b.m_p2.y () ? m_p2.y () : b.m_p2.y ());
      m_p1 = p1;
      m_p2 = p2;
    }
  }
  return *this;
}

template <class C, class R>
inline box<C, R> 
box<C, R>::joined (const box<C, R> &b) const
{
  box<C, R> r (*this);
  r += b;
  return r;
}

template <class C, class R>
inline box<C, R> &
box<C, R>::operator+= (const point<C> &p)
{
  if (empty ()) {
    m_p1 = point<R> (p);
    m_p2 = point<R> (p);
  } else {
    db::point<R> p1 (m_p1.x () < p.x () ? m_p1.x () : p.x (),
                     m_p1.y () < p.y () ? m_p1.y () : p.y ());
    db::point<R> p2 (m_p2.x () > p.x () ? m_p2.x () : p.x (),
                     m_p2.y () > p.y () ? m_p2.y () : p.y ());
    m_p1 = p1;
    m_p2 = p2;
  }
  return *this;
}

template <class C, class R>
inline box<C, R> &
box<C, R>::operator&= (const box<C, R> &b)
{
  if (b.empty ()) {
    *this = box<C, R> ();
  } else if (! empty ()) {
    point<R> p1 (m_p1.x () > b.m_p1.x () ? m_p1.x () : b.m_p1.x (),
                 m_p1.y () > b.m_p1.y () ? m_p1.y () : b.m_p1.y ());
    point<R> p2 (m_p2.x () < b.m_p2.x () ? m_p2.x () : b.m_p2.x (),
                 m_p2.y () < b.m_p2.y () ? m_p2.y () : b.m_p2.y ());
    m_p1 = p1;
    m_p2 = p2;
  } 
  return *this;
}

template <class C, class R>
inline box<C, R> 
box<C, R>::intersection (const box<C, R> &b) const
{
  box<C, R> r (*this);
  r &= b;
  return r;
}

template <class C, class R>
inline box<C, R> 
box<C, R>::moved (const vector<C> &p) const
{
  box<C, R> b (*this);
  b.move (p);
  return b;
}

template <class C, class R>
inline box<C, R> 
box<C, R>::enlarged (const vector<C> &p) const
{
  box<C, R> b (*this);
  b.enlarge (p);
  return b;
}

template <class C, class R> template <class Tr>
inline box<C, R> &
box<C, R>::transform (const Tr &t)
{
  if (! empty ()) {
    if (t.is_ortho ()) {
      *this = box<C, R> (point<C> (t * m_p1), point<C> (t * m_p2));
    } else {
      box<C, R> b (point<C> (t * m_p1), point<C> (t * m_p2));
      b += point<C> (t * upper_left ());
      b += point<C> (t * lower_right ());
      *this = b;
    }
  } 
  return *this;
}

template <class C, class R> template <class Tr>
inline box<typename Tr::target_coord_type> 
box<C, R>::transformed (const Tr &t) const
{
  if (! empty ()) {
    if (t.is_ortho ()) {
      return box<typename Tr::target_coord_type> (t * m_p1, t * m_p2);
    } else {
      box<typename Tr::target_coord_type> b (t * m_p1, t * m_p2);
      b += t * upper_left ();
      b += t * lower_right ();
      return b;
    }
  } else {
    return box<typename Tr::target_coord_type> ();
  }
}

template <class C, class R>
inline box<C, R> &
box<C, R>::move (const vector<C> &p)
{
  if (! empty ()) {
    m_p1 += p;
    m_p2 += p;
  }
  return *this;
}

template <class C, class R>
inline box<C, R> &
box<C, R>::enlarge (const vector<C> &p)
{
  if (! empty ()) {
    m_p1 -= p;
    m_p2 += p;
  }
  return *this;
}

template <class C, class R>
inline const point<R> &
box<C, R>::p1 () const
{
  return m_p1;
}

template <class C, class R>
inline const point<R> &
box<C, R>::p2 () const
{
  return m_p2;
}

template <class C, class R>
inline const point<R> &
box<C, R>::lower_left () const
{
  return m_p1;
}

template <class C, class R>
inline const point<R> &
box<C, R>::upper_right () const
{
  return m_p2;
}

template <class C, class R>
inline point<R> 
box<C, R>::upper_left () const
{
  return point<R> (m_p1.x (), m_p2.y ());
}

template <class C, class R>
inline point<R> 
box<C, R>::lower_right () const
{
  return point<R> (m_p2.x (), m_p1.y ());
}

template <class C, class R>
inline bool 
box<C, R>::is_point () const
{
  return m_p1 == m_p2;
}

template <class C, class R>
inline void 
box<C, R>::set_p1 (const point<C> &_p1)
{
  *this = box_type (_p1, p2 ());
}

template <class C, class R>
inline void 
box<C, R>::set_p2 (const point<C> &_p2)
{
  *this = box_type (p1 (), _p2);
}

template <class C, class R>
inline point<C> 
box<C, R>::center () const
{
  return point<C> (m_p1.x () + width () / 2, m_p1.y () + height () / 2);
}

template <class C, class R>
inline typename box<C, R>::distance_type 
box<C, R>::width () const
{
  return m_p2.x () - m_p1.x ();
}

template <class C, class R>
inline typename box<C, R>::distance_type 
box<C, R>::height () const
{
  return m_p2.y () - m_p1.y ();
}

template <class C, class R>
inline C 
box<C, R>::left () const
{
  return m_p1.x ();
}

template <class C, class R>
inline C 
box<C, R>::right () const
{
  return m_p2.x ();
}

template <class C, class R>
inline C 
box<C, R>::top () const
{
  return m_p2.y ();
}

template <class C, class R>
inline C 
box<C, R>::bottom () const
{
  return m_p1.y ();
}

template <class C, class R>
inline void 
box<C, R>::set_left (C l)
{
  if (empty ()) {
    *this = box (l, 0, l, 0);
  } else {
    *this = box (l, bottom (), std::max (right (), l), top ());
  }
}

template <class C, class R>
inline void 
box<C, R>::set_right (C r)
{
  if (empty ()) {
    *this = box (r, 0, r, 0);
  } else {
    *this = box (std::min (left (), r), bottom (), r, top ());
  }
}

template <class C, class R>
inline void 
box<C, R>::set_bottom (C b)
{
  if (empty ()) {
    *this = box (0, b, 0, b);
  } else {
    *this = box (left (), b, right (), std::max (top (), b));
  }
}

template <class C, class R>
inline void 
box<C, R>::set_top (C t)
{
  if (empty ()) {
    *this = box (0, t, 0, t);
  } else {
    *this = box (left (), std::min (bottom (), t), right (), t);
  }
}

template <class C, class R>
inline bool 
box<C, R>::empty () const
{
  return m_p1.x () > m_p2.x () || m_p1.y () > m_p2.y ();
}

template <class C, class R>
inline bool 
box<C, R>::contains (const point<C> &p) const
{
  if (empty ()) {
    return false;
  } else {
    return (m_p2.x () >= p.x () && m_p1.x () <= p.x ()) &&
           (m_p2.y () >= p.y () && m_p1.y () <= p.y ());
  }
}

template <class C, class R>
inline bool 
box<C, R>::inside (const box<C, R> &b) const
{
  if (b.empty () || empty ()) {
    return false;
  } else {
    return (m_p1.x () >= b.m_p1.x () && m_p2.x () <= b.m_p2.x ()) &&
           (m_p1.y () >= b.m_p1.y () && m_p2.y () <= b.m_p2.y ());
  }
}

template <class C, class R>
inline bool 
box<C, R>::touches (const box<C, R> &b) const
{
  if (b.empty () || empty ()) {
    return false;
  } else {
    return (m_p1.x () <= b.m_p2.x () && b.m_p1.x () <= m_p2.x ()) &&
           (m_p1.y () <= b.m_p2.y () && b.m_p1.y () <= m_p2.y ());
  }
}

template <class C, class R>
inline bool 
box<C, R>::overlaps (const box<C, R> &b) const
{
  if (b.empty () || empty ()) {
    return false;
  } else {
    return (m_p1.x () < b.m_p2.x () && b.m_p1.x () < m_p2.x ()) &&
           (m_p1.y () < b.m_p2.y () && b.m_p1.y () < m_p2.y ());
  }
}

template <class C, class R>
inline typename box<C, R>::area_type 
box<C, R>::area () const
{
  if (empty ()) {
    return area_type (0);
  } else {
    return area_type (height ()) * area_type (width ());
  }
}

template <class C, class R>
inline typename box<C, R>::perimeter_type 
box<C, R>::perimeter () const
{
  if (empty ()) {
    return perimeter_type (0);
  } else {
    return 2 * (perimeter_type (height ()) + perimeter_type (width ()));
  }
}

template <class C, class R>
inline double 
box<C, R>::double_area () const
{
  if (empty ()) {
    return 0.0;
  } else {
    return double (height ()) * double (width ());
  }
}

/**
 *  @brief The left side as a unary function
 */
template <class Box>
struct box_left 
#if __cplusplus < 201703L
  : public std::unary_function<Box, typename Box::coord_type>
#endif
{
  typename Box::coord_type operator() (const Box &b) const
  {
    return b.left ();
  }
};

/**
 *  @brief The right side as a unary function
 */
template <class Box>
struct box_right 
#if __cplusplus < 201703L
  : public std::unary_function<Box, typename Box::coord_type>
#endif
{
  typename Box::coord_type operator() (const Box &b) const
  {
    return b.right ();
  }
};

/**
 *  @brief The bottom side as a unary function
 */
template <class Box>
struct box_bottom 
#if __cplusplus < 201703L
  : public std::unary_function<Box, typename Box::coord_type>
#endif
{
  typename Box::coord_type operator() (const Box &b) const
  {
    return b.bottom ();
  }
};

/**
 *  @brief The top side as a unary function
 */
template <class Box>
struct box_top 
#if __cplusplus < 201703L
  : public std::unary_function<Box, typename Box::coord_type>
#endif
{
  typename Box::coord_type operator() (const Box &b) const
  {
    return b.top ();
  }
};

/**
 *  @brief "overlap" binary predicate
 */
template <class Box>
struct boxes_overlap
#if __cplusplus < 201703L
  : public std::binary_function<Box, Box, bool>
#endif
{
  bool operator() (const Box &b1, const Box &b2) const
  {
    return b1.overlaps (b2);
  }
};

/**
 *  @brief "touch" binary predicate
 */
template <class Box>
struct boxes_touch
#if __cplusplus < 201703L
  : public std::binary_function<Box, Box, bool>
#endif
{
  bool operator() (const Box &b1, const Box &b2) const
  {
    return b1.touches (b2);
  }
};

/**
 *  @brief Transformation with the * operator
 *
 *  @param t Transformation to apply (first parameter)
 *  @param b The box to transform (second parameter)
 *
 *  @return The box b transformed by t.
 */
template <class R, class Tr>
inline box<typename Tr::target_coord_type> 
operator* (const Tr &t, const box<typename Tr::coord_type, R> &b)
{
  return b.transformed (t);
}

/**
 *  @brief Intersection mapped on the & operator.
 *
 *  @param b1 The first box.
 *  @param b2 The second box.
 *
 *  @return The intersection of b1 and b2.
 */ 
template <class C>
inline box<C> 
operator& (const box<C> &b1, const box<C> &b2)
{
  box<C> bb (b1);
  bb &= b2;
  return bb;
}

/**
 *  @brief Scaling of a box
 *
 *  @param b The box to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled box
 */ 
template <class C>
inline box<double> 
operator* (const box<C> &b, double s)
{
  if (b.empty ()) {
    return box<double> ();
  } else {
    return box<double> (b.p1 () * s, b.p2 () * s);
  }
}

/**
 *  @brief Scaling of a box with some integer factor
 *
 *  @param b The box to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled box
 */ 
template <class C>
inline box<C> 
operator* (const box<C> &b, long s)
{
  if (b.empty ()) {
    return box<C> ();
  } else {
    return box<C> (b.p1 () * s, b.p2 () * s);
  }
}

/**
 *  @brief Scaling of a box with some integer factor
 *
 *  @param b The box to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled box
 */ 
template <class C>
inline box<C> 
operator* (const box<C> &b, int s)
{
  if (b.empty ()) {
    return box<C> ();
  } else {
    return box<C> (b.p1 () * s, b.p2 () * s);
  }
}

/**
 *  @brief Scaling of a box with some integer factor
 *
 *  @param b The box to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled box
 */ 
template <class C>
inline box<C> 
operator* (const box<C> &b, unsigned int s)
{
  if (b.empty ()) {
    return box<C> ();
  } else {
    return box<C> (b.p1 () * s, b.p2 () * s);
  }
}

/**
 *  @brief Scaling of a box with some integer factor
 *
 *  @param b The box to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled box
 */ 
template <class C>
inline box<C> 
operator* (const box<C> &b, unsigned long s)
{
  if (b.empty ()) {
    return box<C> ();
  } else {
    return box<C> (b.p1 () * s, b.p2 () * s);
  }
}

/**
 *  @brief Box joining mapped on the + operator.
 *
 *  @param b1 The first box.
 *  @param b2 The second box.
 *
 *  @return b1 joined with b2.
 */ 
template <class C>
inline box<C> 
operator+ (const box<C> &b1, const box<C> &b2)
{
  box<C> bb (b1);
  bb += b2;
  return bb;
}

/**
 *  @brief "Folding" of two boxes
 *
 *  @param b1 The first box.
 *  @param b2 The second box.
 *
 *  @return b1 folded with b2 (see db::box::operator*= ()).
 */ 
template <class C>
inline box<C> 
operator* (const box<C> &b1, const box<C> &b2)
{
  box<C> bb (b1);
  bb *= b2;
  return bb;
}

/**
 *  @brief Stream output operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const box<C> &b)
{
  return (os << b.to_string ());
}

/** 
 *  @brief A helper inserter that adds a point to a box
 */

template <class Box>
class box_inserter 
{
public:
  box_inserter (Box &box) 
    : mp_box (&box)
  {
    //  .. nothing yet ..
  }

  box_inserter &operator* () { return *this; }
  box_inserter &operator++ () { return *this; }
  box_inserter &operator++ (int) { return *this; }

  template <class Point>
  box_inserter &operator= (const Point &p)
  {
    *mp_box += p;
    return *this;
  }

private:
  Box *mp_box;
};

/**
 *  @brief The standard short integer coordinate box
 */
typedef box<db::Coord, db::coord_traits<db::Coord>::short_coord_type> ShortBox;

/**
 *  @brief The standard integer coordinate box
 */
typedef box<db::Coord> Box;

/**
 *  @brief The double coordinate box
 */
typedef box<db::DCoord> DBox;

} // namespace db

/**
 *  @brief Special extractors for the boxes
 */

namespace tl 
{
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Box &b);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DBox &b);

  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Box &b);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DBox &b);

} // namespace tl

#endif


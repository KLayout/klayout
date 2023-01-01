
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



#ifndef HDR_dbEdge
#define HDR_dbEdge

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbPoint.h"
#include "dbVector.h"
#include "dbTrans.h"
#include "dbObjectTag.h"
#include "dbBox.h"

#include <string>

namespace db {

/**
 *  @brief A helper function for dividing integers with exact rounding
 *  This function computes (a*b/d) where rounding is exact in the sense of:
 *    a*b/d == N+0.5 => div_exact(a*b/d) = N
 *  b and d needs to be positive.
 *  a can be positive or negative.
 *  The implementation uses the gcd to reduce the ratios. This way we can
 *  represent the numbers with the area type.
 */
db::Coord DB_PUBLIC div_exact (db::Coord a, db::coord_traits<db::Coord>::area_type b, db::coord_traits<db::Coord>::area_type d);

/**
 *  @brief An overload of div_exact for double types
 */
inline db::DCoord div_exact (db::DCoord a, db::coord_traits<db::DCoord>::area_type b, db::coord_traits<db::DCoord>::area_type d)
{
  return db::coord_traits<db::DCoord>::rounded (double (a) * double (b) / double (d));
}

template <class C> class generic_repository;
class ArrayRepository;

template <class C>
class DB_PUBLIC_TEMPLATE edge
{
public:
  typedef C coord_type;
  typedef db::box<C> box_type;
  typedef db::point<C> point_type;
  typedef db::vector<C> vector_type;
  typedef db::coord_traits<C> coord_traits;
  typedef typename coord_traits::distance_type distance_type; 
  typedef typename coord_traits::area_type area_type; 
  typedef db::object_tag< edge<C> > tag;

  /**
   *  @brief The default constructor.
   *  
   *  The default constructor creates a degenerated edge
   *  with two points (0, 0).
   */
  edge ()
    : m_p1 (0, 0), m_p2 (0, 0)
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The standard constructor taking four coordinates.
   *
   *  Creates an edge from (x1,y1) to (x2,y2).
   *
   *  @param x1 The first point's x coordinate.
   *  @param y1 The first point's y coordinate.
   *  @param x2 The second point's x coordinate.
   *  @param y2 The second point's y coordinate.
   */
  template <class D>
  edge (D x1, D y1, D x2, D y2)
    : m_p1 (x1, y1), m_p2 (x2, y2)
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The standard constructor taking two point objects.
   *
   *  Creates an edge from p1 to p2.
   *
   *  @param p1 The first point.
   *  @param p2 The first point.
   */
  template <class D>
  edge (const point<D> &p1, const point<D> &p2)
    : m_p1 (p1), m_p2 (p2)
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The standard constructor taking two point objects.
   *
   *  Creates an edge from p1 to p2.
   *
   *  @param p The first point.
   *  @param v The distance to the end point.
   */
  template <class D>
  edge (const point<D> &p, const vector<D> &v)
    : m_p1 (p), m_p2 (p + v)
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The copy constructor
   */
  template <class D>
  explicit edge (const edge<D> &e)
    : m_p1 (e.p1 ()), m_p2 (e.p2 ())
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The (dummy) translation operator
   */
  void translate (const edge<C> &d, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
  }

  /**
   *  @brief The (dummy) translation operator
   */
  template <class T>
  void translate (const edge<C> &d, const T &t, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
    transform (t);
  }

  /**
   *  @brief A less operator to establish a sorting order.
   */
  bool operator< (const edge<C> &b) const
  {
    return m_p1 < b.m_p1 || (m_p1 == b.m_p1 && m_p2 < b.m_p2);
  }

  /** 
   *  @brief Equality test
   */
  bool operator== (const edge<C> &b) const
  {
    return m_p1 == b.m_p1 && m_p2 == b.m_p2;
  }

  /** 
   *  @brief Inequality test
   */
  bool operator!= (const edge<C> &b) const
  {
    return !operator== (b);
  }

  /**
   *  @brief A fuzzy less operator to establish a sorting order.
   */
  bool less (const edge<C> &b) const
  {
    return m_p1.less (b.m_p1) || (m_p1.equal (b.m_p1) && m_p2.less (b.m_p2));
  }

  /**
   *  @brief Fuzzy equality test
   */
  bool equal (const edge<C> &b) const
  {
    return m_p1.equal (b.m_p1) && m_p2.equal (b.m_p2);
  }

  /**
   *  @brief Fuzzy inequality test
   */
  bool not_equal (const edge<C> &b) const
  {
    return !equal (b);
  }

  /**
   *  @brief A method binding of operator* (mainly for automation purposes)
   */
  edge<C> scaled (double s) const
  {
    return edge<C> (point_type (p1 () * s), point_type (p2 () * s));
  }

  /**
   *  @brief Returns the moved edge
   *
   *  Moves the edge by the given offset and returns the 
   *  moved edge. The edge is not modified.
   *
   *  @param p The distance to move the edge.
   * 
   *  @return The moved edge.
   */
  edge<C> moved (const vector<C> &p) const
  {
    edge<C> b (*this);
    b.move (p);
    return b;
  }

  /**
   *  @brief Returns the enlarged edge
   *
   *  Enlarges the edge by the given offset and returns the 
   *  moved edge. The edge is not modified. Enlargement means
   *  that the first point is shifted by -p, the second by p.
   *
   *  @param p The distance to move the edge.
   * 
   *  @return The moved edge.
   */
  edge<C> enlarged (const vector<C> &p) const
  {
    edge<C> b (*this);
    b.enlarge (p);
    return b;
  }

  /**
   *  @brief Extends the edge
   *
   *  The extension is applied parallel to the edge at the 
   *  start and end point.
   *  Degenerated edges become horizontal edges.
   */
  edge<C> &extend (C e) 
  {
    vector<double> dp;
    if (is_degenerate ()) {
      dp = vector<double> (e, 0.0);
    } else {
      dp = d () * (double (e) / double_length ());
    }
    *this = edge<C> (point<C> (point<double> (p1 ()) - dp), point<C> (point<double> (p2 ()) + dp));
    return *this;
  }

  /**
   *  @brief Returns the extended edge
   *
   *  The extension is applied parallel to the edge at the 
   *  start and end point.
   *  Degenerated edges become horizontal edges.
   */
  edge<C> extended (C e) const
  {
    vector<double> dp;
    if (is_degenerate ()) {
      dp = vector<double> (e, 0.0);
    } else {
      dp = d () * (double (e) / double_length ());
    }
    return edge<C> (point<C> (point<double> (p1 ()) - dp), point<C> (point<double> (p2 ()) + dp));
  }

  /**
   *  @brief Returns the shifted edge
   *
   *  The shift is applied perpendicular to the edge to the left of the edge 
   *  if the shift is positive and to the right if negative.
   *  Degenerated edges are not shifted.
   */
  edge<C> shifted (C e) const
  {
    if (is_degenerate ()) {
      return *this;
    } else {
      vector<double> dp = d () * (double (e) / double_length ());
      dp = vector<double> (-dp.y (), dp.x ());
      return edge<C> (point<C> (point<double> (p1 ()) + dp), point<C> (point<double> (p2 ()) + dp));
    }
  }

  /**
   *  @brief Shifts the edge
   *
   *  The shift is applied perpendicular to the edge to the left of the edge 
   *  if the shift is positive and to the right if negative.
   *  Degenerated edges are not shifted.
   */
  edge<C> &shift (C e) 
  {
    if (! is_degenerate ()) {
      vector<double> dp = d () * (double (e) / double_length ());
      dp = vector<double> (-dp.y (), dp.x ());
      *this = edge<C> (point<C> (point<double> (p1 ()) + dp), point<C> (point<double> (p2 ()) + dp));
    }
    return *this;
  }

  /**
   *  @brief Transform the edge.
   *
   *  Transforms the edge with the given transformation.
   *  Modifies the edge with the transformed edge.
   *  
   *  @param t The transformation to apply.
   *
   *  @return The transformed edge.
   */
  template <class Tr>
  edge<C> &transform (const Tr &t)
  {
    if (t.is_mirror ()) {
      //  NOTE: in case of mirroring transformations we swap p1 and p2. The reasoning is that
      //  this way we maintain the orientation semantics: "right" of the edge is "inside" of
      //  an area.
      *this = edge<C> (t * m_p2, t * m_p1);
    } else {
      *this = edge<C> (t * m_p1, t * m_p2);
    }
    return *this;
  }

  /**
   *  @brief Transform the edge.
   *
   *  Transforms the edge with the given transformation.
   *  Does not modify the edge but returns the transformed edge.
   *  
   *  @param t The transformation to apply.
   *
   *  @return The transformed edge.
   */
  template <class Tr>
  edge<typename Tr::target_coord_type> transformed (const Tr &t) const
  {
    if (t.is_mirror ()) {
      //  NOTE: in case of mirroring transformations we swap p1 and p2. The reasoning is that
      //  this way we maintain the orientation semantics: "right" of the edge is "inside" of
      //  an area.
      return edge<typename Tr::target_coord_type> (t * m_p2, t * m_p1);
    } else {
      return edge<typename Tr::target_coord_type> (t * m_p1, t * m_p2);
    }
  }

  /**
   *  @brief Moves the edge.
   *
   *  Moves the edge by the given offset and returns the 
   *  moved edge. The edge is overwritten.
   *
   *  @param p The distance to move the edge.
   * 
   *  @return The moved edge.
   */
  edge<C> &move (const vector<C> &p)
  {
    m_p1 += p;
    m_p2 += p;
    return *this;
  }

  /**
   *  @brief Enlarges the edge.
   *
   *  Enlarges the edge by the given distance and returns the 
   *  enlarged edge. The edge is overwritten.
   *
   *  @param p The distance to move the edge points.
   * 
   *  @return The enlarged edge.
   */
  edge<C> &enlarge (const vector<C> &p)
  {
    m_p1 -= p;
    m_p2 += p;
    return *this;
  }

  /**
   *  @brief Set the first point.
   */
  void set_p1 (const point<C> &p) 
  {
    m_p1 = p;
  }

  /**
   *  @brief Set the second point.
   */
  void set_p2 (const point<C> &p) 
  {
    m_p2 = p;
  }

  /**
   *  @brief The first point.
   */
  const point<C> &p1 () const
  {
    return m_p1;
  }

  /**
   *  @brief The second point.
   */
  const point<C> &p2 () const
  {
    return m_p2;
  }

  /**
   *  @brief Returns the bounding box
   */
  box_type bbox () const
  {
    return box_type (m_p1, m_p2);
  }

  /**
   *  @brief The horizontal extend of the edge.
   */
  vector_type d () const
  {
    return vector_type (dx (), dy ());
  }

  /**
   *  @brief The horizontal extend of the edge.
   */
  C dx () const
  {
    return m_p2.x () - m_p1.x ();
  }

  /**
   *  @brief The vertical extend of the edge.
   */
  C dy () const
  {
    return m_p2.y () - m_p1.y ();
  }

  /**
   *  @brief Shortcut for p1().x()
   */
  C x1 () const
  {
    return m_p1.x ();
  }

  /**
   *  @brief Shortcut for p1().y()
   */
  C y1 () const
  {
    return m_p1.y ();
  }

  /**
   *  @brief Shortcut for p2().x()
   */
  C x2 () const
  {
    return m_p2.x ();
  }

  /**
   *  @brief Shortcut for p2().y()
   */
  C y2 () const
  {
    return m_p2.y ();
  }

  /**
   *  @brief The absolute value of the horizontal extend of the edge.
   *
   *  This function is safe against coordinate overflow for int32
   *  types.
   */
  distance_type dx_abs () const
  {
    return m_p2.x () > m_p1.x () ? m_p2.x () - m_p1.x () : m_p1.x () - m_p2.x ();
  }

  /**
   *  @brief The vertical extend of the edge.
   *
   *  This function is safe against coordinate overflow for int32
   *  types.
   */
  distance_type dy_abs () const
  {
    return m_p2.y () > m_p1.y () ? m_p2.y () - m_p1.y () : m_p1.y () - m_p2.y ();
  }

  /**
   *  @brief Test if the edge is orthogonal (vertical or horizontal)
   */
  bool is_ortho () const
  {
    return m_p1.x () == m_p2.x () || m_p1.y () == m_p2.y ();
  }

  /**
   *  @brief Test for degenerated edge
   */
  bool is_degenerate () const 
  {
    return m_p1 == m_p2;
  }

  /**
   *  @brief The length of the edge
   */
  distance_type length () const
  {
    double ddx (m_p2.x () - m_p1.x ());
    double ddy (m_p2.y () - m_p1.y ());
    return coord_traits::rounded_distance (sqrt (ddx * ddx + ddy * ddy));
  }

  /**
   *  @brief The length of the edge
   */
  double double_length () const
  {
    double ddx (m_p2.x () - m_p1.x ());
    double ddy (m_p2.y () - m_p1.y ());
    return sqrt (ddx * ddx + ddy * ddy);
  }

  /**
   *  @brief The square of the length of the edge
   */
  area_type sq_length () const
  {
    return coord_traits::sq_length (m_p2.x (), m_p2.y (), m_p1.x (), m_p1.y ());
  }

  /**
   *  @brief The square of the length of the edge
   */
  double double_sq_length () const
  {
    double ddx (m_p2.x () - m_p1.x ());
    double ddy (m_p2.y () - m_p1.y ());
    return (ddx * ddx + ddy * ddy);
  }

  /**
   *  @brief The orthogonal length of the edge
   *
   *  @return The orthogonal length (abs(dx)+abs(dy))
   */
  distance_type ortho_length () const
  {
    return dx_abs () + dy_abs ();
  }

  /**
   *  @brief Conversion to a string.
   *
   *  If dbu is set, it determines the factor by which the coordinates are multiplied to render
   *  micron units. In addition, a micron format is chosen for output of these coordinates.
   */
  std::string to_string (double dbu = 0.0) const
  {
    return "(" + m_p1.to_string (dbu) + ";" + m_p2.to_string (dbu) + ")";
  }
  
  /**
   *  @brief Reduce the edge
   *
   *  Reduction of a edge normalizes the edge by extracting
   *  a suitable transformation and placing the edge in a unique
   *  way. In this implementation, p1 is set to zero.
   *
   *  @return The transformation that must be applied to render the original edge
   */
  void reduce (simple_trans<coord_type> &tr)
  {
    point_type d (m_p1);
    move (-d);
    tr = simple_trans<coord_type> (simple_trans<coord_type>::r0, d);
  }

  /**
   *  @brief Reduce the edge
   *
   *  Reduction of a edge normalizes the edge by extracting
   *  a suitable transformation and placing the edge in a unique
   *  way. In this implementation, p1 is set to zero.
   *
   *  @return The transformation that must be applied to render the original edge
   */
  void reduce (disp_trans<coord_type> &tr)
  {
    vector_type d (m_p1);
    move (-d);
    tr = disp_trans<coord_type> (d);
  }

  /**
   *  @brief Reduce the edge
   *
   *  Reduction of a edge normalizes the edge by extracting
   *  a suitable transformation and placing the edge in a unique
   *  way. In this implementation, p1 is set to zero.
   *
   *  @return The transformation that must be applied to render the original edge
   */
  void reduce (unit_trans<coord_type> & /*tr*/)
  {
    //  .. no reduction possible ..
  }

  /**
   *  @brief Test for being parallel
   *
   *  @param e The edge to test against
   *
   *  @return True if both edges are parallel
   */
  bool parallel (const db::edge<C> &e) const
  {
    return coord_traits::vprod_sign (m_p2.x () - m_p1.x (), m_p2.y () - m_p1.y (),
                                     e.m_p2.x () - e.m_p1.x (), e.m_p2.y () - e.m_p1.y (),
                                     0, 0) == 0;
  }

  /**
   *  @brief Test whether a point is on an edge.
   *
   *  A point is on a edge if it is on (at least closer 
   *  than a grid point) the edge.
   *
   *  @param The point to test with the edge.
   *
   *  @return True if the point is on the edge.
   */
  bool contains (const db::point<C> &p) const
  {
    if (is_degenerate ()) {
      return m_p1 == p;
    } else {
      return distance_abs (p) < coord_traits::prec_distance () &&
             coord_traits::sprod_sign (p.x (), p.y (), m_p2.x (), m_p2.y (), m_p1.x (), m_p1.y ()) >= 0 &&
             coord_traits::sprod_sign (p.x (), p.y (), m_p1.x (), m_p1.y (), m_p2.x (), m_p2.y ()) >= 0;
    }
  }

  /**
   *  @brief Test whether a point is on an edge excluding the endpoints.
   *
   *  A point is on a edge if it is on (at least closer 
   *  than a grid point) the edge.
   *
   *  @param The point to test with the edge.
   *
   *  @return True if the point is on the edge but not equal p1 or p2.
   */
  bool contains_excl (const db::point<C> &p) const
  {
    if (is_degenerate ()) {
      return false;
    } else {
      return distance_abs (p) < coord_traits::prec_distance () &&
             coord_traits::sprod_sign (p.x (), p.y (), m_p2.x (), m_p2.y (), m_p1.x (), m_p1.y ()) > 0 &&
             coord_traits::sprod_sign (p.x (), p.y (), m_p1.x (), m_p1.y (), m_p2.x (), m_p2.y ()) > 0;
    }
  }

  /**
   *  @brief Coincidence check.
   *
   *  Checks whether a edge is coincident with another edge. 
   *  Coincidence is defined by being parallel, oriented the same way and that 
   *  both edges share more than one point.
   *
   *  @param e the edge to test with
   *
   *  @return True if the edges are coincident.
   */
  bool coincident (const db::edge<C> &e) const
  {
    return ! is_degenerate () && ! e.is_degenerate () &&
           distance_abs (e.p1 ()) < coord_traits::prec_distance () && 
           distance_abs (e.p2 ()) < coord_traits::prec_distance () && 
           (sprod_sign (*this, e) < 0 ? 
            (coord_traits::sprod_sign (e.p2 ().x (), e.p2 ().y (), p1 ().x (), p1 ().y (), p2 ().x (), p2 ().y ()) > 0 && 
             coord_traits::sprod_sign (e.p1 ().x (), e.p1 ().y (), p2 ().x (), p2 ().y (), p1 ().x (), p1 ().y ()) > 0) :
            (coord_traits::sprod_sign (e.p1 ().x (), e.p1 ().y (), p1 ().x (), p1 ().y (), p2 ().x (), p2 ().y ()) > 0 && 
             coord_traits::sprod_sign (e.p2 ().x (), e.p2 ().y (), p2 ().x (), p2 ().y (), p1 ().x (), p1 ().y ()) > 0));
  }

  /**
   *  @brief Intersection test. 
   *
   *  Returns true if the edges intersect. 
   *  If the edges coincide, they also intersect.
   *  For degenerated edges, the intersection is mapped to
   *  point containment tests.
   *
   *  @param e The edge to test.
   */
  bool intersect (const db::edge<C> &e) const
  {
    if (is_degenerate ()) {
      return e.contains (p1 ());
    } else if (e.is_degenerate ()) {
      return contains (e.p1 ());
    } else if (! box_type (p1 (), p2 ()).touches (box_type (e.p1 (), e.p2 ()))) {
      return false;
    } else if (is_ortho () && e.is_ortho ()) {
      return true;
    } else {
      return crossed_by (e) && e.crossed_by (*this);
    }
  }

  /**
   *  @brief Intersection test with intersect point. 
   *
   *  Returns true if the edges intersect and returns the
   *  intersection point if true. For coinciding edges one
   *  of the points that coincide is returned.
   *
   *  @param e The edge to test.
   *
   *  @return A pair <bool,point> with true as the first element
   *  if the edges intersect and the intersection point as the second.
   *  If the edges do not intersect, returns <false,undef.>.
   */
  std::pair <bool, db::point<C> > intersect_point (const db::edge<C> &e) const
  {
    if (is_degenerate ()) {
      if (e.contains (p1 ())) {
        return std::make_pair (true, p1 ());
      } else {
        return std::make_pair (false, db::point<C> (0, 0));
      }
    } else if (e.is_degenerate ()) {
      if (contains (e.p1 ())) {
        return std::make_pair (true, e.p1 ());
      } else {
        return std::make_pair (false, db::point<C> (0, 0));
      }
    } else if (! box_type (p1 (), p2 ()).touches (box_type (e.p1 (), e.p2 ()))) {
      return std::make_pair (false, db::point<C> (0, 0));
    } else if (is_ortho () && e.is_ortho ()) {
      coord_type x = std::max (std::min (p1 ().x (), p2 ().x ()), std::min (e.p1 ().x (), e.p2 ().x ()));
      coord_type y = std::max (std::min (p1 ().y (), p2 ().y ()), std::min (e.p1 ().y (), e.p2 ().y ()));
      return std::make_pair (true, db::point<C> (x, y));
    } else if (! crossed_by (e)) {
      return std::make_pair (false, db::point<C> (0, 0));
    } else {

      bool res = true;
      bool ends_on_edge = false;

      std::pair<area_type, int> vsa = coord_traits::vprod_with_sign (e.p2 ().x (), e.p2 ().y (), m_p1.x (), m_p1.y (), e.p1 ().x (), e.p1 ().y ());
      area_type vxa = vsa.first;
      if (vsa.second < 0) {
        res = false;
      } else if (vsa.second == 0) {
        ends_on_edge = true;
      }

      std::pair<area_type, int> vsb = coord_traits::vprod_with_sign (e.p2 ().x (), e.p2 ().y (), m_p2.x (), m_p2.y (), e.p1 ().x (), e.p1 ().y ());
      area_type vxb = -vsb.first;
      if (vsb.second > 0) {
        res = !res;
      } else if (vsb.second == 0) {
        ends_on_edge = true;
      }

      if (ends_on_edge) {

        if (contains (e.p1 ())) {
          return std::make_pair (true, e.p1 ());
        } else if (contains (e.p2 ())) {
          return std::make_pair (true, e.p2 ());
        } else if (e.contains (p1 ())) {
          return std::make_pair (true, p1 ());
        } else if (e.contains (p2 ())) {
          return std::make_pair (true, p2 ());
        } else {
          return std::make_pair (false, db::point<C> (0, 0));
        }

      } else if (res) {

        if (vxa < 0) {
          vxa = -vxa;
        }
        if (vxb < 0) {
          vxb = -vxb;
        }

        coord_type x = m_p1.x () + div_exact (dx (), vxa, vxa + vxb);
        coord_type y = m_p1.y () + div_exact (dy (), vxa, vxa + vxb);

        return std::make_pair (true, db::point<C> (x, y));

      } else {
        return std::make_pair (false, db::point<C> (0, 0));
      }

    }
  }

  /**
   *  @brief Distance between the edge and a point.
   *
   *  Returns the distance between the edge and the point. The 
   *  distance is signed which is negative if the point is to the
   *  "right" of the edge and positive if the point is to the "left".
   *  The distance is measured by projecting the point onto the
   *  line through the edge. If the edge is degenerated, the distance
   *  is not defined.
   *
   *  The distance is through as a distance of the point from the line
   *  through the edge.
   *
   *  @param p The point to test.
   *
   *  @return The distance
   */
  coord_type distance (const db::point<C> &p) const
  {
    //  the distance is computed from 
    //    d = (a x b) / sqrt (a * a)
    //  where b = p - p1, a = p2 - p1
    if (is_degenerate ()) {
      //  for safety handle this case - without a reasonable result
      return 0;
    } else {
      //  compute the distance as described above 
      area_type axb = coord_traits::vprod (m_p2.x (), m_p2.y (), p.x (), p.y (), m_p1.x (), m_p1.y ()); 
      double d = double (axb) / double (length ());
      //  and round
      return coord_traits::rounded (d);
    }
  }

  /**
   *  @brief Gets the distance of the point from the edge.
   *
   *  The distance is computed as the minimum distance of the point to any of the edge's
   *  points.
   *
   *  @param p The point whose distance is to be computed
   *
   *  @return The distance
   */
  distance_type euclidian_distance (const db::point<C> &p)
  {
    if (db::sprod_sign (p - p1 (), d ()) < 0) {
      return p1 ().distance (p);
    } else if (db::sprod_sign (p - p2 (), d ()) > 0) {
      return p2 ().distance (p);
    } else {
      return std::abs (distance (p));
    }
  }

  /**
   *  @brief Side of the point
   *
   *  Returns 1 if the point is "left" of the edge, 0 if on
   *  and -1 if the point is "right" of the edge.
   *
   *  @param p The point to test.
   *
   *  @return The side value
   */
  int side_of (const db::point<C> &p) const
  {
    //  the distance is computed from 
    //    d = (a x b) / sqrt (a * a)
    //  where b = p - p1, a = p2 - p1
    if (is_degenerate ()) {
      //  for safety handle this case - without a reasonable result
      return 0;
    } else {
      //  compute the side as the sign of the distance as in "distance"
      return coord_traits::vprod_sign (m_p2.x (), m_p2.y (), p.x (), p.y (), m_p1.x (), m_p1.y ());
    }
  }

  /**
   *  @brief Absolute distance between the edge and a point.
   *
   *  Returns the distance between the edge and the point. 
   *
   *  @param p The point to test.
   *
   *  @return The distance
   */
  distance_type distance_abs (const db::point<C> &p) const
  {
    //  the distance is computed from 
    //    d = (a x b) / sqrt (a * a)
    //  where b = p - p1, a = p2 - p1
    if (is_degenerate ()) {
      //  for safety handle this case - without a reasonable result
      return 0;
    } else {
      //  compute the distance as described above 
      area_type axb = coord_traits::vprod (m_p2.x (), m_p2.y (), p.x (), p.y (), m_p1.x (), m_p1.y ()); 
      double d = fabs (double (axb)) / double (length ());
      //  and round
      return coord_traits::rounded_distance (d);
    }
  }

  /**
   *  @brief Swaps the points of the edge
   */
  edge<C> &swap_points () 
  {
    std::swap (m_p1, m_p2);
    return *this;
  }

  /**
   *  @brief Returns the edge with swapped points
   */
  edge<C> swapped_points () const
  {
    edge<C> e = *this;
    e.swap_points ();
    return e;
  }

  /**
   *  @brief Clip the line given by the edge at the given box
   *
   *  Determines the part of the line (given by the edge) that runs through the given box.
   *  If the line does not hit the box, false is returned in the first member of the 
   *  return value.
   *
   *  @return first: false if the line does not hit the box, second: the part of the line inside the box
   */
  std::pair<bool, edge> clipped_line (const db::box<C> &box) const
  {
    if (box.empty ()) {
      return std::make_pair (false, db::edge<C> ());
    }

    std::pair <bool, db::point<C> > pc1 (false, db::point<C> ()), pc2 (false, db::point<C> ());

    pc1 = cut_point (db::edge<C> (box.p1 (), db::point<C> (box.p1 ().x (), box.p2 ().y ())));
    if (pc1.first) {
      pc2 = cut_point (db::edge<C> (db::point<C> (box.p2 ().x (), box.p1 ().y ()), box.p2 ()));
    }

    if (! pc1.first || ! pc2.first) {
      pc1 = cut_point (db::edge<C> (box.p1 (), db::point<C> (box.p2 ().x (), box.p1 ().y ())));
      if (pc1.first) {
        pc2 = cut_point (db::edge<C> (db::point<C> (box.p1 ().x (), box.p2 ().y ()), box.p2 ()));
      }
    }

    if (pc1.first && pc2.first) {
      return db::edge<C> (pc1.second, pc2.second).clipped (box);
    } else {
      return std::make_pair (false, db::edge<C> ());
    }
  }

  /**
   *  @brief Clip at rectangle
   *
   *  Clips the edge at the box provided. Maintains the orientation of the
   *  edge.
   *
   *  @return first: false if edge disappears. second: the clipped edge
   */
  std::pair<bool, edge> clipped (const db::box<C> &box) const
  {
    if (box.empty ()) {
      return std::make_pair (false, db::edge<C> ());
    }

    bool swapped = false;

    db::point<C> p1 (m_p1), p2 (m_p2);

    if (p1.x () > p2.x ()) {
      std::swap (p1, p2);
      swapped = !swapped;
    }

    if (p2.x () < box.left ()) {
      return std::make_pair (false, db::edge<C>());
    } else if (p1.x () < box.left ()) {
      p1 = db::point<C> (box.left (), m_p1.y () + db::coord_traits<C>::rounded((double)(box.left () - m_p1.x ()) * double (dy ()) / double (dx ())));
    }
    if (p1.x () > box.right ()) {
      return std::make_pair (false, db::edge<C> ());
    } else if (p2.x () > box.right ()) {
      p2 = db::point<C> (box.right (), m_p1.y () + db::coord_traits<C>::rounded((double)(box.right () - m_p1.x ()) * double (dy ()) / double (dx ())));
    }
    
    if (p1.y () > p2.y ()) {
      std::swap (p1, p2);
      swapped = !swapped;
    }

    if (p2.y () < box.bottom ()) {
      return std::make_pair (false, db::edge<C> ());
    } else if (p1.y () < box.bottom ()) {
      p1 = db::point<C> (std::max (box.left (), std::min (box.right (), m_p1.x () + db::coord_traits<C>::rounded((double)(box.bottom () - m_p1.y ()) * double (dx ()) / double (dy ())))), box.bottom ());
    }
    if (p1.y () > box.top ()) {
      return std::make_pair (false, db::edge<C> ());
    } else if (p2.y () > box.top ()) {
      p2 = db::point<C> (std::max (box.left (), std::min (box.right (), m_p1.x () + db::coord_traits<C>::rounded((double)(box.top () - m_p1.y ()) * double (dx ()) / double (dy ())))), box.top ());
    }

    if (swapped) {
      return std::make_pair (true, db::edge<C> (p2, p1));
    } else {
      return std::make_pair (true, db::edge<C> (p1, p2));
    }
  }

  /** 
   *  @brief Check, if an edge is cut by a line (given by an edge)
   *
   *  This method returns true if p1 is in one semispace 
   *  while p2 is in the other or one of them is on the line
   *  through the edge
   */
  bool crossed_by (const db::edge <C> &e) const
  {
    //  this is basically this algorithm:
    //  res = true;
    //  if (side_of (e.p1 ()) < 0) {
    //    res = false
    //  } else if (side_of (e.p1 ()) == 0) {
    //    return true;
    //  } 
    //  if (side_of (e.p2 ()) > 0) {
    //    res = !res
    //  } else if (side_of (e.p2 ()) == 0) {
    //    return true;
    //  }
    //  return res;

    bool res = true;

    int vsa = coord_traits::vprod_sign (m_p2.x (), m_p2.y (), e.p1 ().x (), e.p1 ().y (), m_p1.x (), m_p1.y ());
    if (vsa < 0) {
      res = false;
    } else if (vsa == 0) {
      return true;
    }

    int vsb = coord_traits::vprod_sign (m_p2.x (), m_p2.y (), e.p2 ().x (), e.p2 ().y (), m_p1.x (), m_p1.y ());
    if (vsb > 0) {
      res = !res;
    } else if (vsb == 0) {
      return true;
    }

    return res;
  }

  /** 
   *  @brief Check, if an edge is cut by a line (given by an edge)
   *
   *  This method returns true if p1 is in one semispace 
   *  while p2 is in the other or one of them is on the line
   *  through the edge. In addition to "crossed_by", also returns
   *  the point at which the edge is crossed.
   */
  std::pair <bool, db::point<C> > crossed_by_point (const db::edge <C> &e) const
  {
    bool res = true;

    std::pair<area_type, int> vsa = coord_traits::vprod_with_sign (m_p2.x (), m_p2.y (), e.p1 ().x (), e.p1 ().y (), m_p1.x (), m_p1.y ());
    area_type vxa = vsa.first;
    if (vsa.second < 0) {
      res = false;
    } else if (vsa.second == 0) {
      return std::make_pair (true, e.p1 ());
    }

    std::pair<area_type, int> vsb = coord_traits::vprod_with_sign (m_p2.x (), m_p2.y (), e.p2 ().x (), e.p2 ().y (), m_p1.x (), m_p1.y ());
    area_type vxb = -vsb.first;
    if (vsb.second > 0) {
      res = !res;
    } else if (vsb.second == 0) {
      return std::make_pair (true, e.p2 ());
    }

    if (res) {

      if (vxa < 0) {
        vxa = -vxa;
      }
      if (vxb < 0) {
        vxb = -vxb;
      }

      coord_type x = e.p1 ().x () + div_exact (e.dx (), vxa, vxa + vxb);
      coord_type y = e.p1 ().y () + div_exact (e.dy (), vxa, vxa + vxb);

      return std::make_pair (true, db::point<C> (x, y));

    } else {
      return std::make_pair (false, db::point<C> (0, 0));
    }
  }

  /** 
   *  @brief Compute the projection of a point on the edge
   *
   *  This method returns true in the first member of the return
   *  value if the point can be projected on the edge. In this case,
   *  the second member will return the point projected on the edge.
   *
   *  @param point The point to be projected
   */
  std::pair <bool, db::point<C> > projected (const db::point<C> &pt) const
  {
    return db::edge <C> (pt, pt + db::vector<C> (dy (), -dx ())).crossed_by_point (*this);
  }

  /**
   *  @brief Compute cut point of two lines (given by edges)
   *
   *  This method returns true in the first member if both edges cut.
   *  If this is the case, the second member of the returned pair is
   *  the point at which the edges would intersect, given they are 
   *  extended beyond their ends.
   */
  std::pair <bool, db::point<C> > cut_point (const db::edge<C> &e2) const
  {
    std::pair<typename coord_traits::area_type, int> vps = coord_traits::vprod_with_sign (e2.dx (), e2.dy (), this->dx (), this->dy (), 0, 0);
    if (vps.second != 0) {
      double pr1 = double (coord_traits::vprod (e2.p1 ().x (), e2.p1 ().y (), this->p2 ().x (), this->p2 ().y (), this->p1 ().x (), this->p1 ().y ()));
      double pr2 = double (vps.first);
      db::point<C> p = e2.p1 () - db::vector<C> ((e2.p2 () - e2.p1 ()) * (pr1 / pr2));
      return std::make_pair (true, p);
    } else {
      return std::make_pair (false, db::point<C> (0, 0));
    }
  }

private:
  point<C> m_p1, m_p2;
};

/**
 *  @brief "intersect" binary predicate
 */
template <class Edge>
struct edges_intersect
{
  bool operator() (const Edge &e1, const Edge &e2) const
  {
    return e1.intersect (e2);
  }
};

/**
 *  @brief Scaling of an edge
 *
 *  @param e The edge to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled edge
 */ 
template <class C>
inline edge<double> 
operator* (const edge<C> &e, double s)
{
  return edge<double> (e.p1 () * s, e.p2 () * s);
}

/**
 *  @brief Binary * operator (transformation)
 *
 *  Transforms the edge with the given transformation and 
 *  returns the result.
 *
 *  @param t The transformation to apply
 *  @param e The edge to transform
 *  @return t * e
 */
template <class Tr>
inline edge<typename Tr::target_coord_type> 
operator* (const Tr &t, const edge<typename Tr::coord_type> &e)
{
  return e.transformed (t);
}

/**
 *  @brief Output stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const edge<C> &e)
{
  return (os << e.to_string ());
}

/**
 *  @brief The standard edge typedef
 */
typedef edge<db::Coord>  Edge;

/**
 *  @brief The double coordinate edge typedef
 */
typedef edge<db::DCoord> DEdge;

/**
 *  @brief Convenience wrappers for coord_traits functions: vector product: p x q
 */
template <class C>
typename db::coord_traits<C>::area_type vprod (const db::edge<C> &p, const db::edge<C> &q)
{
  return db::coord_traits<C>::vprod (p.dx (), p.dy (), q.dx (), q.dy (), 0, 0);
}

/**
 *  @brief Convenience wrappers for coord_traits functions: vector product sign: sign(p x q)
 */
template <class C>
int vprod_sign (const db::edge<C> &p, const db::edge<C> &q)
{
  return db::coord_traits<C>::vprod_sign (p.dx (), p.dy (), q.dx (), q.dy (), 0, 0);
}

/**
 *  @brief Convenience wrappers for coord_traits functions: scalar product: 0->p x 0->q
 */
template <class C>
typename db::coord_traits<C>::area_type sprod (const db::edge<C> &p, const db::edge<C> &q)
{
  return db::coord_traits<C>::sprod (p.dx (), p.dy (), q.dx (), q.dy (), 0, 0);
}

/**
 *  @brief Convenience wrappers for coord_traits functions: scalar product sign: sign(0->p x 0->q)
 */
template <class C>
int sprod_sign (const db::edge<C> &p, const db::edge<C> &q)
{
  return db::coord_traits<C>::sprod_sign (p.dx (), p.dy (), q.dx (), q.dy (), 0, 0);
}

/**
 *  @brief Determines the lower bound of the edge
 */
template <class C>
inline C edge_ymin (const db::edge<C> &e) 
{
  return std::min (e.p1 ().y (), e.p2 ().y ());
}

/**
 *  @brief Determines the upper bound of the edge
 */
template <class C>
inline C edge_ymax (const db::edge<C> &e) 
{
  return std::max (e.p1 ().y (), e.p2 ().y ());
}

/**
 *  @brief Determines the left bound of the edge
 */
template <class C>
inline C edge_xmin (const db::edge<C> &e) 
{
  return std::min (e.p1 ().x (), e.p2 ().x ());
}

/**
 *  @brief Determines the right bound of the edge
 */
template <class C>
inline C edge_xmax (const db::edge<C> &e) 
{
  return std::max (e.p1 ().x (), e.p2 ().x ());
}

/**
 *  @brief Computes the x value of an edge at the given y value
 *
 *  HINT: for application in the scanline algorithm 
 *  it is important that this method delivers exactly (!) the same x for the same edge 
 *  (after normalization to dy()>0) and same y!
 */
template <class C>
inline double edge_xaty (db::edge<C> e, C y)
{
  if (e.p1 ().y () > e.p2 ().y ()) {
    e.swap_points ();
  }

  if (y <= e.p1 ().y ()) {
    return e.p1 ().x ();
  } else if (y >= e.p2 ().y ()) {
    return e.p2 ().x ();
  } else {
    return double (e.p1 ().x ()) + double (e.dx ()) * double (y - e.p1 ().y ()) / double (e.dy ());
  }
}

/**
 *  @brief Functor that compares two edges by their lower bound.
 */
template <class C>
class edge_ymin_compare
{
public:
  bool operator() (const db::edge<C> &a, const db::edge<C> &b) const
  {
    C ya = edge_ymin (a);
    C yb = edge_ymin (b);
    if (ya != yb) {
      return ya < yb;
    } else {
      return a < b;
    }
  }
};

/**
 *  @brief Functor that compares two edges by their upper bound.
 */
template <class C>
class edge_ymax_compare
{
public:
  bool operator() (const db::edge<C> &a, const db::edge<C> &b) const
  {
    C ya = edge_ymax (a);
    C yb = edge_ymax (b);
    if (ya != yb) {
      return ya < yb;
    } else {
      return a < b;
    }
  }
};

/**
 *  @brief Functor that compares two edges by their left bound.
 */
template <class C>
class edge_xmin_compare
{
public:
  bool operator() (const db::edge<C> &a, const db::edge<C> &b) const
  {
    C ya = edge_xmin (a);
    C yb = edge_xmin (b);
    if (ya != yb) {
      return ya < yb;
    } else {
      return a < b;
    }
  }
};

/**
 *  @brief Functor that compares two edges by their right bound.
 */
template <class C>
class edge_xmax_compare
{
public:
  bool operator() (const db::edge<C> &a, const db::edge<C> &b) const
  {
    C ya = edge_xmax (a);
    C yb = edge_xmax (b);
    if (ya != yb) {
      return ya < yb;
    } else {
      return a < b;
    }
  }
};

/**
 *  @brief Computes the left bound of the edge geometry for a given band [y1..y2].
 */
template <class C>
inline C edge_xmin_at_yinterval (const db::edge<C> &e, C y1, C y2) 
{
  if (e.dx () == 0) {
    return e.p1 ().x ();
  } else if (e.dy () == 0) {
    return std::min (e.p1 ().x (), e.p2 ().x ());
  } else {
    return C (floor (edge_xaty (e, ((e.dy () < 0) ^ (e.dx () < 0)) == 0 ? y1 : y2)));
  }
}

/**
 *  @brief Computes the right bound of the edge geometry for a given band [y1..y2].
 */
template <class C>
inline C edge_xmax_at_yinterval (const db::edge<C> &e, C y1, C y2) 
{
  if (e.dx () == 0) {
    return e.p1 ().x ();
  } else if (e.dy () == 0) {
    return std::max (e.p1 ().x (), e.p2 ().x ());
  } else {
    return C (ceil (edge_xaty (e, ((e.dy () < 0) ^ (e.dx () < 0)) != 0 ? y1 : y2)));
  }
}

/**
 *  @brief Functor that compares two edges by their left bound for a given interval [y1..y2].
 *
 *  This function is intended for use in scanline scenarios to determine what edges are 
 *  interacting in a certain y interval.
 */
template <class C>
struct edge_xmin_at_yinterval_compare
{
  edge_xmin_at_yinterval_compare (C y1, C y2)
    : m_y1 (y1), m_y2 (y2)
  {
    // .. nothing yet ..
  }

  bool operator() (const db::edge<C> &a, const db::edge<C> &b) const
  {
    if (edge_xmax (a) < edge_xmin (b)) {
      return true;
    } else if (edge_xmin (a) >= edge_xmax (b)) {
      return false;
    } else {
      C xa = edge_xmin_at_yinterval (a, m_y1, m_y2);
      C xb = edge_xmin_at_yinterval (b, m_y1, m_y2);
      if (xa != xb) {
        return xa < xb;
      } else {
        return a < b;
      }
    }
  }

public:
  C m_y1, m_y2;
};

} // namespace db

/**
 *  @brief Special extractors for the edges
 */

namespace tl 
{
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Edge &b);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DEdge &b);

  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Edge &b);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DEdge &b);

} // namespace tl

#endif



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


#ifndef HDR_dbEdgePair
#define HDR_dbEdgePair

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbPoint.h"
#include "dbVector.h"
#include "dbTrans.h"
#include "dbObjectTag.h"
#include "dbBox.h"
#include "dbPolygon.h"
#include "dbEdge.h"

#include <string>

namespace db {

/**
 *  @brief A pair of edges
 *
 *  An edge pair is a pair of edges which usually is used to mark a DRC violation. It flags the relationship of two edges.
 *  It's a compoisition of two edges: first and second.
 *
 *  By default, an edge pair is directed: first and second are not commutable and indicate a relationship ("from first towards second").
 *  The edge pair carries a flag which allows indicating symmetric mode: in this mode, the first and second edge are commutable.
 *  As a consequence, when the symmetric flag is used, edge_pair(e1, e2, true) == edge_pair(e2, e1, true).
 */
template <class C>
class DB_PUBLIC_TEMPLATE edge_pair
{
public:
  typedef C coord_type;
  typedef db::edge<C> edge_type;
  typedef db::box<C> box_type;
  typedef db::point<C> point_type;
  typedef db::vector<C> vector_type;
  typedef db::polygon<C> polygon_type;
  typedef db::simple_polygon<C> simple_polygon_type;
  typedef db::coord_traits<C> coord_traits;
  typedef typename coord_traits::distance_type distance_type; 
  typedef typename coord_traits::area_type area_type; 
  typedef typename coord_traits::perimeter_type perimeter_type;
  typedef db::object_tag< edge_pair<C> > tag;

  /**
   *  @brief The default constructor.
   *  
   *  The default constructor creates an edge pair with two default edges.
   */
  edge_pair ()
    : m_first (), m_second (), m_symmetric (false)
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The standard constructor taking two edges
   *
   *  @param first The first edge
   *  @param second The second edge
   *  @param symmetric True, if the edge pair is symmetric
   */
  template <class D>
  edge_pair (const db::edge<D> &first, const db::edge<D> &second, bool symmetric = false)
    : m_first (first), m_second (second), m_symmetric (symmetric)
  {
    //  .. nothing else ..
  }

  /**
   *  @brief The copy constructor
   */
  template <class D>
  edge_pair (const edge_pair<D> &e)
    : m_first (e.first ()), m_second (e.second ()), m_symmetric (e.is_symmetric ())
  {
    //  .. nothing else ..
  }

  /**
   *  @brief Gets the symmetric flag
   */
  bool is_symmetric () const
  {
    return m_symmetric;
  }

  /**
   *  @brief Sets the symmetric flag
   */
  void set_symmetric (bool s)
  {
    m_symmetric = s;
  }

  /**
   *  @brief The (dummy) translation operator
   */
  void translate (const edge_pair<C> &d, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
  }

  /**
   *  @brief The (dummy) translation operator
   */
  template <class T>
  void translate (const edge_pair<C> &d, const T &t, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
    transform (t);
  }

  /**
   *  @brief A less operator to establish a sorting order.
   */
  bool operator< (const edge_pair<C> &b) const
  {
    if (m_symmetric != b.m_symmetric) {
      return m_symmetric < b.m_symmetric;
    }

    const edge_type &l = lesser ();
    const edge_type &g = greater ();
    const edge_type &bl = b.lesser ();
    const edge_type &bg = b.greater ();
    return l < bl || (l == bl && g < bg);
  }

  /** 
   *  @brief Equality test
   */
  bool operator== (const edge_pair<C> &b) const
  {
    if (m_symmetric != b.m_symmetric) {
      return false;
    }
    return lesser () == b.lesser () && greater () == b.greater ();
  }

  /** 
   *  @brief Inequality test
   */
  bool operator!= (const edge_pair<C> &b) const
  {
    return !operator== (b);
  }

  /**
   *  @brief A fuzzy less operator to establish a sorting order.
   */
  bool less (const edge_pair<C> &b) const
  {
    if (m_symmetric != b.m_symmetric) {
      return m_symmetric < b.m_symmetric;
    }

    const edge_type &l = lesser ();
    const edge_type &g = greater ();
    const edge_type &bl = b.lesser ();
    const edge_type &bg = b.greater ();
    return l.less (bl) || (l.equal (bl) && g.less (bg));
  }

  /**
   *  @brief Fuzzy equality test
   */
  bool equal (const edge_pair<C> &b) const
  {
    if (m_symmetric != b.m_symmetric) {
      return false;
    }
    return lesser ().equal (b.lesser ()) && greater ().equal (b.greater ());
  }

  /**
   *  @brief Fuzzy inequality test
   */
  bool not_equal (const edge_pair<C> &b) const
  {
    return !equal (b);
  }

  /**
   *  @brief Computes the distance of the edges in the edge pair
   *
   *  The distance is the minimum distance of any of the points from
   *  each edge.
   */
  distance_type distance () const
  {
    db::edge<C> e1 = first (), e2 = second ();
    if (! e1.intersect (e2)) {
      distance_type d12 = std::min (e2.euclidian_distance (e1.p1 ()), e2.euclidian_distance (e1.p2 ()));
      distance_type d21 = std::min (e1.euclidian_distance (e2.p1 ()), e1.euclidian_distance (e2.p2 ()));
      return std::min (d12, d21);
    } else {
      return 0;
    }
  }

  /**
   *  @brief A method binding of operator* (mainly for automation purposes)
   */
  edge_pair<C> scaled (double s) const
  {
    return edge_pair<C> (edge_type (first () * s), edge_type (second () * s), m_symmetric);
  }

  /**
   *  @brief Returns the moved edge pair
   *
   *  Moves the edge by the given offset and returns the 
   *  moved edge. The edge is not modified.
   *
   *  @param p The distance to move the edge.
   * 
   *  @return The moved edge.
   */
  edge_pair<C> moved (const vector<C> &p) const
  {
    edge_pair<C> b (*this);
    b.move (p);
    return b;
  }

  /**
   *  @brief Transform the edge pair.
   *
   *  Transforms the edge pair with the given transformation.
   *  Modifies the edge pair with the transformed edge.
   *  
   *  @param t The transformation to apply.
   *
   *  @return The transformed edge pair.
   */
  template <class Tr>
  edge_pair<C> &transform (const Tr &t)
  {
    *this = edge_pair<C> (t * m_first, t * m_second, m_symmetric);
    return *this;
  }

  /**
   *  @brief Transform the edge pair.
   *
   *  Transforms the edge pair with the given transformation.
   *  Does not modify the edge pair but returns the transformed edge.
   *  
   *  @param t The transformation to apply.
   *
   *  @return The transformed edge pair.
   */
  template <class Tr>
  edge_pair<typename Tr::target_coord_type> transformed (const Tr &t) const
  {
    return edge_pair<typename Tr::target_coord_type> (t * m_first, t * m_second, m_symmetric);
  }

  /**
   *  @brief Moves the edge pair.
   *
   *  Moves the edge pair by the given offset and returns the 
   *  moved edge pair. The edge pair is overwritten.
   *
   *  @param p The distance to move the edge pair.
   * 
   *  @return The moved edge pair.
   */
  edge_pair<C> &move (const vector<C> &p)
  {
    m_first.move (p);
    m_second.move (p);
    return *this;
  }

  /**
   *  @brief Sets the first edge.
   */
  void set_first (const edge_type &e) 
  {
    m_first = e;
  }

  /**
   *  @brief Sets the second edge.
   */
  void set_second (const edge_type &e) 
  {
    m_second = e;
  }

  /**
   *  @brief The first edge (non-const).
   */
  edge_type &first () 
  {
    return m_first;
  }

  /**
   *  @brief The second edge (non-const).
   */
  edge_type &second () 
  {
    return m_second;
  }

  /**
   *  @brief The first edge.
   */
  const edge_type &first () const
  {
    return m_first;
  }

  /**
   *  @brief The second edge.
   */
  const edge_type &second () const
  {
    return m_second;
  }

  /**
   *  @brief The "lesser" edge.
   *  This feature is used for comparing symmetric edge pairs.
   */
  const edge_type &lesser () const
  {
    if (m_symmetric) {
      return m_first < m_second ? m_first : m_second;
    } else {
      return m_first;
    }
  }

  /**
   *  @brief The "greater" edge.
   *  This feature is used for comparing symmetric edge pairs.
   */
  const edge_type &greater () const
  {
    if (m_symmetric) {
      return m_second < m_first ? m_first : m_second;
    } else {
      return m_second;
    }
  }

  /**
   *  @brief returns the bounding box
   */
  const box_type bbox () const
  {
    return box_type (m_first.p1 (), m_first.p2 ()) + box_type (m_second.p1 (), m_second.p2 ());
  }

  /**
   *  @brief Gets the perimeter of the edge pair
   *
   *  The perimeter is defined by then sum of the lengths of the edges ("active perimeter")
   */
  perimeter_type perimeter () const
  {
    return m_first.length () + m_second.length ();
  }

  /**
   *  @brief Gets the area of the edge pair
   *
   *  This is the area covered between the edges.
   */
  area_type area () const
  {
    vector_type v12 = m_first.p2 () - m_first.p1 ();
    vector_type v13 = m_second.p1 () - m_first.p1 ();
    vector_type v14 = m_second.p2 () - m_first.p1 ();
    area_type a = (db::vprod (v12, v13) + db::vprod (v13, v14)) / 2;
    return a < 0 ? -a : a;
  }

  /**
   *  @brief Test if the edges are orthogonal (vertical or horizontal)
   */
  bool is_ortho () const
  {
    return m_first.is_ortho () && m_second.is_ortho ();
  }

  /**
   *  @brief Conversion to a string.
   *
   *  If dbu is set, it determines the factor by which the coordinates are multiplied to render
   *  micron units. In addition, a micron format is chosen for output of these coordinates.
   */
  std::string to_string (double dbu = 0.0) const
  {
    return lesser ().to_string (dbu) + (m_symmetric ? "|" : "/") + greater ().to_string (dbu);
  }
  
  /**
   *  @brief Test whether the edges inside the edge pair are parallel
   *
   *  @return True if both edges are parallel
   */
  bool parallel () const
  {
    return m_first.parallel (m_second);
  }

  /**
   *  @brief Test whether the edges inside the edge pair are coincident
   *
   *  Such an edge pair will have an area of zero.
   *
   *  @return True if both edges are coincident
   */
  bool coincident () const
  {
    return m_first.coincident (m_second);
  }

  /**
   *  @brief Swaps the edges
   */
  void swap_edges () 
  {
    std::swap (m_first, m_second);
  }

  /**
   *  @brief Normalize the edge orientation 
   *
   *  This method modifies the orientation of the first edge such that both 
   *  edge are anti-parallel. Such edge pairs will generate polygons which are non-self overlapping.
   *  In addition, the edges are sorted such that the edges form a closed loop in clockwise
   *  direction.
   */
  edge_pair<C> &normalize () 
  {
    area_type a1 = db::vprod (m_first.p2 () - m_second.p2 (), m_first.p1 () - m_second.p1 ());
    area_type a2 = db::vprod (m_first.p1 () - m_second.p2 (), m_first.p2 () - m_second.p1 ());
    if ((a2 < 0 ? -a2 : a2) > (a1 < 0 ? -a1 : a1)) {
      m_first.swap_points ();
      std::swap (a1, a2);
    }

    if (a1 < 0) {

      m_first.swap_points ();
      m_second.swap_points ();

    } else if (a1 == 0) {

      //  fallback for zero-area edge pairs:
      if (db::sprod_sign (m_first, m_second) > 0) {
        m_first.swap_points ();
      }
      //  Note: to account for degenerate edges we do both tests:
      if (m_first.side_of (m_second.p1 ()) > 0 || m_second.side_of (m_first.p1 ()) > 0) {
        m_first.swap_points ();
        m_second.swap_points ();
      }

    }

    return *this;
  }

  /**
   *  @brief Returns the normalized edge pair
   */
  edge_pair<C> normalized () const
  {
    db::edge_pair<C> e = *this;
    e.normalize ();
    return e;
  }

  /**
   *  @brief Convert to a polygon (template)
   *
   *  The given extension is applied to start and end points as well as perpendicular.
   *  This way it is possible to map degenerated edge pairs (points, coincident etc.) 
   *  to get an area and hence they can be mapped to polygons without vanishing.
   *  This method does not automatically normalize the edge pairs but it is recommended 
   *  to normalize them before converting them to polygons.
   */
  template <class Poly>
  Poly to_polygon_generic (coord_type e) const
  {
    DEdge e1 (m_first);
    DEdge e2 (m_second);

    if (e) {

      if (! m_first.is_degenerate ()) {
        e1.extend (e);
      } 
      if (! m_second.is_degenerate ()) {
        e2.extend (e);
      }

      //  special handling for double degeneration
      if (m_first.is_degenerate () && m_second.is_degenerate ()) {

        if (m_first.p1 () == m_second.p1 ()) {
          //  single-point edge pair: create a box
          e1.extend (e);
          e2.extend (e);
          e2.swap_points ();
        } else {
          //  a single line connecting two points: modify the edges
          e1 = DEdge (m_first.p1 (), m_second.p1 ());
          e2 = DEdge (m_second.p1 (), m_first.p1 ());
        } 

      } 
        
      e1.shift (e);
      e2.shift (e);

    }

    point_type pts[4] = { point_type (e1.p1 ()), point_type (e1.p2 ()),
                          point_type (e2.p1 ()), point_type (e2.p2 ()) };
    Poly p;
    p.assign_hull (pts + 0, pts + 4);
    return p;
  }

  /**
   *  @brief Convert to a polygon
   *
   *  See \to_polygon_generic for a description of the functionality.
   */
  polygon_type to_polygon (coord_type e) const
  {
    return to_polygon_generic<polygon_type> (e);
  }

  /**
   *  @brief Convert to a simple polygon
   *
   *  See \to_polygon_generic for a description of the functionality.
   */
  simple_polygon_type to_simple_polygon (coord_type e) const
  {
    return to_polygon_generic<simple_polygon_type> (e);
  }

private:
  edge_type m_first, m_second;
  bool m_symmetric;
};

/**
 *  @brief Scaling of an edge pair
 *
 *  @param e The edge pair to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled edge pair
 */ 
template <class C>
inline edge_pair<double> 
operator* (const edge_pair<C> &e, double s)
{
  return edge_pair<double> (e.first () * s, e.second () * s);
}

/**
 *  @brief Binary * operator (transformation)
 *
 *  Transforms the edge pair with the given transformation and 
 *  returns the result.
 *
 *  @param t The transformation to apply
 *  @param e The edge pair to transform
 *  @return t * e
 */
template <class C, class Tr>
inline edge_pair<typename Tr::target_coord_type> 
operator* (const Tr &t, const edge_pair<C> &e)
{
  return e.transformed (t);
}

/**
 *  @brief Output stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const edge_pair<C> &e)
{
  return (os << e.to_string ());
}

/**
 *  @brief The standard edge pair typedef
 */
typedef edge_pair<db::Coord>  EdgePair;

/**
 *  @brief The double coordinate edge pair typedef
 */
typedef edge_pair<db::DCoord> DEdgePair;

} // namespace db

/**
 *  @brief Special extractors for the edges
 */

namespace tl 
{
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::EdgePair &b);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DEdgePair &b);

  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::EdgePair &b);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DEdgePair &b);

} // namespace tl

#endif


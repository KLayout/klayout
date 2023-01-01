
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


#ifndef HDR_dbPath
#define HDR_dbPath

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbMemStatistics.h"
#include "dbPoint.h"
#include "dbTrans.h"
#include "dbEdge.h"
#include "dbBox.h"
#include "dbPolygon.h"
#include "dbObjectTag.h"
#include "dbShapeRepository.h"
#include "dbStatic.h"
#include "tlVector.h"
#include "tlString.h"

#include <string>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cmath>

namespace db {

template <class Coord> class generic_repository;
class ArrayRepository;

/** 
 *  @brief A point iterator for paths
 *
 *  The point iterator delivers all points of the path.
 *  It is based on the random access operator of the point list.
 */
 
template <class Path, class Tr>
class path_point_iterator
{
public:
  typedef typename Path::pointlist_type pointlist_type;
  typedef typename pointlist_type::value_type point_type;
  typedef typename point_type::coord_type coord_type;
  typedef void pointer;           //  no operator->
  typedef point_type reference;   //  operator* returns a value
  typedef point_type value_type;
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef size_t difference_type;
  typedef Tr trans_type;

  /** 
   *  @brief The default constructor 
   */
  path_point_iterator ()
    : mp_pointlist (0), m_index (0)
  {
    //  .. nothing yet .. 
  }
  
  /** 
   *  @brief The standard constructor 
   */
  path_point_iterator (const pointlist_type &pointlist, size_t n)
    : mp_pointlist (&pointlist), m_index (n)
  {
    //  .. nothing yet .. 
  }
  
  /** 
   *  @brief The standard constructor with a transformation
   */
  template <class T>
  path_point_iterator (const path_point_iterator<Path, T> &d, const trans_type &trans) 
    : mp_pointlist (d.mp_pointlist), m_index (d.m_index), m_trans (trans)
  {
    //  .. nothing yet .. 
  }
  
  /**
   *  @brief Sorting order
   */
  bool operator< (const path_point_iterator &d) const
  {
    return m_index < d.m_index;
  }

  /**
   *  @brief Equality test
   */
  bool operator== (const path_point_iterator &d) const
  {
    return m_index == d.m_index;
  }

  /**
   *  @brief Inequality test
   */
  bool operator!= (const path_point_iterator &d) const
  {
    return m_index != d.m_index;
  }

  /**
   *  @brief Point access
   */
  point_type operator* () const 
  {
    return m_trans ((*mp_pointlist) [m_index]);
  }

  /**
   *  @brief Addition of distances
   */
  path_point_iterator operator+ (difference_type d) const
  {
    return path_point_iterator (mp_pointlist, m_index + d);
  }

  /**
   *  @brief Addition of distances
   */
  path_point_iterator &operator+= (difference_type d) 
  {
    m_index += d;
    return *this;
  }

  /**
   *  @brief Subtraction of distances
   */
  path_point_iterator operator- (difference_type d) const
  {
    return path_point_iterator (mp_pointlist, m_index - d);
  }

  /**
   *  @brief Subtraction of distances
   */
  path_point_iterator &operator-= (difference_type d) 
  {
    m_index -= d;
    return *this;
  }

  /**
   *  @brief Subtraction of iterators
   */
  difference_type operator- (const path_point_iterator &d) const
  {
    return m_index - d.m_index;
  }

  /**
   *  @brief Increment operator
   */
  path_point_iterator &operator++ () 
  {
    ++m_index;
    return *this;
  }

  /**
   *  @brief Decrement operator
   */
  path_point_iterator &operator-- () 
  {
    --m_index;
    return *this;
  }

private:
  template <class Pl, class T> friend class path_point_iterator;

  const pointlist_type *mp_pointlist;
  size_t m_index;
  trans_type m_trans;
};

/** 
 *  @brief A path class
 *
 *  A path consists of an sequence of line segments 
 *  and a width.
 *  The path can be converted to a polygon.
 */

//  NOTE: we do explicit instantiation, so the exposure is declared
//  as DB_PUBLIC - as if it wasn't a template
template <class C>
class DB_PUBLIC path
{
public:
  typedef C coord_type;
  typedef db::simple_trans<coord_type> trans_type;
  typedef db::point<coord_type> point_type;
  typedef db::vector<coord_type> vector_type;
  typedef db::box<coord_type> box_type;
  typedef db::coord_traits<coord_type> coord_traits;
  typedef typename coord_traits::distance_type distance_type; 
  typedef typename coord_traits::perimeter_type perimeter_type; 
  typedef typename coord_traits::area_type area_type; 
  typedef object_tag< path<C> > tag;
  typedef tl::vector<point_type> pointlist_type;
  typedef db::path_point_iterator<path <C>, db::unit_trans<C> > iterator;

  /**
   *  @brief The default constructor.
   *  
   *  The default constructor creates a empty path with a width
   *  a zero.
   */
  path ()
    : m_width (0), m_bgn_ext (0), m_end_ext (0)
  {
    // .. nothing yet ..
  }

  /** 
   *  @brief Standard constructor
   *
   *  The standard ctor creates a path from a sequence of points
   *  and a width.
   */
  template <class Iter>
  path (Iter from, Iter to, coord_type width, coord_type bgn_ext = 0, coord_type end_ext = 0, bool round = false)
    : m_width (round ? -width : width), m_bgn_ext (bgn_ext), m_end_ext (end_ext)
  {
    m_points.insert (m_points.end (), from, to);
  }

  /**
   *  @brief Constructor that provides conversion and transformation
   *
   *  This constructor allows converting a path from any type to this one.
   *  In addition, transformation operators can be provided that specify
   *  how to transform points and lengths.
   *
   *  @param p The source path
   *  @param tp The point transformation operator
   *  @param tl The length transformation operator
   */
  template <class D, class TP, class TL>
  path (const path<D> &p, const TP &tp, const TL &tl)
    : m_width (p.m_width < 0 ? -tl (-p.m_width) : tl (p.m_width)),
      m_bgn_ext (p.m_bgn_ext < 0 ? -tl (-p.m_bgn_ext) : tl (p.m_bgn_ext)),
      m_end_ext (p.m_end_ext < 0 ? -tl (-p.m_end_ext) : tl (p.m_end_ext))
  {
    m_points.reserve (p.m_points.size ());
    for (typename path<D>::pointlist_type::const_iterator pp = p.m_points.begin (); pp != p.m_points.end (); ++pp) {
      m_points.push_back (tp (*pp));
    }
  }

  /**
   *  @brief Constructor that provides conversion from another coordinate type
   *
   *  This constructor allows converting a path from any type to this one.
   *
   *  @param p The source path
   */
  template <class D>
  explicit path (const path<D> &p)
    : m_width (p.m_width < 0 ? -db::coord_converter<C, D> () (-p.m_width) : db::coord_converter<C, D> () (p.m_width)),
      m_bgn_ext (db::coord_converter<C, D> () (p.m_bgn_ext)),
      m_end_ext (db::coord_converter<C, D> () (p.m_end_ext))
  {
    db::point_coord_converter<C, D> tp;
    m_points.reserve (p.m_points.size ());
    for (typename path<D>::pointlist_type::const_iterator pp = p.m_points.begin (); pp != p.m_points.end (); ++pp) {
      m_points.push_back (tp (*pp));
    }
  }

  /**
   *  @brief The (dummy) translation operator
   */
  void translate (const path<C> &d, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
  }

  /**
   *  @brief The (dummy) translation operator with transformation
   */
  template <class T>
  void translate (const path<C> &d, const T &t, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
    transform (t);
  }

  /**
   *  @brief A less operator to establish a sorting order.
   */
  bool operator< (const path<C> &b) const
  {
    return (m_width < b.m_width || (m_width == b.m_width &&
           (m_bgn_ext < b.m_bgn_ext || (m_bgn_ext == b.m_bgn_ext &&  
           (m_end_ext < b.m_end_ext || (m_end_ext == b.m_end_ext &&  
           (m_points < b.m_points)))))));
  }

  /** 
   *  @brief Equality test
   */
  bool operator== (const path<C> &b) const
  {
    return m_width == b.m_width && m_bgn_ext == b.m_bgn_ext && 
           m_end_ext == b.m_end_ext && m_points == b.m_points;
  }

  /** 
   *  @brief Inequality test
   */
  bool operator!= (const path<C> &b) const
  {
    return !operator== (b);
  }

  /**
   *  @brief A fuzzy operator to establish a sorting order.
   */
  bool less (const path<C> &b) const
  {
    if (! coord_traits::equal (m_width, b.m_width)) {
      return m_width < b.m_width;
    }
    if (! coord_traits::equal (m_bgn_ext, b.m_bgn_ext)) {
      return m_bgn_ext < b.m_bgn_ext;
    }
    if (! coord_traits::equal (m_end_ext, b.m_end_ext)) {
      return m_end_ext < b.m_end_ext;
    }
    return db::less (m_points, b.m_points);
  }

  /**
   *  @brief Fuzzy equality test
   */
  bool equal (const path<C> &b) const
  {
    if (! coord_traits::equal (m_width, b.m_width)) {
      return false;
    }
    if (! coord_traits::equal (m_bgn_ext, b.m_bgn_ext)) {
      return false;
    }
    if (! coord_traits::equal (m_end_ext, b.m_end_ext)) {
      return false;
    }
    return db::equal (m_points, b.m_points);
  }

  /**
   *  @brief Fuzzy inequality test
   */
  bool not_equal (const path<C> &b) const
  {
    return !equal (b);
  }

  /**
   *  @brief Set the width
   */
  void width (coord_type w)
  {
    if (w != width ()) {
      m_bbox = box_type ();
      m_width = m_width < 0 ? -w : w;
    }
  }

  /**
   *  @brief Get the width
   */
  coord_type width () const
  {
    return m_width < 0 ? -m_width : m_width;
  }

  /**
   *  @brief Make it a round path
   */
  void round (bool r)
  {
    if (r != round ()) {
      m_bbox = box_type ();
      coord_type w = (m_width < 0 ? -m_width : m_width);
      m_width = r ? -w : w;
    }
  }

  /**
   *  @brief Check if it is a round path
   */
  bool round () const
  {
    return m_width < 0;
  }

  /**
   *  @brief Set the begin extension
   */
  void bgn_ext (coord_type bgn)
  {
    if (m_bgn_ext != bgn) {
      m_bbox = box_type ();
      m_bgn_ext = bgn;
    }
  }

  /**
   *  @brief Get the begin extension
   */
  coord_type bgn_ext () const
  {
    return m_bgn_ext;
  }

  /**
   *  @brief Set the end extension
   */
  void end_ext (coord_type end)
  {
    if (m_end_ext != end) {
      m_bbox = box_type ();
      m_end_ext = end;
    }
  }

  /**
   *  @brief Get the end extension
   */
  coord_type end_ext () const
  {
    return m_end_ext;
  }

  /**
   *  @brief Set the extensions
   */
  void extensions (coord_type bgn, coord_type end)
  {
    if (m_bgn_ext != bgn || m_end_ext != end) {
      m_bbox = box_type ();
      m_bgn_ext = bgn;
      m_end_ext = end;
    }
  }

  /**
   *  @brief Get the extensions
   */
  std::pair<coord_type, coord_type> extensions () const
  {
    return std::make_pair (m_bgn_ext, m_end_ext);
  }

  /**
   *  @brief assign a sequence [from,to) of points
   *
   *  @param from The iterator pointing to the first point
   *  @param to The past-end iterator
   */
  template <class Iter>
  void assign (Iter from, Iter to)
  {
    m_bbox = box_type ();
    m_points.assign (from, to);
  }

  /**
   *  @brief assign a sequence [from,to) of points with transformation
   *
   *  @param from The iterator pointing to the first point
   *  @param to The past-end iterator
   *  @param t The transformation to apply
   */
  template <class Iter, class Op>
  void assign (Iter from, Iter to, Op t)
  {
    m_bbox = box_type ();
    m_points.clear ();
    m_points.reserve (std::distance (from, to));

    for (Iter p = from; p != to; ++p) {
      m_points.push_back (t (*p));
    }
  }

  /**
   *  @brief Access to the points through the iterator
   *
   *  @return A iterator pointing to the first point
   */
  iterator begin () const
  {
    return iterator (m_points, 0);
  }
  
  /**
   *  @brief Access to the points through the iterator
   *
   *  @return A iterator pointing to the past-end point
   */
  iterator end () const
  {
    return iterator (m_points, m_points.size ());
  }

  /**
   *  @brief Get the number of points
   */
  size_t points () const
  {
    return m_points.size ();
  }

  /**
   *  @brief Transform the path.
   *
   *  Transforms the path with the given transformation.
   *  Modifies the path with the transformed path.
   *  
   *  @param t The transformation to apply.
   *
   *  @return The transformed path.
   */
  template <class Tr>
  path<C> &transform (const Tr &t)
  {
    m_bbox = box_type ();
    if (m_width < 0) {
      m_width = -coord_type (t.ctrans (-m_width));
    } else {
      m_width = t.ctrans (m_width);
    }
    if (m_bgn_ext < 0) {
      m_bgn_ext = -coord_type (t.ctrans (-m_bgn_ext));
    } else {
      m_bgn_ext = t.ctrans (m_bgn_ext);
    }
    if (m_end_ext < 0) {
      m_end_ext = -coord_type (t.ctrans (-m_end_ext));
    } else {
      m_end_ext = t.ctrans (m_end_ext);
    }
    for (typename pointlist_type::iterator p = m_points.begin (); p != m_points.end (); ++p) {
      p->transform (t);
    }
    
    return *this;
  }

  /**
   *  @brief Transform the path.
   *
   *  Transforms the path with the given transformation.
   *  Does not modify the path but returns the transformed path.
   *  
   *  @param t The transformation to apply.
   *
   *  @return The transformed path.
   */
  template <class Tr>
  path<typename Tr::target_coord_type> transformed (const Tr &t) const
  {
    typedef typename Tr::target_coord_type target_coord_type;
    path<target_coord_type> res;

    if (m_width < 0) {
      res.m_width = -target_coord_type (t.ctrans (-m_width));
    } else {
      res.m_width = t.ctrans (m_width);
    }
    if (m_bgn_ext < 0) {
      res.m_bgn_ext = -target_coord_type (t.ctrans (-m_bgn_ext));
    } else {
      res.m_bgn_ext = t.ctrans (m_bgn_ext);
    }
    if (m_end_ext < 0) {
      res.m_end_ext = -target_coord_type (t.ctrans (-m_end_ext));
    } else {
      res.m_end_ext = t.ctrans (m_end_ext);
    }
    res.m_points.reserve (m_points.size ());

    for (typename pointlist_type::const_iterator p = m_points.begin (); p != m_points.end (); ++p) {
      res.m_points.push_back (t (*p));
    }

    return res;
  }

  /**
   *  @brief Returns the moved path
   *
   *  Moves the path by the given offset and returns the 
   *  moved path. The path is not modified.
   *
   *  @param p The distance to move the path.
   * 
   *  @return The moved path.
   */
  path<C> moved (const vector<C> &p) const
  {
    path<C> b (*this);
    b.move (p);
    return b;
  }

  /**
   *  @brief Moves the path.
   *
   *  Moves the path by the given offset and returns the 
   *  moved path. The path is overwritten.
   *
   *  @param p The distance to move the path.
   * 
   *  @return The moved path.
   */
  path<C> &move (const vector<C> &d)
  {
    for (typename pointlist_type::iterator p = m_points.begin (); p != m_points.end (); ++p) {
      *p += d;
    }
    if (! m_bbox.empty ()) {
      m_bbox.move (d);
    }
    return *this;
  }

  /** 
   *  @brief The length of the path
   */
  distance_type length () const;

  /** 
   *  @brief The perimeter of the path
   *
   *  This method returns the approximate perimeter of the path. 
   *  It is basically two times the length plus width. Extensions are taken into account but 
   *  the precise effect of the corner treatment is not.
   */
  perimeter_type perimeter () const;

  /** 
   *  @brief The area of the path
   *
   *  This method returns the approximate area of the path. 
   *  It is basically the length times the width. Extensions are taken into account but 
   *  the precise effect of the corner treatment is not.
   */
  area_type area () const;

  /**
   *  @brief Returns an approximation of the bounding box of the path
   *
   *  Simple version:
   *  The box is the pointlist bbox enlarged by 2 times the width
   *  which is an approximation of the true bounding box being
   *  slightly too large.
   *
   *  Complex version:
   *  Compute the bbox exactly.
   */
  box_type box () const
  {
    update_bbox ();
    return m_bbox;
  }

  /**
   *  @brief Clears the path
   *
   *  Clears the point list and resets the width to 0.
   */
  void clear ()
  {
    m_points.clear ();
    m_bbox = box_type ();
    m_width = 0;
  }

  /**
   *  @brief The string conversion function
   */
  std::string to_string () const;

  /**
   *  @brief The conversion to a hull
   *
   *  The path is converted to a pointlist describing the
   *  hull polygon. The resulting pointlist is not guaranteed not to
   *  be self-overlapping. 
   */
  template <class PointList>
  void hull (PointList &pts, int semi_circ_pts = db::num_circle_points () / 2) const
  {
    pts.reserve (m_points.size () * 2);  //  minimum number of points required

    //  use the real points
    pointlist_type tmp_points;
    real_points (tmp_points);

    create_shifted_points (m_bgn_ext, m_end_ext, width (), true, tmp_points.begin (), tmp_points.end (), round () ? semi_circ_pts : 2, std::back_inserter (pts));
    create_shifted_points (m_end_ext, m_bgn_ext, width (), false, tmp_points.rbegin (), tmp_points.rend (), round () ? semi_circ_pts : 2, std::back_inserter (pts));
  }

  /**
   *  @brief The conversion to a hull
   *
   *  The path is converted to a pointlist describing the
   *  hull polygon. The resulting pointlist is not guaranteed not to
   *  be self-overlapping. 
   *
   *  This version allows one to override the left and right side's width hence creating
   *  asymmetric paths. dleft is the shift to the left (as seen in the direction of the
   *  path) and dright the shift to the right. The default path is created if dleft+dright=width.
   */
  template <class PointList>
  void hull (PointList &pts, C dleft, C dright, int semi_circ_pts = db::num_circle_points () / 2) const
  {
    pts.reserve (m_points.size () * 2);  //  minimum number of points required

    //  use the real points
    pointlist_type tmp_points;
    real_points (tmp_points);

    create_shifted_points (m_bgn_ext, m_end_ext, dleft * 2, true, tmp_points.begin (), tmp_points.end (), round () ? semi_circ_pts : 2, std::back_inserter (pts));
    create_shifted_points (m_end_ext, m_bgn_ext, dright * 2, false, tmp_points.rbegin (), tmp_points.rend (), round () ? semi_circ_pts : 2, std::back_inserter (pts));
  }

  /**
   *  @brief Convert a path to a polygon
   *
   *  This function basically uses the hull method with the same
   *  constraints.
   *  Because of the overhead of creating a canonical representation
   *  and copying of the polygon, this method is somewhat slower than 
   *  the hull method.
   */
  db::polygon<C> polygon () const
  {
    pointlist_type pts;
    hull (pts);

    db::polygon<C> poly;
    poly.assign_hull (pts.begin (), pts.end (), false /*don't compress*/);
    return poly;
  }

  /**
   *  @brief Convert a path to a simple polygon
   *
   *  This function basically uses the hull method with the same
   *  constraints.
   *  Because of the overhead of creating a canonical representation
   *  and copying of the polygon, this method is somewhat slower than 
   *  the hull method.
   */
  db::simple_polygon<C> simple_polygon () const
  {
    pointlist_type pts;
    hull (pts);

    db::simple_polygon<C> poly;
    poly.assign_hull (pts.begin (), pts.end (), false /*don't compress*/);
    return poly;
  }

  /**
   *  @brief Swap the path with another one
   * 
   *  The global std::swap function injected into the std namespace
   *  is redirected to this implementation.
   */
  void swap (path<C> &d)
  {
    m_points.swap (d.m_points);
    std::swap (m_width, d.m_width);
    std::swap (m_bgn_ext, d.m_bgn_ext);
    std::swap (m_end_ext, d.m_end_ext);
    std::swap (m_bbox, d.m_bbox);
  }

  /**
   *  @brief Reduce the path
   *
   *  Reduction of a path normalizes the path by extracting
   *  a suitable transformation and placing the path in a unique
   *  way.
   *
   *  @return The transformation that must be applied to render the original path
   */
  void reduce (simple_trans<coord_type> &tr)
  {
    if (m_points.size () < 1) {
      tr = trans_type ();
    } else {
      vector<C> d (m_points [0] - point_type ());
      move (-d);
      tr = trans_type (trans_type::r0, d);
    }
  }

  /**
   *  @brief Reduce the path
   *
   *  Reduction of a path normalizes the path by extracting
   *  a suitable transformation and placing the path in a unique
   *  way.
   *
   *  @return The transformation that must be applied to render the original path
   */
  void reduce (disp_trans<coord_type> &tr)
  {
    if (m_points.size () < 1) {
      tr = disp_trans<coord_type> ();
    } else {
      vector_type d (m_points [0]);
      move (-d);
      tr = disp_trans<coord_type> (d);
    }
  }

  /**
   *  @brief Reduce the path for unit transformation references
   *
   *  Does not do any reduction since no transformation can be provided.
   *
   *  @return A unit transformation
   */
  void reduce (unit_trans<C> &)
  {
    //  .. nothing ..
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (!no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
    db::mem_stat (stat, purpose, cat, m_points, true, (void *) this);
  }

private:
  template <class X> friend class path;

  coord_type m_width;
  coord_type m_bgn_ext, m_end_ext;
  pointlist_type m_points;
  mutable box_type m_bbox;

  /**
   *  @brief Updates the bounding box
   */
  void update_bbox () const;

  /**
   *  @brief Gets the real points - without redundant ones
   */
  void real_points (pointlist_type &real_pts) const;

  /**
   *  @brief Create a sequence of points shifted by a certain distance
   *  This will render half of the path's outline.
   */
  template <class Iter, class Inserter>
  void create_shifted_points (coord_type start, coord_type end, coord_type width, bool forward, Iter from, Iter to, int ncircle, Inserter pts) const;
};

/**
 *  @brief Collect memory statistics
 */
template <class C>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const path<C> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief Binary * operator (transformation)
 *
 *  Transforms the path with the given transformation and 
 *  returns the result.
 *
 *  @param t The transformation to apply
 *  @param p The path to transform
 *  @return t * p
 */
template <class Tr>
inline path<typename Tr::target_coord_type> 
operator* (const Tr &t, const path<typename Tr::coord_type> &p)
{
  return p.transformed (t);
}

/**
 *  @brief Binary * operator (scaling)
 *
 *  @param p The path to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled path
 */
template <class C>
inline path<double>
operator* (const path<C> &p, double s)
{
  db::complex_trans<C, double> ct (s);
  return ct * p;
}

/**
 *  @brief Output stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const path<C> &p)
{
  return (os << p.to_string ());
}

/**
 *  @brief The standard path typedef
 */
typedef path<db::Coord> Path;

/**
 *  @brief The double coordinate path typedef
 */
typedef path<db::DCoord> DPath;

/** 
 *  @brief A path reference
 *
 *  A path reference is basically a proxy to a path and
 *  is used to implement path references with a repository.
 */

template <class Path, class Trans>
struct path_ref
  : public shape_ref<Path, Trans>
{
  typedef typename Path::coord_type coord_type;
  typedef typename Path::point_type point_type;
  typedef typename Path::box_type box_type;
  typedef Trans trans_type;
  typedef Path path_type;
  typedef db::path_point_iterator<Path, trans_type> iterator;
  typedef db::generic_repository<coord_type> repository_type;
  typedef db::object_tag< path_ref<Path, Trans> > tag;

  /**
   *  @brief The default constructor.
   *  
   *  The default constructor creates a invalid path reference
   */
  path_ref ()
    : shape_ref<Path, Trans> ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor creating a reference from an actual path
   */
  path_ref (const path_type &p, repository_type &rep)
    : shape_ref<Path, Trans> (p, rep)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor creating a reference from an path pointer and transformation
   *
   *  The path pointer passed is assumed to reside in a proper repository.
   */
  template <class TransIn>
  path_ref (const path_type *p, const TransIn &t)
    : shape_ref<Path, Trans> (p, Trans (t))
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The translation constructor.
   *  
   *  This constructor allows one to copy a path reference from one
   *  repository to another
   */
  path_ref (const path_ref &ref, repository_type &rep)
    : shape_ref<Path, Trans> (ref, rep)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The transformation translation constructor
   *  
   *  This constructor allows one to copy a path reference with a certain transformation
   *  to one with another transformation
   */
  template <class TransIn>
  path_ref (const path_ref<Path, TransIn> &ref)
    : shape_ref<Path, Trans> (ref.ptr (), Trans (ref.trans ()))
  {
    // .. nothing yet ..
  }

  /** 
   *  @brief The point iterator begin function
   *
   *  The point iterator delivers all points of the path.
   *
   *  @return the begin value of the iterator
   */
  iterator begin () const 
  { 
    return iterator (this->obj ().begin (), this->trans ());
  }
  
  /** 
   *  @brief The point iterator begin function
   *
   *  The point iterator delivers all points of the path.
   *
   *  @return the end value of the iterator
   */
  iterator end () const 
  { 
    return iterator (this->obj ().end (), this->trans ());
  }
  
  /** 
   *  @brief Return the transformed object
   * 
   *  This version does not change the object and is const.
   */
  template <class TargetTrans>
  path_ref<Path, TargetTrans> transformed (const TargetTrans &t) const
  {
    path_ref<Path, TargetTrans> pref (*this);
    pref.transform (t);
    return pref;
  }
};

/**
 *  @brief Binary * operator (transformation)
 *
 *  Transforms the path reference with the given transformation and 
 *  returns the result.
 *
 *  @param t The transformation to apply
 *  @param p The path reference to transform
 *  @return t * p
 */
template <class Path, class Tr, class TargetTr>
inline path_ref<Path, TargetTr>
operator* (const TargetTr &t, const path_ref<Path, Tr> &p)
{
  return p.transformed (t);
}

/**
 *  @brief The path reference typedef
 */
typedef path_ref<Path, Disp> PathRef;

/**
 *  @brief The path reference typedef for double coordinates
 */
typedef path_ref<DPath, DDisp> DPathRef;

/**
 *  @brief The path reference (without transformation) typedef
 */
typedef path_ref<Path, UnitTrans> PathPtr;

/**
 *  @brief The path reference (without transformation) typedef for double coordinates
 */
typedef path_ref<DPath, DUnitTrans> DPathPtr;

/**
 *  @brief Rounds the path by smoothing the corners with a circle approximation
 *  @param input The input path
 *  @param rad The radius applied to all corners
 *  @param npoints The number of points per full circle used for the circle approximation
 *  @param accuracy The internal numerical accuracy. This value should be half the database unit.
 *  @return The new path with the corners replaced by an arc interpolation
 */
DB_PUBLIC DPath round_path_corners (const DPath &input, double rad, int npoints, double accuracy);

/**
 *  @brief Rounds the path by smoothing the corners with a circle approximation
 *  @param input The input path
 *  @param rad The radius applied to all corners
 *  @param npoints The number of points per full circle used for the circle approximation
 *  @param accuracy The internal numerical accuracy. This value should be half the database unit.
 *  @return The new path with the corners replaced by an arc interpolation
 */
Path round_path_corners (const Path &input, int rad, int npoints);

} // namespace db

namespace tl 
{
  /**
   *  @brief Special extractors for the paths
   */
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Path &p);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DPath &p);

  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Path &p);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DPath &p);

} // namespace tl

namespace std
{

//  injecting a global std::swap for polygons into the 
//  std namespace
template <class C>
void swap (db::path<C> &a, db::path<C> &b)
{
  a.swap (b);
}

} // namespace std


#endif


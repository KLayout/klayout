
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



#ifndef HDR_dbPolygon
#define HDR_dbPolygon

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbMemStatistics.h"
#include "dbPoint.h"
#include "dbTrans.h"
#include "dbEdge.h"
#include "dbBox.h"
#include "dbObjectTag.h"
#include "dbShapeRepository.h"
#include "tlTypeTraits.h"
#include "tlVector.h"
#include "tlAlgorithm.h"
#include "tlAssert.h"

#include <cstddef>
#include <cmath>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>

namespace db {

template <class Coord> class generic_repository;
class ArrayRepository;

template <class Contour, class Tr> class polygon_contour_iterator;

// define the default compression mode:
// double coordinate polygons are not compressed - this is not beneficial

template<class X> 
inline bool default_compression () 
{
  return true;
}

template<> 
inline bool default_compression<db::Coord> () 
{
  return true;
}

template<> 
inline bool default_compression<db::DCoord> () 
{
  return false;
}

/**
 *  @brief A "closed" contour type 
 *
 *  A contour is a set of points that form a closed loop.
 *  Contours are stored "normalized", that is with the 
 *  "smallest" point (as determined by the operator<) first
 *  and a clockwise or counter-clockwise orientation for
 *  hull and holes respectively.
 */

//  NOTE: we do explicit instantiation, so the exposure is declared
//  as DB_PUBLIC - as if it wasn't a template
template <class C>
class DB_PUBLIC polygon_contour
{
public:
  typedef C coord_type;
  typedef size_t size_type;
  typedef db::coord_traits<coord_type> coord_traits;
  typedef typename coord_traits::distance_type distance_type; 
  typedef typename coord_traits::perimeter_type perimeter_type; 
  typedef typename coord_traits::area_type area_type; 
  typedef db::point<C> point_type;
  typedef db::vector<C> vector_type;
  typedef point_type value_type;
  typedef db::box<C> box_type;
  typedef db::simple_trans<C> trans_type;
  typedef tl::vector<point_type> container_type;
  typedef typename container_type::const_iterator const_iterator;
  typedef polygon_contour_iterator<polygon_contour, db::unit_trans<C> > simple_iterator;

private:
  /**
   *  @brief A helper predicate function that returns true if p1-p2 is colinear with p2-p3
   */
  static 
  bool is_colinear (const point_type &p1, const point_type &p2, const point_type &p3, bool remove_reflected)
  {
    if (db::coord_traits<C>::vprod_sign (p1.x (), p1.y (), p3.x (), p3.y (), p2.x (), p2.y ()) == 0) {
      return remove_reflected || (db::coord_traits<C>::sprod_sign (p1.x (), p1.y (), p3.x (), p3.y (), p2.x (), p2.y ()) < 0);
    } else {
      return false;
    }
  }

public:
  /**
   *  @brief Default ctor
   *
   *  This ctor creates an empty contour.
   */
  polygon_contour ()
    : mp_points (0), m_size (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Copy ctor
   */
  polygon_contour (const polygon_contour &d)
    : m_size (d.m_size)
  {
    if (d.mp_points == 0) {
      mp_points = 0;
    } else {
      point_type *p = new point_type [m_size];
      point_type *pp = (point_type *) ((size_t) d.mp_points & ~3);
      mp_points = (point_type *)((size_t) p | ((size_t) d.mp_points & 3));
      for (unsigned int i = 0; i < m_size; ++i) {
        p[i] = pp[i];
      }
    }
  }

  /**
   *  @brief Destructor
   */
  ~polygon_contour ()
  {
    release ();
  }

  /**
   *  @brief Assignment
   */
  polygon_contour &operator= (const polygon_contour &d)
  {
    if (&d != this) {
      release ();
      new (this) polygon_contour (d);
    }
    return *this;
  }

  /**
   *  @brief Contour created from a sequence
   *
   *  This ctor creates a contour from a given sequence (from,to].
   *  The contour is stored "normalized". 
   *
   *  @param from Begin of the sequence
   *  @param to End of the sequence
   *  @param hole true, if a hole is constructed, false for a hull
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected true, to remove reflecting spikes if compress is true
   */
  template <class Iter>
  polygon_contour (Iter from, Iter to, bool hole, bool compress = default_compression<C> (), bool normalize = true, bool remove_reflected = false)
    : mp_points (0), m_size (0)
  {
    assign (from, to, hole, compress, normalize, remove_reflected);
  }

  /**
   *  @brief Contour created from a sequence with transformation
   *
   *  This ctor creates a contour from a given sequence (from,to], each point.
   *  transformed with the transformation "tr".
   *  The contour is stored "normalized". 
   *
   *  @param from Begin of the sequence
   *  @param to End of the sequence
   *  @param tr The transformation to apply
   *  @param hole true, if a hole is constructed, false for a hull
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected true, to remove reflecting spikes if compress is true
   */
  template <class Iter, class Trans>
  polygon_contour (Iter from, Iter to, Trans tr, bool hole, bool compress = default_compression<C> (), bool normalize = true, bool remove_reflected = false)
    : mp_points (0), m_size (0)
  {
    assign (from, to, tr, hole, compress, normalize, remove_reflected);
  }

  /**
   *  @brief Fill the contour from a sequence 
   *
   *  This ctor fills the contour with a given sequence (from,to].
   *  The contour is stored "normalized". 
   *
   *  @param from Begin of the sequence
   *  @param to End of the sequence
   *  @param hole true, if a hole is constructed, false for a hull
   *  @param compress true, if the sequence shall be compressed (colinear points removed)
   *  @param remove_reflected true, to remove reflecting spikes if compress is true
   */
  template <class Iter>
  void assign (Iter from, Iter to, bool hole, bool compress = default_compression<C> (), bool normalize = true, bool remove_reflected = false)
  {
    assign (from, to, db::unit_trans<C> (), hole, compress, normalize, remove_reflected);
  }

  /**
   *  @brief Fill the contour from a sequence with transformation
   *
   *  This ctor fills the contour with a given sequence (from,to], each point.
   *  transformed with the transformation "tr".
   *  The contour is stored "normalized". 
   *
   *  @param from Begin of the sequence
   *  @param to End of the sequence
   *  @param tr The transformation to apply
   *  @param hole true, if a hole is constructed, false for a hull
   *  @param compress true, if the sequence shall be compressed (colinear points removed)
   *  @param normalize true, if the sequence shall be normalized (oriented)
   *  @param remove_reflected true, to remove reflecting spikes if compress is true
   */
  template <class Iter, class Trans>
  void assign (Iter from, Iter to, Trans tr, bool hole, bool compress = default_compression<C> (), bool normalize = true, bool remove_reflected = false)
  {
    if (compress && remove_reflected) {

      //  with less than 3 points there is nothing to do at all ..
      if (std::distance (from, to) < 3) {
        release ();
        return;
      }

      //  the remove_reflected case is somewhat more complicated: it may happen that bends vanish because the 
      //  next edge is member of a spike edge pair. Thus the simple single-pass approach no longer holds.
      //  We therefore need to create an intermediate buffer that is used to eliminate redundant points successively.

      std::vector<point_type> points;
      points.reserve (std::distance (from, to));
      for (Iter i = from; i != to; ++i) {
        points.push_back (point_type (tr (*i)));
      }

      bool any_skipped = true;
      while (any_skipped) {

        typename std::vector<point_type>::iterator w = points.begin ();

        typename std::vector<point_type>::const_iterator p = points.begin ();

        point_type plast (points.back ());
        point_type pcurr (points.front ());
        point_type pnext;

        any_skipped = false;

        do {

          if (++p == points.end ()) {
            p = points.begin ();
          }

          pnext = *p;

          if (pcurr == plast || pcurr == pnext || is_colinear (plast, pcurr, pnext, remove_reflected)) {
            //  skip this point ..
            any_skipped = true;
          } else {
            //  we have a point to consider
            *w++ = pcurr;
            plast = pcurr;
          }

          pcurr = pnext;
         
        } while (p != points.begin ());

        points.erase (w, points.end ());

        //  if not enough points remain, stop here.
        if (points.size () < 3) {
          release ();
          return;
        }

      } 

      //  Do the normal assignment now.
      assign (points.begin (), points.end (), db::unit_trans<C> (), hole, compress, normalize, false);

    } else if (! compress) {

      release ();

      //  on empty source there is nothing to do
      if (from == to) {
        return;
      }

      //  count distinct points, determine minimum and if the contour is manhattan
      size_type n = 0;

      //  count distinct points and determine minimum
      Iter p = from;

      bool min_set = false;
      point_type pmin;
      Iter min = p;

      while (p != to) {

        //  we have a point to consider
        ++n; 

        point_type pcurr = point_type (tr (*p));

        //  determine min point and corresponding iterator
        if (! min_set || pcurr < pmin) {
          pmin = pcurr;
          min = p;
          min_set = true;
        }

        ++p;

      }

      point_type *pts;

      m_size = n;
      pts = new point_type [m_size];

      //  copy distinct points now
      p = min;
      for (n = 0; n < m_size; ++n) {
        pts [n] = point_type (tr (*p));
        if (++p == to) {
          p = from;
        }
      } 

      //  normalize the orientation if required
      if (normalize) {

        area_type a = 0;

        point_type pl = pts[m_size - 1];
        const point_type *p = pts;
        for (size_type i = 0; i < m_size; ++i, ++p) {
          a += vprod (pl - point_type (), *p - point_type ());
          pl = *p;
        }

        bool clockwise = (a < 0);

        if ((! clockwise) != hole) {
          std::reverse (pts + 1, pts + n);
        }

      }

      //  and store the pointer along with the hole flag
      tl_assert (((size_t) pts & 3) == 0);
      mp_points = (point_type *) ((size_t) pts | (hole ? 2 : 0)); 

    } else {

      release ();

      //  with less than 3 points there is nothing to do at all ..
      if (std::distance (from, to) < 3) {
        return;
      }

      //  count distinct points, determine minimum and if the contour is manhattan
      bool ortho = normalize; // don't compress manhattan sequences if not normalizing - normalisation is a prerequisite for compression! 
      size_type n = 0;

      //  count distinct points and determine minimum
      Iter p = from;
      point_type plast (tr (*p++));

      Iter pp = p;
      point_type pcurr (tr (*p++));
      if (pcurr.equal (plast)) {
        do {
          pp = p;
          pcurr = point_type (tr (*p++));
        } while (pcurr.equal (plast) && p != to);
        if (p == to) {
          return;
        }
      }

      point_type pnext;

      bool min_set = false;
      bool one_round = false;
      point_type pmin;
      Iter min = p;
      Iter pstop = to;

      do {

        pnext = point_type (tr (*p));

        if (pcurr.equal (plast) || pcurr.equal (pnext) || is_colinear (plast, pcurr, pnext, remove_reflected)) {

          //  skip this point ..

        } else {

          if (one_round) {
            if (pp == pstop) {
              //  we found the closing point - stop now.
              break;
            }
          } else if (pstop == to) {
            pstop = pp;
          }

          //  we have a point to consider
          ++n; 

          //  test, if the contour is manhattan
          //  there is a strict criterion what is "manhattan": only such segment pairs 
          //  that really bend by 90 degree are recognized as manhattan.
          if (ortho && !((  coord_traits::equals (plast.x (), pcurr.x ()) && ! coord_traits::equals (plast.y (), pcurr.y ()) && 
                          ! coord_traits::equals (pcurr.x (), pnext.x ()) &&   coord_traits::equals (pcurr.y (), pnext.y ())) ||
                         (! coord_traits::equals (plast.x (), pcurr.x ()) &&   coord_traits::equals (plast.y (), pcurr.y ()) && 
                            coord_traits::equals (pcurr.x (), pnext.x ()) && ! coord_traits::equals (pcurr.y (), pnext.y ())))) {
            //  non-manhattan
            ortho = false;
          }
          
          //  determine min point and corresponding iterator
          if (! min_set || pcurr < pmin) {
            pmin = pcurr;
            min = pp;
            min_set = true;
          }

          plast = pcurr;

        }

        pcurr = pnext;
       
        pp = p;
        if (++p == to) {
          p = from;
        }

        if (pp == from) {
          if (one_round) {
            //  stop on second round
            return; 
          }
          one_round = true;
        }

      } while (true);

      //  if there are less than 3 points remaining, stop now
      if (n < 3) {
        return;
      }

      point_type *pts;
      bool clockwise;

      //  in ortho mode, allocate only half the number of points and compress
      if (ortho) {

        tl_assert ((n % 2) == 0);

        m_size = n / 2;
        pts = new point_type [m_size];

        //  determine orientation:
        //  it is that simple since we know that the segments attached to 
        //  the min point can only point to positive x or y direction:
        p = min;
        pcurr = pmin;
        do {
          if (++p == to) {
            p = from;
          }
          pnext = point_type (tr (*p));
        } while (pnext.equal (pcurr));

        bool eqx = coord_traits::equals (pnext.x (), pcurr.x ());
        bool eqy = coord_traits::equals (pnext.y (), pcurr.y ());

        clockwise = eqx;

        //  perform the actual compression: just store those points that are
        //  distinct by x and y:
        n = 0;
        plast = pts [n++] = pcurr;
        while (n < m_size) {
          do {
            pcurr = pnext;
            if (++p == to) {
              p = from;
            }
            pnext = point_type (tr (*p));
          } while (coord_traits::equals (plast.x (), pcurr.x ()) || coord_traits::equals (plast.y (), pcurr.y ()) || 
                   coord_traits::equals (pnext.x (), pcurr.x ()) != eqx || coord_traits::equals (pnext.y (), pcurr.y ()) != eqy);
          plast = pts [n++] = pcurr;
        }

      } else {

        m_size = n;
        pts = new point_type [m_size];

        //  copy distinct points now
        n = 0;
        p = min;
        point_type plast (tr (*p++));
        if (p == to) {
          p = from;
        }
        point_type pcurr (tr (*p++));
        if (p == to) {
          p = from;
        }

        area_type a = 0;
        pts [n++] = plast;

        while (true) {

          pnext = point_type (tr (*p));

          if (pcurr.equal (plast) || pcurr.equal (pnext) || is_colinear (plast, pcurr, pnext, remove_reflected)) {
            //  skip this point ..
          } else if (n == m_size) {
            //  the contour has closed
            a += vprod (plast - point_type (), pcurr - point_type ());
            break;
          } else {
            //  remember this point
            pts [n++] = pcurr; 
            a += vprod (plast - point_type (), pcurr - point_type ());
            plast = pcurr;
          }

          pcurr = pnext;
         
          if (++p == to) {
            p = from;
          }

        } 

        clockwise = (a < 0);

      }

      //  normalize the orientation
      if ((! clockwise) != hole && normalize) {
        std::reverse (pts + 1, pts + n);
      }

      //  and store the pointer along with two flags: ortho mode and hole flag
      tl_assert (((size_t) pts & 3) == 0);
      mp_points = (point_type *) ((size_t) pts | (hole ? 2 : 0) | (ortho ? 1 : 0)); 

    }
  }

  /**
   *  @brief Moves the contour.
   *
   *  Moves the contour by the given displacement.
   *  Modifies the polygon with the moved contour.
   *  Moving a contour is a fast operation since no renormalization is required.
   *  
   *  @param d The displacement to apply.
   *
   *  @return The moved contour.
   */
  polygon_contour<C> &move (const vector_type &d)
  {
    point_type *p = (point_type *) ((size_t) mp_points & ~3);
    for (size_type i = 0; i < m_size; ++i, ++p) {
      *p += d;
    }
    return *this;
  }

  /**
   *  @brief Return the moved contour
   *
   *  Moves the contour by the given displacement and returns a new object.
   *  Does not modify the contour.
   *  
   *  @param d The displacement to apply.
   *
   *  @return The moved contour.
   */
  polygon_contour<C> moved (const vector_type &d) const
  {
    polygon_contour<C> cont (*this);
    cont.move (d);
    return cont;
  }

  /**
   *  @brief Transform the contour with a generic transformation.
   *
   *  Transforms the contour with the given transformation.
   *  Modifies the polygon with the transformed contour.
   *  
   *  @param t The transformation to apply.
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected true, to remove reflecting spikes if compress is true
   *
   *  @return The transformed contour.
   */
  template <class Trans>
  polygon_contour<C> &transform (const Trans &tr, bool compress = default_compression<C> (), bool remove_reflected = false)
  {
    //  does the transformation the hard way: extract and insert again
    std::vector<point_type> buffer;
    size_type n = size ();
    buffer.reserve (n);
    for (size_type i = 0; i < n; ++i) {
      buffer.push_back ((*this) [i]);
    }
    assign (buffer.begin (), buffer.end (), tr, is_hole (), compress, true, remove_reflected);
    return *this;
  }

  /**
   *  @brief Transform the contour.
   *
   *  Transforms the contour with the given transformation.
   *  Modifies the polygon with the transformed contour.
   *  
   *  @param t The transformation to apply.
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected true, to remove reflecting spikes if compress is true
   *
   *  @return The transformed contour.
   */
  polygon_contour<C> &transform (const trans_type &tr, bool compress = default_compression<C> (), bool remove_reflected = false)
  {
    if (tr.rot () == trans_type::r0 && !compress) {
      move (tr.disp ());
    } else {
      //  does the transformation the hard way: extract and insert again
      std::vector<point_type> buffer;
      size_type n = size ();
      buffer.reserve (n);
      for (size_type i = 0; i < n; ++i) {
        buffer.push_back ((*this) [i]);
      }
      assign (buffer.begin (), buffer.end (), tr, is_hole (), compress, true, remove_reflected);
    }
    return *this;
  }

  /**
   *  @brief Transform the contour.
   *
   *  Transforms the contour with the given transformation.
   *  Does not modify the contour but returns the transformed contour.
   *  
   *  @param t The transformation to apply.
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected true, to remove reflecting spikes if compress is true
   *
   *  @return The transformed contour.
   */
  template <class Tr>
  polygon_contour<typename Tr::target_coord_type> transformed (const Tr &t, bool compress = default_compression<typename Tr::target_coord_type> (), bool remove_reflected = false) const
  {
    //  construct the transformed polygon
    typedef polygon_contour_iterator<polygon_contour<C>, db::unit_trans<C> > iter;
    return polygon_contour<typename Tr::target_coord_type> (iter (this, 0), iter (this, size ()), t, is_hole (), compress, true, remove_reflected);
  }

  /** 
   *  @brief begin iterator for the points of this contour
   */
  simple_iterator begin () const
  {
    return simple_iterator (this, 0);
  }

  /** 
   *  @brief end iterator for the points of this contour
   */
  simple_iterator end () const
  {
    return simple_iterator (this, size ());
  }

  /** 
   *  @brief returns true if the contour is a rectilinear (manhattan) contour
   */
  bool is_rectilinear () const
  {
    if (((size_t) mp_points & 1) != 0) {
      return true;
    }
    if (m_size < 2) {
      return false;
    }
    point_type pl = mp_points [m_size - 1];
    for (size_t i = 0; i < m_size; ++i) {
      point_type p = mp_points [i];
      if (! coord_traits::equals (p.x (), pl.x ()) && ! coord_traits::equals (p.y (), pl.y ())) {
        return false;
      }
      pl = p;
    }
    return true;
  }
  
  /**
   *  @brief returns true if the contour is a half-manhattan contour (multiples of 45 degree)
   */
  bool is_halfmanhattan () const
  {
    if (((size_t) mp_points & 1) != 0) {
      return true;
    }
    if (m_size < 2) {
      return false;
    }
    point_type pl = mp_points [m_size - 1];
    for (size_t i = 0; i < m_size; ++i) {
      point_type p = mp_points [i];
      if (! coord_traits::equals (p.x (), pl.x ()) && ! coord_traits::equals (p.y (), pl.y ()) && ! coord_traits::equals (std::abs (p.x () - pl.x ()), std::abs (p.y () - pl.y ()))) {
        return false;
      }
      pl = p;
    }
    return true;
  }

  /**
   *  @brief Returns true if the contour is a hole
   *
   *  Since this method employs the orientation property the results are only valid for
   *  contours with more than 2 points.
   */
  bool is_hole () const
  {
    return ((size_t) mp_points & 2) != 0;
  }
  
  /** 
   *  @brief The area of the contour
   */
  area_type area () const 
  {
    return area2 () / 2;
  }

  /**
   *  @brief The area of the contour times 2
   *  For integer area types, this is the more precise value as the division
   *  by 2 might round off.
   */
  area_type area2 () const
  {
    size_type n = size ();
    if (n < 3) {
      return 0;
    }

    area_type a = 0;
    point_type pl = (*this) [n - 1];
    for (size_type p = 0; p < n; ++p) {
      point_type pp = (*this) [p];
      a += db::vprod (pp - point_type (), pl - point_type ());
      pl = pp;
    }
    return a;
  }

  /**
   *  @brief The perimeter of the contour
   */
  perimeter_type perimeter () const 
  {
    size_type n = size ();
    if (n < 2) {
      return 0;
    }

    double d = 0;
    point_type pl = (*this) [n - 1];
    for (size_type p = 0; p < n; ++p) {
      point_type pp = (*this) [p];
      d += pp.double_distance (pl);
      pl = pp;
    }

    return coord_traits::rounded_perimeter (d);
  }

  /**
   *  @brief Random access operator
   *
   *  The time for the access operation is guaranteed to be constant.
   */
  point_type operator[] (size_type index) const
  {
    size_t f = (size_t) mp_points;
    point_type *pts = (point_type *) (f & ~3);
    if ((f & 1) != 0) {
      if ((index & 1) != 0) {
        if ((f & 2) != 0) {
          return point_type (pts [((index + 1) / 2) % m_size].x (), pts [(index - 1) / 2].y ());
        } else {
          return point_type (pts [(index - 1) / 2].x (), pts [((index + 1) / 2) % m_size].y ());
        }
      } else {
        return pts [index / 2];
      }
    } else {
      return pts [index];
    }
  }

  /**
   *  @brief Sizing
   *
   *  Shifts the contour outwards (dx,dy>0) or inwards (dx,dy<0).
   *  May create invalid (self-overlapping, reverse oriented) contours. 
   *  The sign of dx and dy should be identical.
   *
   *  The mode defines at which bending angle cutoff occurs
   *    0: >0
   *    1: >45
   *    2: >90
   *    3: >135
   *    4: >168 (approx)
   *    other: >179 (approx)
   */
  void size (coord_type dx, coord_type dy, unsigned int mode);

  /**
   *  @brief Return the size of the contour
   *
   *  The time for the size operation is guaranteed to be constant.
   */
  size_type size () const 
  {
    if ((size_t) mp_points & 1) {
      return m_size * 2;
    } else {
      return m_size;
    }
  }

  /**
   *  @brief Compute the bounding box
   *
   *  The bounding box is computed on-the-fly and is not stored.
   *  Computing the bbox is not a cheap operation.
   *  It is somewhat simplified in the manhattan case since it 
   *  can be computed from the reduced point set.
   */
  box_type bbox () const
  {
    box_type box;
    point_type *p = (point_type *) ((size_t) mp_points & ~3);
    for (size_type i = 0; i < m_size; ++i, ++p) {
      box += *p;
    }
    return box;
  }

  /** 
   *  @brief Clear the contour
   */
  void clear ()
  {
    release ();
  }

  /**
   *  @brief Equality test
   */
  bool operator== (const polygon_contour<C> &d) const
  {
    if (size () != d.size ()) {
      return false;
    }
    if (is_hole () != d.is_hole ()) {
      return false;
    }
    simple_iterator p1 = begin (), p2 = d.begin ();
    while (p1 != end ()) {
      if (*p1 != *p2) {
        return false;
      }
      ++p1; ++p2;
    }
    return true;
  }

  /**
   *  @brief Inequality test
   */
  bool operator!= (const polygon_contour<C> &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief Less than-operator
   */
  bool operator< (const polygon_contour<C> &d) const
  {
    if (size () != d.size ()) {
      return size () < d.size ();
    }
    if (is_hole () != d.is_hole ()) {
      return is_hole () < d.is_hole ();
    }
    simple_iterator p1 = begin (), p2 = d.begin ();
    while (p1 != end ()) {
      if (*p1 != *p2) {
        return *p1 < *p2;
      }
      ++p1; ++p2;
    }
    return false;
  }

  /**
   *  @brief Fuzzy equality test
   */
  bool equal (const polygon_contour<C> &d) const
  {
    if (size () != d.size ()) {
      return false;
    }
    if (is_hole () != d.is_hole ()) {
      return false;
    }
    simple_iterator p1 = begin (), p2 = d.begin ();
    while (p1 != end ()) {
      if (! (*p1).equal (*p2)) {
        return false;
      }
      ++p1; ++p2;
    }
    return true;
  }

  /**
   *  @brief Fuzzy inequality test
   */
  bool not_equal (const polygon_contour<C> &d) const
  {
    return ! equal (d);
  }

  /**
   *  @brief Fuzzy less than-operator
   */
  bool less (const polygon_contour<C> &d) const
  {
    if (size () != d.size ()) {
      return size () < d.size ();
    }
    if (is_hole () != d.is_hole ()) {
      return is_hole () < d.is_hole ();
    }
    simple_iterator p1 = begin (), p2 = d.begin ();
    while (p1 != end ()) {
      if (! (*p1).equal (*p2)) {
        return (*p1).less (*p2);
      }
      ++p1; ++p2;
    }
    return false;
  }

  /**
   *  @brief swap with a different contour
   */
  void swap (polygon_contour<C> &d)
  {
    std::swap (m_size, d.m_size);
    std::swap (mp_points, d.mp_points);
  }

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
    stat->add (typeid (point_type []), (void *) mp_points, sizeof (point_type) * m_size, sizeof (point_type) * m_size, (void *) this, purpose, cat);
  }

private:
  point_type *mp_points;
  size_type m_size;

  void release ()
  {
    point_type *p = (point_type *) ((size_t) mp_points & ~3);
    if (p) {
      delete [] p;
    }
    mp_points = 0;
    m_size = 0;
  }
};

} // namespace db

namespace std
{

//  injecting a global std::swap for polygons into the
//  std namespace
template <class C>
void swap (db::polygon_contour<C> &a, db::polygon_contour<C> &b)
{
  a.swap (b);
}

}

namespace db
{

/**
 *  @brief The contour point iterator
 *
 *  The point iterator delivers all points of a contour.
 *  It is based on the random access operator of the contour
 */
 
template <class Contour, class Tr>
class polygon_contour_iterator
{
public:
  typedef Contour contour_type;
  typedef typename contour_type::value_type point_type;
  typedef typename contour_type::value_type value_type;
  typedef void pointer;
  typedef value_type reference; 
  typedef typename point_type::coord_type coord_type;
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef Tr trans_type;
  typedef ptrdiff_t difference_type;

  /** 
   *  @brief The default constructor 
   */
  polygon_contour_iterator ()
    : mp_contour (0), m_index (0), m_reverse (false)
  {
    //  .. nothing yet .. 
  }
  
  /** 
   *  @brief The standard constructor 
   */
  polygon_contour_iterator (const Contour *contour, size_t n, bool reverse = false)
    : mp_contour (contour), m_index (n), m_reverse (reverse)
  {
    //  .. nothing yet .. 
  }
  
  /** 
   *  @brief The standard constructor with a transformation
   */
  template <class T> 
  polygon_contour_iterator (const polygon_contour_iterator<Contour, T> &d, const trans_type &trans, bool reverse = false) 
    : mp_contour (d.mp_contour), m_index (d.m_index), m_trans (trans), m_reverse (reverse)
  {
    //  .. nothing yet .. 
  }
  
  /**
   *  @brief Sorting order
   */
  bool operator< (const polygon_contour_iterator &d) const
  {
    return m_index < d.m_index;
  }

  /**
   *  @brief Equality test
   */
  bool operator== (const polygon_contour_iterator &d) const
  {
    return m_index == d.m_index;
  }

  /**
   *  @brief Inequality test
   */
  bool operator!= (const polygon_contour_iterator &d) const
  {
    return m_index != d.m_index;
  }

  /**
   *  @brief Point access
   */
  point_type operator* () const 
  {
    return m_trans ((*mp_contour) [m_index]);
  }

  /**
   *  @brief Addition of differences
   */
  polygon_contour_iterator operator+ (difference_type d) const
  {
    if (m_reverse) {
      return polygon_contour_iterator (mp_contour, m_index - d);
    } else {
      return polygon_contour_iterator (mp_contour, m_index + d);
    }
  }

  /**
   *  @brief Addition of distances
   */
  polygon_contour_iterator &operator+= (difference_type d) 
  {
    if (m_reverse) {
      m_index -= d;
    } else {
      m_index += d;
    }
    return *this;
  }

  /**
   *  @brief Subtraction of distances
   */
  polygon_contour_iterator operator- (difference_type d) const
  {
    return operator+ (-d);
  }

  /**
   *  @brief Subtraction of distances
   */
  polygon_contour_iterator &operator-= (difference_type d) 
  {
    return operator+= (-d);
  }

  /**
   *  @brief Subtraction of iterators
   */
  difference_type operator- (const polygon_contour_iterator &d) const
  {
    if (m_reverse) {
      return d.m_index - m_index;
    } else {
      return m_index - d.m_index;
    }
  }

  /**
   *  @brief Increment operator
   */
  polygon_contour_iterator &operator++ () 
  {
    return operator+= (1);
  }

  /**
   *  @brief Postfix increment operator
   */
  polygon_contour_iterator operator++ (int) 
  {
    polygon_contour_iterator i (*this);
    operator+= (1);
    return i;
  }

  /**
   *  @brief Decrement operator
   */
  polygon_contour_iterator &operator-- () 
  {
    return operator-= (1);
  }

  /**
   *  @brief Postfix decrement operator
   */
  polygon_contour_iterator operator-- (int) 
  {
    polygon_contour_iterator i (*this);
    operator-= (1);
    return i;
  }

private:
  template <class Ct, class T> friend class polygon_contour_iterator;

  const contour_type *mp_contour;
  size_t m_index;
  trans_type m_trans;
  bool m_reverse;
};

/**
 *  @brief The polygon edge iterator
 *
 *  The edge iterator delivers all edges of the polygon with a 
 *  distinct orientation (inside is 'right', outside is 'left).
 */
 
template <class P, class Tr>
class polygon_edge_iterator
{
public:
  typedef P polygon_type;
  typedef typename polygon_type::coord_type coord_type;
  typedef typename polygon_type::contour_type contour_type;
  typedef db::edge<coord_type> edge_type;
  typedef edge_type value_type;
  typedef db::point<coord_type> point_type;
  typedef Tr trans_type;
  typedef void pointer;           //  no operator->
  typedef edge_type reference;    //  operator* returns a value
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef void difference_type;

  /** 
   *  @brief The default constructor 
   */
  polygon_edge_iterator ()
    : mp_polygon (0), m_ctr (0), m_num_ctr (0), m_pt (0), m_trans ()
  {
    //  .. nothing yet .. 
  }
  
  /** 
   *  @brief The standard constructor 
   */
  polygon_edge_iterator (const polygon_type &polygon)
    : mp_polygon (&polygon), m_ctr (0), m_num_ctr (polygon.holes () + 1), m_pt (0), m_trans ()
  {
    //  A polygon may be "empty": then it does not even have a hull ..
    if (mp_polygon->hull ().size () == 0) {
      m_num_ctr = 0;
    }
  }
  
  /** 
   *  @brief The standard constructor with a transformation
   */
  polygon_edge_iterator (const polygon_type &polygon, const trans_type &trans) 
    : mp_polygon (&polygon), m_ctr (0), m_num_ctr (polygon.holes () + 1), m_pt (0), m_trans (trans)
  {
    //  A polygon may be "empty": then it does not even have a hull ..
    if (mp_polygon->hull ().size () == 0) {
      m_num_ctr = 0;
    }
  }
  
  /** 
   *  @brief The standard constructor for one specific contour
   */
  polygon_edge_iterator (const polygon_type &polygon, unsigned int ctr)
    : mp_polygon (&polygon), m_ctr (ctr), m_num_ctr (std::min (polygon.holes (), ctr) + 1), m_pt (0), m_trans ()
  {
    //  The contour may be "empty"
    while (m_ctr < m_num_ctr && mp_polygon->contour (m_ctr).size () == 0) {
      ++m_ctr;
    }
  }
  
  /** 
   *  @brief The standard constructor for one specific contour with a transformation
   */
  polygon_edge_iterator (const polygon_type &polygon, unsigned int ctr, const trans_type &trans) 
    : mp_polygon (&polygon), m_ctr (ctr), m_num_ctr (std::min (polygon.holes (), ctr) + 1), m_pt (0), m_trans (trans)
  {
    //  The contour may be "empty"
    while (m_ctr < m_num_ctr && mp_polygon->contour (m_ctr).size () == 0) {
      ++m_ctr;
    }
  }
  
  /**
   *  @brief at_end predicate
   */
  bool at_end () const
  {
    return m_ctr >= m_num_ctr;
  }

  /**
   *  @brief Gets the contour number
   */
  unsigned int contour () const
  {
    return m_ctr;
  }

  /**
   *  @brief Edge access
   */
  edge_type operator* () const 
  {
    const contour_type *c = get_ctr ();
    
    point_type p1 (m_trans ((*c) [m_pt]));
    point_type p2 (m_trans ((*c) [m_pt + 1 >= c->size () ? 0 : m_pt + 1]));

    //  to maintain the edge orientation we need to swap start end end point
    //  if the transformation is mirroring
    if (m_trans.is_mirror ()) {
      return edge_type (p2, p1);
    } else {
      return edge_type (p1, p2);
    }
  }

  /**
   *  @brief Increment operator
   */
  polygon_edge_iterator &operator++ () 
  {
    const contour_type *c = get_ctr ();
    if (++m_pt == c->size ()) {
      m_pt = 0;
      //  polygons may contain empty contours (holes): skip those
      do { 
        ++m_ctr;
      } while (! at_end () && get_ctr ()->size () == 0);
    }
    return *this;
  }

  /**
   *  @brief Decrement operator
   */
  polygon_edge_iterator &operator-- () 
  {
    if (m_pt == 0) {
      //  polygons may contain empty contours (holes): skip those
      do {
        --m_ctr;
      } while (m_ctr > 0 && get_ctr ()->size () == 0);
      m_pt = get_ctr ()->size () - 1;
    } else {
      --m_pt;
    }
    return *this;
  }

private:
  const polygon_type *mp_polygon;
  unsigned int m_ctr, m_num_ctr;
  size_t m_pt;
  trans_type m_trans;

  //  fetch the contour pointer to the current contour
  const contour_type *get_ctr () const
  {
    return &(mp_polygon->contour (m_ctr));
  }
};

/** 
 *  @brief A polygon class
 *
 *  A polygon consists of an outer hull and zero to many
 *  holes. Each contour consists of several points. The point
 *  list is normalized such that the leftmost, lowest point is 
 *  the first one. The orientation is normalized such that
 *  the orientation of the hull contour is clockwise, while
 *  the orientation of the holes is counterclockwise.
 *
 *  It is in no way checked that the contours are not over-
 *  lapping. This must be ensured by the user of the object
 *  when filling the contours.
 */

template <class C>
class DB_PUBLIC_TEMPLATE polygon
{
public:
  typedef C coord_type;
  typedef db::edge<coord_type> edge_type;
  typedef db::simple_trans<coord_type> trans_type;
  typedef db::point<coord_type> point_type;
  typedef db::vector<coord_type> vector_type;
  typedef db::box<coord_type> box_type;
  typedef db::coord_traits<coord_type> coord_traits;
  typedef typename coord_traits::distance_type distance_type; 
  typedef typename coord_traits::perimeter_type perimeter_type; 
  typedef typename coord_traits::area_type area_type; 
  typedef polygon_contour<C> contour_type;
  typedef tl::vector<contour_type> contour_list_type;
  typedef db::polygon_edge_iterator< polygon<C>, db::unit_trans<C> > polygon_edge_iterator;
  typedef db::polygon_contour_iterator< contour_type, db::unit_trans<C> > polygon_contour_iterator;
  typedef db::object_tag< polygon<C> > tag;


  /**
   *  @brief The default constructor.
   *  
   *  The default constructor creates a empty polygon.
   */
  polygon ()
  {
    //  create an entry for the hull contour
    m_ctrs.push_back (contour_type ());
  }

  /**
   *  @brief The copy constructor from another polygon with a transformation
   *
   *  The transformation must have the ability to transform a point to another point of type C.
   *
   *  @param p The source polygon
   *  @param tr The transformation to apply on assignment
   *  @param compress True, if the contours shall be compressed
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   */
  template <class D, class T>
  polygon (const db::polygon<D> &p, const T &tr, bool compress = default_compression<C> (), bool remove_reflected = false)
  {
    //  create an entry for the hull contour
    m_bbox = box_type (tr (p.box ().p1 ()), tr (p.box ().p2 ()));
    m_ctrs.resize (p.holes () + 1);
    m_ctrs [0].assign (p.begin_hull (), p.end_hull (), tr, false, compress, true /*normalize*/, remove_reflected);
    for (unsigned int i = 0; i < m_ctrs.size () - 1; ++i) {
      m_ctrs [i + 1].assign (p.begin_hole (i), p.end_hole (i), tr, true, compress, true /*normalize*/, remove_reflected);
    }
  }

  /**
   *  @brief The copy constructor from another polygon a different coordinate type
   *
   *  @param p The source polygon
   *  @param compress True, if the contours shall be compressed
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   */
  template <class D>
  explicit polygon (const db::polygon<D> &p, bool compress = default_compression<C> (), bool remove_reflected = false)
  {
    db::point_coord_converter<C, D> tr;

    //  create an entry for the hull contour
    m_bbox = box_type (tr (p.box ().p1 ()), tr (p.box ().p2 ()));
    m_ctrs.resize (p.holes () + 1);
    m_ctrs [0].assign (p.begin_hull (), p.end_hull (), tr, false, compress, true /*normalize*/, remove_reflected);
    for (unsigned int i = 0; i < m_ctrs.size () - 1; ++i) {
      m_ctrs [i + 1].assign (p.begin_hole (i), p.end_hole (i), tr, true, compress, true /*normalize*/, remove_reflected);
    }
  }

  /**
   *  @brief The box constructor.
   *  
   *  Creates a polygon from a box
   */
  explicit polygon (const db::box<C> &b)
  {
    //  create an entry for the hull contour
    m_ctrs.push_back (contour_type ());

    point_type p[4];
    p[0] = point_type (b.left (), b.bottom ());
    p[1] = point_type (b.left (), b.top ());
    p[2] = point_type (b.right (), b.top ());
    p[3] = point_type (b.right (), b.bottom ());
    m_ctrs.back ().assign (p, p + 4, false, false /*don't compress*/);
    m_bbox = b;
  }

  /**
   *  @brief The (dummy) translation operator
   */
  void translate (const polygon<C> &d, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
  }

  /**
   *  @brief The (dummy) translation operator with transformation
   */
  template <class T>
  void translate (const polygon<C> &d, const T &t, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
    transform (t);
  }

  /**
   *  @brief A less operator to establish a sorting order.
   */
  bool operator< (const polygon<C> &b) const
  {
    //  do the simple tests first
    if (holes () < b.holes ()) {
      return true;
    } else if (holes () != b.holes ()) {
      return false;
    }

    if (m_bbox < b.m_bbox) {
      return true;
    } else if (m_bbox != b.m_bbox) {
      return false;
    }

    //  since the list of holes is maintained sorted, we can just
    //  compare by comparing the holes contours (all must be equal)
    typename contour_list_type::const_iterator hh = b.m_ctrs.begin (); 
    typename contour_list_type::const_iterator h = m_ctrs.begin (); 
    while (h != m_ctrs.end ()) {
      if (*h < *hh) {
        return true;
      } else if (*h != *hh) {
        return false;
      }
      ++h;
      ++hh;
    }

    return false;
  }

  /** 
   *  @brief Equality test
   */
  bool operator== (const polygon<C> &b) const
  {
    if (m_bbox == b.m_bbox && holes () == b.holes ()) {

      //  since the list of holes is maintained sorted, we can just
      //  compare by comparing the holes contours (all must be equal)
      typename contour_list_type::const_iterator hh = b.m_ctrs.begin (); 
      typename contour_list_type::const_iterator h = m_ctrs.begin (); 
      while (h != m_ctrs.end ()) {
        if (*h != *hh) {
          return false;
        }
        ++h;
        ++hh;
      }

      return true;

    } else {
      return false;
    }
  }

  /** 
   *  @brief Inequality test
   */
  bool operator!= (const polygon<C> &b) const
  {
    return !operator== (b);
  }

  /**
   *  @brief A fuzzy less operator to establish a sorting order.
   */
  bool less (const polygon<C> &b) const
  {
    //  do the simple tests first
    if (holes () < b.holes ()) {
      return true;
    } else if (holes () != b.holes ()) {
      return false;
    }

    if (m_bbox.less (b.m_bbox)) {
      return true;
    } else if (m_bbox.not_equal (b.m_bbox)) {
      return false;
    }

    //  since the list of holes is maintained sorted, we can just
    //  compare by comparing the holes contours (all must be equal)
    typename contour_list_type::const_iterator hh = b.m_ctrs.begin ();
    typename contour_list_type::const_iterator h = m_ctrs.begin ();
    while (h != m_ctrs.end ()) {
      if (h->less (*hh)) {
        return true;
      } else if (h->not_equal (*hh)) {
        return false;
      }
      ++h;
      ++hh;
    }

    return false;
  }

  /**
   *  @brief Fuzzy equality test
   */
  bool equal (const polygon<C> &b) const
  {
    if (m_bbox.equal (b.m_bbox) && holes () == b.holes ()) {

      //  since the list of holes is maintained sorted, we can just
      //  compare by comparing the holes contours (all must be equal)
      typename contour_list_type::const_iterator hh = b.m_ctrs.begin ();
      typename contour_list_type::const_iterator h = m_ctrs.begin ();
      while (h != m_ctrs.end ()) {
        if (h->not_equal (*hh)) {
          return false;
        }
        ++h;
        ++hh;
      }

      return true;

    } else {
      return false;
    }
  }

  /**
   *  @brief Fuzzy inequality test
   */
  bool not_equal (const polygon<C> &b) const
  {
    return !equal (b);
  }

  /**
   *  @brief Returns true, if the polygon is a simple box
   */
  bool is_box () const
  {
    return m_ctrs.size () == 1 && m_ctrs [0].size () == 4 && m_ctrs [0].is_rectilinear ();
  }

  /**
   *  @brief Returns true, if the polygon is rectilinear
   */
  bool is_rectilinear () const
  {
    for (size_t i = 0; i < m_ctrs.size (); ++i) {
      if (! m_ctrs [i].is_rectilinear ()) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief Returns true, if the polygon is halfmanhattan
   */
  bool is_halfmanhattan () const
  {
    for (size_t i = 0; i < m_ctrs.size (); ++i) {
      if (! m_ctrs [i].is_halfmanhattan ()) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief Returns the number of points in the polygon
   */
  size_t vertices () const
  {
    size_t n = 0;
    for (typename contour_list_type::const_iterator h = m_ctrs.begin (); h != m_ctrs.end (); ++h) {
      n += h->size ();
    }
    return n;
  }

  /**
   *  @brief Returns the area ratio between the polygon's bounding box and actual area
   *
   *  This number is a measure how well the polygon is approximated by the bounding box.
   *  Values are bigger than 1 for well-formed polygons. Bigger values mean worse
   *  approximation.
   */
  double area_ratio () const
  {
    area_type a = area2 ();
    if (a == 0) {
      //  By our definition, an empty polygon has an area ratio of 0
      return 0.0;
    } else {
      return double (box ().area ()) / (0.5 * a);
    }
  }

  /**
   *  @brief The hull "begin" point iterator
   *
   *  The hull point sequence delivers the points that the hull
   *  is made of.
   */
  polygon_contour_iterator begin_hull () const
  {
    return polygon_contour_iterator (& hull (), 0);
  }

  /**
   *  @brief The hull "end" point iterator
   *
   *  The hull point sequence delivers the points that the hull
   *  is made of.
   */
  polygon_contour_iterator end_hull () const
  {
    return polygon_contour_iterator (& hull (), hull ().size ());
  }

  /**
   *  @brief The hole "begin" point iterator
   *
   *  The hole point sequence delivers the points that the specific hole
   *  is made of.
   */
  polygon_contour_iterator begin_hole (unsigned int h) const
  {
    return polygon_contour_iterator (& begin_holes () [h], 0);
  }

  /**
   *  @brief The hole "end" point iterator
   *
   *  The hole point sequence delivers the points that the specific hole
   *  is made of.
   */
  polygon_contour_iterator end_hole (unsigned int h) const
  {
    return polygon_contour_iterator (& begin_holes () [h], begin_holes () [h].size ());
  }

  /**
   *  @brief Compress the polygon.
   *
   *  Compressing means removing redundant points (identical, colinear)
   *
   *  @param remove_reflected Removes reflected edges (spiked) if this parameter is true.
   *  @return The compressed polygon.
   */
  polygon<C> &compress (bool remove_reflected = false)
  {
    //  compress the contours by employing the transform method with a unit transformation
    for (typename contour_list_type::iterator h = m_ctrs.begin (); h != m_ctrs.end (); ++h) {
      h->transform (db::unit_trans<C> (), true /*compress*/, remove_reflected);
    }
    return *this;
  }

  /**
   *  @brief Transform the polygon.
   *
   *  Transforms the polygon with the given transformation.
   *  Modifies the polygon with the transformed polygon.
   *  
   *  @param t The transformation to apply.
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   *
   *  @return The transformed polygon.
   */
  template <class Trans>
  polygon<C> &transform (const Trans &t, bool compress = default_compression<C> (), bool remove_reflected = false)
  {
    //  insert the transformed holes and normalize
    for (typename contour_list_type::iterator h = m_ctrs.begin (); h != m_ctrs.end (); ++h) {
      h->transform (t, compress, remove_reflected);
    }

    //  transform or recompute the bbox
    if (t.is_ortho ()) {
      m_bbox.transform (t);
    } else {
      m_bbox = m_ctrs [0].bbox ();
    }

    //  keep the list of holes sorted ..
    tl::sort (m_ctrs.begin () + 1, m_ctrs.end ());

    return *this;
  }

  /**
   *  @brief Transform the polygon.
   *
   *  Transforms the polygon with the given transformation.
   *  Does not modify the polygon but returns the transformed polygon.
   *  
   *  @param t The transformation to apply.
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   *
   *  @return The transformed polygon.
   */
  template <class Tr>
  polygon<typename Tr::target_coord_type> transformed (const Tr &t, bool compress = default_compression<typename Tr::target_coord_type> (), bool remove_reflected = false) const
  {
    typedef typename Tr::target_coord_type target_coord_type;
    polygon<target_coord_type> poly;

    //  transform the contours and add as new ones
    poly.assign_hull (begin_hull (), end_hull (), t, compress, remove_reflected);
    for (unsigned int h = 0; h < holes (); ++h) {
      poly.insert_hole (begin_hole (h), end_hole (h), t, compress, remove_reflected);
    }

    return poly;
  }

  /**
   *  @brief Returns the moved polyon
   *
   *  Moves the polygon by the given offset and returns the 
   *  moved polygon. The polygon is not modified.
   *
   *  @param p The distance to move the polygon.
   * 
   *  @return The moved polygon.
   */
  polygon<C> moved (const vector<C> &p) const
  {
    polygon<C> b (*this);
    b.move (p);
    return b;
  }

  /**
   *  @brief Moves the polygon.
   *
   *  Moves the polygon by the given offset and returns the 
   *  moved polygon. The polygon is overwritten.
   *
   *  @param p The distance to move the polygon.
   * 
   *  @return The moved polygon.
   */
  polygon<C> &move (const vector<C> &d)
  {
    m_bbox.move (d);
    //  move the contours
    for (typename contour_list_type::iterator h = m_ctrs.begin (); h != m_ctrs.end (); ++h) {
      h->move (d);
    }
    return *this;
  }

  /**
   *  @brief Returns the bounding box of the polygon
   */
  const db::box<C> &box () const
  {
    return m_bbox;
  }

  /**
   *  @brief Clears the polygon
   *
   *  @param n The number of holes to reserve memory for
   */
  void clear (unsigned int n = 0) 
  {
    m_bbox = db::box<C> ();
    m_ctrs.clear ();
    m_ctrs.reserve (n + 1);
    m_ctrs.push_back (contour_type ());
  }

  /**
   *  @brief Reserves memory for n holes
   *
   *  @param n The number of holes to reserve memory for
   */
  void reserve_holes (unsigned int n) 
  {
    m_ctrs.reserve (n + 1);
  }

  /** 
   *  @brief Assign the contour from another contour
   *
   *  Replaces the outer contour by the given other contour
   *  The given contour must not be a hole contour and is_hole must be false.
   *
   *  @param other The other contour
   */
  void assign_hull (const contour_type &other)
  {
    tl_assert (! other.is_hole ());
    m_ctrs [0] = other;
    m_bbox = m_ctrs [0].bbox ();
  }

  /** 
   *  @brief Set the outer contour
   *
   *  Replaces the outer contour by the points given by
   *  the sequence [start,end). This method will update the
   *  bounding box and normalize the hull, so it is oriented
   *  properly.
   *
   *  @param start The start of the sequence of points for the contour
   *  @param end The end of the sequence of points for the contour
   *  @param compress true, if the sequence shall be compressed (colinear points removed)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   */
  template <class I> 
  void assign_hull (I start, I end, bool compress = default_compression<C> (), bool remove_reflected = false) 
  {
    m_ctrs [0].assign (start, end, false, compress, true /*normalize*/, remove_reflected);
    m_bbox = m_ctrs [0].bbox ();
  }

  /** 
   *  @brief Set the outer contour by a transformed set of points
   *
   *  Replaces the outer contour by the points given by
   *  the sequence [start,end), transformed with the operator op. 
   *  This method will update the bounding box and normalize the hull, 
   *  so it is oriented properly.
   *  @param compress true, if the sequence shall be compressed (colinear points removed)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   *
   *  @param start The start of the sequence of points for the contour
   *  @param end The end of the sequence of points for the contour
   */
  template <class I, class T> 
  void assign_hull (I start, I end, T op, bool compress = default_compression<C> (), bool remove_reflected = false) 
  {
    m_ctrs [0].assign (start, end, op, false, compress, true /*normalize*/, remove_reflected);
    m_bbox = m_ctrs [0].bbox ();
  }

  /** 
   *  @brief Assigns a hole from another contour
   *
   *  Replaced a hole with a copy of the given contour.
   *  The given contour must be a hole contour and is_hole must be true.
   *
   *  @param h The hole index of the hole to replace
   *  @param other The other contour
   */
  void assign_hole (unsigned int h, const contour_type &other)
  {
    tl_assert (other.is_hole ());
    m_ctrs [h + 1] = other;
  }

  /** 
   *  @brief Set a hole contour
   *
   *  Replaces a hole contour by the points given by
   *  the sequence [start,end). This method will update the
   *  bounding box and normalize the contour, so it is oriented
   *  properly.
   *
   *  @param h The hole index of the hole to replace
   *  @param start The start of the sequence of points for the contour
   *  @param end The end of the sequence of points for the contour
   *  @param compress true, if the sequence shall be compressed (colinear points removed)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   */
  template <class I> 
  void assign_hole (unsigned int h, I start, I end, bool compress = default_compression<C> (), bool remove_reflected = false) 
  {
    m_ctrs [h + 1].assign (start, end, true, compress, true /*normalize*/, remove_reflected);
  }

  /** 
   *  @brief Set a hole by a transformed set of points
   *
   *  Replaces a hole contour by the points given by
   *  the sequence [start,end), transformed with the operator op. 
   *  This method will update the bounding box and normalize the contour, 
   *  so it is oriented properly.
   *  @param compress true, if the sequence shall be compressed (colinear points removed)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   *
   *  @param start The start of the sequence of points for the contour
   *  @param end The end of the sequence of points for the contour
   */
  template <class I, class T> 
  void assign_hole (unsigned int h, I start, I end, T op, bool compress = default_compression<C> (), bool remove_reflected = false) 
  {
    m_ctrs [h + 1].assign (start, end, op, true, compress, true /*normalize*/, remove_reflected);
  }

  /** 
   *  @brief Add a hole from another contour
   *
   *  Add a contour hole as a copy of the given contour.
   *  The given contour must be a hole contour and is_hole must be true.
   *
   *  @param other The other contour
   */
  void insert_hole (const contour_type &other)
  {
    tl_assert (other.is_hole ());
    contour_type &h = add_hole ();
    h = other;
  }

  /** 
   *  @brief Add a hole
   *
   *  Adds a hole contour with the points given by
   *  the sequence [start,end). This method will update the
   *  bounding box and normalize the hull, so it is oriented
   *  properly.
   *  It is not checked, whether the hole really is inside
   *  the hull.
   *
   *  @param start The start of the sequence of points for the contour
   *  @param end The end of the sequence of points for the contour
   *  @param compress true, if the sequence shall be compressed (colinear points removed)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   */
  template <class I> 
  void insert_hole (I start, I end, bool compress = default_compression<C> (), bool remove_reflected = false)
  {
    insert_hole (start, end, db::unit_trans<C> (), compress, remove_reflected);
  }

  /** 
   *  @brief Add a hole of transformed points
   *
   *  Adds a hole contour with the points given by
   *  the sequence [start,end), transformed with the given 
   *  function. This method will update the bounding box and 
   *  normalize the hull, so it is oriented properly.
   *  It is not checked, whether the hole really is inside
   *  the hull.
   *
   *  @param start The start of the sequence of points for the contour
   *  @param end The end of the sequence of points for the contour
   *  @param compress true, if the sequence shall be compressed (colinear points removed)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   */
  template <class I, class T> 
  void insert_hole (I start, I end, T op, bool compress = default_compression<C> (), bool remove_reflected = false) 
  {
    //  add the hole
    contour_type &h = add_hole ();
    h.assign (start, end, op, true, compress, true /*normalize*/, remove_reflected);
  }

  /**
   *  @brief Sort the holes
   *
   *  Sorting the holes makes certain algorithms more effective.
   */
  void sort_holes ()
  {
    if (! m_ctrs.empty ()) {
      std::sort (m_ctrs.begin () + 1, m_ctrs.end ());
    }
  }

  /**
   *  @brief Outer contour
   */
  const contour_type &hull () const
  {
    return m_ctrs [0];
  }

  /**
   *  @brief The contour of the nth hole
   */
  const contour_type &hole (unsigned int h) const
  {
    return m_ctrs [h + 1];
  }

  /**
   *  @brief The number of holes
   */
  unsigned int holes () const
  {
    return (unsigned int) m_ctrs.size () - 1;
  }

  /**
   *  @brief Hole iterator: start
   */
  typename contour_list_type::const_iterator begin_holes () const 
  {
    return m_ctrs.begin () + 1;
  }

  /**
   *  @brief Hole iterator: end
   */
  typename contour_list_type::const_iterator end_holes () const 
  {
    return m_ctrs.end ();
  }

  /** 
   *  @brief The edge iterator begin function
   *
   *  The edge iterator delivers all edges of the polygon.
   *
   *  @return the begin value of the iterator
   */
  polygon_edge_iterator begin_edge () const 
  { 
    return polygon_edge_iterator (*this);
  }
  
  /** 
   *  @brief The edge iterator begin function for a specific contour
   *
   *  The edge iterator delivers all edges of the polygon for the given contour.
   *  Contour 0 is the hull, 1 the first hole and so forth.
   *
   *  @return the begin value of the iterator
   */
  polygon_edge_iterator begin_edge (unsigned int ctr) const 
  { 
    return polygon_edge_iterator (*this, ctr);
  }
  
  /** 
   *  @brief The area of the polygon
   */
  area_type area () const 
  {
    area_type a = 0;
    for (typename contour_list_type::const_iterator h = m_ctrs.begin (); h != m_ctrs.end (); ++h) {
      a += h->area ();
    }
    return a;
  }

  /**
   *  @brief The area of the polygon times 2
   *  For integer area types, this is the more precise value as the division
   *  by 2 might round off.
   */
  area_type area2 () const
  {
    area_type a = 0;
    for (typename contour_list_type::const_iterator h = m_ctrs.begin (); h != m_ctrs.end (); ++h) {
      a += h->area2 ();
    }
    return a;
  }

  /**
   *  @brief The perimeter of the polygon
   */
  perimeter_type perimeter () const 
  {
    perimeter_type p = 0;
    for (typename contour_list_type::const_iterator h = m_ctrs.begin (); h != m_ctrs.end (); ++h) {
      p += h->perimeter ();
    }
    return p;
  }

  /**
   *  @brief The string conversion function
   */
  std::string to_string () const
  {
    std::string s = "(";

    //  the hull contour
    for (polygon_contour_iterator p = begin_hull (); p != end_hull (); ++p) {
      if (p != begin_hull ()) {
        s += ";";
      }
      s += (*p).to_string ();
    }

    //  and the hole contours
    for (unsigned int h = 0; h < holes (); ++h) {
      s += "/";
      for (polygon_contour_iterator p = begin_hole (h); p != end_hole (h); ++p) {
        if (p != begin_hole (h)) {
          s += ";";
        }
        s += (*p).to_string ();
      }
    }

    s += ")";
    return s;
  }

  /**
   *  @brief Swap the polygon with another one
   * 
   *  The global std::swap function injected into the std namespace
   *  is redirected to this implementation.
   */
  void swap (polygon<C> &d)
  {
    m_ctrs.swap (d.m_ctrs);
    std::swap (m_bbox, d.m_bbox);
  }

  /**
   *  @brief Direct access to the contours (used for iterators)
   */
  const contour_type &contour (unsigned int n) const
  {
    return m_ctrs [n];
  }

  /**
   *  @brief Reduce the polygon
   *
   *  Reduction of a polygon normalizes the polygon by extracting
   *  a suitable transformation and placing the polygon in a unique
   *  way.
   *
   *  @return The transformation that must be applied to render the original polygon
   */
  void reduce (simple_trans<coord_type> &tr)
  {
    if (! m_ctrs.empty () && m_ctrs [0].size () > 0) {
      vector_type d (m_ctrs [0][0] - point_type ());
      move (-d);
      tr = simple_trans<coord_type> (simple_trans<coord_type>::r0, d);
    }
  }

  /**
   *  @brief Reduce the polygon
   *
   *  Reduction of a polygon normalizes the polygon by extracting
   *  a suitable transformation and placing the polygon in a unique
   *  way.
   *
   *  @return The transformation that must be applied to render the original polygon
   */
  void reduce (disp_trans<coord_type> &tr)
  {
    if (! m_ctrs.empty () && m_ctrs [0].size () > 0) {
      vector_type d (m_ctrs [0][0] - point_type ());
      move (-d);
      tr = disp_trans<coord_type> (d);
    }
  }

  /**
   *  @brief Reduce the polygon for unit transformation references
   *
   *  Does not do any reduction since no transformation can be provided.
   *
   *  @return A unit transformation
   */
  void reduce (unit_trans<C> &)
  {
    //  .. nothing ..
  }

  /**
   *  @brief Sizing with isotropic valued
   *
   *  This convenience method is identical to size (d, d, mode).
   */
  void size (coord_type d, unsigned int mode = 2)
  {
    size (d, d, mode);
  }

  /**
   *  @brief Sizing with anisotropic values
   *
   *  Shifts the polygon contours outwards (dx,dy>0) or inwards (dx,dy<0).
   *  May create invalid (self-overlapping, reverse oriented) polygons. In particular, holes
   *  may grow larger than the hull (in which case the bbox is not correct because it is computed
   *  from the hull only) and contours do not vanish if shrinked below their own size but are reversed.
   *  This method is intended to be used in simple cases or as preparation step for a full-blown sizing
   *  algorithm using a merge step. 
   *  The sign of dx and dy should be identical.
   *
   *  The mode defines at which bending angle cutoff occurs
   *    0: >0
   *    1: >45
   *    2: >90
   *    3: >135
   *    4: >168 (approx)
   *    other: >179 (approx)
   */
  void size (coord_type dx, coord_type dy, unsigned int mode = 2)
  {
    for (typename contour_list_type::iterator c = m_ctrs.begin (); c != m_ctrs.end (); ++c) {
      c->size (dx, dy, mode);
    }
    m_bbox = m_ctrs [0].bbox ();
  }

  /**
   *  @brief Sizing with isotropic values, const version
   *
   *  @return the sized polygon
   */
  polygon sized (coord_type d, unsigned int mode = 2) const
  {
    polygon copy (*this);
    copy.size (d, mode);
    return copy;
  }

  /**
   *  @brief Sizing with anisotropic values, const version
   *
   *  @return the sized polygon
   */
  polygon sized (coord_type dx, coord_type dy, unsigned int mode = 2) const
  {
    polygon copy (*this);
    copy.size (dx, dy, mode);
    return copy;
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    db::mem_stat (stat, purpose, cat, m_ctrs, no_self, parent);
    db::mem_stat (stat, purpose, cat, m_bbox, no_self, parent);
  }

private:
  contour_list_type m_ctrs;
  db::box<C> m_bbox;

  //  Add a new hole entry to the end.
  //  This is done somewhat more intelligently here because
  //  we want to avoid excessive copying.
  contour_type &add_hole ()
  {
    if (m_ctrs.size () == m_ctrs.capacity ()) {

      //  if the capacity is less than expected, create 
      //  a new vector with twice as much elements and swap the elements
      contour_list_type new_holes;
      new_holes.reserve (m_ctrs.size () * 2);
      for (typename contour_list_type::iterator h = m_ctrs.begin (); h != m_ctrs.end (); ++h) {
        new_holes.push_back (contour_type ());
        h->swap (new_holes.back ()); 
      }
       
      //  swap the old and new holes list
      m_ctrs.swap (new_holes);

    } 

    //  add a new hole contour and return a reference to it
    m_ctrs.push_back (contour_type ());
    return m_ctrs.back ();
  }

};

/** 
 *  @brief A simple polygon class
 *
 *  A simple polygon consists of an outer hull only.
 *  The contour consists of several points. The point
 *  list is normalized such that the leftmost, lowest point is 
 *  the first one. The orientation is normalized such that
 *  the orientation of the hull contour is clockwise.
 *
 *  It is in no way checked that the contours are not over-
 *  lapping. This must be ensured by the user of the object
 *  when filling the contours.
 */

template <class C>
class DB_PUBLIC_TEMPLATE simple_polygon
{
public:
  typedef C coord_type;
  typedef db::edge<coord_type> edge_type;
  typedef db::simple_trans<coord_type> trans_type;
  typedef db::point<coord_type> point_type;
  typedef db::vector<coord_type> vector_type;
  typedef db::box<coord_type> box_type;
  typedef db::coord_traits<coord_type> coord_traits;
  typedef typename coord_traits::distance_type distance_type; 
  typedef typename coord_traits::perimeter_type perimeter_type; 
  typedef typename coord_traits::area_type area_type; 
  typedef polygon_contour<C> contour_type;
  typedef tl::vector<contour_type> contour_list_type;
  typedef db::polygon_edge_iterator< simple_polygon<C>, db::unit_trans<C> > polygon_edge_iterator;
  typedef db::polygon_contour_iterator< contour_type, db::unit_trans<C> > polygon_contour_iterator;
  typedef db::object_tag< simple_polygon<C> > tag;

  /**
   *  @brief The default constructor.
   *  
   *  The default constructor creates a empty polygon.
   */
  simple_polygon ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The box constructor.
   *  
   *  Creates a polygon from a box
   */
  explicit simple_polygon (const db::box<C> &b)
  {
    //  fill the contour
    point_type p[4];
    p[0] = point_type (b.left (), b.bottom ());
    p[1] = point_type (b.left (), b.top ());
    p[2] = point_type (b.right (), b.top ());
    p[3] = point_type (b.right (), b.bottom ());
    m_hull.assign (p, p + 4, false, false /*don't compress*/);
    m_bbox = b;
  }

  /**
   *  @brief The constructor from a polygon
   *  
   *  Creates a simple polygon from a polygon
   *  TODO: currently there is no treatment of holes!
   */
  explicit simple_polygon (const db::polygon<C> &p)
  {
    assign_hull (p.hull ());
    tl_assert (p.holes () == 0);
  }

  /**
   *  @brief The copy constructor from another polygon with a transformation
   *
   *  The transformation must have the ability to transform a point to another point of type C.
   *
   *  @param p The source polygon
   *  @param tr The transformation to apply
   *  @param compress true, if the sequence shall be compressed (colinear points removed)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   */
  template <class D, class T>
  simple_polygon (const db::simple_polygon<D> &p, const T &tr, bool compress = default_compression<C> (), bool remove_reflected = false)
  {
    //  create an entry for the hull contour
    m_bbox = box_type (tr (p.box ().p1 ()), tr (p.box ().p2 ()));
    m_hull.assign (p.begin_hull (), p.end_hull (), tr, false, compress, true /*normalize*/, remove_reflected);
  }

  /**
   *  @brief The copy constructor from another polygon with a different coordinate type
   *
   *  @param p The source polygon
   *  @param compress true, if the sequence shall be compressed (colinear points removed)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   */
  template <class D>
  explicit simple_polygon (const db::simple_polygon<D> &p, bool compress = default_compression<C> (), bool remove_reflected = false)
  {
    db::point_coord_converter<C, D> tr;

    //  create an entry for the hull contour
    m_bbox = box_type (tr (p.box ().p1 ()), tr (p.box ().p2 ()));
    m_hull.assign (p.begin_hull (), p.end_hull (), tr, false, compress, true /*normalize*/, remove_reflected);
  }

  /**
   *  @brief The (dummy) translation operator
   */
  void translate (const simple_polygon<C> &d, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
  }

  /**
   *  @brief The (dummy) translation operator with transformation
   */
  template <class T>
  void translate (const simple_polygon<C> &d, const T &t, db::generic_repository<C> &, db::ArrayRepository &)
  {
    *this = d;
    transform (t);
  }

  /**
   *  @brief A less operator to establish a sorting order.
   */
  bool operator< (const simple_polygon<C> &b) const
  {
    if (m_bbox < b.m_bbox) {
      return true;
    } else if (m_bbox != b.m_bbox) {
      return false;
    }

    return m_hull < b.m_hull;
  }

  /** 
   *  @brief Equality test
   */
  bool operator== (const simple_polygon<C> &b) const
  {
    return m_hull == b.m_hull;
  }

  /** 
   *  @brief Inequality test
   */
  bool operator!= (const simple_polygon<C> &b) const
  {
    return !operator== (b);
  }

  /**
   *  @brief A fuzzy less operator to establish a sorting order.
   */
  bool less (const simple_polygon<C> &b) const
  {
    if (m_bbox.less (b.m_bbox)) {
      return true;
    } else if (m_bbox.not_equal (b.m_bbox)) {
      return false;
    }

    return m_hull.less (b.m_hull);
  }

  /**
   *  @brief Fuzzy equality test
   */
  bool equal (const simple_polygon<C> &b) const
  {
    return m_hull.equal (b.m_hull);
  }

  /**
   *  @brief Fuzzy inequality test
   */
  bool not_equal (const simple_polygon<C> &b) const
  {
    return ! equal (b);
  }

  /**
   *  @brief The hull "begin" point iterator
   *
   *  The hull point sequence delivers the points that the hull
   *  is made of.
   */
  polygon_contour_iterator begin_hull () const
  {
    return polygon_contour_iterator (& hull (), 0);
  }

  /**
   *  @brief The hull "end" point iterator
   *
   *  The hull point sequence delivers the points that the hull
   *  is made of.
   */
  polygon_contour_iterator end_hull () const
  {
    return polygon_contour_iterator (& hull (), hull ().size ());
  }

  /**
   *  @brief The hole "begin" point iterator
   *
   *  The hole point sequence delivers the points that the specific hole
   *  is made of.
   */
  polygon_contour_iterator begin_hole (unsigned int h) const
  {
    return polygon_contour_iterator (& begin_holes () [h], 0);
  }

  /**
   *  @brief The hole "end" point iterator
   *
   *  The hole point sequence delivers the points that the specific hole
   *  is made of.
   */
  polygon_contour_iterator end_hole (unsigned int h) const
  {
    return polygon_contour_iterator (& begin_holes () [h], begin_holes () [h].size ());
  }

  /**
   *  @brief Compress the polygon.
   *
   *  Compressing means removing redundant points (identical, colinear)
   *
   *  @param remove_reflected Removes reflected edges (spiked) if this parameter is true.
   *  @return The compressed polygon.
   */
  simple_polygon<C> &compress (bool remove_reflected = false)
  {
    //  compress the polygon by employing the transform method 
    m_hull.transform (db::unit_trans<C> (), true, remove_reflected);
    return *this;
  }

  /**
   *  @brief Transform the polygon.
   *
   *  Transforms the polygon with the given transformation.
   *  Modifies the polygon with the transformed polygon.
   *  
   *  @param t The transformation to apply.
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   *
   *  @return The transformed polygon.
   */
  template <class Tr>
  simple_polygon<C> &transform (const Tr &t, bool compress = default_compression<C> (), bool remove_reflected = false)
  {
    m_hull.transform (t, compress, remove_reflected);

    //  transform or recompute the bbox
    if (t.is_ortho ()) {
      m_bbox.transform (t);
    } else {
      m_bbox = m_hull.bbox ();
    }

    return *this;
  }

  /**
   *  @brief Transform the polygon.
   *
   *  Transforms the polygon with the given transformation.
   *  Does not modify the polygon but returns the transformed polygon.
   *  
   *  @param t The transformation to apply.
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   *
   *  @return The transformed polygon.
   */
  template <class Tr>
  simple_polygon<typename Tr::target_coord_type> transformed (const Tr &t, bool compress = default_compression<typename Tr::target_coord_type> (), bool remove_reflected = false) const
  {
    typedef typename Tr::target_coord_type target_coord_type;
    simple_polygon<target_coord_type> poly;

    //  transform the contours and add as new ones
    poly.assign_hull (begin_hull (), end_hull (), t, compress, remove_reflected);

    return poly;
  }

  /**
   *  @brief Returns the moved polyon
   *
   *  Moves the polygon by the given offset and returns the 
   *  moved polygon. The polygon is not modified.
   *
   *  @param p The distance to move the polygon.
   * 
   *  @return The moved polygon.
   */
  simple_polygon<C> moved (const vector<C> &p) const
  {
    simple_polygon<C> b (*this);
    b.move (p);
    return b;
  }

  /**
   *  @brief Moves the polygon.
   *
   *  Moves the polygon by the given offset and returns the 
   *  moved polygon. The polygon is overwritten.
   *
   *  @param p The distance to move the polygon.
   * 
   *  @return The moved polygon.
   */
  simple_polygon<C> &move (const vector<C> &d)
  {
    m_bbox.move (d);
    m_hull.move (d);
    return *this;
  }

  /**
   *  @brief Returns the bounding box of the polygon
   */
  const db::box<C> &box () const
  {
    return m_bbox;
  }

  /**
   *  @brief Clears the polygon
   */
  void clear (unsigned int /*n*/ = 0) 
  {
    m_bbox = db::box<C> ();
    m_hull.clear ();
  }

  /** 
   *  @brief Set the outer contour from another contour
   *
   *  Replaces the outer contour by the given other contour
   *  The given contour must not be a hole contour and is_hole must be false.
   *
   *  @param other The other contour
   */
  void assign_hull (const contour_type &other)
  {
    tl_assert (! other.is_hole ());
    m_hull = other;
    m_bbox = m_hull.bbox ();
  }

  /** 
   *  @brief Set the outer contour
   *
   *  Replaces the outer contour by the points given by
   *  the sequence [start,end). This method will update the
   *  bounding box and normalize the hull, so it is oriented
   *  properly.
   *
   *  @param start The start of the sequence of points for the contour
   *  @param end The end of the sequence of points for the contour
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   */
  template <class I> 
  void assign_hull (I start, I end, bool compress = default_compression<C> (), bool remove_reflected = false) 
  {
    m_hull.assign (start, end, false, compress, true /*normalize*/, remove_reflected);
    m_bbox = m_hull.bbox ();
  }

  /** 
   *  @brief Set the outer contour by a transformed set of points
   *
   *  Replaces the outer contour by the points given by
   *  the sequence [start,end), transformed with the operator op. 
   *  This method will update the bounding box and normalize the hull, 
   *  so it is oriented properly.
   *
   *  @param start The start of the sequence of points for the contour
   *  @param end The end of the sequence of points for the contour
   *  @param compress true, if the sequence shall be compressed (colinear segments joined)
   *  @param remove_reflected True, if reflecting spikes shall be removed on compression
   */
  template <class I, class T> 
  void assign_hull (I start, I end, T op, bool compress = default_compression<C> (), bool remove_reflected = false) 
  {
    m_hull.assign (start, end, op, false, compress, true /*normalize*/, remove_reflected);
    m_bbox = m_hull.bbox ();
  }

  /**
   *  @brief Outer contour
   */
  const contour_type &hull () const
  {
    return m_hull;
  }

  /**
   *  @brief Returns true, if the polygon is a simple box
   */
  bool is_box () const
  {
    return m_hull.size () == 4 && m_hull.is_rectilinear ();
  }

  /**
   *  @brief Returns true, if the polygon is rectilinear
   */
  bool is_rectilinear () const
  {
    return m_hull.is_rectilinear ();
  }

  /**
   *  @brief Returns true, if the polygon is halfmanhattan
   */
  bool is_halfmanhattan () const
  {
    return m_hull.is_halfmanhattan ();
  }

  /**
   *  @brief The number of holes
   *
   *  This method is provided for compatibility with the standard polygon but does
   *  always return 0.
   */
  unsigned int holes () const
  {
    return 0;
  }

  /**
   *  @brief Hole iterator: start
   *
   *  This method is provided for compatibility with the standard polygon but does
   *  always return the begin iterator of an empty list.
   */
  typename contour_list_type::const_iterator begin_holes () const 
  {
    //  we exploit here the fact that the iterator of an empty vector is always 0
    contour_list_type ctrs;
    return ctrs.begin ();
  }

  /**
   *  @brief Hole iterator: end
   *
   *  This method is provided for compatibility with the standard polygon but does
   *  always return the end iterator of an empty list.
   */
  typename contour_list_type::const_iterator end_holes () const 
  {
    //  we exploit here the fact that the iterator of an empty vector is always 0
    contour_list_type ctrs;
    return ctrs.end ();
  }

  /** 
   *  @brief A dummy implementation of "insert_hole" provided for template instantiation
   *
   *  Asserts, if begin called.
   */
  template <class I> 
  void insert_hole (I, I, bool /*compress*/ = default_compression<C> ()) 
  {
    tl_assert (false);
  }

  /** 
   *  @brief A dummy implementation of "insert_hole" provided for template instantiation
   *
   *  Asserts, if begin called.
   */
  template <class I, class T> 
  void insert_hole (I, I, const T &, bool /*compress*/ = default_compression<C> ()) 
  {
    tl_assert (false);
  }

  /**
   *  @brief A dummy implementation of "sort_holes" provided for template instantiation
   *
   *  Asserts, if begin called.
   */
  void sort_holes ()
  {
    tl_assert (false);
  }

  /**
   *  @brief A dummy implementation of "hole" provided for template instantiation
   *
   *  Asserts, if begin called.
   */
  const contour_type &hole (unsigned int /*h*/) const
  {
    tl_assert (false);
    return hull (); // to please the compiler
  }

  /** 
   *  @brief The edge iterator begin function
   *
   *  The edge iterator delivers all edges of the polygon.
   *
   *  @return the begin value of the iterator
   */
  polygon_edge_iterator begin_edge () const 
  { 
    return polygon_edge_iterator (*this);
  }
  
  /** 
   *  @brief The area of the polygon
   */
  area_type area () const 
  {
    return m_hull.area ();
  }

  /**
   *  @brief The area of the polygon times 2
   *  For integer area types, this is the more precise value as the division
   *  by 2 might round off.
   */
  area_type area2 () const
  {
    return m_hull.area2 ();
  }

  /**
   *  @brief The perimeter of the polygon
   */
  perimeter_type perimeter () const 
  {
    return m_hull.perimeter ();
  }

  /**
   *  @brief The string conversion function
   */
  std::string to_string () const
  {
    std::string s = "(";

    //  the hull contour
    for (polygon_contour_iterator p = begin_hull (); p != end_hull (); ++p) {
      if (p != begin_hull ()) {
        s += ";";
      }
      s += (*p).to_string ();
    }

    s += ")";
    return s;
  }

  /**
   *  @brief Swap the polygon with another one
   * 
   *  The global std::swap function injected into the std namespace
   *  is redirected to this implementation.
   */
  void swap (simple_polygon<C> &d)
  {
    m_hull.swap (d.m_hull);
    std::swap (m_bbox, d.m_bbox);
  }

  /**
   *  @brief Direct access to the contours (used for iterators)
   */
  const contour_type &contour (unsigned int /*n*/) const
  {
    return m_hull;
  }

  /**
   *  @brief Reduce the polygon
   *
   *  Reduction of a polygon normalizes the polygon by extracting
   *  a suitable transformation and placing the polygon in a unique
   *  way.
   *
   *  @return The transformation that must be applied to render the original polygon
   */
  void reduce (simple_trans<coord_type> &tr)
  {
    if (m_hull.size () < 1) {
      tr = simple_trans<coord_type> ();
    } else {
      point_type d (m_hull [0]);
      move (-d);
      tr = simple_trans<coord_type> (simple_trans<coord_type>::r0, d);
    }
  }

  /**
   *  @brief Reduce the polygon
   *
   *  Reduction of a polygon normalizes the polygon by extracting
   *  a suitable transformation and placing the polygon in a unique
   *  way.
   *
   *  @return The transformation that must be applied to render the original polygon
   */
  void reduce (disp_trans<coord_type> &tr)
  {
    if (m_hull.size () < 1) {
      tr = disp_trans<coord_type> ();
    } else {
      vector_type d (m_hull [0]);
      move (-d);
      tr = disp_trans<coord_type> (d);
    }
  }

  /**
   *  @brief Reduce the polygon for unit transformation references
   *
   *  Does not do any reduction since no transformation can be provided.
   *
   *  @return A unit transformation
   */
  void reduce (unit_trans<C> &)
  {
    //  .. nothing ..
  }

  /**
   *  @brief Return the number of points in the polygon
   */
  size_t vertices () const
  {
    return m_hull.size ();
  }

  /**
   *  @brief Returns the area ratio between the polygon's bounding box and actual area
   *
   *  This number is a measure how well the polygon is approximated by the bounding box.
   *  Values are bigger than 1 for well-formed polygons. Bigger values mean worse
   *  approximation.
   */
  double area_ratio () const
  {
    area_type a = area2 ();
    if (a == 0) {
      //  By our definition, an empty polygon has an area ratio of 0
      return 0.0;
    } else {
      return double (box ().area ()) / (0.5 * a);
    }
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    db::mem_stat (stat, purpose, cat, m_hull, no_self, parent);
    db::mem_stat (stat, purpose, cat, m_bbox, no_self, parent);
  }

private:
  contour_type m_hull;
  db::box<C> m_bbox;
};

/** 
 *  @brief A polygon reference
 *
 *  A polygon reference is basically a proxy to a polygon and
 *  is used to implement polygon references with a repository.
 */

template <class Poly, class Trans>
class polygon_ref
  : public shape_ref<Poly, Trans>
{
public:
  typedef typename Poly::coord_type coord_type;
  typedef typename Poly::point_type point_type;
  typedef typename Poly::box_type box_type;
  typedef typename Poly::edge_type edge_type;
  typedef Trans trans_type;
  typedef Poly polygon_type;
  typedef db::polygon_edge_iterator<Poly, trans_type> polygon_edge_iterator;
  typedef db::polygon_contour_iterator<typename polygon_type::contour_type, trans_type> polygon_contour_iterator;
  typedef db::generic_repository<coord_type> repository_type;
  typedef typename Poly::distance_type distance_type;
  typedef typename Poly::perimeter_type perimeter_type; 
  typedef typename Poly::area_type area_type;
  typedef db::object_tag< polygon_ref<Poly, Trans> > tag;

  /**
   *  @brief The default constructor.
   *  
   *  The default constructor creates a invalid polygon reference
   */
  polygon_ref ()
    : shape_ref<Poly, Trans> ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor creating a reference from an actual polygon
   */
  polygon_ref (const polygon_type &p, repository_type &rep)
    : shape_ref<Poly, Trans> (p, rep)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor creating a reference from an polygon pointer and transformation
   *
   *  The polygon pointer passed is assumed to reside in a proper repository.
   */
  template <class TransIn>
  polygon_ref (const polygon_type *p, const TransIn &t)
    : shape_ref<Poly, Trans> (p, Trans (t))
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The translation constructor.
   *  
   *  This constructor allows one to copy a polygon reference from one
   *  repository to another
   */
  polygon_ref (const polygon_ref &ref, repository_type &rep)
    : shape_ref<Poly, Trans> (ref, rep)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The transformation translation constructor
   *  
   *  This constructor allows one to copy a polygon reference with a certain transformation
   *  to one with another transformation
   */
  template <class TransIn>
  polygon_ref (const polygon_ref<Poly, TransIn> &ref)
    : shape_ref<Poly, Trans> (ref.ptr (), Trans (ref.trans ()))
  {
    // .. nothing yet ..
  }

  /** 
   *  @brief The edge iterator 
   *
   *  The edge iterator delivers all edges of the polygon.
   *
   *  @return the begin value of the iterator
   */
  polygon_edge_iterator begin_edge () const 
  { 
    return polygon_edge_iterator (this->obj (), this->trans ());
  }
  
  /** 
   *  @brief The edge iterator for a given contour
   *
   *  The edge iterator delivers all edges of the polygon for the given contour.
   *  Contour 0 is the hull, 1 the first hole and so forth.
   *
   *  @return the begin value of the iterator
   */
  polygon_edge_iterator begin_edge (unsigned int ctr) const 
  { 
    return polygon_edge_iterator (this->obj (), ctr, this->trans ());
  }
  
  /** 
   *  @brief The hull iterator begin function
   */
  polygon_contour_iterator begin_hull () const 
  { 
    if (this->trans ().is_mirror ()) {
      return polygon_contour_iterator (--(this->obj ().end_hull ()), this->trans (), true);
    } else {
      return polygon_contour_iterator (this->obj ().begin_hull (), this->trans (), false);
    }
  }
  
  /** 
   *  @brief The hull iterator end function
   */
  polygon_contour_iterator end_hull () const 
  { 
    if (this->trans ().is_mirror ()) {
      return polygon_contour_iterator (--(this->obj ().begin_hull ()), this->trans (), true);
    } else {
      return polygon_contour_iterator (this->obj ().end_hull (), this->trans (), false);
    }
  }
  
  /** 
   *  @brief The hole iterator begin function
   */
  polygon_contour_iterator begin_hole (unsigned int h) const 
  { 
    if (this->trans ().is_mirror ()) {
      return polygon_contour_iterator (--(this->obj ().end_hole (h)), this->trans (), true);
    } else {
      return polygon_contour_iterator (this->obj ().begin_hole (h), this->trans (), false);
    }
  }
  
  /** 
   *  @brief The hull iterator end function
   */
  polygon_contour_iterator end_hole (unsigned int h) const 
  { 
    if (this->trans ().is_mirror ()) {
      return polygon_contour_iterator (--(this->obj ().begin_hole (h)), this->trans (), true);
    } else {
      return polygon_contour_iterator (this->obj ().end_hole (h), this->trans (), false);
    }
  }
  
  /**
   *  @brief The area ratio of the polygon
   */
  double area_ratio () const
  {
    return this->obj ().area_ratio ();
  }

  /**
   *  @brief The area of the polygon
   */
  area_type area () const 
  {
    return this->obj ().area ();
  }

  /**
   *  @brief The area of the polygon times 2
   *  For integer area types, this is the more precise value as the division
   *  by 2 might round off.
   */
  area_type area2 () const
  {
    return this->obj ().area2 ();
  }

  /**
   *  @brief The perimeter of the polygon
   */
  perimeter_type perimeter () const 
  {
    return this->obj ().perimeter ();
  }

  /**
   *  @brief Returns a value indicating whether the polygon is a box
   */
  bool is_box () const
  {
    return this->obj ().is_box ();
  }

  /**
   *  @brief Returns a value indicating whether the polygon is rectilinear
   */
  bool is_rectilinear () const
  {
    return this->obj ().is_rectilinear ();
  }

  /**
   *  @brief Returns the number of vertices
   */
  size_t vertices () const
  {
    return this->obj ().vertices ();
  }

  /** 
   *  @brief Return the transformed object
   * 
   *  This version does not change the object and is const.
   */
  template <class TargetTrans>
  polygon_ref<Poly, TargetTrans> transformed (const TargetTrans &t) const
  {
    polygon_ref<Poly, TargetTrans> pref (*this);
    pref.transform (t);
    return pref;
  }
};

/**
 *  @brief Binary * operator (transformation)
 *
 *  Transforms the polygon reference with the given transformation and 
 *  returns the result.
 *
 *  @param t The transformation to apply
 *  @param p The polygon reference to transform
 *  @return t * p
 */
template <class Poly, class Tr, class TargetTr>
inline polygon_ref<Poly, TargetTr>
operator* (const TargetTr &t, const polygon_ref<Poly, Tr> &p)
{
  return p.transformed (t);
}

/**
 *  @brief Binary * operator (transformation)
 *
 *  Transforms the polygon with the given transformation and 
 *  returns the result.
 *
 *  @param t The transformation to apply
 *  @param p The polygon to transform
 *  @return t * p
 */
template <class Tr>
inline polygon<typename Tr::target_coord_type> 
operator* (const Tr &t, const polygon<typename Tr::coord_type> &p)
{
  return p.transformed (t);
}

/**
 *  @brief Binary * operator (transformation)
 *
 *  Transforms the simple polygon with the given transformation and 
 *  returns the result.
 *
 *  @param t The transformation to apply
 *  @param p The polygon to transform
 *  @return t * p
 */
template <class Tr>
inline simple_polygon<typename Tr::target_coord_type> 
operator* (const Tr &t, const simple_polygon<typename Tr::coord_type> &p)
{
  return p.transformed (t);
}

/**
 *  @brief Binary * operator (scaling)
 *
 *  @param p The polygon to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled polygon
 */
template <class C>
inline polygon<double>
operator* (const polygon<C> &p, double s)
{
  db::complex_trans<C, double> ct (s);
  return ct * p;
}

/**
 *  @brief Binary * operator (scaling)
 *
 *  @param p The polygon to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled polygon
 */
template <class C>
inline simple_polygon<double>
operator* (const simple_polygon<C> &p, double s)
{
  db::complex_trans<C, double> ct (s);
  return ct * p;
}

/**
 *  @brief Inside predicate 
 *
 *  This template function returns 1, if the point is inside (not on)
 *  the polygon. It returns 0, if the point is on the polygon and -1
 *  if outside. It is made into a template in order to be able to operate
 *  on every kind of edge iterator.
 *
 *  @param edge The edge iterator of the polygon
 *  @param pt The point to test
 */

template<class Iter, class Point>
int inside_poly (Iter edge, const Point &pt)
{
  int wrapcount_left = 0;

  while (! edge.at_end ()) {
    if ((*edge).p1 ().y () <= pt.y () && (*edge).p2 ().y () > pt.y ()) {
      int side = (*edge).side_of (pt);
      if (side < 0) { 
        ++wrapcount_left;
      } else if (side == 0) {
        //  "on" the line is excluded in the predicate
        return 0;
      }
    } else if ((*edge).p2 ().y () <= pt.y () && (*edge).p1 ().y () > pt.y ()) {
      int side = (*edge).side_of (pt);
      if (side > 0) { 
        --wrapcount_left;
      } else if (side == 0) {
        //  "on" the line is excluded in the predicate
        return 0;
      }
    } else if ((*edge).p1 ().y () == pt.y () && (*edge).p2 ().y () == pt.y () &&
               (((*edge).p1 ().x () <= pt.x () && (*edge).p2 ().x () >= pt.x ()) ||
                ((*edge).p2 ().x () <= pt.x () && (*edge).p1 ().x () >= pt.x ()))) {
      //  "on" the horizontal line is excluded in the predicate
      return 0;
    }
    ++edge;
  }

  return (wrapcount_left != 0) ? 1 : -1;
}

/**
 *  @brief Output stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const polygon<C> &p)
{
  return (os << p.to_string ());
}

/**
 *  @brief Output stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const simple_polygon<C> &p)
{
  return (os << p.to_string ());
}

/**
 *  @brief The standard polygon typedef
 */
typedef polygon<db::Coord> Polygon;

/**
 *  @brief The double coordinate polygon typedef
 */
typedef polygon<db::DCoord> DPolygon;

/**
 *  @brief The simple polygon typedef
 */
typedef simple_polygon<db::Coord> SimplePolygon;

/**
 *  @brief The double coordinate simple polygon typedef
 */
typedef simple_polygon<db::DCoord> DSimplePolygon;

/**
 *  @brief The standard polygon reference typedef
 */
typedef polygon_ref<Polygon, Disp> PolygonRef;

/**
 *  @brief The double coordinate polygon reference typedef
 */
typedef polygon_ref<DPolygon, DDisp> DPolygonRef;

/**
 *  @brief The simple polygon reference typedef
 */
typedef polygon_ref<SimplePolygon, Disp> SimplePolygonRef;

/**
 *  @brief The double coordinate simple polygon reference typedef
 */
typedef polygon_ref<DSimplePolygon, DDisp> DSimplePolygonRef;

/**
 *  @brief The standard polygon reference (without transformation) typedef
 */
typedef polygon_ref<Polygon, UnitTrans> PolygonPtr;

/**
 *  @brief The double coordinate polygon reference (without transformation) typedef
 */
typedef polygon_ref<DPolygon, DUnitTrans> DPolygonPtr;

/**
 *  @brief The simple polygon reference (without transformation) typedef
 */
typedef polygon_ref<SimplePolygon, UnitTrans> SimplePolygonPtr;

/**
 *  @brief The double coordinate simple polygon reference (without transformation) typedef
 */
typedef polygon_ref<DSimplePolygon, DUnitTrans> DSimplePolygonPtr;

/**
 *  @brief Collect memory statistics
 */
template <class X>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const polygon_contour<X> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief Collect memory statistics
 */
template <class X>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const polygon<X> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief Collect memory statistics
 */
template <class X>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const simple_polygon<X> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

} // namespace db

namespace std
{

//  injecting a global std::swap for polygons into the
//  std namespace
template <class C>
void swap (db::polygon<C> &a, db::polygon<C> &b)
{
  a.swap (b);
}

//  injecting a global std::swap for polygons into the 
//  std namespace
template <class C>
void swap (db::simple_polygon<C> &a, db::simple_polygon<C> &b)
{
  a.swap (b);
}

} // namespace std

namespace tl 
{
  /**
   *  @brief Special extractors for the polygons
   */
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Polygon &p);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DPolygon &p);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::SimplePolygon &p);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::DSimplePolygon &p);

  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Polygon &p);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DPolygon &p);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::SimplePolygon &p);
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::DSimplePolygon &p);

} // namespace tl


#endif



/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#ifndef HDR_dbEdgesToContours
#define HDR_dbEdgesToContours

#include "dbPoint.h"
#include "dbEdge.h"
#include "dbVector.h"
#include "tlProgress.h"

#include <vector>
#include <algorithm>
#include <limits>

namespace db {

class DB_PUBLIC ECRef
{
public:
  template <class Iter>
  ECRef (Iter from, Iter e, bool swap)
  {
    m_index = ((long) std::distance (from, e)) + 1;
    if (swap) {
      m_index = -m_index;
    }
  }

  ECRef ()
    : m_index (0)
  {
    // .. nothing yet ..
  }

  ECRef (long i)
    : m_index (i)
  {
    // .. nothing yet ..
  }

  bool is_null () const
  {
    return m_index == 0;
  }

  bool operator!= (ECRef b)
  {
    return m_index != b.m_index;
  }

  bool operator== (ECRef b)
  {
    return m_index == b.m_index;
  }

  template <class Iter>
  const Point &a (Iter from) const
  {
    if (m_index > 0) {
      return from [m_index - 1].p1 ();
    } else {
      return from [-m_index - 1].p2 ();
    }
  }

  template <class Iter>
  const Point &b (Iter from) const
  {
    if (m_index > 0) {
      return from [m_index - 1].p2 ();
    } else {
      return from [-m_index - 1].p1 ();
    }
  }

  ECRef reverse () const
  {
    return ECRef (-m_index);
  }

  size_t index () const
  {
    return (size_t)((m_index < 0 ? -m_index : m_index) - 1);
  }

  template <class Iter>
  Vector d (Iter from) const
  {
    if (m_index > 0) {
      return from [m_index - 1].d ();
    } else {
      return -(from [-m_index - 1].d ());
    }
  }

  static ECRef invalid () 
  {
    return ECRef (std::numeric_limits <long>::min ());
  }

private:
  long m_index;
};

template <class Iter>
class ECLess 
{
public:
  ECLess (Iter from)
    : m_from (from)
  {
    // .. nothing yet ..
  }

  bool operator() (ECRef p, ECRef q) const
  {
    return p.a (m_from) < q.a (m_from);
  }

private:
  Iter m_from;
};

/**
 *  @brief A facility to create contours from edges
 *  
 *  This object will convert a set of edges to contours. "contours" are sequences of points, not
 *  necessarily closed ones. Contours may also be holes or outer contours - this object is not
 *  capable of making that distinction.
 *
 *  The use of this objects is to first fill it with edges and then deliver the contours
 *  collected in the fill step.
 */
class EdgesToContours 
{
public:
  EdgesToContours ()
  {
    // .. nothing yet ..
  }

  size_t contours () const
  {
    return m_contours.size ();
  }

  const std::vector <db::Point> &contour (size_t i) const
  {
    return m_contours [i];
  }

  template <class Iter>
  void fill (Iter from, Iter to, bool no = false, tl::RelativeProgress *progress = 0)
  {
    m_contours.clear ();

    std::vector <ECRef> ptmap;
    ptmap.reserve ((size_t)(no ? 2 : 1) * std::distance (from, to));

    for (Iter e = from; e != to; ++e) {
      ptmap.push_back (ECRef (from, e, false));
      if (no) {
        ptmap.push_back (ECRef (from, e, true));
      }
    }

    std::sort (ptmap.begin (), ptmap.end (), ECLess<Iter> (from));

    std::vector <ECRef> cmap;
    cmap.resize (std::distance (from, to), ECRef::invalid ());

    for (std::vector <ECRef>::iterator s0 = cmap.begin (); s0 != cmap.end (); ++s0) {

      if (*s0 != ECRef::invalid ()) {
        continue;
      }

      m_contours.push_back (std::vector <db::Point> ());

      std::vector <ECRef>::iterator s = s0;

      ECRef fr (from, from + std::distance (cmap.begin (), s0), false);
      while (! fr.is_null ()) {

        std::vector <ECRef>::iterator s_old = s;
        *s_old = ECRef ();
        double vp_min = 0.0;
        ECRef fr_old = fr;
        fr = ECRef ();

        std::vector <ECRef>::const_iterator f = std::lower_bound (ptmap.begin (), ptmap.end (), fr_old.reverse (), ECLess<Iter> (from));
        while (f != ptmap.end () && f->a (from) == fr_old.b (from)) {

          if (progress) {
            ++*progress;
          }

          std::vector <ECRef>::iterator s_new = cmap.begin () + f->index ();
          if (*s_new == ECRef::invalid ()) {

            double vp = db::vprod (f->d (from), fr_old.d (from)) * (1.0 / f->d (from).double_length ());
            if (fr.is_null () || vp < vp_min) {
              vp_min = vp;
              fr = *f;
              s = s_new; 
              *s_old = *f;
            }

          }

          ++f;

        }

        if (progress) {
          ++*progress;
        }

      }

      //  create the contour

      size_t n = 2;
      s = s0;
      while (! s->is_null ()) {
        s = cmap.begin () + s->index ();
        ++n;
      }

      m_contours.back ().reserve (n);

      const db::Edge e0 (from [std::distance (cmap.begin (), s0)]);
      m_contours.back ().push_back (e0.p1 ());
      m_contours.back ().push_back (e0.p2 ());

      s = s0;

      while (! s->is_null ()) {
        m_contours.back ().push_back (s->b (from));
        s = cmap.begin () + s->index ();
        ++n;
      }

    }

  }

private:
  std::vector <std::vector <db::Point> > m_contours;
};

} // namespace db

#endif


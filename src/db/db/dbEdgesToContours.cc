
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


#include "dbEdgesToContours.h"
#include "dbBoxTree.h"
#include "dbBoxConvert.h"
#include "tlProgress.h"

#include <vector>
#include <algorithm>
#include <limits>

namespace
{

template<class Iter>
class EdgeRef
{
public:
  typedef typename std::iterator_traits<Iter>::value_type value_type;
  typedef typename value_type::coord_type coord_type;
  typedef db::point<coord_type> point_type;

  EdgeRef (Iter i)
    : iter (i), swapped (0), connected (false), delivered (false), seen (false), next (0)
  { }

  Iter iter;
  short swapped;
  bool connected : 1;
  bool delivered : 1;
  bool seen : 1;
  EdgeRef<Iter> *next;

  point_type a () const
  {
    return swapped > 0 ? iter->p2 () : iter->p1 ();
  }

  point_type b () const
  {
    return swapped > 0 ? iter->p1 () : iter->p2 ();
  }
};

template<class Iter, bool swapped>
struct EdgeRefToBox
{
public:
  typedef typename std::iterator_traits<Iter>::value_type value_type;
  typedef typename value_type::coord_type coord_type;
  typedef db::box<coord_type> box_type;
  typedef db::simple_bbox_tag complexity;

  EdgeRefToBox (coord_type d)
    : distance (d)
  { }

  box_type operator() (EdgeRef<Iter> *er) const
  {
    db::point<coord_type> p = swapped ? er->iter->p2 () : er->iter->p1 ();
    db::vector<coord_type> d (distance, distance);
    return box_type (p - d, p + d);
  }

  coord_type distance;
};

}

namespace db
{

// -----------------------------------------------------------------------------
//  EdgesToContours implementation

EdgesToContours::EdgesToContours ()
{
  // .. nothing yet ..
}

const std::vector<db::Point> &
EdgesToContours::contour (size_t i) const
{
  static std::vector<db::Point> empty;
  if (i < m_contours.size ()) {
    return m_contours [i];
  } else {
    return empty;
  }
}

namespace {

template <class C>
class point_matcher
{
public:

  point_matcher () : m_vp_min (0.0), m_d_min (0.0), m_any (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief A search criterion for fitting next edges for a point (with attached edge)
   *  This search will select the edge whose starting point is closest to the
   *  end point of the reference edge and - if both points are coincident - forms
   *  the smallest angle with the reference edge.
   */
  bool more (const db::point<C> &p, const db::edge<C> &e, const db::edge<C> &other, bool swapped)
  {
    typedef db::coord_traits<C> coord_traits;

    double d = p.double_distance (swapped ? other.p2 () : other.p1 ());
    double vp = db::vprod (other.d (), e.d ()) * (1.0 / other.d ().double_length ());

    if (! m_any) {

      m_vp_min = vp;
      m_d_min = d;
      m_any = true;
      return true;

    } else if (fabs (d - m_d_min) < coord_traits::prec ()) {

      double vp = db::vprod (other.d (), e.d ()) * (1.0 / other.d ().double_length ());
      if (vp < m_vp_min) {
        m_vp_min = vp;
        return true;
      } else {
        return false;
      }

    } else if (d < m_d_min) {

      m_vp_min = vp;
      m_d_min = d;
      return true;

    } else {
      return false;
    }
  }

private:
  double m_vp_min;
  double m_d_min;
  bool m_any;
};

}

template <class Iter, class C> static
EdgeRef<Iter> *search_follower (const db::point<C> &p, const EdgeRef<Iter> *e, C distance, const db::box_tree<db::box<C>, EdgeRef<Iter> *, EdgeRefToBox<Iter, false> > &t1, const db::box_tree<db::box<C>, EdgeRef<Iter> *, EdgeRefToBox<Iter, true> > &t2)
{
  typedef db::box<C> box_type;

  EdgeRef<Iter> *cand = 0;
  bool fwd = true;
  point_matcher<C> pm;

  //  try in forward tree

  typename db::box_tree<box_type, EdgeRef<Iter> *, EdgeRefToBox<Iter, false> >::touching_iterator f = t1.begin_touching (box_type (p, p), EdgeRefToBox <Iter, false> (distance));
  while (! f.at_end ()) {
    if (*f != e && ! (*f)->connected && (*f)->swapped != 1 && pm.more (p, *e->iter, *(*f)->iter, false)) {
      cand = *f;
    }
    ++f;
  }

  if (! t2.empty ()) {
    typename db::box_tree<box_type, EdgeRef<Iter> *, EdgeRefToBox<Iter, true> >::touching_iterator f = t2.begin_touching (box_type (p, p), EdgeRefToBox <Iter, true> (distance));
    while (! f.at_end ()) {
      if (*f != e && ! (*f)->connected && (*f)->swapped != -1 && pm.more (p, *e->iter, *(*f)->iter, true)) {
        cand = *f;
        fwd = false;
      }
      ++f;
    }
  }

  if (cand) {
    cand->swapped = fwd ? -1 : 1;
    cand->connected = true;
  }

  return cand;
}

template <class Iter>
void
EdgesToContours::fill (Iter from, Iter to, bool no, typename std::iterator_traits<Iter>::value_type::coord_type distance, tl::RelativeProgress *progress)
{
  typedef typename std::iterator_traits<Iter>::value_type value_type;
  typedef typename value_type::coord_type coord_type;
  typedef db::box<coord_type> box_type;
  typedef db::point<coord_type> point_type;
  typedef db::box_tree<box_type, EdgeRef<Iter> *, EdgeRefToBox<Iter, false> > box_tree;
  typedef db::box_tree<box_type, EdgeRef<Iter> *, EdgeRefToBox<Iter, true> > box_tree_rev;

  std::vector<EdgeRef<Iter> > erefs;
  erefs.reserve (to - from);

  for (Iter i = from; i < to; ++i) {
    erefs.push_back (EdgeRef<Iter> (i));
  }

  //  prepare two box trees: one forward (with p1 being the key)
  //  and a backward one with p2 being the key

  box_tree bt;
  box_tree_rev btr;

  bt.reserve (erefs.size ());
  for (typename std::vector<EdgeRef<Iter> >::iterator e = erefs.begin (); e != erefs.end (); ++e) {
    bt.insert (e.operator-> ());
  }
  bt.sort (EdgeRefToBox <Iter, false> (distance));

  if (no) {
    btr.reserve (erefs.size ());
    for (typename std::vector<EdgeRef<Iter> >::iterator e = erefs.begin (); e != erefs.end (); ++e) {
      btr.insert (e.operator-> ());
    }
    btr.sort (EdgeRefToBox <Iter, true> (distance));
  }

  //  Build the edge dependency graph (e->next being the following one)

  for (typename std::vector<EdgeRef<Iter> >::iterator e = erefs.begin (); e != erefs.end (); ++e) {

    EdgeRef<Iter> *ee = e.operator-> ();
    while (ee && !ee->seen) {

      if (progress) {
        ++*progress;
      }

      ee->seen = true;

      EdgeRef<Iter> *f1 = 0, *f2 = 0;
      if (ee->swapped != 1) {
        f1 = search_follower (ee->iter->p2 (), ee, distance, bt, btr);
      }
      if (! f1 && ee->swapped != -1 && no) {
        f2 = search_follower (ee->iter->p1 (), ee, distance, bt, btr);
      }

      if (f1) {
        ee->swapped = -1;
        ee->next = f1;
      } else if (f2) {
        ee->swapped = 1;
        ee->next = f2;
      }

      ee = ee->next;

    }

  }

  //  Delivery

  m_contours.clear ();
  m_contours_closed.clear ();

  //  Extract the open contours

  for (typename std::vector<EdgeRef<Iter> >::iterator e = erefs.begin (); e != erefs.end (); ++e) {

    if (progress) {
      ++*progress;
    }

    if (! e->delivered && ! e->connected) {

      m_contours.push_back (std::vector<point_type> ());
      m_contours_closed.push_back (false);

      m_contours.back ().push_back (e->a ());
      EdgeRef<Iter> *ee = e.operator-> ();
      while (ee) {
        tl_assert (! ee->delivered);
        m_contours.back ().push_back (ee->b ());
        ee->delivered = true;
        ee = ee->next;
      }

    }

  }

  //  Extract the closed contours

  for (typename std::vector<EdgeRef<Iter> >::iterator e = erefs.begin (); e != erefs.end (); ++e) {

    if (progress) {
      ++*progress;
    }

    if (! e->delivered) {

      m_contours.push_back (std::vector<point_type> ());
      m_contours_closed.push_back (true);

      EdgeRef<Iter> *ee = e.operator-> ();
      while (ee && ! ee->delivered) {
        m_contours.back ().push_back (ee->b ());
        ee->delivered = true;
        ee = ee->next;
      }

    }

  }
}

//  explicit instantiation
template DB_PUBLIC void EdgesToContours::fill (std::vector<db::Edge>::iterator, std::vector<db::Edge>::iterator, bool, db::Coord, tl::RelativeProgress *);
template DB_PUBLIC void EdgesToContours::fill (db::Edge *, db::Edge *, bool, db::Coord, tl::RelativeProgress *);

} // namespace db


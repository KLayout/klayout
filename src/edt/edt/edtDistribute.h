/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#ifndef HDR_edtDistribute
#define HDR_edtDistribute

#include "dbBox.h"

#include "dbTypes.h"
#include "tlIntervalMap.h"

namespace edt
{

/**
 *  @brief Gets the box position by reference position
 */
template <class Box, bool horizontally>
typename Box::coord_type box_position (const Box &box, int ref)
{
  if (horizontally) {
    if (ref < 0) {
      return box.left ();
    } else if (ref == 0) {
      return box.center ().x ();
    } else {
      return box.right ();
    }
  } else {
    if (ref < 0) {
      return box.bottom ();
    } else if (ref == 0) {
      return box.center ().y ();
    } else {
      return box.top ();
    }
  }
}

/**
 *  @brief Compares boxes by their reference position
 */
template <class Box, class Value, bool horizontally>
class box_compare
{
public:
  typedef typename Box::coord_type coord_type;

  box_compare (int ref)
    : m_ref (ref)
  {
    //  .. nothing yet ..
  }

  bool operator() (const std::pair<Box, Value> &a, const std::pair<Box, Value> &b) const
  {
    coord_type ca = box_position<Box, horizontally> (a.first, m_ref);
    coord_type cb = box_position<Box, horizontally> (b.first, m_ref);

    if (! db::coord_traits<coord_type>::equal (ca, cb)) {
      return db::coord_traits<coord_type>::less (ca, cb);
    } else {
      coord_type ca2 = box_position<Box, !horizontally> (a.first, m_ref);
      coord_type cb2 = box_position<Box, !horizontally> (b.first, m_ref);
      return db::coord_traits<coord_type>::less (ca2, cb2);
    }
  }

private:
  int m_ref;
};

/**
 *  @brief Does some heuristic binning of coordinates
 */
template <class Box, bool horizontally>
void do_bin (typename std::vector<std::pair<Box, size_t> >::const_iterator b, typename std::vector<std::pair<Box, size_t> >::const_iterator e, int ref, std::vector<std::vector<size_t> > &bins)
{
  typedef typename Box::coord_type coord_type;

  //  determine maximum distance between adjacent coordinates

  coord_type max_dist = 0;
  for (typename std::vector<std::pair<Box, size_t> >::const_iterator i = b + 1; i != e; ++i) {
    max_dist = std::max (max_dist, box_position<Box, horizontally> (i->first, ref) - box_position<Box, horizontally> ((i - 1)->first, ref));
  }

  //  heuristically, everything that has a distance of less than 1/3 of the maximum distance falls into one bin

  coord_type bin_start = box_position<Box, horizontally> (b->first, ref);
  bins.push_back (std::vector<size_t> ());
  bins.back ().push_back (b->second);

  coord_type thr = max_dist / 3;

  for (typename std::vector<std::pair<Box, size_t> >::const_iterator i = b + 1; i != e; ++i) {
    coord_type c = box_position<Box, horizontally> (i->first, ref);
    if (c - bin_start > thr) {
      //  start a new bin
      bins.push_back (std::vector<size_t> ());
      bin_start = c;
    }
    bins.back ().push_back (i->second);
  }
}

template <class Coord>
struct max_coord_join_op
{
  void operator() (Coord &a, const Coord &b) const
  {
    a = std::max (a, b);
  }
};

/**
 *  @brief Computes the actual row/columns positions starting from 0
 *  The positions are the "low" side positions of the boxes (ref = -1)
 */
template <class Box, class Objects, bool horizontally>
void compute_positions (int ref, typename Box::coord_type pitch, typename Box::coord_type space, std::vector<std::vector<size_t> > &bins, Objects &objects)
{
  typedef typename Box::coord_type coord_type;

  tl::interval_map<coord_type, coord_type> limits;

  coord_type min, max;
  bool first = true;
  for (typename Objects::const_iterator o = objects.begin (); o != objects.end (); ++o) {
    coord_type b1 = box_position<Box, ! horizontally> (o->first, -1);
    coord_type b2 = box_position<Box, ! horizontally> (o->first, 1);
    if (first) {
      min = b1;
      max = b2;
    } else {
      min = std::min (min, b1);
      max = std::max (max, b1);
    }
  }

  max_coord_join_op<coord_type> join_op;

  limits.add (min, max, (coord_type) 0, join_op);

  //  Determines the next column/row's position as the minimum position which is compatible with
  //  the space constraint.

  for (std::vector<std::vector<size_t> >::const_iterator b = bins.begin (); b != bins.end (); ++b) {

    coord_type min_pos = 1;

    for (std::vector<size_t>::const_iterator i = b->begin (); i != b->end (); ++i) {

      const Box &box = objects [*i].first;

      coord_type b1 = box_position<Box, ! horizontally> (box, -1);
      coord_type b2 = box_position<Box, ! horizontally> (box, 1);

      coord_type start = box_position<Box, horizontally> (box, -1);
      coord_type ref_pos = box_position<Box, horizontally> (box, ref);

      for (typename tl::interval_map<coord_type, coord_type>::const_iterator j = limits.find (b1); j != limits.end () && db::coord_traits<coord_type>::less (j->first.first, b2); ++j) {
        min_pos = std::max (min_pos, j->second + space + (ref_pos - start));
      }

    }

    if (pitch > 0) {
      min_pos = db::coord_traits<coord_type>::rounded (ceil (double (min_pos) / double (pitch) - 1e-10) * double (pitch));
    }

    for (std::vector<size_t>::const_iterator i = b->begin (); i != b->end (); ++i) {

      Box &box = objects [*i].first;

      coord_type b1 = box_position<Box, ! horizontally> (box, -1);
      coord_type b2 = box_position<Box, ! horizontally> (box, 1);

      coord_type ref_pos = box_position<Box, horizontally> (box, ref);
      coord_type end_pos = box_position<Box, horizontally> (box, 1);

      if (horizontally) {
        box.move (db::vector<coord_type> (min_pos - ref_pos, 0));
      } else {
        box.move (db::vector<coord_type> (0, min_pos - ref_pos));
      }

      limits.add (b1, b2, min_pos + (end_pos - ref_pos), join_op);

    }

  }
}

/**
 *  @brief Implements an algorithm for 2d-distributing rectangular objects
 */
template <class Box, class Value>
class distributed_placer
{
public:
  typedef typename Box::coord_type coord_type;
  typedef std::vector<std::pair<Box, Value> > objects;
  typedef typename objects::const_iterator iterator;

  /**
   *  @brief Constructor
   */
  distributed_placer ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Reserves space for n objects
   */
  void reserve (size_t n)
  {
    m_objects.reserve (n);
  }

  /**
   *  @brief Inserts a new object
   */
  void insert (const Box &box, const Value &value)
  {
    tl_assert (! box.empty ());
    m_objects.push_back (std::make_pair (box, value));
  }

  /**
   *  @brief Stored objects iterator: begin
   */
  iterator begin () const
  {
    return m_objects.begin ();
  }

  /**
   *  @brief Stored objects iterator: end
   */
  iterator end () const
  {
    return m_objects.end ();
  }

  /**
   *  @brief Distributes the stored objects in vertical direction only
   *
   *  @param ref The reference location (-1: bottom, 0: center, 1: top)
   *  @param pitch The distribution pitch (grid) or 0 for no pitch
   *  @param space The minimum space between the objects
   */
  void distribute_v (int ref, coord_type pitch, coord_type space)
  {
    do_distribute_1d<false> (ref, pitch, space);
  }

  /**
   *  @brief Distributes the stored objects in horizontal direction only
   *
   *  @param ref The reference location (-1: left, 0: center, 1: right)
   *  @param pitch The distribution pitch (grid) or 0 for no pitch
   *  @param space The minimum space between the objects
   */
  void distribute_h (int ref, coord_type pitch, coord_type space)
  {
    do_distribute_1d<true> (ref, pitch, space);
  }

  /**
   *  @brief Distributes the stored objects in horizontal and vertical direction
   *
   *  @param href The horizontal reference location (-1: left, 0: center, 1: right)
   *  @param hpitch The horizontal distribution pitch (grid) or 0 for no pitch
   *  @param hspace The horizontal minimum space between the objects
   *  @param vref The vertical reference location (-1: bottom, 0: center, 1: top)
   *  @param vpitch The vertical distribution pitch (grid) or 0 for no pitch
   *  @param vspace The vertical minimum space between the objects
   */
  void distribute_matrix (int href, coord_type hpitch, coord_type hspace, int vref, coord_type vpitch, coord_type vspace)
  {
    if (m_objects.size () < 2) {
      return;
    }

    //  The algorithm is this:
    //  1.) Bin the boxes according to their positions in horizontal and vertical direction.
    //      This forms the potential columns and rows
    //  2.) Compute the actual column and row positions by applying space and pitch constraints:
    //      horizontally first, then vertically (TODO: we could try both ways and test which one is better)

    std::vector<std::pair<Box, size_t> > indexed_boxes;
    indexed_boxes.reserve (m_objects.size ());

    Box all;
    for (typename objects::iterator i = m_objects.begin (); i != m_objects.end (); ++i) {
      all += i->first;
    }

    size_t n = 0;
    for (typename objects::iterator i = m_objects.begin (); i != m_objects.end (); ++i, ++n) {
      indexed_boxes.push_back (std::make_pair (i->first, n));
    }

    std::vector<std::vector<size_t> > hbins, vbins;

    std::sort (indexed_boxes.begin (), indexed_boxes.end (), box_compare<Box, size_t, true> (href));
    do_bin<Box, true> (indexed_boxes.begin (), indexed_boxes.end (), href, hbins);

    std::sort (indexed_boxes.begin (), indexed_boxes.end (), box_compare<Box, size_t, false> (vref));
    do_bin<Box, false> (indexed_boxes.begin (), indexed_boxes.end (), vref, vbins);

    compute_positions<Box, objects, true> (href, hpitch, hspace, hbins, m_objects);
    compute_positions<Box, objects, false> (vref, vpitch, vspace, vbins, m_objects);

    //  Final adjustments

    Box new_all;
    for (typename objects::iterator i = m_objects.begin (); i != m_objects.end (); ++i, ++n) {
      new_all += i->first;
    }

    coord_type dh = box_position<Box, true> (all, href) - box_position<Box, true> (new_all, href);
    coord_type dv = box_position<Box, false> (all, vref) - box_position<Box, false> (new_all, vref);
    db::vector<coord_type> mv (dh, dv);

    for (typename objects::iterator i = m_objects.begin (); i != m_objects.end (); ++i) {
      i->first.move (mv);
    }
  }

private:
  objects m_objects;

  template <bool horizontally>
  void do_distribute_1d (int ref, coord_type pitch, coord_type space)
  {
    if (m_objects.size () < 2) {
      return;
    }

    Box all;
    for (typename objects::const_iterator i = m_objects.begin () + 1; i != m_objects.end (); ++i) {
      all += i->first;
    }

    std::sort (m_objects.begin (), m_objects.end (), box_compare<Box, Value, horizontally> (ref));

    Box current = m_objects.front ().first;
    coord_type p0 = box_position<Box, horizontally> (current, ref);

    for (typename objects::iterator i = m_objects.begin () + 1; i != m_objects.end (); ++i) {

      coord_type p = box_position<Box, horizontally> (i->first, -1);
      coord_type offset = box_position<Box, horizontally> (i->first, ref) - p;
      coord_type pnew = box_position<Box, horizontally> (current, 1) + space;

      if (db::coord_traits<coord_type>::less (0, pitch)) {
        pnew = coord_type (ceil (double (pnew + offset - p0) / double (pitch) - 1e-10)) * pitch - offset;
      }

      db::vector<coord_type> mv;
      if (horizontally) {
        mv = db::vector<coord_type> (pnew - p, 0);
      } else {
        mv = db::vector<coord_type> (0, pnew - p);
      }

      i->first.move (mv);
      current = i->first;

    }

    //  final adjustment
    Box new_all = m_objects.front ().first + m_objects.back ().first;

    db::vector<coord_type> mv;
    coord_type d = box_position<Box, horizontally> (all, ref) - box_position<Box, horizontally> (new_all, ref);
    if (horizontally) {
      mv = db::vector<coord_type> (d, 0);
    } else {
      mv = db::vector<coord_type> (0, d);
    }

    for (typename objects::iterator i = m_objects.begin (); i != m_objects.end (); ++i) {
      i->first.move (mv);
    }
  }
};

}

#endif


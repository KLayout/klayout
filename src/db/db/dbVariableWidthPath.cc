
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

#include "dbVariableWidthPath.h"

namespace db
{

template <class C>
void
variable_width_path<C>::init ()
{
  //  compress the points

  typename std::vector<db::point<C> >::iterator pw = m_points.begin ();
  typename std::vector<db::point<C> >::const_iterator pr = m_points.begin ();

  typename std::vector<std::pair<size_t, C> >::iterator ow = m_org_widths.begin ();

  while (pr != m_points.end ()) {

    size_t ir = pr - m_points.begin ();
    *pw = *pr++;
    while (pr != m_points.end () && *pw == *pr) {
      ++pr;
    }
    size_t irr = pr - m_points.begin ();

    size_t iw = pw - m_points.begin ();
    ++pw;

    while (ow != m_org_widths.end () && ow->first < irr && ow->first >= ir) {
      ow->first = iw;
      ++ow;
    }
    if (ow != m_org_widths.end ()) {
      tl_assert (ow->first >= irr);
    }

  }

  m_points.erase (pw, m_points.end ());

  //  create a per-point width specification

  width_type w = 0;
  size_t i = 0;
  bool last_set = false;

  for (typename std::vector<std::pair<size_t, C> >::const_iterator j = m_org_widths.begin (); j != m_org_widths.end (); ++j) {

    width_type w0 = w;
    w = j->second;

    tl_assert (j->first < m_points.size ());

    if (j->first == i) {

      if (last_set) {
        m_widths.back ().second = j->second;
      } else {
        m_widths.push_back (std::make_pair (w0, j->second));
      }

    } else {

      tl_assert (j->first > i);
      tl_assert (j->first < m_points.size ());

      //  interpolation: first determine the whole length from last point to next
      //  and then interpolate each segment

      double ll = 0;
      for (size_t ii = i; ii < j->first; ++ii) {
        ll += (m_points [ii + 1] - m_points [ii]).double_length ();
      }

      double l = 0;
      for (size_t ii = i; ii <= j->first; ++ii) {
        if (! last_set) {
          width_type ww = db::coord_traits<C>::rounded (w0 + (w - w0) * (l / ll));
          m_widths.push_back (std::make_pair (ww, ww));
        }
        last_set = false;
        if (ii < j->first) {
          l += (m_points [ii + 1] - m_points [ii]).double_length ();
        }
      }

      i = j->first;

    }

    last_set = true;

  }

  //  fill up the remaining widths (should not happen if the last width_spec is for
  //  the last point)
  while (m_points.size () > m_widths.size ()) {
    if (! last_set) {
      m_widths.push_back (std::make_pair (w, w));
    }
    last_set = false;
  }
}

template <class C, class Iter, class WIter, class Inserter>
static
void create_shifted_points (C /*c*/, bool forward, Iter from, Iter to, WIter wfrom, WIter wto, Inserter pts)
{
  //  for safety reasons
  if (from == to) {
    return;
  }

  WIter w = wfrom;
  WIter ww = w;
  ++ww;

  Iter p = from;
  Iter pp = p;
  ++pp;

  bool first = true;

  while (pp != to) {

    Iter ppp = pp;
    ++ppp;

    WIter www = ww;
    ++www;

    //  Compute the unit vector of the line and its normal (times width)

    db::DVector ed (*pp - *p);
    ed *= 1.0 / ed.double_length ();

    if (first) {

      first = false;

      db::DVector nd (-ed.y (), ed.x ());
      nd *= (forward ? w->second : w->first) * 0.5;

      *pts++ = (*p + vector<C> (nd));

    }

    if (ppp == to) {

      db::DVector nd (-ed.y (), ed.x ());
      nd *= (forward ? ww->first : ww->second) * 0.5;

      *pts++ = (*pp + vector<C> (nd));

    } else if (fabs (double (ww->first) - double (ww->second)) > db::epsilon) {

      //  switching widths -> create a direct connection

      db::DVector eed (*ppp - *pp);
      eed *= 1.0 / eed.double_length ();

      db::DVector nd (-ed.y (), ed.x ());
      nd *= (forward ? ww->first : ww->second) * 0.5;
      db::DVector nnd (-eed.y (), eed.x ());
      nnd *= (forward ? ww->second : ww->first) * 0.5;

      *pts++ = *pp + vector<C> (nd);
      *pts++ = *pp + vector<C> (nnd);

    } else {

      tl_assert (www != wto);

      double wi = ww->first;

      db::DVector eed (*ppp - *pp);
      eed *= 1.0 / eed.double_length ();

      //  Points in between are determined from taking two
      //  edges being shifted perpendicular from the original
      //  and being slightly extended. The intersection point
      //  of both gives the new vertex. If there is no intersection,
      //  the edges are simply connected.

      db::DVector nd1 (-ed.y (), ed.x ());
      nd1 *= (forward ? w->second : w->first) * 0.5;
      db::DVector nd2 (-ed.y (), ed.x ());
      nd2 *= wi * 0.5;

      db::DVector nnd1 (-eed.y (), eed.x ());
      nnd1 *= wi * 0.5;
      db::DVector nnd2 (-eed.y (), eed.x ());
      nnd2 *= (forward ? www->first : www->second) * 0.5;

      bool is_folded = false;

      double du = db::vprod (ed, eed);
      if (fabs (du) > db::epsilon) {

        double u1 = db::vprod (nnd1 - nd2, eed) / du;
        double u2 = db::vprod (nd2 - nnd1, ed) / du;

        is_folded = ((u1 < -db::epsilon) != (u2 < -db::epsilon));

      }

      if (is_folded) {

        //  No well-formed intersection (reflecting/back-folded segments) ->
        //  create a direct (inner) connection
        *pts++ = *pp + vector<C> (nd2);
        *pts++ = *pp + vector<C> (nnd1);

      } else {

        db::DVector g = (db::DPoint (*pp) + nd2) - (db::DPoint (*p) + nd1);
        double gl = g.double_length ();
        g *= 1.0 / gl;
        db::DVector gg = (db::DPoint (*ppp) + nnd2) - (db::DPoint (*pp) + nnd1);
        double ggl = gg.double_length ();
        gg *= 1.0 / ggl;

        double l1max = wi;
        double l2max = wi;

        double l1min = -gl - wi;
        double l2min = -ggl - wi;

        double dv = db::vprod (g, gg);
        if (fabs (dv) > db::epsilon) {

          double l1 = db::vprod (nnd1 - nd2, gg) / dv;
          double l2 = db::vprod (nd2 - nnd1, g) / dv;

          if (l1 < l1min - db::epsilon || l2 < l2min - db::epsilon) {

            //  Segments are too short - the won't intersect: In this case we create a loop of three
            //  points which define the area in self-overlapping way but confined to the path within
            //  the limits of its width.
            //  HINT: the execution of this code is a pretty strong evidence for the existence to loops
            //  in the contour delivered. A proof however is missing ..
            *pts++ = *pp + vector<C> (nd2);
            *pts++ = *pp;
            *pts++ = *pp + vector<C> (nnd1);

          } else if (l1 < l1max + db::epsilon && l2 < l2max + db::epsilon) {

            //  well-formed corner
            *pts++ = *pp + vector<C> (nd2 + g * l1);

          } else {

            //  cut-off corner: produce two points connecting the edges
            *pts++ = *pp + vector<C> (nd2 + g * std::min (l1max, l1));
            *pts++ = *pp + vector<C> (nnd1 - gg * std::min (l2max, l2));

          }

        } else if (db::sprod (g, gg) < -db::epsilon) {

          //  reflecting segment
          *pts++ = *pp + vector<C> (nd2 + g * wi);
          *pts++ = *pp + vector<C> (nnd1 - gg * wi);

        }

      }

    }

    p = pp;
    pp = ppp;
    w = ww;
    ww = www;

  }
}

template <class C>
typename variable_width_path<C>::simple_polygon_type variable_width_path<C>::to_poly () const
{
  std::vector<point<C> > pts;
  pts.reserve (m_points.size () * 2);  //  minimum number of points required

  C c = 0;
  create_shifted_points (c, true, m_points.begin (), m_points.end (), m_widths.begin (), m_widths.end (), std::back_inserter (pts));
  create_shifted_points (c, false, m_points.rbegin (), m_points.rend (), m_widths.rbegin (), m_widths.rend (), std::back_inserter (pts));

  simple_polygon_type poly;
  poly.assign_hull (pts.begin (), pts.end ());
  return poly;
}

template class variable_width_path<Coord>;
template class variable_width_path<DCoord>;

}

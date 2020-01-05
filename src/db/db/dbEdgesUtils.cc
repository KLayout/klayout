
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

#include "dbEdgesUtils.h"
#include "dbRegion.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  JoinEdgesCluster implementation

JoinEdgesCluster::JoinEdgesCluster (db::PolygonSink *output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i)
  : mp_output (output), m_ext_b (ext_b), m_ext_e (ext_e), m_ext_o (ext_o), m_ext_i (ext_i)
{
  //  .. nothing yet ..
}

void
JoinEdgesCluster::finish ()
{
  std::multimap<db::Point, iterator> objects_by_p1;
  std::multimap<db::Point, iterator> objects_by_p2;
  for (iterator o = begin (); o != end (); ++o) {
    if (o->first->p1 () != o->first->p2 ()) {
      objects_by_p1.insert (std::make_pair (o->first->p1 (), o));
      objects_by_p2.insert (std::make_pair (o->first->p2 (), o));
    }
  }

  while (! objects_by_p2.empty ()) {

    tl_assert (! objects_by_p1.empty ());

    //  Find the beginning of a new sequence
    std::multimap<db::Point, iterator>::iterator j0 = objects_by_p1.begin ();
    std::multimap<db::Point, iterator>::iterator j = j0;
    do {
      std::multimap<db::Point, iterator>::iterator jj = objects_by_p2.find (j->first);
      if (jj == objects_by_p2.end ()) {
        break;
      } else {
        j = objects_by_p1.find (jj->second->first->p1 ());
        tl_assert (j != objects_by_p1.end ());
      }
    } while (j != j0);

    iterator i = j->second;

    //  determine a sequence
    //  TODO: this chooses any solution in case of forks. Choose a specific one?
    std::vector<db::Point> pts;
    pts.push_back (i->first->p1 ());

    do {

      //  record the next point
      pts.push_back (i->first->p2 ());

      //  remove the edge as it's taken
      std::multimap<db::Point, iterator>::iterator jj;
      for (jj = objects_by_p2.find (i->first->p2 ()); jj != objects_by_p2.end () && jj->first == i->first->p2 (); ++jj) {
        if (jj->second == i) {
          break;
        }
      }
      tl_assert (jj != objects_by_p2.end () && jj->second == i);
      objects_by_p2.erase (jj);
      objects_by_p1.erase (j);

      //  process along the edge to the next one
      //  TODO: this chooses any solution in case of forks. Choose a specific one?
      j = objects_by_p1.find (i->first->p2 ());
      if (j != objects_by_p1.end ()) {
        i = j->second;
      } else {
        break;
      }

    } while (true);

    bool cyclic = (pts.back () == pts.front ());

    if (! cyclic) {

      //  non-cyclic sequence
      db::Path path (pts.begin (), pts.end (), 0, m_ext_b, m_ext_e, false);
      std::vector<db::Point> hull;
      path.hull (hull, m_ext_o, m_ext_i);
      db::Polygon poly;
      poly.assign_hull (hull.begin (), hull.end ());
      mp_output->put (poly);

    } else {

      //  we have a loop: form a contour by using the polygon size functions and a "Not" to form the hole
      db::Polygon poly;
      poly.assign_hull (pts.begin (), pts.end ());

      db::EdgeProcessor ep;
      db::PolygonGenerator pg (*mp_output, false, true);

      int mode_a = -1, mode_b = -1;

      if (m_ext_o == 0) {
        ep.insert (poly, 0);
      } else {
        db::Polygon sized_poly (poly);
        sized_poly.size (m_ext_o, m_ext_o, 2 /*sizing mode*/);
        ep.insert (sized_poly, 0);
        mode_a = 1;
      }

      if (m_ext_i == 0) {
        ep.insert (poly, 1);
      } else {
        db::Polygon sized_poly (poly);
        sized_poly.size (-m_ext_i, -m_ext_i, 2 /*sizing mode*/);
        ep.insert (sized_poly, 1);
        mode_b = 1;
      }

      db::BooleanOp2 op (db::BooleanOp::ANotB, mode_a, mode_b);
      ep.process (pg, op);

    }

  }
}

// -------------------------------------------------------------------------------------------------------------
//  extended_edge implementation

db::Polygon
extended_edge (const db::Edge &edge, db::Coord ext_b, db::Coord ext_e, db::Coord ext_o, db::Coord ext_i)
{
  db::DVector d;
  if (edge.is_degenerate ()) {
    d = db::DVector (1.0, 0.0);
  } else {
    d = db::DVector (edge.d ()) * (1.0 / edge.double_length ());
  }

  db::DVector n (-d.y (), d.x ());

  db::Point pts[4] = {
    db::Point (db::DPoint (edge.p1 ()) - d * double (ext_b) + n * double (ext_o)),
    db::Point (db::DPoint (edge.p2 ()) + d * double (ext_e) + n * double (ext_o)),
    db::Point (db::DPoint (edge.p2 ()) + d * double (ext_e) - n * double (ext_i)),
    db::Point (db::DPoint (edge.p1 ()) - d * double (ext_b) - n * double (ext_i)),
  };

  db::Polygon poly;
  poly.assign_hull (pts + 0, pts + 4);
  return poly;
}

}

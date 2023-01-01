
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


#include "dbCellHullGenerator.h"
#include "dbClip.h"
#include "dbPolygonGenerators.h"
#include "tlIntervalMap.h"

namespace db
{

// ----------------------------------------------------------------------------
//  HullEdgeCollector definition and implementation

struct ECJoinOp
{
  void operator () (db::Coord &a, db::Coord b)
  {
    if (b > a) {
      a = b;
    }
  }
};

struct ECAreaCompareOp
{
  bool operator () (const db::Box &a, const db::Box &b) const
  {
    return a.area () < b.area ();
  }
};

/**
 *  @brief A utility class that collects all edges along one axis of the hull
 */
class HullEdgeCollector
{
public:
  HullEdgeCollector ()
  {
    //  .. nothing yet ..
  }

  HullEdgeCollector (const db::Edge &e)
    : m_e (e)
  {
    int rot = 0;
    if (e.dx () > 0) {
      rot = db::FTrans::r0;
    } else if (e.dy () > 0) {
      rot = db::FTrans::r90;
    } else if (e.dx () < 0) {
      rot = db::FTrans::r180;
    } else if (e.dy () < 0) {
      rot = db::FTrans::r270;
    }
    m_tn = db::Trans (rot, db::Vector (e.p1 ()));
  }

  void add (const db::Polygon &poly)
  {
    ECJoinOp jo;
    db::Trans ti = m_tn.inverted ();
    for (db::Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
      if (db::sprod_sign (*e, m_e) > 0) {
        db::Edge en = (*e).transformed (ti);
        m_cmap.add (en.x1 (), en.x2 (), std::max (en.y1 (), en.y2 ()), jo);
      }
    }
  }

  void produce (std::vector <db::Point> &points)
  {
    if (m_cmap.begin () == m_cmap.end ()) {
      return;
    }

    //  produce the contour
    db::Coord xl = 0, yl = 0;

    points.push_back (m_tn.trans (db::Point (xl, yl)));

    for (tl::interval_map <db::Coord, db::Coord>::const_iterator cm = m_cmap.begin (); cm != m_cmap.end (); ++cm) {

      db::Coord x1 = cm->first.first;
      db::Coord x2 = cm->first.second;
      db::Coord y = cm->second;

      if (x1 != xl || y != yl) {
        db::Coord yi = std::min (yl, y);
        if (yi != yl) {
          points.push_back (m_tn.trans (db::Point (xl, yi)));
        }
        if (x1 != xl) {
          points.push_back (m_tn.trans (db::Point (x1, yi)));
        }
      }

      points.push_back (m_tn.trans (db::Point (x1, y)));
      points.push_back (m_tn.trans (db::Point (x2, y)));

      yl = y;
      xl = x2;

    }

    db::Coord xe = m_e.length ();

    if (xe != xl || yl != 0) {
      if (yl != 0) {
        points.push_back (m_tn.trans (db::Point (xl, 0)));
      }
      if (xl != xe) {
        points.push_back (m_tn.trans (db::Point (xe, 0)));
      }
    }
  }

  void reduce (size_t n) 
  {
    //  remove as many concave pockets in the contour to achieve size n 
    //  (proceed in the order of area)

    std::vector <db::Box> pockets;

    size_t ntot = 0;
    while ((ntot = m_cmap.size ()) > n) {

      pockets.clear ();

      if (ntot > 1) {

        tl::interval_map <db::Coord, db::Coord>::const_iterator cl = m_cmap.begin ();
        for (tl::interval_map <db::Coord, db::Coord>::const_iterator cm = m_cmap.begin (); cm != m_cmap.end (); ) {

          tl::interval_map <db::Coord, db::Coord>::const_iterator cc = cm;
          ++cm;

          if ((cc == m_cmap.begin () || cc->second < cl->second) && (cm == m_cmap.end () || cc->second < cm->second)) {
            if (cc == m_cmap.begin ()) {
              pockets.push_back (db::Box (db::Point (cc->first.first, cm->second), db::Point (cc->first.second, cc->second)));
            } else if (cm == m_cmap.end ()) {
              pockets.push_back (db::Box (db::Point (cc->first.first, cl->second), db::Point (cc->first.second, cc->second)));
            } else {
              pockets.push_back (db::Box (db::Point (cc->first.first, std::max (cl->second, cm->second)), db::Point (cc->first.second, cc->second)));
            }
          }

          cl = cc;

        }

      }

      if (pockets.size () > ntot - n) {
        ECAreaCompareOp ac_op;
        std::nth_element (pockets.begin (), pockets.begin () + (ntot - n), pockets.end (), ac_op);
        pockets.erase (pockets.begin () + (ntot - n), pockets.end ());
      }

      if (pockets.empty ()) {
        break;
      }

      for (std::vector <db::Box>::const_iterator p = pockets.begin (); p != pockets.end (); ++p) {
        ECJoinOp jo;
        m_cmap.add (p->left (), p->right (), p->top (), jo);
      }

    }
  }

private:
  db::Edge m_e;
  db::Trans m_tn;
  tl::interval_map <db::Coord, db::Coord> m_cmap;
};

// ----------------------------------------------------------------------------
//  CellHullGenerator implementation

const size_t default_complexity = 100;

CellHullGenerator::CellHullGenerator (const db::Layout &layout)
  : m_all_layers (true), m_small_cell_size (100), m_complexity (default_complexity)
{
  for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
    m_layers.push_back ((*l).first);
  }
}

CellHullGenerator::CellHullGenerator (const db::Layout &layout, const std::vector <unsigned int> &layers)
  : m_all_layers (true), m_small_cell_size (100), m_complexity (default_complexity)
{
  std::set <unsigned int> ll;
  ll.insert (layers.begin (), layers.end ());
  for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
    if (ll.find ((*l).first) != ll.end ()) {
      m_layers.push_back ((*l).first);
    } else {
      m_all_layers = false;
    }
  }
}

void 
CellHullGenerator::set_small_cell_size (db::Coord sms)
{
  m_small_cell_size = sms;
}

void 
CellHullGenerator::set_complexity (size_t complexity)
{
  m_complexity = complexity;
}

void CellHullGenerator::generate_hull (const db::Cell &cell, std::vector <db::Polygon> &hull)
{
  db::Box bbox;
  if (m_all_layers) {
    bbox = cell.bbox ();
  } else {
    for (std::vector <unsigned int>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
      bbox += cell.bbox (*l);
    }
  }

  //  empty cells don't contribute
  if (bbox.empty ()) {
    return;
  }

  //  for small cells just take the bbox
  if (bbox.height () <= db::coord_traits <db::Coord>::distance_type (m_small_cell_size) && bbox.width () <= db::coord_traits <db::Coord>::distance_type (m_small_cell_size)) {
    hull.push_back (db::Polygon (bbox));
    return;
  }

  db::Box sectors [4] = {
    db::Box (bbox.lower_left (), bbox.center ()),
    db::Box (bbox.lower_right (), bbox.center ()),
    db::Box (bbox.upper_left (), bbox.center ()),
    db::Box (bbox.upper_right (), bbox.center ())
  };

  db::HullEdgeCollector ec [4][4];
  for (unsigned int i = 0; i < 4; ++i) {
    db::Polygon ps (sectors [i]);
    unsigned int j = 0;
    for (db::Polygon::polygon_edge_iterator es = ps.begin_edge (); ! es.at_end () && j < 4; ++es, ++j) {
      ec [i][j] = db::HullEdgeCollector (*es);
    }
  }

  std::vector <db::Polygon> clipped_polygons;

  for (std::vector <unsigned int>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {

    for (db::ShapeIterator s = cell.shapes (*l).begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes | db::ShapeIterator::Paths); ! s.at_end (); ++s) {

      db::Polygon poly;
      s->polygon (poly);

      for (unsigned int is = 0; is < 4; ++is) {

        if (! poly.box ().overlaps (sectors [is])) {
          continue;
        }

        if (poly.box ().inside (sectors [is])) {
          for (unsigned int ie = 0; ie < 4; ++ie) {
            ec [is][ie].add (poly);
          }
        } else {
          clipped_polygons.clear ();
          db::clip_poly (poly, sectors [is], clipped_polygons);
          for (std::vector <db::Polygon>::const_iterator p = clipped_polygons.begin (); p != clipped_polygons.end (); ++p) {
            for (unsigned int ie = 0; ie < 4; ++ie) {
              ec [is][ie].add (*p);
            }
          }
        }

      }

    }

  }

  //  reduce the number of intervals on the edges 
  //  (the complexity is roughly distributed on the 
  //  various contributions, 1/10th is a rough estimate)
  for (unsigned int is = 0; is < 4; ++is) {
    for (unsigned int ie = 0; ie < 4; ++ie) {
      ec [is][ie].reduce (m_complexity / 10);
    }
  }

  db::EdgeProcessor ep;

  //  produce points
  for (unsigned int is = 0; is < 4; ++is) {

    std::vector <db::Point> points;
    size_t s1 [4], s2 [4];

    for (unsigned int ie = 0; ie < 4; ++ie) {
      s1 [ie] = points.size ();
      ec [is][ie].produce (points);
      s2 [ie] = points.size ();
    }

    if (! points.empty ()) {

      // produce the edges
      for (unsigned int ie = 0; ie < 4; ++ie) {
        if (s1 [ie] != s2 [ie]) {
          for (size_t si = s1 [ie] + 1; si != s2 [ie]; ++si) {
            ep.insert (db::Edge (points [si - 1], points [si]));
          }
        }
      }

    }

  }

  db::PolygonContainer ps (hull);
  db::PolygonGenerator pg (ps, false);
  //  use mode 1 so the loops appearing at the corners don't hurt
  db::SimpleMerge op (1);
  ep.process (pg, op);
}

}



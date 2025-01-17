
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "dbPolygon.h"
#include "dbPolygonTools.h"
#include "dbPath.h"
#include "dbBox.h"
#include "dbEdge.h"
#include "dbText.h"
#include "dbShapeRepository.h"
#include "tlReuseVector.h"
#include "tlUnitTest.h"

#include <vector>

namespace
{

class TestMemStatistics
  : public db::MemStatistics
{
public:
  TestMemStatistics ()
    : used (0), reqd (0)
  { }

  virtual void add (const std::type_info & /*ti*/, void * /*ptr*/, size_t r, size_t u, void * /*parent*/, purpose_t /*purpose*/ = None, int /*cat*/ = 0)
  {
    used += u;
    reqd += r;
  }

  void clear ()
  {
    used = reqd = 0;
  }

public:
  size_t used, reqd;
};

}

TEST(1)
{
  db::Polygon p;
  db::Polygon empty;
  db::Box b;

  EXPECT_EQ (empty == p, true);
  EXPECT_EQ (p.is_box (), false);

  std::vector <db::Point> c1, c2, c3;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p.assign_hull (c1.begin (), c1.end ());
  b = p.box ();
  EXPECT_EQ (p.holes (), size_t (0));
  EXPECT_EQ (p.area (), 1000*100);
  EXPECT_EQ (p.area2 (), 2*1000*100);
  EXPECT_EQ (tl::to_string (p.area_ratio ()), "1");
  EXPECT_EQ (p.perimeter (), db::Polygon::perimeter_type (2200));
  EXPECT_EQ (p.is_box (), true);
  EXPECT_EQ (p.is_rectilinear (), true);
  EXPECT_EQ (p.is_halfmanhattan (), true);

  c2.push_back (db::Point (10, 10));
  c2.push_back (db::Point (10, 390));
  c2.push_back (db::Point (90, 390));
  c2.push_back (db::Point (90, 10));
  p.insert_hole (c2.begin (), c2.end ());

  c3.push_back (db::Point (10, 510));
  c3.push_back (db::Point (10, 890));
  c3.push_back (db::Point (90, 890));
  c3.push_back (db::Point (90, 510));
  p.insert_hole (c3.begin (), c3.end ());
  EXPECT_EQ (p.holes (), size_t (2));
  EXPECT_EQ (p.is_box (), false);
  EXPECT_EQ (p.is_rectilinear (), true);
  EXPECT_EQ (p.is_halfmanhattan (), true);

  EXPECT_EQ (p.to_string (), std::string ("(0,0;0,1000;100,1000;100,0/10,10;90,10;90,390;10,390/10,510;90,510;90,890;10,890)"));
  db::DPolygon dp (p, db::cast_op<db::DPoint, db::Point> ());
  EXPECT_EQ (dp.to_string (), std::string ("(0,0;0,1000;100,1000;100,0/10,10;90,10;90,390;10,390/10,510;90,510;90,890;10,890)"));
  db::Polygon ip = db::Polygon (dp);
  EXPECT_EQ (ip.to_string (), std::string ("(0,0;0,1000;100,1000;100,0/10,10;90,10;90,390;10,390/10,510;90,510;90,890;10,890)"));
  EXPECT_EQ (ip.vertices (), size_t (12));

  EXPECT_EQ (p.area (), 1000*100-2*380*80);
  EXPECT_EQ (p.area2 (), 2*(1000*100-2*380*80));
  EXPECT_EQ (tl::to_string (p.area_ratio (), 6), "2.55102");
  EXPECT_EQ (p.perimeter (), db::Polygon::perimeter_type (2000+200+4*(380+80)));
  EXPECT_EQ (p.is_box (), false);
  EXPECT_EQ (p.box (), b);

  unsigned int e = 0;
  db::Edge::distance_type u = 0;
  for (db::Polygon::polygon_edge_iterator i = p.begin_edge (); ! i.at_end (); ++i) {
    ++e;
    u += (*i).length ();
  }
  EXPECT_EQ (e, (unsigned int) 12);
  EXPECT_EQ (u, db::Edge::distance_type (2*(1000+100)+4*(380+80)));

  db::Polygon pp;
  pp.insert_hole (c3.begin (), c3.end ());
  pp.insert_hole (c2.begin (), c2.end ());
  pp.sort_holes ();
  pp.assign_hull (c1.begin (), c1.end ());
  EXPECT_EQ (pp.area (), 1000*100-2*380*80);
  EXPECT_EQ (pp.area2 (), 2*(1000*100-2*380*80));
  EXPECT_EQ (pp.box (), b);

  EXPECT_EQ (p, pp);

  pp.transform (db::Trans (1, true, db::Vector (0, 0)));
  EXPECT_EQ (p == pp, false);
  EXPECT_EQ (p != pp, true);
  EXPECT_EQ (pp.box (), b.transformed (db::Trans (1, true, db::Vector (0, 0))));
  pp.transform (db::Trans (3, false, db::Vector (0, 0)));
  pp.transform (db::Trans (0, true, db::Vector (0, 0)));
  EXPECT_EQ (pp.area (), 1000*100-2*380*80);
  EXPECT_EQ (pp.box (), b);

  EXPECT_EQ (pp, p);
  pp.transform (db::Trans (0, false, db::Vector (100, -200)));
  EXPECT_EQ (pp.box (), b.moved (db::Vector (100, -200)));
  pp.move (-db::Vector (100, -200));
  EXPECT_EQ (pp, p);
  EXPECT_EQ (pp.box (), b);

  p.clear ();
  EXPECT_EQ (p, empty);

  c1.clear ();
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1100));
  c1.push_back (db::Point (100, 0));
  p.assign_hull (c1.begin (), c1.end ());
  EXPECT_EQ (p.is_box (), false);
  EXPECT_EQ (p.is_rectilinear (), false);
  EXPECT_EQ (p.is_halfmanhattan (), true);


  c1.clear ();
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1101));
  c1.push_back (db::Point (100, 0));
  p.assign_hull (c1.begin (), c1.end ());
  EXPECT_EQ (p.is_box (), false);
  EXPECT_EQ (p.is_rectilinear (), false);
  EXPECT_EQ (p.is_halfmanhattan (), false);
}


TEST(2) 
{
  db::SimplePolygon p;
  db::SimplePolygon empty;
  db::Box b;

  EXPECT_EQ (empty == p, true);

  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p.assign_hull (c1.begin (), c1.end ());
  b = p.box ();
  EXPECT_EQ (p.holes (), size_t (0));
  EXPECT_EQ (p.area (), 1000*100);
  EXPECT_EQ (p.area2 (), 2*1000*100);
  EXPECT_EQ (tl::to_string (p.area_ratio ()), "1");
  EXPECT_EQ (p.perimeter (), db::SimplePolygon::perimeter_type (2000+200));
  EXPECT_EQ (p.is_box (), true);
  EXPECT_EQ (p.is_rectilinear (), true);
  EXPECT_EQ (p.is_halfmanhattan (), true);

  EXPECT_EQ (p.to_string (), "(0,0;0,1000;100,1000;100,0)");
  db::DSimplePolygon dp (p, db::cast_op<db::DPoint, db::Point> ());
  EXPECT_EQ (dp.to_string (), "(0,0;0,1000;100,1000;100,0)");
  db::SimplePolygon ip = db::SimplePolygon (dp);
  EXPECT_EQ (ip.to_string (), "(0,0;0,1000;100,1000;100,0)");

  unsigned int e = 0;
  db::Edge::distance_type u = 0;
  for (db::SimplePolygon::polygon_edge_iterator i = p.begin_edge (); ! i.at_end (); ++i) {
    ++e;
    u += (*i).length ();
  }
  EXPECT_EQ (e, (unsigned int) 4);
  EXPECT_EQ (u, db::Edge::distance_type (2*(1000+100)));

  db::SimplePolygon pp;
  pp = p;
  EXPECT_EQ (pp.area (), 1000*100);
  EXPECT_EQ (pp.box (), b);

  EXPECT_EQ (p, pp);

  pp.transform (db::Trans (1, true, db::Vector (0, 0)));
  EXPECT_EQ (p == pp, false);
  EXPECT_EQ (p != pp, true);
  EXPECT_EQ (pp.box (), b.transformed (db::Trans (1, true, db::Vector (0, 0))));
  pp.transform (db::Trans (3, false, db::Vector (0, 0)));
  pp.transform (db::Trans (0, true, db::Vector (0, 0)));
  EXPECT_EQ (pp.area (), 1000*100);
  EXPECT_EQ (pp.box (), b);

  EXPECT_EQ (pp, p);
  pp.transform (db::Trans (0, false, db::Vector (100, -200)));
  EXPECT_EQ (pp.box (), b.moved (db::Vector (100, -200)));
  pp.move (-db::Vector (100, -200));
  EXPECT_EQ (pp, p);
  EXPECT_EQ (pp.box (), b);

  p.clear ();
  EXPECT_EQ (p, empty);

  c1.clear ();
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1100));
  c1.push_back (db::Point (100, 0));
  p.assign_hull (c1.begin (), c1.end ());
  EXPECT_EQ (p.is_rectilinear (), false);
  EXPECT_EQ (p.is_halfmanhattan (), true);

  c1.clear ();
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1101));
  c1.push_back (db::Point (100, 0));
  p.assign_hull (c1.begin (), c1.end ());
  EXPECT_EQ (p.is_rectilinear (), false);
  EXPECT_EQ (p.is_halfmanhattan (), false);
}

TEST(3) 
{
  db::Point pts [] = {
    db::Point (100, 120),
    db::Point (100, 140),
    db::Point (100, 160),
    db::Point (100, 180),
    db::Point (100, 200),
    db::Point (0, 200),
    db::Point (0, 300),
    db::Point (300, 300),
    db::Point (300, 100),
    db::Point (100, 100)
  };

  for (unsigned int off = 0; off < sizeof (pts) / sizeof (pts [0]); ++off) {

    typedef db::polygon_contour<db::Coord> Ctr;
    Ctr contour;

    std::vector <db::Point> c1;
    for (unsigned int i = 0; i < sizeof (pts) / sizeof (pts [0]); ++i) {
      c1.push_back (pts [(i + off) % (sizeof (pts) / sizeof (pts [0]))]);
    }
    contour.assign (c1.begin (), c1.end (), false);

    TestMemStatistics ms;

    EXPECT_EQ (contour.size (), size_t (6));
    EXPECT_EQ (contour.is_hole (), false);
    ms.clear ();
    contour.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.used, 3 * sizeof(db::Point) + sizeof(Ctr));
    EXPECT_EQ (contour[0], db::Point (100,100));
    EXPECT_EQ (contour[1], db::Point (100,200));
    EXPECT_EQ (contour[2], db::Point (0,200));
    EXPECT_EQ (contour[3], db::Point (0,300));
    EXPECT_EQ (contour[4], db::Point (300,300));
    EXPECT_EQ (contour[5], db::Point (300,100));

    contour.assign (c1.begin (), c1.end (), true);

    EXPECT_EQ (contour.size (), size_t (6));
    EXPECT_EQ (contour.is_hole (), true);
    ms.clear ();
    contour.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.used, 3 * sizeof(db::Point) + sizeof(Ctr));
    EXPECT_EQ (contour[0], db::Point (100,100));
    EXPECT_EQ (contour[1], db::Point (300,100));
    EXPECT_EQ (contour[2], db::Point (300,300));
    EXPECT_EQ (contour[3], db::Point (0,300));
    EXPECT_EQ (contour[4], db::Point (0,200));
    EXPECT_EQ (contour[5], db::Point (100,200));

    Ctr contour2;
    contour2 = contour;
    db::Trans t (db::Trans::m45, db::Vector (123, -456));
    contour2.transform (t);
    EXPECT_EQ (contour2 == contour, false);
    EXPECT_EQ (contour2 != contour, true);
    contour2.transform (t.inverted ());
    EXPECT_EQ (contour2 == contour, true);

    EXPECT_EQ (contour2.size (), size_t (6));
    EXPECT_EQ (contour2.is_hole (), true);
    ms.clear ();
    contour.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.used, 3 * sizeof(db::Point) + sizeof(Ctr));
    EXPECT_EQ (contour2[0], db::Point (100,100));
    EXPECT_EQ (contour2[1], db::Point (300,100));
    EXPECT_EQ (contour2[2], db::Point (300,300));
    EXPECT_EQ (contour2[3], db::Point (0,300));
    EXPECT_EQ (contour2[4], db::Point (0,200));
    EXPECT_EQ (contour2[5], db::Point (100,200));

  }

}

TEST(4) 
{
  TestMemStatistics ms;

  db::Point pts [] = {
    db::Point (100, 150),
    db::Point (100, 200),
    db::Point (0, 300),
    db::Point (300, 300),
    db::Point (300, 100),
    db::Point (100, 100)
  };

  for (unsigned int off = 0; off < sizeof (pts) / sizeof (pts [0]); ++off) {

    typedef db::polygon_contour<db::Coord> Ctr;
    Ctr contour;

    std::vector <db::Point> c1;
    for (unsigned int i = 0; i < sizeof (pts) / sizeof (pts [0]); ++i) {
      c1.push_back (pts [(i + off) % (sizeof (pts) / sizeof (pts [0]))]);
    }
    contour.assign (c1.begin (), c1.end (), false);

    EXPECT_EQ (contour.size (), size_t (5));
    EXPECT_EQ (contour.is_hole (), false);
    ms.clear ();
    contour.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.used, 5 * sizeof(db::Point) + sizeof(Ctr));
    EXPECT_EQ (contour[0], db::Point (100,100));
    EXPECT_EQ (contour[1], db::Point (100,200));
    EXPECT_EQ (contour[2], db::Point (0,300));
    EXPECT_EQ (contour[3], db::Point (300,300));
    EXPECT_EQ (contour[4], db::Point (300,100));

    contour.assign (c1.begin (), c1.end (), true);

    EXPECT_EQ (contour.size (), size_t (5));
    EXPECT_EQ (contour.is_hole (), true);
    ms.clear ();
    contour.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.used, 5 * sizeof(db::Point) + sizeof(Ctr));
    EXPECT_EQ (contour[0], db::Point (100,100));
    EXPECT_EQ (contour[1], db::Point (300,100));
    EXPECT_EQ (contour[2], db::Point (300,300));
    EXPECT_EQ (contour[3], db::Point (0,300));
    EXPECT_EQ (contour[4], db::Point (100,200));

    Ctr contour2;
    db::Trans t (db::Trans::m45, db::Vector (123, -456));
    contour2 = contour.transformed (t);
    EXPECT_EQ (contour2 == contour, false);
    EXPECT_EQ (contour2 != contour, true);
    EXPECT_EQ (contour2.area (), contour.area ());
    EXPECT_EQ (contour2.perimeter (), contour.perimeter ());
    contour2.transform (t.inverted ());
    EXPECT_EQ (contour2 == contour, true);

    EXPECT_EQ (contour2.size (), size_t (5));
    EXPECT_EQ (contour2.is_hole (), true);
    ms.clear ();
    contour.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.used, 5 * sizeof(db::Point) + sizeof(Ctr));
    EXPECT_EQ (contour2[0], db::Point (100,100));
    EXPECT_EQ (contour2[1], db::Point (300,100));
    EXPECT_EQ (contour2[2], db::Point (300,300));
    EXPECT_EQ (contour2[3], db::Point (0,300));
    EXPECT_EQ (contour2[4], db::Point (100,200));

  }

}

TEST(5) 
{
  db::Polygon p;

  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p.assign_hull (c1.begin (), c1.end ());
  EXPECT_EQ (p.vertices (), size_t (4));

  std::vector <db::Point> c2;
  c2.push_back (db::Point (10, 10));
  c2.push_back (db::Point (10, 110));
  c2.push_back (db::Point (20, 110));
  c2.push_back (db::Point (20, 10));
  p.insert_hole (c2.begin (), c2.end ());
  EXPECT_EQ (p.vertices (), size_t (8));

  {
    db::Polygon::polygon_contour_iterator pt = p.begin_hull ();
    EXPECT_EQ (*pt, db::Point (0, 0)); ++pt;
    EXPECT_EQ (*pt, db::Point (0, 1000)); ++pt;
    EXPECT_EQ (*pt, db::Point (100, 1000)); ++pt;
    EXPECT_EQ (*pt, db::Point (100, 0)); ++pt;
    EXPECT_EQ (pt == p.end_hull (), true);
  }

  {
    db::Polygon::polygon_contour_iterator pt = p.begin_hole (0);
    EXPECT_EQ (*pt, db::Point (10, 10)); ++pt;
    EXPECT_EQ (*pt, db::Point (20, 10)); ++pt;
    EXPECT_EQ (*pt, db::Point (20, 110)); ++pt;
    EXPECT_EQ (*pt, db::Point (10, 110)); ++pt;
    EXPECT_EQ (pt == p.end_hole (0), true);
  }

  db::GenericRepository rep;
  db::polygon_ref<db::Polygon, db::Trans> pref (p, rep);
  
  {
    db::polygon_ref<db::Polygon, db::Trans>::polygon_contour_iterator pt = pref.begin_hull ();
    EXPECT_EQ (*pt, db::Point (0, 0)); ++pt;
    EXPECT_EQ (*pt, db::Point (0, 1000)); ++pt;
    EXPECT_EQ (*pt, db::Point (100, 1000)); ++pt;
    EXPECT_EQ (*pt, db::Point (100, 0)); ++pt;
    EXPECT_EQ (pt == pref.end_hull (), true);
  }

  {
    db::polygon_ref<db::Polygon, db::Trans>::polygon_contour_iterator pt = pref.begin_hole (0);
    EXPECT_EQ (*pt, db::Point (10, 10)); ++pt;
    EXPECT_EQ (*pt, db::Point (20, 10)); ++pt;
    EXPECT_EQ (*pt, db::Point (20, 110)); ++pt;
    EXPECT_EQ (*pt, db::Point (10, 110)); ++pt;
    EXPECT_EQ (pt == pref.end_hole (0), true);
  }

  db::Trans t (db::Trans::m45, db::Vector (123, -456));
  p.transform (t);
  pref.transform (t);

  {
    db::Polygon::polygon_contour_iterator pt = p.begin_hull ();
    EXPECT_EQ (*pt, db::Point (123, -456)); ++pt;
    EXPECT_EQ (*pt, db::Point (123, -356)); ++pt;
    EXPECT_EQ (*pt, db::Point (1123, -356)); ++pt;
    EXPECT_EQ (*pt, db::Point (1123, -456)); ++pt;
    EXPECT_EQ (pt == p.end_hull (), true);
  }

  {
    db::Polygon::polygon_contour_iterator pt = p.begin_hole (0);
    EXPECT_EQ (*pt, db::Point (133, -446)); ++pt;
    EXPECT_EQ (*pt, db::Point (233, -446)); ++pt;
    EXPECT_EQ (*pt, db::Point (233, -436)); ++pt;
    EXPECT_EQ (*pt, db::Point (133, -436)); ++pt;
    EXPECT_EQ (pt == p.end_hole (0), true);
  }

  {
    db::polygon_ref<db::Polygon, db::Trans>::polygon_contour_iterator pt = pref.begin_hull ();
    EXPECT_EQ (*pt, db::Point (123, -356)); ++pt;
    EXPECT_EQ (*pt, db::Point (1123, -356)); ++pt;
    EXPECT_EQ (*pt, db::Point (1123, -456)); ++pt;
    EXPECT_EQ (*pt, db::Point (123, -456)); ++pt;
    EXPECT_EQ (pt == pref.end_hull (), true);
  }

  {
    db::polygon_ref<db::Polygon, db::Trans>::polygon_contour_iterator pt = pref.begin_hole (0);
    EXPECT_EQ (*pt, db::Point (233, -446)); ++pt;
    EXPECT_EQ (*pt, db::Point (233, -436)); ++pt;
    EXPECT_EQ (*pt, db::Point (133, -436)); ++pt;
    EXPECT_EQ (*pt, db::Point (133, -446)); ++pt;
    EXPECT_EQ (pt == pref.end_hole (0), true);
  }

  EXPECT_EQ (p.area (), 100*1000-10*100);
  EXPECT_EQ (p.area2 (), 2*(100*1000-10*100));
  EXPECT_EQ (tl::to_string (p.area_ratio (), 6), "1.0101");
  EXPECT_EQ (p.perimeter (), db::Polygon::perimeter_type (200+2000+20+200));
  EXPECT_EQ (pref.area (), 100*1000-10*100);
  EXPECT_EQ (pref.area2 (), 2*(100*1000-10*100));
  EXPECT_EQ (pref.perimeter (), db::Polygon::perimeter_type (200+2000+20+200));

}

TEST(6) 
{
  db::Box bx (db::Point (0, 0), db::Point (1000, 2000));
  db::Polygon p(bx);

  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (-1, 0)), -1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (0, -1)), -1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (0, 0)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (1, 0)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (1, 1)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (999, 1999)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (999, 2000)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (1000, 2000)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (1000, 1999)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (1000, 2001)), -1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (1001, 2000)), -1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (500, 500)), 1);

  db::inside_poly_test<db::Polygon> it (p);
  EXPECT_EQ (it (db::Point (-1, 0)), -1);
  EXPECT_EQ (it (db::Point (0, -1)), -1);
  EXPECT_EQ (it (db::Point (0, 0)), 0);
  EXPECT_EQ (it (db::Point (1, 0)), 0);
  EXPECT_EQ (it (db::Point (1, 1)), 1);
  EXPECT_EQ (it (db::Point (999, 1999)), 1);
  EXPECT_EQ (it (db::Point (999, 2000)), 0);
  EXPECT_EQ (it (db::Point (1000, 2000)), 0);
  EXPECT_EQ (it (db::Point (1000, 1999)), 0);
  EXPECT_EQ (it (db::Point (1000, 2001)), -1);
  EXPECT_EQ (it (db::Point (1001, 2000)), -1);
  EXPECT_EQ (it (db::Point (500, 500)), 1);

  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 4));
  c1.push_back (db::Point (0, 7));
  c1.push_back (db::Point (2, 7));
  c1.push_back (db::Point (3, 2));
  c1.push_back (db::Point (4, 7));
  c1.push_back (db::Point (5, 7));
  c1.push_back (db::Point (6, 4));
  c1.push_back (db::Point (7, 7));
  c1.push_back (db::Point (8, 7));
  c1.push_back (db::Point (9, 3));
  c1.push_back (db::Point (10, 7));
  c1.push_back (db::Point (12, 7));
  c1.push_back (db::Point (12, 4));
  c1.push_back (db::Point (12, 0));
  p.assign_hull (c1.begin (), c1.end ());
  EXPECT_EQ (p.is_box (), false);

  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (-1, 2)), -1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (0, 2)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (1, 2)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (2, 2)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (3, 2)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (4, 2)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (11, 2)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (12, 2)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (13, 2)), -1);

  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (-1, 4)), -1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (0, 4)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (1, 4)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (2, 4)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (3, 4)), -1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (4, 4)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (5, 4)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (6, 4)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (7, 4)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (8, 4)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (9, 4)), -1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (10, 4)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (11, 4)), 1);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (12, 4)), 0);
  EXPECT_EQ (db::inside_poly (p.begin_edge (), db::Point (13, 4)), -1);

  db::inside_poly_test<db::Polygon> it2 (p);

  EXPECT_EQ (it2 (db::Point (-1, 2)), -1);
  EXPECT_EQ (it2 (db::Point (0, 2)), 0);
  EXPECT_EQ (it2 (db::Point (1, 2)), 1);
  EXPECT_EQ (it2 (db::Point (2, 2)), 1);
  EXPECT_EQ (it2 (db::Point (3, 2)), 0);
  EXPECT_EQ (it2 (db::Point (4, 2)), 1);
  EXPECT_EQ (it2 (db::Point (11, 2)), 1);
  EXPECT_EQ (it2 (db::Point (12, 2)), 0);
  EXPECT_EQ (it2 (db::Point (13, 2)), -1);

  EXPECT_EQ (it2 (db::Point (-1, 4)), -1);
  EXPECT_EQ (it2 (db::Point (0, 4)), 0);
  EXPECT_EQ (it2 (db::Point (1, 4)), 1);
  EXPECT_EQ (it2 (db::Point (2, 4)), 1);
  EXPECT_EQ (it2 (db::Point (3, 4)), -1);
  EXPECT_EQ (it2 (db::Point (4, 4)), 1);
  EXPECT_EQ (it2 (db::Point (5, 4)), 1);
  EXPECT_EQ (it2 (db::Point (6, 4)), 0);
  EXPECT_EQ (it2 (db::Point (7, 4)), 1);
  EXPECT_EQ (it2 (db::Point (8, 4)), 1);
  EXPECT_EQ (it2 (db::Point (9, 4)), -1);
  EXPECT_EQ (it2 (db::Point (10, 4)), 1);
  EXPECT_EQ (it2 (db::Point (11, 4)), 1);
  EXPECT_EQ (it2 (db::Point (12, 4)), 0);
  EXPECT_EQ (it2 (db::Point (13, 4)), -1);
}

TEST(7) 
{
  TestMemStatistics ms;

  db::Point pts [] = {
    db::Point (0, 0),
    db::Point (0, 4),
    db::Point (4, 4),
    db::Point (4, 0),
    db::Point (4, 4),
    db::Point (0, 4),
  };

  for (unsigned int off = 0; off < sizeof (pts) / sizeof (pts [0]); ++off) {

    typedef db::polygon_contour<db::Coord> Ctr;
    Ctr contour;

    std::vector <db::Point> c1;
    for (unsigned int i = 0; i < sizeof (pts) / sizeof (pts [0]); ++i) {
      c1.push_back (pts [(i + off) % (sizeof (pts) / sizeof (pts [0]))]);
    }
    contour.assign (c1.begin (), c1.end (), false);

    EXPECT_EQ (contour.size (), size_t (6));
    EXPECT_EQ (contour.is_hole (), false);
    ms.clear ();
    contour.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.used, 6 * sizeof(db::Point) + sizeof(Ctr));
    EXPECT_EQ (contour[0], db::Point (0,0));
    EXPECT_EQ (contour[1], db::Point (0,4));
    EXPECT_EQ (contour[2], db::Point (4,4));
    EXPECT_EQ (contour[3], db::Point (4,0));
    EXPECT_EQ (contour[4], db::Point (4,4));
    EXPECT_EQ (contour[5], db::Point (0,4));

    contour.assign (c1.begin (), c1.end (), true);

    EXPECT_EQ (contour.size (), size_t (6));
    EXPECT_EQ (contour.is_hole (), true);
    ms.clear ();
    contour.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.used, 6 * sizeof(db::Point) + sizeof(Ctr));
    EXPECT_EQ (contour[0], db::Point (0,0));
    EXPECT_EQ (contour[1], db::Point (0,4));
    EXPECT_EQ (contour[2], db::Point (4,4));
    EXPECT_EQ (contour[3], db::Point (4,0));
    EXPECT_EQ (contour[4], db::Point (4,4));
    EXPECT_EQ (contour[5], db::Point (0,4));

    Ctr contour2;
    contour2 = contour;
    db::Trans t (db::Trans::m45, db::Vector (123, -456));
    contour2.transform (t);
    EXPECT_EQ (contour2 == contour, false);
    EXPECT_EQ (contour2 != contour, true);
    contour2.transform (t.inverted ());
    EXPECT_EQ (contour2 == contour, true);

    EXPECT_EQ (contour2.size (), size_t (6));
    EXPECT_EQ (contour2.is_hole (), true);
    ms.clear ();
    contour.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.used, 6 * sizeof(db::Point) + sizeof(Ctr));
    EXPECT_EQ (contour2[0], db::Point (0,0));
    EXPECT_EQ (contour2[1], db::Point (0,4));
    EXPECT_EQ (contour2[2], db::Point (4,4));
    EXPECT_EQ (contour2[3], db::Point (4,0));
    EXPECT_EQ (contour2[4], db::Point (4,4));
    EXPECT_EQ (contour2[5], db::Point (0,4));

  }

}

TEST(8)
{
  db::SimplePolygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (6800, -35));    //  redundant
  pts.push_back (db::Point (6800, -325));
  pts.push_back (db::Point (5240, -325));
  pts.push_back (db::Point (5240, 5915));
  pts.push_back (db::Point (6800, 5915));   //  redundant
  pts.push_back (db::Point (10200, 5915));
  pts.push_back (db::Point (10200, 5685));
  pts.push_back (db::Point (6800, 5685));
  pts.push_back (db::Point (6800, 195));    //  redundant

  for (unsigned int i = 0; i < 16; ++i) {

    p.assign_hull (pts.begin (), pts.end ());

    db::SimplePolygon::polygon_contour_iterator h = p.begin_hull ();
    EXPECT_EQ (*h, db::Point (5240, -325)); h++;
    EXPECT_EQ (*h, db::Point (5240, 5915)); h++;
    EXPECT_EQ (*h, db::Point (10200, 5915)); h++;
    EXPECT_EQ (*h, db::Point (10200, 5685)); h++;
    EXPECT_EQ (*h, db::Point (6800, 5685)); h++;
    EXPECT_EQ (*h, db::Point (6800, -325)); h++;
    EXPECT_EQ (h == p.end_hull (), true);

    db::Point p0 = pts [0];
    pts.erase (pts.begin ());
    pts.push_back (p0);

  }

  std::reverse (pts.begin (), pts.end ());

  for (unsigned int i = 0; i < 16; ++i) {

    p.assign_hull (pts.begin (), pts.end ());

    db::SimplePolygon::polygon_contour_iterator h = p.begin_hull ();
    EXPECT_EQ (*h, db::Point (5240, -325)); h++;
    EXPECT_EQ (*h, db::Point (5240, 5915)); h++;
    EXPECT_EQ (*h, db::Point (10200, 5915)); h++;
    EXPECT_EQ (*h, db::Point (10200, 5685)); h++;
    EXPECT_EQ (*h, db::Point (6800, 5685)); h++;
    EXPECT_EQ (*h, db::Point (6800, -325)); h++;
    EXPECT_EQ (h == p.end_hull (), true);

    db::Point p0 = pts [0];
    pts.erase (pts.begin ());
    pts.push_back (p0);

  }

  std::vector<db::Point> ppts;
  for (std::vector<db::Point>::const_iterator pp = pts.begin (); pp != pts.end (); ++pp) {
    ppts.push_back (*pp);
    ppts.push_back (*pp);
  }
  pts = ppts;

  for (unsigned int i = 0; i < 32; ++i) {

    p.assign_hull (pts.begin (), pts.end ());

    db::SimplePolygon::polygon_contour_iterator h = p.begin_hull ();
    EXPECT_EQ (*h, db::Point (5240, -325)); h++;
    EXPECT_EQ (*h, db::Point (5240, 5915)); h++;
    EXPECT_EQ (*h, db::Point (10200, 5915)); h++;
    EXPECT_EQ (*h, db::Point (10200, 5685)); h++;
    EXPECT_EQ (*h, db::Point (6800, 5685)); h++;
    EXPECT_EQ (*h, db::Point (6800, -325)); h++;
    EXPECT_EQ (h == p.end_hull (), true);

    db::Point p0 = pts [0];
    pts.erase (pts.begin ());
    pts.push_back (p0);

  }

  std::reverse (pts.begin (), pts.end ());

  for (unsigned int i = 0; i < 32; ++i) {

    p.assign_hull (pts.begin (), pts.end ());

    db::SimplePolygon::polygon_contour_iterator h = p.begin_hull ();
    EXPECT_EQ (*h, db::Point (5240, -325)); h++;
    EXPECT_EQ (*h, db::Point (5240, 5915)); h++;
    EXPECT_EQ (*h, db::Point (10200, 5915)); h++;
    EXPECT_EQ (*h, db::Point (10200, 5685)); h++;
    EXPECT_EQ (*h, db::Point (6800, 5685)); h++;
    EXPECT_EQ (*h, db::Point (6800, -325)); h++;
    EXPECT_EQ (h == p.end_hull (), true);

    db::Point p0 = pts [0];
    pts.erase (pts.begin ());
    pts.push_back (p0);

  }

}

TEST(9)
{
  db::SimplePolygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (6800, -35));    //  redundant
  pts.push_back (db::Point (6800, -35));    //  redundant
  pts.push_back (db::Point (6800, -35));    //  redundant
  pts.push_back (db::Point (6800, -35));    //  redundant
  pts.push_back (db::Point (6800, -35));    //  redundant
  pts.push_back (db::Point (6800, -35));    //  redundant
  pts.push_back (db::Point (6800, -35));    //  redundant

  p.assign_hull (pts.begin (), pts.end ());

  db::SimplePolygon::polygon_contour_iterator h = p.begin_hull ();
  EXPECT_EQ (h == p.end_hull (), true);

}

TEST(10)
{
  db::SimplePolygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 3000));
  pts.push_back (db::Point (0, 2000));
  pts.push_back (db::Point (0, 0));
  pts.push_back (db::Point (1000, 1000));
  pts.push_back (db::Point (1000, 2000));

  p.assign_hull (pts.begin (), pts.end ());

  db::SimplePolygon::polygon_contour_iterator h = p.begin_hull ();
  EXPECT_EQ (*h, db::Point (0, 0)); h++;
  EXPECT_EQ (*h, db::Point (0, 2000)); h++;
  EXPECT_EQ (*h, db::Point (1000, 3000)); h++;
  EXPECT_EQ (*h, db::Point (1000, 1000)); h++;
  EXPECT_EQ (h == p.end_hull (), true);

}

TEST(11)
{
  db::SimplePolygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 3000));
  pts.push_back (db::Point (0, 2000));
  pts.push_back (db::Point (0, 0));
  pts.push_back (db::Point (1000, 1000));
  pts.push_back (db::Point (1000, 2000));

  p.assign_hull (pts.begin (), pts.end (), false /*not compressed*/);

  db::SimplePolygon::polygon_contour_iterator h = p.begin_hull ();
  EXPECT_EQ (*h, db::Point (0, 0)); h++;
  EXPECT_EQ (*h, db::Point (0, 2000)); h++;
  EXPECT_EQ (*h, db::Point (1000, 3000)); h++;
  EXPECT_EQ (*h, db::Point (1000, 2000)); h++;
  EXPECT_EQ (*h, db::Point (1000, 2000)); h++;
  EXPECT_EQ (*h, db::Point (1000, 1000)); h++;
  EXPECT_EQ (h == p.end_hull (), true);

}

TEST(12)
{
  db::Polygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 3000));
  pts.push_back (db::Point (0, 2000));
  pts.push_back (db::Point (0, 0));
  pts.push_back (db::Point (1000, 1000));
  pts.push_back (db::Point (1000, 2000));

  p.assign_hull (pts.begin (), pts.end (), false /*not compressed*/);

  db::SimplePolygon::polygon_contour_iterator h = p.begin_hull ();
  EXPECT_EQ (*h, db::Point (0, 0)); h++;
  EXPECT_EQ (*h, db::Point (0, 2000)); h++;
  EXPECT_EQ (*h, db::Point (1000, 3000)); h++;
  EXPECT_EQ (*h, db::Point (1000, 2000)); h++;
  EXPECT_EQ (*h, db::Point (1000, 2000)); h++;
  EXPECT_EQ (*h, db::Point (1000, 1000)); h++;
  EXPECT_EQ (h == p.end_hull (), true);

}

TEST(13)
{
  db::Polygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 3000));
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (0, 2000));
  pts.push_back (db::Point (0, 0));
  pts.push_back (db::Point (1000, 1000));
  pts.push_back (db::Point (1000, 1500));
  pts.push_back (db::Point (1000, 2000));

  p.assign_hull (pts.begin (), pts.end (), false /*not compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,3000;1000,2000;1000,2000;1000,2000;1000,1500;1000,1000)");

  p.compress (true /*remove reflected*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,1000)");

  p.assign_hull (pts.begin (), pts.end (), true /*compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,3000;1000,1000)");

  p.assign_hull (pts.begin (), pts.end (), true /*compressed*/, true /*remove reflected*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,1000)");

}

TEST(14)
{
  db::Polygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 3000));
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (0, 2000));
  pts.push_back (db::Point (0, 0));
  pts.push_back (db::Point (200, 200));
  pts.push_back (db::Point (500, 500));
  pts.push_back (db::Point (1000, 1000));
  pts.push_back (db::Point (1000, 1500));
  pts.push_back (db::Point (1000, 2000));

  p.assign_hull (pts.begin (), pts.end (), false /*not compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,3000;1000,2000;1000,2000;1000,2000;1000,1500;1000,1000;500,500;200,200)");

  p.compress (true);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,1000)");

  p.assign_hull (pts.begin (), pts.end (), true /*not compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,3000;1000,1000)");

  p.assign_hull (pts.begin (), pts.end (), true /*not compressed*/, true /*remove reflected*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,1000)");

}

TEST(13M)
{
  db::Polygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 3000));
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (0, 2000));
  pts.push_back (db::Point (0, 0));
  pts.push_back (db::Point (1000, 0));
  pts.push_back (db::Point (1000, 1000));
  pts.push_back (db::Point (1000, 1500));
  pts.push_back (db::Point (1000, 2000));

  p.assign_hull (pts.begin (), pts.end (), false /*not compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,3000;1000,2000;1000,2000;1000,2000;1000,1500;1000,1000;1000,0)");
  EXPECT_EQ (p.vertices (), size_t (10));

  p.compress (true /*remove reflected*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,0)");
  EXPECT_EQ (p.vertices (), size_t (4));

  p.assign_hull (pts.begin (), pts.end (), true /*compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,3000;1000,0)");
  EXPECT_EQ (p.vertices (), size_t (5));

  p.assign_hull (pts.begin (), pts.end (), true /*compressed*/, true /*remove reflected*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,0)");
  EXPECT_EQ (p.vertices (), size_t (4));

}

TEST(13S)
{
  db::SimplePolygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 3000));
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (0, 2000));
  pts.push_back (db::Point (0, 0));
  pts.push_back (db::Point (1000, 1000));
  pts.push_back (db::Point (1000, 1500));
  pts.push_back (db::Point (1000, 2000));

  p.assign_hull (pts.begin (), pts.end (), false /*not compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,3000;1000,2000;1000,2000;1000,2000;1000,1500;1000,1000)");

  p.compress (true /*remove reflected*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,1000)");

  p.assign_hull (pts.begin (), pts.end (), true /*compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,3000;1000,1000)");

  p.assign_hull (pts.begin (), pts.end (), true /*compressed*/, true /*remove reflected*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,1000)");

}

TEST(14S)
{
  db::SimplePolygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (1000, 3000));
  pts.push_back (db::Point (1000, 2000));
  pts.push_back (db::Point (0, 2000));
  pts.push_back (db::Point (0, 0));
  pts.push_back (db::Point (200, 200));
  pts.push_back (db::Point (500, 500));
  pts.push_back (db::Point (1000, 1000));
  pts.push_back (db::Point (1000, 1500));
  pts.push_back (db::Point (1000, 2000));

  p.assign_hull (pts.begin (), pts.end (), false /*not compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,3000;1000,2000;1000,2000;1000,2000;1000,1500;1000,1000;500,500;200,200)");
  EXPECT_EQ (p.vertices (), size_t (11));

  p.compress (true);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,1000)");
  EXPECT_EQ (p.vertices (), size_t (4));

  p.assign_hull (pts.begin (), pts.end (), true /*not compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,3000;1000,1000)");

  p.assign_hull (pts.begin (), pts.end (), true /*not compressed*/, true /*remove reflected*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,2000;1000,2000;1000,1000)");

}

TEST(14S2)
{
  db::SimplePolygon p;

  std::vector<db::Point> pts;
  pts.push_back (db::Point (200, 200));
  pts.push_back (db::Point (200, 200));
  pts.push_back (db::Point (300, 100));
  pts.push_back (db::Point (400, 100));
  pts.push_back (db::Point (400, 200));
  pts.push_back (db::Point (500, 200));
  pts.push_back (db::Point (500, 0));
  pts.push_back (db::Point (0, 0));
  pts.push_back (db::Point (0, 100));
  pts.push_back (db::Point (100, 100));

  p.assign_hull (pts.begin (), pts.end (), false /*not compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,100;100,100;200,200;200,200;300,100;400,100;400,200;500,200;500,0)");

  p.compress (true);

  EXPECT_EQ (p.to_string(), "(0,0;0,100;100,100;200,200;300,100;400,100;400,200;500,200;500,0)");

  p.assign_hull (pts.begin (), pts.end (), true /*not compressed*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,100;100,100;200,200;300,100;400,100;400,200;500,200;500,0)");

  p.assign_hull (pts.begin (), pts.end (), true /*not compressed*/, true /*remove reflected*/);

  EXPECT_EQ (p.to_string(), "(0,0;0,100;100,100;200,200;300,100;400,100;400,200;500,200;500,0)");

}

TEST(20)
{
  db::Polygon poly;
  EXPECT_EQ (poly.to_string (), "()");
  poly.size (100);
  EXPECT_EQ (poly.to_string (), "()");

  db::Point pts[] = {
    db::Point (100, 100),
    db::Point (400, 100),
    db::Point (400, 400),
    db::Point (100, 400)
  };

  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
  poly.size (100);
  EXPECT_EQ (poly.to_string (), "(0,0;0,500;500,500;500,0)");

  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
  poly.size (-100);
  EXPECT_EQ (poly.to_string (), "(100,100;200,100;200,400;100,400;100,300;400,300;400,400;300,400;300,100;400,100;400,200;100,200)");

  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
  poly.size (db::Coord (100), db::Coord (0));
  EXPECT_EQ (poly.to_string (), "(0,100;0,400;500,400;500,100)");

  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
  poly.size (db::Coord (0), db::Coord (100));
  EXPECT_EQ (poly.to_string (), "(100,0;100,500;400,500;400,0)");

  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
  poly.size (db::Coord (-100), db::Coord (0));
  EXPECT_EQ (poly.to_string (), "(100,100;200,100;200,400;100,400;400,400;300,400;300,100;400,100)");

  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
  poly.size (db::Coord (0), db::Coord (-100));
  EXPECT_EQ (poly.to_string (), "(100,100;100,400;100,300;400,300;400,400;400,100;400,200;100,200)");

  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
  poly.size (-400);
  EXPECT_EQ (poly.to_string (), "(100,0;400,0;400,400;0,400;0,100;400,100;400,500;100,500;100,100;500,100;500,400;100,400)");

  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));
  poly.size (100, 100, 0);
  EXPECT_EQ (poly.to_string (), "(100,0;0,100;0,400;100,500;400,500;500,400;500,100;400,0)");

  db::Point pts2[] = {
    db::Point (0, 0),
    db::Point (0, 400),
    db::Point (100, 400),
    db::Point (100, 100),
    db::Point (400, 100),
    db::Point (400, 0)
  };

  poly.assign_hull (pts2, pts2 + sizeof (pts2) / sizeof (pts2[0]));
  poly.size (0, -100, 2);
  EXPECT_EQ (poly.to_string (), "(0,0;0,400;0,300;100,300;100,400;100,0;400,0;400,100;400,0;400,100;0,100)");

  db::Point pts3[] = {
    db::Point (0, 0),
    db::Point (0, 100),
    db::Point (50, 100),
    db::Point (75, 50),
    db::Point (150, 300),
    db::Point (200, 300),
    db::Point (200, 0)
  };

  poly.assign_hull (pts3, pts3 + sizeof (pts3) / sizeof (pts3[0]));
  poly.size (100, 100, 4);
  EXPECT_EQ (poly.to_string (), "(-100,-100;-100,200;112,200;164,95;75,50;-21,79;76,400;300,400;300,-100)");

  poly.assign_hull (pts3, pts3 + sizeof (pts3) / sizeof (pts3[0]));
  poly.size (100, 100, 5);
  EXPECT_EQ (poly.to_string (), "(-100,-100;-100,200;112,200;164,95;75,50;-21,79;76,400;300,400;300,-100)");

  db::Point pts4[] = {
    db::Point (0, 0),
    db::Point (0, 200),
    db::Point (100, 300),
    db::Point (400, 300),
    db::Point (200, 100),
    db::Point (200, 0)
  };

  poly.assign_hull (pts4, pts4 + sizeof (pts4) / sizeof (pts4[0]));
  poly.size (-100, -100, 2);
  EXPECT_EQ (poly.to_string (), "(0,0;100,0;100,200;0,200;71,129;171,229;100,300;100,200;400,200;400,300;329,371;100,142;100,0;200,0;200,100;0,100)");
  
  db::Point pts5[] = {
    db::Point (0, 0),
    db::Point (0, 100),
    db::Point (100, 100),
    db::Point (100, 50),
    db::Point (150, 250),
    db::Point (250, 250),
    db::Point (250, 0)
  };

  poly.assign_hull (pts5, pts5 + sizeof (pts5) / sizeof (pts5[0]));
  poly.size (50, 50, 4);
  EXPECT_EQ (poly.to_string (), "(-50,-50;-50,150;150,150;150,50;100,50;51,62;111,300;300,300;300,-50)");

  db::Point pts6[] = {
    db::Point (100, 0),
    db::Point (100, 100),
    db::Point (0, 200),
    db::Point (50, 250),
    db::Point (200, 100),
    db::Point (200, 0)
  };

  poly.assign_hull (pts6, pts6 + sizeof (pts6) / sizeof (pts6[0]));
  poly.size (2, 2, 4);
  EXPECT_EQ (poly.to_string (), "(98,-2;98,100;100,100;99,99;-2,200;50,252;202,100;202,-2)");

  db::Point pts7[] = {
    db::Point (-90122, -84700),
    db::Point (-90162, -84652),
    db::Point (-90195, -84613),
    db::Point (-90229, -84572),
    db::Point (-90265, -84528),
    db::Point (-90304, -84481),
    db::Point (-90346, -84431),
    db::Point (-90390, -84378),
    db::Point (-90400, -84366),
    db::Point (-90400, -84300),
    db::Point (-90000, -84300),
    db::Point (-90000, -84700)
  };

  poly.assign_hull (pts7, pts7 + sizeof (pts7) / sizeof (pts7[0]));
  poly.size (50, 50, 4);
  EXPECT_EQ (poly.to_string (), "(-90145,-84750;-90200,-84684;-90233,-84645;-90267,-84604;-90304,-84560;-90342,-84513;-90384,-84463;-90428,-84410;-90450,-84384;-90450,-84250;-89950,-84250;-89950,-84750)");
}


TEST(21)
{
  TestMemStatistics ms;

  {
    db::Box box (0,0,2048,1536);
    db::Polygon poly (box);
    db::ICplxTrans t (7, 45, false, db::Vector (123,-10152));
    EXPECT_EQ (poly.to_string (), "(0,0;0,1536;2048,1536;2048,0)");
    poly.transform (t);
    EXPECT_EQ (poly.to_string (), "(123,-10152;-7480,-2549;2657,7588;10260,-15)");
#if !defined(_MSC_VER)
    ms.clear ();
    poly.mem_stat (&ms, db::MemStatistics::None, 0);
#if defined(HAVE_64BIT_COORD)
    EXPECT_EQ (ms.reqd, (sizeof(void *)-4)*5+116);
#else
    EXPECT_EQ (ms.reqd, (sizeof(void *)-4)*5+68);
#endif
#endif
  }

  {
    db::Box box (0,0,2048,1536);
    db::Polygon poly (box);
    db::ICplxTrans t (7, 0, false, db::Vector (123,-10152));
    EXPECT_EQ (poly.to_string (), "(0,0;0,1536;2048,1536;2048,0)");
    poly.transform (t);
    EXPECT_EQ (poly.to_string (), "(123,-10152;123,600;14459,600;14459,-10152)");
#if !defined(_MSC_VER)
    ms.clear ();
    poly.mem_stat (&ms, db::MemStatistics::None, 0);
#if defined(HAVE_64BIT_COORD)
    EXPECT_EQ (ms.reqd, (sizeof(void *)-4)*5+84);
#else
    EXPECT_EQ (ms.reqd, (sizeof(void *)-4)*5+52);
#endif
#endif
  }
  {
    db::DBox box (0,0,2048,1536);
    db::DPolygon poly (box);
    EXPECT_EQ (poly.is_box (), true);
    db::DCplxTrans t (7.02268521, 45, false, db::DVector (123.88147866,-10152.0640046));
    EXPECT_EQ (poly.to_string (), "(0,0;0,1536;2048,1536;2048,0)");
    poly.transform (t);
    EXPECT_EQ (poly.is_box (), false);
    EXPECT_EQ (poly.to_string (), "(123.88147866,-10152.0640046;-7503.56940256,-2524.61312338;2666.36510573,7645.32138492;10293.815987,17.8705036972)");
#if !defined(_MSC_VER)
    ms.clear ();
    poly.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.reqd, (sizeof(void *)-4)*5+116);
#endif
  }

  {
    db::DBox box (0,0,2048,1536);
    db::DPolygon poly (box);
    db::DCplxTrans t (7.02268521, 0, false, db::DVector (123.88147866,-10152.0640046));
    EXPECT_EQ (poly.to_string (), "(0,0;0,1536;2048,1536;2048,0)");
    // This transformation was not terminating in some builds (release):
    poly.transform (t);
    EXPECT_EQ (poly.to_string (), "(123.88147866,-10152.0640046;123.88147866,634.78047796;14506.3407887,634.78047796;14506.3407887,-10152.0640046)");
#if !defined(_MSC_VER)
    ms.clear ();
    poly.mem_stat (&ms, db::MemStatistics::None, 0);
    EXPECT_EQ (ms.reqd, (sizeof(void *)-4)*5+116); // no compression for doubles!
#endif
  }
}

TEST(22)
{
  db::Polygon poly;

  std::string s ("(0,0;0,1000;100,1000;100,0/10,10;90,10;90,390;10,390/10,510;90,510;90,890;10,890)");
  tl::Extractor ex (s.c_str ());
  ex.read (poly);
  EXPECT_EQ (poly.to_string (), s);
}

TEST(23)
{
  db::DPolygon poly;

  std::string s ("(0,0;0,1000;100,1000;100,0/10,10;90,10;90,390;10,390/10,510;90,510;90,890;10,890)");
  tl::Extractor ex (s.c_str ());
  ex.read (poly);
  EXPECT_EQ (poly.to_string (), s);
}

TEST(24)
{
  db::SimplePolygon poly;

  std::string s ("(0,0;0,1000;100,1000;100,0)");
  tl::Extractor ex (s.c_str ());
  ex.read (poly);
  EXPECT_EQ (poly.to_string (), s);
}

TEST(25)
{
  db::DSimplePolygon poly;

  std::string s ("(0,0;0,1000;100,1000;100,0)");
  tl::Extractor ex (s.c_str ());
  ex.read (poly);
  EXPECT_EQ (poly.to_string (), s);
}

TEST(26)
{
  db::DSimplePolygon poly;
  std::string s ("(0,0;0,1000;100,1000;100,0)");
  tl::Extractor ex (s.c_str ());
  ex.read (poly);

  tl::reuse_vector<db::DSimplePolygon> v;
  for (int i = 0; i < 10; ++i) {
    v.insert (poly);
  }
  EXPECT_EQ (v.begin ()->to_string (), s);
  for (int i = 0; i < 9; ++i) {
    v.erase (v.begin ());
  }
  EXPECT_EQ (v.begin ()->to_string (), s);
  v.clear ();
}

TEST(27)
{
  db::DPolygon poly;
  std::string s ("(0,0;0,1000;100,1000;100,0)");
  tl::Extractor ex (s.c_str ());
  ex.read (poly);

  tl::reuse_vector<db::DPolygon> v;
  for (int i = 0; i < 10; ++i) {
    v.insert (poly);
  }
  EXPECT_EQ (v.begin ()->to_string (), s);
  for (int i = 0; i < 9; ++i) {
    v.erase (v.begin ());
  }
  EXPECT_EQ (v.begin ()->to_string (), s);
  v.clear ();
}

TEST(28)
{
  //  32bit overflow for perimeter
  db::Polygon b (db::Box (-1000000000, -1000000000, 1000000000, 1000000000));
  EXPECT_EQ (b.perimeter (), 8000000000.0);
}

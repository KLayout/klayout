
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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
#include "dbEdgeProcessor.h"
#include "tlString.h"
#include "tlUnitTest.h"

static std::string h2s (const std::vector<db::Polygon> &hull)
{
  std::string r;
  for (std::vector<db::Polygon>::const_iterator h = hull.begin (); h != hull.end (); ++h) {
    if (!r.empty ()) {
      r += ";";
    }
    r += h->to_string ();
  }
  return r;
}

static bool check_hull (const std::vector<db::Polygon> &hull, const db::Shapes &shapes)
{
  std::vector<db::Polygon> sp;
  for (db::ShapeIterator s = shapes.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes | db::ShapeIterator::Paths); ! s.at_end (); ++s) {
    sp.push_back (db::Polygon ());
    s->polygon (sp.back ());
  }

  db::EdgeProcessor ep;
  std::vector<db::Polygon> out;
  ep.boolean (hull, sp, out, db::BooleanOp::BNotA, false, false);

  if (! out.empty ()) {
    std::cout << "check_hull(): not empty" << std::endl;
    for (std::vector<db::Polygon>::const_iterator p = out.begin (); p != out.end (); ++p) {
      std::cout << "  " << p->to_string () << std::endl;
    }
  }
  return out.empty ();
}

TEST(1) 
{
  db::Manager m (true);
  db::Layout g (&m);
  unsigned int l1 = g.insert_layer (db::LayerProperties (1, 0)); 
  unsigned int l2 = g.insert_layer (db::LayerProperties (2, 0)); 
  db::Cell &c1 (g.cell (g.add_cell ()));

  c1.shapes (l1).insert (db::Box (0, 0, 2100, 2100));
  c1.shapes (l2).insert (db::Box (-100, -100, 2000, 2000));

  db::CellHullGenerator chg (g);

  std::vector <db::Polygon> hull;
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(-100,-100;-100,2000;0,2000;0,2100;2100,2100;2100,0;2000,0;2000,-100)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  c1.shapes (l1).clear ();
  c1.shapes (l2).clear ();

  c1.shapes (l1).insert (db::Box (0, 0, 2000, 200));
  c1.shapes (l1).insert (db::Box (0, 0, 200, 2000));

  hull.clear ();
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,2000;200,2000;200,200;2000,200;2000,0)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  c1.shapes (l1).clear ();
  c1.shapes (l2).clear ();

  c1.shapes (l1).insert (db::Box (0, 0, 2000, 200));
  c1.shapes (l1).insert (db::Box (0, 1800, 2000, 2000));
  c1.shapes (l1).insert (db::Box (0, 0, 200, 2000));

  hull.clear ();
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,2000;2000,2000;2000,1800;200,1800;200,200;2000,200;2000,0)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  c1.shapes (l1).clear ();
  c1.shapes (l2).clear ();

  c1.shapes (l1).insert (db::Box (0, 0, 2000, 200));
  c1.shapes (l1).insert (db::Box (0, 1800, 2000, 2000));
  c1.shapes (l1).insert (db::Box (1800, 0, 2000, 2000));
  c1.shapes (l1).insert (db::Box (0, 0, 200, 2000));

  hull.clear ();
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,2000;2000,2000;2000,0/200,200;1800,200;1800,1800;200,1800)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  c1.shapes (l1).clear ();
  c1.shapes (l2).clear ();

  c1.shapes (l1).insert (db::Box (0, 0, 2000, 100));
  c1.shapes (l1).insert (db::Box (0, 150, 2000, 200));
  c1.shapes (l1).insert (db::Box (0, 1800, 2000, 1900));
  c1.shapes (l1).insert (db::Box (0, 1950, 2000, 2000));
  c1.shapes (l1).insert (db::Box (1800, 0, 2000, 1900));
  c1.shapes (l1).insert (db::Box (1950, 0, 2000, 2000));
  c1.shapes (l1).insert (db::Box (0, 0, 100, 2000));
  c1.shapes (l1).insert (db::Box (150, 0, 200, 2000));

  hull.clear ();
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,2000;2000,2000;2000,0/200,100;1800,100;1800,150;200,150/100,200;150,200;150,1800;100,1800/200,200;1800,200;1800,1800;200,1800/200,1900;1950,1900;1950,1950;200,1950)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  c1.shapes (l1).clear ();
  c1.shapes (l2).clear ();

  c1.shapes (l1).insert (db::Box (0, 0, 200, 200));
  c1.shapes (l1).insert (db::Box (1800, 1800, 2000, 2000));

  hull.clear ();
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,200;200,200;200,0);(1800,1800;1800,2000;2000,2000;2000,1800)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  c1.shapes (l1).clear ();
  c1.shapes (l2).clear ();

  {
    db::Point pts[3] = { db::Point(0, 0), db::Point(0, 200), db::Point(200, 200) };
    db::Polygon poly;
    poly.assign_hull (&pts[0], &pts[0] + 3);
    c1.shapes (l1).insert (poly);
  }
  {
    db::Point pts[3] = { db::Point(1800, 1800), db::Point(1800, 2000), db::Point(2000, 2000) };
    db::Polygon poly;
    poly.assign_hull (&pts[0], &pts[0] + 3);
    c1.shapes (l1).insert (poly);
  }

  hull.clear ();
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,200;200,200;200,0);(1800,1800;1800,2000;2000,2000;2000,1800)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  c1.shapes (l1).clear ();
  c1.shapes (l2).clear ();

  c1.shapes (l1).insert (db::Box (0, 0, 200, 200));
  c1.shapes (l1).insert (db::Box (1800, 1800, 2000, 2000));
  c1.shapes (l1).insert (db::Box (1500, 1800, 1700, 2000));
  c1.shapes (l1).insert (db::Box (1500, 1500, 1700, 1700));

  hull.clear ();
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,200;200,200;200,0);(1500,1500;1500,2000;2000,2000;2000,1800;1700,1800;1700,1500)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  c1.shapes (l1).clear ();
  c1.shapes (l2).clear ();

  c1.shapes (l1).insert (db::Box (1900, 0, 2000, 2000));
  c1.shapes (l1).insert (db::Box (1800, 0, 1900, 1950));
  c1.shapes (l1).insert (db::Box (1700, 0, 1800, 1900));
  c1.shapes (l1).insert (db::Box (1600, 0, 1700, 1950));
  c1.shapes (l1).insert (db::Box (1500, 0, 1600, 1850));
  c1.shapes (l1).insert (db::Box (1400, 0, 1500, 1950));
  c1.shapes (l1).insert (db::Box (1300, 0, 1400, 1900));
  c1.shapes (l1).insert (db::Box (1200, 0, 1300, 1750));
  c1.shapes (l1).insert (db::Box (1100, 0, 1200, 1800));
  c1.shapes (l1).insert (db::Box (1000, 0, 1100, 1950));
  c1.shapes (l1).insert (db::Box ( 900, 0, 1000, 1800));
  c1.shapes (l1).insert (db::Box ( 800, 0,  900, 1750));
  c1.shapes (l1).insert (db::Box ( 700, 0,  800, 1700));
  c1.shapes (l1).insert (db::Box ( 600, 0,  700, 1750));
  c1.shapes (l1).insert (db::Box ( 500, 0,  600, 1850));
  c1.shapes (l1).insert (db::Box ( 400, 0,  500, 1900));
  c1.shapes (l1).insert (db::Box ( 300, 0,  400, 1950));
  c1.shapes (l1).insert (db::Box ( 200, 0,  300, 1750));
  c1.shapes (l1).insert (db::Box ( 100, 0,  200, 1800));
  c1.shapes (l1).insert (db::Box (   0, 0,  100, 1950));

  hull.clear ();
  chg.set_complexity (0);
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,1950;1000,1950;1000,2000;2000,2000;2000,0)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  hull.clear ();
  chg.set_complexity (20);
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,1950;400,1950;400,1900;600,1900;600,1800;1000,1800;1000,1950;1900,1950;1900,2000;2000,2000;2000,0)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  hull.clear ();
  chg.set_complexity (40);
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,1950;400,1950;400,1900;500,1900;500,1850;600,1850;600,1800;1000,1800;1000,1950;1200,1950;1200,1900;1400,1900;1400,1950;1900,1950;1900,2000;2000,2000;2000,0)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);

  hull.clear ();
  chg.set_complexity (1000);
  chg.generate_hull (c1, hull);
  EXPECT_EQ (h2s (hull), "(0,0;0,1950;100,1950;100,1800;200,1800;200,1750;300,1750;300,1950;400,1950;400,1900;500,1900;500,1850;600,1850;600,1750;700,1750;700,1700;800,1700;800,1750;900,1750;900,1800;1000,1800;1000,1950;1100,1950;1100,1800;1200,1800;1200,1750;1300,1750;1300,1900;1400,1900;1400,1950;1500,1950;1500,1850;1600,1850;1600,1950;1700,1950;1700,1900;1800,1900;1800,1950;1900,1950;1900,2000;2000,2000;2000,0)");
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l2)), true);
}

TEST(2) 
{
  db::Manager m (true);
  db::Layout g (&m);
  unsigned int l1 = g.insert_layer (db::LayerProperties (1, 0)); 
  db::Cell &c1 (g.cell (g.add_cell ()));

  for (size_t i = 0; i < 10000; ++i) {
    db::Coord x = rand () % 2000;
    db::Coord y = rand () % 2000;
    c1.shapes (l1).insert (db::Box (x, y, x + 100, y + 100));
  }

  db::CellHullGenerator chg (g);

  std::vector <db::Polygon> hull;
  chg.generate_hull (c1, hull);
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);

  hull.clear ();
  chg.set_complexity (0);
  chg.generate_hull (c1, hull);
  EXPECT_EQ (hull.size (), size_t (1));
  EXPECT_EQ (hull.front ().holes (), size_t (0));
  EXPECT_EQ (hull.front ().hull ().size () <= 10, true);
  EXPECT_EQ (check_hull (hull, c1.shapes (l1)), true);
}




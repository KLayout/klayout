
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#include "tlUnitTest.h"

#include "dbEdges.h"
#include "dbEdgesUtils.h"
#include "dbPolygonTools.h"
#include "dbRegion.h"

#include <cstdlib>

TEST(1) 
{
  db::Edges r;
  EXPECT_EQ (r.to_string (), "");
  EXPECT_EQ (r == db::Edges (), true);
  EXPECT_EQ (r < db::Edges (), false);
  EXPECT_EQ (r != db::Edges (), false);
  EXPECT_EQ (r.bbox ().to_string (), "()");
  EXPECT_EQ (r.empty (), true);
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.begin ().at_end (), true);

  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  EXPECT_EQ (r == db::Edges (), false);
  EXPECT_EQ (r < db::Edges (), true);
  EXPECT_EQ (r != db::Edges (), true);
  EXPECT_EQ (r != r, false);
  EXPECT_EQ (r == r, true);
  EXPECT_EQ (r < r, false);
  EXPECT_EQ (r.to_string (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)");
  EXPECT_EQ (r.transformed (db::Trans (db::Vector (1, 2))).to_string (), "(1,2;1,202);(1,202;101,202);(101,202;101,2);(101,2;1,2)");
  EXPECT_EQ (r.bbox ().to_string (), "(0,0;100,200)");
  EXPECT_EQ (r.transformed (db::Trans (db::Vector (1, 2))).bbox ().to_string (), "(1,2;101,202)");
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.begin ().at_end (), false);

  db::Edges r1 = r;
  db::Edges r2;
  EXPECT_EQ (r1.to_string (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)");
  EXPECT_EQ (r1.merged ().to_string (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)");
  EXPECT_EQ (r2.to_string (), "");
  EXPECT_EQ (r1.bbox ().to_string (), "(0,0;100,200)");
  EXPECT_EQ (r2.bbox ().to_string (), "()");
  r1.swap (r2);
  EXPECT_EQ (r1.to_string (), "");
  EXPECT_EQ (r2.to_string (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)");
  EXPECT_EQ (r1.bbox ().to_string (), "()");
  EXPECT_EQ (r2.bbox ().to_string (), "(0,0;100,200)");

  EXPECT_EQ ((r | db::Edges (db::Box (db::Point (10, 0), db::Point (110, 200)))).to_string (), "(0,0;0,200);(100,200;100,0);(10,0;10,200);(0,200;110,200);(110,200;110,0);(110,0;0,0)");
  EXPECT_EQ ((r + db::Edges (db::Box (db::Point (10, 0), db::Point (110, 200)))).to_string (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0);(10,0;10,200);(10,200;110,200);(110,200;110,0);(110,0;10,0)");

  db::Edges rr = r;
  rr |= db::Edges (db::Box (db::Point (10, 0), db::Point (110, 200)));
  EXPECT_EQ (rr.is_merged (), true);
  EXPECT_EQ (rr.to_string (), "(0,0;0,200);(100,200;100,0);(10,0;10,200);(0,200;110,200);(110,200;110,0);(110,0;0,0)");

  r += db::Edges (db::Box (db::Point (10, 0), db::Point (110, 200)));
  EXPECT_EQ (r.to_string (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0);(10,0;10,200);(10,200;110,200);(110,200;110,0);(110,0;10,0)");
  EXPECT_EQ (r.is_merged (), false);
  EXPECT_EQ (r.size (), size_t (8));
  r.set_merged_semantics (false);
  EXPECT_EQ (r.length (), db::Edges::length_type (1200));
  EXPECT_EQ (r.length (db::Box (db::Point (-10, -10), db::Point (50, 50))), db::Edges::length_type (190));
  EXPECT_EQ (r.length (db::Box (db::Point (-10, -10), db::Point (0, 50))), db::Edges::length_type (0));
  EXPECT_EQ (r.length (db::Box (db::Point (0, 0), db::Point (50, 50))), db::Edges::length_type (190));
  r.set_merged_semantics (true);
  EXPECT_EQ (r.length (), db::Edges::length_type (1020));
  EXPECT_EQ (r.length (db::Box (db::Point (-10, -10), db::Point (50, 50))), db::Edges::length_type (150));
  EXPECT_EQ (r.length (db::Box (db::Point (-10, -10), db::Point (0, 50))), db::Edges::length_type (0));
  EXPECT_EQ (r.length (db::Box (db::Point (0, 0), db::Point (50, 50))), db::Edges::length_type (150));
  r.merge ();
  EXPECT_EQ (r.to_string (), "(0,0;0,200);(100,200;100,0);(10,0;10,200);(0,200;110,200);(110,200;110,0);(110,0;0,0)");
  EXPECT_EQ (r.bbox ().to_string (), "(0,0;110,200)");
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.size (), size_t (6));
  EXPECT_EQ (r.length (), db::Edges::length_type (1020));

  r.clear ();
  EXPECT_EQ (r.empty (), true);
  EXPECT_EQ (r.is_merged (), true);
}

TEST(2) 
{
  db::Edges r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));

  db::Edges r1;
  r1.insert (db::Box (db::Point (10, 0), db::Point (110, 200)));

  db::Edges r2;
  r2.insert (db::Box (db::Point (0, 10), db::Point (100, 210)));

  EXPECT_EQ ((r & r1).to_string (), "(10,200;100,200);(100,0;10,0)");
  EXPECT_EQ ((r & r2).to_string (), "(0,10;0,200);(100,200;100,10)");
  db::Edges o1 = r;
  o1 &= r1;
  EXPECT_EQ (o1.is_merged (), true);
  EXPECT_EQ (o1.to_string (), "(10,200;100,200);(100,0;10,0)");

  EXPECT_EQ ((r - r1).to_string (), "(0,0;0,200);(100,200;100,0);(0,200;10,200);(10,0;0,0)");
  db::Edges o2 = r;
  o2 -= r1;
  EXPECT_EQ (o2.is_merged (), true);
  EXPECT_EQ (o2.to_string (), "(0,0;0,200);(100,200;100,0);(0,200;10,200);(10,0;0,0)");

  EXPECT_EQ ((r ^ r1).to_string (), "(0,0;0,200);(100,200;100,0);(10,0;10,200);(0,200;10,200);(100,200;110,200);(110,200;110,0);(110,0;100,0);(10,0;0,0)");
  db::Edges o3 = r;
  o3 ^= r1;
  EXPECT_EQ (o3.is_merged (), true);
  EXPECT_EQ (o3.to_string (), "(0,0;0,200);(100,200;100,0);(10,0;10,200);(0,200;10,200);(100,200;110,200);(110,200;110,0);(110,0;100,0);(10,0;0,0)");

  r.clear ();
  r.insert (db::Box (db::Point (1000, 0), db::Point (6000, 4000)));
  r1.clear ();
  r1.insert (db::Box (db::Point (0, 4000), db::Point (2000, 6000)));

  EXPECT_EQ ((r & r1).to_string (), "(1000,4000;2000,4000)");
  EXPECT_EQ ((r1 & r).to_string (), "(2000,4000;1000,4000)");
}

TEST(3) 
{
  db::Edges r;
  r.insert (db::Edge (db::Point (0, 0), db::Point (100, 10)));
  r.insert (db::Edge (db::Point (50, 5), db::Point (150, 15)));
  r.insert (db::Edge (db::Point (200, 20), db::Point (220, 22)));
  r.insert (db::Edge (db::Point (220, 22), db::Point (230, 23)));
  db::Edges rr;
  rr.insert (db::Edge (db::Point (10, 1), db::Point (60, 6)));
  rr.insert (db::Edge (db::Point (50, 5), db::Point (70, 7)));

  EXPECT_EQ (r.merged ().to_string (), "(0,0;150,15);(200,20;230,23)");
  EXPECT_EQ (rr.merged ().to_string (), "(10,1;70,7)");
  EXPECT_EQ ((r ^ rr).to_string (), "(200,20;230,23);(0,0;10,1);(70,7;150,15)");
  EXPECT_EQ ((rr ^ r).to_string (), "(0,0;10,1);(70,7;150,15);(200,20;230,23)");
  EXPECT_EQ ((r - rr).to_string (), "(200,20;230,23);(0,0;10,1);(70,7;150,15)");
  EXPECT_EQ ((rr - r).to_string (), "");
  EXPECT_EQ ((r & rr).to_string (), "(10,1;70,7)");
  EXPECT_EQ ((rr & r).to_string (), "(10,1;70,7)");
}

TEST(4) 
{
  db::Edges r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Edge (db::Point (200, 0), db::Point (250, 200)));
  r.insert (db::Edge (db::Point (250, 200), db::Point (300, 0)));
  r.insert (db::Edge (db::Point (300, 0), db::Point (200, 0)));
  r.insert (db::Edge (db::Point (200, 0), db::Point (250, -200)));
  r.insert (db::Edge (db::Point (250, -200), db::Point (300, 0)));

  {
    db::EdgeLengthFilter f1 (100, 101, false);
    db::Edges rr = r;
    rr.filter (f1);
    EXPECT_EQ (rr.to_string (), "(0,200;100,200);(100,0;0,0);(300,0;200,0)");
  }
  {
    db::EdgeLengthFilter f1 (201, 1000, false);
    db::Edges rr = r;
    rr.filter (f1);
    EXPECT_EQ (rr.to_string (), "(200,0;250,200);(250,200;300,0);(200,0;250,-200);(250,-200;300,0)");
  }
  {
    db::EdgeLengthFilter f1 (201, 1000, true);
    db::Edges rr = r;
    rr.filter (f1);
    EXPECT_EQ (rr.to_string (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0);(300,0;200,0)");
  }
  {
    db::EdgeOrientationFilter f1 (0.0, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "(0,200;100,200);(100,0;0,0);(300,0;200,0)");
  }
  {
    db::EdgeOrientationFilter f1 (50.0, 80.0, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "(200,0;250,200);(250,-200;300,0)");
  }
  {
    db::EdgeOrientationFilter f1 (50.0, 80.0, true);
    EXPECT_EQ (r.filtered (f1).to_string (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0);(250,200;300,0);(300,0;200,0);(200,0;250,-200)");
  }
  {
    db::EdgeOrientationFilter f1 (0.0, 1.0, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "(0,200;100,200);(100,0;0,0);(300,0;200,0)");
  }
  {
    db::EdgeOrientationFilter f1 (-1.0, 1.0, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "(0,200;100,200);(100,0;0,0);(300,0;200,0)");
  }
  {
    db::EdgeOrientationFilter f1 (-1.0, 0.0, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "");
  }
  {
    db::EdgeOrientationFilter f1 (90.0, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "(0,0;0,200);(100,200;100,0)");
  }
  {
    db::EdgeOrientationFilter f1 (90.0, 91.0, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "(0,0;0,200);(100,200;100,0)");
  }
  {
    db::EdgeOrientationFilter f1 (89.0, 91.0, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "(0,0;0,200);(100,200;100,0)");
  }
  {
    db::EdgeOrientationFilter f1 (89.0, 90.0, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "");
  }
}

TEST(5) 
{
  db::Edges r;
  r.insert (db::Polygon (db::Box (db::Point (0, 0), db::Point (100, 200))));
  EXPECT_EQ (r.to_string (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)");
  r.clear ();
  r.insert (db::SimplePolygon (db::Box (db::Point (0, 0), db::Point (100, 200))));
  EXPECT_EQ (r.to_string (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)");
  r.transform (db::ICplxTrans (2.5));
  EXPECT_EQ (r.to_string (), "(0,0;0,500);(0,500;250,500);(250,500;250,0);(250,0;0,0)");

  db::Edges rr;
  std::string s = r.to_string ();
  tl::Extractor ex (s.c_str ());
  EXPECT_EQ (ex.try_read (rr), true);
  EXPECT_EQ (rr.to_string (), "(0,0;0,500);(0,500;250,500);(250,500;250,0);(250,0;0,0)");
}

TEST(6) 
{
  db::Edges e;
  e.insert (db::Edge (db::Point (0, 0), db::Point (0, 200)));
  e.insert (db::Edge (db::Point (0, 200), db::Point (100, 200)));
  e.insert (db::Edge (db::Point (200, 0), db::Point (250, 200)));
  e.insert (db::Edge (db::Point (250, 200), db::Point (300, 0)));

  db::Region r;
  e.extended (r, 0, 0, 20, 0, false);
  EXPECT_EQ (r.to_string (), "(-20,0;-20,200;0,200;0,0);(0,200;0,220;100,220;100,200);(200,0;181,5;231,205;250,200);(300,0;250,200;269,205;319,5)");

  r.clear ();
  e.extended (r, 0, 0, 20, 0, true);
  EXPECT_EQ (r.to_string (), "(-20,0;-20,220;100,220;100,200;0,200;0,0);(200,0;181,5;235,224;265,224;319,5;300,0;250,200)");

  r.clear ();
  e.extended (r, 0, 0, 0, 10, false);
  EXPECT_EQ (r.to_string (), "(0,0;0,200;10,200;10,0);(0,190;0,200;100,200;100,190);(210,-2;200,0;250,200;260,198);(290,-2;240,198;250,200;300,0)");

  r.clear ();
  e.extended (r, 0, 0, 0, 10, true);
  EXPECT_EQ (r.to_string (), "(0,0;0,200;100,200;100,190;10,190;10,0);(210,-2;200,0;250,200;300,0;290,-2;250,159)");

  r.clear ();
  e.extended (r, 10, 20, 0, 10, true);
  EXPECT_EQ (r.to_string (), "(0,-10;0,200;120,200;120,190;10,190;10,-10);(295,-22;250,159;207,-12;198,-10;250,200;305,-19)");

  r.clear ();
  e.extended (r, 10, 20, 0, 10, false);
  EXPECT_EQ (r.to_string (), "(0,-10;0,220;10,220;10,-10);(-10,190;-10,200;120,200;120,190);(207,-12;198,-10;255,219;265,217);(295,-22;238,207;248,210;305,-19)");

  r.clear ();
  e.extended (r, 10, 20, 20, -10, false);
  EXPECT_EQ (r.to_string (), "(-20,-10;-20,220;-10,220;-10,-10);(-10,210;-10,220;120,220;120,210);(188,-7;178,-5;235,224;245,222);(315,-17;257,212;267,215;324,-15)");

  /* This is not working properly yet:
   * Apparently db::Path is not able to produce the right inner corner.
  r.clear ();
  e.extended (r, 10, 20, 20, -10, true);
  EXPECT_EQ (r.to_string (), "(-20,-10;-20,220;120,220;120,210;-10,210;-10,-10);...");
  */

  e.clear ();
  e.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));

  r.clear ();
  e.extended (r, 10, 20, 0, 10, false);
  EXPECT_EQ (r.to_string (), "(0,-10;0,220;10,220;10,-10);(-10,190;-10,200;120,200;120,190);(90,-20;90,210;100,210;100,-20);(-20,0;-20,10;110,10;110,0)");

  r.clear ();
  e.extended (r, 10, 20, 0, 10, true);
  EXPECT_EQ (r.to_string (), "(0,0;0,200;100,200;100,0/10,10;90,10;90,190;10,190)");

  r.clear ();
  e.extended (r, 10, 20, 20, -10, true);
  EXPECT_EQ (r.to_string (), "(-20,-20;-20,220;120,220;120,-20/-10,-10;110,-10;110,210;-10,210)");
}

TEST(6b)
{
  //  Ticket #90: order of edges as input to the edge collector should not matter

  db::Edges e;
  e.insert (db::Edge (db::Point (0, -200), db::Point (100, -200)));
  e.insert (db::Edge (db::Point (250, -200), db::Point (300, 0)));
  e.insert (db::Edge (db::Point (0, 0), db::Point (0, -200)));
  e.insert (db::Edge (db::Point (200, 0), db::Point (250, -200)));

  db::Region r;
  e.extended (r, 0, 0, 20, 0, true);
  EXPECT_EQ (r.to_string (), "(0,-200;0,0;20,0;20,-180;100,-180;100,-200);(250,-200;200,0;219,5;250,-118;281,5;300,0)");
}

TEST(6c)
{
  //  A more complex scenario with forks

  db::Edges e;
  e.insert (db::Edge (db::Point (0, -200), db::Point (100, -200)));
  e.insert (db::Edge (db::Point (250, -200), db::Point (300, 0)));
  e.insert (db::Edge (db::Point (0, 0), db::Point (0, -200)));
  e.insert (db::Edge (db::Point (0, -100), db::Point (0, -200)));
  e.insert (db::Edge (db::Point (200, 0), db::Point (250, -200)));
  e.insert (db::Edge (db::Point (0, -200), db::Point (200, -200)));
  e.insert (db::Edge (db::Point (250, -200), db::Point (350, 0)));

  db::Region r;
  e.extended (r, 0, 0, 20, 0, true);
  EXPECT_EQ (r.to_string (), "(0,-200;0,0;20,0;20,-180;100,-180;100,-200);(0,-200;0,-100;20,-100;20,-180;200,-180;200,-200);(250,-200;200,0;219,5;250,-118;281,5;300,0);(250,-200;232,-191;332,9;350,0)");
}

TEST(7)
{
  db::Edges e;
  e.insert (db::Edge (db::Point (0, 0), db::Point (0, 200)));
  e.insert (db::Edge (db::Point (250, 200), db::Point (300, 0)));

  EXPECT_EQ (e.start_segments (10, 0).to_string (), "(0,0;0,10);(250,200;252,190)");
  EXPECT_EQ (e.start_segments (10, 0.25).to_string (), "(0,0;0,50);(250,200;263,150)");
  EXPECT_EQ (e.start_segments (0, 1.0).to_string (), "(0,0;0,200);(250,200;300,0)");
  EXPECT_EQ (e.start_segments (0, 0).to_string (), "(0,0;0,0);(250,200;250,200)");

  EXPECT_EQ (e.end_segments (10, 0).to_string (), "(0,190;0,200);(298,10;300,0)");
  EXPECT_EQ (e.end_segments (10, 0.25).to_string (), "(0,150;0,200);(288,50;300,0)");
  EXPECT_EQ (e.end_segments (0, 1.0).to_string (), "(0,0;0,200);(250,200;300,0)");
  EXPECT_EQ (e.end_segments (0, 0).to_string (), "(0,200;0,200);(300,0;300,0)");

  EXPECT_EQ (e.centers (10, 0).to_string (), "(0,95;0,105);(274,105;276,95)");
  EXPECT_EQ (e.centers (10, 0.25).to_string (), "(0,75;0,125);(269,125;281,75)");
  EXPECT_EQ (e.centers (0, 1.0).to_string (), "(0,0;0,200);(250,200;300,0)");
  EXPECT_EQ (e.centers (0, 0).to_string (), "(0,100;0,100);(275,100;275,100)");
}

TEST(8) 
{
  db::Edges e;
  e.insert (db::Edge (db::Point (0, 0), db::Point (0, 200)));
  e.insert (db::Edge (db::Point (250, 200), db::Point (300, 0)));

  db::Edges e2;
  e2.insert (db::Edge (db::Point (0, 100), db::Point (100, 100)));

  EXPECT_EQ (e.selected_interacting (e2).to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_not_interacting (e2).to_string (), "(250,200;300,0)");

  e2.clear ();
  e2.insert (db::Edge (db::Point (0, 100), db::Point (0, 100)));

  EXPECT_EQ (e.selected_interacting (e2).to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_not_interacting (e2).to_string (), "(250,200;300,0)");

  e2.clear ();
  e2.insert (db::Edge (db::Point (100, 0), db::Point (0, 0)));

  EXPECT_EQ (e.selected_interacting (e2).to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_not_interacting (e2).to_string (), "(250,200;300,0)");

  e2.clear ();
  e2.insert (db::Edge (db::Point (-100, -1), db::Point (100, -1)));

  EXPECT_EQ (e.selected_interacting (e2).to_string (), "");
  EXPECT_EQ (e.selected_not_interacting (e2).to_string (), "(0,0;0,200);(250,200;300,0)");

  e2.clear ();
  e2.insert (db::Edge (db::Point (-100, 0), db::Point (100, 0)));

  EXPECT_EQ (e.selected_interacting (e2).to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_not_interacting (e2).to_string (), "(250,200;300,0)");

  db::Edges ee = e;
  e.select_interacting (e2);
  EXPECT_EQ (e.to_string (), "(0,0;0,200)");

  ee.select_not_interacting (e2);
  EXPECT_EQ (ee.to_string (), "(250,200;300,0)");
}

TEST(9) 
{
  for (unsigned int seed = 1; seed < 20; ++seed) {

    srand(seed);

    for (int pass = 0; pass < 10; ++pass) {

      int d = pass >= 5 ? 10 : 1000;

      db::Edges e;
      for (int i = 0; i < 100; ++i) {
        e.insert (db::Edge (db::Point (abs (rand ()) % d, abs (rand ()) % d), db::Point (abs (rand ()) % d, abs (rand ()) % d)));
        db::Point p (abs (rand ()) % d, abs (rand ()) % d);
        e.insert (db::Edge (p, p));
      }

      db::Edges e2;
      for (int i = 0; i < 2; ++i) {
        e2.insert (db::Edge (db::Point (abs (rand ()) % d, abs (rand ()) % d), db::Point (abs (rand ()) % d, abs (rand ()) % d)));
        db::Point p (abs (rand ()) % d, abs (rand ()) % d);
        e2.insert (db::Edge (p, p));
      }

      std::set<db::Edge> ea, eb;

      e.set_merged_semantics (false);
      db::Edges ia = e.selected_interacting (e2);
      for (db::Edges::const_iterator i = ia.begin (); ! i.at_end (); ++i) {
        ea.insert (*i);
      }

      EXPECT_NE (ea.size (), size_t (0));

      //  brute force
      for (db::Edges::const_iterator i = e.begin (); ! i.at_end (); ++i) {
        for (db::Edges::const_iterator j = e2.begin (); ! j.at_end (); ++j) {
          if (i->intersect (*j)) {
            eb.insert (*i);
          }
        }
      }

      if (ea != eb) {
        tl::info << "Seed = " << seed;
        tl::info << "In implementation but not in brute-force:";
        for (std::set<db::Edge>::const_iterator i = ea.begin (); i != ea.end (); ++i) {
          if (eb.find (*i) == eb.end ()) {
            tl::info << "  " << i->to_string ();
          }
        }
        tl::info << "In brute-force but not in implementation:";
        for (std::set<db::Edge>::const_iterator i = eb.begin (); i != eb.end (); ++i) {
          if (ea.find (*i) == ea.end ()) {
            tl::info << "  " << i->to_string ();
          }
        }
        EXPECT_EQ (true, false);
      }

    }

  }
}

TEST(10) 
{
  for (unsigned int seed = 1; seed < 20; ++seed) {

    srand(seed);

    for (int pass = 0; pass < 10; ++pass) {

      int d = pass >= 5 ? 10 : 1000;

      db::Edges e;
      for (int i = 0; i < 100; ++i) {
        e.insert (db::Edge (db::Point (abs (rand ()) % d, abs (rand ()) % d), db::Point (abs (rand ()) % d, abs (rand ()) % d)));
        db::Point p (abs (rand ()) % d, abs (rand ()) % d);
        e.insert (db::Edge (p, p));
      }

      db::Region r;
      for (int i = 0; i < 2; ++i) {
        db::Box b;
        do {
          b = db::Box (db::Point (abs (rand ()) % d, abs (rand ()) % d), db::Point (abs (rand ()) % d, abs (rand ()) % d));
        } while (b.width () == 0 || b.height () == 0);
        r.insert (b);
      }

      std::set<db::Edge> ea, eb;

      e.set_merged_semantics (false);
      db::Edges ia = e.selected_interacting (r);
      for (db::Edges::const_iterator i = ia.begin (); ! i.at_end (); ++i) {
        ea.insert (*i);
      }

      EXPECT_NE (ea.size (), size_t (0));

      //  brute force
      for (db::Edges::const_iterator i = e.begin (); ! i.at_end (); ++i) {
        for (db::Region::const_iterator j = r.begin (); ! j.at_end (); ++j) {
          if (db::interact (*j, *i)) {
            eb.insert (*i);
          }
        }
      }

      if (ea != eb) {
        tl::info << "Seed = " << seed;
        tl::info << "Boxes:";
        for (db::Region::const_iterator j = r.begin (); ! j.at_end (); ++j) {
          tl::info << "  " << j->to_string ();
        }
        tl::info << "In implementation but not in brute-force:";
        for (std::set<db::Edge>::const_iterator i = ea.begin (); i != ea.end (); ++i) {
          if (eb.find (*i) == eb.end ()) {
            tl::info << "  " << i->to_string ();
          }
        }
        tl::info << "In brute-force but not in implementation:";
        for (std::set<db::Edge>::const_iterator i = eb.begin (); i != eb.end (); ++i) {
          if (ea.find (*i) == ea.end ()) {
            tl::info << "  " << i->to_string ();
          }
        }
        EXPECT_EQ (true, false);
      }

    }

  }
}

TEST(11) 
{
  db::Box bb[3] = { db::Box (db::Point (0, 0), db::Point (10, 10)), db::Box (), db::Box (db::Point (20, 20), db::Point (40, 50)) };
  db::Region r (bb + 0, bb + 3);

  EXPECT_EQ (r.edges ().width_check (15).to_string (), "(0,0;0,10)/(10,10;10,0);(0,10;10,10)/(10,0;0,0)");
  EXPECT_EQ (r.edges ().width_check (5).to_string (), "");
  EXPECT_EQ (r.edges ().width_check (5, false, db::Euclidian, 91).to_string (), "(0,5;0,10)/(0,10;5,10);(0,0;0,5)/(5,0;0,0);(5,10;10,10)/(10,10;10,5);(10,5;10,0)/(10,0;5,0);(20,45;20,50)/(20,50;25,50);(20,20;20,25)/(25,20;20,20);(35,50;40,50)/(40,50;40,45);(40,25;40,20)/(40,20;35,20)");
  EXPECT_EQ (r.edges ().space_check (15, false, db::Euclidian, 91).to_string (), "(9,10;10,10)/(20,20;20,21);(9,10;10,10)/(21,20;20,20);(10,10;10,9)/(20,20;20,21);(10,10;10,9)/(21,20;20,20)");
  EXPECT_EQ (r.edges ().space_check (15, false, db::Square, 91).to_string (), "(5,10;10,10)/(20,20;20,25);(5,10;10,10)/(25,20;20,20);(10,10;10,5)/(20,20;20,25);(10,10;10,5)/(25,20;20,20)");
  EXPECT_EQ (r.edges ().space_check (15).to_string (), "(9,10;10,10)/(21,20;20,20);(10,10;10,9)/(20,20;20,21)");
  EXPECT_EQ (r.edges ().space_check (15, true).to_string (), "(0,10;10,10)/(40,20;20,20);(10,10;10,0)/(20,20;20,50)");
  EXPECT_EQ (r.edges ().space_check (15, false, db::Square).to_string (), "(5,10;10,10)/(25,20;20,20);(10,10;10,5)/(20,20;20,25)");
}

TEST(12) 
{
  db::Region a;
  a.insert (db::Box (db::Point (10, 20), db::Point (20, 30)));

  db::Region b;
  b.insert (db::Box (db::Point (0, 0), db::Point (100, 100)));

  EXPECT_EQ (a.edges ().inside_check (b.edges (), 15).to_string (), "(10,20;10,30)/(0,9;0,41)");
  EXPECT_EQ (a.edges ().inside_check (b.edges (), 15, true).to_string (), "(10,20;10,30)/(0,0;0,100)");
  EXPECT_EQ (a.edges ().inside_check (b.edges (), 15, false, db::Euclidian, 91).to_string (), "(10,20;10,30)/(0,9;0,41);(10,30;15,30)/(0,30;0,41);(15,20;10,20)/(0,9;0,20)");
  EXPECT_EQ (b.edges ().enclosing_check (a.edges (), 15).to_string (), "(0,9;0,41)/(10,20;10,30)");
  EXPECT_EQ (b.edges ().enclosing_check (a.edges (), 15, true).to_string (), "(0,0;0,100)/(10,20;10,30)");
  EXPECT_EQ (b.edges ().enclosing_check (a.edges (), 15, false, db::Euclidian, 91).to_string (), "(0,9;0,41)/(10,20;10,30);(0,30;0,41)/(10,30;15,30);(0,9;0,20)/(15,20;10,20)");

  b.clear ();
  b.insert (db::Box (db::Point (30, 0), db::Point (100, 100)));
  EXPECT_EQ (b.separation_check (a, 15).to_string (), "(30,9;30,41)/(20,30;20,20)");
  EXPECT_EQ (b.separation_check (a, 15, true).to_string (), "(30,0;30,100)/(20,30;20,20)");
  EXPECT_EQ (b.separation_check (a, 15, false, db::Euclidian, 91).to_string (), "(30,30;30,41)/(15,30;20,30);(30,9;30,41)/(20,30;20,20);(30,9;30,20)/(20,20;15,20)");

  b.clear ();
  b.insert (db::Box (db::Point (15, 0), db::Point (100, 100)));
  EXPECT_EQ (b.overlap_check (a, 15).to_string (), "(15,6;15,44)/(20,30;20,20)");
  EXPECT_EQ (b.overlap_check (a, 15, true).to_string (), "(15,0;15,100)/(20,30;20,20)");
  EXPECT_EQ (b.overlap_check (a, 15, false, db::Euclidian, 91).to_string (), "(15,15;15,30)/(15,30;20,30);(15,6;15,44)/(20,30;20,20);(15,20;15,35)/(20,20;15,20)");
}

TEST(20)
{
  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));
  unsigned int lp1 = ly.insert_layer (db::LayerProperties (10, 0));
  unsigned int l2 = ly.insert_layer (db::LayerProperties (2, 0));
  db::cell_index_type top = ly.add_cell ("TOP");
  db::cell_index_type c1 = ly.add_cell ("C1");
  db::cell_index_type c2 = ly.add_cell ("C2");
  ly.cell (c1).shapes (l1).insert (db::Edge (0, 0, 0, 30));
  ly.cell (c1).shapes (l1).insert (db::Edge (0, 30, 30, 30));
  ly.cell (c1).shapes (l1).insert (db::Edge (30, 30, 30, 0));
  ly.cell (c1).shapes (l1).insert (db::Edge (30, 0, 0, 0));
  ly.cell (c2).shapes (l2).insert (db::Edge (0, 0, 0, 30));
  ly.cell (c2).shapes (l2).insert (db::Edge (0, 30, 30, 30));
  ly.cell (c2).shapes (l2).insert (db::Edge (30, 30, 30, 0));
  ly.cell (c2).shapes (l2).insert (db::Edge (30, 0, 0, 0));
  ly.cell (c1).shapes (lp1).insert (db::Box (0, 0, 30, 30));
  ly.cell (top).insert (db::CellInstArray (c1, db::Trans (db::Vector (0, 0))));
  ly.cell (top).insert (db::CellInstArray (c1, db::Trans (db::Vector (50, 0))));
  ly.cell (top).insert (db::CellInstArray (c1, db::Trans (db::Vector (50, 40))));
  ly.cell (top).insert (db::CellInstArray (c2, db::Trans (db::Vector (10, 10))));
  ly.cell (top).insert (db::CellInstArray (c2, db::Trans (db::Vector (80, 40))));
  ly.cell (top).insert (db::CellInstArray (c2, db::Trans (db::Vector (110, 40))));
  ly.cell (top).shapes (l2).insert (db::Edge (60, 10, 60, 20));
  ly.cell (top).shapes (l2).insert (db::Edge (60, 20, 70, 20));
  ly.cell (top).shapes (l2).insert (db::Edge (70, 20, 70, 10));
  ly.cell (top).shapes (l2).insert (db::Edge (70, 10, 60, 10));

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l1), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.to_string (100), "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(50,70;80,70);(80,70;80,40);(80,40;50,40)");
    EXPECT_EQ (r1.has_valid_edges (), false);
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2, db::Box (60, 10, 90, 50)), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.to_string (), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(80,40;80,70);(110,40;80,40)");
    EXPECT_EQ (r1.has_valid_edges (), false);
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2, db::Box (60, 10, 90, 50)), db::ICplxTrans (2.0), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.to_string (), "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20);(160,80;160,140);(220,80;160,80)");
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.length (), db::Edges::length_type (200));
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.bbox ().to_string (), "(120,20;220,140)");
    EXPECT_EQ (r1.size (), size_t (6));
    EXPECT_EQ (r1.empty (), false);

    db::EdgeLengthFilter f0 (0, 50, false);
    db::Edges rr = r1.filtered (f0);
    EXPECT_EQ (rr.has_valid_edges (), true);
    EXPECT_EQ (rr.to_string (), "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20)");

    db::Edges r2 = r1;
    EXPECT_EQ (r2.has_valid_edges (), false);
    EXPECT_EQ (r2.length (), db::Edges::length_type (200));
    EXPECT_EQ (r2.bbox ().to_string (), "(120,20;220,140)");
    EXPECT_EQ (r2.size (), size_t (6));
    EXPECT_EQ (r2.empty (), false);
    r2.filter (f0);
    EXPECT_EQ (r2.has_valid_edges (), true);
    EXPECT_EQ (r2.to_string (), "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20)");
    EXPECT_EQ (r2.size (), size_t (4));
    EXPECT_EQ (r2.empty (), false);
    EXPECT_EQ (r2.length (), db::Edges::length_type (80));

    r1.insert (db::Box (0, 0, 10, 20));
    EXPECT_EQ (r1.has_valid_edges (), true);
    EXPECT_EQ (r1.to_string (), "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20);(160,80;160,140);(220,80;160,80);(0,0;0,20);(0,20;10,20);(10,20;10,0);(10,0;0,0)");
    EXPECT_EQ (r1.to_string (2), "(120,20;120,40);(120,40;140,40)...");
    EXPECT_EQ (r1.size (), size_t (10));
    EXPECT_EQ (r1.length (), db::Edges::length_type (260));

    rr = r1.filtered (f0);
    EXPECT_EQ (rr.to_string (), "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20);(0,0;0,20);(0,20;10,20);(10,20;10,0);(10,0;0,0)");
    EXPECT_EQ (r1.to_string (), "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20);(160,80;160,140);(220,80;160,80);(0,0;0,20);(0,20;10,20);(10,20;10,0);(10,0;0,0)");

    r1.filter (f0);
    EXPECT_EQ (r1.to_string (), "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20);(0,0;0,20);(0,20;10,20);(10,20;10,0);(10,0;0,0)");
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2, db::Box (60, 10, 70, 50)), db::ICplxTrans (2.0), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.to_string (), "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20)");
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.size (), size_t (4));
    EXPECT_EQ (r1.empty (), false);

    db::Edges r2 = r1;

    EXPECT_EQ (r1.transformed (db::ICplxTrans (0.5)).to_string (), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10)");
    r1.transform (db::ICplxTrans (0.5));
    EXPECT_EQ (r1.has_valid_edges (), true);
    EXPECT_EQ (r1.to_string (), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10)");

    r1.clear ();
    EXPECT_EQ (r1.has_valid_edges (), true);
    EXPECT_EQ (r1.size (), size_t (0));
    EXPECT_EQ (r1.empty (), true);
    EXPECT_EQ (r1.length (), db::Edges::length_type (0));

    EXPECT_EQ (r2.to_string (), "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20)");
    r1.swap (r2);

    EXPECT_EQ (r1.to_string (), "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20)");
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r2.has_valid_edges (), true);
    EXPECT_EQ (r2.size (), size_t (0));
    EXPECT_EQ (r2.empty (), true);
    EXPECT_EQ (r2.length (), db::Edges::length_type (0));
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.is_merged (), false);
    EXPECT_EQ (r1.merged ().to_string (100), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,70;140,40);(140,40;80,40)");
    r1.merge ();
    EXPECT_EQ (r1.to_string (100), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,70;140,40);(140,40;80,40)");
    EXPECT_EQ (r1.has_valid_edges (), true);
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2), false);
    EXPECT_EQ (r1.width_check (20).to_string (), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10)");
    EXPECT_EQ (r1.width_check (50).to_string (), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10);(60,20;70,20)/(40,10;11,10);(70,10;60,10)/(20,40;40,40);(10,10;10,40)/(40,40;40,10);(10,40;40,40)/(40,10;10,10);(80,70;140,70)/(140,40;80,40)");
    EXPECT_EQ (r1.width_check (50, true).to_string (), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10);(60,20;70,20)/(40,10;10,10);(70,10;60,10)/(10,40;40,40);(10,10;10,40)/(40,40;40,10);(10,40;40,40)/(40,10;10,10);(80,70;140,70)/(140,40;80,40)");
    EXPECT_EQ (r1.width_check (50, false, db::Projection).to_string (), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10);(10,10;10,40)/(40,40;40,10);(10,40;40,40)/(40,10;10,10);(80,70;140,70)/(140,40;80,40)");
    EXPECT_EQ (r1.width_check (50, false, db::Euclidian, 90, 1).to_string (), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10);(10,10;10,40)/(40,40;40,10);(10,40;40,40)/(40,10;10,10);(80,70;140,70)/(140,40;80,40)");
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.space_check (30).to_string (), "(60,10;60,20)/(40,40;40,10);(60,20;70,20)/(92,40;80,40);(70,20;70,12)/(80,40;80,48)");
    EXPECT_EQ (r1.space_check (2).to_string (), "");
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l1), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    db::Edges r2 (db::RecursiveShapeIterator (ly, ly.cell (top), l2), false);
    EXPECT_EQ (r2.has_valid_edges (), false);
    EXPECT_EQ (r1.separation_check (r2, 20).to_string (), "(50,0;50,30)/(40,40;40,10);(63,30;80,30)/(97,40;80,40);(50,40;50,57)/(40,40;40,23);(80,70;80,40)/(80,40;80,70)");
    EXPECT_EQ (r1.separation_check (r2, 20, false, db::Projection).to_string (), "(50,10;50,30)/(40,30;40,10);(80,70;80,40)/(80,40;80,70)");
    EXPECT_EQ (r1.separation_check (r2, 20, false, db::Euclidian, 90, 1).to_string (), "(50,0;50,30)/(40,40;40,10);(80,70;80,40)/(80,40;80,70)");
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l1), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    db::Edges r2 (db::RecursiveShapeIterator (ly, ly.cell (top), l2), false);
    EXPECT_EQ (r2.has_valid_edges (), false);
    db::Region rr1 (db::RecursiveShapeIterator (ly, ly.cell (top), lp1), db::ICplxTrans (), false);
    EXPECT_EQ (rr1.has_valid_polygons (), false);
    EXPECT_EQ ((r1 & r2).to_string (100), "(80,70;80,40)");
    EXPECT_EQ ((r1 + r2).to_string (100), "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(50,70;80,70);(80,70;80,40);(80,40;50,40);(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(80,40;80,70);(80,70;110,70);(110,70;110,40);(110,40;80,40);(110,40;110,70);(110,70;140,70);(140,70;140,40);(140,40;110,40)");
    EXPECT_EQ ((r1 + r2).merged ().to_string (100), "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(50,70;140,70);(140,70;140,40);(140,40;50,40)");
    EXPECT_EQ ((r1 | r2).to_string (100), "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(50,70;140,70);(140,70;140,40);(140,40;50,40)");
    EXPECT_EQ ((r1 ^ r2).to_string (100), "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(50,70;140,70);(140,70;140,40);(140,40;50,40)");
    EXPECT_EQ ((r1 ^ r1).to_string (100), "");
    EXPECT_EQ ((r1 - r2).to_string (100), "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(50,70;80,70);(80,40;50,40)");
    EXPECT_EQ ((r1 - r1).to_string (100), "");
    EXPECT_EQ (r2.merged ().to_string (100), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,70;140,40);(140,40;80,40)");
    EXPECT_EQ (rr1.to_string (100), "(0,0;0,30;30,30;30,0);(50,0;50,30;80,30;80,0);(50,40;50,70;80,70;80,40)");
    EXPECT_EQ (r2.selected_interacting (rr1).to_string (100), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,40;80,40)");
    EXPECT_EQ (r2.selected_not_interacting (rr1).to_string (100), "(10,40;40,40);(40,40;40,10);(140,70;140,40)");

    db::Edges r2dup = r2;
    r2.select_interacting (rr1);
    EXPECT_EQ (r2.to_string (), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,40;80,40)");
    r2 = r2dup;
    r2.select_not_interacting (rr1);
    EXPECT_EQ (r2.to_string (100), "(10,40;40,40);(40,40;40,10);(140,70;140,40)");

    r2 = db::Edges (db::RecursiveShapeIterator (ly, ly.cell (top), l2), false);
    EXPECT_EQ (r2.has_valid_edges (), false);
    r2.select_interacting (r1);
    EXPECT_EQ (r2.to_string (), "(10,10;10,40);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,40;80,40)");
  }
}

TEST(21)
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));

  db::Edges e, ee;
  e.insert (db::Edge (-100, 100, 200, 100));
  EXPECT_EQ ((e & r).to_string (), "(0,100;100,100)");
  EXPECT_EQ (e.inside_part (r).to_string (), "(0,100;100,100)");

  ee = e;
  ee &= r;
  EXPECT_EQ (ee.to_string (), "(0,100;100,100)");

  ee = e;
  ee.select_inside_part (r);
  EXPECT_EQ (ee.to_string (), "(0,100;100,100)");

  EXPECT_EQ ((e - r).to_string (), "(-100,100;0,100);(100,100;200,100)");
  EXPECT_EQ (e.outside_part (r).to_string (), "(-100,100;0,100);(100,100;200,100)");

  ee = e;
  ee -= r;
  EXPECT_EQ (ee.to_string (), "(-100,100;0,100);(100,100;200,100)");

  ee = e;
  ee.select_outside_part (r);
  EXPECT_EQ (ee.to_string (), "(-100,100;0,100);(100,100;200,100)");

  e.clear ();
  e.insert (db::Edge (-100, 0, 200, 0));
  EXPECT_EQ ((e & r).to_string (), "(0,0;100,0)");
  EXPECT_EQ (e.inside_part (r).to_string (), "");

  ee = e;
  ee &= r;
  EXPECT_EQ (ee.to_string (), "(0,0;100,0)");

  ee = e;
  ee.select_inside_part (r);
  EXPECT_EQ (ee.to_string (), "");

  EXPECT_EQ ((e - r).to_string (), "(-100,0;0,0);(100,0;200,0)");
  EXPECT_EQ (e.outside_part (r).to_string (), "(-100,0;0,0);(0,0;100,0);(100,0;200,0)");

  ee = e;
  ee -= r;
  EXPECT_EQ (ee.to_string (), "(-100,0;0,0);(100,0;200,0)");

  ee = e;
  ee.select_outside_part (r);
  EXPECT_EQ (ee.to_string (), "(-100,0;0,0);(0,0;100,0);(100,0;200,0)");
}

TEST(22)
{
  db::Edges e;
  e.insert (db::Edge (500,-173,400,0));
  e.insert (db::Edge (400,0,-2000,0));
  e.insert (db::Edge (4000,0,1000,0));
  e.insert (db::Edge (1000,0,900,-173));

  db::Edges ee;
  ee.insert (db::Edge (-2000,-2000,-2000,0));
  ee.insert (db::Edge (-2000,0,400,0));
  ee.insert (db::Edge (400,0,573,-300));
  ee.insert (db::Edge (573,-300,827,-300));
  ee.insert (db::Edge (827,-300,1000,0));
  ee.insert (db::Edge (1000,0,4000,0));
  ee.insert (db::Edge (4000,0,4000,-2000));
  ee.insert (db::Edge (4000,-2000,-2000,-2000));

  EXPECT_EQ ((e & ee).to_string (), "(400,0;-2000,0);(500,-174;400,0);(1000,0;900,-173);(4000,0;1000,0)");
}

//  GitHub issue #72 (Edges/Region NOT issue)
TEST(23)
{
  db::Edges e;
  e.insert (db::Edge (0, 0, 0, 1000));
  e.insert (db::Edge (0, 1000, 3000, 1000));
  e.insert (db::Edge (3000, 1000, 3000, 0));
  e.insert (db::Edge (3000, 0, 0, 0));

  db::Region r;
  r.insert (db::Box (1000, -1000, 2000, 0));
  r.insert (db::Box (1000, 1000, 2000, 2000));

  EXPECT_EQ ((e - r).to_string (), "(0,0;0,1000);(1000,0;0,0);(3000,0;2000,0);(3000,1000;3000,0);(0,1000;1000,1000);(2000,1000;3000,1000)");

  r.clear ();
  r.insert (db::Box (1000, -1000, 2000, 2000));

  EXPECT_EQ ((e - r).to_string (), "(0,0;0,1000);(1000,0;0,0);(3000,0;2000,0);(3000,1000;3000,0);(0,1000;1000,1000);(2000,1000;3000,1000)");

  e.clear ();
  e.insert (db::Edge (0, 0, 100, 1000));
  e.insert (db::Edge (100, 1000, 3100, 1000));
  e.insert (db::Edge (3100, 1000, 3000, 0));
  e.insert (db::Edge (3000, 0, 0, 0));

  r.clear ();
  r.insert (db::Box (1000, -1000, 2000, 0));
  r.insert (db::Box (1000, 1000, 2000, 2000));

  EXPECT_EQ ((e - r).to_string (), "(0,0;100,1000);(1000,0;0,0);(3000,0;2000,0);(3100,1000;3000,0);(100,1000;1000,1000);(2000,1000;3100,1000)");

  r.clear ();
  r.insert (db::Box (1000, -1000, 2000, 2000));

  EXPECT_EQ ((e - r).to_string (), "(0,0;100,1000);(1000,0;0,0);(3000,0;2000,0);(3100,1000;3000,0);(100,1000;1000,1000);(2000,1000;3100,1000)");

  e.clear ();
  e.insert (db::Edge (0, 0, 1000, 0));
  e.insert (db::Edge (1000, 0, 1000, 3000));
  e.insert (db::Edge (1000, 3000, 0, 3000));
  e.insert (db::Edge (0, 3000, 0, 0));

  r.clear ();
  r.insert (db::Box (-1000, 1000, 0, 2000));
  r.insert (db::Box (1000, 1000, 2000, 2000));

  EXPECT_EQ ((e - r).to_string (), "(0,1000;0,0);(0,0;1000,0);(1000,0;1000,1000);(0,3000;0,2000);(1000,2000;1000,3000);(1000,3000;0,3000)");

  r.clear ();
  r.insert (db::Box (-1000, 1000, 2000, 2000));

  EXPECT_EQ ((e - r).to_string (), "(0,1000;0,0);(0,0;1000,0);(1000,0;1000,1000);(0,3000;0,2000);(1000,2000;1000,3000);(1000,3000;0,3000)");
}


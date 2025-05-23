
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


#include "tlUnitTest.h"
#include "tlStream.h"

#include "dbEdges.h"
#include "dbEdgesUtils.h"
#include "dbPolygonTools.h"
#include "dbRegion.h"
#include "dbTestSupport.h"
#include "dbReader.h"

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
  EXPECT_EQ (db::compare (r, "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)"), true);
  EXPECT_EQ (db::compare (r.transformed (db::Trans (db::Vector (1, 2))), "(1,2;1,202);(1,202;101,202);(101,202;101,2);(101,2;1,2)"), true);
  EXPECT_EQ (r.bbox ().to_string (), "(0,0;100,200)");
  EXPECT_EQ (r.transformed (db::Trans (db::Vector (1, 2))).bbox ().to_string (), "(1,2;101,202)");
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.is_merged (), false);
  EXPECT_EQ (r.begin ().at_end (), false);

  db::Edges r1 = r;
  db::Edges r2;
  EXPECT_EQ (db::compare (r1, "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)"), true);
  EXPECT_EQ (db::compare (r1.merged (), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)"), true);
  EXPECT_EQ (r2.to_string (), "");
  EXPECT_EQ (r1.bbox ().to_string (), "(0,0;100,200)");
  EXPECT_EQ (r2.bbox ().to_string (), "()");
  r1.swap (r2);
  EXPECT_EQ (r1.to_string (), "");
  EXPECT_EQ (db::compare (r2, "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)"), true);
  EXPECT_EQ (r1.bbox ().to_string (), "()");
  EXPECT_EQ (r2.bbox ().to_string (), "(0,0;100,200)");

  EXPECT_EQ (db::compare ((r | db::Edges (db::Box (db::Point (10, 0), db::Point (110, 200)))), "(0,0;0,200);(100,200;100,0);(10,0;10,200);(0,200;110,200);(110,200;110,0);(110,0;0,0)"), true);
  EXPECT_EQ (db::compare ((r + db::Edges (db::Box (db::Point (10, 0), db::Point (110, 200)))), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0);(10,0;10,200);(10,200;110,200);(110,200;110,0);(110,0;10,0)"), true);

  db::Edges rr = r;
  rr |= db::Edges (db::Box (db::Point (10, 0), db::Point (110, 200)));
  EXPECT_EQ (rr.is_merged (), true);
  EXPECT_EQ (db::compare (rr, "(0,0;0,200);(100,200;100,0);(10,0;10,200);(0,200;110,200);(110,200;110,0);(110,0;0,0)"), true);

  r += db::Edges (db::Box (db::Point (10, 0), db::Point (110, 200)));
  EXPECT_EQ (db::compare (r, "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0);(10,0;10,200);(10,200;110,200);(110,200;110,0);(110,0;10,0)"), true);
  EXPECT_EQ (r.is_merged (), false);
  EXPECT_EQ (r.count (), size_t (8));
  EXPECT_EQ (r.hier_count (), size_t (8));
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
  EXPECT_EQ (db::compare (r, "(0,0;0,200);(100,200;100,0);(10,0;10,200);(0,200;110,200);(110,200;110,0);(110,0;0,0)"), true);
  EXPECT_EQ (r.bbox ().to_string (), "(0,0;110,200)");
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.count (), size_t (6));
  EXPECT_EQ (r.hier_count (), size_t (6));
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

  EXPECT_EQ (db::compare ((r & r1), "(10,200;100,200);(100,0;10,0)"), true);
  EXPECT_EQ (db::compare (r.andnot(r1).first, "(10,200;100,200);(100,0;10,0)"), true);
  EXPECT_EQ (db::compare ((r & r2), "(0,10;0,200);(100,200;100,10)"), true);
  EXPECT_EQ (db::compare (r.andnot(r2).first, "(0,10;0,200);(100,200;100,10)"), true);
  db::Edges o1 = r;
  o1 &= r1;
  EXPECT_EQ (o1.is_merged (), true);
  EXPECT_EQ (db::compare (o1, "(10,200;100,200);(100,0;10,0)"), true);

  EXPECT_EQ (db::compare ((r - r1), "(0,0;0,200);(100,200;100,0);(0,200;10,200);(10,0;0,0)"), true);
  EXPECT_EQ (db::compare (r.andnot(r1).second, "(0,0;0,200);(100,200;100,0);(0,200;10,200);(10,0;0,0)"), true);
  db::Edges o2 = r;
  o2 -= r1;
  EXPECT_EQ (o2.is_merged (), true);
  EXPECT_EQ (db::compare (o2, "(0,0;0,200);(100,200;100,0);(0,200;10,200);(10,0;0,0)"), true);

  EXPECT_EQ (db::compare ((r ^ r1), "(0,0;0,200);(100,200;100,0);(10,0;10,200);(0,200;10,200);(100,200;110,200);(110,200;110,0);(110,0;100,0);(10,0;0,0)"), true);
  db::Edges o3 = r;
  o3 ^= r1;
  EXPECT_EQ (o3.is_merged (), true);
  EXPECT_EQ (db::compare (o3, "(0,0;0,200);(100,200;100,0);(10,0;10,200);(0,200;10,200);(100,200;110,200);(110,200;110,0);(110,0;100,0);(10,0;0,0)"), true);

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

  EXPECT_EQ (db::compare (r.merged (), "(0,0;150,15);(200,20;230,23)"), true);
  EXPECT_EQ (rr.merged ().to_string (), "(10,1;70,7)");
  EXPECT_EQ (db::compare ((r ^ rr), "(200,20;230,23);(0,0;10,1);(70,7;150,15)"), true);
  EXPECT_EQ (db::compare ((rr ^ r), "(0,0;10,1);(70,7;150,15);(200,20;230,23)"), true);
  EXPECT_EQ (db::compare ((r - rr), "(200,20;230,23);(0,0;10,1);(70,7;150,15)"), true);
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
    EXPECT_EQ (db::compare (rr, "(0,200;100,200);(100,0;0,0);(300,0;200,0)"), true);
  }
  {
    db::EdgeLengthFilter f1 (201, 1000, false);
    db::Edges rr = r;
    rr.filter (f1);
    EXPECT_EQ (db::compare (rr, "(200,0;250,200);(250,200;300,0);(200,0;250,-200);(250,-200;300,0)"), true);
  }
  {
    db::EdgeLengthFilter f1 (201, 1000, true);
    db::Edges rr = r;
    rr.filter (f1);
    EXPECT_EQ (db::compare (rr, "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0);(300,0;200,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (0.0, false, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(0,200;100,200);(100,0;0,0);(300,0;200,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (50.0, true, 80.0, false, false, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(200,0;250,200);(250,-200;300,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (-80.0, true, -50.0, false, false, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(250,200;300,0);(200,0;250,-200)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (50.0, true, 80.0, false, false, true);
    EXPECT_EQ (db::compare (r.filtered (f1), "(200,0;250,200);(250,200;300,0);(200,0;250,-200);(250,-200;300,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (50.0, true, 80.0, false, true, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0);(250,200;300,0);(300,0;200,0);(200,0;250,-200)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (0.0, true, 1.0, false, false, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(0,200;100,200);(100,0;0,0);(300,0;200,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (-1.0, true, 1.0, false, false, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(0,200;100,200);(100,0;0,0);(300,0;200,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (-1.0, true, 0.0, false, false, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "");
  }
  {
    db::EdgeOrientationFilter f1 (-1.0, true, 0.0, true, false, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(0,200;100,200);(100,0;0,0);(300,0;200,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (0.0, true, 1.0, true, false, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(0,200;100,200);(100,0;0,0);(300,0;200,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (0.0, false, 1.0, true, false, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "");
  }
  {
    db::EdgeOrientationFilter f1 (90.0, false, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(0,0;0,200);(100,200;100,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (90.0, true, 91.0, false, false, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(0,0;0,200);(100,200;100,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (89.0, true, 91.0, false, false, false);
    EXPECT_EQ (db::compare (r.filtered (f1), "(0,0;0,200);(100,200;100,0)"), true);
  }
  {
    db::EdgeOrientationFilter f1 (89.0, true, 90.0, false, false, false);
    EXPECT_EQ (r.filtered (f1).to_string (), "");
  }

  //  issue-2060
  {
    db::EdgeOrientationFilter f1 (90.0, true, false);
    db::EdgeOrientationFilter f2 (90.0, false, false);
    db::EdgeOrientationFilter f45 (45.0, false, false);
    db::SpecialEdgeOrientationFilter fs (db::SpecialEdgeOrientationFilter::Diagonal, false);

    db::Edges rr;
    rr.insert (db::Box (db::Point (0, 0), db::Point (1000, 4000000)));
    EXPECT_EQ (db::compare (rr.filtered (f1), "(1000,0;0,0);(0,4000000;1000,4000000)"), true);

    rr.clear ();
    rr.insert (db::Box (db::Point (0, 0), db::Point (1000, 400000)));
    EXPECT_EQ (db::compare (rr.filtered (f1), "(1000,0;0,0);(0,400000;1000,400000)"), true);

    rr.clear ();
    rr.insert (db::Box (db::Point (0, -1000000000), db::Point (1000, 1000000000)));
    EXPECT_EQ (db::compare (rr.filtered (f1), "(1000,-1000000000;0,-1000000000);(0,1000000000;1000,1000000000)"), true);

    rr.clear ();
    rr.insert (db::Box (db::Point (0, -1000000000), db::Point (1000, 1000000000)));
    EXPECT_EQ (db::compare (rr.filtered (f2), "(0,-1000000000;0,1000000000);(1000,1000000000;1000,-1000000000)"), true);

    EXPECT_EQ (f2.selected (db::Edge (db::Point (0, -1000000000), db::Point (0, 1000000000)), size_t (0)), true);
    EXPECT_EQ (f2.selected (db::Edge (db::Point (0, -1000000000), db::Point (1, 1000000000)), size_t (0)), false);
    EXPECT_EQ (f45.selected (db::Edge (db::Point (-1000000000, -1000000000), db::Point (1000000000, 1000000000)), size_t (0)), true);
    EXPECT_EQ (f45.selected (db::Edge (db::Point (-1000000000, -1000000000), db::Point (1000000000, 1000000001)), size_t (0)), false);
    EXPECT_EQ (fs.selected (db::Edge (db::Point (-1000000000, -1000000000), db::Point (1000000000, 1000000000)), size_t (0)), true);
    EXPECT_EQ (fs.selected (db::Edge (db::Point (-1000000000, -1000000000), db::Point (1000000000, 1000000001)), size_t (0)), false);
  }
}

TEST(5) 
{
  db::Edges r;
  r.insert (db::Polygon (db::Box (db::Point (0, 0), db::Point (100, 200))));
  EXPECT_EQ (db::compare (r, "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)"), true);
  r.clear ();
  r.insert (db::SimplePolygon (db::Box (db::Point (0, 0), db::Point (100, 200))));
  EXPECT_EQ (db::compare (r, "(0,0;0,200);(0,200;100,200);(100,200;100,0);(100,0;0,0)"), true);
  r.transform (db::ICplxTrans (2.5));
  EXPECT_EQ (db::compare (r, "(0,0;0,500);(0,500;250,500);(250,500;250,0);(250,0;0,0)"), true);

  db::Edges rr;
  std::string s = r.to_string ();
  tl::Extractor ex (s.c_str ());
  EXPECT_EQ (ex.try_read (rr), true);
  EXPECT_EQ (db::compare (rr, "(0,0;0,500);(0,500;250,500);(250,500;250,0);(250,0;0,0)"), true);
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
  EXPECT_EQ (db::compare (r, "(-20,0;-20,200;0,200;0,0);(0,200;0,220;100,220;100,200);(200,0;181,5;231,205;250,200);(300,0;250,200;269,205;319,5)"), true);

  r.clear ();
  e.extended (r, 0, 0, 20, 0, true);
  EXPECT_EQ (db::compare (r, "(-20,0;-20,220;100,220;100,200;0,200;0,0);(200,0;181,5;235,224;265,224;319,5;300,0;250,200)"), true);

  r.clear ();
  e.extended (r, 0, 0, 0, 10, false);
  EXPECT_EQ (db::compare (r, "(0,0;0,200;10,200;10,0);(0,190;0,200;100,200;100,190);(210,-2;200,0;250,200;260,198);(290,-2;240,198;250,200;300,0)"), true);

  r.clear ();
  e.extended (r, 0, 0, 0, 10, true);
  EXPECT_EQ (db::compare (r, "(0,0;0,200;100,200;100,190;10,190;10,0);(210,-2;200,0;250,200;300,0;290,-2;250,159)"), true);

  r.clear ();
  e.extended (r, 10, 20, 0, 10, true);
  EXPECT_EQ (db::compare (r, "(0,-10;0,200;120,200;120,190;10,190;10,-10);(295,-22;250,159;207,-12;198,-10;250,200;305,-19)"), true);

  r.clear ();
  e.extended (r, 10, 20, 0, 10, false);
  EXPECT_EQ (db::compare (r, "(0,-10;0,220;10,220;10,-10);(-10,190;-10,200;120,200;120,190);(207,-12;198,-10;255,219;265,217);(295,-22;238,207;248,210;305,-19)"), true);

  r.clear ();
  e.extended (r, 10, 20, 20, -10, false);
  EXPECT_EQ (db::compare (r, "(-20,-10;-20,220;-10,220;-10,-10);(-10,210;-10,220;120,220;120,210);(188,-7;178,-5;235,224;245,222);(315,-17;257,212;267,215;324,-15)"), true);

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
  EXPECT_EQ (db::compare (r, "(0,-10;0,220;10,220;10,-10);(-10,190;-10,200;120,200;120,190);(90,-20;90,210;100,210;100,-20);(-20,0;-20,10;110,10;110,0)"), true);

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
  EXPECT_EQ (db::compare (r, "(0,-200;0,0;20,0;20,-180;100,-180;100,-200);(250,-200;200,0;219,5;250,-118;281,5;300,0)"), true);
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
  EXPECT_EQ (db::compare (r, "(0,-200;0,0;20,0;20,-180;100,-180;100,-200);(0,-200;0,-100;20,-100;20,-180;200,-180;200,-200);(250,-200;200,0;219,5;250,-118;281,5;300,0);(250,-200;232,-191;332,9;350,0)"), true);
}

TEST(7)
{
  db::Edges e;
  e.insert (db::Edge (db::Point (0, 0), db::Point (0, 200)));
  e.insert (db::Edge (db::Point (250, 200), db::Point (300, 0)));

  EXPECT_EQ (db::compare (e.start_segments (10, 0), "(0,0;0,10);(250,200;252,190)"), true);
  EXPECT_EQ (db::compare (e.start_segments (10, 0.25), "(0,0;0,50);(250,200;263,150)"), true);
  EXPECT_EQ (db::compare (e.start_segments (0, 1.0), "(0,0;0,200);(250,200;300,0)"), true);
  EXPECT_EQ (db::compare (e.start_segments (0, 0), "(0,0;0,0);(250,200;250,200)"), true);

  EXPECT_EQ (db::compare (e.end_segments (10, 0), "(0,190;0,200);(298,10;300,0)"), true);
  EXPECT_EQ (db::compare (e.end_segments (10, 0.25), "(0,150;0,200);(288,50;300,0)"), true);
  EXPECT_EQ (db::compare (e.end_segments (0, 1.0), "(0,0;0,200);(250,200;300,0)"), true);
  EXPECT_EQ (db::compare (e.end_segments (0, 0), "(0,200;0,200);(300,0;300,0)"), true);

  EXPECT_EQ (db::compare (e.centers (10, 0), "(0,95;0,105);(274,105;276,95)"), true);
  EXPECT_EQ (db::compare (e.centers (10, 0.25), "(0,75;0,125);(269,125;281,75)"), true);
  EXPECT_EQ (db::compare (e.centers (0, 1.0), "(0,0;0,200);(250,200;300,0)"), true);
  EXPECT_EQ (db::compare (e.centers (0, 0), "(0,100;0,100);(275,100;275,100)"), true);
}

TEST(8) 
{
  db::Edges e;
  e.insert (db::Edge (db::Point (0, 0), db::Point (0, 200)));
  e.insert (db::Edge (db::Point (250, 200), db::Point (300, 0)));

  db::Edges e2;
  e2.insert (db::Edge (db::Point (0, 100), db::Point (100, 100)));

  EXPECT_EQ (e.selected_interacting (e2).to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_interacting_differential (e2).first.to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_not_interacting (e2).to_string (), "(250,200;300,0)");
  EXPECT_EQ (e.selected_interacting_differential (e2).second.to_string (), "(250,200;300,0)");

  e2.clear ();
  e2.insert (db::Edge (db::Point (0, 100), db::Point (0, 100)));

  EXPECT_EQ (e.selected_interacting (e2).to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_interacting_differential (e2).first.to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_not_interacting (e2).to_string (), "(250,200;300,0)");
  EXPECT_EQ (e.selected_interacting_differential (e2).second.to_string (), "(250,200;300,0)");

  e2.clear ();
  e2.insert (db::Edge (db::Point (100, 0), db::Point (0, 0)));

  EXPECT_EQ (e.selected_interacting (e2).to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_interacting_differential (e2).first.to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_not_interacting (e2).to_string (), "(250,200;300,0)");
  EXPECT_EQ (e.selected_interacting_differential (e2).second.to_string (), "(250,200;300,0)");

  e2.clear ();
  e2.insert (db::Edge (db::Point (-100, -1), db::Point (100, -1)));

  EXPECT_EQ (e.selected_interacting (e2).to_string (), "");
  EXPECT_EQ (e.selected_interacting_differential (e2).first.to_string (), "");
  EXPECT_EQ (db::compare (e.selected_not_interacting (e2), "(0,0;0,200);(250,200;300,0)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting_differential (e2).second, "(0,0;0,200);(250,200;300,0)"), true);

  e2.clear ();
  e2.insert (db::Edge (db::Point (-100, 0), db::Point (100, 0)));

  EXPECT_EQ (e.selected_interacting (e2).to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_interacting_differential (e2).first.to_string (), "(0,0;0,200)");
  EXPECT_EQ (e.selected_not_interacting (e2).to_string (), "(250,200;300,0)");
  EXPECT_EQ (e.selected_interacting_differential (e2).second.to_string (), "(250,200;300,0)");

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

  EXPECT_EQ (db::compare (r.edges ().width_check (15), "(0,0;0,10)/(10,10;10,0);(0,10;10,10)/(10,0;0,0)"), true);
  EXPECT_EQ (r.edges ().width_check (5).to_string (), "");
  EXPECT_EQ (db::compare (r.edges ().width_check (5, db::EdgesCheckOptions (false, db::Euclidian, 91)), "(0,5;0,10)/(0,10;5,10);(0,0;0,5)/(5,0;0,0);(5,10;10,10)/(10,10;10,5);(10,5;10,0)/(10,0;5,0);(20,45;20,50)/(20,50;25,50);(20,20;20,25)/(25,20;20,20);(35,50;40,50)/(40,50;40,45);(40,25;40,20)/(40,20;35,20)"), true);
  EXPECT_EQ (db::compare (r.edges ().space_check (15, db::EdgesCheckOptions (false, db::Euclidian, 91)), "(9,10;10,10)/(20,20;20,21);(9,10;10,10)/(21,20;20,20);(10,10;10,9)/(20,20;20,21);(10,10;10,9)/(21,20;20,20)"), true);
  EXPECT_EQ (db::compare (r.edges ().space_check (15, db::EdgesCheckOptions (false, db::Square, 91)), "(5,10;10,10)/(20,20;20,25);(5,10;10,10)/(25,20;20,20);(10,10;10,5)/(20,20;20,25);(10,10;10,5)/(25,20;20,20)"), true);
  EXPECT_EQ (db::compare (r.edges ().space_check (15), "(9,10;10,10)/(21,20;20,20);(10,10;10,9)/(20,20;20,21)"), true);
  EXPECT_EQ (db::compare (r.edges ().space_check (15, db::EdgesCheckOptions (true)), "(0,10;10,10)/(40,20;20,20);(10,10;10,0)/(20,20;20,50)"), true);
  EXPECT_EQ (db::compare (r.edges ().space_check (15, db::EdgesCheckOptions (false, db::Square)), "(5,10;10,10)/(25,20;20,20);(10,10;10,5)/(20,20;20,25)"), true);
}

TEST(12) 
{
  db::Region a;
  a.insert (db::Box (db::Point (10, 20), db::Point (20, 30)));

  db::Region b;
  b.insert (db::Box (db::Point (0, 0), db::Point (100, 100)));

  EXPECT_EQ (a.edges ().inside_check (b.edges (), 15).to_string (), "(10,20;10,30)/(0,9;0,41)");
  EXPECT_EQ (a.edges ().inside_check (b.edges (), 15, db::EdgesCheckOptions (true)).to_string (), "(10,20;10,30)/(0,0;0,100)");
  EXPECT_EQ (db::compare (a.edges ().inside_check (b.edges (), 15, db::EdgesCheckOptions (false, db::Euclidian, 91)), "(10,20;10,30)/(0,9;0,41);(10,30;15,30)/(0,30;0,41);(15,20;10,20)/(0,9;0,20)"), true);
  EXPECT_EQ (b.edges ().enclosing_check (a.edges (), 15).to_string (), "(0,9;0,41)/(10,20;10,30)");
  EXPECT_EQ (b.edges ().enclosing_check (a.edges (), 15, db::EdgesCheckOptions (true)).to_string (), "(0,0;0,100)/(10,20;10,30)");
  EXPECT_EQ (db::compare (b.edges ().enclosing_check (a.edges (), 15, db::EdgesCheckOptions (false, db::Euclidian, 91)), "(0,9;0,41)/(10,20;10,30);(0,30;0,41)/(10,30;15,30);(0,9;0,20)/(15,20;10,20)"), true);

  b.clear ();
  b.insert (db::Box (db::Point (30, 0), db::Point (100, 100)));
  EXPECT_EQ (b.separation_check (a, 15).to_string (), "(30,9;30,41)/(20,30;20,20)");
  EXPECT_EQ (b.separation_check (a, 15, true).to_string (), "(30,0;30,100)/(20,30;20,20)");
  EXPECT_EQ (db::compare (b.separation_check (a, 15, db::RegionCheckOptions (false, db::Euclidian, 91)), "(30,30;30,41)/(15,30;20,30);(30,9;30,41)/(20,30;20,20);(30,9;30,20)/(20,20;15,20)"), true);

  b.clear ();
  b.insert (db::Box (db::Point (15, 0), db::Point (100, 100)));
  EXPECT_EQ (b.overlap_check (a, 15).to_string (), "(15,6;15,44)/(20,30;20,20)");
  EXPECT_EQ (b.overlap_check (a, 15, true).to_string (), "(15,0;15,100)/(20,30;20,20)");
  EXPECT_EQ (db::compare (b.overlap_check (a, 15, db::RegionCheckOptions (false, db::Euclidian, 91)), "(15,15;15,30)/(15,30;20,30);(15,6;15,44)/(20,30;20,20);(15,20;15,35)/(20,20;15,20)"), true);
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
    EXPECT_EQ (db::compare (r1, "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(80,40;80,70);(110,40;80,40)"), true);
    EXPECT_EQ (r1.has_valid_edges (), false);
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2, db::Box (60, 10, 90, 50)), db::ICplxTrans (2.0), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (db::compare (r1, "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20);(160,80;160,140);(220,80;160,80)"), true);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.length (), db::Edges::length_type (200));
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.bbox ().to_string (), "(120,20;220,140)");
    EXPECT_EQ (r1.count (), size_t (6));
    EXPECT_EQ (r1.hier_count (), size_t (6));
    EXPECT_EQ (r1.empty (), false);

    db::EdgeLengthFilter f0 (0, 50, false);
    db::Edges rr = r1.filtered (f0);
    EXPECT_EQ (rr.has_valid_edges (), true);
    EXPECT_EQ (db::compare (rr, "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20)"), true);

    db::Edges r2 = r1;
    EXPECT_EQ (r2.has_valid_edges (), false);
    EXPECT_EQ (r2.length (), db::Edges::length_type (200));
    EXPECT_EQ (r2.bbox ().to_string (), "(120,20;220,140)");
    EXPECT_EQ (r2.count (), size_t (6));
    EXPECT_EQ (r2.hier_count (), size_t (6));
    EXPECT_EQ (r2.empty (), false);
    r2.filter (f0);
    EXPECT_EQ (r2.has_valid_edges (), true);
    EXPECT_EQ (db::compare (r2, "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20)"), true);
    EXPECT_EQ (r2.count (), size_t (4));
    EXPECT_EQ (r2.hier_count (), size_t (4));
    EXPECT_EQ (r2.empty (), false);
    EXPECT_EQ (r2.length (), db::Edges::length_type (80));

    r1.insert (db::Box (0, 0, 10, 20));
    EXPECT_EQ (r1.has_valid_edges (), true);
    EXPECT_EQ (db::compare (r1, "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20);(160,80;160,140);(220,80;160,80);(0,0;0,20);(0,20;10,20);(10,20;10,0);(10,0;0,0)"), true);
    EXPECT_EQ (r1.to_string (2), "(120,20;120,40);(120,40;140,40)...");
    EXPECT_EQ (r1.count (), size_t (10));
    EXPECT_EQ (r1.hier_count (), size_t (10));
    EXPECT_EQ (r1.length (), db::Edges::length_type (260));

    rr = r1.filtered (f0);
    EXPECT_EQ (db::compare (rr, "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20);(0,0;0,20);(0,20;10,20);(10,20;10,0);(10,0;0,0)"), true);
    EXPECT_EQ (db::compare (r1, "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20);(160,80;160,140);(220,80;160,80);(0,0;0,20);(0,20;10,20);(10,20;10,0);(10,0;0,0)"), true);

    r1.filter (f0);
    EXPECT_EQ (db::compare (r1, "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20);(0,0;0,20);(0,20;10,20);(10,20;10,0);(10,0;0,0)"), true);
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2, db::Box (60, 10, 70, 50)), db::ICplxTrans (2.0), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (db::compare (r1, "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20)"), true);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r1.count (), size_t (4));
    EXPECT_EQ (r1.hier_count (), size_t (4));
    EXPECT_EQ (r1.empty (), false);

    db::Edges r2 = r1;

    EXPECT_EQ (db::compare (r1.transformed (db::ICplxTrans (0.5)), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10)"), true);
    r1.transform (db::ICplxTrans (0.5));
    EXPECT_EQ (r1.has_valid_edges (), true);
    EXPECT_EQ (db::compare (r1, "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10)"), true);

    r1.clear ();
    EXPECT_EQ (r1.has_valid_edges (), true);
    EXPECT_EQ (r1.count (), size_t (0));
    EXPECT_EQ (r1.hier_count (), size_t (0));
    EXPECT_EQ (r1.empty (), true);
    EXPECT_EQ (r1.length (), db::Edges::length_type (0));

    EXPECT_EQ (db::compare (r2, "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20)"), true);
    r1.swap (r2);

    EXPECT_EQ (db::compare (r1, "(120,20;120,40);(120,40;140,40);(140,40;140,20);(140,20;120,20)"), true);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (r2.has_valid_edges (), true);
    EXPECT_EQ (r2.count (), size_t (0));
    EXPECT_EQ (r2.hier_count (), size_t (0));
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
    EXPECT_EQ (db::compare (r1.width_check (20), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10)"), true);
    EXPECT_EQ (db::compare (r1.width_check (50), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10);(60,20;70,20)/(40,10;11,10);(70,10;60,10)/(20,40;40,40);(10,10;10,40)/(40,40;40,10);(10,40;40,40)/(40,10;10,10);(80,70;140,70)/(140,40;80,40)"), true);
    EXPECT_EQ (db::compare (r1.width_check (50, db::EdgesCheckOptions (true)), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10);(60,20;70,20)/(40,10;10,10);(70,10;60,10)/(10,40;40,40);(10,10;10,40)/(40,40;40,10);(10,40;40,40)/(40,10;10,10);(80,70;140,70)/(140,40;80,40)"), true);
    EXPECT_EQ (db::compare (r1.width_check (50, db::EdgesCheckOptions (false, db::Projection)), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10);(10,10;10,40)/(40,40;40,10);(10,40;40,40)/(40,10;10,10);(80,70;140,70)/(140,40;80,40)"), true);
    EXPECT_EQ (db::compare (r1.width_check (50, db::EdgesCheckOptions (false, db::Euclidian, 90, 1)), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10);(10,10;10,40)/(40,40;40,10);(10,40;40,40)/(40,10;10,10);(80,70;140,70)/(140,40;80,40)"), true);
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    EXPECT_EQ (db::compare (r1.space_check (30), "(60,10;60,20)/(40,40;40,10);(60,20;70,20)/(92,40;80,40);(70,20;70,12)/(80,40;80,48)"), true);
    EXPECT_EQ (r1.space_check (2).to_string (), "");
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l1), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    db::Edges r2 (db::RecursiveShapeIterator (ly, ly.cell (top), l2), false);
    EXPECT_EQ (r2.has_valid_edges (), false);
    EXPECT_EQ (db::compare (r1.separation_check (r2, 20), "(50,0;50,30)/(40,40;40,10);(63,30;80,30)/(97,40;80,40);(50,40;50,57)/(40,40;40,23);(80,70;80,40)/(80,40;80,70)"), true);
    EXPECT_EQ (db::compare (r1.separation_check (r2, 20, db::EdgesCheckOptions (false, db::Projection)), "(50,10;50,30)/(40,30;40,10);(80,70;80,40)/(80,40;80,70)"), true);
    EXPECT_EQ (db::compare (r1.separation_check (r2, 20, db::EdgesCheckOptions (false, db::Euclidian, 90, 1)), "(50,0;50,30)/(40,40;40,10);(80,70;80,40)/(80,40;80,70)"), true);
  }

  {
    db::Edges r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l1), false);
    EXPECT_EQ (r1.has_valid_edges (), false);
    db::Edges r2 (db::RecursiveShapeIterator (ly, ly.cell (top), l2), false);
    EXPECT_EQ (r2.has_valid_edges (), false);
    db::Region rr1 (db::RecursiveShapeIterator (ly, ly.cell (top), lp1), db::ICplxTrans (), false);
    EXPECT_EQ (rr1.has_valid_polygons (), false);
    EXPECT_EQ (db::compare (r1 & r2, "(80,70;80,40)"), true);
    EXPECT_EQ (db::compare (r1 + r2, "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(50,70;80,70);(80,70;80,40);(80,40;50,40);(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(80,40;80,70);(80,70;110,70);(110,70;110,40);(110,40;80,40);(110,40;110,70);(110,70;140,70);(140,70;140,40);(140,40;110,40)"), true);
    EXPECT_EQ (db::compare ((r1 + r2).merged (), "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(50,70;140,70);(140,70;140,40);(140,40;50,40)"), true);
    EXPECT_EQ (db::compare (r1 | r2, "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(50,70;140,70);(140,70;140,40);(140,40;50,40)"), true);
    EXPECT_EQ (db::compare (r1 ^ r2, "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(50,70;140,70);(140,70;140,40);(140,40;50,40)"), true);
    EXPECT_EQ (db::compare (r1 ^ r1, ""), true);
    EXPECT_EQ (db::compare (r1 - r2, "(0,0;0,30);(0,30;30,30);(30,30;30,0);(30,0;0,0);(50,0;50,30);(50,30;80,30);(80,30;80,0);(80,0;50,0);(50,40;50,70);(50,70;80,70);(80,40;50,40)"), true);
    EXPECT_EQ (db::compare (r1 - r1, ""), true);
    EXPECT_EQ (db::compare (r2.merged (), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,70;140,40);(140,40;80,40)"), true);
    EXPECT_EQ (db::compare (rr1, "(0,0;0,30;30,30;30,0);(50,0;50,30;80,30;80,0);(50,40;50,70;80,70;80,40)"), true);
    EXPECT_EQ (db::compare (r2.selected_interacting (rr1), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,40;80,40)"), true);
    EXPECT_EQ (db::compare (r2.selected_interacting_differential (rr1).first, "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,40;80,40)"), true);
    EXPECT_EQ (db::compare (r2.selected_not_interacting (rr1), "(10,40;40,40);(40,40;40,10);(140,70;140,40)"), true);
    EXPECT_EQ (db::compare (r2.selected_interacting_differential (rr1).second, "(10,40;40,40);(40,40;40,10);(140,70;140,40)"), true);

    db::Edges r2dup = r2;
    r2.select_interacting (rr1);
    EXPECT_EQ (db::compare (r2, "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,40;80,40)"), true);
    r2 = r2dup;
    r2.select_not_interacting (rr1);
    EXPECT_EQ (db::compare (r2, "(10,40;40,40);(40,40;40,10);(140,70;140,40)"), true);

    r2 = db::Edges (db::RecursiveShapeIterator (ly, ly.cell (top), l2), false);
    EXPECT_EQ (r2.has_valid_edges (), false);
    r2.select_interacting (r1);
    EXPECT_EQ (db::compare (r2, "(10,10;10,40);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,40;80,40)"), true);
  }
}

TEST(21)
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));

  db::Edges e, ee;
  e.insert (db::Edge (-100, 100, 200, 100));
  EXPECT_EQ ((e & r).to_string (), "(0,100;100,100)");
  EXPECT_EQ (e.andnot(r).first.to_string (), "(0,100;100,100)");
  EXPECT_EQ (e.inside_part (r).to_string (), "(0,100;100,100)");
  EXPECT_EQ (e.inside_outside_part (r).first.to_string (), "(0,100;100,100)");

  ee = e;
  ee &= r;
  EXPECT_EQ (ee.to_string (), "(0,100;100,100)");

  ee = e;
  ee.select_inside_part (r);
  EXPECT_EQ (ee.to_string (), "(0,100;100,100)");

  EXPECT_EQ (db::compare ((e - r), "(-100,100;0,100);(100,100;200,100)"), true);
  EXPECT_EQ (db::compare (e.andnot(r).second, "(-100,100;0,100);(100,100;200,100)"), true);
  EXPECT_EQ (db::compare (e.outside_part (r), "(-100,100;0,100);(100,100;200,100)"), true);
  EXPECT_EQ (db::compare (e.inside_outside_part (r).second, "(-100,100;0,100);(100,100;200,100)"), true);

  ee = e;
  ee -= r;
  EXPECT_EQ (db::compare (ee, "(-100,100;0,100);(100,100;200,100)"), true);

  ee = e;
  ee.select_outside_part (r);
  EXPECT_EQ (db::compare (ee, "(-100,100;0,100);(100,100;200,100)"), true);

  e.clear ();
  e.insert (db::Edge (-100, 0, 200, 0));
  EXPECT_EQ ((e & r).to_string (), "(0,0;100,0)");
  EXPECT_EQ (e.andnot(r).first.to_string (), "(0,0;100,0)");
  EXPECT_EQ (e.inside_part (r).to_string (), "");
  EXPECT_EQ (e.inside_outside_part (r).first.to_string (), "");

  ee = e;
  ee &= r;
  EXPECT_EQ (ee.to_string (), "(0,0;100,0)");

  ee = e;
  ee.select_inside_part (r);
  EXPECT_EQ (ee.to_string (), "");

  EXPECT_EQ (db::compare ((e - r), "(-100,0;0,0);(100,0;200,0)"), true);
  EXPECT_EQ (db::compare (e.andnot(r).second, "(-100,0;0,0);(100,0;200,0)"), true);
  EXPECT_EQ (db::compare (e.outside_part (r), "(-100,0;0,0);(0,0;100,0);(100,0;200,0)"), true);

  ee = e;
  ee -= r;
  EXPECT_EQ (db::compare (ee, "(-100,0;0,0);(100,0;200,0)"), true);

  ee = e;
  ee.select_outside_part (r);
  EXPECT_EQ (db::compare (ee, "(-100,0;0,0);(0,0;100,0);(100,0;200,0)"), true);
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

  EXPECT_EQ (db::compare ((e & ee), "(400,0;-2000,0);(500,-173;400,0);(1000,0;900,-174);(4000,0;1000,0)"), true);
  EXPECT_EQ (db::compare (e.andnot(ee).first, "(400,0;-2000,0);(500,-173;400,0);(1000,0;900,-174);(4000,0;1000,0)"), true);
  EXPECT_EQ (db::compare (e.intersections (ee), "(400,0;-2000,0);(500,-173;400,0);(1000,0;900,-174);(4000,0;1000,0)"), true);

  //  Edge/edge intersections
  ee.clear ();
  e.clear ();
  e.insert (db::Edge (0, -100, 0, 150));
  ee.insert (db::Edge (-50, 50, 50, 50));
  ee.insert (db::Edge (-50, 100, 50, 100));
  EXPECT_EQ ((e & ee).to_string (), "");  //  AND does not report intersection points
  EXPECT_EQ (e.andnot(ee).first.to_string (), "");  //  AND does not report intersection points
  EXPECT_EQ (db::compare (e.intersections (ee), "(0,50;0,50);(0,100;0,100)"), true);

  //  Edge is intersected by pair with connection point on this line
  ee.clear ();
  e.clear ();
  e.insert (db::Edge (0, -100, 0, 150));
  ee.insert (db::Edge (-50, 50, 0, 50));
  ee.insert (db::Edge (0, 60, 50, 60));
  ee.insert (db::Edge (-50, 100, 0, 100));
  ee.insert (db::Edge (0, 100, 50, 100));
  EXPECT_EQ ((e & ee).to_string (), "");  //  AND does not report intersection points
  EXPECT_EQ (e.andnot(ee).first.to_string (), "");  //  AND does not report intersection points
  EXPECT_EQ (db::compare (e.intersections (ee), "(0,50;0,50);(0,60;0,60);(0,100;0,100)"), true);

  //  Coincident edges are crossed by another one
  ee.clear ();
  e.clear ();
  e.insert (db::Edge (0, -100, 0, 250));
  ee.insert (db::Edge (0, 0, 0, 150));
  ee.insert (db::Edge (-50, 100, 50, 100));
  ee.insert (db::Edge (-50, 200, 50, 200));
  EXPECT_EQ ((e & ee).to_string (), "(0,0;0,150)");
  EXPECT_EQ (e.andnot(ee).first.to_string (), "(0,0;0,150)");
  EXPECT_EQ (db::compare (e.intersections (ee), "(0,0;0,150);(0,200;0,200)"), true);
}

TEST(23)
{
  db::Edges e;
  e.insert (db::Edge (db::Point (0, 0), db::Point (0, 200)));
  e.insert (db::Edge (db::Point (250, 200), db::Point (300, 0)));

  db::Edges e2;
  e2.insert (db::Edge (db::Point (0, 100), db::Point (100, 100)));

  EXPECT_EQ (e2.pull_interacting (e).to_string (), "(0,0;0,200)");

  e2.clear ();
  e2.insert (db::Edge (db::Point (0, 100), db::Point (0, 100)));

  EXPECT_EQ (e2.pull_interacting (e).to_string (), "(0,0;0,200)");

  e2.clear ();
  e2.insert (db::Edge (db::Point (100, 0), db::Point (0, 0)));

  EXPECT_EQ (e2.pull_interacting (e).to_string (), "(0,0;0,200)");

  e2.clear ();
  e2.insert (db::Edge (db::Point (-100, -1), db::Point (100, -1)));

  EXPECT_EQ (e2.pull_interacting (e).to_string (), "");

  e2.clear ();
  e2.insert (db::Edge (db::Point (-100, 0), db::Point (100, 0)));

  EXPECT_EQ (e2.pull_interacting (e).to_string (), "(0,0;0,200)");
}

//  Edges::selected_inside(region)
TEST(24)
{
  db::Edges e;
  e.insert (db::Edge (0, 0, 0, 1000));
  e.insert (db::Edge (100, 0, 100, 3000));
  e.insert (db::Edge (1100, -1000, 1100, 2000));
  e.insert (db::Edge (1200, -1000, 1200, 0));
  e.insert (db::Edge (1300, -800, 1300, -200));
  e.insert (db::Edge (1400, 1000, 1400, 1100));
  e.insert (db::Edge (1500, 1000, 1500, 2100));
  e.insert (db::Edge (1600, -800, 1600, -400));
  e.insert (db::Edge (1600, -400, 1600, -200));
  e.insert (db::Edge (1700, 1500, 1600, 2500));
  e.insert (db::Edge (1800, 2500, 1800, 3500));
  e.insert (db::Edge (1900, 1000, 1900, 2000));
  e.insert (db::Edge (-1500, 0, -1500, 1000));

  db::Region r;
  r.insert (db::Box (0, -1000, 2000, 0));
  r.insert (db::Box (1000, 1000, 2000, 1500));
  r.insert (db::Box (1000, 1500, 2000, 2000));

  EXPECT_EQ (db::compare (e.selected_inside (db::Region ()), ""), true);
  EXPECT_EQ (db::compare (e.selected_not_inside (db::Region ()), "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-400);(1600,-400;1600,-200);(1700,1500;1600,2500);(1800,2500;1800,3500);(1900,1000;1900,2000);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_inside_differential (db::Region ()).first, ""), true);
  EXPECT_EQ (db::compare (e.selected_inside_differential (db::Region ()).second, "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-400);(1600,-400;1600,-200);(1700,1500;1600,2500);(1800,2500;1800,3500);(1900,1000;1900,2000);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_inside (r), ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_not_inside (r), ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_inside_differential (r).first, ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_inside_differential (r).second, ""), true);
  EXPECT_EQ (db::compare (e.selected_inside (r), "(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1600,-800;1600,-200);(1900,1000;1900,2000)"), true);
  EXPECT_EQ (db::compare (e.selected_not_inside (r), "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1500,1000;1500,2100);(1700,1500;1600,2500);(1800,2500;1800,3500);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_inside_differential (r).first, "(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1600,-800;1600,-200);(1900,1000;1900,2000)"), true);
  EXPECT_EQ (db::compare (e.selected_inside_differential (r).second, "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1500,1000;1500,2100);(1700,1500;1600,2500);(1800,2500;1800,3500);(-1500,0;-1500,1000)"), true);
}

//  Edges::selected_inside(edges)
TEST(25)
{
  db::Edges e;
  e.insert (db::Edge (0, 0, 0, 1000));
  e.insert (db::Edge (100, 0, 100, 3000));
  e.insert (db::Edge (1100, -1000, 1100, 2000));
  e.insert (db::Edge (1200, -1000, 1200, 0));
  e.insert (db::Edge (1300, -800, 1300, -200));
  e.insert (db::Edge (1400, 1000, 1400, 1100));
  e.insert (db::Edge (1500, 1000, 1500, 2100));
  e.insert (db::Edge (1600, -800, 1600, -400));
  e.insert (db::Edge (1600, -400, 1600, -200));
  e.insert (db::Edge (1700, 1500, 1600, 2500));
  e.insert (db::Edge (1800, 2500, 1800, 3500));
  e.insert (db::Edge (1900, 1000, 1900, 2000));
  e.insert (db::Edge (-1500, 0, -1500, 1000));

  db::Edges ee;
  for (int i = 0; i <= 2000; i += 100) {
    ee.insert (db::Edge (i, -1000, i, 0));
  }
  for (int i = 1000; i <= 2000; i += 100) {
    ee.insert (db::Edge (i, 1000, i, 1500));
    ee.insert (db::Edge (i, 1500, i, 2000));
  }

  EXPECT_EQ (db::compare (e.selected_inside (db::Edges ()), ""), true);
  EXPECT_EQ (db::compare (e.selected_not_inside (db::Edges ()), "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-400);(1600,-400;1600,-200);(1700,1500;1600,2500);(1800,2500;1800,3500);(1900,1000;1900,2000);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_inside_differential (db::Edges ()).first, ""), true);
  EXPECT_EQ (db::compare (e.selected_inside_differential (db::Edges ()).second, "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-400);(1600,-400;1600,-200);(1700,1500;1600,2500);(1800,2500;1800,3500);(1900,1000;1900,2000);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_inside (ee), ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_not_inside (ee), ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_inside_differential (ee).first, ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_inside_differential (ee).second, ""), true);
  EXPECT_EQ (db::compare (e.selected_inside (ee), "(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1600,-800;1600,-200);(1900,1000;1900,2000)"), true);
  EXPECT_EQ (db::compare (e.selected_not_inside (ee), "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1500,1000;1500,2100);(1700,1500;1600,2500);(1800,2500;1800,3500);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_inside_differential (ee).first, "(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1600,-800;1600,-200);(1900,1000;1900,2000)"), true);
  EXPECT_EQ (db::compare (e.selected_inside_differential (ee).second, "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1500,1000;1500,2100);(1700,1500;1600,2500);(1800,2500;1800,3500);(-1500,0;-1500,1000)"), true);
}

//  Edges::selected_outside(region)
TEST(26)
{
  db::Edges e;
  e.insert (db::Edge (0, 0, 0, 1000));
  e.insert (db::Edge (100, 0, 100, 3000));
  e.insert (db::Edge (1100, -1000, 1100, 2000));
  e.insert (db::Edge (1200, -1000, 1200, 0));
  e.insert (db::Edge (1300, -800, 1300, -200));
  e.insert (db::Edge (1400, 1000, 1400, 1100));
  e.insert (db::Edge (1500, 1000, 1500, 2100));
  e.insert (db::Edge (1600, -800, 1600, -400));
  e.insert (db::Edge (1600, -400, 1600, -200));
  e.insert (db::Edge (1700, 1500, 1600, 2500));
  e.insert (db::Edge (1800, 2500, 1800, 3500));
  e.insert (db::Edge (1900, 1000, 1900, 2000));
  e.insert (db::Edge (-1500, 0, -1500, 1000));

  db::Region r;
  r.insert (db::Box (0, -1000, 2000, 0));
  r.insert (db::Box (1000, 1000, 2000, 1500));
  r.insert (db::Box (1000, 1500, 2000, 2000));

  EXPECT_EQ (db::compare (e.selected_outside (db::Region ()), "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-400);(1600,-400;1600,-200);(1700,1500;1600,2500);(1800,2500;1800,3500);(1900,1000;1900,2000);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_not_outside (db::Region ()), ""), true);
  EXPECT_EQ (db::compare (e.selected_outside_differential (db::Region ()).first, "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-400);(1600,-400;1600,-200);(1700,1500;1600,2500);(1800,2500;1800,3500);(1900,1000;1900,2000);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_outside_differential (db::Region ()).second, ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_outside (r), ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_not_outside (r), ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_outside_differential (r).first, ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_outside_differential (r).second, ""), true);
  EXPECT_EQ (db::compare (e.selected_outside (r), "(0,0;0,1000);(100,0;100,3000);(1800,2500;1800,3500);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_not_outside (r), "(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-200);(1700,1500;1600,2500);(1900,1000;1900,2000)"), true);
  EXPECT_EQ (db::compare (e.selected_outside_differential (r).first, "(0,0;0,1000);(100,0;100,3000);(1800,2500;1800,3500);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_outside_differential (r).second, "(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-200);(1700,1500;1600,2500);(1900,1000;1900,2000)"), true);
}

//  Edges::selected_outside(edges)
TEST(27)
{
  db::Edges e;
  e.insert (db::Edge (0, 0, 0, 1000));
  e.insert (db::Edge (100, 0, 100, 3000));
  e.insert (db::Edge (1100, -1000, 1100, 2000));
  e.insert (db::Edge (1200, -1000, 1200, 0));
  e.insert (db::Edge (1300, -800, 1300, -200));
  e.insert (db::Edge (1400, 1000, 1400, 1100));
  e.insert (db::Edge (1500, 1000, 1500, 2100));
  e.insert (db::Edge (1600, -800, 1600, -400));
  e.insert (db::Edge (1600, -400, 1600, -200));
  e.insert (db::Edge (1700, 1500, 1600, 2500));
  e.insert (db::Edge (1800, 2500, 1800, 3500));
  e.insert (db::Edge (1900, 1000, 1900, 2000));
  e.insert (db::Edge (-1500, 0, -1500, 1000));

  db::Edges ee;
  for (int i = 0; i <= 2000; i += 100) {
    ee.insert (db::Edge (i, -1000, i, 0));
  }
  for (int i = 1000; i <= 2000; i += 100) {
    ee.insert (db::Edge (i, 1000, i, 1500));
    ee.insert (db::Edge (i, 1500, i, 2000));
  }

  EXPECT_EQ (db::compare (e.selected_outside (db::Edges ()), "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-400);(1600,-400;1600,-200);(1700,1500;1600,2500);(1800,2500;1800,3500);(1900,1000;1900,2000);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_not_outside (db::Edges ()), ""), true);
  EXPECT_EQ (db::compare (e.selected_outside_differential (db::Edges ()).first, "(0,0;0,1000);(100,0;100,3000);(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-400);(1600,-400;1600,-200);(1700,1500;1600,2500);(1800,2500;1800,3500);(1900,1000;1900,2000);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_outside_differential (db::Edges ()).second, ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_outside (ee), ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_not_outside (ee), ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_outside_differential (ee).first, ""), true);
  EXPECT_EQ (db::compare (db::Edges ().selected_outside_differential (ee).second, ""), true);
  EXPECT_EQ (db::compare (e.selected_outside (ee), "(0,0;0,1000);(100,0;100,3000);(1700,1500;1600,2500);(1800,2500;1800,3500);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_not_outside (ee), "(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-200);(1900,1000;1900,2000)"), true);
  EXPECT_EQ (db::compare (e.selected_outside_differential (ee).first, "(0,0;0,1000);(100,0;100,3000);(1700,1500;1600,2500);(1800,2500;1800,3500);(-1500,0;-1500,1000)"), true);
  EXPECT_EQ (db::compare (e.selected_outside_differential (ee).second, "(1100,-1000;1100,2000);(1200,-1000;1200,0);(1300,-800;1300,-200);(1400,1000;1400,1100);(1500,1000;1500,2100);(1600,-800;1600,-200);(1900,1000;1900,2000)"), true);
}

//  Edges::in and Edges:in_and_out
TEST(28)
{
  db::Edges e;
  e.insert (db::Edge (0, 0, 0, 1000));
  e.insert (db::Edge (0, 1000, 0, 2000));
  e.insert (db::Edge (100, 0, 100, 1000));

  db::Edges ee;
  ee.insert (db::Edge (0, 0, 0, 2000));
  ee.insert (db::Edge (100, 1000, 0, 2000));
  ee.insert (db::Edge (100, 0, 100, 1000));

  EXPECT_EQ (db::compare (e.in (db::Edges ()), ""), true);
  EXPECT_EQ (db::compare (e.in (db::Edges (), true), "(0,0;0,1000);(0,1000;0,2000);(100,0;100,1000)"), true);
  EXPECT_EQ (db::compare (e.in_and_out (db::Edges ()).first, ""), true);
  EXPECT_EQ (db::compare (e.in_and_out (db::Edges ()).second, "(0,0;0,1000);(0,1000;0,2000);(100,0;100,1000)"), true);
  EXPECT_EQ (db::compare (db::Edges ().in (ee), ""), true);
  EXPECT_EQ (db::compare (db::Edges ().in (ee, true), ""), true);
  EXPECT_EQ (db::compare (db::Edges ().in_and_out (ee).first, ""), true);
  EXPECT_EQ (db::compare (db::Edges ().in_and_out (ee).second, ""), true);
  EXPECT_EQ (db::compare (e.in (ee), "(0,0;0,2000);(100,0;100,1000)"), true);
  EXPECT_EQ (db::compare (e.in (ee, true), ""), true);
  EXPECT_EQ (db::compare (e.in_and_out (ee).first, "(0,0;0,2000);(100,0;100,1000)"), true);
  EXPECT_EQ (db::compare (e.in_and_out (ee).second, ""), true);
  EXPECT_EQ (db::compare (ee.in (e, true), "(100,1000;0,2000)"), true);
  EXPECT_EQ (db::compare (ee.in_and_out (e).second, "(100,1000;0,2000)"), true);

  e.set_merged_semantics (false);
  ee.set_merged_semantics (false);

  EXPECT_EQ (db::compare (e.in (ee), "(100,0;100,1000)"), true);
  EXPECT_EQ (db::compare (e.in (ee, true), "(0,0;0,1000);(0,1000;0,2000)"), true);
  EXPECT_EQ (db::compare (ee.in (e, true), "(0,0;0,2000);(100,1000;0,2000)"), true);
}

//  edge merge with dots -> dots are merged, but are retained
TEST(29)
{
  db::Edges e;
  e.insert (db::Edge (db::Point(0, 0), db::Point (100, 0)));
  e.insert (db::Edge (db::Point(110, 0), db::Point (110, 0)));
  EXPECT_EQ (e.merged ().to_string (), "(0,0;100,0);(110,0;110,0)");

  e.insert (db::Edge (db::Point(100, 0), db::Point (110, 0)));
  //  dots do not participate in merge
  EXPECT_EQ (e.merged ().to_string (), "(0,0;110,0)");

  e.clear ();
  e.insert (db::Edge (db::Point(110, 0), db::Point (110, 0)));
  e.insert (db::Edge (db::Point(110, 0), db::Point (110, 0)));
  //  dots do not participate in merge
  EXPECT_EQ (e.merged ().to_string (), "(110,0;110,0)");
}

//  interacting with count
TEST(30)
{
  db::Edges e;
  e.insert (db::Edge (db::Point (0, 0), db::Point (100, 0)));
  e.insert (db::Edge (db::Point (100, 0), db::Point (200, 0)));
  e.insert (db::Edge (db::Point (0, 10), db::Point (200, 10)));
  e.insert (db::Edge (db::Point (0, 20), db::Point (200, 20)));
  e.insert (db::Edge (db::Point (0, 30), db::Point (200, 30)));

  db::Edges e2;
  e2.insert (db::Edge (db::Point (100, 0), db::Point (100, 10)));
  e2.insert (db::Edge (db::Point (100, 0), db::Point (100, 30)));
  e2.insert (db::Edge (db::Point (110, 10), db::Point (110, 30)));
  e2.merge ();
  e2.insert (db::Edge (db::Point (120, 20), db::Point (120, 20)));
  e2.insert (db::Edge (db::Point (130, 30), db::Point (130, 30)));
  e2.set_merged_semantics (false);

  db::Edges edup;

  EXPECT_EQ (db::compare (e.selected_interacting (e2), "(0,0;200,0);(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting (e2, size_t (2)), "(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting (e2, size_t (2), size_t(2)), "(0,10;200,10)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting (e2, size_t (2), size_t(3)), "(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting (e2, size_t (3)), "(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting (e2, size_t (4)), ""), true);

  edup = e;
  edup.select_interacting (e2, size_t (2), size_t(3));
  EXPECT_EQ (db::compare (edup, "(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);

  EXPECT_EQ (db::compare (e.selected_not_interacting (e2), ""), true);
  EXPECT_EQ (db::compare (e.selected_not_interacting (e2, size_t (2)), "(0,0;200,0)"), true);
  EXPECT_EQ (db::compare (e.selected_not_interacting (e2, size_t (2), size_t(2)), "(0,0;200,0);(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_not_interacting (e2, size_t (2), size_t(3)), "(0,0;200,0)"), true);
  EXPECT_EQ (db::compare (e.selected_not_interacting (e2, size_t (3)), "(0,0;200,0);(0,10;200,10)"), true);
  EXPECT_EQ (db::compare (e.selected_not_interacting (e2, size_t (4)), "(0,0;200,0);(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);

  edup = e;
  edup.select_not_interacting (e2, size_t (2), size_t(3));
  EXPECT_EQ (db::compare (edup, "(0,0;200,0)"), true);

  EXPECT_EQ (db::compare (e.selected_interacting_differential (e2, size_t (2), size_t(3)).first, "(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting_differential (e2, size_t (2), size_t(3)).second, "(0,0;200,0)"), true);

  db::Region r2;
  r2.insert (db::Box (db::Point (99, 0), db::Point (101, 10)));
  r2.insert (db::Box (db::Point (99, 0), db::Point (101, 30)));
  r2.insert (db::Box (db::Point (109, 10), db::Point (111, 30)));
  r2.insert (db::Box (db::Point (119, 19), db::Point (121, 21)));
  r2.insert (db::Box (db::Point (129, 29), db::Point (131, 31)));

  EXPECT_EQ (db::compare (e.selected_interacting (r2), "(0,0;200,0);(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting (r2, size_t (2)), "(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting (r2, size_t (2), size_t(2)), "(0,10;200,10)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting (r2, size_t (2), size_t(3)), "(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting (r2, size_t (3)), "(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting (r2, size_t (4)), ""), true);

  edup = e;
  edup.select_interacting (r2, size_t (2), size_t(3));
  EXPECT_EQ (db::compare (edup, "(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);

  EXPECT_EQ (db::compare (e.selected_not_interacting (r2), ""), true);
  EXPECT_EQ (db::compare (e.selected_not_interacting (r2, size_t (2)), "(0,0;200,0)"), true);
  EXPECT_EQ (db::compare (e.selected_not_interacting (r2, size_t (2), size_t(2)), "(0,0;200,0);(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_not_interacting (r2, size_t (2), size_t(3)), "(0,0;200,0)"), true);
  EXPECT_EQ (db::compare (e.selected_not_interacting (r2, size_t (3)), "(0,0;200,0);(0,10;200,10)"), true);
  EXPECT_EQ (db::compare (e.selected_not_interacting (r2, size_t (4)), "(0,0;200,0);(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);

  edup = e;
  edup.select_not_interacting (r2, size_t (2), size_t(3));
  EXPECT_EQ (db::compare (edup, "(0,0;200,0)"), true);

  EXPECT_EQ (db::compare (e.selected_interacting_differential (r2, size_t (2), size_t(3)).first, "(0,10;200,10);(0,20;200,20);(0,30;200,30)"), true);
  EXPECT_EQ (db::compare (e.selected_interacting_differential (r2, size_t (2), size_t(3)).second, "(0,0;200,0)"), true);
}

//  borrowed from deep edges tests
TEST(31)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_edges_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l21 = ly.get_layer (db::LayerProperties (2, 1));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int lempty = ly.insert_layer ();

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region r21 (db::RecursiveShapeIterator (ly, top_cell, l21));
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));
  db::Region r2and3 = r2 & r3;

  db::Edges e2 = r2.edges ();
  db::Edges e21 = r21.edges ();
  db::Edges e3 = r3.edges ();
  db::Edges e3copy = r3.edges ();
  db::Edges e2and3 = r2and3.edges ();
  db::Edges eempty (db::RecursiveShapeIterator (ly, top_cell, lempty));
  db::Edges edots = e2and3.processed (db::EdgeSegmentSelector (-1, 0, 0));
  db::Edges edotscopy = e2and3.processed (db::EdgeSegmentSelector (-1, 0, 0));

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), r2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (3, 0)), r3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), e3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), e2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), edots);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), edots.merged ());

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), e3 & e2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), e3 & edots);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), e3 & eempty);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), e3 & e3copy);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (24, 0)), eempty & e2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (25, 0)), edots & edotscopy);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (26, 0)), edots & e2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (27, 0)), e21 & edots);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (28, 0)), edots & e21);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), e3 - e2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), e3 - edots);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (32, 0)), e3 - eempty);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (33, 0)), e3 - e3copy);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (34, 0)), eempty - e2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (35, 0)), edots - edotscopy);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (36, 0)), edots - e2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (37, 0)), e21 - edots);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (38, 0)), edots - e21);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 0)), e3 ^ e2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 0)), e3 ^ edots);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 0)), e3 ^ eempty);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (43, 0)), e3 ^ e3copy);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (44, 0)), eempty ^ e2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (45, 0)), edots ^ edotscopy);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (46, 0)), edots ^ e2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (47, 0)), e21 ^ edots);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (48, 0)), edots ^ e21);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (50, 0)), e3.andnot(e2and3).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (51, 0)), e3.andnot(edots).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (52, 0)), e3.andnot(eempty).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (53, 0)), e3.andnot(e3copy).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (54, 0)), eempty.andnot(e2and3).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (55, 0)), edots.andnot(edotscopy).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (56, 0)), edots.andnot(e2).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (57, 0)), e21.andnot(edots).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (58, 0)), edots.andnot(e21).first);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (60, 0)), e3.andnot(e2and3).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (61, 0)), e3.andnot(edots).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (62, 0)), e3.andnot(eempty).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (63, 0)), e3.andnot(e3copy).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (64, 0)), eempty.andnot(e2and3).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (65, 0)), edots.andnot(edotscopy).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (66, 0)), edots.andnot(e2).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (67, 0)), e21.andnot(edots).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (68, 0)), edots.andnot(e21).second);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (70, 0)), e3.intersections(e2and3));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (71, 0)), e3.intersections(edots));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (72, 0)), e3.intersections(eempty));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (73, 0)), e3.intersections(e3copy));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (74, 0)), eempty.intersections(e2and3));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (75, 0)), edots.intersections(edotscopy));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (76, 0)), edots.intersections(e2));
  //  test, whether dots are not merged
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (77, 0)), edots.intersections(e2).select_interacting(e2));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (78, 0)), e21.intersections(edots));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (79, 0)), edots.intersections(e21));

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/deep_edges_au3_flat.gds");
}

TEST(32_add_with_properties)
{
  db::DeepShapeStore dss ("TOP", 0.001);
  db::Edges rd1 (dss), rd2 (dss);
  db::Edges rf1, rf2;

  db::PropertiesSet ps;
  ps.insert ("net", 17);
  db::properties_id_type pid = db::properties_id (ps);

  rf1.insert (db::EdgeWithProperties (db::Edge (-10, 20, 20, 60), pid));
  rd1.insert (db::EdgeWithProperties (db::Edge (-10, 20, 20, 60), pid));

  rf2.insert (db::EdgeWithProperties (db::Edge (10, 20, 40, 60), pid));
  rd2.insert (db::EdgeWithProperties (db::Edge (10, 20, 40, 60), pid));

  db::Layout ly;
  db::Cell &top_cell = ly.cell (ly.add_cell ("TOP"));
  unsigned int l1 = ly.insert_layer ();
  unsigned int l2 = ly.insert_layer ();

  top_cell.shapes (l1).insert (db::EdgeWithProperties (db::Edge (-10, 20, 20, 60), pid));
  top_cell.shapes (l2).insert (db::EdgeWithProperties (db::Edge (10, 20, 40, 60), pid));

  db::Edges ro1 (db::RecursiveShapeIterator (ly, top_cell, l1), false);
  db::Edges ro2 (db::RecursiveShapeIterator (ly, top_cell, l2), false);

  //  enable properties
  ro1.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  ro2.apply_property_translator (db::PropertiesTranslator::make_pass_all ());

  db::Edges r;
  r += rf1;
  r += rf2;
  EXPECT_EQ (r.to_string (), "(-10,20;20,60){net=>17};(10,20;40,60){net=>17}");
  EXPECT_EQ ((rf1 + rf2).to_string (), "(-10,20;20,60){net=>17};(10,20;40,60){net=>17}");

  r = db::Edges ();
  r += rd1;
  r += rf2;
  EXPECT_EQ (r.to_string (), "(-10,20;20,60){net=>17};(10,20;40,60){net=>17}");
  EXPECT_EQ ((rd1 + rf2).to_string (), "(-10,20;20,60){net=>17};(10,20;40,60){net=>17}");

  r = db::Edges ();
  r += rf1;
  r += rd2;
  EXPECT_EQ (r.to_string (), "(-10,20;20,60){net=>17};(10,20;40,60){net=>17}");
  EXPECT_EQ ((rf1 + rd2).to_string (), "(-10,20;20,60){net=>17};(10,20;40,60){net=>17}");

  r = db::Edges ();
  r += rd1;
  r += rd2;
  EXPECT_EQ (r.to_string (), "(-10,20;20,60){net=>17};(10,20;40,60){net=>17}");
  EXPECT_EQ ((rd1 + rd2).to_string (), "(-10,20;20,60){net=>17};(10,20;40,60){net=>17}");

  r = db::Edges ();
  r += ro1;
  r += ro2;
  EXPECT_EQ (r.to_string (), "(-10,20;20,60){net=>17};(10,20;40,60){net=>17}");
  EXPECT_EQ ((ro1 + ro2).to_string (), "(-10,20;20,60){net=>17};(10,20;40,60){net=>17}");

  r = db::Edges ();
  r += ro1;
  r += rf2;
  EXPECT_EQ (r.to_string (), "(10,20;40,60){net=>17};(-10,20;20,60){net=>17}");
  EXPECT_EQ ((ro1 + rf2).to_string (), "(10,20;40,60){net=>17};(-10,20;20,60){net=>17}");
}

TEST(33_properties)
{
  db::PropertiesSet ps;

  ps.insert (tl::Variant ("id"), 1);
  db::properties_id_type pid1 = db::properties_id (ps);

  db::Edges edges;
  edges.insert (db::EdgeWithProperties (db::Edge (db::Point (0, 0), db::Point (10, 20)), pid1));
  edges.insert (db::Edge (db::Point (0, 0), db::Point (10, 20)));

  EXPECT_EQ (edges.nth (0)->to_string (), "(0,0;10,20)");
  EXPECT_EQ (edges.nth (1)->to_string (), "(0,0;10,20)");
  EXPECT_EQ (edges.nth (2) == 0, true);

  EXPECT_EQ (edges.nth_prop_id (0), db::properties_id_type (0));
  EXPECT_EQ (edges.nth_prop_id (1), pid1);
}

//  GitHub issue #72 (Edges/Region NOT issue)
TEST(100)
{
  db::Edges e;
  e.insert (db::Edge (0, 0, 0, 1000));
  e.insert (db::Edge (0, 1000, 3000, 1000));
  e.insert (db::Edge (3000, 1000, 3000, 0));
  e.insert (db::Edge (3000, 0, 0, 0));

  db::Region r;
  r.insert (db::Box (1000, -1000, 2000, 0));
  r.insert (db::Box (1000, 1000, 2000, 2000));

  EXPECT_EQ (db::compare ((e - r), "(0,0;0,1000);(1000,0;0,0);(3000,0;2000,0);(3000,1000;3000,0);(0,1000;1000,1000);(2000,1000;3000,1000)"), true);

  r.clear ();
  r.insert (db::Box (1000, -1000, 2000, 2000));

  EXPECT_EQ (db::compare ((e - r), "(0,0;0,1000);(1000,0;0,0);(3000,0;2000,0);(3000,1000;3000,0);(0,1000;1000,1000);(2000,1000;3000,1000)"), true);

  e.clear ();
  e.insert (db::Edge (0, 0, 100, 1000));
  e.insert (db::Edge (100, 1000, 3100, 1000));
  e.insert (db::Edge (3100, 1000, 3000, 0));
  e.insert (db::Edge (3000, 0, 0, 0));

  r.clear ();
  r.insert (db::Box (1000, -1000, 2000, 0));
  r.insert (db::Box (1000, 1000, 2000, 2000));

  EXPECT_EQ (db::compare ((e - r), "(0,0;100,1000);(1000,0;0,0);(3000,0;2000,0);(3100,1000;3000,0);(100,1000;1000,1000);(2000,1000;3100,1000)"), true);

  r.clear ();
  r.insert (db::Box (1000, -1000, 2000, 2000));

  EXPECT_EQ (db::compare ((e - r), "(0,0;100,1000);(1000,0;0,0);(3000,0;2000,0);(3100,1000;3000,0);(100,1000;1000,1000);(2000,1000;3100,1000)"), true);

  e.clear ();
  e.insert (db::Edge (0, 0, 1000, 0));
  e.insert (db::Edge (1000, 0, 1000, 3000));
  e.insert (db::Edge (1000, 3000, 0, 3000));
  e.insert (db::Edge (0, 3000, 0, 0));

  r.clear ();
  r.insert (db::Box (-1000, 1000, 0, 2000));
  r.insert (db::Box (1000, 1000, 2000, 2000));

  EXPECT_EQ (db::compare ((e - r), "(0,1000;0,0);(0,0;1000,0);(1000,0;1000,1000);(0,3000;0,2000);(1000,2000;1000,3000);(1000,3000;0,3000)"), true);

  r.clear ();
  r.insert (db::Box (-1000, 1000, 2000, 2000));

  EXPECT_EQ (db::compare ((e - r), "(0,1000;0,0);(0,0;1000,0);(1000,0;1000,1000);(0,3000;0,2000);(1000,2000;1000,3000);(1000,3000;0,3000)"), true);
}


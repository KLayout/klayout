
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
#include "dbEdgeProcessor.h"
#include "dbPolygonGenerators.h"
#include "tlUnitTest.h"

TEST(1) 
{
  db::Box box (0, 0, 1000, 1000);
  db::Polygon in (box);
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (0, 500), db::Point (1, 500)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,500;1000,500;1000,0)");

  right_of.clear ();

  db::cut_polygon (in, db::Edge (db::Point (0, -100), db::Point (1, -100)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (0));

  right_of.clear ();

  db::cut_polygon (in, db::Edge (db::Point (0, 0), db::Point (1, 0)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (0));

  right_of.clear ();

  db::cut_polygon (in, db::Edge (db::Point (0, 1000), db::Point (1, 1000)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,1000;1000,1000;1000,0)");

  right_of.clear ();

  db::cut_polygon (in, db::Edge (db::Point (0, 1001), db::Point (1, 1001)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,1000;1000,1000;1000,0)");
}

TEST(2) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (0, 0));
  c.push_back (db::Point (0, 400));
  c.push_back (db::Point (400, 400));
  c.push_back (db::Point (400, 100));
  c.push_back (db::Point (600, 100));
  c.push_back (db::Point (600, 300));
  c.push_back (db::Point (700, 300));
  c.push_back (db::Point (700, 0));
  c.push_back (db::Point (300, 0));
  c.push_back (db::Point (300, 300));
  c.push_back (db::Point (100, 300));
  c.push_back (db::Point (100, 100));
  c.push_back (db::Point (200, 100));
  c.push_back (db::Point (200, 0));

  db::Polygon in;
  in.assign_hull (c.begin (), c.end ());
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (0, 200), db::Point (1, 200)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (2));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,200;100,200;100,100;200,100;200,0)");
  EXPECT_EQ (right_of[1].to_string (), "(300,0;300,200;400,200;400,100;600,100;600,200;700,200;700,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 100), db::Point (1, 100)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (2));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,100;200,100;200,0)");
  EXPECT_EQ (right_of[1].to_string (), "(300,0;300,100;700,100;700,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 50), db::Point (1, 50)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (2));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,50;200,50;200,0)");
  EXPECT_EQ (right_of[1].to_string (), "(300,0;300,50;700,50;700,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 300), db::Point (1, 300)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (2));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,300;100,300;100,100;200,100;200,0)");
  EXPECT_EQ (right_of[1].to_string (), "(300,0;300,300;400,300;400,100;600,100;600,300;700,300;700,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 400), db::Point (1, 400)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,400;400,400;400,100;600,100;600,300;700,300;700,0;300,0;300,300;100,300;100,100;200,100;200,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 500), db::Point (1, 500)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,400;400,400;400,100;600,100;600,300;700,300;700,0;300,0;300,300;100,300;100,100;200,100;200,0)");
}

TEST(3) 
{
  db::Box box (0, 0, 1000, 1000);
  db::Polygon in (box);

  std::vector <db::Point> c;
  c.push_back (db::Point (100, 100));
  c.push_back (db::Point (100, 400));
  c.push_back (db::Point (200, 400));
  c.push_back (db::Point (200, 100));
  in.insert_hole (c.begin (), c.end ());

  c.clear ();
  c.push_back (db::Point (400, 100));
  c.push_back (db::Point (400, 400));
  c.push_back (db::Point (500, 400));
  c.push_back (db::Point (500, 100));
  in.insert_hole (c.begin (), c.end ());

  std::vector<db::Polygon> right_of;
  db::cut_polygon (in, db::Edge (db::Point (0, 200), db::Point (1, 200)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,200;100,200;100,100;200,100;200,200;400,200;400,100;500,100;500,200;1000,200;1000,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 50), db::Point (1, 50)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,50;1000,50;1000,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 500), db::Point (1, 500)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,500;1000,500;1000,0/100,100;200,100;200,400;100,400/400,100;500,100;500,400;400,400)");
}

TEST(4) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (0, 0));
  c.push_back (db::Point (0, 400));
  c.push_back (db::Point (400, 400));
  c.push_back (db::Point (400, 200));
  c.push_back (db::Point (300, 200));
  c.push_back (db::Point (300, 100));
  c.push_back (db::Point (400, 100));
  c.push_back (db::Point (400, 400));
  c.push_back (db::Point (600, 400));
  c.push_back (db::Point (600, 0));

  db::Polygon in;
  in.assign_hull (c.begin (), c.end ());
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (0, 300), db::Point (1, 300)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,300;400,300;400,200;300,200;300,100;400,100;400,300;600,300;600,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (1, 300), db::Point (0, 300)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (2));
  EXPECT_EQ (right_of[0].to_string (), "(400,300;400,400;600,400;600,300)");
  EXPECT_EQ (right_of[1].to_string (), "(0,300;0,400;400,400;400,300)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 50), db::Point (1, 50)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,50;600,50;600,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 100), db::Point (1, 100)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,100;600,100;600,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 150), db::Point (1, 150)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,150;300,150;300,100;400,100;400,150;600,150;600,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (0, 200), db::Point (1, 200)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,200;300,200;300,100;400,100;400,200;600,200;600,0)");

}


TEST(5) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (0, 0));
  c.push_back (db::Point (0, 884));
  c.push_back (db::Point (1010, 884));
  c.push_back (db::Point (1010, 396));
  c.push_back (db::Point (565, 396));
  c.push_back (db::Point (565, 372));
  c.push_back (db::Point (568, 372));
  c.push_back (db::Point (568, 396));
  c.push_back (db::Point (1010, 396));
  c.push_back (db::Point (1010, 332));
  c.push_back (db::Point (72, 332));
  c.push_back (db::Point (72, 313));
  c.push_back (db::Point (89, 313));
  c.push_back (db::Point (89, 332));
  c.push_back (db::Point (1010, 332));
  c.push_back (db::Point (1010, 327));
  c.push_back (db::Point (173, 327));
  c.push_back (db::Point (173, 304));
  c.push_back (db::Point (211, 304));
  c.push_back (db::Point (211, 327));
  c.push_back (db::Point (1010, 327));
  c.push_back (db::Point (1010, 302));
  c.push_back (db::Point (174, 302));
  c.push_back (db::Point (174, 275));
  c.push_back (db::Point (212, 275));
  c.push_back (db::Point (212, 302));
  c.push_back (db::Point (1010, 302));
  c.push_back (db::Point (1010, 268));
  c.push_back (db::Point (47, 268));
  c.push_back (db::Point (47, 257));
  c.push_back (db::Point (62, 257));
  c.push_back (db::Point (62, 268));
  c.push_back (db::Point (1010, 268));
  c.push_back (db::Point (1010, 243));
  c.push_back (db::Point (49, 243));
  c.push_back (db::Point (49, 231));
  c.push_back (db::Point (63, 231));
  c.push_back (db::Point (63, 243));
  c.push_back (db::Point (1010, 243));
  c.push_back (db::Point (1010, 214));
  c.push_back (db::Point (72, 214));
  c.push_back (db::Point (72, 194));
  c.push_back (db::Point (93, 194));
  c.push_back (db::Point (93, 214));
  c.push_back (db::Point (1010, 214));
  c.push_back (db::Point (1010, 77));
  c.push_back (db::Point (5, 77));
  c.push_back (db::Point (5, 15));
  c.push_back (db::Point (67, 15));
  c.push_back (db::Point (67, 77));
  c.push_back (db::Point (1010, 77));
  c.push_back (db::Point (1010, 38));
  c.push_back (db::Point (328, 38));
  c.push_back (db::Point (328, 17));
  c.push_back (db::Point (405, 17));
  c.push_back (db::Point (405, 38));
  c.push_back (db::Point (1010, 38));
  c.push_back (db::Point (1010, 0));

  db::Polygon in;
  in.assign_hull (c.begin (), c.end ());
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (565, 1), db::Point (565, 0)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,884;565,884;565,332;72,332;72,313;89,313;89,332;565,332;565,327;173,327;173,304;211,304;211,327;565,327;565,302;174,302;174,275;212,275;212,302;565,302;565,268;47,268;47,257;62,257;62,268;565,268;565,243;49,243;49,231;63,231;63,243;565,243;565,214;72,214;72,194;93,194;93,214;565,214;565,77;5,77;5,15;67,15;67,77;565,77;565,38;328,38;328,17;405,17;405,38;565,38;565,0)");
}

TEST(6) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (0, 0));
  c.push_back (db::Point (0, 100));
  c.push_back (db::Point (100, 100));
  c.push_back (db::Point (200, 200));
  c.push_back (db::Point (300, 100));
  c.push_back (db::Point (400, 100));
  c.push_back (db::Point (400, 400));
  c.push_back (db::Point (500, 400));
  c.push_back (db::Point (500, 0));

  db::Polygon in;
  in.assign_hull (c.begin (), c.end ());
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (0, 200), db::Point (1, 200)), std::back_inserter (right_of));

  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,100;100,100;200,200;300,100;400,100;400,200;500,200;500,0)");
  c.push_back (db::Point ());
}

TEST(7) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (0, 0));
  c.push_back (db::Point (0, 1));
  c.push_back (db::Point (3, 1));
  c.push_back (db::Point (3, 0));
  c.push_back (db::Point (0, 1));
  c.push_back (db::Point (2, 0));

  db::Polygon in;
  in.assign_hull (c.begin (), c.end ());
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (2, 0), db::Point (2, 1)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(2,0;2,1;3,1;3,0)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (2, 1), db::Point (2, 0)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,1;2,1;2,0)");

}

TEST(8) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (0, 0));
  c.push_back (db::Point (0, 300));
  c.push_back (db::Point (300, 300));
  c.push_back (db::Point (200, 200));
  c.push_back (db::Point (100, 200));
  c.push_back (db::Point (100, 100));
  c.push_back (db::Point (200, 200));
  c.push_back (db::Point (150, 50));
  c.push_back (db::Point (300, 50));
  c.push_back (db::Point (300, 0));

  db::Polygon in;
  in.assign_hull (c.begin (), c.end ());
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (200, 0), db::Point (200, 1)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (2));
  EXPECT_EQ (right_of[0].to_string (), "(200,0;200,50;300,50;300,0)");
  EXPECT_EQ (right_of[1].to_string (), "(200,200;200,300;300,300)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (200, 1), db::Point (200, 0)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(0,0;0,300;200,300;200,200;100,200;100,100;200,200;150,50;200,50;200,0)");

}

TEST(9) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (0, 0));
  c.push_back (db::Point (0, 200));
  c.push_back (db::Point (250, 200));
  c.push_back (db::Point (250, 100));
  c.push_back (db::Point (300, 100));
  c.push_back (db::Point (300, 200));
  c.push_back (db::Point (0, 200));
  c.push_back (db::Point (0, 500));
  c.push_back (db::Point (400, 500));
  c.push_back (db::Point (400, 400));
  c.push_back (db::Point (100, 400));
  c.push_back (db::Point (100, 300));
  c.push_back (db::Point (150, 300));
  c.push_back (db::Point (150, 400));
  c.push_back (db::Point (400, 400));
  c.push_back (db::Point (400, 0));

  db::Polygon in;
  in.assign_hull (c.begin (), c.end ());
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (200, 0), db::Point (200, 1)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (2));
  EXPECT_EQ (right_of[0].to_string (), "(200,0;200,200;250,200;250,100;300,100;300,200;200,200;200,400;400,400;400,0)");
  EXPECT_EQ (right_of[1].to_string (), "(200,400;200,500;400,500;400,400)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (200, 1), db::Point (200, 0)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (2));
  EXPECT_EQ (right_of[0].to_string (), "(0,200;0,500;200,500;200,400;100,400;100,300;150,300;150,400;200,400;200,200)");
  EXPECT_EQ (right_of[1].to_string (), "(0,0;0,200;200,200;200,0)");
}

TEST(9a) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (942, 10230));
  c.push_back (db::Point (943, 10272));
  c.push_back (db::Point (988, 10278));
  c.push_back (db::Point (999, 10278));
  c.push_back (db::Point (1002, 10280));
  c.push_back (db::Point (1034, 10280));
  c.push_back (db::Point (1032, 10285));
  c.push_back (db::Point (1090, 10285));
  c.push_back (db::Point (1090, 10302));
  c.push_back (db::Point (1043, 10302));
  c.push_back (db::Point (1041, 10286));
  c.push_back (db::Point (1036, 10285));
  c.push_back (db::Point (1031, 10297));
  c.push_back (db::Point (1027, 10297));
  c.push_back (db::Point (1032, 10285));
  c.push_back (db::Point (1022, 10283));
  c.push_back (db::Point (1024, 10288));
  c.push_back (db::Point (1011, 10288));
  c.push_back (db::Point (1017, 10283));
  c.push_back (db::Point (1003, 10281));
  c.push_back (db::Point (1011, 10288));
  c.push_back (db::Point (1024, 10288));
  c.push_back (db::Point (1027, 10297));
  c.push_back (db::Point (1031, 10297));
  c.push_back (db::Point (1029, 10302));
  c.push_back (db::Point (1027, 10297));
  c.push_back (db::Point (1026, 10300));
  c.push_back (db::Point (1028, 10302));
  c.push_back (db::Point (994, 10302));
  c.push_back (db::Point (1002, 10280));
  c.push_back (db::Point (988, 10278));
  c.push_back (db::Point (983, 10281));
  c.push_back (db::Point (942, 10281));
  c.push_back (db::Point (942, 10230));
  c.push_back (db::Point (1019, 10230));
  c.push_back (db::Point (1017, 10237));
  c.push_back (db::Point (1027, 10252));
  c.push_back (db::Point (1090, 10252));
  c.push_back (db::Point (1090, 10285));
  c.push_back (db::Point (1036, 10285));
  c.push_back (db::Point (1039, 10277));
  c.push_back (db::Point (1038, 10269));
  c.push_back (db::Point (1034, 10280));
  c.push_back (db::Point (1021, 10280));
  c.push_back (db::Point (1037, 10266));
  c.push_back (db::Point (1027, 10252));
  c.push_back (db::Point (1014, 10260));
  c.push_back (db::Point (1016, 10265));
  c.push_back (db::Point (1007, 10265));
  c.push_back (db::Point (1014, 10260));
  c.push_back (db::Point (1012, 10252));
  c.push_back (db::Point (1007, 10265));
  c.push_back (db::Point (1016, 10265));
  c.push_back (db::Point (1021, 10280));
  c.push_back (db::Point (1002, 10280));
  c.push_back (db::Point (1007, 10265));
  c.push_back (db::Point (994, 10274));
  c.push_back (db::Point (999, 10278));
  c.push_back (db::Point (988, 10278));
  c.push_back (db::Point (994, 10274));

  db::Polygon in;
  db::Polygon::contour_type contour;
  contour.assign (c.begin (), c.end (), false, false, true);
  in.assign_hull (contour);
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (1016, 0), db::Point (1016, 1)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (4));
  EXPECT_EQ (right_of[0].to_string (), "(1016,10230;1016,10259;1027,10252;1037,10266;1021,10280;1034,10280;1038,10269;1039,10277;1036,10285;1090,10285;1090,10252;1027,10252;1017,10237;1019,10230)");
  EXPECT_EQ (right_of[1].to_string (), "(1016,10265;1016,10280;1021,10280)");
  EXPECT_EQ (right_of[2].to_string (), "(1016,10280;1016,10283;1017,10283;1016,10284;1016,10288;1024,10288;1022,10283;1032,10285;1027,10297;1031,10297;1036,10285;1041,10286;1043,10302;1090,10302;1090,10285;1032,10285;1034,10280)");
  EXPECT_EQ (right_of[3].to_string (), "(1016,10288;1016,10302;1028,10302;1026,10300;1027,10297;1029,10302;1031,10297;1027,10297;1024,10288)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (1016, 1), db::Point (1016, 0)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (2));
  EXPECT_EQ (right_of[0].to_string (), "(942,10230;994,10274;988,10278;999,10278;994,10274;1007,10265;1002,10280;1016,10280;1016,10265;1007,10265;1012,10252;1014,10260;1007,10265;1016,10265;1014,10260;1016,10259;1016,10230;942,10230;942,10281;983,10281;988,10278;1002,10280;994,10302;1016,10302;1016,10288;1011,10288;1003,10281;1016,10283;1016,10280;1002,10280;999,10278;988,10278;943,10272)");
  EXPECT_EQ (right_of[1].to_string (), "(1016,10284;1011,10288;1016,10288)");
}

TEST(9b) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (942, 10230));
  c.push_back (db::Point (942, 10265));
  c.push_back (db::Point (943, 10265));
  c.push_back (db::Point (942, 10230));
  c.push_back (db::Point (983, 10265));
  c.push_back (db::Point (1007, 10265));
  c.push_back (db::Point (1012, 10252));
  c.push_back (db::Point (1014, 10260));
  c.push_back (db::Point (1007, 10265));
  c.push_back (db::Point (1016, 10265));
  c.push_back (db::Point (1014, 10260));
  c.push_back (db::Point (1016, 10259));
  c.push_back (db::Point (1016, 10230));

  db::Polygon in;
  db::Polygon::contour_type contour;
  contour.assign (c.begin (), c.end (), false, false, true);
  in.assign_hull (contour);
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (1007, 0), db::Point (1007, 1)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(1007,10230;1007,10265;1012,10252;1014,10260;1007,10265;1016,10265;1014,10260;1016,10259;1016,10230)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (1007, 1), db::Point (1007, 0)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(942,10230;942,10265;943,10265;942,10230;983,10265;1007,10265;1007,10230)");

}

TEST(9c) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (14335, 8265));
  c.push_back (db::Point (14335, 10265));
  c.push_back (db::Point (17335, 10265));
  c.push_back (db::Point (15335, 10265));
  c.push_back (db::Point (15335, 9765));
  c.push_back (db::Point (15668, 9932));
  c.push_back (db::Point (15335, 10265));
  c.push_back (db::Point (17335, 10265));
  c.push_back (db::Point (17335, 10015));
  c.push_back (db::Point (15835, 10015));
  c.push_back (db::Point (15668, 9932));
  c.push_back (db::Point (15835, 9765));
  c.push_back (db::Point (16002, 9932));
  c.push_back (db::Point (15835, 10015));
  c.push_back (db::Point (17335, 10015));
  c.push_back (db::Point (17335, 9765));
  c.push_back (db::Point (15335, 9765));
  c.push_back (db::Point (14335, 9265));
  c.push_back (db::Point (15335, 9265));
  c.push_back (db::Point (15335, 9765));
  c.push_back (db::Point (17335, 9765));
  c.push_back (db::Point (17335, 8265));
  c.push_back (db::Point (16335, 9265));
  c.push_back (db::Point (15335, 9265));

  db::Polygon in;
  db::Polygon::contour_type contour;
  contour.assign (c.begin (), c.end (), false, false, true);
  in.assign_hull (contour);
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (15835, 0), db::Point (15835, 1)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(17335,8265;16335,9265;15835,9265;15835,9765;16002,9932;15835,10015;15835,10265;17335,10265)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (15835, 1), db::Point (15835, 0)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (4));
  if (right_of.size () >= 4) {
    EXPECT_EQ (right_of[0].to_string (), "(14335,8265;14335,9265;15335,9265)");
    EXPECT_EQ (right_of[1].to_string (), "(15335,9265;15335,9765;15668,9932;15835,9765;15835,9265)");
    EXPECT_EQ (right_of[2].to_string (), "(14335,9265;14335,10265;15335,10265;15335,9765)");
    EXPECT_EQ (right_of[3].to_string (), "(15668,9932;15335,10265;15835,10265;15835,10015)");
  }
}

TEST(9d) 
{
  std::vector <db::Point> c;
  c.push_back (db::Point (17335, 8265));
  c.push_back (db::Point (16335, 9265));
  c.push_back (db::Point (15335, 9265));
  c.push_back (db::Point (15335, 9765));
  c.push_back (db::Point (15668, 9932));
  c.push_back (db::Point (15835, 9765));
  c.push_back (db::Point (16002, 9932));
  c.push_back (db::Point (15835, 10015));
  c.push_back (db::Point (15668, 9932));
  c.push_back (db::Point (15335, 10265));
  c.push_back (db::Point (17335, 10265));

  db::Polygon in;
  db::Polygon::contour_type contour;
  contour.assign (c.begin (), c.end (), false, false, true);
  in.assign_hull (contour);
  std::vector<db::Polygon> right_of;

  db::cut_polygon (in, db::Edge (db::Point (16002, 0), db::Point (16002, 1)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (1));
  EXPECT_EQ (right_of[0].to_string (), "(17335,8265;16335,9265;16002,9265;16002,10265;17335,10265)");

  right_of.clear ();
  db::cut_polygon (in, db::Edge (db::Point (16002, 1), db::Point (16002, 0)), std::back_inserter (right_of));
  EXPECT_EQ (right_of.size (), size_t (2));
  EXPECT_EQ (right_of[0].to_string (), "(15668,9932;15335,10265;16002,10265;16002,9932;15835,10015)");
  EXPECT_EQ (right_of[1].to_string (), "(15335,9265;15335,9765;15668,9932;15835,9765;16002,9932;16002,9265)");
}

TEST(10)
{
  //  Simple test for polygon-box/edge interaction (integer coordinates)
  db::Polygon poly;
  db::Point p[] = {
    db::Point (0, 100),
    db::Point (100, 100),
    db::Point (0, 0)
  };
  poly.assign_hull (p, p + sizeof (p) / sizeof (p[0]));

  EXPECT_EQ (interact (poly, db::Edge (-10, -10, -1, -1)), false);
  EXPECT_EQ (interact (poly, db::Edge (-10, -10, 0, 0)), true);
  EXPECT_EQ (interact (poly, db::Edge (-10, -10, 1, 1)), true);
  EXPECT_EQ (interact (poly, db::Edge (-10, -10, 20, 10)), false);
  EXPECT_EQ (interact (poly, db::Edge (-10, -10, 10, 20)), true);
  EXPECT_EQ (interact (poly, db::Edge (10, 20, 20, 30)), true);
  EXPECT_EQ (interact (poly, db::Edge (10, 20, 15, 25)), true);
  EXPECT_EQ (interact (poly, db::Edge (30, 10, 40, 20)), false);
  EXPECT_EQ (interact (poly, db::Edge (30, 20, 40, 50)), true);
  EXPECT_EQ (interact (poly, db::Edge (-10, 20, 0, 30)), true);
  EXPECT_EQ (interact (poly, db::Edge (-10, 20, -5, 30)), false);
  EXPECT_EQ (interact (poly, db::Edge (-10, 100, -5, 110)), false);
  EXPECT_EQ (interact (poly, db::Edge (-10, 100, 0, 110)), false);
  EXPECT_EQ (interact (poly, db::Edge (-10, 100, 5, 100)), true);

  EXPECT_EQ (interact (db::Box (0, 0, 100, 100), db::Box (-10, 100, 5, 110)), true);
  EXPECT_EQ (interact (db::Box (0, 0, 100, 100), db::Box (-10, -10, 110, 110)), true);
  EXPECT_EQ (interact (db::Box (0, 0, 100, 100), db::Box (-10, -10, 50, 110)), true);
  EXPECT_EQ (interact (db::Box (0, 0, 100, 100), db::Box ()), false);

  EXPECT_EQ (interact (poly, db::Box (-10, -10, -1, -1)), false);
  EXPECT_EQ (interact (poly, db::Box (-10, -10, 0, 0)), true);
  EXPECT_EQ (interact (poly, db::Box (-10, -10, 1, 1)), true);
  EXPECT_EQ (interact (poly, db::Box (-10, -10, 20, 10)), true);
  EXPECT_EQ (interact (poly, db::Box (10, 20, 20, 30)), true);
  EXPECT_EQ (interact (poly, db::Box (10, 20, 15, 25)), true);
  EXPECT_EQ (interact (poly, db::Box (30, 10, 40, 20)), false);
  EXPECT_EQ (interact (poly, db::Box (30, 20, 40, 30)), true);
  EXPECT_EQ (interact (poly, db::Box (-10, 20, 0, 30)), true);
  EXPECT_EQ (interact (poly, db::Box (-10, 20, -5, 30)), false);
  EXPECT_EQ (interact (poly, db::Box (-10, 100, -5, 110)), false);
  EXPECT_EQ (interact (poly, db::Box (-10, 100, 0, 110)), true);
  EXPECT_EQ (interact (poly, db::Box (-10, 100, 5, 110)), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (0, 0, 100, 100)), db::Box (-10, 100, 5, 110)), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (0, 0, 100, 100)), db::Box (-10, -10, 110, 110)), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (0, 0, 100, 100)), db::Box (-10, -10, 50, 110)), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (0, 0, 100, 100)), db::Box ()), false);
  EXPECT_EQ (interact (db::Polygon (), db::Box (-10, -10, 50, 110)), false);

  EXPECT_EQ (interact (poly, db::Polygon (db::Box (-10, -10, -1, -1))), false);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (-10, -10, 0, 0))), true);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (-10, -10, 1, 1))), true);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (-10, -10, 20, 10))), true);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (10, 20, 20, 30))), true);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (10, 20, 15, 25))), true);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (30, 10, 40, 20))), false);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (30, 20, 40, 30))), true);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (-10, 20, 0, 30))), true);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (-10, 20, -5, 30))), false);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (-10, 100, -5, 110))), false);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (-10, 100, 0, 110))), true);
  EXPECT_EQ (interact (poly, db::Polygon (db::Box (-10, 100, 5, 110))), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (0, 0, 100, 100)), db::Polygon (db::Box (-10, 100, 5, 110))), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (0, 0, 100, 100)), db::Polygon (db::Box (-10, -10, 110, 110))), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (0, 0, 100, 100)), db::Polygon (db::Box (-10, -10, 50, 110))), true);
  EXPECT_EQ (interact (db::Polygon (), db::Polygon (db::Box (-10, -10, 50, 110))), false);
  EXPECT_EQ (interact (db::Polygon (db::Box (0, 0, 100, 100)), db::Polygon ()), false);
  EXPECT_EQ (interact (db::Polygon (db::Box (0, 0, 100, 100)), db::Polygon (db::Box ())), false);

  EXPECT_EQ (interact (db::Polygon (db::Box (-10, -10, -1, -1)), poly), false);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, -10, 0, 0)), poly), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, -10, 1, 1)), poly), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, -10, 20, 10)), poly), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (10, 20, 20, 30)), poly), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (10, 20, 15, 25)), poly), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (30, 10, 40, 20)), poly), false);
  EXPECT_EQ (interact (db::Polygon (db::Box (30, 20, 40, 30)), poly), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, 20, 0, 30)), poly), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, 20, -5, 30)), poly), false);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, 100, -5, 110)), poly), false);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, 100, 0, 110)), poly), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, 100, 5, 110)), poly), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, 100, 5, 110)), db::Polygon (db::Box (0, 0, 100, 100))), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, -10, 110, 110)), db::Polygon (db::Box (0, 0, 100, 100))), true);
  EXPECT_EQ (interact (db::Polygon (db::Box (-10, -10, 50, 110)), db::Polygon (db::Box (0, 0, 100, 100))), true);
}

TEST(11)
{
  //  Simple test for polygon-box interaction (double coordinates)
  db::DPolygon poly;
  db::DPoint p[] = {
    db::DPoint (0, 100),
    db::DPoint (100, 100),
    db::DPoint (0, 0)
  };
  poly.assign_hull (p, p + sizeof (p) / sizeof (p[0]));

  EXPECT_EQ (interact (poly, db::DEdge (-1.0, -1.0, -0.1, -0.1)), false);
  EXPECT_EQ (interact (poly, db::DEdge (-10, -10, 0, 0)), true);
  EXPECT_EQ (interact (poly, db::DEdge (-0.01, -0.01, 0.001, 0.001)), true);
  EXPECT_EQ (interact (poly, db::DEdge (-10, -10, 20, 10)), false);
  EXPECT_EQ (interact (poly, db::DEdge (-10, -10, 10, 20)), true);
  EXPECT_EQ (interact (poly, db::DEdge (10, 20, 20, 30)), true);
  EXPECT_EQ (interact (poly, db::DEdge (10, 20, 15, 25)), true);
  EXPECT_EQ (interact (poly, db::DEdge (30, 10, 40, 20)), false);
  EXPECT_EQ (interact (poly, db::DEdge (30, 20, 40, 50)), true);
  EXPECT_EQ (interact (poly, db::DEdge (-10, 20, 0, 30)), true);
  EXPECT_EQ (interact (poly, db::DEdge (-10, 20, -5, 30)), false);
  EXPECT_EQ (interact (poly, db::DEdge (-10, 100, -5, 110)), false);
  EXPECT_EQ (interact (poly, db::DEdge (-10.0, 100.0, 0.0, 100.5)), false);
  EXPECT_EQ (interact (poly, db::DEdge (-10, 100, 5, 100)), true);

  EXPECT_EQ (interact (db::DBox (0, 0, 100, 100), db::DBox (-10, 100, 5, 110)), true);
  EXPECT_EQ (interact (db::DBox (0, 0, 100, 100), db::DBox (-10, -10, 110, 110)), true);
  EXPECT_EQ (interact (db::DBox (0, 0, 100, 100), db::DBox (-10, -10, 50, 110)), true);
  EXPECT_EQ (interact (db::DBox (0, 0, 100, 100), db::DBox ()), false);

  EXPECT_EQ (interact (poly, db::DBox (-10, -10, -1, -1)), false);
  EXPECT_EQ (interact (poly, db::DBox (-10, -10, 0, 0)), true);
  EXPECT_EQ (interact (poly, db::DBox (-10, -10, 1, 1)), true);
  EXPECT_EQ (interact (poly, db::DBox (-10, -10, 20, 10)), true);
  EXPECT_EQ (interact (poly, db::DBox (10, 20, 20, 30)), true);
  EXPECT_EQ (interact (poly, db::DBox (10, 20, 15, 25)), true);
  EXPECT_EQ (interact (poly, db::DBox (30, 10, 40, 20)), false);
  EXPECT_EQ (interact (poly, db::DBox (30, 20, 40, 30)), true);
  EXPECT_EQ (interact (poly, db::DBox (-10, 20, 0, 30)), true);
  EXPECT_EQ (interact (poly, db::DBox (-10, 20, -5, 30)), false);
  EXPECT_EQ (interact (poly, db::DBox (-10, 100, -5, 110)), false);
  EXPECT_EQ (interact (poly, db::DBox (-10, 100, 0, 110)), true);
  EXPECT_EQ (interact (poly, db::DBox (-10, 100, 5, 110)), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (0, 0, 100, 100)), db::DBox (-10, 100, 5, 110)), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (0, 0, 100, 100)), db::DBox (-10, -10, 110, 110)), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (0, 0, 100, 100)), db::DBox (-10, -10, 50, 110)), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (0, 0, 100, 100)), db::DBox ()), false);
  EXPECT_EQ (interact (db::DPolygon (), db::DBox (-10, -10, 50, 110)), false);

  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (-10, -10, -1, -1))), false);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (-10, -10, 0, 0))), true);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (-10, -10, 1, 1))), true);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (-10, -10, 20, 10))), true);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (10, 20, 20, 30))), true);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (10, 20, 15, 25))), true);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (30, 10, 40, 20))), false);
  // That is a numerical problem: this test fails
  // EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (30, 20, 40, 30))), true);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (-10, 20, 0, 30))), true);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (-10, 20, -5, 30))), false);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (-10, 100, -5, 110))), false);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (-10, 100, 0, 110))), true);
  EXPECT_EQ (interact (poly, db::DPolygon (db::DBox (-10, 100, 5, 110))), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (0, 0, 100, 100)), db::DPolygon (db::DBox (-10, 100, 5, 110))), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (0, 0, 100, 100)), db::DPolygon (db::DBox (-10, -10, 110, 110))), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (0, 0, 100, 100)), db::DPolygon (db::DBox (-10, -10, 50, 110))), true);
  EXPECT_EQ (interact (db::DPolygon (), db::DPolygon (db::DBox (-10, -10, 50, 110))), false);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (0, 0, 100, 100)), db::DPolygon ()), false);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (0, 0, 100, 100)), db::DPolygon (db::DBox ())), false);

  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, -10, -1, -1)), poly), false);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, -10, 0, 0)), poly), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, -10, 1, 1)), poly), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, -10, 20, 10)), poly), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (10, 20, 20, 30)), poly), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (10, 20, 15, 25)), poly), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (30, 10, 40, 20)), poly), false);
  // That is a numerical problem: this test fails
  // EXPECT_EQ (interact (db::DPolygon (db::DBox (30, 20, 40, 30)), poly), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, 20, 0, 30)), poly), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, 20, -5, 30)), poly), false);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, 100, -5, 110)), poly), false);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, 100, 0, 110)), poly), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, 100, 5, 110)), poly), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, 100, 5, 110)), db::DPolygon (db::DBox (0, 0, 100, 100))), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, -10, 110, 110)), db::DPolygon (db::DBox (0, 0, 100, 100))), true);
  EXPECT_EQ (interact (db::DPolygon (db::DBox (-10, -10, 50, 110)), db::DPolygon (db::DBox (0, 0, 100, 100))), true);
}

TEST(12)
{
  //  Simple test for polygon-box interaction (integer coordinates)
  db::Polygon poly;
  db::Point p[] = {
    db::Point (3595000+960,3812000+680),
    db::Point (3595000+960,3812000+1080),
    db::Point (3595000+680,3812000+1080),
    db::Point (3595000+680,3812000+1320),
    db::Point (3595000+1720,3812000+1320),
    db::Point (3595000+1720,3812000+1080),
    db::Point (3595000+1240,3812000+1080),
    db::Point (3595000+1240,3812000+680)
  };
  poly.assign_hull (p, p + sizeof (p) / sizeof (p[0]));

  db::Polygon poly2;
  db::Point p2[] = {
    db::Point (3595000+660-1000,3812000+480-1000),
    db::Point (3595000+660-1000,3812000+520),
    db::Point (3595000+480,3812000+520),
    db::Point (3595000+480,3812000+880),
    db::Point (3595000+760,3812000+880),
    db::Point (3595000+760,3812000+520),
    db::Point (3595000+1460,3812000+520),
    db::Point (3595000+1460,3812000+830),
    db::Point (3595000+1720,3812000+830),
    db::Point (3595000+1720,3812000+520),
    db::Point (3595000+1940,3812000+520),
    db::Point (3595000+1940,3812000+480-1000)
  };
  poly2.assign_hull (p2, p2 + sizeof (p2) / sizeof (p2[0]));

  EXPECT_EQ (interact (poly, poly2), false);
  EXPECT_EQ (interact (poly2, poly), false);
}


static std::string am_to_string (const db::AreaMap &am)
{
  std::string r;
  for (size_t i = 0; i < am.ny (); ++i) {
    if (i > 0) {
      r += ",";
    }
    r += "(";
    for (size_t j = 0; j < am.nx (); ++j) {
      if (j > 0) {
        r += ",";
      }
      r += tl::to_string (am.get (j, i));
    }
    r += ")";
  }
  return r;
}

TEST(20) 
{
  db::Box box (100, 100, 500, 500);
  db::Polygon in (box);

  db::AreaMap am (db::Point (0, 0), db::Vector (200, 200), 3, 3);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(10000,20000,10000),(20000,40000,20000),(10000,20000,10000)");
}

TEST(21) 
{
  db::Box box (200, 200, 400, 400);
  db::Polygon in (box);

  db::AreaMap am (db::Point (0, 0), db::Vector (200, 200), 3, 3);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(0,0,0),(0,40000,0),(0,0,0)");
}

TEST(22) 
{
  db::Box box (250, 250, 350, 350);
  db::Polygon in (box);

  db::AreaMap am (db::Point (0, 0), db::Vector (200, 200), 3, 3);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(0,0,0),(0,10000,0),(0,0,0)");
}

TEST(23) 
{
  db::Box box (-1000, -500, 2000, 3000);
  db::Polygon in (box);

  db::AreaMap am (db::Point (0, 0), db::Vector (200, 200), 3, 3);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(40000,40000,40000),(40000,40000,40000),(40000,40000,40000)");
}

TEST(24) 
{
  db::Point p[3] = {
    db::Point (0, 100),
    db::Point (500, 500),
    db::Point (0, 0)
  };

  db::Polygon in;
  in.assign_hull (&p[0], &p[3]);

  db::AreaMap am (db::Point (0, 0), db::Vector (100, 100), 5, 5);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(5000,0,0,0,0),(4000,4750,0,0,0),(0,2250,4000,0,0),(0,0,1000,2750,0),(0,0,0,250,1000)");
  EXPECT_EQ (am.total_area (), 25000);
}

TEST(25) 
{
  db::Point p[3] = {
    db::Point (0, 100),
    db::Point (600, 500),
    db::Point (300, 0)
  };

  db::Polygon in;
  in.assign_hull (&p[0], &p[3]);

  db::AreaMap am (db::Point (0, 0), db::Vector (100, 100), 5, 5);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(1650,5000,8350,3000,0),(3350,9175,10000,8660,330),(0,825,6650,10000,5000),(0,0,0,3350,8845),(0,0,0,0,825)");
  EXPECT_EQ (am.total_area (), 85010);

  am.reinitialize (db::Point (0, 0), db::Vector (100, 100), db::Vector (50, 50), 5, 5);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(0,0,1250,750,0),(825,2500,2500,2500,0),(0,0,2287,2500,1750),(0,0,0,825,2500),(0,0,0,0,0)");
  EXPECT_EQ (am.total_area (), 20187);

  am.reinitialize (db::Point (200, 0), db::Vector (100, 100), db::Vector (50, 50), 1, 1);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(1250)");

  am.reinitialize (db::Point (300, 0), db::Vector (100, 100), db::Vector (50, 50), 1, 1);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(750)");

  am.reinitialize (db::Point (400, 0), db::Vector (100, 100), db::Vector (50, 50), 1, 1);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(0)");

  am.reinitialize (db::Point (400, 100), db::Vector (100, 100), db::Vector (50, 50), 1, 1);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(0)");

  am.reinitialize (db::Point (400, 200), db::Vector (100, 100), db::Vector (50, 50), 1, 1);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(1750)");
}

TEST(26) 
{
  db::Point p[10] = {
    db::Point (0, 300),
    db::Point (300, 300),
    db::Point (300, 0),
    db::Point (100, 0),
    db::Point (100, 100),
    db::Point (200, 100),
    db::Point (200, 200),
    db::Point (100, 200),
    db::Point (100, 0),
    db::Point (0, 0)
  };

  db::Polygon in;
  in.assign_hull (&p[0], &p[10]);

  db::AreaMap am (db::Point (0, 0), db::Vector (100, 100), 3, 3);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(10000,10000,10000),(10000,0,10000),(10000,10000,10000)");
}

TEST(27) 
{
  db::Point p[10] = {
    db::Point (-100, 400),
    db::Point (400, 400),
    db::Point (400, -100),
    db::Point (100, -100),
    db::Point (100, 100),
    db::Point (200, 100),
    db::Point (200, 200),
    db::Point (100, 200),
    db::Point (100, -100),
    db::Point (-100, -100)
  };

  db::Polygon in;
  in.assign_hull (&p[0], &p[10]);

  db::AreaMap am (db::Point (0, 0), db::Vector (100, 100), 3, 3);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(10000,10000,10000),(10000,0,10000),(10000,10000,10000)");
}

TEST(28) 
{
  db::Point p[10] = {
    db::Point (-100, 400),
    db::Point (400, 400),
    db::Point (400, -100),
    db::Point (120, -100),
    db::Point (120, 120),
    db::Point (180, 120),
    db::Point (180, 180),
    db::Point (120, 180),
    db::Point (120, -100),
    db::Point (-100, -100)
  };

  db::Polygon in;
  in.assign_hull (&p[0], &p[10]);

  db::AreaMap am (db::Point (0, 0), db::Vector (100, 100), 3, 3);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(10000,10000,10000),(10000,6400,10000),(10000,10000,10000)");
}

TEST(29) 
{
  db::Point p[] = {
    db::Point (1600,7009),
    db::Point (1600,7351),
    db::Point (1335,8538),
    db::Point (1341,8545),
    db::Point (1669,8545),
    db::Point (1669,7009)
  };

  db::Polygon in;
  in.assign_hull (&p[0], &p[sizeof(p)/sizeof(p[0])]);

  db::AreaMap am (db::Point (1360,7038), db::Vector (60,60), 6, 26);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(0,0,0,0,3600,540),(0,0,0,0,3600,540),(0,0,0,0,3600,540),(0,0,0,0,3600,540),(0,0,0,0,3600,540),(0,0,0,235,3600,540),(0,0,0,1020,3600,540),(0,0,0,1830,3600,540),(0,0,0,2640,3600,540),(0,0,36,3411,3600,540),(0,0,630,3600,3600,540),(0,0,1440,3600,3600,540),(0,0,2250,3600,3600,540),(0,0,3060,3600,3600,540),(0,269,3589,3600,3600,540),(0,1050,3600,3600,3600,540),(0,1860,3600,3600,3600,540),(0,2670,3600,3600,3600,540),(52,3424,3600,3600,3600,540),(690,3600,3600,3600,3600,540),(1470,3600,3600,3600,3600,540),(2280,3600,3600,3600,3600,540),(3090,3600,3600,3600,3600,540),(3592,3600,3600,3600,3600,540),(3600,3600,3600,3600,3600,540),(420,420,420,420,420,63)");
}

TEST(30) 
{
  db::Point p[] = {
    db::Point (7161,-9547),
    db::Point (7128,-9531),
    db::Point (7128,-9198),
    db::Point (7398,-9198),
    db::Point (7398,-8928),
    db::Point (7668,-8928),
    db::Point (7668,-8658),
    db::Point (7938,-8658),
    db::Point (7938,-8388),
    db::Point (8208,-8388),
    db::Point (8208,-8118),
    db::Point (8478,-8118),
    db::Point (8478,-7848),
    db::Point (8748,-7848),
    db::Point (8748,-7578),
    db::Point (9045,-7578),
    db::Point (9061,-7610),
    db::Point (8951,-7759),
    db::Point (8550,-8245),
    db::Point (8121,-8703),
    db::Point (7661,-9133)
  };

  db::Polygon in;
  in.assign_hull (&p[0], &p[sizeof(p)/sizeof(p[0])]);

  db::AreaMap am (db::Point (7128,-9547), db::Vector (81,82), 24, 25);
  db::rasterize (in, am);

  EXPECT_EQ (am_to_string (am), "(5418,1071,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),(6642,6267,1967,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),(6642,6642,6582,3119,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),(6642,6642,6642,6642,4317,240,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),(1701,1701,1701,4995,6642,5303,735,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),(0,0,0,4428,6642,6642,5952,1127,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),(0,0,0,4428,6642,6642,6642,6178,1484,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0),(0,0,0,2430,3645,3645,4644,6642,6355,1859,0,0,0,0,0,0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,2214,6642,6642,6489,2275,0,0,0,0,0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,2214,6642,6642,6642,6582,2698,0,0,0,0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,1863,5589,5589,5589,6642,6632,2994,0,0,0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0,0,6642,6642,6624,2698,0,0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0,0,6642,6642,6642,6587,2379,0,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0,0,6642,6642,6642,6642,6537,2111,0,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0,0,891,891,891,4725,6642,6471,1859,0,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0,0,0,0,0,4428,6642,6642,6378,1570,0,0,0,0,0,0),(0,0,0,0,0,0,0,0,0,0,0,0,0,4428,6642,6642,6642,6166,960,0,0,0,0,0),(0,0,0,0,0,0,0,0,0,0,0,0,0,1890,2835,2835,4104,6642,5617,432,0,0,0,0),(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2214,6642,6642,4860,104,0,0,0),(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2214,6642,6642,6642,3854,0,0,0),(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1593,4779,4779,4779,6642,2788,0,0),(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6642,6538,1777,0),(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6642,6642,6110,704),(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6642,6642,6642,4539),(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,81,81,81,55)");
}

TEST(41)
{
  db::Point pattern [] = {
    db::Point (0, -100),
    db::Point (0, -50),
    db::Point (-100, -75),
    db::Point (0, 100),
    db::Point (50, 50),
    db::Point (100, 75),
    db::Point (100, 0),
    db::Point (100, -50)
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  db::Polygon pout = minkowski_sum (p, db::Edge (db::Point (10, 10), db::Point (210, 110)), true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-40;-90,-65;10,110;210,210;260,160;310,185;310,60)");

  pout = minkowski_sum (p, db::Edge (db::Point (10, 10), db::Point (10, 110)), true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-40;-90,-65;-90,35;10,210;60,160;110,185;110,-40)");

  pout = minkowski_sum (p, db::Edge (db::Point (10, 110), db::Point (10, 10)), true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-40;-90,-65;-90,35;10,210;60,160;110,185;110,-40)");

  pout = minkowski_sum (p, db::Edge (db::Point (10, 10), db::Point (210, 10)), true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-65;-90,-65;10,110;210,110;235,85;310,85;310,-40;210,-90)");

  pout = minkowski_sum (p, db::Edge (db::Point (210, 10), db::Point (10, 10)), true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-65;-90,-65;10,110;210,110;235,85;310,85;310,-40;210,-90)");

  pout = minkowski_sum (p, db::Edge (db::Point (10, 10), db::Point (210, -90)), true);

  EXPECT_EQ (pout.to_string (), "(210,-190;143,-157;110,-165;-90,-65;10,110;85,72;110,85;310,-15;310,-140)");

  std::vector <db::Point> c;
  c.push_back (db::Point (10, 10));
  c.push_back (db::Point (10, 110));
  c.push_back (db::Point (210, 110));
  c.push_back (db::Point (210, 10));
  c.push_back (db::Point (10, 10));

  pout = minkowski_sum (p, c, true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-65;-90,-65;-90,35;10,210;210,210;235,185;310,185;310,-40;210,-90)");

  c.clear ();
  c.push_back (db::Point (10, 10));
  c.push_back (db::Point (10, 310));
  c.push_back (db::Point (510, 310));
  c.push_back (db::Point (510, 10));
  c.push_back (db::Point (10, 10));

  pout = minkowski_sum (p, c, true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)");

  // test hole resolution btw
  EXPECT_EQ (db::resolve_holes (pout).to_string (), "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)");
  EXPECT_EQ (db::polygon_to_simple_polygon (pout).to_string (), "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)");

  pout = minkowski_sum (p, c, false);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-65;-90,-65;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90/110,110;410,110;410,210;110,210)");

  // test hole resolution btw
  EXPECT_EQ (db::resolve_holes (pout).to_string (), "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)");
  EXPECT_EQ (db::polygon_to_simple_polygon (pout).to_string (), "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)");
  EXPECT_EQ (db::simple_polygon_to_polygon (db::polygon_to_simple_polygon (pout)).to_string (), "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)");

  pout = minkowski_sum (p, db::Box (db::Point (10, 10), db::Point (210, 110)), true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-65;-90,-65;-90,35;10,210;210,210;235,185;310,185;310,-40;210,-90)");

  pout = minkowski_sum (p, db::Box (db::Point (10, 10), db::Point (510, 310)), false);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-65;-90,-65;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)");
}

TEST(42)
{
  db::Point pattern [] = {
    db::Point (0, -100),
    db::Point (0, -50),
    db::Point (-100, -75),
    db::Point (0, 100),
    db::Point (50, 50),
    db::Point (100, 75),
    db::Point (100, 0),
    db::Point (100, -50)
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  db::Point hole [] = {
    db::Point (20, -67), 
    db::Point (20, -30), 
    db::Point (15, -26), 
    db::Point (-60, -45), 
    db::Point (4, 68), 
    db::Point (46, 26), 
    db::Point (80, 43), 
    db::Point (80, -37)
  };

  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));
  
  db::Polygon pout = minkowski_sum (p, db::Edge (db::Point (10, 10), db::Point (30, 10)), true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-45;-70,-65;-90,-65;-15,65;27,65;-27,-29;25,-16;45,-16;50,-20;50,-47;90,-27;90,43;76,36;56,36;27,65;-15,65;-8,78;10,110;30,110;73,67;110,85;130,85;130,-40;30,-90)");

  pout = minkowski_sum (p, db::Edge (db::Point (10, 10), db::Point (110, 110)), true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-40;-90,-65;-8,78;10,110;110,210;160,160;210,185;210,60;110,-40)");

  pout = minkowski_sum (p, db::Edge (db::Point (10, 10), db::Point (50, 10)), true);

  EXPECT_EQ (pout.to_string (), "(10,-90;10,-50;-50,-65;-90,-65;-23,52;40,52;-3,-23;25,-16;65,-16;70,-20;70,-37;90,-27;90,36;56,36;40,52;-23,52;-8,78;10,110;50,110;87,73;110,85;150,85;150,-40;50,-90)");
}

//  smoothing
TEST(100) 
{
  db::Point pattern [] = {
    db::Point (0, -100),
    db::Point (0, 0),
    db::Point (50, 10),
    db::Point (100, -10),
    db::Point (150, 0),
    db::Point (150, -100),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  EXPECT_EQ (smooth (p, 5, true).to_string (),  "(0,-100;0,0;50,10;100,-10;150,0;150,-100)");
  EXPECT_EQ (smooth (p, 20, true).to_string (), "(0,-100;0,0;150,0;150,-100)");
}

//  smoothing
TEST(101) 
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (50, 10),
    db::Point (100, -10),
    db::Point (150, 0),
    db::Point (150, 100),
    db::Point (0, 100),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  EXPECT_EQ (smooth (p, 5, true).to_string (),  "(100,-10;50,10;0,0;0,100;150,100;150,0)");
  EXPECT_EQ (smooth (p, 20, true).to_string (), "(0,0;0,100;150,100;150,0)");
}

//  smoothing
TEST(102) 
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (50, 10),
    db::Point (100, -10),
    db::Point (150, 0),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  EXPECT_EQ (smooth (p, 20, true).to_string (),  "()");
  EXPECT_EQ (smooth (p, 5, true).to_string (), "(100,-10;150,0;0,0;50,10)");
}

//  smoothing
TEST(103)
{
  db::Point pattern [] = {
    db::Point (56852, -237283),
    db::Point (56961, -237258),
    db::Point (60061, -236492),
    db::Point (63152, -235686),
    db::Point (66231, -234839),
    db::Point (69300, -233952),
    db::Point (69407, -233919),
    db::Point (73105, -246382),
    db::Point (72992, -246417),
    db::Point (69760, -247351),
    db::Point (66516, -248243),
    db::Point (63261, -249092),
    db::Point (59995, -249899),
    db::Point (59881, -249925)
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  EXPECT_EQ (smooth (p, 0, true).to_string (),  "(59881,-249925;56852,-237283;56961,-237258;60061,-236492;63152,-235686;66231,-234839;69300,-233952;69407,-233919;73105,-246382;72992,-246417;69760,-247351;66516,-248243;63261,-249092;59995,-249899)");
  EXPECT_EQ (smooth (p, 50, true).to_string (),  "(59881,-249925;56852,-237283;63152,-235686;69407,-233919;73105,-246382;69760,-247351)");
  EXPECT_EQ (smooth (p, 5000, true).to_string (),  "(59881,-249925;56852,-237283;69407,-233919;73105,-246382)");
}

//  smoothing
TEST(104)
{
  db::Point pattern [] = {
    db::Point (-245, -942),
    db::Point (-942, -247),
    db::Point (-942, -246),
    db::Point (247, 943),
    db::Point (248, 943),
    db::Point (943, 246),
    db::Point (-244, -942)
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  EXPECT_EQ (smooth (p, 12, false).to_string (),  "(-244,-942;-942,-246;248,943;943,246)");
  EXPECT_EQ (smooth (p, 12, true).to_string (),  "(-245,-942;-942,-247;-942,-246;247,943;248,943;943,246;-244,-942)");
}

//  smoothing
TEST(105)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 1000),
    db::Point (100, 1000),
    db::Point (100, 1100),
    db::Point (800, 1100),
    db::Point (800, 1000),
    db::Point (2000, 1000),
    db::Point (2000, 0)
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  EXPECT_EQ (smooth (p, 0, false).to_string (),  "(0,0;0,1000;100,1000;100,1100;800,1100;800,1000;2000,1000;2000,0)");
  EXPECT_EQ (smooth (p, 50, false).to_string (),  "(0,0;0,1000;100,1000;100,1100;800,1100;800,1000;2000,1000;2000,0)");
  EXPECT_EQ (smooth (p, 80, false).to_string (),  "(0,0;0,1000;100,1100;800,1100;800,1000;2000,1000;2000,0)");
  EXPECT_EQ (smooth (p, 90, false).to_string (),  "(0,0;0,1000;800,1100;800,1000;2000,1000;2000,0)");
  EXPECT_EQ (smooth (p, 100, false).to_string (),  "(0,0;0,1000;2000,1000;2000,0)");
  EXPECT_EQ (smooth (p, 100, true).to_string (),  "(0,0;0,1000;100,1000;100,1100;800,1100;800,1000;2000,1000;2000,0)");
}

//  smoothing
TEST(106)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 73235),
    db::Point (100, 74568),
    db::Point (700, 82468),
    db::Point (1200, 90468),
    db::Point (2000, 106468),
    db::Point (2300, 114468),
    db::Point (2700, 130468),
    db::Point (2800, 138468),
    db::Point (2800, 154468),
    db::Point (2700, 162468),
    db::Point (2300, 178468),
    db::Point (2000, 186468),
    db::Point (1200, 202468),
    db::Point (700, 210468),
    db::Point (100, 218368),
    db::Point (0, 219701),
    db::Point (0, 272971),
    db::Point (126450, 272971),
    db::Point (126450, 0),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  EXPECT_EQ (smooth (p, 0, false).to_string (),  "(0,0;0,73235;100,74568;700,82468;1200,90468;2000,106468;2300,114468;2700,130468;2800,138468;2800,154468;2700,162468;2300,178468;2000,186468;1200,202468;700,210468;100,218368;0,219701;0,272971;126450,272971;126450,0)");
  EXPECT_EQ (smooth (p, 100, false).to_string (),  "(0,0;100,74568;1200,90468;2300,114468;2800,138468;2700,162468;2000,186468;700,210468;0,219701;0,272971;126450,272971;126450,0)");
  EXPECT_EQ (smooth (p, 100, true).to_string (),  "(0,0;0,73235;1200,90468;2300,114468;2800,138468;2800,154468;2000,186468;700,210468;0,219701;0,272971;126450,272971;126450,0)");
}

//  smoothing, small units
TEST(107)
{
  db::Point pattern [] = {
    db::Point (1, 1),
    db::Point (1, 2),
    db::Point (2, 2),
    db::Point (2, 4),
    db::Point (3, 4),
    db::Point (3, 5),
    db::Point (4, 5),
    db::Point (4, 7),
    db::Point (5, 7),
    db::Point (5, 8),
    db::Point (6, 8),
    db::Point (6, 9),
    db::Point (7, 9),
    db::Point (7, 16),
    db::Point (8, 16),
    db::Point (8, 17),
    db::Point (9, 17),
    db::Point (9, 18),
    db::Point (10, 18),
    db::Point (10, 19),
    db::Point (12, 19),
    db::Point (12, 20),
    db::Point (16, 20),
    db::Point (16, 21),
    db::Point (17, 21),
    db::Point (17, 22),
    db::Point (18, 22),
    db::Point (18, 23),
    db::Point (24, 23),
    db::Point (24, 15),
    db::Point (23, 15),
    db::Point (23, 14),
    db::Point (22, 14),
    db::Point (22, 12),
    db::Point (21, 12),
    db::Point (21, 10),
    db::Point (20, 10),
    db::Point (20, 8),
    db::Point (19, 8),
    db::Point (19, 6),
    db::Point (18, 6),
    db::Point (18, 4),
    db::Point (17, 4),
    db::Point (17, 3),
    db::Point (16, 3),
    db::Point (16, 1)
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  EXPECT_EQ (smooth (p, 0, false).to_string (),  "(1,1;1,2;2,2;2,4;3,4;3,5;4,5;4,7;5,7;5,8;6,8;6,9;7,9;7,16;8,16;8,17;9,17;9,18;10,18;10,19;12,19;12,20;16,20;16,21;17,21;17,22;18,22;18,23;24,23;24,15;23,15;23,14;22,14;22,12;21,12;21,10;20,10;20,8;19,8;19,6;18,6;18,4;17,4;17,3;16,3;16,1)");
  EXPECT_EQ (smooth (p, 1, false).to_string (),  "(1,1;2,4;4,5;4,7;7,9;7,16;10,18;18,22;24,23;24,15;22,14;18,4;17,4;16,1)");
}

//  rounding
TEST(200) 
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (100000, 0),
    db::Point (100000, 100000),
    db::Point (0, 100000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  {
    double rinner = 0.0, router = 0.0;
    unsigned int n;
    db::Polygon pr;
    db::Polygon pp = compute_rounded (p, 0, 20000, 200);
    EXPECT_EQ (pp.hull ().size (), size_t (200));
    EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);
    
    EXPECT_EQ (tl::to_string (rinner),  "0");
    EXPECT_EQ (tl::to_string (router),  "20000");
    EXPECT_EQ (tl::to_string (n),  "200");
    EXPECT_EQ (pr.to_string (),  "(0,0;0,100000;100000,100000;100000,0)");
  }

  {
    double rinner = 0.0, router = 0.0;
    unsigned int n;
    db::Polygon pr;
    db::Polygon pp = compute_rounded (p, 0, 50000, 200);
    EXPECT_EQ (pp.hull ().size (), size_t (200));
    EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);
    
    EXPECT_EQ (tl::to_string (rinner),  "0");
    EXPECT_EQ (tl::to_string (router),  "50000");
    EXPECT_EQ (tl::to_string (n),  "200");
    EXPECT_EQ (pr.to_string (),  "(0,0;0,100000;100000,100000;100000,0)");
  }

  {
    double rinner = 0.0, router = 0.0;
    unsigned int n;
    db::Polygon pr;
    db::Polygon pp = compute_rounded (p, 0, 70000, 200);
    EXPECT_EQ (pp.hull ().size (), size_t (200));
    EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);
    
    EXPECT_EQ (tl::to_string (rinner),  "0");
    EXPECT_EQ (tl::to_string (router),  "50000");
    EXPECT_EQ (tl::to_string (n),  "200");
    EXPECT_EQ (pr.to_string (),  "(0,0;0,100000;100000,100000;100000,0)");
  }
}

//  rounding
TEST(201) 
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (50000, 0),
    db::Point (50000, 100000),
    db::Point (0, 100000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  {
    double rinner = 0.0, router = 0.0;
    unsigned int n;
    db::Polygon pr;
    db::Polygon pp = compute_rounded (p, 0, 50000, 200);
    EXPECT_EQ (pp.hull ().size (), size_t (200));
    EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);
    
    EXPECT_EQ (tl::to_string (rinner),  "0");
    EXPECT_EQ (tl::to_string (router),  "25000");
    EXPECT_EQ (tl::to_string (n),  "200");
    EXPECT_EQ (pr.to_string (),  "(0,0;0,100000;50000,100000;50000,0)");
  }
}

//  rounding
TEST(202) 
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 600000),
    db::Point (400000, 600000),
    db::Point (400000, 400000),
    db::Point (600000, 400000),
    db::Point (600000, 0),
  };

  db::Point hole [] = {
    db::Point (100000, 100000),
    db::Point (100000, 500000),
    db::Point (300000, 500000),
    db::Point (300000, 300000),
    db::Point (500000, 300000),
    db::Point (500000, 100000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));
  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  {
    double rinner = 0.0, router = 0.0;
    unsigned int n;
    db::Polygon pr;
    db::Polygon pp = compute_rounded (p, 50000, 150000, 200);
    EXPECT_EQ (pp.hull ().size (), size_t (300));
    EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);
    
    EXPECT_EQ (tl::to_string (rinner),  "50000");
    EXPECT_EQ (tl::to_string (router),  "150000");
    EXPECT_EQ (tl::to_string (n),  "200");
    EXPECT_EQ (pr.to_string (),  "(0,0;0,600000;400000,600000;400000,400000;600000,400000;600000,0/100000,100000;500000,100000;500000,300000;300000,300000;300000,500000;100000,500000)");
  }

  {
    double rinner = 0.0, router = 0.0;
    unsigned int n;
    db::Polygon pr;
    db::Polygon pp = compute_rounded (p, 100000, 150000, 200);
    EXPECT_EQ (pp.hull ().size (), size_t (300));
    EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);
    
    EXPECT_EQ (tl::to_string (rinner),  "92000");
    EXPECT_EQ (tl::to_string (router),  "120000");
    EXPECT_EQ (tl::to_string (n),  "200");
    EXPECT_EQ (pr.to_string (),  "(0,0;0,600000;400000,600000;400000,400000;600000,400000;600000,0/100000,100000;500000,100000;500000,300000;300000,300000;300000,500000;100000,500000)");
  }

  {
    double rinner = 0.0, router = 0.0;
    unsigned int n;
    db::Polygon pr;
    db::Polygon pp = compute_rounded (p, 50000, 150000, 200);
    db::EdgeProcessor ep;
    std::vector<db::Polygon> in, out;
    in.push_back (pp);
    ep.simple_merge (in, out, true /*insert cut line*/);
    pp = out.front ();
    in.clear ();
    out.clear ();
    in.push_back (pp);
    ep.simple_merge (in, out, false /*no cut line*/);
    pp = out.front ();

    EXPECT_EQ (pp.hull ().size (), size_t (301));
    EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);
    
    EXPECT_EQ (tl::to_string (rinner),  "50000");
    EXPECT_EQ (tl::to_string (router),  "150000");
    EXPECT_EQ (tl::to_string (n),  "200");
    EXPECT_EQ (pr.to_string (),  "(0,0;0,600000;400000,600000;400000,400000;600000,400000;600000,0/100000,100000;500000,100000;500000,300000;300000,300000;300000,500000;100000,500000)");
  }
}

//  rounding
TEST(203) 
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 60000),
    db::Point (40000, 60000),
    db::Point (40000, 40000),
    db::Point (60000, 40000),
    db::Point (60000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 50000),
    db::Point (30000, 50000),
    db::Point (30000, 30000),
    db::Point (50000, 30000),
    db::Point (50000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));
  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  double rinner = 0.0, router = 0.0;
  unsigned int n;
  db::Polygon pr;
  db::Polygon pp = compute_rounded (p, 5000, 15000, 200);
  db::EdgeProcessor ep;
  std::vector<db::Polygon> in, out;
  in.push_back (pp);
  ep.simple_merge (in, out, true /*insert cut line*/);
  pp = out.front ();
  in.clear ();
  out.clear ();
  in.push_back (pp);
  ep.simple_merge (in, out, false /*no cut line*/);
  pp = out.front ();
  pp = smooth (pp, 1, true);

  EXPECT_EQ (pp.hull ().size (), size_t (300));
  EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);
  
  EXPECT_EQ (tl::to_string (rinner),  "5000");
  EXPECT_EQ (tl::to_string (router),  "15000");
  EXPECT_EQ (tl::to_string (n),  "200");
  EXPECT_EQ (pr.to_string (),  "(0,0;0,60000;40000,60000;40000,40000;60000,40000;60000,0/10000,10000;50000,10000;50000,30000;30000,30000;30000,50000;10000,50000)");
}

//  rounding
TEST(204) 
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000),
    db::Point (40000, 40000),
    db::Point (40000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 30000),
    db::Point (30000, 30000),
    db::Point (30000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));
  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  double rinner = 0.0, router = 0.0;
  unsigned int n;
  db::Polygon pr;
  db::Polygon pp = compute_rounded (p, 10000, 20000, 200);
  db::EdgeProcessor ep;
  std::vector<db::Polygon> in, out;
  in.push_back (pp);
  ep.simple_merge (in, out, true /*insert cut line*/);
  pp = out.front ();
  in.clear ();
  out.clear ();
  in.push_back (pp);
  ep.simple_merge (in, out, false /*no cut line*/);
  pp = out.front ();
  pp = smooth (pp, 1, true);

  EXPECT_EQ (pp.hull ().size (), size_t (200));
  EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);
  
  EXPECT_EQ (tl::to_string (rinner),  "10000");
  EXPECT_EQ (tl::to_string (router),  "20000");
  EXPECT_EQ (tl::to_string (n),  "200");
  EXPECT_EQ (pr.to_string (),  "(0,0;0,40000;40000,40000;40000,0/10000,10000;30000,10000;30000,30000;10000,30000)");
}

//  rounding
TEST(205_issue318)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 420000),
    db::Point (400000, 400000),
    db::Point (400000, 0),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  double rinner = 0.0, router = 0.0;
  unsigned int n;
  db::Polygon pr;
  db::Polygon pp = compute_rounded (p, 100000, 200000, 64);
  EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);

  EXPECT_EQ (tl::to_string (rinner),  "0");
  EXPECT_EQ (tl::to_string (router),  "200000");
  EXPECT_EQ (tl::to_string (n),  "64");
  //  slight rounding errors, but still a good approximation ...
  EXPECT_EQ (pr.to_string (),  "(0,0;0,419998;400000,400002;400000,0)");

  pp = compute_rounded (p, 50000, 100000, 64);
  EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);

  EXPECT_EQ (tl::to_string (rinner),  "0");
  EXPECT_EQ (tl::to_string (router),  "100000");
  EXPECT_EQ (tl::to_string (n),  "64");
  //  slight rounding issue due to  ...
  EXPECT_EQ (pr.to_string (),  "(0,0;0,420001;400000,400000;400000,0)");
}

//  rounding
TEST(206_issue318)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000000),
    db::Point (400000, 400000),
    db::Point (400000, 0),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  double rinner = 0.0, router = 0.0;
  unsigned int n;
  db::Polygon pr;
  db::Polygon pp = compute_rounded (p, 100000, 200000, 64);
  EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);

  EXPECT_EQ (tl::to_string (rinner),  "0");
  EXPECT_EQ (tl::to_string (router),  "199992");
  EXPECT_EQ (tl::to_string (n),  "65");
  //  good approximation of a top edge ...
  EXPECT_EQ (pr.to_string (),  "(0,0;0,618467;400000,581242;400000,0)");

  pp = compute_rounded (p, 50000, 100000, 64);
  EXPECT_EQ (extract_rad (pp, rinner, router, n, &pr), true);

  EXPECT_EQ (tl::to_string (rinner),  "0");
  EXPECT_EQ (tl::to_string (router),  "100000");
  EXPECT_EQ (tl::to_string (n),  "64");
  //  the acute corner is split into two parts
  EXPECT_EQ (pr.to_string (),  "(0,0;0,20309228;199083,20290710;400000,400000;400000,0)");
}

//  rounding
TEST(207_issue318)
{
  db::Point pattern [] = {
    db::Point(-2523825, -4693678),
    db::Point(-2627783, -4676814),
    db::Point(-2705532, -4629488),
    db::Point(-2747861, -4559084),
    db::Point(-2750596, -4499543),
    db::Point(-2753284, -4335751),
    db::Point(-2764621, -4271381),
    db::Point(-2828260, -4154562),
    db::Point(-2808940, -4144038),
    db::Point(-2743579, -4264019),
    db::Point(-2731316, -4333649),
    db::Point(-2728604, -4498857),
    db::Point(-2726139, -4552516),
    db::Point(-2689468, -4613512),
    db::Point(-2620017, -4655786),
    db::Point(-2529175, -4670522),
    db::Point(-2468652, -4627768),
    db::Point(-2437469, -4536777),
    db::Point(-2434902, -4384723),
    db::Point(-2436252, -4320529),
    db::Point(-2395450, -4234678),
    db::Point(-2338494, -4144716),
    db::Point(-2319906, -4156484),
    db::Point(-2376150, -4245322),
    db::Point(-2414148, -4325271),
    db::Point(-2412898, -4384677),
    db::Point(-2415531, -4540623),
    db::Point(-2450148, -4641632)
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  double rinner = 0.0, router = 0.0;
  unsigned int n;
  db::Polygon pr;
  //  this polygon should not be recognized as rounded - it kind of looks like ...
  EXPECT_EQ (extract_rad (p, rinner, router, n, &pr), false);
}

//  is_convex
TEST(300)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000),
    db::Point (40000, 40000),
    db::Point (40000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 30000),
    db::Point (30000, 30000),
    db::Point (30000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  EXPECT_EQ (db::is_convex (p), true);
  EXPECT_EQ (db::is_convex (db::polygon_to_simple_polygon (p)), true);

  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  EXPECT_EQ (db::is_convex (p), false);
  EXPECT_EQ (db::is_convex (db::polygon_to_simple_polygon (p)), false);
  EXPECT_EQ (db::is_convex (db::simple_polygon_to_polygon (db::polygon_to_simple_polygon (p))), false);
}

struct TestPolygonSink
  : db::SimplePolygonSink
{
  void put (const db::SimplePolygon &p) {
    if (! s.empty ()) {
      s += "\n";
    }
    s += p.to_string ();
  }
  std::string s;
};

//  decompose_to_convex
TEST(310)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000),
    db::Point (40000, 40000),
    db::Point (40000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 30000),
    db::Point (30000, 30000),
    db::Point (30000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  TestPolygonSink ps;

  db::decompose_convex (p, db::PO_any, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  ps.s.clear ();
  db::decompose_convex (db::polygon_to_simple_polygon (p), db::PO_any, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  ps.s.clear ();
  db::decompose_convex (p, db::PO_any, ps);
  EXPECT_EQ (ps.s,
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(0,0;0,10000;40000,10000;40000,0)"
  );

  ps.s.clear ();
  db::decompose_convex (db::polygon_to_simple_polygon (p), db::PO_any, ps);
  EXPECT_EQ (ps.s,
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(10000,0;10000,10000;40000,10000;40000,0)\n"
    "(0,0;0,30000;10000,30000;10000,0)"
  );

  ps.s.clear ();
  db::decompose_convex (db::simple_polygon_to_polygon (db::polygon_to_simple_polygon (p)), db::PO_any, ps);
  EXPECT_EQ (ps.s,
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(0,0;0,10000;40000,10000;40000,0)"
  );
}

//  decompose_to_convex
TEST(311)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000),
    db::Point (40000, 40000),
    db::Point (40000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 30000),
    db::Point (30000, 30000),
    db::Point (30000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  TestPolygonSink ps;

  db::decompose_convex (p, db::PO_horizontal, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  ps.s.clear ();
  db::decompose_convex (db::polygon_to_simple_polygon (p), db::PO_horizontal, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  ps.s.clear ();
  db::decompose_convex (p, db::PO_horizontal, ps);
  EXPECT_EQ (ps.s,
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(0,30000;0,40000;40000,40000;40000,30000)\n"
    "(30000,10000;30000,30000;40000,30000;40000,10000)\n"
    "(0,0;0,10000;40000,10000;40000,0)"
  );

  ps.s.clear ();
  db::decompose_convex (db::polygon_to_simple_polygon (p), db::PO_horizontal, ps);
  EXPECT_EQ (ps.s,
    "(0,30000;0,40000;40000,40000;40000,30000)\n"
    "(30000,10000;30000,30000;40000,30000;40000,10000)\n"
    "(0,0;0,10000;40000,10000;40000,0)\n"
    "(0,10000;0,30000;10000,30000;10000,10000)"
  );

  ps.s.clear ();
  db::decompose_convex (db::simple_polygon_to_polygon (db::polygon_to_simple_polygon (p)), db::PO_horizontal, ps);
  EXPECT_EQ (ps.s,
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(0,30000;0,40000;40000,40000;40000,30000)\n"
    "(30000,10000;30000,30000;40000,30000;40000,10000)\n"
    "(0,0;0,10000;40000,10000;40000,0)"
  );
}

//  decompose_to_convex
TEST(312)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000),
    db::Point (40000, 40000),
    db::Point (40000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 30000),
    db::Point (30000, 30000),
    db::Point (30000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  TestPolygonSink ps;

  db::decompose_convex (p, db::PO_htrapezoids, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  ps.s.clear ();
  db::decompose_convex (db::polygon_to_simple_polygon (p), db::PO_htrapezoids, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  ps.s.clear ();
  db::decompose_convex (p, db::PO_htrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(0,0;0,10000;40000,10000;40000,0)"
  );

  ps.s.clear ();
  db::decompose_convex (db::polygon_to_simple_polygon (p), db::PO_htrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(10000,0;10000,10000;40000,10000;40000,0)\n"
    "(0,0;0,30000;10000,30000;10000,0)"
  );

  ps.s.clear ();
  db::decompose_convex (db::simple_polygon_to_polygon (db::polygon_to_simple_polygon (p)), db::PO_htrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(0,0;0,10000;40000,10000;40000,0)"
  );
}

//  decompose_to_convex
TEST(313)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000),
    db::Point (40000, 40000),
    db::Point (40000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 30000),
    db::Point (30000, 30000),
    db::Point (30000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  TestPolygonSink ps;

  db::decompose_convex (p, db::PO_vertical, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  ps.s.clear ();
  db::decompose_convex (db::polygon_to_simple_polygon (p), db::PO_vertical, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  ps.s.clear ();
  db::decompose_convex (p, db::PO_vertical, ps);
  EXPECT_EQ (ps.s,
    "(10000,0;10000,10000;30000,10000;30000,0)\n"
    "(0,0;0,40000;10000,40000;10000,0)\n"
    "(10000,30000;10000,40000;30000,40000;30000,30000)\n"
    "(30000,0;30000,40000;40000,40000;40000,0)"
  );

  ps.s.clear ();
  db::decompose_convex (db::polygon_to_simple_polygon (p), db::PO_vertical, ps);
  EXPECT_EQ (ps.s,
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,0;30000,40000;40000,40000;40000,0)\n"
    "(10000,0;10000,10000;30000,10000;30000,0)\n"
    "(0,0;0,30000;10000,30000;10000,0)"
  );

  ps.s.clear ();
  db::decompose_convex (db::simple_polygon_to_polygon (db::polygon_to_simple_polygon (p)), db::PO_vertical, ps);
  EXPECT_EQ (ps.s,
    "(10000,0;10000,10000;30000,10000;30000,0)\n"
    "(0,0;0,40000;10000,40000;10000,0)\n"
    "(10000,30000;10000,40000;30000,40000;30000,30000)\n"
    "(30000,0;30000,40000;40000,40000;40000,0)"
  );
}

//  decompose_to_convex
TEST(314)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000),
    db::Point (40000, 40000),
    db::Point (40000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 30000),
    db::Point (30000, 30000),
    db::Point (30000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  TestPolygonSink ps;

  db::decompose_convex (p, db::PO_vtrapezoids, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  ps.s.clear ();
  db::decompose_convex (db::polygon_to_simple_polygon (p), db::PO_vtrapezoids, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  ps.s.clear ();
  db::decompose_convex (p, db::PO_vtrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(10000,0;10000,10000;30000,10000;30000,0)\n"
    "(0,0;0,30000;10000,30000;10000,0)\n"
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,0;30000,40000;40000,40000;40000,0)"
  );

  ps.s.clear ();
  db::decompose_convex (db::polygon_to_simple_polygon (p), db::PO_vtrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(10000,0;10000,10000;40000,10000;40000,0)\n"
    "(0,0;0,30000;10000,30000;10000,0)"
  );

  ps.s.clear ();
  db::decompose_convex (db::simple_polygon_to_polygon (db::polygon_to_simple_polygon (p)), db::PO_vtrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(10000,0;10000,10000;30000,10000;30000,0)\n"
    "(0,0;0,30000;10000,30000;10000,0)\n"
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,0;30000,40000;40000,40000;40000,0)"
  );
}

//  decompose_to_trapezoids
TEST(320)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000),
    db::Point (40000, 40000),
    db::Point (40000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 30000),
    db::Point (30000, 30000),
    db::Point (30000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  TestPolygonSink ps;

  db::decompose_trapezoids (p, db::TD_simple, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  ps.s.clear ();
  db::decompose_trapezoids (db::polygon_to_simple_polygon (p), db::TD_simple, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  ps.s.clear ();
  db::decompose_trapezoids (p, db::TD_simple, ps);
  EXPECT_EQ (ps.s,
    "(0,0;0,10000;40000,10000;40000,0)\n"
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(30000,10000;30000,30000;40000,30000;40000,10000)\n"
    "(0,30000;0,40000;40000,40000;40000,30000)"
  );

  ps.s.clear ();
  db::decompose_trapezoids (db::polygon_to_simple_polygon (p), db::TD_simple, ps);
  EXPECT_EQ (ps.s,
    "(0,0;0,10000;40000,10000;40000,0)\n"
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(30000,10000;30000,30000;40000,30000;40000,10000)\n"
    "(0,30000;0,40000;40000,40000;40000,30000)"
  );

  ps.s.clear ();
  db::decompose_trapezoids (db::simple_polygon_to_polygon (db::polygon_to_simple_polygon (p)), db::TD_simple, ps);
  EXPECT_EQ (ps.s,
    "(0,0;0,10000;40000,10000;40000,0)\n"
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(30000,10000;30000,30000;40000,30000;40000,10000)\n"
    "(0,30000;0,40000;40000,40000;40000,30000)"
  );
}

//  decompose_to_trapezoids
TEST(321)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000),
    db::Point (40000, 40000),
    db::Point (40000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 30000),
    db::Point (30000, 30000),
    db::Point (30000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  TestPolygonSink ps;

  db::decompose_trapezoids (p, db::TD_htrapezoids, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  ps.s.clear ();
  db::decompose_trapezoids (db::polygon_to_simple_polygon (p), db::TD_htrapezoids, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  ps.s.clear ();
  db::decompose_trapezoids (p, db::TD_htrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(0,0;0,10000;40000,10000;40000,0)"
  );

  ps.s.clear ();
  db::decompose_trapezoids (db::polygon_to_simple_polygon (p), db::TD_htrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(10000,0;10000,10000;40000,10000;40000,0)\n"
    "(0,0;0,30000;10000,30000;10000,0)"
  );

  ps.s.clear ();
  db::decompose_trapezoids (db::simple_polygon_to_polygon (db::polygon_to_simple_polygon (p)), db::TD_htrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(0,10000;0,30000;10000,30000;10000,10000)\n"
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(0,0;0,10000;40000,10000;40000,0)"
  );
}

//  decompose_to_trapezoids
TEST(322)
{
  db::Point pattern [] = {
    db::Point (0, 0),
    db::Point (0, 40000),
    db::Point (40000, 40000),
    db::Point (40000, 0),
  };

  db::Point hole [] = {
    db::Point (10000, 10000),
    db::Point (10000, 30000),
    db::Point (30000, 30000),
    db::Point (30000, 10000),
  };

  db::Polygon p;
  p.assign_hull (&pattern[0], &pattern[0] + sizeof (pattern) / sizeof (pattern[0]));

  TestPolygonSink ps;

  db::decompose_trapezoids (p, db::TD_vtrapezoids, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  ps.s.clear ();
  db::decompose_trapezoids (db::polygon_to_simple_polygon (p), db::TD_vtrapezoids, ps);
  EXPECT_EQ (ps.s, "(0,0;0,40000;40000,40000;40000,0)");

  p.insert_hole (&hole[0], &hole[0] + sizeof (hole) / sizeof (hole[0]));

  ps.s.clear ();
  db::decompose_trapezoids (p, db::TD_vtrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(10000,0;10000,10000;30000,10000;30000,0)\n"
    "(0,0;0,30000;10000,30000;10000,0)\n"
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,0;30000,40000;40000,40000;40000,0)"
  );

  ps.s.clear ();
  db::decompose_trapezoids (db::polygon_to_simple_polygon (p), db::TD_vtrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,10000;30000,40000;40000,40000;40000,10000)\n"
    "(10000,0;10000,10000;40000,10000;40000,0)\n"
    "(0,0;0,30000;10000,30000;10000,0)"
  );

  ps.s.clear ();
  db::decompose_trapezoids (db::simple_polygon_to_polygon (db::polygon_to_simple_polygon (p)), db::TD_vtrapezoids, ps);
  EXPECT_EQ (ps.s,
    "(10000,0;10000,10000;30000,10000;30000,0)\n"
    "(0,0;0,30000;10000,30000;10000,0)\n"
    "(0,30000;0,40000;30000,40000;30000,30000)\n"
    "(30000,0;30000,40000;40000,40000;40000,0)"
  );
}

//  cut self-overlapping polygon
TEST(400)
{
  std::vector <db::Point> c;
  c.push_back (db::Point (0, 0));
  c.push_back (db::Point (0, 100));
  c.push_back (db::Point (1000, 100));
  c.push_back (db::Point (1000, 1000));
  c.push_back (db::Point (0, 1000));
  c.push_back (db::Point (0, 900));
  c.push_back (db::Point (900, 900));
  c.push_back (db::Point (900, 0));

  {
    db::Polygon in;
    in.assign_hull (c.begin (), c.end ());
    std::vector<db::Polygon> right_of;

    db::cut_polygon (in, db::Edge (db::Point (500, 0), db::Point (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (2));
    EXPECT_EQ (right_of[0].to_string (), "(500,0;500,100;900,100;900,0)");
    EXPECT_EQ (right_of[1].to_string (), "(900,100;900,900;500,900;500,1000;1000,1000;1000,100)");

    right_of.clear ();
    db::cut_polygon (in, db::Edge (db::Point (500, 1), db::Point (500, 0)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (2));
    EXPECT_EQ (right_of[0].to_string (), "(0,0;0,100;500,100;500,0)");
    EXPECT_EQ (right_of[1].to_string (), "(0,900;0,1000;500,1000;500,900)");
  }

  {
    db::SimplePolygon in;
    in.assign_hull (c.begin (), c.end ());
    std::vector<db::SimplePolygon> right_of;

    db::cut_polygon (in, db::Edge (db::Point (500, 0), db::Point (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (2));
    EXPECT_EQ (right_of[0].to_string (), "(500,0;500,100;900,100;900,0)");
    EXPECT_EQ (right_of[1].to_string (), "(900,100;900,900;500,900;500,1000;1000,1000;1000,100)");

    right_of.clear ();
    db::cut_polygon (in, db::Edge (db::Point (500, 1), db::Point (500, 0)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (2));
    EXPECT_EQ (right_of[0].to_string (), "(0,0;0,100;500,100;500,0)");
    EXPECT_EQ (right_of[1].to_string (), "(0,900;0,1000;500,1000;500,900)");
  }
}

//  cut self-overlapping polygon (with double types)
TEST(401)
{
  std::vector <db::DPoint> c;
  c.push_back (db::DPoint (0, 0));
  c.push_back (db::DPoint (0, 100));
  c.push_back (db::DPoint (1000, 100));
  c.push_back (db::DPoint (1000, 1000));
  c.push_back (db::DPoint (0, 1000));
  c.push_back (db::DPoint (0, 900));
  c.push_back (db::DPoint (900, 900));
  c.push_back (db::DPoint (900, 0));

  {
    db::DPolygon in;
    in.assign_hull (c.begin (), c.end ());
    std::vector<db::DPolygon> right_of;

    db::cut_polygon (in, db::DEdge (db::DPoint (500, 0), db::DPoint (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (2));
    EXPECT_EQ (right_of[0].to_string (), "(500,0;500,100;900,100;900,0)");
    EXPECT_EQ (right_of[1].to_string (), "(900,100;900,900;500,900;500,1000;1000,1000;1000,100)");

    right_of.clear ();
    db::cut_polygon (in, db::DEdge (db::DPoint (500, 1), db::DPoint (500, 0)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (2));
    EXPECT_EQ (right_of[0].to_string (), "(0,0;0,100;500,100;500,0)");
    EXPECT_EQ (right_of[1].to_string (), "(0,900;0,1000;500,1000;500,900)");
  }

  {
    db::DSimplePolygon in;
    in.assign_hull (c.begin (), c.end ());
    std::vector<db::DSimplePolygon> right_of;

    db::cut_polygon (in, db::DEdge (db::DPoint (500, 0), db::DPoint (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (2));
    EXPECT_EQ (right_of[0].to_string (), "(500,0;500,100;900,100;900,0)");
    EXPECT_EQ (right_of[1].to_string (), "(900,100;900,900;500,900;500,1000;1000,1000;1000,100)");

    right_of.clear ();
    db::cut_polygon (in, db::DEdge (db::DPoint (500, 1), db::DPoint (500, 0)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (2));
    EXPECT_EQ (right_of[0].to_string (), "(0,0;0,100;500,100;500,0)");
    EXPECT_EQ (right_of[1].to_string (), "(0,900;0,1000;500,1000;500,900)");
  }
}

//  cut empty polygons
TEST(402)
{
  {
    db::Polygon in;
    std::vector<db::Polygon> right_of;
    db::cut_polygon (in, db::Edge (db::Point (500, 0), db::Point (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (0));
  }
  {
    db::SimplePolygon in;
    std::vector<db::SimplePolygon> right_of;
    db::cut_polygon (in, db::Edge (db::Point (500, 0), db::Point (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (0));
  }
  {
    db::DPolygon in;
    std::vector<db::DPolygon> right_of;
    db::cut_polygon (in, db::DEdge (db::DPoint (500, 0), db::DPoint (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (0));
  }
  {
    db::DSimplePolygon in;
    std::vector<db::DSimplePolygon> right_of;
    db::cut_polygon (in, db::DEdge (db::DPoint (500, 0), db::DPoint (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (0));
  }
}

//  cut point-like polygons
TEST(403)
{
  {
    db::Polygon in (db::Box (1000, 0, 1000, 0));
    std::vector<db::Polygon> right_of;
    db::cut_polygon (in, db::Edge (db::Point (500, 0), db::Point (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (1));
    EXPECT_EQ (right_of[0].to_string (), "()");  // bad, but no contour available :-(
    right_of.clear ();
    db::cut_polygon (in, db::Edge (db::Point (500, 1), db::Point (500, 0)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (0));
  }

  {
    db::SimplePolygon in (db::Box (1000, 0, 1000, 0));
    std::vector<db::SimplePolygon> right_of;
    db::cut_polygon (in, db::Edge (db::Point (500, 0), db::Point (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (1));
    EXPECT_EQ (right_of[0].to_string (), "()");  // bad, but no contour available :-(
    right_of.clear ();
    db::cut_polygon (in, db::Edge (db::Point (500, 1), db::Point (500, 0)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (0));
  }

  {
    db::DPolygon in (db::DBox (1000, 0, 1000, 0));
    std::vector<db::DPolygon> right_of;
    db::cut_polygon (in, db::DEdge (db::DPoint (500, 0), db::DPoint (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (1));
    EXPECT_EQ (right_of[0].to_string (), "()");  // bad, but no contour available :-(
    right_of.clear ();
    db::cut_polygon (in, db::DEdge (db::DPoint (500, 1), db::DPoint (500, 0)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (0));
  }

  {
    db::DSimplePolygon in (db::DBox (1000, 0, 1000, 0));
    std::vector<db::DSimplePolygon> right_of;
    db::cut_polygon (in, db::DEdge (db::DPoint (500, 0), db::DPoint (500, 1)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (1));
    EXPECT_EQ (right_of[0].to_string (), "()");  // bad, but no contour available :-(
    right_of.clear ();
    db::cut_polygon (in, db::DEdge (db::DPoint (500, 1), db::DPoint (500, 0)), std::back_inserter (right_of));
    EXPECT_EQ (right_of.size (), size_t (0));
  }
}

//  issue 166
TEST(404)
{
  db::Polygon poly;
  std::string s ("(390,0;438,936;176,874;0,832;438,937;541,961;821,102)");
  tl::Extractor ex (s.c_str ());
  ex.read (poly);

  std::vector<db::Polygon> sp;
  db::split_polygon (poly, sp);
  EXPECT_EQ (sp.size (), size_t (2));
  if (sp.size () >= 2) {
    EXPECT_EQ (sp[0].to_string (), "(390,0;438,936;390,925;438,937;541,961;821,102)");
    EXPECT_EQ (sp[1].to_string (), "(0,832;176,874;390,925)");
  }
}

TEST(405)
{
  db::Polygon poly;
  std::string s ("(0,0;0,1126;30,1126;30,30;3044,30;3044,1126;5782,1126;5782,30;8796,30;8796,1126;0,1126;0,1141;3009,1141;3009,1156;3194,1156;3194,1141;8826,1141;8826,0;5742,0;5742,1126;3084,1126;3084,0)");
  tl::Extractor ex (s.c_str ());
  ex.read (poly);

  std::vector<db::Polygon> sp;
  db::split_polygon (poly, sp);

  EXPECT_EQ (sp.size (), size_t (2));
  if (sp.size () >= 2) {
    EXPECT_EQ (sp[0].to_string (), "(5742,0;5742,1126;5782,1126;5782,30;8796,30;8796,1126;3194,1126;3194,1141;8826,1141;8826,0)");
    EXPECT_EQ (sp[1].to_string (), "(0,0;0,1126;30,1126;30,30;3044,30;3044,1126;0,1126;0,1141;3009,1141;3009,1156;3194,1156;3194,1126;3084,1126;3084,0)");
  }
}

static db::Polygon str2poly (const std::string &s)
{
  db::Polygon poly;
  tl::Extractor ex (s.c_str ());
  ex.read (poly);
  return poly;
}

//  self-overlapping, non-orientable check
TEST(500)
{
  std::string ps;
  std::vector<db::Polygon> parts;

  //  null polygon
  ps = "()";
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps)), false);
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps)), false);

  //  triangle
  ps = "(0,0;1000,0;1000,1000)";
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps)), false);
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps)), false);

  //  rectangle counter-clockwise
  ps = "(0,0;1000,0;1000,1000;0,1000)";
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps)), false);
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps)), false);

  //  rectangle clockwise
  ps = "(0,0;0,1000;1000,1000;1000,0)";
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps)), false);
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps)), false);

  //  "8" shape
  ps = "(0,0;1000,1000;0,1000;1000,0)";
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps)), true);
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps)), true);

  parts.clear ();
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps), &parts), true);
  EXPECT_EQ (parts.size (), size_t (1));
  if (! parts.empty ()) {
    EXPECT_EQ (parts[0].to_string (), "(0,0;500,500;1000,0)");
  }

  parts.clear ();
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps), &parts), true);
  EXPECT_EQ (parts.size (), size_t (1));
  if (! parts.empty ()) {
    EXPECT_EQ (parts[0].to_string (), "(0,0;500,500;1000,0)");
  }

  //  self-touching
  ps = "(0,0;0,2000;1000,2000;1000,1000;3000,1000;3000,3000;1000,3000;1000,2000;0,2000;0,4000;4000,4000;4000,0)";
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps)), false);
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps)), false);

  //  self-overlap
  ps = "(0,0;0,2500;1000,2500;1000,1000;3000,1000;3000,3000;1000,3000;1000,2000;0,2000;0,4000;4000,4000;4000,0)";
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps)), true);
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps)), false);

  parts.clear ();
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps), &parts), true);
  EXPECT_EQ (parts.size (), size_t (1));
  if (! parts.empty ()) {
    EXPECT_EQ (parts[0].to_string (), "(0,2000;0,2500;1000,2500;1000,2000)");
  }

  //  inner loop twisted
  ps = "(0,0;0,2000;1000,2000;1000,3000;3000,3000;3000,1000;1000,1000;1000,2000;0,2000;0,4000;4000,4000;4000,0)";
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps)), true);
  //  This is a double loop, so it's orientable
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps)), false);

  //  non-orientable hole
  ps = "(0,0;0,4000;4000,4000;4000,0/1000,1000;3000,3000;1000,3000;3000,1000)";
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps)), true);

  //  NOTE: a non-orientable holes does not generate -1 wrapcount, but just 0. So the polygon is "orientable"
  //  as a whole. Which isn't good for detecting invalid input polygons, but as those are hull-only for GDS and
  //  OASIS and most other formats (except DXF), we don't care too much here:
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps)), false);

  //  hole outside hull
  ps = "(0,0;0,4000;4000,4000;4000,0/1000,1000;5000,1000;5000,3000;1000,3000)";
  EXPECT_EQ (db::is_strange_polygon (str2poly (ps)), true);
  EXPECT_EQ (db::is_non_orientable_polygon (str2poly (ps)), true);
}

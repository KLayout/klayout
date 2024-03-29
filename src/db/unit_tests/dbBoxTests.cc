
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


#include "dbBox.h"
#include "tlUnitTest.h"


TEST(1) 
{
  db::Box b (0, 0, 100, 200);
  db::Box empty;

  EXPECT_EQ (empty.empty (), true);
  EXPECT_EQ (b.empty (), false);
}

TEST(2) 
{
  db::Box b (0, 0, 100, 200);
  db::Box empty;

  EXPECT_EQ (b.moved (db::Vector (10, 20)), db::Box (110, 20, 10, 220));
  EXPECT_EQ (b.enlarged (db::Vector (10, 20)), db::Box (-10, -20, 110, 220));
  EXPECT_EQ (empty.moved (db::Vector (10, 20)).empty (), true);
  EXPECT_EQ (empty.enlarged (db::Vector (10, 20)).empty (), true);
  EXPECT_EQ (b + db::Box (-10, 20, 100, 200), db::Box (-10, 0, 100, 200));
  EXPECT_EQ (b + db::Box (-10, -20, 100, -10), db::Box (-10, -20, 100, 200));
  EXPECT_EQ (b + db::Box (110, 220, 120, 250), db::Box (0, 0, 120, 250));
  EXPECT_EQ (b & db::Box (110, 220, 120, 250), empty);
  EXPECT_EQ (b & db::Box (50, 100, 120, 250), db::Box (50, 100, 100, 200));
  EXPECT_EQ (b & db::Box (50, 100, 60, 120), db::Box (50, 100, 60, 120));
  EXPECT_EQ (b - b, db::Box ());
  EXPECT_EQ (b - db::Box (), b);
  EXPECT_EQ (db::Box () - b, db::Box ());
  EXPECT_EQ (db::Box () - db::Box (), db::Box ());
  EXPECT_EQ (b - db::Box (0, 0, 50, 50), b);
  EXPECT_EQ (b - db::Box (0, 0, 50, 200), db::Box (50, 0, 100, 200));
  EXPECT_EQ (b - db::Box (50, 0, 100, 200), db::Box (0, 0, 50, 200));
  EXPECT_EQ (b - db::Box (0, 0, 100, 100), db::Box (0, 100, 100, 200));
  EXPECT_EQ (b - db::Box (0, 100, 100, 200), db::Box (0, 0, 100, 100));
  EXPECT_EQ (db::Box::world () - b, db::Box::world ());
  EXPECT_EQ (b - db::Box::world (), db::Box ());

  empty.move (db::Vector (10, 20));
  EXPECT_EQ (empty == db::Box (), true);
  empty.enlarge (db::Vector (10, 20));
  EXPECT_EQ (empty == db::Box (), true);
}

TEST(3) 
{
  db::Box b (0, 0, 100, 200);
  db::Box empty;

  db::Trans t (2, true, db::Vector (10, 20));
  EXPECT_EQ (t * b, db::Box (-90, 20, 10, 220));
  EXPECT_EQ (t * empty, empty);
}

TEST(4) 
{
  db::Box b (0, 0, 100, 200);
  db::Box empty;

  EXPECT_EQ (b.inside (empty), false);
  EXPECT_EQ (empty.inside (b), false);
  EXPECT_EQ (b.enlarged (db::Vector (-10, -10)).inside (b), true);
  EXPECT_EQ (b.enlarged (db::Vector (10, 10)).inside (b), false);
  EXPECT_EQ (b.moved (db::Vector (10, 10)).inside (b), false);
  EXPECT_EQ (b.overlaps (b.moved (db::Vector (10, 10))), true);
  EXPECT_EQ (b.overlaps (b.moved (db::Vector (110, 110))), false);
  EXPECT_EQ (b.overlaps (b.moved (db::Vector (100, 100))), false);
  EXPECT_EQ (b.touches (b.moved (db::Vector (110, 110))), false);
  EXPECT_EQ (b.touches (b.moved (db::Vector (100, 100))), true);
  EXPECT_EQ (b.touches (b.moved (db::Vector (10, 10))), true);
}

TEST(5)
{
  db::Box b (10, 10, 110, 210);
  db::Box empty;

  EXPECT_EQ (b.area (), 100.0 * 200.0);
  EXPECT_EQ (b.perimeter (), db::Box::perimeter_type (600));
  EXPECT_EQ (b.to_string (), "(10,10;110,210)");
  EXPECT_EQ (b.width (), db::Box::distance_type (100));
  EXPECT_EQ (b.height (), db::Box::distance_type (200));
  EXPECT_EQ (b.top (), 210);
  EXPECT_EQ (b.left (), 10);
  EXPECT_EQ (b.right (), 110);
  EXPECT_EQ (b.bottom (), 10);
  EXPECT_EQ (db::Box (db::Point (110, 10), db::Point (10, 210)).p1 (), db::Point (10, 10));
  EXPECT_EQ (db::Box (db::Point (110, 210), db::Point (10, 10)).p2 (), db::Point (110, 210));
}

TEST(6)
{
  db::Box b (10, 10, 110, 210);
  EXPECT_EQ (b.contains (db::Point (50, 50)), true);
  EXPECT_EQ (b.contains (db::Point (10, 50)), true);
  EXPECT_EQ (b.contains (db::Point (5, 50)), false);
  EXPECT_EQ (b.contains (db::Point (110, 50)), true);
  EXPECT_EQ (b.contains (db::Point (115, 50)), false);
  EXPECT_EQ (b.contains (db::Point (10, 10)), true);
  EXPECT_EQ (b.contains (db::Point (5, 5)), false);
}

TEST(7)
{
  db::Box b;
  db::Box bc (0, 1, 2, 3);

  std::string s (b.to_string ());
  EXPECT_EQ (s, "()");
  tl::Extractor ex (s.c_str ());
  ex.read (bc);

  EXPECT_EQ (bc.to_string (), s);
}

TEST(8)
{
  db::Box b (10, 20, 45, 60);
  db::Box bc (0, 1, 2, 3);

  std::string s (b.to_string ());
  EXPECT_EQ (s, "(10,20;45,60)");
  tl::Extractor ex (s.c_str ());
  ex.read (bc);

  EXPECT_EQ (bc.to_string (), s);
}

TEST(9)
{
  db::DBox b;
  db::DBox bc (0, 1, 2, 3);

  std::string s (b.to_string ());
  EXPECT_EQ (s, "()");
  tl::Extractor ex (s.c_str ());
  ex.read (bc);

  EXPECT_EQ (bc.to_string (), s);
}

TEST(10)
{
  db::Box b (10, 20, 45, 60);
  db::Box bc (0, 1, 2, 3);

  std::string s (b.to_string ());
  EXPECT_EQ (s, "(10,20;45,60)");
  tl::Extractor ex (s.c_str ());
  ex.read (bc);

  EXPECT_EQ (bc.to_string (), s);
}

TEST(11)
{
  db::Box b;

  b.set_left (10);
  EXPECT_EQ (b.to_string (), "(10,0;10,0)");
  b.set_left (5);
  EXPECT_EQ (b.to_string (), "(5,0;10,0)");
  b.set_left (15);
  EXPECT_EQ (b.to_string (), "(15,0;15,0)");

  b = db::Box ();
  b.set_right (10);
  EXPECT_EQ (b.to_string (), "(10,0;10,0)");
  b.set_right (15);
  EXPECT_EQ (b.to_string (), "(10,0;15,0)");
  b.set_right (5);
  EXPECT_EQ (b.to_string (), "(5,0;5,0)");

  b = db::Box ();
  b.set_bottom (10);
  EXPECT_EQ (b.to_string (), "(0,10;0,10)");
  b.set_bottom (5);
  EXPECT_EQ (b.to_string (), "(0,5;0,10)");
  b.set_bottom (15);
  EXPECT_EQ (b.to_string (), "(0,15;0,15)");

  b = db::Box ();
  b.set_top (10);
  EXPECT_EQ (b.to_string (), "(0,10;0,10)");
  b.set_top (15);
  EXPECT_EQ (b.to_string (), "(0,10;0,15)");
  b.set_top (5);
  EXPECT_EQ (b.to_string (), "(0,5;0,5)");
}

TEST(12)
{
  db::DBox b;

  b.set_left (10);
  EXPECT_EQ (b.to_string (), "(10,0;10,0)");
  b.set_left (5);
  EXPECT_EQ (b.to_string (), "(5,0;10,0)");
  b.set_left (15);
  EXPECT_EQ (b.to_string (), "(15,0;15,0)");

  b = db::DBox ();
  b.set_right (10);
  EXPECT_EQ (b.to_string (), "(10,0;10,0)");
  b.set_right (15);
  EXPECT_EQ (b.to_string (), "(10,0;15,0)");
  b.set_right (5);
  EXPECT_EQ (b.to_string (), "(5,0;5,0)");

  b = db::DBox ();
  b.set_bottom (10);
  EXPECT_EQ (b.to_string (), "(0,10;0,10)");
  b.set_bottom (5);
  EXPECT_EQ (b.to_string (), "(0,5;0,10)");
  b.set_bottom (15);
  EXPECT_EQ (b.to_string (), "(0,15;0,15)");

  b = db::DBox ();
  b.set_top (10);
  EXPECT_EQ (b.to_string (), "(0,10;0,10)");
  b.set_top (15);
  EXPECT_EQ (b.to_string (), "(0,10;0,15)");
  b.set_top (5);
  EXPECT_EQ (b.to_string (), "(0,5;0,5)");
}

TEST(13)
{
  //  32bit overflow for perimeter
  db::Box b (-1000000000, -1000000000, 1000000000, 1000000000);
  EXPECT_EQ (b.perimeter (), 8000000000.0);
}

TEST(14)
{
  //  world, specifically with 64bit coordinates and
  //  transfer via double coordinates

  db::Box b = db::Box::world ();

  EXPECT_EQ (b == db::Box::world (), true);
  db::ICplxTrans t;
  EXPECT_EQ (t * b == db::Box::world (), true);
  EXPECT_EQ (t.inverted () * b == db::Box::world (), true);
}

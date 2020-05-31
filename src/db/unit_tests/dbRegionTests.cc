
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


#include "tlUnitTest.h"

#include "dbRegion.h"
#include "dbRegionUtils.h"
#include "dbRegionProcessors.h"
#include "dbEdgesUtils.h"
#include "dbBoxScanner.h"
#include "dbReader.h"
#include "dbTestSupport.h"

#include "tlStream.h"

#include <cstdio>

TEST(1) 
{
  db::Region r;
  EXPECT_EQ (r.to_string (), "");
  EXPECT_EQ (r == db::Region (), true);
  EXPECT_EQ (r < db::Region (), false);
  EXPECT_EQ (r != db::Region (), false);
  EXPECT_EQ (r.bbox ().to_string (), "()");
  EXPECT_EQ (r.empty (), true);
  EXPECT_EQ (r.is_box (), false);
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.begin ().at_end (), true);

  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  EXPECT_EQ (r == db::Region (), false);
  EXPECT_EQ (r < db::Region (), true);
  EXPECT_EQ (r != db::Region (), true);
  EXPECT_EQ (r != r, false);
  EXPECT_EQ (r == r, true);
  EXPECT_EQ (r < r, false);
  EXPECT_EQ (r.to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.transformed (db::Trans (db::Vector (1, 2))).to_string (), "(1,2;1,202;101,202;101,2)");
  EXPECT_EQ (r.bbox ().to_string (), "(0,0;100,200)");
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.is_box (), true);
  EXPECT_EQ (r.begin ().at_end (), false);

  db::Region rr = r;
  rr.insert (db::Box (db::Point (10, 10), db::Point (110, 30)));
  EXPECT_EQ (rr.bbox ().to_string (), "(0,0;110,200)");
  EXPECT_EQ (rr.to_string (), "(0,0;0,200;100,200;100,0);(10,10;10,30;110,30;110,10)");
  EXPECT_EQ (rr.empty (), false);
  EXPECT_EQ (rr.is_merged (), false);
  EXPECT_EQ (rr.is_box (), false);
  EXPECT_EQ (rr.begin ().at_end (), false);

  db::Region r1 = r;
  db::Region r2;
  EXPECT_EQ (r1.to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r2.to_string (), "");
  EXPECT_EQ (r1.bbox ().to_string (), "(0,0;100,200)");
  EXPECT_EQ (r2.bbox ().to_string (), "()");
  r1.swap (r2);
  EXPECT_EQ (r1.to_string (), "");
  EXPECT_EQ (r2.to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r1.bbox ().to_string (), "()");
  EXPECT_EQ (r2.bbox ().to_string (), "(0,0;100,200)");

  EXPECT_EQ ((r + db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).to_string (), "(0,0;0,200;100,200;100,0);(10,20;10,220;110,220;110,20)");
  EXPECT_EQ ((r + db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).merged ().to_string (), "(0,0;0,200;10,200;10,220;110,220;110,20;100,20;100,0)");
  EXPECT_EQ ((r + db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).merged (false, 1).to_string (), "(10,20;10,200;100,200;100,20)");
  EXPECT_EQ ((r | db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).to_string (), "(0,0;0,200;10,200;10,220;110,220;110,20;100,20;100,0)");

  r += db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)));
  EXPECT_EQ (r.is_box (), false);
  EXPECT_EQ (r.to_string (), "(0,0;0,200;100,200;100,0);(10,20;10,220;110,220;110,20)");
  EXPECT_EQ (r.is_merged (), false);
  EXPECT_EQ (r.size (), size_t (2));
  r.set_merged_semantics (false);
  EXPECT_EQ (r.area (), 40000);
  EXPECT_EQ (r.area (db::Box (db::Point (-10, -10), db::Point (50, 50))), 50 * 50 + 40 * 30);
  EXPECT_EQ (r.perimeter (), db::Region::perimeter_type (1200));
  EXPECT_EQ (r.perimeter (db::Box (db::Point (-10, -10), db::Point (50, 50))), db::Region::perimeter_type (170));
  EXPECT_EQ (r.perimeter (db::Box (db::Point (-10, -10), db::Point (0, 50))), db::Region::perimeter_type (0));
  EXPECT_EQ (r.perimeter (db::Box (db::Point (0, 0), db::Point (50, 50))), db::Region::perimeter_type (170));
  EXPECT_EQ (r.perimeter (db::Box (db::Point (10, 20), db::Point (50, 50))), db::Region::perimeter_type (70));
  EXPECT_EQ (r.perimeter (db::Box (db::Point (90, 200), db::Point (110, 220))), db::Region::perimeter_type (40));
  db::Coord ptot = 0;
  for (db::Coord x = 0; x < 110; x += 10) {
    for (db::Coord y = 0; y < 220; y += 10) {
      ptot += r.perimeter (db::Box (db::Point (x, y), db::Point (x + 10, y + 10)));
    }
  }
  EXPECT_EQ (ptot, 1200);
  r.set_merged_semantics (true);
  EXPECT_EQ (r.area (), 23800);
  EXPECT_EQ (r.area (db::Box (db::Point (-10, -10), db::Point (50, 50))), 50 * 50);
  EXPECT_EQ (r.perimeter (), db::Region::perimeter_type (660));
  EXPECT_EQ (r.perimeter (db::Box (db::Point (-10, -10), db::Point (50, 50))), db::Region::perimeter_type (100));
  EXPECT_EQ (r.perimeter (db::Box (db::Point (-10, -10), db::Point (0, 50))), db::Region::perimeter_type (0));
  EXPECT_EQ (r.perimeter (db::Box (db::Point (0, 0), db::Point (50, 50))), db::Region::perimeter_type (100));
  r.merge ();
  EXPECT_EQ (r.to_string (), "(0,0;0,200;10,200;10,220;110,220;110,20;100,20;100,0)");
  EXPECT_EQ (r.bbox ().to_string (), "(0,0;110,220)");
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.is_box (), false);
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.size (), size_t (1));
  EXPECT_EQ (r.area (), 23800);
  EXPECT_EQ (r.perimeter (), db::Region::perimeter_type (660));

  r.clear ();
  EXPECT_EQ (r.empty (), true);
  EXPECT_EQ (r.is_merged (), true);

  r.insert (db::Box (db::Point (0, 0), db::Point (50, 50)));
  r.insert (db::Box (db::Point (50, 50), db::Point (100, 100)));
  EXPECT_EQ (r.merged (false).to_string (), "(0,0;0,50;50,50;50,100;100,100;100,50;50,50;50,0)");
  EXPECT_EQ (r.merged (true).to_string (), "(0,0;0,50;50,50;50,0);(50,50;50,100;100,100;100,50)");

  r.set_merged_semantics (false);
  EXPECT_EQ (r.sized (10).to_string (), "(-10,-10;-10,60;60,60;60,-10);(40,40;40,110;110,110;110,40)");
  EXPECT_EQ (r.sized (db::Coord (10), db::Coord (20)).to_string (), "(-10,-20;-10,70;60,70;60,-20);(40,30;40,120;110,120;110,30)");
  EXPECT_EQ (r.sized (10, 20, 0).to_string (), "(0,-20;-10,0;-10,50;0,70;50,70;60,50;60,0;50,-20);(50,30;40,50;40,100;50,120;100,120;110,100;110,50;100,30)");
  r.size (10, 20, 2);
  EXPECT_EQ (r.to_string (), "(-10,-20;-10,70;60,70;60,-20);(40,30;40,120;110,120;110,30)");
}

TEST(1b)
{
  //  special perimeter bug
  db::Region r;
  r.insert (db::Box (db::Point (52200, 20000), db::Point (55200, 21000)));
  r.set_merged_semantics (false);
  EXPECT_EQ (r.perimeter (db::Box (db::Point (51100, 20000), db::Point (55200, 21000))), db::Region::perimeter_type (8000));
  EXPECT_EQ (r.perimeter (db::Box (db::Point (55200, 20000), db::Point (59300, 21000))), db::Region::perimeter_type (0));
}

TEST(2) 
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  EXPECT_EQ (r.is_box (), true);

  EXPECT_EQ ((r & db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).to_string (), "(10,20;10,200;100,200;100,20)");

  r &= db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)));
  EXPECT_EQ (r.is_box (), true);
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.to_string (), "(10,20;10,200;100,200;100,20)");

  r.insert (db::Box (db::Point (-50, -50), db::Point (50, 50)));
  EXPECT_EQ (r.is_box (), false);
  EXPECT_EQ (r.is_merged (), false);

  EXPECT_EQ ((r & db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).to_string (), "(10,20;10,200;100,200;100,20);(10,20;10,50;50,50;50,20)");
  EXPECT_EQ ((db::Region (db::Box (db::Point (10, 20), db::Point (110, 220))) & r).to_string (), "(10,20;10,200;100,200;100,20);(10,20;10,50;50,50;50,20)");

  r &= db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)));
  EXPECT_EQ (r.is_box (), false);
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.is_merged (), false);
  EXPECT_EQ (r.to_string (), "(10,20;10,200;100,200;100,20);(10,20;10,50;50,50;50,20)");
}

TEST(3) 
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (200, 400)));
  EXPECT_EQ (r.is_box (), false);

  db::Region rr;
  rr.insert (db::Box (db::Point (10, 20), db::Point (110, 220)));
  //  force non-box to enable scanline algorithm
  rr.insert (db::Box (db::Point (10, 20), db::Point (110, 220)));
  EXPECT_EQ (rr.is_box (), false);

  EXPECT_EQ ((r & rr).to_string (), "(10,20;10,220;110,220;110,20)");
  EXPECT_EQ ((rr & r).to_string (), "(10,20;10,220;110,220;110,20)");
  EXPECT_EQ ((r & db::Region ()).to_string (), "");
  EXPECT_EQ ((r & db::Region ()).empty (), true);
  EXPECT_EQ ((db::Region () & r).to_string (), "");
  EXPECT_EQ ((db::Region () & r).empty (), true);

  r &= rr;
  EXPECT_EQ (r.is_box (), true);
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.to_string (), "(10,20;10,220;110,220;110,20)");
}

TEST(4) 
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  EXPECT_EQ (r.is_box (), true);

  r.set_min_coherence (false);
  EXPECT_EQ ((r ^ db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).to_string (), "(0,0;0,200;10,200;10,220;110,220;110,20;100,20;100,0/10,20;100,20;100,200;10,200)");

  r.set_min_coherence (true);
  EXPECT_EQ ((r ^ db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).to_string (), "(0,0;0,200;10,200;10,20;100,20;100,0);(100,20;100,200;10,200;10,220;110,220;110,20)");

  r ^= db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)));
  EXPECT_EQ (r.is_box (), false);
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.to_string (), "(0,0;0,200;10,200;10,20;100,20;100,0);(100,20;100,200;10,200;10,220;110,220;110,20)");
}

TEST(5) 
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (200, 400)));
  EXPECT_EQ (r.is_box (), false);

  EXPECT_EQ ((r ^ db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).to_string (), "(-100,-100;-100,400;200,400;200,-100/10,20;110,20;110,220;10,220)");
  EXPECT_EQ ((r ^ db::Region ()).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,400;200,400;200,-100)");
  EXPECT_EQ ((r ^ db::Region ()).empty (), false);
  EXPECT_EQ ((r ^ db::Region ()).is_merged (), false);
  EXPECT_EQ ((db::Region () ^ r).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,400;200,400;200,-100)");
  EXPECT_EQ ((db::Region () ^ r).empty (), false);
  EXPECT_EQ ((db::Region () ^ r).is_merged (), false);

  r ^= db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)));
  EXPECT_EQ (r.is_box (), false);
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,400;200,400;200,-100/10,20;110,20;110,220;10,220)");

  db::Region rr;
  std::string s = r.to_string ();
  tl::Extractor ex (s.c_str ());
  EXPECT_EQ (ex.try_read (rr), true);
  EXPECT_EQ (rr.to_string (), "(-100,-100;-100,400;200,400;200,-100/10,20;110,20;110,220;10,220)");

  EXPECT_EQ (r.holes ().to_string (), "(10,20;10,220;110,220;110,20)");
  EXPECT_EQ (r.hulls ().to_string (), "(-100,-100;-100,400;200,400;200,-100)");
}

TEST(6) 
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  EXPECT_EQ (r.is_box (), true);

  EXPECT_EQ ((r - db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).to_string (), "(0,0;0,200;10,200;10,20;100,20;100,0)");

  r -= db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)));
  EXPECT_EQ (r.is_box (), false);
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.to_string (), "(0,0;0,200;10,200;10,20;100,20;100,0)");
}

TEST(7) 
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (200, 400)));
  EXPECT_EQ (r.is_box (), false);

  EXPECT_EQ ((r - db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)))).to_string (), "(-100,-100;-100,400;200,400;200,-100/10,20;110,20;110,220;10,220)");
  EXPECT_EQ ((r - db::Region ()).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,400;200,400;200,-100)");
  EXPECT_EQ ((r - db::Region ()).empty (), false);
  EXPECT_EQ ((r - db::Region ()).is_merged (), false);
  EXPECT_EQ ((db::Region () - r).to_string (), "");
  EXPECT_EQ ((db::Region () - r).empty (), true);
  EXPECT_EQ ((db::Region () - r).is_merged (), true);

  r -= db::Region (db::Box (db::Point (10, 20), db::Point (110, 220)));
  EXPECT_EQ (r.is_box (), false);
  EXPECT_EQ (r.empty (), false);
  EXPECT_EQ (r.is_merged (), true);
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,400;200,400;200,-100/10,20;110,20;110,220;10,220)");
}

TEST(8) 
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  EXPECT_EQ (r.is_box (), true);

  EXPECT_EQ (r.sized (10).to_string (), "(-10,-10;-10,210;110,210;110,-10)");
  EXPECT_EQ (r.sized (10).is_box (), true);
  EXPECT_EQ (r.sized (10).is_merged (), true);
  EXPECT_EQ (r.sized (db::Coord (10), db::Coord (20)).to_string (), "(-10,-20;-10,220;110,220;110,-20)");
  EXPECT_EQ (r.sized (db::Coord (10), db::Coord (20)).is_box (), true);
  EXPECT_EQ (r.sized (db::Coord (10), db::Coord (20)).is_merged (), true);

  r.size (10);
  EXPECT_EQ (r.to_string (), "(-10,-10;-10,210;110,210;110,-10)");
  r.size (db::Coord (10), db::Coord (20));
  EXPECT_EQ (r.to_string (), "(-20,-30;-20,230;120,230;120,-30)");
}

TEST(9) 
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (200, 400)));
  EXPECT_EQ (r.sized (10).to_string (), "(-110,-110;-110,410;210,410;210,-110)");
  EXPECT_EQ (r.sized (10).is_box (), true);
  EXPECT_EQ (r.sized (10).is_merged (), false);
  EXPECT_EQ (r.sized (db::Coord (10), db::Coord (20)).to_string (), "(-110,-120;-110,420;210,420;210,-120)");
  EXPECT_EQ (r.sized (db::Coord (10), db::Coord (20)).is_box (), true);
  EXPECT_EQ (r.sized (db::Coord (10), db::Coord (20)).is_merged (), false);
}

TEST(10a) 
{
  db::Region r;
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (20, 20), db::Point (30, 30)))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (false);
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (20, 20), db::Point (30, 30)))).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_not_interacting (db::Region (db::Box (db::Point (20, 20), db::Point (30, 30)))).to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (-20, -20), db::Point (30, 30)))).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (-200, -200), db::Point (-190, -190)))).to_string (), "");
  db::Region rr = r;
  r.select_interacting (db::Region (db::Box (db::Point (-20, -20), db::Point (-10, -10))));
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  rr.select_not_interacting (db::Region (db::Box (db::Point (-20, -20), db::Point (-10, -10))));
  EXPECT_EQ (rr.to_string (), "(0,0;0,200;100,200;100,0)");

  r.clear ();
  r.insert(db::Box (db::Point (1000, 0), db::Point (6000, 4000)));
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (0, 4000), db::Point (2000, 6000)))).to_string (), "(1000,0;1000,4000;6000,4000;6000,0)");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 4000), db::Point (2000, 6000))).selected_interacting (r).to_string (), "(0,4000;0,6000;2000,6000;2000,4000)");
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (0, 4000), db::Point (1000, 6000)))).to_string (), "(1000,0;1000,4000;6000,4000;6000,0)");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 4000), db::Point (1000, 6000))).selected_interacting (r).to_string (), "(0,4000;0,6000;1000,6000;1000,4000)");
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (0, 4001), db::Point (2000, 6000)))).to_string (), "");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 4001), db::Point (2000, 6000))).selected_interacting (r).to_string (), "");
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (0, 3999), db::Point (1000, 6000)))).to_string (), "(1000,0;1000,4000;6000,4000;6000,0)");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 3999), db::Point (1000, 6000))).selected_interacting (r).to_string (), "(0,3999;0,6000;1000,6000;1000,3999)");
  EXPECT_EQ (r.selected_overlapping (db::Region (db::Box (db::Point (0, 4000), db::Point (2000, 6000)))).to_string (), "");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 4000), db::Point (2000, 6000))).selected_overlapping (r).to_string (), "");
  EXPECT_EQ (r.selected_overlapping (db::Region (db::Box (db::Point (0, 4000), db::Point (1000, 6000)))).to_string (), "");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 4000), db::Point (1000, 6000))).selected_overlapping (r).to_string (), "");
  EXPECT_EQ (r.selected_overlapping (db::Region (db::Box (db::Point (0, 4001), db::Point (2000, 6000)))).to_string (), "");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 4001), db::Point (2000, 6000))).selected_overlapping (r).to_string (), "");
  EXPECT_EQ (r.selected_overlapping (db::Region (db::Box (db::Point (0, 3999), db::Point (1001, 6000)))).to_string (), "(1000,0;1000,4000;6000,4000;6000,0)");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 3999), db::Point (1001, 6000))).selected_overlapping (r).to_string (), "(0,3999;0,6000;1001,6000;1001,3999)");
}

TEST(10b) 
{
  db::Region r;
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (20, 20), db::Point (30, 30)))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (true);
  r.set_min_coherence (true);
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (20, 20), db::Point (30, 30)))).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (-20, -20), db::Point (30, 30)))).to_string (), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (-200, -200), db::Point (-190, -190)))).to_string (), "");
  r.select_interacting (db::Region (db::Box (db::Point (-20, -20), db::Point (-10, -10))));
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,0;0,0;0,-100)");
}

TEST(10c) 
{
  db::Region r;
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (20, 20), db::Point (30, 30)))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (true);
  r.set_min_coherence (false);
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (20, 20), db::Point (30, 30)))).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (-20, -20), db::Point (30, 30)))).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Region (db::Box (db::Point (-200, -200), db::Point (-190, -190)))).to_string (), "");
  r.select_interacting (db::Region (db::Box (db::Point (-20, -20), db::Point (-10, -10))));
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
}

TEST(11)
{
  db::Box bb[3] = { db::Box (db::Point (0, 0), db::Point (10, 10)), db::Box (), db::Box (db::Point (20, 20), db::Point (40, 50)) };
  EXPECT_EQ (db::Region (bb + 0, bb + 3).to_string (), "(0,0;0,10;10,10;10,0);(20,20;20,50;40,50;40,20)");
  EXPECT_EQ (db::Region (db::Polygon (db::Box (db::Point (0, 0), db::Point (10, 10)))).to_string (), "(0,0;0,10;10,10;10,0)");
  EXPECT_EQ (db::Region (db::Polygon ()).to_string (), "");
  EXPECT_EQ (db::Region (db::Box ()).to_string (), "");

  db::Point pts[2] = { db::Point (0, 0), db::Point (0, 20) };
  EXPECT_EQ (db::Region (db::Path (pts + 0, pts + 2, 10)).to_string (), "(-5,0;-5,20;5,20;5,0)");
}

TEST(12) 
{
  db::Box bb[3] = { db::Box (db::Point (0, 0), db::Point (10, 10)), db::Box (), db::Box (db::Point (20, 20), db::Point (40, 50)) };
  db::Region r (bb + 0, bb + 3);

  EXPECT_EQ (r.to_string (), "(0,0;0,10;10,10;10,0);(20,20;20,50;40,50;40,20)");
  db::RegionPerimeterFilter f0 (0, 40, false);
  db::Region rr = r.filtered (f0);
  EXPECT_EQ (rr.to_string (), "");
  db::RegionPerimeterFilter f1 (0, 41, false);
  rr = r.filtered (f1);
  EXPECT_EQ (rr.to_string (), "(0,0;0,10;10,10;10,0)");
  db::RegionPerimeterFilter f2 (0, 41, true);
  rr = r.filtered (f2);
  EXPECT_EQ (rr.to_string (), "(20,20;20,50;40,50;40,20)");
  EXPECT_EQ (rr.to_string (), "(20,20;20,50;40,50;40,20)");
  db::RegionPerimeterFilter f3 (50, std::numeric_limits<db::RegionPerimeterFilter::perimeter_type>::max (), false);
  r.filter (f3);
  EXPECT_EQ (r.to_string (), "(20,20;20,50;40,50;40,20)");
}

TEST(13) 
{
  db::Box bb[3] = { db::Box (db::Point (0, 0), db::Point (10, 10)), db::Box (), db::Box (db::Point (20, 20), db::Point (40, 50)) };
  db::Region r (bb + 0, bb + 3);

  EXPECT_EQ (r.to_string (), "(0,0;0,10;10,10;10,0);(20,20;20,50;40,50;40,20)");
  db::RegionAreaFilter f0 (0, 100, false);
  db::Region rr = r.filtered (f0);
  EXPECT_EQ (rr.to_string (), "");
  db::RegionAreaFilter f1 (0, 101, false);
  rr = r.filtered (f1);
  EXPECT_EQ (rr.to_string (), "(0,0;0,10;10,10;10,0)");
  db::RegionAreaFilter f2 (0, 101, true);
  rr = r.filtered (f2);
  EXPECT_EQ (rr.to_string (), "(20,20;20,50;40,50;40,20)");
  db::RegionAreaFilter f3 (110, std::numeric_limits<db::RegionAreaFilter::area_type>::max (), false);
  r.filter (f3);
  EXPECT_EQ (r.to_string (), "(20,20;20,50;40,50;40,20)");
}

TEST(14) 
{
  db::Box bb[3] = { db::Box (db::Point (0, 0), db::Point (10, 10)), db::Box (), db::Box (db::Point (20, 20), db::Point (40, 50)) };
  db::Region r (bb + 0, bb + 3);

  EXPECT_EQ (r.to_string (), "(0,0;0,10;10,10;10,0);(20,20;20,50;40,50;40,20)");
  db::RegionBBoxFilter f0 (0, 10, false, db::RegionBBoxFilter::BoxWidth);
  db::Region rr = r.filtered (f0);
  EXPECT_EQ (rr.to_string (), "");
  db::RegionBBoxFilter f1 (0, 11, false, db::RegionBBoxFilter::BoxWidth);
  rr = r.filtered (f1);
  EXPECT_EQ (rr.to_string (), "(0,0;0,10;10,10;10,0)");
  db::RegionBBoxFilter f2 (20, 21, false, db::RegionBBoxFilter::BoxWidth);
  rr = r.filtered (f2);
  EXPECT_EQ (rr.to_string (), "(20,20;20,50;40,50;40,20)");
  db::RegionBBoxFilter f3 (20, 31, false, db::RegionBBoxFilter::BoxHeight);
  rr = r.filtered (f3);
  EXPECT_EQ (rr.to_string (), "(20,20;20,50;40,50;40,20)");
  db::RegionBBoxFilter f4 (20, 31, true, db::RegionBBoxFilter::BoxHeight);
  rr = r.filtered (f4);
  EXPECT_EQ (rr.to_string (), "(0,0;0,10;10,10;10,0)");
  db::RegionBBoxFilter f5 (20, 31, false, db::RegionBBoxFilter::BoxMaxDim);
  rr = r.filtered (f5);
  EXPECT_EQ (rr.to_string (), "(20,20;20,50;40,50;40,20)");
  db::RegionBBoxFilter f6 (20, 31, false, db::RegionBBoxFilter::BoxMinDim);
  rr = r.filtered (f6);
  EXPECT_EQ (rr.to_string (), "(20,20;20,50;40,50;40,20)");
  db::RegionBBoxFilter f7 (20, 31, true, db::RegionBBoxFilter::BoxMinDim);
  rr = r.filtered (f7);
  EXPECT_EQ (rr.to_string (), "(0,0;0,10;10,10;10,0)");
  db::RegionBBoxFilter f8 (25, 26, false, db::RegionBBoxFilter::BoxAverageDim);
  rr = r.filtered (f8);
  EXPECT_EQ (rr.to_string (), "(20,20;20,50;40,50;40,20)");
}

TEST(15a) 
{
  db::Box bb[3] = { db::Box (db::Point (0, 0), db::Point (10, 10)), db::Box (), db::Box (db::Point (20, 20), db::Point (40, 50)) };
  db::Region r (bb + 0, bb + 3);

  EXPECT_EQ (r.width_check (15).to_string (), "(0,0;0,10)/(10,10;10,0);(0,10;10,10)/(10,0;0,0)");
  EXPECT_EQ (r.width_check (5).to_string (), "");
  EXPECT_EQ (r.width_check (5, false, db::Euclidian, 91).to_string (), "(0,5;0,10)/(0,10;5,10);(0,0;0,5)/(5,0;0,0);(5,10;10,10)/(10,10;10,5);(10,5;10,0)/(10,0;5,0);(20,45;20,50)/(20,50;25,50);(20,20;20,25)/(25,20;20,20);(35,50;40,50)/(40,50;40,45);(40,25;40,20)/(40,20;35,20)");
  EXPECT_EQ (r.space_check (15, false, db::Euclidian, 91).to_string (), "(9,10;10,10)/(20,20;20,21);(9,10;10,10)/(21,20;20,20);(10,10;10,9)/(20,20;20,21);(10,10;10,9)/(21,20;20,20)");
  EXPECT_EQ (r.space_check (15, false, db::Square, 91).to_string (), "(5,10;10,10)/(20,20;20,25);(5,10;10,10)/(25,20;20,20);(10,10;10,5)/(20,20;20,25);(10,10;10,5)/(25,20;20,20)");
  EXPECT_EQ (r.space_check (15).to_string (), "(9,10;10,10)/(21,20;20,20);(10,10;10,9)/(20,20;20,21)");
  EXPECT_EQ (r.space_check (15, true).to_string (), "(0,10;10,10)/(40,20;20,20);(10,10;10,0)/(20,20;20,50)");
  EXPECT_EQ (r.space_check (15, false, db::Square).to_string (), "(5,10;10,10)/(25,20;20,20);(10,10;10,5)/(20,20;20,25)");

  r.clear ();
  db::Point pts[] = {
    db::Point (20550000, -18950000),
    db::Point (20550000, -18920000),
    db::Point (20530000, -18920000),
    db::Point (20530000, -18910000),
    db::Point (20450000, -18910000),
    db::Point (20450000, -18850000),
    db::Point (20550000, -18850000),
    db::Point (20550000, -18880000),
    db::Point (20570000, -18880000),
    db::Point (20570000, -18890000),
    db::Point (20650000, -18890000),
    db::Point (20650000, -18950000)
  };
  db::Polygon poly;
  poly.assign_hull(pts + 0, pts + sizeof(pts)/sizeof(pts[0]));

  r.insert (poly);
  EXPECT_EQ (r.width_check (70000).to_string (), "(20550000,-18950000;20550000,-18920000)/(20570000,-18880000;20570000,-18890000);(20550000,-18920000;20530000,-18920000)/(20550000,-18880000;20570000,-18880000);(20550000,-18920000;20530000,-18920000)/(20570000,-18890000;20613246,-18890000);(20530000,-18920000;20530000,-18910000)/(20550000,-18850000;20550000,-18880000);(20530000,-18920000;20530000,-18910000)/(20570000,-18880000;20570000,-18890000);(20530000,-18910000;20450000,-18910000)/(20450000,-18850000;20550000,-18850000);(20530000,-18910000;20486754,-18910000)/(20550000,-18880000;20570000,-18880000);(20530000,-18910000;20502918,-18910000)/(20570000,-18890000;20597082,-18890000);(20570000,-18890000;20650000,-18890000)/(20650000,-18950000;20550000,-18950000)");
}

TEST(15b) 
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (200, 500)));
  r.insert (db::Box (db::Point (300, 0), db::Point (500, 200)));
  r.insert (db::Box (db::Point (300, 300), db::Point (500, 500)));
  r.insert (db::Box (db::Point (400, 200), db::Point (500, 300)));

  EXPECT_EQ (r.width_check (120, false, db::Projection).to_string (), "(400,200;400,300)/(500,300;500,200)");
  EXPECT_EQ (r.space_check (120, false, db::Projection).to_string (), "(200,200;200,0)/(300,0;300,200);(200,500;200,300)/(300,300;300,500);(300,200;400,200)/(400,300;300,300)");
  EXPECT_EQ (r.notch_check (120, false, db::Projection).to_string (), "(300,200;400,200)/(400,300;300,300)");
  EXPECT_EQ (r.isolated_check (120, false, db::Projection).to_string (), "(200,200;200,0)/(300,0;300,200);(200,500;200,300)/(300,300;300,500)");
}

TEST(15c) 
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (200, 300)));
  r.insert (db::Box (db::Point (0, 300), db::Point (200, 500)));
  r.insert (db::Box (db::Point (300, 0), db::Point (400, 200)));
  r.insert (db::Box (db::Point (400, 0), db::Point (500, 200)));
  r.insert (db::Box (db::Point (300, 300), db::Point (500, 400)));
  r.insert (db::Box (db::Point (300, 400), db::Point (500, 500)));
  r.insert (db::Box (db::Point (400, 200), db::Point (500, 250)));
  r.insert (db::Box (db::Point (400, 250), db::Point (500, 300)));

  EXPECT_EQ (r.width_check (120, false, db::Projection).to_string (), "(400,200;400,300)/(500,300;500,200)");
  EXPECT_EQ (r.space_check (120, false, db::Projection).to_string (), "(200,200;200,0)/(300,0;300,200);(200,500;200,300)/(300,300;300,500);(300,200;400,200)/(400,300;300,300)");
  EXPECT_EQ (r.notch_check (120, false, db::Projection).to_string (), "(300,200;400,200)/(400,300;300,300)");
  EXPECT_EQ (r.isolated_check (120, false, db::Projection).to_string (), "(200,200;200,0)/(300,0;300,200);(200,500;200,300)/(300,300;300,500)");
}

TEST(15d) 
{
  //  shielding
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 100)));
  r.insert (db::Box (db::Point (0, 200), db::Point (100, 300)));
  r.insert (db::Box (db::Point (300, 0), db::Point (400, 100)));
  r.insert (db::Box (db::Point (300, 200), db::Point (400, 300)));
  r.insert (db::Box (db::Point (600, 0), db::Point (700, 100)));
  r.insert (db::Box (db::Point (600, 200), db::Point (700, 300)));
  r.insert (db::Box (db::Point (0, 140), db::Point (350, 160)));

  EXPECT_EQ (r.space_check (120, false, db::Projection).to_string (), "(0,100;100,100)/(100,140;0,140);(300,100;350,100)/(350,140;300,140);(300,100;400,100)/(400,200;300,200);(600,100;700,100)/(700,200;600,200);(0,160;100,160)/(100,200;0,200);(300,160;350,160)/(350,200;300,200)");
}

TEST(15e) 
{
  //  #650
  db::Region r;

  for (int i = 0; i < 5; ++i) {
    db::Point pts[] = {
      db::Point (342800 + i * 2000, 29000),
      db::Point (342800 + i * 2000, 40600),
      db::Point (342801 + i * 2000, 40600),
      db::Point (342801 + i * 2000, 29000)
    };
    db::Polygon poly;
    poly.assign_hull(pts + 0, pts + sizeof(pts)/sizeof(pts[0]));
    r.insert (poly);
  }

  {
    db::Point pts[] = {
      db::Point (0, 0),
      db::Point (0, 69600),
      db::Point (501154, 69600),
      db::Point (501154, 66000),
      db::Point (19957, 66000),
      db::Point (19957, 3600),
      db::Point (20857, 3600),
      db::Point (20857, 66000),
      db::Point (23457, 66000),
      db::Point (23457, 3600),
      db::Point (61857, 3600),
      db::Point (61857, 66000),
      db::Point (207457, 66000),
      db::Point (207457, 3600),
      db::Point (245857, 3600),
      db::Point (245857, 66000),
      db::Point (248457, 66000),
      db::Point (248457, 3600),
      db::Point (501154, 3600),
      db::Point (501154, 0)
    };
    db::Polygon poly;
    poly.assign_hull(pts + 0, pts + sizeof(pts)/sizeof(pts[0]));
    r.insert (poly);
  }

  EXPECT_EQ (r.space_check (1000).to_string (), "(20857,3600;20857,66000)/(19957,66000;19957,3600)");
}

TEST(16) 
{
  db::Region a;
  a.insert (db::Box (db::Point (10, 20), db::Point (20, 30)));

  db::Region b;
  b.insert (db::Box (db::Point (0, 0), db::Point (100, 100)));

  EXPECT_EQ (a.inside_check (b, 15).to_string (), "(10,20;10,30)/(0,9;0,41)");
  EXPECT_EQ (a.inside_check (b, 15, true).to_string (), "(10,20;10,30)/(0,0;0,100)");
  EXPECT_EQ (a.inside_check (b, 15, false, db::Euclidian, 91).to_string (), "(10,20;10,30)/(0,9;0,41);(10,30;15,30)/(0,30;0,41);(15,20;10,20)/(0,9;0,20)");
  EXPECT_EQ (b.enclosing_check (a, 15).to_string (), "(0,9;0,41)/(10,20;10,30)");
  EXPECT_EQ (b.enclosing_check (a, 15, true).to_string (), "(0,0;0,100)/(10,20;10,30)");
  EXPECT_EQ (b.enclosing_check (a, 15, false, db::Euclidian, 91).to_string (), "(0,9;0,41)/(10,20;10,30);(0,30;0,41)/(10,30;15,30);(0,9;0,20)/(15,20;10,20)");

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

TEST(17) 
{
  db::Box bb[3] = { db::Box (db::Point (0, 0), db::Point (10, 10)), db::Box (), db::Box (db::Point (20, 20), db::Point (40, 50)) };
  db::Region r (bb + 0, bb + 3);

  EXPECT_EQ (r.edges ().to_string (), "(0,0;0,10);(0,10;10,10);(10,10;10,0);(10,0;0,0);(20,20;20,50);(20,50;40,50);(40,50;40,20);(40,20;20,20)");
  db::EdgeLengthFilter f (11, 21, false);
  EXPECT_EQ (r.edges (f).to_string (), "(20,50;40,50);(40,20;20,20)");
}

TEST(18a) 
{
  db::Region r;
  r.set_merged_semantics (false);
  r.insert (db::Box (db::Point (0, 0), db::Point (20, 20)));
  r.insert (db::Box (db::Point (20, 30), db::Point (40, 50)));
  r.insert (db::Box (db::Point (50, 10), db::Point (70, 30)));
  r.insert (db::Box (db::Point (70, 60), db::Point (90, 80)));
  r.insert (db::Box (db::Point (0, 60), db::Point (60, 80)));
  r.insert (db::Box (db::Point (0, 100), db::Point (30, 130)));

  db::Region rr;
  rr.insert (db::Box (db::Point (10, 10), db::Point (50, 90)));
  rr.insert (db::Box (db::Point (10, 110), db::Point (20, 120)));

  EXPECT_EQ (r.selected_outside (rr).to_string (), "(50,10;50,30;70,30;70,10);(70,60;70,80;90,80;90,60)");
  {
    db::Region o = r;
    o.select_outside (rr);
    EXPECT_EQ (o.to_string (), "(50,10;50,30;70,30;70,10);(70,60;70,80;90,80;90,60)");
    o = r;
    EXPECT_EQ (o.selected_not_outside (rr).to_string (), "(0,0;0,20;20,20;20,0);(20,30;20,50;40,50;40,30);(0,60;0,80;60,80;60,60);(0,100;0,130;30,130;30,100)");
    EXPECT_EQ (o.selected_outside (rr).size () + o.selected_not_outside (rr).size (), size_t (6));
    o.select_not_outside (rr);
    EXPECT_EQ (o.to_string (), "(0,0;0,20;20,20;20,0);(20,30;20,50;40,50;40,30);(0,60;0,80;60,80;60,60);(0,100;0,130;30,130;30,100)");
  }
  EXPECT_EQ (r.selected_inside (rr).to_string (), "(20,30;20,50;40,50;40,30)");
  {
    db::Region o = r;
    o.select_inside (rr);
    EXPECT_EQ (o.to_string (), "(20,30;20,50;40,50;40,30)");
    o = r;
    EXPECT_EQ (o.selected_not_inside (rr).to_string (), "(0,0;0,20;20,20;20,0);(50,10;50,30;70,30;70,10);(70,60;70,80;90,80;90,60);(0,60;0,80;60,80;60,60);(0,100;0,130;30,130;30,100)");
    EXPECT_EQ (o.selected_inside (rr).size () + o.selected_not_inside (rr).size (), size_t (6));
    o.select_not_inside (rr);
    EXPECT_EQ (o.to_string (), "(0,0;0,20;20,20;20,0);(50,10;50,30;70,30;70,10);(70,60;70,80;90,80;90,60);(0,60;0,80;60,80;60,60);(0,100;0,130;30,130;30,100)");
  }
  EXPECT_EQ (r.selected_interacting (rr).to_string (), "(0,0;0,20;20,20;20,0);(20,30;20,50;40,50;40,30);(50,10;50,30;70,30;70,10);(0,60;0,80;60,80;60,60);(0,100;0,130;30,130;30,100)");
  {
    db::Region o = r;
    o.select_interacting (rr);
    EXPECT_EQ (o.to_string (), "(0,0;0,20;20,20;20,0);(20,30;20,50;40,50;40,30);(50,10;50,30;70,30;70,10);(0,60;0,80;60,80;60,60);(0,100;0,130;30,130;30,100)");
    o = r;
    EXPECT_EQ (o.selected_not_interacting (rr).to_string (), "(70,60;70,80;90,80;90,60)");
    EXPECT_EQ (o.selected_interacting (rr).size () + o.selected_not_interacting (rr).size (), size_t (6));
    o.select_not_interacting (rr);
    EXPECT_EQ (o.to_string (), "(70,60;70,80;90,80;90,60)");
  }
  EXPECT_EQ (r.selected_overlapping (rr).to_string (), "(0,0;0,20;20,20;20,0);(20,30;20,50;40,50;40,30);(0,60;0,80;60,80;60,60);(0,100;0,130;30,130;30,100)");
  {
    db::Region o = r;
    o.select_overlapping (rr);
    EXPECT_EQ (o.to_string (), "(0,0;0,20;20,20;20,0);(20,30;20,50;40,50;40,30);(0,60;0,80;60,80;60,60);(0,100;0,130;30,130;30,100)");
    o = r;
    EXPECT_EQ (o.selected_not_overlapping (rr).to_string (), "(50,10;50,30;70,30;70,10);(70,60;70,80;90,80;90,60)");
    EXPECT_EQ (o.selected_overlapping (rr).size () + o.selected_not_overlapping (rr).size (), size_t (6));
    o.select_not_overlapping (rr);
    EXPECT_EQ (o.to_string (), "(50,10;50,30;70,30;70,10);(70,60;70,80;90,80;90,60)");
  }
}

TEST(18b) 
{
  //  complete test (#679)

  struct {
    int i1, i2;
    bool inside;
    bool outside;
  } intervals[] = {
    { 10, 20, false, true },
    { 20, 30, false, true },
    { 20, 40, false, false },
    { 30, 50, true, false },
    { 30, 60, true, false },
    { 40, 50, true, false },
    { 50, 60, true, false },
    { 50, 70, false, false },
    { 60, 70, false, true },
    { 70, 80, false, true }
  };

  for (int ix = 0; ix < int (sizeof (intervals) / sizeof (intervals[0])); ++ix) {

    for (int iy = 0; iy < int (sizeof (intervals) / sizeof (intervals[0])); ++iy) {

      db::Region r;
      r.insert (db::Box (intervals [ix].i1, intervals [iy].i1, intervals [ix].i2, intervals [iy].i2));

      db::Region rr;
      rr.insert (db::Box (db::Point (30, 30), db::Point (60, 60)));

#if 0
      std::cout << "--------" << std::endl;
      std::cout << "r = " << r.to_string () << std::endl;
      std::cout << "rr = " << rr.to_string () << std::endl;
      std::cout << "r.selected_outside(rr) = " << (r.selected_outside (rr)).to_string () << std::endl;
      std::cout << "r.selected_not_outside(rr) = " << (r.selected_not_outside (rr)).to_string () << std::endl;
      std::cout << "r.selected_inside(rr) = " << (r.selected_inside (rr)).to_string () << std::endl;
      std::cout << "r.selected_not_inside(rr) = " << (r.selected_not_inside (rr)).to_string () << std::endl;
#endif

      EXPECT_EQ (r.selected_outside (rr).empty (), ! (intervals [ix].outside || intervals [iy].outside));
      EXPECT_EQ (r.selected_not_outside (rr).empty (), intervals [ix].outside || intervals [iy].outside);
      EXPECT_EQ (r.selected_inside (rr).empty (), ! (intervals [ix].inside && intervals [iy].inside));
      EXPECT_EQ (r.selected_not_inside (rr).empty (), intervals [ix].inside && intervals [iy].inside);

    }

  }
}

TEST(18c)
{
  //  GitHub issue #69

  db::Region r;
  r.insert (db::Box (db::Point (-120, 0), db::Point (-100, 20)));
  r.insert (db::Box (db::Point (-20, 0), db::Point (0, 20)));
  r.insert (db::Box (db::Point (0, 0), db::Point (20, 20)));
  r.insert (db::Box (db::Point (100, 0), db::Point (120, 20)));

  db::Region rr;
  rr.insert (db::Box (db::Point (-100, -10), db::Point (0, 30)));
  rr.insert (db::Box (db::Point (0, -10), db::Point (100, 30)));

  EXPECT_EQ (r.selected_outside (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_inside (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_overlapping (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_interacting (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(-20,0;-20,20;20,20;20,0);(100,0;100,20;120,20;120,0)");

  EXPECT_EQ (r.selected_not_outside (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_not_inside (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_not_overlapping (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");

  r.clear ();
  r.insert (db::Box (db::Point (-120, 0), db::Point (-100, 20)));
  r.insert (db::Box (db::Point (-20, 0), db::Point (20, 20)));
  r.insert (db::Box (db::Point (100, 0), db::Point (120, 20)));

  rr.clear ();
  rr.insert (db::Box (db::Point (-100, -10), db::Point (0, 30)));
  rr.insert (db::Box (db::Point (0, -10), db::Point (100, 30)));

  EXPECT_EQ (r.selected_outside (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_inside (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_overlapping (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_interacting (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(-20,0;-20,20;20,20;20,0);(100,0;100,20;120,20;120,0)");

  EXPECT_EQ (r.selected_not_outside (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_not_inside (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_not_overlapping (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");

  r.clear ();
  r.insert (db::Box (db::Point (-120, 0), db::Point (-100, 20)));
  r.insert (db::Box (db::Point (-20, 0), db::Point (20, 20)));
  r.insert (db::Box (db::Point (100, 0), db::Point (120, 20)));

  rr.clear ();
  rr.insert (db::Box (db::Point (-100, -10), db::Point (100, 30)));

  EXPECT_EQ (r.selected_outside (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_inside (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_overlapping (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_interacting (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(-20,0;-20,20;20,20;20,0);(100,0;100,20;120,20;120,0)");

  EXPECT_EQ (r.selected_not_outside (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_not_inside (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_not_overlapping (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");

  r.clear ();
  r.insert (db::Box (db::Point (-120, 0), db::Point (-100, 20)));
  r.insert (db::Box (db::Point (-20, 0), db::Point (20, 20)));
  r.insert (db::Box (db::Point (100, 0), db::Point (120, 20)));

  rr.clear ();
  rr.insert (db::Box (db::Point (-100, -10), db::Point (0, 30)));
  rr.insert (db::Box (db::Point (1, -10), db::Point (100, 30)));

  EXPECT_EQ (r.selected_outside (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_inside (rr).to_string (), "");
  EXPECT_EQ (r.selected_overlapping (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_interacting (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(-20,0;-20,20;20,20;20,0);(100,0;100,20;120,20;120,0)");

  EXPECT_EQ (r.selected_not_outside (rr).to_string (), "(-20,0;-20,20;20,20;20,0)");
  EXPECT_EQ (r.selected_not_inside (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(-20,0;-20,20;20,20;20,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_not_overlapping (rr).to_string (), "(-120,0;-120,20;-100,20;-100,0);(100,0;100,20;120,20;120,0)");
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");

  r.clear ();
  r.insert (db::Box (db::Point (-100, 0), db::Point (-80, 20)));
  r.insert (db::Box (db::Point (-20, 0), db::Point (0, 20)));
  r.insert (db::Box (db::Point (0, 0), db::Point (20, 20)));
  r.insert (db::Box (db::Point (80, 0), db::Point (100, 20)));

  rr.clear ();
  rr.insert (db::Box (db::Point (-100, -10), db::Point (0, 30)));
  rr.insert (db::Box (db::Point (0, -10), db::Point (100, 30)));

  EXPECT_EQ (r.selected_outside (rr).to_string (), "");
  EXPECT_EQ (r.selected_inside (rr).to_string (), "(-100,0;-100,20;-80,20;-80,0);(-20,0;-20,20;20,20;20,0);(80,0;80,20;100,20;100,0)");
  EXPECT_EQ (r.selected_overlapping (rr).to_string (), "(-100,0;-100,20;-80,20;-80,0);(-20,0;-20,20;20,20;20,0);(80,0;80,20;100,20;100,0)");
  EXPECT_EQ (r.selected_interacting (rr).to_string (), "(-100,0;-100,20;-80,20;-80,0);(-20,0;-20,20;20,20;20,0);(80,0;80,20;100,20;100,0)");

  EXPECT_EQ (r.selected_not_outside (rr).to_string (), "(-100,0;-100,20;-80,20;-80,0);(-20,0;-20,20;20,20;20,0);(80,0;80,20;100,20;100,0)");
  EXPECT_EQ (r.selected_not_inside (rr).to_string (), "");
  EXPECT_EQ (r.selected_not_overlapping (rr).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");
}

TEST(18d)
{
  db::Region r;
  r.set_merged_semantics (false);
  r.insert (db::Box (db::Point (0, 0), db::Point (10, 10)));
  r.insert (db::Box (db::Point (20, 30), db::Point (40, 50)));
  r.insert (db::Box (db::Point (50, 10), db::Point (70, 30)));
  r.insert (db::Box (db::Point (70, 60), db::Point (90, 80)));
  r.insert (db::Box (db::Point (0, 60), db::Point (60, 80)));
  r.insert (db::Box (db::Point (0, 100), db::Point (30, 130)));

  db::Region rr;
  rr.insert (db::Box (db::Point (10, 0), db::Point (20, 10)));
  rr.insert (db::Box (db::Point (10, 10), db::Point (50, 90)));
  rr.insert (db::Box (db::Point (10, 110), db::Point (20, 120)));

  EXPECT_EQ (r.pull_inside (rr).to_string (), "(10,110;10,120;20,120;20,110)");

  EXPECT_EQ (r.pull_interacting (rr).to_string (), "(10,0;10,90;50,90;50,10;20,10;20,0);(10,110;10,120;20,120;20,110)");
  EXPECT_EQ (r.pull_overlapping (rr).to_string (), "(10,0;10,90;50,90;50,10;20,10;20,0);(10,110;10,120;20,120;20,110)");

  rr.set_merged_semantics (false);
  EXPECT_EQ (r.pull_interacting (rr).to_string (), "(10,0;10,10;20,10;20,0);(10,10;10,90;50,90;50,10);(10,110;10,120;20,120;20,110)");
  EXPECT_EQ (r.pull_overlapping (rr).to_string (), "(10,10;10,90;50,90;50,10);(10,110;10,120;20,120;20,110)");
}

TEST(19)
{
  db::Region r1;
  r1.insert (db::Box (db::Point (0, 0), db::Point (10, 20)));
  r1.insert (db::Box (db::Point (0, 0), db::Point (20, 20)));
  r1.insert (db::Box (db::Point (0, 0), db::Point (20, 30)));
  EXPECT_EQ (r1.has_valid_polygons (), true);

  db::Region r2;
  r2.insert (db::Box (db::Point (0, 0), db::Point (20, 20)));
  r2.insert (db::Box (db::Point (0, 0), db::Point (20, 10)));
  r2.insert (db::Box (db::Point (0, 0), db::Point (20, 30)));

  EXPECT_EQ (r1.in (r2, false).to_string (), "(0,0;0,30;20,30;20,0)");
  EXPECT_EQ (r1.in (r2, true).to_string (), "");
  EXPECT_EQ (r2.in (r1, true).to_string (), "");

  r1.set_merged_semantics (false);
  r2.set_merged_semantics (false);

  EXPECT_EQ (r1.in (r2, false).to_string (), "(0,0;0,20;20,20;20,0);(0,0;0,30;20,30;20,0)");
  EXPECT_EQ (r1.in (r2, true).to_string (), "(0,0;0,20;10,20;10,0)");
  EXPECT_EQ (r2.in (r1, true).to_string (), "(0,0;0,10;20,10;20,0)");
}

TEST(20)
{
  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.insert_layer (db::LayerProperties (2, 0));
  db::cell_index_type top = ly.add_cell ("TOP");
  db::cell_index_type c1 = ly.add_cell ("C1");
  db::cell_index_type c2 = ly.add_cell ("C2");
  ly.cell (c1).shapes (l1).insert (db::Box (0, 0, 30, 30));
  ly.cell (c2).shapes (l2).insert (db::Box (0, 0, 30, 30));
  ly.cell (top).insert (db::CellInstArray (c1, db::Trans (db::Vector (0, 0))));
  ly.cell (top).insert (db::CellInstArray (c1, db::Trans (db::Vector (50, 0))));
  ly.cell (top).insert (db::CellInstArray (c1, db::Trans (db::Vector (50, 40))));
  ly.cell (top).insert (db::CellInstArray (c2, db::Trans (db::Vector (10, 10))));
  ly.cell (top).insert (db::CellInstArray (c2, db::Trans (db::Vector (80, 40))));
  ly.cell (top).insert (db::CellInstArray (c2, db::Trans (db::Vector (110, 40))));
  ly.cell (top).shapes (l2).insert (db::Box (60, 10, 70, 20));

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l1));
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r1.to_string (), "(0,0;0,30;30,30;30,0);(50,0;50,30;80,30;80,0);(50,40;50,70;80,70;80,40)");
    EXPECT_EQ (r1.has_valid_polygons (), false);
  }

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2, db::Box (60, 10, 90, 50)));
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r1.to_string (), "(60,10;60,20;70,20;70,10);(80,40;80,70;110,70;110,40)");
    EXPECT_EQ (r1.has_valid_polygons (), false);
  }

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2, db::Box (60, 10, 90, 50)), db::ICplxTrans (2.0));
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r1.to_string (), "(120,20;120,40;140,40;140,20);(160,80;160,140;220,140;220,80)");
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r1.area (), 4000);
    EXPECT_EQ (r1.perimeter (), db::Region::perimeter_type (320));
    EXPECT_EQ (r1.bbox ().to_string (), "(120,20;220,140)");
    EXPECT_EQ (r1.is_box (), false);
    EXPECT_EQ (r1.size (), size_t (2));
    EXPECT_EQ (r1.empty (), false);

    db::RegionPerimeterFilter f0 (0, 100, false);
    db::Region rr = r1.filtered (f0);
    EXPECT_EQ (rr.to_string (), "(120,20;120,40;140,40;140,20)");

    db::Region r2 = r1;
    EXPECT_EQ (r2.has_valid_polygons (), false);
    EXPECT_EQ (r2.area (), 4000);
    EXPECT_EQ (r2.perimeter (), db::Region::perimeter_type (320));
    EXPECT_EQ (r2.bbox ().to_string (), "(120,20;220,140)");
    EXPECT_EQ (r2.is_box (), false);
    EXPECT_EQ (r2.size (), size_t (2));
    EXPECT_EQ (r2.empty (), false);
    r2.filter (f0);
    EXPECT_EQ (r2.has_valid_polygons (), true);
    EXPECT_EQ (r2.to_string (), "(120,20;120,40;140,40;140,20)");
    EXPECT_EQ (r2.size (), size_t (1));
    EXPECT_EQ (r2.empty (), false);
    EXPECT_EQ (r2.is_box (), true);
    EXPECT_EQ (r2.area (), 400);
    EXPECT_EQ (r2.perimeter (), db::Region::perimeter_type (80));

    r1.insert (db::Box (0, 0, 10, 20));
    EXPECT_EQ (r1.has_valid_polygons (), true);
    EXPECT_EQ (r1.to_string (), "(120,20;120,40;140,40;140,20);(160,80;160,140;220,140;220,80);(0,0;0,20;10,20;10,0)");
    EXPECT_EQ (r1.to_string (2), "(120,20;120,40;140,40;140,20);(160,80;160,140;220,140;220,80)...");
    EXPECT_EQ (r1.size (), size_t (3));
    EXPECT_EQ (r1.area (), 4200);
    EXPECT_EQ (r1.perimeter (), db::Region::perimeter_type (380));

    rr = r1.filtered (f0);
    EXPECT_EQ (rr.to_string (), "(0,0;0,20;10,20;10,0);(120,20;120,40;140,40;140,20)");
    EXPECT_EQ (r1.to_string (), "(120,20;120,40;140,40;140,20);(160,80;160,140;220,140;220,80);(0,0;0,20;10,20;10,0)");

    r1.filter (f0);
    EXPECT_EQ (r1.to_string (), "(0,0;0,20;10,20;10,0);(120,20;120,40;140,40;140,20)");
  }

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2, db::Box (60, 10, 70, 50)), db::ICplxTrans (2.0));
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r1.to_string (), "(120,20;120,40;140,40;140,20)");
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r1.is_box (), true);
    EXPECT_EQ (r1.size (), size_t (1));
    EXPECT_EQ (r1.empty (), false);

    db::Region r2 = r1;

    EXPECT_EQ (r1.transformed (db::ICplxTrans (0.5)).to_string (), "(60,10;60,20;70,20;70,10)");
    r1.transform (db::ICplxTrans (0.5));
    EXPECT_EQ (r1.has_valid_polygons (), true);
    EXPECT_EQ (r1.to_string (), "(60,10;60,20;70,20;70,10)");

    r1.clear ();
    EXPECT_EQ (r1.has_valid_polygons (), true);
    EXPECT_EQ (r1.size (), size_t (0));
    EXPECT_EQ (r1.empty (), true);
    EXPECT_EQ (r1.perimeter (), db::Region::perimeter_type (0));
    EXPECT_EQ (r1.area (), 0);

    EXPECT_EQ (r2.to_string (), "(120,20;120,40;140,40;140,20)");
    r1.swap (r2);

    EXPECT_EQ (r1.to_string (), "(120,20;120,40;140,40;140,20)");
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r2.has_valid_polygons (), true);
    EXPECT_EQ (r2.size (), size_t (0));
    EXPECT_EQ (r2.empty (), true);
    EXPECT_EQ (r2.perimeter (), db::Region::perimeter_type (0));
    EXPECT_EQ (r2.area (), 0);
  }

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2));
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r1.is_merged (), false);
    EXPECT_EQ (r1.merged ().to_string (), "(60,10;60,20;70,20;70,10);(10,10;10,40;40,40;40,10);(80,40;80,70;140,70;140,40)");
    r1.merge ();
    EXPECT_EQ (r1.to_string (), "(60,10;60,20;70,20;70,10);(10,10;10,40;40,40;40,10);(80,40;80,70;140,70;140,40)");
    EXPECT_EQ (r1.has_valid_polygons (), true);
  }

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2));
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r1.edges ().to_string (30), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(80,40;80,70);(80,70;140,70);(140,70;140,40);(140,40;80,40)");
    r1.set_merged_semantics (false);
    EXPECT_EQ (r1.edges ().to_string (30), "(60,10;60,20);(60,20;70,20);(70,20;70,10);(70,10;60,10);(10,10;10,40);(10,40;40,40);(40,40;40,10);(40,10;10,10);(80,40;80,70);(80,70;110,70);(110,70;110,40);(110,40;80,40);(110,40;110,70);(110,70;140,70);(140,70;140,40);(140,40;110,40)");
  }

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2));
    EXPECT_EQ (r1.width_check (20).to_string (), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10)");
    EXPECT_EQ (r1.width_check (50).to_string (), "(60,10;60,20)/(70,20;70,10);(60,20;70,20)/(70,10;60,10);(10,10;10,40)/(40,40;40,10);(10,40;40,40)/(40,10;10,10);(80,70;140,70)/(140,40;80,40)");
  }

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2));
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r1.space_check (30).to_string (), "(60,10;60,20)/(40,40;40,10);(60,20;70,20)/(92,40;80,40);(70,20;70,12)/(80,40;80,48)");
    EXPECT_EQ (r1.space_check (2).to_string (), "");
  }

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l1));
    EXPECT_EQ (r1.has_valid_polygons (), false);
    db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (top), l2));
    EXPECT_EQ (r2.has_valid_polygons (), false);
    EXPECT_EQ (r1.separation_check (r2, 20).to_string (), "(50,0;50,30)/(40,40;40,10);(63,30;80,30)/(97,40;80,40);(50,40;50,57)/(40,40;40,23);(80,70;80,40)/(80,40;80,70)");
  }

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l2));
    EXPECT_EQ (r1.sized (10).to_string (), "(50,0;50,30;80,30;80,0);(0,0;0,50;50,50;50,0);(70,30;70,80;150,80;150,30)");
    r1.size (10);
    EXPECT_EQ (r1.has_valid_polygons (), true);
    EXPECT_EQ (r1.to_string (), "(50,0;50,30;80,30;80,0);(0,0;0,50;50,50;50,0);(70,30;70,80;150,80;150,30)");
  }

  {
    db::Region r1 (db::RecursiveShapeIterator (ly, ly.cell (top), l1));
    EXPECT_EQ (r1.has_valid_polygons (), false);
    EXPECT_EQ (r1.hulls ().to_string (), "(0,0;0,30;30,30;30,0);(50,0;50,30;80,30;80,0);(50,40;50,70;80,70;80,40)");
    EXPECT_EQ (r1.holes ().to_string (), "");
    db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (top), l2));
    EXPECT_EQ (r2.has_valid_polygons (), false);
    EXPECT_EQ ((r1 & r2).to_string (), "(60,10;60,20;70,20;70,10);(10,10;10,30;30,30;30,10)");
    EXPECT_EQ ((r1 | r2).to_string (), "(50,0;50,30;80,30;80,0);(0,0;0,30;10,30;10,40;40,40;40,10;30,10;30,0);(50,40;50,70;140,70;140,40)");
    EXPECT_EQ ((r1 + r2).to_string (), "(0,0;0,30;30,30;30,0);(50,0;50,30;80,30;80,0);(50,40;50,70;80,70;80,40);(60,10;60,20;70,20;70,10);(10,10;10,40;40,40;40,10);(80,40;80,70;110,70;110,40);(110,40;110,70;140,70;140,40)");
    EXPECT_EQ ((r1 ^ r2).to_string (), "(50,0;50,30;80,30;80,0/60,10;70,10;70,20;60,20);(0,0;0,30;10,30;10,40;40,40;40,10;30,10;30,0/10,10;30,10;30,30;10,30);(50,40;50,70;140,70;140,40)");
    EXPECT_EQ ((r1 ^ r1).to_string (), "");
    EXPECT_EQ ((r1 - r2).to_string (), "(0,0;0,30;10,30;10,10;30,10;30,0);(50,0;50,30;80,30;80,0/60,10;70,10;70,20;60,20);(50,40;50,70;80,70;80,40)");
    EXPECT_EQ ((r1 - r1).to_string (), "");
    EXPECT_EQ (r2.selected_outside (r1).to_string (), "(80,40;80,70;140,70;140,40)");
    EXPECT_EQ (r2.selected_inside (r1).to_string (), "(60,10;60,20;70,20;70,10)");
    EXPECT_EQ (r2.selected_interacting (r1).to_string (), "(60,10;60,20;70,20;70,10);(10,10;10,40;40,40;40,10);(80,40;80,70;140,70;140,40)");
    EXPECT_EQ (r2.selected_overlapping (r1).to_string (), "(60,10;60,20;70,20;70,10);(10,10;10,40;40,40;40,10)");
    r2.select_outside (r1);
    EXPECT_EQ (r2.to_string (), "(80,40;80,70;140,70;140,40)");
  }
}

TEST(21)
{
  db::Region r;
  EXPECT_EQ (r.strange_polygon_check ().to_string (), "");

  db::Point pts1[] = {
    db::Point (0, 0),
    db::Point (0, 1000),
    db::Point (1000, 1000),
    db::Point (1000, 500),
    db::Point (500, 500),
    db::Point (500, 600),
    db::Point (600, 600),
    db::Point (600, 0)
  };
  db::Point pts2[] = {
    db::Point (2000, 0),
    db::Point (2000, 500),
    db::Point (3000, 500),
    db::Point (3000, 1000),
    db::Point (2500, 1000),
    db::Point (2500, 0)
  };

  db::Polygon poly;
  poly.assign_hull(pts1 + 0, pts1 + sizeof(pts1)/sizeof(pts1[0]));
  r.insert (poly);
  poly.assign_hull(pts2 + 0, pts2 + sizeof(pts2)/sizeof(pts2[0]));
  r.insert (poly);

  EXPECT_EQ (r.strange_polygon_check ().to_string (), "(500,500;500,600;600,600;600,500);(2500,500;2500,1000;3000,1000;3000,500)");
  r.merge ();
  EXPECT_EQ (r.strange_polygon_check ().to_string (), "");
}

TEST(22)
{
  db::Region r;
  EXPECT_EQ (r.angle_check (0, 180.0, false).to_string (), "");
  EXPECT_EQ (r.angle_check (0, 180.0, true).to_string (), "");

  db::Point pts1[] = {
    db::Point (0, 0),
    db::Point (0, 1000),
    db::Point (1000, 2000),
    db::Point (1000, 0)
  };

  db::Polygon poly;
  poly.assign_hull(pts1 + 0, pts1 + sizeof(pts1)/sizeof(pts1[0]));
  r.insert (poly);

  EXPECT_EQ (r.angle_check (0, 180.0, false).to_string (), "(0,0;0,1000)/(0,1000;1000,2000);(0,1000;1000,2000)/(1000,2000;1000,0);(1000,2000;1000,0)/(1000,0;0,0);(1000,0;0,0)/(0,0;0,1000)");
  EXPECT_EQ (r.angle_check (0, 180.0, true).to_string (), "");
  EXPECT_EQ (r.angle_check (45.0, 45.1, false).to_string (), "(0,1000;1000,2000)/(1000,2000;1000,0)");
  EXPECT_EQ (r.angle_check (0.0, 90.0, false).to_string (), "(0,1000;1000,2000)/(1000,2000;1000,0)");
  EXPECT_EQ (r.angle_check (0.0, 90.0, true).to_string (), "(0,0;0,1000)/(0,1000;1000,2000);(1000,2000;1000,0)/(1000,0;0,0);(1000,0;0,0)/(0,0;0,1000)");
  EXPECT_EQ (r.angle_check (90.1, 180.0, false).to_string (), "(0,0;0,1000)/(0,1000;1000,2000)");
  EXPECT_EQ (r.angle_check (90.1, 180.0, true).to_string (), "(0,1000;1000,2000)/(1000,2000;1000,0);(1000,2000;1000,0)/(1000,0;0,0);(1000,0;0,0)/(0,0;0,1000)");
}

TEST(22b)
{
  db::Region r;

  db::Point pts1[] = {
    db::Point (0, 0),
    db::Point (1000, 1000),
    db::Point (1000, 800),
    db::Point (200, 800),
    db::Point (800, 200),
    db::Point (800, 0)
  };

  db::Polygon poly;
  poly.assign_hull(pts1 + 0, pts1 + sizeof(pts1)/sizeof(pts1[0]));
  r.insert (poly);
  r.set_merged_semantics (false);

  EXPECT_EQ (r.angle_check (0, 180.0, false).to_string (), "(0,0;1000,1000)/(1000,1000;1000,800);(1000,1000;1000,800)/(1000,800;200,800);(200,800;800,200)/(800,200;800,0);(800,200;800,0)/(800,0;0,0);(800,0;0,0)/(0,0;1000,1000)");
  EXPECT_EQ (r.angle_check (0, 180.0, true).to_string (), "(1000,800;200,800)/(200,800;800,200)");
  EXPECT_EQ (r.angle_check (45.0, 45.1, false).to_string (), "(0,0;1000,1000)/(1000,1000;1000,800);(800,0;0,0)/(0,0;1000,1000)");
  EXPECT_EQ (r.angle_check (315.0, 315.1, false).to_string (), "(1000,800;200,800)/(200,800;800,200)");
  EXPECT_EQ (r.angle_check (45.1, 315.0, true).to_string (), "(0,0;1000,1000)/(1000,1000;1000,800);(1000,800;200,800)/(200,800;800,200);(800,0;0,0)/(0,0;1000,1000)");
}

TEST(23)
{
  db::Region r;
  EXPECT_EQ (r.grid_check (10, 20).to_string (), "");

  r.insert (db::Box (db::Point (0, 0), db::Point (1000, 100)));
  r.insert (db::Box (db::Point (0, 100), db::Point (105, 300)));
  r.insert (db::Box (db::Point (910, 100), db::Point (1000, 300)));
  r.insert (db::Box (db::Point (0, 290), db::Point (1000, 500)));

  EXPECT_EQ (r.grid_check (0, 0).to_string (), "");
  EXPECT_EQ (r.grid_check (5, 0).to_string (), "");
  EXPECT_EQ (r.grid_check (0, 10).to_string (), "");
  EXPECT_EQ (r.grid_check (10, 10).to_string (), "(105,100;105,100)/(105,100;105,100);(105,290;105,290)/(105,290;105,290)");
  EXPECT_EQ (r.grid_check (10, 20).to_string (), "(105,100;105,100)/(105,100;105,100);(910,290;910,290)/(910,290;910,290);(105,290;105,290)/(105,290;105,290)");
}

TEST(24)
{
  db::Region r;
  EXPECT_EQ (r.snapped (10, 20).to_string (), "");

  r.insert (db::Box (db::Point (0, 0), db::Point (1000, 100)));
  r.insert (db::Box (db::Point (0, 100), db::Point (105, 300)));
  r.insert (db::Box (db::Point (910, 100), db::Point (1000, 300)));
  r.insert (db::Box (db::Point (0, 290), db::Point (1000, 500)));

  EXPECT_EQ (r.snapped (0, 0).to_string (), "(0,0;0,500;1000,500;1000,0/105,100;910,100;910,290;105,290)");
  EXPECT_EQ (r.snapped (5, 0).to_string (), "(0,0;0,500;1000,500;1000,0/105,100;910,100;910,290;105,290)");
  EXPECT_EQ (r.snapped (0, 10).to_string (), "(0,0;0,500;1000,500;1000,0/105,100;910,100;910,290;105,290)");
  EXPECT_EQ (r.snapped (10, 10).to_string (), "(0,0;0,500;1000,500;1000,0/110,100;910,100;910,290;110,290)");
  EXPECT_EQ (r.snapped (10, 20).to_string (), "(0,0;0,500;1000,500;1000,0/110,100;910,100;910,300;110,300)");
}

TEST(24b)
{
  db::Region r;
  EXPECT_EQ (r.snapped (10, 20).to_string (), "");

  r.insert (db::Box (db::Point (-15, -15), db::Point (15, 15)));

  EXPECT_EQ (r.snapped (10, 10).to_string (), "(-10,-10;-10,20;20,20;20,-10)");
}

TEST(25)
{
  db::Region r;

  r.insert (db::Box (db::Point (0, 0), db::Point (500, 1000)));
  r.insert (db::Box (db::Point (0, -500), db::Point (1000, 0)));

  EXPECT_EQ (r.rounded_corners (50, 100, 16).to_string (), "(80,-500;43,-485;15,-457;0,-420;0,920;15,957;43,985;80,1000;420,1000;457,985;485,957;500,920;500,40;508,22;522,8;540,0;920,0;957,-15;985,-43;1000,-80;1000,-420;985,-457;957,-485;920,-500)");
  db::Region rr = r;
  rr.round_corners (50, 100, 16);
  EXPECT_EQ (r.rounded_corners (50, 100, 16).to_string (), rr.to_string ());
}

TEST(26) 
{
  //  strict mode
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (50, 50), db::Point (150, 250)));

  EXPECT_EQ ((r - db::Region ()).to_string (), "(0,0;0,200;100,200;100,0);(50,50;50,250;150,250;150,50)");
  EXPECT_EQ ((r | db::Region ()).to_string (), "(0,0;0,200;100,200;100,0);(50,50;50,250;150,250;150,50)");
  EXPECT_EQ ((r ^ db::Region ()).to_string (), "(0,0;0,200;100,200;100,0);(50,50;50,250;150,250;150,50)");
  EXPECT_EQ ((db::Region () - r).to_string (), "");
  EXPECT_EQ ((db::Region () | r).to_string (), "(0,0;0,200;100,200;100,0);(50,50;50,250;150,250;150,50)");
  EXPECT_EQ ((db::Region () ^ r).to_string (), "(0,0;0,200;100,200;100,0);(50,50;50,250;150,250;150,50)");
  EXPECT_EQ ((r & db::Region (db::Box (db::Point (20, 20), db::Point (120, 220)))).to_string (), "(20,20;20,200;100,200;100,20);(50,50;50,220;120,220;120,50)");
  EXPECT_EQ ((db::Region (db::Box (db::Point (20, 20), db::Point (120, 220))) & r).to_string (), "(20,20;20,200;100,200;100,20);(50,50;50,220;120,220;120,50)");

  r.set_strict_handling (true);
  EXPECT_EQ ((r - db::Region ()).to_string (), "(0,0;0,200;50,200;50,250;150,250;150,50;100,50;100,0)");
  EXPECT_EQ ((r | db::Region ()).to_string (), "(0,0;0,200;50,200;50,250;150,250;150,50;100,50;100,0)");
  EXPECT_EQ ((r ^ db::Region ()).to_string (), "(0,0;0,200;50,200;50,250;150,250;150,50;100,50;100,0)");
  EXPECT_EQ ((db::Region () - r).to_string (), "");
  EXPECT_EQ ((db::Region () | r).to_string (), "(0,0;0,200;50,200;50,250;150,250;150,50;100,50;100,0)");
  EXPECT_EQ ((db::Region () ^ r).to_string (), "(0,0;0,200;50,200;50,250;150,250;150,50;100,50;100,0)");
  EXPECT_EQ ((r & db::Region (db::Box (db::Point (20, 20), db::Point (120, 220)))).to_string (), "(20,20;20,200;50,200;50,220;120,220;120,50;100,50;100,20)");
  EXPECT_EQ ((db::Region (db::Box (db::Point (20, 20), db::Point (120, 220))) & r).to_string (), "(20,20;20,200;50,200;50,220;120,220;120,50;100,50;100,20)");

  r.set_strict_handling (false);
  EXPECT_EQ ((r - db::Region ()).to_string (), "(0,0;0,200;100,200;100,0);(50,50;50,250;150,250;150,50)");
  EXPECT_EQ ((r | db::Region ()).to_string (), "(0,0;0,200;100,200;100,0);(50,50;50,250;150,250;150,50)");
  EXPECT_EQ ((r ^ db::Region ()).to_string (), "(0,0;0,200;100,200;100,0);(50,50;50,250;150,250;150,50)");
  EXPECT_EQ ((r & db::Region (db::Box (db::Point (20, 20), db::Point (120, 220)))).to_string (), "(20,20;20,200;100,200;100,20);(50,50;50,220;120,220;120,50)");
}

TEST(27) 
{
  //  single box sizing
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));

  EXPECT_EQ (r.sized (db::Coord (-10), db::Coord (-20)).to_string (), "(10,20;10,180;90,180;90,20)");
  EXPECT_EQ (r.sized (db::Coord (-50), db::Coord (-20)).to_string (), "");
  EXPECT_EQ (r.sized (db::Coord (-50), db::Coord (-100)).to_string (), "");
  EXPECT_EQ (r.sized (db::Coord (-55), db::Coord (-20)).to_string (), "");
  EXPECT_EQ (r.sized (db::Coord (-10), db::Coord (-105)).to_string (), "");
  EXPECT_EQ (r.sized (db::Coord (-55), db::Coord (-105)).to_string (), "");
}

TEST(28) 
{
  //  single box intersections
  db::Region r1;
  r1.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));

  db::Region r2;
  r2.insert (db::Box (db::Point (100, 100), db::Point (200, 300)));

  EXPECT_EQ ((r1 & r2).to_string (), "");
  EXPECT_EQ ((r1 & r2.sized (db::Coord (5), db::Coord (5))).to_string (), "(95,95;95,200;100,200;100,95)");
}

TEST(29) 
{
  //  32bit overflow for perimeter
  db::Region b (db::Box (-1000000000, -1000000000, 1000000000, 1000000000));
  EXPECT_EQ (b.perimeter (), 8000000000.0);
}

TEST(30a)
{
  db::Region r;
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (20, 20), db::Point (30, 30)))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (false);
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (20, 20), db::Point (30, 30)))).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_not_interacting (db::Edges (db::Edge (db::Point (20, 20), db::Point (30, 30)))).to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (-20, -20), db::Point (30, 30)))).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (-200, -200), db::Point (-190, -190)))).to_string (), "");
  db::Region rr = r;
  r.select_interacting (db::Edges (db::Edge (db::Point (-20, -20), db::Point (-10, -10))));
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  rr.select_not_interacting (db::Edges (db::Edge (db::Point (-20, -20), db::Point (-10, -10))));
  EXPECT_EQ (rr.to_string (), "(0,0;0,200;100,200;100,0)");

  r.clear ();
  r.insert(db::Box (db::Point (1000, 0), db::Point (6000, 4000)));
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (0, 4000), db::Point (2000, 6000)))).to_string (), "");
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (1000, 4000), db::Point (2000, 6000)))).to_string (), "(1000,0;1000,4000;6000,4000;6000,0)");
  EXPECT_EQ (db::Edges (db::Edge (db::Point (0, 4000), db::Point (2000, 6000))).selected_interacting (r).to_string (), "");
  EXPECT_EQ (db::Edges (db::Edge (db::Point (1000, 4000), db::Point (2000, 6000))).selected_interacting (r).to_string (), "(1000,4000;2000,6000)");
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (1000, 4001), db::Point (2000, 6000)))).to_string (), "");
  EXPECT_EQ (db::Edges (db::Edge (db::Point (1000, 4001), db::Point (2000, 6000))).selected_interacting (r).to_string (), "");
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (1000, 3999), db::Point (1000, 6000)))).to_string (), "(1000,0;1000,4000;6000,4000;6000,0)");
  EXPECT_EQ (db::Edges (db::Edge (db::Point (1000, 3999), db::Point (1000, 6000))).selected_interacting (r).to_string (), "(1000,3999;1000,6000)");
}

TEST(30b)
{
  db::Region r;
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (20, 20), db::Point (30, 30)))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (true);
  r.set_min_coherence (true);
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (20, 20), db::Point (30, 30)))).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (-20, -20), db::Point (30, 30)))).to_string (), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (-200, -200), db::Point (-190, -190)))).to_string (), "");
  r.select_interacting (db::Edges (db::Edge (db::Point (-20, -20), db::Point (-10, -10))));
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,0;0,0;0,-100)");
}

TEST(30c)
{
  db::Region r;
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (20, 20), db::Point (30, 30)))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (true);
  r.set_min_coherence (false);
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (20, 20), db::Point (30, 30)))).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (-20, -20), db::Point (30, 30)))).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Edges (db::Edge (db::Point (-200, -200), db::Point (-190, -190)))).to_string (), "");
  r.select_interacting (db::Edges (db::Edge (db::Point (-20, -20), db::Point (-10, -10))));
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
}

TEST(31)
{
  db::Region r;
  EXPECT_EQ (r.pull_interacting (db::Region (db::Box (db::Point (20, 20), db::Point (30, 30)))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (true);
  r.set_min_coherence (false);
  EXPECT_EQ (r.pull_interacting (db::Region (db::Box (db::Point (20, 20), db::Point (30, 30)))).to_string (), "(20,20;20,30;30,30;30,20)");
  EXPECT_EQ (r.pull_interacting (db::Region (db::Box (db::Point (-20, -20), db::Point (30, 30)))).to_string (), "(-20,-20;-20,30;30,30;30,-20)");
  EXPECT_EQ (r.pull_interacting (db::Region (db::Box (db::Point (-200, -200), db::Point (-190, -190)))).to_string (), "");

  r.clear ();
  r.insert(db::Box (db::Point (1000, 0), db::Point (6000, 4000)));
  EXPECT_EQ (r.pull_overlapping (db::Region (db::Box (db::Point (0, 4000), db::Point (2000, 6000)))).to_string (), "");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 4000), db::Point (2000, 6000))).pull_overlapping (r).to_string (), "");
  EXPECT_EQ (r.pull_overlapping (db::Region (db::Box (db::Point (0, 4000), db::Point (1000, 6000)))).to_string (), "");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 4000), db::Point (1000, 6000))).pull_overlapping (r).to_string (), "");
  EXPECT_EQ (r.pull_overlapping (db::Region (db::Box (db::Point (0, 4001), db::Point (2000, 6000)))).to_string (), "");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 4001), db::Point (2000, 6000))).pull_overlapping (r).to_string (), "");
  EXPECT_EQ (r.pull_overlapping (db::Region (db::Box (db::Point (0, 3999), db::Point (1001, 6000)))).to_string (), "(0,3999;0,6000;1001,6000;1001,3999)");
  EXPECT_EQ (db::Region (db::Box (db::Point (0, 3999), db::Point (1001, 6000))).pull_overlapping (r).to_string (), "(1000,0;1000,4000;6000,4000;6000,0)");
}

TEST(32a_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  r1.set_merged_semantics (false);
  db::Region r2 = r1.snapped (19, 19);

  r2.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/region_au32.gds");
}

TEST(32b_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  r1.set_merged_semantics (false);
  r1.snap (19, 19);

  r1.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/region_au32.gds");
}

TEST(33a_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  r1.set_merged_semantics (false);
  db::Region r2 = r1.scaled_and_snapped (19, 2, 10, 19, 2, 10);

  r2.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/region_au33.gds");
}

TEST(33b_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  r1.set_merged_semantics (false);
  r1.scale_and_snap (19, 2, 10, 19, 2, 10);

  r1.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/region_au33.gds");
}

TEST(34a)
{
  db::Region r;
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (30, 30))))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (false);
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (30, 30))))).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_not_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (30, 30))))).to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  db::Texts tt;
  tt.insert (db::Text ("abc", db::Trans (db::Vector (30, 30))));
  tt.insert (db::Text ("xyz", db::Trans (db::Vector (-100, 0))));
  EXPECT_EQ (r.selected_interacting (tt).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (300, 30))))).to_string (), "");
  db::Region rr = r;
  r.select_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (-10, -10)))));
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  rr.select_not_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (-10, -10)))));
  EXPECT_EQ (rr.to_string (), "(0,0;0,200;100,200;100,0)");

  r.clear ();
  r.insert(db::Box (db::Point (1000, 0), db::Point (6000, 4000)));
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (2000, 6000))))).to_string (), "");
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (2000, 2000))))).to_string (), "(1000,0;1000,4000;6000,4000;6000,0)");
  EXPECT_EQ (db::Texts (db::Text ("abc", db::Trans (db::Vector (2000, 6000)))).selected_interacting (r).to_string (), "");
  EXPECT_EQ (db::Texts (db::Text ("abc", db::Trans (db::Vector (2000, 2000)))).selected_interacting (r).to_string (), "('abc',r0 2000,2000)");
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (2000, 6000))))).to_string (), "");
  EXPECT_EQ (db::Texts (db::Text ("abc", db::Trans (db::Vector (2000, 6000)))).selected_interacting (r).to_string (), "");
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (1000, 2000))))).to_string (), "(1000,0;1000,4000;6000,4000;6000,0)");
  EXPECT_EQ (db::Texts (db::Text ("abc", db::Trans (db::Vector (1000, 2000)))).selected_interacting (r).to_string (), "('abc',r0 1000,2000)");
}

TEST(34b)
{
  db::Region r;
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (30, 30))))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (true);
  r.set_min_coherence (true);
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (30, 30))))).to_string (), "(0,0;0,200;100,200;100,0)");
  db::Texts tt;
  tt.insert (db::Text ("abc", db::Trans (db::Vector (30, 30))));
  tt.insert (db::Text ("xyz", db::Trans (db::Vector (-100, 0))));
  EXPECT_EQ (r.selected_interacting (tt).to_string (), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (-190, -190))))).to_string (), "");
  r.select_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (-10, -10)))));
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,0;0,0;0,-100)");
}

TEST(34c)
{
  db::Region r;
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (30, 30))))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (true);
  r.set_min_coherence (false);
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (30, 30))))).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (0, 0))))).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (-190, -190))))).to_string (), "");
  r.select_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (-10, -10)))));
  EXPECT_EQ (r.to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
}

TEST(34d)
{
  db::Region r;
  EXPECT_EQ (r.pull_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (30, 30))))).to_string (), "");
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));
  r.set_merged_semantics (true);
  r.set_min_coherence (false);
  EXPECT_EQ (r.pull_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (30, 30))))).to_string (), "('abc',r0 30,30)");
  EXPECT_EQ (r.pull_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (0, 0))))).to_string (), "('abc',r0 0,0)");
  EXPECT_EQ (r.pull_interacting (db::Texts (db::Text ("abc", db::Trans (db::Vector (-190, -190))))).to_string (), "");
}

TEST(100_Processors)
{
  db::Region r;
  r.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  r.insert (db::Box (db::Point (0, 300), db::Point (200, 400)));
  r.insert (db::Box (db::Point (0, 300), db::Point (200, 400)));
  r.insert (db::Box (db::Point (100, 300), db::Point (200, 500)));

  EXPECT_EQ (r.processed (db::CornersAsDots (-180.0, 180.0)).to_string (), "(100,0;100,0);(0,0;0,0);(0,200;0,200);(100,200;100,200);(200,300;200,300);(0,300;0,300);(0,400;0,400);(100,400;100,400);(100,500;100,500);(200,500;200,500)");
  EXPECT_EQ (r.processed (db::CornersAsDots (0.0, 180.0)).to_string (), "(100,400;100,400)");
  db::Region ext;
  r.processed (db::CornersAsDots (0.0, 180.0)).extended (ext, 10, 10, 20, 20);
  EXPECT_EQ (ext.to_string (), "(90,380;90,420;110,420;110,380)");
  EXPECT_EQ (r.processed (db::CornersAsRectangles (-180.0, 180.0, 2)).to_string (), "(98,-2;98,2;102,2;102,-2);(-2,-2;-2,2;2,2;2,-2);(-2,198;-2,202;2,202;2,198);(98,198;98,202;102,202;102,198);(198,298;198,302;202,302;202,298);(-2,298;-2,302;2,302;2,298);(-2,398;-2,402;2,402;2,398);(98,398;98,402;102,402;102,398);(98,498;98,502;102,502;102,498);(198,498;198,502;202,502;202,498)");
  EXPECT_EQ (r.processed (db::CornersAsRectangles (0.0, 180.0, 2)).to_string (), "(98,398;98,402;102,402;102,398)");

  EXPECT_EQ (r.processed (db::extents_processor<db::Polygon> (0, 0)).to_string (), "(0,0;0,200;100,200;100,0);(0,300;0,500;200,500;200,300)");
  EXPECT_EQ (r.processed (db::extents_processor<db::Polygon> (10, 20)).to_string (), "(-10,-20;-10,220;110,220;110,-20);(-10,280;-10,520;210,520;210,280)");
  EXPECT_EQ (r.processed (db::RelativeExtents (0, 0, 1.0, 1.0, 0, 0)).to_string (), "(0,0;0,200;100,200;100,0);(0,300;0,500;200,500;200,300)");
  EXPECT_EQ (r.processed (db::RelativeExtents (0.25, 0.4, 0.75, 0.6, 10, 20)).to_string (), "(15,60;15,140;85,140;85,60);(40,360;40,440;160,440;160,360)");
  EXPECT_EQ (r.processed (db::RelativeExtentsAsEdges (0, 0, 1.0, 1.0)).to_string (), "(0,0;100,200);(0,300;200,500)");
  EXPECT_EQ (r.processed (db::RelativeExtentsAsEdges (0.5, 0.5, 0.5, 0.5)).to_string (), "(50,100;50,100);(100,400;100,400)");
  EXPECT_EQ (r.processed (db::RelativeExtentsAsEdges (0.25, 0.4, 0.75, 0.6)).to_string (), "(25,80;75,120);(50,380;150,420)");

  EXPECT_EQ (r.processed (db::minkowsky_sum_computation<db::Box> (db::Box (-10, -20, 30, 40))).to_string (), "(-10,-20;-10,240;130,240;130,-20);(-10,280;-10,440;90,440;90,540;230,540;230,280)");
  EXPECT_EQ (r.processed (db::minkowsky_sum_computation<db::Edge> (db::Edge (-10, 0, 30, 0))).to_string (), "(-10,0;-10,200;130,200;130,0);(-10,300;-10,400;90,400;90,500;230,500;230,300)");

  EXPECT_EQ (r.processed (db::TrapezoidDecomposition (db::TD_htrapezoids)).to_string (), "(0,0;0,200;100,200;100,0);(100,300;100,500;200,500;200,300);(0,300;0,400;100,400;100,300)");
  EXPECT_EQ (r.processed (db::ConvexDecomposition (db::PO_vertical)).to_string (),       "(0,0;0,200;100,200;100,0);(100,300;100,500;200,500;200,300);(0,300;0,400;100,400;100,300)");
  EXPECT_EQ (r.processed (db::ConvexDecomposition (db::PO_horizontal)).to_string (),     "(0,0;0,200;100,200;100,0);(100,400;100,500;200,500;200,400);(0,300;0,400;200,400;200,300)");
}

TEST(issue_228)
{
  db::Region r;
  db::Point pts[] = {
    db::Point (0, 10),
    db::Point (0, 290),
    db::Point (280, 290),
    db::Point (280, 230),
    db::Point (360, 230),
    db::Point (360, 70),
    db::Point (280,70),
    db::Point (280,10)
  };

  db::Polygon poly;
  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts [0]));
  r.insert (poly);

  db::Region rr;
  rr.insert (db::Box (360, 70, 480, 230));

  EXPECT_EQ (r.selected_interacting (rr).to_string (), r.to_string ());
  EXPECT_EQ (rr.selected_interacting (r).to_string (), rr.to_string ());
}

TEST(issue_277)
{
  db::Region r;
  r.insert (db::Box (0, 0, 400, 400));
  r.insert (db::Box (400, 400, 800, 800));

  EXPECT_EQ (r.sized (1).merged (false, 1).to_string (), "");

  r.set_min_coherence (true);
  EXPECT_EQ (r.sized (1).merged (false, 1).to_string (), "(399,399;399,401;401,401;401,399)");

  r.merge ();
  EXPECT_EQ (r.sized (1).merged (false, 1).to_string (), "(399,399;399,401;401,401;401,399)");

  r.set_min_coherence (false);  //  needs to merge again
  EXPECT_EQ (r.sized (1).merged (false, 1).to_string (), "");
}

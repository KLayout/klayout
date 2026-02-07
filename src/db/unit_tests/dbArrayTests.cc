
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


#include "dbArray.h"
#include "dbBoxConvert.h"
#include "tlUnitTest.h"

typedef db::array <db::Box, db::Trans> BoxArray;

class MyBoxConvert
  : public db::box_convert<db::Box> 
{
  // ...
};

std::string positions (const BoxArray &arr, const db::Point &pt, BoxArray::iterator it, db::ICplxTrans tr = db::ICplxTrans ())
{
  bool first = true;
  std::string s;
  while (! it.at_end ()) {
    if (! first) {
      s += ";";
    }
    first = false;
    s += (tr * (arr.complex_trans (*it) * pt)).to_string ();
    ++it;
  }
  return s;
}

TEST(1) 
{
  db::Vector a (0, 100);
  db::Vector b (200, 0);
  BoxArray ba (db::Box (10, 30, 30, 40), db::Trans (db::Vector (0, 0)), new db::regular_array<db::Coord> (a, b, 2, 3));

  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin ()), "0,0;0,100;200,0;200,100;400,0;400,100");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 0, 200, 100), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 50, 200, 110), MyBoxConvert ())), "");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 40, 200, 110), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 40, 200, 130), MyBoxConvert ())), "0,0;0,100");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 40, 209, 130), MyBoxConvert ())), "0,0;0,100");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 40, 210, 130), MyBoxConvert ())), "0,0;0,100;200,0;200,100");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 40, 409, 130), MyBoxConvert ())), "0,0;0,100;200,0;200,100");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 40, 410, 130), MyBoxConvert ())), "0,0;0,100;200,0;200,100;400,0;400,100");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (30, 40, 210, 130), MyBoxConvert ())), "0,0;0,100;200,0;200,100");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (31, 40, 210, 130), MyBoxConvert ())), "200,0;200,100");
}

TEST(2) 
{
  db::Vector a (10, 100);
  db::Vector b (-200, 20);
  BoxArray ba (db::Box (10, -20, 30, -10), db::Trans (db::Vector (0, 50)), new db::regular_array<db::Coord> (a, b, 2, 3));

  EXPECT_EQ (positions (ba, db::Point (0, -50), ba.begin ()), "0,0;10,100;-200,20;-190,120;-400,40;-390,140");
  EXPECT_EQ (positions (ba, db::Point (0, -50), ba.begin_touching (db::Box (-180, 50, 10, 160), MyBoxConvert ())), "-200,20;-190,120");
  EXPECT_EQ (positions (ba, db::Point (0, -50), ba.begin_touching (db::Box (-180, 10, 10, 160), MyBoxConvert ())), "0,0;10,100;-200,20;-190,120");
  EXPECT_EQ (positions (ba, db::Point (0, -50), ba.begin_touching (db::Box (-180, 50, 10, 130), MyBoxConvert ())), "-200,20");
  EXPECT_EQ (positions (ba, db::Point (0, -50), ba.begin_touching (db::Box (), MyBoxConvert ())), "");

  ba.transform (db::Trans (db::Vector (10, -10)), 0);
  EXPECT_EQ (positions (ba, db::Point (-10, -40), ba.begin ()), "0,0;10,100;-200,20;-190,120;-400,40;-390,140");
  EXPECT_EQ (ba.is_complex (), false);

  BoxArray bba (ba);
  bba.transform (db::Trans (db::FTrans::m90), 0);
  EXPECT_EQ (positions (bba, db::Point (-10, -40), bba.begin (), db::ICplxTrans (db::FTrans::m90)), "0,0;10,100;-200,20;-190,120;-400,40;-390,140");
  EXPECT_EQ (bba.is_complex (), false);

  db::ICplxTrans t1 = db::ICplxTrans (db::Trans (db::FTrans::r90));
  db::ICplxTrans t2 = db::ICplxTrans (2.0, 45.0, false, db::Vector ());

  ba.transform (t1, 0);
  EXPECT_EQ (ba.is_complex (), false);
  EXPECT_EQ (positions (ba, db::Point (-10, -40), ba.begin (), t1.inverted ()), "0,0;10,100;-200,20;-190,120;-400,40;-390,140");

  ba.transform (t2, 0);
  EXPECT_EQ (ba.is_complex (), true);
  EXPECT_EQ (positions (ba, db::Point (-10, -40), ba.begin (), (t2 * t1).inverted ()), "0,0;10,100;-200,20;-190,120;-400,40;-390,140");
}

TEST(3) 
{
  db::Vector a (10, 100);
  db::Vector b (-200, 20);
  BoxArray ba = BoxArray (db::Box (), db::Trans (db::Vector (0, 0)), new db::regular_array<db::Coord> (a, b, 2, 3));

  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin ()), "0,0;10,100;-200,20;-190,120;-400,40;-390,140");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (-180, 10, 10, 160), MyBoxConvert ())), "");
}

TEST(4) 
{
  db::Vector a (10, 100);
  db::Vector b (-200, 20);
  BoxArray ba (db::Box (80, 0, 240, 80), db::Trans (db::Vector (0, 30)), new db::regular_complex_array<db::Coord> (1.0, 0.125, a, b, 2, 3));

  std::vector<db::Vector> v;
  EXPECT_EQ (ba.is_iterated_array (&v), false);

  EXPECT_EQ (positions (ba, db::Point (0, -240), ba.begin ()), "0,0;10,100;-200,20;-190,120;-400,40;-390,140");
  EXPECT_EQ (positions (ba, db::Point (0, -240), ba.begin_touching (db::Box (-180, 50, 10, 160), MyBoxConvert ())), "-200,20;-190,120");
  EXPECT_EQ (positions (ba, db::Point (0, -240), ba.begin_touching (db::Box (-180, 10, 10, 160), MyBoxConvert ())), "0,0;10,100;-200,20;-190,120");
  EXPECT_EQ (positions (ba, db::Point (0, -240), ba.begin_touching (db::Box (-180, 50, 10, 130), MyBoxConvert ())), "-200,20");
  EXPECT_EQ (positions (ba, db::Point (0, -240), ba.begin_touching (db::Box (), MyBoxConvert ())), "");

  BoxArray ba2 (ba);
  ba2.invert ();
  EXPECT_EQ (ba.size (), ba2.size ());
  for (BoxArray::iterator i1 = ba.begin (), i2 = ba2.begin (); ! i1.at_end (); ++i1, ++i2) {
    db::Point p = (ba2.complex_trans (*i2) * ba.complex_trans (*i1)) * db::Point (1000, 1000);
    EXPECT_EQ (p.to_string (), "1000,1000");
  }
}

TEST(4a) 
{
  db::Vector a (10, 100);
  db::Vector b (-200, 20);
  BoxArray ba (db::Box (80, 0, 240, 80), db::Trans (db::Vector (0, 30)), new db::regular_complex_array<db::Coord> (sqrt(0.5), 1.0, a, b, 2, 3));

  EXPECT_EQ (ba.complex_trans().to_string(), "r45 *1 0,30");
  EXPECT_EQ (positions (ba, db::Point (0, 100), ba.begin ()), "-71,101;-61,201;-271,121;-261,221;-471,141;-461,241");

  ba.transform (db::ICplxTrans (1.0, 45.0, false, db::Vector (21, 9)), 0);

  EXPECT_EQ (ba.complex_trans().to_string(), "r90 *1 0,30");
  EXPECT_EQ (positions (ba, db::Point (0, 100), ba.begin ()), "-100,30;-164,108;-256,-97;-320,-19;-412,-224;-476,-146");
}

TEST(4b) 
{
  BoxArray ba (db::Box (80, 0, 240, 80), db::Trans (db::Vector (0, 30)), new db::single_complex_inst<db::Coord> (sqrt(0.5), 1.0));

  std::vector<db::Vector> v;
  EXPECT_EQ (ba.is_iterated_array (&v), false);

  EXPECT_EQ (ba.complex_trans().to_string(), "r45 *1 0,30");
  EXPECT_EQ (positions (ba, db::Point (0, 100), ba.begin ()), "-71,101");

  ba.transform (db::ICplxTrans (1.0, 45.0, false, db::Vector (21, 9)), 0);

  EXPECT_EQ (ba.complex_trans().to_string(), "r90 *1 0,30");
  EXPECT_EQ (positions (ba, db::Point (0, 100), ba.begin ()), "-100,30");
}

TEST(5) 
{
  BoxArray ba (db::Box (-9, 3, -7, 4), db::Trans (db::Vector (100, 0)), new db::single_complex_inst<db::Coord> (1.0, 10.0));

  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin ()), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin_touching (db::Box (0, 0, 200, 100), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin_touching (db::Box (0, 50, 200, 110), MyBoxConvert ())), "");
  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin_touching (db::Box (0, 40, 200, 110), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin_touching (db::Box (0, 40, 200, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin_touching (db::Box (0, 40, 209, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin_touching (db::Box (0, 40, 210, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin_touching (db::Box (0, 40, 409, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin_touching (db::Box (0, 40, 410, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin_touching (db::Box (30, 40, 210, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-10, 0), ba.begin_touching (db::Box (31, 40, 210, 130), MyBoxConvert ())), "");
}

TEST(6) 
{
  BoxArray ba (db::Box (-90, 0, -70, 10), db::Trans (db::Vector (100, 30)));

  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin ()), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin_touching (db::Box (0, 0, 200, 100), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin_touching (db::Box (0, 50, 200, 110), MyBoxConvert ())), "");
  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin_touching (db::Box (0, 40, 200, 110), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin_touching (db::Box (0, 40, 200, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin_touching (db::Box (0, 40, 209, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin_touching (db::Box (0, 40, 210, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin_touching (db::Box (0, 40, 409, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin_touching (db::Box (0, 40, 410, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin_touching (db::Box (30, 40, 210, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (-100, -30), ba.begin_touching (db::Box (31, 40, 210, 130), MyBoxConvert ())), "");

  ba.transform (db::Trans (db::Vector (10, -10)), 0);
  EXPECT_EQ (positions (ba, db::Point (-110, -20), ba.begin ()), "0,0");
  EXPECT_EQ (ba.is_complex (), false);

  BoxArray bba (ba);
  bba.transform (db::Trans (db::FTrans::m90), 0);
  EXPECT_EQ (positions (bba, db::Point (-110, -20), bba.begin (), db::ICplxTrans (db::FTrans::m90)), "0,0");
  EXPECT_EQ (bba.is_complex (), false);

  db::ICplxTrans t1 = db::ICplxTrans (db::Trans (db::FTrans::r90));
  db::ICplxTrans t2 = db::ICplxTrans (2.0, 45.0, false, db::Vector ());

  ba.transform (t1, 0);
  EXPECT_EQ (ba.is_complex (), false);
  EXPECT_EQ (positions (ba, db::Point (-110, -20), ba.begin (), t1.inverted ()), "0,0");

  ba.transform (t2, 0);
  EXPECT_EQ (ba.is_complex (), true);
  EXPECT_EQ (positions (ba, db::Point (-110, -20), ba.begin (), (t2 * t1).inverted ()), "0,0");

  BoxArray ba2 (ba);
  ba2.invert ();
  EXPECT_EQ (ba.size (), ba2.size ());
  for (BoxArray::iterator i1 = ba.begin (), i2 = ba2.begin (); ! i1.at_end (); ++i1, ++i2) {
    db::Point p = (ba2.complex_trans (*i2) * ba.complex_trans (*i1)) * db::Point (1000, 1000);
    EXPECT_EQ (p.to_string (), "1000,1000");
  }
}

TEST(7) 
{
  db::Vector a (0, 100);
  db::Vector b (200, 0);

  db::ArrayRepository rep;
  BoxArray ba1 (db::Box (10, 30, 30, 40), db::Trans (db::Vector (0, 0)), rep.insert (db::regular_array<db::Coord> (a, b, 2, 3)));
  BoxArray ba1dup (db::Box (-10, 30, -30, 40), db::Trans (db::Vector (0, 123)), rep.insert (db::regular_array<db::Coord> (a, b, 2, 3)));
  BoxArray ba2 (db::Box (-9, 3, -7, 4), db::Trans (db::Vector (100, 0)), rep.insert (db::single_complex_inst<db::Coord> (1.0, 10.0)));
  BoxArray ba2dup (db::Box (0, 0, 1000, 2000), db::Trans (db::Vector (-100, 0)), rep.insert (db::single_complex_inst<db::Coord> (1.0, 10.0)));

  BoxArray cpy1 (ba1);
  BoxArray cpy2;
  cpy2 = cpy1;

  EXPECT_EQ (ba1.delegate () == ba1dup.delegate (), true);
  EXPECT_EQ (ba2.delegate () == ba2dup.delegate (), true);
  EXPECT_EQ (ba1.delegate () != ba2.delegate (), true);

  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin ()), "0,0;0,100;200,0;200,100;400,0;400,100");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin_touching (db::Box (0, 0, 200, 100), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin_touching (db::Box (0, 50, 200, 110), MyBoxConvert ())), "");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin_touching (db::Box (0, 40, 200, 110), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin_touching (db::Box (0, 40, 200, 130), MyBoxConvert ())), "0,0;0,100");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin_touching (db::Box (0, 40, 209, 130), MyBoxConvert ())), "0,0;0,100");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin_touching (db::Box (0, 40, 210, 130), MyBoxConvert ())), "0,0;0,100;200,0;200,100");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin_touching (db::Box (0, 40, 409, 130), MyBoxConvert ())), "0,0;0,100;200,0;200,100");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin_touching (db::Box (0, 40, 410, 130), MyBoxConvert ())), "0,0;0,100;200,0;200,100;400,0;400,100");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin_touching (db::Box (30, 40, 210, 130), MyBoxConvert ())), "0,0;0,100;200,0;200,100");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin_touching (db::Box (31, 40, 210, 130), MyBoxConvert ())), "200,0;200,100");

  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin ()), "0,0");
  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin_touching (db::Box (0, 0, 200, 100), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin_touching (db::Box (0, 50, 200, 110), MyBoxConvert ())), "");
  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin_touching (db::Box (0, 40, 200, 110), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin_touching (db::Box (0, 40, 200, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin_touching (db::Box (0, 40, 209, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin_touching (db::Box (0, 40, 210, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin_touching (db::Box (0, 40, 409, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin_touching (db::Box (0, 40, 410, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin_touching (db::Box (30, 40, 210, 130), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba2, db::Point (-10, 0), ba2.begin_touching (db::Box (31, 40, 210, 130), MyBoxConvert ())), "");

  BoxArray ba1copy (ba1);
  EXPECT_EQ (positions (ba1copy, db::Point (0, 0), ba1copy.begin ()), "0,0;0,100;200,0;200,100;400,0;400,100");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin ()), "0,0;0,100;200,0;200,100;400,0;400,100");
  ba1copy.invert ();
  EXPECT_EQ (positions (ba1copy, db::Point (0, 0), ba1copy.begin ()), "0,0;0,-100;-200,0;-200,-100;-400,0;-400,-100");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin ()), "0,0;0,100;200,0;200,100;400,0;400,100");
  ba1copy = ba1;
  ba1.invert ();
  EXPECT_EQ (positions (ba1copy, db::Point (0, 0), ba1copy.begin ()), "0,0;0,100;200,0;200,100;400,0;400,100");
  EXPECT_EQ (positions (ba1, db::Point (0, 0), ba1.begin ()), "0,0;0,-100;-200,0;-200,-100;-400,0;-400,-100");
}

TEST(8) 
{
  db::Vector a (0, 100);
  db::Vector b (200, 0);
  BoxArray ba;
  ba = BoxArray (db::Box (10, 30, 30, 40), db::Trans (db::Vector (-65, 25)));
  EXPECT_EQ (ba.bbox (db::box_convert<db::Box> ()).to_string (), "(-55,55;-35,65)");
  EXPECT_EQ (ba.bbox_from_raw_bbox (ba.raw_bbox (), db::box_convert<db::Box> ()).to_string (), "(-55,55;-35,65)");
  ba = BoxArray (db::Box (10, 30, 30, 40), db::Trans (db::Vector (-65, 25)), new db::regular_array<db::Coord> (a, b, 2, 3));
  EXPECT_EQ (ba.bbox (db::box_convert<db::Box> ()).to_string (), "(-55,55;365,165)");
  EXPECT_EQ (ba.bbox_from_raw_bbox (ba.raw_bbox (), db::box_convert<db::Box> ()).to_string (), "(-55,55;365,165)");
  ba = BoxArray (db::Box (-9, 3, -7, 4), db::Trans (db::Vector (100, 0)), new db::single_complex_inst<db::Coord> (1.0, 10.0));
  EXPECT_EQ (ba.bbox (db::box_convert<db::Box> ()).to_string (), "(10,30;30,40)");
  EXPECT_EQ (ba.bbox_from_raw_bbox (ba.raw_bbox (), db::box_convert<db::Box> ()).to_string (), "(10,30;30,40)");
  ba = BoxArray (db::Box (10, 30, 30, 40), db::Trans (db::Vector (-65, 25)), new db::regular_complex_array<db::Coord> (1.0, 10.0, a, b, 2, 3));
  EXPECT_EQ (ba.bbox (db::box_convert<db::Box> ()).to_string (), "(35,325;635,525)");
  EXPECT_EQ (ba.bbox_from_raw_bbox (ba.raw_bbox (), db::box_convert<db::Box> ()).to_string (), "(35,325;635,525)");
  ba = BoxArray (db::Box (10, 30, 30, 40), db::Trans (1 /*90 degree*/, true /*mirror*/, db::Vector (-65, 25)), new db::regular_complex_array<db::Coord> (sqrt (0.5) /*45degree*/, 10.0, a, b, 2, 3));
  EXPECT_EQ (ba.bbox (db::box_convert<db::Box> ()).to_string (), "(-65,308;547,620)");
  EXPECT_EQ (ba.bbox_from_raw_bbox (ba.raw_bbox (), db::box_convert<db::Box> ()).to_string (), "(-65,308;547,620)");
}

TEST(9)
{
  db::Vector a1 (100, 500);
  db::Vector a2 (-100, 200);
  db::Vector a3 (-200, -100);

  db::iterated_array<db::Coord> *ia = new db::iterated_array<db::Coord> ();
  ia->insert (a1);
  ia->insert (a2);
  ia->insert (a3);
  ia->sort ();

  BoxArray ba (db::Box (10, 10, 50, 50), db::Trans (db::Vector (10, 10)), ia);
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin ()), "110,510;-90,210;-190,-90");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (-200, -1000, 200, 1000), MyBoxConvert ())), 
    "110,510;-90,210;-190,-90");

  BoxArray ba2 (ba);
  EXPECT_EQ (ba2 == ba, true);
  EXPECT_EQ (ba2 < ba, false);

  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin ()), "110,510;-90,210;-190,-90");
  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin_touching (db::Box (-200, -1000, 200, 1000), MyBoxConvert ())), 
    "110,510;-90,210;-190,-90");
  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin_touching (db::Box (0, 0, 200, 1000), MyBoxConvert ())), 
    "110,510");
  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin_touching (db::Box (0, 0, 200, 100), MyBoxConvert ())), 
    "");
  EXPECT_EQ (ba2.bbox (db::box_convert<db::Box> ()).to_string (), "(-180,-80;160,560)");

  ba2.invert ();
  EXPECT_EQ (ba2 == ba, false);
  EXPECT_EQ (ba2 < ba, ! (ba < ba2));
  
  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin ()), "-110,-510;90,-210;190,90");
  EXPECT_EQ (ba2.bbox (db::box_convert<db::Box> ()).to_string (), "(-100,-500;240,140)");

  ba2 = ba;
  ba.transform (db::Trans (db::Vector (-10, -10)), 0);
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin ()), "100,500;-100,200;-200,-100");

  ba2.transform (db::ICplxTrans (db::Trans (db::Vector (-10, -10))), 0);
  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin ()), "100,500;-100,200;-200,-100");
}

TEST(10)
{
  db::Vector a1 (100, 500);
  db::Vector a2 (-100, 200);
  db::Vector a3 (-200, -100);

  db::iterated_complex_array<db::Coord> *ia = new db::iterated_complex_array<db::Coord> (1.0, 2.0);
  ia->insert (a1);
  ia->insert (a2);
  ia->insert (a3);
  ia->sort ();

  BoxArray ba (db::Box (10, 10, 50, 50), db::Trans (db::Vector (10, 10)), ia);
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin ()), "110,510;-90,210;-190,-90");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (-200, -1000, 200, 1000), MyBoxConvert ())), 
    "110,510;-90,210;-190,-90");

  EXPECT_EQ (ba.is_complex (), true);
  std::vector<db::Vector> v;
  EXPECT_EQ (ba.is_iterated_array (&v), true);

  BoxArray ba2 (ba);
  EXPECT_EQ (ba2 == ba, true);
  EXPECT_EQ (ba2 < ba, false);

  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin ()), "110,510;-90,210;-190,-90");
  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin_touching (db::Box (-200, -1000, 200, 1000), MyBoxConvert ())), 
    "110,510;-90,210;-190,-90");
  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin_touching (db::Box (0, 0, 200, 1000), MyBoxConvert ())), 
    "110,510;-90,210");
  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin_touching (db::Box (0, 0, 200, 100), MyBoxConvert ())), 
    "");
  EXPECT_EQ (ba2.bbox (db::box_convert<db::Box> ()).to_string (), "(-170,-70;210,610)");

  ba2.invert ();
  EXPECT_EQ (ba2 == ba, false);
  EXPECT_EQ (ba2 < ba, ! (ba < ba2));
  
  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin ()), "-55,-255;45,-105;95,45");
  EXPECT_EQ (ba2.bbox (db::box_convert<db::Box> ()).to_string (), "(-50,-250;120,70)");

  EXPECT_EQ (ba.size (), ba2.size ());
  for (BoxArray::iterator i1 = ba.begin (), i2 = ba2.begin (); ! i1.at_end (); ++i1, ++i2) {
    db::Point p = (ba2.complex_trans (*i2) * ba.complex_trans (*i1)) * db::Point (1000, 1000);
    EXPECT_EQ (p.to_string (), "1000,1000");
  }

  ba2 = ba;
  ba.transform (db::Trans (db::Vector (-10, -10)), 0);
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin ()), "100,500;-100,200;-200,-100");

  ba2.transform (db::ICplxTrans (db::Trans (db::Vector (-10, -10))), 0);
  EXPECT_EQ (positions (ba2, db::Point (0, 0), ba2.begin ()), "100,500;-100,200;-200,-100");
}

TEST(11)
{
  BoxArray ba1 (db::Box (10, 10, 50, 50), db::Trans (db::Vector (10, 10)));
  BoxArray ba2x3 (db::Box (10, 10, 50, 50), db::Trans (db::Vector (10, 20)), db::Vector (0, 1), db::Vector (1, 0), 2, 3);
  BoxArray ba1cplx (db::Box (10, 10, 50, 50), db::Trans (db::Vector (10, 30)), 0.6, 0.5);
  BoxArray ba1cplx2 (db::Box (10, 10, 50, 50), db::Trans (db::Vector (10, 30)), 0.6, 0.6);
  BoxArray ba2x3cplx (db::Box (10, 10, 50, 50), db::Trans (db::Vector (20, 20)), 0.5, 2.5, db::Vector (0, 1), db::Vector (1, 0), 2, 3);
  BoxArray ba2x3cplx2 (db::Box (10, 10, 50, 50), db::Trans (db::Vector (20, 20)), 0.6, 2.5, db::Vector (0, 1), db::Vector (1, 0), 2, 3);

  EXPECT_EQ (ba1 < ba2x3, true);
  EXPECT_EQ (ba1 == ba2x3, false);
  EXPECT_EQ (ba1 < ba1, false);
  EXPECT_EQ (ba1 == ba1, true);
  EXPECT_EQ (ba2x3 < ba2x3, false);
  EXPECT_EQ (ba2x3 == ba2x3, true);
  EXPECT_EQ (ba2x3 < ba1, false);
  EXPECT_EQ (ba2x3 == ba1, false);
  EXPECT_EQ (ba1 < ba1cplx, true);
  EXPECT_EQ (ba1 == ba1cplx, false);
  EXPECT_EQ (ba1 < ba1, false);
  EXPECT_EQ (ba1 == ba1, true);
  EXPECT_EQ (ba1cplx < ba1cplx, false);
  EXPECT_EQ (ba1cplx == ba1cplx, true);
  EXPECT_EQ (ba1cplx < ba1cplx2, true);
  EXPECT_EQ (ba1cplx == ba1cplx2, false);
  EXPECT_EQ (ba1cplx2 < ba1cplx, false);
  EXPECT_EQ (ba1cplx2 == ba1cplx, false);
  EXPECT_EQ (ba1cplx2 < ba1cplx2, false);
  EXPECT_EQ (ba1cplx2 == ba1cplx2, true);
  EXPECT_EQ (ba1cplx < ba1, false);
  EXPECT_EQ (ba1cplx == ba1, false);
  EXPECT_EQ (ba1 < ba2x3cplx, true);
  EXPECT_EQ (ba1 == ba2x3cplx, false);
  EXPECT_EQ (ba1 < ba1, false);
  EXPECT_EQ (ba1 == ba1, true);
  EXPECT_EQ (ba2x3cplx < ba2x3cplx, false);
  EXPECT_EQ (ba2x3cplx == ba2x3cplx, true);
  EXPECT_EQ (ba2x3cplx < ba2x3cplx2, true);
  EXPECT_EQ (ba2x3cplx == ba2x3cplx2, false);
  EXPECT_EQ (ba2x3cplx2 < ba2x3cplx, false);
  EXPECT_EQ (ba2x3cplx2 == ba2x3cplx, false);
  EXPECT_EQ (ba2x3cplx2 < ba2x3cplx2, false);
  EXPECT_EQ (ba2x3cplx2 == ba2x3cplx2, true);
  EXPECT_EQ (ba2x3cplx < ba1, false);
  EXPECT_EQ (ba2x3cplx == ba1, false);
  EXPECT_EQ (ba2x3 < ba1cplx, true);
  EXPECT_EQ (ba2x3 == ba1cplx, false);
  EXPECT_EQ (ba1cplx < ba2x3, false);
  EXPECT_EQ (ba1cplx == ba2x3, false);
  EXPECT_EQ (ba2x3 < ba2x3cplx, true);
  EXPECT_EQ (ba2x3 == ba2x3cplx, false);
  EXPECT_EQ (ba2x3cplx < ba2x3, false);
  EXPECT_EQ (ba2x3cplx == ba2x3, false);
  EXPECT_EQ (ba2x3cplx < ba1cplx, true);
  EXPECT_EQ (ba2x3cplx == ba1cplx, false);
  EXPECT_EQ (ba1cplx < ba2x3cplx, false);
  EXPECT_EQ (ba1cplx == ba2x3cplx, false);
}

TEST(12_1dArraysX)
{
  db::Vector a (0, 0);
  db::Vector b (200, 0);
  BoxArray ba (db::Box (10, 30, 30, 40), db::Trans (db::Vector (0, 0)), new db::regular_array<db::Coord> (a, b, 1, 3));

  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin ()), "0,0;200,0;400,0");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 0, 200, 100), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 50, 200, 110), MyBoxConvert ())), "");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 40, 200, 110), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (31, 40, 210, 130), MyBoxConvert ())), "200,0");

  ba = BoxArray (db::Box (10, 30, 30, 40), db::Trans (db::Vector (0, 0)), new db::regular_array<db::Coord> (b, a, 3, 1));

  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin ()), "0,0;200,0;400,0");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 0, 200, 100), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 50, 200, 110), MyBoxConvert ())), "");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (0, 40, 200, 110), MyBoxConvert ())), "0,0");
  EXPECT_EQ (positions (ba, db::Point (0, 0), ba.begin_touching (db::Box (31, 40, 210, 130), MyBoxConvert ())), "200,0");
}

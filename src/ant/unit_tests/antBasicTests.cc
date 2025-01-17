
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
#include "antObject.h"
#include "antTemplate.h"

//  NOTE: most tests are in ruby/antTest.rb

TEST(1) 
{
  ant::Template tmp = ant::Template ("title", "fmt_x", "fmt_y", "fmt",
                                     ant::Object::STY_arrow_both,
                                     ant::Object::OL_diag_xy,
                                     true,
                                     lay::AC_Ortho,
                                     "cat");

  ant::Object a = ant::Object (db::DPoint (1.0, 2.0), db::DPoint (3.0, 4.0), 17, tmp);

  EXPECT_EQ (a.fmt (), "fmt");
  EXPECT_EQ (a.fmt_x (), "fmt_x");
  EXPECT_EQ (a.fmt_y (), "fmt_y");
  EXPECT_EQ (a.box ().to_string (), "(1,2;3,4)");
  EXPECT_EQ (a.p1 ().to_string (), "1,2");
  EXPECT_EQ (a.p2 ().to_string (), "3,4");
  EXPECT_EQ (a.angle_constraint (), lay::AC_Ortho);
  EXPECT_EQ (a.id (), 17);
  EXPECT_EQ (a.snap (), true);
  EXPECT_EQ (a.category (), "cat");
}

TEST(2)
{
  ant::Template tmp = ant::Template ("title", "fmt_x", "fmt_y", "fmt",
                                     ant::Object::STY_arrow_both,
                                     ant::Object::OL_diag_xy,
                                     true,
                                     lay::AC_Ortho,
                                     "cat");

  ant::Object obj;

  EXPECT_EQ (obj.p1 ().to_string (), "0,0");
  EXPECT_EQ (obj.p2 ().to_string (), "0,0");

  EXPECT_EQ (int (obj.segments ()), 1);
  EXPECT_EQ (int (obj.points ().size ()), 0);

  obj.p1 (db::DPoint (1, 2));

  EXPECT_EQ (obj.p1 ().to_string (), "1,2");
  EXPECT_EQ (obj.p2 ().to_string (), "1,2");

  EXPECT_EQ (int (obj.segments ()), 1);
  EXPECT_EQ (int (obj.points ().size ()), 1);

  obj.p2 (db::DPoint (2, 3));

  EXPECT_EQ (obj.p1 ().to_string (), "1,2");
  EXPECT_EQ (obj.p2 ().to_string (), "2,3");

  EXPECT_EQ (int (obj.segments ()), 1);
  EXPECT_EQ (int (obj.points ().size ()), 2);

  obj = ant::Object ();

  EXPECT_EQ (obj.p1 ().to_string (), "0,0");
  EXPECT_EQ (obj.p2 ().to_string (), "0,0");

  EXPECT_EQ (int (obj.segments ()), 1);
  EXPECT_EQ (int (obj.points ().size ()), 0);

  obj.p1 (db::DPoint ());

  EXPECT_EQ (obj.p1 ().to_string (), "0,0");
  EXPECT_EQ (obj.p2 ().to_string (), "0,0");

  EXPECT_EQ (int (obj.segments ()), 1);
  EXPECT_EQ (int (obj.points ().size ()), 1);

  obj.p2 (db::DPoint ());

  EXPECT_EQ (obj.p1 ().to_string (), "0,0");
  EXPECT_EQ (obj.p2 ().to_string (), "0,0");

  EXPECT_EQ (int (obj.segments ()), 1);
  EXPECT_EQ (int (obj.points ().size ()), 1);

  obj = ant::Object (db::DPoint (1, 2), db::DPoint (2, 3), 0, tmp);

  EXPECT_EQ (obj.p1 ().to_string (), "1,2");
  EXPECT_EQ (obj.p2 ().to_string (), "2,3");

  EXPECT_EQ (int (obj.segments ()), 1);
  EXPECT_EQ (int (obj.points ().size ()), 2);

  obj = ant::Object (db::DPoint (1, 2), db::DPoint (1, 2), 0, tmp);

  EXPECT_EQ (obj.p1 ().to_string (), "1,2");
  EXPECT_EQ (obj.p2 ().to_string (), "1,2");

  EXPECT_EQ (int (obj.segments ()), 1);
  EXPECT_EQ (int (obj.points ().size ()), 1);

  obj = ant::Object (db::DPoint (), db::DPoint (), 0, tmp);

  EXPECT_EQ (obj.p1 ().to_string (), "0,0");
  EXPECT_EQ (obj.p2 ().to_string (), "0,0");

  EXPECT_EQ (int (obj.segments ()), 1);
  EXPECT_EQ (int (obj.points ().size ()), 1);
}

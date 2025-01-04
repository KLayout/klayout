
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


#include "dbVariableWidthPath.h"
#include "tlUnitTest.h"

TEST(EmptyVP)
{
  db::Point pts[] = { db::Point (0, 0) };  //  should be { }, but zero-size arrays are non-standard
  std::pair<size_t, db::Coord> widths[] = { std::make_pair (size_t (0), 0) };  //  should be { }, but zero-size arrays are non-standard

  db::VariableWidthPath vp (&pts[0], &pts[0], &widths[0], &widths[0]);
  EXPECT_EQ (vp.to_poly ().to_string (), "()");
}

TEST(VP1Point)
{
  db::Point pts[] = { db::Point (0, 0) };  //  should be { }, but zero-size arrays are non-standard
  std::pair<size_t, db::Coord> widths[] = { std::make_pair (size_t (0), 0) };

  db::VariableWidthPath vp (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])], &widths[0], &widths[0]);
  EXPECT_EQ (vp.to_poly ().to_string (), "()");
}

TEST(VP2Point)
{
  db::Point pts[] = { db::Point (0, 0), db::Point (200, 0) };
  std::pair<size_t, db::Coord> widths[] = { std::make_pair (size_t (0), 100), std::make_pair (size_t (1), 50) };

  db::VariableWidthPath vp (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])], &widths[0], &widths[sizeof (widths) / sizeof (widths[0])]);
  EXPECT_EQ (vp.to_poly ().to_string (), "(0,-50;0,50;200,25;200,-25)");
}

TEST(VP3Point_Interpolate)
{
  db::Point pts[] = { db::Point (0, 0), db::Point (100, 0), db::Point (200, 0) };
  std::pair<size_t, db::Coord> widths[] = { std::make_pair (size_t (0), 100), std::make_pair (size_t (2), 50) };

  db::VariableWidthPath vp (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])], &widths[0], &widths[sizeof (widths) / sizeof (widths[0])]);
  EXPECT_EQ (vp.to_poly ().to_string (), "(0,-50;0,50;200,25;200,-25)");
}

TEST(VP3Point_Step)
{
  db::Point pts[] = { db::Point (0, 0), db::Point (100, 0), db::Point (200, 0) };
  std::pair<size_t, db::Coord> widths[] = { std::make_pair (size_t (0), 100), std::make_pair (size_t (1), 100), std::make_pair (size_t (1), 50), std::make_pair (size_t (2), 50) };

  db::VariableWidthPath vp (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])], &widths[0], &widths[sizeof (widths) / sizeof (widths[0])]);
  EXPECT_EQ (vp.to_poly ().to_string (), "(0,-50;0,50;100,50;100,25;200,25;200,-25;100,-25;100,-50)");
}

TEST(VP3Point_Step2)
{
  db::Point pts[] = { db::Point (0, 0), db::Point (100, 0), db::Point (100, 0), db::Point (200, 0) };
  std::pair<size_t, db::Coord> widths[] = { std::make_pair (size_t (0), 100), std::make_pair (size_t (1), 100), std::make_pair (size_t (2), 50), std::make_pair (size_t (3), 50) };

  db::VariableWidthPath vp (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])], &widths[0], &widths[sizeof (widths) / sizeof (widths[0])]);
  EXPECT_EQ (vp.to_poly ().to_string (), "(0,-50;0,50;100,50;100,25;200,25;200,-25;100,-25;100,-50)");
}

TEST(VP3Point90_Step)
{
  db::Point pts[] = { db::Point (0, 0), db::Point (100, 0), db::Point (100, -100) };
  std::pair<size_t, db::Coord> widths[] = { std::make_pair (size_t (0), 100), std::make_pair (size_t (1), 100), std::make_pair (size_t (1), 50), std::make_pair (size_t (2), 50) };

  db::VariableWidthPath vp (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])], &widths[0], &widths[sizeof (widths) / sizeof (widths[0])]);
  EXPECT_EQ (vp.to_poly ().to_string (), "(75,-100;75,0;100,-50;0,-50;0,50;100,50;125,0;125,-100)");
}

TEST(VP3Point90)
{
  db::Point pts[] = { db::Point (0, 0), db::Point (100, 0), db::Point (100, -100) };
  std::pair<size_t, db::Coord> widths[] = { std::make_pair (size_t (0), 100), std::make_pair (size_t (2), 0) };

  db::VariableWidthPath vp (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])], &widths[0], &widths[sizeof (widths) / sizeof (widths[0])]);
  EXPECT_EQ (vp.to_poly ().to_string (), "(100,-100;82,-29;0,-50;0,50;129,18)");
}

TEST(VP3Point90_ConstWidth)
{
  db::Point pts[] = { db::Point (0, 0), db::Point (100, 0), db::Point (100, -100) };
  std::pair<size_t, db::Coord> widths[] = { std::make_pair (size_t (0), 100), std::make_pair (size_t (2), 100) };

  db::VariableWidthPath vp (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])], &widths[0], &widths[sizeof (widths) / sizeof (widths[0])]);
  EXPECT_EQ (vp.to_poly ().to_string (), "(50,-100;50,-50;0,-50;0,50;150,50;150,-100)");
}

TEST(VP3Point135_ConstWidth)
{
  db::Point pts[] = { db::Point (0, 0), db::Point (100, 0), db::Point (0, -100) };
  std::pair<size_t, db::Coord> widths[] = { std::make_pair (size_t (0), 100), std::make_pair (size_t (2), 100) };

  db::VariableWidthPath vp (&pts[0], &pts[sizeof (pts) / sizeof (pts[0])], &widths[0], &widths[sizeof (widths) / sizeof (widths[0])]);
  EXPECT_EQ (vp.to_poly ().to_string (), "(35,-135;-35,-65;-21,-50;0,-50;0,50;200,50;206,35)");
}


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


#include "dbPoint.h"
#include "dbVector.h"
#include "tlString.h"
#include "tlUnitTest.h"

#include <type_traits>

TEST(1) 
{
  db::Point p;
  EXPECT_EQ (p.x (), 0);
  EXPECT_EQ (p.y (), 0);
  db::Vector pp (100, 200);
  EXPECT_EQ ((p + pp).x (), 100);
  EXPECT_EQ ((p + pp).y (), 200);
  p += pp;
  p += db::Vector (p) + pp;
  EXPECT_EQ ((p - pp).x (), 200);
  EXPECT_EQ ((p - pp).y (), 400);
  EXPECT_EQ ((db::Point () - p).to_string (), "-300,-600");
}

TEST(2)
{
  db::DPoint p (12.5, -17.1);
  EXPECT_EQ (p.to_string (), "12.5,-17.1");

  db::DPoint pp;
  tl::Extractor ex ("a");
  EXPECT_EQ (ex.try_read (pp), false);
  ex = tl::Extractor ("12.500, -171e-1   a");
  EXPECT_EQ (ex.try_read (pp), true);
  EXPECT_EQ (pp.to_string (), p.to_string ());
  EXPECT_EQ (ex.test ("a"), true);
}

TEST(3)
{
  db::Point p (125, -171);
  EXPECT_EQ (p.to_string (), "125,-171");

  db::Point pp;
  tl::Extractor ex ("a");
  EXPECT_EQ (ex.try_read (pp), false);
  ex = tl::Extractor (" 125, -171 a");
  EXPECT_EQ (ex.try_read (pp), true);
  EXPECT_EQ (pp == p, true);
  EXPECT_EQ (ex.test ("a"), true);
}

//  Bug: user provided copy ctor/assignment for point and vector made them non-trivially-copyable
TEST(4_trivially_copyable)
{
  EXPECT_EQ (std::is_trivially_copyable<db::Point>::value, true);
  EXPECT_EQ (std::is_trivially_copyable<db::Vector>::value, true);
  EXPECT_EQ (std::is_trivially_copyable<db::DPoint>::value, true);
  EXPECT_EQ (std::is_trivially_copyable<db::DVector>::value, true);

  db::Point p (125, -171);
  db::Point pc (p);
  EXPECT_EQ (pc, p);
  db::Point pa;
  pa = p;
  EXPECT_EQ (pa, p);
  pa.set_x (0);
  EXPECT_EQ (pc, p);

  db::Vector v (-7, 13);
  db::Vector vc (v);
  EXPECT_EQ (vc, v);
  db::Vector va;
  va = v;
  EXPECT_EQ (va, v);
  va.set_y (0);
  EXPECT_EQ (vc, v);

  db::DPoint dp (12.5, -17.1);
  db::DPoint dpc (dp);
  EXPECT_EQ (dpc, dp);
  db::DPoint dpa;
  dpa = dp;
  EXPECT_EQ (dpa, dp);
  dpa.set_x (0.0);
  EXPECT_EQ (dpc, dp);
}



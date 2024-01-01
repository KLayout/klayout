
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

#include "tlMath.h"
#include "tlUnitTest.h"

TEST(1) 
{
  int x;
  x = tl::gcd (17, 6);
  EXPECT_EQ (x, 1);
  x = tl::gcd (27, 6);
  EXPECT_EQ (x, 3);
  x = tl::gcd (30, 6);
  EXPECT_EQ (x, 6);
  x = tl::gcd (31*17, 371*17);
  EXPECT_EQ (x, 17);
  x = tl::gcd (702*17, 372*17);
  EXPECT_EQ (x, 102);

  EXPECT_EQ (tl::less (100, 200), true);
  EXPECT_EQ (tl::less (200, 100), false);
  EXPECT_EQ (tl::less (100, 100), false);
  EXPECT_EQ (tl::equal (200, 100), false);
  EXPECT_EQ (tl::equal (100, 100), true);

  EXPECT_EQ (tl::less (0.1, 0.2), true);
  EXPECT_EQ (tl::less (0.2, 0.1), false);
  EXPECT_EQ (tl::less (0.1, 0.1), false);
  EXPECT_EQ (tl::less (0.1, 0.1 + 1e-7), true);
  EXPECT_EQ (tl::less (0.1, 0.1 + 1e-10), false);
  EXPECT_EQ (tl::equal (0.2, 0.1), false);
  EXPECT_EQ (tl::equal (0.1, 0.1), true);
  EXPECT_EQ (tl::equal (0.1, 0.1 + 1e-7), false);
  EXPECT_EQ (tl::equal (0.1, 0.1 + 1e-12), true);

  double d;
  d = tl::gcd (702*1.7, 372*1.7);
  EXPECT_EQ (tl::equal (d, 10.2), true);
  d = tl::gcd (0.0025, 0.001);
  EXPECT_EQ (tl::equal (d, 0.0005), true);
  d = tl::gcd (0.0025, 0.001 + 1e-12);
  EXPECT_EQ (tl::equal (d, 0.0005), true);

  d = tl::lcm (0.0025, 0.001);
  EXPECT_EQ (tl::equal (d, 0.005), true);
  d = tl::lcm (0.0025, 0.001 + 1e-12);
  EXPECT_EQ (tl::equal (d, 0.005), true);

  EXPECT_EQ (tl::equal (tl::round_down (1.3, 1.0), 1.0), true);
  EXPECT_EQ (tl::equal (tl::round_down (-1.3, 1.0), -2.0), true);
  EXPECT_EQ (tl::equal (tl::round_down (1.0 + 1e-7, 1.0), 1.0), true);
  EXPECT_EQ (tl::equal (tl::round_down (1.0 - 1e-7, 1.0), 0.0), true);
  EXPECT_EQ (tl::equal (tl::round_down (1.0 - 1e-12, 1.0), 1.0), true);
  EXPECT_EQ (tl::equal (tl::round_up (1.3, 1.0), 2.0), true);
  EXPECT_EQ (tl::equal (tl::round_up (-1.3, 1.0), -1.0), true);
  EXPECT_EQ (tl::equal (tl::round_up (1.0 - 1e-7, 1.0), 1.0), true);
  EXPECT_EQ (tl::equal (tl::round_up (1.0 + 1e-7, 1.0), 2.0), true);
  EXPECT_EQ (tl::equal (tl::round_up (1.0 + 1e-12, 1.0), 1.0), true);
  EXPECT_EQ (tl::equal (tl::round (1.3, 1.0), 1.0), true);
  EXPECT_EQ (tl::equal (tl::round (1.5, 1.0), 1.0), true);
  EXPECT_EQ (tl::equal (tl::round (1.5 + 1e-12, 1.0), 1.0), true);
  EXPECT_EQ (tl::equal (tl::round (1.5 + 1e-7, 1.0), 2.0), true);
  EXPECT_EQ (tl::equal (tl::round (1.7, 1.0), 2.0), true);
  EXPECT_EQ (tl::equal (tl::round (-1.3, 1.0), -1.0), true);
  EXPECT_EQ (tl::equal (tl::round (-1.7, 1.0), -2.0), true);
}


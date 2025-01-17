
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

#if defined(HAVE_64BIT_COORD)

#include "tlInt128Support.h"
#include "tlUnitTest.h"

#include <cstdio>

TEST(1) 
{
  EXPECT_EQ (tl::to_string (__int128 (0)), "0");
  EXPECT_EQ (tl::to_string (__int128 (42)), "42");
  EXPECT_EQ (tl::to_string (__int128 (-42)), "-42");

  __int128 x = 1;
  for (int i = 0; i < 30; ++i) {
     x *= 10;
  }

  EXPECT_EQ (tl::to_string (x), "1000000000000000000000000000000");
  EXPECT_EQ (tl::to_string (-x), "-1000000000000000000000000000000");
  EXPECT_EQ (tl::to_string (x + 1), "1000000000000000000000000000001");
  EXPECT_EQ (tl::to_string (x - 1), "999999999999999999999999999999");
}

TEST(2)
{
  EXPECT_EQ (tl::to_string ((unsigned __int128) 0), "0");
  EXPECT_EQ (tl::to_string ((unsigned __int128) 42), "42");

  unsigned __int128 x = 1;
  for (int i = 0; i < 30; ++i) {
     x *= 10;
  }

  EXPECT_EQ (tl::to_string (x), "1000000000000000000000000000000");
  EXPECT_EQ (tl::to_string (x + 1), "1000000000000000000000000000001");
  EXPECT_EQ (tl::to_string (x - 1), "999999999999999999999999999999");
}

#endif

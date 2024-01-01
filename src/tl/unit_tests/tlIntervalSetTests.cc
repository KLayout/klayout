
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


#include <algorithm>

#include "tlIntervalSet.h"
#include "tlString.h"
#include "tlUnitTest.h"

typedef tl::interval_set<int> is_t;

std::string to_string (const is_t &is)
{
  bool f = true;
  std::string s;
  for (is_t::const_iterator i = is.begin (); i != is.end (); ++i) {
    if (!f) { 
      s += ","; 
    }
    f = 0;
    s += tl::to_string (i->first) + ".." + tl::to_string (i->second);
  }
  return s;
}

TEST(1) 
{
  is_t is;

  is.add (1, 5);
  EXPECT_EQ (to_string (is), "1..5");
  is.add (2, 6);
  EXPECT_EQ (to_string (is), "1..6");
  is.clear ();
  EXPECT_EQ (to_string (is), "");
  is.add (1, 5);
  EXPECT_EQ (to_string (is), "1..5");
  is.add (7, 10);
  EXPECT_EQ (to_string (is), "1..5,7..10");
  is.add (5, 7);
  EXPECT_EQ (to_string (is), "1..10");
  is.erase (5, 5);
  EXPECT_EQ (to_string (is), "1..10");
  is.erase (5, 7);
  EXPECT_EQ (to_string (is), "1..5,7..10");
  is.add (15, 17);
  EXPECT_EQ (to_string (is), "1..5,7..10,15..17");
  is.add (0, 100);
  EXPECT_EQ (to_string (is), "0..100");
  EXPECT_EQ (is.check (), true);
  is.erase (2, 99);
  EXPECT_EQ (to_string (is), "0..2,99..100");

}


TEST(2) 
{
  is_t is;

  is.add (1, 6);
  EXPECT_EQ (to_string (is), "1..6");
  is.add (7, 8);
  EXPECT_EQ (to_string (is), "1..6,7..8");

  EXPECT_EQ (is.mapped (1), true);
  EXPECT_EQ (is.mapped (6), false);
  EXPECT_EQ (is.mapped (2), true);
  EXPECT_EQ (is.mapped (3), true);
  EXPECT_EQ (is.mapped (5), true);
  EXPECT_EQ (is.mapped (7), true);
  EXPECT_EQ (is.mapped (8), false);
  EXPECT_EQ (is.mapped (9), false);

}



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


#include "tlUnitTest.h"
#include "tlVector.h"

#include <utility>

namespace {

struct Counted
{
  static int copies;
  static int moves;
  static void reset () { copies = moves = 0; }

  Counted () : x (0) { }
  explicit Counted (int n) : x (n) { }
  Counted (const Counted &d) : x (d.x) { ++copies; }
  Counted (Counted &&d) noexcept : x (d.x) { ++moves; }
  Counted &operator= (const Counted &d) { x = d.x; ++copies; return *this; }
  Counted &operator= (Counted &&d) noexcept { x = d.x; ++moves; return *this; }

  int x;
};

int Counted::copies = 0;
int Counted::moves = 0;

}

//  Test: the move constructor really moves (the original code copied)
TEST(1)
{
  tl::vector<int> a;
  a.push_back (1);
  a.push_back (2);
  a.push_back (3);

  tl::vector<int> b (std::move (a));

  EXPECT_EQ (a.size (), size_t (0));
  EXPECT_EQ (b.size (), size_t (3));
  EXPECT_EQ (b [0], 1);
  EXPECT_EQ (b [1], 2);
  EXPECT_EQ (b [2], 3);
}

//  Test: move assignment really moves (the original code copied)
TEST(2)
{
  tl::vector<int> a;
  a.push_back (1);
  a.push_back (2);

  tl::vector<int> b;
  b.push_back (9);
  b = std::move (a);

  EXPECT_EQ (a.size (), size_t (0));
  EXPECT_EQ (b.size (), size_t (2));
  EXPECT_EQ (b [0], 1);
  EXPECT_EQ (b [1], 2);
}

//  Test: moving does not copy elements (the original code copied them)
TEST(3)
{
  tl::vector<Counted> a;
  for (int i = 0; i < 8; ++i) {
    a.push_back (Counted (i));
  }

  Counted::reset ();

  tl::vector<Counted> b (std::move (a));

  EXPECT_EQ (Counted::copies, 0);
  EXPECT_EQ (a.size (), size_t (0));
  EXPECT_EQ (b.size (), size_t (8));
  EXPECT_EQ (b [3].x, 3);

  Counted::reset ();

  tl::vector<Counted> c;
  c = std::move (b);

  EXPECT_EQ (Counted::copies, 0);
  EXPECT_EQ (b.size (), size_t (0));
  EXPECT_EQ (c.size (), size_t (8));
  EXPECT_EQ (c [5].x, 5);
}

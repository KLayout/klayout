
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



#include "tlIntervalMap.h"
#include "tlString.h"
#include "tlUnitTest.h"

#include <algorithm>

struct AddOp 
{
  void operator() (std::string &a, const std::string &b) { a += b; }
};

struct MaxOp 
{
  void operator() (std::string &a, const std::string &b) { if (b > a) a = b; }
};

typedef tl::interval_map<int, std::string> im_t;

std::string to_string (const im_t &im)
{
  bool f = true;
  std::string s;
  for (im_t::const_iterator i = im.begin (); i != im.end (); ++i) {
    if (!f) { 
      s += ","; 
    }
    f = 0;
    s += tl::to_string (i->first.first) + ".." + tl::to_string (i->first.second) + ":" + i->second;
  }
  return s;
}

TEST(1) 
{
  im_t im;

  AddOp aop;
  im.add (1, 5, "a", aop);
  EXPECT_EQ (to_string (im), "1..5:a");
  im.add (2, 6, "b", aop);
  EXPECT_EQ (to_string (im), "1..2:a,2..5:ab,5..6:b");
  im.clear ();
  EXPECT_EQ (to_string (im), "");
  im.add (1, 5, "a", aop);
  EXPECT_EQ (to_string (im), "1..5:a");
  im.add (7, 10, "a", aop);
  EXPECT_EQ (to_string (im), "1..5:a,7..10:a");
  im.add (5, 7, "a", aop);
  EXPECT_EQ (to_string (im), "1..10:a");
  im.erase (5, 5);
  EXPECT_EQ (to_string (im), "1..10:a");
  im.erase (5, 7);
  EXPECT_EQ (to_string (im), "1..5:a,7..10:a");
  im.add (15, 17, "b", aop);
  EXPECT_EQ (to_string (im), "1..5:a,7..10:a,15..17:b");
  im.add (0, 100, "a", aop);
  EXPECT_EQ (to_string (im), "0..1:a,1..5:aa,5..7:a,7..10:aa,10..15:a,15..17:ba,17..100:a");
  EXPECT_EQ (im.check (), true);
  im.erase (2, 99);
  EXPECT_EQ (to_string (im), "0..1:a,1..2:aa,99..100:a");

}


TEST(2) 
{
  im_t im;

  AddOp aop;
  im.add (1, 5, "a", aop);
  EXPECT_EQ (to_string (im), "1..5:a");
  im.add (2, 6, "b", aop);
  EXPECT_EQ (to_string (im), "1..2:a,2..5:ab,5..6:b");
  im.add (7, 8, "c", aop);
  EXPECT_EQ (to_string (im), "1..2:a,2..5:ab,5..6:b,7..8:c");

  EXPECT_NE (im.mapped (1), 0);
  EXPECT_EQ (*im.mapped (1), "a");
  EXPECT_EQ (im.mapped (6), 0);
  EXPECT_NE (im.mapped (2), 0);
  EXPECT_EQ (*im.mapped (2), "ab");
  EXPECT_NE (im.mapped (3), 0);
  EXPECT_EQ (*im.mapped (3), "ab");
  EXPECT_NE (im.mapped (5), 0);
  EXPECT_EQ (*im.mapped (5), "b");
  EXPECT_NE (im.mapped (7), 0);
  EXPECT_EQ (*im.mapped (7), "c");

}

TEST(3) 
{
  im_t im;

  AddOp aop;
  im.add (0, 10, "a", aop);
  EXPECT_EQ (to_string (im), "0..10:a");
  im.add (0, 6, "b", aop);
  EXPECT_EQ (to_string (im), "0..6:ab,6..10:a");
  im.add (5, 10, "c", aop);
  EXPECT_EQ (to_string (im), "0..5:ab,5..6:abc,6..10:ac");

  EXPECT_NE (im.mapped (1), 0);
  EXPECT_EQ (*im.mapped (1), "ab");
  EXPECT_EQ (im.mapped (11), 0);
  EXPECT_NE (im.mapped (4), 0);
  EXPECT_EQ (*im.mapped (4), "ab");
  EXPECT_NE (im.mapped (5), 0);
  EXPECT_EQ (*im.mapped (5), "abc");
  EXPECT_NE (im.mapped (6), 0);
  EXPECT_EQ (*im.mapped (6), "ac");
  EXPECT_NE (im.mapped (9), 0);
  EXPECT_EQ (*im.mapped (9), "ac");

}

TEST(4) 
{
  im_t im;

  MaxOp aop;
  im.add (0, 10, "a", aop);
  EXPECT_EQ (to_string (im), "0..10:a");
  im.add (0, 5, "b", aop);
  EXPECT_EQ (to_string (im), "0..5:b,5..10:a");
  im.add (6, 10, "c", aop);
  EXPECT_EQ (to_string (im), "0..5:b,5..6:a,6..10:c");

  im_t im2;
  im2 = im;
  im2.add (-5, 15, "c", aop);
  EXPECT_EQ (to_string (im2), "-5..15:c");

  im2 = im;
  im2.add (-5, 6, "c", aop);
  EXPECT_EQ (to_string (im2), "-5..10:c");

  im2 = im;
  im2.add (0, 6, "c", aop);
  EXPECT_EQ (to_string (im2), "0..10:c");

  im.add (0, 1, "c", aop);
  EXPECT_EQ (to_string (im), "0..1:c,1..5:b,5..6:a,6..10:c");
  im.add (2, 3, "c", aop);
  EXPECT_EQ (to_string (im), "0..1:c,1..2:b,2..3:c,3..5:b,5..6:a,6..10:c");
  im.add (1, 2, "c", aop);
  EXPECT_EQ (to_string (im), "0..3:c,3..5:b,5..6:a,6..10:c");
  im.add (5, 6, "c", aop);
  EXPECT_EQ (to_string (im), "0..3:c,3..5:b,5..10:c");
  im.add (2, 6, "c", aop);
  EXPECT_EQ (to_string (im), "0..10:c");
}


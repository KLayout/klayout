
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

#include "tlBitSetMap.h"
#include "tlUnitTest.h"
#include "tlString.h"
#include "tlTimer.h"

static tl::BitSet bs (const char *s)
{
  tl::BitSet res;
  for (unsigned int i = 0; *s; ++i, ++s) {
    res.set (i);
    if (*s == '0') {
      res.reset (i);
    }
  }
  return res;
}

static tl::BitSetMask bsm (const char *s)
{
  tl::BitSetMask res;
  for (unsigned int i = 0; *s; ++i, ++s) {
    if (*s == '0') {
      res.set (i, tl::BitSetMask::False);
    } else if (*s == '1') {
      res.set (i, tl::BitSetMask::True);
    } else if (*s == 'X') {
      res.set (i, tl::BitSetMask::Any);
    } else if (*s == '-') {
      res.set (i, tl::BitSetMask::Never);
    }
  }
  return res;
}

struct SetInserter
{
  SetInserter (std::set<int> &s) : ps (&s) { }

  SetInserter &operator++(int) { return *this; }
  SetInserter &operator* () { return *this; }

  SetInserter &operator= (int v) { ps->insert (v); return *this; }

  std::set<int> *ps;
};

static std::string match (const tl::bit_set_map<int> &bsm, const tl::BitSet &bs)
{
  std::set<int> values;
  bsm.lookup (bs, SetInserter (values));

  std::string res;
  for (auto i = values.begin (); i != values.end (); ++i) {
    if (!res.empty ()) {
      res += ",";
    }
    res += tl::to_string (*i);
  }
  return res;
}

namespace
{

TEST(1_Basic)
{
  tl::bit_set_map<int> map;

  map.insert (bsm ("X10"), 1);
  map.insert (bsm ("X10"), 11);
  map.insert (bsm ("1"), 2);
  map.insert (bsm ("101"), 3);
  map.insert (bsm ("1X0"), 4);
  map.insert (bsm ("110"), 5);
  map.sort ();

  EXPECT_EQ (match (map, bs ("")), "");
  EXPECT_EQ (match (map, bs ("1")), "2,4");
  EXPECT_EQ (match (map, bs ("110")), "1,2,4,5,11");
  EXPECT_EQ (match (map, bs ("01")), "1,11");
  EXPECT_EQ (match (map, bs ("010000")), "1,11");

  map.insert (bsm (""), 0);
  try {
    match (map, bs (""));
    EXPECT_EQ (true, false);  // not sorted
  } catch (...) { }

  map.sort ();
  EXPECT_EQ (match (map, bs ("")), "0");
}

static std::string bitstr (unsigned int n, unsigned int nbits)
{
  std::string r;
  while (nbits > 0) {
    r += ((n & 1) != 0 ? '1' : '0');
    n >>= 1;
    --nbits;
  }
  return r;
}

TEST(2_Regular)
{
  tl::bit_set_map<int> map;

  unsigned int num = 10000;
  unsigned int nbits = 20;

  for (unsigned int i = 0; i < num; ++i) {
    map.insert (bsm (bitstr (i, nbits).c_str ()), int (i));
  }

  {
    tl::SelfTimer timer ("sorting");
    map.sort ();
  }

  {
    tl::SelfTimer timer ("match method");
    for (unsigned int i = 0; i < num; ++i) {
      EXPECT_EQ (match (map, bs (bitstr (i, nbits).c_str ())), tl::to_string (i));
    }
  }

  //  brute force
  {
    tl::SelfTimer timer ("brute force");
    for (unsigned int i = 0; i < num; ++i) {
      tl::BitSet k = bs (bitstr (i, nbits).c_str ());
      int value = 0;
      for (auto j = map.begin (); j != map.end (); ++j) {
        if (j->mask.match (k)) {
          value = j->value;
        }
      }
      EXPECT_EQ (value, int (i));
    }
  }
}

}

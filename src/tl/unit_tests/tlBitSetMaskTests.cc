
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

#include "tlBitSetMask.h"
#include "tlUnitTest.h"
#include "tlString.h"

namespace
{

static std::string l2s (const tl::BitSetMask &s)
{
  std::string x;
  for (tl::BitSetMask::index_type i = 0; i < s.size (); ++i) {
    switch (s[i]) {
    case tl::BitSetMask::Any:
      x += "X";
      break;
    case tl::BitSetMask::True:
      x += "1";
      break;
    case tl::BitSetMask::False:
      x += "0";
      break;
    default:
      x += "-";
      break;
    }
  }
  return x;
}

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

TEST(1_Basic)
{
  tl::BitSetMask bs;
  EXPECT_EQ (bs.is_empty (), true);
  EXPECT_EQ (bs.size (), 0u);
  EXPECT_EQ (l2s (bs), "");

  bs.set (1, tl::BitSetMask::True);
  EXPECT_EQ (bs.size (), 2u);
  EXPECT_EQ (l2s (bs), "X1");

  bs.set (32, tl::BitSetMask::False);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "X1XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX0");

  bs.set (3, tl::BitSetMask::False);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "X1X0XXXXXXXXXXXXXXXXXXXXXXXXXXXX0");

  bs.set (128, tl::BitSetMask::Any);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "X1X0XXXXXXXXXXXXXXXXXXXXXXXXXXXX0");

  bs.clear ();
  EXPECT_EQ (bs.size (), 0u);
  EXPECT_EQ (l2s (bs), "");

  bs.resize (6);
  EXPECT_EQ (bs.size (), 6u);
  EXPECT_EQ (l2s (bs), "XXXXXX");
}

TEST(2_Equality)
{
  tl::BitSetMask bs1, bs2, bs3;

  EXPECT_EQ (bs1 == bs2, true);
  EXPECT_EQ (bs1 != bs2, false);

  bs1.set (0, tl::BitSetMask::True);
  EXPECT_EQ (bs1 == bs2, false);
  EXPECT_EQ (bs1 != bs2, true);

  bs1.set (32, tl::BitSetMask::False);
  EXPECT_EQ (bs1 == bs2, false);
  EXPECT_EQ (bs1 != bs2, true);

  bs2.set (0, tl::BitSetMask::True);
  bs2.set (32, tl::BitSetMask::False);
  EXPECT_EQ (bs1 == bs2, true);
  EXPECT_EQ (bs1 == bs3, false);
  EXPECT_EQ (bs1 != bs2, false);
  EXPECT_EQ (bs1 != bs3, true);

  bs1.set (0, tl::BitSetMask::Any);
  bs1.set (32, tl::BitSetMask::Any);
  EXPECT_EQ (bs1 == bs2, false);
  EXPECT_EQ (bs1 == bs3, true);
  EXPECT_EQ (bs1 != bs2, true);
  EXPECT_EQ (bs1 != bs3, false);
}

TEST(3_Compare)
{
  tl::BitSetMask bs1, bs2, bs3;

  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, false);

  bs1.set (0, tl::BitSetMask::True);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, true);

  bs1.set (32, tl::BitSetMask::False);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, true);

  bs2.set (32, tl::BitSetMask::False);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs1 < bs3, false);
  EXPECT_EQ (bs2 < bs1, true);
  EXPECT_EQ (bs3 < bs1, true);

  bs2.set (0, tl::BitSetMask::True);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs1 < bs3, false);
  EXPECT_EQ (bs2 < bs1, false);
  EXPECT_EQ (bs3 < bs1, true);

  bs1.set (0, tl::BitSetMask::Any);
  bs1.set (32, tl::BitSetMask::Any);
  EXPECT_EQ (bs1 < bs2, true);
  EXPECT_EQ (bs1 < bs3, false);
  EXPECT_EQ (bs2 < bs1, false);
  EXPECT_EQ (bs3 < bs1, false);

  bs1.clear ();
  bs2.clear ();
  bs1.set (0, tl::BitSetMask::Any);

  bs2.set (0, tl::BitSetMask::Any);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, false);

  bs2.set (0, tl::BitSetMask::False);
  EXPECT_EQ (bs1 < bs2, true);
  EXPECT_EQ (bs2 < bs1, false);

  bs2.set (0, tl::BitSetMask::True);
  EXPECT_EQ (bs1 < bs2, true);
  EXPECT_EQ (bs2 < bs1, false);

  bs2.set (0, tl::BitSetMask::Never);
  EXPECT_EQ (bs1 < bs2, true);
  EXPECT_EQ (bs2 < bs1, false);

  bs1.set (0, tl::BitSetMask::False);

  bs2.set (0, tl::BitSetMask::Any);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, true);

  bs2.set (0, tl::BitSetMask::False);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, false);

  bs2.set (0, tl::BitSetMask::True);
  EXPECT_EQ (bs1 < bs2, true);
  EXPECT_EQ (bs2 < bs1, false);

  bs2.set (0, tl::BitSetMask::Never);
  EXPECT_EQ (bs1 < bs2, true);
  EXPECT_EQ (bs2 < bs1, false);

  bs1.set (0, tl::BitSetMask::True);

  bs2.set (0, tl::BitSetMask::Any);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, true);

  bs2.set (0, tl::BitSetMask::False);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, true);

  bs2.set (0, tl::BitSetMask::True);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, false);

  bs2.set (0, tl::BitSetMask::Never);
  EXPECT_EQ (bs1 < bs2, true);
  EXPECT_EQ (bs2 < bs1, false);

  bs1.set (0, tl::BitSetMask::Never);

  bs2.set (0, tl::BitSetMask::Any);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, true);

  bs2.set (0, tl::BitSetMask::False);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, true);

  bs2.set (0, tl::BitSetMask::True);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, true);

  bs2.set (0, tl::BitSetMask::Never);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, false);
}

TEST(4_Assign)
{
  tl::BitSetMask bs;
  EXPECT_EQ (l2s (bs), "");
  EXPECT_EQ (l2s (tl::BitSetMask (bs)), "");

  bs.set (3, tl::BitSetMask::True);
  bs.set (32, tl::BitSetMask::False);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "XXX1XXXXXXXXXXXXXXXXXXXXXXXXXXXX0");
  EXPECT_EQ (tl::BitSetMask (bs).size (), 33u);
  EXPECT_EQ (l2s (tl::BitSetMask (bs)), "XXX1XXXXXXXXXXXXXXXXXXXXXXXXXXXX0");

  tl::BitSetMask bs2;
  bs2.swap (bs);
  EXPECT_EQ (bs.size (), 0u);
  EXPECT_EQ (bs2.size (), 33u);
  EXPECT_EQ (l2s (bs), "");
  EXPECT_EQ (l2s (bs2), "XXX1XXXXXXXXXXXXXXXXXXXXXXXXXXXX0");

  bs = bs2;
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "XXX1XXXXXXXXXXXXXXXXXXXXXXXXXXXX0");

  bs2.clear ();
  EXPECT_EQ (bs2.size (), 0u);
  EXPECT_EQ (l2s (bs2), "");

  bs2 = std::move (bs);
  EXPECT_EQ (bs.size (), 0u);
  EXPECT_EQ (l2s (bs), "");
  EXPECT_EQ (bs2.size (), 33u);
  EXPECT_EQ (l2s (bs2), "XXX1XXXXXXXXXXXXXXXXXXXXXXXXXXXX0");

  tl::BitSetMask bs3 (std::move (bs2));
  EXPECT_EQ (bs2.size (), 0u);
  EXPECT_EQ (l2s (bs2), "");
  EXPECT_EQ (bs3.size (), 33u);
  EXPECT_EQ (l2s (bs3), "XXX1XXXXXXXXXXXXXXXXXXXXXXXXXXXX0");
}

TEST(5_Match)
{
  tl::BitSetMask bsm;
  EXPECT_EQ (bsm.match (bs ("")), true);
  EXPECT_EQ (bsm.match (bs ("0")), true);
  EXPECT_EQ (bsm.match (bs ("1")), true);
  EXPECT_EQ (bsm.match (bs ("10101")), true);

  bsm.set (1, tl::BitSetMask::Never);
  EXPECT_EQ (l2s (bsm), "X-");
  EXPECT_EQ (bsm.match (bs ("")), true);
  EXPECT_EQ (bsm.match (bs ("0")), true);
  EXPECT_EQ (bsm.match (bs ("1")), true);
  EXPECT_EQ (bsm.match (bs ("10101")), false);  //  fails because "never bit" is used.

  bsm.clear ();
  bsm.set (1, tl::BitSetMask::True);
  bsm.set (2, tl::BitSetMask::False);
  EXPECT_EQ (l2s (bsm), "X10");

  EXPECT_EQ (bsm.match (bs ("")), false);
  EXPECT_EQ (bsm.match (bs ("0")), false);
  EXPECT_EQ (bsm.match (bs ("000")), false);
  EXPECT_EQ (bsm.match (bs ("001")), false);
  EXPECT_EQ (bsm.match (bs ("010")), true);
  EXPECT_EQ (bsm.match (bs ("011")), false);
  EXPECT_EQ (bsm.match (bs ("1")), false);
  EXPECT_EQ (bsm.match (bs ("100")), false);
  EXPECT_EQ (bsm.match (bs ("101")), false);
  EXPECT_EQ (bsm.match (bs ("110")), true);
  EXPECT_EQ (bsm.match (bs ("111")), false);
  EXPECT_EQ (bsm.match (bs ("10101")), false);
  EXPECT_EQ (bsm.match (bs ("11001")), true);

  bsm.clear ();
  bsm.set (32, tl::BitSetMask::True);
  EXPECT_EQ (bsm.match (bs ("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX0")), false);
  EXPECT_EQ (bsm.match (bs ("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX1")), true);
  EXPECT_EQ (bsm.match (bs ("")), false);

  bsm.clear ();
  bsm.set (32, tl::BitSetMask::False);
  EXPECT_EQ (bsm.match (bs ("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX0")), true);
  EXPECT_EQ (bsm.match (bs ("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX1")), false);
  EXPECT_EQ (bsm.match (bs ("")), true);  //  because unset bits count as zero
}

}

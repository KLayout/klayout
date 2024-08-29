
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

#include "tlBitSet.h"
#include "tlUnitTest.h"
#include "tlString.h"

namespace
{

static std::string l2s (const tl::BitSet &s)
{
  return s.to_string ();
}

TEST(1_Basic)
{
  tl::BitSet bs;
  EXPECT_EQ (bs.is_empty (), true);
  EXPECT_EQ (bs.size (), 0u);
  EXPECT_EQ (l2s (bs), "");
  EXPECT_EQ (l2s (tl::BitSet (l2s (bs))), "");

  bs.set (1);
  EXPECT_EQ (bs.size (), 2u);
  EXPECT_EQ (l2s (bs), "01");
  EXPECT_EQ (l2s (tl::BitSet (l2s (bs))), "01");

  bs.set (32);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "010000000000000000000000000000001");
  EXPECT_EQ (l2s (tl::BitSet (l2s (bs))), "010000000000000000000000000000001");

  bs.set (3);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "010100000000000000000000000000001");
  EXPECT_EQ (l2s (tl::BitSet (l2s (bs))), "010100000000000000000000000000001");

  unsigned int indexes[] = { 5, 6, 7 };
  bs.set (indexes + 0, indexes + sizeof (indexes) / sizeof (indexes [0]));
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "010101110000000000000000000000001");
  EXPECT_EQ (l2s (tl::BitSet (l2s (bs))), "010101110000000000000000000000001");

  bs.reset (128);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "010101110000000000000000000000001");

  bs.reset (1);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "000101110000000000000000000000001");

  bs.reset (indexes + 0, indexes + sizeof (indexes) / sizeof (indexes [0]));
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "000100000000000000000000000000001");

  bs.set_value (0, true);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "100100000000000000000000000000001");

  bs.set_value (indexes + 0, indexes + sizeof (indexes) / sizeof (indexes [0]), true);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "100101110000000000000000000000001");

  bs.set_value (indexes + 0, indexes + sizeof (indexes) / sizeof (indexes [0]), false);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "100100000000000000000000000000001");

  bs.set_value (0, false);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "000100000000000000000000000000001");

  bs.clear ();
  EXPECT_EQ (bs.size (), 0u);
  EXPECT_EQ (l2s (bs), "");

  bs.resize (6);
  EXPECT_EQ (bs.size (), 6u);
  EXPECT_EQ (l2s (bs), "000000");
}

TEST(2_Equality)
{
  tl::BitSet bs1, bs2, bs3;

  EXPECT_EQ (bs1 == bs2, true);
  EXPECT_EQ (bs1 != bs2, false);

  bs1.set (0);
  EXPECT_EQ (bs1 == bs2, false);
  EXPECT_EQ (bs1 != bs2, true);

  bs1.set (32);
  EXPECT_EQ (bs1 == bs2, false);
  EXPECT_EQ (bs1 != bs2, true);

  bs2.set (0);
  bs2.set (32);
  EXPECT_EQ (bs1 == bs2, true);
  EXPECT_EQ (bs1 == bs3, false);
  EXPECT_EQ (bs1 != bs2, false);
  EXPECT_EQ (bs1 != bs3, true);

  bs1.reset (0);
  bs1.reset (32);
  EXPECT_EQ (bs1 == bs2, false);
  EXPECT_EQ (bs1 == bs3, true);
  EXPECT_EQ (bs1 != bs2, true);
  EXPECT_EQ (bs1 != bs3, false);
}

TEST(3_Compare)
{
  tl::BitSet bs1, bs2, bs3;

  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, false);

  bs1.set (0);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, true);

  bs1.set (32);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs2 < bs1, true);

  bs2.set (0);
  bs2.set (32);
  EXPECT_EQ (bs1 < bs2, false);
  EXPECT_EQ (bs1 < bs3, false);
  EXPECT_EQ (bs2 < bs1, false);
  EXPECT_EQ (bs3 < bs1, true);

  bs1.reset (0);
  bs1.reset (32);
  EXPECT_EQ (bs1 < bs2, true);
  EXPECT_EQ (bs1 < bs3, false);
  EXPECT_EQ (bs2 < bs1, false);
  EXPECT_EQ (bs3 < bs1, false);
}

TEST(4_Assign)
{
  tl::BitSet bs;
  EXPECT_EQ (l2s (bs), "");
  EXPECT_EQ (l2s (tl::BitSet (bs)), "");

  bs.set (3);
  bs.set (32);
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "000100000000000000000000000000001");
  EXPECT_EQ (tl::BitSet (bs).size (), 33u);
  EXPECT_EQ (l2s (tl::BitSet (bs)), "000100000000000000000000000000001");

  tl::BitSet bs2;
  bs2.swap (bs);
  EXPECT_EQ (bs.size (), 0u);
  EXPECT_EQ (bs2.size (), 33u);
  EXPECT_EQ (l2s (bs), "");
  EXPECT_EQ (l2s (bs2), "000100000000000000000000000000001");

  bs = bs2;
  EXPECT_EQ (bs.size (), 33u);
  EXPECT_EQ (l2s (bs), "000100000000000000000000000000001");

  bs2.clear ();
  EXPECT_EQ (bs2.size (), 0u);
  EXPECT_EQ (l2s (bs2), "");

  bs2 = std::move (bs);
  EXPECT_EQ (bs.size (), 0u);
  EXPECT_EQ (l2s (bs), "");
  EXPECT_EQ (bs2.size (), 33u);
  EXPECT_EQ (l2s (bs2), "000100000000000000000000000000001");

  tl::BitSet bs3 (std::move (bs2));
  EXPECT_EQ (bs2.size (), 0u);
  EXPECT_EQ (l2s (bs2), "");
  EXPECT_EQ (bs3.size (), 33u);
  EXPECT_EQ (l2s (bs3), "000100000000000000000000000000001");

  bs.clear ();

  unsigned int indexes[] = { 5, 6, 7 };
  bs = std::move (tl::BitSet (indexes + 0, indexes + sizeof (indexes) / sizeof (indexes [0])));
  EXPECT_EQ (bs.size (), 8u);
  EXPECT_EQ (l2s (bs), "00000111");
}

}

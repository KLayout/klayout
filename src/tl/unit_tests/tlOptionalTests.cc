
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

#include "tlOptional.h"
#include "tlUnitTest.h"

namespace
{

TEST(1_Basic)
{
  tl::optional<int> opt;

  //  value not set

  EXPECT_EQ (opt.has_value (), false);
  EXPECT_EQ (opt.operator-> (), (int *) 0);
  EXPECT_EQ (((const tl::optional<int> &) opt).operator-> (), (const int *) 0);
  EXPECT_EQ (tl::to_string (opt), "");

  try {
    opt.value (); // asserts
    EXPECT_EQ (true, false);
  } catch (...) {
  }

  //  make_optional, assignment

  opt = tl::make_optional (17);

  //  value set

  EXPECT_EQ (opt.has_value (), true);
  EXPECT_EQ (opt.value (), 17);
  EXPECT_EQ (tl::to_string (opt), "17");
  EXPECT_EQ (((const tl::optional<int> &) opt).value (), 17);
  EXPECT_EQ (*opt, 17);
  EXPECT_EQ (*((const tl::optional<int> &) opt), 17);
  EXPECT_EQ (*(opt.operator-> ()), 17);
  EXPECT_EQ (*(((const tl::optional<int> &) opt).operator-> ()), 17);

  //  compare operators

  EXPECT_EQ (opt == tl::make_optional (-1), false);
  EXPECT_EQ (opt == tl::make_optional (17), true);
  EXPECT_EQ (opt == tl::optional<int> (), false);

  EXPECT_EQ (opt != tl::make_optional (-1), true);
  EXPECT_EQ (opt != tl::make_optional (17), false);
  EXPECT_EQ (opt != tl::optional<int> (), true);

  //  copy ctor

  tl::optional<int> opt2 (opt);

  EXPECT_EQ (opt2.has_value (), true);
  EXPECT_EQ (opt2.value (), 17);

  //  reset method

  opt = tl::make_optional (17);
  opt.reset ();

  EXPECT_EQ (opt.has_value (), false);
  EXPECT_EQ (opt == tl::optional<int> (), true);
  EXPECT_EQ (opt != tl::optional<int> (), false);

  //  tl::nullopt tag

  opt = tl::make_optional (17);
  opt = tl::optional<int> (tl::nullopt);

  EXPECT_EQ (opt.has_value (), false);
  EXPECT_EQ (opt == tl::optional<int> (), true);
  EXPECT_EQ (opt != tl::optional<int> (), false);
}

}

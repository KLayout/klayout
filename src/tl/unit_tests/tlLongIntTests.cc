
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


#include <algorithm>
#include <stdint.h>
#include <cstdio>

#include "tlLongInt.h"
#include "tlUnitTest.h"

typedef tl::long_int<4, uint8_t, uint16_t> li_type;
typedef int32_t i_type;

typedef tl::long_uint<4, uint8_t, uint16_t> lui_type;
typedef uint32_t ui_type;

i_type to_i (li_type x)
{
  return i_type (x);
}

ui_type to_i (lui_type x)
{
  return ui_type (x);
}

li_type to_e (i_type x)
{
  return li_type (x);
}

lui_type to_e (ui_type x)
{
  return lui_type (x);
}

template <class I1, class LI1, class I2, class LI2>
static void run_test_int (tl::TestBase *_this, I1 a, I2 b)
{
  typedef typename LI1::basic_type basic_type;
  if (tl::verbose ()) {
    printf("Long int test with pair (%ld,%ld)\n", long (a), long (b));
  }

  LI1 ae = to_e (a);
  LI2 be = to_e (b);
  LI1 r;

  EXPECT_EQ (to_i (ae), a);
  EXPECT_EQ (to_i (be), b);

  EXPECT_EQ (to_i (ae + be), a + b);
  r = ae;
  r += be;
  EXPECT_EQ (to_i (r), a + b);
  EXPECT_EQ (to_i (ae + basic_type (2)), a + basic_type (2));
  r = ae;
  r += basic_type (2);
  EXPECT_EQ (to_i (r), a + basic_type (2));

  EXPECT_EQ (to_i (ae - be), a - b);
  r = ae;
  r -= be;
  EXPECT_EQ (to_i (r), a - b);
  EXPECT_EQ (to_i (ae - basic_type (2)), a - basic_type (2));
  r = ae;
  r -= basic_type (2);
  EXPECT_EQ (to_i (r), a - basic_type (2));

  EXPECT_EQ (LI2 (ae) == be, I2 (a) == b);
  EXPECT_EQ (LI2 (ae) != be, I2 (a) != b);
  EXPECT_EQ (LI2 (ae) < be, I2 (a) < b);
  EXPECT_EQ (LI2 (ae) <= be, I2 (a) <= b);
  EXPECT_EQ (LI2 (ae) > be, I2 (a) > b);
  EXPECT_EQ (LI2 (ae) >= be, I2 (a) >= b);
  EXPECT_EQ (ae.is_zero (), a == 0);

  EXPECT_EQ (to_i (ae * be), a * b);
  r = ae;
  r *= be;
  EXPECT_EQ (to_i (r), a * b);

  if (b != 0) {
    EXPECT_EQ (to_i (ae / be), a / b);
    r = ae;
    r /= be;
    EXPECT_EQ (to_i (r), a / b);
    EXPECT_EQ (to_i (ae % be), a % b);
    r = ae;
    r %= be;
    EXPECT_EQ (to_i (r), a % b);
  }
}

template <class I1, class LI1, class I2, class LI2>
static void run_test (tl::TestBase *_this, I1 a, I2 b)
{
  run_test_int<I1, LI1, I2, LI2> (_this, a, b);
  run_test_int<I1, LI1, I2, LI2> (_this, a, a);
  run_test_int<I1, LI1, I2, LI2> (_this, b, b);
  run_test_int<I1, LI1, I2, LI2> (_this, b, a);
}

TEST(1)
{
  run_test<i_type, li_type, i_type, li_type> (_this, 0, 1);
  run_test<i_type, li_type, i_type, li_type> (_this, 256, 257);
  run_test<i_type, li_type, i_type, li_type> (_this, 256, 2);
  run_test<i_type, li_type, i_type, li_type> (_this, 65535, 65536);
  run_test<i_type, li_type, i_type, li_type> (_this, 65535, 2);
  run_test<i_type, li_type, i_type, li_type> (_this, 0xfffffffe, 0xffffffff);
  run_test<i_type, li_type, i_type, li_type> (_this, 0xfffffffe, 2);
  for (unsigned int i = 0; i < 100000; ++i) {
    run_test<i_type, li_type, i_type, li_type> (_this, rand () * rand (), rand () * rand ());
  }
}

TEST(2)
{
  run_test<ui_type, lui_type, i_type, li_type> (_this, 0, 1);
  run_test<ui_type, lui_type, i_type, li_type> (_this, 256, 257);
  run_test<ui_type, lui_type, i_type, li_type> (_this, 256, 2);
  run_test<ui_type, lui_type, i_type, li_type> (_this, 65535, 65536);
  run_test<ui_type, lui_type, i_type, li_type> (_this, 65535, 2);
  run_test<ui_type, lui_type, i_type, li_type> (_this, 0xfffffffe, 0xffffffff);
  run_test<ui_type, lui_type, i_type, li_type> (_this, 0xfffffffe, 2);
  for (unsigned int i = 0; i < 100000; ++i) {
    run_test<ui_type, lui_type, i_type, li_type> (_this, rand () * rand (), rand () * rand ());
  }
}

TEST(3)
{
  run_test<i_type, li_type, ui_type, lui_type> (_this, 0, 1);
  run_test<i_type, li_type, ui_type, lui_type> (_this, 256, 257);
  run_test<i_type, li_type, ui_type, lui_type> (_this, 256, 2);
  run_test<i_type, li_type, ui_type, lui_type> (_this, 65535, 65536);
  run_test<i_type, li_type, ui_type, lui_type> (_this, 65535, 2);
  run_test<i_type, li_type, ui_type, lui_type> (_this, 0xfffffffe, 0xffffffff);
  run_test<i_type, li_type, ui_type, lui_type> (_this, 0xfffffffe, 2);
  for (unsigned int i = 0; i < 100000; ++i) {
    run_test<i_type, li_type, ui_type, lui_type> (_this, rand () * rand (), rand () * rand ());
  }
}

TEST(4)
{
  run_test<ui_type, lui_type, ui_type, lui_type> (_this, 0, 1);
  run_test<ui_type, lui_type, ui_type, lui_type> (_this, 256, 257);
  run_test<ui_type, lui_type, ui_type, lui_type> (_this, 256, 2);
  run_test<ui_type, lui_type, ui_type, lui_type> (_this, 65535, 65536);
  run_test<ui_type, lui_type, ui_type, lui_type> (_this, 65535, 2);
  run_test<ui_type, lui_type, ui_type, lui_type> (_this, 0xfffffffe, 0xffffffff);
  run_test<ui_type, lui_type, ui_type, lui_type> (_this, 0xfffffffe, 2);
  for (unsigned int i = 0; i < 100000; ++i) {
    run_test<ui_type, lui_type, ui_type, lui_type> (_this, rand () * rand (), rand () * rand ());
  }
}

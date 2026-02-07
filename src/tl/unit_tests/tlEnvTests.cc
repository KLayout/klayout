
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

#include "tlEnv.h"
#include "tlUnitTest.h"

const char *dne_name = "__DOES_NOT_EXIST__";

TEST(1)
{
  EXPECT_EQ (tl::has_env (dne_name), false);

  tl::set_env (dne_name, "123");
  EXPECT_EQ (tl::has_env (dne_name), true);

  EXPECT_EQ (tl::get_env (dne_name), "123");

  tl::set_env (dne_name, "42");
  EXPECT_EQ (tl::has_env (dne_name), true);

  EXPECT_EQ (tl::get_env (dne_name), "42");

  tl::unset_env (dne_name);
  EXPECT_EQ (tl::get_env (dne_name, "bla"), "bla");
  EXPECT_EQ (tl::has_env (dne_name), false);
}

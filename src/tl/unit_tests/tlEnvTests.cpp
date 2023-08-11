
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

TEST(1)
{
  const char *env_name = "$$$DOESNOTEXIST";

  EXPECT_EQ (tl::has_env (env_name), false);
  EXPECT_EQ (tl::has_env ("HOME"), true);

  tl::set_env (env_name, "abc");
  EXPECT_EQ (tl::has_env (env_name), true);
  EXPECT_EQ (tl::get_env (env_name), "abc");

  tl::set_env (env_name, "uvw");
  EXPECT_EQ (tl::has_env (env_name), true);
  EXPECT_EQ (tl::get_env (env_name), "uvw");

  tl::unset_env (env_name);
  EXPECT_EQ (tl::has_env (env_name), false);
}

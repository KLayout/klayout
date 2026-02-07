
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


#include "dbTechnology.h"
#include "tlUnitTest.h"
#include "tlFileUtils.h"

TEST(1_Basic)
{
  db::Technology tech ("name", "description");

  EXPECT_EQ (tech.name (), "name");
  EXPECT_EQ (tech.description (), "description");

  tech.set_name ("x");
  EXPECT_EQ (tech.name (), "x");

  tech.set_description ("y");
  EXPECT_EQ (tech.description (), "y");

  tech.set_grain_name ("a");
  EXPECT_EQ (tech.grain_name (), "a");

  tech.set_grain_name ("a");
  EXPECT_EQ (tech.grain_name (), "a");

  tech.set_dbu (2.5);
  EXPECT_EQ (tech.dbu (), 2.5);
}

TEST(2_BasePath)
{
  db::Technology tech ("x", "description");

  tech.set_default_base_path ("def");
  EXPECT_EQ (tech.default_base_path (), "def");

  tech.set_explicit_base_path ("$(tech_name)_plus");
  EXPECT_EQ (tech.explicit_base_path (), "$(tech_name)_plus");

  EXPECT_EQ (tech.base_path (), "x_plus");
  EXPECT_EQ (tech.correct_path (tl::combine_path ("x_plus", "z")), "z");

  tech.set_tech_file_path ("lyt");
  tech.set_explicit_base_path ("$(tech_file)_plus");
  EXPECT_EQ (tech.base_path (), "lyt_plus");

  tech.set_explicit_base_path ("$(tech_dir)_plus");
  EXPECT_EQ (tech.base_path (), "def_plus");
}

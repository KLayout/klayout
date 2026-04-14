
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


#include "layLibraryController.h"
#include "tlUnitTest.h"
#include "tlFileUtils.h"

TEST (1)
{
  std::string lib_file = tl::testdata () + "/lay/a.libdef";

  std::vector<lay::LibraryController::LibFileInfo> file_info;
  lay::LibraryController::read_lib_file (lib_file, "T1", file_info);

  tl_assert (file_info.size () == size_t (3));

  EXPECT_EQ (file_info[0].name, "");
  EXPECT_EQ (file_info[0].path, tl::combine_path (tl::absolute_path (lib_file), "noname.gds"));
  EXPECT_EQ (file_info[0].replicate, true);
  EXPECT_EQ (file_info[0].description, "");
  EXPECT_EQ (tl::join (file_info[0].tech.begin (), file_info[0].tech.end (), ","), "T1");

  EXPECT_EQ (file_info[1].name, "L2");
  EXPECT_EQ (file_info[1].path, tl::absolute_file_path (lib_file) + ".zzz");
  EXPECT_EQ (file_info[1].replicate, true);
  EXPECT_EQ (file_info[1].description, "Library L2");
  EXPECT_EQ (file_info[1].tech.size (), size_t (0));

  EXPECT_EQ (file_info[2].name, "L3");
  EXPECT_EQ (file_info[2].path, tl::combine_path (tl::absolute_path (lib_file), "subdir/l3.gds"));
  EXPECT_EQ (file_info[2].replicate, false);
  EXPECT_EQ (tl::join (file_info[2].tech.begin (), file_info[2].tech.end (), ","), "T2,T3");
}

TEST(2)
{
  std::string lib_file = tl::testdata () + "/lay/b.libdef";

  std::vector<lay::LibraryController::LibFileInfo> file_info;
  lay::LibraryController::read_lib_file (lib_file, "TX", file_info);

  tl_assert (file_info.size () == size_t (5));

  EXPECT_EQ (file_info[0].name, "L0");
  EXPECT_EQ (file_info[0].path, tl::combine_path (tl::absolute_path (lib_file), "l0.gds"));
  EXPECT_EQ (file_info[0].replicate, true);
  EXPECT_EQ (file_info[0].tech.size (), size_t (0));

  EXPECT_EQ (file_info[1].name, "");
  EXPECT_EQ (file_info[1].path, tl::combine_path (tl::absolute_path (lib_file), "noname.gds"));
  EXPECT_EQ (file_info[1].replicate, true);
  EXPECT_EQ (tl::join (file_info[1].tech.begin (), file_info[1].tech.end (), ","), "TX");

  EXPECT_EQ (file_info[4].name, "L4");
  EXPECT_EQ (file_info[4].path, tl::combine_path (tl::absolute_path (lib_file), "l4.gds"));
  EXPECT_EQ (file_info[4].replicate, true);
  EXPECT_EQ (tl::join (file_info[4].tech.begin (), file_info[4].tech.end (), ","), "TX");
}


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

#include "tlUniqueName.h"
#include "tlUnitTest.h"

#include <set>
#include <string>

//  basic tests
TEST(1)
{
  std::set<std::string> names;

  EXPECT_EQ (tl::unique_name ("A", names), "A");
  names.insert ("A");
  EXPECT_EQ (tl::unique_name ("A", names), "A$1");
  EXPECT_EQ (tl::unique_name ("A", names, "_"), "A_1");
  names.insert ("A$1");
  EXPECT_EQ (tl::unique_name ("A", names), "A$2");
  names.insert ("A$2");
  names.insert ("A$5");
  EXPECT_EQ (tl::unique_name ("A", names), "A$3");
  names.insert ("A$3");
  EXPECT_EQ (tl::unique_name ("A", names), "A$4");
  names.insert ("A$4");
  EXPECT_EQ (tl::unique_name ("A", names), "A$6");
  names.insert ("A$6");
  EXPECT_EQ (tl::unique_name ("B", names), "B");
  names.insert ("B");
  EXPECT_EQ (tl::unique_name ("", names), "");
  names.insert ("");
  EXPECT_EQ (tl::unique_name ("", names), "$1");
}

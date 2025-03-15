
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


#include "dbQuadTree.h"
#include "dbBoxConvert.h"
#include "tlUnitTest.h"
#include "tlString.h"

typedef db::quad_tree<db::DBox, db::box_convert<db::DBox>, size_t (1)> my_quad_tree;

std::string find_touching (const my_quad_tree &qt, const db::DBox &box)
{
  std::vector<std::string> v;
  auto i = qt.begin_touching (box);
  while (! i.at_end ()) {
    v.push_back (i->to_string ());
    ++i;
  }
  std::sort (v.begin (), v.end ());
  return tl::join (v, "/");
}

std::string find_overlapping (const my_quad_tree &qt, const db::DBox &box)
{
  std::vector<std::string> v;
  auto i = qt.begin_overlapping (box);
  while (! i.at_end ()) {
    v.push_back (i->to_string ());
    ++i;
  }
  std::sort (v.begin (), v.end ());
  return tl::join (v, "/");
}

TEST(basic)
{
  my_quad_tree tree;
  EXPECT_EQ (tree.empty (), true);
  EXPECT_EQ (tree.size (), size_t (0));

  tree.insert (db::DBox ());
  EXPECT_EQ (tree.empty (), true);
  EXPECT_EQ (tree.size (), size_t (0));

  tree.insert (db::DBox (-1, -2, 3, 4));
  EXPECT_EQ (tree.empty (), false);
  EXPECT_EQ (tree.size (), size_t (1));

  EXPECT_EQ (find_touching (tree, db::DBox (-2, 0, -1, 0)), "...");

  tree.insert (db::DBox (-1, -3, 3, 0));
  EXPECT_EQ (tree.empty (), false);
  EXPECT_EQ (tree.size (), size_t (1));

  EXPECT_EQ (find_touching (tree, db::DBox (-2, 0, -1, 0)), "...");
}


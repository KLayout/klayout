
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

#include <stdlib.h>

typedef db::quad_tree<db::DBox, db::box_convert<db::DBox>, size_t (1)> my_quad_tree;

std::string find_all (const my_quad_tree &qt)
{
  std::vector<std::string> v;
  auto i = qt.begin ();
  while (! i.at_end ()) {
    v.push_back (i->to_string ());
    ++i;
  }
  std::sort (v.begin (), v.end ());
  return tl::join (v, "/");
}

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

std::string find_touching_from_all (const my_quad_tree &qt, const db::DBox &box)
{
  std::vector<std::string> v;
  auto i = qt.begin ();
  while (! i.at_end ()) {
    if (i->touches (box)) {
      v.push_back (i->to_string ());
    }
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

std::string find_overlapping_from_all (const my_quad_tree &qt, const db::DBox &box)
{
  std::vector<std::string> v;
  auto i = qt.begin ();
  while (! i.at_end ()) {
    if (i->overlaps (box)) {
      v.push_back (i->to_string ());
    }
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
  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (tree.levels (), size_t (1));

  tree.insert (db::DBox ());
  EXPECT_EQ (tree.empty (), true);
  EXPECT_EQ (tree.size (), size_t (0));
  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (tree.levels (), size_t (1));

  tree.insert (db::DBox (-1, -2, 3, 4));
  EXPECT_EQ (tree.empty (), false);
  EXPECT_EQ (tree.size (), size_t (1));
  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (tree.levels (), size_t (1));

  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)");

  db::DBox bx;

  bx = db::DBox (-2, 0, -1, 0);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, -3, -1, -2);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, -3, -1, -2.5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 4, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 4.5, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 3, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 3, -1.5, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));

  tree.insert (db::DBox (-1, -3, 3, 0));
  EXPECT_EQ (tree.empty (), false);
  EXPECT_EQ (tree.size (), size_t (2));
  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (tree.levels (), size_t (1));

  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;3,0)");

  bx = db::DBox (-2, 0, -1, 0);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, -3, -1, -2);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, -3, -1, -2.5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 4, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 4.5, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 3, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 3, -1.5, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));

  tree.insert (db::DBox (-1, -3, -0.5, -2));
  EXPECT_EQ (tree.empty (), false);
  EXPECT_EQ (tree.size (), size_t (3));
  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (tree.levels (), size_t (2));

  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;3,0)");

  bx = db::DBox (-2, 0, -1, 0);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, -3, -1, -2);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, -3, -1, -2.5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 4, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 4.5, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 3, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 3, -1.5, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));

  tree.insert (db::DBox (-1, -3, -0.5, 2));
  EXPECT_EQ (tree.empty (), false);
  EXPECT_EQ (tree.size (), size_t (4));
  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (tree.levels (), size_t (2));

  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");

  bx = db::DBox (-2, 0, -1, 0);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, -3, -1, -2);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, -3, -1, -2.5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 4, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 4.5, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 3, -1, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
  bx = db::DBox (-2, 3, -1.5, 5);
  EXPECT_EQ (find_touching (tree, bx), find_touching_from_all (tree, bx));
  EXPECT_EQ (find_overlapping (tree, bx), find_overlapping_from_all (tree, bx));
}

TEST(remove)
{
  my_quad_tree tree;
  tree.insert (db::DBox (-1, -2, 3, 4));
  tree.insert (db::DBox (-1, -3, 3, 0));
  tree.insert (db::DBox (-1, -3, -0.5, -2));
  tree.insert (db::DBox (-1, -3, -0.5, 2));

  EXPECT_EQ (tree.check (), true);

  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");

  EXPECT_EQ (tree.erase (db::DBox (-1, -3, -0.5, -1)), false);
  EXPECT_EQ (tree.erase (db::DBox (-1, -3, -0.5, -2)), true);
  EXPECT_EQ (tree.check (), true);

  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,2)/(-1,-3;3,0)");

  while (! tree.empty ()) {
    EXPECT_EQ (tree.erase (*tree.begin ()), true);
    EXPECT_EQ (tree.check (), true);
  }

  EXPECT_EQ (tree.size (), size_t (0));
  EXPECT_EQ (tree.levels (), size_t (1));
}

TEST(grow)
{
  my_quad_tree tree;
  tree.insert (db::DBox (-1, -2, 3, 4));
  tree.insert (db::DBox (-1, -3, 3, 0));
  tree.insert (db::DBox (-1, -3, -0.5, -2));
  tree.insert (db::DBox (-1, -3, -0.5, 2));
  EXPECT_EQ (tree.levels (), size_t (2));
  tree.insert (db::DBox (-100, -3, -99, 2));
  EXPECT_EQ (tree.levels (), size_t (8));

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)/(-100,-3;-99,2)");
  EXPECT_EQ (find_overlapping (tree, db::DBox (-100, -100, -90, 100)), "(-100,-3;-99,2)");

  bool r = true;
  while (r && ! tree.empty ()) {
    r = tree.erase (*tree.begin ());
    EXPECT_EQ (r, true);
    EXPECT_EQ (tree.check (), true);
  }

  EXPECT_EQ (tree.size (), size_t (0));
  EXPECT_EQ (tree.levels (), size_t (1));
}

TEST(grow2)
{
  my_quad_tree tree;
  tree.insert (db::DBox (-1, -2, 3, 4));
  tree.insert (db::DBox (-1, -3, 3, 0));
  tree.insert (db::DBox (-1, -3, -0.5, -2));
  tree.insert (db::DBox (-1, -3, -0.5, 2));
  EXPECT_EQ (tree.levels (), size_t (2));
  tree.insert (db::DBox (-100, -3, -99, -1));
  EXPECT_EQ (tree.levels (), size_t (8));

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)/(-100,-3;-99,-1)");
  EXPECT_EQ (find_overlapping (tree, db::DBox (-100, -100, -90, 100)), "(-100,-3;-99,-1)");

  bool r = true;
  while (r && ! tree.empty ()) {
    r = tree.erase (*tree.begin ());
    EXPECT_EQ (r, true);
    EXPECT_EQ (tree.check (), true);
  }

  EXPECT_EQ (tree.size (), size_t (0));
  EXPECT_EQ (tree.levels (), size_t (1));
}

TEST(clear)
{
  my_quad_tree tree;
  tree.insert (db::DBox (-1, -2, 3, 4));
  tree.insert (db::DBox (-1, -3, 3, 0));
  tree.insert (db::DBox (-1, -3, -0.5, -2));
  tree.insert (db::DBox (-1, -3, -0.5, 2));

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");

  tree.clear ();

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (tree.empty (), true);
  EXPECT_EQ (tree.size (), size_t (0));
  EXPECT_EQ (tree.levels (), size_t (1));
  EXPECT_EQ (find_all (tree), "");
}

TEST(copy)
{
  my_quad_tree tree;
  tree.insert (db::DBox (-1, -2, 3, 4));
  tree.insert (db::DBox (-1, -3, 3, 0));
  tree.insert (db::DBox (-1, -3, -0.5, -2));
  tree.insert (db::DBox (-1, -3, -0.5, 2));

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");
  EXPECT_EQ (tree.levels (), size_t (2));

  my_quad_tree tree2 (tree);
  tree.clear ();

  EXPECT_EQ (tree2.check (), true);
  EXPECT_EQ (find_all (tree2), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");
  EXPECT_EQ (tree2.levels (), size_t (2));
}

TEST(assign)
{
  my_quad_tree tree;
  tree.insert (db::DBox (-1, -2, 3, 4));
  tree.insert (db::DBox (-1, -3, 3, 0));
  tree.insert (db::DBox (-1, -3, -0.5, -2));
  tree.insert (db::DBox (-1, -3, -0.5, 2));

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");
  EXPECT_EQ (tree.levels (), size_t (2));

  my_quad_tree tree2;
  tree2 = tree;
  tree.clear ();

  EXPECT_EQ (tree2.check (), true);
  EXPECT_EQ (find_all (tree2), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");
  EXPECT_EQ (tree2.levels (), size_t (2));
}

TEST(swap)
{
  my_quad_tree tree;
  tree.insert (db::DBox (-1, -2, 3, 4));
  tree.insert (db::DBox (-1, -3, 3, 0));
  tree.insert (db::DBox (-1, -3, -0.5, -2));
  tree.insert (db::DBox (-1, -3, -0.5, 2));

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");
  EXPECT_EQ (tree.levels (), size_t (2));

  my_quad_tree tree2;
  tree2.swap (tree);

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (tree.empty (), true);
  EXPECT_EQ (find_all (tree), "");
  EXPECT_EQ (tree.levels (), size_t (1));

  EXPECT_EQ (tree2.check (), true);
  EXPECT_EQ (find_all (tree2), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");
  EXPECT_EQ (tree2.levels (), size_t (2));
}

TEST(move)
{
  my_quad_tree tree;
  tree.insert (db::DBox (-1, -2, 3, 4));
  tree.insert (db::DBox (-1, -3, 3, 0));
  tree.insert (db::DBox (-1, -3, -0.5, -2));
  tree.insert (db::DBox (-1, -3, -0.5, 2));

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");
  EXPECT_EQ (tree.levels (), size_t (2));

  my_quad_tree tree2;
  tree2 = std::move (tree);

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (tree.empty (), true);
  EXPECT_EQ (find_all (tree), "");
  EXPECT_EQ (tree.levels (), size_t (1));

  EXPECT_EQ (tree2.check (), true);
  EXPECT_EQ (find_all (tree2), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");
  EXPECT_EQ (tree2.levels (), size_t (2));
}

TEST(move_ctor)
{
  my_quad_tree tree;
  tree.insert (db::DBox (-1, -2, 3, 4));
  tree.insert (db::DBox (-1, -3, 3, 0));
  tree.insert (db::DBox (-1, -3, -0.5, -2));
  tree.insert (db::DBox (-1, -3, -0.5, 2));

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (find_all (tree), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");
  EXPECT_EQ (tree.levels (), size_t (2));

  my_quad_tree tree2 (std::move (tree));

  EXPECT_EQ (tree.check (), true);
  EXPECT_EQ (tree.empty (), true);
  EXPECT_EQ (find_all (tree), "");
  EXPECT_EQ (tree.levels (), size_t (1));

  EXPECT_EQ (tree2.check (), true);
  EXPECT_EQ (find_all (tree2), "(-1,-2;3,4)/(-1,-3;-0.5,-2)/(-1,-3;-0.5,2)/(-1,-3;3,0)");
  EXPECT_EQ (tree2.levels (), size_t (2));
}

static double rvalue ()
{
  return ((rand () % 1000000) - 5000) * 0.001;
}

static db::DBox rbox ()
{
  return db::DBox (db::DPoint (rvalue (), rvalue ()), db::DPoint (rvalue (), rvalue ()));
}

TEST(many)
{
  return; // @@@
  my_quad_tree tree;

  unsigned int n = 1000;

  for (unsigned int i = 0; i < n; ++i) {
    tree.insert (rbox ());
  }

  EXPECT_EQ (tree.check (), true);

  bool r = true;
  while (r && ! tree.empty ()) {
    r = tree.erase (*tree.begin ());
    EXPECT_EQ (r, true);
    EXPECT_EQ (tree.check (), true);
  }
}

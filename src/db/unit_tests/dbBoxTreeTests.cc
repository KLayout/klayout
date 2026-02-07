
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


#include "dbBoxTree.h"
#include "dbBoxConvert.h"
#include "tlUnitTest.h"
#include "tlTimer.h"
#include "tlLog.h"

#include <iostream>
#include <set>
#include <stdlib.h>

template <class Box>
Box qbox (int q, const Box &bbox) 
{
  typedef typename Box::coord_type coord_type;
  typedef db::point<coord_type> point_type;
  coord_type dx = bbox.width () / 2;
  coord_type dy = bbox.height () / 2;
  point_type o ((((q + 3) & 2) >> 1) * dx, (((q + 2) & 2) >> 1) * dy);
  o += bbox.p1 ();
  return Box (o, o + point_type (dx, dy));
}

template <class Box, class Tree>
void print_tree_node (const Tree *tree, const Box &bbox, size_t pos, db::box_tree_node<Tree> *node, const std::string &in)
{
  std::cout << in << "x [\n";
  if (! node) { 
    for (size_t i = pos; i < pos + tree->size (); ++i) {
      std::cout << in << "  " << tree->elements ()[i]->to_string () << "\n";
    }
  } else {
    for (size_t i = pos; i < pos + node->lenq (-1); ++i) {
      std::cout << in << "  " << tree->elements ()[i]->to_string () << "\n";
    }
  }
  std::cout << in << "]\n";
  if (node) {
    pos += node->lenq (-1);
    for (int q = 0; q < 4; ++q) {
      Box qb (qbox (q, bbox));
      if (node->child (q)) {
        std::cout << in << q << " (" << qb.to_string () << ") [\n";
        print_tree_node (tree, qb, pos, node->child (q), in + "  "); 
        std::cout << in << "]\n";
      } else {
        std::cout << in << q << " (" << qb.to_string () << ") [\n";
        for (size_t i = pos; i < pos + node->lenq (q); ++i) {
          std::cout << in << "  " << tree->elements ()[i]->to_string () << " #" << i << "\n";
        }
        std::cout << in << "]\n";
      }
      pos += node->lenq (q);
    }
  }
}

template <class Tree>
void print_tree (const Tree &t, db::Box bbox)
{
  std::cout << "size = " << t.size () << "\n";
  print_tree_node (&t, bbox, 0, t.root (), "> ");
}

template <class Box, class Tree>
void print_unstable_tree_node (const Tree *tree, const Box &bbox, size_t pos, db::box_tree_node<Tree> *node, const std::string &in)
{
  std::cout << in << "x [\n";
  if (! node) { 
    for (size_t i = pos; i < pos + tree->size (); ++i) {
      std::cout << in << "  " << tree->objects ()[i].to_string () << "\n";
    }
  } else {
    for (size_t i = pos; i < pos + node->lenq (-1); ++i) {
      std::cout << in << "  " << tree->objects ()[i].to_string () << "\n";
    }
  }
  std::cout << in << "]\n";
  if (node) {
    pos += node->lenq (-1);
    for (int q = 0; q < 4; ++q) {
      Box qb (qbox (q, bbox));
      if (node->child (q)) {
        std::cout << in << q << " (" << qb.to_string () << ") [\n";
        print_unstable_tree_node (tree, qb, pos, node->child (q), in + "  "); 
        std::cout << in << "]\n";
      } else {
        std::cout << in << q << " (" << qb.to_string () << ") [\n";
        for (size_t i = pos; i < pos + node->lenq (q); ++i) {
          std::cout << in << "  " << tree->objects ()[i].to_string () << " #" << i << "\n";
        }
        std::cout << in << "]\n";
      }
      pos += node->lenq (q);
    }
  }
}

template <class Tree>
void print_unstable_tree (const Tree &t, db::Box bbox)
{
  std::cout << "size = " << t.size () << "\n";
  print_unstable_tree_node (&t, bbox, 0, t.root (), "> ");
}

struct Box2Box {
  typedef db::simple_bbox_tag complexity;
  const db::Box &operator() (const db::Box &b) const { return b; }
};

struct Box2BoxCmplx {
  typedef db::complex_bbox_tag complexity;
  const db::Box &operator() (const db::Box &b) const { return b; }
};

typedef db::box_tree<db::Box, db::Box, Box2Box, 4, 0> TestTree;
typedef db::box_tree<db::Box, db::Box, Box2BoxCmplx, 4, 0> TestTreeCmplx;

typedef db::box_tree<db::Box, db::Box, Box2Box> TestTreeL;
typedef db::box_tree<db::Box, db::Box, Box2BoxCmplx> TestTreeCmplxL;

typedef db::unstable_box_tree<db::Box, db::Box, Box2Box, 4, 0> UnstableTestTree;
typedef db::unstable_box_tree<db::Box, db::Box, Box2BoxCmplx, 4, 0> UnstableTestTreeCmplx;

typedef db::unstable_box_tree<db::Box, db::Box, Box2Box> UnstableTestTreeL;
typedef db::unstable_box_tree<db::Box, db::Box, Box2BoxCmplx> UnstableTestTreeCmplxL;

template <class Tree, class Box, class BoxConv>
static void test_tree_overlap (tl::TestBase *_this, const Tree &t, const Box &b, BoxConv conv)
{
  typedef typename Tree::object_type value_type; 

  if (tl::verbose ()) {
    std::cout << "Testing vs. " << b << " overlapping" << std::endl;
  }

  std::set <const value_type *> good_idx;
  for (typename Tree::const_iterator e = t.begin (); e != t.end (); ++e) {
    if (b.overlaps (*e)) {
      good_idx.insert (&*e);
    }
  }
  
  if (tl::verbose ()) {
    for (typename Tree::const_iterator e = t.begin (); e != t.end (); ++e) {
      std::cout << " v=" << *e << std::endl;
    }
  }

  typename Tree::overlapping_iterator i = t.begin_overlapping (b, conv);
  while (! i.at_end ()) {
    if (good_idx.find (&*i) == good_idx.end ()) {
      FAIL_ARG ("not found in good indices list", *i);
    } else {
      good_idx.erase (&*i);
    }
    if (tl::verbose ()) {
      std::cout << *i << std::endl;
    }
    ++i;
  }

  EXPECT_EQ (good_idx.size (), size_t (0));
}

template <class Tree, class Box, class BoxConv>
static void test_tree_touching (tl::TestBase *_this, const Tree &t, const Box &b, BoxConv conv)
{
  typedef typename Tree::object_type value_type; 

  if (tl::verbose ()) {
    std::cout << "Testing vs. " << b << " touching" << std::endl;
  }

  std::set <const value_type *> good_idx;
  for (typename Tree::const_iterator e = t.begin (); e != t.end (); ++e) {
    if (b.touches (*e)) {
      good_idx.insert (&*e);
    }
  }
  
  if (tl::verbose ()) {
    for (typename Tree::const_iterator e = t.begin (); e != t.end (); ++e) {
      std::cout << " v=" << *e << std::endl;
    }
  }

  typename Tree::touching_iterator i = t.begin_touching (b, conv);
  while (! i.at_end ()) {
    if (good_idx.find (&*i) == good_idx.end ()) {
      FAIL_ARG ("not found in good indices list", *i);
    } else {
      good_idx.erase (&*i);
    }
    if (tl::verbose ()) {
      std::cout << *i << std::endl;
    }
    ++i;
  }

  EXPECT_EQ (good_idx.size (), size_t (0));
}

inline int rvalue () 
{
  return (rand () % 10000) - 5000;
}

inline db::Box rboxx ()
{
  int x = rvalue ();
  int y = 100;
  return db::Box (x, y, x + rvalue () % 20, y + 200);
}

inline db::Box rboxy ()
{
  int x = -100;
  int y = rvalue ();
  return db::Box (x, y, x + 200, y + rvalue () % 20);
}

inline db::Box rbox ()
{
  int x = rvalue ();
  int y = rvalue ();
  return db::Box (x, y, x + rvalue () % 200, y + rvalue () % 200);
}

namespace
{

class TestMemStatistics
  : public db::MemStatistics
{
public:
  TestMemStatistics ()
    : used (0), reqd (0)
  { }

  virtual void add (const std::type_info & /*ti*/, void * /*ptr*/, size_t r, size_t u, void * /*parent*/, purpose_t /*purpose*/ = None, int /*cat*/ = 0)
  {
    used += u;
    reqd += r;
  }

  void clear ()
  {
    used = reqd = 0;
  }

public:
  size_t used, reqd;
};

}

TEST(0)
{
  Box2Box conv;
  db::Coord m = std::numeric_limits<db::Coord>::max ();
  
  db::Box b(-m, -m, m, m);
  TestTree t;

  unsigned int bitset;
  TestTree::touching_iterator it;

  t.insert (db::Box (10, 20, 20, 25));

  t.sort (conv);

  EXPECT_EQ (t.size (), size_t (1));

  it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
  bitset = 0;
  while (! it.at_end ()) {
    unsigned int b = 0;
    for (TestTree::const_iterator e = t.begin (); e != t.end (); ++e, ++b) {
      if (&*e == &*it) {
        bitset |= (1 << b);
      }
    }
    ++it;
  }

  EXPECT_EQ (bitset, (unsigned int) 0x1);

  t.insert (db::Box (-10, 20, 0, 100));
  t.insert (db::Box (-10, -20, 20, -15));
  t.insert (db::Box (-10, -20, 20, -10));
  t.insert (db::Box (-10, -20, 0, -9));
  t.insert (db::Box (10, 20, 20, 50));
  t.insert (db::Box (-10, 20, -5, 100));
  t.insert (db::Box (-10, -20, 20, 22));
  t.insert (db::Box (-10, -20, 5, -10));
  t.sort (conv);

  EXPECT_EQ (t.size (), size_t (9));

  it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
  bitset = 0;
  while (! it.at_end ()) {
    unsigned int b = 0;
    for (TestTree::const_iterator e = t.begin (); e != t.end (); ++e, ++b) {
      if (&*e == &*it) {
        bitset |= (1 << b);
      }
    }
    ++it;
  }

  EXPECT_EQ (bitset, (unsigned int) 0x1ff);

  it = t.begin_touching (db::Box (-10, 20, -9, 21), conv);
  bitset = 0;
  while (! it.at_end ()) {
    unsigned int b = 0;
    for (TestTree::const_iterator e = t.begin (); e != t.end (); ++e, ++b) {
      if (&*e == &*it) {
        bitset |= (1 << b);
      }
    }
    ++it;
  }

  EXPECT_EQ (bitset, (unsigned int) 0xc2);

  it = t.begin_touching (db::Box (-20, 20, -19, 21), conv);
  bitset = 0;
  while (! it.at_end ()) {
    unsigned int b = 0;
    for (TestTree::const_iterator e = t.begin (); e != t.end (); ++e, ++b) {
      if (&*e == &*it) {
        bitset |= (1 << b);
      }
    }
    ++it;
  }

  EXPECT_EQ (bitset, (unsigned int) 0);
}

TEST(1)
{
  Box2Box conv;
  
  db::Box b(-10, -10, 10, 10);
  TestTree t;
  t.sort (conv);
  test_tree_overlap (_this, t, b, conv);
  test_tree_touching (_this, t, b, conv);

  t.insert (db::Box (10, 20, 20, 100));
  t.insert (db::Box (-10, 20, 20, 100));
  t.insert (db::Box (-10, -20, 20, 100));
  t.insert (db::Box (-10, -20, 20, -10));
  t.insert (db::Box (-10, -20, 20, -9));
  t.sort (conv);
  test_tree_overlap (_this, t, b, conv);
  test_tree_touching (_this, t, b, conv);
}

TEST(2)
{
  Box2Box conv;
  TestTree t;

  int n = 231;

  for (int i = 0; i < n; ++i) {
    t.insert (rbox ());
  }
  t.sort (conv);

  for (int i = 0; i < n; ++i) {
    db::Box b (rbox ());
    test_tree_overlap (_this, t, b, conv);
    test_tree_touching (_this, t, b, conv);
  }

}


TEST(3)
{
  Box2Box conv;
  TestTree t;

  int n = 215;

  for (int i = 0; i < n; ++i) {
    //  insert some empty boxes ..
    if (rvalue () % 3 == 0) {
      t.insert (db::Box ());
    } else {
      t.insert (rbox ());
    }
  }
  t.sort (conv);

  for (int i = 0; i < n; ++i) {
    db::Box b (rbox ());
    test_tree_overlap (_this, t, b, conv);
    test_tree_touching (_this, t, b, conv);
  }

}

TEST(1C)
{
  Box2BoxCmplx conv;
  
  db::Box b(-10, -10, 10, 10);
  TestTreeCmplx t;
  t.sort (conv);
  test_tree_overlap (_this, t, b, conv);
  test_tree_touching (_this, t, b, conv);

  t.insert (db::Box (10, 20, 20, 100));
  t.insert (db::Box (-10, 20, 20, 100));
  t.insert (db::Box (-10, -20, 20, 100));
  t.insert (db::Box (-10, -20, 20, -10));
  t.insert (db::Box (-10, -20, 20, -9));
  t.sort (conv);
  test_tree_overlap (_this, t, b, conv);
  test_tree_touching (_this, t, b, conv);
}

TEST(2C)
{
  Box2BoxCmplx conv;
  TestTreeCmplx t;

  int n = 231;

  for (int i = 0; i < n; ++i) {
    t.insert (rbox ());
  }
  t.sort (conv);

  for (int i = 0; i < n; ++i) {
    db::Box b (rbox ());
    test_tree_overlap (_this, t, b, conv);
    test_tree_touching (_this, t, b, conv);
  }

}


TEST(3C)
{
  Box2BoxCmplx conv;
  TestTreeCmplx t;

  int n = 215;

  for (int i = 0; i < n; ++i) {
    //  insert some empty boxes ..
    if (rvalue () % 3 == 0) {
      t.insert (db::Box ());
    } else {
      t.insert (rbox ());
    }
  }
  t.sort (conv);

  for (int i = 0; i < n; ++i) {
    db::Box b (rbox ());
    test_tree_overlap (_this, t, b, conv);
    test_tree_touching (_this, t, b, conv);
  }

}

TEST(4)
{
  Box2BoxCmplx conv;
  TestTreeCmplxL t;

  int n = 2000000;
  int nempty = 0;

  db::Box bbox;
  for (int i = 0; i < n; ++i) {
    //  insert some empty boxes ..
    db::Box bx;
    if (rvalue () % 3000 == 0) {
      ++nempty;
    } else {
      bx = rbox ();
    }
    t.insert (bx);
    bbox += bx;
  }
  {
    tl::SelfTimer timer ("test 4 sort");
    t.sort (conv);
  }
  {
    tl::SelfTimer timer ("test 4 traverse");
    db::Coord m = std::numeric_limits<db::Coord>::max ();
    size_t n = 0;
    for (unsigned int i = 0; i < 10; ++i) {
      TestTreeCmplxL::touching_iterator it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
      while (!it.at_end ()) {
        ++it;
        ++n;
      }
    }
    EXPECT_EQ (n, (t.size () - nempty) * 10);
  }
  {
    tl::SelfTimer timer ("test 4 lookup");
    for (unsigned int i = 0; i < 10; ++i) {
      for (unsigned int j = 0; j < 10; ++j) {
        db::Box sbox (bbox.left () + (bbox.width () * i) / 10, 
                      bbox.bottom () + (bbox.height () * j) / 10, 
                      bbox.left () + (bbox.width () * (i + 1)) / 10, 
                      bbox.bottom () + (bbox.height () * (j + 1)) / 10);
        TestTreeCmplxL::touching_iterator it = t.begin_touching (sbox, conv);
        while (!it.at_end ()) {
          ++it;
        }
      }
    }
  }

  TestMemStatistics ms;
  t.mem_stat (&ms, db::MemStatistics::None, 0);
  tl::info << "Memory: " << ms.used;
}

TEST(4A)
{
  Box2BoxCmplx conv;
  TestTreeCmplxL t;

  int n = 2000000;
  int nempty = 0;

  db::Box bbox;
  for (int i = 0; i < n; ++i) {
    //  insert some empty boxes ..
    db::Box bx;
    if (rvalue () % 3000 == 0) {
      ++nempty;
    } else {
      bx = rboxx ();
    }
    t.insert (bx);
    bbox += bx;
  }
  {
    tl::SelfTimer timer ("test 4a sort");
    t.sort (conv);
  }
  {
    tl::SelfTimer timer ("test 4a traverse");
    db::Coord m = std::numeric_limits<db::Coord>::max ();
    size_t n = 0;
    for (unsigned int i = 0; i < 10; ++i) {
      TestTreeCmplxL::touching_iterator it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
      while (!it.at_end ()) {
        ++it;
        ++n;
      }
    }
    EXPECT_EQ (n, (t.size () - nempty) * 10);
  }
  {
    tl::SelfTimer timer ("test 4a lookup");
    for (unsigned int i = 0; i < 10; ++i) {
      for (unsigned int j = 0; j < 10; ++j) {
        db::Box sbox (bbox.left () + (bbox.width () * i) / 10, 
                      bbox.bottom () + (bbox.height () * j) / 10, 
                      bbox.left () + (bbox.width () * (i + 1)) / 10, 
                      bbox.bottom () + (bbox.height () * (j + 1)) / 10);
        TestTreeCmplxL::touching_iterator it = t.begin_touching (sbox, conv);
        while (!it.at_end ()) {
          ++it;
        }
      }
    }
  }

  TestMemStatistics ms;
  t.mem_stat (&ms, db::MemStatistics::None, 0);
  tl::info << "Memory: " << ms.used;
}

TEST(4B)
{
  Box2BoxCmplx conv;
  TestTreeCmplxL t;

  int n = 2000000;
  int nempty = 0;

  db::Box bbox;
  for (int i = 0; i < n; ++i) {
    //  insert some empty boxes ..
    db::Box bx;
    if (rvalue () % 3000 == 0) {
      ++nempty;
    } else {
      bx = rboxy ();
    }
    t.insert (bx);
    bbox += bx;
  }
  {
    tl::SelfTimer timer ("test 4b sort");
    t.sort (conv);
  }
  {
    tl::SelfTimer timer ("test 4b traverse");
    db::Coord m = std::numeric_limits<db::Coord>::max ();
    size_t n = 0;
    for (unsigned int i = 0; i < 10; ++i) {
      TestTreeCmplxL::touching_iterator it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
      while (!it.at_end ()) {
        ++it;
        ++n;
      }
    }
    EXPECT_EQ (n, (t.size () - nempty) * 10);
  }
  {
    tl::SelfTimer timer ("test 4b lookup");
    for (unsigned int i = 0; i < 10; ++i) {
      for (unsigned int j = 0; j < 10; ++j) {
        db::Box sbox (bbox.left () + (bbox.width () * i) / 10, 
                      bbox.bottom () + (bbox.height () * j) / 10, 
                      bbox.left () + (bbox.width () * (i + 1)) / 10, 
                      bbox.bottom () + (bbox.height () * (j + 1)) / 10);
        TestTreeCmplxL::touching_iterator it = t.begin_touching (sbox, conv);
        while (!it.at_end ()) {
          ++it;
        }
      }
    }
  }

  TestMemStatistics ms;
  t.mem_stat (&ms, db::MemStatistics::None, 0);
  tl::info << "Memory: " << ms.used;
}

TEST(5)
{
  Box2BoxCmplx conv;
  TestTreeCmplxL t;

  int n = 2000000;

  for (int i = n - 1; i >= 0; --i) {
    t.insert (db::Box (i, n - i - 1, i + 1000, n - i - 1 + 1000));
  }
  {
    tl::SelfTimer timer ("test 5 sort");
    t.sort (conv);
  }
  {
    tl::SelfTimer timer ("test 5 traverse");
    db::Coord m = std::numeric_limits<db::Coord>::max ();
    size_t n = 0;
    for (unsigned int i = 0; i < 10; ++i) {
      TestTreeCmplxL::touching_iterator it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
      while (!it.at_end ()) {
        ++it;
        ++n;
      }
    }
    EXPECT_EQ (n, t.size () * 10);
  }

  TestMemStatistics ms;
  t.mem_stat (&ms, db::MemStatistics::None, 0);
  tl::info << "Memory: " << ms.used;
}

TEST(0U)
{
  Box2Box conv;
  db::Coord m = std::numeric_limits<db::Coord>::max ();
  
  db::Box b(-m, -m, m, m);
  UnstableTestTree t;

  unsigned int bitset;
  UnstableTestTree::touching_iterator it;

  t.insert (db::Box (10, 20, 20, 25));

  t.sort (conv);

  EXPECT_EQ (t.size (), size_t (1));

  it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
  bitset = 0;
  while (! it.at_end ()) {
    bitset |= (1 << (it.index ()));
    ++it;
  }

  EXPECT_EQ (bitset, (unsigned int) 0x1);

  t.insert (db::Box (-10, 20, 0, 100));
  t.insert (db::Box (-10, -20, 20, -15));
  t.insert (db::Box (-10, -20, 20, -10));
  t.insert (db::Box (-10, -20, 0, -9));
  t.insert (db::Box (10, 20, 20, 50));
  t.insert (db::Box (-10, 20, -5, 100));
  t.insert (db::Box (-10, -20, 20, 22));
  t.insert (db::Box (-10, -20, 5, -10));
  t.sort (conv);

  EXPECT_EQ (t.size (), size_t (9));

  it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
  bitset = 0;
  while (! it.at_end ()) {
    bitset |= (1 << (it.index ()));
    ++it;
  }

  EXPECT_EQ (bitset, (unsigned int) 0x1ff);

  it = t.begin_touching (db::Box (-10, 20, -9, 21), conv);
  bitset = 0;
  while (! it.at_end ()) {
    bitset |= (1 << (it.index ()));
    ++it;
  }

  EXPECT_EQ (bitset, (unsigned int) 0x31);

  it = t.begin_touching (db::Box (-20, 20, -19, 21), conv);
  bitset = 0;
  while (! it.at_end ()) {
    bitset |= (1 << (it.index ()));
    ++it;
  }

  EXPECT_EQ (bitset, (unsigned int) 0);
}

TEST(1U)
{
  Box2Box conv;
  
  db::Box b(-10, -10, 10, 10);
  UnstableTestTree t;
  t.sort (conv);
  test_tree_overlap (_this, t, b, conv);
  test_tree_touching (_this, t, b, conv);

  t.insert (db::Box (10, 20, 20, 100));
  t.insert (db::Box (-10, 20, 20, 100));
  t.insert (db::Box (-10, -20, 20, 100));
  t.insert (db::Box (-10, -20, 20, -10));
  t.insert (db::Box (-10, -20, 20, -9));
  t.sort (conv);
  test_tree_overlap (_this, t, b, conv);
  test_tree_touching (_this, t, b, conv);
}

TEST(2U)
{
  Box2Box conv;
  UnstableTestTree t;

  int n = 231;

  for (int i = 0; i < n; ++i) {
    t.insert (rbox ());
  }
  t.sort (conv);

  for (int i = 0; i < n; ++i) {
    db::Box b (rbox ());
    test_tree_overlap (_this, t, b, conv);
    test_tree_touching (_this, t, b, conv);
  }

}


TEST(3U)
{
  Box2Box conv;
  UnstableTestTree t;

  int n = 215;

  for (int i = 0; i < n; ++i) {
    //  insert some empty boxes ..
    if (rvalue () % 3 == 0) {
      t.insert (db::Box ());
    } else {
      t.insert (rbox ());
    }
  }
  t.sort (conv);

  for (int i = 0; i < n; ++i) {
    db::Box b (rbox ());
    test_tree_overlap (_this, t, b, conv);
    test_tree_touching (_this, t, b, conv);
  }

}

TEST(1CU)
{
  Box2BoxCmplx conv;
  
  db::Box b(-10, -10, 10, 10);
  UnstableTestTreeCmplx t;
  t.sort (conv);
  test_tree_overlap (_this, t, b, conv);
  test_tree_touching (_this, t, b, conv);

  t.insert (db::Box (10, 20, 20, 100));
  t.insert (db::Box (-10, 20, 20, 100));
  t.insert (db::Box (-10, -20, 20, 100));
  t.insert (db::Box (-10, -20, 20, -10));
  t.insert (db::Box (-10, -20, 20, -9));
  t.sort (conv);
  test_tree_overlap (_this, t, b, conv);
  test_tree_touching (_this, t, b, conv);
}

TEST(2CU)
{
  Box2BoxCmplx conv;
  UnstableTestTreeCmplx t;

  int n = 231;

  db::Box bbox;
  for (int i = 0; i < n; ++i) {
    db::Box b (rbox ());
    bbox += b;
    t.insert (b);
  }
  t.sort (conv);

  for (int i = 0; i < n; ++i) {
    db::Box b (rbox ());
    test_tree_overlap (_this, t, b, conv);
    test_tree_touching (_this, t, b, conv);
  }

}

TEST(3CU)
{
  Box2BoxCmplx conv;
  UnstableTestTreeCmplx t;

  int n = 215;

  for (int i = 0; i < n; ++i) {
    //  insert some empty boxes ..
    if (rvalue () % 3 == 0) {
      t.insert (db::Box ());
    } else {
      t.insert (rbox ());
    }
  }
  t.sort (conv);

  for (int i = 0; i < n; ++i) {
    db::Box b (rbox ());
    test_tree_overlap (_this, t, b, conv);
    test_tree_touching (_this, t, b, conv);
  }

}

TEST(4U)
{
  Box2BoxCmplx conv;
  UnstableTestTreeCmplxL t;

  int n = 2000000;
  int nempty = 0;

  db::Box bbox;
  for (int i = 0; i < n; ++i) {
    //  insert some empty boxes ..
    db::Box bx;
    if (rvalue () % 3000 == 0) {
      ++nempty;
    } else {
      bx = rbox ();
    }
    t.insert (bx);
    bbox += bx;
  }
  {
    tl::SelfTimer timer ("test 4 sort");
    t.sort (conv);
  }
  {
    tl::SelfTimer timer ("test 4 traverse");
    db::Coord m = std::numeric_limits<db::Coord>::max ();
    size_t n = 0;
    for (unsigned int i = 0; i < 10; ++i) {
      UnstableTestTreeCmplxL::touching_iterator it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
      while (!it.at_end ()) {
        ++it;
        ++n;
      }
    }
    EXPECT_EQ (n, (t.size () - nempty) * 10);
  }
  {
    tl::SelfTimer timer ("test 4 lookup");
    for (unsigned int i = 0; i < 10; ++i) {
      for (unsigned int j = 0; j < 10; ++j) {
        db::Box sbox (bbox.left () + (bbox.width () * i) / 10, 
                      bbox.bottom () + (bbox.height () * j) / 10, 
                      bbox.left () + (bbox.width () * (i + 1)) / 10, 
                      bbox.bottom () + (bbox.height () * (j + 1)) / 10);
        UnstableTestTreeCmplxL::touching_iterator it = t.begin_touching (sbox, conv);
        while (!it.at_end ()) {
          ++it;
        }
      }
    }
  }

  TestMemStatistics ms;
  t.mem_stat (&ms, db::MemStatistics::None, 0);
  tl::info << "Memory: " << ms.used;
}

TEST(5U)
{
  Box2BoxCmplx conv;
  UnstableTestTreeCmplxL t;

  int n = 2000000;

  for (int i = n - 1; i >= 0; --i) {
    t.insert (db::Box (i, n - i - 1, i + 1000, n - i - 1 + 1000));
  }
  {
    tl::SelfTimer timer ("test 5 sort");
    t.sort (conv);
  }
  {
    tl::SelfTimer timer ("test 5 traverse");
    db::Coord m = std::numeric_limits<db::Coord>::max ();
    size_t n = 0;
    for (unsigned int i = 0; i < 10; ++i) {
      UnstableTestTreeCmplxL::touching_iterator it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
      while (!it.at_end ()) {
        ++it;
        ++n;
      }
    }
    EXPECT_EQ (n, t.size () * 10);
  }

  TestMemStatistics ms;
  t.mem_stat (&ms, db::MemStatistics::None, 0);
  tl::info << "Memory: " << ms.used;
}

TEST(6)
{
  Box2Box conv;
  TestTree t;

  int n = 1000;
  for (int i = 0; i < n; ++i) {
    t.insert (db::Box (1, 1, 1, 1));
  }
  t.sort (conv);

  for (int i = 0; i < n; ++i) {
    db::Box b (0, 0, 10, 10);
    test_tree_overlap (_this, t, b, conv);
    test_tree_touching (_this, t, b, conv);
  }

}

TEST(6U)
{
  Box2Box conv;
  UnstableTestTree t;

  int n = 1000;
  for (int i = 0; i < n; ++i) {
    t.insert (db::Box (1, 1, 1, 1));
  }
  t.sort (conv);

  for (int i = 0; i < n; ++i) {
    db::Box b (0, 0, 10, 10);
    test_tree_overlap (_this, t, b, conv);
    test_tree_touching (_this, t, b, conv);
  }

}

TEST(7)
{
  Box2Box conv;
  TestTree t;

  int n = 200000;

  for (int i = n - 1; i >= 0; --i) {
    t.insert (db::Box (i * 10, 0, i * 10 + 5, 5));
  }
  t.sort (conv);

  {
    tl::SelfTimer timer ("test 7 lookup");
    size_t n = 0;
    for (unsigned int i = 0; i < 2000; ++i) {
      db::Coord sx = 0, sy = 0;
      TestTree::touching_iterator it = t.begin_touching (db::Box (db::Point (2000, 0), db::Point (3000, 0)), conv);
      while (!it.at_end ()) {
        sx += abs (it->left ());
        sy += abs (it->bottom ());
        ++it;
        ++n;
      }
      EXPECT_EQ (sx, 252500);
      EXPECT_EQ (sy, 0);
    }
    EXPECT_EQ (n, size_t (101 * 2000));
  }

  {
    tl::SelfTimer timer ("test 7 traverse");
    db::Coord m = std::numeric_limits<db::Coord>::max ();
    size_t n = 0;
    for (unsigned int i = 0; i < 10; ++i) {
      TestTree::touching_iterator it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
      while (!it.at_end ()) {
        ++it;
        ++n;
      }
    }
    EXPECT_EQ (n, t.size () * 10);
  }
}

TEST(7U)
{
  Box2Box conv;
  UnstableTestTree t;

  int n = 200000;

  for (int i = n - 1; i >= 0; --i) {
    t.insert (db::Box (i * 10, 0, i * 10 + 5, 5));
  }
  t.sort (conv);

  {
    tl::SelfTimer timer ("test 7U lookup");
    size_t n = 0;
    for (unsigned int i = 0; i < 2000; ++i) {
      db::Coord sx = 0, sy = 0;
      UnstableTestTree::touching_iterator it = t.begin_touching (db::Box (db::Point (2000, 0), db::Point (3000, 0)), conv);
      while (!it.at_end ()) {
        sx += abs (it->left ());
        sy += abs (it->bottom ());
        ++it;
        ++n;
      }
      EXPECT_EQ (sx, 252500);
      EXPECT_EQ (sy, 0);
    }
    EXPECT_EQ (n, size_t (101 * 2000));
  }

  {
    tl::SelfTimer timer ("test 7U traverse");
    db::Coord m = std::numeric_limits<db::Coord>::max ();
    size_t n = 0;
    for (unsigned int i = 0; i < 10; ++i) {
      UnstableTestTree::touching_iterator it = t.begin_touching (db::Box (db::Point (-m,-m), db::Point (m, m)), conv);
      while (!it.at_end ()) {
        ++it;
        ++n;
      }
    }
    EXPECT_EQ (n, t.size () * 10);
  }
}

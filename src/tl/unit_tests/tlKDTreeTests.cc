
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


#include "tlKDTree.h"
#include "tlUnitTest.h"

#include <iostream>
#include <set>
#include <stdlib.h>

struct TestObj {
  TestObj (int v0, int v1) { v[0]=v0; v[1]=v1; }
  int v[2];
};

std::ostream &operator<< (std::ostream &os, const TestObj &o)
{
  return (os << "(" << o.v[0] << "," << o.v[1] << ")");
}

struct TestCoordPicker {
  int operator() (unsigned int i, const TestObj &o) const { return o.v[i % 2]; }
};

struct TestCoordGetter {
  TestCoordGetter (const TestObj &o) : obj (o) { }
  int operator() (unsigned int i) const { return obj.v[i % 2]; }
private:
  const TestObj &obj;
};

struct TestCmp 
{
  bool operator() (unsigned, int v1, int v2) const { return v1 < v2; }
};

typedef tl::kd_tree <TestObj, int, TestCoordPicker, TestCmp> TestTree;

struct TestSearch
{
  TestSearch (bool eq, int i1, int i2) : _i1 (i1), _i2 (i2), _eq (eq) { }
  size_t size() const { return 2; }
  bool operator() (size_t i, int v) const
  {
    if ((i % 2) == 0) { return cmp (v, _i1); }
    else              { return cmp (v, _i2); }
  }
  bool operator() (const TestObj &a) const
  {
    return (cmp (a.v[0], _i1) && cmp (a.v[1], _i2));
  }
  std::string to_string () const 
  {
    return (_eq ? ">=" : ">") + std::string ("(") + tl::to_string (_i1) + "," + tl::to_string (_i2) + ")";
  }
private:
  int _i1, _i2;
  bool _eq;

  bool cmp (int a, int b) const {
    if (_eq) { return a >= b; }
    else     { return a > b; }
  }
};

typedef tl::kd_tree_it <TestTree, TestSearch> TestTreeIt;


template <class Tree, class Picker, class Search>
static void test_tree (tl::TestBase *_this, const Tree &t, const Picker &p, const Search &s)
{
  if (tl::verbose ()) {
    std::cout << "Testing vs. " << s.to_string () << std::endl;
  }

  std::set <typename Tree::difference_type> good_idx;
  for (typename Tree::const_iterator e = t.begin (); e != t.end (); ++e) {
    if (s (*e)) {
      good_idx.insert (std::distance (t.begin (), e));
    }
  }
  
  if (tl::verbose ()) {
    for (typename Tree::size_type i = 0; i < t.size (); ++i) {
      std::cout << i << " b=" << t.bounds() [i] << ", v=" << p (0, t.objects() [i]) << "," << p (1, t.objects() [i]) << std::endl;
    }
  }

  tl::kd_tree_it<Tree, Search> i = t.sel_begin (p, s);
  while (i != t.sel_end ()) {
    if (good_idx.find (i) == good_idx.end ()) {
      FAIL_ARG ("not found in good indices list", *i);
    } else {
      good_idx.erase (i);
    }
    if (tl::verbose ()) {
      std::cout << p(0, *i) << "," << p(1, *i) << std::endl;
    }
    ++i;
  }

  EXPECT_EQ (good_idx.size (), size_t (0));
}

TEST(1)
{
  TestCmp cmp;
  TestSearch s1 (true, 1, 1);
  TestSearch s2 (false, 1, 1);

  TestCoordPicker p;
  TestTree t;
  t.sort (p, cmp);
  test_tree (_this, t, p, s1);
  test_tree (_this, t, p, s2);

  t.insert (TestObj (1, 2));
  t.sort (p, cmp);
  test_tree (_this, t, p, s1);
  test_tree (_this, t, p, s2);

  t.insert (TestObj (3, 1));
  t.sort (p, cmp);
  test_tree (_this, t, p, s1);
  test_tree (_this, t, p, s2);

  t.insert (TestObj (-1, 10));
  t.insert (TestObj (-1, 1));
  t.insert (TestObj (3, 1));
  t.insert (TestObj (3, 7));
  t.insert (TestObj (2, 9));
  t.sort (p, cmp);

  test_tree (_this, t, p, s1);
  test_tree (_this, t, p, s2);

  t.insert (TestObj (1, 1));
  t.sort (p, cmp);

  test_tree (_this, t, p, s1);
  test_tree (_this, t, p, s2);
}

inline int rvalue () 
{
  return (rand () % 10000) - 5000;
}

TEST(2)
{
  TestCmp cmp;
  TestCoordPicker p;
  TestTree t;

  int n = 231;

  for (int i = 0; i < n; ++i) {
    t.insert (TestObj (rvalue (), rvalue ()));
  }
  t.sort (p, cmp);

  for (int i = 0; i < n; ++i) {
    int v1 = rvalue ();
    int v2 = rvalue ();
    TestSearch s1 (true, v1, v2);
    TestSearch s2 (false, v1, v2);
    test_tree (_this, t, p, s1);
    test_tree (_this, t, p, s2);
  }

}


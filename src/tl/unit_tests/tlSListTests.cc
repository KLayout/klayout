
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "tlSList.h"
#include "tlUnitTest.h"
#include "tlString.h"

namespace
{

static size_t obj_count = 0;

struct MyClass1
{
  MyClass1 (int _n) : n (_n) { ++obj_count; }
  MyClass1 (const MyClass1 &other) : n (other.n) { ++obj_count; }
  MyClass1 &operator= (const MyClass1 &other) { n = other.n; return *this; }
  ~MyClass1 () { --obj_count; }
  int n;
  bool operator== (const MyClass1 &other) const { return n == other.n; }
  bool operator< (const MyClass1 &other) const { return n < other.n; }
};

}

template <class C>
static std::string l2s (const tl::slist<C> &l)
{
  std::string x;
  for (typename tl::slist<C>::const_iterator i = l.begin (); i != l.end (); ++i) {
    if (!x.empty ()) {
      x += ",";
    }
    x += tl::to_string (i->n);
  }
  return x;
}

template <class C>
static std::string l2s_nc (tl::slist<C> &l)
{
  std::string x;
  for (typename tl::slist<C>::iterator i = l.begin (); i != l.end (); ++i) {
    if (!x.empty ()) {
      x += ",";
    }
    x += tl::to_string (i->n);
  }
  return x;
}

TEST(1_Basic)
{
  obj_count = 0;

  {
    MyClass1 c1 (-1);
    tl::slist<MyClass1> l1, l2;

    EXPECT_EQ (l1.empty (), true);
    EXPECT_EQ (l1.size (), size_t (0));
    EXPECT_EQ (l2s (l1), "");

    l1.push_back (MyClass1 (17));
    EXPECT_EQ (l1.empty (), false);
    EXPECT_EQ (l1.size (), size_t (1));
    EXPECT_EQ (l2s (l1), "17");

    l1.push_back (MyClass1 (42));
    l2 = l1;
    tl::slist<MyClass1> l3 (l2);
    EXPECT_EQ (l1.empty (), false);
    EXPECT_EQ (l1.size (), size_t (2));
    EXPECT_EQ (l2s (l1), "17,42");

    l1.pop_front ();
    EXPECT_EQ (l1.empty (), false);
    EXPECT_EQ (l1.size (), size_t (1));
    EXPECT_EQ (l2s (l1), "42");

    l1.clear ();
    EXPECT_EQ (l1.empty (), true);
    EXPECT_EQ (l1.size (), size_t (0));
    EXPECT_EQ (l2s (l1), "");

    EXPECT_EQ (l2s (l2), "17,42");
    l2.pop_front ();
    EXPECT_EQ (l2s (l2), "42");

    l3.push_back (MyClass1 (2));
    l3.push_front (MyClass1 (1));
    EXPECT_EQ (l2s (l3), "1,17,42,2");
    EXPECT_EQ (l2s_nc (l3), "1,17,42,2");
    EXPECT_EQ (l3.size (), size_t (4));

    l3.pop_front ();
    EXPECT_EQ (l2s (l3), "17,42,2");
    EXPECT_EQ (l3.size (), size_t (3));

    c1 = l3.front ();
    EXPECT_EQ (c1.n, 17);

    c1 = l3.back ();
    EXPECT_EQ (c1.n, 2);

    l3.pop_front ();
    EXPECT_EQ (l2s (l3), "42,2");
    EXPECT_EQ (l3.size (), size_t (2));

    l3.push_back (MyClass1 (1));
    EXPECT_EQ (l2s (l3), "42,2,1");
    EXPECT_EQ (l3.size (), size_t (3));

    l3.swap (l2);
    EXPECT_EQ (l2s (l2), "42,2,1");
    EXPECT_EQ (l2s (l3), "42");

    l1.clear ();
    l2.swap (l1);
    EXPECT_EQ (l2s (l1), "42,2,1");
    EXPECT_EQ (l2s (l2), "");

    l1.clear ();
    l3.clear ();

    l2.swap (l1);
    EXPECT_EQ (l2s (l1), "");
    EXPECT_EQ (l2s (l2), "");
  }

  EXPECT_EQ (obj_count, size_t (0));
}

TEST(2_SpliceAndMove)
{
  obj_count = 0;

  {
    tl::slist<MyClass1> l1, l2;

    l1.splice (l2);
    EXPECT_EQ (l2s (l1), "");

    l1.push_back (MyClass1 (17));
    l1.push_back (MyClass1 (42));

    l1.splice (l2);
    EXPECT_EQ (l2s (l1), "17,42");
    EXPECT_EQ (l2s (l2), "");
    l2.splice (l1);
    EXPECT_EQ (l2s (l2), "17,42");
    EXPECT_EQ (l2s (l1), "");

    l1.swap (l2);

    l2.push_back (MyClass1 (2));
    l2.push_back (MyClass1 (1));

    l1.splice (l2);
    EXPECT_EQ (l2s (l1), "17,42,2,1");
    EXPECT_EQ (l2s (l2), "");

    l2 = std::move (l1);
    EXPECT_EQ (l2s (l2), "17,42,2,1");
    EXPECT_EQ (l2s (l1), "");

    l1 = tl::slist<MyClass1> (std::move (l2));
    EXPECT_EQ (l2s (l1), "17,42,2,1");
    EXPECT_EQ (l2s (l2), "");
  }

  EXPECT_EQ (obj_count, size_t (0));
}

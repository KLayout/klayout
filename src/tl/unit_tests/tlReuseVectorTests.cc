
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


#include "tlUnitTest.h"
#include "tlReuseVector.h"

#include <string>
#include <vector>
#include <cstdio>

std::string to_string (const tl::reuse_vector<std::string> &x)
{
  std::string r;
  size_t n = 0;
  for (tl::reuse_vector<std::string>::const_iterator s = x.begin (); s != x.end (); ++s) {
    if (! r.empty ()) {
      r += ",";
    }
    r += *s;
    ++n;
  }
  tl_assert (n == x.size ());
  return r;
}

tl::reuse_vector<std::string>::iterator 
test_insert (tl::TestBase *_this, tl::reuse_vector<std::string> &rv, const std::string &s)
{
  tl::reuse_vector<std::string>::iterator si = rv.insert (s);
  EXPECT_EQ (*si, s);
  return si;
}

TEST(1) 
{
  tl::reuse_vector<std::string> rv;
  tl::reuse_vector<std::string> rv_copy;
  tl::reuse_vector<std::string>::iterator a, b, c;
  tl::reuse_vector<std::string>::const_iterator ca, cb, cc;

  EXPECT_EQ (to_string (rv), "");
  a = test_insert (_this, rv, "a");
  EXPECT_EQ (to_string (rv), "a");
  b = test_insert (_this, rv, "b");
  EXPECT_EQ (to_string (rv), "a,b");
  c = test_insert (_this, rv, "c");
  EXPECT_EQ (to_string (rv), "a,b,c");
  cc = c;
  ca = a;
  cb = b;
  EXPECT_EQ (*cc, "c");
  EXPECT_EQ (*ca, "a");
  EXPECT_EQ (*cb, "b");
  rv_copy = rv;

  rv.erase (c);
  EXPECT_EQ (to_string (rv), "a,b");
  EXPECT_EQ (rv == rv_copy, false);
  EXPECT_EQ (rv != rv_copy, true);
  EXPECT_EQ (rv < rv_copy, true);
  EXPECT_EQ (rv_copy < rv, false);

  c = test_insert (_this, rv, "c");
  EXPECT_EQ (to_string (rv), "a,b,c");
  cc = c;

  EXPECT_EQ (*cc, "c");
  EXPECT_EQ (*ca, "a");
  EXPECT_EQ (*cb, "b");

  EXPECT_EQ (to_string (rv_copy), "a,b,c");
  EXPECT_EQ (rv == rv_copy, true);
  EXPECT_EQ (rv != rv_copy, false);
  EXPECT_EQ (rv < rv_copy, false);
}

TEST(2) 
{
  tl::reuse_vector<std::string> rv;
  tl::reuse_vector<std::string> rv_copy;
  tl::reuse_vector<std::string>::iterator a, b, c;
  tl::reuse_vector<std::string>::const_iterator ca, cb, cc;

  EXPECT_EQ (to_string (rv), "");
  a = test_insert (_this, rv, "a");
  EXPECT_EQ (to_string (rv), "a");
  rv.erase (a);
  EXPECT_EQ (to_string (rv), "");
  a = test_insert (_this, rv, "a");
  b = test_insert (_this, rv, "b");
  EXPECT_EQ (to_string (rv), "a,b");
  rv.erase (a);
  EXPECT_EQ (to_string (rv), "b");
  a = test_insert (_this, rv, "a");
  c = test_insert (_this, rv, "c");
  EXPECT_EQ (to_string (rv), "a,b,c");
  rv.erase (a);
  rv.erase (c);
  EXPECT_EQ (to_string (rv), "b");
  c = test_insert (_this, rv, "c");
  a = test_insert (_this, rv, "a");
  EXPECT_EQ (to_string (rv), "c,b,a");
  rv.erase (a);
  rv.erase (c);
  a = test_insert (_this, rv, "a");
  c = test_insert (_this, rv, "c");
  EXPECT_EQ (to_string (rv), "a,b,c");
  cc = c;
  ca = a;
  cb = b;
  EXPECT_EQ (*cc, "c");
  EXPECT_EQ (*ca, "a");
  EXPECT_EQ (*cb, "b");
  rv_copy = rv;

  rv.erase (c);
  EXPECT_EQ (to_string (rv), "a,b");
  EXPECT_EQ (rv == rv_copy, false);
  EXPECT_EQ (rv != rv_copy, true);
  EXPECT_EQ (rv < rv_copy, true);
  EXPECT_EQ (rv_copy < rv, false);

  c = test_insert (_this, rv, "c");
  EXPECT_EQ (to_string (rv), "a,b,c");
  cc = c;

  EXPECT_EQ (*cc, "c");
  EXPECT_EQ (*ca, "a");
  EXPECT_EQ (*cb, "b");

  EXPECT_EQ (to_string (rv_copy), "a,b,c");
  EXPECT_EQ (rv == rv_copy, true);
  EXPECT_EQ (rv != rv_copy, false);
  EXPECT_EQ (rv < rv_copy, false);
}

//  Test: reallocation of special element if no more are available
TEST(3) 
{
  tl::reuse_vector<std::string> rv;
  tl::reuse_vector<std::string>::iterator a, b, c, d;

  a = test_insert (_this, rv, "a");
  b = test_insert (_this, rv, "b");
  c = test_insert (_this, rv, "c");
  d = test_insert (_this, rv, "d");
  EXPECT_EQ (rv.size (), rv.capacity ());
  EXPECT_EQ (to_string (rv), "a,b,c,d");
  EXPECT_EQ (a.is_valid (), true);

  rv.erase (a);
  EXPECT_EQ (a.is_valid (), false);
  EXPECT_EQ (rv.size (), size_t (3));
  EXPECT_EQ (rv.capacity (), size_t (4));
  EXPECT_EQ (to_string (rv), "b,c,d");

  a = rv.insert ("a");
  EXPECT_EQ (a.is_valid (), true);
  EXPECT_EQ (rv.size (), size_t (4));
  EXPECT_EQ (to_string (rv), "a,b,c,d")

  EXPECT_EQ (*d, "d");

  rv.erase (c);
  rv.erase (b);
  rv.erase (d);
  rv.erase (a);
  EXPECT_EQ (a.is_valid (), false);
  EXPECT_EQ (to_string (rv), "")
}

namespace {

class A {
public:
  static int cc;
  static int ccc;
  static int dc;
  static void reset () { cc = ccc = dc = 0; }
  A() : x(0) { ++cc; }
  A(int n) : x(n) { ++cc; }
  A(const A &d) : x(d.x) { ++ccc; }
  ~A() { ++dc; }
  int x;
};

int A::cc = 0;
int A::ccc = 0;
int A::dc = 0;

}

namespace {

class B {
public:
  static int cc;
  static int ccc;
  static int dc;
  static void reset () { cc = ccc = dc = 0; }
  B() : x(0) { ++cc; }
  B(int n) : x(n) { ++cc; }
  B(const B &d) : x(d.x) { ++ccc; }
  ~B() { ++dc; }
  int x;
};

int B::cc = 0;
int B::ccc = 0;
int B::dc = 0;

}

//  Test: relocate strategy
TEST(4) 
{
  tl::reuse_vector<A, true> va;
  tl::reuse_vector<B, false> vb;
  A::reset ();
  B::reset ();

  for (int i = 0; i < 10; ++i) {
    va.insert (A (100 - i * 10));
    vb.insert (B (100 - i * 10));
  }

  int i = 0;
  for (tl::reuse_vector<A, true>::const_iterator n = va.begin (); n != va.end (); ++n, ++i) {
    EXPECT_EQ (n->x, 100 - i * 10);
  }
  i = 0;
  for (tl::reuse_vector<B, false>::const_iterator n = vb.begin (); n != vb.end (); ++n, ++i) {
    EXPECT_EQ (n->x, 100 - i * 10);
  }

  for (int i = 0; i < 9; ++i) {
    va.erase (va.begin ());
    vb.erase (vb.begin ());
  }

  EXPECT_EQ (va.begin ()->x, 10);
  EXPECT_EQ (vb.begin ()->x, 10);

  va.clear ();
  vb.clear ();

  EXPECT_EQ (A::dc, A::cc + A::ccc);
  EXPECT_EQ (A::cc, 10);
  EXPECT_EQ (A::ccc, 10);
  EXPECT_EQ (B::dc, B::cc + B::ccc);
  EXPECT_EQ (B::cc, 10);
  EXPECT_EQ (B::ccc, 22);
}

//  destroy while iterate
TEST(5)
{
  tl::reuse_vector<A> v;
  v.insert (A (1));
  v.insert (A (2));
  v.insert (A (3));

  tl::reuse_vector<A>::iterator i;
  tl::reuse_vector<A>::iterator ii;

  ii = v.end ();
  for (i = v.begin (); i != ii; ++i) {
    if (i->x == 2) {
      v.erase (i);
    }
  }

  EXPECT_EQ (v.size (), size_t (2));
  i = v.begin ();
  EXPECT_EQ (i->x, 1);
  ++i;
  EXPECT_EQ (i->x, 3);

  v = tl::reuse_vector<A> ();
  v.insert (A (1));
  v.insert (A (2));
  v.insert (A (3));

  for (i = v.begin (); !i.at_end (); ++i) {
    if (i->x == 2) {
      v.erase (i);
    }
  }

  EXPECT_EQ (v.size (), size_t (2));
  i = v.begin ();
  EXPECT_EQ (i->x, 1);
  ++i;
  EXPECT_EQ (i->x, 3);

  v = tl::reuse_vector<A> ();
  v.insert (A (1));
  v.insert (A (2));
  v.insert (A (3));

  ii = v.end ();
  for (i = v.begin (); i != ii; ++i) {
    if (i->x == 3) {
      v.erase (i);
    }
  }

  EXPECT_EQ (v.size (), size_t (2));
  i = v.begin ();
  EXPECT_EQ (i->x, 1);
  ++i;
  EXPECT_EQ (i->x, 2);

  v = tl::reuse_vector<A> ();
  v.insert (A (1));
  v.insert (A (2));
  v.insert (A (3));

  for (i = v.begin (); !i.at_end (); ++i) {
    if (i->x == 3) {
      v.erase (i);
    }
  }

  EXPECT_EQ (v.size (), size_t (2));
  i = v.begin ();
  EXPECT_EQ (i->x, 1);
  ++i;
  EXPECT_EQ (i->x, 2);

  v = tl::reuse_vector<A> ();
  v.insert (A (1));
  v.insert (A (2));
  v.insert (A (3));

  ii = v.end ();
  for (i = v.begin (); i != ii; ++i) {
    v.erase (i);
  }
  EXPECT_EQ (v.size (), size_t (0));
  EXPECT_EQ (v.empty (), true);

  v = tl::reuse_vector<A> ();
  v.insert (A (1));
  v.insert (A (2));
  v.insert (A (3));

  for (i = v.begin (); !i.at_end (); ++i) {
    v.erase (i);
  }
  EXPECT_EQ (v.size (), size_t (0));
  EXPECT_EQ (v.empty (), true);
}


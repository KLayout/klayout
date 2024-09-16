
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

#include "tlList.h"
#include "tlUnitTest.h"
#include "tlString.h"

namespace
{

static size_t obj_count = 0;

struct MyClass1 : public tl::list_node<MyClass1>
{
  MyClass1 (int _n) : n (_n) { ++obj_count; }
  MyClass1 (const MyClass1 &other) : tl::list_node<MyClass1> (), n (other.n) { ++obj_count; }
  ~MyClass1 () { --obj_count; }
  int n;
  bool operator== (const MyClass1 &other) const { return n == other.n; }
  bool operator< (const MyClass1 &other) const { return n < other.n; }
};

struct MyClass2 : public tl::list_node<MyClass2>
{
  MyClass2 (int _n) : n (_n) { ++obj_count; }
  ~MyClass2 () { --obj_count; }
  int n;
public:
  MyClass2 (const MyClass2 &other);
  MyClass2 &operator= (const MyClass2 &other);
  bool operator== (const MyClass2 &other) const { return n == other.n; }
  bool operator< (const MyClass2 &other) const { return n < other.n; }
};

}

template <class C>
static std::string l2s (const tl::list<C> &l)
{
  std::string x;
  for (typename tl::list<C>::const_iterator i = l.begin (); i != l.end (); ++i) {
    if (!x.empty ()) {
      x += ",";
    }
    x += tl::to_string (i->n);
  }
  return x;
}

template <class C>
static std::string l2sr (const tl::list<C> &l)
{
  std::string x;
  for (typename tl::list<C>::const_reverse_iterator i = l.rbegin (); i != l.rend (); ++i) {
    if (!x.empty ()) {
      x += ",";
    }
    x += tl::to_string (i->n);
  }
  return x;
}

template <class C>
static std::string l2sm (const tl::list<C> &l)
{
  std::string x;
  for (typename tl::list<C>::const_iterator i = l.end (); i != l.begin (); ) {
    --i;
    if (!x.empty ()) {
      x += ",";
    }
    x += tl::to_string (i->n);
  }
  return x;
}

template <class C>
static std::string l2srm (const tl::list<C> &l)
{
  std::string x;
  for (typename tl::list<C>::const_reverse_iterator i = l.rend (); i != l.rbegin (); ) {
    --i;
    if (!x.empty ()) {
      x += ",";
    }
    x += tl::to_string (i->n);
  }
  return x;
}

template <class C>
static std::string l2s_nc (tl::list<C> &l)
{
  std::string x;
  for (typename tl::list<C>::iterator i = l.begin (); i != l.end (); ++i) {
    if (!x.empty ()) {
      x += ",";
    }
    x += tl::to_string (i->n);
  }
  return x;
}

template <class C>
static std::string l2sr_nc (tl::list<C> &l)
{
  std::string x;
  for (typename tl::list<C>::reverse_iterator i = l.rbegin (); i != l.rend (); ++i) {
    if (!x.empty ()) {
      x += ",";
    }
    x += tl::to_string (i->n);
  }
  return x;
}

template <class C>
static std::string l2sm_nc (tl::list<C> &l)
{
  std::string x;
  for (typename tl::list<C>::iterator i = l.end (); i != l.begin (); ) {
    --i;
    if (!x.empty ()) {
      x += ",";
    }
    x += tl::to_string (i->n);
  }
  return x;
}

template <class C>
static std::string l2srm_nc (tl::list<C> &l)
{
  std::string x;
  for (typename tl::list<C>::reverse_iterator i = l.rend (); i != l.rbegin (); ) {
    --i;
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

  tl::list<MyClass1> l1, l2;

  EXPECT_EQ (l1.empty (), true);
  EXPECT_EQ (l1.size (), size_t (0));
  EXPECT_EQ (l2s (l1), "");
  EXPECT_EQ (l2sr (l1), "");

  l1.push_back (new MyClass1 (17));
  EXPECT_EQ (l1.empty (), false);
  EXPECT_EQ (l1.size (), size_t (1));
  EXPECT_EQ (l2s (l1), "17");
  EXPECT_EQ (l2sr (l1), "17");

  l1.push_back (MyClass1 (42));
  l2 = l1;
  tl::list<MyClass1> l3 (l2);
  EXPECT_EQ (l1.empty (), false);
  EXPECT_EQ (l1.size (), size_t (2));
  EXPECT_EQ (l2s (l1), "17,42");
  EXPECT_EQ (l2sr (l1), "42,17");

  delete l1.first ();
  EXPECT_EQ (l1.empty (), false);
  EXPECT_EQ (l1.size (), size_t (1));
  EXPECT_EQ (l2s (l1), "42");
  EXPECT_EQ (l2sr (l1), "42");

  l1.clear ();
  EXPECT_EQ (l1.empty (), true);
  EXPECT_EQ (l1.size (), size_t (0));
  EXPECT_EQ (l2s (l1), "");
  EXPECT_EQ (l2sr (l1), "");

  EXPECT_EQ (l2s (l2), "17,42");
  EXPECT_EQ (l2sr (l2), "42,17");
  l2.pop_back ();
  EXPECT_EQ (l2s (l2), "17");
  EXPECT_EQ (l2sr (l2), "17");

  l3.push_back (new MyClass1 (2));
  l3.push_front (new MyClass1 (1));
  EXPECT_EQ (l2s (l3), "1,17,42,2");
  EXPECT_EQ (l2srm (l3), "1,17,42,2");
  EXPECT_EQ (l2sm (l3), "2,42,17,1");
  EXPECT_EQ (l2sr (l3), "2,42,17,1");
  EXPECT_EQ (l2s_nc (l3), "1,17,42,2");
  EXPECT_EQ (l2srm_nc (l3), "1,17,42,2");
  EXPECT_EQ (l2sm_nc (l3), "2,42,17,1");
  EXPECT_EQ (l2sr_nc (l3), "2,42,17,1");
  EXPECT_EQ (l3.size (), size_t (4));

  l3.pop_back ();
  EXPECT_EQ (l2s (l3), "1,17,42");
  EXPECT_EQ (l2sr (l3), "42,17,1");
  EXPECT_EQ (l3.size (), size_t (3));

  MyClass1 *c1;
  c1 = l3.first ();
  EXPECT_EQ (c1->n, 1);
  c1 = c1->next ();
  EXPECT_EQ (c1->n, 17);
  c1 = c1->next ();
  EXPECT_EQ (c1->n, 42);
  EXPECT_EQ (c1->next (), 0);

  c1 = l3.last ();
  EXPECT_EQ (c1->n, 42);
  c1 = c1->prev ();
  EXPECT_EQ (c1->n, 17);
  c1 = c1->prev ();
  EXPECT_EQ (c1->n, 1);
  EXPECT_EQ (c1->prev (), 0);

  l3.pop_front ();
  EXPECT_EQ (l2s (l3), "17,42");
  EXPECT_EQ (l2sr (l3), "42,17");
  EXPECT_EQ (l3.size (), size_t (2));

  l3.push_back (new MyClass1 (1));
  EXPECT_EQ (l2s (l3), "17,42,1");
  EXPECT_EQ (l2sr (l3), "1,42,17");
  EXPECT_EQ (l3.size (), size_t (3));

  c1 = l3.first ()->next ();
  delete c1;
  EXPECT_EQ (l2s (l3), "17,1");
  EXPECT_EQ (l2sr (l3), "1,17");
  EXPECT_EQ (l3.size (), size_t (2));

  EXPECT_EQ (l2sr (l2), "17");
  EXPECT_EQ (l2sr (l3), "1,17");
  l3.swap (l2);
  EXPECT_EQ (l2sr (l2), "1,17");
  EXPECT_EQ (l2sr (l3), "17");

  l1.clear ();
  l2.swap (l1);
  EXPECT_EQ (l2sr (l1), "1,17");
  EXPECT_EQ (l2sr (l2), "");

  l1.clear ();
  l3.clear ();

  l2.swap (l1);
  EXPECT_EQ (l2sr (l1), "");
  EXPECT_EQ (l2sr (l2), "");

  EXPECT_EQ (obj_count, size_t (0));
}

TEST(2_BasicNoCopy)
{
  {
    obj_count = 0;

    MyClass2 mc2 (42);  //  will not be owned
    tl::list<MyClass2> l1, l2, l3;

    EXPECT_EQ (l1.empty (), true);
    EXPECT_EQ (l1.size (), size_t (0));
    EXPECT_EQ (l2s (l1), "");
    EXPECT_EQ (l2sr (l1), "");

    l1.push_back (new MyClass2 (17));
    EXPECT_EQ (l1.empty (), false);
    EXPECT_EQ (l1.size (), size_t (1));
    EXPECT_EQ (l2s (l1), "17");
    EXPECT_EQ (l2sr (l1), "17");

    l1.push_back (mc2);
    EXPECT_EQ (l1.empty (), false);
    EXPECT_EQ (l1.size (), size_t (2));
    EXPECT_EQ (l2s (l1), "17,42");
    EXPECT_EQ (l2sr (l1), "42,17");

    delete l1.first ();
    EXPECT_EQ (l1.empty (), false);
    EXPECT_EQ (l1.size (), size_t (1));
    EXPECT_EQ (l2s (l1), "42");
    EXPECT_EQ (l2sr (l1), "42");

    l1.clear ();
    EXPECT_EQ (l1.empty (), true);
    EXPECT_EQ (l1.size (), size_t (0));
    EXPECT_EQ (l2s (l1), "");
    EXPECT_EQ (l2sr (l1), "");

    l2.push_back (new MyClass2 (17));
    l2.push_back (new MyClass2 (42));

    EXPECT_EQ (l2s (l2), "17,42");
    EXPECT_EQ (l2sr (l2), "42,17");
    l2.pop_back ();
    EXPECT_EQ (l2s (l2), "17");
    EXPECT_EQ (l2sr (l2), "17");

    EXPECT_EQ (l2 == l3, false);
    EXPECT_EQ (l2 != l3, true);
    EXPECT_EQ (l2 < l3, false);

    l3.push_back (new MyClass2 (17));
    EXPECT_EQ (l2 == l3, true);
    EXPECT_EQ (l2 != l3, false);
    EXPECT_EQ (l2 < l3, false);

    l3.push_back (new MyClass2 (42));
    EXPECT_EQ (l2 == l3, false);
    EXPECT_EQ (l2 != l3, true);
    EXPECT_EQ (l2 < l3, true);

    l3.push_back (new MyClass2 (2));
    l3.push_front (new MyClass2 (1));
    EXPECT_EQ (l2 == l3, false);
    EXPECT_EQ (l2 != l3, true);
    EXPECT_EQ (l2 < l3, false);

    EXPECT_EQ (l2s (l3), "1,17,42,2");
    EXPECT_EQ (l2srm (l3), "1,17,42,2");
    EXPECT_EQ (l2sm (l3), "2,42,17,1");
    EXPECT_EQ (l2sr (l3), "2,42,17,1");
    EXPECT_EQ (l2s_nc (l3), "1,17,42,2");
    EXPECT_EQ (l2srm_nc (l3), "1,17,42,2");
    EXPECT_EQ (l2sm_nc (l3), "2,42,17,1");
    EXPECT_EQ (l2sr_nc (l3), "2,42,17,1");
    EXPECT_EQ (l3.size (), size_t (4));

    l3.pop_back ();
    EXPECT_EQ (l2s (l3), "1,17,42");
    EXPECT_EQ (l2sr (l3), "42,17,1");
    EXPECT_EQ (l3.size (), size_t (3));

    MyClass2 *c1;
    c1 = l3.first ();
    EXPECT_EQ (c1->n, 1);
    c1 = c1->next ();
    EXPECT_EQ (c1->n, 17);
    c1 = c1->next ();
    EXPECT_EQ (c1->n, 42);
    EXPECT_EQ (c1->next (), 0);

    c1 = l3.last ();
    EXPECT_EQ (c1->n, 42);
    c1 = c1->prev ();
    EXPECT_EQ (c1->n, 17);
    c1 = c1->prev ();
    EXPECT_EQ (c1->n, 1);
    EXPECT_EQ (c1->prev (), 0);

    l3.pop_front ();
    EXPECT_EQ (l2s (l3), "17,42");
    EXPECT_EQ (l2sr (l3), "42,17");
    EXPECT_EQ (l3.size (), size_t (2));

    l3.push_back (new MyClass2 (1));
    EXPECT_EQ (l2s (l3), "17,42,1");
    EXPECT_EQ (l2sr (l3), "1,42,17");
    EXPECT_EQ (l3.size (), size_t (3));

    c1 = l3.first ()->next ();
    delete c1;
    EXPECT_EQ (l2s (l3), "17,1");
    EXPECT_EQ (l2sr (l3), "1,17");
    EXPECT_EQ (l3.size (), size_t (2));

    EXPECT_EQ (l2sr (l2), "17");
    EXPECT_EQ (l2sr (l3), "1,17");
    l3.swap (l2);
    EXPECT_EQ (l2sr (l2), "1,17");
    EXPECT_EQ (l2sr (l3), "17");

    l1.clear ();
    l2.clear ();
    l3.clear ();
    EXPECT_EQ (obj_count, size_t (1)); // one for mc2
  }

  EXPECT_EQ (obj_count, size_t (0)); // mc2 gone as well
}

TEST(3_Insert)
{
  obj_count = 0;

  tl::list<MyClass1> l1;
  tl::list<MyClass1>::iterator i1;

  EXPECT_EQ (l1.empty (), true);
  EXPECT_EQ (l1.size (), size_t (0));
  EXPECT_EQ (l2s (l1), "");

  l1.push_back (MyClass1 (42));
  EXPECT_EQ (l2s (l1), "42");
  EXPECT_EQ (l1.size (), size_t (1));

  i1 = l1.insert_before (l1.end (), MyClass1 (17));
  EXPECT_EQ (l2s (l1), "42,17");
  EXPECT_EQ (i1->n, 17);
  EXPECT_EQ (l1.size (), size_t (2));

  i1 = l1.insert_before (i1, MyClass1 (11));
  EXPECT_EQ (l2s (l1), "42,11,17");
  EXPECT_EQ (i1->n, 11);
  EXPECT_EQ (l1.size (), size_t (3));

  i1 = l1.insert (i1, MyClass1 (12));
  EXPECT_EQ (l2s (l1), "42,11,12,17");
  EXPECT_EQ (i1->n, 12);
  EXPECT_EQ (l1.size (), size_t (4));

  MyClass1 arr[3] = { MyClass1 (1), MyClass1 (2), MyClass1 (3) };

  i1 = l1.insert (i1, arr + 0, arr + 0);
  EXPECT_EQ (l2s (l1), "42,11,12,17");
  EXPECT_EQ (i1->n, 12);
  EXPECT_EQ (l1.size (), size_t (4));

  i1 = l1.insert (i1, arr + 0, arr + 3);
  EXPECT_EQ (l2s (l1), "42,11,12,1,2,3,17");
  EXPECT_EQ (i1->n, 1);
  EXPECT_EQ (l1.size (), size_t (7));

  l1.clear ();
  l1.push_back (MyClass1 (42));
  i1 = l1.insert_before (l1.end (), MyClass1 (17));
  EXPECT_EQ (l2s (l1), "42,17");
  EXPECT_EQ (i1->n, 17);
  EXPECT_EQ (l1.size (), size_t (2));

  i1 = l1.insert_before (i1, arr + 0, arr + 0);
  EXPECT_EQ (l2s (l1), "42,17");
  EXPECT_EQ (i1->n, 17);
  EXPECT_EQ (l1.size (), size_t (2));

  i1 = l1.insert_before (i1, arr + 0, arr + 3);
  EXPECT_EQ (l2s (l1), "42,1,2,3,17");
  EXPECT_EQ (i1->n, 1);
  EXPECT_EQ (l1.size (), size_t (5));

  //  test erase range
  l1.erase (i1, l1.end ());
  EXPECT_EQ (l2s (l1), "42");
  EXPECT_EQ (l1.size (), size_t (1));
}



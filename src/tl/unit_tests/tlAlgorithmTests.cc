
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


#include "tlAlgorithm.h"
#include "tlString.h"
#include "tlTimer.h"
#include "tlUnitTest.h"

#include <cstring>
#include <string>
#include <algorithm>
#include <vector>

std::string to_string (const std::vector<std::string> &v) 
{
  std::string t;
  for (std::vector<std::string>::const_iterator s = v.begin (); s != v.end (); ++s) {
    t += *s;
    t += " ";
  }
  return t;
}

class SimpleString
{
public:
  ~SimpleString () 
  {
    delete [] m_cp;
  }

  SimpleString () 
  {
    m_cp = new char [2];
    strcpy (m_cp, "");
  }

  SimpleString &operator= (const SimpleString &d)
  {
    if (this != &d) {
      delete [] m_cp;
      m_cp = new char [strlen (d.m_cp) + 1];
      strcpy (m_cp, d.m_cp);
    }
    return *this;
  }
    
  SimpleString (const SimpleString &d)
  {
    m_cp = new char [strlen (d.m_cp) + 1];
    strcpy (m_cp, d.m_cp);
  }
    
  SimpleString (const std::string &cp)
  {
    m_cp = new char [strlen (cp.c_str ()) + 1];
    strcpy (m_cp, cp.c_str ());
  }
    
  SimpleString (const char *cp)
  {
    m_cp = new char [strlen (cp) + 1];
    strcpy (m_cp, cp);
  }

  bool operator< (const SimpleString &d) const
  {
    return strcmp (m_cp, d.m_cp) < 0;
  }
    
  bool operator== (const SimpleString &d) const
  {
    return strcmp (m_cp, d.m_cp) == 0;
  }
  
  const char *c_str () const 
  {
    return m_cp;
  }

private:
  char *m_cp;
};

std::ostream &operator<< (std::ostream &os, const SimpleString &s)
{
  os << s.c_str ();
  return os;
}

struct test_compare {
  bool operator() (const SimpleString &a, const SimpleString &b) const
  {
    return b < a;
  }
  bool operator() (const std::string &a, const std::string &b) const
  {
    return b < a;
  }
  bool operator() (int a, int b) const
  {
    return b < a;
  }
};

TEST(1) 
{
  std::vector<std::string> v;
  v.push_back ("d");
  v.push_back ("a");
  v.push_back ("bx");
  v.push_back ("ba");

  tl::sort (v.begin (), v.end ());
  EXPECT_EQ (to_string (v), "a ba bx d ");
  
  tl::sort (v.begin (), v.end (), test_compare ());
  EXPECT_EQ (to_string (v), "d bx ba a ");
}

TEST(2) 
{
  std::vector<SimpleString> v;

  int n = 0x100000;

  v.reserve (n);

  for (int i = 0; i < n; ++i) {
    v.push_back (tl::sprintf ("%06x", i ^ 0x43abc)); // "unsorted"
  }

  {
    tl::SelfTimer timer ("sorting to reverse");
    tl::sort (v.begin (), v.end (), test_compare ());
  }
  {
    tl::SelfTimer timer ("sorting");
    tl::sort (v.begin (), v.end ());
  }

  for (int i = 0; i < n; ++i) {
    EXPECT_EQ (v[i], tl::sprintf ("%06x", i));
  }

  {
    tl::SelfTimer timer ("sorting again");
    tl::sort (v.begin (), v.end ());
  }

  v.clear ();
  v.reserve (n);

  for (int i = 0; i < n; ++i) {
    v.push_back (tl::sprintf ("%06x", i ^ 0x43abc)); // "unsorted"
  }

  {
    tl::SelfTimer timer ("std::sorting to reverse");
    std::sort (v.begin (), v.end (), test_compare ());
  }
  {
    tl::SelfTimer timer ("std::sorting");
    std::sort (v.begin (), v.end ());
  }

  for (int i = 0; i < n; ++i) {
    EXPECT_EQ (v[i], tl::sprintf ("%06x", i));
  }

  {
    tl::SelfTimer timer ("std::sorting again");
    std::sort (v.begin (), v.end ());
  }

}

TEST(3) 
{
  std::vector<int> v;

  int n = 10000;

  v.reserve (n);

  for (int i = 0; i < n; ++i) {
    v.push_back (i);
  }

  {
    tl::SelfTimer timer ("sorting");
    tl::sort (v.begin (), v.end (), test_compare ());
    tl::sort (v.begin (), v.end ());
  }

  for (int i = 0; i < n; ++i) {
    EXPECT_EQ (v[i], i);
  }
}

//  deterministic shuffle so the tests are reproducible without <random>
static void deterministic_shuffle (std::vector<int> &v)
{
  unsigned int x = 2463534242u ^ (unsigned int) v.size ();
  for (size_t i = v.size (); i > 1; --i) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    size_t j = (size_t) (x % (unsigned int) i);
    std::swap (v [i - 1], v [j]);
  }
}

//  a permutation of the distinct values 0 .. n-1
static std::vector<int> permutation_of_distinct (int n)
{
  std::vector<int> v;
  v.reserve (n);
  for (int i = 0; i < n; ++i) {
    v.push_back (i);
  }
  deterministic_shuffle (v);
  return v;
}

//  heap API: tl::make_heap/sort_heap must yield the same order as std::sort on distinct values
TEST(4)
{
  for (int n = 0; n <= 2000; ++n) {

    std::vector<int> base = permutation_of_distinct (n);

    std::vector<int> tl_v (base);
    tl::make_heap (tl_v.begin (), tl_v.end ());
    tl::sort_heap (tl_v.begin (), tl_v.end ());

    std::vector<int> std_v (base);
    std::sort (std_v.begin (), std_v.end ());

    EXPECT_EQ ((int) tl_v.size (), (int) std_v.size ());
    for (size_t i = 0; i < std_v.size (); ++i) {
      EXPECT_EQ (tl_v [i], std_v [i]);
    }

    std::vector<int> tl_c (base);
    tl::make_heap (tl_c.begin (), tl_c.end (), test_compare ());
    tl::sort_heap (tl_c.begin (), tl_c.end (), test_compare ());

    std::vector<int> std_c (base);
    std::sort (std_c.begin (), std_c.end (), test_compare ());

    EXPECT_EQ ((int) tl_c.size (), (int) std_c.size ());
    for (size_t i = 0; i < std_c.size (); ++i) {
      EXPECT_EQ (tl_c [i], std_c [i]);
    }

    //  push_heap/pop_heap: popping a max-heap yields the elements in descending order
    std::vector<int> heap;
    for (size_t i = 0; i < base.size (); ++i) {
      heap.push_back (base [i]);
      tl::push_heap (heap.begin (), heap.end ());
    }
    std::vector<int> popped;
    while (! heap.empty ()) {
      tl::pop_heap (heap.begin (), heap.end ());
      popped.push_back (heap.back ());
      heap.pop_back ();
    }
    EXPECT_EQ ((int) popped.size (), (int) std_v.size ());
    for (size_t i = 0; i < popped.size (); ++i) {
      EXPECT_EQ (popped [i], std_v [std_v.size () - 1 - i]);
    }

  }
}

//  nth_element on distinct values: same nth element as std and a valid partition
TEST(5)
{
  for (int n = 1; n <= 2000; ++n) {

    std::vector<int> base = permutation_of_distinct (n);
    std::vector<int> sorted_base (base);
    std::sort (sorted_base.begin (), sorted_base.end ());
    size_t nth = (size_t) n / 2;

    std::vector<int> tl_v (base);
    tl::nth_element (tl_v.begin (), tl_v.begin () + nth, tl_v.end ());

    std::vector<int> std_v (base);
    std::nth_element (std_v.begin (), std_v.begin () + nth, std_v.end ());

    EXPECT_EQ (tl_v [nth], std_v [nth]);
    EXPECT_EQ (tl_v [nth], sorted_base [nth]);
    for (size_t i = 0; i < nth; ++i) {
      EXPECT (! (tl_v [nth] < tl_v [i]));
    }
    for (size_t i = nth; i < (size_t) n; ++i) {
      EXPECT (! (tl_v [i] < tl_v [nth]));
    }

    //  the result must still be a permutation of the original multiset
    std::vector<int> tl_sorted (tl_v);
    std::sort (tl_sorted.begin (), tl_sorted.end ());
    for (size_t i = 0; i < sorted_base.size (); ++i) {
      EXPECT_EQ (tl_sorted [i], sorted_base [i]);
    }

    //  same with the comparator overload (descending order)
    std::vector<int> tl_c (base);
    tl::nth_element (tl_c.begin (), tl_c.begin () + nth, tl_c.end (), test_compare ());
    EXPECT_EQ (tl_c [nth], sorted_base [sorted_base.size () - 1 - nth]);
    for (size_t i = 0; i < nth; ++i) {
      EXPECT (! test_compare () (tl_c [nth], tl_c [i]));
    }
    for (size_t i = nth; i < (size_t) n; ++i) {
      EXPECT (! test_compare () (tl_c [i], tl_c [nth]));
    }

  }
}

//  nth_element with many equal keys must still produce a valid partition
TEST(6)
{
  std::vector<int> base;
  for (int i = 0; i < 2000; ++i) {
    base.push_back (i % 7);
  }
  deterministic_shuffle (base);

  for (size_t nth = 0; nth < base.size (); nth += 137) {

    std::vector<int> v (base);
    tl::nth_element (v.begin (), v.begin () + nth, v.end ());

    for (size_t i = 0; i < nth; ++i) {
      EXPECT (! (v [nth] < v [i]));
    }
    for (size_t i = nth; i < v.size (); ++i) {
      EXPECT (! (v [i] < v [nth]));
    }

    std::vector<int> sorted (v);
    std::sort (sorted.begin (), sorted.end ());
    std::vector<int> sorted_base (base);
    std::sort (sorted_base.begin (), sorted_base.end ());
    for (size_t i = 0; i < sorted.size (); ++i) {
      EXPECT_EQ (sorted [i], sorted_base [i]);
    }

  }
}

namespace
{

struct MoveCounter
{
  MoveCounter (int _v = 0) : v (_v) { }
  MoveCounter (const MoveCounter &d) : v (d.v) { ++copies; }
  MoveCounter (MoveCounter &&d) : v (d.v) { ++moves; }
  MoveCounter &operator= (const MoveCounter &d) { v = d.v; ++copies; return *this; }
  MoveCounter &operator= (MoveCounter &&d) { v = d.v; ++moves; return *this; }
  bool operator< (const MoveCounter &d) const { return v < d.v; }

  int v;
  static int copies;
  static int moves;
};

int MoveCounter::copies = 0;
int MoveCounter::moves = 0;

}

TEST(sort_moves_elements)
{
  //  the insertion sort range does not copy at all, the partitioning only copies its pivots
  for (int n = 5; n <= 500; n *= 10) {

    std::vector<MoveCounter> v;
    v.reserve (size_t (n));
    for (int i = 0; i < n; ++i) {
      v.push_back (MoveCounter ((i * 7919) % n));
    }

    MoveCounter::copies = 0;
    MoveCounter::moves = 0;

    tl::sort (v.begin (), v.end ());

    if (n < 16) {
      EXPECT_EQ (MoveCounter::copies, 0);
    } else {
      EXPECT_EQ (MoveCounter::copies < n / 4, true);
    }
    EXPECT_EQ (MoveCounter::moves > n, true);
    for (int i = 1; i < n; ++i) {
      EXPECT_EQ (v [i - 1].v <= v [i].v, true);
    }

  }
}


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



#include "tlStableVector.h"
#include "tlAlgorithm.h"
#include "tlUnitTest.h"

std::string to_string (const tl::stable_vector<std::string> &v) 
{
  std::string t;
  for (tl::stable_vector<std::string>::const_iterator s = v.begin (); s != v.end (); ++s) {
    t += *s;
    t += " ";
  }
  return t;
}

struct test_compare {
  bool operator() (const std::string &a, const std::string &b) const
  {
    return a > b;
  }
  bool operator() (int a, int b) const
  {
    return a > b;
  }
};

TEST(1) 
{
  tl::stable_vector<std::string> v;
  EXPECT_EQ (v.size (), size_t (0));
  v.push_back ("d");
  EXPECT_EQ (v.back (), "d");
  v.push_back (new std::string ("a"));
  EXPECT_EQ (v.back (), "a");
  EXPECT_EQ (v.front (), "d");
  v.push_back ("bx");
  EXPECT_EQ (v.front (), "d");
  EXPECT_EQ (v.back (), "bx");
  v.push_back ("ba");
  EXPECT_EQ (v.front (), "d");
  EXPECT_EQ (v.back (), "ba");

  EXPECT_EQ (v.begin () [0], "d");
  EXPECT_EQ (v.begin () [1], "a");
  EXPECT_EQ (v.begin () [2], "bx");
  EXPECT_EQ (v.begin () [3], "ba");

  EXPECT_EQ (v.end () [-1], "ba");
  EXPECT_EQ (v.end () [-2], "bx");
  EXPECT_EQ (v.end () [-3], "a");
  EXPECT_EQ (v.end () [-4], "d");

  EXPECT_EQ (*(v.begin () + 3), "ba");
  tl::stable_vector<std::string>::iterator i = v.begin ();
  EXPECT_EQ (*i, "d");
  ++i;
  EXPECT_EQ (*i, "a");
  i += 1;
  EXPECT_EQ (*i, "bx");
  i++;
  EXPECT_EQ (*i, "ba");
  i--;
  EXPECT_EQ (*i, "bx");
  i -= 1;
  EXPECT_EQ (*i, "a");
  --i;
  EXPECT_EQ (*i, "d");
  EXPECT_EQ (i == v.begin (), true);
  EXPECT_EQ (i != v.begin (), false);
  EXPECT_EQ (i == v.end (), false);
  EXPECT_EQ (i != v.end (), true);

  EXPECT_EQ (v.size (), size_t (4));

  tl::sort (v.begin (), v.end ());
  EXPECT_EQ (to_string (v), "a ba bx d ");
  
  tl::sort (v.begin (), v.end (), test_compare ());
  EXPECT_EQ (to_string (v), "d bx ba a ");

  v.insert (v.begin (), "u");
  EXPECT_EQ (to_string (v), "u d bx ba a ");

  v.erase (v.begin ());
  EXPECT_EQ (to_string (v), "d bx ba a ");

  tl::stable_vector<std::string> vv;
  EXPECT_EQ (to_string (vv), "");
  vv.swap (v);
  EXPECT_EQ (to_string (v), "");
  EXPECT_EQ (to_string (vv), "d bx ba a ");
  EXPECT_EQ (v < vv, true);
  EXPECT_EQ (vv < v, false);

  vv.pop_back ();
  EXPECT_EQ (to_string (vv), "d bx ba ");
  EXPECT_EQ (vv.size (), size_t (3));

  v = vv;
  EXPECT_EQ (to_string (vv), "d bx ba ");
  EXPECT_EQ (to_string (v), "d bx ba ");
  EXPECT_EQ (v == vv, true);
  EXPECT_EQ (v != vv, false);

  EXPECT_EQ (vv.empty (), false);
  vv.clear ();
  EXPECT_EQ (v == vv, false);
  EXPECT_EQ (v != vv, true);
  EXPECT_EQ (to_string (vv), "");
  EXPECT_EQ (vv.size (), size_t (0));
  EXPECT_EQ (vv.empty (), true);

  EXPECT_EQ (v.empty (), false);
  v.erase (v.begin (), v.end ());
  EXPECT_EQ (to_string (v), "");
  EXPECT_EQ (v.size (), size_t (0));
  EXPECT_EQ (v.empty (), true);

}

TEST(2) 
{
  tl::stable_vector<std::string> u;
  u.push_back ("d");
  u.push_back ("a");
  u.push_back ("bx");
  u.push_back ("ba");

  const tl::stable_vector<std::string> &v = u;

  EXPECT_EQ (*(v.begin () + 3), "ba");
  tl::stable_vector<std::string>::const_iterator i = v.begin ();
  EXPECT_EQ (*i, "d");
  ++i;
  EXPECT_EQ (*i, "a");
  i += 1;
  EXPECT_EQ (*i, "bx");
  i++;
  EXPECT_EQ (*i, "ba");
  i--;
  EXPECT_EQ (*i, "bx");
  i -= 1;
  EXPECT_EQ (*i, "a");
  --i;
  EXPECT_EQ (*i, "d");
  EXPECT_EQ (i == v.begin (), true);
  EXPECT_EQ (i != v.begin (), false);
  EXPECT_EQ (i == v.end (), false);
  EXPECT_EQ (i != v.end (), true);

  EXPECT_EQ (v.begin () [0], "d");
  EXPECT_EQ (v.begin () [1], "a");
  EXPECT_EQ (v.begin () [2], "bx");
  EXPECT_EQ (v.begin () [3], "ba");
  EXPECT_EQ (*(v.begin () + 3), "ba");
  EXPECT_EQ (v.end () [-1], "ba");
  EXPECT_EQ (v.end () [-2], "bx");
  EXPECT_EQ (v.end () [-3], "a");
  EXPECT_EQ (v.end () [-4], "d");
}

TEST(3) 
{
  tl::stable_vector<int> v;
  tl::stable_vector<int>::stable_const_iterator i1 = v.begin_stable ();
  tl::stable_vector<int>::stable_iterator i2 = v.begin_stable ();
  EXPECT_EQ (((const tl::stable_vector<int> &)v).begin_stable () == i1, true);
  EXPECT_EQ (v.begin_stable () == i2, true);
  EXPECT_EQ (((const tl::stable_vector<int> &)v).end_stable () == i1, true);
  EXPECT_EQ (v.end_stable () == i2, true);

  v.push_back (123);
  EXPECT_EQ (((const tl::stable_vector<int> &)v).end_stable () == i1, false);
  EXPECT_EQ (v.end_stable () == i2, false);
  EXPECT_EQ (((const tl::stable_vector<int> &)v).end_stable () != i1, true);
  EXPECT_EQ (v.end_stable () != i2, true);
  EXPECT_EQ (*i1, 123);
  EXPECT_EQ (*i2, 123);

  v.push_back (200);
  v.push_back (250);
  v.push_back (500);
  v.push_back (12);
  v.push_back (12);
  v.push_back (12);
  v.push_back (12);
  v.push_back (12);
  v.push_back (12);
  EXPECT_EQ (*i1, 123);
  EXPECT_EQ (*i2, 123);
}



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


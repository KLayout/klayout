
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


#include "tlCommandLineParser.h"
#include "tlUnitTest.h"

TEST(1)
{
  std::string a;
  int b = 0;
  bool c = false;
  double d = 1;
  bool e = false;
  std::string f;

  tl::CommandLineOptions cmd;
  cmd << tl::arg ("a", &a, "")
      << tl::arg ("?b", &b, "")
      << tl::arg ("-c", &c, "")
      << tl::arg ("!-cc", &c, "")
      << tl::arg ("--plong|-p", &d, "")
      << tl::arg ("--elong", &e, "")
      << tl::arg ("-f|--flong=value", &f, "");

  {
    const char *argv[] = { "x", "y" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "y");
  EXPECT_EQ (b, 0);

  {
    const char *argv[] = { "x", "z", "17" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "z");
  EXPECT_EQ (b, 17);

  b = 0;
  {
    const char *argv[] = { "x", "u", "-c" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "u");
  EXPECT_EQ (b, 0);
  EXPECT_EQ (c, true);

  b = 0;
  c = true;
  {
    const char *argv[] = { "x", "u", "-cc" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "u");
  EXPECT_EQ (b, 0);
  EXPECT_EQ (c, false);

  b = 0;
  c = true;
  {
    const char *argv[] = { "x", "u", "-cc=false" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "u");
  EXPECT_EQ (b, 0);
  EXPECT_EQ (c, true);

  b = 0;
  c = true;
  {
    const char *argv[] = { "x", "u", "-cc=true" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "u");
  EXPECT_EQ (b, 0);
  EXPECT_EQ (c, false);

  {
    const char *argv[] = { "x", "u", "-c", "-cc" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (c, false);

  b = 0;
  c = false;
  {
    const char *argv[] = { "x", "u", "-c", "-p=21" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "u");
  EXPECT_EQ (b, 0);
  EXPECT_EQ (c, true);
  EXPECT_EQ (d, 21);

  b = 0;
  c = false;
  {
    const char *argv[] = { "x", "u", "-p", "22", "-c" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "u");
  EXPECT_EQ (b, 0);
  EXPECT_EQ (c, true);
  EXPECT_EQ (d, 22);

  e = false;
  {
    const char *argv[] = { "x", "u", "--plong", "23" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "u");
  EXPECT_EQ (d, 23);
  EXPECT_EQ (e, false);

  {
    const char *argv[] = { "x", "u", "--plong=24", "--elong" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "u");
  EXPECT_EQ (d, 24);
  EXPECT_EQ (e, true);

  {
    const char *argv[] = { "x", "u", "-c", "-f=foo" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "u");
  EXPECT_EQ (f, "foo");

  {
    const char *argv[] = { "x", "u", "--flong", "bar" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (a, "u");
  EXPECT_EQ (f, "bar");
}


struct Values
{
  Values ()
  {
    b = 0;
    d = 1;
    c = e = false;
  }

  void set_a (const std::string &x) { a = x; }
  void set_b (int x) { b = x; }
  void set_c (bool x) { c = x; }
  void set_d (const double &x) { d = x; }
  void set_e (bool x) { e = x; }
  void set_f (std::string x) { f = x; }

  std::string a;
  int b;
  bool c;
  double d;
  bool e;
  std::string f;
};

TEST(2)
{
  Values v;

  tl::CommandLineOptions cmd;
  cmd << tl::arg ("a", &v, &Values::set_a, "")
      << tl::arg ("?b", &v, &Values::set_b, "")
      << tl::arg ("-c", &v, &Values::set_c, "")
      << tl::arg ("!-cc", &v, &Values::set_c, "")
      << tl::arg ("--plong|-p", &v, &Values::set_d, "")
      << tl::arg ("--elong", &v, &Values::set_e, "")
      << tl::arg ("-f|--flong=value", &v, &Values::set_f, "");

  {
    const char *argv[] = { "x", "y" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.a, "y");
  EXPECT_EQ (v.b, 0);

  {
    const char *argv[] = { "x", "z", "17" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.a, "z");
  EXPECT_EQ (v.b, 17);

  v.b = 0;
  {
    const char *argv[] = { "x", "u", "-c" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.a, "u");
  EXPECT_EQ (v.b, 0);
  EXPECT_EQ (v.c, true);

  v.b = 0;
  v.c = true;
  {
    const char *argv[] = { "x", "u", "-cc" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.a, "u");
  EXPECT_EQ (v.b, 0);
  EXPECT_EQ (v.c, false);

  {
    const char *argv[] = { "x", "u", "-c", "-cc" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.c, false);

  v.b = 0;
  v.c = false;
  {
    const char *argv[] = { "x", "u", "-c", "-p=21" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.a, "u");
  EXPECT_EQ (v.b, 0);
  EXPECT_EQ (v.c, true);
  EXPECT_EQ (v.d, 21);

  v.b = 0;
  v.c = false;
  {
    const char *argv[] = { "x", "u", "-p", "22", "-c" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.a, "u");
  EXPECT_EQ (v.b, 0);
  EXPECT_EQ (v.c, true);
  EXPECT_EQ (v.d, 22);

  v.e = false;
  {
    const char *argv[] = { "x", "u", "--plong", "23" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.a, "u");
  EXPECT_EQ (v.d, 23);
  EXPECT_EQ (v.e, false);

  {
    const char *argv[] = { "x", "u", "--plong=24", "--elong" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.a, "u");
  EXPECT_EQ (v.d, 24);
  EXPECT_EQ (v.e, true);

  {
    const char *argv[] = { "x", "u", "-c", "-f=foo" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.a, "u");
  EXPECT_EQ (v.f, "foo");

  {
    const char *argv[] = { "x", "u", "--flong", "bar" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (v.a, "u");
  EXPECT_EQ (v.f, "bar");
}

//  Array arguments
TEST(3)
{
  std::vector<std::string> a;
  std::vector<int> b;

  tl::CommandLineOptions cmd;
  cmd << tl::arg ("a", &a, "")
      << tl::arg ("-b", &b, "");

  {
    const char *argv[] = { "x", "r,u,v" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (int (a.size ()), 3);
  EXPECT_EQ (a[0], "r");
  EXPECT_EQ (a[1], "u");
  EXPECT_EQ (a[2], "v");
  EXPECT_EQ (b.empty (), true);

  a.clear ();
  {
    const char *argv[] = { "x", "\"r,u\",v" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (int (a.size ()), 2);
  EXPECT_EQ (a[0], "r,u");
  EXPECT_EQ (a[1], "v");
  EXPECT_EQ (b.empty (), true);

  a.clear ();
  {
    const char *argv[] = { "x", "'\"'", "-b=1,5,-13" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (int (a.size ()), 1);
  EXPECT_EQ (a[0], "\"");
  EXPECT_EQ (int (b.size ()), 3);
  EXPECT_EQ (b[0], 1);
  EXPECT_EQ (b[1], 5);
  EXPECT_EQ (b[2], -13);

  a.clear ();
  b.clear ();
  {
    const char *argv[] = { "x", "", "-b", "-13,21" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (int (a.size ()), 0);
  EXPECT_EQ (int (b.size ()), 2);
  EXPECT_EQ (b[0], -13);
  EXPECT_EQ (b[1], 21);
}

//  Repeated array arguments
TEST(4)
{
  std::vector<std::string> a;
  std::vector<int> b;

  tl::CommandLineOptions cmd;
  cmd << tl::arg ("*-a|--along", &a, "")
      << tl::arg ("*-b|--blong", &b, "");

  {
    const char *argv[] = { "x", "-a", "r,u,v" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (int (a.size ()), 1);
  EXPECT_EQ (a[0], "r,u,v");
  EXPECT_EQ (b.empty (), true);

  a.clear ();
  b.clear ();
  {
    const char *argv[] = { "x", "-b", "1", "-a=r", "-a", "u", "--along=v", "--blong=2" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (int (a.size ()), 3);
  EXPECT_EQ (int (b.size ()), 2);
  EXPECT_EQ (a[0], "r");
  EXPECT_EQ (a[1], "u");
  EXPECT_EQ (a[2], "v");
  EXPECT_EQ (b[0], 1);
  EXPECT_EQ (b[1], 2);
}

//  Repeated and non-repeated plain arguments
TEST(5)
{
  std::string a;
  std::vector<std::string> b;
  std::vector<std::string> c;

  tl::CommandLineOptions cmd;
  cmd << tl::arg ("a", &a, "")
      << tl::arg ("b", &b, "")
      << tl::arg ("?*c", &c, "");

  {
    const char *argv[] = { "x", "y", "r,u,v" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (int (b.size ()), 3);
  EXPECT_EQ (int (c.size ()), 0);
  EXPECT_EQ (a, "y");
  EXPECT_EQ (b[0], "r");
  EXPECT_EQ (b[1], "u");
  EXPECT_EQ (b[2], "v");

  a.clear ();
  b.clear ();
  c.clear ();

  {
    const char *argv[] = { "x", "y", "r,u,v", "a,b", "c", "d" };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }
  EXPECT_EQ (int (b.size ()), 3);
  EXPECT_EQ (int (c.size ()), 3);
  EXPECT_EQ (a, "y");
  EXPECT_EQ (b[0], "r");
  EXPECT_EQ (b[1], "u");
  EXPECT_EQ (b[2], "v");
  EXPECT_EQ (c[0], "a,b");
  EXPECT_EQ (c[1], "c");
  EXPECT_EQ (c[2], "d");
}

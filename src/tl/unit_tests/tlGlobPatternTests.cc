
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



#include "tlGlobPattern.h"
#include "tlUnitTest.h"

TEST(1) 
{
  tl::GlobPattern a ("*");
  tl::GlobPattern b ("a");

  EXPECT_EQ (a.is_catchall (), true);
  EXPECT_EQ (a.is_const (), false);
  EXPECT_EQ (tl::GlobPattern (a).is_catchall (), true);
  EXPECT_EQ (tl::GlobPattern (a).is_const (), false);
  EXPECT_EQ (b.is_catchall (), false);
  EXPECT_EQ (b.is_const (), true);
  EXPECT_EQ (tl::GlobPattern (b).is_catchall (), false);
  EXPECT_EQ (tl::GlobPattern (b).is_const (), true);

  EXPECT_EQ (a.match ("abc"), true);
  EXPECT_EQ (a.match ("a"), true);
  EXPECT_EQ (a.match (""), true);
  EXPECT_EQ (b.match ("abc"), false);
  EXPECT_EQ (b.match ("a"), true);
  EXPECT_EQ (b.match (""), false);
}

TEST(2) 
{
  tl::GlobPattern a ("*a*");
  tl::GlobPattern b ("*a?");
  tl::GlobPattern c ("*a\\?");

  EXPECT_EQ (a.is_catchall (), false);
  EXPECT_EQ (a.is_const (), false);
  EXPECT_EQ (b.is_catchall (), false);
  EXPECT_EQ (b.is_const (), false);
  EXPECT_EQ (c.is_catchall (), false);
  EXPECT_EQ (c.is_const (), false);

  EXPECT_EQ (a.match ("abc"), true);
  EXPECT_EQ (a.match ("a"), true);
  EXPECT_EQ (a.match (""), false);
  EXPECT_EQ (a.match ("bcd"), false);
  EXPECT_EQ (a.match ("bad"), true);
  EXPECT_EQ (a.match ("dba"), true);

  //  copy works too ...
  tl::GlobPattern aa = a;

  EXPECT_EQ (aa.match ("abc"), true);
  EXPECT_EQ (aa.match ("a"), true);
  EXPECT_EQ (aa.match (""), false);
  EXPECT_EQ (aa.match ("bcd"), false);
  EXPECT_EQ (aa.match ("bad"), true);
  EXPECT_EQ (aa.match ("dba"), true);

  EXPECT_EQ (b.match ("abc"), false);
  EXPECT_EQ (b.match ("a"), false);
  EXPECT_EQ (b.match (""), false);
  EXPECT_EQ (b.match ("bcd"), false);
  EXPECT_EQ (b.match ("bad"), true);
  EXPECT_EQ (b.match ("dba"), false);

  EXPECT_EQ (c.match ("bcd"), false);
  EXPECT_EQ (c.match ("bad"), false);
  EXPECT_EQ (c.match ("ba?"), true);
}

TEST(3) 
{
  tl::GlobPattern a ("*a[bcd]");

  EXPECT_EQ (a.match ("ab"), true);
  EXPECT_EQ (a.match ("a"), false);
  EXPECT_EQ (a.match ("had"), true);
  EXPECT_EQ (a.match ("hax"), false);

  //  copy works too ...
  tl::GlobPattern aa = a;

  EXPECT_EQ (aa.match ("ab"), true);
  EXPECT_EQ (aa.match ("a"), false);
  EXPECT_EQ (aa.match ("had"), true);
  EXPECT_EQ (aa.match ("hax"), false);

  tl::GlobPattern b ("a[0-9\\abcdef]");

  EXPECT_EQ (b.match ("a0"), true);
  EXPECT_EQ (b.match ("aa"), true);
  EXPECT_EQ (b.match ("aax"), false);
  EXPECT_EQ (b.match ("ax"), false);
  EXPECT_EQ (b.match ("a"), false);
}

TEST(4) 
{
  tl::GlobPattern a ("*a[^bcd]");

  EXPECT_EQ (a.match ("ab"), false);
  EXPECT_EQ (a.match ("a"), false);
  EXPECT_EQ (a.match ("had"), false);
  EXPECT_EQ (a.match ("hax"), true);

  //  copy works too ...
  tl::GlobPattern aa = a;

  EXPECT_EQ (aa.match ("ab"), false);
  EXPECT_EQ (aa.match ("a"), false);
  EXPECT_EQ (aa.match ("had"), false);
  EXPECT_EQ (aa.match ("hax"), true);
}

TEST(5) 
{
  tl::GlobPattern a ("*a[bcd]*");

  EXPECT_EQ (a.match ("ab"), true);
  EXPECT_EQ (a.match ("a"), false);
  EXPECT_EQ (a.match ("had"), true);

  EXPECT_EQ (a.match ("abx"), true);
  EXPECT_EQ (a.match ("ax"), false);
  EXPECT_EQ (a.match ("hadx"), true);

  //  copy works too ...
  tl::GlobPattern aa = a;

  EXPECT_EQ (aa.match ("ab"), true);
  EXPECT_EQ (aa.match ("a"), false);
  EXPECT_EQ (aa.match ("had"), true);

  EXPECT_EQ (aa.match ("abx"), true);
  EXPECT_EQ (aa.match ("ax"), false);
  EXPECT_EQ (aa.match ("hadx"), true);
}

TEST(6) 
{
  tl::GlobPattern a ("a{bc,d}g");

  EXPECT_EQ (a.match ("abcg"), true);
  EXPECT_EQ (a.match ("adg"), true);
  EXPECT_EQ (a.match ("ad"), false);
  EXPECT_EQ (a.match ("ag"), false);
  EXPECT_EQ (a.match ("abch"), false);
  EXPECT_EQ (a.match ("adh"), false);
  EXPECT_EQ (a.match ("ah"), false);

  //  copy works too ...
  tl::GlobPattern aa = a;

  EXPECT_EQ (aa.match ("abcg"), true);
  EXPECT_EQ (aa.match ("adg"), true);
  EXPECT_EQ (aa.match ("ad"), false);
  EXPECT_EQ (aa.match ("ag"), false);
  EXPECT_EQ (aa.match ("abch"), false);
  EXPECT_EQ (aa.match ("adh"), false);
  EXPECT_EQ (aa.match ("ah"), false);

  a = tl::GlobPattern ("a{,d}g");

  EXPECT_EQ (a.match ("abcg"), false);
  EXPECT_EQ (a.match ("ad"), false);
  EXPECT_EQ (a.match ("adg"), true);
  EXPECT_EQ (a.match ("ag"), true);
  EXPECT_EQ (a.match ("a"), false);
  EXPECT_EQ (a.match ("abch"), false);
  EXPECT_EQ (a.match ("adh"), false);
  EXPECT_EQ (a.match ("ah"), false);

  a = tl::GlobPattern ("a{,d}");

  EXPECT_EQ (a.match ("abcg"), false);
  EXPECT_EQ (a.match ("ad"), true);
  EXPECT_EQ (a.match ("adg"), false);
  EXPECT_EQ (a.match ("ag"), false);
  EXPECT_EQ (a.match ("a"), true);
  EXPECT_EQ (a.match ("abch"), false);
  EXPECT_EQ (a.match ("adh"), false);
  EXPECT_EQ (a.match ("ah"), false);
}

TEST(7)
{
  tl::GlobPattern a ("a{bc*,d?}g");

  EXPECT_EQ (a.match ("abcg"), true);
  EXPECT_EQ (a.match ("adg"), false);
  EXPECT_EQ (a.match ("adxg"), true);
  EXPECT_EQ (a.match ("adxyg"), false);
  EXPECT_EQ (a.match ("ag"), false);
  EXPECT_EQ (a.match ("abch"), false);
  EXPECT_EQ (a.match ("abcg"), true);
  EXPECT_EQ (a.match ("abchg"), true);
  EXPECT_EQ (a.match ("abchhg"), true);
  EXPECT_EQ (a.match ("adh"), false);
  EXPECT_EQ (a.match ("ah"), false);

  //  copy works too ...
  tl::GlobPattern aa = a;

  EXPECT_EQ (aa.match ("abcg"), true);
  EXPECT_EQ (aa.match ("adg"), false);
  EXPECT_EQ (aa.match ("adxg"), true);
  EXPECT_EQ (aa.match ("adxyg"), false);
  EXPECT_EQ (aa.match ("ag"), false);
  EXPECT_EQ (aa.match ("abch"), false);
  EXPECT_EQ (aa.match ("abcg"), true);
  EXPECT_EQ (aa.match ("abchg"), true);
  EXPECT_EQ (aa.match ("abchhg"), true);
  EXPECT_EQ (aa.match ("adh"), false);
  EXPECT_EQ (aa.match ("ah"), false);
}

TEST(8)
{
  tl::GlobPattern a ("(*({bc,d}))(*)");

  std::vector <std::string> v;
  EXPECT_EQ (a.match ("abcg", v), true);
  EXPECT_EQ (v.size (), size_t (3));
  EXPECT_EQ (v[0], "abc");
  EXPECT_EQ (v[1], "bc");
  EXPECT_EQ (v[2], "g");

  //  copy works too ...
  EXPECT_EQ (tl::GlobPattern (a).match ("abcg", v), true);
  EXPECT_EQ (v.size (), size_t (3));
  EXPECT_EQ (v[0], "abc");
  EXPECT_EQ (v[1], "bc");
  EXPECT_EQ (v[2], "g");

  EXPECT_EQ (a.match ("bc", v), true);
  EXPECT_EQ (v.size (), size_t (3));
  EXPECT_EQ (v[0], "bc");
  EXPECT_EQ (v[1], "bc");
  EXPECT_EQ (v[2], "");
}

TEST(9)
{
  //  case insensitive

  tl::GlobPattern a ("(*({bc,d}))(*)");

  std::vector <std::string> v;
  EXPECT_EQ (a.case_sensitive (), true);
  EXPECT_EQ (a.match ("aBcG", v), false);

  a.set_case_sensitive (false);
  EXPECT_EQ (a.case_sensitive (), false);
  EXPECT_EQ (a.match ("aBcG", v), true);
  EXPECT_EQ (v.size (), size_t (3));
  EXPECT_EQ (v[0], "aBc");
  EXPECT_EQ (v[1], "Bc");
  EXPECT_EQ (v[2], "G");

  //  copy works too ...
  EXPECT_EQ (tl::GlobPattern (a).match ("aBcG", v), true);
  EXPECT_EQ (v.size (), size_t (3));
  EXPECT_EQ (v[0], "aBc");
  EXPECT_EQ (v[1], "Bc");
  EXPECT_EQ (v[2], "G");

  tl::GlobPattern b ("*a[bcd]");

  EXPECT_EQ (b.match ("ab"), true);
  EXPECT_EQ (b.match ("Ab"), false);
  EXPECT_EQ (b.match ("aB"), false);

  b.set_case_sensitive (false);
  EXPECT_EQ (b.match ("ab"), true);
  EXPECT_EQ (b.match ("Ab"), true);
  EXPECT_EQ (b.match ("aB"), true);
}

TEST(10)
{
  //  exact match

  tl::GlobPattern a ("(*({bc,d}))(*)");
  a.set_exact (true);

  EXPECT_EQ (a.exact (), true);
  EXPECT_EQ (a.match ("abcg"), false);
  EXPECT_EQ (a.match ("(*({bc,d}))(*)"), true);
  EXPECT_EQ (a.match ("(*({bc,D}))(*)"), false);

  a.set_case_sensitive (false);
  EXPECT_EQ (a.match ("abcg"), false);
  EXPECT_EQ (a.match ("(*({bc,d}))(*)"), true);
  EXPECT_EQ (a.match ("(*({bc,D}))(*)"), true);
}

TEST(11)
{
  //  header match

  tl::GlobPattern a ("abc");
  EXPECT_EQ (a.match ("abcg"), false);
  EXPECT_EQ (a.match ("abc"), true);

  a.set_header_match (true);
  EXPECT_EQ (a.header_match (), true);
  EXPECT_EQ (a.match ("abcg"), true);
  EXPECT_EQ (a.match ("abc"), true);
}

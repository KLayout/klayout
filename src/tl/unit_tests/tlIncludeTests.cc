
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


#include "tlUnitTest.h"
#include "tlFileUtils.h"
#include "tlStream.h"
#include "tlInclude.h"

static std::string np (const std::string &s)
{
  std::string t = tl::replaced (s, "\\\\", "/");
  return tl::replaced (t, "\\", "/");
}

TEST(1_simple)
{
  std::string fn = tl::testdata () + "/tl/x.txt";

  std::string et;
  tl::IncludeExpander ie = tl::IncludeExpander::expand (fn, et);
  EXPECT_EQ (et, "A line\nAnother line\n");
  EXPECT_EQ (ie.to_string (), fn);
  EXPECT_EQ (tl::IncludeExpander::from_string (ie.to_string ()).to_string (), ie.to_string ());

  EXPECT_EQ (ie.translate_to_original (2).first, fn);
  EXPECT_EQ (ie.translate_to_original (2).second, 2);
}

TEST(2_single_include)
{
  std::string fn = tl::testdata () + "/tl/x_inc1.txt";

  std::string et;
  tl::IncludeExpander ie = tl::IncludeExpander::expand (fn, tl::InputStream (fn).read_all (), et);
  EXPECT_EQ (et, "A line\nincluded.1\nAnother line\n");

  EXPECT_EQ (np (ie.to_string ()), np ("@1*" + tl::testdata () + "/tl/x_inc1.txt*0;2*" + tl::testdata () + "/tl/inc1.txt*-1;3*" + tl::testdata () + "/tl/x_inc1.txt*0;"));
  EXPECT_EQ (tl::IncludeExpander::from_string (ie.to_string ()).to_string (), ie.to_string ());

  EXPECT_EQ (ie.translate_to_original (1).first, fn);
  EXPECT_EQ (ie.translate_to_original (1).second, 1);
  EXPECT_EQ (np (ie.translate_to_original (2).first), np (tl::testdata () + "/tl/inc1.txt"));
  EXPECT_EQ (ie.translate_to_original (2).second, 1);
  EXPECT_EQ (ie.translate_to_original (3).first, fn);
  EXPECT_EQ (ie.translate_to_original (3).second, 3);
}

TEST(3_multi_include)
{
  std::string fn = tl::testdata () + "/tl/x_inc3.txt";

  std::string et;
  tl::IncludeExpander ie = tl::IncludeExpander::expand (fn, et);
  EXPECT_EQ (et, "A line\ninclude.3a\nincluded.2a\nincluded.2b\ninclude.3b\nAnother line\n");

  EXPECT_EQ (tl::IncludeExpander::from_string (ie.to_string ()).to_string (), ie.to_string ());

  EXPECT_EQ (ie.translate_to_original (1).first, fn);
  EXPECT_EQ (ie.translate_to_original (1).second, 1);
  EXPECT_EQ (np (ie.translate_to_original (2).first), np (tl::testdata () + "/tl/inc3.txt"));
  EXPECT_EQ (ie.translate_to_original (2).second, 1);
  EXPECT_EQ (np (ie.translate_to_original (3).first), np (tl::testdata () + "/tl/inc2.txt"));
  EXPECT_EQ (ie.translate_to_original (3).second, 1);
  EXPECT_EQ (np (ie.translate_to_original (5).first), np (tl::testdata () + "/tl/inc3.txt"));
  EXPECT_EQ (ie.translate_to_original (5).second, 3);
  EXPECT_EQ (ie.translate_to_original (6).first, fn);
  EXPECT_EQ (ie.translate_to_original (6).second, 3);
}

TEST(4_multi_include_interpolate)
{
  std::string fn = tl::testdata () + "/tl/x_inc3_ip.txt";

  std::string et;
  tl::IncludeExpander ie = tl::IncludeExpander::expand (fn, et);
  EXPECT_EQ (et, "A line\ninclude.3a\nincluded.2a\nincluded.2b\ninclude.3b\nAnother line\n");

  EXPECT_EQ (tl::IncludeExpander::from_string (ie.to_string ()).to_string (), ie.to_string ());

  EXPECT_EQ (ie.translate_to_original (1).first, fn);
  EXPECT_EQ (ie.translate_to_original (1).second, 1);
  EXPECT_EQ (np (ie.translate_to_original (2).first), np (tl::testdata () + "/tl/inc3.txt"));
  EXPECT_EQ (ie.translate_to_original (2).second, 1);
  EXPECT_EQ (np (ie.translate_to_original (3).first), np (tl::testdata () + "/tl/inc2.txt"));
  EXPECT_EQ (ie.translate_to_original (3).second, 1);
  EXPECT_EQ (np (ie.translate_to_original (5).first), np (tl::testdata () + "/tl/inc3.txt"));
  EXPECT_EQ (ie.translate_to_original (5).second, 3);
  EXPECT_EQ (ie.translate_to_original (6).first, fn);
  EXPECT_EQ (ie.translate_to_original (6).second, 3);
}

TEST(5_issue946)
{
  std::string fn = tl::testdata () + "/tl/x_inc4.txt";

  std::string et;
  tl::IncludeExpander ie = tl::IncludeExpander::expand (fn, tl::InputStream (fn).read_all (), et);
  EXPECT_EQ (et, "A line\nincluded.4\nAnother line\n");

  EXPECT_EQ (np (ie.to_string ()), np ("@1*" + tl::testdata () + "/tl/x_inc4.txt*0;2*'" + tl::testdata () + "/tl/inc 4.txt'*-1;3*" + tl::testdata () + "/tl/x_inc4.txt*0;"));
  EXPECT_EQ (tl::IncludeExpander::from_string (ie.to_string ()).to_string (), ie.to_string ());

  EXPECT_EQ (ie.translate_to_original (1).first, fn);
  EXPECT_EQ (ie.translate_to_original (1).second, 1);
  EXPECT_EQ (np (ie.translate_to_original (2).first), np (tl::testdata () + "/tl/inc 4.txt"));
  EXPECT_EQ (ie.translate_to_original (2).second, 1);
  EXPECT_EQ (ie.translate_to_original (3).first, fn);
  EXPECT_EQ (ie.translate_to_original (3).second, 3);

  fn = tl::testdata () + "/tl/inc 4.txt";

  et.clear ();
  ie = tl::IncludeExpander::expand (fn, tl::InputStream (fn).read_all (), et);
  EXPECT_EQ (et, "included.4\n");

  //  no quotes here so this string can be used as the original file name if there is no include
  EXPECT_EQ (np (ie.to_string ()), np (tl::testdata () + "/tl/inc 4.txt"));
}


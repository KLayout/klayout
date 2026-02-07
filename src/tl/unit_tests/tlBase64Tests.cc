
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


#include "tlBase64.h"
#include "tlUnitTest.h"

TEST(1)
{
  std::vector<unsigned char> r = tl::from_base64 ("");
  EXPECT_EQ (r.empty (), true);

  r = tl::from_base64 ("YQ==");
  EXPECT_EQ (std::string ((const char *) r.begin ().operator-> (), r.size ()), "a");

  r = tl::from_base64 ("YQ==");
  EXPECT_EQ (std::string ((const char *) r.begin ().operator-> (), r.size ()), "a");

  r = tl::from_base64 ("YWI=");
  EXPECT_EQ (std::string ((const char *) r.begin ().operator-> (), r.size ()), "ab");

  r = tl::from_base64 ("YWJj");
  EXPECT_EQ (std::string ((const char *) r.begin ().operator-> (), r.size ()), "abc");

  r = tl::from_base64 ("YWJjZA==");
  EXPECT_EQ (std::string ((const char *) r.begin ().operator-> (), r.size ()), "abcd");

  r = tl::from_base64 ("YWJjZA=");
  EXPECT_EQ (std::string ((const char *) r.begin ().operator-> (), r.size ()), "abcd");

  r = tl::from_base64 ("SGVsbG8sIHdvcmxkIQo=");
  EXPECT_EQ (std::string ((const char *) r.begin ().operator-> (), r.size ()), "Hello, world!\n");

  r = tl::from_base64 ("SGVsbG\n8sIHd  \tvcmxkIQo=");
  EXPECT_EQ (std::string ((const char *) r.begin ().operator-> (), r.size ()), "Hello, world!\n");

  try {
    r = tl::from_base64 ("YWJjZ==");
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Error decoding base64 data: padding character does not match zero byte");
  }

  try {
    r = tl::from_base64 ("YW#jZA==");
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Error decoding base64 data: invalid character '#'");
  }
}

TEST(2)
{
  std::string s, r;

  s = "";
  r = tl::to_base64 ((const unsigned char *) s.c_str (), s.size ());
  EXPECT_EQ (r, "");

  s = "a";
  r = tl::to_base64 ((const unsigned char *) s.c_str (), s.size ());
  EXPECT_EQ (r, "YQ==");

  s = "ab";
  r = tl::to_base64 ((const unsigned char *) s.c_str (), s.size ());
  EXPECT_EQ (r, "YWI=");

  s = "abc";
  r = tl::to_base64 ((const unsigned char *) s.c_str (), s.size ());
  EXPECT_EQ (r, "YWJj");

  s = "abcd";
  r = tl::to_base64 ((const unsigned char *) s.c_str (), s.size ());
  EXPECT_EQ (r, "YWJjZA==");

  s = "Hello, world!\n";
  r = tl::to_base64 ((const unsigned char *) s.c_str (), s.size ());
  EXPECT_EQ (r, "SGVsbG8sIHdvcmxkIQo=");
}

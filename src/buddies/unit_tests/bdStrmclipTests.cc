
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

#include "bdCommon.h"
#include "dbReader.h"
#include "dbTestSupport.h"
#include "tlUnitTest.h"

BD_PUBLIC int strmclip (int argc, char *argv[]);

TEST(1A)
{
  std::string input = tl::testdata ();
  input += "/bd/strm2clip_in.gds";

  std::string au = tl::testdata ();
  au += "/bd/strm2clip_au1.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str (), "-r=0,-2,9,5", "-r=13,-2,16,3", "-r=13,5,16,7" };

  EXPECT_EQ (strmclip (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NoNormalization);
}

TEST(1B)
{
  std::string input = tl::testdata ();
  input += "/bd/strm2clip_in.gds";

  std::string au = tl::testdata ();
  au += "/bd/strm2clip_au1.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str (), "-l=100/0" };

  EXPECT_EQ (strmclip (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NoNormalization);
}

TEST(2)
{
  std::string input = tl::testdata ();
  input += "/bd/strm2clip_in.gds";

  std::string au = tl::testdata ();
  au += "/bd/strm2clip_au2.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str (), "-r=0,-2,9,5", "-t", "INV2", "-x=CLIP_OUT" };

  EXPECT_EQ (strmclip (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NoNormalization);
}


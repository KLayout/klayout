
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "utHead.h"
#include "bdCommon.h"
#include "dbReader.h"
#include "tlLog.h"

#include <sstream>

BD_PUBLIC int strmcmp (int argc, char *argv[]);


TEST(1)
{
  ut::CaptureChannel cap;

  tl::info << "Self test";
  EXPECT_EQ (cap.captured_text (), "Self test\n");
  cap.clear ();
  EXPECT_EQ (cap.captured_text (), "");

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref1.gds";

  char *argv[] = { "x", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 0);

  EXPECT_EQ (cap.captured_text (), "");
}

TEST(2A)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref2.gds";

  char *argv[] = { "x", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Boxes differ for layer 8/0 in cell RINGO\n"
    "Not in b but in a:\n"
    "  (-1720,1600;23160,2000)\n"
    "Not in a but in b:\n"
    "  (-1520,1600;23160,2000)\n"
    "Texts differ for layer 8/1 in cell RINGO\n"
    "Not in b but in a:\n"
    "  ('FB',r0 0,1800)\n"
    "Not in a but in b:\n"
    "  ('BF',r0 0,1800)\n"
    "Layouts differ\n"
  );
}

TEST(2B)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref2.gds";

  char *argv[] = { "x", "-s", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (), "");
}

TEST(2C)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref2.gds";

  char *argv[] = { "x", "-am=8/0", "-as", "-bm=8/0", "-bs", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Boxes differ for layer 8/0 in cell RINGO\n"
    "Not in b but in a:\n"
    "  (-1720,1600;23160,2000)\n"
    "Not in a but in b:\n"
    "  (-1520,1600;23160,2000)\n"
    "Layouts differ\n"
  );
}

TEST(2D)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref2.gds";

  char *argv[] = { "x", "-m=1", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "...\n"
    "Report is shortened after 0 lines.\n"
    "Layouts differ\n"
  );
}

TEST(2E)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref2.gds";

  char *argv[] = { "x", "-ta=INV2", "-tb=INV2", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 0);
}

TEST(2F)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref2.gds";

  char *argv[] = { "x", "-u", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Bounding boxes differ for cell RINGO, (-1720,-800;25160,3800) vs. (-1700,-800;25160,3800)\n"
    "Per-layer bounding boxes differ for cell RINGO, layer (8/0), (-1720,-450;25160,3250) vs. (-1520,-450;25160,3250)\n"
    "Boxes differ for layer 8/0 in cell RINGO\n"
    "Texts differ for layer 8/1 in cell RINGO\n"
    "Layouts differ\n"
  );
}

TEST(3A)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref3.gds";

  char *argv[] = { "x", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Boxes differ for layer 8/0 in cell RINGO\n"
    "Not in b but in a:\n"
    "  (-1720,1600;23160,2000)\n"
    "Not in a but in b:\n"
    "  (-1721,1600;23160,2000)\n"
    "Layouts differ\n"
  );
}

TEST(3B)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref3.gds";

  char *argv[] = { "x", "-t=0.001", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 0);
}

TEST(4A)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref4.gds";

  char *argv[] = { "x", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Paths differ for layer 3/0 in cell TRANS\n"
    "Not in b but in a:\n"
    "  (0,-800;0,800) w=250 bx=0 ex=0 r=false\n"
    "Not in a but in b:\n"
    "Boxes differ for layer 3/0 in cell TRANS\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  (-125,-800;125,800)\n"
    "Layouts differ\n"
  );
}

TEST(4B)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref4.gds";

  char *argv[] = { "x", "-p", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 0);
}

TEST(5A)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref5.gds";

  char *argv[] = { "x", "-u", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Instances differ in cell RINGO\n"
    "Layouts differ\n"
  );
}

TEST(5B)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref5.gds";

  char *argv[] = { "x", "--expand-arrays", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 0);
}

TEST(6A)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref6.gds";

  char *argv[] = { "x", "-r", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Boxes differ for layer 8/0 in cell RINGO\n"
    "Not in b but in a:\n"
    "  (-1720,1600;23160,2000)\n"
    "Not in a but in b:\n"
    "  (-1720,1600;23160,2000) {1 {VALUE}}\n"
    "Layouts differ\n"
  );
}

TEST(6B)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref6.gds";

  char *argv[] = { "x", "-np", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 0);
}

TEST(7A)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref7.oas";

  char *argv[] = { "x", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Layer names differ between layout a and b for layer 3/0:  vs. NAME\n"
    "Layouts differ\n"
  );
}

TEST(7B)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref7.oas";

  char *argv[] = { "x", "-nl", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 0);
}

TEST(8A)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref8.gds";

  char *argv[] = { "x", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Cell TRANS is not present in layout b, but in a\n"
    "Cell SNART is not present in layout a, but in b\n"
    "Instances differ in cell INV2\n"
    "Not in b but in a:\n"
    "  TRANS r0 *1 -400,0\n"
    "  TRANS r0 *1 -400,2800\n"
    "  TRANS m0 *1 400,0\n"
    "  TRANS m0 *1 400,2800\n"
    "Not in a but in b:\n"
    "  SNART r0 *1 -400,0\n"
    "  SNART r0 *1 -400,2800\n"
    "  SNART m0 *1 400,0\n"
    "  SNART m0 *1 400,2800\n"
    "Layouts differ\n"
  );
}

TEST(8B)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref8.gds";

  char *argv[] = { "x", "-c", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 0);

  EXPECT_EQ (cap.captured_text (),
    "Cell TRANS in a is renamed to SNART in b\n"
  );
}

TEST(9A)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref9.gds";

  char *argv[] = { "x", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Layer 8/1 is not present in layout b, but in a\n"
    "Layouts differ\n"
  );
}

TEST(9B)
{
  ut::CaptureChannel cap;

  std::string input_a = ut::testsrc ();
  input_a += "/testdata/bd/strmcmp_in.gds";

  std::string input_b = ut::testsrc ();
  input_b += "/testdata/bd/strmcmp_ref9.gds";

  char *argv[] = { "x", "-l", const_cast<char *> (input_a.c_str ()), const_cast<char *> (input_b.c_str ()) };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Texts differ for layer 8/1 in cell RINGO\n"
    "Not in b but in a:\n"
    "  ('FB',r0 0,1800)\n"
    "  ('OSC',r0 24560,1800)\n"
    "  ('VDD',r0 0,2800)\n"
    "  ('VSS',r0 0,0)\n"
    "Not in a but in b:\n"
    "Layouts differ\n"
  );
}

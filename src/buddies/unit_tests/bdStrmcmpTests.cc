
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
#include "tlUnitTest.h"
#include "tlLog.h"

#include <sstream>

BD_PUBLIC int strmcmp (int argc, char *argv[]);


TEST(1)
{
  tl::CaptureChannel cap;

  tl::info << "Self test";
  EXPECT_EQ (cap.captured_text (), "Self test\n");
  cap.clear ();
  EXPECT_EQ (cap.captured_text (), "");

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref1.gds";

  const char *argv[] = { "x", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);

  EXPECT_EQ (cap.captured_text (), "");
}

TEST(2A)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref2.gds";

  const char *argv[] = { "x", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

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
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref2.gds";

  const char *argv[] = { "x", "-s", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (), "");
}

TEST(2C)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref2.gds";

  const char *argv[] = { "x", "-am=8/0", "-as", "-bm=8/0", "-bs", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

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
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref2.gds";

  const char *argv[] = { "x", "-m=1", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "...\n"
    "Report is shortened after 0 lines.\n"
    "Layouts differ\n"
  );
}

TEST(2E)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref2.gds";

  const char *argv[] = { "x", "-ta=INV2", "-tb=INV2", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);
}

TEST(2F)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref2.gds";

  const char *argv[] = { "x", "-u", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

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
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref3.gds";

  const char *argv[] = { "x", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

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
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref3.gds";

  const char *argv[] = { "x", "-t=0.001", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);
}

TEST(4A)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref4.gds";

  const char *argv[] = { "x", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

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
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref4.gds";

  const char *argv[] = { "x", "-p", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);
}

TEST(5A)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref5.gds";

  const char *argv[] = { "x", "-u", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Instances differ in cell RINGO\n"
    "Layouts differ\n"
  );
}

TEST(5B)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref5.gds";

  const char *argv[] = { "x", "--expand-arrays", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);
}

TEST(6A)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref6.gds";

  const char *argv[] = { "x", "-r", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

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
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref6.gds";

  const char *argv[] = { "x", "-np", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);
}

TEST(7A)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref7.oas";

  const char *argv[] = { "x", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Layer names differ between layout a and b for layer 3/0:  vs. NAME\n"
    "Layouts differ\n"
  );
}

TEST(7B)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref7.oas";

  const char *argv[] = { "x", "-nl", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);
}

TEST(8A)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref8.gds";

  const char *argv[] = { "x", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

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
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref8.gds";

  const char *argv[] = { "x", "-c", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);

  EXPECT_EQ (cap.captured_text (),
    "Cell TRANS in a is renamed to SNART in b\n"
  );
}

TEST(9A)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref9.gds";

  const char *argv[] = { "x", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Layer 8/1 is not present in layout b, but in a\n"
    "Layouts differ\n"
  );
}

TEST(9B)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmcmp_in.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmcmp_ref9.gds";

  const char *argv[] = { "x", "-l", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmcmp (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Texts differ for layer 8/1 in cell RINGO\n"
    "Not in b but in a:\n"
    "  ('VSS',r0 0,0)\n"
    "  ('FB',r0 0,1800)\n"
    "  ('OSC',r0 24560,1800)\n"
    "  ('VDD',r0 0,2800)\n"
    "Not in a but in b:\n"
    "Layouts differ\n"
  );
}

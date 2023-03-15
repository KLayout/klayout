
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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
#include "tlLog.h"
#include "tlUnitTest.h"

#include <sstream>

BD_PUBLIC int strmxor (int argc, char *argv[]);

TEST(0_Basic_Flat)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in1.gds";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);

  EXPECT_EQ (cap.captured_text (),
    "No differences found\n"
  );
}

TEST(0_Basic_Deep)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in1.gds";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-u", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 0);

  EXPECT_EQ (cap.captured_text (),
    "No differences found\n"
  );
}

TEST(1A_Flat)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au1.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
    "Result summary (layers without differences are not shown):\n"
    "\n"
    "  Layer      Output       Differences (shape count)\n"
    "  -------------------------------------------------------\n"
    "  3/0        3/0          30\n"
    "  6/0        6/0          41\n"
    "  8/1        8/1          1\n"
    "  10/0       -            (no such layer in first layout)\n"
    "\n"
  );
}

TEST(1A_Deep)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au1d.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "--deep", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
    "Result summary (layers without differences are not shown):\n"
    "\n"
    "  Layer      Output       Differences (shape count)\n"
    "  -------------------------------------------------------\n"
    "  3/0        3/0          3\n"
    "  6/0        6/0          314\n"
    "  8/1        8/1          1\n"
    "  10/0       -            (no such layer in first layout)\n"
    "\n"
  );
}

TEST(1B_Flat)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
    "Result summary (layers without differences are not shown):\n"
    "\n"
    "  Layer      Output       Differences (shape count)\n"
    "  -------------------------------------------------------\n"
    "  3/0        -            30\n"
    "  6/0        -            41\n"
    "  8/1        -            1\n"
    "  10/0       -            (no such layer in first layout)\n"
    "\n"
  );
}

TEST(1B_Deep)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-u", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
    "Result summary (layers without differences are not shown):\n"
    "\n"
    "  Layer      Output       Differences (shape count)\n"
    "  -------------------------------------------------------\n"
    "  3/0        -            30\n"
    "  6/0        -            314\n"
    "  8/1        -            1\n"
    "  10/0       -            (no such layer in first layout)\n"
    "\n"
  );
}

TEST(1C_Flat)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "--no-summary", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(1C_Deep)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-u", "--no-summary", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(1D_Flat)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-s", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);
  EXPECT_EQ (cap.captured_text (), "");
}

TEST(1D_Deep)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-u", "-s", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);
  EXPECT_EQ (cap.captured_text (), "");
}

TEST(2_Flat)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au2.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "--no-summary", "-l", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    ""
  );
}

TEST(2_Deep)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au2d.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-u", "--no-summary", "-l", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    ""
  );
}

TEST(3_Flat)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au3.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "--no-summary", "-p=1.0", "-n=4", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(3_FlatCount)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au3.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-p=1.0", "-n=4", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
    "Result summary (layers without differences are not shown):\n"
    "\n"
    "  Layer      Output       Differences (shape count)\n"
    "  -------------------------------------------------------\n"
    "  3/0        -            31\n"
    "  6/0        -            217\n"
    "  8/1        -            168\n"
    "  10/0       -            (no such layer in first layout)\n"
    "\n"
  );
}

TEST(3_FlatHeal)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au3_heal.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "--heal", "--no-summary", "-p=1.0", "-n=4", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(3_FlatCountHeal)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au3.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-m", "-p=1.0", "-n=4", input_a.c_str (), input_b.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
    "Result summary (layers without differences are not shown):\n"
    "\n"
    "  Layer      Output       Differences (shape count)\n"
    "  -------------------------------------------------------\n"
    "  3/0        -            30\n"
    "  6/0        -            41\n"
    "  8/1        -            1\n"
    "  10/0       -            (no such layer in first layout)\n"
    "\n"
  );
}

TEST(3_Deep)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au3d.oas";

  std::string output = this->tmp_file ("tmp.oas");

  //  NOTE: -p is ignored in deep mode
  const char *argv[] = { "x", "-u", "--no-summary", "-p=1.0", "-n=4", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(4_Flat)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au4.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "--no-summary", "-p=1.0", "-n=4", "-t=0.0,0.005,0.01,0.02,0.09,0.1", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(4_FlatHeal)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au4_heal.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "--heal", "--no-summary", "-p=1.0", "-n=4", "-t=0.0,0.005,0.01,0.02,0.09,0.1", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(4_Deep)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au4d.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-u", "--no-summary", "-p=1.0", "-n=4", "-t=0.0,0.005,0.01,0.02,0.09,0.1", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(5_Flat)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au5.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "--no-summary", "-b=1000", "-t=0.0,0.005,0.01,0.02,0.09,0.1", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(5_Deep)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au5d.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-u", "--no-summary", "-b=1000", "-t=0.0,0.005,0.01,0.02,0.09,0.1", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(6_Flat)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au6.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "--no-summary", "-ta=INV2", "-tb=2VNI", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

TEST(6_Deep)
{
  tl::CaptureChannel cap;

  std::string input_a = tl::testdata ();
  input_a += "/bd/strmxor_in1.gds";

  std::string input_b = tl::testdata ();
  input_b += "/bd/strmxor_in2.gds";

  std::string au = tl::testdata ();
  au += "/bd/strmxor_au6d.oas";

  std::string output = this->tmp_file ("tmp.oas");

  const char *argv[] = { "x", "-u", "--no-summary", "-ta=INV2", "-tb=2VNI", input_a.c_str (), input_b.c_str (), output.c_str () };

  EXPECT_EQ (strmxor (sizeof (argv) / sizeof (argv[0]), (char **) argv), 1);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (this, layout, au, db::NormalizationMode (db::NoNormalization | db::AsPolygons));
  EXPECT_EQ (cap.captured_text (),
    "Layer 10/0 is not present in first layout, but in second\n"
  );
}

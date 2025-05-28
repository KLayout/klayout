
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


#include "dbMALYReader.h"
#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbTestSupport.h"
#include "tlUnitTest.h"

#include <stdlib.h>

static void run_test (tl::TestBase *_this, const std::string &base, const char *file, const char *file_au, const char *map = 0, double dbu = 0.001)
{
  db::MALYReaderOptions *opt = new db::MALYReaderOptions();
  opt->dbu = dbu;

  db::LayerMap lm;
  if (map) {
    unsigned int ln = 0;
    tl::Extractor ex (map);
    while (! ex.at_end ()) {
      std::string n;
      int l;
      ex.read_word_or_quoted (n);
      ex.test (":");
      ex.read (l);
      ex.test (",");
      lm.map (n, ln++, db::LayerProperties (l, 0));
    }
    opt->layer_map = lm;
    opt->create_other_layers = true;
  }

  db::LoadLayoutOptions options;
  options.set_options (opt);

  db::Manager m (false);
  db::Layout layout (&m);

  {
    std::string fn (base);
    fn += "/maly/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  std::string fn_au (base);
  fn_au += "/maly/";
  fn_au += file_au;

  db::compare_layouts (_this, layout, fn_au, db::WriteOAS);
}

TEST(1_Basic)
{
  std::string fn (tl::testdata ());
  fn += "/maly/MALY_test1.maly";

  tl::InputStream stream (fn);
  db::MALYReader reader (stream);

  db::MALYData data = reader.read_maly_file ();

  EXPECT_EQ (data.to_string (),
    "Mask A\n"
    "  Size 127000\n"
    "    Title \"<SERIAL>\" m90 0,-50 1,1,1 [Standard]\n"
    "    Title \"MaskA1\" m90 50,50 1,1,1 [Standard]\n"
    "    Title \"WITH \"QUOTES\"\" r270 -50,0 1,1,1 [Standard]\n"
    "    Ref A1.oas{CHIP_A}(1) (0,0;10,10) m90 *1 20,0\n"
    "    Ref A2.oas{CHIP_A}(2) ename(e001) dname(d001) (0,0;50,50) m90 *0.8 20,0 [2x5,1x2]\n"
    "    Ref B3.oas{CHIP_A}(2) (0,0;12,12) m90 *1 20,0"
  )
}

static std::string run_test_with_error (tl::TestBase * /*_this*/, const std::string &file)
{
  std::string fn (tl::testdata ());
  fn += "/maly/";
  fn += file;

  tl::InputStream stream (fn);
  db::MALYReader reader (stream);

  try {
    reader.read_maly_file ();
    tl_assert (false);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    return ex.msg ();
  }
}

TEST(2_Errors)
{
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2a.maly").find ("Line break inside quoted string (line=17,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2b.maly").find ("/*...*/ comment not closed (line=43,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2c.maly").find ("Expected value STANDARD or NATIVE for FONT (line=7,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2d.maly").find ("Unknown base specification: NOVALIDBASE (line=8,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2e.maly").find ("Expected end of text here: NOVALIDKEY .. (line=15,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2f.maly").find ("Expected 'Y' or 'NONE' for MIRROR spec (line=15,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2g.maly").find ("Expected end of text here: UNEXPECTED (line=20,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2h.maly").find ("Expected value Y or NONE for MASKMIRROR (line=23,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2i.maly").find ("Expected end of text here: UNEXPECTED (line=29,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2j.maly").find ("Expected end of text here: NOVALIDKEY .. (line=30,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2k.maly").find ("Expected a real number here: SCALE 0.80 .. (line=31,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2l.maly").find ("Expected 'PARAMETER' here: CMASK (line=19,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2m.maly").find ("Expected 'CMASK' here: TITLE (line=18,"), size_t (0));
  EXPECT_EQ (run_test_with_error (_this, "MALY_test2n.maly").find ("Header expected ('BEGIN MALY') (line=2, "), size_t (0));
}

TEST(10_BasicLayout)
{
  run_test (_this, tl::testdata (), "MALY_test10.maly", "maly_test10_au.oas");
  run_test (_this, tl::testdata (), "MALY_test10.maly", "maly_test10_lm_au.oas", "A: 10, B: 11, C: 12, D: 13");
}

TEST(11_Titles)
{
  run_test (_this, tl::testdata (), "MALY_test11.maly", "maly_test11_au.oas");
}


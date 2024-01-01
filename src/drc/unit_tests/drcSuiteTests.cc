
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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
#include "dbReader.h"
#include "dbTestSupport.h"
#include "lymMacro.h"

void runtest (tl::TestBase *_this, int mode)
{
  std::string force_gc = "true";

  std::string rs = tl::testdata ();
  rs += "/drc/drcSuiteTests.drc";

  std::string input = tl::testdata ();
  input += "/drc/drctest.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSuiteTests_au";
  au += tl::to_string (mode);
  au += ".oas";

  std::string output = _this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = %s\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
        "$drc_test_mode = %d\n"
      , force_gc, input, output, mode)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro drc;
  drc.load_from (rs);
  EXPECT_EQ (drc.run (), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  //  NOTE: WriteOAS normalization will remove shape duplicates. For TEST(3)
  //  shape duplicates are produced because we use a rather small tile size
  //  and clipping of error shapes does not happen. This normalization removes
  //  these redundancies.
  db::compare_layouts (_this, layout, au, db::WriteOAS);
}

TEST(1_Flat)
{
  runtest (_this, 1);
}

TEST(2_BigFlat)
{
  test_is_long_runner ();
  runtest (_this, 2);
}

TEST(3_Tiled)
{
  test_is_long_runner ();
  runtest (_this, 3);
}

TEST(4_BigTiled)
{
  test_is_long_runner ();
  runtest (_this, 4);
}

TEST(5_Hier)
{
  runtest (_this, 5);
}

TEST(6_BigHier)
{
  test_is_long_runner ();
  runtest (_this, 6);
}

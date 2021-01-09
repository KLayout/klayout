
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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
#include "dbNetlist.h"
#include "dbNetlistSpiceReader.h"
#include "lymMacro.h"
#include "tlFileUtils.h"

static void run_test (tl::TestBase *_this, const std::string &number, bool deep)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcGenericTests_" + number + ".drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/drcGenericTests_" + number + ".gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcGenericTests_au" + number + std::string (deep ? "d" : "") + ".gds";

  std::string output = _this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
        "$drc_test_deep = %s\n"
      , input, output, deep ? "true" : "false")
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

  db::compare_layouts (_this, layout, au, db::NoNormalization);
}

TEST(1)
{
  run_test (_this, "1", false);
}

TEST(1d)
{
  run_test (_this, "1", true);
}

TEST(2)
{
  run_test (_this, "2", false);
}

TEST(2d)
{
  run_test (_this, "2", true);
}

TEST(3)
{
  run_test (_this, "3", false);
}

TEST(3d)
{
  run_test (_this, "3", true);
}

TEST(4)
{
  run_test (_this, "4", false);
}

TEST(4d)
{
  run_test (_this, "4", true);
}

TEST(5)
{
  run_test (_this, "5", false);
}

TEST(5d)
{
  run_test (_this, "5", true);
}

TEST(6)
{
  run_test (_this, "6", false);
}

TEST(6d)
{
  run_test (_this, "6", true);
}

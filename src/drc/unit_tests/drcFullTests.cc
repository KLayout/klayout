
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
#include "dbReader.h"
#include "dbTestSupport.h"
#include "lymMacro.h"

TEST(1_IHPMetal1Fill)
{
  test_is_long_runner ();

  std::string rs = tl::testdata ();
  rs += "/drc/drcFullTest_1a.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcFullTest_1.oas";

  std::string au = tl::testdata ();
  au += "/drc/drcFullTest_au1a.oas";

  std::string output = this->tmp_file ("tmp.oas");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
      , input, output)
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

TEST(1b_IHPMetal1FillAutoOrigin)
{
  test_is_long_runner ();

  std::string rs = tl::testdata ();
  rs += "/drc/drcFullTest_1b.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcFullTest_1.oas";

  std::string au = tl::testdata ();
  au += "/drc/drcFullTest_au1b.oas";

  std::string output = this->tmp_file ("tmp.oas");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
      , input, output)
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

TEST(1c_IHPMetal1FillSingleOrigin)
{
  test_is_long_runner ();

  std::string rs = tl::testdata ();
  rs += "/drc/drcFullTest_1c.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcFullTest_1.oas";

  std::string au = tl::testdata ();
  au += "/drc/drcFullTest_au1c.oas";

  std::string output = this->tmp_file ("tmp.oas");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
      , input, output)
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


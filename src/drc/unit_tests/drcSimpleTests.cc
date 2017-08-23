
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
#include "dbReader.h"
#include "lymMacro.h"

TEST(1)
{
  std::string rs = ut::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_1.drc";

  std::string au = ut::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au1.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_test_source = nil\n"
        "$drc_test_target = \"%s\"\n"
      , output)
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

  this->compare_layouts (layout, au, ut::NoNormalization);
}

TEST(2)
{
  std::string rs = ut::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_2.drc";

  std::string input = ut::testsrc ();
  input += "/testdata/drc/drctest.gds";

  std::string au = ut::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au2.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_test_source = \"%s\"\n"
        "$drc_test_target = \"%s\"\n"
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

  this->compare_layouts (layout, au, ut::NoNormalization);
}

TEST(3)
{
  std::string rs = ut::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_3.drc";

  std::string input = ut::testsrc ();
  input += "/testdata/drc/drctest.gds";

  std::string au = ut::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au3.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_test_source = \"%s\"\n"
        "$drc_test_target = \"%s\"\n"
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

  this->compare_layouts (layout, au, ut::NoNormalization);
}

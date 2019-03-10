
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
#include "tlFileUtils.h"

TEST(1)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_1.drc";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au1.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_test_source = nil\n"
        "$drc_test_target = '%s'\n"
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

  db::compare_layouts (_this, layout, au, db::NoNormalization);
}

TEST(2)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_2.drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/drctest.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au2.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
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

TEST(3_Flat)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_3.drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/drctest.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au3.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
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

TEST(4_Hierarchical)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_4.drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/drctest.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au4.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
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

TEST(5_FlatAntenna)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_5.drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/antenna_l1.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au5.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
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

TEST(6_HierarchicalAntenna)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_6.drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/antenna_l1.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au6.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
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

TEST(7_AntennaWithDiodes)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_7.drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/antenna_l1.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au7.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
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

TEST(8_TextsAndPolygons)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_8.drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/texts.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au8.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
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

TEST(9_NetlistExtraction)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_9.drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/ringo.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au9a.cir";

  std::string au_simplified = tl::testsrc ();
  au_simplified += "/testdata/drc/drcSimpleTests_au9b.cir";

  std::string output = this->tmp_file ("tmp.cir");
  std::string output_simplified = this->tmp_file ("tmp_simplified.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
        "$drc_test_target_simplified = '%s'\n"
      , input, output, output_simplified)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro drc;
  drc.load_from (rs);
  EXPECT_EQ (drc.run (), 0);


  //  verify

  {
    tl::InputStream is (output);
    tl::InputStream is_au (au);

    if (is.read_all () != is_au.read_all ()) {
      _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s",
                                 tl::absolute_file_path (output),
                                 tl::absolute_file_path (au)));
    }
  }

  {
    tl::InputStream is (output_simplified);
    tl::InputStream is_au (au_simplified);

    if (is.read_all () != is_au.read_all ()) {
      _this->raise (tl::sprintf ("Compare failed (simplified netlist) - see\n  actual: %s\n  golden: %s",
                                 tl::absolute_file_path (output_simplified),
                                 tl::absolute_file_path (au_simplified)));
    }
  }
}

TEST(10_NetlistExtractionFlat)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_10.drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/ringo.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au10a.cir";

  std::string au_simplified = tl::testsrc ();
  au_simplified += "/testdata/drc/drcSimpleTests_au10b.cir";

  std::string output = this->tmp_file ("tmp.cir");
  std::string output_simplified = this->tmp_file ("tmp_simplified.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
        "$drc_test_target_simplified = '%s'\n"
      , input, output, output_simplified)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro drc;
  drc.load_from (rs);
  EXPECT_EQ (drc.run (), 0);


  //  verify

  {
    tl::InputStream is (output);
    tl::InputStream is_au (au);

    if (is.read_all () != is_au.read_all ()) {
      _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s",
                                 tl::absolute_file_path (output),
                                 tl::absolute_file_path (au)));
    }
  }

  {
    tl::InputStream is (output_simplified);
    tl::InputStream is_au (au_simplified);

    if (is.read_all () != is_au.read_all ()) {
      _this->raise (tl::sprintf ("Compare failed (simplified netlist) - see\n  actual: %s\n  golden: %s",
                                 tl::absolute_file_path (output_simplified),
                                 tl::absolute_file_path (au_simplified)));
    }
  }
}

TEST(11_CustomDevices)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/drc/drcSimpleTests_11.drc";

  std::string input = tl::testsrc ();
  input += "/testdata/drc/vdiv.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/drc/drcSimpleTests_au11a.cir";

  std::string au_simplified = tl::testsrc ();
  au_simplified += "/testdata/drc/drcSimpleTests_au11b.cir";

  std::string output = this->tmp_file ("tmp.cir");
  std::string output_simplified = this->tmp_file ("tmp_simplified.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
        "$drc_test_target_simplified = '%s'\n"
      , input, output, output_simplified)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro drc;
  drc.load_from (rs);
  EXPECT_EQ (drc.run (), 0);


  //  verify

  {
    tl::InputStream is (output);
    tl::InputStream is_au (au);

    if (is.read_all () != is_au.read_all ()) {
      _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s",
                                 tl::absolute_file_path (output),
                                 tl::absolute_file_path (au)));
    }
  }

  {
    tl::InputStream is (output_simplified);
    tl::InputStream is_au (au_simplified);

    if (is.read_all () != is_au.read_all ()) {
      _this->raise (tl::sprintf ("Compare failed (simplified netlist) - see\n  actual: %s\n  golden: %s",
                                 tl::absolute_file_path (output_simplified),
                                 tl::absolute_file_path (au_simplified)));
    }
  }
}

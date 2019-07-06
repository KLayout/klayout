
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
#include "dbNetlist.h"
#include "dbNetlistSpiceReader.h"
#include "lymMacro.h"
#include "tlFileUtils.h"

#if 0 // @@@
TEST(1)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/lvs/lvsSimpleTests_1.lvs";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au1.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = nil\n"
        "$lvs_test_target = '%s'\n"
      , output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

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
  rs += "/testdata/lvs/lvsSimpleTests_2.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/lvstest.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au2.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

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
  rs += "/testdata/lvs/lvsSimpleTests_3.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/lvstest.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au3.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

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
  rs += "/testdata/lvs/lvsSimpleTests_4.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/lvstest.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au4.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

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
  rs += "/testdata/lvs/lvsSimpleTests_5.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/antenna_l1.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au5.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

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
  rs += "/testdata/lvs/lvsSimpleTests_6.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/antenna_l1.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au6.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

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
  rs += "/testdata/lvs/lvsSimpleTests_7.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/antenna_l1.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au7.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

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
  rs += "/testdata/lvs/lvsSimpleTests_8.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/texts.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au8.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  db::compare_layouts (_this, layout, au, db::NoNormalization);
}

static void compare_netlists (tl::TestBase *_this, const std::string &cir, const std::string &cir_au)
{
  db::Netlist nl, nl_au;

  db::NetlistSpiceReader reader;

  {
    tl::info << "Output: " << cir;
    tl::InputStream is (cir);
    reader.read (is, nl);
  }

  {
    tl::info << "Golden: " << cir_au;
    tl::InputStream is (cir_au);
    reader.read (is, nl_au);
  }

  db::compare_netlist (_this, nl, nl_au);
}

TEST(9_NetlistExtraction)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/lvs/lvsSimpleTests_9.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/ringo.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au9a.cir";

  std::string au_simplified = tl::testsrc ();
  au_simplified += "/testdata/lvs/lvsSimpleTests_au9b.cir";

  std::string output = this->tmp_file ("tmp.cir");
  std::string output_simplified = this->tmp_file ("tmp_simplified.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
        "$lvs_test_target_simplified = '%s'\n"
      , input, output, output_simplified)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);


  //  verify

  CHECKPOINT ();
  compare_netlists (_this, output, au);

  CHECKPOINT ();
  compare_netlists (_this, output_simplified, au_simplified);
}

TEST(10_NetlistExtractionFlat)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/lvs/lvsSimpleTests_10.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/ringo.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au10a.cir";

  std::string au_simplified = tl::testsrc ();
  au_simplified += "/testdata/lvs/lvsSimpleTests_au10b.cir";

  std::string output = this->tmp_file ("tmp.cir");
  std::string output_simplified = this->tmp_file ("tmp_simplified.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
        "$lvs_test_target_simplified = '%s'\n"
      , input, output, output_simplified)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);


  //  verify

  CHECKPOINT ();
  compare_netlists (_this, output, au);

  CHECKPOINT ();
  compare_netlists (_this, output_simplified, au_simplified);
}

TEST(11_CustomDevices)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/lvs/lvsSimpleTests_11.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/vdiv.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au11a.cir";

  std::string au_simplified = tl::testsrc ();
  au_simplified += "/testdata/lvs/lvsSimpleTests_au11b.cir";

  std::string output = this->tmp_file ("tmp.cir");
  std::string output_simplified = this->tmp_file ("tmp_simplified.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
        "$lvs_test_target_simplified = '%s'\n"
      , input, output, output_simplified)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);


  //  verify

  CHECKPOINT ();
  compare_netlists (_this, output, au);

  CHECKPOINT ();
  compare_netlists (_this, output_simplified, au_simplified);
}

TEST(12_NetlistJoinLabels)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/lvs/lvsSimpleTests_12.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/implicit_nets.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au12a.cir";

  std::string output = this->tmp_file ("tmp.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
        "$lvs_test_target_simplified = nil\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

  //  verify

  CHECKPOINT ();
  compare_netlists (_this, output, au);
}

TEST(13a_KissingCorners)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/lvs/lvsSimpleTests_13a.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/kissing_corners.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au13a.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

  //  verify

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  CHECKPOINT ();
  db::compare_layouts (_this, layout, au, db::NoNormalization);
}

TEST(13b_KissingCornersDeep)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/lvs/lvsSimpleTests_13b.lvs";

  std::string input = tl::testsrc ();
  input += "/testdata/lvs/kissing_corners.gds";

  std::string au = tl::testsrc ();
  au += "/testdata/lvs/lvsSimpleTests_au13b.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target = '%s'\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

  //  verify

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::Reader reader (stream);
    reader.read (layout);
  }

  CHECKPOINT ();
  db::compare_layouts (_this, layout, au, db::NoNormalization);
}
#endif

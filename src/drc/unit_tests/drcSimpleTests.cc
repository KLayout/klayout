
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
#include "rdb.h"
#include "lymMacro.h"
#include "tlFileUtils.h"

TEST(1)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_1.drc";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au1.gds";

  std::string output = this->tmp_file ("tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
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
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_2.drc";

  std::string input = tl::testdata ();
  input += "/drc/drctest.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au2.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(3_Flat)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_3.drc";

  std::string input = tl::testdata ();
  input += "/drc/drctest.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au3.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(4_Hierarchical)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_4.drc";

  std::string input = tl::testdata ();
  input += "/drc/drctest.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au4.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(5_FlatAntenna)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_5.drc";

  std::string input = tl::testdata ();
  input += "/drc/antenna_l1.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au5.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(5_FlatAntennaIncremental)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_5i.drc";

  std::string input = tl::testdata ();
  input += "/drc/antenna_l1.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au5.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(6_HierarchicalAntenna)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_6.drc";

  std::string input = tl::testdata ();
  input += "/drc/antenna_l1.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au6.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(7_AntennaWithDiodes)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_7.drc";

  std::string input = tl::testdata ();
  input += "/drc/antenna_l1.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au7.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(8_TextsAndPolygons)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_8.drc";

  std::string input = tl::testdata ();
  input += "/drc/texts.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au8.gds";

  std::string output = this->tmp_file ("tmp.gds");

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
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_9.drc";

  std::string input = tl::testdata ();
  input += "/drc/ringo.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au9a.cir";

  std::string au_simplified = tl::testdata ();
  au_simplified += "/drc/drcSimpleTests_au9b.cir";

  std::string output = this->tmp_file ("tmp.cir");
  std::string output_simplified = this->tmp_file ("tmp_simplified.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
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

  CHECKPOINT ();
  compare_netlists (_this, output, au);

  CHECKPOINT ();
  compare_netlists (_this, output_simplified, au_simplified);
}

TEST(10_NetlistExtractionFlat)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_10.drc";

  std::string input = tl::testdata ();
  input += "/drc/ringo.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au10a.cir";

  std::string au_simplified = tl::testdata ();
  au_simplified += "/drc/drcSimpleTests_au10b.cir";

  std::string output = this->tmp_file ("tmp.cir");
  std::string output_simplified = this->tmp_file ("tmp_simplified.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
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

  CHECKPOINT ();
  compare_netlists (_this, output, au);

  CHECKPOINT ();
  compare_netlists (_this, output_simplified, au_simplified);
}

TEST(11_CustomDevices)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_11.drc";

  std::string input = tl::testdata ();
  input += "/drc/vdiv.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au11a.cir";

  std::string au_simplified = tl::testdata ();
  au_simplified += "/drc/drcSimpleTests_au11b.cir";

  std::string output = this->tmp_file ("tmp.cir");
  std::string output_simplified = this->tmp_file ("tmp_simplified.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
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

  CHECKPOINT ();
  compare_netlists (_this, output, au);

  CHECKPOINT ();
  compare_netlists (_this, output_simplified, au_simplified);
}

TEST(12_NetlistJoinLabels)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_12.drc";

  std::string input = tl::testdata ();
  input += "/drc/implicit_nets.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au12a.cir";

  std::string output = this->tmp_file ("tmp.cir");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
        "$drc_test_target_simplified = nil\n"
      , input, output)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro drc;
  drc.load_from (rs);
  EXPECT_EQ (drc.run (), 0);

  //  verify

  CHECKPOINT ();
  compare_netlists (_this, output, au);
}

TEST(13a_KissingCorners)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_13a.drc";

  std::string input = tl::testdata ();
  input += "/drc/kissing_corners.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au13a.gds";

  std::string output = this->tmp_file ("tmp.gds");

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
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_13b.drc";

  std::string input = tl::testdata ();
  input += "/drc/kissing_corners.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au13b.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(14_SwitchingTargets)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_14.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_14.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au14.gds";

  std::string au2 = tl::testdata ();
  au2 += "/drc/drcSimpleTests_au14_2.gds";

  std::string output = this->tmp_file ("tmp.gds");
  std::string output2 = this->tmp_file ("tmp2.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
        "$drc_test_target2 = '%s'\n"
      , input, output, output2)
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

  db::Layout layout2;

  {
    tl::InputStream stream (output2);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  db::compare_layouts (_this, layout2, au2, db::NoNormalization);
}

TEST(14b_SideTargetsAndReports)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_14b.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_14b.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au14b.gds";

  std::string au2 = tl::testdata ();
  au2 += "/drc/drcSimpleTests_au14b_2.gds";

  std::string au_report = tl::testdata ();
  au_report += "/drc/drcSimpleTests_au14b.lyrdb";

  std::string au_report2 = tl::testdata ();
  au_report2 += "/drc/drcSimpleTests_au14b_2.lyrdb";

  std::string output = this->tmp_file ("tmp.gds");
  std::string output2 = this->tmp_file ("tmp2.gds");
  std::string report = this->tmp_file ("tmp.lydrc");
  std::string report2 = this->tmp_file ("tmp2.lydrc");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
        "$drc_test_target2 = '%s'\n"
        "$drc_test_report = '%s'\n"
        "$drc_test_report2 = '%s'\n"
      , input, output, output2, report, report2)
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

  db::Layout layout2;

  {
    tl::InputStream stream (output2);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  db::compare_layouts (_this, layout2, au2, db::NoNormalization);

  compare_text_files (report, au_report);
  compare_text_files (report2, au_report2);
}

TEST(14c_OnlySpecialInputsAndReports)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_14c.drc";

  //  apart from that it's a variant of 14b ...

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_14b.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au14b.gds";

  std::string au2 = tl::testdata ();
  au2 += "/drc/drcSimpleTests_au14b_2.gds";

  std::string au_report = tl::testdata ();
  au_report += "/drc/drcSimpleTests_au14b.lyrdb";

  std::string au_report2 = tl::testdata ();
  au_report2 += "/drc/drcSimpleTests_au14b_2.lyrdb";

  std::string output = this->tmp_file ("tmp.gds");
  std::string output2 = this->tmp_file ("tmp2.gds");
  std::string report = this->tmp_file ("tmp.lydrc");
  std::string report2 = this->tmp_file ("tmp2.lydrc");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
        "$drc_test_target2 = '%s'\n"
        "$drc_test_report = '%s'\n"
        "$drc_test_report2 = '%s'\n"
      , input, output, output2, report, report2)
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

  db::Layout layout2;

  {
    tl::InputStream stream (output2);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  db::compare_layouts (_this, layout2, au2, db::NoNormalization);

  compare_text_files (report, au_report);
  compare_text_files (report2, au_report2);
}

TEST(15_issue548)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_15.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_15.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au15.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

//  Edges::extents isn't deep-enabled
TEST(16_issue570)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_16.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_16.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au16.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

//  Problems with Source#select
TEST(17_issue570)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_17.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_17.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au17.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(18_forget)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_18.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_18.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au18.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(19_shielding)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_19.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_19.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au19.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(20_interact_with_count)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_20.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_20.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au20.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(21_breaking)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_21.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_21.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au21.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(22_opposite_filter)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_22.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_22.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au22.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(23_rect_filter)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_23.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_23.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au23.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

TEST(24_enclosing)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_24.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_24.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au24.gds";

  std::string output = this->tmp_file ("tmp.gds");

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

static void run_test (tl::TestBase *_this, const std::string &number, bool deep, bool oasis = false)
{
  std::string force_gc = "true";

  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_" + number + ".drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_" + number + "." + (oasis ? "oas" : "gds");

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au" + number + std::string (deep ? "d" : "") + "." + (oasis ? "oas" : "gds");

  std::string output = _this->tmp_file (oasis ? "tmp.oas" : "tmp.gds");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = '%s'\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_target = '%s'\n"
        "$drc_test_deep = %s\n"
      , force_gc, input, output, deep ? "true" : "false")
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

TEST(25_spaceWithOptions)
{
  run_test (_this, "25", false);
}

TEST(25d_spaceWithOptions)
{
  run_test (_this, "25", true);
}

TEST(26_attributes)
{
  run_test (_this, "26", false);
}

TEST(26d_attributes)
{
  run_test (_this, "26", true);
}

TEST(27_advancedShielding)
{
  run_test (_this, "27", false);
}

TEST(27d_advancedShielding)
{
  run_test (_this, "27", true);
}

TEST(28_inputFragmentation)
{
  run_test (_this, "28", true);
}

TEST(29_holes)
{
  run_test (_this, "29", false);
}

TEST(29d_holes)
{
  run_test (_this, "29", true);
}

TEST(30_density)
{
  run_test (_this, "30", false);
}

TEST(31_globaTransformation)
{
  run_test (_this, "31", false);
}

TEST(31d_globalTransformation)
{
  run_test (_this, "31", true);
}

TEST(32_globalTransformationWithClip)
{
  run_test (_this, "32", false);
}

TEST(32d_globalTransformationWithClip)
{
  run_test (_this, "32", true);
}

TEST(33_globalTransformationWithTiles)
{
  run_test (_this, "33", true);
}

TEST(40_fill)
{
  run_test (_this, "40", false);
}

TEST(41_fillTiled)
{
  run_test (_this, "41", false);
}

TEST(42_fillWithLeft)
{
  run_test (_this, "42", false);
}

TEST(43_fillWithLeftTiled)
{
  run_test (_this, "43", false);
}

TEST(44_fillWithOverlappingBoxes)
{
  run_test (_this, "44", false);
}

TEST(45_fillWithOverlappingBoxesTiled)
{
  run_test (_this, "45", false);
}

TEST(46_fillWithOverlappingBoxes)
{
  run_test (_this, "46", false);
}

TEST(47_fillWithOverlappingBoxesTiled)
{
  run_test (_this, "47", false);
}

TEST(47b_fillWithUsingOutput)
{
  run_test (_this, "47b", false);
}

TEST(47bd_fillWithUsingOutputDeep)
{
  run_test (_this, "47b", true);
}

TEST(47c_fillWithExcludeArea)
{
  run_test (_this, "47c", false);
}

TEST(47cd_fillWithExcludeAreaDeep)
{
  run_test (_this, "47c", true);
}

TEST(48_drcWithFragments)
{
  run_test (_this, "48", false);
}

TEST(48d_drcWithFragments)
{
  run_test (_this, "48", true);
}

TEST(49_epAngle)
{
  run_test (_this, "49", false);
}

TEST(49d_epAngle)
{
  run_test (_this, "49", true);
}

TEST(50_issue826)
{
  run_test (_this, "50", false, true /*OASIS*/);
}

TEST(51_epInternalAngle)
{
  run_test (_this, "51", false);
}

TEST(52_cellWiseExtent)
{
  run_test (_this, "52", false);
}

TEST(53_cellWiseExtentWithClip)
{
  run_test (_this, "53", false);
}

TEST(54_issue1011)
{
  run_test (_this, "54", false);
}

TEST(55_drccount)
{
  run_test (_this, "55", false);
}

TEST(55d_drccount)
{
  run_test (_this, "55", true);
}

TEST(56_angle_classes)
{
  run_test (_this, "56", false);
}

TEST(56d_angle_classes)
{
  run_test (_this, "56", true);
}

TEST(57_issue_1190)
{
  run_test (_this, "57", false);
}

TEST(57d_issue_1190)
{
  run_test (_this, "57", true);
}

TEST(58_in_and_out)
{
  run_test (_this, "58", false);
}

TEST(58d_in_and_out)
{
  run_test (_this, "58", true);
}

TEST(60_issue1216)
{
  run_test (_this, "60", false);
}

TEST(60d_issue1216)
{
  run_test (_this, "60", true);
}

TEST(61_issue1485)
{
  run_test (_this, "61", false);
}

TEST(61d_issue1485)
{
  run_test (_this, "61", true);
}

TEST(70_props)
{
  run_test (_this, "70", false);
}

TEST(70d_props)
{
  run_test (_this, "70", true);
}

TEST(71_netter)
{
  run_test (_this, "71", false);
}

TEST(71d_netter)
{
  run_test (_this, "71", true);
}

TEST(80_deep_with_mag_width)
{
  run_test (_this, "80", true);
}

TEST(81_deep_with_mag_space)
{
  run_test (_this, "81", true);
}

TEST(82_deep_with_mag_cop_width)
{
  run_test (_this, "82", true);
}

TEST(83_deep_with_mag_cop_space)
{
  run_test (_this, "83", true);
}

TEST(84_deep_with_mag_edge_width)
{
  run_test (_this, "84", true);
}

TEST(85_deep_with_mag_edge_space)
{
  run_test (_this, "85", true);
}

TEST(86_deep_with_mag_size)
{
  run_test (_this, "86", true);
}

TEST(87_deep_with_mag_size_aniso)
{
  run_test (_this, "87", true);
}

TEST(88_deep_with_mag_cop_size)
{
  run_test (_this, "88", true);
}

TEST(89_deep_with_mag_cop_size_aniso)
{
  run_test (_this, "89", true);
}

TEST(90_zero_distance_mode)
{
  run_test (_this, "90", false);
}

TEST(90d_zero_distance_mode)
{
  run_test (_this, "90", true);
}

TEST(91_zero_distance_mode)
{
  run_test (_this, "91", false);
}

TEST(91d_zero_distance_mode)
{
  run_test (_this, "91", true);
}

TEST(92_issue1594_dual_top)
{
  std::string rs = tl::testdata ();
  rs += "/drc/issue_1594.drc";

  std::string input = tl::testdata ();
  input += "/drc/issue_1594.gds";

  std::string au = tl::testdata ();
  au += "/drc/issue_1594_au.cir";

  std::string output = this->tmp_file ("tmp.cir");

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

  //  verify

  CHECKPOINT ();
  compare_netlists (_this, output, au);
}

TEST(93_withAngle)
{
  run_test (_this, "93", false);
}

TEST(93d_withAngle)
{
  run_test (_this, "93", true);
}

TEST(94_texts_in_region_xor)
{
  run_test (_this, "94", false);
}

TEST(94d_texts_in_region_xor)
{
  run_test (_this, "94", true);
}

TEST(100_edge_interaction_with_count)
{
  run_test (_this, "100", false);
}

TEST(100d_edge_interaction_with_count)
{
  run_test (_this, "100", true);
}

TEST(101_edge_booleans_with_dots)
{
  run_test (_this, "101", false);
}

TEST(101d_edge_booleans_with_dots)
{
  run_test (_this, "101", true);
}

TEST(102_edge_modes)
{
  run_test (_this, "102", false);
}

TEST(102d_edge_modes)
{
  run_test (_this, "102", true);
}

TEST(110_RDBVariantAssignment)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_110.drc";

  //  apart from that it's a variant of 14b ...

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_110.gds";

  std::string au_report = tl::testdata ();
  au_report += "/drc/drcSimpleTests_au110.lyrdb";

  std::string report = this->tmp_file ("tmp.lydrc");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_report = '%s'\n"
      , input, report)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro drc;
  drc.load_from (rs);
  EXPECT_EQ (drc.run (), 0);

  compare_text_files (report, au_report);
}

TEST(111_RDBCategoryHierarchy)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_111.drc";

  //  apart from that it's a variant of 14b ...

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_111.gds";

  std::string au_report = tl::testdata ();
  au_report += "/drc/drcSimpleTests_au111.lyrdb";

  std::string report = this->tmp_file ("tmp.lydrc");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_report = '%s'\n"
      , input, report)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro drc;
  drc.load_from (rs);
  EXPECT_EQ (drc.run (), 0);

  compare_text_files (report, au_report);
}

TEST(112_Waiving)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_112.drc";

  //  apart from that it's a variant of 14b ...

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_112.gds";

  std::string au_report = tl::testdata ();
  au_report += "/drc/drcSimpleTests_au112.lyrdb";

  std::string report = this->tmp_file ("tmp.lydrc");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$drc_test_source = '%s'\n"
        "$drc_test_report = '%s'\n"
      , input, report)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  //  prepare a waiver db
  {
    std::string report_w = this->tmp_file ("tmp.lydrc.w");
    rdb::Database rdb_w;
    rdb_w.load (au_report + ".w");
    rdb_w.write (report_w);
  }

  lym::Macro drc;
  drc.load_from (rs);
  EXPECT_EQ (drc.run (), 0);

  compare_text_files (report, au_report);
}


TEST(120_ShapesOfPin)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_120.drc";

  //  apart from that it's a variant of 14b ...

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_120.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au120.gds";

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

TEST(121_ShapesOfTerminal)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_121.drc";

  //  apart from that it's a variant of 14b ...

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_121.gds";

  std::string au = tl::testdata ();
  au += "/drc/drcSimpleTests_au121.gds";

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

TEST(122_NamedLayers)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_122.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_122.gds";

  std::string au_output = tl::testdata ();
  au_output += "/drc/drcSimpleTests_au122.l2n";

  std::string output = this->tmp_file ("tmp.l2n");

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

  compare_text_files (output, au_output);
}

TEST(123_DirectInsert)
{
  run_test (_this, "123", false);
}

TEST(130_size_inside_outside)
{
  run_test (_this, "130", false);
}

TEST(130d_size_inside_outside)
{
  run_test (_this, "130", true);
}

TEST(131_edge_pair_interactions)
{
  run_test (_this, "131", false);
}

TEST(131d_edge_pair_interactions)
{
  run_test (_this, "131", true);
}

TEST(132d_sensitive_breaking)
{
  run_test (_this, "132", true);
}

TEST(140_target_modification)
{
  run_test (_this, "140", false);
}

TEST(140d_target_modification)
{
  run_test (_this, "140", true);
}

TEST(141_merge_properties)
{
  run_test (_this, "141", false);
}

TEST(141d_merge_properties)
{
  run_test (_this, "141", true);
}

TEST(142_evaluate_nets)
{
  run_test (_this, "142", false);
}

TEST(142d_evaluate_nets)
{
  run_test (_this, "142", true);
}

TEST(143_evaluate_and_filter)
{
  run_test (_this, "143", false);
}

TEST(143d_evaluate_and_filter)
{
  run_test (_this, "143", true);
}

TEST(144_combined_antennas)
{
  run_test (_this, "144", false);
}

TEST(144d_combined_antennas)
{
  run_test (_this, "144", true);
}

//  issue 2134
TEST(145_edges_merge)
{
  run_test (_this, "145", false);
}

TEST(145d_edges_merge)
{
  run_test (_this, "145", true);
}

//  issue 2141
TEST(146_edges_and_corners)
{
  run_test (_this, "146", false);
}

TEST(146d_edges_and_corners)
{
  run_test (_this, "146", true);
}

TEST(147_MeasureNetsWithL2N)
{
  std::string rs = tl::testdata ();
  rs += "/drc/drcSimpleTests_147.drc";

  std::string input = tl::testdata ();
  input += "/drc/drcSimpleTests_147.gds";

  std::string au_output = tl::testdata ();
  au_output += "/drc/drcSimpleTests_au147.l2n";

  std::string output = this->tmp_file ("tmp.l2n");

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

  compare_text_files (output, au_output);
}


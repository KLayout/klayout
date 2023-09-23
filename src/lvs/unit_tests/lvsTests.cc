
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

#include "tlUnitTest.h"
#include "dbReader.h"
#include "dbTestSupport.h"
#include "dbNetlist.h"
#include "dbNetlistSpiceReader.h"
#include "dbNetlistCompare.h"
#include "dbNetlistCrossReference.h"
#include "lymMacro.h"
#include "tlFileUtils.h"

void run_test (tl::TestBase *_this, const std::string &lvs_rs, const std::string &au_netlist, const std::string &layout, bool priv = false, const std::string &au_lvsdb_name = std::string (), const std::string &added = std::string ())
{
  std::string testsrc = priv ? tl::testdata_private () : tl::testdata ();
  testsrc = tl::combine_path (testsrc, "lvs");

  std::string rs = tl::combine_path (testsrc, lvs_rs);
  std::string ly = tl::combine_path (testsrc, layout);
  std::string au_cir = tl::combine_path (testsrc, au_netlist);

  std::string output_lvsdb = _this->tmp_file ("tmp.lvsdb");
  std::string output_cir = _this->tmp_file ("tmp.cir");
  std::string output_l2n = _this->tmp_file ("tmp.l2n");

  {
    //  Set some variables
    lym::Macro config;
    config.set_text (tl::sprintf (
        "$drc_force_gc = true\n"
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target_lvsdb = '%s'\n"
        "$lvs_test_target_cir = '%s'\n"
        "$lvs_test_target_l2n = '%s'\n"
        "%s"
      , ly, output_lvsdb, output_cir, output_l2n, added)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

  db::Netlist nl1, nl2;

  {
    db::NetlistSpiceReader reader;
    tl::InputStream stream (output_cir);
    reader.read (stream, nl1);
  }

  {
    db::NetlistSpiceReader reader;
    tl::InputStream stream (au_cir);
    reader.read (stream, nl2);
  }

  db::compare_netlist (_this, nl1, nl2);

  if (! au_lvsdb_name.empty ()) {
    std::string au_lvsdb = tl::combine_path (testsrc, au_lvsdb_name);
    _this->compare_text_files (output_lvsdb, au_lvsdb);
  }
}

TEST(1_full)
{
  test_is_long_runner ();
  run_test (_this, "vexriscv.lvs", "vexriscv.cir.gz", "vexriscv.oas.gz");
}

TEST(2_fullWithAlign)
{
  test_is_long_runner ();
  run_test (_this, "vexriscv_align.lvs", "vexriscv.cir.gz", "vexriscv.oas.gz");
}

TEST(10_private)
{
  // test_is_long_runner ();
  run_test (_this, "test_10.lvs", "test_10.cir.gz", "test_10.gds.gz", true);
}

TEST(11_private)
{
  // test_is_long_runner ();
  run_test (_this, "test_11.lvs", "test_11.cir.gz", "test_11.gds.gz", true);
}

TEST(12_private)
{
  // test_is_long_runner ();
  run_test (_this, "test_12.lvs", "test_12b.cir.gz", "test_12.gds.gz", true);
}

TEST(13_private)
{
  // test_is_long_runner ();
  run_test (_this, "test_13.lvs", "test_13b.cir.gz", "test_13.gds.gz", true);
}

TEST(14_private)
{
  test_is_long_runner ();
  run_test (_this, "test_14.lvs", "test_14.cir.gz", "test_14.gds.gz", true);
}

TEST(15_private)
{
  // test_is_long_runner ();
  run_test (_this, "test_15.lvs", "test_15.cir.gz", "test_15.gds.gz", true);
}

TEST(16_private)
{
  // test_is_long_runner ();lvs-blackbox
  run_test (_this, "test_16.lvs", "test_16.cir.gz", "test_16.gds.gz", true);
}

TEST(17_private)
{
  test_is_long_runner ();
  run_test (_this, "test_17.lylvs", "test_17b.cir.gz", "test_17.gds.gz", true, "test_17b_4.lvsdb");
}

TEST(18_private)
{
  // test_is_long_runner ();
  run_test (_this, "test_18.lvs", "test_18.cir.gz", "test_18.gds.gz", true);
}

TEST(19_private)
{
  // test_is_long_runner ();
  run_test (_this, "test_19.lvs", "test_19.cir.gz", "test_19.gds.gz", true);
}

TEST(20_private)
{
  // test_is_long_runner ();
  run_test (_this, "test_20.lylvs", "test_20.cir.gz", "test_20.gds.gz", true, "test_20_4.lvsdb");
}

TEST(21_private)
{
  run_test (_this, "test_21.lylvs", "test_21.cir.gz", "test_21.gds.gz", true, "test_21_3.lvsdb");
}

//  issue #1021
TEST(22a_SP6TArray2X4)
{
  run_test (_this, "SP6TArray_2X4.lvs", "test_22a.cir", "SP6TArray_2X4.gds", false, "test_22a.lvsdb", "$test22_texts = false\n$test22_deep = false");
}
TEST(22b_SP6TArray2X4)
{
  run_test (_this, "SP6TArray_2X4.lvs", "test_22b.cir", "SP6TArray_2X4.gds", false, "test_22b.lvsdb", "$test22_texts = true\n$test22_deep = false");
}
TEST(22c_SP6TArray2X4)
{
  run_test (_this, "SP6TArray_2X4.lvs", "test_22c.cir", "SP6TArray_2X4.gds", false, "test_22c.lvsdb", "$test22_texts = false\n$test22_deep = true");
}
TEST(22d_SP6TArray2X4)
{
  run_test (_this, "SP6TArray_2X4.lvs", "test_22d.cir", "SP6TArray_2X4.gds", false, "test_22d.lvsdb", "$test22_texts = true\n$test22_deep = true");
}

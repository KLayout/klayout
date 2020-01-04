
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

void run_test (tl::TestBase *_this, const std::string &lvs_rs, const std::string &au_netlist, const std::string &layout, bool priv = false)
{
  std::string testsrc = priv ? tl::testsrc_private () : tl::testsrc ();
  testsrc = tl::combine_path (tl::combine_path (testsrc, "testdata"), "lvs");

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
        "$lvs_test_source = '%s'\n"
        "$lvs_test_target_lvsdb = '%s'\n"
        "$lvs_test_target_cir = '%s'\n"
        "$lvs_test_target_l2n = '%s'\n"
      , ly, output_lvsdb, output_cir, output_l2n)
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

  //  NOTE: it's kind of redundant to use the comparer for checking the LVS
  //  output, but this will essentially verify the output netlist's consistency.
  db::NetlistCrossReference xref;
  db::NetlistComparer comparer (&xref);
  comparer.set_max_branch_complexity (500);
  comparer.set_max_depth (20);
  bool res = comparer.compare (&nl1, &nl2);
  if (! res) {
    tl::info << "Netlist mismatch:";
    tl::info << "  current: " << output_cir;
    tl::info << "  golden: " << au_cir;
  }
  EXPECT_EQ (res, true);
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
  run_test (_this, "test_12.lvs", "test_12.cir.gz", "test_12.gds.gz", true);
}

TEST(13_private)
{
  // test_is_long_runner ();
  run_test (_this, "test_13.lvs", "test_13.cir.gz", "test_13.gds.gz", true);
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
  // test_is_long_runner ();
  run_test (_this, "test_16.lvs", "test_16.cir.gz", "test_16.gds.gz", true);
}


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
#include "dbNetlistCompare.h"
#include "dbNetlistCrossReference.h"
#include "lymMacro.h"
#include "tlFileUtils.h"

void run_test (tl::TestBase *_this, const std::string &lvs_rs, const std::string &suffix, const std::string &layout)
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/lvs/" + lvs_rs;

  std::string src = tl::testsrc ();
  src += "/testdata/lvs/" + layout;

  std::string au_lvsdb = tl::testsrc ();
  au_lvsdb += "/testdata/lvs/" + suffix + ".lvsdb.gz";

  std::string au_cir = tl::testsrc ();
  au_cir += "/testdata/lvs/" + suffix + ".cir.gz";

  std::string au_l2n = tl::testsrc ();
  au_l2n += "/testdata/lvs/" + suffix + ".l2n.gz";

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
      , src, output_lvsdb, output_cir, output_l2n)
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
  run_test (_this, "vexriscv.lvs", "vexriscv", "vexriscv.oas.gz");
}

TEST(2_fullWithAlign)
{
  test_is_long_runner ();
  run_test (_this, "vexriscv_align.lvs", "vexriscv", "vexriscv.oas.gz");
}

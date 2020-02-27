
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
#include "lymMacro.h"
#include "tlFileUtils.h"

void run_test (tl::TestBase *_this, const std::string &suffix, const std::string &layout, bool with_l2n = false, const std::string &top = std::string ())
{
  std::string rs = tl::testsrc ();
  rs += "/testdata/lvs/" + suffix + ".lvs";

  std::string src = tl::testsrc ();
  src += "/testdata/lvs/" + layout;

  std::string au_lvsdb = tl::testsrc ();
  au_lvsdb += "/testdata/lvs/" + suffix + ".lvsdb";

  std::string au_cir = tl::testsrc ();
  au_cir += "/testdata/lvs/" + suffix + ".cir";

  std::string au_l2n = tl::testsrc ();
  au_l2n += "/testdata/lvs/" + suffix + ".l2n";

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
        "$lvs_test_top = '%s'\n"
      , src, output_lvsdb, output_cir, output_l2n, top)
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

  _this->compare_text_files (output_lvsdb, au_lvsdb);
  _this->compare_text_files (output_cir, au_cir);
  if (with_l2n) {
    _this->compare_text_files (output_l2n, au_l2n);
  }
}

TEST(1_simple)
{
  run_test (_this, "ringo_simple", "ringo.gds");
}

TEST(1b_simple_with_tolerance)
{
  run_test (_this, "ringo_simple_with_tol", "ringo.gds");
}

TEST(2_simple_io)
{
  run_test (_this, "ringo_simple_io", "ringo.gds");
}

TEST(3_simple_io2)
{
  run_test (_this, "ringo_simple_io2", "ringo.gds", true);
}

TEST(4_simple_implicit_connections)
{
  run_test (_this, "ringo_simple_implicit_connections", "ringo_implicit_connections.gds");
}

TEST(5_simple_same_device_classes)
{
  run_test (_this, "ringo_simple_same_device_classes", "ringo.gds");
}

TEST(6_simple_pin_swapping)
{
  run_test (_this, "ringo_simple_pin_swapping", "ringo.gds");
}

TEST(7_net_and_circuit_equivalence)
{
  run_test (_this, "ringo_simple_net_and_circuit_equivalence", "ringo_renamed.gds");
}

TEST(8_simplification)
{
  run_test (_this, "ringo_simple_simplification", "ringo_for_simplification.gds");
}

TEST(9_blackboxing)
{
  run_test (_this, "ringo_simple_blackboxing", "ringo_for_blackboxing.gds");
}

TEST(10_simplification_with_align)
{
  run_test (_this, "ringo_simple_simplification_with_align", "ringo_for_simplification.gds");
}

TEST(11_device_scaling)
{
  run_test (_this, "ringo_simple_device_scaling", "ringo.gds");
}

TEST(12_simple_dmos)
{
  run_test (_this, "ringo_simple_dmos", "ringo.gds");
}

TEST(13_simple_ringo_device_subcircuits)
{
  run_test (_this, "ringo_device_subcircuits", "ringo.gds");
}

TEST(14_simple_ringo_mixed_hierarchy)
{
  run_test (_this, "ringo_mixed_hierarchy", "ringo_mixed_hierarchy.gds");
}

TEST(15_simple_dummy_device)
{
  run_test (_this, "ringo_simple_dummy_device", "ringo_dummy_device.gds");
}

TEST(16_floating)
{
  run_test (_this, "floating", "floating.gds", false, "TOP");
}

TEST(17_layout_variants)
{
  run_test (_this, "ringo_layout_var", "ringo_layout_var.gds");
}

TEST(18_cheats)
{
  run_test (_this, "invchain_cheat", "invchain_for_cheat.gds");
}

//  testing cell specific net joining for VSS of the double-height inverter standard cell
TEST(19_double_height_inv)
{
  run_test (_this, "double_height", "double_height_inv.gds");
}

//  testing cell specific net joining for VSS of the double-height inverter standard cell
TEST(20_double_height2_inv)
{
  run_test (_this, "double_height2", "double_height2_inv.gds");
}

TEST(21_split_gate)
{
  run_test (_this, "nand2_split_gate", "nand2_split_gate.oas");
}


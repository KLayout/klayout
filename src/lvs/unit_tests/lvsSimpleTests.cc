
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
#include "dbNetlist.h"
#include "dbNetlistSpiceReader.h"
#include "lymMacro.h"
#include "tlFileUtils.h"

void run_test (tl::TestBase *_this, const std::string &suffix, const std::string &layout, bool with_l2n = false, bool with_lvs = true, const std::string &top = std::string (), bool change_case = false)
{
  std::string rs = tl::testdata ();
  rs += "/lvs/" + suffix + ".lvs";

  std::string src = tl::testdata ();
  src += "/lvs/" + layout;

  std::string au_lvsdb = tl::testdata ();
  au_lvsdb += "/lvs/" + suffix + ".lvsdb";

  std::string au_cir = tl::testdata ();
  au_cir += "/lvs/" + suffix + ".cir";

  std::string au_l2n = tl::testdata ();
  au_l2n += "/lvs/" + suffix + ".l2n";

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
        "$lvs_test_top = '%s'\n"
        "$change_case = %s\n"
      , src, output_lvsdb, output_cir, output_l2n, top, change_case ? "true" : "false")
    );
    config.set_interpreter (lym::Macro::Ruby);
    EXPECT_EQ (config.run (), 0);
  }

  lym::Macro lvs;
  lvs.load_from (rs);
  EXPECT_EQ (lvs.run (), 0);

  if (with_lvs) {
    _this->compare_text_files (output_lvsdb, au_lvsdb);
  }
  _this->compare_text_files (output_cir, au_cir);
  if (with_l2n) {
    _this->compare_text_files (output_l2n, au_l2n);
  }
}

TEST(1_simple)
{
  run_test (_this, "ringo_simple", "ringo.gds");
}

TEST(1a_simple_double_compare)
{
  run_test (_this, "ringo_simple_compare2", "ringo.gds");
}

TEST(1b_simple_with_tolerance)
{
  run_test (_this, "ringo_simple_with_tol", "ringo.gds");
}

TEST(1c_simple_with_tolerance_early)
{
  run_test (_this, "ringo_simple_with_tol_early", "ringo.gds");
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
  //  change case
  run_test (_this, "ringo_simple_pin_swapping", "ringo.gds", false, true, std::string (), true);
}

TEST(7_net_and_circuit_equivalence)
{
  run_test (_this, "ringo_simple_net_and_circuit_equivalence", "ringo_renamed.gds");
  //  change case
  run_test (_this, "ringo_simple_net_and_circuit_equivalence", "ringo_renamed.gds", false, true, std::string (), true);
}

TEST(8_simplification)
{
  run_test (_this, "ringo_simple_simplification", "ringo_for_simplification.gds");
}

TEST(9_blackboxing)
{
  run_test (_this, "ringo_simple_blackboxing", "ringo_for_blackboxing.gds");
}

TEST(9b_blackboxing_netter)
{
  run_test (_this, "ringo_simple_blackboxing_netter", "ringo_for_blackboxing.gds");
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
  run_test (_this, "ringo_simple_dmos_fixed", "ringo_fixed_sources.gds");
}

TEST(13_simple_ringo_device_subcircuits)
{
  run_test (_this, "ringo_device_subcircuits", "ringo.gds");
  //  change case
  run_test (_this, "ringo_device_subcircuits", "ringo.gds", false, true, std::string (), true);
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
  run_test (_this, "floating", "floating.gds", false, true, "TOP");
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

//  testing cell specific net joining for VSS of the double-height inverter standard cell
TEST(21_double_height2_inv_texts)
{
  run_test (_this, "double_height2_texts", "double_height2_inv.gds");
}

TEST(22_split_gate)
{
  run_test (_this, "nand2_split_gate", "nand2_split_gate.oas");
}

TEST(22b_split_gate_early)
{
  run_test (_this, "nand2_split_gate_early", "nand2_split_gate.oas");
}

//  empty gds
TEST(23_issue709)
{
  run_test (_this, "empty_subcells", "empty_subcells.gds");
}

TEST(24_issue806)
{
  run_test (_this, "custom_compare", "custom_compare.gds");
}

TEST(25_blackbox)
{
  run_test (_this, "blackbox1", "blackbox.gds");
  run_test (_this, "blackbox2", "blackbox_swapped.gds");
  run_test (_this, "blackbox3", "blackbox_open.gds");
  run_test (_this, "blackbox4", "blackbox_short.gds");
  run_test (_this, "blackbox5", "blackbox_short_and_open.gds");
}

TEST(26_enableWandL)
{
  run_test (_this, "enable_wl1", "resistor.gds");
  run_test (_this, "enable_wl2", "resistor.gds");
  run_test (_this, "enable_wl3", "resistor.gds");
}

TEST(27_BlackBoxDevicesWithAlign)
{
  run_test (_this, "bbdevices1", "bbdevices1.gds");
  run_test (_this, "bbdevices2", "bbdevices2.gds");
  run_test (_this, "bbdevices3", "bbdevices3.gds");
  run_test (_this, "bbdevices4", "bbdevices4.gds");
  run_test (_this, "bbdevices5", "bbdevices5.gds");
  run_test (_this, "bbdevices6", "bbdevices6.gds");
}

TEST(28_BlackBoxDevicesWithBlank)
{
  run_test (_this, "bbdevices1b", "bbdevices1.gds");
  run_test (_this, "bbdevices2b", "bbdevices2.gds");
  run_test (_this, "bbdevices3b", "bbdevices3.gds");
  run_test (_this, "bbdevices4b", "bbdevices4.gds");
  run_test (_this, "bbdevices5b", "bbdevices5.gds");
  run_test (_this, "bbdevices6b", "bbdevices6.gds");
}

TEST(29_DeviceCombineAndTolerances)
{
  run_test (_this, "res_combine1", "res_combine.gds");
  run_test (_this, "res_combine2", "res_combine.gds");
  run_test (_this, "res_combine3", "res_combine.gds");
}

TEST(30_MustConnect1)
{
  run_test (_this, "must_connect1", "must_connect1.gds");
  run_test (_this, "must_connect1_tl", "must_connect1.gds");
}

TEST(31_MustConnect2)
{
  run_test (_this, "must_connect2", "must_connect2.gds");
}

//  Intermediate cell propagates must-connect pins
TEST(32_MustConnect3)
{
  run_test (_this, "must_connect3", "must_connect3.gds");
}

//  issue 1609
TEST(40_DeviceExtractorErrors)
{
  run_test (_this, "custom_resistors", "custom_resistors.gds", true, false /*no LVS*/);
}

//  Basic soft connection
TEST(50_BasicSoftConnection)
{
  run_test (_this, "soft_connect1", "soft_connect1.gds", true, false /*no LVS*/);
  //  issue #1691
  run_test (_this, "soft_connect1a", "soft_connect1.gds", true, false /*no LVS*/);
}

//  No errors
TEST(51_SoftConnectionNoErrors)
{
  run_test (_this, "soft_connect2", "soft_connect2.gds", true, false /*no LVS*/);
}

//  Simple hierarchy
TEST(52_SoftConnectionSimpleHierarchy)
{
  run_test (_this, "soft_connect3", "soft_connect3.gds", true, false /*no LVS*/);
}

//  Soft connected nets from different subcircuits
TEST(53_SoftConnectionFromSubcircuits)
{
  run_test (_this, "soft_connect4", "soft_connect4.gds", true, false /*no LVS*/);
}

//  Soft connected nets from different subcircuits (propagated)
TEST(54_SoftConnectionFromSubcircuits2)
{
  run_test (_this, "soft_connect5", "soft_connect5.gds", true, false /*no LVS*/);
}

//  Level 2 soft connection
TEST(55_SoftConnectionSecondLevel)
{
  run_test (_this, "soft_connect6", "soft_connect6.gds", true, false /*no LVS*/);
}

//  Issue #1719, part 2 (ignore stray texts)
TEST(60_StrayTextsDoNotMakeNets)
{
  run_test (_this, "stray_texts1", "stray_texts.gds", true, false /*no LVS*/);
}

//  Issue #1719, part 2 (ignore stray texts)
TEST(61_StrayTextsDoNotMakeNets)
{
  run_test (_this, "stray_texts2", "stray_texts.gds", true, false /*no LVS*/);
}

//  Issue #1719, part 3 (layer naming)
TEST(62_LayerNames)
{
  run_test (_this, "layer_names", "layer_names.gds", false, true, "TOP");
}

TEST(63_FlagMissingPorts)
{
  run_test (_this, "flag_missing_ports", "flag_missing_ports.gds", false, true, "TOP");
}


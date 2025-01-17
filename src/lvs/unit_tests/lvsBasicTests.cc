
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

TEST(1)
{
  std::string input = tl::testdata ();
  input += "/lvs/inv.oas";
  std::string schematic = "inv.cir";   //  relative to inv.oas
  std::string au_cir = tl::testdata ();
  au_cir += "/lvs/inv_layout.cir";
  std::string au_lvsdb = tl::testdata ();
  au_lvsdb += "/lvs/inv.lvsdb";

  std::string output_cir = this->tmp_file ("tmp.cir");
  std::string output_lvsdb = this->tmp_file ("tmp.lvsdb");

  lym::Macro lvs;
  lvs.set_text (tl::sprintf (
      "$drc_force_gc = true\n"
      "\n"
      "source('%s', 'INVERTER')\n"
      "\n"
      "deep\n"
      "\n"
      "# Reports generated\n"
      "\n"
      "# LVS report to inv.lvsdb\n"
      "report_lvs('%s', true)\n"
      "\n"
      "# Write extracted netlist to inv_extracted.cir\n"
      "target_netlist('%s', write_spice, 'Extracted by KLayout')\n"
      "\n"
      "schematic('%s')\n"
      "\n"
      "# Drawing layers\n"
      "\n"
      "nwell       = input(1, 0)\n"
      "active      = input(2, 0)\n"
      "pplus       = input(3, 0)\n"
      "nplus       = input(4, 0)\n"
      "poly        = input(5, 0)\n"
      "contact     = input(6, 0)\n"
      "metal1      = input(7, 0)\n"
      "metal1_lbl  = labels(7, 1)\n"
      "via1        = input(8, 0)\n"
      "metal2      = input(9, 0)\n"
      "metal2_lbl  = labels(9, 1)\n"
      "\n"
      "# Bulk layer for terminal provisioning\n"
      "\n"
      "bulk        = polygon_layer\n"
      "\n"
      "# Computed layers\n"
      "\n"
      "active_in_nwell       = active & nwell\n"
      "pactive               = active_in_nwell & pplus\n"
      "pgate                 = pactive & poly\n"
      "psd                   = pactive - pgate\n"
      "\n"
      "active_outside_nwell  = active - nwell\n"
      "nactive               = active_outside_nwell & nplus\n"
      "ngate                 = nactive & poly\n"
      "nsd                   = nactive - ngate\n"
      "\n"
      "# Device extraction\n"
      "\n"
      "# PMOS transistor device extraction\n"
      "extract_devices(mos4('PMOS'), { 'SD' => psd, 'G' => pgate, 'W' => nwell, \n"
      "                                'tS' => psd, 'tD' => psd, 'tG' => poly, 'tW' => nwell })\n"
      "\n"
      "# NMOS transistor device extraction\n"
      "extract_devices(mos4('NMOS'), { 'SD' => nsd, 'G' => ngate, 'W' => bulk, \n"
      "                                'tS' => nsd, 'tD' => nsd, 'tG' => poly, 'tW' => bulk })\n"
      "\n"
      "# Define connectivity for netlist extraction\n"
      "\n"
      "# Inter-layer\n"
      "connect(psd,        contact)\n"
      "connect(nsd,        contact)\n"
      "connect(poly,       contact)\n"
      "connect(contact,    metal1)\n"
      "connect(metal1,     metal1_lbl)   # attaches labels\n"
      "connect(metal1,     via1)\n"
      "connect(via1,       metal2)\n"
      "connect(metal2,     metal2_lbl)   # attaches labels\n"
      "\n"
      "# Global\n"
      "connect_global(bulk,  'SUBSTRATE')\n"
      "connect_global(nwell, 'NWELL')\n"
      "\n"
      "# Compare section\n"
      "\n"
      "compare\n"
    , input, output_lvsdb, output_cir, schematic)
  );
  lvs.set_interpreter (lym::Macro::DSLInterpreter);
  lvs.set_dsl_interpreter ("lvs-dsl");

  EXPECT_EQ (lvs.run (), 0);

  compare_text_files (output_cir, au_cir);
  compare_text_files (output_lvsdb, au_lvsdb);
}

TEST(2)
{
  std::string input = tl::testdata ();
  input += "/lvs/inv2.oas";
  std::string schematic = "inv2.cir";   //  relative to inv2.oas
  std::string au_cir = tl::testdata ();
  au_cir += "/lvs/inv2_layout.cir";
  std::string au_lvsdb = tl::testdata ();
  au_lvsdb += "/lvs/inv2.lvsdb";

  std::string output_cir = this->tmp_file ("tmp.cir");
  std::string output_lvsdb = this->tmp_file ("tmp.lvsdb");

  lym::Macro lvs;
  lvs.set_text (tl::sprintf (
      "source('%s', 'INVERTER_WITH_DIODES')\n"
      "\n"
      "deep\n"
      "\n"
      "# Reports generated\n"
      "\n"
      "# LVS report to inv.lvsdb\n"
      "report_lvs('%s')\n"
      "\n"
      "# Write extracted netlist to inv_extracted.cir\n"
      "target_netlist('%s', write_spice, 'Extracted by KLayout')\n"
      "\n"
      "# Drawing layers\n"
      "\n"
      "nwell       = input(1, 0)\n"
      "active      = input(2, 0)\n"
      "pplus       = input(3, 0)\n"
      "nplus       = input(4, 0)\n"
      "poly        = input(5, 0)\n"
      "contact     = input(6, 0)\n"
      "metal1      = input(7, 0)\n"
      "metal1_lbl  = labels(7, 1)\n"
      "via1        = input(8, 0)\n"
      "metal2      = input(9, 0)\n"
      "metal2_lbl  = labels(9, 1)\n"
      "\n"
      "# Bulk layer for terminal provisioning\n"
      "\n"
      "bulk        = polygon_layer\n"
      "\n"
      "# Computed layers\n"
      "\n"
      "active_in_nwell       = active & nwell\n"
      "pactive               = active_in_nwell & pplus\n"
      "pgate                 = pactive & poly\n"
      "psd                   = pactive - pgate\n"
      "ntie                  = active_in_nwell & nplus\n"
      "\n"
      "active_outside_nwell  = active - nwell\n"
      "nactive               = active_outside_nwell & nplus\n"
      "ngate                 = nactive & poly\n"
      "nsd                   = nactive - ngate\n"
      "ptie                  = active_outside_nwell & pplus\n"
      "\n"
      "# Device extraction\n"
      "\n"
      "# PMOS transistor device extraction\n"
      "extract_devices(mos4('PMOS'), { 'SD' => psd, 'G' => pgate, 'W' => nwell, \n"
      "                                'tS' => psd, 'tD' => psd, 'tG' => poly, 'tW' => nwell })\n"
      "\n"
      "# NMOS transistor device extraction\n"
      "extract_devices(mos4('NMOS'), { 'SD' => nsd, 'G' => ngate, 'W' => bulk, \n"
      "                                'tS' => nsd, 'tD' => nsd, 'tG' => poly, 'tW' => bulk })\n"
      "\n"
      "# Define connectivity for netlist extraction\n"
      "\n"
      "# Inter-layer\n"
      "connect(psd,        contact)\n"
      "connect(nsd,        contact)\n"
      "connect(poly,       contact)\n"
      "connect(ntie,       contact)\n"
      "connect(nwell,      ntie)\n"
      "connect(ptie,       contact)\n"
      "connect(contact,    metal1)\n"
      "connect(metal1,     metal1_lbl)   # attaches labels\n"
      "connect(metal1,     via1)\n"
      "connect(via1,       metal2)\n"
      "connect(metal2,     metal2_lbl)   # attaches labels\n"
      "\n"
      "# Global\n"
      "connect_global(bulk,  'SUBSTRATE')\n"
      "connect_global(ptie,  'SUBSTRATE')\n"
      "\n"
      "# Compare section\n"
      "\n"
      "schematic('%s')\n"
      "\n"
      "compare\n"
    , input, output_lvsdb, output_cir, schematic)
  );
  lvs.set_interpreter (lym::Macro::DSLInterpreter);
  lvs.set_dsl_interpreter ("lvs-dsl");

  EXPECT_EQ (lvs.run (), 0);

  compare_text_files (output_cir, au_cir);
  compare_text_files (output_lvsdb, au_lvsdb);
}

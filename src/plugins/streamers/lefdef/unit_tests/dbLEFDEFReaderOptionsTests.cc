
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

#include "dbLEFDEFImporter.h"

#include "tlUnitTest.h"

#include <cstdlib>

TEST(1)
{
  db::LEFDEFReaderOptions options;
  EXPECT_EQ (options.max_mask_number (), (unsigned int) 0);

  options.set_dbu (1.5);
  EXPECT_EQ (options.dbu (), 1.5);

  options.set_produce_net_names (true);
  EXPECT_EQ (options.produce_net_names (), true);
  options.set_produce_net_names (false);
  EXPECT_EQ (options.produce_net_names (), false);

  options.set_net_property_name (tl::Variant (12));
  EXPECT_EQ (options.net_property_name ().to_string (), "12");

  options.set_produce_inst_names (true);
  EXPECT_EQ (options.produce_inst_names (), true);
  options.set_produce_inst_names (false);
  EXPECT_EQ (options.produce_inst_names (), false);

  options.set_inst_property_name (tl::Variant (12));
  EXPECT_EQ (options.inst_property_name ().to_string (), "12");

  options.set_produce_pin_names (true);
  EXPECT_EQ (options.produce_pin_names (), true);
  options.set_produce_pin_names (false);
  EXPECT_EQ (options.produce_pin_names (), false);

  options.set_pin_property_name (tl::Variant (12));
  EXPECT_EQ (options.pin_property_name ().to_string (), "12");

  options.set_produce_cell_outlines (true);
  EXPECT_EQ (options.produce_cell_outlines (), true);
  options.set_produce_cell_outlines (false);
  EXPECT_EQ (options.produce_cell_outlines (), false);

  options.set_cell_outline_layer ("OL");
  EXPECT_EQ (options.cell_outline_layer (), "OL");

  options.set_produce_placement_blockages (true);
  EXPECT_EQ (options.produce_placement_blockages (), true);
  options.set_produce_placement_blockages (false);
  EXPECT_EQ (options.produce_placement_blockages (), false);

  options.set_placement_blockage_layer ("B");
  EXPECT_EQ (options.placement_blockage_layer (), "B");

  options.set_produce_regions (true);
  EXPECT_EQ (options.produce_regions (), true);
  options.set_produce_regions (false);
  EXPECT_EQ (options.produce_regions (), false);

  options.set_region_layer ("R");
  EXPECT_EQ (options.region_layer (), "R");


  options.set_via_cellname_prefix ("VIACELL");
  EXPECT_EQ (options.via_cellname_prefix (), "VIACELL");

  options.set_produce_via_geometry (true);
  EXPECT_EQ (options.produce_via_geometry (), true);
  options.set_produce_via_geometry (false);
  EXPECT_EQ (options.produce_via_geometry (), false);

  options.set_via_geometry_suffix ("VIAG");
  EXPECT_EQ (options.via_geometry_suffix (), "VIAG");

  options.set_via_geometry_datatype (17);
  EXPECT_EQ (options.via_geometry_datatype (), 17);

  options.set_via_geometry_suffix_str ("VIAG, 2: VIAGM2, 1: VIAGM1");
  EXPECT_EQ (options.via_geometry_suffix_str (), "VIAG,1:VIAGM1,2:VIAGM2");
  EXPECT_EQ (options.via_geometry_suffix_per_mask (0), "VIAG");
  EXPECT_EQ (options.via_geometry_suffix_per_mask (1), "VIAGM1");
  EXPECT_EQ (options.via_geometry_suffix_per_mask (2), "VIAGM2");
  EXPECT_EQ (options.via_geometry_suffix_per_mask (3), "VIAG");
  options.set_via_geometry_suffix_per_mask (2, "AB");
  EXPECT_EQ (options.via_geometry_suffix_per_mask (2), "AB");
  EXPECT_EQ (options.max_mask_number (), (unsigned int) 2);

  options.set_via_geometry_datatype_str ("17, 2:217, 1: 117");
  EXPECT_EQ (options.via_geometry_datatype_str (), "17,1:117,2:217");
  EXPECT_EQ (options.via_geometry_datatype_per_mask (0), 17);
  EXPECT_EQ (options.via_geometry_datatype_per_mask (1), 117);
  EXPECT_EQ (options.via_geometry_datatype_per_mask (2), 217);
  EXPECT_EQ (options.via_geometry_datatype_per_mask (3), 17);
  options.set_via_geometry_datatype_per_mask (2, 42);
  EXPECT_EQ (options.via_geometry_datatype_per_mask (2), 42);

  options.clear_via_geometry_suffixes_per_mask ();
  EXPECT_EQ (options.via_geometry_suffix_str (), "VIAG");

  options.clear_via_geometry_datatypes_per_mask ();
  EXPECT_EQ (options.via_geometry_datatype_str (), "17");


  options.set_produce_pins (true);
  EXPECT_EQ (options.produce_pins (), true);
  options.set_produce_pins (false);
  EXPECT_EQ (options.produce_pins (), false);

  options.set_pins_suffix ("PIN");
  EXPECT_EQ (options.pins_suffix (), "PIN");

  options.set_pins_datatype (42);
  EXPECT_EQ (options.pins_datatype (), 42);

  options.set_pins_suffix_str ("PIN, 2: PINM2, 1: PINM1");
  EXPECT_EQ (options.pins_suffix_str (), "PIN,1:PINM1,2:PINM2");
  EXPECT_EQ (options.pins_suffix_per_mask (0), "PIN");
  EXPECT_EQ (options.pins_suffix_per_mask (1), "PINM1");
  EXPECT_EQ (options.pins_suffix_per_mask (2), "PINM2");
  EXPECT_EQ (options.pins_suffix_per_mask (3), "PIN");
  options.set_pins_suffix_per_mask (2, "AB");
  EXPECT_EQ (options.pins_suffix_per_mask (2), "AB");

  options.set_pins_datatype_str ("42, 2:242, 1: 142");
  EXPECT_EQ (options.pins_datatype_str (), "42,1:142,2:242");
  EXPECT_EQ (options.pins_datatype_per_mask (0), 42);
  EXPECT_EQ (options.pins_datatype_per_mask (1), 142);
  EXPECT_EQ (options.pins_datatype_per_mask (2), 242);
  EXPECT_EQ (options.pins_datatype_per_mask (3), 42);
  options.set_pins_datatype_per_mask (2, 42);
  EXPECT_EQ (options.pins_datatype_per_mask (2), 42);

  options.clear_pins_suffixes_per_mask ();
  EXPECT_EQ (options.pins_suffix_str (), "PIN");

  options.clear_pins_datatypes_per_mask ();
  EXPECT_EQ (options.pins_datatype_str (), "42");


  options.set_produce_lef_pins (true);
  EXPECT_EQ (options.produce_lef_pins (), true);
  options.set_produce_lef_pins (false);
  EXPECT_EQ (options.produce_lef_pins (), false);

  options.set_lef_pins_suffix ("LEFPIN");
  EXPECT_EQ (options.lef_pins_suffix (), "LEFPIN");

  options.set_lef_pins_datatype (41);
  EXPECT_EQ (options.lef_pins_datatype (), 41);

  options.set_lef_pins_suffix_str ("LEFPIN, 2: LEFPINM2, 1: LEFPINM1");
  EXPECT_EQ (options.lef_pins_suffix_str (), "LEFPIN,1:LEFPINM1,2:LEFPINM2");
  EXPECT_EQ (options.lef_pins_suffix_per_mask (0), "LEFPIN");
  EXPECT_EQ (options.lef_pins_suffix_per_mask (1), "LEFPINM1");
  EXPECT_EQ (options.lef_pins_suffix_per_mask (2), "LEFPINM2");
  EXPECT_EQ (options.lef_pins_suffix_per_mask (3), "LEFPIN");
  options.set_lef_pins_suffix_per_mask (2, "AB");
  EXPECT_EQ (options.lef_pins_suffix_per_mask (2), "AB");

  options.set_lef_pins_datatype_str ("41, 2:241, 1: 141");
  EXPECT_EQ (options.lef_pins_datatype_str (), "41,1:141,2:241");
  EXPECT_EQ (options.lef_pins_datatype_per_mask (0), 41);
  EXPECT_EQ (options.lef_pins_datatype_per_mask (1), 141);
  EXPECT_EQ (options.lef_pins_datatype_per_mask (2), 241);
  EXPECT_EQ (options.lef_pins_datatype_per_mask (3), 41);
  options.set_lef_pins_datatype_per_mask (2, 41);
  EXPECT_EQ (options.lef_pins_datatype_per_mask (2), 41);

  options.clear_lef_pins_suffixes_per_mask ();
  EXPECT_EQ (options.lef_pins_suffix_str (), "LEFPIN");

  options.clear_lef_pins_datatypes_per_mask ();
  EXPECT_EQ (options.lef_pins_datatype_str (), "41");

  options.set_produce_obstructions (true);
  EXPECT_EQ (options.produce_obstructions (), true);
  options.set_produce_obstructions (false);
  EXPECT_EQ (options.produce_obstructions (), false);

  options.set_obstructions_suffix ("OBS");
  EXPECT_EQ (options.obstructions_suffix (), "OBS");

  options.set_obstructions_datatype (31);
  EXPECT_EQ (options.obstructions_datatype (), 31);

  options.set_produce_blockages (true);
  EXPECT_EQ (options.produce_blockages (), true);
  options.set_produce_blockages (false);
  EXPECT_EQ (options.produce_blockages (), false);

  options.set_blockages_suffix ("BLK");
  EXPECT_EQ (options.blockages_suffix (), "BLK");

  options.set_blockages_datatype (41);
  EXPECT_EQ (options.blockages_datatype (), 41);

  options.set_produce_labels (true);
  EXPECT_EQ (options.produce_labels (), true);
  options.set_produce_labels (false);
  EXPECT_EQ (options.produce_labels (), false);

  options.set_labels_suffix ("LBL");
  EXPECT_EQ (options.labels_suffix (), "LBL");

  options.set_labels_datatype (51);
  EXPECT_EQ (options.labels_datatype (), 51);


  options.set_produce_routing (true);
  EXPECT_EQ (options.produce_routing (), true);
  options.set_produce_routing (false);
  EXPECT_EQ (options.produce_routing (), false);

  options.set_routing_suffix ("ROUTING");
  EXPECT_EQ (options.routing_suffix (), "ROUTING");

  options.set_routing_datatype (40);
  EXPECT_EQ (options.routing_datatype (), 40);

  options.set_routing_suffix_str ("ROUTING, 2: ROUTINGM2, 1: ROUTINGM1");
  EXPECT_EQ (options.routing_suffix_str (), "ROUTING,1:ROUTINGM1,2:ROUTINGM2");
  EXPECT_EQ (options.routing_suffix_per_mask (0), "ROUTING");
  EXPECT_EQ (options.routing_suffix_per_mask (1), "ROUTINGM1");
  EXPECT_EQ (options.routing_suffix_per_mask (2), "ROUTINGM2");
  EXPECT_EQ (options.routing_suffix_per_mask (3), "ROUTING");
  options.set_routing_suffix_per_mask (2, "AB");
  EXPECT_EQ (options.routing_suffix_per_mask (2), "AB");

  options.set_routing_datatype_str ("40, 2:240, 1: 140");
  EXPECT_EQ (options.routing_datatype_str (), "40,1:140,2:240");
  EXPECT_EQ (options.routing_datatype_per_mask (0), 40);
  EXPECT_EQ (options.routing_datatype_per_mask (1), 140);
  EXPECT_EQ (options.routing_datatype_per_mask (2), 240);
  EXPECT_EQ (options.routing_datatype_per_mask (3), 40);
  options.set_routing_datatype_per_mask (2, 40);
  EXPECT_EQ (options.routing_datatype_per_mask (2), 40);

  options.clear_routing_suffixes_per_mask ();
  EXPECT_EQ (options.routing_suffix_str (), "ROUTING");

  options.clear_routing_datatypes_per_mask ();
  EXPECT_EQ (options.routing_datatype_str (), "40");


  options.set_produce_special_routing (true);
  EXPECT_EQ (options.produce_special_routing (), true);
  options.set_produce_special_routing (false);
  EXPECT_EQ (options.produce_special_routing (), false);

  options.set_special_routing_suffix ("SPECIALROUTING");
  EXPECT_EQ (options.special_routing_suffix (), "SPECIALROUTING");

  options.set_special_routing_datatype (44);
  EXPECT_EQ (options.special_routing_datatype (), 44);

  options.set_special_routing_suffix_str ("SPECIALROUTING, 2: SPECIALROUTINGM2, 1: SPECIALROUTINGM1");
  EXPECT_EQ (options.special_routing_suffix_str (), "SPECIALROUTING,1:SPECIALROUTINGM1,2:SPECIALROUTINGM2");
  EXPECT_EQ (options.special_routing_suffix_per_mask (0), "SPECIALROUTING");
  EXPECT_EQ (options.special_routing_suffix_per_mask (1), "SPECIALROUTINGM1");
  EXPECT_EQ (options.special_routing_suffix_per_mask (2), "SPECIALROUTINGM2");
  EXPECT_EQ (options.special_routing_suffix_per_mask (3), "SPECIALROUTING");
  options.set_special_routing_suffix_per_mask (2, "AB");
  EXPECT_EQ (options.special_routing_suffix_per_mask (2), "AB");

  options.set_special_routing_datatype_str ("44, 2:244, 1: 144");
  EXPECT_EQ (options.special_routing_datatype_str (), "44,1:144,2:244");
  EXPECT_EQ (options.special_routing_datatype_per_mask (0), 44);
  EXPECT_EQ (options.special_routing_datatype_per_mask (1), 144);
  EXPECT_EQ (options.special_routing_datatype_per_mask (2), 244);
  EXPECT_EQ (options.special_routing_datatype_per_mask (3), 44);
  options.set_special_routing_datatype_per_mask (2, 44);
  EXPECT_EQ (options.special_routing_datatype_per_mask (2), 44);

  options.clear_special_routing_suffixes_per_mask ();
  EXPECT_EQ (options.special_routing_suffix_str (), "SPECIALROUTING");

  options.clear_special_routing_datatypes_per_mask ();
  EXPECT_EQ (options.special_routing_datatype_str (), "44");

  EXPECT_EQ (options.begin_lef_files () == options.end_lef_files (), true);
  options.push_lef_file ("ABC.lef");
  EXPECT_EQ (options.begin_lef_files () == options.end_lef_files (), false);
  EXPECT_EQ (options.lef_files ().size (), size_t (1));
  EXPECT_EQ (*options.begin_lef_files (), "ABC.lef");
  std::vector<std::string> lf = options.lef_files ();
  options.clear_lef_files ();
  EXPECT_EQ (options.begin_lef_files () == options.end_lef_files (), true);
  EXPECT_EQ (options.lef_files ().size (), size_t (0));
  options.set_lef_files (lf);
  EXPECT_EQ (options.lef_files ().size (), size_t (1));
  EXPECT_EQ (*options.begin_lef_files (), "ABC.lef");

  EXPECT_EQ (options.read_lef_with_def (), true);
  options.set_read_lef_with_def (false);
  EXPECT_EQ (options.read_lef_with_def (), false);


  options.set_map_file ("ABC.map");
  EXPECT_EQ (options.map_file (), "ABC.map");


  options.set_macro_resolution_mode (2);
  EXPECT_EQ (options.macro_resolution_mode (), (unsigned int) 2);
}


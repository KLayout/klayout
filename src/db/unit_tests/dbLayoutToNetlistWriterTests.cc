
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

#include "dbLayoutToNetlist.h"
#include "dbLayoutToNetlistWriter.h"
#include "dbStream.h"
#include "dbCommonReader.h"
#include "dbNetlistDeviceExtractorClasses.h"
#include "dbTestSupport.h"

#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlFileUtils.h"

static unsigned int define_layer (db::Layout &ly, db::LayerMap &lmap, int gds_layer, int gds_datatype = 0)
{
  unsigned int lid = ly.insert_layer (db::LayerProperties (gds_layer, gds_datatype));
  lmap.map (ly.get_properties (lid), lid);
  return lid;
}

TEST(1_WriterBasic)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int nwell      = define_layer (ly, lmap, 1);
  unsigned int active     = define_layer (ly, lmap, 2);
  unsigned int poly       = define_layer (ly, lmap, 3);
  unsigned int poly_lbl   = define_layer (ly, lmap, 3, 1);
  unsigned int diff_cont  = define_layer (ly, lmap, 4);
  unsigned int poly_cont  = define_layer (ly, lmap, 5);
  unsigned int metal1     = define_layer (ly, lmap, 6);
  unsigned int metal1_lbl = define_layer (ly, lmap, 6, 1);
  unsigned int via1       = define_layer (ly, lmap, 7);
  unsigned int metal2     = define_layer (ly, lmap, 8);
  unsigned int metal2_lbl = define_layer (ly, lmap, 8, 1);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());
  db::LayoutToNetlist l2n (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::unique_ptr<db::Region> rnwell (l2n.make_layer (nwell, "nwell"));
  std::unique_ptr<db::Region> ractive (l2n.make_layer (active, "active"));
  std::unique_ptr<db::Region> rpoly (l2n.make_polygon_layer (poly, "poly"));
  std::unique_ptr<db::Texts> rpoly_lbl (l2n.make_text_layer (poly_lbl, "poly_lbl"));
  std::unique_ptr<db::Region> rdiff_cont (l2n.make_polygon_layer (diff_cont, "diff_cont"));
  std::unique_ptr<db::Region> rpoly_cont (l2n.make_polygon_layer (poly_cont, "poly_cont"));
  std::unique_ptr<db::Region> rmetal1 (l2n.make_polygon_layer (metal1, "metal1"));
  std::unique_ptr<db::Texts> rmetal1_lbl (l2n.make_text_layer (metal1_lbl, "metal1_lbl"));
  std::unique_ptr<db::Region> rvia1 (l2n.make_polygon_layer (via1, "via1"));
  std::unique_ptr<db::Region> rmetal2 (l2n.make_polygon_layer (metal2, "metal2"));
  std::unique_ptr<db::Texts> rmetal2_lbl (l2n.make_text_layer (metal2_lbl, "metal2_lbl"));

  //  derived regions

  db::Region rpactive = *ractive & *rnwell;
  db::Region rpgate   = rpactive & *rpoly;
  db::Region rpsd     = rpactive - rpgate;
  l2n.register_layer (rpactive, "pactive");
  l2n.register_layer (rpgate,   "pgate");
  l2n.register_layer (rpsd,     "psd");

  db::Region rnactive = *ractive - *rnwell;
  db::Region rngate   = rnactive & *rpoly;
  db::Region rnsd     = rnactive - rngate;
  l2n.register_layer (rnactive, "nactive");
  l2n.register_layer (rngate,   "ngate");
  l2n.register_layer (rnsd,     "nsd");

  db::NetlistDeviceExtractorMOS3Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS3Transistor nmos_ex ("NMOS");

  //  device extraction

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  l2n.extract_devices (pmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  l2n.extract_devices (nmos_ex, dl);

  //  return the computed layers into the original layout and write it for debugging purposes
  //  NOTE: this will include the device layers too

  unsigned int lgate  = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> Gate
  unsigned int lsd    = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> Source/Drain
  unsigned int lpdiff = ly.insert_layer (db::LayerProperties (12, 0));      // 12/0 -> P Diffusion
  unsigned int lndiff = ly.insert_layer (db::LayerProperties (13, 0));      // 13/0 -> N Diffusion
  unsigned int lpoly  = ly.insert_layer (db::LayerProperties (14, 0));      // 14/0 -> Poly with gate terminal

  rpgate.insert_into (&ly, tc.cell_index (), lgate);
  rngate.insert_into (&ly, tc.cell_index (), lgate);
  rpsd.insert_into (&ly, tc.cell_index (), lsd);
  rnsd.insert_into (&ly, tc.cell_index (), lsd);
  rpsd.insert_into (&ly, tc.cell_index (), lpdiff);
  rnsd.insert_into (&ly, tc.cell_index (), lndiff);
  rpoly->insert_into (&ly, tc.cell_index (), lpoly);

  //  net extraction

  //  Intra-layer
  l2n.connect (rpsd);
  l2n.connect (rnsd);
  l2n.connect (*rpoly);
  l2n.connect (*rdiff_cont);
  l2n.connect (*rpoly_cont);
  l2n.connect (*rmetal1);
  l2n.connect (*rvia1);
  l2n.connect (*rmetal2);
  //  Inter-layer
  l2n.connect (rpsd,        *rdiff_cont);
  l2n.connect (rnsd,        *rdiff_cont);
  l2n.connect (*rpoly,      *rpoly_cont);
  l2n.connect (*rpoly_cont, *rmetal1);
  l2n.connect (*rdiff_cont, *rmetal1);
  l2n.connect (*rmetal1,    *rvia1);
  l2n.connect (*rvia1,      *rmetal2);
  l2n.connect (*rpoly,      *rpoly_lbl);     //  attaches labels
  l2n.connect (*rmetal1,    *rmetal1_lbl);   //  attaches labels
  l2n.connect (*rmetal2,    *rmetal2_lbl);   //  attaches labels

  //  create some mess - we have to keep references to the layers to make them not disappear
  rmetal1_lbl.reset (0);
  rmetal2_lbl.reset (0);
  rpoly_lbl.reset (0);

  l2n.extract_netlist ();
  l2n.netlist ()->make_top_level_pins ();
  l2n.netlist ()->purge ();

  std::string path = tmp_file ("tmp_l2nwriter_1.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, false);
    writer.write (&l2n);
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "l2n_writer_au.txt");

  compare_text_files (path, au_path);

  path = tmp_file ("tmp_l2nwriter_1s.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, true);
    writer.write (&l2n);
  }

  au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "l2n_writer_au_s.txt");

  compare_text_files (path, au_path);

  //  test build_all_nets (verify reference for reader)

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, unsigned int> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = l2n.layer_of (rpsd);
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = l2n.layer_of (rnsd);
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = l2n.layer_of (*rpoly);
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = l2n.layer_of (*rdiff_cont);
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = l2n.layer_of (*rpoly_cont);
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = l2n.layer_of (*rmetal1);
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = l2n.layer_of (*rvia1);
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = l2n.layer_of (*rmetal2);

    l2n.build_all_nets (cm, ly2, lmap, "NET_", db::NPM_NoProperties, tl::Variant (), db::BNH_Disconnected, 0, "DEVICE_");

    std::string au = tl::testdata ();
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_writer_au.gds");

    db::compare_layouts (_this, ly2, au);
  }

  l2n.netlist ()->begin_circuits ()->set_property (17, 42);
  l2n.netlist ()->begin_circuits ()->set_property ("a_float", 0.5);
  l2n.netlist ()->begin_circuits ()->set_property ("a_\"non_quoted\"_string", "s");

  l2n.netlist ()->begin_circuits ()->begin_nets ()->set_property (17, 142);
  l2n.netlist ()->begin_circuits ()->begin_nets ()->set_property ("a_float", 10.5);
  l2n.netlist ()->begin_circuits ()->begin_nets ()->set_property ("a_\"non_quoted\"_string", "1s");

  l2n.netlist ()->circuit_by_name ("INV2")->begin_devices ()->set_property (17, 242);
  l2n.netlist ()->circuit_by_name ("INV2")->begin_devices ()->set_property ("a_float", 20.5);
  l2n.netlist ()->circuit_by_name ("INV2")->begin_devices ()->set_property ("a_\"non_quoted\"_string", "2s");

  l2n.netlist ()->circuit_by_name ("RINGO")->begin_subcircuits ()->set_property (17, 342);
  l2n.netlist ()->circuit_by_name ("RINGO")->begin_subcircuits ()->set_property ("a_float", 30.5);
  l2n.netlist ()->circuit_by_name ("RINGO")->begin_subcircuits ()->set_property ("a_\"non_quoted\"_string", "3s");

  path = tmp_file ("tmp_l2nwriter_1p.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, true);
    writer.write (&l2n);
  }

  au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "l2n_writer_au_p.txt");

  compare_text_files (path, au_path);
}

TEST(2_WriterWithGlobalNets)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int nwell      = define_layer (ly, lmap, 1);
  unsigned int active     = define_layer (ly, lmap, 2);
  unsigned int pplus      = define_layer (ly, lmap, 10);
  unsigned int nplus      = define_layer (ly, lmap, 11);
  unsigned int poly       = define_layer (ly, lmap, 3);
  unsigned int poly_lbl   = define_layer (ly, lmap, 3, 1);
  unsigned int diff_cont  = define_layer (ly, lmap, 4);
  unsigned int poly_cont  = define_layer (ly, lmap, 5);
  unsigned int metal1     = define_layer (ly, lmap, 6);
  unsigned int metal1_lbl = define_layer (ly, lmap, 6, 1);
  unsigned int via1       = define_layer (ly, lmap, 7);
  unsigned int metal2     = define_layer (ly, lmap, 8);
  unsigned int metal2_lbl = define_layer (ly, lmap, 8, 1);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l3.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());
  db::LayoutToNetlist l2n (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::unique_ptr<db::Region> rbulk (l2n.make_layer (ly.insert_layer (), "rbulk"));
  std::unique_ptr<db::Region> rnwell (l2n.make_layer (nwell, "nwell"));
  std::unique_ptr<db::Region> ractive (l2n.make_layer (active, "active"));
  std::unique_ptr<db::Region> rpplus (l2n.make_layer (pplus, "pplus"));
  std::unique_ptr<db::Region> rnplus (l2n.make_layer (nplus, "nplus"));
  std::unique_ptr<db::Region> rpoly (l2n.make_polygon_layer (poly, "poly"));
  std::unique_ptr<db::Region> rpoly_lbl (l2n.make_layer (poly_lbl, "poly_lbl"));
  std::unique_ptr<db::Region> rdiff_cont (l2n.make_polygon_layer (diff_cont, "diff_cont"));
  std::unique_ptr<db::Region> rpoly_cont (l2n.make_polygon_layer (poly_cont, "poly_cont"));
  std::unique_ptr<db::Region> rmetal1 (l2n.make_polygon_layer (metal1, "metal1"));
  std::unique_ptr<db::Region> rmetal1_lbl (l2n.make_layer (metal1_lbl, "metal1_lbl"));
  std::unique_ptr<db::Region> rvia1 (l2n.make_polygon_layer (via1, "via1"));
  std::unique_ptr<db::Region> rmetal2 (l2n.make_polygon_layer (metal2, "metal2"));
  std::unique_ptr<db::Region> rmetal2_lbl (l2n.make_layer (metal2_lbl, "metal2_lbl"));

  //  derived regions

  db::Region ractive_in_nwell = *ractive & *rnwell;
  db::Region rpactive = ractive_in_nwell & *rpplus;
  db::Region rntie    = ractive_in_nwell & *rnplus;
  db::Region rpgate   = rpactive & *rpoly;
  db::Region rpsd     = rpactive - rpgate;
  l2n.register_layer (rpactive, "pactive");
  l2n.register_layer (rntie,    "ntie");
  l2n.register_layer (rpgate,   "pgate");
  l2n.register_layer (rpsd,     "psd");

  db::Region ractive_outside_nwell = *ractive - *rnwell;
  db::Region rnactive = ractive_outside_nwell & *rnplus;
  db::Region rptie    = ractive_outside_nwell & *rpplus;
  db::Region rngate   = rnactive & *rpoly;
  db::Region rnsd     = rnactive - rngate;
  l2n.register_layer (rnactive, "nactive");
  l2n.register_layer (rptie,    "ptie");
  l2n.register_layer (rngate,   "ngate");
  l2n.register_layer (rnsd,     "nsd");

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int lgate  = ly.insert_layer (db::LayerProperties (20, 0));      // 20/0 -> Gate
  unsigned int lsd    = ly.insert_layer (db::LayerProperties (21, 0));      // 21/0 -> Source/Drain
  unsigned int lpdiff = ly.insert_layer (db::LayerProperties (22, 0));      // 22/0 -> P Diffusion
  unsigned int lndiff = ly.insert_layer (db::LayerProperties (23, 0));      // 23/0 -> N Diffusion
  unsigned int lptie  = ly.insert_layer (db::LayerProperties (24, 0));      // 24/0 -> P Tie
  unsigned int lntie  = ly.insert_layer (db::LayerProperties (25, 0));      // 25/0 -> N Tie

  rpgate.insert_into (&ly, tc.cell_index (), lgate);
  rngate.insert_into (&ly, tc.cell_index (), lgate);
  rpsd.insert_into (&ly, tc.cell_index (), lsd);
  rnsd.insert_into (&ly, tc.cell_index (), lsd);
  rpsd.insert_into (&ly, tc.cell_index (), lpdiff);
  rnsd.insert_into (&ly, tc.cell_index (), lndiff);
  rpsd.insert_into (&ly, tc.cell_index (), lptie);
  rnsd.insert_into (&ly, tc.cell_index (), lntie);

  db::NetlistDeviceExtractorMOS4Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS4Transistor nmos_ex ("NMOS");

  //  device extraction

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rnwell.get ();
  l2n.extract_devices (pmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (nmos_ex, dl);

  //  net extraction

  //  Intra-layer
  l2n.connect (rpsd);
  l2n.connect (rnsd);
  l2n.connect (*rnwell);
  l2n.connect (*rpoly);
  l2n.connect (*rdiff_cont);
  l2n.connect (*rpoly_cont);
  l2n.connect (*rmetal1);
  l2n.connect (*rvia1);
  l2n.connect (*rmetal2);
  l2n.connect (rptie);
  l2n.connect (rntie);
  //  Inter-layer
  l2n.connect (rpsd,        *rdiff_cont);
  l2n.connect (rnsd,        *rdiff_cont);
  l2n.connect (*rpoly,      *rpoly_cont);
  l2n.connect (*rpoly_cont, *rmetal1);
  l2n.connect (*rdiff_cont, *rmetal1);
  l2n.connect (*rdiff_cont, rptie);
  l2n.connect (*rdiff_cont, rntie);
  l2n.connect (*rnwell,     rntie);
  l2n.connect (*rmetal1,    *rvia1);
  l2n.connect (*rvia1,      *rmetal2);
  l2n.connect (*rpoly,      *rpoly_lbl);     //  attaches labels
  l2n.connect (*rmetal1,    *rmetal1_lbl);   //  attaches labels
  l2n.connect (*rmetal2,    *rmetal2_lbl);   //  attaches labels
  //  Global
  l2n.connect_global (rptie, "BULK");
  l2n.connect_global (*rbulk, "BULK");

  //  create some mess - we have to keep references to the layers to make them not disappear
  rmetal1_lbl.reset (0);
  rmetal2_lbl.reset (0);
  rpoly_lbl.reset (0);

  l2n.extract_netlist ();
  l2n.netlist ()->make_top_level_pins ();
  l2n.netlist ()->purge ();

  std::string path = tmp_file ("tmp_l2nwriter_2b.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, false);
    writer.write (&l2n);
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "l2n_writer_au_2b.txt");

  compare_text_files (path, au_path);

  path = tmp_file ("tmp_l2nwriter_2s.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, true);
    writer.write (&l2n);
  }

  au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "l2n_writer_au_2s.txt");

  compare_text_files (path, au_path);

  //  test build_all_nets as reference for the reader

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, unsigned int> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = l2n.layer_of (rpsd);
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = l2n.layer_of (rnsd);
    lmap [ly2.insert_layer (db::LayerProperties (12, 0))] = l2n.layer_of (*rbulk);
    lmap [ly2.insert_layer (db::LayerProperties (13, 0))] = l2n.layer_of (rptie);
    lmap [ly2.insert_layer (db::LayerProperties (14, 0))] = l2n.layer_of (rntie);
    lmap [ly2.insert_layer (db::LayerProperties (1, 0)) ] = l2n.layer_of (*rnwell);
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = l2n.layer_of (*rpoly);
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = l2n.layer_of (*rdiff_cont);
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = l2n.layer_of (*rpoly_cont);
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = l2n.layer_of (*rmetal1);
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = l2n.layer_of (*rvia1);
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = l2n.layer_of (*rmetal2);

    l2n.build_all_nets (cm, ly2, lmap, "NET_", db::NPM_NoProperties, tl::Variant (), db::BNH_SubcircuitCells, "CIRCUIT_", "DEVICE_");

    std::string au = tl::testdata ();
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "l2n_writer_au_2.gds");

    db::compare_layouts (_this, ly2, au);
  }
}

TEST(3_Messages)
{
  db::Layout ly;
  db::Cell &tc = ly.cell (ly.add_cell ("TOP"));
  db::LayoutToNetlist l2n (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  l2n.extract_netlist ();

  l2n.log_entry (db::LogEntryData (db::Info, "info"));
  l2n.log_entry (db::LogEntryData (db::Warning, "warning"));
  l2n.log_entry (db::LogEntryData (db::Error, "error"));

  std::string path = tmp_file ("tmp_l2nwriter_3.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, false);
    writer.write (&l2n);
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "l2n_writer_au_3.txt");

  compare_text_files (path, au_path);

  path = tmp_file ("tmp_l2nwriter_3s.txt");
  {
    tl::OutputStream stream (path);
    db::LayoutToNetlistStandardWriter writer (stream, true);
    writer.write (&l2n);
  }

  au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "l2n_writer_au_3s.txt");

  compare_text_files (path, au_path);
}

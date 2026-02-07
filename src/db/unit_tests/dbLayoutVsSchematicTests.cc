
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

#include "dbNetlistDeviceExtractorClasses.h"
#include "dbLayoutVsSchematic.h"
#include "dbStream.h"
#include "dbDeepRegion.h"
#include "dbDeepShapeStore.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbCommonReader.h"
#include "dbTestSupport.h"
#include "dbNetlistSpiceWriter.h"  //  to create debug files
#include "dbNetlistSpiceReader.h"
#include "dbNetlistCompare.h"

#include "tlUnitTest.h"
#include "tlString.h"
#include "tlFileUtils.h"

#include <memory>
#include <limits>

static unsigned int define_layer (db::Layout &ly, db::LayerMap &lmap, int gds_layer, int gds_datatype = 0)
{
  unsigned int lid = ly.insert_layer (db::LayerProperties (gds_layer, gds_datatype));
  lmap.map (ly.get_properties (lid), lid);
  return lid;
}

static void compare_lvsdbs (tl::TestBase *_this, const std::string &path, const std::string &au_path)
{
  _this->compare_text_files (path, au_path);
}

TEST(1_BasicFlow)
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
    fn = tl::combine_path (fn, "lvs_test_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());
  db::LayoutVsSchematic lvs (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::unique_ptr<db::Region> rbulk (lvs.make_layer ("bulk"));
  std::unique_ptr<db::Region> rnwell (lvs.make_layer (nwell, "nwell"));
  std::unique_ptr<db::Region> ractive (lvs.make_layer (active, "active"));
  std::unique_ptr<db::Region> rpplus (lvs.make_layer (pplus, "pplus"));
  std::unique_ptr<db::Region> rnplus (lvs.make_layer (nplus, "nplus"));
  std::unique_ptr<db::Region> rpoly (lvs.make_polygon_layer (poly, "poly"));
  std::unique_ptr<db::Region> rpoly_lbl (lvs.make_layer (poly_lbl, "poly_lbl"));
  std::unique_ptr<db::Region> rdiff_cont (lvs.make_polygon_layer (diff_cont, "diff_cont"));
  std::unique_ptr<db::Region> rpoly_cont (lvs.make_polygon_layer (poly_cont, "poly_cont"));
  std::unique_ptr<db::Region> rmetal1 (lvs.make_polygon_layer (metal1, "metal1"));
  std::unique_ptr<db::Region> rmetal1_lbl (lvs.make_layer (metal1_lbl, "metal1_lbl"));
  std::unique_ptr<db::Region> rvia1 (lvs.make_polygon_layer (via1, "via1"));
  std::unique_ptr<db::Region> rmetal2 (lvs.make_polygon_layer (metal2, "metal2"));
  std::unique_ptr<db::Region> rmetal2_lbl (lvs.make_layer (metal2_lbl, "metal2_lbl"));

  //  derived regions

  db::Region ractive_in_nwell = *ractive & *rnwell;
  db::Region rpactive = ractive_in_nwell & *rpplus;
  db::Region rntie    = ractive_in_nwell & *rnplus;
  db::Region rpgate   = rpactive & *rpoly;
  db::Region rpsd     = rpactive - rpgate;

  db::Region ractive_outside_nwell = *ractive - *rnwell;
  db::Region rnactive = ractive_outside_nwell & *rnplus;
  db::Region rptie    = ractive_outside_nwell & *rpplus;
  db::Region rngate   = rnactive & *rpoly;
  db::Region rnsd     = rnactive - rngate;

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
  lvs.extract_devices (pmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  lvs.extract_devices (nmos_ex, dl);

  //  net extraction

  lvs.register_layer (rpsd, "psd");
  lvs.register_layer (rnsd, "nsd");
  lvs.register_layer (rptie, "ptie");
  lvs.register_layer (rntie, "ntie");

  //  Intra-layer
  lvs.connect (rpsd);
  lvs.connect (rnsd);
  lvs.connect (*rnwell);
  lvs.connect (*rpoly);
  lvs.connect (*rdiff_cont);
  lvs.connect (*rpoly_cont);
  lvs.connect (*rmetal1);
  lvs.connect (*rvia1);
  lvs.connect (*rmetal2);
  lvs.connect (rptie);
  lvs.connect (rntie);
  //  Inter-layer
  lvs.connect (rpsd,        *rdiff_cont);
  lvs.connect (rnsd,        *rdiff_cont);
  lvs.connect (*rpoly,      *rpoly_cont);
  lvs.connect (*rpoly_cont, *rmetal1);
  lvs.connect (*rdiff_cont, *rmetal1);
  lvs.connect (*rdiff_cont, rptie);
  lvs.connect (*rdiff_cont, rntie);
  lvs.connect (*rnwell,     rntie);
  lvs.connect (*rmetal1,    *rvia1);
  lvs.connect (*rvia1,      *rmetal2);
  lvs.connect (*rpoly,      *rpoly_lbl);     //  attaches labels
  lvs.connect (*rmetal1,    *rmetal1_lbl);   //  attaches labels
  lvs.connect (*rmetal2,    *rmetal2_lbl);   //  attaches labels
  //  Global
  lvs.connect_global (rptie, "BULK");
  lvs.connect_global (*rbulk, "BULK");

  //  create some mess - we have to keep references to the layers to make them not disappear
  rmetal1_lbl.reset (0);
  rmetal2_lbl.reset (0);
  rpoly_lbl.reset (0);

  lvs.extract_netlist ();

  // doesn't do anything here, but we test that this does not destroy anything:
  lvs.netlist ()->combine_devices ();

  //  make pins for named nets of top-level circuits - this way they are not purged
  lvs.netlist ()->make_top_level_pins ();
  lvs.netlist ()->purge ();

  //  read the reference netlist
  {
    db::NetlistSpiceReader reader;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "lvs_test_1.spi");

    std::unique_ptr<db::Netlist> netlist (new db::Netlist ());
    tl::InputStream stream (fn);
    reader.read (stream, *netlist);
    lvs.set_reference_netlist (netlist.release ());
  }

  //  perform the compare
  {
    db::NetlistComparer comparer;
    lvs.compare_netlists (&comparer);
  }

  //  save and compare

  std::string path = tmp_file ("tmp_lvstest1.lvsdb");
  lvs.save (path, false);

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "lvs_test1_au.lvsdb");

  compare_lvsdbs (_this, path, au_path);

  //  load, save and compare

  db::LayoutVsSchematic lvs2;

  std::string path2 = tmp_file ("tmp_lvstest1b.lvsdb");
  lvs2.load (path);
  lvs2.save (path2, false);

  std::string au_path2 = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "lvs_test1b_au.lvsdb");

  compare_lvsdbs (_this, path2, au_path2);
}


TEST(2_FlowWithErrors)
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
    fn = tl::combine_path (fn, "lvs_test_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());
  db::LayoutVsSchematic lvs (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::unique_ptr<db::Region> rbulk (lvs.make_layer ("bulk"));
  std::unique_ptr<db::Region> rnwell (lvs.make_layer (nwell, "nwell"));
  std::unique_ptr<db::Region> ractive (lvs.make_layer (active, "active"));
  std::unique_ptr<db::Region> rpplus (lvs.make_layer (pplus, "pplus"));
  std::unique_ptr<db::Region> rnplus (lvs.make_layer (nplus, "nplus"));
  std::unique_ptr<db::Region> rpoly (lvs.make_polygon_layer (poly, "poly"));
  std::unique_ptr<db::Region> rpoly_lbl (lvs.make_layer (poly_lbl, "poly_lbl"));
  std::unique_ptr<db::Region> rdiff_cont (lvs.make_polygon_layer (diff_cont, "diff_cont"));
  std::unique_ptr<db::Region> rpoly_cont (lvs.make_polygon_layer (poly_cont, "poly_cont"));
  std::unique_ptr<db::Region> rmetal1 (lvs.make_polygon_layer (metal1, "metal1"));
  std::unique_ptr<db::Region> rmetal1_lbl (lvs.make_layer (metal1_lbl, "metal1_lbl"));
  std::unique_ptr<db::Region> rvia1 (lvs.make_polygon_layer (via1, "via1"));
  std::unique_ptr<db::Region> rmetal2 (lvs.make_polygon_layer (metal2, "metal2"));
  std::unique_ptr<db::Region> rmetal2_lbl (lvs.make_layer (metal2_lbl, "metal2_lbl"));

  //  derived regions

  db::Region ractive_in_nwell = *ractive & *rnwell;
  db::Region rpactive = ractive_in_nwell & *rpplus;
  db::Region rntie    = ractive_in_nwell & *rnplus;
  db::Region rpgate   = rpactive & *rpoly;
  db::Region rpsd     = rpactive - rpgate;

  db::Region ractive_outside_nwell = *ractive - *rnwell;
  db::Region rnactive = ractive_outside_nwell & *rnplus;
  db::Region rptie    = ractive_outside_nwell & *rpplus;
  db::Region rngate   = rnactive & *rpoly;
  db::Region rnsd     = rnactive - rngate;

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
  lvs.extract_devices (pmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  lvs.extract_devices (nmos_ex, dl);

  //  net extraction

  lvs.register_layer (rpsd, "psd");
  lvs.register_layer (rnsd, "nsd");
  lvs.register_layer (rptie, "ptie");
  lvs.register_layer (rntie, "ntie");

  //  Intra-layer
  lvs.connect (rpsd);
  lvs.connect (rnsd);
  lvs.connect (*rnwell);
  lvs.connect (*rpoly);
  lvs.connect (*rdiff_cont);
  lvs.connect (*rpoly_cont);
  lvs.connect (*rmetal1);
  lvs.connect (*rvia1);
  lvs.connect (*rmetal2);
  lvs.connect (rptie);
  lvs.connect (rntie);
  //  Inter-layer
  lvs.connect (rpsd,        *rdiff_cont);
  lvs.connect (rnsd,        *rdiff_cont);
  lvs.connect (*rpoly,      *rpoly_cont);
  lvs.connect (*rpoly_cont, *rmetal1);
  lvs.connect (*rdiff_cont, *rmetal1);
  lvs.connect (*rdiff_cont, rptie);
  lvs.connect (*rdiff_cont, rntie);
  lvs.connect (*rnwell,     rntie);
  lvs.connect (*rmetal1,    *rvia1);
  lvs.connect (*rvia1,      *rmetal2);
  lvs.connect (*rpoly,      *rpoly_lbl);     //  attaches labels
  lvs.connect (*rmetal1,    *rmetal1_lbl);   //  attaches labels
  lvs.connect (*rmetal2,    *rmetal2_lbl);   //  attaches labels
  //  Global
  lvs.connect_global (rptie, "BULK");
  lvs.connect_global (*rbulk, "BULK");

  //  create some mess - we have to keep references to the layers to make them not disappear
  rmetal1_lbl.reset (0);
  rmetal2_lbl.reset (0);
  rpoly_lbl.reset (0);

  lvs.extract_netlist ();

  // doesn't do anything here, but we test that this does not destroy anything:
  lvs.netlist ()->combine_devices ();

  //  make pins for named nets of top-level circuits - this way they are not purged
  lvs.netlist ()->make_top_level_pins ();
  lvs.netlist ()->purge ();

  //  read the reference netlist
  {
    db::NetlistSpiceReader reader;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "lvs_test_2.spi");

    std::unique_ptr<db::Netlist> netlist (new db::Netlist ());
    tl::InputStream stream (fn);
    reader.read (stream, *netlist);
    lvs.set_reference_netlist (netlist.release ());
  }

  //  perform the compare
  {
    db::NetlistComparer comparer;
    lvs.compare_netlists (&comparer);
  }

  //  save and compare

  std::string path = tmp_file ("tmp_lvstest2.lvsdb");
  lvs.save (path, false);

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "lvs_test2_au.lvsdb");

  compare_lvsdbs (_this, path, au_path);

  //  load, save and compare

  db::LayoutVsSchematic lvs2;

  std::string path2 = tmp_file ("tmp_lvstest2b.lvsdb");
  lvs2.load (path);
  lvs2.save (path2, false);

  std::string au_path2 = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "lvs_test2b_au.lvsdb");

  compare_lvsdbs (_this, path2, au_path2);
}

TEST(3_ReaderFuture)
{
  db::LayoutVsSchematic lvs;

  std::string in_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "lvs_test3.lvsdb");
  lvs.load (in_path);

  //  verify against the input

  std::string path = tmp_file ("tmp.txt");
  lvs.save (path, false);

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "lvs_test3_au.lvsdb");

  compare_text_files (path, au_path);
}

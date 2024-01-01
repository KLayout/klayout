
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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
#include "dbLayoutToNetlist.h"
#include "dbStream.h"
#include "dbDeepRegion.h"
#include "dbDeepShapeStore.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbCommonReader.h"
#include "dbTestSupport.h"

#include "tlUnitTest.h"
#include "tlString.h"
#include "tlFileUtils.h"

#include <memory>
#include <limits>

static std::string qnet_name (const db::Net *net)
{
  return net ? net->qname () : "(null)";
}

static void dump_nets_to_layout (const db::LayoutToNetlist &l2n, db::Layout &ly, const std::map<const db::Region *, unsigned int> &lmap, const db::CellMapping &cmap)
{
  const db::Netlist &nl = *l2n.netlist ();
  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {

    db::Cell &cell = ly.cell (cmap.cell_mapping (c->cell_index ()));

    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {

      db::cell_index_type nci = std::numeric_limits<db::cell_index_type>::max ();

      for (std::map<const db::Region *, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end (); ++m) {

        std::unique_ptr<db::Region> shapes (l2n.shapes_of_net (*n, *m->first, false));
        if (shapes->empty ()) {
          continue;
        }

        if (nci == std::numeric_limits<db::cell_index_type>::max ()) {
          std::string nn = "NET_" + c->name () + "_" + n->expanded_name ();
          nci = ly.add_cell (nn.c_str ());
          cell.insert (db::CellInstArray (db::CellInst (nci), db::Trans ()));
        }

        shapes->insert_into (&ly, nci, m->second);

      }

    }

  }
}

static void dump_recursive_nets_to_layout (const db::LayoutToNetlist &l2n, db::Layout &ly, const std::map<const db::Region *, unsigned int> &lmap, const db::CellMapping &cmap)
{
  const db::Netlist &nl = *l2n.netlist ();
  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {

    db::Cell &cell = ly.cell (cmap.cell_mapping (c->cell_index ()));

    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {

      //  only handle nets without outgoing pins - these are local
      if (n->pin_count () > 0) {
        continue;
      }

      bool any = false;
      for (std::map<const db::Region *, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end () && !any; ++m) {
        any = !db::recursive_cluster_shape_iterator<db::NetShape> (l2n.net_clusters (), l2n.layer_of (*m->first), c->cell_index (), n->cluster_id ()).at_end ();
      }

      if (!any) {
        continue;
      }

      db::cell_index_type nci = std::numeric_limits<db::cell_index_type>::max ();

      if (nci == std::numeric_limits<db::cell_index_type>::max ()) {
        std::string nn = "RNET_" + c->name () + "_" + n->expanded_name ();
        nci = ly.add_cell (nn.c_str ());
        cell.insert (db::CellInstArray (db::CellInst (nci), db::Trans ()));
      }

      for (std::map<const db::Region *, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end (); ++m) {
        l2n.shapes_of_net (*n, *m->first, true, ly.cell (nci).shapes (m->second));
      }

    }

  }
}

static unsigned int define_layer (db::Layout &ly, db::LayerMap &lmap, int gds_layer, int gds_datatype = 0)
{
  unsigned int lid = ly.insert_layer (db::LayerProperties (gds_layer, gds_datatype));
  lmap.map (ly.get_properties (lid), lid);
  return lid;
}

TEST(0_Basic)
{
  db::LayoutToNetlist l2n;

  std::unique_ptr<db::Region> reg (l2n.make_layer ("l1"));
  EXPECT_EQ (l2n.is_persisted (*reg), true);
  EXPECT_EQ (l2n.name (*reg), "l1");
  EXPECT_EQ (l2n.layer_of (*reg), 0u);
  EXPECT_EQ (l2n.internal_layout ()->is_valid_layer (0), true);
  reg.reset (0);
  EXPECT_EQ (l2n.internal_layout ()->is_valid_layer (0), true);
  EXPECT_EQ (l2n.name (0u), "l1");

  EXPECT_EQ (l2n.layer_by_index (1) == 0, true);
  EXPECT_EQ (l2n.layer_by_name ("l2") == 0, true);

  std::unique_ptr<db::Region> reg_copy (l2n.layer_by_name ("l1"));
  EXPECT_EQ (reg_copy.get () != 0, true);
  EXPECT_EQ (l2n.name (*reg_copy), "l1");
  EXPECT_EQ (l2n.layer_of (*reg_copy), 0u);
  reg_copy.reset (l2n.layer_by_index (0));
  EXPECT_EQ (reg_copy.get () != 0, true);
  EXPECT_EQ (l2n.name (*reg_copy), "l1");
  EXPECT_EQ (l2n.layer_of (*reg_copy), 0u);
  reg_copy.reset (0);

  std::unique_ptr<db::Region> reg2 (l2n.make_layer ());
  EXPECT_EQ (l2n.name (1u), "$1");
  EXPECT_EQ (l2n.name (*reg2), "$1");
  EXPECT_EQ (l2n.layer_of (*reg2), 1u);
  EXPECT_EQ (l2n.internal_layout ()->is_valid_layer (1), true);
  reg2.reset (0);
  //  NOTE: deleting the region does not free the layer as we hold it internally inside LayoutToNetlist
  EXPECT_EQ (l2n.internal_layout ()->is_valid_layer (1), true);

  std::unique_ptr<db::Region> reg3 (l2n.make_layer ("l3"));
  EXPECT_EQ (l2n.name (*reg3), "l3");
  EXPECT_EQ (l2n.layer_of (*reg3), 2u);

  std::string s;
  for (db::LayoutToNetlist::layer_iterator l = l2n.begin_layers (); l != l2n.end_layers (); ++l) {
    s += tl::to_string (l->first) + ":" + l->second + ";";
  }
  EXPECT_EQ (s, "0:l1;1:$1;2:l3;");
}

TEST(1_BasicExtraction)
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

  db::Region rnactive = *ractive - *rnwell;
  db::Region rngate   = rnactive & *rpoly;
  db::Region rnsd     = rnactive - rngate;

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

  l2n.register_layer (rpsd, "psd");
  l2n.register_layer (rnsd, "nsd");

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

  //  debug layers produced for nets
  //    202/0 -> Active
  //    203/0 -> Poly
  //    204/0 -> Diffusion contacts
  //    205/0 -> Poly contacts
  //    206/0 -> Metal1
  //    207/0 -> Via1
  //    208/0 -> Metal2
  //    210/0 -> N source/drain
  //    211/0 -> P source/drain
  std::map<const db::Region *, unsigned int> dump_map;
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = l2n.cell_mapping_into (ly, tc, true /*with device cells*/);
  dump_nets_to_layout (l2n, ly, dump_map, cm);

  dump_map.clear ();
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (310, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (311, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (303, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (304, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (305, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (306, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (307, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (308, 0));

  dump_recursive_nets_to_layout (l2n, ly, dump_map, cm);

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1_with_rec_nets.gds");

  db::compare_layouts (_this, ly, au);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO ();\n"
    "  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit TRANS $4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n"
  );

  //  do some probing before purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VSS");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "RINGO:$I39");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "RINGO:$I2");

  //  test build_all_nets

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, 0, db::NPM_NoProperties, tl::Variant (), db::BNH_Disconnected, 0, 0);

    std::string au = tl::testdata ();
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "device_extract_au1_rebuild_ff.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, "NET_", db::NPM_NoProperties, tl::Variant (), db::BNH_Disconnected, 0, 0);

    std::string au = tl::testdata ();
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "device_extract_au1_rebuild_nf.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, 0, db::NPM_NoProperties, tl::Variant (), db::BNH_SubcircuitCells, "CIRCUIT_", 0);

    std::string au = tl::testdata ();
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "device_extract_au1_rebuild_fr.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, 0, db::NPM_AllProperties, tl::Variant (42), db::BNH_Flatten, 0, 0);

    std::string au = tl::testdata ();
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "device_extract_au1_rebuild_pf.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, 0, db::NPM_AllProperties, tl::Variant (42), db::BNH_SubcircuitCells, "CIRCUIT_", 0);

    std::string au = tl::testdata ();
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "device_extract_au1_rebuild_pr.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, "NET_", db::NPM_NoProperties, tl::Variant (), db::BNH_SubcircuitCells, "CIRCUIT_", "DEVICE_");

    std::string au = tl::testdata ();
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "device_extract_au1_rebuild_nr.gds");

    db::compare_layouts (_this, ly2, au);
  }

  // doesn't do anything here, but we test that this does not destroy anything:
  l2n.netlist ()->combine_devices ();

  //  make pins for named nets of top-level circuits - this way they are not purged
  l2n.netlist ()->make_top_level_pins ();
  l2n.netlist ()->purge ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO (FB=FB,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n"
  );

  //  do some probing after purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VSS");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "RINGO:$I39");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "RINGO:$I2");

  //  use this opportunity to check joining of nets with cluster joining
  db::Circuit *top = l2n.netlist ()->circuit_by_name ("RINGO");
  top->join_nets (top->net_by_name ("VSS"), top->net_by_name ("VDD"));
  top->join_nets (top->net_by_name ("FB"), top->net_by_name ("OSC"));

  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO (FB=FB,OSC=FB,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=FB,$4=VSS,$5=VSS);\n"
    "  subcircuit INV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VSS);\n"
    "  subcircuit INV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VSS);\n"
    "  subcircuit INV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VSS);\n"
    "  subcircuit INV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VSS);\n"
    "  subcircuit INV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VSS);\n"
    "  subcircuit INV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VSS);\n"
    "  subcircuit INV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VSS);\n"
    "  subcircuit INV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VSS);\n"
    "  subcircuit INV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VSS);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n"
  );

  //  do some probing after purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB,OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB,OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB,OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:FB,OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VDD,VSS");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "RINGO:$I39");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "RINGO:$I2");

  //  compare the collected test data

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2, true /*with device cells*/);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, "NET_", db::NPM_NoProperties, tl::Variant (), db::BNH_SubcircuitCells, "CIRCUIT_", "DEVICE_");

    std::string au = tl::testdata ();
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "device_extract_au1_joined_nets.gds");

    db::compare_layouts (_this, ly2, au);
  }
}

TEST(2_Probing)
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
    fn = tl::combine_path (fn, "device_extract_l2.gds");

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

  db::Region rnactive = *ractive - *rnwell;
  db::Region rngate   = rnactive & *rpoly;
  db::Region rnsd     = rnactive - rngate;

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int lgate  = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> Gate
  unsigned int lsd    = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> Source/Drain
  unsigned int lpdiff = ly.insert_layer (db::LayerProperties (12, 0));      // 12/0 -> P Diffusion
  unsigned int lndiff = ly.insert_layer (db::LayerProperties (13, 0));      // 13/0 -> N Diffusion

  rpgate.insert_into (&ly, tc.cell_index (), lgate);
  rngate.insert_into (&ly, tc.cell_index (), lgate);
  rpsd.insert_into (&ly, tc.cell_index (), lsd);
  rnsd.insert_into (&ly, tc.cell_index (), lsd);
  rpsd.insert_into (&ly, tc.cell_index (), lpdiff);
  rnsd.insert_into (&ly, tc.cell_index (), lndiff);

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

  //  net extraction

  l2n.register_layer (rpsd, "psd");
  l2n.register_layer (rnsd, "nsd");

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

  //  debug layers produced for nets
  //    202/0 -> Active
  //    203/0 -> Poly
  //    204/0 -> Diffusion contacts
  //    205/0 -> Poly contacts
  //    206/0 -> Metal1
  //    207/0 -> Via1
  //    208/0 -> Metal2
  //    210/0 -> N source/drain
  //    211/0 -> P source/drain
  std::map<const db::Region *, unsigned int> dump_map;
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = l2n.cell_mapping_into (ly, tc);
  dump_nets_to_layout (l2n, ly, dump_map, cm);

  dump_map.clear ();
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (310, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (311, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (303, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (304, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (305, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (306, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (307, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (308, 0));

  dump_recursive_nets_to_layout (l2n, ly, dump_map, cm);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO ();\n"
    "  subcircuit INV2PAIR $1 ($1=FB,$2=VDD,$3=VSS,$4=$I3,$5=OSC);\n"
    "  subcircuit INV2PAIR $2 ($1=$I18,$2=VDD,$3=VSS,$4=FB,$5=$I9);\n"
    "  subcircuit INV2PAIR $3 ($1=$I19,$2=VDD,$3=VSS,$4=$I9,$5=$I1);\n"
    "  subcircuit INV2PAIR $4 ($1=$I20,$2=VDD,$3=VSS,$4=$I1,$5=$I2);\n"
    "  subcircuit INV2PAIR $5 ($1=$I21,$2=VDD,$3=VSS,$4=$I2,$5=$I3);\n"
    "end;\n"
    "circuit INV2PAIR ($1=$I7,$2=$I5,$3=$I4,$4=$I2,$5=$I1);\n"
    "  subcircuit INV2 $1 (IN=$I3,$2=$I7,OUT=$I1,$4=$I4,$5=$I5);\n"
    "  subcircuit INV2 $2 (IN=$I2,$2=$I6,OUT=$I3,$4=$I4,$5=$I5);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit TRANS $4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au2_with_rec_nets.gds");

  db::compare_layouts (_this, ly, au);

  //  do some probing before purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VSS");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "RINGO:$I18");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "INV2PAIR:$I3");

  // doesn't do anything here, but we test that this does not destroy anything:
  l2n.netlist ()->combine_devices ();

  //  make pins for named nets of top-level circuits - this way they are not purged
  l2n.netlist ()->make_top_level_pins ();
  l2n.netlist ()->purge ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO (FB=FB,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2PAIR $1 ($1=FB,$2=VDD,$3=VSS,$4=$I3,$5=OSC);\n"
    "  subcircuit INV2PAIR $2 ($1=$I18,$2=VDD,$3=VSS,$4=FB,$5=$I9);\n"
    "  subcircuit INV2PAIR $3 ($1=$I19,$2=VDD,$3=VSS,$4=$I9,$5=$I1);\n"
    "  subcircuit INV2PAIR $4 ($1=$I20,$2=VDD,$3=VSS,$4=$I1,$5=$I2);\n"
    "  subcircuit INV2PAIR $5 ($1=$I21,$2=VDD,$3=VSS,$4=$I2,$5=$I3);\n"
    "end;\n"
    "circuit INV2PAIR ($1=$I7,$2=$I5,$3=$I4,$4=$I2,$5=$I1);\n"
    "  subcircuit INV2 $1 (IN=$I3,$2=$I7,OUT=$I1,$4=$I4,$5=$I5);\n"
    "  subcircuit INV2 $2 (IN=$I2,$2=$I6,OUT=$I3,$4=$I4,$5=$I5);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n"
  );

  //  do some probing after purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VSS");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "RINGO:$I18");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "INV2PAIR:$I3");
}

TEST(3_GlobalNetConnections)
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

  std::unique_ptr<db::Region> rnwell (l2n.make_layer (nwell, "nwell"));
  std::unique_ptr<db::Region> ractive (l2n.make_layer (active, "active"));
  std::unique_ptr<db::Region> rpplus (l2n.make_layer (pplus, "pplus"));
  std::unique_ptr<db::Region> rnplus (l2n.make_layer (nplus, "nplus"));
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

  //  net extraction

  l2n.register_layer (rpsd, "psd");
  l2n.register_layer (rnsd, "nsd");
  l2n.register_layer (rptie, "ptie");
  l2n.register_layer (rntie, "ntie");

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

  //  create some mess - we have to keep references to the layers to make them not disappear
  rmetal1_lbl.reset (0);
  rmetal2_lbl.reset (0);
  rpoly_lbl.reset (0);

  l2n.extract_netlist ();

  //  debug layers produced for nets
  //    201/0 -> Well
  //    203/0 -> Poly
  //    204/0 -> Diffusion contacts
  //    205/0 -> Poly contacts
  //    206/0 -> Metal1
  //    207/0 -> Via1
  //    208/0 -> Metal2
  //    210/0 -> N source/drain
  //    211/0 -> P source/drain
  //    212/0 -> N tie
  //    213/0 -> P tie
  std::map<const db::Region *, unsigned int> dump_map;
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [&rptie           ] = ly.insert_layer (db::LayerProperties (212, 0));
  dump_map [&rntie           ] = ly.insert_layer (db::LayerProperties (213, 0));
  dump_map [rnwell.get ()    ] = ly.insert_layer (db::LayerProperties (201, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = l2n.cell_mapping_into (ly, tc);
  dump_nets_to_layout (l2n, ly, dump_map, cm);

  dump_map.clear ();
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (310, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (311, 0));
  dump_map [&rptie           ] = ly.insert_layer (db::LayerProperties (312, 0));
  dump_map [&rntie           ] = ly.insert_layer (db::LayerProperties (313, 0));
  dump_map [rnwell.get ()    ] = ly.insert_layer (db::LayerProperties (301, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (303, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (304, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (305, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (306, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (307, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (308, 0));

  dump_recursive_nets_to_layout (l2n, ly, dump_map, cm);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO ();\n"
    "  subcircuit INV2PAIR $1 (BULK='BULK,VSS',$2=FB,$3=VDD,$4='BULK,VSS',$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 (BULK='BULK,VSS',$2=$I22,$3=VDD,$4='BULK,VSS',$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 (BULK='BULK,VSS',$2=$I23,$3=VDD,$4='BULK,VSS',$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 (BULK='BULK,VSS',$2=$I24,$3=VDD,$4='BULK,VSS',$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 (BULK='BULK,VSS',$2=$I25,$3=VDD,$4='BULK,VSS',$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    "circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device PMOS $1 (S=$3,G=IN,D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$3,$2=VSS,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$3,$2=VDD,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=VDD,$2=OUT,$3=$3);\n"
    "  subcircuit TRANS $4 ($1=VSS,$2=OUT,$3=$3);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au3_with_rec_nets.gds");

  db::compare_layouts (_this, ly, au);

  //  do some probing before purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VSS");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "RINGO:$I22");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "INV2PAIR:$I4");

  // doesn't do anything here, but we test that this does not destroy anything:
  l2n.netlist ()->combine_devices ();

  //  make pins for named nets of top-level circuits - this way they are not purged
  l2n.netlist ()->make_top_level_pins ();
  l2n.netlist ()->purge ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2PAIR $1 ($2=FB,$3=VDD,$4=VSS,$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 ($2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 ($2=$I23,$3=VDD,$4=VSS,$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 ($2=$I24,$3=VDD,$4=VSS,$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 ($2=$I25,$3=VDD,$4=VSS,$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    "circuit INV2PAIR ($2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6);\n"
    "  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD);\n"
    "  device PMOS $1 (S=$3,G=IN,D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n"
  );

  //  do some probing after purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VSS");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "RINGO:$I22");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "INV2PAIR:$I4");
}

TEST(4_GlobalNetDeviceExtraction)
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

  std::unique_ptr<db::Region> rbulk (l2n.make_layer (ly.insert_layer (), "bulk"));
  std::unique_ptr<db::Region> rnwell (l2n.make_layer (nwell, "nwell"));
  std::unique_ptr<db::Region> ractive (l2n.make_layer (active, "active"));
  std::unique_ptr<db::Region> rpplus (l2n.make_layer (pplus, "pplus"));
  std::unique_ptr<db::Region> rnplus (l2n.make_layer (nplus, "nplus"));
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
  l2n.extract_devices (pmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (nmos_ex, dl);

  //  net extraction

  l2n.register_layer (rpsd, "psd");
  l2n.register_layer (rnsd, "nsd");
  l2n.register_layer (rptie, "ptie");
  l2n.register_layer (rntie, "ntie");

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

  //  debug layers produced for nets
  //    201/0 -> Well
  //    203/0 -> Poly
  //    204/0 -> Diffusion contacts
  //    205/0 -> Poly contacts
  //    206/0 -> Metal1
  //    207/0 -> Via1
  //    208/0 -> Metal2
  //    210/0 -> N source/drain
  //    211/0 -> P source/drain
  //    212/0 -> N tie
  //    213/0 -> P tie
  std::map<const db::Region *, unsigned int> dump_map;
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [&rptie           ] = ly.insert_layer (db::LayerProperties (212, 0));
  dump_map [&rntie           ] = ly.insert_layer (db::LayerProperties (213, 0));
  dump_map [rbulk.get ()     ] = ly.insert_layer (db::LayerProperties (214, 0));
  dump_map [rnwell.get ()    ] = ly.insert_layer (db::LayerProperties (201, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = l2n.cell_mapping_into (ly, tc);
  dump_nets_to_layout (l2n, ly, dump_map, cm);

  dump_map.clear ();
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (310, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (311, 0));
  dump_map [&rptie           ] = ly.insert_layer (db::LayerProperties (312, 0));
  dump_map [&rntie           ] = ly.insert_layer (db::LayerProperties (313, 0));
  dump_map [rbulk.get ()     ] = ly.insert_layer (db::LayerProperties (314, 0));
  dump_map [rnwell.get ()    ] = ly.insert_layer (db::LayerProperties (301, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (303, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (304, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (305, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (306, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (307, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (308, 0));

  dump_recursive_nets_to_layout (l2n, ly, dump_map, cm);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO ();\n"
    "  subcircuit INV2PAIR $1 (BULK='BULK,VSS',$2=FB,$3=VDD,$4='BULK,VSS',$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 (BULK='BULK,VSS',$2=$I22,$3=VDD,$4='BULK,VSS',$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 (BULK='BULK,VSS',$2=$I23,$3=VDD,$4='BULK,VSS',$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 (BULK='BULK,VSS',$2=$I24,$3=VDD,$4='BULK,VSS',$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 (BULK='BULK,VSS',$2=$I25,$3=VDD,$4='BULK,VSS',$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    "circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device PMOS $1 (S=$3,G=IN,D=VDD,B=$1) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$3,D=OUT,B=$1) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS,B=BULK) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT,B=BULK) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$3,$2=VSS,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$3,$2=VDD,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=VDD,$2=OUT,$3=$3);\n"
    "  subcircuit TRANS $4 ($1=VSS,$2=OUT,$3=$3);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au4_with_rec_nets.gds");

  db::compare_layouts (_this, ly, au);

  //  do some probing before purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VSS");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "RINGO:$I22");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "INV2PAIR:$I4");

  // doesn't do anything here, but we test that this does not destroy anything:
  l2n.netlist ()->combine_devices ();

  //  make pins for named nets of top-level circuits - this way they are not purged
  l2n.netlist ()->make_top_level_pins ();
  l2n.netlist ()->purge ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2PAIR $1 (BULK=VSS,$2=FB,$3=VDD,$4=VSS,$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 (BULK=VSS,$2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 (BULK=VSS,$2=$I23,$3=VDD,$4=VSS,$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 (BULK=VSS,$2=$I24,$3=VDD,$4=VSS,$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 (BULK=VSS,$2=$I25,$3=VDD,$4=VSS,$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    "circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device PMOS $1 (S=$3,G=IN,D=VDD,B=$1) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$3,D=OUT,B=$1) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS,B=BULK) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT,B=BULK) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n"
  );

  //  do some probing after purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VSS");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "RINGO:$I22");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "INV2PAIR:$I4");
}

TEST(5_DeviceExtractionWithDeviceCombination)
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
    fn = tl::combine_path (fn, "device_extract_l5.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());
  db::LayoutToNetlist l2n (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::unique_ptr<db::Region> rbulk (l2n.make_layer ("bulk"));
  std::unique_ptr<db::Region> rnwell (l2n.make_layer (nwell, "nwell"));
  std::unique_ptr<db::Region> ractive (l2n.make_layer (active, "active"));
  std::unique_ptr<db::Region> rpplus (l2n.make_layer (pplus, "pplus"));
  std::unique_ptr<db::Region> rnplus (l2n.make_layer (nplus, "nplus"));
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
  l2n.extract_devices (pmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (nmos_ex, dl);

  //  net extraction

  l2n.register_layer (rpsd, "psd");
  l2n.register_layer (rnsd, "nsd");
  l2n.register_layer (rptie, "ptie");
  l2n.register_layer (rntie, "ntie");

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

  //  debug layers produced for nets
  //    201/0 -> Well
  //    203/0 -> Poly
  //    204/0 -> Diffusion contacts
  //    205/0 -> Poly contacts
  //    206/0 -> Metal1
  //    207/0 -> Via1
  //    208/0 -> Metal2
  //    210/0 -> N source/drain
  //    211/0 -> P source/drain
  //    212/0 -> N tie
  //    213/0 -> P tie
  std::map<const db::Region *, unsigned int> dump_map;
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [&rptie           ] = ly.insert_layer (db::LayerProperties (212, 0));
  dump_map [&rntie           ] = ly.insert_layer (db::LayerProperties (213, 0));
  dump_map [rbulk.get ()     ] = ly.insert_layer (db::LayerProperties (214, 0));
  dump_map [rnwell.get ()    ] = ly.insert_layer (db::LayerProperties (201, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = l2n.cell_mapping_into (ly, tc);
  dump_nets_to_layout (l2n, ly, dump_map, cm);

  dump_map.clear ();
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (310, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (311, 0));
  dump_map [&rptie           ] = ly.insert_layer (db::LayerProperties (312, 0));
  dump_map [&rntie           ] = ly.insert_layer (db::LayerProperties (313, 0));
  dump_map [rbulk.get ()     ] = ly.insert_layer (db::LayerProperties (314, 0));
  dump_map [rnwell.get ()    ] = ly.insert_layer (db::LayerProperties (301, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (303, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (304, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (305, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (306, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (307, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (308, 0));

  dump_recursive_nets_to_layout (l2n, ly, dump_map, cm);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO ();\n"
    "  subcircuit INV2PAIR $1 (BULK='BULK,VSS',$2=VDD,$3='BULK,VSS',$4=FB,$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 (BULK='BULK,VSS',$2=VDD,$3='BULK,VSS',$4=$I22,$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 (BULK='BULK,VSS',$2=VDD,$3='BULK,VSS',$4=$I23,$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 (BULK='BULK,VSS',$2=VDD,$3='BULK,VSS',$4=$I24,$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 (BULK='BULK,VSS',$2=VDD,$3='BULK,VSS',$4=$I25,$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    "circuit INV2PAIR (BULK=BULK,$2=$I6,$3=$I5,$4=$I4,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I1,IN=$I3,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "  subcircuit INV2 $2 ($1=$I1,IN=$I4,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device PMOS $1 (S=OUT,G=IN,D=VDD,B=$1) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=OUT,B=$1) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $3 (S=OUT,G=IN,D=VSS,B=BULK) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $4 (S=VSS,G=IN,D=OUT,B=BULK) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  subcircuit TRANS $1 ($1=OUT,$2=VSS,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=OUT,$2=VDD,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=OUT,$2=VSS,$3=IN);\n"
    "  subcircuit TRANS $4 ($1=OUT,$2=VDD,$3=IN);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au5_with_rec_nets.gds");

  db::compare_layouts (_this, ly, au);

  //  do some probing before purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VSS");

  // doesn't do anything here, but we test that this does not destroy anything:
  l2n.netlist ()->combine_devices ();

  //  make pins for named nets of top-level circuits - this way they are not purged
  l2n.netlist ()->make_top_level_pins ();
  l2n.netlist ()->purge ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2PAIR $1 (BULK=VSS,$2=VDD,$3=VSS,$4=FB,$5=$I7,$6=OSC,$7=VDD);\n"
    "  subcircuit INV2PAIR $2 (BULK=VSS,$2=VDD,$3=VSS,$4=$I22,$5=FB,$6=$I13,$7=VDD);\n"
    "  subcircuit INV2PAIR $3 (BULK=VSS,$2=VDD,$3=VSS,$4=$I23,$5=$I13,$6=$I5,$7=VDD);\n"
    "  subcircuit INV2PAIR $4 (BULK=VSS,$2=VDD,$3=VSS,$4=$I24,$5=$I5,$6=$I6,$7=VDD);\n"
    "  subcircuit INV2PAIR $5 (BULK=VSS,$2=VDD,$3=VSS,$4=$I25,$5=$I6,$6=$I7,$7=VDD);\n"
    "end;\n"
    "circuit INV2PAIR (BULK=BULK,$2=$I6,$3=$I5,$4=$I4,$5=$I3,$6=$I2,$7=$I1);\n"
    "  subcircuit INV2 $1 ($1=$I1,IN=$I3,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "  subcircuit INV2 $2 ($1=$I1,IN=$I4,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);\n"
    "end;\n"
    "circuit INV2 ($1=$1,IN=IN,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device PMOS $1 (S=OUT,G=IN,D=VDD,B=$1) (L=0.25,W=3.5,AS=1.4,AD=1.4,PS=6.85,PD=6.85);\n"
    "  device NMOS $3 (S=OUT,G=IN,D=VSS,B=BULK) (L=0.25,W=3.5,AS=1.4,AD=1.4,PS=6.85,PD=6.85);\n"
    "end;\n"
  );

  //  do some probing after purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "RINGO:VSS");
}

TEST(6_MoreDeviceTypes)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int nwell      = define_layer (ly, lmap, 1);
  unsigned int active     = define_layer (ly, lmap, 2);
  unsigned int thickgox   = define_layer (ly, lmap, 3);
  unsigned int pplus      = define_layer (ly, lmap, 4);
  unsigned int nplus      = define_layer (ly, lmap, 5);
  unsigned int poly       = define_layer (ly, lmap, 6);
  unsigned int poly_lbl   = define_layer (ly, lmap, 7);
  unsigned int cont       = define_layer (ly, lmap, 8);
  unsigned int metal1     = define_layer (ly, lmap, 9);
  unsigned int metal1_lbl = define_layer (ly, lmap, 10);
  unsigned int via1       = define_layer (ly, lmap, 11);
  unsigned int metal2     = define_layer (ly, lmap, 12);
  unsigned int metal2_lbl = define_layer (ly, lmap, 13);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l6.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());
  db::LayoutToNetlist l2n (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::unique_ptr<db::Region> rbulk (l2n.make_layer ("bulk"));
  std::unique_ptr<db::Region> rnwell (l2n.make_layer (nwell, "nwell"));
  std::unique_ptr<db::Region> rthickgox (l2n.make_layer (thickgox, "thickgox"));
  std::unique_ptr<db::Region> ractive (l2n.make_layer (active, "active"));
  std::unique_ptr<db::Region> rpplus (l2n.make_layer (pplus, "pplus"));
  std::unique_ptr<db::Region> rnplus (l2n.make_layer (nplus, "nplus"));
  std::unique_ptr<db::Region> rpoly (l2n.make_polygon_layer (poly, "poly"));
  std::unique_ptr<db::Texts> rpoly_lbl (l2n.make_text_layer (poly_lbl, "poly_lbl"));
  std::unique_ptr<db::Region> rcont (l2n.make_polygon_layer (cont, "cont"));
  std::unique_ptr<db::Region> rmetal1 (l2n.make_polygon_layer (metal1, "metal1"));
  std::unique_ptr<db::Texts> rmetal1_lbl (l2n.make_text_layer (metal1_lbl, "metal1_lbl"));
  std::unique_ptr<db::Region> rvia1 (l2n.make_polygon_layer (via1, "via1"));
  std::unique_ptr<db::Region> rmetal2 (l2n.make_polygon_layer (metal2, "metal2"));
  std::unique_ptr<db::Texts> rmetal2_lbl (l2n.make_text_layer (metal2_lbl, "metal2_lbl"));

  //  derived regions

  db::Region ractive_in_nwell = *ractive & *rnwell;
  db::Region rpactive    = ractive_in_nwell - *rnplus;
  db::Region rntie       = ractive_in_nwell & *rnplus;
  db::Region rpgate      = rpactive & *rpoly;
  db::Region rpsd        = rpactive - rpgate;
  db::Region rhv_pgate   = rpgate & *rthickgox;
  db::Region rlv_pgate   = rpgate - rhv_pgate;
  db::Region rhv_psd     = rpsd & *rthickgox;
  db::Region rlv_psd     = rpsd - *rthickgox;

  l2n.register_layer(rntie, "ntie");
  l2n.register_layer(rpsd,  "psd");

  db::Region ractive_outside_nwell = *ractive - *rnwell;
  db::Region rnactive    = ractive_outside_nwell - *rpplus;
  db::Region rptie       = ractive_outside_nwell & *rpplus;
  db::Region rngate      = rnactive & *rpoly;
  db::Region rnsd        = rnactive - rngate;
  db::Region rhv_ngate   = rngate & *rthickgox;
  db::Region rlv_ngate   = rngate - rhv_ngate;
  db::Region rhv_nsd     = rnsd & *rthickgox;
  db::Region rlv_nsd     = rnsd - *rthickgox;

  l2n.register_layer(rptie, "ptie");
  l2n.register_layer(rnsd,  "nsd");

  db::NetlistDeviceExtractorMOS4Transistor hvpmos_ex ("HVPMOS");
  db::NetlistDeviceExtractorMOS4Transistor hvnmos_ex ("HVNMOS");
  db::NetlistDeviceExtractorMOS4Transistor lvpmos_ex ("LVPMOS");
  db::NetlistDeviceExtractorMOS4Transistor lvnmos_ex ("LVNMOS");

  //  device extraction

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rhv_pgate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rnwell.get ();
  l2n.extract_devices (hvpmos_ex, dl);

  dl["SD"] = &rpsd;
  dl["G"] = &rlv_pgate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rnwell.get ();
  l2n.extract_devices (lvpmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rhv_ngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (hvnmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rlv_ngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (lvnmos_ex, dl);

  //  Intra-layer
  l2n.connect (rpsd);
  l2n.connect (rnsd);
  l2n.connect (*rnwell);
  l2n.connect (*rpoly);
  l2n.connect (*rcont);
  l2n.connect (*rmetal1);
  l2n.connect (*rvia1);
  l2n.connect (*rmetal2);
  l2n.connect (rptie);
  l2n.connect (rntie);
  //  Inter-layer
  l2n.connect (*rcont,      rntie);
  l2n.connect (*rcont,      rptie);
  l2n.connect (*rnwell,     rntie);
  l2n.connect (rpsd,        *rcont);
  l2n.connect (rnsd,        *rcont);
  l2n.connect (*rpoly,      *rcont);
  l2n.connect (*rcont,      *rmetal1);
  l2n.connect (*rmetal1,    *rvia1);
  l2n.connect (*rvia1,      *rmetal2);
  l2n.connect (*rpoly,      *rpoly_lbl);     //  attaches labels
  l2n.connect (*rmetal1,    *rmetal1_lbl);   //  attaches labels
  l2n.connect (*rmetal2,    *rmetal2_lbl);   //  attaches labels
  //  Global
  l2n.connect_global (rptie, "BULK");
  l2n.connect_global (*rbulk, "BULK");

  l2n.extract_netlist ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit TOP ();\n"
    "  device HVPMOS $1 (S=Z,G=$5,D=VDD2,B=$8) (L=1.5,W=4.05,AS=5.4675,AD=2.73375,PS=10.8,PD=5.4);\n"
    "  device HVPMOS $2 (S=VDD2,G=Z,D=$5,B=$8) (L=1.5,W=4.05,AS=2.73375,AD=5.4675,PS=5.4,PD=10.8);\n"
    "  device LVPMOS $3 (S=$10,G=A,D=$6,B=$9) (L=1.5,W=2.475,AS=4.77675,AD=3.155625,PS=8.81,PD=7.5);\n"
    "  device HVNMOS $4 (S=Z,G=$6,D=VSS,B=BULK) (L=1.5,W=5.25,AS=7.0875,AD=3.54375,PS=13.2,PD=6.6);\n"
    "  device HVNMOS $5 (S=VSS,G=A,D=$5,B=BULK) (L=1.5,W=5.25,AS=3.54375,AD=7.0875,PS=6.6,PD=13.2);\n"
    "  device LVNMOS $6 (S=VSS,G=A,D=$6,B=BULK) (L=1.2,W=1.7,AS=2.346,AD=2.1165,PS=6.16,PD=5.89);\n"
    "end;\n"
  );
}

TEST(7_MoreByEmptyDeviceTypes)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int nwell      = define_layer (ly, lmap, 1);
  unsigned int active     = define_layer (ly, lmap, 2);
  unsigned int thickgox   = define_layer (ly, lmap, 1003);  //  does not exist
  unsigned int pplus      = define_layer (ly, lmap, 4);
  unsigned int nplus      = define_layer (ly, lmap, 5);
  unsigned int poly       = define_layer (ly, lmap, 6);
  unsigned int poly_lbl   = define_layer (ly, lmap, 7);
  unsigned int cont       = define_layer (ly, lmap, 8);
  unsigned int metal1     = define_layer (ly, lmap, 9);
  unsigned int metal1_lbl = define_layer (ly, lmap, 10);
  unsigned int via1       = define_layer (ly, lmap, 11);
  unsigned int metal2     = define_layer (ly, lmap, 12);
  unsigned int metal2_lbl = define_layer (ly, lmap, 13);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l6.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());
  db::LayoutToNetlist l2n (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::unique_ptr<db::Region> rbulk (l2n.make_layer ("bulk"));
  std::unique_ptr<db::Region> rnwell (l2n.make_layer (nwell, "nwell"));
  std::unique_ptr<db::Region> rthickgox (l2n.make_layer (thickgox, "thickgox"));
  std::unique_ptr<db::Region> ractive (l2n.make_layer (active, "active"));
  std::unique_ptr<db::Region> rpplus (l2n.make_layer (pplus, "pplus"));
  std::unique_ptr<db::Region> rnplus (l2n.make_layer (nplus, "nplus"));
  std::unique_ptr<db::Region> rpoly (l2n.make_polygon_layer (poly, "poly"));
  std::unique_ptr<db::Texts> rpoly_lbl (l2n.make_text_layer (poly_lbl, "poly_lbl"));
  std::unique_ptr<db::Region> rcont (l2n.make_polygon_layer (cont, "cont"));
  std::unique_ptr<db::Region> rmetal1 (l2n.make_polygon_layer (metal1, "metal1"));
  std::unique_ptr<db::Texts> rmetal1_lbl (l2n.make_text_layer (metal1_lbl, "metal1_lbl"));
  std::unique_ptr<db::Region> rvia1 (l2n.make_polygon_layer (via1, "via1"));
  std::unique_ptr<db::Region> rmetal2 (l2n.make_polygon_layer (metal2, "metal2"));
  std::unique_ptr<db::Texts> rmetal2_lbl (l2n.make_text_layer (metal2_lbl, "metal2_lbl"));

  //  derived regions

  db::Region ractive_in_nwell = *ractive & *rnwell;
  db::Region rpactive    = ractive_in_nwell - *rnplus;
  db::Region rntie       = ractive_in_nwell & *rnplus;
  db::Region rpgate      = rpactive & *rpoly;
  db::Region rpsd        = rpactive - rpgate;
  db::Region rhv_pgate   = rpgate & *rthickgox;
  db::Region rlv_pgate   = rpgate - rhv_pgate;
  db::Region rhv_psd     = rpsd & *rthickgox;
  db::Region rlv_psd     = rpsd - *rthickgox;

  l2n.register_layer(rntie, "ntie");
  l2n.register_layer(rpsd,  "psd");

  db::Region ractive_outside_nwell = *ractive - *rnwell;
  db::Region rnactive    = ractive_outside_nwell - *rpplus;
  db::Region rptie       = ractive_outside_nwell & *rpplus;
  db::Region rngate      = rnactive & *rpoly;
  db::Region rnsd        = rnactive - rngate;
  db::Region rhv_ngate   = rngate & *rthickgox;
  db::Region rlv_ngate   = rngate - rhv_ngate;
  db::Region rhv_nsd     = rnsd & *rthickgox;
  db::Region rlv_nsd     = rnsd - *rthickgox;

  l2n.register_layer(rptie, "ptie");
  l2n.register_layer(rnsd,  "nsd");

  db::NetlistDeviceExtractorMOS4Transistor hvpmos_ex ("HVPMOS");
  db::NetlistDeviceExtractorMOS4Transistor hvnmos_ex ("HVNMOS");
  db::NetlistDeviceExtractorMOS4Transistor lvpmos_ex ("LVPMOS");
  db::NetlistDeviceExtractorMOS4Transistor lvnmos_ex ("LVNMOS");

  //  device extraction

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rhv_pgate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rnwell.get ();
  l2n.extract_devices (hvpmos_ex, dl);

  dl["SD"] = &rpsd;
  dl["G"] = &rlv_pgate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rnwell.get ();
  l2n.extract_devices (lvpmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rhv_ngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (hvnmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rlv_ngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (lvnmos_ex, dl);

  //  Intra-layer
  l2n.connect (rpsd);
  l2n.connect (rnsd);
  l2n.connect (*rnwell);
  l2n.connect (*rpoly);
  l2n.connect (*rcont);
  l2n.connect (*rmetal1);
  l2n.connect (*rvia1);
  l2n.connect (*rmetal2);
  l2n.connect (rptie);
  l2n.connect (rntie);
  //  Inter-layer
  l2n.connect (*rcont,      rntie);
  l2n.connect (*rcont,      rptie);
  l2n.connect (*rnwell,     rntie);
  l2n.connect (rpsd,        *rcont);
  l2n.connect (rnsd,        *rcont);
  l2n.connect (*rpoly,      *rcont);
  l2n.connect (*rcont,      *rmetal1);
  l2n.connect (*rmetal1,    *rvia1);
  l2n.connect (*rvia1,      *rmetal2);
  l2n.connect (*rpoly,      *rpoly_lbl);     //  attaches labels
  l2n.connect (*rmetal1,    *rmetal1_lbl);   //  attaches labels
  l2n.connect (*rmetal2,    *rmetal2_lbl);   //  attaches labels
  //  Global
  l2n.connect_global (rptie, "BULK");
  l2n.connect_global (*rbulk, "BULK");

  l2n.extract_netlist ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit TOP ();\n"
    "  device LVPMOS $1 (S=Z,G=$5,D=VDD2,B=$8) (L=1.5,W=4.05,AS=5.4675,AD=2.73375,PS=10.8,PD=5.4);\n"
    "  device LVPMOS $2 (S=VDD2,G=Z,D=$5,B=$8) (L=1.5,W=4.05,AS=2.73375,AD=5.4675,PS=5.4,PD=10.8);\n"
    "  device LVPMOS $3 (S=$10,G=A,D=$6,B=$9) (L=1.5,W=2.475,AS=4.77675,AD=3.155625,PS=8.81,PD=7.5);\n"
    "  device LVNMOS $4 (S=VSS,G=A,D=$6,B=BULK) (L=1.2,W=1.7,AS=2.346,AD=2.1165,PS=6.16,PD=5.89);\n"
    "  device LVNMOS $5 (S=Z,G=$6,D=VSS,B=BULK) (L=1.5,W=5.25,AS=7.0875,AD=3.54375,PS=13.2,PD=6.6);\n"
    "  device LVNMOS $6 (S=VSS,G=A,D=$5,B=BULK) (L=1.5,W=5.25,AS=3.54375,AD=7.0875,PS=6.6,PD=13.2);\n"
    "end;\n"
  );
}

TEST(8_FlatExtraction)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int nwell      = define_layer (ly, lmap, 1);
  unsigned int active     = define_layer (ly, lmap, 2);
  unsigned int thickgox   = define_layer (ly, lmap, 3);
  unsigned int pplus      = define_layer (ly, lmap, 4);
  unsigned int nplus      = define_layer (ly, lmap, 5);
  unsigned int poly       = define_layer (ly, lmap, 6);
  unsigned int poly_lbl   = define_layer (ly, lmap, 7);
  unsigned int cont       = define_layer (ly, lmap, 8);
  unsigned int metal1     = define_layer (ly, lmap, 9);
  unsigned int metal1_lbl = define_layer (ly, lmap, 10);
  unsigned int via1       = define_layer (ly, lmap, 11);
  unsigned int metal2     = define_layer (ly, lmap, 12);
  unsigned int metal2_lbl = define_layer (ly, lmap, 13);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l6.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::LayoutToNetlist l2n (ly.cell_name (tc.cell_index ()), ly.dbu ());

  std::unique_ptr<db::Region> rbulk (new db::Region ());
  std::unique_ptr<db::Region> rnwell (new db::Region (db::RecursiveShapeIterator (ly, tc, nwell)));
  std::unique_ptr<db::Region> rthickgox (new db::Region (db::RecursiveShapeIterator (ly, tc, thickgox)));
  std::unique_ptr<db::Region> ractive (new db::Region (db::RecursiveShapeIterator (ly, tc, active)));
  std::unique_ptr<db::Region> rpplus (new db::Region (db::RecursiveShapeIterator (ly, tc, pplus)));
  std::unique_ptr<db::Region> rnplus (new db::Region (db::RecursiveShapeIterator (ly, tc, nplus)));
  std::unique_ptr<db::Region> rpoly (new db::Region (db::RecursiveShapeIterator (ly, tc, poly)));
  std::unique_ptr<db::Region> rpoly_lbl (new db::Region (db::RecursiveShapeIterator (ly, tc, poly_lbl)));
  std::unique_ptr<db::Region> rcont (new db::Region (db::RecursiveShapeIterator (ly, tc, cont)));
  std::unique_ptr<db::Region> rmetal1 (new db::Region (db::RecursiveShapeIterator (ly, tc, metal1)));
  std::unique_ptr<db::Region> rmetal1_lbl (new db::Region (db::RecursiveShapeIterator (ly, tc, metal1_lbl)));
  std::unique_ptr<db::Region> rvia1 (new db::Region (db::RecursiveShapeIterator (ly, tc, via1)));
  std::unique_ptr<db::Region> rmetal2 (new db::Region (db::RecursiveShapeIterator (ly, tc, metal2)));
  std::unique_ptr<db::Region> rmetal2_lbl (new db::Region (db::RecursiveShapeIterator (ly, tc, metal2_lbl)));

  l2n.register_layer (*rbulk, "bulk");
  l2n.register_layer (*rnwell, "nwell");
  l2n.register_layer (*rthickgox, "thickgox");
  l2n.register_layer (*ractive, "active");
  l2n.register_layer (*rpplus, "pplus");
  l2n.register_layer (*rnplus, "nplus");
  l2n.register_layer (*rpoly, "poly");
  l2n.register_layer (*rpoly_lbl, "poly_lbl");
  l2n.register_layer (*rcont, "cont");
  l2n.register_layer (*rmetal1, "metal1");
  l2n.register_layer (*rmetal1_lbl, "metal1_lbl");
  l2n.register_layer (*rvia1, "via1");
  l2n.register_layer (*rmetal2, "metal2");
  l2n.register_layer (*rmetal2_lbl, "metal2_lbl");

  //  derived regions

  db::Region ractive_in_nwell = *ractive & *rnwell;
  db::Region rpactive    = ractive_in_nwell - *rnplus;
  db::Region rntie       = ractive_in_nwell & *rnplus;
  db::Region rpgate      = rpactive & *rpoly;
  db::Region rpsd        = rpactive - rpgate;
  db::Region rhv_pgate   = rpgate & *rthickgox;
  db::Region rlv_pgate   = rpgate - rhv_pgate;
  db::Region rhv_psd     = rpsd & *rthickgox;
  db::Region rlv_psd     = rpsd - *rthickgox;

  l2n.register_layer (rntie, "ntie");
  l2n.register_layer (rpsd,  "psd");
  //  required to provide deep layers for flat ones for the extractor:
  l2n.register_layer (rhv_pgate,  "hv_pgate");
  l2n.register_layer (rlv_pgate,  "lv_pgate");

  db::Region ractive_outside_nwell = *ractive - *rnwell;
  db::Region rnactive    = ractive_outside_nwell - *rpplus;
  db::Region rptie       = ractive_outside_nwell & *rpplus;
  db::Region rngate      = rnactive & *rpoly;
  db::Region rnsd        = rnactive - rngate;
  db::Region rhv_ngate   = rngate & *rthickgox;
  db::Region rlv_ngate   = rngate - rhv_ngate;
  db::Region rhv_nsd     = rnsd & *rthickgox;
  db::Region rlv_nsd     = rnsd - *rthickgox;

  l2n.register_layer (rptie, "ptie");
  l2n.register_layer (rnsd,  "nsd");
  //  required to provide deep layers for flat ones for the extractor:
  l2n.register_layer (rhv_ngate,  "hv_ngate");
  l2n.register_layer (rlv_ngate,  "lv_ngate");

  db::NetlistDeviceExtractorMOS4Transistor hvpmos_ex ("HVPMOS");
  db::NetlistDeviceExtractorMOS4Transistor hvnmos_ex ("HVNMOS");
  db::NetlistDeviceExtractorMOS4Transistor lvpmos_ex ("LVPMOS");
  db::NetlistDeviceExtractorMOS4Transistor lvnmos_ex ("LVNMOS");

  //  device extraction

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rhv_pgate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rnwell.get ();
  l2n.extract_devices (hvpmos_ex, dl);

  dl["SD"] = &rpsd;
  dl["G"] = &rlv_pgate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rnwell.get ();
  l2n.extract_devices (lvpmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rhv_ngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (hvnmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rlv_ngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (lvnmos_ex, dl);

  //  Intra-layer
  l2n.connect (rpsd);
  l2n.connect (rnsd);
  l2n.connect (*rnwell);
  l2n.connect (*rpoly);
  l2n.connect (*rcont);
  l2n.connect (*rmetal1);
  l2n.connect (*rvia1);
  l2n.connect (*rmetal2);
  l2n.connect (rptie);
  l2n.connect (rntie);
  //  Inter-layer
  l2n.connect (*rcont,      rntie);
  l2n.connect (*rcont,      rptie);
  l2n.connect (*rnwell,     rntie);
  l2n.connect (rpsd,        *rcont);
  l2n.connect (rnsd,        *rcont);
  l2n.connect (*rpoly,      *rcont);
  l2n.connect (*rcont,      *rmetal1);
  l2n.connect (*rmetal1,    *rvia1);
  l2n.connect (*rvia1,      *rmetal2);
  l2n.connect (*rpoly,      *rpoly_lbl);     //  attaches labels
  l2n.connect (*rmetal1,    *rmetal1_lbl);   //  attaches labels
  l2n.connect (*rmetal2,    *rmetal2_lbl);   //  attaches labels
  //  Global
  l2n.connect_global (rptie, "BULK");
  l2n.connect_global (*rbulk, "BULK");

  l2n.extract_netlist ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit TOP ();\n"
    "  device HVPMOS $1 (S=Z,G=$5,D=VDD2,B=$8) (L=1.5,W=4.05,AS=5.4675,AD=2.73375,PS=10.8,PD=5.4);\n"
    "  device HVPMOS $2 (S=VDD2,G=Z,D=$5,B=$8) (L=1.5,W=4.05,AS=2.73375,AD=5.4675,PS=5.4,PD=10.8);\n"
    "  device LVPMOS $3 (S=$10,G=A,D=$6,B=$9) (L=1.5,W=2.475,AS=4.77675,AD=3.155625,PS=8.81,PD=7.5);\n"
    "  device HVNMOS $4 (S=Z,G=$6,D=VSS,B=BULK) (L=1.5,W=5.25,AS=7.0875,AD=3.54375,PS=13.2,PD=6.6);\n"
    "  device HVNMOS $5 (S=VSS,G=A,D=$5,B=BULK) (L=1.5,W=5.25,AS=3.54375,AD=7.0875,PS=6.6,PD=13.2);\n"
    "  device LVNMOS $6 (S=VSS,G=A,D=$6,B=BULK) (L=1.2,W=1.7,AS=2.346,AD=2.1165,PS=6.16,PD=5.89);\n"
    "end;\n"
  );
}

TEST(9_FlatExtractionWithExternalDSS)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int nwell      = define_layer (ly, lmap, 1);
  unsigned int active     = define_layer (ly, lmap, 2);
  unsigned int thickgox   = define_layer (ly, lmap, 103);  //  does not exist
  unsigned int pplus      = define_layer (ly, lmap, 4);
  unsigned int nplus      = define_layer (ly, lmap, 5);
  unsigned int poly       = define_layer (ly, lmap, 6);
  unsigned int poly_lbl   = define_layer (ly, lmap, 7);
  unsigned int cont       = define_layer (ly, lmap, 8);
  unsigned int metal1     = define_layer (ly, lmap, 9);
  unsigned int metal1_lbl = define_layer (ly, lmap, 10);
  unsigned int via1       = define_layer (ly, lmap, 11);
  unsigned int metal2     = define_layer (ly, lmap, 12);
  unsigned int metal2_lbl = define_layer (ly, lmap, 13);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l6.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  //  NOTE: we use a DSS from a LayoutToNetlist object - this one is initialized properly
  //  with the text representation settings.
  db::LayoutToNetlist l2n_master;
  db::DeepShapeStore &dss = l2n_master.dss ();

  db::LayoutToNetlist l2n (&dss);

  std::unique_ptr<db::Region> rbulk (new db::Region ());
  std::unique_ptr<db::Region> rnwell (new db::Region (db::RecursiveShapeIterator (ly, tc, nwell), dss));
  std::unique_ptr<db::Region> rthickgox (new db::Region (db::RecursiveShapeIterator (ly, tc, thickgox), dss));
  std::unique_ptr<db::Region> ractive (new db::Region (db::RecursiveShapeIterator (ly, tc, active), dss));
  std::unique_ptr<db::Region> rpplus (new db::Region (db::RecursiveShapeIterator (ly, tc, pplus), dss));
  std::unique_ptr<db::Region> rnplus (new db::Region (db::RecursiveShapeIterator (ly, tc, nplus), dss));
  std::unique_ptr<db::Region> rpoly (new db::Region (db::RecursiveShapeIterator (ly, tc, poly), dss));
  std::unique_ptr<db::Region> rpoly_lbl (new db::Region (db::RecursiveShapeIterator (ly, tc, poly_lbl), dss));
  std::unique_ptr<db::Region> rcont (new db::Region (db::RecursiveShapeIterator (ly, tc, cont), dss));
  std::unique_ptr<db::Region> rmetal1 (new db::Region (db::RecursiveShapeIterator (ly, tc, metal1), dss));
  std::unique_ptr<db::Region> rmetal1_lbl (new db::Region (db::RecursiveShapeIterator (ly, tc, metal1_lbl), dss));
  std::unique_ptr<db::Region> rvia1 (new db::Region (db::RecursiveShapeIterator (ly, tc, via1), dss));
  std::unique_ptr<db::Region> rmetal2 (new db::Region (db::RecursiveShapeIterator (ly, tc, metal2), dss));
  std::unique_ptr<db::Region> rmetal2_lbl (new db::Region (db::RecursiveShapeIterator (ly, tc, metal2_lbl), dss));

  l2n.register_layer (*rbulk, "bulk");
  l2n.register_layer (*rnwell, "nwell");
  l2n.register_layer (*rthickgox, "thickgox");
  l2n.register_layer (*ractive, "active");
  l2n.register_layer (*rpplus, "pplus");
  l2n.register_layer (*rnplus, "nplus");
  l2n.register_layer (*rpoly, "poly");
  l2n.register_layer (*rpoly_lbl, "poly_lbl");
  l2n.register_layer (*rcont, "cont");
  l2n.register_layer (*rmetal1, "metal1");
  l2n.register_layer (*rmetal1_lbl, "metal1_lbl");
  l2n.register_layer (*rvia1, "via1");
  l2n.register_layer (*rmetal2, "metal2");
  l2n.register_layer (*rmetal2_lbl, "metal2_lbl");

  //  derived regions

  db::Region ractive_in_nwell = *ractive & *rnwell;
  db::Region rpactive    = ractive_in_nwell - *rnplus;
  db::Region rntie       = ractive_in_nwell & *rnplus;
  db::Region rpgate      = rpactive & *rpoly;
  db::Region rpsd        = rpactive - rpgate;
  db::Region rhv_pgate   = rpgate & *rthickgox;
  db::Region rlv_pgate   = rpgate - rhv_pgate;
  db::Region rhv_psd     = rpsd & *rthickgox;
  db::Region rlv_psd     = rpsd - *rthickgox;

  l2n.register_layer (rntie, "ntie");
  l2n.register_layer (rpsd,  "psd");
  //  required to provide deep layers for flat ones for the extractor:
  l2n.register_layer (rhv_pgate,  "hv_pgate");
  l2n.register_layer (rlv_pgate,  "lv_pgate");

  db::Region ractive_outside_nwell = *ractive - *rnwell;
  db::Region rnactive    = ractive_outside_nwell - *rpplus;
  db::Region rptie       = ractive_outside_nwell & *rpplus;
  db::Region rngate      = rnactive & *rpoly;
  db::Region rnsd        = rnactive - rngate;
  db::Region rhv_ngate   = rngate & *rthickgox;
  db::Region rlv_ngate   = rngate - rhv_ngate;
  db::Region rhv_nsd     = rnsd & *rthickgox;
  db::Region rlv_nsd     = rnsd - *rthickgox;

  l2n.register_layer (rptie, "ptie");
  l2n.register_layer (rnsd,  "nsd");
  //  required to provide deep layers for flat ones for the extractor:
  l2n.register_layer (rhv_ngate,  "hv_ngate");
  l2n.register_layer (rlv_ngate,  "lv_ngate");

  db::NetlistDeviceExtractorMOS4Transistor hvpmos_ex ("HVPMOS");
  db::NetlistDeviceExtractorMOS4Transistor hvnmos_ex ("HVNMOS");
  db::NetlistDeviceExtractorMOS4Transistor lvpmos_ex ("LVPMOS");
  db::NetlistDeviceExtractorMOS4Transistor lvnmos_ex ("LVNMOS");

  //  device extraction

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rhv_pgate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rnwell.get ();
  l2n.extract_devices (hvpmos_ex, dl);

  dl["SD"] = &rpsd;
  dl["G"] = &rlv_pgate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rnwell.get ();
  l2n.extract_devices (lvpmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rhv_ngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (hvnmos_ex, dl);

  dl["SD"] = &rnsd;
  dl["G"] = &rlv_ngate;
  dl["P"] = rpoly.get ();  //  not needed for extraction but to return terminal shapes
  dl["W"] = rbulk.get ();
  l2n.extract_devices (lvnmos_ex, dl);

  //  Intra-layer
  l2n.connect (rpsd);
  l2n.connect (rnsd);
  l2n.connect (*rnwell);
  l2n.connect (*rpoly);
  l2n.connect (*rcont);
  l2n.connect (*rmetal1);
  l2n.connect (*rvia1);
  l2n.connect (*rmetal2);
  l2n.connect (rptie);
  l2n.connect (rntie);
  //  Inter-layer
  l2n.connect (*rcont,      rntie);
  l2n.connect (*rcont,      rptie);
  l2n.connect (*rnwell,     rntie);
  l2n.connect (rpsd,        *rcont);
  l2n.connect (rnsd,        *rcont);
  l2n.connect (*rpoly,      *rcont);
  l2n.connect (*rcont,      *rmetal1);
  l2n.connect (*rmetal1,    *rvia1);
  l2n.connect (*rvia1,      *rmetal2);
  l2n.connect (*rpoly,      *rpoly_lbl);     //  attaches labels
  l2n.connect (*rmetal1,    *rmetal1_lbl);   //  attaches labels
  l2n.connect (*rmetal2,    *rmetal2_lbl);   //  attaches labels
  //  Global
  l2n.connect_global (rptie, "BULK");
  l2n.connect_global (*rbulk, "BULK");

  l2n.extract_netlist ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    "circuit TOP ();\n"
    "  device LVPMOS $1 (S=Z,G=$5,D=VDD2,B=$8) (L=1.5,W=4.05,AS=5.4675,AD=2.73375,PS=10.8,PD=5.4);\n"
    "  device LVPMOS $2 (S=VDD2,G=Z,D=$5,B=$8) (L=1.5,W=4.05,AS=2.73375,AD=5.4675,PS=5.4,PD=10.8);\n"
    "  device LVPMOS $3 (S=$10,G=A,D=$6,B=$9) (L=1.5,W=2.475,AS=4.77675,AD=3.155625,PS=8.81,PD=7.5);\n"
    "  device LVNMOS $4 (S=VSS,G=A,D=$6,B=BULK) (L=1.2,W=1.7,AS=2.346,AD=2.1165,PS=6.16,PD=5.89);\n"
    "  device LVNMOS $5 (S=Z,G=$6,D=VSS,B=BULK) (L=1.5,W=5.25,AS=7.0875,AD=3.54375,PS=13.2,PD=6.6);\n"
    "  device LVNMOS $6 (S=VSS,G=A,D=$5,B=BULK) (L=1.5,W=5.25,AS=3.54375,AD=7.0875,PS=6.6,PD=13.2);\n"
    "end;\n"
  );
}

TEST(10_Antenna)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int poly       = define_layer (ly, lmap, 6);
  unsigned int cont       = define_layer (ly, lmap, 8);
  unsigned int metal1     = define_layer (ly, lmap, 9);
  unsigned int via1       = define_layer (ly, lmap, 11);
  unsigned int metal2     = define_layer (ly, lmap, 12);
  unsigned int diode      = define_layer (ly, lmap, 1);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "antenna_l1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;

  std::unique_ptr<db::Region> rdiode (new db::Region (db::RecursiveShapeIterator (ly, tc, diode), dss));
  std::unique_ptr<db::Region> rpoly (new db::Region (db::RecursiveShapeIterator (ly, tc, poly), dss));
  std::unique_ptr<db::Region> rcont (new db::Region (db::RecursiveShapeIterator (ly, tc, cont), dss));
  std::unique_ptr<db::Region> rmetal1 (new db::Region (db::RecursiveShapeIterator (ly, tc, metal1), dss));
  std::unique_ptr<db::Region> rvia1 (new db::Region (db::RecursiveShapeIterator (ly, tc, via1), dss));
  std::unique_ptr<db::Region> rmetal2 (new db::Region (db::RecursiveShapeIterator (ly, tc, metal2), dss));

  db::Layout ly2;
  ly2.dbu (ly.dbu ());
  db::Cell &top2 = ly2.cell (ly2.add_cell ("TOPTOP"));

  rdiode->insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (1, 0)));
  rpoly->insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (6, 0)));
  rcont->insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (8, 0)));
  rmetal1->insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (9, 0)));
  rvia1->insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (11, 0)));
  rmetal2->insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (12, 0)));

  {
    db::LayoutToNetlist l2n (&dss);

    l2n.register_layer (*rpoly, "poly");
    l2n.register_layer (*rcont, "cont");
    l2n.register_layer (*rmetal1, "metal1");
    l2n.register_layer (*rvia1, "via1");
    l2n.register_layer (*rmetal2, "metal2");

    //  Intra-layer
    l2n.connect (*rpoly);
    l2n.connect (*rcont);
    l2n.connect (*rmetal1);
    /*  not yet:
    l2n.connect (*rvia1);
    l2n.connect (*rmetal2);
    */
    //  Inter-layer
    l2n.connect (*rpoly,      *rcont);
    l2n.connect (*rcont,      *rmetal1);
    /*  not yet:
    l2n.connect (*rmetal1,    *rvia1);
    l2n.connect (*rvia1,      *rmetal2);
    */

    l2n.extract_netlist ();

    db::Region a1_3 = l2n.antenna_check (*rpoly, *rmetal1, 3);
    db::Region a1_10 = l2n.antenna_check (*rpoly, *rmetal1, 10);
    db::Region a1_30 = l2n.antenna_check (*rpoly, *rmetal1, 30);

    a1_3.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (100, 0)));
    a1_10.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (101, 0)));
    a1_30.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (102, 0)));
  }

  {
    db::LayoutToNetlist l2n (&dss);

    l2n.register_layer (*rpoly, "poly");
    l2n.register_layer (*rcont, "cont");
    l2n.register_layer (*rmetal1, "metal1");
    l2n.register_layer (*rvia1, "via1");
    l2n.register_layer (*rmetal2, "metal2");

    //  Intra-layer
    l2n.connect (*rpoly);
    l2n.connect (*rcont);
    l2n.connect (*rmetal1);
    l2n.connect (*rvia1);
    l2n.connect (*rmetal2);
    //  Inter-layer
    l2n.connect (*rpoly,      *rcont);
    l2n.connect (*rcont,      *rmetal1);
    l2n.connect (*rmetal1,    *rvia1);
    l2n.connect (*rvia1,      *rmetal2);

    l2n.extract_netlist ();

    db::Region a2_5 = l2n.antenna_check (*rpoly, *rmetal2, 5);
    db::Region a2_10 = l2n.antenna_check (*rpoly, *rmetal2, 10);
    db::Region a2_17 = l2n.antenna_check (*rpoly, *rmetal2, 17);

    a2_5.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (200, 0)));
    a2_10.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (201, 0)));
    a2_17.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (202, 0)));
  }

  {
    db::LayoutToNetlist l2n (&dss);

    l2n.register_layer (*rdiode, "diode");
    l2n.register_layer (*rpoly, "poly");
    l2n.register_layer (*rcont, "cont");
    l2n.register_layer (*rmetal1, "metal1");

    //  Intra-layer
    l2n.connect (*rdiode);
    l2n.connect (*rpoly);
    l2n.connect (*rcont);
    l2n.connect (*rmetal1);
    //  Inter-layer
    l2n.connect (*rdiode,     *rcont);
    l2n.connect (*rpoly,      *rcont);
    l2n.connect (*rcont,      *rmetal1);

    l2n.extract_netlist ();

    std::vector<std::pair<const db::Region *, double> > diodes;
    //  8.0 means: increase r by 8.0 for each um^2 of diode attached to a net
    diodes.push_back (std::make_pair (rdiode.get (), 8.0));

    db::Region a3_3 = l2n.antenna_check (*rpoly, *rmetal1, 3, diodes);
    db::Region a3_10 = l2n.antenna_check (*rpoly, *rmetal1, 10, diodes);
    db::Region a3_30 = l2n.antenna_check (*rpoly, *rmetal1, 30, diodes);

    a3_3.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (300, 0)));
    a3_10.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (301, 0)));
    a3_30.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (302, 0)));
  }

  {
    db::LayoutToNetlist l2n (&dss);

    l2n.register_layer (*rdiode, "diode");
    l2n.register_layer (*rpoly, "poly");
    l2n.register_layer (*rcont, "cont");
    l2n.register_layer (*rmetal1, "metal1");

    //  Intra-layer
    l2n.connect (*rdiode);
    l2n.connect (*rpoly);
    l2n.connect (*rcont);
    l2n.connect (*rmetal1);
    //  Inter-layer
    l2n.connect (*rdiode,     *rcont);
    l2n.connect (*rpoly,      *rcont);
    l2n.connect (*rcont,      *rmetal1);

    l2n.extract_netlist ();

    std::vector<std::pair<const db::Region *, double> > diodes;
    //  0.0 means: skip all nets where there is a rdiode attached
    diodes.push_back (std::make_pair (rdiode.get (), 0.0));

    db::Region a4_3 = l2n.antenna_check (*rpoly, *rmetal1, 3, diodes);
    db::Region a4_10 = l2n.antenna_check (*rpoly, *rmetal1, 10, diodes);
    db::Region a4_30 = l2n.antenna_check (*rpoly, *rmetal1, 30, diodes);

    a4_3.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (400, 0)));
    a4_10.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (401, 0)));
    a4_30.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (402, 0)));
  }

  {
    db::LayoutToNetlist l2n (&dss);

    l2n.register_layer (*rpoly, "poly");
    l2n.register_layer (*rcont, "cont");
    l2n.register_layer (*rmetal1, "metal1");
    l2n.register_layer (*rvia1, "via1");
    l2n.register_layer (*rmetal2, "metal2");

    //  Intra-layer
    l2n.connect (*rpoly);
    l2n.connect (*rcont);
    l2n.connect (*rmetal1);
    l2n.connect (*rvia1);
    l2n.connect (*rmetal2);
    //  Inter-layer
    l2n.connect (*rpoly,      *rcont);
    l2n.connect (*rcont,      *rmetal1);
    l2n.connect (*rmetal1,    *rvia1);
    l2n.connect (*rvia1,      *rmetal2);

    l2n.extract_netlist ();

    db::Region a5_5 = l2n.antenna_check (*rpoly, 0.0, *rmetal2, 1.0, 5);
    db::Region a5_15 = l2n.antenna_check (*rpoly, 0.0, *rmetal2, 1.0, 15);
    db::Region a5_29 = l2n.antenna_check (*rpoly, 0.0, *rmetal2, 1.0, 29);

    a5_5.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (500, 0)));
    a5_15.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (501, 0)));
    a5_29.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (502, 0)));

    //  with area factor
    db::Region b5_5 = l2n.antenna_check (*rpoly, 2.0, 0.0, *rmetal2, 1.0, 1.0, 2.5);
    db::Region b5_15 = l2n.antenna_check (*rpoly, 2.0, 0.0, *rmetal2, 1.0, 1.0, 7.5);
    db::Region b5_29 = l2n.antenna_check (*rpoly, 2.0, 0.0, *rmetal2, 1.0, 1.0, 14.5);

    b5_5.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (550, 0)));
    b5_15.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (551, 0)));
    b5_29.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (552, 0)));
  }

  {
    db::LayoutToNetlist l2n (&dss);

    l2n.register_layer (*rpoly, "poly");
    l2n.register_layer (*rcont, "cont");
    l2n.register_layer (*rmetal1, "metal1");
    l2n.register_layer (*rvia1, "via1");
    l2n.register_layer (*rmetal2, "metal2");

    //  Intra-layer
    l2n.connect (*rpoly);
    l2n.connect (*rcont);
    l2n.connect (*rmetal1);
    l2n.connect (*rvia1);
    l2n.connect (*rmetal2);
    //  Inter-layer
    l2n.connect (*rpoly,      *rcont);
    l2n.connect (*rcont,      *rmetal1);
    l2n.connect (*rmetal1,    *rvia1);
    l2n.connect (*rvia1,      *rmetal2);

    l2n.extract_netlist ();

    db::Region a6_3 = l2n.antenna_check (*rpoly, 0.3, *rmetal2, 0.0, 3);
    db::Region a6_5 = l2n.antenna_check (*rpoly, 0.3, *rmetal2, 0.0, 5);
    db::Region a6_9 = l2n.antenna_check (*rpoly, 0.3, *rmetal2, 0.0, 9);

    a6_3.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (600, 0)));
    a6_5.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (601, 0)));
    a6_9.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (602, 0)));

    //  with area factor
    db::Region b6_3 = l2n.antenna_check (*rpoly, 1.0, 0.3, *rmetal2, 2.0, 0.0, 6);
    db::Region b6_5 = l2n.antenna_check (*rpoly, 1.0, 0.3, *rmetal2, 2.0, 0.0, 10);
    db::Region b6_9 = l2n.antenna_check (*rpoly, 1.0, 0.3, *rmetal2, 2.0, 0.0, 18);

    b6_3.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (650, 0)));
    b6_5.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (651, 0)));
    b6_9.insert_into (&ly2, top2.cell_index (), ly2.insert_layer (db::LayerProperties (652, 0)));
  }

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "antenna_au1.gds");

  db::compare_layouts (_this, ly2, au);
}

TEST(11_DuplicateInstances)
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
    fn = tl::combine_path (fn, "device_extract_l1_dup_inst.gds");

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

  db::Region rnactive = *ractive - *rnwell;
  db::Region rngate   = rnactive & *rpoly;
  db::Region rnsd     = rnactive - rngate;

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

  l2n.register_layer (rpsd, "psd");
  l2n.register_layer (rnsd, "nsd");

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

  //  debug layers produced for nets
  //    202/0 -> Active
  //    203/0 -> Poly
  //    204/0 -> Diffusion contacts
  //    205/0 -> Poly contacts
  //    206/0 -> Metal1
  //    207/0 -> Via1
  //    208/0 -> Metal2
  //    210/0 -> N source/drain
  //    211/0 -> P source/drain
  std::map<const db::Region *, unsigned int> dump_map;
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = l2n.cell_mapping_into (ly, tc, true /*with device cells*/);
  dump_nets_to_layout (l2n, ly, dump_map, cm);

  dump_map.clear ();
  dump_map [&rpsd            ] = ly.insert_layer (db::LayerProperties (310, 0));
  dump_map [&rnsd            ] = ly.insert_layer (db::LayerProperties (311, 0));
  dump_map [rpoly.get ()     ] = ly.insert_layer (db::LayerProperties (303, 0));
  dump_map [rdiff_cont.get ()] = ly.insert_layer (db::LayerProperties (304, 0));
  dump_map [rpoly_cont.get ()] = ly.insert_layer (db::LayerProperties (305, 0));
  dump_map [rmetal1.get ()   ] = ly.insert_layer (db::LayerProperties (306, 0));
  dump_map [rvia1.get ()     ] = ly.insert_layer (db::LayerProperties (307, 0));
  dump_map [rmetal2.get ()   ] = ly.insert_layer (db::LayerProperties (308, 0));

  dump_recursive_nets_to_layout (l2n, ly, dump_map, cm);

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1_dup_inst_with_rec_nets.gds");

  db::compare_layouts (_this, ly, au);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n.netlist (),
    //  suppressing duplicate cells:
    "circuit RINGO ();\n"
    "  subcircuit INV2 $1 (IN=$I12,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $2 (IN=FB,$2=$I25,OUT=$I1,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $3 (IN=$I1,$2=$I26,OUT=$I2,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $4 (IN=$I2,$2=$I27,OUT=$I3,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $5 (IN=$I3,$2=$I28,OUT=$I4,$4=VSS,$5=VDD);\n"
    "  subcircuit BLOCK $6 ($1=$I29,$2=VDD,$3=VSS,$4=$I11,$5=$I5,$6=$I4,$7=$I12);\n"
    "  subcircuit INV2 $7 (IN=$I11,$2=$I29,OUT=$I5,$4=VSS,$5=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit TRANS $4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n"
    "circuit BLOCK ($1=$I21,$2=$I14,$3=$I10,$4=$I9,$5=$I6,$6=$I5,$7=$I3);\n"
    "  subcircuit INV2 $1 (IN=$I6,$2=$I22,OUT=$I1,$4=$I10,$5=$I14);\n"
    "  subcircuit INV2 $2 (IN=$I9,$2=$I21,OUT=$I6,$4=$I10,$5=$I14);\n"
    "  subcircuit INV2 $3 (IN=$I2,$2=$I20,OUT=$I3,$4=$I10,$5=$I14);\n"
    "  subcircuit INV2 $4 (IN=$I1,$2=$I19,OUT=$I2,$4=$I10,$5=$I14);\n"
    "  subcircuit INV2 $5 (IN=$I5,$2=$I18,OUT=$I9,$4=$I10,$5=$I14);\n"
    "end;\n"
  );
}

namespace
{

class TestRig
{
public:
  TestRig (db::Layout &ly)
    : m_ly (ly)
  {
    nwell      = define_layer (m_ly, m_lmap, 1);
    nwell_lbl  = define_layer (m_ly, m_lmap, 1, 1);
    active     = define_layer (m_ly, m_lmap, 2);
    pplus      = define_layer (m_ly, m_lmap, 10);
    nplus      = define_layer (m_ly, m_lmap, 11);
    poly       = define_layer (m_ly, m_lmap, 3);
    poly_lbl   = define_layer (m_ly, m_lmap, 3, 1);
    diff_cont  = define_layer (m_ly, m_lmap, 4);
    poly_cont  = define_layer (m_ly, m_lmap, 5);
    metal1     = define_layer (m_ly, m_lmap, 6);
    metal1_lbl = define_layer (m_ly, m_lmap, 6, 1);
    via1       = define_layer (m_ly, m_lmap, 7);
    metal2     = define_layer (m_ly, m_lmap, 8);
    metal2_lbl = define_layer (m_ly, m_lmap, 8, 1);
  }

  db::LayoutToNetlist *
  make_l2n ()
  {
    db::Cell &tc = m_ly.cell (*m_ly.begin_top_down ());

    std::unique_ptr<db::LayoutToNetlist> l2n_p (new db::LayoutToNetlist (db::RecursiveShapeIterator (m_ly, tc, std::set<unsigned int> ())));
    db::LayoutToNetlist &l2n = *l2n_p;

    rbulk.reset (l2n.make_layer ("bulk"));
    rnwell.reset (l2n.make_layer (nwell, "nwell"));
    rnwell_lbl.reset (l2n.make_layer (nwell_lbl, "nwell_lbl"));
    ractive.reset (l2n.make_layer (active, "active"));
    rpplus.reset (l2n.make_layer (pplus, "pplus"));
    rnplus.reset (l2n.make_layer (nplus, "nplus"));
    rpoly.reset (l2n.make_polygon_layer (poly, "poly"));
    rpoly_lbl.reset (l2n.make_text_layer (poly_lbl, "poly_lbl"));
    rdiff_cont.reset (l2n.make_polygon_layer (diff_cont, "diff_cont"));
    rpoly_cont.reset (l2n.make_polygon_layer (poly_cont, "poly_cont"));
    rmetal1.reset (l2n.make_polygon_layer (metal1, "metal1"));
    rmetal1_lbl.reset (l2n.make_text_layer (metal1_lbl, "metal1_lbl"));
    rvia1.reset (l2n.make_polygon_layer (via1, "via1"));
    rmetal2.reset (l2n.make_polygon_layer (metal2, "metal2"));
    rmetal2_lbl.reset (l2n.make_text_layer (metal2_lbl, "metal2_lbl"));

    //  derived regions

    ractive_in_nwell = *ractive & *rnwell;
    rpactive = ractive_in_nwell & *rpplus;
    rntie    = ractive_in_nwell & *rnplus;
    rpgate   = rpactive & *rpoly;
    rpsd     = rpactive - rpgate;

    ractive_outside_nwell = *ractive - *rnwell;
    rnactive = ractive_outside_nwell & *rnplus;
    rptie    = ractive_outside_nwell & *rpplus;
    rngate   = rnactive & *rpoly;
    rnsd     = rnactive - rngate;

    //  return the computed layers into the original layout and write it for debugging purposes

    unsigned int lgate  = m_ly.insert_layer (db::LayerProperties (20, 0));      // 20/0 -> Gate
    unsigned int lsd    = m_ly.insert_layer (db::LayerProperties (21, 0));      // 21/0 -> Source/Drain
    unsigned int lpdiff = m_ly.insert_layer (db::LayerProperties (22, 0));      // 22/0 -> P Diffusion
    unsigned int lndiff = m_ly.insert_layer (db::LayerProperties (23, 0));      // 23/0 -> N Diffusion
    unsigned int lptie  = m_ly.insert_layer (db::LayerProperties (24, 0));      // 24/0 -> P Tie
    unsigned int lntie  = m_ly.insert_layer (db::LayerProperties (25, 0));      // 25/0 -> N Tie

    rpgate.insert_into (&m_ly, tc.cell_index (), lgate);
    rngate.insert_into (&m_ly, tc.cell_index (), lgate);
    rpsd.insert_into (&m_ly, tc.cell_index (), lsd);
    rnsd.insert_into (&m_ly, tc.cell_index (), lsd);
    rpsd.insert_into (&m_ly, tc.cell_index (), lpdiff);
    rnsd.insert_into (&m_ly, tc.cell_index (), lndiff);
    rpsd.insert_into (&m_ly, tc.cell_index (), lptie);
    rnsd.insert_into (&m_ly, tc.cell_index (), lntie);

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

    l2n.register_layer (rpsd, "psd");
    l2n.register_layer (rnsd, "nsd");
    l2n.register_layer (rptie, "ptie");
    l2n.register_layer (rntie, "ntie");

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
    l2n.connect (rntie,       *rnwell);
    l2n.connect (*rpoly,      *rpoly_cont);
    l2n.connect (*rpoly_cont, *rmetal1);
    l2n.connect (*rdiff_cont, *rmetal1);
    l2n.connect (*rdiff_cont, rptie);
    l2n.connect (*rdiff_cont, rntie);
    l2n.connect (*rnwell,     rntie);
    l2n.connect (*rmetal1,    *rvia1);
    l2n.connect (*rvia1,      *rmetal2);
    l2n.connect (*rnwell,     *rnwell_lbl);    //  attaches labels
    l2n.connect (*rpoly,      *rpoly_lbl);     //  attaches labels
    l2n.connect (*rmetal1,    *rmetal1_lbl);   //  attaches labels
    l2n.connect (*rmetal2,    *rmetal2_lbl);   //  attaches labels

    return l2n_p.release ();
  }

  void dump_nets (db::LayoutToNetlist &l2n)
  {
    //  debug layers produced for nets
    //    201/0 -> Well
    //    203/0 -> Poly
    //    204/0 -> Diffusion contacts
    //    205/0 -> Poly contacts
    //    206/0 -> Metal1
    //    207/0 -> Via1
    //    208/0 -> Metal2
    //    210/0 -> N source/drain
    //    211/0 -> P source/drain
    //    212/0 -> N tie
    //    213/0 -> P tie
    std::map<const db::Region *, unsigned int> dump_map;
    dump_map [&rpsd            ] = m_ly.insert_layer (db::LayerProperties (210, 0));
    dump_map [&rnsd            ] = m_ly.insert_layer (db::LayerProperties (211, 0));
    dump_map [&rptie           ] = m_ly.insert_layer (db::LayerProperties (212, 0));
    dump_map [&rntie           ] = m_ly.insert_layer (db::LayerProperties (213, 0));
    dump_map [rbulk.get ()     ] = m_ly.insert_layer (db::LayerProperties (214, 0));
    dump_map [rnwell.get ()    ] = m_ly.insert_layer (db::LayerProperties (201, 0));
    dump_map [rpoly.get ()     ] = m_ly.insert_layer (db::LayerProperties (203, 0));
    dump_map [rdiff_cont.get ()] = m_ly.insert_layer (db::LayerProperties (204, 0));
    dump_map [rpoly_cont.get ()] = m_ly.insert_layer (db::LayerProperties (205, 0));
    dump_map [rmetal1.get ()   ] = m_ly.insert_layer (db::LayerProperties (206, 0));
    dump_map [rvia1.get ()     ] = m_ly.insert_layer (db::LayerProperties (207, 0));
    dump_map [rmetal2.get ()   ] = m_ly.insert_layer (db::LayerProperties (208, 0));

    //  write nets to layout
    db::Cell &tc = m_ly.cell (*m_ly.begin_top_down ());
    db::CellMapping cm = l2n.cell_mapping_into (m_ly, tc);
    dump_nets_to_layout (l2n, m_ly, dump_map, cm);
  }

  void dump_nets_recursive (db::LayoutToNetlist &l2n)
  {
    //  debug layers produced for nets
    //    301/0 -> Well
    //    303/0 -> Poly
    //    304/0 -> Diffusion contacts
    //    305/0 -> Poly contacts
    //    306/0 -> Metal1
    //    307/0 -> Via1
    //    308/0 -> Metal2
    //    310/0 -> N source/drain
    //    311/0 -> P source/drain
    //    312/0 -> N tie
    //    313/0 -> P tie
    std::map<const db::Region *, unsigned int> dump_map;
    dump_map [&rpsd            ] = m_ly.insert_layer (db::LayerProperties (310, 0));
    dump_map [&rnsd            ] = m_ly.insert_layer (db::LayerProperties (311, 0));
    dump_map [&rptie           ] = m_ly.insert_layer (db::LayerProperties (312, 0));
    dump_map [&rntie           ] = m_ly.insert_layer (db::LayerProperties (313, 0));
    dump_map [rbulk.get ()     ] = m_ly.insert_layer (db::LayerProperties (314, 0));
    dump_map [rnwell.get ()    ] = m_ly.insert_layer (db::LayerProperties (301, 0));
    dump_map [rpoly.get ()     ] = m_ly.insert_layer (db::LayerProperties (303, 0));
    dump_map [rdiff_cont.get ()] = m_ly.insert_layer (db::LayerProperties (304, 0));
    dump_map [rpoly_cont.get ()] = m_ly.insert_layer (db::LayerProperties (305, 0));
    dump_map [rmetal1.get ()   ] = m_ly.insert_layer (db::LayerProperties (306, 0));
    dump_map [rvia1.get ()     ] = m_ly.insert_layer (db::LayerProperties (307, 0));
    dump_map [rmetal2.get ()   ] = m_ly.insert_layer (db::LayerProperties (308, 0));

    //  write nets to layout
    db::Cell &tc = m_ly.cell (*m_ly.begin_top_down ());
    db::CellMapping cm = l2n.cell_mapping_into (m_ly, tc);
    dump_recursive_nets_to_layout (l2n, m_ly, dump_map, cm);
  }

  const db::LayerMap &lmap () const { return m_lmap; }

private:
  db::Layout &m_ly;
  db::LayerMap m_lmap;

public:
  unsigned int nwell;
  unsigned int nwell_lbl;
  unsigned int active;
  unsigned int pplus;
  unsigned int nplus;
  unsigned int poly;
  unsigned int poly_lbl;
  unsigned int diff_cont;
  unsigned int poly_cont;
  unsigned int metal1;
  unsigned int metal1_lbl;
  unsigned int via1;
  unsigned int metal2;
  unsigned int metal2_lbl;
  std::unique_ptr<db::Region> rbulk;
  std::unique_ptr<db::Region> rnwell;
  std::unique_ptr<db::Region> rnwell_lbl;
  std::unique_ptr<db::Region> ractive;
  std::unique_ptr<db::Region> rpplus;
  std::unique_ptr<db::Region> rnplus;
  std::unique_ptr<db::Region> rpoly;
  std::unique_ptr<db::Texts> rpoly_lbl;
  std::unique_ptr<db::Region> rdiff_cont;
  std::unique_ptr<db::Region> rpoly_cont;
  std::unique_ptr<db::Region> rmetal1;
  std::unique_ptr<db::Texts> rmetal1_lbl;
  std::unique_ptr<db::Region> rvia1;
  std::unique_ptr<db::Region> rmetal2;
  std::unique_ptr<db::Texts> rmetal2_lbl;
  db::Region ractive_in_nwell;
  db::Region rpactive;
  db::Region rntie;
  db::Region rpgate;
  db::Region rpsd;
  db::Region ractive_outside_nwell;
  db::Region rnactive;
  db::Region rptie;
  db::Region rngate;
  db::Region rnsd;
};

}

TEST(12_FlattenCircuitDoesFlattenLayout)
{
  db::Layout ly;
  TestRig test_rig (ly);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = test_rig.lmap ();
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l5.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  std::unique_ptr<db::LayoutToNetlist> l2n (test_rig.make_l2n ());

  //  Global
  l2n->connect_global (test_rig.rptie, "BULK");
  l2n->connect_global (*test_rig.rbulk, "BULK");

  //  Extract with joining VSS and VDD
  l2n->join_net_names (tl::GlobPattern ("{VSS,VDD}"));

  //  create some mess - we have to keep references to the layers to make them not disappear
  test_rig.rmetal1_lbl.reset (0);
  test_rig.rmetal2_lbl.reset (0);
  test_rig.rpoly_lbl.reset (0);

  l2n->extract_netlist ();

  l2n->netlist ()->flatten_circuit (l2n->netlist ()->circuit_by_name ("INV2"));
  l2n->netlist ()->flatten_circuit (l2n->netlist ()->circuit_by_name ("INV2PAIR"));
  l2n->netlist ()->flatten_circuit (l2n->netlist ()->circuit_by_name ("TRANS"));

  test_rig.dump_nets (*l2n);
  test_rig.dump_nets_recursive (*l2n);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n->netlist (),
    "circuit RINGO ();\n"
    "  device PMOS $1 (S=FB,G=$I7,D=VDD,B=VDD) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $2 (S=VDD,G=$I7,D=FB,B=VDD) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $3 (S=FB,G=$I7,D=VSS,B=VSS) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $4 (S=VSS,G=$I7,D=FB,B=VSS) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device PMOS $5 (S=OSC,G=FB,D=VDD,B=VDD) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $6 (S=VDD,G=FB,D=OSC,B=VDD) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $7 (S=OSC,G=FB,D=VSS,B=VSS) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $8 (S=VSS,G=FB,D=OSC,B=VSS) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device PMOS $9 (S=$I22,G=FB,D=VDD,B=VDD) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $10 (S=VDD,G=FB,D=$I22,B=VDD) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $11 (S=$I22,G=FB,D=VSS,B=VSS) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $12 (S=VSS,G=FB,D=$I22,B=VSS) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device PMOS $13 (S=$I13,G=$I22,D=VDD,B=VDD) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $14 (S=VDD,G=$I22,D=$I13,B=VDD) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $15 (S=$I13,G=$I22,D=VSS,B=VSS) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $16 (S=VSS,G=$I22,D=$I13,B=VSS) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device PMOS $17 (S=$I23,G=$I13,D=VDD,B=VDD) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $18 (S=VDD,G=$I13,D=$I23,B=VDD) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $19 (S=$I23,G=$I13,D=VSS,B=VSS) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $20 (S=VSS,G=$I13,D=$I23,B=VSS) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device PMOS $21 (S=$I5,G=$I23,D=VDD,B=VDD) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $22 (S=VDD,G=$I23,D=$I5,B=VDD) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $23 (S=$I5,G=$I23,D=VSS,B=VSS) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $24 (S=VSS,G=$I23,D=$I5,B=VSS) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device PMOS $25 (S=$I24,G=$I5,D=VDD,B=VDD) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $26 (S=VDD,G=$I5,D=$I24,B=VDD) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $27 (S=$I24,G=$I5,D=VSS,B=VSS) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $28 (S=VSS,G=$I5,D=$I24,B=VSS) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device PMOS $29 (S=$I6,G=$I24,D=VDD,B=VDD) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $30 (S=VDD,G=$I24,D=$I6,B=VDD) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $31 (S=$I6,G=$I24,D=VSS,B=VSS) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $32 (S=VSS,G=$I24,D=$I6,B=VSS) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device PMOS $33 (S=$I25,G=$I6,D=VDD,B=VDD) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $34 (S=VDD,G=$I6,D=$I25,B=VDD) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $35 (S=$I25,G=$I6,D=VSS,B=VSS) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $36 (S=VSS,G=$I6,D=$I25,B=VSS) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device PMOS $37 (S=$I7,G=$I25,D=VDD,B=VDD) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device PMOS $38 (S=VDD,G=$I25,D=$I7,B=VDD) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "  device NMOS $39 (S=$I7,G=$I25,D=VSS,B=VSS) (L=0.25,W=1.75,AS=0.91875,AD=0.48125,PS=4.55,PD=2.3);\n"
    "  device NMOS $40 (S=VSS,G=$I25,D=$I7,B=VSS) (L=0.25,W=1.75,AS=0.48125,AD=0.91875,PS=2.3,PD=4.55);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au5_flattened_circuits.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(13_JoinNetNames)
{
  db::Layout ly;
  TestRig test_rig (ly);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = test_rig.lmap ();
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l13.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  std::unique_ptr<db::LayoutToNetlist> l2n (test_rig.make_l2n ());

  //  Global
  l2n->connect_global (test_rig.rntie, "VDD");
  l2n->connect_global (*test_rig.rnwell, "VDD");
  l2n->connect_global (test_rig.rptie, "VSS");
  l2n->connect_global (*test_rig.rbulk, "VSS");

  //  Extract with joining VSS and VDD
  l2n->join_net_names (tl::GlobPattern ("{VSS,VDD}"));

  l2n->extract_netlist ();

  test_rig.dump_nets (*l2n);
  test_rig.dump_nets_recursive (*l2n);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n->netlist (),
    "circuit RINGO ();\n"
    "  subcircuit INV2 $1 (IN=$I7,$2=FB,OUT=OSC,$4=VSS,$5=VDD,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2 $2 (IN=FB,$2=$I34,OUT=$I17,$4=VSS,$5=VDD,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2 $3 (IN=$3,$2=$I38,OUT=$I4,$4=VSS,$5=VDD,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2 $4 (IN=$I2,$2=$I37,OUT=$3,$4=VSS,$5=VDD,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2 $5 (IN=$I4,$2=$I39,OUT=$I5,$4=VSS,$5=VDD,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2 $6 (IN=$I5,$2=$I40,OUT=$I6,$4=VSS,$5=VDD,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2 $7 (IN=$I6,$2=$I41,OUT=$I7,$4=VSS,$5=VDD,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2 $8 (IN=$I17,$2=$I35,OUT=$I1,$4=VSS,$5=VDD,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INV2 $9 (IN=$I1,$2=$I36,OUT=$I2,$4=VSS,$5=VDD,VDD=VDD,VSS=VSS);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5,VDD=VDD,VSS=VSS);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5,B=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT,B=VDD) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4,B=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT,B=VSS) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit TRANS $4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au13_circuits.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(14_JoinNets)
{
  db::Layout ly;
  TestRig test_rig (ly);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = test_rig.lmap ();
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l14.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  std::unique_ptr<db::LayoutToNetlist> l2n (test_rig.make_l2n ());

  //  Global
  l2n->connect_global (*test_rig.rbulk, "BULK");
  l2n->connect_global (test_rig.rptie, "BULK");

  //  Extract while joining VSS with BULK and VDD with NWELL
  std::set<std::string> jn;
  jn.insert ("VDD");
  jn.insert ("NWELL");
  l2n->join_nets (tl::GlobPattern ("INV2"), jn);
  jn.clear ();
  jn.insert ("VSS");
  jn.insert ("BULK");
  l2n->join_nets (tl::GlobPattern ("INV2"), jn);

  //  Implicit connection of nets with same name "VDD"
  jn.clear ();
  jn.insert ("VDD");
  l2n->join_nets (jn);

  l2n->extract_netlist ();

  test_rig.dump_nets (*l2n);
  test_rig.dump_nets_recursive (*l2n);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, *l2n->netlist (),
    "circuit RINGO ();\n"
    "  subcircuit INV2 $1 ('NWELL,VDD'=VDD,IN=$I15,$3=FB,OUT=OSC,VSS=VSS);\n"
    "  subcircuit INV2 $2 ('NWELL,VDD'=VDD,IN=FB,$3=$I26,OUT=$I25,VSS=VSS);\n"
    "  subcircuit INV2 $3 ('NWELL,VDD'=VDD,IN=$3,$3=$I30,OUT=$I12,VSS=VSS);\n"
    "  subcircuit INV2 $4 ('NWELL,VDD'=VDD,IN=$I10,$3=$I29,OUT=$3,VSS=VSS);\n"
    "  subcircuit INV2 $5 ('NWELL,VDD'=VDD,IN=$I12,$3=$I31,OUT=$I13,VSS=VSS);\n"
    "  subcircuit INV2 $6 ('NWELL,VDD'=VDD,IN=$I13,$3=$I32,OUT=$I14,VSS=VSS);\n"
    "  subcircuit INV2 $7 ('NWELL,VDD'=VDD,IN=$I14,$3=$I33,OUT=$I15,VSS=VSS);\n"
    "  subcircuit INV2 $8 ('NWELL,VDD'=VDD,IN=$I25,$3=$I27,OUT=$I9,VSS=VSS);\n"
    "  subcircuit INV2 $9 ('NWELL,VDD'=VDD,IN=$I9,$3=$I28,OUT=$I10,VSS=VSS);\n"
    "end;\n"
    "circuit INV2 ('NWELL,VDD'='NWELL,VDD',IN=IN,$3=$3,OUT=OUT,VSS=VSS);\n"
    "  device PMOS $1 (S=$3,G=IN,D='NWELL,VDD',B='NWELL,VDD') (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S='NWELL,VDD',G=$3,D=OUT,B='NWELL,VDD') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$3,G=IN,D=VSS,B=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$3,D=OUT,B=VSS) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$3,$2='NWELL,VDD',$3=IN);\n"
    "  subcircuit TRANS $2 ($1='NWELL,VDD',$2=OUT,$3=$3);\n"
    "  subcircuit TRANS $3 ($1=$3,$2=VSS,$3=IN);\n"
    "  subcircuit TRANS $4 ($1=VSS,$2=OUT,$3=$3);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au14_circuits.gds");

  db::compare_layouts (_this, ly, au);
}


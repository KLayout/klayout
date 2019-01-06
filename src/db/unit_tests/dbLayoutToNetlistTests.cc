
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

namespace
{

static std::string device_name (const db::Device &device)
{
  if (device.name ().empty ()) {
    return "$" + tl::to_string (device.id ());
  } else {
    return device.name ();
  }
}

class MOSFETExtractor
  : public db::NetlistDeviceExtractorMOS3Transistor
{
public:
  MOSFETExtractor (const std::string &name, db::Layout *debug_out)
    : db::NetlistDeviceExtractorMOS3Transistor (name), mp_debug_out (debug_out), m_ldiff (0), m_lgate (0)
  {
    if (mp_debug_out) {
      m_ldiff = mp_debug_out->insert_layer (db::LayerProperties (100, 0));
      m_lgate = mp_debug_out->insert_layer (db::LayerProperties (101, 0));
    }
  }

private:
  db::Layout *mp_debug_out;
  unsigned int m_ldiff, m_lgate;

  void device_out (const db::Device *device, const db::Region &diff, const db::Region &gate)
  {
    if (! mp_debug_out) {
      return;
    }

    std::string cn = layout ()->cell_name (cell_index ());
    std::pair<bool, db::cell_index_type> target_cp = mp_debug_out->cell_by_name (cn.c_str ());
    tl_assert (target_cp.first);

    db::cell_index_type dci = mp_debug_out->add_cell ((device->device_class ()->name () + "_" + device->circuit ()->name () + "_" + device_name (*device)).c_str ());
    mp_debug_out->cell (target_cp.second).insert (db::CellInstArray (db::CellInst (dci), db::Trans ()));

    db::Cell &device_cell = mp_debug_out->cell (dci);
    for (db::Region::const_iterator p = diff.begin (); ! p.at_end (); ++p) {
      device_cell.shapes (m_ldiff).insert (*p);
    }
    for (db::Region::const_iterator p = gate.begin (); ! p.at_end (); ++p) {
      device_cell.shapes (m_lgate).insert (*p);
    }

    std::string ps;
    const std::vector<db::DeviceParameterDefinition> &pd = device->device_class ()->parameter_definitions ();
    for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
      if (! ps.empty ()) {
        ps += ",";
      }
      ps += i->name () + "=" + tl::to_string (device->parameter_value (i->id ()));
    }
    device_cell.shapes (m_ldiff).insert (db::Text (ps, db::Trans (diff.bbox ().center () - db::Point ())));
  }
};

}

static void dump_nets_to_layout (const db::LayoutToNetlist &l2n, db::Layout &ly, const std::map<const db::Region *, unsigned int> &lmap, const db::CellMapping &cmap)
{
  const db::Netlist &nl = *l2n.netlist ();
  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {

    db::Cell &cell = ly.cell (cmap.cell_mapping (c->cell_index ()));

    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {

      db::cell_index_type nci = std::numeric_limits<db::cell_index_type>::max ();

      for (std::map<const db::Region *, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end (); ++m) {

        std::auto_ptr<db::Region> shapes (l2n.shapes_of_net (*n, *m->first, false));
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
      if (c->is_external_net (n.operator-> ())) {
        continue;
      }

      bool any = false;
      for (std::map<const db::Region *, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end () && !any; ++m) {
        any = !db::recursive_cluster_shape_iterator<db::PolygonRef> (l2n.net_clusters (), l2n.layer_of (*m->first), c->cell_index (), n->cluster_id ()).at_end ();
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

//  TODO: may be useful elsewhere?
static std::string qnet_name (const db::Net *net)
{
  return net ? net->qname () : "(null)";
}

static unsigned int define_layer (db::Layout &ly, db::LayerMap &lmap, int gds_layer, int gds_datatype = 0)
{
  unsigned int lid = ly.insert_layer (db::LayerProperties (gds_layer, gds_datatype));
  lmap.map (ly.get_properties (lid), lid);
  return lid;
}

TEST(1_Basic)
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

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());
  db::LayoutToNetlist l2n (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::auto_ptr<db::Region> rnwell (l2n.make_layer (nwell));
  std::auto_ptr<db::Region> ractive (l2n.make_layer (active));
  std::auto_ptr<db::Region> rpoly (l2n.make_polygon_layer (poly));
  std::auto_ptr<db::Region> rpoly_lbl (l2n.make_text_layer (poly_lbl));
  std::auto_ptr<db::Region> rdiff_cont (l2n.make_polygon_layer (diff_cont));
  std::auto_ptr<db::Region> rpoly_cont (l2n.make_polygon_layer (poly_cont));
  std::auto_ptr<db::Region> rmetal1 (l2n.make_polygon_layer (metal1));
  std::auto_ptr<db::Region> rmetal1_lbl (l2n.make_text_layer (metal1_lbl));
  std::auto_ptr<db::Region> rvia1 (l2n.make_polygon_layer (via1));
  std::auto_ptr<db::Region> rmetal2 (l2n.make_polygon_layer (metal2));
  std::auto_ptr<db::Region> rmetal2_lbl (l2n.make_text_layer (metal2_lbl));

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

  //  NOTE: the device extractor will add more debug layers for the transistors:
  //    20/0 -> Diffusion
  //    21/0 -> Gate
  MOSFETExtractor pmos_ex ("PMOS", &ly);
  MOSFETExtractor nmos_ex ("NMOS", &ly);

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
  EXPECT_EQ (l2n.netlist ()->to_string (),
    "Circuit RINGO ():\n"
    "  XINV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD)\n"
    "  XINV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD)\n"
    "  XINV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD)\n"
    "  XINV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD)\n"
    "  XINV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD)\n"
    "  XINV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD)\n"
    "  XINV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD)\n"
    "  XINV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD)\n"
    "  XINV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD)\n"
    "  XINV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD)\n"
    "Circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5):\n"
    "  DPMOS $1 (S=$2,G=IN,D=$5) [L=0.25,W=0.95,AS=0.49875,AD=0.26125]\n"
    "  DPMOS $2 (S=$5,G=$2,D=OUT) [L=0.25,W=0.95,AS=0.26125,AD=0.49875]\n"
    "  DNMOS $3 (S=$2,G=IN,D=$4) [L=0.25,W=0.95,AS=0.49875,AD=0.26125]\n"
    "  DNMOS $4 (S=$4,G=$2,D=OUT) [L=0.25,W=0.95,AS=0.26125,AD=0.49875]\n"
    "  XTRANS $1 ($1=$2,$2=$4,$3=IN)\n"
    "  XTRANS $2 ($1=$2,$2=$5,$3=IN)\n"
    "  XTRANS $3 ($1=$5,$2=OUT,$3=$2)\n"
    "  XTRANS $4 ($1=$4,$2=OUT,$3=$2)\n"
    "Circuit TRANS ($1=$1,$2=$2,$3=$3):\n"
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

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, 0, 0);

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "device_extract_au1_rebuild_ff.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, "NET_", 0);

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "device_extract_au1_rebuild_nf.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, 0, "CIRCUIT_");

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
    au = tl::combine_path (au, "algo");
    au = tl::combine_path (au, "device_extract_au1_rebuild_fr.gds");

    db::compare_layouts (_this, ly2, au);
  }

  {
    db::Layout ly2;
    ly2.dbu (ly.dbu ());
    db::Cell &top2 = ly2.cell (ly2.add_cell ("TOP"));

    db::CellMapping cm = l2n.cell_mapping_into (ly2, top2);

    std::map<unsigned int, const db::Region *> lmap;
    lmap [ly2.insert_layer (db::LayerProperties (10, 0))] = &rpsd;
    lmap [ly2.insert_layer (db::LayerProperties (11, 0))] = &rnsd;
    lmap [ly2.insert_layer (db::LayerProperties (3, 0)) ] = rpoly.get ();
    lmap [ly2.insert_layer (db::LayerProperties (4, 0)) ] = rdiff_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (5, 0)) ] = rpoly_cont.get ();
    lmap [ly2.insert_layer (db::LayerProperties (6, 0)) ] = rmetal1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (7, 0)) ] = rvia1.get ();
    lmap [ly2.insert_layer (db::LayerProperties (8, 0)) ] = rmetal2.get ();

    l2n.build_all_nets (cm, ly2, lmap, "NET_", "CIRCUIT_");

    std::string au = tl::testsrc ();
    au = tl::combine_path (au, "testdata");
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
  EXPECT_EQ (l2n.netlist ()->to_string (),
    "Circuit RINGO (FB=FB,OSC=OSC,VSS=VSS,VDD=VDD):\n"
    "  XINV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD)\n"
    "  XINV2 $2 (IN=FB,$2=(null),OUT=$I19,$4=VSS,$5=VDD)\n"
    "  XINV2 $3 (IN=$I19,$2=(null),OUT=$I1,$4=VSS,$5=VDD)\n"
    "  XINV2 $4 (IN=$I1,$2=(null),OUT=$I2,$4=VSS,$5=VDD)\n"
    "  XINV2 $5 (IN=$I2,$2=(null),OUT=$I3,$4=VSS,$5=VDD)\n"
    "  XINV2 $6 (IN=$I3,$2=(null),OUT=$I4,$4=VSS,$5=VDD)\n"
    "  XINV2 $7 (IN=$I4,$2=(null),OUT=$I5,$4=VSS,$5=VDD)\n"
    "  XINV2 $8 (IN=$I5,$2=(null),OUT=$I6,$4=VSS,$5=VDD)\n"
    "  XINV2 $9 (IN=$I6,$2=(null),OUT=$I7,$4=VSS,$5=VDD)\n"
    "  XINV2 $10 (IN=$I7,$2=(null),OUT=$I8,$4=VSS,$5=VDD)\n"
    "Circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5):\n"
    "  DPMOS $1 (S=$2,G=IN,D=$5) [L=0.25,W=0.95,AS=0.49875,AD=0.26125]\n"
    "  DPMOS $2 (S=$5,G=$2,D=OUT) [L=0.25,W=0.95,AS=0.26125,AD=0.49875]\n"
    "  DNMOS $3 (S=$2,G=IN,D=$4) [L=0.25,W=0.95,AS=0.49875,AD=0.26125]\n"
    "  DNMOS $4 (S=$4,G=$2,D=OUT) [L=0.25,W=0.95,AS=0.26125,AD=0.49875]\n"
  );

  //  compare the collected test data

  std::string au = tl::testsrc ();
  au = tl::combine_path (au, "testdata");
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1_with_rec_nets.gds");

  db::compare_layouts (_this, ly, au);

  //  do some probing after purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  //  the transistor which supplies this probe target has been optimized away by "purge".
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "(null)");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "INV2:$2");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "RINGO:$I2");
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

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l2.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());
  db::LayoutToNetlist l2n (db::RecursiveShapeIterator (ly, tc, std::set<unsigned int> ()));

  std::auto_ptr<db::Region> rnwell (l2n.make_layer (nwell));
  std::auto_ptr<db::Region> ractive (l2n.make_layer (active));
  std::auto_ptr<db::Region> rpoly (l2n.make_polygon_layer (poly));
  std::auto_ptr<db::Region> rpoly_lbl (l2n.make_text_layer (poly_lbl));
  std::auto_ptr<db::Region> rdiff_cont (l2n.make_polygon_layer (diff_cont));
  std::auto_ptr<db::Region> rpoly_cont (l2n.make_polygon_layer (poly_cont));
  std::auto_ptr<db::Region> rmetal1 (l2n.make_polygon_layer (metal1));
  std::auto_ptr<db::Region> rmetal1_lbl (l2n.make_text_layer (metal1_lbl));
  std::auto_ptr<db::Region> rvia1 (l2n.make_polygon_layer (via1));
  std::auto_ptr<db::Region> rmetal2 (l2n.make_polygon_layer (metal2));
  std::auto_ptr<db::Region> rmetal2_lbl (l2n.make_text_layer (metal2_lbl));

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

  //  NOTE: the device extractor will add more debug layers for the transistors:
  //    20/0 -> Diffusion
  //    21/0 -> Gate
  MOSFETExtractor pmos_ex ("PMOS", &ly);
  MOSFETExtractor nmos_ex ("NMOS", &ly);

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
  EXPECT_EQ (l2n.netlist ()->to_string (),
    "Circuit RINGO ():\n"
    "  XINV2PAIR $1 ($1=FB,$2=VDD,$3=VSS,$4=$I3,$5=OSC)\n"
    "  XINV2PAIR $2 ($1=$I18,$2=VDD,$3=VSS,$4=FB,$5=$I9)\n"
    "  XINV2PAIR $3 ($1=$I19,$2=VDD,$3=VSS,$4=$I9,$5=$I1)\n"
    "  XINV2PAIR $4 ($1=$I20,$2=VDD,$3=VSS,$4=$I1,$5=$I2)\n"
    "  XINV2PAIR $5 ($1=$I21,$2=VDD,$3=VSS,$4=$I2,$5=$I3)\n"
    "Circuit INV2PAIR ($1=$I7,$2=$I5,$3=$I4,$4=$I2,$5=$I1):\n"
    "  XINV2 $1 (IN=$I3,$2=$I7,OUT=$I1,$4=$I4,$5=$I5)\n"
    "  XINV2 $2 (IN=$I2,$2=$I6,OUT=$I3,$4=$I4,$5=$I5)\n"
    "Circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5):\n"
    "  DPMOS $1 (S=$2,G=IN,D=$5) [L=0.25,W=0.95,AS=0.49875,AD=0.26125]\n"
    "  DPMOS $2 (S=$5,G=$2,D=OUT) [L=0.25,W=0.95,AS=0.26125,AD=0.49875]\n"
    "  DNMOS $3 (S=$2,G=IN,D=$4) [L=0.25,W=0.95,AS=0.49875,AD=0.26125]\n"
    "  DNMOS $4 (S=$4,G=$2,D=OUT) [L=0.25,W=0.95,AS=0.26125,AD=0.49875]\n"
    "  XTRANS $1 ($1=$2,$2=$4,$3=IN)\n"
    "  XTRANS $2 ($1=$2,$2=$5,$3=IN)\n"
    "  XTRANS $3 ($1=$5,$2=OUT,$3=$2)\n"
    "  XTRANS $4 ($1=$4,$2=OUT,$3=$2)\n"
    "Circuit TRANS ($1=$1,$2=$2,$3=$3):\n"
  );

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
  EXPECT_EQ (l2n.netlist ()->to_string (),
    "Circuit RINGO (FB=FB,OSC=OSC,VSS=VSS,VDD=VDD):\n"
    "  XINV2PAIR $1 ($1=FB,$2=VDD,$3=VSS,$4=$I3,$5=OSC)\n"
    "  XINV2PAIR $2 ($1=(null),$2=VDD,$3=VSS,$4=FB,$5=$I9)\n"
    "  XINV2PAIR $3 ($1=(null),$2=VDD,$3=VSS,$4=$I9,$5=$I1)\n"
    "  XINV2PAIR $4 ($1=(null),$2=VDD,$3=VSS,$4=$I1,$5=$I2)\n"
    "  XINV2PAIR $5 ($1=(null),$2=VDD,$3=VSS,$4=$I2,$5=$I3)\n"
    "Circuit INV2PAIR ($1=$I7,$2=$I5,$3=$I4,$4=$I2,$5=$I1):\n"
    "  XINV2 $1 (IN=$I3,$2=$I7,OUT=$I1,$4=$I4,$5=$I5)\n"
    "  XINV2 $2 (IN=$I2,$2=(null),OUT=$I3,$4=$I4,$5=$I5)\n"
    "Circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5):\n"
    "  DPMOS $1 (S=$2,G=IN,D=$5) [L=0.25,W=0.95,AS=0.49875,AD=0.26125]\n"
    "  DPMOS $2 (S=$5,G=$2,D=OUT) [L=0.25,W=0.95,AS=0.26125,AD=0.49875]\n"
    "  DNMOS $3 (S=$2,G=IN,D=$4) [L=0.25,W=0.95,AS=0.49875,AD=0.26125]\n"
    "  DNMOS $4 (S=$4,G=$2,D=OUT) [L=0.25,W=0.95,AS=0.26125,AD=0.49875]\n"
  );

  //  compare the collected test data

  std::string au = tl::testsrc ();
  au = tl::combine_path (au, "testdata");
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au2_with_rec_nets.gds");

  db::compare_layouts (_this, ly, au);

  //  do some probing after purging

  //  top level
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (0.0, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::Point (0, 1800))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal2, db::DPoint (-2.0, 1.8))), "(null)");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (-1.5, 1.8))), "RINGO:FB");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (24.5, 1.8))), "RINGO:OSC");
  //  the transistor which supplies this probe target has been optimized away by "purge".
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (5.3, 0.0))), "(null)");

  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (2.6, 1.0))), "INV2PAIR:$I7");
  EXPECT_EQ (qnet_name (l2n.probe_net (*rmetal1, db::DPoint (6.4, 1.0))), "INV2PAIR:$I3");
}

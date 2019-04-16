
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


#include "dbNetlistDeviceExtractorClasses.h"
#include "dbNetlistExtractor.h"
#include "dbNetlistDeviceClasses.h"
#include "dbLayout.h"
#include "dbDeepShapeStore.h"
#include "dbRegion.h"
#include "dbStream.h"
#include "dbDeepRegion.h"
#include "dbDeepShapeStore.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbCommonReader.h"
#include "dbTestSupport.h"
#include "dbCellMapping.h"
#include "dbTestSupport.h"

#include "tlUnitTest.h"
#include "tlString.h"
#include "tlFileUtils.h"

#include <memory>
#include <limits>

static unsigned int layer_of (const db::Region &region)
{
  const db::DeepRegion *dr = dynamic_cast<const db::DeepRegion *> (region.delegate ());
  tl_assert (dr != 0);
  return dr->deep_layer ().layer ();
}

static unsigned int define_layer (db::Layout &ly, db::LayerMap &lmap, int gds_layer, int gds_datatype = 0)
{
  unsigned int lid = ly.insert_layer (db::LayerProperties (gds_layer, gds_datatype));
  lmap.map (ly.get_properties (lid), lid);
  return lid;
}

static void dump_nets_to_layout (const db::Netlist &nl, const db::hier_clusters<db::PolygonRef> &clusters, db::Layout &ly, const std::map<unsigned int, unsigned int> &lmap, const db::CellMapping &cmap)
{
  std::set<db::cell_index_type> device_cells_seen;

  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {

    db::Cell &cell = ly.cell (cmap.cell_mapping (c->cell_index ()));

    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {

      const db::local_cluster<db::PolygonRef> &lc = clusters.clusters_per_cell (c->cell_index ()).cluster_by_id (n->cluster_id ());

      bool any_shapes = false;
      for (std::map<unsigned int, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end () && !any_shapes; ++m) {
        any_shapes = ! lc.begin (m->first).at_end ();
      }

      if (any_shapes) {

        std::string nn = "NET_" + c->name () + "_" + n->expanded_name ();
        db::Cell &net_cell = ly.cell (ly.add_cell (nn.c_str ()));
        cell.insert (db::CellInstArray (db::CellInst (net_cell.cell_index ()), db::Trans ()));

        for (std::map<unsigned int, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end (); ++m) {
          db::Shapes &target = net_cell.shapes (m->second);
          for (db::local_cluster<db::PolygonRef>::shape_iterator s = lc.begin (m->first); !s.at_end (); ++s) {
            target.insert (*s);
          }
        }

      }

    }

    for (db::Circuit::const_device_iterator d = c->begin_devices (); d != c->end_devices (); ++d) {

      db::cell_index_type dci = d->device_abstract ()->cell_index ();

      if (device_cells_seen.find (dci) != device_cells_seen.end ()) {
        continue;
      }

      db::Cell &device_cell = ly.cell (cmap.cell_mapping (dci));
      device_cells_seen.insert (dci);

      std::string ps;
      const std::vector<db::DeviceParameterDefinition> &pd = d->device_class ()->parameter_definitions ();
      for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
        if (! ps.empty ()) {
          ps += ",";
        }
        ps += i->name () + "=" + tl::to_string (d->parameter_value (i->id ()));
      }

      const std::vector<db::DeviceTerminalDefinition> &td = d->device_class ()->terminal_definitions ();
      for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {

        const db::local_cluster<db::PolygonRef> &dc = clusters.clusters_per_cell (dci).cluster_by_id (d->device_abstract ()->cluster_id_for_terminal (t->id ()));

        for (std::map<unsigned int, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end (); ++m) {
          db::Shapes &target = device_cell.shapes (m->second);
          for (db::local_cluster<db::PolygonRef>::shape_iterator s = dc.begin (m->first); !s.at_end (); ++s) {
            target.insert (*s);
          }
        }

      }

    }

  }
}

TEST(1_DeviceAndNetExtraction)
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

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region rnwell (db::RecursiveShapeIterator (ly, tc, nwell), dss);
  db::Region ractive (db::RecursiveShapeIterator (ly, tc, active), dss);
  db::Region rpoly (db::RecursiveShapeIterator (ly, tc, poly), dss);
  db::Region rpoly_lbl (db::RecursiveShapeIterator (ly, tc, poly_lbl), dss);
  db::Region rdiff_cont (db::RecursiveShapeIterator (ly, tc, diff_cont), dss);
  db::Region rpoly_cont (db::RecursiveShapeIterator (ly, tc, poly_cont), dss);
  db::Region rmetal1 (db::RecursiveShapeIterator (ly, tc, metal1), dss);
  db::Region rmetal1_lbl (db::RecursiveShapeIterator (ly, tc, metal1_lbl), dss);
  db::Region rvia1 (db::RecursiveShapeIterator (ly, tc, via1), dss);
  db::Region rmetal2 (db::RecursiveShapeIterator (ly, tc, metal2), dss);
  db::Region rmetal2_lbl (db::RecursiveShapeIterator (ly, tc, metal2_lbl), dss);

  //  derived regions

  db::Region rpactive = ractive & rnwell;
  db::Region rpgate   = rpactive & rpoly;
  db::Region rpsd     = rpactive - rpgate;

  db::Region rnactive = ractive - rnwell;
  db::Region rngate   = rnactive & rpoly;
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

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS3Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS3Transistor nmos_ex ("NMOS");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  pmos_ex.extract (dss, 0, dl, nl, cl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  nmos_ex.extract (dss, 0, dl, nl, cl);

  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;
  //  Intra-layer
  conn.connect (rpsd);
  conn.connect (rnsd);
  conn.connect (rpoly);
  conn.connect (rdiff_cont);
  conn.connect (rpoly_cont);
  conn.connect (rmetal1);
  conn.connect (rvia1);
  conn.connect (rmetal2);
  //  Inter-layer
  conn.connect (rpsd,       rdiff_cont);
  conn.connect (rnsd,       rdiff_cont);
  conn.connect (rpoly,      rpoly_cont);
  conn.connect (rpoly_cont, rmetal1);
  conn.connect (rdiff_cont, rmetal1);
  conn.connect (rmetal1,    rvia1);
  conn.connect (rvia1,      rmetal2);
  conn.connect (rpoly,      rpoly_lbl);     //  attaches labels
  conn.connect (rmetal1,    rmetal1_lbl);   //  attaches labels
  conn.connect (rmetal2,    rmetal2_lbl);   //  attaches labels

  //  extract the nets

  net_ex.extract_nets (dss, 0, conn, nl, cl);

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
  std::map<unsigned int, unsigned int> dump_map;
  dump_map [layer_of (rpsd)      ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [layer_of (rnsd)      ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [layer_of (rpoly)     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [layer_of (rdiff_cont)] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [layer_of (rpoly_cont)] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [layer_of (rmetal1)   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [layer_of (rvia1)     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [layer_of (rmetal2)   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = dss.cell_mapping_to_original (0, &ly, tc.cell_index ());
  dump_nets_to_layout (nl, cl, ly, dump_map, cm);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
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

  //  use this opportunity to test serialization to and from string
  db::Netlist nldup;
  for (db::Netlist::device_class_iterator i = nl.begin_device_classes (); i != nl.end_device_classes (); ++i) {
    nldup.add_device_class (i->clone ());
  }
  nldup.from_string (nl.to_string ());
  EXPECT_EQ (nldup.to_string (), nl.to_string ());

  // doesn't do anything here, but we test that this does not destroy anything:
  nl.combine_devices ();

  //  make pins for named nets of top-level circuits - this way they are not purged
  nl.make_top_level_pins ();
  nl.purge ();
  nl.purge_nets ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit RINGO (FB=FB,OSC=OSC,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $2 (IN=FB,$2=(null),OUT=$I19,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $3 (IN=$I19,$2=(null),OUT=$I1,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $4 (IN=$I1,$2=(null),OUT=$I2,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $5 (IN=$I2,$2=(null),OUT=$I3,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $6 (IN=$I3,$2=(null),OUT=$I4,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $7 (IN=$I4,$2=(null),OUT=$I5,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $8 (IN=$I5,$2=(null),OUT=$I6,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $9 (IN=$I6,$2=(null),OUT=$I7,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $10 (IN=$I7,$2=(null),OUT=$I8,$4=VSS,$5=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testsrc ();
  au = tl::combine_path (au, "testdata");
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(2_DeviceAndNetExtractionFlat)
{
  db::Layout ly (true);
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
    ly.flatten (ly.cell (*ly.begin_top_down ()), -1, true);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region rnwell (db::RecursiveShapeIterator (ly, tc, nwell), dss);
  db::Region ractive (db::RecursiveShapeIterator (ly, tc, active), dss);
  db::Region rpoly (db::RecursiveShapeIterator (ly, tc, poly), dss);
  db::Region rpoly_lbl (db::RecursiveShapeIterator (ly, tc, poly_lbl), dss);
  db::Region rdiff_cont (db::RecursiveShapeIterator (ly, tc, diff_cont), dss);
  db::Region rpoly_cont (db::RecursiveShapeIterator (ly, tc, poly_cont), dss);
  db::Region rmetal1 (db::RecursiveShapeIterator (ly, tc, metal1), dss);
  db::Region rmetal1_lbl (db::RecursiveShapeIterator (ly, tc, metal1_lbl), dss);
  db::Region rvia1 (db::RecursiveShapeIterator (ly, tc, via1), dss);
  db::Region rmetal2 (db::RecursiveShapeIterator (ly, tc, metal2), dss);
  db::Region rmetal2_lbl (db::RecursiveShapeIterator (ly, tc, metal2_lbl), dss);

  //  derived regions

  db::Region rpactive = ractive & rnwell;
  db::Region rpgate   = rpactive & rpoly;
  db::Region rpsd     = rpactive - rpgate;

  db::Region rnactive = ractive - rnwell;
  db::Region rngate   = rnactive & rpoly;
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

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS3Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS3Transistor nmos_ex ("NMOS");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  pmos_ex.extract (dss, 0, dl, nl, cl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  nmos_ex.extract (dss, 0, dl, nl, cl);

  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;
  //  Intra-layer
  conn.connect (rpsd);
  conn.connect (rnsd);
  conn.connect (rpoly);
  conn.connect (rdiff_cont);
  conn.connect (rpoly_cont);
  conn.connect (rmetal1);
  conn.connect (rvia1);
  conn.connect (rmetal2);
  //  Inter-layer
  conn.connect (rpsd,       rdiff_cont);
  conn.connect (rnsd,       rdiff_cont);
  conn.connect (rpoly,      rpoly_cont);
  conn.connect (rpoly_cont, rmetal1);
  conn.connect (rdiff_cont, rmetal1);
  conn.connect (rmetal1,    rvia1);
  conn.connect (rvia1,      rmetal2);
  conn.connect (rpoly,      rpoly_lbl);     //  attaches labels
  conn.connect (rmetal1,    rmetal1_lbl);   //  attaches labels
  conn.connect (rmetal2,    rmetal2_lbl);   //  attaches labels

  //  extract the nets

  //  don't use "join_nets_by_label" because the flattened texts will spoil everything
  net_ex.extract_nets (dss, 0, conn, nl, cl);

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
  std::map<unsigned int, unsigned int> dump_map;
  dump_map [layer_of (rpsd)      ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [layer_of (rnsd)      ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [layer_of (rpoly)     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [layer_of (rdiff_cont)] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [layer_of (rpoly_cont)] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [layer_of (rmetal1)   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [layer_of (rvia1)     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [layer_of (rmetal2)   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = dss.cell_mapping_to_original (0, &ly, tc.cell_index ());
  dump_nets_to_layout (nl, cl, ly, dump_map, cm);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit RINGO ();\n"
    "  device PMOS $1 (S=$16,G='IN,OUT$6',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$16,D='IN,OUT$7') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $3 (S=$14,G='IN,OUT$5',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $4 (S=VDD,G=$14,D='IN,OUT$6') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $5 (S=$12,G='IN,OUT$4',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $6 (S=VDD,G=$12,D='IN,OUT$5') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $7 (S='IN,FB',G='IN,OUT$8',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $8 (S=VDD,G='IN,FB',D='OUT,OSC') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $9 (S=$4,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $10 (S=VDD,G=$4,D='IN,OUT$1') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $11 (S=$8,G='IN,OUT$2',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $12 (S=VDD,G=$8,D='IN,OUT$3') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $13 (S=$2,G='IN,FB',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $14 (S=VDD,G=$2,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $15 (S=$6,G='IN,OUT$1',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $16 (S=VDD,G=$6,D='IN,OUT$2') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $17 (S=$18,G='IN,OUT$7',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $18 (S=VDD,G=$18,D='IN,OUT$8') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $19 (S=$10,G='IN,OUT$3',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $20 (S=VDD,G=$10,D='IN,OUT$4') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $21 (S='IN,FB',G='IN,OUT$8',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $22 (S=VSS,G='IN,FB',D='OUT,OSC') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $23 (S=$18,G='IN,OUT$7',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $24 (S=VSS,G=$18,D='IN,OUT$8') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $25 (S=$14,G='IN,OUT$5',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $26 (S=VSS,G=$14,D='IN,OUT$6') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $27 (S=$12,G='IN,OUT$4',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $28 (S=VSS,G=$12,D='IN,OUT$5') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $29 (S=$4,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $30 (S=VSS,G=$4,D='IN,OUT$1') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $31 (S=$2,G='IN,FB',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $32 (S=VSS,G=$2,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $33 (S=$8,G='IN,OUT$2',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $34 (S=VSS,G=$8,D='IN,OUT$3') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $35 (S=$6,G='IN,OUT$1',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $36 (S=VSS,G=$6,D='IN,OUT$2') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $37 (S=$16,G='IN,OUT$6',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $38 (S=VSS,G=$16,D='IN,OUT$7') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $39 (S=$10,G='IN,OUT$3',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $40 (S=VSS,G=$10,D='IN,OUT$4') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testsrc ();
  au = tl::combine_path (au, "testdata");
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1_flat.gds");

  db::compare_layouts (_this, ly, au);
}

static bool
all_net_names_unique (const db::Circuit &c)
{
  std::set<std::string> names;
  for (db::Circuit::const_net_iterator n = c.begin_nets (); n != c.end_nets (); ++n) {
    if (! n->name ().empty ()) {
      if (names.find (n->name ()) != names.end ()) {
        return false;
      } else {
        names.insert (n->name ());
      }
    }
  }
  return true;
}

static bool
all_net_names_unique (const db::Netlist &nl)
{
  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {
    if (! all_net_names_unique (*c)) {
      return false;
    }
  }
  return true;
}

TEST(3_DeviceAndNetExtractionWithImplicitConnections)
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
    fn = tl::combine_path (fn, "device_extract_l1_implicit_nets.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region rnwell (db::RecursiveShapeIterator (ly, tc, nwell), dss);
  db::Region ractive (db::RecursiveShapeIterator (ly, tc, active), dss);
  db::Region rpoly (db::RecursiveShapeIterator (ly, tc, poly), dss);
  db::Region rpoly_lbl (db::RecursiveShapeIterator (ly, tc, poly_lbl), dss);
  db::Region rdiff_cont (db::RecursiveShapeIterator (ly, tc, diff_cont), dss);
  db::Region rpoly_cont (db::RecursiveShapeIterator (ly, tc, poly_cont), dss);
  db::Region rmetal1 (db::RecursiveShapeIterator (ly, tc, metal1), dss);
  db::Region rmetal1_lbl (db::RecursiveShapeIterator (ly, tc, metal1_lbl), dss);
  db::Region rvia1 (db::RecursiveShapeIterator (ly, tc, via1), dss);
  db::Region rmetal2 (db::RecursiveShapeIterator (ly, tc, metal2), dss);
  db::Region rmetal2_lbl (db::RecursiveShapeIterator (ly, tc, metal2_lbl), dss);

  //  derived regions

  db::Region rpactive = ractive & rnwell;
  db::Region rpgate   = rpactive & rpoly;
  db::Region rpsd     = rpactive - rpgate;

  db::Region rnactive = ractive - rnwell;
  db::Region rngate   = rnactive & rpoly;
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

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS3Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS3Transistor nmos_ex ("NMOS");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  pmos_ex.extract (dss, 0, dl, nl, cl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  nmos_ex.extract (dss, 0, dl, nl, cl);

  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;
  //  Intra-layer
  conn.connect (rpsd);
  conn.connect (rnsd);
  conn.connect (rpoly);
  conn.connect (rdiff_cont);
  conn.connect (rpoly_cont);
  conn.connect (rmetal1);
  conn.connect (rvia1);
  conn.connect (rmetal2);
  //  Inter-layer
  conn.connect (rpsd,       rdiff_cont);
  conn.connect (rnsd,       rdiff_cont);
  conn.connect (rpoly,      rpoly_cont);
  conn.connect (rpoly_cont, rmetal1);
  conn.connect (rdiff_cont, rmetal1);
  conn.connect (rmetal1,    rvia1);
  conn.connect (rvia1,      rmetal2);
  conn.connect (rpoly,      rpoly_lbl);     //  attaches labels
  conn.connect (rmetal1,    rmetal1_lbl);   //  attaches labels
  conn.connect (rmetal2,    rmetal2_lbl);   //  attaches labels

  //  extract the nets

  db::Netlist nl2 = nl;
  net_ex.extract_nets (dss, 0, conn, nl2, cl, "{VDDZ,VSSZ,NEXT,FB}");

  EXPECT_EQ (all_net_names_unique (nl2), true);

  nl2 = nl;
  net_ex.extract_nets (dss, 0, conn, nl2, cl, "{VDDZ,VSSZ,NEXT}");

  EXPECT_EQ (all_net_names_unique (nl2), false);

  net_ex.extract_nets (dss, 0, conn, nl, cl, "*");

  EXPECT_EQ (all_net_names_unique (nl), true);

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
  std::map<unsigned int, unsigned int> dump_map;
  dump_map [layer_of (rpsd)      ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [layer_of (rnsd)      ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [layer_of (rpoly)     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [layer_of (rdiff_cont)] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [layer_of (rpoly_cont)] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [layer_of (rmetal1)   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [layer_of (rvia1)     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [layer_of (rmetal2)   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = dss.cell_mapping_to_original (0, &ly, tc.cell_index ());
  dump_nets_to_layout (nl, cl, ly, dump_map, cm);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit RINGO ();\n"
    "  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $3 (IN=NEXT,$2=$I43,OUT=$I5,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $4 (IN=$I3,$2=$I42,OUT=NEXT,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $5 (IN=$I5,$2=$I44,OUT=$I6,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $6 (IN=$I6,$2=$I45,OUT=$I7,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $7 (IN=$I7,$2=$I46,OUT=$I8,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $8 (IN=$I19,$2=$I39,OUT=$I1,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $9 (IN=$I1,$2=$I40,OUT=$I2,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $10 (IN=$I2,$2=$I41,OUT=$I3,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
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

  // doesn't do anything here, but we test that this does not destroy anything:
  nl.combine_devices ();

  //  make pins for named nets of top-level circuits - this way they are not purged
  nl.make_top_level_pins ();
  nl.purge ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit RINGO (FB=FB,OSC=OSC,NEXT=NEXT,'VSSZ,VSS'='VSSZ,VSS','VDDZ,VDD'='VDDZ,VDD');\n"
    "  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $2 (IN=FB,$2=(null),OUT=$I19,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $3 (IN=NEXT,$2=(null),OUT=$I5,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $4 (IN=$I3,$2=(null),OUT=NEXT,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $5 (IN=$I5,$2=(null),OUT=$I6,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $6 (IN=$I6,$2=(null),OUT=$I7,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $7 (IN=$I7,$2=(null),OUT=$I8,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $8 (IN=$I19,$2=(null),OUT=$I1,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $9 (IN=$I1,$2=(null),OUT=$I2,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "  subcircuit INV2 $10 (IN=$I2,$2=(null),OUT=$I3,$4='VSSZ,VSS',$5='VDDZ,VDD');\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testsrc ();
  au = tl::combine_path (au, "testdata");
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1_implicit_nets.gds");

  db::compare_layouts (_this, ly, au);
}


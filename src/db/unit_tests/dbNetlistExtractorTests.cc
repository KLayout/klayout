
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


#include "dbNetlistDeviceExtractorClasses.h"
#include "dbNetlistExtractor.h"
#include "dbNetlistDeviceClasses.h"
#include "dbLayout.h"
#include "dbRegion.h"
#include "dbTexts.h"
#include "dbStream.h"
#include "dbDeepRegion.h"
#include "dbDeepTexts.h"
#include "dbDeepShapeStore.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbCommonReader.h"
#include "dbTestSupport.h"
#include "dbCellMapping.h"
#include "dbTestSupport.h"
#include "dbNetlistCompare.h"

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

static unsigned int layer_of (const db::Texts &region)
{
  const db::DeepTexts *dr = dynamic_cast<const db::DeepTexts *> (region.delegate ());
  tl_assert (dr != 0);
  return dr->deep_layer ().layer ();
}

static unsigned int define_layer (db::Layout &ly, db::LayerMap &lmap, int gds_layer, int gds_datatype = 0)
{
  unsigned int lid = ly.insert_layer (db::LayerProperties (gds_layer, gds_datatype));
  lmap.map (ly.get_properties (lid), lid);
  return lid;
}

static void dump_nets_to_layout (const db::Netlist &nl, const db::hier_clusters<db::NetShape> &clusters, db::Layout &ly, const std::map<unsigned int, unsigned int> &lmap, const db::CellMapping &cmap, bool with_device_cells = false)
{
  std::set<db::cell_index_type> device_cells_seen;

  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {

    db::Cell &cell = ly.cell (cmap.cell_mapping (c->cell_index ()));

    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {

      std::string nn = "NET_" + c->name () + "_" + n->expanded_name ();
      const db::local_cluster<db::NetShape> &lc = clusters.clusters_per_cell (c->cell_index ()).cluster_by_id (n->cluster_id ());

      bool any_shapes = false;
      for (std::map<unsigned int, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end () && !any_shapes; ++m) {
        any_shapes = ! lc.begin (m->first).at_end ();
      }

      if (any_shapes || (with_device_cells && n->terminal_count() > 0)) {

        db::Cell &net_cell = ly.cell (ly.add_cell (nn.c_str ()));
        cell.insert (db::CellInstArray (db::CellInst (net_cell.cell_index ()), db::Trans ()));

        for (std::map<unsigned int, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end (); ++m) {
          db::Shapes &target = net_cell.shapes (m->second);
          for (db::local_cluster<db::NetShape>::shape_iterator s = lc.begin (m->first); !s.at_end (); ++s) {
            s->insert_into (target);
          }
        }

        if (with_device_cells) {
          for (db::Net::const_terminal_iterator t = n->begin_terminals (); t != n->end_terminals (); ++t) {

            const db::NetTerminalRef &tref = *t;

            if (! tref.device ()->device_abstract ()) {
              continue;
            }

            db::cell_index_type dci = tref.device ()->device_abstract ()->cell_index ();
            db::DCplxTrans dtr = tref.device ()->trans ();

            net_cell.insert (db::CellInstArray (db::CellInst (dci), db::CplxTrans (ly.dbu ()).inverted () * dtr * db::CplxTrans (ly.dbu ())));

            for (std::vector<db::DeviceAbstractRef>::const_iterator a = tref.device ()->other_abstracts ().begin (); a != tref.device ()->other_abstracts ().end (); ++a) {
              const db::DeviceAbstractRef &aref = *a;
              dci = aref.device_abstract->cell_index ();
              net_cell.insert (db::CellInstArray (db::CellInst (dci), db::CplxTrans (ly.dbu ()).inverted () * dtr * aref.trans * db::CplxTrans (ly.dbu ())));
            }

          }
        }

      }

    }

    for (db::Circuit::const_device_iterator d = c->begin_devices (); d != c->end_devices (); ++d) {

      if (! d->device_abstract ()) {
        continue;
      }

      std::vector<db::cell_index_type> original_device_cells;

      db::cell_index_type dci = d->device_abstract ()->cell_index ();
      if (device_cells_seen.find (dci) == device_cells_seen.end ()) {
        original_device_cells.push_back (dci);
        device_cells_seen.insert (dci);
      }

      for (std::vector<db::DeviceAbstractRef>::const_iterator a = d->other_abstracts ().begin (); a != d->other_abstracts ().end (); ++a) {
        db::cell_index_type dci = a->device_abstract->cell_index ();
        if (device_cells_seen.find (dci) == device_cells_seen.end ()) {
          original_device_cells.push_back (dci);
          device_cells_seen.insert (dci);
        }
      }

      for (std::vector<db::cell_index_type>::const_iterator i = original_device_cells.begin (); i != original_device_cells.end (); ++i) {

        db::cell_index_type dci = *i;
        db::Cell &device_cell = ly.cell (cmap.cell_mapping (dci));

        const std::vector<db::DeviceTerminalDefinition> &td = d->device_class ()->terminal_definitions ();
        for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {

          const db::local_cluster<db::NetShape> &dc = clusters.clusters_per_cell (dci).cluster_by_id (d->device_abstract ()->cluster_id_for_terminal (t->id ()));

          for (std::map<unsigned int, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end (); ++m) {
            db::Shapes &target = device_cell.shapes (m->second);
            for (db::local_cluster<db::NetShape>::shape_iterator s = dc.begin (m->first); !s.at_end (); ++s) {
              s->insert_into (target);
            }
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

    std::string fn (tl::testdata ());
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
  db::hier_clusters<db::NetShape> cl;

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

  //  check if net names are properly assigned
  db::Circuit *top_circuit = nl.circuit_by_name ("RINGO");
  EXPECT_EQ (top_circuit != 0, true);
  if (top_circuit) {
    db::Net *fb_net = top_circuit->net_by_name ("FB");
    EXPECT_EQ (fb_net != 0, true);
  }

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
  dump_map [layer_of (rpsd)        ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [layer_of (rnsd)        ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [layer_of (rpoly)       ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [layer_of (rpoly_lbl)   ] = ly.insert_layer (db::LayerProperties (203, 1));
  dump_map [layer_of (rdiff_cont)  ] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [layer_of (rpoly_cont)  ] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [layer_of (rmetal1)     ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [layer_of (rmetal1_lbl) ] = ly.insert_layer (db::LayerProperties (206, 1));
  dump_map [layer_of (rvia1)       ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [layer_of (rmetal2)     ] = ly.insert_layer (db::LayerProperties (208, 0));
  dump_map [layer_of (rmetal2_lbl) ] = ly.insert_layer (db::LayerProperties (208, 1));

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

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(1a_DeviceAndNetExtractionWithTextsAsLabels)
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

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region rnwell (db::RecursiveShapeIterator (ly, tc, nwell), dss);
  db::Region ractive (db::RecursiveShapeIterator (ly, tc, active), dss);
  db::Region rpoly (db::RecursiveShapeIterator (ly, tc, poly), dss);
  db::Texts rpoly_lbl (db::RecursiveShapeIterator (ly, tc, poly_lbl), dss);
  db::Region rdiff_cont (db::RecursiveShapeIterator (ly, tc, diff_cont), dss);
  db::Region rpoly_cont (db::RecursiveShapeIterator (ly, tc, poly_cont), dss);
  db::Region rmetal1 (db::RecursiveShapeIterator (ly, tc, metal1), dss);
  db::Texts rmetal1_lbl (db::RecursiveShapeIterator (ly, tc, metal1_lbl), dss);
  db::Region rvia1 (db::RecursiveShapeIterator (ly, tc, via1), dss);
  db::Region rmetal2 (db::RecursiveShapeIterator (ly, tc, metal2), dss);
  db::Texts rmetal2_lbl (db::RecursiveShapeIterator (ly, tc, metal2_lbl), dss);

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
  db::hier_clusters<db::NetShape> cl;

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

  //  check if net names are properly assigned
  db::Circuit *top_circuit = nl.circuit_by_name ("RINGO");
  EXPECT_EQ (top_circuit != 0, true);
  if (top_circuit) {
    db::Net *fb_net = top_circuit->net_by_name ("FB");
    EXPECT_EQ (fb_net != 0, true);
  }

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
  dump_map [layer_of (rpsd)        ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [layer_of (rnsd)        ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [layer_of (rpoly)       ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [layer_of (rpoly_lbl)   ] = ly.insert_layer (db::LayerProperties (203, 1));
  dump_map [layer_of (rdiff_cont)  ] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [layer_of (rpoly_cont)  ] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [layer_of (rmetal1)     ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [layer_of (rmetal1_lbl) ] = ly.insert_layer (db::LayerProperties (206, 1));
  dump_map [layer_of (rvia1)       ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [layer_of (rmetal2)     ] = ly.insert_layer (db::LayerProperties (208, 0));
  dump_map [layer_of (rmetal2_lbl) ] = ly.insert_layer (db::LayerProperties (208, 1));

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

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1a.gds");

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

    std::string fn (tl::testdata ());
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
  db::hier_clusters<db::NetShape> cl;

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

  std::string au = tl::testdata ();
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

    std::string fn (tl::testdata ());
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
  db::hier_clusters<db::NetShape> cl;

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

  std::list<tl::GlobPattern> gp;

  db::Netlist nl2 = nl;
  gp.clear ();
  gp.push_back (tl::GlobPattern ("{VDDZ,VSSZ,NEXT,FB}"));
  net_ex.set_joined_net_names (gp);
  net_ex.extract_nets (dss, 0, conn, nl2, cl);

  EXPECT_EQ (all_net_names_unique (nl2), true);

  nl2 = nl;
  gp.clear ();
  gp.push_back (tl::GlobPattern ("VDDZ"));
  gp.push_back (tl::GlobPattern ("VSSZ"));
  gp.push_back (tl::GlobPattern ("NEXT"));
  gp.push_back (tl::GlobPattern ("FB"));
  net_ex.set_joined_net_names (gp);
  net_ex.extract_nets (dss, 0, conn, nl2, cl);

  EXPECT_EQ (all_net_names_unique (nl2), true);

  nl2 = nl;
  gp.clear ();
  gp.push_back (tl::GlobPattern ("{VDDZ,VSSZ,NEXT}"));
  net_ex.set_joined_net_names (gp);
  net_ex.extract_nets (dss, 0, conn, nl2, cl);

  EXPECT_EQ (all_net_names_unique (nl2), false);

  gp.clear ();
  gp.push_back (tl::GlobPattern ("*"));
  net_ex.set_joined_net_names (gp);
  net_ex.extract_nets (dss, 0, conn, nl, cl);

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
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1_implicit_nets.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(4_ResAndCapExtraction)
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
  unsigned int cap        = define_layer (ly, lmap, 10);
  unsigned int res        = define_layer (ly, lmap, 11);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "devices_test.oas");

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
  db::Region rpoly_all (db::RecursiveShapeIterator (ly, tc, poly), dss);
  db::Region rpoly_lbl (db::RecursiveShapeIterator (ly, tc, poly_lbl), dss);
  db::Region rdiff_cont (db::RecursiveShapeIterator (ly, tc, diff_cont), dss);
  db::Region rpoly_cont (db::RecursiveShapeIterator (ly, tc, poly_cont), dss);
  db::Region rmetal1 (db::RecursiveShapeIterator (ly, tc, metal1), dss);
  db::Region rmetal1_lbl (db::RecursiveShapeIterator (ly, tc, metal1_lbl), dss);
  db::Region rvia1 (db::RecursiveShapeIterator (ly, tc, via1), dss);
  db::Region rmetal2 (db::RecursiveShapeIterator (ly, tc, metal2), dss);
  db::Region rmetal2_lbl (db::RecursiveShapeIterator (ly, tc, metal2_lbl), dss);
  db::Region rcap (db::RecursiveShapeIterator (ly, tc, cap), dss);
  db::Region rres (db::RecursiveShapeIterator (ly, tc, res), dss);

  //  derived regions

  db::Region rpoly     = rpoly_all - rres;
  db::Region rpoly_res = rpoly_all & rres;

  db::Region rpactive  = ractive & rnwell;
  db::Region rpgate    = rpactive & rpoly;
  db::Region rpsd      = rpactive - rpgate;

  db::Region rnactive  = ractive - rnwell;
  db::Region rngate    = rnactive & rpoly;
  db::Region rnsd      = rnactive - rngate;

  db::Region rcap1     = rmetal1 & rcap;
  db::Region rcap2     = rmetal2 & rcap;

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int lgate     = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> Gate
  unsigned int lsd       = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> Source/Drain
  unsigned int lpdiff    = ly.insert_layer (db::LayerProperties (12, 0));      // 12/0 -> P Diffusion
  unsigned int lndiff    = ly.insert_layer (db::LayerProperties (13, 0));      // 13/0 -> N Diffusion
  unsigned int lpoly_res = ly.insert_layer (db::LayerProperties (14, 0));      // 14/0 -> Resistor Poly
  unsigned int lcap1     = ly.insert_layer (db::LayerProperties (15, 0));      // 15/0 -> Cap 1
  unsigned int lcap2     = ly.insert_layer (db::LayerProperties (16, 0));      // 16/0 -> Cap 2

  rpgate.insert_into (&ly, tc.cell_index (), lgate);
  rngate.insert_into (&ly, tc.cell_index (), lgate);
  rpsd.insert_into (&ly, tc.cell_index (), lsd);
  rnsd.insert_into (&ly, tc.cell_index (), lsd);
  rpsd.insert_into (&ly, tc.cell_index (), lpdiff);
  rnsd.insert_into (&ly, tc.cell_index (), lndiff);
  rpoly_res.insert_into (&ly, tc.cell_index (), lpoly_res);
  rcap1.insert_into (&ly, tc.cell_index (), lcap1);
  rcap2.insert_into (&ly, tc.cell_index (), lcap2);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS3Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS3Transistor nmos_ex ("NMOS");
  db::NetlistDeviceExtractorResistor res_ex ("POLY_RES", 50.0);
  db::NetlistDeviceExtractorCapacitor cap_ex ("MIM_CAP", 1.0e-15);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  //  terminal patches
  dl["tG"] = &rpoly;
  dl["tS"] = &rpsd;
  dl["tD"] = &rpsd;
  pmos_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  //  terminal patches
  dl["tG"] = &rpoly;
  dl["tS"] = &rnsd;
  dl["tD"] = &rnsd;
  nmos_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["R"] = &rpoly_res;
  dl["C"] = &rpoly;
  //  terminal patches
  dl["tA"] = &rpoly;
  dl["tB"] = &rpoly;
  res_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["P1"] = &rcap1;
  dl["P2"] = &rcap2;
  //  terminal patches
  dl["tA"] = &rmetal1;
  dl["tB"] = &rmetal2;
  cap_ex.extract (dss, 0, dl, nl, cl);


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

  std::list<tl::GlobPattern> gp;
  gp.push_back (tl::GlobPattern ("*"));
  net_ex.set_joined_net_names (gp);
  net_ex.extract_nets (dss, 0, conn, nl, cl);

  //  Flatten device circuits

  std::vector<std::string> circuits_to_flatten;
  circuits_to_flatten.push_back ("RES");
  circuits_to_flatten.push_back ("RES_MEANDER");
  circuits_to_flatten.push_back ("TRANS");
  circuits_to_flatten.push_back ("TRANS2");

  for (std::vector<std::string>::const_iterator i = circuits_to_flatten.begin (); i != circuits_to_flatten.end (); ++i) {
    db::Circuit *c = nl.circuit_by_name (*i);
    tl_assert (c != 0);
    nl.flatten_circuit (c);
  }

  //  cleanup + completion
  nl.combine_devices ();
  nl.make_top_level_pins ();
  nl.purge ();

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
  dump_nets_to_layout (nl, cl, ly, dump_map, cm, true /*with device cells*/);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit TOP (VSS=VSS,IN=IN,OUT=OUT,VDD=VDD);\n"
    "  device PMOS $1 (S=VDD,G=$4,D=OUT) (L=0.4,W=2.3,AS=1.38,AD=1.38,PS=5.8,PD=5.8);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=$3) (L=0.4,W=2.3,AS=1.38,AD=1.38,PS=5.8,PD=5.8);\n"
    "  device NMOS $3 (S=VSS,G=$4,D=OUT) (L=0.4,W=4.6,AS=2.185,AD=2.185,PS=8.8,PD=8.8);\n"
    "  device MIM_CAP $5 (A=$4,B=VSS) (C=2.622e-14,A=26.22,P=29.8);\n"
    "  device POLY_RES $7 (A=$3,B=$4) (R=750,L=6,W=0.4,A=2.4,P=13.6);\n"
    "  device POLY_RES $9 (A=$4,B=VSS) (R=1825,L=14.6,W=0.4,A=5.84,P=30);\n"
    "  device NMOS $10 (S=VSS,G=IN,D=$3) (L=0.4,W=3.1,AS=1.86,AD=1.86,PS=7.4,PD=7.4);\n"
    "end;\n",
    true /*exact parameter compare*/
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_capres_nets.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(5_ResAndCapWithBulkExtraction)
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
  unsigned int cap        = define_layer (ly, lmap, 10);
  unsigned int res        = define_layer (ly, lmap, 11);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "devices_with_bulk_test.oas");

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
  db::Region rpoly_all (db::RecursiveShapeIterator (ly, tc, poly), dss);
  db::Region rpoly_lbl (db::RecursiveShapeIterator (ly, tc, poly_lbl), dss);
  db::Region rdiff_cont (db::RecursiveShapeIterator (ly, tc, diff_cont), dss);
  db::Region rpoly_cont (db::RecursiveShapeIterator (ly, tc, poly_cont), dss);
  db::Region rmetal1 (db::RecursiveShapeIterator (ly, tc, metal1), dss);
  db::Region rmetal1_lbl (db::RecursiveShapeIterator (ly, tc, metal1_lbl), dss);
  db::Region rvia1 (db::RecursiveShapeIterator (ly, tc, via1), dss);
  db::Region rmetal2 (db::RecursiveShapeIterator (ly, tc, metal2), dss);
  db::Region rmetal2_lbl (db::RecursiveShapeIterator (ly, tc, metal2_lbl), dss);
  db::Region rcap (db::RecursiveShapeIterator (ly, tc, cap), dss);
  db::Region rres (db::RecursiveShapeIterator (ly, tc, res), dss);
  db::Region rbulk (dss);

  //  derived regions

  db::Region rpoly     = rpoly_all - rres;
  db::Region rpoly_res = rpoly_all & rres;
  db::Region rpoly_res_nw = rpoly_res & rnwell;
  db::Region rpoly_res_sub = rpoly_res - rnwell;

  db::Region rpactive  = ractive & rnwell;
  db::Region rpgate    = rpactive & rpoly;
  db::Region rpsd      = rpactive - rpgate;

  db::Region rnactive  = ractive - rnwell;
  db::Region rngate    = rnactive & rpoly;
  db::Region rnsd      = rnactive - rngate;

  db::Region rcap1     = rmetal1 & rcap;
  db::Region rcap2     = rmetal2 & rcap;

  db::Region rcap1_nw  = rcap1 & rnwell;
  db::Region rcap2_nw  = rcap2 & rnwell;

  db::Region rcap1_sub = rcap1 - rnwell;
  db::Region rcap2_sub = rcap2 - rnwell;

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int lgate     = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> Gate
  unsigned int lsd       = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> Source/Drain
  unsigned int lpdiff    = ly.insert_layer (db::LayerProperties (12, 0));      // 12/0 -> P Diffusion
  unsigned int lndiff    = ly.insert_layer (db::LayerProperties (13, 0));      // 13/0 -> N Diffusion
  unsigned int lpoly_res = ly.insert_layer (db::LayerProperties (14, 0));      // 14/0 -> Resistor Poly
  unsigned int lcap1     = ly.insert_layer (db::LayerProperties (15, 0));      // 15/0 -> Cap 1
  unsigned int lcap2     = ly.insert_layer (db::LayerProperties (16, 0));      // 16/0 -> Cap 2

  rpgate.insert_into (&ly, tc.cell_index (), lgate);
  rngate.insert_into (&ly, tc.cell_index (), lgate);
  rpsd.insert_into (&ly, tc.cell_index (), lsd);
  rnsd.insert_into (&ly, tc.cell_index (), lsd);
  rpsd.insert_into (&ly, tc.cell_index (), lpdiff);
  rnsd.insert_into (&ly, tc.cell_index (), lndiff);
  rpoly_res.insert_into (&ly, tc.cell_index (), lpoly_res);
  rcap1.insert_into (&ly, tc.cell_index (), lcap1);
  rcap2.insert_into (&ly, tc.cell_index (), lcap2);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS4Transistor nmos_ex ("NMOS");
  db::NetlistDeviceExtractorResistorWithBulk res_substrate_ex ("POLY_RES_SUBSTRATE", 50.0);
  db::NetlistDeviceExtractorResistorWithBulk res_nwell_ex ("POLY_RES_NWELL", 50.0);
  db::NetlistDeviceExtractorCapacitorWithBulk cap_substrate_ex ("MIM_CAP_SUBSTRATE", 1.0e-15);
  db::NetlistDeviceExtractorCapacitorWithBulk cap_nwell_ex ("MIM_CAP_NWELL", 1.0e-15);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["W"] = &rnwell;
  //  terminal patches
  dl["tG"] = &rpoly;
  dl["tS"] = &rpsd;
  dl["tD"] = &rpsd;
  pmos_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["W"] = &rbulk;
  //  terminal patches
  dl["tG"] = &rpoly;
  dl["tS"] = &rnsd;
  dl["tD"] = &rnsd;
  nmos_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["R"] = &rpoly_res_sub;
  dl["C"] = &rpoly;
  dl["W"] = &rbulk;
  //  terminal patches
  dl["tA"] = &rpoly;
  dl["tB"] = &rpoly;
  res_substrate_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["R"] = &rpoly_res_nw;
  dl["C"] = &rpoly;
  dl["W"] = &rnwell;
  //  terminal patches
  dl["tA"] = &rpoly;
  dl["tB"] = &rpoly;
  res_nwell_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["P1"] = &rcap1_sub;
  dl["P2"] = &rcap2_sub;
  dl["W"] = &rbulk;
  //  terminal patches
  dl["tA"] = &rmetal1;
  dl["tB"] = &rmetal2;
  cap_substrate_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["P1"] = &rcap1_nw;
  dl["P2"] = &rcap2_nw;
  dl["W"] = &rnwell;
  //  terminal patches
  dl["tA"] = &rmetal1;
  dl["tB"] = &rmetal2;
  cap_nwell_ex.extract (dss, 0, dl, nl, cl);


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
  //  Global nets
  conn.connect_global (rbulk, "BULK");
  conn.connect_global (rnwell, "NWELL");

  //  extract the nets

  std::list<tl::GlobPattern> gp;
  gp.push_back (tl::GlobPattern ("*"));
  net_ex.set_joined_net_names (gp);
  net_ex.extract_nets (dss, 0, conn, nl, cl);

  //  Flatten device circuits

  std::vector<std::string> circuits_to_flatten;
  circuits_to_flatten.push_back ("RES");
  circuits_to_flatten.push_back ("RES_MEANDER");
  circuits_to_flatten.push_back ("TRANS");
  circuits_to_flatten.push_back ("TRANS2");

  for (std::vector<std::string>::const_iterator i = circuits_to_flatten.begin (); i != circuits_to_flatten.end (); ++i) {
    db::Circuit *c = nl.circuit_by_name (*i);
    tl_assert (c != 0);
    nl.flatten_circuit (c);
  }

  //  cleanup + completion
  nl.combine_devices ();
  nl.make_top_level_pins ();
  nl.purge ();

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
  dump_nets_to_layout (nl, cl, ly, dump_map, cm, true /*with device cells*/);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit TOP (VSS=VSS,IN=IN,OUT=OUT,VDD=VDD,BULK=BULK,NWELL=NWELL);\n"
    "  device PMOS $1 (S=VDD,G=$4,D=OUT,B=NWELL) (L=0.4,W=2.3,AS=1.38,AD=1.38,PS=5.8,PD=5.8);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=$3,B=NWELL) (L=0.4,W=2.3,AS=1.38,AD=1.38,PS=5.8,PD=5.8);\n"
    "  device NMOS $3 (S=VSS,G=$4,D=OUT,B=BULK) (L=0.4,W=4.6,AS=2.185,AD=2.185,PS=8.8,PD=8.8);\n"
    "  device MIM_CAP_SUBSTRATE $5 (A=$4,B=VSS,W=BULK) (C=1.334e-14,A=13.34,P=15);\n"
    "  device MIM_CAP_NWELL $6 (A=$4,B=VSS,W=NWELL) (C=1.288e-14,A=12.88,P=14.8);\n"
    "  device POLY_RES_NWELL $7 (A=$3,B=$4,W=NWELL) (R=750,L=6,W=0.4,A=2.4,P=13.6);\n"
    "  device POLY_RES_SUBSTRATE $9 (A=$4,B=VSS,W=BULK) (R=1825,L=14.6,W=0.4,A=5.84,P=30);\n"
    "  device NMOS $10 (S=VSS,G=IN,D=$3,B=BULK) (L=0.4,W=3.1,AS=1.86,AD=1.86,PS=7.4,PD=7.4);\n"
    "end;\n",
    true /*exact parameter compare*/
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_capres_with_bulk_nets.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(6_BJT3TransistorExtraction)
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
  unsigned int pplus      = define_layer (ly, lmap, 9);
  unsigned int nplus      = define_layer (ly, lmap, 10);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "bipolar_devices_test.oas");

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
  db::Region rpplus (db::RecursiveShapeIterator (ly, tc, pplus), dss);
  db::Region rnplus (db::RecursiveShapeIterator (ly, tc, nplus), dss);
  db::Region rbulk (dss);

  //  derived regions

  db::Region rpactive  = ractive & rnwell;
  db::Region rbase     = rpactive.selected_not_interacting (rpoly).selected_interacting (rpplus);
  db::Region rpactive_mos = rpactive - rbase;

  db::Region rpgate    = rpactive_mos & rpoly;
  db::Region rpsd      = rpactive_mos - rpgate;

  db::Region rnactive  = ractive - rnwell;
  db::Region rngate    = rnactive & rpoly;
  db::Region rnsd      = rnactive - rngate;

  db::Region rntie     = rnwell & rnplus;
  db::Region remitter  = rpplus & rbase;

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int lgate     = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> Gate
  unsigned int lsd       = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> Source/Drain
  unsigned int lpdiff    = ly.insert_layer (db::LayerProperties (12, 0));      // 12/0 -> P Diffusion
  unsigned int lndiff    = ly.insert_layer (db::LayerProperties (13, 0));      // 13/0 -> N Diffusion
  unsigned int lbase     = ly.insert_layer (db::LayerProperties (14, 0));      // 14/0 -> Base
  unsigned int lemitter  = ly.insert_layer (db::LayerProperties (15, 0));      // 15/0 -> Base
  unsigned int lntie     = ly.insert_layer (db::LayerProperties (16, 0));      // 16/0 -> N Tiedown

  rpgate.insert_into (&ly, tc.cell_index (), lgate);
  rngate.insert_into (&ly, tc.cell_index (), lgate);
  rpsd.insert_into (&ly, tc.cell_index (), lsd);
  rnsd.insert_into (&ly, tc.cell_index (), lsd);
  rpsd.insert_into (&ly, tc.cell_index (), lpdiff);
  rnsd.insert_into (&ly, tc.cell_index (), lndiff);
  rbase.insert_into (&ly, tc.cell_index (), lbase);
  remitter.insert_into (&ly, tc.cell_index (), lemitter);
  rntie.insert_into (&ly, tc.cell_index (), lntie);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS4Transistor nmos_ex ("NMOS");
  db::NetlistDeviceExtractorBJT3Transistor bjt_ex ("PNP");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["W"] = &rnwell;
  //  terminal patches
  dl["tG"] = &rpoly;
  dl["tS"] = &rpsd;
  dl["tD"] = &rpsd;
  pmos_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["W"] = &rbulk;
  //  terminal patches
  dl["tG"] = &rpoly;
  dl["tS"] = &rnsd;
  dl["tD"] = &rnsd;
  nmos_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["E"] = &remitter;
  dl["B"] = &rbase;
  dl["C"] = &rbulk;
  //  terminal patches
  dl["tB"] = &rnwell;
  bjt_ex.extract (dss, 0, dl, nl, cl);


  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;
  //  Intra-layer
  conn.connect (rnwell);
  conn.connect (rpsd);
  conn.connect (rnsd);
  conn.connect (rbase);
  conn.connect (remitter);
  conn.connect (rntie);
  conn.connect (rpoly);
  conn.connect (rdiff_cont);
  conn.connect (rpoly_cont);
  conn.connect (rmetal1);
  conn.connect (rvia1);
  conn.connect (rmetal2);
  //  Inter-layer
  conn.connect (rntie,      rnwell);
  conn.connect (rntie,      rdiff_cont);
  conn.connect (remitter,   rdiff_cont);
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
  //  Global nets
  conn.connect_global (rbulk, "BULK");

  //  extract the nets

  std::list<tl::GlobPattern> gp;
  gp.push_back (tl::GlobPattern ("*"));
  net_ex.set_joined_net_names (gp);
  net_ex.extract_nets (dss, 0, conn, nl, cl);

  //  Flatten device circuits

  std::vector<std::string> circuits_to_flatten;
  circuits_to_flatten.push_back ("TRANS");
  circuits_to_flatten.push_back ("TRANS2");

  for (std::vector<std::string>::const_iterator i = circuits_to_flatten.begin (); i != circuits_to_flatten.end (); ++i) {
    db::Circuit *c = nl.circuit_by_name (*i);
    tl_assert (c != 0);
    nl.flatten_circuit (c);
  }

  //  cleanup + completion
  nl.combine_devices ();
  nl.make_top_level_pins ();
  nl.purge ();

  EXPECT_EQ (all_net_names_unique (nl), true);

  //  debug layers produced for nets
  //    201/0 -> n well
  //    203/0 -> Poly
  //    204/0 -> Diffusion contacts
  //    205/0 -> Poly contacts
  //    206/0 -> Metal1
  //    207/0 -> Via1
  //    208/0 -> Metal2
  //    210/0 -> N source/drain
  //    211/0 -> P source/drain
  //    212/0 -> Emitter
  //    213/0 -> N tiedown
  std::map<unsigned int, unsigned int> dump_map;
  dump_map [layer_of (rpsd)      ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [layer_of (rnsd)      ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [layer_of (remitter)  ] = ly.insert_layer (db::LayerProperties (212, 0));
  dump_map [layer_of (rntie)     ] = ly.insert_layer (db::LayerProperties (213, 0));
  dump_map [layer_of (rnwell)    ] = ly.insert_layer (db::LayerProperties (201, 0));
  dump_map [layer_of (rpoly)     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [layer_of (rdiff_cont)] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [layer_of (rpoly_cont)] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [layer_of (rmetal1)   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [layer_of (rvia1)     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [layer_of (rmetal2)   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = dss.cell_mapping_to_original (0, &ly, tc.cell_index ());
  dump_nets_to_layout (nl, cl, ly, dump_map, cm, true /*with device cells*/);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit TOP (VSS=VSS,IN=IN,OUT=OUT,VDD=VDD,BULK=BULK);\n"
    "  device PMOS $1 (S=$4,G=VSS,D=VDD,B=VDD) (L=0.4,W=2.3,AS=1.38,AD=1.38,PS=5.8,PD=5.8);\n"
    "  device PMOS $2 (S=VDD,G=$4,D=OUT,B=VDD) (L=0.4,W=2.3,AS=1.38,AD=1.38,PS=5.8,PD=5.8);\n"
    "  device PMOS $3 (S=VDD,G=IN,D=$3,B=VDD) (L=0.4,W=2.3,AS=1.38,AD=1.38,PS=5.8,PD=5.8);\n"
    "  device NMOS $4 (S=VSS,G=$4,D=OUT,B=BULK) (L=0.4,W=4.6,AS=2.185,AD=2.185,PS=8.8,PD=8.8);\n"
    "  device PNP $6 (C=BULK,B=$3,E=$3) (AE=3.06,PE=7,AB=25.2,PB=21.2,AC=25.2,PC=21.2,NE=1);\n"
    "  device PNP $7 (C=BULK,B=$3,E=$4) (AE=6.12,PE=14,AB=25.2,PB=21.2,AC=25.2,PC=21.2,NE=2);\n"
    "  device NMOS $9 (S=VSS,G=IN,D=$3,B=BULK) (L=0.4,W=3.1,AS=1.86,AD=1.86,PS=7.4,PD=7.4);\n"
    "end;\n",
    true /*exact parameter compare*/
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "bipolar_devices_nets.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(7_DiodeExtraction)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int nwell      = define_layer (ly, lmap, 1);
  unsigned int active     = define_layer (ly, lmap, 2);
  unsigned int diff_cont  = define_layer (ly, lmap, 4);
  unsigned int metal1     = define_layer (ly, lmap, 6);
  unsigned int metal1_lbl = define_layer (ly, lmap, 6, 1);
  unsigned int pplus      = define_layer (ly, lmap, 9);
  unsigned int nplus      = define_layer (ly, lmap, 10);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "diode_devices_test.oas");

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
  db::Region rdiff_cont (db::RecursiveShapeIterator (ly, tc, diff_cont), dss);
  db::Region rmetal1 (db::RecursiveShapeIterator (ly, tc, metal1), dss);
  db::Region rmetal1_lbl (db::RecursiveShapeIterator (ly, tc, metal1_lbl), dss);
  db::Region rpplus (db::RecursiveShapeIterator (ly, tc, pplus), dss);
  db::Region rnplus (db::RecursiveShapeIterator (ly, tc, nplus), dss);

  //  derived regions

  db::Region rn = ractive & rnwell;
  db::Region rntie     = rnwell & rnplus;

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int ln    = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> N layer
  unsigned int lntie = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> N contact
  rn.insert_into (&ly, tc.cell_index (), ln);
  rntie.insert_into (&ly, tc.cell_index (), lntie);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorDiode diode_ex ("DIODE");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["N"] = &rn;
  dl["P"] = &rpplus;
  dl["tC"] = &rnwell;
  diode_ex.extract (dss, 0, dl, nl, cl);


  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;
  //  Intra-layer
  conn.connect (rnwell);
  conn.connect (rntie);
  conn.connect (rpplus);
  conn.connect (rdiff_cont);
  conn.connect (rmetal1);
  //  Inter-layer
  conn.connect (rntie,      rnwell);
  conn.connect (rntie,      rdiff_cont);
  conn.connect (rpplus,     rdiff_cont);
  conn.connect (rdiff_cont, rmetal1);
  conn.connect (rmetal1,    rmetal1_lbl);   //  attaches labels

  //  extract the nets

  std::list<tl::GlobPattern> gp;
  gp.push_back (tl::GlobPattern ("*"));
  net_ex.set_joined_net_names (gp);
  net_ex.extract_nets (dss, 0, conn, nl, cl);

  //  cleanup + completion
  nl.combine_devices ();
  nl.make_top_level_pins ();
  nl.purge ();

  EXPECT_EQ (all_net_names_unique (nl), true);

  //  debug layers produced for nets
  //    201/0 -> n well
  //    204/0 -> Diffusion contacts
  //    206/0 -> Metal1
  //    210/0 -> N tiedown
  std::map<unsigned int, unsigned int> dump_map;
  dump_map [layer_of (rntie)     ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [layer_of (rnwell)    ] = ly.insert_layer (db::LayerProperties (201, 0));
  dump_map [layer_of (rdiff_cont)] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [layer_of (rmetal1)   ] = ly.insert_layer (db::LayerProperties (206, 0));

  //  write nets to layout
  db::CellMapping cm = dss.cell_mapping_to_original (0, &ly, tc.cell_index ());
  dump_nets_to_layout (nl, cl, ly, dump_map, cm, true /*with device cells*/);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit TOP (A=A,C=C);\n"
    "  device DIODE $1 (A=A,C=C) (A=9.18,P=21);\n"
    "end;\n",
    true /*exact parameter compare*/
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "diode_devices_nets.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(8_DiodeExtractionScaled)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int nwell      = define_layer (ly, lmap, 1);
  unsigned int active     = define_layer (ly, lmap, 2);
  unsigned int diff_cont  = define_layer (ly, lmap, 4);
  unsigned int metal1     = define_layer (ly, lmap, 6);
  unsigned int metal1_lbl = define_layer (ly, lmap, 6, 1);
  unsigned int pplus      = define_layer (ly, lmap, 9);
  unsigned int nplus      = define_layer (ly, lmap, 10);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "diode_devices_test.oas");

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
  db::Region rdiff_cont (db::RecursiveShapeIterator (ly, tc, diff_cont), dss);
  db::Region rmetal1 (db::RecursiveShapeIterator (ly, tc, metal1), dss);
  db::Region rmetal1_lbl (db::RecursiveShapeIterator (ly, tc, metal1_lbl), dss);
  db::Region rpplus (db::RecursiveShapeIterator (ly, tc, pplus), dss);
  db::Region rnplus (db::RecursiveShapeIterator (ly, tc, nplus), dss);

  //  derived regions

  db::Region rn = ractive & rnwell;
  db::Region rntie     = rnwell & rnplus;

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int ln    = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> N layer
  unsigned int lntie = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> N contact
  rn.insert_into (&ly, tc.cell_index (), ln);
  rntie.insert_into (&ly, tc.cell_index (), lntie);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorDiode diode_ex ("DIODE");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["N"] = &rn;
  dl["P"] = &rpplus;
  dl["tC"] = &rnwell;
  diode_ex.extract (dss, 0, dl, nl, cl, 2.0);

  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;
  //  Intra-layer
  conn.connect (rnwell);
  conn.connect (rntie);
  conn.connect (rpplus);
  conn.connect (rdiff_cont);
  conn.connect (rmetal1);
  //  Inter-layer
  conn.connect (rntie,      rnwell);
  conn.connect (rntie,      rdiff_cont);
  conn.connect (rpplus,     rdiff_cont);
  conn.connect (rdiff_cont, rmetal1);
  conn.connect (rmetal1,    rmetal1_lbl);   //  attaches labels

  //  extract the nets

  std::list<tl::GlobPattern> gp;
  gp.push_back (tl::GlobPattern ("*"));
  net_ex.set_joined_net_names (gp);
  net_ex.extract_nets (dss, 0, conn, nl, cl);

  //  cleanup + completion
  nl.combine_devices ();
  nl.make_top_level_pins ();
  nl.purge ();

  EXPECT_EQ (all_net_names_unique (nl), true);

  //  debug layers produced for nets
  //    201/0 -> n well
  //    204/0 -> Diffusion contacts
  //    206/0 -> Metal1
  //    210/0 -> N tiedown
  std::map<unsigned int, unsigned int> dump_map;
  dump_map [layer_of (rntie)     ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [layer_of (rnwell)    ] = ly.insert_layer (db::LayerProperties (201, 0));
  dump_map [layer_of (rdiff_cont)] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [layer_of (rmetal1)   ] = ly.insert_layer (db::LayerProperties (206, 0));

  //  write nets to layout
  db::CellMapping cm = dss.cell_mapping_to_original (0, &ly, tc.cell_index ());
  dump_nets_to_layout (nl, cl, ly, dump_map, cm, true /*with device cells*/);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit TOP (A=A,C=C);\n"
    "  device DIODE $1 (A=A,C=C) (A=36.72,P=42);\n"
    "end;\n",
    true /*exact parameter compare*/
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "diode_devices_nets.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(9_StrictDeviceExtraction)
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
  unsigned int source     = define_layer (ly, lmap, 10);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l9.gds");

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
  db::Region rsource (db::RecursiveShapeIterator (ly, tc, source), dss);

  //  derived regions

  db::Region rpactive = ractive & rnwell;
  db::Region rpgate   = rpactive & rpoly;
  db::Region rpsd     = rpactive - rpgate;
  db::Region rps      = rpsd & rsource;
  db::Region rpd      = rpsd - rsource;

  db::Region rnactive = ractive - rnwell;
  db::Region rngate   = rnactive & rpoly;
  db::Region rnsd     = rnactive - rngate;
  db::Region rns      = rnsd & rsource;
  db::Region rnd      = rnsd - rsource;

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int lgate  = ly.insert_layer (db::LayerProperties (20, 0));      // 10/0 -> Gate
  unsigned int lsd    = ly.insert_layer (db::LayerProperties (21, 0));      // 11/0 -> Source/Drain
  unsigned int lpdiff = ly.insert_layer (db::LayerProperties (22, 0));      // 12/0 -> P Diffusion
  unsigned int lndiff = ly.insert_layer (db::LayerProperties (23, 0));      // 13/0 -> N Diffusion

  rpgate.insert_into (&ly, tc.cell_index (), lgate);
  rngate.insert_into (&ly, tc.cell_index (), lgate);
  rps.insert_into (&ly, tc.cell_index (), lsd);
  rpd.insert_into (&ly, tc.cell_index (), lsd);
  rns.insert_into (&ly, tc.cell_index (), lsd);
  rnd.insert_into (&ly, tc.cell_index (), lsd);
  rpsd.insert_into (&ly, tc.cell_index (), lpdiff);
  rnsd.insert_into (&ly, tc.cell_index (), lndiff);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS3Transistor pmos_ex ("PMOS", true /*strict*/);
  db::NetlistDeviceExtractorMOS3Transistor nmos_ex ("NMOS", true /*strict*/);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &rps;
  dl["D"] = &rpd;
  dl["G"] = &rpgate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  pmos_ex.extract (dss, 0, dl, nl, cl);

  dl["S"] = &rns;
  dl["D"] = &rnd;
  dl["G"] = &rngate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  nmos_ex.extract (dss, 0, dl, nl, cl);

  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;
  //  Intra-layer
  conn.connect (rps);
  conn.connect (rpd);
  conn.connect (rns);
  conn.connect (rnd);
  conn.connect (rpoly);
  conn.connect (rdiff_cont);
  conn.connect (rpoly_cont);
  conn.connect (rmetal1);
  conn.connect (rvia1);
  conn.connect (rmetal2);
  //  Inter-layer
  conn.connect (rps,        rdiff_cont);
  conn.connect (rpd,        rdiff_cont);
  conn.connect (rns,        rdiff_cont);
  conn.connect (rnd,        rdiff_cont);
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
  dump_map [layer_of (rps)       ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [layer_of (rpd)       ] = ly.insert_layer (db::LayerProperties (211, 0));
  dump_map [layer_of (rns)       ] = ly.insert_layer (db::LayerProperties (212, 0));
  dump_map [layer_of (rnd)       ] = ly.insert_layer (db::LayerProperties (213, 0));
  dump_map [layer_of (rpoly)     ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [layer_of (rdiff_cont)] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [layer_of (rpoly_cont)] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [layer_of (rmetal1)   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [layer_of (rvia1)     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [layer_of (rmetal2)   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = dss.cell_mapping_to_original (0, &ly, tc.cell_index ());
  dump_nets_to_layout (nl, cl, ly, dump_map, cm);

  std::string nl_au_string =
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
    "  device PMOS $1 (S=$5,G=IN,D=$2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$4,G=IN,D=$2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=$5,$2=OUT,$3=$2);\n"
    "  subcircuit TRANS $4 ($1=$4,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n";

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl, nl_au_string);

  {
    //  compare vs. non-strict device classes
    db::Netlist au_nl;
    //  non-strict
    db::DeviceClass *dc;
    dc = new db::DeviceClassMOS3Transistor ();
    dc->set_name ("PMOS");
    au_nl.add_device_class (dc);
    dc = new db::DeviceClassMOS3Transistor ();
    dc->set_name ("NMOS");
    au_nl.add_device_class (dc);
    au_nl.from_string (nl_au_string);

    CHECKPOINT ();
    db::compare_netlist (_this, nl, au_nl);
  }

  {
    std::string nl_au_string_wrong_terminals = nl_au_string;
    nl_au_string_wrong_terminals = tl::replaced (nl_au_string_wrong_terminals, "(S=$5,G=IN,D=$2)", "(S=$2,G=IN,D=$5)");
    nl_au_string_wrong_terminals = tl::replaced (nl_au_string_wrong_terminals, "(S=$4,G=IN,D=$2)", "(S=$2,G=IN,D=$4)");

    //  compare vs. non-strict device classes with WRONG terminal assignment
    db::Netlist au_nl;
    //  non-strict
    db::DeviceClass *dc;
    dc = new db::DeviceClassMOS3Transistor ();
    dc->set_name ("PMOS");
    au_nl.add_device_class (dc);
    dc = new db::DeviceClassMOS3Transistor ();
    dc->set_name ("NMOS");
    au_nl.add_device_class (dc);
    au_nl.from_string (nl_au_string_wrong_terminals);

    db::NetlistComparer comp (0);
    EXPECT_EQ (comp.compare (&nl, &au_nl), false);
  }

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au9.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(10_DeviceExtractionWithBreakoutCells)
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
    fn = tl::combine_path (fn, "device_extract_l10.gds");

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

  unsigned int lpgate = ly.insert_layer (db::LayerProperties (20, 0));      // 20/0 -> P Gate
  unsigned int lngate = ly.insert_layer (db::LayerProperties (21, 0));      // 21/0 -> N Gate
  unsigned int lpdiff = ly.insert_layer (db::LayerProperties (22, 0));      // 22/0 -> P Diffusion
  unsigned int lndiff = ly.insert_layer (db::LayerProperties (23, 0));      // 23/0 -> N Diffusion

  rpgate.insert_into (&ly, tc.cell_index (), lpgate);
  rngate.insert_into (&ly, tc.cell_index (), lngate);
  rpsd.insert_into (&ly, tc.cell_index (), lpdiff);
  rnsd.insert_into (&ly, tc.cell_index (), lndiff);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS3Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS3Transistor nmos_ex ("NMOS");

  db::NetlistDeviceExtractor::input_layers dl;

  dss.push_state ();
  std::set<db::cell_index_type> boc;
  boc.insert (dss.layout (0).cell_by_name ("INV").second);
  dss.set_breakout_cells (0, boc);

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  pmos_ex.extract (dss, 0, dl, nl, cl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  nmos_ex.extract (dss, 0, dl, nl, cl);

  dss.pop_state ();

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

  std::string nl_au_string =
    "circuit INVCHAIN ();"
    "  subcircuit INV3 $1 ($1=$9,$2=$7,$3=$8,$4=IN,$5=$9,$6=$6,$7=$2,$8=$6,$9=$4,$10=$3,$11=VDD,$12=VSS);"
    "  subcircuit INV2 $2 ($1=$8,$2=$11,$3=$9,$4=$10,$5=$8,$6=$11,$7=$3,$8=OUT,$9=$3,$10=OUT,$11=$6,$12=$5,$13=VDD,$14=VSS);"
    "  subcircuit INV $3 ($1=VSS,$2=VDD,$3=$11,$4=OUT,$5=$5,$6=$5,$7=$10,$8=$10);"
    "end;"
    "circuit INV2 ($1=$I16,$2=$I15,$3=$I14,$4=$I13,$5=$I12,$6=$I11,$7=$I10,$8=$I9,$9=$I8,$10=$I7,$11=$I6,$12=$I5,$13=$I4,$14=$I2);"
    "  subcircuit INV $1 ($1=$I2,$2=$I4,$3=$I14,$4=$I6,$5=$I8,$6=$I10,$7=$I16,$8=$I12);"
    "  subcircuit INV $2 ($1=$I2,$2=$I4,$3=$I13,$4=$I5,$5=$I7,$6=$I9,$7=$I15,$8=$I11);"
    "end;"
    "circuit INV ($1=$1,$2=$2,$3=$3,$4=$4,$5=$5,$6=$9,$7=$I8,$8=$I7);"
    "  device PMOS $1 (S=$4,G=$3,D=$2) (L=0.25,W=0.95,AS=0.79325,AD=0.26125,PS=3.57,PD=1.5);"
    "  device PMOS $2 (S=$2,G=$I8,D=$5) (L=0.25,W=0.95,AS=0.26125,AD=0.03325,PS=1.5,PD=1.97);"
    "  device NMOS $3 (S=$4,G=$3,D=$1) (L=0.25,W=0.95,AS=0.79325,AD=0.26125,PS=3.57,PD=1.5);"
    "  device NMOS $4 (S=$1,G=$I7,D=$9) (L=0.25,W=0.95,AS=0.26125,AD=0.03325,PS=1.5,PD=1.97);"
    "  subcircuit TRANS $1 ($1=$4,$2=$1,$3=$3);"
    "  subcircuit TRANS $2 ($1=$4,$2=$2,$3=$3);"
    "end;"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);"
    "end;"
    "circuit INV3 ($1=$I17,$2=$I16,$3=$I15,$4=$I14,$5=$I12,$6=$I11,$7=$I9,$8=$I8,$9=$I7,$10=$I5,$11=$I4,$12=$I2);"
    "  subcircuit INV $1 ($1=$I2,$2=$I4,$3=$I15,$4=$I5,$5=$I8,$6=$I11,$7=$I17,$8=$I12);"
    "  subcircuit INV $2 ($1=$I2,$2=$I4,$3=$I14,$4=$I9,$5=$I7,$6=$I7,$7=$I16,$8=$I16);"
    "  subcircuit INV $3 ($1=$I2,$2=$I4,$3=$I16,$4=$I7,$5=$I9,$6=$I9,$7=$I14,$8=$I14);"
    "end;"
  ;

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl, nl_au_string);

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au10.gds");

  db::compare_layouts (_this, ly, au);
}

TEST(11_DeviceExtractionWithSameClass)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int rmarker    = define_layer (ly, lmap, 1);
  unsigned int poly       = define_layer (ly, lmap, 2);
  unsigned int diff       = define_layer (ly, lmap, 3);
  unsigned int contact    = define_layer (ly, lmap, 4);
  unsigned int metal      = define_layer (ly, lmap, 5);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_l11.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region rrmarker (db::RecursiveShapeIterator (ly, tc, rmarker), dss);
  db::Region rpoly (db::RecursiveShapeIterator (ly, tc, poly), dss);
  db::Region rdiff (db::RecursiveShapeIterator (ly, tc, diff), dss);
  db::Region rcontact (db::RecursiveShapeIterator (ly, tc, contact), dss);
  db::Region rmetal (db::RecursiveShapeIterator (ly, tc, metal), dss);

  //  derived regions

  db::Region rpoly_cap = rpoly - rrmarker;
  db::Region rpoly_res = rpoly & rrmarker;
  db::Region rdiff_cap = rdiff - rrmarker;
  db::Region rdiff_res = rdiff & rrmarker;

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorResistor polyres_ex ("RES", 50.0);
  db::NetlistDeviceExtractorResistor diffres_ex ("RES", 150.0);

  db::NetlistDeviceExtractor::input_layers dl;
  dl["R"] = &rpoly_res;
  dl["C"] = &rpoly_cap;
  polyres_ex.extract (dss, 0, dl, nl, cl);

  dl.clear ();
  dl["R"] = &rdiff_res;
  dl["C"] = &rdiff_cap;
  diffres_ex.extract (dss, 0, dl, nl, cl);

  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;
  //  Intra-layer
  conn.connect (rpoly_cap);
  conn.connect (rdiff_cap);
  conn.connect (rcontact);
  //  Inter-layer
  conn.connect (rpoly_cap,  rcontact);
  conn.connect (rdiff_cap,  rcontact);
  conn.connect (rmetal,     rcontact);

  //  extract the nets

  net_ex.extract_nets (dss, 0, conn, nl, cl);

  std::string nl_au_string =
    "circuit TOP ();\n"
    "  device RES $1 (A=$1,B=$2) (R=175,L=2.8,W=0.8,A=0.56,P=3.6);\n"
    "  device RES $2 (A=$2,B=$3) (R=175,L=2.8,W=0.8,A=0.56,P=3.6);\n"
    "  device RES $3 (A=$3,B=$4) (R=525,L=2.8,W=0.8,A=0.56,P=3.6);\n"
    "end;\n"
  ;

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl, nl_au_string);

  nl.combine_devices ();

  std::string nl_au_string_post =
    "circuit TOP ();\n"
    "  device RES $3 (A=$1,B=$4) (R=875,L=8.4,W=0.8,A=1.68,P=10.8);\n"
    "end;\n"
  ;

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl, nl_au_string_post);
}

TEST(12_FloatingSubcircuitExtraction)
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
    fn = tl::combine_path (fn, "device_extract_l1_floating_subcircuits.gds");

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

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

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

  db::NetlistExtractor net_ex;
  net_ex.set_include_floating_subcircuits (true);
  net_ex.extract_nets (dss, 0, conn, nl, cl);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit RINGO ();\n"
    "  subcircuit INV2 $1 (IN=FB,$2=$I29,OUT=$I1,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $2 (IN=$I1,$2=$I30,OUT=$I2,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2X $3 ($1=VDD,$2=VSS,$3=$I25,$4=$I11);\n"
    "  subcircuit TRANSISO $4 ();\n"   //  effect of "include floating subcircuits"!
    "  subcircuit INV2 $5 (IN=$I9,$2=$I31,OUT=$I3,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $6 (IN=$I3,$2=$I32,OUT=$I25,$4=VSS,$5=VDD);\n"
    "  subcircuit TRANSISOB $7 ();\n"   //  effect of "include floating subcircuits"!
    "  subcircuit INV2 $7 (IN=$I11,$2=$I33,OUT=$I5,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2 $8 (IN=$I5,$2=FB,OUT=OSC,$4=VSS,$5=VDD);\n"
    "  subcircuit INV2X $9 ($1=VDD,$2=VSS,$3=$I2,$4=$I9);\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=$4,$2=OUT,$3=$2);\n"
    "  subcircuit TRANS $4 ($1=$5,$2=OUT,$3=$2);\n"
    "end;\n"
    "circuit TRANS ($1=$1,$2=$2,$3=$3);\n"
    "end;\n"
    "circuit INV2X ($1=$I4,$2=$I3,$3=$I2,$4=$I1);\n"
    "  subcircuit INV2 $1 (IN=$I2,$2=$I6,OUT=$I5,$4=$I3,$5=$I4);\n"
    "  subcircuit INV2 $2 (IN=$I5,$2=$I7,OUT=$I1,$4=$I3,$5=$I4);\n"
    "  subcircuit TRANSISO $3 ();\n"   //  effect of "include floating subcircuits"!
    "  subcircuit TRANSISO $4 ();\n"   //  effect of "include floating subcircuits"!
    "  subcircuit TRANSISOB $5 ();\n"   //  effect of "include floating subcircuits"!
    "end;\n"
    "circuit TRANSISO ();\n"
    "  device NMOS $1 (S=$1,G=$2,D=$1) (L=0.25,W=0.95,AS=0.49875,AD=0.49875,PS=2.95,PD=2.95);\n"
    "end;\n"
    "circuit TRANSISOB ();\n"
    "  device NMOS $1 (S=$1,G=$2,D=$1) (L=0.25,W=0.95,AS=0.49875,AD=0.49875,PS=2.95,PD=2.95);\n"
    "end;\n"
  );
}

TEST(13_RemoveDummyPins)
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
    fn = tl::combine_path (fn, "issue-425.gds");

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

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

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

  db::NetlistExtractor net_ex;
  net_ex.set_include_floating_subcircuits (true);
  net_ex.extract_nets (dss, 0, conn, nl, cl);

  nl.simplify ();

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit RINGO (FB=FB,VSS=VSS,VDD=VDD);\n"
    "  subcircuit INV2 $1 (IN=FB,OUT=$I25,$3=VSS,$4=VDD);\n"
    "  subcircuit INV2 $2 (IN=$I25,OUT=$I1,$3=VSS,$4=VDD);\n"
    "  subcircuit INV2 $3 (IN=$I1,OUT=$I2,$3=VSS,$4=VDD);\n"
    "  subcircuit INV2 $4 (IN=$I2,OUT=$I3,$3=VSS,$4=VDD);\n"
    "end;\n"
    "circuit INV2 (IN=IN,OUT=OUT,$3=$4,$4=$5);\n"
    "  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n"
  );
}

TEST(14_JoinNets)
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
    fn = tl::combine_path (fn, "device_extract_l1_join_nets.gds");

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

  //  Global

  db::Region bulk (dss);

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
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS4Transistor nmos_ex ("NMOS");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["W"] = &rnwell;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  pmos_ex.extract (dss, 0, dl, nl, cl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["W"] = &bulk;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  nmos_ex.extract (dss, 0, dl, nl, cl);

  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;

  //  Global nets
  conn.connect_global (bulk, "BULK");
  conn.connect_global (rnwell, "NWELL");

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

  std::list<std::set<std::string> > jn;

  jn.push_back (std::set<std::string> ());
  jn.back ().insert ("BULK");
  jn.back ().insert ("VSS");

  jn.push_back (std::set<std::string> ());
  jn.back ().insert ("NWELL");
  jn.back ().insert ("VDD");

  net_ex.set_joined_nets ("INV2", jn);

  std::list<tl::GlobPattern> gp;
  gp.push_back (tl::GlobPattern ("NEXT"));
  gp.push_back (tl::GlobPattern ("FB"));
  net_ex.set_joined_net_names (gp);

  net_ex.extract_nets (dss, 0, conn, nl, cl);

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
  //    212/0 -> Bulk
  //    213/0 -> NWell
  std::map<unsigned int, unsigned int> dump_map;
  dump_map [layer_of (bulk)      ] = ly.insert_layer (db::LayerProperties (212, 0));
  dump_map [layer_of (rnwell)    ] = ly.insert_layer (db::LayerProperties (213, 0));
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
  dump_nets_to_layout (nl, cl, ly, dump_map, cm, true);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit RINGO ();\n"
    "  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $2 (IN=FB,$2=$I20,OUT=$I19,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $3 (IN=NEXT,$2=$I25,OUT=$I5,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $4 (IN=$I3,$2=$I24,OUT=NEXT,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $5 (IN=$I5,$2=$I26,OUT=$I6,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $6 (IN=$I6,$2=$I27,OUT=$I7,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $7 (IN=$I7,$2=$I28,OUT=$I8,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $8 (IN=$I19,$2=$I21,OUT=$I1,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $9 (IN=$I1,$2=$I22,OUT=$I2,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $10 (IN=$I2,$2=$I23,OUT=$I3,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,VDD=VDD,VSS=VSS);\n"
    "  device PMOS $1 (S=$2,G=IN,D=VDD,B=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$2,D=OUT,B=VDD) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=VSS,B=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$2,D=OUT,B=VSS) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  subcircuit TRANS $1 ($1=$2,$2=VSS,$3=IN);\n"
    "  subcircuit TRANS $2 ($1=$2,$2=VDD,$3=IN);\n"
    "  subcircuit TRANS $3 ($1=VDD,$2=OUT,$3=$2);\n"
    "  subcircuit TRANS $4 ($1=VSS,$2=OUT,$3=$2);\n"
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
    "circuit RINGO (FB=FB,OSC=OSC,NEXT=NEXT,'VDD,VDDZ'='VDD,VDDZ','VSS,VSSZ'='VSS,VSSZ');\n"
    "  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $2 (IN=FB,$2=$I20,OUT=$I19,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $3 (IN=NEXT,$2=$I25,OUT=$I5,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $4 (IN=$I3,$2=$I24,OUT=NEXT,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $5 (IN=$I5,$2=$I26,OUT=$I6,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $6 (IN=$I6,$2=$I27,OUT=$I7,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $7 (IN=$I7,$2=$I28,OUT=$I8,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $8 (IN=$I19,$2=$I21,OUT=$I1,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $9 (IN=$I1,$2=$I22,OUT=$I2,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "  subcircuit INV2 $10 (IN=$I2,$2=$I23,OUT=$I3,VDD='VDD,VDDZ',VSS='VSS,VSSZ');\n"
    "end;\n"
    "circuit INV2 (IN=IN,$2=$2,OUT=OUT,VDD=VDD,VSS=VSS);\n"
    "  device PMOS $1 (S=$2,G=IN,D=VDD,B=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$2,D=OUT,B=VDD) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $3 (S=$2,G=IN,D=VSS,B=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=$2,D=OUT,B=VSS) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1_join_nets.gds");

  db::compare_layouts (_this, ly, au);
}

static
void annotate_soft_connections (db::Netlist &netlist, const db::hier_clusters<db::NetShape> &net_clusters)
{
  db::DeviceClassDiode *soft_diode = new db::DeviceClassDiode ();
  soft_diode->set_name ("SOFT");
  netlist.add_device_class (soft_diode);

  for (auto c = netlist.begin_bottom_up (); c != netlist.end_bottom_up (); ++c) {

    auto clusters = net_clusters.clusters_per_cell (c->cell_index ());

    for (auto n = c->begin_nets (); n != c->end_nets (); ++n) {

      auto soft_connections = clusters.upward_soft_connections (n->cluster_id ());
      for (auto sc = soft_connections.begin (); sc != soft_connections.end (); ++sc) {

        db::Device *sc_device = new db::Device (soft_diode);
        c->add_device (sc_device);

        auto nn = c->net_by_cluster_id (*sc);
        if (nn) {
          sc_device->connect_terminal (db::DeviceClassDiode::terminal_id_C, n.operator-> ());
          sc_device->connect_terminal (db::DeviceClassDiode::terminal_id_A, nn);
        }

      }

    }

  }
}

TEST(15_SoftConnections)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int nwell      = define_layer (ly, lmap, 1);
  unsigned int diff       = define_layer (ly, lmap, 2);
  unsigned int pplus      = define_layer (ly, lmap, 3);
  unsigned int nplus      = define_layer (ly, lmap, 4);
  unsigned int poly       = define_layer (ly, lmap, 5);
  unsigned int contact    = define_layer (ly, lmap, 8);
  unsigned int metal1     = define_layer (ly, lmap, 9);
  unsigned int via1       = define_layer (ly, lmap, 10);
  unsigned int metal2     = define_layer (ly, lmap, 11);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "soft_connections.gds");

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
  db::Region rdiff (db::RecursiveShapeIterator (ly, tc, diff), dss);
  db::Region rpplus (db::RecursiveShapeIterator (ly, tc, pplus), dss);
  db::Region rnplus (db::RecursiveShapeIterator (ly, tc, nplus), dss);
  db::Region rpoly (db::RecursiveShapeIterator (ly, tc, poly), dss);
  db::Region rcontact (db::RecursiveShapeIterator (ly, tc, contact), dss);
  db::Region rmetal1 (db::RecursiveShapeIterator (ly, tc, metal1), dss);
  db::Region rvia1 (db::RecursiveShapeIterator (ly, tc, via1), dss);
  db::Region rmetal2 (db::RecursiveShapeIterator (ly, tc, metal2), dss);

  //  derived regions

  db::Region rdiff_in_nwell = rdiff & rnwell;
  db::Region rpdiff      = rdiff_in_nwell - rnplus;
  db::Region rntie       = rdiff_in_nwell & rnplus;
  db::Region rpgate      = rpdiff & rpoly;
  db::Region rpsd        = rpdiff - rpgate;

  db::Region rdiff_outside_nwell = rdiff - rnwell;
  db::Region rndiff      = rdiff_outside_nwell - rpplus;
  db::Region rptie       = rdiff_outside_nwell & rpplus;
  db::Region rngate      = rndiff & rpoly;
  db::Region rnsd        = rndiff - rngate;

  //  Global

  db::Region bulk (dss);

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int lpgate = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> Gate (p)
  unsigned int lngate = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> Gate (n)
  unsigned int lpsd   = ly.insert_layer (db::LayerProperties (12, 0));      // 12/0 -> Source/Drain (p)
  unsigned int lnsd   = ly.insert_layer (db::LayerProperties (13, 0));      // 13/0 -> Source/Drain (n)
  unsigned int lpdiff = ly.insert_layer (db::LayerProperties (14, 0));      // 14/0 -> P Diffusion
  unsigned int lndiff = ly.insert_layer (db::LayerProperties (15, 0));      // 15/0 -> N Diffusion
  unsigned int lptie  = ly.insert_layer (db::LayerProperties (16, 0));      // 16/0 -> P Tie
  unsigned int lntie  = ly.insert_layer (db::LayerProperties (17, 0));      // 17/0 -> N Tie

  rpgate.insert_into (&ly, tc.cell_index (), lpgate);
  rngate.insert_into (&ly, tc.cell_index (), lngate);
  rpsd.insert_into (&ly, tc.cell_index (), lpsd);
  rnsd.insert_into (&ly, tc.cell_index (), lnsd);
  rpdiff.insert_into (&ly, tc.cell_index (), lpdiff);
  rndiff.insert_into (&ly, tc.cell_index (), lndiff);
  rptie.insert_into (&ly, tc.cell_index (), lptie);
  rntie.insert_into (&ly, tc.cell_index (), lntie);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor pmos_ex ("PMOS");
  db::NetlistDeviceExtractorMOS4Transistor nmos_ex ("NMOS");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["W"] = &rnwell;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  pmos_ex.extract (dss, 0, dl, nl, cl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["W"] = &bulk;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  nmos_ex.extract (dss, 0, dl, nl, cl);

  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;

  //  Global nets
  conn.connect_global (bulk, "BULK");
  conn.soft_connect_global (rptie, "BULK");

  //  Intra-layer
  conn.connect (rpsd);
  conn.connect (rnsd);
  conn.connect (rptie);
  conn.connect (rntie);
  conn.connect (rnwell);
  conn.connect (rpoly);
  conn.connect (rcontact);
  conn.connect (rmetal1);
  conn.connect (rvia1);
  conn.connect (rmetal2);
  //  Inter-layer
  conn.soft_connect (rcontact, rpsd);
  conn.soft_connect (rcontact, rnsd);
  conn.soft_connect (rntie, rnwell);
  conn.soft_connect (rcontact, rptie);
  conn.soft_connect (rcontact, rntie);
  conn.soft_connect (rcontact, rpoly);
  conn.connect (rcontact, rmetal1);
  conn.connect (rvia1, rmetal1);
  conn.connect (rvia1, rmetal2);

  //  extract the nets
  std::list<std::set<std::string> > jn;

  jn.push_back (std::set<std::string> ());
  jn.back ().insert ("BULK");
  jn.back ().insert ("VSS");

  jn.push_back (std::set<std::string> ());
  jn.back ().insert ("NWELL");
  jn.back ().insert ("VDD");

  net_ex.set_joined_nets ("INV2", jn);

  std::list<tl::GlobPattern> gp;
  gp.push_back (tl::GlobPattern ("NEXT"));
  gp.push_back (tl::GlobPattern ("FB"));
  net_ex.set_joined_net_names (gp);

  net_ex.extract_nets (dss, 0, conn, nl, cl);

  annotate_soft_connections (nl, cl);

  EXPECT_EQ (all_net_names_unique (nl), true);

  //  debug layers produced for nets
  std::map<unsigned int, unsigned int> dump_map;
  dump_map [layer_of (bulk)      ] = ly.insert_layer (db::LayerProperties (200, 0));
  dump_map [layer_of (rnwell)    ] = ly.insert_layer (db::LayerProperties (201, 0));
  dump_map [layer_of (rpsd)      ] = ly.insert_layer (db::LayerProperties (202, 0));
  dump_map [layer_of (rnsd)      ] = ly.insert_layer (db::LayerProperties (203, 0));
  dump_map [layer_of (rpoly)     ] = ly.insert_layer (db::LayerProperties (204, 0));
  dump_map [layer_of (rcontact)  ] = ly.insert_layer (db::LayerProperties (205, 0));
  dump_map [layer_of (rmetal1)   ] = ly.insert_layer (db::LayerProperties (206, 0));
  dump_map [layer_of (rvia1)     ] = ly.insert_layer (db::LayerProperties (207, 0));
  dump_map [layer_of (rmetal2)   ] = ly.insert_layer (db::LayerProperties (208, 0));

  //  write nets to layout
  db::CellMapping cm = dss.cell_mapping_to_original (0, &ly, tc.cell_index ());
  dump_nets_to_layout (nl, cl, ly, dump_map, cm, true);

  //  compare netlist as string
  CHECKPOINT ();
  db::compare_netlist (_this, nl,
    "circuit RINGO ();\n"
    "  subcircuit ND2X1 $1 (NWELL=$4,B=FB,A=ENABLE,VDD=VDD,OUT=$5,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $2 (NWELL=$4,IN=$14,VDD=VDD,OUT=FB,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $3 (NWELL=$4,IN=FB,VDD=$I17,OUT=OUT,VSS=$I27,BULK=BULK);\n"
    "  subcircuit TIE $4 (NWELL=$4,VSS=$I27,VDD=$I17,BULK=BULK);\n"
    "  subcircuit EMPTY $5 ($1=$4,$2=$I27,$3=$I17);\n"
    "  subcircuit TIE $6 (NWELL=$4,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  subcircuit INVX1 $7 (NWELL=$4,IN=$5,VDD=VDD,OUT=$6,VSS=VSS,BULK=BULK);\n"
    "  subcircuit EMPTY $8 ($1=$4,$2=VSS,$3=VDD);\n"
    "  subcircuit INVX1 $9 (NWELL=$4,IN=$6,VDD=VDD,OUT=$7,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $10 (NWELL=$4,IN=$7,VDD=VDD,OUT=$8,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $11 (NWELL=$4,IN=$8,VDD=VDD,OUT=$9,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $12 (NWELL=$4,IN=$9,VDD=VDD,OUT=$10,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $13 (NWELL=$4,IN=$10,VDD=VDD,OUT=$11,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $14 (NWELL=$4,IN=$11,VDD=VDD,OUT=$12,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $15 (NWELL=$4,IN=$12,VDD=VDD,OUT=$15,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $16 (NWELL=$4,IN=$15,VDD=VDD,OUT=$14,VSS=VSS,BULK=BULK);\n"
    "end;\n"
    "circuit ND2X1 (NWELL=NWELL,B=B,A=A,VDD=VDD,OUT=OUT,VSS=VSS,BULK=BULK);\n"
    "  device PMOS $1 (S=$7,G=$4,D=$9,B=NWELL) (L=0.25,W=1.5,AS=0.6375,AD=0.3375,PS=3.85,PD=1.95);\n"
    "  device PMOS $2 (S=$9,G=$2,D=$6,B=NWELL) (L=0.25,W=1.5,AS=0.3375,AD=0.6375,PS=1.95,PD=3.85);\n"
    "  device NMOS $3 (S=$13,G=$4,D=$14,B=BULK) (L=0.25,W=0.95,AS=0.40375,AD=0.21375,PS=2.75,PD=1.4);\n"
    "  device NMOS $4 (S=$14,G=$2,D=$11,B=BULK) (L=0.25,W=0.95,AS=0.21375,AD=0.40375,PS=1.4,PD=2.75);\n"
    "  device SOFT $5 (A=B,C=$2) (A=0,P=0);\n"
    "  device SOFT $6 (A=A,C=$4) (A=0,P=0);\n"
    "  device SOFT $7 (A=OUT,C=$6) (A=0,P=0);\n"
    "  device SOFT $8 (A=OUT,C=$7) (A=0,P=0);\n"
    "  device SOFT $9 (A=VDD,C=$9) (A=0,P=0);\n"
    "  device SOFT $10 (A=OUT,C=$11) (A=0,P=0);\n"
    "  device SOFT $11 (A=VSS,C=$13) (A=0,P=0);\n"
    "end;\n"
    "circuit TIE (NWELL=NWELL,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device SOFT $1 (A=$2,C=NWELL) (A=0,P=0);\n"
    "  device SOFT $2 (A=VDD,C=$2) (A=0,P=0);\n"
    "  device SOFT $3 (A=VSS,C=$3) (A=0,P=0);\n"
    "  device SOFT $4 (A=$3,C=BULK) (A=0,P=0);\n"
    "end;\n"
    "circuit EMPTY ($1=$1,$2=$2,$3=$3);\n"
    "end;\n"
    "circuit INVX1 (NWELL=NWELL,IN=IN,VDD=VDD,OUT=OUT,VSS=VSS,BULK=BULK);\n"
    "  device PMOS $1 (S=$5,G=$2,D=$7,B=NWELL) (L=0.25,W=1.5,AS=0.6375,AD=0.6375,PS=3.85,PD=3.85);\n"
    "  device NMOS $2 (S=$10,G=$2,D=$8,B=BULK) (L=0.25,W=0.95,AS=0.40375,AD=0.40375,PS=2.75,PD=2.75);\n"
    "  device SOFT $3 (A=IN,C=$2) (A=0,P=0);\n"
    "  device SOFT $4 (A=VDD,C=$5) (A=0,P=0);\n"
    "  device SOFT $5 (A=OUT,C=$7) (A=0,P=0);\n"
    "  device SOFT $6 (A=OUT,C=$8) (A=0,P=0);\n"
    "  device SOFT $7 (A=VSS,C=$10) (A=0,P=0);\n"
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
    "circuit RINGO (FB=FB,ENABLE=ENABLE,OUT=OUT,VDD=VDD,VSS=VSS,BULK=BULK);\n"
    "  subcircuit ND2X1 $1 (NWELL=$4,B=FB,A=ENABLE,VDD=VDD,OUT=$5,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $2 (NWELL=$4,IN=$14,VDD=VDD,OUT=FB,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $3 (NWELL=$4,IN=FB,VDD=$I17,OUT=OUT,VSS=$I27,BULK=BULK);\n"
    "  subcircuit TIE $4 (NWELL=$4,VSS=$I27,VDD=$I17,BULK=BULK);\n"
    "  subcircuit TIE $5 (NWELL=$4,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  subcircuit INVX1 $6 (NWELL=$4,IN=$5,VDD=VDD,OUT=$6,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $7 (NWELL=$4,IN=$6,VDD=VDD,OUT=$7,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $8 (NWELL=$4,IN=$7,VDD=VDD,OUT=$8,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $9 (NWELL=$4,IN=$8,VDD=VDD,OUT=$9,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $10 (NWELL=$4,IN=$9,VDD=VDD,OUT=$10,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $11 (NWELL=$4,IN=$10,VDD=VDD,OUT=$11,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $12 (NWELL=$4,IN=$11,VDD=VDD,OUT=$12,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $13 (NWELL=$4,IN=$12,VDD=VDD,OUT=$15,VSS=VSS,BULK=BULK);\n"
    "  subcircuit INVX1 $14 (NWELL=$4,IN=$15,VDD=VDD,OUT=$14,VSS=VSS,BULK=BULK);\n"
    "end;\n"
    "circuit ND2X1 (NWELL=NWELL,B=B,A=A,VDD=VDD,OUT=OUT,VSS=VSS,BULK=BULK);\n"
    "  device PMOS $1 (S=$7,G=$4,D=$9,B=NWELL) (L=0.25,W=1.5,AS=0.6375,AD=0.3375,PS=3.85,PD=1.95);\n"
    "  device PMOS $2 (S=$9,G=$2,D=$6,B=NWELL) (L=0.25,W=1.5,AS=0.3375,AD=0.6375,PS=1.95,PD=3.85);\n"
    "  device NMOS $3 (S=$13,G=$4,D=$14,B=BULK) (L=0.25,W=0.95,AS=0.40375,AD=0.21375,PS=2.75,PD=1.4);\n"
    "  device NMOS $4 (S=$14,G=$2,D=$11,B=BULK) (L=0.25,W=0.95,AS=0.21375,AD=0.40375,PS=1.4,PD=2.75);\n"
    "  device SOFT $5 (A=B,C=$2) (A=0,P=0);\n"
    "  device SOFT $6 (A=A,C=$4) (A=0,P=0);\n"
    "  device SOFT $7 (A=OUT,C=$6) (A=0,P=0);\n"
    "  device SOFT $8 (A=OUT,C=$7) (A=0,P=0);\n"
    "  device SOFT $9 (A=VDD,C=$9) (A=0,P=0);\n"
    "  device SOFT $10 (A=OUT,C=$11) (A=0,P=0);\n"
    "  device SOFT $11 (A=VSS,C=$13) (A=0,P=0);\n"
    "end;\n"
    "circuit TIE (NWELL=NWELL,VSS=VSS,VDD=VDD,BULK=BULK);\n"
    "  device SOFT $1 (A=$2,C=NWELL) (A=0,P=0);\n"
    "  device SOFT $2 (A=VDD,C=$2) (A=0,P=0);\n"
    "  device SOFT $3 (A=VSS,C=$3) (A=0,P=0);\n"
    "  device SOFT $4 (A=$3,C=BULK) (A=0,P=0);\n"
    "end;\n"
    "circuit INVX1 (NWELL=NWELL,IN=IN,VDD=VDD,OUT=OUT,VSS=VSS,BULK=BULK);\n"
    "  device PMOS $1 (S=$5,G=$2,D=$7,B=NWELL) (L=0.25,W=1.5,AS=0.6375,AD=0.6375,PS=3.85,PD=3.85);\n"
    "  device NMOS $2 (S=$10,G=$2,D=$8,B=BULK) (L=0.25,W=0.95,AS=0.40375,AD=0.40375,PS=2.75,PD=2.75);\n"
    "  device SOFT $3 (A=IN,C=$2) (A=0,P=0);\n"
    "  device SOFT $4 (A=VDD,C=$5) (A=0,P=0);\n"
    "  device SOFT $5 (A=OUT,C=$7) (A=0,P=0);\n"
    "  device SOFT $6 (A=OUT,C=$8) (A=0,P=0);\n"
    "  device SOFT $7 (A=VSS,C=$10) (A=0,P=0);\n"
    "end;\n"
  );

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "soft_connections_au.gds");

  db::compare_layouts (_this, ly, au);
}


TEST(100_issue954)
{
  db::Layout ly;
  db::LayerMap lmap;

  unsigned int active     = define_layer (ly, lmap, 1);
  unsigned int poly       = define_layer (ly, lmap, 2);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn (tl::testdata ());
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "device_extract_issue954.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region ractive (db::RecursiveShapeIterator (ly, tc, active), dss);
  db::Region rpoly (db::RecursiveShapeIterator (ly, tc, poly), dss);

  //  derived regions

  db::Region rpgate   = ractive & rpoly;
  db::Region rpsd     = ractive - rpgate;

  //  Global

  db::Region bulk (dss);

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int lgate  = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> Gate
  unsigned int lsd    = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> Source/Drain
  unsigned int lpdiff = ly.insert_layer (db::LayerProperties (12, 0));      // 12/0 -> P Diffusion

  rpgate.insert_into (&ly, tc.cell_index (), lgate);
  rpsd.insert_into (&ly, tc.cell_index (), lsd);
  rpsd.insert_into (&ly, tc.cell_index (), lpdiff);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::NetShape> cl;

  db::NetlistDeviceExtractorMOS4Transistor pmos_ex ("PMOS");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["W"] = &bulk;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  pmos_ex.extract (dss, 0, dl, nl, cl);

  //  perform the net extraction

  db::NetlistExtractor net_ex;

  db::Connectivity conn;

  //  Global nets
  conn.connect_global (bulk, "BULK");

  //  Intra-layer
  conn.connect (rpsd);
  conn.connect (rpoly);

  //  extract the nets

  std::list<std::set<std::string> > jn;

  jn.push_back (std::set<std::string> ());
  jn.back ().insert ("BULK");
  jn.back ().insert ("VSS");

  jn.push_back (std::set<std::string> ());
  jn.back ().insert ("NWELL");
  jn.back ().insert ("VDD");

  net_ex.set_joined_nets ("INV2", jn);

  std::list<tl::GlobPattern> gp;
  gp.push_back (tl::GlobPattern ("NEXT"));
  gp.push_back (tl::GlobPattern ("FB"));
  net_ex.set_joined_net_names (gp);

  net_ex.extract_nets (dss, 0, conn, nl, cl);

  EXPECT_EQ (all_net_names_unique (nl), true);

  //  debug layers produced for nets
  //    200/0 -> Poly
  //    201/0 -> N source/drain
  //    202/0 -> Bulk
  std::map<unsigned int, unsigned int> dump_map;
  dump_map [layer_of (bulk)      ] = ly.insert_layer (db::LayerProperties (202, 0));
  dump_map [layer_of (rpsd)      ] = ly.insert_layer (db::LayerProperties (201, 0));
  dump_map [layer_of (rpoly)     ] = ly.insert_layer (db::LayerProperties (200, 0));

  //  write nets to layout
  db::CellMapping cm = dss.cell_mapping_to_original (0, &ly, tc.cell_index ());
  dump_nets_to_layout (nl, cl, ly, dump_map, cm, true);

  //  compare the collected test data

  std::string au = tl::testdata ();
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_issue954_au.gds");

  db::compare_layouts (_this, ly, au);
}


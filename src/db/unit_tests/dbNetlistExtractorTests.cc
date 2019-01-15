
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

      if (device_cells_seen.find (d->cell_index ()) != device_cells_seen.end ()) {
        continue;
      }

      db::Cell &device_cell = ly.cell (cmap.cell_mapping (d->cell_index ()));
      device_cells_seen.insert (d->cell_index ());

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

        const db::local_cluster<db::PolygonRef> &dc = clusters.clusters_per_cell (d->cell_index ()).cluster_by_id (d->cluster_id_for_terminal (t->id ()));

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
  pmos_ex.extract (dss, dl, nl, cl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  nmos_ex.extract (dss, dl, nl, cl);

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

  net_ex.extract_nets (dss, conn, nl, cl);

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
  EXPECT_EQ (nl.to_string (),
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

  // doesn't do anything here, but we test that this does not destroy anything:
  nl.combine_devices ();

  //  make pins for named nets of top-level circuits - this way they are not purged
  nl.make_top_level_pins ();
  nl.purge ();

  //  compare netlist as string
  EXPECT_EQ (nl.to_string (),
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
  au = tl::combine_path (au, "device_extract_au1.gds");

  db::compare_layouts (_this, ly, au);
}


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


#include "dbNetlistDeviceExtractor.h"
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
#include "dbNetlistProperty.h"
#include "dbCellMapping.h"

#include "tlUnitTest.h"
#include "tlString.h"
#include "tlFileUtils.h"

#include <memory>
#include <limits>

static unsigned int layer_of (const db::Region &region)
{
  return db::DeepLayer (region).layer ();
}

static std::string net_name (const db::Net *net)
{
  return net ? net->expanded_name () : "(null)";
}

static std::string device_name (const db::Device &device)
{
  if (device.name ().empty ()) {
    return "$" + tl::to_string (device.id ());
  } else {
    return device.name ();
  }
}

static std::string subcircuit_name (const db::SubCircuit &subcircuit)
{
  if (subcircuit.name ().empty ()) {
    return "$" + tl::to_string (subcircuit.id ());
  } else {
    return subcircuit.name ();
  }
}

static std::string pin_name (const db::Pin &pin)
{
  if (pin.name ().empty ()) {
    //  the pin ID is zero-based and essentially the index, so we add 1 to make it compliant with the other IDs
    return "$" + tl::to_string (pin.id () + 1);
  } else {
    return pin.name ();
  }
}


class MOSFETExtractor
  : public db::NetlistDeviceExtractor
{
public:
  MOSFETExtractor (const std::string &name, db::Layout *debug_out)
    : db::NetlistDeviceExtractor (name), mp_debug_out (debug_out), m_ldiff (0), m_lgate (0)
  {
    if (mp_debug_out) {
      m_ldiff = mp_debug_out->insert_layer (db::LayerProperties (100, 0));
      m_lgate = mp_debug_out->insert_layer (db::LayerProperties (101, 0));
    }
  }

  virtual void setup ()
  {
    define_layer ("SD", "Source/drain diffusion");
    define_layer ("G", "Gate");
    define_layer ("P", "Poly");

    register_device_class (new db::DeviceClassMOS3Transistor ());
  }

  virtual db::Connectivity get_connectivity (const db::Layout & /*layout*/, const std::vector<unsigned int> &layers) const
  {
    tl_assert (layers.size () == 3);

    unsigned int diff = layers [0];
    unsigned int gate = layers [1];
    //  not used for device recognition: poly (2), but used for producing the gate terminals

    //  The layer definition is diff, gate
    db::Connectivity conn;
    //  collect all connected diffusion shapes
    conn.connect (diff, diff);
    //  collect all connected gate shapes
    conn.connect (gate, gate);
    //  connect gate with diff to detect gate/diffusion boundary
    conn.connect (diff, gate);
    return conn;
  }

  virtual void extract_devices (const std::vector<db::Region> &layer_geometry)
  {
    const db::Region &rdiff = layer_geometry [0];
    const db::Region &rgates = layer_geometry [1];

    for (db::Region::const_iterator p = rgates.begin_merged (); !p.at_end (); ++p) {

      db::Region rgate (*p);
      db::Region rdiff2gate = rdiff.selected_interacting (rgate);

      if (rdiff2gate.empty ()) {
        error (tl::to_string (tr ("Gate shape touches no diffusion - ignored")), *p);
      } else {

        unsigned int terminal_geometry_index = 0;
        unsigned int gate_geometry_index = 2;

        if (rdiff2gate.size () != 2) {
          error (tl::sprintf (tl::to_string (tr ("Expected two polygons on diff interacting one gate shape (found %d) - gate shape ignored")), int (rdiff2gate.size ())), *p);
          continue;
        }

        db::Edges edges (rgate.edges () & rdiff2gate.edges ());
        if (edges.size () != 2) {
          error (tl::sprintf (tl::to_string (tr ("Expected two edges interacting gate/diff (found %d) - width and length may be incorrect")), int (edges.size ())), *p);
          continue;
        }

        if (! p->is_box ()) {
          error (tl::to_string (tr ("Gate shape is not a box - width and length may be incorrect")), *p);
        }

        db::Device *device = create_device ();

        device->set_parameter_value ("W", dbu () * edges.length () * 0.5);
        device->set_parameter_value ("L", dbu () * (p->perimeter () - edges.length ()) * 0.5);

        int diff_index = 0;
        for (db::Region::const_iterator d = rdiff2gate.begin (); !d.at_end () && diff_index < 2; ++d, ++diff_index) {

          //  count the number of gate shapes attached to this shape and distribute the area of the
          //  diffusion region to the number of gates
          int n = rgates.selected_interacting (db::Region (*d)).size ();
          tl_assert (n > 0);

          device->set_parameter_value (diff_index == 0 ? "AS" : "AD", dbu () * dbu () * d->area () / double (n));

          define_terminal (device, device->device_class ()->terminal_id_for_name (diff_index == 0 ? "S" : "D"), terminal_geometry_index, *d);

        }

        define_terminal (device, device->device_class ()->terminal_id_for_name ("G"), gate_geometry_index, *p);

        //  output the device for debugging
        device_out (device, rdiff2gate, rgate);

      }

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

static unsigned int define_layer (db::Layout &ly, db::LayerMap &lmap, int gds_layer, int gds_datatype = 0)
{
  unsigned int lid = ly.insert_layer (db::LayerProperties (gds_layer, gds_datatype));
  lmap.map (ly.get_properties (lid), lid);
  return lid;
}

//  @@@ TODO: move this somewhere else

static void dump_nets (const db::Netlist &nl, const db::hier_clusters<db::PolygonRef> &clusters, db::Layout &ly, const std::map<unsigned int, unsigned int> &lmap, const db::CellMapping &cmap)
{
  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {

    db::Cell &cell = ly.cell (cmap.cell_mapping (c->cell_index ()));

    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {

      const db::local_cluster<db::PolygonRef> &lc = clusters.clusters_per_cell (c->cell_index ()).cluster_by_id (n->cluster_id ());

      bool any_shapes = false;
      for (std::map<unsigned int, unsigned int>::const_iterator m = lmap.begin (); m != lmap.end () && !any_shapes; ++m) {
        any_shapes = ! lc.begin (m->first).at_end ();
      }

      if (any_shapes) {

        std::string nn = "NET_" + c->name () + "_" + net_name (n.operator-> ());
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

  }
}

static std::string netlist2string (const db::Netlist &nl)
{
  std::string res;
  for (db::Netlist::const_circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {

    std::string ps;
    for (db::Circuit::const_pin_iterator p = c->begin_pins (); p != c->end_pins (); ++p) {
      if (! ps.empty ()) {
        ps += ",";
      }
      ps += pin_name (*p) + "=" + net_name (c->net_for_pin (p->id ()));
    }

    res += std::string ("Circuit ") + c->name () + " (" + ps + "):\n";

#if 0  //  for debugging
    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
      res += "  N" + net_name (n.operator-> ()) + " pins=" + tl::to_string (n->pin_count ()) + " terminals=" + tl::to_string (n->terminal_count ()) + "\n";
    }
#endif

    for (db::Circuit::const_device_iterator d = c->begin_devices (); d != c->end_devices (); ++d) {
      std::string ts;
      const std::vector<db::DeviceTerminalDefinition> &td = d->device_class ()->terminal_definitions ();
      for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
        if (t != td.begin ()) {
          ts += ",";
        }
        ts += t->name () + "=" + net_name (d->net_for_terminal (t->id ()));
      }
      std::string ps;
      const std::vector<db::DeviceParameterDefinition> &pd = d->device_class ()->parameter_definitions ();
      for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
        if (p != pd.begin ()) {
          ps += ",";
        }
        ps += p->name () + "=" + tl::to_string (d->parameter_value (p->id ()));
      }
      res += std::string ("  D") + d->device_class ()->name () + " " + device_name (*d) + " (" + ts + ") [" + ps + "]\n";
    }

    for (db::Circuit::const_subcircuit_iterator sc = c->begin_subcircuits (); sc != c->end_subcircuits (); ++sc) {
      std::string ps;
      const db::SubCircuit &subcircuit = *sc;
      const db::Circuit *circuit = sc->circuit_ref ();
      for (db::Circuit::const_pin_iterator p = circuit->begin_pins (); p != circuit->end_pins (); ++p) {
        if (p != circuit->begin_pins ()) {
          ps += ",";
        }
        const db::Pin &pin = *p;
        ps += pin_name (pin) + "=" + net_name (subcircuit.net_for_pin (pin.id ()));
      }
      res += std::string ("  X") + circuit->name () + " " + subcircuit_name (*sc) + " (" + ps + ")\n";
    }

  }

  return res;
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

  //  NOTE: the device extractor will add more debug layers for the transistors:
  //    20/0 -> Diffusion
  //    21/0 -> Gate
  MOSFETExtractor pmos_ex ("PMOS", &ly);
  MOSFETExtractor nmos_ex ("NMOS", &ly);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &rpsd;
  dl["G"] = &rpgate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  pmos_ex.extract (dss, dl, &nl);

  dl["SD"] = &rnsd;
  dl["G"] = &rngate;
  dl["P"] = &rpoly;  //  not needed for extraction but to return terminal shapes
  nmos_ex.extract (dss, dl, &nl);

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

  net_ex.extract_nets (dss, conn, &nl);

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
  dump_nets (nl, net_ex.clusters (), ly, dump_map, cm);

  //  compare netlist as string
  EXPECT_EQ (netlist2string (nl),
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
  EXPECT_EQ (netlist2string (nl),
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

#if 0

// --------------------------------------------------------------------------------------
//  An attempt to simplify things.

/*
TODO:
- netlist query functions such as net_by_name, device_by_name, circuit_by_name
- terminal geometry (Polygon) for device, combined device geometry (all terminals)
- netlist manipulation methods (i.e. flatten certain cells, purging etc.)
*/

#include "tlGlobPattern.h"
#include "dbHierNetworkProcessor.h"

namespace db
{

class DB_PUBLIC LayoutToNetlist
{
public:
  //  the iterator provides the hierarchical selection (enabling/disabling cells etc.)
  LayoutToNetlist (const db::RecursiveShapeIterator &iter);

  //  --- Step 0: configuration

  void set_threads (unsigned int n);
  void set_area_ratio (double ar);
  void set_max_vertex_count (size_t n);

  //  --- Step 1: preparation

  //  returns new'd regions
  db::Region *make_layer (unsigned int layer_index);
  db::Region *make_text_layer (unsigned int layer_index);
  db::Region *make_polygon_layer (unsigned int layer_index);

  //  --- Step 2: device extraction

  //  after this, the device extractor will have errors if some occured.
  void extract_devices (db::NetlistDeviceExtractor &extractor, const std::map<std::string, db::Region *> &layers);

  //  --- Step 3: net extraction

  //  define connectivity for the netlist extraction
  void connect (const db::Region &l);
  void connect (const db::Region &a, const db::Region &b);

  //  runs the netlist extraction
  void extract_netlist ();

  //  --- Step 4: retrieval

  //  gets the internal layout and cell (0 if not available)
  const db::Layout *internal_layout () const;
  const db::Cell *internal_top_cell () const;

  //  gets the internal layer index of the given region
  unsigned int layer_of (const db::Region &region) const;

  //  creates a cell mapping for copying the internal hierarchy to the given layout
  //  CAUTION: may create new cells in "layout".
  db::CellMapping cell_mapping_into (db::Layout &layout, db::Cell &cell);

  //  creates a cell mapping for copying the internal hierarchy to the given layout
  //  This version will not create new cells in the target layout.
  db::CellMapping const_cell_mapping_into (const db::Layout &layout, const db::Cell &cell);

  //  gets the netlist extracted (0 if no extraction happened yet)
  db::Netlist *netlist () const;

  //  gets the hierarchical clusters of the nets (CAUTION: the layer indexes therein are
  //  internal layer indexes), same for cell indexes.
  //  -> NOT GSI
  const db::hier_clusters<db::PolygonRef> &net_clusters () const;

  //  copies the shapes of the given net from a given layer
  //  (recursive true: include nets from subcircuits)
  db::Region shapes_of_net (const db::Net &net, const db::Region &of_layer, bool recursive);

  //  finds the net by probing a specific location on the given layer looking through the
  //  hierarchy. Returns 0 if no net was found.
  db::Net *probe_net (const db::Region &of_region, const db::DPoint &point);
  db::Net *probe_net (const db::Region &of_region, const db::Point &point);
};

}

#endif

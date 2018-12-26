
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
#include "dbNetlistDeviceClasses.h"
#include "dbHierNetworkProcessor.h"
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

class MOSFETExtractor
  : public db::NetlistDeviceExtractor
{
public:
  MOSFETExtractor (db::Netlist &nl, db::Layout *debug_out)
    : db::NetlistDeviceExtractor (), mp_debug_out (debug_out), m_ldiff (0), m_lgate (0)
  {
    initialize (&nl);
    if (mp_debug_out) {
      m_ldiff = mp_debug_out->insert_layer (db::LayerProperties (100, 0));
      m_lgate = mp_debug_out->insert_layer (db::LayerProperties (101, 0));
    }
  }

  virtual void create_device_classes ()
  {
    db::DeviceClassMOS3Transistor *pmos_class = new db::DeviceClassMOS3Transistor ();
    pmos_class->set_name ("PMOS");
    register_device_class (pmos_class);

    db::DeviceClassMOS3Transistor *nmos_class = new db::DeviceClassMOS3Transistor ();
    nmos_class->set_name ("NMOS");
    register_device_class (nmos_class);
  }

  virtual db::Connectivity get_connectivity (const db::Layout & /*layout*/, const std::vector<unsigned int> &layers) const
  {
    tl_assert (layers.size () == 4);

    unsigned int lpdiff = layers [0];
    unsigned int lndiff = layers [1];
    unsigned int gate = layers [2];
    //  not used for device recognition: poly (3), but used for producing the gate terminals

    //  The layer definition is pdiff, ndiff, gate
    db::Connectivity conn;
    //  collect all connected pdiff
    conn.connect (lpdiff, lpdiff);
    //  collect all connected ndiff
    conn.connect (lndiff, lndiff);
    //  collect all connected gate shapes
    conn.connect (gate, gate);
    //  connect gate with pdiff
    conn.connect (lpdiff, gate);
    //  connect gate with ndiff
    conn.connect (lndiff, gate);
    return conn;
  }

  virtual void extract_devices (const std::vector<db::Region> &layer_geometry)
  {
    const db::Region &rpdiff = layer_geometry [0];
    const db::Region &rndiff = layer_geometry [1];
    const db::Region &rgates = layer_geometry [2];

    for (db::Region::const_iterator p = rgates.begin_merged (); !p.at_end (); ++p) {

      db::Region rgate (*p);
      db::Region rpdiff_on_gate = rpdiff.selected_interacting (rgate);
      db::Region rndiff_on_gate = rndiff.selected_interacting (rgate);

      if (! rpdiff_on_gate.empty () && ! rndiff_on_gate.empty ()) {
        error (tl::to_string (tr ("Gate shape touches both ndiff and pdiff - ignored")), *p);
      } else if (rpdiff_on_gate.empty () && rndiff_on_gate.empty ()) {
        error (tl::to_string (tr ("Gate shape touches neither ndiff and pdiff - ignored")), *p);
      } else {

        bool is_pmos = ! rpdiff_on_gate.empty ();

        db::Region &diff = (is_pmos ? rpdiff_on_gate : rndiff_on_gate);
        unsigned int terminal_geometry_index = (is_pmos ? 0 : 1);
        unsigned int gate_geometry_index = 3;
        unsigned int device_class_index = (is_pmos ? 0 /*PMOS*/ : 1 /*NMOS*/);

        if (diff.size () != 2) {
          error (tl::sprintf (tl::to_string (tr ("Expected two polygons on diff interacting one gate shape (found %d) - gate shape ignored")), int (diff.size ())), *p);
          continue;
        }

        db::Edges edges (rgate.edges () & diff.edges ());
        if (edges.size () != 2) {
          error (tl::sprintf (tl::to_string (tr ("Expected two edges interacting gate/diff (found %d) - width and length may be incorrect")), int (edges.size ())), *p);
          continue;
        }

        if (! p->is_box ()) {
          error (tl::to_string (tr ("Gate shape is not a box - width and length may be incorrect")), *p);
        }

        db::Device *device = create_device (device_class_index);

        device->set_parameter_value ("W", dbu () * edges.length () * 0.5);
        device->set_parameter_value ("L", dbu () * (p->perimeter () - edges.length ()) * 0.5);

        int diff_index = 0;
        for (db::Region::const_iterator d = diff.begin (); !d.at_end () && diff_index < 2; ++d, ++diff_index) {

          //  count the number of gate shapes attached to this shape and distribute the area of the
          //  diffusion region to the number of gates
          int n = rgates.selected_interacting (db::Region (*d)).size ();
          tl_assert (n > 0);

          device->set_parameter_value (diff_index == 0 ? "AS" : "AD", dbu () * dbu () * d->area () / double (n));

          define_terminal (device, device->device_class ()->terminal_id_for_name (diff_index == 0 ? "S" : "D"), terminal_geometry_index, *d);

        }

        define_terminal (device, device->device_class ()->terminal_id_for_name ("G"), gate_geometry_index, *p);

        //  output the device for debugging
        device_out (device, diff, rgate);

      }

    }
  }

  void error (const std::string &msg)
  {
    // @@@ TODO: move this to device extractor
    tl::error << tr ("Error in cell '") << cell_name () << "': " << msg;
  }

  void error (const std::string &msg, const db::Polygon &poly)
  {
    // @@@ TODO: move this to device extractor
    tl::error << tr ("Error in cell '") << cell_name () << "': " << msg << " (" << poly.to_string () << ")";
  }

  void error (const std::string &msg, const db::Region &region)
  {
    // @@@ TODO: move this to device extractor
    tl::error << tr ("Error in cell '") << cell_name () << "': " << msg << " (" << region.to_string () << ")";
  }

  std::string cell_name () const
  {
    // @@@ TODO: move this to device extractor
    return layout ()->cell_name (cell_index ());
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

    db::cell_index_type dci = mp_debug_out->add_cell ((device->device_class ()->name () + "_" + device->name ()).c_str ());
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

//  @@@ TODO: move somewhere else
static unsigned int layer_of (const db::Region &region)
{
  //  TODO: this is clumsy ...
  db::DeepRegion *dr = dynamic_cast<db::DeepRegion *> (region.delegate ());
  tl_assert (dr != 0);

  return dr->deep_layer ().layer ();
}

//  @@@ TODO: move somewhere else
class NetExtractor
{
public:
  typedef db::hier_clusters<db::PolygonRef> hier_clusters_type;
  typedef db::connected_clusters<db::PolygonRef> connected_clusters_type;
  typedef db::local_cluster<db::PolygonRef> local_cluster_type;

  NetExtractor ()
  {
    //  .. nothing yet ..
  }

  void extract_nets (const db::DeepShapeStore &dss, const db::Connectivity &conn, db::Netlist *nl)
  {
    tl::Variant terminal_property_name (0); // @@@ take somewhere else

    //  only works for singular-layout stores currently. This rules out layers from different sources
    //  and clipping.
    tl_assert (dss.layouts () == 1);
    const db::Layout *layout = dss.const_layout (0);

    tl_assert (layout->cells () != 0);
    const db::Cell &cell = layout->cell (*layout->begin_top_down ());

    //  gets the text annotation property ID
    std::pair<bool, db::property_names_id_type> text_annot_name_id (false, 0);
    if (! dss.text_property_name ().is_nil ()) {
      text_annot_name_id = layout->properties_repository ().get_id_of_name (dss.text_property_name ());
    }

    //  gets the device terminal annotation property ID
    std::pair<bool, db::property_names_id_type> terminal_annot_name_id (false, 0);
    if (! terminal_property_name.is_nil ()) {
      terminal_annot_name_id = layout->properties_repository ().get_id_of_name (terminal_property_name);
    }

    m_net_clusters.build (*layout, cell, db::ShapeIterator::Polygons, conn);

    std::map<db::cell_index_type, db::Circuit *> circuits;
    //  some circuits may be there because of device extraction
    for (db::Netlist::circuit_iterator c = nl->begin_circuits (); c != nl->end_circuits (); ++c) {
      //  @@@ TODO: what if the circuits don't have a cell index?
      circuits.insert (std::make_pair (c->cell_index (), c.operator-> ()));
    }

    std::map<db::cell_index_type, std::map<size_t, size_t> > pins_per_cluster;

    for (db::Layout::bottom_up_const_iterator cid = layout->begin_bottom_up (); cid != layout->end_bottom_up (); ++cid) {

      const connected_clusters_type &clusters = m_net_clusters.clusters_per_cell (*cid);
      if (clusters.empty ()) {
        continue;
      }

      //  a cell makes a new circuit (or uses an existing one)

      db::Circuit *circuit = 0;

      std::map<db::cell_index_type, db::Circuit *>::const_iterator k = circuits.find (*cid);
      if (k == circuits.end ()) {
        circuit = new db::Circuit ();
        nl->add_circuit (circuit);
        circuit->set_name (layout->cell_name (*cid));
        circuit->set_cell_index (*cid);
        circuits.insert (std::make_pair (*cid, circuit));
      } else {
        circuit = k->second;
      }

      std::map<size_t, size_t> &c2p = pins_per_cluster [*cid];

      std::map<db::InstElement, db::SubCircuit *> subcircuits;

      for (connected_clusters_type::all_iterator c = clusters.begin_all (); ! c.at_end (); ++c) {

        db::Net *net = new db::Net ();
        net->set_cluster_id (*c);
        circuit->add_net (net);

        if (! clusters.is_root (*c)) {

          //  a non-root cluster makes a pin
          db::Pin pin (net->name ());
          size_t pin_id = circuit->add_pin (pin).id ();
          net->add_pin (db::NetPinRef (pin_id));
          c2p.insert (std::make_pair (*c, pin_id));
          circuit->connect_pin (pin_id, net);

        }

        const connected_clusters_type::connections_type &connections = clusters.connections_for_cluster (*c);
        for (connected_clusters_type::connections_type::const_iterator i = connections.begin (); i != connections.end (); ++i) {

          db::SubCircuit *subcircuit = 0;
          db::cell_index_type ccid = i->inst ().inst_ptr.cell_index ();

          std::map<db::InstElement, db::SubCircuit *>::const_iterator j = subcircuits.find (i->inst ());
          if (j == subcircuits.end ()) {

            //  make subcircuit if required

            std::map<db::cell_index_type, db::Circuit *>::const_iterator k = circuits.find (ccid);
            tl_assert (k != circuits.end ());  //  because we walk bottom-up

            // @@@ name?
            subcircuit = new db::SubCircuit (k->second);
            db::CplxTrans dbu_trans (layout->dbu ());
            subcircuit->set_trans (dbu_trans * i->inst ().complex_trans () * dbu_trans.inverted ());
            circuit->add_sub_circuit (subcircuit);
            subcircuits.insert (std::make_pair (i->inst (), subcircuit));

          } else {
            subcircuit = j->second;
          }

          //  create the pin connection to the subcircuit
          std::map<db::cell_index_type, std::map<size_t, size_t> >::const_iterator icc2p = pins_per_cluster.find (ccid);
          tl_assert (icc2p != pins_per_cluster.end ());
          std::map<size_t, size_t>::const_iterator ip = icc2p->second.find (i->id ());
          tl_assert (ip != icc2p->second.end ());
          subcircuit->connect_pin (ip->second, net);

        }

        //  collect the properties - we know that the cluster attributes are property ID's because the
        //  cluster processor converts shape property IDs to attributes
        const local_cluster_type &lc = clusters.cluster_by_id (*c);
        for (local_cluster_type::attr_iterator a = lc.begin_attr (); a != lc.end_attr (); ++a) {

          //  @@@ TODO: needs refactoring!!!
          //  -> use two distinct and reserved property name ID's for names (=string) and device terminal refs (=single number) instead
          //  of the scary DeviceTerminalProperty (pointer!!!)
          const db::PropertiesRepository::properties_set &ps = layout->properties_repository ().properties (*a);
          for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end (); ++j) {

            if (terminal_annot_name_id.first && j->first == terminal_annot_name_id.second) {

              if (j->second.is_user<db::NetlistProperty> ()) {
                const db::NetlistProperty *np = &j->second.to_user<db::NetlistProperty> ();
                const db::DeviceTerminalProperty *tp = dynamic_cast<const db::DeviceTerminalProperty *> (np);
                const db::NetNameProperty *nnp = dynamic_cast<const db::NetNameProperty *> (np);
                if (tp) {
                  const_cast<db::Device *> (tp->terminal_ref ().device ())->connect_terminal (tp->terminal_ref ().terminal_id (), net);
                } else if (nnp) {
                  net->set_name (nnp->name ());
                }
              }

            } else if (text_annot_name_id.first && j->first == text_annot_name_id.second) {

              std::string n = j->second.to_string ();
              if (! n.empty ()) {
                if (! net->name ().empty ()) {
                  n = net->name () + "," + n;
                }
                net->set_name (n);
              }

            }

          }

        }

      }

    }
  }

  const hier_clusters_type clusters () const
  {
    return m_net_clusters;
  }

private:
  hier_clusters_type m_net_clusters;
};

//  @@@ TODO: move this somewhere else
static std::string net_name (const db::Net *net)
{
  if (! net) {
    return "(null)";
  } else if (net->name ().empty ()) {
    if (net->cluster_id () > std::numeric_limits<size_t>::max () / 2) {
      return "$I" + tl::to_string ((std::numeric_limits<size_t>::max () - net->cluster_id ()) + 1);
    } else {
      return "$" + tl::to_string (net->cluster_id ());
    }
  } else {
    return net->name ();
  }
}

//  @@@ TODO: refactor. This is inefficient. Give an ID automatically.
static std::string device_name (const db::Device &device, const db::Circuit &circuit)
{
  if (device.name ().empty ()) {
    int id = 1;
    for (db::Circuit::const_device_iterator d = circuit.begin_devices (); d != circuit.end_devices () && d.operator-> () != &device; ++d, ++id)
      ;
    return "$" + tl::to_string (id);
  } else {
    return device.name ();
  }
}

//  @@@ TODO: refactor. This is inefficient. Give an ID automatically.
static std::string subcircuit_name (const db::SubCircuit &subcircuit, const db::Circuit &circuit)
{
  if (subcircuit.name ().empty ()) {
    int id = 1;
    for (db::Circuit::const_sub_circuit_iterator d = circuit.begin_sub_circuits (); d != circuit.end_sub_circuits () && d.operator-> () != &subcircuit; ++d, ++id)
      ;
    return "$" + tl::to_string (id);
  } else {
    return subcircuit.name ();
  }
}

//  @@@ TODO: refactor. This is inefficient. Give an ID automatically.
static std::string pin_name (const db::Pin &pin, const db::Circuit &circuit)
{
  if (pin.name ().empty ()) {
    int id = 1;
    for (db::Circuit::const_pin_iterator p = circuit.begin_pins (); p != circuit.end_pins () && p.operator-> () != &pin; ++p, ++id)
      ;
    return "$" + tl::to_string (id);
  } else {
    return pin.name ();
  }
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
      ps += pin_name (*p, *c) + "=" + net_name (c->net_for_pin (p->id ()));
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
      res += std::string ("  D") + d->device_class ()->name () + " " + device_name (*d, *c) + " (" + ts + ")\n";
    }

    for (db::Circuit::const_sub_circuit_iterator sc = c->begin_sub_circuits (); sc != c->end_sub_circuits (); ++sc) {
      std::string ps;
      const db::SubCircuit &subcircuit = *sc;
      for (db::Circuit::const_pin_iterator p = sc->circuit ()->begin_pins (); p != sc->circuit ()->end_pins (); ++p) {
        if (p != sc->circuit ()->begin_pins ()) {
          ps += ",";
        }
        const db::Pin &pin = *p;
        ps += pin_name (pin, *subcircuit.circuit ()) + "=" + net_name (subcircuit.net_for_pin (pin.id ()));
      }
      res += std::string ("  X") + sc->circuit ()->name () + " " + subcircuit_name (*sc, *c) + " (" + ps + ")\n";
    }

  }

  return res;
}

TEST(1_DeviceNetExtraction)
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
  db::Region rgate  = ractive & rpoly;
  db::Region rsd    = ractive - rgate;
  db::Region rpdiff = rsd & rnwell;
  db::Region rndiff = rsd - rnwell;

  //  return the computed layers into the original layout and write it for debugging purposes

  unsigned int lgate  = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> Gate
  unsigned int lsd    = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> Source/Drain
  unsigned int lpdiff = ly.insert_layer (db::LayerProperties (12, 0));      // 12/0 -> P Diffusion
  unsigned int lndiff = ly.insert_layer (db::LayerProperties (13, 0));      // 13/0 -> N Diffusion

  rgate.insert_into (&ly, tc.cell_index (), lgate);
  rsd.insert_into (&ly, tc.cell_index (), lsd);
  rpdiff.insert_into (&ly, tc.cell_index (), lpdiff);
  rndiff.insert_into (&ly, tc.cell_index (), lndiff);

  //  perform the extraction

  db::Netlist nl;

  //  NOTE: the device extractor will add more debug layers for the transistors:
  //    20/0 -> Diffusion
  //    21/0 -> Gate
  MOSFETExtractor ex (nl, &ly);

  std::vector<db::Region *> region_ptrs;
  region_ptrs.push_back (&rpdiff);
  region_ptrs.push_back (&rndiff);
  region_ptrs.push_back (&rgate);
  region_ptrs.push_back (&rpoly);

  ex.extract (region_ptrs);

  //  perform the net extraction

  NetExtractor net_ex;

  db::Connectivity conn;
  //  Intra-layer
  conn.connect (layer_of (rpdiff));
  conn.connect (layer_of (rndiff));
  conn.connect (layer_of (rpoly));
  conn.connect (layer_of (rdiff_cont));
  conn.connect (layer_of (rpoly_cont));
  conn.connect (layer_of (rmetal1));
  conn.connect (layer_of (rvia1));
  conn.connect (layer_of (rmetal2));
  //  Inter-layer
  conn.connect (layer_of (rpdiff),     layer_of (rdiff_cont));
  conn.connect (layer_of (rndiff),     layer_of (rdiff_cont));
  conn.connect (layer_of (rpoly),      layer_of (rpoly_cont));
  conn.connect (layer_of (rpoly_cont), layer_of (rmetal1));
  conn.connect (layer_of (rdiff_cont), layer_of (rmetal1));
  conn.connect (layer_of (rmetal1),    layer_of (rvia1));
  conn.connect (layer_of (rvia1),      layer_of (rmetal2));
  conn.connect (layer_of (rpoly),      layer_of (rpoly_lbl));     //  attaches labels
  conn.connect (layer_of (rmetal1),    layer_of (rmetal1_lbl));   //  attaches labels
  conn.connect (layer_of (rmetal2),    layer_of (rmetal2_lbl));   //  attaches labels

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
  std::map<unsigned int, unsigned int> dump_map;
  dump_map [layer_of (rpdiff)    ] = ly.insert_layer (db::LayerProperties (210, 0));
  dump_map [layer_of (rndiff)    ] = ly.insert_layer (db::LayerProperties (211, 0));
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
    "  XINV2 $1 ($1=$I8,$2=FB,$3=OSC,$4=VSS,$5=VDD)\n"
    "  XINV2 $2 ($1=FB,$2=$I38,$3=$I19,$4=VSS,$5=VDD)\n"
    "  XINV2 $3 ($1=$I19,$2=$I39,$3=$I1,$4=VSS,$5=VDD)\n"
    "  XINV2 $4 ($1=$I1,$2=$I40,$3=$I2,$4=VSS,$5=VDD)\n"
    "  XINV2 $5 ($1=$I2,$2=$I41,$3=$I3,$4=VSS,$5=VDD)\n"
    "  XINV2 $6 ($1=$I3,$2=$I42,$3=$I4,$4=VSS,$5=VDD)\n"
    "  XINV2 $7 ($1=$I4,$2=$I43,$3=$I5,$4=VSS,$5=VDD)\n"
    "  XINV2 $8 ($1=$I5,$2=$I44,$3=$I6,$4=VSS,$5=VDD)\n"
    "  XINV2 $9 ($1=$I6,$2=$I45,$3=$I7,$4=VSS,$5=VDD)\n"
    "  XINV2 $10 ($1=$I7,$2=$I46,$3=$I8,$4=VSS,$5=VDD)\n"
    "Circuit INV2 ($1=IN,$2=$2,$3=OUT,$4=$4,$5=$5):\n"
    "  DPMOS 1 (S=$2,G=IN,D=$5)\n"
    "  DPMOS 2 (S=$5,G=$2,D=OUT)\n"
    "  DNMOS 3 (S=$2,G=IN,D=$4)\n"
    "  DNMOS 4 (S=$4,G=$2,D=OUT)\n"
    "  XTRANS $1 ($1=$2,$2=$4,$3=IN)\n"
    "  XTRANS $2 ($1=$2,$2=$5,$3=IN)\n"
    "  XTRANS $3 ($1=$5,$2=OUT,$3=$2)\n"
    "  XTRANS $4 ($1=$4,$2=OUT,$3=$2)\n"
    "Circuit TRANS ($1=$1,$2=$2,$3=$3):\n"
  );

  //  compare the collected test data

  std::string au = tl::testsrc ();
  au = tl::combine_path (au, "testdata");
  au = tl::combine_path (au, "algo");
  au = tl::combine_path (au, "device_extract_au1.gds");

  db::compare_layouts (_this, ly, au);
}

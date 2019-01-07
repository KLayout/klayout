
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

#include "dbNetlistExtractor.h"
#include "dbDeepShapeStore.h"
#include "dbNetlistDeviceExtractor.h"
#include "dbNetlistProperty.h"

namespace db
{

NetlistExtractor::NetlistExtractor ()
{
  //  .. nothing yet ..
}

void
NetlistExtractor::extract_nets (const db::DeepShapeStore &dss, const db::Connectivity &conn, db::Netlist *nl)
{
  const db::Layout &layout = dss.const_layout ();
  const db::Cell &cell = dss.const_initial_cell ();

  //  gets the text annotation property ID -
  //  this is how the texts are passed for annotating the net names
  std::pair<bool, db::property_names_id_type> text_annot_name_id (false, 0);
  if (! dss.text_property_name ().is_nil ()) {
    text_annot_name_id = layout.properties_repository ().get_id_of_name (dss.text_property_name ());
  }

  //  gets the device terminal annotation property ID -
  //  this is how the device extractor conveys terminal shape annotations.
  std::pair<bool, db::property_names_id_type> terminal_annot_name_id;
  terminal_annot_name_id = layout.properties_repository ().get_id_of_name (db::NetlistDeviceExtractor::terminal_property_name ());

  //  the big part: actually extract the nets

  m_net_clusters.build (layout, cell, db::ShapeIterator::Polygons, conn);

  //  reverse lookup for Circuit vs. cell index
  std::map<db::cell_index_type, db::Circuit *> circuits;

  //  some circuits may be there because of device extraction
  for (db::Netlist::circuit_iterator c = nl->begin_circuits (); c != nl->end_circuits (); ++c) {
    tl_assert (layout.is_valid_cell_index (c->cell_index ()));
    circuits.insert (std::make_pair (c->cell_index (), c.operator-> ()));
  }

  std::map<db::cell_index_type, std::map<size_t, size_t> > pins_per_cluster_per_cell;
  for (db::Layout::bottom_up_const_iterator cid = layout.begin_bottom_up (); cid != layout.end_bottom_up (); ++cid) {

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
      circuit->set_name (layout.cell_name (*cid));
      circuit->set_cell_index (*cid);
      circuits.insert (std::make_pair (*cid, circuit));
    } else {
      circuit = k->second;
    }

    std::map<size_t, size_t> &c2p = pins_per_cluster_per_cell [*cid];

    std::map<db::InstElement, db::SubCircuit *> subcircuits;

    for (connected_clusters_type::all_iterator c = clusters.begin_all (); ! c.at_end (); ++c) {

      db::Net *net = new db::Net ();
      net->set_cluster_id (*c);
      circuit->add_net (net);

      const db::local_cluster<db::PolygonRef>::global_nets &gn = clusters.cluster_by_id (*c).get_global_nets ();
      for (db::local_cluster<db::PolygonRef>::global_nets::const_iterator g = gn.begin (); g != gn.end (); ++g) {
        assign_net_name (conn.global_net_name (*g), net);
      }

      //  make subcircuit connections (also make the subcircuits if required) from the connections of the clusters
      make_and_connect_subcircuits (circuit, clusters, *c, net, subcircuits, circuits, pins_per_cluster_per_cell, layout.dbu ());

      //  collect the properties - we know that the cluster attributes are property ID's because the
      //  cluster processor converts shape property IDs to attributes
      const local_cluster_type &lc = clusters.cluster_by_id (*c);
      for (local_cluster_type::attr_iterator a = lc.begin_attr (); a != lc.end_attr (); ++a) {
        const db::PropertiesRepository::properties_set &ps = layout.properties_repository ().properties (*a);
        for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end (); ++j) {
          if (terminal_annot_name_id.first && j->first == terminal_annot_name_id.second) {
            make_device_terminal_from_property (j->second, circuit, net);
          } else if (text_annot_name_id.first && j->first == text_annot_name_id.second) {
            assign_net_name (j->second.to_string (), net);
          }
        }
      }

      if (! clusters.is_root (*c)) {
        //  a non-root cluster makes a pin
        size_t pin_id = make_pin (circuit, net);
        c2p.insert (std::make_pair (*c, pin_id));
      }

    }

  }
}

void NetlistExtractor::make_and_connect_subcircuits (db::Circuit *circuit,
                                                     const connected_clusters_type &clusters,
                                                     size_t cid,
                                                     db::Net *net,
                                                     std::map<db::InstElement, db::SubCircuit *> &subcircuits,
                                                     const std::map<db::cell_index_type, db::Circuit *> &circuits,
                                                     const std::map<db::cell_index_type, std::map<size_t, size_t> > &pins_per_cluster,
                                                     double dbu)
{
  const connected_clusters_type::connections_type &connections = clusters.connections_for_cluster (cid);
  for (connected_clusters_type::connections_type::const_iterator i = connections.begin (); i != connections.end (); ++i) {

    db::SubCircuit *subcircuit = 0;
    db::cell_index_type ccid = i->inst ().inst_ptr.cell_index ();

    std::map<db::InstElement, db::SubCircuit *>::const_iterator j = subcircuits.find (i->inst ());
    if (j == subcircuits.end ()) {

      //  make subcircuit if required

      std::map<db::cell_index_type, db::Circuit *>::const_iterator k = circuits.find (ccid);
      tl_assert (k != circuits.end ());  //  because we walk bottom-up

      subcircuit = new db::SubCircuit (k->second);
      db::CplxTrans dbu_trans (dbu);
      subcircuit->set_trans (dbu_trans * i->inst ().complex_trans () * dbu_trans.inverted ());
      circuit->add_subcircuit (subcircuit);
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
}

size_t NetlistExtractor::make_pin (db::Circuit *circuit, db::Net *net)
{
  size_t pin_id = circuit->add_pin (net->name ()).id ();
  net->add_pin (db::NetPinRef (pin_id));

  circuit->connect_pin (pin_id, net);

  return pin_id;
}

void NetlistExtractor::make_device_terminal_from_property (const tl::Variant &v, db::Circuit *circuit, db::Net *net)
{
  if (v.is_user<db::NetlistProperty> ()) {
    const db::NetlistProperty *np = &v.to_user<db::NetlistProperty> ();
    const db::DeviceTerminalProperty *tp = dynamic_cast<const db::DeviceTerminalProperty *> (np);
    if (tp) {
      db::Device *device = circuit->device_by_id (tp->device_id ());
      tl_assert (device != 0);
      device->connect_terminal (tp->terminal_id (), net);
    }
  }
}

void NetlistExtractor::assign_net_name (const std::string &n, db::Net *net)
{
  std::string nn = n;
  if (! nn.empty ()) {
    if (! net->name ().empty ()) {
      nn = net->name () + "," + nn;
    }
    net->set_name (nn);
  }
}

}

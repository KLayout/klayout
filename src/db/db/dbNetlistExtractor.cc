
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

#include "dbNetlistExtractor.h"
#include "dbDeepShapeStore.h"
#include "dbNetlistDeviceExtractor.h"
#include "tlGlobPattern.h"

namespace db
{

NetlistExtractor::NetlistExtractor ()
  : mp_clusters (0), mp_layout (0), mp_cell (0)
{
  //  .. nothing yet ..
}

static void
build_net_name_equivalence (const db::Layout *layout, db::property_names_id_type net_name_id, const std::string &joined_net_names, tl::equivalence_clusters<unsigned int> &eq)
{
  std::map<std::string, std::set<unsigned int> > prop_by_name;
  tl::GlobPattern jn_pattern (joined_net_names);

  for (db::PropertiesRepository::iterator i = layout->properties_repository ().begin (); i != layout->properties_repository ().end (); ++i) {
    for (db::PropertiesRepository::properties_set::const_iterator p = i->second.begin (); p != i->second.end (); ++p) {
      if (p->first == net_name_id) {
        std::string nn = p->second.to_string ();
        if (jn_pattern.match (nn)) {
          prop_by_name [nn].insert (i->first);
        }
      }
    }
  }

  for (std::map<std::string, std::set<unsigned int> >::const_iterator pn = prop_by_name.begin (); pn != prop_by_name.end (); ++pn) {
    std::set<unsigned int>::const_iterator p = pn->second.begin ();
    std::set<unsigned int>::const_iterator p0 = p;
    while (p != pn->second.end ()) {
      eq.same (*p0, *p);
      ++p;
    }
  }
}

void
NetlistExtractor::extract_nets (const db::DeepShapeStore &dss, unsigned int layout_index, const db::Connectivity &conn, db::Netlist &nl, hier_clusters_type &clusters, const std::string &joined_net_names)
{
  mp_clusters = &clusters;
  mp_layout = &dss.const_layout (layout_index);
  mp_cell = &dss.const_initial_cell (layout_index);

  //  gets the text annotation property ID -
  //  this is how the texts are passed for annotating the net names
  m_text_annot_name_id = std::pair<bool, db::property_names_id_type> (false, 0);
  if (! dss.text_property_name ().is_nil ()) {
    m_text_annot_name_id = mp_layout->properties_repository ().get_id_of_name (dss.text_property_name ());
  }

  m_terminal_annot_name_id = mp_layout->properties_repository ().get_id_of_name (db::NetlistDeviceExtractor::terminal_id_property_name ());
  m_device_annot_name_id = mp_layout->properties_repository ().get_id_of_name (db::NetlistDeviceExtractor::device_id_property_name ());

  //  the big part: actually extract the nets

  tl::equivalence_clusters<unsigned int> net_name_equivalence;
  if (m_text_annot_name_id.first && ! joined_net_names.empty ()) {
    build_net_name_equivalence (mp_layout, m_text_annot_name_id.second, joined_net_names, net_name_equivalence);
  }
  mp_clusters->build (*mp_layout, *mp_cell, db::ShapeIterator::Polygons, conn, &net_name_equivalence);

  //  reverse lookup for Circuit vs. cell index
  std::map<db::cell_index_type, db::Circuit *> circuits;

  //  some circuits may be there because of device extraction
  for (db::Netlist::circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {
    tl_assert (mp_layout->is_valid_cell_index (c->cell_index ()));
    circuits.insert (std::make_pair (c->cell_index (), c.operator-> ()));
  }

  std::map<db::cell_index_type, std::map<size_t, size_t> > pins_per_cluster_per_cell;
  for (db::Layout::bottom_up_const_iterator cid = mp_layout->begin_bottom_up (); cid != mp_layout->end_bottom_up (); ++cid) {

    const connected_clusters_type &clusters = mp_clusters->clusters_per_cell (*cid);
    if (clusters.empty ()) {
      continue;
    }

    db::DeviceAbstract *dm = nl.device_abstract_by_cell_index (*cid);
    if (dm) {
      //  make the terminal to cluster ID connections for the device abstract from the device cells
      make_device_abstract_connections (dm, clusters);
      continue;
    }

    //  a cell makes a new circuit (or uses an existing one)

    db::Circuit *circuit = 0;

    std::map<db::cell_index_type, db::Circuit *>::const_iterator k = circuits.find (*cid);
    if (k == circuits.end ()) {
      circuit = new db::Circuit ();
      nl.add_circuit (circuit);
      circuit->set_name (mp_layout->cell_name (*cid));
      circuit->set_cell_index (*cid);
      circuits.insert (std::make_pair (*cid, circuit));
    } else {
      circuit = k->second;
    }

    std::map<size_t, size_t> &c2p = pins_per_cluster_per_cell [*cid];

    std::map<std::pair<db::cell_index_type, db::ICplxTrans>, db::SubCircuit *> subcircuits;

    for (connected_clusters_type::all_iterator c = clusters.begin_all (); ! c.at_end (); ++c) {

      const db::local_cluster<db::PolygonRef> &lc = clusters.cluster_by_id (*c);
      if (clusters.connections_for_cluster (*c).empty () && lc.empty ()) {
        //  this is an entirely empty cluster so we skip it.
        //  Such clusters are left over when joining clusters.
        continue;
      }

      db::Net *net = new db::Net ();
      net->set_cluster_id (*c);
      circuit->add_net (net);

      const db::local_cluster<db::PolygonRef>::global_nets &gn = lc.get_global_nets ();
      for (db::local_cluster<db::PolygonRef>::global_nets::const_iterator g = gn.begin (); g != gn.end (); ++g) {
        assign_net_name (conn.global_net_name (*g), net);
      }

      //  make subcircuit connections (also make the subcircuits if required) from the connections of the clusters
      make_and_connect_subcircuits (circuit, clusters, *c, net, subcircuits, circuits, pins_per_cluster_per_cell);

      //  connect devices
      connect_devices (circuit, clusters, *c, net);

      //  collect labels to net names
      collect_labels (clusters, *c, net);

      if (! clusters.is_root (*c)) {
        //  a non-root cluster makes a pin
        size_t pin_id = make_pin (circuit, net);
        c2p.insert (std::make_pair (*c, pin_id));
      }

    }

  }
}

void
NetlistExtractor::make_device_abstract_connections (db::DeviceAbstract *dm, const connected_clusters_type &clusters)
{
  //  make the terminal to cluster ID connections for the device abstract from the device cells

  if (m_terminal_annot_name_id.first) {

    for (connected_clusters_type::const_iterator dc = clusters.begin (); dc != clusters.end (); ++dc) {

      for (local_cluster_type::attr_iterator a = dc->begin_attr (); a != dc->end_attr (); ++a) {

        const db::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (*a);
        for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end (); ++j) {
          if (j->first == m_terminal_annot_name_id.second) {
            dm->set_cluster_id_for_terminal (j->second.to<size_t> (), dc->id ());
          }
        }

      }

    }

  }
}

void NetlistExtractor::collect_labels (const connected_clusters_type &clusters,
                                       size_t cid,
                                       db::Net *net)
{
  //  collect the properties - we know that the cluster attributes are property ID's because the
  //  cluster processor converts shape property IDs to attributes

  const local_cluster_type &lc = clusters.cluster_by_id (cid);
  for (local_cluster_type::attr_iterator a = lc.begin_attr (); a != lc.end_attr (); ++a) {

    const db::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (*a);
    for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end (); ++j) {

      if (m_text_annot_name_id.first && j->first == m_text_annot_name_id.second) {
        assign_net_name (j->second.to_string (), net);
      }

    }

  }
}

bool NetlistExtractor::instance_is_device (db::properties_id_type prop_id) const
{
  if (! prop_id || ! m_device_annot_name_id.first) {
    return false;
  }

  const db::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (prop_id);
  for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end (); ++j) {
    if (j->first == m_device_annot_name_id.second) {
      return true;
    }
  }

  return false;
}

db::Device *NetlistExtractor::device_from_instance (db::properties_id_type prop_id, db::Circuit *circuit) const
{
  if (! prop_id || ! m_device_annot_name_id.first) {
    return 0;
  }

  const db::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (prop_id);
  for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end (); ++j) {
    if (j->first == m_device_annot_name_id.second) {
      return circuit->device_by_id (j->second.to<size_t> ());
    }
  }

  return 0;
}

void NetlistExtractor::connect_devices (db::Circuit *circuit,
                                        const connected_clusters_type &clusters,
                                        size_t cid,
                                        db::Net *net)
{
  const connected_clusters_type::connections_type &connections = clusters.connections_for_cluster (cid);
  for (connected_clusters_type::connections_type::const_iterator i = connections.begin (); i != connections.end (); ++i) {

    db::cell_index_type inst_cell_index = i->inst_cell_index ();
    db::properties_id_type inst_prop_id = i->inst_prop_id ();

    //  only consider devices in this pass
    db::Device *device = device_from_instance (inst_prop_id, circuit);
    if (! device) {
      continue;
    }

    const db::local_cluster<db::PolygonRef> &dc = mp_clusters->clusters_per_cell (inst_cell_index).cluster_by_id (i->id ());

    //  connect the net to the terminal of the device: take the terminal ID from the properties on the
    //  device cluster
    for (local_cluster_type::attr_iterator a = dc.begin_attr (); a != dc.end_attr (); ++a) {

      const db::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (*a);
      for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end (); ++j) {

        if (m_terminal_annot_name_id.first && j->first == m_terminal_annot_name_id.second) {

          size_t tid = j->second.to<size_t> ();
          device->connect_terminal (tid, net);

        }

      }

    }

  }
}

void NetlistExtractor::make_and_connect_subcircuits (db::Circuit *circuit,
                                                     const connected_clusters_type &clusters,
                                                     size_t cid,
                                                     db::Net *net,
                                                     std::map<std::pair<db::cell_index_type, db::ICplxTrans>, db::SubCircuit *> &subcircuits,
                                                     const std::map<db::cell_index_type, db::Circuit *> &circuits,
                                                     const std::map<db::cell_index_type, std::map<size_t, size_t> > &pins_per_cluster)
{
  const connected_clusters_type::connections_type &connections = clusters.connections_for_cluster (cid);
  for (connected_clusters_type::connections_type::const_iterator i = connections.begin (); i != connections.end (); ++i) {

    db::cell_index_type inst_cell_index = i->inst_cell_index ();
    db::properties_id_type inst_prop_id = i->inst_prop_id ();
    const db::ICplxTrans &inst_trans = i->inst_trans ();

    //  skip devices in this pass
    if (instance_is_device (inst_prop_id)) {
      continue;
    }

    db::SubCircuit *subcircuit = 0;

    std::pair<db::cell_index_type, db::ICplxTrans> subcircuit_key (inst_cell_index, inst_trans);

    std::map<std::pair<db::cell_index_type, db::ICplxTrans>, db::SubCircuit *>::const_iterator j = subcircuits.find (subcircuit_key);
    if (j == subcircuits.end ()) {

      //  make subcircuit if required

      std::map<db::cell_index_type, db::Circuit *>::const_iterator k = circuits.find (inst_cell_index);
      tl_assert (k != circuits.end ());  //  because we walk bottom-up

      subcircuit = new db::SubCircuit (k->second);
      db::CplxTrans dbu_trans (mp_layout->dbu ());
      subcircuit->set_trans (dbu_trans * inst_trans * dbu_trans.inverted ());
      circuit->add_subcircuit (subcircuit);
      subcircuits.insert (std::make_pair (subcircuit_key, subcircuit));

    } else {
      subcircuit = j->second;
    }

    //  create the pin connection to the subcircuit
    std::map<db::cell_index_type, std::map<size_t, size_t> >::const_iterator icc2p = pins_per_cluster.find (inst_cell_index);
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

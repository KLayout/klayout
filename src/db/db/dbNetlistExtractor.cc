
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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
#include "dbShapeRepository.h"
#include "tlGlobPattern.h"

namespace db
{

NetlistExtractor::NetlistExtractor ()
  : mp_clusters (0), mp_layout (0), mp_cell (0), m_include_floating_subcircuits (false)
{
  //  .. nothing yet ..
}

void NetlistExtractor::set_joined_net_names (const std::list<tl::GlobPattern>  &jnn)
{
  m_joined_net_names = jnn;
}

void NetlistExtractor::set_joined_net_names (const std::string &cellname, const std::list<tl::GlobPattern> &jnn)
{
  m_joined_net_names_per_cell.push_back (std::make_pair (cellname, jnn));
}

void NetlistExtractor::set_joined_nets (const std::list<std::set<std::string> > &jnn)
{
  m_joined_nets = jnn;
}

void NetlistExtractor::set_joined_nets (const std::string &cell_name, const std::list<std::set<std::string> > &jnn)
{
  m_joined_nets_per_cell.push_back (std::make_pair (cell_name, jnn));
}

void NetlistExtractor::set_include_floating_subcircuits (bool f)
{
  m_include_floating_subcircuits = f;
}

static void
build_net_name_equivalence (const db::Layout *layout, const db::Connectivity &conn, db::property_names_id_type net_name_id, const std::list<tl::GlobPattern> &jn_pattern, tl::equivalence_clusters<size_t> &eq)
{
  std::map<std::string, std::set<size_t> > prop_by_name;

  for (db::PropertiesRepository::iterator i = layout->properties_repository ().begin (); i != layout->properties_repository ().end (); ++i) {
    for (db::PropertiesRepository::properties_set::const_iterator p = i->second.begin (); p != i->second.end (); ++p) {
      if (p->first == net_name_id) {
        std::string nn = p->second.to_string ();
        for (std::list<tl::GlobPattern>::const_iterator jp = jn_pattern.begin (); jp != jn_pattern.end (); ++jp) {
          if (jp->match (nn)) {
            prop_by_name [nn].insert (db::prop_id_to_attr (i->first));
          }
        }
      }
    }
  }

  //  include pseudo-attributes for global nets to implement "join_with" for global nets
  for (size_t gid = 0; gid < conn.global_nets (); ++gid) {
    const std::string &gn = conn.global_net_name (gid);
    for (std::list<tl::GlobPattern>::const_iterator jp = jn_pattern.begin (); jp != jn_pattern.end (); ++jp) {
      if (jp->match (gn)) {
        prop_by_name [gn].insert (db::global_net_id_to_attr (gid));
      }
    }
  }

  const db::repository<db::Text> &text_repository = layout->shape_repository ().repository (db::object_tag<db::Text> ());
  for (db::repository<db::Text>::iterator t = text_repository.begin (); t != text_repository.end (); ++t) {
    std::string nn = t->string ();
    for (std::list<tl::GlobPattern>::const_iterator jp = jn_pattern.begin (); jp != jn_pattern.end (); ++jp) {
      if (jp->match (nn)) {
        prop_by_name [nn].insert (db::text_ref_to_attr (t.operator-> ()));
      }
    }
  }

  for (std::map<std::string, std::set<size_t> >::const_iterator pn = prop_by_name.begin (); pn != prop_by_name.end (); ++pn) {
    std::set<size_t>::const_iterator p = pn->second.begin ();
    std::set<size_t>::const_iterator p0 = p;
    while (p != pn->second.end ()) {
      eq.same (*p0, *p);
      ++p;
    }
  }
}

static void
build_net_name_equivalence_for_explicit_connections (const db::Layout *layout, const db::Connectivity &conn, db::property_names_id_type net_name_id, const std::set<std::string> &nets_to_join, tl::equivalence_clusters<size_t> &eq)
{
  std::map<std::string, std::set<size_t> > prop_by_name;

  for (db::PropertiesRepository::iterator i = layout->properties_repository ().begin (); i != layout->properties_repository ().end (); ++i) {
    for (db::PropertiesRepository::properties_set::const_iterator p = i->second.begin (); p != i->second.end (); ++p) {
      if (p->first == net_name_id) {
        std::string nn = p->second.to_string ();
        if (nets_to_join.find (nn) != nets_to_join.end ()) {
          prop_by_name [nn].insert (db::prop_id_to_attr (i->first));
        }
      }
    }
  }

  //  include pseudo-attributes for global nets to implement "join_with" for global nets
  for (size_t gid = 0; gid < conn.global_nets (); ++gid) {
    const std::string &gn = conn.global_net_name (gid);
    if (nets_to_join.find (gn) != nets_to_join.end ()) {
      prop_by_name [gn].insert (db::global_net_id_to_attr (gid));
    }
  }

  const db::repository<db::Text> &text_repository = layout->shape_repository ().repository (db::object_tag<db::Text> ());
  for (db::repository<db::Text>::iterator t = text_repository.begin (); t != text_repository.end (); ++t) {
    std::string nn = t->string ();
    if (nets_to_join.find (nn) != nets_to_join.end ()) {
      prop_by_name [nn].insert (db::text_ref_to_attr (t.operator-> ()));
    }
  }

  //  first inter-name equivalence (this implies implicit connections for all n1 and n2 labels)
  for (std::map<std::string, std::set<size_t> >::const_iterator pn = prop_by_name.begin (); pn != prop_by_name.end (); ++pn) {
    std::set<size_t>::const_iterator p = pn->second.begin ();
    std::set<size_t>::const_iterator p0 = p;
    while (p != pn->second.end ()) {
      eq.same (*p0, *p);
      ++p;
    }
  }

  //  second intra-name equivalence
  for (std::map<std::string, std::set<size_t> >::const_iterator pn1 = prop_by_name.begin (); pn1 != prop_by_name.end (); ++pn1) {
    std::map<std::string, std::set<size_t> >::const_iterator pn2 = pn1;
    ++pn2;
    for ( ; pn2 != prop_by_name.end (); ++pn2) {
      eq.same (*pn1->second.begin (), *pn2->second.begin ());
    }
  }
}

void
NetlistExtractor::extract_nets (const db::DeepShapeStore &dss, unsigned int layout_index, const db::Connectivity &conn, db::Netlist &nl, hier_clusters_type &clusters)
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

  //  build an attribute equivalence map which lists the "attribute IDs" which are identical in terms of net names
  //  TODO: this feature is not really used as must-connect nets now are handled in the LayoutToNetlist class on netlist level.
  //  Remove this later.

  std::map<db::cell_index_type, tl::equivalence_clusters<size_t> > net_name_equivalence;
  if (m_text_annot_name_id.first) {

    if (! m_joined_net_names.empty ()) {
      build_net_name_equivalence (mp_layout, conn, m_text_annot_name_id.second, m_joined_net_names, net_name_equivalence [hier_clusters_type::top_cell_index]);
    }
    for (std::list<std::pair<std::string, std::list<tl::GlobPattern> > >::const_iterator m = m_joined_net_names_per_cell.begin (); m != m_joined_net_names_per_cell.end (); ++m) {
      std::pair<bool, db::cell_index_type> cp = mp_layout->cell_by_name (m->first.c_str ());
      if (cp.first) {
        build_net_name_equivalence (mp_layout, conn, m_text_annot_name_id.second, m->second, net_name_equivalence [cp.second]);
      }
    }

    if (! m_joined_nets.empty ()) {
      for (std::list<std::set<std::string> >::const_iterator n = m_joined_nets.begin (); n != m_joined_nets.end (); ++n) {
        build_net_name_equivalence_for_explicit_connections (mp_layout, conn, m_text_annot_name_id.second, *n, net_name_equivalence [hier_clusters_type::top_cell_index]);
      }
    }
    for (std::list<std::pair<std::string, std::list<std::set<std::string> > > >::const_iterator m = m_joined_nets_per_cell.begin (); m != m_joined_nets_per_cell.end (); ++m) {
      std::pair<bool, db::cell_index_type> cp = mp_layout->cell_by_name (m->first.c_str ());
      if (cp.first) {
        for (std::list<std::set<std::string> >::const_iterator n = m->second.begin (); n != m->second.end (); ++n) {
          build_net_name_equivalence_for_explicit_connections (mp_layout, conn, m_text_annot_name_id.second, *n, net_name_equivalence [cp.second]);
        }
      }
    }

  }

  //  the big part: actually extract the nets

  mp_clusters->build (*mp_layout, *mp_cell, conn, &net_name_equivalence);

  //  reverse lookup for Circuit vs. cell index
  std::map<db::cell_index_type, db::Circuit *> circuits;

  //  some circuits may be there because of device extraction
  for (db::Netlist::circuit_iterator c = nl.begin_circuits (); c != nl.end_circuits (); ++c) {
    tl_assert (mp_layout->is_valid_cell_index (c->cell_index ()));
    circuits.insert (std::make_pair (c->cell_index (), c.operator-> ()));
  }

  std::map<db::cell_index_type, std::map<size_t, size_t> > pins_per_cluster_per_cell;
  for (db::Layout::bottom_up_const_iterator cid = mp_layout->begin_bottom_up (); cid != mp_layout->end_bottom_up (); ++cid) {

    const db::Cell &cell = mp_layout->cell (*cid);

    const connected_clusters_type &clusters = mp_clusters->clusters_per_cell (*cid);
    if (clusters.empty ()) {

      bool any_good = false;

      //  in case of "include floating subcircuits" check whether we have a child cell which has a circuit attached in this case
      if (include_floating_subcircuits ()) {
        for (db::Cell::child_cell_iterator cc = cell.begin_child_cells (); ! any_good && ! cc.at_end (); ++cc) {
          any_good = (circuits.find (*cc) != circuits.end ());
        }
      }

      if (! any_good) {
        //  skip this cell
        continue;
      }

    }

    db::DeviceAbstract *dm = nl.device_abstract_by_cell_index (*cid);
    if (dm) {
      //  This is a device abstract cell:
      //  make the terminal to cluster ID connections for the device abstract from the device cells
      make_device_abstract_connections (dm, clusters);
      continue;
    }

    //  a cell makes a new circuit (or uses an existing one)

    db::Circuit *circuit = 0;

    std::map<db::cell_index_type, db::Circuit *>::const_iterator k = circuits.find (*cid);
    if (k == circuits.end ()) {
      circuit = new db::Circuit (*mp_layout, *cid);
      nl.add_circuit (circuit);
      circuits.insert (std::make_pair (*cid, circuit));
    } else {
      circuit = k->second;
    }

    std::map<size_t, size_t> &c2p = pins_per_cluster_per_cell [*cid];

    std::map<std::pair<db::cell_index_type, db::ICplxTrans>, db::SubCircuit *> subcircuits;

    if (include_floating_subcircuits ()) {

      //  Make sure we create one subcircuit for each instance of cells which do have circuits
      //  associated.
      for (db::Cell::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
        for (db::CellInstArray::iterator ii = inst->begin (); ! ii.at_end (); ++ii) {
          make_subcircuit (circuit, inst->cell_index (), inst->complex_trans (*ii), subcircuits, circuits);
        }
      }

    }

    for (connected_clusters_type::all_iterator c = clusters.begin_all (); ! c.at_end (); ++c) {

      const db::local_cluster<db::NetShape> &lc = clusters.cluster_by_id (*c);
      const connected_clusters_type::connections_type &cc = clusters.connections_for_cluster (*c);
      if (cc.empty () && lc.empty ()) {
        //  this is an entirely empty cluster so we skip it.
        //  Such clusters are left over when joining clusters.
        continue;
      }

      db::Net *net = new db::Net ();
      net->set_cluster_id (*c);
      circuit->add_net (net);

      //  make subcircuit connections (also make the subcircuits if required) from the connections of the clusters
      make_and_connect_subcircuits (circuit, clusters, *c, net, subcircuits, circuits, pins_per_cluster_per_cell);

      //  connect devices
      connect_devices (circuit, clusters, *c, net);

      //  collect labels to net names
      std::set<std::string> net_names;
      collect_labels (clusters, *c, net_names);

      //  add the global names as second priority
      if (net_names.empty ()) {
        const db::local_cluster<db::NetShape>::global_nets &gn = lc.get_global_nets ();
        for (db::local_cluster<db::NetShape>::global_nets::const_iterator g = gn.begin (); g != gn.end (); ++g) {
          net_names.insert (conn.global_net_name (*g));
        }
      }

#if 0
      //  This code will pull net names from subcircuits into their parents if those nets are dummy connections
      //  made to satisfy the subcircuit's pin, but not to make a physical connection.
      //  Don't know whether this is a good idea, so this code is disabled for now.

      if (net_names.empty () && clusters.is_dummy (*c) && net->subcircuit_pin_count () == 1) {
        //  in the case of a dummy connection (partially connected subcircuits) create a
        //  new name indicating the subcircuit and the subcircuit net name - this makes subcircuit
        //  net names available (the net is pseudo-root inside in the subcircuit)
        const db::NetSubcircuitPinRef &sc_pin = *net->begin_subcircuit_pins ();
        const db::Net *sc_net = sc_pin.subcircuit ()->circuit_ref ()->net_for_pin (sc_pin.pin_id ());
        if (sc_net && ! sc_net->name ().empty ()) {
          net_names.insert (sc_pin.subcircuit ()->expanded_name () + ":" + sc_net->name ());
        }
      }
#endif

      assign_net_names (net, net_names);

      if (! clusters.is_root (*c)) {
        //  a non-root cluster makes a pin
        size_t pin_id = make_pin (circuit, net);
        c2p.insert (std::make_pair (*c, pin_id));
      }

    }

  }
}

void
NetlistExtractor::assign_net_names (db::Net *net, const std::set<std::string> &net_names)
{
  std::string nn;
  for (std::set<std::string>::const_iterator n = net_names.begin (); n != net_names.end (); ++n) {
    if (! n->empty ()) {
      if (! nn.empty ()) {
        nn += ",";
      }
      nn += *n;
    }
  }

  net->set_name (nn);
}

void
NetlistExtractor::make_device_abstract_connections (db::DeviceAbstract *dm, const connected_clusters_type &clusters)
{
  //  make the terminal to cluster ID connections for the device abstract from the device cells

  if (m_terminal_annot_name_id.first) {

    for (connected_clusters_type::const_iterator dc = clusters.begin (); dc != clusters.end (); ++dc) {

      for (local_cluster_type::attr_iterator a = dc->begin_attr (); a != dc->end_attr (); ++a) {

        if (! db::is_prop_id_attr (*a)) {
          continue;
        }

        db::properties_id_type pi = db::prop_id_from_attr (*a);

        const db::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (pi);
        for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end (); ++j) {
          if (j->first == m_terminal_annot_name_id.second) {
            dm->set_cluster_id_for_terminal (j->second.to<size_t> (), dc->id ());
          }
        }

      }

    }

  }

  //  check whether all connections have been made
  const std::vector<db::DeviceTerminalDefinition> &td = dm->device_class ()->terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
    if (! dm->cluster_id_for_terminal (t->id ())) {
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("Terminal '%s' of a device of class '%s' isn't connected - maybe the terminal annotation layer of this device type isn't part of the connectivity?")), t->name (), dm->device_class ()->name ()));
    }
  }
}

void NetlistExtractor::collect_labels (const connected_clusters_type &clusters,
                                       size_t cid,
                                       std::set<std::string> &net_names)
{
  //  collect the properties - we know that the cluster attributes are property ID's because the
  //  cluster processor converts shape property IDs to attributes

  const local_cluster_type &lc = clusters.cluster_by_id (cid);
  for (local_cluster_type::attr_iterator a = lc.begin_attr (); a != lc.end_attr (); ++a) {

    if (db::is_prop_id_attr (*a)) {

      db::properties_id_type pi = db::prop_id_from_attr (*a);

      const db::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (pi);
      for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end (); ++j) {

        if (m_text_annot_name_id.first && j->first == m_text_annot_name_id.second) {
          net_names.insert (j->second.to_string ());
        }

      }

    } else if (db::is_text_ref_attr (*a)) {

      net_names.insert (db::text_from_attr (*a));

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

    const db::local_cluster<db::NetShape> &dc = mp_clusters->clusters_per_cell (inst_cell_index).cluster_by_id (i->id ());

    //  connect the net to the terminal of the device: take the terminal ID from the properties on the
    //  device cluster
    for (local_cluster_type::attr_iterator a = dc.begin_attr (); a != dc.end_attr (); ++a) {

      if (! db::is_prop_id_attr (*a)) {
        continue;
      }

      db::properties_id_type pi = db::prop_id_from_attr (*a);

      const db::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (pi);
      for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end (); ++j) {

        if (m_terminal_annot_name_id.first && j->first == m_terminal_annot_name_id.second) {

          size_t tid = j->second.to<size_t> ();
          device->connect_terminal (tid, net);

        }

      }

    }

  }
}

db::SubCircuit * NetlistExtractor::make_subcircuit (db::Circuit *circuit,
                                                    db::cell_index_type inst_cell_index,
                                                    const db::ICplxTrans &inst_trans,
                                                    std::map<std::pair<db::cell_index_type, db::ICplxTrans>, db::SubCircuit *> &subcircuits,
                                                    const std::map<db::cell_index_type, db::Circuit *> &circuits)
{
  db::SubCircuit *subcircuit = 0;

  std::pair<db::cell_index_type, db::ICplxTrans> subcircuit_key (inst_cell_index, inst_trans);

  std::map<std::pair<db::cell_index_type, db::ICplxTrans>, db::SubCircuit *>::const_iterator j = subcircuits.find (subcircuit_key);
  if (j == subcircuits.end ()) {

    //  make subcircuit if required

    std::map<db::cell_index_type, db::Circuit *>::const_iterator k = circuits.find (inst_cell_index);
    if (k == circuits.end ()) {
      return 0;
    }

    subcircuit = new db::SubCircuit (k->second);
    db::CplxTrans dbu_trans (mp_layout->dbu ());
    subcircuit->set_trans (dbu_trans * inst_trans * dbu_trans.inverted ());
    circuit->add_subcircuit (subcircuit);
    subcircuits.insert (std::make_pair (subcircuit_key, subcircuit));

  } else {
    subcircuit = j->second;
  }

  return subcircuit;
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

    db::SubCircuit *subcircuit = make_subcircuit (circuit, inst_cell_index, inst_trans, subcircuits, circuits);
    tl_assert (subcircuit != 0);

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

}

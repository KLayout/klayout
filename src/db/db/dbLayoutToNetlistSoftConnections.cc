

/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "dbCommon.h"
#include "dbLayoutToNetlistSoftConnections.h"
#include "dbLayoutToNetlist.h"
#include "dbHierNetworkProcessor.h"
#include "dbNetlist.h"

namespace db
{

// -------------------------------------------------------------------------------
//  SoftConnectionNetGraph implementation

SoftConnectionNetGraph::SoftConnectionNetGraph ()
  : m_partial_net_count (0)
{
  //  .. nothing yet ..
}

void SoftConnectionNetGraph::add (const db::Net *net, SoftConnectionPinDir dir, const db::Pin *pin, size_t partial_net_count)
{
  m_partial_net_count += partial_net_count;

  //  this is where we make the decision about the partial nets ...
  if (! pin && dir == SoftConnectionPinDir::down ()) {
    m_partial_net_count += 1;
  }

  if (pin) {
    m_pin_ids.insert (pin->id ());
  }

  m_cluster_dir.insert (std::make_pair (net->cluster_id (), dir));
}

// -------------------------------------------------------------------------------
//  SoftConnectionCircuitInfo implementation

SoftConnectionCircuitInfo::SoftConnectionCircuitInfo (const db::Circuit *circuit)
  : mp_circuit (circuit)
{
  //  .. nothing yet ..
}

SoftConnectionNetGraph &SoftConnectionCircuitInfo::make_net_graph ()
{
  m_net_graphs.push_back (SoftConnectionNetGraph ());
  return m_net_graphs.back ();
}

void SoftConnectionCircuitInfo::add_pin_info (const db::Pin *pin, SoftConnectionPinDir dir, SoftConnectionNetGraph *graph_info)
{
  if (pin) {
    m_pin_info.insert (std::make_pair (pin->id (), std::make_pair (dir, graph_info)));
  }
}

SoftConnectionPinDir SoftConnectionCircuitInfo::direction_per_pin (const db::Pin *pin) const
{
  if (! pin) {
    return SoftConnectionPinDir ();
  }

  auto p = m_pin_info.find (pin->id ());
  return p != m_pin_info.end () ? p->second.first : SoftConnectionPinDir ();
}

const SoftConnectionNetGraph *SoftConnectionCircuitInfo::get_net_graph_per_pin (const db::Pin *pin) const
{
  if (! pin) {
    return 0;
  }

  auto p = m_pin_info.find (pin->id ());
  return p != m_pin_info.end () ? p->second.second : 0;
}

// -------------------------------------------------------------------------------
//  SoftConnectionInfo implementation

SoftConnectionInfo::SoftConnectionInfo ()
{
  //  .. nothing yet ..
}

void SoftConnectionInfo::build (const db::Netlist &netlist, const db::hier_clusters<db::NetShape> &shape_clusters)
{
  for (auto c = netlist.begin_bottom_up (); c != netlist.end_bottom_up (); ++c) {
    build_graphs_for_circuit (c.operator-> (), shape_clusters.clusters_per_cell (c->cell_index ()));
  }
}

void SoftConnectionInfo::join_soft_connections (db::Netlist &netlist)
{
  if (tl::verbosity () >= 20) {
    tl::info << "Joining soft-connected net graphs ..";
  }

  size_t nnet_graphs_tot = 0;
  size_t npartial_tot = 0;

  for (auto c = netlist.begin_top_down (); c != netlist.end_top_down (); ++c) {

    size_t nnet_graphs = 0;
    size_t npartial = 0;

    auto scc = m_scc_per_circuit.find (c.operator-> ());
    if (scc == m_scc_per_circuit.end ()) {
      continue;
    }

    const SoftConnectionCircuitInfo &sc_info = scc->second;

    for (auto sc = sc_info.begin (); sc != sc_info.end (); ++sc) {

      auto cc = sc->begin_clusters ();
      if (cc != sc->end_clusters ()) {
        db::Net *net0 = c->net_by_cluster_id (cc->first);
        tl_assert (net0 != 0);
        ++nnet_graphs;
        while (++cc != sc->end_clusters ()) {
          //  TODO: logging?
          c->join_nets (net0, c->net_by_cluster_id (cc->first));
          ++npartial;
        }
      }

    }

    nnet_graphs_tot += nnet_graphs;
    npartial_tot += npartial;

    if (nnet_graphs > 0 && tl::verbosity () >= 30) {
      tl::info << "Circuit " << c->name () << ": joined " << nnet_graphs << " soft-connected net clusters with " << npartial << " partial nets.";
    }

  }

  if (tl::verbosity () >= 20) {
    tl::info << "Joined " << nnet_graphs_tot << " soft-connected net clusters with " << npartial_tot << " partial nets in total.";
  }

  m_scc_per_circuit.clear ();
}

db::DPolygon
SoftConnectionInfo::representative_polygon (const db::Net *net, const db::LayoutToNetlist &l2n, const db::CplxTrans &trans)
{
  const db::Connectivity &conn = l2n.connectivity ();
  const db::hier_clusters<db::NetShape> &net_clusters = l2n.net_clusters ();

  db::DBox bbox;

  for (auto l = conn.begin_layers (); l != conn.end_layers (); ++l) {
    db::recursive_cluster_shape_iterator<db::NetShape> si (net_clusters, *l, net->circuit ()->cell_index (), net->cluster_id ());
    while (! si.at_end ()) {
      if (si->type () == db::NetShape::Polygon) {
        bbox += trans * (si.trans () * si->polygon_ref ().box ());
      }
      ++si;
    }
  }

  return db::DPolygon (bbox);
}

void SoftConnectionInfo::report_partial_nets (const db::Circuit *circuit, const SoftConnectionNetGraph &net_graph, db::LayoutToNetlist &l2n, const std::string &path, const db::DCplxTrans &trans, const std::string &top_cell, int &index, std::set<std::pair<const db::Net *, db::DCplxTrans> > &seen)
{
  for (auto cc = net_graph.begin_clusters (); cc != net_graph.end_clusters (); ++cc) {

    const db::Net *net = circuit->net_by_cluster_id (cc->first);

    if (! seen.insert (std::make_pair (net, trans)).second) {
      continue;
    }

    if (cc->second == SoftConnectionPinDir::down () && ! net->is_floating () && net->begin_pins () == net->end_pins ()) {

      std::string msg = tl::sprintf (tl::to_string (tr ("\tPartial net #%d: %s - %s")), ++index, path, net->expanded_name ());

      db::LogEntryData entry (db::NoSeverity, top_cell, msg);
      entry.set_geometry (representative_polygon (net, l2n, trans * db::CplxTrans (l2n.internal_layout ()->dbu ())));

      l2n.log_entry (entry);

    }

    for (auto sc = net->begin_subcircuit_pins (); sc != net->end_subcircuit_pins (); ++sc) {

      const db::SubCircuit *subcircuit = sc->subcircuit ();
      const db::Circuit *circuit_ref = subcircuit->circuit_ref ();

      auto scc = m_scc_per_circuit.find (circuit_ref);
      if (scc == m_scc_per_circuit.end ()) {
        continue;
      }

      const SoftConnectionCircuitInfo &sci = scc->second;

      const SoftConnectionNetGraph *scci = sci.get_net_graph_per_pin (sc->pin ());
      if (! scci || scci->partial_net_count () == 0) {
        continue;
      }

      std::string p = path;
      p += std::string ("/") + circuit_ref->name ();
      p += std::string ("[") + subcircuit->trans ().to_string (true /*short*/) + "]:" + subcircuit->expanded_name ();
      report_partial_nets (circuit_ref, *scci, l2n, p, trans * subcircuit->trans (), top_cell, index, seen);

    }

  }
}

void SoftConnectionInfo::report (db::LayoutToNetlist &l2n)
{
  const db::Netlist *netlist = l2n.netlist ();
  if (! netlist) {
    return;
  }

  for (auto c = netlist->begin_bottom_up (); c != netlist->end_bottom_up (); ++c) {

    auto scc = m_scc_per_circuit.find (c.operator-> ());
    if (scc == m_scc_per_circuit.end ()) {
      continue;
    }

    const SoftConnectionCircuitInfo &sc_info = scc->second;

    for (auto sc = sc_info.begin (); sc != sc_info.end (); ++sc) {

      if (sc->partial_net_count () < 2) {
        continue;
      }

      db::LogEntryData log_entry (l2n.top_level_mode () ? db::Error : db::Warning, c->name (), tl::to_string (tr ("Net with incomplete wiring (soft-connected partial nets)")));
      log_entry.set_category_name ("soft-connection-check");
      l2n.log_entry (log_entry);

      int index = 0;
      std::set<std::pair<const db::Net *, db::DCplxTrans> > seen;
      report_partial_nets (c.operator-> (), *sc, l2n, c->name (), db::DCplxTrans (), c->name (), index, seen);

    }

  }
}

void SoftConnectionInfo::build_graphs_for_circuit (const db::Circuit *circuit, const db::connected_clusters<db::NetShape> &shape_clusters)
{
  SoftConnectionCircuitInfo &sc_circuit_info = m_scc_per_circuit.insert (std::make_pair (circuit, SoftConnectionCircuitInfo (circuit))).first->second;

  std::set<size_t> seen;
  for (auto c = shape_clusters.begin (); c != shape_clusters.end (); ++c) {

    if (seen.find (c->id ()) != seen.end ()) {
      continue;
    }

    //  incrementally collect further connected nets (shape clusters)

    std::set<size_t> connected;
    connected.insert (c->id ());
    seen.insert (c->id ());

    SoftConnectionNetGraph *sc_net_graph = 0;

    while (! connected.empty ()) {

      std::set<size_t> next_connected;

      for (auto cc = connected.begin (); cc != connected.end (); ++cc) {

        const db::Net *net = circuit->net_by_cluster_id (*cc);
        if (! net) {
          continue;
        }

        //  the direction of a net is 0 for "no connections" or "both up and down"
        //  and -1 for "down-only" connections and +1 for "up-only" connections:

        SoftConnectionPinDir dir;

        //  direct soft connections to other nets

        for (int up = 0; up < 2; ++up) {
          std::set<size_t> next = up ? shape_clusters.upward_soft_connections (*cc) : shape_clusters.downward_soft_connections (*cc);
          if (! next.empty () || net_has_up_or_down_subcircuit_connections (net, up)) {
            dir |= up ? SoftConnectionPinDir::up () : SoftConnectionPinDir::down ();
          }
          for (auto i = next.begin (); i != next.end (); ++i) {
            if (seen.insert (*i).second) {
              next_connected.insert (*i);
            }
          }
        }

        //  collect soft connections via subcircuits

        size_t sc_partial_net_count = 0;
        std::set<size_t> next = net_connections_through_subcircuits (net, sc_partial_net_count);

        for (auto i = next.begin (); i != next.end (); ++i) {
          if (seen.insert (*i).second) {
            next_connected.insert (*i);
          }
        }

        //  is this net associated with a pin?

        const db::Pin *pin = 0;
        if (net->begin_pins () != net->end_pins ()) {
          //  TODO: multiple pins per net need to be supported?
          tl_assert (net->pin_count () == 1);
          pin = net->begin_pins ()->pin ();
        }

        if (! sc_net_graph) {
          sc_net_graph = &sc_circuit_info.make_net_graph ();
        }

        //  we do not count floating nets as they cannot make a functional connection
        if (! net->is_floating ()) {
          sc_net_graph->add (net, dir, pin, sc_partial_net_count);
        }

        sc_circuit_info.add_pin_info (pin, dir, sc_net_graph);

      }

      connected.swap (next_connected);

    }

  }

}

bool SoftConnectionInfo::net_has_up_or_down_subcircuit_connections (const db::Net *net, bool up)
{
  SoftConnectionPinDir look_for_dir = up ? SoftConnectionPinDir::up () : SoftConnectionPinDir::down ();

  for (auto sc = net->begin_subcircuit_pins (); sc != net->end_subcircuit_pins (); ++sc) {
    const db::Pin *pin = sc->pin ();
    const db::Circuit *ref = sc->subcircuit ()->circuit_ref ();
    auto scc_ref = m_scc_per_circuit.find (ref);
    if (scc_ref != m_scc_per_circuit.end ()) {
      SoftConnectionPinDir dir = scc_ref->second.direction_per_pin (pin);
      if (dir & look_for_dir) {
        return true;
      }
    }
  }

  return false;
}

void SoftConnectionInfo::get_net_connections_through_subcircuit (const db::SubCircuit *subcircuit, const db::Pin *pin, std::set<size_t> &ids, size_t &partial_net_count)
{
  auto scc = m_scc_per_circuit.find (subcircuit->circuit_ref ());
  if (scc != m_scc_per_circuit.end ()) {

    const SoftConnectionCircuitInfo &sci = scc->second;

    const SoftConnectionNetGraph *scci = sci.get_net_graph_per_pin (pin);
    if (scci) {

      //  NOTE: limiting the partial net count here means we do report a partially connected once in the
      //  hierarchy, not on every level.
      //  Say, if you have two subcircuits, one (A) having 2 partial nets and the other (B) none. Then
      //  (A) would be reported to partial nets only and when combining (A) and (B) we just need to check
      //  whether B would also have partial nets. By not taking 2 + 0, but 1 + 0 the combination of (A)
      //  and (B) does not give an error (error = number of partial nets > 1).
      partial_net_count += std::min (size_t (1), scci->partial_net_count ());

      for (auto p = scci->begin_pins (); p != scci->end_pins (); ++p) {
        if (*p != pin->id ()) {
          const NetSubcircuitPinRef *netref = subcircuit->netref_for_pin (*p);
          if (netref && netref->net ()) {
            ids.insert (netref->net ()->cluster_id ());
          }
        }
      }

    }

  }
}

std::set<size_t> SoftConnectionInfo::net_connections_through_subcircuits (const db::Net *net, size_t &partial_net_count)
{
  std::set<size_t> ids;
  for (auto sc = net->begin_subcircuit_pins (); sc != net->end_subcircuit_pins (); ++sc) {
    get_net_connections_through_subcircuit (sc->subcircuit (), sc->pin (), ids, partial_net_count);
  }
  return ids;
}

}

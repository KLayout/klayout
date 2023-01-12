
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

#include "dbNetlistCompareGraph.h"
#include "dbNetlistCompare.h"
#include "dbDevice.h"
#include "dbDeviceClass.h"
#include "dbNet.h"
#include "dbSubCircuit.h"
#include "dbCircuit.h"

#include "tlAssert.h"
#include "tlLog.h"

namespace db
{

// --------------------------------------------------------------------------------------------------------------------

static bool is_non_trivial_net (const db::Net *net)
{
  return net->pin_count () == 0 && net->terminal_count () == 0 && net->subcircuit_pin_count () == 1;
}

static size_t translate_terminal_id (size_t tid, const db::Device *device)
{
  return device->device_class () ? device->device_class ()->normalize_terminal_id (tid) : tid;
}

// --------------------------------------------------------------------------------------------------------------------
//  Transition implementation

Transition::Transition (const db::Device *device, size_t device_category, size_t terminal1_id, size_t terminal2_id)
{
  m_ptr = (void *) device;
  m_cat = device_category;
  tl_assert (terminal1_id < std::numeric_limits<size_t>::max () / 2);
  m_id1 = terminal1_id;
  m_id2 = terminal2_id;
}

Transition::Transition (const db::SubCircuit *subcircuit, size_t subcircuit_category, size_t pin1_id, size_t pin2_id)
{
  m_ptr = (void *) subcircuit;
  m_cat = subcircuit_category;
  //  m_id1 between max/2 and max indicates subcircuit
  tl_assert (pin1_id < std::numeric_limits<size_t>::max () / 2);
  m_id1 = std::numeric_limits<size_t>::max () - pin1_id;
  m_id2 = pin2_id;
}

size_t
Transition::first_unique_pin_id ()
{
  return std::numeric_limits<size_t>::max () / 4;
}

CatAndIds
Transition::make_key () const
{
  if (is_for_subcircuit ()) {
    return CatAndIds (m_cat, m_id1, size_t (0));
  } else {
    return CatAndIds (m_cat, m_id1, m_id2);
  }
}

bool
Transition::operator< (const Transition &other) const
{
  if (is_for_subcircuit () != other.is_for_subcircuit ()) {
    return is_for_subcircuit () < other.is_for_subcircuit ();
  }

  if (is_for_subcircuit ()) {

    if ((subcircuit () != 0) != (other.subcircuit () != 0)) {
      return (subcircuit () != 0) < (other.subcircuit () != 0);
    }

    if (subcircuit () != 0) {
      SubCircuitCompare scc;
      if (! scc.equals (std::make_pair (subcircuit (), cat ()), std::make_pair (other.subcircuit (), other.cat ()))) {
        return scc (std::make_pair (subcircuit (), cat ()), std::make_pair (other.subcircuit (), other.cat ()));
      }
    }

    return m_id1 < other.m_id1;

  } else {

    if ((device () != 0) != (other.device () != 0)) {
      return (device () != 0) < (other.device () != 0);
    }

    if (device () != 0) {
      DeviceCompare dc;
      if (! dc.equals (std::make_pair (device (), cat ()), std::make_pair (other.device (), other.cat ()))) {
        return dc (std::make_pair (device (), cat ()), std::make_pair (other.device (), other.cat ()));
      }
    }

    if (m_id1 != other.m_id1) {
      return m_id1 < other.m_id1;
    }
    return m_id2 < other.m_id2;

  }
}

bool
Transition::operator== (const Transition &other) const
{
  if (is_for_subcircuit () != other.is_for_subcircuit ()) {
    return false;
  }

  if (is_for_subcircuit ()) {

    if ((subcircuit () != 0) != (other.subcircuit () != 0)) {
      return false;
    }

    if (subcircuit () != 0) {
      SubCircuitCompare scc;
      if (! scc.equals (std::make_pair (subcircuit (), cat ()), std::make_pair (other.subcircuit (), other.cat ()))) {
        return false;
      }
    }

    return (m_id1 == other.m_id1);

  } else {

    if ((device () != 0) != (other.device () != 0)) {
      return false;
    }

    if (device () != 0) {
      DeviceCompare dc;
      if (! dc.equals (std::make_pair (device (), cat ()), std::make_pair (other.device (), other.cat ()))) {
        return false;
      }
    }

    return (m_id1 == other.m_id1 && m_id2 == other.m_id2);

  }
}

std::string
Transition::to_string () const
{
  if (is_for_subcircuit ()) {
    const db::SubCircuit *sc = subcircuit ();
    const db::Circuit *c = sc->circuit_ref ();
    return std::string ("X") + sc->expanded_name () + " " + c->name () + " " + c->pin_by_id (m_id2)->expanded_name () + " (virtual)";
 } else {
    size_t term_id1 = m_id1;
    size_t term_id2 = m_id2;
    const db::Device *d = device ();
    const db::DeviceClass *dc = d->device_class ();
    return std::string ("D") + d->expanded_name () + " " + dc->name () + " "
      + "(" + dc->terminal_definitions () [term_id1].name () + ")->(" + dc->terminal_definitions () [term_id2].name () + ")";
  }
}

// --------------------------------------------------------------------------------------------------------------------
//  NetGraphNode implementation

NetGraphNode::NetGraphNode (const db::Net *net, DeviceCategorizer &device_categorizer, CircuitCategorizer &circuit_categorizer, const DeviceFilter &device_filter, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinCategorizer *pin_map, size_t *unique_pin_id)
  : mp_net (net), m_other_net_index (invalid_id)
{
  if (! net) {
    return;
  }

  std::map<const void *, size_t> n2entry;

  for (db::Net::const_subcircuit_pin_iterator i = net->begin_subcircuit_pins (); i != net->end_subcircuit_pins (); ++i) {

    const db::SubCircuit *sc = i->subcircuit ();
    size_t circuit_cat = circuit_categorizer.cat_for_subcircuit (sc);
    if (! circuit_cat) {
      //  circuit is ignored
      continue;
    }

    size_t pin_id = i->pin ()->id ();
    const db::Circuit *cr = sc->circuit_ref ();

    std::map<const db::Circuit *, CircuitMapper>::const_iterator icm = circuit_map->find (cr);
    if (icm == circuit_map->end ()) {
      //  this can happen if the other circuit is not present - this is allowed for single-pin
      //  circuits.
      continue;
    }

    const CircuitMapper *cm = & icm->second;

    //  A pin assignment may be missing because there is no (real) net for a pin -> skip this pin

    size_t original_pin_id = pin_id;

    if (! cm->has_other_pin_for_this_pin (pin_id)) {

      //  isolated pins are ignored, others are considered for the matching
      if (! unique_pin_id || is_non_trivial_net (net)) {
        continue;
      } else {
        pin_id = (*unique_pin_id)++;
      }

    } else {

      //  NOTE: if cm is given, cr and pin_id are given in terms of the canonical "other" circuit.
      //  For c1 this is the c1->c2 mapper, for c2 this is the c2->c2 dummy mapper.

      pin_id = cm->other_pin_from_this_pin (pin_id);

      //  realize pin swapping by normalization of pin ID

      pin_id = pin_map->normalize_pin_id (cm->other (), pin_id);

    }

    //  Subcircuits are routed to a null node and descend from a virtual node inside the subcircuit.
    //  The reasoning is that this way we don't need #pins*(#pins-1) edges but rather #pins.

    Transition ed (sc, circuit_cat, pin_id, original_pin_id);

    std::map<const void *, size_t>::const_iterator in = n2entry.find ((const void *) sc);
    if (in == n2entry.end ()) {
      in = n2entry.insert (std::make_pair ((const void *) sc, m_edges.size ())).first;
      m_edges.push_back (edge_type (std::vector<Transition> (), std::make_pair (size_t (0), (const db::Net *) 0)));
    }

    m_edges [in->second].first.push_back (ed);

  }

  for (db::Net::const_terminal_iterator i = net->begin_terminals (); i != net->end_terminals (); ++i) {

    const db::Device *d = i->device ();
    if (! device_filter.filter (d)) {
      continue;
    }

    size_t device_cat = device_categorizer.cat_for_device (d);
    if (! device_cat) {
      //  device is ignored
      continue;
    }

    bool is_strict = device_categorizer.is_strict_device_category (device_cat);

    //  strict device checking means no terminal swapping
    size_t terminal1_id = is_strict ? i->terminal_id () : translate_terminal_id (i->terminal_id (), d);

    const std::vector<db::DeviceTerminalDefinition> &td = d->device_class ()->terminal_definitions ();
    for (std::vector<db::DeviceTerminalDefinition>::const_iterator it = td.begin (); it != td.end (); ++it) {

      if (it->id () != i->terminal_id ()) {

        size_t terminal2_id = is_strict ? it->id () : translate_terminal_id (it->id (), d);
        Transition ed2 (d, device_cat, terminal1_id, terminal2_id);

        const db::Net *net2 = d->net_for_terminal (it->id ());
        if (! net2) {
          continue;
        }

        std::map<const void *, size_t>::const_iterator in = n2entry.find ((const void *) net2);
        if (in == n2entry.end ()) {
          in = n2entry.insert (std::make_pair ((const void *) net2, m_edges.size ())).first;
          m_edges.push_back (edge_type (std::vector<Transition> (), std::make_pair (size_t (0), net2)));
        }

        m_edges [in->second].first.push_back (ed2);

      }

    }

  }
}

NetGraphNode::NetGraphNode (const db::SubCircuit *sc, CircuitCategorizer &circuit_categorizer, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinCategorizer *pin_map, size_t *unique_pin_id)
  : mp_net (0), m_other_net_index (invalid_id)
{
  std::map<const db::Net *, size_t> n2entry;

  size_t circuit_cat = circuit_categorizer.cat_for_subcircuit (sc);
  tl_assert (circuit_cat != 0);

  const db::Circuit *cr = sc->circuit_ref ();
  tl_assert (cr != 0);

  std::map<const db::Circuit *, CircuitMapper>::const_iterator icm = circuit_map->find (cr);
  tl_assert (icm != circuit_map->end ());

  const CircuitMapper *cm = & icm->second;

  for (db::Circuit::const_pin_iterator p = cr->begin_pins (); p != cr->end_pins (); ++p) {

    size_t pin_id = p->id ();
    const db::Net *net_at_pin = sc->net_for_pin (pin_id);
    if (! net_at_pin) {
      continue;
    }

    //  A pin assignment may be missing because there is no (real) net for a pin -> skip this pin

    size_t original_pin_id = pin_id;

    if (! cm->has_other_pin_for_this_pin (pin_id)) {

      //  isolated pins are ignored, others are considered for the matching
      if (! unique_pin_id || is_non_trivial_net (net_at_pin)) {
        continue;
      } else {
        pin_id = (*unique_pin_id)++;
      }

    } else {

      //  NOTE: if cm is given, cr and pin_id are given in terms of the canonical "other" circuit.
      //  For c1 this is the c1->c2 mapper, for c2 this is the c2->c2 dummy mapper.

      pin_id = cm->other_pin_from_this_pin (pin_id);

      //  realize pin swapping by normalization of pin ID

      pin_id = pin_map->normalize_pin_id (cm->other (), pin_id);

    }

    //  Make the other endpoint

    Transition ed (sc, circuit_cat, pin_id, original_pin_id);

    std::map<const db::Net *, size_t>::const_iterator in = n2entry.find (net_at_pin);
    if (in == n2entry.end ()) {
      in = n2entry.insert (std::make_pair ((const db::Net *) net_at_pin, m_edges.size ())).first;
      m_edges.push_back (edge_type (std::vector<Transition> (), std::make_pair (size_t (0), net_at_pin)));
    }

    m_edges [in->second].first.push_back (ed);

  }
}

void
NetGraphNode::expand_subcircuit_nodes (NetGraph *graph)
{
  std::map<const db::Net *, size_t> n2entry;

  std::list<edge_type> sc_edges;

  size_t ii = 0;
  for (size_t i = 0; i < m_edges.size (); ++i) {
    if (ii != i) {
      swap_edges (m_edges [ii], m_edges [i]);
    }
    if (m_edges [ii].second.second == 0) {
      //  subcircuit pin
      sc_edges.push_back (m_edges [ii]);
    } else {
      n2entry.insert (std::make_pair (m_edges [ii].second.second, ii));
      ++ii;
    }
  }

  m_edges.erase (m_edges.begin () + ii, m_edges.end ());

  for (std::list<edge_type>::const_iterator e = sc_edges.begin (); e != sc_edges.end (); ++e) {

    const db::SubCircuit *sc = 0;
    for (std::vector<Transition>::const_iterator t = e->first.begin (); t != e->first.end (); ++t) {
      tl_assert (t->is_for_subcircuit ());
      if (! sc) {
        sc = t->subcircuit ();
      } else {
        tl_assert (sc == t->subcircuit ());
      }
    }

    const NetGraphNode &dn = graph->virtual_node (sc);
    for (NetGraphNode::edge_iterator de = dn.begin (); de != dn.end (); ++de) {

      const db::Net *net_at_pin = de->second.second;
      if (net_at_pin == net ()) {
        continue;
      }

      std::map<const db::Net *, size_t>::const_iterator in = n2entry.find (net_at_pin);
      if (in == n2entry.end ()) {
        in = n2entry.insert (std::make_pair ((const db::Net *) net_at_pin, m_edges.size ())).first;
        m_edges.push_back (edge_type (std::vector<Transition> (), de->second));
      }

      m_edges [in->second].first.insert (m_edges [in->second].first.end (), de->first.begin (), de->first.end ());

    }

  }

  //  "deep sorting" of the edge descriptor
  for (std::vector<edge_type>::iterator i = m_edges.begin (); i != m_edges.end (); ++i) {
    std::sort (i->first.begin (), i->first.end ());
  }

  std::sort (m_edges.begin (), m_edges.end ());
}

std::string
NetGraphNode::to_string () const
{
  std::string res = std::string ("[");
  if (mp_net) {
    res += mp_net->expanded_name ();
  } else {
    res += "(null)";
  }
  res += "]";
  if (m_other_net_index != invalid_id) {
    res += " (other: #" + tl::to_string (m_other_net_index) + ")";
  }
  res += "\n";

  for (std::vector<edge_type>::const_iterator e = m_edges.begin (); e != m_edges.end (); ++e) {
    res += "  (\n";
    for (std::vector<Transition>::const_iterator i = e->first.begin (); i != e->first.end (); ++i) {
      res += std::string ("    ") + i->to_string () + "\n";
    }
    res += "  )->";
    if (! e->second.second) {
      res += "(null)";
    } else {
      res += e->second.second->expanded_name () + "[#" + tl::to_string (e->second.first) + "]";
    }
    res += "\n";
  }
  return res;
}

void
NetGraphNode::apply_net_index (const std::map<const db::Net *, size_t> &ni)
{
  for (std::vector<edge_type>::iterator i = m_edges.begin (); i != m_edges.end (); ++i) {
    std::map<const db::Net *, size_t>::const_iterator j = ni.find (i->second.second);
    tl_assert (j != ni.end ());
    i->second.first = j->second;
  }

  //  "deep sorting" of the edge descriptor
  for (std::vector<edge_type>::iterator i = m_edges.begin (); i != m_edges.end (); ++i) {
    std::sort (i->first.begin (), i->first.end ());
  }

  std::sort (m_edges.begin (), m_edges.end ());
}

bool
NetGraphNode::less (const NetGraphNode &node, bool with_name) const
{
  if (m_edges.size () != node.m_edges.size ()) {
    return m_edges.size () < node.m_edges.size ();
  }
  for (size_t i = 0; i < m_edges.size (); ++i) {
    if (m_edges [i].first != node.m_edges [i].first) {
      return m_edges [i].first < node.m_edges [i].first;
    }
  }
  if (m_edges.empty ()) {
    //  do a more detailed analysis on the nets involved
    return net_less (net (), node.net (), with_name);
  }
  return false;
}

bool
NetGraphNode::equal (const NetGraphNode &node, bool with_name) const
{
  if (m_edges.size () != node.m_edges.size ()) {
    return false;
  }
  for (size_t i = 0; i < m_edges.size (); ++i) {
    if (m_edges [i].first != node.m_edges [i].first) {
      return false;
    }
  }
  if (m_edges.empty ()) {
    //  do a more detailed analysis on the edges
    return net_equal (net (), node.net (), with_name);
  }
  return true;
}

bool
NetGraphNode::net_less (const db::Net *a, const db::Net *b, bool with_name)
{
  if ((a != 0) != (b != 0)) {
    return (a != 0) < (b != 0);
  }
  if (a == 0) {
    return false;
  }
  if (a->pin_count () != b->pin_count ()) {
    return a->pin_count () < b->pin_count ();
  }
  return with_name ? name_compare (a, b) < 0 : false;
}

bool
NetGraphNode::net_equal (const db::Net *a, const db::Net *b, bool with_name)
{
  if ((a != 0) != (b != 0)) {
    return false;
  }
  if (a == 0) {
    return true;
  }
  if (a->pin_count () != b->pin_count ()) {
    return false;
  }
  return with_name ? name_compare (a, b) == 0 : true;
}

// --------------------------------------------------------------------------------------------------------------------
//  NetGraph implementation

NetGraph::NetGraph ()
{
  //  .. nothing yet ..
}

void
NetGraph::build (const db::Circuit *c, DeviceCategorizer &device_categorizer, CircuitCategorizer &circuit_categorizer, const db::DeviceFilter &device_filter, const std::map<const db::Circuit *, CircuitMapper> *circuit_and_pin_mapping, const CircuitPinCategorizer *circuit_pin_mapper, size_t *unique_pin_id)
{
  tl::SelfTimer timer (tl::verbosity () >= 31, tl::to_string (tr ("Building net graph for circuit: ")) + c->name ());

  mp_circuit = c;

  m_nodes.clear ();
  m_net_index.clear ();

  //  create a dummy node for a null net
  m_nodes.push_back (NetGraphNode (0, device_categorizer, circuit_categorizer, device_filter, circuit_and_pin_mapping, circuit_pin_mapper, unique_pin_id));

  size_t nets = 0;
  for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
    ++nets;
  }
  m_nodes.reserve (nets);

  for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
    NetGraphNode node (n.operator-> (), device_categorizer, circuit_categorizer, device_filter, circuit_and_pin_mapping, circuit_pin_mapper, unique_pin_id);
    if (! node.empty () || n->pin_count () > 0) {
      m_nodes.push_back (node);
    }
  }

  for (std::vector<NetGraphNode>::const_iterator i = m_nodes.begin (); i != m_nodes.end (); ++i) {
    m_net_index.insert (std::make_pair (i->net (), i - m_nodes.begin ()));
  }

  for (std::vector<NetGraphNode>::iterator i = m_nodes.begin (); i != m_nodes.end (); ++i) {
    i->apply_net_index (m_net_index);
  }

  if (db::NetlistCompareGlobalOptions::options ()->debug_netgraph) {
    for (std::vector<NetGraphNode>::iterator i = m_nodes.begin (); i != m_nodes.end (); ++i) {
      tl::info << i->to_string () << tl::noendl;
    }
  }

  //  create subcircuit virtual nodes

  for (db::Circuit::const_subcircuit_iterator i = c->begin_subcircuits (); i != c->end_subcircuits (); ++i) {

    size_t circuit_cat = circuit_categorizer.cat_for_subcircuit (i.operator-> ());
    if (! circuit_cat) {
      continue;
    }

    const db::Circuit *cr = i->circuit_ref ();
    std::map<const db::Circuit *, CircuitMapper>::const_iterator icm = circuit_and_pin_mapping->find (cr);
    if (icm == circuit_and_pin_mapping->end ()) {
      continue;
    }

    m_virtual_nodes.insert (std::make_pair (i.operator-> (), NetGraphNode (i.operator-> (), circuit_categorizer, circuit_and_pin_mapping, circuit_pin_mapper, unique_pin_id)));

  }

  for (std::map<const db::SubCircuit *, NetGraphNode>::iterator i = m_virtual_nodes.begin (); i != m_virtual_nodes.end (); ++i) {
    i->second.apply_net_index (m_net_index);
  }

  if (db::NetlistCompareGlobalOptions::options ()->debug_netgraph) {
    for (std::map<const db::SubCircuit *, NetGraphNode>::iterator i = m_virtual_nodes.begin (); i != m_virtual_nodes.end (); ++i) {
      tl::info << i->second.to_string () << tl::noendl;
    }
  }
}

NetGraphNode
NetGraph::joined (const NetGraphNode &a, const NetGraphNode &b) const
{
  NetGraphNode nj = a;

  nj.edges ().clear ();
  nj.edges ().reserve ((a.end () - a.begin ()) + (b.end () - b.begin ()));

  std::map<const db::Net *, NetGraphNode::edge_type> joined;

  for (int m = 0; m < 2; ++m) {

    const NetGraphNode &n = (m == 0 ? a : b);

    for (auto i = n.begin (); i != n.end (); ++i) {

      if (i->second.second) {

        const db::Net *net = i->second.second == b.net () ? a.net () : i->second.second;

        auto j = joined.find (net);
        if (j != joined.end ()) {
          j->second.first.insert (j->second.first.end (), i->first.begin (), i->first.end ());
        } else {
          j = joined.insert (std::make_pair (net, *i)).first;
          j->second.second.second = net;
        }

      } else {
        nj.edges ().push_back (*i);
      }

    }

  }

  for (auto i = joined.begin (); i != joined.end (); ++i) {
    nj.edges ().push_back (i->second);
  }

  nj.apply_net_index (m_net_index);
  return nj;
}

}

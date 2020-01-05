
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "layNetlistCrossReferenceModel.h"

namespace lay
{

static void build_top_circuit_list (const db::NetlistCrossReference *cross_ref, std::vector<NetlistCrossReferenceModel::circuit_pair> &top_level_circuits)
{
  if (! top_level_circuits.empty ()) {
    return;
  }

  for (db::NetlistCrossReference::circuits_iterator c = cross_ref->begin_circuits(); c != cross_ref->end_circuits (); ++c) {
    const db::Circuit *cfirst = c->first;
    const db::Circuit *csecond = c->first;
    if ((! cfirst || cfirst->begin_refs () == cfirst->end_refs ()) && (! csecond || csecond->begin_refs () == csecond->end_refs ())) {
      top_level_circuits.push_back (*c);
    }
  }
}

static void build_child_circuit_list (const db::NetlistCrossReference *cross_ref, const NetlistCrossReferenceModel::circuit_pair &cp, std::vector<NetlistCrossReferenceModel::circuit_pair> &child_circuits)
{
  const db::NetlistCrossReference::PerCircuitData *data = cross_ref->per_circuit_data_for (cp);
  if (! data) {
    return;
  }

  if (data->status == db::NetlistCrossReference::Skipped) {

    //  For skipped circuits there is no subcircuit event list, so we have to create our own

    std::set<const db::Circuit *> seen;

    if (cp.first) {

      for (db::Circuit::const_subcircuit_iterator s = cp.first->begin_subcircuits (); s != cp.first->end_subcircuits (); ++s) {

        const db::Circuit *cr = s->circuit_ref ();
        if (seen.find (cr) == seen.end ()) {
          seen.insert (cr);
          const db::Circuit *cro = cross_ref->other_circuit_for (cr);
          NetlistCrossReferenceModel::circuit_pair cp (cr, cro);
          child_circuits.push_back (cp);
        }

      }

    }

    if (cp.second) {

      for (db::Circuit::const_subcircuit_iterator s = cp.second->begin_subcircuits (); s != cp.second->end_subcircuits (); ++s) {

        const db::Circuit *cr = s->circuit_ref ();
        if (seen.find (cr) == seen.end ()) {
          seen.insert (cr);
          const db::Circuit *cro = cross_ref->other_circuit_for (cr);
          if (! cro) {
            NetlistCrossReferenceModel::circuit_pair cp (cro, cr);
            child_circuits.push_back (cp);
          }
        }

      }

    }

  } else {

    std::set<NetlistCrossReferenceModel::circuit_pair> seen;
    for (db::NetlistCrossReference::PerCircuitData::subcircuit_pairs_const_iterator s = data->subcircuits.begin (); s != data->subcircuits.end (); ++s) {
      const db::Circuit *cfirst = s->pair.first ? s->pair.first->circuit_ref () : 0;
      const db::Circuit *csecond = s->pair.second ? s->pair.second->circuit_ref () : 0;
      NetlistCrossReferenceModel::circuit_pair cp (cfirst, csecond);
      if (seen.find (cp) == seen.end ()) {
        seen.insert (cp);
        child_circuits.push_back (cp);
      }
    }

  }
}

static void build_child_circuit_map (const db::NetlistCrossReference *cross_ref, std::map<NetlistCrossReferenceModel::circuit_pair, std::vector<NetlistCrossReferenceModel::circuit_pair> > &child_circuit_map)
{
  if (! child_circuit_map.empty ()) {
    return;
  }

  for (db::NetlistCrossReference::circuits_iterator c = cross_ref->begin_circuits(); c != cross_ref->end_circuits (); ++c) {
    build_child_circuit_list (cross_ref, *c, child_circuit_map [*c]);
  }
}

NetlistCrossReferenceModel::NetlistCrossReferenceModel (const db::NetlistCrossReference *cross_ref)
  : mp_cross_ref (const_cast<db::NetlistCrossReference *> (cross_ref))
{
  //  .. nothing yet ..
}

size_t NetlistCrossReferenceModel::circuit_count () const
{
  return mp_cross_ref.get () ? mp_cross_ref->circuit_count () : 0;
}

size_t NetlistCrossReferenceModel::top_circuit_count () const
{
  if (mp_cross_ref.get ()) {
    build_top_circuit_list (mp_cross_ref.get (), m_top_level_circuits);
    return m_top_level_circuits.size ();
  } else {
    return 0;
  }
}

size_t NetlistCrossReferenceModel::child_circuit_count (const circuit_pair &circuits) const
{
  build_child_circuit_map (mp_cross_ref.get (), m_child_circuits);
  return m_child_circuits [circuits].size ();
}

size_t NetlistCrossReferenceModel::net_count (const circuit_pair &circuits) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  return data ? data->nets.size () : 0;
}

size_t NetlistCrossReferenceModel::net_terminal_count (const net_pair &nets) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  return data ? data->terminals.size () : 0;
}

size_t NetlistCrossReferenceModel::net_subcircuit_pin_count (const net_pair &nets) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  return data ? data->subcircuit_pins.size () : 0;
}

size_t NetlistCrossReferenceModel::net_pin_count (const net_pair &nets) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  return data ? data->pins.size () : 0;
}

size_t NetlistCrossReferenceModel::device_count (const circuit_pair &circuits) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  return data ? data->devices.size () : 0;
}

size_t NetlistCrossReferenceModel::pin_count (const circuit_pair &circuits) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  return data ? data->pins.size () : 0;
}

size_t NetlistCrossReferenceModel::subcircuit_count (const circuit_pair &circuits) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  return data ? data->subcircuits.size () : 0;
}

namespace {

template <class Obj> struct DataGetter;

template <>
struct DataGetter<const db::Net *>
{
  typedef std::vector<db::NetlistCrossReference::NetPairData>::const_iterator iterator_type;
  iterator_type begin (const db::NetlistCrossReference::PerCircuitData &data) const { return data.nets.begin (); }
  iterator_type end (const db::NetlistCrossReference::PerCircuitData &data) const { return data.nets.end (); }
};

template <>
struct DataGetter<const db::Device *>
{
  typedef std::vector<db::NetlistCrossReference::DevicePairData>::const_iterator iterator_type;
  iterator_type begin (const db::NetlistCrossReference::PerCircuitData &data) const { return data.devices.begin (); }
  iterator_type end (const db::NetlistCrossReference::PerCircuitData &data) const { return data.devices.end (); }
};

template <>
struct DataGetter<const db::SubCircuit *>
{
  typedef std::vector<db::NetlistCrossReference::SubCircuitPairData>::const_iterator iterator_type;
  iterator_type begin (const db::NetlistCrossReference::PerCircuitData &data) const { return data.subcircuits.begin (); }
  iterator_type end (const db::NetlistCrossReference::PerCircuitData &data) const { return data.subcircuits.end (); }
};

}

template <class Pair>
static IndexedNetlistModel::circuit_pair get_parent_of (const Pair &pair, const db::NetlistCrossReference *cross_ref, std::map<Pair, IndexedNetlistModel::circuit_pair> &cache)
{
  typename std::map<Pair, IndexedNetlistModel::circuit_pair>::iterator i = cache.find (pair);
  if (i == cache.end ()) {

    for (db::NetlistCrossReference::circuits_iterator c = cross_ref->begin_circuits (); c != cross_ref->end_circuits (); ++c) {
      const db::NetlistCrossReference::PerCircuitData *data = cross_ref->per_circuit_data_for (*c);
      typedef DataGetter<typename Pair::first_type> getter_type;
      typedef typename getter_type::iterator_type iterator_type;
      iterator_type b = getter_type ().begin (*data);
      iterator_type e = getter_type ().end (*data);
      for (iterator_type j = b; j != e; ++j) {
        cache.insert (std::make_pair (j->pair, *c));
        if (j->pair.first) {
          cache.insert (std::make_pair (Pair (j->pair.first, (typename Pair::second_type) 0), *c));
        }
        if (j->pair.second) {
          cache.insert (std::make_pair (Pair ((typename Pair::first_type) 0, j->pair.second), *c));
        }
      }
    }

    i = cache.find (pair);

  }

  if (i == cache.end ()) {
    return IndexedNetlistModel::circuit_pair ((const db::Circuit *) 0, (const db::Circuit *) 0);
  } else {
    return i->second;
  }
}

IndexedNetlistModel::circuit_pair NetlistCrossReferenceModel::parent_of (const IndexedNetlistModel::net_pair &net_pair) const
{
  return get_parent_of (net_pair, mp_cross_ref.get (), m_parents_of_nets);
}

IndexedNetlistModel::circuit_pair NetlistCrossReferenceModel::parent_of (const IndexedNetlistModel::device_pair &device_pair) const
{
  return get_parent_of (device_pair, mp_cross_ref.get (), m_parents_of_devices);
}

IndexedNetlistModel::circuit_pair NetlistCrossReferenceModel::parent_of (const IndexedNetlistModel::subcircuit_pair &subcircuit_pair) const
{
  return get_parent_of (subcircuit_pair, mp_cross_ref.get (), m_parents_of_subcircuits);
}

std::pair<IndexedNetlistModel::circuit_pair, NetlistCrossReferenceModel::Status> NetlistCrossReferenceModel::top_circuit_from_index (size_t index) const
{
  build_top_circuit_list (mp_cross_ref.get (), m_top_level_circuits);

  IndexedNetlistModel::circuit_pair cp = m_top_level_circuits [index];
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (cp);
  tl_assert (data != 0);
  return std::make_pair (cp, data->status);
}

std::pair<IndexedNetlistModel::circuit_pair, NetlistCrossReferenceModel::Status> NetlistCrossReferenceModel::child_circuit_from_index (const circuit_pair &circuits, size_t index) const
{
  build_child_circuit_map (mp_cross_ref.get (), m_child_circuits);

  IndexedNetlistModel::circuit_pair cp = m_child_circuits [circuits][index];
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (cp);
  tl_assert (data != 0);
  return std::make_pair (cp, data->status);
}

std::pair<IndexedNetlistModel::circuit_pair, NetlistCrossReferenceModel::Status> NetlistCrossReferenceModel::circuit_from_index (size_t index) const
{
  IndexedNetlistModel::circuit_pair cp = mp_cross_ref->begin_circuits () [index];
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (cp);
  tl_assert (data != 0);
  return std::make_pair (cp, data->status);
}

std::pair<IndexedNetlistModel::net_pair, NetlistCrossReferenceModel::Status> NetlistCrossReferenceModel::net_from_index (const circuit_pair &circuits, size_t index) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (data != 0);
  return std::make_pair (data->nets [index].pair, data->nets [index].status);
}

const db::Net *NetlistCrossReferenceModel::second_net_for (const db::Net *first) const
{
  return mp_cross_ref->other_net_for (first);
}

const db::Circuit *NetlistCrossReferenceModel::second_circuit_for (const db::Circuit *first) const
{
  return mp_cross_ref->other_circuit_for (first);
}

IndexedNetlistModel::net_subcircuit_pin_pair NetlistCrossReferenceModel::net_subcircuit_pinref_from_index (const net_pair &nets, size_t index) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  tl_assert (data != 0);
  return data->subcircuit_pins [index];
}

IndexedNetlistModel::net_terminal_pair NetlistCrossReferenceModel::net_terminalref_from_index (const net_pair &nets, size_t index) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  tl_assert (data != 0);
  return data->terminals [index];
}

IndexedNetlistModel::net_pin_pair NetlistCrossReferenceModel::net_pinref_from_index (const net_pair &nets, size_t index) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  tl_assert (data != 0);
  return data->pins [index];
}

std::pair<IndexedNetlistModel::device_pair, NetlistCrossReferenceModel::Status> NetlistCrossReferenceModel::device_from_index (const circuit_pair &circuits, size_t index) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (data != 0);
  return std::make_pair (data->devices [index].pair, data->devices [index].status);
}

std::pair<IndexedNetlistModel::pin_pair, NetlistCrossReferenceModel::Status> NetlistCrossReferenceModel::pin_from_index (const circuit_pair &circuits, size_t index) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (data != 0);
  return std::make_pair (data->pins [index].pair, data->pins [index].status);
}

std::pair<IndexedNetlistModel::subcircuit_pair, NetlistCrossReferenceModel::Status> NetlistCrossReferenceModel::subcircuit_from_index (const circuit_pair &circuits, size_t index) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (data != 0);
  return std::make_pair (data->subcircuits [index].pair, data->subcircuits [index].status);
}

template <class Pair, class Iter>
static size_t get_index_of (const Pair &pair, Iter begin, Iter end, std::map<Pair, size_t> &cache)
{
  typename std::map<Pair, size_t>::iterator i = cache.find (pair);
  if (i == cache.end ()) {

    size_t index = 0;
    for (Iter j = begin; j != end; ++j, ++index) {
      cache.insert (std::make_pair (j->pair, index));
      if (j->pair.first) {
        cache.insert (std::make_pair (Pair (j->pair.first, (typename Pair::second_type)0), index));
      }
      if (j->pair.second) {
        cache.insert (std::make_pair (Pair ((typename Pair::first_type)0, j->pair.second), index));
      }
    }

    i = cache.find (pair);
    if (i == cache.end ()) {
      return lay::no_netlist_index;
    }

  }

  return i->second;
}


size_t NetlistCrossReferenceModel::circuit_index (const circuit_pair &circuits) const
{
  std::map<circuit_pair, size_t>::iterator i = m_index_of_circuits.find (circuits);
  if (i == m_index_of_circuits.end ()) {

    size_t index = 0;
    for (db::NetlistCrossReference::circuits_iterator j = mp_cross_ref->begin_circuits (); j != mp_cross_ref->end_circuits (); ++j, ++index) {
      m_index_of_circuits.insert (std::make_pair (*j, index));
      if (j->first) {
        m_index_of_circuits.insert (std::make_pair (circuit_pair (j->first, (const db::Circuit *)0), index));
      }
      if (j->second) {
        m_index_of_circuits.insert (std::make_pair (circuit_pair ((const db::Circuit *)0, j->second), index));
      }
    }

    i = m_index_of_circuits.find (circuits);
    if (i == m_index_of_circuits.end ()) {
      return lay::no_netlist_index;
    }

  }

  return i->second;
}

size_t NetlistCrossReferenceModel::net_index (const net_pair &nets) const
{
  circuit_pair circuits = parent_of (nets);

  const db::NetlistCrossReference::PerCircuitData *org_data = mp_cross_ref->per_circuit_data_for (circuits);
  if (! org_data) {
    return lay::no_netlist_index;
  }

  PerCircuitCacheData *data = & m_per_circuit_data [circuits];
  return get_index_of (nets, org_data->nets.begin (), org_data->nets.end (), data->index_of_nets);
}

size_t NetlistCrossReferenceModel::device_index (const device_pair &devices) const
{
  circuit_pair circuits = parent_of (devices);

  const db::NetlistCrossReference::PerCircuitData *org_data = mp_cross_ref->per_circuit_data_for (circuits);
  if (! org_data) {
    return lay::no_netlist_index;
  }

  PerCircuitCacheData *data = & m_per_circuit_data [circuits];
  return get_index_of (devices, org_data->devices.begin (), org_data->devices.end (), data->index_of_devices);
}

size_t NetlistCrossReferenceModel::pin_index (const pin_pair &pins, const circuit_pair &circuits) const
{
  const db::NetlistCrossReference::PerCircuitData *org_data = mp_cross_ref->per_circuit_data_for (circuits);
  if (! org_data) {
    return lay::no_netlist_index;
  }

  PerCircuitCacheData *data = & m_per_circuit_data [circuits];
  return get_index_of (pins, org_data->pins.begin (), org_data->pins.end (), data->index_of_pins);
}

size_t NetlistCrossReferenceModel::subcircuit_index (const subcircuit_pair &subcircuits) const
{
  circuit_pair circuits = parent_of (subcircuits);

  const db::NetlistCrossReference::PerCircuitData *org_data = mp_cross_ref->per_circuit_data_for (circuits);
  if (! org_data) {
    return lay::no_netlist_index;
  }

  PerCircuitCacheData *data = & m_per_circuit_data [circuits];
  return get_index_of (subcircuits, org_data->subcircuits.begin (), org_data->subcircuits.end (), data->index_of_subcircuits);
}

std::string NetlistCrossReferenceModel::circuit_pair_status_hint (const std::pair<IndexedNetlistModel::circuit_pair, NetlistCrossReferenceModel::Status> &cps) const
{
  if (cps.second == db::NetlistCrossReference::Mismatch || cps.second == db::NetlistCrossReference::NoMatch) {
    if (! cps.first.first || ! cps.first.second) {
      return tl::to_string (tr ("No matching circuit found in the other netlist.\n"
                                "By default, circuits are identified by their name.\n"
                                "A missing circuit probably means there is no circuit in the other netlist with this name.\n"
                                "If circuits with different names need to be associated, use 'same_circuits' in the\n"
                                "LVS script to establish such an association."));
    } else {
      return tl::to_string (tr ("Circuits could be paired, but there is a mismatch inside.\n"
                                "Browse the circuit's component list to identify the mismatching elements."));
    }
  } else if (cps.second == db::NetlistCrossReference::Skipped) {
      return tl::to_string (tr ("Circuits can only be matched if their child circuits have a known counterpart and a\n"
                                "pin-to-pin correspondence could be established for each child circuit.\n"
                                "This is not the case here. Browse the child circuits to identify the blockers.\n"
                                "Potential blockers are subcircuits without a corresponding other circuit or circuits\n"
                                "where some pins could not be mapped to pins from the corresponding other circuit."));
  }
  return std::string ();
}

std::string NetlistCrossReferenceModel::top_circuit_status_hint (size_t index) const
{
  return circuit_pair_status_hint (top_circuit_from_index (index));
}

std::string NetlistCrossReferenceModel::circuit_status_hint (size_t index) const
{
  return circuit_pair_status_hint (circuit_from_index (index));
}

std::string NetlistCrossReferenceModel::child_circuit_status_hint (const circuit_pair &circuits, size_t index) const
{
  std::pair<IndexedNetlistModel::circuit_pair, NetlistCrossReferenceModel::Status> cps = child_circuit_from_index (circuits, index);
  if (cps.second == db::NetlistCrossReference::Mismatch || cps.second == db::NetlistCrossReference::NoMatch) {
    if (!cps.first.first || !cps.first.second) {
      return tl::to_string (tr ("No matching subcircuit was found in the other netlist - this is likely because pin\n"
                                "assignment could not be derived from the nets connected to the pins.\n"
                                "Check, if the pins are attached properly. If pins need to be swappable, consider using\n"
                                "'equivalent_pins' in the LVS script."));
    } else {
      return tl::to_string (tr ("Two different subcircuits fit here in the same way, but they are not\n"
                                "originating from equivalent circuits.\n"
                                "If the circuits behind the subcircuits are identical, using 'same_circuits'\n"
                                "in the LVS script will associate them."));
    }
  }
  return std::string ();
}

std::string NetlistCrossReferenceModel::net_status_hint (const circuit_pair &circuits, size_t index) const
{
  std::pair<IndexedNetlistModel::net_pair, NetlistCrossReferenceModel::Status> cps = net_from_index (circuits, index);
  if (cps.second == db::NetlistCrossReference::Mismatch || cps.second == db::NetlistCrossReference::NoMatch) {
    return tl::to_string (tr ("Nets don't match. Nets match, if connected subcircuit pins and device terminals match to a\n"
                              "counterpart in the other netlist (component-wise and pin/terminal-wise).\n"
                              "If there already is a net candidate from the other netlist, scan the net members for\n"
                              "mismatching items (with errors or warnings) and fix these issues.\n"
                              "Otherwise, look for the corresponding other net.\n"
                              "Net items not found in the reference netlist indicate additional connections.\n"
                              "Net items only found in the reference netlist indicate missing connections."));
  } else if (cps.second == db::NetlistCrossReference::MatchWithWarning) {
    return tl::to_string (tr ("Nets match, but the choice was ambiguous. This may lead to mismatching nets in other places.\n"));
  }
  return std::string ();
}

std::string NetlistCrossReferenceModel::device_status_hint (const circuit_pair &circuits, size_t index) const
{
  std::pair<IndexedNetlistModel::device_pair, NetlistCrossReferenceModel::Status> cps = device_from_index (circuits, index);
  if (cps.second == db::NetlistCrossReference::Mismatch || cps.second == db::NetlistCrossReference::NoMatch) {
    if (!cps.first.first || !cps.first.second) {
      return tl::to_string (tr ("No matching device was found in the other netlist.\n"
                                "Devices are identified by the nets they are attached to. Unmatched devices mean that\n"
                                "at least one terminal net isn't matched with a corresponding net from the other netlist.\n"
                                "Make all terminal nets match and the devices will match too."));
    } else {
      return tl::to_string (tr ("Devices don't match topologically.\n"
                                "Check the terminal connections to identify the terminals not being connected to\n"
                                "corresponding nets. Either the devices are not connected correctly or the nets\n"
                                "need to be fixed before the devices will match too."));
    }
  } else if (cps.second == db::NetlistCrossReference::MatchWithWarning) {
    return tl::to_string (tr ("Topologically matching devices are found here but either the parameters or the\n"
                              "device classes don't match.\n"
                              "If the device class is different but should be considered the same, using\n"
                              "'same_device_classed' in the LVS script will solve this issue."));
  }
  return std::string ();
}

std::string NetlistCrossReferenceModel::pin_status_hint (const circuit_pair &circuits, size_t index) const
{
  std::pair<IndexedNetlistModel::pin_pair, NetlistCrossReferenceModel::Status> cps = pin_from_index (circuits, index);
  if (cps.second == db::NetlistCrossReference::Mismatch || cps.second == db::NetlistCrossReference::NoMatch) {
    if (!cps.first.first || !cps.first.second) {
      return tl::to_string (tr ("No matching pin was found in the other netlist.\n"
                                "Pins are identified by the nets they are attached to - pins on equivalent nets are also\n"
                                "equivalent. Making the nets match will make the pins match too."));
    }
  }
  return std::string ();
}

std::string NetlistCrossReferenceModel::subcircuit_status_hint (const circuit_pair &circuits, size_t index) const
{
  std::pair<IndexedNetlistModel::subcircuit_pair, NetlistCrossReferenceModel::Status> cps = subcircuit_from_index (circuits, index);
  if (cps.second == db::NetlistCrossReference::Mismatch || cps.second == db::NetlistCrossReference::NoMatch) {
    if (!cps.first.first || !cps.first.second) {
      return tl::to_string (tr ("No matching subcircuit was found in the other netlist - this is likely because pin assignment\n"
                                "could not be derived from the nets connected to the pins.\n"
                                "Check, if the pins are attached properly. If pins need to be swappable, consider using\n"
                                "'equivalent_pins' in the LVS script."));
    } else {
      return tl::to_string (tr ("Two different subcircuits fit here in the same way, but they are not originating from\n"
                                "equivalent circuits.\n"
                                "If the circuits behind the subcircuits are identical, using 'same_circuits' in the LVS script\n"
                                "will associate them."));
    }
  }
  return std::string ();
}

}

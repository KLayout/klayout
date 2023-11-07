
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


#include "dbNetlistCrossReference.h"

namespace db
{

NetlistCrossReference::NetlistCrossReference ()
  : mp_per_circuit_data (0)
{
  //  .. nothing yet ..
}

NetlistCrossReference::~NetlistCrossReference ()
{
  //  .. nothing yet ..
}

const NetlistCrossReference::PerCircuitData *
NetlistCrossReference::per_circuit_data_for (const std::pair<const db::Circuit *, const db::Circuit *> &circuits) const
{
  std::map<const db::Circuit *, PerCircuitData *>::const_iterator i;
  if (circuits.first) {
    i = m_data_refs.find (circuits.first);
    if (i != m_data_refs.end ()) {
      return i->second;
    }
  }
  if (circuits.second) {
    i = m_data_refs.find (circuits.second);
    if (i != m_data_refs.end ()) {
      return i->second;
    }
  }
  return 0;
}

const db::Pin *
NetlistCrossReference::other_pin_for (const db::Pin *pin) const
{
  std::map<const db::Pin *, const db::Pin *>::const_iterator i = m_other_pin.find (pin);
  if (i != m_other_pin.end ()) {
    return i->second;
  } else {
    return 0;
  }
}

const db::Device *
NetlistCrossReference::other_device_for (const db::Device *device) const
{
  std::map<const db::Device *, const db::Device *>::const_iterator i = m_other_device.find (device);
  if (i != m_other_device.end ()) {
    return i->second;
  } else {
    return 0;
  }
}

const db::SubCircuit *
NetlistCrossReference::other_subcircuit_for (const db::SubCircuit *subcircuit) const
{
  std::map<const db::SubCircuit *, const db::SubCircuit *>::const_iterator i = m_other_subcircuit.find (subcircuit);
  if (i != m_other_subcircuit.end ()) {
    return i->second;
  } else {
    return 0;
  }
}

const db::Circuit *
NetlistCrossReference::other_circuit_for (const db::Circuit *circuit) const
{
  std::map<const db::Circuit *, const db::Circuit *>::const_iterator i = m_other_circuit.find (circuit);
  if (i != m_other_circuit.end ()) {
    return i->second;
  } else {
    return 0;
  }
}

const db::Net *
NetlistCrossReference::other_net_for (const db::Net *net) const
{
  std::map<const db::Net *, const db::Net *>::const_iterator i = m_other_net.find (net);
  if (i != m_other_net.end ()) {
    return i->second;
  } else {
    return 0;
  }
}

const NetlistCrossReference::PerNetData *
NetlistCrossReference::per_net_data_for (const std::pair<const db::Net *, const db::Net *> &nets) const
{
  if (! nets.first && ! nets.second) {
    return 0;
  }

  std::map<std::pair<const db::Net *, const db::Net *>, PerNetData>::iterator i = m_per_net_data.find (nets);
  if (i == m_per_net_data.end ()) {
    i = m_per_net_data.insert (std::make_pair (nets, PerNetData ())).first;
    build_per_net_info (nets, i->second);
  }

  return &i->second;
}

void
NetlistCrossReference::clear ()
{
  mp_netlist_a.reset (0);
  mp_netlist_b.reset (0);
  m_circuits.clear ();
  m_per_circuit_data.clear ();
  m_data_refs.clear ();
  m_per_net_data.clear ();
  m_other_circuit.clear ();
  m_other_net.clear ();
  m_other_device.clear ();
  m_other_pin.clear ();
  m_other_subcircuit.clear ();
  m_current_circuits.first = 0;
  m_current_circuits.second = 0;
  mp_per_circuit_data = 0;
}

void
NetlistCrossReference::gen_begin_netlist (const db::Netlist *a, const db::Netlist *b)
{
  mp_netlist_a.reset (const_cast <db::Netlist *> (a));
  mp_netlist_b.reset (const_cast <db::Netlist *> (b));
  m_current_circuits = std::make_pair ((const db::Circuit *)0, (const db::Circuit *)0);
}

namespace {

static int string_value_compare (const std::string &a, const std::string &b)
{
  return a == b ? 0 : (a < b ? -1 : 1);
}

template <class Obj>
struct by_name_value_compare
{
  int operator() (const Obj &a, const Obj &b) const
  {
    return string_value_compare (a.name (), b.name ());
  }
};

template <class Obj>
struct by_expanded_name_value_compare
{
  int operator() (const Obj &a, const Obj &b) const
  {
    return string_value_compare (a.expanded_name (), b.expanded_name ());
  }
};

struct ByDeviceClassNameCompare
{
  int operator() (const db::Device &a, const db::Device &b) const
  {
    if ((a.device_class () == 0) != (b.device_class () == 0)) {
      return a.device_class () == 0 ? -1 : 1;
    }
    if (a.device_class () == 0) {
      return 0;
    } else {
      return string_value_compare (a.device_class ()->name (), b.device_class ()->name ());
    }
  }
};

struct ByRefCircuitNameCompare
{
  int operator() (const db::SubCircuit &a, const db::SubCircuit &b) const
  {
    if ((a.circuit_ref () == 0) != (b.circuit_ref () == 0)) {
      return a.circuit_ref () == 0 ? -1 : 1;
    }
    if (a.circuit_ref () == 0) {
      return 0;
    } else {
      return string_value_compare (a.circuit_ref ()->name (), b.circuit_ref ()->name ());
    }
  }
};

template <class Obj>
struct net_object_compare;

template <>
struct net_object_compare<db::NetTerminalRef>
{
  int operator() (const db::NetTerminalRef &a, const db::NetTerminalRef &b) const
  {
    int ct = by_expanded_name_value_compare<db::Device> () (*a.device (), *b.device ());
    if (ct == 0) {
      return (a.terminal_id () != b.terminal_id () ? (a.terminal_id () < b.terminal_id () ? -1 : 1) : 0);
    } else {
      return ct;
    }
  }
};

template <>
struct net_object_compare<db::NetSubcircuitPinRef>
{
  int operator() (const db::NetSubcircuitPinRef &a, const db::NetSubcircuitPinRef &b) const
  {
    int ct = by_expanded_name_value_compare<db::SubCircuit> () (*a.subcircuit (), *b.subcircuit ());
    if (ct == 0) {
      return by_expanded_name_value_compare<db::Pin> () (*a.pin (), *b.pin ());
    } else {
      return ct;
    }
  }
};

template <>
struct net_object_compare<db::NetPinRef>
{
  int operator() (const db::NetPinRef &a, const db::NetPinRef &b) const
  {
    return by_expanded_name_value_compare<db::Pin> () (*a.pin (), *b.pin ());
  }
};

template <class Obj, class ValueCompare>
struct two_pointer_compare
{
  int operator() (const Obj *a, const Obj *b) const
  {
    if ((a == 0) != (b == 0)) {
      return (a == 0) > (b == 0) ? -1 : 1;
    }
    if (a != 0) {
      return ValueCompare () (*a, *b);
    } else {
      return 0;
    }
  }
};

template <class Obj, class ValueCompare>
struct two_pair_compare
{
  bool operator() (const std::pair<const Obj *, const Obj *> &a, const std::pair<const Obj *, const Obj *> &b)
  {
    int ct = two_pointer_compare<Obj, ValueCompare> () (a.first, b.first);
    if (ct != 0) {
      return ct < 0;
    }
    return two_pointer_compare<Obj, ValueCompare> () (a.second, b.second) < 0;
  }
};

template <class PairData, class ValueCompare>
struct pair_data_compare
{
  bool operator () (const PairData &a, const PairData &b) const
  {
    return two_pair_compare<typename PairData::object_type, ValueCompare> () (a.pair, b.pair);
  }
};

struct CircuitsCompareByName
  : public two_pair_compare<db::Circuit, by_name_value_compare<db::Circuit> >
{
  //  .. nothing yet ..
};

struct SortNetTerminals
  : public two_pair_compare<db::NetTerminalRef, net_object_compare<db::NetTerminalRef> >
{
  //  .. nothing yet ..
};

struct SortNetPins
  : public two_pair_compare<db::NetPinRef, net_object_compare<db::NetPinRef> >
{
  //  .. nothing yet ..
};

struct SortNetSubCircuitPins
  : public two_pair_compare<db::NetSubcircuitPinRef, net_object_compare<db::NetSubcircuitPinRef> >
{
  //  .. nothing yet ..
};

}

void
NetlistCrossReference::sort_netlist ()
{
  std::sort (m_circuits.begin (), m_circuits.end (), CircuitsCompareByName ());
}

void
NetlistCrossReference::gen_end_netlist (const db::Netlist *, const db::Netlist *)
{
  //  .. nothing yet ..
}

void
NetlistCrossReference::establish_pair (const db::Circuit *a, const db::Circuit *b)
{
  m_circuits.push_back (std::make_pair (a, b));
  m_per_circuit_data.push_back (PerCircuitData ());
  mp_per_circuit_data = & m_per_circuit_data.back ();
  m_data_refs [a] = mp_per_circuit_data;
  m_data_refs [b] = mp_per_circuit_data;

  if (a) {
    m_other_circuit [a] = b;
  }
  if (b) {
    m_other_circuit [b] = a;
  }
}

void
NetlistCrossReference::establish_pair (const db::Net *a, const db::Net *b, Status status, const std::string &msg)
{
  mp_per_circuit_data->nets.push_back (NetPairData (a, b, status, msg));
  if (a) {
    m_other_net [a] = b;
  }
  if (b) {
    m_other_net [b] = a;
  }
}

void
NetlistCrossReference::establish_pair (const db::Device *a, const db::Device *b, Status status, const std::string &msg)
{
  mp_per_circuit_data->devices.push_back (DevicePairData (a, b, status, msg));
  if (a) {
    m_other_device [a] = b;
  }
  if (b) {
    m_other_device [b] = a;
  }
}

void
NetlistCrossReference::establish_pair (const db::Pin *a, const db::Pin *b, Status status, const std::string &msg)
{
  mp_per_circuit_data->pins.push_back (PinPairData (a, b, status, msg));
  if (a) {
    m_other_pin [a] = b;
  }
  if (b) {
    m_other_pin [b] = a;
  }
}

void
NetlistCrossReference::establish_pair (const db::SubCircuit *a, const db::SubCircuit *b, Status status, const std::string &msg)
{
  mp_per_circuit_data->subcircuits.push_back (SubCircuitPairData (a, b, status, msg));
  if (a) {
    m_other_subcircuit [a] = b;
  }
  if (b) {
    m_other_subcircuit [b] = a;
  }
}

void
NetlistCrossReference::gen_begin_circuit (const db::Circuit *a, const db::Circuit *b)
{
  m_current_circuits = std::pair<const db::Circuit *, const db::Circuit *> (a, b);
  establish_pair (a, b);
}

void
NetlistCrossReference::sort_circuit ()
{
  std::stable_sort (mp_per_circuit_data->devices.begin (), mp_per_circuit_data->devices.end (), pair_data_compare<DevicePairData, ByDeviceClassNameCompare> ());
  std::stable_sort (mp_per_circuit_data->subcircuits.begin (), mp_per_circuit_data->subcircuits.end (), pair_data_compare<SubCircuitPairData, ByRefCircuitNameCompare> ());

  std::stable_sort (mp_per_circuit_data->pins.begin (), mp_per_circuit_data->pins.end (), pair_data_compare<PinPairData, by_name_value_compare<db::Pin> > ());
  std::stable_sort (mp_per_circuit_data->nets.begin (), mp_per_circuit_data->nets.end (), pair_data_compare<NetPairData, by_name_value_compare<db::Net> > ());
}

void
NetlistCrossReference::gen_end_circuit (const db::Circuit *, const db::Circuit *, Status status, const std::string &msg)
{
  mp_per_circuit_data->status = status;
  mp_per_circuit_data->msg = msg;

  m_current_circuits = std::make_pair((const db::Circuit *)0, (const db::Circuit *)0);
  mp_per_circuit_data = 0;
}

void
NetlistCrossReference::gen_log_entry (Severity severity, const std::string &msg)
{
  if (mp_per_circuit_data) {
    mp_per_circuit_data->log_entries.push_back (LogEntryData (severity, msg));
  } else {
    m_other_log_entries.push_back (LogEntryData (severity, msg));
  }
}

void
NetlistCrossReference::gen_nets (const db::Net *a, const db::Net *b, Status status, const std::string &msg)
{
  establish_pair (a, b, status, msg);
}

void
NetlistCrossReference::gen_devices (const db::Device *a, const db::Device *b, Status status, const std::string &msg)
{
  establish_pair (a, b, status, msg);
}

void
NetlistCrossReference::gen_pins (const db::Pin *a, const db::Pin *b, Status status, const std::string &msg)
{
  establish_pair (a, b, status, msg);
}

void
NetlistCrossReference::gen_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b, Status status, const std::string &msg)
{
  establish_pair (a, b, status, msg);
}

static void init_data_from_single (const db::Net *net, NetlistCrossReference::PerNetData &data, bool first)
{
  data.pins.reserve (net->pin_count ());
  for (db::Net::const_pin_iterator i = net->begin_pins (); i != net->end_pins (); ++i) {
    if (! first) {
      data.pins.push_back (std::make_pair ((const db::NetPinRef *) 0, i.operator-> ()));
    } else {
      data.pins.push_back (std::make_pair (i.operator-> (), (const db::NetPinRef *) 0));
    }
  }

  data.subcircuit_pins.reserve (net->subcircuit_pin_count ());
  for (db::Net::const_subcircuit_pin_iterator i = net->begin_subcircuit_pins (); i != net->end_subcircuit_pins (); ++i) {
    if (! first) {
      data.subcircuit_pins.push_back (std::make_pair ((const db::NetSubcircuitPinRef *) 0, i.operator-> ()));
    } else {
      data.subcircuit_pins.push_back (std::make_pair (i.operator-> (), (const db::NetSubcircuitPinRef *) 0));
    }
  }

  data.terminals.reserve (net->terminal_count ());
  for (db::Net::const_terminal_iterator i = net->begin_terminals (); i != net->end_terminals (); ++i) {
    if (! first) {
      data.terminals.push_back (std::make_pair ((const db::NetTerminalRef *) 0, i.operator-> ()));
    } else {
      data.terminals.push_back (std::make_pair (i.operator-> (), (const db::NetTerminalRef *) 0));
    }
  }
}

void
NetlistCrossReference::build_terminal_refs (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const
{
  std::map<std::pair<const db::Device *, size_t>, const db::NetTerminalRef *> d2t_a, d2t_b;

  for (db::Net::const_terminal_iterator i = nets.first->begin_terminals (); i != nets.first->end_terminals (); ++i) {
    d2t_a.insert (std::make_pair (std::make_pair (i->device (), i->terminal_id ()), i.operator-> ()));
  }

  for (db::Net::const_terminal_iterator i = nets.second->begin_terminals (); i != nets.second->end_terminals (); ++i) {
    d2t_b.insert (std::make_pair (std::make_pair (i->device (), i->terminal_id ()), i.operator-> ()));
  }

  for (std::map<std::pair<const db::Device *, size_t>, const db::NetTerminalRef *>::const_iterator a = d2t_a.begin (); a != d2t_a.end (); ++a) {

    const db::Device *da = a->first.first;

    const db::NetTerminalRef *pb = 0;

    std::map<const db::Device *, const db::Device *>::const_iterator idb = m_other_device.find (da);
    if (idb != m_other_device.end () && idb->second) {

      const db::Device *db = idb->second;

      //  we have a device pair - now we need to match the terminals: we do so on the basis
      //  of normalized terminal ID's

      size_t atid = da->device_class ()->normalize_terminal_id (a->first.second);
      const std::vector<db::DeviceTerminalDefinition> &termdefs_b = db->device_class ()->terminal_definitions ();

      for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = termdefs_b.begin (); t != termdefs_b.end (); ++t) {
        if (atid == db->device_class ()->normalize_terminal_id (t->id ())) {
          std::map<std::pair<const db::Device *, size_t>, const db::NetTerminalRef *>::iterator b = d2t_b.find (std::make_pair (db, t->id ()));
          if (b != d2t_b.end ()) {
            pb = b->second;
            //  remove the entry so we won't find it again
            d2t_b.erase (b);
            break;
          }
        }
      }

    }

    data.terminals.push_back (std::make_pair (a->second, pb));
  }

  for (std::map<std::pair<const db::Device *, size_t>, const db::NetTerminalRef *>::const_iterator b = d2t_b.begin (); b != d2t_b.end (); ++b) {
    data.terminals.push_back (std::make_pair ((const db::NetTerminalRef *) 0, b->second));
  }

  std::stable_sort (data.terminals.begin (), data.terminals.end (), SortNetTerminals ());
}

void
NetlistCrossReference::build_pin_refs (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const
{
  std::map<const db::Pin *, const db::NetPinRef *> p2r_a, p2r_b;

  for (db::Net::const_pin_iterator i = nets.first->begin_pins (); i != nets.first->end_pins (); ++i) {
    p2r_a.insert (std::make_pair (i->pin (), i.operator-> ()));
  }

  for (db::Net::const_pin_iterator i = nets.second->begin_pins (); i != nets.second->end_pins (); ++i) {
    p2r_b.insert (std::make_pair (i->pin (), i.operator-> ()));
  }

  for (std::map<const Pin *, const db::NetPinRef *>::const_iterator a = p2r_a.begin (); a != p2r_a.end (); ++a) {

    const db::Pin *pa = a->first;

    const db::NetPinRef *prb = 0;

    std::map<const db::Pin *, const db::Pin *>::const_iterator ipb = m_other_pin.find (pa);
    if (ipb != m_other_pin.end () && ipb->second) {

      std::map<const Pin *, const db::NetPinRef *>::iterator b = p2r_b.find (ipb->second);
      if (b != p2r_b.end ()) {
        prb = b->second;
        //  remove the entry so we won't find it again
        p2r_b.erase (b);
      }

    }

    data.pins.push_back (std::make_pair (a->second, prb));

  }

  for (std::map<const Pin *, const db::NetPinRef *>::const_iterator b = p2r_b.begin (); b != p2r_b.end (); ++b) {
    data.pins.push_back (std::make_pair ((const db::NetPinRef *) 0, b->second));
  }

  std::stable_sort (data.pins.begin (), data.pins.end (), SortNetPins ());
}

void
NetlistCrossReference::build_subcircuit_pin_refs (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const
{
  std::map<std::pair<const db::SubCircuit *, size_t>, const db::NetSubcircuitPinRef *> s2t_a, s2t_b;

  for (db::Net::const_subcircuit_pin_iterator i = nets.first->begin_subcircuit_pins (); i != nets.first->end_subcircuit_pins (); ++i) {
    s2t_a.insert (std::make_pair (std::make_pair (i->subcircuit (), i->pin_id ()), i.operator-> ()));
  }

  for (db::Net::const_subcircuit_pin_iterator i = nets.second->begin_subcircuit_pins (); i != nets.second->end_subcircuit_pins (); ++i) {
    s2t_b.insert (std::make_pair (std::make_pair (i->subcircuit (), i->pin_id ()), i.operator-> ()));
  }

  for (std::map<std::pair<const db::SubCircuit *, size_t>, const db::NetSubcircuitPinRef *>::const_iterator a = s2t_a.begin (); a != s2t_a.end (); ++a) {

    const db::SubCircuit *sa = a->first.first;

    const db::NetSubcircuitPinRef *pb = 0;

    std::map<const db::SubCircuit *, const db::SubCircuit *>::const_iterator isb = m_other_subcircuit.find (sa);
    if (isb != m_other_subcircuit.end () && isb->second) {

      const db::SubCircuit *sb = isb->second;

      //  we have a subcircuit pair - now we need to match the pins: we do so on the basis
      //  pin matching

      const db::Pin *pa = sa->circuit_ref ()->pin_by_id (a->first.second);
      std::map<const db::Pin *, const db::Pin *>::const_iterator ipb = m_other_pin.find (pa);
      if (ipb != m_other_pin.end () && ipb->second) {

        std::map<std::pair<const db::SubCircuit *, size_t>, const db::NetSubcircuitPinRef *>::iterator b = s2t_b.find (std::make_pair (sb, ipb->second->id ()));
        if (b != s2t_b.end ()) {
          pb = b->second;
          //  remove the entry so we won't find it again
          s2t_b.erase (b);
        }

      }

      //  Fallback for swappable pins: match based on the subcircuit alone
      if (! pb) {
        std::map<std::pair<const db::SubCircuit *, size_t>, const db::NetSubcircuitPinRef *>::iterator b = s2t_b.lower_bound (std::make_pair (sb, 0));
        if (b != s2t_b.end () && b->first.first == sb) {
          pb = b->second;
          //  remove the entry so we won't find it again
          s2t_b.erase (b);
        }
      }

    }

    data.subcircuit_pins.push_back (std::make_pair (a->second, pb));

  }

  for (std::map<std::pair<const db::SubCircuit *, size_t>, const db::NetSubcircuitPinRef *>::const_iterator b = s2t_b.begin (); b != s2t_b.end (); ++b) {
    data.subcircuit_pins.push_back (std::make_pair ((const db::NetSubcircuitPinRef *) 0, b->second));
  }

  std::stable_sort (data.subcircuit_pins.begin (), data.subcircuit_pins.end (), SortNetSubCircuitPins ());
}

void
NetlistCrossReference::build_per_net_info (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const
{
  if (! nets.first && ! nets.second) {
    //  .. nothing ..
  } else if (! nets.second) {
    init_data_from_single (nets.first, data, true);
  } else if (! nets.first) {
    init_data_from_single (nets.second, data, false);
  } else {
    build_terminal_refs (nets, data);
    build_pin_refs (nets, data);
    build_subcircuit_pin_refs (nets, data);
  }
}

}

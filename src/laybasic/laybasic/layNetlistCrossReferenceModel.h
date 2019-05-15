
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


#ifndef HDR_layNetlistCrossReference
#define HDR_layNetlistCrossReference

#include "laybasicCommon.h"
#include "layIndexedNetlistModel.h"

#include "tlObject.h"
#include "dbNetlistCompare.h" // @@@

// @@@
namespace db
{

class NetlistCrossReference
  : public db::NetlistCompareLogger, public tl::Object
{
public:
  NetlistCrossReference ();

  enum status {
    None = 0,
    Match,              //  objects are paired and match
    NoMatch,            //  objects are paired, but don't match
    Skipped,            //  objects are skipped
    MatchWithWarning,   //  objects are paired and match, but with warning (i.e. ambiguous nets)
    Mismatch            //  objects are not paired
  };

  struct PerCircuitData
  {
    std::vector<std::pair<const db::Net *, const db::Net *> > nets;
    std::vector<std::pair<const db::Device *, const db::Device *> > devices;
    std::vector<std::pair<const db::Pin *, const db::Pin *> > pins;
    std::vector<std::pair<const db::SubCircuit *, const db::SubCircuit *> > subcircuits;
  };

  struct PerNetData
  {
    std::vector<std::pair<const db::NetTerminalRef *, const db::NetTerminalRef *> > terminals;
    std::vector<std::pair<const db::NetPinRef *, const db::NetPinRef *> > pins;
    std::vector<std::pair<const db::NetSubcircuitPinRef *, const db::NetSubcircuitPinRef *> > subcircuit_pins;
  };

  virtual void begin_netlist (const db::Netlist *a, const db::Netlist *b);
  virtual void end_netlist (const db::Netlist *a, const db::Netlist *b);
  virtual void device_class_mismatch (const db::DeviceClass *a, const db::DeviceClass *b);
  virtual void begin_circuit (const db::Circuit *a, const db::Circuit *b);
  virtual void end_circuit (const db::Circuit *a, const db::Circuit *b, bool matching);
  virtual void circuit_skipped (const db::Circuit *a, const db::Circuit *b);
  virtual void circuit_mismatch (const db::Circuit *a, const db::Circuit *b);
  virtual void match_nets (const db::Net *a, const db::Net *b);
  virtual void match_ambiguous_nets (const db::Net *a, const db::Net *b);
  virtual void net_mismatch (const db::Net *a, const db::Net *b);
  virtual void match_devices (const db::Device *a, const db::Device *b);
  virtual void match_devices_with_different_parameters (const db::Device *a, const db::Device *b);
  virtual void match_devices_with_different_device_classes (const db::Device *a, const db::Device *b);
  virtual void device_mismatch (const db::Device *a, const db::Device *b);
  virtual void match_pins (const db::Pin *a, const db::Pin *b);
  virtual void pin_mismatch (const db::Pin *a, const db::Pin *b);
  virtual void match_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b);
  virtual void subcircuit_mismatch (const db::SubCircuit *a, const db::SubCircuit *b);

  size_t circuit_count () const
  {
    return m_per_circuit_data.size ();
  }

  typedef std::map<std::pair<const db::Circuit *, const db::Circuit *>, PerCircuitData>::const_iterator per_circuit_data_iterator;

  per_circuit_data_iterator begin_per_circuit_data () const
  {
    return m_per_circuit_data.begin ();
  }

  per_circuit_data_iterator end_per_circuit_data () const
  {
    return m_per_circuit_data.end ();
  }

  const PerCircuitData *per_circuit_data_for (const std::pair<const db::Circuit *, const db::Circuit *> &circuits) const
  {
    per_circuit_data_iterator i = m_per_circuit_data.find (circuits);
    if (i == m_per_circuit_data.end ()) {
      return 0;
    } else {
      return & i->second;
    }
  }

  typedef std::vector<std::pair<const db::Circuit *, const db::Circuit *> >::const_iterator circuits_iterator;

  circuits_iterator begin_circuits () const
  {
    return m_circuits.begin ();
  }

  circuits_iterator end_circuits () const
  {
    return m_circuits.end ();
  }

  const db::Net *other_net_for (const db::Net *net) const
  {
    std::map<const db::Net *, const db::Net *>::const_iterator i = m_other_net.find (net);
    if (i != m_other_net.end ()) {
      return i->second;
    } else {
      return 0;
    }
  }

  const PerNetData *per_net_data_for (const std::pair<const db::Net *, const db::Net *> &nets) const
  {
    std::map<std::pair<const db::Net *, const db::Net *>, PerNetData>::iterator i = m_per_net_data.find (nets);
    if (i == m_per_net_data.end ()) {
      i = m_per_net_data.insert (std::make_pair (nets, PerNetData ())).first;
      build_per_net_info (nets, i->second);
    }

    return &i->second;
  }

private:
  tl::weak_ptr<db::Netlist> mp_netlist_a, mp_netlist_b;
  std::vector<std::pair<const db::Circuit *, const db::Circuit *> > m_circuits;
  std::map<std::pair<const db::Circuit *, const db::Circuit *>, PerCircuitData> m_per_circuit_data;
  mutable std::map<std::pair<const db::Net *, const db::Net *>, PerNetData> m_per_net_data;
  std::map<const db::Circuit *, const db::Circuit *> m_other_circuit;
  std::map<const db::Net *, const db::Net *> m_other_net;
  std::map<const db::Device *, const db::Device *> m_other_device;
  std::map<const db::Pin *, const db::Pin *> m_other_pin;
  std::map<const db::SubCircuit *, const db::SubCircuit *> m_other_subcircuit;
  std::pair<const db::Circuit *, const db::Circuit *> m_current_circuits;
  std::map<std::pair<const db::Circuit *, const db::Circuit *>, status> m_circuit_status;
  std::map<std::pair<const db::Net *, const db::Net *>, status> m_net_status;
  std::map<std::pair<const db::Device *, const db::Device *>, status> m_device_status;
  std::map<std::pair<const db::Pin *, const db::Pin *>, status> m_pin_status;
  std::map<std::pair<const db::SubCircuit *, const db::SubCircuit *>, status> m_subcircuit_status;
  PerCircuitData *mp_per_circuit_data;

  void establish_pair (const db::Circuit *a, const db::Circuit *b);
  void establish_pair (const db::Net *a, const db::Net *b);
  void establish_pair (const db::Device *a, const db::Device *b);
  void establish_pair (const db::Pin *a, const db::Pin *b);
  void establish_pair (const db::SubCircuit *a, const db::SubCircuit *b);

  void build_per_net_info (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const;
  void build_subcircuit_pin_refs (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const;
  void build_pin_refs (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const;
  void build_terminal_refs (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const;
};

void
NetlistCrossReference::begin_netlist (const db::Netlist *a, const db::Netlist *b)
{
  mp_netlist_a.reset (const_cast <db::Netlist *> (a));
  mp_netlist_b.reset (const_cast <db::Netlist *> (b));
  m_current_circuits = std::pair<const db::Circuit *, const db::Circuit *> (0, 0);
}

void
NetlistCrossReference::end_netlist (const db::Netlist *, const db::Netlist *)
{
  m_circuits.reserve (m_per_circuit_data.size ());
  for (per_circuit_data_iterator i = begin_per_circuit_data (); i != end_per_circuit_data (); ++i) {
    m_circuits.push_back (i->first);
  }


  //  @@@ TODO: sort m_circuits by name?
}

void
NetlistCrossReference::device_class_mismatch (const db::DeviceClass *, const db::DeviceClass *)
{
  //  .. nothing yet ..
}

void
NetlistCrossReference::establish_pair (const db::Circuit *a, const db::Circuit *b)
{
  mp_per_circuit_data = & m_per_circuit_data [std::make_pair (a, b)];

  if (a) {
    m_other_circuit [a] = b;
  }
  if (b) {
    m_other_circuit [b] = a;
  }
}

void
NetlistCrossReference::establish_pair (const db::Net *a, const db::Net *b)
{
  mp_per_circuit_data->nets.push_back (std::make_pair (a, b));
  if (a) {
    m_other_net [a] = b;
  }
  if (b) {
    m_other_net [b] = a;
  }
}

void
NetlistCrossReference::establish_pair (const db::Device *a, const db::Device *b)
{
  mp_per_circuit_data->devices.push_back (std::make_pair (a, b));
  if (a) {
    m_other_device [a] = b;
  }
  if (b) {
    m_other_device [b] = a;
  }
}

void
NetlistCrossReference::establish_pair (const db::Pin *a, const db::Pin *b)
{
  mp_per_circuit_data->pins.push_back (std::make_pair (a, b));
  if (a) {
    m_other_pin [a] = b;
  }
  if (b) {
    m_other_pin [b] = a;
  }
}

void
NetlistCrossReference::establish_pair (const db::SubCircuit *a, const db::SubCircuit *b)
{
  mp_per_circuit_data->subcircuits.push_back (std::make_pair (a, b));
  if (a) {
    m_other_subcircuit [a] = b;
  }
  if (b) {
    m_other_subcircuit [b] = a;
  }
}

void
NetlistCrossReference::begin_circuit (const db::Circuit *a, const db::Circuit *b)
{
  m_current_circuits = std::pair<const db::Circuit *, const db::Circuit *> (a, b);
  establish_pair (a, b);
}

void
NetlistCrossReference::end_circuit (const db::Circuit *, const db::Circuit *, bool matching)
{
  if (matching) {
    m_circuit_status [m_current_circuits] = Match;
  }

  //  @@@ TODO: sort per-circuit data vectors by name?

  m_current_circuits = std::pair<const db::Circuit *, const db::Circuit *> (0, 0);
  mp_per_circuit_data = 0;
}

void
NetlistCrossReference::circuit_skipped (const db::Circuit *a, const db::Circuit *b)
{
  establish_pair (a, b);
  m_circuit_status [std::pair<const db::Circuit *, const db::Circuit *> (a, b)] = Skipped;
}

void
NetlistCrossReference::circuit_mismatch (const db::Circuit *a, const db::Circuit *b)
{
  establish_pair (a, b);
  m_circuit_status [std::pair<const db::Circuit *, const db::Circuit *> (a, b)] = Mismatch;
}

void
NetlistCrossReference::match_nets (const db::Net *a, const db::Net *b)
{
  establish_pair (a, b);
  m_net_status [std::pair<const db::Net *, const db::Net *> (a, b)] = Match;
}

void
NetlistCrossReference::match_ambiguous_nets (const db::Net *a, const db::Net *b)
{
  establish_pair (a, b);
  m_net_status [std::pair<const db::Net *, const db::Net *> (a, b)] = MatchWithWarning;
}

void
NetlistCrossReference::net_mismatch (const db::Net *a, const db::Net *b)
{
  establish_pair (a, b);
  m_net_status [std::pair<const db::Net *, const db::Net *> (a, b)] = Mismatch;
}

void
NetlistCrossReference::match_devices (const db::Device *a, const db::Device *b)
{
  establish_pair (a, b);
  m_device_status [std::pair<const db::Device *, const db::Device *> (a, b)] = Match;
}

void
NetlistCrossReference::match_devices_with_different_parameters (const db::Device *a, const db::Device *b)
{
  establish_pair (a, b);
  m_device_status [std::pair<const db::Device *, const db::Device *> (a, b)] = MatchWithWarning;
}

void
NetlistCrossReference::match_devices_with_different_device_classes (const db::Device *a, const db::Device *b)
{
  establish_pair (a, b);
  m_device_status [std::pair<const db::Device *, const db::Device *> (a, b)] = MatchWithWarning;
}

void
NetlistCrossReference::device_mismatch (const db::Device *a, const db::Device *b)
{
  establish_pair (a, b);
  m_device_status [std::pair<const db::Device *, const db::Device *> (a, b)] = Mismatch;
}

void
NetlistCrossReference::match_pins (const db::Pin *a, const db::Pin *b)
{
  establish_pair (a, b);
  m_pin_status [std::pair<const db::Pin *, const db::Pin *> (a, b)] = Match;
}

void
NetlistCrossReference::pin_mismatch (const db::Pin *a, const db::Pin *b)
{
  establish_pair (a, b);
  m_pin_status [std::pair<const db::Pin *, const db::Pin *> (a, b)] = Mismatch;
}

void
NetlistCrossReference::match_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b)
{
  establish_pair (a, b);
  m_subcircuit_status [std::pair<const db::SubCircuit *, const db::SubCircuit *> (a, b)] = Match;
}

void
NetlistCrossReference::subcircuit_mismatch (const db::SubCircuit *a, const db::SubCircuit *b)
{
  establish_pair (a, b);
  m_subcircuit_status [std::pair<const db::SubCircuit *, const db::SubCircuit *> (a, b)] = Mismatch;
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
    if (idb != m_other_device.end ()) {

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
    if (ipb != m_other_pin.end ()) {

      const db::Pin *pb = ipb->second;
      std::map<const Pin *, const db::NetPinRef *>::iterator b = p2r_b.find (pb);
      if (b != p2r_b.end ()) {
        prb = b->second;
        //  remove the entry so we won't find it again
        p2r_b.erase (b);
        break;
      }

    }

    data.pins.push_back (std::make_pair (a->second, prb));

  }

  for (std::map<const Pin *, const db::NetPinRef *>::const_iterator b = p2r_b.begin (); b != p2r_b.end (); ++b) {
    data.pins.push_back (std::make_pair ((const db::NetPinRef *) 0, b->second));
  }
}

void
NetlistCrossReference::build_subcircuit_pin_refs (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const
{
  std::map<std::pair<const db::SubCircuit *, size_t>, const db::NetSubcircuitPinRef *> s2t_a, s2t_b;

  for (db::Net::const_subcircuit_pin_iterator i = nets.first->begin_subcircuit_pins (); i != nets.first->begin_subcircuit_pins (); ++i) {
    s2t_a.insert (std::make_pair (std::make_pair (i->subcircuit (), i->pin_id ()), i.operator-> ()));
  }

  for (db::Net::const_subcircuit_pin_iterator i = nets.second->begin_subcircuit_pins (); i != nets.second->begin_subcircuit_pins (); ++i) {
    s2t_b.insert (std::make_pair (std::make_pair (i->subcircuit (), i->pin_id ()), i.operator-> ()));
  }

  for (std::map<std::pair<const db::SubCircuit *, size_t>, const db::NetSubcircuitPinRef *>::const_iterator a = s2t_a.begin (); a != s2t_a.end (); ++a) {

    const db::SubCircuit *sa = a->first.first;

    const db::NetSubcircuitPinRef *pb = 0;

    std::map<const db::SubCircuit *, const db::SubCircuit *>::const_iterator isb = m_other_subcircuit.find (sa);
    if (isb != m_other_subcircuit.end ()) {

      const db::SubCircuit *sb = isb->second;

      //  we have a subcircuit pair - now we need to match the pins: we do so on the basis
      //  pin matching

      const db::Pin *pa = sa->circuit_ref ()->pin_by_id (a->first.second);
      std::map<const db::Pin *, const db::Pin *>::const_iterator ipb = m_other_pin.find (pa);
      if (ipb != m_other_pin.end ()) {

        std::map<std::pair<const db::SubCircuit *, size_t>, const db::NetSubcircuitPinRef *>::iterator b = s2t_b.find (std::make_pair (sb, ipb->second->id ()));
        if (b != s2t_b.end ()) {
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
}

void
NetlistCrossReference::build_per_net_info (const std::pair<const db::Net *, const db::Net *> &nets, PerNetData &data) const
{
  if (! nets.second) {
    init_data_from_single (nets.first, data, true);
  } else if (! nets.first) {
    init_data_from_single (nets.second, data, false);
  } else if (nets.first) {
    build_terminal_refs (nets, data);
    build_pin_refs (nets, data);
    build_subcircuit_pin_refs (nets, data);
  }
}

}
// @@@

namespace lay
{

/**
 *  @brief An indexed netlist model for the netlist cross-reference
 */
class LAYBASIC_PUBLIC NetlistCrossReferenceModel
  : public lay::IndexedNetlistModel
{
public:
  NetlistCrossReferenceModel (db::NetlistCrossReference *cross_ref);

  virtual bool is_single () const { return false; }

  virtual size_t circuit_count () const;
  virtual size_t net_count (const circuit_pair &circuits) const;
  virtual size_t net_terminal_count (const net_pair &nets) const;
  virtual size_t net_subcircuit_pin_count (const net_pair &nets) const;
  virtual size_t net_pin_count (const net_pair &nets) const;
  virtual size_t device_count (const circuit_pair &circuits) const;
  virtual size_t pin_count (const circuit_pair &circuits) const;
  virtual size_t subcircuit_count (const circuit_pair &circuits) const;

  virtual circuit_pair parent_of (const net_pair &net_pair) const;
  virtual circuit_pair parent_of (const device_pair &device_pair) const;
  virtual circuit_pair parent_of (const subcircuit_pair &subcircuit_pair) const;

  virtual circuit_pair circuit_from_index (size_t index) const;
  virtual net_pair net_from_index (const circuit_pair &circuits, size_t index) const;
  virtual const db::Net *second_net_for (const db::Net *first) const;
  virtual net_subcircuit_pin_pair net_subcircuit_pinref_from_index (const net_pair &nets, size_t index) const;
  virtual net_terminal_pair net_terminalref_from_index (const net_pair &nets, size_t index) const;
  virtual net_pin_pair net_pinref_from_index (const net_pair &nets, size_t index) const;
  virtual device_pair device_from_index (const circuit_pair &circuits, size_t index) const;
  virtual pin_pair pin_from_index (const circuit_pair &circuits, size_t index) const;
  virtual subcircuit_pair subcircuit_from_index (const circuit_pair &circuits, size_t index) const;

  virtual size_t circuit_index (const circuit_pair &circuits) const;
  virtual size_t net_index (const net_pair &nets) const;
  virtual size_t device_index (const device_pair &devices) const;
  virtual size_t pin_index (const pin_pair &pins, const circuit_pair &circuits) const;
  virtual size_t subcircuit_index (const subcircuit_pair &subcircuits) const;

public:
  struct PerCircuitCacheData
  {
    std::map<std::pair<const db::Net *, const db::Net *>, size_t> index_of_nets;
    std::map<std::pair<const db::Device *, const db::Device *>, size_t> index_of_devices;
    std::map<std::pair<const db::Pin *, const db::Pin *>, size_t> index_of_pins;
    std::map<std::pair<const db::SubCircuit *, const db::SubCircuit *>, size_t> index_of_subcircuits;
  };

  tl::weak_ptr<db::NetlistCrossReference> mp_cross_ref;
  mutable std::map<net_pair, circuit_pair> m_parents_of_nets;
  mutable std::map<device_pair, circuit_pair> m_parents_of_devices;
  mutable std::map<pin_pair, circuit_pair> m_parents_of_pins;
  mutable std::map<subcircuit_pair, circuit_pair> m_parents_of_subcircuits;
  mutable std::map<std::pair<const db::Circuit *, const db::Circuit *>, PerCircuitCacheData> m_per_circuit_data;
  mutable std::map<std::pair<const db::Circuit *, const db::Circuit *>, size_t> m_index_of_circuits;
};

}

#endif

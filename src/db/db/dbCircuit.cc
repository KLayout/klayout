
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

#include "dbCircuit.h"
#include "dbNetlist.h"

namespace db
{

// --------------------------------------------------------------------------------
//  Circuit class implementation

Circuit::Circuit ()
  : m_cell_index (0), mp_netlist (0),
    m_device_by_id (this, &Circuit::begin_devices, &Circuit::end_devices),
    m_subcircuit_by_id (this, &Circuit::begin_subcircuits, &Circuit::end_subcircuits),
    m_net_by_cluster_id (this, &Circuit::begin_nets, &Circuit::end_nets),
    m_device_by_name (this, &Circuit::begin_devices, &Circuit::end_devices),
    m_subcircuit_by_name (this, &Circuit::begin_subcircuits, &Circuit::end_subcircuits),
    m_net_by_name (this, &Circuit::begin_nets, &Circuit::end_nets),
    m_index (0)
{
  m_devices.changed ().add (this, &Circuit::devices_changed);
  m_nets.changed ().add (this, &Circuit::nets_changed);
  m_subcircuits.changed ().add (this, &Circuit::subcircuits_changed);
}

Circuit::Circuit (const Circuit &other)
  : gsi::ObjectBase (other), tl::Object (other), m_cell_index (0), mp_netlist (0),
    m_device_by_id (this, &Circuit::begin_devices, &Circuit::end_devices),
    m_subcircuit_by_id (this, &Circuit::begin_subcircuits, &Circuit::end_subcircuits),
    m_net_by_cluster_id (this, &Circuit::begin_nets, &Circuit::end_nets),
    m_device_by_name (this, &Circuit::begin_devices, &Circuit::end_devices),
    m_subcircuit_by_name (this, &Circuit::begin_subcircuits, &Circuit::end_subcircuits),
    m_net_by_name (this, &Circuit::begin_nets, &Circuit::end_nets),
    m_index (0)
{
  operator= (other);
  m_devices.changed ().add (this, &Circuit::devices_changed);
  m_nets.changed ().add (this, &Circuit::nets_changed);
  m_subcircuits.changed ().add (this, &Circuit::subcircuits_changed);
}

Circuit::~Circuit ()
{
  m_devices.changed ().remove (this, &Circuit::devices_changed);
  m_nets.changed ().remove (this, &Circuit::nets_changed);
  m_subcircuits.changed ().remove (this, &Circuit::subcircuits_changed);

  //  the default destructor will make the nets access "this" to unregister the
  //  objects - hence we have to do this explicitly.
  m_nets.clear ();
  m_subcircuits.clear ();
  m_devices.clear ();
}

Circuit &Circuit::operator= (const Circuit &other)
{
  if (this != &other) {

    clear ();

    m_name = other.m_name;
    m_cell_index = other.m_cell_index;
    m_pins = other.m_pins;

    std::map<const Device *, Device *> device_table;
    for (const_device_iterator i = other.begin_devices (); i != other.end_devices (); ++i) {
      Device *d = new Device (*i);
      device_table [i.operator-> ()] = d;
      add_device (d);
    }

    std::map<const SubCircuit *, SubCircuit *> sc_table;
    for (const_subcircuit_iterator i = other.begin_subcircuits (); i != other.end_subcircuits (); ++i) {
      SubCircuit *sc = new SubCircuit (*i);
      sc_table [i.operator-> ()] = sc;
      add_subcircuit (sc);
    }

    for (const_net_iterator i = other.begin_nets (); i != other.end_nets (); ++i) {

      //  translate the net
      Net *n = new Net ();
      n->set_cluster_id (i->cluster_id ());
      n->set_name (i->name ());
      add_net (n);

      for (Net::const_terminal_iterator p = i->begin_terminals (); p != i->end_terminals (); ++p) {
        std::map<const Device *, Device *>::const_iterator m = device_table.find (p->device ());
        tl_assert (m != device_table.end ());
        n->add_terminal (NetTerminalRef (m->second, p->terminal_id ()));
      }

      for (Net::const_pin_iterator p = i->begin_pins (); p != i->end_pins (); ++p) {
        n->add_pin (NetPinRef (p->pin_id ()));
      }

      for (Net::const_subcircuit_pin_iterator p = i->begin_subcircuit_pins (); p != i->end_subcircuit_pins (); ++p) {
        std::map<const SubCircuit *, SubCircuit *>::const_iterator m = sc_table.find (p->subcircuit ());
        tl_assert (m != sc_table.end ());
        n->add_subcircuit_pin (NetSubcircuitPinRef (m->second, p->pin_id ()));
      }

    }

  }

  return *this;
}

void Circuit::set_netlist (Netlist *netlist)
{
  mp_netlist = netlist;
}

const Pin *Circuit::pin_by_id (size_t id) const
{
  if (id >= m_pins.size ()) {
    return 0;
  } else {
    return &m_pins [id];
  }
}

const Pin *Circuit::pin_by_name (const std::string &name) const
{
  for (Circuit::const_pin_iterator p = begin_pins (); p != end_pins (); ++p) {
    if (p->name () == name) {
      return p.operator-> ();
    }
  }
  return 0;
}

void Circuit::devices_changed ()
{
  m_device_by_id.invalidate ();
  m_device_by_name.invalidate ();
}

void Circuit::subcircuits_changed ()
{
  m_subcircuit_by_id.invalidate ();
  m_subcircuit_by_name.invalidate ();

  if (mp_netlist) {
    mp_netlist->invalidate_topology ();
  }
}

void Circuit::nets_changed ()
{
  m_net_by_cluster_id.invalidate ();
  m_net_by_name.invalidate ();
}

void Circuit::clear ()
{
  m_name.clear ();
  m_pins.clear ();
  m_devices.clear ();
  m_nets.clear ();
  m_subcircuits.clear ();
}

void Circuit::set_name (const std::string &name)
{
  m_name = name;
  if (mp_netlist) {
    mp_netlist->m_circuit_by_name.invalidate ();
  }
}

void Circuit::set_cell_index (const db::cell_index_type ci)
{
  m_cell_index = ci;
  if (mp_netlist) {
    mp_netlist->m_circuit_by_cell_index.invalidate ();
  }
}

Circuit::child_circuit_iterator Circuit::begin_children ()
{
  tl_assert (mp_netlist != 0);
  return mp_netlist->child_circuits (this).begin ();
}

Circuit::child_circuit_iterator Circuit::end_children ()
{
  tl_assert (mp_netlist != 0);
  return mp_netlist->child_circuits (this).end ();
}

Circuit::const_child_circuit_iterator Circuit::begin_children () const
{
  tl_assert (mp_netlist != 0);
  return reinterpret_cast<const tl::vector<const Circuit *> &> (mp_netlist->child_circuits (const_cast <Circuit *> (this))).begin ();
}

Circuit::const_child_circuit_iterator Circuit::end_children () const
{
  tl_assert (mp_netlist != 0);
  return reinterpret_cast<const tl::vector<const Circuit *> &> (mp_netlist->child_circuits (const_cast <Circuit *> (this))).end ();
}

Circuit::child_circuit_iterator Circuit::begin_parents ()
{
  tl_assert (mp_netlist != 0);
  return mp_netlist->parent_circuits (this).begin ();
}

Circuit::child_circuit_iterator Circuit::end_parents ()
{
  tl_assert (mp_netlist != 0);
  return mp_netlist->parent_circuits (this).end ();
}

Circuit::const_child_circuit_iterator Circuit::begin_parents () const
{
  tl_assert (mp_netlist != 0);
  return reinterpret_cast<const tl::vector<const Circuit *> &> (mp_netlist->parent_circuits (const_cast <Circuit *> (this))).begin ();
}

Circuit::const_child_circuit_iterator Circuit::end_parents () const
{
  tl_assert (mp_netlist != 0);
  return reinterpret_cast<const tl::vector<const Circuit *> &> (mp_netlist->parent_circuits (const_cast <Circuit *> (this))).end ();
}

void Circuit::clear_pins ()
{
  m_pins.clear ();
}

const Pin &Circuit::add_pin (const std::string &name)
{
  m_pins.push_back (Pin (name));
  m_pins.back ().set_id (m_pins.size () - 1);
  return m_pins.back ();
}

void Circuit::add_net (Net *net)
{
  m_nets.push_back (net);
  net->set_circuit (this);
}

void Circuit::remove_net (Net *net)
{
  m_nets.erase (net);
}

void Circuit::add_device (Device *device)
{
  device->set_circuit (this);

  size_t id = 0;
  if (! m_devices.empty ()) {
    tl_assert (m_devices.back () != 0);
    id = m_devices.back ()->id ();
  }
  device->set_id (id + 1);

  m_devices.push_back (device);
}

void Circuit::remove_device (Device *device)
{
  m_devices.erase (device);
}

void Circuit::add_subcircuit (SubCircuit *subcircuit)
{
  subcircuit->set_circuit (this);

  size_t id = 0;
  if (! m_subcircuits.empty ()) {
    tl_assert (m_subcircuits.back () != 0);
    id = m_subcircuits.back ()->id ();
  }
  subcircuit->set_id (id + 1);

  m_subcircuits.push_back (subcircuit);
}

void Circuit::remove_subcircuit (SubCircuit *subcircuit)
{
  m_subcircuits.erase (subcircuit);
}

void Circuit::register_ref (SubCircuit *r)
{
  m_refs.push_back (r);
}

void Circuit::unregister_ref (SubCircuit *r)
{
  m_refs.erase (r);
}

void Circuit::flatten_subcircuit (SubCircuit *subcircuit)
{
  tl_assert (subcircuit != 0);

  const db::Circuit *c = subcircuit->circuit_ref ();

  //  copy the nets, produce a net map

  std::map<const db::Net *, db::Net *> net2net;

  for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {

    //  TODO: cannot join pins through subcircuits currently
    tl_assert (n->pin_count () <= 1);

    db::Net *outside_net = 0;

    if (n->pin_count () > 0) {
      size_t pin_id = n->begin_pins ()->pin_id ();
      outside_net = subcircuit->net_for_pin (pin_id);
    } else {
      outside_net = new db::Net ();
      if (! n->name ().empty ()) {
        outside_net->set_name (subcircuit->expanded_name () + "." + n->name ());
      }
      add_net (outside_net);
    }

    net2net.insert (std::make_pair (n.operator-> (), outside_net));

  }

  //  copy the devices

  for (db::Circuit::const_device_iterator d = c->begin_devices (); d != c->end_devices (); ++d) {

    db::Device *device = new db::Device (*d);
    if (! d->name ().empty ()) {
      device->set_name (subcircuit->expanded_name () + "." + d->name ());
    }
    add_device (device);

    const std::vector<db::DeviceTerminalDefinition> &td = d->device_class ()->terminal_definitions ();
    for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {

      const db::Net *tnet = d->net_for_terminal (t->id ());
      if (tnet) {
        std::map<const db::Net *, db::Net *>::const_iterator n2n = net2net.find (tnet);
        tl_assert (n2n != net2net.end ());
        device->connect_terminal (t->id (), n2n->second);
      }

    }

  }

  //  copy the subcircuits

  for (db::Circuit::const_subcircuit_iterator sc = c->begin_subcircuits (); sc != c->end_subcircuits (); ++sc) {

    db::SubCircuit *new_subcircuit = new db::SubCircuit (*sc);
    if (! new_subcircuit->name ().empty ()) {
      new_subcircuit->set_name (subcircuit->expanded_name () + "." + new_subcircuit->name ());
    }
    add_subcircuit (new_subcircuit);

    const db::Circuit *cr = sc->circuit_ref ();
    for (db::Circuit::const_pin_iterator p = cr->begin_pins (); p != cr->end_pins (); ++p) {

      const db::Net *pnet = sc->net_for_pin (p->id ());
      if (pnet) {
        std::map<const db::Net *, db::Net *>::const_iterator n2n = net2net.find (pnet);
        tl_assert (n2n != net2net.end ());
        new_subcircuit->connect_pin (p->id (), n2n->second);
      }

    }

  }

  delete subcircuit;
}

void Circuit::translate_circuits (const std::map<const Circuit *, Circuit *> &map)
{
  for (subcircuit_iterator i = m_subcircuits.begin (); i != m_subcircuits.end (); ++i) {
    std::map<const Circuit *, Circuit *>::const_iterator m = map.find (i->circuit_ref ());
    tl_assert (m != map.end ());
    i->set_circuit_ref (m->second);
  }
}

void Circuit::translate_device_classes (const std::map<const DeviceClass *, DeviceClass *> &map)
{
  for (device_iterator i = m_devices.begin (); i != m_devices.end (); ++i) {
    std::map<const DeviceClass *, DeviceClass *>::const_iterator m = map.find (i->device_class ());
    tl_assert (m != map.end ());
    i->set_device_class (m->second);
  }
}

void Circuit::translate_device_abstracts (const std::map<const DeviceAbstract *, DeviceAbstract *> &map)
{
  for (device_iterator i = m_devices.begin (); i != m_devices.end (); ++i) {
    if (i->device_abstract ()) {
      std::map<const DeviceAbstract *, DeviceAbstract *>::const_iterator m = map.find (i->device_abstract ());
      tl_assert (m != map.end ());
      i->set_device_abstract (m->second);
    }
  }
}

void Circuit::set_pin_ref_for_pin (size_t pin_id, Net::pin_iterator iter)
{
  if (m_pin_refs.size () < pin_id + 1) {
    m_pin_refs.resize (pin_id + 1, Net::pin_iterator ());
  }
  m_pin_refs [pin_id] = iter;
}

const Net *Circuit::net_for_pin (size_t pin_id) const
{
  if (pin_id < m_pin_refs.size ()) {
    Net::pin_iterator p = m_pin_refs [pin_id];
    if (p != Net::pin_iterator ()) {
      return p->net ();
    }
  }
  return 0;
}

void Circuit::connect_pin (size_t pin_id, Net *net)
{
  if (net_for_pin (pin_id) == net) {
    return;
  }

  if (pin_id < m_pin_refs.size ()) {
    Net::pin_iterator p = m_pin_refs [pin_id];
    if (p != Net::pin_iterator () && p->net ()) {
      p->net ()->erase_pin (p);
    }
    m_pin_refs [pin_id] = Net::pin_iterator ();
  }

  if (net) {
    net->add_pin (NetPinRef (pin_id));
  }
}

void Circuit::purge_nets ()
{
  std::vector<db::Net *> nets_to_be_purged;
  for (net_iterator n = begin_nets (); n != end_nets (); ++n) {
    if (n->is_floating ()) {
      nets_to_be_purged.push_back (n.operator-> ());
    }
  }
  for (std::vector<db::Net *>::const_iterator n = nets_to_be_purged.begin (); n != nets_to_be_purged.end (); ++n) {
    delete *n;
  }
}

/**
 *  @brief Sanity check for device to be removed
 */
static void check_device_before_remove (db::Circuit *c, const db::Device *d)
{
  if (d->device_class () == 0) {
    throw tl::Exception (tl::to_string (tr ("Internal error: No device class after removing device in device combination")) + ": name=" + d->name () + ", circuit=" + c->name ());
  }
  const std::vector<db::DeviceTerminalDefinition> &pd = d->device_class ()->terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
    if (d->net_for_terminal (p->id ()) != 0) {
      throw tl::Exception (tl::to_string (tr ("Internal error: Terminal still connected after removing device in device combination")) + ": name=" + d->name () + ", circuit=" + c->name () + ", terminal=" + p->name ());
    }
  }
}

bool Circuit::combine_parallel_devices (const db::DeviceClass &cls)
{
  typedef std::vector<const db::Net *> key_type;
  std::map<key_type, std::vector<db::Device *> > combination_candidates;

  bool any = false;

  //  identify the candidates for combination - all devices sharing the same nets
  //  are candidates for combination in parallel mode
  for (device_iterator d = begin_devices (); d != end_devices (); ++d) {

    if (tl::id_of (d->device_class ()) != tl::id_of (&cls)) {
      continue;
    }

    key_type k;
    const std::vector<db::DeviceTerminalDefinition> &terminals = cls.terminal_definitions ();
    for (std::vector<db::DeviceTerminalDefinition>::const_iterator p = terminals.begin (); p != terminals.end (); ++p) {
      const db::Net *n = d->net_for_terminal (p->id ());
      if (n) {
        k.push_back (n);
      }
    }

    std::sort (k.begin (), k.end ());
    k.erase (std::unique (k.begin (), k.end ()), k.end ());
    combination_candidates[k].push_back (d.operator-> ());

  }

  //  actually combine the devices
  for (std::map<key_type, std::vector<db::Device *> >::iterator cc = combination_candidates.begin (); cc != combination_candidates.end (); ++cc) {

    std::vector<db::Device *> &cl = cc->second;
    for (size_t i = 0; i < cl.size () - 1; ++i) {
      for (size_t j = i + 1; j < cl.size (); ) {
        if (cls.combine_devices (cl [i], cl [j])) {
          check_device_before_remove (this, cl [j]);  //  sanity check
          delete cl [j];
          cl.erase (cl.begin () + j);
          any = true;
        } else {
          ++j;
        }
      }
    }

  }

  return any;
}

static std::pair<db::Device *, db::Device *> attached_two_devices (db::Net &net, const db::DeviceClass &cls)
{
  if (net.begin_pins () != net.end_pins ()) {
    return std::make_pair ((db::Device *) 0, (db::Device *) 0);
  }

  db::Device *d1 = 0, *d2 = 0;

  Net::terminal_iterator p = net.begin_terminals ();
  if (p == net.end_terminals () || tl::id_of (p->device_class ()) != tl::id_of (&cls)) {
    return std::make_pair ((db::Device *) 0, (db::Device *) 0);
  } else {
    d1 = p->device ();
  }

  ++p;
  if (p == net.end_terminals () || tl::id_of (p->device_class ()) != tl::id_of (&cls)) {
    return std::make_pair ((db::Device *) 0, (db::Device *) 0);
  } else {
    d2 = p->device ();
  }

  ++p;
  if (p != net.end_terminals () || d1 == d2 || !d1 || !d2) {
    return std::make_pair ((db::Device *) 0, (db::Device *) 0);
  } else {
    return std::make_pair (d1, d2);
  }
}

template <class T>
static bool same_or_swapped (const std::pair<T, T> &p1, const std::pair<T, T> &p2)
{
  return (p1.first == p2.first && p1.second == p2.second) || (p1.first == p2.second && p1.second == p2.first);
}

bool Circuit::combine_serial_devices(const db::DeviceClass &cls)
{
  bool any = false;

  for (net_iterator n = begin_nets (); n != end_nets (); ++n) {

    std::pair<db::Device *, db::Device *> dd = attached_two_devices (*n, cls);
    if (! dd.first) {
      continue;
    }

    //  The net is an internal node: the devices attached to this internal node are
    //  combination candidates if the number of nets emerging from the attached device pair (not counting
    //  the internal node we just found) does not exceed the number of pins available for the
    //  new device.

    std::vector<const db::Net *> other_nets;

    const std::vector<db::DeviceTerminalDefinition> &terminals = cls.terminal_definitions ();
    for (std::vector<db::DeviceTerminalDefinition>::const_iterator p = terminals.begin (); p != terminals.end (); ++p) {
      db::Net *on;
      on = dd.first->net_for_terminal (p->id ());
      if (on && ! same_or_swapped (dd, attached_two_devices (*on, cls))) {
        other_nets.push_back (on);
      }
      on = dd.second->net_for_terminal (p->id ());
      if (on && ! same_or_swapped (dd, attached_two_devices (*on, cls))) {
        other_nets.push_back (on);
      }
    }

    std::sort (other_nets.begin (), other_nets.end ());
    other_nets.erase (std::unique (other_nets.begin (), other_nets.end ()), other_nets.end ());

    if (other_nets.size () <= cls.terminal_definitions().size ()) {

      //  found a combination candidate
      if (cls.combine_devices (dd.first, dd.second)) {
        check_device_before_remove (this, dd.second);  //  sanity check
        delete dd.second;
        any = true;
      }

    }

  }

  return any;
}

void Circuit::combine_devices ()
{
  tl_assert (netlist () != 0);

  for (Netlist::device_class_iterator dc = netlist ()->begin_device_classes (); dc != netlist ()->end_device_classes (); ++dc) {

    //  repeat the combination step unless no combination happens - this is required to take care of combinations that arise after
    //  other combinations have been realized.
    bool any = true;
    while (any) {

      any = false;

      if (dc->supports_parallel_combination ()) {
        if (combine_parallel_devices (*dc)) {
          any = true;
        }
      }
      if (dc->supports_serial_combination ()) {
        if (combine_serial_devices (*dc)) {
          any = true;
        }
      }

    }

  }
}

}

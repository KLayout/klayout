
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

#include "dbCircuit.h"
#include "dbNetlist.h"
#include "dbLayout.h"
#include "tlIteratorUtils.h"

#include <set>

namespace db
{

/**
 *  @brief Creates a joined name for nets and pins
 */
static std::string
join_names (const std::string &n1, const std::string &n2)
{
  //  create a new name for the joined net
  if (n2.empty ()) {
    return n1;
  } else if (n1.empty ()) {
    return n2;
  } else if (n1 == n2) {
    return n1;
  } else {
    //  separate parts (if already joined) and mix
    auto p1 = tl::split (n1, ",");
    auto p2 = tl::split (n2, ",");
    std::set<std::string> ps;
    ps.insert (p1.begin (), p1.end ());
    ps.insert (p2.begin (), p2.end ());
    return tl::join (ps.begin (), ps.end (), ",");
  }
}

// --------------------------------------------------------------------------------
//  Circuit class implementation

Circuit::Circuit ()
  : db::NetlistObject (), gsi::ObjectBase (), m_dont_purge (false), m_cell_index (0), mp_netlist (0),
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

Circuit::Circuit (const db::Layout &layout, db::cell_index_type ci)
  : db::NetlistObject (), gsi::ObjectBase (), m_name (layout.cell_name (ci)), m_dont_purge (false), m_cell_index (ci), mp_netlist (0),
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

  set_boundary (db::DPolygon (db::CplxTrans (layout.dbu ()) * layout.cell (ci).bbox ()));
}

Circuit::Circuit (const Circuit &other)
  : db::NetlistObject (other), gsi::ObjectBase (other), m_dont_purge (false), m_cell_index (0), mp_netlist (0),
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
  clear ();
}

Circuit &Circuit::operator= (const Circuit &other)
{
  if (this != &other) {

    db::NetlistObject::operator= (other);

    clear ();

    m_name = other.m_name;
    m_boundary = other.m_boundary;
    m_dont_purge = other.m_dont_purge;
    m_cell_index = other.m_cell_index;
    m_pins = other.m_pins;

    m_pin_by_id.clear ();
    for (pin_list::iterator p = m_pins.begin (); p != m_pins.end (); ++p) {
      if (m_pin_by_id.size () <= p->id ()) {
        m_pin_by_id.resize (p->id () + 1, pin_list::iterator ());
      }
      m_pin_by_id [p->id ()] = p;
    }

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
  if (id >= m_pin_by_id.size ()) {
    return 0;
  } else {
    pin_list::iterator pi = m_pin_by_id [id];
    if (tl::is_null_iterator (pi)) {
      return 0;
    } else {
      return pi.operator-> ();
    }
  }
}

void Circuit::rename_pin (size_t id, const std::string &name)
{
  if (id < m_pin_by_id.size () && ! tl::is_null_iterator (m_pin_by_id [id])) {
    m_pin_by_id [id]->set_name (name);
  }
}

const Pin *Circuit::pin_by_name (const std::string &name) const
{
  std::string nn = mp_netlist ? mp_netlist->normalize_name (name) : name;

  for (Circuit::const_pin_iterator p = begin_pins (); p != end_pins (); ++p) {
    if (p->name () == nn) {
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
  m_pin_by_id.clear ();
  m_devices.clear ();
  m_nets.clear ();
  m_subcircuits.clear ();
  m_boundary.clear ();
}

void Circuit::set_name (const std::string &name)
{
  m_name = name;
  if (mp_netlist) {
    mp_netlist->m_circuit_by_name.invalidate ();
  }
}

void Circuit::set_boundary (const db::DPolygon &boundary)
{
  m_boundary = boundary;
}

void Circuit::set_dont_purge (bool dp)
{
  m_dont_purge = dp;
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
  m_pin_by_id.clear ();
}

Pin &Circuit::add_pin (const Pin &pin)
{
  m_pins.push_back (pin);
  m_pins.back ().set_id (m_pin_by_id.size ());
  m_pin_by_id.push_back (--m_pins.end ());
  return m_pins.back ();
}

Pin &Circuit::add_pin (const std::string &name)
{
  m_pins.push_back (Pin (name));
  m_pins.back ().set_id (m_pin_by_id.size ());
  m_pin_by_id.push_back (--m_pins.end ());
  return m_pins.back ();
}

void Circuit::remove_pin (size_t id)
{
  if (id < m_pin_by_id.size () && ! tl::is_null_iterator (m_pin_by_id [id])) {
    m_pins.erase (m_pin_by_id [id]);
    m_pin_by_id [id] = pin_list::iterator ();
  }
}

Net *Circuit::net_by_name (const std::string &name)
{
  return m_net_by_name.object_by (mp_netlist ? mp_netlist->normalize_name (name) : name);
}

void Circuit::add_net (Net *net)
{
  if (! net) {
    return;
  }
  if (net->circuit ()) {
    throw tl::Exception (tl::to_string (tr ("Net already part of a circuit")));
  }

  m_nets.push_back (net);
  net->set_circuit (this);
}

void Circuit::remove_net (Net *net)
{
  if (! net) {
    return;
  }
  if (net->circuit () != this) {
    throw tl::Exception (tl::to_string (tr ("Net not withing given circuit")));
  }

  m_nets.erase (net);
}

void Circuit::join_nets (Net *net, Net *with)
{
  if (net == with || ! with || ! net) {
    return;
  }
  if (net->circuit () != this || with->circuit () != this) {
    throw tl::Exception (tl::to_string (tr ("Nets not within given circuit")));
  }

  while (with->begin_terminals () != with->end_terminals ()) {
    db::Device *device = const_cast<db::Device *> (with->begin_terminals ()->device ());
    device->connect_terminal (with->begin_terminals ()->terminal_id (), net);
  }

  while (with->begin_subcircuit_pins () != with->end_subcircuit_pins ()) {
    db::SubCircuit *subcircuit = const_cast<db::SubCircuit *> (with->begin_subcircuit_pins ()->subcircuit ());
    subcircuit->connect_pin (with->begin_subcircuit_pins ()->pin_id (), net);
  }

  while (with->begin_pins () != with->end_pins ()) {
    join_pin_with_net (with->begin_pins ()->pin_id (), net);
  }

  if (netlist ()->callbacks ()) {
    netlist ()->callbacks ()->link_nets (net, with);
  }

  //  create a new name for the joined net
  net->set_name (join_names (net->name (), with->name ()));

  remove_net (with);
}

void Circuit::add_device (Device *device)
{
  if (! device) {
    return;
  }
  if (device->circuit ()) {
    throw tl::Exception (tl::to_string (tr ("Device already in a circuit")));
  }

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
  if (! device) {
    return;
  }
  if (device->circuit () != this) {
    throw tl::Exception (tl::to_string (tr ("Device not withing given circuit")));
  }

  m_devices.erase (device);
}

Device *Circuit::device_by_name (const std::string &name)
{
  return m_device_by_name.object_by (mp_netlist ? mp_netlist->normalize_name (name) : name);
}

void Circuit::add_subcircuit (SubCircuit *subcircuit)
{
  if (! subcircuit) {
    return;
  }
  if (subcircuit->circuit ()) {
    throw tl::Exception (tl::to_string (tr ("Subcircuit already in a circuit")));
  }

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
  if (! subcircuit) {
    return;
  }
  if (subcircuit->circuit () != this) {
    throw tl::Exception (tl::to_string (tr ("Subcircuit not withing given circuit")));
  }

  m_subcircuits.erase (subcircuit);
}

SubCircuit *Circuit::subcircuit_by_name (const std::string &name)
{
  return m_subcircuit_by_name.object_by (mp_netlist ? mp_netlist->normalize_name (name) : name);
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
  if (! subcircuit) {
    return;
  }
  if (subcircuit->circuit () != this) {
    throw tl::Exception (tl::to_string (tr ("Subcircuit not withing given circuit")));
  }

  const db::Circuit *c = subcircuit->circuit_ref ();

  //  copy the nets, produce a net map

  std::map<const db::Net *, db::Net *> net2net;

  for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {

    db::Net *outside_net = 0;

    if (n->pin_count () > 0) {

      for (db::Net::const_pin_iterator p = n->begin_pins (); p != n->end_pins (); ++p) {

        size_t pin_id = p->pin_id ();

        if (outside_net) {
          join_nets (outside_net, subcircuit->net_for_pin (pin_id));
        } else {
          outside_net = subcircuit->net_for_pin (pin_id);
        }

      }

    } else {

      outside_net = new db::Net ();
      if (! n->name ().empty ()) {
        outside_net->set_name (subcircuit->expanded_name () + "." + n->name ());
      }
      add_net (outside_net);

      if (netlist ()->callbacks ()) {
        outside_net->set_cluster_id (netlist ()->callbacks ()->link_net_to_parent_circuit (n.operator-> (), this, subcircuit->trans ()));
      }

    }

    net2net.insert (std::make_pair (n.operator-> (), outside_net));

  }

  //  copy the devices

  for (db::Circuit::const_device_iterator d = c->begin_devices (); d != c->end_devices (); ++d) {

    db::Device *device = new db::Device (*d);
    if (! d->name ().empty ()) {
      device->set_name (subcircuit->expanded_name () + "." + d->name ());
    }
    device->set_trans (subcircuit->trans () * device->trans ());
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
    new_subcircuit->set_trans (subcircuit->trans () * new_subcircuit->trans ());
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
    i->translate_device_abstracts (map);
  }
}

void Circuit::set_pin_ref_for_pin (size_t pin_id, Net::pin_iterator iter)
{
  if (m_pin_refs.size () < pin_id + 1) {
    m_pin_refs.resize (pin_id + 1, Net::pin_iterator ());
  }
  m_pin_refs [pin_id] = iter;
}

void Circuit::blank ()
{
  tl_assert (netlist () != 0);

  std::set<db::Circuit *> cs;
  for (subcircuit_iterator i = m_subcircuits.begin (); i != m_subcircuits.end (); ++i) {
    cs.insert (i->circuit_ref ());
  }

  //  weak pointers are good because deleting a subcircuit might delete others ahead in
  //  this list:
  std::list<tl::weak_ptr<db::Circuit> > called_circuits;
  for (std::set<db::Circuit *>::const_iterator c = cs.begin (); c != cs.end (); ++c) {
    called_circuits.push_back (*c);
  }

  m_nets.clear ();
  m_subcircuits.clear ();
  m_devices.clear ();

  for (std::list<tl::weak_ptr<db::Circuit> >::iterator c = called_circuits.begin (); c != called_circuits.end (); ++c) {
    if (c->get () && ! (*c)->has_refs ()) {
      netlist ()->purge_circuit (c->get ());
    }
  }

  set_dont_purge (true);
}

const Net *Circuit::net_for_pin (size_t pin_id) const
{
  if (pin_id < m_pin_refs.size ()) {
    Net::pin_iterator p = m_pin_refs [pin_id];
    if (! tl::is_null_iterator (p)) {
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
    if (! tl::is_null_iterator (p) && p->net ()) {
      p->net ()->erase_pin (p);
    }
    m_pin_refs [pin_id] = Net::pin_iterator ();
  }

  if (net) {
    net->add_pin (NetPinRef (pin_id));
  }
}

void Circuit::join_pin_with_net (size_t pin_id, Net *net)
{
  if (net_for_pin (pin_id) == net) {
    return;
  }

  if (pin_id < m_pin_refs.size ()) {
    Net::pin_iterator p = m_pin_refs [pin_id];
    if (! tl::is_null_iterator (p) && p->net ()) {
      p->net ()->erase_pin (p);
    }
    m_pin_refs [pin_id] = Net::pin_iterator ();
  }

  if (net) {
    if (net->begin_pins () != net->end_pins ()) {
      join_pins (net->begin_pins ()->pin_id (), pin_id);
    } else {
      net->add_pin (NetPinRef (pin_id));
    }
  }
}

void Circuit::join_pins (size_t pin, size_t with)
{
  if (with != pin && with < m_pin_by_id.size () && ! tl::is_null_iterator (m_pin_by_id [with])) {

    //  create a new joined name
    m_pin_by_id [pin]->set_name (join_names (m_pin_by_id [pin]->name (), m_pin_by_id [with]->name ()));

    m_pins.erase (m_pin_by_id [with]);
    m_pin_by_id.erase (m_pin_by_id.begin () + with);
    m_pin_refs.erase (m_pin_refs.begin () + with);

    //  correct the pin IDs inside the circuit: all IDS > with will be reduced by 1
    if (pin > with) {
      --pin;
    }
    for (auto p = m_pins.begin (); p != m_pins.end (); ++p) {
      if (p->id () > with) {
        p->set_id (p->id () - 1);
      }
    }
    for (auto p = m_pin_refs.begin () + with; p != m_pin_refs.end (); ++p) {
      (*p)->set_pin_id ((*p)->pin_id () - 1);
    }

    //  join nets in calls
    for (auto s = begin_refs (); s != end_refs (); ++s) {

      db::SubCircuit &sc = *s;
      db::Net *with_net = sc.net_for_pin (with);

      //  NOTE: this will also correct the Pin IDs on the attached nets
      sc.erase_pin (with);

      sc.circuit ()->join_nets (sc.net_for_pin (pin), with_net);

    }

  }
}

void Circuit::purge_nets_keep_pins ()
{
  do_purge_nets (true);
}

void Circuit::purge_nets ()
{
  do_purge_nets (false);
}

void Circuit::do_purge_nets (bool keep_pins)
{
  std::vector<db::Net *> nets_to_be_purged;
  for (net_iterator n = begin_nets (); n != end_nets (); ++n) {
    if (n->is_passive ()) {
      nets_to_be_purged.push_back (n.operator-> ());
    }
  }

  std::set<size_t> pins_to_delete;

  for (std::vector<db::Net *>::const_iterator n = nets_to_be_purged.begin (); n != nets_to_be_purged.end (); ++n) {
    if (! keep_pins) {
      for (db::Net::pin_iterator p = (*n)->begin_pins (); p != (*n)->end_pins (); ++p) {
        pins_to_delete.insert (p->pin_id ());
      }
    }
    delete *n;
  }

  if (! pins_to_delete.empty ()) {

    //  remove the pin references of the pins we're going to delete
    for (refs_iterator r = begin_refs (); r != end_refs (); ++r) {
      db::SubCircuit *subcircuit = r.operator-> ();
      for (std::set<size_t>::const_iterator p = pins_to_delete.begin (); p != pins_to_delete.end (); ++p) {
        db::Net *net = subcircuit->net_for_pin (*p);
        for (db::Net::subcircuit_pin_iterator sp = net->begin_subcircuit_pins (); sp != net->end_subcircuit_pins (); ++sp) {
          if (sp->pin_id () == *p && sp->subcircuit () == subcircuit) {
            net->erase_subcircuit_pin (sp);
            break;
          }
        }
      }
    }

    //  and actually remove those pins
    for (std::set<size_t>::const_iterator p = pins_to_delete.begin (); p != pins_to_delete.end (); ++p) {
      remove_pin (*p);
    }

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
          cl [i]->join_device (cl [j]);
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
        dd.first->join_device (dd.second);
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

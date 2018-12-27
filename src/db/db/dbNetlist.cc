
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

#include "dbNetlist.h"

#include <set>

namespace db
{

// --------------------------------------------------------------------------------
//  Pin class implementation

Pin::Pin ()
  : m_id (0)
{
  //  .. nothing yet ..
}

Pin::Pin (const std::string &name)
  : m_name (name), m_id (0)
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------------------
//  Device class implementation

Device::Device ()
  : mp_device_class (0), m_id (0)
{
  //  .. nothing yet ..
}

Device::~Device ()
{
  for (std::vector<Net::terminal_iterator>::const_iterator t = m_terminal_refs.begin (); t != m_terminal_refs.end (); ++t) {
    if (*t != Net::terminal_iterator () && (*t)->net ()) {
      (*t)->net ()->erase_terminal (*t);
    }
  }
}

Device::Device (DeviceClass *device_class, const std::string &name)
  : mp_device_class (device_class), m_name (name), m_id (0)
{
  //  .. nothing yet ..
}

Device::Device (const Device &other)
  : mp_device_class (0), m_id (0)
{
  operator= (other);
}

Device &Device::operator= (const Device &other)
{
  if (this != &other) {
    m_name = other.m_name;
    mp_device_class = other.mp_device_class;
  }
  return *this;
}

void Device::set_name (const std::string &n)
{
  m_name = n;
}

void Device::set_terminal_ref_for_terminal (size_t terminal_id, Net::terminal_iterator iter)
{
  if (m_terminal_refs.size () < terminal_id + 1) {
    m_terminal_refs.resize (terminal_id + 1, Net::terminal_iterator ());
  }
  m_terminal_refs [terminal_id] = iter;
}

const Net *Device::net_for_terminal (size_t terminal_id) const
{
  if (terminal_id < m_terminal_refs.size ()) {
    Net::terminal_iterator p = m_terminal_refs [terminal_id];
    if (p != Net::terminal_iterator ()) {
      return p->net ();
    }
  }
  return 0;
}

void Device::connect_terminal (size_t terminal_id, Net *net)
{
  if (net_for_terminal (terminal_id) == net) {
    return;
  }

  if (terminal_id < m_terminal_refs.size ()) {
    Net::terminal_iterator p = m_terminal_refs [terminal_id];
    if (p != Net::terminal_iterator () && p->net ()) {
      p->net ()->erase_terminal (p);
    }
    m_terminal_refs [terminal_id] = Net::terminal_iterator ();
  }

  if (net) {
    net->add_terminal (NetTerminalRef (this, terminal_id));
  }
}

double Device::parameter_value (size_t param_id) const
{
  if (m_parameters.size () > param_id) {
    return m_parameters [param_id];
  } else if (mp_device_class) {
    const db::DeviceParameterDefinition *pd = mp_device_class->parameter_definition (param_id);
    if (pd) {
      return pd->default_value ();
    }
  }
  return 0.0;
}

void Device::set_parameter_value (size_t param_id, double v)
{
  if (m_parameters.size () <= param_id) {

    //  resize the parameter vector with default values
    size_t from_size = m_parameters.size ();
    m_parameters.resize (param_id + 1, 0.0);

    if (mp_device_class) {
      for (size_t n = from_size; n < param_id; ++n) {
        const db::DeviceParameterDefinition *pd = mp_device_class->parameter_definition (n);
        if (pd) {
          m_parameters [n] = pd->default_value ();
        }
      }
    }

  }

  m_parameters [param_id] = v;
}

double Device::parameter_value (const std::string &name) const
{
  return device_class () ? parameter_value (device_class ()->parameter_id_for_name (name)) : 0.0;
}

void Device::set_parameter_value (const std::string &name, double v)
{
  if (device_class ()) {
    set_parameter_value (device_class ()->parameter_id_for_name (name), v);
  }
}


// --------------------------------------------------------------------------------
//  SubCircuit class implementation

SubCircuit::SubCircuit ()
  : m_id (0)
{
  //  .. nothing yet ..
}

SubCircuit::~SubCircuit()
{
  for (std::vector<Net::pin_iterator>::const_iterator p = m_pin_refs.begin (); p != m_pin_refs.end (); ++p) {
    if (*p != Net::pin_iterator () && (*p)->net ()) {
      (*p)->net ()->erase_pin (*p);
    }
  }
}

SubCircuit::SubCircuit (Circuit *circuit, const std::string &name)
  : m_circuit (circuit), m_name (name), m_id (0)
{
  //  .. nothing yet ..
}

SubCircuit::SubCircuit (const SubCircuit &other)
  : m_id (0)
{
  operator= (other);
}

SubCircuit &SubCircuit::operator= (const SubCircuit &other)
{
  if (this != &other) {
    m_name = other.m_name;
    m_circuit = other.m_circuit;
    m_trans = other.m_trans;
  }
  return *this;
}

void SubCircuit::set_name (const std::string &n)
{
  m_name = n;
}

void SubCircuit::set_trans (const db::DCplxTrans &t)
{
  m_trans = t;
}

void SubCircuit::set_pin_ref_for_pin (size_t pin_id, Net::pin_iterator iter)
{
  if (m_pin_refs.size () < pin_id + 1) {
    m_pin_refs.resize (pin_id + 1, Net::pin_iterator ());
  }
  m_pin_refs [pin_id] = iter;
}

const Net *SubCircuit::net_for_pin (size_t pin_id) const
{
  if (pin_id < m_pin_refs.size ()) {
    Net::pin_iterator p = m_pin_refs [pin_id];
    if (p != Net::pin_iterator ()) {
      return p->net ();
    }
  }
  return 0;
}

void SubCircuit::connect_pin (size_t pin_id, Net *net)
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
    net->add_pin (NetPinRef (this, pin_id));
  }
}

// --------------------------------------------------------------------------------
//  NetTerminalRef class implementation

NetTerminalRef::NetTerminalRef ()
  : m_terminal_id (0), mp_device (0), mp_net (0)
{
  //  .. nothing yet ..
}

NetTerminalRef::NetTerminalRef (Device *device, size_t terminal_id)
  : m_terminal_id (terminal_id), mp_device (device), mp_net (0)
{
  //  .. nothing yet ..
}

NetTerminalRef::NetTerminalRef (const NetTerminalRef &other)
  : m_terminal_id (other.m_terminal_id), mp_device (other.mp_device), mp_net (0)
{
  //  .. nothing yet ..
}

NetTerminalRef &NetTerminalRef::operator= (const NetTerminalRef &other)
{
  if (this != &other) {
    m_terminal_id = other.m_terminal_id;
    mp_device = other.mp_device;
  }
  return *this;
}

const DeviceTerminalDefinition *
NetTerminalRef::terminal_def () const
{
  const DeviceClass *dc = device_class ();
  if (dc) {
    return dc->terminal_definition (m_terminal_id);
  } else {
    return 0;
  }
}

const DeviceClass *
NetTerminalRef::device_class () const
{
  return mp_device ? mp_device->device_class () : 0;
}

// --------------------------------------------------------------------------------
//  NetPinRef class implementation

NetPinRef::NetPinRef ()
  : m_pin_id (0), mp_subcircuit (0), mp_net (0)
{
  //  .. nothing yet ..
}

NetPinRef::NetPinRef (size_t pin_id)
  : m_pin_id (pin_id), mp_subcircuit (0), mp_net (0)
{
  //  .. nothing yet ..
}

NetPinRef::NetPinRef (SubCircuit *circuit, size_t pin_id)
  : m_pin_id (pin_id), mp_subcircuit (circuit), mp_net (0)
{
  //  .. nothing yet ..
}

NetPinRef::NetPinRef (const NetPinRef &other)
  : m_pin_id (other.m_pin_id), mp_subcircuit (other.mp_subcircuit), mp_net (0)
{
  //  .. nothing yet ..
}

NetPinRef &NetPinRef::operator= (const NetPinRef &other)
{
  if (this != &other) {
    m_pin_id = other.m_pin_id;
    mp_subcircuit = other.mp_subcircuit;
  }
  return *this;
}

const Pin *NetPinRef::pin () const
{
  if (! mp_subcircuit) {
    if (mp_net && mp_net->circuit ()) {
      return mp_net->circuit ()->pin_by_id (m_pin_id);
    }
  } else if (mp_subcircuit->circuit ()) {
    return mp_subcircuit->circuit ()->pin_by_id (m_pin_id);
  }
  return 0;
}

// --------------------------------------------------------------------------------
//  Net class implementation

Net::Net ()
  : m_cluster_id (0), mp_circuit (0)
{
  //  .. nothing yet ..
}

Net::Net (const Net &other)
  : m_cluster_id (0), mp_circuit (0)
{
  operator= (other);
}

Net &Net::operator= (const Net &other)
{
  if (this != &other) {

    clear ();

    m_name = other.m_name;
    m_cluster_id = other.m_cluster_id;

    for (const_pin_iterator i = other.begin_pins (); i != other.end_pins (); ++i) {
      add_pin (*i);
    }

    for (const_terminal_iterator i = other.begin_terminals (); i != other.end_terminals (); ++i) {
      add_terminal (*i);
    }

  }
  return *this;
}

Net::~Net ()
{
  clear ();
}

void Net::clear ()
{
  m_name.clear ();
  m_cluster_id = 0;

  while (! m_terminals.empty ()) {
    erase_terminal (begin_terminals ());
  }

  while (! m_pins.empty ()) {
    erase_pin (begin_pins ());
  }
}

void Net::set_name (const std::string &name)
{
  m_name = name;
}

void Net::set_cluster_id (size_t ci)
{
  m_cluster_id = ci;
}

void Net::add_pin (const NetPinRef &pin)
{
  m_pins.push_back (pin);
  NetPinRef &new_pin = m_pins.back ();
  new_pin.set_net (this);

  if (! pin.subcircuit ()) {
    if (mp_circuit) {
      mp_circuit->set_pin_ref_for_pin (new_pin.pin_id (), --m_pins.end ());
    }
  } else {
    new_pin.subcircuit ()->set_pin_ref_for_pin (new_pin.pin_id (), --m_pins.end ());
  }
}

void Net::erase_pin (pin_iterator iter)
{
  if (iter->subcircuit ()) {
    iter->subcircuit ()->set_pin_ref_for_pin (iter->pin_id (), pin_iterator ());
  } else if (mp_circuit) {
    mp_circuit->set_pin_ref_for_pin (iter->pin_id (), pin_iterator ());
  }
  m_pins.erase (iter);
}

void Net::add_terminal (const NetTerminalRef &terminal)
{
  if (! terminal.device ()) {
    return;
  }

  m_terminals.push_back (terminal);
  NetTerminalRef &new_terminal = m_terminals.back ();
  new_terminal.set_net (this);
  new_terminal.device ()->set_terminal_ref_for_terminal (new_terminal.terminal_id (), --m_terminals.end ());
}

void Net::erase_terminal (terminal_iterator iter)
{
  if (iter->device ()) {
    iter->device ()->set_terminal_ref_for_terminal (iter->terminal_id (), terminal_iterator ());
  }
  m_terminals.erase (iter);
}

void Net::set_circuit (Circuit *circuit)
{
  mp_circuit = circuit;
}

// --------------------------------------------------------------------------------
//  Circuit class implementation

Circuit::Circuit ()
  : mp_netlist (0), m_valid_device_id_table (false), m_valid_subcircuit_id_table (false)
{
  //  .. nothing yet ..
}

Circuit::Circuit (const Circuit &other)
  : mp_netlist (0), m_valid_device_id_table (false), m_valid_subcircuit_id_table (false)
{
  operator= (other);
}

Circuit &Circuit::operator= (const Circuit &other)
{
  if (this != &other) {

    m_name = other.m_name;
    invalidate_device_id_table ();
    invalidate_subcircuit_id_table ();

    for (const_pin_iterator i = other.begin_pins (); i != other.end_pins (); ++i) {
      add_pin (*i);
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
        if (! p->subcircuit ()) {
          n->add_pin (NetPinRef (p->pin_id ()));
        } else {
          std::map<const SubCircuit *, SubCircuit *>::const_iterator m = sc_table.find (p->subcircuit ());
          tl_assert (m != sc_table.end ());
          n->add_pin (NetPinRef (m->second, p->pin_id ()));
        }
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

void Circuit::clear ()
{
  m_name.clear ();
  m_pins.clear ();
  m_devices.clear ();
  m_nets.clear ();
  m_subcircuits.clear ();
  m_device_id_table.clear ();
  m_subcircuit_id_table.clear ();
  m_valid_device_id_table = false;
  m_valid_subcircuit_id_table = false;
}

void Circuit::set_name (const std::string &name)
{
  m_name = name;
}

void Circuit::set_cell_index (const db::cell_index_type ci)
{
  m_cell_index = ci;
}

const Pin &Circuit::add_pin (const Pin &pin)
{
  m_pins.push_back (pin);
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
  size_t id = 0;
  if (! m_devices.empty ()) {
    tl_assert (m_devices.back () != 0);
    id = m_devices.back ()->id ();
  }
  device->set_id (id + 1);

  m_devices.push_back (device);
  invalidate_device_id_table ();
}

void Circuit::remove_device (Device *device)
{
  m_devices.erase (device);
  invalidate_device_id_table ();
}

void Circuit::validate_device_id_table ()
{
  m_device_id_table.clear ();
  for (device_iterator d = begin_devices (); d != end_devices (); ++d) {
    m_device_id_table.insert (std::make_pair (d->id (), d.operator-> ()));
  }

  m_valid_device_id_table = true;
}

void Circuit::invalidate_device_id_table ()
{
  m_valid_device_id_table = false;
  m_device_id_table.clear ();
}

Device *Circuit::device_by_id (size_t id)
{
  if (! m_valid_device_id_table) {
    validate_device_id_table ();
  }

  std::map<size_t, Device *>::const_iterator d = m_device_id_table.find (id);
  return d != m_device_id_table.end () ? d->second : 0;
}

void Circuit::add_subcircuit (SubCircuit *subcircuit)
{
  size_t id = 0;
  if (! m_subcircuits.empty ()) {
    tl_assert (m_subcircuits.back () != 0);
    id = m_subcircuits.back ()->id ();
  }
  subcircuit->set_id (id + 1);

  m_subcircuits.push_back (subcircuit);
  invalidate_subcircuit_id_table ();
}

void Circuit::remove_subcircuit (SubCircuit *subcircuit)
{
  m_subcircuits.erase (subcircuit);
  invalidate_subcircuit_id_table ();
}

void Circuit::validate_subcircuit_id_table ()
{
  m_subcircuit_id_table.clear ();
  for (subcircuit_iterator d = begin_subcircuits (); d != end_subcircuits (); ++d) {
    m_subcircuit_id_table.insert (std::make_pair (d->id (), d.operator-> ()));
  }

  m_valid_subcircuit_id_table = true;
}

void Circuit::invalidate_subcircuit_id_table ()
{
  m_valid_subcircuit_id_table = false;
  m_subcircuit_id_table.clear ();
}

SubCircuit *Circuit::subcircuit_by_id (size_t id)
{
  if (! m_valid_subcircuit_id_table) {
    validate_subcircuit_id_table ();
  }

  std::map<size_t, SubCircuit *>::const_iterator d = m_subcircuit_id_table.find (id);
  return d != m_subcircuit_id_table.end () ? d->second : 0;
}

void Circuit::translate_circuits (const std::map<const Circuit *, Circuit *> &map)
{
  for (subcircuit_iterator i = m_subcircuits.begin (); i != m_subcircuits.end (); ++i) {
    std::map<const Circuit *, Circuit *>::const_iterator m = map.find (i->circuit ());
    tl_assert (m != map.end ());
    i->set_circuit (m->second);
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
  if (d->device_class () != 0) {
    throw tl::Exception (tl::to_string (tr ("Internal error: No device class after removing device in device combination")) + ": name=" + d->name () + ", circuit=" + c->name ());
  }
  const std::vector<db::DeviceTerminalDefinition> &pd = d->device_class ()->terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
    if (d->net_for_terminal (p->id ()) != 0) {
      throw tl::Exception (tl::to_string (tr ("Internal error: Terminal still connected after removing device in device combination")) + ": name=" + d->name () + ", circuit=" + c->name () + ", terminal=" + p->name ());
    }
  }
}

void Circuit::combine_parallel_devices (const db::DeviceClass &cls)
{
  typedef std::vector<const db::Net *> key_type;
  std::map<key_type, std::vector<db::Device *> > combination_candidates;

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
    for (size_t i = 0; i != cl.size () - 1; ++i) {
      for (size_t j = i + 1; j != cl.size (); ) {
        if (cls.combine_devices (cl [i], cl [j])) {
          check_device_before_remove (this, cl [j]);  //  sanity check
          delete cl [j];
          cl.erase (cl.begin () + j);
        } else {
          ++j;
        }
      }
    }

  }
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

void Circuit::combine_serial_devices (const db::DeviceClass &cls)
{
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
      }

    }

  }
}

void Circuit::combine_devices ()
{
  tl_assert (netlist () != 0);

  for (Netlist::device_class_iterator dc = netlist ()->begin_device_classes (); dc != netlist ()->end_device_classes (); ++dc) {
    if (dc->supports_parallel_combination ()) {
      combine_parallel_devices (*dc);
    }
    if (dc->supports_serial_combination ()) {
      combine_serial_devices (*dc);
    }
  }
}

// --------------------------------------------------------------------------------
//  DeviceClass class implementation

DeviceClass::DeviceClass ()
  : mp_netlist (0)
{
  // .. nothing yet ..
}

DeviceClass::DeviceClass (const DeviceClass &other)
  : mp_netlist (0)
{
  operator= (other);
}

DeviceClass &DeviceClass::operator= (const DeviceClass &other)
{
  if (this != &other) {
    m_terminal_definitions = other.m_terminal_definitions;
    m_name = other.m_name;
    m_description = other.m_description;
  }
  return *this;
}

const DeviceTerminalDefinition &DeviceClass::add_terminal_definition (const DeviceTerminalDefinition &pd)
{
  m_terminal_definitions.push_back (pd);
  m_terminal_definitions.back ().set_id (m_terminal_definitions.size () - 1);
  return m_terminal_definitions.back ();
}

void DeviceClass::clear_terminal_definitions ()
{
  m_terminal_definitions.clear ();
}

const DeviceTerminalDefinition *DeviceClass::terminal_definition (size_t id) const
{
  if (id < m_terminal_definitions.size ()) {
    return & m_terminal_definitions [id];
  } else {
    return 0;
  }
}

const DeviceParameterDefinition &DeviceClass::add_parameter_definition (const DeviceParameterDefinition &pd)
{
  m_parameter_definitions.push_back (pd);
  m_parameter_definitions.back ().set_id (m_parameter_definitions.size () - 1);
  return m_parameter_definitions.back ();
}

void DeviceClass::clear_parameter_definitions ()
{
  m_parameter_definitions.clear ();
}

const DeviceParameterDefinition *DeviceClass::parameter_definition (size_t id) const
{
  if (id < m_parameter_definitions.size ()) {
    return & m_parameter_definitions [id];
  } else {
    return 0;
  }
}

bool DeviceClass::has_parameter_with_name (const std::string &name) const
{
  const std::vector<db::DeviceParameterDefinition> &pd = parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    if (i->name () == name) {
      return true;
    }
  }
  return false;
}

size_t DeviceClass::parameter_id_for_name (const std::string &name) const
{
  const std::vector<db::DeviceParameterDefinition> &pd = parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    if (i->name () == name) {
      return i->id ();
    }
  }
  throw tl::Exception (tl::to_string (tr ("Invalid parameter name")) + ": '" + name + "'");
}

bool DeviceClass::has_terminal_with_name (const std::string &name) const
{
  const std::vector<db::DeviceTerminalDefinition> &td = terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator i = td.begin (); i != td.end (); ++i) {
    if (i->name () == name) {
      return true;
    }
  }
  return false;
}

size_t DeviceClass::terminal_id_for_name (const std::string &name) const
{
  const std::vector<db::DeviceTerminalDefinition> &td = terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator i = td.begin (); i != td.end (); ++i) {
    if (i->name () == name) {
      return i->id ();
    }
  }
  throw tl::Exception (tl::to_string (tr ("Invalid terminal name")) + ": '" + name + "'");
}

// --------------------------------------------------------------------------------
//  Netlist class implementation

Netlist::Netlist ()
{
  //  .. nothing yet ..
}

Netlist::Netlist (const Netlist &other)
{
  operator= (other);
}

Netlist &Netlist::operator= (const Netlist &other)
{
  if (this != &other) {

    std::map<const DeviceClass *, DeviceClass *> dct;
    m_device_classes.clear ();
    for (const_device_class_iterator dc = other.begin_device_classes (); dc != other.end_device_classes (); ++dc) {
      DeviceClass *dc_new = dc->clone ();
      dct [dc.operator-> ()] = dc_new;
      m_device_classes.push_back (dc_new);
    }

    std::map<const Circuit *, Circuit *> ct;
    for (const_circuit_iterator i = other.begin_circuits (); i != other.end_circuits (); ++i) {
      Circuit *ct_new = new Circuit (*i);
      ct_new->translate_device_classes (dct);
      ct [i.operator-> ()] = ct_new;
      add_circuit (ct_new);
    }

    for (circuit_iterator i = begin_circuits (); i != end_circuits (); ++i) {
      i->translate_circuits (ct);
    }

  }
  return *this;
}

void Netlist::clear ()
{
  m_device_classes.clear ();
  m_circuits.clear ();
}

void Netlist::add_circuit (Circuit *circuit)
{
  m_circuits.push_back (circuit);
  circuit->set_netlist (this);
}

void Netlist::remove_circuit (Circuit *circuit)
{
  circuit->set_netlist (0);
  m_circuits.erase (circuit);
}

void Netlist::add_device_class (DeviceClass *device_class)
{
  m_device_classes.push_back (device_class);
  device_class->set_netlist (this);
}

void Netlist::remove_device_class (DeviceClass *device_class)
{
  device_class->set_netlist (0);
  m_device_classes.erase (device_class);
}

void Netlist::purge_nets ()
{
  for (circuit_iterator c = begin_circuits (); c != end_circuits (); ++c) {
    c->purge_nets ();
  }
}

void Netlist::combine_devices ()
{
  for (circuit_iterator c = begin_circuits (); c != end_circuits (); ++c) {
    c->combine_devices ();
  }
}

}


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
{
  //  .. nothing yet ..
}

Device::~Device ()
{
  for (std::vector<Net::port_iterator>::const_iterator p = m_port_refs.begin (); p != m_port_refs.end (); ++p) {
    if (*p != Net::port_iterator () && (*p)->net ()) {
      (*p)->net ()->erase_port (*p);
    }
  }
}

Device::Device (DeviceClass *device_class, const std::string &name)
  : m_device_class (device_class), m_name (name)
{
  //  .. nothing yet ..
}

Device::Device (const Device &other)
{
  operator= (other);
}

Device &Device::operator= (const Device &other)
{
  if (this != &other) {
    m_name = other.m_name;
    m_device_class = other.m_device_class;
  }
  return *this;
}

void Device::set_name (const std::string &n)
{
  m_name = n;
}

void Device::set_port_ref_for_port (size_t port_id, Net::port_iterator iter)
{
  if (m_port_refs.size () < port_id + 1) {
    m_port_refs.resize (port_id + 1, Net::port_iterator ());
  }
  m_port_refs [port_id] = iter;
}

const Net *Device::net_for_port (size_t port_id) const
{
  if (port_id < m_port_refs.size ()) {
    Net::port_iterator p = m_port_refs [port_id];
    if (p != Net::port_iterator ()) {
      return p->net ();
    }
  }
  return 0;
}

void Device::connect_port (size_t port_id, Net *net)
{
  if (net_for_port (port_id) == net) {
    return;
  }

  if (port_id < m_port_refs.size ()) {
    Net::port_iterator p = m_port_refs [port_id];
    if (p != Net::port_iterator () && p->net ()) {
      p->net ()->erase_port (p);
    }
    m_port_refs [port_id] = Net::port_iterator ();
  }

  if (net) {
    net->add_port (NetPortRef (this, port_id));
  }
}

// --------------------------------------------------------------------------------
//  SubCircuit class implementation

SubCircuit::SubCircuit ()
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
  : m_circuit (circuit), m_name (name)
{
  //  .. nothing yet ..
}

SubCircuit::SubCircuit (const SubCircuit &other)
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
//  NetPortRef class implementation

NetPortRef::NetPortRef ()
  : m_port_id (0), mp_device (0), mp_net (0)
{
  //  .. nothing yet ..
}

NetPortRef::NetPortRef (Device *device, size_t port_id)
  : m_port_id (port_id), mp_device (device), mp_net (0)
{
  //  .. nothing yet ..
}

NetPortRef::NetPortRef (const NetPortRef &other)
  : m_port_id (other.m_port_id), mp_device (other.mp_device), mp_net (0)
{
  //  .. nothing yet ..
}

NetPortRef &NetPortRef::operator= (const NetPortRef &other)
{
  if (this != &other) {
    m_port_id = other.m_port_id;
    mp_device = other.mp_device;
  }
  return *this;
}

const DevicePortDefinition *
NetPortRef::port_def () const
{
  const DeviceClass *dc = device_class ();
  if (dc) {
    return dc->port_definition (m_port_id);
  } else {
    return 0;
  }
}

const DeviceClass *
NetPortRef::device_class () const
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

const Pin *NetPinRef::pin (const db::Circuit *c) const
{
  if (! mp_subcircuit) {
    tl_assert (c != 0);
    return c->pin_by_id (m_pin_id);
  } else {
    tl_assert (mp_subcircuit->circuit () != 0);
    return mp_subcircuit->circuit ()->pin_by_id (m_pin_id);
  }
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

    for (const_port_iterator i = other.begin_ports (); i != other.end_ports (); ++i) {
      add_port (*i);
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

  while (! m_ports.empty ()) {
    erase_port (begin_ports ());
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

void Net::add_port (const NetPortRef &port)
{
  if (! port.device ()) {
    return;
  }

  m_ports.push_back (port);
  NetPortRef &new_port = m_ports.back ();
  new_port.set_net (this);
  new_port.device ()->set_port_ref_for_port (new_port.port_id (), --m_ports.end ());
}

void Net::erase_port (port_iterator iter)
{
  if (iter->device ()) {
    iter->device ()->set_port_ref_for_port (iter->port_id (), port_iterator ());
  }
  m_ports.erase (iter);
}

void Net::set_circuit (Circuit *circuit)
{
  mp_circuit = circuit;
}

// --------------------------------------------------------------------------------
//  Circuit class implementation

Circuit::Circuit ()
  : mp_netlist (0)
{
  //  .. nothing yet ..
}

Circuit::Circuit (const Circuit &other)
  : mp_netlist (0)
{
  operator= (other);
}

Circuit &Circuit::operator= (const Circuit &other)
{
  if (this != &other) {

    m_name = other.m_name;

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
    for (const_sub_circuit_iterator i = other.begin_sub_circuits (); i != other.end_sub_circuits (); ++i) {
      SubCircuit *sc = new SubCircuit (*i);
      sc_table [i.operator-> ()] = sc;
      add_sub_circuit (sc);
    }

    for (const_net_iterator i = other.begin_nets (); i != other.end_nets (); ++i) {

      //  translate the net
      Net *n = new Net ();
      n->set_cluster_id (i->cluster_id ());
      n->set_name (i->name ());
      add_net (n);

      for (Net::const_port_iterator p = i->begin_ports (); p != i->end_ports (); ++p) {
        std::map<const Device *, Device *>::const_iterator m = device_table.find (p->device ());
        tl_assert (m != device_table.end ());
        n->add_port (NetPortRef (m->second, p->port_id ()));
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
  m_sub_circuits.clear ();
}

void Circuit::set_name (const std::string &name)
{
  m_name = name;
}

void Circuit::set_cell_index (const db::cell_index_type ci)
{
  m_cell_index = ci;
}

void Circuit::add_pin (const Pin &pin)
{
  m_pins.push_back (pin);
  m_pins.back ().set_id (m_pins.size () - 1);
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
  m_devices.push_back (device);
}

void Circuit::remove_device (Device *device)
{
  m_devices.erase (device);
}

void Circuit::add_sub_circuit (SubCircuit *sub_circuit)
{
  m_sub_circuits.push_back (sub_circuit);
}

void Circuit::remove_sub_circuit (SubCircuit *sub_circuit)
{
  m_sub_circuits.erase (sub_circuit);
}

void Circuit::translate_circuits (const std::map<const Circuit *, Circuit *> &map)
{
  for (sub_circuit_iterator i = m_sub_circuits.begin (); i != m_sub_circuits.end (); ++i) {
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

// --------------------------------------------------------------------------------
//  DeviceClass class implementation

DeviceClass::DeviceClass ()
{
  // .. nothing yet ..
}

DeviceClass::DeviceClass (const DeviceClass &other)
{
  operator= (other);
}

DeviceClass &DeviceClass::operator= (const DeviceClass &other)
{
  if (this != &other) {
    m_port_definitions = other.m_port_definitions;
  }
  return *this;
}

const std::string &DeviceClass::name () const
{
  static std::string no_name;
  return no_name;
}

const std::string &DeviceClass::description () const
{
  static std::string no_description;
  return no_description;
}

void DeviceClass::add_port_definition (const DevicePortDefinition &pd)
{
  m_port_definitions.push_back (pd);
  m_port_definitions.back ().set_id (m_port_definitions.size () - 1);
}

void DeviceClass::clear_port_definitions ()
{
  m_port_definitions.clear ();
}

const DevicePortDefinition *DeviceClass::port_definition (size_t id) const
{
  if (id < m_port_definitions.size ()) {
    return & m_port_definitions [id];
  } else {
    return 0;
  }
}

// --------------------------------------------------------------------------------
//  GenericDeviceClass class implementation

GenericDeviceClass::GenericDeviceClass ()
{
  //  .. nothing yet ..
}

GenericDeviceClass::GenericDeviceClass (const GenericDeviceClass &other)
{
  operator= (other);
}

GenericDeviceClass &GenericDeviceClass::operator= (const GenericDeviceClass &other)
{
  if (this != &other) {
    DeviceClass::operator= (other);
    m_name = other.m_name;
    m_description = other.m_description;
  }
  return *this;
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
  m_circuits.erase (circuit);
}

void Netlist::add_device_class (DeviceClass *device_class)
{
  m_device_classes.push_back (device_class);
}

void Netlist::remove_device_class (DeviceClass *device_class)
{
  m_device_classes.erase (device_class);
}

}

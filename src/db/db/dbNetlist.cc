
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

Device::Device (DeviceClass *device_class, const std::string &name)
  : m_device_class (device_class), m_name (name)
{
  //  .. nothing yet ..
}

void Device::set_name (const std::string &n)
{
  m_name = n;
}

// --------------------------------------------------------------------------------
//  SubCircuit class implementation

SubCircuit::SubCircuit ()
{
  //  .. nothing yet ..
}

SubCircuit::SubCircuit (Circuit *circuit)
  : m_circuit (circuit)
{
  //  .. nothing yet ..
}

void SubCircuit::set_name (const std::string &n)
{
  m_name = n;
}

void SubCircuit::set_trans (const db::DCplxTrans &t)
{
  m_trans = t;
}

// --------------------------------------------------------------------------------
//  NetPortRef class implementation

NetPortRef::NetPortRef ()
{
  //  .. nothing yet ..
}

NetPortRef::NetPortRef (Device *device, size_t port_id)
  : m_device (device), m_port_id (port_id)
{
  //  .. nothing yet ..
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
  const Device *device = m_device.get ();
  return device ? device->device_class () : 0;
}

// --------------------------------------------------------------------------------
//  NetPinRef class implementation

NetPinRef::NetPinRef ()
{
  //  .. nothing yet ..
}

NetPinRef::NetPinRef (size_t pin_id)
  : m_pin_id (pin_id)
{
  //  .. nothing yet ..
}

NetPinRef::NetPinRef (size_t pin_id, SubCircuit *circuit)
  : m_pin_id (pin_id), m_subcircuit (circuit)
{
  //  .. nothing yet ..
}

const Pin *NetPinRef::pin (const db::Circuit *c) const
{
  if (! m_subcircuit.get ()) {
    tl_assert (c != 0);
    return c->pin_by_id (m_pin_id);
  } else {
    tl_assert (m_subcircuit->circuit () != 0);
    return m_subcircuit->circuit ()->pin_by_id (m_pin_id);
  }
}

// --------------------------------------------------------------------------------
//  Net class implementation

Net::Net ()
  : m_cluster_id (0)
{
  //  .. nothing yet ..
}

Net::Net (const Net &other)
  : m_cluster_id (0)
{
  operator= (other);
}

Net &Net::operator= (const Net &other)
{
  if (this != &other) {
    m_name = other.m_name;
    m_pins = other.m_pins;
    m_ports = other.m_ports;
    m_cluster_id = other.m_cluster_id;
  }
  return *this;
}

void Net::clear ()
{
  m_name.clear ();
  m_ports.clear ();
  m_pins.clear ();
  m_cluster_id = 0;
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
}

void Net::add_port (const NetPortRef &port)
{
  m_ports.push_back (port);
}

void Net::translate_devices (const std::map<const Device *, Device *> &map)
{
  for (port_list::iterator i = m_ports.begin (); i != m_ports.end (); ++i) {
    std::map<const Device *, Device *>::const_iterator m = map.find (i->device ());
    tl_assert (m != map.end ());
    i->set_device (m->second);
  }
}

void Net::translate_subcircuits (const std::map<const SubCircuit *, SubCircuit *> &map)
{
  for (pin_list::iterator i = m_pins.begin (); i != m_pins.end (); ++i) {
    if (i->subcircuit ()) {
      std::map<const SubCircuit *, SubCircuit *>::const_iterator m = map.find (i->subcircuit ());
      tl_assert (m != map.end ());
      i->set_subcircuit (m->second);
    }
  }
}

// --------------------------------------------------------------------------------
//  Circuit class implementation

Circuit::Circuit ()
{
  //  .. nothing yet ..
}

Circuit::Circuit (const Circuit &other)
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
      Net *n = new Net (*i);
      n->translate_devices (device_table);
      n->translate_subcircuits (sc_table);
      add_net (n);
    }

  }
  return *this;
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

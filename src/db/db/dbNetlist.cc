
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
{
  //  .. nothing yet ..
}

Pin::Pin (Circuit *circuit, const std::string &name)
  : m_circuit (circuit), m_name (name)
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------------------
//  Port class implementation

Port::Port ()
{
  //  .. nothing yet ..
}

Port::Port (Device *device, port_id_type port_id)
  : m_device (device), m_port_id (port_id)
{
  //  .. nothing yet ..
}

const DevicePortDefinition *
Port::port_def () const
{
  const DeviceClass *dc = device_class ();
  if (dc && m_port_id < dc->port_definitions ().size ()) {
    return &dc->port_definitions ()[m_port_id];
  } else {
    return 0;
  }
}

const DeviceClass *
Port::device_class () const
{
  const Device *device = m_device.get ();
  return device ? device->device_class () : 0;
}

// --------------------------------------------------------------------------------
//  Device class implementation

Device::Device (DeviceClass *device_class)
  : m_device_class (device_class)
{
  //  .. nothing yet ..
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

// --------------------------------------------------------------------------------
//  NetPortRef class implementation

NetPortRef::NetPortRef ()
{
  //  .. nothing yet ..
}

NetPortRef::NetPortRef (Port *port)
  : m_port (port)
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------------------
//  NetPinRef class implementation

NetPinRef::NetPinRef ()
{
  //  .. nothing yet ..
}

NetPinRef::NetPinRef (Pin *pin)
  : m_pin (pin)
{
  //  .. nothing yet ..
}

NetPinRef::NetPinRef (Pin *pin, SubCircuit *circuit)
  : m_pin (pin), m_subcircuit (circuit)
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------------------
//  Net class implementation

Net::Net ()
{
  //  .. nothing yet ..
}

Net::Net (const Net &other)
{
  operator= (other);
}

Net &Net::operator= (const Net &other)
{
  if (this != &other) {
    m_name = other.m_name;
    m_pins = other.m_pins;
    m_ports = other.m_ports;
  }
  return *this;
}

void Net::clear ()
{
  m_name.clear ();
  m_ports.clear ();
  m_pins.clear ();
}

void Net::set_name (const std::string &name)
{
  m_name = name;
}

void Net::add_pin (const NetPinRef &pin)
{
  m_pins.push_back (pin);
}

void Net::add_port (const NetPortRef &port)
{
  m_ports.push_back (port);
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
      add_pin (new Pin (*i));
    }
    for (const_device_iterator i = other.begin_devices (); i != other.end_devices (); ++i) {
      add_device (new Device (*i));
    }
    for (const_net_iterator i = other.begin_nets (); i != other.end_nets (); ++i) {
      add_net (new Net (*i));
    }
    for (const_sub_circuit_iterator i = other.begin_sub_circuits (); i != other.end_sub_circuits (); ++i) {
      add_sub_circuit (new SubCircuit (*i));
    }
  }
  return *this;
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

void Circuit::add_pin (Pin *pin)
{
  m_pins.push_back (pin);
}

void Circuit::remove_pin (Pin *pin)
{
  m_pins.erase (pin);
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

// --------------------------------------------------------------------------------
//  DeviceClass class implementation

DeviceClass::DeviceClass ()
{
  // .. nothing yet ..
}

DeviceClass::DeviceClass (const DeviceClass & /*other*/)
{
  // .. nothing yet ..
}

DeviceClass &DeviceClass::operator= (const DeviceClass & /*other*/)
{
  // .. nothing yet ..
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

const std::vector<DevicePortDefinition> &DeviceClass::port_definitions () const
{
  static std::vector<DevicePortDefinition> no_defs;
  return no_defs;
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
    m_port_definitions = other.m_port_definitions;
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
    for (const_circuit_iterator i = other.begin_circuits (); i != other.end_circuits (); ++i) {
      add_circuit (new Circuit (*i));
    }

    m_device_classes.clear ();
    for (const_device_class_iterator dc = other.begin_device_classes (); dc != other.end_device_classes (); ++dc) {
      m_device_classes.push_back (dc->clone ());
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

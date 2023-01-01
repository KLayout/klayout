
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

#include "dbNet.h"
#include "dbDevice.h"
#include "dbDeviceClass.h"
#include "dbCircuit.h"
#include "dbSubCircuit.h"

namespace db
{

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
  : m_pin_id (0), mp_net (0)
{
  //  .. nothing yet ..
}

NetPinRef::NetPinRef (size_t pin_id)
  : m_pin_id (pin_id), mp_net (0)
{
  //  .. nothing yet ..
}

NetPinRef::NetPinRef (const NetPinRef &other)
  : m_pin_id (other.m_pin_id), mp_net (0)
{
  //  .. nothing yet ..
}

NetPinRef &NetPinRef::operator= (const NetPinRef &other)
{
  if (this != &other) {
    m_pin_id = other.m_pin_id;
  }
  return *this;
}

const Pin *NetPinRef::pin () const
{
  if (mp_net && mp_net->circuit ()) {
    return mp_net->circuit ()->pin_by_id (m_pin_id);
  }
  return 0;
}

// --------------------------------------------------------------------------------
//  NetSubcircuitPinRef class implementation

NetSubcircuitPinRef::NetSubcircuitPinRef ()
  : m_pin_id (0), mp_subcircuit (0), mp_net (0)
{
  //  .. nothing yet ..
}

NetSubcircuitPinRef::NetSubcircuitPinRef (SubCircuit *circuit, size_t pin_id)
  : m_pin_id (pin_id), mp_subcircuit (circuit), mp_net (0)
{
  //  .. nothing yet ..
}

NetSubcircuitPinRef::NetSubcircuitPinRef (const NetSubcircuitPinRef &other)
  : m_pin_id (other.m_pin_id), mp_subcircuit (other.mp_subcircuit), mp_net (0)
{
  //  .. nothing yet ..
}

NetSubcircuitPinRef &NetSubcircuitPinRef::operator= (const NetSubcircuitPinRef &other)
{
  if (this != &other) {
    m_pin_id = other.m_pin_id;
    mp_subcircuit = other.mp_subcircuit;
  }
  return *this;
}

const Pin *NetSubcircuitPinRef::pin () const
{
  if (mp_subcircuit && mp_subcircuit->circuit_ref ()) {
    return mp_subcircuit->circuit_ref ()->pin_by_id (m_pin_id);
  }
  return 0;
}

// --------------------------------------------------------------------------------
//  Net class implementation

Net::Net ()
  : NetlistObject (), m_cluster_id (0), mp_circuit (0)
{
  //  .. nothing yet ..
}

Net::Net (const std::string &name)
  : NetlistObject (), m_cluster_id (0), mp_circuit (0)
{
  m_name = name;
}

Net::Net (const Net &other)
  : NetlistObject (other), m_cluster_id (0), mp_circuit (0)
{
  operator= (other);
}

Net &Net::operator= (const Net &other)
{
  if (this != &other) {

    db::NetlistObject::operator= (other);

    clear ();

    m_name = other.m_name;
    m_cluster_id = other.m_cluster_id;

    for (const_subcircuit_pin_iterator i = other.begin_subcircuit_pins (); i != other.end_subcircuit_pins (); ++i) {
      add_subcircuit_pin (*i);
    }

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

Netlist *Net::netlist ()
{
  return mp_circuit ? mp_circuit->netlist () : 0;
}

const Netlist *Net::netlist () const
{
  return mp_circuit ? mp_circuit->netlist () : 0;
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

  while (! m_subcircuit_pins.empty ()) {
    erase_subcircuit_pin (begin_subcircuit_pins ());
  }
}

void Net::set_name (const std::string &name)
{
  m_name = name;
  if (mp_circuit) {
    mp_circuit->m_net_by_name.invalidate ();
  }
}

std::string Net::qname () const
{
  if (circuit ()) {
    return circuit ()->name () + ":" + expanded_name ();
  } else {
    return expanded_name ();
  }
}

std::string Net::expanded_name () const
{
  if (name ().empty ()) {
    if (cluster_id () > std::numeric_limits<size_t>::max () / 2) {
      //  avoid printing huge ID numbers for internal cluster IDs
      return "$I" + tl::to_string ((std::numeric_limits<size_t>::max () - cluster_id ()) + 1);
    } else {
      return "$" + tl::to_string (cluster_id ());
    }
  } else {
    return name ();
  }
}

void Net::set_cluster_id (size_t ci)
{
  m_cluster_id = ci;
  if (mp_circuit) {
    mp_circuit->m_net_by_cluster_id.invalidate ();
  }
}

void Net::add_pin (const NetPinRef &pin)
{
  m_pins.push_back (pin);
  NetPinRef &new_pin = m_pins.back ();
  new_pin.set_net (this);

  if (mp_circuit) {
    mp_circuit->set_pin_ref_for_pin (new_pin.pin_id (), --m_pins.end ());
  }
}

void Net::add_subcircuit_pin (const NetSubcircuitPinRef &pin)
{
  m_subcircuit_pins.push_back (pin);
  NetSubcircuitPinRef &new_pin = m_subcircuit_pins.back ();
  new_pin.set_net (this);

  tl_assert (pin.subcircuit () != 0);
  new_pin.subcircuit ()->set_pin_ref_for_pin (new_pin.pin_id (), --m_subcircuit_pins.end ());
}

void Net::erase_pin (pin_iterator iter)
{
  if (mp_circuit) {
    mp_circuit->set_pin_ref_for_pin (iter->pin_id (), pin_iterator ());
  }
  m_pins.erase (iter);
}

void Net::erase_subcircuit_pin (subcircuit_pin_iterator iter)
{
  if (iter->subcircuit ()) {
    iter->subcircuit ()->set_pin_ref_for_pin (iter->pin_id (), subcircuit_pin_iterator ());
  }
  m_subcircuit_pins.erase (iter);
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

}

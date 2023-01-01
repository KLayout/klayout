
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

#include "dbDevice.h"
#include "dbCircuit.h"
#include "dbDeviceClass.h"
#include "tlIteratorUtils.h"

namespace db
{

// --------------------------------------------------------------------------------
//  Device class implementation

Device::Device ()
  : db::NetlistObject (), mp_device_class (0), mp_device_abstract (0), m_id (0), mp_circuit (0)
{
  //  .. nothing yet ..
}

Device::~Device ()
{
  for (std::vector<Net::terminal_iterator>::const_iterator t = m_terminal_refs.begin (); t != m_terminal_refs.end (); ++t) {
    if (! tl::is_null_iterator (*t) && (*t)->net ()) {
      (*t)->net ()->erase_terminal (*t);
    }
  }
}

Device::Device (DeviceClass *device_class, const std::string &name)
  : db::NetlistObject (), mp_device_class (device_class), mp_device_abstract (0), m_name (name), m_id (0), mp_circuit (0)
{
  //  .. nothing yet ..
}

Device::Device (DeviceClass *device_class, DeviceAbstract *device_abstract, const std::string &name)
  : db::NetlistObject (), mp_device_class (device_class), mp_device_abstract (device_abstract), m_name (name), m_id (0), mp_circuit (0)
{
  //  .. nothing yet ..
}

Device::Device (const Device &other)
  : db::NetlistObject (other), mp_device_class (0), mp_device_abstract (0), m_id (0), mp_circuit (0)
{
  operator= (other);
}

Device &Device::operator= (const Device &other)
{
  if (this != &other) {
    db::NetlistObject::operator= (other);
    m_name = other.m_name;
    m_trans = other.m_trans;
    m_parameters = other.m_parameters;
    mp_device_class = other.mp_device_class;
    mp_device_abstract = other.mp_device_abstract;
  }
  return *this;
}

std::string Device::expanded_name () const
{
  if (name ().empty ()) {
    return "$" + tl::to_string (id ());
  } else {
    return name ();
  }
}

void Device::set_circuit (Circuit *circuit)
{
  mp_circuit = circuit;
}

const Netlist *Device::netlist () const
{
  return mp_circuit ? mp_circuit->netlist () : 0;
}

Netlist *Device::netlist ()
{
  return mp_circuit ? mp_circuit->netlist () : 0;
}

void Device::set_name (const std::string &n)
{
  m_name = n;
  if (mp_circuit) {
    mp_circuit->m_device_by_name.invalidate ();
  }
}

void Device::set_trans (const db::DCplxTrans &tr)
{
  m_trans = tr;
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
    if (! tl::is_null_iterator (p)) {
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
    if (! tl::is_null_iterator (p) && p->net ()) {
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

void Device::add_others_terminals (unsigned int this_terminal, db::Device *other, unsigned int other_terminal)
{
  std::vector<DeviceReconnectedTerminal> &terminals = m_reconnected_terminals [this_terminal];

  std::map<unsigned int, std::vector<DeviceReconnectedTerminal> >::const_iterator ot = other->m_reconnected_terminals.find (other_terminal);
  if (ot == other->m_reconnected_terminals.end ()) {

    terminals.push_back (DeviceReconnectedTerminal (other_abstracts ().size () + 1, other_terminal));

  } else {

    size_t n = terminals.size ();
    terminals.insert (terminals.end (), ot->second.begin (), ot->second.end ());

    while (n < terminals.size ()) {
      terminals [n].device_index += other_abstracts ().size () + 1;
      ++n;
    }

  }
}

void Device::init_terminal_routes ()
{
  if (! device_class ()) {
    return;
  }

  size_t n = device_class ()->terminal_definitions ().size ();
  for (size_t i = 0; i < n; ++i) {
    m_reconnected_terminals [(unsigned int) i].push_back (DeviceReconnectedTerminal (0, (unsigned int) i));
  }
}

void Device::join_terminals (unsigned int this_terminal, db::Device *other, unsigned int other_terminal)
{
  if (m_reconnected_terminals.empty ()) {
    init_terminal_routes ();
  }

  other->connect_terminal (other_terminal, 0);

  add_others_terminals (this_terminal, other, other_terminal);
}

void Device::reroute_terminal (unsigned int this_terminal, db::Device *other, unsigned int from_other_terminal, unsigned int other_terminal)
{
  //  TODO: the internal connection is not represented currently ...

  if (m_reconnected_terminals.empty ()) {
    init_terminal_routes ();
  }

  if (! m_reconnected_terminals.empty ()) {
    m_reconnected_terminals.erase (this_terminal);
  }

  add_others_terminals (this_terminal, other, other_terminal);

  connect_terminal (this_terminal, other->net_for_terminal (other_terminal));

  other->connect_terminal (from_other_terminal, 0);
  other->connect_terminal (other_terminal, 0);
}

void Device::join_device (db::Device *other)
{
  db::DCplxTrans d = trans ().inverted () * other->trans ();

  m_other_abstracts.reserve (m_other_abstracts.size () + 1 + other->m_other_abstracts.size ());

  m_other_abstracts.push_back (db::DeviceAbstractRef (other->device_abstract (), d));

  for (std::vector<db::DeviceAbstractRef>::const_iterator a = other->m_other_abstracts.begin (); a != other->m_other_abstracts.end (); ++a) {
    m_other_abstracts.push_back (*a);
    m_other_abstracts.back ().trans = d * m_other_abstracts.back ().trans;
  }
}

static db::DeviceAbstract *map_da (const std::map<const DeviceAbstract *, DeviceAbstract *> &map, const db::DeviceAbstract *da)
{
  if (! da) {
    return 0;
  } else {
    std::map<const DeviceAbstract *, DeviceAbstract *>::const_iterator m = map.find (da);
    tl_assert (m != map.end ());
    return m->second;
  }
}

void Device::translate_device_abstracts (const std::map<const DeviceAbstract *, DeviceAbstract *> &map)
{
  set_device_abstract (map_da (map, device_abstract ()));

  for (std::vector<db::DeviceAbstractRef>::iterator a = m_other_abstracts.begin (); a != m_other_abstracts.end (); ++a) {
    a->device_abstract = map_da (map, a->device_abstract);
  }
}

}

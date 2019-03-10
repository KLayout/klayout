
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

#include "dbDevice.h"
#include "dbCircuit.h"
#include "dbDeviceClass.h"

namespace db
{

// --------------------------------------------------------------------------------
//  Device class implementation

Device::Device ()
  : mp_device_class (0), mp_device_abstract (0), m_id (0), mp_circuit (0)
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
  : mp_device_class (device_class), mp_device_abstract (0), m_name (name), m_id (0), mp_circuit (0)
{
  //  .. nothing yet ..
}

Device::Device (DeviceClass *device_class, DeviceAbstract *device_abstract, const std::string &name)
  : mp_device_class (device_class), mp_device_abstract (device_abstract), m_name (name), m_id (0), mp_circuit (0)
{
  //  .. nothing yet ..
}

Device::Device (const Device &other)
  : tl::Object (other), mp_device_class (0), mp_device_abstract (0), m_id (0), mp_circuit (0)
{
  operator= (other);
}

Device &Device::operator= (const Device &other)
{
  if (this != &other) {
    m_name = other.m_name;
    m_position = other.m_position;
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

void Device::set_name (const std::string &n)
{
  m_name = n;
  if (mp_circuit) {
    mp_circuit->m_device_by_name.invalidate ();
  }
}

void Device::set_position (const db::DPoint &pt)
{
  m_position = pt;
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

}

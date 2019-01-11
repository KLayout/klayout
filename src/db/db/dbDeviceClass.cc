
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

#include "dbDeviceClass.h"

namespace db
{

// --------------------------------------------------------------------------------
//  DeviceClass class implementation

DeviceClass::DeviceClass ()
  : mp_netlist (0)
{
  // .. nothing yet ..
}

DeviceClass::DeviceClass (const DeviceClass &other)
  : gsi::ObjectBase (other), tl::Object (other), tl::UniqueId (other), mp_netlist (0)
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

}

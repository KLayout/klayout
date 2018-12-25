
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


#include "gsiDecl.h"
#include "dbNetlistProperty.h"

namespace gsi
{

// ---------------------------------------------------------------
//  db::NetlistProperty binding

static db::NetlistProperty *new_property ()
{
  return new db::NetlistProperty ();
}

gsi::Class<db::NetlistProperty> decl_NetlistProperty ("db", "NetlistProperty",
  gsi::constructor ("new", &new_property,
    "@brief Creates a plain netlist property"
  ) +
  gsi::method ("to_s", &db::NetlistProperty::to_string,
    "@brief Convert the property to a string.\n"
    "@return The string representing this property\n"
  ),
  "@brief A generic base class for netlist properties.\n"
  "\n"
  "Netlist properties are used to annotate shapes or other objects with net properties. "
  "Netlist properties are net names or device terminals. "
  "Netlist properties can be stored inside property sets. "
  "This class provides the base class for such netlist properties."
  "\n\n"
  "This class was introduced in version 0.26.\n"
);

// ---------------------------------------------------------------
//  db::NetNameProperty binding

static db::NetNameProperty *new_netname ()
{
  return new db::NetNameProperty ();
}

static db::NetNameProperty *new_netname2 (const std::string &n)
{
  return new db::NetNameProperty (n);
}

gsi::Class<db::NetNameProperty> decl_NetNameProperty (decl_NetlistProperty, "db", "NetNameProperty",
  gsi::constructor ("new", &new_netname,
    "@brief Creates a new net name property object without a specific name"
  ) +
  gsi::constructor ("new", &new_netname2, gsi::arg ("name"),
    "@brief Creates a new net name property object with the given name"
  ) +
  gsi::method ("name=", &db::NetNameProperty::set_name, gsi::arg ("n"),
    "@brief Sets the name\n"
  ) +
  gsi::method ("name", &db::NetNameProperty::name,
    "@brief Gets the name\n"
  ),
  "@brief A net name property.\n"
  "\n"
  "The netlist property annotates a shape or other object with a net name."
  "\n\n"
  "This class was introduced in version 0.26.\n"
);

// ---------------------------------------------------------------
//  db::DeviceTerminalProperty binding

static db::DeviceTerminalProperty *new_devterminal ()
{
  return new db::DeviceTerminalProperty ();
}

static db::DeviceTerminalProperty *new_devterminal2 (const db::NetTerminalRef &n)
{
  return new db::DeviceTerminalProperty (n);
}

gsi::Class<db::DeviceTerminalProperty> decl_DeviceTerminalProperty (decl_NetlistProperty, "db", "DeviceTerminalProperty",
  gsi::constructor ("new", &new_devterminal,
    "@brief Creates a new device terminal property"
  ) +
  gsi::constructor ("new", &new_devterminal2, gsi::arg ("terminal_ref"),
    "@brief Creates a new device terminal property with the given terminal reference"
  ) +
  gsi::method ("terminal_ref=", &db::DeviceTerminalProperty::set_terminal_ref, gsi::arg ("p"),
    "@brief Sets the terminal reference\n"
  ) +
  gsi::method ("terminal_ref", &db::DeviceTerminalProperty::terminal_ref,
    "@brief Gets the terminal reference\n"
  ),
  "@brief A device terminal reference property.\n"
  "\n"
  "The netlist property annotates a shape or other object with a reference to a device terminal."
  "\n\n"
  "This class was introduced in version 0.26.\n"
);

}

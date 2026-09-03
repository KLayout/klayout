
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
#include "dbLayoutConnectivity.h"
#include "dbPropertiesRepository.h"

namespace gsi
{

static db::LayoutPin *make_layout_pin (const std::string &name, bool must_connect)
{
  return new db::LayoutPin (name, must_connect);
}

static tl::Variant layout_pin_property_name ()
{
  return db::property_name (db::pin_property_name_id);
}

static db::LayoutPin *layout_pin_from_string (const std::string &s)
{
  std::unique_ptr<db::LayoutPin> obj (new db::LayoutPin ());
  tl::Extractor ex (s.c_str ());
  if (obj->parse (ex)) {
    return obj.release ();
  } else {
    return 0;
  }
}

Class<db::LayoutPin> decl_LayoutPin ("db", "LayoutPin",
  constructor ("new", &make_layout_pin, gsi::arg ("name"), gsi::arg ("must_connect", false),
    "@brief Creates LayoutPin information with the given name and 'must_connect' flag\n"
  ) +
  constructor ("from_string", &layout_pin_from_string, gsi::arg ("string"),
    "@brief Creates LayoutPin information from a string\n"
  ) +
  method ("PinPropertyName", &layout_pin_property_name,
    "@brief Gets the name (key) of the property to store the pin information with a shape."
  ) +
  method ("name=", &db::LayoutPin::set_name, gsi::arg ("name"),
    "@brief Sets the pins name\n"
  ) +
  method ("name", &db::LayoutPin::name,
    "@brief Gets the pins name\n"
  ) +
  method ("must_connect=", &db::LayoutPin::set_must_connect, gsi::arg ("must_connect"),
    "@brief Sets the 'must connect' flag\n"
    "See the class description for an explanation of this attribute."
  ) +
  method ("must_connect", &db::LayoutPin::must_connect,
    "@brief Gets the 'must connect' flag\n"
    "See the class description for an explanation of this attribute."
  ) +
  method ("to_string", &db::LayoutPin::to_string,
    "@brief Converts the LayoutPin object to a string\n"
    "The string returned by this method can be used to recreate the object with \\from_string."
  ),
  "@brief Pin information stored in a shape's properties\n"
  "\n"
  "This object is used to encode pin information for storage in the shape's properties. "
  "Adding this information to the shape properties (with name \\PinPropertyName) makes the "
  "shape a pin. Pins are used to indicate where connections are supposed to be made "
  "to subcircuits or devices.\n"
  "\n"
  "The 'must_connect' attribute indicates that all pins with the same name "
  "need to be connected. If this attribute is false on all the pin shapes, "
  "a single connection is enough.\n"
  "\n"
  "This class has been introduced in version 0.31.0."
);

static tl::Variant layout_instance_connections_property_name ()
{
  return db::property_name (db::instance_connections_property_name_id);
}

static db::LayoutInstanceConnections *layout_instance_connections_from_string (const std::string &s)
{
  std::unique_ptr<db::LayoutInstanceConnections> obj (new db::LayoutInstanceConnections ());
  tl::Extractor ex (s.c_str ());
  if (obj->parse (ex)) {
    return obj.release ();
  } else {
    return 0;
  }
}

Class<db::LayoutInstanceConnections> decl_LayoutInstanceConnections ("db", "LayoutInstanceConnections",
  constructor ("from_string", &layout_instance_connections_from_string, gsi::arg ("string"),
    "@brief Creates LayoutPin information from a string\n"
  ) +
  method ("LayoutInstanceConnectionsPropertyName", &layout_instance_connections_property_name,
    "@brief Gets the name (key) of the property to store the instance connection information with an instance."
  ) +
  method ("clear", &db::LayoutInstanceConnections::clear,
    "@brief Clears the connection information\n"
  ) +
  method ("add_connection", static_cast<void (db::LayoutInstanceConnections::*) (const std::string &, const std::string &)> (&db::LayoutInstanceConnections::add_connection), gsi::arg ("pin_name"), gsi::arg ("net"),
    "@brief Adds a connection for the given pin to the given net\n"
  ) +
  method ("has_pin", static_cast<bool (db::LayoutInstanceConnections::*) (const std::string &) const> (&db::LayoutInstanceConnections::has_pin), gsi::arg ("pin_name"),
    "@brief Gets a value indicating whether a connection is available for the given pin\n"
  ) +
  method ("net_for_pin", &db::LayoutInstanceConnections::net_for_pin, gsi::arg ("pin_name"),
    "@brief Gets name of the net connected to the given pin\n"
    "If no net is attached to the given pin, nil is returned."
  ) +
  method ("to_string", &db::LayoutInstanceConnections::to_string,
    "@brief Converts the LayoutInstanceConnections object to a string\n"
    "The string returned by this method can be used to recreate the object with \\from_string."
  ),
  "@brief Connection information stored along with an instance\n"
  "\n"
  "This object is used to encode instance connection information and is stored as a property\n"
  "with name \\LayoutInstanceConnectionsPropertyName with the instance object.\n"
  "The object registers which net is connected to which pin of the instance. The pin names "
  "should correspond to pins inside the cell that the instance refers to.\n"
  "\n"
  "This class has been introduced in version 0.31.0."
);

}

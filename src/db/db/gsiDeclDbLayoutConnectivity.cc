
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

Class<db::LayoutPin> decl_LayoutPin ("db", "LayoutPin",
  constructor ("new", &make_layout_pin, gsi::arg ("name"), gsi::arg ("must_connect", false),
    "@brief Creates LayoutPin information with the given name and 'must_connect' flag\n"
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
  method ("name", &db::LayoutPin::name,
    "@brief Gets the 'must connect' flag\n"
    "See the class description for an explanation of this attribute."
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


}

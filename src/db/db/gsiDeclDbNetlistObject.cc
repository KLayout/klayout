
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
#include "dbNetlist.h"

namespace gsi
{

static std::vector<tl::Variant> property_keys (const db::NetlistObject *object)
{
  std::vector<tl::Variant> v;
  for (db::NetlistObject::property_iterator p = object->begin_properties (); p != object->end_properties (); ++p) {
    v.push_back (p->first);
  }
  return v;
}

Class<db::NetlistObject> decl_dbNetlistObject ("db", "NetlistObject",
  gsi::method ("property", &db::NetlistObject::property, gsi::arg ("key"),
    "@brief Gets the property value for the given key or nil if there is no value with this key."
  ) +
  gsi::method ("set_property", &db::NetlistObject::set_property, gsi::arg ("key"), gsi::arg ("value"),
    "@brief Sets the property value for the given key.\n"
    "Use a nil value to erase the property with this key."
  ) +
  gsi::method_ext ("property_keys", &property_keys,
    "@brief Gets the keys for the properties stored in this object."
  ),
  "@brief The base class for some netlist objects.\n"
  "The main purpose of this class is to supply user properties for netlist objects.\n"
  "\n"
  "This class has been introduced in version 0.26.2"
);

}


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

static void subcircuit_connect_pin1 (db::SubCircuit *subcircuit, const db::Pin *pin, db::Net *net)
{
  if (pin) {
    subcircuit->connect_pin (pin->id (), net);
  }
}

static void subcircuit_disconnect_pin  (db::SubCircuit *subcircuit, size_t pin_id)
{
  subcircuit->connect_pin (pin_id, 0);
}

static void subcircuit_disconnect_pin1 (db::SubCircuit *subcircuit, const db::Pin *pin)
{
  if (pin) {
    subcircuit->connect_pin (pin->id (), 0);
  }
}

extern Class<db::NetlistObject> decl_dbNetlistObject;

Class<db::SubCircuit> decl_dbSubCircuit (decl_dbNetlistObject, "db", "SubCircuit",
  gsi::method ("circuit_ref", (const db::Circuit *(db::SubCircuit::*) () const) &db::SubCircuit::circuit_ref,
    "@brief Gets the circuit referenced by the subcircuit.\n"
  ) +
  gsi::method ("circuit_ref", (db::Circuit *(db::SubCircuit::*) ()) &db::SubCircuit::circuit_ref,
    "@brief Gets the circuit referenced by the subcircuit (non-const version).\n"
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("circuit", (const db::Circuit *(db::SubCircuit::*) () const) &db::SubCircuit::circuit,
    "@brief Gets the circuit the subcircuit lives in.\n"
    "This is NOT the circuit which is referenced. For getting the circuit that the subcircuit references, use \\circuit_ref."
  ) +
  gsi::method ("circuit", (db::Circuit *(db::SubCircuit::*) ()) &db::SubCircuit::circuit,
    "@brief Gets the circuit the subcircuit lives in (non-const version).\n"
    "This is NOT the circuit which is referenced. For getting the circuit that the subcircuit references, use \\circuit_ref."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("id", &db::SubCircuit::id,
    "@brief Gets the subcircuit ID.\n"
    "The ID is a unique integer which identifies the subcircuit.\n"
    "It can be used to retrieve the subcircuit from the circuit using \\Circuit#subcircuit_by_id.\n"
    "When assigned, the subcircuit ID is not 0.\n"
  ) +
  gsi::method ("name=", &db::SubCircuit::set_name, gsi::arg ("name"),
    "@brief Sets the name of the subcircuit.\n"
    "SubCircuit names are used to name a subcircuits inside a netlist file. "
    "SubCircuit names should be unique within a circuit."
  ) +
  gsi::method ("name", &db::SubCircuit::name,
    "@brief Gets the name of the subcircuit.\n"
  ) +
  gsi::method ("expanded_name", &db::SubCircuit::expanded_name,
    "@brief Gets the expanded name of the subcircuit.\n"
    "The expanded name takes the name of the subcircuit. If the name is empty, the numeric ID will be used to build a name. "
  ) +
  gsi::method ("net_for_pin", (const db::Net *(db::SubCircuit::*) (size_t) const) &db::SubCircuit::net_for_pin, gsi::arg ("pin_id"),
    "@brief Gets the net connected to the specified pin of the subcircuit.\n"
    "If the pin is not connected, nil is returned for the net."
  ) +
  gsi::method ("net_for_pin", (db::Net *(db::SubCircuit::*) (size_t)) &db::SubCircuit::net_for_pin, gsi::arg ("pin_id"),
    "@brief Gets the net connected to the specified pin of the subcircuit (non-const version).\n"
    "If the pin is not connected, nil is returned for the net."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("connect_pin", &db::SubCircuit::connect_pin, gsi::arg ("pin_id"), gsi::arg ("net"),
    "@brief Connects the given pin to the specified net.\n"
  ) +
  gsi::method_ext ("connect_pin", &gsi::subcircuit_connect_pin1, gsi::arg ("pin"), gsi::arg ("net"),
    "@brief Connects the given pin to the specified net.\n"
    "This version takes a \\Pin reference instead of a pin ID."
  ) +
  gsi::method_ext ("disconnect_pin", &gsi::subcircuit_disconnect_pin, gsi::arg ("pin_id"),
    "@brief Disconnects the given pin from any net.\n"
  ) +
  gsi::method_ext ("disconnect_pin", &gsi::subcircuit_disconnect_pin1, gsi::arg ("pin"),
    "@brief Disconnects the given pin from any net.\n"
    "This version takes a \\Pin reference instead of a pin ID."
  ) +
  gsi::method ("trans", &db::SubCircuit::trans,
    "@brief Gets the physical transformation for the subcircuit.\n"
    "\n"
    "This property applies to subcircuits derived from a layout. It specifies the "
    "placement of the respective cell.\n"
    "\n"
    "This property has been introduced in version 0.27."
  ) +
  gsi::method ("trans=", &db::SubCircuit::set_trans, gsi::arg ("trans"),
    "@brief Sets the physical transformation for the subcircuit.\n"
    "\n"
    "See \\trans for details about this property.\n"
    "\n"
    "This property has been introduced in version 0.27."
  ),
  "@brief A subcircuit inside a circuit.\n"
  "Circuits may instantiate other circuits as subcircuits similar to cells "
  "in layouts. Such an instance is a subcircuit. A subcircuit refers to a "
  "circuit implementation (a \\Circuit object), and presents connections through "
  "pins. The pins of a subcircuit can be connected to nets. The subcircuit pins "
  "are identical to the outgoing pins of the circuit the subcircuit refers to.\n"
  "\n"
  "Subcircuits connect to nets through the \\SubCircuit#connect_pin method. "
  "SubCircuit pins can be disconnected using \\SubCircuit#disconnect_pin.\n"
  "\n"
  "Subcircuit objects are created inside a circuit with \\Circuit#create_subcircuit.\n"
  "\n"
  "This class has been added in version 0.26."
);

}


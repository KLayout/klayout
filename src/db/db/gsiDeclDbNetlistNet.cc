
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

Class<db::NetTerminalRef> decl_dbNetTerminalRef ("db", "NetTerminalRef",
  gsi::method ("terminal_id", &db::NetTerminalRef::terminal_id,
    "@brief Gets the ID of the terminal of the device the connection is made to."
  ) +
  gsi::method ("device", (const db::Device *(db::NetTerminalRef::*) () const) &db::NetTerminalRef::device,
    "@brief Gets the device reference.\n"
    "Gets the device object that this connection is made to."
  ) +
  gsi::method ("device", (db::Device *(db::NetTerminalRef::*) ()) &db::NetTerminalRef::device,
    "@brief Gets the device reference (non-const version).\n"
    "Gets the device object that this connection is made to."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("net", (const db::Net *(db::NetTerminalRef::*) () const) &db::NetTerminalRef::net,
    "@brief Gets the net this terminal reference is attached to."
  ) +
  gsi::method ("net", (db::Net *(db::NetTerminalRef::*) ()) &db::NetTerminalRef::net,
    "@brief Gets the net this terminal reference is attached to (non-const version)."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("device_class", (const db::DeviceClass *(db::NetTerminalRef::*) () const) &db::NetTerminalRef::device_class,
    "@brief Gets the class of the device which is addressed."
  ) +
  gsi::method ("terminal_def", (const db::DeviceTerminalDefinition *(db::NetTerminalRef::*) () const) &db::NetTerminalRef::terminal_def,
    "@brief Gets the terminal definition of the terminal that is connected"
  ),
  "@brief A connection to a terminal of a device.\n"
  "This object is used inside a net (see \\Net) to describe the connections a net makes.\n"
  "\n"
  "This class has been added in version 0.26."
);

Class<db::NetPinRef> decl_dbNetPinRef ("db", "NetPinRef",
  gsi::method ("pin_id", &db::NetPinRef::pin_id,
    "@brief Gets the ID of the pin the connection is made to."
  ) +
  gsi::method ("pin", &db::NetPinRef::pin,
    "@brief Gets the \\Pin object of the pin the connection is made to."
  ) +
  gsi::method ("net", (const db::Net *(db::NetPinRef::*) () const) &db::NetPinRef::net,
    "@brief Gets the net this pin reference is attached to."
  ) +
  gsi::method ("net", (db::Net *(db::NetPinRef::*) ()) &db::NetPinRef::net,
    "@brief Gets the net this pin reference is attached to (non-const version)."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ),
  "@brief A connection to an outgoing pin of the circuit.\n"
  "This object is used inside a net (see \\Net) to describe the connections a net makes.\n"
  "\n"
  "This class has been added in version 0.26."
);

Class<db::NetSubcircuitPinRef> decl_dbNetSubcircuitPinRef ("db", "NetSubcircuitPinRef",
  gsi::method ("pin_id", &db::NetSubcircuitPinRef::pin_id,
    "@brief Gets the ID of the pin the connection is made to."
  ) +
  gsi::method ("pin", &db::NetSubcircuitPinRef::pin,
    "@brief Gets the \\Pin object of the pin the connection is made to."
  ) +
  gsi::method ("subcircuit", (const db::SubCircuit *(db::NetSubcircuitPinRef::*) () const) &db::NetSubcircuitPinRef::subcircuit,
    "@brief Gets the subcircuit reference.\n"
    "This attribute indicates the subcircuit the net attaches to. The "
    "subcircuit lives in the same circuit than the net. "
  ) +
  gsi::method ("subcircuit", (db::SubCircuit *(db::NetSubcircuitPinRef::*) ()) &db::NetSubcircuitPinRef::subcircuit,
    "@brief Gets the subcircuit reference (non-const version).\n"
    "This attribute indicates the subcircuit the net attaches to. The "
    "subcircuit lives in the same circuit than the net. "
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("net", (const db::Net *(db::NetSubcircuitPinRef::*) () const) &db::NetSubcircuitPinRef::net,
    "@brief Gets the net this pin reference is attached to."
  ) +
  gsi::method ("net", (db::Net *(db::NetSubcircuitPinRef::*) ()) &db::NetSubcircuitPinRef::net,
    "@brief Gets the net this pin reference is attached to (non-const version)."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ),
  "@brief A connection to a pin of a subcircuit.\n"
  "This object is used inside a net (see \\Net) to describe the connections a net makes.\n"
  "\n"
  "This class has been added in version 0.26."
);

extern Class<db::NetlistObject> decl_dbNetlistObject;

Class<db::Net> decl_dbNet (decl_dbNetlistObject, "db", "Net",
  gsi::method ("circuit", (db::Circuit *(db::Net::*) ()) &db::Net::circuit,
    "@brief Gets the circuit the net lives in."
  ) +
  gsi::method ("clear", &db::Net::clear,
    "@brief Clears the net."
  ) +
  gsi::method ("name=", &db::Net::set_name, gsi::arg ("name"),
    "@brief Sets the name of the net.\n"
    "The name of the net is used for naming the net in schematic files for example. "
    "The name of the net has to be unique."
  ) +
  gsi::method ("name", &db::Net::name,
    "@brief Gets the name of the net.\n"
    "See \\name= for details about the name."
  ) +
  gsi::method ("qname|to_s", &db::Net::qname,
    "@brief Gets the qualified name.\n"
    "The qualified name is like the expanded name, but the circuit's name is preceded\n"
    "(i.e. 'CIRCUIT:NET') if available.\n"
  ) +
  gsi::method ("expanded_name", &db::Net::expanded_name,
    "@brief Gets the expanded name of the net.\n"
    "The expanded name takes the name of the net. If the name is empty, the cluster ID will be used to build a name. "
  ) +
  gsi::method ("cluster_id=", &db::Net::set_cluster_id, gsi::arg ("id"),
    "@brief Sets the cluster ID of the net.\n"
    "The cluster ID connects the net with a layout cluster. It is set when "
    "the net is extracted from a layout."
  ) +
  gsi::method ("cluster_id", &db::Net::cluster_id,
    "@brief Gets the cluster ID of the net.\n"
    "See \\cluster_id= for details about the cluster ID."
  ) +
  gsi::iterator ("each_pin", gsi::return_reference (), (db::Net::const_pin_iterator (db::Net::*) () const) &db::Net::begin_pins, (db::Net::const_pin_iterator (db::Net::*) () const) &db::Net::end_pins,
    "@brief Iterates over all outgoing pins the net connects.\n"
    "Pin connections are described by \\NetPinRef objects. Pin connections "
    "are connections to outgoing pins of the circuit the net lives in."
  ) +
  gsi::iterator ("each_pin", gsi::return_reference (), (db::Net::pin_iterator (db::Net::*) ()) &db::Net::begin_pins, (db::Net::pin_iterator (db::Net::*) ()) &db::Net::end_pins,
    "@brief Iterates over all outgoing pins the net connects (non-const version).\n"
    "Pin connections are described by \\NetPinRef objects. Pin connections "
    "are connections to outgoing pins of the circuit the net lives in."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::iterator ("each_subcircuit_pin", gsi::return_reference (), (db::Net::const_subcircuit_pin_iterator (db::Net::*) () const) &db::Net::begin_subcircuit_pins, (db::Net::const_subcircuit_pin_iterator (db::Net::*) () const) &db::Net::end_subcircuit_pins,
    "@brief Iterates over all subcircuit pins the net connects.\n"
    "Subcircuit pin connections are described by \\NetSubcircuitPinRef objects. These are "
    "connections to specific pins of subcircuits."
  ) +
  gsi::iterator ("each_subcircuit_pin", gsi::return_reference (), (db::Net::subcircuit_pin_iterator (db::Net::*) ()) &db::Net::begin_subcircuit_pins, (db::Net::subcircuit_pin_iterator (db::Net::*) ()) &db::Net::end_subcircuit_pins,
    "@brief Iterates over all subcircuit pins the net connects (non-const version).\n"
    "Subcircuit pin connections are described by \\NetSubcircuitPinRef objects. These are "
    "connections to specific pins of subcircuits."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::iterator ("each_terminal", gsi::return_reference (), (db::Net::const_terminal_iterator (db::Net::*) () const) &db::Net::begin_terminals, (db::Net::const_terminal_iterator (db::Net::*) () const) &db::Net::end_terminals,
    "@brief Iterates over all terminals the net connects.\n"
    "Terminals connect devices. Terminal connections are described by \\NetTerminalRef "
    "objects."
  ) +
  gsi::iterator ("each_terminal", gsi::return_reference (), (db::Net::terminal_iterator (db::Net::*) ()) &db::Net::begin_terminals, (db::Net::terminal_iterator (db::Net::*) ()) &db::Net::end_terminals,
    "@brief Iterates over all terminals the net connects (non-const version).\n"
    "Terminals connect devices. Terminal connections are described by \\NetTerminalRef "
    "objects."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("is_floating?", &db::Net::is_floating,
    "@brief Returns true, if the net is floating.\n"
    "Floating nets are those which don't have any device or subcircuit on it and are not connected through a pin."
  ) +
  gsi::method ("is_passive?", &db::Net::is_passive,
    "@brief Returns true, if the net is passive.\n"
    "Passive nets don't have devices or subcircuits on it. They can be exposed through a pin.\n"
    "\\is_floating? implies \\is_passive?.\n"
    "\n"
    "This method has been introduced in version 0.26.1.\n"
  ) +
  gsi::method ("is_internal?", &db::Net::is_internal,
    "@brief Returns true, if the net is an internal net.\n"
    "Internal nets are those which connect exactly two terminals and nothing else (pin_count = 0 and  terminal_count == 2)."
  ) +
  gsi::method ("pin_count", &db::Net::pin_count,
    "@brief Returns the number of outgoing pins connected by this net.\n"
  ) +
  gsi::method ("subcircuit_pin_count", &db::Net::subcircuit_pin_count,
    "@brief Returns the number of subcircuit pins connected by this net.\n"
  ) +
  gsi::method ("terminal_count", &db::Net::terminal_count,
    "@brief Returns the number of terminals connected by this net.\n"
  ),
  "@brief A single net.\n"
  "A net connects multiple pins or terminals together. Pins are either "
  "pin or subcircuits of outgoing pins of the circuit the net lives in. "
  "Terminals are connections made to specific terminals of devices.\n"
  "\n"
  "Net objects are created inside a circuit with \\Circuit#create_net.\n"
  "\n"
  "To connect a net to an outgoing pin of a circuit, use \\Circuit#connect_pin, to "
  "disconnect a net from an outgoing pin use \\Circuit#disconnect_pin. "
  "To connect a net to a pin of a subcircuit, use \\SubCircuit#connect_pin, to "
  "disconnect a net from a pin of a subcircuit, use \\SubCircuit#disconnect_pin. "
  "To connect a net to a terminal of a device, use \\Device#connect_terminal, to "
  "disconnect a net from a terminal of a device, use \\Device#disconnect_terminal.\n"
  "\n"
  "This class has been added in version 0.26."
);

}

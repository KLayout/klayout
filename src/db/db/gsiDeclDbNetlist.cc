
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

#include "gsiDecl.h"
#include "dbNetlist.h"
#include "dbNetlistWriter.h"
#include "dbNetlistSpiceWriter.h"
#include "dbNetlistReader.h"
#include "dbNetlistSpiceReader.h"
#include "dbNetlistSpiceReaderDelegate.h"
#include "tlException.h"
#include "tlInternational.h"
#include "tlStream.h"
#include "tlGlobPattern.h"

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

Class<db::Pin> decl_dbPin (decl_dbNetlistObject, "db", "Pin",
  gsi::method ("id", &db::Pin::id,
    "@brief Gets the ID of the pin.\n"
  ) +
  gsi::method ("name", &db::Pin::name,
    "@brief Gets the name of the pin.\n"
  ) +
  gsi::method ("expanded_name", &db::Pin::expanded_name,
    "@brief Gets the expanded name of the pin.\n"
    "The expanded name is the name or a generic identifier made from the ID if the name is empty."
  ),
  "@brief A pin of a circuit.\n"
  "Pin objects are used to describe the outgoing pins of "
  "a circuit. To create a new pin of a circuit, use \\Circuit#create_pin.\n"
  "\n"
  "This class has been added in version 0.26."
);

static void device_connect_terminal_by_name (db::Device *device, const std::string &terminal_name, db::Net *net)
{
  if (! device->device_class ()) {
    throw tl::Exception (tl::to_string (tr ("Device does not have a device class")));
  }
  size_t terminal_id = device->device_class ()->terminal_id_for_name (terminal_name);
  device->connect_terminal (terminal_id, net);
}

static void device_disconnect_terminal (db::Device *device, size_t terminal_id)
{
  device->connect_terminal (terminal_id, 0);
}

static void device_disconnect_terminal_by_name (db::Device *device, const std::string &terminal_name)
{
  device_connect_terminal_by_name (device, terminal_name, 0);
}

static size_t get_device_index (const db::DeviceReconnectedTerminal *obj)
{
  return obj->device_index;
}

static void set_device_index (db::DeviceReconnectedTerminal *obj, size_t device_index)
{
  obj->device_index = device_index;
}

static size_t get_other_terminal_id (const db::DeviceReconnectedTerminal *obj)
{
  return obj->other_terminal_id;
}

static void set_other_terminal_id (db::DeviceReconnectedTerminal *obj, unsigned int other_terminal_id)
{
  obj->other_terminal_id = other_terminal_id;
}

Class<db::DeviceReconnectedTerminal> decl_dbDeviceReconnectedTerminal ("db", "DeviceReconnectedTerminal",
  gsi::method_ext ("device_index=", &set_device_index, gsi::arg ("device_index"),
    "@brief The device abstract index setter.\n"
    "See the class description for details."
  ) +
  gsi::method_ext ("device_index", &get_device_index,
    "@brief The device abstract index getter.\n"
    "See the class description for details."
  ) +
  gsi::method_ext ("other_terminal_id=", &set_other_terminal_id, gsi::arg ("other_terminal_id"),
    "@brief The setter for the abstract's connected terminal.\n"
    "See the class description for details."
  ) +
  gsi::method_ext ("other_terminal_id", &get_other_terminal_id,
    "@brief The getter for the abstract's connected terminal.\n"
    "See the class description for details."
  ),
  "@brief Describes a terminal rerouting in combined devices.\n"
  "Combined devices are implemented as a generalization of the device abstract concept in \\Device. For "
  "combined devices, multiple \\DeviceAbstract references are present. To support different combination schemes, "
  "device-to-abstract routing is supported. Parallel combinations will route all outer terminals to corresponding "
  "terminals of all device abstracts (because of terminal swapping these may be different ones).\n"
  "\n"
  "This object describes one route to an abstract's terminal. The device index is 0 for the main device abstract and "
  "1 for the first combined device abstract.\n"
  "\n"
  "This class has been introduced in version 0.26.\n"
);

static const db::DeviceAbstract *get_device_abstract (const db::DeviceAbstractRef *obj)
{
  return obj->device_abstract;
}

static void set_device_abstract (db::DeviceAbstractRef *obj, const db::DeviceAbstract *device_abstract)
{
  obj->device_abstract = device_abstract;
}

static db::DCplxTrans get_trans (const db::DeviceAbstractRef *obj)
{
  return obj->trans;
}

static void set_trans (db::DeviceAbstractRef *obj, const db::DCplxTrans &trans)
{
  obj->trans = trans;
}

Class<db::DeviceAbstractRef> decl_dbDeviceAbstractRef ("db", "DeviceAbstractRef",
  gsi::method_ext ("device_abstract=", &set_device_abstract, gsi::arg ("device_abstract"),
    "@brief The setter for the device abstract reference.\n"
    "See the class description for details."
  ) +
  gsi::method_ext ("device_abstract", &get_device_abstract,
    "@brief The getter for the device abstract reference.\n"
    "See the class description for details."
  ) +
  gsi::method_ext ("trans=", &set_trans, gsi::arg ("tr"),
    "@brief The setter for the relative transformation of the instance.\n"
    "See the class description for details."
  ) +
  gsi::method_ext ("trans", &get_trans,
    "@brief The getter for the relative transformation of the instance.\n"
    "See the class description for details."
  ),
  "@brief Describes an additional device abstract reference for combined devices.\n"
  "Combined devices are implemented as a generalization of the device abstract concept in \\Device. For "
  "combined devices, multiple \\DeviceAbstract references are present. This class describes such an "
  "additional reference. A reference is a pointer to an abstract plus a transformation by which the abstract "
  "is transformed geometrically as compared to the first (initial) abstract.\n"
  "\n"
  "This class has been introduced in version 0.26.\n"
);

static bool is_combined_device (const db::Device *device)
{
  return ! device->reconnected_terminals ().empty ();
}

static std::vector<db::DeviceReconnectedTerminal> empty;

static std::vector<db::DeviceReconnectedTerminal>::const_iterator begin_reconnected_terminals_for (const db::Device *device, size_t terminal_id)
{
  const std::vector<db::DeviceReconnectedTerminal> *ti = device->reconnected_terminals_for ((unsigned int) terminal_id);
  if (! ti) {
    return empty.begin ();
  } else {
    return ti->begin ();
  }
}

static std::vector<db::DeviceReconnectedTerminal>::const_iterator end_reconnected_terminals_for (const db::Device *device, size_t terminal_id)
{
  const std::vector<db::DeviceReconnectedTerminal> *ti = device->reconnected_terminals_for ((unsigned int) terminal_id);
  if (! ti) {
    return empty.end ();
  } else {
    return ti->end ();
  }
}

static void clear_reconnected_terminals (db::Device *device)
{
  device->reconnected_terminals ().clear ();
}

static void add_reconnected_terminals (db::Device *device, size_t outer_terminal, const db::DeviceReconnectedTerminal &t)
{
  device->reconnected_terminals () [(unsigned int) outer_terminal].push_back (t);
}

static std::vector<db::DeviceAbstractRef>::const_iterator begin_other_abstracts (const db::Device *device)
{
  return device->other_abstracts ().begin ();
}

static std::vector<db::DeviceAbstractRef>::const_iterator end_other_abstracts (const db::Device *device)
{
  return device->other_abstracts ().end ();
}

static void clear_other_abstracts (db::Device *device)
{
  device->other_abstracts ().clear ();
}

static void add_other_abstracts (db::Device *device, const db::DeviceAbstractRef &ref)
{
  device->other_abstracts ().push_back (ref);
}

static const db::Net *net_for_terminal_by_name_const (const db::Device *device, const std::string &name)
{
  if (! device->device_class () || ! device->device_class ()->has_terminal_with_name (name)) {
    return 0;
  } else {
    return device->net_for_terminal (device->device_class ()->terminal_id_for_name (name));
  }
}

static db::Net *net_for_terminal_by_name (db::Device *device, const std::string &name)
{
  if (! device->device_class () || ! device->device_class ()->has_terminal_with_name (name)) {
    return 0;
  } else {
    return device->net_for_terminal (device->device_class ()->terminal_id_for_name (name));
  }
}

Class<db::Device> decl_dbDevice (decl_dbNetlistObject, "db", "Device",
  gsi::method ("device_class", &db::Device::device_class,
    "@brief Gets the device class the device belongs to.\n"
  ) +
  gsi::method ("device_abstract", &db::Device::device_abstract,
    "@brief Gets the device abstract for this device instance.\n"
    "See \\DeviceAbstract for more details.\n"
  ) +
  gsi::method ("device_abstract=", &db::Device::set_device_abstract,
    "@hide\n"
    "Provided for test purposes mainly. Be careful with pointers!"
  ) +
  gsi::method_ext ("is_combined_device?", &is_combined_device,
    "@brief Returns true, if the device is a combined device.\n"
    "Combined devices feature multiple device abstracts and device-to-abstract terminal connections.\n"
    "See \\each_reconnected_terminal and \\each_combined_abstract for more details.\n"
  ) +
  gsi::iterator_ext ("each_reconnected_terminal_for", &begin_reconnected_terminals_for, &end_reconnected_terminals_for, gsi::arg ("terminal_id"),
    "@brief Iterates over the reconnected terminal specifications for a given outer terminal.\n"
    "This feature applies to combined devices. This iterator will deliver all device-to-abstract terminal reroutings.\n"
  ) +
  gsi::method_ext ("clear_reconnected_terminals", &clear_reconnected_terminals,
    "@hide\n"
    "Provided for test purposes mainly."
  ) +
  gsi::method_ext ("add_reconnected_terminal_for", &add_reconnected_terminals, gsi::arg ("outer_terminal"), gsi::arg ("descriptor"),
    "@hide\n"
    "Provided for test purposes mainly."
  ) +
  gsi::iterator_ext ("each_combined_abstract", &begin_other_abstracts, &end_other_abstracts,
    "@brief Iterates over the combined device specifications.\n"
    "This feature applies to combined devices. This iterator will deliver all device abstracts present in addition to the default device abstract.\n"
  ) +
  gsi::method_ext ("clear_combined_abstracts", &clear_other_abstracts,
    "@hide\n"
    "Provided for test purposes mainly."
  ) +
  gsi::method_ext ("add_combined_abstract", &add_other_abstracts, gsi::arg ("ref"),
    "@hide\n"
    "Provided for test purposes mainly."
  ) +
  gsi::method ("circuit", (const db::Circuit *(db::Device::*) () const) &db::Device::circuit,
    "@brief Gets the circuit the device lives in."
  ) +
  gsi::method ("circuit", (db::Circuit *(db::Device::*) ()) &db::Device::circuit,
    "@brief Gets the circuit the device lives in (non-const version)."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("id", &db::Device::id,
    "@brief Gets the device ID.\n"
    "The ID is a unique integer which identifies the device.\n"
    "It can be used to retrieve the device from the circuit using \\Circuit#device_by_id.\n"
    "When assigned, the device ID is not 0.\n"
  ) +
  gsi::method ("name=", &db::Device::set_name, gsi::arg ("name"),
    "@brief Sets the name of the device.\n"
    "Device names are used to name a device inside a netlist file. "
    "Device names should be unique within a circuit."
  ) +
  gsi::method ("name", &db::Device::name,
    "@brief Gets the name of the device.\n"
  ) +
  gsi::method ("trans=", &db::Device::set_trans, gsi::arg ("t"),
    "@brief Sets the location of the device.\n"
    "The device location is essentially describing the position of the device. The position is typically the center of some "
    "recognition shape. In this case the transformation is a plain displacement to the center of this shape."
  ) +
  gsi::method ("trans", &db::Device::trans,
    "@brief Gets the location of the device.\n"
    "See \\trans= for details about this method."
  ) +
  gsi::method ("expanded_name", &db::Device::expanded_name,
    "@brief Gets the expanded name of the device.\n"
    "The expanded name takes the name of the device. If the name is empty, the numeric ID will be used to build a name. "
  ) +
  gsi::method ("net_for_terminal", (const db::Net *(db::Device::*) (size_t) const) &db::Device::net_for_terminal, gsi::arg ("terminal_id"),
    "@brief Gets the net connected to the specified terminal.\n"
    "If the terminal is not connected, nil is returned for the net."
  ) +
  gsi::method ("net_for_terminal", (db::Net *(db::Device::*) (size_t)) &db::Device::net_for_terminal, gsi::arg ("terminal_id"),
    "@brief Gets the net connected to the specified terminal (non-const version).\n"
    "If the terminal is not connected, nil is returned for the net."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method_ext ("net_for_terminal", net_for_terminal_by_name_const, gsi::arg ("terminal_name"),
    "@brief Gets the net connected to the specified terminal.\n"
    "If the terminal is not connected, nil is returned for the net."
    "\n\n"
    "This convenience method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method_ext ("net_for_terminal", net_for_terminal_by_name, gsi::arg ("terminal_name"),
    "@brief Gets the net connected to the specified terminal (non-const version).\n"
    "If the terminal is not connected, nil is returned for the net."
    "\n\n"
    "This convenience method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method ("connect_terminal", &db::Device::connect_terminal, gsi::arg ("terminal_id"), gsi::arg ("net"),
    "@brief Connects the given terminal to the specified net.\n"
  ) +
  gsi::method_ext ("disconnect_terminal", &device_disconnect_terminal, gsi::arg ("terminal_id"),
    "@brief Disconnects the given terminal from any net.\n"
    "If the terminal has been connected to a global, this connection will be disconnected too."
  ) +
  gsi::method_ext ("connect_terminal", &device_connect_terminal_by_name, gsi::arg ("terminal_name"), gsi::arg ("net"),
    "@brief Connects the given terminal to the specified net.\n"
    "This version accepts a terminal name. If the name is not a valid terminal name, an exception is raised.\n"
    "If the terminal has been connected to a global net, it will be disconnected from there."
  ) +
  gsi::method_ext ("disconnect_terminal", &device_disconnect_terminal_by_name, gsi::arg ("terminal_name"),
    "@brief Disconnects the given terminal from any net.\n"
    "This version accepts a terminal name. If the name is not a valid terminal name, an exception is raised."
  ) +
  gsi::method ("parameter", (double (db::Device::*) (size_t) const) &db::Device::parameter_value, gsi::arg ("param_id"),
    "@brief Gets the parameter value for the given parameter ID."
  ) +
  gsi::method ("set_parameter", (void (db::Device::*) (size_t, double)) &db::Device::set_parameter_value, gsi::arg ("param_id"), gsi::arg ("value"),
    "@brief Sets the parameter value for the given parameter ID."
  ) +
  gsi::method ("parameter", (double (db::Device::*) (const std::string &) const) &db::Device::parameter_value, gsi::arg ("param_name"),
    "@brief Gets the parameter value for the given parameter name.\n"
    "If the parameter name is not valid, an exception is thrown."
  ) +
  gsi::method ("set_parameter", (void (db::Device::*) (const std::string &, double)) &db::Device::set_parameter_value, gsi::arg ("param_name"), gsi::arg ("value"),
    "@brief Sets the parameter value for the given parameter name.\n"
    "If the parameter name is not valid, an exception is thrown."
  ),
  "@brief A device inside a circuit.\n"
  "Device object represent atomic devices such as resistors, diodes or transistors. "
  "The \\Device class represents a particular device with specific parameters. "
  "The type of device is represented by a \\DeviceClass object. Device objects "
  "live in \\Circuit objects, the device class objects live in the \\Netlist object.\n"
  "\n"
  "Devices connect to nets through terminals. Terminals are described by a terminal ID which is "
  "essentially the zero-based index of the terminal. Terminal definitions can be "
  "obtained from the device class using the \\DeviceClass#terminal_definitions method.\n"
  "\n"
  "Devices connect to nets through the \\Device#connect_terminal method. "
  "Device terminals can be disconnected using \\Device#disconnect_terminal.\n"
  "\n"
  "Device objects are created inside a circuit with \\Circuit#create_device.\n"
  "\n"
  "This class has been added in version 0.26."
);

Class<db::DeviceAbstract> decl_dbDeviceAbstract ("db", "DeviceAbstract",
  gsi::method ("netlist", (const db::Netlist *(db::DeviceAbstract::*) () const) &db::DeviceAbstract::netlist,
    "@brief Gets the netlist the device abstract lives in."
  ) +
  gsi::method ("netlist", (db::Netlist *(db::DeviceAbstract::*) ()) &db::DeviceAbstract::netlist,
    "@brief Gets the netlist the device abstract lives in (non-const version)."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("device_class", &db::DeviceAbstract::device_class,
    "@brief Gets the device class of the device."
  ) +
  gsi::method ("name=", &db::DeviceAbstract::set_name, gsi::arg ("name"),
    "@brief Sets the name of the device abstract.\n"
    "Device names are used to name a device abstract inside a netlist file. "
    "Device names should be unique within a netlist."
  ) +
  gsi::method ("name", &db::DeviceAbstract::name,
    "@brief Gets the name of the device abstract.\n"
  ) +
  gsi::method ("cell_index", &db::DeviceAbstract::cell_index,
    "@brief Gets the cell index of the device abstract.\n"
    "This is the cell that represents the device."
  ) +
  gsi::method ("cluster_id_for_terminal", &db::DeviceAbstract::cluster_id_for_terminal, gsi::arg ("terminal_id"),
    "@brief Gets the cluster ID for the given terminal.\n"
    "The cluster ID links the terminal to geometrical shapes within the clusters of the cell (see \\cell_index)"
  ),
  "@brief A geometrical device abstract\n"
  "This class represents the geometrical model for the device. It links into the extracted layout "
  "to a cell which holds the terminal shapes for the device.\n"
  "\n"
  "This class has been added in version 0.26."
);

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

static db::DeviceTerminalDefinition *new_terminal_definition (const std::string &name, const std::string &description)
{
  return new db::DeviceTerminalDefinition (name, description);
}

Class<db::DeviceTerminalDefinition> decl_dbDeviceTerminalDefinition ("db", "DeviceTerminalDefinition",
  gsi::constructor ("new", &gsi::new_terminal_definition, gsi::arg ("name"), gsi::arg ("description", std::string ()),
    "@brief Creates a new terminal definition."
  ) +
  gsi::method ("name", &db::DeviceTerminalDefinition::name,
    "@brief Gets the name of the terminal."
  ) +
  gsi::method ("name=", &db::DeviceTerminalDefinition::set_name, gsi::arg ("name"),
    "@brief Sets the name of the terminal."
  ) +
  gsi::method ("description", &db::DeviceTerminalDefinition::description,
    "@brief Gets the description of the terminal."
  ) +
  gsi::method ("description=", &db::DeviceTerminalDefinition::set_description, gsi::arg ("description"),
    "@brief Sets the description of the terminal."
  ) +
  gsi::method ("id", &db::DeviceTerminalDefinition::id,
    "@brief Gets the ID of the terminal.\n"
    "The ID of the terminal is used in some places to refer to a specific terminal (e.g. in "
    "the \\NetTerminalRef object)."
  ),
  "@brief A terminal descriptor\n"
  "This class is used inside the \\DeviceClass class to describe a terminal of the device.\n"
  "\n"
  "This class has been added in version 0.26."
);

static db::DeviceParameterDefinition *new_parameter_definition (const std::string &name, const std::string &description, double default_value, bool is_primary, double si_scaling, double geo_scaling_exponent)
{
  return new db::DeviceParameterDefinition (name, description, default_value, is_primary, si_scaling, geo_scaling_exponent);
}

Class<db::DeviceParameterDefinition> decl_dbDeviceParameterDefinition ("db", "DeviceParameterDefinition",
  gsi::constructor ("new", &gsi::new_parameter_definition, gsi::arg ("name"), gsi::arg ("description", std::string ()), gsi::arg ("default_value", 0.0), gsi::arg ("is_primary", true), gsi::arg ("si_scaling", 1.0), gsi::arg ("geo_scaling_exponent", 0.0),
    "@brief Creates a new parameter definition.\n"
    "@param name The name of the parameter\n"
    "@param description The human-readable description\n"
    "@param default_value The initial value\n"
    "@param is_primary True, if the parameter is a primary parameter (see \\is_primary=)\n"
    "@param si_scaling The scaling factor to SI units\n"
    "@param geo_scaling_exponent Indicates how the parameter scales with geometrical scaling (0: no scaling, 1.0: linear, 2.0: quadratic)\n"
  ) +
  gsi::method ("name", &db::DeviceParameterDefinition::name,
    "@brief Gets the name of the parameter."
  ) +
  gsi::method ("name=", &db::DeviceParameterDefinition::set_name, gsi::arg ("name"),
    "@brief Sets the name of the parameter."
  ) +
  gsi::method ("description", &db::DeviceParameterDefinition::description,
    "@brief Gets the description of the parameter."
  ) +
  gsi::method ("description=", &db::DeviceParameterDefinition::set_description, gsi::arg ("description"),
    "@brief Sets the description of the parameter."
  ) +
  gsi::method ("default_value", &db::DeviceParameterDefinition::default_value,
    "@brief Gets the default value of the parameter."
  ) +
  gsi::method ("default_value=", &db::DeviceParameterDefinition::set_default_value, gsi::arg ("default_value"),
    "@brief Sets the default value of the parameter.\n"
    "The default value is used to initialize parameters of \\Device objects."
  ) +
  gsi::method ("is_primary?", &db::DeviceParameterDefinition::is_primary,
    "@brief Gets a value indicating whether the parameter is a primary parameter\n"
    "See \\is_primary= for details about this predicate."
  ) +
  gsi::method ("is_primary=", &db::DeviceParameterDefinition::set_is_primary, gsi::arg ("primary"),
    "@brief Sets a value indicating whether the parameter is a primary parameter\n"
    "If this flag is set to true (the default), the parameter is considered a primary parameter.\n"
    "Only primary parameters are compared by default.\n"
  ) +
  gsi::method ("si_scaling", &db::DeviceParameterDefinition::si_scaling,
    "@brief Gets the scaling factor to SI units.\n"
    "For parameters in micrometers - for example W and L of MOS devices - this factor can be set to 1e-6 to reflect "
    "the unit."
  ) +
  gsi::method ("si_scaling=", &db::DeviceParameterDefinition::set_si_scaling,
    "@brief Sets the scaling factor to SI units.\n"
    "\n"
    "This setter has been added in version 0.28.6."
  ) +
  gsi::method ("geo_scaling_exponent", &db::DeviceParameterDefinition::geo_scaling_exponent,
    "@brief Gets the geometry scaling exponent.\n"
    "This value is used when applying '.options scale' in the SPICE reader for example. "
    "It is zero for 'no scaling', 1.0 for linear scaling and 2.0 for quadratic scaling.\n"
    "\n"
    "This attribute has been added in version 0.28.6."
  ) +
  gsi::method ("geo_scaling_exponent=", &db::DeviceParameterDefinition::set_geo_scaling_exponent,
    "@brief Sets the geometry scaling exponent.\n"
    "See \\geo_scaling_exponent for details.\n"
    "\n"
    "This attribute has been added in version 0.28.6."
  ) +
  gsi::method ("id", &db::DeviceParameterDefinition::id,
    "@brief Gets the ID of the parameter.\n"
    "The ID of the parameter is used in some places to refer to a specific parameter (e.g. in "
    "the \\NetParameterRef object)."
  ),
  "@brief A parameter descriptor\n"
  "This class is used inside the \\DeviceClass class to describe a parameter of the device.\n"
  "\n"
  "This class has been added in version 0.26."
);

namespace
{

/**
 *  @brief A DeviceParameterCompare implementation that allows reimplementation of the virtual methods
 */
class GenericDeviceParameterCompare
  : public db::EqualDeviceParameters
{
public:
  GenericDeviceParameterCompare ()
    : db::EqualDeviceParameters ()
  {
    //  .. nothing yet ..
  }

  virtual bool less (const db::Device &a, const db::Device &b) const
  {
    if (cb_less.can_issue ()) {
      return cb_less.issue<db::EqualDeviceParameters, bool, const db::Device &, const db::Device &> (&db::EqualDeviceParameters::less, a, b);
    } else {
      return db::EqualDeviceParameters::less (a, b);
    }
  }

  gsi::Callback cb_less;
};

/**
 *  @brief A DeviceCombiner implementation that allows reimplementation of the virtual methods
 */
class GenericDeviceCombiner
  : public db::DeviceCombiner
{
public:
  GenericDeviceCombiner ()
    : db::DeviceCombiner ()
  {
    //  .. nothing yet ..
  }

  virtual bool combine_devices (db::Device *a, db::Device *b) const
  {
    if (cb_combine.can_issue ()) {
      return cb_combine.issue<db::DeviceCombiner, bool, db::Device *, db::Device *> (&db::DeviceCombiner::combine_devices, a, b);
    } else {
      return false;
    }
  }

  gsi::Callback cb_combine;
};

}

db::EqualDeviceParameters *make_equal_dp (size_t param_id, double absolute, double relative)
{
  return new db::EqualDeviceParameters (param_id, absolute, relative);
}

db::EqualDeviceParameters *make_ignore_dp (size_t param_id)
{
  return new db::EqualDeviceParameters (param_id, true);
}

Class<db::EqualDeviceParameters> decl_dbEqualDeviceParameters ("db", "EqualDeviceParameters",
  gsi::constructor ("new", &make_equal_dp, gsi::arg ("param_id"), gsi::arg ("absolute", 0.0), gsi::arg ("relative", 0.0),
    "@brief Creates a device parameter comparer for a single parameter.\n"
    "'absolute' is the absolute deviation allowed for the parameter values. "
    "'relative' is the relative deviation allowed for the parameter values (a value between 0 and 1).\n"
    "\n"
    "A value of 0 for both absolute and relative deviation means the parameters have to match exactly.\n"
    "\n"
    "If 'absolute' and 'relative' are both given, their deviations will add to the allowed difference between "
    "two parameter values. The relative deviation will be applied to the mean value of both parameter values. "
    "For example, when comparing parameter values of 40 and 60, a relative deviation of 0.35 means an absolute "
    "deviation of 17.5 (= 0.35 * average of 40 and 60) which does not make both values match."
  ) +
  gsi::constructor ("ignore", &make_ignore_dp, gsi::arg ("param_id"),
    "@brief Creates a device parameter comparer which ignores the parameter.\n"
    "\n"
    "This specification can be used to make a parameter ignored. Starting with version 0.27.4, all primary parameters "
    "are compared. Before 0.27.4, giving a tolerance meant only those parameters are compared. To exclude a primary "
    "parameter from the compare, use the 'ignore' specification for that parameter.\n"
    "\n"
    "This constructor has been introduced in version 0.27.4.\n"
  ) +
  gsi::method ("+", &db::EqualDeviceParameters::operator+, gsi::arg ("other"),
    "@brief Combines two parameters for comparison.\n"
    "The '+' operator will join the parameter comparers and produce one that checks the combined parameters.\n"
  ) +
  gsi::method ("+=", &db::EqualDeviceParameters::operator+, gsi::arg ("other"),
    "@brief Combines two parameters for comparison (in-place).\n"
    "The '+=' operator will join the parameter comparers and produce one that checks the combined parameters.\n"
  ) +
  gsi::method ("to_string", &db::EqualDeviceParameters::to_string, "@hide"),
  "@brief A device parameter equality comparer.\n"
  "Attach this object to a device class with \\DeviceClass#equal_parameters= to make the device "
  "class use this comparer:\n"
  "\n"
  "@code\n"
  "# 20nm tolerance for length:\n"
  "equal_device_parameters = RBA::EqualDeviceParameters::new(RBA::DeviceClassMOS4Transistor::PARAM_L, 0.02, 0.0)\n"
  "# one percent tolerance for width:\n"
  "equal_device_parameters += RBA::EqualDeviceParameters::new(RBA::DeviceClassMOS4Transistor::PARAM_W, 0.0, 0.01)\n"
  "# applies the compare delegate:\n"
  "netlist.device_class_by_name(\"NMOS\").equal_parameters = equal_device_parameters\n"
  "@/code\n"
  "\n"
  "You can use this class to specify fuzzy equality criteria for the comparison of device parameters in "
  "netlist verification or to confine the equality of devices to certain parameters only.\n"
  "\n"
  "This class has been added in version 0.26."
);

Class<GenericDeviceParameterCompare> decl_GenericDeviceParameterCompare (decl_dbEqualDeviceParameters, "db", "GenericDeviceParameterCompare",
  gsi::callback ("less", &GenericDeviceParameterCompare::less, &GenericDeviceParameterCompare::cb_less, gsi::arg ("device_a"), gsi::arg ("device_b"),
    "@brief Compares the parameters of two devices for a begin less than b. "
    "Returns true, if the parameters of device a are considered less than those of device b."
    "The 'less' implementation needs to ensure strict weak ordering. Specifically, less(a,b) == false and less(b,a) implies that a is equal to b and "
    "less(a,b) == true implies that less(b,a) is false and vice versa. If not, an internal error "
    "will be encountered on netlist compare."
  ),
  "@brief A class implementing the comparison of device parameters.\n"
  "Reimplement this class to provide a custom device parameter compare scheme.\n"
  "Attach this object to a device class with \\DeviceClass#equal_parameters= to make the device "
  "class use this comparer.\n"
  "\n"
  "This class is intended for special cases. In most scenarios it is easier to use \\EqualDeviceParameters instead of "
  "implementing a custom comparer class.\n"
  "\n"
  "This class has been added in version 0.26. The 'equal' method has been dropped in 0.27.1 as it can be expressed as !less(a,b) && !less(b,a)."
);

Class<GenericDeviceCombiner> decl_GenericDeviceCombiner ("db", "GenericDeviceCombiner",
  gsi::callback ("combine_devices", &GenericDeviceCombiner::combine_devices, &GenericDeviceCombiner::cb_combine, gsi::arg ("device_a"), gsi::arg ("device_b"),
    "@brief Combines two devices if possible.\n"
    "This method needs to test, whether the two devices can be combined. Both devices "
    "are guaranteed to share the same device class. "
    "If they cannot be combined, this method shall do nothing and return false. "
    "If they can be combined, this method shall reconnect the nets of the first "
    "device and entirely disconnect the nets of the second device. "
    "The second device will be deleted afterwards. "
  ),
  "@brief A class implementing the combination of two devices (parallel or serial mode).\n"
  "Reimplement this class to provide a custom device combiner.\n"
  "Device combination requires 'supports_paralell_combination' or 'supports_serial_combination' to be set "
  "to true for the device class. In the netlist device combination step, the algorithm will try to identify "
  "devices which can be combined into single devices and use the combiner object to implement the actual "
  "joining of such devices.\n"
  "\n"
  "Attach this object to a device class with \\DeviceClass#combiner= to make the device "
  "class use this combiner.\n"
  "\n"
  "This class has been added in version 0.27.3."
);

static tl::id_type id_of_device_class (const db::DeviceClass *cls)
{
  return tl::id_of (cls);
}

static void equal_parameters (db::DeviceClass *cls, db::EqualDeviceParameters *comparer)
{
  cls->set_parameter_compare_delegate (comparer);
}

static db::EqualDeviceParameters *get_equal_parameters (db::DeviceClass *cls)
{
  return dynamic_cast<db::EqualDeviceParameters *> (cls->parameter_compare_delegate ());
}

static void set_combiner (db::DeviceClass *cls, GenericDeviceCombiner *combiner)
{
  cls->set_device_combiner (combiner);
}

static GenericDeviceCombiner *get_combiner (db::DeviceClass *cls)
{
  return dynamic_cast<GenericDeviceCombiner *> (cls->device_combiner ());
}

static void enable_parameter (db::DeviceClass *cls, size_t id, bool en)
{
  db::DeviceParameterDefinition *pd = cls->parameter_definition_non_const (id);
  if (pd) {
    pd->set_is_primary (en);
  }
}

static void enable_parameter2 (db::DeviceClass *cls, const std::string &name, bool en)
{
  if (! cls->has_parameter_with_name (name)) {
    return;
  }

  size_t id = cls->parameter_id_for_name (name);
  db::DeviceParameterDefinition *pd = cls->parameter_definition_non_const (id);
  if (pd) {
    pd->set_is_primary (en);
  }
}

static const db::DeviceParameterDefinition *parameter_definition2 (const db::DeviceClass *cls, const std::string &name)
{
  if (! cls->has_parameter_with_name (name)) {
    return 0;
  } else {
    return cls->parameter_definition (cls->parameter_id_for_name (name));
  }
}

static void dc_add_terminal_definition (db::DeviceClass *cls, db::DeviceTerminalDefinition *terminal_def)
{
  if (terminal_def) {
    *terminal_def = cls->add_terminal_definition (*terminal_def);
  }
}

static void dc_add_parameter_definition (db::DeviceClass *cls, db::DeviceParameterDefinition *parameter_def)
{
  if (parameter_def) {
    *parameter_def = cls->add_parameter_definition (*parameter_def);
  }
}

Class<db::DeviceClass> decl_dbDeviceClass ("db", "DeviceClass",
  gsi::method ("name", &db::DeviceClass::name,
    "@brief Gets the name of the device class."
  ) +
  gsi::method ("name=", &db::DeviceClass::set_name, gsi::arg ("name"),
    "@brief Sets the name of the device class."
  ) +
  gsi::method ("strict?", &db::DeviceClass::is_strict,
    "@brief Gets a value indicating whether this class performs strict terminal mapping\n"
    "See \\strict= for details about this attribute."
  ) +
  gsi::method ("strict=", &db::DeviceClass::set_strict, gsi::arg ("s"),
    "@brief Sets a value indicating whether this class performs strict terminal mapping\n"
    "\n"
    "Classes with this flag set never allow terminal swapping, even if the device symmetry supports that. "
    "If two classes are involved in a netlist compare,\n"
    "terminal swapping will be disabled if one of the classes is in strict mode.\n"
    "\n"
    "By default, device classes are not strict and terminal swapping is allowed as far as the "
    "device symmetry supports that."
  ) +
  gsi::method ("description", &db::DeviceClass::description,
    "@brief Gets the description text of the device class."
  ) +
  gsi::method ("description=", &db::DeviceClass::set_description, gsi::arg ("description"),
    "@brief Sets the description of the device class."
  ) +
  gsi::method ("netlist", (db::Netlist *(db::DeviceClass::*) ()) &db::DeviceClass::netlist,
    "@brief Gets the netlist the device class lives in."
  ) +
  gsi::method_ext ("id", &gsi::id_of_device_class,
    "@brief Gets the unique ID of the device class\n"
    "The ID is a unique integer that identifies the device class. Use the ID "
    "to check for object identity - i.e. to determine whether two devices share the "
    "same device class."
  ) +
  gsi::method ("terminal_definitions", &db::DeviceClass::terminal_definitions,
    "@brief Gets the list of terminal definitions of the device.\n"
    "See the \\DeviceTerminalDefinition class description for details."
  ) +
  gsi::method ("terminal_definition", &db::DeviceClass::terminal_definition, gsi::arg ("terminal_id"),
    "@brief Gets the terminal definition object for a given ID.\n"
    "Terminal definition IDs are used in some places to reference a specific terminal of a device. "
    "This method obtains the corresponding definition object."
  ) +
  gsi::method ("parameter_definitions", &db::DeviceClass::parameter_definitions,
    "@brief Gets the list of parameter definitions of the device.\n"
    "See the \\DeviceParameterDefinition class description for details."
  ) +
  gsi::method ("parameter_definition", &db::DeviceClass::parameter_definition, gsi::arg ("parameter_id"),
    "@brief Gets the parameter definition object for a given ID.\n"
    "Parameter definition IDs are used in some places to reference a specific parameter of a device. "
    "This method obtains the corresponding definition object."
  ) +
  gsi::method_ext ("parameter_definition", &parameter_definition2, gsi::arg ("parameter_name"),
    "@brief Gets the parameter definition object for a given ID.\n"
    "Parameter definition IDs are used in some places to reference a specific parameter of a device. "
    "This method obtains the corresponding definition object."
    "\n"
    "This version accepts a parameter name.\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method_ext ("enable_parameter", &enable_parameter, gsi::arg ("parameter_id"), gsi::arg ("enable"),
    "@brief Enables or disables a parameter.\n"
    "Some parameters are 'secondary' parameters which are extracted but not handled in device compare and are not shown in the netlist browser. "
    "For example, the 'W' parameter of the resistor is such a secondary parameter. This method allows turning a parameter in a primary one ('enable') or "
    "into a secondary one ('disable').\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method_ext ("enable_parameter", &enable_parameter2, gsi::arg ("parameter_name"), gsi::arg ("enable"),
    "@brief Enables or disables a parameter.\n"
    "Some parameters are 'secondary' parameters which are extracted but not handled in device compare and are not shown in the netlist browser. "
    "For example, the 'W' parameter of the resistor is such a secondary parameter. This method allows turning a parameter in a primary one ('enable') or "
    "into a secondary one ('disable').\n"
    "\n"
    "This version accepts a parameter name.\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method ("has_parameter?", &db::DeviceClass::has_parameter_with_name, gsi::arg ("name"),
    "@brief Returns true, if the device class has a parameter with the given name.\n"
  ) +
  gsi::method ("parameter_id", &db::DeviceClass::parameter_id_for_name, gsi::arg ("name"),
    "@brief Returns the parameter ID of the parameter with the given name.\n"
    "An exception is thrown if there is no parameter with the given name. Use \\has_parameter to check "
    "whether the name is a valid parameter name."
  ) +
  gsi::method ("has_terminal?", &db::DeviceClass::has_terminal_with_name, gsi::arg ("name"),
    "@brief Returns true, if the device class has a terminal with the given name.\n"
  ) +
  gsi::method ("terminal_id", &db::DeviceClass::terminal_id_for_name, gsi::arg ("name"),
    "@brief Returns the terminal ID of the terminal with the given name.\n"
    "An exception is thrown if there is no terminal with the given name. Use \\has_terminal to check "
    "whether the name is a valid terminal name."
  ) +
  gsi::method_ext ("equal_parameters", &get_equal_parameters,
    "@brief Gets the device parameter comparer for netlist verification or nil if no comparer is registered.\n"
    "See \\equal_parameters= for the setter.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method_ext ("equal_parameters=", &equal_parameters, gsi::arg ("comparer"),
    "@brief Specifies a device parameter comparer for netlist verification.\n"
    "By default, all devices are compared with all parameters. If you want to select only certain parameters "
    "for comparison or use a fuzzy compare criterion, use an \\EqualDeviceParameters object and assign it "
    "to the device class of one netlist. You can also chain multiple \\EqualDeviceParameters objects with the '+' operator "
    "for specifying multiple parameters in the equality check.\n"
    "\n"
    "You can assign nil for the parameter comparer to remove it.\n"
    "\n"
    "In special cases, you can even implement a custom compare scheme by deriving your own comparer from the \\GenericDeviceParameterCompare class.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method_ext ("add_terminal", &gsi::dc_add_terminal_definition, gsi::arg ("terminal_def"),
    "@brief Adds the given terminal definition to the device class\n"
    "This method will define a new terminal. The new terminal is added at the end of existing terminals. "
    "The terminal definition object passed as the argument is modified to contain the "
    "new ID of the terminal.\n"
    "\n"
    "The terminal is copied into the device class. Modifying the terminal object later "
    "does not have the effect of changing the terminal definition.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method ("clear_terminals", &db::DeviceClass::clear_terminal_definitions,
    "@brief Clears the list of terminals\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method_ext ("add_parameter", &gsi::dc_add_parameter_definition, gsi::arg ("parameter_def"),
    "@brief Adds the given parameter definition to the device class\n"
    "This method will define a new parameter. The new parameter is added at the end of existing parameters. "
    "The parameter definition object passed as the argument is modified to contain the "
    "new ID of the parameter."
    "\n"
    "The parameter is copied into the device class. Modifying the parameter object later "
    "does not have the effect of changing the parameter definition.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method ("clear_parameters", &db::DeviceClass::clear_parameter_definitions,
    "@brief Clears the list of parameters\n"
    "\n"
    "This method has been added in version 0.27.3.\n"
  ) +
  gsi::method_ext ("combiner=", &set_combiner, gsi::arg ("combiner"),
    "@brief Specifies a device combiner (parallel or serial device combination).\n"
    "\n"
    "You can assign nil for the combiner to remove it.\n"
    "\n"
    "In special cases, you can even implement a custom combiner by deriving your own comparer from the \\GenericDeviceCombiner class.\n"
    "\n"
    "This method has been added in version 0.27.3.\n"
  ) +
  gsi::method_ext ("combiner", &get_combiner,
    "@brief Gets a device combiner or nil if none is registered.\n"
    "\n"
    "This method has been added in version 0.27.3.\n"
  ) +
  gsi::method ("supports_parallel_combination=", &db::DeviceClass::set_supports_parallel_combination, gsi::arg ("f"),
    "@brief Specifies whether the device supports parallel device combination.\n"
    "Parallel device combination means that all terminals of two combination candidates are connected to the same nets. "
    "If the device does not support this combination mode, this predicate can be set to false. This will make the device "
    "extractor skip the combination test in parallel mode and improve performance somewhat.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method ("supports_serial_combination=", &db::DeviceClass::set_supports_serial_combination, gsi::arg ("f"),
    "@brief Specifies whether the device supports serial device combination.\n"
    "Serial device combination means that the devices are connected by internal nodes. "
    "If the device does not support this combination mode, this predicate can be set to false. This will make the device "
    "extractor skip the combination test in serial mode and improve performance somewhat.\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method ("equivalent_terminal_id", &db::DeviceClass::equivalent_terminal_id, gsi::arg ("original_id"), gsi::arg ("equivalent_id"),
    "@brief Specifies a terminal to be equivalent to another.\n"
    "Use this method to specify two terminals to be exchangeable. For example to make S and D of a MOS transistor equivalent, "
    "call this method with S and D terminal IDs. In netlist matching, S will be translated to D and thus made equivalent to D.\n"
    "\n"
    "Note that terminal equivalence is not effective if the device class operates in strict mode (see \\DeviceClass#strict=).\n"
    "\n"
    "This method has been moved from 'GenericDeviceClass' to 'DeviceClass' in version 0.27.3.\n"
  ) +
  gsi::method ("clear_equivalent_terminal_ids", &db::DeviceClass::clear_equivalent_terminal_ids,
    "@brief Clears all equivalent terminal ids\n"
    "\n"
    "This method has been added in version 0.27.3.\n"
  ),
  "@brief A class describing a specific type of device.\n"
  "Device class objects live in the context of a \\Netlist object. After a "
  "device class is created, it must be added to the netlist using \\Netlist#add. "
  "The netlist will own the device class object. When the netlist is destroyed, the "
  "device class object will become invalid.\n"
  "\n"
  "The \\DeviceClass class is the base class for other device classes.\n"
  "\n"
  "This class has been added in version 0.26. In version 0.27.3, the 'GenericDeviceClass' has been integrated with \\DeviceClass "
  "and the device class was made writeable in most respects. This enables manipulating built-in device classes."
);

namespace {

/**
 *  @brief A DeviceClass implementation that allows reimplementation of the virtual methods
 *
 *  NOTE: cloning of the generic device class is not supported currently. Hence when the
 *  netlist is copied, the device class attributes will remain, but the functionality is lost.
 */
class GenericDeviceClass
  : public db::DeviceClass
{
public:
  GenericDeviceClass ()
    : db::DeviceClass (), m_supports_parallel_combination (true), m_supports_serial_combination (true)
  {
    //  .. nothing yet ..
  }

  virtual bool combine_devices (db::Device *a, db::Device *b) const
  {
    if (cb_combine_devices.can_issue ()) {
      return cb_combine_devices.issue<db::DeviceClass, bool, db::Device *, db::Device *> (&db::DeviceClass::combine_devices, a, b);
    } else {
      return db::DeviceClass::combine_devices (a, b);
    }
  }

  virtual bool supports_parallel_combination () const
  {
    return m_supports_parallel_combination;
  }

  virtual bool supports_serial_combination () const
  {
    return m_supports_serial_combination;
  }

  void set_supports_parallel_combination (bool f)
  {
    m_supports_parallel_combination = f;
  }

  void set_supports_serial_combination (bool f)
  {
    m_supports_serial_combination = f;
  }

  void equivalent_terminal_id (size_t tid, size_t equiv_tid)
  {
    m_equivalent_terminal_ids.insert (std::make_pair (tid, equiv_tid));
  }

  virtual size_t normalize_terminal_id (size_t tid) const
  {
    std::map<size_t, size_t>::const_iterator ntid = m_equivalent_terminal_ids.find (tid);
    if (ntid != m_equivalent_terminal_ids.end ()) {
      return ntid->second;
    } else {
      return tid;
    }
  }

  gsi::Callback cb_combine_devices;

private:
  bool m_supports_parallel_combination;
  bool m_supports_serial_combination;
  std::map<size_t, size_t> m_equivalent_terminal_ids;
};

}

static db::Net *create_net (db::Circuit *c, const std::string &name)
{
  db::Net *n = new db::Net ();
  c->add_net (n);
  n->set_name (name);
  return n;
}

static db::Device *create_device1 (db::Circuit *c, db::DeviceClass *dc, const std::string &name)
{
  db::Device *d = new db::Device (dc, name);
  c->add_device (d);
  return d;
}

static db::SubCircuit *create_subcircuit1 (db::Circuit *c, db::Circuit *cc, const std::string &name)
{
  db::SubCircuit *sc = new db::SubCircuit (cc, name);
  c->add_subcircuit (sc);
  return sc;
}

static db::Net *circuit_net_for_pin (db::Circuit *c, const db::Pin *pin)
{
  return pin ? c->net_for_pin (pin->id ()) : 0;
}

static const db::Net *circuit_net_for_pin_const (const db::Circuit *c, const db::Pin *pin)
{
  return pin ? c->net_for_pin (pin->id ()) : 0;
}

static void circuit_connect_pin1 (db::Circuit *c, const db::Pin *pin, db::Net *net)
{
  if (pin) {
    c->connect_pin (pin->id (), net);
  }
}

static void circuit_disconnect_pin (db::Circuit *c, size_t pin_id)
{
  c->connect_pin (pin_id, 0);
}

static void circuit_disconnect_pin1 (db::Circuit *c, const db::Pin *pin)
{
  if (pin) {
    c->connect_pin (pin->id (), 0);
  }
}

static db::Pin *create_pin (db::Circuit *circuit, const std::string &name)
{
  return & circuit->add_pin (name);
}

static std::vector<db::Net *>
nets_non_const (const std::vector<const db::Net *> &nc)
{
  std::vector<db::Net *> n;
  n.reserve (nc.size ());
  for (auto i = nc.begin (); i != nc.end (); ++i) {
    n.push_back (const_cast<db::Net *> (*i));
  }

  return n;
}

static std::vector<const db::Net *>
nets_by_name_const (const db::Circuit *circuit, const std::string &name_pattern)
{
  std::vector<const db::Net *> res;
  if (! circuit) {
    return res;
  }

  tl::GlobPattern glob (name_pattern);
  if (circuit->netlist ()) {
    glob.set_case_sensitive (circuit->netlist ()->is_case_sensitive ());
  }
  for (db::Circuit::const_net_iterator n = circuit->begin_nets (); n != circuit->end_nets (); ++n) {
    const db::Net *net = n.operator-> ();
    if (glob.match (net->name ())) {
      res.push_back (net);
    }
  }

  return res;
}

static std::vector<db::Net *>
nets_by_name (db::Circuit *circuit, const std::string &name_pattern)
{
  return nets_non_const (nets_by_name_const (circuit, name_pattern));
}

static std::vector<const db::Net *>
nets_by_name_const_from_netlist (const db::Netlist *netlist, const std::string &name_pattern)
{
  std::vector<const db::Net *> res;
  if (! netlist) {
    return res;
  }

  tl::GlobPattern glob (name_pattern);
  glob.set_case_sensitive (netlist->is_case_sensitive ());
  for (auto c = netlist->begin_circuits (); c != netlist->end_circuits (); ++c) {
    bool is_top = (c->begin_parents () == c->end_parents ());
    for (auto n = c->begin_nets (); n != c->end_nets (); ++n) {
      const db::Net *net = n.operator-> ();
      //  NOTE: we only pick root nets (pin_count == 0 or in top cell)
      if ((is_top || net->pin_count () == 0) && glob.match (net->name ())) {
        res.push_back (net);
      }
    }
  }

  return res;
}

static std::vector<db::Net *>
nets_by_name_from_netlist (db::Netlist *netlist, const std::string &name_pattern)
{
  return nets_non_const (nets_by_name_const_from_netlist (netlist, name_pattern));
}

Class<db::Circuit> decl_dbCircuit (decl_dbNetlistObject, "db", "Circuit",
  gsi::method_ext ("create_pin", &create_pin, gsi::arg ("name"),
    "@brief Creates a new \\Pin object inside the circuit\n"
    "This object will describe a pin of the circuit. A circuit connects "
    "to the outside through such a pin. The pin is added after all existing "
    "pins. For more details see the \\Pin class."
    "\n\n"
    "Starting with version 0.26.8, this method returns a reference to a \\Pin object rather than a copy."
  ) +
  gsi::method ("remove_pin", &db::Circuit::remove_pin, gsi::arg ("id"),
    "@brief Removes the pin with the given ID from the circuit\n"
    "\n"
    "This method has been introduced in version 0.26.2.\n"
  ) +
  gsi::method ("rename_pin", &db::Circuit::rename_pin, gsi::arg ("id"), gsi::arg ("new_name"),
    "@brief Renames the pin with the given ID to 'new_name'\n"
    "\n"
    "This method has been introduced in version 0.26.8.\n"
  ) +
  gsi::iterator ("each_child", (db::Circuit::child_circuit_iterator (db::Circuit::*) ()) &db::Circuit::begin_children, (db::Circuit::child_circuit_iterator (db::Circuit::*) ()) &db::Circuit::end_children,
    "@brief Iterates over the child circuits of this circuit\n"
    "Child circuits are the ones that are referenced from this circuit via subcircuits."
  ) +
  gsi::iterator ("each_child", (db::Circuit::const_child_circuit_iterator (db::Circuit::*) () const) &db::Circuit::begin_children, (db::Circuit::const_child_circuit_iterator (db::Circuit::*) () const) &db::Circuit::end_children,
    "@brief Iterates over the child circuits of this circuit (const version)\n"
    "Child circuits are the ones that are referenced from this circuit via subcircuits."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::iterator ("each_parent", (db::Circuit::parent_circuit_iterator (db::Circuit::*) ()) &db::Circuit::begin_parents, (db::Circuit::parent_circuit_iterator (db::Circuit::*) ()) &db::Circuit::end_parents,
    "@brief Iterates over the parent circuits of this circuit\n"
    "Child circuits are the ones that are referencing this circuit via subcircuits."
  ) +
  gsi::iterator ("each_parent", (db::Circuit::const_parent_circuit_iterator (db::Circuit::*) () const) &db::Circuit::begin_parents, (db::Circuit::const_parent_circuit_iterator (db::Circuit::*) () const) &db::Circuit::end_parents,
    "@brief Iterates over the parent circuits of this circuit (const version)\n"
    "Child circuits are the ones that are referencing this circuit via subcircuits."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("has_refs?", &db::Circuit::has_refs,
    "@brief Returns a value indicating whether the circuit has references\n"
    "A circuit has references if there is at least one subcircuit referring to it."
  ) +
  gsi::iterator ("each_ref", (db::Circuit::refs_iterator (db::Circuit::*) ()) &db::Circuit::begin_refs, (db::Circuit::refs_iterator (db::Circuit::*) ()) &db::Circuit::end_refs,
    "@brief Iterates over the subcircuit objects referencing this circuit\n"
  ) +
  gsi::iterator ("each_ref", (db::Circuit::const_refs_iterator (db::Circuit::*) () const) &db::Circuit::begin_refs, (db::Circuit::const_refs_iterator (db::Circuit::*) () const) &db::Circuit::end_refs,
    "@brief Iterates over the subcircuit objects referencing this circuit (const version)\n"
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::iterator ("each_pin", (db::Circuit::pin_iterator (db::Circuit::*) ()) &db::Circuit::begin_pins, (db::Circuit::pin_iterator (db::Circuit::*) ()) &db::Circuit::end_pins,
    "@brief Iterates over the pins of the circuit"
  ) +
  gsi::iterator ("each_pin", (db::Circuit::const_pin_iterator (db::Circuit::*) () const) &db::Circuit::begin_pins, (db::Circuit::const_pin_iterator (db::Circuit::*) () const) &db::Circuit::end_pins,
    "@brief Iterates over the pins of the circuit (const version)"
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("device_by_id", (db::Device *(db::Circuit::*) (size_t)) &db::Circuit::device_by_id, gsi::arg ("id"),
    "@brief Gets the device object for a given ID.\n"
    "If the ID is not a valid device ID, nil is returned."
  ) +
  gsi::method ("device_by_id", (const db::Device *(db::Circuit::*) (size_t) const) &db::Circuit::device_by_id, gsi::arg ("id"),
    "@brief Gets the device object for a given ID (const version).\n"
    "If the ID is not a valid device ID, nil is returned."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("device_by_name", (db::Device *(db::Circuit::*) (const std::string &)) &db::Circuit::device_by_name, gsi::arg ("name"),
    "@brief Gets the device object for a given name.\n"
    "If the ID is not a valid device name, nil is returned."
  ) +
  gsi::method ("device_by_name", (const db::Device *(db::Circuit::*) (const std::string &) const) &db::Circuit::device_by_name, gsi::arg ("name"),
    "@brief Gets the device object for a given name (const version).\n"
    "If the ID is not a valid device name, nil is returned."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("subcircuit_by_id", (db::SubCircuit *(db::Circuit::*) (size_t)) &db::Circuit::subcircuit_by_id, gsi::arg ("id"),
    "@brief Gets the subcircuit object for a given ID.\n"
    "If the ID is not a valid subcircuit ID, nil is returned."
  ) +
  gsi::method ("subcircuit_by_id", (const db::SubCircuit *(db::Circuit::*) (size_t) const) &db::Circuit::subcircuit_by_id, gsi::arg ("id"),
    "@brief Gets the subcircuit object for a given ID (const version).\n"
    "If the ID is not a valid subcircuit ID, nil is returned."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("subcircuit_by_name", (db::SubCircuit *(db::Circuit::*) (const std::string &)) &db::Circuit::subcircuit_by_name, gsi::arg ("name"),
    "@brief Gets the subcircuit object for a given name.\n"
    "If the ID is not a valid subcircuit name, nil is returned."
  ) +
  gsi::method ("subcircuit_by_name", (const db::SubCircuit *(db::Circuit::*) (const std::string &) const) &db::Circuit::subcircuit_by_name, gsi::arg ("name"),
    "@brief Gets the subcircuit object for a given name (const version).\n"
    "If the ID is not a valid subcircuit name, nil is returned."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("net_by_cluster_id", (db::Net *(db::Circuit::*) (size_t)) &db::Circuit::net_by_cluster_id, gsi::arg ("cluster_id"),
    "@brief Gets the net object corresponding to a specific cluster ID\n"
    "If the ID is not a valid pin cluster ID, nil is returned."
  ) +
  gsi::method ("net_by_name", (db::Net *(db::Circuit::*) (const std::string &)) &db::Circuit::net_by_name, gsi::arg ("name"),
    "@brief Gets the net object for a given name.\n"
    "If the ID is not a valid net name, nil is returned."
  ) +
  gsi::method ("net_by_name", (const db::Net *(db::Circuit::*) (const std::string &) const) &db::Circuit::net_by_name, gsi::arg ("name"),
    "@brief Gets the net object for a given name (const version).\n"
    "If the ID is not a valid net name, nil is returned."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method_ext ("nets_by_name", &nets_by_name, gsi::arg ("name_pattern"),
    "@brief Gets the net objects for a given name filter.\n"
    "The name filter is a glob pattern. This method will return all \\Net objects matching the glob pattern.\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method_ext ("nets_by_name", &nets_by_name_const, gsi::arg ("name_pattern"),
    "@brief Gets the net objects for a given name filter (const version).\n"
    "The name filter is a glob pattern. This method will return all \\Net objects matching the glob pattern.\n"
    "\n\n"
    "This constness variant has been introduced in version 0.27.3"
  ) +
  gsi::method ("pin_by_id", (db::Pin *(db::Circuit::*) (size_t)) &db::Circuit::pin_by_id, gsi::arg ("id"),
    "@brief Gets the \\Pin object corresponding to a specific ID\n"
    "If the ID is not a valid pin ID, nil is returned."
  ) +
  gsi::method ("pin_by_id", (const db::Pin *(db::Circuit::*) (size_t) const) &db::Circuit::pin_by_id, gsi::arg ("id"),
    "@brief Gets the \\Pin object corresponding to a specific ID (const version)\n"
    "If the ID is not a valid pin ID, nil is returned."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("pin_by_name", (db::Pin *(db::Circuit::*) (const std::string &)) &db::Circuit::pin_by_name, gsi::arg ("name"),
    "@brief Gets the \\Pin object corresponding to a specific name\n"
    "If the ID is not a valid pin name, nil is returned."
  ) +
  gsi::method ("pin_by_name", (const db::Pin *(db::Circuit::*) (const std::string &) const) &db::Circuit::pin_by_name, gsi::arg ("name"),
    "@brief Gets the \\Pin object corresponding to a specific name (const version)\n"
    "If the ID is not a valid pin name, nil is returned."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("pin_count", &db::Circuit::pin_count,
    "@brief Gets the number of pins in the circuit"
  ) +
  gsi::method_ext ("create_net", &gsi::create_net, gsi::arg ("name", std::string ()),
    "@brief Creates a new \\Net object inside the circuit\n"
    "This object will describe a net of the circuit. The nets are basically "
    "connections between the different components of the circuit (subcircuits, "
    "devices and pins).\n"
    "\n"
    "A net needs to be filled with references to connect to specific objects. "
    "See the \\Net class for more details."
  ) +
  gsi::method ("remove_net", &db::Circuit::remove_net, gsi::arg ("net"),
    "@brief Removes the given net from the circuit\n"
  ) +
  gsi::method ("join_nets", &db::Circuit::join_nets, gsi::arg ("net"), gsi::arg ("with"),
    "@brief Joins (connects) two nets into one\n"
    "This method will connect the 'with' net with 'net' and remove 'with'.\n"
    "\n"
    "This method has been introduced in version 0.26.4. Starting with version 0.28.13, "
    "net names will be formed from both input names, combining them with as a comma-separated list."
  ) +
  gsi::iterator ("each_net", (db::Circuit::net_iterator (db::Circuit::*) ()) &db::Circuit::begin_nets, (db::Circuit::net_iterator (db::Circuit::*) ()) &db::Circuit::end_nets,
    "@brief Iterates over the nets of the circuit"
  ) +
  gsi::iterator ("each_net", (db::Circuit::const_net_iterator (db::Circuit::*) () const) &db::Circuit::begin_nets, (db::Circuit::const_net_iterator (db::Circuit::*) () const) &db::Circuit::end_nets,
    "@brief Iterates over the nets of the circuit (const version)"
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method_ext ("create_device", &gsi::create_device1, gsi::arg ("device_class"), gsi::arg ("name", std::string ()),
    "@brief Creates a new bound \\Device object inside the circuit\n"
    "This object describes a device of the circuit. The device is already attached "
    "to the device class. The name is optional and is used to identify the device in a "
    "netlist file.\n"
    "\n"
    "For more details see the \\Device class."
  ) +
  gsi::method ("remove_device", &db::Circuit::remove_device, gsi::arg ("device"),
    "@brief Removes the given device from the circuit\n"
  ) +
  gsi::iterator ("each_device", (db::Circuit::device_iterator (db::Circuit::*) ()) &db::Circuit::begin_devices, (db::Circuit::device_iterator (db::Circuit::*) ()) &db::Circuit::end_devices,
    "@brief Iterates over the devices of the circuit"
  ) +
  gsi::iterator ("each_device", (db::Circuit::const_device_iterator (db::Circuit::*) () const) &db::Circuit::begin_devices, (db::Circuit::const_device_iterator (db::Circuit::*) () const) &db::Circuit::end_devices,
    "@brief Iterates over the devices of the circuit (const version)"
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method_ext ("create_subcircuit", &gsi::create_subcircuit1, gsi::arg ("circuit"), gsi::arg ("name", std::string ()),
    "@brief Creates a new bound \\SubCircuit object inside the circuit\n"
    "This object describes an instance of another circuit inside the circuit. The subcircuit is already attached "
    "to the other circuit. The name is optional and is used to identify the subcircuit in a "
    "netlist file.\n"
    "\n"
    "For more details see the \\SubCircuit class."
  ) +
  gsi::method ("remove_subcircuit", &db::Circuit::remove_subcircuit, gsi::arg ("subcircuit"),
    "@brief Removes the given subcircuit from the circuit\n"
  ) +
  gsi::method ("flatten_subcircuit", &db::Circuit::flatten_subcircuit, gsi::arg ("subcircuit"),
    "@brief Flattens a subcircuit\n"
    "This method will substitute the given subcircuit by its contents. The subcircuit is removed "
    "after this."
  ) +
  gsi::iterator ("each_subcircuit", (db::Circuit::subcircuit_iterator (db::Circuit::*) ()) &db::Circuit::begin_subcircuits, (db::Circuit::subcircuit_iterator (db::Circuit::*) ()) &db::Circuit::end_subcircuits,
    "@brief Iterates over the subcircuits of the circuit"
  ) +
  gsi::iterator ("each_subcircuit", (db::Circuit::const_subcircuit_iterator (db::Circuit::*) () const) &db::Circuit::begin_subcircuits, (db::Circuit::const_subcircuit_iterator (db::Circuit::*) () const) &db::Circuit::end_subcircuits,
    "@brief Iterates over the subcircuits of the circuit (const version)"
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("blank", &db::Circuit::blank,
    "@brief Blanks out the circuit\n"
    "This method will remove all the innards of the circuit and just leave the pins. "
    "The pins won't be connected to inside nets anymore, but the circuit can still be "
    "called by subcircuit references. "
    "This method will eventually create a 'circuit abstract' (or black box). It will "
    "set the \\dont_purge flag to mark this circuit as 'intentionally empty'."
  ) +
  gsi::method ("netlist", (db::Netlist *(db::Circuit::*) ()) &db::Circuit::netlist,
    "@brief Gets the netlist object the circuit lives in"
  ) +
  gsi::method ("netlist", (const db::Netlist *(db::Circuit::*) () const) &db::Circuit::netlist,
    "@brief Gets the netlist object the circuit lives in (const version)"
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("name=", &db::Circuit::set_name, gsi::arg ("name"),
    "@brief Sets the name of the circuit"
  ) +
  gsi::method ("name", &db::Circuit::name,
    "@brief Gets the name of the circuit"
  ) +
  gsi::method ("boundary=", &db::Circuit::set_boundary, gsi::arg ("boundary"),
    "@brief Sets the boundary of the circuit"
  ) +
  gsi::method ("boundary", &db::Circuit::boundary,
    "@brief Gets the boundary of the circuit"
  ) +
  gsi::method ("dont_purge", &db::Circuit::dont_purge,
    "@brief Gets a value indicating whether the circuit can be purged on \\Netlist#purge.\n"
  ) +
  gsi::method ("dont_purge=", &db::Circuit::set_dont_purge, gsi::arg ("f"),
    "@brief Sets a value indicating whether the circuit can be purged on \\Netlist#purge.\n"
    "If this attribute is set to true, \\Netlist#purge will never delete this circuit.\n"
    "This flag therefore marks this circuit as 'precious'."
  ) +
  gsi::method ("cell_index=", &db::Circuit::set_cell_index, gsi::arg ("cell_index"),
    "@brief Sets the cell index\n"
    "The cell index relates a circuit with a cell from a layout. It's intended to "
    "hold a cell index number if the netlist was extracted from a layout.\n"
  ) +
  gsi::method ("cell_index", &db::Circuit::cell_index,
    "@brief Gets the cell index of the circuit\n"
    "See \\cell_index= for details.\n"
  ) +
  gsi::method ("net_for_pin", (db::Net *(db::Circuit::*) (size_t)) &db::Circuit::net_for_pin, gsi::arg ("pin_id"),
    "@brief Gets the net object attached to a specific pin.\n"
    "This is the net object inside the circuit which attaches to the given outward-bound pin.\n"
    "This method returns nil if the pin is not connected or the pin ID is invalid."
  ) +
  gsi::method ("net_for_pin", (const db::Net *(db::Circuit::*) (size_t) const) &db::Circuit::net_for_pin, gsi::arg ("pin_id"),
    "@brief Gets the net object attached to a specific pin (const version).\n"
    "This is the net object inside the circuit which attaches to the given outward-bound pin.\n"
    "This method returns nil if the pin is not connected or the pin ID is invalid."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method_ext ("net_for_pin", &gsi::circuit_net_for_pin, gsi::arg ("pin"),
    "@brief Gets the net object attached to a specific pin.\n"
    "This is the net object inside the circuit which attaches to the given outward-bound pin.\n"
    "This method returns nil if the pin is not connected or the pin object is nil."
  ) +
  gsi::method_ext ("net_for_pin", &gsi::circuit_net_for_pin_const, gsi::arg ("pin"),
    "@brief Gets the net object attached to a specific pin (const version).\n"
    "This is the net object inside the circuit which attaches to the given outward-bound pin.\n"
    "This method returns nil if the pin is not connected or the pin object is nil."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8"
  ) +
  gsi::method ("connect_pin", &db::Circuit::connect_pin, gsi::arg ("pin_id"), gsi::arg ("net"),
    "@brief Connects the given pin with the given net.\n"
    "The net must be one inside the circuit. Any previous connected is resolved before this "
    "connection is made. A pin can only be connected to one net at a time."
  ) +
  gsi::method_ext ("connect_pin", &gsi::circuit_connect_pin1, gsi::arg ("pin"), gsi::arg ("net"),
    "@brief Connects the given pin with the given net.\n"
    "The net and the pin must be objects from inside the circuit. Any previous connected is resolved before this "
    "connection is made. A pin can only be connected to one net at a time."
  ) +
  gsi::method_ext ("disconnect_pin", &gsi::circuit_disconnect_pin, gsi::arg ("pin_id"),
    "@brief Disconnects the given pin from any net.\n"
  ) +
  gsi::method_ext ("disconnect_pin", &gsi::circuit_disconnect_pin1, gsi::arg ("pin"),
    "@brief Disconnects the given pin from any net.\n"
  ) +
  gsi::method ("clear", &db::Circuit::clear,
    "@brief Clears the circuit\n"
    "This method removes all objects and clears the other attributes."
  ) +
  gsi::method ("combine_devices", &db::Circuit::combine_devices,
    "@brief Combines devices where possible\n"
    "This method will combine devices that can be combined according "
    "to their device classes 'combine_devices' method.\n"
    "For example, serial or parallel resistors can be combined into "
    "a single resistor.\n"
  ) +
  gsi::method ("purge_nets", &db::Circuit::purge_nets,
    "@brief Purges floating nets.\n"
    "Floating nets are nets with no device or subcircuit attached to. Such floating "
    "nets are removed in this step. If these nets are connected outward to a circuit pin, this "
    "circuit pin is also removed."
  ) +
  gsi::method ("purge_nets_keep_pins", &db::Circuit::purge_nets_keep_pins,
    "@brief Purges floating nets but keep pins.\n"
    "This method will remove floating nets like \\purge_nets, but if these nets are attached "
    "to a pin, the pin will be left disconnected from any net.\n"
    "\n"
    "This method has been introduced in version 0.26.2.\n"
  ),
  "@brief Circuits are the basic building blocks of the netlist\n"
  "A circuit has pins by which it can connect to the outside. Pins are "
  "created using \\create_pin and are represented by the \\Pin class.\n"
  "\n"
  "Furthermore, a circuit manages the components of the netlist. "
  "Components are devices (class \\Device) and subcircuits (class \\SubCircuit). "
  "Devices are basic devices such as resistors or transistors. Subcircuits "
  "are other circuits to which nets from this circuit connect. "
  "Devices are created using the \\create_device method. Subcircuits are "
  "created using the \\create_subcircuit method.\n"
  "\n"
  "Devices are connected through 'terminals', subcircuits are connected through "
  "their pins. Terminals and pins are described by integer ID's in the context of "
  "most methods.\n"
  "\n"
  "Finally, the circuit consists of the nets. Nets connect terminals of devices "
  "and pins of subcircuits or the circuit itself. Nets are created using "
  "\\create_net and are represented by objects of the \\Net class.\n"
  "See there for more about nets.\n"
  "\n"
  "The Circuit object is only valid if the netlist object is alive. "
  "Circuits must be added to a netlist using \\Netlist#add to become "
  "part of the netlist.\n"
  "\n"
  "The Circuit class has been introduced in version 0.26."
);

static void add_circuit (db::Netlist *nl, db::Circuit *c)
{
  tl_assert (c != 0);
  c->keep ();
  nl->add_circuit (c);
}

static void add_device_class (db::Netlist *nl, db::DeviceClass *cl)
{
  tl_assert (cl != 0);
  cl->keep ();
  nl->add_device_class (cl);
}

static void write_netlist (const db::Netlist *nl, const std::string &file, db::NetlistWriter *writer, const std::string &description)
{
  tl_assert (writer != 0);
  tl::OutputStream os (file);
  writer->write (os, *nl, description);
}

static void read_netlist (db::Netlist *nl, const std::string &file, db::NetlistReader *reader)
{
  tl_assert (reader != 0);
  tl::InputStream os (file);
  reader->read (os, *nl);
}

static void flatten_circuit_by_name (db::Netlist *nl, const std::string &name_pattern)
{
  std::vector<db::Circuit *> circuits_to_flatten;
  tl::GlobPattern pat (name_pattern);
  for (db::Netlist::circuit_iterator c = nl->begin_circuits (); c != nl->end_circuits (); ++c) {
    if (pat.match (c->name ())) {
      circuits_to_flatten.push_back (c.operator-> ());
    }
  }

  nl->flatten_circuits (circuits_to_flatten);
}

static void blank_circuit_by_name (db::Netlist *nl, const std::string &name_pattern)
{
  std::list<tl::weak_ptr<db::Circuit> > circuits_to_blank;
  tl::GlobPattern pat (name_pattern);
  for (db::Netlist::circuit_iterator c = nl->begin_circuits (); c != nl->end_circuits (); ++c) {
    if (pat.match (c->name ())) {
      circuits_to_blank.push_back (c.operator-> ());
    }
  }

  for (std::list<tl::weak_ptr<db::Circuit> >::iterator c = circuits_to_blank.begin (); c != circuits_to_blank.end (); ++c) {
    if (c->get ()) {
      (*c)->blank ();
    }
  }
}

static std::vector<db::Circuit *>
circuits_by_name (db::Netlist *netlist, const std::string &name_pattern)
{
  std::vector<db::Circuit *> res;
  if (! netlist) {
    return res;
  }

  tl::GlobPattern glob (name_pattern);
  glob.set_case_sensitive (netlist->is_case_sensitive ());

  for (db::Netlist::circuit_iterator c = netlist->begin_circuits (); c != netlist->end_circuits (); ++c) {
    db::Circuit *circuit = c.operator-> ();
    if (glob.match (circuit->name ())) {
      res.push_back (circuit);
    }
  }

  return res;
}

static std::vector<const db::Circuit *>
circuits_by_name_const (const db::Netlist *netlist, const std::string &name_pattern)
{
  std::vector<const db::Circuit *> res;
  if (! netlist) {
    return res;
  }

  tl::GlobPattern glob (name_pattern);
  glob.set_case_sensitive (netlist->is_case_sensitive ());

  for (db::Netlist::const_circuit_iterator c = netlist->begin_circuits (); c != netlist->end_circuits (); ++c) {
    const db::Circuit *circuit = c.operator-> ();
    if (glob.match (circuit->name ())) {
      res.push_back (circuit);
    }
  }

  return res;
}

Class<db::Netlist> decl_dbNetlist ("db", "Netlist",
  gsi::method ("is_case_sensitive?", &db::Netlist::is_case_sensitive,
    "@brief Returns a value indicating whether the netlist names are case sensitive\n"
    "This method has been added in version 0.27.3.\n"
  ) +
  gsi::method ("case_sensitive=", &db::Netlist::set_case_sensitive, gsi::arg ("cs"),
    "@brief Sets a value indicating whether the netlist names are case sensitive\n"
    "This method has been added in version 0.27.3.\n"
  ) +
  gsi::method_ext ("add", &gsi::add_circuit, gsi::arg ("circuit"),
    "@brief Adds the circuit to the netlist\n"
    "This method will add the given circuit object to the netlist. "
    "After the circuit has been added, it will be owned by the netlist."
  ) +
  gsi::method ("remove", &db::Netlist::remove_circuit, gsi::arg ("circuit"),
    "@brief Removes the given circuit object from the netlist\n"
    "After the circuit has been removed, the object becomes invalid and cannot be used further. "
    "A circuit with references (see \\has_refs?) should not be removed as the "
    "subcircuits calling it would afterwards point to nothing."
  ) +
  gsi::method ("purge_circuit", &db::Netlist::purge_circuit, gsi::arg ("circuit"),
    "@brief Removes the given circuit object and all child circuits which are not used otherwise from the netlist\n"
    "After the circuit has been removed, the object becomes invalid and cannot be used further. "
    "A circuit with references (see \\has_refs?) should not be removed as the "
    "subcircuits calling it would afterwards point to nothing."
  ) +
  gsi::method ("flatten", &db::Netlist::flatten,
    "@brief Flattens all circuits of the netlist\n"
    "After calling this method, only the top circuits will remain."
  ) +
  gsi::method ("flatten_circuits", &db::Netlist::flatten_circuits,
    "@brief Flattens all given circuits of the netlist\n"
    "This method is equivalent to calling \\flatten_circuit for all given circuits, but more efficient.\n"
    "\n"
    "This method has been introduced in version 0.26.1"
  ) +
  gsi::method ("flatten_circuit", &db::Netlist::flatten_circuit, gsi::arg ("circuit"),
    "@brief Flattens a subcircuit\n"
    "This method will substitute all instances (subcircuits) of the given circuit by its "
    "contents. After this, the circuit is removed."
  ) +
  gsi::method_ext ("flatten_circuit", &flatten_circuit_by_name, gsi::arg ("pattern"),
    "@brief Flattens circuits matching a certain pattern\n"
    "This method will substitute all instances (subcircuits) of all circuits with names matching the given name pattern. "
    "The name pattern is a glob expression. For example, 'flatten_circuit(\"np*\")' will flatten all circuits with names "
    "starting with 'np'."
  ) +
  gsi::method_ext ("blank_circuit", &blank_circuit_by_name, gsi::arg ("pattern"),
    "@brief Blanks circuits matching a certain pattern\n"
    "This method will erase everything from inside the circuits matching the given pattern. It will only leave pins which are "
    "not connected to any net. Hence, this method forms 'abstract' or black-box circuits which can be instantiated through "
    "subcircuits like the former ones, but are empty shells.\n"
    "The name pattern is a glob expression. For example, 'blank_circuit(\"np*\")' will blank out all circuits with names "
    "starting with 'np'.\n"
    "\n"
    "For more details see \\Circuit#blank which is the corresponding method on the actual object."
  ) +
  gsi::method ("circuit_by_cell_index", (db::Circuit *(db::Netlist::*) (db::cell_index_type)) &db::Netlist::circuit_by_cell_index, gsi::arg ("cell_index"),
    "@brief Gets the circuit object for a given cell index.\n"
    "If the cell index is not valid or no circuit is registered with this index, nil is returned."
  ) +
  gsi::method ("circuit_by_cell_index", (const db::Circuit *(db::Netlist::*) (db::cell_index_type) const) &db::Netlist::circuit_by_cell_index, gsi::arg ("cell_index"),
    "@brief Gets the circuit object for a given cell index (const version).\n"
    "If the cell index is not valid or no circuit is registered with this index, nil is returned."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8."
  ) +
  gsi::method ("circuit_by_name", (db::Circuit *(db::Netlist::*) (const std::string &)) &db::Netlist::circuit_by_name, gsi::arg ("name"),
    "@brief Gets the circuit object for a given name.\n"
    "If the name is not a valid circuit name, nil is returned."
  ) +
  gsi::method ("circuit_by_name", (const db::Circuit *(db::Netlist::*) (const std::string &) const) &db::Netlist::circuit_by_name, gsi::arg ("name"),
    "@brief Gets the circuit object for a given name (const version).\n"
    "If the name is not a valid circuit name, nil is returned."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8."
  ) +
  gsi::method_ext ("circuits_by_name", &circuits_by_name, gsi::arg ("name_pattern"),
    "@brief Gets the circuit objects for a given name filter.\n"
    "The name filter is a glob pattern. This method will return all \\Circuit objects matching the glob pattern.\n"
    "\n"
    "This method has been introduced in version 0.26.4.\n"
  ) +
  gsi::method_ext ("circuits_by_name", &circuits_by_name_const, gsi::arg ("name_pattern"),
    "@brief Gets the circuit objects for a given name filter (const version).\n"
    "The name filter is a glob pattern. This method will return all \\Circuit objects matching the glob pattern.\n"
    "\n\n"
    "This constness variant has been introduced in version 0.26.8."
  ) +
  gsi::method_ext ("nets_by_name", &nets_by_name_from_netlist, gsi::arg ("name_pattern"),
    "@brief Gets the net objects for a given name filter.\n"
    "The name filter is a glob pattern. This method will return all \\Net objects matching the glob pattern.\n"
    "\n"
    "This method has been introduced in version 0.28.4.\n"
  ) +
  gsi::method_ext ("nets_by_name", &nets_by_name_const_from_netlist, gsi::arg ("name_pattern"),
    "@brief Gets the net objects for a given name filter (const version).\n"
    "The name filter is a glob pattern. This method will return all \\Net objects matching the glob pattern.\n"
    "\n\n"
    "This constness variant has been introduced in version 0.28.4."
  ) +
  gsi::iterator ("each_circuit_top_down", (db::Netlist::top_down_circuit_iterator (db::Netlist::*) ()) &db::Netlist::begin_top_down, (db::Netlist::top_down_circuit_iterator (db::Netlist::*) ()) &db::Netlist::end_top_down,
    "@brief Iterates over the circuits top-down\n"
    "Iterating top-down means the parent circuits come before the child circuits. "
    "The first \\top_circuit_count circuits are top circuits - i.e. those which are not referenced by other circuits."
  ) +
  gsi::iterator ("each_circuit_top_down", (db::Netlist::const_top_down_circuit_iterator (db::Netlist::*) () const) &db::Netlist::begin_top_down, (db::Netlist::const_top_down_circuit_iterator (db::Netlist::*) () const) &db::Netlist::end_top_down,
    "@brief Iterates over the circuits top-down (const version)\n"
    "Iterating top-down means the parent circuits come before the child circuits. "
    "The first \\top_circuit_count circuits are top circuits - i.e. those which are not referenced by other circuits."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8."
  ) +
  gsi::iterator ("each_circuit_bottom_up", (db::Netlist::bottom_up_circuit_iterator (db::Netlist::*) ()) &db::Netlist::begin_bottom_up, (db::Netlist::bottom_up_circuit_iterator (db::Netlist::*) ()) &db::Netlist::end_bottom_up,
    "@brief Iterates over the circuits bottom-up\n"
    "Iterating bottom-up means the parent circuits come after the child circuits. "
    "This is the basically the reverse order as delivered by \\each_circuit_top_down."
  ) +
  gsi::iterator ("each_circuit_bottom_up", (db::Netlist::const_bottom_up_circuit_iterator (db::Netlist::*) () const) &db::Netlist::begin_bottom_up, (db::Netlist::const_bottom_up_circuit_iterator (db::Netlist::*) () const) &db::Netlist::end_bottom_up,
    "@brief Iterates over the circuits bottom-up (const version)\n"
    "Iterating bottom-up means the parent circuits come after the child circuits. "
    "This is the basically the reverse order as delivered by \\each_circuit_top_down."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8."
  ) +
  gsi::method ("top_circuit_count", &db::Netlist::top_circuit_count,
    "@brief Gets the number of top circuits.\n"
    "Top circuits are those which are not referenced by other circuits via subcircuits. "
    "A well-formed netlist has a single top circuit."
  ) +
  gsi::iterator ("each_circuit", (db::Netlist::circuit_iterator (db::Netlist::*) ()) &db::Netlist::begin_circuits, (db::Netlist::circuit_iterator (db::Netlist::*) ()) &db::Netlist::end_circuits,
    "@brief Iterates over the circuits of the netlist"
  ) +
  gsi::iterator ("each_circuit", (db::Netlist::const_circuit_iterator (db::Netlist::*) () const) &db::Netlist::begin_circuits, (db::Netlist::const_circuit_iterator (db::Netlist::*) () const) &db::Netlist::end_circuits,
    "@brief Iterates over the circuits of the netlist (const version)"
    "\n\n"
    "This constness variant has been introduced in version 0.26.8."
  ) +
  gsi::method_ext ("add", &gsi::add_device_class, gsi::arg ("device_class"),
    "@brief Adds the device class to the netlist\n"
    "This method will add the given device class object to the netlist. "
    "After the device class has been added, it will be owned by the netlist."
  ) +
  gsi::method ("remove", &db::Netlist::remove_device_class, gsi::arg ("device_class"),
    "@brief Removes the given device class object from the netlist\n"
    "After the object has been removed, it becomes invalid and cannot be used further. "
    "Use this method with care as it may corrupt the internal structure of the netlist. "
    "Only use this method when device refers to this device class."
  ) +
  gsi::method ("device_class_by_name", (db::DeviceClass *(db::Netlist::*) (const std::string &)) &db::Netlist::device_class_by_name, gsi::arg ("name"),
    "@brief Gets the device class for a given name.\n"
    "If the name is not a valid device class name, nil is returned."
  ) +
  gsi::method ("device_class_by_name", (const db::DeviceClass *(db::Netlist::*) (const std::string &) const) &db::Netlist::device_class_by_name, gsi::arg ("name"),
    "@brief Gets the device class for a given name (const version).\n"
    "If the name is not a valid device class name, nil is returned."
    "\n\n"
    "This constness variant has been introduced in version 0.26.8."
  ) +
  gsi::iterator ("each_device_class", (db::Netlist::device_class_iterator (db::Netlist::*) ()) &db::Netlist::begin_device_classes, (db::Netlist::device_class_iterator (db::Netlist::*) ()) &db::Netlist::end_device_classes,
    "@brief Iterates over the device classes of the netlist"
  ) +
  gsi::iterator ("each_device_class", (db::Netlist::const_device_class_iterator (db::Netlist::*) () const) &db::Netlist::begin_device_classes, (db::Netlist::const_device_class_iterator (db::Netlist::*) () const) &db::Netlist::end_device_classes,
    "@brief Iterates over the device classes of the netlist (const version)"
    "\n\n"
    "This constness variant has been introduced in version 0.26.8."
  ) +
  gsi::method ("to_s", &db::Netlist::to_string,
    "@brief Converts the netlist to a string representation.\n"
    "This method is intended for test purposes mainly."
  ) +
  gsi::method ("from_s", &db::Netlist::from_string, gsi::arg ("str"),
    "@brief Reads the netlist from a string representation.\n"
    "This method is intended for test purposes mainly. It turns a string returned by \\to_s back into "
    "a netlist. Note that the device classes must be created before as they are not persisted inside the string."
  ) +
  gsi::method ("combine_devices", &db::Netlist::combine_devices,
    "@brief Combines devices where possible\n"
    "This method will combine devices that can be combined according "
    "to their device classes 'combine_devices' method.\n"
    "For example, serial or parallel resistors can be combined into "
    "a single resistor.\n"
  ) +
  gsi::method ("make_top_level_pins", &db::Netlist::make_top_level_pins,
    "@brief Creates pins for top-level circuits.\n"
    "This method will turn all named nets of top-level circuits (such that are not "
    "referenced by subcircuits) into pins. This method can be used before purge to "
    "avoid that purge will remove nets which are directly connecting to subcircuits."
  ) +
  gsi::method ("purge", &db::Netlist::purge,
    "@brief Purge unused nets, circuits and subcircuits.\n"
    "This method will purge all nets which return \\floating == true. Circuits which don't have any "
    "nets (or only floating ones) and removed. Their subcircuits are disconnected.\n"
    "This method respects the \\Circuit#dont_purge attribute and will never delete circuits "
    "with this flag set."
  ) +
  gsi::method ("purge_nets", &db::Netlist::purge_nets,
    "@brief Purges floating nets.\n"
    "Floating nets can be created as effect of reconnections of devices or pins. "
    "This method will eliminate all nets that make less than two connections."
  ) +
  gsi::method ("simplify", &db::Netlist::simplify,
    "@brief Convenience method that combines the simplification.\n"
    "This method is a convenience method that runs \\make_top_level_pins, \\purge, \\combine_devices and \\purge_nets."
  ) +
  gsi::method_ext ("read", &read_netlist, gsi::arg ("file"), gsi::arg ("reader"),
    "@brief Writes the netlist to the given file using the given reader object to parse the file\n"
    "See \\NetlistSpiceReader for an example for a parser. "
  ) +
  gsi::method_ext ("write", &write_netlist, gsi::arg ("file"), gsi::arg ("writer"), gsi::arg ("description", std::string ()),
    "@brief Writes the netlist to the given file using the given writer object to format the file\n"
    "See \\NetlistSpiceWriter for an example for a formatter. "
    "The description is an arbitrary text which will be put into the file somewhere at the beginning."
  ),
  "@brief The netlist top-level class\n"
  "A netlist is a hierarchical structure of circuits. At least one circuit is the "
  "top-level circuit, other circuits may be referenced as subcircuits.\n"
  "Circuits are created with \\create_circuit and are represented by objects of the \\Circuit class.\n"
  "\n"
  "Beside circuits, the netlist manages device classes. Device classes describe specific "
  "types of devices. Device classes are represented by objects of the \\DeviceClass class "
  "and are created using \\create_device_class.\n"
  "\n"
  "The netlist class has been introduced with version 0.26."
);

/**
 *  @brief A SPICE writer delegate base class for reimplementation
 */
class NetlistSpiceWriterDelegateImpl
  : public db::NetlistSpiceWriterDelegate, public gsi::ObjectBase
{
public:
  NetlistSpiceWriterDelegateImpl ()
    : db::NetlistSpiceWriterDelegate ()
  {
    //  .. nothing yet ..
  }

  virtual void write_header () const
  {
    if (cb_write_header.can_issue ()) {
      cb_write_header.issue<db::NetlistSpiceWriterDelegate> (&db::NetlistSpiceWriterDelegate::write_header);
    } else {
      db::NetlistSpiceWriterDelegate::write_header ();
    }
  }

  virtual void write_device_intro (const db::DeviceClass &ccls) const
  {
    reimpl_write_device_intro (const_cast<db::DeviceClass &> (ccls));
  }

  //  NOTE: we pass non-const refs to Ruby/Python - everything else is a bit of a nightmare.
  //  Still that's not really clean. Just say, the implementation promises not to change the objects.
  void reimpl_write_device_intro (db::DeviceClass &cls) const
  {
    if (cb_write_device_intro.can_issue ()) {
      cb_write_device_intro.issue<NetlistSpiceWriterDelegateImpl, db::DeviceClass &> (&NetlistSpiceWriterDelegateImpl::org_write_device_intro, const_cast<db::DeviceClass &> (cls));
    } else {
      org_write_device_intro (cls);
    }
  }

  void org_write_device_intro (db::DeviceClass &cls) const
  {
    db::NetlistSpiceWriterDelegate::write_device_intro (cls);
  }

  virtual void write_device (const db::Device &cdev) const
  {
    reimpl_write_device (const_cast<db::Device &> (cdev));
  }

  //  NOTE: we pass non-const refs to Ruby/Python - everthing else is a bit of a nightmare.
  //  Still that's not really clean. Just say, the implementation promises not to change the objects.
  void reimpl_write_device (db::Device &dev) const
  {
    if (cb_write_device.can_issue ()) {
      cb_write_device.issue<NetlistSpiceWriterDelegateImpl, db::Device &> (&NetlistSpiceWriterDelegateImpl::org_write_device, dev);
    } else {
      org_write_device (dev);
    }
  }

  void org_write_device (db::Device &dev) const
  {
    db::NetlistSpiceWriterDelegate::write_device (dev);
  }

  void org_write_header () const
  {
    db::NetlistSpiceWriterDelegate::write_header ();
  }

  gsi::Callback cb_write_header;
  gsi::Callback cb_write_device_intro;
  gsi::Callback cb_write_device;
};

Class<NetlistSpiceWriterDelegateImpl> db_NetlistSpiceWriterDelegate ("db", "NetlistSpiceWriterDelegate",
  gsi::method ("write_header", &NetlistSpiceWriterDelegateImpl::org_write_header, "@hide") +
  gsi::callback ("write_header", &NetlistSpiceWriterDelegateImpl::write_header, &NetlistSpiceWriterDelegateImpl::cb_write_header,
    "@brief Writes the text at the beginning of the SPICE netlist\n"
    "Reimplement this method to insert your own text at the beginning of the file"
  ) +
  gsi::method ("write_device_intro", &NetlistSpiceWriterDelegateImpl::org_write_device_intro, "@hide") +
  gsi::callback ("write_device_intro", &NetlistSpiceWriterDelegateImpl::reimpl_write_device_intro, &NetlistSpiceWriterDelegateImpl::cb_write_device_intro, gsi::arg ("device_class"),
    "@brief Inserts a text for the given device class\n"
    "Reimplement this method to insert your own text at the beginning of the file for the given device class"
  ) +
  gsi::method ("write_device", &NetlistSpiceWriterDelegateImpl::org_write_device, gsi::arg ("device"), "@hide") +
  gsi::callback ("write_device", &NetlistSpiceWriterDelegateImpl::reimpl_write_device, &NetlistSpiceWriterDelegateImpl::cb_write_device, gsi::arg ("device"),
    "@brief Inserts a text for the given device\n"
    "Reimplement this method to write the given device in the desired way. "
    "The default implementation will utilize the device class information to write native SPICE "
    "elements for the devices."
  ) +
  gsi::method ("emit_comment", &NetlistSpiceWriterDelegateImpl::emit_comment, gsi::arg ("comment"),
    "@brief Writes the given comment into the file"
  ) +
  gsi::method ("emit_line", &NetlistSpiceWriterDelegateImpl::emit_line, gsi::arg ("line"),
    "@brief Writes the given line into the file"
  ) +
  gsi::method ("net_to_string", &NetlistSpiceWriterDelegateImpl::net_to_string, gsi::arg ("net"),
    "@brief Gets the node ID for the given net\n"
    "The node ID is a numeric string instead of the full name of the net. Numeric IDs are used within "
    "SPICE netlist because they are usually shorter.\n"
  ) +
  gsi::method ("format_name", &NetlistSpiceWriterDelegateImpl::format_name, gsi::arg ("name"),
    "@brief Formats the given name in a SPICE-compatible way"
  ),
  "@brief Provides a delegate for the SPICE writer for doing special formatting for devices\n"
  "Supply a customized class to provide a specialized writing scheme for devices. "
  "You need a customized class if you want to implement special devices or you want to use "
  "subcircuits rather than the built-in devices.\n"
  "\n"
  "See \\NetlistSpiceWriter for more details.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

namespace {

class NetlistSpiceWriterWithOwnership
  : public db::NetlistSpiceWriter
{
public:
  NetlistSpiceWriterWithOwnership (NetlistSpiceWriterDelegateImpl *delegate)
    : db::NetlistSpiceWriter (delegate), m_ownership (delegate)
  {
    if (delegate) {
      delegate->keep ();
    }
  }

private:
  tl::shared_ptr<NetlistSpiceWriterDelegateImpl> m_ownership;
};

}

db::NetlistSpiceWriter *new_spice_writer ()
{
  return new db::NetlistSpiceWriter ();
}

db::NetlistSpiceWriter *new_spice_writer2 (NetlistSpiceWriterDelegateImpl *delegate)
{
  return new NetlistSpiceWriterWithOwnership (delegate);
}

Class<db::NetlistWriter> db_NetlistWriter ("db", "NetlistWriter",
  gsi::Methods (),
  "@brief Base class for netlist writers\n"
  "This class is provided as a base class for netlist writers. It is not intended for reimplementation on script level, but used internally as an interface.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::NetlistSpiceWriter> db_NetlistSpiceWriter (db_NetlistWriter, "db", "NetlistSpiceWriter",
  gsi::constructor ("new", &new_spice_writer,
    "@brief Creates a new writer without delegate.\n"
  ) +
  gsi::constructor ("new", &new_spice_writer2,
    "@brief Creates a new writer with a delegate.\n"
  ) +
  gsi::method ("use_net_names=", &db::NetlistSpiceWriter::set_use_net_names, gsi::arg ("f"),
    "@brief Sets a value indicating whether to use net names (true) or net numbers (false).\n"
    "The default is to use net numbers."
  ) +
  gsi::method ("use_net_names?", &db::NetlistSpiceWriter::use_net_names,
    "@brief Gets a value indicating whether to use net names (true) or net numbers (false).\n"
  ) +
  gsi::method ("with_comments=", &db::NetlistSpiceWriter::set_with_comments, gsi::arg ("f"),
    "@brief Sets a value indicating whether to embed comments for position etc. (true) or not (false).\n"
    "The default is to embed comments."
  ) +
  gsi::method ("with_comments?", &db::NetlistSpiceWriter::with_comments,
    "@brief Gets a value indicating whether to embed comments for position etc. (true) or not (false).\n"
  ),
  "@brief Implements a netlist writer for the SPICE format.\n"
  "Provide a delegate for customizing the way devices are written.\n"
  "\n"
  "Use the SPICE writer like this:\n"
  "\n"
  "@code\n"
  "writer = RBA::NetlistSpiceWriter::new\n"
  "netlist.write(path, writer)\n"
  "@/code\n"
  "\n"
  "You can give a custom description for the headline:\n"
  "\n"
  "@code\n"
  "writer = RBA::NetlistSpiceWriter::new\n"
  "netlist.write(path, writer, \"A custom description\")\n"
  "@/code\n"
  "\n"
  "To customize the output, you can use a device writer delegate.\n"
  "The delegate is an object of a class derived from \\NetlistSpiceWriterDelegate which "
  "reimplements several methods to customize the following parts:\n"
  "\n"
  "@ul\n"
  "@li A global header (\\NetlistSpiceWriterDelegate#write_header): this method is called to print the part right after the headline @/li\n"
  "@li A per-device class header (\\NetlistSpiceWriterDelegate#write_device_intro): this method is called for every device class and may print device-class specific headers (e.g. model definitions) @/li\n"
  "@li Per-device output: this method (\\NetlistSpiceWriterDelegate#write_device): this method is called for every device and may print the device statement(s) in a specific way. @/li\n"
  "@/ul\n"
  "\n"
  "The delegate must use \\NetlistSpiceWriterDelegate#emit_line to print a line, \\NetlistSpiceWriterDelegate#emit_comment to print a comment etc.\n"
  "For more method see \\NetlistSpiceWriterDelegate.\n"
  "\n"
  "A sample with a delegate is this:\n"
  "\n"
  "@code\n"
  "class MyDelegate < RBA::NetlistSpiceWriterDelegate\n"
  "\n"
  "  def write_header\n"
  "    emit_line(\"*** My special header\")\n"
  "  end\n"
  "\n"
  "  def write_device_intro(cls)\n"
  "    emit_comment(\"My intro for class \" + cls.name)\n"
  "  end\n"
  "\n"
  "  def write_device(dev)\n"
  "    if dev.device_class.name != \"MYDEVICE\"\n"
  "      emit_comment(\"Terminal #1: \" + net_to_string(dev.net_for_terminal(0)))\n"
  "      emit_comment(\"Terminal #2: \" + net_to_string(dev.net_for_terminal(1)))\n"
  "      super(dev)\n"
  "      emit_comment(\"After device \" + dev.expanded_name)\n"
  "    else\n"
  "      super(dev)\n"
  "    end\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "# write the netlist with delegate:\n"
  "writer = RBA::NetlistSpiceWriter::new(MyDelegate::new)\n"
  "netlist.write(path, writer)\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::NetlistReader> db_NetlistReader ("db", "NetlistReader",
  gsi::Methods (),
  "@brief Base class for netlist readers\n"
  "This class is provided as a base class for netlist readers. It is not intended for reimplementation on script level, but used internally as an interface.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

/**
 *  @brief A helper class wrapping the return values for NetlistSpiceReaderDelegateImpl::parse_element
 */
class ParseElementData
{
public:
  ParseElementData () : m_value (0.0) { }

  const std::string &model_name () const { return m_model; }
  std::string &model_name_nc () { return m_model; }
  void set_model_name (const std::string &model) { m_model = model; }
  double value () const { return m_value; }
  double &value_nc () { return m_value; }
  void set_value (double value) { m_value = value; }
  const std::vector<std::string> &net_names () const { return m_net_names; }
  std::vector<std::string> &net_names_nc () { return m_net_names; }
  void set_net_names (const std::vector<std::string> &nn) { m_net_names = nn; }
  const db::NetlistSpiceReader::parameters_type &parameters () const { return m_parameters; }
  db::NetlistSpiceReader::parameters_type &parameters_nc () { return m_parameters; }
  void set_parameters (const db::NetlistSpiceReader::parameters_type &parameters) { m_parameters = parameters; }

private:
  std::string m_model;
  double m_value;
  std::vector<std::string> m_net_names;
  db::NetlistSpiceReader::parameters_type m_parameters;
};

/**
 *  @brief A helper class for the return values of NetlistSpiceReaderDelegateImpl::parse_element_components
 */
class ParseElementComponentsData
{
public:
  ParseElementComponentsData () { }

  const std::vector<std::string> &strings () const { return m_strings; }
  std::vector<std::string> &strings_nc () { return m_strings; }
  void set_strings (const std::vector<std::string> &nn) { m_strings = nn; }
  const db::NetlistSpiceReader::parameters_type &parameters () const { return m_parameters; }
  db::NetlistSpiceReader::parameters_type &parameters_nc () { return m_parameters; }
  void set_parameters (const db::NetlistSpiceReader::parameters_type &parameters) { m_parameters = parameters; }

private:
  std::vector<std::string> m_strings;
  db::NetlistSpiceReader::parameters_type m_parameters;
};

/**
 *  @brief A SPICE reader delegate base class for reimplementation
 */
class NetlistSpiceReaderDelegateImpl
  : public db::NetlistSpiceReaderDelegate, public gsi::ObjectBase
{
public:
  NetlistSpiceReaderDelegateImpl ()
    : db::NetlistSpiceReaderDelegate (), mp_variables (0)
  {
    //  .. nothing yet ..
  }

  virtual void error (const std::string &msg)
  {
    //  doing this avoids passing exceptions through script code which spoils the message
    //  (the exception will be decorated with a stack trace). TODO: a better solution was
    //  to define a specific exception type for "raw exception".
    m_error = msg;
    db::NetlistSpiceReaderDelegate::error (msg);
  }

  virtual void start (db::Netlist *netlist)
  {
    try {
      m_error.clear ();
      if (cb_start.can_issue ()) {
        cb_start.issue<db::NetlistSpiceReaderDelegate, db::Netlist *> (&db::NetlistSpiceReaderDelegate::start, netlist);
      } else {
        db::NetlistSpiceReaderDelegate::start (netlist);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
    }
  }

  virtual void finish (db::Netlist *netlist)
  {
    try {
      m_error.clear ();
      if (cb_finish.can_issue ()) {
        cb_finish.issue<db::NetlistSpiceReaderDelegate, db::Netlist *> (&db::NetlistSpiceReaderDelegate::finish, netlist);
      } else {
        db::NetlistSpiceReaderDelegate::finish (netlist);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
    }
  }

  virtual bool control_statement (const std::string &line)
  {
    try {
      m_error.clear ();
      if (cb_control_statement.can_issue ()) {
        return cb_control_statement.issue<db::NetlistSpiceReaderDelegate, bool, const std::string &> (&db::NetlistSpiceReaderDelegate::control_statement, line);
      } else {
        return db::NetlistSpiceReaderDelegate::control_statement (line);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
      return false;
    }
  }

  virtual bool wants_subcircuit (const std::string &circuit_name)
  {
    try {
      m_error.clear ();
      if (cb_wants_subcircuit.can_issue ()) {
        return cb_wants_subcircuit.issue<db::NetlistSpiceReaderDelegate, bool, const std::string &> (&db::NetlistSpiceReaderDelegate::wants_subcircuit, circuit_name);
      } else {
        return db::NetlistSpiceReaderDelegate::wants_subcircuit (circuit_name);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
      return false;
    }
  }

  virtual std::string translate_net_name (const std::string &nn)
  {
    try {
      m_error.clear ();
      if (cb_translate_net_name.can_issue ()) {
        return cb_translate_net_name.issue<db::NetlistSpiceReaderDelegate, std::string, const std::string &> (&db::NetlistSpiceReaderDelegate::translate_net_name, nn);
      } else {
        return db::NetlistSpiceReaderDelegate::translate_net_name (nn);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
      return std::string ();
    }
  }

  ParseElementData parse_element_helper (const std::string &s, const std::string &element)
  {
    ParseElementData data;
    db::NetlistSpiceReaderDelegate::parse_element (s, element, data.model_name_nc (), data.value_nc (), data.net_names_nc (), data.parameters_nc (), variables ());
    return data;
  }

  virtual void parse_element (const std::string &s, const std::string &element, std::string &model, double &value, std::vector<std::string> &nn, db::NetlistSpiceReader::parameters_type &pv, const db::NetlistSpiceReader::parameters_type &variables)
  {
    try {

      m_error.clear ();
      mp_variables = &variables;

      ParseElementData data;
      if (cb_parse_element.can_issue ()) {
        data = cb_parse_element.issue<NetlistSpiceReaderDelegateImpl, ParseElementData, const std::string &, const std::string &> (&NetlistSpiceReaderDelegateImpl::parse_element_helper, s, element);
      } else {
        data = parse_element_helper (s, element);
      }

      model = data.model_name ();
      value = data.value ();
      nn = data.net_names ();
      pv = data.parameters ();

      mp_variables = 0;

    } catch (tl::Exception &) {
      mp_variables = 0;
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
    } catch (...) {
      mp_variables = 0;
      throw;
    }
  }

  virtual bool element (db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const db::NetlistSpiceReader::parameters_type &params)
  {
    try {
      m_error.clear ();
      if (cb_element.can_issue ()) {
        return cb_element.issue<db::NetlistSpiceReaderDelegate, bool, db::Circuit *, const std::string &, const std::string &, const std::string &, double, const std::vector<db::Net *> &, const db::NetlistSpiceReader::parameters_type &> (&db::NetlistSpiceReaderDelegate::element, circuit, element, name, model, value, nets, params);
      } else {
        return db::NetlistSpiceReaderDelegate::element (circuit, element, name, model, value, nets, params);
      }
    } catch (tl::Exception &) {
      if (! m_error.empty ()) {
        db::NetlistSpiceReaderDelegate::error (m_error);
      } else {
        throw;
      }
      return false;
    }
  }

  const db::NetlistSpiceReader::parameters_type &variables () const
  {
    static db::NetlistSpiceReader::parameters_type empty;
    return mp_variables ? *mp_variables : empty;
  }

  gsi::Callback cb_start;
  gsi::Callback cb_finish;
  gsi::Callback cb_control_statement;
  gsi::Callback cb_wants_subcircuit;
  gsi::Callback cb_translate_net_name;
  gsi::Callback cb_element;
  gsi::Callback cb_parse_element;

private:
  std::string m_error;
  const db::NetlistSpiceReader::parameters_type *mp_variables;
};

static void start_fb (NetlistSpiceReaderDelegateImpl *delegate, db::Netlist *netlist)
{
  delegate->db::NetlistSpiceReaderDelegate::start (netlist);
}

static void finish_fb (NetlistSpiceReaderDelegateImpl *delegate, db::Netlist *netlist)
{
  delegate->db::NetlistSpiceReaderDelegate::finish (netlist);
}

static bool wants_subcircuit_fb (NetlistSpiceReaderDelegateImpl *delegate, const std::string &model)
{
  return delegate->db::NetlistSpiceReaderDelegate::wants_subcircuit (model);
}

static bool control_statement_fb (NetlistSpiceReaderDelegateImpl *delegate, const std::string &line)
{
  return delegate->db::NetlistSpiceReaderDelegate::control_statement (line);
}

static std::string translate_net_name_fb (NetlistSpiceReaderDelegateImpl *delegate, const std::string &name)
{
  return delegate->db::NetlistSpiceReaderDelegate::translate_net_name (name);
}

static bool element_fb (NetlistSpiceReaderDelegateImpl *delegate, db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const db::NetlistSpiceReader::parameters_type &params)
{
  return delegate->db::NetlistSpiceReaderDelegate::element (circuit, element, name, model, value, nets, params);
}

static ParseElementData parse_element_fb (NetlistSpiceReaderDelegateImpl *delegate, const std::string &s, const std::string &element)
{
  return delegate->parse_element_helper (s, element);
}

static tl::Variant value_from_string (NetlistSpiceReaderDelegateImpl * /*delegate*/, const std::string &s, const db::NetlistSpiceReader::parameters_type &variables)
{
  tl::Variant res;
  double v = 0.0;
  if (db::NetlistSpiceReaderDelegate::try_read_value (s, v, variables)) {
    res = v;
  }
  return res;
}

static ParseElementComponentsData parse_element_components (NetlistSpiceReaderDelegateImpl *delegate, const std::string &s, const db::NetlistSpiceReader::parameters_type &variables)
{
  ParseElementComponentsData data;
  delegate->parse_element_components (s, data.strings_nc (), data.parameters_nc (), variables);
  return data;
}

Class<ParseElementComponentsData> db_ParseElementComponentsData ("db", "ParseElementComponentsData",
  gsi::method ("strings", &ParseElementComponentsData::strings,
    "@brief Gets the (unnamed) string parameters\n"
    "These parameters are typically net names or model name."
  ) +
  gsi::method ("strings=", &ParseElementComponentsData::set_strings, gsi::arg ("list"),
    "@brief Sets the (unnamed) string parameters\n"
  ) +
  gsi::method ("parameters", &ParseElementComponentsData::parameters,
    "@brief Gets the (named) parameters\n"
    "Named parameters are typically (but not neccessarily) numerical, like 'w=0.15u'."
  ) +
  gsi::method ("parameters=", &ParseElementComponentsData::set_parameters, gsi::arg ("dict"),
    "@brief Sets the (named) parameters\n"
  ),
  "@brief Supplies the return value for \\NetlistSpiceReaderDelegate#parse_element_components.\n"
  "This is a structure with two members: 'strings' for the string arguments and 'parameters' for the "
  "named arguments.\n"
  "\n"
  "This helper class has been introduced in version 0.27.1. Starting with version 0.28.6, named parameters can be string types too.\n"
);

Class<ParseElementData> db_ParseElementData ("db", "ParseElementData",
  gsi::method ("value", &ParseElementData::value,
    "@brief Gets the value\n"
  ) +
  gsi::method ("value=", &ParseElementData::set_value, gsi::arg ("v"),
    "@brief Sets the value\n"
  ) +
  gsi::method ("model_name", &ParseElementData::model_name,
    "@brief Gets the model name\n"
  ) +
  gsi::method ("model_name=", &ParseElementData::set_model_name, gsi::arg ("m"),
    "@brief Sets the model name\n"
  ) +
  gsi::method ("net_names", &ParseElementData::net_names,
    "@brief Gets the net names\n"
  ) +
  gsi::method ("net_names=", &ParseElementData::set_net_names, gsi::arg ("list"),
    "@brief Sets the net names\n"
  ) +
  gsi::method ("parameters", &ParseElementData::parameters,
    "@brief Gets the (named) parameters\n"
  ) +
  gsi::method ("parameters=", &ParseElementData::set_parameters, gsi::arg ("dict"),
    "@brief Sets the (named) parameters\n"
  ),
  "@brief Supplies the return value for \\NetlistSpiceReaderDelegate#parse_element.\n"
  "This is a structure with four members: 'model_name' for the model name, 'value' for the default numerical value, 'net_names' for the net names and 'parameters' for the "
  "named parameters.\n"
  "\n"
  "This helper class has been introduced in version 0.27.1. Starting with version 0.28.6, named parameters can be string types too.\n"
);

static double get_delegate_scale (const db::NetlistSpiceReaderDelegate *delegate)
{
  return delegate->options ().scale;
}

static void apply_parameter_scaling (const db::NetlistSpiceReaderDelegate *delegate, db::Device *device)
{
  delegate->apply_parameter_scaling (device);
}

Class<NetlistSpiceReaderDelegateImpl> db_NetlistSpiceReaderDelegate ("db", "NetlistSpiceReaderDelegate",
  gsi::method_ext ("start", &start_fb, "@hide") +
  gsi::method_ext ("finish", &finish_fb, "@hide") +
  gsi::method_ext ("wants_subcircuit", &wants_subcircuit_fb, "@hide") +
  gsi::method_ext ("element", &element_fb, "@hide") +
  gsi::method_ext ("parse_element", &parse_element_fb, "@hide") +
  gsi::method_ext ("control_statement", &control_statement_fb, "@hide") +
  gsi::method_ext ("translate_net_name", &translate_net_name_fb, "@hide") +
  gsi::callback ("start", &NetlistSpiceReaderDelegateImpl::start, &NetlistSpiceReaderDelegateImpl::cb_start, gsi::arg ("netlist"),
    "@brief This method is called when the reader starts reading a netlist\n"
  ) +
  gsi::callback ("finish", &NetlistSpiceReaderDelegateImpl::finish, &NetlistSpiceReaderDelegateImpl::cb_finish, gsi::arg ("netlist"),
    "@brief This method is called when the reader is done reading a netlist successfully\n"
  ) +
  gsi::callback ("wants_subcircuit", &NetlistSpiceReaderDelegateImpl::wants_subcircuit, &NetlistSpiceReaderDelegateImpl::cb_wants_subcircuit, gsi::arg ("circuit_name"),
    "@brief Returns true, if the delegate wants subcircuit elements with this name\n"
    "The name is always upper case.\n"
  ) +
  gsi::callback ("control_statement", &NetlistSpiceReaderDelegateImpl::control_statement, &NetlistSpiceReaderDelegateImpl::cb_control_statement, gsi::arg ("line"),
    "@brief Receives control statements not understood by the standard reader\n"
    "When the reader encounters a control statement not understood by the parser, it will pass the line to the delegate using this method.\n"
    "The delegate can decide if it wants to read this statement. It should return true in this case.\n"
    "\n"
    "This method has been introduced in version 0.27.1\n"
  ) +
  gsi::callback ("translate_net_name", &NetlistSpiceReaderDelegateImpl::translate_net_name, &NetlistSpiceReaderDelegateImpl::cb_translate_net_name, gsi::arg ("net_name"),
    "@brief Translates a net name from the raw net name to the true net name\n"
    "The default implementation will replace backslash sequences by the corresponding character.\n"
    "'translate_net_name' is called before a net name is turned into a net object.\n"
    "The method can be reimplemented to supply a different translation scheme for net names. For example, to translate special characters.\n"
    "\n"
    "This method has been introduced in version 0.27.1\n"
  ) +
  gsi::method ("variables", &NetlistSpiceReaderDelegateImpl::variables,
    "@brief Gets the variables defined inside the SPICE file during execution of 'parse_element'\n"
    "In order to evaluate formulas, this method allows accessing the variables that are "
    "present during the execution of the SPICE reader.\n"
    "\n"
    "This method has been introduced in version 0.28.6."
  ) +
  gsi::callback ("parse_element", &NetlistSpiceReaderDelegateImpl::parse_element_helper, &NetlistSpiceReaderDelegateImpl::cb_parse_element,
    gsi::arg ("s"), gsi::arg ("element"),
    "@brief Parses an element card\n"
    "@param s The specification part of the element line (the part after element code and name).\n"
    "@param element The upper-case element code (\"M\", \"R\", ...).\n"
    "@return A \\ParseElementData object with the parts of the element.\n"
    "\n"
    "This method receives a string with the element specification and the element code. It is supposed to "
    "parse the element line and return a model name, a value, a list of net names and a parameter value dictionary.\n"
    "\n"
    "'parse_element' is called on every element card. The results of this call go into the \\element method "
    "to actually create the device. This method can be reimplemented to support other flavors of SPICE.\n"
    "\n"
    "This method has been introduced in version 0.27.1\n"
  ) +
  gsi::callback ("element", &NetlistSpiceReaderDelegateImpl::element, &NetlistSpiceReaderDelegateImpl::cb_element,
    gsi::arg ("circuit"), gsi::arg ("element"), gsi::arg ("name"), gsi::arg ("model"), gsi::arg ("value"), gsi::arg ("nets"), gsi::arg ("parameters"),
    "@brief Makes a device from an element line\n"
    "@param circuit The circuit that is currently read.\n"
    "@param element The upper-case element code (\"M\", \"R\", ...).\n"
    "@param name The element's name.\n"
    "@param model The upper-case model name (may be empty).\n"
    "@param value The default value (e.g. resistance for resistors) and may be zero.\n"
    "@param nets The nets given in the element line.\n"
    "@param parameters The parameters of the element statement (parameter names are upper case).\n"
    "\n"
    "The default implementation will create corresponding devices for\n"
    "some known elements using the Spice writer's parameter conventions.\n"
    "\n"
    "The method must return true, if the element was was understood and false otherwise.\n"
    "\n"
    "Starting with version 0.28.6, the parameter values can be strings too."
  ) +
  gsi::method ("error", &NetlistSpiceReaderDelegateImpl::error, gsi::arg ("msg"),
    "@brief Issues an error with the given message.\n"
    "Use this method to generate an error."
  ) +
  gsi::method_ext ("get_scale", &get_delegate_scale,
    "@brief Gets the scale factor set with '.options scale=...'\n"
    "This method has been introduced in version 0.28.6."
  ) +
  gsi::method_ext ("apply_parameter_scaling", &apply_parameter_scaling, gsi::arg ("device"),
    "@brief Applies parameter scaling to the given device\n"
    "Applies SI scaling (according to the parameter's si_scaling attribute) and "
    "geometry scaling (according to the parameter's geo_scale_exponent attribute) to "
    "the device parameters. Use this method of finish the device when you have created "
    "a custom device yourself.\n"
    "\n"
    "The geometry scale is taken from the '.options scale=...' control statement.\n"
    "\n"
    "This method has been introduced in version 0.28.6."
  ) +
  gsi::method_ext ("value_from_string", &value_from_string, gsi::arg ("s"), gsi::arg ("variables", db::NetlistSpiceReader::parameters_type (), "{}"),
    "@brief Translates a string into a value\n"
    "This function simplifies the implementation of SPICE readers by providing a translation of a unit-annotated string "
    "into double values. For example, '1k' is translated to 1000.0. In addition, simple formula evaluation is supported, e.g "
    "'(1+3)*2' is translated into 8.0.\n"
    "\n"
    "The variables dictionary defines named variables with the given values.\n"
    "\n"
    "This method has been introduced in version 0.27.1. The variables argument has been added in version 0.28.6.\n"
  ) +
  gsi::method_ext ("parse_element_components", &parse_element_components, gsi::arg ("s"), gsi::arg ("variables", db::NetlistSpiceReader::parameters_type (), "{}"),
    "@brief Parses a string into string and parameter components.\n"
    "This method is provided to simplify the implementation of 'parse_element'. It takes a string and splits it into "
    "string arguments and parameter values. For example, 'a b c=6' renders two string arguments in 'nn' and one parameter ('C'->6.0). "
    "It returns data \\ParseElementComponentsData object with the strings and parameters.\n"
    "The parameter names are already translated to upper case.\n"
    "\n"
    "The variables dictionary defines named variables with the given values.\n"
    "\n"
    "This method has been introduced in version 0.27.1. The variables argument has been added in version 0.28.6.\n"
  ),
  "@brief Provides a delegate for the SPICE reader for translating device statements\n"
  "Supply a customized class to provide a specialized reading scheme for devices. "
  "You need a customized class if you want to implement device reading from model subcircuits or to "
  "translate device parameters.\n"
  "\n"
  "See \\NetlistSpiceReader for more details.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

namespace {

class NetlistSpiceReaderWithOwnership
  : public db::NetlistSpiceReader
{
public:
  NetlistSpiceReaderWithOwnership (NetlistSpiceReaderDelegateImpl *delegate)
    : db::NetlistSpiceReader (delegate), m_ownership (delegate)
  {
    if (delegate) {
      delegate->keep ();
    }
  }

private:
  tl::shared_ptr<NetlistSpiceReaderDelegateImpl> m_ownership;
};

}

db::NetlistSpiceReader *new_spice_reader ()
{
  return new db::NetlistSpiceReader ();
}

db::NetlistSpiceReader *new_spice_reader2 (NetlistSpiceReaderDelegateImpl *delegate)
{
  return new NetlistSpiceReaderWithOwnership (delegate);
}

Class<db::NetlistSpiceReader> db_NetlistSpiceReader (db_NetlistReader, "db", "NetlistSpiceReader",
  gsi::constructor ("new", &new_spice_reader,
    "@brief Creates a new reader.\n"
  ) +
  gsi::constructor ("new", &new_spice_reader2, gsi::arg ("delegate"),
    "@brief Creates a new reader with a delegate.\n"
  ),
  "@brief Implements a netlist Reader for the SPICE format.\n"
  "Use the SPICE reader like this:\n"
  "\n"
  "@code\n"
  "reader = RBA::NetlistSpiceReader::new\n"
  "netlist = RBA::Netlist::new\n"
  "netlist.read(path, reader)\n"
  "@/code\n"
  "\n"
  "The translation of SPICE elements can be tailored by providing a \\NetlistSpiceReaderDelegate class. "
  "This allows translating of device parameters and mapping of some subcircuits to devices.\n"
  "\n"
  "The following example is a delegate that turns subcircuits called HVNMOS and HVPMOS into "
  "MOS4 devices with the parameters scaled by 1.5:\n"
  "\n"
  "@code\n"
  "class MyDelegate < RBA::NetlistSpiceReaderDelegate\n"
  "\n"
  "  # says we want to catch these subcircuits as devices\n"
  "  def wants_subcircuit(name)\n"
  "    name == \"HVNMOS\" || name == \"HVPMOS\"\n"
  "  end\n"
  "\n"
  "  # translate the element\n"
  "  def element(circuit, el, name, model, value, nets, params)\n"
  "\n"
  "    if el != \"X\"\n"
  "      # all other elements are left to the standard implementation\n"
  "      return super\n"
  "    end\n"
  "\n"
  "    if nets.size != 4\n"
  "      error(\"Subcircuit #{model} needs four nodes\")\n"
  "    end\n"
  "\n"
  "    # provide a device class\n"
  "    cls = circuit.netlist.device_class_by_name(model)\n"
  "    if ! cls\n"
  "      cls = RBA::DeviceClassMOS4Transistor::new\n"
  "      cls.name = model\n"
  "      circuit.netlist.add(cls)\n"
  "    end\n"
  "\n"
  "    # create a device\n"
  "    device = circuit.create_device(cls, name)\n"
  "\n"
  "    # and configure the device\n"
  "    [ \"S\", \"G\", \"D\", \"B\" ].each_with_index do |t,index|\n"
  "      device.connect_terminal(t, nets[index])\n"
  "    end\n"
  "    params.each do |p,value|\n"
  "      device.set_parameter(p, value * 1.5)\n"
  "    end\n"
  "\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "# usage:\n"
  "\n"
  "mydelegate = MyDelegate::new\n"
  "reader = RBA::NetlistSpiceReader::new(mydelegate)\n"
  "\n"
  "nl = RBA::Netlist::new\n"
  "nl.read(input_file, reader)\n"
  "@/code\n"
  "\n"
  "A somewhat contrived example for using the delegate to translate net names is this:\n"
  "\n"
  "@code\n"
  "class MyDelegate < RBA::NetlistSpiceReaderDelegate\n"
  "\n"
  "  # translates 'VDD' to 'VXX' and leave all other net names as is:\n"
  "  alias translate_net_name_org translate_net_name\n"
  "  def translate_net_name(n)\n"
  "    return n == \"VDD\" ? \"VXX\" : translate_net_name_org(n)}\n"
  "  end\n"
  "\n"
  "end\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.26. It has been extended in version 0.27.1."
);

}

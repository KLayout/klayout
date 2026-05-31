
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

static const db::NetTerminalRef *terminal_ref_by_name_const (const db::Device *device, const std::string &name)
{
  if (! device->device_class () || ! device->device_class ()->has_terminal_with_name (name)) {
    return 0;
  } else {
    return device->terminal_ref_for_terminal (device->device_class ()->terminal_id_for_name (name));
  }
}

static db::NetTerminalRef *terminal_ref_by_name (db::Device *device, const std::string &name)
{
  if (! device->device_class () || ! device->device_class ()->has_terminal_with_name (name)) {
    return 0;
  } else {
    return device->terminal_ref_for_terminal (device->device_class ()->terminal_id_for_name (name));
  }
}

extern Class<db::NetlistObject> decl_dbNetlistObject;

Class<db::Device> decl_dbDevice (decl_dbNetlistObject, "db", "Device",
  gsi::method ("device_class", &db::Device::device_class,
    "@brief Gets the device class the device belongs to.\n"
  ) +
  gsi::method ("device_abstract", &db::Device::device_abstract,
    "@brief Gets the device abstract for this device instance.\n"
    "See \\DeviceAbstract for more details.\n"
  ) +
  gsi::method ("device_abstract=", &db::Device::set_device_abstract, gsi::arg ("device_abstract"),
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
  gsi::method_ext ("net_for_terminal", &net_for_terminal_by_name_const, gsi::arg ("terminal_name"),
    "@brief Gets the net connected to the specified terminal.\n"
    "If the terminal is not connected, nil is returned for the net."
    "\n\n"
    "This convenience method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method_ext ("net_for_terminal", &net_for_terminal_by_name, gsi::arg ("terminal_name"),
    "@brief Gets the net connected to the specified terminal (non-const version).\n"
    "If the terminal is not connected, nil is returned for the net."
    "\n\n"
    "This convenience method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method ("terminal_ref", (const db::NetTerminalRef *(db::Device::*) (size_t) const) &db::Device::terminal_ref_for_terminal, gsi::arg ("terminal_id"),
    "@brief Gets the terminal refeference for a specific terminal.\n"
    "The terminal ref is the connector between a net and a device terminal. "
    "It knows the net the terminal is connected to and is useful to obtain the shapes making the terminal of the device. "
    "If the terminal is not connected, nil is returned for the net.\n"
    "\n"
    "This method has been introduced in version 0.30."
  ) +
  gsi::method ("terminal_ref", (db::NetTerminalRef *(db::Device::*) (size_t)) &db::Device::terminal_ref_for_terminal, gsi::arg ("terminal_id"),
    "@brief Gets the terminal refeference for a specific terminal (non-const version).\n"
    "The terminal ref is the connector between a net and a device terminal. "
    "It knows the net the terminal is connected to and is useful to obtain the shapes making the terminal of the device. "
    "If the terminal is not connected, nil is returned for the net.\n"
    "\n"
    "This method has been introduced in version 0.30."
  ) +
  gsi::method_ext ("terminal_ref", &terminal_ref_by_name_const, gsi::arg ("terminal_name"),
    "@brief Gets the terminal refeference for a specific terminal where the terminal is given by name.\n"
    "The terminal ref is the connector between a net and a device terminal. "
    "It knows the net the terminal is connected to and is useful to obtain the shapes making the terminal of the device. "
    "If the terminal is not connected, nil is returned for the net.\n"
    "\n"
    "This method has been introduced in version 0.30."
  ) +
  gsi::method_ext ("terminal_ref", &terminal_ref_by_name, gsi::arg ("terminal_name"),
    "@brief Gets the terminal refeference for a specific terminal where the terminal is given by name (non-const version).\n"
    "The terminal ref is the connector between a net and a device terminal. "
    "It knows the net the terminal is connected to and is useful to obtain the shapes making the terminal of the device. "
    "If the terminal is not connected, nil is returned for the net.\n"
    "\n"
    "This method has been introduced in version 0.30."
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
  gsi::method ("parameter", (const tl::Variant &(db::Device::*) (size_t) const) &db::Device::parameter_value, gsi::arg ("param_id"),
    "@brief Gets the parameter value for the given parameter ID."
  ) +
  gsi::method ("set_parameter", (void (db::Device::*) (size_t, const tl::Variant &)) &db::Device::set_parameter_value, gsi::arg ("param_id"), gsi::arg ("value"),
    "@brief Sets the parameter value for the given parameter ID."
  ) +
  gsi::method ("parameter", (const tl::Variant &(db::Device::*) (const std::string &) const) &db::Device::parameter_value, gsi::arg ("param_name"),
    "@brief Gets the parameter value for the given parameter name.\n"
    "If the parameter name is not valid, an exception is thrown."
  ) +
  gsi::method ("set_parameter", (void (db::Device::*) (const std::string &, const tl::Variant &)) &db::Device::set_parameter_value, gsi::arg ("param_name"), gsi::arg ("value"),
    "@brief Sets the parameter value for the given parameter name.\n"
    "If the parameter name is not valid, an exception is thrown."
  ) +
  gsi::method ("set_parameter_create", &db::Device::set_parameter_value_create, gsi::arg ("param_name"), gsi::arg ("value"), gsi::arg ("primary", true), gsi::arg ("default_value", tl::Variant (), "auto"),
    "@brief Sets the parameter value for the given parameter name and creates a new parameter if no parameter is present with this name.\n"
    "If a parameter exists already with the given name, the value of that parameter is set to the given value and the other arguments are ignored. "
    "If no parameter with the given name exists in the device class's parameter table, a new parameter is created. "
    "In this case, \\primary indicates that this parameter will be a primary one and \\default_value is the default value for this parameter. "
    "The default value also implicitly defines the type of the parameter. A float value will create a float-type parameter, a string value for default will create a string-type parameter, "
    "hence it should 'fit' to the parameter value. If no default value is given, an automatic default is picked depending on the parameter type."
    "\n"
    "This method has been introduced in version 0.31.0."
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

}


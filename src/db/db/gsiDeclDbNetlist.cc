
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
#include "dbNetlist.h"
#include "tlException.h"
#include "tlInternational.h"

namespace gsi
{

Class<db::Pin> decl_dbPin ("db", "Pin",
  gsi::method ("id", &db::Pin::id,
    "@brief Gets the ID of the pin.\n"
  ) +
  gsi::method ("name", &db::Pin::name,
    "@brief Gets the name of the pin.\n"
  ),
  "@brief A pin of a circuit.\n"
  "Pin objects are used to describe the outgoing pins of "
  "a circuit. To create a new pin of a circuit, use \\Circuit#create_pin.\n"
  "\n"
  "This class has been added in version 0.26."
);

static void device_disconnect_port (db::Device *device, size_t port_id)
{
  device->connect_port (port_id, 0);
}

static bool device_has_param_with_name (const db::DeviceClass *device_class, const std::string &name)
{
  const std::vector<db::DeviceParameterDefinition> &pd = device_class->parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    if (i->name () == name) {
      return true;
    }
  }
  return false;
}

static size_t device_param_id_for_name (const db::DeviceClass *device_class, const std::string &name)
{
  if (device_class) {
    const std::vector<db::DeviceParameterDefinition> &pd = device_class->parameter_definitions ();
    for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
      if (i->name () == name) {
        return i->id ();
      }
    }
  }
  throw tl::Exception (tl::to_string (tr ("Invalid parameter name")) + ": '" + name + "'");
}

static double device_parameter_value (const db::Device *device, const std::string &name)
{
  return device->parameter_value (device_param_id_for_name (device->device_class (), name));
}

static void device_set_parameter_value (db::Device *device, const std::string &name, double value)
{
  return device->set_parameter_value (device_param_id_for_name (device->device_class (), name), value);
}

Class<db::Device> decl_dbDevice ("db", "Device",
  gsi::method ("device_class", &db::Device::device_class,
    "@brief Gets the device class the device belongs to.\n"
  ) +
  gsi::method ("name=", &db::Device::set_name, gsi::arg ("name"),
    "@brief Sets the name of the device.\n"
    "Device names are used to name a device inside a netlist file. "
    "Device names should be unique within a circuit."
  ) +
  gsi::method ("name", &db::Device::name,
    "@brief Gets the name of the device.\n"
  ) +
  gsi::method ("net_for_port", (db::Net *(db::Device::*) (size_t)) &db::Device::net_for_port, gsi::arg ("port_id"),
    "@brief Gets the net connected to the specified port.\n"
    "If the port is not connected, nil is returned for the net."
  ) +
  gsi::method ("connect_port", &db::Device::connect_port, gsi::arg ("port_id"), gsi::arg ("net"),
    "@brief Connects the given port to the specified net.\n"
  ) +
  gsi::method_ext ("disconnect_port", &device_disconnect_port, gsi::arg ("port_id"),
    "@brief Disconnects the given port from any net.\n"
  ) +
  gsi::method ("parameter", &db::Device::parameter_value, gsi::arg ("param_id"),
    "@brief Gets the parameter value for the given parameter ID."
  ) +
  gsi::method ("set_parameter", &db::Device::set_parameter_value, gsi::arg ("param_id"), gsi::arg ("value"),
    "@brief Sets the parameter value for the given parameter ID."
  ) +
  gsi::method_ext ("parameter", &gsi::device_parameter_value, gsi::arg ("param_name"),
    "@brief Gets the parameter value for the given parameter name.\n"
    "If the parameter name is not valid, an exception is thrown."
  ) +
  gsi::method_ext ("set_parameter", &gsi::device_set_parameter_value, gsi::arg ("param_name"), gsi::arg ("value"),
    "@brief Sets the parameter value for the given parameter name.\n"
    "If the parameter name is not valid, an exception is thrown."
  ),
  "@brief A device inside a circuit.\n"
  "Device object represent atomic devices such as resistors, diodes or transistors. "
  "The \\Device class represents a particular device with specific parameters. "
  "The type of device is represented by a \\DeviceClass object. Device objects "
  "live in \\Circuit objects, the device class objects live in the \\Netlist object.\n"
  "\n"
  "Devices connect to nets through ports. Ports are described by a port ID which is "
  "essentially the zero-based index of the port. Port definitions can be "
  "obtained from the device class using the \\DeviceClass#port_definitions method.\n"
  "\n"
  "Devices connect to nets through the \\Device#connect_port method. "
  "Device ports can be disconnected using \\Device#disconnect_port.\n"
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

Class<db::SubCircuit> decl_dbSubCircuit ("db", "SubCircuit",
  gsi::method ("circuit", (db::Circuit *(db::SubCircuit::*) ()) &db::SubCircuit::circuit,
    "@brief Gets the circuit referenced by the subcircuit.\n"
  ) +
  gsi::method ("name=", &db::SubCircuit::set_name, gsi::arg ("name"),
    "@brief Sets the name of the subcircuit.\n"
    "SubCircuit names are used to name a subcircuits inside a netlist file. "
    "SubCircuit names should be unique within a circuit."
  ) +
  gsi::method ("name", &db::SubCircuit::name,
    "@brief Gets the name of the subcircuit.\n"
  ) +
  gsi::method ("net_for_pin", (db::Net *(db::SubCircuit::*) (size_t)) &db::SubCircuit::net_for_pin, gsi::arg ("pin_id"),
    "@brief Gets the net connected to the specified pin of the subcircuit.\n"
    "If the pin is not connected, nil is returned for the net."
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
  "This class has been added in version 0.26."
);

Class<db::NetPortRef> decl_dbNetPortRef ("db", "NetPortRef",
  gsi::method ("port_id", &db::NetPortRef::port_id,
    "@brief Gets the ID of the port of the device the connection is made to."
  ) +
  gsi::method ("device", (db::Device *(db::NetPortRef::*) ()) &db::NetPortRef::device,
    "@brief Gets the device reference.\n"
    "Gets the device object that this connection is made to."
  ) +
  gsi::method ("net", (db::Net *(db::NetPortRef::*) ()) &db::NetPortRef::net,
    "@brief Gets the net this port reference is attached to"
  ) +
  gsi::method ("device_class", (db::DeviceClass *(db::NetPortRef::*) ()) &db::NetPortRef::device_class,
    "@brief Gets the class of the device which is addressed."
  ) +
  gsi::method ("port_def", (db::DevicePortDefinition *(db::NetPortRef::*) ()) &db::NetPortRef::port_def,
    "@brief Gets the port definition of the port that is connected"
  ),
  "@brief A connection to a port of a device.\n"
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
  gsi::method ("subcircuit", (db::SubCircuit *(db::NetPinRef::*) ()) &db::NetPinRef::subcircuit,
    "@brief Gets the subcircuit reference.\n"
    "If the pin is a pin of a subcircuit, this attribute "
    "indicates the subcircuit the net attaches to. The "
    "subcircuit lives in the same circuit than the net. "
    "If the pin is a outgoing pin of the circuit, this "
    "attribute is nil."
  ) +
  gsi::method ("net", (db::Net *(db::NetPinRef::*) ()) &db::NetPinRef::net,
    "@brief Gets the net this pin reference is attached to"
  ),
  "@brief A connection to a pin of a subcircuit or an outgoing pin of the circuit.\n"
  "This object is used inside a net (see \\Net) to describe the connections a net makes.\n"
  "\n"
  "This class has been added in version 0.26."
);

Class<db::Net> decl_dbNet ("db", "Net",
  gsi::method ("circuit", (db::Circuit *(db::Net::*) ()) &db::Net::circuit,
    "@brief Gets the circuit the net lives in."
  ) +
  gsi::method ("clear", &db::Net::clear,
    "@brief Clears the net."
  ) +
  gsi::method ("name=", &db::Net::set_name, gsi::arg ("name"),
    "@brief Sets the name of the net.\n"
    "The name of the net is used for nameing the net in schematic files for example. "
    "The name of the net has to be unique."
  ) +
  gsi::method ("name", &db::Net::name,
    "@brief Gets the name of the net.\n"
    "See \\name= for details about the name."
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
  gsi::iterator ("each_pin", (db::Net::pin_iterator (db::Net::*) ()) &db::Net::begin_pins, (db::Net::pin_iterator (db::Net::*) ()) &db::Net::end_pins,
    "@brief Iterates over all pins the net connects.\n"
    "Pin connections are described by \\NetPinRef objects. Pin connections "
    "are either connections to subcircuit pins or to outgoing pins of the "
    "circuit the net lives in."
  ) +
  gsi::iterator ("each_port", (db::Net::port_iterator (db::Net::*) ()) &db::Net::begin_ports, (db::Net::port_iterator (db::Net::*) ()) &db::Net::end_ports,
    "@brief Iterates over all ports the net connects.\n"
    "Ports connect devices. Port connections are described by \\NetPortRef "
    "objects."
  ),
  "@brief A single net.\n"
  "A net connects multiple pins or ports together. Pins are either "
  "pin or subcircuits of outgoing pins of the circuit the net lives in. "
  "Ports are connections made to specific ports of devices.\n"
  "\n"
  "To connect a net to an outgoing pin of a circuit, use \\Circuit#connect_pin, to "
  "disconnect a net from an outgoing pin use \\Circuit#disconnect_pin. "
  "To connect a net to a pin of a subcircuit, use \\SubCircuit#connect_pin, to "
  "disconnect a net from a pin of a subcircuit, use \\SubCircuit#disconnect_pin. "
  "To connect a net to a port of a device, use \\Device#connect_port, to "
  "disconnect a net from a port of a device, use \\Device#disconnect_port.\n"
  "\n"
  "This class has been added in version 0.26."
);

static db::DevicePortDefinition *new_port_definition (const std::string &name, const std::string &description)
{
  return new db::DevicePortDefinition (name, description);
}

Class<db::DevicePortDefinition> decl_dbDevicePortDefinition ("db", "DevicePortDefinition",
  gsi::constructor ("new", &gsi::new_port_definition, gsi::arg ("name"), gsi::arg ("description", std::string ()),
    "@brief Creates a new port definition."
  ) +
  gsi::method ("name", &db::DevicePortDefinition::name,
    "@brief Gets the name of the port."
  ) +
  gsi::method ("name=", &db::DevicePortDefinition::set_name, gsi::arg ("name"),
    "@brief Sets the name of the port."
  ) +
  gsi::method ("description", &db::DevicePortDefinition::description,
    "@brief Gets the description of the port."
  ) +
  gsi::method ("description=", &db::DevicePortDefinition::set_description, gsi::arg ("description"),
    "@brief Sets the description of the port."
  ) +
  gsi::method ("id", &db::DevicePortDefinition::id,
    "@brief Gets the ID of the port.\n"
    "The ID of the port is used in some places to refer to a specific port (e.g. in "
    "the \\NetPortRef object)."
  ),
  "@brief A port descriptor\n"
  "This class is used inside the \\DeviceClass class to describe a port of the device.\n"
  "\n"
  "This class has been added in version 0.26."
);

static db::DeviceParameterDefinition *new_parameter_definition (const std::string &name, const std::string &description, double default_value)
{
  return new db::DeviceParameterDefinition (name, description, default_value);
}

Class<db::DeviceParameterDefinition> decl_dbDeviceParameterDefinition ("db", "DeviceParameterDefinition",
  gsi::constructor ("new", &gsi::new_parameter_definition, gsi::arg ("name"), gsi::arg ("description", std::string ()), gsi::arg ("default_value", 0.0),
    "@brief Creates a new parameter definition."
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

static tl::id_type id_of_device_class (const db::DeviceClass *cls)
{
  return tl::id_of (cls);
}

Class<db::DeviceClass> decl_dbDeviceClass ("db", "DeviceClass",
  gsi::method ("name", &db::DeviceClass::name,
    "@brief Gets the name of the device class."
  ) +
  gsi::method ("description", &db::DeviceClass::description,
    "@brief Gets the description text of the device class."
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
  gsi::method ("port_definitions", &db::DeviceClass::port_definitions,
    "@brief Gets the list of port definitions of the device.\n"
    "See the \\DevicePortDefinition class description for details."
  ) +
  gsi::method ("port_definition", &db::DeviceClass::port_definition, gsi::arg ("port_id"),
    "@brief Gets the port definition object for a given ID.\n"
    "Port definition IDs are used in some places to reference a specific port of a device. "
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
  gsi::method_ext ("has_parameter", &gsi::device_has_param_with_name, gsi::arg ("name"),
    "@brief Returns true, if the device class has a parameter with the given name.\n"
  ) +
  gsi::method_ext ("parameter_id", &gsi::device_param_id_for_name, gsi::arg ("name"),
    "@brief Returns the parameter ID of the parameter with the given name.\n"
    "An exception is thrown if there is no parameter with the given name. Use \\has_parameter to check "
    "whether the name is a valid parameter name."
  ),
  "@brief A class describing a specific type of device.\n"
  "Device class objects live in the context of a \\Netlist object. After a "
  "device class is created, it must be added to the netlist using \\Netlist#add. "
  "The netlist will own the device class object. When the netlist is destroyed, the "
  "device class object will become invalid.\n"
  "\n"
  "The \\DeviceClass class is the base class for other device classes.\n"
  "\n"
  "This class has been added in version 0.26."
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
    : db::DeviceClass ()
  {
    //  .. nothing yet ..
  }

  virtual bool combine_devices (db::Device *a, db::Device *b) const
  {
    if (cb_combine_devices.can_issue ()) {
      return cb_combine_devices.issue<const db::DeviceClass, bool, db::Device *, db::Device *> (&db::DeviceClass::combine_devices, a, b);
    } else {
      return db::DeviceClass::combine_devices (a, b);
    }
  }

private:
  gsi::Callback cb_combine_devices;
};

}

static void gdc_add_port_definition (GenericDeviceClass *cls, db::DevicePortDefinition *port_def)
{
  if (port_def) {
    *port_def = cls->add_port_definition (*port_def);
  }
}

static void gdc_add_parameter_definition (GenericDeviceClass *cls, db::DeviceParameterDefinition *parameter_def)
{
  if (parameter_def) {
    *parameter_def = cls->add_parameter_definition (*parameter_def);
  }
}

Class<GenericDeviceClass> decl_GenericDeviceClass (decl_dbDeviceClass, "db", "GenericDeviceClass",
  gsi::method_ext ("add_port", &gsi::gdc_add_port_definition, gsi::arg ("port_def"),
    "@brief Adds the given port definition to the device class\n"
    "This method will define a new port. The new port is added at the end of existing ports. "
    "The port definition object passed as the argument is modified to contain the "
    "new ID of the port.\n"
    "\n"
    "The port is copied into the device class. Modifying the port object later "
    "does not have the effect of changing the port definition."
  ) +
  gsi::method ("clear_ports", &GenericDeviceClass::clear_port_definitions,
    "@brief Clears the list of ports\n"
  ) +
  gsi::method_ext ("add_parameter", &gsi::gdc_add_parameter_definition, gsi::arg ("parameter_def"),
    "@brief Adds the given parameter definition to the device class\n"
    "This method will define a new parameter. The new parameter is added at the end of existing parameters. "
    "The parameter definition object passed as the argument is modified to contain the "
    "new ID of the parameter."
    "\n"
    "The parameter is copied into the device class. Modifying the parameter object later "
    "does not have the effect of changing the parameter definition."
  ) +
  gsi::method ("clear_parameters", &GenericDeviceClass::clear_parameter_definitions,
    "@brief Clears the list of parameters\n"
  ) +
  gsi::method ("name=", &GenericDeviceClass::set_name, gsi::arg ("name"),
    "@brief Sets the name of the device\n"
  ) +
  gsi::method ("description=", &GenericDeviceClass::set_description, gsi::arg ("description"),
    "@brief Sets the description of the device\n"
  ),
  "@brief A generic device class\n"
  "This class allows building generic device classes. Specificially, ports can be defined "
  "by adding port definitions. Port definitions should not be added dynamically. To create "
  "your own device, instantiate the \\GenericDeviceClass object, set name and description and "
  "specify the ports. Then add this new device class to the \\Netlist object where it will live "
  "and be used to define device instances (\\Device objects).\n"
  "\n"
  "In addition, parameters can be defined which correspond to values stored inside the "
  "specific device instance (\\Device object)."
  "\n"
  "This class has been added in version 0.26."
);

static const db::Pin &create_pin (db::Circuit *c, const std::string &name)
{
  return c->add_pin (db::Pin (name));
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
  c->add_sub_circuit (sc);
  return sc;
}

static db::Net *circuit_net_for_pin (db::Circuit *c, const db::Pin *pin)
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

Class<db::Circuit> decl_dbCircuit ("db", "Circuit",
  gsi::method_ext ("create_pin", &gsi::create_pin, gsi::arg ("name"),
    "@brief Creates a new \\Pin object inside the circuit\n"
    "This object will describe a pin of the circuit. A circuit connects "
    "to the outside through such a pin. The pin is added after all existing "
    "pins. For more details see the \\Pin class."
  ) +
  gsi::iterator ("each_pin", (db::Circuit::pin_iterator (db::Circuit::*) ()) &db::Circuit::begin_pins, (db::Circuit::pin_iterator (db::Circuit::*) ()) &db::Circuit::end_pins,
    "@brief Iterates over the pins of the circuit"
  ) +
  gsi::method ("pin_by_id", &db::Circuit::pin_by_id, gsi::arg ("id"),
    "@brief Gets the \\Pin object corresponding to a specific ID\n"
    "If the ID is not a valid pin ID, nil is returned."
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
  gsi::iterator ("each_net", (db::Circuit::net_iterator (db::Circuit::*) ()) &db::Circuit::begin_nets, (db::Circuit::net_iterator (db::Circuit::*) ()) &db::Circuit::end_nets,
    "@brief Iterates over the nets of the circuit"
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
  gsi::method_ext ("create_subcircuit", &gsi::create_subcircuit1, gsi::arg ("circuit"), gsi::arg ("name", std::string ()),
    "@brief Creates a new bound \\SubCircuit object inside the circuit\n"
    "This object describes an instance of another circuit inside the circuit. The subcircuit is already attached "
    "to the other circuit. The name is optional and is used to identify the subcircuit in a "
    "netlist file.\n"
    "\n"
    "For more details see the \\SubCircuit class."
  ) +
  gsi::method ("remove_subcircuit", &db::Circuit::remove_sub_circuit, gsi::arg ("subcircuit"),
    "@brief Removes the given subcircuit from the circuit\n"
  ) +
  gsi::iterator ("each_subcircuit", (db::Circuit::sub_circuit_iterator (db::Circuit::*) ()) &db::Circuit::begin_sub_circuits, (db::Circuit::sub_circuit_iterator (db::Circuit::*) ()) &db::Circuit::end_sub_circuits,
    "@brief Iterates over the subcircuits of the circuit"
  ) +
  gsi::method ("netlist", (db::Netlist *(db::Circuit::*) ()) &db::Circuit::netlist,
    "@brief Gets the netlist object the circuit lives in"
  ) +
  gsi::method ("name=", &db::Circuit::set_name, gsi::arg ("name"),
    "@brief Sets the name of the circuit"
  ) +
  gsi::method ("name", &db::Circuit::name,
    "@brief Gets the name of the circuit"
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
  gsi::method_ext ("net_for_pin", &gsi::circuit_net_for_pin, gsi::arg ("pin"),
    "@brief Gets the net object attached to a specific pin.\n"
    "This is the net object inside the circuit which attaches to the given outward-bound pin.\n"
    "This method returns nil if the pin is not connected or the pin object is nil."
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
  ),
  "@brief Circuits are the basic building blocks of the netlist\n"
  "A circuit has pins by which it can connect to the outside. Pins are "
  "created using \\create_pin and are represented by the \\Pin class.\n"
  "\n"
  "Futhermore, a circuit manages the components of the netlist. "
  "Components are devices (class \\Device) and subcircuits (class \\SubCircuit). "
  "Devices are basic devices such as resistors or transistors. Subcircuits "
  "are other circuits to which nets from this circuit connect. "
  "Devices are created using the \\create_device method. Subcircuits are "
  "created using the \\create_subcircuit method.\n"
  "\n"
  "Devices are connected through 'ports', subcircuits are connected through "
  "their pins. Ports and pins are described by integer ID's in the context of "
  "most methods.\n"
  "\n"
  "Finally, the circuit consists of the nets. Nets connect ports of devices "
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
  c->keep ();
  nl->add_circuit (c);
}

static void add_device_class (db::Netlist *nl, db::DeviceClass *cl)
{
  cl->keep ();
  nl->add_device_class (cl);
}

Class<db::Netlist> decl_dbNetlist ("db", "Netlist",
  gsi::method_ext ("add", &gsi::add_circuit,
    "@brief Adds the circuit to the netlist\n"
    "This method will add the given circuit object to the netlist. "
    "After the circuit has been added, it will be owned by the netlist."
  ) +
  gsi::method ("remove", &db::Netlist::remove_circuit, gsi::arg ("circuit"),
    "@brief Removes the given circuit object from the netlist\n"
    "After the object has been removed, it becomes invalid and cannot be used further."
  ) +
  gsi::iterator ("each_circuit", (db::Netlist::circuit_iterator (db::Netlist::*) ()) &db::Netlist::begin_circuits, (db::Netlist::circuit_iterator (db::Netlist::*) ()) &db::Netlist::end_circuits,
    "@brief Iterates over the circuits of the netlist"
  ) +
  gsi::method_ext ("add", &gsi::add_device_class,
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
  gsi::iterator ("each_device_class", (db::Netlist::device_class_iterator (db::Netlist::*) ()) &db::Netlist::begin_device_classes, (db::Netlist::device_class_iterator (db::Netlist::*) ()) &db::Netlist::end_device_classes,
    "@brief Iterates over the device classes of the netlist"
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

}

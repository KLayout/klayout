
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
#include "tlGlobPattern.h"

namespace gsi
{

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
nets_by_name_const (const db::Circuit *circuit, const std::string &name_pattern, const tl::Variant &cs)
{
  std::vector<const db::Net *> res;
  if (! circuit) {
    return res;
  }

  tl::GlobPattern glob (name_pattern);
  if (! cs.is_nil ()) {
    glob.set_case_sensitive (cs.to_bool ());
  } else if (circuit->netlist ()) {
    glob.set_case_sensitive (circuit->netlist ()->is_case_sensitive ());
  }
  for (db::Circuit::const_net_iterator n = circuit->begin_nets (); n != circuit->end_nets (); ++n) {
    const db::Net *net = n.operator-> ();
    if (!net->name ().empty () && glob.match (net->name ())) {
      res.push_back (net);
    }
  }

  return res;
}

static std::vector<db::Net *>
nets_by_name (db::Circuit *circuit, const std::string &name_pattern, const tl::Variant &cs)
{
  return nets_non_const (nets_by_name_const (circuit, name_pattern, cs));
}

extern Class<db::NetlistObject> decl_dbNetlistObject;

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
  gsi::method_ext ("nets_by_name", &nets_by_name, gsi::arg ("name_pattern"), gsi::arg ("case_sensitive", tl::Variant (), "default"),
    "@brief Gets the net objects for a given name filter.\n"
    "The name filter is a glob pattern. This method will return all \\Net objects matching the glob pattern.\n"
    "The 'case_sensitive' argument will control whether the name is looked up in a case sensitive way or not. Note that "
    "with case insensitive search on a netlist that is case sensitive, the same name may render more than one hit. By "
    "default, case sensitivity is taken from the netlist.\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
    "The 'case_sensitive' argument has been added in version 0.30.2."
  ) +
  gsi::method_ext ("nets_by_name", &nets_by_name_const, gsi::arg ("name_pattern"), gsi::arg ("case_sensitive", tl::Variant (), "default"),
    "@brief Gets the net objects for a given name filter (const version).\n"
    "The name filter is a glob pattern. This method will return all \\Net objects matching the glob pattern.\n"
    "The 'case_sensitive' argument will control whether the name is looked up in a case sensitive way or not. Note that "
    "with case insensitive search on a netlist that is case sensitive, the same name may render more than one hit. By "
    "default, case sensitivity is taken from the netlist.\n"
    "\n"
    "This constness variant has been introduced in version 0.27.3.\n"
    "The 'case_sensitive' argument has been added in version 0.30.2."
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
  gsi::method ("purge_devices", &db::Circuit::purge_devices,
    "@brief Purges invalid devices.\n"
    "Purges devices which are considered invalid. Such devices are for example those whose terminals are all connected to a single net.\n"
    "\n"
    "This method has been added in version 0.29.7."
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

}


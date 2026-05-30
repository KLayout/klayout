
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
#include "dbNetlistWriter.h"
#include "dbNetlistReader.h"
#include "tlStream.h"
#include "tlGlobPattern.h"

namespace gsi
{

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
circuits_by_name (db::Netlist *netlist, const std::string &name_pattern, const tl::Variant &cs)
{
  std::vector<db::Circuit *> res;
  if (! netlist) {
    return res;
  }

  tl::GlobPattern glob (name_pattern);
  glob.set_case_sensitive (cs.is_nil () ? netlist->is_case_sensitive () : cs.to_bool ());

  for (db::Netlist::circuit_iterator c = netlist->begin_circuits (); c != netlist->end_circuits (); ++c) {
    db::Circuit *circuit = c.operator-> ();
    if (glob.match (circuit->name ())) {
      res.push_back (circuit);
    }
  }

  return res;
}

static std::vector<const db::Circuit *>
circuits_by_name_const (const db::Netlist *netlist, const std::string &name_pattern, const tl::Variant &cs)
{
  std::vector<const db::Circuit *> res;
  if (! netlist) {
    return res;
  }

  tl::GlobPattern glob (name_pattern);
  glob.set_case_sensitive (cs.is_nil () ? netlist->is_case_sensitive () : cs.to_bool ());

  for (db::Netlist::const_circuit_iterator c = netlist->begin_circuits (); c != netlist->end_circuits (); ++c) {
    const db::Circuit *circuit = c.operator-> ();
    if (glob.match (circuit->name ())) {
      res.push_back (circuit);
    }
  }

  return res;
}

static std::vector<const db::Net *>
nets_by_name_const_from_netlist (const db::Netlist *netlist, const std::string &name_pattern, const tl::Variant &cs)
{
  std::vector<const db::Net *> res;
  if (! netlist) {
    return res;
  }

  tl::GlobPattern glob (name_pattern);
  glob.set_case_sensitive (cs.is_nil () ? netlist->is_case_sensitive () : cs.to_bool ());
  for (auto c = netlist->begin_circuits (); c != netlist->end_circuits (); ++c) {
    bool is_top = (c->begin_parents () == c->end_parents ());
    for (auto n = c->begin_nets (); n != c->end_nets (); ++n) {
      const db::Net *net = n.operator-> ();
      //  NOTE: we only pick root nets (pin_count == 0 or in top cell)
      if ((is_top || net->pin_count () == 0) && !net->name ().empty () && glob.match (net->name ())) {
        res.push_back (net);
      }
    }
  }

  return res;
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

static std::vector<db::Net *>
nets_by_name_from_netlist (db::Netlist *netlist, const std::string &name_pattern, const tl::Variant &cs)
{
  return nets_non_const (nets_by_name_const_from_netlist (netlist, name_pattern, cs));
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
  gsi::method ("flatten_circuits", &db::Netlist::flatten_circuits, gsi::arg ("circuits"),
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
  gsi::method_ext ("circuits_by_name", &circuits_by_name, gsi::arg ("name_pattern"), gsi::arg ("case_sensitive", tl::Variant (), "default"),
    "@brief Gets the circuit objects for a given name filter.\n"
    "The name filter is a glob pattern. This method will return all \\Circuit objects matching the glob pattern.\n"
    "The 'case_sensitive' argument will control whether the name is looked up in a case sensitive way or not. Note that "
    "with case insensitive search on a netlist that is case sensitive, the same name may render more than one hit. By "
    "default, case sensitivity is taken from the netlist.\n"
    "\n"
    "This method has been introduced in version 0.26.4.\n"
    "The 'case_sensitive' argument has been added in version 0.30.2."
  ) +
  gsi::method_ext ("circuits_by_name", &circuits_by_name_const, gsi::arg ("name_pattern"), gsi::arg ("case_sensitive", tl::Variant (), "default"),
    "@brief Gets the circuit objects for a given name filter (const version).\n"
    "The name filter is a glob pattern. This method will return all \\Circuit objects matching the glob pattern.\n"
    "The 'case_sensitive' argument will control whether the name is looked up in a case sensitive way or not. Note that "
    "with case insensitive search on a netlist that is case sensitive, the same name may render more than one hit. By "
    "default, case sensitivity is taken from the netlist.\n"
    "\n"
    "This constness variant has been introduced in version 0.26.8."
    "The 'case_sensitive' argument has been added in version 0.30.2."
  ) +
  gsi::method_ext ("nets_by_name", &nets_by_name_from_netlist, gsi::arg ("name_pattern"), gsi::arg ("case_sensitive", tl::Variant (), "default"),
    "@brief Gets the net objects for a given name filter.\n"
    "The name filter is a glob pattern. This method will return all \\Net objects matching the glob pattern.\n"
    "The 'case_sensitive' argument will control whether the name is looked up in a case sensitive way or not. Note that "
    "with case insensitive search on a netlist that is case sensitive, the same name may render more than one hit. By "
    "default, case sensitivity is taken from the netlist.\n"
    "\n"
    "This method has been introduced in version 0.28.4.\n"
    "The 'case_sensitive' argument has been added in version 0.30.2."
  ) +
  gsi::method_ext ("nets_by_name", &nets_by_name_const_from_netlist, gsi::arg ("name_pattern"), gsi::arg ("case_sensitive", tl::Variant (), "default"),
    "@brief Gets the net objects for a given name filter (const version).\n"
    "The name filter is a glob pattern. This method will return all \\Net objects matching the glob pattern.\n"
    "The 'case_sensitive' argument will control whether the name is looked up in a case sensitive way or not. Note that "
    "with case insensitive search on a netlist that is case sensitive, the same name may render more than one hit. By "
    "default, case sensitivity is taken from the netlist.\n"
    "\n"
    "This constness variant has been introduced in version 0.28.4."
    "The 'case_sensitive' argument has been added in version 0.30.2."
  ) +
  gsi::method ("top_circuit", static_cast<db::Circuit *(db::Netlist::*) ()> (&db::Netlist::top_circuit),
    "@brief Gets the top circuit.\n"
    "This method will return nil, if there is no top circuit. It will raise an error, if there is more than "
    "a single top circuit.\n"
    "\n"
    "This convenience method has been added in version 0.29.5."
  ) +
  gsi::method ("top_circuit", static_cast<const db::Circuit *(db::Netlist::*) () const> (&db::Netlist::top_circuit),
    "@brief Gets the top circuit (const version).\n"
    "This method will return nil, if there is no top circuit. It will raise an error, if there is more than "
    "a single top circuit.\n"
    "\n"
    "This convenience method has been added in version 0.29.5."
  ) +
  gsi::method ("top_circuits", static_cast<std::vector<db::Circuit *> (db::Netlist::*) ()> (&db::Netlist::top_circuits),
    "@brief Gets the top circuits.\n"
    "Returns a list of top circuits.\n"
    "\n"
    "This convenience method has been added in version 0.29.5."
  ) +
  gsi::method ("top_circuits", static_cast<std::vector<const db::Circuit *> (db::Netlist::*) () const> (&db::Netlist::top_circuits),
    "@brief Gets the top circuits.\n"
    "Returns a list of top circuits.\n"
    "\n"
    "This convenience method has been added in version 0.29.5."
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
  gsi::method ("make_top_level_pins", &db::Netlist::make_top_level_pins, gsi::arg ("sorted_by_name", true),
    "@brief Creates pins for top-level circuits.\n"
    "This method will turn all named nets of top-level circuits (such that are not "
    "referenced by subcircuits) into pins. This method can be used before purge to "
    "avoid that purge will remove nets which are directly connecting to subcircuits.\n"
    "\n"
    "Starting from version 0.30.9, the pins will be sorted by name by default. Sorting can be "
    "disabled by setting \\sorted_by_name to false for backward compatibility."
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
  gsi::method ("purge_devices", &db::Netlist::purge_devices,
    "@brief Purges invalid devices.\n"
    "Purges devices which are considered invalid. Such devices are for example those whose terminals are all connected to a single net.\n"
    "\n"
    "This method has been added in version 0.29.7."
  ) +
  gsi::method ("simplify", &db::Netlist::simplify,
    "@brief Convenience method that combines the simplification.\n"
    "This method is a convenience method that runs \\make_top_level_pins, \\purge, \\combine_devices and \\purge_nets."
  ) +
  gsi::method_ext ("read", &read_netlist, gsi::arg ("file"), gsi::arg ("reader"),
    "@brief Reads the netlist from the given file using the given reader object to parse the file\n"
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

}

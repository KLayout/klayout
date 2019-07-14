
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
#include "dbNetlistCompare.h"

namespace {

/**
 *  @brief A NetlistDeviceExtractor implementation that allows reimplementation of the virtual methods
 */
class GenericNetlistCompareLogger
  : public gsi::ObjectBase, public db::NetlistCompareLogger
{
public:
  GenericNetlistCompareLogger ()
    : db::NetlistCompareLogger ()
  {
    //  .. nothing yet ..
  }

  virtual void begin_netlist (const db::Netlist *a, const db::Netlist *b)
  {
    if (cb_begin_netlist.can_issue ()) {
      cb_begin_netlist.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::begin_netlist_fb, a, b);
    } else {
      db::NetlistCompareLogger::begin_netlist (a, b);
    }
  }

  void begin_netlist_fb (const db::Netlist *a, const db::Netlist *b)
  {
    db::NetlistCompareLogger::begin_netlist (a, b);
  }

  virtual void end_netlist (const db::Netlist *a, const db::Netlist *b)
  {
    if (cb_end_netlist.can_issue ()) {
      cb_end_netlist.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::end_netlist_fb, a, b);
    } else {
      db::NetlistCompareLogger::end_netlist (a, b);
    }
  }

  void end_netlist_fb (const db::Netlist *a, const db::Netlist *b)
  {
    db::NetlistCompareLogger::end_netlist (a, b);
  }

  virtual void device_class_mismatch (const db::DeviceClass *a, const db::DeviceClass *b)
  {
    if (cb_device_class_mismatch.can_issue ()) {
      cb_device_class_mismatch.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::device_class_mismatch_fb, a, b);
    } else {
      db::NetlistCompareLogger::device_class_mismatch (a, b);
    }
  }

  void device_class_mismatch_fb (const db::DeviceClass *a, const db::DeviceClass *b)
  {
    db::NetlistCompareLogger::device_class_mismatch (a, b);
  }

  virtual void begin_circuit (const db::Circuit *a, const db::Circuit *b)
  {
    if (cb_begin_circuit.can_issue ()) {
      cb_begin_circuit.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::begin_circuit_fb, a, b);
    } else {
      db::NetlistCompareLogger::begin_circuit (a, b);
    }
  }

  void begin_circuit_fb (const db::Circuit *a, const db::Circuit *b)
  {
    db::NetlistCompareLogger::begin_circuit (a, b);
  }

  virtual void end_circuit (const db::Circuit *a, const db::Circuit *b, bool matching)
  {
    if (cb_end_circuit.can_issue ()) {
      cb_end_circuit.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::end_circuit_fb, a, b, matching);
    } else {
      db::NetlistCompareLogger::end_circuit (a, b, matching);
    }
  }

  void end_circuit_fb (const db::Circuit *a, const db::Circuit *b, bool matching)
  {
    db::NetlistCompareLogger::end_circuit (a, b, matching);
  }

  virtual void circuit_skipped (const db::Circuit *a, const db::Circuit *b)
  {
    if (cb_circuit_skipped.can_issue ()) {
      cb_circuit_skipped.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::circuit_skipped_fb, a, b);
    } else {
      db::NetlistCompareLogger::circuit_skipped (a, b);
    }
  }

  void circuit_skipped_fb (const db::Circuit *a, const db::Circuit *b)
  {
    db::NetlistCompareLogger::circuit_skipped (a, b);
  }

  virtual void circuit_mismatch (const db::Circuit *a, const db::Circuit *b)
  {
    if (cb_circuit_mismatch.can_issue ()) {
      cb_circuit_mismatch.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::circuit_mismatch_fb, a, b);
    } else {
      db::NetlistCompareLogger::circuit_mismatch (a, b);
    }
  }

  void circuit_mismatch_fb (const db::Circuit *a, const db::Circuit *b)
  {
    db::NetlistCompareLogger::circuit_mismatch (a, b);
  }

  virtual void match_nets (const db::Net *a, const db::Net *b)
  {
    if (cb_match_nets.can_issue ()) {
      cb_match_nets.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::match_nets_fb, a, b);
    } else {
      db::NetlistCompareLogger::match_nets (a, b);
    }
  }

  void match_nets_fb (const db::Net *a, const db::Net *b)
  {
    db::NetlistCompareLogger::match_nets (a, b);
  }

  virtual void match_ambiguous_nets (const db::Net *a, const db::Net *b)
  {
    if (cb_match_ambiguous_nets.can_issue ()) {
      cb_match_ambiguous_nets.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::match_ambiguous_nets_fb, a, b);
    } else {
      db::NetlistCompareLogger::match_ambiguous_nets (a, b);
    }
  }

  void match_ambiguous_nets_fb (const db::Net *a, const db::Net *b)
  {
    db::NetlistCompareLogger::match_ambiguous_nets (a, b);
  }

  virtual void net_mismatch (const db::Net *a, const db::Net *b)
  {
    if (cb_net_mismatch.can_issue ()) {
      cb_net_mismatch.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::net_mismatch_fb, a, b);
    } else {
      db::NetlistCompareLogger::net_mismatch (a, b);
    }
  }

  void net_mismatch_fb (const db::Net *a, const db::Net *b)
  {
    db::NetlistCompareLogger::net_mismatch (a, b);
  }

  virtual void match_devices (const db::Device *a, const db::Device *b)
  {
    if (cb_match_devices.can_issue ()) {
      cb_match_devices.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::match_devices_fb, a, b);
    } else {
      db::NetlistCompareLogger::match_devices (a, b);
    }
  }

  void match_devices_fb (const db::Device *a, const db::Device *b)
  {
    db::NetlistCompareLogger::match_devices (a, b);
  }

  virtual void match_devices_with_different_parameters (const db::Device *a, const db::Device *b)
  {
    if (cb_match_devices_with_different_parameters.can_issue ()) {
      cb_match_devices_with_different_parameters.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::match_devices_with_different_parameters_fb, a, b);
    } else {
      db::NetlistCompareLogger::match_devices_with_different_parameters (a, b);
    }
  }

  void match_devices_with_different_parameters_fb (const db::Device *a, const db::Device *b)
  {
    db::NetlistCompareLogger::match_devices_with_different_parameters (a, b);
  }

  virtual void match_devices_with_different_device_classes (const db::Device *a, const db::Device *b)
  {
    if (cb_match_devices_with_different_device_classes.can_issue ()) {
      cb_match_devices_with_different_device_classes.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::match_devices_with_different_device_classes_fb, a, b);
    } else {
      db::NetlistCompareLogger::match_devices_with_different_device_classes (a, b);
    }
  }

  void match_devices_with_different_device_classes_fb (const db::Device *a, const db::Device *b)
  {
    db::NetlistCompareLogger::match_devices_with_different_device_classes (a, b);
  }

  virtual void device_mismatch (const db::Device *a, const db::Device *b)
  {
    if (cb_device_mismatch.can_issue ()) {
      cb_device_mismatch.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::device_mismatch_fb, a, b);
    } else {
      db::NetlistCompareLogger::device_mismatch (a, b);
    }
  }

  void device_mismatch_fb (const db::Device *a, const db::Device *b)
  {
    db::NetlistCompareLogger::device_mismatch (a, b);
  }

  virtual void match_pins (const db::Pin *a, const db::Pin *b)
  {
    if (cb_match_pins.can_issue ()) {
      cb_match_pins.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::match_pins_fb, a, b);
    } else {
      db::NetlistCompareLogger::match_pins (a, b);
    }
  }

  void match_pins_fb (const db::Pin *a, const db::Pin *b)
  {
    db::NetlistCompareLogger::match_pins (a, b);
  }

  virtual void pin_mismatch (const db::Pin *a, const db::Pin *b)
  {
    if (cb_pin_mismatch.can_issue ()) {
      cb_pin_mismatch.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::pin_mismatch_fb, a, b);
    } else {
      db::NetlistCompareLogger::pin_mismatch (a, b);
    }
  }

  void pin_mismatch_fb (const db::Pin *a, const db::Pin *b)
  {
    db::NetlistCompareLogger::pin_mismatch (a, b);
  }

  virtual void match_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    if (cb_match_subcircuits.can_issue ()) {
      cb_match_subcircuits.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::match_subcircuits_fb, a, b);
    } else {
      db::NetlistCompareLogger::match_subcircuits (a, b);
    }
  }

  void match_subcircuits_fb (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    db::NetlistCompareLogger::match_subcircuits (a, b);
  }

  virtual void subcircuit_mismatch (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    if (cb_subcircuit_mismatch.can_issue ()) {
      cb_subcircuit_mismatch.issue<GenericNetlistCompareLogger> (&GenericNetlistCompareLogger::subcircuit_mismatch_fb, a, b);
    } else {
      db::NetlistCompareLogger::subcircuit_mismatch (a, b);
    }
  }

  void subcircuit_mismatch_fb (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    db::NetlistCompareLogger::subcircuit_mismatch (a, b);
  }

  gsi::Callback cb_begin_netlist;
  gsi::Callback cb_end_netlist;
  gsi::Callback cb_device_class_mismatch;
  gsi::Callback cb_begin_circuit;
  gsi::Callback cb_end_circuit;
  gsi::Callback cb_circuit_skipped;
  gsi::Callback cb_match_nets;
  gsi::Callback cb_net_mismatch;
  gsi::Callback cb_circuit_mismatch;
  gsi::Callback cb_match_ambiguous_nets;
  gsi::Callback cb_match_devices;
  gsi::Callback cb_match_devices_with_different_parameters;
  gsi::Callback cb_match_devices_with_different_device_classes;
  gsi::Callback cb_device_mismatch;
  gsi::Callback cb_match_pins;
  gsi::Callback cb_pin_mismatch;
  gsi::Callback cb_match_subcircuits;
  gsi::Callback cb_subcircuit_mismatch;
};

}

namespace gsi
{

Class<db::NetlistCompareLogger> decl_dbNetlistCompareLogger ("db", "NetlistCompareLogger",
  gsi::Methods (),
  "@brief A base class for netlist comparer event receivers\n"
  "See \\GenericNetlistCompareLogger for custom implementations of such receivers."
);

Class<GenericNetlistCompareLogger> decl_GenericNetlistCompareLogger (decl_dbNetlistCompareLogger, "db", "GenericNetlistCompareLogger",
  gsi::callback ("begin_netlist", &GenericNetlistCompareLogger::begin_netlist, &GenericNetlistCompareLogger::cb_begin_netlist, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called at the beginning of the compare process.\n"
    "This method is called once when the compare run begins.\n"
  ) +
  gsi::callback ("end_netlist", &GenericNetlistCompareLogger::end_netlist, &GenericNetlistCompareLogger::cb_end_netlist, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called at the end of the compare process.\n"
    "This method is called once when the compare run ended.\n"
  ) +
  gsi::callback ("device_class_mismatch", &GenericNetlistCompareLogger::device_class_mismatch, &GenericNetlistCompareLogger::cb_device_class_mismatch, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when device classes can't be compared.\n"
    "This method is called when a device class can't be mapped to a partner in the other netlist. In this case, "
    "this method is called with the one device class and nil for the other class.\n"
  ) +
  gsi::callback ("begin_circuit", &GenericNetlistCompareLogger::begin_circuit, &GenericNetlistCompareLogger::cb_begin_circuit, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when a new circuit is compared.\n"
    "This compare procedure will run the netlist compare circuit vs. circuit in a bottom-up fashion.\n"
    "Before each circuit is compared, this method is called once with the circuits that are about to be compared.\n"
    "After the circuit has been compared, \\end_circuit will be called.\n"
    "\n"
    "In some cases, the compare algorithm will decide that circuits can't be compared. This happens if for "
    "some or all subcircuits the pin assignment can't be derived. In this case, \\circuit_skipped will be called once "
    "instead of \\begin_circuit and \\end_circuit.\n"
  ) +
  gsi::callback ("end_circuit", &GenericNetlistCompareLogger::end_circuit, &GenericNetlistCompareLogger::cb_end_circuit, gsi::arg ("a"), gsi::arg ("b"), gsi::arg ("matching"),
    "@brief This function is called at the end of the compare process.\n"
    "The 'matching' argument indicates whether the circuits have been identified as identical.\n"
    "See \\begin_circuit for details."
  ) +
  gsi::callback ("circuit_skipped", &GenericNetlistCompareLogger::circuit_skipped, &GenericNetlistCompareLogger::cb_circuit_skipped, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when circuits can't be compared.\n"
    "If there is a known circuit pair, but the circuits can be compared - for example because subcircuits can't be identified - this method will be called with "
    "both circuits.\n"
    "\n"
    "This method is called instead of \\begin_circuit and \\end_circuit."
  ) +
  gsi::callback ("circuit_mismatch", &GenericNetlistCompareLogger::circuit_mismatch, &GenericNetlistCompareLogger::cb_circuit_mismatch, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when circuits can't be compared.\n"
    "This method is called when a circuit can't be mapped to a partner in the other netlist. In this case, "
    "this method is called with the one circuit and nil for the other circuit.\n"
    "\n"
    "This method is called instead of \\begin_circuit and \\end_circuit."
  ) +
  gsi::callback ("match_nets", &GenericNetlistCompareLogger::match_nets, &GenericNetlistCompareLogger::cb_match_nets, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when two nets are identified.\n"
    "If two nets are identified as a corresponding pair, this method will be called with both nets.\n"
    "If the nets can be paired, but this match is ambiguous, \\match_ambiguous_nets will be called instead.\n"
    "If nets can't be matched to a partner, \\net_mismatch will be called.\n"
  ) +
  gsi::callback ("match_ambiguous_nets", &GenericNetlistCompareLogger::match_ambiguous_nets, &GenericNetlistCompareLogger::cb_match_ambiguous_nets, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when two nets are identified, but this choice is ambiguous.\n"
    "This choice is a last-resort fallback to allow continuation of the compare procedure. It is likely that this "
    "compare will fail later. Looking for ambiguous nets allows deduction of the origin of this faulty decision. "
    "See \\match_nets for more details."
  ) +
  gsi::callback ("net_mismatch", &GenericNetlistCompareLogger::net_mismatch, &GenericNetlistCompareLogger::cb_net_mismatch, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when a net can't be paired.\n"
    "This method will be called, if a net cannot be identified as identical with another net. The corresponding argument "
    "will identify the net and source netlist. The other argument will be nil.\n"
    "\n"
    "In some cases, a mismatch is reported with two nets given. This means,\n"
    "nets are known not to match. Still the compare algorithm will proceed as\n"
    "if these nets were equivalent to derive further matches.\n"
  ) +
  gsi::callback ("match_devices", &GenericNetlistCompareLogger::match_devices, &GenericNetlistCompareLogger::cb_match_devices, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when two devices are identified.\n"
    "If two devices are identified as a corresponding pair, this method will be called with both devices.\n"
    "If the devices can be paired, but the device parameters don't match, \\match_devices_with_different_parameters will be called instead.\n"
    "If the devices can be paired, but the device classes don't match, \\match_devices_with_different_device_classes will be called instead.\n"
    "If devices can't be matched, \\device_mismatch will be called with the one device considered and the other device being nil."
  ) +
  gsi::callback ("match_devices_with_different_parameters", &GenericNetlistCompareLogger::match_devices_with_different_parameters, &GenericNetlistCompareLogger::cb_match_devices_with_different_parameters, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when two devices are identified but have different parameters.\n"
    "See \\match_devices for details.\n"
  ) +
  gsi::callback ("match_devices_with_different_device_classes", &GenericNetlistCompareLogger::match_devices_with_different_device_classes, &GenericNetlistCompareLogger::cb_match_devices_with_different_device_classes, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when two devices are identified but have different device classes.\n"
    "See \\match_devices for details.\n"
  ) +
  gsi::callback ("device_mismatch", &GenericNetlistCompareLogger::device_mismatch, &GenericNetlistCompareLogger::cb_device_mismatch, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when two devices can't be paired.\n"
    "This will report the device considered in a or b. The other argument is nil. "
    "See \\match_devices for details.\n"
  ) +
  gsi::callback ("match_pins", &GenericNetlistCompareLogger::match_pins, &GenericNetlistCompareLogger::cb_match_pins, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when two pins are identified.\n"
    "If two pins are identified as a corresponding pair, this method will be called with both pins.\n"
    "If pins can't be matched, \\pin_mismatch will be called with the one pin considered and the other pin being nil."
  ) +
  gsi::callback ("pin_mismatch", &GenericNetlistCompareLogger::pin_mismatch, &GenericNetlistCompareLogger::cb_pin_mismatch, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when two pins can't be paired.\n"
    "This will report the pin considered in a or b. The other argument is nil. "
    "See \\match_pins for details.\n"
  ) +
  gsi::callback ("match_subcircuits", &GenericNetlistCompareLogger::match_subcircuits, &GenericNetlistCompareLogger::cb_match_subcircuits, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when two subcircuits are identified.\n"
    "If two subcircuits are identified as a corresponding pair, this method will be called with both subcircuits.\n"
    "If subcircuits can't be matched, \\subcircuit_mismatch will be called with the one subcircuit considered and the other subcircuit being nil."
  ) +
  gsi::callback ("subcircuit_mismatch", &GenericNetlistCompareLogger::subcircuit_mismatch, &GenericNetlistCompareLogger::cb_subcircuit_mismatch, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This function is called when two subcircuits can't be paired.\n"
    "This will report the subcircuit considered in a or b. The other argument is nil. "
    "See \\match_subcircuits for details.\n"
  ),
  "@brief An event receiver for the netlist compare feature.\n"
  "The \\NetlistComparer class will send compare events to a logger derived from this class. "
  "Use this class to implement your own logger class. You can override on of it's methods to receive certain "
  "kind of events."
  "\n"
  "This class has been introduced in version 0.26.\n"
);

static db::NetlistComparer *make_comparer0 ()
{
  return new db::NetlistComparer ();
}

static db::NetlistComparer *make_comparer1 (GenericNetlistCompareLogger *logger)
{
  return new db::NetlistComparer (logger);
}

Class<db::NetlistComparer> decl_dbNetlistComparer ("db", "NetlistComparer",
  gsi::constructor ("new", &make_comparer0,
    "@brief Creates a new comparer object.\n"
    "See the class description for more details."
  ) +
  gsi::constructor ("new", &make_comparer1, gsi::arg ("logger"),
    "@brief Creates a new comparer object.\n"
    "The logger is a delegate or event receiver which the comparer will send compare events to. "
    "See the class description for more details."
  ) +
  gsi::method ("same_nets", &db::NetlistComparer::same_nets, gsi::arg ("net_a"), gsi::arg ("net_b"),
    "@brief Marks two nets as identical.\n"
    "This makes a net net_a in netlist a identical to the corresponding\n"
    "net net_b in netlist b (see \\compare).\n"
    "Otherwise, the algorithm will try to identify nets according to their topology. "
    "This method can be used to supply hints to the compare algorithm. It will use "
    "these hints to derive further identities."
  ) +
  gsi::method ("equivalent_pins", (void (db::NetlistComparer::*) (const db::Circuit *, size_t, size_t)) &db::NetlistComparer::equivalent_pins, gsi::arg ("circuit_b"), gsi::arg ("pin_id1"), gsi::arg ("pin_id2"),
    "@brief Marks two pins of the given circuit as equivalent (i.e. they can be swapped).\n"
    "Only circuits from the second input can be given swappable pins. "
    "This will imply the same swappable pins on the equivalent circuit of the first input. "
    "To mark multiple pins as swappable, use the version that takes a list of pins."
  ) +
  gsi::method ("equivalent_pins", (void (db::NetlistComparer::*) (const db::Circuit *, const std::vector<size_t> &)) &db::NetlistComparer::equivalent_pins, gsi::arg ("circuit_b"), gsi::arg ("pin_ids"),
    "@brief Marks several pins of the given circuit as equivalent (i.e. they can be swapped).\n"
    "Only circuits from the second input can be given swappable pins. "
    "This will imply the same swappable pins on the equivalent circuit of the first input. "
    "This version is a generic variant of the two-pin version of this method."
  ) +
  gsi::method ("same_device_classes", &db::NetlistComparer::same_device_classes, gsi::arg ("dev_cls_a"), gsi::arg ("dev_cls_b"),
    "@brief Marks two device classes as identical.\n"
    "This makes a device class dev_cls_a in netlist a identical to the corresponding\n"
    "device class dev_cls_b in netlist b (see \\compare).\n"
    "By default device classes with the same name are identical.\n"
  ) +
  gsi::method ("same_circuits", &db::NetlistComparer::same_circuits, gsi::arg ("circuit_a"), gsi::arg ("circuit_b"),
    "@brief Marks two circuits as identical.\n"
    "This method makes a circuit circuit_a in netlist a identical to the corresponding\n"
    "circuit circuit_b in netlist b (see \\compare). By default circuits with the same name are identical.\n"
  ) +
  gsi::method ("min_capacitance=", &db::NetlistComparer::exclude_caps, gsi::arg ("threshold"),
    "@brief Excludes all capacitor devices with a capacitance values less than the given threshold.\n"
    "To reset this constraint, set this attribute to zero."
  ) +
  gsi::method ("max_resistance=", &db::NetlistComparer::exclude_resistors, gsi::arg ("threshold"),
    "@brief Excludes all resistor devices with a resistance values higher than the given threshold.\n"
    "To reset this constraint, set this attribute to zero."
  ) +
  gsi::method ("max_depth=", &db::NetlistComparer::set_max_depth, gsi::arg ("n"),
    "@brief Sets the maximum seach depth\n"
    "This value limits the search depth of the backtracking algorithm to the\n"
    "given number of jumps.\n"
  ) +
  gsi::method ("max_depth", &db::NetlistComparer::max_depth,
    "@brief Gets the maximum seach depth\n"
    "See \\max_depth= for details."
  ) +
  gsi::method ("max_branch_complexity=", &db::NetlistComparer::set_max_branch_complexity, gsi::arg ("n"),
    "@brief Sets the maximum branch complexity\n"
    "This value limits the maximum branch complexity of the backtracking algorithm.\n"
    "The complexity is the accumulated number of branch options with ambiguous\n"
    "net matches. Backtracking will stop when the maximum number of options\n"
    "has been exceeded.\n"
    "\n"
    "As the computational complexity is the square of the branch count,\n"
    "this value should be adjusted carefully.\n"
  ) +
  gsi::method ("max_branch_complexity", &db::NetlistComparer::max_branch_complexity,
    "@brief Gets the maximum branch complexity\n"
    "See \\max_branch_complexity= for details."
  ) +
  gsi::method ("compare", (bool (db::NetlistComparer::*) (const db::Netlist *, const db::Netlist *) const) &db::NetlistComparer::compare, gsi::arg ("netlist_a"), gsi::arg ("netlist_b"),
    "@brief Compares two netlists.\n"
    "This method will perform the actual netlist compare. It will return true if both netlists are identical. "
    "If the comparer has been configured with \\same_nets or similar methods, the objects given there must "
    "be located inside 'circuit_a' and 'circuit_b' respectively."
  ) +
  gsi::method ("compare", (bool (db::NetlistComparer::*) (const db::Netlist *, const db::Netlist *, db::NetlistCompareLogger *) const) &db::NetlistComparer::compare, gsi::arg ("netlist_a"), gsi::arg ("netlist_b"), gsi::arg ("logger"),
    "@brief Compares two netlists.\n"
    "This method will perform the actual netlist compare using the given logger. It will return true if both netlists are identical. "
    "If the comparer has been configured with \\same_nets or similar methods, the objects given there must "
    "be located inside 'circuit_a' and 'circuit_b' respectively."
  ),
  "@brief Compares two netlists\n"
  "This class performs a comparison of two netlists.\n"
  "It can be used with an event receiver (logger) to track the errors and net mismatches. "
  "Event receivers are derived from class \\GenericNetlistCompareLogger."
  "\n"
  "The netlist comparer can be configured in different ways. Specific hints can be given for nets, device classes or circuits "
  "to improve efficiency and reliability of the graph equivalence deduction algorithm. "
  "For example, objects can be marked as equivalent using \\same_nets, \\same_circuits etc. "
  "The compare algorithm will then use these hints to derive further equivalences. This way, "
  "ambiguities can be resolved.\n"
  "\n"
  "Another configuration option relates to swappable pins of subcircuits. If pins are marked this way, the compare algorithm may swap them to "
  "achieve net matching. Swappable pins belong to an 'equivalence group' and can be defined with \\equivalent_pins.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

}

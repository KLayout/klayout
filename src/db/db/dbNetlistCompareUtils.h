
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

#ifndef _HDR_dbNetlistCompareUtils
#define _HDR_dbNetlistCompareUtils

#include "dbCommon.h"

#include "tlEquivalenceClusters.h"
#include "tlProgress.h"

#include <string>
#include <limits>
#include <map>
#include <set>

namespace db
{

class Netlist;
class Net;
class SubCircuit;
class Device;
class DeviceClass;
class Circuit;
class SubCircuit;
class NetGraph;
class NetGraphNode;
class NetlistCompareLogger;

// --------------------------------------------------------------------------------------------------------------------
//  Global netlist compare options

struct DB_PUBLIC NetlistCompareGlobalOptions
{
  NetlistCompareGlobalOptions ();
  void ensure_initialized ();

  bool debug_netcompare;
  bool debug_netgraph;

  static NetlistCompareGlobalOptions *options ();

private:
  bool m_is_initialized;
};

// --------------------------------------------------------------------------------------------------------------------
//  Some definitions for pseudo-Ids

//  A constant indicating a failed match
const size_t failed_match = std::numeric_limits<size_t>::max ();

//  A constant indicating an unknown match
// const size_t unknown_match = std::numeric_limits<size_t>::max () - 1;

//  A constant indicating an invalid ID
const size_t invalid_id = std::numeric_limits<size_t>::max ();

//  A constant indicating an unknown ID
const size_t unknown_id = std::numeric_limits<size_t>::max () - 1;

// --------------------------------------------------------------------------------------------------------------------
//  Some utilities

std::string nl_compare_debug_indent (size_t depth);
std::string nets2string (const db::Net *a, const db::Net *b);
std::string nets2string (const std::pair<const db::Net *, const db::Net *> &np);

// --------------------------------------------------------------------------------------------------------------------
//  Net name compare

/**
 *  @brief Derives the common case sensitivity for two netlists
 */
bool combined_case_sensitive (const db::Netlist *a, const db::Netlist *b);

/**
 *  @brief Gets the extended net name
 *  This name is used for comparing the net names and also employs the pin name if one is given
 */
const std::string &extended_net_name (const db::Net *n);

/**
 *  @brief Compare two nets by name
 */
int name_compare (const db::Net *a, const db::Net *b);

/**
 *  @brief Returns a value indicating whether two nets are different by name
 *  Two unnamed nets are never different.
 */
bool net_names_are_different (const db::Net *a, const db::Net *b);

/**
 *  @brief Returns a value indicating whether two nets are equal by name
 *  Two unnamed nets are never equal.
 */
bool net_names_are_equal (const db::Net *a, const db::Net *b);

// --------------------------------------------------------------------------------------------------------------------
//  DeviceCompare definition and implementation

/**
 *  @brief The device compare function with "less" (operator()) and "equal" predicates
 *
 *  Device comparison is based on the equivalence of device classes (by category) and
 *  in a second step, by equivalence of the devices. The device class will implement
 *  the device equivalence function.
 */
struct DB_PUBLIC DeviceCompare
{
  bool operator() (const std::pair<const db::Device *, size_t> &d1, const std::pair<const db::Device *, size_t> &d2) const;
  bool equals (const std::pair<const db::Device *, size_t> &d1, const std::pair<const db::Device *, size_t> &d2) const;
};

// --------------------------------------------------------------------------------------------------------------------
//  SubCircuitCompare definition and implementation

/**
 *  @brief The compare function for subcircuits
 *
 *  As Subcircuits are not parametrized, the comparison of subcircuits is only based on
 *  the circuit equivalence (via category).
 */
struct DB_PUBLIC SubCircuitCompare
{
  bool operator() (const std::pair<const db::SubCircuit *, size_t> &sc1, const std::pair<const db::SubCircuit *, size_t> &sc2) const;
  bool equals (const std::pair<const db::SubCircuit *, size_t> &sc1, const std::pair<const db::SubCircuit *, size_t> &sc2) const;
};

// --------------------------------------------------------------------------------------------------------------------
//  CircuitPinMapper definition

/**
 *  @brief The Circuit pin categorizer handles swappable pin definitions per circuit
 *
 *  Swappable pins are implemented by mapping a pin ID to an equivalent or
 *  effective ID which is shared by all swappable pins.
 *
 *  This class manages swappable pins on a per-circuit basis.
 */
class DB_PUBLIC CircuitPinCategorizer
{
public:
  CircuitPinCategorizer ();

  void map_pins (const db::Circuit *circuit, size_t pin1_id, size_t pin2_id);
  void map_pins (const db::Circuit *circuit, const std::vector<size_t> &pin_ids);

  size_t is_mapped (const db::Circuit *circuit, size_t pin_id) const;
  size_t normalize_pin_id (const db::Circuit *circuit, size_t pin_id) const;

private:
  std::map<const db::Circuit *, tl::equivalence_clusters<size_t> > m_pin_map;
};

// --------------------------------------------------------------------------------------------------------------------
//  CircuitMapper definition

/**
 *  @brief Handles circuit equivalence (A to B netlist)
 *
 *  The object specifies the mapping between the circuits of
 *  netlist A and B and also the pin mapping between the circuits from these netlists.
 *
 *  The "other" attribute will hold the circuit for the other netlist.
 *  The other methods handle pin mapping from "other" into "this" pin space.
 */
class DB_PUBLIC CircuitMapper
{
public:
  CircuitMapper ();

  void set_other (const db::Circuit *other)
  {
    mp_other = other;
  }

  const db::Circuit *other () const
  {
    return mp_other;
  }

  void map_pin (size_t this_pin, size_t other_pin);
  bool has_other_pin_for_this_pin (size_t this_pin) const;
  bool has_this_pin_for_other_pin (size_t other_pin) const;

  size_t other_pin_from_this_pin (size_t this_pin) const;
  size_t this_pin_from_other_pin (size_t other_pin) const;

private:
  const db::Circuit *mp_other;
  std::map<size_t, size_t> m_pin_map, m_rev_pin_map;
};

// --------------------------------------------------------------------------------------------------------------------
//  DeviceFilter definition and implementation

/**
 *  @brief A device filter class
 *
 *  This class implements a device filter which is used to skip devices when
 *  generating the net graph. This is useful for stripping small caps or big
 *  resistors.
 */
class DB_PUBLIC DeviceFilter
{
public:
  DeviceFilter (double cap_threshold, double res_threshold);

  bool filter (const db::Device *device) const;

private:
  double m_cap_threshold, m_res_threshold;
};

// --------------------------------------------------------------------------------------------------------------------
//  A generic equivalence mapper

template <class Obj>
class generic_equivalence_tracker
{
public:
  generic_equivalence_tracker ()
  {
    //  .. nothing yet ..
  }

  bool map (const Obj *a, const Obj *b)
  {
    std::pair<typename std::map<const Obj *, const Obj *>::iterator, bool> inserted1 = m_eq.insert (std::make_pair (a, b));
    tl_assert (inserted1.first->second == b);
    std::pair<typename std::map<const Obj *, const Obj *>::iterator, bool> inserted2 = m_eq.insert (std::make_pair (b, a));
    tl_assert (inserted2.first->second == a);
    return inserted1.second;
  }

  void unmap (const Obj *a, const Obj *b)
  {
    m_eq.erase (a);
    m_eq.erase (b);
  }

  const Obj *other (const Obj *o) const
  {
    typename std::map<const Obj *, const Obj *>::const_iterator i = m_eq.find (o);
    return i == m_eq.end () ? 0 : i->second;
  }

public:
  std::map<const Obj *, const Obj *> m_eq;
};

// --------------------------------------------------------------------------------------------------------------------
//  A class describing the equivalence between subcircuits we established so far

class SubCircuitEquivalenceTracker
  : public generic_equivalence_tracker<db::SubCircuit>
{
public:
  SubCircuitEquivalenceTracker () : generic_equivalence_tracker<db::SubCircuit> () { }
};

// --------------------------------------------------------------------------------------------------------------------
//  A class describing the equivalence between devices we established so far

class DeviceEquivalenceTracker
  : public generic_equivalence_tracker<db::Device>
{
public:
  DeviceEquivalenceTracker () : generic_equivalence_tracker<db::Device> () { }
};

// --------------------------------------------------------------------------------------------------------------------
//  generic_categorizer definition and implementation

/**
 *  @brief A generic categorizer
 *
 *  The objective of this class is to supply a category ID for a given object.
 *  The category ID also identifies equivalent objects from netlist A and B.
 */
template <class Obj>
class DB_PUBLIC generic_categorizer
{
public:
  generic_categorizer (bool with_name = true);

  void set_case_sensitive (bool f);
  void same (const Obj *ca, const Obj *cb);
  bool has_cat_for (const Obj *cls);
  size_t cat_for (const Obj *cls);

public:
  std::map<const Obj *, size_t> m_cat_by_ptr;
  std::map<std::string, size_t> m_cat_by_name;
  size_t m_next_cat;
  bool m_with_name;
  bool m_case_sensitive;
};

// --------------------------------------------------------------------------------------------------------------------
//  DeviceCategorizer definition and implementation

/**
 *  @brief A device categorizer
 *
 *  The objective of this class is to supply a category ID for a given device class.
 *  The category ID also identities equivalent device classes from netlist A and B.
 */
class DB_PUBLIC DeviceCategorizer
  : private generic_categorizer<db::DeviceClass>
{
public:
  DeviceCategorizer ();

  void same_class (const db::DeviceClass *ca, const db::DeviceClass *cb);
  size_t cat_for_device (const db::Device *device);

  bool has_cat_for_device_class (const db::DeviceClass *cls)
  {
    return generic_categorizer<db::DeviceClass>::has_cat_for (cls);
  }

  size_t cat_for_device_class (const db::DeviceClass *cls)
  {
    return generic_categorizer<db::DeviceClass>::cat_for (cls);
  }

  void clear_strict_device_categories ();
  void set_strict_device_category (size_t cat);
  bool is_strict_device_category (size_t cat) const;

  void set_case_sensitive (bool f)
  {
    generic_categorizer::set_case_sensitive (f);
  }

private:
  std::set<size_t> m_strict_device_categories;
};

// --------------------------------------------------------------------------------------------------------------------
//  CircuitCategorizer definition and implementation

/**
 *  @brief A circuit categorizer
 *
 *  The objective of this class is to supply a category ID for a given device circuit.
 *  The category ID also identities equivalent circuit from netlist A and B.
 */
class DB_PUBLIC CircuitCategorizer
  : private generic_categorizer<db::Circuit>
{
public:
  CircuitCategorizer ();

  void same_circuit (const db::Circuit *ca, const db::Circuit *cb);
  size_t cat_for_subcircuit (const db::SubCircuit *subcircuit);

  size_t cat_for_circuit (const db::Circuit *cr)
  {
    return generic_categorizer<db::Circuit>::cat_for (cr);
  }

  void set_case_sensitive (bool f)
  {
    generic_categorizer::set_case_sensitive (f);
  }
};

}

#endif

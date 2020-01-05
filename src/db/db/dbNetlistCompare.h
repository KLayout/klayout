
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#ifndef _HDR_dbNetlistCompare
#define _HDR_dbNetlistCompare

#include "dbCommon.h"
#include "dbNetlist.h"

#include <set>
#include <map>

namespace db
{

class CircuitPinMapper;
class DeviceFilter;
class DeviceCategorizer;
class CircuitCategorizer;
class CircuitMapper;
class NetGraph;

/**
 * @brief A receiver for netlist compare events
 */
class DB_PUBLIC NetlistCompareLogger
{
public:
  NetlistCompareLogger () { }
  virtual ~NetlistCompareLogger () { }

  /**
   *  @brief Begin logging for netlist a and b
   */
  virtual void begin_netlist (const db::Netlist * /*a*/, const db::Netlist * /*b*/) { }

  /**
   *  @brief End logging for netlist a and b
   */
  virtual void end_netlist (const db::Netlist * /*a*/, const db::Netlist * /*b*/) { }

  /**
   *  @brief There is a device class mismatch
   *  "a" is null if there is no match for b and vice versa.
   */
  virtual void device_class_mismatch (const db::DeviceClass * /*a*/, const db::DeviceClass * /*b*/) { }

  /**
   *  @brief Begin logging for circuit a and b
   */
  virtual void begin_circuit (const db::Circuit * /*a*/, const db::Circuit * /*b*/) { }

  /**
   *  @brief End logging for circuit a and b
   */
  virtual void end_circuit (const db::Circuit * /*a*/, const db::Circuit * /*b*/, bool /*matching*/) { }

  /**
   *  @brief Circuits are skipped
   *  Circuits are skipped if their subcircuits could not be matched.
   */
  virtual void circuit_skipped (const db::Circuit * /*a*/, const db::Circuit * /*b*/) { }

  /**
   *  @brief There is a circuit mismatch
   *  "a" is null if there is no match for b and vice versa.
   */
  virtual void circuit_mismatch (const db::Circuit * /*a*/, const db::Circuit * /*b*/) { }

  /**
   *  @brief Nets a and b match exactly
   */
  virtual void match_nets (const db::Net * /*a*/, const db::Net * /*b*/) { }

  /**
   *  @brief Nets a and b are matched, but are ambiguous
   *  Other nets might also match with a and also with b. Matching this a and b is
   *  an arbitrary decision.
   */
  virtual void match_ambiguous_nets (const db::Net * /*a*/, const db::Net * /*b*/) { }

  /**
   *  @brief Net a or b doesn't match
   *  "a" is null if there is no match for b and vice versa.
   *  In some cases, a mismatch is reported with two nets given. This means,
   *  nets are known not to match. Still the compare algorithm will proceed as
   *  if these nets were equivalent to derive further matches.
   */
  virtual void net_mismatch (const db::Net * /*a*/, const db::Net * /*b*/) { }

  /**
   *  @brief Devices a and b match exactly
   */
  virtual void match_devices (const db::Device * /*a*/, const db::Device * /*b*/) { }

  /**
   *  @brief Devices a and b are matched but have different parameters
   */
  virtual void match_devices_with_different_parameters (const db::Device * /*a*/, const db::Device * /*b*/) { }

  /**
   *  @brief Devices a and b are matched but have different device classes
   */
  virtual void match_devices_with_different_device_classes (const db::Device * /*a*/, const db::Device * /*b*/) { }

  /**
   *  @brief Device a or b doesn't match
   *  "a" is null if there is no match for b and vice versa.
   */
  virtual void device_mismatch (const db::Device * /*a*/, const db::Device * /*b*/) { }

  /**
   *  @brief Pins a and b of the current circuit are matched
   */
  virtual void match_pins (const db::Pin * /*a*/, const db::Pin * /*b*/) { }

  /**
   *  @brief Pin a or b doesn't match
   *  "a" is null if there is no match for b and vice versa.
   */
  virtual void pin_mismatch (const db::Pin * /*a*/, const db::Pin * /*b*/) { }

  /**
   *  @brief Subcircuits a and b match exactly
   */
  virtual void match_subcircuits (const db::SubCircuit * /*a*/, const db::SubCircuit * /*b*/) { }

  /**
   *  @brief SubCircuit a or b doesn't match
   *  "a" is null if there is no match for b and vice versa.
   */
  virtual void subcircuit_mismatch (const db::SubCircuit * /*a*/, const db::SubCircuit * /*b*/) { }

private:
  //  No copying
  NetlistCompareLogger (const NetlistCompareLogger &);
  NetlistCompareLogger &operator= (const NetlistCompareLogger &);
};

/**
 *  @brief The netlist comparer
 */
class DB_PUBLIC NetlistComparer
{
public:
  /**
   *  @brief Constructor
   */
  NetlistComparer (NetlistCompareLogger *logger = 0);

  /**
   *  @brief Destructor
   */
  ~NetlistComparer ();

  /**
   *  @brief Mark two nets as identical
   *
   *  This makes a net na in netlist a identical to the corresponding
   *  net nb in netlist b.
   *  By default nets are not identical expect through their topology.
   */
  void same_nets (const db::Net *na, const db::Net *nb);

  /**
   *  @brief Mark two pins as equivalent (i.e. can be swapped)
   *
   *  Only circuits from the *second* input can be given swappable pins.
   *  This will imply the same swappable pins on the equivalent circuit of the first input.
   *  To mark multiple pins as swappable, use the version that takes a list of pins.
   */
  void equivalent_pins (const db::Circuit *cb, size_t pin1_id, size_t pin2_id);

  /**
   *  @brief Mark multiple pins as equivalent (i.e. can be swapped)
   *
   *  Only circuits from the *second* input can be given swappable pins.
   *  This will imply the same swappable pins on the equivalent circuit of the first input.
   */
  void equivalent_pins (const db::Circuit *cb, const std::vector<size_t> &pin_ids);

  /**
   *  @brief Mark two device classes as identical
   *
   *  This makes a device class ca in netlist a identical to the corresponding
   *  device class cb in netlist b.
   *  By default device classes with the same name are identical.
   */
  void same_device_classes (const db::DeviceClass *ca, const db::DeviceClass *cb);

  /**
   *  @brief Mark two circuits as identical
   *
   *  This makes a circuit ca in netlist a identical to the corresponding
   *  circuit cb in netlist b.
   *  By default circuits with the same name are identical.
   */
  void same_circuits (const db::Circuit *ca, const db::Circuit *cb);

  /**
   *  @brief Exclude caps with less than the given capacity value
   */
  void exclude_caps (double threshold);

  /**
   *  @brief Exclude resistors with more than the given resistance value
   */
  void exclude_resistors (double threshold);

  /**
   *  @brief Sets the maximum seach depth
   *
   *  This value limits the search depth of the backtracking algorithm to the
   *  given number of jumps.
   */
  void set_max_depth (size_t n)
  {
    m_max_depth = n;
  }

  /**
   *  @brief Gets the maximum search depth
   */
  size_t max_depth () const
  {
    return m_max_depth;
  }

  /**
   *  @brief Sets a value indicating whether not to consider net names
   *  This feature is mainly intended for testing.
   */
  void set_dont_consider_net_names (bool f)
  {
    m_dont_consider_net_names = f;
  }

  /**
   *  @brief Gets a value indicating whether not to consider net names
   */
  bool dont_consider_net_names () const
  {
    return m_dont_consider_net_names;
  }

  /**
   *  @brief Sets the maximum branch complexity
   *
   *  This value limits the maximum branch complexity of the backtracking algorithm.
   *  The complexity is the accumulated number of branch options with ambiguous
   *  net matches. Backtracking will stop when the maximum number of options
   *  has been exceeded.
   *  As the computational complexity is the square of the branch count,
   *  this value should be adjusted carefully.
   */
  void set_max_branch_complexity (size_t n)
  {
    m_max_n_branch = n;
  }

  /**
   *  @brief Gets the maximum branch complexity
   */
  size_t max_branch_complexity () const
  {
    return m_max_n_branch;
  }

  /**
   *  @brief Gets the list of circuits without matching circuit in the other netlist
   *  The result can be used to flatten these circuits prior to compare.
   *  Mismatching top level circuits are not reported because they cannot be flattened.
   */
  void unmatched_circuits (db::Netlist *a, db::Netlist *b, std::vector<db::Circuit *> &in_a, std::vector<db::Circuit *> &in_b) const;

  /**
   *  @brief Actually compares the two netlists
   */
  bool compare (const db::Netlist *a, const db::Netlist *b) const;

  /**
   *  @brief Actually compares the two netlists using the given logger
   */
  bool compare (const db::Netlist *a, const db::Netlist *b, db::NetlistCompareLogger *logger) const;

private:
  //  No copying
  NetlistComparer (const NetlistComparer &);
  NetlistComparer &operator= (const NetlistComparer &);

protected:
  bool compare_circuits (const db::Circuit *c1, const db::Circuit *c2, db::DeviceCategorizer &device_categorizer, db::CircuitCategorizer &circuit_categorizer, db::CircuitPinMapper &circuit_pin_mapper, const std::vector<std::pair<const Net *, const Net *> > &net_identity, bool &pin_mismatch, std::map<const db::Circuit *, CircuitMapper> &c12_circuit_and_pin_mapping, std::map<const db::Circuit *, CircuitMapper> &c22_circuit_and_pin_mapping) const;
  bool all_subcircuits_verified (const db::Circuit *c, const std::set<const db::Circuit *> &verified_circuits) const;
  static void derive_pin_equivalence (const db::Circuit *ca, const db::Circuit *cb, CircuitPinMapper *circuit_pin_mapper);
  void do_pin_assignment (const db::Circuit *c1, const db::NetGraph &g1, const db::Circuit *c2, const db::NetGraph &g2, std::map<const db::Circuit *, CircuitMapper> &c12_circuit_and_pin_mapping, std::map<const db::Circuit *, CircuitMapper> &c22_circuit_and_pin_mapping, bool &pin_mismatch, bool &good) const;
  void do_device_assignment (const db::Circuit *c1, const db::NetGraph &g1, const db::Circuit *c2, const db::NetGraph &g2, const db::DeviceFilter &device_filter, DeviceCategorizer &device_categorizer, bool &good) const;
  void do_subcircuit_assignment (const db::Circuit *c1, const db::NetGraph &g1, const db::Circuit *c2, const db::NetGraph &g2, CircuitCategorizer &circuit_categorizer, const db::CircuitPinMapper &circuit_pin_mapper, std::map<const db::Circuit *, CircuitMapper> &c12_circuit_and_pin_mapping, std::map<const db::Circuit *, CircuitMapper> &c22_circuit_and_pin_mapping, bool &good) const;
  bool handle_pin_mismatch (const NetGraph &g1, const db::Circuit *c1, const db::Pin *pin1, const NetGraph &g2, const db::Circuit *c2, const db::Pin *p2) const;

  mutable NetlistCompareLogger *mp_logger;
  std::map<std::pair<const db::Circuit *, const db::Circuit *>, std::vector<std::pair<const Net *, const Net *> > > m_same_nets;
  std::auto_ptr<CircuitPinMapper> mp_circuit_pin_mapper;
  std::auto_ptr<DeviceCategorizer> mp_device_categorizer;
  std::auto_ptr<CircuitCategorizer> mp_circuit_categorizer;
  double m_cap_threshold;
  double m_res_threshold;
  size_t m_max_n_branch;
  size_t m_max_depth;
  bool m_dont_consider_net_names;
};

}

namespace tl
{

template<> struct type_traits<db::NetlistComparer> : public tl::type_traits<void>
{
  //  mark "NetlistDeviceExtractor" as having a default ctor and no copy ctor
  typedef tl::false_tag has_copy_constructor;
  typedef tl::false_tag has_default_constructor;
};

template<> struct type_traits<db::NetlistCompareLogger> : public tl::type_traits<void>
{
  //  mark "NetlistDeviceExtractor" as having a default ctor and no copy ctor
  typedef tl::false_tag has_copy_constructor;
};

}

#endif

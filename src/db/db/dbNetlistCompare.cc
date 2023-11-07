
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

#include "dbNetlistCompare.h"
#include "dbNetlistCompareUtils.h"
#include "dbNetlistCompareGraph.h"
#include "dbNetlistCompareCore.h"
#include "dbNetlistDeviceClasses.h"
#include "tlProgress.h"
#include "tlTimer.h"
#include "tlEquivalenceClusters.h"
#include "tlLog.h"
#include "tlEnv.h"
#include "tlInternational.h"

#include <cstring>

namespace db
{

// --------------------------------------------------------------------------------------------------------------------
//  NetlistComparer implementation

NetlistComparer::NetlistComparer (NetlistCompareLogger *logger)
  : mp_logger (logger)
{
  mp_device_categorizer.reset (new DeviceCategorizer ());
  mp_circuit_categorizer.reset (new CircuitCategorizer ());
  mp_circuit_pin_categorizer.reset (new CircuitPinCategorizer ());

  m_cap_threshold = -1.0;   //  not set
  m_res_threshold = -1.0;   //  not set

  //  NOTE: as the backtracking algorithm is recursive, we need to limit the number of steps to follow
  //  Long chains can happen in case of depth-first because the backtracking algorithm will follow
  //  each successful path further to the very end. Depending on the circuit's complexity a long chain of
  //  jumps is possible leading to a deep stack. A value of 500 is compatible with 4M stack depth on a
  //  64bit machine which is considered acceptable for now.
  m_max_depth = 500;

  m_max_n_branch = std::numeric_limits<size_t>::max ();
  m_depth_first = true;

  m_dont_consider_net_names = false;
  m_case_sensitive = false;

  m_with_log = true;
}

NetlistComparer::~NetlistComparer ()
{
  //  .. nothing yet ..
}

void
NetlistComparer::exclude_caps (double threshold)
{
  m_cap_threshold = threshold;
}

void
NetlistComparer::exclude_resistors (double threshold)
{
  m_res_threshold = threshold;
}

void
NetlistComparer::same_nets (const db::Net *na, const db::Net *nb, bool must_match)
{
  tl_assert (na && na);
  m_same_nets [std::make_pair (na->circuit (), nb->circuit ())].push_back (std::make_pair (std::make_pair (na, nb), must_match));
}

void
NetlistComparer::same_nets (const db::Circuit *ca, const db::Circuit *cb, const db::Net *na, const db::Net *nb, bool must_match)
{
  m_same_nets [std::make_pair (ca, cb)].push_back (std::make_pair (std::make_pair (na, nb), must_match));
}

void
NetlistComparer::equivalent_pins (const db::Circuit *cb, size_t pin1_id, size_t pin2_id)
{
  mp_circuit_pin_categorizer->map_pins (cb, pin1_id, pin2_id);
}

void
NetlistComparer::equivalent_pins (const db::Circuit *cb, const std::vector<size_t> &pin_ids)
{
  mp_circuit_pin_categorizer->map_pins (cb, pin_ids);
}

void
NetlistComparer::same_device_classes (const db::DeviceClass *ca, const db::DeviceClass *cb)
{
  mp_device_categorizer->same_class (ca, cb);
}

void
NetlistComparer::same_circuits (const db::Circuit *ca, const db::Circuit *cb)
{
  mp_circuit_categorizer->same_circuit (ca, cb);
}

bool
NetlistComparer::compare (const db::Netlist *a, const db::Netlist *b, NetlistCompareLogger *logger) const
{
  db::NetlistCompareLogger *org_logger = mp_logger;
  mp_logger = logger;
  bool res = false;

  try {
    res = compare (a, b);
  } catch (...) {
    mp_logger = org_logger;
    throw;
  }

  mp_logger = org_logger;
  return res;
}

void
NetlistComparer::unmatched_circuits (db::Netlist *a, db::Netlist *b, std::vector<db::Circuit *> &in_a, std::vector<db::Circuit *> &in_b) const
{
  //  we need to create a copy because this method is supposed to be const.
  db::CircuitCategorizer circuit_categorizer = *mp_circuit_categorizer;
  circuit_categorizer.set_case_sensitive (m_case_sensitive);

  std::map<size_t, std::pair<std::vector<db::Circuit *>, std::vector<db::Circuit *> > > cat2circuits;

  for (db::Netlist::circuit_iterator i = a->begin_circuits (); i != a->end_circuits (); ++i) {
    size_t cat = circuit_categorizer.cat_for_circuit (i.operator-> ());
    if (cat) {
      cat2circuits[cat].first.push_back (i.operator-> ());
    }
  }

  for (db::Netlist::circuit_iterator i = b->begin_circuits (); i != b->end_circuits (); ++i) {
    size_t cat = circuit_categorizer.cat_for_circuit (i.operator-> ());
    if (cat) {
      cat2circuits[cat].second.push_back (i.operator-> ());
    }
  }

  size_t na = 0, nb = 0;
  for (std::map<size_t, std::pair<std::vector<db::Circuit *>, std::vector<db::Circuit *> > >::const_iterator i = cat2circuits.begin (); i != cat2circuits.end (); ++i) {
    if (i->second.first.empty ()) {
      nb += i->second.second.size ();
    } else if (i->second.second.empty ()) {
      na += i->second.first.size ();
    }
  }

  in_a.reserve (na);
  in_b.reserve (nb);

  for (std::map<size_t, std::pair<std::vector<db::Circuit *>, std::vector<db::Circuit *> > >::const_iterator i = cat2circuits.begin (); i != cat2circuits.end (); ++i) {
    if (i->second.first.empty ()) {
      in_b.insert (in_b.end (), i->second.second.begin (), i->second.second.end ());
    } else if (i->second.second.empty ()) {
      in_a.insert (in_a.end (), i->second.first.begin (), i->second.first.end ());
    }
  }
}

static void clear_primary_classes (const db::Netlist *nl)
{
  for (db::Netlist::const_device_class_iterator dc = nl->begin_device_classes (); dc != nl->end_device_classes (); ++dc) {
    dc->set_primary_class (0);
  }
}

bool
NetlistComparer::compare (const db::Netlist *a, const db::Netlist *b) const
{
  bool res = false;

  try {
    res = compare_impl (a, b);
    clear_primary_classes (a);
    clear_primary_classes (b);
  } catch (...) {
    clear_primary_classes (a);
    clear_primary_classes (b);
    throw;
  }

  return res;
}

bool
NetlistComparer::compare_impl (const db::Netlist *a, const db::Netlist *b) const
{
  clear_primary_classes (a);
  clear_primary_classes (b);

  m_case_sensitive = combined_case_sensitive (a, b);

  //  we need to create a copy because this method is supposed to be const.
  db::CircuitCategorizer circuit_categorizer = *mp_circuit_categorizer;
  db::DeviceCategorizer device_categorizer = *mp_device_categorizer;
  db::CircuitPinCategorizer circuit_pin_mapper = *mp_circuit_pin_categorizer;

  circuit_categorizer.set_case_sensitive (m_case_sensitive);
  device_categorizer.set_case_sensitive (m_case_sensitive);

  bool good = true;

  std::map<size_t, std::pair<std::vector<const db::Circuit *>, std::vector<const db::Circuit *> > > cat2circuits;
  std::set<const db::Circuit *> verified_circuits_a, verified_circuits_b;

  for (db::Netlist::const_circuit_iterator i = a->begin_circuits (); i != a->end_circuits (); ++i) {
    size_t cat = circuit_categorizer.cat_for_circuit (i.operator-> ());
    if (cat) {
      cat2circuits[cat].first.push_back (i.operator-> ());
    } else {
      //  skip circuit (but count it as verified)
      verified_circuits_a.insert (i.operator-> ());
    }
  }

  for (db::Netlist::const_circuit_iterator i = b->begin_circuits (); i != b->end_circuits (); ++i) {
    size_t cat = circuit_categorizer.cat_for_circuit (i.operator-> ());
    if (cat) {
      cat2circuits[cat].second.push_back (i.operator-> ());
    } else {
      //  skip circuit (but count it as verified)
      verified_circuits_b.insert (i.operator-> ());
    }
  }

  if (mp_logger) {
    mp_logger->begin_netlist (a, b);
  }

  //  check for device classes that don't match

  std::map<size_t, std::pair<const db::DeviceClass *, const db::DeviceClass *> > cat2dc;

  for (db::Netlist::const_device_class_iterator dc = a->begin_device_classes (); dc != a->end_device_classes (); ++dc) {
    size_t cat = device_categorizer.cat_for_device_class (dc.operator-> ());
    if (cat) {
      cat2dc.insert (std::make_pair (cat, std::make_pair ((const db::DeviceClass *) 0, (const db::DeviceClass *) 0))).first->second.first = dc.operator-> ();
    }
  }

  for (db::Netlist::const_device_class_iterator dc = b->begin_device_classes (); dc != b->end_device_classes (); ++dc) {
    size_t cat = device_categorizer.cat_for_device_class (dc.operator-> ());
    if (cat) {
      cat2dc.insert (std::make_pair (cat, std::make_pair ((const db::DeviceClass *) 0, (const db::DeviceClass *) 0))).first->second.second = dc.operator-> ();
    }
  }

  for (std::map<size_t, std::pair<const db::DeviceClass *, const db::DeviceClass *> >::const_iterator i = cat2dc.begin (); i != cat2dc.end (); ++i) {
    if (! i->second.first || ! i->second.second) {
      //  NOTE: device class mismatch does not set good to false.
      //  Reasoning: a device class may not be present because there is no device of a certain kind (e.g. in SPICE).
      //  This isn't necessarily a failure.
      if (mp_logger) {
        mp_logger->device_class_mismatch (i->second.first, i->second.second);
      }
    }
  }

  //  Register the primary netlist's device classes as primary ones for the second netlist's device classes.
  //  This way, the tolerances and parameter definitions are imposed on the second netlist, creating a common basis.
  for (std::map<size_t, std::pair<const db::DeviceClass *, const db::DeviceClass *> >::const_iterator i = cat2dc.begin (); i != cat2dc.end (); ++i) {
    if (i->second.first && i->second.second) {
      i->second.second->set_primary_class (i->second.first);
    }
  }

  //  decide whether to use a device category in strict mode

  device_categorizer.clear_strict_device_categories ();

  for (std::map<size_t, std::pair<const db::DeviceClass *, const db::DeviceClass *> >::const_iterator i = cat2dc.begin (); i != cat2dc.end (); ++i) {
    if (i->second.first && i->second.second && (i->second.first->is_strict () || i->second.second->is_strict ())) {
      device_categorizer.set_strict_device_category (i->first);
    }
  }

  //  check for circuits that don't match

  for (std::map<size_t, std::pair<std::vector<const db::Circuit *>, std::vector<const db::Circuit *> > >::const_iterator i = cat2circuits.begin (); i != cat2circuits.end (); ++i) {
    if (i->second.first.empty ()) {
      good = false;
      if (mp_logger) {
        for (std::vector<const db::Circuit *>::const_iterator j = i->second.second.begin (); j != i->second.second.end (); ++j) {
          mp_logger->circuit_mismatch (0, *j);
        }
      }
    }
    if (i->second.second.empty ()) {
      good = false;
      if (mp_logger) {
        for (std::vector<const db::Circuit *>::const_iterator j = i->second.first.begin (); j != i->second.first.end (); ++j) {
          mp_logger->circuit_mismatch (*j, 0);
        }
      }
    }
  }

  std::map<const db::Circuit *, CircuitMapper> c12_pin_mapping, c22_pin_mapping;

  tl::RelativeProgress progress (tl::to_string (tr ("Comparing netlists")), a->circuit_count (), 1);

  for (db::Netlist::const_bottom_up_circuit_iterator c = a->begin_bottom_up (); c != a->end_bottom_up (); ++c) {

    const db::Circuit *ca = c.operator-> ();

    size_t ccat = circuit_categorizer.cat_for_circuit (ca);
    if (! ccat) {
      continue;
    }

    std::map<size_t, std::pair<std::vector<const db::Circuit *>, std::vector<const db::Circuit *> > >::const_iterator i = cat2circuits.find (ccat);
    tl_assert (i != cat2circuits.end ());
    tl_assert (! i->second.first.empty ());
    if (i->second.second.empty ()) {
      continue;
    }

    //  NOTE: there can only be one schematic circuit
    tl_assert (i->second.second.size () == size_t (1));
    const db::Circuit *cb = i->second.second.front ();

    std::vector<std::pair<std::pair<const Net *, const Net *>, bool> > empty;
    const std::vector<std::pair<std::pair<const Net *, const Net *>, bool> > *net_identity = &empty;
    std::map<std::pair<const db::Circuit *, const db::Circuit *>, std::vector<std::pair<std::pair<const Net *, const Net *>, bool> > >::const_iterator sn = m_same_nets.find (std::make_pair (ca, cb));
    if (sn != m_same_nets.end ()) {
      net_identity = &sn->second;
    }

    if (all_subcircuits_verified (ca, verified_circuits_a) && all_subcircuits_verified (cb, verified_circuits_b)) {

      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
        tl::info << "----------------------------------------------------------------------";
        tl::info << "treating circuit: " << ca->name () << " vs. " << cb->name ();
      }
      if (mp_logger) {
        mp_logger->begin_circuit (ca, cb);
      }

      bool pin_mismatch = false;
      bool g = compare_circuits (ca, cb, device_categorizer, circuit_categorizer, circuit_pin_mapper, *net_identity, pin_mismatch, c12_pin_mapping, c22_pin_mapping);
      if (! g) {
        good = false;
      }

      if (! pin_mismatch) {
        verified_circuits_a.insert (ca);
        verified_circuits_b.insert (cb);
      }

      derive_pin_equivalence (ca, cb, &circuit_pin_mapper);

      if (mp_logger) {
        mp_logger->end_circuit (ca, cb, g);
      }

    } else {

      if (mp_logger) {

        std::string msg = generate_subcircuits_not_verified_warning (ca, verified_circuits_a, cb, verified_circuits_b);

        if (m_with_log) {
          mp_logger->log_entry (db::Error, msg);
        }

        mp_logger->circuit_skipped (ca, cb, msg);
        good = false;

      }

    }

    ++progress;

  }

  if (mp_logger) {
    mp_logger->end_netlist (a, b);
  }

  return good;
}

static
std::vector<size_t> collect_anonymous_empty_pins (const db::Circuit *c, CircuitPinCategorizer *circuit_pin_mapper)
{
  std::vector<size_t> pins;

  for (db::Circuit::const_pin_iterator p = c->begin_pins (); p != c->end_pins (); ++p) {
    if (p->name ().empty () && ! circuit_pin_mapper->is_mapped (c, p->id ())) {
      const db::Net *net = c->net_for_pin (p->id ());
      if (! net || net->is_passive ()) {
        pins.push_back (p->id ());
      }
    }
  }

  return pins;
}

void
NetlistComparer::derive_pin_equivalence (const db::Circuit *ca, const db::Circuit *cb, CircuitPinCategorizer *circuit_pin_mapper)
{
  //  NOTE: All unnamed pins with empty nets are treated as equivalent. There is no other criterion to match these pins.

  std::vector<size_t> pa, pb;
  pa = collect_anonymous_empty_pins (ca, circuit_pin_mapper);
  pb = collect_anonymous_empty_pins (cb, circuit_pin_mapper);

  circuit_pin_mapper->map_pins (ca, pa);
  circuit_pin_mapper->map_pins (cb, pb);
}

static bool is_valid_circuit (const db::Circuit *c)
{
  //  typical via subcircuits attach through one pin. We can safely ignore such subcircuits because they don't
  //  contribute graph edges.
  return c->pin_count () > 1;
}

bool
NetlistComparer::all_subcircuits_verified (const db::Circuit *c, const std::set<const db::Circuit *> &verified_circuits) const
{
  for (db::Circuit::const_subcircuit_iterator sc = c->begin_subcircuits (); sc != c->end_subcircuits (); ++sc) {
    const db::Circuit *cr = sc->circuit_ref ();
    if (is_valid_circuit (cr) && verified_circuits.find (cr) == verified_circuits.end ()) {
      return false;
    }
  }
  return true;
}

static std::vector<std::string> unverified_names (const db::Circuit *c, const std::set<const db::Circuit *> &verified_circuits)
{
  std::vector<std::string> names;

  std::set<const db::Circuit *> seen;
  for (db::Circuit::const_subcircuit_iterator sc = c->begin_subcircuits (); sc != c->end_subcircuits (); ++sc) {
    const db::Circuit *cr = sc->circuit_ref ();
    if (is_valid_circuit (cr) && seen.find (cr) == seen.end () && verified_circuits.find (cr) == verified_circuits.end ()) {
      seen.insert (cr);
      names.push_back (cr->name ());
    }
  }

  std::sort (names.begin (), names.end ());
  return names;
}

std::string
NetlistComparer::generate_subcircuits_not_verified_warning (const db::Circuit *ca, const std::set<const db::Circuit *> &verified_circuits_a, const db::Circuit *cb, const std::set<const db::Circuit *> &verified_circuits_b) const
{
  std::string msg = tl::sprintf (tl::to_string (tr ("Circuits %s and %s could not be compared because the following subcircuits failed to compare:")), ca->name (), cb->name ());

  std::vector<std::string> names_a = unverified_names (ca, verified_circuits_a);
  if (! names_a.empty ()) {
    msg += "\n  A: " + tl::join (names_a, ",");
  }

  std::vector<std::string> names_b = unverified_names (cb, verified_circuits_b);
  if (! names_b.empty ()) {
    msg += "\n  B: " + tl::join (names_b, ",");
  }

  return msg;
}

static size_t translate_terminal_id (size_t tid, const db::Device *device)
{
  return device->device_class () ? device->device_class ()->normalize_terminal_id (tid) : tid;
}

static std::vector<std::pair<size_t, size_t> >
compute_device_key (const db::Device &device, const db::NetGraph &g, bool strict)
{
  std::vector<std::pair<size_t, size_t> > k;

  const std::vector<db::DeviceTerminalDefinition> &td = device.device_class ()->terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
    size_t terminal_id = strict ? t->id () : translate_terminal_id (t->id (), &device);
    const db::Net *net = device.net_for_terminal (t->id ());
    size_t net_id = g.node_index_for_net (net);
    k.push_back (std::make_pair (terminal_id, net_id));
  }

  return k;
}

static std::vector<std::pair<size_t, size_t> >
compute_device_key_for_this (const db::Device &device, const db::NetGraph &g, bool strict, bool &mapped)
{
  std::vector<std::pair<size_t, size_t> > k = compute_device_key (device, g, strict);

  mapped = true;
  for (std::vector<std::pair<size_t, size_t> >::iterator i = k.begin (); i != k.end (); ++i) {
    if (! g.begin () [i->second].has_other ()) {
      i->second = invalid_id;  //  normalization
      mapped = false;
    }
  }

  std::sort (k.begin (), k.end ());
  return k;
}

static std::vector<std::pair<size_t, size_t> >
compute_device_key_for_other (const db::Device &device, const db::NetGraph &g, bool strict, bool &mapped)
{
  std::vector<std::pair<size_t, size_t> > k = compute_device_key (device, g, strict);

  mapped = true;
  for (std::vector<std::pair<size_t, size_t> >::iterator i = k.begin (); i != k.end (); ++i) {
    if (! g.begin () [i->second].has_other ()) {
      i->second = invalid_id;  //  normalization
      mapped = false;
    } else {
      i->second = g.begin () [i->second].other_net_index ();
    }
  }

  std::sort (k.begin (), k.end ());
  return k;
}

static bool
compute_subcircuit_key (const db::SubCircuit &subcircuit, const db::NetGraph &g, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinCategorizer *pin_map, std::vector<std::pair<size_t, size_t> > &k)
{
  const db::Circuit *cr = subcircuit.circuit_ref ();

  std::map<const db::Circuit *, CircuitMapper>::const_iterator icm = circuit_map->find (cr);
  if (icm == circuit_map->end ()) {
    //  this can happen if the other circuit does not exist - report invalid mapping.
    return false;
  }

  const CircuitMapper *cm = & icm->second;
  cr = cm->other ();

  //  NOTE: cr is given in terms of the canonical "other" circuit.

  for (db::Circuit::const_pin_iterator p = cr->begin_pins (); p != cr->end_pins (); ++p) {

    if (cm->has_this_pin_for_other_pin (p->id ())) {

      size_t this_pin_id = cm->this_pin_from_other_pin (p->id ());
      size_t pin_id = pin_map->normalize_pin_id (cr, p->id ());

      const db::Net *net = subcircuit.net_for_pin (this_pin_id);
      size_t net_id = g.node_index_for_net (net);
      k.push_back (std::make_pair (pin_id, net_id));

    }

  }

  return true;
}

static std::vector<std::pair<size_t, size_t> >
compute_subcircuit_key_for_this (const db::SubCircuit &subcircuit, const db::NetGraph &g, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinCategorizer *pin_map, bool &mapped, bool &valid)
{
  valid = true;
  std::vector<std::pair<size_t, size_t> > k;
  if (! compute_subcircuit_key (subcircuit, g, circuit_map, pin_map, k)) {
    valid = false;
  }

  mapped = true;
  for (std::vector<std::pair<size_t, size_t> >::iterator i = k.begin (); i != k.end () && mapped; ++i) {
    if (! g.begin () [i->second].has_other ()) {
      mapped = false;
    }
  }

  std::sort (k.begin (), k.end ());
  return k;
}

static std::vector<std::pair<size_t, size_t> >
compute_subcircuit_key_for_other (const db::SubCircuit &subcircuit, const db::NetGraph &g, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinCategorizer *pin_map, bool &mapped, bool &valid)
{
  valid = true;
  std::vector<std::pair<size_t, size_t> > k;
  if (! compute_subcircuit_key (subcircuit, g, circuit_map, pin_map, k)) {
    valid = false;
  }

  mapped = true;
  for (std::vector<std::pair<size_t, size_t> >::iterator i = k.begin (); i != k.end (); ++i) {
    if (! g.begin () [i->second].has_other ()) {
      mapped = false;
    } else {
      i->second = g.begin () [i->second].other_net_index ();
    }
  }

  std::sort (k.begin (), k.end ());
  return k;
}

namespace {

inline double size_dist (size_t a, size_t b)
{
  double d = a - b;
  return d * d;
}

struct KeyDistance
{
  typedef std::pair<std::vector<std::pair<size_t, size_t> >, const db::SubCircuit *> value_type;

  double operator() (const value_type &a, const value_type &b) const
  {
    tl_assert (a.first.size () == b.first.size ());
    double d = 0.0;
    for (std::vector<std::pair<size_t, size_t> >::const_iterator i = a.first.begin (), j = b.first.begin (); i != a.first.end (); ++i, ++j) {
      d += size_dist (i->first, j->first) + size_dist (i->second, j->second);
    }
    return d;
  }
};

struct KeySize
{
  typedef std::pair<std::vector<std::pair<size_t, size_t> >, const db::SubCircuit *> value_type;

  bool operator() (const value_type &a, const value_type &b) const
  {
    return (a.first.size () < b.first.size ());
  }
};

struct DeviceConnectionDistance
{
  typedef std::pair<std::vector<std::pair<size_t, size_t> >, std::pair<const db::Device *, size_t> > value_type;

  double operator() (const value_type &a, const value_type &b) const
  {
    int d = 0.0;
    for (std::vector<std::pair<size_t, size_t> >::const_iterator i = a.first.begin (), j = b.first.begin (); i != a.first.end () && j != b.first.end (); ++i, ++j) {
      if (i->second != j->second || i->second == invalid_id || j->second == invalid_id) {
        ++d;
      }
    }
    return double (d);
  }
};

struct DeviceParametersCompare
{
  typedef std::pair<std::vector<std::pair<size_t, size_t> >, std::pair<const db::Device *, size_t> > value_type;

  bool operator() (const value_type &a, const value_type &b) const
  {
    //  category and parameters
    return m_dc (a.second, b.second);
  }

  bool equals (const value_type &a, const value_type &b) const
  {
    //  category and parameters
    return m_dc.equals (a.second, b.second);
  }

private:
  db::DeviceCompare m_dc;
};

template <class Iter, class Distance>
void align (Iter i1, Iter i2, Iter j1, Iter j2, Distance distance)
{
  //  TODO: this can probably be done more efficiently

  std::vector<Iter> vi, vj;
  vi.reserve (std::max (i2 - i1, j2 - j1));
  vj.reserve (std::max (i2 - i1, j2 - j1));

  for (Iter i = i1; i != i2; ++i) {
    vi.push_back (i);
  }

  for (Iter j = j1; j != j2; ++j) {
    vj.push_back (j);
  }

  size_t sz = std::max (vi.size (), vj.size ());

  if (sz <= 1) {
    return;
  }

  //  Caution: this is an O(2) algorithm ...
  bool any_swapped = true;
  for (size_t n = 0; n < sz - 1 && any_swapped; ++n) {

    any_swapped = false;

    for (size_t m = n + 1; m < sz; ++m) {
      if (n >= vi.size () || m >= vi.size () || n >= vj.size () || m >= vj.size ()) {
        continue;
      } else if (distance (*vi [n], *vj [m]) + distance (*vi [m], *vj [n]) < distance (*vi [n], *vj [n]) + distance (*vi [m], *vj [m])) {
        //  this will reduce the overall distance:
        std::swap (*vj [n], *vj [m]);
        any_swapped = true;
      }
    }

  }
}

}

/**
 *  @brief Returns true, if the given net is passive
 *  A passive net does not have devices nor pins except those which are ignored (not mapped).
 */
static bool is_passive_net (const db::Net *net, const std::map<const db::Circuit *, CircuitMapper> &circuit_and_pin_mapping)
{
  if (net->terminal_count () != 0) {
    return false;
  }

  if (net->subcircuit_pin_count () == 0) {
    return true;
  }

  for (db::Net::const_subcircuit_pin_iterator p = net->begin_subcircuit_pins (); p != net->end_subcircuit_pins (); ++p) {
    const db::Circuit *c = p->subcircuit ()->circuit_ref ();
    std::map<const db::Circuit *, CircuitMapper>::const_iterator ic;
    ic = circuit_and_pin_mapping.find (c);
    if (ic != circuit_and_pin_mapping.end () && ic->second.has_other_pin_for_this_pin (p->pin_id ())) {
      return false;
    }
  }

  return true;
}

namespace
{

/**
 *  @brief A class for eliminating redundant nodes from the next-generation deduction step
 *
 *  The purpose of this class is to reduce the number of attempts to derive new relationships from
 *  already matched nodes. Such nodes may be redundant - specifically in bus-like configurations
 *  where many nets attach to the same devices or subcircuits, but different terminals or pins.
 *  Such nodes are redundant - they do not contribute new knowledge about additional matches.
 *
 *  This cache is intended to filter these out. It is built from a graph with "fill". "is_redundant" can
 *  be used to check whether any node is redundant to different node (one of them will not be marked
 *  as redundant).
 *
 *  Redundancy is defined in terms of attached subcircuits or devices, not by the type of pin or
 *  terminal which the node's net attaches to.
 */
class RedundantNodeCache
{
public:
  RedundantNodeCache ()
  {
    //  .. nothing yet ..
  }

  void fill (const db::NetGraph &g)
  {
    std::set<std::pair<subcircuit_signature_t, device_signature_t> > signatures;

    for (db::NetGraph::node_iterator n = g.begin (); n != g.end (); ++n) {
      std::pair<subcircuit_signature_t, device_signature_t> key (subcircuit_signature (*n), device_signature (*n));
      if (signatures.find (key) == signatures.end ()) {
        signatures.insert (key);
      } else {
        m_redundant.insert (n->net ());
      }
    }
  }

  bool is_redundant (const db::NetGraphNode &n) const
  {
    return m_redundant.find (n.net ()) != m_redundant.end ();
  }

private:
  typedef std::vector<std::pair<const db::SubCircuit *, size_t> > subcircuit_signature_t;
  typedef std::vector<std::pair<const db::Device *, size_t> > device_signature_t;

  std::set<const db::Net *> m_redundant;

  subcircuit_signature_t subcircuit_signature (const db::NetGraphNode &n) const
  {
    std::map<const db::SubCircuit *, size_t> count;
    for (db::NetGraphNode::edge_iterator e = n.begin (); e != n.end (); ++e) {
      for (std::vector<Transition>::const_iterator t = e->first.begin (); t != e->first.end (); ++t) {
        if (t->is_for_subcircuit ()) {
          count [t->subcircuit ()] += 1;
        }
      }
    }

    return subcircuit_signature_t (count.begin (), count.end ());
  }

  device_signature_t device_signature (const db::NetGraphNode &n) const
  {
    std::map<const db::Device *, size_t> count;
    for (db::NetGraphNode::edge_iterator e = n.begin (); e != n.end (); ++e) {
      for (std::vector<Transition>::const_iterator t = e->first.begin (); t != e->first.end (); ++t) {
        if (! t->is_for_subcircuit ()) {
          count [t->device ()] += 1;
        }
      }
    }

    return device_signature_t (count.begin (), count.end ());
  }
};

}

bool
NetlistComparer::compare_circuits (const db::Circuit *c1, const db::Circuit *c2,
                                   db::DeviceCategorizer &device_categorizer,
                                   db::CircuitCategorizer &circuit_categorizer,
                                   db::CircuitPinCategorizer &circuit_pin_mapper,
                                   const std::vector<std::pair<std::pair<const Net *, const Net *>, bool> > &net_identity,
                                   bool &pin_mismatch,
                                   std::map<const db::Circuit *, CircuitMapper> &c12_circuit_and_pin_mapping,
                                   std::map<const db::Circuit *, CircuitMapper> &c22_circuit_and_pin_mapping) const
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Comparing circuits ")) + c1->name () + "/" + c2->name ());

  db::DeviceFilter device_filter (m_cap_threshold, m_res_threshold);
  SubCircuitEquivalenceTracker subcircuit_equivalence;
  DeviceEquivalenceTracker device_equivalence;

  db::NetGraph g1, g2;

  size_t unique_pin_id = Transition::first_unique_pin_id ();

  //  NOTE: for normalization we map all subcircuits of c1 to c2.
  //  Also, pin swapping will only happen there.
  if (db::NetlistCompareGlobalOptions::options ()->debug_netgraph) {
    tl::info << "Netlist graph:";
  }
  g1.build (c1, device_categorizer, circuit_categorizer, device_filter, &c12_circuit_and_pin_mapping, &circuit_pin_mapper, (size_t *)0);

  if (db::NetlistCompareGlobalOptions::options ()->debug_netgraph) {
    tl::info << "Other netlist graph:";
  }
  //  NOTE: the second netlist graph is the reference (schematic). We treat it a little more carefully by using pins from subcircuits which
  //  lead to passive nets but connect to non-trivial nets on the outside. This is done by specifying a unique_pin_id counter for the last argument.
  g2.build (c2, device_categorizer, circuit_categorizer, device_filter, &c22_circuit_and_pin_mapping, &circuit_pin_mapper, &unique_pin_id);

  //  Match dummy nodes for null nets
  g1.identify (0, 0);
  g2.identify (0, 0);

  for (std::vector<std::pair<std::pair<const Net *, const Net *>, bool> >::const_iterator p = net_identity.begin (); p != net_identity.end (); ++p) {

    //  NOTE: nets may vanish, hence there
    if (g1.has_node_index_for_net (p->first.first) && g2.has_node_index_for_net (p->first.second)) {

      size_t ni1 = g1.node_index_for_net (p->first.first);
      size_t ni2 = g2.node_index_for_net (p->first.second);

      bool exact_match = (g1.node (ni1) == g2.node (ni2));

      g1.identify (ni1, ni2, exact_match);
      g2.identify (ni2, ni1, exact_match);

      //  in must_match mode, check if the nets are identical
      if (mp_logger) {
        if (p->second && ! exact_match) {
          if (m_with_log) {
            mp_logger->log_entry (db::Error,
                                  tl::sprintf (tl::to_string (tr ("Nets %s are paired explicitly, but are not identical topologically")), nets2string (p->first)));
          }
          mp_logger->net_mismatch (p->first.first, p->first.second);
        } else {
          mp_logger->match_nets (p->first.first, p->first.second);
        }
      }

    } else if (p->second && g1.has_node_index_for_net (p->first.first)) {

      if (mp_logger) {
        mp_logger->net_mismatch (p->first.first, 0);
      }

      size_t ni1 = g1.node_index_for_net (p->first.first);
      g1.identify (ni1, 0);

    } else if (p->second && g2.has_node_index_for_net (p->first.second)) {

      if (mp_logger) {
        mp_logger->net_mismatch (0, p->first.second);
      }

      size_t ni2 = g2.node_index_for_net (p->first.second);
      g2.identify (ni2, 0);

    }

  }

  int iter = 0;

  tl::RelativeProgress progress (tl::to_string (tr ("Comparing circuits ")) + c1->name () + "/" + c2->name (), std::max (g1.end () - g1.begin (), g2.end () - g2.begin ()), 1);

  bool good = false;

  //  check if there is anything to do

  bool has_any_unresolved = false;
  for (db::NetGraph::node_iterator i1 = g1.begin (); i1 != g1.end () && ! has_any_unresolved; ++i1) {
    has_any_unresolved = (! i1->has_other () && i1->net ());
  }
  for (db::NetGraph::node_iterator i2 = g2.begin (); i2 != g2.end () && ! has_any_unresolved; ++i2) {
    has_any_unresolved = (! i2->has_other () && i2->net ());
  }

  if (! has_any_unresolved) {
    good = true;
  }

  //  build the redundant node cache

  RedundantNodeCache redundant_nodes;
  redundant_nodes.fill (g1);

  //  Three passes: one without ambiguities, the second one with ambiguities and names (optional) and a third with ambiguities with topology

  int num_passes = 3;
  for (int pass = 0; pass < num_passes && ! good; ++pass) {

    if (pass == 1 && m_dont_consider_net_names) {
      //  skip the named pass in "don't consider net names" mode
      continue;
    }

    if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
      if (pass > 0) {
        if (pass == 1) {
          tl::info << "including ambiguous nodes now (with names)";
        } else {
          tl::info << "including ambiguous nodes now (ignoreing names)";
        }
      }
    }

    NetlistCompareCore compare (&g1, &g2);
    compare.max_depth = m_max_depth;
    compare.max_n_branch = m_max_n_branch;
    compare.depth_first = m_depth_first;
    compare.dont_consider_net_names = (pass > 1);
    compare.with_ambiguous = (pass > 0);
    compare.circuit_pin_mapper = &circuit_pin_mapper;
    compare.subcircuit_equivalence = &subcircuit_equivalence;
    compare.device_equivalence = &device_equivalence;
    compare.logger = mp_logger;
    compare.with_log = m_with_log;
    compare.progress = &progress;

    std::vector<NodeEdgePair> nodes, other_nodes;

    good = true;
    while (true) {

      ++iter;
      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
        tl::info << "new compare iteration #" << iter;
        tl::info << "deducing from present nodes ...";
      }

      //  deduce new identities from known nodes if possible

      size_t new_identities = 0;

      for (db::NetGraph::node_iterator i1 = g1.begin (); i1 != g1.end (); ++i1) {

        //  note: ambiguous nodes don't contribute additional paths, so skip them
        if (i1->has_other () && i1->net () && i1->other_net_index () > 0 && i1->exact_match ()) {

          if (! redundant_nodes.is_redundant (*i1)) {

            size_t ni = compare.derive_node_identities (i1 - g1.begin ());
            if (ni > 0 && ni != failed_match) {
              new_identities += ni;
              if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
                tl::info << ni << " new identities.";
              }
            }

          } else if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
            tl::info << "skipped redundant node: " << i1->net()->expanded_name ();
          }

        }

      }

      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
        tl::info << "checking topological identity ...";
      }

      //  derive new identities through topology: first collect all nets with the same topological signature

      nodes.clear ();
      other_nodes.clear ();

      std::vector<NetGraphNode::edge_type> no_edges;
      no_edges.push_back (NetGraphNode::edge_type ());

      nodes.reserve (g1.end () - g1.begin ());
      for (db::NetGraph::node_iterator i1 = g1.begin (); i1 != g1.end (); ++i1) {
        if (! i1->has_other () && i1->net ()) {
          nodes.push_back (NodeEdgePair (i1.operator-> (), no_edges.begin ()));
        }
      }

      other_nodes.reserve (g2.end () - g2.begin ());
      for (db::NetGraph::node_iterator i2 = g2.begin (); i2 != g2.end (); ++i2) {
        if (! i2->has_other () && i2->net ()) {
          other_nodes.push_back (NodeEdgePair (i2.operator-> (), no_edges.begin ()));
        }
      }

      if (nodes.empty () || other_nodes.empty ()) {
        //  active mismatched nodes give an error
        for (std::vector<NodeEdgePair>::const_iterator n = nodes.begin (); n != nodes.end () && good; ++n) {
          good = is_passive_net (n->node->net (), c12_circuit_and_pin_mapping);
        }
        for (std::vector<NodeEdgePair  >::const_iterator n = other_nodes.begin (); n != other_nodes.end () && good; ++n) {
          good = is_passive_net (n->node->net (), c22_circuit_and_pin_mapping);
        }
        if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
          tl::info << "Stopped with " << nodes.size () << "/" << other_nodes.size () << " nodes left unresolved " << (good ? "(accepted)" : "(not accepted)");
        }
        //  this assumes that we don't gain anything here. Stop now.
        break;
      }

      std::sort (nodes.begin (), nodes.end (), CompareNodeEdgePair ());
      std::sort (other_nodes.begin (), other_nodes.end (), CompareNodeEdgePair ());

      size_t ni = compare.derive_node_identities_from_node_set (nodes, other_nodes);
      if (ni > 0 && ni != failed_match) {
        new_identities += ni;
        if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
          tl::info << ni << " new identities.";
        }
      }

      if (new_identities == 0) {
        if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
          tl::info << "Stopped with " << nodes.size () << "/" << other_nodes.size () << " nodes left unresolved.";
        }
        good = false;
        break;
      }

    }

    if (pass + 1 == num_passes && ! good && mp_logger && m_with_log) {
      compare.analyze_failed_matches ();
    }

  }

  //  Report missing net assignment

  for (db::NetGraph::node_iterator i = g1.begin (); i != g1.end (); ++i) {
    if (! i->has_other ()) {
      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare || tl::verbosity () >= 40) {
        tl::info << "Unresolved net from left: " << i->net ()->expanded_name () << " " << (good ? "(accepted)" : "(not accepted)");
      }
      if (mp_logger) {
        if (good) {
          mp_logger->match_nets (i->net (), 0);
        } else {
          mp_logger->net_mismatch (i->net (), 0);
        }
      }
      if (good) {
        //  in the "good" case, match the nets against 0
        g1.identify (g1.node_index_for_net (i->net ()), 0);
      }
    }
  }

  for (db::NetGraph::node_iterator i = g2.begin (); i != g2.end (); ++i) {
    if (! i->has_other ()) {
      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
        tl::info << "Unresolved net from right: " << i->net ()->expanded_name () << " " << (good ? "(accepted)" : "(not accepted)");
      }
      if (mp_logger) {
        if (good) {
          mp_logger->match_nets (0, i->net ());
        } else {
          mp_logger->net_mismatch (0, i->net ());
        }
      }
      if (good) {
        //  in the "good" case, match the nets against 0
        g2.identify (g2.node_index_for_net (i->net ()), 0);
      }
    }
  }

  do_pin_assignment (c1, g1, c2, g2, c12_circuit_and_pin_mapping, c22_circuit_and_pin_mapping, pin_mismatch, good);
  do_device_assignment (c1, g1, c2, g2, device_filter, device_categorizer, device_equivalence, good);
  do_subcircuit_assignment (c1, g1, c2, g2, circuit_categorizer, circuit_pin_mapper, c12_circuit_and_pin_mapping, c22_circuit_and_pin_mapping, subcircuit_equivalence, good);

  return good;
}

static void
analyze_pin_mismatch (const db::Pin *pin1, const db::Circuit *c1, const db::Pin *pin2, const db::Circuit * /*c2*/, db::NetlistCompareLogger *logger)
{
  if (! pin1) {
    logger->log_entry (db::Error, tl::sprintf (tl::to_string (tr ("No equivalent pin %s from reference netlist found in netlist.\nThis is an indication that a physical connection is not made to the subcircuit.")), pin2->expanded_name ()));
  }

  if (! pin2) {

    logger->log_entry (db::Error, tl::sprintf (tl::to_string (tr ("No equivalent pin %s from netlist found in reference netlist.\nThis is an indication that additional physical connections are made to the subcircuit cell.")), pin1->expanded_name ()));

    //  attempt to identify pins which are creating invalid connections
    for (auto p = c1->begin_parents (); p != c1->end_parents (); ++p) {
      for (auto c = p->begin_subcircuits (); c != p->end_subcircuits (); ++c) {
        const db::SubCircuit &sc = *c;
        if (sc.circuit_ref () == c1) {
          const db::Net *net = sc.net_for_pin (pin1->id ());
          if (net && (net->subcircuit_pin_count () > 1 || net->terminal_count () > 0 || net->pin_count () > 0)) {
            logger->log_entry (db::Info, tl::sprintf (tl::to_string (tr ("Potential invalid connection in circuit %s, subcircuit cell reference at %s")), p->name (), sc.trans ().to_string ()));
          }
        }
      }
    }

  }
}

bool
NetlistComparer::handle_pin_mismatch (const db::NetGraph &g1, const db::Circuit *c1, const db::Pin *pin1, const db::NetGraph &g2, const db::Circuit *c2, const db::Pin *pin2) const
{
  const db::Circuit *c = pin1 ? c1 : c2;
  const db::Pin *pin = pin1 ? pin1 : pin2;
  const db::NetGraph *graph = pin1 ? &g1 : &g2;
  const db::Net *net = c->net_for_pin (pin->id ());

  //  Nets which are paired with "null" are "safely to be ignored" and
  //  pin matching against "null" is valid.
  if (net) {
    const db::NetGraphNode &n = graph->node (graph->node_index_for_net (net));
    if (n.has_other () && n.other_net_index () == 0) {
      if (mp_logger) {
        mp_logger->match_pins (pin1, pin2);
      }
      return true;
    }
  }

  //  Determine whether the pin in question is used - only in this case we will report an error.
  //  Otherwise, the report will be "match" against 0.
  //  "used" follows a heuristic criterion derived from the subcircuits which make use of this circuit:
  //  if one of these connects the pin to a net with either connections upwards, other subcircuits or
  //  devices, the pin is regarded "used".
  //  TODO: it would be better probably to have a global concept of "used pins" which considers all
  //  devices and propagates their presence as "used" property upwards, then downwards to the subcircuit
  //  pins.

  bool is_not_connected = true;
  for (db::Circuit::const_refs_iterator r = c->begin_refs (); r != c->end_refs () && is_not_connected; ++r) {
    const db::SubCircuit *sc = r.operator-> ();
    const db::Net *net = sc->net_for_pin (pin->id ());
    if (net && ((net->terminal_count () + net->pin_count ()) > 0 || net->subcircuit_pin_count () > 1)) {
      is_not_connected = false;
    }
  }

  if (is_not_connected) {
    if (mp_logger) {
      mp_logger->match_pins (pin1, pin2);
    }
    return true;
  } else {

    if (mp_logger) {
      if (m_with_log) {
        analyze_pin_mismatch (pin1, c1, pin2, c2, mp_logger);
      }
      mp_logger->pin_mismatch (pin1, pin2);
    }
    return false;
  }
}

void
NetlistComparer::do_pin_assignment (const db::Circuit *c1, const db::NetGraph &g1, const db::Circuit *c2, const db::NetGraph &g2, std::map<const db::Circuit *, CircuitMapper> &c12_circuit_and_pin_mapping, std::map<const db::Circuit *, CircuitMapper> &c22_circuit_and_pin_mapping, bool &pin_mismatch, bool &good) const
{
  //  Report pin assignment
  //  This step also does the pin identity mapping.

  //  try to assign abstract pins by name with higher prio
  //  NOTE: An "abstract" pin is a concept that is used in the context of circuit abstracts where the circuits is
  //  intentionally emptied. An "abstract pin" is not just a floating pin. An abstract does not have a net while
  //  a floating pin has a net, but this net is passive - i.e. not attached to any device.
  std::map<std::string, std::pair<const db::Pin *, const db::Pin *> > abstract_pins_by_name;

  for (db::Circuit::const_pin_iterator p = c2->begin_pins (); p != c2->end_pins (); ++p) {
    const db::Net *net = c2->net_for_pin (p->id ());
    if (!net && !p->name ().empty ()) {
      abstract_pins_by_name.insert (std::make_pair (db::Netlist::normalize_name (m_case_sensitive, p->name ()), std::make_pair ((const db::Pin *) 0, (const db::Pin *) 0))).first->second.second = p.operator-> ();
    }
  }

  for (db::Circuit::const_pin_iterator p = c1->begin_pins (); p != c1->end_pins (); ++p) {
    const db::Net *net = c1->net_for_pin (p->id ());
    if (!net && !p->name ().empty ()) {
      abstract_pins_by_name.insert (std::make_pair (db::Netlist::normalize_name (m_case_sensitive, p->name ()), std::make_pair ((const db::Pin *) 0, (const db::Pin *) 0))).first->second.first = p.operator-> ();
    }
  }

  std::map<const db::Pin *, const db::Pin *> abstract_pin_name_mapping;
  for (std::map<std::string, std::pair<const db::Pin *, const db::Pin *> >::const_iterator i = abstract_pins_by_name.begin (); i != abstract_pins_by_name.end (); ++i) {
    if (i->second.first && i->second.second) {
      abstract_pin_name_mapping [i->second.first] = i->second.second;
      abstract_pin_name_mapping [i->second.second] = i->second.first;
    }
  }

  std::vector<const db::Pin *> abstract_pins2;
  std::multimap<size_t, const db::Pin *> net2pin2;
  for (db::Circuit::const_pin_iterator p = c2->begin_pins (); p != c2->end_pins (); ++p) {
    const db::Net *net = c2->net_for_pin (p->id ());
    if (net) {
      net2pin2.insert (std::make_pair (g2.node_index_for_net (net), p.operator-> ()));
    } else if (abstract_pin_name_mapping.find (p.operator-> ()) == abstract_pin_name_mapping.end ()) {
      abstract_pins2.push_back (p.operator-> ());
    }
  }

  //  collect missing assignment for circuit 1
  std::multimap<size_t, const db::Pin *> net2pin1;

  std::vector<const db::Pin *>::iterator next_abstract = abstract_pins2.begin ();

  CircuitMapper &c12_pin_mapping = c12_circuit_and_pin_mapping [c1];
  c12_pin_mapping.set_other (c2);

  //  dummy mapping: we show this circuit is used.
  CircuitMapper &c22_pin_mapping = c22_circuit_and_pin_mapping [c2];
  c22_pin_mapping.set_other (c2);

  for (db::Circuit::const_pin_iterator p = c1->begin_pins (); p != c1->end_pins (); ++p) {

    const db::Net *net = c1->net_for_pin (p->id ());
    if (! net) {

      std::map<const db::Pin *, const db::Pin *>::const_iterator fp = abstract_pin_name_mapping.find (p.operator-> ());
      if (fp != abstract_pin_name_mapping.end ()) {

        //  assign an abstract pin - this is a dummy assignment which is mitigated
        //  by declaring the pins equivalent in derive_pin_equivalence
        if (mp_logger) {
          mp_logger->match_pins (p.operator-> (), fp->second);
        }
        c12_pin_mapping.map_pin (p->id (), fp->second->id ());
        c22_pin_mapping.map_pin (fp->second->id (), fp->second->id ());

      } else if (next_abstract != abstract_pins2.end ()) {

        //  assign an abstract pin - this is a dummy assignment which is mitigated
        //  by declaring the pins equivalent in derive_pin_equivalence
        if (mp_logger) {
          mp_logger->match_pins (p.operator-> (), *next_abstract);
        }
        c12_pin_mapping.map_pin (p->id (), (*next_abstract)->id ());
        c22_pin_mapping.map_pin ((*next_abstract)->id (), (*next_abstract)->id ());

        ++next_abstract;

      } else {

        //  otherwise this is an error for subcircuits or worth a report for top-level circuits
        if (! handle_pin_mismatch (g1, c1, p.operator-> (), g2, c2, 0)) {
          good = false;
          pin_mismatch = true;
        }

      }

      continue;

    }

    const db::NetGraphNode &n = *(g1.begin () + g1.node_index_for_net (net));

    if (! n.has_other ()) {
      //  remember and handle later when we know which pins are not mapped
      net2pin1.insert (std::make_pair (g1.node_index_for_net (net), p.operator-> ()));
      continue;
    }

    std::multimap<size_t, const db::Pin *>::iterator np = net2pin2.find (n.other_net_index ());
    for (db::Net::const_pin_iterator pi = net->begin_pins (); pi != net->end_pins (); ++pi) {

      if (np != net2pin2.end () && np->first == n.other_net_index ()) {

        if (mp_logger) {
          mp_logger->match_pins (pi->pin (), np->second);
        }
        c12_pin_mapping.map_pin (pi->pin ()->id (), np->second->id ());
        //  dummy mapping: we show this pin is used.
        c22_pin_mapping.map_pin (np->second->id (), np->second->id ());

        std::multimap<size_t, const db::Pin *>::iterator np_delete = np;
        ++np;
        net2pin2.erase (np_delete);

      } else {

        //  remember and handle later when we know which pins are not mapped
        net2pin1.insert (std::make_pair (g1.node_index_for_net (net), p.operator-> ()));

      }

    }

  }

  for (std::multimap<size_t, const db::Pin *>::iterator np = net2pin1.begin (); np != net2pin1.end (); ++np) {
    if (! handle_pin_mismatch (g1, c1, np->second, g2, c2, 0)) {
      good = false;
      pin_mismatch = true;
    }
  }

  for (std::multimap<size_t, const db::Pin *>::iterator np = net2pin2.begin (); np != net2pin2.end (); ++np) {
    if (! handle_pin_mismatch (g1, c1, 0, g2, c2, np->second)) {
      good = false;
      pin_mismatch = true;
    }
  }

  //  abstract pins must match.
  while (next_abstract != abstract_pins2.end ()) {
    if (! handle_pin_mismatch (g1, c1, 0, g2, c2, *next_abstract)) {
      good = false;
      pin_mismatch = true;
    }
    ++next_abstract;
  }
}

void
NetlistComparer::do_device_assignment (const db::Circuit *c1, const db::NetGraph &g1, const db::Circuit *c2, const db::NetGraph &g2, const db::DeviceFilter &device_filter, db::DeviceCategorizer &device_categorizer, DeviceEquivalenceTracker &device_eq, bool &good) const
{
  //  Report device assignment

  std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::Device *, size_t> > device_map;

  typedef std::vector<std::pair<std::vector<std::pair<size_t, size_t> >, std::pair<const db::Device *, size_t> > > unmatched_list;
  unmatched_list unmatched_a, unmatched_b;

  //  check mapping of devices whose equivalence is established topologically

  for (db::Circuit::const_device_iterator d = c1->begin_devices (); d != c1->end_devices (); ++d) {

    if (! device_filter.filter (d.operator-> ())) {
      continue;
    }

    if (device_eq.other (d.operator-> ())) {
      continue;
    }

    size_t device_cat = device_categorizer.cat_for_device (d.operator-> ());
    if (! device_cat) {
      //  device is ignored
      continue;
    }

    bool mapped = true;
    std::vector<std::pair<size_t, size_t> > k = compute_device_key_for_this (*d, g1, device_categorizer.is_strict_device_category (device_cat), mapped);

    if (! mapped) {
      if (mp_logger) {
        unmatched_a.push_back (std::make_pair (k, std::make_pair (d.operator-> (), device_cat)));
      }
      good = false;
    } else {
      //  TODO: report devices which cannot be distinguished topologically?
      device_map.insert (std::make_pair (k, std::make_pair (d.operator-> (), device_cat)));
    }

  }

  for (db::Circuit::const_device_iterator d = c2->begin_devices (); d != c2->end_devices (); ++d) {

    if (! device_filter.filter (d.operator-> ())) {
      continue;
    }

    size_t device_cat = device_categorizer.cat_for_device (d.operator-> ());
    if (! device_cat) {
      //  device is ignored
      continue;
    }

    const db::Device *c1_device = 0;
    size_t c1_device_cat = 0;

    const db::Device *d_this = device_eq.other (d.operator-> ());
    if (d_this) {

      size_t device_cat_this = device_categorizer.cat_for_device (d_this);
      if (! device_cat_this) {
        //  device is ignored
        continue;
      }

      bool mapped1 = true, mapped2 = true;
      std::vector<std::pair<size_t, size_t> > k_this = compute_device_key_for_this (*d_this, g1, device_categorizer.is_strict_device_category (device_cat_this), mapped1);
      std::vector<std::pair<size_t, size_t> > k = compute_device_key_for_other (*d, g2, device_categorizer.is_strict_device_category (device_cat), mapped2);

      if (! mapped1 || ! mapped2 || k != k_this) {

        //  topological mismatch
        if (mp_logger) {
          mp_logger->device_mismatch (d_this, d.operator-> ());
        }
        good = false;

      } else {

        c1_device = d_this;
        c1_device_cat = device_cat_this;

      }

    } else {

      bool mapped = true;
      std::vector<std::pair<size_t, size_t> > k = compute_device_key_for_other (*d, g2, device_categorizer.is_strict_device_category (device_cat), mapped);

      auto dm = device_map.find (k);

      if (! mapped || dm == device_map.end () || dm->first != k) {

        if (mp_logger) {
          unmatched_b.push_back (std::make_pair (k, std::make_pair (d.operator-> (), device_cat)));
        }
        good = false;

      } else {

        auto dmm = dm;
        ++dmm;
        size_t n = 1;
        while (dmm != device_map.end () && dmm->first == k) {
          ++dmm;
          ++n;
        }

        if (n > 1) {

          //  device ambiguities may arise from different devices being connected in parallel:
          //  try to identify the device which matches

          db::DeviceCompare dc;

          for (auto i = dm; i != dmm; ++i) {
            if (dc.equals (std::make_pair (i->second.first, i->second.second), std::make_pair (d.operator-> (), device_cat))) {
              dm = i;
              break;
            }
          }

        }

        c1_device = dm->second.first;
        c1_device_cat = dm->second.second;

        device_map.erase (dm);

      }

    }

    if (c1_device) {

      db::DeviceCompare dc;

      if (! dc.equals (std::make_pair (c1_device, c1_device_cat), std::make_pair (d.operator-> (), device_cat))) {
        if (c1_device_cat != device_cat) {
          if (mp_logger) {
            mp_logger->match_devices_with_different_device_classes (c1_device, d.operator-> ());
          }
          good = false;
        } else {
          if (mp_logger) {
            mp_logger->match_devices_with_different_parameters (c1_device, d.operator-> ());
          }
          good = false;
        }
      } else {
        if (mp_logger) {
          mp_logger->match_devices (c1_device, d.operator-> ());
        }
      }

    }

  }

  for (std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::Device *, size_t> >::const_iterator dm = device_map.begin (); dm != device_map.end (); ++dm) {
    if (mp_logger) {
      unmatched_a.push_back (*dm);
    }
    good = false;
  }

  //  try to do some better mapping of unmatched devices - they will still be reported as mismatching, but their pairing gives some hint
  //  what to fix.

  if (mp_logger) {

    size_t max_analysis_set = 1000;
    if (unmatched_a.size () + unmatched_b.size () > max_analysis_set) {

      //  don't try too much analysis - this may be a waste of time
      for (unmatched_list::const_iterator i = unmatched_a.begin (); i != unmatched_a.end (); ++i) {
        mp_logger->device_mismatch (i->second.first, 0);
      }
      for (unmatched_list::const_iterator i = unmatched_b.begin (); i != unmatched_b.end (); ++i) {
        mp_logger->device_mismatch (0, i->second.first);
      }

    } else {

      DeviceParametersCompare cmp;

      std::sort (unmatched_a.begin (), unmatched_a.end (), cmp);
      std::sort (unmatched_b.begin (), unmatched_b.end (), cmp);

      for (unmatched_list::iterator i = unmatched_a.begin (), j = unmatched_b.begin (); i != unmatched_a.end () || j != unmatched_b.end (); ) {

        while (j != unmatched_b.end () && (i == unmatched_a.end () || !cmp.equals (*j, *i))) {
          mp_logger->device_mismatch (0, j->second.first);
          ++j;
        }

        while (i != unmatched_a.end () && (j == unmatched_b.end () || !cmp.equals (*i, *j))) {
          mp_logger->device_mismatch (i->second.first, 0);
          ++i;
        }

        if (i == unmatched_a.end () || j == unmatched_b.end ()) {
          break;
        }

        unmatched_list::iterator ii = i, jj = j;
        ++i, ++j;

        while (i != unmatched_a.end () && cmp.equals (*i, *ii)) {
          ++i;
        }

        while (j != unmatched_b.end () && cmp.equals (*j, *jj)) {
          ++j;
        }

        align (ii, i, jj, j, DeviceConnectionDistance ());

        for ( ; ii != i && jj != j; ++ii, ++jj) {
          mp_logger->device_mismatch (ii->second.first, jj->second.first);
        }

        for ( ; jj != j; ++jj) {
          mp_logger->device_mismatch (0, jj->second.first);
        }

        for ( ; ii != i; ++ii) {
          mp_logger->device_mismatch (ii->second.first, 0);
        }

      }

    }

  }
}

void
NetlistComparer::do_subcircuit_assignment (const db::Circuit *c1, const db::NetGraph &g1, const db::Circuit *c2, const db::NetGraph &g2, CircuitCategorizer &circuit_categorizer, const CircuitPinCategorizer &circuit_pin_mapper, std::map<const Circuit *, CircuitMapper> &c12_circuit_and_pin_mapping, std::map<const Circuit *, CircuitMapper> &c22_circuit_and_pin_mapping, SubCircuitEquivalenceTracker &subcircuit_eq, bool &good) const
{
  //  Report subcircuit assignment

  std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::SubCircuit *, size_t> > subcircuit_map;

  for (db::Circuit::const_subcircuit_iterator sc = c1->begin_subcircuits (); sc != c1->end_subcircuits (); ++sc) {

    size_t sc_cat = circuit_categorizer.cat_for_subcircuit (sc.operator-> ());
    if (! sc_cat) {
      //  subcircuit is ignored
      continue;
    }

    if (subcircuit_eq.other (sc.operator-> ())) {
      continue;
    }

    bool mapped = true, valid = true;
    std::vector<std::pair<size_t, size_t> > k = compute_subcircuit_key_for_this (*sc, g1, &c12_circuit_and_pin_mapping, &circuit_pin_mapper, mapped, valid);

    if (! mapped) {
      if (mp_logger) {
        mp_logger->subcircuit_mismatch (sc.operator-> (), 0);
      }
      good = false;
    } else if (valid) {
      //  TODO: report devices which cannot be distinguished topologically?
      subcircuit_map.insert (std::make_pair (k, std::make_pair (sc.operator-> (), sc_cat)));
    } else {
      //  emit a mismatch event but do not consider that an error - this may happen if the circuit has been dropped intentionally (e.g. via cells)
      if (mp_logger) {
        mp_logger->subcircuit_mismatch (sc.operator-> (), 0);
      }
    }

  }

  typedef std::vector<std::pair<std::vector<std::pair<size_t, size_t> >, const db::SubCircuit *> > unmatched_list;
  unmatched_list unmatched_a, unmatched_b;

  for (db::Circuit::const_subcircuit_iterator sc = c2->begin_subcircuits (); sc != c2->end_subcircuits (); ++sc) {

    size_t sc_cat = circuit_categorizer.cat_for_subcircuit (sc.operator-> ());
    if (! sc_cat) {
      //  subcircuit is ignored
      continue;
    }

    const db::SubCircuit *sc_this = subcircuit_eq.other (sc.operator-> ());
    if (sc_this) {

      size_t sc_cat_this = circuit_categorizer.cat_for_subcircuit (sc_this);
      if (! sc_cat_this) {
        //  subcircuit is ignored
        continue;
      }

      bool mapped1 = true, mapped2 = true;
      bool valid1 = true, valid2 = true;
      std::vector<std::pair<size_t, size_t> > k_this = compute_subcircuit_key_for_this (*sc_this, g1, &c12_circuit_and_pin_mapping, &circuit_pin_mapper, mapped1, valid1);
      std::vector<std::pair<size_t, size_t> > k = compute_subcircuit_key_for_other (*sc, g2, &c22_circuit_and_pin_mapping, &circuit_pin_mapper, mapped2, valid2);

      if (! valid1 || ! valid2 || ! mapped1 || ! mapped2 || k_this != k || sc_cat != sc_cat_this) {
        if (mp_logger) {
          mp_logger->subcircuit_mismatch (sc_this, sc.operator-> ());
        }
        good = false;
      } else {
        if (mp_logger) {
          mp_logger->match_subcircuits (sc_this, sc.operator-> ());
        }
      }

    } else {

      bool mapped = true, valid = true;
      std::vector<std::pair<size_t, size_t> > k = compute_subcircuit_key_for_other (*sc, g2, &c22_circuit_and_pin_mapping, &circuit_pin_mapper, mapped, valid);

      std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::SubCircuit *, size_t> >::iterator scm = subcircuit_map.find (k);

      if (! mapped || scm == subcircuit_map.end () || scm->first != k) {

        if (mp_logger) {
          unmatched_b.push_back (std::make_pair (k, sc.operator-> ()));
        }
        good = false;

      } else {

        db::SubCircuitCompare scc;

        std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::SubCircuit *, size_t> >::iterator scm_start = scm;

        bool found = false;
        size_t nscm = 0;
        while (! found && scm != subcircuit_map.end () && scm->first == k) {
          ++nscm;
          if (scc.equals (scm->second, std::make_pair (sc.operator-> (), sc_cat))) {
            found = true;
          } else {
            ++scm;
          }
        }

        if (! found) {

          if (nscm == 1) {

            //  unique match, but doesn't fit: report this one as paired, but mismatching:
            if (mp_logger) {
              mp_logger->subcircuit_mismatch (scm_start->second.first, sc.operator-> ());
            }

            //  no longer look for this one
            subcircuit_map.erase (scm_start);

          } else {

            //  no unique match
            if (mp_logger) {
              mp_logger->subcircuit_mismatch (0, sc.operator-> ());
            }

          }

          good = false;

        } else {

          if (mp_logger) {
            mp_logger->match_subcircuits (scm->second.first, sc.operator-> ());
          }

          //  no longer look for this one
          subcircuit_map.erase (scm);

        }

      }

    }

  }

  for (std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::SubCircuit *, size_t> >::const_iterator scm = subcircuit_map.begin (); scm != subcircuit_map.end (); ++scm) {
    if (mp_logger) {
      unmatched_a.push_back (std::make_pair (scm->first, scm->second.first));
    }
    good = false;
  }

  //  try to do some pairing between the mismatching subcircuits - even though we will still report them as
  //  mismatches it will give some better hint about what needs to be fixed

  if (mp_logger) {

    size_t max_analysis_set = 1000;
    if (unmatched_a.size () + unmatched_b.size () > max_analysis_set) {

      //  don't try too much analysis - this may be a waste of time
      for (unmatched_list::const_iterator i = unmatched_a.begin (); i != unmatched_a.end (); ++i) {
        mp_logger->subcircuit_mismatch (i->second, 0);
      }
      for (unmatched_list::const_iterator i = unmatched_b.begin (); i != unmatched_b.end (); ++i) {
        mp_logger->subcircuit_mismatch (0, i->second);
      }

    } else {

      std::sort (unmatched_a.begin (), unmatched_a.end (), KeySize ());
      std::sort (unmatched_b.begin (), unmatched_b.end (), KeySize ());

      for (unmatched_list::iterator i = unmatched_a.begin (), j = unmatched_b.begin (); i != unmatched_a.end () || j != unmatched_b.end (); ) {

        while (j != unmatched_b.end () && (i == unmatched_a.end () || j->first.size () < i->first.size ())) {
          mp_logger->subcircuit_mismatch (0, j->second);
          ++j;
        }

        while (i != unmatched_a.end () && (j == unmatched_b.end () || i->first.size () < j->first.size ())) {
          mp_logger->subcircuit_mismatch (i->second, 0);
          ++i;
        }

        if (i == unmatched_a.end () || j == unmatched_b.end ()) {
          break;
        }

        unmatched_list::iterator ii = i, jj = j;
        ++i, ++j;
        size_t n = ii->first.size ();
        tl_assert (n == jj->first.size ());

        while (i != unmatched_a.end () && i->first.size () == n) {
          ++i;
        }

        while (j != unmatched_b.end () && j->first.size () == n) {
          ++j;
        }

        align (ii, i, jj, j, KeyDistance ());

        for ( ; ii != i && jj != j; ++ii, ++jj) {
          mp_logger->subcircuit_mismatch (ii->second, jj->second);
        }

        for ( ; jj != j; ++jj) {
          mp_logger->subcircuit_mismatch (0, jj->second);
        }

        for ( ; ii != i; ++ii) {
          mp_logger->subcircuit_mismatch (ii->second, 0);
        }

      }

    }

  }
}

static bool derive_symmetry_groups (const db::NetGraph &graph, const tl::equivalence_clusters<const NetGraphNode *> &identical_nodes, std::set<size_t> &considered_nodes, const std::set<size_t> &symmetry_group, std::vector<std::set<size_t> > &symmetry_groups)
{
  std::set<size_t> cids;
  std::set<size_t> new_symmetry_group;
  NetGraphNode::edge_type::first_type edge;

  std::vector<NetGraphNode::edge_type> common_nodes_first;

  bool has_candidate = true;

  for (std::set<size_t>::const_iterator g = symmetry_group.begin (); g != symmetry_group.end () && has_candidate; ++g) {

    std::vector<NetGraphNode::edge_type> common_nodes;

    const NetGraphNode &n = graph.node (*g);
    for (NetGraphNode::edge_iterator e = n.begin (); e != n.end () && has_candidate; ++e) {

      if (considered_nodes.find (e->second.first) != considered_nodes.end ()) {
        continue;
      }

      const NetGraphNode *other = &graph.node (e->second.first);
      //  NOTE: nodes with pins don't go into a symmetry group as we don't know what the pins connect to
      if (other->net ()->pin_count () == 0 && identical_nodes.has_attribute (other)) {

        if (cids.empty ()) {
          edge = e->first;
        } else if (edge != e->first) {
          has_candidate = false;
        }

        if (has_candidate) {
          cids.insert (identical_nodes.cluster_id (other));
          if (cids.size () > 1) {
            has_candidate = false;
          } else {
            new_symmetry_group.insert (e->second.first);
          }
        }

      } else {
        common_nodes.push_back (*e);
      }

    }

    if (has_candidate) {

      //  all other edges need to have identical destinations for the symmetry group to be
      //  actually symmetric
      std::sort (common_nodes.begin (), common_nodes.end ());
      if (g == symmetry_group.begin ()) {
        common_nodes_first.swap (common_nodes);
      } else if (common_nodes_first != common_nodes) {
        has_candidate = false;
      }

    }

  }

  if (has_candidate && ! cids.empty () && new_symmetry_group.size () > 1) {

    considered_nodes.insert (new_symmetry_group.begin (), new_symmetry_group.end ());

    if (derive_symmetry_groups (graph, identical_nodes, considered_nodes, new_symmetry_group, symmetry_groups)) {

      symmetry_groups.push_back (new_symmetry_group);

    } else {

      std::set<size_t> cn;
      std::set_difference (considered_nodes.begin (), considered_nodes.end (), new_symmetry_group.begin (), new_symmetry_group.end (), std::inserter (cn, cn.begin ()));
      considered_nodes.swap (cn);

      has_candidate = false;

    }

  }

  return has_candidate;
}

void
NetlistComparer::join_symmetric_nets (db::Circuit *circuit)
{
  if (! circuit) {
    return;
  }

  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Join symmetric nodes for circuit: ")) + circuit->name ());

  db::DeviceFilter device_filter (m_cap_threshold, m_res_threshold);
  db::CircuitPinCategorizer circuit_pin_equivalence;
  std::map<const db::Circuit *, CircuitMapper> circuit_and_pin_mapping;

  db::NetGraph graph;
  db::CircuitCategorizer circuit_categorizer;
  db::DeviceCategorizer device_categorizer;
  graph.build (circuit, device_categorizer, circuit_categorizer, device_filter, &circuit_and_pin_mapping, &circuit_pin_equivalence, (size_t *) 0);

  //  sort the nodes so we can easily identify the identical ones (in terms of topology)
  //  nodes are identical if the attached devices and circuits are of the same kind and with the same parameters
  //  and connect to other nodes in identical configurations.

  std::vector<NodeEdgePair> nodes;

  std::vector<NetGraphNode::edge_type> no_edges;
  no_edges.push_back (NetGraphNode::edge_type ());

  nodes.reserve (graph.end () - graph.begin ());
  for (db::NetGraph::node_iterator i = graph.begin (); i != graph.end (); ++i) {
    if (! i->has_other () && i->net ()) {
      nodes.push_back (NodeEdgePair (i.operator-> (), no_edges.begin ()));
    }
  }

  std::sort (nodes.begin (), nodes.end (), CompareNodeEdgePair ());

  //  Identical nodes leading to the same nodes on the other side are candidates for symmetry.

  tl::equivalence_clusters<const NetGraphNode *> identical_nodes;

  for (std::vector<NodeEdgePair>::const_iterator np = nodes.begin (); np + 1 != nodes.end (); ++np) {
    if (*np[0].node == *np[1].node) {
      identical_nodes.same (np[0].node, np[1].node);
    }
  }

  std::vector<std::set<size_t> > symmetry_groups;
  std::set<size_t> visited;

  for (std::vector<NodeEdgePair>::const_iterator np = nodes.begin (); np != nodes.end (); ++np) {

    size_t node_id = graph.node_index_for_net (np[0].node->net ());
    if (visited.find (node_id) != visited.end ()) {
      continue;
    }

    std::set<size_t> considered_nodes;
    considered_nodes.insert (node_id);

    std::set<size_t> symmetry_group;
    symmetry_group.insert (node_id);

    derive_symmetry_groups (graph, identical_nodes, considered_nodes, symmetry_group, symmetry_groups);

    visited.insert (considered_nodes.begin (), considered_nodes.end ());

  }

  std::sort (symmetry_groups.begin (), symmetry_groups.end ());
  symmetry_groups.erase (std::unique (symmetry_groups.begin (), symmetry_groups.end ()), symmetry_groups.end ());

  if (! symmetry_groups.empty () && tl::verbosity () >= 30) {
    tl::info << tl::to_string (tr ("Symmetry groups:"));
    int index = 0;
    for (std::vector<std::set<size_t> >::const_iterator g = symmetry_groups.begin (); g != symmetry_groups.end (); ++g) {
      tl::info << "  [" << index << "] " << tl::noendl;
      for (std::set<size_t>::const_iterator i = g->begin (); i != g->end (); ++i) {
        tl::info << (i == g->begin () ? "" : ",") << (graph.node (*i).net () ? graph.node (*i).net ()->expanded_name ().c_str () : "(null)") << tl::noendl;
      }
      ++index;
      tl::info << "";
    }
  }

  //  join the nets

  for (std::vector<std::set<size_t> >::const_iterator g = symmetry_groups.begin (); g != symmetry_groups.end (); ++g) {
    for (std::set<size_t>::const_iterator i = g->begin (); i != g->end (); ++i) {
      if (i != g->begin ()) {
        circuit->join_nets (const_cast<db::Net *> (graph.net_by_node_index (*g->begin ())), const_cast<db::Net *> (graph.net_by_node_index (*i)));
      }
    }
  }
}

}

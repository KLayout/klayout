

/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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
#include "dbNetlistDeviceClasses.h"
#include "tlProgress.h"
#include "tlTimer.h"
#include "tlEquivalenceClusters.h"
#include "tlLog.h"
#include "tlEnv.h"
#include "tlInternational.h"

#include <cstring>

namespace {

struct GlobalCompareOptions
{
  GlobalCompareOptions ()
  {
    m_is_initialized = false;
  }

  void ensure_initialized ()
  {
    if (! m_is_initialized) {
      //  $KLAYOUT_NETLIST_COMPARE_DEBUG_NETCOMPARE
      debug_netcompare = tl::app_flag ("netlist-compare-debug-netcompare");
      //  $KLAYOUT_NETLIST_COMPARE_DEBUG_NETGRAPH
      debug_netgraph = tl::app_flag ("netlist-compare-debug-netgraph");
      m_is_initialized = true;
    }
  }

  bool debug_netcompare;
  bool debug_netgraph;

private:
  bool m_is_initialized;
};

}

static GlobalCompareOptions *options ()
{
  //  TODO: thread safe?
  static GlobalCompareOptions s_options;
  s_options.ensure_initialized ();
  return &s_options;
}

//  A constant indicating a failed match
const size_t failed_match = std::numeric_limits<size_t>::max ();

//  A constant indicating an unknown match
// const size_t unknown_match = std::numeric_limits<size_t>::max () - 1;

//  A constant indicating an invalid ID
const size_t invalid_id = std::numeric_limits<size_t>::max ();

//  A constant indicating an unknown ID
const size_t unknown_id = std::numeric_limits<size_t>::max () - 1;

namespace db
{

// --------------------------------------------------------------------------------------------------------------------
//  A generic string compare

bool combined_case_sensitive (const db::Netlist *a, const db::Netlist *b)
{
  bool csa = a ? a->is_case_sensitive () : true;
  bool csb = b ? b->is_case_sensitive () : true;
  return csa && csb;
}

int name_compare (const db::Net *a, const db::Net *b)
{
  return db::Netlist::name_compare (combined_case_sensitive (a->netlist (), b->netlist ()), a->name (), b->name ());
}


// --------------------------------------------------------------------------------------------------------------------
//  DeviceCompare definition and implementation

/**
 *  @brief The device compare function with "less" (operator()) and "equal" predicates
 *
 *  Device comparison is based on the equivalence of device classes (by category) and
 *  in a second step, by equivalence of the devices. The device class will implement
 *  the device equivalence function.
 */
struct DeviceCompare
{
  bool operator() (const std::pair<const db::Device *, size_t> &d1, const std::pair<const db::Device *, size_t> &d2) const
  {
    if (d1.second != d2.second) {
      return d1.second < d2.second;
    }
    return db::DeviceClass::less (*d1.first, *d2.first);
  }

  bool equals (const std::pair<const db::Device *, size_t> &d1, const std::pair<const db::Device *, size_t> &d2) const
  {
    if (d1.second != d2.second) {
      return false;
    }
    return db::DeviceClass::equal (*d1.first, *d2.first);
  }
};

// --------------------------------------------------------------------------------------------------------------------
//  SubCircuitCompare definition and implementation

/**
 *  @brief The compare function for subcircuits
 *
 *  As Subcircuits are not parametrized, the comparison of subcircuits is only based on
 *  the circuit equivalence (via category).
 */
struct SubCircuitCompare
{
  bool operator() (const std::pair<const db::SubCircuit *, size_t> &sc1, const std::pair<const db::SubCircuit *, size_t> &sc2) const
  {
    return sc1.second < sc2.second;
  }

  bool equals (const std::pair<const db::SubCircuit *, size_t> &sc1, const std::pair<const db::SubCircuit *, size_t> &sc2) const
  {
    return sc1.second == sc2.second;
  }
};

// --------------------------------------------------------------------------------------------------------------------
//  CircuitPinMapper definition and implementation

/**
 *  @brief The Circuit pin mapper handles swappable pin definitions per circuit
 *
 *  Swappable pins are implemented by mapping a pin ID to an equivalent or
 *  effective ID which is shared by all swappable pins.
 *
 *  This class manages swappable pins on a per-circuit basis.
 */
class CircuitPinMapper
{
public:
  CircuitPinMapper ()
  {
    //  .. nothing yet ..
  }

  void map_pins (const db::Circuit *circuit, size_t pin1_id, size_t pin2_id)
  {
    m_pin_map [circuit].same (pin1_id, pin2_id);
  }

  void map_pins (const db::Circuit *circuit, const std::vector<size_t> &pin_ids)
  {
    if (pin_ids.size () < 2) {
      return;
    }

    tl::equivalence_clusters<size_t> &pm = m_pin_map [circuit];
    for (size_t i = 1; i < pin_ids.size (); ++i) {
      pm.same (pin_ids [0], pin_ids [i]);
    }
  }

  size_t is_mapped (const db::Circuit *circuit, size_t pin_id) const
  {
    std::map<const db::Circuit *, tl::equivalence_clusters<size_t> >::const_iterator pm = m_pin_map.find (circuit);
    if (pm != m_pin_map.end ()) {
      return pm->second.has_attribute (pin_id);
    } else {
      return false;
    }
  }

  size_t normalize_pin_id (const db::Circuit *circuit, size_t pin_id) const
  {
    std::map<const db::Circuit *, tl::equivalence_clusters<size_t> >::const_iterator pm = m_pin_map.find (circuit);
    if (pm != m_pin_map.end ()) {
      size_t cluster_id = pm->second.cluster_id (pin_id);
      if (cluster_id > 0) {
        return (*pm->second.begin_cluster (cluster_id))->first;
      }
    }
    return pin_id;
  }

private:
  std::map<const db::Circuit *, tl::equivalence_clusters<size_t> > m_pin_map;
};

// --------------------------------------------------------------------------------------------------------------------
//  CircuitMapper definition and implementation

/**
 *  @brief A circuit mapper
 *
 *  The circuit mapper handles the circuit equivalence between the circuits of
 *  netlist A and B and also the pin mapping between the circuits from these netlists.
 *
 *  The "other" attribute will hold the circuit for the other netlist.
 *  The other methods handle pin mapping from "other" into "this" pin space.
 */
class CircuitMapper
{
public:
  CircuitMapper ()
    : mp_other (0)
  {
    //  .. nothing yet ..
  }

  void set_other (const db::Circuit *other)
  {
    mp_other = other;
  }

  const db::Circuit *other () const
  {
    return mp_other;
  }

  void map_pin (size_t this_pin, size_t other_pin)
  {
    m_pin_map.insert (std::make_pair (this_pin, other_pin));
    m_rev_pin_map.insert (std::make_pair (other_pin, this_pin));
  }

  bool has_other_pin_for_this_pin (size_t this_pin) const
  {
    return m_pin_map.find (this_pin) != m_pin_map.end ();
  }

  bool has_this_pin_for_other_pin (size_t other_pin) const
  {
    return m_rev_pin_map.find (other_pin) != m_rev_pin_map.end ();
  }

  size_t other_pin_from_this_pin (size_t this_pin) const
  {
    std::map<size_t, size_t>::const_iterator i = m_pin_map.find (this_pin);
    tl_assert (i != m_pin_map.end ());
    return i->second;
  }

  size_t this_pin_from_other_pin (size_t other_pin) const
  {
    std::map<size_t, size_t>::const_iterator i = m_rev_pin_map.find (other_pin);
    tl_assert (i != m_rev_pin_map.end ());
    return i->second;
  }

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
class DeviceFilter
{
public:
  DeviceFilter (double cap_threshold, double res_threshold)
    : m_cap_threshold (cap_threshold), m_res_threshold (res_threshold)
  {
    //  .. nothing yet ..
  }

  bool filter (const db::Device *device) const
  {
    const db::DeviceClassResistor *res = dynamic_cast<const db::DeviceClassResistor *> (device->device_class ());
    const db::DeviceClassCapacitor *cap = dynamic_cast<const db::DeviceClassCapacitor *> (device->device_class ());

    if (res) {
      if (m_res_threshold > 0.0 && device->parameter_value (db::DeviceClassResistor::param_id_R) > m_res_threshold) {
        return false;
      }
    } else if (cap) {
      if (m_cap_threshold > 0.0 && device->parameter_value (db::DeviceClassCapacitor::param_id_C) < m_cap_threshold) {
        return false;
      }
    }

    return true;
  }

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
class generic_categorizer
{
public:
  generic_categorizer (bool with_name = true)
    : m_next_cat (0), m_with_name (with_name), m_case_sensitive (true)
  {
    //  .. nothing yet ..
  }

  void set_case_sensitive (bool f)
  {
    m_case_sensitive = f;
  }

  void same (const Obj *ca, const Obj *cb)
  {
    if (! ca && ! cb) {
      return;
    } else if (! ca) {
      same (cb, ca);
    } else if (! cb) {
      //  making a object same as null will make this device being ignored
      m_cat_by_ptr [ca] = 0;
      return;
    }

    //  reuse existing category if one is assigned already -> this allows associating
    //  multiple categories to other ones (A->C, B->C)
    typename std::map<const Obj *, size_t>::const_iterator cpa = m_cat_by_ptr.find (ca);
    typename std::map<const Obj *, size_t>::const_iterator cpb = m_cat_by_ptr.find (cb);

    if (cpa != m_cat_by_ptr.end () && cpb != m_cat_by_ptr.end ()) {

      if (cpa->second != cpb->second) {
        //  join categories (cat(B)->cat(A))
        for (typename std::map<const Obj *, size_t>::iterator cp = m_cat_by_ptr.begin (); cp != m_cat_by_ptr.end (); ++cp) {
          if (cp->second == cpb->second) {
            cp->second = cpa->second;
          }
        }
      }

    } else if (cpb != m_cat_by_ptr.end ()) {

      //  reuse cat(B) category
      m_cat_by_ptr.insert (std::make_pair (ca, cpb->second));

    } else if (cpa != m_cat_by_ptr.end ()) {

      //  reuse cat(A) category
      m_cat_by_ptr.insert (std::make_pair (cb, cpa->second));

    } else {

      //  new category
      ++m_next_cat;
      m_cat_by_ptr.insert (std::make_pair (ca, m_next_cat));
      m_cat_by_ptr.insert (std::make_pair (cb, m_next_cat));

    }
  }

  bool has_cat_for (const Obj *cls)
  {
    return m_cat_by_ptr.find (cls) != m_cat_by_ptr.end ();
  }

  size_t cat_for (const Obj *cls)
  {
    typename std::map<const Obj *, size_t>::const_iterator cp = m_cat_by_ptr.find (cls);
    if (cp != m_cat_by_ptr.end ()) {
      return cp->second;
    }

    if (m_with_name) {

      std::string cls_name = db::Netlist::normalize_name (m_case_sensitive, cls->name ());

      std::map<std::string, size_t>::const_iterator c = m_cat_by_name.find (cls_name);
      if (c != m_cat_by_name.end ()) {
        m_cat_by_ptr.insert (std::make_pair (cls, c->second));
        return c->second;
      } else {
        ++m_next_cat;
        m_cat_by_name.insert (std::make_pair (cls_name, m_next_cat));
        m_cat_by_ptr.insert (std::make_pair (cls, m_next_cat));
        return m_next_cat;
      }

    } else {

      ++m_next_cat;
      m_cat_by_ptr.insert (std::make_pair (cls, m_next_cat));
      return m_next_cat;

    }
  }

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
class DeviceCategorizer
  : private generic_categorizer<db::DeviceClass>
{
public:
  DeviceCategorizer ()
    : generic_categorizer<db::DeviceClass> ()
  {
    //  .. nothing yet ..
  }

  void same_class (const db::DeviceClass *ca, const db::DeviceClass *cb)
  {
    generic_categorizer<db::DeviceClass>::same (ca, cb);
  }

  size_t cat_for_device (const db::Device *device)
  {
    const db::DeviceClass *cls = device->device_class ();
    if (! cls) {
      return 0;
    }

    return cat_for_device_class (cls);
  }

  bool has_cat_for_device_class (const db::DeviceClass *cls)
  {
    return generic_categorizer<db::DeviceClass>::has_cat_for (cls);
  }

  size_t cat_for_device_class (const db::DeviceClass *cls)
  {
    return generic_categorizer<db::DeviceClass>::cat_for (cls);
  }

  void clear_strict_device_categories ()
  {
    m_strict_device_categories.clear ();
  }

  void set_strict_device_category (size_t cat)
  {
    m_strict_device_categories.insert (cat);
  }

  bool is_strict_device_category (size_t cat) const
  {
    return m_strict_device_categories.find (cat) != m_strict_device_categories.end ();
  }

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
class CircuitCategorizer
  : private generic_categorizer<db::Circuit>
{
public:
  CircuitCategorizer ()
    : generic_categorizer<db::Circuit> ()
  {
    //  .. nothing yet ..
  }

  void same_circuit (const db::Circuit *ca, const db::Circuit *cb)
  {
    //  no arbitrary cross-pairing
    //  NOTE: many layout circuits are allowed for one schematic to account for layout alternatives.
    if (ca && has_cat_for (ca)) {
      throw tl::Exception (tl::to_string (tr ("Circuit is already paired with other circuit: ")) + ca->name ());
    }
    generic_categorizer<db::Circuit>::same (ca, cb);
  }

  size_t cat_for_subcircuit (const db::SubCircuit *subcircuit)
  {
    const db::Circuit *cr = subcircuit->circuit_ref ();
    if (! cr) {
      return 0;
    } else {
      return cat_for_circuit (cr);
    }
  }

  size_t cat_for_circuit (const db::Circuit *cr)
  {
    return generic_categorizer<db::Circuit>::cat_for (cr);
  }

  void set_case_sensitive (bool f)
  {
    generic_categorizer::set_case_sensitive (f);
  }
};

// --------------------------------------------------------------------------------------------------------------------
//  A structure to keep the data during compare

class NetGraph;

struct CompareData
{
  CompareData ()
    : other (0), max_depth (0), max_n_branch (0), depth_first (true), dont_consider_net_names (false), with_ambiguous (false), logger (0),
      circuit_pin_mapper (0), subcircuit_equivalence (0), device_equivalence (0), progress (0)
  { }

  NetGraph *other;
  size_t max_depth;
  size_t max_n_branch;
  bool depth_first;
  bool dont_consider_net_names;
  bool with_ambiguous;
  NetlistCompareLogger *logger;
  CircuitPinMapper *circuit_pin_mapper;
  SubCircuitEquivalenceTracker *subcircuit_equivalence;
  DeviceEquivalenceTracker *device_equivalence;
  tl::RelativeProgress *progress;
};

// --------------------------------------------------------------------------------------------------------------------
//  NetGraphNode definition and implementation

static size_t translate_terminal_id (size_t tid, const db::Device *device)
{
  return device->device_class () ? device->device_class ()->normalize_terminal_id (tid) : tid;
}

/**
 *  @brief A node within the net graph
 *
 *  This class represents a node and the edges leading from this node to
 *  other nodes.
 *
 *  A graph edge is a collection of transitions, connecting terminals of
 *  devices or pins of subcircuits plus the index of node at the other end
 *  of the edge.
 *
 *  Transitions are sorted within the edge.
 */
class NetGraphNode
{
public:
  /**
   *  @brief Represents one transition within an edge_iterator
   *
   *  Each transition connects two pins of subcircuits or terminals
   *  of devices.
   *  An edge is basically a collection of transitions.
   */
  struct Transition
  {

    Transition (const db::Device *device, size_t device_category, size_t terminal1_id, size_t terminal2_id)
    {
      device_pair ().first = device;
      device_pair ().second = device_category;
      m_id1 = terminal1_id;
      m_id2 = terminal2_id;
    }

    Transition (const db::SubCircuit *subcircuit, size_t subcircuit_category, size_t pin1_id, size_t pin2_id)
    {
      subcircuit_pair ().first = subcircuit;
      subcircuit_pair ().second = subcircuit_category;
      m_id1 = std::numeric_limits<size_t>::max () - pin1_id;
      m_id2 = pin2_id;
    }

    size_t id1 () const
    {
      return m_id1;
    }

    size_t id2 () const
    {
      return m_id2;
    }

    bool operator< (const Transition &other) const
    {
      if (is_for_subcircuit () != other.is_for_subcircuit ()) {
        return is_for_subcircuit () < other.is_for_subcircuit ();
      }

      if (is_for_subcircuit ()) {

        if ((subcircuit_pair ().first != 0) != (other.subcircuit_pair ().first != 0)) {
          return (subcircuit_pair ().first != 0) < (other.subcircuit_pair ().first != 0);
        }

        if (subcircuit_pair ().first != 0) {
          SubCircuitCompare scc;
          if (! scc.equals (subcircuit_pair (), other.subcircuit_pair ())) {
            return scc (subcircuit_pair (), other.subcircuit_pair ());
          }
        }

      } else {

        if ((device_pair ().first != 0) != (other.device_pair ().first != 0)) {
          return (device_pair ().first != 0) < (other.device_pair ().first != 0);
        }

        if (device_pair ().first != 0) {
          DeviceCompare dc;
          if (! dc.equals (device_pair (), other.device_pair ())) {
            return dc (device_pair (), other.device_pair ());
          }
        }

      }

      if (m_id1 != other.m_id1) {
        return m_id1 < other.m_id1;
      }
      return m_id2 < other.m_id2;
    }

    bool operator== (const Transition &other) const
    {
      if (is_for_subcircuit () != other.is_for_subcircuit ()) {
        return false;
      }

      if (is_for_subcircuit ()) {

        if ((subcircuit_pair ().first != 0) != (other.subcircuit_pair ().first != 0)) {
          return false;
        }

        if (subcircuit_pair ().first != 0) {
          SubCircuitCompare scc;
          if (! scc.equals (subcircuit_pair (), other.subcircuit_pair ())) {
            return false;
          }
        }

      } else {

        if ((device_pair ().first != 0) != (other.device_pair ().first != 0)) {
          return false;
        }

        if (device_pair ().first != 0) {
          DeviceCompare dc;
          if (! dc.equals (device_pair (), other.device_pair ())) {
            return false;
          }
        }

      }

      return (m_id1 == other.m_id1 && m_id2 == other.m_id2);
    }

    std::string to_string () const
    {
      if (is_for_subcircuit ()) {
        const db::SubCircuit *sc = subcircuit_pair ().first;
        const db::Circuit *c = sc->circuit_ref ();
        return std::string ("X") + sc->expanded_name () + " " + c->name ();
     } else {
        size_t term_id1 = m_id1;
        size_t term_id2 = m_id2;
        const db::Device *d = device_pair ().first;
        const db::DeviceClass *dc = d->device_class ();
        return std::string ("D") + d->expanded_name () + " " + dc->name () + " "
          + "(" + dc->terminal_definitions () [term_id1].name () + ")->(" + dc->terminal_definitions () [term_id2].name () + ")";
      }
    }

    inline bool is_for_subcircuit () const
    {
      return m_id1 > std::numeric_limits<size_t>::max () / 2;
    }

    std::pair<const db::Device *, size_t> &device_pair ()
    {
      return *reinterpret_cast<std::pair<const db::Device *, size_t> *> ((void *) &m_ref);
    }

    const std::pair<const db::Device *, size_t> &device_pair () const
    {
      return *reinterpret_cast<const std::pair<const db::Device *, size_t> *> ((const void *) &m_ref);
    }

    std::pair<const db::SubCircuit *, size_t> &subcircuit_pair ()
    {
      return *reinterpret_cast<std::pair<const db::SubCircuit *, size_t> *> ((void *) &m_ref);
    }

    const std::pair<const db::SubCircuit *, size_t> &subcircuit_pair () const
    {
      return *reinterpret_cast<const std::pair<const db::SubCircuit *, size_t> *> ((const void *) &m_ref);
    }

  private:
    char m_ref [sizeof (std::pair<const void *, size_t>)];
    size_t m_id1, m_id2;
  };

  typedef std::pair<std::vector<Transition>, std::pair<size_t, const db::Net *> > edge_type;

  static void swap_edges (edge_type &e1, edge_type &e2)
  {
    e1.first.swap (e2.first);
    std::swap (e1.second, e2.second);
  }

  struct EdgeToEdgeOnlyCompare
  {
    bool operator() (const edge_type &a, const std::vector<Transition> &b) const
    {
      return a.first < b;
    }
  };

  typedef std::vector<edge_type>::const_iterator edge_iterator;

  NetGraphNode ()
    : mp_net (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Builds a node for a net
   */
  NetGraphNode (const db::Net *net, DeviceCategorizer &device_categorizer, CircuitCategorizer &circuit_categorizer, const DeviceFilter &device_filter, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinMapper *pin_map);

  /**
   *  @brief Builds a virtual node for a subcircuit
   */
  NetGraphNode (const db::SubCircuit *sc, CircuitCategorizer &circuit_categorizer, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinMapper *pin_map);

  void expand_subcircuit_nodes (NetGraph *graph);

  std::string to_string () const;

  const db::Net *net () const
  {
    return mp_net;
  }

  bool has_other () const
  {
    return m_other_net_index != invalid_id && m_other_net_index != unknown_id;
  }

  bool has_any_other () const
  {
    return m_other_net_index != invalid_id;
  }

  bool has_unknown_other () const
  {
    return m_other_net_index == unknown_id;
  }

  size_t other_net_index () const
  {
    return m_other_net_index;
  }

  void set_other_net (size_t index)
  {
    m_other_net_index = index;
  }

  void unset_other_net ()
  {
    m_other_net_index = invalid_id;
  }

  bool empty () const
  {
    return m_edges.empty ();
  }

  void apply_net_index (const std::map<const db::Net *, size_t> &ni);

  bool operator< (const NetGraphNode &node) const;
  bool operator== (const NetGraphNode &node) const;

  void swap (NetGraphNode &other)
  {
    std::swap (m_other_net_index, other.m_other_net_index);
    std::swap (mp_net, other.mp_net);
    m_edges.swap (other.m_edges);
  }

  edge_iterator begin () const
  {
    return m_edges.begin ();
  }

  edge_iterator end () const
  {
    return m_edges.end ();
  }

  edge_iterator find_edge (const std::vector<Transition> &edge) const
  {
    edge_iterator res = std::lower_bound (begin (), end (), edge, NetGraphNode::EdgeToEdgeOnlyCompare ());
    if (res == end () || res->first != edge) {
      return end ();
    } else {
      return res;
    }
  }

private:
  const db::Net *mp_net;
  size_t m_other_net_index;
  std::vector<edge_type> m_edges;

  /**
   *  @brief Compares edges as "less"
   *  Edge comparison is based on the pins attached (name of the first pin).
   */
  static bool net_less (const db::Net *a, const db::Net *b);

  /**
   *  @brief Compares edges as "equal"
   *  See edge_less for the comparison details.
   */
  static bool edge_equal (const db::Net *a, const db::Net *b);
};

// --------------------------------------------------------------------------------------------------------------------
//  NetGraph definition and implementation

}

namespace std
{
  void swap (db::NetGraphNode &a, db::NetGraphNode &b)
  {
    a.swap (b);
  }
}

namespace db
{

struct CompareNodePtr
{
  bool operator() (const std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> &a, const std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> &b) const
  {
    return *a.first < *b.first;
  }
};

class TentativeNodeMapping;
struct NodeRange;
class DeviceMapperForTargetNode;
class SubCircuitMapperForTargetNode;

std::string indent (size_t depth)
{
  std::string s;
  for (size_t d = 0; d < depth; ++d) {
    s += "|  ";
  }
  return s;
}

/**
 *  @brief The net graph for the compare algorithm
 */
class NetGraph
{
public:
  typedef std::vector<NetGraphNode>::const_iterator node_iterator;

  NetGraph ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Builds the net graph
   */
  void build (const db::Circuit *c, DeviceCategorizer &device_categorizer, CircuitCategorizer &circuit_categorizer, const db::DeviceFilter &device_filter, const std::map<const db::Circuit *, CircuitMapper> *circuit_and_pin_mapping, const CircuitPinMapper *circuit_pin_mapper);

  /**
   *  @brief Gets the node index for the given net
   */
  size_t node_index_for_net (const db::Net *net) const
  {
    std::map<const db::Net *, size_t>::const_iterator j = m_net_index.find (net);
    tl_assert (j != m_net_index.end ());
    return j->second;
  }

  /**
   *  @brief Gets the node for a given node index
   */
  const db::NetGraphNode &node (size_t net_index) const
  {
    return m_nodes [net_index];
  }

  /**
   *  @brief Gets the node for a given node index (non-const version)
   */
  db::NetGraphNode &node (size_t net_index)
  {
    return m_nodes [net_index];
  }

  /**
   *  @brief Gets the subcircuit virtual node per subcircuit
   *  These nodes are a concept provided to reduce the effort for
   *  subcircuit transitions. Instead of a transition from every pin
   *  to every other pin the virtual node provides edges to
   *  all pins of the subcircuit, but no front end.
   */
  const db::NetGraphNode &virtual_node (const db::SubCircuit *sc) const
  {
    std::map<const db::SubCircuit *, db::NetGraphNode>::const_iterator j = m_virtual_nodes.find (sc);
    tl_assert (j != m_virtual_nodes.end ());
    return j->second;
  }

  /**
   *  @brief Gets the subcircuit virtual node per subcircuit
   */
  db::NetGraphNode &virtual_node (const db::SubCircuit *sc)
  {
    return const_cast<db::NetGraphNode &> (((const NetGraph *) this)->virtual_node (sc));
  }

  /**
   *  @brief Gets the net for a given node index
   */
  const db::Net *net_by_node_index (size_t net_index) const
  {
    return m_nodes [net_index].net ();
  }

  /**
   *  @brief Establishes an equivalence between two nodes of netlist A (this) and B (other)
   */
  void identify (size_t net_index, size_t other_net_index)
  {
    m_nodes [net_index].set_other_net (other_net_index);
  }

  /**
   *  @brief Removes the equivalence from the node with the given index
   */
  void unidentify (size_t net_index)
  {
    m_nodes [net_index].unset_other_net ();
  }

  /**
   *  @brief Iterator over the nodes in this graph (begin)
   */
  node_iterator begin () const
  {
    return m_nodes.begin ();
  }

  /**
   *  @brief Iterator over the nodes in this graph (end)
   */
  node_iterator end () const
  {
    return m_nodes.end ();
  }

  /**
   *  @brief The circuit this graph is associated with
   */
  const db::Circuit *circuit () const
  {
    return mp_circuit;
  }

  /**
   *  @brief Implementation of the backtracking algorithm
   *
   *  This method derives new node assignments based on the (proposed)
   *  identity of nodes this->[net_index] and other[node].
   *  The return value will be:
   *
   *   >0    if node identity could be established. The return value
   *         is the number of new node pairs established. All
   *         node pairs (including the initial proposed identity)
   *         are assigned.
   *   ==0   identity could be established. No more assignments are made.
   *   max   no decision could be made because the max. complexity
   *         was exhausted. No assignments were made.
   *
   *  (here: max=max of size_t). The complexity is measured in
   *  backtracking depth (number of graph jumps) and decision tree
   *  branching complexity N (="n_branch", means: N*N decisions to be made).
   *
   *  If "tentative" is non-null, assignments will be recorded in the TentativeMapping
   *  audit object and can be undone afterwards when backtracking recursion happens.
   */
  size_t derive_node_identities (size_t net_index, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, CompareData *data);

  /**
   *  @brief The backtracking driver
   *
   *  This method will analyze the given nodes and call "derive_node_identities" for all nodes
   *  with a proposed identity.
   */
  size_t derive_node_identities_from_node_set (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> > &nodes, std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> > &other_nodes, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, CompareData *data);

private:
  std::vector<NetGraphNode> m_nodes;
  std::map<const db::SubCircuit *, NetGraphNode> m_virtual_nodes;
  std::map<const db::Net *, size_t> m_net_index;
  const db::Circuit *mp_circuit;

  size_t derive_node_identities_for_edges (NetGraphNode::edge_iterator e, NetGraphNode::edge_iterator ee, NetGraphNode::edge_iterator e_other, NetGraphNode::edge_iterator ee_other, size_t net_index, size_t other_net_index, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, CompareData *data);
  size_t derive_node_identities_from_ambiguity_group (const NodeRange &nr, DeviceMapperForTargetNode &dm, DeviceMapperForTargetNode &dm_other, SubCircuitMapperForTargetNode &scm, SubCircuitMapperForTargetNode &scm_other, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, CompareData *data);
  size_t derive_node_identities_from_singular_match (const NetGraphNode *n, const NetGraphNode::edge_iterator &e, const NetGraphNode *n_other, const NetGraphNode::edge_iterator &e_other, DeviceMapperForTargetNode &dm, DeviceMapperForTargetNode &dm_other, SubCircuitMapperForTargetNode &scm, SubCircuitMapperForTargetNode &scm_other, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, CompareData *data, bool consider_net_names);
};

// --------------------------------------------------------------------------------------------------------------------

NetGraphNode::NetGraphNode (const db::Net *net, DeviceCategorizer &device_categorizer, CircuitCategorizer &circuit_categorizer, const DeviceFilter &device_filter, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinMapper *pin_map)
  : mp_net (net), m_other_net_index (invalid_id)
{
  if (! net) {
    return;
  }

  std::map<const void *, size_t> n2entry;

  for (db::Net::const_subcircuit_pin_iterator i = net->begin_subcircuit_pins (); i != net->end_subcircuit_pins (); ++i) {

    const db::SubCircuit *sc = i->subcircuit ();
    size_t circuit_cat = circuit_categorizer.cat_for_subcircuit (sc);
    if (! circuit_cat) {
      //  circuit is ignored
      continue;
    }

    size_t pin_id = i->pin ()->id ();
    const db::Circuit *cr = sc->circuit_ref ();

    std::map<const db::Circuit *, CircuitMapper>::const_iterator icm = circuit_map->find (cr);
    if (icm == circuit_map->end ()) {
      //  this can happen if the other circuit is not present - this is allowed for single-pin
      //  circuits.
      continue;
    }

    const CircuitMapper *cm = & icm->second;

    //  A pin assignment may be missing because there is no net for a pin -> skip this

    if (! cm->has_other_pin_for_this_pin (pin_id)) {
      continue;
    }

    //  NOTE: if cm is given, cr and pin_id are given in terms of the canonical "other" circuit.
    //  For c1 this is the c1->c2 mapper, for c2 this is the c2->c2 dummy mapper.

    pin_id = cm->other_pin_from_this_pin (pin_id);

    //  realize pin swapping by normalization of pin ID

    pin_id = pin_map->normalize_pin_id (cm->other (), pin_id);

    //  Subcircuits are routed to a null node and descend from a virtual node inside the subcircuit.
    //  The reasoning is that this way we don't need #pins*(#pins-1) edges but rather #pins.

    Transition ed (sc, circuit_cat, pin_id, pin_id);

    std::map<const void *, size_t>::const_iterator in = n2entry.find ((const void *) sc);
    if (in == n2entry.end ()) {
      in = n2entry.insert (std::make_pair ((const void *) sc, m_edges.size ())).first;
      m_edges.push_back (edge_type (std::vector<Transition> (), std::make_pair (size_t (0), (const db::Net *) 0)));
    }

    m_edges [in->second].first.push_back (ed);

  }

  for (db::Net::const_terminal_iterator i = net->begin_terminals (); i != net->end_terminals (); ++i) {

    const db::Device *d = i->device ();
    if (! device_filter.filter (d)) {
      continue;
    }

    size_t device_cat = device_categorizer.cat_for_device (d);
    if (! device_cat) {
      //  device is ignored
      continue;
    }

    bool is_strict = device_categorizer.is_strict_device_category (device_cat);

    //  strict device checking means no terminal swapping
    size_t terminal1_id = is_strict ? i->terminal_id () : translate_terminal_id (i->terminal_id (), d);

    const std::vector<db::DeviceTerminalDefinition> &td = d->device_class ()->terminal_definitions ();
    for (std::vector<db::DeviceTerminalDefinition>::const_iterator it = td.begin (); it != td.end (); ++it) {

      if (it->id () != i->terminal_id ()) {

        size_t terminal2_id = is_strict ? it->id () : translate_terminal_id (it->id (), d);
        Transition ed2 (d, device_cat, terminal1_id, terminal2_id);

        const db::Net *net2 = d->net_for_terminal (it->id ());
        if (! net2) {
          continue;
        }

        std::map<const void *, size_t>::const_iterator in = n2entry.find ((const void *) net2);
        if (in == n2entry.end ()) {
          in = n2entry.insert (std::make_pair ((const void *) net2, m_edges.size ())).first;
          m_edges.push_back (edge_type (std::vector<Transition> (), std::make_pair (size_t (0), net2)));
        }

        m_edges [in->second].first.push_back (ed2);

      }

    }

  }
}

NetGraphNode::NetGraphNode (const db::SubCircuit *sc, CircuitCategorizer &circuit_categorizer, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinMapper *pin_map)
  : mp_net (0), m_other_net_index (invalid_id)
{
  std::map<const db::Net *, size_t> n2entry;

  size_t circuit_cat = circuit_categorizer.cat_for_subcircuit (sc);
  tl_assert (circuit_cat != 0);

  const db::Circuit *cr = sc->circuit_ref ();
  tl_assert (cr != 0);

  std::map<const db::Circuit *, CircuitMapper>::const_iterator icm = circuit_map->find (cr);
  tl_assert (icm != circuit_map->end ());

  const CircuitMapper *cm = & icm->second;

  for (db::Circuit::const_pin_iterator p = cr->begin_pins (); p != cr->end_pins (); ++p) {

    size_t pin_id = p->id ();
    const db::Net *net_at_pin = sc->net_for_pin (pin_id);
    if (! net_at_pin) {
      continue;
    }

    //  A pin assignment may be missing because there is no net for a pin -> skip this

    if (! cm->has_other_pin_for_this_pin (pin_id)) {
      continue;
    }

    //  NOTE: if cm is given, cr and pin_id are given in terms of the canonical "other" circuit.
    //  For c1 this is the c1->c2 mapper, for c2 this is the c2->c2 dummy mapper.

    pin_id = cm->other_pin_from_this_pin (pin_id);

    //  realize pin swapping by normalization of pin ID

    pin_id = pin_map->normalize_pin_id (cm->other (), pin_id);

    //  Make the other endpoint

    Transition ed (sc, circuit_cat, pin_id, pin_id);

    std::map<const db::Net *, size_t>::const_iterator in = n2entry.find (net_at_pin);
    if (in == n2entry.end ()) {
      in = n2entry.insert (std::make_pair ((const db::Net *) net_at_pin, m_edges.size ())).first;
      m_edges.push_back (edge_type (std::vector<Transition> (), std::make_pair (size_t (0), net_at_pin)));
    }

    m_edges [in->second].first.push_back (ed);

  }
}

void
NetGraphNode::expand_subcircuit_nodes (NetGraph *graph)
{
  std::map<const db::Net *, size_t> n2entry;

  std::list<edge_type> sc_edges;

  size_t ii = 0;
  for (size_t i = 0; i < m_edges.size (); ++i) {
    if (ii != i) {
      swap_edges (m_edges [ii], m_edges [i]);
    }
    if (m_edges [ii].second.second == 0) {
      //  subcircuit pin
      sc_edges.push_back (m_edges [ii]);
    } else {
      n2entry.insert (std::make_pair (m_edges [ii].second.second, ii));
      ++ii;
    }
  }

  m_edges.erase (m_edges.begin () + ii, m_edges.end ());

  for (std::list<edge_type>::const_iterator e = sc_edges.begin (); e != sc_edges.end (); ++e) {

    const db::SubCircuit *sc = 0;
    for (std::vector<Transition>::const_iterator t = e->first.begin (); t != e->first.end (); ++t) {
      tl_assert (t->is_for_subcircuit ());
      if (! sc) {
        sc = t->subcircuit_pair ().first;
      } else {
        tl_assert (sc == t->subcircuit_pair ().first);
      }
    }

    const NetGraphNode &dn = graph->virtual_node (sc);
    for (NetGraphNode::edge_iterator de = dn.begin (); de != dn.end (); ++de) {

      const db::Net *net_at_pin = de->second.second;
      if (net_at_pin == net ()) {
        continue;
      }

      std::map<const db::Net *, size_t>::const_iterator in = n2entry.find (net_at_pin);
      if (in == n2entry.end ()) {
        in = n2entry.insert (std::make_pair ((const db::Net *) net_at_pin, m_edges.size ())).first;
        m_edges.push_back (edge_type (std::vector<Transition> (), de->second));
      }

      m_edges [in->second].first.insert (m_edges [in->second].first.end (), de->first.begin (), de->first.end ());

    }

  }

  //  "deep sorting" of the edge descriptor
  for (std::vector<edge_type>::iterator i = m_edges.begin (); i != m_edges.end (); ++i) {
    std::sort (i->first.begin (), i->first.end ());
  }

  std::sort (m_edges.begin (), m_edges.end ());
}

std::string
NetGraphNode::to_string () const
{
  std::string res = std::string ("[");
  if (mp_net) {
    res += mp_net->expanded_name ();
  } else {
    res += "(null)";
  }
  res += "]";
  if (m_other_net_index != invalid_id) {
    res += " (other: #" + tl::to_string (m_other_net_index) + ")";
  }
  res += "\n";

  for (std::vector<edge_type>::const_iterator e = m_edges.begin (); e != m_edges.end (); ++e) {
    res += "  (\n";
    for (std::vector<Transition>::const_iterator i = e->first.begin (); i != e->first.end (); ++i) {
      res += std::string ("    ") + i->to_string () + "\n";
    }
    res += "  )->";
    if (! e->second.second) {
      res += "(null)";
    } else {
      res += e->second.second->expanded_name () + "[#" + tl::to_string (e->second.first) + "]";
    }
    res += "\n";
  }
  return res;
}

void
NetGraphNode::apply_net_index (const std::map<const db::Net *, size_t> &ni)
{
  for (std::vector<edge_type>::iterator i = m_edges.begin (); i != m_edges.end (); ++i) {
    std::map<const db::Net *, size_t>::const_iterator j = ni.find (i->second.second);
    tl_assert (j != ni.end ());
    i->second.first = j->second;
  }

  //  "deep sorting" of the edge descriptor
  for (std::vector<edge_type>::iterator i = m_edges.begin (); i != m_edges.end (); ++i) {
    std::sort (i->first.begin (), i->first.end ());
  }

  std::sort (m_edges.begin (), m_edges.end ());
}

bool
NetGraphNode::operator< (const NetGraphNode &node) const
{
  if (m_edges.size () != node.m_edges.size ()) {
    return m_edges.size () < node.m_edges.size ();
  }
  for (size_t i = 0; i < m_edges.size (); ++i) {
    if (m_edges [i].first != node.m_edges [i].first) {
      return m_edges [i].first < node.m_edges [i].first;
    }
  }
  if (m_edges.empty ()) {
    //  do a more detailed analysis on the nets involved
    return net_less (net (), node.net ());
  }
  return false;
}

bool
NetGraphNode::operator== (const NetGraphNode &node) const
{
  if (m_edges.size () != node.m_edges.size ()) {
    return false;
  }
  for (size_t i = 0; i < m_edges.size (); ++i) {
    if (m_edges [i].first != node.m_edges [i].first) {
      return false;
    }
  }
  if (m_edges.empty ()) {
    //  do a more detailed analysis on the edges
    return edge_equal (net (), node.net ());
  }
  return true;
}

bool
NetGraphNode::net_less (const db::Net *a, const db::Net *b)
{
  if ((a != 0) != (b != 0)) {
    return (a != 0) < (b != 0);
  }
  if (a != 0) {
    if (a->pin_count () != b->pin_count ()) {
      return a->pin_count () < b->pin_count ();
    }
    if (a->pin_count () > 0) {
      const std::string &pna = a->begin_pins ()->pin ()->name ();
      const std::string &pnb = b->begin_pins ()->pin ()->name ();
      if (! pna.empty () && ! pnb.empty ()) {
        return db::Netlist::name_compare (combined_case_sensitive (a->netlist (), b->netlist ()), pna, pnb) < 0;
      }
    }
    return false;
  } else {
    return false;
  }
}

bool
NetGraphNode::edge_equal (const db::Net *a, const db::Net *b)
{
  if ((a != 0) != (b != 0)) {
    return false;
  }
  if (a != 0) {
    if (a->pin_count () != b->pin_count ()) {
      return false;
    }
    if (a->pin_count () > 0) {
      const std::string &pna = a->begin_pins ()->pin ()->name ();
      const std::string &pnb = b->begin_pins ()->pin ()->name ();
      if (! pna.empty () && ! pnb.empty ()) {
        return db::Netlist::name_compare (combined_case_sensitive (a->netlist (), b->netlist ()), pna, pnb) == 0;
      }
    }
    return true;
  } else {
    return true;
  }
}

// --------------------------------------------------------------------------------------------------------------------

/**
 *  @brief Represents an interval of NetGraphNode objects in a node set
 */
struct NodeRange
{
  NodeRange (size_t _num, std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::iterator _n1, std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::iterator _nn1,
                          std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::iterator _n2, std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::iterator _nn2)
    : num (_num), n1 (_n1), nn1 (_nn1), n2 (_n2), nn2 (_nn2)
  {
    //  .. nothing yet ..
  }

  bool operator< (const NodeRange &other) const
  {
    return num < other.num;
  }

  size_t num;
  std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::iterator n1, nn1, n2, nn2;
};

// --------------------------------------------------------------------------------------------------------------------

struct CatAndIds
{
public:
  CatAndIds (size_t _cat, size_t _id1, size_t _id2)
    : cat (_cat), id1 (_id1), id2 (_id2)
  { }

  bool operator== (const CatAndIds &other) const
  {
    return cat == other.cat && id1 == other.id1 && id2 == other.id2;
  }

  bool operator< (const CatAndIds &other) const
  {
    if (cat != other.cat) {
      return cat < other.cat;
    }
    if (id1 != other.id1) {
      return id1 < other.id1;
    }
    if (id2 != other.id2) {
      return id2 < other.id2;
    }
    return false;
  }

  size_t cat, id1, id2;
};

template <class Obj>
class generic_mapper_for_target_node
{
public:
  generic_mapper_for_target_node ()
  {
    //  .. nothing yet ..
  }

  static void derive_mapping (const generic_mapper_for_target_node<Obj> &m1, const generic_mapper_for_target_node<Obj> &m2, size_t n1, size_t n2, std::vector<std::pair<const Obj *, const Obj *> > &mapped)
  {
    if (m1.empty () || m2.empty ()) {
      return;
    }

    const std::set<std::pair<CatAndIds, const Obj *> > &s1 = m1.for_node (n1);
    const std::set<std::pair<CatAndIds, const Obj *> > &s2 = m2.for_node (n2);

    typename std::set<std::pair<CatAndIds, const Obj *> >::const_iterator i1 = s1.begin (), i2 = s2.begin ();

    while (i1 != s1.end () && i2 != s2.end ()) {

      if (i1->first < i2->first) {
        ++i1;
      } else if (i2->first < i1->first) {
        ++i2;
      } else {
        typename std::set<std::pair<CatAndIds, const Obj *> >::const_iterator i10 = i1, i20 = i2;
        size_t n1 = 0, n2 = 0;
        while (i1 != s1.end () && i1->first == i10->first) {
          ++i1;
          ++n1;
        }
        while (i2 != s2.end () && i2->first == i20->first) {
          ++i2;
          ++n2;
        }
        if (n1 == 1 && n2 == 1) {
          //  unique mapping - one device of one category
          mapped.push_back (std::make_pair (i10->second, i20->second));
        }
      }

    }

  }

protected:
  const std::set<std::pair<CatAndIds, const Obj *> > &for_node (size_t ni) const
  {
    typename std::map<size_t, std::set<std::pair<CatAndIds, const Obj *> > >::const_iterator d = m_per_target_node.find (ni);
    tl_assert (d != m_per_target_node.end ());
    return d->second;
  }

  std::set<std::pair<CatAndIds, const Obj *> > &for_node_nc (size_t ni)
  {
    return m_per_target_node [ni];
  }

  bool empty () const
  {
    return m_per_target_node.empty ();
  }

private:
  std::map<size_t, std::set<std::pair<CatAndIds, const Obj *> > > m_per_target_node;
};

class DeviceMapperForTargetNode
  : public generic_mapper_for_target_node<db::Device>
{
public:
  DeviceMapperForTargetNode ()
    : generic_mapper_for_target_node<db::Device> ()
  {
    //  .. nothing yet ..
  }

  void insert (const NetGraphNode::edge_type &e)
  {
    if (e.first.empty ()) {
      //  happens initially
      return;
    }

    size_t ni = e.second.first;
    std::set<std::pair<CatAndIds, const Device *> > &dev = for_node_nc (ni);
    for (std::vector<NetGraphNode::Transition>::const_iterator j = e.first.begin (); j != e.first.end (); ++j) {
      if (! j->is_for_subcircuit ()) {
        dev.insert (std::make_pair (CatAndIds (j->device_pair ().second, j->id1 (), j->id2 ()), j->device_pair ().first));
      }
    }
  }
};

class SubCircuitMapperForTargetNode
  : public generic_mapper_for_target_node<db::SubCircuit>
{
public:
  SubCircuitMapperForTargetNode ()
    : generic_mapper_for_target_node<db::SubCircuit> ()
  {
    //  .. nothing yet ..
  }

  void insert (const NetGraphNode::edge_type &e)
  {
    if (e.first.empty ()) {
      //  happens initially
      return;
    }

    size_t ni = e.second.first;
    std::set<std::pair<CatAndIds, const SubCircuit *> > &sc = for_node_nc (ni);
    for (std::vector<NetGraphNode::Transition>::const_iterator j = e.first.begin (); j != e.first.end (); ++j) {
      if (j->is_for_subcircuit ()) {
        sc.insert (std::make_pair (CatAndIds (j->subcircuit_pair ().second, j->id1 (), j->id2 ()), j->subcircuit_pair ().first));
      }
    }
  }
};

/**
 *  @brief An audit object which allows reverting tentative node assignments
 */
class TentativeNodeMapping
{
public:
  TentativeNodeMapping ()
  { }

  ~TentativeNodeMapping ()
  {
    for (std::vector<std::pair<NetGraph *, size_t> >::const_iterator i = m_to_undo.begin (); i != m_to_undo.end (); ++i) {
      i->first->unidentify (i->second);
    }
    for (std::vector<std::pair<NetGraph *, size_t> >::const_iterator i = m_to_undo_to_unknown.begin (); i != m_to_undo_to_unknown.end (); ++i) {
      i->first->identify (i->second, unknown_id);
    }
    for (std::vector<std::pair<DeviceEquivalenceTracker *, std::pair<const db::Device *, const db::Device *> > >::const_iterator i = m_to_undo_devices.begin (); i != m_to_undo_devices.end (); ++i) {
      i->first->unmap (i->second.first, i->second.second);
    }
    for (std::vector<std::pair<SubCircuitEquivalenceTracker *, std::pair<const db::SubCircuit *, const db::SubCircuit *> > >::const_iterator i = m_to_undo_subcircuits.begin (); i != m_to_undo_subcircuits.end (); ++i) {
      i->first->unmap (i->second.first, i->second.second);
    }
  }

  static void map_pair (TentativeNodeMapping *nm, NetGraph *g1, size_t n1, NetGraph *g2, size_t n2,
                        const DeviceMapperForTargetNode &dm1, const DeviceMapperForTargetNode &dm2, DeviceEquivalenceTracker &device_eq,
                        const SubCircuitMapperForTargetNode &scm1, const SubCircuitMapperForTargetNode &scm2, SubCircuitEquivalenceTracker &subcircuit_eq,
                        size_t depth)
  {
    g1->identify (n1, n2);
    g2->identify (n2, n1);

    if (nm) {
      nm->keep (g1, n1);
      nm->keep (g2, n2);
    }

    derive_device_equivalence (nm, n1, n2, dm1, dm2, device_eq, depth);
    derive_subcircuit_equivalence (nm, n1, n2, scm1, scm2, subcircuit_eq, depth);
  }

  static void map_pair_from_unknown (TentativeNodeMapping *nm, NetGraph *g1, size_t n1, NetGraph *g2, size_t n2,
                                     const DeviceMapperForTargetNode &dm1, const DeviceMapperForTargetNode &dm2, DeviceEquivalenceTracker &device_eq,
                                     const SubCircuitMapperForTargetNode &scm1, const SubCircuitMapperForTargetNode &scm2, SubCircuitEquivalenceTracker &subcircuit_eq,
                                     size_t depth)
  {
    g1->identify (n1, n2);
    g2->identify (n2, n1);

    if (nm) {
      nm->keep_for_unknown (g1, n1);
      nm->keep_for_unknown (g2, n2);
    }

    derive_device_equivalence (nm, n1, n2, dm1, dm2, device_eq, depth);
    derive_subcircuit_equivalence (nm, n1, n2, scm1, scm2, subcircuit_eq, depth);
  }

  static void map_to_unknown (TentativeNodeMapping *nm, NetGraph *g1, size_t n1)
  {
    g1->identify (n1, unknown_id);
    if (nm) {
      nm->keep (g1, n1);
    }
  }

  static void derive_device_equivalence (TentativeNodeMapping *nm, size_t n1, size_t n2,
                                         const DeviceMapperForTargetNode &dm1, const DeviceMapperForTargetNode &dm2, DeviceEquivalenceTracker &device_eq, size_t depth)
  {
    std::vector<std::pair<const db::Device *, const db::Device *> > device_map;
    DeviceMapperForTargetNode::derive_mapping (dm1, dm2, n1, n2, device_map);

    for (std::vector<std::pair<const db::Device *, const db::Device *> >::const_iterator dd = device_map.begin (); dd != device_map.end (); ++dd) {
      if (device_eq.map (dd->first, dd->second)) {
        if (nm) {
          nm->keep (&device_eq, dd->first, dd->second);
        } else {
          if (options ()->debug_netcompare) {
            tl::info << indent(depth) << "enforcing device equivalence: " << dd->first->expanded_name () << " vs. " << dd->second->expanded_name ();
          }
        }
      }
    }
  }

  static void derive_subcircuit_equivalence (TentativeNodeMapping *nm, size_t n1, size_t n2,
                                             const SubCircuitMapperForTargetNode &scm1, const SubCircuitMapperForTargetNode &scm2, SubCircuitEquivalenceTracker &subcircuit_eq, size_t depth)
  {
    std::vector<std::pair<const db::SubCircuit *, const db::SubCircuit *> > subcircuit_map;
    SubCircuitMapperForTargetNode::derive_mapping (scm1, scm2, n1, n2, subcircuit_map);

    for (std::vector<std::pair<const db::SubCircuit *, const db::SubCircuit *> >::const_iterator cc = subcircuit_map.begin (); cc != subcircuit_map.end (); ++cc) {
      if (subcircuit_eq.map (cc->first, cc->second)) {
        if (nm) {
          nm->keep (&subcircuit_eq, cc->first, cc->second);
        } else {
          if (options ()->debug_netcompare) {
            tl::info << indent(depth) << "enforcing subcircuit equivalence: " << cc->first->expanded_name () << " vs. " << cc->second->expanded_name ();
          }
        }
      }
    }
  }

private:
  std::vector<std::pair<NetGraph *, size_t> > m_to_undo, m_to_undo_to_unknown;
  std::vector<std::pair<DeviceEquivalenceTracker *, std::pair<const db::Device *, const db::Device *> > > m_to_undo_devices;
  std::vector<std::pair<SubCircuitEquivalenceTracker *, std::pair<const db::SubCircuit *, const db::SubCircuit *> > > m_to_undo_subcircuits;

  void keep (NetGraph *g1, size_t n1)
  {
    m_to_undo.push_back (std::make_pair (g1, n1));
  }

  void keep_for_unknown (NetGraph *g1, size_t n1)
  {
    m_to_undo_to_unknown.push_back (std::make_pair (g1, n1));
  }

  void keep (DeviceEquivalenceTracker *dt, const db::Device *a, const db::Device *b)
  {
    m_to_undo_devices.push_back (std::make_pair (dt, std::make_pair (a, b)));
  }

  void keep (SubCircuitEquivalenceTracker *dt, const db::SubCircuit *a, const db::SubCircuit *b)
  {
    m_to_undo_subcircuits.push_back (std::make_pair (dt, std::make_pair (a, b)));
  }
};

// --------------------------------------------------------------------------------------------------------------------
//  NetGraph implementation

void
NetGraph::build (const db::Circuit *c, DeviceCategorizer &device_categorizer, CircuitCategorizer &circuit_categorizer, const db::DeviceFilter &device_filter, const std::map<const db::Circuit *, CircuitMapper> *circuit_and_pin_mapping, const CircuitPinMapper *circuit_pin_mapper)
{
  tl::SelfTimer timer (tl::verbosity () >= 31, tl::to_string (tr ("Building net graph for circuit: ")) + c->name ());

  mp_circuit = c;

  m_nodes.clear ();
  m_net_index.clear ();

  //  create a dummy node for a null net
  m_nodes.push_back (NetGraphNode (0, device_categorizer, circuit_categorizer, device_filter, circuit_and_pin_mapping, circuit_pin_mapper));

  size_t nets = 0;
  for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
    ++nets;
  }
  m_nodes.reserve (nets);

  for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
    NetGraphNode node (n.operator-> (), device_categorizer, circuit_categorizer, device_filter, circuit_and_pin_mapping, circuit_pin_mapper);
    if (! node.empty () || n->pin_count () > 0) {
      m_nodes.push_back (node);
    }
  }

  for (std::vector<NetGraphNode>::const_iterator i = m_nodes.begin (); i != m_nodes.end (); ++i) {
    m_net_index.insert (std::make_pair (i->net (), i - m_nodes.begin ()));
  }

  for (std::vector<NetGraphNode>::iterator i = m_nodes.begin (); i != m_nodes.end (); ++i) {
    i->apply_net_index (m_net_index);
  }

  if (options ()->debug_netgraph) {
    for (std::vector<NetGraphNode>::iterator i = m_nodes.begin (); i != m_nodes.end (); ++i) {
      tl::info << i->to_string () << tl::noendl;
    }
  }

  //  create subcircuit virtual nodes

  for (db::Circuit::const_subcircuit_iterator i = c->begin_subcircuits (); i != c->end_subcircuits (); ++i) {

    size_t circuit_cat = circuit_categorizer.cat_for_subcircuit (i.operator-> ());
    if (! circuit_cat) {
      continue;
    }

    const db::Circuit *cr = i->circuit_ref ();
    std::map<const db::Circuit *, CircuitMapper>::const_iterator icm = circuit_and_pin_mapping->find (cr);
    if (icm == circuit_and_pin_mapping->end ()) {
      continue;
    }

    m_virtual_nodes.insert (std::make_pair (i.operator-> (), NetGraphNode (i.operator-> (), circuit_categorizer, circuit_and_pin_mapping, circuit_pin_mapper)));

  }

  for (std::map<const db::SubCircuit *, NetGraphNode>::iterator i = m_virtual_nodes.begin (); i != m_virtual_nodes.end (); ++i) {
    i->second.apply_net_index (m_net_index);
  }

  if (options ()->debug_netgraph) {
    for (std::map<const db::SubCircuit *, NetGraphNode>::iterator i = m_virtual_nodes.begin (); i != m_virtual_nodes.end (); ++i) {
      tl::info << i->second.to_string () << tl::noendl;
    }
  }
}

/**
 *  @brief Returns true if the edges (given by transition iterators) are compatible with already established device or subcircuit equivalences.
 */
static bool edges_are_compatible (const NetGraphNode::edge_type &e, const NetGraphNode::edge_type &e_other, const DeviceEquivalenceTracker &device_eq, const SubCircuitEquivalenceTracker &sc_eq)
{
  std::vector<NetGraphNode::Transition>::const_iterator t1 = e.first.begin (), tt1 = e.first.end ();
  std::vector<NetGraphNode::Transition>::const_iterator t2 = e_other.first.begin (), tt2 = e_other.first.end ();

  std::vector<void *> p1, p2;

  while (t1 != tt1 && t2 != tt2) {

    std::vector<NetGraphNode::Transition>::const_iterator t10 = t1, t20 = t2;

    p1.clear ();
    while (t1 != tt1 && *t1 == *t10) {
      if (t1->is_for_subcircuit ()) {
        p1.push_back ((void *) sc_eq.other (t1->subcircuit_pair ().first));
      } else {
        p1.push_back ((void *) device_eq.other (t1->device_pair ().first));
      }
      ++t1;
    }

    p2.clear ();
    while (t2 != tt2 && *t2 == *t20) {
      if (t2->is_for_subcircuit ()) {
        p2.push_back ((void *) (sc_eq.other (t2->subcircuit_pair ().first) ? t2->subcircuit_pair ().first : 0));
      } else {
        p2.push_back ((void *) (device_eq.other (t2->device_pair ().first) ? t2->device_pair ().first : 0));
      }
      ++t2;
    }

    std::sort (p1.begin (), p1.end ());
    std::sort (p2.begin (), p2.end ());

    if (p1 != p2) {
      return false;
    }

  }

  tl_assert (t1 == tt1 && t2 == tt2);
  return true;
}

size_t
NetGraph::derive_node_identities_for_edges (NetGraphNode::edge_iterator e, NetGraphNode::edge_iterator ee, NetGraphNode::edge_iterator e_other, NetGraphNode::edge_iterator ee_other, size_t net_index, size_t other_net_index, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, CompareData *data)
{
  //  NOTE: we can skip edges to known nodes because we did a pre-analysis making sure those are compatible

  std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> > nodes;
  nodes.reserve (ee - e);

  std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> > other_nodes;
  other_nodes.reserve (ee - e);

  tl_assert (e->first == e_other->first);

  for (NetGraphNode::edge_iterator i = e; i != ee; ++i) {
    if (i->second.first != net_index) {
      const NetGraphNode *nn = &node (i->second.first);
      if (! nn->has_other ()) {
        nodes.push_back (std::make_pair (nn, i));
      }
    }
  }

  if (! nodes.empty ()) {   //  if non-ambiguous, non-assigned

    for (NetGraphNode::edge_iterator i = e_other; i != ee_other; ++i) {
      if (i->second.first != other_net_index) {
        const NetGraphNode *nn = &data->other->node (i->second.first);
        if (! nn->has_other ()) {
          other_nodes.push_back (std::make_pair (nn, i));
        }
      }
    }

  }

  if (nodes.empty () || other_nodes.empty ()) {
    return 0;
  }

  std::sort (nodes.begin (), nodes.end (), CompareNodePtr ());
  std::sort (other_nodes.begin (), other_nodes.end (), CompareNodePtr ());

  size_t new_nodes = 0;

  if (options ()->debug_netcompare) {

    //  print transitions if requested

    tl::info << indent(depth) << "considering transitions:";

    bool first = true;

    for (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i = nodes.begin (); i != nodes.end (); ++i) {
      const NetGraphNode *nn = i->first;
      if (first) {
        tl::info << indent (depth) << "  here: " << (node (net_index).net () ? node (net_index).net ()->expanded_name ().c_str () : "(null)") << " ->";
        first = false;
      }
      tl::info << indent (depth) << "    " << (nn->net () ? nn->net ()->expanded_name ().c_str() : "(null)") << " via: " << tl::noendl;
      for (std::vector<NetGraphNode::Transition>::const_iterator t = i->second->first.begin (); t != i->second->first.end(); ++t) {
        tl::info << (t != i->second->first.begin () ? "; " : "") << t->to_string() << tl::noendl;
      }
      tl::info << "";
    }

    first = true;

    for (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i = other_nodes.begin (); i != other_nodes.end (); ++i) {
      const NetGraphNode *nn = i->first;
      if (first) {
        tl::info << indent (depth) << "  there: " << (data->other->node (other_net_index).net () ? data->other->node (other_net_index).net ()->expanded_name ().c_str () : "(null)") << " ->";
        first = false;
      }
      tl::info << indent(depth) << "    " << (nn->net() ? nn->net()->expanded_name().c_str() : "(null)") << " via: " << tl::noendl;
      for (std::vector<NetGraphNode::Transition>::const_iterator t = i->second->first.begin (); t != i->second->first.end(); ++t) {
        tl::info << (t != i->second->first.begin () ? "; " : "") << t->to_string() << tl::noendl;
      }
      tl::info << "";
    }

  }

  //  for the purpose of match evaluation we require an exact match of the node structure

  if (tentative) {

    if (nodes.size () != other_nodes.size ()) {
      if (options ()->debug_netcompare) {
        tl::info << indent(depth) << "=> rejected branch.";
      }
      return failed_match;
    }

    //  1:1 pairing is less strict
    if (nodes.size () > 1 || other_nodes.size () > 1) {
      for (size_t i = 0; i < nodes.size (); ++i) {
        if (! (*nodes[i].first == *other_nodes[i].first)) {
          if (options ()->debug_netcompare) {
            tl::info << indent(depth) << "=> rejected branch.";
          }
          return failed_match;
        }
      }
    }

  }

  //  propagate pairing in picky mode: this means we only accept a match if the node set
  //  is exactly identical and no ambiguous nodes are present when ambiguous nodes are forbidden

  size_t bt_count = derive_node_identities_from_node_set (nodes, other_nodes, depth, n_branch, tentative, data);

  if (bt_count == failed_match) {
    if (tentative) {
      if (options ()->debug_netcompare) {
        tl::info << indent(depth) << "=> rejected branch.";
      }
      return bt_count;
    }
  } else {
    new_nodes += bt_count;
  }

  if (options ()->debug_netcompare) {
    if (! new_nodes) {
      tl::info << indent(depth) << "=> no updates.";
    }
  }
  return new_nodes;
}

static bool has_subcircuits (db::NetGraphNode::edge_iterator e, db::NetGraphNode::edge_iterator ee)
{
  while (e != ee) {
    for (std::vector<NetGraphNode::Transition>::const_iterator t = e->first.begin (); t != e->first.end (); ++t) {
      if (t->is_for_subcircuit ()) {
        return true;
      }
    }
    ++e;
  }
  return false;
}

size_t
NetGraph::derive_node_identities (size_t net_index, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, CompareData *data)
{
  NetGraphNode *n = & node (net_index);

  size_t other_net_index = n->other_net_index ();
  NetGraphNode *n_other = & data->other->node (other_net_index);

  NetGraphNode nn, nn_other;

  //  If there are subcircuits on the graph we temporarily create edges from our node to the other nodes of
  //  the subcircuit. This way we don't need to keep #pin*(#pin-1) edges

  if (has_subcircuits (n->begin (), n->end ())) {

    nn = *n;
    nn.expand_subcircuit_nodes (this);
    n = &nn;

    nn_other = *n_other;
    nn_other.expand_subcircuit_nodes (data->other);
    n_other = &nn_other;

  }

  //  do a pre-analysis filtering out all nodes with fully satisfied edges or which provide a contradiction

  bool analysis_required = false;

  for (NetGraphNode::edge_iterator e = n->begin (); e != n->end (); ) {

    NetGraphNode::edge_iterator ee = e;
    ++ee;

    while (ee != n->end () && ee->first == e->first) {
      ++ee;
    }

    NetGraphNode::edge_iterator e_other = n_other->find_edge (e->first);
    if (e_other != n_other->end ()) {

      NetGraphNode::edge_iterator ee_other = e_other;
      ++ee_other;

      while (ee_other != n_other->end () && ee_other->first == e_other->first) {
        ++ee_other;
      }

      std::vector<const NetGraphNode *> nodes;
      nodes.reserve (ee - e);

      std::vector<const NetGraphNode *> other_nodes_translated;
      other_nodes_translated.reserve (ee - e);

      tl_assert (e->first == e_other->first);

      for (NetGraphNode::edge_iterator i = e; i != ee; ++i) {
        if (i->second.first != net_index) {
          const NetGraphNode *nn = &node (i->second.first);
          if (nn->has_other ()) {
            nodes.push_back (nn);
          } else {
            analysis_required = true;
          }
        }
      }

      for (NetGraphNode::edge_iterator i = e_other; i != ee_other; ++i) {
        if (i->second.first != other_net_index) {
          const NetGraphNode *nn = &data->other->node (i->second.first);
          if (nn->has_other ()) {
            other_nodes_translated.push_back (&node (nn->other_net_index ()));
          } else {
            analysis_required = true;
          }
        }
      }

      std::sort (nodes.begin (), nodes.end ());
      std::sort (other_nodes_translated.begin (), other_nodes_translated.end ());

      //  No fit, we can shortcut
      if (nodes != other_nodes_translated) {
        return tentative ? failed_match : 0;
      }

    } else if (tentative) {
      //  in tentative mode an exact match is required: no having the same edges for a node disqualifies the node
      //  as matching.
      return failed_match;
    }

    e = ee;

  }

  if (tentative) {

    //  in tentative mode, again an exact match is required

    for (NetGraphNode::edge_iterator e_other = n_other->begin (); e_other != n_other->end (); ) {

      NetGraphNode::edge_iterator ee_other = e_other;
      ++ee_other;

      while (ee_other != n_other->end () && ee_other->first == e_other->first) {
        ++ee_other;
      }

      NetGraphNode::edge_iterator e = n->find_edge (e_other->first);
      if (e == n->end ()) {
        return failed_match;
      }

      e_other = ee_other;

    }

  }

  if (! analysis_required) {
    return 0;
  }

  //  do a detailed analysis

  size_t new_nodes = 0;

  if (options ()->debug_netcompare) {
    if (! tentative) {
      tl::info << indent(depth) << "deducing from pair: " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
    } else {
      tl::info << indent(depth) << "tentatively deducing from pair: " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
    }
  }

  //  non-ambiguous paths to non-assigned nodes create a node identity on the
  //  end of this path

  for (NetGraphNode::edge_iterator e = n->begin (); e != n->end (); ) {

    NetGraphNode::edge_iterator ee = e;
    ++ee;

    while (ee != n->end () && ee->first == e->first) {
      ++ee;
    }

    NetGraphNode::edge_iterator e_other = n_other->find_edge (e->first);
    if (e_other != n_other->end ()) {

      NetGraphNode::edge_iterator ee_other = e_other;
      ++ee_other;

      while (ee_other != n_other->end () && ee_other->first == e_other->first) {
        ++ee_other;
      }

      size_t bt_count = derive_node_identities_for_edges (e, ee, e_other, ee_other, net_index, other_net_index, depth, n_branch, tentative, data);
      if (bt_count == failed_match) {
        if (options ()->debug_netcompare) {
          tl::info << indent(depth) << "=> rejected pair.";
        }
        return bt_count;
      } else {
        new_nodes += bt_count;
      }

    }

    e = ee;

  }

  if (options ()->debug_netcompare) {
    if (! tentative && new_nodes > 0) {
      tl::info << indent(depth) << "=> finished pair deduction: " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name () << " with " << new_nodes << " new pairs";
    }
  }

  return new_nodes;
}

namespace {

  struct SortNodeByNet
  {
    public:
      bool operator() (const std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> &a, const std::pair<const NetGraphNode *, NetGraphNode::edge_iterator>b) const
      {
        tl_assert (a.first->net () && b.first->net ());
        return name_compare (a.first->net (), b.first->net ()) < 0;
      }
  };

}

static void sort_node_range_by_best_match (const NodeRange &nr)
{
  std::stable_sort (nr.n1, nr.nn1, SortNodeByNet ());
  std::stable_sort (nr.n2, nr.nn2, SortNodeByNet ());

  std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> > nomatch1, nomatch2;
  nomatch1.reserve (nr.nn1 - nr.n1);
  nomatch2.reserve (nr.nn2 - nr.n2);

  std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i = nr.n1, j = nr.n2;
  std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::iterator iw = nr.n1, jw = nr.n2;

  SortNodeByNet compare;

  while (i != nr.nn1 || j != nr.nn2) {
    if (j == nr.nn2) {
      nomatch1.push_back (*i);
      ++i;
    } else if (i == nr.nn1) {
      nomatch2.push_back (*j);
      ++j;
    } else if (compare (*i, *j)) {
      nomatch1.push_back (*i);
      ++i;
    } else if (compare (*j, *i)) {
      nomatch2.push_back (*j);
      ++j;
    } else {
      if (iw != i) {
        *iw = *i;
      }
      ++iw, ++i;
      if (jw != j) {
        *jw = *j;
      }
      ++jw, ++j;
    }
  }

  tl_assert (iw + nomatch1.size () == nr.nn1);
  tl_assert (jw + nomatch2.size () == nr.nn2);

  for (i = nomatch1.begin (); i != nomatch1.end (); ++i) {
    *iw++ = *i;
  }
  for (j = nomatch2.begin (); j != nomatch2.end (); ++j) {
    *jw++ = *j;
  }
}

static bool net_names_are_different (const db::Net *a, const db::Net *b)
{
  if (! a || ! b || a->name ().empty () || b->name ().empty ()) {
    return false;
  } else {
    return name_compare (a, b) != 0;
  }
}

static bool net_names_are_equal (const db::Net *a, const db::Net *b)
{
  if (! a || ! b || a->name ().empty () || b->name ().empty ()) {
    return false;
  } else {
    return name_compare (a, b) == 0;
  }
}

size_t
NetGraph::derive_node_identities_from_ambiguity_group (const NodeRange &nr, DeviceMapperForTargetNode &dm, DeviceMapperForTargetNode &dm_other, SubCircuitMapperForTargetNode &scm, SubCircuitMapperForTargetNode &scm_other, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, CompareData *data)
{
  tl::AbsoluteProgress progress (tl::to_string (tr ("Deriving match for ambiguous net group")));

  std::string indent_s;
  if (options ()->debug_netcompare) {
    indent_s = indent (depth);
    indent_s += "*" + tl::to_string (n_branch) + " ";
  }

  size_t new_nodes = 0;
  size_t complexity = nr.num;

  //  sort the ambiguity group such that net names match best

  std::vector<std::pair<const NetGraphNode *, const NetGraphNode *> > pairs;
  tl::equivalence_clusters<const NetGraphNode *> equivalent_other_nodes;

  sort_node_range_by_best_match (nr);

  {

    //  marks the nodes from the ambiguity group as unknown so we don't revisit them (causing deep recursion)
    TentativeNodeMapping tn_temp;

    //  collect and mark the ambiguity combinations to consider
    std::vector<std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator> iters1, iters2;

    for (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i1 = nr.n1; i1 != nr.nn1; ++i1) {
      if (! i1->first->has_any_other ()) {
        iters1.push_back (i1);
        size_t ni = node_index_for_net (i1->first->net ());
        TentativeNodeMapping::map_to_unknown (&tn_temp, this, ni);
      }
    }

    for (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i2 = nr.n2; i2 != nr.nn2; ++i2) {
      if (! i2->first->has_any_other ()) {
        iters2.push_back (i2);
        size_t other_ni = data->other->node_index_for_net (i2->first->net ());
        TentativeNodeMapping::map_to_unknown (&tn_temp, data->other, other_ni);
      }
    }

    for (std::vector<std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator>::const_iterator ii1 = iters1.begin (); ii1 != iters1.end (); ++ii1) {

      std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i1 = *ii1;

      //  in tentative mode, reject this choice if nets are named and all other nets in the ambiguity group differ -> this favors net matching by name
      if (! data->dont_consider_net_names && tentative) {

        bool any_matching = false;
        for (std::vector<std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator>::iterator ii2 = iters2.begin (); ii2 != iters2.end () && ! any_matching; ++ii2) {
          std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i2 = *ii2;
          any_matching = !net_names_are_different (i1->first->net (), i2->first->net ());
        }

        if (! any_matching) {
          if (options ()->debug_netcompare) {
            tl::info << indent_s << "ambiguity group rejected - all ambiguous other net names are mismatching for: " << i1->first->net ()->expanded_name ();
          }
          //  a mismatch - stop here.
          return failed_match;
        }

      }

      bool any = false;
      std::vector<std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator>::iterator to_remove = iters2.end ();

      for (std::vector<std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator>::iterator ii2 = iters2.begin (); ii2 != iters2.end (); ++ii2) {

        ++progress;

        std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i2 = *ii2;

        //  try this candidate in tentative mode
        if (options ()->debug_netcompare) {
          tl::info << indent_s << "trying in tentative mode: " << i1->first->net ()->expanded_name () << " vs. " << i2->first->net ()->expanded_name ();
        }

        if (! edges_are_compatible (*i1->second, *i2->second, *data->device_equivalence, *data->subcircuit_equivalence)) {
          if (options ()->debug_netcompare) {
            tl::info << indent_s << "=> rejected because edges are incompatible with already established device or subcircuit equivalences";
          }
          continue;
        }

        if (! data->dont_consider_net_names && net_names_are_equal (i1->first->net (), i2->first->net ())) {

          if (options ()->debug_netcompare) {
            tl::info << indent_s << "=> accepted for identical names";
          }

          //  utilize net names to propose a match
          new_nodes += 1;
          pairs.push_back (std::make_pair (i1->first, i2->first));
          to_remove = ii2;
          any = true;
          break;

        } else {

          size_t ni = node_index_for_net (i1->first->net ());
          size_t other_ni = data->other->node_index_for_net (i2->first->net ());

          TentativeNodeMapping tn;
          TentativeNodeMapping::map_pair_from_unknown (&tn, this, ni, data->other, other_ni, dm, dm_other, *data->device_equivalence, scm, scm_other, *data->subcircuit_equivalence, depth);

          size_t bt_count = derive_node_identities (ni, depth + 1, complexity * n_branch, &tn, data);

          if (bt_count != failed_match) {

            if (options ()->debug_netcompare) {
              tl::info << indent_s << "match found";
            }
            //  we have a match ...

            if (any) {

              //  there is already a known pair, so we can mark *i2 and the previous *i2 as equivalent
              //  (makes them ambiguous)
              equivalent_other_nodes.same (i2->first, pairs.back ().second);
              //  we know enough now ...
              break;

            } else {

              //  identified a new pair
              new_nodes += bt_count + 1;
              pairs.push_back (std::make_pair (i1->first, i2->first));
              to_remove = ii2;
              any = true;

              //  no ambiguity analysis in tentative mode - we can stop now
              if (tentative) {
                break;
              }

            }

          }

        }

      }

      if (to_remove != iters2.end ()) {

        //  Add the new pair to the temporary mapping (even in tentative mode)
        //  Reasoning: doing the mapping may render other nets incompatible, so to ensure "edges_are_compatible" works properly we
        //  need to lock the current pairs resources such as devices by listing them in the mapping. This is doing by "derive_*_equivalence" inside
        //  TentativeNodeMapping::map_pair

        std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i2 = *to_remove;

        size_t ni = node_index_for_net (i1->first->net ());
        size_t other_ni = data->other->node_index_for_net (i2->first->net ());

        TentativeNodeMapping::map_pair (&tn_temp, this, ni, data->other, other_ni, dm, dm_other, *data->device_equivalence, scm, scm_other, *data->subcircuit_equivalence, depth);

        //  now we can get rid of the node and reduce the "other" list of ambiguous nodes
        iters2.erase (to_remove);

      }

      if (! any && tentative) {
        if (options ()->debug_netcompare) {
          tl::info << indent_s << "mismatch.";
        }
        //  a mismatch - stop here.
        return failed_match;
      }

    }

  }

  if (! tentative) {

    //  issue the matching pairs

    for (std::vector<std::pair<const NetGraphNode *, const NetGraphNode *> >::const_iterator p = pairs.begin (); p != pairs.end (); ++p) {

      size_t ni = node_index_for_net (p->first->net ());
      size_t other_ni = data->other->node_index_for_net (p->second->net ());

      TentativeNodeMapping::map_pair (0, this, ni, data->other, other_ni, dm, dm_other, *data->device_equivalence, scm, scm_other, *data->subcircuit_equivalence, depth);

      if (options ()->debug_netcompare) {
        if (equivalent_other_nodes.has_attribute (p->second)) {
          tl::info << indent_s << "deduced ambiguous match: " << p->first->net ()->expanded_name () << " vs. " << p->second->net ()->expanded_name ();
        } else {
          tl::info << indent_s << "deduced match: " << p->first->net ()->expanded_name () << " vs. " << p->second->net ()->expanded_name ();
        }
      }

      if (data->logger) {
        bool ambiguous = equivalent_other_nodes.has_attribute (p->second);
        if (ambiguous) {
          data->logger->match_ambiguous_nets (p->first->net (), p->second->net ());
        } else {
          data->logger->match_nets (p->first->net (), p->second->net ());
        }
      }

      ++*data->progress;

    }

    //  And seek further from these pairs

    for (std::vector<std::pair<const NetGraphNode *, const NetGraphNode *> >::const_iterator p = pairs.begin (); p != pairs.end (); ++p) {

      size_t ni = node_index_for_net (p->first->net ());

      size_t bt_count = derive_node_identities (ni, depth + 1, complexity * n_branch, tentative, data);
      tl_assert (bt_count != failed_match);

    }

  } else {

    for (std::vector<std::pair<const NetGraphNode *, const NetGraphNode *> >::const_iterator p = pairs.begin (); p != pairs.end (); ++p) {

      size_t ni = node_index_for_net (p->first->net ());
      size_t other_ni = data->other->node_index_for_net (p->second->net ());

      TentativeNodeMapping::map_pair (tentative, this, ni, data->other, other_ni, dm, dm_other, *data->device_equivalence, scm, scm_other, *data->subcircuit_equivalence, depth);

    }

  }

  return new_nodes;
}

size_t
NetGraph::derive_node_identities_from_singular_match (const NetGraphNode *n, const NetGraphNode::edge_iterator &e, const NetGraphNode *n_other, const NetGraphNode::edge_iterator &e_other, DeviceMapperForTargetNode &dm, DeviceMapperForTargetNode &dm_other, SubCircuitMapperForTargetNode &scm, SubCircuitMapperForTargetNode &scm_other, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, CompareData *data, bool consider_net_names)
{
  std::string indent_s;
  if (options ()->debug_netcompare) {
    indent_s = indent (depth);
    indent_s += "*" + tl::to_string (n_branch) + " ";
  }

  if (! edges_are_compatible (*e, *e_other, *data->device_equivalence, *data->subcircuit_equivalence)) {

    if (options ()->debug_netcompare) {
      tl::info << indent(depth) << "=> rejected because edges are incompatible with already established device or subcircuit equivalences";
    }
    return tentative ? failed_match : 0;

  } else if (! n->has_any_other () && ! n_other->has_any_other ()) {

    //  in tentative mode, reject this choice if both nets are named and
    //  their names differ -> this favors net matching by name

    if (tentative && consider_net_names && net_names_are_different (n->net (), n_other->net ())) {
      if (options ()->debug_netcompare) {
        tl::info << indent_s << "rejecting pair as names are not identical: " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
      }
      return failed_match;
    }

    //  A single candiate: just take this one -> this may render
    //  inexact matches, but further propagates net pairing

    size_t ni = node_index_for_net (n->net ());
    size_t other_ni = data->other->node_index_for_net (n_other->net ());

    TentativeNodeMapping::map_pair (tentative, this, ni, data->other, other_ni, dm, dm_other, *data->device_equivalence, scm, scm_other, *data->subcircuit_equivalence, depth);

    if (options ()->debug_netcompare) {
      tl::info << indent_s << "deduced match (singular): " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
    }
    if (! tentative) {
      ++*data->progress;
      if (data->logger) {
        if (! (node (ni) == data->other->node (other_ni))) {
          //  this is a mismatch, but we continue with this
          data->logger->net_mismatch (n->net (), n_other->net ());
        } else {
          data->logger->match_nets (n->net (), n_other->net ());
        }
      }
    }

    size_t new_nodes = 1;

    if (data->depth_first || tentative) {
      size_t bt_count = derive_node_identities (ni, depth + 1, n_branch, tentative, data);
      if (bt_count == failed_match) {
        if (tentative) {
          return failed_match;
        }
      } else {
        new_nodes += bt_count;
      }
    }

    return new_nodes;

  } else if (n->has_unknown_other ()) {

    //  accept any other net
    return 0;

  } else if (n->has_other ()) {

    //  this decision leads to a contradiction
    if (data->other->node_index_for_net (n_other->net ()) != n->other_net_index ()) {
      return failed_match;
    } else {
      return 0;
    }

  } else {

    //  mismatch of assignment state
    return failed_match;

  }
}

size_t
NetGraph::derive_node_identities_from_node_set (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> > &nodes, std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> > &other_nodes, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, CompareData *data)
{
  std::string indent_s;
  if (options ()->debug_netcompare) {
    indent_s = indent (depth);
    indent_s += "*" + tl::to_string (n_branch) + " ";
  }

  if (data->max_depth != std::numeric_limits<size_t>::max() && depth > data->max_depth) {
    if (options ()->debug_netcompare) {
      tl::info << indent_s << "max. depth exhausted (" << depth + 1 << ">" << data->max_depth << ")";
    }
    return failed_match;
  }

  DeviceMapperForTargetNode dm;
  SubCircuitMapperForTargetNode scm;
  for (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i = nodes.begin (); i != nodes.end (); ++i) {
    dm.insert (*i->second);
    scm.insert (*i->second);
  }

  DeviceMapperForTargetNode dm_other;
  SubCircuitMapperForTargetNode scm_other;
  for (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i = other_nodes.begin (); i != other_nodes.end (); ++i) {
    dm_other.insert (*i->second);
    scm_other.insert (*i->second);
  }

  if (nodes.size () == 1 && other_nodes.size () == 1) {

    return derive_node_identities_from_singular_match (nodes.front ().first, nodes.front ().second, other_nodes.front ().first, other_nodes.front ().second,
                                                       dm, dm_other, scm, scm_other, depth, n_branch, tentative, data, false /*don't consider net names*/);

  }

  //  Determine the range of nodes with same identity

  std::vector<NodeRange> node_ranges;
  size_t new_nodes = 0;

  std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::iterator n1 = nodes.begin ();
  std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::iterator n2 = other_nodes.begin ();

  while (n1 != nodes.end () && n2 != other_nodes.end ()) {

    if (n1->first->has_other ()) {
      ++n1;
      continue;
    } else if (n2->first->has_other ()) {
      ++n2;
      continue;
    }

    if (*n1->first < *n2->first) {
      ++n1;
      continue;
    } else if (*n2->first < *n1->first) {
      ++n2;
      continue;
    }

    std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::iterator nn1 = n1, nn2 = n2;

    size_t num = 1;
    ++nn1;
    ++nn2;
    while (nn1 != nodes.end () && nn2 != other_nodes.end ()) {
      if (nn1->first->has_other ()) {
        ++nn1;
      } else if (nn2->first->has_other ()) {
        ++nn2;
      } else if (! (*nn1->first == *n1->first) || ! (*nn2->first == *n2->first)) {
        break;
      } else {
        ++num;
        ++nn1;
        ++nn2;
      }
    }

    if (num == 1 || data->with_ambiguous) {
      node_ranges.push_back (NodeRange (num, n1, nn1, n2, nn2));
    }

    //  in tentative mode ambiguous nodes don't make a match without
    //  with_ambiguous
    if (num > 1 && tentative && ! data->with_ambiguous) {
      return failed_match;
    }

    n1 = nn1;
    n2 = nn2;

  }

  if (data->with_ambiguous) {
    std::stable_sort (node_ranges.begin (), node_ranges.end ());
  }

  for (std::vector<NodeRange>::iterator nr = node_ranges.begin (); nr != node_ranges.end (); ++nr) {

    //  node ranges might have changed - adjust to real count and skip leading pairs assigned already

    while (nr->n1 != nr->nn1 && nr->n2 != nr->nn2) {
      if (nr->n1->first->has_other ()) {
        ++nr->n1;
      } else if (nr->n2->first->has_other ()) {
        ++nr->n2;
      } else {
        break;
      }
    }

    nr->num = 0;
    std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator i1 = nr->n1, i2 = nr->n2;

    while (i1 != nr->nn1 && i2 != nr->nn2) {
      if (i1->first->has_other ()) {
        ++i1;
      } else if (i2->first->has_other ()) {
        ++i2;
      } else {
        ++nr->num;
        ++i1;
        ++i2;
      }
    }

    if (nr->num < 1) {

      //  ignore this - it got obsolete.

    } else if (nr->num == 1) {

      size_t n = derive_node_identities_from_singular_match (nr->n1->first, nr->n1->second, nr->n2->first, nr->n2->second, dm, dm_other, scm, scm_other, depth, n_branch, tentative, data, ! data->dont_consider_net_names);
      if (n == failed_match) {
        return failed_match;
      }

      new_nodes += n;

    } else if (data->max_n_branch != std::numeric_limits<size_t>::max () && double (nr->num) * double (n_branch) > double (data->max_n_branch)) {

      if (options ()->debug_netcompare) {
        tl::info << indent_s << "max. complexity exhausted (" << nr->num << "*" << n_branch << ">" << data->max_n_branch << ") - mismatch.";
      }
      return failed_match;

    } else {

      if (options ()->debug_netcompare) {
        tl::info << indent_s << "analyzing ambiguity group with " << nr->num << " members";
      }

      size_t n = derive_node_identities_from_ambiguity_group (*nr, dm, dm_other, scm, scm_other, depth, n_branch, tentative, data);
      if (n == failed_match) {
        return failed_match;
      }

      new_nodes += n;

      if (options ()->debug_netcompare) {
        tl::info << indent_s << "finished analysis of ambiguity group with " << nr->num << " members";
      }

    }

  }

  return new_nodes;
}


// --------------------------------------------------------------------------------------------------------------------
//  NetlistComparer implementation

NetlistComparer::NetlistComparer (NetlistCompareLogger *logger)
  : mp_logger (logger)
{
  mp_device_categorizer.reset (new DeviceCategorizer ());
  mp_circuit_categorizer.reset (new CircuitCategorizer ());
  mp_circuit_pin_mapper.reset (new CircuitPinMapper ());

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
NetlistComparer::same_nets (const db::Net *na, const db::Net *nb)
{
  m_same_nets [std::make_pair (na->circuit (), nb->circuit ())].push_back (std::make_pair (na, nb));
}

void
NetlistComparer::equivalent_pins (const db::Circuit *cb, size_t pin1_id, size_t pin2_id)
{
  mp_circuit_pin_mapper->map_pins (cb, pin1_id, pin2_id);
}

void
NetlistComparer::equivalent_pins (const db::Circuit *cb, const std::vector<size_t> &pin_ids)
{
  mp_circuit_pin_mapper->map_pins (cb, pin_ids);
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

bool
NetlistComparer::compare (const db::Netlist *a, const db::Netlist *b) const
{
  m_case_sensitive = combined_case_sensitive (a, b);

  //  we need to create a copy because this method is supposed to be const.
  db::CircuitCategorizer circuit_categorizer = *mp_circuit_categorizer;
  db::DeviceCategorizer device_categorizer = *mp_device_categorizer;
  db::CircuitPinMapper circuit_pin_mapper = *mp_circuit_pin_mapper;

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

  //  impose the compare tolerances of the layout (first netlist) on the schematic (second netlist)
  //  TODO: this is kind of clumsy. But it's very important to use the same device sorting for both netlists, so we play this trick.
  //  A better solution was to have a common compare framework for both netlists.
  for (std::map<size_t, std::pair<const db::DeviceClass *, const db::DeviceClass *> >::const_iterator i = cat2dc.begin (); i != cat2dc.end (); ++i) {

    if (i->second.first && i->second.second) {

      const db::DeviceClass *da = i->second.first;
      db::DeviceClass *db = const_cast<db::DeviceClass *> (i->second.second);

      const db::DeviceParameterCompareDelegate *cmp = da->parameter_compare_delegate ();
      db->set_parameter_compare_delegate (const_cast<db::DeviceParameterCompareDelegate *> (cmp));

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

    std::vector<std::pair<const Net *, const Net *> > empty;
    const std::vector<std::pair<const Net *, const Net *> > *net_identity = &empty;
    std::map<std::pair<const db::Circuit *, const db::Circuit *>, std::vector<std::pair<const Net *, const Net *> > >::const_iterator sn = m_same_nets.find (std::make_pair (ca, cb));
    if (sn != m_same_nets.end ()) {
      net_identity = &sn->second;
    }

    if (all_subcircuits_verified (ca, verified_circuits_a) && all_subcircuits_verified (cb, verified_circuits_b)) {

      if (options ()->debug_netcompare) {
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
        mp_logger->circuit_skipped (ca, cb, generate_subcircuits_not_verified_warning (ca, verified_circuits_a, cb, verified_circuits_b));
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
std::vector<size_t> collect_pins_with_empty_nets (const db::Circuit *c, CircuitPinMapper *circuit_pin_mapper)
{
  std::vector<size_t> pins;

  for (db::Circuit::const_pin_iterator p = c->begin_pins (); p != c->end_pins (); ++p) {
    const db::Net *net = c->net_for_pin (p->id ());
    if ((! net || net->is_passive ()) && ! circuit_pin_mapper->is_mapped (c, p->id ())) {
      pins.push_back (p->id ());
    }
  }

  return pins;
}

void
NetlistComparer::derive_pin_equivalence (const db::Circuit *ca, const db::Circuit *cb, CircuitPinMapper *circuit_pin_mapper)
{
  //  TODO: All pins with empty nets are treated as equivalent - this as a quick way to
  //  treat circuits abstracts, although it's not really valid. By doing this, we
  //  don't capture the case of multiple (abstract) subcircuits wired in different ways.

  std::vector<size_t> pa, pb;
  pa = collect_pins_with_empty_nets (ca, circuit_pin_mapper);
  pb = collect_pins_with_empty_nets (cb, circuit_pin_mapper);

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
compute_subcircuit_key (const db::SubCircuit &subcircuit, const db::NetGraph &g, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinMapper *pin_map, std::vector<std::pair<size_t, size_t> > &k)
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
compute_subcircuit_key_for_this (const db::SubCircuit &subcircuit, const db::NetGraph &g, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinMapper *pin_map, bool &mapped, bool &valid)
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
compute_subcircuit_key_for_other (const db::SubCircuit &subcircuit, const db::NetGraph &g, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinMapper *pin_map, bool &mapped, bool &valid)
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

bool
NetlistComparer::compare_circuits (const db::Circuit *c1, const db::Circuit *c2,
                                   db::DeviceCategorizer &device_categorizer,
                                   db::CircuitCategorizer &circuit_categorizer,
                                   db::CircuitPinMapper &circuit_pin_mapper,
                                   const std::vector<std::pair<const Net *, const Net *> > &net_identity,
                                   bool &pin_mismatch,
                                   std::map<const db::Circuit *, CircuitMapper> &c12_circuit_and_pin_mapping,
                                   std::map<const db::Circuit *, CircuitMapper> &c22_circuit_and_pin_mapping) const
{
  db::DeviceFilter device_filter (m_cap_threshold, m_res_threshold);
  SubCircuitEquivalenceTracker subcircuit_equivalence;
  DeviceEquivalenceTracker device_equivalence;

  db::NetGraph g1, g2;

  //  NOTE: for normalization we map all subcircuits of c1 to c2.
  //  Also, pin swapping will only happen there.
  g1.build (c1, device_categorizer, circuit_categorizer, device_filter, &c12_circuit_and_pin_mapping, &circuit_pin_mapper);
  g2.build (c2, device_categorizer, circuit_categorizer, device_filter, &c22_circuit_and_pin_mapping, &circuit_pin_mapper);

  //  Match dummy nodes for null nets
  g1.identify (0, 0);
  g2.identify (0, 0);

  for (std::vector<std::pair<const Net *, const Net *> >::const_iterator p = net_identity.begin (); p != net_identity.end (); ++p) {
    size_t ni1 = g1.node_index_for_net (p->first);
    size_t ni2 = g2.node_index_for_net (p->second);
    g1.identify (ni1, ni2);
    g2.identify (ni2, ni1);
  }

  int iter = 0;

  tl::RelativeProgress progress (tl::to_string (tr ("Comparing circuits ")) + c1->name () + "/" + c2->name (), std::max (g1.end () - g1.begin (), g2.end () - g2.begin ()), 1);

  //  two passes: one without ambiguities, the second one with

  bool good = false;

  for (int pass = 0; pass < 2 && ! good; ++pass) {

    if (options ()->debug_netcompare) {
      if (pass > 0) {
        tl::info << "including ambiguous nodes now.";
      }
    }

    good = true;
    while (true) {

      ++iter;
      if (options ()->debug_netcompare) {
        tl::info << "new compare iteration #" << iter;
        tl::info << "deducing from present nodes ...";
      }

      size_t new_identities = 0;

      for (db::NetGraph::node_iterator i1 = g1.begin (); i1 != g1.end (); ++i1) {

        if (i1->has_other () && i1->net ()) {

          CompareData data;
          data.other = &g2;
          data.max_depth = m_max_depth;
          data.max_n_branch = m_max_n_branch;
          data.depth_first = m_depth_first;
          data.dont_consider_net_names = m_dont_consider_net_names;
          data.with_ambiguous = (pass > 0);
          data.circuit_pin_mapper = &circuit_pin_mapper;
          data.subcircuit_equivalence = &subcircuit_equivalence;
          data.device_equivalence = &device_equivalence;
          data.logger = mp_logger;
          data.progress = &progress;

          size_t ni = g1.derive_node_identities (i1 - g1.begin (), 0, 1, 0 /*not tentative*/, &data);
          if (ni > 0 && ni != failed_match) {
            new_identities += ni;
            if (options ()->debug_netcompare) {
              tl::info << ni << " new identities.";
            }
          }

        }

      }

      if (options ()->debug_netcompare) {
        tl::info << "checking topological identity ...";
      }

      //  derive new identities through topology: first collect all nets with the same topological signature

      std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> > nodes, other_nodes;

      std::vector<NetGraphNode::edge_type> no_edges;
      no_edges.push_back (NetGraphNode::edge_type ());

      nodes.reserve (g1.end () - g1.begin ());
      for (db::NetGraph::node_iterator i1 = g1.begin (); i1 != g1.end (); ++i1) {
        if (! i1->has_other () && i1->net ()) {
          nodes.push_back (std::make_pair (i1.operator-> (), no_edges.begin ()));
        }
      }

      other_nodes.reserve (g2.end () - g2.begin ());
      for (db::NetGraph::node_iterator i2 = g2.begin (); i2 != g2.end (); ++i2) {
        if (! i2->has_other () && i2->net ()) {
          other_nodes.push_back (std::make_pair (i2.operator-> (), no_edges.begin ()));
        }
      }

      if (nodes.empty () || other_nodes.empty ()) {
        //  active mismatched nodes give an error
        for (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator>  >::const_iterator n = nodes.begin (); n != nodes.end () && good; ++n) {
          good = is_passive_net (n->first->net (), c12_circuit_and_pin_mapping);
        }
        for (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator>  >::const_iterator n = other_nodes.begin (); n != other_nodes.end () && good; ++n) {
          good = is_passive_net (n->first->net (), c22_circuit_and_pin_mapping);
        }
        if (options ()->debug_netcompare) {
          tl::info << "Stopped with " << nodes.size () << "/" << other_nodes.size () << " nodes left unresolved " << (good ? "(accepted)" : "(not accepted)");
        }
        //  this assumes that we don't gain anything here. Stop now.
        break;
      }

      std::sort (nodes.begin (), nodes.end (), CompareNodePtr ());
      std::sort (other_nodes.begin (), other_nodes.end (), CompareNodePtr ());

      CompareData data;
      data.other = &g2;
      data.max_depth = m_max_depth;
      data.max_n_branch = m_max_n_branch;
      data.dont_consider_net_names = m_dont_consider_net_names;
      data.with_ambiguous = (pass > 0);
      data.circuit_pin_mapper = &circuit_pin_mapper;
      data.subcircuit_equivalence = &subcircuit_equivalence;
      data.device_equivalence = &device_equivalence;
      data.logger = mp_logger;
      data.progress = &progress;

      size_t ni = g1.derive_node_identities_from_node_set (nodes, other_nodes, 0, 1, 0 /*not tentatively*/, &data);
      if (ni > 0 && ni != failed_match) {
        new_identities += ni;
        if (options ()->debug_netcompare) {
          tl::info << ni << " new identities.";
        }
      }

      if (new_identities == 0) {
        if (options ()->debug_netcompare) {
          tl::info << "Stopped with " << nodes.size () << "/" << other_nodes.size () << " nodes left unresolved.";
        }
        good = false;
        break;
      }

    }

  }

  //  Report missing net assignment

  for (db::NetGraph::node_iterator i = g1.begin (); i != g1.end (); ++i) {
    if (! i->has_other ()) {
      if (options ()->debug_netcompare) {
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
      if (options ()->debug_netcompare) {
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
        c22_pin_mapping.map_pin (fp->second->id (), p->id ());

      } else if (next_abstract != abstract_pins2.end ()) {

        //  assign an abstract pin - this is a dummy assignment which is mitigated
        //  by declaring the pins equivalent in derive_pin_equivalence
        if (mp_logger) {
          mp_logger->match_pins (p.operator-> (), *next_abstract);
        }
        c12_pin_mapping.map_pin (p->id (), (*next_abstract)->id ());
        c22_pin_mapping.map_pin ((*next_abstract)->id (), p->id ());

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
      //  TODO: report devices which cannot be distiguished topologically?
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

      std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::Device *, size_t> >::iterator dm = device_map.find (k);

      if (! mapped || dm == device_map.end () || dm->first != k) {

        if (mp_logger) {
          unmatched_b.push_back (std::make_pair (k, std::make_pair (d.operator-> (), device_cat)));
        }
        good = false;

      } else {

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
NetlistComparer::do_subcircuit_assignment (const db::Circuit *c1, const db::NetGraph &g1, const db::Circuit *c2, const db::NetGraph &g2, CircuitCategorizer &circuit_categorizer, const CircuitPinMapper &circuit_pin_mapper, std::map<const Circuit *, CircuitMapper> &c12_circuit_and_pin_mapping, std::map<const Circuit *, CircuitMapper> &c22_circuit_and_pin_mapping, SubCircuitEquivalenceTracker &subcircuit_eq, bool &good) const
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
      //  TODO: report devices which cannot be distiguished topologically?
      subcircuit_map.insert (std::make_pair (k, std::make_pair (sc.operator-> (), sc_cat)));
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

            //  no unqiue match
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
  db::CircuitPinMapper circuit_pin_mapper;
  std::map<const db::Circuit *, CircuitMapper> circuit_and_pin_mapping;

  db::NetGraph graph;
  graph.build (circuit, *mp_device_categorizer, *mp_circuit_categorizer, device_filter, &circuit_and_pin_mapping, &circuit_pin_mapper);

  //  sort the nodes so we can easily identify the identical ones (in terms of topology)
  //  nodes are identical if the attached devices and circuits are of the same kind and with the same parameters
  //  and connect to other nodes in identical configurations.

  std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> > nodes;

  std::vector<NetGraphNode::edge_type> no_edges;
  no_edges.push_back (NetGraphNode::edge_type ());

  nodes.reserve (graph.end () - graph.begin ());
  for (db::NetGraph::node_iterator i = graph.begin (); i != graph.end (); ++i) {
    if (! i->has_other () && i->net ()) {
      nodes.push_back (std::make_pair (i.operator-> (), no_edges.begin ()));
    }
  }

  std::sort (nodes.begin (), nodes.end (), CompareNodePtr ());

  //  Identical nodes leading to the same nodes on the other side are candidates for symmetry.

  tl::equivalence_clusters<const NetGraphNode *> identical_nodes;

  for (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator np = nodes.begin (); np + 1 != nodes.end (); ++np) {
    if (*np[0].first == *np[1].first) {
      identical_nodes.same (np[0].first, np[1].first);
    }
  }

  std::vector<std::set<size_t> > symmetry_groups;
  std::set<size_t> visited;

  for (std::vector<std::pair<const NetGraphNode *, NetGraphNode::edge_iterator> >::const_iterator np = nodes.begin (); np != nodes.end (); ++np) {

    size_t node_id = graph.node_index_for_net (np[0].first->net ());
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

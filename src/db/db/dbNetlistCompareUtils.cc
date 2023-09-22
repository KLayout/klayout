
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

#include "dbNetlistCompareUtils.h"
#include "dbNetlist.h"
#include "dbNetlistDeviceClasses.h"

#include "tlEnv.h"

namespace db
{

// --------------------------------------------------------------------------------------------------------------------
//  NetlistCompareGlobalOptions implementation

NetlistCompareGlobalOptions::NetlistCompareGlobalOptions ()
{
  m_is_initialized = false;
}

void
NetlistCompareGlobalOptions::ensure_initialized ()
{
  if (! m_is_initialized) {
    //  $KLAYOUT_NETLIST_COMPARE_DEBUG_NETCOMPARE
    debug_netcompare = tl::app_flag ("netlist-compare-debug-netcompare");
    //  $KLAYOUT_NETLIST_COMPARE_DEBUG_NETGRAPH
    debug_netgraph = tl::app_flag ("netlist-compare-debug-netgraph");
    m_is_initialized = true;
  }
}

NetlistCompareGlobalOptions *
NetlistCompareGlobalOptions::options ()
{
  //  TODO: thread safe?
  static NetlistCompareGlobalOptions s_options;
  s_options.ensure_initialized ();
  return &s_options;
}

// --------------------------------------------------------------------------------------------------------------------
//   Some utilities

std::string
nl_compare_debug_indent (size_t depth)
{
  std::string s;
  for (size_t d = 0; d < depth; ++d) {
    s += "|  ";
  }
  return s;
}

const std::string var_sep = tl::to_string (tr (" vs. "));

static std::string
expanded_name (const db::Net *a)
{
  if (a == 0) {
    return tl::to_string (tr ("(not connected)"));
  } else {
    return a->expanded_name ();
  }
}

std::string
nets2string (const db::Net *a, const db::Net *b)
{
  std::string na = expanded_name (a);
  std::string nb = expanded_name (b);
  if (na != nb) {
    return na + var_sep + nb;
  } else {
    return nb;
  }
}

std::string
nets2string (const std::pair<const db::Net *, const db::Net *> &np)
{
  return nets2string (np.first, np.second);
}



// --------------------------------------------------------------------------------------------------------------------
//   Some functions

bool combined_case_sensitive (const db::Netlist *a, const db::Netlist *b)
{
  bool csa = a ? a->is_case_sensitive () : true;
  bool csb = b ? b->is_case_sensitive () : true;
  return csa && csb;
}

//  for comparing the net names also employ the pin name if one is given
const std::string &extended_net_name (const db::Net *n)
{
  if (! n->name ().empty ()) {
    return n->name ();
  } else if (n->begin_pins () != n->end_pins ()) {
    return n->begin_pins ()->pin ()->name ();
  } else {
    return n->name ();
  }
}

int name_compare (const db::Net *a, const db::Net *b)
{
  return db::Netlist::name_compare (combined_case_sensitive (a->netlist (), b->netlist ()), extended_net_name (a), extended_net_name (b));
}

bool net_names_are_different (const db::Net *a, const db::Net *b)
{
  if (! a || ! b || extended_net_name (a).empty () || extended_net_name (b).empty ()) {
    return false;
  } else {
    return name_compare (a, b) != 0;
  }
}

bool net_names_are_equal (const db::Net *a, const db::Net *b)
{
  if (! a || ! b || extended_net_name (a).empty () || extended_net_name (b).empty ()) {
    return false;
  } else {
    return name_compare (a, b) == 0;
  }
}

// --------------------------------------------------------------------------------------------------------------------
//  DeviceCompare implementation

bool
DeviceCompare::operator() (const std::pair<const db::Device *, size_t> &d1, const std::pair<const db::Device *, size_t> &d2) const
{
  if (d1.second != d2.second) {
    return d1.second < d2.second;
  }
  return db::DeviceClass::less (*d1.first, *d2.first);
}

bool
DeviceCompare::equals (const std::pair<const db::Device *, size_t> &d1, const std::pair<const db::Device *, size_t> &d2) const
{
  if (d1.second != d2.second) {
    return false;
  }
  return db::DeviceClass::equal (*d1.first, *d2.first);
}

// --------------------------------------------------------------------------------------------------------------------
//  SubCircuitCompare implementation

bool
SubCircuitCompare::operator() (const std::pair<const db::SubCircuit *, size_t> &sc1, const std::pair<const db::SubCircuit *, size_t> &sc2) const
{
  return sc1.second < sc2.second;
}

bool
SubCircuitCompare::equals (const std::pair<const db::SubCircuit *, size_t> &sc1, const std::pair<const db::SubCircuit *, size_t> &sc2) const
{
  return sc1.second == sc2.second;
}

// --------------------------------------------------------------------------------------------------------------------
//  CircuitPinMapper implementation

CircuitPinCategorizer::CircuitPinCategorizer ()
{
  //  .. nothing yet ..
}

void
CircuitPinCategorizer::map_pins (const db::Circuit *circuit, size_t pin1_id, size_t pin2_id)
{
  m_pin_map [circuit].same (pin1_id, pin2_id);
}

void
CircuitPinCategorizer::map_pins (const db::Circuit *circuit, const std::vector<size_t> &pin_ids)
{
  if (pin_ids.size () < 2) {
    return;
  }

  tl::equivalence_clusters<size_t> &pm = m_pin_map [circuit];
  for (size_t i = 1; i < pin_ids.size (); ++i) {
    pm.same (pin_ids [0], pin_ids [i]);
  }
}

size_t
CircuitPinCategorizer::is_mapped (const db::Circuit *circuit, size_t pin_id) const
{
  std::map<const db::Circuit *, tl::equivalence_clusters<size_t> >::const_iterator pm = m_pin_map.find (circuit);
  if (pm != m_pin_map.end ()) {
    return pm->second.has_attribute (pin_id);
  } else {
    return false;
  }
}

size_t
CircuitPinCategorizer::normalize_pin_id (const db::Circuit *circuit, size_t pin_id) const
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

// --------------------------------------------------------------------------------------------------------------------
//  CircuitMapper implementation

CircuitMapper::CircuitMapper ()
  : mp_other (0)
{
  //  .. nothing yet ..
}

void
CircuitMapper::map_pin (size_t this_pin, size_t other_pin)
{
  m_pin_map.insert (std::make_pair (this_pin, other_pin));
  m_rev_pin_map.insert (std::make_pair (other_pin, this_pin));
}

bool
CircuitMapper::has_other_pin_for_this_pin (size_t this_pin) const
{
  return m_pin_map.find (this_pin) != m_pin_map.end ();
}

bool
CircuitMapper::has_this_pin_for_other_pin (size_t other_pin) const
{
  return m_rev_pin_map.find (other_pin) != m_rev_pin_map.end ();
}

size_t
CircuitMapper::other_pin_from_this_pin (size_t this_pin) const
{
  std::map<size_t, size_t>::const_iterator i = m_pin_map.find (this_pin);
  tl_assert (i != m_pin_map.end ());
  return i->second;
}

size_t
CircuitMapper::this_pin_from_other_pin (size_t other_pin) const
{
  std::map<size_t, size_t>::const_iterator i = m_rev_pin_map.find (other_pin);
  tl_assert (i != m_rev_pin_map.end ());
  return i->second;
}

// --------------------------------------------------------------------------------------------------------------------
//  DeviceFilter implementation

DeviceFilter::DeviceFilter (double cap_threshold, double res_threshold)
  : m_cap_threshold (cap_threshold), m_res_threshold (res_threshold)
{
  //  .. nothing yet ..
}

bool
DeviceFilter::filter (const db::Device *device) const
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

// --------------------------------------------------------------------------------------------------------------------
//  generic_categorizer implementation

template <class Obj> generic_categorizer<Obj>::generic_categorizer (bool with_name)
  : m_next_cat (0), m_with_name (with_name), m_case_sensitive (true)
{
  //  .. nothing yet ..
}

template <class Obj>
void
generic_categorizer<Obj>::set_case_sensitive (bool f)
{
  m_case_sensitive = f;
}

template <class Obj>
void
generic_categorizer<Obj>::same (const Obj *ca, const Obj *cb)
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

template <class Obj>
bool
generic_categorizer<Obj>::has_cat_for (const Obj *cls)
{
  return m_cat_by_ptr.find (cls) != m_cat_by_ptr.end ();
}

template <class Obj>
size_t
generic_categorizer<Obj>::cat_for (const Obj *cls)
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

//  explicit instantiations
template class generic_categorizer<db::DeviceClass>;
template class generic_categorizer<db::Circuit>;

// --------------------------------------------------------------------------------------------------------------------
//  DeviceCategorizer implementation

DeviceCategorizer::DeviceCategorizer ()
  : generic_categorizer<db::DeviceClass> ()
{
  //  .. nothing yet ..
}

void
DeviceCategorizer::same_class (const db::DeviceClass *ca, const db::DeviceClass *cb)
{
  generic_categorizer<db::DeviceClass>::same (ca, cb);
}

size_t
DeviceCategorizer::cat_for_device (const db::Device *device)
{
  const db::DeviceClass *cls = device->device_class ();
  if (! cls) {
    return 0;
  }

  return cat_for_device_class (cls);
}

void
DeviceCategorizer::clear_strict_device_categories ()
{
  m_strict_device_categories.clear ();
}

void
DeviceCategorizer::set_strict_device_category (size_t cat)
{
  m_strict_device_categories.insert (cat);
}

bool
DeviceCategorizer::is_strict_device_category (size_t cat) const
{
  return m_strict_device_categories.find (cat) != m_strict_device_categories.end ();
}

// --------------------------------------------------------------------------------------------------------------------
//  CircuitCategorizer implementation

CircuitCategorizer::CircuitCategorizer ()
  : generic_categorizer<db::Circuit> ()
{
  //  .. nothing yet ..
}

void
CircuitCategorizer::same_circuit (const db::Circuit *ca, const db::Circuit *cb)
{
  //  no arbitrary cross-pairing
  //  NOTE: many layout circuits are allowed for one schematic to account for layout alternatives.
  if (ca && has_cat_for (ca)) {
    throw tl::Exception (tl::to_string (tr ("Circuit is already paired with other circuit: ")) + ca->name ());
  }
  generic_categorizer<db::Circuit>::same (ca, cb);
}

size_t
CircuitCategorizer::cat_for_subcircuit (const db::SubCircuit *subcircuit)
{
  const db::Circuit *cr = subcircuit->circuit_ref ();
  if (! cr) {
    return 0;
  } else {
    return cat_for_circuit (cr);
  }
}

}


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

#include "dbNetlist.h"

#include <set>

namespace db
{

// --------------------------------------------------------------------------------
//  Netlist class implementation

Netlist::Netlist (NetlistManipulationCallbacks *callbacks)
  : m_case_sensitive (true), mp_callbacks (callbacks),
    m_valid_topology (false), m_lock_count (0),
    m_circuit_by_name (this, &Netlist::begin_circuits, &Netlist::end_circuits),
    m_circuit_by_cell_index (this, &Netlist::begin_circuits, &Netlist::end_circuits),
    m_device_abstract_by_name (this, &Netlist::begin_device_abstracts, &Netlist::end_device_abstracts),
    m_device_abstract_by_cell_index (this, &Netlist::begin_device_abstracts, &Netlist::end_device_abstracts)
{
  m_circuits.changed ().add (this, &Netlist::invalidate_topology);
  m_circuits.changed ().add (this, &Netlist::circuits_changed);
  m_device_abstracts.changed ().add (this, &Netlist::device_abstracts_changed);
}

Netlist::Netlist (const Netlist &other)
  : gsi::ObjectBase (other), tl::Object (other),
    m_valid_topology (false), m_lock_count (0),
    m_circuit_by_name (this, &Netlist::begin_circuits, &Netlist::end_circuits),
    m_circuit_by_cell_index (this, &Netlist::begin_circuits, &Netlist::end_circuits),
    m_device_abstract_by_name (this, &Netlist::begin_device_abstracts, &Netlist::end_device_abstracts),
    m_device_abstract_by_cell_index (this, &Netlist::begin_device_abstracts, &Netlist::end_device_abstracts)
{
  operator= (other);
  m_circuits.changed ().add (this, &Netlist::invalidate_topology);
  m_circuits.changed ().add (this, &Netlist::circuits_changed);
  m_device_abstracts.changed ().add (this, &Netlist::device_abstracts_changed);
}

Netlist::~Netlist ()
{
  m_circuits.changed ().remove (this, &Netlist::invalidate_topology);
  m_circuits.changed ().remove (this, &Netlist::circuits_changed);
  m_device_abstracts.changed ().remove (this, &Netlist::device_abstracts_changed);
}

Netlist &Netlist::operator= (const Netlist &other)
{
  if (this != &other) {

    clear ();

    set_case_sensitive (other.is_case_sensitive ());

    std::map<const DeviceClass *, DeviceClass *> dct;
    for (const_device_class_iterator dc = other.begin_device_classes (); dc != other.end_device_classes (); ++dc) {
      DeviceClass *dc_new = dc->clone ();
      dct [dc.operator-> ()] = dc_new;
      m_device_classes.push_back (dc_new);
    }

    std::map<const DeviceAbstract *, DeviceAbstract *> dmt;
    for (const_abstract_model_iterator dm = other.begin_device_abstracts (); dm != other.end_device_abstracts (); ++dm) {
      DeviceAbstract *dm_new = new DeviceAbstract (*dm);
      dmt [dm.operator-> ()] = dm_new;
      m_device_abstracts.push_back (dm_new);
    }

    std::map<const Circuit *, Circuit *> ct;
    for (const_circuit_iterator i = other.begin_circuits (); i != other.end_circuits (); ++i) {
      Circuit *ct_new = new Circuit (*i);
      ct_new->translate_device_classes (dct);
      ct_new->translate_device_abstracts (dmt);
      ct [i.operator-> ()] = ct_new;
      add_circuit (ct_new);
    }

    for (circuit_iterator i = begin_circuits (); i != end_circuits (); ++i) {
      i->translate_circuits (ct);
    }

  }
  return *this;
}

void Netlist::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (! no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }

  db::mem_stat (stat, purpose, cat, m_circuits, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_device_classes, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_device_abstracts, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_top_down_circuits, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_child_circuits, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_parent_circuits, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_circuit_by_name, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_circuit_by_cell_index, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_device_abstract_by_name, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_device_abstract_by_cell_index, true, (void *) this);
}

void Netlist::set_case_sensitive (bool f)
{
  m_case_sensitive = f;
}

int Netlist::name_compare (bool case_sensitive, const std::string &n1, const std::string &n2)
{
  //  TODO: unicode support?
  if (case_sensitive) {
    return strcmp (n1.c_str (), n2.c_str ());
  } else {
#if defined(_WIN32)
    return _stricmp (n1.c_str (), n2.c_str ());
#else
    return strcasecmp (n1.c_str (), n2.c_str ());
#endif
  }
}

std::string Netlist::normalize_name (bool case_sensitive, const std::string &n)
{
  if (case_sensitive) {
    return n;
  } else {
    return tl::to_upper_case (n);
  }
}

void Netlist::circuits_changed ()
{
  m_circuit_by_cell_index.invalidate ();
  m_circuit_by_name.invalidate ();
}

void Netlist::device_abstracts_changed ()
{
  m_device_abstract_by_cell_index.invalidate ();
  m_device_abstract_by_name.invalidate ();
}

void Netlist::invalidate_topology ()
{
  if (m_valid_topology) {

    m_valid_topology = false;

    if (m_lock_count == 0) {
      m_top_circuits = 0;
      m_top_down_circuits.clear ();
      m_child_circuits.clear ();
      m_parent_circuits.clear ();
    }

  }
}

namespace {
  struct sort_by_index
  {
    bool operator () (const Circuit *a, const Circuit *b) const
    {
      return a->index () < b->index ();
    }
  };
}

void Netlist::validate_topology ()
{
  if (m_valid_topology) {
    return;
  } else if (m_lock_count > 0) {
    return;
  }

  m_child_circuits.clear ();
  m_parent_circuits.clear ();

  size_t max_index = 0;
  for (circuit_iterator c = begin_circuits (); c != end_circuits (); ++c) {
    c->set_index (max_index);
    ++max_index;
  }

  //  build the child circuit list ... needed for the topology sorting

  m_child_circuits.reserve (max_index);
  m_parent_circuits.reserve (max_index);

  for (circuit_iterator c = begin_circuits (); c != end_circuits (); ++c) {

    std::set<Circuit *> children;
    for (Circuit::subcircuit_iterator sc = c->begin_subcircuits (); sc != c->end_subcircuits (); ++sc) {
      if (sc->circuit_ref ()) {
        children.insert (sc->circuit_ref ());
      }
    }

    m_child_circuits.push_back (tl::vector<Circuit *> ());
    tl::vector<Circuit *> &cc = m_child_circuits.back ();
    cc.reserve (children.size ());
    cc.insert (cc.end (), children.begin (), children.end ());
    //  sort by index for better reproducibility
    std::sort (cc.begin (), cc.end (), sort_by_index ());

    std::set<Circuit *> parents;
    for (Circuit::refs_iterator sc = c->begin_refs (); sc != c->end_refs (); ++sc) {
      if (sc->circuit ()) {
        parents.insert (sc->circuit ());
      }
    }

    m_parent_circuits.push_back (tl::vector<Circuit *> ());
    tl::vector<Circuit *> &pc = m_parent_circuits.back ();
    pc.reserve (parents.size ());
    pc.insert (pc.end (), parents.begin (), parents.end ());
    //  sort by index for better reproducibility
    std::sort (pc.begin (), pc.end (), sort_by_index ());

  }

  //  do topology sorting

  m_top_circuits = 0;
  m_top_down_circuits.clear ();
  m_top_down_circuits.reserve (max_index);

  std::vector<size_t> num_parents (max_index, 0);

  //  while there are cells to treat ..
  while (m_top_down_circuits.size () != max_index) {

    size_t n_top_down_circuits = m_top_down_circuits.size ();

    //  Treat all circuits that do not have all parents reported.
    //  For all such a circuits, disable the parent counting,
    //  add the circuits's index to the top-down sorted list and
    //  increment the reported parent count in all the
    //  child circuits.

    for (circuit_iterator c = begin_circuits (); c != end_circuits (); ++c) {
      if (m_parent_circuits [c->index ()].size () == num_parents [c->index ()]) {
        m_top_down_circuits.push_back (c.operator-> ());
        num_parents [c->index ()] = std::numeric_limits<cell_index_type>::max ();
      }
    }

    //  For all these a circuits, increment the reported parent instance
    //  count in all the child circuits.
    for (tl::vector<Circuit *>::const_iterator ii = m_top_down_circuits.begin () + n_top_down_circuits; ii != m_top_down_circuits.end (); ++ii) {
      const tl::vector<Circuit *> &cc = m_child_circuits [(*ii)->index ()];
      for (tl::vector<Circuit *>::const_iterator icc = cc.begin (); icc != cc.end (); ++icc) {
        tl_assert (num_parents [(*icc)->index ()] != std::numeric_limits<cell_index_type>::max ());
        num_parents [(*icc)->index ()] += 1;
      }
    }

    //  If no new cells have been reported this is basically a
    //  sign of recursion in the graph.
    if (n_top_down_circuits == m_top_down_circuits.size ()) {
      throw tl::Exception (tl::to_string (tr ("Recursive hierarchy detected in netlist")));
    }

    //  doing this reverse will mean we preserve bottom-up order. This is useful for
    //  netlists where subcircuits have to be defined before they are used.
    std::reverse (m_top_down_circuits.begin () + n_top_down_circuits, m_top_down_circuits.end ());

  }

  //  Determine the number of top cells
  for (tl::vector<Circuit *>::const_iterator e = m_top_down_circuits.begin (); e != m_top_down_circuits.end () && m_parent_circuits [(*e)->index ()].empty (); ++e) {
    ++m_top_circuits;
  }

  m_valid_topology = true;
}

void Netlist::lock ()
{
  if (m_lock_count == 0) {
    validate_topology ();
  }
  ++m_lock_count;
}

void Netlist::unlock ()
{
  if (m_lock_count > 0) {
    --m_lock_count;
  }
}

const tl::vector<Circuit *> &Netlist::child_circuits (Circuit *circuit)
{
  if (circuit->netlist () != this) {
    throw tl::Exception (tl::to_string (tr ("Circuit not within given netlist")));
  }

  if (! m_valid_topology) {
    validate_topology ();
  }

  tl_assert (circuit->index () < m_child_circuits.size ());
  return m_child_circuits [circuit->index ()];
}

const tl::vector<Circuit *> &Netlist::parent_circuits (Circuit *circuit)
{
  if (circuit->netlist () != this) {
    throw tl::Exception (tl::to_string (tr ("Circuit not within given netlist")));
  }

  if (! m_valid_topology) {
    validate_topology ();
  }

  tl_assert (circuit->index () < m_parent_circuits.size ());
  return m_parent_circuits [circuit->index ()];
}

Netlist::top_down_circuit_iterator Netlist::begin_top_down ()
{
  if (! m_valid_topology) {
    validate_topology ();
  }
  return m_top_down_circuits.begin ();
}

Netlist::top_down_circuit_iterator Netlist::end_top_down ()
{
  if (! m_valid_topology) {
    validate_topology ();
  }
  return m_top_down_circuits.end ();
}

Netlist::const_top_down_circuit_iterator Netlist::begin_top_down () const
{
  if (! m_valid_topology) {
    const_cast<Netlist *> (this)->validate_topology ();
  }
  return reinterpret_cast<const tl::vector<const Circuit *> &> (m_top_down_circuits).begin ();
}

Netlist::const_top_down_circuit_iterator Netlist::end_top_down () const
{
  if (! m_valid_topology) {
    const_cast<Netlist *> (this)->validate_topology ();
  }
  return reinterpret_cast<const tl::vector<const Circuit *> &> (m_top_down_circuits).end ();
}

size_t Netlist::top_circuit_count () const
{
  if (! m_valid_topology) {
    const_cast<Netlist *> (this)->validate_topology ();
  }
  return m_top_circuits;
}

Netlist::bottom_up_circuit_iterator Netlist::begin_bottom_up ()
{
  if (! m_valid_topology) {
    validate_topology ();
  }
  return m_top_down_circuits.rbegin ();
}

Netlist::bottom_up_circuit_iterator Netlist::end_bottom_up ()
{
  if (! m_valid_topology) {
    validate_topology ();
  }
  return m_top_down_circuits.rend ();
}

Netlist::const_bottom_up_circuit_iterator Netlist::begin_bottom_up () const
{
  if (! m_valid_topology) {
    const_cast<Netlist *> (this)->validate_topology ();
  }
  return reinterpret_cast<const tl::vector<const Circuit *> &> (m_top_down_circuits).rbegin ();
}

Netlist::const_bottom_up_circuit_iterator Netlist::end_bottom_up () const
{
  if (! m_valid_topology) {
    const_cast<Netlist *> (this)->validate_topology ();
  }
  return reinterpret_cast<const tl::vector<const Circuit *> &> (m_top_down_circuits).rend ();
}

void Netlist::clear ()
{
  m_device_classes.clear ();
  m_device_abstracts.clear ();
  m_circuits.clear ();
}

void Netlist::add_circuit (Circuit *circuit)
{
  if (! circuit) {
    return;
  }
  if (circuit->netlist ()) {
    throw tl::Exception (tl::to_string (tr ("Circuit already contained in a netlist")));
  }

  m_circuits.push_back (circuit);
  circuit->set_netlist (this);
}

void Netlist::remove_circuit (Circuit *circuit)
{
  if (! circuit) {
    return;
  }
  if (circuit->netlist () != this) {
    throw tl::Exception (tl::to_string (tr ("Circuit not within given netlist")));
  }

  circuit->set_netlist (0);
  m_circuits.erase (circuit);
}

void Netlist::purge_circuit (Circuit *circuit)
{
  if (! circuit) {
    return;
  }
  if (circuit->netlist () != this) {
    throw tl::Exception (tl::to_string (tr ("Circuit not within given netlist")));
  }

  circuit->blank ();
  remove_circuit (circuit);
}

void Netlist::flatten_circuits (const std::vector<Circuit *> &circuits)
{
  if (circuits.empty ()) {
    return;
  }

  std::set<Circuit *> circuits_set (circuits.begin (), circuits.end ());

  std::vector<Circuit *> to_flatten;
  to_flatten.reserve (circuits.size ());

  //  Before flatten, we sort top-down. This optimizes for the case of flattening away
  //  some hierarchy above a certain circuit.
  for (top_down_circuit_iterator c = begin_top_down (); c != end_top_down (); ++c) {
    if (circuits_set.find (c.operator-> ()) != circuits_set.end ()) {
      to_flatten.push_back (c.operator-> ());
    }
  }

  for (std::vector<Circuit *>::const_iterator c = to_flatten.begin (); c != to_flatten.end (); ++c) {
    flatten_circuit (*c);
  }
}

void Netlist::flatten_circuit (Circuit *circuit)
{
  if (! circuit) {
    return;
  }
  if (circuit->netlist () != this) {
    throw tl::Exception (tl::to_string (tr ("Circuit not within given netlist")));
  }

  std::vector<db::SubCircuit *> refs;
  for (db::Circuit::refs_iterator sc = circuit->begin_refs (); sc != circuit->end_refs (); ++sc) {
    refs.push_back (sc.operator-> ());
  }

  for (std::vector<db::SubCircuit *>::const_iterator r = refs.begin (); r != refs.end (); ++r) {
    (*r)->circuit ()->flatten_subcircuit (*r);
  }

  delete circuit;
}

void Netlist::flatten ()
{
  db::NetlistLocker locker (this);

  std::set<db::Circuit *> top_circuits;
  size_t ntop = top_circuit_count ();
  for (db::Netlist::top_down_circuit_iterator tc = begin_top_down (); tc != end_top_down () && ntop > 0; ++tc) {
    top_circuits.insert (tc.operator-> ());
    --ntop;
  }

  for (db::Netlist::bottom_up_circuit_iterator c = begin_bottom_up (); c != end_bottom_up(); ++c) {
    if (top_circuits.find (c.operator-> ()) == top_circuits.end ()) {
      flatten_circuit (c.operator-> ());
    }
  }
}

DeviceClass *Netlist::device_class_by_name (const std::string &name)
{
  std::string nn = m_case_sensitive ? name : normalize_name (name);

  for (device_class_iterator d = begin_device_classes (); d != end_device_classes (); ++d) {
    if (d->name () == nn) {
      return d.operator-> ();
    }
  }
  return 0;
}

void Netlist::add_device_class (DeviceClass *device_class)
{
  if (! device_class) {
    return;
  }
  if (device_class->netlist ()) {
    throw tl::Exception (tl::to_string (tr ("Device class already contained in a netlist")));
  }

  m_device_classes.push_back (device_class);
  device_class->set_netlist (this);
}

void Netlist::remove_device_class (DeviceClass *device_class)
{
  if (! device_class) {
    return;
  }
  if (device_class->netlist () != this) {
    throw tl::Exception (tl::to_string (tr ("Device class not within given netlist")));
  }

  device_class->set_netlist (0);
  m_device_classes.erase (device_class);
}

void Netlist::add_device_abstract (DeviceAbstract *device_abstract)
{
  if (! device_abstract) {
    return;
  }
  if (device_abstract->netlist ()) {
    throw tl::Exception (tl::to_string (tr ("Device abstract already contained in a netlist")));
  }

  m_device_abstracts.push_back (device_abstract);
  device_abstract->set_netlist (this);
}

void Netlist::remove_device_abstract (DeviceAbstract *device_abstract)
{
  if (! device_abstract) {
    return;
  }
  if (device_abstract->netlist () != this) {
    throw tl::Exception (tl::to_string (tr ("Device abstract not within given netlist")));
  }

  device_abstract->set_netlist (0);
  m_device_abstracts.erase (device_abstract);
}

void Netlist::purge_nets ()
{
  for (bottom_up_circuit_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
    c->purge_nets ();
  }
}

void Netlist::make_top_level_pins ()
{
  size_t ntop = top_circuit_count ();
  for (top_down_circuit_iterator c = begin_top_down (); c != end_top_down () && ntop > 0; ++c, --ntop) {

    Circuit *circuit = c.operator-> ();

    if (circuit->pin_count () == 0) {

      //  create pins for the named nets and connect them
      for (Circuit::net_iterator n = circuit->begin_nets (); n != circuit->end_nets (); ++n) {
        if (! n->name ().empty () && n->terminal_count () + n->subcircuit_pin_count () > 0) {
          Pin pin = circuit->add_pin (n->name ());
          circuit->connect_pin (pin.id (), n.operator-> ());
        }
      }

    }

  }
}

void Netlist::purge ()
{
  //  This locking is very important as we do not want to recompute the bottom-up list
  //  while iterating.
  NetlistLocker locker (this);

  for (bottom_up_circuit_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {

    Circuit *circuit = c.operator-> ();

    //  purge floating, disconnected nets
    circuit->purge_nets ();

    //  if only passive nets are left, consider this circuit for purging
    bool purge_candidate = ! circuit->dont_purge ();
    for (db::Circuit::net_iterator n = circuit->begin_nets (); n != circuit->end_nets () && purge_candidate; ++n) {
      purge_candidate = n->is_passive ();
    }

    if (purge_candidate) {

      //  No nets left: delete the subcircuits that refer to us and finally delete the circuit
      while (circuit->begin_refs () != circuit->end_refs ()) {
        delete circuit->begin_refs ().operator-> ();
      }
      delete circuit;

    }

  }
}

void Netlist::combine_devices ()
{
  for (circuit_iterator c = begin_circuits (); c != end_circuits (); ++c) {
    c->combine_devices ();
  }
}

void Netlist::simplify ()
{
  make_top_level_pins ();
  purge ();
  combine_devices ();
  purge_nets ();
}

static std::string net2string (const db::Net *net)
{
  return net ? tl::to_word_or_quoted_string (net->expanded_name ()) : "(null)";
}

static std::string device2string (const db::Device &device)
{
  if (device.name ().empty ()) {
    return "$" + tl::to_string (device.id ());
  } else {
    return tl::to_word_or_quoted_string (device.name ());
  }
}

static std::string subcircuit2string (const db::SubCircuit &subcircuit)
{
  if (subcircuit.name ().empty ()) {
    return "$" + tl::to_string (subcircuit.id ());
  } else {
    return tl::to_word_or_quoted_string (subcircuit.name ());
  }
}

static std::string pin2string (const db::Pin &pin)
{
  if (pin.name ().empty ()) {
    //  the pin ID is zero-based and essentially the index, so we add 1 to make it compliant with the other IDs
    return "$" + tl::to_string (pin.id () + 1);
  } else {
    return tl::to_word_or_quoted_string (pin.name ());
  }
}

std::string Netlist::to_string () const
{
  std::string res;
  for (db::Netlist::const_circuit_iterator c = begin_circuits (); c != end_circuits (); ++c) {

    std::string ps;
    for (db::Circuit::const_pin_iterator p = c->begin_pins (); p != c->end_pins (); ++p) {
      if (! ps.empty ()) {
        ps += ",";
      }
      ps += pin2string (*p) + "=" + net2string (c->net_for_pin (p->id ()));
    }

    res += std::string ("circuit ") + tl::to_word_or_quoted_string (c->name ()) + " (" + ps + ");\n";

    for (db::Circuit::const_device_iterator d = c->begin_devices (); d != c->end_devices (); ++d) {
      std::string ts;
      const std::vector<db::DeviceTerminalDefinition> &td = d->device_class ()->terminal_definitions ();
      for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
        if (t != td.begin ()) {
          ts += ",";
        }
        ts += t->name () + "=" + net2string (d->net_for_terminal (t->id ()));
      }
      std::string ps;
      const std::vector<db::DeviceParameterDefinition> &pd = d->device_class ()->parameter_definitions ();
      for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
        if (p != pd.begin ()) {
          ps += ",";
        }
        ps += p->name () + "=" + tl::sprintf ("%.12g", d->parameter_value (p->id ()));
      }
      res += std::string ("  device ") + tl::to_word_or_quoted_string (d->device_class ()->name ()) + " " + device2string (*d) + " (" + ts + ") (" + ps + ");\n";
    }

    for (db::Circuit::const_subcircuit_iterator sc = c->begin_subcircuits (); sc != c->end_subcircuits (); ++sc) {
      std::string ps;
      const db::SubCircuit &subcircuit = *sc;
      const db::Circuit *circuit = sc->circuit_ref ();
      if (circuit) {
        for (db::Circuit::const_pin_iterator p = circuit->begin_pins (); p != circuit->end_pins (); ++p) {
          if (p != circuit->begin_pins ()) {
            ps += ",";
          }
          ps += pin2string (*p) + "=" + net2string (subcircuit.net_for_pin (p->id ()));
        }
        res += std::string ("  subcircuit ") + tl::to_word_or_quoted_string (circuit->name ()) + " " + subcircuit2string (*sc) + " (" + ps + ");\n";
      } else {
        res += std::string ("  subcircuit (null);\n");
      }
    }

    res += std::string ("end;\n");

  }

  return res;
}

static db::Net *read_net (tl::Extractor &ex, db::Circuit *circuit, std::map<std::string, db::Net *> &n2n)
{
  std::string nn;
  bool has_name = false;
  size_t cluster_id = 0;

  if (ex.test ("(")) {

    ex.expect ("null");
    ex.expect (")");

    return 0;

  } else if (ex.test ("$")) {

    bool has_i = ex.test ("I");
    ex.read (cluster_id);

    nn = (has_i ? "$I" : "$") + tl::to_string (cluster_id);

    if (has_i) {
      cluster_id = (std::numeric_limits<size_t>::max () - cluster_id) + 1;
    }

  } else {

    ex.read_word_or_quoted (nn);

    has_name = true;

  }

  std::map<std::string, db::Net *>::const_iterator i = n2n.find (nn);
  if (i == n2n.end ()) {

    db::Net *net = new db::Net ();
    circuit->add_net (net);
    if (has_name) {
      net->set_name (nn);
    } else {
      net->set_cluster_id (cluster_id);
    }

    n2n.insert (std::make_pair (nn, net));
    return net;

  } else {

    return i->second;

  }
}

static void read_pins (tl::Extractor &ex, db::Circuit *circuit, std::map<std::string, db::Net *> &n2n)
{
  std::vector<std::string> org_pins;
  for (db::Circuit::const_pin_iterator p = circuit->begin_pins (); p != circuit->end_pins (); ++p) {
    org_pins.push_back (p->name ());
  }

  circuit->clear_pins ();

  ex.expect ("(");
  while (! ex.test (")")) {

    ex.expect_more ();

    std::string pn;
    if (ex.test ("$")) {
      size_t i;
      ex.read (i);
    } else {
      ex.read_word_or_quoted (pn);
    }

    ex.expect ("=");

    db::Net *net = read_net (ex, circuit, n2n);

    if (circuit->pin_count () < org_pins.size () && pn != org_pins [circuit->pin_count ()]) {
      ex.error (tl::sprintf (tl::to_string (tr ("Circuit defines different name for pin than subcircuit: %s (circuit) vs. %s (subcircuit)")), pn, org_pins [circuit->pin_count ()]));
    }

    const db::Pin &pin = circuit->add_pin (pn);
    if (net) {
      net->add_pin (db::NetPinRef (pin.id ()));
    }

    ex.test (",");

  }

  if (circuit->pin_count () < org_pins.size ()) {
    ex.error (tl::to_string (tr ("Circuit defines less pins that subcircuit")));
  } else if (org_pins.size () > 0 && circuit->pin_count () > org_pins.size ()) {
    ex.error (tl::to_string (tr ("Circuit defines more pins that subcircuit")));
  }
}

static void read_device_terminals (tl::Extractor &ex, db::Device *device, std::map<std::string, db::Net *> &n2n)
{
  ex.expect ("(");
  while (! ex.test (")")) {

    ex.expect_more ();

    std::string tn;
    ex.read_word_or_quoted (tn);

    size_t tid = std::numeric_limits<size_t>::max ();
    const std::vector<DeviceTerminalDefinition> &td = device->device_class ()->terminal_definitions ();
    for (std::vector<DeviceTerminalDefinition>::const_iterator i = td.begin (); i != td.end (); ++i) {
      if (i->name () == tn) {
        tid = i->id ();
        break;
      }
    }

    if (tid == std::numeric_limits<size_t>::max ()) {
      ex.error (tl::to_string (tr ("Not a valid terminal name: ")) + tn);
    }

    ex.expect ("=");

    db::Net *net = read_net (ex, device->circuit (), n2n);
    if (net) {
      device->connect_terminal (tid, net);
    }

    ex.test (",");

  }
}

static void read_device_parameters (tl::Extractor &ex, db::Device *device)
{
  if (! ex.test ("(")) {
    return;
  }

  while (! ex.test (")")) {

    ex.expect_more ();

    std::string pn;
    ex.read_word_or_quoted (pn);

    size_t pid = std::numeric_limits<size_t>::max ();
    const std::vector<DeviceParameterDefinition> &pd = device->device_class ()->parameter_definitions ();
    for (std::vector<DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
      if (i->name () == pn) {
        pid = i->id ();
        break;
      }
    }

    if (pid == std::numeric_limits<size_t>::max ()) {
      ex.error (tl::to_string (tr ("Not a valid parameter name: ")) + pn);
    }

    ex.expect ("=");

    double value = 0;
    ex.read (value);
    device->set_parameter_value (pid, value);

    ex.test (",");

  }
}

static void read_device (tl::Extractor &ex, db::Circuit *circuit, std::map<std::string, db::Net *> &n2n)
{
  db::Netlist *netlist = circuit->netlist ();

  std::string dcn;
  ex.read_word_or_quoted (dcn);
  db::DeviceClass *dc = 0;
  for (db::Netlist::device_class_iterator i = netlist->begin_device_classes (); i != netlist->end_device_classes (); ++i) {
    if (i->name () == dcn) {
      dc = i.operator-> ();
    }
  }
  if (! dc) {
    ex.error (tl::to_string (tr ("Not a valid device class name: ")) + dcn);
  }

  std::string dn;
  if (ex.test ("$")) {
    size_t i;
    ex.read (i);
  } else {
    ex.read_word_or_quoted (dn);
  }

  db::Device *device = new db::Device (dc, dn);
  circuit->add_device (device);

  read_device_terminals (ex, device, n2n);
  read_device_parameters (ex, device);
}

static void read_subcircuit_pins (tl::Extractor &ex, db::Circuit *circuit, db::SubCircuit *subcircuit, std::map<std::string, db::Net *> &n2n)
{
  db::Circuit *circuit_ref = subcircuit->circuit_ref ();
  db::Circuit::pin_iterator pi = circuit_ref->begin_pins ();

  ex.expect ("(");
  while (! ex.test (")")) {

    std::string pn;
    if (ex.test ("$")) {
      size_t i;
      ex.read (i);
    } else {
      ex.read_word_or_quoted (pn);
    }

    ex.expect ("=");

    if (pi == circuit_ref->end_pins ()) {
      //  add a dummy pin
      circuit_ref->add_pin (pn);
      pi = circuit_ref->end_pins ();
      --pi;
    } else if (! pi->name ().empty () && pi->name () != pn) {
      ex.error (tl::to_string (tr ("Expected pin with name: ")) + pi->name ());
    }

    ex.expect_more ();

    db::Net *net = read_net (ex, circuit, n2n);
    if (net) {
      subcircuit->connect_pin (pi->id (), net);
    }

    ex.test (",");

    ++pi;

  }

  if (pi != circuit_ref->end_pins ()) {
    ex.error (tl::to_string (tr ("Too few pins in subcircuit call")));
  }
}

static void read_subcircuit (tl::Extractor &ex, db::Circuit *circuit, std::map<std::string, db::Net *> &n2n, std::map<std::string, db::Circuit *> &c2n)
{
  std::string cn;
  ex.read_word_or_quoted (cn);

  db::Circuit *cc = 0;
  std::map<std::string, db::Circuit *>::const_iterator ic = c2n.find (cn);
  if (ic == c2n.end ()) {

    cc = new db::Circuit ();
    circuit->netlist ()->add_circuit (cc);
    cc->set_name (cn);

    c2n.insert (std::make_pair (cn, cc));

  } else {
    cc = ic->second;
  }

  std::string scn;
  if (ex.test ("$")) {
    size_t i;
    ex.read (i);
  } else {
    ex.read_word_or_quoted (scn);
  }

  db::SubCircuit *subcircuit = new db::SubCircuit (cc, scn);
  circuit->add_subcircuit (subcircuit);

  read_subcircuit_pins (ex, circuit, subcircuit, n2n);
}

void Netlist::from_string (const std::string &s)
{
  tl::Extractor ex (s.c_str ());

  std::map<std::string, db::Circuit *> c2n;

  while (ex.test ("circuit")) {

    std::string n;
    ex.read_word_or_quoted (n);

    db::Circuit *circuit = 0;

    std::map<std::string, db::Circuit *>::const_iterator ic = c2n.find (n);
    if (ic == c2n.end ()) {

      circuit = new db::Circuit ();
      add_circuit (circuit);
      circuit->set_name (n);

      c2n.insert (std::make_pair (n, circuit));

    } else {
      circuit = ic->second;
    }

    std::map<std::string, db::Net *> n2n;
    read_pins (ex, circuit, n2n);

    ex.expect (";");

    while (! ex.test ("end")) {

      ex.expect_more ();

      if (ex.test ("device")) {

        read_device (ex, circuit, n2n);
        ex.expect (";");

      } else if (ex.test ("subcircuit")) {

        read_subcircuit (ex, circuit, n2n, c2n);
        ex.expect (";");

      } else {
        ex.error (tl::to_string (tr ("device or subcircuit expected")));
      }

    }

    ex.expect (";");

  }

  ex.expect_end ();
}

}

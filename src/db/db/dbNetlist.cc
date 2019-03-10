
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

#include "dbNetlist.h"

#include <set>

namespace db
{

// --------------------------------------------------------------------------------
//  Netlist class implementation

Netlist::Netlist ()
  : m_valid_topology (false), m_lock_count (0),
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
  : gsi::ObjectBase (other), tl::Object (other), m_valid_topology (false), m_lock_count (0),
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
  if (! m_valid_topology) {
    validate_topology ();
  }

  tl_assert (circuit->index () < m_child_circuits.size ());
  return m_child_circuits [circuit->index ()];
}

const tl::vector<Circuit *> &Netlist::parent_circuits (Circuit *circuit)
{
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
  m_circuits.push_back (circuit);
  circuit->set_netlist (this);
}

void Netlist::remove_circuit (Circuit *circuit)
{
  circuit->set_netlist (0);
  m_circuits.erase (circuit);
}

void Netlist::add_device_class (DeviceClass *device_class)
{
  m_device_classes.push_back (device_class);
  device_class->set_netlist (this);
}

void Netlist::remove_device_class (DeviceClass *device_class)
{
  device_class->set_netlist (0);
  m_device_classes.erase (device_class);
}

void Netlist::add_device_abstract (DeviceAbstract *device_abstract)
{
  m_device_abstracts.push_back (device_abstract);
  device_abstract->set_netlist (this);
}

void Netlist::remove_device_abstract (DeviceAbstract *device_abstract)
{
  device_abstract->set_netlist (0);
  m_device_abstracts.erase (device_abstract);
}

void Netlist::purge_nets ()
{
  for (circuit_iterator c = begin_circuits (); c != end_circuits (); ++c) {
    c->purge_nets ();
  }
}

void Netlist::make_top_level_pins ()
{
  size_t ntop = top_circuit_count ();
  for (top_down_circuit_iterator c = begin_top_down (); c != end_top_down () && ntop > 0; ++c, --ntop) {

    Circuit *circuit = *c;

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

    Circuit *circuit = *c;

    circuit->purge_nets ();
    if (circuit->begin_nets () == circuit->end_nets ()) {

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

    res += std::string ("Circuit ") + c->name () + " (" + ps + "):\n";

#if 0  //  for debugging
    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
      res += "  N" + net_name (n.operator-> ()) + " pins=" + tl::to_string (n->pin_count ())  + " sc_pins=" + tl::to_string (n->subcircuit_pin_count ()) + " terminals=" + tl::to_string (n->terminal_count ()) + "\n";
    }
#endif

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
        ps += p->name () + "=" + tl::to_string (d->parameter_value (p->id ()));
      }
      res += std::string ("  D") + d->device_class ()->name () + " " + device2string (*d) + " (" + ts + ") [" + ps + "]\n";
    }

    for (db::Circuit::const_subcircuit_iterator sc = c->begin_subcircuits (); sc != c->end_subcircuits (); ++sc) {
      std::string ps;
      const db::SubCircuit &subcircuit = *sc;
      const db::Circuit *circuit = sc->circuit_ref ();
      for (db::Circuit::const_pin_iterator p = circuit->begin_pins (); p != circuit->end_pins (); ++p) {
        if (p != circuit->begin_pins ()) {
          ps += ",";
        }
        const db::Pin &pin = *p;
        ps += pin2string (pin) + "=" + net2string (subcircuit.net_for_pin (pin.id ()));
      }
      res += std::string ("  X") + circuit->name () + " " + subcircuit2string (*sc) + " (" + ps + ")\n";
    }

  }

  return res;
}

}

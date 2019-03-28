

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

#include "dbNetlistCompare.h"
#include "dbHash.h"
#include "tlProgress.h"
#include "tlTimer.h"

namespace db
{

// --------------------------------------------------------------------------------------------------------------------
//  DeviceCompare definition and implementation

struct DeviceCompare
{
  bool operator() (const std::pair<const db::Device *, size_t> &d1, const std::pair<const db::Device *, size_t> &d2) const
  {
    if (d1.second != d2.second) {
      return d1.second < d2.second;
    }

    const std::vector<db::DeviceParameterDefinition> &dp = d1.first->device_class ()->parameter_definitions ();
    for (std::vector<db::DeviceParameterDefinition>::const_iterator i = dp.begin (); i != dp.end (); ++i) {
      double v1 = d1.first->parameter_value (i->id ());
      double v2 = d2.first->parameter_value (i->id ());
      if (fabs (v1 - v2) > db::epsilon) {
        return v1 < v2;
      }
    }
    return false;
  }

  bool equals (const std::pair<const db::Device *, size_t> &d1, const std::pair<const db::Device *, size_t> &d2) const
  {
    if (d1.second != d2.second) {
      return false;
    }

    const std::vector<db::DeviceParameterDefinition> &dp = d1.first->device_class ()->parameter_definitions ();
    for (std::vector<db::DeviceParameterDefinition>::const_iterator i = dp.begin (); i != dp.end (); ++i) {
      double v1 = d1.first->parameter_value (i->id ());
      double v2 = d2.first->parameter_value (i->id ());
      if (fabs (v1 - v2) > db::epsilon) {
        return false;
      }
    }
    return true;
  }
};

// --------------------------------------------------------------------------------------------------------------------
//  SubCircuitCompare definition and implementation

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

class CircuitPinMapper
{
public:
  CircuitPinMapper ()
  {
    //  .. nothing yet ..
  }

  void map_pins (const db::Circuit *circuit, size_t pin1_id, size_t pin2_id)
  {
    m_pin_map [circuit].insert (std::make_pair (pin1_id, pin2_id));
  }

  void map_pins (const db::Circuit *circuit, const std::vector<size_t> &pin_ids)
  {
    if (pin_ids.size () < 2) {
      return;
    }

    std::map<size_t, size_t> &pm = m_pin_map [circuit];
    for (size_t i = 1; i < pin_ids.size (); ++i) {
      pm.insert (std::make_pair (pin_ids [i], pin_ids [0]));
    }
  }

  size_t normalize_pin_id (const db::Circuit *circuit, size_t pin_id) const
  {
    std::map<const db::Circuit *, std::map<size_t, size_t> >::const_iterator pm = m_pin_map.find (circuit);
    if (pm != m_pin_map.end ()) {
      std::map<size_t, size_t>::const_iterator ipm = pm->second.find (pin_id);
      if (ipm != pm->second.end ()) {
        return ipm->second;
      }
    }
    return pin_id;
  }

private:
  std::map<const db::Circuit *, std::map<size_t, size_t> > m_pin_map;
};

// --------------------------------------------------------------------------------------------------------------------
//  CircuitMapper definition and implementation

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
//  DeviceCategorizer definition and implementation

class DeviceCategorizer
{
public:
  DeviceCategorizer ()
    : m_next_cat (0)
  {
    //  .. nothing yet ..
  }

  void same_class (const db::DeviceClass *ca, const db::DeviceClass *cb)
  {
    ++m_next_cat;
    m_cat_by_ptr.insert (std::make_pair (ca, m_next_cat));
    m_cat_by_ptr.insert (std::make_pair (cb, m_next_cat));
  }

  size_t cat_for_device (const db::Device *device)
  {
    const db::DeviceClass *cls = device->device_class ();
    if (! cls) {
      return 0;
    }

    std::map<const db::DeviceClass *, size_t>::const_iterator cp = m_cat_by_ptr.find (cls);
    if (cp != m_cat_by_ptr.end ()) {
      return cp->second;
    }

    std::map<std::string, size_t>::const_iterator c = m_cat_by_name.find (cls->name ());
    if (c != m_cat_by_name.end ()) {
      m_cat_by_ptr.insert (std::make_pair (cls, c->second));
      return c->second;
    } else {
      ++m_next_cat;
      m_cat_by_name.insert (std::make_pair (cls->name (), m_next_cat));
      m_cat_by_ptr.insert (std::make_pair (cls, m_next_cat));
      return m_next_cat;
    }
  }

public:
  std::map<const db::DeviceClass *, size_t> m_cat_by_ptr;
  std::map<std::string, size_t> m_cat_by_name;
  size_t m_next_cat;
};

// --------------------------------------------------------------------------------------------------------------------
//  CircuitCategorizer definition and implementation

class CircuitCategorizer
{
public:
  CircuitCategorizer ()
    : m_next_cat (0)
  {
    //  .. nothing yet ..
  }

  void same_circuit (const db::Circuit *ca, const db::Circuit *cb)
  {
    ++m_next_cat;
    m_cat_by_ptr.insert (std::make_pair (ca, m_next_cat));
    m_cat_by_ptr.insert (std::make_pair (cb, m_next_cat));
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
    std::map<const db::Circuit *, size_t>::const_iterator cp = m_cat_by_ptr.find (cr);
    if (cp != m_cat_by_ptr.end ()) {
      return cp->second;
    }

    std::map<std::string, size_t>::const_iterator c = m_cat_by_name.find (cr->name ());
    if (c != m_cat_by_name.end ()) {
      m_cat_by_ptr.insert (std::make_pair (cr, c->second));
      return c->second;
    } else {
      ++m_next_cat;
      m_cat_by_name.insert (std::make_pair (cr->name (), m_next_cat));
      m_cat_by_ptr.insert (std::make_pair (cr, m_next_cat));
      return m_next_cat;
    }
  }

public:
  std::map<const db::Circuit *, size_t> m_cat_by_ptr;
  std::map<std::string, size_t> m_cat_by_name;
  size_t m_next_cat;
};

// --------------------------------------------------------------------------------------------------------------------
//  NetDeviceGraphNode definition and implementation

static size_t translate_terminal_id (size_t tid, const db::Device *device)
{
  return device->device_class () ? device->device_class ()->normalize_terminal_id (tid) : tid;
}

class NetDeviceGraphNode
{
public:
  struct EdgeDesc {

    EdgeDesc (const db::Device *device, size_t device_category, size_t terminal1_id, size_t terminal2_id)
    {
      device_pair ().first = device;
      device_pair ().second = device_category;
      m_id1 = terminal1_id;
      m_id2 = terminal2_id;
    }

    EdgeDesc (const db::SubCircuit *subcircuit, size_t subcircuit_category, size_t pin1_id, size_t pin2_id)
    {
      subcircuit_pair ().first = subcircuit;
      subcircuit_pair ().second = subcircuit_category;
      m_id1 = std::numeric_limits<size_t>::max () - pin1_id;
      m_id2 = pin2_id;
    }

    bool operator< (const EdgeDesc &other) const
    {
      if (m_id1 > std::numeric_limits<size_t>::max () / 2) {

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

    bool operator== (const EdgeDesc &other) const
    {
      if (m_id1 > std::numeric_limits<size_t>::max () / 2) {

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

  private:
    char m_ref [sizeof (std::pair<const void *, size_t>)];
    size_t m_id1, m_id2;

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
  };

  struct EdgeToEdgeOnlyCompare
  {
    bool operator() (const std::pair<std::vector<EdgeDesc>, std::pair<size_t, const db::Net *> > &a, const std::vector<EdgeDesc> &b) const
    {
      return a.first < b;
    }
  };

  typedef std::vector<std::pair<std::vector<EdgeDesc>, std::pair<size_t, const db::Net *> > >::const_iterator edge_iterator;

  NetDeviceGraphNode (const db::Net *net, DeviceCategorizer &device_categorizer, CircuitCategorizer &circuit_categorizer, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinMapper *pin_map)
    : mp_net (net), m_other_net_index (std::numeric_limits<size_t>::max ())
  {
    std::map<const db::Net *, size_t> n2entry;

    for (db::Net::const_subcircuit_pin_iterator i = net->begin_subcircuit_pins (); i != net->end_subcircuit_pins (); ++i) {

      const db::SubCircuit *sc = i->subcircuit ();
      size_t pin_id = i->pin ()->id ();
      const db::Circuit *cr = sc->circuit_ref ();

      const CircuitMapper *cm = 0;

      if (circuit_map) {
        std::map<const db::Circuit *, CircuitMapper>::const_iterator icm = circuit_map->find (cr);
        if (icm == circuit_map->end ()) {
          //  this can happen if the other circuit is not present - this is allowed for single-pin
          //  circuits.
          continue;
        }
        cm = & icm->second;
      }

      //  NOTE: if cm is given, cr and pin_id are given in terms of the "other" circuit

      if (cm) {
        cr = cm->other ();
        pin_id = cm->other_pin_from_this_pin (pin_id);
      }

      if (pin_map) {
        pin_id = pin_map->normalize_pin_id (cr, pin_id);
      }

      //  we cannot afford creating edges from all to all other pins, so we just create edges to the previous and next
      //  pin. This may take more iterations to solve, but should be equivalent.

      std::vector<size_t> pids;
      size_t pin_count = cr->pin_count ();

      //  take the previous, next and second-next pin as targets for edges
      //  (using the second-next pin avoid isolation of OUT vs. IN in case
      //  of a pin configuration of VSS-IN-VDD-OUT like for an inverter).

      if (pin_count >= 2) {
        pids.push_back ((pin_id + pin_count - 1) % pin_count);
        if (pin_count >= 3) {
          pids.push_back ((pin_id + 1) % pin_count);
          if (pin_count >= 4) {
            pids.push_back ((pin_id + 2) % pin_count);
          }
        }
      }

      for (std::vector<size_t>::const_iterator i = pids.begin (); i != pids.end (); ++i) {

        size_t pin2_id = *i;

        //  NOTE: if a pin mapping is given, EdgeDesc::pin1_id and EdgeDesc::pin2_id are given
        //  as pin ID's of the other circuit.
        EdgeDesc ed (sc, circuit_categorizer.cat_for_subcircuit (sc), pin_id, pin_map ? pin_map->normalize_pin_id (cr, pin2_id) : pin2_id);

        size_t this_pin2_id = cm ? cm->this_pin_from_other_pin (pin2_id) : pin2_id;
        const db::Net *net2 = sc->net_for_pin (this_pin2_id);

        std::map<const db::Net *, size_t>::const_iterator in = n2entry.find (net2);
        if (in == n2entry.end ()) {
          in = n2entry.insert (std::make_pair (net2, m_edges.size ())).first;
          m_edges.push_back (std::make_pair (std::vector<EdgeDesc> (), std::make_pair (size_t (0), net2)));
        }

        m_edges [in->second].first.push_back (ed);

      }

    }

    for (db::Net::const_terminal_iterator i = net->begin_terminals (); i != net->end_terminals (); ++i) {

      const db::Device *d = i->device ();
      size_t device_cat = device_categorizer.cat_for_device (d);
      size_t terminal1_id = translate_terminal_id (i->terminal_id (), d);

      const std::vector<db::DeviceTerminalDefinition> &td = d->device_class ()->terminal_definitions ();
      for (std::vector<db::DeviceTerminalDefinition>::const_iterator it = td.begin (); it != td.end (); ++it) {

        if (it->id () != i->terminal_id ()) {

          size_t terminal2_id = translate_terminal_id (it->id (), d);
          EdgeDesc ed2 (d, device_cat, terminal1_id, terminal2_id);

          const db::Net *net2 = d->net_for_terminal (it->id ());

          std::map<const db::Net *, size_t>::const_iterator in = n2entry.find (net2);
          if (in == n2entry.end ()) {
            in = n2entry.insert (std::make_pair (net2, m_edges.size ())).first;
            m_edges.push_back (std::make_pair (std::vector<EdgeDesc> (), std::make_pair (size_t (0), net2)));
          }

          m_edges [in->second].first.push_back (ed2);

        }

      }

    }
  }

  const db::Net *net () const
  {
    return mp_net;
  }

  bool has_other () const
  {
    return m_other_net_index != std::numeric_limits<size_t>::max ();
  }

  size_t other_net_index () const
  {
    return m_other_net_index;
  }

  void set_other_net (size_t index)
  {
    m_other_net_index = index;
  }

  void apply_net_index (const std::map<const db::Net *, size_t> &ni)
  {
    for (std::vector<std::pair<std::vector<EdgeDesc>, std::pair<size_t, const db::Net *> > >::iterator i = m_edges.begin (); i != m_edges.end (); ++i) {
      std::map<const db::Net *, size_t>::const_iterator j = ni.find (i->second.second);
      tl_assert (j != ni.end ());
      i->second.first = j->second;
    }

    //  "deep sorting" of the edge descriptor
    for (std::vector<std::pair<std::vector<EdgeDesc>, std::pair<size_t, const db::Net *> > >::iterator i = m_edges.begin (); i != m_edges.end (); ++i) {
      std::sort (i->first.begin (), i->first.end ());
    }

    std::sort (m_edges.begin (), m_edges.end ());
  }

  bool operator< (const NetDeviceGraphNode &node) const
  {
    if (m_edges.size () != node.m_edges.size ()) {
      return m_edges.size () < node.m_edges.size ();
    }
    for (size_t i = 0; i < m_edges.size (); ++i) {
      if (m_edges [i].first != node.m_edges [i].first) {
        return m_edges [i].first < node.m_edges [i].first;
      }
    }
    return false;
  }

  bool operator== (const NetDeviceGraphNode &node) const
  {
    if (m_edges.size () != node.m_edges.size ()) {
      return false;
    }
    for (size_t i = 0; i < m_edges.size (); ++i) {
      if (m_edges [i].first != node.m_edges [i].first) {
        return false;
      }
    }
    return true;
  }

  void swap (NetDeviceGraphNode &other)
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

  edge_iterator find_edge (const std::vector<EdgeDesc> &edge) const
  {
    edge_iterator res = std::lower_bound (begin (), end (), edge, NetDeviceGraphNode::EdgeToEdgeOnlyCompare ());
    if (res == end () || res->first != edge) {
      return end ();
    } else {
      return res;
    }
  }

private:
  const db::Net *mp_net;
  size_t m_other_net_index;
  std::vector<std::pair<std::vector<EdgeDesc>, std::pair<size_t, const db::Net *> > > m_edges;
};

// --------------------------------------------------------------------------------------------------------------------
//  NetDeviceGraph definition and implementation

}

namespace std
{
  void swap (db::NetDeviceGraphNode &a, db::NetDeviceGraphNode &b)
  {
    a.swap (b);
  }
}

namespace db
{

class NetDeviceGraph
{
public:
  typedef std::vector<NetDeviceGraphNode>::const_iterator node_iterator;

  NetDeviceGraph ()
  {
    //  .. nothing yet ..
  }

  void build (const db::Circuit *c, DeviceCategorizer &device_categorizer, CircuitCategorizer &circuit_categorizer, const std::map<const db::Circuit *, CircuitMapper> *circuit_and_pin_mapping, const CircuitPinMapper *circuit_pin_mapper)
  {
    tl::SelfTimer timer (tl::verbosity () >= 31, tl::to_string (tr ("Building net graph for circuit: ")) + c->name ());

    m_nodes.clear ();
    m_net_index.clear ();

    size_t nets = 0;
    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
      ++nets;
    }
    m_nodes.reserve (nets);

    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
      NetDeviceGraphNode node (n.operator-> (), device_categorizer, circuit_categorizer, circuit_and_pin_mapping, circuit_pin_mapper);
      m_nodes.push_back (node);
    }

    std::sort (m_nodes.begin (), m_nodes.end ());

    for (std::vector<NetDeviceGraphNode>::const_iterator i = m_nodes.begin (); i != m_nodes.end (); ++i) {
      m_net_index.insert (std::make_pair (i->net (), i - m_nodes.begin ()));
    }
    for (std::vector<NetDeviceGraphNode>::iterator i = m_nodes.begin (); i != m_nodes.end (); ++i) {
      i->apply_net_index (m_net_index);
    }
  }

  size_t node_index_for_net (const db::Net *net) const
  {
    std::map<const db::Net *, size_t>::const_iterator j = m_net_index.find (net);
    tl_assert (j != m_net_index.end ());
    return j->second;
  }

  const db::Net *net_by_node_index (size_t net_index) const
  {
    return m_nodes [net_index].net ();
  }

  void identify (size_t net_index, size_t other_net_index)
  {
    m_nodes [net_index].set_other_net (other_net_index);
  }

  node_iterator begin () const
  {
    return m_nodes.begin ();
  }

  node_iterator end () const
  {
    return m_nodes.end ();
  }

  size_t derive_node_identities (size_t net_index, NetDeviceGraph &other, NetlistCompareLogger *logger)
  {
    size_t added = 0;

    std::vector<size_t> todo, more;
    more.push_back (net_index);

    while (! more.empty ()) {

      todo.swap (more);
      more.clear ();

      for (std::vector<size_t>::const_iterator index = todo.begin (); index != todo.end (); ++index) {

        NetDeviceGraphNode *n = & m_nodes[*index];
        NetDeviceGraphNode *nother = & other.m_nodes[n->other_net_index ()];

        //  non-ambiguous paths to non-assigned nodes create a node identity on the
        //  end of this path

        for (NetDeviceGraphNode::edge_iterator e = n->begin (); e != n->end (); ) {

          NetDeviceGraphNode::edge_iterator ee = e;
          ++ee;

          while (ee != n->end () && ee->first == e->first) {
            ++ee;
          }

          size_t count = 0;
          NetDeviceGraphNode::edge_iterator ec;
          for (NetDeviceGraphNode::edge_iterator i = e; i != ee; ++i) {
            if (! m_nodes[i->second.first].has_other ()) {
              ec = i;
              ++count;
            }
          }

          if (count == 1) {   //  if non-ambiguous, non-assigned

#if defined(PRINT_DEBUG_NETCOMPARE)
            tl::log << "considering " << n->net ()->expanded_name () << " to " << ec->second.second->expanded_name ();
#endif

            NetDeviceGraphNode::edge_iterator e_other = nother->find_edge (ec->first);
            if (e_other != nother->end ()) {

#if defined(PRINT_DEBUG_NETCOMPARE)
              tl::log << "candidate accepted";
#endif
              NetDeviceGraphNode::edge_iterator ee_other = e_other;
              ++ee_other;

              while (ee_other != nother->end () && ee_other->first == e_other->first) {
                ++ee_other;
              }

              size_t count_other = 0;
              NetDeviceGraphNode::edge_iterator ec_other;
              for (NetDeviceGraphNode::edge_iterator i = e_other; i != ee_other; ++i) {
                if (! other.m_nodes[i->second.first].has_other ()) {
                  ec_other = i;
                  ++count_other;
                }
              }

#if defined(PRINT_DEBUG_NETCOMPARE)
              tl::log << "identity count = " << count_other;
#endif
              if (count_other == 1) {
                confirm_identity (*this, begin () + ec->second.first, other, other.begin () + ec_other->second.first, logger);
                ++added;
                more.push_back (ec->second.first);
              }

            }

          }

          e = ee;

        }

      }

    }

    return added;
  }

  static void confirm_identity (db::NetDeviceGraph &g1, db::NetDeviceGraph::node_iterator s1, db::NetDeviceGraph &g2, db::NetDeviceGraph::node_iterator s2, db::NetlistCompareLogger *logger, bool ambiguous = false)
  {
    if (logger) {
      if (ambiguous) {
        logger->match_ambiguous_nets (s1->net (), s2->net ());
      } else {
        logger->match_nets (s1->net (), s2->net ());
      }
    }
    g1.identify (s1 - g1.begin (), s2 - g2.begin ());
    g2.identify (s2 - g2.begin (), s1 - g1.begin ());
  }

private:
  std::vector<NetDeviceGraphNode> m_nodes;
  std::map<const db::Net *, size_t> m_net_index;
};

// --------------------------------------------------------------------------------------------------------------------
//  NetlistComparer implementation

NetlistComparer::NetlistComparer (NetlistCompareLogger *logger)
  : mp_logger (logger)
{
  mp_device_categorizer.reset (new DeviceCategorizer ());
  mp_circuit_categorizer.reset (new CircuitCategorizer ());
  mp_circuit_pin_mapper.reset (new CircuitPinMapper ());
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
NetlistComparer::compare (const db::Netlist *a, const db::Netlist *b) const
{
  //  we need to create a copy because this method is supposed to be const.
  db::CircuitCategorizer circuit_categorizer = *mp_circuit_categorizer;
  db::DeviceCategorizer device_categorizer = *mp_device_categorizer;

  bool good = true;

  std::map<size_t, std::pair<const db::Circuit *, const db::Circuit *> > cat2circuits;

  for (db::Netlist::const_circuit_iterator i = a->begin_circuits (); i != a->end_circuits (); ++i) {
    size_t cat = circuit_categorizer.cat_for_circuit (i.operator-> ());
    cat2circuits[cat].first = i.operator-> ();
  }

  for (db::Netlist::const_circuit_iterator i = b->begin_circuits (); i != b->end_circuits (); ++i) {
    size_t cat = circuit_categorizer.cat_for_circuit (i.operator-> ());
    cat2circuits[cat].second = i.operator-> ();
  }

  if (mp_logger) {
    mp_logger->begin_netlist (a, b);
  }

  for (std::map<size_t, std::pair<const db::Circuit *, const db::Circuit *> >::const_iterator i = cat2circuits.begin (); i != cat2circuits.end (); ++i) {
    if (! i->second.first || ! i->second.second) {
      good = false;
      if (mp_logger) {
        mp_logger->circuit_mismatch (i->second.first, i->second.second);
      }
    }
  }

  std::set<const db::Circuit *> verified_circuits_a, verified_circuits_b;
  std::map<const db::Circuit *, CircuitMapper> pin_mapping;

  for (db::Netlist::const_bottom_up_circuit_iterator c = a->begin_bottom_up (); c != a->end_bottom_up (); ++c) {

    size_t ccat = circuit_categorizer.cat_for_circuit (*c);

    std::map<size_t, std::pair<const db::Circuit *, const db::Circuit *> >::const_iterator i = cat2circuits.find (ccat);
    tl_assert (i != cat2circuits.end ());

    if (i->second.first && i->second.second) {

      const db::Circuit *ca = i->second.first;
      const db::Circuit *cb = i->second.second;

      std::vector<std::pair<const Net *, const Net *> > empty;
      const std::vector<std::pair<const Net *, const Net *> > *net_identity = &empty;
      std::map<std::pair<const db::Circuit *, const db::Circuit *>, std::vector<std::pair<const Net *, const Net *> > >::const_iterator sn = m_same_nets.find (std::make_pair (ca, cb));
      if (sn != m_same_nets.end ()) {
        net_identity = &sn->second;
      }

      if (all_subcircuits_verified (ca, verified_circuits_a) && all_subcircuits_verified (cb, verified_circuits_b)) {

        if (mp_logger) {
          mp_logger->begin_circuit (ca, cb);
        }

        bool pin_mismatch = false;
        bool g = compare_circuits (ca, cb, device_categorizer, circuit_categorizer, *net_identity, pin_mismatch, pin_mapping);
        if (! g) {
          good = false;
        }

        if (! pin_mismatch) {
          verified_circuits_a.insert (ca);
          verified_circuits_b.insert (cb);
        }

        if (mp_logger) {
          mp_logger->end_circuit (ca, cb, g);
        }

      } else {

        if (mp_logger) {
          mp_logger->circuit_skipped (ca, cb);
        }

      }

    }

  }

  if (mp_logger) {
    mp_logger->begin_netlist (a, b);
  }

  return good;
}

bool
NetlistComparer::all_subcircuits_verified (const db::Circuit *c, const std::set<const db::Circuit *> &verified_circuits) const
{
  for (db::Circuit::const_subcircuit_iterator sc = c->begin_subcircuits (); sc != c->end_subcircuits (); ++sc) {

    const db::Circuit *cr = sc->circuit_ref ();

    //  typical via subcircuits attach through one pin. We can safely ignore such subcircuits because they don't
    //  contribute graph edges.
    if (cr->pin_count () > 1 && verified_circuits.find (cr) == verified_circuits.end ()) {
      return false;
    }

  }

  return true;
}

static std::vector<std::pair<size_t, size_t> >
compute_device_key (const db::Device &device, const db::NetDeviceGraph &g)
{
  std::vector<std::pair<size_t, size_t> > k;

  const std::vector<db::DeviceTerminalDefinition> &td = device.device_class ()->terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
    size_t terminal_id = translate_terminal_id (t->id (), &device);
    const db::Net *net = device.net_for_terminal (t->id ());
    size_t net_id = g.node_index_for_net (net);
    k.push_back (std::make_pair (terminal_id, net_id));
  }

  std::sort (k.begin (), k.end ());

  return k;
}

static std::vector<std::pair<size_t, size_t> >
compute_subcircuit_key (const db::SubCircuit &subcircuit, const db::NetDeviceGraph &g, const std::map<const db::Circuit *, CircuitMapper> *circuit_map, const CircuitPinMapper *pin_map)
{
  std::vector<std::pair<size_t, size_t> > k;

  const db::Circuit *cr = subcircuit.circuit_ref ();

  const CircuitMapper *cm = 0;

  if (circuit_map) {
    std::map<const db::Circuit *, CircuitMapper>::const_iterator icm = circuit_map->find (cr);
    if (icm == circuit_map->end ()) {
      //  this can happen if the other circuit does not exist - in this case the key is an invalid one which cannot
      //  be produced by a regular subcircuit.
      return k;
    }
    cm = & icm->second;
    cr = cm->other ();
  }

  //  NOTE: if cm is given, cr is given in terms of the "other" circuit

  for (db::Circuit::const_pin_iterator p = cr->begin_pins (); p != cr->end_pins (); ++p) {

    size_t this_pin_id = cm ? cm->this_pin_from_other_pin (p->id ()) : p->id ();
    size_t pin_id = pin_map ? pin_map->normalize_pin_id (cr, p->id ()) : p->id ();

    const db::Net *net = subcircuit.net_for_pin (this_pin_id);
    size_t net_id = g.node_index_for_net (net);
    k.push_back (std::make_pair (pin_id, net_id));

  }

  std::sort (k.begin (), k.end ());

  return k;
}

bool
NetlistComparer::compare_circuits (const db::Circuit *c1, const db::Circuit *c2, db::DeviceCategorizer &device_categorizer, db::CircuitCategorizer &circuit_categorizer, const std::vector<std::pair<const Net *, const Net *> > &net_identity, bool &pin_mismatch, std::map<const db::Circuit *, CircuitMapper> &circuit_and_pin_mapping) const
{
  db::NetDeviceGraph g1, g2;

  //  NOTE: for normalization we map all subcircuits of c1 to c2.
  //  Also, pin swapping will only happen there.
  g1.build (c1, device_categorizer, circuit_categorizer, &circuit_and_pin_mapping, mp_circuit_pin_mapper.get ());
  g2.build (c2, device_categorizer, circuit_categorizer, 0, mp_circuit_pin_mapper.get ());

  for (std::vector<std::pair<const Net *, const Net *> >::const_iterator p = net_identity.begin (); p != net_identity.end (); ++p) {
    size_t ni1 = g1.node_index_for_net (p->first);
    size_t ni2 = g2.node_index_for_net (p->second);
    g1.identify (ni1, ni2);
    g2.identify (ni2, ni1);
  }

  bool good = true;
  while (good) {

#if defined(PRINT_DEBUG_NETCOMPARE)
    tl::log << "new iteration ...";
#endif

    size_t new_identities = 0;
    for (db::NetDeviceGraph::node_iterator i1 = g1.begin (); i1 != g1.end (); ++i1) {
      if (i1->has_other ()) {
#if defined(PRINT_DEBUG_NETCOMPARE)
        tl::log << "deriving new identities from " << i1->net ()->expanded_name ();
#endif
        size_t ni = g1.derive_node_identities (i1 - g1.begin (), g2, mp_logger);
        new_identities += ni;
        if (ni > 0) {
#if defined(PRINT_DEBUG_NETCOMPARE)
        tl::log << ni << " new identities.";
#endif
        }
      }
    }

    bool any_without = false;
    for (db::NetDeviceGraph::node_iterator i1 = g1.begin (); i1 != g1.end () && ! any_without; ++i1) {
      any_without = ! i1->has_other ();
    }

    if (! any_without) {
      break;
    }

    if (new_identities == 0) {

#if defined(PRINT_DEBUG_NETCOMPARE)
      tl::log << "checking topological identity ...";
#endif
      //  derive new identities through topology

      db::NetDeviceGraph::node_iterator i1 = g1.begin (), i2 = g2.begin ();
      for ( ; i1 != g1.end () && i2 != g2.end (); ) {

        if (i1->has_other ()) {
          ++i1;
        } else if (i2->has_other ()) {
          ++i2;
        } else if (*i1 < *i2) {
          ++i1;
        } else if (*i2 < *i1) {
          ++i2;
        } else {

          db::NetDeviceGraph::node_iterator ii1 = i1, ii2 = i2;

          ++i1;
          ++i2;

          bool ambiguous = (i1 != g1.end () && *i1 == *ii1) || (i2 != g2.end () && *i2 == *ii2);

          //  found a candidate - a single node with the same edges
          db::NetDeviceGraph::confirm_identity (g1, ii1, g2, ii2, mp_logger, ambiguous);
          ++new_identities;

        }

      }

    }

    if (new_identities == 0) {
      good = false;
    }

  }


  //  Report missing net assignment

  for (db::NetDeviceGraph::node_iterator i = g1.begin (); i != g1.end (); ++i) {
    if (! i->has_other ()) {
      mp_logger->net_mismatch (i->net (), 0);
    }
  }

  for (db::NetDeviceGraph::node_iterator i = g2.begin (); i != g2.end (); ++i) {
    if (! i->has_other ()) {
      mp_logger->net_mismatch (0, i->net ());
    }
  }


  //  Report pin assignment

  std::multimap<const db::Net *, const db::Pin *> net2pin;
  for (db::Circuit::const_pin_iterator p = c2->begin_pins (); p != c2->end_pins (); ++p) {
    const db::Net *net = c2->net_for_pin (p->id ());
    tl_assert (net != 0);
    net2pin.insert (std::make_pair (net, p.operator-> ()));
  }

  CircuitMapper &pin_mapping = circuit_and_pin_mapping [c1];
  pin_mapping.set_other (c2);

  for (db::NetDeviceGraph::node_iterator i = g1.begin (); i != g1.end (); ++i) {

    const db::Net *net = i->net ();
    tl_assert (net != 0);

    if (net->pin_count () == 0) {
      continue;
    }

    if (! i->has_other ()) {
      for (db::Net::const_pin_iterator pi = net->begin_pins (); pi != net->end_pins (); ++pi) {
        mp_logger->pin_mismatch (pi->pin (), 0);
        pin_mismatch = true;
        good = false;
      }
      continue;
    }

    const db::Net *other_net = g2.net_by_node_index (i->other_net_index ());

    std::multimap<const db::Net *, const db::Pin *>::iterator np = net2pin.find (other_net);
    for (db::Net::const_pin_iterator pi = net->begin_pins (); pi != net->end_pins (); ++pi) {

      if (np != net2pin.end () && np->first == other_net) {

        mp_logger->match_pins (pi->pin (), np->second);
        pin_mapping.map_pin (pi->pin ()->id (), np->second->id ());

        std::multimap<const db::Net *, const db::Pin *>::iterator np_delete = np;
        ++np;
        net2pin.erase (np_delete);

      } else {
        mp_logger->pin_mismatch (pi->pin (), 0);
        pin_mismatch = true;
        good = false;
      }

    }

  }

  for (std::multimap<const db::Net *, const db::Pin *>::iterator np = net2pin.begin (); np != net2pin.end (); ++np) {
    mp_logger->pin_mismatch (0, np->second);
    pin_mismatch = true;
    good = false;
  }


  //  Report device assignment

  std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::Device *, size_t> > device_map;

  for (db::Circuit::const_device_iterator d = c1->begin_devices (); d != c1->end_devices (); ++d) {

    std::vector<std::pair<size_t, size_t> > k = compute_device_key (*d, g1);

    bool mapped = true;
    for (std::vector<std::pair<size_t, size_t> >::iterator i = k.begin (); i != k.end () && mapped; ++i) {
      if (! g1.begin () [i->second].has_other ()) {
        mapped = false;
      }
    }

    if (! mapped) {
      mp_logger->device_mismatch (d.operator-> (), 0);
      good = false;
    } else {
      //  TODO: report devices which cannot be distiguished topologically?
      device_map.insert (std::make_pair (k, std::make_pair (d.operator-> (), device_categorizer.cat_for_device (d.operator-> ()))));
    }

  }

  for (db::Circuit::const_device_iterator d = c2->begin_devices (); d != c2->end_devices (); ++d) {

    std::vector<std::pair<size_t, size_t> > k = compute_device_key (*d, g2);

    bool mapped = true;
    for (std::vector<std::pair<size_t, size_t> >::iterator i = k.begin (); i != k.end (); ++i) {
      if (! g2.begin () [i->second].has_other ()) {
        mapped = false;
      } else {
        i->second = g2.begin () [i->second].other_net_index ();
      }
    }

    std::sort (k.begin (), k.end ());

    std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::Device *, size_t> >::const_iterator dm = device_map.find (k);

    if (! mapped || dm == device_map.end () || dm->first != k) {

      mp_logger->device_mismatch (0, d.operator-> ());
      good = false;

    } else {

      db::DeviceCompare dc;

      size_t device_cat = device_categorizer.cat_for_device (d.operator-> ());

      if (! dc.equals (dm->second, std::make_pair (d.operator-> (), device_cat))) {
        if (dm->second.second != device_cat) {
          mp_logger->match_devices_with_different_device_classes (dm->second.first, d.operator-> ());
          good = false;
        } else {
          mp_logger->match_devices_with_different_parameters (dm->second.first, d.operator-> ());
          good = false;
        }
      } else {
        mp_logger->match_devices (dm->second.first, d.operator-> ());
      }

      device_map.erase (dm);

    }

  }

  for (std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::Device *, size_t> >::const_iterator dm = device_map.begin (); dm != device_map.end (); ++dm) {
    mp_logger->device_mismatch (dm->second.first, 0);
    good = false;
  }


  //  Report subcircuit assignment

  std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::SubCircuit *, size_t> > subcircuit_map;

  for (db::Circuit::const_subcircuit_iterator sc = c1->begin_subcircuits (); sc != c1->end_subcircuits (); ++sc) {

    std::vector<std::pair<size_t, size_t> > k = compute_subcircuit_key (*sc, g1, &circuit_and_pin_mapping, mp_circuit_pin_mapper.get ());

    bool mapped = true;
    for (std::vector<std::pair<size_t, size_t> >::iterator i = k.begin (); i != k.end () && mapped; ++i) {
      if (! g1.begin () [i->second].has_other ()) {
        mapped = false;
      }
    }

    if (! mapped) {
      mp_logger->subcircuit_mismatch (sc.operator-> (), 0);
      good = false;
    } else if (! k.empty ()) {
      //  TODO: report devices which cannot be distiguished topologically?
      subcircuit_map.insert (std::make_pair (k, std::make_pair (sc.operator-> (), circuit_categorizer.cat_for_subcircuit (sc.operator-> ()))));
    }

  }

  for (db::Circuit::const_subcircuit_iterator sc = c2->begin_subcircuits (); sc != c2->end_subcircuits (); ++sc) {

    std::vector<std::pair<size_t, size_t> > k = compute_subcircuit_key (*sc, g2, 0, mp_circuit_pin_mapper.get ());

    bool mapped = true;
    for (std::vector<std::pair<size_t, size_t> >::iterator i = k.begin (); i != k.end (); ++i) {
      if (! g1.begin () [i->second].has_other ()) {
        mapped = false;
      } else {
        i->second = g2.begin () [i->second].other_net_index ();
      }
    }

    std::sort (k.begin (), k.end ());

    std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::SubCircuit *, size_t> >::const_iterator scm = subcircuit_map.find (k);

    if (! mapped || scm == subcircuit_map.end ()) {

      mp_logger->subcircuit_mismatch (0, sc.operator-> ());
      good = false;

    } else {

      db::SubCircuitCompare scc;
      size_t sc_cat = circuit_categorizer.cat_for_subcircuit (sc.operator-> ());

      if (! scc.equals (scm->second, std::make_pair (sc.operator-> (), sc_cat))) {
        mp_logger->subcircuit_mismatch (scm->second.first, sc.operator-> ());
        good = false;
      } else {
        mp_logger->match_subcircuits (scm->second.first, sc.operator-> ());
      }

      subcircuit_map.erase (scm);

    }

  }

  for (std::multimap<std::vector<std::pair<size_t, size_t> >, std::pair<const db::SubCircuit *, size_t> >::const_iterator scm = subcircuit_map.begin (); scm != subcircuit_map.end (); ++scm) {
    mp_logger->subcircuit_mismatch (scm->second.first, 0);
    good = false;
  }

  return good;
}

}

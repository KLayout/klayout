

// @@@

#include "dbNetlist.h"
#include "dbNetlistDeviceClasses.h"
#include "dbHash.h"
#include "tlUnitTest.h"
#include "tlProgress.h"
#include "tlTimer.h"

namespace db
{

class NetlistCompareLogger
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
};

struct DeviceCompare
{
  bool operator() (const db::Device *d1, const db::Device *d2) const
  {
    //  @@@ TODO: device class identity should not be defined via name
    if (d1->device_class () != d2->device_class () && d1->device_class ()->name () != d2->device_class ()->name ()) {
      return d1->device_class ()->name () < d2->device_class ()->name ();
    }

    const std::vector<db::DeviceParameterDefinition> &dp = d1->device_class ()->parameter_definitions ();
    for (std::vector<db::DeviceParameterDefinition>::const_iterator i = dp.begin (); i != dp.end (); ++i) {
      double v1 = d1->parameter_value (i->id ());
      double v2 = d2->parameter_value (i->id ());
      if (fabs (v1 - v2) > db::epsilon) {
        return v1 < v2;
      }
    }
    return false;
  }
};

struct SubCircuitCompare
{
  bool operator() (const db::SubCircuit *sc1, const db::SubCircuit *sc2) const
  {
    //  @@@ TODO: device class identity should not be defined via name
    if (sc1->circuit_ref () != sc2->circuit_ref () && sc1->circuit_ref ()->name () != sc2->circuit_ref ()->name ()) {
      return sc1->circuit_ref ()->name () < sc2->circuit_ref ()->name ();
    }

    //  no parameters
    return false;
  }
};

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

static size_t translate_terminal_id (size_t tid, const db::Device *device)
{
  // @@@ delegate to device class
  if (dynamic_cast<const db::DeviceClassMOS3Transistor *> (device->device_class ())) {
    if (tid == db::DeviceClassMOS3Transistor::terminal_id_D) {
      return db::DeviceClassMOS3Transistor::terminal_id_S;
    }
  } else if (dynamic_cast<const db::DeviceClassMOS4Transistor *> (device->device_class ())) {
    if (tid == db::DeviceClassMOS4Transistor::terminal_id_D) {
      return db::DeviceClassMOS4Transistor::terminal_id_S;
    }
  }
  return tid;
  // @@@
}

static size_t translate_subcircuit_pin_id (size_t pid, const db::Circuit * /*circuit*/)
{
  // @@@ not implemented yet
  return pid;
  // @@@
}

class NetDeviceGraphNode
{
public:
  struct EdgeDesc {

    //  @@@ TODO: there can be only devices or subcircuits, so we can
    //  compress this structure.
    const db::Device *device;
    size_t terminal1_id, terminal2_id;
    const db::SubCircuit *subcircuit;
    size_t pin1_id, pin2_id;

    EdgeDesc ()
      : device (0), terminal1_id (0), terminal2_id (0),
        subcircuit (0), pin1_id (0), pin2_id (0)
    {
      //  .. nothing yet ..
    }

    bool operator< (const EdgeDesc &other) const
    {
      if ((device != 0) != (other.device != 0)) {
        return (device != 0) < (other.device != 0);
      }

      if (device != 0) {

        DeviceCompare dc;
        bool dlt = dc (device, other.device);
        if (dlt || dc (other.device, device)) {
          return dlt;
        }

        if (terminal1_id != other.terminal1_id) {
          return terminal1_id < other.terminal1_id;
        }
        return terminal2_id < other.terminal2_id;

      } else {

        SubCircuitCompare sc;
        bool sclt = sc (subcircuit, other.subcircuit);
        if (sclt || sc (other.subcircuit, subcircuit)) {
          return sclt;
        }

        if (pin1_id != other.pin1_id) {
          return pin1_id < other.pin1_id;
        }
        return pin2_id < other.pin2_id;

      }
    }

    bool operator== (const EdgeDesc &other) const
    {
      if ((device != 0) != (other.device != 0)) {
        return false;
      }

      if (device != 0) {

        DeviceCompare dc;
        if (dc (device, other.device) || dc (other.device, device)) {
          return false;
        }
        if (terminal1_id != other.terminal1_id) {
          return false;
        }
        return terminal2_id == other.terminal2_id;

      } else {

        SubCircuitCompare sc;
        if (sc (subcircuit, other.subcircuit) || sc (other.subcircuit, subcircuit)) {
          return false;
        }

        if (pin1_id != other.pin1_id) {
          return false;
        }
        return pin2_id == other.pin2_id;

      }
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

  NetDeviceGraphNode (const db::Net *net, std::map<const db::Device *, size_t, DeviceCompare> &devmap, std::vector<const db::Device *> &device_prototypes, const std::map<const db::Circuit *, CircuitMapper> *circuit_map)
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
        tl_assert (icm != circuit_map->end ());
        cm = & icm->second;
      }

      //  NOTE: if cm is given, cr and pin_id are given in terms of the "other" circuit

      if (cm) {
        cr = cm->other ();
        pin_id = cm->other_pin_from_this_pin (pin_id);
      }

      //  we cannot afford creating edges from all to all other pins, so we just create edges to the previous and next
      //  pin. This may take more iterations to solve, but should be equivalent.

      db::Circuit::const_pin_iterator p = cr->begin_pins ();
      for ( ; p != cr->end_pins () && p->id () != pin_id; ++p)
        ;
      tl_assert (p != cr->end_pins ());

      db::Circuit::const_pin_iterator pp = p;
      if (pp == cr->begin_pins ()) {
        pp = cr->end_pins ();
      }
      --pp;

      db::Circuit::const_pin_iterator pn = p;
      ++pn;
      if (pn == cr->end_pins ()) {
        pn = cr->begin_pins ();
      }

      for (int i = 0; i < 2; ++i) {

        size_t pin2_id = (i == 0 ? pp->id () : pn->id ());

        EdgeDesc ed;
        ed.subcircuit = sc;
        //  NOTE: if a pin mapping is given, EdgeDesc::pin1_id and EdgeDesc::pin2_id are given
        //  as pin ID's of the other circuit.
        ed.pin1_id = translate_subcircuit_pin_id (pin_id, cr);
        ed.pin2_id = translate_subcircuit_pin_id (pin2_id, cr);

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

      size_t dev_id = 0;
      std::map<const db::Device *, size_t, DeviceCompare>::iterator di = devmap.find (d);
      if (di != devmap.end ()) {
        dev_id = di->second;
      } else {
        dev_id = device_prototypes.size ();
        device_prototypes.push_back (d);
      }

      EdgeDesc ed;
      ed.device = device_prototypes [dev_id];
      ed.terminal1_id = translate_terminal_id (i->terminal_id (), d);

      const std::vector<db::DeviceTerminalDefinition> &td = d->device_class ()->terminal_definitions ();
      for (std::vector<db::DeviceTerminalDefinition>::const_iterator it = td.begin (); it != td.end (); ++it) {

        if (it->id () != i->terminal_id ()) {

          EdgeDesc ed2 = ed;
          ed2.terminal2_id = translate_terminal_id (it->id (), d);
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
    if (res->first != edge) {
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

  void build (const db::Circuit *c, const std::map<const db::Circuit *, CircuitMapper> *circuit_and_pin_mapping)
  {
    tl::SelfTimer timer (tl::verbosity () >= 31, tl::to_string (tr ("Building net graph for circuit: ")) + c->name ());

    m_device_map.clear ();
    m_device_prototypes.clear ();
    m_nodes.clear ();
    m_net_index.clear ();

    size_t nets = 0;
    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
      ++nets;
    }
    m_nodes.reserve (nets);

    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {
      NetDeviceGraphNode node (n.operator-> (), m_device_map, m_device_prototypes, circuit_and_pin_mapping);
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
                if (! m_nodes[i->second.first].has_other ()) {
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
  std::map<const db::Device *, size_t, DeviceCompare> m_device_map;
  std::vector<const db::Device *> m_device_prototypes;
  std::map<const db::Net *, size_t> m_net_index;
};

class NetlistComparer
{
public:
  NetlistComparer (NetlistCompareLogger *logger)
    : mp_logger (logger)
  { }

  void same_nets (const db::Circuit *a, const db::Net *na, const db::Circuit *b, const db::Net *nb)
  {
    m_same_nets [std::make_pair (a, b)].push_back (std::make_pair (na, nb));
  }

  bool compare (const db::Netlist *a, const db::Netlist *b) const
  {
    bool good = true;

    std::map<std::string, std::pair<const db::Circuit *, const db::Circuit *> > name2circuits;

    for (db::Netlist::const_circuit_iterator i = a->begin_circuits (); i != a->end_circuits (); ++i) {
      name2circuits[i->name ()].first = i.operator-> ();
    }

    for (db::Netlist::const_circuit_iterator i = b->begin_circuits (); i != b->end_circuits (); ++i) {
      name2circuits[i->name ()].second = i.operator-> ();
    }

    if (mp_logger) {
      mp_logger->begin_netlist (a, b);
    }

    for (std::map<std::string, std::pair<const db::Circuit *, const db::Circuit *> >::const_iterator i = name2circuits.begin (); i != name2circuits.end (); ++i) {
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

      std::map<std::string, std::pair<const db::Circuit *, const db::Circuit *> >::const_iterator i = name2circuits.find ((*c)->name ());
      tl_assert (i != name2circuits.end ());

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
          bool g = compare_circuits (ca, cb, *net_identity, pin_mismatch, pin_mapping);
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

protected:
  bool compare_circuits (const db::Circuit *c1, const db::Circuit *c2, const std::vector<std::pair<const Net *, const Net *> > &net_identity, bool &pin_mismatch, std::map<const db::Circuit *, CircuitMapper> &circuit_and_pin_map) const;
  bool all_subcircuits_verified (const db::Circuit *c, const std::set<const db::Circuit *> &verified_circuits) const;

  NetlistCompareLogger *mp_logger;
  std::map<std::pair<const db::Circuit *, const db::Circuit *>, std::vector<std::pair<const Net *, const Net *> > > m_same_nets;
};

bool
NetlistComparer::all_subcircuits_verified (const db::Circuit *c, const std::set<const db::Circuit *> &verified_circuits) const
{
  for (db::Circuit::const_subcircuit_iterator sc = c->begin_subcircuits (); sc != c->end_subcircuits (); ++sc) {
    if (verified_circuits.find (sc->circuit_ref ()) == verified_circuits.end ()) {
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
compute_subcircuit_key (const db::SubCircuit &subcircuit, const db::NetDeviceGraph &g, const std::map<const db::Circuit *, CircuitMapper> *circuit_map)
{
  std::vector<std::pair<size_t, size_t> > k;

  const db::Circuit *cr = subcircuit.circuit_ref ();

  const CircuitMapper *cm = 0;

  if (circuit_map) {
    std::map<const db::Circuit *, CircuitMapper>::const_iterator icm = circuit_map->find (cr);
    tl_assert (icm != circuit_map->end ());
    cm = & icm->second;
    cr = cm->other ();
  }

  //  NOTE: if cm is given, cr is given in terms of the "other" circuit

  for (db::Circuit::const_pin_iterator p = cr->begin_pins (); p != cr->end_pins (); ++p) {

    size_t this_pin_id = cm ? cm->this_pin_from_other_pin (p->id ()) : p->id ();

    size_t pin_id = translate_subcircuit_pin_id (p->id (), cr);
    const db::Net *net = subcircuit.net_for_pin (this_pin_id);
    size_t net_id = g.node_index_for_net (net);
    k.push_back (std::make_pair (pin_id, net_id));

  }

  std::sort (k.begin (), k.end ());

  return k;
}

bool
NetlistComparer::compare_circuits (const db::Circuit *c1, const db::Circuit *c2, const std::vector<std::pair<const Net *, const Net *> > &net_identity, bool &pin_mismatch, std::map<const db::Circuit *, CircuitMapper> &circuit_and_pin_mapping) const
{
  db::NetDeviceGraph g1, g2;

  //  NOTE: for normalization we map all subcircuits of c1 to c2.
  //  Also, pin swapping will only happen there.
  g1.build (c1, &circuit_and_pin_mapping);
  g2.build (c2, 0);

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
      }

    }

  }

  for (std::multimap<const db::Net *, const db::Pin *>::iterator np = net2pin.begin (); np != net2pin.end (); ++np) {
    mp_logger->pin_mismatch (0, np->second);
    pin_mismatch = true;
  }


  //  Report device assignment

  std::map<std::vector<std::pair<size_t, size_t> >, const db::Device *> device_map;

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
    } else {
      //  TODO: report devices which cannot be distiguished topologically?
      device_map[k] = d.operator-> ();
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

    std::map<std::vector<std::pair<size_t, size_t> >, const db::Device *>::const_iterator dm = device_map.find (k);

    if (! mapped || dm == device_map.end ()) {

      mp_logger->device_mismatch (0, d.operator-> ());

    } else {

      db::DeviceCompare dc;

      if (dc (dm->second, d.operator-> ()) || dc (d.operator-> (), dm->second)) {
        if (dm->second->device_class ()->name () != d->device_class ()->name ()) {
          mp_logger->match_devices_with_different_device_classes (dm->second, d.operator-> ());
          good = false;
        } else {
          mp_logger->match_devices_with_different_parameters (dm->second, d.operator-> ());
          good = false;
        }
      } else {
        mp_logger->match_devices (dm->second, d.operator-> ());
      }

      device_map.erase (dm);

    }

  }

  for (std::map<std::vector<std::pair<size_t, size_t> >, const db::Device *>::const_iterator dm = device_map.begin (); dm != device_map.end (); ++dm) {
    mp_logger->device_mismatch (dm->second, 0);
  }


  //  Report subcircuit assignment

  std::map<std::vector<std::pair<size_t, size_t> >, const db::SubCircuit *> subcircuit_map;

  for (db::Circuit::const_subcircuit_iterator sc = c1->begin_subcircuits (); sc != c1->end_subcircuits (); ++sc) {

    std::vector<std::pair<size_t, size_t> > k = compute_subcircuit_key (*sc, g1, &circuit_and_pin_mapping);

    bool mapped = true;
    for (std::vector<std::pair<size_t, size_t> >::iterator i = k.begin (); i != k.end () && mapped; ++i) {
      if (! g1.begin () [i->second].has_other ()) {
        mapped = false;
      }
    }

    if (! mapped) {
      mp_logger->subcircuit_mismatch (sc.operator-> (), 0);
    } else {
      //  TODO: report devices which cannot be distiguished topologically?
      subcircuit_map[k] = sc.operator-> ();
    }

  }

  for (db::Circuit::const_subcircuit_iterator sc = c2->begin_subcircuits (); sc != c2->end_subcircuits (); ++sc) {

    std::vector<std::pair<size_t, size_t> > k = compute_subcircuit_key (*sc, g2, 0);

    bool mapped = true;
    for (std::vector<std::pair<size_t, size_t> >::iterator i = k.begin (); i != k.end (); ++i) {
      if (! g1.begin () [i->second].has_other ()) {
        mapped = false;
      } else {
        i->second = g2.begin () [i->second].other_net_index ();
      }
    }

    std::sort (k.begin (), k.end ());

    std::map<std::vector<std::pair<size_t, size_t> >, const db::SubCircuit *>::const_iterator scm = subcircuit_map.find (k);

    if (! mapped || scm == subcircuit_map.end ()) {

      mp_logger->subcircuit_mismatch (0, sc.operator-> ());

    } else {

      db::SubCircuitCompare scc;

      if (scc (scm->second, sc.operator-> ()) || scc (sc.operator-> (), scm->second)) {
        mp_logger->subcircuit_mismatch (scm->second, sc.operator-> ());
      } else {
        mp_logger->match_subcircuits (scm->second, sc.operator-> ());
      }

    }

  }

  return good;
}

}

class NetlistCompareTestLogger
  : public db::NetlistCompareLogger
{
public:
  NetlistCompareTestLogger () { }

  void out (const std::string &text)
  {
    m_texts.push_back (text);
#if defined(PRINT_DEBUG_NETCOMPARE)
    tl::log << m_texts.back ();
#endif
  }

  virtual void begin_circuit (const db::Circuit *a, const db::Circuit *b)
  {
    out ("begin_circuit " + circuit2str (a) + " " + circuit2str (b));
  }

  virtual void end_circuit (const db::Circuit *a, const db::Circuit *b, bool matching)
  {
    out ("end_circuit " + circuit2str (a) + " " + circuit2str (b) + " " + (matching ? "MATCH" : "NOMATCH"));
  }

  virtual void circuit_skipped (const db::Circuit *a, const db::Circuit *b)
  {
    out ("circuit_skipped " + circuit2str (a) + " " + circuit2str (b));
  }

  virtual void circuit_mismatch (const db::Circuit *a, const db::Circuit *b)
  {
    out ("circuit_mismatch " + circuit2str (a) + " " + circuit2str (b));
  }

  virtual void match_nets (const db::Net *a, const db::Net *b)
  {
    out ("match_nets " + net2str (a) + " " + net2str (b));
  }

  virtual void match_ambiguous_nets (const db::Net *a, const db::Net *b)
  {
    out ("match_ambiguous_nets " + net2str (a) + " " + net2str (b));
  }

  virtual void net_mismatch (const db::Net *a, const db::Net *b)
  {
    out ("net_mismatch " + net2str (a) + " " + net2str (b));
  }

  virtual void match_devices (const db::Device *a, const db::Device *b)
  {
    out ("match_devices " + device2str (a) + " " + device2str (b));
  }

  virtual void device_mismatch (const db::Device *a, const db::Device *b)
  {
    out ("device_mismatch " + device2str (a) + " " + device2str (b));
  }

  virtual void match_devices_with_different_parameters (const db::Device *a, const db::Device *b)
  {
    out ("match_devices_with_different_parameters " + device2str (a) + " " + device2str (b));
  }

  virtual void match_devices_with_different_device_classes (const db::Device *a, const db::Device *b)
  {
    out ("match_devices_with_different_device_classes " + device2str (a) + " " + device2str (b));
  }

  virtual void match_pins (const db::Pin *a, const db::Pin *b)
  {
    out ("match_pins " + pin2str (a) + " " + pin2str (b));
  }

  virtual void pin_mismatch (const db::Pin *a, const db::Pin *b)
  {
    out ("pin_mismatch " + pin2str (a) + " " + pin2str (b));
  }

  virtual void match_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    out ("match_subcircuits " + subcircuit2str (a) + " " + subcircuit2str (b));
  }

  virtual void subcircuit_mismatch (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    out ("subcircuit_mismatch " + subcircuit2str (a) + " " + subcircuit2str (b));
  }

  std::string text () const
  {
    return tl::join (m_texts, "\n");
  }

private:
  std::vector<std::string> m_texts;

  std::string circuit2str (const db::Circuit *x) const
  {
    return x ? x->name () : "(null)";
  }

  std::string device2str (const db::Device *x) const
  {
    return x ? x->expanded_name () : "(null)";
  }

  std::string net2str (const db::Net *x) const
  {
    return x ? x->expanded_name () : "(null)";
  }

  std::string pin2str (const db::Pin *x) const
  {
    return x ? x->expanded_name () : "(null)";
  }

  std::string subcircuit2str (const db::SubCircuit *x) const
  {
    return x ? x->expanded_name () : "(null)";
  }
};

static void prep_nl (db::Netlist &nl, const char *str)
{
  db::DeviceClass *dc;

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("PMOS");
  nl.add_device_class (dc);

  dc = new db::DeviceClassMOS3Transistor ();
  dc->set_name ("NMOS");
  nl.add_device_class (dc);

  nl.from_string (str);
}

TEST(1_SimpleInverter)
{
  const char *nls1 =
    "circuit INV ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit INV INV\n"
     "match_nets VSS VSS\n"
     "match_nets VDD VDD\n"
     "match_nets OUT OUT\n"
     "match_nets IN IN\n"
     "match_pins $3 $2\n"
     "match_pins $2 $0\n"
     "match_pins $1 $3\n"
     "match_pins $0 $1\n"
     "match_devices $2 $1\n"
     "match_devices $1 $2\n"
     "end_circuit INV INV MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(2_SimpleInverterWithForcedNetAssignment)
{
  const char *nls1 =
    "circuit INV ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);
  const db::Circuit *ca = nl1.circuit_by_name ("INV");
  const db::Circuit *cb = nl2.circuit_by_name ("INV");
  comp.same_nets (ca, ca->net_by_name ("VDD"), cb, cb->net_by_name ("VDD"));
  comp.same_nets (ca, ca->net_by_name ("VSS"), cb, cb->net_by_name ("VSS"));

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit INV INV\n"
     "match_nets OUT OUT\n"
     "match_nets IN IN\n"
     "match_pins $3 $2\n"
     "match_pins $2 $0\n"
     "match_pins $1 $3\n"
     "match_pins $0 $1\n"
     "match_devices $2 $1\n"
     "match_devices $1 $2\n"
     "end_circuit INV INV MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(3_Buffer)
{
  const char *nls1 =
    "circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $3 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit BUF BUF\n"
     "match_nets VSS VSS\n"
     "match_nets OUT OUT\n"
     "match_nets VDD VDD\n"
     "match_nets IN IN\n"
     "match_nets INT $10\n"
     "match_pins $3 $2\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $0 $1\n"
     "match_devices $1 $1\n"
     "match_devices $3 $2\n"
     "match_devices $2 $3\n"
     "match_devices $4 $4\n"
     "end_circuit BUF BUF MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(4_BufferTwoPaths)
{
  const char *nls1 =
    "circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
     "begin_circuit BUF BUF\n"
     "match_nets VSS VSS\n"
     "match_nets OUT OUT\n"
     "match_ambiguous_nets INT $10\n"
     "match_nets INT2 $11\n"
     "match_nets VDD VDD\n"
     "match_nets IN IN\n"
     "match_pins $3 $2\n"
     "match_pins $1 $3\n"
     "match_pins $2 $0\n"
     "match_pins $0 $1\n"
     "match_devices $1 $1\n"
     "match_devices $3 $2\n"
     "match_devices $5 $3\n"
     "match_devices $7 $4\n"
     "match_devices $2 $5\n"
     "match_devices $4 $6\n"
     "match_devices $6 $7\n"
     "match_devices $8 $8\n"
     "end_circuit BUF BUF MATCH"
  );
  EXPECT_EQ (good, true);
}

TEST(5_BufferTwoPathsDifferentParameters)
{
  const char *nls1 =
    "circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  const char *nls2 =
    "circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.35,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"   //  NOTE: 0.35 instead of 0.25
    "  device NMOS $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  //  Forcing the power nets into equality makes the parameter error harder to detect
  const db::Circuit *ca = nl1.circuit_by_name ("BUF");
  const db::Circuit *cb = nl2.circuit_by_name ("BUF");
  comp.same_nets (ca, ca->net_by_name ("VDD"), cb, cb->net_by_name ("VDD"));
  comp.same_nets (ca, ca->net_by_name ("VSS"), cb, cb->net_by_name ("VSS"));

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit BUF BUF\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_ambiguous_nets INT $10\n"
    "match_nets INT2 $11\n"
    "match_pins $3 $2\n"
    "match_pins $1 $3\n"
    "match_pins $2 $0\n"
    "match_pins $0 $1\n"
    "match_devices $1 $1\n"
    "match_devices $3 $2\n"
    "match_devices $5 $3\n"
    "match_devices $7 $4\n"
    "match_devices $2 $5\n"
    "match_devices $4 $6\n"
    "match_devices_with_different_parameters $6 $7\n"
    "match_devices $8 $8\n"
    "end_circuit BUF BUF NOMATCH"
  );
  EXPECT_EQ (good, false);
}

TEST(10_SimpleSubCircuits)
{
  const char *nls1 =
    "circuit INV ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($1=IN,$2=OUT,$3=VDD,$4=VSS);\n"
    "  subcircuit INV $1 ($1=IN,$2=INT,$3=VDD,$4=VSS);\n"
    "  subcircuit INV $2 ($1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($1=VDD,$2=IN,$3=VSS,$4=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($1=OUT,$2=VDD,$3=IN,$4=VSS);\n"
    "  subcircuit INV $1 ($1=VDD,$2=INT,$3=VSS,$4=OUT);\n"
    "  subcircuit INV $2 ($1=VDD,$2=IN,$3=VSS,$4=INT);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit INV INV\n"
    "match_nets VSS VSS\n"
    "match_nets VDD VDD\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "match_pins $3 $2\n"
    "match_pins $2 $0\n"
    "match_pins $1 $3\n"
    "match_pins $0 $1\n"
    "match_devices $2 $1\n"
    "match_devices $1 $2\n"
    "end_circuit INV INV MATCH\n"
    "begin_circuit TOP TOP\n"
    "match_nets IN IN\n"
    "match_nets OUT OUT\n"
    "match_nets VDD VDD\n"
    "match_nets VSS VSS\n"
    "match_nets INT INT\n"
    "match_pins $0 $2\n"
    "match_pins $1 $0\n"
    "match_pins $2 $1\n"
    "match_pins $3 $3\n"
    "match_subcircuits $2 $1\n"
    "match_subcircuits $1 $2\n"
    "end_circuit TOP TOP MATCH"
  );

  EXPECT_EQ (good, true);
}

TEST(11_MismatchingSubcircuits)
{
  const char *nls1 =
    "circuit INV ($0=IN,$1=OUT,$2=VDD,$3=VSS);\n"
    "  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=IN,$1=OUT,$2=VDD,$3=VSS);\n"
    "  subcircuit INV $1 ($1=IN,$2=INT,$3=VDD,$4=VSS);\n"
    "  subcircuit INV $2 ($1=INT,$2=OUT,$3=VDD,$4=VSS);\n"
    "end;\n";

  const char *nls2 =
    "circuit INV ($0=VDD,$1=IN,$2=VSS,$3=OUT);\n"
    "  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    //  wrong wiring:
    "  device PMOS $2 (S=IN,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "end;\n"
    "circuit TOP ($0=OUT,$1=VDD,$2=IN,$3=VSS);\n"
    "  subcircuit INV $1 ($1=VDD,$2=INT,$3=VSS,$4=OUT);\n"
    "  subcircuit INV $2 ($1=VDD,$2=IN,$3=VSS,$4=INT);\n"
    "end;\n";

  db::Netlist nl1, nl2;
  prep_nl (nl1, nls1);
  prep_nl (nl2, nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (),
    "begin_circuit INV INV\n"
    "match_nets VSS VSS\n"
    "match_nets OUT OUT\n"
    "match_nets IN IN\n"
    "net_mismatch VDD (null)\n"
    "net_mismatch (null) VDD\n"
    "match_pins $3 $2\n"
    "pin_mismatch $2 (null)\n"
    "match_pins $1 $3\n"
    "match_pins $0 $1\n"
    "pin_mismatch (null) $0\n"
    "device_mismatch $1 (null)\n"
    "match_devices $2 $1\n"
    "device_mismatch (null) $2\n"
    "end_circuit INV INV NOMATCH\n"
    "circuit_skipped TOP TOP"
  );

  EXPECT_EQ (good, false);
}

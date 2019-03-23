

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
      return d1->device_class ()->name () < d1->device_class ()->name ();
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

class NetDeviceGraphNode
{
public:
  struct EdgeDesc {
    const db::Device *device;
    size_t terminal1_id, terminal2_id;

    bool operator< (const EdgeDesc &other) const
    {
      DeviceCompare dc;
      bool dlt = dc (device, other.device);
      if (dlt || dc (other.device, device)) {
        return dlt;
      }
      if (terminal1_id != other.terminal1_id) {
        return terminal1_id < other.terminal1_id;
      }
      return terminal2_id < other.terminal2_id;
    }

    bool operator== (const EdgeDesc &other) const
    {
      DeviceCompare dc;
      if (dc (device, other.device) || dc (other.device, device)) {
        return false;
      }
      if (terminal1_id != other.terminal1_id) {
        return false;
      }
      return terminal2_id == other.terminal2_id;
    }
  };

  typedef std::vector<std::pair<std::vector<EdgeDesc>, std::pair<const db::Net *, size_t> > >::const_iterator edge_iterator;

  NetDeviceGraphNode (const db::Net *net, std::map<const db::Device *, size_t, DeviceCompare> &devmap, std::vector<const db::Device *> &device_prototypes)
    : mp_net (net), m_other_net_index (std::numeric_limits<size_t>::max ())
  {
    std::map<const db::Net *, size_t> n2entry;

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
            m_edges.push_back (std::make_pair (std::vector<EdgeDesc> (), std::make_pair (net2, size_t (0))));
          }

          m_edges [in->second].first.push_back (ed2);

        }

      }

    }

    //  "deep sorting" of the edge descriptor
    for (std::vector<std::pair<std::vector<EdgeDesc>, std::pair<const db::Net *, size_t> > >::iterator i = m_edges.begin (); i != m_edges.end (); ++i) {
      std::sort (i->first.begin (), i->first.end ());
    }

    std::sort (m_edges.begin (), m_edges.end ());
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
    for (std::vector<std::pair<std::vector<EdgeDesc>, std::pair<const db::Net *, size_t> > >::iterator i = m_edges.begin (); i != m_edges.end (); ++i) {
      std::map<const db::Net *, size_t>::const_iterator j = ni.find (i->second.first);
      tl_assert (j != ni.end ());
      i->second.second = j->second;
    }
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

private:
  const db::Net *mp_net;
  size_t m_other_net_index;
  std::vector<std::pair<std::vector<EdgeDesc>, std::pair<const db::Net *, size_t> > > m_edges;
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

  void build (const db::Circuit *c)
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
      NetDeviceGraphNode node (n.operator-> (), m_device_map, m_device_prototypes);
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

  size_t index_for_net (const db::Net *net) const
  {
    std::map<const db::Net *, size_t>::const_iterator j = m_net_index.find (net);
    tl_assert (j != m_net_index.end ());
    return j->second;
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
            if (! m_nodes[i->second.second].has_other ()) {
              ec = i;
              ++count;
            }
          }

          if (count == 1) {   //  if non-ambiguous, non-assigned

            NetDeviceGraphNode::edge_iterator e_other = std::lower_bound (nother->begin (), nother->end (), *ec);
            if (e_other != nother->end () && e_other->first == ec->first) {

              NetDeviceGraphNode::edge_iterator ee_other = e_other;
              ++ee_other;

              while (ee_other != n->end () && ee_other->first == e_other->first) {
                ++ee_other;
              }

              size_t count_other = 0;
              NetDeviceGraphNode::edge_iterator ec_other;
              for (NetDeviceGraphNode::edge_iterator i = e_other; i != ee_other; ++i) {
                if (! m_nodes[i->second.second].has_other ()) {
                  ec_other = i;
                  ++count_other;
                }
              }

              if (count_other == 1) {
                confirm_identity (*this, begin () + ec->second.second, other, other.begin () + ec_other->second.second, logger);
                ++added;
                more.push_back (ec->second.second);
              }

            }

          }

          e = ee;

        }

      }

    }

    return added;
  }

  static void confirm_identity (db::NetDeviceGraph &g1, db::NetDeviceGraph::node_iterator s1, db::NetDeviceGraph &g2, db::NetDeviceGraph::node_iterator s2, db::NetlistCompareLogger *logger)
  {
    if (logger) {
      logger->match_nets (s1->net (), s2->net ());
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

    for (db::Netlist::const_bottom_up_circuit_iterator c = a->begin_bottom_up (); c != a->end_bottom_up (); ++c) {

      std::map<std::string, std::pair<const db::Circuit *, const db::Circuit *> >::const_iterator i = name2circuits.find ((*c)->name ());
      tl_assert (i != name2circuits.end ());

      if (i->second.first && i->second.second) {
        if (mp_logger) {
          mp_logger->begin_circuit (i->second.first, i->second.second);
        }
        bool g = compare_circuits (i->second.first, i->second.second);
        if (! g) {
          good = false;
        }
        if (mp_logger) {
          mp_logger->end_circuit (i->second.first, i->second.second, g);
        }
      }

    }

    if (mp_logger) {
      mp_logger->begin_netlist (a, b);
    }

    return good;
  }

protected:
  bool compare_circuits (const db::Circuit *c1, const db::Circuit *c2) const;

  NetlistCompareLogger *mp_logger;
};

bool
NetlistComparer::compare_circuits (const db::Circuit *c1, const db::Circuit *c2) const
{
  db::NetDeviceGraph g1, g2;

  g1.build (c1);
  g2.build (c2);

  while (true) {

    size_t new_identities = 0;
    for (db::NetDeviceGraph::node_iterator i1 = g1.begin (); i1 != g1.end (); ++i1) {
      if (i1->has_other ()) {
        new_identities += g1.derive_node_identities (i1 - g1.begin (), g2, mp_logger);
      }
    }

    bool any_without = false;
    for (db::NetDeviceGraph::node_iterator i1 = g1.begin (); i1 != g1.end () && ! any_without; ++i1) {
      any_without = ! i1->has_other ();
    }

    if (! any_without) {
      return true;
    }

    bool ambiguous = false;

    if (new_identities == 0) {

      //  derive new identities through topology

      db::NetDeviceGraph::node_iterator s1 = g1.end (), s2 = g2.end ();
      size_t seeds = 0;

      db::NetDeviceGraph::node_iterator i1 = g1.begin (), i2 = g2.begin ();
      for ( ; i1 != g1.end () && i2 != g2.end (); ) {

        if (i1->has_other ()) {
          ++i1;
        } else if (i2->has_other ()) {
          ++i2;
        } else if (*i1 < *i2) {
          seeds = 0;
          ++i1;
        } else if (*i2 < *i1) {
          seeds = 0;
          ++i2;
        } else {

          if (seeds == 0 || *s1 < *i1) {

            if (seeds == 1) {
              //  found a candidate - a single node with the same edges
              db::NetDeviceGraph::confirm_identity (g1, s1, g2, s2, mp_logger);
              ++new_identities;
            } else if (seeds > 1) {
              ambiguous = true;
            }

            s1 = i1; s2 = i2;
            seeds = 1;

          } else {
            ++seeds;
          }

          ++i1;
          ++i2;

        }

      }

      if (seeds == 1) {
        //  found a candidate - a single node with the same edges
        db::NetDeviceGraph::confirm_identity (g1, s1, g2, s2, mp_logger);
        ++new_identities;
      } else if (seeds > 1) {
        ambiguous = true;
      }

    }

    if (new_identities == 0) {
      // @@@
      if (ambiguous) {
        tl::error << tr ("No seed found - no non-ambiguous nets identified");
      } else {
        tl::error << tr ("No seed found - no equivalent nets identified");
      }
      tl::error << tr ("Unassigned in netlist A:");
      for (db::NetDeviceGraph::node_iterator i = g1.begin (); i != g1.end (); ++i) {
        if (! i->has_other ()) {
          tl::error << "  " << i->net ()->expanded_name ();
        }
      }
      tl::error << tr ("Unassigned in netlist B:");
      for (db::NetDeviceGraph::node_iterator i = g2.begin (); i != g2.end (); ++i) {
        if (! i->has_other ()) {
          tl::error << "  " << i->net ()->expanded_name ();
        }
      }
      // @@@
      return false;
    }

  }
}

}

class NetlistCompareTestLogger
  : public db::NetlistCompareLogger
{
public:
  NetlistCompareTestLogger () { }

  virtual void begin_circuit (const db::Circuit *a, const db::Circuit *b)
  {
    m_texts.push_back ("begin_circuit " + circuit2str (a) + " " + circuit2str (b));
  }

  virtual void end_circuit (const db::Circuit *a, const db::Circuit *b, bool matching)
  {
    m_texts.push_back ("end_circuit " + circuit2str (a) + " " + circuit2str (b) + " " + (matching ? "MATCH" : "NOMATCH"));
  }

  virtual void circuit_mismatch (const db::Circuit *a, const db::Circuit *b)
  {
    m_texts.push_back ("circuit_mismatch " + circuit2str (a) + " " + circuit2str (b));
  }

  virtual void match_nets (const db::Net *a, const db::Net *b)
  {
    m_texts.push_back ("match_nets " + net2str (a) + " " + net2str (b));
  }

  virtual void match_ambiguous_nets (const db::Net *a, const db::Net *b)
  {
    m_texts.push_back ("match_ambiguous_nets " + net2str (a) + " " + net2str (b));
  }

  virtual void net_mismatch (const db::Net *a, const db::Net *b)
  {
    m_texts.push_back ("net_mismatch " + net2str (a) + " " + net2str (b));
  }

  virtual void match_devices (const db::Device *a, const db::Device *b)
  {
    m_texts.push_back ("match_devices " + device2str (a) + " " + device2str (b));
  }

  virtual void match_devices_with_different_parameters (const db::Device *a, const db::Device *b)
  {
    m_texts.push_back ("match_devices_with_different_parameters " + device2str (a) + " " + device2str (b));
  }

  virtual void match_devices_with_different_device_classes (const db::Device *a, const db::Device *b)
  {
    m_texts.push_back ("match_devices_with_different_device_classes " + device2str (a) + " " + device2str (b));
  }

  virtual void match_pins (const db::Pin *a, const db::Pin *b)
  {
    m_texts.push_back ("match_pins " + pin2str (a) + " " + pin2str (b));
  }

  virtual void pin_mismatch (const db::Pin *a, const db::Pin *b)
  {
    m_texts.push_back ("pin_mismatch " + pin2str (a) + " " + pin2str (b));
  }

  virtual void match_subcircuits (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    m_texts.push_back ("match_subcircuits " + subcircuit2str (a) + " " + subcircuit2str (b));
  }

  virtual void subcircuit_mismatch (const db::SubCircuit *a, const db::SubCircuit *b)
  {
    m_texts.push_back ("subcircuit_mismatch " + subcircuit2str (a) + " " + subcircuit2str (b));
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


TEST(1)
{
  const char *nls2 =
    "circuit RINGO ();\n"
    "  device PMOS $1 (S=$16,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$16,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $3 (S=$14,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $4 (S=VDD,G=$14,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $5 (S=$12,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $6 (S=VDD,G=$12,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $7 (S='IN,FB',G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $8 (S=VDD,G='IN,FB',D='OUT,OSC') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $9 (S=$4,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $10 (S=VDD,G=$4,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $11 (S=$8,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $12 (S=VDD,G=$8,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $13 (S=$2,G='IN,FB',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $14 (S=VDD,G=$2,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $15 (S=$6,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $16 (S=VDD,G=$6,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $17 (S=$18,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $18 (S=VDD,G=$18,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $19 (S=$10,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $20 (S=VDD,G=$10,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $21 (S='IN,FB',G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $22 (S=VSS,G='IN,FB',D='OUT,OSC') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $23 (S=$18,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $24 (S=VSS,G=$18,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $25 (S=$14,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $26 (S=VSS,G=$14,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $27 (S=$12,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $28 (S=VSS,G=$12,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $29 (S=$4,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $30 (S=VSS,G=$4,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $31 (S=$2,G='IN,FB',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $32 (S=VSS,G=$2,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $33 (S=$8,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $34 (S=VSS,G=$8,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $35 (S=$6,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $36 (S=VSS,G=$6,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $37 (S=$16,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $38 (S=VSS,G=$16,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $39 (S=$10,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $40 (S=VSS,G=$10,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n";

  const char *nls1 =
    "circuit RINGO ();\n"
    "  device PMOS $1 (S=$16,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $2 (S=VDD,G=$16,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $3 (S=$14,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $4 (S=VDD,G=$14,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $5 (S=$12,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $6 (S=VDD,G=$12,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $7 (S='IN,FB',G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $8 (S=VDD,G='IN,FB',D='OUT,OSC') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $9 (S=$4,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $10 (S=VDD,G=$4,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $11 (S=$8,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $12 (S=VDD,G=$8,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $13 (S=$2,G='IN,FB',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $14 (S=VDD,G=$2,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $15 (S=$6,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $16 (S=VDD,G=$6,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $17 (S=$18,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $18 (S=VDD,G=$18,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device PMOS $19 (S=$10,G='IN,OUT',D=VDD) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device PMOS $20 (S=VDD,G=$10,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $21 (S='IN,FB',G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $22 (S=VSS,G='IN,FB',D='OUT,OSC') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $23 (S=$18,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $24 (S=VSS,G=$18,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $25 (S=$14,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $26 (S=VSS,G=$14,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $27 (S=$12,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $28 (S=VSS,G=$12,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $29 (S=$4,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $30 (S=VSS,G=$4,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $31 (S=$2,G='IN,FB',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $32 (S=VSS,G=$2,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $33 (S=$8,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $34 (S=VSS,G=$8,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $35 (S=$6,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $36 (S=VSS,G=$6,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $37 (S=$16,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $38 (S=VSS,G=$16,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "  device NMOS $39 (S=$10,G='IN,OUT',D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);\n"
    "  device NMOS $40 (S=VSS,G=$10,D='IN,OUT') (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);\n"
    "end;\n";

  db::DeviceClass *dc;

  db::Netlist nl1, nl2;

  db::Netlist *nlp[] = { &nl1, &nl2 };
  for (int i = 0; i < 2; ++i) {

    dc = new db::DeviceClassMOS3Transistor ();
    dc->set_name ("PMOS");
    nlp[i]->add_device_class (dc);

    dc = new db::DeviceClassMOS3Transistor ();
    dc->set_name ("NMOS");
    nlp[i]->add_device_class (dc);

  }

  nl1.from_string (nls1);
  nl2.from_string (nls2);

  NetlistCompareTestLogger logger;
  db::NetlistComparer comp (&logger);

  bool good = comp.compare (&nl1, &nl2);

  EXPECT_EQ (logger.text (), 
    ""
  );
  EXPECT_EQ (good, true);
}

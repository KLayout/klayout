
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

#include "dbNetlistCompareCore.h"
#include "dbNetlistCompareUtils.h"
#include "dbNetlistCompare.h"
#include "dbDevice.h"
#include "dbDeviceClass.h"
#include "dbNet.h"
#include "dbSubCircuit.h"
#include "dbCircuit.h"

#include "tlAssert.h"
#include "tlLog.h"
#include "tlInternational.h"

namespace db
{

// --------------------------------------------------------------------------------------------------------------------
//  Some utility classes for NetGraph implementation

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
    for (std::vector<Transition>::const_iterator j = e.first.begin (); j != e.first.end (); ++j) {
      if (! j->is_for_subcircuit ()) {
        dev.insert (std::make_pair (j->make_key (), j->device ()));
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
    for (std::vector<Transition>::const_iterator j = e.first.begin (); j != e.first.end (); ++j) {
      if (j->is_for_subcircuit ()) {
        sc.insert (std::make_pair (j->make_key (), j->subcircuit ()));
      }
    }
  }
};

// --------------------------------------------------------------------------------------------------------------------

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
                        size_t depth, bool exact_match = true)
  {
    g1->identify (n1, n2, exact_match);
    g2->identify (n2, n1, exact_match);

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
          if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
            tl::info << nl_compare_debug_indent (depth) << "enforcing device equivalence: " << dd->first->expanded_name () << " vs. " << dd->second->expanded_name ();
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
          if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
            tl::info << nl_compare_debug_indent(depth) << "enforcing subcircuit equivalence: " << cc->first->expanded_name () << " vs. " << cc->second->expanded_name ();
          }
        }
      }
    }
  }

  void clear ()
  {
    m_to_undo.clear ();
    m_to_undo_to_unknown.clear ();
    m_to_undo_devices.clear ();
    m_to_undo_subcircuits.clear ();
  }

  void swap (TentativeNodeMapping &other)
  {
    m_to_undo.swap (other.m_to_undo);
    m_to_undo_to_unknown.swap (other.m_to_undo_to_unknown);
    m_to_undo_devices.swap (other.m_to_undo_devices);
    m_to_undo_subcircuits.swap (other.m_to_undo_subcircuits);
  }

  std::vector<std::pair<NetGraph *, size_t> > nodes_tracked ()
  {
    std::vector<std::pair<NetGraph *, size_t> > res = m_to_undo;
    res.insert (res.end (), m_to_undo_to_unknown.begin (), m_to_undo_to_unknown.end ());
    return res;
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

/**
 *  @brief Returns true if the edges (given by transition iterators) are compatible with already established device or subcircuit equivalences.
 */
static bool edges_are_compatible (const NetGraphNode::edge_type &e, const NetGraphNode::edge_type &e_other, const DeviceEquivalenceTracker &device_eq, const SubCircuitEquivalenceTracker &sc_eq)
{
  std::vector<Transition>::const_iterator t1 = e.first.begin (), tt1 = e.first.end ();
  std::vector<Transition>::const_iterator t2 = e_other.first.begin (), tt2 = e_other.first.end ();

  std::vector<void *> p1, p2;

  while (t1 != tt1 && t2 != tt2) {

    std::vector<Transition>::const_iterator t10 = t1, t20 = t2;

    p1.clear ();
    while (t1 != tt1 && *t1 == *t10) {
      if (t1->is_for_subcircuit ()) {
        p1.push_back ((void *) sc_eq.other (t1->subcircuit ()));
      } else {
        p1.push_back ((void *) device_eq.other (t1->device ()));
      }
      ++t1;
    }

    p2.clear ();
    while (t2 != tt2 && *t2 == *t20) {
      if (t2->is_for_subcircuit ()) {
        p2.push_back ((void *) (sc_eq.other (t2->subcircuit ()) ? t2->subcircuit () : 0));
      } else {
        p2.push_back ((void *) (device_eq.other (t2->device ()) ? t2->device () : 0));
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

// --------------------------------------------------------------------------------------------------------------------

/**
 *  @brief Represents an interval of NetGraphNode objects in a node set
 */
struct NodeRange
{
  NodeRange (size_t _num1, std::vector<NodeEdgePair>::iterator _n1, std::vector<NodeEdgePair>::iterator _nn1,
             size_t _num2, std::vector<NodeEdgePair>::iterator _n2, std::vector<NodeEdgePair>::iterator _nn2)
    : num1 (_num1), num2 (_num2), n1 (_n1), nn1 (_nn1), n2 (_n2), nn2 (_nn2)
  {
    //  .. nothing yet ..
  }

  bool operator< (const NodeRange &other) const
  {
    if (num1 != other.num1) {
      return num1 < other.num1;
    }
    return num2 < other.num2;
  }

  size_t num1, num2;
  std::vector<NodeEdgePair>::iterator n1, nn1, n2, nn2;
};

// --------------------------------------------------------------------------------------------------------------------
//  NetlistCompareCore implementation

NetlistCompareCore::NetlistCompareCore (NetGraph *graph, NetGraph *other_graph)
  : max_depth (0),
    max_n_branch (0),
    depth_first (true),
    dont_consider_net_names (false),
    with_ambiguous (false),
    logger (0),
    with_log (true),
    circuit_pin_mapper (0),
    subcircuit_equivalence (0),
    device_equivalence (0),
    progress (0),
    mp_graph (graph),
    mp_other_graph (other_graph)
{
  //  .. nothing yet ..
}

size_t
NetlistCompareCore::derive_node_identities_for_edges (NetGraphNode::edge_iterator e, NetGraphNode::edge_iterator ee, NetGraphNode::edge_iterator e_other, NetGraphNode::edge_iterator ee_other, size_t net_index, size_t other_net_index, size_t depth, size_t n_branch, TentativeNodeMapping *tentative) const
{
  //  NOTE: we can skip edges to known nodes because we did a pre-analysis making sure those are compatible

  std::vector<NodeEdgePair> nodes;
  nodes.reserve (ee - e);

  std::vector<NodeEdgePair> other_nodes;
  other_nodes.reserve (ee_other - e_other);

  tl_assert (e->first == e_other->first);

  for (NetGraphNode::edge_iterator i = e; i != ee; ++i) {
    if (i->second.first != net_index) {
      const NetGraphNode *nn = &mp_graph->node (i->second.first);
      if (! nn->has_other ()) {
        nodes.push_back (NodeEdgePair (nn, i));
      }
    }
  }

  if (! nodes.empty ()) {   //  if non-ambiguous, non-assigned

    for (NetGraphNode::edge_iterator i = e_other; i != ee_other; ++i) {
      if (i->second.first != other_net_index) {
        const NetGraphNode *nn = &mp_other_graph->node (i->second.first);
        if (! nn->has_other ()) {
          other_nodes.push_back (NodeEdgePair (nn, i));
        }
      }
    }

  }

  if (nodes.empty () || other_nodes.empty ()) {
    return 0;
  }

  if (tentative) {

    if (nodes.size () != other_nodes.size ()) {
      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
        tl::info << nl_compare_debug_indent(depth) << "=> rejected branch.";
      }
      return failed_match;
    }

  }

  std::sort (nodes.begin (), nodes.end (), CompareNodeEdgePair ());
  std::sort (other_nodes.begin (), other_nodes.end (), CompareNodeEdgePair ());

  if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {

    //  print transitions if requested

    tl::info << nl_compare_debug_indent(depth) << "considering transitions:";

    bool first = true;

    for (std::vector<NodeEdgePair>::const_iterator i = nodes.begin (); i != nodes.end (); ++i) {
      const NetGraphNode *nn = i->node;
      if (first) {
        tl::info << nl_compare_debug_indent (depth) << "  here: " << (mp_graph->node (net_index).net () ? mp_graph->node (net_index).net ()->expanded_name ().c_str () : "(null)") << " ->";
        first = false;
      }
      tl::info << nl_compare_debug_indent (depth) << "    " << (nn->net () ? nn->net ()->expanded_name ().c_str() : "(null)") << " via: " << tl::noendl;
      for (std::vector<Transition>::const_iterator t = i->edge->first.begin (); t != i->edge->first.end(); ++t) {
        tl::info << (t != i->edge->first.begin () ? "; " : "") << t->to_string() << tl::noendl;
      }
      tl::info << "";
    }

    first = true;

    for (std::vector<NodeEdgePair>::const_iterator i = other_nodes.begin (); i != other_nodes.end (); ++i) {
      const NetGraphNode *nn = i->node;
      if (first) {
        tl::info << nl_compare_debug_indent (depth) << "  there: " << (mp_other_graph->node (other_net_index).net () ? mp_other_graph->node (other_net_index).net ()->expanded_name ().c_str () : "(null)") << " ->";
        first = false;
      }
      tl::info << nl_compare_debug_indent(depth) << "    " << (nn->net() ? nn->net()->expanded_name().c_str() : "(null)") << " via: " << tl::noendl;
      for (std::vector<Transition>::const_iterator t = i->edge->first.begin (); t != i->edge->first.end(); ++t) {
        tl::info << (t != i->edge->first.begin () ? "; " : "") << t->to_string() << tl::noendl;
      }
      tl::info << "";
    }

  }

  //  for the purpose of match evaluation we require an exact match of the node structure

  if (tentative) {

    //  1:1 pairing is less strict
    if (nodes.size () > 1) {
      for (size_t i = 0; i < nodes.size (); ++i) {
        if (! (*nodes[i].node == *other_nodes[i].node)) {
          if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
            tl::info << nl_compare_debug_indent(depth) << "=> rejected branch.";
          }
          return failed_match;
        }
      }
    }

  }

  //  propagate pairing in picky mode: this means we only accept a match if the node set
  //  is exactly identical and no ambiguous nodes are present when ambiguous nodes are forbidden

  size_t bt_count = derive_node_identities_from_node_set (nodes, other_nodes, depth, n_branch, tentative);

  if (bt_count == failed_match) {
    if (tentative) {
      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
        tl::info << nl_compare_debug_indent(depth) << "=> rejected branch.";
      }
    } else {
      bt_count = 0;
    }
  }

  if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
    if (! bt_count) {
      tl::info << nl_compare_debug_indent(depth) << "=> no updates.";
    }
  }
  return bt_count;
}

static bool has_subcircuits (db::NetGraphNode::edge_iterator e, db::NetGraphNode::edge_iterator ee)
{
  while (e != ee) {
    for (std::vector<Transition>::const_iterator t = e->first.begin (); t != e->first.end (); ++t) {
      if (t->is_for_subcircuit ()) {
        return true;
      }
    }
    ++e;
  }
  return false;
}

size_t
NetlistCompareCore::derive_node_identities (size_t net_index) const
{
  return derive_node_identities (net_index, 0, 1, (TentativeNodeMapping *) 0);
}

size_t
NetlistCompareCore::derive_node_identities (size_t net_index, size_t depth, size_t n_branch, TentativeNodeMapping *tentative) const
{
  NetGraphNode *n = & mp_graph->node (net_index);

  size_t other_net_index = n->other_net_index ();
  NetGraphNode *n_other = & mp_other_graph->node (other_net_index);

  NetGraphNode nn, nn_other;

  //  If there are subcircuits on the graph we temporarily create edges from our node to the other nodes of
  //  the subcircuit. This way we don't need to keep #pin*(#pin-1) edges

  if (has_subcircuits (n->begin (), n->end ())) {

    nn = *n;
    nn.expand_subcircuit_nodes (mp_graph);
    n = &nn;

    nn_other = *n_other;
    nn_other.expand_subcircuit_nodes (mp_other_graph);
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
      other_nodes_translated.reserve (ee_other - e_other);

      tl_assert (e->first == e_other->first);

      for (NetGraphNode::edge_iterator i = e; i != ee; ++i) {
        if (i->second.first != net_index) {
          const NetGraphNode *nn = &mp_graph->node (i->second.first);
          if (nn->has_other ()) {
            nodes.push_back (nn);
          } else {
            analysis_required = true;
          }
        }
      }

      for (NetGraphNode::edge_iterator i = e_other; i != ee_other; ++i) {
        if (i->second.first != other_net_index) {
          const NetGraphNode *nn = &mp_other_graph->node (i->second.first);
          if (nn->has_other ()) {
            other_nodes_translated.push_back (&mp_graph->node (nn->other_net_index ()));
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

  if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
    if (! tentative) {
      tl::info << nl_compare_debug_indent(depth) << "deducing from pair: " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
    } else {
      tl::info << nl_compare_debug_indent(depth) << "tentatively deducing from pair: " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
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

      size_t bt_count = derive_node_identities_for_edges (e, ee, e_other, ee_other, net_index, other_net_index, depth, n_branch, tentative);
      if (bt_count == failed_match) {
        if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
          tl::info << nl_compare_debug_indent(depth) << "=> rejected pair.";
        }
        return bt_count;
      } else {
        new_nodes += bt_count;
      }

    }

    e = ee;

  }

  if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
    if (! tentative && new_nodes > 0) {
      tl::info << nl_compare_debug_indent(depth) << "=> finished pair deduction: " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name () << " with " << new_nodes << " new pairs";
    }
  }

  return new_nodes;
}

namespace {

  struct SortNodeByNet
  {
    public:
      bool operator() (const NodeEdgePair &a, const NodeEdgePair &b) const
      {
        tl_assert (a.node->net () && b.node->net ());
        return name_compare (a.node->net (), b.node->net ()) < 0;
      }
  };

}

static void sort_node_range_by_best_match (const NodeRange &nr)
{
  std::stable_sort (nr.n1, nr.nn1, SortNodeByNet ());
  std::stable_sort (nr.n2, nr.nn2, SortNodeByNet ());

  std::vector<NodeEdgePair> nomatch1, nomatch2;
  nomatch1.reserve (nr.nn1 - nr.n1);
  nomatch2.reserve (nr.nn2 - nr.n2);

  std::vector<NodeEdgePair>::const_iterator i = nr.n1, j = nr.n2;
  std::vector<NodeEdgePair>::iterator iw = nr.n1, jw = nr.n2;

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

size_t
NetlistCompareCore::derive_node_identities_from_ambiguity_group (const NodeRange &nr, DeviceMapperForTargetNode &dm, DeviceMapperForTargetNode &dm_other, SubCircuitMapperForTargetNode &scm, SubCircuitMapperForTargetNode &scm_other, size_t depth, size_t n_branch, TentativeNodeMapping *tentative) const
{
  tl::AbsoluteProgress local_progress (tl::to_string (tr ("Deriving match for ambiguous net group")));

  std::string indent_s;
  if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
    indent_s = nl_compare_debug_indent (depth);
    indent_s += "*" + tl::to_string (n_branch) + " ";
  }

  size_t new_nodes = 0;
  size_t complexity = std::max (nr.num1, nr.num2);

  //  sort the ambiguity group such that net names match best

  std::vector<std::pair<const NetGraphNode *, const NetGraphNode *> > pairs;
  std::list<TentativeNodeMapping> tn_for_pairs;
  tl::equivalence_clusters<const NetGraphNode *> equivalent_other_nodes;

  sort_node_range_by_best_match (nr);

  {

    //  marks the nodes from the ambiguity group as unknown so we don't revisit them (causing deep recursion)
    TentativeNodeMapping tn_temp;

    //  collect and mark the ambiguity combinations to consider
    std::vector<std::vector<NodeEdgePair>::const_iterator> iters1, iters2;

    for (std::vector<NodeEdgePair>::const_iterator i1 = nr.n1; i1 != nr.nn1; ++i1) {
      if (! i1->node->has_any_other ()) {
        iters1.push_back (i1);
        size_t ni = mp_graph->node_index_for_net (i1->node->net ());
        TentativeNodeMapping::map_to_unknown (&tn_temp, mp_graph, ni);
      }
    }

    for (std::vector<NodeEdgePair>::const_iterator i2 = nr.n2; i2 != nr.nn2; ++i2) {
      if (! i2->node->has_any_other ()) {
        iters2.push_back (i2);
        size_t other_ni = mp_other_graph->node_index_for_net (i2->node->net ());
        TentativeNodeMapping::map_to_unknown (&tn_temp, mp_other_graph, other_ni);
      }
    }

    for (std::vector<std::vector<NodeEdgePair>::const_iterator>::const_iterator ii1 = iters1.begin (); ii1 != iters1.end (); ++ii1) {

      std::vector<NodeEdgePair>::const_iterator i1 = *ii1;

      //  use net names to resolve ambiguities or for passive nets
      //  (Rationale for the latter: passive nets cannot be told apart topologically and are typical for blackbox models.
      //  So the net name is the only differentiator)
      bool use_name = ! dont_consider_net_names || i1->node->net ()->is_passive ();
      bool use_topology = dont_consider_net_names || i1->node->net ()->is_passive ();

      //  in tentative mode, reject this choice if nets are named and all other nets in the ambiguity group differ -> this favors net matching by name
      if (use_name && tentative) {

        bool any_matching = false;
        for (std::vector<std::vector<NodeEdgePair>::const_iterator>::iterator ii2 = iters2.begin (); ii2 != iters2.end () && ! any_matching; ++ii2) {
          std::vector<NodeEdgePair>::const_iterator i2 = *ii2;
          any_matching = !net_names_are_different (i1->node->net (), i2->node->net ());
        }

        if (! any_matching) {
          if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
            tl::info << indent_s << "ambiguity group rejected - all ambiguous other net names are mismatching for: " << i1->node->net ()->expanded_name ();
          }
          //  a mismatch - stop here.
          return failed_match;
        }

      }

      bool any = false;
      bool need_rerun = false;
      size_t node_count = 0;
      std::vector<std::vector<NodeEdgePair>::const_iterator>::iterator to_remove = iters2.end ();

      for (std::vector<std::vector<NodeEdgePair>::const_iterator>::iterator ii2 = iters2.begin (); ii2 != iters2.end (); ++ii2) {

        ++local_progress;

        std::vector<NodeEdgePair>::const_iterator i2 = *ii2;

        //  try this candidate in tentative mode
        if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
          tl::info << indent_s << "trying in tentative mode: " << i1->node->net ()->expanded_name () << " vs. " << i2->node->net ()->expanded_name ();
        }

        if (! edges_are_compatible (*i1->edge, *i2->edge, *device_equivalence, *subcircuit_equivalence)) {
          if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
            tl::info << indent_s << "=> rejected because edges are incompatible with already established device or subcircuit equivalences";
          }
          continue;
        }

        if (use_name && net_names_are_equal (i1->node->net (), i2->node->net ())) {

          if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
            tl::info << indent_s << "=> accepted for identical names";
          }

          //  utilize net names to propose a match
          if (any) {
            pairs.pop_back ();
          }
          pairs.push_back (std::make_pair (i1->node, i2->node));
          to_remove = ii2;
          node_count = 1;
          any = true;
          break;

        } else if (use_topology) {

          size_t ni = mp_graph->node_index_for_net (i1->node->net ());
          size_t other_ni = mp_other_graph->node_index_for_net (i2->node->net ());

          TentativeNodeMapping tn;
          TentativeNodeMapping::map_pair_from_unknown (&tn, mp_graph, ni, mp_other_graph, other_ni, dm, dm_other, *device_equivalence, scm, scm_other, *subcircuit_equivalence, depth);

          size_t bt_count = derive_node_identities (ni, depth + 1, complexity * n_branch, &tn);

          if (bt_count != failed_match) {

            if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
              tl::info << indent_s << "match found";
            }
            //  we have a match ...

            if (any) {

              //  there is already a known pair, so we can mark *i2 and the previous *i2 as equivalent
              //  (makes them ambiguous)
              equivalent_other_nodes.same (i2->node, pairs.back ().second);
              //  we know enough now ...
              break;

            } else {

              //  identified a new pair
              node_count = bt_count + 1;
              pairs.push_back (std::make_pair (i1->node, i2->node));
              to_remove = ii2;
              need_rerun = true;
              any = true;

              //  no ambiguity analysis in tentative mode - we can stop now
              if (tentative) {
                break;
              }

            }

          }

        }

      }

      if (any) {

        new_nodes += node_count;

        //  Add the new pair to the temporary mapping (even in tentative mode)
        //  Reasoning: doing the mapping may render other nets incompatible, so to ensure "edges_are_compatible" works properly we
        //  need to lock the current pairs resources such as devices by listing them in the mapping. This is doing by "derive_*_equivalence" inside
        //  TentativeNodeMapping::map_pair

        std::vector<NodeEdgePair>::const_iterator i2 = *to_remove;

        size_t ni = mp_graph->node_index_for_net (i1->node->net ());
        size_t other_ni = mp_other_graph->node_index_for_net (i2->node->net ());

        TentativeNodeMapping::map_pair (&tn_temp, mp_graph, ni, mp_other_graph, other_ni, dm, dm_other, *device_equivalence, scm, scm_other, *subcircuit_equivalence, depth);

        if (need_rerun && ! tentative) {

          //  Re-run the mapping for the selected pair and stash that - this will lock this mapping when investigating other
          //  branches of the ambiguity resolution tree

          if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare || tl::verbosity () >= 40) {
            tl::info << indent_s << "finalizing decision (rerun tracking): " << i1->node->net ()->expanded_name () << " vs. " << i2->node->net ()->expanded_name ();
          }

          tn_for_pairs.push_back (TentativeNodeMapping ());
          size_t bt_count = derive_node_identities (ni, depth + 1, complexity * n_branch, &tn_for_pairs.back ());
          tl_assert (bt_count != failed_match);

        }

        //  now we can get rid of the node and reduce the "other" list of ambiguous nodes
        iters2.erase (to_remove);

      }

      if (! any && tentative) {
        if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
          tl::info << indent_s << "mismatch.";
        }
        //  a mismatch - stop here.
        return failed_match;
      }

    }

  }

  if (! tentative) {

    //  issue the matching pairs

    //  ambiguous pins
    std::vector<size_t> pa, pb;
    std::set<const db::Net *> seen;

    for (std::vector<std::pair<const NetGraphNode *, const NetGraphNode *> >::const_iterator p = pairs.begin (); p != pairs.end (); ++p) {

      size_t ni = mp_graph->node_index_for_net (p->first->net ());
      size_t other_ni = mp_other_graph->node_index_for_net (p->second->net ());

      TentativeNodeMapping::map_pair (0, mp_graph, ni, mp_other_graph, other_ni, dm, dm_other, *device_equivalence, scm, scm_other, *subcircuit_equivalence, depth);

      bool ambiguous = equivalent_other_nodes.has_attribute (p->second);

      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare || tl::verbosity () >= 40) {
        if (ambiguous) {
          tl::info << indent_s << "deduced ambiguous match: " << p->first->net ()->expanded_name () << " vs. " << p->second->net ()->expanded_name ();
        } else {
          tl::info << indent_s << "deduced match: " << p->first->net ()->expanded_name () << " vs. " << p->second->net ()->expanded_name ();
        }
      }

      tl_assert (seen.find (p->first->net ()) == seen.end ());
      seen.insert (p->first->net ());

      if (ambiguous) {
        if (logger) {
          if (with_log) {
            logger->log_entry (db::Warning,
                               tl::sprintf (tl::to_string (tr ("Matching nets %s from an ambiguous group of nets")), nets2string (p->first->net (), p->second->net ())));
          }
          logger->match_ambiguous_nets (p->first->net (), p->second->net ());
        }
        for (db::Net::const_pin_iterator i = p->first->net ()->begin_pins (); i != p->first->net ()->end_pins (); ++i) {
          pa.push_back (i->pin ()->id ());
        }
        for (db::Net::const_pin_iterator i = p->second->net ()->begin_pins (); i != p->second->net ()->end_pins (); ++i) {
          pb.push_back (i->pin ()->id ());
        }
      } else if (logger) {
        logger->match_nets (p->first->net (), p->second->net ());
      }

      ++*progress;

    }

    //  Establish further mappings from the mappings stashed during tentative evaluation

    std::vector<std::pair<const NetGraphNode *, const NetGraphNode *> >::const_iterator p = pairs.begin ();
    for (std::list<TentativeNodeMapping>::iterator tn_of_pair = tn_for_pairs.begin (); tn_of_pair != tn_for_pairs.end (); ++tn_of_pair, ++p) {

      bool was_ambiguous = equivalent_other_nodes.has_attribute (p->second);

      //  Note: this would propagate ambiguities to all *derived* mappings. But this probably goes too far:
      //    bool ambiguous = was_ambiguous;
      //  Instead we ignore propagated ambiguitied for now:
      bool ambiguous = false;

      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare || tl::verbosity () >= 40) {
        tl::info << indent_s << "propagating from deduced match: " << p->first->net ()->expanded_name () << " vs. " << p->second->net ()->expanded_name ();
      }

      std::vector<std::pair<NetGraph *, size_t> > nn = tn_of_pair->nodes_tracked ();

      for (std::vector<std::pair<NetGraph *, size_t> >::const_iterator i = nn.begin (); i != nn.end (); ++i) {

        if (i->first != mp_graph) {
          continue;
        }

        NetGraphNode *n = & mp_graph->node (i->second);

        //  tentative evaluation paths may render equivalences which are included in the initial node set,
        //  hence we filter those out here
        if (seen.find (n->net ()) != seen.end ()) {
          continue;
        }
        seen.insert (n->net ());

        size_t other_net_index = n->other_net_index ();
        NetGraphNode *n_other = & mp_other_graph->node (other_net_index);

        if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare || tl::verbosity () >= 40) {
          if (was_ambiguous) {
            tl::info << indent_s << "deduced from ambiguous match: " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
          } else {
            tl::info << indent_s << "deduced match: " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
          }
        }

        if (logger && with_log && was_ambiguous) {
          logger->log_entry (db::Info,
                             tl::sprintf (tl::to_string (tr ("Matching nets %s following an ambiguous match")), nets2string (n->net (), n_other->net ())));
        }

        if (ambiguous) {
          if (logger) {
            logger->match_ambiguous_nets (n->net (), n_other->net ());
          }
          for (db::Net::const_pin_iterator i = n->net ()->begin_pins (); i != n->net ()->end_pins (); ++i) {
            pa.push_back (i->pin ()->id ());
          }
          for (db::Net::const_pin_iterator i = n_other->net ()->begin_pins (); i != n_other->net ()->end_pins (); ++i) {
            pb.push_back (i->pin ()->id ());
          }
        } else if (logger) {
          logger->match_nets (n->net (), n_other->net ());
        }

      }

      tn_of_pair->clear ();

    }

    //  marks pins on ambiguous nets as swappable

    if (! pa.empty ()) {
      circuit_pin_mapper->map_pins (mp_graph->circuit (), pa);
    }
    if (! pb.empty ()) {
      circuit_pin_mapper->map_pins (mp_other_graph->circuit (), pb);
    }

  } else {

    for (std::vector<std::pair<const NetGraphNode *, const NetGraphNode *> >::const_iterator p = pairs.begin (); p != pairs.end (); ++p) {

      size_t ni = mp_graph->node_index_for_net (p->first->net ());
      size_t other_ni = mp_other_graph->node_index_for_net (p->second->net ());

      TentativeNodeMapping::map_pair (tentative, mp_graph, ni, mp_other_graph, other_ni, dm, dm_other, *device_equivalence, scm, scm_other, *subcircuit_equivalence, depth);

    }

  }

  return new_nodes;
}

size_t
NetlistCompareCore::derive_node_identities_from_singular_match (const NetGraphNode *n, const NetGraphNode::edge_iterator &e, const NetGraphNode *n_other, const NetGraphNode::edge_iterator &e_other, DeviceMapperForTargetNode &dm, DeviceMapperForTargetNode &dm_other, SubCircuitMapperForTargetNode &scm, SubCircuitMapperForTargetNode &scm_other, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, bool consider_net_names) const
{
  std::string indent_s;
  if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
    indent_s = nl_compare_debug_indent (depth);
    indent_s += "*" + tl::to_string (n_branch) + " ";
  }

  if (! edges_are_compatible (*e, *e_other, *device_equivalence, *subcircuit_equivalence)) {

    if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
      tl::info << nl_compare_debug_indent(depth) << "=> rejected because edges are incompatible with already established device or subcircuit equivalences";
    }
    return tentative ? failed_match : 0;

  } else if ((! n->has_any_other () && ! n_other->has_any_other ()) || (n->has_unknown_other () && n_other->has_unknown_other ())) {

    //  in tentative mode, reject this choice if both nets are named and
    //  their names differ -> this favors net matching by name

    if (tentative && consider_net_names && net_names_are_different (n->net (), n_other->net ())) {
      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
        tl::info << indent_s << "rejecting pair as names are not identical: " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
      }
      return failed_match;
    }

    //  A single candidate: just take this one -> this may render
    //  inexact matches, but further propagates net pairing

    size_t ni = mp_graph->node_index_for_net (n->net ());
    size_t other_ni = mp_other_graph->node_index_for_net (n_other->net ());

    bool exact_match = (mp_graph->node (ni) == mp_other_graph->node (other_ni));

    if (n->has_unknown_other ()) {
      TentativeNodeMapping::map_pair_from_unknown (tentative, mp_graph, ni, mp_other_graph, other_ni, dm, dm_other, *device_equivalence, scm, scm_other, *subcircuit_equivalence, depth);
    } else {
      TentativeNodeMapping::map_pair (tentative, mp_graph, ni, mp_other_graph, other_ni, dm, dm_other, *device_equivalence, scm, scm_other, *subcircuit_equivalence, depth, exact_match);
    }

    if (! tentative) {
      ++*progress;
      if (logger) {
        if (! exact_match) {
          //  this is a mismatch, but we continue with this
          if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare || tl::verbosity () >= 40) {
            tl::info << indent_s << "deduced mismatch (singular): " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
          }
          logger->net_mismatch (n->net (), n_other->net ());
        } else {
          if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare || tl::verbosity () >= 40) {
            tl::info << indent_s << "deduced match (singular): " << n->net ()->expanded_name () << " vs. " << n_other->net ()->expanded_name ();
          }
          logger->match_nets (n->net (), n_other->net ());
        }
      }
    }

    size_t new_nodes = 1;

    if ((depth_first || tentative) && (max_depth == std::numeric_limits<size_t>::max() || depth < max_depth)) {
      size_t bt_count = derive_node_identities (ni, depth + 1, n_branch, tentative);
      if (bt_count == failed_match) {
        if (tentative) {
          return failed_match;
        }
      } else {
        new_nodes += bt_count;
      }
    }

    return new_nodes;

  } else if (n->has_other ()) {

    //  this decision leads to a contradiction
    if (mp_other_graph->node_index_for_net (n_other->net ()) != n->other_net_index ()) {
      return failed_match;
    } else {
      return 0;
    }

  } else {

    //  mismatch of assignment state
    return failed_match;

  }
}

static size_t distance (const NetGraphNode &a, const NetGraphNode &b)
{
  auto i = a.begin ();
  auto j = b.begin ();

  size_t fuzz = 0;

  while (i != a.end () || j != b.end ()) {

    if (j == b.end ()) {
      ++fuzz;
      ++i;
      continue;
    }
    if (i == a.end ()) {
      ++fuzz;
      ++j;
      continue;
    }

    if (i->first < j->first) {
      ++fuzz;
      ++i;
      continue;
    } else if (j->first < i->first) {
      ++fuzz;
      ++j;
      continue;
    }

    ++i;
    ++j;

  }

  return fuzz;
}

static size_t distance3 (const NetGraphNode &a, const NetGraphNode &b1, const NetGraphNode &b2, const NetGraph &gb)
{
  bool needs_join = false;

  for (auto e = b1.begin (); e != b1.end () && ! needs_join; ++e) {
    needs_join = (e->second.second == b1.net () || e->second.second == b2.net ());
  }

  for (auto e = b2.begin (); e != b2.end () && ! needs_join; ++e) {
    needs_join = (e->second.second == b1.net () || e->second.second == b2.net ());
  }

  if (needs_join) {
    return distance (a, gb.joined (b1, b2));
  }

  auto i = a.begin ();
  auto j1 = b1.begin ();
  auto j2 = b2.begin ();

  size_t fuzz = 0;

  while (i != a.end () || j1 != b1.end () || j2 != b2.end ()) {

    if (j1 == b1.end () && j2 == b2.end ()) {
      ++fuzz;
      ++i;
      continue;
    }

    bool use_j1 = j2 == b2.end () || (j1 != b1.end () && *j1 < *j2);
    auto &j = use_j1 ? j1 : j2;

    if (i == a.end ()) {
      ++fuzz;
      ++j;
      continue;
    }

    if (i->first < j->first) {
      ++fuzz;
      ++i;
      continue;
    } else if (j->first < i->first) {
      ++fuzz;
      ++j;
      continue;
    }

    ++i;
    ++j;

  }

  return fuzz;
}

static void
analyze_nodes_for_close_matches (const std::multimap<size_t, const NetGraphNode *> &nodes_by_edges1, const std::multimap<size_t, const NetGraphNode *> &nodes_by_edges2, bool layout2ref, db::NetlistCompareLogger *logger, const db::NetGraph &g2)
{
  size_t max_search = 100;
  double max_fuzz_factor = 0.25;
  size_t max_fuzz_count = 3;
  size_t max_edges_split = 3;  //  by how many edges joining will reduce the edge count at max
  size_t min_edges = 2;

  std::string msg;
  if (layout2ref) {
    msg = tl::to_string (tr ("Net %s may be shorting nets %s and %s from reference netlist (fuzziness %d nodes)"));
  } else {
    msg = tl::to_string (tr ("Connecting nets %s and %s is making a better match to net %s from reference netlist (fuzziness %d nodes)"));
  }

  for (auto i = nodes_by_edges1.begin (); i != nodes_by_edges1.end (); ++i) {

    if (i->first < min_edges) {
      continue;
    }

    std::set<const db::NetGraphNode *> seen;

    for (auto j = nodes_by_edges2.begin (); j != nodes_by_edges2.end (); ++j) {

      seen.insert (j->second);

      if (j->first > i->first + max_fuzz_count - 1) {
        break;
      }

      size_t ne = i->first > j->first ? i->first - j->first : 0;
      if (ne > max_fuzz_count) {
        ne -= max_fuzz_count;
      }

      if (ne == 0 && layout2ref) {

        //  analyze nets for similarities (only layout -> ref as the other case is symmetric)

        size_t fuzz = distance (*i->second, *j->second);
        double fuzz_factor = double (fuzz) / ne;
        if (fuzz_factor < max_fuzz_factor) {
          std::string msg = tl::to_string (tr ("Net %s from netlist approximately matches net %s from reference netlist (fuzziness %d nodes)"));
          logger->log_entry (db::Info, tl::sprintf (msg,
                                  i->second->net ()->expanded_name (),
                                  j->second->net ()->expanded_name (),
                                  int (fuzz)));
        }

      }

      auto k = nodes_by_edges2.lower_bound (ne);

      size_t tries = max_search;
      for ( ; k != nodes_by_edges2.end () && j->first + k->first < i->first + max_fuzz_count + max_edges_split && tries > 0; ++k) {

        if (seen.find (k->second) != seen.end ()) {
          continue;
        }

        size_t fuzz = distance3 (*i->second, *j->second, *k->second, g2);
        double fuzz_factor = double (fuzz) / i->first;
        if (fuzz_factor < max_fuzz_factor) {
          logger->log_entry (db::Info, tl::sprintf (msg,
                                  (layout2ref ? i : j)->second->net ()->expanded_name (),
                                  (layout2ref ? j : k)->second->net ()->expanded_name (),
                                  (layout2ref ? k : i)->second->net ()->expanded_name (),
                                  int (fuzz)));
        }

        --tries;

      }

    }

  }
}

void
NetlistCompareCore::analyze_failed_matches () const
{
  //  Determine the range of nodes with same identity

  std::vector<NetGraphNode::edge_type> no_edges;
  no_edges.push_back (NetGraphNode::edge_type ());

  std::vector<NodeEdgePair> nodes, other_nodes;

  nodes.reserve (mp_graph->end () - mp_graph->begin ());
  for (db::NetGraph::node_iterator i1 = mp_graph->begin (); i1 != mp_graph->end (); ++i1) {
    if (i1->net ()) {
      nodes.push_back (NodeEdgePair (i1.operator-> (), no_edges.begin ()));
    }
  }

  other_nodes.reserve (mp_other_graph->end () - mp_other_graph->begin ());
  for (db::NetGraph::node_iterator i2 = mp_other_graph->begin (); i2 != mp_other_graph->end (); ++i2) {
    if (i2->net ()) {
      other_nodes.push_back (NodeEdgePair (i2.operator-> (), no_edges.begin ()));
    }
  }

  std::sort (nodes.begin (), nodes.end (), CompareNodeEdgePair ());
  std::sort (other_nodes.begin (), other_nodes.end (), CompareNodeEdgePair ());

  auto n1 = nodes.begin ();
  auto n2 = other_nodes.begin ();

  std::vector<const db::NetGraphNode *> singular1, singular2;

  while (n1 != nodes.end () || n2 != other_nodes.end ()) {

    if (n2 == other_nodes.end ()) {
      singular1.push_back (n1->node);
      ++n1;
      continue;
    } else if (n1 == nodes.end ()) {
      singular2.push_back (n2->node);
      ++n2;
      continue;
    }

    if (*n1->node < *n2->node) {
      singular1.push_back (n1->node);
      ++n1;
      continue;
    } else if (*n2->node < *n1->node) {
      singular2.push_back (n2->node);
      ++n2;
      continue;
    }

    ++n1;
    ++n2;

  }

  for (auto i = singular1.begin (); i != singular1.end (); ++i) {
    logger->log_entry (db::Error, tl::sprintf (tl::to_string (tr ("Net %s is not matching any net from reference netlist")), (*i)->net ()->expanded_name ()));
  }

  //  attempt some analysis for close matches (including shorts / opens)

  std::multimap<size_t, const NetGraphNode *> nodes_by_edges1, nodes_by_edges2;

  for (auto i = singular1.begin (); i != singular1.end (); ++i) {
    const NetGraphNode *n = *i;
    nodes_by_edges1.insert (std::make_pair (n->end () - n->begin (), n));
  }

  for (auto i = singular2.begin (); i != singular2.end (); ++i) {
    const NetGraphNode *n = *i;
    nodes_by_edges2.insert (std::make_pair (n->end () - n->begin (), n));
  }

  analyze_nodes_for_close_matches (nodes_by_edges1, nodes_by_edges2, true, logger, *mp_other_graph);
  analyze_nodes_for_close_matches (nodes_by_edges2, nodes_by_edges1, false, logger, *mp_graph);
}

size_t
NetlistCompareCore::derive_node_identities_from_node_set (std::vector<NodeEdgePair> &nodes, std::vector<NodeEdgePair> &other_nodes) const
{
  return derive_node_identities_from_node_set (nodes, other_nodes, 0, 1, (TentativeNodeMapping *) 0);
}

size_t
NetlistCompareCore::derive_node_identities_from_node_set (std::vector<NodeEdgePair> &nodes, std::vector<NodeEdgePair> &other_nodes, size_t depth, size_t n_branch, TentativeNodeMapping *tentative) const
{
  std::string indent_s;
  if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
    indent_s = nl_compare_debug_indent (depth);
    indent_s += "*" + tl::to_string (n_branch) + " ";
  }

  DeviceMapperForTargetNode dm;
  SubCircuitMapperForTargetNode scm;
  for (std::vector<NodeEdgePair>::const_iterator i = nodes.begin (); i != nodes.end (); ++i) {
    dm.insert (*i->edge);
    scm.insert (*i->edge);
  }

  DeviceMapperForTargetNode dm_other;
  SubCircuitMapperForTargetNode scm_other;
  for (std::vector<NodeEdgePair>::const_iterator i = other_nodes.begin (); i != other_nodes.end (); ++i) {
    dm_other.insert (*i->edge);
    scm_other.insert (*i->edge);
  }

  if (nodes.size () == 1 && other_nodes.size () == 1) {

    return derive_node_identities_from_singular_match (nodes.front ().node, nodes.front ().edge, other_nodes.front ().node, other_nodes.front ().edge,
                                                       dm, dm_other, scm, scm_other, depth, n_branch, tentative, false /*don't consider net names*/);

  }

  if (max_depth != std::numeric_limits<size_t>::max() && depth > max_depth) {
    if (with_log) {
      logger->log_entry (db::Warning, tl::sprintf (tl::to_string (tr ("Maximum depth exhausted (max depth is %d)")), int (max_depth)));
    }
    if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
      tl::info << indent_s << "max. depth exhausted (" << depth << ">" << max_depth << ")";
    }
    return failed_match;
  }

  //  Determine the range of nodes with same identity

  std::vector<NodeRange> node_ranges;
  size_t new_nodes = 0;

  std::vector<NodeEdgePair>::iterator n1 = nodes.begin ();
  std::vector<NodeEdgePair>::iterator n2 = other_nodes.begin ();

  while (n1 != nodes.end () && n2 != other_nodes.end ()) {

    if (n1->node->has_other ()) {
      ++n1;
      continue;
    } else if (n2->node->has_other ()) {
      ++n2;
      continue;
    }

    if (*n1->node < *n2->node) {
      ++n1;
      continue;
    } else if (*n2->node < *n1->node) {
      ++n2;
      continue;
    }

    std::vector<NodeEdgePair>::iterator nn1 = n1, nn2 = n2;

    ++nn1;
    ++nn2;

    size_t num1 = 1;
    while (nn1 != nodes.end () && *nn1->node == *n1->node) {
      if (! nn1->node->has_other ()) {
        ++num1;
      }
      ++nn1;
    }

    size_t num2 = 1;
    while (nn2 != other_nodes.end () && *nn2->node == *n2->node) {
      if (! nn2->node->has_other ()) {
        ++num2;
      }
      ++nn2;
    }

    if ((num1 == 1 && num2 == 1) || with_ambiguous) {
      node_ranges.push_back (NodeRange (num1, n1, nn1, num2, n2, nn2));
    }

    //  in tentative mode ambiguous nodes don't make a match without
    //  with_ambiguous
    if ((num1 > 1 || num2 > 1) && tentative && ! with_ambiguous) {
      return failed_match;
    }

    n1 = nn1;
    n2 = nn2;

  }

  if (with_ambiguous) {
    std::stable_sort (node_ranges.begin (), node_ranges.end ());
  }

  for (std::vector<NodeRange>::iterator nr = node_ranges.begin (); nr != node_ranges.end (); ++nr) {

    //  node ranges might have changed - adjust to real count and skip leading pairs assigned already

    while (nr->n1 != nr->nn1 && nr->n2 != nr->nn2) {
      if (nr->n1->node->has_other ()) {
        ++nr->n1;
      } else if (nr->n2->node->has_other ()) {
        ++nr->n2;
      } else {
        break;
      }
    }

    nr->num1 = 0;
    for (std::vector<NodeEdgePair>::const_iterator i = nr->n1; i != nr->nn1; ++i) {
      if (! i->node->has_other ()) {
        ++nr->num1;
      }
    }

    nr->num2 = 0;
    for (std::vector<NodeEdgePair>::const_iterator i = nr->n2; i != nr->nn2; ++i) {
      if (! i->node->has_other ()) {
        ++nr->num2;
      }
    }

    if (nr->num1 < 1 || nr->num2 < 1) {

      //  ignore this - it got obsolete.

    } else if (nr->num1 == 1 && nr->num2 == 1) {

      size_t n = derive_node_identities_from_singular_match (nr->n1->node, nr->n1->edge, nr->n2->node, nr->n2->edge, dm, dm_other, scm, scm_other, depth, n_branch, tentative, ! dont_consider_net_names);
      if (n == failed_match) {
        return failed_match;
      }

      new_nodes += n;

    } else if (max_n_branch != std::numeric_limits<size_t>::max () && double (std::max (nr->num1, nr->num2)) * double (n_branch) > double (max_n_branch)) {

      if (with_log) {
        logger->log_entry (db::Warning, tl::sprintf (tl::to_string (tr ("Maximum complexity exhausted (max complexity is %s, needs at least %s)")), tl::to_string (max_n_branch), tl::to_string (std::max (nr->num1, nr->num2) * n_branch)));
      }
      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
        tl::info << indent_s << "max. complexity exhausted (" << std::max (nr->num1, nr->num2) << "*" << n_branch << ">" << max_n_branch << ") - mismatch.";
      }
      return failed_match;

    } else {

      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
        tl::info << indent_s << "analyzing ambiguity group with " << nr->num1 << "/" << nr->num2 << " members";
      }

      size_t n = derive_node_identities_from_ambiguity_group (*nr, dm, dm_other, scm, scm_other, depth, n_branch, tentative);
      if (n == failed_match) {
        return failed_match;
      }

      new_nodes += n;

      if (db::NetlistCompareGlobalOptions::options ()->debug_netcompare) {
        tl::info << indent_s << "finished analysis of ambiguity group with " << nr->num1 << "/" << nr->num2 << " members";
      }

    }

  }

  return new_nodes;
}

}

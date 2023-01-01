
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

#ifndef _HDR_dbNetlistCompareCore
#define _HDR_dbNetlistCompareCore

#include "dbCommon.h"
#include "dbNetlistCompareGraph.h"

#include <string>
#include <limits>
#include <vector>
#include <algorithm>
#include <map>

namespace db
{

// --------------------------------------------------------------------------------------------------------------------
//  NetlistCompareCore definition

class TentativeNodeMapping;
struct NodeRange;
class DeviceMapperForTargetNode;
class SubCircuitMapperForTargetNode;

/**
 *  @brief The net graph for the compare algorithm
 */
class DB_PUBLIC NetlistCompareCore
{
public:
  typedef std::vector<NetGraphNode>::const_iterator node_iterator;

  NetlistCompareCore (NetGraph *graph, NetGraph *other_graph);

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
  size_t derive_node_identities (size_t net_index) const;

  /**
   *  @brief The backtracking driver
   *
   *  This method will analyze the given nodes and call "derive_node_identities" for all nodes
   *  with a proposed identity.
   */
  size_t derive_node_identities_from_node_set (std::vector<NodeEdgePair> &nodes, std::vector<NodeEdgePair> &other_nodes) const;

  /**
   *  @brief Analyzes the non-matched remaining nodes and produces log output
   */
  void analyze_failed_matches () const;

  size_t max_depth;
  size_t max_n_branch;
  bool depth_first;
  bool dont_consider_net_names;
  bool with_ambiguous;
  NetlistCompareLogger *logger;
  bool with_log;
  CircuitPinCategorizer *circuit_pin_mapper;
  SubCircuitEquivalenceTracker *subcircuit_equivalence;
  DeviceEquivalenceTracker *device_equivalence;
  tl::RelativeProgress *progress;

private:
  NetGraph *mp_graph;
  NetGraph *mp_other_graph;

  size_t derive_node_identities (size_t net_index, size_t depth, size_t n_branch, TentativeNodeMapping *tentative) const;
  size_t derive_node_identities_from_node_set (std::vector<NodeEdgePair> &nodes, std::vector<NodeEdgePair> &other_nodes, size_t depth, size_t n_branch, TentativeNodeMapping *tentative) const;
  size_t derive_node_identities_for_edges (NetGraphNode::edge_iterator e, NetGraphNode::edge_iterator ee, NetGraphNode::edge_iterator e_other, NetGraphNode::edge_iterator ee_other, size_t net_index, size_t other_net_index, size_t depth, size_t n_branch, TentativeNodeMapping *tentative) const;
  size_t derive_node_identities_from_ambiguity_group (const NodeRange &nr, DeviceMapperForTargetNode &dm, DeviceMapperForTargetNode &dm_other, SubCircuitMapperForTargetNode &scm, SubCircuitMapperForTargetNode &scm_other, size_t depth, size_t n_branch, TentativeNodeMapping *tentative) const;
  size_t derive_node_identities_from_singular_match (const NetGraphNode *n, const NetGraphNode::edge_iterator &e, const NetGraphNode *n_other, const NetGraphNode::edge_iterator &e_other, DeviceMapperForTargetNode &dm, DeviceMapperForTargetNode &dm_other, SubCircuitMapperForTargetNode &scm, SubCircuitMapperForTargetNode &scm_other, size_t depth, size_t n_branch, TentativeNodeMapping *tentative, bool consider_net_names) const;
};

}

#endif

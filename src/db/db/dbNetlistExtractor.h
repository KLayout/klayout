
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#ifndef _HDR_dbNetlistExtractor
#define _HDR_dbNetlistExtractor

#include "dbCommon.h"
#include "dbHierNetworkProcessor.h"

#include <map>

namespace db
{

class DeepShapeStore;
class Netlist;
class Circuit;
class SubCircuit;
class Net;

/**
 *  @brief The Netlist Extractor
 *
 *  This is the main object responsible for extracting nets from a layout.
 *
 *  The layout needs to be present as a DeepShapeStore shadow layout. Use hierarchical regions
 *  (db::Region build with a DeepShapeStore) to populate the shape store.
 *
 *  The extraction requires a connectivity definition through db::Connectivity.
 *
 *  In addition, the device extraction needs to happen before net extraction.
 *  Device extraction will pre-fill the netlist with circuits and devices and
 *  annotate the layout with terminal shapes, so the net extraction can connect
 *  to the device terminals.
 *
 *  If the deep shape store has been configured to supply text label annotated
 *  markers (DeepShapeStore::set_text_property_name and DeepShapeStore::set_text_enlargement
 *  to at least 1), texts from layers included in the connectivity will be extracted
 *  as net names. If multiple texts are present, the names will be concatenated using
 *  comma separators.
 *
 *  Upon extraction, the given netlist is filled with circuits (unless present already),
 *  subcircuits, pins and of course nets. This object also supplies access to the net's
 *  geometries through the clusters() method. This method delivers a hierarchical
 *  cluster object. The nets refer to specific clusters through their "cluster_id"
 *  attribute.
 */
class DB_PUBLIC NetlistExtractor
{
public:
  typedef db::hier_clusters<db::PolygonRef> hier_clusters_type;
  typedef db::connected_clusters<db::PolygonRef> connected_clusters_type;
  typedef db::local_cluster<db::PolygonRef> local_cluster_type;

  /**
   *  @brief NetExtractor constructor
   */
  NetlistExtractor ();

  /**
   *  @brief Extract the nets
   *  See the class description for more details.
   */
  void extract_nets (const db::DeepShapeStore &dss, const db::Connectivity &conn, db::Netlist *nl);

  /**
   *  @brief Gets the shape clusters
   *  See the class description for more details.
   */
  const hier_clusters_type &clusters () const
  {
    return m_net_clusters;
  }

private:
  hier_clusters_type m_net_clusters;

  void make_device_terminal_from_property (const tl::Variant &v, db::Circuit *circuit, db::Net *net);
  void make_net_name_from_property (const tl::Variant &v, db::Net *net);

  /**
   *  @brief Make a pin connection from clusters
   *
   *  Non-root clusters make a pin. This function creates the pin inside the given circuit. It
   *  returns the new pin's ID.
   */
  size_t make_pin (db::Circuit *circuit, db::Net *net);

  /**
   *  @brief Turns the connections of a cluster into subcircuit instances
   *
   *  Walks through the connections of a cluster and turns the connections into subcircuit pin
   *  connections. This will also create new subcircuit instances.
   *
   *  Needs:
   *   - circuit: the current circuit that is worked on
   *   - clusters: the connected clusters from the net extraction step (nets plus
   *     connections to child cell clusters inside the current cell)
   *   - cid: the current cluster ID in "clusters"
   *   - net: the net being built
   *   - circuits: a lookup table of circuits vs. cell index (reverse lookup)
   *   - pins_per_cluster: a per-cell, reverse lookup table for the pin id per child cell clusters
   *     (used to find the pin ID of a subcircuit from the child cell cluster ID)
   *   - dbu: the database unit (used to compute the micron-unit transformation of the child
   *     cell)
   *  Updates:
   *   - subcircuits: An cell instance to SubCircuit lookup table
   */
  void make_and_connect_subcircuits (db::Circuit *circuit,
                                     const connected_clusters_type &clusters,
                                     size_t cid,
                                     db::Net *net,
                                     std::map<db::InstElement, db::SubCircuit *> &subcircuits,
                                     const std::map<db::cell_index_type, db::Circuit *> &circuits,
                                     const std::map<db::cell_index_type, std::map<size_t, size_t> > &pins_per_cluster,
                                     double dbu);
};

}

#endif

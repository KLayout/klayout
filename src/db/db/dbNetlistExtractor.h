
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

#ifndef _HDR_dbNetlistExtractor
#define _HDR_dbNetlistExtractor

#include "dbCommon.h"
#include "dbHierNetworkProcessor.h"
#include "dbNetShape.h"
#include "tlGlobPattern.h"

#include <map>
#include <set>
#include <string>

namespace db
{

class DeepShapeStore;
class Netlist;
class Circuit;
class SubCircuit;
class Net;
class Device;
class DeviceAbstract;

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
  typedef db::hier_clusters<db::NetShape> hier_clusters_type;
  typedef db::connected_clusters<db::NetShape> connected_clusters_type;
  typedef db::local_cluster<db::NetShape> local_cluster_type;

  /**
   *  @brief NetExtractor constructor
   */
  NetlistExtractor ();

  /**
   *  @brief Sets a flag indicating whether floating circuits shall be included as subcircuits
   *  If this attribute is set to true, disconnected subcircuits (such that do not have a pin)
   *  are included per instance of a cell. Such subcircuits do not have a connection to their
   *  parent circuit but reflect the hierarchy they are present it. This is useful when the
   *  netlist is supposed to be flattened later, because then each subcircuit will render floating
   *  nets in the parent circuit. With this flag set to false, floating circuits will always appear
   *  as additional top cells.
   */
  void set_include_floating_subcircuits (bool f);

  /**
   *  @brief Gets a flag indicating whether floating circuits shall be included as subcircuits
   */
  bool include_floating_subcircuits () const
  {
    return m_include_floating_subcircuits;
  }

  /**
   *  @brief Sets the joined net names attribute
   *  This is a glob expression rendering net names where partial nets with the
   *  same name are joined even without explicit connection.
   *  The cell-less version applies to top level cells only.
   *  NOTE: this feature is not really used as must-connect nets are handled now in the LayoutToNetlist extractor.
   *  Remove this function later.
   */
  void set_joined_net_names (const std::list<tl::GlobPattern> &jnn);

  /**
   *  @brief Sets the joined net names attribute for a given cell name
   *  While the single-parameter set_joined_net_names only acts on the top cell, this
   *  version will act on the cell with the given name.
   *  NOTE: this feature is not really used as must-connect nets are handled now in the LayoutToNetlist extractor.
   *  Remove this function later.
   */
  void set_joined_net_names (const std::string &cell_name, const std::list<tl::GlobPattern> &jnn);

  /**
   *  @brief Sets the joined nets attribute
   *  This specifies a list of net names to join. Each join group is a set of names which specifies the net
   *  names that are to be connected. Multiple such groups can be specified. Each net name listed in a
   *  group implies implicit joining of the corresponding labels into one net.
   *  The cell-less version applies to top level cells only.
   *  NOTE: this feature is not really used as must-connect nets are handled now in the LayoutToNetlist extractor.
   *  Remove this function later.
   */
  void set_joined_nets (const std::list<std::set<std::string> > &jnn);

  /**
   *  @brief Sets the joined nets attribute per cell
   *  NOTE: this feature is not really used as must-connect nets are handled now in the LayoutToNetlist extractor.
   *  Remove this function later.
   */
  void set_joined_nets (const std::string &cell_name, const std::list<std::set<std::string> > &jnn);

  /**
   *  @brief Extract the nets
   *  See the class description for more details.
   */
  void extract_nets (const db::DeepShapeStore &dss, unsigned int layout_index, const db::Connectivity &conn, db::Netlist &nl, hier_clusters_type &clusters);

private:
  hier_clusters_type *mp_clusters;
  const db::Layout *mp_layout;
  const db::Cell *mp_cell;
  std::pair<bool, db::property_names_id_type> m_text_annot_name_id;
  std::pair<bool, db::property_names_id_type> m_device_annot_name_id;
  std::pair<bool, db::property_names_id_type> m_terminal_annot_name_id;
  std::list<tl::GlobPattern> m_joined_net_names;
  std::list<std::pair<std::string, std::list<tl::GlobPattern> > > m_joined_net_names_per_cell;
  std::list<std::set<std::string> > m_joined_nets;
  std::list<std::pair<std::string, std::list<std::set<std::string> > > > m_joined_nets_per_cell;
  bool m_include_floating_subcircuits;

  bool instance_is_device (db::properties_id_type prop_id) const;
  db::Device *device_from_instance (db::properties_id_type prop_id, db::Circuit *circuit) const;
  void assign_net_names (db::Net *net, const std::set<std::string> &net_names);

  /**
   *  @brief Make a pin connection from clusters
   *
   *  Non-root clusters make a pin. This function creates the pin inside the given circuit. It
   *  returns the new pin's ID.
   */
  size_t make_pin (db::Circuit *circuit, db::Net *net);

  /**
   *  @brief Makes a subcircuit for the given instance (by cell index and transformation)
   *  This method maintains a subcircuit cache in "subcircuits" and will pull the subcircuit from there
   *  if possible.
   *  It returns the new or old subcircuit.
   */
  db::SubCircuit *make_subcircuit (Circuit *circuit,
                                   db::cell_index_type inst_cell_index,
                                   const db::ICplxTrans &inst_trans,
                                   std::map<std::pair<db::cell_index_type, db::ICplxTrans>, db::SubCircuit *> &subcircuits,
                                   const std::map<db::cell_index_type, db::Circuit *> &circuits);

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
   *  Updates:
   *   - subcircuits: An cell instance to SubCircuit lookup table
   */
  void make_and_connect_subcircuits (db::Circuit *circuit,
                                     const connected_clusters_type &clusters,
                                     size_t cid,
                                     db::Net *net,
                                     std::map<std::pair<cell_index_type, ICplxTrans>, SubCircuit *> &subcircuits,
                                     const std::map<db::cell_index_type, db::Circuit *> &circuits,
                                     const std::map<db::cell_index_type, std::map<size_t, size_t> > &pins_per_cluster);

  /**
   *  @brief Connects the devices
   *
   *  Devices are identified by special cells. These carry a property with the
   *  device class name. Inside these cells, the terminals are identified by special clusters.
   *  The terminal IDs are coded on these clusters are a property.
   */
  void connect_devices (db::Circuit *circuit,
                        const connected_clusters_type &clusters,
                        size_t cid,
                        db::Net *net);

  /**
   *  @brief Attaches net names from text properties
   *
   *  Texts (labels) are represented by special shapes. The texts are kept as properties.
   *  This method will collect all these labels and attach them to the nets as (alternative)
   *  names.
   */
  void collect_labels (const connected_clusters_type &clusters,
                       size_t cid,
                       std::set<std::string> &net_names);

  /**
   *  @brief Makes the terminal to cluster ID connections of the device abstract
   */
  void make_device_abstract_connections (db::DeviceAbstract *dm, const connected_clusters_type &clusters);

};

}

#endif

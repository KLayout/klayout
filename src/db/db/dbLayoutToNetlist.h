
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

#ifndef _HDR_dbLayout2Netlist
#define _HDR_dbLayout2Netlist

#include "dbCommon.h"
#include "dbCellMapping.h"
#include "dbNetlistExtractor.h"
#include "dbNetlistDeviceExtractor.h"

namespace db
{

/**
 *  @brief A generic framework for extracting netlists from layouts
 *
 *  This class wraps various concepts from db::NetlistExtractor and db::NetlistDeviceExtractor
 *  and more. It is supposed to provide a framework for extracting a netlist from a layout.
 *
 *  The use model of this class consists of five steps which need to be executed in this order.
 *
 *  @ul
 *  @li Configuration: in this step, the LayoutToNetlist object is created and
 *      if required, configured. Methods to be used in this step are "set_threads",
 *      "set_area_ratio" or "set_max_vertex_count". The constructor for the LayoutToNetlist
 *      object receives a db::RecursiveShapeIterator object which basically supplies the
 *      hierarchy and the layout taken as input.
 *  @/li
 *  @li Preparation
 *      In this step, the device recognitions and extraction layers are drawn from
 *      the framework. Derived can now be computed using boolean operations.
 *      Methods to use in this step are "make_layer" and it's variants.
 *      Layer preparation is not necessarily required to happen before all
 *      other steps. Layers can be computed shortly before they are required.
 *  @/li
 *  @li Following the preparation, the devices can be extracted using "extract_devices".
 *      This method needs to be called for each device extractor required. Each time,
 *      a device extractor needs to be given plus a map of device layers. The device
 *      layers are device extractor specific. Either original or derived layers
 *      may be specified here. Layer preparation may happen between calls to "extract_devices".
 *  @/li
 *  @li Once the devices are derived, the netlist connectivity can be defined and the
 *      netlist extracted. The connectivity is defined with "connect" and it's
 *      flavours. The actual netlist extraction happens with "extract_netlist".
 *  @/li
 *  @li After netlist extraction, the information is ready to be retrieved.
 *      The produced netlist is available with "netlist". The Shapes of a
 *      specific net are available with "shapes_of_net". "probe_net" allows
 *      finding a net by probing a specific location.
 *  @li
 */
class DB_PUBLIC LayoutToNetlist
  : public gsi::ObjectBase, public tl::Object
{
public:
  typedef std::map<unsigned int, std::string>::const_iterator layer_iterator;

  /**
   *  @brief The constructor
   *
   *  See the class description for details.
   */
  LayoutToNetlist (const db::RecursiveShapeIterator &iter);

  /**
   *  @brief Alternative constructor using an external deep shape storage
   *
   *  This constructor allows using an external DSS. It's intended to be used
   *  with existing DSS instances. Existing layers can be registered with
   *  "register_layer". The LayoutToNetlist object will hold a weak reference
   *  to the DSS but not own the DSS.
   *
   *  NOTE: if using make_layer, these new layers will be created in the DSS
   *  given in this constructor.
   */
  LayoutToNetlist (db::DeepShapeStore *dss, unsigned int layout_index = 0);

  /**
   *  @brief Alternative constructor for flat mode
   *
   *  In flat mode, the internal DSS will be initialized to a top-level only
   *  layout. All layers entered through "register_layer" or created with
   *  "make_layer" will become flat ones.
   */
  LayoutToNetlist (const std::string &topcell_name, double dbu);

  /**
   *  @brief The default constructor
   */
  LayoutToNetlist ();

  /**
   *  @brief The destructor
   */
  ~LayoutToNetlist ();

  /**
   *  @brief Sets the number of threads to use for operations which support multiple threads
   */
  void set_threads (int n);

  /**
   *  @brief Gets the number of threads to use
   */
  int threads () const;

  /**
   *  @brief Sets the area_ratio parameter for the hierarchical network processor
   *  This parameter controls splitting of large polygons in order to reduce the
   *  error made by the bounding box approximation.
   */
  void set_area_ratio (double ar);

  /**
   *  @brief Gets the area ratio
   */
  double area_ratio () const;

  /**
   *  @brief Sets the max_vertex_count parameter for the hierarchical network processor
   *  This parameter controls splitting of large polygons in order to enhance performance
   *  for very big polygons.
   */
  void set_max_vertex_count (size_t n);

  /**
   *  @brief Gets the max vertex count
   */
  size_t max_vertex_count () const;

  /**
   *  @brief Register a layer under the given name
   *  This is a formal name for the layer. Using a name or layer properties
   *  (see below) enhances readability of backannotated information
   *  if layers are involved. Use this method to attach a name to a region
   *  derived by boolean operations for example.
   *  Named regions are persisted inside the LayoutToNetlist object. Only
   *  named regions can be put into "connect".
   */
  void register_layer (const db::Region &region, const std::string &name);

  /**
   *  @brief Gets the name of the given region
   *  Returns an empty string if the region does not have a name.
   */
  std::string name (const db::Region &region) const;

  /**
   *  @brief Gets the name of the given layer by index
   *  Returns an empty string if the layer does not have a name.
   */
  std::string name (unsigned int) const;

  /**
   *  @brief Returns true, if the region is a persisted region
   *  Persisted regions have a name and are kept inside the LayoutToNetlist
   *  object.
   */
  bool is_persisted (const db::Region &region) const;

  /**
   *  @brief Gets the region (layer) with the given name
   *  If the name is not valid, this method returns 0. Otherwise it
   *  will return a new'd Region object referencing the layer with
   *  the given name. It must be deleted by the caller.
   */
  db::Region *layer_by_name (const std::string &name);

  /**
   *  @brief Gets the region (layer) by index
   *  If the index is not valid, this method returns 0. Otherwise it
   *  will return a new'd Region object referencing the layer with
   *  the given name. It must be deleted by the caller.
   *  Only named layers are managed by LayoutToNetlist and can
   *  be retrieved with this method.
   */
  db::Region *layer_by_index (unsigned int index);

  /**
   *  @brief Iterates over the layer indexes and names managed by this object (begin)
   */
  layer_iterator begin_layers () const
  {
    return m_name_of_layer.begin ();
  }

  /**
   *  @brief Iterates over the layer indexes and names managed by this object (end)
   */
  layer_iterator end_layers () const
  {
    return m_name_of_layer.end ();
  }

  /**
   *  @brief Creates a new empty region
   *  This method returns a new'd object which must be deleted by the caller.
   */
  db::Region *make_layer (const std::string &name = std::string ());

  /**
   *  @brief Creates a new region representing an original layer
   *  "layer_index" is the layer index of the desired layer in the original layout.
   *  This variant produces polygons and takes texts for net name annotation.
   *  A variant not taking texts is "make_polygon_layer". A Variant only taking
   *  texts is "make_text_layer".
   *  All variants return a new'd object which must be deleted by the caller.
   *  Named regions are considered "precious". The LayoutToNetlist object will
   *  keep a reference on all named layers, so they persist during the lifetime
   *  of the LayoutToNetlist object.
   *  Only named layers can be used for connect (see below).
   */
  db::Region *make_layer (unsigned int layer_index, const std::string &name = std::string ());

  /**
   *  @brief Creates a new region representing an original layer taking texts only
   *  See "make_layer" for details.
   */
  db::Region *make_text_layer (unsigned int layer_index, const std::string &name = std::string ());

  /**
   *  @brief Creates a new region representing an original layer taking polygons and texts
   *  See "make_layer" for details.
   */
  db::Region *make_polygon_layer (unsigned int layer_index, const std::string &name = std::string ());

  /**
   *  @brief Extracts devices
   *  See the class description for more details.
   *  This method will run device extraction for the given extractor. The layer map is specific
   *  for the extractor and uses the region objects derived with "make_layer" and it's variants.
   *
   *  In addition, derived regions can be passed too. Certain limitations apply. It's safe to use
   *  boolean operations for deriving layers. Other operations are applicable as long as they are
   *  capable of delivering hierarchical layers.
   *
   *  If errors occur, the device extractor will contain theses errors.
   */
  void extract_devices (db::NetlistDeviceExtractor &extractor, const std::map<std::string, db::Region *> &layers);

  /**
   *  @brief Defines an intra-layer connection for the given layer.
   *  The layer is either an original layer created with "make_layer" and it's variants or
   *  a derived layer. Certain limitations apply. It's safe to use
   *  boolean operations for deriving layers. Other operations are applicable as long as they are
   *  capable of delivering hierarchical layers.
   *  Regions put into "connect" need to be named.
   */
  void connect (const db::Region &l);

  /**
   *  @brief Defines an inter-layer connection for the given layers.
   *  The conditions mentioned with intra-layer "connect" apply for this method too.
   *  Regions put into "connect" need to be named.
   */
  void connect (const db::Region &a, const db::Region &b);

  /**
   *  @brief Connects the given layer with a global net with the given name
   *  Returns the global net ID
   *  Regions put into "connect" need to be named.
   */
  size_t connect_global (const db::Region &l, const std::string &gn);

  /**
   *  @brief Gets the global net name for a given global net ID
   */
  const std::string &global_net_name (size_t id) const;

  /**
   *  @brief Gets the global net ID for a given name name
   */
  size_t global_net_id (const std::string &name);

  /**
   *  @brief Runs the netlist extraction
   *  See the class description for more details.
   */
  void extract_netlist (const std::string &joined_net_names = std::string ());

  /**
   *  @brief Marks the netlist as extracted
   *  NOTE: this method is provided for special cases such as netlist readers. Don't
   *  use it.
   */
  void set_netlist_extracted ();

  /**
   *  @brief Gets the internal DeepShapeStore object
   *
   *  This method is intended for special cases, i.e. for the master
   *  LayoutToNetlist object in the DRC environment. The DSS provided
   *  for DRC needs to be initialized properly for text representation.
   */
  db::DeepShapeStore &dss ()
  {
    tl_assert (mp_dss.get () != 0);
    return *mp_dss;
  }

  /**
   *  @brief Gets the internal DeepShapeStore object (const version)
   *
   *  See the non-const version for details.
   */
  const db::DeepShapeStore &dss () const
  {
    tl_assert (mp_dss.get () != 0);
    return *mp_dss;
  }

  /**
   *  @brief Gets the internal layout
   */
  const db::Layout *internal_layout () const;

  /**
   *  @brief Gets the internal top cell
   */
  const db::Cell *internal_top_cell () const;

  /**
   *  @brief Gets the internal layout (non-const version)
   */
  db::Layout *internal_layout ();

  /**
   *  @brief Gets the internal top cell (non-const version)
   */
  db::Cell *internal_top_cell ();

  /**
   *  @brief Gets the connectivity object
   */
  const db::Connectivity &connectivity () const
  {
    return m_conn;
  }

  /**
   *  @brief Gets the internal layer for a given extraction layer
   *  This method is required to derive the internal layer index - for example for
   *  investigating the cluster tree.
   */
  unsigned int layer_of (const db::Region &region) const;

  /**
   *  @brief Creates a cell mapping for copying shapes from the internal layout to the given target layout.
   *  If 'with_device_cells' is true, cells will be produced for devices. These are cells not corresponding to circuits, so they are disabled normally.
   *  Use this option, if you want to access device terminal shapes per device.
   *  CAUTION: This function may create new cells in "layout".
   */
  db::CellMapping cell_mapping_into (db::Layout &layout, db::Cell &cell, bool with_device_cells = false);

  /**
   *  @brief Creates a cell mapping for copying shapes from the internal layout to the given target layout.
   *  This version will not create new cells in the target layout.
   *  If the required cells do not exist there yet, flatting will happen.
   */
  db::CellMapping const_cell_mapping_into (const db::Layout &layout, const db::Cell &cell);

  /**
   *  @brief gets the netlist extracted (0 if no extraction happened yet)
   */
  db::Netlist *netlist () const;

  /**
   *  @brief gets the netlist extracted or make on if none exists yet.
   *  NOTE: this method is provided for special cases like readers of persisted
   *  layout to netlist data.
   */
  db::Netlist *make_netlist ();

  /**
   *  @brief Gets the hierarchical shape clusters derived in the net extraction.
   *  NOTE: the layer and cell indexes used inside this structure refer to the
   *  internal layout.
   */
  const db::hier_clusters<db::PolygonRef> &net_clusters () const
  {
    return m_net_clusters;
  }

  /**
   *  @brief Gets the hierarchical shape clusters derived in the net extraction (non-conver version)
   */
  db::hier_clusters<db::PolygonRef> &net_clusters ()
  {
    return m_net_clusters;
  }

  /**
   *  @brief Returns all shapes of a specific net and layer.
   *
   *  If "recursive" is true, the returned region will contain the shapes of
   *  all subcircuits too.
   *
   *  This methods returns a new'd Region. It's the responsibility of the caller
   *  to delete this object.
   */
  db::Region *shapes_of_net (const db::Net &net, const db::Region &of_layer, bool recursive) const;

  /**
   *  @brief Delivers all shapes of a specific net and layer to the given Shapes container.
   *
   *  If "recursive" is true, the returned region will contain the shapes of
   *  all subcircuits too.
   *
   *  This methods returns a new'd Region. It's the responsibility of the caller
   *  to delete this object.
   */
  void shapes_of_net (const db::Net &net, const db::Region &of_layer, bool recursive, db::Shapes &to) const;

  /**
   *  @brief Builds a net representation in the given layout and cell
   *
   *  This method has two modes: recursive and top-level mode. In recursive mode,
   *  it will create a proper hierarchy below the given target cell to hold all subcircuits the
   *  net connects to. It will copy the net's parts from this subcircuits into these cells.
   *
   *  In top-level mode, only the shapes from the net inside it's circuit are copied to
   *  the given target cell. No other cells are created.
   *
   *  Recursive mode is picked when a cell name prefix is given. The new cells will be
   *  named like cell_name_prefix + circuit name.
   *
   *  If a device cell name prefix is given, cells will be produced for each device abstract
   *  using a name like device_cell_name_prefix + device name.
   *
   *  @param target The target layout
   *  @param target_cell The target cell
   *  @param lmap Target layer indexes (keys) and net regions (values)
   *  @param cell_name_prefix Chooses recursive mode if non-null
   *  @param device_cell_name_prefix See above
   */
  void build_net (const db::Net &net, db::Layout &target, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const char *cell_name_prefix, const char *device_cell_name_prefix) const;

  /**
   *  @brief Builds a full hierarchical representation of the nets
   *
   *  This method copies all nets into cells corresponding to the circuits. It uses the cmap
   *  object to determine the target cell (create them with "cell_mapping_into" or "const_cell_mapping_into").
   *  If no mapping is requested, the specific circuit it skipped.
   *
   *  The method has two net annotation modes:
   *   * No annotation (net_cell_name_prefix == 0): the shapes will be put into the target cell simply
   *   * Individual subcells per net (net_cell_name_prefix != 0): for each net, a subcell is created
   *     and the net shapes will be put there (name of the subcell = net_cell_name_prefix + net name).
   *
   *  In addition, net hierarchy is covered in two ways:
   *   * No connection indicated (circuit_cell_name_prefix == 0: the net shapes are simply put into their
   *     respective circuits. The connections are not indicated.
   *   * Subnet hierarchy (circuit_cell_name_prefix != 0): for each root net, a full hierarchy is built
   *     to accommodate the subnets (see build_net in recursive mode).
   *
   *  If a device cell name prefix is given, cells will be produced for each device abstract
   *  using a name like device_cell_name_prefix + device name.
   *
   *  @param cmap The mapping of internal layout to target layout for the circuit mapping
   *  @param target The target layout
   *  @param lmap Target layer indexes (keys) and net regions (values)
   *  @param circuit_cell_name_prefix See method description
   *  @param net_cell_name_prefix See method description
   *  @param device_cell_name_prefix See above
   */
  void build_all_nets (const db::CellMapping &cmap, db::Layout &target, const std::map<unsigned int, const db::Region *> &lmap, const char *net_cell_name_prefix, const char *circuit_cell_name_prefix, const char *device_cell_name_prefix) const;

  /**
   *  @brief Finds the net by probing a specific location on the given layer
   *
   *  This method will find a net looking at the given layer at the specific position.
   *  It will traverse the hierarchy below if no shape in the requested layer is found
   *  in the specified location.
   *
   *  If no net is found at all, 0 is returned.
   *
   *  This variant accepts a micrometer-unit location. The location is given in the
   *  coordinate space of the initial cell.
   */
  db::Net *probe_net (const db::Region &of_region, const db::DPoint &point);

  /**
   *  @brief Finds the net by probing a specific location on the given layer
   *  See the description of the other "probe_net" variant.
   *  This variant accepts a database-unit location. The location is given in the
   *  coordinate space of the initial cell.
   */
  db::Net *probe_net (const db::Region &of_region, const db::Point &point);

  /**
   *  @brief Runs an antenna check on the extracted clusters
   *
   *  The antenna check will traverse all clusters and run an antenna check
   *  for all root clusters. The antenna ratio is defined by the total
   *  area of all "metal" shapes divided by the total area of all "gate" shapes
   *  on the cluster. Of all clusters where the antenna ratio is larger than
   *  the limit ratio all metal shapes are copied to the output region as
   *  error markers.
   *
   *  The limit ratio can be modified by the presence of connections to
   *  other layers (specifically designating diodes for charge removal).
   *  Each of these layers will modify the ratio by adding a value of
   *  A(diode) / Ared[um^2] to the ratio. A(diode) is the area of the
   *  diode layer per cluster. Both the diode layer and the Ared value
   *  are specified as pairs in "diodes".
   *
   *  A special case is Ared = 0: in this case, the presence of any shapes
   *  on the diode layer will entirely disable the check on a cluster,
   *  regardless of the diode's area.
   *  In other words: any diode will make the net safe against antenna discharge.
   */
  db::Region antenna_check (const db::Region &gate, const db::Region &metal, double ratio, const std::vector<std::pair<const db::Region *, double> > &diodes = std::vector<std::pair<const db::Region *, double> > ());

private:
  //  no copying
  LayoutToNetlist (const db::LayoutToNetlist &other);
  LayoutToNetlist &operator= (const db::LayoutToNetlist &other);

  db::RecursiveShapeIterator m_iter;
  std::auto_ptr<db::DeepShapeStore> mp_internal_dss;
  tl::weak_ptr<db::DeepShapeStore> mp_dss;
  unsigned int m_layout_index;
  db::Connectivity m_conn;
  db::hier_clusters<db::PolygonRef> m_net_clusters;
  std::auto_ptr<db::Netlist> mp_netlist;
  std::set<db::DeepLayer> m_dlrefs;
  std::map<std::string, db::DeepLayer> m_named_regions;
  std::map<unsigned int, std::string> m_name_of_layer;
  bool m_netlist_extracted;
  bool m_is_flat;
  db::DeepLayer m_dummy_layer;

  void init ();
  size_t search_net (const db::ICplxTrans &trans, const db::Cell *cell, const db::local_cluster<db::PolygonRef> &test_cluster, std::vector<db::InstElement> &rev_inst_path);
  void build_net_rec (const db::Net &net, db::Layout &target, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const char *net_cell_name_prefix, const char *cell_name_prefix, const char *device_cell_name_prefix, std::map<std::pair<db::cell_index_type, size_t>, db::cell_index_type> &cmap, const ICplxTrans &tr) const;
  void build_net_rec (db::cell_index_type ci, size_t cid, db::Layout &target, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const Net *net, const char *net_cell_name_prefix, const char *cell_name_prefix, const char *device_cell_name_prefix, std::map<std::pair<db::cell_index_type, size_t>, db::cell_index_type> &cmap, const ICplxTrans &tr) const;
  db::DeepLayer deep_layer_of (const db::Region &region) const;
  void ensure_layout () const;
};

}

namespace tl
{

template<> struct type_traits<db::LayoutToNetlist> : public tl::type_traits<void>
{
  //  mark "NetlistDeviceExtractor" as not having a default ctor and no copy ctor
  typedef tl::false_tag has_copy_constructor;
  typedef tl::false_tag has_default_constructor;
};

}

#endif

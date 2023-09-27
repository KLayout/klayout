
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

#ifndef _HDR_dbLayoutToNetlist
#define _HDR_dbLayoutToNetlist

#include "dbCommon.h"
#include "dbCellMapping.h"
#include "dbNetlistExtractor.h"
#include "dbNetlistDeviceExtractor.h"
#include "dbLayoutToNetlistEnums.h"
#include "dbLog.h"
#include "tlGlobPattern.h"

namespace db
{

class NetlistBuilder;

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
  : public gsi::ObjectBase, public db::NetlistManipulationCallbacks
{
public:
  typedef std::map<unsigned int, std::string>::const_iterator layer_iterator;
  typedef std::vector<db::LogEntryData> log_entries_type;

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
   *  @brief Makes the extractor take over ownership over the DSS when it was created with an external DSS
   */
  void keep_dss ();

  /**
   *  @brief Gets the database description
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Sets the database description
   */
  void set_description (const std::string &description)
  {
    m_description = description;
  }

  /**
   *  @brief Gets the original file
   *
   *  The original file describes what original file the netlist database
   *  was derived from.
   */
  const std::string &original_file () const
  {
    return m_original_file;
  }

  /**
   *  @brief Sets the database original file
   */
  void set_original_file (const std::string &original_file)
  {
    m_original_file = original_file;
  }

  /**
   *  @brief Gets the database name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets the database name
   */
  void set_name (const std::string &name)
  {
    m_name = name;
  }

  /**
   *  @brief Gets the file name
   */
  const std::string &filename () const
  {
    return m_filename;
  }

  /**
   *  @brief Sets the file name
   */
  void set_filename (const std::string &filename)
  {
    m_filename = filename;
  }

  /**
   *  @brief Gets the top level mode flag
   */
  bool top_level_mode () const
  {
    return m_top_level_mode;
  }

  /**
   *  @brief Sets top level mode
   *
   *  In top level mode, must-connect warnings are turned into
   *  errors for example.
   *
   *  By default, top-level mode is off.
   */
  void set_top_level_mode (bool f)
  {
    m_top_level_mode = f;
  }

  /**
   *  @brief Gets the log entries
   */
  const log_entries_type &log_entries () const
  {
    return m_log_entries;
  }

  /**
   *  @brief Iterator for the log entries (begin)
   */
  log_entries_type::const_iterator begin_log_entries () const
  {
    return m_log_entries.begin ();
  }

  /**
   *  @brief Iterator for the log entries (end)
   */
  log_entries_type::const_iterator end_log_entries () const
  {
    return m_log_entries.end ();
  }

  /**
   *  @brief Clears the log entries
   */
  void clear_log_entries ()
  {
    m_log_entries.clear ();
  }

  /**
   *  @brief Adds a log entry
   */
  void log_entry (const db::LogEntryData &log_entry)
  {
    if (m_log_entries.empty () || m_log_entries.back () != log_entry) {
      m_log_entries.push_back (log_entry);
    }
  }

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
   *  @brief Sets the device scaling factor
   *  This factor will scale the physical properties of the extracted devices
   *  accordingly. The scale factor applies an isotropic shrink (<1) or expansion (>1).
   */
  void set_device_scaling (double s);

  /**
   *  @brief Gets the device scaling factor
   */
  double device_scaling () const;

  /**
   *  @brief Register a layer, optionally under the given name
   *  Using a name or layer properties (see below) enhances readability of backannotated information
   *  if layers are involved. Use this method to attach a name to a region
   *  derived by boolean operations for example.
   *
   *  Registered regions are persisted inside the LayoutToNetlist object
   *  if they are flat or original layer regions.
   *  This allows passing flat or original layer collections.
   *
   *  If no name is given, the region will not be registered under a name.
   *  Still the collection will be persisted if required.
   *
   *  In addition to regions, text collections can be registered too.
   *  Including texts in "connect" makes net names begin assigned from the text strings.
   */
  void register_layer (const ShapeCollection &collection, const std::string &name = std::string ());

  /**
   *  @brief Gets the name of the given collection
   *  Returns an empty string if the collection does not have a name.
   */
  std::string name (const ShapeCollection &coll) const;

  /**
   *  @brief Gets the name of the given layer by index
   *  Returns an empty string if the layer does not have a name.
   */
  std::string name (unsigned int) const;

  /**
   *  @brief Sets the generator string
   */
  void set_generator (const std::string &g);

  /**
   *  @brief Gets the generator string
   */
  const std::string &generator () const
  {
    return m_generator;
  }

  /**
   *  @brief Returns true, if the region is a persisted region
   *  Persisted regions have a name and are kept inside the LayoutToNetlist
   *  object.
   */
  template <class Collection>
  bool is_persisted (const Collection &coll) const
  {
    return is_persisted_impl (coll);
  }

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
   *  @brief Gets the internal layer from the original layer
   */
  db::Region *layer_by_original (const ShapeCollection &original_layer)
  {
    return layer_by_original (original_layer.get_delegate ());
  }

  /**
   *  @brief Gets the layer from the original layer's delegate
   *  Returns 0 if the original layer was not registered as an input_layer.
   */
  db::Region *layer_by_original (const ShapeCollectionDelegateBase *original_delegate);

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
   *  @brief Creates a new text collection representing an original layer taking texts only
   *  See "make_layer" for details.
   */
  db::Texts *make_text_layer (unsigned int layer_index, const std::string &name = std::string ());

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
   *  In addition, derived regions/text collections can be passed too. Certain limitations apply. It's safe to use
   *  boolean operations for deriving layers. Other operations are applicable as long as they are
   *  capable of delivering hierarchical layers.
   *
   *  If errors occur, the device extractor will contain theses errors. They are also transferred
   *  to the LayoutToNetlist object.
   */
  void extract_devices (db::NetlistDeviceExtractor &extractor, const std::map<std::string, db::ShapeCollection *> &layers);

  /**
   *  @brief Resets the extracted netlist
   *
   *  This method will invalidate the netlist and extraction. It is called automatically when
   *  one of the connect methods is called.
   */
  void reset_extracted ();

  /**
   *  @brief Defines an intra-layer connection for the given layer.
   *  The layer is either an original layer created with "make_layer" and it's variants or
   *  a derived layer. Certain limitations apply. It's safe to use
   *  boolean operations for deriving layers. Other operations are applicable as long as they are
   *  capable of delivering hierarchical layers.
   */
  void connect (const db::Region &l);

  /**
   *  @brief Defines an inter-layer connection for the given layers.
   *  The conditions mentioned with intra-layer "connect" apply for this method too.
   */
  void connect (const db::Region &a, const db::Region &b)
  {
    connect_impl (a, b);
  }

  /**
   *  @brief Defines an inter-layer connection for the given layers.
   *  As one layer is a texts layer, this connection will basically add net labels.
   */
  void connect (const db::Region &a, const db::Texts &b)
  {
    connect_impl (a, b);
  }

  /**
   *  @brief Defines an inter-layer connection for the given layers.
   *  As one layer is a texts layer, this connection will basically add net labels.
   */
  void connect (const db::Texts &a, const db::Region &b)
  {
    connect_impl (b, a);
  }

  /**
   *  @brief Connects the given layer with a global net with the given name
   *  Returns the global net ID
   */
  size_t connect_global (const db::Region &l, const std::string &gn)
  {
    return connect_global_impl (l, gn);
  }

  /**
   *  @brief Connects the given text layer with a global net with the given name
   *  Returns the global net ID
   */
  size_t connect_global (const db::Texts &l, const std::string &gn)
  {
    return connect_global_impl (l, gn);
  }

  /**
   *  @brief Gets the global net name for a given global net ID
   */
  const std::string &global_net_name (size_t id) const;

  /**
   *  @brief Gets the global net ID for a given name name
   */
  size_t global_net_id (const std::string &name);

  /**
   *  @brief Sets a flag indicating whether to include floating subcircuits
   */
  void set_include_floating_subcircuits (bool f);

  /**
   *  @brief Sets a flag indicating whether to include floating subcircuits
   */
  bool include_floating_subcircuits () const
  {
    return m_include_floating_subcircuits;
  }

  /**
   *  @brief Clears the "join net names" settings
   */
  void clear_join_net_names ();

  /**
   *  @brief Joins net names matching the given expression
   *
   *  Using this function will *add* one more rule. To clear all registered rules, use "clear_join_net_names".
   *  These pattern will only act on top level cells.
   */
  void join_net_names (const tl::GlobPattern &gp);

  /**
   *  @brief Joins net names matching the given expression
   *
   *  Using this function will *add* one more rule specific to cells matching the first glob pattern. To clear all registered rules, use "clear_join_net_names".
   *  Pattern registered with this function will act on the given cells, regardless of whether it's top level or not.
   */
  void join_net_names (const tl::GlobPattern &cell, const tl::GlobPattern &gp);

  /**
   *  @brief Gets the joined net names for top level
   *
   *  This method is mainly provided to test purposes.
   */
  const std::list<tl::GlobPattern> &joined_net_names () const
  {
    return m_joined_net_names;
  }

  /**
   *  @brief Gets the joined net names per cell
   *
   *  This method is mainly provided to test purposes.
   */
  const std::list<std::pair<tl::GlobPattern, tl::GlobPattern> > &joined_net_names_per_cell () const
  {
    return m_joined_net_names_per_cell;
  }

  /**
   *  @brief Clears the "join nets" settings
   */
  void clear_join_nets ();

  /**
   *  @brief Joins the given nets for the top level cell
   *
   *  This method will make an explicit connection between the nets given in the name set.
   *  This applies implicit joining of different nets with the same label (intra-net joining)
   *  and of nets with different names (inter-net joining). Intra-net joining is implied always.
   */
  void join_nets (const std::set<std::string> &jn);

  /**
   *  @brief Joins the given nets for cells matching the given pattern
   *
   *  Using this function will *add* one more rule specific to cells matching the first glob pattern. To clear all registered rules, use "clear_join_nets".
   *  Pattern registered with this function will act on the given cells, regardless of whether it's top level or not.
   */
  void join_nets (const tl::GlobPattern &cell, const std::set<std::string> &gp);

  /**
   *  @brief Gets the joined nets for top level
   *
   *  This method is mainly provided to test purposes.
   */
  const std::list<std::set<std::string> > &joined_nets () const
  {
    return m_joined_nets;
  }

  /**
   *  @brief Gets the joined nets per cell
   *
   *  This method is mainly provided to test purposes.
   */
  const std::list<std::pair<tl::GlobPattern, std::set<std::string> > > &joined_nets_per_cell () const
  {
    return m_joined_nets_per_cell;
  }

  /**
   *  @brief Runs the netlist extraction
   *  See the class description for more details.
   */
  void extract_netlist ();

  /**
   *  @brief Throws an exception if the extractor contains errors
   */
  void check_extraction_errors ();

  /**
   *  @brief Marks the netlist as extracted
   *  NOTE: this method is provided for special cases such as netlist readers. Don't
   *  use it.
   */
  void set_netlist_extracted ();

  /**
   *  @brief Gets a value indicating whether the netlist has been extracted
   */
  bool is_netlist_extracted () const
  {
    return m_netlist_extracted;
  }

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
   *  @brief Returns true, if there a layout is set
   */
  bool has_internal_layout () const;

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
  template <class Collection>
  unsigned int layer_of (const Collection &coll) const
  {
    return deep_layer_of (coll).layer ();
  }

  /**
   *  @brief Creates a cell mapping for copying shapes from the internal layout to the given target layout.
   *  If 'with_device_cells' is true, cells will be produced for devices. These are cells not corresponding to circuits, so they are disabled normally.
   *  Use this option, if you want to access device terminal shapes per device.
   *  CAUTION: This function may create new cells in "layout".
   */
  db::CellMapping cell_mapping_into (db::Layout &layout, db::Cell &cell, bool with_device_cells = false);

  /**
   *  @brief Creates a cell mapping for copying shapes from the internal layout to the given target layout for a given list of nets
   *  This version will only create cells which are required to represent the given nets.
   *  If 'with_device_cells' is true, cells will be produced for devices. These are cells not corresponding to circuits, so they are disabled normally.
   *  Use this option, if you want to access device terminal shapes per device.
   *  CAUTION: This function may create new cells in "layout".
   */
  db::CellMapping cell_mapping_into (db::Layout &layout, db::Cell &cell, const std::vector<const db::Net *> &nets, bool with_device_cells = false);

  /**
   *  @brief Creates a cell mapping for copying shapes from the internal layout to the given target layout.
   *  This version will not create new cells in the target layout.
   *  If the required cells do not exist there yet, flattening will happen.
   */
  db::CellMapping const_cell_mapping_into (const db::Layout &layout, const db::Cell &cell);

  /**
   *  @brief Creates a layer mapping for build_nets etc.
   *  This method will create new layers inside the target layout corresponding to the
   *  original layers as kept inside the LayoutToNetlist database.
   *  It will return a layer mapping table suitable for use with build_all_nets, build_nets etc.
   *  Layers without original layer information will be given layer numbers ln, ln+1 etc.
   */
  std::map<unsigned int, const db::Region *> create_layermap (db::Layout &target_layout, int ln) const;

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
  const db::hier_clusters<db::NetShape> &net_clusters () const
  {
    return m_net_clusters;
  }

  /**
   *  @brief Gets the hierarchical shape clusters derived in the net extraction (non-conver version)
   */
  db::hier_clusters<db::NetShape> &net_clusters ()
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
  db::Region *shapes_of_net (const db::Net &net, const db::Region &of_layer, bool recursive, const db::ICplxTrans &trans = db::ICplxTrans ()) const;

  /**
   *  @brief Delivers all shapes of a specific net and layer to the given Shapes container.
   *
   *  If "recursive" is true, the returned region will contain the shapes of
   *  all subcircuits too.
   *
   *  This methods returns a new'd Region. It's the responsibility of the caller
   *  to delete this object.
   *
   *  propid is an optional properties ID which is attached to the shapes if not 0.
   */
  void shapes_of_net (const db::Net &net, const db::Region &of_layer, bool recursive, db::Shapes &to, properties_id_type propid = 0, const db::ICplxTrans &trans = db::ICplxTrans ()) const;

  /**
   *  @brief Builds a net representation in the given layout and cell
   *
   *  This method puts the shapes of a net into the given target cell using a variety of options
   *  to represent the net name and the hierarchy of the net.
   *
   *  If the netname_prop name is not nil, a property with the given name is created and assigned
   *  the net name.
   *
   *  Net hierarchy is covered in three ways:
   *   * No connection indicated (hier_mode == BNH_Disconnected: the net shapes are simply put into their
   *     respective circuits. The connections are not indicated.
   *   * Subnet hierarchy (hier_mode == BNH_SubcircuitCells): for each root net, a full hierarchy is built
   *     to accommodate the subnets (see build_net in recursive mode).
   *   * Flat (hier_mode == BNH_Flatten): each net is flattened and put into the circuit it
   *     belongs to.
   *
   *  If a device cell name prefix is given, cells will be produced for each device abstract
   *  using a name like device_cell_name_prefix + device name. Otherwise the device shapes are
   *  treated as part of the net.
   *
   *  @param net The net to build
   *  @param target The target layout
   *  @param target_cell The target cell
   *  @param lmap Target layer indexes (keys) and net regions (values)
   *  @param hier_mode See description of this method
   *  @param net_prop_mode How to attach properties to shapes
   *  @param netname_prop An (optional) property name to which to attach the net name
   *  @param cell_name_prefix Chooses recursive mode if non-null
   *  @param device_cell_name_prefix See above
   */
  void build_net (const db::Net &net, db::Layout &target, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, NetPropertyMode prop_mode, const tl::Variant &netname_prop, BuildNetHierarchyMode hier_mode, const char *cell_name_prefix, const char *device_cell_name_prefix) const;

  /**
   *  @brief Builds a full hierarchical representation of the nets
   *
   *  This method copies all nets into cells corresponding to the circuits. It uses the cmap
   *  object to determine the target cell (create them with "cell_mapping_into" or "const_cell_mapping_into").
   *  If no mapping is requested, the specific circuit it skipped.
   *
   *  The method has three net annotation modes:
   *   * No annotation (net_cell_name_prefix == 0 and netname_prop == nil): the shapes will be put
   *     into the target cell simply
   *   * Net name property (net_cell_name_prefix == 0 and netname_prop != nil): the shapes will be
   *     annotated with a property named with netname_prop and containing the net name string.
   *   * Individual subcells per net (net_cell_name_prefix != 0): for each net, a subcell is created
   *     and the net shapes will be put there (name of the subcell = net_cell_name_prefix + net name).
   *     (this mode can be combined with netname_prop too).
   *
   *  In addition, net hierarchy is covered in three ways:
   *   * No connection indicated (hier_mode == BNH_Disconnected: the net shapes are simply put into their
   *     respective circuits. The connections are not indicated.
   *   * Subnet hierarchy (hier_mode == BNH_SubcircuitCells): for each root net, a full hierarchy is built
   *     to accommodate the subnets (see build_net in recursive mode).
   *   * Flat (hier_mode == BNH_Flatten): each net is flattened and put into the circuit it
   *     belongs to.
   *
   *  If a device cell name prefix is given, cells will be produced for each device abstract
   *  using a name like device_cell_name_prefix + device name. Otherwise the device shapes are
   *  treated as part of the net.
   *
   *  @param cmap The mapping of internal layout to target layout for the circuit mapping
   *  @param target The target layout
   *  @param lmap Target layer indexes (keys) and net regions (values)
   *  @param net_cell_name_prefix See method description
   *  @param net_prop_mode How to attach properties to shapes
   *  @param netname_prop The property key to use for the net name or "nil" for no netname properties
   *  @param hier_mode See description of this method
   *  @param circuit_cell_name_prefix See method description
   *  @param device_cell_name_prefix See above
   */
  void build_all_nets (const db::CellMapping &cmap, db::Layout &target, const std::map<unsigned int, const db::Region *> &lmap, const char *net_cell_name_prefix, NetPropertyMode prop_mode, const tl::Variant &netname_prop, BuildNetHierarchyMode hier_mode, const char *circuit_cell_name_prefix, const char *device_cell_name_prefix) const;

  /**
   *  @brief Like build_all_nets, but with the ability to select some nets
   */
  void build_nets (const std::vector<const Net *> *nets, const db::CellMapping &cmap, db::Layout &target, const std::map<unsigned int, const db::Region *> &lmap, const char *net_cell_name_prefix, NetPropertyMode prop_mode, const tl::Variant &netname_prop, BuildNetHierarchyMode hier_mode, const char *circuit_cell_name_prefix, const char *device_cell_name_prefix) const;

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
   *
   *  The subcircuit path leading to the topmost net is stored in *sc_path_out if this
   *  pointer is non-null.
   */
  db::Net *probe_net (const db::Region &of_region, const db::DPoint &point, std::vector<SubCircuit *> *sc_path_out = 0, Circuit *initial_circuit = 0);

  /**
   *  @brief Finds the net by probing a specific location on the given layer
   *  See the description of the other "probe_net" variant.
   *  This variant accepts a database-unit location. The location is given in the
   *  coordinate space of the initial cell.
   */
  db::Net *probe_net (const db::Region &of_region, const db::Point &point, std::vector<SubCircuit *> *sc_path_out = 0, Circuit *initial_circuit = 0);

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
   *  The area computation of gate and metal happens by taking the polygon
   *  area (A) and perimeter (P) into account:
   *
   *    A(antenna) = A + P * t
   *
   *  where t is the perimeter factor. The unit of this area factor is
   *  micrometers.
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
  db::Region antenna_check (const db::Region &gate, double gate_perimeter_factor, const db::Region &metal, double metal_perimeter_factor, double ratio, const std::vector<std::pair<const db::Region *, double> > &diodes = std::vector<std::pair<const db::Region *, double> > (), db::Texts *values = 0)
  {
    return antenna_check (gate, 1.0, gate_perimeter_factor, metal, 1.0, metal_perimeter_factor, ratio, diodes, values);
  }

  /**
   *  @brief Variant of the antenna check not using the perimeter
   *  This version uses 0 for the perimeter factor hence not taking into account the perimeter at all.
   */
  db::Region antenna_check (const db::Region &gate, const db::Region &metal, double ratio, const std::vector<std::pair<const db::Region *, double> > &diodes = std::vector<std::pair<const db::Region *, double> > (), db::Texts *values = 0)
  {
    return antenna_check (gate, 1.0, 0.0, metal, 1.0, 0.0, ratio, diodes, values);
  }

  /**
   *  @brief Variant of the antenna check providing an area scale factor
   *
   *  This version provides an additional area scale factor f, so the effective area becomes
   *
   *    A(antenna) = A * f + P * t
   *
   *  where f is the area scale factor and t the perimeter scale factor. This version allows to ignore the
   *  area contribution entirely and switch to a perimeter-based antenna check by setting f to zero.
   *
   *  If values is non-null, texts explaining the violations are placed there.
   */
  db::Region antenna_check (const db::Region &gate, double gate_area_factor, double gate_perimeter_factor, const db::Region &metal, double metal_area_factor, double metal_perimeter_factor, double ratio, const std::vector<std::pair<const db::Region *, double> > &diodes = std::vector<std::pair<const db::Region *, double> > (), Texts *values = 0);

  /**
   *  @brief Saves the database to the given path
   *
   *  Currently, the internal format will be used. If "short_format" is true, the short version
   *  of the format is used.
   *
   *  This is a convenience method. The low-level functionality is the LayoutToNetlistWriter.
   */
  void save (const std::string &path, bool short_format);

  /**
   *  @brief Loads the database from the given path
   *
   *  This is a convenience method. The low-level functionality is the LayoutToNetlistReader.
   */
  void load (const std::string &path);

  /**
   *  @brief Creates a LayoutToNetlist object from a file
   *
   *  This method analyses the file and will create a LayoutToNetlist object
   *  or one of a derived class (specifically LayoutVsSchematic).
   *
   *  The returned object is new'd one and must be deleted by the caller.
   */
  static db::LayoutToNetlist *create_from_file (const std::string &path);

  /**
   *  @brief Generate memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const;

private:
  //  no copying
  LayoutToNetlist (const db::LayoutToNetlist &other);
  LayoutToNetlist &operator= (const db::LayoutToNetlist &other);

  std::string m_description;
  std::string m_name;
  std::string m_original_file;
  std::string m_filename;
  log_entries_type m_log_entries;
  db::RecursiveShapeIterator m_iter;
  std::unique_ptr<db::DeepShapeStore> mp_internal_dss;
  tl::weak_ptr<db::DeepShapeStore> mp_dss;
  unsigned int m_layout_index;
  db::Connectivity m_conn;
  db::hier_clusters<db::NetShape> m_net_clusters;
  std::unique_ptr<db::Netlist> mp_netlist;
  std::set<db::DeepLayer> m_dlrefs;
  std::map<std::string, db::DeepLayer> m_named_regions;
  std::map<unsigned int, std::string> m_name_of_layer;
  std::map<tl::id_type, db::DeepLayer> m_region_by_original;
  std::map<unsigned int, db::DeepLayer> m_region_of_layer;
  bool m_netlist_extracted;
  bool m_is_flat;
  double m_device_scaling;
  db::DeepLayer m_dummy_layer;
  std::string m_generator;
  bool m_include_floating_subcircuits;
  bool m_top_level_mode;
  std::list<tl::GlobPattern> m_joined_net_names;
  std::list<std::pair<tl::GlobPattern, tl::GlobPattern> > m_joined_net_names_per_cell;
  std::list<std::set<std::string> > m_joined_nets;
  std::list<std::pair<tl::GlobPattern, std::set<std::string> > > m_joined_nets_per_cell;

  void init ();
  void ensure_netlist ();
  size_t search_net (const db::ICplxTrans &trans, const db::Cell *cell, const db::local_cluster<NetShape> &test_cluster, std::vector<db::InstElement> &rev_inst_path);
  db::DeepLayer deep_layer_of (const ShapeCollection &coll) const;
  void ensure_layout () const;
  std::string make_new_name (const std::string &stem = std::string ());
  db::CellMapping make_cell_mapping_into (db::Layout &layout, db::Cell &cell, const std::vector<const db::Net *> *nets, bool with_device_cells);
  void connect_impl (const db::ShapeCollection &a, const db::ShapeCollection &b);
  size_t connect_global_impl (const db::ShapeCollection &l, const std::string &gn);
  bool is_persisted_impl (const db::ShapeCollection &coll) const;
  void do_join_nets (db::Circuit &c, const std::vector<Net *> &nets);
  void do_join_nets ();
  void join_nets_from_pattern (db::Circuit &c, const tl::GlobPattern &p);
  void join_nets_from_pattern (db::Circuit &c, const std::set<std::string> &p);
  void check_must_connect (const db::Circuit &c, const db::Net &a, const db::Net &b);

  //  implementation of NetlistManipulationCallbacks
  virtual size_t link_net_to_parent_circuit (const Net *subcircuit_net, Circuit *parent_circuit, const DCplxTrans &trans);
  virtual void link_nets (const db::Net *net, const db::Net *with);
};

/**
 *  @brief An object building nets (net-to-layout)
 *
 *  This object can be used to persist netlist builder information - e.g. reusing net cells when building individual
 *  layers from nets. In this case, build nets with a layer selection and call the build_net function many times.
 */

class DB_PUBLIC NetBuilder
{
public:
  /**
   *  @brief Default constructor
   */
  NetBuilder ();

  /**
   *  @brief Constructs a net builder with a target layout, a cell mapping table and a LayoutToNetlist source
   *
   *  @param target The target layout
   *  @param cmap The cell mapping from the internal layout (inside LayoutToNetlist) to the target - use LayoutInfo::cell_mapping_into to generate this map
   *  @param source The LayoutToNetlist source.
   *
   *  A cell map needs to be supplied only if intending to build many nets in hierarchical mode from multiple circuits.
   */
  NetBuilder (db::Layout *target, const db::CellMapping &cmap, const db::LayoutToNetlist *source);

  /**
   *  @brief Constructs a net builder with a source only
   *
   *  @param source The LayoutToNetlist source.
   *
   *  This net builder can be used to build single nets into dedicated target cells.
   */
  NetBuilder (db::Layout *target, const db::LayoutToNetlist *source);

  /**
   *  @brief Copy constructor
   */
  NetBuilder (const db::NetBuilder &other);

  /**
   *  @brief Move constructor
   */
  NetBuilder (db::NetBuilder &&other);

  /**
   *  @brief Assignment
   */
  NetBuilder &operator= (const db::NetBuilder &other);

  /**
   *  @brief Move
   */
  NetBuilder &operator= (db::NetBuilder &&other);

  /**
   *  @brief Sets the net-to-hierarchy generation mode
   */
  void set_hier_mode (BuildNetHierarchyMode hm)
  {
    m_hier_mode = hm;
  }

  /**
   *  @brief Sets or resets the net cell name prefix
   *
   *  Pass 0 to this string value to reset it.
   */
  void set_net_cell_name_prefix (const char *s);

  /**
   *  @brief Sets or resets the circuit cell name prefix
   *
   *  Pass 0 to this string value to reset it.
   */
  void set_cell_name_prefix (const char *s);

  /**
   *  @brief Sets or resets the device cell name prefix
   *
   *  Pass 0 to this string value to reset it.
   */
  void set_device_cell_name_prefix (const char *s);

  /**
   *  @brief See \LayoutToNetlist for details of this function
   */
  void build_net (db::Cell &target_cell, const db::Net &net, const std::map<unsigned int, const db::Region *> &lmap, NetPropertyMode prop_mode, const tl::Variant &netname_prop) const;

  /**
   *  @brief See \LayoutToNetlist for details of this function
   */
  void build_all_nets (const std::map<unsigned int, const db::Region *> &lmap, NetPropertyMode prop_mode, const tl::Variant &netname_prop) const;

  /**
   *  @brief See \LayoutToNetlist for details of this function
   */
  void build_nets (const std::vector<const Net *> *nets, const std::map<unsigned int, const db::Region *> &lmap, NetPropertyMode prop_mode, const tl::Variant &netname_prop) const;

  /**
   *  @brief A helper function to create a property ID for a given net, net property name and net property mode
   */
  static db::properties_id_type make_netname_propid (db::PropertiesRepository &pr, NetPropertyMode net_prop_mode, const tl::Variant &netname_prop, const db::Net &net, const std::string &net_name_prefix = std::string ());

private:
  struct CellReuseTableKey
  {
    CellReuseTableKey (db::cell_index_type _cell_index, db::properties_id_type _netname_propid, size_t _cluster_id)
      : cell_index (_cell_index), netname_propid (_netname_propid), cluster_id (_cluster_id)
    {
      //  .. nothing yet ..
    }

    bool operator< (const CellReuseTableKey &other) const
    {
      if (cell_index != other.cell_index) {
        return cell_index < other.cell_index;
      }
      if (netname_propid != other.netname_propid) {
        return netname_propid < other.netname_propid;
      }
      if (cluster_id != other.cluster_id) {
        return cluster_id < other.cluster_id;
      }
      return false;
    }

    db::cell_index_type cell_index;
    db::properties_id_type netname_propid;
    size_t cluster_id;
  };

  typedef std::map<CellReuseTableKey, std::pair<db::cell_index_type, bool> > cell_reuse_table_type;

  tl::weak_ptr<db::Layout> mp_target;
  db::CellMapping m_cmap;
  tl::weak_ptr<db::LayoutToNetlist> mp_source;
  mutable cell_reuse_table_type m_reuse_table;
  BuildNetHierarchyMode m_hier_mode;
  bool m_has_net_cell_name_prefix;
  std::string m_net_cell_name_prefix;
  bool m_has_cell_name_prefix;
  std::string m_cell_name_prefix;
  bool m_has_device_cell_name_prefix;
  std::string m_device_cell_name_prefix;

  void build_net_rec (const db::Net &net, cell_index_type circuit_cell, const std::map<unsigned int, const db::Region *> &lmap, const std::string &add_net_cell_name_prefix, db::properties_id_type netname_propid, const ICplxTrans &tr) const;
  void build_net_rec (const db::Net &net, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const std::string &add_net_cell_name_prefix, db::properties_id_type netname_propid, const ICplxTrans &tr) const;
  void build_net_rec (db::cell_index_type ci, size_t cid, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const Net *net, const std::string &add_net_cell_name_prefix, db::properties_id_type netname_propid, const ICplxTrans &tr) const;
  void prepare_build_nets () const;

  db::Layout &target () const
  {
    return const_cast<db::Layout &> (*mp_target);
  }
};

/**
 *  @brief Memory statistics for LayoutToNetlist
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const LayoutToNetlist &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

#endif

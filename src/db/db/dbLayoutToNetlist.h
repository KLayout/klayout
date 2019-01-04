
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
  /**
   *  @brief The constructor
   *
   *  See the class description for details.
   */
  LayoutToNetlist (const db::RecursiveShapeIterator &iter);

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
   *  @brief Creates a new region representing an original layer
   *  "layer_index" is the layer index of the desired layer in the original layout.
   *  The Region object returned is a new object and must be deleted by the caller.
   *  This variant produces polygons and takes texts for net name annotation.
   *  A variant not taking texts is "make_polygon_layer". A Variant only taking
   *  texts is "make_text_layer".
   */
  db::Region *make_layer (unsigned int layer_index);

  /**
   *  @brief Creates a new region representing an original layer taking texts only
   *  See "make_layer" for details.
   */
  db::Region *make_text_layer (unsigned int layer_index);

  /**
   *  @brief Creates a new region representing an original layer taking polygons and texts
   *  See "make_layer" for details.
   */
  db::Region *make_polygon_layer (unsigned int layer_index);

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
   */
  void connect (const db::Region &l);

  /**
   *  @brief Defines an inter-layer connection for the given layers.
   *  The conditions mentioned with intra-layer "connect" apply for this method too.
   */
  void connect (const db::Region &a, const db::Region &b);

  /**
   *  @brief Runs the netlist extraction
   *  See the class description for more details.
   */
  void extract_netlist ();

  /**
   *  @brief Gets the internal layout
   */
  const db::Layout *internal_layout () const;

  /**
   *  @brief Gets the internal top cell
   */
  const db::Cell *internal_top_cell () const;

  /**
   *  @brief Gets the internal layer for a given extraction layer
   *  This method is required to derive the internal layer index - for example for
   *  investigating the cluster tree.
   */
  unsigned int layer_of (const db::Region &region) const;

  /**
   *  @brief Creates a cell mapping for copying shapes from the internal layout to the given target layout.
   *  CAUTION: may create new cells in "layout".
   */
  db::CellMapping cell_mapping_into (db::Layout &layout, db::Cell &cell);

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
   *  @brief Gets the hierarchical shape clusters derived in the net extraction.
   *  NOTE: the layer and cell indexes used inside this structure refer to the
   *  internal layout.
   */
  const db::hier_clusters<db::PolygonRef> &net_clusters () const;

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

private:
  //  no copying
  LayoutToNetlist (const db::LayoutToNetlist &other);
  LayoutToNetlist &operator= (const db::LayoutToNetlist &other);

  db::RecursiveShapeIterator m_iter;
  db::DeepShapeStore m_dss;
  db::Connectivity m_conn;
  db::NetlistExtractor m_netex;
  std::auto_ptr<db::Netlist> mp_netlist;
  std::set<db::DeepLayer> m_dlrefs;
  bool m_netlist_extracted;

  size_t search_net (const db::ICplxTrans &trans, const db::Cell *cell, const db::local_cluster<db::PolygonRef> &test_cluster, std::vector<db::InstElement> &rev_inst_path);
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

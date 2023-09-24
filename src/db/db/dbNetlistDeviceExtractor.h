
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

#ifndef _HDR_dbNetlistDeviceExtractor
#define _HDR_dbNetlistDeviceExtractor

#include "dbCommon.h"
#include "dbNetlist.h"
#include "dbLayout.h"
#include "dbHierNetworkProcessor.h"
#include "dbDeepShapeStore.h"
#include "dbRegion.h"
#include "dbNetShape.h"
#include "dbLog.h"

#include "gsiObject.h"

namespace db
{

/**
 *  @brief Specifies a single layer from the device extractor
 */
class DB_PUBLIC NetlistDeviceExtractorLayerDefinition
{
public:
  NetlistDeviceExtractorLayerDefinition ()
    : index (0)
  {
    //  .. nothing yet ..
  }

  NetlistDeviceExtractorLayerDefinition (const std::string &_name, const std::string &_description, size_t _index, size_t _fallback_index)
    : name (_name), description (_description), index (_index), fallback_index (_fallback_index)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The formal name
   */
  std::string name;

  /**
   *  @brief The human-readable description
   */
  std::string description;

  /**
   *  @brief The index of the layer
   */
  size_t index;

  /**
   *  @brief The index of the fallback layer
   *  This is the layer to be used when this layer isn't specified for input or (more important) output
   */
  size_t fallback_index;
};

/**
 *  @brief Implements the device extraction for a specific setup
 *
 *  This class can be reimplemented to provide the basic algorithms for
 *  device extraction. See the virtual methods below.
 */
class DB_PUBLIC NetlistDeviceExtractor
  : public gsi::ObjectBase, public tl::Object
{
public:
  typedef std::list<db::LogEntryData> log_entry_list;
  typedef log_entry_list::const_iterator log_entry_iterator;
  typedef std::vector<db::NetlistDeviceExtractorLayerDefinition> layer_definitions;
  typedef layer_definitions::const_iterator layer_definitions_iterator;
  typedef std::map<std::string, db::ShapeCollection *> input_layers;
  typedef db::hier_clusters<db::NetShape> hier_clusters_type;

  /**
   *  @brief Constructor
   *
   *  The name is the name of the device class of the devices generated.
   */
  NetlistDeviceExtractor (const std::string &name);

  /**
   *  @brief Destructor
   */
  ~NetlistDeviceExtractor ();

  /**
   *  @brief Gets the property name for the device terminal annotation
   *  This name is used to attach the terminal ID to terminal shapes.
   */
  static const tl::Variant &terminal_id_property_name ();

  /**
   *  @brief Gets the property name for the device id annotation
   *  This name is used to attach the device ID to instances.
   */
  static const tl::Variant &device_id_property_name ();

  /**
   *  @brief Gets the property name for the device class annotation
   *  This name is used to attach the device class name to cells.
   */
  static const tl::Variant &device_class_property_name ();

  /**
   *  @brief Performs the extraction
   *
   *  layout and cell specify the layout and the top cell from which to perform the
   *  extraction.
   *
   *  The netlist will be filled with circuits (unless not present yet) to represent the
   *  cells from the layout.
   *
   *  Devices will be generated inside the netlist's circuits as they are extracted
   *  from the layout. Inside the layout, device terminal annotation shapes are created with the
   *  corresponding DeviceTerminalProperty objects attached. The will be used when extracting
   *  the nets later to associate nets with device terminals.
   *
   *  The definition of the input layers is device class specific.
   */
  void extract (Layout &layout, Cell &cell, const std::vector<unsigned int> &layers, Netlist *netlist, hier_clusters_type &clusters, double device_scaling = 1.0, const std::set<cell_index_type> *breakout_cells = 0);

  /**
   *  @brief Extracts the devices from a list of regions
   *
   *  This method behaves identical to the other "extract" method, but accepts
   *  named regions for input. These regions need to be of deep region type and
   *  originate from the same layout than the DeepShapeStore.
   */
  void extract (DeepShapeStore &dss, unsigned int layout_index, const input_layers &layers, Netlist &netlist, hier_clusters_type &clusters, double device_scaling = 1.0);

  /**
   *  @brief Clears the log entries
   */
  void clear_log_entries ()
  {
    m_log_entries.clear ();
  }

  /**
   *  @brief Gets the log entry iterator, begin
   */
  log_entry_iterator begin_log_entries ()
  {
    return m_log_entries.begin ();
  }

  /**
   *  @brief Gets the log entry iterator, end
   */
  log_entry_iterator end_log_entries ()
  {
    return m_log_entries.end ();
  }

  /**
   *  @brief Gets the layer definition iterator, begin
   */
  layer_definitions_iterator begin_layer_definitions () const
  {
    return m_layer_definitions.begin ();
  }

  /**
   *  @brief Gets the layer definition iterator, end
   */
  layer_definitions_iterator end_layer_definitions () const
  {
    return m_layer_definitions.end ();
  }

  /**
   *  @brief Sets the name of the device class and the device extractor
   */
  void set_name (const std::string &name)
  {
    m_name = name;
  }

  /**
   *  @brief Sets the name of the device class and the device extractor
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Sets up the extractor
   *
   *  This method is supposed to set up the device extractor. This involves two basic steps:
   *  defining the device classes and setting up the device layers.
   *
   *  Use "register_device_class" to register the device class you need.
   *
   *  The device layers need to be defined by calling "define_layer" once or several times.
   */
  virtual void setup ();

  /**
   *  @brief Gets the connectivity object used to extract the device geometry
   *  This method shall raise an error, if the input layer are not properly defined (e.g.
   *  too few etc.)
   */
  virtual db::Connectivity get_connectivity (const db::Layout &layout, const std::vector<unsigned int> &layers) const;

  /**
   *  @brief Extracts the devices from the given shape cluster
   *
   *  The shape cluster is a set of geometries belonging together in terms of the
   *  connectivity defined by "get_connectivity". The cluster might cover multiple devices,
   *  so the implementation needs to consider this case. The geometries are already merged.
   *
   *  The implementation of this method shall use "create_device" to create new
   *  devices based on the geometry found. It shall use "define_terminal" to define
   *  terminals by which the nets extracted in the network extraction step connect
   *  to the new devices.
   */
  virtual void extract_devices (const std::vector<db::Region> &layer_geometry);

  /**
   *  @brief Registers a device class
   *  The device class object will become owned by the netlist and must not be deleted by
   *  the caller. The name of the device class will be changed to the name given to
   *  the device extractor.
   *  This method shall be used inside the implementation of "setup" to register
   *  the device classes.
   */
  void register_device_class (DeviceClass *device_class);

  /**
   *  @brief Defines a layer
   *  Each call will define one more layer for the device extraction.
   *  This method shall be used inside the implementation of "setup" to define
   *  the device layers. The actual geometries are later available to "extract_devices"
   *  in the order the layers are defined.
   */
  const db::NetlistDeviceExtractorLayerDefinition &define_layer (const std::string &name, const std::string &description = std::string ());

  /**
   *  @brief Defines a layer with a fallback layer
   *  Like "define_layer" without fallback layer, but will fall back to the given layer
   *  (by index) if this layer isn't specified for input or terminal markup.
   */
  const db::NetlistDeviceExtractorLayerDefinition &define_layer (const std::string &name, size_t fallback, const std::string &description = std::string ());

  /**
   *  @brief Creates a device
   *  The device object returned can be configured by the caller, e.g. set parameters.
   *  It will be owned by the netlist and must not be deleted by the caller.
   */
  Device *create_device ();

  /**
   *  @brief Gets the device class used during extraction
   *
   *  This member is set in 'extract_devices' and holds the device class object used during extraction.
   */
  DeviceClass *device_class ()
  {
    return mp_device_class.get ();
  }

  /**
   *  @brief Defines a device terminal in the layout (a region)
   */
  void define_terminal (Device *device, size_t terminal_id, size_t layer_index, const db::Region &region);

  /**
   *  @brief Defines a device terminal in the layout (a polygon)
   */
  void define_terminal (Device *device, size_t terminal_id, size_t layer_index, const db::Polygon &polygon);

  /**
   *  @brief Defines a device terminal in the layout (a box)
   */
  void define_terminal (Device *device, size_t terminal_id, size_t layer_index, const db::Box &box);

  /**
   *  @brief Defines a point-like device terminal in the layout
   */
  void define_terminal (Device *device, size_t terminal_id, size_t layer_index, const db::Point &point);

  /**
   *  @brief Gets the database unit
   */
  double dbu () const
  {
    return mp_layout->dbu ();
  }

  /**
   *  @brief Gets the scaled database unit
   *  Use this unit to compute device properties. It is the database unit multiplied with the
   *  device scaling factor.
   */
  double sdbu () const
  {
    return m_device_scaling * mp_layout->dbu ();
  }

  /**
   *  @brief Gets the layout the shapes are taken from
   *  NOTE: this method is provided for testing purposes mainly.
   */
  db::Layout *layout ()
  {
    return mp_layout;
  }

  /**
   *  @brief Gets the layout the shapes are taken from (const version)
   *  NOTE: this method is provided for testing purposes mainly.
   */
  const db::Layout *layout () const
  {
    return mp_layout;
  }

  /**
   *  @brief Gets the cell index of the current cell
   *  NOTE: this method is provided for testing purposes mainly.
   */
  db::cell_index_type cell_index () const
  {
    return m_cell_index;
  }

  /**
   *  @brief Issues an error with the given message
   */
  void error (const std::string &msg);

  /**
   *  @brief Issues an error with the given message and error shape
   */
  void error (const std::string &msg, const db::DPolygon &poly);

  /**
   *  @brief Issues an error with the given message and error shape
   */
  void error (const std::string &msg, const db::Polygon &poly)
  {
    error (msg, poly.transformed (db::CplxTrans (dbu ())));
  }

  /**
   *  @brief Issues an error with the given category name, description and message
   */
  void error (const std::string &category_name, const std::string &category_description, const std::string &msg);

  /**
   *  @brief Issues an error with the given category name, description and message and error shape
   */
  void error (const std::string &category_name, const std::string &category_description, const std::string &msg, const db::DPolygon &poly);

  /**
   *  @brief Issues an error with the given category name, description and message and error shape
   */
  void error (const std::string &category_name, const std::string &category_description, const std::string &msg, const db::Polygon &poly)
  {
    error (category_name, category_description, msg, poly.transformed (db::CplxTrans (dbu ())));
  }

  /**
   *  @brief Issues a warning with the given message
   */
  void warn (const std::string &msg);

  /**
   *  @brief Issues a warning with the given message and warn shape
   */
  void warn (const std::string &msg, const db::DPolygon &poly);

  /**
   *  @brief Issues a warning with the given message and warn shape
   */
  void warn (const std::string &msg, const db::Polygon &poly)
  {
    warn (msg, poly.transformed (db::CplxTrans (dbu ())));
  }

  /**
   *  @brief Issues a warning with the given category name, description and message
   */
  void warn (const std::string &category_name, const std::string &category_description, const std::string &msg);

  /**
   *  @brief Issues a warning with the given category name, description and message and warn shape
   */
  void warn (const std::string &category_name, const std::string &category_description, const std::string &msg, const db::DPolygon &poly);

  /**
   *  @brief Issues a warning with the given category name, description and message and warn shape
   */
  void warn (const std::string &category_name, const std::string &category_description, const std::string &msg, const db::Polygon &poly)
  {
    warn (category_name, category_description, msg, poly.transformed (db::CplxTrans (dbu ())));
  }

  /**
   *  @brief Gets the name of the current cell
   */
  std::string cell_name () const;

  /**
   *  @brief Initializes the extractor
   *  This method will produce the device classes required for the device extraction.
   *  It is mainly provided for test purposes. Don't call it directly.
   */
  void initialize (db::Netlist *nl);

private:
  struct DeviceCellKey
  {
    DeviceCellKey () { }

    bool operator== (const DeviceCellKey &other) const
    {
      if (geometry != other.geometry) {
        return false;
      }
      if (parameters != other.parameters) {
        return false;
      }
      return true;
    }

    bool operator< (const DeviceCellKey &other) const
    {
      if (geometry != other.geometry) {
        return geometry < other.geometry;
      }
      if (parameters != other.parameters) {
        return parameters < other.parameters;
      }
      return false;
    }

    std::map<size_t, std::map<unsigned int, std::set<db::NetShape> > > geometry;
    std::map<size_t, double> parameters;
  };

  typedef std::map<unsigned int, std::vector<db::NetShape> > geometry_per_layer_type;
  typedef std::map<size_t, geometry_per_layer_type> geometry_per_terminal_type;

  tl::weak_ptr<db::Netlist> m_netlist;
  db::Layout *mp_layout;
  db::properties_id_type m_terminal_id_propname_id, m_device_id_propname_id, m_device_class_propname_id;
  hier_clusters_type *mp_clusters;
  db::cell_index_type m_cell_index;
  const std::set<db::cell_index_type> *mp_breakout_cells;
  double m_device_scaling;
  db::Circuit *mp_circuit;
  tl::weak_ptr<db::DeviceClass> mp_device_class;
  std::string m_name;
  layer_definitions m_layer_definitions;
  std::vector<unsigned int> m_layers;
  log_entry_list m_log_entries;
  std::map<size_t, std::pair<db::Device *, geometry_per_terminal_type> > m_new_devices;
  std::map<DeviceCellKey, std::pair<db::cell_index_type, db::DeviceAbstract *> > m_device_cells;

  //  no copying
  NetlistDeviceExtractor (const NetlistDeviceExtractor &);
  NetlistDeviceExtractor &operator= (const NetlistDeviceExtractor &);

  void extract_without_initialize (db::Layout &layout, db::Cell &cell, hier_clusters_type &clusters, const std::vector<unsigned int> &layers, double device_scaling, const std::set<cell_index_type> *breakout_cells);
  void push_new_devices (const Vector &disp_cache);
  void push_cached_devices (const tl::vector<Device *> &cached_devices, const db::Vector &disp_cache, const db::Vector &new_disp);
};

}

#endif

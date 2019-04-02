
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

#ifndef _HDR_dbNetlistDeviceExtractor
#define _HDR_dbNetlistDeviceExtractor

#include "dbCommon.h"
#include "dbNetlist.h"
#include "dbLayout.h"
#include "dbHierNetworkProcessor.h"
#include "dbDeepShapeStore.h"
#include "dbRegion.h"

#include "gsiObject.h"

namespace db
{

/**
 *  @brief An error object for the netlist device extractor
 *
 *  The device extractor will keep errors using objects of this kind.
 */
class DB_PUBLIC NetlistDeviceExtractorError
{
public:
  /**
   *  @brief Creates an error
   */
  NetlistDeviceExtractorError ();

  /**
   *  @brief Creates an error with a cell name and a message (the minimum information)
   */
  NetlistDeviceExtractorError (const std::string &cell_name, const std::string &msg);

  /**
   *  @brief The category name of the error
   *  Specifying the category name is optional. If a category is given, it will be used for
   *  the report.
   */
  const std::string &category_name () const
  {
    return m_category_name;
  }

  /**
   *  @brief Sets the category name
   */
  void set_category_name (const std::string &s)
  {
    m_category_name = s;
  }

  /**
   *  @brief The category description of the error
   *  Specifying the category description is optional. If a category is given, this attribute will
   *  be used for the category description.
   */
  const std::string &category_description () const
  {
    return m_category_description;
  }

  /**
   *  @brief Sets the category description
   */
  void set_category_description (const std::string &s)
  {
    m_category_description = s;
  }

  /**
   *  @brief Gets the geometry for this error
   *  Not all errors may specify a geometry. In this case, the polygon is empty.
   */
  const db::DPolygon &geometry () const
  {
    return m_geometry;
  }

  /**
   *  @brief Sets the geometry
   */
  void set_geometry (const db::DPolygon &g)
  {
    m_geometry = g;
  }

  /**
   *  @brief Gets the message for this error
   */
  const std::string &message () const
  {
    return m_message;
  }

  /**
   *  @brief Sets the message
   */
  void set_message (const std::string &n)
  {
    m_message = n;
  }

  /**
   *  @brief Gets the cell name the error occurred in
   */
  const std::string &cell_name () const
  {
    return m_cell_name;
  }

  /**
   *  @brief Sets the cell name
   */
  void set_cell_name (const std::string &n)
  {
    m_cell_name = n;
  }

  /**
   *  @brief Formats this message for printing
   */
  std::string to_string () const;

private:
  std::string m_cell_name;
  std::string m_message;
  db::DPolygon m_geometry;
  std::string m_category_name, m_category_description;
};

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

  NetlistDeviceExtractorLayerDefinition (const std::string &_name, const std::string &_description, size_t _index)
    : name (_name), description (_description), index (_index)
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
  typedef std::list<db::NetlistDeviceExtractorError> error_list;
  typedef error_list::const_iterator error_iterator;
  typedef std::vector<db::NetlistDeviceExtractorLayerDefinition> layer_definitions;
  typedef layer_definitions::const_iterator layer_definitions_iterator;
  typedef std::map<std::string, db::Region *> input_layers;
  typedef db::hier_clusters<db::PolygonRef> hier_clusters_type;

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
   *  @brief Gets the name of the extractor and the device class
   */
  const std::string &name ()
  {
    return m_name;
  }

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
   *
   *  NOTE: The extractor expects "PolygonRef" type layers.
   */
  void extract (Layout &layout, Cell &cell, const std::vector<unsigned int> &layers, Netlist *netlist, hier_clusters_type &clusters);

  /**
   *  @brief Extracts the devices from a list of regions
   *
   *  This method behaves identical to the other "extract" method, but accepts
   *  named regions for input. These regions need to be of deep region type and
   *  originate from the same layout than the DeepShapeStore.
   */
  void extract (DeepShapeStore &dss, unsigned int layout_index, const input_layers &layers, Netlist &netlist, hier_clusters_type &clusters);

  /**
   *  @brief Gets the error iterator, begin
   */
  error_iterator begin_errors ()
  {
    return m_errors.begin ();
  }

  /**
   *  @brief Gets the error iterator, end
   */
  error_iterator end_errors ()
  {
    return m_errors.end ();
  }

  /**
   *  @brief Returns true, if there are errors
   */
  bool has_errors () const
  {
    return ! m_errors.empty ();
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
   *  @brief Sets up the extractor
   *
   *  This method is supposed to set up the device extractor. This involves two basic steps:
   *  defining the device classe and setting up the device layers.
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
  void define_layer (const std::string &name, const std::string &description = std::string ());

  /**
   *  @brief Creates a device
   *  The device object returned can be configured by the caller, e.g. set parameters.
   *  It will be owned by the netlist and must not be deleted by the caller.
   */
  Device *create_device ();

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
   *  @brief Gets the name of the current cell
   */
  std::string cell_name () const;

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

    std::map<size_t, std::map<unsigned int, std::set<db::PolygonRef> > > geometry;
    std::map<size_t, double> parameters;
  };

  typedef std::map<unsigned int, std::vector<db::PolygonRef> > geometry_per_layer_type;
  typedef std::map<size_t, geometry_per_layer_type> geometry_per_terminal_type;

  tl::weak_ptr<db::Netlist> m_netlist;
  db::Layout *mp_layout;
  db::properties_id_type m_terminal_id_propname_id, m_device_id_propname_id, m_device_class_propname_id;
  hier_clusters_type *mp_clusters;
  db::cell_index_type m_cell_index;
  db::Circuit *mp_circuit;
  db::DeviceClass *mp_device_class;
  std::string m_name;
  layer_definitions m_layer_definitions;
  std::vector<unsigned int> m_layers;
  error_list m_errors;
  std::map<size_t, std::pair<db::Device *, geometry_per_terminal_type> > m_new_devices;
  std::map<DeviceCellKey, std::pair<db::cell_index_type, db::DeviceAbstract *> > m_device_cells;

  //  no copying
  NetlistDeviceExtractor (const NetlistDeviceExtractor &);
  NetlistDeviceExtractor &operator= (const NetlistDeviceExtractor &);

  /**
   *  @brief Initializes the extractor
   *  This method will produce the device classes required for the device extraction.
   */
  void initialize (db::Netlist *nl);

  void extract_without_initialize (db::Layout &layout, db::Cell &cell, hier_clusters_type &clusters, const std::vector<unsigned int> &layers);
  void push_new_devices (const Vector &disp_cache);
  void push_cached_devices (const tl::vector<Device *> &cached_devices, const db::Vector &disp_cache, const db::Vector &new_disp);
};

}

namespace tl
{

template<> struct type_traits<db::NetlistDeviceExtractor> : public tl::type_traits<void>
{
  //  mark "NetlistDeviceExtractor" as not having a default ctor and no copy ctor
  typedef tl::false_tag has_copy_constructor;
  typedef tl::false_tag has_default_constructor;
};

}

#endif

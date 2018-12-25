
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

#ifndef _HDR_dbNetlistDeviceExtractor
#define _HDR_dbNetlistDeviceExtractor

#include "dbCommon.h"
#include "dbNetlist.h"
#include "dbHierNetworkProcessor.h"
#include "dbLayout.h"

#include "gsiObject.h"

namespace db
{

/**
 *  @brief Implements the device extraction for a specific setup
 *
 *  This class can be reimplemented to provide the basic algorithms for
 *  device extraction. See the virtual methods below.
 */
class DB_PUBLIC NetlistDeviceExtractor
  : public gsi::ObjectBase
{
public:
  /**
   *  @brief Default constructor
   */
  NetlistDeviceExtractor ();

  /**
   *  @brief Destructor
   */
  ~NetlistDeviceExtractor ();

  //  TODO: Do we need to declare input layers?

  /**
   *  @brief Initializes the extractor
   *  This method will produce the device classes required for the device extraction.
   */
  void initialize (db::Netlist *nl);

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
  void extract (Layout &layout, Cell &cell, const std::vector<unsigned int> &layers);

  /**
   *  @brief Creates the device classes
   *  At least one device class needs to be defined. Use "register_device_class" to register
   *  the device classes you need. The first device class registered has device class index 0,
   *  the further ones 1, 2, etc.
   */
  virtual void create_device_classes ();

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

protected:
  /**
   *  @brief Registers a device class
   *  The device class object will become owned by the netlist and must not be deleted by
   *  the caller.
   */
  void register_device_class (DeviceClass *device_class);

  /**
   *  @brief Creates a device
   *  The device object returned can be configured by the caller, e.g. set parameters.
   *  It will be owned by the netlist and must not be deleted by the caller.
   */
  Device *create_device (unsigned int device_class_index = 0);

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

private:
  tl::weak_ptr<db::Netlist> m_netlist;
  db::Layout *mp_layout;
  db::properties_id_type m_propname_id;
  db::cell_index_type m_cell_index;
  db::Circuit *mp_circuit;
  std::vector<db::DeviceClass *> m_device_classes;
  unsigned int m_device_name_index;
};

}

#endif

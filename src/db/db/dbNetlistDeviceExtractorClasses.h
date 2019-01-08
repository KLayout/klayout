
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

#ifndef _HDR_dbNetlistDeviceExtractorClasses
#define _HDR_dbNetlistDeviceExtractorClasses

#include "dbNetlistDeviceExtractor.h"

namespace db
{

/**
 *  @brief A device extractor for a three-terminal MOS transistor
 *
 *  This class supplies the generic extractor for a MOS device.
 *  The device is defined by two basic input layers: the diffusion area
 *  (source and drain) and the gate area. It requires a third layer
 *  (poly) to put the gate terminals on. The separation between poly
 *  and gate allows separating the device recognition layer (gate) from the
 *  conductive layer.
 *
 *  The device class produced by this extractor is DeviceClassMOS3Transistor.
 *  The extractor extracts the four parameters of this class: L, W, AS and AD.
 *
 *  The diffusion area is distributed on the number of gates connecting to
 *  the particular source or drain area.
 */
class DB_PUBLIC NetlistDeviceExtractorMOS3Transistor
  : public db::NetlistDeviceExtractor
{
public:
  NetlistDeviceExtractorMOS3Transistor (const std::string &name);

  virtual void setup ();
  virtual db::Connectivity get_connectivity (const db::Layout &layout, const std::vector<unsigned int> &layers) const;
  virtual void extract_devices (const std::vector<db::Region> &layer_geometry);

protected:
  /**
   *  @brief A callback when the device is produced
   *  This callback is provided as a debugging port
   */
  virtual void device_out (const db::Device * /*device*/, const db::Region & /*diff*/, const db::Region & /*gate*/)
  {
    //  .. no specific implementation ..
  }

  /**
   *  @brief Allow derived classes to modify the device
   */
  virtual void modify_device (const db::Polygon & /*rgate*/, const std::vector<db::Region> & /*layer_geometry*/, db::Device * /*device*/)
  {
    //  .. no specific implementation ..
  }

};

/**
 *  @brief A device extractor for a four-terminal MOS transistor
 *
 *  This class is like the MOS3Transistor extractor, but requires a forth
 *  input layer (Well). This layer will be used to output the bulk terminal.
 *
 *  The device class produced by this extractor is DeviceClassMOS4Transistor.
 *  The extractor extracts the four parameters of this class: L, W, AS and AD.
 */
class DB_PUBLIC NetlistDeviceExtractorMOS4Transistor
  : public NetlistDeviceExtractorMOS3Transistor
{
public:
  NetlistDeviceExtractorMOS4Transistor (const std::string &name);

  virtual void setup ();

private:
  virtual void modify_device (const db::Polygon &rgate, const std::vector<db::Region> &layer_geometry, db::Device *device);
};

}

namespace tl
{

template<> struct type_traits<db::NetlistDeviceExtractorMOS3Transistor> : public tl::type_traits<void>
{
  typedef tl::false_tag has_copy_constructor;
  typedef tl::false_tag has_default_constructor;
};

template<> struct type_traits<db::NetlistDeviceExtractorMOS4Transistor> : public tl::type_traits<void>
{
  typedef tl::false_tag has_copy_constructor;
  typedef tl::false_tag has_default_constructor;
};

}

#endif

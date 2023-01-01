
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

#ifndef _HDR_dbNetlistDeviceExtractorClasses
#define _HDR_dbNetlistDeviceExtractorClasses

#include "dbNetlistDeviceExtractor.h"
#include "gsiObject.h"

namespace db
{

/**
 *  @brief A device class factory base class
 */
class DB_PUBLIC DeviceClassFactory
  : public gsi::ObjectBase
{
public:
  DeviceClassFactory () { }
  ~DeviceClassFactory () { }
  virtual db::DeviceClass *create_class () const = 0;
};

/**
 *  @brief A specific factory
 */
template <class C>
class DB_PUBLIC_TEMPLATE device_class_factory
  : public DeviceClassFactory
{
public:
  virtual db::DeviceClass *create_class () const { return new C (); }
};

/**
 *  @brief A base class for the specialized device extractors
 *
 *  The main feature of this class is to supply a device class factory
 *  which actually creates the device class object.
 *
 *  The NetlistDeviceExtractorImplBase object will own the factory object.
 */
class DB_PUBLIC NetlistDeviceExtractorImplBase
  : public db::NetlistDeviceExtractor
{
public:
  NetlistDeviceExtractorImplBase (const std::string &name, DeviceClassFactory *factory)
    : db::NetlistDeviceExtractor (name), mp_factory (factory)
  {
    mp_factory->keep ();
  }

  /**
   *  @brief Creates the device class object
   */
  db::DeviceClass *make_class ()
  {
    return mp_factory->create_class ();
  }

private:
  std::unique_ptr<DeviceClassFactory> mp_factory;
};

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
  : public db::NetlistDeviceExtractorImplBase
{
public:
  NetlistDeviceExtractorMOS3Transistor (const std::string &name, bool strict = false, DeviceClassFactory *factory = 0);

  virtual void setup ();
  virtual db::Connectivity get_connectivity (const db::Layout &layout, const std::vector<unsigned int> &layers) const;
  virtual void extract_devices (const std::vector<db::Region> &layer_geometry);

  bool is_strict () const
  {
    return m_strict;
  }

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

private:
  bool m_strict;
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
  NetlistDeviceExtractorMOS4Transistor (const std::string &name, bool strict = false, DeviceClassFactory *factory = 0);

  virtual void setup ();

private:
  virtual void modify_device (const db::Polygon &rgate, const std::vector<db::Region> &layer_geometry, db::Device *device);
};

/**
 *  @brief A device extractor for a two-terminal resistor
 *
 *  This class supplies the generic extractor for an resistor
 *  The resistor is defined by a "wire" with two connectors on
 *  each side.
 *
 *  The resistance is computed from the width (W) and length (L) of the
 *  wire by R = L / W * sheet_rho.
 *
 *  The device class produced by this extractor is DeviceClassResistor.
 *  The extractor extracts the three parameters of this class: R, A and P.
 *  A is the area of the wire and P is the perimeter.
 *
 *  The layers are R for the "wire" and "C" for the two contacts and the
 *  end of the wire. "tA" and "tB" are the layers on which the A and B
 *  terminals are produced.
 */
class DB_PUBLIC NetlistDeviceExtractorResistor
  : public db::NetlistDeviceExtractorImplBase
{
public:
  NetlistDeviceExtractorResistor (const std::string &name, double sheet_rho, DeviceClassFactory *factory = 0);

  virtual void setup ();
  virtual db::Connectivity get_connectivity (const db::Layout &layout, const std::vector<unsigned int> &layers) const;
  virtual void extract_devices (const std::vector<db::Region> &layer_geometry);

protected:
  /**
   *  @brief A callback when the device is produced
   *  This callback is provided as a debugging port
   */
  virtual void device_out (const db::Device * /*device*/, const db::Region & /*res*/, const db::Region & /*contacts*/)
  {
    //  .. no specific implementation ..
  }

  /**
   *  @brief Allow derived classes to modify the device
   */
  virtual void modify_device (const db::Polygon & /*res*/, const std::vector<db::Region> & /*layer_geometry*/, db::Device * /*device*/)
  {
    //  .. no specific implementation ..
  }

private:
  double m_sheet_rho;
};

/**
 *  @brief A device extractor for a two-terminal resistor with a bulk terminal
 *
 *  Extracts a resistor like NetlistDeviceExtractorResistor, but adds one more terminal
 *  for the bulk or well the resistor is embedded in.
 */
class DB_PUBLIC NetlistDeviceExtractorResistorWithBulk
  : public db::NetlistDeviceExtractorResistor
{
public:
  NetlistDeviceExtractorResistorWithBulk (const std::string &name, double sheet_rho, DeviceClassFactory *factory = 0);

  virtual void setup ();
  virtual void modify_device (const db::Polygon &res, const std::vector<db::Region> & /*layer_geometry*/, db::Device *device);
};

/**
 *  @brief A device extractor for a planar capacitor
 *
 *  This class supplies the generic extractor for a planar capacitor.
 *  The capacitor is defined by two layers whose overlap area forms
 *  the capacitor.
 *
 *  The resistance is computed from the area (A) of the overlapping region
 *  by C = A * area_cap.
 *
 *  The device class produced by this extractor is DeviceClassCapacitor.
 *  The extractor extracts the three parameters of this class: C, A and P.
 *  A is the area of the overlap area and P is the perimeter.
 *
 *  The layers are P1 and P2 for the plates. tA and tB are layers where
 *  the terminals for A and B are produced respectively.
 */
class DB_PUBLIC NetlistDeviceExtractorCapacitor
  : public db::NetlistDeviceExtractorImplBase
{
public:
  NetlistDeviceExtractorCapacitor (const std::string &name, double area_cap, DeviceClassFactory *factory = 0);

  virtual void setup ();
  virtual db::Connectivity get_connectivity (const db::Layout &layout, const std::vector<unsigned int> &layers) const;
  virtual void extract_devices (const std::vector<db::Region> &layer_geometry);

protected:
  /**
   *  @brief A callback when the device is produced
   *  This callback is provided as a debugging port
   */
  virtual void device_out (const db::Device * /*device*/, const db::Polygon & /*cap_area*/)
  {
    //  .. no specific implementation ..
  }

  /**
   *  @brief Allow derived classes to modify the device
   */
  virtual void modify_device (const db::Polygon & /*cap_area*/, const std::vector<db::Region> & /*layer_geometry*/, db::Device * /*device*/)
  {
    //  .. no specific implementation ..
  }

private:
  double m_area_cap;
};

/**
 *  @brief A device extractor for a two-terminal capacitor with a bulk terminal
 *
 *  Extracts a resistor like NetlistDeviceExtractorCapacitor, but adds one more terminal
 *  for the bulk or well the capacitor is embedded in.
 */
class DB_PUBLIC NetlistDeviceExtractorCapacitorWithBulk
  : public db::NetlistDeviceExtractorCapacitor
{
public:
  NetlistDeviceExtractorCapacitorWithBulk (const std::string &name, double cap_area, DeviceClassFactory *factory = 0);

  virtual void setup ();
  virtual void modify_device (const db::Polygon &cap, const std::vector<db::Region> & /*layer_geometry*/, db::Device *device);
};

/**
 *  @brief A device extractor for a bipolar transistor
 *
 *  This class supplies the generic extractor for a bipolar transistor device.
 *  Extraction of vertical and lateral transistors is supported through a generic geometry model:
 *
 *  The basic area is the base area. A marker shape must be provided for this area.
 *  The emitter of the transistor is defined by emitter layer shapes inside the base area.
 *  Multiple emitter shapes can be present. In this case, multiple transistor devices sharing the
 *  same base and collector are generated.
 *
 *  Finally, a collector layer can be given. If non-empty, the parts inside the base region will define
 *  the collector terminals. If empty, the collector is formed by the substrate. In this case, the base
 *  region will be output to the 'tC' terminal output layer. This layer then needs to be connected to a global net
 *  to form the net connection.
 *
 *  The device class produced by this extractor is \\DeviceClassBJT3Transistor.
 *  The extractor extracts the two parameters of this class: AE and PE.
 *
 *  The device recognition layer names are 'C' (collector), 'B' (base) and 'E' (emitter).
 *  The terminal output layer names are 'tC' (collector), 'tB' (base) and 'tE' (emitter).
 */
class DB_PUBLIC NetlistDeviceExtractorBJT3Transistor
  : public db::NetlistDeviceExtractorImplBase
{
public:
  NetlistDeviceExtractorBJT3Transistor (const std::string &name, DeviceClassFactory *factory = 0);

  virtual void setup ();
  virtual db::Connectivity get_connectivity (const db::Layout &layout, const std::vector<unsigned int> &layers) const;
  virtual void extract_devices (const std::vector<db::Region> &layer_geometry);

protected:
  /**
   *  @brief A callback when the device is produced
   *  This callback is provided as a debugging port
   */
  virtual void device_out (const db::Device * /*device*/, const db::Region & /*collector*/, const db::Region & /*base*/, const db::Polygon & /*emitter*/)
  {
    //  .. no specific implementation ..
  }

  /**
   *  @brief Allow derived classes to modify the device
   */
  virtual void modify_device (const db::Polygon & /*emitter*/, const std::vector<db::Region> & /*layer_geometry*/, db::Device * /*device*/)
  {
    //  .. no specific implementation ..
  }
};

/**
 *  @brief A device extractor for a four-terminal BJT transistor
 *
 *  This class is like the BJT3Transistor extractor, but requires a forth
 *  input layer (Substrate). This layer will be used to output the substrate terminal.
 *
 *  The device class produced by this extractor is DeviceClassBJT4Transistor.
 */
class DB_PUBLIC NetlistDeviceExtractorBJT4Transistor
  : public NetlistDeviceExtractorBJT3Transistor
{
public:
  NetlistDeviceExtractorBJT4Transistor (const std::string &name, DeviceClassFactory *factory = 0);

  virtual void setup ();

private:
  virtual void modify_device (const db::Polygon &emitter, const std::vector<db::Region> &layer_geometry, db::Device *device);
};

/**
 *  @brief A device extractor for a planar diode
 *
 *  This class supplies the generic extractor for a planar diode.
 *  The diode is defined by two layers whose overlap area forms
 *  the diode. The p-type layer forms the anode, the n-type layer
 *  the cathode.
 *
 *  The device class produced by this extractor is DeviceClassDiode.
 *  The extractor extracts the two parameters of this class: A and P.
 *  A is the area of the overlap area and P is the perimeter.
 *
 *  The layers are "P" and "N" for the p and n region respectively.
 *  The terminal output layers are "tA" and "tC" for anode and
 *  cathode respectively.
 */
class DB_PUBLIC NetlistDeviceExtractorDiode
  : public db::NetlistDeviceExtractorImplBase
{
public:
  NetlistDeviceExtractorDiode (const std::string &name, DeviceClassFactory *factory = 0);

  virtual void setup ();
  virtual db::Connectivity get_connectivity (const db::Layout &layout, const std::vector<unsigned int> &layers) const;
  virtual void extract_devices (const std::vector<db::Region> &layer_geometry);

protected:
  /**
   *  @brief A callback when the device is produced
   *  This callback is provided as a debugging port
   */
  virtual void device_out (const db::Device * /*device*/, const db::Polygon & /*diode_area*/)
  {
    //  .. no specific implementation ..
  }

  /**
   *  @brief Allow derived classes to modify the device
   */
  virtual void modify_device (const db::Polygon & /*diode_area*/, const std::vector<db::Region> & /*layer_geometry*/, db::Device * /*device*/)
  {
    //  .. no specific implementation ..
  }
};

}

#endif

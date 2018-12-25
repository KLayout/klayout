
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

#ifndef _HDR_dbNetlistDeviceClasses
#define _HDR_dbNetlistDeviceClasses

#include "dbCommon.h"
#include "dbNetlist.h"

namespace db
{

/**
 *  @brief A basic two-port device class
 */
class DB_PUBLIC DeviceClassTwoPortDevice
  : public db::DeviceClass
{
public:
  virtual bool combine_devices (Device *a, Device *b) const;

  virtual void parallel (Device *a, Device *b) const = 0;
  virtual void serial (Device *a, Device *b) const = 0;
};

/**
 *  @brief A basic resistor device class
 *  A resistor defines a single parameter, "R", which is the resistance in Ohm.
 *  It defines two ports, "A" and "B" for the two terminals.
 */
class DB_PUBLIC DeviceClassResistor
  : public db::DeviceClassTwoPortDevice
{
public:
  DeviceClassResistor ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassResistor (*this);
  }

  virtual void parallel (Device *a, Device *b) const;
  virtual void serial (Device *a, Device *b) const;
};

/**
 *  @brief A basic capacitor device class
 *  A capacitor defines a single parameter, "C", which is the capacitance in Farad.
 *  It defines two ports, "A" and "B" for the two terminals.
 */
class DB_PUBLIC DeviceClassCapacitor
  : public db::DeviceClassTwoPortDevice
{
public:
  DeviceClassCapacitor ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassCapacitor (*this);
  }

  virtual void parallel (Device *a, Device *b) const;
  virtual void serial (Device *a, Device *b) const;
};

/**
 *  @brief A basic inductor device class
 *  An inductor defines a single parameter, "L", which is the inductance in Henry.
 *  It defines two ports, "A" and "B" for the two terminals.
 */
class DB_PUBLIC DeviceClassInductor
  : public db::DeviceClassTwoPortDevice
{
public:
  DeviceClassInductor ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassInductor (*this);
  }

  virtual void parallel (Device *a, Device *b) const;
  virtual void serial (Device *a, Device *b) const;
};

/**
 *  @brief A basic diode device class
 *  A diode defines a single parameter, "A", which is the area in square micrometers (YES: micrometers, as this is the basic unit of measure
 *  in KLayout).
 *  It defines two ports, "A" and "C" for anode and cathode.
 */
class DB_PUBLIC DeviceClassDiode
  : public db::DeviceClass
{
public:
  DeviceClassDiode ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassDiode (*this);
  }

  virtual bool combine_devices (Device *a, Device *b) const;
};

/**
 *  @brief A basic MOSFET device class
 *  A MOSFET defines four parameters: "W" for the gate width in micrometers, "L" for the gate length in micrometers,
 *  "AS" for the source area and "AD" for the drain area.
 *  The MOSFET device defines three ports, "S", "D" and "G" for source, drain and gate.
 */
class DB_PUBLIC DeviceClassMOSTransistor
  : public db::DeviceClass
{
public:
  DeviceClassMOSTransistor ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassMOSTransistor (*this);
  }

  virtual bool combine_devices (Device *a, Device *b) const;
};

}

#endif


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

#ifndef _HDR_dbNetlistDeviceClasses
#define _HDR_dbNetlistDeviceClasses

#include "dbCommon.h"
#include "dbNetlist.h"

namespace db
{

/**
 *  @brief A basic two-terminal device class
 */
class DB_PUBLIC DeviceClassTwoTerminalDevice
  : public db::DeviceClass
{
public:
  virtual bool combine_devices (Device *a, Device *b) const;

  virtual void parallel (Device *a, Device *b) const = 0;
  virtual void serial (Device *a, Device *b) const = 0;
  virtual bool supports_parallel_combination () const { return true; }
  virtual bool supports_serial_combination () const { return true; }
};

/**
 *  @brief A basic resistor device class
 *  A resistor defines a single parameter, "R", which is the resistance in Ohm.
 *  It defines two terminals, "A" and "B" for the two terminals.
 */
class DB_PUBLIC DeviceClassResistor
  : public db::DeviceClassTwoTerminalDevice
{
public:
  DeviceClassResistor ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassResistor (*this);
  }

  static size_t param_id_R;

  static size_t terminal_id_A;
  static size_t terminal_id_B;

  virtual void parallel (Device *a, Device *b) const;
  virtual void serial (Device *a, Device *b) const;

  virtual size_t normalize_terminal_id (size_t) const
  {
    return terminal_id_A;
  }
};

/**
 *  @brief A basic capacitor device class
 *  A capacitor defines a single parameter, "C", which is the capacitance in Farad.
 *  It defines two terminals, "A" and "B" for the two terminals.
 */
class DB_PUBLIC DeviceClassCapacitor
  : public db::DeviceClassTwoTerminalDevice
{
public:
  DeviceClassCapacitor ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassCapacitor (*this);
  }

  static size_t param_id_C;

  static size_t terminal_id_A;
  static size_t terminal_id_B;

  virtual void parallel (Device *a, Device *b) const;
  virtual void serial (Device *a, Device *b) const;

  virtual size_t normalize_terminal_id (size_t) const
  {
    return terminal_id_A;
  }
};

/**
 *  @brief A basic inductor device class
 *  An inductor defines a single parameter, "L", which is the inductance in Henry.
 *  It defines two terminals, "A" and "B" for the two terminals.
 */
class DB_PUBLIC DeviceClassInductor
  : public db::DeviceClassTwoTerminalDevice
{
public:
  DeviceClassInductor ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassInductor (*this);
  }

  static size_t param_id_L;

  static size_t terminal_id_A;
  static size_t terminal_id_B;

  virtual void parallel (Device *a, Device *b) const;
  virtual void serial (Device *a, Device *b) const;

  virtual size_t normalize_terminal_id (size_t) const
  {
    return terminal_id_A;
  }
};

/**
 *  @brief A basic diode device class
 *  A diode defines a single parameter, "A", which is the area in square micrometers (YES: micrometers, as this is the basic unit of measure
 *  in KLayout).
 *  It defines two terminals, "A" and "C" for anode and cathode.
 */
class DB_PUBLIC DeviceClassDiode
  : public db::DeviceClass
{
public:
  DeviceClassDiode ();

  static size_t param_id_A;

  static size_t terminal_id_A;
  static size_t terminal_id_C;

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassDiode (*this);
  }

  virtual bool combine_devices (Device *a, Device *b) const;
  virtual bool supports_parallel_combination () const { return true; }
};

/**
 *  @brief A basic MOSFET device class with three terminals
 *  A MOSFET defines six parameters: "W" for the gate width in micrometers, "L" for the gate length in micrometers,
 *  "AS" for the source area and "AD" for the drain area and "PS" and "PD" for the source and drain perimeter.
 *  The MOSFET device defines three terminals, "S", "D" and "G" for source, drain and gate.
 */
class DB_PUBLIC DeviceClassMOS3Transistor
  : public db::DeviceClass
{
public:
  DeviceClassMOS3Transistor ();

  static size_t param_id_L;
  static size_t param_id_W;
  static size_t param_id_AS;
  static size_t param_id_AD;
  static size_t param_id_PS;
  static size_t param_id_PD;

  static size_t terminal_id_S;
  static size_t terminal_id_G;
  static size_t terminal_id_D;

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassMOS3Transistor (*this);
  }

  virtual bool combine_devices (Device *a, Device *b) const;
  virtual bool supports_parallel_combination () const { return true; }

  virtual size_t normalize_terminal_id (size_t tid) const
  {
    return tid == terminal_id_D ? terminal_id_S : tid;
  }

protected:
  void combine_parameters (Device *a, Device *b) const;
};

/**
 *  @brief A basic MOSFET device class with four terminals
 *  The four-terminal MOSFET behaves identical to the three-terminal one but adds one more
 *  terminal for the bulk.
 */
class DB_PUBLIC DeviceClassMOS4Transistor
  : public DeviceClassMOS3Transistor
{
public:
  DeviceClassMOS4Transistor ();

  static size_t terminal_id_B;

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassMOS4Transistor (*this);
  }

  virtual size_t normalize_terminal_id (size_t tid) const
  {
    return tid == terminal_id_D ? terminal_id_S : tid;
  }

  virtual bool combine_devices (Device *a, Device *b) const;
};

}

#endif

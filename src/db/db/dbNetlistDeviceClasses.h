
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

#ifndef _HDR_dbNetlistDeviceClasses
#define _HDR_dbNetlistDeviceClasses

#include "dbCommon.h"
#include "dbNetlist.h"

namespace db
{

/**
 *  @brief A basic resistor device class
 *  A resistor defines a single parameter, "R", which is the resistance in Ohm.
 *  It defines two terminals, "A" and "B" for the two terminals.
 */
class DB_PUBLIC DeviceClassResistor
  : public db::DeviceClass
{
public:
  DeviceClassResistor ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassResistor (*this);
  }

  static size_t param_id_R;
  static size_t param_id_L;
  static size_t param_id_W;
  static size_t param_id_A;
  static size_t param_id_P;

  static size_t terminal_id_A;
  static size_t terminal_id_B;
};

/**
 *  @brief A resistor device class with a bulk terminal (well, bulk)
 *  In addition to DeviceClassResistor, this class defines a third terminal
 *  ("W") for the bulk or well connection.
 */
class DB_PUBLIC DeviceClassResistorWithBulk
  : public db::DeviceClassResistor
{
public:
  DeviceClassResistorWithBulk ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassResistorWithBulk (*this);
  }

  static size_t terminal_id_W;
};

/**
 *  @brief A basic capacitor device class
 *  A capacitor defines a single parameter, "C", which is the capacitance in Farad.
 *  It defines two terminals, "A" and "B" for the two terminals.
 */
class DB_PUBLIC DeviceClassCapacitor
  : public db::DeviceClass
{
public:
  DeviceClassCapacitor ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassCapacitor (*this);
  }

  static size_t param_id_C;
  static size_t param_id_A;
  static size_t param_id_P;

  static size_t terminal_id_A;
  static size_t terminal_id_B;
};

/**
 *  @brief A capacitor device class with a bulk terminal (well, bulk)
 *  In addition to DeviceClassCapacitor, this class defines a third terminal
 *  ("W") for the bulk or well connection.
 */
class DB_PUBLIC DeviceClassCapacitorWithBulk
  : public db::DeviceClassCapacitor
{
public:
  DeviceClassCapacitorWithBulk ();

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassCapacitorWithBulk (*this);
  }

  static size_t terminal_id_W;
};

/**
 *  @brief A basic inductor device class
 *  An inductor defines a single parameter, "L", which is the inductance in Henry.
 *  It defines two terminals, "A" and "B" for the two terminals.
 */
class DB_PUBLIC DeviceClassInductor
  : public db::DeviceClass
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
  static size_t param_id_P;

  static size_t terminal_id_A;
  static size_t terminal_id_C;

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassDiode (*this);
  }
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

  /**
   *  @brief Implements the "split_gates" feature
   *  This feature will join internal source/drain nodes to form fingered multi-gate
   *  transistors.
   */
  void join_split_gates (db::Circuit *circuit) const;

  /**
   * @brief Returns true if device lengths are compatible
   */
  static bool lengths_are_identical (const db::Device *a, const db::Device *b);

protected:
  void combine_parameters (Device *a, Device *b) const;
  virtual bool has_bulk_pin () const;

private:
  bool is_source_terminal (size_t tid) const;
  bool is_drain_terminal (size_t tid) const;
  bool net_is_source_drain_connection (const db::Net *net) const;
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

protected:
  virtual bool has_bulk_pin () const;
};

/**
 *  @brief A bipolar transistor device class with three terminals
 *  A MOSFET defines two parameters: "AE" for the emitter area in square micrometers and "PE" for the emitter perimeter
 *  in micrometers.
 *  The bipolar transistor defines three terminals, "C", "B" and "E" for collector, base and emitter.
 */
class DB_PUBLIC DeviceClassBJT3Transistor
  : public db::DeviceClass
{
public:
  DeviceClassBJT3Transistor ();

  static size_t param_id_AE;
  static size_t param_id_PE;
  static size_t param_id_AB;
  static size_t param_id_PB;
  static size_t param_id_AC;
  static size_t param_id_PC;
  static size_t param_id_NE;

  static size_t terminal_id_C;
  static size_t terminal_id_B;
  static size_t terminal_id_E;

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassBJT3Transistor (*this);
  }
};

/**
 *  @brief A basic bipolar transistor class with four terminals
 *  The four-terminal BJT behaves identical to the three-terminal one but adds one more
 *  terminal for the substrate.
 */
class DB_PUBLIC DeviceClassBJT4Transistor
  : public DeviceClassBJT3Transistor
{
public:
  DeviceClassBJT4Transistor ();

  static size_t terminal_id_S;

  virtual db::DeviceClass *clone () const
  {
    return new DeviceClassBJT4Transistor (*this);
  }
};

}

#endif

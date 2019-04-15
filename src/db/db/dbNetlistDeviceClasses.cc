
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

#include "dbNetlistDeviceClasses.h"

namespace db
{

// ------------------------------------------------------------------------------------
//  DeviceClassTwoTerminalDevice implementation

bool DeviceClassTwoTerminalDevice::combine_devices (Device *a, Device *b) const
{
  db::Net *na1 = a->net_for_terminal (0);
  db::Net *na2 = a->net_for_terminal (1);
  db::Net *nb1 = b->net_for_terminal (0);
  db::Net *nb2 = b->net_for_terminal (1);

  bool res = true;

  if ((na1 == nb1 && na2 == nb2) || (na1 == nb2 && na2 == nb1)) {

    parallel (a, b);

  } else if ((na2 == nb1 || na2 == nb2) && na2->is_internal ()) {

    //  serial a(B) to b(A or B)
    serial (a, b);
    a->connect_terminal (1, (na2 == nb1 ? nb2 : nb1));

  } else if ((na1 == nb1 || na1 == nb2) && na1->is_internal ()) {

    //  serial a(A) to b(A or B)
    serial (a, b);
    a->connect_terminal (0, (na1 == nb1 ? nb2 : nb1));

  }

  if (res) {
    b->connect_terminal (0, 0);
    b->connect_terminal (1, 0);
    return true;
  } else {
    return false;
  }
}


// ------------------------------------------------------------------------------------
//  DeviceClassResistor implementation

DB_PUBLIC size_t DeviceClassResistor::param_id_R = 0;

DB_PUBLIC size_t DeviceClassResistor::terminal_id_A = 0;
DB_PUBLIC size_t DeviceClassResistor::terminal_id_B = 1;

DeviceClassResistor::DeviceClassResistor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("A", "Terminal A"));
  add_terminal_definition (db::DeviceTerminalDefinition ("B", "Terminal B"));

  add_parameter_definition (db::DeviceParameterDefinition ("R", "Resistance (Ohm)", 0.0));
}

void DeviceClassResistor::parallel (Device *a, Device *b) const
{
  double va = a->parameter_value (0);
  double vb = b->parameter_value (0);
  a->set_parameter_value (0, va + vb < 1e-10 ? 0.0 : va * vb / (va + vb));
}

void DeviceClassResistor::serial (Device *a, Device *b) const
{
  double va = a->parameter_value (0);
  double vb = b->parameter_value (0);
  a->set_parameter_value (0, va + vb);
}

// ------------------------------------------------------------------------------------
//  DeviceClassCapacitor implementation

DB_PUBLIC size_t DeviceClassCapacitor::param_id_C = 0;

DB_PUBLIC size_t DeviceClassCapacitor::terminal_id_A = 0;
DB_PUBLIC size_t DeviceClassCapacitor::terminal_id_B = 1;

DeviceClassCapacitor::DeviceClassCapacitor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("A", "Terminal A"));
  add_terminal_definition (db::DeviceTerminalDefinition ("B", "Terminal B"));

  add_parameter_definition (db::DeviceParameterDefinition ("C", "Capacitance (Farad)", 0.0));
}

void DeviceClassCapacitor::serial (Device *a, Device *b) const
{
  double va = a->parameter_value (0);
  double vb = b->parameter_value (0);
  a->set_parameter_value (0, va + vb < 1e-10 ? 0.0 : va * vb / (va + vb));
}

void DeviceClassCapacitor::parallel (Device *a, Device *b) const
{
  double va = a->parameter_value (0);
  double vb = b->parameter_value (0);
  a->set_parameter_value (0, va + vb);
}

// ------------------------------------------------------------------------------------
//  DeviceClassInductor implementation

DB_PUBLIC size_t DeviceClassInductor::param_id_L = 0;

DB_PUBLIC size_t DeviceClassInductor::terminal_id_A = 0;
DB_PUBLIC size_t DeviceClassInductor::terminal_id_B = 1;

DeviceClassInductor::DeviceClassInductor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("A", "Terminal A"));
  add_terminal_definition (db::DeviceTerminalDefinition ("B", "Terminal B"));

  add_parameter_definition (db::DeviceParameterDefinition ("L", "Inductance (Henry)", 0.0));
}

void DeviceClassInductor::parallel (Device *a, Device *b) const
{
  double va = a->parameter_value (0);
  double vb = b->parameter_value (0);
  a->set_parameter_value (0, va + vb < 1e-10 ? 0.0 : va * vb / (va + vb));
}

void DeviceClassInductor::serial (Device *a, Device *b) const
{
  double va = a->parameter_value (0);
  double vb = b->parameter_value (0);
  a->set_parameter_value (0, va + vb);
}

// ------------------------------------------------------------------------------------
//  DeviceClassInductor implementation

DB_PUBLIC size_t DeviceClassDiode::param_id_A = 0;

DB_PUBLIC size_t DeviceClassDiode::terminal_id_A = 0;
DB_PUBLIC size_t DeviceClassDiode::terminal_id_C = 1;

DeviceClassDiode::DeviceClassDiode ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("A", "Anode"));
  add_terminal_definition (db::DeviceTerminalDefinition ("C", "Cathode"));

  add_parameter_definition (db::DeviceParameterDefinition ("A", "Area (square micrometer)", 0.0));
}

bool DeviceClassDiode::combine_devices (Device *a, Device *b) const
{
  const db::Net *na1 = a->net_for_terminal (0);
  const db::Net *na2 = a->net_for_terminal (1);
  const db::Net *nb1 = b->net_for_terminal (0);
  const db::Net *nb2 = b->net_for_terminal (1);

  //  only parallel diodes can be combined and their areas will add
  if (na1 == nb1 && na2 == nb2) {

    a->set_parameter_value (0, a->parameter_value (0) + b->parameter_value (0));
    b->connect_terminal (0, 0);
    b->connect_terminal (1, 0);

    return true;

  } else {
    return false;
  }
}

// ------------------------------------------------------------------------------------
//  DeviceClassMOS3Transistor implementation

DB_PUBLIC size_t DeviceClassMOS3Transistor::param_id_L = 0;
DB_PUBLIC size_t DeviceClassMOS3Transistor::param_id_W = 1;
DB_PUBLIC size_t DeviceClassMOS3Transistor::param_id_AS = 2;
DB_PUBLIC size_t DeviceClassMOS3Transistor::param_id_AD = 3;
DB_PUBLIC size_t DeviceClassMOS3Transistor::param_id_PS = 4;
DB_PUBLIC size_t DeviceClassMOS3Transistor::param_id_PD = 5;

DB_PUBLIC size_t DeviceClassMOS3Transistor::terminal_id_S = 0;
DB_PUBLIC size_t DeviceClassMOS3Transistor::terminal_id_G = 1;
DB_PUBLIC size_t DeviceClassMOS3Transistor::terminal_id_D = 2;

DeviceClassMOS3Transistor::DeviceClassMOS3Transistor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("S", "Source"));
  add_terminal_definition (db::DeviceTerminalDefinition ("G", "Gate"));
  add_terminal_definition (db::DeviceTerminalDefinition ("D", "Drain"));

  add_parameter_definition (db::DeviceParameterDefinition ("L", "Gate length (micrometer)", 0.0));
  add_parameter_definition (db::DeviceParameterDefinition ("W", "Gate width (micrometer)", 0.0));
  add_parameter_definition (db::DeviceParameterDefinition ("AS", "Source area (square micrometer)", 0.0, false));
  add_parameter_definition (db::DeviceParameterDefinition ("AD", "Drain area (square micrometer)", 0.0, false));
  add_parameter_definition (db::DeviceParameterDefinition ("PS", "Source perimeter (micrometer)", 0.0, false));
  add_parameter_definition (db::DeviceParameterDefinition ("PD", "Drain perimeter (micrometer)", 0.0, false));
}

bool DeviceClassMOS3Transistor::combine_devices (Device *a, Device *b) const
{
  const db::Net *nas = a->net_for_terminal (0);
  const db::Net *nag = a->net_for_terminal (1);
  const db::Net *nad = a->net_for_terminal (2);
  const db::Net *nbs = b->net_for_terminal (0);
  const db::Net *nbg = b->net_for_terminal (1);
  const db::Net *nbd = b->net_for_terminal (2);

  //  parallel transistors can be combined into one
  if (((nas == nbs && nad == nbd) || (nas == nbd && nad == nbs)) && nag == nbg) {

    //  for combination the gate length must be identical
    if (fabs (a->parameter_value (0) - b->parameter_value (0)) < 1e-6) {

      combine_parameters (a, b);

      b->connect_terminal (0, 0);
      b->connect_terminal (1, 0);
      b->connect_terminal (2, 0);

      return true;

    }

  }

  return false;
}

void DeviceClassMOS3Transistor::combine_parameters (Device *a, Device *b) const
{
  a->set_parameter_value (1, a->parameter_value (1) + b->parameter_value (1));
  a->set_parameter_value (2, a->parameter_value (2) + b->parameter_value (2));
  a->set_parameter_value (3, a->parameter_value (3) + b->parameter_value (3));
  a->set_parameter_value (4, a->parameter_value (4) + b->parameter_value (4));
  a->set_parameter_value (5, a->parameter_value (5) + b->parameter_value (5));
}

// ------------------------------------------------------------------------------------
//  DeviceClassMOS4Transistor implementation

DB_PUBLIC size_t DeviceClassMOS4Transistor::terminal_id_B = 3;

DeviceClassMOS4Transistor::DeviceClassMOS4Transistor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("B", "Bulk"));
}

bool DeviceClassMOS4Transistor::combine_devices (Device *a, Device *b) const
{
  const db::Net *nas = a->net_for_terminal (0);
  const db::Net *nag = a->net_for_terminal (1);
  const db::Net *nad = a->net_for_terminal (2);
  const db::Net *nab = a->net_for_terminal (3);
  const db::Net *nbs = b->net_for_terminal (0);
  const db::Net *nbg = b->net_for_terminal (1);
  const db::Net *nbd = b->net_for_terminal (2);
  const db::Net *nbb = b->net_for_terminal (3);

  //  parallel transistors can be combined into one
  if (((nas == nbs && nad == nbd) || (nas == nbd && nad == nbs)) && nag == nbg && nab == nbb) {

    //  for combination the gate length must be identical
    if (fabs (a->parameter_value (0) - b->parameter_value (0)) < 1e-6) {

      combine_parameters (a, b);

      b->connect_terminal (0, 0);
      b->connect_terminal (1, 0);
      b->connect_terminal (2, 0);
      b->connect_terminal (3, 0);

      return true;

    }

  }

  return false;
}

}

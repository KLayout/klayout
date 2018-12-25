
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

#include "dbNetlistDeviceClasses.h"

namespace db
{

// ------------------------------------------------------------------------------------
//  DeviceClassTwoPortDevice implementation

bool DeviceClassTwoPortDevice::combine_devices (Device *a, Device *b) const
{
  db::Net *na1 = a->net_for_port (0);
  db::Net *na2 = a->net_for_port (1);
  db::Net *nb1 = b->net_for_port (0);
  db::Net *nb2 = b->net_for_port (1);

  bool res = true;

  if ((na1 == nb1 && na2 == nb2) || (na1 == nb2 && na2 == nb1)) {

    parallel (a, b);

  } else if (na2 == nb1 || na2 == nb2) {

    //  serial a(B) to b(A or B)
    serial (a, b);
    a->connect_port (1, (na2 == nb1 ? nb2 : nb1));

  } else if (na1 == nb1 || na1 == nb2) {

    //  serial a(A) to b(A or B)
    serial (a, b);
    a->connect_port (0, (na1 == nb1 ? nb2 : nb1));

  }

  if (res) {
    b->connect_port (0, 0);
    b->connect_port (1, 0);
    return true;
  } else {
    return false;
  }
}


// ------------------------------------------------------------------------------------
//  DeviceClassResistor implementation

DeviceClassResistor::DeviceClassResistor ()
{
  add_port_definition (db::DevicePortDefinition ("A", "Port A"));
  add_port_definition (db::DevicePortDefinition ("B", "Port B"));

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

DeviceClassCapacitor::DeviceClassCapacitor ()
{
  add_port_definition (db::DevicePortDefinition ("A", "Port A"));
  add_port_definition (db::DevicePortDefinition ("B", "Port B"));

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

DeviceClassInductor::DeviceClassInductor ()
{
  add_port_definition (db::DevicePortDefinition ("A", "Port A"));
  add_port_definition (db::DevicePortDefinition ("B", "Port B"));

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

DeviceClassDiode::DeviceClassDiode ()
{
  add_port_definition (db::DevicePortDefinition ("A", "Anode"));
  add_port_definition (db::DevicePortDefinition ("C", "Cathode"));

  add_parameter_definition (db::DeviceParameterDefinition ("A", "Area (square micrometer)", 0.0));
}

bool DeviceClassDiode::combine_devices (Device *a, Device *b) const
{
  const db::Net *na1 = a->net_for_port (0);
  const db::Net *na2 = a->net_for_port (1);
  const db::Net *nb1 = b->net_for_port (0);
  const db::Net *nb2 = b->net_for_port (1);

  if ((na1 == nb1 && na2 == nb2) || (na1 == nb2 && na2 == nb1)) {

    a->set_parameter_value (0, a->parameter_value (0) + b->parameter_value (0));
    b->connect_port (0, 0);
    b->connect_port (1, 0);

    return true;

  } else {
    return false;
  }
}

// ------------------------------------------------------------------------------------
//  DeviceClassInductor implementation

DeviceClassMOSTransistor::DeviceClassMOSTransistor ()
{
  add_port_definition (db::DevicePortDefinition ("S", "Source"));
  add_port_definition (db::DevicePortDefinition ("G", "Gate"));
  add_port_definition (db::DevicePortDefinition ("D", "Drain"));

  add_parameter_definition (db::DeviceParameterDefinition ("L", "Gate length (micrometer)", 0.0));
  add_parameter_definition (db::DeviceParameterDefinition ("W", "Gate width (micrometer)", 0.0));
  add_parameter_definition (db::DeviceParameterDefinition ("AS", "Source area (square micrometer)", 0.0));
  add_parameter_definition (db::DeviceParameterDefinition ("AD", "Drain area (square micrometer)", 0.0));
}

bool DeviceClassMOSTransistor::combine_devices (Device *a, Device *b) const
{
  const db::Net *nas = a->net_for_port (0);
  const db::Net *nag = a->net_for_port (1);
  const db::Net *nad = a->net_for_port (2);
  const db::Net *nbs = b->net_for_port (0);
  const db::Net *nbg = b->net_for_port (1);
  const db::Net *nbd = b->net_for_port (2);

  //  parallel transistors can be combined into one
  if (((nas == nbs && nad == nbd) || (nas == nbd && nad == nbs)) && nag == nbg) {

    //  for combination the gate length must be identical
    if (fabs (a->parameter_value (0) - b->parameter_value (0)) < 1e-6) {

      a->set_parameter_value (1, a->parameter_value (1) + b->parameter_value (1));
      a->set_parameter_value (2, a->parameter_value (2) + b->parameter_value (2));
      a->set_parameter_value (3, a->parameter_value (3) + b->parameter_value (3));
      b->connect_port (0, 0);
      b->connect_port (1, 0);
      b->connect_port (2, 0);

      return true;

    }

  }

  return false;
}

}

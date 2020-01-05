
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
#include "tlClassRegistry.h"

namespace db
{

// ------------------------------------------------------------------------------------
//  The built-in device class templates

static tl::RegisteredClass<db::DeviceClassTemplateBase> dct_cap (new db::device_class_template<db::DeviceClassCapacitor> ("CAP"));
static tl::RegisteredClass<db::DeviceClassTemplateBase> dct_res (new db::device_class_template<db::DeviceClassResistor> ("RES"));
static tl::RegisteredClass<db::DeviceClassTemplateBase> dct_ind (new db::device_class_template<db::DeviceClassInductor> ("IND"));
static tl::RegisteredClass<db::DeviceClassTemplateBase> dct_diode (new db::device_class_template<db::DeviceClassDiode> ("DIODE"));
static tl::RegisteredClass<db::DeviceClassTemplateBase> dct_mos3 (new db::device_class_template<db::DeviceClassMOS3Transistor> ("MOS3"));
static tl::RegisteredClass<db::DeviceClassTemplateBase> dct_mos4 (new db::device_class_template<db::DeviceClassMOS4Transistor> ("MOS4"));

// ------------------------------------------------------------------------------------
//  DeviceClassTwoTerminalDevice implementation

bool DeviceClassTwoTerminalDevice::combine_devices (Device *a, Device *b) const
{
  db::Net *na1 = a->net_for_terminal (0);
  db::Net *na2 = a->net_for_terminal (1);
  db::Net *nb1 = b->net_for_terminal (0);
  db::Net *nb2 = b->net_for_terminal (1);

  if ((na1 == nb1 && na2 == nb2) || (na1 == nb2 && na2 == nb1)) {

    parallel (a, b);

    if (na1 == nb1 && na2 == nb2) {
      a->join_terminals (0, b, 0);
      a->join_terminals (1, b, 1);
    } else {
      a->join_terminals (0, b, 1);
      a->join_terminals (1, b, 0);
    }

    return true;

  } else if ((na2 == nb1 || na2 == nb2) && na2->is_internal ()) {

    //  serial a(B) to b(A or B)
    serial (a, b);

    if (na2 == nb1) {
      a->reroute_terminal (1, b, 0, 1);
    } else {
      a->reroute_terminal (1, b, 1, 0);
    }

    return true;

  } else if ((na1 == nb1 || na1 == nb2) && na1->is_internal ()) {

    //  serial a(A) to b(A or B)
    serial (a, b);

    if (na1 == nb1) {
      a->reroute_terminal (0, b, 0, 1);
    } else {
      a->reroute_terminal (0, b, 1, 0);
    }

    return true;

  } else {
    return false;
  }
}


// ------------------------------------------------------------------------------------
//  DeviceClassResistor implementation

DB_PUBLIC size_t DeviceClassResistor::param_id_R = 0;
DB_PUBLIC size_t DeviceClassResistor::param_id_L = 1;
DB_PUBLIC size_t DeviceClassResistor::param_id_W = 2;
DB_PUBLIC size_t DeviceClassResistor::param_id_A = 3;
DB_PUBLIC size_t DeviceClassResistor::param_id_P = 4;

DB_PUBLIC size_t DeviceClassResistor::terminal_id_A = 0;
DB_PUBLIC size_t DeviceClassResistor::terminal_id_B = 1;

DeviceClassResistor::DeviceClassResistor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("A", "Terminal A"));
  add_terminal_definition (db::DeviceTerminalDefinition ("B", "Terminal B"));

  add_parameter_definition (db::DeviceParameterDefinition ("R", "Resistance (Ohm)", 0.0));
  add_parameter_definition (db::DeviceParameterDefinition ("L", "Length (micrometer)", 0.0, false, 1e-6));
  add_parameter_definition (db::DeviceParameterDefinition ("W", "Width (micrometer)", 0.0, false, 1e-6));
  add_parameter_definition (db::DeviceParameterDefinition ("A", "Area (square micrometer)", 0.0, false, 1e-12));
  add_parameter_definition (db::DeviceParameterDefinition ("P", "Perimeter (micrometer)", 0.0, false, 1e-6));
}

void DeviceClassResistor::parallel (Device *a, Device *b) const
{
  double va = a->parameter_value (0);
  double vb = b->parameter_value (0);
  a->set_parameter_value (0, va + vb < 1e-10 ? 0.0 : va * vb / (va + vb));

  //  parallel width is sum of both, length is the one that gives the same value of resistance
  //    R = 1/(1/R1 + 1/R2)
  //    R = L/(W1+W2)
  //    R1 = L1/W1
  //    R2 = L2/W2
  //  -> L = (L1*L2*(W1+W2))/(L2*W1+L1*W2))
  double l1 = a->parameter_value (1);
  double w1 = a->parameter_value (2);
  double l2 = b->parameter_value (1);
  double w2 = b->parameter_value (2);
  double dnom = (l2 * w1 + l1 * w2);
  if (fabs (dnom) > 1e-15) {
    a->set_parameter_value (1, (l1 * l2 * (w1 + w2)) / dnom);
  }
  a->set_parameter_value (2, w1 + w2);

  //  TODO: does this implementation make sense? (area)
  double aa = a->parameter_value (3);
  double ab = b->parameter_value (3);
  a->set_parameter_value (3, aa + ab);

  //  TODO: does this implementation make sense? (perimeter)
  double pa = a->parameter_value (4);
  double pb = b->parameter_value (4);
  a->set_parameter_value (4, pa + pb);
}

void DeviceClassResistor::serial (Device *a, Device *b) const
{
  double va = a->parameter_value (0);
  double vb = b->parameter_value (0);
  a->set_parameter_value (0, va + vb);

  //  parallel length is sum of both, width is the one that gives the same value of resistance
  //  assuming same sheet rho
  //    R = R1+R2
  //    R = (L1+L2)/W
  //    R1 = L1/W1
  //    R2 = L2/W2
  //  -> W = ((L1+L2)*W1*W2)/(W1*L2+W2*L1)
  double l1 = a->parameter_value (1);
  double w1 = a->parameter_value (2);
  double l2 = b->parameter_value (1);
  double w2 = b->parameter_value (2);
  a->set_parameter_value (1, l1 + l2);
  double dnom = (l2 * w1 + l1 * w2);
  if (fabs (dnom) > 1e-15) {
    a->set_parameter_value (2, (w1 * w2 * (l1 + l2)) / dnom);
  }

  double aa = a->parameter_value (3);
  double ab = b->parameter_value (3);
  a->set_parameter_value (3, aa + ab);

  double pa = a->parameter_value (4);
  double pb = b->parameter_value (4);
  a->set_parameter_value (4, pa + pb);
}

// ------------------------------------------------------------------------------------
//  DeviceClassResistorWithBulk implementation

DB_PUBLIC size_t DeviceClassResistorWithBulk::terminal_id_W = 2;

DeviceClassResistorWithBulk::DeviceClassResistorWithBulk ()
  : DeviceClassResistor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("W", "Terminal W (well, bulk)"));
}

bool DeviceClassResistorWithBulk::combine_devices (Device *a, Device *b) const
{
  db::Net *nab = a->net_for_terminal (2);
  db::Net *nbb = b->net_for_terminal (2);

  if (nab == nbb && DeviceClassResistor::combine_devices (a, b)) {
    a->join_terminals (2, b, 2);
    return true;
  } else {
    return false;
  }
}

// ------------------------------------------------------------------------------------
//  DeviceClassCapacitor implementation

DB_PUBLIC size_t DeviceClassCapacitor::param_id_C = 0;
DB_PUBLIC size_t DeviceClassCapacitor::param_id_A = 1;
DB_PUBLIC size_t DeviceClassCapacitor::param_id_P = 2;

DB_PUBLIC size_t DeviceClassCapacitor::terminal_id_A = 0;
DB_PUBLIC size_t DeviceClassCapacitor::terminal_id_B = 1;

DeviceClassCapacitor::DeviceClassCapacitor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("A", "Terminal A"));
  add_terminal_definition (db::DeviceTerminalDefinition ("B", "Terminal B"));

  add_parameter_definition (db::DeviceParameterDefinition ("C", "Capacitance (Farad)", 0.0));
  add_parameter_definition (db::DeviceParameterDefinition ("A", "Area (square micrometer)", 0.0, false, 1e-12));
  add_parameter_definition (db::DeviceParameterDefinition ("P", "Perimeter (micrometer)", 0.0, false, 1e-6));
}

void DeviceClassCapacitor::serial (Device *a, Device *b) const
{
  double va = a->parameter_value (0);
  double vb = b->parameter_value (0);
  a->set_parameter_value (0, va + vb < 1e-10 ? 0.0 : va * vb / (va + vb));

  //  TODO: does this implementation make sense?
  double aa = a->parameter_value (1);
  double ab = b->parameter_value (1);
  a->set_parameter_value (1, aa + ab);

  //  TODO: does this implementation make sense?
  double pa = a->parameter_value (2);
  double pb = b->parameter_value (2);
  a->set_parameter_value (2, pa + pb);
}

void DeviceClassCapacitor::parallel (Device *a, Device *b) const
{
  double va = a->parameter_value (0);
  double vb = b->parameter_value (0);
  a->set_parameter_value (0, va + vb);

  double aa = a->parameter_value (1);
  double ab = b->parameter_value (1);
  a->set_parameter_value (1, aa + ab);

  double pa = a->parameter_value (2);
  double pb = b->parameter_value (2);
  a->set_parameter_value (2, pa + pb);
}

// ------------------------------------------------------------------------------------
//  DeviceClassCapacitorWithBulk implementation

DB_PUBLIC size_t DeviceClassCapacitorWithBulk::terminal_id_W = 2;

DeviceClassCapacitorWithBulk::DeviceClassCapacitorWithBulk ()
  : DeviceClassCapacitor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("W", "Terminal W (well, bulk)"));
}

bool DeviceClassCapacitorWithBulk::combine_devices (Device *a, Device *b) const
{
  db::Net *nab = a->net_for_terminal (2);
  db::Net *nbb = b->net_for_terminal (2);

  if (nab == nbb && DeviceClassCapacitor::combine_devices (a, b)) {
    a->join_terminals (2, b, 2);
    return true;
  } else {
    return false;
  }
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
DB_PUBLIC size_t DeviceClassDiode::param_id_P = 1;

DB_PUBLIC size_t DeviceClassDiode::terminal_id_A = 0;
DB_PUBLIC size_t DeviceClassDiode::terminal_id_C = 1;

DeviceClassDiode::DeviceClassDiode ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("A", "Anode"));
  add_terminal_definition (db::DeviceTerminalDefinition ("C", "Cathode"));

  add_parameter_definition (db::DeviceParameterDefinition ("A", "Area (square micrometer)", 0.0, false, 1e-12));
  add_parameter_definition (db::DeviceParameterDefinition ("P", "Perimeter (micrometer)", 0.0, false, 1e-6));
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
    a->set_parameter_value (1, a->parameter_value (1) + b->parameter_value (1));

    a->join_terminals (0, b, 0);
    a->join_terminals (1, b, 1);

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

  add_parameter_definition (db::DeviceParameterDefinition ("L", "Gate length (micrometer)", 0.0, true, 1e-6));
  add_parameter_definition (db::DeviceParameterDefinition ("W", "Gate width (micrometer)", 0.0, true, 1e-6));
  add_parameter_definition (db::DeviceParameterDefinition ("AS", "Source area (square micrometer)", 0.0, false, 1e-12));
  add_parameter_definition (db::DeviceParameterDefinition ("AD", "Drain area (square micrometer)", 0.0, false, 1e-12));
  add_parameter_definition (db::DeviceParameterDefinition ("PS", "Source perimeter (micrometer)", 0.0, false, 1e-6));
  add_parameter_definition (db::DeviceParameterDefinition ("PD", "Drain perimeter (micrometer)", 0.0, false, 1e-6));
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

      if (nas == nbs && nad == nbd) {
        a->join_terminals (0, b, 0);
        a->join_terminals (2, b, 2);
      } else {
        a->join_terminals (0, b, 2);
        a->join_terminals (2, b, 0);
      }

      a->join_terminals (1, b, 1);

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

      if (nas == nbs && nad == nbd) {
        a->join_terminals (0, b, 0);
        a->join_terminals (2, b, 2);
      } else {
        a->join_terminals (0, b, 2);
        a->join_terminals (2, b, 0);
      }

      a->join_terminals (1, b, 1);
      a->join_terminals (3, b, 3);

      return true;

    }

  }

  return false;
}

// ------------------------------------------------------------------------------------
//  DeviceClassBJT3Transistor implementation

DB_PUBLIC size_t DeviceClassBJT3Transistor::param_id_AE = 0;
DB_PUBLIC size_t DeviceClassBJT3Transistor::param_id_PE = 1;
DB_PUBLIC size_t DeviceClassBJT3Transistor::param_id_AB = 2;
DB_PUBLIC size_t DeviceClassBJT3Transistor::param_id_PB = 3;
DB_PUBLIC size_t DeviceClassBJT3Transistor::param_id_AC = 4;
DB_PUBLIC size_t DeviceClassBJT3Transistor::param_id_PC = 5;
DB_PUBLIC size_t DeviceClassBJT3Transistor::param_id_NE = 6;

DB_PUBLIC size_t DeviceClassBJT3Transistor::terminal_id_C = 0;
DB_PUBLIC size_t DeviceClassBJT3Transistor::terminal_id_B = 1;
DB_PUBLIC size_t DeviceClassBJT3Transistor::terminal_id_E = 2;

DeviceClassBJT3Transistor::DeviceClassBJT3Transistor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("C", "Collector"));
  add_terminal_definition (db::DeviceTerminalDefinition ("B", "Base"));
  add_terminal_definition (db::DeviceTerminalDefinition ("E", "Emitter"));

  //  NOTE: the emitter area and the emitter count are the primary parameters
  add_parameter_definition (db::DeviceParameterDefinition ("AE", "Emitter area (square micrometer)", 0.0, true, 1e-12));
  add_parameter_definition (db::DeviceParameterDefinition ("PE", "Emitter perimeter (micrometer)", 0.0, false, 1e-6));
  add_parameter_definition (db::DeviceParameterDefinition ("AB", "Base area (square micrometer)", 0.0, false, 1e-12));
  add_parameter_definition (db::DeviceParameterDefinition ("PB", "Base perimeter (micrometer)", 0.0, false, 1e-6));
  add_parameter_definition (db::DeviceParameterDefinition ("AC", "Collector area (square micrometer)", 0.0, false, 1e-12));
  add_parameter_definition (db::DeviceParameterDefinition ("PC", "Collector perimeter (micrometer)", 0.0, false, 1e-6));
  add_parameter_definition (db::DeviceParameterDefinition ("NE", "Emitter count", 1.0, true));
}

bool DeviceClassBJT3Transistor::combine_devices (Device *a, Device *b) const
{
  const db::Net *nac = a->net_for_terminal (0);
  const db::Net *nab = a->net_for_terminal (1);
  const db::Net *nae = a->net_for_terminal (2);
  const db::Net *nbc = b->net_for_terminal (0);
  const db::Net *nbb = b->net_for_terminal (1);
  const db::Net *nbe = b->net_for_terminal (2);

  //  parallel transistors can be combined into one
  if (nac == nbc && nae == nbe && nab == nbb) {

    combine_parameters (a, b);

    a->join_terminals (0, b, 0);
    a->join_terminals (1, b, 1);
    a->join_terminals (2, b, 2);

    return true;

  }

  return false;
}

void DeviceClassBJT3Transistor::combine_parameters (Device *a, Device *b) const
{
  a->set_parameter_value (param_id_AE, a->parameter_value (param_id_AE) + b->parameter_value (param_id_AE));
  a->set_parameter_value (param_id_PE, a->parameter_value (param_id_PE) + b->parameter_value (param_id_PE));
  a->set_parameter_value (param_id_NE, a->parameter_value (param_id_NE) + b->parameter_value (param_id_NE));
}

// ------------------------------------------------------------------------------------
//  DeviceClassBJT4Transistor implementation

DB_PUBLIC size_t DeviceClassBJT4Transistor::terminal_id_S = 3;

DeviceClassBJT4Transistor::DeviceClassBJT4Transistor ()
{
  add_terminal_definition (db::DeviceTerminalDefinition ("S", "Substrate"));
}

bool DeviceClassBJT4Transistor::combine_devices (Device *a, Device *b) const
{
  const db::Net *nac = a->net_for_terminal (0);
  const db::Net *nab = a->net_for_terminal (1);
  const db::Net *nae = a->net_for_terminal (2);
  const db::Net *nas = a->net_for_terminal (3);
  const db::Net *nbc = b->net_for_terminal (0);
  const db::Net *nbb = b->net_for_terminal (1);
  const db::Net *nbe = b->net_for_terminal (2);
  const db::Net *nbs = b->net_for_terminal (3);

  //  parallel transistors can be combined into one
  if (nac == nbc && nae == nbe && nab == nbb && nas == nbs) {

    combine_parameters (a, b);

    a->join_terminals (0, b, 0);
    a->join_terminals (1, b, 1);
    a->join_terminals (2, b, 2);
    a->join_terminals (3, b, 3);

    return true;

  }

  return false;
}

}

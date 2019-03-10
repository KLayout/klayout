
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

#include "gsiDecl.h"
#include "dbNetlistDeviceClasses.h"

namespace gsi
{

extern Class<db::DeviceClass> decl_dbDeviceClass;

Class<db::DeviceClassResistor> decl_dbDeviceClassResistor (decl_dbDeviceClass, "db", "DeviceClassResistor",
  gsi::constant ("TERMINAL_A", db::DeviceClassResistor::terminal_id_A,
    "@brief A constant giving the terminal ID for terminal A"
  ) +
  gsi::constant ("TERMINAL_B", db::DeviceClassResistor::terminal_id_B,
    "@brief A constant giving the terminal ID for terminal B"
  ) +
  gsi::constant ("PARAM_R", db::DeviceClassResistor::param_id_R,
    "@brief A constant giving the parameter ID for parameter R"
  ),
  "@brief A device class for a resistor.\n"
  "This class can be used to describe resistors. Resistors are defined by their combination behavior and "
  "the basic parameter 'R' which is the resistance in Ohm.\n"
  "\n"
  "A resistor has two terminals, A and B.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::DeviceClassCapacitor> decl_dbDeviceClassCapacitor (decl_dbDeviceClass, "db", "DeviceClassCapacitor",
  gsi::constant ("TERMINAL_A", db::DeviceClassCapacitor::terminal_id_A,
    "@brief A constant giving the terminal ID for terminal A"
  ) +
  gsi::constant ("TERMINAL_B", db::DeviceClassCapacitor::terminal_id_B,
    "@brief A constant giving the terminal ID for terminal B"
  ) +
  gsi::constant ("PARAM_C", db::DeviceClassCapacitor::param_id_C,
    "@brief A constant giving the parameter ID for parameter C"
  ),
  "@brief A device class for a capacitor.\n"
  "This class can be used to describe capacitors. Capacitors are defined by their combination behavior and "
  "the basic parameter 'C' which is the capacitance in Farad.\n"
  "\n"
  "A capacitor has two terminals, A and B.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::DeviceClassInductor> decl_dbDeviceClassInductor (decl_dbDeviceClass, "db", "DeviceClassInductor",
  gsi::constant ("TERMINAL_A", db::DeviceClassInductor::terminal_id_A,
    "@brief A constant giving the terminal ID for terminal A"
  ) +
  gsi::constant ("TERMINAL_B", db::DeviceClassInductor::terminal_id_B,
    "@brief A constant giving the terminal ID for terminal B"
  ) +
  gsi::constant ("PARAM_L", db::DeviceClassInductor::param_id_L,
    "@brief A constant giving the parameter ID for parameter L"
  ),
  "@brief A device class for an inductor.\n"
  "This class can be used to describe inductors. Inductors are defined by their combination behavior and "
  "the basic parameter 'L' which is the inductance in Henry.\n"
  "\n"
  "An inductor has two terminals, A and B.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::DeviceClassDiode> decl_dbDeviceClassDiode (decl_dbDeviceClass, "db", "DeviceClassDiode",
  gsi::constant ("TERMINAL_A", db::DeviceClassDiode::terminal_id_A,
    "@brief A constant giving the terminal ID for terminal A"
  ) +
  gsi::constant ("TERMINAL_C", db::DeviceClassDiode::terminal_id_C,
    "@brief A constant giving the terminal ID for terminal C"
  ) +
  gsi::constant ("PARAM_A", db::DeviceClassDiode::param_id_A,
    "@brief A constant giving the parameter ID for parameter A"
  ),
  "@brief A device class for a diode.\n"
  "This class can be used to describe diodes. Diodes are defined by their combination behavior and "
  "the basic parameter 'A' which is their area in square micrometers.\n"
  "\n"
  "Diodes only combine when parallel and in the same direction. In this case, their areas are added."
  "\n"
  "An inductor has two terminals, A (anode) and C (cathode).\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::DeviceClassMOS3Transistor> decl_dbDeviceClassMOS3Transistor (decl_dbDeviceClass, "db", "DeviceClassMOS3Transistor",
  gsi::constant ("TERMINAL_S", db::DeviceClassMOS3Transistor::terminal_id_S,
    "@brief A constant giving the terminal ID for terminal S"
  ) +
  gsi::constant ("TERMINAL_D", db::DeviceClassMOS3Transistor::terminal_id_D,
    "@brief A constant giving the terminal ID for terminal D"
  ) +
  gsi::constant ("TERMINAL_G", db::DeviceClassMOS3Transistor::terminal_id_G,
    "@brief A constant giving the terminal ID for terminal G"
  ) +
  gsi::constant ("PARAM_L", db::DeviceClassMOS3Transistor::param_id_L,
    "@brief A constant giving the parameter ID for parameter L"
  ) +
  gsi::constant ("PARAM_W", db::DeviceClassMOS3Transistor::param_id_W,
    "@brief A constant giving the parameter ID for parameter W"
  ) +
  gsi::constant ("PARAM_AS", db::DeviceClassMOS3Transistor::param_id_AS,
    "@brief A constant giving the parameter ID for parameter AS"
  ) +
  gsi::constant ("PARAM_AD", db::DeviceClassMOS3Transistor::param_id_AD,
    "@brief A constant giving the parameter ID for parameter AD"
  ) +
  gsi::constant ("PARAM_PS", db::DeviceClassMOS3Transistor::param_id_PS,
    "@brief A constant giving the parameter ID for parameter PS"
  ) +
  gsi::constant ("PARAM_PD", db::DeviceClassMOS3Transistor::param_id_PD,
    "@brief A constant giving the parameter ID for parameter PD"
  ),
  "@brief A device class for a 3-terminal MOS transistor.\n"
  "This class can be used to describe MOS transistors without a bulk terminal. "
  "A device class for a MOS transistor with a bulk terminal is \\DeviceClassMOS4Transistor. "
  "MOS transistors are defined by their combination behavior and the basic parameters.\n"
  "\n"
  "The parameters are L, W, AS, AD, PS and PD for the gate length and width in micrometers, source and drain area "
  "in square micrometers and the source and drain perimeter in micrometers.\n"
  "\n"
  "The terminals are S, G and D for source, gate and drain.\n"
  "\n"
  "MOS transistors combine in parallel mode, when both gate lengths are identical and "
  "their gates are connected (source and drain can be swapped). In this case, their widths and source and drain "
  "areas are added.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::DeviceClassMOS4Transistor> decl_dbDeviceClassMOS4Transistor (decl_dbDeviceClass, "db", "DeviceClassMOS4Transistor",
  gsi::constant ("TERMINAL_S", db::DeviceClassMOS4Transistor::terminal_id_S,
    "@brief A constant giving the terminal ID for terminal S"
  ) +
  gsi::constant ("TERMINAL_D", db::DeviceClassMOS4Transistor::terminal_id_D,
    "@brief A constant giving the terminal ID for terminal D"
  ) +
  gsi::constant ("TERMINAL_G", db::DeviceClassMOS4Transistor::terminal_id_G,
    "@brief A constant giving the terminal ID for terminal G"
  ) +
  gsi::constant ("TERMINAL_B", db::DeviceClassMOS4Transistor::terminal_id_B,
    "@brief A constant giving the terminal ID for terminal B"
  ) +
  gsi::constant ("PARAM_L", db::DeviceClassMOS4Transistor::param_id_L,
    "@brief A constant giving the parameter ID for parameter L"
  ) +
  gsi::constant ("PARAM_W", db::DeviceClassMOS4Transistor::param_id_W,
    "@brief A constant giving the parameter ID for parameter W"
  ) +
  gsi::constant ("PARAM_AS", db::DeviceClassMOS4Transistor::param_id_AS,
    "@brief A constant giving the parameter ID for parameter AS"
  ) +
  gsi::constant ("PARAM_AD", db::DeviceClassMOS4Transistor::param_id_AD,
    "@brief A constant giving the parameter ID for parameter AD"
  ) +
  gsi::constant ("PARAM_PS", db::DeviceClassMOS4Transistor::param_id_PS,
    "@brief A constant giving the parameter ID for parameter PS"
  ) +
  gsi::constant ("PARAM_PD", db::DeviceClassMOS4Transistor::param_id_PD,
    "@brief A constant giving the parameter ID for parameter PD"
  ),
  "@brief A device class for a 4-terminal MOS transistor.\n"
  "This class can be used to describe MOS transistors with a bulk terminal. "
  "A device class for a MOS transistor without a bulk terminal is \\DeviceClassMOS3Transistor. "
  "MOS transistors are defined by their combination behavior and the basic parameters.\n"
  "\n"
  "The parameters are L, W, AS, AD, PS and PD for the gate length and width in micrometers, source and drain area "
  "in square micrometers and the source and drain perimeter in micrometers.\n"
  "\n"
  "The terminals are S, G, D and B for source, gate, drain and bulk.\n"
  "\n"
  "MOS transistors combine in parallel mode, when both gate lengths are identical and "
  "their gates and bulk terminals are connected (source and drain can be swapped). In this case, their widths and source and drain "
  "areas are added.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

}

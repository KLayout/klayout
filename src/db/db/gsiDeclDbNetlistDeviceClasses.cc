
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
  ) +
  gsi::constant ("PARAM_L", db::DeviceClassResistor::param_id_L,
    "@brief A constant giving the parameter ID for parameter L"
  ) +
  gsi::constant ("PARAM_W", db::DeviceClassResistor::param_id_W,
    "@brief A constant giving the parameter ID for parameter W"
  ) +
  gsi::constant ("PARAM_A", db::DeviceClassResistor::param_id_A,
    "@brief A constant giving the parameter ID for parameter A"
  ) +
  gsi::constant ("PARAM_P", db::DeviceClassResistor::param_id_P,
    "@brief A constant giving the parameter ID for parameter P"
  ),
  "@brief A device class for a resistor.\n"
  "This class describes a resistor. Resistors are defined by their combination behavior and "
  "the basic parameter 'R' which is the resistance in Ohm.\n"
  "\n"
  "A resistor has two terminals, A and B.\n"
  "The parameters of a resistor are R (the value in Ohms), L and W (length and width in micrometers) and "
  "A and P (area and perimeter in square micrometers and micrometers respectively).\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::DeviceClassResistorWithBulk> decl_dbDeviceClassResistorWithBulk (decl_dbDeviceClassResistor, "db", "DeviceClassResistorWithBulk",
  gsi::constant ("TERMINAL_W", db::DeviceClassResistorWithBulk::terminal_id_W,
    "@brief A constant giving the terminal ID for terminal W (well, bulk)"
  ),
  "@brief A device class for a resistor with a bulk terminal (substrate, well).\n"
  "This class is similar to \\DeviceClassResistor, but provides an additional terminal (BULK) for the "
  "well or substrate the resistor is embedded in.\n"
  "\n"
  "The additional terminal is 'W' for the well/substrate terminal.\n"
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
  ) +
  gsi::constant ("PARAM_A", db::DeviceClassCapacitor::param_id_A,
    "@brief A constant giving the parameter ID for parameter A"
  ) +
  gsi::constant ("PARAM_P", db::DeviceClassCapacitor::param_id_P,
    "@brief A constant giving the parameter ID for parameter P"
  ),
  "@brief A device class for a capacitor.\n"
  "This describes a capacitor. Capacitors are defined by their combination behavior and "
  "the basic parameter 'C' which is the capacitance in Farad.\n"
  "\n"
  "A capacitor has two terminals, A and B.\n"
  "The parameters of a capacitor are C (the value in Farad) and "
  "A and P (area and perimeter in square micrometers and micrometers respectively).\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::DeviceClassCapacitorWithBulk> decl_dbDeviceClassCapacitorWithBulk (decl_dbDeviceClassCapacitor, "db", "DeviceClassCapacitorWithBulk",
  gsi::constant ("TERMINAL_W", db::DeviceClassCapacitorWithBulk::terminal_id_W,
    "@brief A constant giving the terminal ID for terminal W (well, bulk)"
  ),
  "@brief A device class for a capacitor with a bulk terminal (substrate, well).\n"
  "This class is similar to \\DeviceClassCapacitor, but provides an additional terminal (BULK) for the "
  "well or substrate the capacitor is embedded in.\n"
  "\n"
  "The additional terminal is 'W' for the well/substrate terminal.\n"
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
  "This class describes an inductor. Inductors are defined by their combination behavior and "
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
  ) +
  gsi::constant ("PARAM_P", db::DeviceClassDiode::param_id_P,
    "@brief A constant giving the parameter ID for parameter P"
  ),
  "@brief A device class for a diode.\n"
  "This class describes a diode.\n"
  "A diode has two terminals, A (anode) and C (cathode).\n"
  "It has two parameters: The diode area in square micrometers (A) and the diode area perimeter in micrometers (P).\n"
  "\n"
  "Diodes only combine when parallel and in the same direction. In this case, their areas and perimeters are added."
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::DeviceClassBJT3Transistor> decl_dbDeviceClassBJT3Transistor (decl_dbDeviceClass, "db", "DeviceClassBJT3Transistor",
  gsi::constant ("TERMINAL_C", db::DeviceClassBJT3Transistor::terminal_id_C,
    "@brief A constant giving the terminal ID for terminal C (collector)"
  ) +
  gsi::constant ("TERMINAL_B", db::DeviceClassBJT3Transistor::terminal_id_B,
    "@brief A constant giving the terminal ID for terminal B (base)"
  ) +
  gsi::constant ("TERMINAL_E", db::DeviceClassBJT3Transistor::terminal_id_E,
    "@brief A constant giving the terminal ID for terminal E (emitter)"
  ) +
  gsi::constant ("PARAM_AE", db::DeviceClassBJT3Transistor::param_id_AE,
    "@brief A constant giving the parameter ID for parameter AE (emitter area)"
  ) +
  gsi::constant ("PARAM_PE", db::DeviceClassBJT3Transistor::param_id_PE,
    "@brief A constant giving the parameter ID for parameter PE (emitter perimeter)"
  ) +
  gsi::constant ("PARAM_AB", db::DeviceClassBJT3Transistor::param_id_AB,
    "@brief A constant giving the parameter ID for parameter AB (base area)"
  ) +
  gsi::constant ("PARAM_PB", db::DeviceClassBJT3Transistor::param_id_PB,
    "@brief A constant giving the parameter ID for parameter PB (base perimeter)"
  ) +
  gsi::constant ("PARAM_AC", db::DeviceClassBJT3Transistor::param_id_AC,
    "@brief A constant giving the parameter ID for parameter AC (collector area)"
  ) +
  gsi::constant ("PARAM_PC", db::DeviceClassBJT3Transistor::param_id_PC,
    "@brief A constant giving the parameter ID for parameter PC (collector perimeter)"
  ) +
  gsi::constant ("PARAM_NE", db::DeviceClassBJT3Transistor::param_id_NE,
    "@brief A constant giving the parameter ID for parameter NE (emitter count)"
  ),
  "@brief A device class for a bipolar transistor.\n"
  "This class describes a bipolar transistor. Bipolar transistors have tree terminals: the collector (C), the base (B) and the emitter (E).\n"
  "Multi-emitter transistors are resolved in individual devices."
  "\n"
  "The parameters are AE, AB and AC for the emitter, base and collector areas in square micrometers and "
  "PE, PB and PC for the emitter, base and collector perimeters in micrometers.\n"
  "In addition, the emitter count (NE) is given. The emitter count is 1 always for a transistor extracted initially. "
  "Upon combination of devices, the emitter counts are added, thus forming multi-emitter devices.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::DeviceClassBJT4Transistor> decl_dbDeviceClassBJT4Transistor (decl_dbDeviceClassBJT3Transistor, "db", "DeviceClassBJT4Transistor",
  gsi::constant ("TERMINAL_S", db::DeviceClassBJT4Transistor::terminal_id_S,
    "@brief A constant giving the terminal ID for terminal S"
  ),
  "@brief A device class for a 4-terminal bipolar transistor.\n"
  "This class describes a bipolar transistor with a substrate terminal. "
  "A device class for a bipolar transistor without a substrate terminal is \\DeviceClassBJT3Transistor. "
  "\n"
  "The additional terminal is 'S' for the substrate terminal.\n"
  "BJT4 transistors combine in parallel if both substrate terminals are connected to the same net.\n"
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
  ) +
  gsi::method ("join_split_gates", &db::DeviceClassMOS3Transistor::join_split_gates, gsi::arg ("circuit"),
    "@brief Joins source/drain nets from 'split gate' transistor strings on the given circuit\n"
    "This method has been introduced in version 0.27.9\n"
  ),
  "@brief A device class for a 3-terminal MOS transistor.\n"
  "This class describes a MOS transistor without a bulk terminal. "
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

Class<db::DeviceClassMOS4Transistor> decl_dbDeviceClassMOS4Transistor (decl_dbDeviceClassMOS3Transistor, "db", "DeviceClassMOS4Transistor",
  gsi::constant ("TERMINAL_B", db::DeviceClassMOS4Transistor::terminal_id_B,
    "@brief A constant giving the terminal ID for terminal B"
  ),
  "@brief A device class for a 4-terminal MOS transistor.\n"
  "This class describes a MOS transistor with a bulk terminal. "
  "A device class for a MOS transistor without a bulk terminal is \\DeviceClassMOS3Transistor. "
  "MOS transistors are defined by their combination behavior and the basic parameters.\n"
  "\n"
  "The additional terminal is 'B' for the bulk terminal.\n"
  "MOS4 transistors combine in parallel if both bulk terminals are connected to the same net.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

}

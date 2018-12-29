
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
#include "dbNetlist.h"
#include "tlUnitTest.h"

#include <memory>
#include <limits>

TEST(1_SerialResistors)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin (db::Pin ("A"));
  db::Pin pin_b = circuit->add_pin (db::Pin ("B"));

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n3);
  circuit->connect_pin (pin_b.id (), n3);

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n3):\n"
    "  D r1 (A=n1,B=n2) [R=1]\n"
    "  D r2 (A=n2,B=n3) [R=3]\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n3):\n"
    "  D r1 (A=n1,B=n3) [R=4]\n"
  );
};

TEST(1_SerialResistors1Swapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin (db::Pin ("A"));
  db::Pin pin_b = circuit->add_pin (db::Pin ("B"));

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n3);
  circuit->connect_pin (pin_b.id (), n3);

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n3):\n"
    "  D r1 (A=n1,B=n2) [R=1]\n"
    "  D r2 (A=n3,B=n2) [R=3]\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n3):\n"
    "  D r1 (A=n1,B=n3) [R=4]\n"
  );
};

TEST(1_SerialResistors1OtherSwapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin (db::Pin ("A"));
  db::Pin pin_b = circuit->add_pin (db::Pin ("B"));

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n3);
  circuit->connect_pin (pin_b.id (), n3);

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n3):\n"
    "  D r1 (A=n2,B=n1) [R=1]\n"
    "  D r2 (A=n2,B=n3) [R=3]\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n3):\n"
    "  D r1 (A=n3,B=n1) [R=4]\n"
  );
};

TEST(1_SerialResistors2Swapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin (db::Pin ("A"));
  db::Pin pin_b = circuit->add_pin (db::Pin ("B"));

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n3);
  circuit->connect_pin (pin_b.id (), n3);

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n3):\n"
    "  D r1 (A=n2,B=n1) [R=1]\n"
    "  D r2 (A=n3,B=n2) [R=3]\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n3):\n"
    "  D r1 (A=n3,B=n1) [R=4]\n"
  );
};

TEST(1_SerialResistorsNoCombination)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin (db::Pin ("A"));
  db::Pin pin_b = circuit->add_pin (db::Pin ("B"));
  db::Pin pin_c = circuit->add_pin (db::Pin ("C"));

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_c.id (), n2);  // prevents combination
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n3);
  circuit->connect_pin (pin_b.id (), n3);

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n3,C=n2):\n"
    "  D r1 (A=n1,B=n2) [R=1]\n"
    "  D r2 (A=n2,B=n3) [R=3]\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n3,C=n2):\n"
    "  D r1 (A=n1,B=n2) [R=1]\n"
    "  D r2 (A=n2,B=n3) [R=3]\n"
  );
};

TEST(1_ParallelResistors)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 2.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin (db::Pin ("A"));
  db::Pin pin_b = circuit->add_pin (db::Pin ("B"));

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n2):\n"
    "  D r1 (A=n1,B=n2) [R=2]\n"
    "  D r2 (A=n1,B=n2) [R=3]\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n2):\n"
    "  D r1 (A=n1,B=n2) [R=1.2]\n"
  );
};

TEST(1_ParallelResistors1Swapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 2.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin (db::Pin ("A"));
  db::Pin pin_b = circuit->add_pin (db::Pin ("B"));

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n1);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n2):\n"
    "  D r1 (A=n2,B=n1) [R=2]\n"
    "  D r2 (A=n1,B=n2) [R=3]\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n2):\n"
    "  D r1 (A=n2,B=n1) [R=1.2]\n"
  );
};

TEST(1_ParallelResistors1OtherSwapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 2.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin (db::Pin ("A"));
  db::Pin pin_b = circuit->add_pin (db::Pin ("B"));

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n2):\n"
    "  D r1 (A=n1,B=n2) [R=2]\n"
    "  D r2 (A=n2,B=n1) [R=3]\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n2):\n"
    "  D r1 (A=n1,B=n2) [R=1.2]\n"
  );
};

TEST(1_ParallelResistors2Swapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 2.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin (db::Pin ("A"));
  db::Pin pin_b = circuit->add_pin (db::Pin ("B"));

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n1);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n2):\n"
    "  D r1 (A=n2,B=n1) [R=2]\n"
    "  D r2 (A=n2,B=n1) [R=3]\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n2):\n"
    "  D r1 (A=n2,B=n1) [R=1.2]\n"
  );
};

TEST(1_ComplexRegistorCombination)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  /**
   *                    (n2)
   *       +--[ r1=1.0 ]--+--[ r2=1.0 ]--+
   *       |                             |
   * <a> --x (n1)                   (n3) x--[ r4=0.8 ]--+-- <b>
   *       |                             |             (n4)
   *       +----------[ r3=3.0 ]---------+
   */

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  db::Device *r3 = new db::Device (res, "r3");
  r3->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  db::Device *r4 = new db::Device (res, "r4");
  r4->set_parameter_value (db::DeviceClassResistor::param_id_R, 0.8);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin (db::Pin ("A"));
  db::Pin pin_b = circuit->add_pin (db::Pin ("B"));

  circuit->add_device (r1);
  circuit->add_device (r2);
  circuit->add_device (r3);
  circuit->add_device (r4);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);
  r3->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  r1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  r2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n3);
  r3->connect_terminal (db::DeviceClassResistor::terminal_id_B, n3);
  r4->connect_terminal (db::DeviceClassResistor::terminal_id_A, n3);

  db::Net *n4 = new db::Net ("n4");
  circuit->add_net (n4);
  circuit->connect_pin (pin_b.id (), n4);
  r4->connect_terminal (db::DeviceClassResistor::terminal_id_B, n4);

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n4):\n"
    "  D r1 (A=n1,B=n2) [R=1]\n"
    "  D r2 (A=n2,B=n3) [R=1]\n"
    "  D r3 (A=n1,B=n3) [R=3]\n"
    "  D r4 (A=n3,B=n4) [R=0.8]\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "Circuit  (A=n1,B=n4):\n"
    "  D r4 (A=n1,B=n4) [R=2]\n"
  );
};


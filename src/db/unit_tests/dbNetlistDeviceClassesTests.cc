
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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
  r1->set_parameter_value (db::DeviceClassResistor::param_id_L, 6.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_W, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_L, 12.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_W, 2.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

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
    "circuit '' (A=n1,B=n3);\n"
    "  device '' r1 (A=n1,B=n2) (R=1,L=6,W=1,A=5,P=10);\n"
    "  device '' r2 (A=n2,B=n3) (R=3,L=12,W=2,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3);\n"
    "  device '' r1 (A=n1,B=n3) (R=4,L=18,W=1.5,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(2_SerialResistors1Swapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_L, 6.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_W, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_L, 12.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_W, 2.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

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
    "circuit '' (A=n1,B=n3);\n"
    "  device '' r1 (A=n1,B=n2) (R=1,L=6,W=1,A=5,P=10);\n"
    "  device '' r2 (A=n3,B=n2) (R=3,L=12,W=2,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3);\n"
    "  device '' r1 (A=n1,B=n3) (R=4,L=18,W=1.5,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(3_SerialResistors1OtherSwapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_L, 6.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_W, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_L, 12.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_W, 2.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

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
    "circuit '' (A=n1,B=n3);\n"
    "  device '' r1 (A=n2,B=n1) (R=1,L=6,W=1,A=5,P=10);\n"
    "  device '' r2 (A=n2,B=n3) (R=3,L=12,W=2,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3);\n"
    "  device '' r1 (A=n3,B=n1) (R=4,L=18,W=1.5,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(4_SerialResistors2Swapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_L, 6.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_W, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_L, 12.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_W, 2.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

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
    "circuit '' (A=n1,B=n3);\n"
    "  device '' r1 (A=n2,B=n1) (R=1,L=6,W=1,A=5,P=10);\n"
    "  device '' r2 (A=n3,B=n2) (R=3,L=12,W=2,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3);\n"
    "  device '' r1 (A=n3,B=n1) (R=4,L=18,W=1.5,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(5_SerialResistorsNoCombination)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_L, 6.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_W, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_L, 12.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_W, 2.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c = circuit->add_pin ("C");

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
    "circuit '' (A=n1,B=n3,C=n2);\n"
    "  device '' r1 (A=n1,B=n2) (R=1,L=6,W=1,A=5,P=10);\n"
    "  device '' r2 (A=n2,B=n3) (R=3,L=12,W=2,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3,C=n2);\n"
    "  device '' r1 (A=n1,B=n2) (R=1,L=6,W=1,A=5,P=10);\n"
    "  device '' r2 (A=n2,B=n3) (R=3,L=12,W=2,A=1,P=2);\n"
    "end;\n"
  );
}

TEST(6_ParallelResistors)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 2.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_L, 6.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_W, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_L, 12.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_W, 2.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

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
    "circuit '' (A=n1,B=n2);\n"
    "  device '' r1 (A=n1,B=n2) (R=2,L=6,W=1,A=5,P=10);\n"
    "  device '' r2 (A=n1,B=n2) (R=3,L=12,W=2,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' r1 (A=n1,B=n2) (R=1.2,L=9,W=3,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(7_ParallelResistors1Swapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 2.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_L, 6.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_W, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_L, 12.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_W, 2.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

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
    "circuit '' (A=n1,B=n2);\n"
    "  device '' r1 (A=n2,B=n1) (R=2,L=6,W=1,A=5,P=10);\n"
    "  device '' r2 (A=n1,B=n2) (R=3,L=12,W=2,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' r1 (A=n2,B=n1) (R=1.2,L=9,W=3,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(8_ParallelResistors1OtherSwapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 2.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_L, 6.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_W, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_L, 12.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_W, 2.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

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
    "circuit '' (A=n1,B=n2);\n"
    "  device '' r1 (A=n1,B=n2) (R=2,L=6,W=1,A=5,P=10);\n"
    "  device '' r2 (A=n2,B=n1) (R=3,L=12,W=2,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' r1 (A=n1,B=n2) (R=1.2,L=9,W=3,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(9_ParallelResistors2Swapped)
{
  db::DeviceClassResistor *res = new db::DeviceClassResistor ();

  db::Netlist nl;
  nl.add_device_class (res);

  db::Device *r1 = new db::Device (res, "r1");
  r1->set_parameter_value (db::DeviceClassResistor::param_id_R, 2.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_L, 6.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_W, 1.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_L, 12.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_W, 2.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

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
    "circuit '' (A=n1,B=n2);\n"
    "  device '' r1 (A=n2,B=n1) (R=2,L=6,W=1,A=5,P=10);\n"
    "  device '' r2 (A=n2,B=n1) (R=3,L=12,W=2,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' r1 (A=n2,B=n1) (R=1.2,L=9,W=3,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(10_ComplexResistorCombination)
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
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 2.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 3.0);
  db::Device *r2 = new db::Device (res, "r2");
  r2->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 4.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r3 = new db::Device (res, "r3");
  r3->set_parameter_value (db::DeviceClassResistor::param_id_R, 3.0);
  r3->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r3->set_parameter_value (db::DeviceClassResistor::param_id_P, 1.0);
  db::Device *r4 = new db::Device (res, "r4");
  r4->set_parameter_value (db::DeviceClassResistor::param_id_R, 0.8);
  r4->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r4->set_parameter_value (db::DeviceClassResistor::param_id_P, 1.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

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
    "circuit '' (A=n1,B=n4);\n"
    "  device '' r1 (A=n1,B=n2) (R=1,L=0,W=0,A=2,P=3);\n"
    "  device '' r2 (A=n2,B=n3) (R=1,L=0,W=0,A=4,P=10);\n"
    "  device '' r3 (A=n1,B=n3) (R=3,L=0,W=0,A=1,P=1);\n"
    "  device '' r4 (A=n3,B=n4) (R=0.8,L=0,W=0,A=1,P=1);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n4);\n"
    "  device '' r4 (A=n1,B=n4) (R=2,L=0,W=0,A=8,P=15);\n"
    "end;\n"
  );
}

TEST(11_SerialInductors)
{
  db::DeviceClassInductor *ind = new db::DeviceClassInductor ();

  db::Netlist nl;
  nl.add_device_class (ind);

  db::Device *l1 = new db::Device (ind, "l1");
  l1->set_parameter_value (db::DeviceClassInductor::param_id_L, 1.0);
  db::Device *l2 = new db::Device (ind, "l2");
  l2->set_parameter_value (db::DeviceClassInductor::param_id_L, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

  circuit->add_device (l1);
  circuit->add_device (l2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  l1->connect_terminal (db::DeviceClassResistor::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  l1->connect_terminal (db::DeviceClassResistor::terminal_id_B, n2);
  l2->connect_terminal (db::DeviceClassResistor::terminal_id_A, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  l2->connect_terminal (db::DeviceClassResistor::terminal_id_B, n3);
  circuit->connect_pin (pin_b.id (), n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3);\n"
    "  device '' l1 (A=n1,B=n2) (L=1);\n"
    "  device '' l2 (A=n2,B=n3) (L=3);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3);\n"
    "  device '' l1 (A=n1,B=n3) (L=4);\n"
    "end;\n"
  );
}

TEST(12_ParallelInductors)
{
  db::DeviceClassInductor *ind = new db::DeviceClassInductor ();

  db::Netlist nl;
  nl.add_device_class (ind);

  db::Device *l1 = new db::Device (ind, "l1");
  l1->set_parameter_value (db::DeviceClassInductor::param_id_L, 2.0);
  db::Device *l2 = new db::Device (ind, "l2");
  l2->set_parameter_value (db::DeviceClassInductor::param_id_L, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

  circuit->add_device (l1);
  circuit->add_device (l2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  l1->connect_terminal (db::DeviceClassInductor::terminal_id_A, n1);
  l2->connect_terminal (db::DeviceClassInductor::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  l1->connect_terminal (db::DeviceClassInductor::terminal_id_B, n2);
  l2->connect_terminal (db::DeviceClassInductor::terminal_id_B, n2);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' l1 (A=n1,B=n2) (L=2);\n"
    "  device '' l2 (A=n1,B=n2) (L=3);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' l1 (A=n1,B=n2) (L=1.2);\n"
    "end;\n"
  );
}

TEST(13_SerialCapacitors)
{
  db::DeviceClassCapacitor *cap = new db::DeviceClassCapacitor ();

  db::Netlist nl;
  nl.add_device_class (cap);

  db::Device *c1 = new db::Device (cap, "c1");
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 2.0);
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_A, 5.0);
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_P, 10.0);
  db::Device *c2 = new db::Device (cap, "c2");
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 3.0);
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_A, 1.0);
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

  circuit->add_device (c1);
  circuit->add_device (c2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  c1->connect_terminal (db::DeviceClassCapacitor::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  c1->connect_terminal (db::DeviceClassCapacitor::terminal_id_B, n2);
  c2->connect_terminal (db::DeviceClassCapacitor::terminal_id_A, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  c2->connect_terminal (db::DeviceClassCapacitor::terminal_id_B, n3);
  circuit->connect_pin (pin_b.id (), n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3);\n"
    "  device '' c1 (A=n1,B=n2) (C=2,A=5,P=10);\n"
    "  device '' c2 (A=n2,B=n3) (C=3,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3);\n"
    "  device '' c1 (A=n1,B=n3) (C=1.2,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(14_ParallelCapacitors)
{
  db::DeviceClassCapacitor *cap = new db::DeviceClassCapacitor ();

  db::Netlist nl;
  nl.add_device_class (cap);

  db::Device *c1 = new db::Device (cap, "c1");
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 1.0);
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_A, 5.0);
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_P, 10.0);
  db::Device *c2 = new db::Device (cap, "c2");
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 3.0);
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_A, 1.0);
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

  circuit->add_device (c1);
  circuit->add_device (c2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  c1->connect_terminal (db::DeviceClassCapacitor::terminal_id_A, n1);
  c2->connect_terminal (db::DeviceClassCapacitor::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  c1->connect_terminal (db::DeviceClassCapacitor::terminal_id_B, n2);
  c2->connect_terminal (db::DeviceClassCapacitor::terminal_id_B, n2);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' c1 (A=n1,B=n2) (C=1,A=5,P=10);\n"
    "  device '' c2 (A=n1,B=n2) (C=3,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' c1 (A=n1,B=n2) (C=4,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(15_SerialDiodes)
{
  db::DeviceClassDiode *diode = new db::DeviceClassDiode ();

  db::Netlist nl;
  nl.add_device_class (diode);

  db::Device *d1 = new db::Device (diode, "d1");
  d1->set_parameter_value (db::DeviceClassDiode::param_id_A, 2.0);
  d1->set_parameter_value (db::DeviceClassDiode::param_id_P, 5.0);
  db::Device *d2 = new db::Device (diode, "d2");
  d2->set_parameter_value (db::DeviceClassDiode::param_id_A, 3.0);
  d2->set_parameter_value (db::DeviceClassDiode::param_id_P, 1.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassDiode::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  d1->connect_terminal (db::DeviceClassDiode::terminal_id_C, n2);
  d2->connect_terminal (db::DeviceClassDiode::terminal_id_A, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  d2->connect_terminal (db::DeviceClassDiode::terminal_id_C, n3);
  circuit->connect_pin (pin_b.id (), n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3);\n"
    "  device '' d1 (A=n1,C=n2) (A=2,P=5);\n"
    "  device '' d2 (A=n2,C=n3) (A=3,P=1);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  //  serial diodes are not combined!

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3);\n"
    "  device '' d1 (A=n1,C=n2) (A=2,P=5);\n"
    "  device '' d2 (A=n2,C=n3) (A=3,P=1);\n"
    "end;\n"
  );
}

TEST(16_ParallelDiodes)
{
  db::DeviceClassDiode *diode = new db::DeviceClassDiode ();

  db::Netlist nl;
  nl.add_device_class (diode);

  db::Device *d1 = new db::Device (diode, "d1");
  d1->set_parameter_value (db::DeviceClassDiode::param_id_A, 1.0);
  d1->set_parameter_value (db::DeviceClassDiode::param_id_P, 5.0);
  db::Device *d2 = new db::Device (diode, "d2");
  d2->set_parameter_value (db::DeviceClassDiode::param_id_A, 3.0);
  d2->set_parameter_value (db::DeviceClassDiode::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassDiode::terminal_id_A, n1);
  d2->connect_terminal (db::DeviceClassDiode::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassDiode::terminal_id_C, n2);
  d2->connect_terminal (db::DeviceClassDiode::terminal_id_C, n2);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' d1 (A=n1,C=n2) (A=1,P=5);\n"
    "  device '' d2 (A=n1,C=n2) (A=3,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' d1 (A=n1,C=n2) (A=4,P=7);\n"
    "end;\n"
  );
}

TEST(17_AntiParallelDiodes)
{
  db::DeviceClassDiode *diode = new db::DeviceClassDiode ();

  db::Netlist nl;
  nl.add_device_class (diode);

  db::Device *d1 = new db::Device (diode, "d1");
  d1->set_parameter_value (db::DeviceClassDiode::param_id_A, 1.0);
  d1->set_parameter_value (db::DeviceClassDiode::param_id_P, 5.0);
  db::Device *d2 = new db::Device (diode, "d2");
  d2->set_parameter_value (db::DeviceClassDiode::param_id_A, 3.0);
  d2->set_parameter_value (db::DeviceClassDiode::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassDiode::terminal_id_A, n1);
  d2->connect_terminal (db::DeviceClassDiode::terminal_id_C, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassDiode::terminal_id_C, n2);
  d2->connect_terminal (db::DeviceClassDiode::terminal_id_A, n2);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' d1 (A=n1,C=n2) (A=1,P=5);\n"
    "  device '' d2 (A=n2,C=n1) (A=3,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  //  anti-parallel diodes are not combined

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2);\n"
    "  device '' d1 (A=n1,C=n2) (A=1,P=5);\n"
    "  device '' d2 (A=n2,C=n1) (A=3,P=2);\n"
    "end;\n"
  );
}

TEST(20_ParallelMOS3Transistors)
{
  db::DeviceClassMOS3Transistor *cls = new db::DeviceClassMOS3Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.5);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 2.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 3.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 12.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 13.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.5);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 2.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 3.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 4.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 13.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 14.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c = circuit->add_pin ("C");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  circuit->connect_pin (pin_c.id (), n3);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, n3);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3);\n"
    "  device '' d1 (S=n1,G=n3,D=n2) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n3,D=n2) (L=0.5,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3);\n"
    "  device '' d1 (S=n1,G=n3,D=n2) (L=0.5,W=3,AS=5,AD=7,PS=25,PD=27);\n"
    "end;\n"
  );
}

TEST(21_AntiParallelMOS3Transistors)
{
  db::DeviceClassMOS3Transistor *cls = new db::DeviceClassMOS3Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.5);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 2.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 3.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 12.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 13.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.5);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 2.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 3.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 4.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 13.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 14.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c = circuit->add_pin ("C");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  circuit->connect_pin (pin_c.id (), n3);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, n3);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3);\n"
    "  device '' d1 (S=n1,G=n3,D=n2) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n2,G=n3,D=n1) (L=0.5,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3);\n"
    "  device '' d1 (S=n1,G=n3,D=n2) (L=0.5,W=3,AS=5,AD=7,PS=25,PD=27);\n"
    "end;\n"
  );
}

TEST(22_ParallelMOS3TransistorsDisconnectedGates)
{
  db::DeviceClassMOS3Transistor *cls = new db::DeviceClassMOS3Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.5);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 2.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 3.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 12.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 13.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.5);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 2.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 3.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 4.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 13.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 14.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c1 = circuit->add_pin ("C1");
  db::Pin pin_c2 = circuit->add_pin ("C2");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  circuit->connect_pin (pin_c1.id (), n3);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, n3);

  db::Net *n4 = new db::Net ("n4");
  circuit->add_net (n4);
  circuit->connect_pin (pin_c2.id (), n4);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, n4);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C1=n3,C2=n4);\n"
    "  device '' d1 (S=n1,G=n3,D=n2) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n4,D=n2) (L=0.5,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  //  because of the disconnected gates, devices will no be joined:

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C1=n3,C2=n4);\n"
    "  device '' d1 (S=n1,G=n3,D=n2) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n4,D=n2) (L=0.5,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );
}

TEST(23_ParallelMOS3TransistorsDifferentLength)
{
  db::DeviceClassMOS3Transistor *cls = new db::DeviceClassMOS3Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.5);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 2.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 3.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 12.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 13.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.75);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 2.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 3.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 4.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 13.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 14.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c = circuit->add_pin ("C");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  circuit->connect_pin (pin_c.id (), n3);
  d1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, n3);
  d2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3);\n"
    "  device '' d1 (S=n1,G=n3,D=n2) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n3,D=n2) (L=0.75,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  //  because of different length, the devices will not be combined:

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3);\n"
    "  device '' d1 (S=n1,G=n3,D=n2) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n3,D=n2) (L=0.75,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );
}

TEST(30_ParallelMOS4Transistors)
{
  db::DeviceClassMOS4Transistor *cls = new db::DeviceClassMOS4Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_L, 0.5);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_W, 1.0);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AS, 2.0);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AD, 3.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 12.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 13.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_L, 0.5);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_W, 2.0);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AS, 3.0);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AD, 4.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 13.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 14.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c = circuit->add_pin ("C");
  db::Pin pin_d = circuit->add_pin ("D");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, n1);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, n2);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  circuit->connect_pin (pin_c.id (), n3);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, n3);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, n3);

  db::Net *n0 = new db::Net ("n0");
  circuit->add_net (n0);
  circuit->connect_pin (pin_d.id (), n0);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n0);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n0);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D=n0);\n"
    "  device '' d1 (S=n1,G=n3,D=n2,B=n0) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n3,D=n2,B=n0) (L=0.5,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D=n0);\n"
    "  device '' d1 (S=n1,G=n3,D=n2,B=n0) (L=0.5,W=3,AS=5,AD=7,PS=25,PD=27);\n"
    "end;\n"
  );
}

TEST(31_AntiParallelMOS4Transistors)
{
  db::DeviceClassMOS4Transistor *cls = new db::DeviceClassMOS4Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_L, 0.5);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_W, 1.0);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AS, 2.0);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AD, 3.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 12.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 13.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_L, 0.5);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_W, 2.0);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AS, 3.0);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AD, 4.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 13.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 14.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c = circuit->add_pin ("C");
  db::Pin pin_d = circuit->add_pin ("D");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, n1);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, n2);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  circuit->connect_pin (pin_c.id (), n3);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, n3);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, n3);

  db::Net *n0 = new db::Net ("n0");
  circuit->add_net (n0);
  circuit->connect_pin (pin_d.id (), n0);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n0);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n0);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D=n0);\n"
    "  device '' d1 (S=n1,G=n3,D=n2,B=n0) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n2,G=n3,D=n1,B=n0) (L=0.5,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D=n0);\n"
    "  device '' d1 (S=n1,G=n3,D=n2,B=n0) (L=0.5,W=3,AS=5,AD=7,PS=25,PD=27);\n"
    "end;\n"
  );
}

TEST(32_ParallelMOS4TransistorsDisconnectedGates)
{
  db::DeviceClassMOS4Transistor *cls = new db::DeviceClassMOS4Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_L, 0.5);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_W, 1.0);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AS, 2.0);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AD, 3.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 12.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 13.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_L, 0.5);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_W, 2.0);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AS, 3.0);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AD, 4.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 13.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 14.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c1 = circuit->add_pin ("C1");
  db::Pin pin_c2 = circuit->add_pin ("C2");
  db::Pin pin_d = circuit->add_pin ("D");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, n1);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, n2);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, n2);

  db::Net *n3a = new db::Net ("n3a");
  circuit->add_net (n3a);
  circuit->connect_pin (pin_c1.id (), n3a);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, n3a);

  db::Net *n3b = new db::Net ("n3b");
  circuit->add_net (n3b);
  circuit->connect_pin (pin_c2.id (), n3b);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, n3b);

  db::Net *n0 = new db::Net ("n0");
  circuit->add_net (n0);
  circuit->connect_pin (pin_d.id (), n0);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n0);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n0);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C1=n3a,C2=n3b,D=n0);\n"
    "  device '' d1 (S=n1,G=n3a,D=n2,B=n0) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n3b,D=n2,B=n0) (L=0.5,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  //  not combined because gate is different:

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C1=n3a,C2=n3b,D=n0);\n"
    "  device '' d1 (S=n1,G=n3a,D=n2,B=n0) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n3b,D=n2,B=n0) (L=0.5,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );
}

TEST(33_ParallelMOS4TransistorsDisconnectedBulk)
{
  db::DeviceClassMOS4Transistor *cls = new db::DeviceClassMOS4Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_L, 0.5);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_W, 1.0);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AS, 2.0);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AD, 3.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 12.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 13.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_L, 0.5);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_W, 2.0);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AS, 3.0);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AD, 4.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 13.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 14.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c = circuit->add_pin ("C");
  db::Pin pin_d1 = circuit->add_pin ("D1");
  db::Pin pin_d2 = circuit->add_pin ("D2");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, n1);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, n2);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  circuit->connect_pin (pin_c.id (), n3);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, n3);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, n3);

  db::Net *n0a = new db::Net ("n0a");
  circuit->add_net (n0a);
  circuit->connect_pin (pin_d1.id (), n0a);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n0a);

  db::Net *n0b = new db::Net ("n0b");
  circuit->add_net (n0b);
  circuit->connect_pin (pin_d2.id (), n0b);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n0b);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D1=n0a,D2=n0b);\n"
    "  device '' d1 (S=n1,G=n3,D=n2,B=n0a) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n3,D=n2,B=n0b) (L=0.5,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );

  //  not combined because bulk is different:

  nl.combine_devices ();
  nl.purge ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D1=n0a,D2=n0b);\n"
    "  device '' d1 (S=n1,G=n3,D=n2,B=n0a) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n3,D=n2,B=n0b) (L=0.5,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );
}

TEST(34_ParallelMOS4TransistorsDifferentLength)
{
  db::DeviceClassMOS4Transistor *cls = new db::DeviceClassMOS4Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_L, 0.5);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_W, 1.0);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AS, 2.0);
  d1->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AD, 3.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 12.0);
  d1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 13.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_L, 0.75);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_W, 2.0);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AS, 3.0);
  d2->set_parameter_value (db::DeviceClassMOS4Transistor::param_id_AD, 4.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 13.0);
  d2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 14.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c = circuit->add_pin ("C");
  db::Pin pin_d = circuit->add_pin ("D");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, n1);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, n2);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  circuit->connect_pin (pin_c.id (), n3);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, n3);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, n3);

  db::Net *n0 = new db::Net ("n0");
  circuit->add_net (n0);
  circuit->connect_pin (pin_d.id (), n0);
  d1->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n0);
  d2->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n0);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D=n0);\n"
    "  device '' d1 (S=n1,G=n3,D=n2,B=n0) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n3,D=n2,B=n0) (L=0.75,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );

  nl.combine_devices ();
  nl.purge ();

  //  not combined because length is different:

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D=n0);\n"
    "  device '' d1 (S=n1,G=n3,D=n2,B=n0) (L=0.5,W=1,AS=2,AD=3,PS=12,PD=13);\n"
    "  device '' d2 (S=n1,G=n3,D=n2,B=n0) (L=0.75,W=2,AS=3,AD=4,PS=13,PD=14);\n"
    "end;\n"
  );
}

TEST(35_SerialCapacitorsWithBulk)
{
  db::DeviceClassCapacitorWithBulk *cap = new db::DeviceClassCapacitorWithBulk ();

  db::Netlist nl;
  nl.add_device_class (cap);

  db::Device *c1 = new db::Device (cap, "c1");
  c1->set_parameter_value (db::DeviceClassCapacitorWithBulk::param_id_C, 2.0);
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_A, 5.0);
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_P, 10.0);
  db::Device *c2 = new db::Device (cap, "c2");
  c2->set_parameter_value (db::DeviceClassCapacitorWithBulk::param_id_C, 3.0);
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_A, 1.0);
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_bulk = circuit->add_pin ("BULK");

  circuit->add_device (c1);
  circuit->add_device (c2);

  db::Net *nb = new db::Net ("nb");
  circuit->add_net (nb);
  circuit->connect_pin (pin_bulk.id (), nb);
  c1->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_W, nb);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  c1->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  c1->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_B, n2);
  c2->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_A, n2);
  c2->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_W, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  c2->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_B, n3);
  circuit->connect_pin (pin_b.id (), n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3,BULK=nb);\n"
    "  device '' c1 (A=n1,B=n2,W=nb) (C=2,A=5,P=10);\n"
    "  device '' c2 (A=n2,B=n3,W=n2) (C=3,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();

  //  no combination because bulk terminals are connected differently
  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3,BULK=nb);\n"
    "  device '' c1 (A=n1,B=n2,W=nb) (C=2,A=5,P=10);\n"
    "  device '' c2 (A=n2,B=n3,W=n2) (C=3,A=1,P=2);\n"
    "end;\n"
  );

  c2->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_W, nb);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3,BULK=nb);\n"
    "  device '' c1 (A=n1,B=n2,W=nb) (C=2,A=5,P=10);\n"
    "  device '' c2 (A=n2,B=n3,W=nb) (C=3,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3,BULK=nb);\n"
    "  device '' c1 (A=n1,B=n3,W=nb) (C=1.2,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(36_ParallelCapacitorsWithBulk)
{
  db::DeviceClassCapacitorWithBulk *cap = new db::DeviceClassCapacitorWithBulk ();

  db::Netlist nl;
  nl.add_device_class (cap);

  db::Device *c1 = new db::Device (cap, "c1");
  c1->set_parameter_value (db::DeviceClassCapacitorWithBulk::param_id_C, 1.0);
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_A, 5.0);
  c1->set_parameter_value (db::DeviceClassCapacitor::param_id_P, 10.0);
  db::Device *c2 = new db::Device (cap, "c2");
  c2->set_parameter_value (db::DeviceClassCapacitorWithBulk::param_id_C, 3.0);
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_A, 1.0);
  c2->set_parameter_value (db::DeviceClassCapacitor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_bulk = circuit->add_pin ("BULK");

  circuit->add_device (c1);
  circuit->add_device (c2);

  db::Net *nb = new db::Net ("nb");
  circuit->add_net (nb);
  circuit->connect_pin (pin_bulk.id (), nb);
  c1->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_W, nb);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  c1->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_A, n1);
  c2->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  c1->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_B, n2);
  c2->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_B, n2);
  c2->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_W, n2);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,BULK=nb);\n"
    "  device '' c1 (A=n1,B=n2,W=nb) (C=1,A=5,P=10);\n"
    "  device '' c2 (A=n1,B=n2,W=n2) (C=3,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();

  //  devices are not combined as the bulk terminals are connected differently
  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,BULK=nb);\n"
    "  device '' c1 (A=n1,B=n2,W=nb) (C=1,A=5,P=10);\n"
    "  device '' c2 (A=n1,B=n2,W=n2) (C=3,A=1,P=2);\n"
    "end;\n"
  );

  c2->connect_terminal (db::DeviceClassCapacitorWithBulk::terminal_id_W, nb);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,BULK=nb);\n"
    "  device '' c1 (A=n1,B=n2,W=nb) (C=1,A=5,P=10);\n"
    "  device '' c2 (A=n1,B=n2,W=nb) (C=3,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,BULK=nb);\n"
    "  device '' c1 (A=n1,B=n2,W=nb) (C=4,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(37_SerialResistorsWithBulk)
{
  db::DeviceClassResistorWithBulk *cap = new db::DeviceClassResistorWithBulk ();

  db::Netlist nl;
  nl.add_device_class (cap);

  db::Device *r1 = new db::Device (cap, "r1");
  r1->set_parameter_value (db::DeviceClassResistorWithBulk::param_id_R, 2.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (cap, "r2");
  r2->set_parameter_value (db::DeviceClassResistorWithBulk::param_id_R, 0.5);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_bulk = circuit->add_pin ("BULK");

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *nb = new db::Net ("nb");
  circuit->add_net (nb);
  circuit->connect_pin (pin_bulk.id (), nb);
  r1->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_W, nb);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  r1->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_B, n2);
  r2->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_A, n2);
  r2->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_W, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  r2->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_B, n3);
  circuit->connect_pin (pin_b.id (), n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3,BULK=nb);\n"
    "  device '' r1 (A=n1,B=n2,W=nb) (R=2,L=0,W=0,A=5,P=10);\n"
    "  device '' r2 (A=n2,B=n3,W=n2) (R=0.5,L=0,W=0,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();

  //  no combination because bulk terminals are connected differently
  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3,BULK=nb);\n"
    "  device '' r1 (A=n1,B=n2,W=nb) (R=2,L=0,W=0,A=5,P=10);\n"
    "  device '' r2 (A=n2,B=n3,W=n2) (R=0.5,L=0,W=0,A=1,P=2);\n"
    "end;\n"
  );

  r2->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_W, nb);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3,BULK=nb);\n"
    "  device '' r1 (A=n1,B=n2,W=nb) (R=2,L=0,W=0,A=5,P=10);\n"
    "  device '' r2 (A=n2,B=n3,W=nb) (R=0.5,L=0,W=0,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n3,BULK=nb);\n"
    "  device '' r1 (A=n1,B=n3,W=nb) (R=2.5,L=0,W=0,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(38_ParallelResistorsWithBulk)
{
  db::DeviceClassResistorWithBulk *cap = new db::DeviceClassResistorWithBulk ();

  db::Netlist nl;
  nl.add_device_class (cap);

  db::Device *r1 = new db::Device (cap, "r1");
  r1->set_parameter_value (db::DeviceClassResistorWithBulk::param_id_R, 2.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_A, 5.0);
  r1->set_parameter_value (db::DeviceClassResistor::param_id_P, 10.0);
  db::Device *r2 = new db::Device (cap, "r2");
  r2->set_parameter_value (db::DeviceClassResistorWithBulk::param_id_R, 0.5);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_A, 1.0);
  r2->set_parameter_value (db::DeviceClassResistor::param_id_P, 2.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_bulk = circuit->add_pin ("BULK");

  circuit->add_device (r1);
  circuit->add_device (r2);

  db::Net *nb = new db::Net ("nb");
  circuit->add_net (nb);
  circuit->connect_pin (pin_bulk.id (), nb);
  r1->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_W, nb);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  r1->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_A, n1);
  r2->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_A, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  r1->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_B, n2);
  r2->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_B, n2);
  r2->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_W, n2);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,BULK=nb);\n"
    "  device '' r1 (A=n1,B=n2,W=nb) (R=2,L=0,W=0,A=5,P=10);\n"
    "  device '' r2 (A=n1,B=n2,W=n2) (R=0.5,L=0,W=0,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();

  //  devices are not combined as the bulk terminals are connected differently
  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,BULK=nb);\n"
    "  device '' r1 (A=n1,B=n2,W=nb) (R=2,L=0,W=0,A=5,P=10);\n"
    "  device '' r2 (A=n1,B=n2,W=n2) (R=0.5,L=0,W=0,A=1,P=2);\n"
    "end;\n"
  );

  r2->connect_terminal (db::DeviceClassResistorWithBulk::terminal_id_W, nb);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,BULK=nb);\n"
    "  device '' r1 (A=n1,B=n2,W=nb) (R=2,L=0,W=0,A=5,P=10);\n"
    "  device '' r2 (A=n1,B=n2,W=nb) (R=0.5,L=0,W=0,A=1,P=2);\n"
    "end;\n"
  );

  nl.combine_devices ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,BULK=nb);\n"
    "  device '' r1 (A=n1,B=n2,W=nb) (R=0.4,L=0,W=0,A=6,P=12);\n"
    "end;\n"
  );
}

TEST(39_ParallelBJT3Transistors)
{
  db::DeviceClassBJT3Transistor *cls = new db::DeviceClassBJT3Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AE, 2.0);
  d1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PE, 12.0);
  d1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AB, 3.0);
  d1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PB, 13.0);
  d1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AC, 4.0);
  d1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PC, 14.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AE, 3.0);
  d2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PE, 13.0);
  d2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AB, 4.0);
  d2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PB, 14.0);
  d2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AC, 5.0);
  d2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PC, 15.0);
  d2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_NE, 4.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c = circuit->add_pin ("C");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassBJT3Transistor::terminal_id_C, n1);
  d2->connect_terminal (db::DeviceClassBJT3Transistor::terminal_id_C, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassBJT3Transistor::terminal_id_B, n2);
  d2->connect_terminal (db::DeviceClassBJT3Transistor::terminal_id_B, n2);
  d2->connect_terminal (db::DeviceClassBJT3Transistor::terminal_id_E, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  circuit->connect_pin (pin_c.id (), n3);
  d1->connect_terminal (db::DeviceClassBJT3Transistor::terminal_id_E, n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3);\n"
    "  device '' d1 (C=n1,B=n2,E=n3) (AE=2,PE=12,AB=3,PB=13,AC=4,PC=14,NE=1);\n"
    "  device '' d2 (C=n1,B=n2,E=n2) (AE=3,PE=13,AB=4,PB=14,AC=5,PC=15,NE=4);\n"
    "end;\n"
  );

  nl.combine_devices ();

  //  no combination as emitters are connected differently
  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3);\n"
    "  device '' d1 (C=n1,B=n2,E=n3) (AE=2,PE=12,AB=3,PB=13,AC=4,PC=14,NE=1);\n"
    "  device '' d2 (C=n1,B=n2,E=n2) (AE=3,PE=13,AB=4,PB=14,AC=5,PC=15,NE=4);\n"
    "end;\n"
  );

  d2->connect_terminal (db::DeviceClassBJT3Transistor::terminal_id_E, n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3);\n"
    "  device '' d1 (C=n1,B=n2,E=n3) (AE=2,PE=12,AB=3,PB=13,AC=4,PC=14,NE=1);\n"
    "  device '' d2 (C=n1,B=n2,E=n3) (AE=3,PE=13,AB=4,PB=14,AC=5,PC=15,NE=4);\n"
    "end;\n"
  );

  nl.combine_devices ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3);\n"
    "  device '' d1 (C=n1,B=n2,E=n3) (AE=5,PE=25,AB=3,PB=13,AC=4,PC=14,NE=5);\n"
    "end;\n"
  );
}

TEST(40_ParallelBJT4Transistors)
{
  db::DeviceClassBJT4Transistor *cls = new db::DeviceClassBJT4Transistor ();

  db::Netlist nl;
  nl.add_device_class (cls);

  db::Device *d1 = new db::Device (cls, "d1");
  d1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AE, 2.0);
  d1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PE, 12.0);
  d1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AB, 3.0);
  d1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PB, 13.0);
  d1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AC, 4.0);
  d1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PC, 14.0);
  d1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_NE, 2.0);
  db::Device *d2 = new db::Device (cls, "d2");
  d2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AE, 3.0);
  d2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PE, 13.0);
  d2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AB, 4.0);
  d2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PB, 14.0);
  d2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AC, 5.0);
  d2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PC, 15.0);
  d2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_NE, 3.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_c = circuit->add_pin ("C");
  db::Pin pin_d = circuit->add_pin ("D");

  circuit->add_device (d1);
  circuit->add_device (d2);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  d1->connect_terminal (db::DeviceClassBJT4Transistor::terminal_id_C, n1);
  d2->connect_terminal (db::DeviceClassBJT4Transistor::terminal_id_C, n1);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  d1->connect_terminal (db::DeviceClassBJT4Transistor::terminal_id_B, n2);
  d2->connect_terminal (db::DeviceClassBJT4Transistor::terminal_id_B, n2);
  d2->connect_terminal (db::DeviceClassBJT4Transistor::terminal_id_E, n2);

  db::Net *n3 = new db::Net ("n3");
  circuit->add_net (n3);
  circuit->connect_pin (pin_c.id (), n3);
  d1->connect_terminal (db::DeviceClassBJT4Transistor::terminal_id_E, n3);

  db::Net *n4 = new db::Net ("n4");
  circuit->add_net (n4);
  circuit->connect_pin (pin_d.id (), n4);
  d1->connect_terminal (db::DeviceClassBJT4Transistor::terminal_id_S, n4);
  d2->connect_terminal (db::DeviceClassBJT4Transistor::terminal_id_S, n4);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D=n4);\n"
    "  device '' d1 (C=n1,B=n2,E=n3,S=n4) (AE=2,PE=12,AB=3,PB=13,AC=4,PC=14,NE=2);\n"
    "  device '' d2 (C=n1,B=n2,E=n2,S=n4) (AE=3,PE=13,AB=4,PB=14,AC=5,PC=15,NE=3);\n"
    "end;\n"
  );

  nl.combine_devices ();

  //  no combination as emitters are connected differently
  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D=n4);\n"
    "  device '' d1 (C=n1,B=n2,E=n3,S=n4) (AE=2,PE=12,AB=3,PB=13,AC=4,PC=14,NE=2);\n"
    "  device '' d2 (C=n1,B=n2,E=n2,S=n4) (AE=3,PE=13,AB=4,PB=14,AC=5,PC=15,NE=3);\n"
    "end;\n"
  );

  d2->connect_terminal (db::DeviceClassBJT4Transistor::terminal_id_E, n3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D=n4);\n"
    "  device '' d1 (C=n1,B=n2,E=n3,S=n4) (AE=2,PE=12,AB=3,PB=13,AC=4,PC=14,NE=2);\n"
    "  device '' d2 (C=n1,B=n2,E=n3,S=n4) (AE=3,PE=13,AB=4,PB=14,AC=5,PC=15,NE=3);\n"
    "end;\n"
  );

  nl.combine_devices ();

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,C=n3,D=n4);\n"
    "  device '' d1 (C=n1,B=n2,E=n3,S=n4) (AE=5,PE=25,AB=3,PB=13,AC=4,PC=14,NE=5);\n"
    "end;\n"
  );
}

TEST(50_SplitGatesSimple)
{
  db::DeviceClassMOS3Transistor *mos = new db::DeviceClassMOS3Transistor ();

  db::Netlist nl;
  nl.add_device_class (mos);

  db::Device *m11 = new db::Device (mos, "m11");
  m11->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m11->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 6.0);
  db::Device *m21 = new db::Device (mos, "m21");
  m21->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m21->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 10.0);
  db::Device *m12 = new db::Device (mos, "m12");
  m12->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m12->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 6.0);
  db::Device *m22 = new db::Device (mos, "m22");
  m22->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m22->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 10.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_g1 = circuit->add_pin ("G1");
  db::Pin pin_g2 = circuit->add_pin ("G2");

  circuit->add_device (m11);
  circuit->add_device (m21);
  circuit->add_device (m12);
  circuit->add_device (m22);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);
  m12->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);

  db::Net *sd1 = new db::Net ("sd1");
  circuit->add_net (sd1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, sd1);
  m21->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd1);

  db::Net *sd2 = new db::Net ("sd2");
  circuit->add_net (sd2);
  m12->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, sd2);
  m22->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd2);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  m21->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);
  m22->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);

  db::Net *g1 = new db::Net ("g1");
  circuit->add_net (g1);
  circuit->connect_pin (pin_g1.id (), g1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g1);
  m12->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g1);

  db::Net *g2 = new db::Net ("g2");
  circuit->add_net (g2);
  circuit->connect_pin (pin_g2.id (), g2);
  m21->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g2);
  m22->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g2);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=n1,G=g1,D=sd1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  std::unique_ptr<db::Netlist> nl2;
  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=n1,G=g1,D='sd1,sd2') (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S='sd1,sd2',G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    //  sd2 -> sd1
    "  device '' m12 (S=n1,G=g1,D='sd1,sd2') (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S='sd1,sd2',G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  //  can switch S/D for non-strict devices ...

  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd1);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=sd1,G=g1,D=n1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    //  sd1 -> sd2
    "  device '' m11 (S='sd1,sd2',G=g1,D=n1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S='sd1,sd2',G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D='sd1,sd2') (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S='sd1,sd2',G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  //  different lengths disable split_gate ...

  m11->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 7);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=sd1,G=g1,D=n1) (L=7,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=sd1,G=g1,D=n1) (L=7,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  m11->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 6);

  //  split_gates is not bothered by parallel chain

  db::Device *mp1 = new db::Device (mos, "mp1");
  circuit->add_device (mp1);
  mp1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  mp1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 6.0);

  db::Device *mp2 = new db::Device (mos, "mp2");
  circuit->add_device (mp2);
  mp2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  mp2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 10.0);

  mp1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);
  mp1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g1);
  mp2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);
  mp2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g1);  //  NOTE: not g2!

  db::Net *sd3 = new db::Net ("sd3");
  circuit->add_net (sd3);
  mp1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, sd3);
  mp2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=sd1,G=g1,D=n1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp1 (S=n1,G=g1,D=sd3) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp2 (S=sd3,G=g1,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    //  sd1 -> sd2
    "  device '' m11 (S='sd1,sd2',G=g1,D=n1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S='sd1,sd2',G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D='sd1,sd2') (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S='sd1,sd2',G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp1 (S=n1,G=g1,D=sd3) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp2 (S=sd3,G=g1,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  //  different device class can't be split-gated

  db::DeviceClassMOS3Transistor *mos2 = new db::DeviceClassMOS3Transistor ();
  mos2->set_name ("X");
  nl.add_device_class (mos2);
  m11->set_device_class (mos2);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device X m11 (S=sd1,G=g1,D=n1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp1 (S=n1,G=g1,D=sd3) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp2 (S=sd3,G=g1,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device X m11 (S=sd1,G=g1,D=n1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp1 (S=n1,G=g1,D=sd3) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp2 (S=sd3,G=g1,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );
}

TEST(51_SplitGatesStrict)
{
  db::DeviceClassMOS3Transistor *mos = new db::DeviceClassMOS3Transistor ();
  mos->set_strict (true);

  db::Netlist nl;
  nl.add_device_class (mos);

  db::Device *m11 = new db::Device (mos, "m11");
  m11->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m11->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 6.0);
  db::Device *m21 = new db::Device (mos, "m21");
  m21->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m21->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 10.0);
  db::Device *m12 = new db::Device (mos, "m12");
  m12->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m12->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 6.0);
  db::Device *m22 = new db::Device (mos, "m22");
  m22->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m22->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 10.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_g1 = circuit->add_pin ("G1");
  db::Pin pin_g2 = circuit->add_pin ("G2");

  circuit->add_device (m11);
  circuit->add_device (m21);
  circuit->add_device (m12);
  circuit->add_device (m22);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);
  m12->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);

  db::Net *sd1 = new db::Net ("sd1");
  circuit->add_net (sd1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, sd1);
  m21->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd1);

  db::Net *sd2 = new db::Net ("sd2");
  circuit->add_net (sd2);
  m12->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, sd2);
  m22->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd2);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  m21->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);
  m22->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);

  db::Net *g1 = new db::Net ("g1");
  circuit->add_net (g1);
  circuit->connect_pin (pin_g1.id (), g1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g1);
  m12->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g1);

  db::Net *g2 = new db::Net ("g2");
  circuit->add_net (g2);
  circuit->connect_pin (pin_g2.id (), g2);
  m21->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g2);
  m22->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g2);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=n1,G=g1,D=sd1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  std::unique_ptr<db::Netlist> nl2;
  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=n1,G=g1,D='sd1,sd2') (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S='sd1,sd2',G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    //  sd2 -> sd1
    "  device '' m12 (S=n1,G=g1,D='sd1,sd2') (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S='sd1,sd2',G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  //  cannot switch S/D for non-strict devices ...

  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd1);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=sd1,G=g1,D=n1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    //  no change!
    "  device '' m11 (S=sd1,G=g1,D=n1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, sd1);

  //  different lengths disable split_gate ...

  m11->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 7);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=n1,G=g1,D=sd1) (L=7,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=n1,G=g1,D=sd1) (L=7,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  m11->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 6);

  //  split_gates is not bothered by parallel chain

  db::Device *mp1 = new db::Device (mos, "mp1");
  circuit->add_device (mp1);
  mp1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  mp1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 6.0);

  db::Device *mp2 = new db::Device (mos, "mp2");
  circuit->add_device (mp2);
  mp2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  mp2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 10.0);

  mp1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);
  mp1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g1);
  mp2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);
  mp2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g1);  //  NOTE: not g2!

  db::Net *sd3 = new db::Net ("sd3");
  circuit->add_net (sd3);
  mp1->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, sd3);
  mp2->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd3);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    "  device '' m11 (S=n1,G=g1,D=sd1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp1 (S=n1,G=g1,D=sd3) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp2 (S=sd3,G=g1,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2);\n"
    //  sd1 -> sd2
    "  device '' m11 (S=n1,G=g1,D='sd1,sd2') (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S='sd1,sd2',G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D='sd1,sd2') (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S='sd1,sd2',G=g2,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp1 (S=n1,G=g1,D=sd3) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' mp2 (S=sd3,G=g1,D=n2) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );
}

TEST(52_SplitGatesMOS4)
{
  db::DeviceClassMOS4Transistor *mos = new db::DeviceClassMOS4Transistor ();

  db::Netlist nl;
  nl.add_device_class (mos);

  db::Device *m11 = new db::Device (mos, "m11");
  m11->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m11->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 6.0);
  db::Device *m21 = new db::Device (mos, "m21");
  m21->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m21->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 10.0);
  db::Device *m12 = new db::Device (mos, "m12");
  m12->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m12->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 6.0);
  db::Device *m22 = new db::Device (mos, "m22");
  m22->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 1.0);
  m22->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 10.0);

  db::Circuit *circuit = new db::Circuit ();
  nl.add_circuit (circuit);

  db::Pin pin_a = circuit->add_pin ("A");
  db::Pin pin_b = circuit->add_pin ("B");
  db::Pin pin_g1 = circuit->add_pin ("G1");
  db::Pin pin_g2 = circuit->add_pin ("G2");
  db::Pin pin_vss = circuit->add_pin ("VSS");

  circuit->add_device (m11);
  circuit->add_device (m21);
  circuit->add_device (m12);
  circuit->add_device (m22);

  db::Net *vss = new db::Net ("vss");
  circuit->add_net (vss);
  circuit->connect_pin (pin_vss.id (), vss);
  m11->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, vss);
  m12->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, vss);
  m21->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, vss);
  m22->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, vss);

  db::Net *n1 = new db::Net ("n1");
  circuit->add_net (n1);
  circuit->connect_pin (pin_a.id (), n1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);
  m12->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, n1);

  db::Net *sd1 = new db::Net ("sd1");
  circuit->add_net (sd1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, sd1);
  m21->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd1);

  db::Net *sd2 = new db::Net ("sd2");
  circuit->add_net (sd2);
  m12->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, sd2);
  m22->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd2);

  db::Net *n2 = new db::Net ("n2");
  circuit->add_net (n2);
  circuit->connect_pin (pin_b.id (), n2);
  m21->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);
  m22->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n2);

  db::Net *g1 = new db::Net ("g1");
  circuit->add_net (g1);
  circuit->connect_pin (pin_g1.id (), g1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g1);
  m12->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g1);

  db::Net *g2 = new db::Net ("g2");
  circuit->add_net (g2);
  circuit->connect_pin (pin_g2.id (), g2);
  m21->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g2);
  m22->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_G, g2);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2,VSS=vss);\n"
    "  device '' m11 (S=n1,G=g1,D=sd1,B=vss) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2,B=vss) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  std::unique_ptr<db::Netlist> nl2;
  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2,VSS=vss);\n"
    "  device '' m11 (S=n1,G=g1,D='sd1,sd2',B=vss) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S='sd1,sd2',G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    //  sd2 -> sd1
    "  device '' m12 (S=n1,G=g1,D='sd1,sd2',B=vss) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S='sd1,sd2',G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  //  can switch S/D for non-strict devices ...

  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_D, n1);
  m11->connect_terminal (db::DeviceClassMOS3Transistor::terminal_id_S, sd1);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2,VSS=vss);\n"
    "  device '' m11 (S=sd1,G=g1,D=n1,B=vss) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2,B=vss) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2,VSS=vss);\n"
    //  sd1 -> sd2
    "  device '' m11 (S='sd1,sd2',G=g1,D=n1,B=vss) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S='sd1,sd2',G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D='sd1,sd2',B=vss) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S='sd1,sd2',G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  //  different bulk pins disable split_gates ...

  m11->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, n1);

  EXPECT_EQ (nl.to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2,VSS=vss);\n"
    "  device '' m11 (S=sd1,G=g1,D=n1,B=n1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2,B=vss) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  nl2.reset (new db::Netlist (nl));

  (dynamic_cast<db::DeviceClassMOS3Transistor *> (nl2->begin_device_classes ().operator-> ()))->join_split_gates (nl2->begin_top_down ().operator-> ());

  EXPECT_EQ (nl2->to_string (),
    "circuit '' (A=n1,B=n2,G1=g1,G2=g2,VSS=vss);\n"
    "  device '' m11 (S=sd1,G=g1,D=n1,B=n1) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m21 (S=sd1,G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m12 (S=n1,G=g1,D=sd2,B=vss) (L=6,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "  device '' m22 (S=sd2,G=g2,D=n2,B=vss) (L=10,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

}


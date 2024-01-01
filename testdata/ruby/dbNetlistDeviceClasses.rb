# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2024 Matthias Koefferlein
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

if !$:.member?(File::dirname($0))
  $:.push(File::dirname($0))
end

load("test_prologue.rb")

class DBNetlistDeviceClasses_TestClass < TestBase

  def test_1_Resistors
  
    cls = RBA::DeviceClassResistor::new

    nl = RBA::Netlist::new
    nl.add(cls)

    circuit = RBA::Circuit::new
    nl.add(circuit)

    r1 = circuit.create_device(cls, "r1")
    r1.set_parameter(RBA::DeviceClassResistor::PARAM_R, 1.0)
    r1.set_parameter(RBA::DeviceClassResistor::PARAM_L, 10.0)
    r1.set_parameter(RBA::DeviceClassResistor::PARAM_W, 11.0)
    r1.set_parameter(RBA::DeviceClassResistor::PARAM_A, 12.0)
    r1.set_parameter(RBA::DeviceClassResistor::PARAM_P, 13.0)
    r2 = circuit.create_device(cls, "r2")
    r2.set_parameter("R", 3.0)

    pin_a = circuit.create_pin ("A")
    pin_b = circuit.create_pin ("B")

    n1 = circuit.create_net("n1")
    circuit.connect_pin(pin_a, n1)
    r1.connect_terminal(RBA::DeviceClassResistor::TERMINAL_A, n1)

    n2 = circuit.create_net("n2")
    r1.connect_terminal(RBA::DeviceClassResistor::TERMINAL_B, n2)
    r2.connect_terminal(RBA::DeviceClassResistor::TERMINAL_A, n2)

    n3 = circuit.create_net("n3")
    r2.connect_terminal(RBA::DeviceClassResistor::TERMINAL_B, n3)
    circuit.connect_pin(pin_b, n3)

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n3);
  device '' r1 (A=n1,B=n2) (R=1,L=10,W=11,A=12,P=13);
  device '' r2 (A=n2,B=n3) (R=3,L=0,W=0,A=0,P=0);
end;
END

    nl.combine_devices
    nl.purge

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n3);
  device '' r1 (A=n1,B=n3) (R=4,L=10,W=11,A=12,P=13);
end;
END

  end

  def test_1_ResistorsWithBulk
  
    cls = RBA::DeviceClassResistorWithBulk::new

    nl = RBA::Netlist::new
    nl.add(cls)

    circuit = RBA::Circuit::new
    nl.add(circuit)

    r1 = circuit.create_device(cls, "r1")
    r1.set_parameter(RBA::DeviceClassResistorWithBulk::PARAM_R, 1.0)
    r1.set_parameter(RBA::DeviceClassResistorWithBulk::PARAM_L, 10.0)
    r1.set_parameter(RBA::DeviceClassResistorWithBulk::PARAM_W, 11.0)
    r1.set_parameter(RBA::DeviceClassResistorWithBulk::PARAM_A, 12.0)
    r1.set_parameter(RBA::DeviceClassResistorWithBulk::PARAM_P, 13.0)
    r2 = circuit.create_device(cls, "r2")
    r2.set_parameter("R", 3.0)

    pin_a = circuit.create_pin ("A")
    pin_b = circuit.create_pin ("B")
    pin_bulk = circuit.create_pin ("BULK")

    n1 = circuit.create_net("n1")
    circuit.connect_pin(pin_a, n1)
    r1.connect_terminal(RBA::DeviceClassResistorWithBulk::TERMINAL_A, n1)

    n2 = circuit.create_net("n2")
    r1.connect_terminal(RBA::DeviceClassResistorWithBulk::TERMINAL_B, n2)
    r2.connect_terminal(RBA::DeviceClassResistorWithBulk::TERMINAL_A, n2)

    n3 = circuit.create_net("n3")
    r2.connect_terminal(RBA::DeviceClassResistorWithBulk::TERMINAL_B, n3)
    circuit.connect_pin(pin_b, n3)

    nb = circuit.create_net("nb")
    r1.connect_terminal(RBA::DeviceClassResistorWithBulk::TERMINAL_W, nb)
    r2.connect_terminal(RBA::DeviceClassResistorWithBulk::TERMINAL_W, nb)
    circuit.connect_pin(pin_bulk, nb)

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n3,BULK=nb);
  device '' r1 (A=n1,B=n2,W=nb) (R=1,L=10,W=11,A=12,P=13);
  device '' r2 (A=n2,B=n3,W=nb) (R=3,L=0,W=0,A=0,P=0);
end;
END

    nl.combine_devices
    nl.purge

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n3,BULK=nb);
  device '' r1 (A=n1,B=n3,W=nb) (R=4,L=10,W=11,A=12,P=13);
end;
END

  end

  def test_2_Capacitors
  
    cls = RBA::DeviceClassCapacitor::new

    nl = RBA::Netlist::new
    nl.add(cls)

    circuit = RBA::Circuit::new
    nl.add(circuit)

    c1 = circuit.create_device(cls, "c1")
    c1.set_parameter(RBA::DeviceClassCapacitor::PARAM_C, 2.0)
    c1.set_parameter(RBA::DeviceClassCapacitor::PARAM_A, 10.0)
    c1.set_parameter(RBA::DeviceClassCapacitor::PARAM_P, 11.0)
    c2 = circuit.create_device(cls, "c2")
    c2.set_parameter("C", 3.0)

    pin_a = circuit.create_pin ("A")
    pin_b = circuit.create_pin ("B")

    n1 = circuit.create_net("n1")
    circuit.connect_pin(pin_a, n1)
    c1.connect_terminal(RBA::DeviceClassCapacitor::TERMINAL_A, n1)

    n2 = circuit.create_net("n2")
    c1.connect_terminal(RBA::DeviceClassCapacitor::TERMINAL_B, n2)
    c2.connect_terminal(RBA::DeviceClassCapacitor::TERMINAL_A, n2)

    n3 = circuit.create_net("n3")
    c2.connect_terminal(RBA::DeviceClassCapacitor::TERMINAL_B, n3)
    circuit.connect_pin(pin_b, n3)

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n3);
  device '' c1 (A=n1,B=n2) (C=2,A=10,P=11);
  device '' c2 (A=n2,B=n3) (C=3,A=0,P=0);
end;
END

    nl.combine_devices
    nl.purge

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n3);
  device '' c1 (A=n1,B=n3) (C=1.2,A=10,P=11);
end;
END

  end

  def test_2_CapacitorsWithBulk
  
    cls = RBA::DeviceClassCapacitorWithBulk::new

    nl = RBA::Netlist::new
    nl.add(cls)

    circuit = RBA::Circuit::new
    nl.add(circuit)

    c1 = circuit.create_device(cls, "c1")
    c1.set_parameter(RBA::DeviceClassCapacitorWithBulk::PARAM_C, 2.0)
    c1.set_parameter(RBA::DeviceClassCapacitorWithBulk::PARAM_A, 10.0)
    c1.set_parameter(RBA::DeviceClassCapacitorWithBulk::PARAM_P, 11.0)
    c2 = circuit.create_device(cls, "c2")
    c2.set_parameter("C", 3.0)

    pin_a = circuit.create_pin ("A")
    pin_b = circuit.create_pin ("B")
    pin_bulk = circuit.create_pin ("BULK")

    n1 = circuit.create_net("n1")
    circuit.connect_pin(pin_a, n1)
    c1.connect_terminal(RBA::DeviceClassCapacitorWithBulk::TERMINAL_A, n1)

    n2 = circuit.create_net("n2")
    c1.connect_terminal(RBA::DeviceClassCapacitorWithBulk::TERMINAL_B, n2)
    c2.connect_terminal(RBA::DeviceClassCapacitorWithBulk::TERMINAL_A, n2)

    n3 = circuit.create_net("n3")
    c2.connect_terminal(RBA::DeviceClassCapacitorWithBulk::TERMINAL_B, n3)
    circuit.connect_pin(pin_b, n3)

    nb = circuit.create_net("nb")
    c1.connect_terminal(RBA::DeviceClassCapacitorWithBulk::TERMINAL_W, nb)
    c2.connect_terminal(RBA::DeviceClassCapacitorWithBulk::TERMINAL_W, nb)
    circuit.connect_pin(pin_bulk, nb)

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n3,BULK=nb);
  device '' c1 (A=n1,B=n2,W=nb) (C=2,A=10,P=11);
  device '' c2 (A=n2,B=n3,W=nb) (C=3,A=0,P=0);
end;
END

    nl.combine_devices
    nl.purge

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n3,BULK=nb);
  device '' c1 (A=n1,B=n3,W=nb) (C=1.2,A=10,P=11);
end;
END

  end

  def test_3_Inductors
  
    cls = RBA::DeviceClassInductor::new

    nl = RBA::Netlist::new
    nl.add(cls)

    circuit = RBA::Circuit::new
    nl.add(circuit)

    l1 = circuit.create_device(cls, "l1")
    l1.set_parameter(RBA::DeviceClassInductor::PARAM_L, 1.0)
    l2 = circuit.create_device(cls, "l2")
    l2.set_parameter("L", 3.0)

    pin_a = circuit.create_pin ("A")
    pin_b = circuit.create_pin ("B")

    n1 = circuit.create_net("n1")
    circuit.connect_pin(pin_a, n1)
    l1.connect_terminal(RBA::DeviceClassInductor::TERMINAL_A, n1)

    n2 = circuit.create_net("n2")
    l1.connect_terminal(RBA::DeviceClassInductor::TERMINAL_B, n2)
    l2.connect_terminal(RBA::DeviceClassInductor::TERMINAL_A, n2)

    n3 = circuit.create_net("n3")
    l2.connect_terminal(RBA::DeviceClassInductor::TERMINAL_B, n3)
    circuit.connect_pin(pin_b, n3)

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n3);
  device '' l1 (A=n1,B=n2) (L=1);
  device '' l2 (A=n2,B=n3) (L=3);
end;
END

    nl.combine_devices
    nl.purge

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n3);
  device '' l1 (A=n1,B=n3) (L=4);
end;
END

  end

  def test_4_Diodes
  
    cls = RBA::DeviceClassDiode::new

    nl = RBA::Netlist::new
    nl.add(cls)

    circuit = RBA::Circuit::new
    nl.add(circuit)

    d1 = circuit.create_device(cls, "d1")
    d1.set_parameter(RBA::DeviceClassDiode::PARAM_A, 1.0)
    d1.set_parameter(RBA::DeviceClassDiode::PARAM_P, 2.0)
    d2 = circuit.create_device(cls, "d2")
    d2.set_parameter("A", 3.0)
    d2.set_parameter("P", 4.0)

    pin_a = circuit.create_pin ("A")
    pin_b = circuit.create_pin ("B")

    n1 = circuit.create_net("n1")
    circuit.connect_pin(pin_a, n1)
    d1.connect_terminal(RBA::DeviceClassDiode::TERMINAL_A, n1)
    d2.connect_terminal(RBA::DeviceClassDiode::TERMINAL_A, n1)

    n2 = circuit.create_net("n2")
    d1.connect_terminal(RBA::DeviceClassDiode::TERMINAL_C, n2)
    d2.connect_terminal(RBA::DeviceClassDiode::TERMINAL_C, n2)
    circuit.connect_pin(pin_b, n2)

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n2);
  device '' d1 (A=n1,C=n2) (A=1,P=2);
  device '' d2 (A=n1,C=n2) (A=3,P=4);
end;
END

    nl.combine_devices
    nl.purge

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n2);
  device '' d1 (A=n1,C=n2) (A=4,P=6);
end;
END

  end

  def test_5_MOS3
  
    cls = RBA::DeviceClassMOS3Transistor::new

    nl = RBA::Netlist::new
    nl.add(cls)

    circuit = RBA::Circuit::new
    nl.add(circuit)

    d1 = circuit.create_device(cls, "d1")
    d1.set_parameter(RBA::DeviceClassMOS3Transistor::PARAM_L, 1.0)
    d1.set_parameter(RBA::DeviceClassMOS3Transistor::PARAM_W, 2.0)
    d1.set_parameter(RBA::DeviceClassMOS3Transistor::PARAM_AS, 3.0)
    d1.set_parameter(RBA::DeviceClassMOS3Transistor::PARAM_AD, 4.0)
    d1.set_parameter(RBA::DeviceClassMOS3Transistor::PARAM_PS, 13.0)
    d1.set_parameter(RBA::DeviceClassMOS3Transistor::PARAM_PD, 14.0)
    d2 = circuit.create_device(cls, "d2")
    d2.set_parameter("L", 1.0)
    d2.set_parameter("W", 3.0)
    d2.set_parameter("AS", 4.0)
    d2.set_parameter("AD", 5.0)
    d2.set_parameter("PS", 14.0)
    d2.set_parameter("PD", 15.0)

    pin_a = circuit.create_pin ("A")
    pin_b = circuit.create_pin ("B")
    pin_c = circuit.create_pin ("C")

    n1 = circuit.create_net("n1")
    circuit.connect_pin(pin_a, n1)
    d1.connect_terminal(RBA::DeviceClassMOS3Transistor::TERMINAL_S, n1)
    d2.connect_terminal(RBA::DeviceClassMOS3Transistor::TERMINAL_S, n1)

    n2 = circuit.create_net("n2")
    circuit.connect_pin(pin_b, n2)
    d1.connect_terminal(RBA::DeviceClassMOS3Transistor::TERMINAL_D, n2)
    d2.connect_terminal(RBA::DeviceClassMOS3Transistor::TERMINAL_D, n2)

    n3 = circuit.create_net("n3")
    circuit.connect_pin(pin_c, n3)
    d1.connect_terminal(RBA::DeviceClassMOS3Transistor::TERMINAL_G, n3)
    d2.connect_terminal(RBA::DeviceClassMOS3Transistor::TERMINAL_G, n3)

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n2,C=n3);
  device '' d1 (S=n1,G=n3,D=n2) (L=1,W=2,AS=3,AD=4,PS=13,PD=14);
  device '' d2 (S=n1,G=n3,D=n2) (L=1,W=3,AS=4,AD=5,PS=14,PD=15);
end;
END

    nl.combine_devices
    nl.purge

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n2,C=n3);
  device '' d1 (S=n1,G=n3,D=n2) (L=1,W=5,AS=7,AD=9,PS=27,PD=29);
end;
END

  end

  def test_6_MOS4
  
    cls = RBA::DeviceClassMOS4Transistor::new

    nl = RBA::Netlist::new
    nl.add(cls)

    circuit = RBA::Circuit::new
    nl.add(circuit)

    d1 = circuit.create_device(cls, "d1")
    d1.set_parameter(RBA::DeviceClassMOS4Transistor::PARAM_L, 1.0)
    d1.set_parameter(RBA::DeviceClassMOS4Transistor::PARAM_W, 2.0)
    d1.set_parameter(RBA::DeviceClassMOS4Transistor::PARAM_AS, 3.0)
    d1.set_parameter(RBA::DeviceClassMOS4Transistor::PARAM_AD, 4.0)
    d1.set_parameter(RBA::DeviceClassMOS4Transistor::PARAM_PS, 13.0)
    d1.set_parameter(RBA::DeviceClassMOS4Transistor::PARAM_PD, 14.0)
    d2 = circuit.create_device(cls, "d2")
    d2.set_parameter("L", 1.0)
    d2.set_parameter("W", 3.0)
    d2.set_parameter("AS", 4.0)
    d2.set_parameter("AD", 5.0)
    d2.set_parameter("PS", 14.0)
    d2.set_parameter("PD", 15.0)

    pin_a = circuit.create_pin ("A")
    pin_b = circuit.create_pin ("B")
    pin_c = circuit.create_pin ("C")
    pin_d = circuit.create_pin ("D")

    n1 = circuit.create_net("n1")
    circuit.connect_pin(pin_a, n1)
    d1.connect_terminal(RBA::DeviceClassMOS4Transistor::TERMINAL_S, n1)
    d2.connect_terminal(RBA::DeviceClassMOS4Transistor::TERMINAL_S, n1)

    n2 = circuit.create_net("n2")
    circuit.connect_pin(pin_b, n2)
    d1.connect_terminal(RBA::DeviceClassMOS4Transistor::TERMINAL_D, n2)
    d2.connect_terminal(RBA::DeviceClassMOS4Transistor::TERMINAL_D, n2)

    n3 = circuit.create_net("n3")
    circuit.connect_pin(pin_c, n3)
    d1.connect_terminal(RBA::DeviceClassMOS4Transistor::TERMINAL_G, n3)
    d2.connect_terminal(RBA::DeviceClassMOS4Transistor::TERMINAL_G, n3)

    n4 = circuit.create_net("n4")
    circuit.connect_pin(pin_d, n4)
    d1.connect_terminal(RBA::DeviceClassMOS4Transistor::TERMINAL_B, n4)
    d2.connect_terminal(RBA::DeviceClassMOS4Transistor::TERMINAL_B, n4)

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n2,C=n3,D=n4);
  device '' d1 (S=n1,G=n3,D=n2,B=n4) (L=1,W=2,AS=3,AD=4,PS=13,PD=14);
  device '' d2 (S=n1,G=n3,D=n2,B=n4) (L=1,W=3,AS=4,AD=5,PS=14,PD=15);
end;
END

    nl.combine_devices
    nl.purge

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n2,C=n3,D=n4);
  device '' d1 (S=n1,G=n3,D=n2,B=n4) (L=1,W=5,AS=7,AD=9,PS=27,PD=29);
end;
END

  end

  def test_7_BJT3
  
    cls = RBA::DeviceClassBJT3Transistor::new

    nl = RBA::Netlist::new
    nl.add(cls)

    circuit = RBA::Circuit::new
    nl.add(circuit)

    d1 = circuit.create_device(cls, "d1")
    d1.set_parameter(RBA::DeviceClassBJT3Transistor::PARAM_AE, 1.0)
    d1.set_parameter(RBA::DeviceClassBJT3Transistor::PARAM_AB, 2.0)
    d1.set_parameter(RBA::DeviceClassBJT3Transistor::PARAM_AC, 3.0)
    d1.set_parameter(RBA::DeviceClassBJT3Transistor::PARAM_PE, 12.0)
    d1.set_parameter(RBA::DeviceClassBJT3Transistor::PARAM_PB, 13.0)
    d1.set_parameter(RBA::DeviceClassBJT3Transistor::PARAM_PC, 14.0)
    d1.set_parameter(RBA::DeviceClassBJT3Transistor::PARAM_NE, 2.0)
    d2 = circuit.create_device(cls, "d2")
    d2.set_parameter("AE", 2.0)
    d2.set_parameter("AB", 3.0)
    d2.set_parameter("AC", 4.0)
    d2.set_parameter("PE", 13.0)
    d2.set_parameter("PB", 14.0)
    d2.set_parameter("PC", 15.0)
    d2.set_parameter("NE", 3.0)

    pin_a = circuit.create_pin ("A")
    pin_b = circuit.create_pin ("B")
    pin_c = circuit.create_pin ("C")

    n1 = circuit.create_net("n1")
    circuit.connect_pin(pin_a, n1)
    d1.connect_terminal(RBA::DeviceClassBJT3Transistor::TERMINAL_C, n1)
    d2.connect_terminal(RBA::DeviceClassBJT3Transistor::TERMINAL_C, n1)

    n2 = circuit.create_net("n2")
    circuit.connect_pin(pin_b, n2)
    d1.connect_terminal(RBA::DeviceClassBJT3Transistor::TERMINAL_E, n2)
    d2.connect_terminal(RBA::DeviceClassBJT3Transistor::TERMINAL_E, n2)

    n3 = circuit.create_net("n3")
    circuit.connect_pin(pin_c, n3)
    d1.connect_terminal(RBA::DeviceClassBJT3Transistor::TERMINAL_B, n3)
    d2.connect_terminal(RBA::DeviceClassBJT3Transistor::TERMINAL_B, n3)

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n2,C=n3);
  device '' d1 (C=n1,B=n3,E=n2) (AE=1,PE=12,AB=2,PB=13,AC=3,PC=14,NE=2);
  device '' d2 (C=n1,B=n3,E=n2) (AE=2,PE=13,AB=3,PB=14,AC=4,PC=15,NE=3);
end;
END

    nl.combine_devices
    nl.purge

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n2,C=n3);
  device '' d1 (C=n1,B=n3,E=n2) (AE=3,PE=25,AB=2,PB=13,AC=3,PC=14,NE=5);
end;
END

  end

  def test_8_BJT4
  
    cls = RBA::DeviceClassBJT4Transistor::new

    nl = RBA::Netlist::new
    nl.add(cls)

    circuit = RBA::Circuit::new
    nl.add(circuit)

    d1 = circuit.create_device(cls, "d1")
    d1.set_parameter(RBA::DeviceClassBJT4Transistor::PARAM_AE, 1.0)
    d1.set_parameter(RBA::DeviceClassBJT4Transistor::PARAM_AB, 2.0)
    d1.set_parameter(RBA::DeviceClassBJT4Transistor::PARAM_AC, 3.0)
    d1.set_parameter(RBA::DeviceClassBJT4Transistor::PARAM_PE, 12.0)
    d1.set_parameter(RBA::DeviceClassBJT4Transistor::PARAM_PB, 13.0)
    d1.set_parameter(RBA::DeviceClassBJT4Transistor::PARAM_PC, 14.0)
    d2 = circuit.create_device(cls, "d2")
    d2.set_parameter("AE", 2.0)
    d2.set_parameter("AB", 3.0)
    d2.set_parameter("AC", 4.0)
    d2.set_parameter("PE", 13.0)
    d2.set_parameter("PB", 14.0)
    d2.set_parameter("PC", 15.0)
    d2.set_parameter("NE", 2.0)

    pin_a = circuit.create_pin ("A")
    pin_b = circuit.create_pin ("B")
    pin_c = circuit.create_pin ("C")
    pin_d = circuit.create_pin ("D")

    n1 = circuit.create_net("n1")
    circuit.connect_pin(pin_a, n1)
    d1.connect_terminal(RBA::DeviceClassBJT4Transistor::TERMINAL_C, n1)
    d2.connect_terminal(RBA::DeviceClassBJT4Transistor::TERMINAL_C, n1)

    n2 = circuit.create_net("n2")
    circuit.connect_pin(pin_b, n2)
    d1.connect_terminal(RBA::DeviceClassBJT4Transistor::TERMINAL_E, n2)
    d2.connect_terminal(RBA::DeviceClassBJT4Transistor::TERMINAL_E, n2)

    n3 = circuit.create_net("n3")
    circuit.connect_pin(pin_c, n3)
    d1.connect_terminal(RBA::DeviceClassBJT4Transistor::TERMINAL_B, n3)
    d2.connect_terminal(RBA::DeviceClassBJT4Transistor::TERMINAL_B, n3)

    n4 = circuit.create_net("n4")
    circuit.connect_pin(pin_d, n4)
    d1.connect_terminal(RBA::DeviceClassBJT4Transistor::TERMINAL_S, n4)
    d2.connect_terminal(RBA::DeviceClassBJT4Transistor::TERMINAL_S, n4)

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n2,C=n3,D=n4);
  device '' d1 (C=n1,B=n3,E=n2,S=n4) (AE=1,PE=12,AB=2,PB=13,AC=3,PC=14,NE=1);
  device '' d2 (C=n1,B=n3,E=n2,S=n4) (AE=2,PE=13,AB=3,PB=14,AC=4,PC=15,NE=2);
end;
END

    nl.combine_devices
    nl.purge

    assert_equal(nl.to_s, <<END)
circuit '' (A=n1,B=n2,C=n3,D=n4);
  device '' d1 (C=n1,B=n3,E=n2,S=n4) (AE=3,PE=25,AB=2,PB=13,AC=3,PC=14,NE=3);
end;
END

  end

end

load("test_epilogue.rb")


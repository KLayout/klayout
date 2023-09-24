# encoding: UTF-8

# KLayout Layout Viewer
# Copyright(C) 2006-2019 Matthias Koefferlein
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
#(at your option) any later version.
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

class NetlistCompareTestLogger < RBA::GenericNetlistCompareLogger

  def initialize
    @texts = []
  end

  def out(text)
    @texts << text
  end

  def device_class_mismatch(a, b, msg)
    out("device_class_mismatch " + dc2str(a) + " " + dc2str(b))
  end

  def begin_circuit(a, b)
    out("begin_circuit " + circuit2str(a) + " " + circuit2str(b))
  end

  def end_circuit(a, b, matching, msg)
    out("end_circuit " + circuit2str(a) + " " + circuit2str(b) + " " + (matching ? "MATCH" : "NOMATCH"))
  end

  def circuit_skipped(a, b, msg)
    out("circuit_skipped " + circuit2str(a) + " " + circuit2str(b))
  end

  def circuit_mismatch(a, b, msg)
    out("circuit_mismatch " + circuit2str(a) + " " + circuit2str(b))
  end

  def match_nets(a, b)
    out("match_nets " + net2str(a) + " " + net2str(b))
  end

  def match_ambiguous_nets(a, b, msg)
    out("match_ambiguous_nets " + net2str(a) + " " + net2str(b))
  end

  def net_mismatch(a, b, msg)
    out("net_mismatch " + net2str(a) + " " + net2str(b))
  end

  def match_devices(a, b)
    out("match_devices " + device2str(a) + " " + device2str(b))
  end

  def device_mismatch(a, b, msg)
    out("device_mismatch " + device2str(a) + " " + device2str(b))
  end

  def match_devices_with_different_parameters(a, b)
    out("match_devices_with_different_parameters " + device2str(a) + " " + device2str(b))
  end

  def match_devices_with_different_device_classes(a, b)
    out("match_devices_with_different_device_classes " + device2str(a) + " " + device2str(b))
  end

  def match_pins(a, b)
    out("match_pins " + pin2str(a) + " " + pin2str(b))
  end

  def pin_mismatch(a, b, msg)
    out("pin_mismatch " + pin2str(a) + " " + pin2str(b))
  end

  def match_subcircuits(a, b)
    out("match_subcircuits " + subcircuit2str(a) + " " + subcircuit2str(b))
  end

  def subcircuit_mismatch(a, b, msg)
    out("subcircuit_mismatch " + subcircuit2str(a) + " " + subcircuit2str(b))
  end

  def text
    return @texts.join("\n") + "\n"
  end

  def clear
    @texts = []
  end

  def dc2str(x) 
    return x ? x.name : "(null)"
  end

  def circuit2str(x) 
    return x ? x.name : "(null)"
  end

  def device2str(x) 
    return x ? x.expanded_name : "(null)"
  end

  def net2str(x) 
    return x ? x.expanded_name : "(null)"
  end

  def pin2str(x) 
    return x ? x.expanded_name : "(null)"
  end

  def subcircuit2str(x) 
    return x ? x.expanded_name : "(null)"
  end

end

def prep_nl(nl, str)

  dc = RBA::DeviceClassMOS3Transistor::new
  dc.name = "PMOS"
  nl.add(dc)

  dc = RBA::DeviceClassMOS3Transistor::new
  dc.name = "NMOS"
  nl.add(dc)

  dc = RBA::DeviceClassMOS3Transistor::new
  dc.name = "PMOSB"
  nl.add(dc)

  dc = RBA::DeviceClassMOS3Transistor::new
  dc.name = "NMOSB"
  nl.add(dc)

  dc = RBA::DeviceClassMOS4Transistor::new
  dc.name = "PMOS4"
  nl.add(dc)

  dc = RBA::DeviceClassMOS4Transistor::new
  dc.name = "NMOS4"
  nl.add(dc)

  dc = RBA::DeviceClassResistor::new
  dc.name = "RES"
  nl.add(dc)

  dc = RBA::DeviceClassCapacitor::new
  dc.name = "CAP"
  nl.add(dc)

  dc = RBA::DeviceClassInductor::new
  dc.name = "IND"
  nl.add(dc)

  dc = RBA::DeviceClassDiode::new
  dc.name = "DIODE"
  nl.add(dc)

  nl.from_s(str)

end

class NetlistCompare_TestClass < TestBase

  def test_1

    # severity enums
    assert_equal(NetlistCompareTestLogger::Info.to_s, "Info")
    assert_equal(NetlistCompareTestLogger::Error.to_s, "Error")
    assert_equal(NetlistCompareTestLogger::Warning.to_s, "Warning")
    assert_equal(NetlistCompareTestLogger::NoSeverity.to_s, "NoSeverity")

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    dc = RBA::DeviceClass::new
    dc.name = "A"
    nl1.add(dc)
    dc = RBA::DeviceClass::new
    dc.name = "B"
    nl2.add(dc)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
device_class_mismatch A (null)
device_class_mismatch (null) B
END

    assert_equal(good, true)
  
    nls1 = <<"END"
circuit INV($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1(S=VDD,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $2(S=VSS,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nls2 = <<"END"
circuit INV($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device NMOS $1(S=OUT,G=IN,D=VSS)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2(S=VDD,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit INV INV
match_nets VDD VDD
match_nets OUT OUT
match_nets IN IN
match_nets VSS VSS
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $2 $1
match_devices $1 $2
end_circuit INV INV MATCH
END

    assert_equal(good, true)

  end

  def test_2

    nls1 = <<"END"
circuit INV($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1(S=VDD,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $2(S=VSS,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nls2 = <<"END"
circuit INV($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device NMOSB $1(S=OUT,G=IN,D=VSS)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOSB $2(S=VDD,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    comp.same_device_classes(nl1.device_class_by_name("PMOS"), nl2.device_class_by_name("PMOSB"))

    good = comp.compare(nl1, nl2)
    assert_equal(good, false)

    logger.clear
    comp.same_device_classes(nl1.device_class_by_name("NMOS"), nl2.device_class_by_name("NMOSB"))
    # avoids device class mismatch errors
    comp.same_device_classes(nl1.device_class_by_name("NMOSB"), nl2.device_class_by_name("NMOS"))
    comp.same_device_classes(nl1.device_class_by_name("PMOSB"), nl2.device_class_by_name("PMOS"))
    good = comp.compare(nl1, nl2)

    assert_equal(logger.text(), <<"END")
begin_circuit INV INV
match_nets VDD VDD
match_nets OUT OUT
match_nets IN IN
match_nets VSS VSS
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $2 $1
match_devices $1 $2
end_circuit INV INV MATCH
END

    assert_equal(good, true)

  end

  def test_3

    nls1 = <<"END"
circuit INV($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1(S=VDD,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $2(S=VSS,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nls2 = <<"END"
circuit INV($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device NMOS $1(S=OUT,G=IN,D=VSS)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2(S=VDD,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    ca = nl1.circuit_by_name("INV")
    cb = nl2.circuit_by_name("INV")
    comp.same_nets(ca.net_by_name("VDD"), cb.net_by_name("VDD"))
    comp.same_nets(ca.net_by_name("VSS"), cb.net_by_name("VSS"))

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text(), <<"END")
begin_circuit INV INV
match_nets VDD VDD
match_nets VSS VSS
match_nets OUT OUT
match_nets IN IN
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $2 $1
match_devices $1 $2
end_circuit INV INV MATCH
END

    assert_equal(good, true)

  end

  def test_4

    nls1 = <<"END"
circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nls2 = <<"END"
circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit BUF BUF
match_nets VDD VDD
match_nets OUT OUT
match_nets VSS VSS
match_nets IN IN
match_ambiguous_nets INT $10
match_ambiguous_nets INT2 $11
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $1 $1
match_devices $3 $2
match_devices $5 $3
match_devices $7 $4
match_devices $2 $5
match_devices $4 $6
match_devices $6 $7
match_devices $8 $8
end_circuit BUF BUF MATCH
END

    assert_equal(good, true)

  end

  def test_5

    nls1 = <<"END"
circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nls2 = <<"END"
circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.35,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    #  NOTE: adding this power hint makes the device class error harder to detect
    ca = nl1.circuit_by_name("BUF")
    cb = nl2.circuit_by_name("BUF")
    comp.same_nets(ca.net_by_name("VDD"), cb.net_by_name("VDD"))
    comp.same_nets(ca.net_by_name("VSS"), cb.net_by_name("VSS"))

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit BUF BUF
match_nets VDD VDD
match_nets VSS VSS
match_nets OUT OUT
net_mismatch IN IN
match_nets INT $10
net_mismatch INT2 $11
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $1 $1
match_devices $3 $2
match_devices $5 $3
match_devices $7 $4
match_devices $2 $5
match_devices $4 $6
match_devices_with_different_parameters $6 $7
match_devices $8 $8
end_circuit BUF BUF NOMATCH
END

    assert_equal(good, false)

    logger.clear
    eqp = RBA::EqualDeviceParameters::new(RBA::DeviceClassMOS3Transistor::PARAM_L, 0.2, 0.0)
    nl1.device_class_by_name("NMOS").equal_parameters = eqp
    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit BUF BUF
match_nets VDD VDD
match_nets VSS VSS
match_nets OUT OUT
match_nets IN IN
match_ambiguous_nets INT $10
match_ambiguous_nets INT2 $11
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $1 $1
match_devices $3 $2
match_devices $5 $3
match_devices $7 $4
match_devices $2 $5
match_devices $4 $6
match_devices $6 $7
match_devices $8 $8
end_circuit BUF BUF MATCH
END

    assert_equal(good, true)

    logger.clear
    nl1.device_class_by_name("NMOS").equal_parameters = nil
    assert_equal(nl1.device_class_by_name("NMOS").equal_parameters == nil, true)
    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit BUF BUF
match_nets VDD VDD
match_nets VSS VSS
match_nets OUT OUT
net_mismatch IN IN
match_nets INT $10
net_mismatch INT2 $11
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $1 $1
match_devices $3 $2
match_devices $5 $3
match_devices $7 $4
match_devices $2 $5
match_devices $4 $6
match_devices_with_different_parameters $6 $7
match_devices $8 $8
end_circuit BUF BUF NOMATCH
END

    assert_equal(good, false)

    logger.clear
    eqp = RBA::EqualDeviceParameters::new(RBA::DeviceClassMOS3Transistor::PARAM_W, 0.01, 0.0)
    eqp = eqp + RBA::EqualDeviceParameters::new(RBA::DeviceClassMOS3Transistor::PARAM_L, 0.2, 0.0)
    nl1.device_class_by_name("NMOS").equal_parameters = eqp
    assert_equal(nl1.device_class_by_name("NMOS").equal_parameters == nil, false)
    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit BUF BUF
match_nets VDD VDD
match_nets VSS VSS
match_nets OUT OUT
match_nets IN IN
match_ambiguous_nets INT $10
match_ambiguous_nets INT2 $11
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $1 $1
match_devices $3 $2
match_devices $5 $3
match_devices $7 $4
match_devices $2 $5
match_devices $4 $6
match_devices $6 $7
match_devices $8 $8
end_circuit BUF BUF MATCH
END

    assert_equal(good, true)

    logger.clear
    dc = nl1.device_class_by_name("NMOS")
    dc.equal_parameters = RBA::EqualDeviceParameters::new(RBA::DeviceClassMOS3Transistor::PARAM_W, 0.01, 0.0)
    dc.equal_parameters += RBA::EqualDeviceParameters::new(RBA::DeviceClassMOS3Transistor::PARAM_L, 0.2, 0.0)
    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit BUF BUF
match_nets VDD VDD
match_nets VSS VSS
match_nets OUT OUT
match_nets IN IN
match_ambiguous_nets INT $10
match_ambiguous_nets INT2 $11
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $1 $1
match_devices $3 $2
match_devices $5 $3
match_devices $7 $4
match_devices $2 $5
match_devices $4 $6
match_devices $6 $7
match_devices $8 $8
end_circuit BUF BUF MATCH
END

    assert_equal(good, true)

  end

  def test_6

    nls1 = <<"END"
circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOSB $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOSB $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nls2 = <<"END"
circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOSB $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    #  NOTE: adding this power hint makes the device class error harder to detect
    ca = nl1.circuit_by_name("BUF")
    cb = nl2.circuit_by_name("BUF")
    comp.same_nets(ca.net_by_name("VDD"), cb.net_by_name("VDD"), false)
    comp.same_nets(ca.net_by_name("VSS"), cb.net_by_name("VSS"), false)
    comp.same_nets(ca.net_by_name("OUT"), cb.net_by_name("OUT"), false)

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit BUF BUF
match_nets VDD VDD
match_nets VSS VSS
match_nets OUT OUT
match_nets INT $10
match_nets IN IN
net_mismatch INT2 $11
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $1 $1
match_devices $3 $2
match_devices $5 $3
match_devices_with_different_device_classes $7 $4
match_devices $2 $5
match_devices $4 $6
match_devices $6 $7
match_devices $8 $8
end_circuit BUF BUF NOMATCH
END

    assert_equal(good, false)

  end
 
  def test_6b

    nls1 = <<"END"
circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOSB $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOSB $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nls2 = <<"END"
circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOSB $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    #  NOTE: adding this power hint makes the device class error harder to detect
    ca = nl1.circuit_by_name("BUF")
    cb = nl2.circuit_by_name("BUF")
    comp.same_nets(ca.net_by_name("VDD"), cb.net_by_name("VDD"), true)
    comp.same_nets(ca.net_by_name("VSS"), cb.net_by_name("VSS"), true)
    comp.same_nets(ca.net_by_name("OUT"), cb.net_by_name("OUT"), true)

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit BUF BUF
net_mismatch VDD VDD
match_nets VSS VSS
net_mismatch OUT OUT
match_nets INT $10
match_nets IN IN
net_mismatch INT2 $11
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $1 $1
match_devices $3 $2
match_devices $5 $3
match_devices_with_different_device_classes $7 $4
match_devices $2 $5
match_devices $4 $6
match_devices $6 $7
match_devices $8 $8
end_circuit BUF BUF NOMATCH
END

    assert_equal(good, false)

  end
 
  def test_6c

    nls1 = <<"END"
circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOSB $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOSB $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nls2 = <<"END"
circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOSB $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    #  NOTE: adding this power hint makes the device class error harder to detect
    ca = nl1.circuit_by_name("BUF")
    cb = nl2.circuit_by_name("BUF")
    comp.same_nets(ca, cb, ca.net_by_name("VDD"), cb.net_by_name("VDD"), true)
    comp.same_nets(ca, cb, ca.net_by_name("VSS"), nil, false)
    comp.same_nets(ca, cb, ca.net_by_name("OUT"), nil, true)

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit BUF BUF
net_mismatch VDD VDD
match_nets VSS (null)
net_mismatch OUT (null)
match_nets INT $10
match_nets IN IN
net_mismatch INT2 (null)
net_mismatch (null) VSS
net_mismatch (null) OUT
net_mismatch (null) $11
match_pins $0 $1
match_pins $2 $0
match_pins $1 (null)
match_pins $3 (null)
match_pins (null) $2
match_pins (null) $3
match_devices $1 $1
device_mismatch $5 $3
device_mismatch $3 $2
device_mismatch (null) $4
device_mismatch $6 $5
device_mismatch $4 $6
device_mismatch $2 $7
device_mismatch (null) $8
device_mismatch $7 (null)
device_mismatch $8 (null)
end_circuit BUF BUF NOMATCH
END

    assert_equal(good, false)

  end
 
  def test_7

    nls1 = <<"END"
circuit BUF ($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1 (S=VDD,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $2 (S=VSS,G=IN,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=VSS,G=INT,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $5 (S=VDD,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=VSS,G=IN,D=INT2) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOSB $7 (S=VDD,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOSB $8 (S=VSS,G=INT2,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nls2 = <<"END"
circuit BUF ($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device PMOS $1 (S=VDD,G=IN,D=$10) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=$10,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $3 (S=VDD,G=IN,D=$11) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOSB $4 (S=VDD,G=$11,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $5 (S=$10,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $6 (S=OUT,G=$10,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $7 (S=$11,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOSB $8 (S=OUT,G=$11,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device RES $9 (A=$10,B=$11) (R=42);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    #  Forcing the power nets into equality makes the resistor error harder to detect
    ca = nl1.circuit_by_name("BUF")
    cb = nl2.circuit_by_name("BUF")
    comp.same_nets(ca.net_by_name("VDD"), cb.net_by_name("VDD"))
    comp.same_nets(ca.net_by_name("VSS"), cb.net_by_name("VSS"))

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit BUF BUF
match_nets VDD VDD
match_nets VSS VSS
net_mismatch INT $10
match_nets IN IN
net_mismatch INT2 $11
match_nets OUT OUT
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $1 $1
match_devices $3 $2
match_devices $5 $3
match_devices $7 $4
match_devices $2 $5
match_devices $4 $6
match_devices $6 $7
match_devices $8 $8
device_mismatch (null) $9
end_circuit BUF BUF NOMATCH
END

    assert_equal(good, false)

  end

  def test_8

    nls1 = <<"END"
circuit INV ($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $2 (S=VSS,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
circuit TOP ($1=IN,$2=OUT,$3=VDD,$4=VSS);
  subcircuit INV $1 ($1=IN,$2=INT,$3=VDD,$4=VSS);
  subcircuit INV $2 ($1=INT,$2=OUT,$3=VDD,$4=VSS);
end;
END

    nls2 = <<"END"
circuit INVB ($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device NMOS $1 (S=OUT,G=IN,D=VSS) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=IN,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
circuit TOP ($1=OUT,$2=VDD,$3=IN,$4=VSS);
  subcircuit INVB $1 ($1=VDD,$2=INT,$3=VSS,$4=OUT);
  subcircuit INVB $2 ($1=VDD,$2=IN,$3=VSS,$4=INT);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    good = comp.compare(nl1, nl2)
    assert_equal(good, false)

    logger.clear
    comp.same_circuits(nl1.circuit_by_name("INV"), nl2.circuit_by_name("INVB"))

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit INV INVB
match_nets VDD VDD
match_nets OUT OUT
match_nets IN IN
match_nets VSS VSS
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $2 $1
match_devices $1 $2
end_circuit INV INVB MATCH
begin_circuit TOP TOP
match_nets OUT OUT
match_nets VSS VSS
match_nets INT INT
match_nets IN IN
match_nets VDD VDD
match_pins $0 $2
match_pins $1 $0
match_pins $2 $1
match_pins $3 $3
match_subcircuits $2 $1
match_subcircuits $1 $2
end_circuit TOP TOP MATCH
END

    assert_equal(good, true)
  
  end

  def test_9

    nls1 = <<"END"
circuit NAND ($0=A,$1=B,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1 (S=VDD,G=A,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $3 (S=VSS,G=A,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=INT,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
circuit TOP ($0=IN1,$1=IN2,$2=OUT,$3=VDD,$4=VSS);
  subcircuit NAND $1 ($0=IN1,$1=IN2,$2=INT,$3=VDD,$4=VSS);
  subcircuit NAND $2 ($0=IN1,$1=INT,$2=OUT,$3=VDD,$4=VSS);
end;
END

    nls2 = <<"END"
circuit NAND ($0=A,$1=B,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1 (S=VDD,G=A,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $3 (S=VSS,G=A,D=INT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=INT,G=B,D=OUT) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
circuit TOP ($0=IN1,$1=IN2,$2=OUT,$3=VDD,$4=VSS);
  subcircuit NAND $2 ($0=IN1,$1=INT,$2=OUT,$3=VDD,$4=VSS);
  subcircuit NAND $1 ($0=IN1,$1=IN2,$2=INT,$3=VDD,$4=VSS);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    comp.equivalent_pins(nl2.circuit_by_name("NAND"), 0, 1)

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit NAND NAND
match_nets VSS VSS
match_nets INT INT
match_nets OUT OUT
match_nets VDD VDD
match_nets B B
match_nets A A
match_pins $0 $0
match_pins $1 $1
match_pins $2 $2
match_pins $3 $3
match_pins $4 $4
match_devices $1 $1
match_devices $2 $2
match_devices $3 $3
match_devices $4 $4
end_circuit NAND NAND MATCH
begin_circuit TOP TOP
match_nets OUT OUT
match_nets VSS VSS
match_nets VDD VDD
match_nets INT INT
match_nets IN2 IN2
match_nets IN1 IN1
match_pins $0 $0
match_pins $1 $1
match_pins $2 $2
match_pins $3 $3
match_pins $4 $4
match_subcircuits $2 $1
match_subcircuits $1 $2
end_circuit TOP TOP MATCH
END

    assert_equal(good, true)

    logger.clear
    comp = RBA::NetlistComparer::new(logger)

    comp.equivalent_pins(nl2.circuit_by_name("NAND"), [ 1, 0 ])

    good = comp.compare(nl1, nl2)

    assert_equal(logger.text, <<"END")
begin_circuit NAND NAND
match_nets VSS VSS
match_nets INT INT
match_nets OUT OUT
match_nets VDD VDD
match_nets B B
match_nets A A
match_pins $0 $0
match_pins $1 $1
match_pins $2 $2
match_pins $3 $3
match_pins $4 $4
match_devices $1 $1
match_devices $2 $2
match_devices $3 $3
match_devices $4 $4
end_circuit NAND NAND MATCH
begin_circuit TOP TOP
match_nets OUT OUT
match_nets VSS VSS
match_nets VDD VDD
match_nets INT INT
match_nets IN2 IN2
match_nets IN1 IN1
match_pins $0 $0
match_pins $1 $1
match_pins $2 $2
match_pins $3 $3
match_pins $4 $4
match_subcircuits $2 $1
match_subcircuits $1 $2
end_circuit TOP TOP MATCH
END

    assert_equal(good, true)

  end

  def test_10

    nls1 = <<"END"
circuit INV($1=IN,$2=OUT,$3=VDD,$4=VSS);
  device PMOS $1(S=VDD,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device CAP $2 (A=OUT,B=IN) (C=1e-12);
  device NMOS $2(S=VSS,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nls2 = <<"END"
circuit INV($1=VDD,$2=IN,$3=VSS,$4=OUT);
  device NMOSB $1(S=OUT,G=IN,D=VSS)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device CAP $2 (A=OUT,B=IN) (C=1e-13);
  device RES $3 (A=OUT,B=IN) (R=1000);
  device PMOSB $2(S=VDD,G=IN,D=OUT)(L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
end;
END

    nl1 = RBA::Netlist::new
    nl2 = RBA::Netlist::new
    prep_nl(nl1, nls1)
    prep_nl(nl2, nls2)

    logger = NetlistCompareTestLogger::new
    comp = RBA::NetlistComparer::new(logger)

    comp.same_device_classes(nl1.device_class_by_name("PMOS"), nl2.device_class_by_name("PMOSB"))

    good = comp.compare(nl1, nl2)
    assert_equal(good, false)

    logger.clear
    comp.same_device_classes(nl1.device_class_by_name("NMOS"), nl2.device_class_by_name("NMOSB"))
    # avoids device class mismatch errors
    comp.same_device_classes(nl1.device_class_by_name("NMOSB"), nl2.device_class_by_name("NMOS"))
    comp.same_device_classes(nl1.device_class_by_name("PMOSB"), nl2.device_class_by_name("PMOS"))
    good = comp.compare(nl1, nl2)

    assert_equal(logger.text(), <<"END")
begin_circuit INV INV
match_nets VDD VDD
net_mismatch OUT OUT
match_nets VSS VSS
net_mismatch IN IN
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $3 $1
match_devices_with_different_parameters $2 $2
match_devices $1 $4
device_mismatch (null) $3
end_circuit INV INV NOMATCH
END

    assert_equal(good, false)

    comp.max_resistance = 900.0
    comp.min_capacitance = 1e-11

    logger.clear
    good = comp.compare(nl1, nl2)

    assert_equal(logger.text(), <<"END")
begin_circuit INV INV
match_nets VDD VDD
match_nets OUT OUT
match_nets IN IN
match_nets VSS VSS
match_pins $0 $1
match_pins $1 $3
match_pins $2 $0
match_pins $3 $2
match_devices $3 $1
match_devices $1 $4
end_circuit INV INV MATCH
END

    assert_equal(good, true)

  end

  def test_11

    nls = <<END
      circuit RESCUBE (A=A,B=B);
        device RES $1 (A=A,B=$1) (R=1000);
        device RES $2 (A=A,B=$2) (R=1000);
        device RES $3 (A=A,B=$3) (R=1000);
        device RES $4 (A=$1,B=$4) (R=1000);
        device RES $5 (A=$2,B=$4) (R=1000);
        device RES $6 (A=$2,B=$5) (R=1000);
        device RES $7 (A=$3,B=$5) (R=1000);
        device RES $8 (A=$3,B=$6) (R=1000);
        device RES $9 (A=$1,B=$6) (R=1000);
        device RES $9 (A=$4,B=B) (R=1000);
        device RES $10 (A=$5,B=B) (R=1000);
        device RES $11 (A=$6,B=B) (R=1000);
      end;
END

    nl = RBA::Netlist::new
    prep_nl(nl, nls)

    comp = RBA::NetlistComparer::new
    comp.join_symmetric_nets(nl.circuit_by_name("RESCUBE"))

    assert_equal(nl.to_s, <<END)
circuit RESCUBE (A=A,B=B);
  device RES $1 (A=A,B=$1) (R=1000,L=0,W=0,A=0,P=0);
  device RES $2 (A=A,B=$1) (R=1000,L=0,W=0,A=0,P=0);
  device RES $3 (A=A,B=$1) (R=1000,L=0,W=0,A=0,P=0);
  device RES $4 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);
  device RES $5 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);
  device RES $6 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);
  device RES $7 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);
  device RES $8 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);
  device RES $9 (A=$1,B=$4) (R=1000,L=0,W=0,A=0,P=0);
  device RES $10 (A=$4,B=B) (R=1000,L=0,W=0,A=0,P=0);
  device RES $11 (A=$4,B=B) (R=1000,L=0,W=0,A=0,P=0);
  device RES $12 (A=$4,B=B) (R=1000,L=0,W=0,A=0,P=0);
end;
END

    nl.combine_devices
    assert_equal(nl.to_s, <<END)
circuit RESCUBE (A=A,B=B);
  device RES $10 (A=A,B=B) (R=833.333333333,L=0,W=0,A=0,P=0);
end;
END
    
  end

end

load("test_epilogue.rb")


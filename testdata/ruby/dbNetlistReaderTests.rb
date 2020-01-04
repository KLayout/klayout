# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2020 Matthias Koefferlein
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

# For analysis
module RBA
  class Netlist
    attr_accessor :description
  end
end

class MyNetlistSpiceReaderDelegate < RBA::NetlistSpiceReaderDelegate

  def start(netlist)
    netlist.description = "Read by MyDelegate"
  end

  def finish(netlist)
    netlist.description = "Read by MyDelegate (sucessfully)"
  end

  def wants_subcircuit(name)
    name == "HVNMOS" || name == "HVPMOS"
  end

  def element(circuit, el, name, model, value, nets, params)

    if el != "X"
      return super
    end

    if nets.size != 4
      error("Subcircuit #{model} needs four nodes")
    end

    cls = circuit.netlist.device_class_by_name(model)
    if ! cls
      cls = RBA::DeviceClassMOS4Transistor::new
      cls.name = model
      circuit.netlist.add(cls)
    end

    device = circuit.create_device(cls, name)

    [ "S", "G", "D", "B" ].each_with_index do |t,index|
      device.connect_terminal(t, nets[index])
    end
    params.each do |p,value|
      device.set_parameter(p, value * 1.5)
    end

  end

end

class DBNetlistReaderTests_TestClass < TestBase

  def test_1_Basic

    nl = RBA::Netlist::new

    input = File.join($ut_testsrc, "testdata", "algo", "nreader6.cir")

    mydelegate = MyNetlistSpiceReaderDelegate::new
    reader = RBA::NetlistSpiceReader::new(mydelegate)
    # the delegate is kept by the SPICE writer ..
    mydelegate = nil
    GC.start
    nl.read(input, reader)

    assert_equal(nl.description, "Read by MyDelegate (sucessfully)")

    assert_equal(nl.to_s, <<"END")
circuit SUBCKT ($1=$1,A=A,VDD=VDD,Z=Z,GND=GND,GND$1=GND$1);
  device HVPMOS $1 (S=VDD,G=$3,D=Z,B=$1) (L=0.3,W=1.5,AS=0.27,AD=0.27,PS=3.24,PD=3.24);
  device HVPMOS $2 (S=VDD,G=A,D=$3,B=$1) (L=0.3,W=1.5,AS=0.27,AD=0.27,PS=3.24,PD=3.24);
  device HVNMOS $3 (S=GND,G=$3,D=GND,B=GND$1) (L=1.695,W=3.18,AS=0,AD=0,PS=9,PD=9);
  device HVNMOS $4 (S=GND,G=$3,D=Z,B=GND$1) (L=0.6,W=0.6,AS=0.285,AD=0.285,PS=1.74,PD=1.74);
  device HVNMOS $5 (S=GND,G=A,D=$3,B=GND$1) (L=0.6,W=0.6,AS=0.285,AD=0.285,PS=2.64,PD=2.64);
  device RES $1 (A=A,B=Z) (R=100000,L=0,W=0,A=0,P=0);
end;
circuit .TOP ();
  subcircuit SUBCKT SUBCKT ($1=IN,A=OUT,VDD=VDD,Z=Z,GND=VSS,GND$1=VSS);
end;
END

  end

  def test_2_WithError

    nl = RBA::Netlist::new

    input = File.join($ut_testsrc, "testdata", "algo", "nreader6.cir")

    mydelegate = MyNetlistSpiceReaderDelegate::new

    def mydelegate.element(circuit, el, name, model, value, nets, params)
      self.error("Nothing implemented")
    end

    reader = RBA::NetlistSpiceReader::new(mydelegate)
    # the delegate is kept by the SPICE writer ..
    mydelegate = nil
    GC.start

    msg = "no error happened!"
    begin
      nl.read(input, reader)
    rescue Exception => ex
      msg = ex.message
    end
    assert_equal(msg.sub(input, "<INPUT>"), "Nothing implemented in <INPUT>, line 22 in Netlist::read")

    assert_equal(nl.description, "Read by MyDelegate")

  end

end

load("test_epilogue.rb")



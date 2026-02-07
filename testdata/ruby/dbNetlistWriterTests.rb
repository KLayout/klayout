# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2026 Matthias Koefferlein
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

class MyNetlistSpiceWriterDelegate < RBA::NetlistSpiceWriterDelegate

  def write_header
    emit_line("*** My special header")
  end

  def write_device_intro(cls)
    emit_comment("My intro for class " + cls.name)
  end

  def write_device(dev)
    emit_comment("Before device " + dev.expanded_name)
    emit_comment("Terminal #1: " + net_to_string(dev.net_for_terminal(0)))
    emit_comment("Terminal #2: " + net_to_string(dev.net_for_terminal(1)))
    super(dev)
    emit_comment("After device " + dev.expanded_name)
  end

end

class DBNetlistWriterTests_TestClass < TestBase

  def test_1_Basic

    nl = RBA::Netlist::new

    rcls = RBA::DeviceClassResistor::new
    ccls = RBA::DeviceClassCapacitor::new
    lcls = RBA::DeviceClassInductor::new
    dcls = RBA::DeviceClassDiode::new
    m3cls = RBA::DeviceClassMOS3Transistor::new
    m4cls = RBA::DeviceClassMOS4Transistor::new

    rcls.name = "RCLS"
    lcls.name = "LCLS"
    ccls.name = "CCLS"
    dcls.name = "DCLS"
    m3cls.name = "M3CLS"
    m4cls.name = "M4CLS"

    nl.add(rcls)
    nl.add(lcls)
    nl.add(ccls)
    nl.add(dcls)
    nl.add(m3cls)
    nl.add(m4cls)

    circuit1 = RBA::Circuit::new
    circuit1.name = "C1"
    nl.add(circuit1)

    n1 = circuit1.create_net("n1")
    n2 = circuit1.create_net("n2")
    n3 = circuit1.create_net("n3")

    rdev1 = circuit1.create_device(rcls)
    rdev1.set_parameter(RBA::DeviceClassResistor::PARAM_R, 1.7)
    rdev2 = circuit1.create_device(rcls)
    rdev2.set_parameter(RBA::DeviceClassResistor::PARAM_R, 42e-6)

    pid1 = circuit1.create_pin("p1").id
    pid2 = circuit1.create_pin("p2").id

    circuit1.connect_pin(pid1, n1)
    circuit1.connect_pin(pid2, n2)

    rdev1.connect_terminal("A", n1)
    rdev1.connect_terminal("B", n3)
    rdev2.connect_terminal("A", n3)
    rdev2.connect_terminal("B", n2)

    # verify against the input

    input = File.join($ut_testsrc, "testdata", "algo", "nwriter_rba1_au.txt")

    writer = RBA::NetlistSpiceWriter::new
    tmp = File::join($ut_testtmp, "tmp1.txt")
    nl.write(tmp, writer)

    a = File.open(tmp, "r").read
    a = a.gsub(/e-00/, "e-0").gsub(/e-0/, "e-")

    b = File.open(input, "r").read
    b = a.gsub(/e-00/, "e-0").gsub(/e-0/, "e-")

    assert_equal(a, b)

    # verify against the input with delegate

    input = File.join($ut_testsrc, "testdata", "algo", "nwriter_rba2_au.txt")

    mydelegate = MyNetlistSpiceWriterDelegate::new
    writer = RBA::NetlistSpiceWriter::new(mydelegate)
    # the delegate is kept by the SPICE writer ..
    mydelegate = nil
    GC.start
    tmp = File::join($ut_testtmp, "tmp2.txt")
    nl.write(tmp, writer, "A comment")

    a = File.open(tmp, "r").read
    a = a.gsub(/e-00/, "e-0").gsub(/e-0/, "e-")

    b = File.open(input, "r").read
    b = a.gsub(/e-00/, "e-0").gsub(/e-0/, "e-")

    assert_equal(a, b)

  end

end

load("test_epilogue.rb")



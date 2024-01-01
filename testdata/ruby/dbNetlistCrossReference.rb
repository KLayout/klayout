
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

class DBNetlistCrossReference_TestClass < TestBase

  def test_1_Basic

    xref = RBA::NetlistCrossReference::new

    lvs = RBA::LayoutVsSchematic::new
    input = File.join($ut_testsrc, "testdata", "algo", "lvsdb_read_test.lvsdb")
    lvs.read(input)

    reader = RBA::NetlistSpiceReader::new
    nl = RBA::Netlist::new
    nl.read(File.join($ut_testsrc, "testdata", "algo", "lvs_test_1.spi"), reader)

    assert_equal(xref.circuit_count, 0)

    # A NetlistCrossReference object can act as a receiver for a netlist comparer
    comp = RBA::NetlistComparer::new
    comp.compare(lvs.netlist, nl, xref)

    assert_equal(xref.circuit_count, 3)

    xref.clear
    assert_equal(xref.circuit_count, 0)

  end

  def test_2_CircuitPairs

    lvs = RBA::LayoutVsSchematic::new
    input = File.join($ut_testsrc, "testdata", "algo", "lvsdb_read_test2.lvsdb")
    lvs.read(input)

    xref = lvs.xref
    assert_equal(xref.circuit_count, 4)

    info = []
    xref.each_circuit_pair do |cp|
      info << [ cp.first, cp.second ].collect { |s| s ? s.name : "(nil)" }.join("/") + ":" + cp.status.to_s
    end
    assert_equal(info.join(","), "(nil)/INV2PAIRX:Mismatch,INV2/INV2:Match,INV2PAIR/INV2PAIR:NoMatch,RINGO/RINGO:Skipped")

    cp_inv2 = nil
    xref.each_circuit_pair do |cp|
      if cp.first && cp.first.name == "INV2"
        cp_inv2 = cp
      end
    end

    assert_equal(cp_inv2 != nil, true)

    info = []
    xref.each_pin_pair(cp_inv2) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.name : "(nil)" }.join("/") + ":" + p.status.to_s
    end
    assert_equal(info.join(","), "/1:Match,BULK/6:Match,IN/2:Match,OUT/3:Match,VDD/5:Match,VSS/4:Match")
      
    info = []
    xref.each_net_pair(cp_inv2) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.name : "(nil)" }.join("/") + ":" + p.status.to_s
    end
    assert_equal(info.join(","), "/1:Match,BULK/6:Match,IN/2:Match,OUT/3:Match,VDD/5:Match,VSS/4:Match")

    netp_bulk = nil
    xref.each_net_pair(cp_inv2) do |p|
      if p.first.name == "BULK"
        netp_bulk = p
      end
    end
      
    info = []
    xref.each_net_terminal_pair(netp_bulk) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.terminal_def.name : "(nil)" }.join("/")
    end
    assert_equal(info.join(","), "B/B")

    info = []
    xref.each_net_pin_pair(netp_bulk) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.pin.name : "(nil)" }.join("/")
    end
    assert_equal(info.join(","), "BULK/6")

    info = []
    xref.each_net_subcircuit_pin_pair(netp_bulk) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.pin.name : "(nil)" }.join("/")
    end
    assert_equal(info.join(","), "")

    info = []
    xref.each_device_pair(cp_inv2) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.name : "(nil)" }.join("/") + ":" + p.status.to_s
    end
    assert_equal(info.join(","), "/$1:Match,/$3:Match")
      
    info = []
    xref.each_subcircuit_pair(cp_inv2) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.name : "(nil)" }.join("/") + ":" + p.status.to_s
    end
    assert_equal(info.join(","), "")
      
    cp_inv2pair = nil
    xref.each_circuit_pair do |cp|
      if cp.first && cp.first.name == "INV2PAIR"
        cp_inv2pair = cp
      end
    end

    assert_equal(cp_inv2pair != nil, true)

    info = []
    xref.each_pin_pair(cp_inv2pair) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.name : "(nil)" }.join("/") + ":" + p.status.to_s
    end
    assert_equal(info.join(","), "(nil)/5:Mismatch,/(nil):Mismatch,/2:Match,/3:Match,/4:Match,/6:Match,/7:Match,BULK/1:Match")
      
    info = []
    xref.each_net_pair(cp_inv2pair) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.name : "(nil)" }.join("/") + ":" + p.status.to_s
    end
    assert_equal(info.join(","), "/(nil):Mismatch,/2:Mismatch,/3:Mismatch,/4:Match,/6:Match,/7:Mismatch,BULK/1:Mismatch")
      
    info = []
    xref.each_device_pair(cp_inv2pair) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.name : "(nil)" }.join("/") + ":" + p.status.to_s
    end
    assert_equal(info.join(","), "")
      
    info = []
    xref.each_subcircuit_pair(cp_inv2pair) do |p|
      info << [ p.first, p.second ].collect { |s| s ? s.name : "(nil)" }.join("/") + ":" + p.status.to_s
    end
    assert_equal(info.join(","), "(nil)/$2:Mismatch,/(nil):Mismatch,/(nil):Mismatch")

  end

  def test_3_StatusEnums

    st = RBA::NetlistCrossReference::Status::new
    assert_equal(st.to_i, 0)
    assert_equal(st.to_s, "None")
  
    st = RBA::NetlistCrossReference::Status::Match
    assert_equal(st.to_s, "Match")

    st = RBA::NetlistCrossReference::Skipped
    assert_equal(st.to_s, "Skipped")

  end

end

load("test_epilogue.rb")




# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2025 Matthias Koefferlein
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

class DBLayoutVsSchematic_TestClass < TestBase

  def test_1_Basic

    lvs = RBA::LayoutVsSchematic::new

    assert_equal(lvs.xref == nil, true)
    assert_equal(lvs.reference == nil, true)

  end

  def test_2_Flow

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "lvs_test_1.gds"))

    lvs = RBA::LayoutVsSchematic::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))

    nwell       = lvs.make_layer(ly.layer(1,  0), "nwell")
    active      = lvs.make_layer(ly.layer(2,  0), "active")
    pplus       = lvs.make_layer(ly.layer(10, 0), "pplus")
    nplus       = lvs.make_layer(ly.layer(11, 0), "nplus")
    poly        = lvs.make_layer(ly.layer(3,  0), "poly")
    poly_lbl    = lvs.make_text_layer(ly.layer(3, 1), "poly_lbl")
    diff_cont   = lvs.make_layer(ly.layer(4,  0), "diff_cont")
    poly_cont   = lvs.make_layer(ly.layer(5,  0), "poly_cont")
    metal1      = lvs.make_layer(ly.layer(6,  0), "metal1")
    metal1_lbl  = lvs.make_text_layer(ly.layer(6,  1), "metal1_lbl")
    via1        = lvs.make_layer(ly.layer(7,  0), "via1")
    metal2      = lvs.make_layer(ly.layer(8,  0), "metal2")
    metal2_lbl  = lvs.make_text_layer(ly.layer(8,  1), "metal2_lbl")
    bulk        = lvs.make_layer

    # compute some layers
    active_in_nwell = active & nwell
    pactive = active_in_nwell & pplus
    ntie = active_in_nwell & nplus
    pgate = pactive & poly
    psd = pactive - pgate

    active_outside_nwell = active - nwell
    nactive = active_outside_nwell & nplus
    ptie = active_outside_nwell & pplus
    ngate = nactive & poly
    nsd = nactive - ngate

    # device extraction
    pmos_ex = RBA::DeviceExtractorMOS4Transistor::new("PMOS")
    dl = { "SD" => psd, "G" => pgate, "P" => poly, "W" => nwell }
    lvs.extract_devices(pmos_ex, dl)

    nmos_ex = RBA::DeviceExtractorMOS4Transistor::new("NMOS")
    dl = { "SD" => nsd, "G" => ngate, "P" => poly, "W" => bulk }
    lvs.extract_devices(nmos_ex, dl)

    # register derived layers for connectivity
    lvs.register(psd, "psd")
    lvs.register(nsd, "nsd")
    lvs.register(ptie, "ptie")
    lvs.register(ntie, "ntie")

    # intra-layer
    lvs.connect(psd)
    lvs.connect(nsd)
    lvs.connect(nwell)
    lvs.connect(poly)
    lvs.connect(diff_cont)
    lvs.connect(poly_cont)
    lvs.connect(metal1)
    lvs.connect(via1)
    lvs.connect(metal2)
    lvs.connect(ptie)
    lvs.connect(ntie)
    
    # inter-layer
    lvs.connect(psd,       diff_cont)
    lvs.connect(nsd,       diff_cont)
    lvs.connect(poly,      poly_cont)
    lvs.connect(poly_cont, metal1)
    lvs.connect(diff_cont, metal1)
    lvs.connect(diff_cont, ptie)
    lvs.connect(diff_cont, ntie)
    lvs.connect(nwell,     ntie)
    lvs.connect(metal1,    via1)
    lvs.connect(via1,      metal2)
    lvs.connect(poly,      poly_lbl)     #  attaches labels
    lvs.connect(metal1,    metal1_lbl)   #  attaches labels
    lvs.connect(metal2,    metal2_lbl)   #  attaches labels
    
    # global
    lvs.connect_global(ptie, "BULK")
    lvs.connect_global(bulk, "BULK")

    lvs.extract_netlist

    lvs.netlist.combine_devices

    lvs.netlist.make_top_level_pins
    lvs.netlist.purge

    # read the reference netlist
    reader = RBA::NetlistSpiceReader::new
    nl = RBA::Netlist::new
    nl.read(File.join($ut_testsrc, "testdata", "algo", "lvs_test_1.spi"), reader)

    assert_equal(lvs.reference == nil, true)
    lvs.reference = nl
    assert_equal(lvs.reference == nl, true)

    # do the actual compare
    comparer = RBA::NetlistComparer::new
    res = lvs.compare(comparer)
    assert_equal(res, true)

    assert_equal(lvs.xref != nil, true)

  end

  def test_3_ReadAndWrite

    lvs = RBA::LayoutVsSchematic::new

    input = File.join($ut_testsrc, "testdata", "algo", "lvsdb_read_test.lvsdb")
    lvs.read(input)

    tmp = File::join($ut_testtmp, "tmp.lvsdb")
    lvs.write(tmp)

    assert_equal(File.open(tmp, "r").read, File.open(input, "r").read)

    assert_equal(lvs.layer_names.join(","), "bulk,nwell,poly,poly_lbl,diff_cont,poly_cont,metal1,metal1_lbl,via1,metal2,metal2_lbl,ntie,psd,ptie,nsd")
    assert_equal(lvs.layer_name(lvs.layer_by_name("metal1")), "metal1")
    assert_equal(lvs.layer_name(lvs.layer_by_index(lvs.layer_of(lvs.layer_by_name("metal1")))), "metal1")

    tmp = File::join($ut_testtmp, "tmp.l2n")
    lvs.write_l2n(tmp)

    l2n = RBA::LayoutToNetlist::new
    l2n.read(tmp)
    assert_equal(l2n.layer_names.join(","), "bulk,nwell,poly,poly_lbl,diff_cont,poly_cont,metal1,metal1_lbl,via1,metal2,metal2_lbl,ntie,psd,ptie,nsd")

    lvs2 = RBA::LayoutVsSchematic::new
    lvs2.read_l2n(tmp)
    assert_equal(lvs2.layer_names.join(","), "bulk,nwell,poly,poly_lbl,diff_cont,poly_cont,metal1,metal1_lbl,via1,metal2,metal2_lbl,ntie,psd,ptie,nsd")

  end

end

load("test_epilogue.rb")


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

class DBLayoutToNetlist_TestClass < TestBase

  def test_1_Basic

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "device_extract_l1.gds"))

    l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))

    l2n.threads = 17
    l2n.max_vertex_count = 42
    l2n.area_ratio = 7.5
    assert_equal(l2n.threads, 17)
    assert_equal(l2n.max_vertex_count, 42)
    assert_equal(l2n.area_ratio, 7.5)

    r = l2n.make_layer(ly.layer(6, 0))

    assert_not_equal(l2n.internal_layout.object_id, ly.object_id)
    assert_equal(l2n.internal_layout.top_cell.name, ly.top_cell.name)
    assert_equal(l2n.internal_top_cell.name, ly.top_cell.name)

    assert_not_equal(l2n.layer_of(r), ly.layer(6, 0))  # would be a strange coincidence ... 

    cm = l2n.const_cell_mapping_into(ly, ly.top_cell)
    (0 .. l2n.internal_layout.cells - 1).each do |ci|
      assert_equal(l2n.internal_layout.cell(ci).name, ly.cell(cm.cell_mapping(ci)).name)
    end  

    ly2 = RBA::Layout::new
    ly2.create_cell(ly.top_cell.name)

    cm = l2n.cell_mapping_into(ly2, ly2.top_cell)
    assert_equal(ly2.cells, ly.cells)
    (0 .. l2n.internal_layout.cells - 1).each do |ci|
      assert_equal(l2n.internal_layout.cell(ci).name, ly2.cell(cm.cell_mapping(ci)).name)
    end

    rmetal1 = l2n.make_polygon_layer( ly.layer(6, 0), "metal1" )
    bulk_id = l2n.connect_global(rmetal1, "BULK")
    assert_equal(l2n.global_net_name(bulk_id), "BULK")

    # cell mapping with nets

    l2n = RBA::LayoutToNetlist::new
    l2n.read(File.join($ut_testsrc, "testdata", "algo", "l2n_reader_in.txt"))

    nets = [
      l2n.netlist.circuit_by_name("RINGO").net_by_name("VSS"),
      l2n.netlist.circuit_by_name("RINGO").net_by_name("VDD")
    ]

    ly2 = RBA::Layout::new
    ly2.create_cell("TOP")

    cm = l2n.cell_mapping_into(ly2, ly2.top_cell, nets)

    map = (0 .. l2n.internal_layout.cells - 1).collect do |ci|
      cm.has_mapping?(ci) && (l2n.internal_layout.cell(ci).name + "=>" + ly2.cell(cm.cell_mapping(ci)).name)
    end
    assert_equal(map.select { |i| i }.join(","), "RINGO=>TOP")

    nets = [
      l2n.netlist.circuit_by_name("INV2").net_by_name("IN"),
    ]

    ly2 = RBA::Layout::new
    ly2.create_cell("TOP")

    cm = l2n.cell_mapping_into(ly2, ly2.top_cell, nets)

    map = (0 .. l2n.internal_layout.cells - 1).collect do |ci|
      cm.has_mapping?(ci) && (l2n.internal_layout.cell(ci).name + "=>" + ly2.cell(cm.cell_mapping(ci)).name)
    end
    assert_equal(map.select { |i| i }.join(","), "RINGO=>TOP,INV2=>INV2")

    # extended attributes for extract_netlist

    l2n = RBA::LayoutToNetlist::new
    l2n.include_floating_subcircuits = true
    assert_equal(l2n.include_floating_subcircuits, true)
    l2n.include_floating_subcircuits = false
    assert_equal(l2n.include_floating_subcircuits, false)

    assert_equal(l2n.dump_joined_nets, "")
    l2n.join_nets([ "VDD", "NWELL" ])
    l2n.join_nets([ "VSS", "BULK" ])
    assert_equal(l2n.dump_joined_nets, "NWELL+VDD,BULK+VSS")
    l2n.clear_join_nets
    assert_equal(l2n.dump_joined_nets, "")
    assert_equal(l2n.dump_joined_nets_per_cell, "")

    l2n.join_nets("INV*", [ "VDD", "NWELL" ])
    l2n.join_nets("ND2*", [ "VSS", "BULK" ])
    assert_equal(l2n.dump_joined_nets_per_cell, "INV*:NWELL+VDD,ND2*:BULK+VSS")

    l2n.clear_join_nets
    assert_equal(l2n.dump_joined_nets, "")
    assert_equal(l2n.dump_joined_nets_per_cell, "")

    assert_equal(l2n.dump_joined_net_names, "")
    l2n.join_net_names("VDD")
    l2n.join_net_names("VSS")
    assert_equal(l2n.dump_joined_net_names, "VDD,VSS")
    l2n.clear_join_net_names
    assert_equal(l2n.dump_joined_net_names, "")
    assert_equal(l2n.dump_joined_net_names_per_cell, "")

    l2n.join_net_names("INV*", "VDD")
    l2n.join_net_names("ND2*", "VSS")
    assert_equal(l2n.dump_joined_net_names_per_cell, "INV*:VDD,ND2*:VSS")

    l2n.clear_join_net_names
    assert_equal(l2n.dump_joined_net_names, "")
    assert_equal(l2n.dump_joined_net_names_per_cell, "")

  end

  def test_2_ShapesFromNet

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "device_extract_l1_with_inv_nodes.gds"))

    l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))

    # only plain backend connectivity

    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0), "metal1" )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1), "metal1_lbl" )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0), "via1" )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0), "metal2" )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1), "metal2_lbl" )
    
    # Intra-layer
    l2n.connect(rmetal1)
    l2n.connect(rvia1)
    l2n.connect(rmetal2)

    # Inter-layer
    l2n.connect(rmetal1,    rvia1)
    l2n.connect(rvia1,      rmetal2)
    l2n.connect(rmetal1,    rmetal1_lbl)   #  attaches labels
    l2n.connect(rmetal2,    rmetal2_lbl)   #  attaches labels

    # Perform netlist extraction 
    l2n.extract_netlist

    nl_string = l2n.netlist.to_s
    assert_equal(nl_string, <<END)
circuit TRANS ($1=$1,$2=$2);
end;
circuit INV2 (OUT=OUT,$2=$3,$3=$4);
  subcircuit TRANS $1 ($1=$4,$2=OUT);
  subcircuit TRANS $2 ($1=$3,$2=OUT);
  subcircuit TRANS $3 ($1=$2,$2=$4);
  subcircuit TRANS $4 ($1=$2,$2=$3);
end;
circuit RINGO ();
  subcircuit INV2 $1 (OUT='FB,OSC',$2=VSS,$3=VDD);
  subcircuit INV2 $2 (OUT=$I20,$2=VSS,$3=VDD);
  subcircuit INV2 $3 (OUT=$I19,$2=VSS,$3=VDD);
  subcircuit INV2 $4 (OUT=$I21,$2=VSS,$3=VDD);
  subcircuit INV2 $5 (OUT=$I22,$2=VSS,$3=VDD);
  subcircuit INV2 $6 (OUT=$I23,$2=VSS,$3=VDD);
  subcircuit INV2 $7 (OUT=$I24,$2=VSS,$3=VDD);
  subcircuit INV2 $8 (OUT=$I25,$2=VSS,$3=VDD);
  subcircuit INV2 $9 (OUT=$I26,$2=VSS,$3=VDD);
  subcircuit INV2 $10 (OUT=$I27,$2=VSS,$3=VDD);
end;
END

    assert_equal(l2n.probe_net(rmetal2, RBA::DPoint::new(0.0, 1.8)).to_s, "RINGO:FB,OSC")
    sc_path = []
    assert_equal(l2n.probe_net(rmetal2, RBA::DPoint::new(0.0, 1.8), sc_path).to_s, "RINGO:FB,OSC")
    assert_equal(sc_path.size, 0)
    assert_equal(l2n.probe_net(rmetal2, RBA::DPoint::new(-2.0, 1.8)).inspect, "nil")

    n = l2n.probe_net(rmetal1, RBA::Point::new(2600, 1000), nil)
    assert_equal(n.to_s, "INV2:$2")
    sc_path = []
    n = l2n.probe_net(rmetal1, RBA::Point::new(2600, 1000), sc_path)
    assert_equal(n.to_s, "INV2:$2")
    assert_equal(sc_path.size, 1)
    assert_equal(sc_path.collect(&:expanded_name).join(","), "$2")
    assert_equal(sc_path.collect(&:trans).inject(&:*).to_s, "r0 *1 2.64,0")

    assert_equal(l2n.shapes_of_net(n, rmetal1, true).to_s, 
        "(-980,-420;-980,2420;-620,2420;-620,-420);(-800,820;-800,1180;580,1180;580,820);(-980,2420;-980,3180;-620,3180;-620,2420);(-980,-380;-980,380;-620,380;-620,-380)")

    shapes = RBA::Shapes::new
    l2n.shapes_of_net(n, rmetal1, true, shapes)
    r = RBA::Region::new
    shapes.each { |s| r.insert(s.polygon) }
    assert_equal(r.to_s, 
        "(-980,-420;-980,2420;-620,2420;-620,-420);(-800,820;-800,1180;580,1180;580,820);(-980,2420;-980,3180;-620,3180;-620,2420);(-980,-380;-980,380;-620,380;-620,-380)")

    assert_equal(l2n.is_extracted?, true)
    l2n.reset_extracted
    assert_equal(l2n.is_extracted?, false)
    assert_equal(l2n.netlist.inspect, "nil")

    l2n.extract_netlist
    assert_equal(l2n.is_extracted?, true)
    assert_equal(l2n.netlist.to_s, nl_string)

  end
  
  def test_10_LayoutToNetlistExtractionWithoutDevices

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "device_extract_l1.gds"))

    l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))

    # only plain connectivity

    ractive     = l2n.make_layer(         ly.layer(2, 0), "active" )
    rpoly       = l2n.make_polygon_layer( ly.layer(3, 0), "poly" )
    rpoly_lbl   = l2n.make_text_layer(    ly.layer(3, 1), "poly_lbl" )
    rdiff_cont  = l2n.make_polygon_layer( ly.layer(4, 0), "diff_cont" )
    rpoly_cont  = l2n.make_polygon_layer( ly.layer(5, 0), "poly_cont" )
    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0), "metal1" )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1), "metal1_lbl" )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0), "via1" )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0), "metal2" )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1), "metal2_lbl" )
    
    rsd         = ractive - rpoly

    l2n.register(rsd, "sd")

    # Intra-layer
    l2n.connect(rsd)
    l2n.connect(rpoly)
    l2n.connect(rdiff_cont)
    l2n.connect(rpoly_cont)
    l2n.connect(rmetal1)
    l2n.connect(rvia1)
    l2n.connect(rmetal2)

    # Inter-layer
    l2n.connect(rsd,        rdiff_cont)
    l2n.connect(rpoly,      rpoly_cont)
    l2n.connect(rpoly_cont, rmetal1)
    l2n.connect(rdiff_cont, rmetal1)
    l2n.connect(rmetal1,    rvia1)
    l2n.connect(rvia1,      rmetal2)
    l2n.connect(rpoly,      rpoly_lbl)     #  attaches labels
    l2n.connect(rmetal1,    rmetal1_lbl)   #  attaches labels
    l2n.connect(rmetal2,    rmetal2_lbl)   #  attaches labels

    # Perform netlist extraction 
    l2n.extract_netlist

    assert_equal(l2n.netlist.to_s, <<END)
circuit TRANS ($1=$1,$2=$2,$3=$3);
end;
circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);
  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);
  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);
  subcircuit TRANS $3 ($1=$5,$2=OUT,$3=$2);
  subcircuit TRANS $4 ($1=$4,$2=OUT,$3=$2);
end;
circuit RINGO ();
  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);
  subcircuit INV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD);
  subcircuit INV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD);
  subcircuit INV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD);
  subcircuit INV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD);
  subcircuit INV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD);
  subcircuit INV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD);
  subcircuit INV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD);
  subcircuit INV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD);
  subcircuit INV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD);
end;
END

  end

  def test_11_LayoutToNetlistExtractionWithDevices

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "device_extract_l1.gds"))

    l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))

    rnwell      = l2n.make_layer(         ly.layer(1, 0), "nwell" )
    ractive     = l2n.make_layer(         ly.layer(2, 0), "active" )
    rpoly       = l2n.make_polygon_layer( ly.layer(3, 0), "poly" )
    rpoly_lbl   = l2n.make_text_layer(    ly.layer(3, 1), "poly_lbl" )
    rdiff_cont  = l2n.make_polygon_layer( ly.layer(4, 0), "diff_cont" )
    rpoly_cont  = l2n.make_polygon_layer( ly.layer(5, 0), "poly_cont" )
    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0), "metal1" )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1), "metal1_lbl" )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0), "via1" )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0), "metal2" )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1), "metal2_lbl" )

    rpactive    = ractive & rnwell
    rpgate      = rpactive & rpoly
    rpsd        = rpactive - rpgate

    rnactive    = ractive - rnwell
    rngate      = rnactive & rpoly
    rnsd        = rnactive - rngate
    
    # PMOS transistor device extraction
    pmos_ex = RBA::DeviceExtractorMOS3Transistor::new("PMOS")
    l2n.extract_devices(pmos_ex, { "SD" => rpsd, "G" => rpgate, "P" => rpoly })

    # NMOS transistor device extraction
    nmos_ex = RBA::DeviceExtractorMOS3Transistor::new("NMOS")
    l2n.extract_devices(nmos_ex, { "SD" => rnsd, "G" => rngate, "P" => rpoly })

    # Define connectivity for netlist extraction

    l2n.register(rpsd, "psd")
    l2n.register(rnsd, "nsd")

    # Intra-layer
    l2n.connect(rpsd)
    l2n.connect(rnsd)
    l2n.connect(rpoly)
    l2n.connect(rdiff_cont)
    l2n.connect(rpoly_cont)
    l2n.connect(rmetal1)
    l2n.connect(rvia1)
    l2n.connect(rmetal2)

    # Inter-layer
    l2n.connect(rpsd,       rdiff_cont)
    l2n.connect(rnsd,       rdiff_cont)
    l2n.connect(rpoly,      rpoly_cont)
    l2n.connect(rpoly_cont, rmetal1)
    l2n.connect(rdiff_cont, rmetal1)
    l2n.connect(rmetal1,    rvia1)
    l2n.connect(rvia1,      rmetal2)
    l2n.connect(rpoly,      rpoly_lbl)     #  attaches labels
    l2n.connect(rmetal1,    rmetal1_lbl)   #  attaches labels
    l2n.connect(rmetal2,    rmetal2_lbl)   #  attaches labels
    
    # Perform netlist extraction 
    l2n.extract_netlist

    assert_equal(l2n.netlist.to_s, <<END)
circuit RINGO ();
  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);
  subcircuit INV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD);
  subcircuit INV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD);
  subcircuit INV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD);
  subcircuit INV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD);
  subcircuit INV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD);
  subcircuit INV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD);
  subcircuit INV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD);
  subcircuit INV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD);
  subcircuit INV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD);
end;
circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);
  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);
  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);
  subcircuit TRANS $3 ($1=$5,$2=OUT,$3=$2);
  subcircuit TRANS $4 ($1=$4,$2=OUT,$3=$2);
end;
circuit TRANS ($1=$1,$2=$2,$3=$3);
end;
END

    # cleanup now
    l2n._destroy

  end

  def test_12_LayoutToNetlistExtractionWithDevicesAndGlobalNets

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "device_extract_l3.gds"))

    l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))

    rbulk       = l2n.make_layer(                           "bulk" )
    rnwell      = l2n.make_polygon_layer( ly.layer(1, 0)  , "nwell" )
    ractive     = l2n.make_polygon_layer( ly.layer(2, 0)  , "active" )
    rpoly       = l2n.make_polygon_layer( ly.layer(3, 0)  , "poly" )
    rpoly_lbl   = l2n.make_text_layer(    ly.layer(3, 1)  , "poly_lbl" )
    rdiff_cont  = l2n.make_polygon_layer( ly.layer(4, 0)  , "diff_cont" )
    rpoly_cont  = l2n.make_polygon_layer( ly.layer(5, 0)  , "poly_cont" )
    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0)  , "metal1" )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1)  , "metal1_lbl" )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0)  , "via1" )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0)  , "metal2" )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1)  , "metal2_lbl" )
    rpplus      = l2n.make_polygon_layer( ly.layer(10, 0) , "pplus" )
    rnplus      = l2n.make_polygon_layer( ly.layer(11, 0) , "nplus" )

    ractive_in_nwell = ractive & rnwell
    rpactive    = ractive_in_nwell & rpplus
    rntie       = ractive_in_nwell & rnplus
    rpgate      = rpactive & rpoly
    rpsd        = rpactive - rpgate

    ractive_outside_nwell = ractive - rnwell
    rnactive    = ractive_outside_nwell & rnplus
    rptie       = ractive_outside_nwell & rpplus
    rngate      = rnactive & rpoly
    rnsd        = rnactive - rngate

    # PMOS transistor device extraction
    pmos_ex = RBA::DeviceExtractorMOS4Transistor::new("PMOS")
    l2n.extract_devices(pmos_ex, { "SD" => rpsd, "G" => rpgate, "P" => rpoly, "W" => rnwell })

    # NMOS transistor device extraction
    nmos_ex = RBA::DeviceExtractorMOS4Transistor::new("NMOS")
    l2n.extract_devices(nmos_ex, { "SD" => rnsd, "G" => rngate, "P" => rpoly, "W" => rbulk })

    # Define connectivity for netlist extraction

    l2n.register(rpsd, "psd")
    l2n.register(rnsd, "nsd")
    l2n.register(rptie, "ptie")
    l2n.register(rntie, "ntie")

    # Intra-layer
    l2n.connect(rpsd)
    l2n.connect(rnsd)
    l2n.connect(rnwell)
    l2n.connect(rpoly)
    l2n.connect(rdiff_cont)
    l2n.connect(rpoly_cont)
    l2n.connect(rmetal1)
    l2n.connect(rvia1)
    l2n.connect(rmetal2)
    l2n.connect(rptie)
    l2n.connect(rntie)

    # Inter-layer
    l2n.connect(rpsd,       rdiff_cont)
    l2n.connect(rnsd,       rdiff_cont)
    l2n.connect(rpoly,      rpoly_cont)
    l2n.connect(rpoly_cont, rmetal1)
    l2n.connect(rdiff_cont, rmetal1)
    l2n.connect(rdiff_cont, rntie)
    l2n.connect(rdiff_cont, rptie)
    l2n.connect(rnwell,     rntie)
    l2n.connect(rmetal1,    rvia1)
    l2n.connect(rvia1,      rmetal2)
    l2n.connect(rpoly,      rpoly_lbl)     #  attaches labels
    l2n.connect(rmetal1,    rmetal1_lbl)   #  attaches labels
    l2n.connect(rmetal2,    rmetal2_lbl)   #  attaches labels

    # Global connections
    l2n.connect_global(rptie, "BULK")
    l2n.connect_global(rbulk, "BULK")
    
    # Perform netlist extraction 
    l2n.extract_netlist

    assert_equal(l2n.netlist.to_s, <<END)
circuit RINGO ();
  subcircuit INV2PAIR $1 (BULK=VSS,$2=FB,$3=VDD,$4=VSS,$5=$I11,$6=OSC,$7=VDD);
  subcircuit INV2PAIR $2 (BULK=VSS,$2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I17,$7=VDD);
  subcircuit INV2PAIR $3 (BULK=VSS,$2=$I23,$3=VDD,$4=VSS,$5=$I17,$6=$I9,$7=VDD);
  subcircuit INV2PAIR $4 (BULK=VSS,$2=$I24,$3=VDD,$4=VSS,$5=$I9,$6=$I10,$7=VDD);
  subcircuit INV2PAIR $5 (BULK=VSS,$2=$I25,$3=VDD,$4=VSS,$5=$I10,$6=$I11,$7=VDD);
end;
circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);
  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);
  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);
end;
circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);
  device PMOS $1 (S=$3,G=IN,D=VDD,B=$1) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=$3,D=OUT,B=$1) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
  device NMOS $3 (S=$3,G=IN,D=VSS,B=BULK) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=VSS,G=$3,D=OUT,B=BULK) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
  subcircuit TRANS $1 ($1=$3,$2=VSS,$3=IN);
  subcircuit TRANS $2 ($1=$3,$2=VDD,$3=IN);
  subcircuit TRANS $3 ($1=VDD,$2=OUT,$3=$3);
  subcircuit TRANS $4 ($1=VSS,$2=OUT,$3=$3);
end;
circuit TRANS ($1=$1,$2=$2,$3=$3);
end;
END

    l2n.netlist.combine_devices
    l2n.netlist.make_top_level_pins
    l2n.netlist.purge

    assert_equal(l2n.netlist.to_s, <<END)
circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);
  subcircuit INV2PAIR $1 (BULK=VSS,$2=FB,$3=VDD,$4=VSS,$5=$I11,$6=OSC,$7=VDD);
  subcircuit INV2PAIR $2 (BULK=VSS,$2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I17,$7=VDD);
  subcircuit INV2PAIR $3 (BULK=VSS,$2=$I23,$3=VDD,$4=VSS,$5=$I17,$6=$I9,$7=VDD);
  subcircuit INV2PAIR $4 (BULK=VSS,$2=$I24,$3=VDD,$4=VSS,$5=$I9,$6=$I10,$7=VDD);
  subcircuit INV2PAIR $5 (BULK=VSS,$2=$I25,$3=VDD,$4=VSS,$5=$I10,$6=$I11,$7=VDD);
end;
circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);
  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);
  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);
end;
circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);
  device PMOS $1 (S=$3,G=IN,D=VDD,B=$1) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=$3,D=OUT,B=$1) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
  device NMOS $3 (S=$3,G=IN,D=VSS,B=BULK) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=VSS,G=$3,D=OUT,B=BULK) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
end;
END

    # cleanup now
    l2n._destroy

  end

  def test_13_ReadAndWrite

    l2n = RBA::LayoutToNetlist::new

    input = File.join($ut_testsrc, "testdata", "algo", "l2n_reader_in.txt")
    l2n.read(input)

    tmp = File::join($ut_testtmp, "tmp.txt")
    l2n.write(tmp)

    assert_equal(File.open(tmp, "r").read, File.open(input, "r").read)

    assert_equal(l2n.layer_names.join(","), "poly,poly_lbl,diff_cont,poly_cont,metal1,metal1_lbl,via1,metal2,metal2_lbl,psd,nsd")
    assert_equal(l2n.layer_indexes.join(","), "1,2,3,4,5,6,7,8,9,10,11")
    assert_equal(l2n.layer_indexes.collect { |li| l2n.layer_info(li).to_s }.join(","), "3/0,3/1,4/0,5/0,6/0,6/1,7/0,8/0,8/1,,")
    assert_equal(l2n.layer_name(l2n.layer_by_name("metal1")), "metal1")
    assert_equal(l2n.layer_name(l2n.layer_by_index(l2n.layer_of(l2n.layer_by_name("metal1")))), "metal1")

  end

  def shape_to_s(s)
    if s.is_box || s.is_polygon
      s.polygon.to_s
    elsif s.is_text
      s.text.to_s
    else
      "nn"
    end
  end

  def compare_layouts(ly, au_file, tmp_file = nil)

    ret = true

    begin
      ly_au = RBA::Layout::new
      ly_au.read(au_file)
    rescue
      ly.write(tmp_file)
      puts "Actual layout written to: #{tmp_file}"
      raise
    end

    lmap = {}

    [ ly, ly_au ].each_with_index do |l,i|
      l.layer_indexes.each do |li|
        info = l.get_info(li)
        lmap[info] ||= [ nil, nil ]
        lmap[info][i] = li
      end
    end

    lmap.each do |info,lis|
      s    = !lis[0] ? "" : ly   .top_cell.begin_shapes_rec(lis[0]).each.collect { |i| shape_to_s(i.shape) }.sort.join("\n")
      s_au = !lis[1] ? "" : ly_au.top_cell.begin_shapes_rec(lis[1]).each.collect { |i| shape_to_s(i.shape) }.sort.join("\n")
      if s != s_au
        puts "Layer #{info}:\nActual shapes:\n#{s}\nGolden layout shapes:\n#{s_au}"
        ret = false
      end
    end

    if !ret && tmp_file
      ly.write(tmp_file)
      puts "Golden: #{au_file}"
      puts "Actual: #{tmp_file}"
    end

    return ret

  end

  def test_14_BuildNets

    l2n = RBA::LayoutToNetlist::new

    input = File.join($ut_testsrc, "testdata", "algo", "l2n_reader_in.txt")
    l2n.read(input)

    # build_all_nets

    ly = RBA::Layout::new
    ly.create_cell("TOP")

    cm = l2n.cell_mapping_into(ly, ly.top_cell)

    lmap = { 
      ly.insert_layer(RBA::LayerInfo::new(10, 0)) => l2n.layer_by_name("psd"),
      ly.insert_layer(RBA::LayerInfo::new(11, 0)) => l2n.layer_by_name("nsd"),
      ly.insert_layer(RBA::LayerInfo::new(3, 0)) => l2n.layer_by_name("poly"),
      ly.insert_layer(RBA::LayerInfo::new(4, 0)) => l2n.layer_by_name("diff_cont"),
      ly.insert_layer(RBA::LayerInfo::new(5, 0)) => l2n.layer_by_name("poly_cont"),
      ly.insert_layer(RBA::LayerInfo::new(6, 0)) => l2n.layer_by_name("metal1"),
      ly.insert_layer(RBA::LayerInfo::new(7, 0)) => l2n.layer_by_name("via1"),
      ly.insert_layer(RBA::LayerInfo::new(8, 0)) => l2n.layer_by_name("metal2")
    }

    l2n.build_all_nets(cm, ly, lmap, "NET_", nil, RBA::LayoutToNetlist::BNH_Disconnected, nil, "DEVICE_")

    au_file = File.join($ut_testsrc, "testdata", "algo", "l2n_reader_au_1.gds")
    tmp_file = File.join($ut_testtmp, "l2n_reader_1.gds")

    assert_equal(true, compare_layouts(ly, au_file, tmp_file))

    # build_all_nets with int-to-int layer map

    ly = RBA::Layout::new
    ly.create_cell("TOP")

    cm = l2n.cell_mapping_into(ly, ly.top_cell)

    lmap = { 
      ly.insert_layer(RBA::LayerInfo::new(10, 0)) => l2n.layer_index("psd"),
      ly.insert_layer(RBA::LayerInfo::new(11, 0)) => l2n.layer_index("nsd"),
      ly.insert_layer(RBA::LayerInfo::new(3, 0)) => l2n.layer_index("poly"),
      ly.insert_layer(RBA::LayerInfo::new(4, 0)) => l2n.layer_index("diff_cont"),
      ly.insert_layer(RBA::LayerInfo::new(5, 0)) => l2n.layer_index("poly_cont"),
      ly.insert_layer(RBA::LayerInfo::new(6, 0)) => l2n.layer_index("metal1"),
      ly.insert_layer(RBA::LayerInfo::new(7, 0)) => l2n.layer_index("via1"),
      ly.insert_layer(RBA::LayerInfo::new(8, 0)) => l2n.layer_index("metal2")
    }

    l2n.build_all_nets(cm, ly, lmap, "NET_", nil, RBA::LayoutToNetlist::BNH_Disconnected, nil, "DEVICE_")

    au_file = File.join($ut_testsrc, "testdata", "algo", "l2n_reader_au_1.gds")
    tmp_file = File.join($ut_testtmp, "l2n_reader_1.gds")

    assert_equal(true, compare_layouts(ly, au_file, tmp_file))

    # build_all_nets with auto layer map

    ly = RBA::Layout::new
    ly.create_cell("TOP")

    cm = l2n.cell_mapping_into(ly, ly.top_cell)

    l2n.build_all_nets(cm, ly, nil, "NET_", nil, RBA::LayoutToNetlist::BNH_Disconnected, nil, "DEVICE_")

    au_file = File.join($ut_testsrc, "testdata", "algo", "l2n_lmap_au.gds")
    tmp_file = File.join($ut_testtmp, "l2n_lmap.gds")

    assert_equal(true, compare_layouts(ly, au_file, tmp_file))

    # build_nets

    ly = RBA::Layout::new
    ly.create_cell("TOP")

    cm = l2n.cell_mapping_into(ly, ly.top_cell)

    lmap = { 
      ly.insert_layer(RBA::LayerInfo::new(10, 0)) => l2n.layer_by_name("psd"),
      ly.insert_layer(RBA::LayerInfo::new(11, 0)) => l2n.layer_by_name("nsd"),
      ly.insert_layer(RBA::LayerInfo::new(3, 0)) => l2n.layer_by_name("poly"),
      ly.insert_layer(RBA::LayerInfo::new(4, 0)) => l2n.layer_by_name("diff_cont"),
      ly.insert_layer(RBA::LayerInfo::new(5, 0)) => l2n.layer_by_name("poly_cont"),
      ly.insert_layer(RBA::LayerInfo::new(6, 0)) => l2n.layer_by_name("metal1"),
      ly.insert_layer(RBA::LayerInfo::new(7, 0)) => l2n.layer_by_name("via1"),
      ly.insert_layer(RBA::LayerInfo::new(8, 0)) => l2n.layer_by_name("metal2")
    }

    nets = [
      l2n.netlist.circuit_by_name("RINGO").net_by_name("VSS"),
      l2n.netlist.circuit_by_name("RINGO").net_by_name("VDD")
    ]

    l2n.build_nets(nets, cm, ly, lmap, "NET_", nil, RBA::LayoutToNetlist::BNH_SubcircuitCells, "CIRCUIT_", "DEVICE_")

    au_file = File.join($ut_testsrc, "testdata", "algo", "l2n_reader_au_1d.gds")
    tmp_file = File.join($ut_testtmp, "l2n_reader_1d.gds")

    assert_equal(true, compare_layouts(ly, au_file, tmp_file))

    # build_nets

    ly = RBA::Layout::new
    ly.create_cell("TOP")

    cm = l2n.cell_mapping_into(ly, ly.top_cell)

    nets = [
      l2n.netlist.circuit_by_name("RINGO").net_by_name("VSS"),
      l2n.netlist.circuit_by_name("RINGO").net_by_name("VDD")
    ]

    l2n.build_nets(nets, cm, ly, nil, "NET_", nil, RBA::LayoutToNetlist::BNH_SubcircuitCells, "CIRCUIT_", "DEVICE_")

    au_file = File.join($ut_testsrc, "testdata", "algo", "l2n_lmap_au_2.gds")
    tmp_file = File.join($ut_testtmp, "l2n_lmap_2.gds")

    assert_equal(true, compare_layouts(ly, au_file, tmp_file))

  end

  def test_15_BuildNetShapes

    l2n = RBA::LayoutToNetlist::new

    input = File.join($ut_testsrc, "testdata", "algo", "l2n_reader_in.txt")
    l2n.read(input)

    # build_all_nets using Region#net

    metal1 = l2n.layer_by_name("metal1")

    metal1_all = metal1.nets(l2n)
    metal1_vdd = metal1.nets(l2n, nil, l2n.netlist.nets_by_name("VDD"))
    metal1_all_wp = metal1.nets(l2n, 1)

    ly = RBA::Layout::new
    tc = ly.create_cell("TOP")
    metal1_all.insert_into(ly, tc.cell_index, ly.layer(1, 0))
    metal1_vdd.insert_into(ly, tc.cell_index, ly.layer(2, 0))
    metal1_all_wp.insert_into(ly, tc.cell_index, ly.layer(3, 0))

    si = tc.begin_shapes_rec(ly.layer(1, 0))
    assert_equal(si.each.count, 111)

    si = tc.begin_shapes_rec(ly.layer(1, 0))
    si.each do |i|
      assert_equal(i.shape.prop_id, 0)
    end

    # VDD net is smaller
    si = tc.begin_shapes_rec(ly.layer(2, 0))
    assert_equal(si.each.count, 20)
    assert_equal(tc.dbbox(ly.layer(2, 0)).to_s, "(-0.18,2.42;23.94,3.18)")

    si = tc.begin_shapes_rec(ly.layer(3, 0))
    assert_equal(si.each.count, 111)

    # properties are net names + ID
    si = tc.begin_shapes_rec(ly.layer(3, 0))
    net_names = []
    si.each do |i|
      ly.properties(i.shape.prop_id).each do |k,v|
        if k == 1
          net_names << v[0]
        end
      end
    end

    assert_equal(net_names.sort.uniq.join(";"), "$10;$11;$12;$13;$14;$15;$16;$17;$18;$19;$20;$21;$22;$5;$6;$7;$8;$9;FB;OSC;VDD;VSS")

  end

  def test_20_Antenna

    # --- simple antenna check

    input = File.join($ut_testsrc, "testdata", "algo", "antenna_l1.gds")
    ly = RBA::Layout::new
    ly.read(input)

    au = File.join($ut_testsrc, "testdata", "algo", "antenna_au1.gds")
    ly_au = RBA::Layout::new
    ly_au.read(au)

    dss = RBA::DeepShapeStore::new
    assert_equal(dss.is_singular?, false)

    rdiode = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(1, 0)), dss)
    rpoly = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(6, 0)), dss)
    rcont = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(8, 0)), dss)
    rmetal1 = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(9, 0)), dss)
    rvia1 = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(11, 0)), dss)
    rmetal2 = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(12, 0)), dss)
    assert_equal(dss.is_singular?, true)

    l2n = RBA::LayoutToNetlist::new(dss)

    l2n.register(rdiode, "diode")
    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)

    l2n.extract_netlist

    a1_3 = l2n.antenna_check(rpoly, rmetal1, 3)
    a1_10 = l2n.antenna_check(rpoly, rmetal1, 10)
    a1_30 = l2n.antenna_check(rpoly, rmetal1, 30)

    # Note: flatten.merged performs some normalization
    assert_equal((a1_3.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(100, 0)))).to_s, "")
    assert_equal((a1_10.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(101, 0)))).to_s, "")
    assert_equal((a1_30.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(102, 0)))).to_s, "")

    # --- same with flat

    l2n._destroy

    input = File.join($ut_testsrc, "testdata", "algo", "antenna_l1.gds")
    ly = RBA::Layout::new
    ly.read(input)

    au = File.join($ut_testsrc, "testdata", "algo", "antenna_au1.gds")
    ly_au = RBA::Layout::new
    ly_au.read(au)

    rfdiode = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(1, 0)))
    rfpoly = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(6, 0)))
    rfcont = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(8, 0)))
    rfmetal1 = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(9, 0)))
    rfvia1 = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(11, 0)))
    rfmetal2 = RBA::Region::new(ly.top_cell.begin_shapes_rec(ly.layer(12, 0)))
    assert_equal(rfdiode.is_deep?, false)
    assert_equal(rfpoly.is_deep?, false)
    assert_equal(rfmetal1.is_deep?, false)
    assert_equal(rfvia1.is_deep?, false)
    assert_equal(rfmetal2.is_deep?, false)

    l2n = RBA::LayoutToNetlist::new(ly.top_cell.name, ly.dbu)

    l2n.register(rfdiode, "diode")
    l2n.register(rfpoly, "poly")
    l2n.register(rfcont, "cont")
    l2n.register(rfmetal1, "metal1")
    l2n.register(rfvia1, "via1")
    l2n.register(rfmetal2, "metal2")

    l2n.connect(rfpoly)
    l2n.connect(rfcont)
    l2n.connect(rfmetal1)
    l2n.connect(rfpoly, rfcont)
    l2n.connect(rfcont, rfmetal1)

    l2n.extract_netlist

    a1_3 = l2n.antenna_check(rfpoly, rfmetal1, 3)
    a1_10 = l2n.antenna_check(rfpoly, rfmetal1, 10)
    a1_30 = l2n.antenna_check(rfpoly, rfmetal1, 30)

    # Note: flatten.merged performs some normalization
    assert_equal((a1_3.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(100, 0)))).to_s, "")
    assert_equal((a1_10.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(101, 0)))).to_s, "")
    assert_equal((a1_30.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(102, 0)))).to_s, "")

    # --- simple antenna check with metal2

    l2n._destroy
    l2n = RBA::LayoutToNetlist::new(dss)

    l2n.register(rdiode, "diode")
    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rmetal2)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)
    l2n.connect(rmetal1, rvia1)
    l2n.connect(rvia1, rmetal2)

    l2n.extract_netlist

    a2_5 = l2n.antenna_check(rpoly, rmetal2, 5)
    a2_10 = l2n.antenna_check(rpoly, rmetal2, 10)
    a2_17 = l2n.antenna_check(rpoly, rmetal2, 17)

    # Note: flatten.merged performs some normalization
    assert_equal((a2_5.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(200, 0)))).to_s, "")
    assert_equal((a2_10.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(201, 0)))).to_s, "")
    assert_equal((a2_17.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(202, 0)))).to_s, "")

    # --- simple incremental antenna check with metal1 + metal2

    l2n._destroy
    l2n = RBA::LayoutToNetlist::new(dss)

    l2n.register(rdiode, "diode")
    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rmetal2)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)

    assert_equal(l2n.is_extracted?, false)
    l2n.extract_netlist
    assert_equal(l2n.is_extracted?, true)

    a1_3 = l2n.antenna_check(rpoly, rmetal1, 3)
    a1_10 = l2n.antenna_check(rpoly, rmetal1, 10)
    a1_30 = l2n.antenna_check(rpoly, rmetal1, 30)

    # Note: flatten.merged performs some normalization
    assert_equal((a1_3.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(100, 0)))).to_s, "")
    assert_equal((a1_10.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(101, 0)))).to_s, "")
    assert_equal((a1_30.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(102, 0)))).to_s, "")

    l2n.connect(rmetal1, rvia1)
    l2n.connect(rvia1, rmetal2)

    assert_equal(l2n.is_extracted?, false)
    l2n.extract_netlist
    assert_equal(l2n.is_extracted?, true)

    a2_5 = l2n.antenna_check(rpoly, rmetal2, 5)
    a2_10 = l2n.antenna_check(rpoly, rmetal2, 10)
    a2_17 = l2n.antenna_check(rpoly, rmetal2, 17)

    # Note: flatten.merged performs some normalization
    assert_equal((a2_5.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(200, 0)))).to_s, "")
    assert_equal((a2_10.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(201, 0)))).to_s, "")
    assert_equal((a2_17.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(202, 0)))).to_s, "")

    # --- antenna check with diodes and antenna effect reduction

    l2n._destroy
    l2n = RBA::LayoutToNetlist::new(dss)

    l2n.register(rdiode, "diode")
    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rdiode)
    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rdiode, rcont)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)

    l2n.extract_netlist

    a3_3 = l2n.antenna_check(rpoly, rmetal1, 3, [ [ rdiode, 8.0 ] ] )
    a3_10 = l2n.antenna_check(rpoly, rmetal1, 10, [ [ rdiode, 8.0 ] ])
    a3_30 = l2n.antenna_check(rpoly, rmetal1, 30, [ [ rdiode, 8.0 ] ])

    # Note: flatten.merged performs some normalization
    assert_equal((a3_3.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(300, 0)))).to_s, "")
    assert_equal((a3_10.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(301, 0)))).to_s, "")
    assert_equal((a3_30.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(302, 0)))).to_s, "")

    # --- antenna check with diodes

    l2n._destroy
    l2n = RBA::LayoutToNetlist::new(dss)

    l2n.register(rdiode, "diode")
    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rdiode)
    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rdiode, rcont)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)

    l2n.extract_netlist

    a4_3 = l2n.antenna_check(rpoly, rmetal1, 3, [ rdiode ])
    a4_10 = l2n.antenna_check(rpoly, rmetal1, 10, [ rdiode ])
    a4_30 = l2n.antenna_check(rpoly, rmetal1, 30, [ rdiode ])

    # Note: flatten.merged performs some normalization
    assert_equal((a4_3.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(400, 0)))).to_s, "")
    assert_equal((a4_10.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(401, 0)))).to_s, "")
    assert_equal((a4_30.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(402, 0)))).to_s, "")

    # --- antenna check metal perimeter included

    l2n._destroy
    l2n = RBA::LayoutToNetlist::new(dss)

    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rvia1)
    l2n.connect(rmetal2)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)
    l2n.connect(rmetal1, rvia1)
    l2n.connect(rvia1, rmetal2)

    l2n.extract_netlist

    a5_5 = l2n.antenna_check(rpoly, 0.0, rmetal2, 1.0, 5)
    a5_15 = l2n.antenna_check(rpoly, 0.0, rmetal2, 1.0, 15)
    a5_29 = l2n.antenna_check(rpoly, 0.0, rmetal2, 1.0, 29)

    # Note: flatten.merged performs some normalization
    assert_equal((a5_5.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(500, 0)))).to_s, "")
    assert_equal((a5_15.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(501, 0)))).to_s, "")
    assert_equal((a5_29.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(502, 0)))).to_s, "")

    b5_5 = l2n.antenna_check(rpoly, 2.0, 0.0, rmetal2, 1.0, 1.0, 2.5)
    b5_15 = l2n.antenna_check(rpoly, 2.0, 0.0, rmetal2, 1.0, 1.0, 7.5)
    b5_29 = l2n.antenna_check(rpoly, 2.0, 0.0, rmetal2, 1.0, 1.0, 14.5)

    # Note: flatten.merged performs some normalization
    assert_equal((b5_5.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(500, 0)))).to_s, "")
    assert_equal((b5_15.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(501, 0)))).to_s, "")
    assert_equal((b5_29.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(502, 0)))).to_s, "")

    # --- antenna check gate perimeter included

    l2n._destroy
    l2n = RBA::LayoutToNetlist::new(dss)

    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rvia1)
    l2n.connect(rmetal2)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)
    l2n.connect(rmetal1, rvia1)
    l2n.connect(rvia1, rmetal2)

    l2n.extract_netlist

    a6_3 = l2n.antenna_check(rpoly, 0.3, rmetal2, 0.0, 3)
    a6_5 = l2n.antenna_check(rpoly, 0.3, rmetal2, 0.0, 5)
    a6_9 = l2n.antenna_check(rpoly, 0.3, rmetal2, 0.0, 9)

    # Note: flatten.merged performs some normalization
    assert_equal((a6_3.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(600, 0)))).to_s, "")
    assert_equal((a6_5.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(601, 0)))).to_s, "")
    assert_equal((a6_9.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(602, 0)))).to_s, "")

    b6_3 = l2n.antenna_check(rpoly, 1.0, 0.3, rmetal2, 2.0, 0.0, 6)
    b6_5 = l2n.antenna_check(rpoly, 1.0, 0.3, rmetal2, 2.0, 0.0, 10)
    b6_9 = l2n.antenna_check(rpoly, 1.0, 0.3, rmetal2, 2.0, 0.0, 18)

    # Note: flatten.merged performs some normalization
    assert_equal((b6_3.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(600, 0)))).to_s, "")
    assert_equal((b6_5.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(601, 0)))).to_s, "")
    assert_equal((b6_9.flatten ^ RBA::Region::new(ly_au.top_cell.begin_shapes_rec(ly_au.layer(602, 0)))).to_s, "")

  end

  def test_21_LogAPI

    l2n = RBA::LayoutToNetlist::new
    l2n.read(File.join($ut_testsrc, "testdata", "algo", "l2n_reader_au_6.l2n"))

    le = l2n.each_log_entry.collect { |s| s.to_s }
    assert_equal(le.size, 4)
    assert_equal(le[0].to_s, "info")
    assert_equal(le[1].to_s, "[cat description] In cell cell_name: info, shape: (1,1;2,2;3,1)")
    assert_equal(le[2].to_s, "warning")
    assert_equal(le[3].to_s, "error")

  end

  def test_22_Layers

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "device_extract_l1.gds"))

    l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))
    assert_equal(l2n.original_layout.object_id, ly.object_id) 
    assert_equal(l2n.original_top_cell.cell_index, ly.top_cell.cell_index) 

    # only plain connectivity

    ractive     = l2n.make_layer(         ly.layer(2, 0), "active" )
    rpoly       = l2n.make_polygon_layer( ly.layer(3, 0), "poly" )
    rlabels     = l2n.make_text_layer(    ly.layer(6, 1), "labels" )
    rptemp      = l2n.make_polygon_layer( "poly_temp" )
    rltemp      = l2n.make_text_layer   ( "labels_temp" )
    
    rsd         = ractive - rpoly
    l2n.register(rsd, "sd")

    li_ptemp = l2n.layer_index(rptemp)
    assert_equal(l2n.layer_name(li_ptemp), "poly_temp")
    assert_equal(l2n.polygons_by_index(li_ptemp).to_s, "")

    li_ltemp = l2n.layer_index(rltemp)
    assert_equal(l2n.layer_name(li_ltemp), "labels_temp")
    assert_equal(l2n.texts_by_index(li_ltemp).to_s, "")

    assert_equal(l2n.layer_name(ractive), "active")
    assert_equal(l2n.layer_name(rpoly),   "poly")
    assert_equal(l2n.layer_name(rsd),     "sd")
    assert_equal(l2n.layer_name(rlabels), "labels")

    li_active = l2n.layer_index("active")
    li_labels = l2n.layer_index("labels")
    assert_equal(l2n.layer_index(ractive), li_active)

    ractive2 = l2n.layer_by_index(li_active)
    assert_equal(ractive.to_s, ractive2.to_s)

    ractive2 = l2n.layer_by_name("active")
    assert_equal(ractive.to_s, ractive2.to_s)

    ractive2 = l2n.polygons_by_index(li_active)
    assert_equal(ractive.to_s, ractive2.to_s)

    ractive2 = l2n.polygons_by_name("active")
    assert_equal(ractive.to_s, ractive2.to_s)

    rlabels2 = l2n.texts_by_index(li_labels)
    assert_equal(rlabels.to_s, rlabels2.to_s)

    rlabels2 = l2n.texts_by_name("labels")
    assert_equal(rlabels.to_s, rlabels2.to_s)

  end

  def test_23_ShapesOfNet

    ly = RBA::Layout::new
    top = ly.create_cell("TOP")

    l1 = ly.layer(1, 0)
    l2 = ly.layer(2, 0)
    l3 = ly.layer(3, 0)

    top.shapes(l1).insert(RBA::DBox::new(0, 0, 1000, 10))
    top.shapes(l2).insert(RBA::DBox::new(0, 0, 10, 10))
    top.shapes(l2).insert(RBA::DBox::new(990, 0, 1000, 10))
    top.shapes(l3).insert(RBA::DText::new("A", RBA::DTrans::new(5, 5)))
    top.shapes(l3).insert(RBA::DText::new("B", RBA::DTrans::new(995, 5)))

    l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, top, []))
    l1r = l2n.make_polygon_layer(l1, "L1")
    l2r = l2n.make_polygon_layer(l2, "L1.pin")
    l3r = l2n.make_text_layer(l3, "L1.label")

    l2n.connect(l1r)
    l2n.connect(l1r, l2r)
    l2n.connect(l2r)
    l2n.connect(l2r, l3r)

    l2n.extract_netlist

    nl = l2n.netlist
    net = nl.top_circuit.net_by_name("A,B")

    assert_equal(l2n.shapes_of_net(net, l1r).to_s, "(0,0;0,10000;1000000,10000;1000000,0)")
    assert_equal(l2n.shapes_of_net(net, l2r).to_s, "(0,0;0,10000;10000,10000;10000,0);(990000,0;990000,10000;1000000,10000;1000000,0)")
    assert_equal(l2n.shapes_of_net(net, l3r).to_s, "('A',m45 0,0);('B',m135 0,0)")

    assert_equal(l2n.polygons_of_net(net, l2n.layer_index(l1r)).to_s, "(0,0;0,10000;1000000,10000;1000000,0)")
    assert_equal(l2n.polygons_of_net(net, l2n.layer_index(l2r)).to_s, "(0,0;0,10000;10000,10000;10000,0);(990000,0;990000,10000;1000000,10000;1000000,0)")
    assert_equal(l2n.polygons_of_net(net, l2n.layer_index(l3r)).to_s, "")

    assert_equal(l2n.texts_of_net(net, l2n.layer_index(l1r)).to_s, "")
    assert_equal(l2n.texts_of_net(net, l2n.layer_index(l2r)).to_s, "")
    assert_equal(l2n.texts_of_net(net, l2n.layer_index(l3r)).to_s, "('A',m45 0,0);('B',m135 0,0)")

    shapes = RBA::Shapes::new
    l2n.shapes_of_net(net, l1r, true, shapes)
    assert_equal(shapes.each.collect(&:to_s).join("/"), "box (0,0;1000000,10000)")

    shapes = RBA::Shapes::new
    l2n.shapes_of_net(net, l2r, true, shapes)
    assert_equal(shapes.each.collect(&:to_s).join("/"), "box (0,0;10000,10000)/box (990000,0;1000000,10000)")

    shapes = RBA::Shapes::new
    l2n.shapes_of_net(net, l2n.layer_index(l2r), true, shapes)
    assert_equal(shapes.each.collect(&:to_s).join("/"), "box (0,0;10000,10000)/box (990000,0;1000000,10000)")

    shapes = RBA::Shapes::new
    l2n.shapes_of_net(net, l3r, true, shapes)
    assert_equal(shapes.each.collect(&:to_s).join("/"), "text ('A',m45 0,0)/text ('B',m135 0,0)")

    shapes = RBA::Shapes::new
    # wrong container type, correct content (layer_by_index gives a Region)
    l2n.shapes_of_net(net, l2n.layer_by_index(l2n.layer_index(l3r)), true, shapes)
    assert_equal(shapes.each.collect(&:to_s).join("/"), "text ('A',m45 0,0)/text ('B',m135 0,0)")

    shapes = RBA::Shapes::new
    l2n.shapes_of_net(net, l2n.layer_index(l3r), true, shapes)
    assert_equal(shapes.each.collect(&:to_s).join("/"), "text ('A',m45 0,0)/text ('B',m135 0,0)")

  end

end

load("test_epilogue.rb")



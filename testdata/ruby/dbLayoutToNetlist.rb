# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2018 Matthias Koefferlein
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

  end

  def test_2_ShapesFromNet

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "device_extract_l1.gds"))

    l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))

    # only plain backend connectivity

    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0) )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1) )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0) )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0) )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1) )
    
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

    assert_equal(l2n.netlist.to_s, <<END)
Circuit TRANS ($1=$1,$2=$2):
Circuit INV2 (OUT=OUT,$2=$2,$3=$3,$4=$4):
  XTRANS $1 ($1=$4,$2=OUT)
  XTRANS $2 ($1=$3,$2=OUT)
  XTRANS $3 ($1=$2,$2=$4)
  XTRANS $4 ($1=$2,$2=$3)
Circuit RINGO ():
  XINV2 $1 (OUT=OSC,$2=FB,$3=VSS,$4=VDD)
  XINV2 $2 (OUT=$I29,$2=$I20,$3=VSS,$4=VDD)
  XINV2 $3 (OUT=$I28,$2=$I19,$3=VSS,$4=VDD)
  XINV2 $4 (OUT=$I30,$2=$I21,$3=VSS,$4=VDD)
  XINV2 $5 (OUT=$I31,$2=$I22,$3=VSS,$4=VDD)
  XINV2 $6 (OUT=$I32,$2=$I23,$3=VSS,$4=VDD)
  XINV2 $7 (OUT=$I33,$2=$I24,$3=VSS,$4=VDD)
  XINV2 $8 (OUT=$I34,$2=$I25,$3=VSS,$4=VDD)
  XINV2 $9 (OUT=$I35,$2=$I26,$3=VSS,$4=VDD)
  XINV2 $10 (OUT=$I36,$2=$I27,$3=VSS,$4=VDD)
END

    assert_equal(l2n.probe_net(rmetal2, RBA::DPoint::new(0.0, 1.8)).inspect, "RINGO:FB")
    assert_equal(l2n.probe_net(rmetal2, RBA::DPoint::new(-2.0, 1.8)).inspect, "nil")

    n = l2n.probe_net(rmetal1, RBA::Point::new(2600, 1000))
    assert_equal(n.inspect, "RINGO:$I20")

    assert_equal(l2n.shapes_of_net(n, rmetal1, true).to_s, "(1660,-420;1660,2420;2020,2420;2020,-420);(1840,820;1840,1180;3220,1180;3220,820);(1660,2420;1660,3180;2020,3180;2020,2420);(1660,-380;1660,380;2020,380;2020,-380)")

    shapes = RBA::Shapes::new
    l2n.shapes_of_net(n, rmetal1, true, shapes)
    r = RBA::Region::new
    shapes.each { |s| r.insert(s.polygon) }
    assert_equal(r.to_s, "(1660,-420;1660,2420;2020,2420;2020,-420);(1840,820;1840,1180;3220,1180;3220,820);(1660,2420;1660,3180;2020,3180;2020,2420);(1660,-380;1660,380;2020,380;2020,-380)")

  end
  
  def test_10_LayoutToNetlistExtractionWithoutDevices

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "device_extract_l1.gds"))

    l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))

    # only plain connectivity

    ractive     = l2n.make_layer(         ly.layer(2, 0) )
    rpoly       = l2n.make_polygon_layer( ly.layer(3, 0) )
    rpoly_lbl   = l2n.make_text_layer(    ly.layer(3, 1) )
    rdiff_cont  = l2n.make_polygon_layer( ly.layer(4, 0) )
    rpoly_cont  = l2n.make_polygon_layer( ly.layer(5, 0) )
    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0) )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1) )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0) )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0) )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1) )
    
    rsd         = ractive - rpoly

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
Circuit TRANS ($1=$1,$2=$2,$3=$3):
Circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5):
  XTRANS $1 ($1=$2,$2=$4,$3=IN)
  XTRANS $2 ($1=$2,$2=$5,$3=IN)
  XTRANS $3 ($1=$5,$2=OUT,$3=$2)
  XTRANS $4 ($1=$4,$2=OUT,$3=$2)
Circuit RINGO ():
  XINV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD)
  XINV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD)
  XINV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD)
  XINV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD)
  XINV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD)
  XINV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD)
  XINV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD)
  XINV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD)
  XINV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD)
  XINV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD)
END

  end

  def test_11_LayoutToNetlistExtractionWithDevices

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "device_extract_l1.gds"))

    l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))

    rnwell      = l2n.make_layer(         ly.layer(1, 0) )
    ractive     = l2n.make_layer(         ly.layer(2, 0) )
    rpoly       = l2n.make_polygon_layer( ly.layer(3, 0) )
    rpoly_lbl   = l2n.make_text_layer(    ly.layer(3, 1) )
    rdiff_cont  = l2n.make_polygon_layer( ly.layer(4, 0) )
    rpoly_cont  = l2n.make_polygon_layer( ly.layer(5, 0) )
    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0) )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1) )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0) )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0) )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1) )

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
Circuit RINGO ():
  XINV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD)
  XINV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD)
  XINV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD)
  XINV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD)
  XINV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD)
  XINV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD)
  XINV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD)
  XINV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD)
  XINV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD)
  XINV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD)
Circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5):
  DPMOS $1 (S=$2,G=IN,D=$5) [L=0.25,W=0.95,AS=0.49875,AD=0.26125]
  DPMOS $2 (S=$5,G=$2,D=OUT) [L=0.25,W=0.95,AS=0.26125,AD=0.49875]
  DNMOS $3 (S=$2,G=IN,D=$4) [L=0.25,W=0.95,AS=0.49875,AD=0.26125]
  DNMOS $4 (S=$4,G=$2,D=OUT) [L=0.25,W=0.95,AS=0.26125,AD=0.49875]
  XTRANS $1 ($1=$2,$2=$4,$3=IN)
  XTRANS $2 ($1=$2,$2=$5,$3=IN)
  XTRANS $3 ($1=$5,$2=OUT,$3=$2)
  XTRANS $4 ($1=$4,$2=OUT,$3=$2)
Circuit TRANS ($1=$1,$2=$2,$3=$3):
END

    # cleanup now
    l2n._destroy

  end

end

load("test_epilogue.rb")



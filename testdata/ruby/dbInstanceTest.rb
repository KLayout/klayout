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

class DBInstance_TestClass < TestBase

  # Instance edit functions
  def test_1_InstEdit

    # this only works in editable mode
    ly = RBA::Layout::new(true)

    pid4 = ly.properties_id( { "id" => 4 } )
    pid5 = ly.properties_id( { "id" => 5 } )
    pid7 = ly.properties_id( { "id" => 7 } )
    pid14 = ly.properties_id( { "id" => 14 } )
    pid15 = ly.properties_id( { "id" => 15 } )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    assert_equal(c1.bbox.to_s, "(10,-10;50,40)")
    assert_equal(c1.bbox_per_layer(lindex).to_s, "(10,-10;50,40)")
    assert_equal(c1.bbox_per_layer(ldummy).to_s, "()")
    assert_equal(c1.bbox(lindex).to_s, "(10,-10;50,40)")
    assert_equal(c1.bbox(ldummy).to_s, "()")

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 101, -51 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    assert_equal(inst.bbox(ly).to_s, "(61,-41;111,-1)")
    assert_equal(inst.bbox(ly, lindex).to_s, "(61,-41;111,-1)")
    assert_equal(inst.bbox(ly, ldummy).to_s, "()")

    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )
    assert_equal( inst_ref.cell_inst.bbox(ly).to_s, "(61,-41;111,-1)" )
    assert_equal( inst_ref.bbox(lindex).to_s, "(61,-41;111,-1)" )
    assert_equal( inst_ref.cell_inst.bbox(ly, lindex).to_s, "(61,-41;111,-1)" )
    assert_equal( inst_ref.bbox(ldummy).to_s, "()" )
    assert_equal( inst_ref.cell_inst.bbox(ly, ldummy).to_s, "()" )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    inst_ref = c2.replace_prop_id( inst_ref, pid5 )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>5}" )

    inst_ref = c2.replace( inst_ref, RBA::CellInstArray::new( c3.cell_index, tr ) )
    assert_equal( inst_ref.to_s, "cell_index=2 r90 100,-50 props={id=>5}" )

    inst_ref = c2.replace( inst_ref, RBA::CellInstArray::new( c1.cell_index, tr ), pid7 )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>7}" )

    inst_ref = c2.transform( inst_ref, RBA::Trans::new( 3, false, 100, 200 ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r0 50,100 props={id=>7}" )

    inst_ref = c2.transform( inst_ref, RBA::ICplxTrans::new( 0.5, 45.0, false, RBA::Point::new ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r45 *0.5 -18,53 props={id=>7}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r45 *0.5 -18,53 props={id=>7}" ] )

    arr = []
    c2.each_touching_inst(RBA::Box::new(-30, 50, -28, 80)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r45 *0.5 -18,53 props={id=>7}" ] )

    arr = []
    c2.each_touching_inst(RBA::Box::new(-30, 50, -29, 80)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r45 *0.5 -18,53 props={id=>7}" ] )

    arr = []
    c2.each_touching_inst(RBA::Box::new(-31, 50, -30, 80)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ ] )

    arr = []
    c2.each_overlapping_inst(RBA::Box::new(-30, 50, -28, 80)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r45 *0.5 -18,53 props={id=>7}" ] )

    arr = []
    c2.each_overlapping_inst(RBA::Box::new(-30, 50, -29, 80)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ ] )

    arr = []
    c2.each_overlapping_inst(RBA::Box::new(-31, 50, -30, 80)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ ] )

    inst_ref2 = c2.insert( RBA::CellInstArray::new( c3.cell_index, tr ), pid14 )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r45 *0.5 -18,53 props={id=>7}",
                         "cell_index=2 r90 100,-50 props={id=>14}" ] )

    assert_equal( inst_ref.is_valid?, true )
    assert_equal( c2.is_valid?(inst_ref), true )
    c2.erase( inst_ref )
    assert_equal( inst_ref.is_valid?, false )
    assert_equal( c2.is_valid?(inst_ref), false )
    inst_ref3 = c2.insert( RBA::CellInstArray::new( c3.cell_index, tr ) )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50",
                         "cell_index=2 r90 100,-50 props={id=>14}" ] )

    c2.erase( inst_ref2 )
    c2.erase( inst_ref3 )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ ] )

    inst_ref2 = c2.insert( RBA::CellInstArray::new( c3.cell_index, tr ), pid14 )
    inst_ref = c2.insert( RBA::CellInstArray::new( c3.cell_index, tr ), pid15 )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}",  "cell_index=2 r90 100,-50 props={id=>15}" ] )

    inst_ref.delete

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}" ] )

    inst_ref = c2.insert( RBA::CellInstArray::new( c3.cell_index, tr ), pid15 )
    assert_equal( inst_ref.is_regular_array?, false )
    inst_ref.na = 2
    assert_equal( inst_ref.is_regular_array?, true )
    inst_ref.nb = 21
    inst_ref.a = RBA::Point::new(1, 2)
    inst_ref.b = RBA::Point::new(10, 20)
    assert_equal( inst_ref.na, 2 )
    assert_equal( inst_ref.nb, 21 )
    assert_equal( inst_ref.a.to_s, "1,2" )
    assert_equal( inst_ref.b.to_s, "10,20" )
    assert_equal( inst_ref.is_regular_array?, true )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}", "cell_index=2 r90 100,-50 array=(1,2,10,20 2x21) props={id=>15}" ] )

    inst_ref.cplx_trans = RBA::ICplxTrans::new(2.5)

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}", "cell_index=2 r0 *2.5 0,0 array=(1,2,10,20 2x21) props={id=>15}" ] )

    inst_ref.transform( RBA::ICplxTrans::new(2.0) )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}", "cell_index=2 r0 *5 0,0 array=(2,4,20,40 2x21) props={id=>15}" ] )

    inst_ref.transform( RBA::ICplxTrans::new(0.5) )
    assert_equal( inst_ref.cplx_trans.to_s, "r0 *2.5 0,0" )

    inst_ref.trans = RBA::Trans::new(3, true)

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}", "cell_index=2 m135 0,0 array=(1,2,10,20 2x21) props={id=>15}" ] )

    inst_ref.transform( RBA::Trans::new(3) )
    assert_equal( inst_ref.cplx_trans.to_s, "m90 *1 0,0" )

    inst_ref.nb = 3
    inst_ref2.delete

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 m90 0,0 array=(2,-1,20,-10 2x3) props={id=>15}" ] )

    inst_ref.explode
    assert_equal( inst_ref.to_s, "cell_index=2 m90 0,0 props={id=>15}" );

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 m90 2,-1 props={id=>15}", "cell_index=2 m90 0,0 props={id=>15}", "cell_index=2 m90 20,-10 props={id=>15}", "cell_index=2 m90 22,-11 props={id=>15}", "cell_index=2 m90 40,-20 props={id=>15}", "cell_index=2 m90 42,-21 props={id=>15}" ] )

  end

  # Instance edit functions
  def test_2_InstEdit

    # this only works in editable mode
    ly = RBA::Layout::new(true)

    pid4 = ly.properties_id( { "id" => 4 } )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 101, -51 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )

    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    inst_ref = c2.transform_into( inst_ref, RBA::Trans::new( 3, false, 100, 200 ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 250,0 props={id=>4}" )

    inst_ref = c2.transform_into( inst_ref, RBA::ICplxTrans::new( 0.5, 45.0, false, RBA::Point::new ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 88,88 props={id=>4}" )

    inst_ref = c2.transform_into( inst_ref, RBA::ICplxTrans::new( 0.5, 45.0, false, RBA::Point::new ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 0,62 props={id=>4}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r90 0,62 props={id=>4}" ] )

    c2.clear 
    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 101, -51 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    inst_ref.transform_into( RBA::Trans::new( 3, false, 100, 200 ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 250,0 props={id=>4}" )

    inst_ref.transform_into( RBA::ICplxTrans::new( 0.5, 45.0, false, RBA::Point::new ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 88,88 props={id=>4}" )

    inst_ref.transform_into( RBA::ICplxTrans::new( 0.5, 45.0, false, RBA::Point::new ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 0,62 props={id=>4}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r90 0,62 props={id=>4}" ] )

    c2.clear 
    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 101, -51 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    c2.transform_into( RBA::Trans::new( 3, false, 100, 200 ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 250,0 props={id=>4}" )

    c2.transform_into( RBA::ICplxTrans::new( 0.5, 45.0, false, RBA::Point::new ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 88,88 props={id=>4}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r90 88,88 props={id=>4}" ] )

    c2.clear 
    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 101, -51 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    c2.transform_into( RBA::Trans::new( 3, false, 100, 200 ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 250,0 props={id=>4}" )

    c2.transform_into( RBA::ICplxTrans::new( 0.5, 45.0, false, RBA::Point::new ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 88,88 props={id=>4}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r90 88,88 props={id=>4}" ] )

    c2.clear 
    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 101, -51 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    c2.layout.transform( RBA::Trans::new( 3, false, 100, 200 ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 250,0 props={id=>4}" )

    c2.layout.transform( RBA::ICplxTrans::new( 0.5, 45.0, false, RBA::Point::new ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 88,88 props={id=>4}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r90 88,88 props={id=>4}" ] )

  end

  # Instance functions
  def test_3_InstCreate

    ly = RBA::Layout::new

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 101, -51 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )

    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    assert_equal( inst_ref.is_valid?, true )

  end

  # Instance flattening
  def test_4_InstFlatten

    # this only works in editable mode
    ly = RBA::Layout::new(true)

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 101, -51 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )

    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.is_valid?, true )
    inst_ref.flatten
    assert_equal( inst_ref.is_valid?, false )

    shapes = []
    c2.shapes( lindex ).each { |s| shapes << s.to_s }
    insts = []
    c2.each_inst { |i| insts << i.to_s }

    assert_equal( shapes.join( ":" ), "box (61,-41;111,-1)" )
    assert_equal( insts.join( ":" ), "" )

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    c2.shapes( lindex ).clear

    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ref = c2.insert( inst )

    inst = RBA::CellInstArray::new( c2.cell_index, tr )
    inst_ref = c3.insert( inst )

    inst_ref.flatten( 1 )

    shapes = []
    c3.shapes( lindex ).each { |s| shapes << s.to_s }
    insts = []
    c3.each_inst { |i| insts << i.to_s }

    assert_equal( shapes.join( ":" ), "" )
    assert_equal( insts.join( ":" ), "cell_index=0 r180 152,50" )

  end

  # Instance edit functions
  def test_5_InstEdit_Double

    # this only works in editable mode
    ly = RBA::Layout::new(true)

    pid4 = ly.properties_id( { "id" => 4 } )
    pid5 = ly.properties_id( { "id" => 5 } )
    pid7 = ly.properties_id( { "id" => 7 } )
    pid14 = ly.properties_id( { "id" => 14 } )
    pid15 = ly.properties_id( { "id" => 15 } )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    assert_equal(c1.dbbox.to_s, "(0.01,-0.01;0.05,0.04)")
    assert_equal(c1.dbbox_per_layer(lindex).to_s, "(0.01,-0.01;0.05,0.04)")
    assert_equal(c1.dbbox_per_layer(ldummy).to_s, "()")
    assert_equal(c1.dbbox(lindex).to_s, "(0.01,-0.01;0.05,0.04)")
    assert_equal(c1.dbbox(ldummy).to_s, "()")

    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.101, -0.051 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    assert_equal(inst.bbox(ly).to_s, "(0.061,-0.041;0.111,-0.001)")
    assert_equal(inst.bbox(ly, lindex).to_s, "(0.061,-0.041;0.111,-0.001)")
    assert_equal(inst.bbox(ly, ldummy).to_s, "()")

    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.dbbox.to_s, "(0.061,-0.041;0.111,-0.001)" )
    assert_equal( inst_ref.dcell_inst.bbox(ly).to_s, "(0.061,-0.041;0.111,-0.001)" )
    assert_equal( inst_ref.dbbox(lindex).to_s, "(0.061,-0.041;0.111,-0.001)" )
    assert_equal( inst_ref.dcell_inst.bbox(ly, lindex).to_s, "(0.061,-0.041;0.111,-0.001)" )
    assert_equal( inst_ref.dbbox(ldummy).to_s, "()" )
    assert_equal( inst_ref.dcell_inst.bbox(ly, ldummy).to_s, "()" )

    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.100, -0.050 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    inst_ref.dcell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    inst_ref = c2.replace_prop_id( inst_ref, pid5 )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>5}" )

    inst_ref = c2.replace( inst_ref, RBA::DCellInstArray::new( c3.cell_index, tr ) )
    assert_equal( inst_ref.to_s, "cell_index=2 r90 100,-50 props={id=>5}" )

    inst_ref = c2.replace( inst_ref, RBA::DCellInstArray::new( c1.cell_index, tr ), pid7 )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>7}" )

    inst_ref = c2.transform( inst_ref, RBA::DTrans::new( 3, false, 0.1, 0.2 ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r0 50,100 props={id=>7}" )

    inst_ref = c2.transform( inst_ref, RBA::DCplxTrans::new( 0.5, 45.0, false, RBA::DPoint::new ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r45 *0.5 -18,53 props={id=>7}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r45 *0.5 -18,53 props={id=>7}" ] )

    arr = []
    c2.each_touching_inst(RBA::DBox::new(-0.030, 0.050, -0.028, 0.080)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r45 *0.5 -18,53 props={id=>7}" ] )

    arr = []
    c2.each_touching_inst(RBA::DBox::new(-0.030, 0.050, -0.029, 0.080)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r45 *0.5 -18,53 props={id=>7}" ] )

    arr = []
    c2.each_touching_inst(RBA::DBox::new(-0.031, 0.050, -0.030, 0.080)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ ] )

    arr = []
    c2.each_overlapping_inst(RBA::DBox::new(-0.030, 0.050, -0.028, 0.080)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r45 *0.5 -18,53 props={id=>7}" ] )

    arr = []
    c2.each_overlapping_inst(RBA::DBox::new(-0.030, 0.050, -0.029, 0.080)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ ] )

    arr = []
    c2.each_overlapping_inst(RBA::DBox::new(-0.031, 0.050, -0.030, 0.080)) { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ ] )

    inst_ref2 = c2.insert( RBA::DCellInstArray::new( c3.cell_index, tr ), pid14 )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r45 *0.5 -18,53 props={id=>7}",
                         "cell_index=2 r90 100,-50 props={id=>14}" ] )

    assert_equal( inst_ref.is_valid?, true )
    assert_equal( c2.is_valid?(inst_ref), true )
    c2.erase( inst_ref )
    assert_equal( inst_ref.is_valid?, false )
    assert_equal( c2.is_valid?(inst_ref), false )
    inst_ref3 = c2.insert( RBA::DCellInstArray::new( c3.cell_index, tr ) )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50",
                         "cell_index=2 r90 100,-50 props={id=>14}" ] )

    c2.erase( inst_ref2 )
    c2.erase( inst_ref3 )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ ] )

    inst_ref2 = c2.insert( RBA::DCellInstArray::new( c3.cell_index, tr ), pid14 )
    inst_ref = c2.insert( RBA::DCellInstArray::new( c3.cell_index, tr ), pid15 )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}",  "cell_index=2 r90 100,-50 props={id=>15}" ] )

    inst_ref.delete

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}" ] )

    inst_ref = c2.insert( RBA::DCellInstArray::new( c3.cell_index, tr ), pid15 )
    assert_equal( inst_ref.is_regular_array?, false )
    inst_ref.na = 2
    assert_equal( inst_ref.is_regular_array?, true )
    inst_ref.nb = 21
    inst_ref.da = RBA::DPoint::new(0.001, 0.002)
    inst_ref.db = RBA::DPoint::new(0.010, 0.020)
    assert_equal( inst_ref.na, 2 )
    assert_equal( inst_ref.nb, 21 )
    assert_equal( inst_ref.a.to_s, "1,2" )
    assert_equal( inst_ref.b.to_s, "10,20" )
    assert_equal( inst_ref.is_regular_array?, true )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}", "cell_index=2 r90 100,-50 array=(1,2,10,20 2x21) props={id=>15}" ] )

    inst_ref.dcplx_trans = RBA::DCplxTrans::new(2.5)

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}", "cell_index=2 r0 *2.5 0,0 array=(1,2,10,20 2x21) props={id=>15}" ] )

    inst_ref.transform( RBA::DCplxTrans::new(2.0) )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}", "cell_index=2 r0 *5 0,0 array=(2,4,20,40 2x21) props={id=>15}" ] )

    inst_ref.transform( RBA::DCplxTrans::new(0.5) )
    assert_equal( inst_ref.cplx_trans.to_s, "r0 *2.5 0,0" )

    inst_ref.trans = RBA::DTrans::new(3, true)

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 r90 100,-50 props={id=>14}", "cell_index=2 m135 0,0 array=(1,2,10,20 2x21) props={id=>15}" ] )

    inst_ref.transform( RBA::DTrans::new(3) )
    assert_equal( inst_ref.cplx_trans.to_s, "m90 *1 0,0" )

    inst_ref.nb = 3
    inst_ref2.delete

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 m90 0,0 array=(2,-1,20,-10 2x3) props={id=>15}" ] )

    inst_ref.explode
    assert_equal( inst_ref.to_s, "cell_index=2 m90 0,0 props={id=>15}" );

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=2 m90 2,-1 props={id=>15}", "cell_index=2 m90 0,0 props={id=>15}", "cell_index=2 m90 20,-10 props={id=>15}", "cell_index=2 m90 22,-11 props={id=>15}", "cell_index=2 m90 40,-20 props={id=>15}", "cell_index=2 m90 42,-21 props={id=>15}" ] )

  end

  # Instance edit functions
  def test_6_InstEdit_Double

    # this only works in editable mode
    ly = RBA::Layout::new(true)

    pid4 = ly.properties_id( { "id" => 4 } )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )

    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.101, -0.051 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )

    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.100, -0.050 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    inst_ref = c2.transform_into( inst_ref, RBA::DTrans::new( 3, false, 0.100, 0.200 ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 250,0 props={id=>4}" )

    inst_ref = c2.transform_into( inst_ref, RBA::DCplxTrans::new( 0.5, 45.0, false, RBA::DPoint::new(0.001, 0.002) ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 91,89 props={id=>4}" )

    inst_ref = c2.transform_into( inst_ref, RBA::DCplxTrans::new( 0.5, 45.0, false, RBA::DPoint::new ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 1,64 props={id=>4}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r90 1,64 props={id=>4}" ] )

    c2.clear 
    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.101, -0.051 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.100, -0.050 ) )
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    inst_ref.transform_into( RBA::DTrans::new( 3, false, 0.100, 0.200 ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 250,0 props={id=>4}" )

    inst_ref.transform_into( RBA::DCplxTrans::new( 0.5, 45.0, false, RBA::DPoint::new ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 88,88 props={id=>4}" )

    inst_ref.transform_into( RBA::DCplxTrans::new( 0.5, 45.0, false, RBA::DPoint::new ) )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 0,62 props={id=>4}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r90 0,62 props={id=>4}" ] )

    c2.clear 
    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.101, -0.051 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.100, -0.050 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    c2.transform_into( RBA::DTrans::new( 3, false, 0.100, 0.200 ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 250,0 props={id=>4}" )

    c2.transform_into( RBA::DCplxTrans::new( 0.5, 45.0, false, RBA::DPoint::new(0.001, -0.001) ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 88,86 props={id=>4}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r90 88,86 props={id=>4}" ] )

    c2.clear 
    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.101, -0.051 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.100, -0.050 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    c2.transform_into( RBA::DTrans::new( 3, false, 0.100, 0.200 ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 250,0 props={id=>4}" )

    c2.transform_into( RBA::DCplxTrans::new( 0.5, 45.0, false, RBA::DPoint::new ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 88,88 props={id=>4}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r90 88,88 props={id=>4}" ] )

    c2.clear 
    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.101, -0.051 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.bbox.to_s, "(61,-41;111,-1)" )

    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.100, -0.050 ) ) 
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )
    inst_ref.cell_inst = inst
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50" )

    inst_ref.prop_id = pid4
    assert_equal( inst_ref.to_s, "cell_index=0 r90 100,-50 props={id=>4}" )

    c2.layout.transform( RBA::DTrans::new( 3, false, 0.100, 0.200 ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 250,0 props={id=>4}" )

    c2.layout.transform( RBA::DCplxTrans::new( 0.5, 45.0, false, RBA::DPoint::new(0.001, 0.001) ) )
    inst_ref = nil
    c2.each_inst { |i| inst_ref = i }
    assert_equal( inst_ref.to_s, "cell_index=0 r90 90,88 props={id=>4}" )

    arr = []
    c2.each_inst { |i| arr.push( i.to_s ) }
    assert_equal( arr, [ "cell_index=0 r90 90,88 props={id=>4}" ] )

  end

  # Instance functions
  def test_7_InstCreate_Double

    ly = RBA::Layout::new

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )

    tr = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 0.101, -0.051 ) )
    inst = RBA::DCellInstArray::new( c1.cell_index, tr )

    inst_ref = c2.insert( inst )
    assert_equal( inst_ref.to_s, "cell_index=0 r90 101,-51" )
    assert_equal( inst_ref.dbbox.to_s, "(0.061,-0.041;0.111,-0.001)" )
    assert_equal( inst_ref.dbbox.to_s, "(0.061,-0.041;0.111,-0.001)" )

    assert_equal( inst_ref.is_valid?, true )

  end

end

load("test_epilogue.rb")

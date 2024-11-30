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

def mi2s(obj)
  obj.each_meta_info.collect { |mi| mi.name + ":" + mi.value.to_s }.sort.join(";")
end

class DBLayoutTests2_TestClass < TestBase

  # LayerInfo
  def test_1_Layout

    lp = RBA::LayerInfo::new

    lp.name = "hallo"
    lp.layer = 5
    lp.datatype = 7
    assert_equal( lp.name, "hallo" )
    assert_equal( lp.layer, 5 )
    assert_equal( lp.datatype, 7 )
    assert_equal( RBA::LayerInfo::from_string(lp.to_s).to_s, "hallo (5/7)" )

    lp.name = "x"
    assert_equal( lp.name, "x" )
    assert_equal( lp.layer, 5 )
    assert_equal( lp.datatype, 7 )

    lp.layer = 15
    assert_equal( lp.name, "x" )
    assert_equal( lp.layer, 15 )
    assert_equal( lp.datatype, 7 )

    lp.datatype = 3
    assert_equal( lp.name, "x" )
    assert_equal( lp.layer, 15 )
    assert_equal( lp.datatype, 3 )

    assert_equal( RBA::LayerInfo::from_string("").to_s, "" )
    assert_equal( RBA::LayerInfo::from_string("name").to_s, "name" )
    assert_equal( RBA::LayerInfo::from_string("1").to_s, "1/0" )
    assert_equal( RBA::LayerInfo::from_string("name (17)").to_s, "name (17/0)" )
    assert_equal( RBA::LayerInfo::from_string("'1' (8/5)").to_s, "'1' (8/5)" )

  end

  # Basics: cells and instances
  def test_2_Layout

    ly = RBA::Layout::new

    ly.dbu = 0.25
    assert_equal( ly.dbu, 0.25 )

    assert_equal( ly.has_cell?("cell"), false )
    assert_equal( ly.cells, 0 )

    ci = ly.add_cell( "new_cell" )
    assert_equal( ly.cells, 1 )
    assert_equal( ly.has_cell?("cell"), false )
    assert_equal( ly.has_cell?("new_cell"), true )
    assert_equal( ly.cell_name(ci), "new_cell" )

    assert_equal( ly.cell_by_name("new_cell"), ci )
    assert_equal( ly.cell(ci).cell_index, ci )
    assert_equal( ly.cells("A*"), [] )
    assert_equal( ly.cell("new_cell").name, "new_cell" )
    assert_equal( ly.cell("x").inspect, "nil" )
    lyc = ly.dup
    assert_equal( lyc._to_const_object.cell("new_cell").name, "new_cell" ) 
    assert_equal( lyc._to_const_object.cell(ci).cell_index, ci ) 

    ci2 = ly.add_cell( "new_cell_b" )
    assert_equal( ly.cells, 2 )
    assert_equal( ly.cell_by_name("new_cell_b"), ci2 )
    assert_equal( ly.cells("new*").collect { |c| c.name }.sort, ['new_cell', 'new_cell_b'] )
    assert_equal( ci != ci2, true )
    lyc = ly.dup
    assert_equal( lyc._to_const_object.cells("new*").collect { |c| c.name }.sort, ['new_cell', 'new_cell_b'] )

    ly.rename_cell( ci2, "x" )
    assert_equal( ly.cell_by_name("x"), ci2 )
    assert_equal( ly.cell_name(ci), "new_cell" )
    assert_equal( ly.cell_name(ci2), "x" )

    assert_equal( ly.under_construction, false )
    ly.start_changes 
    assert_equal( ly.under_construction, true )
    ly.end_changes 
    assert_equal( ly.under_construction, false )

    li = RBA::LayerInfo::new
    li.layer = 16
    li.datatype = 1
    lindex = ly.insert_layer( li )
    assert_equal( ly.is_valid_layer?( lindex ), true )
    assert_equal( ly.is_special_layer?( lindex ), false )
    assert_equal( ly.is_valid_layer?( 1234 ), false )

    li2 = ly.get_info( lindex ).dup
    assert_equal( li2.layer, li.layer )
    assert_equal( li2.datatype, li.datatype )

    li2.layer = 17
    li2.datatype = 2

    ly.set_info( lindex, li2 )
    li3 = ly.get_info( lindex )
    assert_equal( li3.layer, 17 )
    assert_equal( li3.datatype, 2 )

    ly.delete_layer( lindex )
    assert_equal( ly.is_valid_layer?( lindex ), false )

    arr = []
    ly.each_cell { |c| arr.push( c.cell_index ) }
    assert_equal( arr, [ 0, 1 ] )

    arr = []
    ly.each_top_cell { |c| arr.push( c ) }
    assert_equal( arr, [ 0, 1 ] )

    arr = []
    ly.each_cell_top_down { |c| arr.push( c ) }
    assert_equal( arr, [ 0, 1 ] )

    arr = []
    ly.each_cell_bottom_up { |c| arr.push( c ) }
    assert_equal( arr, [ 1, 0 ] )

    ly.cell(ci2).insert( RBA::CellInstArray::new( ci, RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 1000, -500 ) ) ) )

    arr = []
    ly.each_cell { |c| arr.push( c.cell_index ) }
    assert_equal( arr, [ 0, 1 ] )

    arr = []
    ly.each_top_cell { |c| arr.push( c ) }
    assert_equal( arr, [ 1 ] )

    arr = []
    ly.each_cell_top_down { |c| arr.push( c ) }
    assert_equal( arr, [ 1, 0 ] )

    arr = []
    ly.each_cell_bottom_up { |c| arr.push( c ) }
    assert_equal( arr, [ 0, 1 ] )

    assert_equal( ly.cell( ci2 ).is_top?, true )
    assert_equal( ly.cell( ci ).is_top?, false )
    assert_equal( ly.cell( ci2 ).is_leaf?, false )
    assert_equal( ly.cell( ci ).is_leaf?, true )

    ly.cell(ci2).clear_insts

    arr = []
    ly.each_cell { |c| arr.push( c.cell_index ) }
    assert_equal( arr, [ 0, 1 ] )

    arr = []
    ly.each_top_cell { |c| arr.push( c ) }
    assert_equal( arr, [ 0, 1 ] )

    arr = []
    ly.each_cell_top_down { |c| arr.push( c ) }
    assert_equal( arr, [ 0, 1 ] )

    arr = []
    ly.each_cell_bottom_up { |c| arr.push( c ) }
    assert_equal( arr, [ 1, 0 ] )

  end

  # Instances and bboxes
  def test_5_Layout

    ly = RBA::Layout::new

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

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

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    assert_equal( c1.bbox.to_s, "(10,-10;50,40)" )
    assert_equal( c1.bbox_per_layer( lindex ).to_s, "(10,-10;50,40)" )
    assert_equal( c1.bbox_per_layer( ldummy ).to_s, "()" )
    assert_equal( c1.bbox( lindex ).to_s, "(10,-10;50,40)" )
    assert_equal( c1.bbox( ldummy ).to_s, "()" )
    assert_equal( c1.dbbox_per_layer( lindex ).to_s, "(0.01,-0.01;0.05,0.04)" )
    assert_equal( c1.dbbox_per_layer( ldummy ).to_s, "()" )
    assert_equal( c1.dbbox( lindex ).to_s, "(0.01,-0.01;0.05,0.04)" )
    assert_equal( c1.dbbox( ldummy ).to_s, "()" )

    c1.swap( lindex, ldummy )
    assert_equal( c1.bbox( lindex ).to_s, "()" )
    assert_equal( c1.bbox( ldummy ).to_s, "(10,-10;50,40)" )
    
    c1.clear( lindex )
    c1.clear( ldummy )
    assert_equal( c1.bbox( lindex ).to_s, "()" )
    assert_equal( c1.bbox( ldummy ).to_s, "()" )
    
    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    assert_equal( c1.bbox( lindex ).to_s, "(10,-10;50,40)" )
    assert_equal( c1.bbox( ldummy ).to_s, "()" )
    c1.clear_shapes
    assert_equal( c1.bbox( lindex ).to_s, "()" )
    assert_equal( c1.bbox( ldummy ).to_s, "()" )

    assert_equal( ly.unique_cell_name("c3"), "c3" )
    assert_equal( ly.unique_cell_name("c1"), "c1$1" )

  end

  # Instances and bboxes
  def test_6_Layout

    ly = RBA::Layout::new

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

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

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    assert_equal( c1.bbox.to_s, "(10,-10;50,40)" )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    assert_equal( inst.bbox( ly ).to_s, c1.bbox.transformed(tr).to_s )
    assert_equal( inst.bbox( ly, lindex ).to_s, c1.bbox.transformed(tr).to_s )
    assert_equal( inst.bbox( ly, ldummy ).to_s, "()" )
    assert_equal( inst.size, 1 )
    assert_equal( inst.is_complex?, false )
    c2.insert( inst )
    assert_equal( c2.bbox.to_s, c1.bbox.transformed(tr).to_s )

    c2.clear_insts
    tr = RBA::CplxTrans::new( 1.5, 90.0, true, RBA::DPoint::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst2 = inst.dup
    assert_equal( inst.bbox( ly ).to_s, c1.bbox.transformed(tr).to_s )
    assert_equal( inst.size, 1 )
    assert_equal( inst.is_complex?, true )
    assert_equal( inst.is_regular_array?, false )
    c2.insert( inst )
    assert_equal( c2.bbox.to_s, c1.bbox.transformed(tr).to_s )

    c2.clear_insts
    assert_equal( c2.bbox.to_s, "()" )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) )
    inst = RBA::CellInstArray::new( c1.cell_index, tr, RBA::Point::new( 100, 0 ), RBA::Point::new( 0, 100 ), 10, 20 )
    assert_equal( inst == inst2, false )
    assert_equal( inst != inst2, true )
    inst2 = inst.dup
    assert_equal( inst == inst2, true )
    assert_equal( inst != inst2, false )
    assert_equal( inst.bbox( ly ).to_s, "(60,-40;1010,1900)" )
    assert_equal( inst.trans.to_s, "r90 100,-50" )
    assert_equal( inst.cplx_trans.to_s, "r90 *1 100,-50" )
    assert_equal( inst.size, 200 )
    assert_equal( inst.is_complex?, false )
    assert_equal( inst.is_regular_array?, true )
    assert_equal( inst.a.to_s, "100,0" )
    assert_equal( inst.b.to_s, "0,100" )
    assert_equal( inst.na, 10 )
    assert_equal( inst.nb, 20 )
    assert_equal( inst.cell_index, c1.cell_index )
    c2.insert( inst )
    assert_equal( c2.bbox.to_s, "(60,-40;1010,1900)" )

    inst.invert
    assert_equal( inst == inst2, false )
    assert_equal( inst != inst2, true )
    assert_equal( inst.bbox( ly ).to_s, "(-1860,50;90,990)" )
    assert_equal( inst.size, 200 )
    assert_equal( inst.is_complex?, false )
    assert_equal( inst.is_regular_array?, true )
    assert_equal( inst.a.to_s, "0,100" )
    assert_equal( inst.b.to_s, "-100,0" )
    assert_equal( inst.na, 10 )
    assert_equal( inst.nb, 20 )
    assert_equal( inst.cell_index, c1.cell_index )

    c2.each_inst { |inst| c2.erase( inst ) }
    assert_equal( c2.bbox.to_s, "()" )

    tr = RBA::CplxTrans::new( 1.5, 90.0, true, RBA::DPoint::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr, RBA::Point::new( 100, 0 ), RBA::Point::new( 0, 100 ), 10, 20 )
    assert_equal( inst.bbox( ly ).to_s, "(85,-35;1060,1925)" )
    assert_equal( inst.size, 200 )
    assert_equal( inst.is_complex?, true )
    assert_equal( inst.cplx_trans.to_s, "m45 *1.5 100,-50" )
    inst_ret = c2.insert( inst )
    assert_equal( c2.bbox.to_s, "(85,-35;1060,1925)" )
    assert_equal( inst == inst_ret.cell_inst, true )
    assert_equal( inst_ret.prop_id, 0 )

    child_insts = [] 
    c2.each_inst do
      |i| child_insts.push( i )
    end
    assert_equal( inst == child_insts[0].cell_inst, true )
    # in editable mode, the properties are present, but the id is 0:
    # therefore we do not check: assert_equal( inst_ret.has_prop_id?, false )
    assert_equal( child_insts[0].prop_id, 0 )
    assert_equal( c2.child_instances, 1 )

    arr = []
    c2.each_inst { |i| arr.push( i ) }
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].cell_inst == inst, true )

    arr = []
    c2.each_parent_inst { |i| arr.push( i ) }
    assert_equal( arr.size, 0 )

    arr = []
    c1.each_inst { |i| arr.push( i ) }
    assert_equal( arr.size, 0 )

    arr = []
    c1.each_parent_inst { |i| arr.push( i ) }
    assert_equal( arr.size, 1 )

    assert_equal( arr[0].parent_cell_index, c2.cell_index )
    assert_equal( arr[0].child_inst.cell_index, c1.cell_index )
    assert_equal( arr[0].inst.cell_index, c2.cell_index )
    assert_equal( arr[0].inst.cplx_trans.to_s, "m45 *0.666666667 33,-67" )
    
    arr = []
    c2.each_overlapping_inst( RBA::Box::new( 100, 0, 110, 10 ) ) { |i| arr.push( i ) }
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].cell_inst == inst, true )

    arr = []
    c2.each_overlapping_inst( RBA::Box::new( -100, 0, -90, 10 ) ) { |i| arr.push( i ) }
    assert_equal( arr.size, 0 )

    arr = []
    c2.each_touching_inst( RBA::Box::new( 100, 0, 110, 10 ) ) { |i| arr.push( i ) }
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].cell_inst == inst, true )

    arr = []
    c2.each_touching_inst( RBA::Box::new( -100, 0, -90, 10 ) ) { |i| arr.push( i ) }
    assert_equal( arr.size, 0 )

    arr = []
    c2.each_child_cell { |c| arr.push( ly.cell_name( c ) ) }
    assert_equal( arr, [ "c1" ] )
    arr = []
    c2.each_parent_cell { |c| arr.push( ly.cell_name( c ) ) }
    assert_equal( arr, [] )
    assert_equal( c2.child_cells, 1 )
    assert_equal( c2.parent_cells, 0 )
    assert_equal( c2.hierarchy_levels, 1 )

    arr = []
    c1.each_child_cell { |c| arr.push( ly.cell_name( c ) ) }
    assert_equal( arr, [] )
    arr = []
    c1.each_parent_cell { |c| arr.push( ly.cell_name( c ) ) }
    assert_equal( arr, [ "c2" ] )
    assert_equal( c1.child_cells, 0 )
    assert_equal( c1.parent_cells, 1 )
    assert_equal( c1.hierarchy_levels, 0 )

  end

  # Instances and editable mode
  def test_6_EditableLayout

    ly = RBA::Layout::new( true )
    assert_equal( ly.is_editable?, true )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    tr = RBA::CplxTrans::new( 1.5, 90.0, true, RBA::DPoint::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ret = c2.insert( inst )
    assert_equal( inst_ret.trans.to_s, "m45 100,-50" )
    inst = RBA::CellInstArray::new( c1.cell_index, RBA::CplxTrans::new )
    inst_ret.cell_inst = inst
    assert_equal( inst_ret.trans.to_s, "r0 0,0" )

    manager = RBA::Manager.new

    ly = RBA::Layout::new( true, manager )
    assert_equal( ly.is_editable?, true )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    tr = RBA::CplxTrans::new( 1.5, 90.0, true, RBA::DPoint::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ret = c2.insert( inst )
    assert_equal( inst_ret.trans.to_s, "m45 100,-50" )
    manager.transaction( "trans" )
    inst = RBA::CellInstArray::new( c1.cell_index, RBA::CplxTrans::new )
    inst_ret.cell_inst = inst
    assert_equal( inst_ret.trans.to_s, "r0 0,0" )
    manager.commit
    manager.undo
    c2 = ly.cell( ci2 )
    c2.each_inst { |i| inst_ret = i; break }
    assert_equal( inst_ret.trans.to_s, "m45 100,-50" )

    manager = nil

    ly = RBA::Layout::new( false )
    assert_equal( ly.is_editable?, false )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    tr = RBA::CplxTrans::new( 1.5, 90.0, true, RBA::DPoint::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst_ret = c2.insert( inst )
    assert_equal( inst_ret.trans.to_s, "m45 100,-50" )
    inst = RBA::CellInstArray::new( c1.cell_index, RBA::CplxTrans::new )
    error = true
    begin
      # should raise an error - non-editable layout does not support replacement of instances
      inst_ret.cell_inst = inst
      error = false
    rescue => ex
    end
    assert_equal( error, true )

  end

  # Instances and bboxes
  def test_6_Layout_props

    ly = RBA::Layout::new
    pid = ly.properties_id({ 17 => "a", "b" => [ 1, 5, 7 ] }.to_a)
    assert_equal(ly.properties_id({ 17 => "a", "b" => [ 1, 5, 7 ] }), pid)

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

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

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ), pid )
    assert_equal( c1.bbox.to_s, "(10,-10;50,40)" )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    assert_equal( inst.bbox( ly ).to_s, c1.bbox.transformed(tr).to_s )
    assert_equal( inst.bbox( ly, lindex ).to_s, c1.bbox.transformed(tr).to_s )
    assert_equal( inst.bbox( ly, ldummy ).to_s, "()" )
    assert_equal( inst.size, 1 )
    assert_equal( inst.is_complex?, false )
    c2.insert( inst, pid )
    assert_equal( c2.bbox.to_s, c1.bbox.transformed(tr).to_s )

    c2.clear_insts
    tr = RBA::CplxTrans::new( 1.5, 90.0, true, RBA::DPoint::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr )
    inst2 = inst.dup
    assert_equal( inst.bbox( ly ).to_s, c1.bbox.transformed(tr).to_s )
    assert_equal( inst.size, 1 )
    assert_equal( inst.is_complex?, true )
    assert_equal( inst.is_regular_array?, false )
    c2.insert( inst, pid )
    assert_equal( c2.bbox.to_s, c1.bbox.transformed(tr).to_s )

    c2.clear_insts
    assert_equal( c2.bbox.to_s, "()" )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) )
    inst = RBA::CellInstArray::new( c1.cell_index, tr, RBA::Point::new( 100, 0 ), RBA::Point::new( 0, 100 ), 10, 20 )
    assert_equal( inst == inst2, false )
    assert_equal( inst != inst2, true )
    inst2 = inst.dup
    assert_equal( inst == inst2, true )
    assert_equal( inst != inst2, false )
    assert_equal( inst.bbox( ly ).to_s, "(60,-40;1010,1900)" )
    assert_equal( inst.trans.to_s, "r90 100,-50" )
    assert_equal( inst.cplx_trans.to_s, "r90 *1 100,-50" )
    assert_equal( inst.size, 200 )
    assert_equal( inst.is_complex?, false )
    assert_equal( inst.is_regular_array?, true )
    assert_equal( inst.a.to_s, "100,0" )
    assert_equal( inst.b.to_s, "0,100" )
    assert_equal( inst.na, 10 )
    assert_equal( inst.nb, 20 )
    assert_equal( inst.cell_index, c1.cell_index )
    c2.insert( inst, pid )
    assert_equal( c2.bbox.to_s, "(60,-40;1010,1900)" )

    inst.invert
    assert_equal( inst == inst2, false )
    assert_equal( inst != inst2, true )
    assert_equal( inst.bbox( ly ).to_s, "(-1860,50;90,990)" )
    assert_equal( inst.size, 200 )
    assert_equal( inst.is_complex?, false )
    assert_equal( inst.is_regular_array?, true )
    assert_equal( inst.a.to_s, "0,100" )
    assert_equal( inst.b.to_s, "-100,0" )
    assert_equal( inst.na, 10 )
    assert_equal( inst.nb, 20 )
    assert_equal( inst.cell_index, c1.cell_index )

    c2.each_inst { |inst| c2.erase( inst ) }
    assert_equal( c2.bbox.to_s, "()" )

    tr = RBA::CplxTrans::new( 1.5, 90.0, true, RBA::DPoint::new( 100, -50 ) ) 
    inst = RBA::CellInstArray::new( c1.cell_index, tr, RBA::Point::new( 100, 0 ), RBA::Point::new( 0, 100 ), 10, 20 )
    assert_equal( inst.bbox( ly ).to_s, "(85,-35;1060,1925)" )
    assert_equal( inst.size, 200 )
    assert_equal( inst.is_complex?, true )
    assert_equal( inst.cplx_trans.to_s, "m45 *1.5 100,-50" )
    inst_ret = c2.insert( inst, pid )
    assert_equal( c2.bbox.to_s, "(85,-35;1060,1925)" )
    assert_equal( inst == inst_ret.cell_inst, true )
    assert_equal( inst_ret.has_prop_id?, true )
    assert_equal( inst_ret.prop_id, pid )

    child_insts = [] 
    c2.each_inst do
      |i| child_insts.push( i )
    end
    assert_equal( inst == child_insts[0].cell_inst, true )
    assert_equal( child_insts[0].has_prop_id?, true )
    assert_equal( child_insts[0].prop_id, pid )
    assert_equal( c2.child_instances, 1 )

    arr = []
    c2.each_inst { |i| arr.push( i ) }
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].cell_inst == inst, true )
    assert_equal( arr[0].prop_id == pid, true )

    arr = []
    c2.each_parent_inst { |i| arr.push( i ) }
    assert_equal( arr.size, 0 )

    arr = []
    c1.each_inst { |i| arr.push( i ) }
    assert_equal( arr.size, 0 )

    arr = []
    c1.each_parent_inst { |i| arr.push( i ) }
    assert_equal( arr.size, 1 )

    strarr = []
    c1.each_parent_inst { |i| strarr.push( i.inst.to_s ) }
    assert_equal( strarr.join(";"), "#1 m45 *0.666666667 33,-67 [0,-67*10;-67,0*20]" )

    strarr = []
    c1.each_parent_inst { |i| strarr.push( i.dinst.to_s ) }
    assert_equal( strarr.join(";"), "#1 m45 *0.666666667 0.033,-0.067 [0,-0.067*10;-0.067,0*20]" )

    assert_equal( arr[0].parent_cell_index, c2.cell_index )
    assert_equal( arr[0].child_inst.cell_index, c1.cell_index )
    assert_equal( arr[0].inst.cell_index, c2.cell_index )
    assert_equal( arr[0].inst.cplx_trans.to_s, "m45 *0.666666667 33,-67" )
    assert_equal( arr[0].child_inst.prop_id, pid )
    
    arr = []
    c2.each_overlapping_inst( RBA::Box::new( 100, 0, 110, 10 ) ) { |i| arr.push( i ) }
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].cell_inst == inst, true )
    assert_equal( arr[0].prop_id == pid, true )

    arr = []
    c2.each_overlapping_inst( RBA::Box::new( -100, 0, -90, 10 ) ) { |i| arr.push( i ) }
    assert_equal( arr.size, 0 )

    arr = []
    c2.each_touching_inst( RBA::Box::new( 100, 0, 110, 10 ) ) { |i| arr.push( i ) }
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].cell_inst == inst, true )
    assert_equal( arr[0].prop_id == pid, true )

    arr = []
    c2.each_touching_inst( RBA::Box::new( -100, 0, -90, 10 ) ) { |i| arr.push( i ) }
    assert_equal( arr.size, 0 )

    arr = []
    c2.each_child_cell { |c| arr.push( ly.cell_name( c ) ) }
    assert_equal( arr, [ "c1" ] )
    arr = []
    c2.each_parent_cell { |c| arr.push( ly.cell_name( c ) ) }
    assert_equal( arr, [] )
    assert_equal( c2.child_cells, 1 )
    assert_equal( c2.parent_cells, 0 )
    assert_equal( c2.hierarchy_levels, 1 )

    arr = []
    c1.each_child_cell { |c| arr.push( ly.cell_name( c ) ) }
    assert_equal( arr, [] )
    arr = []
    c1.each_parent_cell { |c| arr.push( ly.cell_name( c ) ) }
    assert_equal( arr, [ "c2" ] )
    assert_equal( c1.child_cells, 0 )
    assert_equal( c1.parent_cells, 1 )
    assert_equal( c1.hierarchy_levels, 0 )

  end

  # Properties
  def test_6_Layout_props2

    ly = RBA::Layout::new(true)
    pid = ly.properties_id( { 17 => "a", "b" => [ 1, 5, 7 ] }.to_a )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    assert_equal( c1.properties, {} )
    c2 = ly.cell( ci2 )
    tr = RBA::Trans::new
    inst = c2.insert( RBA::CellInstArray::new( c1.cell_index, tr ) )
    assert_equal( inst.parent_cell.name, c2.name )
    assert_equal( inst.cell.name, c1.name )
    assert_equal( inst.layout.inspect, ly.inspect )

    s1 = c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ), pid )
    s2 = c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    assert_equal( s1.property( 17 ).inspect, "\"a\"" )
    s1.set_property( 5, 23 ) 
    s1.delete_property( 17 )
    assert_equal( s1.property( 17 ).inspect, "nil" )
    assert_equal( s1.property( 5 ).inspect, "23" )
    assert_equal( s2.property( 17 ).inspect, "nil" )

    assert_equal( inst.property( "a" ).inspect, "nil" )
    inst.set_property( "a", 33 )
    assert_equal( inst.property( "a" ).inspect, "33" )
    inst.delete_property( "a" )
    assert_equal( inst.property( "a" ).inspect, "nil" )

    # cell properties
    assert_equal( c1.property( 17 ).inspect, "nil" )
    c1.prop_id = pid
    assert_equal( c1.prop_id, pid )
    assert_equal( c1.property( 17 ).inspect, "\"a\"" )
    assert_equal( c1.properties, { 17 => "a", "b" => [1, 5, 7] } )
    c1.set_property( 5, 23 ) 
    c1.delete_property( 17 )
    assert_equal( c1.property( 17 ).inspect, "nil" )
    assert_equal( c1.property( 5 ).inspect, "23" )

  end

  # Instances and bboxes (editable mode)
  def test_6_Layout_new

    ly = RBA::Layout::new
    pid1 = ly.properties_id( { 17 => "a", "b" => [ 1, 5, 7 ] }.to_a )
    pid2 = ly.properties_id( { 100 => "x" }.to_a )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )
    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst_1 = RBA::CellInstArray::new( c1.cell_index, tr )
    new_inst_1 = c2.insert( inst_1, pid1 )
    new_inst_2 = c2.insert( inst_1, pid2 )
    inst_2 = RBA::CellInstArray::new( c3.cell_index, tr*tr )
    new_inst_3 = c1.insert( inst_2 )

    assert_equal( new_inst_1.cell_index, c1.cell_index )
    assert_equal( new_inst_1.trans.to_s, tr.to_s )
    assert_equal( new_inst_1.prop_id, pid1 )
    assert_equal( new_inst_2.cell_index, c1.cell_index )
    assert_equal( new_inst_2.trans.to_s, tr.to_s )
    assert_equal( new_inst_2.prop_id, pid2 )
    assert_equal( new_inst_3.cell_index, c3.cell_index )
    assert_equal( new_inst_3.prop_id, 0 )
    assert_equal( new_inst_3.trans.to_s, (tr*tr).to_s )

    new_inst_3 = c1.replace_prop_id( new_inst_3, pid2 )
  
    assert_equal( new_inst_1.cell_index, c1.cell_index )
    assert_equal( new_inst_1.trans.to_s, tr.to_s )
    assert_equal( new_inst_1.prop_id, pid1 )
    assert_equal( new_inst_2.cell_index, c1.cell_index )
    assert_equal( new_inst_2.trans.to_s, tr.to_s )
    assert_equal( new_inst_2.prop_id, pid2 )
    assert_equal( new_inst_3.cell_index, c3.cell_index )
    assert_equal( new_inst_3.prop_id, pid2 )
    assert_equal( new_inst_3.trans.to_s, (tr*tr).to_s )

    begin
      new_inst_1 = c1.replace( new_inst_1, inst_2 )
      assert_equal( true, false )
    rescue
      # OK: gives an error since we are trying to erase an object from a list that is does not belong to
    end

    new_inst_1 = c2.replace( new_inst_1, inst_2 )
  
    assert_equal( new_inst_1.cell_index, c3.cell_index )
    assert_equal( new_inst_1.trans.to_s, (tr*tr).to_s )
    assert_equal( new_inst_1.prop_id, pid1 )
    assert_equal( new_inst_2.cell_index, c1.cell_index )
    assert_equal( new_inst_2.trans.to_s, tr.to_s )
    assert_equal( new_inst_2.prop_id, pid2 )
    assert_equal( new_inst_3.cell_index, c3.cell_index )
    assert_equal( new_inst_3.prop_id, pid2 )
    assert_equal( new_inst_3.trans.to_s, (tr*tr).to_s )

    new_inst_1 = c2.replace( new_inst_1, inst_2, pid1 )
  
    assert_equal( new_inst_1.cell_index, c3.cell_index )
    assert_equal( new_inst_1.trans.to_s, (tr*tr).to_s )
    assert_equal( new_inst_1.prop_id, pid1 )
    assert_equal( new_inst_2.cell_index, c1.cell_index )
    assert_equal( new_inst_2.trans.to_s, tr.to_s )
    assert_equal( new_inst_2.prop_id, pid2 )
    assert_equal( new_inst_3.cell_index, c3.cell_index )
    assert_equal( new_inst_3.prop_id, pid2 )
    assert_equal( new_inst_3.trans.to_s, (tr*tr).to_s )

    assert_equal( new_inst_1.is_null?, false )
    assert_equal( RBA::Instance.new.is_null?, true )

    assert_equal( c2.is_leaf?, false )
    c2.erase( new_inst_1 )
    c2.erase( new_inst_2 )
    assert_equal( c2.is_leaf?, true )
    assert_equal( c2.child_instances, 0 )

  end

  def shapes_to_s(ly, shapes)
    r = ""
    shapes.each do |s|
      r == "" || (r += ";")
      r += s.to_s
      if s.prop_id > 0
        pr = ""
        ly.properties(s.prop_id).each do |pp|
          pr == "" || (pr += ",")
          pr += pp[0].to_s + "=>" + pp[1].to_s
        end
        r += "["
        r += pr
        r += "]"
      end
    end
    r
  end

  # Copy/move between cells
  def test_7_cells_copy_move

    ly1 = RBA::Layout::new
    la1 = ly1.insert_layer(RBA::LayerInfo::new(1, 0))
    lb1 = ly1.insert_layer(RBA::LayerInfo::new(2, 0))
    ly1.dbu = 0.001
    ca1 = ly1.cell(ly1.add_cell("a"))
    cb1 = ly1.cell(ly1.add_cell("b"))

    s1 = ca1.shapes(la1).insert(RBA::Box::new(0, 500, 1000, 2000))
    s1.set_property(17, 5.0)
    s1.set_property(17, "hallo")
      
    ca1.copy(la1, lb1)
    cb1.copy(ca1, la1, lb1)
    assert_equal(shapes_to_s(ly1, ca1.shapes(la1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")
    assert_equal(shapes_to_s(ly1, ca1.shapes(lb1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")
    assert_equal(shapes_to_s(ly1, cb1.shapes(lb1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")
    assert_equal(shapes_to_s(ly1, cb1.shapes(la1)), "")

    ly2 = RBA::Layout::new
    la2 = ly2.insert_layer(RBA::LayerInfo::new(10, 0))
    lb2 = ly2.insert_layer(RBA::LayerInfo::new(11, 0))
    ly2.dbu = 0.0005
    ca2 = ly2.cell(ly2.add_cell("a"))
    cb2 = ly2.cell(ly2.add_cell("b"))

    ca2.copy(ca1, la1, lb2)
    assert_equal(shapes_to_s(ly2, ca2.shapes(lb2)), "box (0,1000;2000,4000) prop_id=1[17=>hallo]")
    assert_equal(shapes_to_s(ly1, ca1.shapes(la1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")

    # move
    ca1.clear
    cb1.clear
    ca2.clear
    cb2.clear
    
    s1 = ca1.shapes(la1).insert(RBA::Box::new(0, 500, 1000, 2000))
    s1.set_property(17, 5.0)
    s1.set_property(17, "hallo")
    assert_equal(shapes_to_s(ly1, ca1.shapes(la1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")
    assert_equal(shapes_to_s(ly1, ca1.shapes(lb1)), "")
    ca1.move(la1, lb1)
    assert_equal(shapes_to_s(ly1, ca1.shapes(la1)), "")
    assert_equal(shapes_to_s(ly1, ca1.shapes(lb1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")

    cb1.move(ca1, lb1, lb1)
    assert_equal(shapes_to_s(ly1, ca1.shapes(la1)), "")
    assert_equal(shapes_to_s(ly1, ca1.shapes(lb1)), "")
    assert_equal(shapes_to_s(ly1, cb1.shapes(lb1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")
    assert_equal(shapes_to_s(ly1, cb1.shapes(la1)), "")

    ly2 = RBA::Layout::new
    la2 = ly2.insert_layer(RBA::LayerInfo::new(10, 0))
    lb2 = ly2.insert_layer(RBA::LayerInfo::new(11, 0))
    ly2.dbu = 0.0005
    ca2 = ly2.cell(ly2.add_cell("a"))
    cb2 = ly2.cell(ly2.add_cell("b"))

    ca2.move(cb1, lb1, lb2)
    assert_equal(shapes_to_s(ly1, ca1.shapes(la1)), "")
    assert_equal(shapes_to_s(ly1, ca1.shapes(lb1)), "")
    assert_equal(shapes_to_s(ly1, cb1.shapes(lb1)), "")
    assert_equal(shapes_to_s(ly1, cb1.shapes(la1)), "")
    assert_equal(shapes_to_s(ly2, ca2.shapes(lb2)), "box (0,1000;2000,4000) prop_id=1[17=>hallo]")
    
  end

  # top cells
  def test_8

    l = RBA::Layout.new
    tc = []
    l.each_top_cell { |t| tc.push(l.cell(t).name) }
    assert_equal(tc.collect { |s| s.to_s }.join(","), "")
    assert_equal(l.top_cell.inspect, "nil")
    assert_equal(l.top_cells.collect { |t| t.name }.inspect, "[]")

    c0 = l.create_cell("c0")
    assert_equal(c0.name, "c0")
    tc = []
    l.each_top_cell { |t| tc.push(l.cell(t).name) }
    assert_equal(tc.collect { |s| s.to_s }.join(","), "c0")
    assert_equal(l.top_cell.name, "c0")
    assert_equal(l.top_cells.collect { |t| t.name }.join(","), "c0")
    lc = l.dup
    assert_equal(lc._to_const_object.top_cell.name, "c0")
    assert_equal(lc._to_const_object.top_cells.collect { |t| t.name }.join(","), "c0")

    c1 = l.create_cell("c1")
    assert_equal(c1.name, "c1")
    tc = []
    l.each_top_cell { |t| tc.push(l.cell(t).name) }
    assert_equal(tc.collect { |s| s.to_s }.join(","), "c0,c1")
    error = false
    begin
      assert_equal(l.top_cell.inspect, "never-true")
    rescue => ex
      error = true
    end
    assert_equal(error, true)
    assert_equal(l.top_cells.collect { |t| t.name }.join(","), "c0,c1")

    c2 = l.create_cell("c1")
    assert_equal(c2.name, "c1$1")

  end

  # under construction and update
  def test_9

    ly = RBA::Layout::new
    l1 = ly.insert_layer(RBA::LayerInfo::new(1, 0))
    c1 = ly.create_cell("a")
    assert_equal(ly.under_construction?, false)

    s1 = c1.shapes(l1).insert(RBA::Box::new(0, 500, 1000, 2000))
    assert_equal(c1.bbox.to_s, "(0,500;1000,2000)")

    ly = RBA::Layout::new
    ly.start_changes
    assert_equal(ly.under_construction?, true)
    ly.start_changes
    assert_equal(ly.under_construction?, true)
    l1 = ly.insert_layer(RBA::LayerInfo::new(1, 0))
    c1 = ly.create_cell("a")
    s1 = c1.shapes(l1).insert(RBA::Box::new(0, 500, 1000, 2000))
    assert_equal(c1.bbox.to_s, "()")
    # while under_construction, an explicit update is required to update the cell's bbox
    ly.update
    assert_equal(c1.bbox.to_s, "(0,500;1000,2000)")
    ly.end_changes
    assert_equal(ly.under_construction?, true)
    ly.end_changes
    assert_equal(ly.under_construction?, false)

  end

  # Instance editing
  def test_10

    ly = RBA::Layout::new

    ci1 = ly.add_cell("c1")
    ci2 = ly.add_cell("c2")
    ci3 = ly.add_cell("c3")

    c1 = ly.cell(ci1)
    c2 = ly.cell(ci2)
    c3 = ly.cell(ci3)

    tr = RBA::Trans::new(RBA::Trans::R90, RBA::Point::new(100, -50)) 
    i1 = c1.insert(RBA::CellInstArray::new(c2.cell_index, tr))

    str = []
    c1.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "c2 r90 100,-50")

    str = []
    c2.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "")

    str = []
    c3.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "")

    i1.cell_index = ci3

    str = []
    c1.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "c3 r90 100,-50")

    str = []
    c2.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "")

    str = []
    c3.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "")

    i1.cell = c2

    str = []
    c1.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "c2 r90 100,-50")

    str = []
    c2.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "")

    str = []
    c3.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "")

    assert_equal(i1.parent_cell.name, "c1")

    i1.cell = c1
    i1.parent_cell = c2

    str = []
    c1.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "")

    str = []
    c2.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "c1 r90 100,-50")

    str = []
    c3.each_inst { |i| str << i.to_s(true) }
    assert_equal(str.join(";"), "")

  end

  # User properties
  def test_11

    ly = RBA::Layout::new
    assert_equal(ly.properties, {})

    assert_equal(ly.prop_id, 0)
    ly.prop_id = 1
    assert_equal(ly.prop_id, 1)
    ly.prop_id = 0
    assert_equal(ly.prop_id, 0)

    ly.set_property("x", 1)
    assert_equal(ly.prop_id, 1)
    assert_equal(ly.property("x"), 1)
    assert_equal(ly.properties, {"x" => 1})
    assert_equal(ly.properties_hash(ly.prop_id), {"x" => 1})
    assert_equal(ly.properties_id(ly.properties_hash(ly.prop_id)), ly.prop_id)
    assert_equal(ly.properties_array(ly.prop_id), [["x", 1]])
    assert_equal(ly.properties_id(ly.properties_array(ly.prop_id)), ly.prop_id)
    ly.set_property("x", 17)
    assert_equal(ly.prop_id, 2)
    assert_equal(ly.property("x"), 17)
    assert_equal(ly.property("y"), nil)

    ly.delete_property("x")
    assert_equal(ly.property("x"), nil)

  end

  # Meta information
  def test_12a

    mi = RBA::LayoutMetaInfo::new("myinfo", "a")

    assert_equal(mi.name, "myinfo")
    assert_equal(mi.description, "")
    assert_equal(mi.value, "a")
    assert_equal(mi.is_persisted?, false)

    mi.name = "x"
    mi.description = "y"
    mi.value = "z"
    mi.persisted = true

    assert_equal(mi.name, "x")
    assert_equal(mi.description, "y")
    assert_equal(mi.value, "z")
    assert_equal(mi.is_persisted?, true)

    ly = RBA::Layout::new

    ly.add_meta_info(RBA::LayoutMetaInfo::new("myinfo", "a"))
    ly.add_meta_info(RBA::LayoutMetaInfo::new("another", 42, "description", true))

    assert_equal(ly.meta_info_value("myinfo"), "a")
    assert_equal(ly.meta_info_value("doesnotexist"), nil)
    assert_equal(ly.meta_info_value("another"), 42)

    a = []
    ly.each_meta_info { |mi| a << mi.name }
    assert_equal(a.join(","), "myinfo,another")
    a = []
    ly.each_meta_info { |mi| a << mi.value.to_s }
    assert_equal(a.join(","), "a,42")
    a = []
    ly.each_meta_info { |mi| a << mi.description }
    assert_equal(a.join(","), ",description")
    a = []
    ly.each_meta_info { |mi| a << mi.is_persisted?.to_s }
    assert_equal(a.join(","), "false,true")

    ly.add_meta_info(RBA::LayoutMetaInfo::new("myinfo", "b"))
    assert_equal(ly.meta_info_value("myinfo"), "b")
    assert_equal(ly.meta_info_value("doesnotexist"), nil)
    assert_equal(ly.meta_info_value("another"), 42)

    ly.remove_meta_info("doesnotexist")  # should not fail

    ly.remove_meta_info("myinfo")
    assert_equal(ly.meta_info_value("myinfo"), nil)
    assert_equal(ly.meta_info_value("doesnotexist"), nil)
    assert_equal(ly.meta_info_value("another"), 42)

    assert_equal(ly.meta_info("doesnotexist"), nil)
    assert_equal(ly.meta_info("another").value, 42)
    assert_equal(ly.meta_info("another").description, "description")

    ly.clear_meta_info
    assert_equal(ly.meta_info_value("another"), nil)
    assert_equal(ly.meta_info("another"), nil)

    # cellwise

    c1 = ly.create_cell("X")
    c2 = ly.create_cell("U")

    c1.add_meta_info(RBA::LayoutMetaInfo::new("a", true))
    c1.add_meta_info(RBA::LayoutMetaInfo::new("b", [ 1, 17, 42 ], "description", true))

    assert_equal(c2.meta_info("a"), nil)
    assert_equal(c2.meta_info_value("a"), nil)

    a = []
    c2.each_meta_info { |mi| a << mi.value.to_s }
    assert_equal(a.join(","), "")

    assert_equal(c1.meta_info("a").value, true)
    assert_equal(c1.meta_info("b").value, [ 1, 17, 42 ])
    assert_equal(c1.meta_info_value("b"), [ 1, 17, 42 ])

    a = []
    c1.each_meta_info { |mi| a << mi.value.to_s }
    assert_equal(a.join(","), "true,[1, 17, 42]")

    a = []
    c1.each_meta_info { |mi| a << mi.description }
    assert_equal(a.join(","), ",description")

    a = []
    c1.each_meta_info { |mi| a << mi.is_persisted?.to_s }
    assert_equal(a.join(","), "false,true")

    c1.remove_meta_info("doesnotexist")   # should not fail

    a = []
    c1.each_meta_info { |mi| a << mi.value.to_s }
    assert_equal(a.join(","), "true,[1, 17, 42]")

    c1.remove_meta_info("b")

    a = []
    c1.each_meta_info { |mi| a << mi.value.to_s }
    assert_equal(a.join(","), "true")

    c1.clear_meta_info

    a = []
    c1.each_meta_info { |mi| a << mi.value.to_s }
    assert_equal(a.join(","), "")

  end

  # Slow cleanup test
  def test_13

    n = 100
    w = 10000

    ly = RBA::Layout::new
    l1 = ly.layer(1, 0)
    top = ly.create_cell("TOP")

    n.times do |ix|
      $stdout.puts("#{ix}/#{n}")
      $stdout.flush
      n.times do |iy|
        x = ix * w
        y = iy * w
        cell = ly.create_cell("X#{ix}Y#{iy}")
        cell.shapes(l1).insert(RBA::Box::new(0, 0, w, w))
        top.insert(RBA::CellInstArray::new(cell.cell_index, RBA::Trans::new(RBA::Point::new(ix * w, iy * w))))
      end
    end

    # took forever:
    ly._destroy

  end

  # Cell#transform and Cell#transform_into
  def test_14

    g = RBA::Layout::new
    c0 = g.create_cell("c0")
    c1 = g.create_cell("c1")

    t = RBA::Trans::new(RBA::Vector::new(100, -100))
    inst = c0.insert(RBA::CellInstArray::new(c1.cell_index, t))

    ti = RBA::ICplxTrans::new(2.5, 45.0, false, RBA::Vector::new(10, 20))
    t = RBA::Trans::new(1)

    assert_equal(inst.to_s, "cell_index=1 r0 100,-100")

    c0.transform_into(t)
    assert_equal(inst.to_s, "cell_index=1 r0 100,100")

    c0.transform_into(ti)
    assert_equal(inst.to_s, "cell_index=1 r0 0,354")

    c0.transform(t)
    assert_equal(inst.to_s, "cell_index=1 r90 -354,0")

    c0.transform(ti)
    assert_equal(inst.to_s, "cell_index=1 r135 *2.5 -616,-606")

    g = RBA::Layout::new
    c0 = g.create_cell("c0")
    c1 = g.create_cell("c1")

    t = RBA::Trans::new(RBA::Vector::new(100, -100))
    inst = c0.insert(RBA::CellInstArray::new(c1.cell_index, t))

    ti = RBA::DCplxTrans::new(2.5, 45.0, false, RBA::DVector::new(0.01, 0.02))
    t = RBA::DTrans::new(1)

    assert_equal(inst.to_s, "cell_index=1 r0 100,-100")

    c0.transform_into(t)
    assert_equal(inst.to_s, "cell_index=1 r0 100,100")

    c0.transform_into(ti)
    assert_equal(inst.to_s, "cell_index=1 r0 0,354")

    c0.transform(t)
    assert_equal(inst.to_s, "cell_index=1 r90 -354,0")

    c0.transform(ti)
    assert_equal(inst.to_s, "cell_index=1 r135 *2.5 -616,-606")

  end

  # Layout#cells
  def test_15

    g = RBA::Layout::new
    c1 = g.create_cell("B1")
    c2 = g.create_cell("B2")
    c0 = g.create_cell("A")
    c0.insert(RBA::CellInstArray::new(c1.cell_index, RBA::Trans::new))
    c0.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new))

    assert_equal(g.cells("B*").collect(&:name).join(","), "B1,B2")
    assert_equal(g.cells("*").collect(&:name).join(","), "A,B1,B2")
    assert_equal(g.cells("A").collect(&:name).join(","), "A")
    assert_equal(g.cells("X").collect(&:name).join(","), "")

  end

  # Cell#read and meta info (issue #1609)
  def test_16

    tmp = File::join($ut_testtmp, "test16.gds")

    ly1 = RBA::Layout::new
    a = ly1.create_cell("A")
    b = ly1.create_cell("B")
    a.insert(RBA::DCellInstArray::new(b, RBA::Trans::new))

    a.add_meta_info(RBA::LayoutMetaInfo::new("am1", 42.0, "", true))
    a.add_meta_info(RBA::LayoutMetaInfo::new("am2", "u", "", true))
    assert_equal(mi2s(a), "am1:42.0;am2:u")

    b.add_meta_info(RBA::LayoutMetaInfo::new("bm1", 17, "", true))
    assert_equal(mi2s(b), "bm1:17")

    ly1.add_meta_info(RBA::LayoutMetaInfo::new("lm1", -2.0, "", true))
    ly1.add_meta_info(RBA::LayoutMetaInfo::new("lm2", "v", "", true))
    assert_equal(mi2s(ly1), "lm1:-2.0;lm2:v")

    ly1.write(tmp)

    ly2 = RBA::Layout::new
    top = ly2.create_cell("TOP")
    a = ly2.create_cell("A")
    c = ly2.create_cell("C")
    top.insert(RBA::DCellInstArray::new(a, RBA::Trans::new))
    a.insert(RBA::DCellInstArray::new(c, RBA::Trans::new))

    top.add_meta_info(RBA::LayoutMetaInfo::new("topm1", "abc"))
    assert_equal(mi2s(top), "topm1:abc")
    a.add_meta_info(RBA::LayoutMetaInfo::new("am1", "a number"))
    a.add_meta_info(RBA::LayoutMetaInfo::new("am3", 0))
    assert_equal(mi2s(a), "am1:a number;am3:0")
    c.add_meta_info(RBA::LayoutMetaInfo::new("cm1", 3))
    assert_equal(mi2s(c), "cm1:3")

    ly2.add_meta_info(RBA::LayoutMetaInfo::new("lm1", 5))
    assert_equal(mi2s(ly2), "lm1:5")

    a.read(tmp)
    # not modified
    assert_equal(mi2s(ly2), "lm1:5")
    # merged
    assert_equal(mi2s(a), "am1:42.0;am2:u;am3:0")
    # not modified
    assert_equal(mi2s(c), "cm1:3")

    b2 = ly2.cell("B")
    # imported
    assert_equal(mi2s(b2), "bm1:17")

    puts "done."

  end

  # Layout, meta info diverse
  def test_17

    manager = RBA::Manager::new

    ly = RBA::Layout::new(manager)
    a = ly.create_cell("A")

    manager.transaction("trans")
    ly.add_meta_info(RBA::LayoutMetaInfo::new("lm1", 17))
    a.add_meta_info(RBA::LayoutMetaInfo::new("am1", "u"))
    manager.commit

    assert_equal(mi2s(ly), "lm1:17")
    assert_equal(mi2s(a), "am1:u")

    manager.undo
    assert_equal(mi2s(ly), "")
    assert_equal(mi2s(a), "")

    manager.redo
    assert_equal(mi2s(ly), "lm1:17")
    assert_equal(mi2s(a), "am1:u")

    manager.transaction("trans")
    ly.add_meta_info(RBA::LayoutMetaInfo::new("lm1", 117))
    a.add_meta_info(RBA::LayoutMetaInfo::new("am1", "v"))
    manager.commit

    assert_equal(mi2s(ly), "lm1:117")
    assert_equal(mi2s(a), "am1:v")

    manager.undo
    assert_equal(mi2s(ly), "lm1:17")
    assert_equal(mi2s(a), "am1:u")

    manager.redo
    assert_equal(mi2s(ly), "lm1:117")
    assert_equal(mi2s(a), "am1:v")

    manager.undo
    assert_equal(mi2s(ly), "lm1:17")
    assert_equal(mi2s(a), "am1:u")

    manager.transaction("trans")
    ly.remove_meta_info("lm1")
    a.remove_meta_info("am1")
    a.remove_meta_info("doesnotexist")
    manager.commit

    assert_equal(mi2s(ly), "")
    assert_equal(mi2s(a), "")

    manager.undo
    assert_equal(mi2s(ly), "lm1:17")
    assert_equal(mi2s(a), "am1:u")

    manager.transaction("trans")
    ly.clear_all_meta_info
    manager.commit

    assert_equal(mi2s(ly), "")
    assert_equal(mi2s(a), "")

    manager.undo
    assert_equal(mi2s(ly), "lm1:17")
    assert_equal(mi2s(a), "am1:u")

    manager.redo
    assert_equal(mi2s(ly), "")
    assert_equal(mi2s(a), "")

    ly2 = RBA::Layout::new
    ly.add_meta_info(RBA::LayoutMetaInfo::new("lm1", 17))
    ly2.add_meta_info(RBA::LayoutMetaInfo::new("lm2", 42))
    assert_equal(mi2s(ly), "lm1:17")
    ly.merge_meta_info(ly2)
    assert_equal(mi2s(ly), "lm1:17;lm2:42")
    ly.copy_meta_info(ly2)
    assert_equal(mi2s(ly), "lm2:42")

    a = ly.create_cell("A")
    a.add_meta_info(RBA::LayoutMetaInfo::new("am1", "u"))
    b = ly2.create_cell("B")
    b.add_meta_info(RBA::LayoutMetaInfo::new("bm1", "v"))

    assert_equal(mi2s(a), "am1:u")
    a.merge_meta_info(b)
    assert_equal(mi2s(a), "am1:u;bm1:v")
    a.copy_meta_info(b)
    assert_equal(mi2s(a), "bm1:v")

    ly = RBA::Layout::new
    ly2 = RBA::Layout::new

    a = ly.create_cell("A")
    a.add_meta_info(RBA::LayoutMetaInfo::new("am1", "u"))
    ly2.create_cell("X") 
    b = ly2.create_cell("B")
    b.add_meta_info(RBA::LayoutMetaInfo::new("bm1", "v"))

    cm = RBA::CellMapping::new
    cm.map(b.cell_index, a.cell_index)

    assert_equal(mi2s(a), "am1:u")
    ly.merge_meta_info(ly2, cm)
    assert_equal(mi2s(a), "am1:u;bm1:v")
    ly.copy_meta_info(ly2, cm)
    assert_equal(mi2s(a), "bm1:v")

  end

  def test_read_bytes

    file_gds = File::join($ut_testtmp, "bytes.gds")

    ly = RBA::Layout::new
    top = ly.create_cell("TOP")
    l1 = ly.layer(1, 0)
    shape = top.shapes(l1).insert(RBA::Box::new(0, 10, 20, 30))
    ly.write(file_gds)

    byte_buffer = File.open(file_gds, "rb") { |f| f.read }

    ly2 = RBA::Layout::new
    ly2.read_bytes(byte_buffer, RBA::LoadLayoutOptions::new)
    l2 = ly2.layer(1, 0)
    assert_equal(ly2.top_cell.bbox.to_s, "(0,10;20,30)")

  end

  def test_write_bytes

    ly = RBA::Layout::new
    top = ly.create_cell("TOP")
    l1 = ly.layer(1, 0)
    shape = top.shapes(l1).insert(RBA::Box::new(0, 10, 20, 30))
    options = RBA::SaveLayoutOptions::new
    options.format = "GDS2"
    byte_buffer = ly.write_bytes(options)

    ly2 = RBA::Layout::new
    ly2.read_bytes(byte_buffer)
    l2 = ly2.layer(1, 0)
    assert_equal(ly2.top_cell.bbox.to_s, "(0,10;20,30)")

  end

end

load("test_epilogue.rb")

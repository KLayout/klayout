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


import pya
import unittest
import sys
import os

class DBLayoutTest(unittest.TestCase):

  # LayerInfo
  def test_1_Layout(self):

    lp = pya.LayerInfo()

    lp.name = "hallo"
    lp.layer = 5
    lp.datatype = 7
    self.assertEqual( lp.name, "hallo" )
    self.assertEqual( lp.layer, 5 )
    self.assertEqual( lp.datatype, 7 )
    self.assertEqual( pya.LayerInfo.from_string(lp.to_s()).to_s(), "hallo (5/7)" )

    lp.name = "x"
    self.assertEqual( lp.name, "x" )
    self.assertEqual( lp.layer, 5 )
    self.assertEqual( lp.datatype, 7 )

    lp.layer = 15
    self.assertEqual( lp.name, "x" )
    self.assertEqual( lp.layer, 15 )
    self.assertEqual( lp.datatype, 7 )

    lp.datatype = 3
    self.assertEqual( lp.name, "x" )
    self.assertEqual( lp.layer, 15 )
    self.assertEqual( lp.datatype, 3 )

    self.assertEqual( pya.LayerInfo.from_string("").to_s(), "" )
    self.assertEqual( pya.LayerInfo.from_string("name").to_s(), "name" )
    self.assertEqual( pya.LayerInfo.from_string("1").to_s(), "1/0" )
    self.assertEqual( pya.LayerInfo.from_string("name (17)").to_s(), "name (17/0)" )
    self.assertEqual( pya.LayerInfo.from_string("'1' (8/5)").to_s(), "'1' (8/5)" )

  # Basics: cells and instances
  def test_2_Layout(self):

    ly = pya.Layout()

    ly.dbu = 0.25
    self.assertEqual( ly.dbu, 0.25 )

    self.assertEqual( ly.has_cell("cell"), False )
    self.assertEqual( ly.cells(), 0 )

    ci = ly.add_cell( "new_cell" )
    self.assertEqual( ly.cells(), 1 )
    self.assertEqual( ly.has_cell("cell"), False )
    self.assertEqual( ly.has_cell("new_cell"), True )
    self.assertEqual( ly.cell_name(ci), "new_cell" )

    self.assertEqual( ly.cell_by_name("new_cell"), ci )
    self.assertEqual( ly.cell(ci).cell_index(), ci )
    self.assertEqual( ly.cell("new_cell").name, "new_cell" )
    self.assertEqual( repr(ly.cell("x")), "None" )
    lyc = ly.dup()
    self.assertEqual( lyc._to_const_object().cell("new_cell").name, "new_cell" )
    self.assertEqual( lyc._to_const_object().cell(ci).cell_index(), ci )

    ci2 = ly.add_cell( "new_cell_b" )
    self.assertEqual( ly.cells(), 2 )
    self.assertEqual( ly.cell_by_name("new_cell_b"), ci2 )
    self.assertEqual( sorted([ c.name for c in ly.cells("new*") ]), ['new_cell', 'new_cell_b'] )
    self.assertEqual( ci != ci2, True )
    lyc = ly.dup()
    self.assertEqual( sorted([ c.name for c in lyc._to_const_object().cells("new*") ]), ['new_cell', 'new_cell_b'] )

    ly.rename_cell( ci2, "x" )
    self.assertEqual( ly.cell_by_name("x"), ci2 )
    self.assertEqual( ly.cell_name(ci), "new_cell" )
    self.assertEqual( ly.cell_name(ci2), "x" )

    self.assertEqual( ly.under_construction(), False )
    ly.start_changes() 
    self.assertEqual( ly.under_construction(), True )
    ly.end_changes() 
    self.assertEqual( ly.under_construction(), False )

    li = pya.LayerInfo()
    li.layer = 16
    li.datatype = 1
    lindex = ly.insert_layer( li )
    self.assertEqual( ly.is_valid_layer( lindex ), True )
    self.assertEqual( ly.is_special_layer( lindex ), False )
    self.assertEqual( ly.is_valid_layer( 1234 ), False )

    li2 = ly.get_info( lindex ).dup()
    self.assertEqual( li2.layer, li.layer )
    self.assertEqual( li2.datatype, li.datatype )

    li2.layer = 17
    li2.datatype = 2

    ly.set_info( lindex, li2 )
    li3 = ly.get_info( lindex )
    self.assertEqual( li3.layer, 17 )
    self.assertEqual( li3.datatype, 2 )

    ly.delete_layer( lindex )
    self.assertEqual( ly.is_valid_layer( lindex ), False )

    arr = []
    for c in ly.each_cell():
      arr.append( c.cell_index() )
    self.assertEqual( arr, [ 0, 1 ] )

    arr = []
    for c in ly.each_top_cell():
      arr.append( c )
    self.assertEqual( arr, [ 0, 1 ] )

    arr = []
    for c in ly.each_cell_top_down():
      arr.append( c )
    self.assertEqual( arr, [ 0, 1 ] )

    arr = []
    for c in ly.each_cell_bottom_up():
      arr.append( c )
    self.assertEqual( arr, [ 1, 0 ] )

    ly.cell(ci2).insert( pya.CellInstArray( ci, pya.Trans( pya.Trans.R90, pya.Point( 1000, -500 ) ) ) )

    arr = []
    for c in ly.each_cell():
      arr.append( c.cell_index() )
    self.assertEqual( arr, [ 0, 1 ] )

    arr = []
    for c in ly.each_top_cell():
      arr.append( c )
    self.assertEqual( arr, [ 1 ] )

    arr = []
    for c in ly.each_cell_top_down():
      arr.append( c )
    self.assertEqual( arr, [ 1, 0 ] )

    arr = []
    for c in ly.each_cell_bottom_up():
      arr.append( c )
    self.assertEqual( arr, [ 0, 1 ] )

    self.assertEqual( ly.cell(ci2).is_top(), True )
    self.assertEqual( ly.cell(ci).is_top(), False )
    self.assertEqual( ly.cell(ci2).is_leaf(), False )
    self.assertEqual( ly.cell(ci).is_leaf(), True )

    ly.cell(ci2).clear_insts()

    arr = []
    for c in ly.each_cell():
      arr.append( c.cell_index() )
    self.assertEqual( arr, [ 0, 1 ] )

    arr = []
    for c in ly.each_top_cell():
      arr.append( c )
    self.assertEqual( arr, [ 0, 1 ] )

    arr = []
    for c in ly.each_cell_top_down():
      arr.append( c )
    self.assertEqual( arr, [ 0, 1 ] )

    arr = []
    for c in ly.each_cell_bottom_up():
      arr.append( c )
    self.assertEqual( arr, [ 1, 0 ] )

  # Instances and bboxes
  def test_5_Layout(self):

    ly = pya.Layout()

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = pya.LayerInfo()
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = pya.LayerInfo()
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    c1.shapes( lindex ).insert( pya.Box( 10, -10, 50, 40 ) )
    self.assertEqual( c1.bbox().to_s(), "(10,-10;50,40)" )
    self.assertEqual( c1.bbox_per_layer( lindex ).to_s(), "(10,-10;50,40)" )
    self.assertEqual( c1.bbox_per_layer( ldummy ).to_s(), "()" )

    c1.swap( lindex, ldummy )
    self.assertEqual( c1.bbox_per_layer( lindex ).to_s(), "()" )
    self.assertEqual( c1.bbox_per_layer( ldummy ).to_s(), "(10,-10;50,40)" )
    
    c1.clear( lindex )
    c1.clear( ldummy )
    self.assertEqual( c1.bbox_per_layer( lindex ).to_s(), "()" )
    self.assertEqual( c1.bbox_per_layer( ldummy ).to_s(), "()" )
    
    c1.shapes( lindex ).insert( pya.Box( 10, -10, 50, 40 ) )
    self.assertEqual( c1.bbox_per_layer( lindex ).to_s(), "(10,-10;50,40)" )
    self.assertEqual( c1.bbox_per_layer( ldummy ).to_s(), "()" )
    c1.clear_shapes()
    self.assertEqual( c1.bbox_per_layer( lindex ).to_s(), "()" )
    self.assertEqual( c1.bbox_per_layer( ldummy ).to_s(), "()" )

  # Instances and bboxes
  def test_6_Layout(self):

    ly = pya.Layout()

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = pya.LayerInfo()
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = pya.LayerInfo()
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    c1.shapes( lindex ).insert( pya.Box( 10, -10, 50, 40 ) )
    self.assertEqual( c1.bbox().to_s(), "(10,-10;50,40)" )

    tr = pya.Trans( pya.Trans.R90, pya.Point( 100, -50 ) ) 
    inst = pya.CellInstArray( c1.cell_index(), tr )
    self.assertEqual( inst.bbox( ly ).to_s(), c1.bbox().transformed(tr).to_s() )
    self.assertEqual( inst.bbox_per_layer( ly, lindex ).to_s(), c1.bbox().transformed(tr).to_s() )
    self.assertEqual( inst.bbox_per_layer( ly, ldummy ).to_s(), "()" )
    self.assertEqual( inst.size(), 1 )
    self.assertEqual( inst.is_complex(), False )
    c2.insert( inst )
    self.assertEqual( c2.bbox().to_s(), c1.bbox().transformed(tr).to_s() )

    c2.clear_insts()
    tr = pya.CplxTrans( 1.5, 90.0, True, pya.DPoint( 100, -50 ) ) 
    inst = pya.CellInstArray( c1.cell_index(), tr )
    inst2 = inst.dup()
    self.assertEqual( inst.bbox( ly ).to_s(), c1.bbox().transformed(tr).to_s() )
    self.assertEqual( inst.size(), 1 )
    self.assertEqual( inst.is_complex(), True )
    self.assertEqual( inst.is_regular_array(), False )
    c2.insert( inst )
    self.assertEqual( c2.bbox().to_s(), c1.bbox().transformed(tr).to_s() )

    c2.clear_insts()
    self.assertEqual( c2.bbox().to_s(), "()" )

    tr = pya.Trans( pya.Trans.R90, pya.Point( 100, -50 ) )
    inst = pya.CellInstArray( c1.cell_index(), tr, pya.Point( 100, 0 ), pya.Point( 0, 100 ), 10, 20 )
    self.assertEqual( inst == inst2, False )
    self.assertEqual( inst != inst2, True )
    inst2 = inst.dup()
    self.assertEqual( inst == inst2, True )
    self.assertEqual( inst != inst2, False )
    self.assertEqual( inst.bbox( ly ).to_s(), "(60,-40;1010,1900)" )
    self.assertEqual( inst.trans.to_s(), "r90 100,-50" )
    self.assertEqual( inst.cplx_trans.to_s(), "r90 *1 100,-50" )
    self.assertEqual( inst.size(), 200 )
    self.assertEqual( inst.is_complex(), False )
    self.assertEqual( inst.is_regular_array(), True )
    self.assertEqual( inst.a.to_s(), "100,0" )
    self.assertEqual( inst.b.to_s(), "0,100" )
    self.assertEqual( inst.na, 10 )
    self.assertEqual( inst.nb, 20 )
    self.assertEqual( inst.cell_index, c1.cell_index() )
    c2.insert( inst )
    self.assertEqual( c2.bbox().to_s(), "(60,-40;1010,1900)" )

    inst.invert()
    self.assertEqual( inst == inst2, False )
    self.assertEqual( inst != inst2, True )
    self.assertEqual( inst.bbox( ly ).to_s(), "(-1860,50;90,990)" )
    self.assertEqual( inst.size(), 200 )
    self.assertEqual( inst.is_complex(), False )
    self.assertEqual( inst.is_regular_array(), True )
    self.assertEqual( inst.a.to_s(), "0,100" )
    self.assertEqual( inst.b.to_s(), "-100,0" )
    self.assertEqual( inst.na, 10 )
    self.assertEqual( inst.nb, 20 )
    self.assertEqual( inst.cell_index, c1.cell_index() )

    if ly.is_editable():
      for inst in c2.each_inst():
        c2.erase( inst )
      self.assertEqual( c2.bbox().to_s(), "()" )
    else:
      c2.clear_insts()

    tr = pya.CplxTrans( 1.5, 90.0, True, pya.DPoint( 100, -50 ) ) 
    inst = pya.CellInstArray( c1.cell_index(), tr, pya.Point( 100, 0 ), pya.Point( 0, 100 ), 10, 20 )
    self.assertEqual( inst.bbox( ly ).to_s(), "(85,-35;1060,1925)" )
    self.assertEqual( inst.size(), 200 )
    self.assertEqual( inst.is_complex(), True )
    self.assertEqual( inst.cplx_trans.to_s(), "m45 *1.5 100,-50" )
    inst_ret = c2.insert( inst )
    self.assertEqual( c2.bbox().to_s(), "(85,-35;1060,1925)" )
    self.assertEqual( inst == inst_ret.cell_inst, True )
    self.assertEqual( inst_ret.prop_id, 0 )

    child_insts = [] 
    for i in c2.each_inst():
      child_insts.append( i )
    self.assertEqual( inst == child_insts[0].cell_inst, True )
    # in editable mode, the properties are present, but the id is 0:
    # therefore we do not check: self.assertEqual( inst_ret.has_prop_id, False )
    self.assertEqual( child_insts[0].prop_id, 0 )
    self.assertEqual( c2.child_instances(), 1 )

    arr = []
    for i in c2.each_inst():
      arr.append( i )
    self.assertEqual( len(arr), 1 )
    self.assertEqual( arr[0].cell_inst == inst, True )

    arr = []
    for i in c2.each_parent_inst():
      arr.append( i )
    self.assertEqual( len(arr), 0 )

    arr = []
    for i in c1.each_inst():
      arr.append( i )
    self.assertEqual( len(arr), 0 )

    arr = []
    for i in c1.each_parent_inst():
      arr.append( i )
    self.assertEqual( len(arr), 1 )

    self.assertEqual( arr[0].parent_cell_index(), c2.cell_index() )
    self.assertEqual( arr[0].child_inst().cell_index, c1.cell_index() )
    self.assertEqual( arr[0].inst().cell_index, c2.cell_index() )
    self.assertEqual( arr[0].inst().cplx_trans.to_s(), "m45 *0.666666667 33,-67" )
    
    arr = []
    for i in c2.each_overlapping_inst( pya.Box( 100, 0, 110, 10 ) ):
      arr.append( i )
    self.assertEqual( len(arr), 1 )
    self.assertEqual( arr[0].cell_inst == inst, True )

    arr = []
    for i in c2.each_overlapping_inst( pya.Box( -100, 0, -90, 10 ) ):
      arr.append( i )
    self.assertEqual( len(arr), 0 )

    arr = []
    for i in c2.each_touching_inst( pya.Box( 100, 0, 110, 10 ) ):
      arr.append( i )
    self.assertEqual( len(arr), 1 )
    self.assertEqual( arr[0].cell_inst == inst, True )

    arr = []
    for i in c2.each_touching_inst( pya.Box( -100, 0, -90, 10 ) ):
      arr.append( i )
    self.assertEqual( len(arr), 0 )

    arr = []
    for c in c2.each_child_cell():
      arr.append( ly.cell_name( c ) )
    self.assertEqual( arr, [ "c1" ] )
    arr = []
    for c in c2.each_parent_cell(): 
      arr.append( ly.cell_name( c ) )
    self.assertEqual( arr, [] )
    self.assertEqual( c2.child_cells(), 1 )
    self.assertEqual( c2.parent_cells(), 0 )
    self.assertEqual( c2.hierarchy_levels(), 1 )

    arr = []
    for c in c1.each_child_cell():
      arr.append( ly.cell_name( c ) )
    self.assertEqual( arr, [] )
    arr = []
    for c in c1.each_parent_cell():
      arr.append( ly.cell_name( c ) )
    self.assertEqual( arr, [ "c2" ] )
    self.assertEqual( c1.child_cells(), 0 )
    self.assertEqual( c1.parent_cells(), 1 )
    self.assertEqual( c1.hierarchy_levels(), 0 )


  # Instances and editable mode
  def test_6_EditableLayout(self):

    ly = pya.Layout( True )
    self.assertEqual( ly.is_editable(), True )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    tr = pya.CplxTrans( 1.5, 90.0, True, pya.DPoint( 100, -50 ) ) 
    inst = pya.CellInstArray( c1.cell_index(), tr )
    inst_ret = c2.insert( inst )
    self.assertEqual( inst_ret.trans.to_s(), "m45 100,-50" )
    inst = pya.CellInstArray( c1.cell_index(), pya.CplxTrans() )
    inst_ret.cell_inst = inst
    self.assertEqual( inst_ret.trans.to_s(), "r0 0,0" )

    manager = pya.Manager()

    ly = pya.Layout( True, manager )
    self.assertEqual( ly.is_editable(), True )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    tr = pya.CplxTrans( 1.5, 90.0, True, pya.DPoint( 100, -50 ) ) 
    inst = pya.CellInstArray( c1.cell_index(), tr )
    inst_ret = c2.insert( inst )
    self.assertEqual( inst_ret.trans.to_s(), "m45 100,-50" )
    manager.transaction( "trans" )
    inst = pya.CellInstArray( c1.cell_index(), pya.CplxTrans() )
    inst_ret.cell_inst = inst
    self.assertEqual( inst_ret.trans.to_s(), "r0 0,0" )
    manager.commit()
    manager.undo()
    c2 = ly.cell( ci2 )
    for i in c2.each_inst():
      inst_ret = i
      break
    self.assertEqual( inst_ret.trans.to_s(), "m45 100,-50" )

    manager = None

    ly = pya.Layout( False )
    self.assertEqual( ly.is_editable(), False )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    tr = pya.CplxTrans( 1.5, 90.0, True, pya.DPoint( 100, -50 ) ) 
    inst = pya.CellInstArray( c1.cell_index(), tr )
    inst_ret = c2.insert( inst )
    self.assertEqual( inst_ret.trans.to_s(), "m45 100,-50" )
    inst = pya.CellInstArray( c1.cell_index(), pya.CplxTrans() )
    error = True
    try:
      # should raise an error - non-editable layout does not support replacement of instances
      inst_ret.cell_inst = inst
      error = False
    except:
      pass
    self.assertEqual( error, True )

  # Instances and bboxes
  def test_6_Layout_props(self):

    ly = pya.Layout()
    pv = [ [ 17, "a" ], [ "b", [ 1, 5, 7 ] ] ]
    pid = ly.properties_id( pv )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = pya.LayerInfo()
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = pya.LayerInfo()
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    c1.shapes( lindex ).insert( pya.Box( 10, -10, 50, 40 ), pid )
    self.assertEqual( c1.bbox().to_s(), "(10,-10;50,40)" )

    tr = pya.Trans( pya.Trans.R90, pya.Point( 100, -50 ) ) 
    inst = pya.CellInstArray( c1.cell_index(), tr )
    self.assertEqual( inst.bbox( ly ).to_s(), c1.bbox().transformed(tr).to_s() )
    self.assertEqual( inst.bbox_per_layer( ly, lindex ).to_s(), c1.bbox().transformed(tr).to_s() )
    self.assertEqual( inst.bbox_per_layer( ly, ldummy ).to_s(), "()" )
    self.assertEqual( inst.size(), 1 )
    self.assertEqual( inst.is_complex(), False )
    c2.insert( inst, pid )
    self.assertEqual( c2.bbox().to_s(), c1.bbox().transformed(tr).to_s() )

    c2.clear_insts()
    tr = pya.CplxTrans( 1.5, 90.0, True, pya.DPoint( 100, -50 ) ) 
    inst = pya.CellInstArray( c1.cell_index(), tr )
    inst2 = inst.dup()
    self.assertEqual( inst.bbox( ly ).to_s(), c1.bbox().transformed(tr).to_s() )
    self.assertEqual( inst.size(), 1 )
    self.assertEqual( inst.is_complex(), True )
    self.assertEqual( inst.is_regular_array(), False )
    c2.insert( inst, pid )
    self.assertEqual( c2.bbox().to_s(), c1.bbox().transformed(tr).to_s() )

    c2.clear_insts()
    self.assertEqual( c2.bbox().to_s(), "()" )

    tr = pya.Trans( pya.Trans.R90, pya.Point( 100, -50 ) )
    inst = pya.CellInstArray( c1.cell_index(), tr, pya.Point( 100, 0 ), pya.Point( 0, 100 ), 10, 20 )
    self.assertEqual( inst == inst2, False )
    self.assertEqual( inst != inst2, True )
    inst2 = inst.dup()
    self.assertEqual( inst == inst2, True )
    self.assertEqual( inst != inst2, False )
    self.assertEqual( inst.bbox( ly ).to_s(), "(60,-40;1010,1900)" )
    self.assertEqual( inst.trans.to_s(), "r90 100,-50" )
    self.assertEqual( inst.cplx_trans.to_s(), "r90 *1 100,-50" )
    self.assertEqual( inst.size(), 200 )
    self.assertEqual( inst.is_complex(), False )
    self.assertEqual( inst.is_regular_array(), True )
    self.assertEqual( inst.a.to_s(), "100,0" )
    self.assertEqual( inst.b.to_s(), "0,100" )
    self.assertEqual( inst.na, 10 )
    self.assertEqual( inst.nb, 20 )
    self.assertEqual( inst.cell_index, c1.cell_index() )
    c2.insert( inst, pid )
    self.assertEqual( c2.bbox().to_s(), "(60,-40;1010,1900)" )

    inst.invert()
    self.assertEqual( inst == inst2, False )
    self.assertEqual( inst != inst2, True )
    self.assertEqual( inst.bbox( ly ).to_s(), "(-1860,50;90,990)" )
    self.assertEqual( inst.size(), 200 )
    self.assertEqual( inst.is_complex(), False )
    self.assertEqual( inst.is_regular_array(), True )
    self.assertEqual( inst.a.to_s(), "0,100" )
    self.assertEqual( inst.b.to_s(), "-100,0" )
    self.assertEqual( inst.na, 10 )
    self.assertEqual( inst.nb, 20 )
    self.assertEqual( inst.cell_index, c1.cell_index() )

    if ly.is_editable():
      for inst in c2.each_inst():
        c2.erase( inst ) 
      self.assertEqual( c2.bbox().to_s(), "()" )
    else:
      c2.clear_insts()
    self.assertEqual( c2.bbox().to_s(), "()" )

    tr = pya.CplxTrans( 1.5, 90.0, True, pya.DPoint( 100, -50 ) ) 
    inst = pya.CellInstArray( c1.cell_index(), tr, pya.Point( 100, 0 ), pya.Point( 0, 100 ), 10, 20 )
    self.assertEqual( inst.bbox( ly ).to_s(), "(85,-35;1060,1925)" )
    self.assertEqual( inst.size(), 200 )
    self.assertEqual( inst.is_complex(), True )
    self.assertEqual( inst.cplx_trans.to_s(), "m45 *1.5 100,-50" )
    inst_ret = c2.insert( inst, pid )
    self.assertEqual( c2.bbox().to_s(), "(85,-35;1060,1925)" )
    self.assertEqual( inst == inst_ret.cell_inst, True )
    self.assertEqual( inst_ret.has_prop_id(), True )
    self.assertEqual( inst_ret.prop_id, pid )

    child_insts = [] 
    for i in c2.each_inst():
      child_insts.append( i )
    self.assertEqual( inst == child_insts[0].cell_inst, True )
    self.assertEqual( child_insts[0].has_prop_id(), True )
    self.assertEqual( child_insts[0].prop_id, pid )
    self.assertEqual( c2.child_instances(), 1 )

    arr = []
    for i in c2.each_inst():
      arr.append( i ) 
    self.assertEqual( len(arr), 1 )
    self.assertEqual( arr[0].cell_inst == inst, True )
    self.assertEqual( arr[0].prop_id == pid, True )

    arr = []
    for i in c2.each_parent_inst():
      arr.append( i )
    self.assertEqual( len(arr), 0 )

    arr = []
    for i in c1.each_inst():
      arr.append( i )
    self.assertEqual( len(arr), 0 )

    arr = []
    for i in c1.each_parent_inst():
      arr.append( i )
    self.assertEqual( len(arr), 1 )

    self.assertEqual( arr[0].parent_cell_index(), c2.cell_index() )
    self.assertEqual( arr[0].child_inst().cell_index, c1.cell_index() )
    self.assertEqual( arr[0].inst().cell_index, c2.cell_index() )
    self.assertEqual( arr[0].inst().cplx_trans.to_s(), "m45 *0.666666667 33,-67" )
    self.assertEqual( arr[0].child_inst().prop_id, pid )
    
    arr = []
    for i in c2.each_overlapping_inst( pya.Box( 100, 0, 110, 10 ) ):
      arr.append( i )
    self.assertEqual( len(arr), 1 )
    self.assertEqual( arr[0].cell_inst == inst, True )
    self.assertEqual( arr[0].prop_id == pid, True )

    arr = []
    for i in c2.each_overlapping_inst( pya.Box( -100, 0, -90, 10 ) ):
      arr.append( i )
    self.assertEqual( len(arr), 0 )

    arr = []
    for i in c2.each_touching_inst( pya.Box( 100, 0, 110, 10 ) ):
      arr.append( i )
    self.assertEqual( len(arr), 1 )
    self.assertEqual( arr[0].cell_inst == inst, True )
    self.assertEqual( arr[0].prop_id == pid, True )

    arr = []
    for i in c2.each_touching_inst( pya.Box( -100, 0, -90, 10 ) ):
      arr.append( i )
    self.assertEqual( len(arr), 0 )

    arr = []
    for c in c2.each_child_cell():
      arr.append( ly.cell_name( c ) )
    self.assertEqual( arr, [ "c1" ] )
    arr = []
    for c in c2.each_parent_cell():
      arr.append( ly.cell_name( c ) )
    self.assertEqual( arr, [] )
    self.assertEqual( c2.child_cells(), 1 )
    self.assertEqual( c2.parent_cells(), 0 )
    self.assertEqual( c2.hierarchy_levels(), 1 )

    arr = []
    for c in c1.each_child_cell():
      arr.append( ly.cell_name( c ) )
    self.assertEqual( arr, [] )
    arr = []
    for c in c1.each_parent_cell():
      arr.append( ly.cell_name( c ) )
    self.assertEqual( arr, [ "c2" ] )
    self.assertEqual( c1.child_cells(), 0 )
    self.assertEqual( c1.parent_cells(), 1 )
    self.assertEqual( c1.hierarchy_levels(), 0 )


  # Properties
  def test_6_Layout_props2(self):

    ly = pya.Layout(True)
    pv = [ [ 17, "a" ], [ "b", [ 1, 5, 7 ] ] ]
    pid = ly.properties_id( pv )
    # does not work? @@@
    # pv = { 17: "a", "b": [ 1, 5, 7 ] }
    # pid2 = ly.properties_id( pv )
    # self.assertEqual( pid, pid2 )

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = pya.LayerInfo()
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    tr = pya.Trans()
    inst = c2.insert( pya.CellInstArray( c1.cell_index(), tr ) )
    self.assertEqual( inst.parent_cell.name, c2.name )
    self.assertEqual( inst.cell.name, c1.name )
    self.assertEqual( repr(inst.layout()), repr(ly) )

    s1 = c1.shapes( lindex ).insert( pya.Box( 10, -10, 50, 40 ), pid )
    s2 = c1.shapes( lindex ).insert( pya.Box( 10, -10, 50, 40 ) )
    self.assertEqual( repr(s1.property( 17 )), "\'a\'" )
    s1.set_property( 5, 23 ) 
    s1.delete_property( 17 )
    self.assertEqual( repr(s1.property( 17 )), "None" )
    self.assertEqual( str(s1.property( 5 )), "23" )
    self.assertEqual( repr(s2.property( 17 )), "None" )

    self.assertEqual( repr(inst.property( "a" )), "None" )
    inst.set_property( "a", 33 )
    self.assertEqual( str(inst.property( "a" )), "33" )
    inst.delete_property( "a" )
    self.assertEqual( repr(inst.property( "a" )), "None" )

    # cell properties
    self.assertEqual( repr(c1.property( 17 )), "None" )
    c1.prop_id = pid
    self.assertEqual( c1.prop_id, pid )
    self.assertEqual( repr(c1.property( 17 )), "\'a\'" )
    c1.set_property( 5, 23 ) 
    c1.delete_property( 17 )
    self.assertEqual( repr(c1.property( 17 )), "None" )
    self.assertEqual( str(c1.property( 5 )), "23" )


  # Instances and bboxes (editable mode)
  def test_6_Layout_new(self):

    ly = pya.Layout()
    if ly.is_editable():

      pv = { 17: "a", "b": [ 1, 5, 7 ] }
      pid1 = ly.properties_id( pv )
      pv = { 100: "x" }
      pid2 = ly.properties_id( pv )

      ci1 = ly.add_cell( "c1" )
      ci2 = ly.add_cell( "c2" )
      ci3 = ly.add_cell( "c3" )
      c1 = ly.cell( ci1 )
      c2 = ly.cell( ci2 )
      c3 = ly.cell( ci3 )

      tr = pya.Trans( pya.Trans.R90, pya.Point( 100, -50 ) ) 
      inst_1 = pya.CellInstArray( c1.cell_index(), tr )
      new_inst_1 = c2.insert( inst_1, pid1 )
      new_inst_2 = c2.insert( inst_1, pid2 )
      inst_2 = pya.CellInstArray( c3.cell_index(), tr*tr )
      new_inst_3 = c1.insert( inst_2 )

      self.assertEqual( new_inst_1.cell_index, c1.cell_index() )
      self.assertEqual( str(new_inst_1.trans), str(tr) )
      self.assertEqual( new_inst_1.prop_id, pid1 )
      self.assertEqual( new_inst_2.cell_index, c1.cell_index() )
      self.assertEqual( str(new_inst_2.trans), str(tr) )
      self.assertEqual( new_inst_2.prop_id, pid2 )
      self.assertEqual( new_inst_3.cell_index, c3.cell_index() )
      self.assertEqual( new_inst_3.prop_id, 0 )
      self.assertEqual( str(new_inst_3.trans), str(tr*tr) )

      new_inst_3 = c1.replace_prop_id( new_inst_3, pid2 )
    
      self.assertEqual( new_inst_1.cell_index, c1.cell_index() )
      self.assertEqual( str(new_inst_1.trans), str(tr) )
      self.assertEqual( new_inst_1.prop_id, pid1 )
      self.assertEqual( new_inst_2.cell_index, c1.cell_index() )
      self.assertEqual( str(new_inst_2.trans), str(tr) )
      self.assertEqual( new_inst_2.prop_id, pid2 )
      self.assertEqual( new_inst_3.cell_index, c3.cell_index() )
      self.assertEqual( new_inst_3.prop_id, pid2 )
      self.assertEqual( str(new_inst_3.trans), str(tr*tr) )

      try:
        new_inst_1 = c1.replace( new_inst_1, inst_2 )
        self.assertEqual( True, False )
      except:
        # OK: gives an error since we are trying to erase an object from a list that is does not belong to
        pass

      new_inst_1 = c2.replace( new_inst_1, inst_2 )
    
      self.assertEqual( new_inst_1.cell_index, c3.cell_index() )
      self.assertEqual( str(new_inst_1.trans), str(tr*tr) )
      self.assertEqual( new_inst_1.prop_id, pid1 )
      self.assertEqual( new_inst_2.cell_index, c1.cell_index() )
      self.assertEqual( str(new_inst_2.trans), str(tr) )
      self.assertEqual( new_inst_2.prop_id, pid2 )
      self.assertEqual( new_inst_3.cell_index, c3.cell_index() )
      self.assertEqual( new_inst_3.prop_id, pid2 )
      self.assertEqual( str(new_inst_3.trans), str(tr*tr) )

      new_inst_1 = c2.replace( new_inst_1, inst_2, pid1 )
    
      self.assertEqual( new_inst_1.cell_index, c3.cell_index() )
      self.assertEqual( str(new_inst_1.trans), str(tr*tr) )
      self.assertEqual( new_inst_1.prop_id, pid1 )
      self.assertEqual( new_inst_2.cell_index, c1.cell_index() )
      self.assertEqual( str(new_inst_2.trans), str(tr) )
      self.assertEqual( new_inst_2.prop_id, pid2 )
      self.assertEqual( new_inst_3.cell_index, c3.cell_index() )
      self.assertEqual( new_inst_3.prop_id, pid2 )
      self.assertEqual( str(new_inst_3.trans), str(tr*tr) )

      self.assertEqual( new_inst_1.is_null(), False )
      self.assertEqual( pya.Instance().is_null(), True )

      self.assertEqual( c2.is_leaf(), False )
      c2.erase( new_inst_1 )
      c2.erase( new_inst_2 )
      self.assertEqual( c2.is_leaf(), True )
      self.assertEqual( c2.child_instances(), 0 )

  def shapes_to_s(self, ly, shapes):
    r = ""
    for s in shapes.each():
      if r != "":
        r += ";"
      r += s.to_s()
      if s.prop_id > 0:
        pr = ""
        for pp in ly.properties(s.prop_id):
          if pr != "":
            pr += ","
          pr += str(pp[0]) + "=>" + str(pp[1])
        r += "["
        r += pr
        r += "]"
    return r

  # Copy/move between cells
  def test_7_cells_copy_move(self):

    ly1 = pya.Layout()

    # because of set_property ...
    if not ly1.is_editable():
      return

    la1 = ly1.insert_layer(pya.LayerInfo(1, 0))
    lb1 = ly1.insert_layer(pya.LayerInfo(2, 0))
    ly1.dbu = 0.001
    ca1 = ly1.cell(ly1.add_cell("a"))
    cb1 = ly1.cell(ly1.add_cell("b"))

    s1 = ca1.shapes(la1).insert(pya.Box(0, 500, 1000, 2000))
    s1.set_property(17, 5.0)
    s1.set_property(17, "hallo")
      
    ca1.copy(la1, lb1)
    cb1.copy(ca1, la1, lb1)
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(la1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(lb1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")
    self.assertEqual(self.shapes_to_s(ly1, cb1.shapes(lb1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")
    self.assertEqual(self.shapes_to_s(ly1, cb1.shapes(la1)), "")

    ly2 = pya.Layout()
    la2 = ly2.insert_layer(pya.LayerInfo(10, 0))
    lb2 = ly2.insert_layer(pya.LayerInfo(11, 0))
    ly2.dbu = 0.0005
    ca2 = ly2.cell(ly2.add_cell("a"))
    cb2 = ly2.cell(ly2.add_cell("b"))

    ca2.copy(ca1, la1, lb2)
    self.assertEqual(self.shapes_to_s(ly2, ca2.shapes(lb2)), "box (0,1000;2000,4000) prop_id=1[17=>hallo]")
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(la1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")

    # move
    ca1.clear()
    cb1.clear()
    ca2.clear()
    cb2.clear()
    
    s1 = ca1.shapes(la1).insert(pya.Box(0, 500, 1000, 2000))
    s1.set_property(17, 5.0)
    s1.set_property(17, "hallo")
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(la1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(lb1)), "")
    ca1.move(la1, lb1)
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(la1)), "")
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(lb1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")

    cb1.move(ca1, lb1, lb1)
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(la1)), "")
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(lb1)), "")
    self.assertEqual(self.shapes_to_s(ly1, cb1.shapes(lb1)), "box (0,500;1000,2000) prop_id=2[17=>hallo]")
    self.assertEqual(self.shapes_to_s(ly1, cb1.shapes(la1)), "")

    ly2 = pya.Layout()
    la2 = ly2.insert_layer(pya.LayerInfo(10, 0))
    lb2 = ly2.insert_layer(pya.LayerInfo(11, 0))
    ly2.dbu = 0.0005
    ca2 = ly2.cell(ly2.add_cell("a"))
    cb2 = ly2.cell(ly2.add_cell("b"))

    ca2.move(cb1, lb1, lb2)
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(la1)), "")
    self.assertEqual(self.shapes_to_s(ly1, ca1.shapes(lb1)), "")
    self.assertEqual(self.shapes_to_s(ly1, cb1.shapes(lb1)), "")
    self.assertEqual(self.shapes_to_s(ly1, cb1.shapes(la1)), "")
    self.assertEqual(self.shapes_to_s(ly2, ca2.shapes(lb2)), "box (0,1000;2000,4000) prop_id=1[17=>hallo]")
    
  # top cells
  def test_8(self):

    l = pya.Layout()
    tc = []
    for t in l.each_top_cell():
      tc.append(l.cell(t).name())
    self.assertEqual(",".join(tc), "")
    self.assertEqual(repr(l.top_cell()), "None")
    self.assertEqual(len(l.top_cells()), 0)

    c0 = l.create_cell("c0")
    self.assertEqual(c0.name, "c0")
    tc = []
    for t in l.each_top_cell():
      tc.append(l.cell(t).name) 
    self.assertEqual(",".join(tc), "c0")
    self.assertEqual(l.top_cell().name, "c0")
    lc = l.dup()
    self.assertEqual(lc._to_const_object().top_cell().name, "c0")  # const version
    tc = []
    for t in l.top_cells():
      tc.append(t.name) 
    self.assertEqual(",".join(tc), "c0")

    c1 = l.create_cell("c1")
    self.assertEqual(c1.name, "c1")
    tc = []
    for t in l.each_top_cell():
      tc.append(l.cell(t).name)
    self.assertEqual(",".join(tc), "c0,c1")
    error = False
    try:
      self.assertEqual(l.top_cell.inspect, "never-true")
    except:
      error = True
    self.assertEqual(error, True)
    tc = []
    for t in l.top_cells():
      tc.append(t.name) 
    self.assertEqual(",".join(tc), "c0,c1")
    tc = []
    lc = l.dup()
    for t in lc._to_const_object().top_cells(): # const version
      tc.append(t.name) 
    self.assertEqual(",".join(tc), "c0,c1")

    c2 = l.create_cell("c1")
    self.assertEqual(c2.name, "c1$1")

  # under construction and update
  def test_9(self):

    ly = pya.Layout()
    l1 = ly.insert_layer(pya.LayerInfo(1, 0))
    c1 = ly.create_cell("a")
    self.assertEqual(ly.under_construction(), False)

    s1 = c1.shapes(l1).insert(pya.Box(0, 500, 1000, 2000))
    self.assertEqual(c1.bbox().to_s(), "(0,500;1000,2000)")

    ly = pya.Layout()
    ly.start_changes()
    self.assertEqual(ly.under_construction(), True)
    ly.start_changes()
    self.assertEqual(ly.under_construction(), True)
    l1 = ly.insert_layer(pya.LayerInfo(1, 0))
    c1 = ly.create_cell("a")
    s1 = c1.shapes(l1).insert(pya.Box(0, 500, 1000, 2000))
    self.assertEqual(c1.bbox().to_s(), "()")
    # while under_construction, an explicit update is required to update the cell's bbox
    ly.update()
    self.assertEqual(c1.bbox().to_s(), "(0,500;1000,2000)")
    ly.end_changes()
    self.assertEqual(ly.under_construction(), True)
    ly.end_changes()
    self.assertEqual(ly.under_construction(), False)

  # Instance editing
  def test_10(self):

    ly = pya.Layout()

    ci1 = ly.add_cell("c1")
    ci2 = ly.add_cell("c2")
    ci3 = ly.add_cell("c3")

    c1 = ly.cell(ci1)
    c2 = ly.cell(ci2)
    c3 = ly.cell(ci3)

    tr = pya.Trans(pya.Trans.R90, pya.Point(100, -50)) 
    i1 = c1.insert(pya.CellInstArray(c2.cell_index(), tr))

    sv = []
    for i in c1.each_inst():
      sv.append(i.to_s(True))
    self.assertEqual(";".join(sv), "c2 r90 100,-50")

    sv = []
    for i in c2.each_inst():
      sv.append(i.to_s(True))
    self.assertEqual(";".join(sv), "")

    sv = []
    for i in c3.each_inst():
      sv.append(i.to_s(True))
    self.assertEqual(";".join(sv), "")

    if ly.is_editable():

      i1.cell_index = ci3

      sv = []
      for i in c1.each_inst():
        sv.append(i.to_s(True))
      self.assertEqual(";".join(sv), "c3 r90 100,-50")

      sv = []
      for i in c2.each_inst():
        sv.append(i.to_s(True))
      self.assertEqual(";".join(sv), "")

      sv = []
      for i in c3.each_inst():
        sv.append(i.to_s(True))
      self.assertEqual(";".join(sv), "")

      i1.cell = c2

      sv = []
      for i in c1.each_inst():
        sv.append(i.to_s(True))
      self.assertEqual(";".join(sv), "c2 r90 100,-50")

      sv = []
      for i in c2.each_inst():
        sv.append(i.to_s(True))
      self.assertEqual(";".join(sv), "")

      sv = []
      for i in c3.each_inst():
        sv.append(i.to_s(True))
      self.assertEqual(";".join(sv), "")

      self.assertEqual(i1.parent_cell.name, "c1")

      i1.cell = c1
      i1.parent_cell = c2

      sv = []
      for i in c1.each_inst():
        sv.append(i.to_s(True))
      self.assertEqual(";".join(sv), "")

      sv = []
      for i in c2.each_inst():
        sv.append(i.to_s(True))
      self.assertEqual(";".join(sv), "c1 r90 100,-50")

      sv = []
      for i in c3.each_inst():
        sv.append(i.to_s(True))
      self.assertEqual(";".join(sv), "")

  # User properties
  def test_11(self):

    ly = pya.Layout()

    self.assertEqual(ly.prop_id, 0)
    ly.prop_id = 1
    self.assertEqual(ly.prop_id, 1)
    ly.prop_id = 0
    self.assertEqual(ly.prop_id, 0)

    ly.set_property("x", 1)
    self.assertEqual(ly.prop_id, 1)
    self.assertEqual(ly.property("x"), 1)
    ly.set_property("x", 17)
    self.assertEqual(ly.prop_id, 2)
    self.assertEqual(ly.property("x"), 17)
    self.assertEqual(ly.property("y"), None)

    ly.delete_property("x")
    self.assertEqual(ly.property("x"), None)

  # Performance test
  def test_13(self):

    n = 100
    w = 10000

    ly = pya.Layout()
    l1 = ly.layer(1, 0)
    top = ly.create_cell("TOP")

    ix = 0
    while ix < n:
      sys.stdout.write(str(ix) + "/" + str(n) + "\n")
      sys.stdout.flush()
      iy = 0
      while iy < n:
        x = ix * w
        y = iy * w
        cell = ly.create_cell("X" + str(ix) + "Y" + str(iy))
        cell.shapes(l1).insert(pya.Box(0, 0, w, w))
        top.insert(pya.CellInstArray(cell.cell_index(), pya.Trans(pya.Point(ix * w, iy * w))))
        iy += 1
      ix += 1

    ly._destroy()

  # Bug #109
  def test_bug109(self):

    testtmp = os.getenv("TESTTMP_WITH_NAME", os.getenv("TESTTMP", "."))
    
    file_gds = os.path.join(testtmp, "bug109.gds")
    file_oas = os.path.join(testtmp, "bug109.oas")

    ly = pya.Layout()
    top = ly.create_cell("TOP")
    l1 = ly.layer(1, 0)
    shape = top.shapes(l1).insert(pya.Box(0, 10, 20, 30))
    shape.set_property(2, "hello, world")
    shape.set_property("42", "the answer")

    ly.write(file_gds)

    opt = pya.SaveLayoutOptions()
    opt.format = "OASIS"
    opt.oasis_strict_mode = False
    ly.write(file_oas, opt)

    ly2 = pya.Layout()
    ly2.read(file_gds)
    l2 = ly2.layer(1, 0)
    shape = None
    for s in ly2.top_cell().shapes(l2).each():
      shape = s
    self.assertEqual(shape.property(2), "hello, world")
    self.assertEqual(shape.property("2"), None)
    self.assertEqual(shape.property(2.0), "hello, world")
    self.assertEqual(shape.property(22), None)
    self.assertEqual(shape.property(42), "the answer")
    self.assertEqual(shape.property("42"), None)
    self.assertEqual(shape.property(42.0), "the answer")

    ly2 = pya.Layout()
    ly2.read(file_oas)
    l2 = ly2.layer(1, 0)
    shape = None
    for s in ly2.top_cell().shapes(l2).each():
      shape = s
    self.assertEqual(shape.property(2), "hello, world")
    self.assertEqual(shape.property("2"), None)
    self.assertEqual(shape.property(2.0), "hello, world")
    self.assertEqual(shape.property(22), None)
    self.assertEqual(shape.property("42"), "the answer")
    self.assertEqual(shape.property(42), None)
    self.assertEqual(shape.property(42.0), None)

  # Bug #1397
  def test_bug1397(self):

    testtmp = os.getenv("TESTTMP_WITH_NAME", os.getenv("TESTTMP", "."))
    tmp = os.path.join(testtmp, "tmp.gds")

    l = pya.Layout()

    c = l.create_cell("test_cell")

    li = pya.LayerInfo(1, 0)
    t = pya.Trans.R180
    c.add_meta_info(pya.LayoutMetaInfo("kfactory:li", li, None, True))
    c.add_meta_info(pya.LayoutMetaInfo("kfactory:t", t, None, True))

    l.write(tmp)

    l2 = pya.Layout()
    l2.read(tmp)

    c2 = l2.cell("test_cell")

    li = c2.meta_info("kfactory:li").value
    self.assertEqual(li.layer, 1)
    self.assertEqual(li.datatype, 0)

    t = c2.meta_info("kfactory:t").value
    self.assertEqual(str(t), "r180 0,0")
    

  def test_read_bytes(self):

    testtmp = os.getenv("TESTTMP_WITH_NAME", os.getenv("TESTTMP", "."))

    file_gds = os.path.join(testtmp, "bytes.gds")

    ly = pya.Layout()
    top = ly.create_cell("TOP")
    l1 = ly.layer(1, 0)
    shape = top.shapes(l1).insert(pya.Box(0, 10, 20, 30))
    ly.write(file_gds)

    with open(file_gds, "rb") as f:
      byte_buffer = f.read()

    ly2 = pya.Layout()
    ly2.read_bytes(byte_buffer, pya.LoadLayoutOptions())
    l2 = ly2.layer(1, 0)
    self.assertEqual(ly2.top_cell().bbox().to_s(), "(0,10;20,30)")

  def test_write_bytes(self):

    ly = pya.Layout()
    top = ly.create_cell("TOP")
    l1 = ly.layer(1, 0)
    shape = top.shapes(l1).insert(pya.Box(0, 10, 20, 30))
    options = pya.SaveLayoutOptions()
    options.format = "GDS2"
    byte_buffer = ly.write_bytes(options)

    ly2 = pya.Layout()
    ly2.read_bytes(byte_buffer)
    l2 = ly2.layer(1, 0)
    self.assertEqual(ly2.top_cell().bbox().to_s(), "(0,10;20,30)")


# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBLayoutTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)


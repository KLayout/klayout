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

class DBInstElement_TestClass < TestBase

  #  InstElement tests
  def test_1_InstElement 

    ly = RBA::Layout::new
    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )
    ci4 = ly.add_cell( "c4" )
    assert_equal( ci1, 0 )
    assert_equal( ci2, 1 )
    assert_equal( ci3, 2 )
    assert_equal( ci4, 3 )
    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )
    c4 = ly.cell( ci4 )
    pid = ly.properties_id( { 17 => "a", "b" => [ 1, 5, 7 ] }.to_a )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst1 = RBA::CellInstArray::new( 1, tr )
    c1.insert( inst1 )

    tr = RBA::CplxTrans::new( 1.5, 90.0, true, RBA::DPoint::new( 100, -50 ) ) 
    inst2 = RBA::CellInstArray::new( 2, tr )
    c1.insert( inst2, pid )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) )
    inst3 = RBA::CellInstArray::new( 3, tr, RBA::Point::new( 100, 0 ), RBA::Point::new( 0, 100 ), 10, 20 )
    c1.insert( inst3 )

    inst1r = nil
    inst2r = nil
    inst3r = nil
    c1.each_inst { |inst| inst1r = inst if inst.cell_index == 1; }
    c1.each_inst { |inst| inst2r = inst if inst.cell_index == 2; }
    c1.each_inst { |inst| inst3r = inst if inst.cell_index == 3; }
    assert_equal( inst1r.cell_index, 1 )
    assert_equal( inst2r.cell_index, 2 )
    assert_equal( inst3r.cell_index, 3 )
    assert_equal( inst1r == inst1r, true )
    assert_equal( inst1r < inst1r, false )
    assert_equal( inst1r == inst2r, false )
    assert_equal( (inst1r < inst2r ? 1 : 0) + (inst2r < inst1r ? 1 : 0), 1 )
    assert_equal( inst1r != inst1r, false )
    assert_equal( inst1r != inst2r, true )

    ie1 = RBA::InstElement::new( inst1r )
    ie2 = RBA::InstElement::new( inst2r )
    ie3 = RBA::InstElement::new( inst3r )

    assert_equal( ie1.cell_inst.cell_index, 1 )
    assert_equal( ie2.cell_inst.cell_index, 2 )
    assert_equal( ie3.cell_inst.cell_index, 3 )
    assert_equal( ie1 == ie1, true )
    assert_equal( ie1 == ie2, false )
    assert_equal( ie1 < ie1, false )
    assert_equal( (ie1 < ie2 ? 1 : 0) + (ie2 < ie1 ? 1 : 0), 1 )
    assert_equal( ie1 != ie1, false )
    assert_equal( ie1 != ie2, true )

    assert_equal( ie1.array_member_trans.to_s, "r0 0,0" )
    assert_equal( ie1.specific_trans.to_s, "r90 100,-50" )
    assert_equal( ie1.cell_inst.trans.to_s, "r90 100,-50" )
    assert_equal( ie1.specific_cplx_trans.to_s, "r90 *1 100,-50" )
    assert_equal( ie1.cell_inst.cplx_trans.to_s, "r90 *1 100,-50" )

    assert_equal( ie2.array_member_trans.to_s, "r0 0,0" )
    assert_equal( ie2.specific_trans.to_s, "m45 100,-50" )
    assert_equal( ie2.cell_inst.trans.to_s, "m45 100,-50" )
    assert_equal( ie2.specific_cplx_trans.to_s, "m45 *1.5 100,-50" )
    assert_equal( ie2.cell_inst.cplx_trans.to_s, "m45 *1.5 100,-50" )

    assert_equal( ie3.array_member_trans.to_s, "r0 0,0" )
    assert_equal( ie3.specific_trans.to_s, "r90 100,-50" )
    assert_equal( ie3.cell_inst.trans.to_s, "r90 100,-50" )
    assert_equal( ie3.specific_cplx_trans.to_s, "r90 *1 100,-50" )
    assert_equal( ie3.cell_inst.cplx_trans.to_s, "r90 *1 100,-50" )

    ie3 = RBA::InstElement::new( inst3r, 1, 5 )
    assert_equal( ie3.cell_inst.cell_index, 3 )

    assert_equal( ie3.array_member_trans.to_s, "r0 100,500" )
    assert_equal( ie3.specific_trans.to_s, "r90 200,450" )
    assert_equal( ie3.cell_inst.trans.to_s, "r90 100,-50" )
    assert_equal( ie3.specific_cplx_trans.to_s, "r90 *1 200,450" )
    assert_equal( ie3.cell_inst.cplx_trans.to_s, "r90 *1 100,-50" )

  end

end

load("test_epilogue.rb")

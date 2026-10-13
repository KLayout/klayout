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

def mapping_to_s(ly1, ly2, cm)
  r = ""
  ly2.each_cell_top_down do |c|
    s = ly2.cell(c).name
    if cm.has_mapping?(c)
      t = cm.cell_mapping(c)
      if t == RBA::CellMapping::DropCell
        s += "=>(0)"
      else
        s += "=>" + ly1.cell(t).name
      end
    end
    r == "" || (r += ";")
    r += s
  end
  r
end

def mapping_to_s_from_table(ly1, ly2, cm)
  table = cm.table
  r = ""
  ly2.each_cell_top_down do |c|
    s = ly2.cell(c).name
    if table[c]
      t = table[c]
      if t == RBA::CellMapping::DropCell
        s += "=>(0)"
      else
        s += "=>" + ly1.cell(t).name
      end
    end
    r == "" || (r += ";")
    r += s
  end
  r
end

class TestLib < RBA::Library

  def initialize  
  
    # set the description
    self.description = "Test lib for cell merging"
    
    sb_index = layout.add_cell("StaticBox")
    hier_index = layout.add_cell("HierBox")

    l1 = layout.insert_layer(RBA::LayerInfo::new(1, 0))

    sb_cell = layout.cell(sb_index)
    sb_cell.shapes(l1).insert(RBA::Box::new(0, 0, 100, 200))
    
    hier_cell = layout.cell(hier_index)
    hier_cell.insert(RBA::CellInstArray::new(sb_index, RBA::Trans::new(1, 2)))
    
    # register us with the name "CellMappingTestLib"
    register("CellMappingTestLib")
    
  end

end

class DBCellMapping_TestClass < TestBase

  def test_1

    mp = RBA::CellMapping::new
    mp.map(0, 1)
    assert_equal(mp.has_mapping?(0), true)
    assert_equal(mp.has_mapping?(1), false)
    assert_equal(mp.cell_mapping(0), 1)
    mp.clear
    assert_equal(mp.has_mapping?(0), false)
    assert_equal(mp.has_mapping?(1), false)
    mp.map(1, 2)
    assert_equal(mp.cell_mapping(1), 2)
    mp.map(1, 3)
    assert_equal(mp.cell_mapping(1), 3)

    ly = RBA::Layout::new

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )
    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst_1 = RBA::CellInstArray::new( c1.cell_index, tr )
    new_inst_1 = c2.insert( inst_1 )
    new_inst_2 = c2.insert( inst_1 )
    inst_2 = RBA::CellInstArray::new( c3.cell_index, tr*tr )
    new_inst_3 = c1.insert( inst_2 )

    top1 = ci2

    ly1 = ly

    ly = RBA::Layout::new

    ci0 = ly.add_cell( "c0" )
    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )
    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst_1 = RBA::CellInstArray::new( c1.cell_index, tr )
    new_inst_1 = c2.insert( inst_1 )
    new_inst_2 = c2.insert( inst_1 )
    inst_2 = RBA::CellInstArray::new( c3.cell_index, tr*tr )
    new_inst_3 = c1.insert( inst_2 )

    top2 = ci2

    ly2 = ly

    mp = RBA::CellMapping::new
    mp.from_names(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly1, ly2, mp), "c0;c2=>c2;c1=>c1;c3=>c3")
    assert_equal(mapping_to_s_from_table(ly1, ly2, mp), "c0;c2=>c2;c1=>c1;c3=>c3")

    mp = RBA::CellMapping::new
    mp.from_names(ly1.cell(top1), ly2.cell(top2))
    assert_equal(mapping_to_s(ly1, ly2, mp), "c0;c2=>c2;c1=>c1;c3=>c3")
    assert_equal(mapping_to_s_from_table(ly1, ly2, mp), "c0;c2=>c2;c1=>c1;c3=>c3")

    mp = RBA::CellMapping::new
    mp.from_geometry(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly1, ly2, mp), "c0;c2=>c2;c1=>c1;c3=>c3")

    mp = RBA::CellMapping::new
    mp.from_geometry(ly1.cell(top1), ly2.cell(top2))
    assert_equal(mapping_to_s(ly1, ly2, mp), "c0;c2=>c2;c1=>c1;c3=>c3")

    ly = RBA::Layout::new

    ci0 = ly.add_cell( "c0" )
    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "cx" )
    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst_1 = RBA::CellInstArray::new( c1.cell_index, tr )
    new_inst_1 = c2.insert( inst_1 )
    new_inst_2 = c2.insert( inst_1 )
    inst_2 = RBA::CellInstArray::new( c3.cell_index, tr*tr )
    new_inst_3 = c1.insert( inst_2 )

    top2 = ci2

    ly2 = ly

    mp = RBA::CellMapping::new
    mp.from_names(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly1, ly2, mp), "c0;c2=>c2;c1=>c1;cx")

    ly1dup = ly1.dup
    mp = RBA::CellMapping::new
    nc = mp.from_names_full(ly1dup, top1, ly2, top2)
    assert_equal(mapping_to_s(ly1dup, ly2, mp), "c0;c2=>c2;c1=>c1;cx=>cx")
    assert_equal(nc.inspect, "[3]")

    ly1dup = ly1.dup
    mp = RBA::CellMapping::new
    nc = mp.from_names_full(ly1dup.cell(top1), ly2.cell(top2))
    assert_equal(mapping_to_s(ly1dup, ly2, mp), "c0;c2=>c2;c1=>c1;cx=>cx")
    assert_equal(nc.inspect, "[3]")

    mp = RBA::CellMapping::new
    mp.from_geometry(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly1, ly2, mp), "c0;c2=>c2;c1=>c1;cx=>c3")

    ly = RBA::Layout::new

    ci0 = ly.add_cell( "c0" )
    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )
    ci3 = ly.add_cell( "c3" )
    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )
    c3 = ly.cell( ci3 )

    tr = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 100, -50 ) ) 
    inst_1 = RBA::CellInstArray::new( c1.cell_index, tr )
    new_inst_1 = c2.insert( inst_1 )
    new_inst_2 = c2.insert( inst_1 )
    inst_2 = RBA::CellInstArray::new( c3.cell_index, tr )
    new_inst_3 = c1.insert( inst_2 )

    top2 = ci2

    ly2 = ly

    mp = RBA::CellMapping::new
    mp.from_names(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly1, ly2, mp), "c0;c2=>c2;c1=>c1;c3=>c3")

    mp = RBA::CellMapping::new
    mp.from_geometry(ly1, top1, ly2, top2)
    assert_equal(mapping_to_s(ly1, ly2, mp), "c0;c2=>c2;c1=>c1;c3")

    ly1dup = ly1.dup
    mp = RBA::CellMapping::new
    nc = mp.from_geometry_full(ly1dup, top1, ly2, top2)
    assert_equal(mapping_to_s(ly1dup, ly2, mp), "c0;c2=>c2;c1=>c1;c3=>c3$1")
    assert_equal(nc.inspect, "[3]")

    ly1dup = ly1.dup
    mp = RBA::CellMapping::new
    nc = mp.from_geometry_full(ly1dup.cell(top1), ly2.cell(top2))
    assert_equal(mapping_to_s(ly1dup, ly2, mp), "c0;c2=>c2;c1=>c1;c3=>c3$1")
    assert_equal(nc.inspect, "[3]")

    mp.clear
    mp.from_geometry(ly1, top1, ly2, top2)
    mp.map(ci2, RBA::CellMapping::DropCell)
    assert_equal(mapping_to_s(ly1, ly2, mp), "c0;c2=>(0);c1=>c1;c3")
    assert_equal(mapping_to_s_from_table(ly1, ly2, mp), "c0;c2=>(0);c1=>c1;c3")

  end

  def test_2

    ly = RBA::Layout::new

    a0 = ly.create_cell("a0")
    a1 = ly.create_cell("a1")
    a2 = ly.create_cell("a2")
    a3 = ly.create_cell("a3")
    a4 = ly.create_cell("a4")
    a5 = ly.create_cell("a5")
    
    a3.insert(RBA::CellInstArray::new(a4.cell_index, RBA::Trans::new))
    a3.insert(RBA::CellInstArray::new(a5.cell_index, RBA::Trans::new))

    a1.insert(RBA::CellInstArray::new(a4.cell_index, RBA::Trans::new))
    a1.insert(RBA::CellInstArray::new(a3.cell_index, RBA::Trans::new))
    a2.insert(RBA::CellInstArray::new(a4.cell_index, RBA::Trans::new))

    ly_target = RBA::Layout::new
    b0 = ly_target.create_cell("b0")

    cm = RBA::CellMapping::new
    cm.for_single_cell(ly_target, b0.cell_index, ly, a1.cell_index)

    assert_equal(mapping_to_s(ly_target, ly, cm), "a0;a1=>b0;a2;a3;a4;a5")

    cm = RBA::CellMapping::new
    cm.for_single_cell(b0, a1)

    assert_equal(mapping_to_s(ly_target, ly, cm), "a0;a1=>b0;a2;a3;a4;a5")

    lytdup = ly_target.dup
    cm = RBA::CellMapping::new
    nc = cm.for_single_cell_full(lytdup, b0.cell_index, ly, a1.cell_index)
    assert_equal(nc.inspect, "[1, 2, 3]")

    assert_equal(mapping_to_s(lytdup, ly, cm), "a0;a1=>b0;a2;a3=>a3;a4=>a4;a5=>a5")

    lytdup = ly_target.dup
    cm = RBA::CellMapping::new
    nc = cm.for_single_cell_full(lytdup.cell(b0.cell_index), a1)
    assert_equal(nc.inspect, "[1, 2, 3]")

    assert_equal(mapping_to_s(lytdup, ly, cm), "a0;a1=>b0;a2;a3=>a3;a4=>a4;a5=>a5")

  end

  def test_3

    ly = RBA::Layout::new

    a0 = ly.create_cell("a0")
    a1 = ly.create_cell("a1")
    a2 = ly.create_cell("a2")
    a3 = ly.create_cell("a3")
    a4 = ly.create_cell("a4")
    a5 = ly.create_cell("a5")
    
    a3.insert(RBA::CellInstArray::new(a4.cell_index, RBA::Trans::new))
    a3.insert(RBA::CellInstArray::new(a5.cell_index, RBA::Trans::new))

    a1.insert(RBA::CellInstArray::new(a4.cell_index, RBA::Trans::new))
    a1.insert(RBA::CellInstArray::new(a3.cell_index, RBA::Trans::new))
    a2.insert(RBA::CellInstArray::new(a4.cell_index, RBA::Trans::new))

    ly_target = RBA::Layout::new
    b0 = ly_target.create_cell("b0")
    b1 = ly_target.create_cell("b1")
    b2 = ly_target.create_cell("b2")

    cm = RBA::CellMapping::new
    cm.for_multi_cells(ly_target, [ b0, b1, b2 ].collect { |c| c.cell_index }, ly, [ a0, a1, a2 ].collect { |c| c.cell_index })

    assert_equal(mapping_to_s(ly_target, ly, cm), "a0=>b0;a1=>b1;a2=>b2;a3;a4;a5")

    cm = RBA::CellMapping::new
    cm.for_multi_cells([ b0, b1, b2 ], [ a0, a1, a2 ])

    assert_equal(mapping_to_s(ly_target, ly, cm), "a0=>b0;a1=>b1;a2=>b2;a3;a4;a5")

    lytdup = ly_target.dup
    cm = RBA::CellMapping::new
    nc = cm.for_multi_cells_full(lytdup, [ b0, b1, b2 ].collect { |c| c.cell_index }, ly, [ a0, a1, a2 ].collect { |c| c.cell_index })
    assert_equal(nc.inspect, "[3, 4, 5]")

    assert_equal(mapping_to_s(lytdup, ly, cm), "a0=>b0;a1=>b1;a2=>b2;a3=>a3;a4=>a4;a5=>a5")

    lytdup = ly_target.dup
    cm = RBA::CellMapping::new
    cm.for_multi_cells_full([ b0, b1, b2 ].collect { |c| lytdup.cell(c.cell_index) }, [ a0, a1, a2 ])
    assert_equal(nc.inspect, "[3, 4, 5]")

    assert_equal(mapping_to_s(lytdup, ly, cm), "a0=>b0;a1=>b1;a2=>b2;a3=>a3;a4=>a4;a5=>a5")

  end

  # Mapping while skipping library references
  def test_4

    tl = TestLib::new

    a = RBA::Layout::new
    l2 = a.layer(2, 0)

    a0 = a.create_cell("a0")
    a0.shapes(l2).insert(RBA::Box::new(0, 0, 100, 200))

    a1 = a.create_cell("HierBox", tl.name)
    a0.insert(RBA::CellInstArray::new(a1, RBA::Trans::new))

    a2 = a.create_cell("a2")
    a2.shapes(l2).insert(RBA::Box::new(10, 20, 110, 120))
    a0.insert(RBA::CellInstArray::new(a2, RBA::Trans::new(0, 1000)))

    a3 = a.create_cell("a3")

    l1 = a.layer(1, 0)

    assert_equal(RBA::Region::new(a0.begin_shapes_rec(l1)).to_s, "(1,2;1,202;101,202;101,2)")
    assert_equal(RBA::Region::new(a0.begin_shapes_rec(l2)).to_s, "(0,0;0,200;100,200;100,0);(10,1020;10,1120;110,1120;110,1020)")

    b = RBA::Layout::new
    l3 = b.layer(3, 0)
    
    b0 = b.create_cell("b0")
    b0.shapes(l3).insert(RBA::Box::new(0, 0, 200, 100))

    b1 = b.create_cell("HierBox", tl.name)

    b0.insert(RBA::CellInstArray::new(b1, RBA::Trans::new(2, 1)))

    l1 = b.layer(1, 0)

    assert_equal(RBA::Region::new(b0.begin_shapes_rec(l1)).to_s, "(3,3;3,203;103,203;103,3)")
    assert_equal(RBA::Region::new(b0.begin_shapes_rec(l3)).to_s, "(0,0;0,100;200,100;200,0)")

    # Testing mapping of b0 to a0

    adup = a.dup
    bdup = b.dup

    cm = RBA::CellMapping::new
    nc = cm.for_single_cell_full(adup, a0.cell_index, bdup, b0.cell_index)
    assert_equal(nc.inspect, "[5, 6]")

    # This mapping creates new cells for HierBox and StaticBox as the identical library cell is not recognized
    assert_equal(mapping_to_s(adup, bdup, cm), "b0=>a0;HierBox=>HierBox$1;StaticBox=>StaticBox$1")

    adup.copy_tree_shapes(bdup, cm)

    a0dup = adup.cell(a0.cell_index)
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(1, 0))).to_s, "(1,2;1,202;101,202;101,2);(3,3;3,203;103,203;103,3)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(2, 0))).to_s, "(0,0;0,200;100,200;100,0);(10,1020;10,1120;110,1120;110,1020)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(3, 0))).to_s, "(0,0;0,100;200,100;200,0)")

    assert_equal(adup.each_cell.collect { |c| c.qname }.sort.join(","), "CellMappingTestLib.HierBox,CellMappingTestLib.StaticBox,HierBox$1,StaticBox$1,a0,a2,a3")

    # Testing mapping with library cell preservation 

    adup = a.dup
    bdup = b.dup

    cm = RBA::CellMapping::new
    nc = cm.for_single_cell_full(adup, a0.cell_index, bdup, b0.cell_index, true)
    assert_equal(nc.inspect, "[]")

    # This mapping creates new cells for HierBox and StaticBox as the identical library cell is not recognized
    assert_equal(mapping_to_s(adup, bdup, cm), "b0=>a0;HierBox=>(0);StaticBox=>(0)")

    adup.copy_tree_shapes(bdup, cm)

    a0dup = adup.cell(a0.cell_index)
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(1, 0))).to_s, "(1,2;1,202;101,202;101,2);(3,3;3,203;103,203;103,3)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(2, 0))).to_s, "(0,0;0,200;100,200;100,0);(10,1020;10,1120;110,1120;110,1020)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(3, 0))).to_s, "(0,0;0,100;200,100;200,0)")

    assert_equal(adup.each_cell.collect { |c| c.qname }.sort.join(","), "CellMappingTestLib.HierBox,CellMappingTestLib.StaticBox,a0,a2,a3")

    # Testing mapping with library cell preservation in cell object mode

    adup = a.dup
    bdup = b.dup

    cm = RBA::CellMapping::new
    nc = cm.for_single_cell_full(adup.cell(a0.cell_index), bdup.cell(b0.cell_index), true)
    assert_equal(nc.inspect, "[]")

    # This mapping creates new cells for HierBox and StaticBox as the identical library cell is not recognized
    assert_equal(mapping_to_s(adup, bdup, cm), "b0=>a0;HierBox=>(0);StaticBox=>(0)")

    adup.copy_tree_shapes(bdup, cm)

    a0dup = adup.cell(a0.cell_index)
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(1, 0))).to_s, "(1,2;1,202;101,202;101,2);(3,3;3,203;103,203;103,3)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(2, 0))).to_s, "(0,0;0,200;100,200;100,0);(10,1020;10,1120;110,1120;110,1020)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(3, 0))).to_s, "(0,0;0,100;200,100;200,0)")

    assert_equal(adup.each_cell.collect { |c| c.qname }.sort.join(","), "CellMappingTestLib.HierBox,CellMappingTestLib.StaticBox,a0,a2,a3")

    # Testing mapping with library cell preservation and name-based mapper

    adup = a.dup
    bdup = b.dup

    # makes the names match ...
    adup.cell(a0.cell_index).name = "b0"

    cm = RBA::CellMapping::new
    nc = cm.from_names_full(adup, a0.cell_index, bdup, b0.cell_index, true)
    assert_equal(nc.inspect, "[]")

    # This mapping creates new cells for HierBox and StaticBox as the identical library cell is not recognized
    assert_equal(mapping_to_s(adup, bdup, cm), "b0=>b0;HierBox=>(0);StaticBox=>(0)")

    adup.copy_tree_shapes(bdup, cm)

    a0dup = adup.cell(a0.cell_index)
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(1, 0))).to_s, "(1,2;1,202;101,202;101,2);(3,3;3,203;103,203;103,3)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(2, 0))).to_s, "(0,0;0,200;100,200;100,0);(10,1020;10,1120;110,1120;110,1020)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(3, 0))).to_s, "(0,0;0,100;200,100;200,0)")

    assert_equal(adup.each_cell.collect { |c| c.qname }.sort.join(","), "CellMappingTestLib.HierBox,CellMappingTestLib.StaticBox,a2,a3,b0")

    # Testing mapping with library cell preservation and name-based mapper in cell object mode

    adup = a.dup
    bdup = b.dup

    # makes the names match ...
    adup.cell(a0.cell_index).name = "b0"

    cm = RBA::CellMapping::new
    nc = cm.from_names_full(adup.cell(a0.cell_index), bdup.cell(b0.cell_index), true)
    assert_equal(nc.inspect, "[]")

    # This mapping creates new cells for HierBox and StaticBox as the identical library cell is not recognized
    assert_equal(mapping_to_s(adup, bdup, cm), "b0=>b0;HierBox=>(0);StaticBox=>(0)")

    adup.copy_tree_shapes(bdup, cm)

    a0dup = adup.cell(a0.cell_index)
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(1, 0))).to_s, "(1,2;1,202;101,202;101,2);(3,3;3,203;103,203;103,3)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(2, 0))).to_s, "(0,0;0,200;100,200;100,0);(10,1020;10,1120;110,1120;110,1020)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(3, 0))).to_s, "(0,0;0,100;200,100;200,0)")

    assert_equal(adup.each_cell.collect { |c| c.qname }.sort.join(","), "CellMappingTestLib.HierBox,CellMappingTestLib.StaticBox,a2,a3,b0")

    # Testing mapping with library cell preservation and geometry-based mapper

    adup = a.dup
    bdup = b.dup

    cm = RBA::CellMapping::new
    nc = cm.from_geometry_full(adup, a0.cell_index, bdup, b0.cell_index, true)
    assert_equal(nc.inspect, "[]")

    # This mapping creates new cells for HierBox and StaticBox as the identical library cell is not recognized
    assert_equal(mapping_to_s(adup, bdup, cm), "b0=>a0;HierBox=>(0);StaticBox=>(0)")

    adup.copy_tree_shapes(bdup, cm)

    a0dup = adup.cell(a0.cell_index)
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(1, 0))).to_s, "(1,2;1,202;101,202;101,2);(3,3;3,203;103,203;103,3)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(2, 0))).to_s, "(0,0;0,200;100,200;100,0);(10,1020;10,1120;110,1120;110,1020)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(3, 0))).to_s, "(0,0;0,100;200,100;200,0)")

    assert_equal(adup.each_cell.collect { |c| c.qname }.sort.join(","), "CellMappingTestLib.HierBox,CellMappingTestLib.StaticBox,a0,a2,a3")

    # Testing mapping with library cell preservation and geometry-based mapper in cell object mode

    adup = a.dup
    bdup = b.dup

    cm = RBA::CellMapping::new
    nc = cm.from_geometry_full(adup.cell(a0.cell_index), bdup.cell(b0.cell_index), true)
    assert_equal(nc.inspect, "[]")

    # This mapping creates new cells for HierBox and StaticBox as the identical library cell is not recognized
    assert_equal(mapping_to_s(adup, bdup, cm), "b0=>a0;HierBox=>(0);StaticBox=>(0)")

    adup.copy_tree_shapes(bdup, cm)

    a0dup = adup.cell(a0.cell_index)
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(1, 0))).to_s, "(1,2;1,202;101,202;101,2);(3,3;3,203;103,203;103,3)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(2, 0))).to_s, "(0,0;0,200;100,200;100,0);(10,1020;10,1120;110,1120;110,1020)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(3, 0))).to_s, "(0,0;0,100;200,100;200,0)")

    assert_equal(adup.each_cell.collect { |c| c.qname }.sort.join(","), "CellMappingTestLib.HierBox,CellMappingTestLib.StaticBox,a0,a2,a3")

    # Testing mapping with library cell preservation and multi-cell mapper

    adup = a.dup
    bdup = b.dup

    cm = RBA::CellMapping::new
    nc = cm.for_multi_cells_full(adup, [ a0.cell_index ], bdup, [ b0.cell_index ], true)
    assert_equal(nc.inspect, "[]")

    # This mapping creates new cells for HierBox and StaticBox as the identical library cell is not recognized
    assert_equal(mapping_to_s(adup, bdup, cm), "b0=>a0;HierBox=>(0);StaticBox=>(0)")

    adup.copy_tree_shapes(bdup, cm)

    a0dup = adup.cell(a0.cell_index)
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(1, 0))).to_s, "(1,2;1,202;101,202;101,2);(3,3;3,203;103,203;103,3)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(2, 0))).to_s, "(0,0;0,200;100,200;100,0);(10,1020;10,1120;110,1120;110,1020)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(3, 0))).to_s, "(0,0;0,100;200,100;200,0)")

    assert_equal(adup.each_cell.collect { |c| c.qname }.sort.join(","), "CellMappingTestLib.HierBox,CellMappingTestLib.StaticBox,a0,a2,a3")

    # Testing mapping with library cell preservation and multi-cell mapper in cell-object mode

    adup = a.dup
    bdup = b.dup

    cm = RBA::CellMapping::new
    nc = cm.for_multi_cells_full([ adup.cell(a0.cell_index) ], [ bdup.cell(b0.cell_index) ], true)
    assert_equal(nc.inspect, "[]")

    # This mapping creates new cells for HierBox and StaticBox as the identical library cell is not recognized
    assert_equal(mapping_to_s(adup, bdup, cm), "b0=>a0;HierBox=>(0);StaticBox=>(0)")

    adup.copy_tree_shapes(bdup, cm)

    a0dup = adup.cell(a0.cell_index)
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(1, 0))).to_s, "(1,2;1,202;101,202;101,2);(3,3;3,203;103,203;103,3)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(2, 0))).to_s, "(0,0;0,200;100,200;100,0);(10,1020;10,1120;110,1120;110,1020)")
    assert_equal(RBA::Region::new(a0dup.begin_shapes_rec(adup.layer(3, 0))).to_s, "(0,0;0,100;200,100;200,0)")

    assert_equal(adup.each_cell.collect { |c| c.qname }.sort.join(","), "CellMappingTestLib.HierBox,CellMappingTestLib.StaticBox,a0,a2,a3")

    # cleanup 

    tl._destroy

  end

end

load("test_epilogue.rb")

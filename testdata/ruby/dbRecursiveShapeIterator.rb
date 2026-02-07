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

class DBRecursiveShapeIterator_TestClass < TestBase

  def collect(s, l)

    # check native iteration here too ..
    res2 = []
    s.each do |s|
      r = "[#{l.cell_name(s.cell_index)}]"
      if s.shape.is_box?
        box = s.shape.box
        r += box.transformed(s.trans).to_s
      else 
        r += "X";
      end
      res2.push(r)
    end

    res = []
    s.reset
    while !s.at_end?
      r = "[#{l.cell_name(s.cell_index)}]"
      if s.shape.is_box?
        box = s.shape.box
        r += box.transformed(s.trans).to_s
      else 
        r += "X";
      end
      s.next
      res.push(r)
    end

    assert_equal(res, res2)

    return res.join("/")

  end

  def dcollect(s, l)

    res = []
    while !s.at_end?
      r = "[#{l.cell_name(s.cell_index)}]"
      if s.shape.is_box?
        box = s.shape.dbox
        r += box.transformed(s.dtrans).to_s
      else 
        r += "X";
      end
      s.next
      res.push(r)
    end

    return res.join("/")

  end

  def acollect(s, l)

    res = []
    while !s.at_end?
      r = "[#{l.cell_name(s.cell_index)}]"
      if s.shape.is_box?
        box = s.shape.box
        r += box.transformed(s.always_apply_trans).to_s
      else 
        r += "X";
      end
      s.next
      res.push(r)
    end

    return res.join("/")

  end

  def test_1

    # Recursive shape iterator tests
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    bb = RBA::Box.new(1, 101, 1001, 1201)
    c3.shapes(1).insert(bb)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    i1 = l.begin_shapes_touching(c0.cell_index, 0, RBA::Box.new(0, 0, 100, 100))
    i1copy = i1.dup
    assert_equal(i1copy.overlapping?, false)
    assert_equal(collect(i1, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)")
    i1.reset
    assert_equal(dcollect(i1, l), "[c0](0,0.1;1,1.2)/[c1](0,0.1;1,1.2)/[c2](0.1,0;1.1,1.1)")
    assert_equal(collect(i1copy, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)")

    i1 = l.begin_shapes_touching(c0.cell_index, 0, RBA::DBox.new(0, 0, 0.100, 0.100))
    assert_equal(collect(i1, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)")

    i1 = c0.begin_shapes_rec_touching(0, RBA::Box.new(0, 0, 100, 100))
    assert_equal(collect(i1, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)")

    i1 = c0.begin_shapes_rec_touching(0, RBA::DBox.new(0, 0, 0.100, 0.100))
    assert_equal(collect(i1, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)")

    i1o = l.begin_shapes_overlapping(c0.cell_index, 0, RBA::Box.new(0, 0, 100, 100));
    assert_equal(collect(i1o, l), "");
    i1o = l.begin_shapes_overlapping(c0.cell_index, 0, RBA::DBox.new(0, 0, 0.100, 0.100));
    assert_equal(collect(i1o, l), "");
    i1copy.overlapping = true
    assert_equal(i1copy.overlapping?, true)
    assert_equal(collect(i1copy, l), "");
    i1o = l.begin_shapes_overlapping(c0.cell_index, 0, RBA::Box.new(0, 0, 100, 101));
    assert_equal(collect(i1o, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)");
    i1o = l.begin_shapes_overlapping(c0.cell_index, 0, RBA::DBox.new(0, 0, 0.100, 0.101));
    assert_equal(collect(i1o, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)");
    i1copy.region = RBA::Box.new(0, 0, 100, 101)
    assert_equal(i1copy.region.to_s, "(0,0;100,101)")
    assert_equal(collect(i1copy, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)");
    i1copy.region = RBA::Region::new(RBA::Box.new(0, 0, 100, 101))
    assert_equal(i1copy.region.to_s, "(0,0;100,101)")
    assert_equal(collect(i1copy, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)");
    i1copy.region = RBA::Box.new(-1000, -1000, 1100, 1101)
    i1copy.confine_region(RBA::Box.new(0, 0, 100, 101))
    assert_equal(i1copy.region.to_s, "(0,0;100,101)")
    assert_equal(collect(i1copy, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)");
    i1copy.region = RBA::Box.new(-1000, -1000, 1100, 1101)
    i1copy.confine_region(RBA::Region::new(RBA::Box.new(0, 0, 100, 101)))
    assert_equal(i1copy.region.to_s, "(0,0;100,101)")
    assert_equal(collect(i1copy, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)");
    i1o = l.begin_shapes_overlapping(c0.cell_index, 0, RBA::Box.new(0, 0, 101, 101));
    assert_equal(collect(i1o, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)");
    i1o = c0.begin_shapes_rec_overlapping(0, RBA::Box.new(0, 0, 101, 101));
    assert_equal(collect(i1o, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)");

    i2 = l.begin_shapes_touching(c0.cell_index, 0, RBA::Box.new(-100, 0, 100, 100));
    assert_equal(collect(i2, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](-1200,0;-100,1000)");
    i2o = l.begin_shapes_overlapping(c0.cell_index, 0, RBA::Box.new(-100, 0, 100, 100));
    assert_equal(collect(i2o, l), "");
    i2o = l.begin_shapes_overlapping(c0.cell_index, 0, RBA::Box.new(-101, 0, 101, 101));
    assert_equal(collect(i2o, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](-1200,0;-100,1000)");

    i4 = l.begin_shapes_touching(c0.cell_index, 0, RBA::Box.new(-100, 0, 2000, 100));
    i4_copy = l.begin_shapes_touching(c0.cell_index, 0, RBA::Box.new(-100, 0, 2000, 100));
    i4.max_depth = 0;
    assert_equal(collect(i4, l), "[c0](0,100;1000,1200)");

    assert_equal(i4 == i4, true);
    assert_equal(i4 != i4, false);
    assert_equal(i4 == i4_copy, false);
    assert_equal(i4 != i4_copy, true);
    i4 = i4_copy.dup;
    assert_equal(i4 == i4_copy, true);
    assert_equal(i4 != i4_copy, false);
    i4.max_depth = 1;
    assert_equal(collect(i4, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](-1200,0;-100,1000)");

    i4.assign(i4_copy);
    assert_equal(collect(i4, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)");

    i5 = l.begin_shapes(c0.cell_index, 0);
    assert_equal(collect(i5, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)");

    ii = RBA::RecursiveShapeIterator::new
    assert_equal(collect(ii, l), "")

    ii = RBA::RecursiveShapeIterator::new(l, c1, 0)
    assert_equal(collect(ii, l), "[c1](0,100;1000,1200)")
    assert_equal(ii.top_cell.name, "c1")
    assert_equal(ii.layout == l, true)

    ii.max_depth = 2
    assert_equal(ii.max_depth, 2)
    ii.min_depth = 1
    assert_equal(ii.min_depth, 1)
    
    ii = RBA::RecursiveShapeIterator::new(l, c2, [0, 1])
    ic = ii.dup
    assert_equal(collect(ii, l), "[c2](0,100;1000,1200)/[c3](1100,100;2100,1200)/[c3](1101,101;2101,1201)")
    assert_equal(collect(ic, l), "[c2](0,100;1000,1200)/[c3](1100,100;2100,1200)/[c3](1101,101;2101,1201)")
    
    ii = RBA::RecursiveShapeIterator::new(l, c2, 0, RBA::Box.new(-100, 0, 2000, 100), false)
    assert_equal(collect(ii, l), "[c2](0,100;1000,1200)/[c3](1100,100;2100,1200)")
    
    ii = RBA::RecursiveShapeIterator::new(l, c2, 0, RBA::Box.new(-100, 0, 2000, 100), true)
    assert_equal(collect(ii, l), "")
    
    ii = RBA::RecursiveShapeIterator::new(l, c2, 0, RBA::Box.new(-100, 0, 2000, 101), true)
    assert_equal(collect(ii, l), "[c2](0,100;1000,1200)/[c3](1100,100;2100,1200)")

    ii = RBA::RecursiveShapeIterator::new(l, c2, [0, 1], RBA::Box.new(-100, 0, 2000, 100), false)
    assert_equal(collect(ii, l), "[c2](0,100;1000,1200)/[c3](1100,100;2100,1200)")
    
    ii = RBA::RecursiveShapeIterator::new(l, c2, [0, 1], RBA::Box.new(-100, 0, 2000, 101), false)
    assert_equal(collect(ii, l), "[c2](0,100;1000,1200)/[c3](1100,100;2100,1200)/[c3](1101,101;2101,1201)")
    ii.reset
    assert_equal(acollect(ii, l), "[c2](0,100;1000,1200)/[c3](0,100;1000,1200)/[c3](1,101;1001,1201)")
    
    ii = RBA::RecursiveShapeIterator::new(l, c2, [0, 1], RBA::Box.new(-100, 20, 2000, 121), true)
    ii.global_trans = RBA::ICplxTrans::new(RBA::Vector::new(10, 20))
    assert_equal(ii.global_trans.to_s, "r0 *1 10,20")
    assert_equal(collect(ii, l), "[c2](10,120;1010,1220)/[c3](1110,120;2110,1220)")
    ii.global_dtrans = RBA::DCplxTrans::new(RBA::DVector::new(0.01, 0.02))
    ii.reset
    assert_equal(ii.global_dtrans.to_s, "r0 *1 0.01,0.02")
    assert_equal(collect(ii, l), "[c2](10,120;1010,1220)/[c3](1110,120;2110,1220)")
    ii.reset
    assert_equal(acollect(ii, l), "[c2](10,120;1010,1220)/[c3](0,100;1000,1200)")
    
    ii = RBA::RecursiveShapeIterator::new(l, c2, [0, 1], RBA::Box.new(-100, 0, 2000, 101), true)
    assert_equal(collect(ii, l), "[c2](0,100;1000,1200)/[c3](1100,100;2100,1200)")
    
    ii = RBA::RecursiveShapeIterator::new(l, c0, 0)
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")

    ii.reset
    ii.unselect_cells("c0")
    ii.select_cells("c2")
    assert_equal(collect(ii, l), "[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)")
    
    ii.reset_selection
    ii.unselect_cells("c*")
    ii.select_cells([ c2.cell_index ])
    assert_equal(collect(ii, l), "[c2](100,0;1100,1100)")
    
    ii.reset_selection
    ii.unselect_all_cells
    ii.select_cells("c2")
    assert_equal(collect(ii, l), "[c2](100,0;1100,1100)")
    
    ii.reset_selection
    ii.select_all_cells
    ii.unselect_cells("c2")
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")
    
    ii.reset_selection
    ii.select_cells("c*")
    ii.unselect_cells([ c1.cell_index, c2.cell_index ])
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")
    
  end

  def test_2

    # Recursive shape iterator tests
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c3.shapes(0).insert(b)

    bb = RBA::Box.new(1, 101, 1001, 1201)
    c3.shapes(1).insert(bb)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1), RBA::Vector::new(10, 20), RBA::Vector::new(11, 21), 2, 3))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    res = []
    i = l.begin_shapes(c0.cell_index, 0)
    while !i.at_end?
      res << i.shape.box.transformed(i.trans).to_s + " " + i.path.collect { |e| "[" + l.cell(e.cell_inst.cell_index).name + " #" + e.ia.to_s + "," + e.ib.to_s + " -> " + e.specific_trans.to_s + "]" }.join("")
      i.next
    end

    assert_equal(res.join("\n") + "\n", <<"END")
(1200,0;2200,1100) [c2 #-1,-1 -> r0 100,-100][c3 #-1,-1 -> r0 1100,0]
(-1200,0;-100,1000) [c3 #0,0 -> r90 0,0]
(-1190,20;-90,1020) [c3 #1,0 -> r90 10,20]
(-1189,21;-89,1021) [c3 #0,1 -> r90 11,21]
(-1179,41;-79,1041) [c3 #1,1 -> r90 21,41]
(-1178,42;-78,1042) [c3 #0,2 -> r90 22,42]
(-1168,62;-68,1062) [c3 #1,2 -> r90 32,62]
END

  end

  def test_3

    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(RBA::Box.new(0, 0, 3000, 2000))
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Vector.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Vector.new(1100, 0))))

    ii = RBA::RecursiveShapeIterator.new(l, c0, 0)
    assert_equal(ii.for_merged_input, false)
    assert_equal(collect(ii, l), "[c0](0,0;3000,2000)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")

    ii.for_merged_input = true
    assert_equal(ii.for_merged_input, true)
    assert_equal(collect(ii, l), "[c0](0,0;3000,2000)/[c3](-1200,0;-100,1000)")

  end

  def test_4

    ly = RBA::Layout::new
    top = ly.create_cell("TOP")
    l1 = ly.layer(1, 0)

    shape = top.shapes(l1).insert(RBA::BoxWithProperties::new(RBA::Box::new(1000, 2000), { 1 => 'A', 'X' => 17 }))

    iter = top.begin_shapes_rec(l1)
    iter.enable_properties
    
    assert_equal(iter.at_end, false)
    assert_equal(iter.prop_id, shape.prop_id)
    assert_equal(iter.property('Y'), nil)
    assert_equal(iter.property('X'), 17)
    assert_equal(iter.properties, { 1 => 'A', 'X' => 17 })

  end

end

load("test_epilogue.rb")

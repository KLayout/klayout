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

class DBLayout_TestClass < TestBase

  def collect(s, l)

    # check native iteration here too ..
    res2 = []
    s.each do |s|
      r = "[#{s.inst_cell.name}]"
      r += (s.trans * s.inst_trans).to_s
      res2.push(r)
    end

    res = []
    s.reset
    while !s.at_end?
      r = "[#{s.inst_cell.name}]"
      r += (s.trans * s.inst_trans).to_s
      res.push(r)
      s.next
    end

    assert_equal(res, res2)

    return res.join("/")

  end

  def dcollect(s, l)

    res = []
    while !s.at_end?
      r = "[#{s.inst_cell.name}]"
      r += (s.dtrans * s.inst_dtrans).to_s
      res.push(r)
      s.next
    end

    return res.join("/")

  end

  def test_1

    # Recursive instance iterator tests
   
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

    i1 = c0.begin_instances_rec_touching(RBA::Box.new(0, 0, 100, 100))
    i1copy = i1.dup
    assert_equal(i1copy.overlapping?, false)
    assert_equal(collect(i1, l), "[c1]r0 *1 0,0/[c2]r0 *1 100,-100")
    i1.reset
    assert_equal(dcollect(i1, l), "[c1]r0 *1 0,0/[c2]r0 *1 0.1,-0.1")
    assert_equal(collect(i1copy, l), "[c1]r0 *1 0,0/[c2]r0 *1 100,-100")

    i1 = c0.begin_instances_rec_touching(RBA::DBox.new(0, 0, 0.100, 0.100))
    assert_equal(collect(i1, l), "[c1]r0 *1 0,0/[c2]r0 *1 100,-100")

    i1o = c0.begin_instances_rec_overlapping(RBA::Box.new(0, 0, 100, 100));
    assert_equal(collect(i1o, l), "");
    i1o = c0.begin_instances_rec_overlapping(RBA::DBox.new(0, 0, 0.100, 0.100));
    assert_equal(collect(i1o, l), "");
    i1copy.overlapping = true
    assert_equal(i1copy.overlapping?, true)
    assert_equal(collect(i1copy, l), "");
    i1o = c0.begin_instances_rec_overlapping(RBA::Box.new(0, 0, 100, 101));
    assert_equal(collect(i1o, l), "[c1]r0 *1 0,0");
    i1o = c0.begin_instances_rec_overlapping(RBA::DBox.new(0, 0, 0.100, 0.101));
    assert_equal(collect(i1o, l), "[c1]r0 *1 0,0");
    i1copy.region = RBA::Box.new(0, 0, 100, 101)
    assert_equal(i1copy.region.to_s, "(0,0;100,101)")
    assert_equal(collect(i1copy, l), "[c1]r0 *1 0,0");
    i1copy.region = RBA::Region::new(RBA::Box.new(0, 0, 100, 101))
    assert_equal(i1copy.region.to_s, "(0,0;100,101)")
    assert_equal(collect(i1copy, l), "[c1]r0 *1 0,0");
    i1copy.region = RBA::Box.new(-1000, -1000, 1100, 1101)
    i1copy.confine_region(RBA::Box.new(0, 0, 100, 101))
    assert_equal(i1copy.region.to_s, "(0,0;100,101)")
    assert_equal(collect(i1copy, l), "[c1]r0 *1 0,0");
    i1copy.region = RBA::Box.new(-1000, -1000, 1100, 1101)
    i1copy.confine_region(RBA::Region::new(RBA::Box.new(0, 0, 100, 101)))
    assert_equal(i1copy.region.to_s, "(0,0;100,101)")
    assert_equal(collect(i1copy, l), "[c1]r0 *1 0,0");
    i1o = c0.begin_instances_rec_overlapping(RBA::Box.new(0, 0, 101, 101));
    assert_equal(collect(i1o, l), "[c1]r0 *1 0,0/[c2]r0 *1 100,-100");
    i1o = c0.begin_instances_rec_overlapping(RBA::Box.new(0, 0, 101, 101));
    assert_equal(collect(i1o, l), "[c1]r0 *1 0,0/[c2]r0 *1 100,-100");

    i2 = c0.begin_instances_rec_touching(RBA::Box.new(-100, 0, 100, 100));
    assert_equal(collect(i2, l), "[c1]r0 *1 0,0/[c2]r0 *1 100,-100/[c3]r90 *1 0,0");
    i2o = c0.begin_instances_rec_overlapping(RBA::Box.new(-100, 0, 100, 100));
    assert_equal(collect(i2o, l), "");
    i2o = c0.begin_instances_rec_overlapping(RBA::Box.new(-101, 0, 101, 101));
    assert_equal(collect(i2o, l), "[c1]r0 *1 0,0/[c2]r0 *1 100,-100/[c3]r90 *1 0,0");

    i4 = c0.begin_instances_rec_touching(RBA::Box.new(-100, 0, 2000, 100));
    i4_copy = c0.begin_instances_rec_touching(RBA::Box.new(-100, 0, 2000, 100));
    i4.max_depth = 0;
    assert_equal(collect(i4, l), "[c1]r0 *1 0,0/[c2]r0 *1 100,-100/[c3]r90 *1 0,0");

    assert_equal(i4 == i4, true);
    assert_equal(i4 != i4, false);
    assert_equal(i4 == i4_copy, false);
    assert_equal(i4 != i4_copy, true);
    i4 = i4_copy.dup
    assert_equal(i4 == i4_copy, true);
    assert_equal(i4 != i4_copy, false);
    i4.max_depth = 1;
    assert_equal(i4.max_depth, 1)
    assert_equal(collect(i4, l), "[c1]r0 *1 0,0/[c3]r0 *1 1200,-100/[c2]r0 *1 100,-100/[c3]r90 *1 0,0");
    i4.min_depth = 1;
    assert_equal(i4.min_depth, 1)
    assert_equal(collect(i4, l), "[c3]r0 *1 1200,-100");

    i4.assign(i4_copy);
    assert_equal(collect(i4, l), "[c1]r0 *1 0,0/[c3]r0 *1 1200,-100/[c2]r0 *1 100,-100/[c3]r90 *1 0,0");

    i5 = c0.begin_instances_rec
    assert_equal(collect(i5, l), "[c1]r0 *1 0,0/[c3]r0 *1 1200,-100/[c2]r0 *1 100,-100/[c3]r90 *1 0,0");

    ii = RBA::RecursiveInstanceIterator::new
    assert_equal(collect(ii, l), "")

    ii = RBA::RecursiveInstanceIterator::new(l, c2)
    assert_equal(collect(ii, l), "[c3]r0 *1 1100,0")
    assert_equal(ii.top_cell.name, "c2")
    assert_equal(ii.layout == l, true)
    
    ii = RBA::RecursiveInstanceIterator::new(l, c0, RBA::Box.new(-100, 0, 2000, 100), false)
    assert_equal(collect(ii, l), "[c1]r0 *1 0,0/[c3]r0 *1 1200,-100/[c2]r0 *1 100,-100/[c3]r90 *1 0,0")
    
    ii = RBA::RecursiveInstanceIterator::new(l, c0, RBA::Box.new(-100, 0, 2000, 100), true)
    assert_equal(collect(ii, l), "[c3]r0 *1 1200,-100/[c2]r0 *1 100,-100")
    
    ii = RBA::RecursiveInstanceIterator::new(l, c0, RBA::Box.new(-100, 0, 2000, 101), true)
    assert_equal(collect(ii, l), "[c1]r0 *1 0,0/[c3]r0 *1 1200,-100/[c2]r0 *1 100,-100")

    ii = RBA::RecursiveInstanceIterator::new(l, c0, RBA::Region::new(RBA::Box.new(-100, 0, 2000, 101)), true)
    assert_equal(collect(ii, l), "[c1]r0 *1 0,0/[c3]r0 *1 1200,-100/[c2]r0 *1 100,-100")

    ii = RBA::RecursiveInstanceIterator::new(l, c0)
    assert_equal(collect(ii, l), "[c1]r0 *1 0,0/[c3]r0 *1 1200,-100/[c2]r0 *1 100,-100/[c3]r90 *1 0,0")

    ii.reset
    ii.unselect_cells("c0")
    ii.select_cells("c2")
    assert_equal(collect(ii, l), "[c3]r0 *1 1200,-100")
    
    ii.reset_selection
    ii.unselect_cells("c*")
    ii.select_cells([ c2.cell_index ])
    assert_equal(collect(ii, l), "[c3]r0 *1 1200,-100")
    
    ii.reset_selection
    ii.unselect_all_cells
    ii.select_cells("c2")
    assert_equal(collect(ii, l), "[c3]r0 *1 1200,-100")
    
    ii.reset_selection
    ii.select_all_cells
    ii.unselect_cells("c2")
    assert_equal(collect(ii, l), "[c1]r0 *1 0,0/[c2]r0 *1 100,-100/[c3]r90 *1 0,0")
    
    ii.reset_selection
    ii.select_cells("c*")
    ii.unselect_cells([ c1.cell_index, c2.cell_index ])
    assert_equal(collect(ii, l), "[c1]r0 *1 0,0/[c2]r0 *1 100,-100/[c3]r90 *1 0,0")
    
    ii = RBA::RecursiveInstanceIterator::new(l, c0)
    assert_equal(ii.all_targets_enabled?, true)
    ii.targets = "c3"
    assert_equal(ii.all_targets_enabled?, false)
    assert_equal(collect(ii, l), "[c3]r0 *1 1200,-100/[c3]r90 *1 0,0")
    ii.enable_all_targets
    assert_equal(ii.all_targets_enabled?, true)
    assert_equal(collect(ii, l), "[c1]r0 *1 0,0/[c3]r0 *1 1200,-100/[c2]r0 *1 100,-100/[c3]r90 *1 0,0")
    ii.targets = [ c3.cell_index, c1.cell_index ]
    assert_equal(ii.all_targets_enabled?, false)
    assert_equal(collect(ii, l), "[c1]r0 *1 0,0/[c3]r0 *1 1200,-100/[c3]r90 *1 0,0")

  end

  def test_2

    # Recursive instance iterator tests
   
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
    i = c0.begin_instances_rec
    while !i.at_end?
      res << i.inst_cell.bbox.transformed(i.trans * i.inst_trans).to_s + " " + (i.path + [ i.current_inst_element ]).collect { |e| "[" + l.cell(e.cell_inst.cell_index).name + " #" + e.ia.to_s + "," + e.ib.to_s + " -> " + e.specific_trans.to_s + "]" }.join("")
      i.next
    end

    assert_equal(res.join("\n") + "\n", <<"END")
() [c1 #-1,-1 -> r0 0,0]
(1200,0;2201,1101) [c2 #-1,-1 -> r0 100,-100][c3 #-1,-1 -> r0 1100,0]
(1200,0;2201,1101) [c2 #-1,-1 -> r0 100,-100]
(-1201,0;-100,1001) [c3 #0,0 -> r90 0,0]
(-1191,20;-90,1021) [c3 #1,0 -> r90 10,20]
(-1190,21;-89,1022) [c3 #0,1 -> r90 11,21]
(-1180,41;-79,1042) [c3 #1,1 -> r90 21,41]
(-1179,42;-78,1043) [c3 #0,2 -> r90 22,42]
(-1169,62;-68,1063) [c3 #1,2 -> r90 32,62]
END

  end

end

load("test_epilogue.rb")

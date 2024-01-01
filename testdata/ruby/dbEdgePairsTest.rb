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

# normalizes a specification string for region, edges etc.
# such that the order of the objects becomes irrelevant
def csort(s)
  # splits at ");(" without consuming the brackets
  s.split(/(?<=\));(?=\()/).sort.join(";")
end

class DBEdgePairs_TestClass < TestBase

  # Basics
  def test_1

    r = RBA::EdgePairs::new
    assert_equal(r.to_s, "")
    assert_equal(r.is_empty?, true)
    assert_equal(r.size, 0)
    assert_equal(r.bbox.to_s, "()")
    data_id = r.data_id

    r.insert(RBA::Edge::new(0, 0, 0, 100), RBA::Edge::new(-10, 0, -20, 50))
    assert_equal(data_id != r.data_id, true)
    assert_equal(r.to_s, "(0,0;0,100)/(-10,0;-20,50)")
    r.clear

    r.insert(RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 100), RBA::Edge::new(-10, 0, -20, 50)))
    assert_equal(r.to_s, "(0,0;0,100)/(-10,0;-20,50)")
    assert_equal(r.extents.to_s, "(-20,0;-20,100;0,100;0,0)")
    assert_equal(r.extents(10).to_s, "(-30,-10;-30,110;10,110;10,-10)")
    assert_equal(r.extents(5, -5).to_s, "(-25,5;-25,95;5,95;5,5)")
    assert_equal(r.first_edges.to_s, "(0,0;0,100)")
    assert_equal(r.second_edges.to_s, "(-10,0;-20,50)")
    assert_equal(csort(r.edges.to_s), csort("(0,0;0,100);(-10,0;-20,50)"))
    assert_equal(r.is_empty?, false)
    assert_equal(r.size, 1)
    assert_equal(r[0].to_s, "(0,0;0,100)/(-10,0;-20,50)")
    assert_equal(r[1].to_s, "")
    assert_equal(r.bbox.to_s, "(-20,0;0,100)")

    assert_equal(r.moved(-10, 10).to_s, "(-10,10;-10,110)/(-20,10;-30,60)")
    assert_equal(r.moved(RBA::Point::new(-10, 10)).to_s, "(-10,10;-10,110)/(-20,10;-30,60)")
    rr = r.dup
    assert_equal(rr.data_id != r.data_id, true)
    rr.move(-10, 10)
    assert_equal(rr.to_s, "(-10,10;-10,110)/(-20,10;-30,60)")
    rr = r.dup
    rr.move(RBA::Point::new(-10, 10))
    assert_equal(rr.to_s, "(-10,10;-10,110)/(-20,10;-30,60)")

    assert_equal(r.transformed(RBA::Trans::new(1)).to_s, "(0,0;-100,0)/(0,-10;-50,-20)")
    assert_equal(r.transformed(RBA::ICplxTrans::new(2.0)).to_s, "(0,0;0,200)/(-20,0;-40,100)")
    rr = r.dup
    rr.transform(RBA::Trans::new(1))
    assert_equal(rr.to_s, "(0,0;-100,0)/(0,-10;-50,-20)")
    rr = r.dup
    rr.transform(RBA::ICplxTrans::new(2.0))
    assert_equal(rr.to_s, "(0,0;0,200)/(-20,0;-40,100)")

    rr = RBA::EdgePairs::new
    rr.swap(r)
    assert_equal(rr.to_s, "(0,0;0,100)/(-10,0;-20,50)")
    assert_equal(r.to_s, "")
    rr.swap(r)
    assert_equal(r.to_s, "(0,0;0,100)/(-10,0;-20,50)")
    r.clear

    assert_equal(r.to_s, "")
    assert_equal(r.is_empty?, true)
    assert_equal(r.size, 0)
    assert_equal(r.bbox.to_s, "()")

    ep = RBA::EdgePairs::new
    e = RBA::EdgePairs::new
    e.insert(RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 100), RBA::Edge::new(-10, 0, -20, 50)))
    ep.insert(e)
    assert_equal(ep.to_s, "(0,0;0,100)/(-10,0;-20,50)")

  end

  # Basics
  def test_2

    r1 = RBA::EdgePairs::new
    r1.insert(RBA::Edge::new(0, 0, 0, 100), RBA::Edge::new(-10, 0, -20, 50))
    r1.insert(RBA::Edge::new(0, 1, 0, 101), RBA::Edge::new(-10, 1, -20, 51))

    r2 = RBA::EdgePairs::new
    r2.insert(RBA::Edge::new(1, 0, 1, 100), RBA::Edge::new(-11, 0, -21, 50))
    r2.insert(RBA::Edge::new(1, 1, 1, 101), RBA::Edge::new(-11, 1, -21, 51))

    assert_equal(csort((r1 + r2).to_s), csort("(0,0;0,100)/(-10,0;-20,50);(0,1;0,101)/(-10,1;-20,51);(1,0;1,100)/(-11,0;-21,50);(1,1;1,101)/(-11,1;-21,51)"))
    assert_equal(csort((r1.join(r2)).to_s), csort("(0,0;0,100)/(-10,0;-20,50);(0,1;0,101)/(-10,1;-20,51);(1,0;1,100)/(-11,0;-21,50);(1,1;1,101)/(-11,1;-21,51)"))
    rr1 = r1.dup
    rr1 += r2
    assert_equal(csort(rr1.to_s), csort("(0,0;0,100)/(-10,0;-20,50);(0,1;0,101)/(-10,1;-20,51);(1,0;1,100)/(-11,0;-21,50);(1,1;1,101)/(-11,1;-21,51)"))
    rr1 = r1.dup
    rr1.join_with(r2)
    assert_equal(csort(rr1.to_s), csort("(0,0;0,100)/(-10,0;-20,50);(0,1;0,101)/(-10,1;-20,51);(1,0;1,100)/(-11,0;-21,50);(1,1;1,101)/(-11,1;-21,51)"))

  end

  def test_3

    ep1 = RBA::EdgePair::new(RBA::Edge::new(0, 1, 2, 3), RBA::Edge::new(10, 11, 12, 13))
    ep2 = RBA::EdgePair::new(RBA::Edge::new(20, 21, 22, 23), RBA::Edge::new(30, 31, 32, 33))
    ep3 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(10, 10, 10, 0))

    r1 = RBA::EdgePairs::new([ ep1, ep2 ])
    assert_equal(csort(r1.to_s), csort("(0,1;2,3)/(10,11;12,13);(20,21;22,23)/(30,31;32,33)"))

    r1 = RBA::EdgePairs::new(ep1)
    assert_equal(r1.to_s, "(0,1;2,3)/(10,11;12,13)")

    s = RBA::Shapes::new
    s.insert(ep1)
    s.insert(ep2)
    r1 = RBA::EdgePairs::new(s)
    assert_equal(csort(r1.to_s), csort("(0,1;2,3)/(10,11;12,13);(20,21;22,23)/(30,31;32,33)"))

    ly = RBA::Layout::new
    l1 = ly.layer("l1")
    l2 = ly.layer("l2")
    l3 = ly.layer("l3")
    c1 = ly.create_cell("C1")
    c2 = ly.create_cell("C2")
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 0)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 100)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(200, 100)))
    c2.shapes(l1).insert(ep1)
    c2.shapes(l2).insert(ep2)
    c2.shapes(l3).insert(ep3)
    
    r = RBA::EdgePairs::new(ly.begin_shapes(c1.cell_index, l1))
    assert_equal(r.to_s(30), "(0,1;2,3)/(10,11;12,13);(0,101;2,103)/(10,111;12,113);(200,101;202,103)/(210,111;212,113)")
    assert_equal(r.to_s(2), "(0,1;2,3)/(10,11;12,13);(0,101;2,103)/(10,111;12,113)...")
    assert_equal(r.is_empty?, false)
    assert_equal(r.size, 3)

    assert_equal(r.has_valid_edge_pairs?, false)
    assert_equal(r.bbox.to_s, "(0,1;212,113)")

    assert_equal(r.is_deep?, false)

    r.flatten
    assert_equal(r.has_valid_edge_pairs?, true)
    assert_equal(r[1].to_s, "(0,101;2,103)/(10,111;12,113)")
    assert_equal(r[100].inspect, "nil")
    assert_equal(r.bbox.to_s, "(0,1;212,113)")
    
    dss = RBA::DeepShapeStore::new
    r = RBA::EdgePairs::new(ly.begin_shapes(c1.cell_index, l1), dss)
    assert_equal(r.to_s(30), "(0,1;2,3)/(10,11;12,13);(0,101;2,103)/(10,111;12,113);(200,101;202,103)/(210,111;212,113)")
    assert_equal(r.to_s(2), "(0,1;2,3)/(10,11;12,13);(0,101;2,103)/(10,111;12,113)...")
    assert_equal(r.is_empty?, false)
    assert_equal(r.size, 3)

    assert_equal(r.has_valid_edge_pairs?, false)
    assert_equal(r.bbox.to_s, "(0,1;212,113)")

    assert_equal(r.is_deep?, true)

    r.flatten
    assert_equal(r.has_valid_edge_pairs?, false)
    assert_equal(r.to_s, "(0,1;2,3)/(10,11;12,13);(0,101;2,103)/(10,111;12,113);(200,101;202,103)/(210,111;212,113)")
    assert_equal(r.bbox.to_s, "(0,1;212,113)")

    assert_equal(r.is_deep?, true)

  end

  def test_4

    # insert_into and insert_into_as_polygons

    ep1 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(10, 10, 10, 0))

    ly = RBA::Layout::new
    l1 = ly.layer("l1")
    c1 = ly.create_cell("C1")
    c2 = ly.create_cell("C2")
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 0)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 100)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(200, 100)))
    c2.shapes(l1).insert(ep1)

    dss = RBA::DeepShapeStore::new
    r = RBA::EdgePairs::new(ly.begin_shapes(c1.cell_index, l1), dss)

    target = RBA::Layout::new
    target_top = target.add_cell("TOP")
    target_li = target.layer
    r.insert_into(target, target_top, target_li)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::EdgePairs::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::EdgePairs::new(target.cell("C2").shapes(target_li)).to_s, "(0,0;0,10)/(10,10;10,0)")

    target_li = target.layer
    r.insert_into_as_polygons(target, target_top, target_li, 1)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Region::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Region::new(target.cell("C2").shapes(target_li)).to_s, "(-1,-1;-1,11;11,11;11,-1)")

  end

  def test_5

    # filters

    ep1 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(10, 20, 10, 0))
    ep2 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(10, 0, 10, 20))
    ep3 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 20), RBA::Edge::new(10, 20, 10, 0))
    ep4 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(10, 0, 10, 10))

    r1 = RBA::EdgePairs::new([ ep1, ep2, ep3, ep4 ])

    assert_equal(r1.with_distance(10, false).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_distance(5, 20, false).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_distance(15, 20, false).to_s, "")
    assert_equal(r1.with_distance(15, 20, true).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")

    assert_equal(r1.with_length(10, false).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_length(10, 20, false).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_length(10, 21, false).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_length(10, true).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0)")
    assert_equal(r1.with_length_both(10, false).to_s, "(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_length_both(10, 20, false).to_s, "(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_length_both(10, 21, false).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_length_both(10, true).to_s, "(0,0;0,20)/(10,20;10,0)")

    assert_equal(r1.with_angle(0, false).to_s, "")
    assert_equal(r1.with_angle(0, true).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_angle(90, false).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_angle(0, 90, false).to_s, "")
    assert_equal(r1.with_angle(0, 90, false, true, true).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_angle_both(0, false).to_s, "")
    assert_equal(r1.with_angle_both(0, true).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_angle_both(90, false).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_angle_both(0, 90, false).to_s, "")
    assert_equal(r1.with_angle_both(0, 90, false, true, true).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")

    assert_equal(r1.with_area(0, false).to_s, "(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_area(150, false).to_s, "(0,0;0,10)/(10,20;10,0)")
    assert_equal(r1.with_area(0, true).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0)")
    assert_equal(r1.with_area(150, 151, false).to_s, "(0,0;0,10)/(10,20;10,0)")
    assert_equal(r1.with_area(150, 150, false).to_s, "")
    assert_equal(r1.with_area(150, 151, true).to_s, "(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")

    assert_equal(r1.with_internal_angle(0, false).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(10,0;10,20);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;10,10)")
    assert_equal(r1.with_internal_angle(0, 0, false).to_s, "")
    assert_equal(r1.with_internal_angle(0, true).to_s, "")

    ep1 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(10, 20, 10, 0))
    ep2 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(20, 0, 30, 0))
    ep3 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(-20, 0, -30, 0))

    r1 = RBA::EdgePairs::new([ ep1, ep2, ep3 ])

    assert_equal(r1.with_distance(20, false).to_s, "(0,0;0,10)/(20,0;30,0);(0,0;0,10)/(-20,0;-30,0)")
    assert_equal(r1.with_distance(20, true).to_s, "(0,0;0,10)/(10,20;10,0)")

    assert_equal(r1.with_internal_angle(0, false).to_s, "(0,0;0,10)/(10,20;10,0)")
    assert_equal(r1.with_internal_angle(90, false).to_s, "(0,0;0,10)/(20,0;30,0);(0,0;0,10)/(-20,0;-30,0)")
    assert_equal(r1.with_internal_angle(-90, false).to_s, "")

    assert_equal(r1.with_angle(90, false).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(20,0;30,0);(0,0;0,10)/(-20,0;-30,0)")
    assert_equal(r1.with_angle(0, false).to_s, "(0,0;0,10)/(20,0;30,0);(0,0;0,10)/(-20,0;-30,0)")
    assert_equal(r1.with_angle(0, 90, false, true, true).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(20,0;30,0);(0,0;0,10)/(-20,0;-30,0)")
    assert_equal(r1.with_angle_both(90, false).to_s, "(0,0;0,10)/(10,20;10,0)")
    assert_equal(r1.with_angle_both(0, false).to_s, "")

    ep1 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(10, 20, 10, 0))
    ep2 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(20, 10, 30, 0))

    r1 = RBA::EdgePairs::new([ ep1, ep2 ])

    assert_equal(r1.with_internal_angle(0, false).to_s, "(0,0;0,10)/(10,20;10,0)")
    assert_equal(r1.with_internal_angle(90, false).to_s, "")
    assert_equal(r1.with_internal_angle(90, true).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(20,10;30,0)")
    assert_equal(r1.with_internal_angle(45, false).to_s, "(0,0;0,10)/(20,10;30,0)")
    assert_equal(r1.with_internal_angle(0, 45, false, true, true).to_s, "(0,0;0,10)/(10,20;10,0);(0,0;0,10)/(20,10;30,0)")
    assert_equal(r1.with_internal_angle(0, 45, true, true, true).to_s, "")

    ep1 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(10, 0, 20, 0))
    ep2 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(10, 0, 20, 10))
    ep3 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 20), RBA::Edge::new(10, 20, 10, 0))
    ep4 = RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 10), RBA::Edge::new(10, 0, 15, 10))

    r = RBA::EdgePairs::new([ ep1, ep2, ep3, ep4 ])

    assert_equal(r.with_angle(RBA::Edges::OrthoEdges, false).to_s, "(0,0;0,10)/(10,0;20,0);(0,0;0,10)/(10,0;20,10);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;15,10)")
    assert_equal(r.with_angle(RBA::Edges::OrthoEdges, true).to_s, "(0,0;0,10)/(10,0;20,10);(0,0;0,10)/(10,0;15,10)")
    assert_equal(r.with_angle(RBA::Edges::DiagonalEdges, false).to_s, "(0,0;0,10)/(10,0;20,10)")
    assert_equal(r.with_angle(RBA::Edges::DiagonalEdges, true).to_s, "(0,0;0,10)/(10,0;20,0);(0,0;0,10)/(10,0;20,10);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;15,10)")
    assert_equal(r.with_angle(RBA::Edges::OrthoDiagonalEdges, false).to_s, "(0,0;0,10)/(10,0;20,0);(0,0;0,10)/(10,0;20,10);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;15,10)")
    assert_equal(r.with_angle(RBA::Edges::OrthoDiagonalEdges, true).to_s, "(0,0;0,10)/(10,0;15,10)")

    assert_equal(r.with_angle_both(RBA::Edges::OrthoEdges, false).to_s, "(0,0;0,10)/(10,0;20,0);(0,0;0,20)/(10,20;10,0)")
    assert_equal(r.with_angle_both(RBA::Edges::OrthoEdges, true).to_s, "")
    assert_equal(r.with_angle_both(RBA::Edges::DiagonalEdges, false).to_s, "")
    assert_equal(r.with_angle_both(RBA::Edges::DiagonalEdges, true).to_s, "(0,0;0,10)/(10,0;20,0);(0,0;0,20)/(10,20;10,0);(0,0;0,10)/(10,0;15,10)")
    assert_equal(r.with_angle_both(RBA::Edges::OrthoDiagonalEdges, false).to_s, "(0,0;0,10)/(10,0;20,0);(0,0;0,10)/(10,0;20,10);(0,0;0,20)/(10,20;10,0)")
    assert_equal(r.with_angle_both(RBA::Edges::OrthoDiagonalEdges, true).to_s, "")

  end

end


load("test_epilogue.rb")

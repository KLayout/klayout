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

# normalizes a specification string for region, edges etc.
# such that the order of the objects becomes irrelevant
def csort(s)
  # splits at ");(" or "};(" without consuming the brackets
  s.split(/(?<=[\)\}]);(?=\()/).sort.join(";")
end

class ParallelFilter < RBA::EdgeFilter

  # Constructor
  def initialize(ref_edge)
    self.is_scale_invariant   # orientation matters, but scale does not
    @ref_edge = ref_edge
  end
  
  # Select only parallel ones
  def selected(edge)
    return edge.is_parallel?(@ref_edge)
  end

end

class ShrinkToHalfEdgeOperator < RBA::EdgeOperator

  # Constructor
  def initialize
    self.is_isotropic_and_scale_invariant   # scale or orientation do not matter
  end
  
  # Shrink to half size
  def process(edge)
    shift = edge.bbox.center - RBA::Point::new   # shift vector
    return [ (edge.moved(-shift) * 0.5).moved(shift) ]
  end

end

class SomeEdgeToEdgePairOperator < RBA::EdgeToEdgePairOperator

  # Constructor
  def initialize
    self.is_isotropic_and_scale_invariant   # scale or orientation do not matter
  end
  
  def process(edge)
    box = edge.bbox
    return [ RBA::EdgePair::new([ box.left, box.bottom, box.left, box.top ], [ box.right, box.bottom, box.right, box.top ]) ]
  end

end

class SomeEdgeToPolygonOperator < RBA::EdgeToPolygonOperator

  # Constructor
  def initialize
    self.is_isotropic_and_scale_invariant   # scale or orientation do not matter
  end
  
  def process(edge)
    box = edge.bbox
    return [ RBA::Polygon::new(box) ]
  end

end

class DBEdges_TestClass < TestBase

  # Basics
  def test_1

    r = RBA::Edges::new
    assert_equal(r.to_s, "")
    assert_equal(r.is_empty?, true)
    assert_equal(r.count, 0)
    assert_equal(r.hier_count, 0)
    assert_equal(r.bbox.to_s, "()")
    assert_equal(r.is_merged?, true)
    data_id = r.data_id
    
    r.assign(RBA::Edges::new([RBA::Edge::new(10, 20, 100, 200)]))
    assert_equal(data_id != r.data_id, true)
    assert_equal(r.to_s, "(10,20;100,200)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 1)
    assert_equal(r.hier_count, 1)
    assert_equal(r.bbox.to_s, "(10,20;100,200)")
    assert_equal(r.is_merged?, true)

    r.assign(RBA::Edges::new([RBA::Edge::new(10, 20, 100, 200), RBA::Edge::new(11, 21, 101, 201)]))
    assert_equal(csort(r.to_s), csort("(10,20;100,200);(11,21;101,201)"))
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 2)
    assert_equal(r.hier_count, 2)
    assert_equal(r.bbox.to_s, "(10,20;101,201)")
    assert_equal(r.is_merged?, false)

    r.assign(RBA::Edges::new(RBA::Edge::new(10, 20, 100, 200)))
    assert_equal(r.to_s, "(10,20;100,200)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 1)
    assert_equal(r.hier_count, 1)
    assert_equal(r.bbox.to_s, "(10,20;100,200)")
    assert_equal(r.is_merged?, true)

    r.assign(RBA::Edges::new(RBA::Box::new(10, 20, 100, 200)))
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))
    s = r.each.collect(&:to_s).join(";")
    assert_equal(s, "(10,20;10,200) props={};(10,200;100,200) props={};(100,200;100,20) props={};(100,20;10,20) props={}")
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 4)
    assert_equal(r.hier_count, 4)
    assert_equal(r.bbox.to_s, "(10,20;100,200)")
    assert_equal(r.is_merged?, false)

    assert_equal(r.moved(RBA::Point::new(10, 20)).bbox.to_s, "(20,40;110,220)")
    assert_equal(r.moved(10, 20).bbox.to_s, "(20,40;110,220)")
    rr = r.dup
    assert_equal(rr.data_id != r.data_id, true)
    rr.move(RBA::Point::new(10, 20))
    assert_equal(rr.bbox.to_s, "(20,40;110,220)")
    rr = r.dup
    rr.move(10, 20)
    assert_equal(rr.bbox.to_s, "(20,40;110,220)")
    rr.swap(r)
    assert_equal(r.bbox.to_s, "(20,40;110,220)")
    assert_equal(rr.bbox.to_s, "(10,20;100,200)")

    assert_equal(r.transformed(RBA::Trans::new(1)).bbox.to_s, "(-220,20;-40,110)")
    rr = r.dup
    rr.transform(RBA::Trans::new(1))
    assert_equal(rr.bbox.to_s, "(-220,20;-40,110)")

    assert_equal(r.transformed(RBA::ICplxTrans::R180).bbox.to_s, "(-110,-220;-20,-40)")
    rr = r.dup
    rr.transform(RBA::ICplxTrans::R180)
    assert_equal(rr.bbox.to_s, "(-110,-220;-20,-40)")

    r.clear
    assert_equal(r.to_s, "")
    assert_equal(r.is_empty?, true)
    assert_equal(r.count, 0)
    assert_equal(r.hier_count, 0)
    assert_equal(r.bbox.to_s, "()")
    assert_equal(r.is_merged?, true)
    
    r = RBA::Edges::new(RBA::Polygon::new(RBA::Box::new(10, 20, 100, 200)))
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 4)
    assert_equal(r.hier_count, 4)
    assert_equal(r.bbox.to_s, "(10,20;100,200)")
    assert_equal(r.is_merged?, false)
    
    r = RBA::Edges::new(RBA::SimplePolygon::new(RBA::Box::new(10, 20, 100, 200)))
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 4)
    assert_equal(r.hier_count, 4)
    assert_equal(r.bbox.to_s, "(10,20;100,200)")
    assert_equal(r.length, 2*90+2*180)
    assert_equal(r.length(RBA::Box::new(0, 0, 50, 50)), 70)
    assert_equal(r.is_merged?, false)

    r.insert(RBA::Box::new(10, 20, 100, 200))
    assert_equal(r.is_merged?, false)
    assert_equal(r.merged_semantics?, true)
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20);(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))
    assert_equal(csort(r.merged.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))
    assert_equal(r.length, 2*90+2*180)
    r.merged_semantics = false
    assert_equal(r.merged_semantics?, false)
    assert_equal(r.length, 2*(2*90+2*180))
    
    r = RBA::Edges::new(RBA::Path::new([ RBA::Point::new(0, 0), RBA::Point::new(100, 0) ], 20))
    assert_equal(csort(r.to_s), csort("(0,-10;0,10);(0,10;100,10);(100,10;100,-10);(100,-10;0,-10)"))
    assert_equal(csort(r.extents.to_s), csort("(0,-10;0,-10;0,10;0,10);(0,10;100,10;100,10;0,10);(100,-10;100,-10;100,10;100,10);(0,-10;100,-10;100,-10;0,-10)"))
    assert_equal(csort(r.extents(10).to_s), csort("(-10,-20;-10,20;10,20;10,-20);(-10,0;-10,20;110,20;110,0);(90,-20;90,20;110,20;110,-20);(-10,-20;-10,0;110,0;110,-20)"))
    assert_equal(csort(r.extents(5, -5).to_s), csort("(-5,-5;-5,5;5,5;5,-5);(95,-5;95,5;105,5;105,-5)"))
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 4)
    assert_equal(r.hier_count, 4)
    assert_equal(r.bbox.to_s, "(0,-10;100,10)")
    assert_equal(r.is_merged?, false)
    
    r = RBA::Edges::new( [
        RBA::Polygon::new(RBA::Box::new(10, 20, 100, 200)),
        RBA::Polygon::new(RBA::Box::new(20, 50, 120, 250))
    ] )
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20);(20,50;20,250);(20,250;120,250);(120,250;120,50);(120,50;20,50)"))
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 8)
    assert_equal(r.hier_count, 8)
    assert_equal(r.bbox.to_s, "(10,20;120,250)")
    assert_equal(r.is_merged?, false)

    ly = RBA::Layout::new
    l1 = ly.layer("l1")
    l2 = ly.layer("l2")
    c1 = ly.create_cell("C1")
    c2 = ly.create_cell("C2")
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 0)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 100)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(200, 100)))
    c2.shapes(l1).insert(RBA::Box::new(-10, -20, 10, 20))
    c2.shapes(l2).insert(RBA::Edge::new(-10, -20, 10, 20))
    
    r = RBA::Edges::new(ly.begin_shapes(c1.cell_index, l1), true)
    assert_equal(r.to_s(30), "(-10,-20;-10,20);(-10,20;10,20);(10,20;10,-20);(10,-20;-10,-20);(-10,80;-10,120);(-10,120;10,120);(10,120;10,80);(10,80;-10,80);(190,80;190,120);(190,120;210,120);(210,120;210,80);(210,80;190,80)")
    assert_equal(r.to_s(2), "(-10,-20;-10,20);(-10,20;10,20)...")
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 12)
    assert_equal(r.hier_count, 12)
    assert_equal(r[1].to_s, "(-10,20;10,20)")
    assert_equal(r[100].to_s, "")
    assert_equal(r.bbox.to_s, "(-10,-20;210,120)")
    assert_equal(r.is_merged?, false)
    
    r = RBA::Edges::new(ly.begin_shapes(c1.cell_index, l2), false)
    assert_equal(r.to_s(30), "(-10,-20;10,20);(-10,80;10,120);(190,80;210,120)")
    assert_equal(r.to_s(2), "(-10,-20;10,20);(-10,80;10,120)...")
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 3)
    assert_equal(r.hier_count, 3)
    assert_equal(r.bbox.to_s, "(-10,-20;210,120)")
    assert_equal(r.is_merged?, false)
    assert_equal(r.has_valid_edges?, false)

    r.flatten
    assert_equal(r.has_valid_edges?, true)
    assert_equal(r[1].to_s, "(-10,80;10,120)")
    assert_equal(r[100].to_s, "")
    assert_equal(r.bbox.to_s, "(-10,-20;210,120)")
    assert_equal(r.is_merged?, false)
    
    r = RBA::Edges::new(ly.begin_shapes(c1.cell_index, l1), RBA::ICplxTrans::new(10, 20), true)
    assert_equal(r.to_s(30), "(0,0;0,40);(0,40;20,40);(20,40;20,0);(20,0;0,0);(0,100;0,140);(0,140;20,140);(20,140;20,100);(20,100;0,100);(200,100;200,140);(200,140;220,140);(220,140;220,100);(220,100;200,100)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 12)
    assert_equal(r.hier_count, 12)
    assert_equal(r.bbox.to_s, "(0,0;220,140)")
    assert_equal(r.is_merged?, false)

    a = 0
    r.each { |p| a += p.length }
    assert_equal(r.length, a)

    r = RBA::Edges::new(ly.begin_shapes(c1.cell_index, l2), RBA::ICplxTrans::new(10, 20), false)
    assert_equal(r.to_s(30), "(0,0;20,40);(0,100;20,140);(200,100;220,140)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.count, 3)
    assert_equal(r.hier_count, 3)
    assert_equal(r.bbox.to_s, "(0,0;220,140)")
    assert_equal(r.is_merged?, false)

    a = 0
    r.each { |p| a += p.length }
    assert_equal(r.length, a)

    r = RBA::Edges::new
    r.insert(RBA::Edge::new(10, 20, 100, 200))
    assert_equal(r.to_s, "(10,20;100,200)")
    r.clear
    r.insert(RBA::Box::new(10, 20, 100, 200))
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))
    r.clear
    r.insert(RBA::Polygon::new(RBA::Box::new(10, 20, 100, 200)))
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))
    r.clear
    r.insert(RBA::SimplePolygon::new(RBA::Box::new(10, 20, 100, 200)))
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))
    r.clear
    r.insert(RBA::Path::new([ RBA::Point::new(0, 0), RBA::Point::new(100, 0) ], 20))
    assert_equal(csort(r.to_s), csort("(0,-10;0,10);(0,10;100,10);(100,10;100,-10);(100,-10;0,-10)"))
    r.clear
    r.insert( [
        RBA::Polygon::new(RBA::Box::new(10, 20, 100, 200)),
        RBA::Polygon::new(RBA::Box::new(20, 50, 120, 250))
    ] )
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20);(20,50;20,250);(20,250;120,250);(120,250;120,50);(120,50;20,50)"))
    r.clear
    r.insert( [
        RBA::Edge::new(10, 20, 100, 200),
        RBA::Edge::new(20, 50, 120, 250)
    ] )
    assert_equal(csort(r.to_s), csort("(10,20;100,200);(20,50;120,250)"))
    r.clear
    r.insert(ly.begin_shapes(c1.cell_index, l1))
    assert_equal(r.to_s(30), "(-10,-20;-10,20);(-10,20;10,20);(10,20;10,-20);(10,-20;-10,-20);(-10,80;-10,120);(-10,120;10,120);(10,120;10,80);(10,80;-10,80);(190,80;190,120);(190,120;210,120);(210,120;210,80);(210,80;190,80)")
    r.clear
    r.insert(ly.begin_shapes(c1.cell_index, l1), RBA::ICplxTrans::new(10, 20))
    assert_equal(r.to_s(30), "(0,0;0,40);(0,40;20,40);(20,40;20,0);(20,0;0,0);(0,100;0,140);(0,140;20,140);(20,140;20,100);(20,100;0,100);(200,100;200,140);(200,140;220,140);(220,140;220,100);(220,100;200,100)")

    r = RBA::Edges::new
    rr = RBA::Edges::new
    rr.insert(RBA::Edge::new(10, 20, 100, 200))
    r.insert(rr)
    assert_equal(r.to_s, "(10,20;100,200)")

    r = RBA::Edges::new
    rr = RBA::Region::new
    rr.insert(RBA::Box::new(10, 20, 100, 200))
    r.insert(rr)
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))

    r = RBA::Edges::new
    s = RBA::Shapes::new
    s.insert(RBA::Edge::new(10, 20, 100, 200))
    r.insert(s)
    assert_equal(r.to_s, "(10,20;100,200)")

    r = RBA::Edges::new
    s = RBA::Shapes::new
    s.insert(RBA::Polygon::new(RBA::Box::new(10, 20, 100, 200)))
    r.insert(s)
    assert_equal(csort(r.to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))

    r = RBA::Edges::new
    s = RBA::Shapes::new
    s.insert(RBA::Text::new("ABC", RBA::Trans::new))
    r.insert(s)
    assert_equal(r.to_s, "")

    r = RBA::Edges::new
    s = RBA::Shapes::new
    s.insert(RBA::Edge::new(10, 20, 100, 200))
    r.insert(s, RBA::Trans::new(1, 1))
    assert_equal(r.to_s, "(11,21;101,201)")

    r = RBA::Edges::new
    s = RBA::Shapes::new
    s.insert(RBA::Edge::new(10, 20, 100, 200))
    r.insert(s, RBA::ICplxTrans::new(RBA::Trans::new(1, 1)))
    assert_equal(r.to_s, "(11,21;101,201)")

  end

  # Merge, booleans
  def test_2

    r1 = RBA::Edges::new
    r1.insert(RBA::Edge::new(0, 0, 100, 0))
    r2 = RBA::Edges::new
    r2.insert(RBA::Edge::new(50, 0, 200, 0))

    r = r1 + r2
    assert_equal(csort(r.to_s), csort("(0,0;100,0);(50,0;200,0)"))
    r = r1.join(r2)
    assert_equal(csort(r.to_s), csort("(0,0;100,0);(50,0;200,0)"))
    assert_equal(r.merged.to_s, "(0,0;200,0)")
    r.merge
    assert_equal(r.is_merged?, true)
    assert_equal(r.to_s, "(0,0;200,0)")
    r = r1.dup
    r += r2
    assert_equal(csort(r.to_s), csort("(0,0;100,0);(50,0;200,0)"))
    r = r1.dup
    r.join_with(r2)
    assert_equal(csort(r.to_s), csort("(0,0;100,0);(50,0;200,0)"))

    r = r1 | r2
    assert_equal(r.to_s, "(0,0;200,0)")
    r = r1.or(r2)
    assert_equal(r.to_s, "(0,0;200,0)")
    assert_equal(r.merged.to_s, "(0,0;200,0)")
    r.merge
    assert_equal(r.is_merged?, true)
    assert_equal(r.to_s, "(0,0;200,0)")
    r = r1.dup
    r |= r2
    assert_equal(r.to_s, "(0,0;200,0)")
    r = r1.dup
    r.or_with(r2)
    assert_equal(r.to_s, "(0,0;200,0)")

    r = r1 & r2
    assert_equal(r.to_s, "(50,0;100,0)")
    r = r1.and(r2)
    assert_equal(r.to_s, "(50,0;100,0)")
    assert_equal(r.is_merged?, true)
    r = r1.andnot(r2)[0]
    assert_equal(r.to_s, "(50,0;100,0)")
    assert_equal(r.is_merged?, true)
    r = r1.dup
    r &= r2
    assert_equal(r.to_s, "(50,0;100,0)")
    r = r1.dup
    r.and_with(r2)
    assert_equal(r.to_s, "(50,0;100,0)")

    r = r1 - r2
    assert_equal(r.to_s, "(0,0;50,0)")
    r = r1.not(r2)
    assert_equal(r.to_s, "(0,0;50,0)")
    assert_equal(r.is_merged?, true)
    r = r1.andnot(r2)[1]
    assert_equal(r.to_s, "(0,0;50,0)")
    assert_equal(r.is_merged?, true)
    r = r1.dup
    r -= r2
    assert_equal(r.to_s, "(0,0;50,0)")
    r = r1.dup
    r.not_with(r2)
    assert_equal(r.to_s, "(0,0;50,0)")

    r = r1 ^ r2
    assert_equal(csort(r.to_s), csort("(0,0;50,0);(100,0;200,0)"))
    r = r1.xor(r2)
    assert_equal(csort(r.to_s), csort("(0,0;50,0);(100,0;200,0)"))
    assert_equal(r.is_merged?, true)
    r = r1.dup
    r ^= r2
    assert_equal(csort(r.to_s), csort("(0,0;50,0);(100,0;200,0)"))
    r = r1.dup
    r.xor_with(r2)
    assert_equal(csort(r.to_s), csort("(0,0;50,0);(100,0;200,0)"))

  end

  # segments
  def test_3a

    r = RBA::Edges::new
    r.insert(RBA::Edge::new(0, 0, 100, 0))
    assert_equal(r.centers(10, 0.0).to_s, "(45,0;55,0)")
    assert_equal(r.centers(0, 0.0).to_s, "(50,0;50,0)")
    assert_equal(r.centers(0, 0.2).to_s, "(40,0;60,0)")
    assert_equal(r.start_segments(10, 0.0).to_s, "(0,0;10,0)")
    assert_equal(r.start_segments(0, 0.2).to_s, "(0,0;20,0)")
    assert_equal(r.end_segments(10, 0.0).to_s, "(90,0;100,0)")
    assert_equal(r.end_segments(0, 0.2).to_s, "(80,0;100,0)")

  end

  # extended
  def test_3b

    r = RBA::Edges::new
    r.insert(RBA::Edge::new(0, 0, 100, 0))
    r.insert(RBA::Edge::new(100, 0, 100, 100))
    assert_equal(csort(r.extended(1, 2, 3, 4, false).to_s), csort("(-1,-4;-1,3;102,3;102,-4);(97,-1;97,102;104,102;104,-1)"))
    assert_equal(r.extended(1, 2, 3, 4, true).to_s, "(-1,-4;-1,3;97,3;97,102;104,102;104,-4)")
    assert_equal(csort(r.extended_in(1).to_s), csort("(0,-1;0,0;100,0;100,-1);(100,0;100,100;101,100;101,0)"))
    assert_equal(csort(r.extended_out(1).to_s), csort("(0,0;0,1;100,1;100,0);(99,0;99,100;100,100;100,0)"))

  end

  # DRC
  def test_4

    r1 = RBA::Region::new
    r1.insert(RBA::Box::new(0, 0, 100, 200))
    r1.insert(RBA::Box::new(50, 200, 100, 220))
    r1.insert(RBA::Box::new(10, 220, 100, 400))
    r1 = r1.merged.edges
    r2 = RBA::Edges::new(RBA::Box::new(120, 20, 130, 380))
    r3 = RBA::Edges::new(RBA::Box::new(110, 0, 150, 400))
    r3a = RBA::Edges::new(RBA::Box::new(-30, -10, -10, 10))
    r3b = RBA::Edges::new(RBA::Box::new(-10, -10, 10, 10))

    assert_equal(r2.inside_check(r3, 15).to_s, "(120,20;120,380)/(110,9;110,391)")
    # "enclosed" alias
    assert_equal(r2.enclosed_check(r3, 15).to_s, "(120,20;120,380)/(110,9;110,391)")
    assert_equal(r2.inside_check(r3, 15, false, RBA::Edges::Projection, nil, nil, nil).to_s, "(120,20;120,380)/(110,20;110,380)")
    assert_equal(r2.inside_check(r3, 15, true, RBA::Edges::Projection, nil, nil, nil).to_s, "(120,20;120,380)/(110,0;110,400)")
    assert_equal(r2.inside_check(r3, 15, true, RBA::Edges::Projection, 0.0, nil, nil).to_s, "")
    assert_equal(r2.inside_check(r3, 15, false, RBA::Edges::Projection, nil, 0, 500).to_s, "(120,20;120,380)/(110,20;110,380)")
    assert_equal(r2.inside_check(r3, 15, false, RBA::Edges::Projection, nil, 380, 500).to_s, "")
    assert_equal(r2.inside_check(r3, 15, false, RBA::Edges::Projection, nil, 0, 300).to_s, "")
    
    assert_equal(r3.enclosing_check(r2, 15).to_s, "(110,9;110,391)/(120,20;120,380)")
    assert_equal(r3.enclosing_check(r2, 15, false, RBA::Region::Projection, nil, nil, nil).to_s, "(110,20;110,380)/(120,20;120,380)")
    assert_equal(r3.enclosing_check(r2, 15, true, RBA::Region::Projection, nil, nil, nil).to_s, "(110,0;110,400)/(120,20;120,380)")
    assert_equal(r3.enclosing_check(r2, 15, true, RBA::Region::Projection, 0.0, nil, nil).to_s, "")
    assert_equal(r3.enclosing_check(r2, 15, false, RBA::Region::Projection, nil, 0, 500).to_s, "(110,20;110,380)/(120,20;120,380)")
    assert_equal(r3.enclosing_check(r2, 15, false, RBA::Region::Projection, nil, 380, 500).to_s, "")
    assert_equal(r3.enclosing_check(r2, 15, false, RBA::Region::Projection, nil, 0, 300).to_s, "")
    
    assert_equal(r3a.separation_check(r1, 15).to_s, "(-10,10;-10,-10)/(0,0;0,21)")
    assert_equal(r3a.separation_check(r1, 15, false, RBA::Edges::Projection, nil, nil, nil).to_s, "(-10,10;-10,0)/(0,0;0,10)")
    assert_equal(r3a.separation_check(r1, 15, true, RBA::Edges::Projection, nil, nil, nil).to_s, "(-10,10;-10,-10)/(0,0;0,200)")
    assert_equal(r3a.separation_check(r1, 15, true, RBA::Edges::Projection, 0.0, nil, nil).to_s, "")
    assert_equal(r3a.separation_check(r1, 15, false, RBA::Edges::Projection, nil, 0, 500).to_s, "(-10,10;-10,0)/(0,0;0,10)")
    assert_equal(r3a.separation_check(r1, 15, false, RBA::Edges::Projection, nil, 380, 500).to_s, "")
    assert_equal(r3a.separation_check(r1, 15, false, RBA::Edges::Projection, nil, 0, 300).to_s, "(-10,10;-10,0)/(0,0;0,10)")
    
    assert_equal(csort(r3b.overlap_check(r1, 15).to_s), csort("(-10,10;10,10)/(21,0;0,0);(10,10;10,-10)/(0,0;0,21)"))
    assert_equal(csort(r3b.overlap_check(r1, 15, false, RBA::Edges::Projection, nil, nil, nil).to_s), csort("(0,10;10,10)/(10,0;0,0);(10,10;10,0)/(0,0;0,10)"))
    assert_equal(csort(r3b.overlap_check(r1, 15, true, RBA::Edges::Projection, nil, nil, nil).to_s), csort("(-10,10;10,10)/(100,0;0,0);(10,10;10,-10)/(0,0;0,200)"))
    assert_equal(r3b.overlap_check(r1, 15, true, RBA::Edges::Projection, 0.0, nil, nil).to_s, "")
    assert_equal(csort(r3b.overlap_check(r1, 15, false, RBA::Edges::Projection, nil, 0, 500).to_s), csort("(0,10;10,10)/(10,0;0,0);(10,10;10,0)/(0,0;0,10)"))
    assert_equal(r3b.overlap_check(r1, 15, false, RBA::Edges::Projection, nil, 380, 500).to_s, "")
    assert_equal(csort(r3b.overlap_check(r1, 15, false, RBA::Edges::Projection, nil, 0, 300).to_s), csort("(0,10;10,10)/(10,0;0,0);(10,10;10,0)/(0,0;0,10)"))
    
    assert_equal(csort((r2 | r1).space_check(25).to_s), csort("(120,20;120,380)/(100,395;100,5);(0,200;50,200)/(50,220;10,220)"))
    assert_equal(csort((r2 | r1).space_check(25, false, RBA::Edges::Projection, nil, nil, nil).to_s), csort("(120,20;120,380)/(100,380;100,20);(10,200;50,200)/(50,220;10,220)"))
    assert_equal(csort((r2 | r1).space_check(25, true, RBA::Edges::Projection, nil, nil, nil).to_s), csort("(120,20;120,380)/(100,400;100,0);(0,200;50,200)/(50,220;10,220)"))
    assert_equal((r2 | r1).space_check(25, true, RBA::Edges::Projection, 0.0, nil, nil).to_s, "")
    assert_equal((r2 | r1).space_check(25, true, RBA::Edges::Projection, nil, 50, nil).to_s, "(120,20;120,380)/(100,400;100,0)")
    assert_equal((r2 | r1).space_check(25, true, RBA::Edges::Projection, nil, nil, 50).to_s, "(0,200;50,200)/(50,220;10,220)")

    assert_equal(csort((r2 | r1).width_check(60).to_s), csort("(120,20;120,380)/(130,380;130,20);(50,200;50,220)/(100,253;100,167)"))
    assert_equal(csort((r2 | r1).width_check(60, false, RBA::Edges::Projection, nil, nil, nil).to_s), csort("(120,20;120,380)/(130,380;130,20);(50,200;50,220)/(100,220;100,200)"))
    assert_equal(csort((r2 | r1).width_check(60, true, RBA::Edges::Projection, nil, nil, nil).to_s), csort("(120,20;120,380)/(130,380;130,20);(50,200;50,220)/(100,400;100,0)"))
    assert_equal((r2 | r1).width_check(60, true, RBA::Edges::Projection, 0.0, nil, nil).to_s, "")
    assert_equal((r2 | r1).width_check(60, true, RBA::Edges::Projection, nil, 50, nil).to_s, "(120,20;120,380)/(130,380;130,20)")
    assert_equal((r2 | r1).width_check(60, true, RBA::Edges::Projection, nil, nil, 50).to_s, "(50,200;50,220)/(100,400;100,0)")

    # kissing corner/separation case

    r1 = RBA::Region::new
    r1.insert(RBA::Box::new(0, 0, 100, 200))
    r1 = r1.edges

    r2 = RBA::Region::new
    r2.insert(RBA::Box::new(100, 200, 200, 400))
    r2 = r2.edges

    r3a = RBA::Region::new
    r3a.insert(RBA::Box::new(-10, 0, 110, 100))
    r3a = r3a.edges

    r3b = RBA::Region::new
    r3b.insert(RBA::Box::new(10, 0, 90, 100))
    r3b = r3b.edges

    assert_equal(csort((r2 | r1).width_check(60, false, RBA::Edges::Euclidian, nil, nil, nil).to_s), csort("(100,200;100,140)/(100,200;100,260);(160,200;100,200)/(40,200;100,200)"))
    assert_equal(csort((r2 | r1).width_check(60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::NeverIncludeZeroDistance).to_s), csort(""))
    assert_equal(csort((r2 | r1).width_check(60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::IncludeZeroDistanceWhenTouching).to_s), csort("(100,200;100,140)/(100,200;100,260);(160,200;100,200)/(40,200;100,200)"))

    assert_equal(csort((r2 | r1).space_check(60, false, RBA::Edges::Euclidian, nil, nil, nil).to_s), csort("(100,200;100,140)/(100,200;100,260);(160,200;100,200)/(40,200;100,200)"))
    assert_equal(csort((r2 | r1).space_check(60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::NeverIncludeZeroDistance).to_s), csort(""))
    assert_equal(csort((r2 | r1).space_check(60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::IncludeZeroDistanceWhenTouching).to_s), csort("(100,200;100,140)/(100,200;100,260);(160,200;100,200)/(40,200;100,200)"))

    assert_equal(csort(r1.separation_check(r2, 60, false, RBA::Edges::Euclidian, nil, nil, nil).to_s), csort("(100,200;100,140)/(100,200;100,260);(40,200;100,200)/(160,200;100,200)"))
    assert_equal(csort(r1.separation_check(r2, 60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::NeverIncludeZeroDistance).to_s), csort(""))
    assert_equal(csort(r1.separation_check(r2, 60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::IncludeZeroDistanceWhenTouching).to_s), csort("(100,200;100,140)/(100,200;100,260);(40,200;100,200)/(160,200;100,200)"))

    assert_equal(csort(r1.inside_check(r3b, 60, false, RBA::Edges::Euclidian, nil, nil, nil).to_s), csort("(100,0;0,0)/(90,0;10,0)"))
    assert_equal(csort(r1.inside_check(r3b, 60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::NeverIncludeZeroDistance).to_s), csort(""))
    assert_equal(csort(r1.inside_check(r3b, 60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::IncludeZeroDistanceWhenTouching).to_s), csort("(100,0;0,0)/(90,0;10,0)"))

    assert_equal(csort(r1.enclosing_check(r3a, 60, false, RBA::Edges::Euclidian, nil, nil, nil).to_s), csort("(100,0;0,0)/(110,0;-10,0)"))
    assert_equal(csort(r1.enclosing_check(r3a, 60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::NeverIncludeZeroDistance).to_s), csort(""))
    assert_equal(csort(r1.enclosing_check(r3a, 60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::IncludeZeroDistanceWhenTouching).to_s), csort("(100,0;0,0)/(110,0;-10,0)"))

    assert_equal(csort(r1.overlap_check(r2, 60, false, RBA::Edges::Euclidian, nil, nil, nil).to_s), csort("(100,200;100,140)/(100,200;100,260);(40,200;100,200)/(160,200;100,200)"))
    assert_equal(csort(r1.overlap_check(r2, 60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::NeverIncludeZeroDistance).to_s), csort(""))
    assert_equal(csort(r1.overlap_check(r2, 60, false, RBA::Edges::Euclidian, nil, nil, nil, RBA::Edges::IncludeZeroDistanceWhenTouching).to_s), csort("(100,200;100,140)/(100,200;100,260);(40,200;100,200)/(160,200;100,200)"))

  end

  # with..
  def test_5

    r = RBA::Edges::new
    r.insert(RBA::Edge::new(0, 0, 100, 0))
    r.insert(RBA::Edge::new(100, 0, 100, 50))
    assert_equal(r.with_angle(0, false).to_s, "(0,0;100,0)")
    assert_equal(r.split_with_angle(0)[0].to_s, "(0,0;100,0)")
    assert_equal(r.with_abs_angle(0, false).to_s, "(0,0;100,0)")
    assert_equal(r.split_with_abs_angle(0)[0].to_s, "(0,0;100,0)")
    assert_equal(r.with_angle(0, true).to_s, "(100,0;100,50)")
    assert_equal(r.split_with_angle(0)[1].to_s, "(100,0;100,50)")
    assert_equal(r.with_abs_angle(0, true).to_s, "(100,0;100,50)")
    assert_equal(r.split_with_abs_angle(0)[1].to_s, "(100,0;100,50)")
    assert_equal(r.with_angle(90, false).to_s, "(100,0;100,50)")
    assert_equal(r.with_abs_angle(90, false).to_s, "(100,0;100,50)")
    assert_equal(r.with_angle(90, true).to_s, "(0,0;100,0)")
    assert_equal(r.with_abs_angle(90, true).to_s, "(0,0;100,0)")
    assert_equal(r.with_angle(-10, 10, false).to_s, "(0,0;100,0)")
    assert_equal(r.split_with_angle(-10, 10)[0].to_s, "(0,0;100,0)")
    assert_equal(r.with_abs_angle(-10, 10, false).to_s, "(0,0;100,0)")
    assert_equal(r.with_angle(-10, 10, true).to_s, "(100,0;100,50)")
    assert_equal(r.split_with_angle(-10, 10)[1].to_s, "(100,0;100,50)")
    assert_equal(r.with_abs_angle(-10, 10, true).to_s, "(100,0;100,50)")
    assert_equal(r.with_angle(80, 100, false).to_s, "(100,0;100,50)")
    assert_equal(r.with_abs_angle(80, 100, false).to_s, "(100,0;100,50)")
    assert_equal(r.with_length(100, false).to_s, "(0,0;100,0)")
    assert_equal(r.split_with_length(100)[0].to_s, "(0,0;100,0)")
    assert_equal(r.with_length(100, true).to_s, "(100,0;100,50)")
    assert_equal(r.split_with_length(100)[1].to_s, "(100,0;100,50)")
    assert_equal(r.with_length(50, false).to_s, "(100,0;100,50)")
    assert_equal(r.with_length(50, true).to_s, "(0,0;100,0)")
    assert_equal(r.with_length(100, nil, false).to_s, "(0,0;100,0)")
    assert_equal(r.split_with_length(100, nil)[0].to_s, "(0,0;100,0)")
    assert_equal(r.with_length(100, 200, true).to_s, "(100,0;100,50)")
    assert_equal(r.split_with_length(100, 200)[1].to_s, "(100,0;100,50)")
    assert_equal(r.with_length(nil, 100, false).to_s, "(100,0;100,50)")

    r = RBA::Edges::new
    r.insert(RBA::Edge::new(0, 0, 100, 0))
    r.insert(RBA::Edge::new(100, 0, 100, 50))
    r.insert(RBA::Edge::new(0, 0, 100, 100))
    r.insert(RBA::Edge::new(0, 0, 100, -100))
    r.insert(RBA::Edge::new(0, 0, 100, 120))
    assert_equal(r.with_angle(RBA::Edges::OrthoEdges, false).to_s, "(0,0;100,0);(100,0;100,50)")
    assert_equal(r.with_angle(RBA::Edges::OrthoEdges, true).to_s, "(0,0;100,100);(0,0;100,-100);(0,0;100,120)")
    assert_equal(r.with_angle(RBA::Edges::DiagonalEdges, false).to_s, "(0,0;100,100);(0,0;100,-100)")
    assert_equal(r.with_angle(RBA::Edges::DiagonalEdges, true).to_s, "(0,0;100,0);(100,0;100,50);(0,0;100,120)")
    assert_equal(r.with_angle(RBA::Edges::OrthoDiagonalEdges, false).to_s, "(0,0;100,0);(100,0;100,50);(0,0;100,100);(0,0;100,-100)")
    assert_equal(r.with_angle(RBA::Edges::OrthoDiagonalEdges, true).to_s, "(0,0;100,120)")

  end

  # interact
  def test_6

    r = RBA::Edges::new
    r.insert(RBA::Edge::new(0, 0, 100, 0))
    r.insert(RBA::Edge::new(100, 0, 100, 50))
    r2 = RBA::Edges::new
    r2.insert(RBA::Edge::new(0, 10, 200, 10))
    g2 = RBA::Region::new
    g2.insert(RBA::Box::new(0, 10, 200, 20))

    assert_equal(r.interacting(r2).to_s, "(100,0;100,50)")
    assert_equal(r.split_interacting(r2)[0].to_s, "(100,0;100,50)")
    assert_equal(r.not_interacting(r2).to_s, "(0,0;100,0)")
    assert_equal(r.split_interacting(r2)[1].to_s, "(0,0;100,0)")
    rr = r.dup
    rr.select_interacting(r2)
    assert_equal(rr.to_s, "(100,0;100,50)")
    rr = r.dup
    rr.select_not_interacting(r2)
    assert_equal(rr.to_s, "(0,0;100,0)")

    assert_equal(r.interacting(g2).to_s, "(100,0;100,50)")
    assert_equal(r.split_interacting(g2)[0].to_s, "(100,0;100,50)")
    assert_equal(r.not_interacting(g2).to_s, "(0,0;100,0)")
    assert_equal(r.split_interacting(g2)[1].to_s, "(0,0;100,0)")
    rr = r.dup
    rr.select_interacting(g2)
    assert_equal(rr.to_s, "(100,0;100,50)")
    rr = r.dup
    rr.select_not_interacting(g2)
    assert_equal(rr.to_s, "(0,0;100,0)")

  end

  # Selections
  def test_7

    r1 = RBA::Edges::new
    r1.insert(RBA::Box::new(10, 20, 100, 200))
    r1.insert(RBA::Box::new(50, 70, 150, 270))
    r2 = RBA::Edges::new
    r2.insert(RBA::Box::new(100, 70, 250, 270))
    r2.insert(RBA::Box::new(10, 20, 100, 200))

    r1.merged_semantics = false
    r2.merged_semantics = false

    assert_equal(csort(r1.in(r2).to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))
    assert_equal(csort(r2.in(r1).to_s), csort("(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)"))
    assert_equal(csort(r1.not_in(r2).to_s), csort("(50,70;50,270);(50,270;150,270);(150,270;150,70);(150,70;50,70)"))
    assert_equal(csort(r2.not_in(r1).to_s), csort("(100,70;100,270);(100,270;250,270);(250,270;250,70);(250,70;100,70)"))

    r1.merged_semantics = true
    r2.merged_semantics = true

    assert_equal(csort(r1.in(r2).to_s), csort("(10,20;10,200);(10,200;100,200);(100,20;10,20)"))
    assert_equal(csort(r2.in(r1).to_s), csort("(10,20;10,200);(10,200;100,200);(100,20;10,20)"))
    assert_equal(csort(r1.not_in(r2).to_s), csort("(100,200;100,20);(50,70;50,270);(50,270;150,270);(150,270;150,70);(150,70;50,70)"))
    assert_equal(csort(r2.not_in(r1).to_s), csort("(100,270;250,270);(250,270;250,70);(250,70;100,70);(100,70;100,20);(100,200;100,270)"))

  end

  # Edges and regions
  def test_8

    r = RBA::Region::new
    r.insert(RBA::Box::new(0, 0, 100, 200))

    e = RBA::Edges::new
    e.insert(RBA::Edge::new(-100, 100, 200, 100))
    assert_equal((e & r).to_s, "(0,100;100,100)")
    assert_equal(e.inside_part(r).to_s, "(0,100;100,100)")
    assert_equal(e.inside_outside_part(r)[0].to_s, "(0,100;100,100)")

    ee = e.dup
    assert_equal((ee & r).to_s, "(0,100;100,100)")
    assert_equal(ee.andnot(r)[0].to_s, "(0,100;100,100)")
    ee &= r
    assert_equal(ee.to_s, "(0,100;100,100)")

    ee = e.dup
    ee.select_inside_part(r)
    assert_equal(ee.to_s, "(0,100;100,100)")

    assert_equal(csort((e - r).to_s), csort("(-100,100;0,100);(100,100;200,100)"))
    assert_equal(csort(e.outside_part(r).to_s), csort("(-100,100;0,100);(100,100;200,100)"))
    assert_equal(csort(e.inside_outside_part(r)[1].to_s), csort("(-100,100;0,100);(100,100;200,100)"))

    ee = e.dup
    assert_equal((ee - r).to_s, csort("(-100,100;0,100);(100,100;200,100)"))
    assert_equal(ee.andnot(r)[1].to_s, csort("(-100,100;0,100);(100,100;200,100)"))
    ee -= r
    assert_equal(csort(ee.to_s), csort("(-100,100;0,100);(100,100;200,100)"))

    ee = e.dup
    ee.select_outside_part(r)
    assert_equal(csort(ee.to_s), csort("(-100,100;0,100);(100,100;200,100)"))

    e.clear
    e.insert(RBA::Edge::new(-100, 0, 200, 0))
    assert_equal((e & r).to_s, "(0,0;100,0)")
    assert_equal(e.inside_part(r).to_s, "")

    ee = e.dup
    ee &= r
    assert_equal(ee.to_s, "(0,0;100,0)")

    ee = e.dup
    ee.select_inside_part(r)
    assert_equal(ee.to_s, "")

    assert_equal(csort((e - r).to_s), csort("(-100,0;0,0);(100,0;200,0)"))
    assert_equal(csort(e.outside_part(r).to_s), csort("(-100,0;0,0);(0,0;100,0);(100,0;200,0)"))

    ee = e.dup
    ee -= r
    assert_equal(csort(ee.to_s), csort("(-100,0;0,0);(100,0;200,0)"))

    ee = e.dup
    ee.select_outside_part(r)
    assert_equal(csort(ee.to_s), csort("(-100,0;0,0);(0,0;100,0);(100,0;200,0)"))

  end

  # deep edges
  def test_9

    ly = RBA::Layout::new
    l1 = ly.layer("l1")
    c1 = ly.create_cell("C1")
    c2 = ly.create_cell("C2")
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 0)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 100)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(200, 100)))
    c2.shapes(l1).insert(RBA::Box::new(-10, -20, 10, 20))
    
    dss = RBA::DeepShapeStore::new
    r = RBA::Edges::new(ly.begin_shapes(c1.cell_index, l1), dss, true)
    assert_equal(r.to_s(30), "(-10,-20;-10,20);(-10,20;10,20);(10,20;10,-20);(10,-20;-10,-20);(-10,80;-10,120);(-10,120;10,120);(10,120;10,80);(10,80;-10,80);(190,80;190,120);(190,120;210,120);(210,120;210,80);(210,80;190,80)")
    assert_equal(r.to_s(2), "(-10,-20;-10,20);(-10,20;10,20)...")
    assert_equal(r.is_empty?, false)
    assert_equal(r.bbox.to_s, "(-10,-20;210,120)")
    assert_equal(r.is_deep?, true)

    target = RBA::Layout::new
    target_top = target.add_cell("TOP")
    target_li = target.layer
    r.insert_into(target, target_top, target_li)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Edges::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(csort(RBA::Edges::new(target.cell("C2").shapes(target_li)).to_s), csort("(-10,-20;-10,20);(-10,20;10,20);(10,20;10,-20);(10,-20;-10,-20)"))

    r.flatten

    assert_equal(r.is_deep?, true)

    assert_equal(r.count, 12)
    assert_equal(r.hier_count, 12)
    assert_equal(r.to_s, "(-10,-20;-10,20);(-10,20;10,20);(10,20;10,-20);(10,-20;-10,-20);(-10,80;-10,120);(-10,120;10,120);(10,120;10,80);(10,80;-10,80);(190,80;190,120);(190,120;210,120)...")

    dss._destroy

  end

  # inside
  def test_10

    r = RBA::Edges::new
    r.insert(RBA::Edge::new(0, 0, 100, 0))
    r.insert(RBA::Edge::new(0, 10, 100, 10))
    r2 = RBA::Edges::new
    r2.insert(RBA::Edge::new(0, 10, 200, 10))
    g2 = RBA::Region::new
    g2.insert(RBA::Box::new(0, 10, 200, 20))

    assert_equal(r.inside(r2).to_s, "(0,10;100,10)")
    assert_equal(r.split_inside(r2)[0].to_s, "(0,10;100,10)")
    assert_equal(r.not_inside(r2).to_s, "(0,0;100,0)")
    assert_equal(r.split_inside(r2)[1].to_s, "(0,0;100,0)")
    rr = r.dup
    rr.select_inside(r2)
    assert_equal(rr.to_s, "(0,10;100,10)")
    rr = r.dup
    rr.select_not_inside(r2)
    assert_equal(rr.to_s, "(0,0;100,0)")

    assert_equal(r.inside(g2).to_s, "(0,10;100,10)")
    assert_equal(r.split_inside(g2)[0].to_s, "(0,10;100,10)")
    assert_equal(r.not_inside(g2).to_s, "(0,0;100,0)")
    assert_equal(r.split_inside(g2)[1].to_s, "(0,0;100,0)")
    rr = r.dup
    rr.select_inside(g2)
    assert_equal(rr.to_s, "(0,10;100,10)")
    rr = r.dup
    rr.select_not_inside(g2)
    assert_equal(rr.to_s, "(0,0;100,0)")

  end

  # outside
  def test_11

    r = RBA::Edges::new
    r.insert(RBA::Edge::new(0, 0, 100, 0))
    r.insert(RBA::Edge::new(0, 10, 100, 10))
    r2 = RBA::Edges::new
    r2.insert(RBA::Edge::new(0, 0, 200, 0))
    g2 = RBA::Region::new
    g2.insert(RBA::Box::new(0, -5, 200, 5))

    assert_equal(r.outside(r2).to_s, "(0,10;100,10)")
    assert_equal(r.split_outside(r2)[0].to_s, "(0,10;100,10)")
    assert_equal(r.not_outside(r2).to_s, "(0,0;100,0)")
    assert_equal(r.split_outside(r2)[1].to_s, "(0,0;100,0)")
    rr = r.dup
    rr.select_outside(r2)
    assert_equal(rr.to_s, "(0,10;100,10)")
    rr = r.dup
    rr.select_not_outside(r2)
    assert_equal(rr.to_s, "(0,0;100,0)")

    assert_equal(r.outside(g2).to_s, "(0,10;100,10)")
    assert_equal(r.split_outside(g2)[0].to_s, "(0,10;100,10)")
    assert_equal(r.not_outside(g2).to_s, "(0,0;100,0)")
    assert_equal(r.split_outside(g2)[1].to_s, "(0,0;100,0)")
    rr = r.dup
    rr.select_outside(g2)
    assert_equal(rr.to_s, "(0,10;100,10)")
    rr = r.dup
    rr.select_not_outside(g2)
    assert_equal(rr.to_s, "(0,0;100,0)")

  end

  # Generic filters
  def test_generic_filters

    # Some basic tests for the filter class

    f = ParallelFilter::new(RBA::Edge::new(0, 0, 100, 100))
    assert_equal(f.wants_variants?, true)
    f.wants_variants = false
    assert_equal(f.wants_variants?, false)
    assert_equal(f.requires_raw_input, false)
    f.requires_raw_input = false
    assert_equal(f.requires_raw_input, false)

    # Smoke test
    f.is_isotropic
    f.is_scale_invariant

    # Some application

    f = ParallelFilter::new(RBA::Edge::new(0, 0, 100, 100))

    edges = RBA::Edges::new
    edges.insert(RBA::Edge::new(100, 0, 200, 100))
    edges.insert(RBA::Edge::new(100, 100, 100, 200))

    assert_equal(edges.filtered(f).to_s, "(100,0;200,100)")
    assert_equal(edges.split_filter(f)[0].to_s, "(100,0;200,100)")
    assert_equal(edges.split_filter(f)[1].to_s, "(100,100;100,200)")
    assert_equal(edges.to_s, "(100,0;200,100);(100,100;100,200)")
    edges.filter(f)
    assert_equal(edges.to_s, "(100,0;200,100)")

  end

  # Generic processors
  def test_generic_processors_ee

    # Some basic tests for the processor class

    f = ShrinkToHalfEdgeOperator::new
    assert_equal(f.wants_variants?, true)
    f.wants_variants = false
    assert_equal(f.wants_variants?, false)
    assert_equal(f.requires_raw_input, false)
    f.requires_raw_input = true
    assert_equal(f.requires_raw_input, true)
    assert_equal(f.result_is_merged, false)
    f.result_is_merged = true
    assert_equal(f.result_is_merged, true)
    assert_equal(f.result_must_not_be_merged, false)
    f.result_must_not_be_merged = true
    assert_equal(f.result_must_not_be_merged, true)

    # Smoke test
    f.is_isotropic
    f.is_scale_invariant

    # Some application

    edges = RBA::Edges::new

    edges.insert(RBA::Edge::new(0, 0, 100, 100))
    edges.insert(RBA::Edge::new(200, 300, 200, 500))

    assert_equal(edges.processed(ShrinkToHalfEdgeOperator::new).to_s, "(25,25;75,75);(200,350;200,450)")
    assert_equal(edges.to_s, "(0,0;100,100);(200,300;200,500)")
    edges.process(ShrinkToHalfEdgeOperator::new)
    assert_equal(edges.to_s, "(25,25;75,75);(200,350;200,450)")

  end

  # Generic processors
  def test_generic_processors_eep

    p = SomeEdgeToEdgePairOperator::new

    edges = RBA::Edges::new

    edges.insert(RBA::Edge::new(0, 0, 100, 100))
    edges.insert(RBA::Edge::new(200, 300, 200, 500))

    assert_equal(edges.processed(p).to_s, "(0,0;0,100)/(100,0;100,100);(200,300;200,500)/(200,300;200,500)")
    assert_equal(edges.to_s, "(0,0;100,100);(200,300;200,500)")

  end

  # Generic processors
  def test_generic_processors_ep

    p = SomeEdgeToPolygonOperator::new

    edges = RBA::Edges::new

    edges.insert(RBA::Edge::new(0, 0, 100, 100))
    edges.insert(RBA::Edge::new(200, 300, 200, 500))

    assert_equal(edges.processed(p).to_s, "(0,0;0,100;100,100;100,0);(200,300;200,300;200,500;200,500)")
    assert_equal(edges.to_s, "(0,0;100,100);(200,300;200,500)")

  end

  # properties
  def test_props

    r = RBA::Edges::new([ RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 100), { 1 => "one" }) ])
    assert_equal(r.to_s, "(0,0;100,100){1=>one}")

    r = RBA::Edges::new([])
    assert_equal(r.to_s, "")

    r = RBA::Edges::new
    r.insert([ RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 100), { 1 => "one" }) ])
    assert_equal(r.to_s, "(0,0;100,100){1=>one}")

    r = RBA::Edges::new(RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 100), { 1 => "one" }))
    assert_equal(r.to_s, "(0,0;100,100){1=>one}")

    r = RBA::Edges::new
    r.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 100), { 1 => "one" }))
    assert_equal(r.to_s, "(0,0;100,100){1=>one}")

    r = RBA::Edges::new([ RBA::PolygonWithProperties::new(RBA::Box::new(0, 0, 100, 200), { 1 => "one" }) ])
    assert_equal(r.to_s, "(0,0;0,200){1=>one};(0,200;100,200){1=>one};(100,200;100,0){1=>one};(100,0;0,0){1=>one}")

    r = RBA::Edges::new
    r.insert([ RBA::PolygonWithProperties::new(RBA::Box::new(0, 0, 100, 200), { 1 => "one" }) ])
    assert_equal(r.to_s, "(0,0;0,200){1=>one};(0,200;100,200){1=>one};(100,200;100,0){1=>one};(100,0;0,0){1=>one}")

    r = RBA::Edges::new(RBA::PolygonWithProperties::new(RBA::Box::new(0, 0, 100, 200), { 1 => "one" }))
    assert_equal(r.to_s, "(0,0;0,200){1=>one};(0,200;100,200){1=>one};(100,200;100,0){1=>one};(100,0;0,0){1=>one}")

    r = RBA::Edges::new
    r.insert(RBA::PolygonWithProperties::new(RBA::Box::new(0, 0, 100, 200), { 1 => "one" }))
    assert_equal(r.to_s, "(0,0;0,200){1=>one};(0,200;100,200){1=>one};(100,200;100,0){1=>one};(100,0;0,0){1=>one}")

    r = RBA::Edges::new(RBA::SimplePolygonWithProperties::new(RBA::Box::new(0, 0, 100, 200), { 1 => "one" }))
    assert_equal(r.to_s, "(0,0;0,200){1=>one};(0,200;100,200){1=>one};(100,200;100,0){1=>one};(100,0;0,0){1=>one}")

    r = RBA::Edges::new
    r.insert(RBA::SimplePolygonWithProperties::new(RBA::Box::new(0, 0, 100, 200), { 1 => "one" }))
    assert_equal(r.to_s, "(0,0;0,200){1=>one};(0,200;100,200){1=>one};(100,200;100,0){1=>one};(100,0;0,0){1=>one}")

    r = RBA::Edges::new(RBA::BoxWithProperties::new(RBA::Box::new(0, 0, 100, 200), { 1 => "one" }))
    assert_equal(r.to_s, "(0,0;0,200){1=>one};(0,200;100,200){1=>one};(100,200;100,0){1=>one};(100,0;0,0){1=>one}")

    r = RBA::Edges::new
    r.insert(RBA::BoxWithProperties::new(RBA::Box::new(0, 0, 100, 200), { 1 => "one" }))
    assert_equal(r.to_s, "(0,0;0,200){1=>one};(0,200;100,200){1=>one};(100,200;100,0){1=>one};(100,0;0,0){1=>one}")

    r = RBA::Edges::new
    r.insert(RBA::Region::new(RBA::BoxWithProperties::new(RBA::Box::new(0, 0, 100, 200), { 1 => "one" })))
    assert_equal(r.to_s, "(0,0;0,200){1=>one};(0,200;100,200){1=>one};(100,200;100,0){1=>one};(100,0;0,0){1=>one}")

    r = RBA::Edges::new
    s = RBA::Shapes::new
    s.insert(RBA::BoxWithProperties::new(RBA::Box::new(0, 0, 100, 200), { 1 => "one" }))
    r.insert(s)
    assert_equal(r.to_s, "(0,0;0,200){1=>one};(0,200;100,200){1=>one};(100,200;100,0){1=>one};(100,0;0,0){1=>one}")

    r = RBA::Edges::new
    s = RBA::Shapes::new
    s.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 100), { 1 => "one" }))
    r.insert(s)
    assert_equal(r.to_s, "(0,0;100,100){1=>one}")

    r = RBA::Edges::new
    s = RBA::Shapes::new
    s.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 100), { 1 => "one" }))
    r.insert(s)
    assert_equal(r.to_s, "(0,0;100,100){1=>one}")

    r = RBA::Edges::new
    r.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 0), { 1 => "one" }))
    r.insert(RBA::Edge::new(10, 0, 110, 0))
    s = r.each.collect(&:to_s).join(";")
    assert_equal(s, "(10,0;110,0) props={};(0,0;100,0) props={1=>one}")
    s = r.each_merged.collect(&:to_s).join(";")
    assert_equal(s, "(10,0;110,0) props={};(0,0;100,0) props={1=>one}")

    r = RBA::Edges::new
    r.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 0), { 1 => "one" }))
    r.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(10, 0, 110, 0), { 1 => "one" }))
    s = r.each_merged.collect(&:to_s).join(";")
    assert_equal(s, "(0,0;110,0) props={1=>one}")

  end

  # properties
  def test_prop_filters

    r = RBA::Edges::new
    r.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 200), { "one" => -1 }))
    r.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(1, 1, 101, 201), { "one" => 17 }))
    r.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(2, 2, 102, 202), { "one" => 42 }))

    assert_equal(r.filtered(RBA::EdgeFilter::property_filter("one", 11)).to_s, "")
    assert_equal(r.filtered(RBA::EdgeFilter::property_filter("two", 17)).to_s, "")
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter("one", 17)).to_s), csort("(1,1;101,201){one=>17}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter("one", 17, true)).to_s), csort("(0,0;100,200){one=>-1};(2,2;102,202){one=>42}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter_bounded("one", 17, nil)).to_s), csort("(2,2;102,202){one=>42};(1,1;101,201){one=>17}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter_bounded("one", 17, 18)).to_s), csort("(1,1;101,201){one=>17}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter_bounded("one", 17, 18, true)).to_s), csort("(2,2;102,202){one=>42};(0,0;100,200){one=>-1}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter_bounded("one", nil, 18)).to_s), csort("(1,1;101,201){one=>17};(0,0;100,200){one=>-1}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_glob("one", "1*")).to_s), csort("(1,1;101,201){one=>17}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_glob("one", "1*", true)).to_s), csort("(2,2;102,202){one=>42};(0,0;100,200){one=>-1}"))

    ly = RBA::Layout::new
    top = ly.create_cell("TOP")
    l1 = ly.layer(1, 0)

    s = top.shapes(l1)
    s.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 200), { "one" => -1 }))
    s.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(1, 1, 101, 201), { "one" => 17 }))
    s.insert(RBA::EdgeWithProperties::new(RBA::Edge::new(2, 2, 102, 202), { "one" => 42 }))

    dss = RBA::DeepShapeStore::new
    iter = top.begin_shapes_rec(l1)
    iter.enable_properties()
    r = RBA::Edges::new(iter, dss)

    assert_equal(r.filtered(RBA::EdgeFilter::property_filter("one", 11)).to_s, "")
    assert_equal(r.filtered(RBA::EdgeFilter::property_filter("two", 17)).to_s, "")
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter("one", 17)).to_s), csort("(1,1;101,201){one=>17}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter("one", 17, true)).to_s), csort("(0,0;100,200){one=>-1};(2,2;102,202){one=>42}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter_bounded("one", 17, nil)).to_s), csort("(2,2;102,202){one=>42};(1,1;101,201){one=>17}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter_bounded("one", 17, 18)).to_s), csort("(1,1;101,201){one=>17}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter_bounded("one", 17, 18, true)).to_s), csort("(2,2;102,202){one=>42};(0,0;100,200){one=>-1}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_filter_bounded("one", nil, 18)).to_s), csort("(1,1;101,201){one=>17};(0,0;100,200){one=>-1}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_glob("one", "1*")).to_s), csort("(1,1;101,201){one=>17}"))
    assert_equal(csort(r.filtered(RBA::EdgeFilter::property_glob("one", "1*", true)).to_s), csort("(2,2;102,202){one=>42};(0,0;100,200){one=>-1}"))

    rr = r.dup
    rr.filter(RBA::EdgeFilter::property_filter("one", 17))
    assert_equal(csort(rr.to_s), csort("(1,1;101,201){one=>17}"))

    dss._destroy

  end

end

load("test_epilogue.rb")


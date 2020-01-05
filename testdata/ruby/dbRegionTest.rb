# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2020 Matthias Koefferlein
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

class DBRegion_TestClass < TestBase

  # Basics
  def test_1

    r = RBA::Region::new
    assert_equal(r.to_s, "")
    assert_equal(r.is_empty?, true)
    assert_equal(r.size, 0)
    assert_equal(r.bbox.to_s, "()")
    assert_equal(r.is_merged?, true)
    assert_equal(r.is_box?, false)
    data_id = r.data_id
    
    r.assign(RBA::Region::new(RBA::Box::new(10, 20, 100, 200)))
    assert_equal(data_id != r.data_id, true)
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.size, 1)
    assert_equal(r.bbox.to_s, "(10,20;100,200)")
    assert_equal(r.is_merged?, true)
    assert_equal(r.is_box?, true)
    assert_equal(r.edges.to_s, "(10,20;10,200);(10,200;100,200);(100,200;100,20);(100,20;10,20)")

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
    assert_equal(r.size, 0)
    assert_equal(r.bbox.to_s, "()")
    assert_equal(r.is_merged?, true)
    assert_equal(r.is_box?, false)
    
    r = RBA::Region::new(RBA::Polygon::new(RBA::Box::new(10, 20, 100, 200)))
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.is_box?, true)
    assert_equal(r.size, 1)
    assert_equal(r.bbox.to_s, "(10,20;100,200)")
    assert_equal(r.is_merged?, false)
    
    r = RBA::Region::new(RBA::SimplePolygon::new(RBA::Box::new(10, 20, 100, 200)))
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.is_box?, true)
    assert_equal(r.size, 1)
    assert_equal(r.bbox.to_s, "(10,20;100,200)")
    assert_equal(r.area, 90*180)
    assert_equal(r.perimeter, 2*90+2*180)
    assert_equal(r.perimeter(RBA::Box::new(0, 0, 50, 50)), 30+40)
    assert_equal(r.is_merged?, false)
    
    r = RBA::Region::new(RBA::Path::new([ RBA::Point::new(0, 0), RBA::Point::new(100, 0) ], 20))
    assert_equal(r.to_s, "(0,-10;0,10;100,10;100,-10)")
    assert_equal(r.is_box?, true)
    assert_equal(r.is_empty?, false)
    assert_equal(r.size, 1)
    assert_equal(r.bbox.to_s, "(0,-10;100,10)")
    assert_equal(r.is_merged?, false)
    
    r = RBA::Region::new( [
        RBA::Polygon::new(RBA::Box::new(10, 20, 100, 200)),
        RBA::Polygon::new(RBA::Box::new(20, 50, 120, 250))
    ] )
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20);(20,50;20,250;120,250;120,50)")
    s = "" 
    r.each do |p|
      s.empty? || s += ";"
      s += p.to_s
    end
    assert_equal(s, "(10,20;10,200;100,200;100,20);(20,50;20,250;120,250;120,50)")
    assert_equal(r.merged.to_s, "(10,20;10,200;20,200;20,250;120,250;120,50;100,50;100,20)")
    assert_equal(r.merged(false, 1).to_s, "(10,20;10,200;20,200;20,250;120,250;120,50;100,50;100,20)")
    assert_equal(r.merged(1).to_s, "(10,20;10,200;20,200;20,250;120,250;120,50;100,50;100,20)")
    assert_equal(r.merged(false, 2).to_s, "(20,50;20,200;100,200;100,50)")
    assert_equal(r.merged(2).to_s, "(20,50;20,200;100,200;100,50)")
    assert_equal(r.merged(false, 3).to_s, "")
    assert_equal(r.merged(3).to_s, "")
    assert_equal(r.is_empty?, false)
    assert_equal(r.is_box?, false)
    assert_equal(r.size, 2)
    assert_equal(r.bbox.to_s, "(10,20;120,250)")
    assert_equal(r.is_merged?, false)

    rr = r.dup
    rr.merge
    assert_equal(rr.to_s, "(10,20;10,200;20,200;20,250;120,250;120,50;100,50;100,20)")
    rr = r.dup
    rr.merge(false, 2)
    assert_equal(rr.to_s, "(20,50;20,200;100,200;100,50)")

    ly = RBA::Layout::new
    l1 = ly.layer("l1")
    l2 = ly.layer("l2")
    c1 = ly.create_cell("C1")
    c2 = ly.create_cell("C2")
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 0)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 100)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(200, 100)))
    c2.shapes(l1).insert(RBA::Box::new(-10, -20, 10, 20))
    c2.shapes(l2).insert(RBA::Text::new("AA", RBA::Vector::new(-10, -20)))
    c2.shapes(l2).insert(RBA::Text::new("BB", RBA::Vector::new(10, 20)))
    
    r = RBA::Region::new(ly.begin_shapes(c1.cell_index, l1))
    assert_equal(r.to_s, "(-10,-20;-10,20;10,20;10,-20);(-10,80;-10,120;10,120;10,80);(190,80;190,120;210,120;210,80)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.size, 3)
    assert_equal(r.bbox.to_s, "(-10,-20;210,120)")
    assert_equal(r.is_merged?, false)
    assert_equal(r.has_valid_polygons?, false)

    r.flatten
    assert_equal(r.has_valid_polygons?, true)
    assert_equal(r[1].to_s, "(-10,80;-10,120;10,120;10,80)")
    assert_equal(r[4].to_s, "")
    assert_equal(r.bbox.to_s, "(-10,-20;210,120)")
    assert_equal(r.is_merged?, false)
    
    r = RBA::Region::new(ly.begin_shapes(c1.cell_index, l2), "*")
    assert_equal(r.to_s, "(-11,-21;-11,-19;-9,-19;-9,-21);(9,19;9,21;11,21;11,19);(-11,79;-11,81;-9,81;-9,79);(9,119;9,121;11,121;11,119);(189,79;189,81;191,81;191,79);(209,119;209,121;211,121;211,119)")
    r = RBA::Region::new(ly.begin_shapes(c1.cell_index, l2), "A*")
    assert_equal(r.to_s, "(-11,-21;-11,-19;-9,-19;-9,-21);(-11,79;-11,81;-9,81;-9,79);(189,79;189,81;191,81;191,79)")
    r = RBA::Region::new(ly.begin_shapes(c1.cell_index, l2), "A*", false)
    assert_equal(r.to_s, "")
    r = RBA::Region::new(ly.begin_shapes(c1.cell_index, l2), "AA", false)
    assert_equal(r.to_s, "(-11,-21;-11,-19;-9,-19;-9,-21);(-11,79;-11,81;-9,81;-9,79);(189,79;189,81;191,81;191,79)")

    r = RBA::Region::new(ly.begin_shapes(c1.cell_index, l1), RBA::ICplxTrans::new(10, 20))
    assert_equal(r.to_s, "(0,0;0,40;20,40;20,0);(0,100;0,140;20,140;20,100);(200,100;200,140;220,140;220,100)")
    assert_equal(r.extents.to_s, "(0,0;0,40;20,40;20,0);(0,100;0,140;20,140;20,100);(200,100;200,140;220,140;220,100)")
    assert_equal(r.extents(10).to_s, "(-10,-10;-10,50;30,50;30,-10);(-10,90;-10,150;30,150;30,90);(190,90;190,150;230,150;230,90)")
    assert_equal(r.extents(5, -5).to_s, "(-5,5;-5,35;25,35;25,5);(-5,105;-5,135;25,135;25,105);(195,105;195,135;225,135;225,105)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.is_box?, false)
    assert_equal(r.size, 3)
    assert_equal(r.bbox.to_s, "(0,0;220,140)")
    assert_equal(r.is_merged?, false)

    a = 0
    r.each { |p| a += p.area }
    assert_equal(r.area, a)

    r = RBA::Region::new
    r.insert(RBA::Box::new(10, 20, 100, 200))
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20)")
    assert_equal(r.area(RBA::Box::new(0, 0, 50, 50)), 30*40)
    r.clear
    r.insert(RBA::Polygon::new(RBA::Box::new(10, 20, 100, 200)))
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20)")
    r.clear
    r.insert(RBA::SimplePolygon::new(RBA::Box::new(10, 20, 100, 200)))
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20)")
    r.clear
    r.insert(RBA::Path::new([ RBA::Point::new(0, 0), RBA::Point::new(100, 0) ], 20))
    assert_equal(r.to_s, "(0,-10;0,10;100,10;100,-10)")
    r.clear
    r.insert( [
        RBA::Polygon::new(RBA::Box::new(10, 20, 100, 200)),
        RBA::Polygon::new(RBA::Box::new(20, 50, 120, 250))
    ] )
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20);(20,50;20,250;120,250;120,50)")
    r.clear
    r.insert(ly.begin_shapes(c1.cell_index, l1))
    assert_equal(r.to_s, "(-10,-20;-10,20;10,20;10,-20);(-10,80;-10,120;10,120;10,80);(190,80;190,120;210,120;210,80)")
    r.clear
    r.insert(ly.begin_shapes(c1.cell_index, l1), RBA::ICplxTrans::new(10, 20))
    assert_equal(r.to_s, "(0,0;0,40;20,40;20,0);(0,100;0,140;20,140;20,100);(200,100;200,140;220,140;220,100)")

    r = RBA::Region::new
    rr = RBA::Region::new
    rr.insert(RBA::Box::new(10, 20, 100, 200))
    r.insert(rr)
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20)")

    r = RBA::Region::new
    s = RBA::Shapes::new
    s.insert(RBA::Box::new(10, 20, 100, 200))
    r.insert(s)
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20)")

    r = RBA::Region::new
    s = RBA::Shapes::new
    s.insert(RBA::Polygon::new(RBA::Box::new(10, 20, 100, 200)))
    r.insert(s)
    assert_equal(r.to_s, "(10,20;10,200;100,200;100,20)")

    r = RBA::Region::new
    s = RBA::Shapes::new
    s.insert(RBA::Edge::new(10, 20, 100, 200))
    r.insert(s)
    assert_equal(r.to_s, "")

    r = RBA::Region::new
    s = RBA::Shapes::new
    s.insert(RBA::Box::new(10, 20, 100, 200))
    r.insert(s, RBA::Trans::new(1, 1))
    assert_equal(r.to_s, "(11,21;11,201;101,201;101,21)")

    r = RBA::Region::new
    s = RBA::Shapes::new
    s.insert(RBA::Box::new(10, 20, 100, 200))
    r.insert(s, RBA::ICplxTrans::new(RBA::Trans::new(1, 1)))
    assert_equal(r.to_s, "(11,21;11,201;101,201;101,21)")

  end

  # Booleans
  def test_2

    r1 = RBA::Region::new(RBA::Box::new(10, 20, 100, 200))
    r2 = RBA::Region::new(RBA::Box::new(-10, -20, 80, 160))

    assert_equal((r1 & r2).to_s, "(10,20;10,160;80,160;80,20)")
    rr = r1.dup
    rr &= r2
    assert_equal(rr.to_s, "(10,20;10,160;80,160;80,20)")
    
    assert_equal((r1 - r2).to_s, "(80,20;80,160;10,160;10,200;100,200;100,20)")
    rr = r1.dup
    rr -= r2
    assert_equal(rr.to_s, "(80,20;80,160;10,160;10,200;100,200;100,20)")
    
    assert_equal((r1 ^ r2).to_s, "(-10,-20;-10,160;10,160;10,200;100,200;100,20;80,20;80,-20/10,20;80,20;80,160;10,160)")
    r1.min_coherence = true
    assert_equal((r1 ^ r2).to_s, "(-10,-20;-10,160;10,160;10,20;80,20;80,-20);(80,20;80,160;10,160;10,200;100,200;100,20)")
    rr = r1.dup
    rr ^= r2
    assert_equal(rr.to_s, "(-10,-20;-10,160;10,160;10,20;80,20;80,-20);(80,20;80,160;10,160;10,200;100,200;100,20)")
    
    assert_equal((r1 + r2).to_s, "(10,20;10,200;100,200;100,20);(-10,-20;-10,160;80,160;80,-20)")
    rr = r1.dup
    rr += r2
    assert_equal(rr.to_s, "(10,20;10,200;100,200;100,20);(-10,-20;-10,160;80,160;80,-20)")
    
    assert_equal((r1 | r2).to_s, "(-10,-20;-10,160;10,160;10,200;100,200;100,20;80,20;80,-20)")
    rr = r1.dup
    rr |= r2
    assert_equal(rr.to_s, "(-10,-20;-10,160;10,160;10,200;100,200;100,20;80,20;80,-20)")
    
    assert_equal((r1 + r2).sized(10).to_s, "(-20,-30;-20,170;0,170;0,210;110,210;110,10;90,10;90,-30)")
    rr = (r1 | r2).dup
    rr.size(10)
    assert_equal(rr.to_s, "(-20,-30;-20,170;0,170;0,210;110,210;110,10;90,10;90,-30)")

    assert_equal((r1 | r2).sized(10, 2).to_s, "(-20,-30;-20,170;0,170;0,210;110,210;110,10;90,10;90,-30)")
    rr = (r1 | r2).dup
    rr.size(10, 2)
    assert_equal(rr.to_s, "(-20,-30;-20,170;0,170;0,210;110,210;110,10;90,10;90,-30)")

    assert_equal((r1 | r2).sized(10, 20, 2).to_s, "(-20,-40;-20,180;0,180;0,220;110,220;110,0;90,0;90,-40)")
    rr = (r1 | r2).dup
    rr.size(10, 20, 2)
    assert_equal(rr.to_s, "(-20,-40;-20,180;0,180;0,220;110,220;110,0;90,0;90,-40)")

    r1.merged_semantics = false

    assert_equal((r1 | r2).sized(10, 2).to_s, "(-20,-30;-20,170;0,170;0,210;110,210;110,10;90,10;90,-30)")
    rr = (r1 | r2).dup
    rr.size(10, 2)
    assert_equal(rr.to_s, "(-20,-30;-20,170;0,170;0,210;110,210;110,10;90,10;90,-30)")

    assert_equal((r1 + r2).sized(10, 2).to_s, "(0,10;0,210;110,210;110,10);(-20,-30;-20,170;90,170;90,-30)")
    rr = (r1 + r2).dup
    rr.size(10, 2)
    assert_equal(rr.to_s, "(0,10;0,210;110,210;110,10);(-20,-30;-20,170;90,170;90,-30)")

    assert_equal((r1 + r2).sized(10, 20, 2).to_s, "(0,0;0,220;110,220;110,0);(-20,-40;-20,180;90,180;90,-40)")
    rr = (r1 + r2).dup
    rr.size(10, 20, 2)
    assert_equal(rr.to_s, "(0,0;0,220;110,220;110,0);(-20,-40;-20,180;90,180;90,-40)")

  end

  # Special
  def test_3

    r = RBA::Region::new(RBA::Box::new(0, 0, 100, 200))
    assert_equal(r.minkowsky_sum(RBA::Edge::new(0, 0, 10, 10)).to_s, "(0,0;0,200;10,210;110,210;110,10;100,0)")
    assert_equal(r.minkowsky_sum(RBA::Polygon::new(RBA::Box::new(0, 0, 10, 10))).to_s, "(0,0;0,210;110,210;110,0)")
    assert_equal(r.minkowsky_sum(RBA::Box::new(0, 0, 10, 10)).to_s, "(0,0;0,210;110,210;110,0)")
    assert_equal(r.minkowsky_sum([RBA::Point::new(0, 0), RBA::Point::new(10, 10)]).to_s, "(0,0;0,200;10,210;110,210;110,10;100,0)")

    rr = r.dup
    rr.round_corners(10, 20, 8)
    assert_equal(rr.to_s, "(12,0;0,12;0,188;12,200;88,200;100,188;100,12;88,0)")

    rr = r.dup 
    rr += RBA::Region::new(RBA::Box::new(10, 192, 20, 202)).rounded_corners(5, 5, 16)
    rr.merge
    assert_equal(rr.to_s, "(0,0;0,200;11,200;12,201;14,202;16,202;18,201;19,200;100,200;100,0)")
    rr.smooth(1)
    assert_equal(rr.to_s, "(0,0;0,200;11,200;14,202;18,201;100,200;100,0)")
    assert_equal(rr.smoothed(5).to_s, "(0,0;0,200;100,200;100,0)")
    rr.smooth(5)
    assert_equal(rr.to_s, "(0,0;0,200;100,200;100,0)")

    r1 = RBA::Region::new(RBA::Box::new(-10, -20, 100, 200))
    r1.insert(RBA::Box::new(-10, 220, 100, 400))
    r2 = RBA::Region::new(RBA::Box::new(10, 20, 30, 50))
    r2.insert(RBA::Box::new(40, 20, 60, 50))

    r = r1 ^ r2
    assert_equal(r.holes.to_s, "(10,20;10,50;30,50;30,20);(40,20;40,50;60,50;60,20)")
    assert_equal(r.hulls.to_s, "(-10,-20;-10,200;100,200;100,-20);(-10,220;-10,400;100,400;100,220)")

    r1 = RBA::Region::new
    r1.insert(RBA::Box::new(0, 0, 1000, 100))
    r1.insert(RBA::Box::new(900, 0, 1000, 1000))
    r1.merge
    r2 = r1.dup
    r2.break(0, 0.0)
    assert_equal(r2.to_s, "(0,0;0,100;900,100;900,1000;1000,1000;1000,0)")
    r2 = r1.dup
    r2.break(0, 3.0)
    assert_equal(r2.to_s, "(0,0;0,100;1000,100;1000,0);(900,100;900,1000;1000,1000;1000,100)")
    r2 = r1.dup
    r2.break(4, 0.0)
    assert_equal(r2.to_s, "(0,0;0,100;1000,100;1000,0);(900,100;900,1000;1000,1000;1000,100)")
    r2 = r1.dup
    r2.break(4, 3.0)
    assert_equal(r2.to_s, "(0,0;0,100;1000,100;1000,0);(900,100;900,1000;1000,1000;1000,100)")

  end

  # Selection operators
  def test_4

    r = RBA::Region::new
    r.insert(RBA::Box::new(0, 0, 100, 200))
    r.insert(RBA::Box::new(0, 0, 400, 100))

    r.merged_semantics = false

    assert_equal(r.with_area(20000, false).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_area(20000, true).to_s, "(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_area(10000, nil, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_area(10000, 20000, false).to_s, "")
    assert_equal(r.with_area(nil, 20001, false).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_area(10000, 20000, true).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    
    assert_equal(r.with_perimeter(600, false).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_perimeter(600, true).to_s, "(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_perimeter(600, nil, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_perimeter(600, 1000, false).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_perimeter(nil, 1000, false).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_perimeter(600, 1000, true).to_s, "(0,0;0,100;400,100;400,0)")
    
    assert_equal(r.with_bbox_height(200, false).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_bbox_height(200, true).to_s, "(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_height(100, nil, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_height(100, 200, false).to_s, "(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_height(nil, 201, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_height(100, 200, true).to_s, "(0,0;0,200;100,200;100,0)")
    
    assert_equal(r.with_bbox_width(400, false).to_s, "(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_width(400, true).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_bbox_width(100, nil, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_width(100, 400, false).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_bbox_width(nil, 401, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_width(100, 400, true).to_s, "(0,0;0,100;400,100;400,0)")
    
    assert_equal(r.with_bbox_min(100, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_min(100, true).to_s, "")
    assert_equal(r.with_bbox_min(100, nil, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_min(100, 101, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_min(nil, 101, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_min(100, 101, true).to_s, "")
    
    assert_equal(r.with_bbox_max(200, false).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_bbox_max(200, true).to_s, "(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_max(200, nil, false).to_s, "(0,0;0,200;100,200;100,0);(0,0;0,100;400,100;400,0)")
    assert_equal(r.with_bbox_max(200, 400, false).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_bbox_max(nil, 400, false).to_s, "(0,0;0,200;100,200;100,0)")
    assert_equal(r.with_bbox_max(200, 400, true).to_s, "(0,0;0,100;400,100;400,0)")
    
  end

  # DRC
  def test_5

    r1 = RBA::Region::new
    r1.insert(RBA::Box::new(0, 0, 100, 200))
    r1.insert(RBA::Box::new(50, 200, 100, 220))
    r1.insert(RBA::Box::new(10, 220, 100, 400))
    r2 = RBA::Region::new(RBA::Box::new(120, 20, 130, 380))
    r3 = RBA::Region::new(RBA::Box::new(110, 0, 150, 400))
    r3a = RBA::Region::new(RBA::Box::new(-30, -10, -10, 10))
    r3b = RBA::Region::new(RBA::Box::new(-10, -10, 10, 10))

    assert_equal(r2.inside_check(r3, 15).to_s, "(120,20;120,380)/(110,9;110,391)")
    assert_equal(r2.inside_check(r3, 15, false, RBA::Region::Projection, nil, nil, nil).to_s, "(120,20;120,380)/(110,20;110,380)")
    assert_equal(r2.inside_check(r3, 15, true, RBA::Region::Projection, nil, nil, nil).to_s, "(120,20;120,380)/(110,0;110,400)")
    assert_equal(r2.inside_check(r3, 15, true, RBA::Region::Projection, 0.0, nil, nil).to_s, "")
    assert_equal(r2.inside_check(r3, 15, false, RBA::Region::Projection, nil, 0, 500).to_s, "(120,20;120,380)/(110,20;110,380)")
    assert_equal(r2.inside_check(r3, 15, false, RBA::Region::Projection, nil, 380, 500).to_s, "")
    assert_equal(r2.inside_check(r3, 15, false, RBA::Region::Projection, nil, 0, 300).to_s, "")
    
    assert_equal(r3.enclosing_check(r2, 15).to_s, "(110,9;110,391)/(120,20;120,380)")
    assert_equal(r3.enclosing_check(r2, 15, false, RBA::Region::Projection, nil, nil, nil).to_s, "(110,20;110,380)/(120,20;120,380)")
    assert_equal(r3.enclosing_check(r2, 15, true, RBA::Region::Projection, nil, nil, nil).to_s, "(110,0;110,400)/(120,20;120,380)")
    assert_equal(r3.enclosing_check(r2, 15, true, RBA::Region::Projection, 0.0, nil, nil).to_s, "")
    assert_equal(r3.enclosing_check(r2, 15, false, RBA::Region::Projection, nil, 0, 500).to_s, "(110,20;110,380)/(120,20;120,380)")
    assert_equal(r3.enclosing_check(r2, 15, false, RBA::Region::Projection, nil, 380, 500).to_s, "")
    assert_equal(r3.enclosing_check(r2, 15, false, RBA::Region::Projection, nil, 0, 300).to_s, "")
    
    assert_equal(r3a.separation_check(r1, 15).to_s, "(-10,10;-10,-10)/(0,0;0,21)")
    assert_equal(r3a.separation_check(r1, 15, false, RBA::Region::Projection, nil, nil, nil).to_s, "(-10,10;-10,0)/(0,0;0,10)")
    assert_equal(r3a.separation_check(r1, 15, true, RBA::Region::Projection, nil, nil, nil).to_s, "(-10,10;-10,-10)/(0,0;0,200)")
    assert_equal(r3a.separation_check(r1, 15, true, RBA::Region::Projection, 0.0, nil, nil).to_s, "")
    assert_equal(r3a.separation_check(r1, 15, false, RBA::Region::Projection, nil, 0, 500).to_s, "(-10,10;-10,0)/(0,0;0,10)")
    assert_equal(r3a.separation_check(r1, 15, false, RBA::Region::Projection, nil, 380, 500).to_s, "")
    assert_equal(r3a.separation_check(r1, 15, false, RBA::Region::Projection, nil, 0, 300).to_s, "(-10,10;-10,0)/(0,0;0,10)")
    
    assert_equal(r3b.overlap_check(r1, 15).to_s, "(-10,10;10,10)/(21,0;0,0);(10,10;10,-10)/(0,0;0,21)")
    assert_equal(r3b.overlap_check(r1, 15, false, RBA::Region::Projection, nil, nil, nil).to_s, "(0,10;10,10)/(10,0;0,0);(10,10;10,0)/(0,0;0,10)")
    assert_equal(r3b.overlap_check(r1, 15, true, RBA::Region::Projection, nil, nil, nil).to_s, "(-10,10;10,10)/(100,0;0,0);(10,10;10,-10)/(0,0;0,200)")
    assert_equal(r3b.overlap_check(r1, 15, true, RBA::Region::Projection, 0.0, nil, nil).to_s, "")
    assert_equal(r3b.overlap_check(r1, 15, false, RBA::Region::Projection, nil, 0, 500).to_s, "(0,10;10,10)/(10,0;0,0);(10,10;10,0)/(0,0;0,10)")
    assert_equal(r3b.overlap_check(r1, 15, false, RBA::Region::Projection, nil, 380, 500).to_s, "")
    assert_equal(r3b.overlap_check(r1, 15, false, RBA::Region::Projection, nil, 0, 300).to_s, "(0,10;10,10)/(10,0;0,0);(10,10;10,0)/(0,0;0,10)")
    
    assert_equal((r1 | r2).merged.isolated_check(25).to_s, "(120,20;120,380)/(100,395;100,5)")
    assert_equal((r1 | r2).merged.isolated_check(25, false, RBA::Region::Projection, nil, nil, nil).to_s, "(120,20;120,380)/(100,380;100,20)")
    assert_equal((r1 | r2).merged.isolated_check(25, false, RBA::Region::Square, nil, nil, nil).to_s, "(120,20;120,380)/(100,400;100,0)")
    assert_equal((r1 | r2).merged.isolated_check(25, false, RBA::Region::Euclidian, nil, nil, nil).to_s, "(120,20;120,380)/(100,395;100,5)")
    assert_equal((r1 | r2).merged.isolated_check(25, true, RBA::Region::Euclidian, nil, nil, nil).to_s, "(120,20;120,380)/(100,400;100,0)")
    assert_equal((r1 | r2).merged.isolated_check(25, false, RBA::Region::Euclidian, 0.0, nil, nil).to_s, "")
    assert_equal((r1 | r2).merged.isolated_check(25, false, RBA::Region::Euclidian, nil, 0, 300).to_s, "")
    assert_equal((r1 | r2).merged.isolated_check(25, false, RBA::Region::Euclidian, nil, 300, 500).to_s, "(120,20;120,380)/(100,395;100,5)")
    assert_equal((r1 | r2).merged.isolated_check(25, false, RBA::Region::Euclidian, nil, 300, nil).to_s, "(120,20;120,380)/(100,395;100,5)")

    assert_equal((r1 | r2).merged.notch_check(25).to_s, "(0,200;50,200)/(50,220;10,220)")
    assert_equal((r1 | r2).merged.notch_check(25, false, RBA::Region::Projection, nil, nil, nil).to_s, "(10,200;50,200)/(50,220;10,220)")
    assert_equal((r1 | r2).merged.notch_check(25, true, RBA::Region::Projection, nil, nil, nil).to_s, "(0,200;50,200)/(50,220;10,220)")
    assert_equal((r1 | r2).merged.notch_check(25, true, RBA::Region::Projection, 0.0, nil, nil).to_s, "")
    assert_equal((r1 | r2).merged.notch_check(25, true, RBA::Region::Projection, nil, 40, nil).to_s, "(0,200;50,200)/(50,220;10,220)")
    assert_equal((r1 | r2).merged.notch_check(25, true, RBA::Region::Projection, nil, 50, nil).to_s, "")
    assert_equal((r1 | r2).merged.notch_check(25, true, RBA::Region::Projection, nil, 40, 50).to_s, "(0,200;50,200)/(50,220;10,220)")
    assert_equal((r1 | r2).merged.notch_check(25, true, RBA::Region::Projection, nil, nil, 40).to_s, "")

    assert_equal((r1 | r2).merged.space_check(25).to_s, "(120,20;120,380)/(100,395;100,5);(0,200;50,200)/(50,220;10,220)")
    assert_equal((r1 | r2).merged.space_check(25, false, RBA::Region::Projection, nil, nil, nil).to_s, "(120,20;120,380)/(100,380;100,20);(10,200;50,200)/(50,220;10,220)")
    assert_equal((r1 | r2).merged.space_check(25, true, RBA::Region::Projection, nil, nil, nil).to_s, "(120,20;120,380)/(100,400;100,0);(0,200;50,200)/(50,220;10,220)")
    assert_equal((r1 | r2).merged.space_check(25, true, RBA::Region::Projection, 0.0, nil, nil).to_s, "")
    assert_equal((r1 | r2).merged.space_check(25, true, RBA::Region::Projection, nil, 50, nil).to_s, "(120,20;120,380)/(100,400;100,0)")
    assert_equal((r1 | r2).merged.space_check(25, true, RBA::Region::Projection, nil, nil, 50).to_s, "(0,200;50,200)/(50,220;10,220)")

    assert_equal((r1 | r2).merged.width_check(60).to_s, "(120,20;120,380)/(130,380;130,20);(50,200;50,220)/(100,253;100,167)")
    assert_equal((r1 | r2).merged.width_check(60, false, RBA::Region::Projection, nil, nil, nil).to_s, "(120,20;120,380)/(130,380;130,20);(50,200;50,220)/(100,220;100,200)")
    assert_equal((r1 | r2).merged.width_check(60, true, RBA::Region::Projection, nil, nil, nil).to_s, "(120,20;120,380)/(130,380;130,20);(50,200;50,220)/(100,400;100,0)")
    assert_equal((r1 | r2).merged.width_check(60, true, RBA::Region::Projection, 0.0, nil, nil).to_s, "")
    assert_equal((r1 | r2).merged.width_check(60, true, RBA::Region::Projection, nil, 50, nil).to_s, "(120,20;120,380)/(130,380;130,20)")
    assert_equal((r1 | r2).merged.width_check(60, true, RBA::Region::Projection, nil, nil, 50).to_s, "(50,200;50,220)/(100,400;100,0)")

  end

  # Others
  def test_6

    r1 = RBA::Region::new
    r1.insert(RBA::Box::new(10, 20, 100, 200))
    r1.insert(RBA::Box::new(50, 70, 150, 270))
    r1.insert(RBA::Box::new(100, 70, 250, 270))
    r2 = RBA::Region::new(RBA::Box::new(-10, -20, 100, 200))

    assert_equal(r1.merged_semantics?, true)
    r1.merged_semantics = false
    assert_equal(r1.merged_semantics?, false)

    assert_equal(r1.inside(r2).to_s, "(10,20;10,200;100,200;100,20)")
    assert_equal(r1.not_inside(r2).to_s, "(50,70;50,270;150,270;150,70);(100,70;100,270;250,270;250,70)")
    assert_equal(r1.interacting(r2).to_s, "(10,20;10,200;100,200;100,20);(50,70;50,270;150,270;150,70);(100,70;100,270;250,270;250,70)")
    assert_equal(r1.not_interacting(r2).to_s, "")
    assert_equal(r1.overlapping(r2).to_s, "(10,20;10,200;100,200;100,20);(50,70;50,270;150,270;150,70)")
    assert_equal(r1.not_overlapping(r2).to_s, "(100,70;100,270;250,270;250,70)")
    assert_equal(r1.outside(r2).to_s, "(100,70;100,270;250,270;250,70)")
    assert_equal(r1.not_outside(r2).to_s, "(10,20;10,200;100,200;100,20);(50,70;50,270;150,270;150,70)")

    rr = r1.dup
    rr.select_interacting(r2)
    assert_equal(rr.to_s, "(10,20;10,200;100,200;100,20);(50,70;50,270;150,270;150,70);(100,70;100,270;250,270;250,70)")
    
    rr = r1.dup
    rr.select_not_interacting(r2)
    assert_equal(rr.to_s, "")
    
    rr = r1.dup
    rr.select_overlapping(r2)
    assert_equal(rr.to_s, "(10,20;10,200;100,200;100,20);(50,70;50,270;150,270;150,70)")
    
    rr = r1.dup
    rr.select_not_overlapping(r2)
    assert_equal(rr.to_s, "(100,70;100,270;250,270;250,70)")
    
    rr = r1.dup
    rr.select_outside(r2)
    assert_equal(rr.to_s, "(100,70;100,270;250,270;250,70)")
    
    rr = r1.dup
    rr.select_not_outside(r2)
    assert_equal(rr.to_s, "(10,20;10,200;100,200;100,20);(50,70;50,270;150,270;150,70)")
    
    rr = r1.dup
    rr.select_inside(r2)
    assert_equal(rr.to_s, "(10,20;10,200;100,200;100,20)")
    
    rr = r1.dup
    rr.select_not_inside(r2)
    assert_equal(rr.to_s, "(50,70;50,270;150,270;150,70);(100,70;100,270;250,270;250,70)")
    
    r1.merged_semantics = true

    assert_equal(r1.inside(r2).to_s, "")
    assert_equal(r1.interacting(r2).to_s, "(10,20;10,200;50,200;50,270;250,270;250,70;100,70;100,20)")
    assert_equal(r1.overlapping(r2).to_s, "(10,20;10,200;50,200;50,270;250,270;250,70;100,70;100,20)")
    assert_equal(r1.outside(r2).to_s, "")

    rr = r1.dup
    rr.select_interacting(r2)
    assert_equal(rr.to_s, "(10,20;10,200;50,200;50,270;250,270;250,70;100,70;100,20)")
    
    rr = r1.dup
    rr.select_overlapping(r2)
    assert_equal(rr.to_s, "(10,20;10,200;50,200;50,270;250,270;250,70;100,70;100,20)")
    
    rr = r1.dup
    rr.select_outside(r2)
    assert_equal(rr.to_s, "")
    
    rr = r1.dup
    rr.select_inside(r2)
    assert_equal(rr.to_s, "")
    
  end

  # Selections
  def test_7

    r1 = RBA::Region::new
    r1.insert(RBA::Box::new(10, 20, 100, 200))
    r1.insert(RBA::Box::new(50, 70, 150, 270))
    r2 = RBA::Region::new
    r2.insert(RBA::Box::new(100, 70, 250, 270))
    r2.insert(RBA::Box::new(10, 20, 100, 200))

    r1.merged_semantics = false
    r2.merged_semantics = false

    assert_equal(r1.in(r2).to_s, "(10,20;10,200;100,200;100,20)")
    assert_equal(r2.in(r1).to_s, "(10,20;10,200;100,200;100,20)")
    assert_equal(r1.not_in(r2).to_s, "(50,70;50,270;150,270;150,70)")
    assert_equal(r2.not_in(r1).to_s, "(100,70;100,270;250,270;250,70)")

    r1.merged_semantics = true
    r2.merged_semantics = true

    assert_equal(r1.in(r2).to_s, "")
    assert_equal(r2.in(r1).to_s, "")
    assert_equal(r1.not_in(r2).to_s, "(10,20;10,200;50,200;50,270;150,270;150,70;100,70;100,20)")
    assert_equal(r2.not_in(r1).to_s, "(10,20;10,200;100,200;100,270;250,270;250,70;100,70;100,20)")

    r1.insert(RBA::Polygon::new([ RBA::Point::new(0, 0), RBA::Point::new(100, 100), RBA::Point::new(100, 0) ]))
    r1.insert(RBA::Polygon::new([ RBA::Point::new(0, 0), RBA::Point::new(0, 100), RBA::Point::new(50, 100), RBA::Point::new(50, 200), RBA::Point::new(100, 200), RBA::Point::new(100, 0) ]))

    r1.merged_semantics = false
    assert_equal(r1.rectangles.to_s, "(10,20;10,200;100,200;100,20);(50,70;50,270;150,270;150,70)")
    assert_equal(r1.non_rectangles.to_s, "(0,0;100,100;100,0);(0,0;0,100;50,100;50,200;100,200;100,0)")
    assert_equal(r1.rectilinear.to_s, "(10,20;10,200;100,200;100,20);(50,70;50,270;150,270;150,70);(0,0;0,100;50,100;50,200;100,200;100,0)")
    assert_equal(r1.non_rectilinear.to_s, "(0,0;100,100;100,0)")

    r1.merged_semantics = true
    assert_equal(r1.rectangles.to_s, "")
    assert_equal(r1.non_rectangles.to_s, "(0,0;0,100;10,100;10,200;50,200;50,270;150,270;150,70;100,70;100,0)")
    assert_equal(r1.rectilinear.to_s, "(0,0;0,100;10,100;10,200;50,200;50,270;150,270;150,70;100,70;100,0)")
    assert_equal(r1.non_rectilinear.to_s, "")

  end

  # strange polygon check
  def test_8

    r = RBA::Region::new
    assert_equal(r.strange_polygon_check.to_s, "")

    pts = [ 
      RBA::Point::new(0, 0),
      RBA::Point::new(0, 1000),
      RBA::Point::new(1000, 1000),
      RBA::Point::new(1000, 500),
      RBA::Point::new(500, 500),
      RBA::Point::new(500, 600),
      RBA::Point::new(600, 600),
      RBA::Point::new(600, 0)
    ]
    r.insert(RBA::Polygon::new(pts))
    pts = [ 
      RBA::Point::new(2000, 0),
      RBA::Point::new(2000, 500),
      RBA::Point::new(3000, 500),
      RBA::Point::new(3000, 1000),
      RBA::Point::new(2500, 1000),
      RBA::Point::new(2500, 0)
    ]
    r.insert(RBA::Polygon::new(pts))

    assert_equal(r.strange_polygon_check.to_s, "(500,500;500,600;600,600;600,500);(2500,500;2500,1000;3000,1000;3000,500)")
    r.merge 
    assert_equal(r.strange_polygon_check.to_s, "")

  end

  # angle check
  def test_9a

    r = RBA::Region::new
    assert_equal(r.with_angle(0, 180.0, false).to_s, "")
    assert_equal(r.with_angle(0, 180.0, true).to_s, "")

    pts = [
      RBA::Point::new(0, 0),
      RBA::Point::new(0, 1000),
      RBA::Point::new(1000, 2000),
      RBA::Point::new(1000, 0)
    ]

    r.insert(RBA::Polygon::new(pts))

    assert_equal(r.with_angle(0, 180.0, false).to_s, "(0,0;0,1000)/(0,1000;1000,2000);(0,1000;1000,2000)/(1000,2000;1000,0);(1000,2000;1000,0)/(1000,0;0,0);(1000,0;0,0)/(0,0;0,1000)")
    assert_equal(r.with_angle(0, 180.0, true).to_s, "")
    assert_equal(r.with_angle(45.0, 45.1, false).to_s, "(0,1000;1000,2000)/(1000,2000;1000,0)")
    assert_equal(r.with_angle(45.0, false).to_s, "(0,1000;1000,2000)/(1000,2000;1000,0)")
    assert_equal(r.with_angle(0.0, 90.0, false).to_s, "(0,1000;1000,2000)/(1000,2000;1000,0)")
    assert_equal(r.with_angle(0.0, 90.0, true).to_s, "(0,0;0,1000)/(0,1000;1000,2000);(1000,2000;1000,0)/(1000,0;0,0);(1000,0;0,0)/(0,0;0,1000)")
    assert_equal(r.with_angle(90.1, 180.0, false).to_s, "(0,0;0,1000)/(0,1000;1000,2000)")
    assert_equal(r.with_angle(90.1, 180.0, true).to_s, "(0,1000;1000,2000)/(1000,2000;1000,0);(1000,2000;1000,0)/(1000,0;0,0);(1000,0;0,0)/(0,0;0,1000)")
  
  end

  # grid check
  def test_9b

    r = RBA::Region::new;
    assert_equal(r.grid_check(10, 20).to_s, "")

    r.insert(RBA::Box::new(0, 0, 1000, 100))
    r.insert(RBA::Box::new(0, 100, 105, 300))
    r.insert(RBA::Box::new(910, 100, 1000, 300))
    r.insert(RBA::Box::new(0, 290, 1000, 500))

    assert_equal(r.grid_check(0, 0).to_s, "")
    assert_equal(r.grid_check(5, 0).to_s, "")
    assert_equal(r.grid_check(0, 10).to_s, "")
    assert_equal(r.grid_check(10, 10).to_s, "(105,100;105,100)/(105,100;105,100);(105,290;105,290)/(105,290;105,290)")
    assert_equal(r.grid_check(10, 20).to_s, "(105,100;105,100)/(105,100;105,100);(910,290;910,290)/(910,290;910,290);(105,290;105,290)/(105,290;105,290)")

  end

  # snap
  def test_10

    r = RBA::Region::new;
    assert_equal(r.snapped(10, 20).to_s, "")

    r.insert(RBA::Box::new(0, 0, 1000, 100))
    r.insert(RBA::Box::new(0, 100, 105, 300))
    r.insert(RBA::Box::new(910, 100, 1000, 300))
    r.insert(RBA::Box::new(0, 290, 1000, 500))

    assert_equal(r.snapped(0, 0).to_s, "(0,0;0,500;1000,500;1000,0/105,100;910,100;910,290;105,290)")
    assert_equal(r.snapped(5, 0).to_s, "(0,0;0,500;1000,500;1000,0/105,100;910,100;910,290;105,290)")
    assert_equal(r.snapped(0, 10).to_s, "(0,0;0,500;1000,500;1000,0/105,100;910,100;910,290;105,290)")
    assert_equal(r.snapped(10, 10).to_s, "(0,0;0,500;1000,500;1000,0/110,100;910,100;910,290;110,290)")
    assert_equal(r.snapped(10, 20).to_s, "(0,0;0,500;1000,500;1000,0/110,100;910,100;910,300;110,300)")
    r.snap(10, 20)
    assert_equal(r.to_s, "(0,0;0,500;1000,500;1000,0/110,100;910,100;910,300;110,300)")

  end

  # corner rounding
  def test_11

    r = RBA::Region::new
    r.insert(RBA::Box::new(0, 0, 500, 1000))
    r.insert(RBA::Box::new(0, -500, 1000, 0))

    assert_equal(r.rounded_corners(50, 100, 16).to_s, "(80,-500;43,-485;15,-457;0,-420;0,920;15,957;43,985;80,1000;420,1000;457,985;485,957;500,920;500,40;508,22;522,8;540,0;920,0;957,-15;985,-43;1000,-80;1000,-420;985,-457;957,-485;920,-500)")
    rr = r.dup
    rr.round_corners(50, 100, 16)
    assert_equal(rr.to_s, r.rounded_corners(50, 100, 16).to_s)

  end

  # strict handling
  def test_12

    r = RBA::Region::new
    r.insert(RBA::Box::new(0, 0, 500, 1000))
    r.insert(RBA::Box::new(200, 300, 600, 1200))

    r2 = RBA::Region::new;

    assert_equal((r - r2).to_s, "(0,0;0,1000;500,1000;500,0);(200,300;200,1200;600,1200;600,300)")
    assert_equal(r.strict_handling?, false)

    r.strict_handling = true
    assert_equal(r.strict_handling?, true)
    assert_equal((r - r2).to_s, "(0,0;0,1000;200,1000;200,1200;600,1200;600,300;500,300;500,0)")

  end

  # region decomposition
  def test_13

    r = RBA::Region::new
    p = RBA::Polygon::from_s("(0,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    r.insert(p)
    p = RBA::Polygon::from_s("(0,40;0,50;10,50;10,40;0,40)")
    r.insert(p)

    assert_equal(RBA::Region::new(r.decompose_convex).to_s, "(0,10;0,30;10,30;10,10);(10,30;10,40;40,40;40,30);(0,30;0,50;10,50;10,30);(30,0;30,30;40,30;40,0);(0,0;0,10;30,10;30,0)")
    assert_equal(r.decompose_convex_to_region.to_s, "(0,10;0,30;10,30;10,10);(10,30;10,40;40,40;40,30);(0,30;0,50;10,50;10,30);(30,0;30,30;40,30;40,0);(0,0;0,10;30,10;30,0)")
    assert_equal(RBA::Region::new(r.decompose_convex(RBA::Polygon::PO_horizontal)).to_s, "(0,10;0,30;10,30;10,10);(0,30;0,40;40,40;40,30);(0,40;0,50;10,50;10,40);(30,10;30,30;40,30;40,10);(0,0;0,10;40,10;40,0)")
    assert_equal(RBA::Region::new(r.decompose_trapezoids).to_s, "(0,0;0,10;40,10;40,0);(0,10;0,30;10,30;10,10);(30,10;30,30;40,30;40,10);(0,30;0,40;40,40;40,30);(0,40;0,50;10,50;10,40)")
    assert_equal(r.decompose_trapezoids_to_region.to_s, "(0,0;0,10;40,10;40,0);(0,10;0,30;10,30;10,10);(30,10;30,30;40,30;40,10);(0,30;0,40;40,40;40,30);(0,40;0,50;10,50;10,40)")
    assert_equal(RBA::Region::new(r.decompose_trapezoids(RBA::Polygon::TD_htrapezoids)).to_s, "(0,10;0,30;10,30;10,10);(10,30;10,40;40,40;40,30);(0,30;0,50;10,50;10,30);(30,0;30,30;40,30;40,0);(0,0;0,10;30,10;30,0)")

  end

  # corners functions
  def test_14

    r = RBA::Region::new
    p = RBA::Polygon::from_s("(0,0;0,80;40,40;40,0/10,10;30,10;30,30;10,30)")
    r.insert(p)
    
    assert_equal(r.corners.to_s, "(39,-1;39,1;41,1;41,-1);(-1,-1;-1,1;1,1;1,-1);(-1,79;-1,81;1,81;1,79);(39,39;39,41;41,41;41,39);(9,29;9,31;11,31;11,29);(9,9;9,11;11,11;11,9);(29,9;29,11;31,11;31,9);(29,29;29,31;31,31;31,29)")
    assert_equal(r.corners(-90.0, 90.0).to_s, "(39,-1;39,1;41,1;41,-1);(-1,-1;-1,1;1,1;1,-1);(39,39;39,41;41,41;41,39);(9,29;9,31;11,31;11,29);(9,9;9,11;11,11;11,9);(29,9;29,11;31,11;31,9);(29,29;29,31;31,31;31,29)")
    assert_equal(r.corners(-90.0, -90.0).to_s, "(39,-1;39,1;41,1;41,-1);(-1,-1;-1,1;1,1;1,-1)")
    assert_equal(r.corners(-45.0, -45.0).to_s, "(39,39;39,41;41,41;41,39)")
    assert_equal(r.corners(-90.0, -45.0).to_s, "(39,-1;39,1;41,1;41,-1);(-1,-1;-1,1;1,1;1,-1);(39,39;39,41;41,41;41,39)")
    assert_equal(r.corners(90.0, 90.0).to_s, "(9,29;9,31;11,31;11,29);(9,9;9,11;11,11;11,9);(29,9;29,11;31,11;31,9);(29,29;29,31;31,31;31,29)")

  end

  # texts
  def test_15

    r = RBA::Region::new
    t = r.texts("*", true)
    assert_equal(t.to_s, "")

    r.insert(RBA::Box::new(1, 2, 3, 4))
    t = r.texts("*", true)
    assert_equal(t.to_s, "")

    ly = RBA::Layout::new
    top = ly.create_cell("TOP")
    l1 = ly.layer(1, 0)
    top.shapes(l1).insert(RBA::Text::new("ABC", RBA::Trans::new(RBA::Vector::new(100, 200))))

    r = RBA::Region::new(top.begin_shapes_rec(l1))
    t = r.texts("*", true)
    assert_equal(t.to_s, "(99,199;99,201;101,201;101,199)")
    assert_equal(t.is_deep?, false)
    t = r.texts("*", true, 10)
    assert_equal(t.to_s, "(90,190;90,210;110,210;110,190)")
    t = r.texts("A*", true, 10)
    assert_equal(t.to_s, "(90,190;90,210;110,210;110,190)")
    t = r.texts("A*", false, 10)
    assert_equal(t.to_s, "")
    t = r.texts("ABC", false, 10)
    assert_equal(t.to_s, "(90,190;90,210;110,210;110,190)")

    dss = RBA::DeepShapeStore::new
    r = RBA::Region::new(top.begin_shapes_rec(l1))
    t = r.texts(dss, "*", true)
    assert_equal(t.to_s, "(99,199;99,201;101,201;101,199)")
    assert_equal(t.is_deep?, true)

    r = RBA::Region::new(top.begin_shapes_rec(l1))
    t = r.texts_dots("*", true)
    assert_equal(t.to_s, "(100,200;100,200)")
    assert_equal(t.is_deep?, false)

    dss = RBA::DeepShapeStore::new
    r = RBA::Region::new(top.begin_shapes_rec(l1))
    t = r.texts_dots(dss, "A*", true)
    assert_equal(t.to_s, "(100,200;100,200)")
    assert_equal(t.is_deep?, true)

  end

  # deep region tests
  def test_deep1

    # construction/destruction magic ...
    GC.start
    assert_equal(RBA::DeepShapeStore::instance_count, 0)
    dss = RBA::DeepShapeStore::new
    dss._create
    assert_equal(RBA::DeepShapeStore::instance_count, 1)
    # do a little testing on the DSS:
    dss.max_vertex_count = 8
    assert_equal(dss.max_vertex_count, 8)
    dss.max_area_ratio = 42.0
    assert_equal(dss.max_area_ratio, 42.0)
    dss.threads = 3
    assert_equal(dss.threads, 3)
    dss = nil
    GC.start
    assert_equal(RBA::DeepShapeStore::instance_count, 0)

    dss = RBA::DeepShapeStore::new
    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "algo", "deep_region_l1.gds"))
    l1 = ly.layer(1, 0)
    r = RBA::Region::new(ly.top_cell.begin_shapes_rec(l1), dss)
    rf = RBA::Region::new(ly.top_cell.begin_shapes_rec(l1))

    assert_equal(r.is_deep?, true)
    assert_equal(r.area, 53120000)
    assert_equal(rf.area, 53120000)

    ly_new = RBA::Layout::new
    tc = ly_new.add_cell("TOP")
    l1 = ly_new.layer(1, 0)
    l2 = ly_new.layer(2, 0)
    ly_new.insert(tc, l1, r)
    ly_new.insert(tc, l2, rf)

    s1 = { }
    s2 = { }
    ly_new.each_cell do |cell|
      s1[cell.name] = cell.shapes(l1).size
      s2[cell.name] = cell.shapes(l2).size
    end
    assert_equal(s1, {"INV2"=>1, "TOP"=>0, "TRANS"=>0})
    assert_equal(s2, {"INV2"=>0, "TOP"=>10, "TRANS"=>0})

    target = RBA::Layout::new
    target_top = target.add_cell("TOP")
    target_li = target.layer
    r.insert_into(target, target_top, target_li)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,INV2,TRANS")
    assert_equal(RBA::Region::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Region::new(target.cell("INV2").shapes(target_li)).to_s, "(-1400,1800;-1400,3800;1400,3800;1400,1800)")
    assert_equal(RBA::Region::new(target.cell("TRANS").shapes(target_li)).to_s, "")

    r.flatten
    assert_equal(r.is_deep?, false)
    assert_equal(r.area, 53120000)

    # force destroy, so the unit tests pass on the next iteration
    dss._destroy
    assert_equal(RBA::DeepShapeStore::instance_count, 0)

  end

end

load("test_epilogue.rb")

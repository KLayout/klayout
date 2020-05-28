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

class DBTexts_TestClass < TestBase

  # Basics
  def test_1

    r = RBA::Texts::new
    assert_equal(r.to_s, "")
    assert_equal(r.is_empty?, true)
    assert_equal(r.size, 0)
    assert_equal(r.bbox.to_s, "()")
    data_id = r.data_id

    r.insert(RBA::Text::new("abc", RBA::Trans::new(RBA::Vector::new(100, -200))))
    assert_equal(data_id != r.data_id, true)
    assert_equal(r.to_s, "('abc',r0 100,-200)")
    r.clear

    r.insert(RBA::Text::new("uvw", RBA::Trans::new(RBA::Vector::new(110, 210))))
    assert_equal(r.to_s, "('uvw',r0 110,210)")
    assert_equal(r.extents.to_s, "(109,209;109,211;111,211;111,209)")
    assert_equal(r.extents(10).to_s, "(100,200;100,220;120,220;120,200)")
    assert_equal(r.extents(5, 10).to_s, "(105,200;105,220;115,220;115,200)")
    assert_equal(r.edges.to_s, "(110,210;110,210)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.size, 1)
    assert_equal(r[0].to_s, "('uvw',r0 110,210)")
    assert_equal(r[1].to_s, "")
    assert_equal(r.bbox.to_s, "(110,210;110,210)")

    assert_equal(r.moved(-10, 10).to_s, "('uvw',r0 100,220)")
    assert_equal(r.moved(RBA::Point::new(-10, 10)).to_s, "('uvw',r0 100,220)")
    rr = r.dup
    assert_equal(rr.data_id != r.data_id, true)
    rr.move(-10, 10)
    assert_equal(rr.to_s, "('uvw',r0 100,220)")
    rr = r.dup
    rr.move(RBA::Point::new(-10, 10))
    assert_equal(rr.to_s, "('uvw',r0 100,220)")

    assert_equal(r.transformed(RBA::Trans::new(1)).to_s, "('uvw',r90 -210,110)")
    assert_equal(r.transformed(RBA::ICplxTrans::new(2.0)).to_s, "('uvw',r0 220,420)")
    rr = r.dup
    rr.transform(RBA::Trans::new(1))
    assert_equal(rr.to_s, "('uvw',r90 -210,110)")
    rr = r.dup
    rr.transform(RBA::ICplxTrans::new(2.0))
    assert_equal(rr.to_s, "('uvw',r0 220,420)")

    rr = RBA::Texts::new
    rr.swap(r)
    assert_equal(rr.to_s, "('uvw',r0 110,210)")
    assert_equal(r.to_s, "")
    rr.swap(r)
    assert_equal(r.to_s, "('uvw',r0 110,210)")
    r.clear

    assert_equal(r.to_s, "")
    assert_equal(r.is_empty?, true)
    assert_equal(r.size, 0)
    assert_equal(r.bbox.to_s, "()")

    texts = RBA::Texts::new
    t = RBA::Texts::new
    t.insert(RBA::Text::new("uvw", RBA::Trans::new(RBA::Vector::new(-110, 210))))
    texts.insert(t)
    assert_equal(texts.to_s, "('uvw',r0 -110,210)")

  end

  # Basics
  def test_2

    r1 = RBA::Texts::new
    r1.insert(RBA::Text::new("abc", RBA::Trans::new(RBA::Vector::new(100, -200))))
    r1.insert(RBA::Text::new("uvm", RBA::Trans::new(RBA::Vector::new(110, 210))))

    r2 = RBA::Texts::new
    r1.insert(RBA::Text::new("abc", RBA::Trans::new(RBA::Vector::new(101, -201))))
    r1.insert(RBA::Text::new("uvm", RBA::Trans::new(RBA::Vector::new(111, 211))))

    assert_equal((r1 + r2).to_s, "('abc',r0 100,-200);('uvm',r0 110,210);('abc',r0 101,-201);('uvm',r0 111,211)")
    r1 += r2
    assert_equal(r1.to_s, "('abc',r0 100,-200);('uvm',r0 110,210);('abc',r0 101,-201);('uvm',r0 111,211)")

  end

  def test_3

    text1 = RBA::Text::new("abc", RBA::Trans::new(RBA::Vector::new(100, -200)))
    text2 = RBA::Text::new("uvm", RBA::Trans::new(RBA::Vector::new(110, 210)))
    text3 = RBA::Text::new("xyz", RBA::Trans::new(RBA::Vector::new(-101, 201)))

    r1 = RBA::Texts::new([ text1, text2 ])
    assert_equal(r1.to_s, "('abc',r0 100,-200);('uvm',r0 110,210)")
    assert_equal(r1.with_text("abc", false).to_s, "('abc',r0 100,-200)")
    assert_equal(r1.with_text("abc", true).to_s, "('uvm',r0 110,210)")
    assert_equal(r1.with_match("*b*", false).to_s, "('abc',r0 100,-200)")
    assert_equal(r1.with_match("*b*", true).to_s, "('uvm',r0 110,210)")

    r1 = RBA::Texts::new(text1)
    assert_equal(r1.to_s, "('abc',r0 100,-200)")

    s = RBA::Shapes::new
    s.insert(text1)
    s.insert(text2)
    r1 = RBA::Texts::new(s)
    assert_equal(r1.to_s, "('abc',r0 100,-200);('uvm',r0 110,210)")

    ly = RBA::Layout::new
    l1 = ly.layer("l1")
    l2 = ly.layer("l2")
    l3 = ly.layer("l3")
    c1 = ly.create_cell("C1")
    c2 = ly.create_cell("C2")
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 0)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 100)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(200, 100)))
    c2.shapes(l1).insert(text1)
    c2.shapes(l2).insert(text2)
    c2.shapes(l3).insert(text3)
    
    r = RBA::Texts::new(ly.begin_shapes(c1.cell_index, l1))
    assert_equal(r.to_s(30), "('abc',r0 100,-200);('abc',r0 100,-100);('abc',r0 300,-100)")
    assert_equal(r.to_s(2), "('abc',r0 100,-200);('abc',r0 100,-100)...")
    assert_equal(r.is_empty?, false)
    assert_equal(r.size, 3)

    assert_equal(r.has_valid_texts?, false)
    assert_equal(r.bbox.to_s, "(100,-200;300,-100)")

    assert_equal(r.is_deep?, false)

    r.flatten
    assert_equal(r.has_valid_texts?, true)
    assert_equal(r[1].to_s, "('abc',r0 100,-100)")
    assert_equal(r[100].inspect, "nil")
    assert_equal(r.bbox.to_s, "(100,-200;300,-100)")
    
    dss = RBA::DeepShapeStore::new
    r = RBA::Texts::new(ly.begin_shapes(c1.cell_index, l1), dss)
    assert_equal(r.to_s(30), "('abc',r0 100,-200);('abc',r0 100,-100);('abc',r0 300,-100)")
    assert_equal(r.to_s(2), "('abc',r0 100,-200);('abc',r0 100,-100)...")
    assert_equal(r.is_empty?, false)
    assert_equal(r.size, 3)

    assert_equal(r.has_valid_texts?, false)
    assert_equal(r.bbox.to_s, "(100,-200;300,-100)")

    assert_equal(r.is_deep?, true)

    r.flatten
    assert_equal(r.has_valid_texts?, true)
    assert_equal(r[1].to_s, "('abc',r0 100,-100)")
    assert_equal(r[100].inspect, "nil")
    assert_equal(r.bbox.to_s, "(100,-200;300,-100)")

    assert_equal(r.is_deep?, false)

  end

  def test_4

    # insert_into and insert_into_as_polygons

    text1 = RBA::Text::new("abc", RBA::Trans::new(RBA::Vector::new(100, -200)))

    ly = RBA::Layout::new
    l1 = ly.layer("l1")
    c1 = ly.create_cell("C1")
    c2 = ly.create_cell("C2")
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 0)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(0, 100)))
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(200, 100)))
    c2.shapes(l1).insert(text1)

    dss = RBA::DeepShapeStore::new
    r = RBA::Texts::new(ly.begin_shapes(c1.cell_index, l1), dss)

    target = RBA::Layout::new
    target_top = target.add_cell("TOP")
    target_li = target.layer
    r.insert_into(target, target_top, target_li)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Texts::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Texts::new(target.cell("C2").shapes(target_li)).to_s, "('abc',r0 100,-200)")

    target = RBA::Layout::new
    target_top = target.add_cell("TOP")
    target_li = target.layer
    r.with_text("abc", false).insert_into(target, target_top, target_li)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Texts::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Texts::new(target.cell("C2").shapes(target_li)).to_s, "('abc',r0 100,-200)")

    target_li = target.layer
    r.with_text("abd", true).insert_into(target, target_top, target_li)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Texts::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Texts::new(target.cell("C2").shapes(target_li)).to_s, "('abc',r0 100,-200)")

    target_li = target.layer
    r.with_text("abc", true).insert_into(target, target_top, target_li)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Texts::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Texts::new(target.cell("C2").shapes(target_li)).to_s, "")

    target_li = target.layer
    r.with_match("*b*", false).insert_into(target, target_top, target_li)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Texts::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Texts::new(target.cell("C2").shapes(target_li)).to_s, "('abc',r0 100,-200)")

    target_li = target.layer
    r.with_match("*bb*", true).insert_into(target, target_top, target_li)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Texts::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Texts::new(target.cell("C2").shapes(target_li)).to_s, "('abc',r0 100,-200)")

    target_li = target.layer
    r.with_match("*b*", true).insert_into(target, target_top, target_li)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Texts::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Texts::new(target.cell("C2").shapes(target_li)).to_s, "")

    target = RBA::Layout::new
    target_top = target.add_cell("TOP")
    target_li = target.layer
    r.insert_into_as_polygons(target, target_top, target_li, 1)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Region::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Region::new(target.cell("C2").shapes(target_li)).to_s, "(99,-201;99,-199;101,-199;101,-201)")

    target_li = target.layer
    target.insert(target_top, target_li, r)
    cells = []
    target.each_cell { |c| cells << c.name }
    assert_equal(cells.join(","), "TOP,C2")
    assert_equal(RBA::Texts::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Region::new(target.cell("TOP").shapes(target_li)).to_s, "")
    assert_equal(RBA::Texts::new(target.cell("C2").shapes(target_li)).to_s, "('abc',r0 100,-200)")
    assert_equal(RBA::Region::new(target.cell("C2").shapes(target_li)).to_s, "")

  end

  # interact
  def test_5

    r = RBA::Texts::new
    r.insert(RBA::Text::new("abc", RBA::Trans::new(RBA::Vector::new(100, 200))))
    r.insert(RBA::Text::new("uvw", RBA::Trans::new(RBA::Vector::new(110, -210))))
    g2 = RBA::Region::new
    g2.insert(RBA::Box::new(0, 100, 200, 200))
    g2.insert(RBA::Box::new(-200, 100, -100, 200))

    assert_equal(r.interacting(g2).to_s, "('abc',r0 100,200)")
    assert_equal(g2.interacting(r).to_s, "(0,100;0,200;200,200;200,100)")
    assert_equal(r.not_interacting(g2).to_s, "('uvw',r0 110,-210)")
    assert_equal(g2.not_interacting(r).to_s, "(-200,100;-200,200;-100,200;-100,100)")
    rr = r.dup
    rr.select_interacting(g2)
    assert_equal(rr.to_s, "('abc',r0 100,200)")
    rr = r.dup
    rr.select_not_interacting(g2)
    assert_equal(rr.to_s, "('uvw',r0 110,-210)")

    assert_equal(r.pull_interacting(g2).to_s, "(0,100;0,200;200,200;200,100)")
    assert_equal(g2.pull_interacting(r).to_s, "('abc',r0 100,200)")

  end

end


load("test_epilogue.rb")

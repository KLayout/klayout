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
 
class TextStringLengthFilter < RBA::TextFilter

  # Constructor
  def initialize(string_length)
    self.is_isotropic_and_scale_invariant   # orientation and scale do not matter
    @string_length = string_length
  end
  
  # Select texts with given string length
  def selected(text)
    return text.string.size == @string_length
  end

end

class ReplaceTextString < RBA::TextOperator

  # Constructor
  def initialize
    self.is_isotropic_and_scale_invariant   # orientation and scale do not matter
  end
  
  # Replaces the string by a number representing the string length
  def process(text)
    new_text = text.dup   # need a copy as we cannot modify the text passed
    new_text.string = text.string.size.to_s
    return [ new_text ]
  end

end

class SomeTextToPolygonOperator < RBA::TextToPolygonOperator

  # Constructor
  def initialize
    self.is_isotropic_and_scale_invariant   # orientation and scale do not matter
  end
  
  # Replaces the string by a number representing the string length
  def process(text)
    s = text.string.size * 10
    return [ RBA::Polygon::new(text.bbox.enlarged(s)) ]
  end

end

class DBTexts_TestClass < TestBase

  # Basics
  def test_1

    r = RBA::Texts::new
    assert_equal(r.to_s, "")
    assert_equal(r.is_empty?, true)
    assert_equal(r.count, 0)
    assert_equal(r.hier_count, 0)
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
    assert_equal(r.count, 1)
    assert_equal(r.hier_count, 1)
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
    assert_equal(r.count, 0)
    assert_equal(r.hier_count, 0)
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

    assert_equal(csort((r1 + r2).to_s), csort("('abc',r0 100,-200);('uvm',r0 110,210);('abc',r0 101,-201);('uvm',r0 111,211)"))
    assert_equal(csort((r1.join(r2)).to_s), csort("('abc',r0 100,-200);('uvm',r0 110,210);('abc',r0 101,-201);('uvm',r0 111,211)"))
    rr1 = r1.dup
    rr1 += r2
    assert_equal(csort(r1.to_s), csort("('abc',r0 100,-200);('uvm',r0 110,210);('abc',r0 101,-201);('uvm',r0 111,211)"))
    rr1 = r1.dup
    rr1.join_with(r2)
    assert_equal(csort(r1.to_s), csort("('abc',r0 100,-200);('uvm',r0 110,210);('abc',r0 101,-201);('uvm',r0 111,211)"))

  end

  def test_3

    text1 = RBA::Text::new("abc", RBA::Trans::new(RBA::Vector::new(100, -200)))
    text2 = RBA::Text::new("uvm", RBA::Trans::new(RBA::Vector::new(110, 210)))
    text3 = RBA::Text::new("xyz", RBA::Trans::new(RBA::Vector::new(-101, 201)))

    r1 = RBA::Texts::new([ text1, text2 ])
    assert_equal(csort(r1.to_s), csort("('abc',r0 100,-200);('uvm',r0 110,210)"))
    assert_equal(r1.with_text("abc", false).to_s, "('abc',r0 100,-200)")
    assert_equal(r1.split_with_text("abc")[0].to_s, "('abc',r0 100,-200)")
    assert_equal(r1.with_text("abc", true).to_s, "('uvm',r0 110,210)")
    assert_equal(r1.split_with_text("abc")[1].to_s, "('uvm',r0 110,210)")
    assert_equal(r1.with_match("*b*", false).to_s, "('abc',r0 100,-200)")
    assert_equal(r1.split_with_match("*b*")[0].to_s, "('abc',r0 100,-200)")
    assert_equal(r1.with_match("*b*", true).to_s, "('uvm',r0 110,210)")
    assert_equal(r1.split_with_match("*b*")[1].to_s, "('uvm',r0 110,210)")

    r1 = RBA::Texts::new(text1)
    assert_equal(r1.to_s, "('abc',r0 100,-200)")

    s = RBA::Shapes::new
    s.insert(text1)
    s.insert(text2)
    r1 = RBA::Texts::new(s)
    assert_equal(csort(r1.to_s), csort("('abc',r0 100,-200);('uvm',r0 110,210)"))

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
    assert_equal(r.count, 3)
    assert_equal(r.hier_count, 3)

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
    assert_equal(r.count, 3)
    assert_equal(r.hier_count, 1)

    assert_equal(r.has_valid_texts?, false)
    assert_equal(r.bbox.to_s, "(100,-200;300,-100)")

    assert_equal(r.is_deep?, true)

    r.flatten
    assert_equal(r.to_s, "('abc',r0 100,-200);('abc',r0 100,-100);('abc',r0 300,-100)")
    assert_equal(r.bbox.to_s, "(100,-200;300,-100)")

    assert_equal(r.is_deep?, true)

    dss._destroy

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

    dss._destroy

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

  # Generic filters
  def test_generic_filters

    # Some basic tests for the filter class

    f = TextStringLengthFilter::new(3)
    assert_equal(f.wants_variants?, true)
    f.wants_variants = false
    assert_equal(f.wants_variants?, false)

    # Smoke test
    f.is_isotropic
    f.is_scale_invariant

    # Some application

    f = TextStringLengthFilter::new(3)

    texts = RBA::Texts::new
    texts.insert(RBA::Text::new("long", [ RBA::Trans::M45, 10, 0 ]))
    texts.insert(RBA::Text::new("tla", [ RBA::Trans::R0, 0, 0 ]))
    texts.insert(RBA::Text::new("00", [ RBA::Trans::R90, 0, 20 ]))

    assert_equal(texts.filtered(f).to_s, "('tla',r0 0,0)")
    assert_equal(texts.split_filter(f)[0].to_s, "('tla',r0 0,0)")
    assert_equal(texts.split_filter(f)[1].to_s, "('long',m45 10,0);('00',r90 0,20)")
    assert_equal(texts.to_s, "('long',m45 10,0);('tla',r0 0,0);('00',r90 0,20)")
    texts.filter(f)
    assert_equal(texts.to_s, "('tla',r0 0,0)")

  end

  # Generic processors
  def test_generic_processors_tt

    # Some basic tests for the processor class

    f = ReplaceTextString::new
    assert_equal(f.wants_variants?, true)
    f.wants_variants = false
    assert_equal(f.wants_variants?, false)

    # Smoke test
    f.is_isotropic
    f.is_scale_invariant

    # Some application

    texts = RBA::Texts::new

    texts.insert(RBA::Text::new("abc", RBA::Trans::new))
    texts.insert(RBA::Text::new("a long text", RBA::Trans::M45))

    assert_equal(texts.processed(ReplaceTextString::new).to_s, "('3',r0 0,0);('11',m45 0,0)")
    assert_equal(texts.to_s, "('abc',r0 0,0);('a long text',m45 0,0)")
    texts.process(ReplaceTextString::new)
    assert_equal(texts.to_s, "('3',r0 0,0);('11',m45 0,0)")

  end

  # Generic processors
  def test_generic_processors_tp

    p = SomeTextToPolygonOperator::new

    texts = RBA::Texts::new

    texts.insert(RBA::Text::new("abc", RBA::Trans::new))
    texts.insert(RBA::Text::new("a long text", RBA::Trans::M45))

    assert_equal(texts.processed(p).to_s, "(-30,-30;-30,30;30,30;30,-30);(-110,-110;-110,110;110,110;110,-110)")
    assert_equal(texts.to_s, "('abc',r0 0,0);('a long text',m45 0,0)")

  end

  # properties
  def test_props

    r = RBA::Texts::new([ RBA::TextWithProperties::new(RBA::Text::new("abc", RBA::Trans::new), { 1 => "one" }) ])
    assert_equal(r.to_s, "('abc',r0 0,0){1=>one}")

    r = RBA::Texts::new([])
    assert_equal(r.to_s, "")

    r = RBA::Texts::new(RBA::TextWithProperties::new(RBA::Text::new("abc", RBA::Trans::new), { 1 => "one" }))
    assert_equal(r.to_s, "('abc',r0 0,0){1=>one}")

    r = RBA::Texts::new
    r.insert(RBA::TextWithProperties::new(RBA::Text::new("abc", RBA::Trans::new), { 1 => "one" }))
    assert_equal(r.to_s, "('abc',r0 0,0){1=>one}")

    r = RBA::Texts::new
    r.insert(RBA::TextWithProperties::new(RBA::Text::new("abc", RBA::Trans::new), { 1 => "one" }))
    r.insert(RBA::Text::new("xuv", RBA::Trans::new))
    s = r.each.collect(&:to_s).join(";")
    assert_equal(s, "('xuv',r0 0,0) props={};('abc',r0 0,0) props={1=>one}")

  end

  # properties
  def test_prop_filters

    r = RBA::Texts::new
    r.insert(RBA::TextWithProperties::new(RBA::Text::new("abc", RBA::Trans::R0), { "one" => -1 }))
    r.insert(RBA::TextWithProperties::new(RBA::Text::new("uvw", RBA::Trans::R0), { "one" => 17 }))
    r.insert(RBA::TextWithProperties::new(RBA::Text::new("xyz", RBA::Trans::R0), { "one" => 42 }))

    assert_equal(r.filtered(RBA::TextFilter::property_filter("one", 11)).to_s, "")
    assert_equal(r.filtered(RBA::TextFilter::property_filter("two", 17)).to_s, "")
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter("one", 17)).to_s), csort("('uvw',r0 0,0){one=>17}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter("one", 17, true)).to_s), csort("('abc',r0 0,0){one=>-1};('xyz',r0 0,0){one=>42}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter_bounded("one", 17, nil)).to_s), csort("('xyz',r0 0,0){one=>42};('uvw',r0 0,0){one=>17}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter_bounded("one", 17, 18)).to_s), csort("('uvw',r0 0,0){one=>17}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter_bounded("one", 17, 18, true)).to_s), csort("('xyz',r0 0,0){one=>42};('abc',r0 0,0){one=>-1}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter_bounded("one", nil, 18)).to_s), csort("('uvw',r0 0,0){one=>17};('abc',r0 0,0){one=>-1}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_glob("one", "1*")).to_s), csort("('uvw',r0 0,0){one=>17}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_glob("one", "1*", true)).to_s), csort("('xyz',r0 0,0){one=>42};('abc',r0 0,0){one=>-1}"))

    ly = RBA::Layout::new
    top = ly.create_cell("TOP")
    l1 = ly.layer(1, 0)

    s = top.shapes(l1)
    s.insert(RBA::TextWithProperties::new(RBA::Text::new("abc", RBA::Trans::R0), { "one" => -1 }))
    s.insert(RBA::TextWithProperties::new(RBA::Text::new("uvw", RBA::Trans::R0), { "one" => 17 }))
    s.insert(RBA::TextWithProperties::new(RBA::Text::new("xyz", RBA::Trans::R0), { "one" => 42 }))

    dss = RBA::DeepShapeStore::new
    iter = top.begin_shapes_rec(l1)
    iter.enable_properties()
    r = RBA::Texts::new(iter, dss)

    assert_equal(r.filtered(RBA::TextFilter::property_filter("one", 11)).to_s, "")
    assert_equal(r.filtered(RBA::TextFilter::property_filter("two", 17)).to_s, "")
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter("one", 17)).to_s), csort("('uvw',r0 0,0){one=>17}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter("one", 17, true)).to_s), csort("('abc',r0 0,0){one=>-1};('xyz',r0 0,0){one=>42}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter_bounded("one", 17, nil)).to_s), csort("('xyz',r0 0,0){one=>42};('uvw',r0 0,0){one=>17}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter_bounded("one", 17, 18)).to_s), csort("('uvw',r0 0,0){one=>17}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter_bounded("one", 17, 18, true)).to_s), csort("('xyz',r0 0,0){one=>42};('abc',r0 0,0){one=>-1}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_filter_bounded("one", nil, 18)).to_s), csort("('uvw',r0 0,0){one=>17};('abc',r0 0,0){one=>-1}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_glob("one", "1*")).to_s), csort("('uvw',r0 0,0){one=>17}"))
    assert_equal(csort(r.filtered(RBA::TextFilter::property_glob("one", "1*", true)).to_s), csort("('xyz',r0 0,0){one=>42};('abc',r0 0,0){one=>-1}"))

    rr = r.dup
    rr.filter(RBA::TextFilter::property_filter("one", 17))
    assert_equal(csort(rr.to_s), csort("('uvw',r0 0,0){one=>17}"))

    dss._destroy

  end

  # polygons
  def test_polygons

    r = RBA::Texts::new
    r.insert(RBA::Text::new("abc", RBA::Trans::new(10, 20)))
    r.insert(RBA::Text::new("uvw", RBA::Trans::new(-10, -20)))

    assert_equal(r.polygons.to_s, "(9,19;9,21;11,21;11,19);(-11,-21;-11,-19;-9,-19;-9,-21)")
    assert_equal(r.polygons(2).to_s, "(8,18;8,22;12,22;12,18);(-12,-22;-12,-18;-8,-18;-8,-22)")
    assert_equal(r.polygons(1, 17).to_s, "(9,19;9,21;11,21;11,19){17=>abc};(-11,-21;-11,-19;-9,-19;-9,-21){17=>uvw}")

  end

end


load("test_epilogue.rb")

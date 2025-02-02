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

class RBA::RdbCell
  def to_s_test
    r = self.name
    r += "["
    refs = []
    self.each_reference do |i|
      refs << self.database.cell_by_id(i.parent_cell_id).name + "->" + i.trans.to_s
    end
    r += refs.join(",")
    r += "]"
  end
  def to_s_items
    r = self.name
    r += "["
    ai = []
    self.each_item do |i|
      vv = []
      i.each_value do |v|
        vv << v.to_s
      end
      ai << vv.join("/")
    end
    r += ai.join(",")
    r += "]"
  end
end

class RDB_TestClass < TestBase

  # RdbReference
  def test_1

    ref = RBA::RdbReference.new(RBA::DCplxTrans.new, 0) 
    assert_equal(ref.trans.to_s, "r0 *1 0,0")
    ref.trans = RBA::DCplxTrans.new(5.0)
    assert_equal(ref.trans.to_s, "r0 *5 0,0")

    assert_equal(ref.parent_cell_id, 0)
    ref.parent_cell_id = 177
    assert_equal(ref.parent_cell_id, 177)

  end

  # RdbCell, RdbCategory
  def test_2

    db = RBA::ReportDatabase.new("name")

    cell = db.create_cell("cell_name")
    assert_equal(cell.database.inspect, db.inspect)
    assert_equal(cell.name, "cell_name")
    assert_equal(cell.rdb_id, 1)

    cell2 = db.create_cell("new_cell", "var1")
    assert_equal(cell2.name, "new_cell")
    assert_equal(cell2.layout_name, "")
    assert_equal(cell2.qname, "new_cell:var1")

    cell2 = db.create_cell("cell_name", "var1", "cell_name$1")
    assert_equal(cell.name, "cell_name")
    assert_equal(cell.qname, "cell_name:1")
    assert_equal(db.cell_by_qname("cell_name:1").rdb_id, cell.rdb_id)
    assert_equal(db.cell_by_id(cell.rdb_id).rdb_id, cell.rdb_id)
    assert_equal(cell2.name, "cell_name")
    assert_equal(cell2.layout_name, "cell_name$1")
    assert_equal(cell2.qname, "cell_name:var1")
    assert_equal(db.cell_by_qname("cell_name:var1").rdb_id, cell2.rdb_id)
    assert_equal(db.cell_by_id(cell2.rdb_id).rdb_id, cell2.rdb_id)
    assert_equal(cell2.rdb_id, 3)
    assert_equal(cell.num_items, 0)
    assert_equal(cell2.num_items, 0)
    assert_equal(cell.num_items_visited, 0)
    assert_equal(cell2.num_items_visited, 0)

    cc = db.variants("cell_name")
    assert_equal(cc.size, 2)
    assert_equal(cc[0], cell.rdb_id)
    assert_equal(cc[1], cell2.rdb_id)
    
    cc = []
    db.each_cell { |c| cc.push(c) }
    assert_equal(cc.size, 3)
    assert_equal(cc[0].rdb_id, cell.rdb_id)
    assert_equal(cc[2].rdb_id, cell2.rdb_id)

    cat = db.create_category("cat")
    assert_equal(cat.database.inspect, db.inspect)
    assert_equal(cat.name, "cat")
    assert_equal(cat.rdb_id, 4)
    assert_equal(cat.path, "cat")

    cats = db.create_category(cat, "subcat")
    assert_equal(cats.name, "subcat")
    assert_equal(cats.rdb_id, 5)
    assert_equal(cats.path, "cat.subcat")
    assert_equal(cats.parent.rdb_id, cat.rdb_id)

    cat.description = "cat_desc"
    assert_equal(cat.description, "cat_desc")

    x = []
    cat.each_sub_category { |c| x.push(c) }
    assert_equal(x.size, 1)
    assert_equal(x[0].rdb_id, cats.rdb_id)

    item = db.create_item(cell.rdb_id, cat.rdb_id)
    assert_equal(db.num_items, 1)
    assert_equal(db.num_items_visited, 0)
    assert_equal(db.num_items(cell.rdb_id, cat.rdb_id), 1)
    assert_equal(db.num_items_visited(cell.rdb_id, cat.rdb_id), 0)
    assert_equal(db.num_items(cell.rdb_id, cats.rdb_id), 0)
    assert_equal(db.num_items_visited(cell.rdb_id, cats.rdb_id), 0)
    assert_equal(cell.num_items, 1)
    assert_equal(cell2.num_items, 0)
    assert_equal(cat.num_items, 1)
    assert_equal(cats.num_items, 0)
    assert_equal(cell.num_items_visited, 0)
    assert_equal(cell2.num_items_visited, 0)
    assert_equal(cat.num_items_visited, 0)
    assert_equal(cats.num_items_visited, 0)

    begin
      item = db.create_item(1000, cat.rdb_id)
      assert_equal(false, true)
    rescue => ex
      assert_equal(ex.to_s, "Not a valid cell ID: 1000 in ReportDatabase::create_item")
    end

    begin
      item = db.create_item(cell.rdb_id, 1001)
      assert_equal(false, true)
    rescue => ex
      assert_equal(ex.to_s, "Not a valid category ID: 1001 in ReportDatabase::create_item")
    end

    item2 = db.create_item(cell2, cats)
    assert_equal(db.num_items, 2)
    assert_equal(db.num_items_visited, 0)
    assert_equal(db.num_items(cell2.rdb_id, cats.rdb_id), 1)
    assert_equal(db.num_items_visited(cell2.rdb_id, cats.rdb_id), 0)
    assert_equal(db.num_items(cell2.rdb_id, cat.rdb_id), 1)
    assert_equal(db.num_items_visited(cell2.rdb_id, cat.rdb_id), 0)
    assert_equal(cell.num_items, 1)
    assert_equal(cell2.num_items, 1)
    assert_equal(cat.num_items, 2)
    assert_equal(cats.num_items, 1)
    assert_equal(cell.num_items_visited, 0)
    assert_equal(cell2.num_items_visited, 0)
    assert_equal(cat.num_items_visited, 0)
    assert_equal(cats.num_items_visited, 0)

    db.set_item_visited(item, true)
    assert_equal(item.is_visited?, true)
    assert_equal(cell.num_items_visited, 1)
    assert_equal(cell2.num_items_visited, 0)
    assert_equal(cat.num_items_visited, 1)
    assert_equal(cats.num_items_visited, 0)
    assert_equal(db.num_items, 2)
    assert_equal(db.num_items_visited, 1)
    assert_equal(db.num_items(cell.rdb_id, cat.rdb_id), 1)
    assert_equal(db.num_items_visited(cell.rdb_id, cat.rdb_id), 1)
    assert_equal(db.num_items(cell2.rdb_id, cats.rdb_id), 1)
    assert_equal(db.num_items_visited(cell2.rdb_id, cats.rdb_id), 0)
    assert_equal(db.num_items(cell.rdb_id, cats.rdb_id), 0)
    assert_equal(db.num_items_visited(cell.rdb_id, cats.rdb_id), 0)

    db.set_item_visited(item2, true)
    assert_equal(cell.num_items_visited, 1)
    assert_equal(cell2.num_items_visited, 1)
    assert_equal(cat.num_items_visited, 2)
    assert_equal(cats.num_items_visited, 1)
    assert_equal(db.num_items, 2)
    assert_equal(db.num_items_visited, 2)
    assert_equal(db.num_items(cell.rdb_id, cat.rdb_id), 1)
    assert_equal(db.num_items_visited(cell.rdb_id, cat.rdb_id), 1)
    assert_equal(db.num_items(cell2.rdb_id, cats.rdb_id), 1)
    assert_equal(db.num_items_visited(cell2.rdb_id, cats.rdb_id), 1)
    assert_equal(db.num_items(cell.rdb_id, cats.rdb_id), 0)
    assert_equal(db.num_items_visited(cell.rdb_id, cats.rdb_id), 0)

    db.set_item_visited(item, false)
    assert_equal(item.is_visited?, false)
    assert_equal(cell.num_items_visited, 0)
    assert_equal(cell2.num_items_visited, 1)
    assert_equal(cat.num_items_visited, 1)
    assert_equal(cats.num_items_visited, 1)
    assert_equal(db.num_items, 2)
    assert_equal(db.num_items_visited, 1)
    assert_equal(db.num_items(cell.rdb_id, cat.rdb_id), 1)
    assert_equal(db.num_items_visited(cell.rdb_id, cat.rdb_id), 0)
    assert_equal(db.num_items(cell2.rdb_id, cats.rdb_id), 1)
    assert_equal(db.num_items_visited(cell2.rdb_id, cats.rdb_id), 1)
    assert_equal(db.num_items(cell.rdb_id, cats.rdb_id), 0)
    assert_equal(db.num_items_visited(cell.rdb_id, cats.rdb_id), 0)

    ii = []
    db.each_item { |i| ii.push(i) }
    assert_equal(ii.size, 2)
    assert_equal(ii[0].cell_id, item.cell_id)
    assert_equal(ii[0].category_id, item.category_id)
    assert_equal(ii[1].cell_id, item2.cell_id)
    assert_equal(ii[1].category_id, item2.category_id)

    ii = []
    db.each_item_per_cell(cell.rdb_id) { |i| ii.push(i) }
    assert_equal(ii.size, 1)
    assert_equal(ii[0].cell_id, item.cell_id)
    assert_equal(ii[0].category_id, item.category_id)

    ii = []
    cell.each_item { |i| ii.push(i) }
    assert_equal(ii.size, 1)
    assert_equal(ii[0].cell_id, item.cell_id)
    assert_equal(ii[0].category_id, item.category_id)

    ii = []
    db.each_item_per_category(cats.rdb_id) { |i| ii.push(i) }
    assert_equal(ii.size, 1)
    assert_equal(ii[0].cell_id, item2.cell_id)
    assert_equal(ii[0].category_id, item2.category_id)

    ii = []
    cats.each_item { |i| ii.push(i) }
    assert_equal(ii.size, 1)
    assert_equal(ii[0].cell_id, item2.cell_id)
    assert_equal(ii[0].category_id, item2.category_id)

    ii = []
    db.each_item_per_cell_and_category(cell.rdb_id, cats.rdb_id) { |i| ii.push(i) }
    assert_equal(ii.size, 0)

    ii = []
    db.each_item_per_cell_and_category(cell2.rdb_id, cats.rdb_id) { |i| ii.push(i) }
    assert_equal(ii.size, 1)
    assert_equal(ii[0].cell_id, item2.cell_id)
    assert_equal(ii[0].category_id, item2.category_id)

    refs = []
    cell.each_reference { |r| refs.push(r) }
    assert_equal(refs.size, 0)

    cell.add_reference(RBA::RdbReference.new(RBA::DCplxTrans.new(2.5), 178))
    refs = []
    cell.each_reference { |r| refs.push(r) }
    assert_equal(refs.size, 1)
    assert_equal(refs[0].parent_cell_id, 178)
    assert_equal(refs[0].database.inspect, db.inspect)

    cell.clear_references
    refs = []
    cell.each_reference { |r| refs.push(r) }
    assert_equal(refs.size, 0)

  end

  # RdbItemValue
  def test_3

    v = RBA::RdbItemValue.new(1.0)
    assert_equal(v.tag_id, 0)
    v.tag_id = 15
    assert_equal(v.tag_id, 15)

    vf = RBA::RdbItemValue.new(1.0)
    vs = RBA::RdbItemValue.new("a string")
    vb = RBA::RdbItemValue.new(RBA::DBox.new(0, 10, 20, 30))
    vl = RBA::RdbItemValue.new(RBA::DText.new("abc", RBA::DTrans.new()))
    vp = RBA::RdbItemValue.new(RBA::DPolygon.new(RBA::DBox.new(100, 101, 102, 103)))
    ve = RBA::RdbItemValue.new(RBA::DEdge.new(RBA::DPoint.new(0, 10), RBA::DPoint.new(20, 30)))
    vee = RBA::RdbItemValue.new(RBA::DEdgePair.new(RBA::DEdge.new(0, 10, 5, 15), RBA::DEdge.new(20, 30, 25, 35)))

    assert_equal(vf.to_s, "float: 1")
    assert_equal(vs.to_s, "text: 'a string'")
    assert_equal(vb.to_s, "box: (0,10;20,30)")
    assert_equal(vp.to_s, "polygon: (100,101;100,103;102,103;102,101)")
    assert_equal(vl.to_s, "label: ('abc',r0 0,0)");
    assert_equal(ve.to_s, "edge: (0,10;20,30)")
    assert_equal(vee.to_s, "edge-pair: (0,10;5,15)/(20,30;25,35)")
    assert_equal(RBA::RdbItemValue::from_s(vf.to_s).to_s, vf.to_s)
    assert_equal(RBA::RdbItemValue::from_s(vs.to_s).to_s, vs.to_s)
    assert_equal(RBA::RdbItemValue::from_s(vb.to_s).to_s, vb.to_s)
    assert_equal(RBA::RdbItemValue::from_s(vp.to_s).to_s, vp.to_s)
    assert_equal(RBA::RdbItemValue::from_s(vl.to_s).to_s, vl.to_s)
    assert_equal(RBA::RdbItemValue::from_s(ve.to_s).to_s, ve.to_s)
    assert_equal(RBA::RdbItemValue::from_s(vee.to_s).to_s, vee.to_s)

    assert_equal(vf.is_float?, true)
    assert_equal(vf.is_string?, false)
    assert_equal(vf.is_polygon?, false)
    assert_equal(vf.is_text?, false)
    assert_equal(vf.is_edge?, false)
    assert_equal(vf.is_box?, false)
    assert_equal(vf.is_edge_pair?, false)
    assert_equal(vf.float, 1)
    assert_equal(vf.string, "1")

    assert_equal(vs.is_float?, false)
    assert_equal(vs.is_string?, true)
    assert_equal(vs.is_polygon?, false)
    assert_equal(vs.is_text?, false)
    assert_equal(vs.is_edge?, false)
    assert_equal(vs.is_box?, false)
    assert_equal(vs.is_edge_pair?, false)
    assert_equal(vs.string, "a string")

    assert_equal(vl.is_float?, false)
    assert_equal(vl.is_string?, false)
    assert_equal(vl.is_polygon?, false)
    assert_equal(vl.is_text?, true)
    assert_equal(vl.is_edge?, false)
    assert_equal(vl.is_box?, false)
    assert_equal(vl.is_edge_pair?, false)
    assert_equal(vl.text.to_s, "('abc',r0 0,0)")
    assert_equal(vl.string, "label: " + vl.text.to_s)

    assert_equal(vp.is_float?, false)
    assert_equal(vp.is_string?, false)
    assert_equal(vp.is_polygon?, true)
    assert_equal(vp.is_text?, false)
    assert_equal(vp.is_edge?, false)
    assert_equal(vp.is_box?, false)
    assert_equal(vp.is_edge_pair?, false)
    assert_equal(vp.polygon.to_s, "(100,101;100,103;102,103;102,101)")
    assert_equal(vp.string, "polygon: " + vp.polygon.to_s)

    assert_equal(ve.is_float?, false)
    assert_equal(ve.is_string?, false)
    assert_equal(ve.is_polygon?, false)
    assert_equal(ve.is_text?, false)
    assert_equal(ve.is_edge?, true)
    assert_equal(ve.is_box?, false)
    assert_equal(ve.is_edge_pair?, false)
    assert_equal(ve.edge.to_s, "(0,10;20,30)")
    assert_equal(ve.string, "edge: " + ve.edge.to_s)

    assert_equal(vb.is_float?, false)
    assert_equal(vb.is_string?, false)
    assert_equal(vb.is_polygon?, false)
    assert_equal(vb.is_text?, false)
    assert_equal(vb.is_edge?, false)
    assert_equal(vb.is_box?, true)
    assert_equal(vb.is_edge_pair?, false)
    assert_equal(vb.box.to_s, "(0,10;20,30)")
    assert_equal(vb.string, "box: " + vb.box.to_s)

    assert_equal(vee.is_float?, false)
    assert_equal(vee.is_string?, false)
    assert_equal(vee.is_polygon?, false)
    assert_equal(vee.is_text?, false)
    assert_equal(vee.is_edge?, false)
    assert_equal(vee.is_box?, false)
    assert_equal(vee.is_edge_pair?, true)
    assert_equal(vee.edge_pair.to_s, "(0,10;5,15)/(20,30;25,35)")
    assert_equal(vee.string, "edge-pair: " + vee.edge_pair.to_s)

  end

  # RdbItem
  def test_4

    db = RBA::ReportDatabase.new("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    item = db.create_item(cell.rdb_id, cat.rdb_id)
    assert_equal(item.database.inspect, db.inspect)
    assert_equal(item.cell_id, cell.rdb_id)
    assert_equal(item.category_id, cat.rdb_id)

    assert_equal(db.user_tag_id("x1") != 0, true)
    assert_equal(db.tag_id("x1") != 0, true)
    assert_equal(db.tag_id("x1") == db.user_tag_id("x1"), false)
    db.set_tag_description(db.user_tag_id("x1"), "D")
    assert_equal(db.tag_description(db.user_tag_id("x1")), "D")
    assert_equal(db.tag_name(db.user_tag_id("x1")), "x1")
    assert_equal(db.tag_description(db.tag_id("x1")), "")
    assert_equal(db.tag_name(db.tag_id("x1")), "x1")
    db.set_tag_description(db.tag_id("x1"), "U")
    assert_equal(db.tag_description(db.user_tag_id("x1")), "D")
    assert_equal(db.tag_description(db.tag_id("x1")), "U")

    item.add_tag(db.tag_id("x1"))
    assert_equal(item.has_tag?(db.tag_id("x2")), false)
    assert_equal(item.has_tag?(db.tag_id("x1")), true)
    assert_equal(item.tags_str, "x1")
    item.add_tag(db.tag_id("x2"))
    assert_equal(item.has_tag?(db.tag_id("x2")), true)
    assert_equal(item.has_tag?(db.tag_id("x1")), true)
    assert_equal(item.tags_str, "x1,x2")
    item.remove_tag(db.tag_id("x1"))
    assert_equal(item.has_tag?(db.tag_id("x2")), true)
    assert_equal(item.has_tag?(db.tag_id("x1")), false)
    assert_equal(item.tags_str, "x2")

    item.tags_str="x2,x1"
    assert_equal(item.has_tag?(db.tag_id("x2")), true)
    assert_equal(item.has_tag?(db.tag_id("x1")), true)
    assert_equal(item.tags_str, "x1,x2")

    item.tags_str=""
    assert_equal(item.has_tag?(db.tag_id("x2")), false)
    assert_equal(item.has_tag?(db.tag_id("x1")), false)
    assert_equal(item.tags_str, "")
    
    assert_equal(item.image_str, "")
    assert_equal(item.has_image?, false)

    assert_equal(item.comment, "")
    item.comment = "abc"
    assert_equal(item.comment, "abc")

    # can actually by any string, but only base64-encoded PNG images make sense
    is="iVBORw0KGgoAAAANSUhEUgAAACoAAAA0CAIAAABzfT3nAAAAA3NCSVQICAjb4U/gAAAACXBIWXMAAA0SAAANOgHo3ZneAAAA3UlEQVRYhe2WwQ3DIAxFoco8XaGZIaeO43FyYgZYgYXcQ6SWuDGgBhWq/qccIvGCEd9SbAwAAPSGaW2lFR2rfWDpXrPpSe2SP10fvnn/PZHZH9IwbKFVZZ/Z6wMtZcjW02Bn2FVpZYdWdkr2nvh23S2FyDNJuVITpwmRjTGbNr0v20U5byNtJuuJt/fO2f93+UlbEJl5UjVPr3Y71EQ/PoPPlU+lDJtWlCt3GwCMG33BuJGAcWMEMG6c1jBudCyf/nzV8nbZPRohclFLHdGbZ8eNSjN1fmf0AACA1jwA4hKxu4C6P7EAAAAASUVORK5CYII="
    item.image_str=is
    assert_equal(item.image_str, is)
    assert_equal(item.has_image?, true)

    if item.respond_to?(:image_pixels)
      px=item.image_pixels
      assert_equal(px.width, 42)
      assert_equal(px.height, 52)
      item.image = px
      px2=item.image_pixels
      assert_equal(px == px2, true)
    end
    
    if item.respond_to?(:image)
      px=item.image
      assert_equal(px.width, 42)
      assert_equal(px.height, 52)
      item.image = px
      px2=item.image
      assert_equal(px2.width, 42)
      assert_equal(px2.height, 52)
    end
    
    vs = RBA::RdbItemValue.new("a string")
    vs2 = RBA::RdbItemValue.new("a string (ii)")
    item.add_value(vs)
    item.add_value(vs2)

    vv=[]
    item.each_value { |v| vv.push(v) }
    assert_equal(vv.size, 2)
    assert_equal(vv[0].to_s, "text: 'a string'")
    assert_equal(vv[1].to_s, "text: 'a string (ii)'")

    item.clear_values
    vv=[]
    item.each_value { |v| vv.push(v) }
    assert_equal(vv.size, 0)

    item.clear_values
    item.add_value(1.0)
    item.add_value("hello")
    item.add_value(RBA::DPolygon::new(RBA::DBox::new(1, 2, 3, 4)))
    item.add_value(RBA::DBox::new(11, 12, 13, 14))
    item.add_value(RBA::DEdge::new(21, 22, 23, 24))
    item.add_value(RBA::DEdgePair::new(RBA::DEdge::new(31, 32, 33, 34), RBA::DEdge::new(41, 42, 43, 44)))
    shapes = RBA::Shapes::new
    pts = [ RBA::Point::new(0, 0), RBA::Point::new(50, 150) ]
    shapes.insert(RBA::Path::new(pts, 100))
    shapes.each do |s|
      item.add_value(s, RBA::CplxTrans::new(0.001))
    end
    vv=[]
    item.each_value { |v| vv.push(v) }
    assert_equal(vv.size, 7)
    assert_equal(vv[0].to_s, "float: 1")
    assert_equal(vv[1].to_s, "text: hello")
    assert_equal(vv[2].to_s, "polygon: (1,2;1,4;3,4;3,2)")
    assert_equal(vv[3].to_s, "box: (11,12;13,14)")
    assert_equal(vv[4].to_s, "edge: (21,22;23,24)")
    assert_equal(vv[5].to_s, "edge-pair: (31,32;33,34)/(41,42;43,44)")
    assert_equal(vv[6].to_s, "path: (0,0;0.05,0.15) w=0.1 bx=0 ex=0 r=false")

  end

  # Multiple items
  def test_5

    db = RBA::ReportDatabase.new("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    r = RBA::Region::new(RBA::Box::new(0, 0, 100, 200))
    db.create_items(cell.rdb_id, cat.rdb_id, RBA::CplxTrans::new(0.001), r)
    a = []
    db.each_item_per_category(cat.rdb_id) { |item| item.each_value { |v| a.push(v.to_s) } }
    assert_equal(a.join(";"), "polygon: (0,0;0,0.2;0.1,0.2;0.1,0)")

    db = RBA::ReportDatabase.new("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    p = RBA::Polygon::new(RBA::Box::new(0, 0, 100, 200))
    db.create_items(cell.rdb_id, cat.rdb_id, RBA::CplxTrans::new(0.001), [p])
    a = []
    db.each_item_per_category(cat.rdb_id) { |item| item.each_value { |v| a.push(v.to_s) } }
    assert_equal(a.join(";"), "polygon: (0,0;0,0.2;0.1,0.2;0.1,0)")

    db = RBA::ReportDatabase.new("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    r = RBA::Edges::new(RBA::Edge::new(0, 0, 100, 200))
    db.create_items(cell.rdb_id, cat.rdb_id, RBA::CplxTrans::new(0.001), r)
    a = []
    db.each_item_per_category(cat.rdb_id) { |item| item.each_value { |v| a.push(v.to_s) } }
    assert_equal(a.join(";"), "edge: (0,0;0.1,0.2)")

    db = RBA::ReportDatabase.new("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    r = [ RBA::Edge::new(0, 0, 100, 200) ]
    db.create_items(cell.rdb_id, cat.rdb_id, RBA::CplxTrans::new(0.001), r)
    a = []
    db.each_item_per_category(cat.rdb_id) { |item| item.each_value { |v| a.push(v.to_s) } }
    assert_equal(a.join(";"), "edge: (0,0;0.1,0.2)")

    db = RBA::ReportDatabase.new("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    r = RBA::EdgePairs::new
    r.insert(RBA::EdgePair::new(RBA::Edge::new(0, 0, 100, 200), RBA::Edge::new(10, 10, 50, 150)))
    db.create_items(cell.rdb_id, cat.rdb_id, RBA::CplxTrans::new(0.001), r)
    a = []
    db.each_item_per_category(cat.rdb_id) { |item| item.each_value { |v| a.push(v.to_s) } }
    assert_equal(a.join(";"), "edge-pair: (0,0;0.1,0.2)/(0.01,0.01;0.05,0.15)")

    db = RBA::ReportDatabase.new("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    r = [ RBA::EdgePair::new(RBA::Edge::new(0, 0, 100, 200), RBA::Edge::new(10, 10, 50, 150)) ]
    db.create_items(cell.rdb_id, cat.rdb_id, RBA::CplxTrans::new(0.001), r)
    a = []
    db.each_item_per_category(cat.rdb_id) { |item| item.each_value { |v| a.push(v.to_s) } }
    assert_equal(a.join(";"), "edge-pair: (0,0;0.1,0.2)/(0.01,0.01;0.05,0.15)")

  end

  # ReportDatabase
  def test_6

    db = RBA::ReportDatabase.new("name")
    db.reset_modified
    assert_equal(db.is_modified?, false)
    assert_equal(db.name, "name")
    db.description = "desc"
    db.generator = "gg"
    db.top_cell_name = "top"
    db.original_file= "of"
    assert_equal(db.description, "desc")
    assert_equal(db.generator, "gg")
    assert_equal(db.top_cell_name, "top")
    assert_equal(db.original_file, "of")

    assert_equal(db.is_modified?, true)
    db.reset_modified
    assert_equal(db.is_modified?, false)

    tag_id = db.tag_id("x")
    assert_equal(tag_id, 1)
    db.set_tag_description(tag_id, "xdesc")
    assert_equal(db.tag_description(tag_id), "xdesc")

    cell = db.create_cell("cell_name")
    cc = []
    db.each_cell { |c| cc.push(c) }
    assert_equal(cc.size, 1)
    assert_equal(cc[0].rdb_id, cell.rdb_id)

    cat = db.create_category("cat")
    cc = []
    db.each_category { |c| cc.push(c) }
    assert_equal(cc.size, 1)
    assert_equal(cc[0].rdb_id, cat.rdb_id)

    cats = db.create_category(cat, "subcat")
    c = db.category_by_path("x")
    assert_equal(c, nil)
    c = db.category_by_path("cat")
    assert_equal(c.rdb_id, cat.rdb_id)
    c = db.category_by_path("cat.subcat")
    assert_equal(c.rdb_id, cats.rdb_id)

    assert_equal(db.category_by_id(cat.rdb_id).rdb_id, cat.rdb_id)
    assert_equal(db.category_by_id(cats.rdb_id).rdb_id, cats.rdb_id)

    item = db.create_item(cell.rdb_id, cat.rdb_id)
    v = RBA::RdbItemValue.new("a")
    v.tag_id = db.user_tag_id("x2")
    item.add_value(v)
    v = RBA::RdbItemValue.new("b")
    v.tag_id = db.tag_id("x1")
    item.add_value(v)
    item.add_tag(db.tag_id("x1"))
    item.add_tag(db.user_tag_id("x2"))
    is="iVBORw0KGgoAAAANSUhEUgAAACoAAAA0CAIAAABzfT3nAAAAA3NCSVQICAjb4U/gAAAACXBIWXMAAA0SAAANOgHo3ZneAAAA3UlEQVRYhe2WwQ3DIAxFoco8XaGZIaeO43FyYgZYgYXcQ6SWuDGgBhWq/qccIvGCEd9SbAwAAPSGaW2lFR2rfWDpXrPpSe2SP10fvnn/PZHZH9IwbKFVZZ/Z6wMtZcjW02Bn2FVpZYdWdkr2nvh23S2FyDNJuVITpwmRjTGbNr0v20U5byNtJuuJt/fO2f93+UlbEJl5UjVPr3Y71EQ/PoPPlU+lDJtWlCt3GwCMG33BuJGAcWMEMG6c1jBudCyf/nzV8nbZPRohclFLHdGbZ8eNSjN1fmf0AACA1jwA4hKxu4C6P7EAAAAASUVORK5CYII="
    if item.respond_to?(:image_str)
      item.image_str=is
    end

    tmp = File::join($ut_testtmp, "tmp.lyrdb")

    assert_equal(db.filename, "")
    db.save(tmp)
    assert_equal(db.filename, tmp)

    # load and save
    db = nil

    db2 = RBA::ReportDatabase.new("neu")
    db2.load(tmp)
    assert_equal(File.basename(db2.filename), File.basename(tmp))
    assert_equal(db2.name, File.basename(tmp))

    assert_equal(db2.description, "desc")
    assert_equal(db2.generator, "gg")
    assert_equal(db2.top_cell_name, "top")
    assert_equal(db2.original_file, "of")

    c = db2.category_by_path("cat.subcat")
    assert_equal(c.path, "cat.subcat")

    cc = []
    db2.each_category { |c| cc.push(c) }
    assert_equal(cc.size, 1)

    ii = []
    db2.each_item { |i| ii.push(i) }
    assert_equal(ii.size, 1)
    assert_equal(ii[0].tags_str, "x1,#x2")
    assert_equal(ii[0].has_tag?(db2.user_tag_id("x2")), true)
    assert_equal(ii[0].has_tag?(db2.tag_id("x1")), true)
    assert_equal(ii[0].has_tag?(db2.tag_id("x")), false)
    # Only the first 30 bytes count ... the remaining part is too different for different versions of Qt
    if ii[0].respond_to?(:image_str)
      assert_equal(ii[0].image_str[0..30], is[0..30])
    end
    assert_equal(db2.cell_by_id(ii[0].cell_id).qname, "cell_name")
    assert_equal(db2.category_by_id(ii[0].category_id).path, "cat")
    vs = ""
    ii[0].each_value { |v| vs += v.string }
    assert_equal(vs, "ab")
    vs = ""
    ii[0].each_value { |v| v.tag_id == db2.tag_id("x1") && vs += v.string }
    assert_equal(vs, "b")
    vs = ""
    ii[0].each_value { |v| v.tag_id == db2.user_tag_id("x1") && vs += v.string }
    assert_equal(vs, "")
    vs = ""
    ii[0].each_value { |v| v.tag_id == db2.user_tag_id("x2") && vs += v.string }
    assert_equal(vs, "a")

  end

  # LayoutView
  def test_10

    if !RBA.constants.member?(:Application)
      return
    end

    mw = RBA::Application.instance.main_window
    mw.create_layout(1)
    view = mw.current_view

    ot = 0
    o = lambda { ot += 1 }

    view.on_rdb_list_changed += o

    rdb_index = view.create_rdb("NEW_RDB")
    assert_equal(view.num_rdbs, 1)
    assert_equal(ot, 1)
    assert_equal(view.rdb(rdb_index).name, "NEW_RDB")
    view.remove_rdb(rdb_index)
    assert_equal(view.num_rdbs, 0)
    assert_equal(ot, 2)

    view.on_rdb_list_changed -= o

    ot = 0
    rdb_index = view.create_rdb("NEW_RDB2")
    assert_equal(view.rdb(rdb_index).name, "NEW_RDB2")
    assert_equal(ot, 0)
    assert_equal(view.num_rdbs, 1)
    rdb_index = view.create_rdb("NEW_RDB3")
    assert_equal(view.rdb(rdb_index).name, "NEW_RDB3")
    assert_equal(view.num_rdbs, 2)
    assert_equal(ot, 0)

    mw.close_current_view

  end

  # scan_... methods
  def test_11

    ly = RBA::Layout::new
    c0 = ly.create_cell("c0")
    c1 = ly.create_cell("c1")
    c2 = ly.create_cell("c2")
    c3 = ly.create_cell("c3")
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(10, 20)))
    c2.insert(RBA::CellInstArray::new(c3.cell_index, RBA::Trans::new(11, 21)))
    l1 = ly.insert_layer(RBA::LayerInfo::new(1, 0))
    prop_id = ly.properties_id([[ "a", 17 ]])
    c0.shapes(l1).insert(RBA::Box::new(0, 1, 2, 3), prop_id)
    prop_id = ly.properties_id([[ "a", 21 ]])
    c1.shapes(l1).insert(RBA::Box::new(0, 1, 20, 30), prop_id)
    c2.shapes(l1).insert(RBA::Box::new(0, 1, 21, 31))
    c3.shapes(l1).insert(RBA::Box::new(0, 1, 22, 32))

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1)
    assert_equal(rdb.tag_name(1), "a") # from property
    assert_equal(cat.num_items, 4)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "c0[];c1[];c2[];c3[]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c0[polygon: (0,0.001;0,0.003;0.002,0.003;0.002,0.001)/float: 17];c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1, nil, -1, false)
    assert_equal(cat.num_items, 4)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "c0[];c1[];c2[];c3[]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c0[polygon: (0,0.001;0,0.003;0.002,0.003;0.002,0.001)];c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1, c1)
    assert_equal(cat.num_items, 3)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "c1[];c2[c1->r0 *1 0.01,0.02];c3[c1->r0 *1 0.021,0.041]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1, c1, 0)
    assert_equal(cat.num_items, 1)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "c1[]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1, c1, -1)
    assert_equal(cat.num_items, 3)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "c1[];c2[c1->r0 *1 0.01,0.02];c3[c1->r0 *1 0.021,0.041]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1, c1, 1)
    assert_equal(cat.num_items, 2)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "c1[];c2[c1->r0 *1 0.01,0.02]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    cat.scan_shapes(c1.begin_shapes_rec(l1))  # hierarchical scan
    assert_equal(cat.num_items, 3)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "c1[];c2[c1->r0 *1 0.01,0.02];c3[c1->r0 *1 0.021,0.041]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    cat.scan_shapes(c1.begin_shapes_rec(l1), false, false)  # hierarchical scan
    assert_equal(cat.num_items, 3)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "c1[];c2[c1->r0 *1 0.01,0.02];c3[c1->r0 *1 0.021,0.041]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    cat.scan_shapes(c1.begin_shapes_rec(l1), true)  # flat scan
    assert_equal(cat.num_items, 3)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "c1[]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21,polygon: (0.01,0.021;0.01,0.051;0.031,0.051;0.031,0.021),polygon: (0.021,0.042;0.021,0.073;0.043,0.073;0.043,0.042)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    r = RBA::Region::new(c1.begin_shapes_rec(l1))
    cat.scan_collection(rdb.create_cell("TOP"), RBA::CplxTrans::new(0.001), r)  # hierarchical scan
    assert_equal(cat.num_items, 3)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "TOP[];c1[TOP->r0 *1 0,0];c2[c1->r0 *1 0.01,0.02];c3[c1->r0 *1 0.021,0.041]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "TOP[];c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    r = RBA::Region::new(c1.begin_shapes_rec(l1))
    cat.scan_collection(rdb.create_cell("TOP"), RBA::CplxTrans::new(0.001), r, true)  # flat scan
    assert_equal(cat.num_items, 3)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "TOP[]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "TOP[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21,polygon: (0.01,0.021;0.01,0.051;0.031,0.051;0.031,0.021),polygon: (0.021,0.042;0.021,0.073;0.043,0.073;0.043,0.042)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    r = RBA::Region::new(c1.begin_shapes_rec(l1))
    cat.scan_collection(rdb.create_cell("TOP"), RBA::CplxTrans::new(0.001), r, true, false)  # flat scan
    assert_equal(cat.num_items, 3)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "TOP[]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "TOP[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001),polygon: (0.01,0.021;0.01,0.051;0.031,0.051;0.031,0.021),polygon: (0.021,0.042;0.021,0.073;0.043,0.073;0.043,0.042)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat = rdb.create_category("l1")
    r = RBA::Region::new(c1.begin_shapes_rec(l1)).merged
    cat.scan_collection(rdb.create_cell("TOP"), RBA::CplxTrans::new(0.001), r, true)  # flat scan
    assert_equal(cat.num_items, 1)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_test }
    assert_equal(cn.join(";"), "TOP[]")
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "TOP[polygon: (0,0.001;0,0.03;0.01,0.03;0.01,0.051;0.021,0.051;0.021,0.073;0.043,0.073;0.043,0.042;0.031,0.042;0.031,0.021;0.02,0.021;0.02,0.001)]")

  end

  # shape insertion from shape, shapes, recursive iterator
  def test_12

    ly = RBA::Layout::new
    c0 = ly.create_cell("c0")
    c1 = ly.create_cell("c1")
    c2 = ly.create_cell("c2")
    c3 = ly.create_cell("c3")
    c1.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(10, 20)))
    c2.insert(RBA::CellInstArray::new(c3.cell_index, RBA::Trans::new(11, 21)))
    l1 = ly.insert_layer(RBA::LayerInfo::new(1, 0))
    c0.shapes(l1).insert(RBA::Box::new(0, 1, 2, 3))
    c1.shapes(l1).insert(RBA::Text::new("Hello, world!", RBA::Trans::new))
    c2.shapes(l1).insert(RBA::Edge::new(0, 1, 21, 31))
    c3.shapes(l1).insert(RBA::Polygon::new(RBA::Box::new(0, 1, 22, 32)))

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    c0.shapes(l1).each do |s|
      rdb.create_item(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), s)
    end
    assert_equal(cat1.num_items, 1)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0.001;0,0.003;0.002,0.003;0.002,0.001)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), c0.shapes(l1))
    assert_equal(cat1.num_items, 1)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0.001;0,0.003;0.002,0.003;0.002,0.001)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, c1.begin_shapes_rec(l1))
    assert_equal(cat1.num_items, 3)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[label: ('Hello, world!',r0 0,0),edge: (0.01,0.021;0.031,0.051),polygon: (0.021,0.042;0.021,0.073;0.043,0.073;0.043,0.042)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), [ RBA::Polygon::new(RBA::Box::new(0, 0, 100, 200)), RBA::Polygon::new(RBA::Box::new(100, 200, 110, 210)) ])
    assert_equal(cat1.num_items, 2)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0;0,0.2;0.1,0.2;0.1,0),polygon: (0.1,0.2;0.1,0.21;0.11,0.21;0.11,0.2)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), [ RBA::Edge::new(0, 0, 100, 200), RBA::Edge::new(100, 200, 110, 210) ])
    assert_equal(cat1.num_items, 2)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[edge: (0,0;0.1,0.2),edge: (0.1,0.2;0.11,0.21)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), [ RBA::EdgePair::new(RBA::Edge::new(0, 0, 100, 200), RBA::Edge::new(100, 200, 110, 210)) ])
    assert_equal(cat1.num_items, 1)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[edge-pair: (0,0;0.1,0.2)/(0.1,0.2;0.11,0.21)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), [ 
        RBA::PolygonWithProperties::new(RBA::Polygon::new(RBA::Box::new(0, 0, 100, 200)), { 1 => "one" }),
        RBA::PolygonWithProperties::new(RBA::Polygon::new(RBA::Box::new(100, 200, 110, 210)), { 42 => 17 }) 
    ])
    assert_equal(cat1.num_items, 2)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0;0,0.2;0.1,0.2;0.1,0)/text: one,polygon: (0.1,0.2;0.1,0.21;0.11,0.21;0.11,0.2)/float: 17]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), [ 
        RBA::PolygonWithProperties::new(RBA::Polygon::new(RBA::Box::new(0, 0, 100, 200)), { 1 => "one" }),
        RBA::PolygonWithProperties::new(RBA::Polygon::new(RBA::Box::new(100, 200, 110, 210)), { 42 => 17 }) 
    ], false)
    assert_equal(cat1.num_items, 2)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[polygon: (0,0;0,0.2;0.1,0.2;0.1,0),polygon: (0.1,0.2;0.1,0.21;0.11,0.21;0.11,0.2)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), [ 
        RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 200), { 1 => "one" }),
        RBA::EdgeWithProperties::new(RBA::Edge::new(100, 200, 110, 210), { 42 => 17 }) 
    ])
    assert_equal(cat1.num_items, 2)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[edge: (0,0;0.1,0.2)/text: one,edge: (0.1,0.2;0.11,0.21)/float: 17]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), [ 
        RBA::EdgeWithProperties::new(RBA::Edge::new(0, 0, 100, 200), { 1 => "one" }),
        RBA::EdgeWithProperties::new(RBA::Edge::new(100, 200, 110, 210), { 42 => 17 }) 
    ], false)
    assert_equal(cat1.num_items, 2)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[edge: (0,0;0.1,0.2),edge: (0.1,0.2;0.11,0.21)]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), [ 
        RBA::EdgePairWithProperties::new(RBA::EdgePair::new(RBA::Edge::new(0, 0, 100, 200), RBA::Edge::new(100, 200, 110, 210)), { 1 => "one" })
    ])
    assert_equal(cat1.num_items, 1)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[edge-pair: (0,0;0.1,0.2)/(0.1,0.2;0.11,0.21)/text: one]")

    rdb = RBA::ReportDatabase.new("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id, cat1.rdb_id, RBA::CplxTrans::new(ly.dbu), [ 
        RBA::EdgePairWithProperties::new(RBA::EdgePair::new(RBA::Edge::new(0, 0, 100, 200), RBA::Edge::new(100, 200, 110, 210)), { 1 => "one" })
    ], false)
    assert_equal(cat1.num_items, 1)
    cn = []
    rdb.each_cell { |c| cn << c.to_s_items }
    assert_equal(cn.join(";"), "c1[edge-pair: (0,0;0.1,0.2)/(0.1,0.2;0.11,0.21)]")

  end

  def test_13

    # manipulations
    rdb = RBA::ReportDatabase::new("")

    _cell = rdb.create_cell("CELL")
    _cat = rdb.create_category("cat")
    _subcat = rdb.create_category(_cat, "subcat")
    _subcat.description = "subcat_d"
    _item1 = rdb.create_item(_cell.rdb_id, _subcat.rdb_id)
    _item1.add_value(17.5)
    _item1.add_value("string")
    _item2 = rdb.create_item(_cell.rdb_id, _subcat.rdb_id)
    _item2.add_value("b")
    _subsubcat = rdb.create_category(_subcat, "subsubcat")
    _cat2 = rdb.create_category("cat2")

    cell = rdb.cell_by_id(_cell.rdb_id)
    assert_equal(cell._is_const_object?, false)
    assert_equal(rdb.each_cell.to_a[0]._is_const_object?, false)

    cell = rdb.cell_by_qname("CELL")
    assert_equal(cell._is_const_object?, false)

    cat = rdb.category_by_id(_cat.rdb_id)
    assert_equal(cat._is_const_object?, false)

    cat = rdb.category_by_path("cat")
    assert_equal(cat._is_const_object?, false)
    subcat = rdb.category_by_path("cat.subcat")

    assert_equal(rdb.each_category.to_a[0]._is_const_object?, false)
    assert_equal(rdb.each_category.collect { |c| c.name }.join(","), "cat,cat2")
    assert_equal(subcat._is_const_object?, false)
    assert_equal(subcat.database._is_const_object?, false)
    assert_equal(subcat.name, "subcat")
    assert_equal(subcat.parent.name, "cat")

    assert_equal(subcat.description, "subcat_d")
    subcat.description = "changed"
    assert_equal(subcat.description, "changed")

    assert_equal(rdb.each_item_per_category(subcat.rdb_id).collect { |item| item.each_value.collect { |v| v.to_s }.join("/") }.join(";"), "float: 17.5/text: string;text: b")

    item1 = rdb.each_item_per_category(subcat.rdb_id).to_a[0]
    assert_equal(item1._is_const_object?, false)
    item1.clear_values
    assert_equal(rdb.each_item_per_category(subcat.rdb_id).collect { |item| item.each_value.collect { |v| v.to_s }.join("/") }.join(";"), ";text: b")
    item1.add_value("x")
    assert_equal(rdb.each_item_per_category(subcat.rdb_id).collect { |item| item.each_value.collect { |v| v.to_s }.join("/") }.join(";"), "text: x;text: b")
    item1.add_tag(17)
    assert_equal(item1.has_tag?(17), true)
    assert_equal(item1.has_tag?(16), false)

    item1 = rdb.each_item.to_a[0]
    assert_equal(item1._is_const_object?, false)
    assert_equal(item1.has_tag?(17), true)

    item1 = rdb.each_item_per_cell(cell.rdb_id).to_a[0]
    assert_equal(item1._is_const_object?, false)
    assert_equal(item1.has_tag?(17), true)

    item1 = rdb.each_item_per_cell_and_category(cell.rdb_id, subcat.rdb_id).to_a[0]
    assert_equal(item1._is_const_object?, false)
    assert_equal(item1.has_tag?(17), true)

    item1 = cell.each_item.to_a[0]
    assert_equal(item1._is_const_object?, false)
    assert_equal(item1.has_tag?(17), true)

  end

  def test_14

    # same names do not generate a new category
    rdb = RBA::ReportDatabase::new("")

    _cell = rdb.create_cell("CELL")

    _cat = rdb.create_category("cat")
    _cat_same = rdb.create_category("cat")
    assert_equal(_cat.rdb_id, _cat_same.rdb_id)

    _subcat = rdb.create_category(_cat, "subcat")
    _subcat_same = rdb.create_category(_cat_same, "subcat")
    assert_equal(_subcat.rdb_id, _subcat_same.rdb_id)

  end

end

load("test_epilogue.rb")

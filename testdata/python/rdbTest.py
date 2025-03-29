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

import pya
import unittest
import os
import sys

def to_s_test(self):
  r = self.name()
  r += "["
  refs = [ self.database().cell_by_id(i.parent_cell_id).name() + "->" + str(i.trans) for i in self.each_reference() ]
  r += ",".join(refs)
  r += "]"
  return r

def to_s_items(self):
  r = self.name()
  r += "["
  ai = []
  for i in self.each_item():
    vv = [ str(v) for v in i.each_value() ]
    ai.append("/".join(vv))
  r += ",".join(ai)
  r += "]"
  return r

pya.RdbCell.to_s_items = to_s_items
pya.RdbCell.to_s_test = to_s_test

class RDB_TestClass(unittest.TestCase):

  # RdbReference
  def test_1(self):

    ref = pya.RdbReference(pya.DCplxTrans(), 0) 
    self.assertEqual(ref.trans.__str__(), "r0 *1 0,0")
    ref.trans = pya.DCplxTrans(5.0)
    self.assertEqual(ref.trans.__str__(), "r0 *5 0,0")

    self.assertEqual(ref.parent_cell_id, 0)
    ref.parent_cell_id = 177
    self.assertEqual(ref.parent_cell_id, 177)

  # RdbCell, RdbCategory
  def test_2(self):

    db = pya.ReportDatabase("name")

    cell = db.create_cell("cell_name")
    self.assertEqual(cell.database().__repr__(), db.__repr__())
    self.assertEqual(cell.name(), "cell_name")
    self.assertEqual(cell.rdb_id(), 1)

    cell2 = db.create_cell("new_cell", "var1")
    self.assertEqual(cell2.name(), "new_cell")
    self.assertEqual(cell2.layout_name(), "")
    self.assertEqual(cell2.qname(), "new_cell:var1")

    cell2 = db.create_cell("cell_name", "var1", "cell_name$1")
    self.assertEqual(cell.name(), "cell_name")
    self.assertEqual(cell.qname(), "cell_name:1")
    self.assertEqual(db.cell_by_qname("cell_name:1").rdb_id(), cell.rdb_id())
    self.assertEqual(db.cell_by_id(cell.rdb_id()).rdb_id(), cell.rdb_id())
    self.assertEqual(cell2.name(), "cell_name")
    self.assertEqual(cell2.layout_name(), "cell_name$1")
    self.assertEqual(cell2.qname(), "cell_name:var1")
    self.assertEqual(db.cell_by_qname("cell_name:var1").rdb_id(), cell2.rdb_id())
    self.assertEqual(db.cell_by_id(cell2.rdb_id()).rdb_id(), cell2.rdb_id())
    self.assertEqual(cell2.rdb_id(), 3)
    self.assertEqual(cell.num_items(), 0)
    self.assertEqual(cell2.num_items(), 0)
    self.assertEqual(cell.num_items_visited(), 0)
    self.assertEqual(cell2.num_items_visited(), 0)

    cc = db.variants("cell_name")
    self.assertEqual(len(cc), 2)
    self.assertEqual(cc[0], cell.rdb_id())
    self.assertEqual(cc[1], cell2.rdb_id())
    
    cc = [ c for c in db.each_cell() ]
    self.assertEqual(len(cc), 3)
    self.assertEqual(cc[0].rdb_id(), cell.rdb_id())
    self.assertEqual(cc[2].rdb_id(), cell2.rdb_id())

    cat = db.create_category("cat")
    self.assertEqual(cat.database().__repr__(), db.__repr__())
    self.assertEqual(cat.name(), "cat")
    self.assertEqual(cat.rdb_id(), 4)
    self.assertEqual(cat.path(), "cat")

    cats = db.create_category(cat, "subcat")
    self.assertEqual(cats.name(), "subcat")
    self.assertEqual(cats.rdb_id(), 5)
    self.assertEqual(cats.path(), "cat.subcat")
    self.assertEqual(cats.parent().rdb_id(), cat.rdb_id())

    cat.description = "cat_desc"
    self.assertEqual(cat.description, "cat_desc")

    x = [ c for c in cat.each_sub_category() ]
    self.assertEqual(len(x), 1)
    self.assertEqual(x[0].rdb_id(), cats.rdb_id())

    item = db.create_item(cell.rdb_id(), cat.rdb_id())
    self.assertEqual(db.num_items(), 1)
    self.assertEqual(db.num_items_visited(), 0)
    self.assertEqual(db.num_items(cell.rdb_id(), cat.rdb_id()), 1)
    self.assertEqual(db.num_items_visited(cell.rdb_id(), cat.rdb_id()), 0)
    self.assertEqual(db.num_items(cell.rdb_id(), cats.rdb_id()), 0)
    self.assertEqual(db.num_items_visited(cell.rdb_id(), cats.rdb_id()), 0)
    self.assertEqual(cell.num_items(), 1)
    self.assertEqual(cell2.num_items(), 0)
    self.assertEqual(cat.num_items(), 1)
    self.assertEqual(cats.num_items(), 0)
    self.assertEqual(cell.num_items_visited(), 0)
    self.assertEqual(cell2.num_items_visited(), 0)
    self.assertEqual(cat.num_items_visited(), 0)
    self.assertEqual(cats.num_items_visited(), 0)

    try:
      item = db.create_item(1000, cat.rdb_id())
      self.assertEqual(False, True)
    except Exception as ex: 
      self.assertEqual(ex.__str__(), "Not a valid cell ID: 1000 in ReportDatabase.create_item")

    try:
      item = db.create_item(cell.rdb_id(), 1001)
      self.assertEqual(False, True)
    except Exception as ex: 
      self.assertEqual(ex.__str__(), "Not a valid category ID: 1001 in ReportDatabase.create_item")

    item2 = db.create_item(cell2, cats)
    self.assertEqual(db.num_items(), 2)
    self.assertEqual(db.num_items_visited(), 0)
    self.assertEqual(db.num_items(cell2.rdb_id(), cats.rdb_id()), 1)
    self.assertEqual(db.num_items_visited(cell2.rdb_id(), cats.rdb_id()), 0)
    self.assertEqual(db.num_items(cell2.rdb_id(), cat.rdb_id()), 1)
    self.assertEqual(db.num_items_visited(cell2.rdb_id(), cat.rdb_id()), 0)
    self.assertEqual(cell.num_items(), 1)
    self.assertEqual(cell2.num_items(), 1)
    self.assertEqual(cat.num_items(), 2)
    self.assertEqual(cats.num_items(), 1)
    self.assertEqual(cell.num_items_visited(), 0)
    self.assertEqual(cell2.num_items_visited(), 0)
    self.assertEqual(cat.num_items_visited(), 0)
    self.assertEqual(cats.num_items_visited(), 0)

    db.set_item_visited(item, True)
    self.assertEqual(item.is_visited(), True)
    self.assertEqual(cell.num_items_visited(), 1)
    self.assertEqual(cell2.num_items_visited(), 0)
    self.assertEqual(cat.num_items_visited(), 1)
    self.assertEqual(cats.num_items_visited(), 0)
    self.assertEqual(db.num_items(), 2)
    self.assertEqual(db.num_items_visited(), 1)
    self.assertEqual(db.num_items(cell.rdb_id(), cat.rdb_id()), 1)
    self.assertEqual(db.num_items_visited(cell.rdb_id(), cat.rdb_id()), 1)
    self.assertEqual(db.num_items(cell2.rdb_id(), cats.rdb_id()), 1)
    self.assertEqual(db.num_items_visited(cell2.rdb_id(), cats.rdb_id()), 0)
    self.assertEqual(db.num_items(cell.rdb_id(), cats.rdb_id()), 0)
    self.assertEqual(db.num_items_visited(cell.rdb_id(), cats.rdb_id()), 0)

    db.set_item_visited(item2, True)
    self.assertEqual(cell.num_items_visited(), 1)
    self.assertEqual(cell2.num_items_visited(), 1)
    self.assertEqual(cat.num_items_visited(), 2)
    self.assertEqual(cats.num_items_visited(), 1)
    self.assertEqual(db.num_items(), 2)
    self.assertEqual(db.num_items_visited(), 2)
    self.assertEqual(db.num_items(cell.rdb_id(), cat.rdb_id()), 1)
    self.assertEqual(db.num_items_visited(cell.rdb_id(), cat.rdb_id()), 1)
    self.assertEqual(db.num_items(cell2.rdb_id(), cats.rdb_id()), 1)
    self.assertEqual(db.num_items_visited(cell2.rdb_id(), cats.rdb_id()), 1)
    self.assertEqual(db.num_items(cell.rdb_id(), cats.rdb_id()), 0)
    self.assertEqual(db.num_items_visited(cell.rdb_id(), cats.rdb_id()), 0)

    db.set_item_visited(item, False)
    self.assertEqual(item.is_visited(), False)
    self.assertEqual(cell.num_items_visited(), 0)
    self.assertEqual(cell2.num_items_visited(), 1)
    self.assertEqual(cat.num_items_visited(), 1)
    self.assertEqual(cats.num_items_visited(), 1)
    self.assertEqual(db.num_items(), 2)
    self.assertEqual(db.num_items_visited(), 1)
    self.assertEqual(db.num_items(cell.rdb_id(), cat.rdb_id()), 1)
    self.assertEqual(db.num_items_visited(cell.rdb_id(), cat.rdb_id()), 0)
    self.assertEqual(db.num_items(cell2.rdb_id(), cats.rdb_id()), 1)
    self.assertEqual(db.num_items_visited(cell2.rdb_id(), cats.rdb_id()), 1)
    self.assertEqual(db.num_items(cell.rdb_id(), cats.rdb_id()), 0)
    self.assertEqual(db.num_items_visited(cell.rdb_id(), cats.rdb_id()), 0)

    ii = [ i for i in db.each_item() ]
    self.assertEqual(len(ii), 2)
    self.assertEqual(ii[0].cell_id(), item.cell_id())
    self.assertEqual(ii[0].category_id(), item.category_id())
    self.assertEqual(ii[1].cell_id(), item2.cell_id())
    self.assertEqual(ii[1].category_id(), item2.category_id())

    ii = [ i for i in db.each_item_per_cell(cell.rdb_id()) ]
    self.assertEqual(len(ii), 1)
    self.assertEqual(ii[0].cell_id(), item.cell_id())
    self.assertEqual(ii[0].category_id(), item.category_id())

    ii = [ i for i in cell.each_item() ]
    self.assertEqual(len(ii), 1)
    self.assertEqual(ii[0].cell_id(), item.cell_id())
    self.assertEqual(ii[0].category_id(), item.category_id())

    ii = [ i for i in db.each_item_per_category(cats.rdb_id()) ]
    self.assertEqual(len(ii), 1)
    self.assertEqual(ii[0].cell_id(), item2.cell_id())
    self.assertEqual(ii[0].category_id(), item2.category_id())

    ii = [ i for i in cats.each_item() ]
    self.assertEqual(len(ii), 1)
    self.assertEqual(ii[0].cell_id(), item2.cell_id())
    self.assertEqual(ii[0].category_id(), item2.category_id())

    ii = [ i for i in db.each_item_per_cell_and_category(cell.rdb_id(), cats.rdb_id()) ]
    self.assertEqual(len(ii), 0)

    ii = [ i for i in db.each_item_per_cell_and_category(cell2.rdb_id(), cats.rdb_id()) ]
    self.assertEqual(len(ii), 1)
    self.assertEqual(ii[0].cell_id(), item2.cell_id())
    self.assertEqual(ii[0].category_id(), item2.category_id())

    refs = [ r for r in cell.each_reference() ]
    self.assertEqual(len(refs), 0)

    cell.add_reference(pya.RdbReference(pya.DCplxTrans(2.5), 178))
    refs = [ r for r in cell.each_reference() ]
    self.assertEqual(len(refs), 1)
    self.assertEqual(refs[0].parent_cell_id, 178)
    self.assertEqual(refs[0].database().__repr__(), db.__repr__())

    cell.clear_references()
    refs = [ r for r in cell.each_reference() ]
    self.assertEqual(len(refs), 0)

  # RdbItemValue
  def test_3(self):

    v = pya.RdbItemValue(1.0)
    self.assertEqual(v.tag_id, 0)
    v.tag_id = 15
    self.assertEqual(v.tag_id, 15)

    vf = pya.RdbItemValue(1.0)
    vs = pya.RdbItemValue("a string")
    vb = pya.RdbItemValue(pya.DBox(0, 10, 20, 30))
    vl = pya.RdbItemValue(pya.DText("abc", pya.DTrans()))
    vp = pya.RdbItemValue(pya.DPolygon(pya.DBox(100, 101, 102, 103)))
    ve = pya.RdbItemValue(pya.DEdge(pya.DPoint(0, 10), pya.DPoint(20, 30)))
    vee = pya.RdbItemValue(pya.DEdgePair(pya.DEdge(0, 10, 5, 15), pya.DEdge(20, 30, 25, 35)))

    self.assertEqual(vf.__str__(), "float: 1")
    self.assertEqual(vs.__str__(), "text: 'a string'")
    self.assertEqual(vb.__str__(), "box: (0,10;20,30)")
    self.assertEqual(vp.__str__(), "polygon: (100,101;100,103;102,103;102,101)")
    self.assertEqual(vl.__str__(), "label: ('abc',r0 0,0)");
    self.assertEqual(ve.__str__(), "edge: (0,10;20,30)")
    self.assertEqual(vee.__str__(), "edge-pair: (0,10;5,15)/(20,30;25,35)")
    self.assertEqual(pya.RdbItemValue.from_s(vf.__str__()).__str__(), vf.__str__())
    self.assertEqual(pya.RdbItemValue.from_s(vs.__str__()).__str__(), vs.__str__())
    self.assertEqual(pya.RdbItemValue.from_s(vb.__str__()).__str__(), vb.__str__())
    self.assertEqual(pya.RdbItemValue.from_s(vp.__str__()).__str__(), vp.__str__())
    self.assertEqual(pya.RdbItemValue.from_s(vl.__str__()).__str__(), vl.__str__())
    self.assertEqual(pya.RdbItemValue.from_s(ve.__str__()).__str__(), ve.__str__())
    self.assertEqual(pya.RdbItemValue.from_s(vee.__str__()).__str__(), vee.__str__())

    self.assertEqual(vf.is_float(), True)
    self.assertEqual(vf.is_string(), False)
    self.assertEqual(vf.is_polygon(), False)
    self.assertEqual(vf.is_text(), False)
    self.assertEqual(vf.is_edge(), False)
    self.assertEqual(vf.is_box(), False)
    self.assertEqual(vf.is_edge_pair(), False)
    self.assertEqual(vf.float(), 1)
    self.assertEqual(vf.string(), "1")

    self.assertEqual(vs.is_float(), False)
    self.assertEqual(vs.is_string(), True)
    self.assertEqual(vs.is_polygon(), False)
    self.assertEqual(vs.is_text(), False)
    self.assertEqual(vs.is_edge(), False)
    self.assertEqual(vs.is_box(), False)
    self.assertEqual(vs.is_edge_pair(), False)
    self.assertEqual(vs.string(), "a string")

    self.assertEqual(vl.is_float(), False)
    self.assertEqual(vl.is_string(), False)
    self.assertEqual(vl.is_polygon(), False)
    self.assertEqual(vl.is_text(), True)
    self.assertEqual(vl.is_edge(), False)
    self.assertEqual(vl.is_box(), False)
    self.assertEqual(vl.is_edge_pair(), False)
    self.assertEqual(vl.text().__str__(), "('abc',r0 0,0)")
    self.assertEqual(vl.string(), "label: " + vl.text().__str__())

    self.assertEqual(vp.is_float(), False)
    self.assertEqual(vp.is_string(), False)
    self.assertEqual(vp.is_polygon(), True)
    self.assertEqual(vp.is_text(), False)
    self.assertEqual(vp.is_edge(), False)
    self.assertEqual(vp.is_box(), False)
    self.assertEqual(vp.is_edge_pair(), False)
    self.assertEqual(vp.polygon().__str__(), "(100,101;100,103;102,103;102,101)")
    self.assertEqual(vp.string(), "polygon: " + vp.polygon().__str__())

    self.assertEqual(ve.is_float(), False)
    self.assertEqual(ve.is_string(), False)
    self.assertEqual(ve.is_polygon(), False)
    self.assertEqual(ve.is_text(), False)
    self.assertEqual(ve.is_edge(), True)
    self.assertEqual(ve.is_box(), False)
    self.assertEqual(ve.is_edge_pair(), False)
    self.assertEqual(ve.edge().__str__(), "(0,10;20,30)")
    self.assertEqual(ve.string(), "edge: " + ve.edge().__str__())

    self.assertEqual(vb.is_float(), False)
    self.assertEqual(vb.is_string(), False)
    self.assertEqual(vb.is_polygon(), False)
    self.assertEqual(vb.is_text(), False)
    self.assertEqual(vb.is_edge(), False)
    self.assertEqual(vb.is_box(), True)
    self.assertEqual(vb.is_edge_pair(), False)
    self.assertEqual(vb.box().__str__(), "(0,10;20,30)")
    self.assertEqual(vb.string(), "box: " + vb.box().__str__())

    self.assertEqual(vee.is_float(), False)
    self.assertEqual(vee.is_string(), False)
    self.assertEqual(vee.is_polygon(), False)
    self.assertEqual(vee.is_text(), False)
    self.assertEqual(vee.is_edge(), False)
    self.assertEqual(vee.is_box(), False)
    self.assertEqual(vee.is_edge_pair(), True)
    self.assertEqual(vee.edge_pair().__str__(), "(0,10;5,15)/(20,30;25,35)")
    self.assertEqual(vee.string(), "edge-pair: " + vee.edge_pair().__str__())

  # RdbItem
  def test_4(self):

    db = pya.ReportDatabase("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    item = db.create_item(cell.rdb_id(), cat.rdb_id())
    self.assertEqual(item.database().__repr__(), db.__repr__())
    self.assertEqual(item.cell_id(), cell.rdb_id())
    self.assertEqual(item.category_id(), cat.rdb_id())

    self.assertEqual(db.user_tag_id("x1") != 0, True)
    self.assertEqual(db.tag_id("x1") != 0, True)
    self.assertEqual(db.tag_id("x1") == db.user_tag_id("x1"), False)
    db.set_tag_description(db.user_tag_id("x1"), "D")
    self.assertEqual(db.tag_description(db.user_tag_id("x1")), "D")
    self.assertEqual(db.tag_name(db.user_tag_id("x1")), "x1")
    self.assertEqual(db.tag_description(db.tag_id("x1")), "")
    self.assertEqual(db.tag_name(db.tag_id("x1")), "x1")
    db.set_tag_description(db.tag_id("x1"), "U")
    self.assertEqual(db.tag_description(db.user_tag_id("x1")), "D")
    self.assertEqual(db.tag_description(db.tag_id("x1")), "U")

    item.add_tag(db.tag_id("x1"))
    self.assertEqual(item.has_tag(db.tag_id("x2")), False)
    self.assertEqual(item.has_tag(db.tag_id("x1")), True)
    self.assertEqual(item.tags_str, "x1")
    item.add_tag(db.tag_id("x2"))
    self.assertEqual(item.has_tag(db.tag_id("x2")), True)
    self.assertEqual(item.has_tag(db.tag_id("x1")), True)
    self.assertEqual(item.tags_str, "x1,x2")
    item.remove_tag(db.tag_id("x1"))
    self.assertEqual(item.has_tag(db.tag_id("x2")), True)
    self.assertEqual(item.has_tag(db.tag_id("x1")), False)
    self.assertEqual(item.tags_str, "x2")

    item.tags_str="x2,x1"
    self.assertEqual(item.has_tag(db.tag_id("x2")), True)
    self.assertEqual(item.has_tag(db.tag_id("x1")), True)
    self.assertEqual(item.tags_str, "x1,x2")

    item.tags_str=""
    self.assertEqual(item.has_tag(db.tag_id("x2")), False)
    self.assertEqual(item.has_tag(db.tag_id("x1")), False)
    self.assertEqual(item.tags_str, "")
    
    self.assertEqual(item.image_str, "")
    self.assertEqual(item.has_image(), False)

    self.assertEqual(item.comment, "")
    item.comment = "abc"
    self.assertEqual(item.comment, "abc")

    # can actually by any string, but only base64-encoded PNG images make sense
    istr="iVBORw0KGgoAAAANSUhEUgAAACoAAAA0CAIAAABzfT3nAAAAA3NCSVQICAjb4U/gAAAACXBIWXMAAA0SAAANOgHo3ZneAAAA3UlEQVRYhe2WwQ3DIAxFoco8XaGZIaeO43FyYgZYgYXcQ6SWuDGgBhWq/qccIvGCEd9SbAwAAPSGaW2lFR2rfWDpXrPpSe2SP10fvnn/PZHZH9IwbKFVZZ/Z6wMtZcjW02Bn2FVpZYdWdkr2nvh23S2FyDNJuVITpwmRjTGbNr0v20U5byNtJuuJt/fO2f93+UlbEJl5UjVPr3Y71EQ/PoPPlU+lDJtWlCt3GwCMG33BuJGAcWMEMG6c1jBudCyf/nzV8nbZPRohclFLHdGbZ8eNSjN1fmf0AACA1jwA4hKxu4C6P7EAAAAASUVORK5CYII="
    item.image_str=istr
    self.assertEqual(item.image_str, istr)
    self.assertEqual(item.has_image(), True)

    if "image_pixels" in item.__dict__:
      px=item.image_pixels
      self.assertEqual(px.width, 42)
      self.assertEqual(px.height, 52)
      item.image = px
      px2=item.image_pixels
      self.assertEqual(px == px2, True)
    
    if "image" in item.__dict__:
      px=item.image
      self.assertEqual(px.width, 42)
      self.assertEqual(px.height, 52)
      item.image = px
      px2=item.image
      self.assertEqual(px2.width, 42)
      self.assertEqual(px2.height, 52)
    
    vs = pya.RdbItemValue("a string")
    vs2 = pya.RdbItemValue("a string (ii)")
    item.add_value(vs)
    item.add_value(vs2)

    vv=[ v for v in item.each_value() ]
    self.assertEqual(len(vv), 2)
    self.assertEqual(vv[0].__str__(), "text: 'a string'")
    self.assertEqual(vv[1].__str__(), "text: 'a string (ii)'")

    item.clear_values()
    vv=[ v for v in item.each_value() ]
    self.assertEqual(len(vv), 0)

    item.clear_values()
    item.add_value(1.0)
    item.add_value("hello")
    item.add_value(pya.DPolygon(pya.DBox(1, 2, 3, 4)))
    item.add_value(pya.DBox(11, 12, 13, 14))
    item.add_value(pya.DEdge(21, 22, 23, 24))
    item.add_value(pya.DEdgePair(pya.DEdge(31, 32, 33, 34), pya.DEdge(41, 42, 43, 44)))
    shapes = pya.Shapes()
    pts = [ pya.Point(0, 0), pya.Point(50, 150) ]
    shapes.insert(pya.Path(pts, 100))
    for s in shapes:
      item.add_value(s, pya.CplxTrans(0.001))
    vv=[ v for v in item.each_value() ]
    self.assertEqual(len(vv), 7)
    self.assertEqual(vv[0].__str__(), "float: 1")
    self.assertEqual(vv[1].__str__(), "text: hello")
    self.assertEqual(vv[2].__str__(), "polygon: (1,2;1,4;3,4;3,2)")
    self.assertEqual(vv[3].__str__(), "box: (11,12;13,14)")
    self.assertEqual(vv[4].__str__(), "edge: (21,22;23,24)")
    self.assertEqual(vv[5].__str__(), "edge-pair: (31,32;33,34)/(41,42;43,44)")
    self.assertEqual(vv[6].__str__(), "path: (0,0;0.05,0.15) w=0.1 bx=0 ex=0 r=false")

  # Multiple items
  def test_5(self):

    db = pya.ReportDatabase("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    r = pya.Region(pya.Box(0, 0, 100, 200))
    db.create_items(cell.rdb_id(), cat.rdb_id(), pya.CplxTrans(0.001), r)
    a = []
    for item in db.each_item_per_category(cat.rdb_id()):
      for v in item.each_value():
        a.append(str(v))
    self.assertEqual(";".join(a), "polygon: (0,0;0,0.2;0.1,0.2;0.1,0)")

    db = pya.ReportDatabase("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    p = pya.Polygon(pya.Box(0, 0, 100, 200))
    db.create_items(cell.rdb_id(), cat.rdb_id(), pya.CplxTrans(0.001), [p])
    a = []
    for item in db.each_item_per_category(cat.rdb_id()):
      for v in item.each_value():
        a.append(str(v))
    self.assertEqual(";".join(a), "polygon: (0,0;0,0.2;0.1,0.2;0.1,0)")

    db = pya.ReportDatabase("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    r = pya.Edges(pya.Edge(0, 0, 100, 200))
    db.create_items(cell.rdb_id(), cat.rdb_id(), pya.CplxTrans(0.001), r)
    a = []
    for item in db.each_item_per_category(cat.rdb_id()):
      for v in item.each_value():
        a.append(str(v))
    self.assertEqual(";".join(a), "edge: (0,0;0.1,0.2)")

    db = pya.ReportDatabase("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    r = [ pya.Edge(0, 0, 100, 200) ]
    db.create_items(cell.rdb_id(), cat.rdb_id(), pya.CplxTrans(0.001), r)
    a = []
    for item in db.each_item_per_category(cat.rdb_id()):
      for v in item.each_value():
        a.append(str(v))
    self.assertEqual(";".join(a), "edge: (0,0;0.1,0.2)")

    db = pya.ReportDatabase("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    r = pya.EdgePairs()
    r.insert(pya.EdgePair(pya.Edge(0, 0, 100, 200), pya.Edge(10, 10, 50, 150)))
    db.create_items(cell.rdb_id(), cat.rdb_id(), pya.CplxTrans(0.001), r)
    a = []
    for item in db.each_item_per_category(cat.rdb_id()):
      for v in item.each_value():
        a.append(str(v))
    self.assertEqual(";".join(a), "edge-pair: (0,0;0.1,0.2)/(0.01,0.01;0.05,0.15)")

    db = pya.ReportDatabase("name")

    cell = db.create_cell("cell_name")
    cat = db.create_category("cat")

    r = [ pya.EdgePair(pya.Edge(0, 0, 100, 200), pya.Edge(10, 10, 50, 150)) ]
    db.create_items(cell.rdb_id(), cat.rdb_id(), pya.CplxTrans(0.001), r)
    a = []
    for item in db.each_item_per_category(cat.rdb_id()):
      for v in item.each_value():
        a.append(str(v))
    self.assertEqual(";".join(a), "edge-pair: (0,0;0.1,0.2)/(0.01,0.01;0.05,0.15)")

  # ReportDatabase
  def test_6(self):

    db = pya.ReportDatabase("name")
    db.reset_modified()
    self.assertEqual(db.is_modified(), False)
    self.assertEqual(db.name(), "name")
    db.description = "desc"
    db.generator = "gg"
    db.top_cell_name = "top"
    db.original_file = "of"
    self.assertEqual(db.description, "desc")
    self.assertEqual(db.generator, "gg")
    self.assertEqual(db.top_cell_name, "top")
    self.assertEqual(db.original_file, "of")

    self.assertEqual(db.is_modified(), True)
    db.reset_modified()
    self.assertEqual(db.is_modified(), False)

    tag_id = db.tag_id("x")
    self.assertEqual(tag_id, 1)
    db.set_tag_description(tag_id, "xdesc")
    self.assertEqual(db.tag_description(tag_id), "xdesc")

    cell = db.create_cell("cell_name")
    cc = [ c for c in db.each_cell() ]
    self.assertEqual(len(cc), 1)
    self.assertEqual(cc[0].rdb_id(), cell.rdb_id())

    cat = db.create_category("cat")
    cc = [ c for c in db.each_category() ]
    self.assertEqual(len(cc), 1)
    self.assertEqual(cc[0].rdb_id(), cat.rdb_id())

    cats = db.create_category(cat, "subcat")
    c = db.category_by_path("x")
    self.assertEqual(c, None)
    c = db.category_by_path("cat")
    self.assertEqual(c.rdb_id(), cat.rdb_id())
    c = db.category_by_path("cat.subcat")
    self.assertEqual(c.rdb_id(), cats.rdb_id())

    self.assertEqual(db.category_by_id(cat.rdb_id()).rdb_id(), cat.rdb_id())
    self.assertEqual(db.category_by_id(cats.rdb_id()).rdb_id(), cats.rdb_id())

    item = db.create_item(cell.rdb_id(), cat.rdb_id())
    v = pya.RdbItemValue("a")
    v.tag_id = db.user_tag_id("x2")
    item.add_value(v)
    v = pya.RdbItemValue("b")
    v.tag_id = db.tag_id("x1")
    item.add_value(v)
    item.add_tag(db.tag_id("x1"))
    item.add_tag(db.user_tag_id("x2"))
    istr="iVBORw0KGgoAAAANSUhEUgAAACoAAAA0CAIAAABzfT3nAAAAA3NCSVQICAjb4U/gAAAACXBIWXMAAA0SAAANOgHo3ZneAAAA3UlEQVRYhe2WwQ3DIAxFoco8XaGZIaeO43FyYgZYgYXcQ6SWuDGgBhWq/qccIvGCEd9SbAwAAPSGaW2lFR2rfWDpXrPpSe2SP10fvnn/PZHZH9IwbKFVZZ/Z6wMtZcjW02Bn2FVpZYdWdkr2nvh23S2FyDNJuVITpwmRjTGbNr0v20U5byNtJuuJt/fO2f93+UlbEJl5UjVPr3Y71EQ/PoPPlU+lDJtWlCt3GwCMG33BuJGAcWMEMG6c1jBudCyf/nzV8nbZPRohclFLHdGbZ8eNSjN1fmf0AACA1jwA4hKxu4C6P7EAAAAASUVORK5CYII="
    if "image_str" in item.__dict__:
      item.image_str=istr

    ut_testtmp = os.getenv("TESTTMP", ".")
    tmp = os.path.join(ut_testtmp, "tmp.lyrdb")

    self.assertEqual(db.filename(), "")
    db.save(tmp)
    self.assertEqual(db.filename(), tmp)

    # load and save
    db = None

    db2 = pya.ReportDatabase("neu")
    db2.load(tmp)
    self.assertEqual(os.path.basename(db2.filename()), os.path.basename(tmp))
    self.assertEqual(db2.name(), os.path.basename(tmp))

    self.assertEqual(db2.description, "desc")
    self.assertEqual(db2.generator, "gg")
    self.assertEqual(db2.top_cell_name, "top")
    self.assertEqual(db2.original_file, "of")

    c = db2.category_by_path("cat.subcat")
    self.assertEqual(c.path(), "cat.subcat")

    cc = [ c for c in db2.each_category() ]
    self.assertEqual(len(cc), 1)

    ii = [ i for i in db2.each_item() ]
    self.assertEqual(len(ii), 1)
    self.assertEqual(ii[0].tags_str, "x1,#x2")
    self.assertEqual(ii[0].has_tag(db2.user_tag_id("x2")), True)
    self.assertEqual(ii[0].has_tag(db2.tag_id("x1")), True)
    self.assertEqual(ii[0].has_tag(db2.tag_id("x")), False)
    # Only the first 30 bytes count ... the remaining part is too different for different versions of Qt
    if "image_str" in ii[0].__dict__:
      self.assertEqual(ii[0].image_str[range(0, 30)], istr[range(0, 30)])
    self.assertEqual(db2.cell_by_id(ii[0].cell_id()).qname(), "cell_name")
    self.assertEqual(db2.category_by_id(ii[0].category_id()).path(), "cat")
    vs = ""
    for v in ii[0].each_value():
      vs += v.string()
    self.assertEqual(vs, "ab")
    vs = ""
    for v in ii[0].each_value():
      if v.tag_id == db2.tag_id("x1"):
        vs += v.string()
    self.assertEqual(vs, "b")
    vs = ""
    for v in ii[0].each_value():
      if v.tag_id == db2.user_tag_id("x1"):
        vs += v.string()
    self.assertEqual(vs, "")
    vs = ""
    for v in ii[0].each_value():
      if v.tag_id == db2.user_tag_id("x2"):
        vs += v.string()
    self.assertEqual(vs, "a")

  # LayoutView
  def test_10(self):

    if not "Application" in pya.__dict__:
      return

    mw = pya.Application.instance().main_window()
    mw.create_layout(1)
    view = mw.current_view()

    self.ot = 0
    def on_changed():
      self.ot += 1
    view.on_rdb_list_changed += on_changed

    rdb_index = view.create_rdb("NEW_RDB")
    self.assertEqual(view.num_rdbs(), 1)
    self.assertEqual(self.ot, 1)
    self.assertEqual(view.rdb(rdb_index).name(), "NEW_RDB")
    view.remove_rdb(rdb_index)
    self.assertEqual(view.num_rdbs(), 0)
    self.assertEqual(self.ot, 2)

    view.on_rdb_list_changed -= on_changed

    self.ot = 0
    rdb_index = view.create_rdb("NEW_RDB2")
    self.assertEqual(view.rdb(rdb_index).name(), "NEW_RDB2")
    self.assertEqual(self.ot, 0)
    self.assertEqual(view.num_rdbs(), 1)
    rdb_index = view.create_rdb("NEW_RDB3")
    self.assertEqual(view.rdb(rdb_index).name(), "NEW_RDB3")
    self.assertEqual(view.num_rdbs(), 2)
    self.assertEqual(self.ot, 0)

    mw.close_current_view()

  # scan_... methods
  def test_11(self):

    ly = pya.Layout()
    c0 = ly.create_cell("c0")
    c1 = ly.create_cell("c1")
    c2 = ly.create_cell("c2")
    c3 = ly.create_cell("c3")
    c1.insert(pya.CellInstArray(c2.cell_index(), pya.Trans(10, 20)))
    c2.insert(pya.CellInstArray(c3.cell_index(), pya.Trans(11, 21)))
    l1 = ly.insert_layer(pya.LayerInfo(1, 0))
    prop_id = ly.properties_id([[ "a", 17 ]])
    c0.shapes(l1).insert(pya.Box(0, 1, 2, 3), prop_id)
    prop_id = ly.properties_id([[ "a", 21 ]])
    c1.shapes(l1).insert(pya.Box(0, 1, 20, 30), prop_id)
    c2.shapes(l1).insert(pya.Box(0, 1, 21, 31))
    c3.shapes(l1).insert(pya.Box(0, 1, 22, 32))

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1)
    self.assertEqual(rdb.tag_name(1), "a") # from property
    self.assertEqual(cat.num_items(), 4)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c0[];c1[];c2[];c3[]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c0[polygon: (0,0.001;0,0.003;0.002,0.003;0.002,0.001)/float: 17];c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1, None, -1, False)
    self.assertEqual(cat.num_items(), 4)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c0[];c1[];c2[];c3[]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c0[polygon: (0,0.001;0,0.003;0.002,0.003;0.002,0.001)];c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1, c1)
    self.assertEqual(cat.num_items(), 3)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[];c2[c1->r0 *1 0.01,0.02];c3[c1->r0 *1 0.021,0.041]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1, c1, 0)
    self.assertEqual(cat.num_items(), 1)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1, c1, -1)
    self.assertEqual(cat.num_items(), 3)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[];c2[c1->r0 *1 0.01,0.02];c3[c1->r0 *1 0.021,0.041]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    cat.scan_layer(ly, l1, c1, 1)
    self.assertEqual(cat.num_items(), 2)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[];c2[c1->r0 *1 0.01,0.02]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    cat.scan_shapes(c1.begin_shapes_rec(l1))  # hierarchical scan
    self.assertEqual(cat.num_items(), 3)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[];c2[c1->r0 *1 0.01,0.02];c3[c1->r0 *1 0.021,0.041]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    cat.scan_shapes(c1.begin_shapes_rec(l1), False, False)  # hierarchical scan
    self.assertEqual(cat.num_items(), 3)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[];c2[c1->r0 *1 0.01,0.02];c3[c1->r0 *1 0.021,0.041]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    cat.scan_shapes(c1.begin_shapes_rec(l1), True)  # flat scan
    self.assertEqual(cat.num_items(), 3)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21,polygon: (0.01,0.021;0.01,0.051;0.031,0.051;0.031,0.021),polygon: (0.021,0.042;0.021,0.073;0.043,0.073;0.043,0.042)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    r = pya.Region(c1.begin_shapes_rec(l1))
    cat.scan_collection(rdb.create_cell("TOP"), pya.CplxTrans(0.001), r)  # hierarchical scan
    self.assertEqual(cat.num_items(), 3)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "TOP[];c1[TOP->r0 *1 0,0];c2[c1->r0 *1 0.01,0.02];c3[c1->r0 *1 0.021,0.041]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "TOP[];c1[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21];c2[polygon: (0,0.001;0,0.031;0.021,0.031;0.021,0.001)];c3[polygon: (0,0.001;0,0.032;0.022,0.032;0.022,0.001)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    r = pya.Region(c1.begin_shapes_rec(l1))
    cat.scan_collection(rdb.create_cell("TOP"), pya.CplxTrans(0.001), r, True)  # flat scan
    self.assertEqual(cat.num_items(), 3)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "TOP[]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "TOP[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001)/float: 21,polygon: (0.01,0.021;0.01,0.051;0.031,0.051;0.031,0.021),polygon: (0.021,0.042;0.021,0.073;0.043,0.073;0.043,0.042)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    r = pya.Region(c1.begin_shapes_rec(l1))
    cat.scan_collection(rdb.create_cell("TOP"), pya.CplxTrans(0.001), r, True, False)  # flat scan
    self.assertEqual(cat.num_items(), 3)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "TOP[]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "TOP[polygon: (0,0.001;0,0.03;0.02,0.03;0.02,0.001),polygon: (0.01,0.021;0.01,0.051;0.031,0.051;0.031,0.021),polygon: (0.021,0.042;0.021,0.073;0.043,0.073;0.043,0.042)]")

    rdb = pya.ReportDatabase("neu")
    cat = rdb.create_category("l1")
    r = pya.Region(c1.begin_shapes_rec(l1)).merged()
    cat.scan_collection(rdb.create_cell("TOP"), pya.CplxTrans(0.001), r, True)  # flat scan
    self.assertEqual(cat.num_items(), 1)
    cn = [ c.to_s_test() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "TOP[]")
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "TOP[polygon: (0,0.001;0,0.03;0.01,0.03;0.01,0.051;0.021,0.051;0.021,0.073;0.043,0.073;0.043,0.042;0.031,0.042;0.031,0.021;0.02,0.021;0.02,0.001)]")

  # shape insertion from shape, shapes, recursive iterator
  def test_12(self):

    ly = pya.Layout()
    c0 = ly.create_cell("c0")
    c1 = ly.create_cell("c1")
    c2 = ly.create_cell("c2")
    c3 = ly.create_cell("c3")
    c1.insert(pya.CellInstArray(c2.cell_index(), pya.Trans(10, 20)))
    c2.insert(pya.CellInstArray(c3.cell_index(), pya.Trans(11, 21)))
    l1 = ly.insert_layer(pya.LayerInfo(1, 0))
    c0.shapes(l1).insert(pya.Box(0, 1, 2, 3))
    c1.shapes(l1).insert(pya.Text("Hello, world!", pya.Trans()))
    c2.shapes(l1).insert(pya.Edge(0, 1, 21, 31))
    c3.shapes(l1).insert(pya.Polygon(pya.Box(0, 1, 22, 32)))

    rdb = pya.ReportDatabase("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    for s in c0.shapes(l1).each():
      rdb.create_item(cell1.rdb_id(), cat1.rdb_id(), pya.CplxTrans(ly.dbu), s)
    self.assertEqual(cat1.num_items(), 1)
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[polygon: (0,0.001;0,0.003;0.002,0.003;0.002,0.001)]")

    rdb = pya.ReportDatabase("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id(), cat1.rdb_id(), pya.CplxTrans(ly.dbu), c0.shapes(l1))
    self.assertEqual(cat1.num_items(), 1)
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[polygon: (0,0.001;0,0.003;0.002,0.003;0.002,0.001)]")

    rdb = pya.ReportDatabase("neu")
    cat1 = rdb.create_category("l1")
    cell1 = rdb.create_cell("c1")
    rdb.create_items(cell1.rdb_id(), cat1.rdb_id(), c1.begin_shapes_rec(l1))
    self.assertEqual(cat1.num_items(), 3)
    cn = [ c.to_s_items() for c in rdb.each_cell() ]
    self.assertEqual(";".join(cn), "c1[label: ('Hello, world!',r0 0,0),edge: (0.01,0.021;0.031,0.051),polygon: (0.021,0.042;0.021,0.073;0.043,0.073;0.043,0.042)]")

  def test_13(self):

    # manipulations
    rdb = pya.ReportDatabase("")

    _cell = rdb.create_cell("CELL")
    _cat = rdb.create_category("cat")
    _subcat = rdb.create_category(_cat, "subcat")
    _subcat.description = "subcat_d"
    _item1 = rdb.create_item(_cell.rdb_id(), _subcat.rdb_id())
    _item1.add_value(17.5)
    _item1.add_value("string")
    _item2 = rdb.create_item(_cell.rdb_id(), _subcat.rdb_id())
    _item2.add_value("b")
    _subsubcat = rdb.create_category(_subcat, "subsubcat")
    _cat2 = rdb.create_category("cat2")

    cell = rdb.cell_by_id(_cell.rdb_id())
    self.assertEqual(cell._is_const_object(), False)
    self.assertEqual([ c for c in rdb.each_cell() ][0]._is_const_object(), False)

    cell = rdb.cell_by_qname("CELL")
    self.assertEqual(cell._is_const_object(), False)

    cat = rdb.category_by_id(_cat.rdb_id())
    self.assertEqual(cat._is_const_object(), False)

    cat = rdb.category_by_path("cat")
    self.assertEqual(cat._is_const_object(), False)
    subcat = rdb.category_by_path("cat.subcat")

    self.assertEqual([ c for c in rdb.each_category() ][0]._is_const_object(), False)
    self.assertEqual(",".join([ c.name() for c in rdb.each_category() ]), "cat,cat2")
    self.assertEqual(subcat._is_const_object(), False)
    self.assertEqual(subcat.database()._is_const_object(), False)
    self.assertEqual(subcat.name(), "subcat")
    self.assertEqual(subcat.parent().name(), "cat")

    self.assertEqual(subcat.description, "subcat_d")
    subcat.description = "changed"
    self.assertEqual(subcat.description, "changed")

    self.assertEqual(";".join([ "/".join([ str(v) for v in item.each_value() ]) for item in rdb.each_item_per_category(subcat.rdb_id())]), "float: 17.5/text: string;text: b")

    item1 = [ i for i in rdb.each_item_per_category(subcat.rdb_id())][0]
    self.assertEqual(item1._is_const_object(), False)
    item1.clear_values()
    self.assertEqual(";".join([ "/".join([ str(v) for v in item.each_value() ]) for item in rdb.each_item_per_category(subcat.rdb_id())]), ";text: b")
    item1.add_value("x")
    self.assertEqual(";".join([ "/".join([ str(v) for v in item.each_value() ]) for item in rdb.each_item_per_category(subcat.rdb_id())]), "text: x;text: b")
    item1.add_tag(17)
    self.assertEqual(item1.has_tag(17), True)
    self.assertEqual(item1.has_tag(16), False)

    item1 = [ i for i in rdb.each_item() ][0]
    self.assertEqual(item1._is_const_object(), False)
    self.assertEqual(item1.has_tag(17), True)

    item1 = [ i for i in rdb.each_item_per_cell(cell.rdb_id()) ][0]
    self.assertEqual(item1._is_const_object(), False)
    self.assertEqual(item1.has_tag(17), True)

    item1 = [ i for i in rdb.each_item_per_cell_and_category(cell.rdb_id(), subcat.rdb_id())][0]
    self.assertEqual(item1._is_const_object(), False)
    self.assertEqual(item1.has_tag(17), True)

    item1 = [ i for i in cell.each_item()][0]
    self.assertEqual(item1._is_const_object(), False)
    self.assertEqual(item1.has_tag(17), True)

  def test_14(self):

    # same names do not generate a new category
    rdb = pya.ReportDatabase("")

    _cell = rdb.create_cell("CELL")

    _cat = rdb.create_category("cat")
    _cat_same = rdb.create_category("cat")
    self.assertEqual(_cat.rdb_id(), _cat_same.rdb_id())

    _subcat = rdb.create_category(_cat, "subcat")
    _subcat_same = rdb.create_category(_cat_same, "subcat")
    self.assertEqual(_subcat.rdb_id(), _subcat_same.rdb_id())

    # testing whether decrementing the reference count would do harm
    _cat = None
    _cat_same = None
    self.assertEqual(_subcat.rdb_id(), _subcat_same.rdb_id())

  def test_15(self):

    p = pya.DPolygon(pya.DBox(0.5, 1, 2, 3))
    pwp = pya.DPolygonWithProperties(p, { 1: "value" })
    e = pya.DEdge(pya.DPoint(0, 0), pya.DPoint(1, 2))
    ewp = pya.DEdgeWithProperties(e, { 1: "value" })
    ep = pya.DEdgePair(e, e.moved(10, 10))
    epwp = pya.DEdgePairWithProperties(ep, { 1: "value" })
    t = pya.DText("text", pya.DTrans.R0)
    twp = pya.DTextWithProperties(t, { 1: "value" })
    b = pya.DBox(0, 0, 1, 2)
    bwp = pya.DBoxWithProperties(b, { 1: "value" })

    ip = pya.Polygon(pya.Box(0, 1, 2, 3))
    ipwp = pya.PolygonWithProperties(ip, { 1: "value" })
    ie = pya.Edge(pya.Point(0, 0), pya.Point(1, 2))
    iewp = pya.EdgeWithProperties(ie, { 1: "value" })
    iep = pya.EdgePair(ie, ie.moved(10, 10))
    iepwp = pya.EdgePairWithProperties(iep, { 1: "value" })
    it = pya.Text("text", pya.Trans.R0)
    itwp = pya.TextWithProperties(it, { 1: "value" })
    ib = pya.Box(0, 0, 1, 2)
    ibwp = pya.BoxWithProperties(ib, { 1: "value" })

    rdb = pya.ReportDatabase()

    cat = rdb.create_category("name")
    cell = rdb.create_cell("TOP")
    item = rdb.create_item(cell, cat)

    item.add_value(p)
    item.add_value(pwp)
    item.add_value(b)
    item.add_value(bwp)
    item.add_value(t)
    item.add_value(twp)
    item.add_value(e)
    item.add_value(ewp)
    item.add_value(ep)
    item.add_value(epwp)

    item.add_value(ip)
    item.add_value(ipwp)
    item.add_value(ib)
    item.add_value(ibwp)
    item.add_value(it)
    item.add_value(itwp)
    item.add_value(ie)
    item.add_value(iewp)
    item.add_value(iep)
    item.add_value(iepwp)

    item.add_value("string")
    item.add_value(17.5)

    values = [ str(v) for v in item.each_value() ]

    self.assertEqual(values, [
      'polygon: (0.5,1;0.5,3;2,3;2,1)', 'polygon: (0.5,1;0.5,3;2,3;2,1)', 
      'box: (0,0;1,2)', 'box: (0,0;1,2)', 
      "label: ('text',r0 0,0)", "label: ('text',r0 0,0)", 
      'edge: (0,0;1,2)', 'edge: (0,0;1,2)', 
      'edge-pair: (0,0;1,2)/(10,10;11,12)', 'edge-pair: (0,0;1,2)/(10,10;11,12)', 
      'polygon: (0,1;0,3;2,3;2,1)', 'polygon: (0,1;0,3;2,3;2,1)', 
      'box: (0,0;1,2)', 'box: (0,0;1,2)', 
      "label: ('text',r0 0,0)", "label: ('text',r0 0,0)", 
      'edge: (0,0;1,2)', 'edge: (0,0;1,2)', 
      'edge-pair: (0,0;1,2)/(10,10;11,12)', 'edge-pair: (0,0;1,2)/(10,10;11,12)', 
      'text: string', 
      'float: 17.5'
    ])

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(RDB_TestClass)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)


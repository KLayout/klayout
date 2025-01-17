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

def can_create_layoutview():
  if not "MainWindow" in pya.__dict__:
    return True  # Qt-less
  elif not "Application" in pya.__dict__:
    return False  # cannot instantiate Application
  elif pya.__dict__["Application"].instance() is None:
    return False  # Application is not present
  else:
    return True

def astr(a):
  astr = []
  for i in a:
    astr.append(str(i))
  return "[" + ", ".join(astr) + "]"

class LAYLayersTests(unittest.TestCase):

  def lnode_str(self, space, l):
    return space + l.current().source_(True) + "\n";

  def lnodes_str(self, space, l):
    res = ""
    while not l.at_end():
      res += self.lnode_str(space, l)
      if l.current().has_children():
        l.down_first_child()
        res += self.lnodes_str("  " + space, l)
        l.up()
      l.next_sibling(1)
    return res

  def lnodes_str2(self, v):
    res = []
    for c in v.each_layer():
      res.append(c.source_(True))
    return "\n".join(res)

  def lnodes_str3(self, v, index):
    res = []
    for c in v.each_layer(index):
      res.append(c.source_(True))
    return "\n".join(res)

  def test_1(self):

    if not can_create_layoutview():
      print("Skipped test as LayoutView cannot be created.")
      return

    cv = pya.LayoutView()
    cv.load_layout(os.getenv("TESTSRC") + "/testdata/gds/t11.gds", True) 
    cv.load_layout(os.getenv("TESTSRC") + "/testdata/gds/t10.gds", True) 

    cv.clear_layers()

    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "")

    pos = cv.end_layers()
    self.assertEqual(pos.parent().is_null(), True)
    p = pos.dup()
    p.up()
    self.assertEqual(p.is_null(), True)
    self.assertEqual(pos.is_null(), False)

    self.assertEqual(pos == cv.begin_layers(), True)
    self.assertEqual(pos != cv.begin_layers(), False)

    l1 = cv.insert_layer(pos, pya.LayerProperties())

    self.assertEqual(pos == cv.begin_layers(), True)
    self.assertEqual(pos != cv.begin_layers(), False)
    self.assertEqual(pos == cv.end_layers(), False)
    self.assertEqual(pos != cv.end_layers(), True)
    self.assertEqual(pos < cv.end_layers(), True)
    self.assertEqual(cv.end_layers() < pos, False)
    self.assertEqual(pos < cv.begin_layers(), False)
    self.assertEqual(cv.begin_layers() < pos, False)
    self.assertEqual(pos.at_top(), True)

    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n")

    new_p = pya.LayerProperties()
    new_p.source = "1/0@1"
    l11 = cv.insert_layer(pos.last_child(), new_p)

    p12 = pos.last_child()
    self.assertEqual(p12.parent().is_null(), False)
    self.assertEqual(p12.parent() == pos, True)

    pp = pos.dup()
    pp.down_last_child()
    self.assertEqual(pp == p12, True)
    self.assertEqual(pp == pos, False)
    self.assertEqual(pp.parent() == pos, True)
    pp.up()
    self.assertEqual(pp == pos, True)

    self.assertEqual(p12.at_top(), False)
    p12.to_sibling(0)
    self.assertEqual(p12 == pos.first_child(), True)
    self.assertEqual(p12.child_index(), 0)
    p12.to_sibling(1)
    self.assertEqual(p12.child_index(), 1)
    self.assertEqual(p12 == pos.last_child(), True)
    self.assertEqual(p12.num_siblings(), 1)

    l12 = cv.insert_layer(p12, pya.LayerProperties())
    l12_new = pya.LayerProperties()
    l12_new.source = "1/0@2"
    cv.set_layer_properties(p12, l12_new)

    self.assertEqual(p12.current().cellview(), 1)
    self.assertEqual(p12.current().has_upper_hier_level(True), False)
    self.assertEqual(p12.current().has_lower_hier_level(True), False)

    l12_new.source = "@* #1..2"
    cv.set_layer_properties(p12, l12_new)

    self.assertEqual(p12.current().cellview(), 0)
    self.assertEqual(p12.current().has_upper_hier_level(True), True)
    self.assertEqual(p12.current().has_upper_hier_level(), True)
    self.assertEqual(p12.current().upper_hier_level_(True), 2)
    self.assertEqual(p12.current().upper_hier_level, 2)
    self.assertEqual(p12.current().has_lower_hier_level(True), True)
    self.assertEqual(p12.current().has_lower_hier_level(), True)
    self.assertEqual(p12.current().lower_hier_level_(True), 1)
    self.assertEqual(p12.current().lower_hier_level, 1)

    l12_new.source = "@* (0,0 *0.5) (0,5 r45 *2.5)"
    cv.set_layer_properties(p12, l12_new)
    trans = p12.current().trans_(True)
    self.assertEqual(astr(trans), astr(p12.current().trans))
    self.assertEqual(len(trans), 2)
    self.assertEqual(str(trans [0]), "r0 *0.5 0,0")
    self.assertEqual(str(trans [1]), "r45 *2.5 0,5")

    l12_new.source = "1/0@2"
    cv.set_layer_properties(p12, l12_new)

    self.assertEqual(p12.num_siblings(), 2)

    pos = cv.end_layers()

    new_p = pya.LayerProperties()
    new_p.source = "@1"
    l2 = cv.insert_layer(pos, new_p)

    new_p = pya.LayerProperties()
    new_p.source = "7/0@*"
    l21 = cv.insert_layer(pos.first_child(), new_p)

    p22 = pos.last_child()
    new_p = pya.LayerProperties()
    l22 = cv.insert_layer(pos.last_child(), new_p)

    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n*/*@1\n  7/0@1\n  */*@1\n")

    new_p = l2.dup()
    new_p.source = "@2"
    cv.set_layer_properties(pos, new_p)

    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n  */*@2\n")

    pos.first_child().current().source = "7/0@1"
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@1\n  */*@2\n")
    pos.current().source = "@*"
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n*/*@*\n  7/0@1\n  */*@*\n")
    pos.current().source = "@2"
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@1\n  */*@2\n")
    pos.first_child().current().source = "7/1@*"
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/1@2\n  */*@2\n")
    pos.first_child().current().source = "7/0@*"
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n  */*@2\n")

    l22_new = pya.LayerProperties()
    l22_new.source = "7/1@*"
    cv.replace_layer_node(p22, l22_new)

    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n  7/1@2\n")

    cv.delete_layer(p22)

    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n")

    new_p = l2.dup()
    new_p.source = "%5@2"
    cv.set_layer_properties(pos, new_p)

    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n%5@2\n  %5@2\n")

    cv._destroy()

  def test_1a(self):

    if not can_create_layoutview():
      print("Skipped test as LayoutView cannot be created.")
      return

    mgr = pya.Manager()

    cv = pya.LayoutView(False, mgr)
    cv.load_layout(os.getenv("TESTSRC") + "/testdata/gds/t11.gds", True) 
    cv.load_layout(os.getenv("TESTSRC") + "/testdata/gds/t10.gds", True) 

    cv.clear_layers()

    cv.insert_layer_list(1)
    cv.rename_layer_list(1, "x")
    self.assertEqual(cv.current_layer_list, 1)
    cv.set_current_layer_list(0)
    self.assertEqual(cv.current_layer_list, 0)
    cv.set_current_layer_list(1)
    self.assertEqual(cv.current_layer_list, 1)

    self.assertEqual(self.lnodes_str("", cv.begin_layers(0)), "")
    self.assertEqual(self.lnodes_str("", cv.begin_layers(1)), "")

    pos = cv.end_layers(0)
    self.assertEqual(pos.parent().is_null(), True)
    p = pos.dup()
    p.up()
    self.assertEqual(p.is_null(), True)
    self.assertEqual(pos.is_null(), False)

    self.assertEqual(pos == cv.begin_layers(0), True)
    self.assertEqual(pos != cv.begin_layers(0), False)

    l1 = cv.insert_layer(0, pos, pya.LayerProperties())

    self.assertEqual(pos == cv.begin_layers(0), True)
    self.assertEqual(pos != cv.begin_layers(0), False)
    self.assertEqual(pos == cv.end_layers(0), False)
    self.assertEqual(pos != cv.end_layers(0), True)
    self.assertEqual(pos < cv.end_layers(0), True)
    self.assertEqual(cv.end_layers(0) < pos, False)
    self.assertEqual(pos < cv.begin_layers(0), False)
    self.assertEqual(cv.begin_layers(0) < pos, False)
    self.assertEqual(pos.at_top(), True)

    self.assertEqual(self.lnodes_str("", cv.begin_layers(0)), "*/*@*\n")
    self.assertEqual(self.lnodes_str("", cv.begin_layers(1)), "")
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "")

    new_p = pya.LayerProperties()
    new_p.source = "1/0@1"
    l11 = cv.insert_layer(0, pos.last_child(), new_p)

    p12 = pos.last_child()
    self.assertEqual(p12.parent().is_null(), False)
    self.assertEqual(p12.parent() == pos, True)

    pp = pos.dup()
    pp.down_last_child()
    self.assertEqual(pp == p12, True)
    self.assertEqual(pp == pos, False)
    self.assertEqual(pp.parent() == pos, True)
    pp.up()
    self.assertEqual(pp == pos, True)

    self.assertEqual(p12.at_top(), False)
    p12.to_sibling(0)
    self.assertEqual(p12 == pos.first_child(), True)
    self.assertEqual(p12.child_index(), 0)
    p12.to_sibling(1)
    self.assertEqual(p12.child_index(), 1)
    self.assertEqual(p12 == pos.last_child(), True)
    self.assertEqual(p12.num_siblings(), 1)

    l12 = cv.insert_layer(0, p12, pya.LayerProperties())
    l12_new = pya.LayerProperties()
    l12_new.source = "1/0@2"
    cv.set_layer_properties(0, p12, l12_new)

    self.assertEqual(p12.current().cellview(), 1)
    self.assertEqual(p12.current().has_upper_hier_level(True), False)
    self.assertEqual(p12.current().has_upper_hier_level(), False)
    self.assertEqual(p12.current().has_lower_hier_level(True), False)
    self.assertEqual(p12.current().has_lower_hier_level(), False)

    l12_new.source = "@* #1..2"
    cv.set_layer_properties(0, p12, l12_new)

    self.assertEqual(p12.current().cellview(), 0)
    self.assertEqual(p12.current().has_upper_hier_level(True), True)
    self.assertEqual(p12.current().has_upper_hier_level(), True)
    self.assertEqual(p12.current().upper_hier_level_(True), 2)
    self.assertEqual(p12.current().upper_hier_level, 2)
    self.assertEqual(p12.current().has_lower_hier_level(True), True)
    self.assertEqual(p12.current().has_lower_hier_level(), True)
    self.assertEqual(p12.current().lower_hier_level_(True), 1)
    self.assertEqual(p12.current().lower_hier_level, 1)

    l12_new.source = "@* (0,0 *0.5) (0,5 r45 *2.5)"
    cv.set_layer_properties(0, p12, l12_new)
    trans = p12.current().trans_(True)
    self.assertEqual(len(trans), 2)
    self.assertEqual(str(trans [0]), "r0 *0.5 0,0")
    self.assertEqual(str(trans [1]), "r45 *2.5 0,5")

    l12_new.source = "1/0@2"
    cv.set_layer_properties(0, p12, l12_new)

    self.assertEqual(p12.num_siblings(), 2)

    pos = cv.end_layers(0)

    new_p = pya.LayerProperties()
    new_p.source = "@1"
    l2 = cv.insert_layer(0, pos, new_p)

    new_p = pya.LayerProperties()
    new_p.source = "7/0@*"
    l21 = cv.insert_layer(0, pos.first_child(), new_p)

    p22 = pos.last_child()
    new_p = pya.LayerProperties()
    l22 = cv.insert_layer(0, pos.last_child(), new_p)

    self.assertEqual(self.lnodes_str("", cv.begin_layers(0)), "*/*@*\n  1/0@1\n  1/0@2\n*/*@1\n  7/0@1\n  */*@1\n")
    self.assertEqual(self.lnodes_str("", cv.begin_layers(1)), "")
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "")

    new_p = l2.dup()
    new_p.source = "@2"
    cv.set_layer_properties(0, pos, new_p)

    self.assertEqual(self.lnodes_str("", cv.begin_layers(0)), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n  */*@2\n")

    l22_new = pya.LayerProperties()
    l22_new.source = "7/1@*"
    cv.replace_layer_node(0, p22, l22_new)

    self.assertEqual(self.lnodes_str("", cv.begin_layers(0)), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n  7/1@2\n")

    cv.delete_layer(0, p22)

    self.assertEqual(self.lnodes_str("", cv.begin_layers(0)), "*/*@*\n  1/0@1\n  1/0@2\n*/*@2\n  7/0@2\n")

    new_p = l2.dup()
    new_p.source = "%5@2"
    cv.set_layer_properties(0, pos, new_p)

    self.assertEqual(self.lnodes_str("", cv.begin_layers(0)), "*/*@*\n  1/0@1\n  1/0@2\n%5@2\n  %5@2\n")

    # build a tree by building a separate tree
    new_p = pya.LayerPropertiesNode()
    self.assertEqual(new_p.has_children(), False)
    n1 = new_p.add_child(pya.LayerProperties())
    self.assertEqual(n1.has_children(), False)
    n1.source = "101/0"
    n2 = pya.LayerPropertiesNode()
    self.assertEqual(n2.has_children(), False)
    n21 = n2.add_child(pya.LayerProperties())
    n21.source = "102/0"
    self.assertEqual(n2.has_children(), True)
    n22 = n2.add_child(pya.LayerProperties())
    self.assertEqual(n2.has_children(), True)
    n22.source = "103/0"
    new_p.add_child(n2)
    self.assertEqual(new_p.has_children(), True)

    p = pos.last_child()
    ll = cv.insert_layer(0, p, new_p)
    self.assertEqual(p.current().has_children(), True)
    self.assertEqual(p.first_child().current().has_children(), False)
    self.assertEqual(p.first_child().current().source_(False), "101/0@1")
    self.assertEqual(p.first_child().current().source, "%5@1")

    # (test clear_children):
    new_p.clear_children()
    self.assertEqual(new_p.has_children(), False)

    self.assertEqual(ll.has_children(), False)

    cv.transaction("Delete")
    li = cv.begin_layers(0)
    a = []
    while not li.at_end():
      a.append(li.dup())
      li.next()
    self.assertEqual(len(a), 10)
    cv.delete_layers(0, a)
    self.assertEqual(cv.begin_layers(0).at_end(), True)
    cv.commit()
    mgr.undo()
    self.assertEqual(cv.begin_layers(0).at_end(), False)

    cv.transaction("Delete")
    i = 0
    while not cv.begin_layers(0).at_end():
      cv.delete_layer(0, cv.begin_layers(0))
      i += 1
    self.assertEqual(i, 2)
    self.assertEqual(cv.begin_layers(0).at_end(), True)
    cv.commit()
    mgr.undo()
    self.assertEqual(cv.begin_layers(0).at_end(), False)

    cv._destroy()

  def test_2(self):

    p = pya.LayerPropertiesNode()

    self.assertEqual(p.source_(False), "*/*@*")
    self.assertEqual(p.source, "*/*@*")
    self.assertEqual(p.has_source_name(False), False)
    self.assertEqual(p.has_source_name(), False)
    self.assertEqual(p.has_frame_color(), False)
    self.assertEqual(p.has_frame_color(True), False)
    self.assertEqual(p.has_fill_color(), False)
    self.assertEqual(p.has_fill_color(True), False)
    self.assertEqual(p.has_dither_pattern(), False)
    self.assertEqual(p.has_dither_pattern(True), False)
    self.assertEqual(p.has_line_style(), False)
    self.assertEqual(p.has_line_style(True), False)

    p.name = "u"
    self.assertEqual(p.name, "u")

    p.source_name = "x"
    self.assertEqual(p.source_name_(False), "x")
    self.assertEqual(p.source_name, "x")
    self.assertEqual(p.source_(False), "x@*")
    self.assertEqual(p.source, "x@*")
    self.assertEqual(p.flat().source, "x@*")
    self.assertEqual(p.dup().source, "x@*")
    self.assertEqual(p.has_source_name(False), True)
    self.assertEqual(p.has_source_name(), True)

    p.clear_source_name()
    self.assertEqual(p.source_(False), "*/*@*")
    self.assertEqual(p.has_source_name(False), False)

    p.source_layer_index = 6
    self.assertEqual(p.source_(False), "%6@*")
    self.assertEqual(p.source_layer_index_(False), 6)
    self.assertEqual(p.source_layer_index, 6)

    p.source_layer = 6
    p.source_datatype = 5
    self.assertEqual(p.source_(False), "%6@*")

    p.source_layer_index = -1
    self.assertEqual(p.source_(False), "6/5@*")
    self.assertEqual(p.source_layer_index_(False), -1)
    self.assertEqual(p.source_layer_index, -1)
    self.assertEqual(p.source_layer_(False), 6)
    self.assertEqual(p.source_layer, 6)
    self.assertEqual(p.source_datatype_(False), 5)
    self.assertEqual(p.source_datatype, 5)

    arr = [ pya.CplxTrans.new(pya.CplxTrans.M45), pya.CplxTrans.new(pya.CplxTrans.R270) ]
    p.trans = arr
    self.assertEqual(p.source_(False), "6/5@* (m45 *1 0,0) (r270 *1 0,0)")
    self.assertEqual(arr == p.trans_(False), True)

    p.source_cellview = 1 
    self.assertEqual(p.source_(False), "6/5@2 (m45 *1 0,0) (r270 *1 0,0)")
    self.assertEqual(p.flat().source, "6/5@2 (m45 *1 0,0) (r270 *1 0,0)")
    self.assertEqual(p.source_cellview_(False), 1)
    self.assertEqual(p.source_cellview, 1)
    p.source_cellview = -1 
    self.assertEqual(p.source_(False), "6/5@* (m45 *1 0,0) (r270 *1 0,0)")

    p.upper_hier_level = 17
    self.assertEqual(p.source_(False), "6/5@* (m45 *1 0,0) (r270 *1 0,0) #..17")
    self.assertEqual(p.upper_hier_level_(False), 17)
    self.assertEqual(p.upper_hier_level, 17)
    self.assertEqual(p.has_upper_hier_level(False), True)
    self.assertEqual(p.upper_hier_level_relative(), False)
    self.assertEqual(p.upper_hier_level_relative(True), False)
    p.set_upper_hier_level(11, True)
    self.assertEqual(p.upper_hier_level_mode(False), 0)
    self.assertEqual(p.upper_hier_level_mode(), 0)
    self.assertEqual(p.upper_hier_level, 11)
    self.assertEqual(p.upper_hier_level_relative(), True)
    self.assertEqual(p.upper_hier_level_relative(True), True)
    p.set_upper_hier_level(11, True, 1)
    self.assertEqual(p.upper_hier_level, 11)
    self.assertEqual(p.upper_hier_level_mode(False), 1)
    self.assertEqual(p.upper_hier_level_mode(), 1)
    p.set_upper_hier_level(11, True, 2)
    self.assertEqual(p.upper_hier_level_mode(False), 2)
    self.assertEqual(p.upper_hier_level_mode(), 2)
    p.clear_upper_hier_level() 
    self.assertEqual(p.source_(False), "6/5@* (m45 *1 0,0) (r270 *1 0,0)")
    self.assertEqual(p.has_upper_hier_level(False), False)
    self.assertEqual(p.has_upper_hier_level(), False)

    p.lower_hier_level = 17
    self.assertEqual(p.source_(False), "6/5@* (m45 *1 0,0) (r270 *1 0,0) #17..")
    self.assertEqual(p.source, "6/5@* (m45 *1 0,0) (r270 *1 0,0) #17..")
    self.assertEqual(p.lower_hier_level_(False), 17)
    self.assertEqual(p.lower_hier_level, 17)
    self.assertEqual(p.has_lower_hier_level(False), True)
    self.assertEqual(p.has_lower_hier_level(), True)
    self.assertEqual(p.lower_hier_level_relative(), False)
    self.assertEqual(p.lower_hier_level_relative(True), False)
    p.set_lower_hier_level(10, True)
    self.assertEqual(p.lower_hier_level, 10)
    self.assertEqual(p.lower_hier_level_relative(), True)
    self.assertEqual(p.lower_hier_level_relative(True), True)
    p.set_lower_hier_level(11, True, 1)
    self.assertEqual(p.lower_hier_level, 11)
    self.assertEqual(p.lower_hier_level_mode(False), 1)
    self.assertEqual(p.lower_hier_level_mode(), 1)
    p.set_lower_hier_level(11, True, 2)
    self.assertEqual(p.lower_hier_level_mode(False), 2)
    self.assertEqual(p.lower_hier_level_mode(), 2)
    p.clear_lower_hier_level() 
    self.assertEqual(p.source_(False), "6/5@* (m45 *1 0,0) (r270 *1 0,0)")
    self.assertEqual(p.source, "6/5@* (m45 *1 0,0) (r270 *1 0,0)")
    self.assertEqual(p.has_lower_hier_level(False), False)
    self.assertEqual(p.has_lower_hier_level(), False)

    p.dither_pattern = 18
    self.assertEqual(p.dither_pattern_(True), 18)
    self.assertEqual(p.flat().dither_pattern_(True), 18)
    self.assertEqual(p.dither_pattern, 18)
    self.assertEqual(p.eff_dither_pattern(), 18)
    self.assertEqual(p.eff_dither_pattern(True), 18)
    self.assertEqual(p.has_dither_pattern(), True)
    self.assertEqual(p.has_dither_pattern(True), True)

    p.line_style = 12
    self.assertEqual(p.line_style_(True), 12)
    self.assertEqual(p.flat().line_style_(True), 12)
    self.assertEqual(p.line_style, 12)
    self.assertEqual(p.eff_line_style(), 12)
    self.assertEqual(p.eff_line_style(True), 12)
    self.assertEqual(p.has_line_style(), True)
    self.assertEqual(p.has_line_style(True), True)

    p.animation = 2
    self.assertEqual(p.animation_(True), 2)
    self.assertEqual(p.flat().animation_(True), 2)
    self.assertEqual(p.animation, 2)

    p.marked = True
    self.assertEqual(p.marked_(True), True)
    self.assertEqual(p.flat().marked_(True), True)
    self.assertEqual(p.marked, True)

    p.marked = False
    self.assertEqual(p.marked_(False), False)
    self.assertEqual(p.flat().marked_(False), False)
    self.assertEqual(p.marked, False)

    p.transparent = True
    self.assertEqual(p.transparent_(True), True)
    self.assertEqual(p.flat().transparent_(True), True)
    self.assertEqual(p.transparent, True)

    p.transparent = False
    self.assertEqual(p.transparent_(False), False)
    self.assertEqual(p.flat().transparent_(False), False)
    self.assertEqual(p.transparent, False)

    p.visible = True
    self.assertEqual(p.visible_(True), True)
    self.assertEqual(p.flat().visible_(True), True)
    self.assertEqual(p.visible, True)

    p.visible = False
    self.assertEqual(p.visible_(False), False)
    self.assertEqual(p.flat().visible_(False), False)
    self.assertEqual(p.visible, False)

    p.valid = True
    self.assertEqual(p.valid_(True), True)
    self.assertEqual(p.flat().valid_(True), True)
    self.assertEqual(p.valid, True)

    p.valid = False
    self.assertEqual(p.valid_(False), False)
    self.assertEqual(p.flat().valid_(False), False)
    self.assertEqual(p.valid, False)

    p.xfill = True
    self.assertEqual(p.xfill_(True), True)
    self.assertEqual(p.flat().xfill_(True), True)
    self.assertEqual(p.xfill, True)

    p.xfill = False
    self.assertEqual(p.xfill_(False), False)
    self.assertEqual(p.flat().xfill_(False), False)
    self.assertEqual(p.xfill, False)

    p.width = 3
    self.assertEqual(p.width_(True), 3)
    self.assertEqual(p.flat().width_(True), 3)
    self.assertEqual(p.width, 3)

    p.frame_color = 0xff000031
    self.assertEqual(p.frame_color_(True), 0xff000031)
    self.assertEqual(p.flat().frame_color_(True), 0xff000031)
    self.assertEqual(p.frame_color, 0xff000031)
    self.assertEqual(p.has_frame_color(), True)
    self.assertEqual(p.has_frame_color(True), True)
    self.assertEqual(p.has_fill_color(), False)
    self.assertEqual(p.has_fill_color(True), False)

    p.fill_color = 0xff000032
    self.assertEqual(p.fill_color_(True), 0xff000032)
    self.assertEqual(p.flat().fill_color_(True), 0xff000032)
    self.assertEqual(p.fill_color, 0xff000032)
    self.assertEqual(p.has_frame_color(), True)
    self.assertEqual(p.has_fill_color(), True)

    p.frame_brightness = 41
    self.assertEqual(p.frame_brightness_(True), 41)
    self.assertEqual(p.flat().frame_brightness_(True), 41)
    self.assertEqual(p.frame_brightness, 41)

    p.fill_brightness = 42
    self.assertEqual(p.fill_brightness_(True), 42)
    self.assertEqual(p.flat().fill_brightness_(True), 42)
    self.assertEqual(p.fill_brightness, 42)
    self.assertEqual("#%06x" % p.eff_frame_color(), "#33335b")
    self.assertEqual("#%06x" % p.eff_fill_color(), "#34345c")
    self.assertEqual("#%06x" % p.eff_frame_color(True), "#33335b")
    self.assertEqual("#%06x" % p.eff_fill_color(True), "#34345c")

    p.clear_fill_color()
    self.assertEqual(p.has_fill_color(), False)
    p.clear_frame_color()
    self.assertEqual(p.has_frame_color(), False)
    p.clear_dither_pattern()
    self.assertEqual(p.has_dither_pattern(), False)
    p.clear_line_style()
    self.assertEqual(p.has_line_style(), False)

    pp = pya.LayerPropertiesNode()
    self.assertEqual(pp == p, False)
    self.assertEqual(pp != p, True)
    pp = p.dup()
    self.assertEqual(pp == p, True)
    self.assertEqual(pp != p, False)

  # direct replacement of objects and attributes
  def test_3(self):

    if not can_create_layoutview():
      print("Skipped test as LayoutView cannot be created.")
      return

    cv = pya.LayoutView()
    cv.load_layout(os.getenv("TESTSRC") + "/testdata/gds/t11.gds", pya.LoadLayoutOptions(), "", True) 
    cv.load_layout(os.getenv("TESTSRC") + "/testdata/gds/t10.gds", pya.LoadLayoutOptions(), "", True) 

    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "1/0@1\n2/0@1\n1/0@2\n2/0@2\n3/0@2\n3/1@2\n4/0@2\n5/0@2\n6/0@2\n6/1@2\n7/0@2\n8/0@2\n8/1@2\n")

    cv.clear_layers()

    pos = cv.end_layers()
    self.assertEqual(pos.current().is_valid(), False)

    cv.insert_layer(pos, pya.LayerProperties())
    self.assertEqual(pos.current().is_valid(), True)
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n")
    self.assertEqual(self.lnodes_str2(cv), "*/*@*")

    self.assertEqual(cv.begin_layers().current().name, "")
    self.assertEqual(cv.begin_layers().current().visible, True)
    self.assertEqual(cv.begin_layers().current().dither_pattern, -1)
    self.assertEqual(cv.begin_layers().current().line_style, -1)
    self.assertEqual(cv.begin_layers().current().valid, True)
    self.assertEqual(cv.begin_layers().current().transparent, False)

    # test LayerPropertiesNodeRef
    pos.current().name = "NAME"
    pos.current().visible = False
    pos.current().fill_color = 0xff012345
    pos.current().frame_color = 0xff123456
    pos.current().fill_brightness = 42
    pos.current().frame_brightness = 17
    pos.current().dither_pattern = 4
    pos.current().line_style = 3
    pos.current().valid = False
    pos.current().transparent = True
    pos.current().marked = False
    pos.current().xfill = False
    pos.current().width = 2
    pos.current().animation = 2

    self.assertEqual(cv.begin_layers().current().name, "NAME")
    self.assertEqual(cv.begin_layers().current().visible, False)
    self.assertEqual(cv.begin_layers().current().fill_color, 0xff012345)
    self.assertEqual(cv.begin_layers().current().frame_color, 0xff123456)
    self.assertEqual(cv.begin_layers().current().fill_brightness, 42)
    self.assertEqual(cv.begin_layers().current().frame_brightness, 17)
    self.assertEqual(cv.begin_layers().current().dither_pattern, 4)
    self.assertEqual(cv.begin_layers().current().line_style, 3)
    self.assertEqual(cv.begin_layers().current().valid, False)
    self.assertEqual(cv.begin_layers().current().transparent, True)
    self.assertEqual(cv.begin_layers().current().marked, False)
    self.assertEqual(cv.begin_layers().current().xfill, False)
    self.assertEqual(cv.begin_layers().current().width, 2)
    self.assertEqual(cv.begin_layers().current().animation, 2)

    pos.current().valid = True

    new_p = pya.LayerProperties()
    new_p.source = "1/0@1"
    self.assertEqual(new_p.flat().source, "1/0@1")
    self.assertEqual(new_p == new_p.flat(), True)
    self.assertEqual(new_p != new_p.flat(), False)
    new_p_ref = pos.current().add_child(new_p)
    self.assertEqual(new_p_ref.layer_index(), cv.cellview(0).layout().layer(1, 0))
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n")
    self.assertEqual(self.lnodes_str2(cv), "*/*@*\n1/0@1")

    p = pos.current().add_child()
    p.source = "1/0@2"
    self.assertEqual(p.is_valid(), True)
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  1/0@2\n")
    self.assertEqual(self.lnodes_str2(cv), "*/*@*\n1/0@1\n1/0@2")
    self.assertEqual(p.layer_index(), cv.cellview(1).layout().layer(1, 0))
    self.assertEqual(str(p.bbox()), "(-1.4,1.8;25.16,3.8)")
    self.assertEqual(p.view() == cv, True)
    self.assertEqual(p.list_index(), 0)

    l12_new = pya.LayerProperties()
    l12_new.source = "@* #1..2"
    self.assertEqual(l12_new.flat().source, "*/*@* #1..2")
    self.assertEqual(pos.first_child().current().source, "1/0@1")
    self.assertEqual(pos.first_child().current().is_valid(), True)
    self.assertEqual(pos.last_child().current().is_valid(), False)
    pos.first_child().next().current().assign(l12_new)
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  */*@* #1..2\n")
    self.assertEqual(self.lnodes_str2(cv), "*/*@*\n1/0@1\n*/*@* #1..2")

    pos.first_child().next_sibling(1).current().source = "@* #3..4"
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  1/0@1\n  */*@* #3..4\n")
    self.assertEqual(self.lnodes_str2(cv), "*/*@*\n1/0@1\n*/*@* #3..4")

    pos.first_child().to_sibling(1).next_sibling(-1).current().source = "7/0"
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "*/*@*\n  7/0@1\n  */*@* #3..4\n")
    self.assertEqual(self.lnodes_str2(cv), "*/*@*\n7/0@1\n*/*@* #3..4")
    self.assertEqual(self.lnodes_str3(cv, 0), "*/*@*\n7/0@1\n*/*@* #3..4")
    self.assertEqual(self.lnodes_str3(cv, 1), "")

    nn = pya.LayerPropertiesNode()
    nn.source = "TOP"

    nn1 = pya.LayerPropertiesNode()
    nn1.source = "nn1"

    nn2 = pya.LayerProperties()
    nn2.source = "nn1"
    nn1.add_child(nn2)

    nn.add_child(nn1)

    pos.current().assign(nn)
    self.assertEqual(pos.current().id(), nn.id())
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "TOP@1\n  nn1@1\n    nn1@1\n")
    self.assertEqual(self.lnodes_str2(cv), "TOP@1\nnn1@1\nnn1@1")

    cv._destroy()

  # propagation of "real" attributes through the hierarchy
  def test_4(self):

    if not can_create_layoutview():
      print("Skipped test as LayoutView cannot be created.")
      return

    cv = pya.LayoutView()
    cv.load_layout(os.getenv("TESTSRC") + "/testdata/gds/t11.gds", True) 
    cv.load_layout(os.getenv("TESTSRC") + "/testdata/gds/t10.gds", True) 

    cv.clear_layers()

    pos = cv.end_layers()
    self.assertEqual(pos.current().is_valid(), False)

    cv.insert_layer(pos, pya.LayerProperties())

    new_p = pya.LayerProperties()
    new_p.source = "1/0@1"
    pos.current().add_child(new_p)

    self.assertEqual(pos.current().visible_(True), True)
    self.assertEqual(pos.current().visible_(False), True)
    self.assertEqual(pos.first_child().current().visible_(True), True)
    self.assertEqual(pos.first_child().current().visible_(False), True)
    pos.current().visible = False
    self.assertEqual(pos.current().visible_(True), False)
    self.assertEqual(pos.current().visible_(False), False)
    self.assertEqual(pos.first_child().current().visible_(True), False)
    self.assertEqual(pos.first_child().current().visible_(False), True)

    cv._destroy()

  # delete method of iterator
  def test_5(self):

    if not can_create_layoutview():
      print("Skipped test as LayoutView cannot be created.")
      return

    cv = pya.LayoutView()
    cv.load_layout(os.getenv("TESTSRC") + "/testdata/gds/t11.gds", True) 

    cv.clear_layers()

    new_p = pya.LayerProperties()
    new_p.source = "1/0@1"
    cv.insert_layer(0, cv.end_layers(), new_p)

    new_p = pya.LayerProperties()
    new_p.source = "2/0@1"
    cv.insert_layer(0, cv.end_layers(), new_p)

    pos = cv.begin_layers()

    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "1/0@1\n2/0@1\n")
    self.assertEqual(pos.at_end(), False)
    self.assertEqual(pos.current().source, "1/0@1")
    self.assertEqual(pos.current().is_valid(), True)

    pos.current().delete()

    self.assertEqual(pos.current().source, "2/0@1")
    self.assertEqual(pos.current().is_valid(), True)
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "2/0@1\n")
    self.assertEqual(pos.at_end(), False)

    pos.current().delete()

    self.assertEqual(pos.current().is_valid(), False)
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "")
    self.assertEqual(pos.at_end(), True)

    pos.current().delete()

    self.assertEqual(pos.current().is_valid(), False)
    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "")
    self.assertEqual(pos.at_end(), True)

    # With hierarchy
    cv.clear_layers()

    new_p = pya.LayerProperties()
    new_p.source = "1/0@1"
    cv.insert_layer(0, cv.end_layers(), new_p)

    new_p = pya.LayerProperties()
    new_p.source = "2/0@1"
    cv.insert_layer(0, cv.end_layers(), new_p)

    new_p = pya.LayerProperties()
    new_p.source = "3/0@1"
    cv.insert_layer(0, cv.end_layers(), new_p)

    pos = cv.begin_layers()
    pos.next_sibling(1)
    c1 = pos.current().add_child()
    c1.source = "21/0@1"
    c2 = pos.current().add_child()
    c2.source = "22/0@1"

    posn = cv.begin_layers()
    posn.next_sibling(2)
    c3 = posn.current().add_child()
    c3.source = "31/0@1"

    self.assertEqual(self.lnodes_str("", cv.begin_layers()), "1/0@1\n2/0@1\n  21/0@1\n  22/0@1\n3/0@1\n  31/0@1\n")

    pc = pos.first_child()

    self.assertEqual(pc.current().source, "21/0@1")
    self.assertEqual(pc.current().is_valid(), True)
    self.assertEqual(pc.at_end(), False)
    pc.current().delete()

    self.assertEqual(pc.current().source, "22/0@1")
    self.assertEqual(pc.current().is_valid(), True)
    self.assertEqual(pc.at_end(), False)
    pc.current().delete()

    self.assertEqual(pc.at_end(), True)
    self.assertEqual(pc.current().is_valid(), False)

    cv._destroy()

  # custom stipples and line styles
  def test_6(self):

    if not can_create_layoutview():
      print("Skipped test as LayoutView cannot be created.")
      return

    cv = pya.LayoutView()
    cv.load_layout(os.getenv("TESTSRC") + "/testdata/gds/t11.gds", True) 

    cv.clear_stipples()

    self.assertEqual(cv.get_stipple(0), "*\n")

    index = cv.add_stipple("something", [ 0x1, 0x2, 0x4, 0x8 ], 4)
    self.assertEqual(cv.get_stipple(index), "...*\n..*.\n.*..\n*...\n")

    cv.remove_stipple(index)
    self.assertEqual(cv.get_stipple(index), "*\n")

    index = cv.add_stipple("something", ".**.\n*..*\n.*.*\n*.*.")
    self.assertEqual(cv.get_stipple(index), ".**.\n*..*\n.*.*\n*.*.\n")

    cv.clear_stipples()
    self.assertEqual(cv.get_stipple(index), "*\n")

    cv.clear_line_styles()

    self.assertEqual(cv.get_line_style(0), "")

    index = cv.add_line_style("something", 0x5, 4)
    self.assertEqual(cv.get_line_style(index), "*.*.")

    cv.remove_line_style(index)
    self.assertEqual(cv.get_line_style(index), "")

    index = cv.add_line_style("something", ".**.*..*")
    self.assertEqual(cv.get_line_style(index), ".**.*..*")

    cv.clear_line_styles()
    self.assertEqual(cv.get_line_style(index), "")

    cv._destroy()


# run unit tests
if __name__ == '__main__':
  suite = unittest.TestSuite()
  # NOTE: Use this instead of loadTestsfromTestCase to select a specific test:
  #   suite.addTest(BasicTest("test_26"))
  suite = unittest.TestLoader().loadTestsFromTestCase(LAYLayersTests)

  # Only runs with Application available
  if "Application" in pya.__all__ and not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)



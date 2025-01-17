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

class EDT_TestClass < TestBase

  # ObjectInstPath
  def test_1

    if !RBA.constants.member?(:ObjectInstPath)
      return
    end

    ly = RBA::Layout::new
    li = ly.insert_layer(RBA::LayerInfo::new(1, 0))
    tc = ly.create_cell("TOP")
    c2 = ly.create_cell("C2")
    shape = tc.shapes(li).insert(RBA::Box::new(-100, -200, 300, 400))
    inst = tc.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(10, 20)))

    p = RBA::ObjectInstPath::new
    p.cv_index = 17
    assert_equal(p.cv_index, 17)

    p.seq = 2
    assert_equal(p.seq, 2)

    assert_equal(p.is_cell_inst?, true)
    assert_equal(p.layer, nil)
    assert_equal(p.shape, nil)
    p.layer = -1
    assert_equal(p.is_cell_inst?, true)
    assert_equal(p.layer, nil)
    assert_equal(p.shape, nil)

    p.layer = 42
    assert_equal(p.layer, 42)
    assert_equal(p.is_cell_inst?, false)

    pp = p.dup
    assert_equal(pp == p, true)
    assert_equal(pp != p, false)
    assert_equal(pp < p, false)
    assert_equal(p < pp, false)

    p.shape = shape
    assert_equal(p.shape.to_s, "box (-100,-200;300,400)")
    assert_equal(pp == p, false)
    assert_equal(pp != p, true)
    assert_equal(pp < p, true)
    assert_equal(p < pp, false)

    p.top = tc.cell_index
    assert_equal(p.top, tc.cell_index)
    assert_equal(p.cell_index, tc.cell_index)
    assert_equal(p.source, tc.cell_index)
    assert_equal(p.trans.to_s, "r0 *1 0,0")
    assert_equal(p.source_trans.to_s, "r0 *1 0,0")
    
    assert_equal(p.path_length, 0)

    p.append_path(RBA::InstElement::new(inst))

    assert_equal(p.path_length, 1)
    assert_equal(p.path_nth(0).inst.to_s, "cell_index=1 r0 10,20")
    assert_equal(p.path_nth(0).cell_inst.to_s, "#1 r0 10,20")
    assert_equal(p.path_nth(0).specific_trans.to_s, "r0 10,20")

    assert_equal(p.trans.to_s, "r0 *1 10,20")
    assert_equal(p.source_trans.to_s, "r0 *1 10,20")
    assert_equal(p.top, tc.cell_index)
    assert_equal(p.cell_index, c2.cell_index)
    assert_equal(p.source, c2.cell_index)

    # turn it into a instance pointer
    p.layer = -1
    assert_equal(p.is_cell_inst?, true)
    assert_equal(p.trans.to_s, "r0 *1 10,20")   # deprecated -> includes instance too
    assert_equal(p.source_trans.to_s, "r0 *1 0,0")
    assert_equal(p.top, tc.cell_index)
    assert_equal(p.cell_index, c2.cell_index)   # deprecated -> includes instance too
    assert_equal(p.source, tc.cell_index)
    assert_equal(p.inst.to_s, "cell_index=1 r0 10,20")

    p.layer = 42
    assert_equal(p.path_length, 1)
    p.clear_path
    assert_equal(p.path_length, 0)
    assert_equal(p.top, tc.cell_index)
    assert_equal(p.cell_index, tc.cell_index)
    assert_equal(p.source, tc.cell_index)
    assert_equal(p.trans.to_s, "r0 *1 0,0")
    assert_equal(p.source_trans.to_s, "r0 *1 0,0")

    # ObjectInstPath from RecursiveShapeIterator

    si = tc.begin_shapes_rec(li)
    oi = RBA::ObjectInstPath::new(si, 2)

    assert_equal(oi.cv_index, 2)
    assert_equal(oi.shape.to_s, si.shape.to_s)
    assert_equal(oi.cell_index, si.cell.cell_index)
    assert_equal(oi.top, tc.cell_index)

  end

  # Selection
  def test_2

    if !RBA.constants.member?(:Application)
      return
    end

    mw = RBA::Application::instance.main_window
    mw.close_all
    mw.create_layout( 0 )
    lv = mw.current_view

    cv = lv.active_cellview 
    ly = cv.layout
    li = ly.insert_layer(RBA::LayerInfo::new(1, 0))
    tc = ly.create_cell("TOP")
    c2 = ly.create_cell("C2")
    tc_shape = tc.shapes(li).insert(RBA::Box::new(-100, -200, 300, 400))
    c2_shape = c2.shapes(li).insert(RBA::Box::new(-1, -2, 3, 4))
    inst = tc.insert(RBA::CellInstArray::new(c2.cell_index, RBA::Trans::new(10, 20)))

    p = RBA::ObjectInstPath::new
    p.cv_index = 0
    p.layer = li
    p.shape = tc_shape

    p2 = RBA::ObjectInstPath::new
    p2.cv_index = 0
    p2.layer = li
    p2.append_path(RBA::InstElement::new(inst))
    p2.shape = c2_shape

    assert_equal(lv.has_object_selection?, false)
    assert_equal(lv.object_selection.size, 0)
    lv.object_selection = [ p ]
    assert_equal(lv.object_selection.size, 1)
    assert_equal(lv.has_object_selection?, true)
    assert_equal(lv.object_selection[0] == p, true)
    lv.clear_object_selection
    assert_equal(lv.object_selection.size, 0)

    lv.select_object(p)
    assert_equal(lv.object_selection.size, 1)
    assert_equal(lv.object_selection[0] == p, true)

    lv.select_object(p2)
    assert_equal(lv.object_selection.size, 2)
    lv.unselect_object(p)
    assert_equal(lv.object_selection.size, 1)
    assert_equal(lv.object_selection[0] == p2, true)

    sel = []
    lv.each_object_selected { |o| sel << o }
    assert_equal(sel.size, 1)
    assert_equal(sel[0] == p2, true)
    assert_equal(sel[0].layout.object_id, ly.object_id)
    assert_equal(sel[0].trans.to_s, "r0 *1 10,20")
    assert_equal(sel[0].dtrans.to_s, "r0 *1 0.01,0.02")
    assert_equal(sel[0].source_trans.to_s, "r0 *1 10,20")
    assert_equal(sel[0].source_dtrans.to_s, "r0 *1 0.01,0.02")

    # without a mouse pointer we can't test more than this:
    assert_equal(lv.has_transient_object_selection?, false)
    sel = []
    lv.each_object_selected_transient { |o| sel << o }
    assert_equal(sel.size, 0)

  end

end

load("test_epilogue.rb")

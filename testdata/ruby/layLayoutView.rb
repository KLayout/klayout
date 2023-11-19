# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2023 Matthias Koefferlein
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

module RBA
  class LayoutView
    def main_window
      RBA::Application.instance.main_window
    end
    def index
      self.main_window.index_of(self)
    end
  end
end

class LAYLayoutView_TestClass < TestBase

  # Basic view creation and MainWindow events
  def test_1

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all

    created = 0
    created_proc = proc { created += 1 }
    current_view_changed = 0
    current_view_changed_proc = proc { current_view_changed += 1 }
    closed = 0
    closed_proc = proc { closed += 1 }
    mw.on_view_created += created_proc
    mw.on_current_view_changed += current_view_changed_proc
    mw.on_view_closed += closed_proc

    cv1 = mw.create_layout(1)
    assert_equal(RBA::LayoutView::current.index, 0)
    assert_equal(RBA::LayoutView::current.active_cellview_index, 0)
    assert_equal(RBA::LayoutView::current.active_cellview.index, 0)
    assert_equal(RBA::CellView::active.index, 0)
    assert_equal(RBA::CellView::active.view.index, 0)
    assert_equal(cv1.index, 0)
    assert_equal(cv1.view.index, 0)

    assert_equal(created, 1)
    assert_equal(current_view_changed, 1)
    assert_equal(closed, 0)

    cv2 = mw.create_layout(2)
    assert_equal(RBA::LayoutView::current.index, 0)
    assert_equal(RBA::LayoutView::current.active_cellview_index, 1)
    assert_equal(RBA::LayoutView::current.active_cellview.index, 1)
    assert_equal(RBA::CellView::active.index, 1)
    assert_equal(RBA::CellView::active.view.index, 0)
    assert_equal(cv2.index, 1)
    assert_equal(cv2.view.index, 0)

    assert_equal(created, 1)
    assert_equal(current_view_changed, 1)
    assert_equal(closed, 0)

    cv3 = mw.create_layout(1)
    assert_equal(RBA::LayoutView::current.index, 1)
    assert_equal(RBA::LayoutView::current.main_window.view(1).index, 1)
    assert_equal(RBA::LayoutView::current.active_cellview_index, 0)
    assert_equal(RBA::LayoutView::current.active_cellview.index, 0)
    assert_equal(RBA::CellView::active.index, 0)
    assert_equal(RBA::CellView::active.view.index, 1)
    assert_equal(cv3.index, 0)
    assert_equal(cv3.view.index, 1)

    assert_equal(mw.current_view_index, 1)
    assert_equal(created, 2)
    assert_equal(current_view_changed, 2)
    assert_equal(closed, 0)

    mw.current_view_index = 0
    assert_equal(mw.current_view_index, 0)
    assert_equal(RBA::LayoutView::current.index, 0)
    assert_equal(created, 2)
    assert_equal(current_view_changed, 3)
    assert_equal(closed, 0)

    mw.close_current_view
    assert_equal(created, 2)
    assert_equal(current_view_changed, 4)
    assert_equal(mw.current_view_index, 0)
    assert_equal(RBA::LayoutView::current.index, 0)
    assert_equal(closed, 1)

    mw.close_all
    assert_equal(RBA::LayoutView::current == nil, true)
    assert_equal(mw.current_view_index, -1)
    assert_equal(current_view_changed, 5)
    assert_equal(closed, 2)

    mw.on_view_created -= created_proc
    mw.on_current_view_changed -= current_view_changed_proc
    mw.on_view_closed -= closed_proc

  end

  def test_2

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all

    active_cellview_changed = 0
    cellviews_changed = 0
    cellview_changed = 0
    file_opened = 0
    viewport_changed = 0
    layer_list_changed = 0
    layer_list_inserted = 0
    layer_list_deleted = 0
    current_layer_list_changed = 0
    cell_visibility_changed = 0
    selection_changed = 0

    active_cellview_changed_proc = proc { active_cellview_changed += 1 }
    cellviews_changed_proc = proc { cellviews_changed += 1 }
    cellview_changed_proc = proc { cellview_changed += 1 }
    file_opened_proc = proc { file_opened += 1 }
    viewport_changed_proc = proc { viewport_changed += 1 }
    layer_list_changed_proc = proc { layer_list_changed += 1 }
    layer_list_inserted_proc = proc { layer_list_inserted += 1 }
    layer_list_deleted_proc = proc { layer_list_deleted += 1 }
    current_layer_list_changed_proc = proc { current_layer_list_changed += 1 }
    cell_visibility_changed_proc = proc { cell_visibility_changed += 1 }
    selection_changed_proc = proc { selection_changed += 1 }

    assert_equal(RBA::CellView::active.is_valid?, false)

    cv1 = mw.load_layout(ENV["TESTSRC"] + "/testdata/gds/t10.gds", 0)
    assert_equal(RBA::CellView::active.is_valid?, true)
    assert_equal(RBA::CellView::active.index, 0)

    view = cv1.view

    view.on_active_cellview_changed += active_cellview_changed_proc
    view.on_cellviews_changed += cellviews_changed_proc
    view.on_cellview_changed += cellview_changed_proc
    view.on_file_open += file_opened_proc
    view.on_viewport_changed += viewport_changed_proc
    view.on_layer_list_changed += layer_list_changed_proc
    view.on_layer_list_inserted += layer_list_inserted_proc
    view.on_layer_list_deleted += layer_list_deleted_proc
    view.on_current_layer_list_changed += current_layer_list_changed_proc
    view.on_cell_visibility_changed += cell_visibility_changed_proc
    view.on_selection_changed += selection_changed_proc

    assert_equal(cv1.index, 0)

    assert_equal(view.has_selection?, false)
    assert_equal(view.selection_size, 0)

    view.set_config("search-range-box", "5")
    view.select_from(RBA::DBox::new(-1.0, -1.0, 1.0, 1.0))
    assert_equal(selection_changed, 1)
    assert_equal(view.selection_size, 2)
    assert_equal(view.has_selection?, true)

    view.select_from(RBA::DPoint::new(0, 0), RBA::LayoutView::Invert)
    assert_equal(selection_changed, 2)
    assert_equal(view.selection_size, 3)
    assert_equal(view.has_selection?, true)

    view.clear_selection
    assert_equal(selection_changed, 3)
    assert_equal(view.has_selection?, false)
    assert_equal(view.selection_size, 0)
    selection_changed = 0

    cv2 = mw.load_layout(ENV["TESTSRC"] + "/testdata/gds/t10.gds", 2)
    assert_equal(RBA::CellView::active.index, 1)
    assert_equal(cv2.index, 1)
    assert_equal(cv2.is_valid?, true)

    assert_equal(active_cellview_changed, 1)
    assert_equal(cellviews_changed, 1)
    assert_equal(cellview_changed, 1)
    assert_equal(file_opened, 1)
    assert_equal(viewport_changed, 1)
    # TODO: this should be 1 - but that does not hurt right now
    assert_equal(layer_list_changed, 2)
    assert_equal(layer_list_inserted, 0)
    assert_equal(layer_list_deleted, 0)
    assert_equal(current_layer_list_changed, 0)
    assert_equal(cell_visibility_changed, 0)
    assert_equal(selection_changed, 0)

    view.pan_up
    assert_equal(viewport_changed, 2)
    view.zoom_fit
    assert_equal(viewport_changed, 3)

    active_cellview_changed = 0
    cellviews_changed = 0
    cellview_changed = 0
    file_opened = 0
    viewport_changed = 0
    layer_list_changed = 0
    layer_list_inserted = 0
    layer_list_deleted = 0
    current_layer_list_changed = 0
    cell_visibility_changed = 0
    selection_changed = 0

    assert_equal(cv2.cell_name, "RINGO")
    assert_equal(view.cellview(1).cell.name, "RINGO")
    cv2.cell_name = "INV2"
    assert_equal(cv2.ctx_cell.name, "INV2")
    assert_equal(view.cellview(1).cell.name, "INV2")
    assert_equal(cv2.path.collect { |p| cv2.layout.cell(p).name }.join(","), "RINGO,INV2")
    assert_equal(cv2.context_path.collect { |p| p.to_s }.join(","), "")
    assert_equal(cv2.context_trans.to_s, "r0 *1 0,0")
    assert_equal(cv2.context_dtrans.to_s, "r0 *1 0,0")

    assert_equal(active_cellview_changed, 0)
    assert_equal(cellviews_changed, 0)
    assert_equal(cellview_changed, 1)
    assert_equal(file_opened, 0)
    assert_equal(viewport_changed, 0)
    assert_equal(layer_list_changed, 0)
    assert_equal(layer_list_inserted, 0)
    assert_equal(layer_list_deleted, 0)
    assert_equal(current_layer_list_changed, 0)
    assert_equal(cell_visibility_changed, 0)
    assert_equal(selection_changed, 0)

    cv2.path = [ cv2.layout.cell("RINGO").cell_index, cv2.layout.cell("INV2").cell_index, cv2.layout.cell("TRANS").cell_index ]
    assert_equal(cv2.cell_name, "TRANS")
    assert_equal(cv2.ctx_cell.name, "TRANS")
    assert_equal(cv2.path.collect { |p| cv2.layout.cell(p).name }.join(","), "RINGO,INV2,TRANS")

    assert_equal(active_cellview_changed, 0)
    assert_equal(cellviews_changed, 0)
    assert_equal(cellview_changed, 2)
    assert_equal(file_opened, 0)
    assert_equal(viewport_changed, 0)
    assert_equal(layer_list_changed, 0)
    assert_equal(layer_list_inserted, 0)
    assert_equal(layer_list_deleted, 0)
    assert_equal(current_layer_list_changed, 0)
    assert_equal(cell_visibility_changed, 0)
    assert_equal(selection_changed, 0)

    cv2.path = [ cv2.layout.cell("RINGO").cell_index, cv2.layout.cell("INV2").cell_index ]
    assert_equal(cv2.cell_name, "INV2")
    assert_equal(cv2.ctx_cell.name, "INV2")
    assert_equal(cv2.path.collect { |p| cv2.layout.cell(p).name }.join(","), "RINGO,INV2")

    assert_equal(active_cellview_changed, 0)
    assert_equal(cellviews_changed, 0)
    assert_equal(cellview_changed, 3)
    assert_equal(file_opened, 0)
    assert_equal(viewport_changed, 0)
    assert_equal(layer_list_changed, 0)
    assert_equal(layer_list_inserted, 0)
    assert_equal(layer_list_deleted, 0)
    assert_equal(current_layer_list_changed, 0)
    assert_equal(cell_visibility_changed, 0)
    assert_equal(selection_changed, 0)

    sp = []
    cv2.cell.each_inst { |i| sp << RBA::InstElement::new(i); break }
    cv2.context_path = sp

    assert_equal(cv2.context_path.collect { |p| p.inst.cell.name + ":" + p.specific_cplx_trans.to_s }.join(","), "TRANS:r0 *1 -400,0")
    assert_equal(cv2.context_trans.to_s, "r0 *1 -400,0")
    assert_equal(cv2.context_dtrans.to_s, "r0 *1 -0.4,0")
    assert_equal(cv2.cell_name, "TRANS")
    assert_equal(cv2.ctx_cell.name, "INV2")

    assert_equal(active_cellview_changed, 0)
    assert_equal(cellviews_changed, 0)
    assert_equal(cellview_changed, 4)
    assert_equal(file_opened, 0)
    assert_equal(viewport_changed, 0)
    assert_equal(layer_list_changed, 0)
    assert_equal(layer_list_inserted, 0)
    assert_equal(layer_list_deleted, 0)
    assert_equal(current_layer_list_changed, 0)
    assert_equal(cell_visibility_changed, 0)
    assert_equal(selection_changed, 0)

    cv2.ascend

    assert_equal(cv2.context_path.collect { |p| p.inst.cell.name + ":" + p.specific_cplx_trans.to_s }.join(","), "")
    assert_equal(cv2.cell_name, "INV2")
    assert_equal(cv2.ctx_cell.name, "INV2")

    assert_equal(active_cellview_changed, 0)
    assert_equal(cellviews_changed, 0)
    assert_equal(cellview_changed, 5)
    assert_equal(file_opened, 0)
    assert_equal(viewport_changed, 0)
    assert_equal(layer_list_changed, 0)
    assert_equal(layer_list_inserted, 0)
    assert_equal(layer_list_deleted, 0)
    assert_equal(current_layer_list_changed, 0)
    assert_equal(cell_visibility_changed, 0)
    assert_equal(selection_changed, 0)

    assert_equal(view.cellviews, 2)

    cv2.close

    assert_equal(cv2.is_valid?, false)
    assert_equal(cv1.is_valid?, true)
    assert_equal(view.cellviews, 1)
    assert_equal(RBA::CellView.active.index, -1)

    assert_equal(active_cellview_changed, 0)
    assert_equal(cellviews_changed, 1)
    assert_equal(cellview_changed, 5)
    assert_equal(file_opened, 0)
    assert_equal(viewport_changed, 0)
    assert_equal(layer_list_changed, 1)
    assert_equal(layer_list_inserted, 0)
    assert_equal(layer_list_deleted, 0)
    assert_equal(current_layer_list_changed, 0)
    assert_equal(cell_visibility_changed, 0)
    assert_equal(selection_changed, 0)

    active_cellview_changed = 0
    cellviews_changed = 0
    cellview_changed = 0
    file_opened = 0
    viewport_changed = 0
    layer_list_changed = 0
    layer_list_inserted = 0
    layer_list_deleted = 0
    current_layer_list_changed = 0
    cell_visibility_changed = 0
    selection_changed = 0

    assert_equal(cv1.is_cell_hidden?(cv1.cell), false)
    cv1.hide_cell(cv1.cell)
    assert_equal(cv1.is_cell_hidden?(cv1.cell), true)
    assert_equal(cell_visibility_changed, 1)
    cv1.show_cell(cv1.cell)
    assert_equal(cv1.is_cell_hidden?(cv1.cell), false)
    assert_equal(cell_visibility_changed, 2)
    cv1.hide_cell(cv1.cell)
    assert_equal(cv1.is_cell_hidden?(cv1.cell), true)
    assert_equal(cell_visibility_changed, 3)
    cv1.show_all_cells
    assert_equal(cv1.is_cell_hidden?(cv1.cell), false)
    assert_equal(cell_visibility_changed, 4)

    view.on_active_cellview_changed -= active_cellview_changed_proc
    view.on_cellviews_changed -= cellviews_changed_proc
    view.on_cellview_changed -= cellview_changed_proc
    view.on_file_open -= file_opened_proc
    view.on_viewport_changed -= viewport_changed_proc
    view.on_layer_list_changed -= layer_list_changed_proc
    view.on_layer_list_inserted -= layer_list_inserted_proc
    view.on_layer_list_deleted -= layer_list_deleted_proc
    view.on_current_layer_list_changed -= current_layer_list_changed_proc
    view.on_cell_visibility_changed -= cell_visibility_changed_proc
    view.on_selection_changed -= selection_changed_proc

  end

  def test_3

    # standalone layout view
    lv = RBA::LayoutView::new

    cv = lv.cellview(lv.create_layout(1))
    cv.layout.create_cell("TOP")
    cv.cell = cv.layout.top_cell

    assert_equal(lv.cellview(cv.index).cell.name, "TOP")

    # insert layer with default initializer
    lp = lv.insert_layer(lv.begin_layers)
    lp.fill_color = 0xffff31cc
    assert_equal(lv.begin_layers.current.fill_color, 0xffff31cc)

    lv.clear_layers

    lp = lv.insert_layer(0, lv.begin_layers)
    lp.fill_color = 0xffff31cc
    assert_equal(lv.begin_layers.current.fill_color, 0xffff31cc)

    # should not segfault
    begin
      lv.insert_layer(42, lv.begin_layers(42))
      assert_equal(true, false)
    rescue => ex
    end

  end

  def test_4

    # check if PNG support is compiled in
    # TODO: remove this once PNG support is available on all platforms
    begin
      RBA::PixelBuffer.read_png(File.join($ut_testsrc, "testdata/lay/au_lv1.png"))
    rescue => ex
      # PNG support missing
      puts "WARNING: PNG support not compiled in - skipping test"
      return
    end

    # standalone image generation (see C++ tests)
    lv = RBA::LayoutView::new
    lv.set_config("inst-color", "#000000")
    lv.set_config("background-color", "#ffffff")
    lv.load_layout(File.join($ut_testsrc, "testdata/gds/t10.gds"), true)

    img = lv.get_pixels_with_options(500, 500, 1, 1, 1.0, RBA::DBox::new)
    au = RBA::PixelBuffer.read_png(File.join($ut_testsrc, "testdata/lay/au_lv1.png"))
    if au
      assert_equal(au == img, true)
    end

    lv.set_config("full-hierarchy-new-cell", "true")
    lv.load_layout(File.join($ut_testsrc, "testdata/gds/t10.gds"), false)

    img = lv.get_pixels_with_options(500, 500, 1, 1, 1.0, RBA::DBox::new)
    au = RBA::PixelBuffer.read_png(File.join($ut_testsrc, "testdata/lay/au_lv2.png"))
    if au
      assert_equal(au == img, true)
    end

    img = lv.get_pixels_with_options_mono(500, 500, 1, RBA::DBox::new)
    au = RBA::BitmapBuffer.read_png(File.join($ut_testsrc, "testdata/lay/au_lv3.png"))
    if au
      assert_equal(au == img, true)
    end

  end

  def test_5

    [true, false].each do |editable|

      lv = RBA::LayoutView::new(editable)
      lv.load_layout(File.join($ut_testsrc, "testdata/gds/t10.gds"), true)

      lv.resize(42, 117)
      img = lv.get_screenshot_pixels
      assert_equal(img.width, 42)
      assert_equal(img.height, 117)

      lv.resize(142, 217)
      img = lv.get_screenshot_pixels
      assert_equal(img.width, 142)
      assert_equal(img.height, 217)

      if editable
        assert_equal(lv.mode_names, ["select", "move", "ruler", "polygon", "box", "text", "path", "instance", "partial"])
      else
        assert_equal(lv.mode_names, ["select", "move", "ruler"])
      end
      assert_equal(lv.mode_name, "select")
      lv.switch_mode("ruler")
      assert_equal(lv.mode_name, "ruler")
      lv.switch_mode("move")
      assert_equal(lv.mode_name, "move")
      lv.switch_mode("select")
      assert_equal(lv.mode_name, "select")

    end

  end

  class DummyPlugin < RBA::Plugin
    def initialize(manager, view)
      self.manager = manager
      self.view = view
    end
  end

  class DummyPluginFactory < RBA::PluginFactory
    def initialize()
      register(1000, "dummy_plugin", "Dummy Plugin")
    end
    def create_plugin(manager, unused, view)
      DummyPlugin::new(manager, view)
    end
  end

  # issue-1242
  def test_6

    if !RBA.constants.member?(:MainWindow)
      return
    end

    # Create a new layout
    main_window = RBA::MainWindow.instance()
    main_window.close_all
    main_window.create_layout(2)

    # Register plugin -> crashes in issue-1242
    dpi = DummyPluginFactory::new

    main_window.close_all

    dpi._destroy

  end

  # issue-1259
  def test_7

    # standalone layout view
    lv = RBA::LayoutView::new(true)
    
    assert_equal(lv.is_editable, true)

    lv.create_layout(true)

    layout = lv.active_cellview.layout
    assert_equal(layout.is_editable, true)

  end

  # issue-1533
  def test_8

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    mw.close_all
    mw.create_view

    ly = RBA::Layout::new
    mw.current_view.show_layout(ly, false)

    # was crashing
    mw.current_view.show_layout(ly, false)

  end

end

load("test_epilogue.rb")


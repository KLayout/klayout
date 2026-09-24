# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2026 Matthias Koefferlein
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

class LAYMainWindow_TestClass < TestBase

  # Basic view creation and MainWindow events
  def test_1

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window

    mw.title = "ABC$(1+2)"
    assert_equal("ABC$(1+2)", mw.title)

    if RBA.constants.member?(:QWidget)
      # string is interpolated
      assert_equal("ABC3", mw.windowTitle)
    end

  end

  def test_2

    # smoke test
    
    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    s = mw.synchronous

    mw.synchronous = true
    assert_equal(true, mw.synchronous)

    mw.synchronous = false
    assert_equal(false, mw.synchronous)

    mw.synchronous = true

  end

  def test_3

    if !RBA.constants.member?(:Application)
      return
    end

    mw = RBA::Application.instance.main_window
    synchronized_layers = mw.get_config("synchronized-layers").to_s
    mw.set_config("synchronized-layers", "false")
    mw.close_all

    begin
      path = ENV["TESTSRC"] + "/testdata/gds/t11.gds"
      mw.load_layout(path, 1)
      first = mw.current_view
      mw.load_layout(path, 1)
      second = mw.current_view
      first.clear_layers
      second.clear_layers

      indexed = RBA::LayerProperties::new
      indexed_source = "%#{first.cellview(0).layout.layer(1, 0)}@1"
      indexed.source = indexed_source
      first.insert_layer(first.end_layers, indexed)

      second.insert_layer(second.end_layers, RBA::LayerProperties::new)
      group = second.begin_layers
      matched = RBA::LayerProperties::new
      matched.source = "1/0@1"
      group.current.add_child(matched)
      unique = RBA::LayerProperties::new
      unique.source = "2/0@1"
      group.current.add_child(unique)
      group.current.visible = false

      mw.current_view_index = 0
      mw.set_config("synchronized-layers", "true")
      assert_equal(group.first_child.current.visible?(true), true)
      sibling = group.first_child
      sibling.next_sibling(1)
      assert_equal(sibling.current.visible?(true), false)

      first.transaction("Hide layer")
      hidden = RBA::LayerProperties::new
      hidden.source = indexed_source
      hidden.visible = false
      first.set_layer_properties(first.begin_layers, hidden)
      first.commit

      mw.cm_undo
      assert_equal(first.begin_layers.current.visible?(true), true)
      assert_equal(group.first_child.current.visible?(true), true)
      mw.cm_redo
      assert_equal(first.begin_layers.current.visible?(true), false)
      assert_equal(group.first_child.current.visible?(true), false)
      assert_equal(sibling.current.visible?(true), false)

      first.transaction("Show layer")
      shown = RBA::LayerProperties::new
      shown.source = indexed_source
      first.set_layer_properties(first.begin_layers, shown)
      first.commit
      mw.current_view_index = 1
      assert_equal(group.first_child.current.visible?(true), true)
      assert_equal(sibling.current.visible?(true), false)
    ensure
      mw.set_config("synchronized-layers", synchronized_layers)
      mw.close_all
    end

  end

end

load("test_epilogue.rb")

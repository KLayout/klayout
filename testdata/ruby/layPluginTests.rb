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

class PluginFactory < RBA::PluginFactory
  def initialize()
    register(1000, "plugin_for_test", "Plugin")
    @pi = nil
  end
  def create_plugin(manager, unused, view)
    @pi = RBA::Plugin::new
    @pi
  end
  def pi
    @pi
  end
end

has_qt = RBA.constants.member?(:EditorOptionsPage) && RBA.constants.member?(:MainWindow)
if has_qt

  class Plugin2EditorOptionsPage < RBA::EditorOptionsPage
    def initialize
      super("title", 1)
    end
  end

  class Plugin2ConfigPage < RBA::ConfigPage
    def initialize
      super("title")
    end
  end

  class Plugin2 < RBA::Plugin
    def set_tp(tp)
      @tp = tp
    end
    def has_tracking_position
      !!@tp
    end
    def tracking_position
      @tp || RBA::DPoint::new
    end
  end

  class PluginFactory2 < RBA::PluginFactory
    def initialize()
      @ep = 0
      @cp = 0
      @pi = nil
      register(1001, "plugin_for_test2", "Plugin2")
    end
    def create_plugin(manager, unused, view)
      @pi = Plugin2::new
      @pi
    end
    def create_editor_options_pages
      add_editor_options_page(Plugin2EditorOptionsPage::new)
      @ep += 1
    end
    def create_config_pages
      add_config_page(Plugin2ConfigPage::new)
      @cp += 1
    end
    def pi
      @pi
    end
    def ep
      @ep
    end
    def cp
      @cp
    end
  end

  class LayPlugin_TestClass < TestBase

    def test_1

      assert_equal(RBA::Plugin::AC_Global.to_s, "AC_Global")
      assert_equal(RBA::Plugin::AC_Any.to_s, "AC_Any")
      assert_equal(RBA::Plugin::AC_Diagonal.to_s, "AC_Diagonal")
      assert_equal(RBA::Plugin::AC_Horizontal.to_s, "AC_Horizontal")
      assert_equal(RBA::Plugin::AC_Vertical.to_s, "AC_Vertical")
      
      assert_equal(RBA::Plugin::ac_from_buttons(0), RBA::Plugin::AC_Global)
      assert_equal(RBA::Plugin::ac_from_buttons(1), RBA::Plugin::AC_Ortho)
      assert_equal(RBA::Plugin::ac_from_buttons(2), RBA::Plugin::AC_Diagonal)

      begin

        dpi = PluginFactory::new
        dpi2 = PluginFactory2::new
        
        # Create a new layout
        main_window = RBA::MainWindow.instance()
        main_window.close_all
        main_window.create_layout(2)

        pi = dpi.pi
        pi2 = dpi2.pi
        
        # smoke test
        pi.grab_mouse
        pi.ungrab_mouse
        pi.set_cursor(RBA::Cursor::Wait)
        pi.add_edge_marker(RBA::DEdge::new)
        pi.add_mouse_cursor(RBA::DPoint::new)
        pi.clear_mouse_cursors

        # virtual methods
        assert_equal(pi.has_tracking_position_test, false)
        pi.clear_mouse_cursors
        pi.add_mouse_cursor(RBA::DPoint::new(1, 2))
        assert_equal(pi.has_tracking_position_test, true)
        assert_equal(pi.tracking_position_test.to_s, "1,2")
        pi.clear_mouse_cursors
        assert_equal(pi.has_tracking_position_test, false)

        assert_equal(pi2.has_tracking_position_test, false)
        pi2.set_tp(RBA::DPoint::new(2, 3))
        assert_equal(pi2.has_tracking_position_test, true)
        assert_equal(pi2.tracking_position_test.to_s, "2,3")
        pi2.set_tp(nil)
        assert_equal(pi2.has_tracking_position_test, false)

        assert_equal(dpi2.ep, 1)
        assert_equal(dpi2.cp, 1)
        assert_equal(pi2.editor_options_pages.size, 1)
        assert_equal(pi2.editor_options_pages[0].class.to_s, "Plugin2EditorOptionsPage")

        pi.configure_test("edit-grid", "0.0")
        assert_equal(pi.snap(RBA::DPoint::new(0.01, 0.02)).to_s, "0.01,0.02")
        assert_equal(pi.snap(RBA::DVector::new(0.01, 0.02)).to_s, "0.01,0.02")
        pi.configure_test("edit-grid", "0.1")
        assert_equal(pi.snap(RBA::DPoint::new(0.11, 0.18)).to_s, "0.1,0.2")
        assert_equal(pi.snap(RBA::DVector::new(0.11, 0.18)).to_s, "0.1,0.2")

        pi.configure_test("edit-connect-angle-mode", "ortho")
        assert_equal(pi.snap(RBA::DPoint::new(1.5, 2.1), RBA::DPoint::new(1, 2), true).to_s, "1.5,2")
        assert_equal(pi.snap(RBA::DPoint::new(1.5, 2.1), RBA::DPoint::new(1, 2), false).to_s, "1.5,2.1")
        assert_equal(pi.snap(RBA::DPoint::new(1.5, 2.1), RBA::DPoint::new(1, 2), false, RBA::Plugin::AC_Ortho).to_s, "1.5,2")

        pi.configure_test("edit-connect-angle-mode", "ortho")
        assert_equal(pi.snap(RBA::DVector::new(0.5, 2.1), true).to_s, "0,2.1")
        assert_equal(pi.snap(RBA::DVector::new(0.5, 2.1), false).to_s, "0.5,2.1")
        assert_equal(pi.snap(RBA::DVector::new(0.5, 2.1), false, RBA::Plugin::AC_Ortho).to_s, "0,2.1")

      ensure
        main_window.close_all
        dpi._destroy
        dpi2._destroy
      end

    end

  end

else

  # at least one test is needed
  class LayPlugin_TestClass < TestBase
  end

end

load("test_epilogue.rb")


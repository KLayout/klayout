# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2024 Matthias Koefferlein
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

end

load("test_epilogue.rb")


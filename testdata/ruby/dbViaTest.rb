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


class DBVia_TestClass < TestBase

  # Via basics
  def test_1_Via

    via = RBA::ViaType::new("name", "desc")

    assert_equal(via.name, "name")
    via.name = "none"
    assert_equal(via.name, "none")

    assert_equal(via.description, "desc")
    via.description = "-"
    assert_equal(via.description, "-")

    via.wbmin = 0.5
    assert_equal(via.wbmin.to_s, "0.5")
    via.wtmin = 0.25
    assert_equal(via.wtmin.to_s, "0.25")

    via.hbmin = 1.5
    assert_equal(via.hbmin.to_s, "1.5")
    via.htmin = 1.25
    assert_equal(via.htmin.to_s, "1.25")

    via.bottom = RBA::LayerInfo::new(1, 0)
    assert_equal(via.bottom.to_s, "1/0")

    via.cut = RBA::LayerInfo::new(2, 0)
    assert_equal(via.cut.to_s, "2/0")

    via.top = RBA::LayerInfo::new(3, 0)
    assert_equal(via.top.to_s, "3/0")

  end

end

load("test_epilogue.rb")

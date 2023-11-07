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


class DBLog_TestClass < TestBase

  def test_1_Log

    assert_equal(RBA::LogEntryData::NoSeverity.to_s, "NoSeverity")
    assert_equal(RBA::LogEntryData::Info.to_s, "Info")
    assert_equal(RBA::LogEntryData::Error.to_s, "Error")
    assert_equal(RBA::LogEntryData::Warning.to_s, "Warning")

    le = RBA::LogEntryData::new

    le.message = "message"
    assert_equal(le.message, "message")
    
    le.geometry = RBA::DPolygon::new(RBA::DBox::new(1, 2, 3, 4))
    assert_equal(le.geometry.to_s, "(1,2;1,4;3,4;3,2)")
    
    le.category_name = "42"
    assert_equal(le.category_name, "42")
    
    le.category_description = "the answer"
    assert_equal(le.category_description, "the answer")
    
    le.cell_name = "TOP"
    assert_equal(le.cell_name, "TOP")
    
    assert_equal(le.to_s, "[the answer] In cell TOP: message, shape: (1,2;1,4;3,4;3,2)")

  end

end

load("test_epilogue.rb")

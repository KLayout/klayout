# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2019 Matthias Koefferlein
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

class DBNetlistExtractorTests_TestClass < TestBase

  def test_1_Error
  
    err = RBA::NetlistDeviceExtractorError::new

    err.message = "MSG"
    err.cell_name = "Cell"
    err.category_name = "Cat"
    err.category_description = "CatDesc"
    err.geometry = RBA::DPolygon::new(RBA::DBox::new(1, 2, 3, 4))

    assert_equal(err.message, "MSG")
    assert_equal(err.cell_name, "Cell")
    assert_equal(err.category_name, "Cat")
    assert_equal(err.category_description, "CatDesc")
    assert_equal(err.geometry.to_s, "(1,2;1,4;3,4;3,2)")

  end

  def test_2_Basic

    ex = RBA::DeviceExtractorMOS3Transistor::new
    assert_equal(ex.class, RBA::DeviceExtractorMOS3Transistor)

    ex = RBA::DeviceExtractorMOS4Transistor::new
    assert_equal(ex.class, RBA::DeviceExtractorMOS4Transistor)

    ex = RBA::DeviceExtractorBJT3Transistor::new
    assert_equal(ex.class, RBA::DeviceExtractorBJT3Transistor)

    ex = RBA::DeviceExtractorBJT4Transistor::new
    assert_equal(ex.class, RBA::DeviceExtractorBJT4Transistor)

    ex = RBA::DeviceExtractorDiode::new
    assert_equal(ex.class, RBA::DeviceExtractorDiode)

    ex = RBA::DeviceExtractorResistor::new
    assert_equal(ex.class, RBA::DeviceExtractorResistor)

    ex = RBA::DeviceExtractorResistorWithBulk::new
    assert_equal(ex.class, RBA::DeviceExtractorResistorWithBulk)

    ex = RBA::DeviceExtractorCapacitor::new
    assert_equal(ex.class, RBA::DeviceExtractorCapacitor)

    ex = RBA::DeviceExtractorCapacitorWithBulk::new
    assert_equal(ex.class, RBA::DeviceExtractorCapacitorWithBulk)

  end

end

load("test_epilogue.rb")


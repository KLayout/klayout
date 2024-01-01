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

class DBLibrary_TestClass < TestBase

  def test_1_registration

    lib = RBA::Library::new

    assert_equal(lib.name, "")
    noid = lib.id;

    lib.register("RBA-unit-test")

    assert_equal(lib.name, "RBA-unit-test")
    lib_id = lib.id
    assert_equal(lib_id != noid, true)

    # the layout inside the library knows the library
    assert_equal(lib.layout.library.id == lib.id, true)
    assert_equal(lib.layout.library.name, "RBA-unit-test")

    assert_equal(RBA::Library::library_names.member?("RBA-unit-test"), true)
    assert_equal(RBA::Library::library_by_name("RBA-unit-test").id, lib_id)

    # destroy should not do anything as libraries are not to be removed through the destructor
    lib._destroy
    assert_equal(RBA::Library::library_by_name("RBA-unit-test").id, lib_id)
    assert_equal(lib.destroyed?, true)

    lib = RBA::Library::library_by_name("RBA-unit-test")
    assert_equal(lib.destroyed?, false)
    lib.delete
    assert_equal(RBA::Library::library_by_name("RBA-unit-test"), nil)

  end

  def test_2_attributes

    lib = RBA::Library::new

    lib.description = "42 is the answer"
    assert_equal(lib.description, "42 is the answer")

    assert_equal(lib.is_for_technology("X"), false)
    assert_equal(lib.technologies, [])
    assert_equal(lib.for_technologies, false)

    lib.technology = "X"
    assert_equal(lib.is_for_technology("X"), true)
    assert_equal(lib.technologies, ["X"])
    assert_equal(lib.for_technologies, true)

    lib.technology = ""
    assert_equal(lib.is_for_technology("X"), false)
    assert_equal(lib.technologies, [])
    assert_equal(lib.for_technologies, false)

    lib.add_technology("Y")
    assert_equal(lib.is_for_technology("X"), false)
    assert_equal(lib.is_for_technology("Y"), true)
    assert_equal(lib.technologies, ["Y"])
    assert_equal(lib.for_technologies, true)

    lib.clear_technologies
    assert_equal(lib.is_for_technology("Y"), false)
    assert_equal(lib.technologies, [])
    assert_equal(lib.for_technologies, false)

  end

  def test_3_layout

    lib = RBA::Library::new
    lib.layout.create_cell("X")
    assert_equal(lib.layout.top_cell.name, "X")

    # this will actually destroy the library as it is not registered
    lib._destroy
    assert_equal(lib.destroyed?, true)

  end

end

load("test_epilogue.rb")


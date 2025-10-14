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

class MyLibImpl < RBA::Library
  def initialize
    @reload_count = 0
  end
  def reload_count
    @reload_count
  end
  def reload
    @reload_count += 1
    return "RBA-unit-test2"
  end
end

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

    # The library reference is kept internally
    lib = nil
    GC.start
    GC.start

    lib = RBA::Library::library_by_name("RBA-unit-test")
    assert_equal(lib.name, "RBA-unit-test")

    lib._destroy
    assert_equal(lib.destroyed?, true)

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

  def test_4_library_registration_and_rename

    lib = RBA::Library::new
    lib.description = "LIB1"
    lib.delete
    assert_equal(lib.destroyed?, true)

    lib = RBA::Library::new
    lib.layout.create_cell("A")
    lib.description = "LIB1"
    lib.register("RBA-unit-test")
    assert_equal(RBA::Library::library_by_name("RBA-unit-test").description, "LIB1")

    lib.unregister
    assert_equal(lib.destroyed?, false)
    assert_equal(RBA::Library::library_by_name("RBA-unit-test"), nil)

    lib.register("RBA-unit-test")
    assert_equal(RBA::Library::library_by_name("RBA-unit-test").description, "LIB1")

    ly = RBA::Layout::new
    ci = ly.create_cell("A", "RBA-unit-test").cell_index
    assert_equal(ly.cell(ci).qname, "RBA-unit-test.A")

    lib.rename("RBA-unit-test2")
    assert_equal(RBA::Library::library_by_name("RBA-unit-test"), nil)
    assert_equal(RBA::Library::library_by_name("RBA-unit-test2").description, "LIB1")

    assert_equal(ly.cell(ci).qname, "RBA-unit-test2.A")

    lib.delete
    assert_equal(RBA::Library::library_by_name("RBA-unit-test"), nil)
    assert_equal(RBA::Library::library_by_name("RBA-unit-test2"), nil)

    assert_equal(ly.cell(ci).qname, "<defunct>RBA-unit-test2.A")

  end

  def test_5_reload

    lib = MyLibImpl::new
    lib.description = "LIB1"
    lib.register("RBA-unit-test")
    assert_equal(RBA::Library::library_by_name("RBA-unit-test").description, "LIB1")

    lib.refresh
    assert_equal(RBA::Library::library_by_name("RBA-unit-test"), nil)
    assert_equal(RBA::Library::library_by_name("RBA-unit-test2").description, "LIB1")

    assert_equal(lib.reload_count, 1)

    lib.delete
    assert_equal(RBA::Library::library_by_name("RBA-unit-test"), nil)
    assert_equal(RBA::Library::library_by_name("RBA-unit-test2"), nil)

  end

  def test_6_cells_become_defunct_after_unregister

    lib = RBA::Library::new
    lib.description = "LIB1"
    lib.register("RBA-unit-test")

    cell_a = lib.layout.create_cell("A")
    l1 = lib.layout.layer(1, 0)
    cell_a.shapes(l1).insert(RBA::Box::new(0, 0, 1000, 2000))

    ly = RBA::Layout::new
    lc = ly.create_cell("A", "RBA-unit-test")
    assert_equal(lc.qname, "RBA-unit-test.A")
    ci = lc.cell_index

    lib.unregister

    # NOTE: cleanup has not been called, so we can find the cell using the cell_index
    # (the actual cell object is no longer there because it has been replaced by
    # a cold proxy)
    lc = ly.cell(ci)
    assert_equal(lc.qname, "<defunct>RBA-unit-test.A")

    # this will restore the reference
    lib.register("RBA-unit-test")

    lc = ly.cell(ci)
    assert_equal(lc.qname, "RBA-unit-test.A")

  end

  def test_7_change_ref

    lib = RBA::Library::new
    lib.description = "LIB1"
    lib.register("RBA-unit-test")
    l1 = lib.layout.layer(1, 0)

    cell_a = lib.layout.create_cell("A")
    cell_a.shapes(l1).insert(RBA::Box::new(0, 0, 1000, 2000))

    lib2 = RBA::Library::new
    lib2.description = "LIB2"
    lib2.register("RBA-unit-test2")
    l1 = lib2.layout.layer(1, 0)

    cell_b = lib2.layout.create_cell("B")
    cell_b.shapes(l1).insert(RBA::Box::new(0, 0, 2000, 1000))

    ly = RBA::Layout::new
    c1 = ly.create_cell("A", "RBA-unit-test")
    assert_equal(c1.qname, "RBA-unit-test.A")

    c1.change_ref(lib2.id, cell_b.cell_index)
    assert_equal(c1.qname, "RBA-unit-test2.B")

    ly = RBA::Layout::new
    c1 = ly.create_cell("A", "RBA-unit-test")
    assert_equal(c1.qname, "RBA-unit-test.A")

    c1.change_ref("RBA-unit-test2", "B")
    assert_equal(c1.qname, "RBA-unit-test2.B")

  end

end

load("test_epilogue.rb")


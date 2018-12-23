# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2018 Matthias Koefferlein
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

class DBNetlist_TestClass < TestBase

  def test_1_Basic

    nl = RBA::Netlist::new
    c = nl.create_circuit

    c.name = "XYZ"
    assert_equal(c.name, "XYZ")

    c.cell_index = 42
    assert_equal(c.cell_index, 42)

    cc = nl.create_circuit
    cc.name = "ABC"

    names = []
    nl.each_circuit { |i| names << i.name }
    assert_equal(names, [ c.name, cc.name ])

    # NOTE: this will also remove the circuit from the netlist
    cc._destroy

    names = []
    nl.each_circuit { |i| names << i.name }
    assert_equal(names, [ c.name ])

    cc = nl.create_circuit
    cc.name = "UVW"

    names = []
    nl.each_circuit { |i| names << i.name }
    assert_equal(names, [ c.name, cc.name ])

    # same as _destroy
    nl.remove_circuit(c)

    names = []
    nl.each_circuit { |i| names << i.name }
    assert_equal(names, [ cc.name ])

  end

end

load("test_epilogue.rb")

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

class DBCellTests_TestClass < TestBase

  def make_layout

    ly = RBA::Layout::new
    top = ly.create_cell("TOP")
    a = ly.create_cell("A")
    l1 = ly.layer(1, 0)
    a.shapes(l1).insert(RBA::DBox::new(1, 2, 3, 4))
    top.insert(RBA::DCellInstArray::new(a.cell_index, RBA::DTrans::R0))
    top.insert(RBA::DCellInstArray::new(a.cell_index, RBA::DTrans::R90))

    ly

  end

  # writing cells
  def test_1

    tmp = File::join($ut_testtmp, "tmp.gds")

    ly = make_layout
    top = ly.top_cell

    top.write(tmp)

    ly2 = RBA::Layout::new
    ly2.read(tmp)

    assert_equal(RBA::Region::new(ly2.top_cell.begin_shapes_rec(ly2.layer(1, 0))).to_s, "(1000,2000;1000,4000;3000,4000;3000,2000);(-4000,1000;-4000,3000;-2000,3000;-2000,1000)")

    tmp = File::join($ut_testtmp, "tmp2.gds")

    opt = RBA::SaveLayoutOptions::new
    opt.dbu = 0.005
    top.write(tmp, opt)

    ly2 = RBA::Layout::new
    ly2.read(tmp)

    assert_equal(RBA::Region::new(ly2.top_cell.begin_shapes_rec(ly2.layer(1, 0))).to_s, "(200,400;200,800;600,800;600,400);(-800,200;-800,600;-400,600;-400,200)")

  end

  # reading cells
  def test_2

    tmp = File::join($ut_testtmp, "tmp.gds")

    ly = make_layout
    top = ly.top_cell

    top.write(tmp)

    ly2 = RBA::Layout::new
    ly2.dbu = 0.005
    top = ly2.create_cell("IMPORTED")
    top.read(tmp)

    assert_equal(RBA::Region::new(top.begin_shapes_rec(ly2.layer(1, 0))).to_s, "(200,400;200,800;600,800;600,400);(-800,200;-800,600;-400,600;-400,200)")

    ly2 = RBA::Layout::new
    ly2.dbu = 0.005
    top = ly2.create_cell("IMPORTED")

    opt = RBA::LoadLayoutOptions::new
    lm = RBA::LayerMap::new
    lm.map("1/0 : 5/0", 0)
    opt.layer_map = lm
    top.read(tmp, opt)

    assert_equal(RBA::Region::new(top.begin_shapes_rec(ly2.layer(5, 0))).to_s, "(200,400;200,800;600,800;600,400);(-800,200;-800,600;-400,600;-400,200)")

  end

  # methods with LayerInfo instead layer index
  def test_3

    ly = RBA::Layout::new
    top = ly.create_cell("TOP")

    l1 = ly.layer(1, 0)
    top.shapes(RBA::LayerInfo::new(1, 0)).insert(RBA::Box::new(0, 0, 100, 200))

    assert_equal(top.shapes(l1).size, 1)

    # unknown layers are ignored in clear
    top.clear(RBA::LayerInfo::new(2, 0))
    assert_equal(top.shapes(l1).size, 1)

    # clear with LayerInfo
    top.clear(RBA::LayerInfo::new(1, 0))
    assert_equal(top.shapes(l1).size, 0)

    # layer is created if not there
    assert_equal(ly.layer_infos, [ RBA::LayerInfo::new(1, 0) ])
    top.shapes(RBA::LayerInfo::new(2, 0)).insert(RBA::Box::new(0, 0, 100, 200))
    assert_equal(ly.layer_infos, [ RBA::LayerInfo::new(1, 0), RBA::LayerInfo::new(2, 0) ])

  end

end

load("test_epilogue.rb")

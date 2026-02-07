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

  # locking
  def test_4

    ly = RBA::Layout::new
    top = ly.create_cell("TOP")
    other = ly.create_cell("OTHER")
    child = ly.create_cell("CHILD")

    l1 = ly.layer(1, 0)
    l2 = ly.layer(2, 0)
    shape = top.shapes(l1).insert(RBA::Box::new(0, 0, 100, 200))
    inst = top.insert(RBA::CellInstArray::new(child, RBA::Trans::new(RBA::Vector::new(100, 200))))
    other.insert(RBA::CellInstArray::new(child, RBA::Trans::new(RBA::Vector::new(10, 20))))

    assert_equal(top.is_locked?, false)
    top.locked = false
    assert_equal(top.is_locked?, false)
    top.locked = true
    assert_equal(top.is_locked?, true)

    # rename is still possible
    top.name = "TOP2"

    # instances of the cell can still be created
    toptop = ly.create_cell("TOPTOP")
    toptop.insert(RBA::CellInstArray::new(top, RBA::Trans::new))
    assert_equal(top.each_parent_inst.to_a[0].child_inst.to_s, "cell_index=0 r0 0,0")

    begin
      # forbidden now
      top.shapes(l2).insert(RBA::Box::new(0, 0, 100, 200))
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Shapes::insert")
    end

    begin
      # forbidden now
      shape.box = RBA::Box::new(1, 2, 101, 202)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Shape::box=")
    end

    begin
      # forbidden now
      shape.prop_id = 1
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Shape::prop_id=")
    end

    begin
      # forbidden now
      shape.polygon = RBA::Polygon::new(RBA::Box::new(0, 0, 200, 100))
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Shape::polygon=")
    end

    begin
      # forbidden now
      inst.cell_index = other.cell_index
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Instance::cell_index=")
    end

    begin
      # forbidden now
      inst.prop_id = 1
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Instance::prop_id=")
    end

    begin
      # also forbidden
      top.insert(RBA::CellInstArray::new(child, RBA::Trans::new))
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::insert")
    end

    begin
      # clear is forbidding
      top.clear
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::clear")
    end

    begin
      # clear layer is forbidden
      top.shapes(l1).clear
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Shapes::clear")
    end

    begin
      # clear layer is forbidden
      top.clear(l1)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::clear")
    end

    begin
      # layer copy is forbidden
      top.copy(l1, l2)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::copy")
    end

    begin
      # layer move is forbidden
      top.move(l1, l2)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::move")
    end

    begin
      # layer swap is forbidden
      top.swap(l1, l2)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::swap")
    end

    begin
      # copy_instances is forbidden
      top.copy_instances(other)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::copy_instances")
    end

    begin
      # move_instances is forbidden
      top.move_instances(other)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::move_instances")
    end

    begin
      # copy_tree is forbidden
      top.copy_tree(other)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::copy_tree")
    end

    begin
      # move_tree is forbidden
      top.move_tree(other)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::move_tree")
    end

    begin
      # copy_shapes is forbidden
      top.copy_shapes(other)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::copy_shapes")
    end

    begin
      # move_shapes is forbidden
      top.move_shapes(other)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::move_shapes")
    end

    begin
      # flatten is forbidden
      top.flatten(true)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::flatten")
    end

    begin
      # cell cannot be deleted
      top.delete
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::delete")
    end

    # locked attribute is copied
    ly2 = ly.dup
    assert_equal(ly2.cell("TOP2").is_locked?, true)

    begin
      # cell cannot be deleted
      ly2.cell("TOP2").delete
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Cell 'TOP2' cannot be modified as it is locked in Cell::delete")
    end

    # clear and _destroy is always possible
    ly2.clear

    ly2 = ly.dup
    ly2._destroy

    # resetting the locked flag enables things again (not all tested here)
    top.locked = false

    inst.cell_index = other.cell_index
    top.shapes(l2).insert(RBA::Box::new(0, 0, 100, 200))
    shape.box = RBA::Box::new(1, 2, 101, 202)
    top.delete

  end

  # variant enums
  def test_VariantType

    vt = RBA::VariantType::NoVariants
    assert_equal(vt.to_s, "NoVariants")

    vt = RBA::VariantType::Orientation
    assert_equal(vt.to_s, "Orientation")

    vt = RBA::VariantType::Orthogonal
    assert_equal(vt.to_s, "Orthogonal")

    vt = RBA::VariantType::Magnification
    assert_equal(vt.to_s, "Magnification")

    vt = RBA::VariantType::XYAnisotropyAndMagnification
    assert_equal(vt.to_s, "XYAnisotropyAndMagnification")

    vt = RBA::VariantType::MagnificationAndOrientation
    assert_equal(vt.to_s, "MagnificationAndOrientation")

  end

end

load("test_epilogue.rb")

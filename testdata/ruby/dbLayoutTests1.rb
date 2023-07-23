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

class DBLayoutTests1_TestClass < TestBase

  def dump_layer(l, layer, cell_name)
    s = []
    cell_index = l.cell_by_name(cell_name)
    iter = l.begin_shapes(cell_index, layer)
    while !iter.at_end
      poly = iter.shape.polygon.transformed(iter.trans)
      s.push(poly.is_box? ? poly.bbox.to_s : poly.to_s)
      iter.next
    end
    return s.join("; ")
  end

  def dump_layer_i(l, layer, cell_index)
    s = []
    iter = l.begin_shapes(cell_index, layer)
    while !iter.at_end
      poly = iter.shape.polygon.transformed(iter.trans)
      s.push(poly.is_box? ? poly.bbox.to_s : poly.to_s)
      iter.next
    end
    return s.join("; ")
  end

  def test_1

    # LayerInfo tests
    l1 = RBA::LayerInfo::new
    assert_equal(l1.to_s, "")
    assert_equal(l1.anonymous?, true)
    assert_equal(l1.is_named?, false)
    l2 = RBA::LayerInfo::new(1, 100)
    assert_equal(l2.to_s, "1/100")
    assert_equal(l2.anonymous?, false)
    assert_equal(l2.is_named?, false)
    l3 = RBA::LayerInfo::new("aber")
    assert_equal(l3.to_s, "aber")
    assert_equal(l3.anonymous?, false)
    assert_equal(l3.is_named?, true)
    assert_equal(l3.is_equivalent?(l2), false)
    assert_equal(l2.is_equivalent?(l3), false)
    l4 = RBA::LayerInfo::new(1, 100, "aber")
    assert_equal(l4.to_s, "aber (1/100)")
    assert_equal(l4.anonymous?, false)
    assert_equal(l4.is_named?, false)
    assert_equal(l4.is_equivalent?(l2), true)
    assert_equal(l2.is_equivalent?(l4), true)
    assert_equal(l4.is_equivalent?(l3), true)
    assert_equal(l3.is_equivalent?(l4), true)
    assert_equal(l4.is_equivalent?(RBA::LayerInfo::new(1, 100, "xyz")), true)
    assert_equal(l4.is_equivalent?(RBA::LayerInfo::new(1, 101, "aber")), false)

    l1.assign(l4)

    assert_equal(l1.to_s, "aber (1/100)")
    assert_equal(l1.is_named?, false)
    assert_equal(l1.is_equivalent?(l4), true)
    assert_equal(l1 == l4, true)

    l1.layer = -1
    l1.datatype = -1
    assert_equal(l1.is_named?, true)
    assert_equal(l1.to_s, "aber")

    l1.name = "xyz"
    assert_equal(l1.is_named?, true)
    assert_equal(l1.to_s, "xyz")

    l1.layer = 100
    l1.datatype = 0
    assert_equal(l1.is_named?, false)
    assert_equal(l1.to_s, "xyz (100/0)")

  end

  def test_2

    ly = RBA::Layout.new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t10.gds")

    def ll(ly)
      s = []
      (0..(ly.layers-1)).each do |l|
        if ly.is_valid_layer?(l)
          s.push(ly.get_info(l).to_s)
        end
      end
      s.join(",")
    end

    assert_equal(ll(ly), "2/0,4/0,6/0,3/0,8/0,1/0,5/0,7/0,3/1,6/1,8/1")

    opt = RBA::LoadLayoutOptions.new
    assert_equal(opt.is_text_enabled?, true)
    assert_equal(opt.is_properties_enabled?, true)
    ly = RBA::Layout.new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t10.gds", opt)
    assert_equal(ll(ly), "2/0,4/0,6/0,3/0,8/0,1/0,5/0,7/0,3/1,6/1,8/1")

    opt = RBA::LoadLayoutOptions.new
    opt.text_enabled = false
    assert_equal(opt.is_text_enabled?, false)
    opt.properties_enabled = false
    assert_equal(opt.is_properties_enabled?, false)
    ly = RBA::Layout.new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t10.gds", opt)
    assert_equal(ll(ly), "2/0,4/0,6/0,3/0,8/0,1/0,5/0,7/0")

    lm = RBA::LayerMap.new
    opt.set_layer_map(lm, false)
    ly = RBA::Layout.new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t10.gds", opt)
    assert_equal(ll(ly), "")

    opt.select_all_layers
    ly = RBA::Layout.new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t10.gds", opt)
    assert_equal(ll(ly), "2/0,4/0,6/0,3/0,8/0,1/0,5/0,7/0")

    assert_equal(ly.layer_indexes.collect { |li| li.to_s }.join(","), "0,1,2,3,4,5,6,7")
    assert_equal(ly.layer_infos.collect { |li| li.to_s }.join(","), "2/0,4/0,6/0,3/0,8/0,1/0,5/0,7/0")

    ly = RBA::Layout.new
    li = RBA::LayerInfo.new(3, 0)
    lm.map(li, ly.insert_layer(li))
    opt.set_layer_map(lm, true)
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t10.gds", opt)
    assert_equal(ll(ly), "3/0,2/0,4/0,6/0,8/0,1/0,5/0,7/0")

    ly = RBA::Layout.new
    opt.set_layer_map(lm, false)
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t10.gds", opt)
    assert_equal(ll(ly), "3/0")

    ly = RBA::Layout.new
    a = ly.layer(RBA::LayerInfo.new(3, 0))
    assert_equal(ll(ly), "3/0")
    b = ly.layer(RBA::LayerInfo.new(3, 0))
    assert_equal(a, b)
    assert_equal(ll(ly), "3/0")
    bb = b
    b = ly.layer
    assert_equal(ll(ly), "3/0,")
    assert_equal(b != bb, true)
    bb = b
    b = ly.layer(RBA::LayerInfo.new)
    assert_equal(ll(ly), "3/0,,")
    assert_equal(b != bb, true)
    n = ly.layer(RBA::LayerInfo.new ("hallo"));
    assert_equal(ll(ly), "3/0,,,hallo")
    assert_equal(b != n, true)

    ly = RBA::Layout.new
    li = ly.layer(2, 0)
    a = ly.find_layer(3, 0)
    assert_equal(a, nil)
    a = ly.find_layer(2, 0)
    assert_equal(a, li)
    a = ly.find_layer(RBA::LayerInfo.new(3, 0))
    assert_equal(a, nil)
    a = ly.find_layer(RBA::LayerInfo.new(2, 0))
    assert_equal(a, li)
    a = ly.find_layer(3, 0, "hallo")
    assert_equal(a, nil)
    li2 = ly.layer("hallo")
    a = ly.find_layer("hillo")
    assert_equal(a, nil)
    a = ly.find_layer("hallo")
    assert_equal(a, li2)
    a = ly.find_layer(3, 0, "hallo")
    assert_equal(a, li2)
    a = ly.find_layer(2, 0, "hallo")
    assert_equal(a, li)

    ly = RBA::Layout.new
    li = ly.layer(2, 0, "hallo")
    a = ly.find_layer(3, 0)
    assert_equal(a, nil)
    a = ly.find_layer(2, 0)
    assert_equal(a, li)
    a = ly.find_layer("hallo")
    assert_equal(a, li)
    a = ly.find_layer(2, 0, "hillo")
    assert_equal(a, li)
    a = ly.find_layer(1, 0, "hallo")
    assert_equal(a, nil)

    ly = RBA::Layout.new
    li0 = ly.layer("hello")
    li = ly.layer("hallo")
    a = ly.find_layer(3, 0)
    assert_equal(a, nil)
    a = ly.find_layer(2, 0)
    assert_equal(a, nil)
    a = ly.find_layer("hallo")
    assert_equal(a, li)
    a = ly.find_layer(2, 0, "hillo")
    assert_equal(a, nil)
    a = ly.find_layer(1, 0, "hallo")
    assert_equal(a, li)
    li2 = ly.layer(1, 0, "hello")
    a = ly.find_layer(1, 0, "hello")
    assert_equal(a, li2)
    a = ly.find_layer("hello")
    assert_equal(a, li0)

  end

  def collect(s, l)

    res = []
    while !s.at_end?
      r = "[#{l.cell_name(s.cell_index)}]"
      if s.shape.is_box?
        box = s.shape.box
        r += box.transformed(s.trans).to_s
      else 
        r += "X";
      end
      s.next
      res.push(r)
    end

    return res.join("/")

  end

  def dcollect(s, l)

    res = []
    while !s.at_end?
      r = "[#{l.cell_name(s.cell_index)}]"
      if s.shape.is_box?
        box = s.shape.dbox
        r += box.transformed(s.dtrans).to_s
      else 
        r += "X";
      end
      s.next
      res.push(r)
    end

    return res.join("/")

  end

  def collect_hier(l)

    s = ""

    l.each_cell do |ci|
      
      if s != ""
        s += "/"
      end

      s += "[" + l.cell_name(ci.cell_index) + "]"

      c = []
      ci.each_parent_cell do |parent|
        c.push(l.cell_name(parent))
      end
      s += "(P=" + c.join(",") + ")"
      
      c = []
      ci.each_child_cell do |child|
        c.push(l.cell_name(child))
      end
      s += "(C=" + c.join(",") + ")"

    end

    return s

  end

  def test_5a

    # delete_cell
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))
    assert_equal(c0.is_empty?, true)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))
    assert_equal(c0.is_empty?, false)

    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    c0_index = c0.cell_index
    assert_equal(l.is_valid_cell_index?(c0_index), true)
    l.delete_cell(c0.cell_index)
    assert_equal(l.is_valid_cell_index?(c0_index), false)

    assert_equal(collect_hier(l), "[c1](P=)(C=)/[c2](P=)(C=c3)/[c3](P=c2)(C=)");
    assert_equal(c3.is_empty?, true)

    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))
    assert_equal(c0.is_empty?, true)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))
    assert_equal(c0.is_empty?, false)

    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    c0_index = c0.cell_index
    assert_equal(l.is_valid_cell_index?(c0_index), true)
    l.cell(c0_index).delete
    assert_equal(l.is_valid_cell_index?(c0_index), false)

    assert_equal(collect_hier(l), "[c1](P=)(C=)/[c2](P=)(C=c3)/[c3](P=c2)(C=)");
    assert_equal(c3.is_empty?, true)

  end

  def test_5b

    # delete_cells
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    assert_equal(c0.is_empty?, false)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    c0_index = c0.cell_index
    c2_index = c2.cell_index

    ll = l.dup

    l.delete_cells([c0_index, c2_index])
    assert_equal(collect_hier(l), "[c1](P=)(C=)/[c3](P=)(C=)");

    l = ll
    # Hint: even though we deleted c0 and c2, their indices are still valid
    l.delete_cells([c2_index, c0_index])
    assert_equal(collect_hier(l), "[c1](P=)(C=)/[c3](P=)(C=)");

  end

  def test_5d

    # prune_cell
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    c0_index = c0.cell_index
    c2_index = c2.cell_index

    ll = l.dup

    l.prune_cell(c0_index, -1)
    assert_equal(collect_hier(l), "");

    l = ll
    ll = l.dup
    # Hint: even though we deleted c0 and c2, their indices are still valid
    l.prune_cell(c2_index, -1)
    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c3)/[c1](P=c0)(C=)/[c3](P=c0)(C=)");

    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    c0_index = c0.cell_index
    c2_index = c2.cell_index

    ll = l.dup

    l.cell(c0_index).prune_cell
    assert_equal(collect_hier(l), "");

    l = ll
    ll = l.dup
    # Hint: even though we deleted c0 and c2, their indices are still valid
    l.cell(c2_index).prune_cell(-1)
    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c3)/[c1](P=c0)(C=)/[c3](P=c0)(C=)");

  end

  def test_5e

    # delete_cell_rec
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    c0_index = c0.cell_index
    c2_index = c2.cell_index

    ll = l.dup

    l.delete_cell_rec(c0_index)
    assert_equal(collect_hier(l), "");

    l = ll
    ll = l.dup
    # Hint: even though we deleted c0 and c2, their indices are still valid
    l.delete_cell_rec(c2_index)
    assert_equal(collect_hier(l), "[c0](P=)(C=c1)/[c1](P=c0)(C=)");

  end

  def test_5f

    # delete_cell_rec
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    c0_index = c0.cell_index
    c2_index = c2.cell_index

    ii = l.begin_shapes(c0_index, 0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)");

    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    ll = l.dup

    l.flatten(c0_index, -1, false)
    assert_equal(collect_hier(l), "[c0](P=)(C=)/[c1](P=)(C=)/[c2](P=)(C=c3)/[c3](P=c2)(C=)");

    ii = l.begin_shapes(c0_index, 0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](1200,0;2200,1100)/[c0](-1200,0;-100,1000)");

    l = ll
    ll = l.dup
    l.flatten(c0_index, -1, true)
    assert_equal(collect_hier(l), "[c0](P=)(C=)");

    ii = l.begin_shapes(c0_index, 0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](1200,0;2200,1100)/[c0](-1200,0;-100,1000)");

    l = ll
    ll = l.dup
    l.flatten(c0_index, 0, false)
    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    ii = l.begin_shapes(c0_index, 0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)");

    l = ll
    ll = l.dup
    l.flatten(c0_index, 1, false)
    assert_equal(collect_hier(l), "[c0](P=)(C=c3)/[c1](P=)(C=)/[c2](P=)(C=c3)/[c3](P=c0,c2)(C=)");

    ii = l.begin_shapes(c0_index, 0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](-1200,0;-100,1000)/[c3](1200,0;2200,1100)");

    l = ll
    ll = l.dup
    l.flatten(c0_index, 1, true)
    assert_equal(collect_hier(l), "[c0](P=)(C=c3)/[c3](P=c0)(C=)");

    l = ll
    ll = l.dup

    l.cell(c0_index).flatten(false)
    assert_equal(collect_hier(l), "[c0](P=)(C=)/[c1](P=)(C=)/[c2](P=)(C=c3)/[c3](P=c2)(C=)");

    ii = l.begin_shapes(c0_index, 0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](1200,0;2200,1100)/[c0](-1200,0;-100,1000)");

    l = ll
    ll = l.dup
    l.cell(c0_index).flatten(true)
    assert_equal(collect_hier(l), "[c0](P=)(C=)");

    ii = l.begin_shapes(c0_index, 0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](1200,0;2200,1100)/[c0](-1200,0;-100,1000)");

    l = ll
    ll = l.dup
    l.cell(c0_index).flatten(0, false)
    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    ii = l.begin_shapes(c0_index, 0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)");

    l = ll
    ll = l.dup
    l.cell(c0_index).flatten(1, false)
    assert_equal(collect_hier(l), "[c0](P=)(C=c3)/[c1](P=)(C=)/[c2](P=)(C=c3)/[c3](P=c0,c2)(C=)");

    ii = l.begin_shapes(c0_index, 0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](-1200,0;-100,1000)/[c3](1200,0;2200,1100)");

    l = ll
    ll = l.dup
    l.cell(c0_index).flatten(1, true)
    assert_equal(collect_hier(l), "[c0](P=)(C=c3)/[c3](P=c0)(C=)");

    ii = l.cell(c0_index).begin_shapes_rec(0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](-1200,0;-100,1000)/[c3](1200,0;2200,1100)");

    ii = l.cell(c0_index).begin_shapes_rec(0);
    assert_equal(collect(ii, l), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](-1200,0;-100,1000)/[c3](1200,0;2200,1100)");

  end

  def test_5g

    # prune_subcells
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    c0_index = c0.cell_index
    c2_index = c2.cell_index

    ll = l.dup

    l.prune_subcells(c0_index, -1)
    assert_equal(collect_hier(l), "[c0](P=)(C=)");

    l = ll
    ll = l.dup
    # Hint: even though we deleted c0 and c2, their indices are still valid
    l.prune_subcells(c2_index, -1)
    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=)/[c3](P=c0)(C=)");

    l = ll
    # Hint: even though we deleted c0 and c2, their indices are still valid
    l.prune_subcells(c0_index, 1)
    assert_equal(collect_hier(l), "[c0](P=)(C=)");

    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=c3)/[c3](P=c0,c2)(C=)");

    c0_index = c0.cell_index
    c2_index = c2.cell_index

    ll = l.dup

    l.cell(c0_index).prune_subcells
    assert_equal(collect_hier(l), "[c0](P=)(C=)");

    l = ll
    ll = l.dup
    # Hint: even though we deleted c0 and c2, their indices are still valid
    l.cell(c2_index).prune_subcells
    assert_equal(collect_hier(l), "[c0](P=)(C=c1,c2,c3)/[c1](P=c0)(C=)/[c2](P=c0)(C=)/[c3](P=c0)(C=)");

    l = ll
    # Hint: even though we deleted c0 and c2, their indices are still valid
    l.cell(c0_index).prune_subcells(1)
    assert_equal(collect_hier(l), "[c0](P=)(C=)");

  end

  def test_6

    m = RBA::Manager.new 
    l = RBA::Layout.new(m)
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(1, 0))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "")

    m.transaction("1");
    l.swap_layers(0, 1)
    m.commit

    assert_equal(dump_layer(l, 0, "c0"), "")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")

    m.transaction("2");
    l.copy_layer(1, 0)
    m.commit

    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")

    m.transaction("3");
    l.clear_layer(1)
    m.commit

    assert_equal(dump_layer(l, 1, "c0"), "")
    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")

    m.transaction("4");
    l.move_layer(0, 1)
    m.commit

    assert_equal(dump_layer(l, 0, "c0"), "")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")

    m.transaction("5");
    c3.swap(0, 1)
    m.commit

    assert_equal(dump_layer(l, 0, "c0"), "(1200,0;2200,1100); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100)")

    m.transaction("6");
    c0.copy(1, 0)
    m.commit

    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (1200,0;2200,1100); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100)")

    m.transaction("7");
    c0.move(1, 0)
    m.commit

    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (1200,0;2200,1100); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (100,0;1100,1100)")

    m.transaction("8");
    c3.copy(0, 0)
    m.commit

    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (1200,0;2200,1100); (1200,0;2200,1100); (-1200,0;-100,1000); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (100,0;1100,1100)")

    ll = l.dup

    lll = RBA::Layout.new
    lll.assign(l)

    m.transaction("9");
    l.copy_layer(1, 1)
    m.commit

    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (1200,0;2200,1100); (1200,0;2200,1100); (-1200,0;-100,1000); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (100,0;1100,1100)")

    assert_equal(dump_layer(ll, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (1200,0;2200,1100); (1200,0;2200,1100); (-1200,0;-100,1000); (-1200,0;-100,1000)")
    assert_equal(dump_layer(ll, 1, "c0"), "(0,100;1000,1200); (100,0;1100,1100)")

    assert_equal(dump_layer(lll, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (1200,0;2200,1100); (1200,0;2200,1100); (-1200,0;-100,1000); (-1200,0;-100,1000)")
    assert_equal(dump_layer(lll, 1, "c0"), "(0,100;1000,1200); (100,0;1100,1100)")

    assert_equal(m.has_undo?, true)
    assert_equal(m.has_redo?, false)
    assert_equal(m.transaction_for_undo, "9")

    m.undo
    assert_equal(m.has_undo?, true)
    assert_equal(m.has_redo?, true)
    assert_equal(m.transaction_for_undo, "8")

    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (1200,0;2200,1100); (1200,0;2200,1100); (-1200,0;-100,1000); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (100,0;1100,1100)")

    m.undo
    assert_equal(m.has_undo?, true)
    assert_equal(m.has_redo?, true)
    assert_equal(m.transaction_for_undo, "7")

    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (1200,0;2200,1100); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (100,0;1100,1100)")

    m.undo
    assert_equal(m.has_undo?, true)
    assert_equal(m.has_redo?, true)
    assert_equal(m.transaction_for_undo, "6")

    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (1200,0;2200,1100); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100)")

    m.undo
    assert_equal(m.has_undo?, true)
    assert_equal(m.has_redo?, true)
    assert_equal(m.transaction_for_undo, "5")

    assert_equal(dump_layer(l, 0, "c0"), "(1200,0;2200,1100); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100)")

    m.undo
    assert_equal(m.has_undo?, true)
    assert_equal(m.has_redo?, true)
    assert_equal(m.transaction_for_undo, "4")

    assert_equal(dump_layer(l, 0, "c0"), "")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")

    m.undo
    assert_equal(m.has_undo?, true)
    assert_equal(m.has_redo?, true)
    assert_equal(m.transaction_for_undo, "3")

    assert_equal(dump_layer(l, 1, "c0"), "")
    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")

    m.undo
    assert_equal(m.has_undo?, true)
    assert_equal(m.has_redo?, true)
    assert_equal(m.transaction_for_undo, "2")

    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")

    m.undo
    assert_equal(m.has_undo?, true)
    assert_equal(m.has_redo?, true)
    assert_equal(m.transaction_for_undo, "1")

    assert_equal(dump_layer(l, 0, "c0"), "")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")

    m.undo
    assert_equal(m.has_undo?, false)
    assert_equal(m.has_redo?, true)
    assert_equal(m.transaction_for_redo, "1")

    assert_equal(dump_layer(l, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")
    assert_equal(dump_layer(l, 1, "c0"), "")

    m.redo
    assert_equal(m.has_undo?, true)
    assert_equal(m.has_redo?, true)
    assert_equal(m.transaction_for_redo, "2")

    assert_equal(dump_layer(l, 0, "c0"), "")
    assert_equal(dump_layer(l, 1, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1100,1100); (1200,0;2200,1100); (-1200,0;-100,1000)")

    assert_equal(dump_layer(ll, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (1200,0;2200,1100); (1200,0;2200,1100); (-1200,0;-100,1000); (-1200,0;-100,1000)")
    assert_equal(dump_layer(ll, 1, "c0"), "(0,100;1000,1200); (100,0;1100,1100)")

    l.destroy
    m.destroy

    assert_equal(dump_layer(lll, 0, "c0"), "(0,100;1000,1200); (0,100;1000,1200); (1200,0;2200,1100); (1200,0;2200,1100); (-1200,0;-100,1000); (-1200,0;-100,1000)")
    assert_equal(dump_layer(lll, 1, "c0"), "(0,100;1000,1200); (100,0;1100,1100)")

  end

  def test_7

    # clip tests
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    bh = (RBA::Region.new(RBA::Box.new(0, 100, 1000, 1200)) - RBA::Region.new(RBA::Box.new(100, 200, 900, 1100)))[0]
    c0.shapes(1).insert(bh)
    c1.shapes(1).insert(bh)
    c2.shapes(1).insert(bh)
    c3.shapes(1).insert(bh)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    ci = l.clip(c0.cell_index, RBA::Box.new(0, 0, 1000, 1200))
    assert_equal(dump_layer_i(l, 0, ci), "(0,100;1000,1200); (0,100;1000,1200); (100,0;1000,1100)")
    assert_equal(dump_layer_i(l, 1, ci), "(0,100;0,1200;1000,1200;1000,100/100,200;900,200;900,1100;100,1100); (0,100;0,1200;1000,1200;1000,100/100,200;900,200;900,1100;100,1100); (100,0;100,1100;1000,1100;1000,1000;200,1000;200,100;1000,100;1000,0)")

    ci = l.clip(c0.cell_index, RBA::Box.new(0, 0, 200, 200))
    assert_equal(dump_layer_i(l, 0, ci), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")
    assert_equal(dump_layer_i(l, 1, ci), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")

    cic = l.clip(c0, RBA::Box.new(0, 0, 200, 200))
    assert_equal(dump_layer_i(l, 0, cic.cell_index), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")

    cid = l.clip(c0.cell_index, RBA::DBox.new(0, 0, 0.2, 0.2))
    assert_equal(dump_layer_i(l, 0, cid), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")

    cicd = l.clip(c0, RBA::DBox.new(0, 0, 0.2, 0.2))
    assert_equal(dump_layer_i(l, 0, cicd.cell_index), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")

    ci = l.multi_clip(c0.cell_index, [RBA::Box.new(0, 0, 200, 200),RBA::Box.new(1000, 0, 1300, 200)])
    assert_equal(dump_layer_i(l, 0, ci[0]), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")
    assert_equal(dump_layer_i(l, 0, ci[1]), "(1000,0;1100,200); (1200,0;1300,200)")
    assert_equal(dump_layer_i(l, 1, ci[0]), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")
    assert_equal(dump_layer_i(l, 1, ci[1]), "(1000,0;1100,200); (1200,0;1300,200)")

    cic = l.multi_clip(c0, [RBA::Box.new(0, 0, 200, 200),RBA::Box.new(1000, 0, 1300, 200)])
    assert_equal(dump_layer_i(l, 0, cic[0].cell_index), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")
    assert_equal(dump_layer_i(l, 0, cic[1].cell_index), "(1000,0;1100,200); (1200,0;1300,200)")

    cid = l.multi_clip(c0.cell_index, [RBA::DBox.new(0, 0, 0.2, 0.2),RBA::DBox.new(1.0, 0, 1.3, 0.2)])
    assert_equal(dump_layer_i(l, 0, cid[0]), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")
    assert_equal(dump_layer_i(l, 0, cid[1]), "(1000,0;1100,200); (1200,0;1300,200)")

    cidc = l.multi_clip(c0, [RBA::DBox.new(0, 0, 0.2, 0.2),RBA::DBox.new(1.0, 0, 1.3, 0.2)])
    assert_equal(dump_layer_i(l, 0, cidc[0].cell_index), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")
    assert_equal(dump_layer_i(l, 0, cidc[1].cell_index), "(1000,0;1100,200); (1200,0;1300,200)")

    ll = RBA::Layout.new
    ll.dbu = l.dbu
    ll.insert_layer_at(0, RBA::LayerInfo.new(2, 0))

    ci = l.clip_into(c0.cell_index, ll, RBA::Box.new(0, 0, 200, 200))
    assert_equal(dump_layer_i(ll, 0, ci), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")

    cic = l.clip_into(c0, ll, RBA::Box.new(0, 0, 200, 200))
    assert_equal(dump_layer_i(ll, 0, cic.cell_index), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")

    cid = l.clip_into(c0.cell_index, ll, RBA::DBox.new(0, 0, 0.2, 0.2))
    assert_equal(dump_layer_i(ll, 0, cid), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")

    cicd = l.clip_into(c0, ll, RBA::DBox.new(0, 0, 0.2, 0.2))
    assert_equal(dump_layer_i(ll, 0, cicd.cell_index), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")

    ci = l.multi_clip_into(c0.cell_index, ll, [RBA::Box.new(0, 0, 200, 200),RBA::Box.new(1000, 0, 1300, 200)])
    assert_equal(dump_layer_i(ll, 0, ci[0]), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")
    assert_equal(dump_layer_i(ll, 0, ci[1]), "(1000,0;1100,200); (1200,0;1300,200)")

    cic = l.multi_clip_into(c0, ll, [RBA::Box.new(0, 0, 200, 200),RBA::Box.new(1000, 0, 1300, 200)])
    assert_equal(dump_layer_i(ll, 0, cic[0]), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")
    assert_equal(dump_layer_i(ll, 0, cic[1]), "(1000,0;1100,200); (1200,0;1300,200)")

    cid = l.multi_clip_into(c0.cell_index, ll, [RBA::DBox.new(0, 0, 0.2, 0.2),RBA::DBox.new(1.0, 0, 1.3, 0.2)])
    assert_equal(dump_layer_i(ll, 0, cid[0]), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")
    assert_equal(dump_layer_i(ll, 0, cid[1]), "(1000,0;1100,200); (1200,0;1300,200)")

    cicd = l.multi_clip_into(c0, ll, [RBA::DBox.new(0, 0, 0.2, 0.2),RBA::DBox.new(1.0, 0, 1.3, 0.2)])
    assert_equal(dump_layer_i(ll, 0, cicd[0]), "(0,100;200,200); (0,100;200,200); (100,0;200,200)")
    assert_equal(dump_layer_i(ll, 0, cicd[1]), "(1000,0;1100,200); (1200,0;1300,200)")

  end

  def test_8

    # copy shapes between cells
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    b = RBA::Box.new(1, 101, 1001, 1201)
    s = c0.shapes(1).insert(b)
    s.set_property("p", 17)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")

    c0c = l.cell(l.add_cell("c0"))
    c0c.copy_shapes(c0)
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](0,100;1000,1200)")
    assert_equal(c0c.begin_shapes_rec(0).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "[c0$1](1,101;1001,1201)")
    assert_equal(c0c.begin_shapes_rec(1).shape.property("p"), 17)
     
    c0c.clear
    lm = RBA::LayerMapping::new
    lm.map(1, 0)
    c0c.copy_shapes(c0, lm)
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](1,101;1001,1201)")
    assert_equal(c0c.begin_shapes_rec(0).shape.property("p"), 17)
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "")
    assert_equal(c0c.begin_shapes_rec(1).shape.property("p"), nil)

    l2 = RBA::Layout::new
    l2.dbu = 0.0005
    c0c = l2.cell(l2.add_cell("c0"))

    c0c.copy_shapes(c0)
    layer1 = l2.find_layer(1, 0)
    layer2 = l2.find_layer(2, 0)
    assert_equal(collect(c0c.begin_shapes_rec(layer1), l), "[c0](0,200;2000,2400)")
    assert_equal(c0c.begin_shapes_rec(layer1).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(layer2), l), "[c0](2,202;2002,2402)")
    assert_equal(c0c.begin_shapes_rec(layer2).shape.property("p"), 17)

    l2 = RBA::Layout::new
    l2.dbu = 0.0005
    c0c = l2.cell(l2.add_cell("c0"))

    lm = RBA::LayerMapping::new
    lm.create(l2, l)
    layer1 = l2.find_layer(1, 0)
    assert_equal(layer1, nil)
    layer2 = l2.find_layer(2, 0)
    assert_equal(layer2, nil)

    lm.create_full(l2, l)

    c0c.copy_shapes(c0, lm)
    layer1 = l2.find_layer(1, 0)
    layer2 = l2.find_layer(2, 0)
    assert_equal(collect(c0c.begin_shapes_rec(layer1), l), "[c0](0,200;2000,2400)")
    assert_equal(c0c.begin_shapes_rec(layer1).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(layer2), l), "[c0](2,202;2002,2402)")
    assert_equal(c0c.begin_shapes_rec(layer2).shape.property("p"), 17)

  end

  def test_9

    # move shapes between cells
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    b = RBA::Box.new(1, 101, 1001, 1201)
    s = c0.shapes(1).insert(b)
    s.set_property("p", 17)

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    ll = l.dup

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")

    c0c = l.cell(l.add_cell("c0"))
    c0c.move_shapes(c0)
    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "")
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](0,100;1000,1200)")
    assert_equal(c0c.begin_shapes_rec(0).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "[c0$1](1,101;1001,1201)")
    assert_equal(c0c.begin_shapes_rec(1).shape.property("p"), 17)
    c0.move_shapes(c0c)
    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "")
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "")
     
    lm = RBA::LayerMapping::new
    lm.map(1, 0)
    c0c.move_shapes(c0, lm)
    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "")
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](1,101;1001,1201)")
    assert_equal(c0c.begin_shapes_rec(0).shape.property("p"), 17)
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "")
    assert_equal(c0c.begin_shapes_rec(1).shape.property("p"), nil)

    l = ll.dup
    c0 = l.cell("c0")

    l2 = RBA::Layout::new
    l2.dbu = 0.0005
    c0c = l2.cell(l2.add_cell("c0"))

    c0c.move_shapes(c0)
    layer1 = l2.find_layer(1, 0)
    layer2 = l2.find_layer(2, 0)
    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "")
    assert_equal(collect(c0c.begin_shapes_rec(layer1), l), "[c0](0,200;2000,2400)")
    assert_equal(c0c.begin_shapes_rec(layer1).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(layer2), l), "[c0](2,202;2002,2402)")
    assert_equal(c0c.begin_shapes_rec(layer2).shape.property("p"), 17)

    l = ll.dup
    c0 = l.cell("c0")

    l2 = RBA::Layout::new
    l2.dbu = 0.0005
    c0c = l2.cell(l2.add_cell("c0"))

    lm = RBA::LayerMapping::new
    lm.create(l2, l)
    layer1 = l2.find_layer(1, 0)
    assert_equal(layer1, nil)
    layer2 = l2.find_layer(2, 0)
    assert_equal(layer2, nil)

    lm.create_full(l2, l)

    c0c.move_shapes(c0, lm)
    layer1 = l2.find_layer(1, 0)
    layer2 = l2.find_layer(2, 0)
    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "")
    assert_equal(collect(c0c.begin_shapes_rec(layer1), l), "[c0](0,200;2000,2400)")
    assert_equal(c0c.begin_shapes_rec(layer1).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(layer2), l), "[c0](2,202;2002,2402)")
    assert_equal(c0c.begin_shapes_rec(layer2).shape.property("p"), 17)

  end

  def test_10

    # copy instances between cells
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    b = RBA::Box.new(1, 101, 1001, 1201)
    s = c0.shapes(1).insert(b)
    s.set_property("p", 17)

    tt = RBA::Trans.new
    i0 = c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    i0.set_property("p", 17)
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")

    c0c = l.cell(l.add_cell("c0"))
    c0c.copy_instances(c0)
    i0 = nil
    c0c.each_inst { |i| i.cell_index == c1.cell_index && i0 = i }
    assert_equal(i0.property("p"), 17)
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "")

    l2 = RBA::Layout::new
    l2.dbu = 0.0005
    c0c = l2.cell(l2.add_cell("c0"))
    err = false
    begin
      c0c.copy_instances(c0)
    rescue => ex
      err = true
    end
    assert_equal(err, true)

  end

  def test_11

    # move instances between cells
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    b = RBA::Box.new(1, 101, 1001, 1201)
    s = c0.shapes(1).insert(b)
    s.set_property("p", 17)

    tt = RBA::Trans.new
    i0 = c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    i0.set_property("p", 17)
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")

    c0c = l.cell(l.add_cell("c0"))
    c0c.move_instances(c0)
    i0 = nil
    c0c.each_inst { |i| i.cell_index == c1.cell_index && i0 = i }
    assert_equal(i0.property("p"), 17)
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "")
    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")

    l2 = RBA::Layout::new
    l2.dbu = 0.0005
    c0c = l2.cell(l2.add_cell("c0"))
    err = false
    begin
      c0c.copy_instances(c0)
    rescue => ex
      err = true
    end
    assert_equal(err, true)

  end

  def test_12

    # copy cell tree
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    s = c1.shapes(0).insert(b)
    s.set_property("p", 17)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    b = RBA::Box.new(1, 101, 1001, 1201)
    c0.shapes(1).insert(b)

    tt = RBA::Trans.new
    i0 = c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    i0.set_property("p", 18)
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")

    c0c = l.cell(l.add_cell("c0"))
    c0c.copy_tree(c0)

    i0 = nil
    c0c.each_inst { |i| i.cell_index == l.cell("c1$1").cell_index && i0 = i }
    assert_equal(i0.property("p"), 18)
    assert_equal(l.cell("c1$1").begin_shapes_rec(0).shape.property("p"), 17)

    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](0,100;1000,1200)/[c2$1](100,0;1100,1100)/[c3$1](1200,0;2200,1100)/[c3$1](-1200,0;-100,1000)/[c1$1](0,100;1000,1200)")
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "[c0$1](1,101;1001,1201)")

    l2 = RBA::Layout::new
    l2.dbu = 0.0005
    c0c = l2.cell(l2.add_cell("c0"))
    c0c.copy_tree(c0)

    i0 = nil
    c0c.each_inst { |i| i.cell_index == l2.cell("c1").cell_index && i0 = i }
    assert_equal(i0.property("p"), 18)

    layer1 = l2.find_layer(1, 0)
    layer2 = l2.find_layer(2, 0)
    assert_equal(l2.cell("c1").begin_shapes_rec(layer1).shape.property("p"), 17)
    assert_equal(collect(c0c.begin_shapes_rec(layer1), l2), "[c0](0,200;2000,2400)/[c2](200,0;2200,2200)/[c3](2400,0;4400,2200)/[c3](-2400,0;-200,2000)/[c1](0,200;2000,2400)")
    assert_equal(collect(c0c.begin_shapes_rec(layer2), l2), "[c0](2,202;2002,2402)")

  end

  def test_13

    # move cell tree
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    s = c1.shapes(0).insert(b)
    s.set_property("p", 17)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    b = RBA::Box.new(1, 101, 1001, 1201)
    c0.shapes(1).insert(b)

    tt = RBA::Trans.new
    i0 = c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    i0.set_property("p", 18)
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    ll = l.dup

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")

    c0c = l.cell(l.add_cell("c0"))
    c0c.move_tree(c0)
    assert_equal(l.has_cell?("c0"), true)
    assert_equal(l.has_cell?("c1"), false)
    assert_equal(l.has_cell?("c2"), false)
    assert_equal(l.has_cell?("c3"), false)

    i0 = nil
    c0c.each_inst { |i| i.cell_index == l.cell("c1$1").cell_index && i0 = i }
    assert_equal(i0.property("p"), 18)
    assert_equal(l.cell("c1$1").begin_shapes_rec(0).shape.property("p"), 17)

    assert_equal(collect(c0.begin_shapes_rec(0), l), "")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "")
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](0,100;1000,1200)/[c2$1](100,0;1100,1100)/[c3$1](1200,0;2200,1100)/[c3$1](-1200,0;-100,1000)/[c1$1](0,100;1000,1200)")
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "[c0$1](1,101;1001,1201)")

    l = ll.dup
    c0 = l.cell("c0")

    l2 = RBA::Layout::new
    l2.dbu = 0.0005
    c0c = l2.cell(l2.add_cell("c0"))
    c0c.move_tree(c0)

    assert_equal(collect(c0.begin_shapes_rec(0), l), "")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "")
    assert_equal(l.has_cell?("c0"), true)
    assert_equal(l.has_cell?("c1"), false)
    assert_equal(l.has_cell?("c2"), false)
    assert_equal(l.has_cell?("c3"), false)

    i0 = nil
    c0c.each_inst { |i| i.cell_index == l2.cell("c1").cell_index && i0 = i }
    assert_equal(i0.property("p"), 18)

    layer1 = l2.find_layer(1, 0)
    layer2 = l2.find_layer(2, 0)
    assert_equal(l2.cell("c1").begin_shapes_rec(layer1).shape.property("p"), 17)
    assert_equal(collect(c0c.begin_shapes_rec(layer1), l2), "[c0](0,200;2000,2400)/[c2](200,0;2200,2200)/[c3](2400,0;4400,2200)/[c3](-2400,0;-200,2000)/[c1](0,200;2000,2400)")
    assert_equal(collect(c0c.begin_shapes_rec(layer2), l2), "[c0](2,202;2002,2402)")

  end

  def test_14

    # copy shapes between cell trees
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    b = RBA::Box.new(1, 101, 1001, 1201)
    s = c0.shapes(1).insert(b)
    s.set_property("p", 17)

    tt = RBA::Trans.new
    s = c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    s.set_property("p", 18)
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")

    c0c = l.cell(l.add_cell("c0"))
    cm = RBA::CellMapping::new
    cm.for_single_cell(l, c0c.cell_index, l, c0.cell_index)
    c0c.copy_tree_shapes(c0, cm)
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](0,100;1000,1200)/[c0$1](0,100;1000,1200)/[c0$1](100,0;1100,1100)/[c0$1](-1200,0;-100,1000)/[c0$1](1200,0;2200,1100)")
    assert_equal(c0c.begin_shapes_rec(0).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "[c0$1](1,101;1001,1201)")
    assert_equal(c0c.begin_shapes_rec(1).shape.property("p"), 17)
     
    c0c.clear
    lm = RBA::LayerMapping::new
    lm.map(1, 0)
    c0c.copy_tree_shapes(c0, cm, lm)
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](1,101;1001,1201)")
    assert_equal(c0c.begin_shapes_rec(0).shape.property("p"), 17)
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "")
    assert_equal(c0c.begin_shapes_rec(1).shape.property("p"), nil)

    c0c.clear
    cm.for_single_cell_full(l, c0c.cell_index, l, c0.cell_index)
    c0c.copy_tree_shapes(c0, cm)
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](0,100;1000,1200)/[c2$1](100,0;1100,1100)/[c3$1](1200,0;2200,1100)/[c3$1](-1200,0;-100,1000)/[c1$1](0,100;1000,1200)")
    assert_equal(c0c.begin_shapes_rec(0).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "[c0$1](1,101;1001,1201)")
    assert_equal(c0c.begin_shapes_rec(1).shape.property("p"), 17)

    i0 = nil
    c0c.each_inst { |i| i.cell_index == l.cell("c1$1").cell_index && i0 = i }
    assert_equal(i0.property("p"), 18)

    l2 = RBA::Layout::new
    l2.dbu = 0.0005
    c0c = l2.cell(l2.add_cell("c0"))

    cm.for_single_cell_full(l2, c0c.cell_index, l, c0.cell_index)
    c0c.copy_tree_shapes(c0, cm)
    layer1 = l2.find_layer(1, 0)
    layer2 = l2.find_layer(2, 0)
    assert_equal(collect(c0c.begin_shapes_rec(layer1), l), "[c0](0,200;2000,2400)/[c2](200,0;2200,2200)/[c3](2400,0;4400,2200)/[c3](-2400,0;-200,2000)/[c1](0,200;2000,2400)")
    assert_equal(c0c.begin_shapes_rec(layer1).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(layer2), l), "[c0](2,202;2002,2402)")
    assert_equal(c0c.begin_shapes_rec(layer2).shape.property("p"), 17)

    i0 = nil
    c0c.each_inst { |i| i.cell_index == l2.cell("c1").cell_index && i0 = i }
    assert_equal(i0.property("p"), 18)

  end

  def test_15

    # move shapes between cell trees
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    b = RBA::Box.new(1, 101, 1001, 1201)
    s = c0.shapes(1).insert(b)
    s.set_property("p", 17)

    tt = RBA::Trans.new
    s = c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    assert_equal(s.is_pcell?, false)
    assert_equal(s.pcell_declaration, nil)
    assert_equal(s.pcell_parameters, [])
    assert_equal(s.pcell_parameters_by_name, {})
    s.set_property("p", 18)
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    ll = l.dup

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")

    c0c = l.cell(l.add_cell("c0"))
    cm = RBA::CellMapping::new
    cm.for_single_cell(l, c0c.cell_index, l, c0.cell_index)
    c0c.move_tree_shapes(c0, cm)
    assert_equal(collect(c0.begin_shapes_rec(0), l), "")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "")
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](0,100;1000,1200)/[c0$1](0,100;1000,1200)/[c0$1](100,0;1100,1100)/[c0$1](-1200,0;-100,1000)/[c0$1](1200,0;2200,1100)")
    assert_equal(c0c.begin_shapes_rec(0).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "[c0$1](1,101;1001,1201)")
    assert_equal(c0c.begin_shapes_rec(1).shape.property("p"), 17)

    l = ll.dup
    c0 = l.cell("c0")
    c0c = l.cell(l.add_cell("c0"))
     
    lm = RBA::LayerMapping::new
    lm.map(1, 0)
    c0c.move_tree_shapes(c0, cm, lm)
    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "")
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](1,101;1001,1201)")
    assert_equal(c0c.begin_shapes_rec(0).shape.property("p"), 17)
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "")
    assert_equal(c0c.begin_shapes_rec(1).shape.property("p"), nil)

    l = ll.dup
    c0 = l.cell("c0")
    c0c = l.cell(l.add_cell("c0"))
     
    cm.for_single_cell_full(l, c0c.cell_index, l, c0.cell_index)
    c0c.move_tree_shapes(c0, cm)
    assert_equal(collect(c0.begin_shapes_rec(0), l), "")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "")
    assert_equal(collect(c0c.begin_shapes_rec(0), l), "[c0$1](0,100;1000,1200)/[c2$1](100,0;1100,1100)/[c3$1](1200,0;2200,1100)/[c3$1](-1200,0;-100,1000)/[c1$1](0,100;1000,1200)")
    assert_equal(c0c.begin_shapes_rec(0).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(1), l), "[c0$1](1,101;1001,1201)")
    assert_equal(c0c.begin_shapes_rec(1).shape.property("p"), 17)

    i0 = nil
    c0c.each_inst { |i| i.cell_index == l.cell("c1$1").cell_index && i0 = i }
    assert_equal(i0.property("p"), 18)

    l = ll.dup
    c0 = l.cell("c0")
     
    l2 = RBA::Layout::new
    l2.dbu = 0.0005
    c0c = l2.cell(l2.add_cell("c0"))

    cm.for_single_cell_full(l2, c0c.cell_index, l, c0.cell_index)
    c0c.move_tree_shapes(c0, cm)
    assert_equal(collect(c0.begin_shapes_rec(0), l), "")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "")
    layer1 = l2.find_layer(1, 0)
    layer2 = l2.find_layer(2, 0)
    assert_equal(collect(c0c.begin_shapes_rec(layer1), l), "[c0](0,200;2000,2400)/[c2](200,0;2200,2200)/[c3](2400,0;4400,2200)/[c3](-2400,0;-200,2000)/[c1](0,200;2000,2400)")
    assert_equal(c0c.begin_shapes_rec(layer1).shape.property("p"), nil)
    assert_equal(collect(c0c.begin_shapes_rec(layer2), l), "[c0](2,202;2002,2402)")
    assert_equal(c0c.begin_shapes_rec(layer2).shape.property("p"), 17)

    i0 = nil
    c0c.each_inst { |i| i.cell_index == l2.cell("c1").cell_index && i0 = i }
    assert_equal(i0.property("p"), 18)

  end

  def test_16

    # fill tool
   
    l = nil
    c0 = nil
    cf = nil
    b = nil

    init = lambda {

      l = RBA::Layout.new
      l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
      c0 = l.cell(l.add_cell("c0"))
      cf = l.cell(l.add_cell("cf"))

      b = RBA::Box.new(0, 0, 100, 200)
      cf.shapes(0).insert(b)

    }

    init.call

    fr = RBA::Region::new
    pts = [ [ 0, 0, ], [ 500, 500 ], [ 500, 0 ] ]
    fr.insert(RBA::Polygon::new(pts.collect { |x,y| RBA::Point::new(x, y) }))

    c0.fill_region(fr, cf.cell_index, b, RBA::Point::new)

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[cf](200,0;300,200)/[cf](300,0;400,200)/[cf](400,0;500,200)/[cf](400,200;500,400)")

    init.call

    fr = RBA::Region::new
    pts = [ [ 0, 0, ], [ 200, 0 ], [ 200, -50 ], [ 400, -50 ], [ 400, 150 ], [ 200, 150 ], [ 200, 400 ], [ 0, 400 ] ]
    fr.insert(RBA::Polygon::new(pts.collect { |x,y| RBA::Point::new(x, y) }))

    c0.fill_region(fr, cf.cell_index, b, nil) 

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[cf](0,150;100,350)/[cf](100,150;200,350)/[cf](200,-50;300,150)/[cf](300,-50;400,150)")

    init.call

    rem = RBA::Region::new
    missed = RBA::Region::new

    c0.fill_region(fr, cf.cell_index, b, nil, rem, RBA::Point::new, missed) 

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[cf](0,150;100,350)/[cf](100,150;200,350)/[cf](200,-50;300,150)/[cf](300,-50;400,150)")
    assert_equal(rem.to_s, "(0,0;0,150;200,150;200,0);(0,350;0,400;200,400;200,350)")
    assert_equal(missed.to_s, "")

    c0.fill_region(rem, cf.cell_index, b, nil, rem, RBA::Point::new, missed) 
    assert_equal(rem.to_s, "")
    assert_equal(missed.to_s, "(0,0;0,150;200,150;200,0);(0,350;0,400;200,400;200,350)")

  end

  def test_17

    # bug #635:
    ly = RBA::Layout::new
    cell = ly.create_cell("TOP")
    child = ly.create_cell("CHILD")

    l1 = ly.layer(1, 0)
    child.shapes(l1).insert(RBA::Box::new(-10000, -10000, 10000, 10000))

    100.times do |j|
      100.times do |i|
        cell.insert(RBA::CellInstArray::new(child.cell_index, RBA::Trans::new(i * 10000, j * 10000)))
      end
    end

    def check(ly, cell)

      cell.each_inst do |inst|
        # dummy - reqired!
      end
      
      bx = RBA::Box::new(0, -100000, 500000, 100000)
      n1 = 0
      cell.each_overlapping_inst(bx) do |inst|
        n1 += 1
      end
      n2 = 0
      cell.each_inst do |inst|
        if inst.bbox.overlaps?(bx)
          n2 += 1
        end  
      end
      
      [ n1, n2 ]
      
    end

    nn = check(ly, cell)
    assert_equal(nn[0], 561)
    assert_equal(nn[1], 561)

    li = ly.layer(101, 0)
    # this is required to reproduce the issue:
    ly.clear_layer(li)

    nn = check(ly, cell)
    assert_equal(nn[0], 561)
    assert_equal(nn[1], 561)

    l1 = ly.layer(1, 0)
    ly.clear_layer(l1)

    nn = check(ly, cell)
    # intentional - each_overlapping_inst delivers more instances (empty ones too)
    # than the brute force check
    assert_equal(nn[0], 490)
    assert_equal(nn[1], 0)

  end

  # Cell user properties
  def test_18

    ly = RBA::Layout::new

    cell = ly.create_cell("X")

    assert_equal(cell.prop_id, 0)
    cell.prop_id = 1
    assert_equal(cell.prop_id, 1)
    cell.prop_id = 0
    assert_equal(cell.prop_id, 0)

    cell.set_property("x", 1)
    assert_equal(cell.prop_id, 1)
    assert_equal(cell.property("x"), 1)
    cell.set_property("x", 17)
    assert_equal(cell.prop_id, 2)
    assert_equal(cell.property("x"), 17)
    assert_equal(cell.property("y"), nil)

    cell.delete_property("x")
    assert_equal(cell.property("x"), nil)

  end

  # LayerInfo hash values 
  def test_19

    l1 = RBA::LayerInfo::new("a")
    l1c = RBA::LayerInfo::new("a")
    l2 = RBA::LayerInfo::new(1, 2, "a")
    l3 = RBA::LayerInfo::new(1, 2)

    assert_equal(l1.eql?(l2), false)
    assert_equal(l1.eql?(l3), false)
    assert_equal(l2.eql?(l3), false)
    assert_equal(l1.eql?(l1c), true)

    assert_equal(l1.hash == l2.hash, false)
    assert_equal(l1.hash == l3.hash, false)
    assert_equal(l2.hash == l3.hash, false)

    h = { l1 => "a", l2 => "b", l3 => "c" }
    assert_equal(h[l1], "a")
    assert_equal(h[l2], "b")
    assert_equal(h[l3], "c")

  end

  def test_20

    # copy shapes between cell trees starting from multiple cells
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c9 = l.cell(l.add_cell("c9"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    b = RBA::Box.new(1, 101, 1001, 1201)
    s = c0.shapes(1).insert(b)
    s.set_property("p", 17)

    tt = RBA::Trans.new
    s = c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    s.set_property("p", 18)
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))
    c9.insert(RBA::CellInstArray.new(c1.cell_index, tt))

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")
    assert_equal(collect(c9.begin_shapes_rec(0), l), "[c1](0,100;1000,1200)")
    assert_equal(collect(c9.begin_shapes_rec(1), l), "")

    lt = RBA::Layout::new

    new_cells = [ c0, c9 ].collect { |c| lt.add_cell(c.name) }
    ( c0t, c9t ) = new_cells.collect { |ci| lt.cell(ci) }
    org_cells = [ c0, c9 ].collect { |c| c.cell_index }

    cm = RBA::CellMapping::new
    cm.for_multi_cells(lt, new_cells, l, org_cells)

    lt.copy_tree_shapes(l, cm)
    assert_equal(collect(c0t.begin_shapes_rec(0), lt), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](-1200,0;-100,1000)/[c0](1200,0;2200,1100)")
    assert_equal(c0t.begin_shapes_rec(0).shape.property("p"), nil)
    assert_equal(collect(c9t.begin_shapes_rec(0), lt), "[c9](0,100;1000,1200)")
    assert_equal(collect(c0t.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")
    assert_equal(c0t.begin_shapes_rec(1).shape.property("p"), 17)
    assert_equal(collect(c9t.begin_shapes_rec(1), l), "")

    lt = RBA::Layout::new

    new_cells = [ c0, c9 ].collect { |c| lt.add_cell(c.name) }
    ( c0t, c9t ) = new_cells.collect { |ci| lt.cell(ci) }
    org_cells = [ c0, c9 ].collect { |c| c.cell_index }

    cm = RBA::CellMapping::new
    cm.for_multi_cells_full(lt, new_cells, l, org_cells)

    lt.copy_tree_shapes(l, cm)
    assert_equal(collect(c0t.begin_shapes_rec(0), lt), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(c0t.begin_shapes_rec(0).shape.property("p"), nil)
    assert_equal(collect(c9t.begin_shapes_rec(0), lt), "[c1](0,100;1000,1200)")
    assert_equal(collect(c0t.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")
    assert_equal(c0t.begin_shapes_rec(1).shape.property("p"), 17)
    assert_equal(collect(c9t.begin_shapes_rec(1), l), "")

    lt = RBA::Layout::new
    lt.layer
    lt.layer
    ll = lt.layer

    new_cells = [ c0, c9 ].collect { |c| lt.add_cell(c.name) }
    ( c0t, c9t ) = new_cells.collect { |ci| lt.cell(ci) }
    org_cells = [ c0, c9 ].collect { |c| c.cell_index }

    cm = RBA::CellMapping::new
    cm.for_multi_cells_full(lt, new_cells, l, org_cells)

    lm = RBA::LayerMapping::new
    lm.map(0, ll)

    lt.copy_tree_shapes(l, cm, lm)
    assert_equal(collect(c0t.begin_shapes_rec(ll), lt), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(c0t.begin_shapes_rec(ll).shape.property("p"), nil)
    assert_equal(collect(c9t.begin_shapes_rec(ll), lt), "[c1](0,100;1000,1200)")

    lt = RBA::Layout::new
    ls = l.dup

    assert_equal(ls.cell(c0.cell_index).name, "c0")
    assert_equal(ls.cell(c9.cell_index).name, "c9")

    new_cells = [ c0, c9 ].collect { |c| lt.add_cell(c.name) }
    ( c0t, c9t ) = new_cells.collect { |ci| lt.cell(ci) }
    org_cells = [ c0, c9 ].collect { |c| c.cell_index }

    cm = RBA::CellMapping::new
    cm.for_multi_cells(lt, new_cells, ls, org_cells)

    lt.move_tree_shapes(ls, cm)

    assert_equal(collect(ls.cell(c0.cell_index).begin_shapes_rec(0), ls), "")
    assert_equal(collect(ls.cell(c0.cell_index).begin_shapes_rec(1), ls), "")
    assert_equal(collect(ls.cell(c9.cell_index).begin_shapes_rec(0), ls), "")
    assert_equal(collect(ls.cell(c9.cell_index).begin_shapes_rec(1), ls), "")

    assert_equal(collect(c0t.begin_shapes_rec(0), lt), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](-1200,0;-100,1000)/[c0](1200,0;2200,1100)")
    assert_equal(c0t.begin_shapes_rec(0).shape.property("p"), nil)
    assert_equal(collect(c9t.begin_shapes_rec(0), lt), "[c9](0,100;1000,1200)")
    assert_equal(collect(c0t.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")
    assert_equal(c0t.begin_shapes_rec(1).shape.property("p"), 17)
    assert_equal(collect(c9t.begin_shapes_rec(1), l), "")

    lt = RBA::Layout::new
    lt.layer
    lt.layer
    ll = lt.layer

    ls = l.dup

    assert_equal(ls.cell(c0.cell_index).name, "c0")
    assert_equal(ls.cell(c9.cell_index).name, "c9")

    new_cells = [ c0, c9 ].collect { |c| lt.add_cell(c.name) }
    ( c0t, c9t ) = new_cells.collect { |ci| lt.cell(ci) }
    org_cells = [ c0, c9 ].collect { |c| c.cell_index }

    cm = RBA::CellMapping::new
    cm.for_multi_cells(lt, new_cells, ls, org_cells)

    lm = RBA::LayerMapping::new
    lm.map(0, ll)

    lt.move_tree_shapes(ls, cm, lm)

    assert_equal(collect(ls.cell(c0.cell_index).begin_shapes_rec(0), ls), "")
    assert_equal(collect(ls.cell(c0.cell_index).begin_shapes_rec(1), ls), "[c0](1,101;1001,1201)")
    assert_equal(collect(ls.cell(c9.cell_index).begin_shapes_rec(0), ls), "")
    assert_equal(collect(ls.cell(c9.cell_index).begin_shapes_rec(1), ls), "")

    assert_equal(collect(c0t.begin_shapes_rec(ll), lt), "[c0](0,100;1000,1200)/[c0](0,100;1000,1200)/[c0](100,0;1100,1100)/[c0](-1200,0;-100,1000)/[c0](1200,0;2200,1100)")
    assert_equal(c0t.begin_shapes_rec(ll).shape.property("p"), nil)
    assert_equal(collect(c9t.begin_shapes_rec(ll), lt), "[c9](0,100;1000,1200)")

  end

  def test_21

    # dup cells
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)
    b = RBA::Box.new(1, 101, 1001, 1201)
    s = c0.shapes(1).insert(b)
    s.set_property("p", 17)

    tt = RBA::Trans.new
    s = c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    s.set_property("p", 18)
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    assert_equal(collect(c0.begin_shapes_rec(0), l), "[c0](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c0.begin_shapes_rec(1), l), "[c0](1,101;1001,1201)")

    c9 = c0.dup

    assert_equal(c9.name, "c0$1")
    assert_equal(collect(c9.begin_shapes_rec(0), l), "[c0$1](0,100;1000,1200)/[c2](100,0;1100,1100)/[c3](1200,0;2200,1100)/[c3](-1200,0;-100,1000)/[c1](0,100;1000,1200)")
    assert_equal(collect(c9.begin_shapes_rec(1), l), "[c0$1](1,101;1001,1201)")

  end

  def test_22

    # invalid cell indexes
   
    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    assert_equal(c0.cell_index, 0)
    assert_equal(c1.cell_index, 1)

    tt = RBA::Trans.new

    error = 0
    begin
      c0.insert(RBA::CellInstArray.new(2, tt))
    rescue
      error = 1
    end
    assert_equal(error, 1)

    error = 0
    begin
      c0.insert(RBA::CellInstArray.new(2, tt), 17)
    rescue
      error = 1
    end
    assert_equal(error, 1)

    dtt = RBA::DTrans.new

    error = 0
    begin
      c0.insert(RBA::DCellInstArray.new(2, dtt))
    rescue
      error = 1
    end
    assert_equal(error, 1)

    error = 0
    begin
      c0.insert(RBA::DCellInstArray.new(2, dtt), 42)
    rescue
      error = 1
    end
    assert_equal(error, 1)

  end

  def test_23

    # layer operations with shape types
   
    m = RBA::Manager::new

    l = RBA::Layout.new(m)
    l1 = l.insert_layer(RBA::LayerInfo.new(1, 0))
    l2 = l.insert_layer(RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
  
    c0.shapes(l1).insert(RBA::Box::new(1, 2, 3, 4))
    c1.shapes(l1).insert(RBA::Polygon::new(RBA::Box::new(1, 2, 3, 4)))

    str1 = l.each_cell.collect { |c| c.name + ":" + c.shapes(l1).each.collect { |sh| sh.to_s }.join(";") }.join("\n")
    assert_equal(str1, "c0:box (1,2;3,4)\nc1:polygon (1,2;1,4;3,4;3,2)")

    m.transaction("T")
    l.clear_layer(l1, RBA::Shapes::SPolygons)
    m.commit

    str1 = l.each_cell.collect { |c| c.name + ":" + c.shapes(l1).each.collect { |sh| sh.to_s }.join(";") }.join("\n")
    assert_equal(str1, "c0:box (1,2;3,4)\nc1:")

    m.undo 

    str1 = l.each_cell.collect { |c| c.name + ":" + c.shapes(l1).each.collect { |sh| sh.to_s }.join(";") }.join("\n")
    assert_equal(str1, "c0:box (1,2;3,4)\nc1:polygon (1,2;1,4;3,4;3,2)")

    m.transaction("T")
    l.move_layer(l1, l2, RBA::Shapes::SPolygons)
    m.commit

    str1 = l.each_cell.collect { |c| c.name + ":" + c.shapes(l1).each.collect { |sh| sh.to_s }.join(";") }.join("\n")
    assert_equal(str1, "c0:box (1,2;3,4)\nc1:")

    str2 = l.each_cell.collect { |c| c.name + ":" + c.shapes(l2).each.collect { |sh| sh.to_s }.join(";") }.join("\n")
    assert_equal(str2, "c0:\nc1:polygon (1,2;1,4;3,4;3,2)")

    m.undo 

    str1 = l.each_cell.collect { |c| c.name + ":" + c.shapes(l1).each.collect { |sh| sh.to_s }.join(";") }.join("\n")
    assert_equal(str1, "c0:box (1,2;3,4)\nc1:polygon (1,2;1,4;3,4;3,2)")

    str2 = l.each_cell.collect { |c| c.name + ":" + c.shapes(l2).each.collect { |sh| sh.to_s }.join(";") }.join("\n")
    assert_equal(str2, "c0:\nc1:")

    m.transaction("T")
    l.copy_layer(l1, l2, RBA::Shapes::SPolygons)
    m.commit

    str1 = l.each_cell.collect { |c| c.name + ":" + c.shapes(l1).each.collect { |sh| sh.to_s }.join(";") }.join("\n")
    assert_equal(str1, "c0:box (1,2;3,4)\nc1:polygon (1,2;1,4;3,4;3,2)")

    str2 = l.each_cell.collect { |c| c.name + ":" + c.shapes(l2).each.collect { |sh| sh.to_s }.join(";") }.join("\n")
    assert_equal(str2, "c0:\nc1:polygon (1,2;1,4;3,4;3,2)")

  end

  # Iterating while flatten
  def test_issue200

    ly = RBA::Layout.new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t200.gds")
    l1 = ly.layer(1, 0)
    l2 = ly.layer(2, 0)
    l3 = ly.layer(3, 0)

    tc_name = ly.top_cell.name
    r1 = RBA::Region::new(ly.top_cell.begin_shapes_rec(l1))
    r2 = RBA::Region::new(ly.top_cell.begin_shapes_rec(l2))
    r3 = RBA::Region::new(ly.top_cell.begin_shapes_rec(l3))
    assert_equal(r1.size > 0, true)
    assert_equal(r2.size > 0, true)
    assert_equal(r3.size == 0, true)

    ly.top_cell.each_inst do |ci|
      ci.flatten
    end

    tc = ly.cell(tc_name)
    assert_equal(ly.top_cells.size, 4)
    assert_equal(tc.child_cells, 0)
    assert_equal(tc.parent_cells, 0)

    rr1 = RBA::Region::new(tc.begin_shapes_rec(l1))
    rr2 = RBA::Region::new(tc.begin_shapes_rec(l2))
    rr3 = RBA::Region::new(tc.begin_shapes_rec(l3))
    assert_equal(r1.size, rr1.size)
    assert_equal(r2.size, rr2.size)
    assert_equal(r3.size, rr3.size)

    assert_equal((rr1 ^ r1).is_empty?, true)
    assert_equal((rr2 ^ r2).is_empty?, true)
    assert_equal((rr3 ^ r3).is_empty?, true)

  end

end

load("test_epilogue.rb")

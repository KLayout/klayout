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

def compare(a, b, flags = RBA::LayoutDiff::Verbose)

  res = []

  diff = RBA::LayoutDiff::new
  diff.on_dbu_differs { |a,b| res << "on_dbu_differs #{'%.12g'%a} #{'%.12g'%b}" }
  diff.on_layer_in_a_only { |a| res << "on_layer_in_a_only #{a}" }
  diff.on_layer_in_b_only { |b| res << "on_layer_in_b_only #{b}" }
  diff.on_layer_name_differs { |a,b| res << "on_layer_name_differs #{a} #{b}" }
  diff.on_cell_name_differs { |a,b| res << "on_cell_name_differs #{a.name} #{b.name}" }
  diff.on_cell_in_a_only { |a| res << "on_cell_in_a_only #{a.name}" }
  diff.on_cell_in_b_only { |b| res << "on_cell_in_b_only #{b.name}" }
  diff.on_bbox_differs { |a,b| res << "on_bbox_differs #{diff.cell_a.name}:#{a.to_s} #{diff.cell_b.name}:#{b.to_s}" }
  diff.on_begin_cell { |a,b| res << "on_begin_cell #{a.name} #{b.name}" }
  diff.on_begin_inst_differences { res << "on_begin_inst_differences" }
  diff.on_instance_in_a_only { |i,p| res << "on_instance_in_a_only #{i.to_s} #{p.to_s}" }
  diff.on_instance_in_b_only { |i,p| res << "on_instance_in_b_only #{i.to_s} #{p.to_s}" }
  diff.on_end_inst_differences { res << "on_end_inst_differences" }
  diff.on_begin_layer { |a,b| res << "on_begin_layer #{diff.layer_info_a.to_s}:#{a.to_s} #{diff.layer_info_b.to_s}:#{b.to_s}" }
  diff.on_per_layer_bbox_differs { |a,b| res << "on_per_layer_bbox_differs #{diff.cell_a.name}@#{diff.layer_info_a.to_s}:#{a.to_s} #{diff.cell_b.name}@#{diff.layer_info_b.to_s}:#{b.to_s}" }
  diff.on_begin_polygon_differences { res << "on_begin_polygon_differences" }
  diff.on_polygon_in_a_only { |p| res << "polygon_in_a_only #{p.to_s}" }
  diff.on_polygon_in_b_only { |p| res << "polygon_in_b_only #{p.to_s}" }
  diff.on_end_polygon_differences { res << "on_end_polygon_differences" }
  diff.on_begin_box_differences { res << "on_begin_box_differences" }
  diff.on_box_in_a_only { |p| res << "box_in_a_only #{p.to_s}" }
  diff.on_box_in_b_only { |p| res << "box_in_b_only #{p.to_s}" }
  diff.on_end_box_differences { res << "on_end_box_differences" }
  diff.on_begin_path_differences { res << "on_begin_path_differences" }
  diff.on_path_in_a_only { |p| res << "path_in_a_only #{p.to_s}" }
  diff.on_path_in_b_only { |p| res << "path_in_b_only #{p.to_s}" }
  diff.on_end_path_differences { res << "on_end_path_differences" }
  diff.on_begin_text_differences { res << "on_begin_text_differences" }
  diff.on_text_in_a_only { |p| res << "text_in_a_only #{p.to_s}" }
  diff.on_text_in_b_only { |p| res << "text_in_b_only #{p.to_s}" }
  diff.on_end_text_differences { res << "on_end_text_differences" }
  diff.on_begin_edge_differences { res << "on_begin_edge_differences" }
  diff.on_edge_in_a_only { |p| res << "edge_in_a_only #{p.to_s}" }
  diff.on_edge_in_b_only { |p| res << "edge_in_b_only #{p.to_s}" }
  diff.on_end_edge_differences { res << "on_end_edge_differences" }
  diff.on_end_layer { res << "on_end_layer" }
  diff.on_end_cell { res << "on_end_cell" }

  assert_equal(diff.compare(a, b, flags), false)

  return res.join("\n") + "\n"

end

class DBLayoutDiff_TestClass < TestBase

  def test_1

    a = RBA::Layout::new
    a.dbu = 0.1

    b = RBA::Layout::new
    b.dbu = 0.001

    res = compare(a, b)
    assert_equal(res, <<"END")
on_dbu_differs 0.1 0.001
END

  end

  def test_2

    a = RBA::Layout::new
    l1a = a.layer(1, 0)
    c1a = a.create_cell("C1")
    c1a.shapes(l1a).insert(RBA::Box::new(0, 0, 1000, 1000))

    b = RBA::Layout::new
    l1b = b.layer(1, 0)
    l2b = b.layer(2, 0)
    c1b = b.create_cell("C1")
    c2b = b.create_cell("C2")
    c1b.shapes(l1b).insert(RBA::Box::new(1, 0, 1000, 1001))
    c1b.shapes(l1b).insert(RBA::Edge::new(1, 0, 1000, 1001))
    c1b.shapes(l1b).insert(RBA::Polygon::new(RBA::Box::new(1, 0, 1000, 1001)))
    c1b.shapes(l1b).insert(RBA::Path::new([RBA::Point::new(1, 0), RBA::Point::new(1000, 1001)], 0))
    c1b.shapes(l1b).insert(RBA::Text::new("abc", RBA::Trans::new(1, 0)))

    res = compare(a, b)
    assert_equal(res, <<"END")
on_layer_in_b_only 2/0
on_cell_in_b_only C2
on_begin_cell C1 C1
on_begin_layer 1/0:1/0 1/0:0
on_begin_polygon_differences
polygon_in_b_only (1,0;1,1001;1000,1001;1000,0)
on_end_polygon_differences
on_begin_path_differences
path_in_b_only (1,0;1000,1001) w=0 bx=0 ex=0 r=false
on_end_path_differences
on_begin_text_differences
text_in_b_only ('abc',r0 1,0)
on_end_text_differences
on_begin_box_differences
box_in_a_only (0,0;1000,1000)
box_in_b_only (1,0;1000,1001)
on_end_box_differences
on_begin_edge_differences
edge_in_b_only (1,0;1000,1001)
on_end_edge_differences
on_end_layer
on_end_cell
END

    res = compare(b, a)
    assert_equal(res, <<"END")
on_layer_in_a_only 2/0
on_cell_in_a_only C2
on_begin_cell C1 C1
on_begin_layer 1/0:1/0 1/0:0
on_begin_polygon_differences
polygon_in_a_only (1,0;1,1001;1000,1001;1000,0)
on_end_polygon_differences
on_begin_path_differences
path_in_a_only (1,0;1000,1001) w=0 bx=0 ex=0 r=false
on_end_path_differences
on_begin_text_differences
text_in_a_only ('abc',r0 1,0)
on_end_text_differences
on_begin_box_differences
box_in_a_only (1,0;1000,1001)
box_in_b_only (0,0;1000,1000)
on_end_box_differences
on_begin_edge_differences
edge_in_a_only (1,0;1000,1001)
on_end_edge_differences
on_end_layer
on_end_cell
END

    res = compare(a, b, 0)
    assert_equal(res, <<"END")
on_layer_in_b_only 2/0
on_cell_in_b_only C2
on_begin_cell C1 C1
on_bbox_differs C1:(0,0;1000,1000) C1:(1,0;1000,1001)
on_begin_layer 1/0:1/0 1/0:0
on_per_layer_bbox_differs C1@1/0:(0,0;1000,1000) C1@1/0:(1,0;1000,1001)
on_begin_polygon_differences
on_end_polygon_differences
on_begin_path_differences
on_end_path_differences
on_begin_text_differences
on_end_text_differences
on_begin_box_differences
on_end_box_differences
on_begin_edge_differences
on_end_edge_differences
on_end_layer
on_end_cell
END

    res = compare(b, a, 0)
    assert_equal(res, <<"END")
on_layer_in_a_only 2/0
on_cell_in_a_only C2
on_begin_cell C1 C1
on_bbox_differs C1:(1,0;1000,1001) C1:(0,0;1000,1000)
on_begin_layer 1/0:1/0 1/0:0
on_per_layer_bbox_differs C1@1/0:(1,0;1000,1001) C1@1/0:(0,0;1000,1000)
on_begin_polygon_differences
on_end_polygon_differences
on_begin_path_differences
on_end_path_differences
on_begin_text_differences
on_end_text_differences
on_begin_box_differences
on_end_box_differences
on_begin_edge_differences
on_end_edge_differences
on_end_layer
on_end_cell
END

  end

  def test_3

    a = RBA::Layout::new
    l1a = a.layer(1, 0, "X")

    b = RBA::Layout::new
    l1b = b.layer(1, 0, "Y")

    res = compare(a, b)
    assert_equal(res, <<"END")
on_layer_name_differs X (1/0) Y (1/0)
END

  end

  def test_4

    a = RBA::Layout::new
    c1a = a.create_cell("C1")
    c2a = a.create_cell("C2")
    c1a.insert(RBA::CellInstArray::new(c2a.cell_index, RBA::Trans::new(10, 20)))

    b = RBA::Layout::new
    c1b = b.create_cell("C1")
    c2b = b.create_cell("C2")
    c1b.insert(RBA::CellInstArray::new(c2b.cell_index, RBA::Trans::new(11, 21)))

    res = compare(a, b)
    assert_equal(res, <<"END")
on_begin_cell C1 C1
on_begin_inst_differences
on_instance_in_a_only #1 r0 *1 10,20 0
on_instance_in_b_only #1 r0 *1 11,21 0
on_end_inst_differences
on_end_cell
on_begin_cell C2 C2
on_end_cell
END

    res = compare(a, b, 0)
    assert_equal(res, <<"END")
on_begin_cell C1 C1
on_begin_inst_differences
on_end_inst_differences
on_end_cell
on_begin_cell C2 C2
on_end_cell
END

  end

  def test_5

    a = RBA::Layout::new
    c1a = a.create_cell("C1")
    c2a = a.create_cell("C2")
    c1a.insert(RBA::CellInstArray::new(c2a.cell_index, RBA::Trans::new(10, 20)))

    b = RBA::Layout::new
    c1b = b.create_cell("C1")
    c2b = b.create_cell("C2X")
    c1b.insert(RBA::CellInstArray::new(c2b.cell_index, RBA::Trans::new(11, 21)))

    res = compare(a, b)
    assert_equal(res, <<"END")
on_cell_in_a_only C2
on_cell_in_b_only C2X
on_begin_cell C1 C1
on_begin_inst_differences
on_instance_in_a_only #1 r0 10,20 0
on_instance_in_b_only #1 r0 11,21 0
on_end_inst_differences
on_end_cell
END

  end

  def test_6

    a = RBA::Layout::new
    l1a = a.layer(1, 0)
    c1a = a.create_cell("C1")
    c2a = a.create_cell("C2")
    c2a.shapes(l1a).insert(RBA::Box::new(0, 0, 100, 100))
    c1a.insert(RBA::CellInstArray::new(c2a.cell_index, RBA::Trans::new(10, 20)))

    b = RBA::Layout::new
    l1b = b.layer(1, 0)
    c1b = b.create_cell("C1")
    c2b = b.create_cell("C2X")
    c2b.shapes(l1b).insert(RBA::Box::new(0, 0, 100, 100))
    c1b.insert(RBA::CellInstArray::new(c2b.cell_index, RBA::Trans::new(10, 20)))

    res = compare(a.top_cell, b.top_cell, 0)
    assert_equal(res, <<"END")
on_cell_in_a_only C2
on_cell_in_b_only C2X
on_begin_cell C1 C1
on_begin_inst_differences
on_end_inst_differences
on_begin_layer 1/0:1/0 1/0:0
on_end_layer
on_end_cell
END

    res = compare(a.top_cell, b.top_cell, RBA::LayoutDiff::Verbose + RBA::LayoutDiff::SmartCellMapping)
    assert_equal(res, <<"END")
on_cell_name_differs C2 C2X
on_begin_cell C1 C1
on_begin_layer 1/0:1/0 1/0:0
on_end_layer
on_end_cell
on_begin_cell C2 C2X
on_begin_layer 1/0:1/0 1/0:0
on_end_layer
on_end_cell
END

  end

end


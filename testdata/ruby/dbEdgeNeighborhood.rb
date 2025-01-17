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

class MyVisitor < RBA::EdgeNeighborhoodVisitor

  def initialize
    @log = {}
    @current_log = nil
  end

  def log
    @log.keys.sort { |a,b| a < b ? -1 : (a == b ? 0 : 1) }.collect { |k| @log[k].join("") }.join("")
  end

  def begin_polygon(layout, cell, polygon)
    polygon = polygon.dup
    output(polygon)
    @log[polygon] ||= []
    @current_log = @log[polygon]
    @current_log << "Polygon: #{polygon}\n"
  end
    
  def end_polygon
    @current_log << "/Polygon\n"
    @current_log = nil
  end
    
  def on_edge(layout, cell, edge, neighborhood)
    @current_log << "edge = #{edge}\n"
    neighborhood.each do |n|
      x1, x2 = n[0]
      polygons = n[1]
      polygons.each do |inp, poly|
        poly_str = poly.collect { |p| p.to_s }.join("/")
        @current_log << "  #{x1},#{x2} -> #{inp}: #{poly_str}\n"
      end
    end
  end

end
    
class MyVisitor2 < RBA::EdgeNeighborhoodVisitor

  def initialize
    self.result_type = RBA::CompoundRegionOperationNode::ResultType::EdgePairs
  end

  def on_edge(layout, cell, edge, neighborhood)
    neighborhood.each do |n|
      polygons = n[1]
      polygons.each do |inp, poly|
        poly.each do |p|
          bbox = p.bbox
          t = RBA::EdgeNeighborhoodVisitor.to_original_trans(edge)
          ep = RBA::EdgePair::new(edge, t * RBA::Edge::new(bbox.p1, RBA::Point::new(bbox.right, bbox.bottom)))
          output(ep)
        end
      end
    end
  end

end
    
class DBEdgeNeighborhood_TestClass < TestBase

  # basic events
  def test_1

    ly = RBA::Layout::new

    l1 = ly.layer(1, 0)
    cell = ly.create_cell("TOP")

    cell.shapes(l1).insert(RBA::Box::new(0, 0, 1000, 1000))
    cell.shapes(l1).insert(RBA::Box::new(-1100, 0, -100, 1000))

    prim = RBA::Region::new(cell.begin_shapes_rec(l1))

    visitor = MyVisitor::new

    visitor.result_type = RBA::CompoundRegionOperationNode::ResultType::Region
    assert_equal(visitor.result_type, RBA::CompoundRegionOperationNode::ResultType::Region)

    bext = 0
    eext = 0
    din = 10
    dout = 100

    children = [
      RBA::CompoundRegionOperationNode::new_foreign
    ] 

    node = RBA::CompoundRegionOperationNode::new_edge_neighborhood(children, visitor, bext, eext, din, dout)
    res = prim.complex_op(node)

    assert_equal(visitor.log, 
      "Polygon: (-1100,0;-1100,1000;-100,1000;-100,0)\n" +
      "edge = (-1100,0;-1100,1000)\n" +
      "edge = (-1100,1000;-100,1000)\n" +
      "edge = (-100,1000;-100,0)\n" +
      "  0.0,1000.0 -> 0: (0,100;0,101;1000,101;1000,100)\n" +
      "edge = (-100,0;-1100,0)\n" +
      "/Polygon\n" +
      "Polygon: (0,0;0,1000;1000,1000;1000,0)\n" +
      "edge = (0,0;0,1000)\n" +
      "  0.0,1000.0 -> 0: (0,100;0,101;1000,101;1000,100)\n" +
      "edge = (0,1000;1000,1000)\n" +
      "edge = (1000,1000;1000,0)\n" +
      "edge = (1000,0;0,0)\n" +
      "/Polygon\n"
    )

    assert_equal(res.to_s, "(-1100,0;-1100,1000;-100,1000;-100,0);(0,0;0,1000;1000,1000;1000,0)")

  end

  # edge pair output, to_original_trans
  def test_2

    ly = RBA::Layout::new

    l1 = ly.layer(1, 0)
    cell = ly.create_cell("TOP")

    cell.shapes(l1).insert(RBA::Box::new(0, 0, 1000, 1000))
    cell.shapes(l1).insert(RBA::Box::new(-1100, 0, -100, 1000))

    prim = RBA::Region::new(cell.begin_shapes_rec(l1))

    visitor = MyVisitor2::new

    bext = 0
    eext = 0
    din = 10
    dout = 100

    children = [
      RBA::CompoundRegionOperationNode::new_foreign
    ] 

    node = RBA::CompoundRegionOperationNode::new_edge_neighborhood(children, visitor, bext, eext, din, dout)
    res = prim.complex_op(node)

    assert_equal(res.to_s, "(-100,1000;-100,0)/(0,1000;0,0);(0,0;0,1000)/(-100,0;-100,1000)")

  end

end

load("test_epilogue.rb")


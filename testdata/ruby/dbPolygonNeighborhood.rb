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

class PMyVisitor < RBA::PolygonNeighborhoodVisitor

  def initialize
    @log = {}
    @current_log = nil
  end

  def log
    @log.keys.sort { |a,b| a < b ? -1 : (a == b ? 0 : 1) }.collect { |k| @log[k].join("") }.join("")
  end

  def neighbors(layout, cell, polygon, neighborhood)
    polygon = polygon.dup
    output(polygon)
    @log[polygon] ||= []
    @current_log = @log[polygon]
    @current_log << "Polygon: #{polygon}\n"
    neighborhood.each do |inp, poly|
      poly_str = poly.collect { |p| p.to_s }.join("/")
      @current_log << "  #{inp}: #{poly_str}\n"
    end
  end
    
end
    
class PMyVisitor2 < RBA::PolygonNeighborhoodVisitor

  def initialize
    self.result_type = RBA::CompoundRegionOperationNode::ResultType::Edges
  end

  def neighbors(layout, cell, polygon, neighborhood)
    neighborhood.each do |inp, poly|
      poly.each do |p|
        e = RBA::Edge::new(polygon.bbox.center, p.bbox.center)
        output(e)
      end
    end
  end

end
    
class DBPolygonNeighborhood_TestClass < TestBase

  # basic events
  def test_1

    ly = RBA::Layout::new

    l1 = ly.layer(1, 0)
    cell = ly.create_cell("TOP")

    pid1 = RBA::Layout::properties_id({ 1 => "one" })

    cell.shapes(l1).insert(RBA::Box::new(0, 0, 1000, 1000), pid1)
    cell.shapes(l1).insert(RBA::Box::new(-1100, 0, -100, 1000))

    prim = RBA::Region::new(cell.begin_shapes_rec(l1))
    prim.enable_properties

    visitor = PMyVisitor::new

    visitor.result_type = RBA::CompoundRegionOperationNode::ResultType::Region
    assert_equal(visitor.result_type, RBA::CompoundRegionOperationNode::ResultType::Region)

    dist = 101

    children = [
      RBA::CompoundRegionOperationNode::new_foreign
    ] 

    node = RBA::CompoundRegionOperationNode::new_polygon_neighborhood(children, visitor, dist)
    res = prim.complex_op(node)

    assert_equal(visitor.log, 
      "Polygon: (-1100,0;-1100,1000;-100,1000;-100,0) props={}\n" +
      "  0: (0,0;0,1000;1000,1000;1000,0) props={1=>one}\n" +
      "Polygon: (0,0;0,1000;1000,1000;1000,0) props={1=>one}\n" +
      "  0: (-1100,0;-1100,1000;-100,1000;-100,0) props={}\n"
    )

    assert_equal(res.to_s, "(-1100,0;-1100,1000;-100,1000;-100,0);(0,0;0,1000;1000,1000;1000,0){1=>one}")

  end

  # edge pair output, to_original_trans
  def test_2

    ly = RBA::Layout::new

    l1 = ly.layer(1, 0)
    cell = ly.create_cell("TOP")

    cell.shapes(l1).insert(RBA::Box::new(0, 0, 1000, 1000))
    cell.shapes(l1).insert(RBA::Box::new(-1100, 0, -100, 1000))

    prim = RBA::Region::new(cell.begin_shapes_rec(l1))

    visitor = PMyVisitor2::new

    dist = 101

    children = [
      RBA::CompoundRegionOperationNode::new_foreign
    ] 

    node = RBA::CompoundRegionOperationNode::new_polygon_neighborhood(children, visitor, dist)
    res = prim.complex_op(node)

    assert_equal(res.to_s, "(-600,500;500,500);(500,500;-600,500)")

  end

end

load("test_epilogue.rb")


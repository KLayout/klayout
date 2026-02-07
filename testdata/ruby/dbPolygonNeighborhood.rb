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
    
class PMyVisitor4Deep < RBA::PolygonNeighborhoodVisitor

  def initialize
    self.result_type = RBA::CompoundRegionOperationNode::ResultType::Region
    self.variant_type = RBA::VariantType::Orientation
  end

  def neighbors(layout, cell, polygon, neighborhood)
    prim_box = polygon.bbox
    neighborhood.each do |inp, others|
      others.each do |other|
        other_box = other.bbox
        top_part = prim_box.top - other_box.top
        bot_part = other_box.bottom - prim_box.bottom
        if top_part > 0 && bot_part > 0 && top_part > 2 * bot_part
          output(polygon)
        end
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

  # full example with deep mode, variant building
  def test_3

    ly = RBA::Layout::new
    l1 = ly.layer(1, 0)
    l2 = ly.layer(2, 0)

    top = ly.create_cell("TOP")
    child = ly.create_cell("CHILD")

    child.shapes(l1).insert(RBA::Box::new(-5000, -100, 5000, 100))
    child.shapes(l2).insert(RBA::Box::new(-1100, -3000, -900, 1000))
    child.shapes(l2).insert(RBA::Box::new(-100, -2000, 100, 2000))
    child.shapes(l2).insert(RBA::Box::new(900, -1000, 1100, 3000))

    top.insert(RBA::CellInstArray::new(child, RBA::Trans::new(RBA::Vector::new(0, -5000))))
    top.insert(RBA::CellInstArray::new(child, RBA::Trans::new(RBA::Trans::M0, RBA::Vector::new(0, 5000))))
    top.insert(RBA::CellInstArray::new(child, RBA::Trans::new(RBA::Trans::R90, RBA::Vector::new(-10000, 0))))

    dss = RBA::DeepShapeStore::new
    # Ruby does not like to be called from threads, so none given here:
    dss.threads = 0
    l1r = RBA::Region::new(RBA::RecursiveShapeIterator::new(ly, top, l1), dss)
    l2r = RBA::Region::new(RBA::RecursiveShapeIterator::new(ly, top, l2), dss)

    overlap = l1r & l2r
    puts overlap.to_s

    children = [
      RBA::CompoundRegionOperationNode::new_secondary(overlap)
    ] 
    visitor = PMyVisitor4Deep::new
    node = RBA::CompoundRegionOperationNode::new_polygon_neighborhood(children, visitor, -1)

    errors = l2r.complex_op(node)

    assert_equal(errors.to_s, "(900,-6000;900,-2000;1100,-2000;1100,-6000);(-1100,4000;-1100,8000;-900,8000;-900,4000)")

  end

end

load("test_epilogue.rb")


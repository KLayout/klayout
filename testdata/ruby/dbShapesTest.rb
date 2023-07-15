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

# cool stuff ...
class RBA::Shapes
  include Enumerable
end

class DBShapes_TestClass < TestBase

  # Shapes
  def test_7_Layout

    shapes = RBA::Shapes::new

    assert_equal(shapes.cell == nil, true)
    assert_equal(shapes.layout == nil, true)

    ly = RBA::Layout::new

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    c1.shapes( lindex ).insert( RBA::Box::new( 100, -10, 150, 40 ) )
    c1.shapes( lindex ).insert( RBA::Box::new( 200, -10, 250, 40 ) )

    shapes = c1.shapes( lindex )

    assert_equal(shapes.cell == c1, true)
    assert_equal(shapes.layout == ly, true)

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)", "(200,-10;250,40)"] )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.bbox.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)", "(200,-10;250,40)"] )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.area ) } 
    assert_equal( arr, [2000, 2500, 2500] )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.perimeter ) } 
    assert_equal( arr, [180, 200, 200] )

    arr = []
    shapes.each { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)", "(200,-10;250,40)"] )

    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)", "(200,-10;250,40)"] )

    arr = []
    shapes.each( RBA::Shapes::SRegions ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)", "(200,-10;250,40)"] )

    arr = []
    shapes.each( RBA::Shapes::SPolygons ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, [] )

    arr = []
    c1.each_overlapping_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.each_overlapping_shape( lindex, RBA::DBox::new( 0.040, 0, 0.100, 0.010 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.each_overlapping_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.each_overlapping_shape( lindex, RBA::DBox::new( 0.040, 0, 0.100, 0.010 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.shapes( lindex ).each_overlapping( RBA::Box::new( 40, 0, 100, 10 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.each_touching_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    arr = []
    c1.each_touching_shape( lindex, RBA::DBox::new( 0.040, 0, 0.100, 10 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    arr = []
    c1.each_touching_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    arr = []
    c1.each_touching_shape( lindex, RBA::DBox::new( 0.040, 0, 0.100, 0.010 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    arr = []
    c1.shapes( lindex ).each_touching( RBA::Box::new( 40, 0, 100, 10 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    c1.clear( lindex )

    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, [] )

    arr = []
    c1.each_touching_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, [] )

    arr = []
    c1.each_overlapping_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, [] )
  
    # boxes

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    arr = []
    shapes.each( RBA::Shapes::SEdges ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 0 )
    arr = []
    shapes.each( RBA::Shapes::SPoints ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 0 )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_box )
    assert_equal( arr[0].polygon.to_s, "(10,-10;10,40;50,40;50,-10)" )
    assert_equal( arr[0].simple_polygon.to_s, "(10,-10;10,40;50,40;50,-10)" )
    assert_equal( arr[0].point.inspect, "nil" )
    assert_equal( arr[0].edge.inspect, "nil" )
    assert_equal( arr[0].edge_pair.inspect, "nil" )
    assert_equal( arr[0].box.to_s, "(10,-10;50,40)" )
    assert_equal( arr[0].path.inspect, "nil" )
    assert_equal( arr[0].text.inspect, "nil" )
    assert_equal( arr[0].is_polygon?, false )
    assert_equal( arr[0].is_box?, true )
    assert_equal( arr[0].box.to_s, "(10,-10;50,40)" )
    assert_equal( arr[0].bbox.to_s, "(10,-10;50,40)" )

    # edges

    a = RBA::Edge::new( RBA::Point::new( -1, 2 ), RBA::Point::new( 5, 2 ) )
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SEdges ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_edge )
    assert_equal( arr[0].is_edge?, true )
    assert_equal( arr[0].polygon.inspect, "nil" )
    assert_equal( arr[0].simple_polygon.inspect, "nil" )
    assert_equal( arr[0].point.inspect, "nil" )
    assert_equal( arr[0].edge.to_s, "(-1,2;5,2)" )
    assert_equal( arr[0].edge_pair.inspect, "nil" )
    assert_equal( arr[0].box.inspect, "nil" )
    assert_equal( arr[0].path.inspect, "nil" )
    assert_equal( arr[0].text.inspect, "nil" )
    assert_equal( arr[0].edge == a, true )
    assert_equal( arr[0].bbox == a.bbox, true )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # edge pairs

    a = RBA::EdgePair::new(RBA::Edge::new(RBA::Point::new(-1, 2), RBA::Point::new(5, 2)), RBA::Edge::new(RBA::Point::new(-1, 5), RBA::Point::new(5, 5)))
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SEdgePairs ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_edge_pair )
    assert_equal( arr[0].is_edge_pair?, true )
    assert_equal( arr[0].polygon.inspect, "nil" )
    assert_equal( arr[0].simple_polygon.inspect, "nil" )
    assert_equal( arr[0].edge_pair.to_s, "(-1,2;5,2)/(-1,5;5,5)" )
    assert_equal( arr[0].point.inspect, "nil" )
    assert_equal( arr[0].edge.inspect, "nil" )
    assert_equal( arr[0].box.inspect, "nil" )
    assert_equal( arr[0].path.inspect, "nil" )
    assert_equal( arr[0].text.inspect, "nil" )
    assert_equal( arr[0].edge_pair == a, true )
    assert_equal( arr[0].bbox == a.bbox, true )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # points

    a = RBA::Point::new( -1, 2 )
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SPoints ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_point )
    assert_equal( arr[0].is_point?, true )
    assert_equal( arr[0].polygon.inspect, "nil" )
    assert_equal( arr[0].simple_polygon.inspect, "nil" )
    assert_equal( arr[0].point.to_s, "-1,2" )
    assert_equal( arr[0].edge.inspect, "nil" )
    assert_equal( arr[0].edge_pair.inspect, "nil" )
    assert_equal( arr[0].box.inspect, "nil" )
    assert_equal( arr[0].path.inspect, "nil" )
    assert_equal( arr[0].text.inspect, "nil" )
    assert_equal( arr[0].point == a, true )
    assert_equal( arr[0].bbox == RBA::Box::new(a, a), true )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # paths

    a = RBA::Path::new( [ RBA::Point::new( 0, 10 ), RBA::Point::new( 10, 50 ) ], 25 )
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SPaths ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_path )
    assert_equal( arr[0].is_path?, true )
    assert_equal( arr[0].polygon.to_s, "(12,7;-12,13;-2,53;22,47)" )
    assert_equal( arr[0].simple_polygon.to_s, "(12,7;-12,13;-2,53;22,47)" )
    assert_equal( arr[0].edge.inspect, "nil" )
    assert_equal( arr[0].edge_pair.inspect, "nil" )
    assert_equal( arr[0].box.inspect, "nil" )
    assert_equal( arr[0].path.to_s, "(0,10;10,50) w=25 bx=0 ex=0 r=false" )
    assert_equal( arr[0].text.inspect, "nil" )
    assert_equal( arr[0].path == a, true )
    assert_equal( arr[0].path_width, 25 )
    assert_equal( arr[0].perimeter, 132 )
    assert_equal( arr[0].area, 1025 )
    assert_equal( arr[0].path_length, 41 )
    assert_equal( arr[0].path_bgnext, 0 )
    assert_equal( arr[0].path_endext, 0 )
    assert_equal( arr[0].bbox == a.bbox, true )
    parr = []
    arr[0].each_point { |p| parr.push( p.to_s ) }
    assert_equal( parr, ["0,10", "10,50"] )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # simple polygons

    a = RBA::SimplePolygon::new( [ RBA::Point::new( 0, 1 ), RBA::Point::new( 1, 5 ), RBA::Point::new( 5, 5 ) ] )
    b = RBA::Polygon::new( [ RBA::Point::new( 0, 1 ), RBA::Point::new( 1, 5 ), RBA::Point::new( 5, 5 ) ] )
    c1.shapes( lindex ).insert( a )
    arr = shapes.each( RBA::Shapes::SPolygons ).to_a
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_simple_polygon )
    assert_equal( arr[0].is_polygon?, true )
    assert_equal( arr[0].polygon.to_s, "(0,1;1,5;5,5)" )
    assert_equal( arr[0].simple_polygon.to_s, "(0,1;1,5;5,5)" )
    assert_equal( arr[0].edge.inspect, "nil" )
    assert_equal( arr[0].edge_pair.inspect, "nil" )
    assert_equal( arr[0].box.inspect, "nil" )
    assert_equal( arr[0].path.inspect, "nil" )
    assert_equal( arr[0].text.inspect, "nil" )
    assert_equal( arr[0].is_simple_polygon?, true )
    begin
      arr[0].each_point { |x| }
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Shape is not a path in Shape::each_point")
    end
    begin
      arr[0].each_point_hole(0) { |x| }
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "A simple polygon doesn't have holes in Shape::each_point_hole")
    end
    begin
      arr[0].text_string
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Shape is not a text in Shape::text_string")
    end
    assert_equal( arr[0].simple_polygon == a, true )
    assert_equal( arr[0].polygon == b, true )
    assert_equal( arr[0].holes, 0 )
    assert_equal( arr[0].bbox == a.bbox, true )
    parr = []
    arr[0].each_point_hull { |p| parr.push( p.to_s ) }
    assert_equal( parr, ["0,1", "1,5", "5,5"] )
    earr = []
    arr[0].each_edge { |e| earr.push( e.to_s ) }
    assert_equal( earr, ["(0,1;1,5)", "(1,5;5,5)", "(5,5;0,1)"] )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # polygons

    a = RBA::Polygon::new( [ RBA::Point::new( 0, 1 ), RBA::Point::new( 1, 5 ), RBA::Point::new( 5, 5 ) ] )
    a.insert_hole( [ RBA::Point::new( 1, 2 ), RBA::Point::new( 2, 4 ), RBA::Point::new( 4, 4 ) ] )
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SPolygons ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 2 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_polygon )
    assert_equal( arr[0].is_polygon?, true )
    assert_equal( arr[0].is_simple_polygon?, false )
    assert_equal( arr[0].polygon == a, true )
    assert_equal( arr[0].holes, 1 )
    assert_equal( arr[0].bbox == a.bbox, true )
    parr = []
    arr[0].each_point_hull { |p| parr.push( p.to_s ) }
    assert_equal( parr, ["0,1", "1,5", "5,5"] )
    parr = []
    arr[0].each_point_hole(0) { |p| parr.push( p.to_s ) }
    assert_equal( parr, ["1,2", "4,4", "2,4"] )
    begin
      arr[0].each_point_hole(1) { |x| }
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Invalid hole index in Shape::each_point_hole")
    end
    earr = []
    arr[0].each_edge { |e| earr.push( e.to_s ) }
    assert_equal( earr, ["(0,1;1,5)", "(1,5;5,5)", "(5,5;0,1)", "(1,2;4,4)", "(4,4;2,4)", "(2,4;1,2)"] )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # All kind of polygonizable shapes:

    assert_equal( shapes.each( RBA::Shapes::SRegions ).collect(&:to_s), [ "polygon (0,1;1,5;5,5/1,2;4,4;2,4)",
                                                                          "simple_polygon (0,1;1,5;5,5)",
                                                                          "path (0,10;10,50) w=25 bx=0 ex=0 r=false",
                                                                          "box (10,-10;50,40)" ] )

  end

  # Shapes (double types)
  def test_7_DLayout

    ly = RBA::Layout::new

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    c1.shapes( lindex ).insert( RBA::DBox::new( 0.010, -0.010, 0.050, 0.040 ) )
    c1.shapes( lindex ).insert( RBA::DBox::new( 0.100, -0.010, 0.150, 0.040 ) )
    c1.shapes( lindex ).insert( RBA::DBox::new( 0.200, -0.010, 0.250, 0.040 ) )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)", "(200,-10;250,40)"] )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.bbox.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)", "(200,-10;250,40)"] )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.area ) } 
    assert_equal( arr, [2000, 2500, 2500] )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.perimeter ) } 
    assert_equal( arr, [180, 200, 200] )

    arr = []
    shapes.each { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)", "(200,-10;250,40)"] )

    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)", "(200,-10;250,40)"] )

    arr = []
    shapes.each( RBA::Shapes::SPolygons ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, [] )

    arr = []
    c1.each_overlapping_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.each_overlapping_shape( lindex, RBA::DBox::new( 0.040, 0, 0.100, 0.010 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.each_overlapping_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.each_overlapping_shape( lindex, RBA::DBox::new( 0.040, 0, 0.100, 0.010 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.shapes( lindex ).each_overlapping( RBA::Box::new( 40, 0, 100, 10 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.shapes( lindex ).each_overlapping( RBA::DBox::new( 0.04, 0.0, 0.100, 0.010 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)"] )

    arr = []
    c1.each_touching_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    arr = []
    c1.each_touching_shape( lindex, RBA::DBox::new( 0.040, 0, 0.100, 10 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    arr = []
    c1.each_touching_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    arr = []
    c1.each_touching_shape( lindex, RBA::DBox::new( 0.040, 0, 0.100, 0.010 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    arr = []
    c1.shapes( lindex ).each_touching( RBA::Box::new( 40, 0, 100, 10 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    arr = []
    c1.shapes( lindex ).each_touching( RBA::DBox::new( 0.040, 0, 0.100, 0.010 ) ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)"] )

    c1.clear( lindex )

    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, [] )

    arr = []
    c1.each_touching_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, [] )

    arr = []
    c1.each_overlapping_shape( lindex, RBA::Box::new( 40, 0, 100, 10 ), RBA::Shapes::SBoxes ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, [] )
  
    # boxes

    c1.shapes( lindex ).insert( RBA::DBox::new( 0.010, -0.010, 0.050, 0.040 ) )
    arr = []
    shapes.each( RBA::Shapes::SEdges ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 0 )
    arr = []
    shapes.each( RBA::Shapes::SPoints ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 0 )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_box )
    assert_equal( arr[0].dpolygon.to_s, "(0.01,-0.01;0.01,0.04;0.05,0.04;0.05,-0.01)" )
    assert_equal( arr[0].dsimple_polygon.to_s, "(0.01,-0.01;0.01,0.04;0.05,0.04;0.05,-0.01)" )
    assert_equal( arr[0].dedge.inspect, "nil" )
    assert_equal( arr[0].dedge_pair.inspect, "nil" )
    assert_equal( arr[0].dbox.to_s, "(0.01,-0.01;0.05,0.04)" )
    assert_equal( arr[0].dpath.inspect, "nil" )
    assert_equal( arr[0].dtext.inspect, "nil" )
    assert_equal( arr[0].is_polygon?, false )
    assert_equal( arr[0].is_box?, true )
    assert_equal( arr[0].dbox.to_s, "(0.01,-0.01;0.05,0.04)" )
    assert_equal( arr[0].dbbox.to_s, "(0.01,-0.01;0.05,0.04)" )

    # edges

    a = RBA::DEdge::new( RBA::DPoint::new( -0.001, 0.002 ), RBA::DPoint::new( 0.005, 0.002 ) )
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SEdges ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_edge )
    assert_equal( arr[0].is_edge?, true )
    assert_equal( arr[0].dpolygon.inspect, "nil" )
    assert_equal( arr[0].dsimple_polygon.inspect, "nil" )
    assert_equal( arr[0].dedge.to_s, "(-0.001,0.002;0.005,0.002)" )
    assert_equal( arr[0].dedge_pair.inspect, "nil" )
    assert_equal( arr[0].dbox.inspect, "nil" )
    assert_equal( arr[0].dpath.inspect, "nil" )
    assert_equal( arr[0].dtext.inspect, "nil" )
    assert_equal( arr[0].dbbox.to_s, "(-0.001,0.002;0.005,0.002)" )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # edge pairs

    a = RBA::DEdgePair::new(RBA::DEdge::new(RBA::DPoint::new(-0.001, 0.002), RBA::DPoint::new(0.005, 0.002)), RBA::DEdge::new(RBA::DPoint::new(-0.001, 0.005), RBA::DPoint::new(0.005, 0.005)))
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SEdgePairs ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_edge_pair )
    assert_equal( arr[0].is_edge_pair?, true )
    assert_equal( arr[0].dpolygon.inspect, "nil" )
    assert_equal( arr[0].dsimple_polygon.inspect, "nil" )
    assert_equal( arr[0].dpoint.inspect, "nil" )
    assert_equal( arr[0].dedge_pair.to_s, "(-0.001,0.002;0.005,0.002)/(-0.001,0.005;0.005,0.005)" )
    assert_equal( arr[0].dedge.inspect, "nil" )
    assert_equal( arr[0].dbox.inspect, "nil" )
    assert_equal( arr[0].dpath.inspect, "nil" )
    assert_equal( arr[0].dtext.inspect, "nil" )
    assert_equal( arr[0].dbbox.to_s, "(-0.001,0.002;0.005,0.005)" )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # points

    a = RBA::DPoint::new( -1, 2 )
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SPoints ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_point )
    assert_equal( arr[0].is_point?, true )
    assert_equal( arr[0].dpolygon.inspect, "nil" )
    assert_equal( arr[0].dsimple_polygon.inspect, "nil" )
    assert_equal( arr[0].dpoint.to_s, "-1,2" )
    assert_equal( arr[0].dedge.inspect, "nil" )
    assert_equal( arr[0].dedge_pair.inspect, "nil" )
    assert_equal( arr[0].dbox.inspect, "nil" )
    assert_equal( arr[0].dpath.inspect, "nil" )
    assert_equal( arr[0].dtext.inspect, "nil" )
    assert_equal( arr[0].dbbox.to_s, "(-1,2;-1,2)" )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # paths

    a = RBA::DPath::new( [ RBA::DPoint::new( 0, 0.010 ), RBA::DPoint::new( 0.010, 0.050 ) ], 0.025 )
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SPaths ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_path )
    assert_equal( arr[0].is_path?, true )
    begin
      arr[0].each_point_hull { |x| }
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Shape is not a general or simple polygon in Shape::each_point_hull")
    end
    begin
      arr[0].each_point_hole(0) { |x| }
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Shape is not a general or simple polygon in Shape::each_point_hole")
    end
    assert_equal( arr[0].dpolygon.to_s, "(0.012,0.007;-0.012,0.013;-0.002,0.053;0.022,0.047)" )
    assert_equal( arr[0].dsimple_polygon.to_s, "(0.012,0.007;-0.012,0.013;-0.002,0.053;0.022,0.047)" )
    assert_equal( arr[0].dedge.inspect, "nil" )
    assert_equal( arr[0].dedge_pair.inspect, "nil" )
    assert_equal( arr[0].dbox.inspect, "nil" )
    assert_equal( arr[0].dpath.to_s, "(0,0.01;0.01,0.05) w=0.025 bx=0 ex=0 r=false" )
    assert_equal( arr[0].dtext.inspect, "nil" )
    assert_equal( "%.12g" % arr[0].path_dwidth, "0.025" )
    assert_equal( "%.12g" % arr[0].dperimeter, "0.132" )
    assert_equal( "%.12g" % arr[0].darea, "0.001025" )
    assert_equal( "%.12g" % arr[0].path_dlength, "0.041" )
    assert_equal( arr[0].path_dbgnext, 0 )
    assert_equal( arr[0].path_dendext, 0 )
    parr = []
    arr[0].each_dpoint { |p| parr.push( p.to_s ) }
    assert_equal( parr, ["0,0.01", "0.01,0.05"] )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # simple polygons

    a = RBA::DSimplePolygon::new( [ RBA::DPoint::new( 0, 0.001 ), RBA::DPoint::new( 0.001, 0.005 ), RBA::DPoint::new( 0.005, 0.005 ) ] )
    b = RBA::DPolygon::new( [ RBA::DPoint::new( 0, 0.001 ), RBA::DPoint::new( 0.001, 0.005 ), RBA::DPoint::new( 0.005, 0.005 ) ] )
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SPolygons ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_simple_polygon )
    assert_equal( arr[0].is_polygon?, true )
    assert_equal( arr[0].dpolygon.to_s, "(0,0.001;0.001,0.005;0.005,0.005)" )
    assert_equal( arr[0].dsimple_polygon.to_s, "(0,0.001;0.001,0.005;0.005,0.005)" )
    assert_equal( arr[0].dedge.inspect, "nil" )
    assert_equal( arr[0].dedge_pair.inspect, "nil" )
    assert_equal( arr[0].dbox.inspect, "nil" )
    assert_equal( arr[0].dpath.inspect, "nil" )
    assert_equal( arr[0].dtext.inspect, "nil" )
    assert_equal( arr[0].is_simple_polygon?, true )
    assert_equal( arr[0].holes, 0 )
    parr = []
    arr[0].each_dpoint_hull { |p| parr.push( p.to_s ) }
    assert_equal( parr, ["0,0.001", "0.001,0.005", "0.005,0.005"] )
    earr = []
    arr[0].each_dedge { |e| earr.push( e.to_s ) }
    assert_equal( earr, ["(0,0.001;0.001,0.005)", "(0.001,0.005;0.005,0.005)", "(0.005,0.005;0,0.001)"] )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

    # polygons

    a = RBA::DPolygon::new( [ RBA::DPoint::new( 0, 0.001 ), RBA::DPoint::new( 0.001, 0.005 ), RBA::DPoint::new( 0.005, 0.005 ) ] )
    c1.shapes( lindex ).insert( a )
    arr = []
    shapes.each( RBA::Shapes::SPolygons ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 2 )
    assert_equal( arr[0].prop_id, 0 )
    assert_equal( arr[0].has_prop_id?, false )
    assert_equal( arr[0].is_null?, false )
    assert_equal( arr[0].type, RBA::Shape::t_polygon )
    assert_equal( arr[0].is_polygon?, true )
    assert_equal( arr[0].is_simple_polygon?, false )
    assert_equal( arr[0].dpolygon.to_s, "(0,0.001;0.001,0.005;0.005,0.005)" )
    assert_equal( arr[0].dsimple_polygon.to_s, "(0,0.001;0.001,0.005;0.005,0.005)" )
    assert_equal( arr[0].holes, 0 )
    assert_equal( arr[0].dbbox.to_s, "(0,0.001;0.005,0.005)" )
    parr = []
    arr[0].each_dpoint_hull { |p| parr.push( p.to_s ) }
    assert_equal( parr, ["0,0.001", "0.001,0.005", "0.005,0.005"] )
    earr = []
    arr[0].each_dedge { |e| earr.push( e.to_s ) }
    assert_equal( earr, ["(0,0.001;0.001,0.005)", "(0.001,0.005;0.005,0.005)", "(0.005,0.005;0,0.001)"] )
    arr = []
    shapes.each( RBA::Shapes::SBoxes ) { |s| arr.push( s ) } 
    assert_equal( arr.size, 1 )
    assert_equal( arr[0].is_box?, true )

  end

  # Shapes with properties
  def test_8_Layout

    ly = RBA::Layout::new

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    c1.shapes( lindex ).insert( RBA::Box::new( 100, -10, 150, 40 ), 5 )
    c1.shapes( lindex ).insert( RBA::Box::new( 200, -10, 250, 40 ), 8 )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.box.to_s ); arr.push( s.prop_id ) } 
    assert_equal( arr, [ "(10,-10;50,40)", 0, "(100,-10;150,40)", 5, "(200,-10;250,40)", 8 ] )

    assert_equal( ly.properties( 17 ), [] )
    assert_equal( ly.properties_id( { 17 => "a", "b" => [ 1, 5, 7 ] }.to_a ), 1 )
    assert_equal( ly.properties_id( { 17 => "a", "b" => [ 1, 5, 7 ] }.to_a ), 1 )
    assert_equal( ly.properties_id( { 17 => "a", "b" => [ 1, 5, 8 ] }.to_a ), 2 )
    assert_equal( ly.properties_id( { [1,2] => "hallo", 5 => nil }.to_a ), 3 )

    h = {}
    ly.properties( 1 ).each { |p| h[p[0]] = p[1] }
    assert_equal( h, { 17 => "a", "b" => [ 1, 5, 7 ] } )
    h = {}
    ly.properties( 2 ).each { |p| h[p[0]] = p[1] }
    assert_equal( h, { 17 => "a", "b" => [ 1, 5, 8 ] } )
    h = {}
    ly.properties( 3 ).each { |p| h[p[0]] = p[1] }
    assert_equal( h, { [1,2] => "hallo", 5 => nil } )

  end 

  # Shapes (edit)
  def test_7_LayoutEdit

    ly = RBA::Layout::new(true)

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    s1 = c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    s2 = c1.shapes( lindex ).insert( RBA::Box::new( 100, -10, 150, 40 ) )
    s3 = c1.shapes( lindex ).insert( RBA::Box::new( 200, -10, 250, 40 ) )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(10,-10;50,40)", "(100,-10;150,40)", "(200,-10;250,40)"] )

    c1.shapes( lindex ).erase( s1 )
    assert_equal( s1.is_valid?, false )
    assert_equal( s1.shapes.is_valid?(s1), false )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(100,-10;150,40)", "(200,-10;250,40)"] )

    c1.shapes( lindex ).erase( s3 )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, ["(100,-10;150,40)"] )

    c1.shapes( lindex ).erase( s2 )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.box.to_s ) } 
    assert_equal( arr, [] )

    # replace etc.

    s1 = c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    path = RBA::Path::new( [ RBA::Point::new( 0, 10 ), RBA::Point::new( 10, 50 ) ], 25 )
    s2 = c1.shapes( lindex ).insert( path )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["path (0,10;10,50) w=25 bx=0 ex=0 r=false", "box (10,-10;50,40)"] )

    sdup = shapes.dup
    sdup.transform( RBA::ICplxTrans::new( 1.0, 45.0, false, RBA::Point::new(0.0, 0.0) ) )

    arr = []
    sdup.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["polygon (14,0;-21,35;7,64;42,28)", "path (-7,7;-28,42) w=25 bx=0 ex=0 r=false"] )

    sdup = shapes.dup
    sdup.transform( RBA::Trans::new( 1, false, RBA::Point::new(0.0, 0.0) ) )

    arr = []
    sdup.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["path (-10,0;-50,10) w=25 bx=0 ex=0 r=false", "box (-40,10;10,50)"] )

    s1 = shapes.transform( s1, RBA::DCplxTrans::new( 1.0, 45.0, false, RBA::DPoint::new(0.001, 0.002) ) )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["simple_polygon (15,2;-20,37;8,66;43,30)", "path (0,10;10,50) w=25 bx=0 ex=0 r=false"] )

    shapes.erase( s1 )
    s1 = c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    s1 = shapes.transform( s1, RBA::ICplxTrans::new( 1.0, 45.0, false, RBA::Point::new(0.0, 0.0) ) )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["simple_polygon (14,0;-21,35;7,64;42,28)", "path (0,10;10,50) w=25 bx=0 ex=0 r=false"] )

    shapes.erase( s1 )
    s1a = c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    s1 = c1.shapes( lindex ).insert( s1a, RBA::Trans::new( 1 ) )
    shapes.erase( s1a )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["path (0,10;10,50) w=25 bx=0 ex=0 r=false", "box (-40,10;10,50)"] )

    shapes.erase( s1 )
    s1a = c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    s1 = c1.shapes( lindex ).insert( s1a, RBA::ICplxTrans::new( 1.0, 45.0, false, RBA::Point::new(0.0, 0.0) ) )
    shapes.erase( s1a )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["simple_polygon (14,0;-21,35;7,64;42,28)", "path (0,10;10,50) w=25 bx=0 ex=0 r=false"] )

    shapes.erase( s2 )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["simple_polygon (14,0;-21,35;7,64;42,28)"] )

    s2 = shapes.insert( s1 )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["simple_polygon (14,0;-21,35;7,64;42,28)", "simple_polygon (14,0;-21,35;7,64;42,28)"] )

    s2 = shapes.replace_prop_id( s2, 17 )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["simple_polygon (14,0;-21,35;7,64;42,28)", "simple_polygon (14,0;-21,35;7,64;42,28) prop_id=17"] )

    s2 = shapes.replace( s2, RBA::Box::new( 10, -10, 50, 40 ) )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["simple_polygon (14,0;-21,35;7,64;42,28)", "box (10,-10;50,40) prop_id=17"] )

    s2 = shapes.replace( s2, RBA::Edge::new( 10, -10, 50, 40 ) )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["simple_polygon (14,0;-21,35;7,64;42,28)", "edge (10,-10;50,40) prop_id=17"] )

    s2 = shapes.replace( s2, RBA::EdgePair::new( RBA::Edge::new( 10, -10, 50, 40 ), RBA::Edge::new( 10, 0, 50, 30 ) ) )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["simple_polygon (14,0;-21,35;7,64;42,28)", "edge_pair (10,-10;50,40)/(10,0;50,30) prop_id=17"] )

    shapes.erase( s2 )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["simple_polygon (14,0;-21,35;7,64;42,28)"] )

    shapes.erase( s1 )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, [] )

  end

  # Shapes (edit, new methods)
  def test_7_LayoutEdit2

    ly = RBA::Layout::new(true)

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    s1 = c1.shapes( lindex ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    s2 = c1.shapes( lindex ).insert( RBA::Box::new( 100, -10, 150, 40 ) )
    s3 = c1.shapes( lindex ).insert( RBA::Box::new( 200, -10, 250, 40 ) )

    assert_equal( s1.cell.name, c1.name ) 
    assert_equal( s1.layout.inspect, ly.inspect ) 

    s1.box = RBA::Box::new( 11, -11, 51, 41 )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["box (11,-11;51,41)", "box (100,-10;150,40)", "box (200,-10;250,40)"] )

    s2.text = RBA::Text::new( "text", RBA::Trans::new( RBA::Point::new( 100, 200 ) ) )
    assert_equal(s2.polygon.inspect, "nil")
    assert_equal(s2.simple_polygon.inspect, "nil")
    assert_equal(s2.text.to_s, "('text',r0 100,200)")
    assert_equal(s2.edge.inspect, "nil")
    assert_equal(s2.edge_pair.inspect, "nil")
    assert_equal(s2.path.inspect, "nil")
    assert_equal(s2.box.inspect, "nil")

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["box (11,-11;51,41)", "box (200,-10;250,40)", "text ('text',r0 100,200)"] )

    s3.edge_pair = RBA::EdgePair::new(RBA::Edge::new(RBA::Point::new(1, 2), RBA::Point::new(3, 4)), RBA::Edge::new(RBA::Point::new(1, 12), RBA::Point::new(3, 14)))

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge_pair (1,2;3,4)/(1,12;3,14)", "box (11,-11;51,41)", "text ('text',r0 100,200)"] )

    arr = []
    shapes.each( RBA::Shapes::SEdgePairs ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge_pair (1,2;3,4)/(1,12;3,14)"] )

    s3.edge = RBA::Edge::new( RBA::Point::new( 1, 2 ), RBA::Point::new( 3, 4 ) )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (1,2;3,4)", "box (11,-11;51,41)", "text ('text',r0 100,200)"] )

    arr = []
    shapes.each( RBA::Shapes::SEdges ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (1,2;3,4)"] )

    pts = [ RBA::Point::new( 100, 200 ), RBA::Point::new( 400, 300 ), RBA::Point::new( 500, 600 ) ]
    s1.polygon = RBA::Polygon::new( pts )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["polygon (100,200;500,600;400,300)", "edge (1,2;3,4)", "text ('text',r0 100,200)"] )

    assert_equal( s1.prop_id, 0 )
    assert_equal( s1.has_prop_id?, false )
    s1.prop_id = 1
    assert_equal( s1.prop_id, 1 )
    assert_equal( s1.has_prop_id?, true )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["polygon (100,200;500,600;400,300) prop_id=1", "edge (1,2;3,4)", "text ('text',r0 100,200)"] )

    s1.path = RBA::Path::new( [ RBA::Point::new( 0, 10 ), RBA::Point::new( 10, 50 ) ], 25 )
    assert_equal( s1.to_s, "path (0,10;10,50) w=25 bx=0 ex=0 r=false prop_id=1" )
    s1.path_width = 12
    assert_equal( s1.to_s, "path (0,10;10,50) w=12 bx=0 ex=0 r=false prop_id=1" )
    assert_equal( s1.path_width, 12 )
    s1.path_bgnext = 1
    assert_equal( s1.to_s, "path (0,10;10,50) w=12 bx=1 ex=0 r=false prop_id=1" )
    assert_equal( s1.path_bgnext, 1 )
    s1.path_endext = 2
    assert_equal( s1.to_s, "path (0,10;10,50) w=12 bx=1 ex=2 r=false prop_id=1" )
    assert_equal( s1.path_endext, 2 )
    s1.round_path = true
    assert_equal( s1.to_s, "path (0,10;10,50) w=12 bx=1 ex=2 r=true prop_id=1" )
    assert_equal( s1.round_path?, true )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (1,2;3,4)", "path (0,10;10,50) w=12 bx=1 ex=2 r=true prop_id=1", "text ('text',r0 100,200)"] )

    s1.box = RBA::Box::new(0, 10, 20, 40)
    assert_equal( s1.to_s, "box (0,10;20,40) prop_id=1" )

    assert_equal( s1.box_width.to_s, "20" )
    s1.box_width = 30
    assert_equal( s1.to_s, "box (-5,10;25,40) prop_id=1" )
    assert_equal( s1.box_width.to_s, "30" )

    assert_equal( s1.box_height.to_s, "30" )
    s1.box_height = 40
    assert_equal( s1.to_s, "box (-5,5;25,45) prop_id=1" )
    assert_equal( s1.box_height.to_s, "40" )

    assert_equal( s1.box_p1.to_s, "-5,5" )
    s1.box_p1 = RBA::Point::new(0, 0)
    assert_equal( s1.to_s, "box (0,0;25,45) prop_id=1" )
    assert_equal( s1.box_p1.to_s, "0,0" )

    assert_equal( s1.box_p2.to_s, "25,45" )
    s1.box_p2 = RBA::Point::new(10, 20)
    assert_equal( s1.to_s, "box (0,0;10,20) prop_id=1" )
    assert_equal( s1.box_p2.to_s, "10,20" )

    assert_equal( s1.box_center.to_s, "5,10" )
    s1.box_center = RBA::Point::new(-10, -20)
    assert_equal( s1.to_s, "box (-15,-30;-5,-10) prop_id=1" )
    assert_equal( s1.box_center.to_s, "-10,-20" )

    s1.text = RBA::Text::new( "text", RBA::Trans::new( RBA::Point::new( 100, 200 ) ) )
    assert_equal( s1.to_s, "text ('text',r0 100,200) prop_id=1" )

    s1.text_string = "blabla"
    assert_equal( s1.to_s, "text ('blabla',r0 100,200) prop_id=1" )
    assert_equal( s1.text_string, "blabla" )

    s1.text_halign = 1
    assert_equal( s1.text_halign, 1 )

    s1.text_valign = 2
    assert_equal( s1.text_valign, 2 )

    s1.text_font = 3
    assert_equal( s1.text_font, 3 )

    s1.text_trans = RBA::Trans::new( 1, false, RBA::Point::new( 101, 201 ) )
    assert_equal( s1.text_trans.to_s, "r90 101,201" )

    assert_equal( s1.text_rot.to_s, "1" )
    s1.text_rot = 4
    assert_equal( s1.text_trans.to_s, "m0 101,201" )

    assert_equal( s1.text_pos.to_s, "101,201" )
    s1.text_pos = RBA::Point::new(100, 200) 
    assert_equal( s1.text_trans.to_s, "m0 100,200" )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (1,2;3,4)", "text ('text',r0 100,200)", "text ('blabla',m0 100,200) prop_id=1"] )

    assert_equal( s1.is_valid?, true )
    assert_equal( s1.shapes.is_valid?(s1), true )
    s1.delete 
    assert_equal( s1.to_s, "null" )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (1,2;3,4)", "text ('text',r0 100,200)"] )

    s3.transform (RBA::Trans::new(RBA::Trans::R90));

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (-2,1;-4,3)", "text ('text',r0 100,200)"] )

    s3.transform (RBA::ICplxTrans::new(2.0));

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (-4,2;-8,6)", "text ('text',r0 100,200)"] )

    s3.transform (RBA::DCplxTrans::new(0.5, 0.0, false, RBA::DVector::new(0.01, 0.02)));

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (8,21;6,23)", "text ('text',r0 100,200)"] )

    s3.transform (RBA::DTrans::new(0, false, RBA::DVector::new(0.01, 0.02)));

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (18,41;16,43)", "text ('text',r0 100,200)"] )

  end

  # Shapes (edit, new methods)
  def test_7_DLayoutEdit2

    ly = RBA::Layout::new(true)

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 2
    ldummy = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    s1 = c1.shapes( lindex ).insert( RBA::DBox::new( 0.010, -0.010, 0.050, 0.040 ) )
    s2 = c1.shapes( lindex ).insert( RBA::DBox::new( 0.100, -0.010, 0.150, 0.040 ) )
    s3 = c1.shapes( lindex ).insert( RBA::DBox::new( 0.200, -0.010, 0.250, 0.040 ) )

    assert_equal( s1.cell.name, c1.name ) 
    assert_equal( s1.layout.inspect, ly.inspect ) 

    s1.box = RBA::DBox::new( 0.011, -0.011, 0.051, 0.041 )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["box (11,-11;51,41)", "box (100,-10;150,40)", "box (200,-10;250,40)"] )

    s2.text = RBA::DText::new( "text", RBA::DTrans::new( RBA::DPoint::new( 0.100, 0.200 ) ) )
    assert_equal(s2.polygon.inspect, "nil")
    assert_equal(s2.simple_polygon.inspect, "nil")
    assert_equal(s2.text.to_s, "('text',r0 100,200)")
    assert_equal(s2.edge.inspect, "nil")
    assert_equal(s2.path.inspect, "nil")
    assert_equal(s2.box.inspect, "nil")

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["box (11,-11;51,41)", "box (200,-10;250,40)", "text ('text',r0 100,200)"] )

    s3.edge_pair = RBA::DEdgePair::new(RBA::DEdge::new(RBA::DPoint::new(0.001, 0.002), RBA::DPoint::new(0.003, 0.004)), RBA::DEdge::new(RBA::DPoint::new(0.001, 0.012), RBA::DPoint::new(0.003, 0.014)))

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge_pair (1,2;3,4)/(1,12;3,14)", "box (11,-11;51,41)", "text ('text',r0 100,200)"] )

    s3.edge = RBA::DEdge::new( RBA::DPoint::new( 0.001, 0.002 ), RBA::DPoint::new( 0.003, 0.004 ) )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (1,2;3,4)", "box (11,-11;51,41)", "text ('text',r0 100,200)"] )

    pts = [ RBA::DPoint::new( 0.100, 0.200 ), RBA::DPoint::new( 0.400, 0.300 ), RBA::DPoint::new( 0.500, 0.600 ) ]
    s1.polygon = RBA::DPolygon::new( pts )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["polygon (100,200;500,600;400,300)", "edge (1,2;3,4)", "text ('text',r0 100,200)"] )

    assert_equal( s1.prop_id, 0 )
    assert_equal( s1.has_prop_id?, false )
    s1.prop_id = 1
    assert_equal( s1.prop_id, 1 )
    assert_equal( s1.has_prop_id?, true )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["polygon (100,200;500,600;400,300) prop_id=1", "edge (1,2;3,4)", "text ('text',r0 100,200)"] )

    s1.path = RBA::DPath::new( [ RBA::DPoint::new( 0, 0.010 ), RBA::DPoint::new( 0.010, 0.050 ) ], 0.025 )
    assert_equal( s1.to_s, "path (0,10;10,50) w=25 bx=0 ex=0 r=false prop_id=1" )
    s1.path_dwidth = 0.012
    assert_equal( s1.to_s, "path (0,10;10,50) w=12 bx=0 ex=0 r=false prop_id=1" )
    assert_equal( "%.12g" % s1.path_dwidth, "0.012" )
    s1.path_dbgnext = 0.001
    assert_equal( s1.to_s, "path (0,10;10,50) w=12 bx=1 ex=0 r=false prop_id=1" )
    assert_equal( "%.12g" % s1.path_dbgnext, "0.001" )
    s1.path_dendext = 0.002
    assert_equal( s1.to_s, "path (0,10;10,50) w=12 bx=1 ex=2 r=false prop_id=1" )
    assert_equal( "%.12g" % s1.path_dendext, "0.002" )
    s1.round_path = true
    assert_equal( s1.to_s, "path (0,10;10,50) w=12 bx=1 ex=2 r=true prop_id=1" )
    assert_equal( s1.round_path?, true )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (1,2;3,4)", "path (0,10;10,50) w=12 bx=1 ex=2 r=true prop_id=1", "text ('text',r0 100,200)"] )

    s1.box = RBA::DBox::new(0, 0.010, 0.020, 0.040)
    assert_equal( s1.to_s, "box (0,10;20,40) prop_id=1" )

    assert_equal( s1.box_dwidth.to_s, "0.02" )
    s1.box_dwidth = 0.03
    assert_equal( s1.to_s, "box (-5,10;25,40) prop_id=1" )
    assert_equal( s1.box_dwidth.to_s, "0.03" )

    assert_equal( s1.box_dheight.to_s, "0.03" )
    s1.box_dheight = 0.04
    assert_equal( s1.to_s, "box (-5,5;25,45) prop_id=1" )
    assert_equal( s1.box_dheight.to_s, "0.04" )

    assert_equal( s1.box_dp1.to_s, "-0.005,0.005" )
    s1.box_dp1 = RBA::DPoint::new(0, 0)
    assert_equal( s1.to_s, "box (0,0;25,45) prop_id=1" )
    assert_equal( s1.box_dp1.to_s, "0,0" )

    assert_equal( s1.box_dp2.to_s, "0.025,0.045" )
    s1.box_dp2 = RBA::DPoint::new(0.010, 0.020)
    assert_equal( s1.to_s, "box (0,0;10,20) prop_id=1" )
    assert_equal( s1.box_dp2.to_s, "0.01,0.02" )

    assert_equal( s1.box_dcenter.to_s, "0.005,0.01" )
    s1.box_dcenter = RBA::DPoint::new(-0.010, -0.020)
    assert_equal( s1.to_s, "box (-15,-30;-5,-10) prop_id=1" )
    assert_equal( s1.box_dcenter.to_s, "-0.01,-0.02" )

    s1.text = RBA::DText::new( "text", RBA::DTrans::new( RBA::DPoint::new( 0.100, 0.200 ) ) )
    assert_equal( s1.to_s, "text ('text',r0 100,200) prop_id=1" )

    s1.text_string = "blabla"
    assert_equal( s1.to_s, "text ('blabla',r0 100,200) prop_id=1" )
    assert_equal( s1.text_string, "blabla" )

    s1.text_halign = 1
    assert_equal( s1.text_halign, 1 )

    s1.text_valign = 2
    assert_equal( s1.text_valign, 2 )

    s1.text_font = 3
    assert_equal( s1.text_font, 3 )

    s1.text_dtrans = RBA::DTrans::new( 1, false, RBA::DPoint::new( 0.101, 0.201 ) )
    assert_equal( s1.text_trans.to_s, "r90 101,201" )

    assert_equal( s1.text_rot.to_s, "1" )
    s1.text_rot = 4
    assert_equal( s1.text_trans.to_s, "m0 101,201" )

    assert_equal( s1.text_dpos.to_s, "0.101,0.201" )
    s1.text_dpos = RBA::DPoint::new(0.100, 0.200) 
    assert_equal( s1.text_trans.to_s, "m0 100,200" )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (1,2;3,4)", "text ('text',r0 100,200)", "text ('blabla',m0 100,200) prop_id=1"] )

    assert_equal( s1.is_valid?, true )
    assert_equal( s1.shapes.is_valid?(s1), true )
    s1.delete 
    assert_equal( s1.to_s, "null" )

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (1,2;3,4)", "text ('text',r0 100,200)"] )

    s3.transform (RBA::DTrans::new(RBA::DTrans::R90));

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (-2,1;-4,3)", "text ('text',r0 100,200)"] )

    s3.transform (RBA::DCplxTrans::new(2.0));

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (-4,2;-8,6)", "text ('text',r0 100,200)"] )

    s3.transform (RBA::DCplxTrans::new(0.5, 0.0, false, RBA::DVector::new(0.01, 0.02)));

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (8,21;6,23)", "text ('text',r0 100,200)"] )

    s3.transform (RBA::DTrans::new(0, false, RBA::DVector::new(0.01, 0.02)));

    shapes = c1.shapes( lindex )

    arr = []
    shapes.each( RBA::Shapes::SAll ) { |s| arr.push( s.to_s ) } 
    assert_equal( arr, ["edge (18,41;16,43)", "text ('text',r0 100,200)"] )

  end

  # Shapes (edit, new methods)
  def test_7_LayoutEdit3

    ly = RBA::Layout::new(true)

    ci1 = ly.add_cell( "c1" )
    ci2 = ly.add_cell( "c2" )

    linfo = RBA::LayerInfo::new
    linfo.layer = 16
    linfo.datatype = 1
    lindex1 = ly.insert_layer( linfo )

    linfo = RBA::LayerInfo::new
    linfo.layer = 10
    linfo.datatype = 2
    lindex2 = ly.insert_layer( linfo )

    c1 = ly.cell( ci1 )
    c2 = ly.cell( ci2 )

    s1 = c1.shapes( lindex1 ).insert( RBA::Box::new( 10, -10, 50, 40 ) )
    s2 = c1.shapes( lindex1 ).insert( RBA::Box::new( 100, -10, 150, 40 ) )
    s3 = c1.shapes( lindex2 ).insert( RBA::Box::new( 200, -10, 250, 40 ) )

    assert_equal( s1.layer, lindex1 ) 
    assert_equal( s3.layer, lindex2 ) 
    assert_equal( s1.layer_info.to_s, "16/1" ) 
    assert_equal( s3.layer_info.to_s, "10/2" ) 

    str = []
    c1.shapes(lindex1).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (10,-10;50,40);box (100,-10;150,40)" )

    str = []
    c1.shapes(lindex2).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (200,-10;250,40)" )

    s3.layer = lindex1

    str = []
    c1.shapes(lindex1).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (10,-10;50,40);box (100,-10;150,40);box (200,-10;250,40)" )

    str = []
    c1.shapes(lindex2).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "" )

    begin 
      s3.layer = 17
      assert_equal( false, true )
    rescue => ex
    end

    s1.layer_info = RBA::LayerInfo::new(10, 2)

    str = []
    c1.shapes(lindex1).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (100,-10;150,40);box (200,-10;250,40)" )

    str = []
    c1.shapes(lindex2).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (10,-10;50,40)" )

    begin 
      s3.layer_info = RBA::LayerInfo("name")
      assert_equal( false, true )
    rescue => ex
    end

    s3.cell = c2

    str = []
    c1.shapes(lindex1).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (100,-10;150,40)" )

    str = []
    c1.shapes(lindex2).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (10,-10;50,40)" )

    str = []
    c2.shapes(lindex1).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (200,-10;250,40)" )

    str = []
    c2.shapes(lindex2).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "" )

    s1.cell = c2

    str = []
    c1.shapes(lindex1).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (100,-10;150,40)" )

    str = []
    c1.shapes(lindex2).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "" )

    str = []
    c2.shapes(lindex1).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (200,-10;250,40)" )

    str = []
    c2.shapes(lindex2).each { |s| str << s.to_s }
    assert_equal( str.join(";"), "box (10,-10;50,40)" )

  end

  # Shapes (edge iterator)
  def test_8

    shapes = RBA::Shapes::new

    hull =  [ RBA::Point::new(0, 0),       RBA::Point::new(6000, 0), 
              RBA::Point::new(6000, 3000), RBA::Point::new(0, 3000) ]
    hole1 = [ RBA::Point::new(1000, 1000), RBA::Point::new(2000, 1000), 
              RBA::Point::new(2000, 2000), RBA::Point::new(1000, 2000) ]
    hole2 = [ RBA::Point::new(3000, 1000), RBA::Point::new(4000, 1000), 
              RBA::Point::new(4000, 2000), RBA::Point::new(3000, 2000) ]
    poly = RBA::Polygon::new(hull)
    poly.insert_hole(hole1)
    poly.insert_hole(hole2)
    
    s = shapes.insert(poly)
    es = []
    s.each_edge { |e| es << e.to_s }
    assert_equal( es.join("/"), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)/(1000,1000;2000,1000)/(2000,1000;2000,2000)/(2000,2000;1000,2000)/(1000,2000;1000,1000)/(3000,1000;4000,1000)/(4000,1000;4000,2000)/(4000,2000;3000,2000)/(3000,2000;3000,1000)" )
    es = []
    s.each_edge(0) { |e| es << e.to_s }
    assert_equal( es.join("/"), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)" )
    es = []
    s.each_edge(1) { |e| es << e.to_s }
    assert_equal( es.join("/"), "(1000,1000;2000,1000)/(2000,1000;2000,2000)/(2000,2000;1000,2000)/(1000,2000;1000,1000)" )
    es = []
    s.each_edge(2) { |e| es << e.to_s }
    assert_equal( es.join("/"), "(3000,1000;4000,1000)/(4000,1000;4000,2000)/(4000,2000;3000,2000)/(3000,2000;3000,1000)" )
    es = []
    s.each_edge(3) { |e| es << e.to_s }
    assert_equal( es.join("/"), "" )

    shapes.clear

    poly = RBA::SimplePolygon::new(hull)

    s = shapes.insert(poly)
    es = []
    s.each_edge { |e| es << e.to_s }
    assert_equal( es.join("/"), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)" )
    es = []
    s.each_edge(0) { |e| es << e.to_s }
    assert_equal( es.join("/"), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)" )
    es = []
    s.each_edge(1) { |e| es << e.to_s }
    assert_equal( es.join("/"), "" )

  end

  # Shapes (insert from recursive shape iterator and other shape container)
  def test_9

    l = RBA::Layout.new
    l.insert_layer_at(0, RBA::LayerInfo.new(1, 0))
    l.insert_layer_at(1, RBA::LayerInfo.new(2, 0))
    c0 = l.cell(l.add_cell("c0"))
    c1 = l.cell(l.add_cell("c1"))
    c2 = l.cell(l.add_cell("c2"))
    c3 = l.cell(l.add_cell("c3"))

    tt = RBA::Trans.new
    c0.insert(RBA::CellInstArray.new(c1.cell_index, tt))
    c0.insert(RBA::CellInstArray.new(c2.cell_index, RBA::Trans.new(RBA::Point.new(100, -100))))
    c0.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(1)))
    c2.insert(RBA::CellInstArray.new(c3.cell_index, RBA::Trans.new(RBA::Point.new(1100, 0))))

    b = RBA::Box.new(0, 100, 1000, 1200)
    c0.shapes(0).insert(b)
    c1.shapes(0).insert(b)
    c2.shapes(0).insert(b)
    c3.shapes(0).insert(b)

    shapes = RBA::Shapes::new
    i1 = l.begin_shapes_touching(c0.cell_index, 0, RBA::Box.new(0, 0, 100, 100))
    shapes.insert(i1)
    assert_equal(shapes.collect { |s| s.to_s }.join(","), "box (0,100;1000,1200),box (0,100;1000,1200),box (100,0;1100,1100)")

    shapes.clear
    shapes.insert(i1, RBA::ICplxTrans::new(2.0))
    assert_equal(shapes.collect { |s| s.to_s }.join(","), "box (0,200;2000,2400),box (0,200;2000,2400),box (200,0;2200,2200)")

    shapes2 = RBA::Shapes::new
    shapes2.insert(shapes)
    assert_equal(shapes2.collect { |s| s.to_s }.join(","), "box (0,200;2000,2400),box (0,200;2000,2400),box (200,0;2200,2200)")

    shapes2.clear
    shapes2.insert(shapes, RBA::ICplxTrans::new(0.5))
    assert_equal(shapes2.collect { |s| s.to_s }.join(","), "box (0,100;1000,1200),box (0,100;1000,1200),box (100,0;1100,1100)")

    shapes2.clear
    shapes2.insert(shapes, RBA::Shapes::SPolygons, RBA::ICplxTrans::new(0.5))
    assert_equal(shapes2.collect { |s| s.to_s }.join(","), "")

    shapes2.clear
    shapes2.insert(shapes, RBA::Shapes::SAll, RBA::ICplxTrans::new(0.5))
    assert_equal(shapes2.collect { |s| s.to_s }.join(","), "box (0,100;1000,1200),box (0,100;1000,1200),box (100,0;1100,1100)")

    shapes2.clear
    shapes2.insert(shapes, RBA::Shapes::SPolygons)
    assert_equal(shapes2.collect { |s| s.to_s }.join(","), "")

    shapes2.clear
    shapes2.insert(shapes, RBA::Shapes::SAll)
    assert_equal(shapes2.collect { |s| s.to_s }.join(","), "box (0,200;2000,2400),box (0,200;2000,2400),box (200,0;2200,2200)")

  end

  # issue 152 (number of shapes count wrong in viewer mode)
  def test_10

    ly = RBA::Layout::new(true)
    ly.read(File.join($ut_testsrc, "testdata", "oasis", "issue_152.oas"))
    assert_equal(ly.top_cell.shapes(ly.layer(1, 0)).size, 200)
    ly.top_cell.shapes(ly.layer(1, 0)).insert(RBA::Polygon::new(RBA::Box::new(0, 0, 100, 100)))
    assert_equal(ly.top_cell.shapes(ly.layer(1, 0)).size, 201)

    ly = RBA::Layout::new(false)
    ly.read(File.join($ut_testsrc, "testdata", "oasis", "issue_152.oas"))
    assert_equal(ly.top_cell.shapes(ly.layer(1, 0)).size, 200)
    ly.top_cell.shapes(ly.layer(1, 0)).insert(RBA::Polygon::new(RBA::Box::new(0, 0, 100, 100)))
    assert_equal(ly.top_cell.shapes(ly.layer(1, 0)).size, 201)

  end

  # Shapes with shape-type specific insert and clear
  def test_11

    s = RBA::Shapes::new
    s.insert(RBA::Box::new(1, 2, 3, 4))
    s.insert(RBA::Polygon::new(RBA::Box::new(1, 2, 3, 4)))

    assert_equal(s.each.collect { |sh| sh.to_s }.join("; "), "polygon (1,2;1,4;3,4;3,2); box (1,2;3,4)")

    s2 = RBA::Shapes::new
    s2.insert(s)

    assert_equal(s2.each.collect { |sh| sh.to_s }.join("; "), "polygon (1,2;1,4;3,4;3,2); box (1,2;3,4)")

    s2.clear(RBA::Shapes::SPolygons)

    assert_equal(s2.each.collect { |sh| sh.to_s }.join("; "), "box (1,2;3,4)")

    s2.clear
    
    assert_equal(s2.each.collect { |sh| sh.to_s }.join("; "), "")

    s2.insert(s, RBA::Shapes::SPolygons)

    assert_equal(s2.each.collect { |sh| sh.to_s }.join("; "), "polygon (1,2;1,4;3,4;3,2)")

  end

end

load("test_epilogue.rb")

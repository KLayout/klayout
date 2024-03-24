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

class DBPolygon_TestClass < TestBase

  # DPolygon basics
  def test_1_DPolygon

    a = RBA::DPolygon::new
    assert_equal( a.to_s, "()" )
    assert_equal( RBA::DPolygon::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.is_box?, false )
    assert_equal( a.is_empty?, true )
    assert_equal( a.is_rectilinear?, false )
    assert_equal( a.is_halfmanhattan?, false )

    b = a.dup 
    a = RBA::DPolygon::new( [ RBA::DPoint::new( 0, 1 ), RBA::DPoint::new( 1, 5 ), RBA::DPoint::new( 5, 5 ) ] )
    assert_equal( a.to_s, "(0,1;1,5;5,5)" )
    assert_equal( (a * 2).to_s, "(0,2;2,10;10,10)" )
    assert_equal( RBA::DPolygon::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.is_box?, false )
    assert_equal( a.num_points_hull, 3 )
    assert_equal( a.is_empty?, false )
    assert_equal( a.is_rectilinear?, false )
    assert_equal( a.is_halfmanhattan?, false )
    c = a.dup 

    assert_equal( a == b, false )
    assert_equal( a == c, true )
    assert_equal( a != b, true )
    assert_equal( a != c, false )

    a = RBA::DPolygon::new( RBA::DBox::new( 5, -10, 20, 15 ) )
    assert_equal( a.is_box?, true )
    assert_equal( a.is_empty?, false )
    assert_equal( a.is_rectilinear?, true )
    assert_equal( a.is_halfmanhattan?, true )
    assert_equal( a.to_s, "(5,-10;5,15;20,15;20,-10)" )
    assert_equal( RBA::Polygon::new(a).to_s, "(5,-10;5,15;20,15;20,-10)" )
    assert_equal( a.num_points_hull, 4 )
    assert_equal( a.area, 15*25 )
    assert_equal( a.perimeter, 80 )
    assert_equal( a.inside( RBA::DPoint::new( 10, 0 ) ), true )
    assert_equal( a.inside( RBA::DPoint::new( 5, 0 ) ), true )
    assert_equal( a.inside( RBA::DPoint::new( 30, 0 ) ), false )

    arr = []
    a.each_point_hull { |p| arr.push( p.to_s ) }
    assert_equal( arr, ["5,-10", "5,15", "20,15", "20,-10"] )
    # with enumerator
    assert_equal( a.each_point_hull.collect(&:to_s), ["5,-10", "5,15", "20,15", "20,-10"] )

    b = a.dup

    assert_equal( a.moved( RBA::DPoint::new( 0, 1 ) ).to_s, "(5,-9;5,16;20,16;20,-9)" )
    assert_equal( a.moved( 0, 1 ).to_s, "(5,-9;5,16;20,16;20,-9)" )
    aa = a.dup
    aa.move( 1, 0 )
    assert_equal( aa.to_s, "(6,-10;6,15;21,15;21,-10)" )
    a.move( RBA::DPoint::new( 1, 0 ) )
    assert_equal( a.to_s, "(6,-10;6,15;21,15;21,-10)" )

    b = b.transformed( RBA::DTrans::new( RBA::DTrans::R0, RBA::DPoint::new( 1, 0 )) )
    assert_equal( b.to_s, "(6,-10;6,15;21,15;21,-10)" )

    m = RBA::DCplxTrans::new( RBA::DTrans::new, 1.5 )
    assert_equal( a.transformed(m).class.to_s, "RBA::DPolygon" )
    assert_equal( a.transformed(m).to_s, "(9,-15;9,22.5;31.5,22.5;31.5,-15)" )

    m = RBA::VCplxTrans::new( 1000.0 )
    assert_equal( a.transformed(m).class.to_s, "RBA::Polygon" )
    assert_equal( a.transformed(m).to_s, "(6000,-10000;6000,15000;21000,15000;21000,-10000)" )

    a.hull = [ RBA::DPoint::new( 0, 1 ), RBA::DPoint::new( 1, 1 ), RBA::DPoint::new( 1, 5 ) ]
    assert_equal( a.bbox.to_s, "(0,1;1,5)" )

    assert_equal( a.holes, 0 )
    a.insert_hole( [ RBA::DPoint::new( 1, 2 ), RBA::DPoint::new( 2, 2 ), RBA::DPoint::new( 2, 6 ) ] )
    assert_equal( a.to_s, "(0,1;1,5;1,1/1,2;2,2;2,6)" )
    assert_equal( RBA::DPolygon::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.area, 0 )
    assert_equal( a.num_points_hole(0), 3 )
    assert_equal( a.holes, 1 )
    assert_equal( a.point_hull(1).to_s, "1,5" )
    assert_equal( a.point_hull(0).to_s, "0,1" )
    assert_equal( a.point_hull(100).to_s, "0,0" )
    assert_equal( a.point_hole(0, 100).to_s, "0,0" )
    assert_equal( a.point_hole(0, 1).to_s, "2,2" )
    assert_equal( a.point_hole(1, 1).to_s, "0,0" )
    a.compress(false);
    assert_equal( a.to_s, "(0,1;1,5;1,1/1,2;2,2;2,6)" )
    a.compress(true);
    assert_equal( a.to_s, "(0,1;1,5;1,1/1,2;2,2;2,6)" )

    b = a.dup
    b.assign_hole(0, RBA::DBox::new( 10, 20, 20, 60 ))
    assert_equal( b.to_s, "(0,1;1,5;1,1/10,20;20,20;20,60;10,60)" )
    b.insert_hole(RBA::DBox::new( 10, 20, 20, 60 ))
    assert_equal( b.to_s, "(0,1;1,5;1,1/10,20;20,20;20,60;10,60/10,20;20,20;20,60;10,60)" )
    assert_equal( b.is_box?, false )

    b = a.dup
    b.assign_hole(0, [ RBA::DPoint::new( 10, 20 ), RBA::DPoint::new( 20, 20 ), RBA::DPoint::new( 20, 60 ) ])
    assert_equal( b.to_s, "(0,1;1,5;1,1/10,20;20,20;20,60)" )
    b.assign_hole(1, [ RBA::DPoint::new( 15, 25 ), RBA::DPoint::new( 25, 25 ), RBA::DPoint::new( 25, 65 ) ])
    assert_equal( b.to_s, "(0,1;1,5;1,1/10,20;20,20;20,60)" )
    b.insert_hole( [ RBA::DPoint::new( 1, 2 ), RBA::DPoint::new( 2, 2 ), RBA::DPoint::new( 2, 6 ) ] )
    assert_equal( b.to_s, "(0,1;1,5;1,1/10,20;20,20;20,60/1,2;2,2;2,6)" )
    b.sort_holes
    assert_equal( b.to_s, "(0,1;1,5;1,1/1,2;2,2;2,6/10,20;20,20;20,60)" )
    b.assign_hole(0, [ RBA::DPoint::new( 15, 25 ), RBA::DPoint::new( 25, 25 ), RBA::DPoint::new( 25, 65 ) ])
    assert_equal( b.to_s, "(0,1;1,5;1,1/15,25;25,25;25,65/10,20;20,20;20,60)" )

    arr = []
    a.each_point_hole(0) { |p| arr.push( p.to_s ) }
    assert_equal( arr, ["1,2", "2,2", "2,6"] )
    # with enumerator
    assert_equal( a.each_point_hole(0).collect(&:to_s), ["1,2", "2,2", "2,6"] )

    arr = []
    a.each_edge { |p| arr.push( p.to_s ) }
    assert_equal( arr, ["(0,1;1,5)", "(1,5;1,1)", "(1,1;0,1)", "(1,2;2,2)", "(2,2;2,6)", "(2,6;1,2)"] )

    # Ellipse constructor
    p = RBA::DPolygon::ellipse( RBA::DBox::new(-10000, -20000, 30000, 40000), 200 )
    assert_equal(p.num_points, 200)
    assert_equal(p.bbox.to_s, "(-10000,-20000;30000,40000)")
    assert_equal(p.area.to_i, 1884645544)    # roughly box.area*PI/4
    
    p = RBA::DPolygon::ellipse( RBA::DBox::new(-10000, -20000, 30000, 40000), 4 )
    assert_equal(p.to_s, "(10000,-20000;-10000,10000;10000,40000;30000,10000)")

    # halfmanhattan variants
    p = RBA::DPolygon::new([ RBA::DPoint::new( 0, 0 ), RBA::DPoint::new( 0, 100 ), RBA::DPoint::new( 100, 100 ) ])
    assert_equal(p.is_halfmanhattan?, true)
    assert_equal(p.is_rectilinear?, false)
    p = RBA::DPolygon::new([ RBA::DPoint::new( 0, 0 ), RBA::DPoint::new( 0, 100 ), RBA::DPoint::new( 100, 101 ) ])
    assert_equal(p.is_halfmanhattan?, false)
    assert_equal(p.is_rectilinear?, false)
    p = RBA::DPolygon::new([ RBA::DPoint::new( 0, 0 ), RBA::DPoint::new( 0, 100 ), RBA::DPoint::new( 100, 100 ), RBA::DPoint::new( 100, 0) ])
    assert_equal(p.is_halfmanhattan?, true)
    assert_equal(p.is_rectilinear?, true)

  end

  # Polygon basics
  def test_1_Polygon

    a = RBA::Polygon::new
    assert_equal( a.to_s, "()" )
    assert_equal( RBA::Polygon::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.is_box?, false )

    b = a.dup 
    a = RBA::Polygon::new( [ RBA::Point::new( 0, 1 ), RBA::Point::new( 1, 5 ), RBA::Point::new( 5, 5 ) ] )
    assert_equal( a.to_s, "(0,1;1,5;5,5)" )
    assert_equal( (a * 2).to_s, "(0,2;2,10;10,10)" )
    assert_equal( RBA::Polygon::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.num_points_hull, 3 )
    c = a.dup 

    assert_equal( a == b, false )
    assert_equal( a == c, true )
    assert_equal( a != b, true )
    assert_equal( a != c, false )

    a = RBA::Polygon::new( RBA::Box::new( 5, -10, 20, 15 ) )
    assert_equal( a.is_box?, true )
    assert_equal( a.to_s, "(5,-10;5,15;20,15;20,-10)" )
    assert_equal( RBA::DPolygon::new(a).to_s, "(5,-10;5,15;20,15;20,-10)" )
    assert_equal( a.num_points_hull, 4 )
    assert_equal( a.area, 15*25 )
    assert_equal( a.perimeter, 80 )
    assert_equal( a.inside( RBA::Point::new( 10, 0 ) ), true )
    assert_equal( a.inside( RBA::Point::new( 5, 0 ) ), true )
    assert_equal( a.inside( RBA::Point::new( 30, 0 ) ), false )

    arr = []
    a.each_point_hull { |p| arr.push( p.to_s ) }
    assert_equal( arr, ["5,-10", "5,15", "20,15", "20,-10"] )

    b = a.dup

    assert_equal( a.moved( RBA::Point::new( 0, 1 ) ).to_s, "(5,-9;5,16;20,16;20,-9)" )
    assert_equal( a.moved( 0, 1 ).to_s, "(5,-9;5,16;20,16;20,-9)" )
    aa = a.dup
    aa.move( 1, 0 )
    assert_equal( aa.to_s, "(6,-10;6,15;21,15;21,-10)" )
    a.move( RBA::Point::new( 1, 0 ) )
    assert_equal( a.to_s, "(6,-10;6,15;21,15;21,-10)" )

    b = b.transformed( RBA::Trans::new( RBA::Trans::R0, RBA::Point::new( 1, 0 )) )
    assert_equal( b.to_s, "(6,-10;6,15;21,15;21,-10)" )

    m = RBA::CplxTrans::new( RBA::Trans::new, 1.5 )
    assert_equal( a.transformed(m).to_s, "(9,-15;9,22.5;31.5,22.5;31.5,-15)" )
    assert_equal( a.transformed(RBA::ICplxTrans::new(m)).to_s, "(9,-15;9,23;32,23;32,-15)" )

    a.hull = [ RBA::Point::new( 0, 1 ), RBA::Point::new( 1, 1 ), RBA::Point::new( 1, 5 ) ]
    assert_equal( a.bbox.to_s, "(0,1;1,5)" )

    assert_equal( a.holes, 0 )
    a.insert_hole( [ RBA::Point::new( 1, 2 ), RBA::Point::new( 2, 2 ), RBA::Point::new( 2, 6 ) ] )
    assert_equal( a.to_s, "(0,1;1,5;1,1/1,2;2,2;2,6)" )
    assert_equal( RBA::Polygon::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.area, 0 )
    assert_equal( a.num_points_hole(0), 3 )
    assert_equal( a.holes, 1 )
    assert_equal( a.point_hull(1).to_s, "1,5" )
    assert_equal( a.point_hull(0).to_s, "0,1" )
    assert_equal( a.point_hull(100).to_s, "0,0" )
    assert_equal( a.point_hole(0, 100).to_s, "0,0" )
    assert_equal( a.point_hole(0, 1).to_s, "2,2" )
    assert_equal( a.point_hole(1, 1).to_s, "0,0" )
    a.compress(false);
    assert_equal( a.to_s, "(0,1;1,5;1,1/1,2;2,2;2,6)" )
    a.compress(true);
    assert_equal( a.to_s, "(0,1;1,5;1,1/1,2;2,2;2,6)" )

    b = a.dup
    b.assign_hole(0, RBA::Box::new( 10, 20, 20, 60 ))
    assert_equal( b.to_s, "(0,1;1,5;1,1/10,20;20,20;20,60;10,60)" )
    assert_equal( b.is_box?, false )
    b.insert_hole(RBA::Box::new( 10, 20, 20, 60 ))
    assert_equal( b.to_s, "(0,1;1,5;1,1/10,20;20,20;20,60;10,60/10,20;20,20;20,60;10,60)" )

    b = a.dup
    b.assign_hole(0, [ RBA::Point::new( 10, 20 ), RBA::Point::new( 20, 20 ), RBA::Point::new( 20, 60 ) ])
    assert_equal( b.to_s, "(0,1;1,5;1,1/10,20;20,20;20,60)" )
    b.assign_hole(1, [ RBA::Point::new( 15, 25 ), RBA::Point::new( 25, 25 ), RBA::Point::new( 25, 65 ) ])
    assert_equal( b.to_s, "(0,1;1,5;1,1/10,20;20,20;20,60)" )
    b.insert_hole( [ RBA::Point::new( 1, 2 ), RBA::Point::new( 2, 2 ), RBA::Point::new( 2, 6 ) ] )
    assert_equal( b.to_s, "(0,1;1,5;1,1/10,20;20,20;20,60/1,2;2,2;2,6)" )
    b.sort_holes
    assert_equal( b.to_s, "(0,1;1,5;1,1/1,2;2,2;2,6/10,20;20,20;20,60)" )
    b.assign_hole(0, [ RBA::Point::new( 15, 25 ), RBA::Point::new( 25, 25 ), RBA::Point::new( 25, 65 ) ])
    assert_equal( b.to_s, "(0,1;1,5;1,1/15,25;25,25;25,65/10,20;20,20;20,60)" )

    arr = []
    a.each_point_hole(0) { |p| arr.push( p.to_s ) }
    assert_equal( arr, ["1,2", "2,2", "2,6"] )

    arr = []
    a.each_edge { |p| arr.push( p.to_s ) }
    assert_equal( arr, ["(0,1;1,5)", "(1,5;1,1)", "(1,1;0,1)", "(1,2;2,2)", "(2,2;2,6)", "(2,6;1,2)"] )

    a = RBA::Polygon::new( [ RBA::Point::new( 0, 1 ), RBA::Point::new( 1, 5 ), RBA::Point::new( 5, 5 ) ] )
    assert_equal( a.to_s, "(0,1;1,5;5,5)" )
    assert_equal( a.sized(2).to_s, "(0,-2;-2,0;-1,7;7,7;8,5)" )
    assert_equal( a.sized(2, 2).to_s, "(0,-2;-2,0;-1,7;7,7;8,5)" )
    aa = a.dup
    a.size(2, 2);
    assert_equal( a.to_s, "(0,-2;-2,0;-1,7;7,7;8,5)" )
    a = aa.dup
    a.size(2);
    assert_equal( a.to_s, "(0,-2;-2,0;-1,7;7,7;8,5)" )

    a = RBA::Polygon::new( [ RBA::Point::new( 0, 1 ), RBA::Point::new( 1, 5 ), RBA::Point::new( 5, 5 ) ] )
    assert_equal( a.to_s, "(0,1;1,5;5,5)" )
    assert_equal( a.sized(2, 0, 2).to_s, "(-2,1;-1,5;7,5;2,1)" )
    assert_equal( a.sized(RBA::Vector::new(2, 0), 2).to_s, "(-2,1;-1,5;7,5;2,1)" )
    assert_equal( a.sized(RBA::Vector::new(2, 0)).to_s, "(-2,1;-1,5;7,5;2,1)" )
    aa = a.dup
    a.size(2, 0, 2);
    assert_equal( a.to_s, "(-2,1;-1,5;7,5;2,1)" )
    a = aa
    aa = a.dup
    a.size(RBA::Vector::new(2, 0), 2);
    assert_equal( a.to_s, "(-2,1;-1,5;7,5;2,1)" )
    a = aa
    aa = a.dup
    a.size(RBA::Vector::new(2, 0));
    assert_equal( a.to_s, "(-2,1;-1,5;7,5;2,1)" )

    a = RBA::Polygon::new
    assert_equal( a.to_s, "()" )

    # corner rounding
    a = RBA::Polygon::new( [ RBA::Point::new(0, 0), RBA::Point::new(0, 2000), RBA::Point::new(4000, 2000),
                             RBA::Point::new(4000, 1000), RBA::Point::new(2000, 1000), RBA::Point::new(2000, 0) ] )
    ar = a.round_corners(100, 200, 8)
    assert_equal( ar.to_s, "(117,0;0,117;0,1883;117,2000;3883,2000;4000,1883;4000,1117;3883,1000;2059,1000;2000,941;2000,117;1883,0)" )
    ar = a.round_corners(200, 100, 32)
    assert_equal( ar.to_s, "(90,0;71,4;53,11;36,22;22,36;11,53;4,71;0,90;0,1910;4,1929;11,1947;22,1964;36,1978;53,1989;71,1996;90,2000;3910,2000;3929,1996;3947,1989;3964,1978;3978,1964;3989,1947;3996,1929;4000,1910;4000,1090;3996,1071;3989,1053;3978,1036;3964,1022;3947,1011;3929,1004;3910,1000;2180,1000;2142,992;2105,977;2073,955;2045,927;2023,895;2008,858;2000,820;2000,90;1996,71;1989,53;1978,36;1964,22;1947,11;1929,4;1910,0)" )

    # Minkowsky sums
    p = RBA::Polygon::new( [ RBA::Point.new(0, -100), RBA::Point.new(0, -50), RBA::Point.new(-100, -75), RBA::Point.new(0, 100), RBA::Point.new(50, 50), RBA::Point.new(100, 75), RBA::Point.new(100, 0), RBA::Point.new(100, -50) ] )
    assert_equal(p.minkowsky_sum(RBA::Edge.new(RBA::Point.new(10, 10), RBA::Point.new(210, 110)), true).to_s, "(10,-90;10,-40;-90,-65;10,110;210,210;260,160;310,185;310,60)")
    assert_equal(p.minkowsky_sum([RBA::Point.new(10, 10), RBA::Point.new(10, 310), RBA::Point.new(510, 310), RBA::Point.new(510, 10), RBA::Point.new(10, 10) ], false).to_s, "(10,-90;10,-65;-90,-65;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90/110,110;410,110;410,210;110,210)")
    assert_equal(p.minkowsky_sum([RBA::Point.new(10, 10), RBA::Point.new(10, 310), RBA::Point.new(510, 310), RBA::Point.new(510, 10), RBA::Point.new(10, 10) ], true).to_s, "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)")
    assert_equal(p.minkowsky_sum(RBA::Box.new(RBA::Point.new(10, 10), RBA::Point.new(210, 110)), true).to_s, "(10,-90;10,-65;-90,-65;-90,35;10,210;210,210;235,185;310,185;310,-40;210,-90)")
    assert_equal(p.minkowsky_sum(RBA::Box.new(RBA::Point.new(10, 10), RBA::Point.new(210, 10)), true).to_s, "(10,-90;10,-65;-90,-65;10,110;210,110;235,85;310,85;310,-40;210,-90)")
    assert_equal(p.minkowsky_sum(RBA::Polygon.new(RBA::Box.new(RBA::Point.new(10, 10), RBA::Point.new(210, 110))), true).to_s, "(10,-90;10,-65;-90,-65;-90,35;10,210;210,210;235,185;310,185;310,-40;210,-90)")

    # Smoothing
    p = RBA::Polygon::new( [ RBA::Point.new(0, 0), RBA::Point.new(10, 50), RBA::Point.new(0, 100), RBA::Point.new(200, 100), RBA::Point.new(200, 0) ])
    assert_equal(p.smooth(5).to_s, "(0,0;10,50;0,100;200,100;200,0)")
    assert_equal(p.smooth(15).to_s, "(0,0;0,100;200,100;200,0)")
    p = RBA::Polygon::new( [ RBA::Point.new(0, 0), RBA::Point.new(10, 50), RBA::Point.new(10, 100), RBA::Point.new(200, 100), RBA::Point.new(200, 0) ])
    assert_equal(p.smooth(15, false).to_s, "(0,0;10,100;200,100;200,0)")
    assert_equal(p.smooth(15, true).to_s, "(0,0;10,50;10,100;200,100;200,0)")

    # Ellipse constructor
    p = RBA::Polygon::ellipse( RBA::Box::new(-10000, -20000, 30000, 40000), 200 )
    assert_equal(p.num_points, 200)
    assert_equal(p.bbox.to_s, "(-10000,-20000;30000,40000)")
    assert_equal(p.area, 1884651158)    # roughly box.area*PI/4
    
    p = RBA::Polygon::ellipse( RBA::Box::new(-10000, -20000, 30000, 40000), 4 )
    assert_equal(p.to_s, "(10000,-20000;-10000,10000;10000,40000;30000,10000)")

    # halfmanhattan variants
    p = RBA::Polygon::new([ RBA::Point::new( 0, 0 ), RBA::Point::new( 0, 100 ), RBA::Point::new( 100, 100 ) ])
    assert_equal(p.is_halfmanhattan?, true)
    assert_equal(p.is_rectilinear?, false)
    p = RBA::Polygon::new([ RBA::Point::new( 0, 0 ), RBA::Point::new( 0, 100 ), RBA::Point::new( 100, 101 ) ])
    assert_equal(p.is_halfmanhattan?, false)
    assert_equal(p.is_rectilinear?, false)
    p = RBA::Polygon::new([ RBA::Point::new( 0, 0 ), RBA::Point::new( 0, 100 ), RBA::Point::new( 100, 100 ), RBA::Point::new( 100, 0) ])
    assert_equal(p.is_halfmanhattan?, true)
    assert_equal(p.is_rectilinear?, true)

  end

  # Polygon parametrized edge iterator
  def test_2

    hull =  [ RBA::Point::new(0, 0),       RBA::Point::new(6000, 0), 
              RBA::Point::new(6000, 3000), RBA::Point::new(0, 3000) ]
    hole1 = [ RBA::Point::new(1000, 1000), RBA::Point::new(2000, 1000), 
              RBA::Point::new(2000, 2000), RBA::Point::new(1000, 2000) ]
    hole2 = [ RBA::Point::new(3000, 1000), RBA::Point::new(4000, 1000), 
              RBA::Point::new(4000, 2000), RBA::Point::new(3000, 2000) ]
    poly = RBA::Polygon::new(hull)
    poly.insert_hole(hole1)
    poly.insert_hole(hole2)
    
    es = []
    poly.each_edge { |e| es << e.to_s }
    assert_equal( es.join("/"), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)/(1000,1000;2000,1000)/(2000,1000;2000,2000)/(2000,2000;1000,2000)/(1000,2000;1000,1000)/(3000,1000;4000,1000)/(4000,1000;4000,2000)/(4000,2000;3000,2000)/(3000,2000;3000,1000)" )
    es = []
    poly.each_edge(0) { |e| es << e.to_s }
    assert_equal( es.join("/"), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)" )
    es = []
    poly.each_edge(1) { |e| es << e.to_s }
    assert_equal( es.join("/"), "(1000,1000;2000,1000)/(2000,1000;2000,2000)/(2000,2000;1000,2000)/(1000,2000;1000,1000)" )
    es = []
    poly.each_edge(2) { |e| es << e.to_s }
    assert_equal( es.join("/"), "(3000,1000;4000,1000)/(4000,1000;4000,2000)/(4000,2000;3000,2000)/(3000,2000;3000,1000)" )
    es = []
    poly.each_edge(3) { |e| es << e.to_s }
    assert_equal( es.join("/"), "" )

    hull =  [ RBA::DPoint::new(0, 0),       RBA::DPoint::new(6000, 0), 
              RBA::DPoint::new(6000, 3000), RBA::DPoint::new(0, 3000) ]
    hole1 = [ RBA::DPoint::new(1000, 1000), RBA::DPoint::new(2000, 1000), 
              RBA::DPoint::new(2000, 2000), RBA::DPoint::new(1000, 2000) ]
    hole2 = [ RBA::DPoint::new(3000, 1000), RBA::DPoint::new(4000, 1000), 
              RBA::DPoint::new(4000, 2000), RBA::DPoint::new(3000, 2000) ]
    poly = RBA::DPolygon::new(hull)
    poly.insert_hole(hole1)
    poly.insert_hole(hole2)
    
    es = []
    poly.each_edge { |e| es << e.to_s }
    assert_equal( es.join("/"), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)/(1000,1000;2000,1000)/(2000,1000;2000,2000)/(2000,2000;1000,2000)/(1000,2000;1000,1000)/(3000,1000;4000,1000)/(4000,1000;4000,2000)/(4000,2000;3000,2000)/(3000,2000;3000,1000)" )
    es = []
    poly.each_edge(0) { |e| es << e.to_s }
    assert_equal( es.join("/"), "(0,0;0,3000)/(0,3000;6000,3000)/(6000,3000;6000,0)/(6000,0;0,0)" )
    es = []
    poly.each_edge(1) { |e| es << e.to_s }
    assert_equal( es.join("/"), "(1000,1000;2000,1000)/(2000,1000;2000,2000)/(2000,2000;1000,2000)/(1000,2000;1000,1000)" )
    es = []
    poly.each_edge(2) { |e| es << e.to_s }
    assert_equal( es.join("/"), "(3000,1000;4000,1000)/(4000,1000;4000,2000)/(4000,2000;3000,2000)/(3000,2000;3000,1000)" )
    es = []
    poly.each_edge(3) { |e| es << e.to_s }
    assert_equal( es.join("/"), "" )

  end

  # raw mode polygons
  def test_2_Polygon

    pts = [ RBA::Point::new(0, 0) ]
    p = RBA::Polygon::new(pts, false)
    assert_equal(p.to_s, "()")
    assert_equal(p.is_empty?, true)
    assert_equal(p.is_rectilinear?, false)
    assert_equal(p.is_halfmanhattan?, false)
    
    pts = [ RBA::Point::new(0, 0) ]
    p = RBA::Polygon::new(pts)
    assert_equal(p.to_s, "()")
    
    pts = [ RBA::Point::new(0, 0) ]
    p = RBA::Polygon::new(pts, true)
    assert_equal(p.to_s, "(0,0)")

    arr = []
    p.each_edge { |e| arr.push( e.to_s ) }
    assert_equal( arr, ["(0,0;0,0)"] )
    
    p = RBA::Polygon::new(RBA::Box::new(0, 0, 100, 100))
    assert_equal(p.to_s, "(0,0;0,100;100,100;100,0)")
    p.insert_hole( [ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ] )
    # TODO: this isn't nice (empty hole):
    assert_equal(p.to_s, "(0,0;0,100;100,100;100,0/)")

    p = RBA::Polygon::new(RBA::Box::new(0, 0, 100, 100))
    p.insert_hole( [ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ], true )
    assert_equal(p.to_s, "(0,0;0,100;100,100;100,0/0,0;10,0)")
    p.assign_hole(0, [ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ] )
    assert_equal(p.to_s, "(0,0;0,100;100,100;100,0/)")
    p.assign_hole(0, [ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ], true )
    assert_equal(p.to_s, "(0,0;0,100;100,100;100,0/0,0;10,0)")

    pts = [ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ]
    p = RBA::Polygon::new(pts, true)
    assert_equal(p.to_s, "(0,0;10,0)")
    # conversion of degenerated polygon to simple polygon is not supported currently:
    assert_equal(p.to_simple_polygon.to_s, "()")
    assert_equal(RBA::DPolygon::new(p).to_s, "(0,0;10,0)")

    p.hull = []
    assert_equal(p.to_s, "()")
    
    p.hull = [ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ]
    assert_equal(p.to_s, "(0,0;10,0)")

    p.assign_hull([ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ], false)
    assert_equal(p.to_s, "()")

    p.assign_hull([ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ], true)
    assert_equal(p.to_s, "(0,0;10,0)")

    arr = []
    p.each_edge { |e| arr.push( e.to_s ) }
    assert_equal( arr, ["(0,0;10,0)", "(10,0;0,0)"] )
    
    assert_equal(p.moved(1, 2).to_s, "(1,2;11,2)")
    assert_equal(p.sized(2).to_s, "(0,-2;0,2;10,2;10,-2)")
    assert_equal((p * 2).to_s, "(0,0;20,0)")
    assert_equal(p.transformed(RBA::Trans::new(RBA::Trans::R90)).to_s, "(0,0;0,10)")

    pp = p.dup
    pp.transform(RBA::Trans::new(RBA::Trans::R90))
    assert_equal(pp.to_s, "(0,0;0,10)")
    
    p = RBA::Polygon::new([ RBA::Point::new(0, 0), RBA::Point::new(0, 10) ], true)
    q = RBA::Polygon::new([ RBA::Point::new(1, 1), RBA::Point::new(-9, 1) ], true)
    assert_equal(p.minkowsky_sum(q, false).to_s, "(-9,1;-9,11;1,11;1,1)")
    
  end

  # raw mode polygons
  def test_2_DPolygon

    pts = [ RBA::DPoint::new(0, 0) ]
    p = RBA::DPolygon::new(pts, true)
    assert_equal(p.to_s, "(0,0)")
    assert_equal(p.is_empty?, false)
    assert_equal(p.is_rectilinear?, false)

    arr = []
    p.each_edge { |e| arr.push( e.to_s ) }
    assert_equal( arr, ["(0,0;0,0)"] )
    
    p = RBA::DPolygon::new(RBA::DBox::new(0, 0, 100, 100))
    assert_equal(p.is_empty?, false)
    assert_equal(p.is_rectilinear?, true)
    p.insert_hole([ RBA::DPoint::new(0, 0), RBA::DPoint::new(10, 0) ], true)
    assert_equal(p.to_s, "(0,0;0,100;100,100;100,0/0,0;10,0)")
    p.assign_hole(0, [ RBA::DPoint::new(0, 0), RBA::DPoint::new(10, 0) ])
    assert_equal(p.to_s, "(0,0;0,100;100,100;100,0/0,0;10,0)")
    p.assign_hole(0, [ RBA::DPoint::new(0, 0), RBA::DPoint::new(10, 0) ], true)
    assert_equal(p.to_s, "(0,0;0,100;100,100;100,0/0,0;10,0)")

    pts = [ RBA::DPoint::new(0, 0), RBA::DPoint::new(10, 0) ]
    p = RBA::DPolygon::new(pts, true)
    assert_equal(p.to_s, "(0,0;10,0)")
    assert_equal(RBA::Polygon::new(p).to_s, "(0,0;10,0)")

    p.hull = []
    assert_equal(p.to_s, "()")
    
    p.hull = [ RBA::DPoint::new(0, 0), RBA::DPoint::new(10, 0) ]
    assert_equal(p.to_s, "(0,0;10,0)")

    p.assign_hull([ RBA::DPoint::new(0, 0), RBA::DPoint::new(10, 0) ], true)
    assert_equal(p.to_s, "(0,0;10,0)")

    arr = []
    p.each_edge { |e| arr.push( e.to_s ) }
    assert_equal( arr, ["(0,0;10,0)", "(10,0;0,0)"] )
    
    assert_equal(p.moved(1, 2).to_s, "(1,2;11,2)")
    assert_equal(p.sized(2).to_s, "(0,-2;0,2;10,2;10,-2)")
    assert_equal((p * 2).to_s, "(0,0;20,0)")
    assert_equal(p.transformed(RBA::DTrans::new(RBA::DTrans::R90)).to_s, "(0,0;0,10)")

    pp = p.dup
    pp.transform(RBA::DTrans::new(RBA::DTrans::R90))
    assert_equal(pp.to_s, "(0,0;0,10)")
    
  end

  # is_convex
  def test_IsConvex

    assert_equal(RBA::Polygon::new(RBA::Box::new(0, 0, 10, 10)).is_convex?, true)

    p = RBA::Polygon::from_s("(0,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    assert_equal(p.is_convex?, false)

  end

  # polygon decomposition
  def test_PolygonDecompose

    p = RBA::Polygon::from_s("(0,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")

    assert_equal(p.decompose_convex.map(&:to_s), %w((0,10;0,30;10,30;10,10) (0,30;0,40;30,40;30,30) (30,10;30,40;40,40;40,10) (0,0;0,10;40,10;40,0)))
    assert_equal(p.decompose_convex(RBA::Polygon::PO_any).map(&:to_s), %w((0,10;0,30;10,30;10,10) (0,30;0,40;30,40;30,30) (30,10;30,40;40,40;40,10) (0,0;0,10;40,10;40,0)))
    assert_equal(p.decompose_convex(RBA::Polygon::PO_horizontal).map(&:to_s), %w((0,10;0,30;10,30;10,10) (0,30;0,40;40,40;40,30) (30,10;30,30;40,30;40,10) (0,0;0,10;40,10;40,0)))
    assert_equal(p.decompose_convex(RBA::Polygon::PO_vertical).map(&:to_s), %w((10,0;10,10;30,10;30,0) (0,0;0,40;10,40;10,0) (10,30;10,40;30,40;30,30) (30,0;30,40;40,40;40,0)))
    assert_equal(p.decompose_convex(RBA::Polygon::PO_htrapezoids).map(&:to_s), %w((0,10;0,30;10,30;10,10) (0,30;0,40;30,40;30,30) (30,10;30,40;40,40;40,10) (0,0;0,10;40,10;40,0)))
    assert_equal(p.decompose_convex(RBA::Polygon::PO_vtrapezoids).map(&:to_s), %w((10,0;10,10;30,10;30,0) (0,0;0,30;10,30;10,0) (0,30;0,40;30,40;30,30) (30,0;30,40;40,40;40,0)))

    assert_equal(p.decompose_trapezoids.map(&:to_s), %w((0,0;0,10;40,10;40,0) (0,10;0,30;10,30;10,10) (30,10;30,30;40,30;40,10) (0,30;0,40;40,40;40,30)))
    assert_equal(p.decompose_trapezoids(RBA::Polygon::TD_simple).map(&:to_s), %w((0,0;0,10;40,10;40,0) (0,10;0,30;10,30;10,10) (30,10;30,30;40,30;40,10) (0,30;0,40;40,40;40,30)))
    assert_equal(p.decompose_trapezoids(RBA::Polygon::TD_htrapezoids).map(&:to_s), %w((0,10;0,30;10,30;10,10) (0,30;0,40;30,40;30,30) (30,10;30,40;40,40;40,10) (0,0;0,10;40,10;40,0)))
    assert_equal(p.decompose_trapezoids(RBA::Polygon::TD_vtrapezoids).map(&:to_s), %w((10,0;10,10;30,10;30,0) (0,0;0,30;10,30;10,0) (0,30;0,40;30,40;30,30) (30,0;30,40;40,40;40,0)))

  end

  # polygon decomposition
  def test_extractRad

    ex = RBA::SimplePolygon::new.extract_rad
    assert_equal(ex.inspect, "[]")

    sp = RBA::SimplePolygon::from_s("(0,0;0,200000;300000,200000;300000,100000;100000,100000;100000,0)")

    sp = sp.round_corners(10000, 5000, 200)
    ex = sp.extract_rad

    assert_equal(ex.map(&:to_s), %w((0,0;0,200000;300000,200000;300000,100000;100000,100000;100000,0) 10000.0 5000.0 200))

    ex = RBA::Polygon::new.extract_rad
    assert_equal(ex.inspect, "[]")

    sp = RBA::Polygon::from_s("(0,0;0,300000;300000,300000;300000,0/100000,100000;200000,100000;200000,200000;100000,200000)")

    sp = sp.round_corners(10000, 5000, 200)
    ex = sp.extract_rad

    assert_equal(ex.map(&:to_s), %w((0,0;0,300000;300000,300000;300000,0/100000,100000;200000,100000;200000,200000;100000,200000) 10000.0 5000.0 200))

    # double coords too ...

    ex = RBA::DSimplePolygon::new.extract_rad
    assert_equal(ex.inspect, "[]")

    sp = RBA::DSimplePolygon::from_s("(0,0;0,200000;300000,200000;300000,100000;100000,100000;100000,0)")

    sp = sp.round_corners(10000, 5000, 200)
    ex = sp.extract_rad

    # round to integers for better comparison
    
    ex[0] = RBA::SimplePolygon::new(ex[0])
    assert_equal(ex.map(&:to_s), %w((0,0;0,200000;300000,200000;300000,100000;100000,100000;100000,0) 10000.0 5000.0 200))

    ex = RBA::DPolygon::new.extract_rad
    assert_equal(ex.inspect, "[]")

    sp = RBA::DPolygon::from_s("(0,0;0,300000;300000,300000;300000,0/100000,100000;200000,100000;200000,200000;100000,200000)")

    sp = sp.round_corners(10000, 5000, 200)
    ex = sp.extract_rad

    # round to integers for better comparison
    ex[0] = RBA::Polygon::new(ex[0])

    assert_equal(ex.map(&:to_s), %w((0,0;0,300000;300000,300000;300000,0/100000,100000;200000,100000;200000,200000;100000,200000) 10000.0 5000.0 200))

  end

  # fuzzy compare 
  def test_FuzzyCompare

    p1 = RBA::DPolygon::from_s("(0,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p2a = RBA::DPolygon::from_s("(0.0000001,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p2b = RBA::DPolygon::from_s("(0,0;0,40;40,40;40,0/10.0000001,10;30,10;30,30;10,30)")
    p3a = RBA::DPolygon::from_s("(0.0001,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p3b = RBA::DPolygon::from_s("(0,0;0,40;40,40;40,0/10.0001,10;30,10;30,30;10,30)")
    p4a = RBA::DPolygon::from_s("(0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p4b = RBA::DPolygon::from_s("(0,0;1,1;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p4c = RBA::DPolygon::from_s("(0,0;0,40;40,40;40,0)")
    p4d = RBA::DPolygon::from_s("(0,0;1,1;0,40;40,40;40,0/10,10;30,10;30,30;10,30/15,15;16,15;16,16;15,16)")

    assert_equal(p1 == p2a, true)
    assert_equal(p1 == p2b, true)
    assert_equal(p1 == p3a, false)
    assert_equal(p1 == p3b, false)
    assert_equal(p1 == p4a, false)
    assert_equal(p1 == p4b, false)
    assert_equal(p1 == p4c, false)
    assert_equal(p1 == p4d, false)
    assert_equal(p1.eql?(p2a), true)
    assert_equal(p1.eql?(p2b), true)
    assert_equal(p1.eql?(p3a), false)
    assert_equal(p1.eql?(p3b), false)
    assert_equal(p3a.eql?(p3b), false)
    assert_equal(p1.eql?(p4a), false)
    assert_equal(p1.eql?(p4b), false)
    assert_equal(p1.eql?(p4c), false)
    assert_equal(p1.eql?(p4d), false)
    assert_equal(p4a.eql?(p4b), false)
    assert_equal(p4a.eql?(p4c), false)
    assert_equal(p4a.eql?(p4d), false)
    assert_equal(p4b.eql?(p4c), false)
    assert_equal(p4b.eql?(p4d), false)
    assert_equal(p4c.eql?(p4d), false)
    assert_equal(p1 < p2a, false)
    assert_equal(p1 < p2b, false)
    assert_equal(p1 < p3a, true)
    assert_equal(p1 < p3b, true)
    assert_equal(p1 < p4a, false)
    assert_equal(p1 < p4b, true)
    assert_equal(p1 < p4c, false)
    assert_equal(p1 < p4d, true)
    assert_equal(p4a < p4b, true)
    assert_equal(p4a < p4c, false)
    assert_equal(p4a < p4d, true)
    assert_equal(p4b < p4c, false)
    assert_equal(p4b < p4d, true)
    assert_equal(p4c < p4d, true)
    assert_equal(p2a < p1, false)
    assert_equal(p2b < p1, false)
    assert_equal(p3a < p1, false)
    assert_equal(p3b < p1, false)
    assert_equal(p4a < p1, true)
    assert_equal(p4b < p1, false)
    assert_equal(p4c < p1, true)
    assert_equal(p4d < p1, false)
    assert_equal(p4b < p4a, false)
    assert_equal(p4c < p4a, true)
    assert_equal(p4d < p4a, false)
    assert_equal(p4c < p4b, true)
    assert_equal(p4d < p4b, false)
    assert_equal(p4d < p4c, false)

  end

  # hash values 
  def test_HashValues

    p1 = RBA::DPolygon::from_s("(0,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p2a = RBA::DPolygon::from_s("(0.0000001,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p2b = RBA::DPolygon::from_s("(0,0;0,40;40,40;40,0/10.0000001,10;30,10;30,30;10,30)")
    p3a = RBA::DPolygon::from_s("(0.0001,0;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p3b = RBA::DPolygon::from_s("(0,0;0,40;40,40;40,0/10.0001,10;30,10;30,30;10,30)")
    p4a = RBA::DPolygon::from_s("(0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p4b = RBA::DPolygon::from_s("(0,0;1,1;0,40;40,40;40,0/10,10;30,10;30,30;10,30)")
    p4c = RBA::DPolygon::from_s("(0,0;0,40;40,40;40,0)")
    p4d = RBA::DPolygon::from_s("(0,0;1,1;0,40;40,40;40,0/10,10;30,10;30,30;10,30/15,15;16,15;16,16;15,16)")

    assert_equal(p1.hash == p2a.hash, true)
    assert_equal(p1.hash == p2b.hash, true)
    assert_equal(p1.hash == p3a.hash, false)
    assert_equal(p1.hash == p3b.hash, false)
    assert_equal(p1.hash == p4a.hash, false)
    assert_equal(p1.hash == p4b.hash, false)
    assert_equal(p1.hash == p4c.hash, false)
    assert_equal(p1.hash == p4d.hash, false)
    assert_equal(p4a.hash == p4b.hash, false)
    assert_equal(p4a.hash == p4c.hash, false)
    assert_equal(p4a.hash == p4d.hash, false)
    assert_equal(p4b.hash == p4c.hash, false)
    assert_equal(p4b.hash == p4d.hash, false)
    assert_equal(p4c.hash == p4d.hash, false)

    h = { p1 => "p1", p3a => "p3a", p3b => "p3b", p4a => "p4a", p4b => "p4b", p4c => "p4c", p4d => "p4d" }

    assert_equal(h[p1], "p1")
    assert_equal(h[p2a], "p1")
    assert_equal(h[p2b], "p1")
    assert_equal(h[p3a], "p3a")
    assert_equal(h[p3b], "p3b")
    assert_equal(h[p4a], "p4a")
    assert_equal(h[p4b], "p4b")
    assert_equal(h[p4c], "p4c")
    assert_equal(h[p4d], "p4d")

  end

  # touches predicate
  def test_touches

    p1 = RBA::Polygon::new(RBA::Box::new(10, 20, 30, 40))
    assert_equal(p1.touches?(RBA::Polygon::new(RBA::Box::new(30, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::Polygon::new(RBA::Box::new(31, 20, 40, 50))), false)
    assert_equal(p1.touches?(RBA::Polygon::new(RBA::Box::new(29, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::SimplePolygon::new(RBA::Box::new(30, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::SimplePolygon::new(RBA::Box::new(31, 20, 40, 50))), false)
    assert_equal(p1.touches?(RBA::SimplePolygon::new(RBA::Box::new(29, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::Box::new(30, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::Box::new(31, 20, 40, 50)), false)
    assert_equal(p1.touches?(RBA::Box::new(29, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::Edge::new(30, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::Edge::new(31, 20, 40, 50)), false)
    assert_equal(p1.touches?(RBA::Edge::new(29, 20, 40, 50)), true)

    p1 = RBA::SimplePolygon::new(RBA::Box::new(10, 20, 30, 40))
    assert_equal(p1.touches?(RBA::Polygon::new(RBA::Box::new(30, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::Polygon::new(RBA::Box::new(31, 20, 40, 50))), false)
    assert_equal(p1.touches?(RBA::Polygon::new(RBA::Box::new(29, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::SimplePolygon::new(RBA::Box::new(30, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::SimplePolygon::new(RBA::Box::new(31, 20, 40, 50))), false)
    assert_equal(p1.touches?(RBA::SimplePolygon::new(RBA::Box::new(29, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::Box::new(30, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::Box::new(31, 20, 40, 50)), false)
    assert_equal(p1.touches?(RBA::Box::new(29, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::Edge::new(30, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::Edge::new(31, 20, 40, 50)), false)
    assert_equal(p1.touches?(RBA::Edge::new(29, 20, 40, 50)), true)

    p1 = RBA::DPolygon::new(RBA::DBox::new(10, 20, 30, 40))
    assert_equal(p1.touches?(RBA::DPolygon::new(RBA::DBox::new(30, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::DPolygon::new(RBA::DBox::new(31, 20, 40, 50))), false)
    assert_equal(p1.touches?(RBA::DPolygon::new(RBA::DBox::new(29, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::DSimplePolygon::new(RBA::DBox::new(30, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::DSimplePolygon::new(RBA::DBox::new(31, 20, 40, 50))), false)
    assert_equal(p1.touches?(RBA::DSimplePolygon::new(RBA::DBox::new(29, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::DBox::new(30, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::DBox::new(31, 20, 40, 50)), false)
    assert_equal(p1.touches?(RBA::DBox::new(29, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::DEdge::new(30, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::DEdge::new(31, 20, 40, 50)), false)
    assert_equal(p1.touches?(RBA::DEdge::new(29, 20, 40, 50)), true)

    p1 = RBA::DSimplePolygon::new(RBA::DBox::new(10, 20, 30, 40))
    assert_equal(p1.touches?(RBA::DPolygon::new(RBA::DBox::new(30, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::DPolygon::new(RBA::DBox::new(31, 20, 40, 50))), false)
    assert_equal(p1.touches?(RBA::DPolygon::new(RBA::DBox::new(29, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::DSimplePolygon::new(RBA::DBox::new(30, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::DSimplePolygon::new(RBA::DBox::new(31, 20, 40, 50))), false)
    assert_equal(p1.touches?(RBA::DSimplePolygon::new(RBA::DBox::new(29, 20, 40, 50))), true)
    assert_equal(p1.touches?(RBA::DBox::new(30, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::DBox::new(31, 20, 40, 50)), false)
    assert_equal(p1.touches?(RBA::DBox::new(29, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::DEdge::new(30, 20, 40, 50)), true)
    assert_equal(p1.touches?(RBA::DEdge::new(31, 20, 40, 50)), false)
    assert_equal(p1.touches?(RBA::DEdge::new(29, 20, 40, 50)), true)

  end

  def test_selfRef

    # p1 is a reference to the new'd object:
    p1 = RBA::Polygon::new(RBA::Box::new(10, 20, 30, 40)).move(10, 20)
    assert_equal(p1.to_s, "(20,40;20,60;40,60;40,40)")
    GC.start
    # after the GC cleaned up the new'd object, the reference still needs to be valid
    assert_equal(p1.to_s, "(20,40;20,60;40,60;40,40)")

  end

  def test_splitPolygon

    pts = []
    pts << RBA::Point::new(0, 0)
    pts << RBA::Point::new(0, 1000)
    pts << RBA::Point::new(100, 1000)
    pts << RBA::Point::new(100, 100)
    pts << RBA::Point::new(1000, 100)
    pts << RBA::Point::new(1000, 0)
    split = RBA::Polygon::new(pts).split
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")

    split = RBA::SimplePolygon::new(pts).split
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")

    pts = []
    pts << RBA::DPoint::new(0, 0)
    pts << RBA::DPoint::new(0, 1000)
    pts << RBA::DPoint::new(100, 1000)
    pts << RBA::DPoint::new(100, 100)
    pts << RBA::DPoint::new(1000, 100)
    pts << RBA::DPoint::new(1000, 0)
    split = RBA::DPolygon::new(pts).split
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")

    split = RBA::DSimplePolygon::new(pts).split
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")

  end

  def test_breakPolygon

    pts = []
    pts << RBA::Point::new(0, 0)
    pts << RBA::Point::new(0, 1000)
    pts << RBA::Point::new(100, 1000)
    pts << RBA::Point::new(100, 100)
    pts << RBA::Point::new(1000, 100)
    pts << RBA::Point::new(1000, 0)

    split = RBA::Polygon::new(pts).break(4, 0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")
    split = RBA::Polygon::new(pts).break(0, 2.0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")
    split = RBA::Polygon::new(pts).break(0, 0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,1000;100,1000;100,100;1000,100;1000,0)")

    split = RBA::SimplePolygon::new(pts).break(4, 0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")
    split = RBA::SimplePolygon::new(pts).break(0, 2.0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")
    split = RBA::SimplePolygon::new(pts).break(0, 0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,1000;100,1000;100,100;1000,100;1000,0)")

    pts = []
    pts << RBA::DPoint::new(0, 0)
    pts << RBA::DPoint::new(0, 1000)
    pts << RBA::DPoint::new(100, 1000)
    pts << RBA::DPoint::new(100, 100)
    pts << RBA::DPoint::new(1000, 100)
    pts << RBA::DPoint::new(1000, 0)

    split = RBA::DPolygon::new(pts).break(4, 0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")
    split = RBA::DPolygon::new(pts).break(0, 2.0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")
    split = RBA::DPolygon::new(pts).break(0, 0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,1000;100,1000;100,100;1000,100;1000,0)")

    split = RBA::DSimplePolygon::new(pts).break(4, 0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")
    split = RBA::DSimplePolygon::new(pts).break(0, 2.0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,100;1000,100;1000,0);(0,100;0,1000;100,1000;100,100)")
    split = RBA::DSimplePolygon::new(pts).break(0, 0)
    assert_equal(split.collect { |p| p.to_s }.join(";"), "(0,0;0,1000;100,1000;100,100;1000,100;1000,0)")

  end

  def test_voidMethodsReturnSelf

    hull =  [ RBA::Point::new(0, 0),       RBA::Point::new(6000, 0), 
              RBA::Point::new(6000, 3000), RBA::Point::new(0, 3000) ]
    hole1 = [ RBA::Point::new(1000, 1000), RBA::Point::new(2000, 1000), 
              RBA::Point::new(2000, 2000), RBA::Point::new(1000, 2000) ]
    hole2 = [ RBA::Point::new(3000, 1000), RBA::Point::new(4000, 1000), 
              RBA::Point::new(4000, 2000), RBA::Point::new(3000, 2000) ]
    poly = RBA::Polygon::new(hull).insert_hole(hole1).insert_hole(hole2)
    assert_equal(poly.to_s, "(0,0;0,3000;6000,3000;6000,0/1000,1000;2000,1000;2000,2000;1000,2000/3000,1000;4000,1000;4000,2000;3000,2000)")

  end

end

load("test_epilogue.rb")

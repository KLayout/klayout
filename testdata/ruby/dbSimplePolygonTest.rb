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

class DBSimplePolygon_TestClass < TestBase

  # DSimplePolygon basics
  def test_1_DSimplePolygon

    a = RBA::DSimplePolygon::new
    assert_equal( a.to_s, "()" )
    assert_equal( RBA::DSimplePolygon::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.is_empty?, true )
    assert_equal( a.is_rectilinear?, false )

    b = a.dup 
    a = RBA::DSimplePolygon::new( [ RBA::DPoint::new( 0, 1 ), RBA::DPoint::new( 1, 5 ), RBA::DPoint::new( 5, 5 ) ] )
    assert_equal( a.to_s, "(0,1;1,5;5,5)" )
    assert_equal( RBA::DSimplePolygon::from_s(a.to_s).to_s, a.to_s )
    assert_equal( RBA::SimplePolygon::new(a).to_s, "(0,1;1,5;5,5)" )
    assert_equal( a.num_points, 3 )
    c = a.dup 

    assert_equal( a == b, false )
    assert_equal( a == c, true )
    assert_equal( a != b, true )
    assert_equal( a != c, false )

    a = RBA::DSimplePolygon::new( RBA::DBox::new( 5, -10, 20, 15 ) )
    assert_equal( a.is_empty?, false )
    assert_equal( a.is_rectilinear?, true )
    assert_equal( a.to_s, "(5,-10;5,15;20,15;20,-10)" )
    assert_equal( RBA::DPolygon.new(a).class.to_s, "RBA::DPolygon" )
    assert_equal( RBA::DPolygon.new(a).to_s, "(5,-10;5,15;20,15;20,-10)" )
    assert_equal( a.num_points, 4 )
    assert_equal( a.area, 15*25 )
    assert_equal( a.perimeter, 80 )
    assert_equal( a.inside( RBA::DPoint::new( 10, 0 ) ), true )
    assert_equal( a.inside( RBA::DPoint::new( 5, 0 ) ), true )
    assert_equal( a.inside( RBA::DPoint::new( 30, 0 ) ), false )

    arr = []
    a.each_point { |p| arr.push( p.to_s ) }
    assert_equal( arr, ["5,-10", "5,15", "20,15", "20,-10"] )

    b = a.dup

    assert_equal( a.moved( RBA::DPoint::new( 0, 1 ) ).to_s, "(5,-9;5,16;20,16;20,-9)" )
    assert_equal( a.moved( 0, 1 ).to_s, "(5,-9;5,16;20,16;20,-9)" )
    aa = a.dup
    aa.move( 1, 0)
    assert_equal( aa.to_s, "(6,-10;6,15;21,15;21,-10)" )
    a.move( RBA::DPoint::new( 1, 0 ) )
    assert_equal( a.to_s, "(6,-10;6,15;21,15;21,-10)" )
    assert_equal( a.point(1).to_s, "6,15" )
    assert_equal( a.point(0).to_s, "6,-10" )
    assert_equal( a.point(100).to_s, "0,0" )
    a.compress(false);
    assert_equal( a.to_s, "(6,-10;6,15;21,15;21,-10)" )
    a.compress(true);
    assert_equal( a.to_s, "(6,-10;6,15;21,15;21,-10)" )

    bb = b.transformed( RBA::DTrans::new( RBA::DTrans::R0, RBA::DPoint::new( 1, 0 )) )
    assert_equal( bb.to_s, "(6,-10;6,15;21,15;21,-10)" )
    assert_equal( b.to_s, "(5,-10;5,15;20,15;20,-10)" )

    b.transform( RBA::DTrans::new( RBA::DTrans::R0, RBA::DPoint::new( 1, 0 )) ) 
    assert_equal( b.to_s, "(6,-10;6,15;21,15;21,-10)" )

    m = RBA::DCplxTrans::new( RBA::DTrans::new, 1.5 )
    assert_equal( a.transformed(m).class.to_s, "RBA::DSimplePolygon" )
    assert_equal( a.transformed(m).to_s, "(9,-15;9,22.5;31.5,22.5;31.5,-15)" )
    assert_equal( a.to_s, "(6,-10;6,15;21,15;21,-10)" )

    b = a.dup
    b.transform(m)
    assert_equal( b.to_s, "(9,-15;9,22.5;31.5,22.5;31.5,-15)" )

    m = RBA::VCplxTrans::new( 1000.0 )
    assert_equal( a.transformed(m).class.to_s, "RBA::SimplePolygon" )
    assert_equal( a.transformed(m).to_s, "(6000,-10000;6000,15000;21000,15000;21000,-10000)" )

    a.points = [ RBA::DPoint::new( 0, 1 ), RBA::DPoint::new( 1, 1 ), RBA::DPoint::new( 1, 5 ) ]
    assert_equal( a.bbox.to_s, "(0,1;1,5)" )

    arr = []
    a.each_edge { |p| arr.push( p.to_s ) }
    assert_equal( arr, ["(0,1;1,5)", "(1,5;1,1)", "(1,1;0,1)"] )

    # Ellipse constructor
    p = RBA::DSimplePolygon::ellipse( RBA::DBox::new(-10000, -20000, 30000, 40000), 200 )
    assert_equal(p.num_points, 200)
    assert_equal(p.is_empty?, false)
    assert_equal(p.is_rectilinear?, false)
    assert_equal(p.bbox.to_s, "(-10000,-20000;30000,40000)")
    assert_equal(p.area.to_i, 1884645544)    # roughly box.area*PI/4
    
    p = RBA::DSimplePolygon::ellipse( RBA::DBox::new(-10000, -20000, 30000, 40000), 4 )
    assert_equal(p.to_s, "(10000,-20000;-10000,10000;10000,40000;30000,10000)")

    # halfmanhattan variants
    p = RBA::DSimplePolygon::new([ RBA::DPoint::new( 0, 0 ), RBA::DPoint::new( 0, 100 ), RBA::DPoint::new( 100, 100 ) ])
    assert_equal(p.is_halfmanhattan?, true)
    assert_equal(p.is_rectilinear?, false)
    p = RBA::DSimplePolygon::new([ RBA::DPoint::new( 0, 0 ), RBA::DPoint::new( 0, 100 ), RBA::DPoint::new( 100, 101 ) ])
    assert_equal(p.is_halfmanhattan?, false)
    assert_equal(p.is_rectilinear?, false)
    p = RBA::DSimplePolygon::new([ RBA::DPoint::new( 0, 0 ), RBA::DPoint::new( 0, 100 ), RBA::DPoint::new( 100, 100 ), RBA::DPoint::new( 100, 0) ])
    assert_equal(p.is_halfmanhattan?, true)
    assert_equal(p.is_rectilinear?, true)

  end

  # SimplePolygon basics
  def test_1_SimplePolygon

    a = RBA::SimplePolygon::new
    assert_equal( a.to_s, "()" )
    assert_equal( RBA::SimplePolygon::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.is_empty?, true )
    assert_equal( a.is_rectilinear?, false )

    b = a.dup 
    a = RBA::SimplePolygon::new( [ RBA::Point::new( 0, 1 ), RBA::Point::new( 1, 5 ), RBA::Point::new( 5, 5 ) ] )
    assert_equal( a.to_s, "(0,1;1,5;5,5)" )
    assert_equal( RBA::SimplePolygon::from_s(a.to_s).to_s, a.to_s )
    assert_equal( RBA::DSimplePolygon::new(a).to_s, "(0,1;1,5;5,5)" )
    assert_equal( a.num_points, 3 )
    c = a.dup 

    assert_equal( a == b, false )
    assert_equal( a == c, true )
    assert_equal( a != b, true )
    assert_equal( a != c, false )

    a = RBA::SimplePolygon::new( RBA::Box::new( 5, -10, 20, 15 ) )
    assert_equal( a.to_s, "(5,-10;5,15;20,15;20,-10)" )
    assert_equal( a.is_empty?, false )
    assert_equal( a.is_rectilinear?, true )
    assert_equal( RBA::Polygon.new(a).class.to_s, "RBA::Polygon" )
    assert_equal( RBA::Polygon.new(a).to_s, "(5,-10;5,15;20,15;20,-10)" )
    assert_equal( a.num_points, 4 )
    assert_equal( a.area, 15*25 )
    assert_equal( a.perimeter, 80 )
    assert_equal( a.inside( RBA::Point::new( 10, 0 ) ), true )
    assert_equal( a.inside( RBA::Point::new( 5, 0 ) ), true )
    assert_equal( a.inside( RBA::Point::new( 30, 0 ) ), false )

    arr = []
    a.each_point { |p| arr.push( p.to_s ) }
    assert_equal( arr, ["5,-10", "5,15", "20,15", "20,-10"] )

    b = a.dup

    assert_equal( a.moved( RBA::Point::new( 0, 1 ) ).to_s, "(5,-9;5,16;20,16;20,-9)" )
    assert_equal( a.moved( 0, 1 ).to_s, "(5,-9;5,16;20,16;20,-9)" )
    aa = a.dup
    aa.move( 1, 0)
    assert_equal( aa.to_s, "(6,-10;6,15;21,15;21,-10)" )
    a.move( RBA::Point::new( 1, 0 ) )
    assert_equal( a.to_s, "(6,-10;6,15;21,15;21,-10)" )
    assert_equal( a.point(1).to_s, "6,15" )
    assert_equal( a.point(0).to_s, "6,-10" )
    assert_equal( a.point(100).to_s, "0,0" )
    a.compress(false);
    assert_equal( a.to_s, "(6,-10;6,15;21,15;21,-10)" )
    a.compress(true);
    assert_equal( a.to_s, "(6,-10;6,15;21,15;21,-10)" )

    bb = b.transformed( RBA::Trans::new( RBA::Trans::R0, RBA::Point::new( 1, 0 )) )
    assert_equal( bb.to_s, "(6,-10;6,15;21,15;21,-10)" )
    assert_equal( b.to_s, "(5,-10;5,15;20,15;20,-10)" )

    b.transform( RBA::Trans::new( RBA::Trans::R0, RBA::Point::new( 1, 0 )) ) 
    assert_equal( b.to_s, "(6,-10;6,15;21,15;21,-10)" )

    m = RBA::CplxTrans::new( RBA::Trans::new, 1.5 )
    assert_equal( a.transformed(m).to_s, "(9,-15;9,22.5;31.5,22.5;31.5,-15)" )
    assert_equal( a.transformed(RBA::ICplxTrans::new(m)).to_s, "(9,-15;9,23;32,23;32,-15)" )
    assert_equal( a.to_s, "(6,-10;6,15;21,15;21,-10)" )

    b = a.dup
    b.transform(RBA::ICplxTrans::new(m))
    assert_equal( b.to_s, "(9,-15;9,23;32,23;32,-15)" )

    a.points = [ RBA::Point::new( 0, 1 ), RBA::Point::new( 1, 1 ), RBA::Point::new( 1, 5 ) ]
    assert_equal( a.bbox.to_s, "(0,1;1,5)" )

    arr = []
    a.each_edge { |p| arr.push( p.to_s ) }
    assert_equal( arr, ["(0,1;1,5)", "(1,5;1,1)", "(1,1;0,1)"] )

    # corner rounding
    a = RBA::SimplePolygon::new( [ RBA::Point::new(0, 0), RBA::Point::new(0, 2000), RBA::Point::new(4000, 2000),
                             RBA::Point::new(4000, 1000), RBA::Point::new(2000, 1000), RBA::Point::new(2000, 0) ] )
    ar = a.round_corners(100, 200, 8)
    assert_equal( ar.to_s, "(117,0;0,117;0,1883;117,2000;3883,2000;4000,1883;4000,1117;3883,1000;2059,1000;2000,941;2000,117;1883,0)" )
    ar = a.round_corners(200, 100, 32)
    assert_equal( ar.to_s, "(90,0;71,4;53,11;36,22;22,36;11,53;4,71;0,90;0,1910;4,1929;11,1947;22,1964;36,1978;53,1989;71,1996;90,2000;3910,2000;3929,1996;3947,1989;3964,1978;3978,1964;3989,1947;3996,1929;4000,1910;4000,1090;3996,1071;3989,1053;3978,1036;3964,1022;3947,1011;3929,1004;3910,1000;2180,1000;2142,992;2105,977;2073,955;2045,927;2023,895;2008,858;2000,820;2000,90;1996,71;1989,53;1978,36;1964,22;1947,11;1929,4;1910,0)" )

    # Minkowsky sums
    p = RBA::SimplePolygon::new( [ RBA::Point.new(0, -100), RBA::Point.new(0, -50), RBA::Point.new(-100, -75), RBA::Point.new(0, 100), RBA::Point.new(50, 50), RBA::Point.new(100, 75), RBA::Point.new(100, 0), RBA::Point.new(100, -50) ] )
    assert_equal(p.minkowsky_sum(RBA::Edge.new(RBA::Point.new(10, 10), RBA::Point.new(210, 110)), true).to_s, "(10,-90;10,-40;-90,-65;10,110;210,210;260,160;310,185;310,60)")
    assert_equal(p.minkowsky_sum([RBA::Point.new(10, 10), RBA::Point.new(10, 310), RBA::Point.new(510, 310), RBA::Point.new(510, 10), RBA::Point.new(10, 10) ], false).to_s, "(10,-90;10,-65;-90,-65;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90/110,110;410,110;410,210;110,210)")
    assert_equal(p.minkowsky_sum([RBA::Point.new(10, 10), RBA::Point.new(10, 310), RBA::Point.new(510, 310), RBA::Point.new(510, 10), RBA::Point.new(10, 10) ], true).to_s, "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)")
    assert_equal(p.minkowsky_sum(RBA::Box.new(RBA::Point.new(10, 10), RBA::Point.new(210, 110)), true).to_s, "(10,-90;10,-65;-90,-65;-90,35;10,210;210,210;235,185;310,185;310,-40;210,-90)")
    assert_equal(p.minkowsky_sum(RBA::Box.new(RBA::Point.new(10, 10), RBA::Point.new(210, 10)), true).to_s, "(10,-90;10,-65;-90,-65;10,110;210,110;235,85;310,85;310,-40;210,-90)")
    assert_equal(p.minkowsky_sum(RBA::SimplePolygon.new(RBA::Box.new(RBA::Point.new(10, 10), RBA::Point.new(210, 110))), true).to_s, "(10,-90;10,-65;-90,-65;-90,35;10,210;210,210;235,185;310,185;310,-40;210,-90)")

    # Conversion to simple polygon and hole resolution
    ph = p.minkowsky_sum([RBA::Point.new(10, 10), RBA::Point.new(10, 310), RBA::Point.new(510, 310), RBA::Point.new(510, 10), RBA::Point.new(10, 10) ], false)
    ph2 = ph.dup
    assert_equal(ph.to_s, "(10,-90;10,-65;-90,-65;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90/110,110;410,110;410,210;110,210)")
    ph.resolve_holes
    assert_equal(ph.to_s, "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)")
    assert_equal(ph2.to_s, "(10,-90;10,-65;-90,-65;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90/110,110;410,110;410,210;110,210)")

    ph3 = ph2.resolved_holes
    assert_equal(ph2.to_s, "(10,-90;10,-65;-90,-65;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90/110,110;410,110;410,210;110,210)")
    assert_equal(ph3.to_s, "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)")

    ph3 = ph2.to_simple_polygon
    assert_equal(ph3.class.to_s, "RBA::SimplePolygon")
    assert_equal(ph2.to_s, "(10,-90;10,-65;-90,-65;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90/110,110;410,110;410,210;110,210)")
    assert_equal(ph3.to_s, "(10,-90;10,-65;-90,-65;-90,210;110,210;110,110;410,110;410,210;-90,210;-90,235;10,410;510,410;535,385;610,385;610,-40;510,-90)")

    # Ellipse constructor
    p = RBA::SimplePolygon::ellipse( RBA::Box::new(-10000, -20000, 30000, 40000), 200 )
    assert_equal(p.num_points, 200)
    assert_equal(p.is_empty?, false)
    assert_equal(p.is_rectilinear?, false)
    assert_equal(p.bbox.to_s, "(-10000,-20000;30000,40000)")
    assert_equal(p.area, 1884651158)    # roughly box.area*PI/4
    
    p = RBA::SimplePolygon::ellipse( RBA::Box::new(-10000, -20000, 30000, 40000), 4 )
    assert_equal(p.to_s, "(10000,-20000;-10000,10000;10000,40000;30000,10000)")

    # halfmanhattan variants
    p = RBA::SimplePolygon::new([ RBA::Point::new( 0, 0 ), RBA::Point::new( 0, 100 ), RBA::Point::new( 100, 100 ) ])
    assert_equal(p.is_halfmanhattan?, true)
    assert_equal(p.is_rectilinear?, false)
    p = RBA::SimplePolygon::new([ RBA::Point::new( 0, 0 ), RBA::Point::new( 0, 100 ), RBA::Point::new( 100, 101 ) ])
    assert_equal(p.is_halfmanhattan?, false)
    assert_equal(p.is_rectilinear?, false)
    p = RBA::SimplePolygon::new([ RBA::Point::new( 0, 0 ), RBA::Point::new( 0, 100 ), RBA::Point::new( 100, 100 ), RBA::Point::new( 100, 0) ])
    assert_equal(p.is_halfmanhattan?, true)
    assert_equal(p.is_rectilinear?, true)

  end

  # raw mode polygons
  def test_2_SimplePolygon

    pts = [ RBA::Point::new(0, 0) ]
    p = RBA::SimplePolygon::new(pts, false)
    assert_equal(p.to_s, "()")
    
    pts = [ RBA::Point::new(0, 0) ]
    p = RBA::SimplePolygon::new(pts)
    assert_equal(p.to_s, "()")
    
    pts = [ RBA::Point::new(0, 0) ]
    p = RBA::SimplePolygon::new(pts, true)
    assert_equal(p.to_s, "(0,0)")

    arr = []
    p.each_edge { |e| arr.push( e.to_s ) }
    assert_equal( arr, ["(0,0;0,0)"] )
    
    pts = [ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ]
    p = RBA::SimplePolygon::new(pts, true)
    assert_equal(p.to_s, "(0,0;10,0)")
    assert_equal(RBA::Polygon::new(p).to_s, "(0,0;10,0)")
    assert_equal(RBA::DSimplePolygon::new(p).to_s, "(0,0;10,0)")

    p.points = []
    assert_equal(p.to_s, "()")
    assert_equal(RBA::Polygon::new(p).to_s, "()")
    
    p.points = [ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ]
    assert_equal(p.to_s, "(0,0;10,0)")

    p.set_points([ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ], false)
    assert_equal(p.to_s, "()")

    p.set_points([ RBA::Point::new(0, 0), RBA::Point::new(10, 0) ], true)
    assert_equal(p.to_s, "(0,0;10,0)")

    arr = []
    p.each_edge { |e| arr.push( e.to_s ) }
    assert_equal( arr, ["(0,0;10,0)", "(10,0;0,0)"] )
    
    assert_equal(p.moved(1, 2).to_s, "(1,2;11,2)")
    assert_equal((p * 2).to_s, "(0,0;20,0)")
    assert_equal(p.transformed(RBA::Trans::new(RBA::Trans::R90)).to_s, "(0,0;0,10)")

    pp = p.dup
    pp.transform(RBA::Trans::new(RBA::Trans::R90))
    assert_equal(pp.to_s, "(0,0;0,10)")
    
    p = RBA::SimplePolygon::new([ RBA::Point::new(0, 0), RBA::Point::new(0, 10) ], true)
    q = RBA::SimplePolygon::new([ RBA::Point::new(1, 1), RBA::Point::new(-9, 1) ], true)
    assert_equal(p.minkowsky_sum(q, false).to_s, "(-9,1;-9,11;1,11;1,1)")
    
  end

  # raw mode polygons
  def test_2_DSimplePolygon

    pts = [ RBA::DPoint::new(0, 0) ]
    p = RBA::DSimplePolygon::new(pts, true)
    assert_equal(p.to_s, "(0,0)")

    arr = []
    p.each_edge { |e| arr.push( e.to_s ) }
    assert_equal( arr, ["(0,0;0,0)"] )
    
    pts = [ RBA::DPoint::new(0, 0), RBA::DPoint::new(10, 0) ]
    p = RBA::DSimplePolygon::new(pts, true)
    assert_equal(p.to_s, "(0,0;10,0)")
    assert_equal(RBA::DPolygon::new(p).to_s, "(0,0;10,0)")
    assert_equal(RBA::SimplePolygon::new(p).to_s, "(0,0;10,0)")

    p.points = []
    assert_equal(p.to_s, "()")
    assert_equal(RBA::DPolygon::new(p).to_s, "()")
    
    p.points = [ RBA::DPoint::new(0, 0), RBA::DPoint::new(10, 0) ]
    assert_equal(p.to_s, "(0,0;10,0)")

    arr = []
    p.each_edge { |e| arr.push( e.to_s ) }
    assert_equal( arr, ["(0,0;10,0)", "(10,0;0,0)"] )
    
    assert_equal(p.moved(1, 2).to_s, "(1,2;11,2)")
    assert_equal((p * 2).to_s, "(0,0;20,0)")
    assert_equal(p.transformed(RBA::DTrans::new(RBA::DTrans::R90)).to_s, "(0,0;0,10)")

    pp = p.dup
    pp.transform(RBA::DTrans::new(RBA::DTrans::R90))
    assert_equal(pp.to_s, "(0,0;0,10)")
    
  end

  # fuzzy compare 
  def test_FuzzyCompare

    p1 = RBA::DSimplePolygon::from_s("(0,0;0,40;40,40;40,0)")
    p2a = RBA::DSimplePolygon::from_s("(0.0000001,0;0,40;40,40;40,0)")
    p3a = RBA::DSimplePolygon::from_s("(0.0001,0;0,40;40,40;40,0)")
    p4a = RBA::DSimplePolygon::from_s("(0,40;40,40;40,0)")
    p4b = RBA::DSimplePolygon::from_s("(0,0;1,1;0,40;40,40;40,0)")

    assert_equal(p1 == p2a, true)
    assert_equal(p1 == p3a, false)
    assert_equal(p1 == p4a, false)
    assert_equal(p1 == p4b, false)
    assert_equal(p1.eql?(p2a), true)
    assert_equal(p1.eql?(p3a), false)
    assert_equal(p1.eql?(p4a), false)
    assert_equal(p1.eql?(p4b), false)
    assert_equal(p4a.eql?(p4b), false)
    assert_equal(p1 < p2a, false)
    assert_equal(p1 < p3a, true)
    assert_equal(p1 < p4a, false)
    assert_equal(p1 < p4b, true)
    assert_equal(p4a < p4b, true)
    assert_equal(p2a < p1, false)
    assert_equal(p3a < p1, false)
    assert_equal(p4a < p1, true)
    assert_equal(p4b < p1, false)
    assert_equal(p4b < p4a, false)

  end

  # hash values 
  def test_HashValues

    p1 = RBA::DSimplePolygon::from_s("(0,0;0,40;40,40;40,0)")
    p2a = RBA::DSimplePolygon::from_s("(0.0000001,0;0,40;40,40;40,0)")
    p3a = RBA::DSimplePolygon::from_s("(0.0001,0;0,40;40,40;40,0)")
    p4a = RBA::DSimplePolygon::from_s("(0,40;40,40;40,0)")
    p4b = RBA::DSimplePolygon::from_s("(0,0;1,1;0,40;40,40;40,0)")

    assert_equal(p1.hash == p2a.hash, true)
    assert_equal(p1.hash == p3a.hash, false)
    assert_equal(p1.hash == p4a.hash, false)
    assert_equal(p1.hash == p4b.hash, false)
    assert_equal(p4a.hash == p4b.hash, false)

    h = { p1 => "p1", p3a => "p3a", p4a => "p4a", p4b => "p4b" }

    assert_equal(h[p1], "p1")
    assert_equal(h[p3a], "p3a")
    assert_equal(h[p4a], "p4a")
    assert_equal(h[p4b], "p4b")

  end

end

load("test_epilogue.rb")


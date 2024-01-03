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

class DBPath_TestClass < TestBase

  # DPath basics
  def test_1_DPath

    a = RBA::DPath::new
    assert_equal( a.to_s, "() w=0 bx=0 ex=0 r=false" )
    assert_equal( a.area.to_s, "0.0" )
    assert_equal( a.length.to_s, "0.0" )
    assert_equal( RBA::DPath::from_s(a.to_s).to_s, a.to_s )

    b = a.dup 
    a = RBA::DPath::new( [ RBA::DPoint::new( 0, 1 ), RBA::DPoint::new( 1, 5 ) ], 2.5 )
    assert_equal( a.to_s, "(0,1;1,5) w=2.5 bx=0 ex=0 r=false" )
    assert_equal( "%.3f" % a.area, "10.308" )
    assert_equal( "%.3f" % a.length, "4.123" )
    assert_equal( RBA::DPath::from_s(a.to_s).to_s, a.to_s )
    c = a.dup 

    assert_equal( a == b, false )
    assert_equal( a == c, true )
    assert_equal( a != b, true )
    assert_equal( a != c, false )

    assert_equal( "%.6f" % a.area, "10.307764" )
    assert_equal( a.width, 2.5 )
    a.width = 3.0
    assert_equal( a.width, 3 )

    a = RBA::DPath::new( [ RBA::DPoint::new( 0, 1 ), RBA::DPoint::new( 1, 5 ) ], 2.5, -0.5, 1.5 )
    assert_equal( a.to_s, "(0,1;1,5) w=2.5 bx=-0.5 ex=1.5 r=false" )
    assert_equal( RBA::DPath::from_s(a.to_s).to_s, a.to_s )
    assert_equal( "%.3f" % a.length, "5.123" )
    assert_equal( RBA::Path::new(a).to_s, "(0,1;1,5) w=3 bx=-1 ex=2 r=false" )

    a.bgn_ext = 0.5
    assert_equal( a.to_s, "(0,1;1,5) w=2.5 bx=0.5 ex=1.5 r=false" )
    assert_equal( a.bgn_ext, 0.5 )

    a.end_ext = 2.5
    assert_equal( a.to_s, "(0,1;1,5) w=2.5 bx=0.5 ex=2.5 r=false" )
    assert_equal( a.end_ext, 2.5 )

    a = RBA::DPath::new( [ RBA::DPoint::new( 0, 1 ), RBA::DPoint::new( 1, 5 ) ], 2.5, -0.5, 1.5, true )
    assert_equal( a.to_s, "(0,1;1,5) w=2.5 bx=-0.5 ex=1.5 r=true" )

    assert_equal( a.is_round?, true )
    a.round = false
    assert_equal( a.is_round?, false )

    a.points = [ RBA::DPoint::new( 0, 1 ), RBA::DPoint::new( 2, 2 ), RBA::DPoint::new( 1, 5 ) ]
    assert_equal( a.to_s, "(0,1;2,2;1,5) w=2.5 bx=-0.5 ex=1.5 r=false" )
    assert_equal( (a * 2).to_s, "(0,2;4,4;2,10) w=5 bx=-1 ex=3 r=false" )
    assert_equal( a.points, 3 )

    arr = []
    a.each_point { |p| arr.push( p.to_s ) }
    assert_equal( arr, [ "0,1", "2,2", "1,5" ] )

    b = a.dup

    assert_equal( a.moved( RBA::DPoint::new( 0, 1 ) ).to_s, "(0,2;2,3;1,6) w=2.5 bx=-0.5 ex=1.5 r=false" )
    assert_equal( a.moved( 0, 1 ).to_s, "(0,2;2,3;1,6) w=2.5 bx=-0.5 ex=1.5 r=false" )
    aa = a.dup
    aa.move( 1, 0 )
    assert_equal( aa.to_s, "(1,1;3,2;2,5) w=2.5 bx=-0.5 ex=1.5 r=false" )
    a.move( RBA::DPoint::new( 1, 0 ) )
    assert_equal( a.to_s, "(1,1;3,2;2,5) w=2.5 bx=-0.5 ex=1.5 r=false" )

    b = b.transformed( RBA::DTrans::new( RBA::DTrans::R0, RBA::DPoint::new( 1, 0 )) )
    assert_equal( b.to_s, "(1,1;3,2;2,5) w=2.5 bx=-0.5 ex=1.5 r=false" )

    m = RBA::DCplxTrans::new( RBA::DTrans::new, 1.5 )
    assert_equal( a.transformed(m).class.to_s, "RBA::DPath")
    assert_equal( a.transformed(m).to_s, "(1.5,1.5;4.5,3;3,7.5) w=3.75 bx=-0.75 ex=2.25 r=false" )

    m = RBA::VCplxTrans::new(1000.0)
    assert_equal( a.transformed(m).class.to_s, "RBA::Path")
    assert_equal( a.transformed(m).to_s, "(1000,1000;3000,2000;2000,5000) w=2500 bx=-500 ex=1500 r=false" )

    a.points = [ RBA::DPoint::new( 0, 1 ), RBA::DPoint::new( 1, 1 ), RBA::DPoint::new( 1, 5 ) ]
    a.width = 2.0
    a.bgn_ext = 1.0
    a.end_ext = 1.0
    assert_equal( a.bbox.to_s, "(-1,0;2,6)" )

    assert_equal( a.simple_polygon.to_s, "(-1,0;-1,2;0,2;0,6;2,6;2,0)" )
    assert_equal( a.polygon.to_s, "(-1,0;-1,2;0,2;0,6;2,6;2,0)" )

  end

  # Path basics
  def test_1_Path

    a = RBA::Path::new
    assert_equal( a.to_s, "() w=0 bx=0 ex=0 r=false" )
    assert_equal( a.area.to_f.to_s, "0.0" )
    assert_equal( a.length.to_s, "0" )
    assert_equal( RBA::Path::from_s(a.to_s).to_s, a.to_s )

    b = a.dup 
    a = RBA::Path::new( [ RBA::Point::new( 0, 10 ), RBA::Point::new( 10, 50 ) ], 25 )
    assert_equal( a.to_s, "(0,10;10,50) w=25 bx=0 ex=0 r=false" )
    assert_equal( a.area.to_f.to_s, "1025.0" )
    assert_equal( a.length.to_s, "41" )
    assert_equal( RBA::Path::from_s(a.to_s).to_s, a.to_s )
    c = a.dup 

    assert_equal( a == b, false )
    assert_equal( a == c, true )
    assert_equal( a != b, true )
    assert_equal( a != c, false )

    assert_equal( a.width, 25 )
    a.width = 30
    assert_equal( a.width, 30 )

    a = RBA::Path::new( [ RBA::Point::new( 0, 10 ), RBA::Point::new( 10, 50 ) ], 25, -5, 15 )
    assert_equal( a.to_s, "(0,10;10,50) w=25 bx=-5 ex=15 r=false" )
    assert_equal( a.length.to_s, "51" )
    assert_equal( RBA::Path::from_s(a.to_s).to_s, a.to_s )
    assert_equal( RBA::DPath::new(a).to_s, "(0,10;10,50) w=25 bx=-5 ex=15 r=false" )

    a.bgn_ext = 5
    assert_equal( a.to_s, "(0,10;10,50) w=25 bx=5 ex=15 r=false" )
    assert_equal( a.bgn_ext, 5 )

    a.end_ext = 25
    assert_equal( a.to_s, "(0,10;10,50) w=25 bx=5 ex=25 r=false" )
    assert_equal( a.end_ext, 25 )

    a = RBA::Path::new( [ RBA::Point::new( 0, 10 ), RBA::Point::new( 10, 50 ) ], 25, -5, 15, true )
    assert_equal( a.to_s, "(0,10;10,50) w=25 bx=-5 ex=15 r=true" )
    assert_equal( (a * 2).to_s, "(0,20;20,100) w=50 bx=-10 ex=30 r=true" )

    assert_equal( a.is_round?, true )
    a.round = false
    assert_equal( a.is_round?, false )

    a.points = [ RBA::Point::new( 0, 10 ), RBA::Point::new( 20, 20 ), RBA::Point::new( 10, 50 ) ]
    assert_equal( a.to_s, "(0,10;20,20;10,50) w=25 bx=-5 ex=15 r=false" )
    assert_equal( a.points, 3 )

    arr = []
    a.each_point { |p| arr.push( p.to_s ) }
    assert_equal( arr, [ "0,10", "20,20", "10,50" ] )

    b = a.dup

    assert_equal( a.moved( RBA::Point::new( 0, 10 ) ).to_s, "(0,20;20,30;10,60) w=25 bx=-5 ex=15 r=false" )
    assert_equal( a.moved( 0, 10 ).to_s, "(0,20;20,30;10,60) w=25 bx=-5 ex=15 r=false" )
    aa = a.dup
    aa.move( 10, 0 )
    assert_equal( aa.to_s, "(10,10;30,20;20,50) w=25 bx=-5 ex=15 r=false" )
    a.move( RBA::Point::new( 10, 0 ) )
    assert_equal( a.to_s, "(10,10;30,20;20,50) w=25 bx=-5 ex=15 r=false" )

    b = b.transformed( RBA::Trans::new( RBA::Trans::R0, RBA::Point::new( 10, 0 )) )
    assert_equal( b.to_s, "(10,10;30,20;20,50) w=25 bx=-5 ex=15 r=false" )

    m = RBA::CplxTrans::new( RBA::Trans::new, 1.5 )
    assert_equal( a.transformed(m).to_s, "(15,15;45,30;30,75) w=37.5 bx=-7.5 ex=22.5 r=false" )
    assert_equal( a.transformed(RBA::ICplxTrans::new(m)).to_s, "(15,15;45,30;30,75) w=38 bx=-8 ex=23 r=false" )

    a.points = [ RBA::Point::new( 0, 10 ), RBA::Point::new( 10, 10 ), RBA::Point::new( 10, 50 ) ]
    a.width = 20
    a.bgn_ext = 10
    a.end_ext = 10
    assert_equal( a.bbox.to_s, "(-10,0;20,60)" )

    assert_equal( a.simple_polygon.to_s, "(-10,0;-10,20;0,20;0,60;20,60;20,0)" )
    assert_equal( a.polygon.to_s, "(-10,0;-10,20;0,20;0,60;20,60;20,0)" )

  end

  # Advanced
  def test_2_DPath

    a = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)
    b = a.round_corners(100, 16, 0.5)
    assert_equal(b.to_s, "(0,0;0,987.689437438;0,992.592935078;1.79647712055,1007.18590848;2.98574998547,1011.94299994;1000,5000) w=2500 bx=-500 ex=1500 r=false")

  end

  def test_2_Path

    a = RBA::Path::new([ RBA::Point::new(0, 0),RBA::Point::new(0, 1000), RBA::Point::new(1000, 5000) ], 2500, -500, 1500)
    b = a.round_corners(100, 16)
    assert_equal(b.to_s, "(0,0;0,988;0,993;2,1007;3,1012;1000,5000) w=2500 bx=-500 ex=1500 r=false")

  end

  # Fuzzy compare
  def test_3_Path

    p1 = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)
    p2a = RBA::DPath::new([ RBA::DPoint::new(1e-7, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)
    p2b = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500 + 1e-7, -500, 1500)
    p2c = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500 + 1e-7, 1500)
    p2d = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500 + 1e-7)
    p3a = RBA::DPath::new([ RBA::DPoint::new(1e-4, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)
    p3b = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500 + 1e-4, -500, 1500)
    p3c = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500 + 1e-4, 1500)
    p3d = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500 + 1e-4)
    p4a = RBA::DPath::new([ RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)
    p4b = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1500, 2000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)

    assert_equal(p1 == p2a, true)
    assert_equal(p1 == p2b, true)
    assert_equal(p1 == p2c, true)
    assert_equal(p1 == p2d, true)
    assert_equal(p1.eql?(p2a), true)
    assert_equal(p1.eql?(p2b), true)
    assert_equal(p1.eql?(p2c), true)
    assert_equal(p1.eql?(p2d), true)
    assert_equal(p1 != p2a, false)
    assert_equal(p1 != p2b, false)
    assert_equal(p1 != p2c, false)
    assert_equal(p1 != p2d, false)
    assert_equal(p1 < p2a, false)
    assert_equal(p1 < p2b, false)
    assert_equal(p1 < p2c, false)
    assert_equal(p1 < p2d, false)
    assert_equal(p2a < p1, false)
    assert_equal(p2b < p1, false)
    assert_equal(p2c < p1, false)
    assert_equal(p2d < p1, false)
    
    assert_equal(p1 == p3a, false)
    assert_equal(p1 == p3b, false)
    assert_equal(p1 == p3c, false)
    assert_equal(p1 == p3d, false)
    assert_equal(p3a == p3b, false)
    assert_equal(p3a == p3c, false)
    assert_equal(p3a == p3d, false)
    assert_equal(p3b == p3c, false)
    assert_equal(p3b == p3d, false)
    assert_equal(p3c == p3d, false)
    assert_equal(p1.eql?(p3a), false)
    assert_equal(p1.eql?(p3b), false)
    assert_equal(p1.eql?(p3c), false)
    assert_equal(p1.eql?(p3d), false)
    assert_equal(p1 != p3a, true)
    assert_equal(p1 != p3b, true)
    assert_equal(p1 != p3c, true)
    assert_equal(p1 != p3d, true)
    assert_equal(p1 < p3a, true)
    assert_equal(p1 < p3b, true)
    assert_equal(p1 < p3c, true)
    assert_equal(p1 < p3d, true)
    assert_equal(p3a < p1, false)
    assert_equal(p3b < p1, false)
    assert_equal(p3c < p1, false)
    assert_equal(p3d < p1, false)
    
    assert_equal(p1 == p4a, false)
    assert_equal(p1 == p4b, false)
    assert_equal(p1.eql?(p4a), false)
    assert_equal(p1.eql?(p4b), false)
    assert_equal(p1 != p4a, true)
    assert_equal(p1 != p4b, true)
    assert_equal(p1 < p4a, false)
    assert_equal(p1 < p4b, true)
    assert_equal(p4a < p1, true)
    assert_equal(p4b < p1, false)

  end

  # Hash values 
  def test_4_Path

    p1 = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)
    p2a = RBA::DPath::new([ RBA::DPoint::new(1e-7, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)
    p2b = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500 + 1e-7, -500, 1500)
    p2c = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500 + 1e-7, 1500)
    p2d = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500 + 1e-7)
    p3a = RBA::DPath::new([ RBA::DPoint::new(1e-4, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)
    p3b = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500 + 1e-4, -500, 1500)
    p3c = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500 + 1e-4, 1500)
    p3d = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500 + 1e-4)
    p4a = RBA::DPath::new([ RBA::DPoint::new(0, 1000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)
    p4b = RBA::DPath::new([ RBA::DPoint::new(0, 0),RBA::DPoint::new(0, 1000), RBA::DPoint::new(1500, 2000), RBA::DPoint::new(1000, 5000) ], 2500, -500, 1500)

    assert_equal(p1.hash == p2a.hash, true)
    assert_equal(p1.hash == p2b.hash, true)
    assert_equal(p1.hash == p2c.hash, true)
    assert_equal(p1.hash == p2d.hash, true)
    assert_equal(p1.hash == p3a.hash, false)
    assert_equal(p1.hash == p3b.hash, false)
    assert_equal(p1.hash == p3c.hash, false)
    assert_equal(p1.hash == p3d.hash, false)
    assert_equal(p1.hash == p4a.hash, false)
    assert_equal(p1.hash == p4b.hash, false)

    h = { p1 => "p1", p3a => "p3a", p3b => "p3b", p3c => "p3c", p3d => "p3d", p4a => "p4a", p4b => "p4b" }

    assert_equal(h[p1], "p1")
    assert_equal(h[p3a], "p3a")
    assert_equal(h[p3b], "p3b")
    assert_equal(h[p3c], "p3c")
    assert_equal(h[p3d], "p3d")
    assert_equal(h[p4a], "p4a")
    assert_equal(h[p4b], "p4b")

  end

end

load("test_epilogue.rb")

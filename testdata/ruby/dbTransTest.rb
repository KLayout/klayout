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

class DBTrans_TestClass < TestBase

  # Transformation basics
  def test_1_DTrans

    a = RBA::DTrans::new
    b = RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ))
    c = RBA::DTrans::new( 3, true, RBA::DPoint::new( 17, 5 ))
    d = RBA::DTrans::new( RBA::DPoint::new( 17, 5 ))
    e = RBA::DTrans::new( RBA::DTrans::M135 )
    e2 = RBA::DTrans::from_itrans( RBA::Trans::M135 )
    f = RBA::DTrans::new( RBA::Trans::new( RBA::Trans::M135, RBA::Point::new( 17, 5 )) )

    assert_equal( a.to_s, "r0 0,0" )
    assert_equal( RBA::DTrans::from_s(a.to_s).to_s, a.to_s )
    assert_equal( b.to_s, "m135 17,5" )
    assert_equal( c.to_s, "m135 17,5" )
    assert_equal( d.to_s, "r0 17,5" )
    assert_equal( e.to_s, "m135 0,0" )
    assert_equal( e2.to_s, "m135 0,0" )
    assert_equal( f.to_s, "m135 17,5" )
    assert_equal( RBA::DTrans::new( RBA::Trans::M135, RBA::DPoint::new( 1.2, 0.25 )).to_itype(0.001).to_s, "m135 1200,250" )
    assert_equal( RBA::Trans::new( RBA::Trans::M135, RBA::Point::new( 1200, 250 )).to_dtype(0.001).to_s, "m135 1.2,0.25" )
    assert_equal( RBA::DTrans::from_s(f.to_s).to_s, f.to_s )

    assert_equal( b.trans( RBA::DPoint::new( 1, 0 )).to_s, "17,4" )

    assert_equal( a == b, false )
    assert_equal( a == a, true )
    assert_equal( a != b, true )
    assert_equal( a != a, false )
    assert_equal( (d * e) == b, true )
    assert_equal( (e * d) == b, false )

    i = c.inverted

    assert_equal( i.to_s, "m135 5,17" )
    assert_equal( (i * b) == a, true )
    assert_equal( (b * i) == a, true )

    c = RBA::DTrans::new( 3, true, RBA::DPoint::new( 17, 5 ))
    assert_equal( c.to_s, "m135 17,5" )
    c.disp = RBA::DPoint::new(1, 7)
    assert_equal( c.to_s, "m135 1,7" )
    c.angle = 1
    assert_equal( c.to_s, "m45 1,7" )
    c.rot = 3
    assert_equal( c.to_s, "r270 1,7" )
    c.mirror = true
    assert_equal( c.to_s, "m135 1,7" )

    assert_equal( RBA::Trans::new(RBA::Trans::R180, 5,-7).to_s, "r180 5,-7" )
    assert_equal( RBA::Trans::new(RBA::Trans::R180, RBA::Point::new(5,-7)).to_s, "r180 5,-7" )
    assert_equal( RBA::Trans::new(RBA::Trans::R180, RBA::Vector::new(5,-7)).to_s, "r180 5,-7" )
    assert_equal( RBA::Trans::new(RBA::Trans::R180, RBA::DVector::new(5,-7)).to_s, "r180 5,-7" )
    assert_equal( RBA::Trans::new(RBA::Trans::R180).to_s, "r180 0,0" )

    assert_equal( e.ctrans( 2.0 ), 2.0 )
    assert_equal( e * 2.0, 2.0 )
    assert_equal( e.trans( RBA::Edge::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( ( e * RBA::Edge::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( e.trans( RBA::Box::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( ( e * RBA::Box::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( e.trans( RBA::Text::new("text", RBA::Vector::new(0, 1)) ).to_s, "('text',m135 -1,0)" )
    assert_equal( ( e * RBA::Text::new("text", RBA::Vector::new(0, 1)) ).to_s, "('text',m135 -1,0)" )
    assert_equal( e.trans( RBA::Polygon::new( [ RBA::Point::new(0, 1), RBA::Point::new(2, -3), RBA::Point::new(4, 5) ] ) ).to_s, "(-5,-4;-1,0;3,-2)" )
    assert_equal( ( e * RBA::Polygon::new( [ RBA::Point::new(0, 1), RBA::Point::new(2, -3), RBA::Point::new(4, 5) ] ) ).to_s, "(-5,-4;-1,0;3,-2)" )
    assert_equal( e.trans( RBA::Path::new( [ RBA::Point::new(0, 1), RBA::Point::new(2, 3) ], 10 ) ).to_s, "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )
    assert_equal( ( e * RBA::Path::new( [ RBA::Point::new(0, 1), RBA::Point::new(2, 3) ], 10 ) ).to_s, "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )

    # Constructor variations
    assert_equal( RBA::Trans::new().to_s, "r0 0,0" )
    assert_equal( RBA::Trans::new(1).to_s, "r90 0,0" )
    assert_equal( RBA::Trans::new(2, true).to_s, "m90 0,0" )
    assert_equal( RBA::Trans::new(2, true, RBA::Vector::new(100, 200)).to_s, "m90 100,200" )
    assert_equal( RBA::Trans::new(2, true, 100, 200).to_s, "m90 100,200" )
    assert_equal( RBA::Trans::new(RBA::Vector::new(100, 200)).to_s, "r0 100,200" )
    assert_equal( RBA::Trans::new(100, 200).to_s, "r0 100,200" )
    assert_equal( RBA::Trans::new(RBA::Trans::new(100, 200), 10, 20).to_s, "r0 110,220" )
    assert_equal( RBA::Trans::new(RBA::Trans::new(100, 200), RBA::Vector::new(10, 20)).to_s, "r0 110,220" )

    assert_equal( RBA::DTrans::new().to_s, "r0 0,0" )
    assert_equal( RBA::DTrans::new(1).to_s, "r90 0,0" )
    assert_equal( RBA::DTrans::new(2, true).to_s, "m90 0,0" )
    assert_equal( RBA::DTrans::new(2, true, RBA::DVector::new(0.1, 0.2)).to_s, "m90 0.1,0.2" )
    assert_equal( RBA::DTrans::new(2, true, 0.1, 0.2).to_s, "m90 0.1,0.2" )
    assert_equal( RBA::DTrans::new(RBA::DVector::new(0.1, 0.2)).to_s, "r0 0.1,0.2" )
    assert_equal( RBA::DTrans::new(0.1, 0.2).to_s, "r0 0.1,0.2" )
    assert_equal( RBA::DTrans::new(RBA::DTrans::new(0.1, 0.2), 0.01, 0.02).to_s, "r0 0.11,0.22" )
    assert_equal( RBA::DTrans::new(RBA::DTrans::new(0.1, 0.2), RBA::DVector::new(0.01, 0.02)).to_s, "r0 0.11,0.22" )

  end

  # Magnification basics
  def test_2_DTrans

    a = RBA::DTrans::new
    b = RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ))
    ma = RBA::DCplxTrans::new( a, 0.5 )
    mb = RBA::DCplxTrans::new( b, 2.0 )
    u = RBA::DCplxTrans::new( a )

    assert_equal( ma.to_s, "r0 *0.5 0,0" )
    assert_equal( mb.to_s, "m135 *2 17,5" )

    assert_equal( ma == mb, false )
    assert_equal( ma == ma, true )
    assert_equal( ma != mb, true )
    assert_equal( ma != ma, false )

    i = mb.inverted

    assert_equal( i.to_s, "m135 *0.5 2.5,8.5" )
    assert_equal( RBA::DCplxTrans::from_s(i.to_s).to_s, i.to_s )
    assert_equal( i * mb == u, true )
    assert_equal( mb * i == u, true )

    assert_equal( mb.trans( RBA::DPoint::new( 1, 0 )).to_s, "17,3" )
    assert_equal( mb.ctrans(2).to_s, "4.0" )
    assert_equal( (mb * 2).to_s, "4.0" )
    assert_equal( i.ctrans(2).to_s, "1.0" )
    assert_equal( (i * 2).to_s, "1.0" )

  end

  # Complex transformation specials
  def test_3_DTrans

    c = RBA::DCplxTrans::new( 5.0, -7.0 )
    assert_equal( c.to_s, "r0 *1 5,-7" )

    c = RBA::DCplxTrans::new( RBA::DCplxTrans::M135 )
    assert_equal( c.to_s, "m135 *1 0,0" )
    assert_equal( c.is_unity?, false )
    assert_equal( c.is_ortho?, true )
    assert_equal( c.is_mag?, false )
    assert_equal( c.is_complex?, false )
    assert_equal( c.is_mirror?, true )
    assert_equal( c.rot, RBA::DCplxTrans::M135.rot )
    assert_equal( c.s_trans.to_s, "m135 0,0" )
    assert_equal( c.angle, 270 )

    assert_equal( c.trans( RBA::DEdge::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( ( c * RBA::DEdge::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( c.trans( RBA::DBox::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( ( c * RBA::DBox::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( c.trans( RBA::DText::new("text", RBA::DVector::new(0, 1)) ).to_s, "('text',m135 -1,0)" )
    assert_equal( ( c * RBA::DText::new("text", RBA::DVector::new(0, 1)) ).to_s, "('text',m135 -1,0)" )
    assert_equal( c.trans( RBA::DPolygon::new( [ RBA::DPoint::new(0, 1), RBA::DPoint::new(2, -3), RBA::DPoint::new(4, 5) ] ) ).to_s, "(-5,-4;-1,0;3,-2)" )
    assert_equal( ( c * RBA::DPolygon::new( [ RBA::DPoint::new(0, 1), RBA::DPoint::new(2, -3), RBA::DPoint::new(4, 5) ] ) ).to_s, "(-5,-4;-1,0;3,-2)" )
    assert_equal( c.trans( RBA::DPath::new( [ RBA::DPoint::new(0, 1), RBA::DPoint::new(2, 3) ], 10 ) ).to_s, "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )
    assert_equal( ( c * RBA::DPath::new( [ RBA::DPoint::new(0, 1), RBA::DPoint::new(2, 3) ], 10 ) ).to_s, "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )

    c = RBA::DCplxTrans::from_itrans( RBA::CplxTrans::M135 )
    assert_equal( c.to_s, "m135 *1 0,0" )

    c = RBA::DCplxTrans::new( 1.5 )
    assert_equal( c.to_s, "r0 *1.5 0,0" )
    assert_equal( c.is_unity?, false )
    assert_equal( c.is_ortho?, true )
    assert_equal( c.is_mag?, true )
    assert_equal( c.is_complex?, true )
    assert_equal( c.is_mirror?, false )
    assert_equal( c.rot, RBA::DCplxTrans::R0.rot )
    assert_equal( c.s_trans.to_s, "r0 0,0" )
    assert_equal( c.angle, 0 )

    c = RBA::DCplxTrans::new( 0.75, 45, true, 2.5, -12.5 )
    assert_equal( c.to_s, "m22.5 *0.75 2.5,-12.5" )
    c = RBA::DCplxTrans::new( 0.75, 45, true, RBA::DPoint::new( 2.5, -12.5 ) )
    assert_equal( c.to_s, "m22.5 *0.75 2.5,-12.5" )
    assert_equal( c.is_unity?, false )
    assert_equal( c.is_ortho?, false )
    assert_equal( c.is_complex?, true )
    assert_equal( c.is_mag?, true )
    assert_equal( c.rot, RBA::DCplxTrans::M0.rot )
    assert_equal( c.s_trans.to_s, "m0 2.5,-12.5" )
    assert_equal( (c.angle - 45).abs < 1e-10, true )

    assert_equal( c.ctrans( 5 ).to_s, "3.75" )
    assert_equal( (c * 5).to_s, "3.75" )
    assert_equal( c.trans( RBA::DPoint::new( 12, 16 ) ).to_s, "17.3492424049,-14.6213203436" )

    assert_equal( RBA::DCplxTrans::new.to_s, "r0 *1 0,0" )
    assert_equal( RBA::DCplxTrans::new.is_unity?, true )
    assert_equal( (c * c.inverted()).is_unity?, true )

    c.mirror = false
    assert_equal( c.to_s, "r45 *0.75 2.5,-12.5" )
    c.mag = 1.5
    assert_equal( c.to_s, "r45 *1.5 2.5,-12.5" )
    c.disp = RBA::DPoint::new( -1.0, 5.5 )
    assert_equal( c.to_s, "r45 *1.5 -1,5.5" )
    assert_equal( c.mag, 1.5 )
    c.angle = 60
    assert_equal( c.to_s, "r60 *1.5 -1,5.5" )
    assert_equal( sprintf("%g",c.angle), "60" )

  end

  # Transformation basics
  def test_1_Trans

    a = RBA::Trans::new
    b = RBA::Trans::new( RBA::Trans::M135, RBA::Point::new( 17, 5 ))
    c = RBA::Trans::new( 3, true, RBA::Point::new( 17, 5 ))
    d = RBA::Trans::new( RBA::Point::new( 17, 5 ))
    e = RBA::Trans::new( RBA::Trans::M135 )
    e2 = RBA::Trans::from_dtrans( RBA::DTrans::M135 )
    f = RBA::Trans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 )) )

    assert_equal( a.to_s, "r0 0,0" )
    assert_equal( RBA::Trans::from_s(a.to_s).to_s, a.to_s )
    assert_equal( b.to_s, "m135 17,5" )
    assert_equal( c.to_s, "m135 17,5" )
    assert_equal( d.to_s, "r0 17,5" )
    assert_equal( e.to_s, "m135 0,0" )
    assert_equal( e2.to_s, "m135 0,0" )
    assert_equal( f.to_s, "m135 17,5" )
    assert_equal( RBA::Trans::from_s(f.to_s).to_s, f.to_s )

    assert_equal( b.trans( RBA::Point::new( 1, 0 )).to_s, "17,4" )

    assert_equal( a == b, false )
    assert_equal( a == a, true )
    assert_equal( a != b, true )
    assert_equal( a != a, false )
    assert_equal( (d * e) == b, true )
    assert_equal( (e * d) == b, false )

    i = c.inverted

    assert_equal( i.to_s, "m135 5,17" )
    assert_equal( (i * b) == a, true )
    assert_equal( (b * i) == a, true )

    c = RBA::Trans::new( 3, true, RBA::Point::new( 17, 5 ))
    assert_equal( c.to_s, "m135 17,5" )
    c.disp = RBA::Point::new(1, 7)
    assert_equal( c.to_s, "m135 1,7" )
    c.angle = 1
    assert_equal( c.to_s, "m45 1,7" )
    c.rot = 3
    assert_equal( c.to_s, "r270 1,7" )
    c.mirror = true
    assert_equal( c.to_s, "m135 1,7" )

    assert_equal( e.trans( RBA::Edge::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( ( e * RBA::Edge::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( e.trans( RBA::Box::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( ( e * RBA::Box::new(0, 1, 2, 3) ).to_s, "(-3,-2;-1,0)" )
    assert_equal( e.trans( RBA::Text::new("text", RBA::Vector::new(0, 1)) ).to_s, "('text',m135 -1,0)" )
    assert_equal( ( e * RBA::Text::new("text", RBA::Vector::new(0, 1)) ).to_s, "('text',m135 -1,0)" )
    assert_equal( e.trans( RBA::Polygon::new( [ RBA::Point::new(0, 1), RBA::Point::new(2, -3), RBA::Point::new(4, 5) ] ) ).to_s, "(-5,-4;-1,0;3,-2)" )
    assert_equal( ( e * RBA::Polygon::new( [ RBA::Point::new(0, 1), RBA::Point::new(2, -3), RBA::Point::new(4, 5) ] ) ).to_s, "(-5,-4;-1,0;3,-2)" )
    assert_equal( e.trans( RBA::Path::new( [ RBA::Point::new(0, 1), RBA::Point::new(2, 3) ], 10 ) ).to_s, "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )
    assert_equal( ( e * RBA::Path::new( [ RBA::Point::new(0, 1), RBA::Point::new(2, 3) ], 10 ) ).to_s, "(-1,0;-3,-2) w=10 bx=0 ex=0 r=false" )

  end

  # Complex transformation basics
  def test_2_Trans

    a = RBA::Trans::new
    b = RBA::Trans::new( RBA::Trans::M135, RBA::Point::new( 17, 5 ))
    ma = RBA::CplxTrans::new( a, 0.5 )
    mb = RBA::CplxTrans::new( b, 2.0 )
    u = RBA::CplxTrans::new( a )

    assert_equal( ma.to_s, "r0 *0.5 0,0" )
    assert_equal( mb.to_s, "m135 *2 17,5" )

    assert_equal( ma == mb, false )
    assert_equal( ma == ma, true )
    assert_equal( ma != mb, true )
    assert_equal( ma != ma, false )

    assert_equal( mb.inverted.to_s, "m135 *0.5 2.5,8.5" )

    i = mb.dup
    i.invert

    assert_equal( i.to_s, "m135 *0.5 2.5,8.5" )
    assert_equal( i * mb == u, true )
    assert_equal( mb * i == u, true )

    assert_equal( mb.trans( RBA::Point::new( 1, 0 )).to_s, "17,3" )
    assert_equal( mb.ctrans(2).to_s, "4.0" )
    assert_equal( (mb * 2).to_s, "4.0" )
    assert_equal( i.ctrans(2).to_s, "1.0" )
    assert_equal( (i * 2).to_s, "1.0" )

  end

  # Complex transformation specials
  def test_3_Trans

    c = RBA::CplxTrans::new( 5, -7 )
    assert_equal( c.to_s, "r0 *1 5,-7" )
    assert_equal( RBA::CplxTrans::from_s(c.to_s).to_s, c.to_s )

    c = RBA::CplxTrans::new( RBA::CplxTrans::M135 )
    assert_equal( c.to_s, "m135 *1 0,0" )
    assert_equal( c.is_unity?, false )
    assert_equal( c.is_ortho?, true )
    assert_equal( c.is_mag?, false )
    assert_equal( c.is_mirror?, true )
    assert_equal( c.rot, RBA::CplxTrans::M135.rot )
    assert_equal( c.s_trans.to_s, "m135 0,0" )
    assert_equal( c.angle, 270 )

    c = RBA::CplxTrans::from_dtrans( RBA::DCplxTrans::M135 )
    assert_equal( c.to_s, "m135 *1 0,0" )

    c = RBA::CplxTrans::new( 1.5 )
    assert_equal( c.to_s, "r0 *1.5 0,0" )
    assert_equal( c.is_unity?, false )
    assert_equal( c.is_ortho?, true )
    assert_equal( c.is_mag?, true )
    assert_equal( c.is_mirror?, false )
    assert_equal( c.rot, RBA::CplxTrans::R0.rot )
    assert_equal( c.s_trans.to_s, "r0 0,0" )
    assert_equal( c.angle, 0 )

    c = RBA::CplxTrans::new( 0.75, 45, true, 2.5, -12.5 )
    assert_equal( c.to_s, "m22.5 *0.75 2.5,-12.5" )
    assert_equal( RBA::CplxTrans::from_s(c.to_s).to_s, c.to_s )
    c = RBA::CplxTrans::new( 0.75, 45, true, RBA::DPoint::new( 2.5, -12.5 ) )
    assert_equal( c.to_s, "m22.5 *0.75 2.5,-12.5" )
    assert_equal( c.is_unity?, false )
    assert_equal( c.is_ortho?, false )
    assert_equal( c.is_mag?, true )
    assert_equal( c.rot, RBA::CplxTrans::M0.rot )
    assert_equal( c.s_trans.to_s, "m0 3,-13" )
    assert_equal( (c.angle - 45).abs < 1e-10, true )

    assert_equal( c.ctrans( 5 ).to_s, "3.75" )
    assert_equal( (c * 5).to_s, "3.75" )
    assert_equal( c.trans( RBA::Point::new( 12, 16 ) ).to_s, "17.3492424049,-14.6213203436" )

    assert_equal( RBA::CplxTrans::new.to_s, "r0 *1 0,0" )
    assert_equal( RBA::CplxTrans::new.is_unity?, true )
    assert_equal( (c.inverted() * c).is_unity?, true )

    c.mirror = false
    assert_equal( c.to_s, "r45 *0.75 2.5,-12.5" )
    c.mag = 1.5
    assert_equal( c.to_s, "r45 *1.5 2.5,-12.5" )
    c.disp = RBA::DPoint::new( -1.0, 5.5 )
    assert_equal( c.to_s, "r45 *1.5 -1,5.5" )
    assert_equal( c.mag, 1.5 )
    c.angle = 60
    assert_equal( c.to_s, "r60 *1.5 -1,5.5" )
    assert_equal( sprintf("%g",c.angle), "60" )

    # Constructor variations
    assert_equal( RBA::ICplxTrans::new().to_s, "r0 *1 0,0" )
    assert_equal( RBA::ICplxTrans::new(1.5).to_s, "r0 *1.5 0,0" )
    assert_equal( RBA::ICplxTrans::new(RBA::Trans::new(1, false, 10, 20), 1.5).to_s, "r90 *1.5 10,20" )
    assert_equal( RBA::ICplxTrans::new(RBA::Trans::new(1, false, 10, 20)).to_s, "r90 *1 10,20" )
    assert_equal( RBA::ICplxTrans::new(1.5, 80, true, RBA::Vector::new(100, 200)).to_s, "m40 *1.5 100,200" )
    assert_equal( RBA::ICplxTrans::new(1.5, 80, true, 100, 200).to_s, "m40 *1.5 100,200" )
    assert_equal( RBA::ICplxTrans::new(RBA::Vector::new(100, 200)).to_s, "r0 *1 100,200" )
    assert_equal( RBA::ICplxTrans::new(100, 200).to_s, "r0 *1 100,200" )
    assert_equal( RBA::ICplxTrans::new(RBA::ICplxTrans::new(100, 200)).to_s, "r0 *1 100,200" )
    assert_equal( RBA::ICplxTrans::new(RBA::ICplxTrans::new(100, 200), 1.5).to_s, "r0 *1.5 150,300" )
    assert_equal( RBA::ICplxTrans::new(RBA::ICplxTrans::new(100, 200), 1.5, RBA::Vector::new(10, 20)).to_s, "r0 *1.5 160,320" )
    assert_equal( RBA::ICplxTrans::new(RBA::ICplxTrans::new(100, 200), 1.5, 10, 20).to_s, "r0 *1.5 160,320" )

    assert_equal( RBA::DCplxTrans::new().to_s, "r0 *1 0,0" )
    assert_equal( RBA::DCplxTrans::new(1.5).to_s, "r0 *1.5 0,0" )
    assert_equal( RBA::DCplxTrans::new(RBA::DTrans::new(1, false, 0.01, 0.02), 1.5).to_s, "r90 *1.5 0.01,0.02" )
    assert_equal( RBA::DCplxTrans::new(RBA::DTrans::new(1, false, 0.01, 0.02)).to_s, "r90 *1 0.01,0.02" )
    assert_equal( RBA::DCplxTrans::new(1.5, 80, true, RBA::DVector::new(0.1, 0.2)).to_s, "m40 *1.5 0.1,0.2" )
    assert_equal( RBA::DCplxTrans::new(1.5, 80, true, 0.1, 0.2).to_s, "m40 *1.5 0.1,0.2" )
    assert_equal( RBA::DCplxTrans::new(RBA::DVector::new(0.1, 0.2)).to_s, "r0 *1 0.1,0.2" )
    assert_equal( RBA::DCplxTrans::new(0.1, 0.2).to_s, "r0 *1 0.1,0.2" )
    assert_equal( RBA::DCplxTrans::new(RBA::DCplxTrans::new(0.1, 0.2)).to_s, "r0 *1 0.1,0.2" )
    assert_equal( RBA::DCplxTrans::new(RBA::DCplxTrans::new(0.1, 0.2), 1.5).to_s, "r0 *1.5 0.15,0.3" )
    assert_equal( RBA::DCplxTrans::new(RBA::DCplxTrans::new(0.1, 0.2), 1.5, RBA::DVector::new(0.01, 0.02)).to_s, "r0 *1.5 0.16,0.32" )
    assert_equal( RBA::DCplxTrans::new(RBA::DCplxTrans::new(0.1, 0.2), 1.5, 0.01, 0.02).to_s, "r0 *1.5 0.16,0.32" )

  end

  # Complex transformation types
  def test_4_Trans

    a = RBA::Trans::new
    m = RBA::CplxTrans::new( a, 1.1 )
    da = RBA::DTrans::new
    dm = RBA::DCplxTrans::new( da, 1.1 )

    assert_equal( m.to_s, "r0 *1.1 0,0" )
    assert_equal( RBA::DCplxTrans::from_s(m.to_s).to_s, m.to_s )
    assert_equal( m.trans( RBA::Point::new( 5, -7 )).to_s, "5.5,-7.7" )

    im = RBA::ICplxTrans::new( a, 0.5 )
    im_old = im.dup

    assert_equal( im.to_s, "r0 *0.5 0,0" )
    assert_equal( RBA::ICplxTrans::from_s(im.to_s).to_s, im.to_s )
    assert_equal( im.trans( RBA::Point::new( 5, -7 )).to_s, "3,-4" )

    im = RBA::ICplxTrans::new(m)
    assert_equal( im.to_s, "r0 *1.1 0,0" )
    assert_equal( im.trans( RBA::Point::new( 5, -7 )).to_s, "6,-8" )

    im = RBA::ICplxTrans::new(dm)
    assert_equal( im.to_s, "r0 *1.1 0,0" )
    assert_equal( im.trans( RBA::Point::new( 5, -7 )).to_s, "6,-8" )

    im.assign(im_old)
    assert_equal( im.to_s, "r0 *0.5 0,0" )
    assert_equal( im.trans( RBA::Point::new( 5, -7 )).to_s, "3,-4" )

    assert_equal( RBA::ICplxTrans::new(5,-7).to_s, "r0 *1 5,-7" )

    assert_equal( RBA::ICplxTrans::new(RBA::ICplxTrans::R180, 1.5, 5,-7).to_s, "r180 *1.5 5,-7" )
    assert_equal( RBA::ICplxTrans::new(RBA::ICplxTrans::R180, 1.5, RBA::Point::new(5,-7)).to_s, "r180 *1.5 5,-7" )
    assert_equal( RBA::ICplxTrans::new(RBA::ICplxTrans::R180, 1.5, RBA::Vector::new(5,-7)).to_s, "r180 *1.5 5,-7" )
    assert_equal( RBA::ICplxTrans::new(RBA::ICplxTrans::R180, 1.5, RBA::DVector::new(5,-7)).to_s, "r180 *1.5 5,-7" )
    assert_equal( RBA::ICplxTrans::new(RBA::ICplxTrans::R180, 1.5).to_s, "r180 *1.5 0,0" )

    c = RBA::ICplxTrans::from_dtrans( RBA::DCplxTrans::M135 )
    assert_equal( c.to_s, "m135 *1 0,0" )
    c = RBA::ICplxTrans::from_trans( RBA::CplxTrans::M135 )
    assert_equal( c.to_s, "m135 *1 0,0" )

  end

  # Fuzzy compare
  def test_5_Trans_FuzzyCompare

    t1 = RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ))
    t2 = RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17 + 1e-7, 5 ))
    t3 = RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17 + 1e-4, 5 ))
    t4a = RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 18, 5 ))
    t4b = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 18, 5 ))

    assert_equal(t1 == t2, true)
    assert_equal(t1.eql?(t2), true)
    assert_equal(t1 != t2, false)
    assert_equal(t1 < t2, false)
    assert_equal(t2 < t1, false)

    assert_equal(t1 == t3, false)
    assert_equal(t1.eql?(t3), false)
    assert_equal(t1 != t3, true)
    assert_equal(t1 < t3, true)
    assert_equal(t3 < t1, false)

    assert_equal(t1 == t4a, false)
    assert_equal(t1.eql?(t4a), false)
    assert_equal(t1 != t4a, true)
    assert_equal(t1 < t4a, true)
    assert_equal(t4a < t1, false)

    assert_equal(t1 == t4b, false)
    assert_equal(t1.eql?(t4b), false)
    assert_equal(t1 != t4b, true)
    assert_equal(t1 < t4b, false)
    assert_equal(t4b < t1, true)

  end

  # Complex trans fuzzy compare
  def test_5_CplxTrans_FuzzyCompare

    t1 = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ) ), 1.0)
    t2a = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17 + 1e-7, 5 ) ), 1.0)
    t2b = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ) ), 1.0 + 1e-11)
    t2c = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ) ), 1.0)
    t2c.angle = t2c.angle + 1e-11
    t3a = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17 + 1e-4, 5 ) ), 1.0)
    t3b = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ) ), 1.0 + 1e-4)
    t3c = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ) ), 1.0)
    t3c.angle = t3c.angle + 1e-4
    t4 = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 18, 5 ) ), 1.0)

    assert_equal(t1 == t2a, true)
    assert_equal(t1.eql?(t2a), true)
    assert_equal(t1 != t2a, false)
    assert_equal(t1 < t2a, false)
    assert_equal(t2a < t1, false)

    assert_equal(t1 == t2b, true)
    assert_equal(t1.eql?(t2b), true)
    assert_equal(t1 != t2b, false)
    assert_equal(t1 < t2b, false)
    assert_equal(t2b < t1, false)

    assert_equal(t1 == t2c, true)
    assert_equal(t1.eql?(t2c), true)
    assert_equal(t1 != t2c, false)
    assert_equal(t1 < t2c, false)
    assert_equal(t2c < t1, false)

    assert_equal(t1 == t3a, false)
    assert_equal(t1.eql?(t3a), false)
    assert_equal(t1 != t3a, true)
    assert_equal(t1 < t3a, true)
    assert_equal(t3a < t1, false)

    assert_equal(t1 == t3b, false)
    assert_equal(t1.eql?(t3b), false)
    assert_equal(t1 != t3b, true)
    assert_equal(t1 < t3b, false)
    assert_equal(t3b < t1, true)

    assert_equal(t1 == t3c, false)
    assert_equal(t1.eql?(t3c), false)
    assert_equal(t1 != t3c, true)
    assert_equal(t1 < t3c, true)
    assert_equal(t3c < t1, false)

    assert_equal(t3a == t3b, false)
    assert_equal(t3a.eql?(t3b), false)
    assert_equal(t3a != t3b, true)
    assert_equal(t3a < t3b, false)
    assert_equal(t3b < t3a, true)

    assert_equal(t3a == t3c, false)
    assert_equal(t3a.eql?(t3c), false)
    assert_equal(t3a != t3c, true)
    assert_equal(t3a < t3c, false)
    assert_equal(t3c < t3a, true)

    assert_equal(t3b == t3c, false)
    assert_equal(t3b.eql?(t3c), false)
    assert_equal(t3b != t3c, true)
    assert_equal(t3b < t3c, true)
    assert_equal(t3c < t3b, false)

    assert_equal(t1 == t4, false)
    assert_equal(t1.eql?(t4), false)
    assert_equal(t1 != t4, true)
    assert_equal(t1 < t4, true)
    assert_equal(t4 < t1, false)

  end

  # Hash values 
  def test_5_Trans_Hash

    t1 = RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ))
    t2 = RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17 + 1e-7, 5 ))
    t3 = RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17 + 1e-4, 5 ))
    t4a = RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 18, 5 ))
    t4b = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 18, 5 ))

    assert_equal(t1.hash == t2.hash, true)
    assert_equal(t1.hash == t3.hash, false)
    assert_equal(t1.hash == t4a.hash, false)
    assert_equal(t1.hash == t4b.hash, false)

    h = { t1 => "t1", t3 => "t3", t4a => "t4a", t4b => "t4b" }

    assert_equal(h[t1], "t1")
    assert_equal(h[t2], "t1")
    assert_equal(h[t3], "t3")
    assert_equal(h[t4a], "t4a")
    assert_equal(h[t4b], "t4b")

  end

  # Complex trans hash values
  def test_5_CplxTrans_Hash

    t1 = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ) ), 1.0)
    t2a = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17 + 1e-7, 5 ) ), 1.0)
    t2b = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ) ), 1.0 + 1e-11)
    t2c = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ) ), 1.0)
    t2c.angle = t2c.angle + 1e-11
    t3a = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17 + 1e-4, 5 ) ), 1.0)
    t3b = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ) ), 1.0 + 1e-4)
    t3c = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::M135, RBA::DPoint::new( 17, 5 ) ), 1.0)
    t3c.angle = t3c.angle + 1e-4
    t4 = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 18, 5 ) ), 1.0)

    assert_equal(t1.hash == t2a.hash, true)
    assert_equal(t1.hash == t2b.hash, true)
    assert_equal(t1.hash == t2c.hash, true)
    assert_equal(t1.hash == t3a.hash, false)
    assert_equal(t1.hash == t3b.hash, false)
    assert_equal(t1.hash == t3c.hash, false)
    assert_equal(t3a.hash == t3b.hash, false)
    assert_equal(t3a.hash == t3c.hash, false)
    assert_equal(t3b.hash == t3c.hash, false)
    assert_equal(t1.hash == t4.hash, false)

    h = { t1 => "t1", t3a => "t3a", t3b => "t3b", t3c => "t3c", t4 => "t4" }

    assert_equal(h[t1], "t1")
    assert_equal(h[t2a], "t1")
    assert_equal(h[t2b], "t1")
    assert_equal(h[t2c], "t1")
    assert_equal(h[t3a], "t3a")
    assert_equal(h[t3b], "t3b")
    assert_equal(h[t3c], "t3c")
    assert_equal(h[t4], "t4")

  end

  # Complex trans conversions, issue #1586
  def test_6_CplxTransConversions

    itrans = RBA::ICplxTrans::new(1.0, 0.0, false, RBA::Vector::new(1, 2))
    vtrans = RBA::VCplxTrans::new(1000.0, 0.0, false, RBA::Vector::new(1, 2))
    dtrans = RBA::DCplxTrans::new(1.0, 0.0, false, RBA::DVector::new(1, 2))
    ctrans = RBA::CplxTrans::new(0.001, 0.0, false, RBA::DVector::new(1, 2))

    assert_equal(RBA::ICplxTrans::new(vtrans, 0.001).to_s, "r0 *1 1,2")
    assert_equal(RBA::ICplxTrans::new(dtrans, 0.001).to_s, "r0 *1 1000,2000")
    assert_equal(RBA::ICplxTrans::new(ctrans, 0.001).to_s, "r0 *1 1000,2000")

    assert_equal(RBA::VCplxTrans::new(itrans, 0.001).to_s, "r0 *1000 1,2")
    assert_equal(RBA::VCplxTrans::new(dtrans, 0.001).to_s, "r0 *1000 1000,2000")
    assert_equal(RBA::VCplxTrans::new(ctrans, 0.001).to_s, "r0 *1000 1000,2000")

    assert_equal(RBA::DCplxTrans::new(itrans, 0.001).to_s, "r0 *1 0.001,0.002")
    assert_equal(RBA::DCplxTrans::new(vtrans, 0.001).to_s, "r0 *1 0.001,0.002")
    assert_equal(RBA::DCplxTrans::new(ctrans, 0.001).to_s, "r0 *1 1,2")

    assert_equal(RBA::CplxTrans::new(itrans, 0.001).to_s, "r0 *0.001 0.001,0.002")
    assert_equal(RBA::CplxTrans::new(vtrans, 0.001).to_s, "r0 *0.001 0.001,0.002")
    assert_equal(RBA::CplxTrans::new(dtrans, 0.001).to_s, "r0 *0.001 1,2")

    # issue #1586 (NOTE: to_itrans is deprecated)
    t = RBA::DCplxTrans::new(1.0, 45.0, false, 12.345678, 20.000000)
    assert_equal(t.to_itrans(0.001).to_s, "r45 *1 12345.678,20000")
    assert_equal(RBA::ICplxTrans::new(t, 0.001).to_s, "r45 *1 12345.678,20000")

  end

end

load("test_epilogue.rb")

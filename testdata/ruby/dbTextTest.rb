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

class DBText_TestClass < TestBase

  # DText basics
  def test_1_DText

    a = RBA::DText::new( "hallo", 10.0, -15.0 )
    assert_equal( RBA::DText::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.to_s, "('hallo',r0 10,-15)" )

    a = RBA::DText::new( RBA::Text::new( "itext",  RBA::Trans::new( RBA::Trans::R270, RBA::Point::new( 100, -150 ))))
    assert_equal( RBA::DText::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.to_s, "('itext',r270 100,-150)" )

    a = RBA::DText::new
    assert_equal( a.to_s, "('',r0 0,0)" )

    b = a.dup 
    a = RBA::DText::new( "hallo", RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 10.0, -15.0 )))
    assert_equal( a.to_s, "('hallo',r90 10,-15)" )
    c = a.dup 

    assert_equal( a == b, false )
    assert_equal( a == c, true )
    assert_equal( a != b, true )
    assert_equal( a != c, false )

    assert_equal( a.string, "hallo" )
    assert_equal( a.trans.to_s, "r90 10,-15" )

    a.string = "hi"
    assert_equal( a.string, "hi" )

    a.trans = RBA::DTrans::new( RBA::DTrans::M45, RBA::DPoint::new( 5.0, 7.0 ))
    assert_equal( a.trans.to_s, "m45 5,7" )

    aa = a.dup
    assert_equal( aa.x.to_s, "5.0" )
    assert_equal( aa.y.to_s, "7.0" )
    aa.x = -3
    assert_equal( aa.trans.to_s, "m45 -3,7" )
    aa.y = -6
    assert_equal( aa.trans.to_s, "m45 -3,-6" )

    a = RBA::DText::new( "hallo", a.trans, 22.0, 7 )
    assert_equal( a.string, "hallo" )
    assert_equal( a.trans.to_s, "m45 5,7" )
    assert_equal( a.position.to_s, "5,7" )
    assert_equal( a.bbox.to_s, "(5,7;5,7)" )
    assert_equal( a.font, 7 )
    assert_equal( a.size, 22.0 )

    a.font = 8
    assert_equal( a.font, 8 )

    a.halign = 1
    assert_equal( a.halign.to_i, 1 )
    assert_equal( a.halign, RBA::DText::HAlignCenter )
    assert_equal( a.halign.to_s, "HAlignCenter" )

    a.halign = RBA::DText::HAlignRight
    assert_equal( a.halign.to_i, 2 )
    assert_equal( a.halign, RBA::DText::HAlignRight )
    assert_equal( a.halign.to_s, "HAlignRight" )

    a.valign = 1
    assert_equal( a.valign.to_i, 1 )
    assert_equal( a.valign, RBA::DText::VAlignCenter )
    assert_equal( a.valign.to_s, "VAlignCenter" )

    a.valign = RBA::DText::VAlignBottom
    assert_equal( a.valign.to_i, 2 )
    assert_equal( a.valign, RBA::DText::VAlignBottom )
    assert_equal( a.valign.to_s, "VAlignBottom" )

    a.size = 23.0
    assert_equal( a.size, 23.0 )

    b = a.dup

    assert_equal( a.moved( RBA::DPoint::new( 0, 1 ) ).to_s, "('hallo',m45 5,8) s=23 f=8 ha=r va=b" )
    a.move( RBA::DPoint::new( 1, 0 ) )
    assert_equal( a.to_s, "('hallo',m45 6,7) s=23 f=8 ha=r va=b" )

    b = b.transformed( RBA::DTrans::new( RBA::DTrans::R0, RBA::DPoint::new( 1, 0 )) )
    assert_equal( b.to_s, "('hallo',m45 6,7) s=23 f=8 ha=r va=b" )

    m = RBA::DCplxTrans::new( RBA::DTrans::new, 1.5 )
    assert_equal( a.transformed(m).class.to_s, "RBA::DText" )
    assert_equal( a.transformed(m).to_s, "('hallo',m45 9,10.5) s=34.5 f=8 ha=r va=b" )

    m = RBA::VCplxTrans::new( 1000.0 )
    assert_equal( a.transformed(m).class.to_s, "RBA::Text" )
    assert_equal( a.transformed(m).to_s, "('hallo',m45 6000,7000) s=23000 f=8 ha=r va=b" )

  end

  # Text basics
  def test_1_Text

    a = RBA::Text::new( "hallo", 10, -15 )
    assert_equal( RBA::Text::from_s(a.to_s).to_s, a.to_s )
    assert_equal( a.to_s, "('hallo',r0 10,-15)" )

    a = RBA::Text::new( RBA::DText::new( "dtext",  RBA::DTrans::new( RBA::DTrans::R270, RBA::DPoint::new( 100.0, -150.0 ))))
    assert_equal( a.to_s, "('dtext',r270 100,-150)" )

    a = RBA::Text::new
    assert_equal( a.to_s, "('',r0 0,0)" )

    b = a.dup 
    a = RBA::Text::new( "hallo", RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 10, -15 )))
    assert_equal( a.to_s, "('hallo',r90 10,-15)" )
    assert_equal( RBA::Text::from_s(a.to_s).to_s, a.to_s )
    c = a.dup 

    assert_equal( a == b, false )
    assert_equal( a == c, true )
    assert_equal( a != b, true )
    assert_equal( a != c, false )

    assert_equal( a.string, "hallo" )
    assert_equal( a.trans.to_s, "r90 10,-15" )

    a.string = "hi"
    assert_equal( a.string, "hi" )

    a.trans = RBA::Trans::new( RBA::Trans::M45, RBA::Point::new( 5, 7 ))
    assert_equal( a.trans.to_s, "m45 5,7" )

    aa = a.dup
    assert_equal( aa.x.to_s, "5" )
    assert_equal( aa.y.to_s, "7" )
    aa.x = -3
    assert_equal( aa.trans.to_s, "m45 -3,7" )
    aa.y = -6
    assert_equal( aa.trans.to_s, "m45 -3,-6" )

    a = RBA::Text::new( "hallo", a.trans, 22, 7 )
    assert_equal( a.string, "hallo" )
    assert_equal( a.trans.to_s, "m45 5,7" )
    assert_equal( a.position.to_s, "5,7" )
    assert_equal( a.bbox.to_s, "(5,7;5,7)" )
    assert_equal( a.font, 7 )
    assert_equal( a.size, 22.0 )

    a.font = 8
    assert_equal( a.font, 8 )

    a.halign = 1
    assert_equal( a.halign.to_i, 1 )
    assert_equal( a.halign, RBA::Text::HAlignCenter )
    assert_equal( a.halign.to_s, "HAlignCenter" )

    a.halign = RBA::Text::HAlignLeft
    assert_equal( a.halign.to_i, 0 )
    assert_equal( a.halign, RBA::Text::HAlignLeft )
    assert_equal( a.halign.to_s, "HAlignLeft" )

    a.valign = 1
    assert_equal( a.valign.to_i, 1 )
    assert_equal( a.valign, RBA::Text::VAlignCenter )
    assert_equal( a.valign.to_s, "VAlignCenter" )

    a.valign = RBA::Text::VAlignTop
    assert_equal( a.valign.to_i, 0 )
    assert_equal( a.valign, RBA::Text::VAlignTop )
    assert_equal( a.valign.to_s, "VAlignTop" )

    a.size = 23
    assert_equal( a.size, 23 )

    b = a.dup

    assert_equal( a.moved( RBA::Point::new( 0, 1 ) ).to_s, "('hallo',m45 5,8) s=23 f=8 ha=l va=t" )
    a.move( RBA::Point::new( 1, 0 ) )
    assert_equal( a.to_s, "('hallo',m45 6,7) s=23 f=8 ha=l va=t" )

    b = b.transformed( RBA::Trans::new( RBA::Trans::R0, RBA::Point::new( 1, 0 )) )
    assert_equal( b.to_s, "('hallo',m45 6,7) s=23 f=8 ha=l va=t" )

    m = RBA::CplxTrans::new( RBA::Trans::new, 1.5 )
    assert_equal( a.transformed(m).to_s, "('hallo',m45 9,10.5) s=34.5 f=8 ha=l va=t" )
    assert_equal( a.transformed(RBA::ICplxTrans::new(m)).to_s, "('hallo',m45 9,11) s=35 f=8 ha=l va=t" )

  end

  # Fuzzy compare
  def test_2_Text

    a1 = RBA::DText::new( "hallo", 10.0, -15.0 )
    a2 = RBA::DText::new( "hallo", 10.0 + 1e-7, -15.0 )
    a3 = RBA::DText::new( "hallo", 10.0 + 1e-4, -15.0 )
    a4 = RBA::DText::new( "hllo", 10.0, -15.0 )

    assert_equal(a1 == a2, true)
    assert_equal(a1 != a2, false)
    assert_equal(a1.eql?(a2), true)
    assert_equal(a1 < a2, false)
    assert_equal(a2 < a1, false)

    assert_equal(a1 == a3, false)
    assert_equal(a1 != a3, true)
    assert_equal(a1.eql?(a3), false)
    assert_equal(a1 < a3, true)
    assert_equal(a3 < a1, false)

    assert_equal(a1 == a4, false)
    assert_equal(a1 != a4, true)
    assert_equal(a1.eql?(a4), false)
    assert_equal(a1 < a4, true)
    assert_equal(a4 < a1, false)

  end

  # Hash function
  def test_3_Text

    a1 = RBA::DText::new( "hallo", 10.0, -15.0 )
    a2 = RBA::DText::new( "hallo", 10.0 + 1e-7, -15.0 )
    a3 = RBA::DText::new( "hallo", 10.0 + 1e-4, -15.0 )
    a4 = RBA::DText::new( "hllo", 10.0, -15.0 )

    h = { a1 => "a1", a3 => "a3", a4 => "a4" }
    
    assert_equal(h[a1], "a1")
    assert_equal(h[a2], "a1")
    assert_equal(h[a3], "a3")
    assert_equal(h[a4], "a4")

  end

end

load("test_epilogue.rb")

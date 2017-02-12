
$:.push(File::dirname($0))

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
    assert_equal( a.font, 7 )
    assert_equal( a.size, 22.0 )

    a.font = 8
    assert_equal( a.font, 8 )

    a.size = 23.0
    assert_equal( a.size, 23.0 )

    b = a.dup

    assert_equal( a.moved( RBA::DPoint::new( 0, 1 ) ).to_s, "('hallo',m45 5,8)" )
    a.move( RBA::DPoint::new( 1, 0 ) )
    assert_equal( a.to_s, "('hallo',m45 6,7)" )

    b = b.transformed( RBA::DTrans::new( RBA::DTrans::R0, RBA::DPoint::new( 1, 0 )) )
    assert_equal( b.to_s, "('hallo',m45 6,7)" )

    m = RBA::DCplxTrans::new( RBA::DTrans::new, 1.5 )
    assert_equal( a.transformed(m).to_s, "('hallo',m45 9,10.5)" )

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
    assert_equal( a.font, 7 )
    assert_equal( a.size, 22.0 )

    a.font = 8
    assert_equal( a.font, 8 )

    a.size = 23
    assert_equal( a.size, 23 )

    b = a.dup

    assert_equal( a.moved( RBA::Point::new( 0, 1 ) ).to_s, "('hallo',m45 5,8)" )
    a.move( RBA::Point::new( 1, 0 ) )
    assert_equal( a.to_s, "('hallo',m45 6,7)" )

    b = b.transformed( RBA::Trans::new( RBA::Trans::R0, RBA::Point::new( 1, 0 )) )
    assert_equal( b.to_s, "('hallo',m45 6,7)" )

    m = RBA::CplxTrans::new( RBA::Trans::new, 1.5 )
    assert_equal( a.transformed(m).to_s, "('hallo',m45 9,10.5)" )
    assert_equal( a.transformed(RBA::ICplxTrans::new(m)).to_s, "('hallo',m45 9,11)" )

  end

end

load("test_epilogue.rb")


$:.push(File::dirname($0))

load("test_prologue.rb")

class DBPoint_TestClass < TestBase

  # DPoint basics
  def test_1_DPoint

    a = RBA::DPoint::new( 1, -17 )
    b = RBA::DPoint::new
    c = RBA::DPoint::new( RBA::Point::new( 5, 11 ) )

    assert_equal( a.to_s, "1,-17" )
    assert_equal( RBA::DPoint::from_s(a.to_s).to_s, a.to_s )
    assert_equal( (-a).to_s, "-1,17" )
    assert_equal( b.to_s, "0,0" )
    assert_equal( c.to_s, "5,11" )

    assert_equal( (a + c).to_s, "6,-6" )
    assert_equal( (a * 0.5).to_s, "0.5,-8.5" )
    assert_equal( (a - c).to_s, "-4,-28" )
    assert_equal( a == c, false )
    assert_equal( a == a, true )
    assert_equal( a != c, true )
    assert_equal( a != a, false )

    assert_equal( a.x.to_s, "1.0" )
    assert_equal( a.y.to_s, "-17.0" )

    assert_equal( (a.distance(c) - Math::sqrt(800.0)).abs < 1e-12, true )
    assert_equal( ((a-c).length - Math::sqrt(800.0)).abs < 1e-12, true )

    assert_equal( a.sq_distance(c).to_s, "800.0" )
    assert_equal( (a-c).sq_length.to_s, "800.0" )

    b.x = a.x
    b.y = a.y
    assert_equal( a, b )

  end

  # Transforming DPoint
  def test_2_DPoint

    a = RBA::DPoint::new( 1, -17 )
    b = RBA::DPoint::new

    t = RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 5, -2 )) 
    assert_equal( t.trans(a).to_s, "22,-1" )

    m = RBA::DCplxTrans::new( RBA::DTrans::new( RBA::DTrans::R90, RBA::DPoint::new( 5, -2 )), 0.5 ) 
    assert_equal( m.trans(a).to_s, "13.5,-1.5" )

  end

  # Point basics
  def test_1_Point

    a = RBA::Point::new( 1, -17 )
    b = RBA::Point::new
    c = RBA::Point::new( RBA::DPoint::new( 5, 11 ) )

    assert_equal( a.to_s, "1,-17" )
    assert_equal( RBA::Point::from_s(a.to_s).to_s, a.to_s )
    assert_equal( (-a).to_s, "-1,17" )
    assert_equal( b.to_s, "0,0" )
    assert_equal( c.to_s, "5,11" )

    assert_equal( (a + c).to_s, "6,-6" )
    assert_equal( (a * 0.5).to_s, "1,-9" )
    assert_equal( (a - c).to_s, "-4,-28" )
    assert_equal( a == c, false )
    assert_equal( a == a, true )
    assert_equal( a != c, true )
    assert_equal( a != a, false )

    assert_equal( a.x.to_s, "1" )
    assert_equal( a.y.to_s, "-17" )

    assert_equal( (a.distance(c) - Math::sqrt(800.0)).abs < 1e-12, true )

    assert_equal( a.sq_distance(c).to_s, "800.0" )

    b.x = a.x
    b.y = a.y
    assert_equal( a, b )

  end

  # Transforming Point
  def test_2_Point

    a = RBA::Point::new( 1, -17 )
    b = RBA::Point::new

    t = RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 5, -2 )) 
    assert_equal( t.trans(a).to_s, "22,-1" )

    m = RBA::CplxTrans::new( RBA::Trans::new( RBA::Trans::R90, RBA::Point::new( 5, -2 )), 0.5 ) 
    assert_equal( m.trans(a).to_s, "13.5,-1.5" )

  end

end

load("test_epilogue.rb")

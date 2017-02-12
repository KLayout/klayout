
$:.push(File::dirname($0))

load("test_prologue.rb")

class DBEdgePair_TestClass < TestBase

  # Basics
  def test_1

    ep = RBA::EdgePair::new
    assert_equal(ep.to_s, "(0,0;0,0)/(0,0;0,0)")

    ep.first = RBA::Edge::new(0, 0, 10, 20)
    assert_equal(ep.to_s, "(0,0;10,20)/(0,0;0,0)")

    ep.second = RBA::Edge::new(-10, 0, -10, 30)
    assert_equal(ep.to_s, "(0,0;10,20)/(-10,0;-10,30)")
    assert_equal(ep.bbox.to_s, "(-10,0;10,30)")

    assert_equal(RBA::EdgePair::new(ep.first, ep.second).to_s, "(0,0;10,20)/(-10,0;-10,30)")

    ep2 = RBA::EdgePair::new
    assert_equal(ep2.to_s, "(0,0;0,0)/(0,0;0,0)")
    ep2.assign(ep)
    assert_equal(ep2.to_s, ep.to_s)

    assert_equal(ep.normalized.to_s, "(10,20;0,0)/(-10,0;-10,30)")

    assert_equal(ep.transformed(RBA::Trans::new(1)).to_s, "(0,0;-20,10)/(0,-10;-30,-10)")
    assert_equal(ep.transformed(RBA::ICplxTrans::new(2.0)).to_s, "(0,0;20,40)/(-20,0;-20,60)")
    assert_equal(ep.transformed(RBA::CplxTrans::new(2.0)).to_s, "(0,0;20,40)/(-20,0;-20,60)")
    assert_equal(RBA::EdgePair::new(ep.transformed(RBA::CplxTrans::new(2.0))).to_s, "(0,0;20,40)/(-20,0;-20,60)")

    assert_equal(ep.polygon(0).to_s, "(-10,0;-10,30;0,0;10,20)")
    assert_equal(ep.polygon(0).class.to_s, "RBA::Polygon")
    assert_equal(ep.simple_polygon(0).to_s, "(-10,0;-10,30;0,0;10,20)")
    assert_equal(ep.simple_polygon(0).class.to_s, "RBA::SimplePolygon")

  end

  # Basics
  def test_2

    ep = RBA::DEdgePair::new
    assert_equal(ep.to_s, "(0,0;0,0)/(0,0;0,0)")

    ep.first = RBA::DEdge::new(0, 0, 10, 20)
    assert_equal(ep.to_s, "(0,0;10,20)/(0,0;0,0)")

    ep.second = RBA::DEdge::new(-10, 0, -10, 30)
    assert_equal(ep.to_s, "(0,0;10,20)/(-10,0;-10,30)")
    assert_equal(ep.bbox.to_s, "(-10,0;10,30)")

    assert_equal(RBA::DEdgePair::new(ep.first, ep.second).to_s, "(0,0;10,20)/(-10,0;-10,30)")

    ep2 = RBA::DEdgePair::new
    assert_equal(ep2.to_s, "(0,0;0,0)/(0,0;0,0)")
    ep2.assign(ep)
    assert_equal(ep2.to_s, ep.to_s)

    assert_equal(ep.normalized.to_s, "(10,20;0,0)/(-10,0;-10,30)")

    assert_equal(ep.transformed(RBA::DTrans::new(1)).to_s, "(0,0;-20,10)/(0,-10;-30,-10)")
    assert_equal(ep.transformed(RBA::DCplxTrans::new(2.5)).to_s, "(0,0;25,50)/(-25,0;-25,75)")

    assert_equal(ep.polygon(0).to_s, "(-10,0;-10,30;0,0;10,20)")
    assert_equal(ep.polygon(0).class.to_s, "RBA::DPolygon")
    assert_equal(ep.simple_polygon(0).to_s, "(-10,0;-10,30;0,0;10,20)")
    assert_equal(ep.simple_polygon(0).class.to_s, "RBA::DSimplePolygon")

  end

end

load("test_epilogue.rb")

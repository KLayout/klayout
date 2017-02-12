
$:.push(File::dirname($0))

load("test_prologue.rb")

class DBEdgePairs_TestClass < TestBase

  # Basics
  def test_1

    r = RBA::EdgePairs::new
    assert_equal(r.to_s, "")
    assert_equal(r.is_empty?, true)
    assert_equal(r.size, 0)
    assert_equal(r.bbox.to_s, "()")

    r.insert(RBA::Edge::new(0, 0, 0, 100), RBA::Edge::new(-10, 0, -20, 50))
    assert_equal(r.to_s, "(0,0;0,100)/(-10,0;-20,50)")
    r.clear

    r.insert(RBA::EdgePair::new(RBA::Edge::new(0, 0, 0, 100), RBA::Edge::new(-10, 0, -20, 50)))
    assert_equal(r.to_s, "(0,0;0,100)/(-10,0;-20,50)")
    assert_equal(r.extents.to_s, "(-20,0;-20,100;0,100;0,0)")
    assert_equal(r.extents(10).to_s, "(-30,-10;-30,110;10,110;10,-10)")
    assert_equal(r.extents(5, -5).to_s, "(-25,5;-25,95;5,95;5,5)")
    assert_equal(r.first_edges.to_s, "(0,0;0,100)")
    assert_equal(r.second_edges.to_s, "(-10,0;-20,50)")
    assert_equal(r.edges.to_s, "(0,0;0,100);(-10,0;-20,50)")
    assert_equal(r.is_empty?, false)
    assert_equal(r.size, 1)
    assert_equal(r[0].to_s, "(0,0;0,100)/(-10,0;-20,50)")
    assert_equal(r[1].to_s, "")
    assert_equal(r.bbox.to_s, "(-20,0;0,100)")

    assert_equal(r.moved(-10, 10).to_s, "(-10,10;-10,110)/(-20,10;-30,60)")
    assert_equal(r.moved(RBA::Point::new(-10, 10)).to_s, "(-10,10;-10,110)/(-20,10;-30,60)")
    rr = r.dup
    rr.move(-10, 10)
    assert_equal(rr.to_s, "(-10,10;-10,110)/(-20,10;-30,60)")
    rr = r.dup
    rr.move(RBA::Point::new(-10, 10))
    assert_equal(rr.to_s, "(-10,10;-10,110)/(-20,10;-30,60)")

    assert_equal(r.transformed(RBA::Trans::new(1)).to_s, "(0,0;-100,0)/(0,-10;-50,-20)")
    assert_equal(r.transformed(RBA::ICplxTrans::new(2.0)).to_s, "(0,0;0,200)/(-20,0;-40,100)")
    rr = r.dup
    rr.transform(RBA::Trans::new(1))
    assert_equal(rr.to_s, "(0,0;-100,0)/(0,-10;-50,-20)")
    rr = r.dup
    rr.transform(RBA::ICplxTrans::new(2.0))
    assert_equal(rr.to_s, "(0,0;0,200)/(-20,0;-40,100)")

    rr = RBA::EdgePairs::new
    rr.swap(r)
    assert_equal(rr.to_s, "(0,0;0,100)/(-10,0;-20,50)")
    assert_equal(r.to_s, "")
    rr.swap(r)
    assert_equal(r.to_s, "(0,0;0,100)/(-10,0;-20,50)")
    r.clear

    assert_equal(r.to_s, "")
    assert_equal(r.is_empty?, true)
    assert_equal(r.size, 0)
    assert_equal(r.bbox.to_s, "()")

  end

  # Basics
  def test_2

    r1 = RBA::EdgePairs::new
    r1.insert(RBA::Edge::new(0, 0, 0, 100), RBA::Edge::new(-10, 0, -20, 50))
    r1.insert(RBA::Edge::new(0, 1, 0, 101), RBA::Edge::new(-10, 1, -20, 51))

    r2 = RBA::EdgePairs::new
    r2.insert(RBA::Edge::new(1, 0, 1, 100), RBA::Edge::new(-11, 0, -21, 50))
    r2.insert(RBA::Edge::new(1, 1, 1, 101), RBA::Edge::new(-11, 1, -21, 51))

    assert_equal((r1 + r2).to_s, "(0,0;0,100)/(-10,0;-20,50);(0,1;0,101)/(-10,1;-20,51);(1,0;1,100)/(-11,0;-21,50);(1,1;1,101)/(-11,1;-21,51)")
    r1 += r2
    assert_equal(r1.to_s, "(0,0;0,100)/(-10,0;-20,50);(0,1;0,101)/(-10,1;-20,51);(1,0;1,100)/(-11,0;-21,50);(1,1;1,101)/(-11,1;-21,51)")

  end

end


load("test_epilogue.rb")

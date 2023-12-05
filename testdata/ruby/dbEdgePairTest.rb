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

class DBEdgePair_TestClass < TestBase

  # Basics
  def test_1

    ep = RBA::EdgePair::new
    assert_equal(ep.to_s, "(0,0;0,0)/(0,0;0,0)")

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

    ep = RBA::EdgePair::new(RBA::Edge::new(0, 0, 10, 0), RBA::Edge::new(0, 20, 0, 0))
  
    assert_equal(ep.distance, 0)
    assert_equal(ep.perimeter, 30)
    assert_equal(ep.area, 100)
    assert_equal(ep.simple_polygon(0).area, 100)

    ep = RBA::EdgePair::new(RBA::Edge::new(0, 0, 10, 0), RBA::Edge::new(0, 0, 0, 20))
  
    assert_equal(ep.perimeter, 30)
    assert_equal(ep.area, 0)
    assert_equal(ep.simple_polygon(0).area, 0)

    ep = RBA::EdgePair::new(RBA::Edge::new(0, 0, 10, 0), RBA::Edge::new(-10, 10, 20, 10))
  
    assert_equal(ep.distance, 10)

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
    assert_equal(ep.transformed(RBA::DCplxTrans::new(2.5)).class.to_s, "RBA::DEdgePair")
    assert_equal(ep.transformed(RBA::DCplxTrans::new(2.5)).to_s, "(0,0;25,50)/(-25,0;-25,75)")
    assert_equal(ep.transformed(RBA::VCplxTrans::new(2.5)).class.to_s, "RBA::EdgePair")
    assert_equal(ep.transformed(RBA::VCplxTrans::new(2.5)).to_s, "(0,0;25,50)/(-25,0;-25,75)")

    assert_equal(ep.polygon(0).to_s, "(-10,0;-10,30;0,0;10,20)")
    assert_equal(ep.polygon(0).class.to_s, "RBA::DPolygon")
    assert_equal(ep.simple_polygon(0).to_s, "(-10,0;-10,30;0,0;10,20)")
    assert_equal(ep.simple_polygon(0).class.to_s, "RBA::DSimplePolygon")

    ep = RBA::DEdgePair::new(RBA::DEdge::new(0, 0, 10, 0), RBA::DEdge::new(0, 20, 0, 0))
  
    assert_equal(ep.distance, 0)
    assert_equal(ep.perimeter, 30)
    assert_equal(ep.area, 100)
    assert_equal(ep.simple_polygon(0).area, 100)

    ep = RBA::DEdgePair::new(RBA::DEdge::new(0, 0, 10, 0), RBA::DEdge::new(0, 0, 0, 20))
  
    assert_equal(ep.perimeter, 30)
    assert_equal(ep.area, 0)
    assert_equal(ep.simple_polygon(0).area, 0)

    ep = RBA::DEdgePair::new(RBA::DEdge::new(0, 0, 10, 0), RBA::DEdge::new(-10, 10, 20, 10))
  
    assert_equal(ep.distance, 10)

  end

  # Fuzzy compare
  def test_3

    b1 = RBA::DEdgePair::new(RBA::DEdge::new(1, 2, 3, 4), RBA::DEdge::new(11, 12, 13, 14))
    b2a = RBA::DEdgePair::new(RBA::DEdge::new(1 + 1e-7, 2, 3, 4), RBA::DEdge::new(11, 12, 13, 14))
    b2b = RBA::DEdgePair::new(RBA::DEdge::new(1, 2, 3, 4), RBA::DEdge::new(11 + 1e-7, 12, 13, 14))
    b3a = RBA::DEdgePair::new(RBA::DEdge::new(1 + 1e-4, 2, 3, 4), RBA::DEdge::new(11, 12, 13, 14))
    b3b = RBA::DEdgePair::new(RBA::DEdge::new(1, 2, 3, 4), RBA::DEdge::new(11 + 1e-4, 12, 13, 14))
    assert_equal(b1.to_s, "(1,2;3,4)/(11,12;13,14)")
    assert_equal(b2a.to_s, "(1.0000001,2;3,4)/(11,12;13,14)")
    assert_equal(b2b.to_s, "(1,2;3,4)/(11.0000001,12;13,14)")
    assert_equal(b3a.to_s, "(1.0001,2;3,4)/(11,12;13,14)")
    assert_equal(b3b.to_s, "(1,2;3,4)/(11.0001,12;13,14)")

    assert_equal(b1 == b2a, true)
    assert_equal(b1.eql?(b2a), true)
    assert_equal(b1 != b2a, false)
    assert_equal(b1 < b2a, false)
    assert_equal(b2a < b1, false)
    assert_equal(b1 == b2b, true)
    assert_equal(b1.eql?(b2b), true)
    assert_equal(b1 != b2b, false)
    assert_equal(b1 < b2b, false)
    assert_equal(b2b < b1, false)
    assert_equal(b2a == b2b, true)
    assert_equal(b2a.eql?(b2b), true)
    assert_equal(b2a != b2b, false)
    assert_equal(b2a < b2b, false)
    assert_equal(b2b < b2a, false)

    assert_equal(b1 == b3a, false)
    assert_equal(b1 == b3b, false)
    assert_equal(b1.eql?(b3a), false)
    assert_equal(b1.eql?(b3b), false)
    assert_equal(b1 != b3a, true)
    assert_equal(b1 != b3b, true)
    assert_equal(b1 < b3a, true)
    assert_equal(b1 < b3b, true)
    assert_equal(b3a < b1, false)
    assert_equal(b3b < b1, false)
    assert_equal(b3b < b3a, true)
    assert_equal(b3a < b3b, false)

  end

  # Hash values 
  def test_4

    b1 = RBA::DEdgePair::new(RBA::DEdge::new(1, 2, 3, 4), RBA::DEdge::new(11, 12, 13, 14))
    b2a = RBA::DEdgePair::new(RBA::DEdge::new(1 + 1e-7, 2, 3, 4), RBA::DEdge::new(11, 12, 13, 14))
    b2b = RBA::DEdgePair::new(RBA::DEdge::new(1, 2, 3, 4), RBA::DEdge::new(11 + 1e-7, 12, 13, 14))
    b3a = RBA::DEdgePair::new(RBA::DEdge::new(1 + 1e-4, 2, 3, 4), RBA::DEdge::new(11, 12, 13, 14))
    b3b = RBA::DEdgePair::new(RBA::DEdge::new(1, 2, 3, 4), RBA::DEdge::new(11 + 1e-4, 12, 13, 14))

    assert_equal(b1.hash == b2a.hash, true)
    assert_equal(b1.hash == b2b.hash, true)
    assert_equal(b1.hash == b3a.hash, false)
    assert_equal(b1.hash == b3b.hash, false)
    assert_equal(b3a.hash == b3b.hash, false)

    h = { b1 => "a", b3a => "b", b3b => "c" }
    assert_equal(h[b1], "a")
    assert_equal(h[b2a], "a")
    assert_equal(h[b2b], "a")
    assert_equal(h[b3a], "b")
    assert_equal(h[b3b], "c")

  end

  # Symmetric edge pairs
  def test_5

    b1 = RBA::DEdgePair::new(RBA::DEdge::new(1, 2, 3, 4), RBA::DEdge::new(11, 12, 13, 14), false)
    b1x = RBA::DEdgePair::new(RBA::DEdge::new(11, 12, 13, 14), RBA::DEdge::new(1, 2, 3, 4), false)
    b2a = RBA::DEdgePair::new(RBA::DEdge::new(1, 2, 3, 4), RBA::DEdge::new(11, 12, 13, 14), true)
    b2b = RBA::DEdgePair::new(RBA::DEdge::new(11, 12, 13, 14), RBA::DEdge::new(1, 2, 3, 4), true)

    assert_equal(b1.hash == b1x.hash, false)
    assert_equal(b1.hash == b2a.hash, false)
    assert_equal(b1.hash == b2b.hash, false)
    assert_equal(b2a.hash == b2b.hash, true)

    assert_equal(b1 < b1x, true)
    assert_equal(b1 == b1x, false)
    assert_equal(b1 < b2a, true)
    assert_equal(b1 == b2a, false)
    assert_equal(b1 < b2b, true)
    assert_equal(b2a < b2b, false)
    assert_equal(b2a == b2b, true)
    assert_equal(b2b < b2a, false)

    assert_equal(b1.to_s, "(1,2;3,4)/(11,12;13,14)")
    assert_equal(b1x.to_s, "(11,12;13,14)/(1,2;3,4)")
    assert_equal(b2a.to_s, "(1,2;3,4)|(11,12;13,14)")
    assert_equal(b2b.to_s, "(1,2;3,4)|(11,12;13,14)")
    
    h = {}
    h[b1] = 1
    h[b1x] = 2
    assert_equal(h.size, 2)
    assert_equal(h.keys.collect(&:to_s).join(","), "(1,2;3,4)/(11,12;13,14),(11,12;13,14)/(1,2;3,4)")

    h = {}
    h[b1] = 1
    h[b2a] = 2
    assert_equal(h.size, 2)
    assert_equal(h.keys.collect(&:to_s).join(","), "(1,2;3,4)/(11,12;13,14),(1,2;3,4)|(11,12;13,14)")

    h = {}
    h[b2a] = 1
    h[b2b] = 2
    assert_equal(h.size, 1)
    assert_equal(h.keys.collect(&:to_s).join(","), "(1,2;3,4)|(11,12;13,14)")

  end

end

load("test_epilogue.rb")

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


class PEX_TestClass < TestBase

  # PEX basics
  def test_1_Basic

    rn = RBA::RNetwork::new

    a = rn.create_node(RBA::RNode::VertexPort, 1)
    b = rn.create_node(RBA::RNode::Internal, 2)
    c = rn.create_node(RBA::RNode::PolygonPort, 3)

    assert_equal(a.type, RBA::RNode::VertexPort)
    assert_equal(a.port_index, 1)
    assert_equal(a.object_id, a.object_id)
    assert_not_equal(a.object_id, b.object_id)
    assert_equal(a.to_s, "V1")

    assert_equal(b.to_s, "$2")
    assert_equal(c.to_s, "P3")

    rab = rn.create_element(1.0, a, b)
    assert_equal(rab.a.object_id, a.object_id)
    assert_equal(rab.b.object_id, b.object_id)
    assert_equal(rab.to_s, "R $2 V1 1")

    rn.create_element(1.0, a, b)
    assert_equal(rab.to_s, "R $2 V1 0.5")

    rbc = rn.create_element(1.0, b, c)

    assert_equal(rn.to_s, "R $2 V1 0.5\n" + "R $2 P3 1")

    assert_equal(b.each_element.collect(&:to_s).sort.join(";"), "R $2 P3 1;R $2 V1 0.5")
    assert_equal(rn.each_element.collect(&:to_s).sort.join(";"), "R $2 P3 1;R $2 V1 0.5")
    assert_equal(rn.each_node.collect(&:to_s).sort.join(";"), "$2;P3;V1")

    rn.simplify
    assert_equal(rn.to_s, "R P3 V1 1.5")

    rn.clear
    assert_equal(rn.to_s, "")

  end

  def test_2_Destroy

    rn = RBA::RNetwork::new

    a = rn.create_node(RBA::RNode::VertexPort, 1)
    b = rn.create_node(RBA::RNode::Internal, 2)
    rab = rn.create_element(1.0, a, b)

    # this should invalid the pointers
    rn._destroy

    begin
      assert_equal(a.to_s, "")
    rescue => ex
      # graph has been destroyed already
    end

    begin
      assert_equal(rab.to_s, "")
    rescue => ex
      # graph has been destroyed already
    end

  end

  def test_3_SQC

    poly = RBA::Polygon::new(RBA::Box::new(0, 0, 1100, 100))

    vp = [ RBA::Point::new(50, 50) ]
    pp = [ RBA::Polygon::new(RBA::Box::new(1000, 0, 1100, 100)) ]

    rex = RBA::RExtractor::square_counting_extractor(0.001)
    rn = rex.extract(poly, vp, pp)

    assert_equal(rn.to_s, "R P0 V0 10")

  end

  def test_3_TX

    poly = RBA::Polygon::new(RBA::Box::new(0, 0, 1100, 100))

    vp = [ RBA::Point::new(50, 50) ]
    pp = [ RBA::Polygon::new(RBA::Box::new(1000, 0, 1100, 100)) ]

    rex = RBA::RExtractor::tesselation_extractor(0.001, 0.8)
    rn = rex.extract(poly, vp, pp)

    assert_equal(rn.to_s, "R P0 V0 9.44")

  end

end

load("test_epilogue.rb")


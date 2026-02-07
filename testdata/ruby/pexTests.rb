# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2026 Matthias Koefferlein
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
    d = rn.create_node(RBA::RNode::PolygonPort, 3, 17)

    assert_equal(a.type, RBA::RNode::VertexPort)
    assert_equal(a.port_index, 1)
    assert_equal(a.object_id, a.object_id)
    assert_not_equal(a.object_id, b.object_id)
    assert_equal(a.to_s, "V1")

    assert_equal(b.to_s, "$2")
    assert_equal(c.to_s, "P3")
    assert_equal(d.to_s, "P3.17")

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
    assert_equal(rn.each_node.collect(&:to_s).sort.join(";"), "$2;P3;P3.17;V1")

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

  def test_4_ExtractorTech

    l1  = 1
    l2  = 2
    l3  = 3

    tech = RBA::RExtractorTech::new

    tech.skip_simplify = true
    assert_equal(tech.skip_simplify, true)
    tech.skip_simplify = false
    assert_equal(tech.skip_simplify, false)

    via1 = RBA::RExtractorTechVia::new
    via1.bottom_conductor = l1
    via1.cut_layer = l2
    via1.top_conductor = l3
    via1.resistance = 2.0
    via1.merge_distance = 0.2
    assert_equal(via1.to_s, "Via(bottom=L1, cut=L2, top=L3, R=2 µm²*Ohm, d_merge=0.2 µm)")

    assert_equal(via1.bottom_conductor, l1)
    assert_equal(via1.cut_layer, l2)
    assert_equal(via1.top_conductor, l3)
    assert_equal(via1.resistance, 2.0)
    assert_equal(via1.merge_distance.to_s, "0.2")

    tech.add_via(via1)
    assert_equal(tech.each_via.collect { |v| v.cut_layer }, [ l2 ])

    tech.clear_vias
    assert_equal(tech.each_via.collect { |v| v.cut_layer }, [])

    tech.add_via(via1)
    assert_equal(tech.each_via.collect { |v| v.cut_layer }, [ l2 ])

    cond1 = RBA::RExtractorTechConductor::new
    cond1.layer = l1
    cond1.resistance = 0.5
    assert_equal(cond1.to_s, "Conductor(layer=L1, R=0.5 Ohm/sq, algo=SquareCounting)")

    assert_equal(cond1.layer, l1)
    assert_equal(cond1.resistance, 0.5)

    cond2 = RBA::RExtractorTechConductor::new
    cond2.layer = l3
    cond2.resistance = 0.25

    tech.add_conductor(cond2)
    assert_equal(tech.each_conductor.collect { |c| c.layer }, [ l3 ])
    assert_equal(tech.to_s, 
        "Via(bottom=L1, cut=L2, top=L3, R=2 µm²*Ohm, d_merge=0.2 µm)\n" +
        "Conductor(layer=L3, R=0.25 Ohm/sq, algo=SquareCounting)"
    )

    tech.clear_conductors
    assert_equal(tech.each_conductor.collect { |c| c.layer }, [])

    tech.add_conductor(cond1)
    tech.add_conductor(cond2)
    assert_equal(tech.each_conductor.collect { |c| c.layer }, [ l1, l3 ])

  end

  # A complete, small example for a R network extraction

  def test_5_NetEx

    ly = RBA::Layout::new
    ly.read(File.join($ut_testsrc, "testdata", "pex", "netex_test1.gds"))

    rex = RBA::RNetExtractor::new(ly.dbu)

    tc = ly.top_cell

    l1  = ly.layer(1, 0)
    l1p = ly.layer(1, 1)
    l1v = ly.layer(1, 2)
    l2  = ly.layer(2, 0)
    l3  = ly.layer(3, 0)
    l3p = ly.layer(3, 1)
    l3v = ly.layer(3, 2)

    #  That is coincidence, but it needs to be that way for the strings to match
    assert_equal(l1, 1)
    assert_equal(l2, 0)
    assert_equal(l3, 2)

    geo = {}
    [ l1, l2, l3 ].each do |l|
      geo[l] = RBA::Region::new(tc.begin_shapes_rec(l))
    end

    tech = RBA::RExtractorTech::new

    via1 = RBA::RExtractorTechVia::new
    via1.bottom_conductor = l1
    via1.cut_layer = l2
    via1.top_conductor = l3
    via1.resistance = 2.0
    via1.merge_distance = 0.2

    tech.add_via(via1)

    cond1 = RBA::RExtractorTechConductor::new
    cond1.layer = l1
    cond1.resistance = 0.5

    cond2 = RBA::RExtractorTechConductor::new
    cond2.layer = l3
    cond2.resistance = 0.25

    tech.add_conductor(cond1)
    tech.add_conductor(cond2)

    polygon_ports = { }
    polygon_ports[l1] = RBA::Region::new(tc.begin_shapes_rec(l1p)).each_merged.to_a
    polygon_ports[l3] = RBA::Region::new(tc.begin_shapes_rec(l3p)).each_merged.to_a

    vertex_ports = { }
    vertex_ports[l1] = RBA::Region::new(tc.begin_shapes_rec(l1v)).each_merged.collect { |p| p.bbox.center }
    vertex_ports[l3] = RBA::Region::new(tc.begin_shapes_rec(l3v)).each_merged.collect { |p| p.bbox.center }

    network = rex.extract(tech, geo, vertex_ports, polygon_ports)

    n = network.to_s(true)
    n = n.gsub(/ \$\d+\./, " $x.")
    n = n.split("\n").sort.join("\n") + "\n"
    assert_equal(n, <<"END")
R $x.1(9.3,-5.9;9.9,-5.3) $x.2(10,-3.5;10,-2.7) 13.2813
R $x.1(9.3,-5.9;9.9,-5.3) P0.1(12.9,-5.9;13.5,-5.3) 2.25
R $x.1(9.3,-5.9;9.9,-5.3) V0.2(0.3,-5.7;0.5,-5.5) 55.75
R $x.2(10,-3.5;10,-2.7) P0.2(12.9,-3.4;13.5,-2.8) 1
R $x.2(10,-3.5;10,-2.7) V0.1(5.2,0.4;5.2,0.4) 28.7812
R V0.1(5.2,0.4;5.2,0.4) V0.2(0.3,-5.7;0.5,-5.5) 17.375
END

  end

end

load("test_epilogue.rb")


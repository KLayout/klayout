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

class EXT_TestClass < TestBase

  def test_1

    ly = RBA::Layout::new
    ly.read(ENV["TESTSRC"] + "/testdata/net_tracer/t1.oas.gz")

    tracer = RBA::NetTracer::new
    
    tech = RBA::NetTracerConnectivity::new
    tech.connection("1/0", "2/0", "3/0")

    tracer.trace(tech, ly, ly.top_cell, RBA::Point::new(7000, 1500), ly.find_layer(1, 0))

    assert_equal(tracer.num_elements, 14)
    assert_equal(tracer.name, "THE_NAME")
    assert_equal(tracer.incomplete?, false)

    ref = [
      "TOP: box (0,0;21550,3300) r0 *1 0,0 @1/0",
      "TOP: box (13210,-17730;16930,-4790) r0 *1 0,0 @3/0",
      "TOP: box (13300,-20030;35190,-15870) r0 *1 0,0 @3/0",
      "TOP: box (2240,-28980;5510,-7040) r0 *1 0,0 @3/0",
      "TOP: box (26150,-18260;41090,-13030) r0 *1 0,0 @2/0",
      "TOP: box (26330,-28770;31730,-25200) r0 *1 0,0 @2/0",
      "TOP: box (27920,-16660;31640,-1950) r0 *1 0,0 @1/0",
      "TOP: box (3060,-9290;15710,-5710) r0 *1 0,0 @3/0",
      "TOP: box (3160,-29180;30610,-26220) r0 *1 0,0 @3/0",
      "TOP: box (34620,-16450;38340,-1740) r0 *1 0,0 @1/0",
      "TOP: box (44080,-29080;55920,-23160) r0 *1 0,0 @1/0",
      "TOP: path (27860,-24180;27860,-39490;46530,-39490;46530,-24180) w=2000 bx=0 ex=0 r=false r0 *1 0,0 @1/0",
      "TOP: simple_polygon (29340,-4430;29340,1680;18440,1680;18440,11610;35990,11610;35990,-4430) r0 *1 0,0 @1/0",
      "TOP: text ('THE_NAME',r0 3580,-17750) r0 *1 0,0 @3/0"
    ]

    res = []
    tracer.each_element do |e|
      res << ly.cell(e.cell_index).name + ": " + e.shape.to_s + " " + e.trans.to_s + " @" + ly.get_info(e.layer).to_s
    end
    assert_equal(res.sort, ref.sort)

    tracer.clear

    assert_equal(tracer.num_elements, 0)

  end

  def test_2

    ly = RBA::Layout::new
    ly.read(ENV["TESTSRC"] + "/testdata/net_tracer/t1.oas.gz")

    tracer = RBA::NetTracer::new
    
    tech = RBA::NetTracerConnectivity::new
    tech.connection("1/0", "2/0", "3/0")

    tracer.trace(tech, ly, ly.top_cell, RBA::Point::new(7000, 1500), ly.find_layer(1, 0), RBA::Point::new(6000, -6000), ly.find_layer(3, 0))

    assert_equal(tracer.num_elements, 7)
    assert_equal(tracer.incomplete?, false)

    ref1 = [
      "TOP: box (0,0;21550,3300) r0 *1 0,0 @1/0",
      "TOP: box (13210,-17730;16930,-4790) r0 *1 0,0 @3/0",
      "TOP: box (13300,-20030;35190,-15870) r0 *1 0,0 @3/0",
      "TOP: box (26150,-18260;41090,-13030) r0 *1 0,0 @2/0",
      "TOP: box (27920,-16660;31640,-1950) r0 *1 0,0 @1/0",
      "TOP: box (3060,-9290;15710,-5710) r0 *1 0,0 @3/0",
      "TOP: simple_polygon (29340,-4430;29340,1680;18440,1680;18440,11610;35990,11610;35990,-4430) r0 *1 0,0 @1/0",
    ]

    ref2 = [
      "TOP: box (0,0;21550,3300) r0 *1 0,0 @1/0",
      "TOP: box (13210,-17730;16930,-4790) r0 *1 0,0 @3/0",
      "TOP: box (13300,-20030;35190,-15870) r0 *1 0,0 @3/0",
      "TOP: box (26150,-18260;41090,-13030) r0 *1 0,0 @2/0",
      "TOP: box (3060,-9290;15710,-5710) r0 *1 0,0 @3/0",
      "TOP: box (34620,-16450;38340,-1740) r0 *1 0,0 @1/0",
      "TOP: simple_polygon (29340,-4430;29340,1680;18440,1680;18440,11610;35990,11610;35990,-4430) r0 *1 0,0 @1/0",
    ]

    res = []
    tracer.each_element do |e|
      res << ly.cell(e.cell_index).name + ": " + e.shape.to_s + " " + e.trans.to_s + " @" + ly.get_info(e.layer).to_s
    end
    # NOTE: both paths (ref1 and ref2) are valid. Currently, the algorithm picks a "random" path 
    # (pointer values in maps?)
    assert_equal(res.sort == ref1.sort || res.sort == ref2.sort, true)

  end

  def test_3

    ly = RBA::Layout::new
    ly.read(ENV["TESTSRC"] + "/testdata/net_tracer/t1.oas.gz")

    tracer = RBA::NetTracer::new
    
    tech = RBA::NetTracerConnectivity::new
    tech.connection("1/0", "3/0")

    tracer.trace(tech, ly, ly.top_cell, RBA::Point::new(7000, 1500), ly.find_layer(1, 0))

    assert_equal(tracer.num_elements, 12)
    assert_equal(tracer.name, "THE_NAME")
    assert_equal(tracer.incomplete?, false)

    ref = [
      "TOP: box (0,0;21550,3300) r0 *1 0,0 @1/0",
      "TOP: box (13210,-17730;16930,-4790) r0 *1 0,0 @3/0",
      "TOP: box (13300,-20030;35190,-15870) r0 *1 0,0 @3/0",
      "TOP: box (2240,-28980;5510,-7040) r0 *1 0,0 @3/0",
      "TOP: box (27920,-16660;31640,-1950) r0 *1 0,0 @1/0",
      "TOP: box (3060,-9290;15710,-5710) r0 *1 0,0 @3/0",
      "TOP: box (3160,-29180;30610,-26220) r0 *1 0,0 @3/0",
      "TOP: box (34620,-16450;38340,-1740) r0 *1 0,0 @1/0",
      "TOP: box (44080,-29080;55920,-23160) r0 *1 0,0 @1/0",
      "TOP: path (27860,-24180;27860,-39490;46530,-39490;46530,-24180) w=2000 bx=0 ex=0 r=false r0 *1 0,0 @1/0",
      "TOP: simple_polygon (29340,-4430;29340,1680;18440,1680;18440,11610;35990,11610;35990,-4430) r0 *1 0,0 @1/0",
      "TOP: text ('THE_NAME',r0 3580,-17750) r0 *1 0,0 @3/0"
    ]

    res = []
    tracer.each_element do |e|
      res << ly.cell(e.cell_index).name + ": " + e.shape.to_s + " " + e.trans.to_s + " @" + ly.get_info(e.layer).to_s
    end
    assert_equal(res.sort, ref.sort)

    tracer.clear

    assert_equal(tracer.num_elements, 0)

  end

  # Technology component
  def test_4

    c1 = RBA::NetTracerConnectivity::new
    c1.connection("1/0", "3/0")
    c1.name = "1to3"

    c2 = RBA::NetTracerConnectivity::new
    c2.connection("2/0", "3/0")
    c2.name = "2to3"

    tc = RBA::NetTracerTechnologyComponent::new
    names = tc.each.collect { |c| c.name }.join(";")
    assert_equal(names, "")

    tc.add(c1)
    names = tc.each.collect { |c| c.name }.join(";")
    assert_equal(names, "1to3")

    tc.add(c2)
    names = tc.each.collect { |c| c.name }.join(";")
    assert_equal(names, "1to3;2to3")

    tc.clear
    names = tc.each.collect { |c| c.name }.join(";")
    assert_equal(names, "")

    tc.add(c2)
    names = tc.each.collect { |c| c.name }.join(";")
    assert_equal(names, "2to3")

  end

end

load("test_epilogue.rb")


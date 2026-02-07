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

class StringCollectingOutputReceiver < RBA::TileOutputReceiver
  attr_accessor :data
  def begin(nx, ny, p0, dx, dy, frame)
    @data = []
    @data.push("begin nx=#{nx} ny=#{ny} p0=#{p0.to_s} dx=#{dx} dy=#{dy}")
  end
  def put(ix, iy, tile, obj, dbu, clip)
    GC.start  # makes Ruby crash if started from a non-ruby thread
    @data ||= []
    @data.push("put ix=#{ix} iy=#{iy} tile=#{tile.to_s} obj=#{obj.to_s} dbu=#{dbu} clip=#{clip.to_s}")
  end
  def finish(success)
    @data.push("finish")
  end
end

class DBTilingProcessor_TestClass < TestBase

  def test_1

    rin = RBA::Region::new
    rin.insert(RBA::Box::new(0, 0, 100, 200))
    rin.insert(RBA::Box::new(9900, 9800, 10000, 10000))

    rall = RBA::Region::new
    rall.insert(RBA::Box::new(0, 0, 10000, 10000))

    tp = RBA::TilingProcessor::new
    tp.input("in", rin)
    tp.input("all", rall)

    rout = RBA::Region::new

    tp.output("out", rout)

    tp.dbu = 0.1
    tp.tile_size(200.0, 500.0)
    tp.queue("_output(out, all-in)")
    tp.execute("A job")

    rout.merged_semantics = false
    assert_equal(rout.size, 10)
    assert_equal(rout.area, 100000000-20000*2)
    assert_equal(rout.to_s, "(100,0;100,200;0,200;0,5000;2000,5000;2000,0);(0,5000;0,10000;2000,10000;2000,5000);(2000,0;2000,5000;4000,5000;4000,0);(2000,5000;2000,10000;4000,10000;4000,5000);(4000,0;4000,5000;6000,5000;6000,0);(4000,5000;4000,10000;6000,10000;6000,5000);(6000,0;6000,5000;8000,5000;8000,0);(6000,5000;6000,10000;8000,10000;8000,5000);(8000,0;8000,5000;10000,5000;10000,0);(8000,5000;8000,10000;9900,10000;9900,9800;10000,9800;10000,5000)")

  end

  def test_2

    rin = RBA::Region::new
    rin.insert(RBA::Box::new(0, 0, 100, 200))
    rin.insert(RBA::Box::new(9900, 9800, 10000, 10000))

    rall = RBA::Region::new
    rall.insert(RBA::Box::new(0, 0, 10000, 10000))

    tp = RBA::TilingProcessor::new
    tp.input("in", rin)
    tp.input("all", rall)

    rout = RBA::Region::new

    tp.output("out", rout)

    tp.dbu = 0.1
    tp.tile_size(20.0, 50.0)
    tp.queue("_output(out, all-in)")
    tp.execute("A job")
    tp.threads = 4

    rout.merged_semantics = false
    assert_equal(rout.size, 1000)
    assert_equal(rout.area, 100000000-20000*2)

  end

  def test_3

    rin = RBA::Region::new
    rin.insert(RBA::Box::new(0, 0, 100, 200))
    rin.insert(RBA::Box::new(9900, 9800, 10000, 10000))

    tp = RBA::TilingProcessor::new
    tp.input("in", rin)

    out = StringCollectingOutputReceiver::new
    tp.output("out", out)

    tp.dbu = 0.1
    tp.tile_size(200.0, 500.0)
    tp.queue("_output(out, _tile.to_s)")
    tp.execute("A job")

    assert_equal(out.data.join("\n") + "\n", <<END)
begin nx=5 ny=2 p0=0,0 dx=200.0 dy=500.0
put ix=0 iy=0 tile=(0,0;2000,5000) obj=(0,0;0,5000;2000,5000;2000,0) dbu=0.1 clip=true
put ix=0 iy=1 tile=(0,5000;2000,10000) obj=(0,5000;0,10000;2000,10000;2000,5000) dbu=0.1 clip=true
put ix=1 iy=0 tile=(2000,0;4000,5000) obj=(2000,0;2000,5000;4000,5000;4000,0) dbu=0.1 clip=true
put ix=1 iy=1 tile=(2000,5000;4000,10000) obj=(2000,5000;2000,10000;4000,10000;4000,5000) dbu=0.1 clip=true
put ix=2 iy=0 tile=(4000,0;6000,5000) obj=(4000,0;4000,5000;6000,5000;6000,0) dbu=0.1 clip=true
put ix=2 iy=1 tile=(4000,5000;6000,10000) obj=(4000,5000;4000,10000;6000,10000;6000,5000) dbu=0.1 clip=true
put ix=3 iy=0 tile=(6000,0;8000,5000) obj=(6000,0;6000,5000;8000,5000;8000,0) dbu=0.1 clip=true
put ix=3 iy=1 tile=(6000,5000;8000,10000) obj=(6000,5000;6000,10000;8000,10000;8000,5000) dbu=0.1 clip=true
put ix=4 iy=0 tile=(8000,0;10000,5000) obj=(8000,0;8000,5000;10000,5000;10000,0) dbu=0.1 clip=true
put ix=4 iy=1 tile=(8000,5000;10000,10000) obj=(8000,5000;8000,10000;10000,10000;10000,5000) dbu=0.1 clip=true
finish
END

    tp.threads = 4
    tp.execute("A job")

    assert_equal(out.data.sort.join("\n") + "\n", <<END)
begin nx=5 ny=2 p0=0,0 dx=200.0 dy=500.0
finish
put ix=0 iy=0 tile=(0,0;2000,5000) obj=(0,0;0,5000;2000,5000;2000,0) dbu=0.1 clip=true
put ix=0 iy=1 tile=(0,5000;2000,10000) obj=(0,5000;0,10000;2000,10000;2000,5000) dbu=0.1 clip=true
put ix=1 iy=0 tile=(2000,0;4000,5000) obj=(2000,0;2000,5000;4000,5000;4000,0) dbu=0.1 clip=true
put ix=1 iy=1 tile=(2000,5000;4000,10000) obj=(2000,5000;2000,10000;4000,10000;4000,5000) dbu=0.1 clip=true
put ix=2 iy=0 tile=(4000,0;6000,5000) obj=(4000,0;4000,5000;6000,5000;6000,0) dbu=0.1 clip=true
put ix=2 iy=1 tile=(4000,5000;6000,10000) obj=(4000,5000;4000,10000;6000,10000;6000,5000) dbu=0.1 clip=true
put ix=3 iy=0 tile=(6000,0;8000,5000) obj=(6000,0;6000,5000;8000,5000;8000,0) dbu=0.1 clip=true
put ix=3 iy=1 tile=(6000,5000;8000,10000) obj=(6000,5000;6000,10000;8000,10000;8000,5000) dbu=0.1 clip=true
put ix=4 iy=0 tile=(8000,0;10000,5000) obj=(8000,0;8000,5000;10000,5000;10000,0) dbu=0.1 clip=true
put ix=4 iy=1 tile=(8000,5000;10000,10000) obj=(8000,5000;8000,10000;10000,10000;10000,5000) dbu=0.1 clip=true
END


  end

  def test_4

    rin = RBA::Region::new
    rin.insert(RBA::Box::new(0, 0, 100, 200))
    rin.insert(RBA::Box::new(9900, 9800, 10000, 10000))

    tp = RBA::TilingProcessor::new
    tp.input("in", rin)

    out = StringCollectingOutputReceiver::new
    tp.output("out", out)

    tp.var("myvar", false)
    tp.var("myvar2", "!")

    tp.dbu = 0.1
    tp.tile_size(200.0, 500.0)
    tp.tile_origin(10.0, 5.0)
    tp.queue("_output(out, _tile.bbox.width + myvar2, myvar)")
    tp.execute("A job")

    assert_equal(out.data.join("\n") + "\n", <<END)
begin nx=5 ny=2 p0=10,5 dx=200.0 dy=500.0
put ix=0 iy=0 tile=(100,50;2100,5050) obj=2000! dbu=0.1 clip=false
put ix=0 iy=1 tile=(100,5050;2100,10050) obj=2000! dbu=0.1 clip=false
put ix=1 iy=0 tile=(2100,50;4100,5050) obj=2000! dbu=0.1 clip=false
put ix=1 iy=1 tile=(2100,5050;4100,10050) obj=2000! dbu=0.1 clip=false
put ix=2 iy=0 tile=(4100,50;6100,5050) obj=2000! dbu=0.1 clip=false
put ix=2 iy=1 tile=(4100,5050;6100,10050) obj=2000! dbu=0.1 clip=false
put ix=3 iy=0 tile=(6100,50;8100,5050) obj=2000! dbu=0.1 clip=false
put ix=3 iy=1 tile=(6100,5050;8100,10050) obj=2000! dbu=0.1 clip=false
put ix=4 iy=0 tile=(8100,50;10100,5050) obj=2000! dbu=0.1 clip=false
put ix=4 iy=1 tile=(8100,5050;10100,10050) obj=2000! dbu=0.1 clip=false
finish
END

  end

  def test_5

    rin = RBA::Region::new
    rin.insert(RBA::Box::new(0, 0, 100, 200))
    rin.insert(RBA::Box::new(9900, 9800, 10000, 10000))

    tp = RBA::TilingProcessor::new
    tp.input("in", rin)

    out = StringCollectingOutputReceiver::new
    tp.output("out", out)

    tp.dbu = 0.1
    tp.tiles(5, 2)
    tp.queue("_output(out, _tile.to_s)")
    tp.execute("A job")

    assert_equal(out.data.join("\n") + "\n", <<END)
begin nx=5 ny=2 p0=0,0 dx=200.0 dy=500.0
put ix=0 iy=0 tile=(0,0;2000,5000) obj=(0,0;0,5000;2000,5000;2000,0) dbu=0.1 clip=true
put ix=0 iy=1 tile=(0,5000;2000,10000) obj=(0,5000;0,10000;2000,10000;2000,5000) dbu=0.1 clip=true
put ix=1 iy=0 tile=(2000,0;4000,5000) obj=(2000,0;2000,5000;4000,5000;4000,0) dbu=0.1 clip=true
put ix=1 iy=1 tile=(2000,5000;4000,10000) obj=(2000,5000;2000,10000;4000,10000;4000,5000) dbu=0.1 clip=true
put ix=2 iy=0 tile=(4000,0;6000,5000) obj=(4000,0;4000,5000;6000,5000;6000,0) dbu=0.1 clip=true
put ix=2 iy=1 tile=(4000,5000;6000,10000) obj=(4000,5000;4000,10000;6000,10000;6000,5000) dbu=0.1 clip=true
put ix=3 iy=0 tile=(6000,0;8000,5000) obj=(6000,0;6000,5000;8000,5000;8000,0) dbu=0.1 clip=true
put ix=3 iy=1 tile=(6000,5000;8000,10000) obj=(6000,5000;6000,10000;8000,10000;8000,5000) dbu=0.1 clip=true
put ix=4 iy=0 tile=(8000,0;10000,5000) obj=(8000,0;8000,5000;10000,5000;10000,0) dbu=0.1 clip=true
put ix=4 iy=1 tile=(8000,5000;10000,10000) obj=(8000,5000;8000,10000;10000,10000;10000,5000) dbu=0.1 clip=true
finish
END

  end

  def test_6

    rin = RBA::Region::new
    rin.insert(RBA::Box::new(0, 0, 100, 200))
    rin.insert(RBA::Box::new(9900, 9800, 10000, 10000))

    rall = RBA::Region::new
    rall.insert(RBA::Box::new(0, 0, 10000, 10000))

    tp = RBA::TilingProcessor::new
    tp.input("in", rin)
    tp.input("all", rall)

    out = RBA::Value::new
    out.value = 0.0
    tp.output("out", out)

    tp.dbu = 0.1
    tp.tile_size(200.0, 500.0)
    tp.queue("_output(out, (all-in).area(_tile.bbox))")
    tp.execute("A job")

    assert_equal(out.value, 99960000.0)

  end

  def test_7

    if !RBA.constants.member?(:Image)
      return
    end

    rin = RBA::Region::new
    rin.insert(RBA::Box::new(0, 0, 100, 200))
    rin.insert(RBA::Box::new(9900, 9800, 10000, 10000))

    rall = RBA::Region::new
    rall.insert(RBA::Box::new(0, 0, 10000, 10000))

    tp = RBA::TilingProcessor::new
    tp.input("in", rin)
    tp.input("all", rall)

    img = RBA::Image::new
    tp.output("out", img)

    tp.dbu = 0.1
    tp.tile_size(200.0, 500.0)
    tp.queue("_output(out, to_f((all-in).area(_tile.bbox)) / _tile.area)")
    tp.execute("A job")

    assert_equal(img.to_s, "mono:matrix=(200,0,500) (0,500,500) (0,0,1);min_value=0;max_value=1;is_visible=true;z_position=0;brightness=0;contrast=0;gamma=1;red_gain=1;green_gain=1;blue_gain=1;color_mapping=[0,'#000000';1,'#ffffff';];width=5;height=2;data=[0.998;1;1;1;1;1;1;1;1;0.998;]")

  end

end

load("test_epilogue.rb")

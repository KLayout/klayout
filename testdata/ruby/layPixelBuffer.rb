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

class LAYPixelBuffer_TestClass < TestBase

  def compare(pb1, pb2)
    if pb1.width != pb2.width || pb1.height != pb2.height
      return false
    end
    pb1.width.times do |x|
      pb1.height.times do |y|
        if pb1.pixel(x, y) != pb2.pixel(x, y)
          return false
        end
      end
    end
    return true
  end

  def test_1

    pb_null = RBA::PixelBuffer::new
    assert_equal(pb_null.width, 0)
    assert_equal(pb_null.height, 0)

    pb = RBA::PixelBuffer::new(10, 20)
    assert_equal(compare(pb_null, pb), false)
    assert_equal(pb_null == pb, false)
    assert_equal(pb.width, 10)
    assert_equal(pb.height, 20)
    assert_equal(pb.transparent, false)

    pb_copy = pb.dup
    pb.transparent = true
    assert_equal(pb.transparent, true)
    assert_equal(pb_copy == pb, false)

    pb.fill(0xf0010203)
    assert_equal(pb.pixel(0, 0), 0xf0010203)
    assert_equal(pb.pixel(1, 2), 0xf0010203)

    pb.set_pixel(1, 2, 0xff102030)
    assert_equal(pb.pixel(0, 0), 0xf0010203)
    assert_equal(pb.pixel(1, 2), 0xff102030)

    pb.transparent = false
    assert_equal(pb.transparent, false)

    pb_copy = pb.dup
    assert_equal(compare(pb_copy, pb), true)
    assert_equal(pb_copy == pb, true)
    pb.set_pixel(1, 2, 0x112233)
    assert_equal(pb.pixel(0, 0), 0xf0010203)
    assert_equal(pb.pixel(1, 2), 0xff112233)
    assert_equal(pb_copy.pixel(1, 2), 0xff102030)

    assert_equal(compare(pb_copy, pb), false)
    assert_equal(pb_copy == pb, false)

    pb_copy.swap(pb)
    assert_equal(pb_copy.pixel(1, 2), 0xff112233)
    assert_equal(pb.pixel(1, 2), 0xff102030)

  end

  def test_2

    pb = RBA::PixelBuffer::new(10, 20)
    pb.fill(0xf0010203)

    pb1 = pb.dup
    pb.set_pixel(1, 2, 0x112233)

    assert_equal(compare(pb1, pb), false)
    assert_equal(pb1 == pb, false)
    
    diff = pb1.diff(pb)
    pb1.patch(diff)
    assert_equal(compare(pb1, pb), true)
    assert_equal(pb1 == pb, true)

  end

  def test_3

    pb = RBA::PixelBuffer::new(10, 20)
    pb.fill(0xf0010203)

    pb1 = pb.dup
    pb.set_pixel(1, 2, 0x112233)

    pb1 = pb.dup
    pb.set_pixel(1, 2, 0xf0112233)
    assert_equal(compare(pb1, pb), true)
    assert_equal(pb1 == pb, true)  # not transparent -> alpha is ignored

    pb1.transparent = true
    pb.transparent = true
    assert_equal(compare(pb1, pb), true)
    assert_equal(pb1 == pb, true)

    pb.set_pixel(1, 2, 0xf0112233)
    assert_equal(compare(pb1, pb), false)
    assert_equal(pb1 == pb, false)  # now, alpha matters

  end

  def test_4
    
    pb = RBA::PixelBuffer::new(10, 20)
    pb.transparent = true
    pb.fill(0xf0010203)

    if pb.respond_to?(:to_qimage)
      assert_equal(pb.to_qimage.pixel(2, 3), 0xf0010203)
      pb_copy = RBA::PixelBuffer::from_qimage(pb.to_qimage)
      assert_equal(compare(pb, pb_copy), true)
    end

    png = pb.to_png_data

    assert_equal(png.size > 20 && png.size < 200, true)  # some range because implementations may differ
    pb_copy = RBA::PixelBuffer.from_png_data(png)
    assert_equal(compare(pb, pb_copy), true)

    tmp = File::join($ut_testtmp, "tmp.png")
    pb.write_png(tmp)
    pb_copy = RBA::PixelBuffer.read_png(tmp)
    assert_equal(compare(pb, pb_copy), true)

  end

  def test_11

    pb = RBA::BitmapBuffer::new
    assert_equal(pb.width, 0)
    assert_equal(pb.height, 0)

    pb = RBA::BitmapBuffer::new(10, 20)
    assert_equal(pb.width, 10)
    assert_equal(pb.height, 20)

    pb.fill(false)
    assert_equal(pb.pixel(0, 0), false)
    assert_equal(pb.pixel(1, 2), false)

    pb.set_pixel(1, 2, true)
    assert_equal(pb.pixel(0, 0), false)
    assert_equal(pb.pixel(1, 2), true)

    pb_copy = pb.dup
    assert_equal(compare(pb_copy, pb), true)
    pb.set_pixel(1, 3, true)
    assert_equal(pb.pixel(0, 0), false)
    assert_equal(pb.pixel(1, 2), true)
    assert_equal(pb.pixel(1, 3), true)
    assert_equal(pb_copy.pixel(1, 3), false)

    assert_equal(compare(pb_copy, pb), false)

    pb_copy.swap(pb)
    assert_equal(pb.pixel(1, 3), false)
    assert_equal(pb_copy.pixel(1, 3), true)

  end

  def test_12
    
    pb = RBA::BitmapBuffer::new(10, 20)
    pb.fill(false)
    pb.set_pixel(2, 3, true)

    if pb.respond_to?(:to_qimage)
      assert_equal(pb.to_qimage.pixel(0, 0), 0xff000000)
      assert_equal(pb.to_qimage.pixel(2, 3), 0xffffffff)
      pb_copy = RBA::BitmapBuffer::from_qimage(pb.to_qimage)
      assert_equal(compare(pb, pb_copy), true)
    end

    png = nil
    begin
      png = pb.to_png_data
    rescue => ex
      # No PNG support
    end

    if png

      assert_equal(png.size > 20 && png.size < 200, true)  # some range because implementations may differ
      pb_copy = RBA::BitmapBuffer.from_png_data(png)
      assert_equal(compare(pb, pb_copy), true)

      tmp = File::join($ut_testtmp, "tmp.png")
      pb.write_png(tmp)
      pb_copy = RBA::BitmapBuffer.read_png(tmp)
      assert_equal(compare(pb, pb_copy), true)

    end

  end

end

load("test_epilogue.rb")

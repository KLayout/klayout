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

class DBUtils_TestClass < TestBase

  def test_1

    pts = [ RBA::DPoint::new(-1, 0), RBA::DPoint::new(-1, 1), RBA::DPoint::new(0, 1) ]
    weights = [ 1, Math::sqrt(0.5), 1 ]
    k = [ 0, 0, 0, 1, 1, 1 ]

    pts = RBA::Utils::spline_interpolation(pts, weights, 2, k, 0.01, 0.01)

    assert_equal(pts.collect(&:to_s).join(","), "-1,0,-0.983305368417,0.181963052412,-0.929788301062,0.368094709562,-0.836995511219,0.547209753385,-0.707106781187,0.707106781187,-0.547209753385,0.836995511219,-0.368094709562,0.929788301062,-0.181963052412,0.983305368417,0,1") 
    assert_equal(pts.collect(&:abs).collect { |d| "%.12g" % d }.join(","), "1,1,1,1,1,1,1,1,1")

  end

  def test_2

    pts = [ RBA::Point::new(-1000, 0), RBA::Point::new(-1000, 1000), RBA::Point::new(0, 1000) ]
    weights = [ 1, Math::sqrt(0.5), 1]
    k = [ 0, 0, 0, 1, 1, 1 ]

    pts = RBA::Utils::spline_interpolation(pts, weights, 2, k, 0.01, 10)

    assert_equal(pts.collect(&:to_s).join(","), "-1000,0,-983,182,-930,368,-837,547,-707,707,-547,837,-368,930,-182,983,0,1000")
    assert_equal(pts.collect(&:abs).collect { |d| "%.0f" % d }.join(","), "1000,1000,1000,1000,1000,1000,1000,1000,1000")

  end

  def test_3

    pts = [ RBA::DPoint::new(-1, 0), RBA::DPoint::new(-1, 1), RBA::DPoint::new(0, 1) ]
    k = [ 0, 0, 0, 1, 1, 1 ]

    pts = RBA::Utils::spline_interpolation(pts, 2, k, 0.01, 0.01)

    assert_equal(pts.collect(&:to_s).join(","), "-1,0,-0.984375,0.234375,-0.9375,0.4375,-0.859375,0.609375,-0.75,0.75,-0.609375,0.859375,-0.4375,0.9375,-0.234375,0.984375,0,1")

  end

  def test_4

    pts = [ RBA::Point::new(-1000, 0), RBA::Point::new(-1000, 1000), RBA::Point::new(0, 1000) ]
    k = [ 0, 0, 0, 1, 1, 1 ]

    pts = RBA::Utils::spline_interpolation(pts, 2, k, 0.01, 10)

    assert_equal(pts.collect(&:to_s).join(","), "-1000,0,-984,234,-938,438,-859,609,-750,750,-609,859,-438,938,-234,984,0,1000")
    assert_equal(pts.collect(&:abs).collect { |d| "%.0f" % d }.join(","), "1000,1011,1035,1053,1061,1053,1035,1011,1000")

  end

end

load("test_epilogue.rb")

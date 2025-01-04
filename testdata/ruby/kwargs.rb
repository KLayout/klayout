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


# NOTE: RBA::CplxTrans and RBA::Trans and good test cases
# for the keyword arguments feature

class KWArgs_TestClass < TestBase

  def test_1

    t = RBA::CplxTrans::new()
    assert_equal(t.to_s, "r0 *1 0,0")

    t = RBA::CplxTrans::new(1.5)
    assert_equal(t.to_s, "r0 *1.5 0,0")

    t = RBA::CplxTrans::new(1, 2)
    assert_equal(t.to_s, "r0 *1 1,2")

    t = RBA::CplxTrans::new(1, y: 2)
    assert_equal(t.to_s, "r0 *1 1,2")

    t = RBA::CplxTrans::new(x: 1, y: 2)
    assert_equal(t.to_s, "r0 *1 1,2")

    t = RBA::CplxTrans::new(u: RBA::DVector::new(1, 2))
    assert_equal(t.to_s, "r0 *1 1,2")

    t = RBA::CplxTrans::new(RBA::DVector::new(1, 2))
    assert_equal(t.to_s, "r0 *1 1,2")

    t = RBA::CplxTrans::new(u: RBA::Vector::new(1, 2))
    assert_equal(t.to_s, "r0 *1 1,2")

    t = RBA::CplxTrans::new(u: [1, 2])
    assert_equal(t.to_s, "r0 *1 1,2")

    t = RBA::CplxTrans::new(mag: 1.5)
    assert_equal(t.to_s, "r0 *1.5 0,0")

    t = RBA::CplxTrans::new(1.5, 45, true, 1, 2)
    assert_equal(t.to_s, "m22.5 *1.5 1,2")

    t = RBA::CplxTrans::new(1.5, 45, true, RBA::DVector::new(1, 2))
    assert_equal(t.to_s, "m22.5 *1.5 1,2")

    t = RBA::CplxTrans::new(1.5, x: 1, y: 2, mirrx: true, rot: 45)
    assert_equal(t.to_s, "m22.5 *1.5 1,2")

    t = RBA::CplxTrans::new(RBA::CplxTrans::M0)
    assert_equal(t.to_s, "m0 *1 0,0")

    t = RBA::CplxTrans::new(RBA::CplxTrans::M0, u: RBA::DVector::new(1, 2))
    assert_equal(t.to_s, "m0 *1 1,2")

    t = RBA::CplxTrans::new(RBA::CplxTrans::M0, mag: 1.5, u: RBA::DVector::new(1, 2))
    assert_equal(t.to_s, "m0 *1.5 1,2")

    t = RBA::CplxTrans::new(RBA::CplxTrans::M0, 1.5, RBA::DVector::new(1, 2))
    assert_equal(t.to_s, "m0 *1.5 1,2")

    t = RBA::CplxTrans::new(RBA::CplxTrans::M0, mag: 1.5, x: 1, y: 2)
    assert_equal(t.to_s, "m0 *1.5 1,2")

    t = RBA::CplxTrans::new(RBA::CplxTrans::M0, 1.5, 1, 2)
    assert_equal(t.to_s, "m0 *1.5 1,2")

    t = RBA::CplxTrans::new(RBA::VCplxTrans::M0)
    assert_equal(t.to_s, "m0 *1 0,0")

    t = RBA::CplxTrans::new(RBA::ICplxTrans::M0)
    assert_equal(t.to_s, "m0 *1 0,0")

    t = RBA::CplxTrans::new(RBA::DCplxTrans::M0)
    assert_equal(t.to_s, "m0 *1 0,0")

    t = RBA::CplxTrans::new(RBA::Trans::M0)
    assert_equal(t.to_s, "m0 *1 0,0")

    t = RBA::CplxTrans::new(RBA::Trans::M0, 1.5)
    assert_equal(t.to_s, "m0 *1.5 0,0")

    t = RBA::CplxTrans::new(RBA::Trans::M0, mag: 1.5)
    assert_equal(t.to_s, "m0 *1.5 0,0")
    
    t = RBA::CplxTrans::new(t: RBA::Trans::M0, mag: 1.5)
    assert_equal(t.to_s, "m0 *1.5 0,0")

    t = RBA::CplxTrans::new
    t.disp = [1, 2]
    assert_equal(t.to_s, "r0 *1 1,2")

    t = RBA::ICplxTrans::new(15, 25)
    assert_equal(t.to_s(dbu: 0.01), "r0 *1 0.15000,0.25000")
    
  end

  def test_2

    t = RBA::Trans::new(RBA::Trans::M0, 1, 2)
    assert_equal(t.to_s, "m0 1,2")

    t = RBA::Trans::new(RBA::Trans::M0, x: 1, y: 2)
    assert_equal(t.to_s, "m0 1,2")

    t = RBA::Trans::new(RBA::Trans::M0, RBA::Vector::new(1, 2))
    assert_equal(t.to_s, "m0 1,2")

    t = RBA::Trans::new(RBA::Trans::M0, u: RBA::Vector::new(1, 2))
    assert_equal(t.to_s, "m0 1,2")

    t = RBA::Trans::new(rot: 3, mirrx: true)
    assert_equal(t.to_s, "m135 0,0")

    t = RBA::Trans::new(rot: 3, mirrx: true, x: 1, y: 2)
    assert_equal(t.to_s, "m135 1,2")

    t = RBA::Trans::new(3, true, 1, 2)
    assert_equal(t.to_s, "m135 1,2")

    t = RBA::Trans::new(3, true, RBA::Vector::new(1, 2))
    assert_equal(t.to_s, "m135 1,2")

    t = RBA::Trans::new(rot: 3, mirrx: true, u: RBA::Vector::new(1, 2))
    assert_equal(t.to_s, "m135 1,2")

    t = RBA::Trans::new
    assert_equal(t.to_s, "r0 0,0")

    t = RBA::Trans::new(RBA::DTrans::M0)
    assert_equal(t.to_s, "m0 0,0")

    t = RBA::Trans::new(RBA::DTrans::M0, 1, 2)
    assert_equal(t.to_s, "m0 1,2")

    t = RBA::Trans::new(RBA::DTrans::M0, x: 1, y: 2)
    assert_equal(t.to_s, "m0 1,2")

    t = RBA::Trans::new(c: RBA::DTrans::M0, x: 1, y: 2)
    assert_equal(t.to_s, "m0 1,2")

    t = RBA::Trans::new(RBA::Vector::new(1, 2))
    assert_equal(t.to_s, "r0 1,2")

    t = RBA::Trans::new(1, 2)
    assert_equal(t.to_s, "r0 1,2")

  end

  def test_3

    begin
      t = RBA::CplxTrans::new(1.5, 2.5)
      t.to_s(dbu: "17")
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "TypeError: no implicit conversion to float from string for argument #2 ('dbu') in CplxTrans::to_s")
    end

    begin
      t = RBA::CplxTrans::new(1.5, 2.5)
      tt = RBA::CplxTrans::new
      tt.assign(other: t)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s, "Keyword arguments not permitted in CplxTrans::assign")
    end

    begin
      t = RBA::CplxTrans::new("17")
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s.index("No overload with matching arguments."), 0)
    end

    begin
      t = RBA::CplxTrans::new(uu: 17)
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s.index("Can't match arguments."), 0)
    end

    begin
      t = RBA::CplxTrans::new(u: "17")
      assert_equal(true, false)
    rescue => ex
      assert_equal(ex.to_s.index("No overload with matching arguments."), 0)
    end

  end

end

load("test_epilogue.rb")


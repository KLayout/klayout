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

class DBCellInst_TestClass < TestBase

  def ci2str(ci)
    a = []
    ci.each_trans { |t| a << t.to_s }
    a.join(";")
  end

  def cci2str(ci)
    a = []
    ci.each_cplx_trans { |t| a << t.to_s }
    a.join(";")
  end

  # CellInstArray
  def test_0_CellInstArray

    a = RBA::CellInstArray::new
    assert_equal(a.to_s, "#0 r0 0,0")
    assert_equal(a.size, 1)

    a = RBA::CellInstArray::new(0, RBA::Trans::new(RBA::Trans::R90))
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r90 0,0")
    assert_equal(a.cplx_trans.to_s, "r90 *1 0,0")

    a = RBA::CellInstArray::new(0, RBA::Vector::new(42, -17))
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r0 42,-17")
    assert_equal(a.cplx_trans.to_s, "r0 *1 42,-17")

    a = RBA::CellInstArray::new(0, RBA::ICplxTrans::new(1.5))
    assert_equal(a.is_complex?, true)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r0 *1.5 0,0")

    a = RBA::CellInstArray::new(0, RBA::ICplxTrans::new(1.0))
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r0 *1 0,0")

    a.transform(RBA::Trans::R90)
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r90 0,0")
    assert_equal(a.cplx_trans.to_s, "r90 *1 0,0")

    at = a.transformed(RBA::ICplxTrans::new(1.5))
    assert_equal(a.is_complex?, false)
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r90 *1.5 0,0")

    a = RBA::CellInstArray::new(0, RBA::ICplxTrans::new(1.0, 45, false, RBA::Vector::new))
    assert_equal(a.is_complex?, true)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r45 *1 0,0")

    at = a.transformed(RBA::Trans::R90)
    atdup = at.dup
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r135 *1 0,0")

    assert_equal(at < a, false)
    assert_equal(at < atdup, false)
    assert_equal(a < at, true)
    assert_equal(atdup < at, false)
    assert_equal(a != at, true)
    assert_equal(a == at, false)
    assert_equal(atdup != at, false)
    assert_equal(atdup == at, true)

    at.transform(RBA::ICplxTrans::new(2.5))
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r135 *2.5 0,0")

    at.invert
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r180 0,0")
    assert_equal(at.cplx_trans.to_s, "r225 *0.4 0,0")

    a = RBA::CellInstArray::new(0, RBA::Trans::new(RBA::Trans::R90), RBA::Vector::new(10, 20), RBA::Vector::new(30, 40), 3, 5)
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r90 0,0")
    assert_equal(a.cplx_trans.to_s, "r90 *1 0,0")
    assert_equal(a.to_s, "#0 r90 0,0 [10,20*3;30,40*5]")

    a = RBA::CellInstArray::new(0, RBA::Vector::new(42, -17), RBA::Vector::new(10, 20), RBA::Vector::new(30, 40), 3, 5)
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r0 42,-17")
    assert_equal(a.cplx_trans.to_s, "r0 *1 42,-17")
    assert_equal(a.to_s, "#0 r0 42,-17 [10,20*3;30,40*5]")

    a = RBA::CellInstArray::new(0, RBA::ICplxTrans::new(1.5), RBA::Vector::new(10, 20), RBA::Vector::new(30, 40), 3, 5)
    assert_equal(a.is_complex?, true)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r0 *1.5 0,0")
    assert_equal(a.to_s, "#0 r0 *1.5 0,0 [10,20*3;30,40*5]")

    a = RBA::CellInstArray::new(0, RBA::ICplxTrans::new(1.0), RBA::Vector::new(10, 20), RBA::Vector::new(30, 40), 3, 5)
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r0 *1 0,0")

    a.transform(RBA::Trans::R90)
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r90 0,0")
    assert_equal(a.cplx_trans.to_s, "r90 *1 0,0")
    assert_equal(a.to_s, "#0 r90 0,0 [-20,10*3;-40,30*5]")

    at = a.transformed(RBA::ICplxTrans::new(1.5))
    assert_equal(a.is_complex?, false)
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r90 *1.5 0,0")
    assert_equal(at.to_s, "#0 r90 *1.5 0,0 [-30,15*3;-60,45*5]")

    a = RBA::CellInstArray::new(0, RBA::ICplxTrans::new(1.0, 45, false, RBA::Vector::new), RBA::Vector::new(10, 20), RBA::Vector::new(30, 40), 3, 5)
    assert_equal(a.is_complex?, true)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r45 *1 0,0")
    assert_equal(a.to_s, "#0 r45 *1 0,0 [10,20*3;30,40*5]")

    at = a.transformed(RBA::Trans::R90)
    atdup = at.dup
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r135 *1 0,0")
    assert_equal(at.to_s, "#0 r135 *1 0,0 [-20,10*3;-40,30*5]")

    assert_equal(at < a, false)
    assert_equal(at < atdup, false)
    assert_equal(a < at, true)
    assert_equal(atdup < at, false)
    assert_equal(a != at, true)
    assert_equal(a == at, false)
    assert_equal(atdup != at, false)
    assert_equal(atdup == at, true)

    at.transform(RBA::ICplxTrans::new(2.5))
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r135 *2.5 0,0")
    assert_equal(at.to_s, "#0 r135 *2.5 0,0 [-50,25*3;-100,75*5]")

    at.invert
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r180 0,0")
    assert_equal(at.cplx_trans.to_s, "r225 *0.4 0,0")
    assert_equal(at.to_s, "#0 r225 *0.4 0,0 [-21,-7*3;-49,-7*5]")

  end

  # DCellInstArray
  def test_0_DCellInstArray

    a = RBA::DCellInstArray::new
    assert_equal(a.to_s, "#0 r0 0,0")
    assert_equal(a.size, 1)

    a = RBA::DCellInstArray::new(0, RBA::DTrans::new(RBA::DTrans::R90))
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r90 0,0")
    assert_equal(a.cplx_trans.to_s, "r90 *1 0,0")

    a = RBA::DCellInstArray::new(0, RBA::DVector::new(42, -17))
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r0 42,-17")
    assert_equal(a.cplx_trans.to_s, "r0 *1 42,-17")

    a = RBA::DCellInstArray::new(0, RBA::DCplxTrans::new(1.5))
    assert_equal(a.is_complex?, true)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r0 *1.5 0,0")

    a = RBA::DCellInstArray::new(0, RBA::DCplxTrans::new(1.0))
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r0 *1 0,0")

    a.transform(RBA::DTrans::R90)
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r90 0,0")
    assert_equal(a.cplx_trans.to_s, "r90 *1 0,0")

    at = a.transformed(RBA::DCplxTrans::new(1.5))
    assert_equal(a.is_complex?, false)
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r90 *1.5 0,0")

    a = RBA::DCellInstArray::new(0, RBA::DCplxTrans::new(1.0, 45, false, RBA::DVector::new))
    assert_equal(a.is_complex?, true)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r45 *1 0,0")

    at = a.transformed(RBA::DTrans::R90)
    atdup = at.dup
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r135 *1 0,0")

    assert_equal(at < a, false)
    assert_equal(at < atdup, false)
    assert_equal(a < at, true)
    assert_equal(atdup < at, false)
    assert_equal(a != at, true)
    assert_equal(a == at, false)
    assert_equal(atdup != at, false)
    assert_equal(atdup == at, true)

    at.transform(RBA::DCplxTrans::new(2.5))
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r135 *2.5 0,0")

    at.invert
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r180 0,0")
    assert_equal(at.cplx_trans.to_s, "r225 *0.4 0,0")

    a = RBA::DCellInstArray::new(0, RBA::DTrans::new(RBA::DTrans::R90), RBA::DVector::new(10, 20), RBA::DVector::new(30, 40), 3, 5)
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r90 0,0")
    assert_equal(a.cplx_trans.to_s, "r90 *1 0,0")
    assert_equal(a.to_s, "#0 r90 0,0 [10,20*3;30,40*5]")

    a = RBA::DCellInstArray::new(0, RBA::DVector::new(42, -17), RBA::DVector::new(10, 20), RBA::DVector::new(30, 40), 3, 5)
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r0 42,-17")
    assert_equal(a.cplx_trans.to_s, "r0 *1 42,-17")
    assert_equal(a.to_s, "#0 r0 42,-17 [10,20*3;30,40*5]")

    a = RBA::DCellInstArray::new(0, RBA::DCplxTrans::new(1.5), RBA::DVector::new(10, 20), RBA::DVector::new(30, 40), 3, 5)
    assert_equal(a.is_complex?, true)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r0 *1.5 0,0")
    assert_equal(a.to_s, "#0 r0 *1.5 0,0 [10,20*3;30,40*5]")

    a = RBA::DCellInstArray::new(0, RBA::DCplxTrans::new(1.0), RBA::DVector::new(10, 20), RBA::DVector::new(30, 40), 3, 5)
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r0 *1 0,0")

    a.transform(RBA::DTrans::R90)
    assert_equal(a.is_complex?, false)
    assert_equal(a.trans.to_s, "r90 0,0")
    assert_equal(a.cplx_trans.to_s, "r90 *1 0,0")
    assert_equal(a.to_s, "#0 r90 0,0 [-20,10*3;-40,30*5]")

    at = a.transformed(RBA::DCplxTrans::new(1.5))
    assert_equal(a.is_complex?, false)
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r90 *1.5 0,0")
    assert_equal(at.to_s, "#0 r90 *1.5 0,0 [-30,15*3;-60,45*5]")

    a = RBA::DCellInstArray::new(0, RBA::DCplxTrans::new(1.0, 45, false, RBA::DVector::new), RBA::DVector::new(10, 20), RBA::Vector::new(30, 40), 3, 5)
    assert_equal(a.is_complex?, true)
    assert_equal(a.trans.to_s, "r0 0,0")
    assert_equal(a.cplx_trans.to_s, "r45 *1 0,0")
    assert_equal(a.to_s, "#0 r45 *1 0,0 [10,20*3;30,40*5]")

    at = a.transformed(RBA::DTrans::R90)
    atdup = at.dup
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r135 *1 0,0")
    assert_equal(at.to_s, "#0 r135 *1 0,0 [-20,10*3;-40,30*5]")

    assert_equal(at < a, false)
    assert_equal(at < atdup, false)
    assert_equal(a < at, true)
    assert_equal(atdup < at, false)
    assert_equal(a != at, true)
    assert_equal(a == at, false)
    assert_equal(atdup != at, false)
    assert_equal(atdup == at, true)

    at.transform(RBA::DCplxTrans::new(2.5))
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r90 0,0")
    assert_equal(at.cplx_trans.to_s, "r135 *2.5 0,0")
    assert_equal(at.to_s, "#0 r135 *2.5 0,0 [-50,25*3;-100,75*5]")

    at.invert
    assert_equal(at.is_complex?, true)
    assert_equal(at.trans.to_s, "r180 0,0")
    assert_equal(at.cplx_trans.to_s, "r225 *0.4 0,0")
    assert_equal(at.to_s, "#0 r225 *0.4 0,0 [-21.2132034356,-7.07106781187*3;-49.4974746831,-7.07106781187*5]")

  end

  # CellInstArray functions
  def test_1_CellInstArray

    i = RBA::CellInstArray.new
    assert_equal(i.to_s, "#0 r0 0,0")
    assert_equal(ci2str(i), "r0 0,0")
    assert_equal(cci2str(i), "r0 *1 0,0")

    i = RBA::CellInstArray.new(7, RBA::Trans.new(RBA::Point.new(1, 2)))
    assert_equal(i.to_s, "#7 r0 1,2")
    assert_equal(ci2str(i), "r0 1,2")
    assert_equal(cci2str(i), "r0 *1 1,2")
    assert_equal(i.cell_index.to_s, "7")
    i.cell_index = 8
    assert_equal(i.cell_index.to_s, "8")

    assert_equal(i.is_complex?, false)
    i.trans = RBA::Trans.new(3)
    assert_equal(i.is_complex?, false)
    assert_equal(i.to_s, "#8 r270 0,0")

    i.cplx_trans = RBA::CplxTrans.new(1.5)
    assert_equal(i.is_complex?, true)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0")
    assert_equal(ci2str(i), "r0 0,0")
    assert_equal(cci2str(i), "r0 *1.5 0,0")

    assert_equal(i.is_regular_array?, false)
    i.a = RBA::Point.new(10, 20)
    assert_equal(i.is_regular_array?, true)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0 [10,20*1;0,0*1]")
    assert_equal(i.a.to_s, "10,20")
    assert_equal(ci2str(i), "r0 0,0")
    assert_equal(cci2str(i), "r0 *1.5 0,0")

    i.na = 5
    assert_equal(i.is_regular_array?, true)
    assert_equal(i.na, 5)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0 [10,20*5;0,0*1]")

    i.b = RBA::Point.new(30, 40)
    assert_equal(i.is_regular_array?, true)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0 [10,20*5;30,40*1]")
    assert_equal(i.b.to_s, "30,40")

    i.nb = 3
    assert_equal(i.is_regular_array?, true)
    assert_equal(i.nb, 3)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0 [10,20*5;30,40*3]")
    assert_equal(ci2str(i), "r0 0,0;r0 10,20;r0 20,40;r0 30,60;r0 40,80;r0 30,40;r0 40,60;r0 50,80;r0 60,100;r0 70,120;r0 60,80;r0 70,100;r0 80,120;r0 90,140;r0 100,160")
    assert_equal(cci2str(i), "r0 *1.5 0,0;r0 *1.5 10,20;r0 *1.5 20,40;r0 *1.5 30,60;r0 *1.5 40,80;r0 *1.5 30,40;r0 *1.5 40,60;r0 *1.5 50,80;r0 *1.5 60,100;r0 *1.5 70,120;r0 *1.5 60,80;r0 *1.5 70,100;r0 *1.5 80,120;r0 *1.5 90,140;r0 *1.5 100,160")

    ii = i.dup 

    ii.trans = RBA::Trans.new(3)
    assert_equal(ii.is_complex?, false)
    assert_equal(ii.to_s, "#8 r270 0,0 [10,20*5;30,40*3]")

    i.nb = 0
    assert_equal(i.is_regular_array?, false)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0")

    assert_equal(ii.to_s, "#8 r270 0,0 [10,20*5;30,40*3]")
    ii.na = 1
    assert_equal(ii.to_s, "#8 r270 0,0 [10,20*1;30,40*3]")
    assert_equal(ci2str(ii), "r270 0,0;r270 30,40;r270 60,80")
    assert_equal(cci2str(ii), "r270 *1 0,0;r270 *1 30,40;r270 *1 60,80")
    ii.na = 0
    assert_equal(ii.to_s, "#8 r270 0,0")

    i = RBA::CellInstArray.new(0, RBA::Vector::new(1, 2), RBA::Vector::new(10, 20), RBA::Vector::new, 2, 2)
    assert_equal(i.to_s, "#0 r0 1,2 [10,20*2;0,0*1]")

    i = RBA::CellInstArray.new(0, RBA::Vector::new(1, 2), RBA::Vector::new(10, 20), RBA::Vector::new(11, 21), 2, 2)
    assert_equal(i.to_s, "#0 r0 1,2 [10,20*2;11,21*2]")

    i = RBA::CellInstArray.new(0, RBA::Vector::new(1, 2), RBA::Vector::new, RBA::Vector::new(11, 21), 2, 2)
    assert_equal(i.to_s, "#0 r0 1,2 [0,0*1;11,21*2]")

    i = RBA::CellInstArray.new(0, RBA::Vector::new(1, 2), RBA::Vector::new, RBA::Vector::new, 2, 2)
    assert_equal(i.to_s, "#0 r0 1,2")

    i = RBA::CellInstArray.new(0, RBA::Vector::new(1, 2), RBA::Vector::new(10, 20), RBA::Vector::new(11, 21), 0, 2)
    assert_equal(i.to_s, "#0 r0 1,2 [10,20*1;11,21*2]")

    i = RBA::CellInstArray.new(0, RBA::Vector::new(1, 2), RBA::Vector::new(10, 20), RBA::Vector::new(11, 21), 2, 0)
    assert_equal(i.to_s, "#0 r0 1,2 [10,20*2;11,21*1]")

    i = RBA::CellInstArray.new(0, RBA::Vector::new(1, 2), RBA::Vector::new(10, 20), RBA::Vector::new(11, 21), 0, 0)
    assert_equal(i.to_s, "#0 r0 1,2")

  end

  # DCellInstArray functions
  def test_1_DCellInstArray

    i = RBA::DCellInstArray.new
    assert_equal(i.to_s, "#0 r0 0,0")
    assert_equal(ci2str(i), "r0 0,0")
    assert_equal(cci2str(i), "r0 *1 0,0")

    i = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)))
    assert_equal(i.to_s, "#7 r0 1.5,2.5")
    assert_equal(ci2str(i), "r0 1.5,2.5")
    assert_equal(cci2str(i), "r0 *1 1.5,2.5")
    assert_equal(i.cell_index.to_s, "7")
    i.cell_index = 8
    assert_equal(i.cell_index.to_s, "8")

    assert_equal(i.is_complex?, false)
    i.trans = RBA::DTrans.new(3)
    assert_equal(i.is_complex?, false)
    assert_equal(i.to_s, "#8 r270 0,0")

    i.cplx_trans = RBA::DCplxTrans.new(1.5)
    assert_equal(i.is_complex?, true)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0")
    assert_equal(ci2str(i), "r0 0,0")
    assert_equal(cci2str(i), "r0 *1.5 0,0")

    assert_equal(i.is_regular_array?, false)
    i.a = RBA::DPoint.new(10.5, 20.5)
    assert_equal(i.is_regular_array?, true)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0 [10.5,20.5*1;0,0*1]")
    assert_equal(i.a.to_s, "10.5,20.5")
    assert_equal(ci2str(i), "r0 0,0")
    assert_equal(cci2str(i), "r0 *1.5 0,0")

    i.na = 5
    assert_equal(i.na, 5)
    assert_equal(i.is_regular_array?, true)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0 [10.5,20.5*5;0,0*1]")

    i.b = RBA::DPoint.new(30, 40)
    assert_equal(i.is_regular_array?, true)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0 [10.5,20.5*5;30,40*1]")
    assert_equal(i.b.to_s, "30,40")

    i.nb = 3
    assert_equal(i.nb, 3)
    assert_equal(i.is_regular_array?, true)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0 [10.5,20.5*5;30,40*3]")
    assert_equal(ci2str(i), "r0 0,0;r0 10.5,20.5;r0 21,41;r0 31.5,61.5;r0 42,82;r0 30,40;r0 40.5,60.5;r0 51,81;r0 61.5,101.5;r0 72,122;r0 60,80;r0 70.5,100.5;r0 81,121;r0 91.5,141.5;r0 102,162")
    assert_equal(cci2str(i), "r0 *1.5 0,0;r0 *1.5 10.5,20.5;r0 *1.5 21,41;r0 *1.5 31.5,61.5;r0 *1.5 42,82;r0 *1.5 30,40;r0 *1.5 40.5,60.5;r0 *1.5 51,81;r0 *1.5 61.5,101.5;r0 *1.5 72,122;r0 *1.5 60,80;r0 *1.5 70.5,100.5;r0 *1.5 81,121;r0 *1.5 91.5,141.5;r0 *1.5 102,162")

    ii = i.dup 

    ii.trans = RBA::DTrans.new(3)
    assert_equal(ii.is_complex?, false)
    assert_equal(ii.to_s, "#8 r270 0,0 [10.5,20.5*5;30,40*3]")

    i.nb = 0
    assert_equal(i.is_regular_array?, false)
    assert_equal(i.to_s, "#8 r0 *1.5 0,0")

    assert_equal(ii.to_s, "#8 r270 0,0 [10.5,20.5*5;30,40*3]")
    ii.na = 1
    assert_equal(ii.to_s, "#8 r270 0,0 [10.5,20.5*1;30,40*3]")
    assert_equal(ci2str(ii), "r270 0,0;r270 30,40;r270 60,80")
    assert_equal(cci2str(ii), "r270 *1 0,0;r270 *1 30,40;r270 *1 60,80")
    ii.na = 0
    assert_equal(ii.to_s, "#8 r270 0,0")

    i = RBA::DCellInstArray.new(0, RBA::DVector::new(1, 2), RBA::DVector::new(10, 20), RBA::DVector::new, 2, 2)
    assert_equal(i.to_s, "#0 r0 1,2 [10,20*2;0,0*1]")

    i = RBA::DCellInstArray.new(0, RBA::DVector::new(1, 2), RBA::DVector::new(10, 20), RBA::DVector::new(11, 21), 2, 2)
    assert_equal(i.to_s, "#0 r0 1,2 [10,20*2;11,21*2]")

    i = RBA::DCellInstArray.new(0, RBA::DVector::new(1, 2), RBA::DVector::new, RBA::DVector::new(11, 21), 2, 2)
    assert_equal(i.to_s, "#0 r0 1,2 [0,0*1;11,21*2]")

    i = RBA::DCellInstArray.new(0, RBA::DVector::new(1, 2), RBA::DVector::new, RBA::DVector::new, 2, 2)
    assert_equal(i.to_s, "#0 r0 1,2")

    i = RBA::DCellInstArray.new(0, RBA::DVector::new(1, 2), RBA::DVector::new(10, 20), RBA::DVector::new(11, 21), 0, 2)
    assert_equal(i.to_s, "#0 r0 1,2 [10,20*1;11,21*2]")

    i = RBA::DCellInstArray.new(0, RBA::DVector::new(1, 2), RBA::DVector::new(10, 20), RBA::DVector::new(11, 21), 2, 0)
    assert_equal(i.to_s, "#0 r0 1,2 [10,20*2;11,21*1]")

    i = RBA::DCellInstArray.new(0, RBA::DVector::new(1, 2), RBA::DVector::new(10, 20), RBA::DVector::new(11, 21), 0, 0)
    assert_equal(i.to_s, "#0 r0 1,2")

  end

  # DCellInstArray fuzzy compare
  def test_2_FuzzyCompare

    i1 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)))
    i2 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5 + 1e-7, 2.5)))
    i3 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5 + 1e-4, 2.5)))

    assert_equal(i1 == i2, true)
    assert_equal(i1 != i2, false)
    assert_equal(i1.eql?(i2), true)
    assert_equal(i1 < i2, false)
    assert_equal(i2 < i1, false)

    assert_equal(i1 == i3, false)
    assert_equal(i1 != i3, true)
    assert_equal(i1.eql?(i3), false)
    assert_equal(i1 < i3, true)
    assert_equal(i3 < i1, false)

  end

  # DCellInstArray hash function 
  def test_2_Hash

    i1 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)))
    i2 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5 + 1e-7, 2.5)))
    i3 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5 + 1e-4, 2.5)))
    i4 = RBA::DCellInstArray.new(3, RBA::DTrans.new(RBA::DPoint.new(1.5 + 1e-4, 2.5)))

    assert_equal(i1.hash == i2.hash, true)
    assert_equal(i1.hash == i3.hash, false)
    assert_equal(i1.hash == i4.hash, false)

    h = { i1 => "i1", i3 => "i3", i4 => "i4" }

    assert_equal(h[i1], "i1")
    assert_equal(h[i2], "i1")
    assert_equal(h[i3], "i3")
    assert_equal(h[i4], "i4")

  end

  # DCellInstArray hash function 
  def test_3_Hash

    i1 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1, 2), RBA::DPoint::new(3, 4), 5, 6)
    i2 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1 + 1e-7, 2), RBA::DPoint::new(3, 4), 5, 6)
    i3 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1 + 1e-4, 2), RBA::DPoint::new(3, 4), 5, 6)
    i4 = RBA::DCellInstArray.new(3, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1, 2), RBA::DPoint::new(3, 4), 7, 6)

    assert_equal(i1.hash == i2.hash, true)
    assert_equal(i1.hash == i3.hash, false)
    assert_equal(i1.hash == i4.hash, false)

    h = { i1 => "i1", i3 => "i3", i4 => "i4" }

    assert_equal(h[i1], "i1")
    assert_equal(h[i2], "i1")
    assert_equal(h[i3], "i3")
    assert_equal(h[i4], "i4")

  end

  # DCellInstArray fuzzy compare
  def test_3_FuzzyCompare

    i1 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1, 2), RBA::DPoint::new(3, 4), 5, 6)
    i2 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1 + 1e-7, 2), RBA::DPoint::new(3, 4), 5, 6)
    i3 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1 + 1e-4, 2), RBA::DPoint::new(3, 4), 5, 6)

    assert_equal(i1 == i2, true)
    assert_equal(i1 != i2, false)
    assert_equal(i1.eql?(i2), true)
    assert_equal(i1 < i2, false)
    assert_equal(i2 < i1, false)

    assert_equal(i1 == i3, false)
    assert_equal(i1 != i3, true)
    assert_equal(i1.eql?(i3), false)
    assert_equal(i1 < i3, true)
    assert_equal(i3 < i1, false)

  end

  # DCellInstArray hash function 
  def test_4_Hash

    i1 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1, 2), RBA::DPoint::new(3, 4), 5, 6)
    i2 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1, 2), RBA::DPoint::new(3 + 1e-7, 4), 5, 6)
    i3 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1, 2), RBA::DPoint::new(3 + 1e-4, 4), 5, 6)
    i4 = RBA::DCellInstArray.new(3, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1, 2), RBA::DPoint::new(3, 4), 5, 8)
    i5 = RBA::DCellInstArray.new(3, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)))

    assert_equal(i1.hash == i2.hash, true)
    assert_equal(i1.hash == i3.hash, false)
    assert_equal(i1.hash == i4.hash, false)

    h = { i1 => "i1", i3 => "i3", i4 => "i4", i5 => "i5" }

    assert_equal(h[i1], "i1")
    assert_equal(h[i2], "i1")
    assert_equal(h[i3], "i3")
    assert_equal(h[i4], "i4")
    assert_equal(h[i5], "i5")

  end

  # DCellInstArray fuzzy compare
  def test_4_FuzzyCompare

    i1 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1, 2), RBA::DPoint::new(3, 4), 5, 6)
    i2 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1, 2), RBA::DPoint::new(3 + 1e-7, 4), 5, 6)
    i3 = RBA::DCellInstArray.new(7, RBA::DTrans.new(RBA::DPoint.new(1.5, 2.5)), RBA::DPoint::new(1, 2), RBA::DPoint::new(3 + 1e-4, 4), 5, 6)

    assert_equal(i1 == i2, true)
    assert_equal(i1 != i2, false)
    assert_equal(i1.eql?(i2), true)
    assert_equal(i1 < i2, false)
    assert_equal(i2 < i1, false)

    assert_equal(i1 == i3, false)
    assert_equal(i1 != i3, true)
    assert_equal(i1.eql?(i3), false)
    assert_equal(i1 < i3, true)
    assert_equal(i3 < i1, false)

  end

  # Cell * variants
  def test_5_CellPointer

    ly = RBA::Layout::new
    cell = ly.create_cell("TOP")

    a = RBA::CellInstArray::new(cell, RBA::Trans::new(RBA::Trans::R90))
    assert_equal(a.cell_index, cell.cell_index)

    a = RBA::CellInstArray::new(cell, RBA::Vector::new(42, -17))
    assert_equal(a.cell_index, cell.cell_index)

    a = RBA::CellInstArray::new(cell, RBA::ICplxTrans::new(1.5))
    assert_equal(a.cell_index, cell.cell_index)

    a = RBA::CellInstArray::new(cell, RBA::ICplxTrans::new(1.0, 45, false, RBA::Vector::new))
    assert_equal(a.cell_index, cell.cell_index)

    a = RBA::CellInstArray::new(cell, RBA::Trans::new(RBA::Trans::R90), RBA::Vector::new(10, 20), RBA::Vector::new(30, 40), 3, 5)
    assert_equal(a.cell_index, cell.cell_index)

    a = RBA::CellInstArray::new(cell, RBA::Vector::new(42, -17), RBA::Vector::new(10, 20), RBA::Vector::new(30, 40), 3, 5)
    assert_equal(a.cell_index, cell.cell_index)

    a = RBA::CellInstArray::new(cell, RBA::ICplxTrans::new(1.5), RBA::Vector::new(10, 20), RBA::Vector::new(30, 40), 3, 5)
    assert_equal(a.cell_index, cell.cell_index)

  end

end

load("test_epilogue.rb")

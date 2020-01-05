# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2020 Matthias Koefferlein
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

class DBMatrix_TestClass < TestBase

  def test_1

    m = RBA::Matrix2d.new
    assert_equal("(1,0) (0,1)", m.to_s)
    m = RBA::Matrix2d.new(2.0)
    assert_equal("(2,0) (0,2)", m.to_s)
    m = RBA::Matrix2d.newc(2, 90, true)
    assert_equal("(0,2) (2,0)", m.to_s)
    m = RBA::Matrix2d.new(RBA::DCplxTrans.new(2, 90, true, RBA::DPoint.new(0, 0)))
    assert_equal("(0,2) (2,0)", m.to_s)
    m = RBA::Matrix2d.newc(0.0, 2, 2, 90, true)
    assert_equal("(0,2) (2,0)", m.to_s)
    m = RBA::Matrix2d.newc(17.0, 2, 3, 90, true)
    assert_equal("17.000000", "%.6f" % m.shear_angle)
    assert_equal("90.0", m.angle.to_s)
    assert_equal("2.000000", "%.6f" % m.mag_x)
    assert_equal("3.000000", "%.6f" % m.mag_y)
    assert_equal(true, m.is_mirror?)
    m = RBA::Matrix2d.new(1, 2, 3, 4)
    assert_equal("(1,2) (3,4)", m.to_s)
    assert_equal("1.0", m.m11.to_s)
    assert_equal("2.0", m.m12.to_s)
    assert_equal("3.0", m.m21.to_s)
    assert_equal("4.0", m.m22.to_s)
    assert_equal("4.0", m.m(1,1).to_s)
    assert_equal("2.0", m.m(0,1).to_s)
    m = RBA::Matrix2d.newc(2, 90, true)
    assert_equal(true, m.is_mirror?)
    t = m.cplx_trans
    assert_equal("m45 *2 0,0", t.to_s)
    m = RBA::Matrix2d.newc(2, 90, false)
    assert_equal("90.0", m.angle.to_s)
    assert_equal(false, m.is_mirror?)
    t = m.inverted.cplx_trans
    assert_equal("r270 *0.5 0,0", t.to_s)
    p = m.trans(RBA::DPoint.new(1, 2))
    assert_equal("-4,2", p.to_s)
    assert_equal("(1,0) (0,1)", (m.inverted*m).to_s)
    assert_equal("(0,-1.5) (1.5,0)", (m+m.inverted).to_s)

  end

  def test_2

    m = RBA::Matrix3d.new
    assert_equal("(1,0,0) (0,1,0) (0,0,1)", m.to_s)
    m = RBA::Matrix3d.new(2.0)
    assert_equal("(2,0,0) (0,2,0) (0,0,1)", m.to_s)
    m = RBA::Matrix3d.newc(2, 90, true)
    assert_equal("(0,2,0) (2,0,0) (0,0,1)", m.to_s)
    m = RBA::Matrix3d.new(RBA::DCplxTrans.new(2, 90, true, RBA::DPoint.new(1, 2)))
    assert_equal("(0,2,1) (2,0,2) (0,0,1)", m.to_s)
    m = RBA::Matrix3d.newc(0.0, 2, 3, 90, true)
    assert_equal("(0,3,0) (2,0,0) (0,0,1)", m.to_s)
    m = RBA::Matrix3d.newc(17.0, 2, 3, 90, true)
    assert_equal("17.000000", "%.6f" % m.shear_angle)
    assert_equal("90.0", m.angle.to_s)
    assert_equal("2.000000", "%.6f" % m.mag_x)
    assert_equal("3.000000", "%.6f" % m.mag_y)
    assert_equal(true, m.is_mirror?)
    m = RBA::Matrix3d.newc(RBA::DPoint.new(1, 2), 17.0, 2, 3, 90, true)
    assert_equal("17.000000", "%.6f" % m.shear_angle)
    assert_equal("90.0", m.angle.to_s)
    assert_equal("2.000000", "%.6f" % m.mag_x)
    assert_equal("3.000000", "%.6f" % m.mag_y)
    assert_equal("1,2", m.disp.to_s)
    assert_equal(true, m.is_mirror?)
    m = RBA::Matrix3d.new(1, 2, 3, 4)
    assert_equal("(1,2,0) (3,4,0) (0,0,1)", m.to_s)
    assert_equal("4.0", m.m(1,1).to_s)
    assert_equal("2.0", m.m(0,1).to_s)
    assert_equal("0.0", m.m(0,2).to_s)
    assert_equal("1.0", m.m(2,2).to_s)
    m = RBA::Matrix3d.newc(2, 90, true)
    assert_equal(true, m.is_mirror?)
    t = m.cplx_trans
    assert_equal("m45 *2 0,0", t.to_s)
    m = RBA::Matrix3d.newc(2, 90, false)
    assert_equal("90.0", m.angle.to_s)
    assert_equal(false, m.is_mirror?)
    t = m.inverted.cplx_trans
    assert_equal("r270 *0.5 0,0", t.to_s)
    p = m.trans(RBA::DPoint.new(1, 2))
    assert_equal("-4,2", p.to_s)
    assert_equal("(1,0,0) (0,1,0) (0,0,1)", (m.inverted*m).to_s)
    assert_equal("(0,-1.5,0) (1.5,0,0) (0,0,2)", (m+m.inverted).to_s)
    m = RBA::Matrix3d.new(0, 1, -1, 0, 1, 2)
    t = m.cplx_trans
    assert_equal("r270 *1 1,2", t.to_s)
    m = RBA::Matrix3d.new(0, 1, 1, -1, 0, 2, 0, 0, 1)
    t = m.cplx_trans
    assert_equal("r270 *1 1,2", t.to_s)
    assert_equal("1.0", m.disp.x.to_s)
    assert_equal("2.0", m.disp.y.to_s)
    m = RBA::Matrix3d.newc(0.1, -0.2, 1.0, RBA::DPoint.new(1, 2), 17.0, 2, 2, 270, true)
    assert_equal("0.100000", "%.6f" % m.tx(1.0))
    assert_equal("-0.200000", "%.6f" % m.ty(1.0))
    assert_equal("17.000000", "%.6f" % m.shear_angle)
    assert_equal("1,2", m.disp.to_s)

  end

  def test_3

    p = [ RBA::DPoint.new(1, 1), RBA::DPoint.new(2, 1), RBA::DPoint.new(2, 2) ]
    q = [ RBA::DPoint.new(1, 1), RBA::DPoint.new(2, 1), RBA::DPoint.new(2, 3) ]
    m = RBA::Matrix3d.new(1.0)
    assert_equal((m * p[0]).to_s, "1,1");
    assert_equal((m * p[1]).to_s, "2,1");
    assert_equal((m * p[2]).to_s, "2,2");
    m.adjust(p, q, RBA::Matrix3d::AdjustAll, -1)
    assert_equal((m * p[0]).to_s, "1,1");
    assert_equal((m * p[1]).to_s, "2,1");
    assert_equal((m * p[2]).to_s, "2,3");

  end

end

load("test_epilogue.rb")

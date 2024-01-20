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


import pya
import unittest

# NOTE: pya.CplxTrans and pya.Trans and good test cases
# for the keyword arguments feature

class KWArgsTest(unittest.TestCase):

  def test_1(self):

    t = pya.CplxTrans()
    self.assertEqual(str(t), "r0 *1 0,0")

    t = pya.CplxTrans(1.5)
    self.assertEqual(str(t), "r0 *1.5 0,0")

    t = pya.CplxTrans(1, 2)
    self.assertEqual(str(t), "r0 *1 1,2")

    t = pya.CplxTrans(1, y = 2)
    self.assertEqual(str(t), "r0 *1 1,2")

    t = pya.CplxTrans(x = 1, y = 2)
    self.assertEqual(str(t), "r0 *1 1,2")

    t = pya.CplxTrans(u = pya.DVector(1, 2))
    self.assertEqual(str(t), "r0 *1 1,2")

    t = pya.CplxTrans(pya.DVector(1, 2))
    self.assertEqual(str(t), "r0 *1 1,2")

    t = pya.CplxTrans(u = pya.Vector(1, 2))
    self.assertEqual(str(t), "r0 *1 1,2")

    t = pya.CplxTrans(u = (1, 2))
    self.assertEqual(str(t), "r0 *1 1,2")

    t = pya.CplxTrans(mag = 1.5)
    self.assertEqual(str(t), "r0 *1.5 0,0")

    t = pya.CplxTrans(1.5, 45, True, 1, 2)
    self.assertEqual(str(t), "m22.5 *1.5 1,2")

    t = pya.CplxTrans(1.5, 45, True, pya.DVector(1, 2))
    self.assertEqual(str(t), "m22.5 *1.5 1,2")

    t = pya.CplxTrans(1.5, x = 1, y = 2, mirrx = True, rot = 45)
    self.assertEqual(str(t), "m22.5 *1.5 1,2")

    t = pya.CplxTrans(pya.CplxTrans.M0)
    self.assertEqual(str(t), "m0 *1 0,0")

    t = pya.CplxTrans(pya.CplxTrans.M0, u = pya.DVector(1, 2))
    self.assertEqual(str(t), "m0 *1 1,2")

    t = pya.CplxTrans(pya.CplxTrans.M0, mag = 1.5, u = pya.DVector(1, 2))
    self.assertEqual(str(t), "m0 *1.5 1,2")

    t = pya.CplxTrans(pya.CplxTrans.M0, 1.5, pya.DVector(1, 2))
    self.assertEqual(str(t), "m0 *1.5 1,2")

    t = pya.CplxTrans(pya.CplxTrans.M0, mag = 1.5, x = 1, y = 2)
    self.assertEqual(str(t), "m0 *1.5 1,2")

    t = pya.CplxTrans(pya.CplxTrans.M0, 1.5, 1, 2)
    self.assertEqual(str(t), "m0 *1.5 1,2")

    t = pya.CplxTrans(pya.VCplxTrans.M0)
    self.assertEqual(str(t), "m0 *1 0,0")

    t = pya.CplxTrans(pya.ICplxTrans.M0)
    self.assertEqual(str(t), "m0 *1 0,0")

    t = pya.CplxTrans(pya.DCplxTrans.M0)
    self.assertEqual(str(t), "m0 *1 0,0")

    t = pya.CplxTrans(pya.Trans.M0)
    self.assertEqual(str(t), "m0 *1 0,0")

    t = pya.CplxTrans(pya.Trans.M0, 1.5)
    self.assertEqual(str(t), "m0 *1.5 0,0")

    t = pya.CplxTrans(pya.Trans.M0, mag = 1.5)
    self.assertEqual(str(t), "m0 *1.5 0,0")
    
    t = pya.CplxTrans(t = pya.Trans.M0, mag = 1.5)
    self.assertEqual(str(t), "m0 *1.5 0,0")

    t = pya.CplxTrans()
    t.disp = (1, 2)
    self.assertEqual(str(t), "r0 *1 1,2")

    t = pya.ICplxTrans(15, 25)
    self.assertEqual(t.to_s(dbu = 0.01), "r0 *1 0.15000,0.25000")
    

  def test_2(self):

    t = pya.Trans(pya.Trans.M0, 1, 2)
    self.assertEqual(str(t), "m0 1,2")

    t = pya.Trans(pya.Trans.M0, x = 1, y = 2)
    self.assertEqual(str(t), "m0 1,2")

    t = pya.Trans(pya.Trans.M0, pya.Vector(1, 2))
    self.assertEqual(str(t), "m0 1,2")

    t = pya.Trans(pya.Trans.M0, u = pya.Vector(1, 2))
    self.assertEqual(str(t), "m0 1,2")

    t = pya.Trans(rot = 3, mirrx = True)
    self.assertEqual(str(t), "m135 0,0")

    t = pya.Trans(rot = 3, mirrx = True, x = 1, y = 2)
    self.assertEqual(str(t), "m135 1,2")

    t = pya.Trans(3, True, 1, 2)
    self.assertEqual(str(t), "m135 1,2")

    t = pya.Trans(3, True, pya.Vector(1, 2))
    self.assertEqual(str(t), "m135 1,2")

    t = pya.Trans(rot = 3, mirrx = True, u = pya.Vector(1, 2))
    self.assertEqual(str(t), "m135 1,2")

    t = pya.Trans()
    self.assertEqual(str(t), "r0 0,0")

    t = pya.Trans(pya.DTrans.M0)
    self.assertEqual(str(t), "m0 0,0")

    t = pya.Trans(pya.DTrans.M0, 1, 2)
    self.assertEqual(str(t), "m0 1,2")

    t = pya.Trans(pya.DTrans.M0, x = 1, y = 2)
    self.assertEqual(str(t), "m0 1,2")

    t = pya.Trans(c = pya.DTrans.M0, x = 1, y = 2)
    self.assertEqual(str(t), "m0 1,2")

    t = pya.Trans(pya.Vector(1, 2))
    self.assertEqual(str(t), "r0 1,2")

    t = pya.Trans(1, 2)
    self.assertEqual(str(t), "r0 1,2")


  def test_3(self):

    try:
      t = pya.CplxTrans(1.5, 2.5)
      t.to_s(dbu = "17")
      self.assertEqual(True, False)
    except Exception as ex:
      self.assertEqual(str(ex), "Value cannot be converted to a floating-point value for argument #2 ('dbu') in CplxTrans.to_s")

    try:
      t = pya.CplxTrans(1.5, 2.5)
      tt = pya.CplxTrans()
      tt.assign(other = t)
      self.assertEqual(True, False)
    except Exception as ex:
      self.assertEqual(str(ex), "Keyword arguments not permitted in CplxTrans.assign")

    try:
      t = pya.CplxTrans("17")
      self.assertEqual(True, False)
    except Exception as ex:
      self.assertEqual(str(ex).find("No overload with matching arguments."), 0)

    try:
      t = pya.CplxTrans(uu = 17)
      self.assertEqual(True, False)
    except Exception as ex:
      self.assertEqual(str(ex).find("Can't match arguments."), 0)

    try:
      t = pya.CplxTrans(u = "17")
      self.assertEqual(True, False)
    except Exception as ex:
      self.assertEqual(str(ex).find("No overload with matching arguments."), 0)


# run unit tests
if __name__ == '__main__':
  suite = unittest.TestSuite()
  # NOTE: Use this instead of loadTestsfromTestCase to select a specific test:
  #   suite.addTest(KWArgsTest("test_26"))
  suite = unittest.TestLoader().loadTestsFromTestCase(KWArgsTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)


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
import sys
import os

class DBRegionTest(unittest.TestCase):

  def test_1_Region(self):

    r = pya.Region()
    self.assertEqual(str(r), "")

    r.insert(pya.Box(0, 100, 200, 300))
    self.assertEqual(str(r), "(0,100;0,300;200,300;200,100)")

    r2 = pya.Region(pya.Box(50, 150, 250, 350))
    self.assertEqual(str(r2), "(50,150;50,350;250,350;250,150)")

    r += r2
    self.assertEqual(str(r), "(0,100;0,300;200,300;200,100);(50,150;50,350;250,350;250,150)")

    r.merge()
    self.assertEqual(str(r), "(0,100;0,300;50,300;50,350;250,350;250,150;200,150;200,100)")

  def test_deep1(self):

    ut_testsrc = os.getenv("TESTSRC")

    # construction/destruction magic ...
    self.assertEqual(pya.DeepShapeStore.instance_count(), 0)
    dss = pya.DeepShapeStore()
    dss._create()
    self.assertEqual(pya.DeepShapeStore.instance_count(), 1)
    dss = None
    self.assertEqual(pya.DeepShapeStore.instance_count(), 0)

    dss = pya.DeepShapeStore()
    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "algo", "deep_region_l1.gds"))
    l1 = ly.layer(1, 0)
    r = pya.Region(ly.top_cell().begin_shapes_rec(l1), dss)
    rf = pya.Region(ly.top_cell().begin_shapes_rec(l1))

    self.assertEqual(r.area(), 53120000)
    self.assertEqual(rf.area(), 53120000)

    ly_new = pya.Layout()
    tc = ly_new.add_cell("TOP")
    l1 = ly_new.layer(1, 0)
    l2 = ly_new.layer(2, 0)
    ly_new.insert(tc, l1, r)
    ly_new.insert(tc, l2, rf)

    s1 = { }
    s2 = { }
    for cell in ly_new.each_cell():
      s1[cell.name] = cell.shapes(l1).size()
      s2[cell.name] = cell.shapes(l2).size()
    self.assertEqual(s1, {"INV2": 1, "TOP": 0, "TRANS": 0})
    self.assertEqual(s2, {"INV2": 0, "TOP": 10, "TRANS": 0})

    # force destroy, so the unit tests pass on the next iteration
    dss = None
    self.assertEqual(pya.DeepShapeStore.instance_count(), 0)

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBRegionTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)


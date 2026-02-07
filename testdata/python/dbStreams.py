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

import pya
import unittest
import sys
import os

class DBStreamsTests(unittest.TestCase):

  # Full-spin read-write tests
  # This test is rather a smoke test
  def rw_test(self, format):

    ly = pya.Layout()
    cell = ly.create_cell("TOP")
    l1 = ly.layer(1, 0)
    cell.shapes(l1).insert(pya.Box(0, 0, 1000, 2000))
    cell.shapes(l1).insert(pya.Box(-100, -200, 2000, 1000))
    bbox = cell.bbox()

    opt = pya.SaveLayoutOptions()
    opt.format = format

    b = ly.write_bytes(opt)
    
    ly2 = pya.Layout()
    ly2.read_bytes(b)

    tc = ly.top_cell()
    self.assertEqual(tc.name, "TOP")
    self.assertEqual(str(tc.bbox()), str(bbox))

  def test_gds2(self):
    self.rw_test("GDS2")
  
  def test_oasis(self):
    self.rw_test("OASIS")
  
  def test_dxf(self):
    self.rw_test("DXF")
  
  def test_cif(self):
    self.rw_test("CIF")
  
  def test_lstream(self):
    self.rw_test("LStream")
  
  def test_magic(self):
    # Smoke test
    ut_testsrc = os.getenv("TESTSRC")
    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "magic", "ringo", "RINGO.mag"))
  
  def test_maly(self):
    # Smoke test
    ut_testsrc = os.getenv("TESTSRC")
    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "maly", "MALY_test10.maly"))
  
  def test_pcb(self):
    # Smoke test
    ut_testsrc = os.getenv("TESTSRC")
    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "pcb", "simple", "import.pcb"))
  
  def test_lefdef(self):
    # Smoke test
    ut_testsrc = os.getenv("TESTSRC")
    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "lefdef", "scanchain", "test.def"))
  
# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBStreamsTests)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)



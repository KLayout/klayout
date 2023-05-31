# KLayout Layout Viewer
# Copyright (C) 2006-2023 Matthias Koefferlein
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

import testprep
import klayout.db as db
import unittest
import sys
import os

# Tests the basic abilities of the module

class BasicTest(unittest.TestCase):

  def test_1(self):
    self.assertEqual("Box" in db.__all__, True)

  def test_2(self):
    # Some smoke test
    v = db.Box()
    self.assertEqual(str(v), "()")
    v = db.Box(1, 2, 3, 4)
    self.assertEqual(str(v), "(1,2;3,4)")

  def test_3(self):
    # db plugins loaded?
    v = db.Layout()
    v.read(os.path.join(os.path.dirname(__file__), "..", "gds", "t10.gds"))
    self.assertEqual(v.top_cell().name, "RINGO")

  def test_4(self):
    # gds2_text plugin loaded? (issue #1393)
    v = db.Layout()
    v.read(os.path.join(os.path.dirname(__file__), "..", "gds2_txt", "read.txt"))
    self.assertEqual(v.top_cell().name, "RINGO")

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestSuite()
  suite = unittest.TestLoader().loadTestsFromTestCase(BasicTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

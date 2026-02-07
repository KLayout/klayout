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

import testprep
import klayout.db as db
import bridge_mod as bridge
import unittest
import sys

# Tests the basic abilities of the module

class BridgeTest(unittest.TestCase):

  def test_1(self):
    p = db.DSimplePolygon(db.DBox(1,2,3,4))
    a = bridge.p2a(p)
    self.assertEqual(repr(bridge.p2a(p)), "[(1.0, 2.0), (1.0, 4.0), (3.0, 4.0), (3.0, 2.0)]")

  def test_2(self):
    p = db.DSimplePolygon(db.DBox(1,2,3,4))
    a = bridge.p2a(p)
    pp = bridge.a2p(a)
    self.assertEqual(str(pp), "(1,2;1,4;3,4;3,2)")
    self.assertEqual(type(pp).__name__, "DSimplePolygon")

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestSuite()
  suite = unittest.TestLoader().loadTestsFromTestCase(BridgeTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)



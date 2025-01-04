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


import pya
import unittest
import sys
import os

class DBShapesTest(unittest.TestCase):

  # Shape objects as hashes
  def test_12(self):

    s = pya.Shapes()
    s1 = s.insert(pya.Box(1, 2, 3, 4))
    s2 = s.insert(pya.Polygon(pya.Box(1, 2, 3, 4)))
    s3 = s.insert(pya.SimplePolygon(pya.Box(1, 2, 3, 4)))

    self.assertEqual(s1.hash != s2.hash, True)   # let's hope so ...
    self.assertEqual(s1.hash != s3.hash, True)
    self.assertEqual(s2.hash != s3.hash, True)

    self.assertEqual(s1 < s2 or s2 < s1, True)
    self.assertEqual(s1 < s3 or s3 < s1, True)
    self.assertEqual(s2 < s3 or s3 < s2, True)

    h = {}
    h[s1] = 1
    h[s2] = 2
    h[s3] = 3
    
    self.assertEqual(len(h), 3)
    
    self.assertEqual(h[s1], 1)
    self.assertEqual(h[s2], 2)
    self.assertEqual(h[s3], 3)

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBShapesTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)


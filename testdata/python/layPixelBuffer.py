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
import os
import sys

def compare(pb1, pb2):

  if pb1.width() != pb2.width() or pb1.height() != pb2.height():
    return False

  for x in range(0, pb1.width()):
    for y in range(0, pb1.height()):
      if pb1.pixel(x, y) != pb2.pixel(x, y):
        return False

  return True


class LAYPixelBufferTests(unittest.TestCase):

  def test_1(self):

    pb = pya.PixelBuffer(10, 20)
    pb.transparent = True
    pb.fill(0xf0010203)

    png = pb.to_png_data()

    # some range because implementations may differ
    self.assertGreater(len(png), 20)
    self.assertLess(len(png), 200)
    pb_copy = pya.PixelBuffer.from_png_data(png)
    self.assertEqual(compare(pb, pb_copy), True)

    ut_testtmp = os.getenv("TESTTMP", ".")
    tmp = os.path.join(ut_testtmp, "tmp.png")

    pb.write_png(tmp)
    pb_copy = pya.PixelBuffer.read_png(tmp)
    self.assertEqual(compare(pb, pb_copy), True)


# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(LAYPixelBufferTests)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

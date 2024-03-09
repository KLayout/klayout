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

class LAYObjectsTests(unittest.TestCase):

  def test_1(self):

    class MyBrowserSource(pya.BrowserSource):
      def get(self, url):
        next_url = "int:" + str(int(url.split(":")[1]) + 1)
        return "This is " + url + ". <a href='" + next_url + "'>Goto next (" + next_url + ")</a>"
      
    dialog = pya.BrowserDialog()
    dialog.home = "int:0"
    dialog.source = MyBrowserSource()

    dialog = pya.BrowserDialog()
    dialog.home = "int:0"
    dialog.source = MyBrowserSource()

    self.assertEqual(True, True)


# run unit tests
if __name__ == '__main__':
  suite = unittest.TestSuite()
  # NOTE: Use this instead of loadTestsfromTestCase to select a specific test:
  #   suite.addTest(BasicTest("test_26"))
  suite = unittest.TestLoader().loadTestsFromTestCase(LAYObjectsTests)

  # Only runs with Application available
  if "Application" in pya.__all__ and not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)




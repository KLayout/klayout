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


import unittest

class ModuleTest(unittest.TestCase):

  def test_1(self):

    import klayout as kl
    self.assertEqual(kl.__version__ != "", True)

    import klayout.db as kldb
    self.assertEqual(kldb.__version__, kl.__version__)

    import klayout.tl as kltl
    self.assertEqual(kltl.__version__, kl.__version__)

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestSuite()
  # NOTE: Use this instead of loadTestsfromTestCase to select a specific test:
  #   suite.addTest(BasicTest("test_26"))
  suite = unittest.TestLoader().loadTestsFromTestCase(BasicTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)


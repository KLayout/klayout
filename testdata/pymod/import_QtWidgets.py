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

import testprep
import klayout.QtCore as QtCore
import klayout.QtGui as QtGui
import klayout.QtWidgets as QtWidgets
import unittest
import sys

# Tests the basic abilities of the module

class BasicTest(unittest.TestCase):

  trg = 0

  def test_1(self):
    self.assertEqual("QAction" in QtWidgets.__all__, True)

  def onTrigger(self):
    self.trg += 17

  def test_2(self):
    a = QtWidgets.QAction(None)
    a.text = "myaction"
    self.assertEqual(a.text, "myaction")
    self.trg = 0
    a.triggered = self.onTrigger
    a.trigger()
    self.assertEqual(self.trg, 17)
    a.trigger()
    self.assertEqual(self.trg, 34)

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestSuite()
  suite = unittest.TestLoader().loadTestsFromTestCase(BasicTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)



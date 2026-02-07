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
import klayout.lay as lay
import unittest
import sys

def can_create_layoutview():
  if not "MainWindow" in lay.__dict__:
    return True  # Qt-less
  elif not "Application" in lay.__dict__:
    return False  # cannot instantiate Application
  elif lay.__dict__["Application"].instance() is None:
    return False  # Application is not present
  else:
    return True

# Tests the basic abilities of the module

class BasicTest(unittest.TestCase):

  def test_1(self):

    if not can_create_layoutview():
      print("Skipped test as LayoutView cannot be instantiated")
      return

    lv = lay.LayoutView()
    lv.resize(800, 600)
    lv.zoom_box(db.DBox(-42, -17, 142, 117))
    bx = lv.box()
    self.assertEqual(str(bx), "(-42.09,-19.09;141.91,118.91)")

  def test_2(self):

    p = lay.LayerPropertiesNode()
    p.name = "u"
    self.assertEqual(p.name, "u")

  def test_3(self):

    # smoke test (issue #2154)
    x = lay.Cursor.Arrow
    x = lay.ButtonState.ShiftKey
    x = lay.KeyCode.Escape

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestSuite()
  suite = unittest.TestLoader().loadTestsFromTestCase(BasicTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)



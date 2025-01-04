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

import testprep
import klayout.QtCore as QtCore
import klayout.QtXml as QtXml
import unittest
import sys

# Tests the basic abilities of the module

class BasicTest(unittest.TestCase):

  def test_1(self):
    self.assertEqual("QDomDocument" in QtXml.__all__, True)

  def test_2(self):
    # Some smoke test
    b = QtCore.QBuffer()
    b.setData("<doc><e>abc</e></doc>")
    b.open(QtCore.QIODevice.ReadOnly)
    doc = QtXml.QDomDocument()
    doc.setContent(b)
    b.close()
    self.assertEqual(doc.documentElement().tagName, "doc")
    self.assertEqual(doc.documentElement().firstChild().toElement().tagName, "e")

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestSuite()
  suite = unittest.TestLoader().loadTestsFromTestCase(BasicTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)



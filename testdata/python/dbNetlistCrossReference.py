
# encoding: UTF-8

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

class DBNetlistCrossReferenceTests(unittest.TestCase):

  def test_1_Basic(self):

    ut_testsrc = os.getenv("TESTSRC")

    xref = pya.NetlistCrossReference()

    lvs = pya.LayoutVsSchematic()
    infile = os.path.join(ut_testsrc, "testdata", "algo", "lvsdb_read_test.lvsdb")
    lvs.read(infile)

    reader = pya.NetlistSpiceReader()
    nl = pya.Netlist()
    nl.read(os.path.join(ut_testsrc, "testdata", "algo", "lvs_test_1.spi"), reader)

    self.assertEqual(xref.circuit_count(), 0)

    # A NetlistCrossReference object can act as a receiver for a netlist comparer
    comp = pya.NetlistComparer()
    comp.compare(lvs.netlist(), nl, xref)

    self.assertEqual(xref.circuit_count(), 3)

    xref.clear()
    self.assertEqual(xref.circuit_count(), 0)

  def name_or_nil(self, s):
    if s:
      return s.name
    else:
      return "(nil)"

  def terminal_def_name_or_nil(self, s):
    if s:
      return s.terminal_def().name
    else:
      return "(nil)"

  def pin_name_or_nil(self, s):
    if s:
      return s.pin().name()
    else:
      return "(nil)"

  def name_call_or_nil(self, s):
    if s:
      return s.name()
    else:
      return "(nil)"

  def test_2_CircuitPairs(self):

    ut_testsrc = os.getenv("TESTSRC")

    lvs = pya.LayoutVsSchematic()
    input = os.path.join(ut_testsrc, "testdata", "algo", "lvsdb_read_test2.lvsdb")
    lvs.read(input)

    xref = lvs.xref()
    self.assertEqual(xref.circuit_count(), 4)

    info = []
    for cp in xref.each_circuit_pair():
      info.append("/".join([ self.name_or_nil(s) for s in [ cp.first(), cp.second() ] ]) + ":" + str(cp.status()))
    self.assertEqual(",".join(info), "(nil)/INV2PAIRX:Mismatch,INV2/INV2:Match,INV2PAIR/INV2PAIR:NoMatch,RINGO/RINGO:Skipped")

    cp_inv2 = None
    for cp in xref.each_circuit_pair():
      if cp.first() and cp.first().name == "INV2":
        cp_inv2 = cp

    self.assertEqual(cp_inv2 != None, True)

    info = []
    for p in xref.each_pin_pair(cp_inv2):
      info.append("/".join([ self.name_call_or_nil(s) for s in [ p.first(), p.second() ] ]) + ":" + str(p.status()))
    self.assertEqual(",".join(info), "/1:Match,BULK/6:Match,IN/2:Match,OUT/3:Match,VDD/5:Match,VSS/4:Match")
      
    info = []
    for p in xref.each_net_pair(cp_inv2):
      info.append("/".join([ self.name_or_nil(s) for s in [ p.first(), p.second() ] ]) + ":" + str(p.status()))
    self.assertEqual(",".join(info), "/1:Match,BULK/6:Match,IN/2:Match,OUT/3:Match,VDD/5:Match,VSS/4:Match")

    netp_bulk = None
    for p in xref.each_net_pair(cp_inv2):
      if p.first().name == "BULK":
        netp_bulk = p
      
    info = []
    for p in xref.each_net_terminal_pair(netp_bulk):
      info.append("/".join([ self.terminal_def_name_or_nil(s) for s in [ p.first(), p.second() ] ]))
    self.assertEqual(",".join(info), "B/B")

    info = []
    for p in xref.each_net_pin_pair(netp_bulk):
      info.append("/".join([ self.pin_name_or_nil(s) for s in [ p.first(), p.second() ] ]))
    self.assertEqual(",".join(info), "BULK/6")

    info = []
    for p in xref.each_net_subcircuit_pin_pair(netp_bulk):
      info.append("/".join([ self.name_or_nil(s) for s in [ p.first(), p.second() ] ]))
    self.assertEqual(",".join(info), "")

    info = []
    for p in xref.each_device_pair(cp_inv2):
      info.append("/".join([ self.name_or_nil(s) for s in [ p.first(), p.second() ] ]) + ":" + str(p.status()))
    self.assertEqual(",".join(info), "/$1:Match,/$3:Match")
      
    info = []
    for p in xref.each_subcircuit_pair(cp_inv2):
      info.append("/".join([ self.name_or_nil(s) for s in [ p.first(), p.second() ] ]) + ":" + str(p.status()))
    self.assertEqual(",".join(info), "")
      
    cp_inv2pair = None
    for cp in xref.each_circuit_pair():
      if cp.first() and cp.first().name == "INV2PAIR":
        cp_inv2pair = cp

    self.assertEqual(cp_inv2pair != None, True)

    info = []
    for p in xref.each_pin_pair(cp_inv2pair):
      info.append("/".join([ self.name_call_or_nil(s) for s in [ p.first(), p.second() ] ]) + ":" + str(p.status()))
    self.assertEqual(",".join(info), "(nil)/5:Mismatch,/(nil):Mismatch,/2:Match,/3:Match,/4:Match,/6:Match,/7:Match,BULK/1:Match")
      
    info = []
    for p in xref.each_net_pair(cp_inv2pair):
      info.append("/".join([ self.name_or_nil(s) for s in [ p.first(), p.second() ] ]) + ":" + str(p.status()))
    self.assertEqual(",".join(info), "/(nil):Mismatch,/2:Mismatch,/3:Mismatch,/4:Match,/6:Match,/7:Mismatch,BULK/1:Mismatch")
      
    info = []
    for p in xref.each_device_pair(cp_inv2pair):
      info.append("/".join([ self.name_or_nil(s) for s in [ p.first(), p.second() ] ]) + ":" + str(p.status()))
    self.assertEqual(",".join(info), "")
      
    info = []
    for p in xref.each_subcircuit_pair(cp_inv2pair):
      info.append("/".join([ self.name_or_nil(s) for s in [ p.first(), p.second() ] ]) + ":" + str(p.status()))
    self.assertEqual(",".join(info), "(nil)/$2:Mismatch,/(nil):Mismatch,/(nil):Mismatch")

  def test_3_StatusEnums(self):

    st = pya.NetlistCrossReference.Status()
    self.assertEqual(st.to_i(), 0)
    self.assertEqual(str(st), "None")
  
    st = pya.NetlistCrossReference.Status.Match
    self.assertEqual(str(st), "Match")

    st = pya.NetlistCrossReference.Skipped
    self.assertEqual(str(st), "Skipped")


# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBNetlistCrossReferenceTests)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)



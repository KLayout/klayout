
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

class DBLayoutVsSchematicTests(unittest.TestCase):

  def test_1_Basic(self):

    ut_testsrc = os.getenv("TESTSRC")

    lvs = pya.LayoutVsSchematic()

    self.assertEqual(lvs.xref() == None, True)
    self.assertEqual(lvs.reference == None, True)

  def test_2_Flow(self):

    ut_testsrc = os.getenv("TESTSRC")

    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "algo", "lvs_test_1.gds"))

    lvs = pya.LayoutVsSchematic(pya.RecursiveShapeIterator(ly, ly.top_cell(), []))

    nwell       = lvs.make_layer(ly.layer(1,  0), "nwell")
    active      = lvs.make_layer(ly.layer(2,  0), "active")
    pplus       = lvs.make_layer(ly.layer(10, 0), "pplus")
    nplus       = lvs.make_layer(ly.layer(11, 0), "nplus")
    poly        = lvs.make_layer(ly.layer(3,  0), "poly")
    poly_lbl    = lvs.make_text_layer(ly.layer(3, 1), "poly_lbl")
    diff_cont   = lvs.make_layer(ly.layer(4,  0), "diff_cont")
    poly_cont   = lvs.make_layer(ly.layer(5,  0), "poly_cont")
    metal1      = lvs.make_layer(ly.layer(6,  0), "metal1")
    metal1_lbl  = lvs.make_text_layer(ly.layer(6,  1), "metal1_lbl")
    via1        = lvs.make_layer(ly.layer(7,  0), "via1")
    metal2      = lvs.make_layer(ly.layer(8,  0), "metal2")
    metal2_lbl  = lvs.make_text_layer(ly.layer(8,  1), "metal2_lbl")
    bulk        = lvs.make_layer()

    # compute some layers
    active_in_nwell = active & nwell
    pactive = active_in_nwell & pplus
    ntie = active_in_nwell & nplus
    pgate = pactive & poly
    psd = pactive - pgate

    active_outside_nwell = active - nwell
    nactive = active_outside_nwell & nplus
    ptie = active_outside_nwell & pplus
    ngate = nactive & poly
    nsd = nactive - ngate

    # device extraction
    pmos_ex = pya.DeviceExtractorMOS4Transistor("PMOS")
    dl = { "SD": psd, "G": pgate, "P": poly, "W": nwell }
    lvs.extract_devices(pmos_ex, dl)

    nmos_ex = pya.DeviceExtractorMOS4Transistor("NMOS")
    dl = { "SD": nsd, "G": ngate, "P": poly, "W": bulk }
    lvs.extract_devices(nmos_ex, dl)

    # register derived layers for connectivity
    lvs.register(psd, "psd")
    lvs.register(nsd, "nsd")
    lvs.register(ptie, "ptie")
    lvs.register(ntie, "ntie")

    # intra-layer
    lvs.connect(psd)
    lvs.connect(nsd)
    lvs.connect(nwell)
    lvs.connect(poly)
    lvs.connect(diff_cont)
    lvs.connect(poly_cont)
    lvs.connect(metal1)
    lvs.connect(via1)
    lvs.connect(metal2)
    lvs.connect(ptie)
    lvs.connect(ntie)

    # inter-layer
    lvs.connect(psd,       diff_cont)
    lvs.connect(nsd,       diff_cont)
    lvs.connect(poly,      poly_cont)
    lvs.connect(poly_cont, metal1)
    lvs.connect(diff_cont, metal1)
    lvs.connect(diff_cont, ptie)
    lvs.connect(diff_cont, ntie)
    lvs.connect(nwell,     ntie)
    lvs.connect(metal1,    via1)
    lvs.connect(via1,      metal2)
    lvs.connect(poly,      poly_lbl)     #  attaches labels
    lvs.connect(metal1,    metal1_lbl)   #  attaches labels
    lvs.connect(metal2,    metal2_lbl)   #  attaches labels

    # global
    lvs.connect_global(ptie, "BULK")
    lvs.connect_global(bulk, "BULK")

    lvs.extract_netlist()

    lvs.netlist().combine_devices()

    lvs.netlist().make_top_level_pins()
    lvs.netlist().purge()

    # read the reference netlist
    reader = pya.NetlistSpiceReader()
    nl = pya.Netlist()
    nl.read(os.path.join(ut_testsrc, "testdata", "algo", "lvs_test_1.spi"), reader)

    self.assertEqual(lvs.reference == None, True)
    lvs.reference = nl
    self.assertEqual(lvs.reference == nl, True)

    # do the actual compare
    comparer = pya.NetlistComparer()
    res = lvs.compare(comparer)
    self.assertEqual(res, True)

    self.assertEqual(lvs.xref != None, True)

  def test_3_ReadAndWrite(self):

    ut_testsrc = os.getenv("TESTSRC")
    ut_testtmp = os.getenv("TESTTMP", "")

    lvs = pya.LayoutVsSchematic()

    infile = os.path.join(ut_testsrc, "testdata", "algo", "lvsdb_read_test.lvsdb")
    lvs.read(infile)

    tmp = os.path.join(ut_testtmp, "tmp.lvsdb")
    lvs.write(tmp)

    with open(tmp, 'r') as file:
      tmp_text = file.read()
    with open(infile, 'r') as file:
      infile_text = file.read()
    self.assertEqual(tmp_text, infile_text)

    self.assertEqual(",".join(lvs.layer_names()), "bulk,nwell,poly,poly_lbl,diff_cont,poly_cont,metal1,metal1_lbl,via1,metal2,metal2_lbl,ntie,psd,ptie,nsd")
    self.assertEqual(lvs.layer_name(lvs.layer_by_name("metal1")), "metal1")
    self.assertEqual(lvs.layer_name(lvs.layer_by_index(lvs.layer_of(lvs.layer_by_name("metal1")))), "metal1")

    tmp = os.path.join(ut_testtmp, "tmp.l2n")
    lvs.write_l2n(tmp)

    l2n = pya.LayoutToNetlist()
    l2n.read(tmp)
    self.assertEqual(",".join(l2n.layer_names()), "bulk,nwell,poly,poly_lbl,diff_cont,poly_cont,metal1,metal1_lbl,via1,metal2,metal2_lbl,ntie,psd,ptie,nsd")

    lvs2 = pya.LayoutVsSchematic()
    lvs2.read_l2n(tmp)
    self.assertEqual(",".join(lvs2.layer_names()), "bulk,nwell,poly,poly_lbl,diff_cont,poly_cont,metal1,metal1_lbl,via1,metal2,metal2_lbl,ntie,psd,ptie,nsd")

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBLayoutVsSchematicTests)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

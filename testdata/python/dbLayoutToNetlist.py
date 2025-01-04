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

class DBLayoutToNetlistTests(unittest.TestCase):

  def test_1_Basic(self):

    ut_testsrc = os.getenv("TESTSRC")

    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "algo", "device_extract_l1.gds"))

    l2n = pya.LayoutToNetlist(pya.RecursiveShapeIterator(ly, ly.top_cell(), []))

    l2n.threads = 17
    l2n.max_vertex_count = 42
    l2n.area_ratio = 7.5
    self.assertEqual(l2n.threads, 17)
    self.assertEqual(l2n.max_vertex_count, 42)
    self.assertEqual(l2n.area_ratio, 7.5)

    r = l2n.make_layer(ly.layer(6, 0))

    self.assertNotEqual(l2n.internal_layout() is ly, True)
    self.assertEqual(l2n.internal_layout().top_cell().name, ly.top_cell().name)
    self.assertEqual(l2n.internal_top_cell().name, ly.top_cell().name)

    self.assertNotEqual(l2n.layer_of(r), ly.layer(6, 0))  # would be a strange coincidence ...

    cm = l2n.const_cell_mapping_into(ly, ly.top_cell())
    for ci in range(0, l2n.internal_layout().cells()):
      self.assertEqual(l2n.internal_layout().cell(ci).name, ly.cell(cm.cell_mapping(ci)).name)

    ly2 = pya.Layout()
    ly2.create_cell(ly.top_cell().name)

    cm = l2n.cell_mapping_into(ly2, ly2.top_cell())
    self.assertEqual(ly2.cells(), ly.cells())
    for ci in range(0, l2n.internal_layout().cells()):
      self.assertEqual(l2n.internal_layout().cell(ci).name, ly2.cell(cm.cell_mapping(ci)).name)

    rmetal1 = l2n.make_polygon_layer(ly.layer(6, 0), "metal1")
    bulk_id = l2n.connect_global(rmetal1, "BULK")
    self.assertEqual(l2n.global_net_name(bulk_id), "BULK")

  def test_2_ShapesFromNet(self):

    ut_testsrc = os.getenv("TESTSRC")

    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "algo", "device_extract_l1_with_inv_nodes.gds"))

    l2n = pya.LayoutToNetlist(pya.RecursiveShapeIterator(ly, ly.top_cell(), []))

    # only plain backend connectivity

    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0), "metal1" )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1), "metal1_lbl" )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0), "via1" )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0), "metal2" )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1), "metal2_lbl" )

    # Intra-layer
    l2n.connect(rmetal1)
    l2n.connect(rvia1)
    l2n.connect(rmetal2)

    # Inter-layer
    l2n.connect(rmetal1,    rvia1)
    l2n.connect(rvia1,      rmetal2)
    l2n.connect(rmetal1,    rmetal1_lbl)   #  attaches labels
    l2n.connect(rmetal2,    rmetal2_lbl)   #  attaches labels

    # Perform netlist extraction
    l2n.extract_netlist()

    self.assertEqual(str(l2n.netlist()), """circuit TRANS ($1=$1,$2=$2);
end;
circuit INV2 (OUT=OUT,$2=$3,$3=$4);
  subcircuit TRANS $1 ($1=$4,$2=OUT);
  subcircuit TRANS $2 ($1=$3,$2=OUT);
  subcircuit TRANS $3 ($1=$2,$2=$4);
  subcircuit TRANS $4 ($1=$2,$2=$3);
end;
circuit RINGO ();
  subcircuit INV2 $1 (OUT='FB,OSC',$2=VSS,$3=VDD);
  subcircuit INV2 $2 (OUT=$I20,$2=VSS,$3=VDD);
  subcircuit INV2 $3 (OUT=$I19,$2=VSS,$3=VDD);
  subcircuit INV2 $4 (OUT=$I21,$2=VSS,$3=VDD);
  subcircuit INV2 $5 (OUT=$I22,$2=VSS,$3=VDD);
  subcircuit INV2 $6 (OUT=$I23,$2=VSS,$3=VDD);
  subcircuit INV2 $7 (OUT=$I24,$2=VSS,$3=VDD);
  subcircuit INV2 $8 (OUT=$I25,$2=VSS,$3=VDD);
  subcircuit INV2 $9 (OUT=$I26,$2=VSS,$3=VDD);
  subcircuit INV2 $10 (OUT=$I27,$2=VSS,$3=VDD);
end;
""")

    self.assertEqual(str(l2n.probe_net(rmetal2, pya.DPoint(0.0, 1.8))), "RINGO:FB,OSC")
    sc_path = []
    self.assertEqual(str(l2n.probe_net(rmetal2, pya.DPoint(0.0, 1.8), sc_path)), "RINGO:FB,OSC")
    self.assertEqual(len(sc_path), 0)
    self.assertEqual(repr(l2n.probe_net(rmetal2, pya.DPoint(-2.0, 1.8))), "None")

    n = l2n.probe_net(rmetal1, pya.Point(2600, 1000), None)
    self.assertEqual(str(n), "INV2:$2")
    sc_path = []
    n = l2n.probe_net(rmetal1, pya.Point(2600, 1000), sc_path)
    self.assertEqual(str(n), "INV2:$2")
    self.assertEqual(len(sc_path), 1)
    a = []
    t = pya.DCplxTrans()
    for sc in sc_path:
      a.append(sc.expanded_name())
      t = t * sc.trans
    self.assertEqual(",".join(a), "$2")
    self.assertEqual(str(t), "r0 *1 2.64,0")

    self.assertEqual(str(l2n.shapes_of_net(n, rmetal1, True)),
        "(-980,-420;-980,2420;-620,2420;-620,-420);(-800,820;-800,1180;580,1180;580,820);(-980,2420;-980,3180;-620,3180;-620,2420);(-980,-380;-980,380;-620,380;-620,-380)")

    shapes = pya.Shapes()
    l2n.shapes_of_net(n, rmetal1, True, shapes)
    r = pya.Region()
    for s in shapes.each():
      r.insert(s.polygon)
    self.assertEqual(str(r),
        "(-980,-420;-980,2420;-620,2420;-620,-420);(-800,820;-800,1180;580,1180;580,820);(-980,2420;-980,3180;-620,3180;-620,2420);(-980,-380;-980,380;-620,380;-620,-380)")

  def test_10_LayoutToNetlistExtractionWithoutDevices(self):

    ut_testsrc = os.getenv("TESTSRC")

    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "algo", "device_extract_l1.gds"))

    l2n = pya.LayoutToNetlist(pya.RecursiveShapeIterator(ly, ly.top_cell(), []))

    # only plain connectivity

    ractive     = l2n.make_layer(         ly.layer(2, 0), "active" )
    rpoly       = l2n.make_polygon_layer( ly.layer(3, 0), "poly" )
    rpoly_lbl   = l2n.make_text_layer(    ly.layer(3, 1), "poly_lbl" )
    rdiff_cont  = l2n.make_polygon_layer( ly.layer(4, 0), "diff_cont" )
    rpoly_cont  = l2n.make_polygon_layer( ly.layer(5, 0), "poly_cont" )
    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0), "metal1" )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1), "metal1_lbl" )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0), "via1" )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0), "metal2" )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1), "metal2_lbl" )

    rsd         = ractive - rpoly

    l2n.register(rsd, "sd")

    # Intra-layer
    l2n.connect(rsd)
    l2n.connect(rpoly)
    l2n.connect(rdiff_cont)
    l2n.connect(rpoly_cont)
    l2n.connect(rmetal1)
    l2n.connect(rvia1)
    l2n.connect(rmetal2)

    # Inter-layer
    l2n.connect(rsd,        rdiff_cont)
    l2n.connect(rpoly,      rpoly_cont)
    l2n.connect(rpoly_cont, rmetal1)
    l2n.connect(rdiff_cont, rmetal1)
    l2n.connect(rmetal1,    rvia1)
    l2n.connect(rvia1,      rmetal2)
    l2n.connect(rpoly,      rpoly_lbl)     #  attaches labels
    l2n.connect(rmetal1,    rmetal1_lbl)   #  attaches labels
    l2n.connect(rmetal2,    rmetal2_lbl)   #  attaches labels

    # Perform netlist extraction
    l2n.extract_netlist()

    self.assertEqual(str(l2n.netlist()), """circuit TRANS ($1=$1,$2=$2,$3=$3);
end;
circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);
  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);
  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);
  subcircuit TRANS $3 ($1=$5,$2=OUT,$3=$2);
  subcircuit TRANS $4 ($1=$4,$2=OUT,$3=$2);
end;
circuit RINGO ();
  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);
  subcircuit INV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD);
  subcircuit INV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD);
  subcircuit INV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD);
  subcircuit INV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD);
  subcircuit INV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD);
  subcircuit INV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD);
  subcircuit INV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD);
  subcircuit INV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD);
  subcircuit INV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD);
end;
""")

  def test_11_LayoutToNetlistExtractionWithDevices(self):

    ut_testsrc = os.getenv("TESTSRC")

    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "algo", "device_extract_l1.gds"))

    l2n = pya.LayoutToNetlist(pya.RecursiveShapeIterator(ly, ly.top_cell(), []))

    rnwell      = l2n.make_layer(         ly.layer(1, 0), "nwell" )
    ractive     = l2n.make_layer(         ly.layer(2, 0), "active" )
    rpoly       = l2n.make_polygon_layer( ly.layer(3, 0), "poly" )
    rpoly_lbl   = l2n.make_text_layer(    ly.layer(3, 1), "poly_lbl" )
    rdiff_cont  = l2n.make_polygon_layer( ly.layer(4, 0), "diff_cont" )
    rpoly_cont  = l2n.make_polygon_layer( ly.layer(5, 0), "poly_cont" )
    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0), "metal1" )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1), "metal1_lbl" )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0), "via1" )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0), "metal2" )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1), "metal2_lbl" )

    rpactive    = ractive & rnwell
    rpgate      = rpactive & rpoly
    rpsd        = rpactive - rpgate

    rnactive    = ractive - rnwell
    rngate      = rnactive & rpoly
    rnsd        = rnactive - rngate

    # PMOS transistor device extraction
    pmos_ex = pya.DeviceExtractorMOS3Transistor("PMOS")
    l2n.extract_devices(pmos_ex, { "SD": rpsd, "G": rpgate, "P": rpoly })

    # NMOS transistor device extraction
    nmos_ex = pya.DeviceExtractorMOS3Transistor("NMOS")
    l2n.extract_devices(nmos_ex, { "SD": rnsd, "G": rngate, "P": rpoly })

    # Define connectivity for netlist extraction

    l2n.register(rpsd, "psd")
    l2n.register(rnsd, "nsd")

    # Intra-layer
    l2n.connect(rpsd)
    l2n.connect(rnsd)
    l2n.connect(rpoly)
    l2n.connect(rdiff_cont)
    l2n.connect(rpoly_cont)
    l2n.connect(rmetal1)
    l2n.connect(rvia1)
    l2n.connect(rmetal2)

    # Inter-layer
    l2n.connect(rpsd,       rdiff_cont)
    l2n.connect(rnsd,       rdiff_cont)
    l2n.connect(rpoly,      rpoly_cont)
    l2n.connect(rpoly_cont, rmetal1)
    l2n.connect(rdiff_cont, rmetal1)
    l2n.connect(rmetal1,    rvia1)
    l2n.connect(rvia1,      rmetal2)
    l2n.connect(rpoly,      rpoly_lbl)     #  attaches labels
    l2n.connect(rmetal1,    rmetal1_lbl)   #  attaches labels
    l2n.connect(rmetal2,    rmetal2_lbl)   #  attaches labels

    # Perform netlist extraction
    l2n.extract_netlist()

    self.assertEqual(str(l2n.netlist()), """circuit RINGO ();
  subcircuit INV2 $1 (IN=$I8,$2=FB,OUT=OSC,$4=VSS,$5=VDD);
  subcircuit INV2 $2 (IN=FB,$2=$I38,OUT=$I19,$4=VSS,$5=VDD);
  subcircuit INV2 $3 (IN=$I19,$2=$I39,OUT=$I1,$4=VSS,$5=VDD);
  subcircuit INV2 $4 (IN=$I1,$2=$I40,OUT=$I2,$4=VSS,$5=VDD);
  subcircuit INV2 $5 (IN=$I2,$2=$I41,OUT=$I3,$4=VSS,$5=VDD);
  subcircuit INV2 $6 (IN=$I3,$2=$I42,OUT=$I4,$4=VSS,$5=VDD);
  subcircuit INV2 $7 (IN=$I4,$2=$I43,OUT=$I5,$4=VSS,$5=VDD);
  subcircuit INV2 $8 (IN=$I5,$2=$I44,OUT=$I6,$4=VSS,$5=VDD);
  subcircuit INV2 $9 (IN=$I6,$2=$I45,OUT=$I7,$4=VSS,$5=VDD);
  subcircuit INV2 $10 (IN=$I7,$2=$I46,OUT=$I8,$4=VSS,$5=VDD);
end;
circuit INV2 (IN=IN,$2=$2,OUT=OUT,$4=$4,$5=$5);
  device PMOS $1 (S=$2,G=IN,D=$5) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=$5,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
  device NMOS $3 (S=$2,G=IN,D=$4) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=$4,G=$2,D=OUT) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
  subcircuit TRANS $1 ($1=$2,$2=$4,$3=IN);
  subcircuit TRANS $2 ($1=$2,$2=$5,$3=IN);
  subcircuit TRANS $3 ($1=$5,$2=OUT,$3=$2);
  subcircuit TRANS $4 ($1=$4,$2=OUT,$3=$2);
end;
circuit TRANS ($1=$1,$2=$2,$3=$3);
end;
""")

    # cleanup now
    l2n._destroy()

  def test_12_LayoutToNetlistExtractionWithDevicesAndGlobalNets(self):

    ut_testsrc = os.getenv("TESTSRC")

    ly = pya.Layout()
    ly.read(os.path.join(ut_testsrc, "testdata", "algo", "device_extract_l3.gds"))

    l2n = pya.LayoutToNetlist(pya.RecursiveShapeIterator(ly, ly.top_cell(), []))

    rbulk       = l2n.make_layer(                           "bulk" )
    rnwell      = l2n.make_polygon_layer( ly.layer(1, 0)  , "nwell" )
    ractive     = l2n.make_polygon_layer( ly.layer(2, 0)  , "active" )
    rpoly       = l2n.make_polygon_layer( ly.layer(3, 0)  , "poly" )
    rpoly_lbl   = l2n.make_text_layer(    ly.layer(3, 1)  , "poly_lbl" )
    rdiff_cont  = l2n.make_polygon_layer( ly.layer(4, 0)  , "diff_cont" )
    rpoly_cont  = l2n.make_polygon_layer( ly.layer(5, 0)  , "poly_cont" )
    rmetal1     = l2n.make_polygon_layer( ly.layer(6, 0)  , "metal1" )
    rmetal1_lbl = l2n.make_text_layer(    ly.layer(6, 1)  , "metal1_lbl" )
    rvia1       = l2n.make_polygon_layer( ly.layer(7, 0)  , "via1" )
    rmetal2     = l2n.make_polygon_layer( ly.layer(8, 0)  , "metal2" )
    rmetal2_lbl = l2n.make_text_layer(    ly.layer(8, 1)  , "metal2_lbl" )
    rpplus      = l2n.make_polygon_layer( ly.layer(10, 0) , "pplus" )
    rnplus      = l2n.make_polygon_layer( ly.layer(11, 0) , "nplus" )

    ractive_in_nwell = ractive & rnwell
    rpactive    = ractive_in_nwell & rpplus
    rntie       = ractive_in_nwell & rnplus
    rpgate      = rpactive & rpoly
    rpsd        = rpactive - rpgate

    ractive_outside_nwell = ractive - rnwell
    rnactive    = ractive_outside_nwell & rnplus
    rptie       = ractive_outside_nwell & rpplus
    rngate      = rnactive & rpoly
    rnsd        = rnactive - rngate

    # PMOS transistor device extraction
    pmos_ex = pya.DeviceExtractorMOS4Transistor("PMOS")
    l2n.extract_devices(pmos_ex, { "SD": rpsd, "G": rpgate, "P": rpoly, "W": rnwell })

    # NMOS transistor device extraction
    nmos_ex = pya.DeviceExtractorMOS4Transistor("NMOS")
    l2n.extract_devices(nmos_ex, { "SD": rnsd, "G": rngate, "P": rpoly, "W": rbulk })

    # Define connectivity for netlist extraction

    l2n.register(rpsd, "psd")
    l2n.register(rnsd, "nsd")
    l2n.register(rptie, "ptie")
    l2n.register(rntie, "ntie")

    # Intra-layer
    l2n.connect(rpsd)
    l2n.connect(rnsd)
    l2n.connect(rnwell)
    l2n.connect(rpoly)
    l2n.connect(rdiff_cont)
    l2n.connect(rpoly_cont)
    l2n.connect(rmetal1)
    l2n.connect(rvia1)
    l2n.connect(rmetal2)
    l2n.connect(rptie)
    l2n.connect(rntie)

    # Inter-layer
    l2n.connect(rpsd,       rdiff_cont)
    l2n.connect(rnsd,       rdiff_cont)
    l2n.connect(rpoly,      rpoly_cont)
    l2n.connect(rpoly_cont, rmetal1)
    l2n.connect(rdiff_cont, rmetal1)
    l2n.connect(rdiff_cont, rntie)
    l2n.connect(rdiff_cont, rptie)
    l2n.connect(rnwell,     rntie)
    l2n.connect(rmetal1,    rvia1)
    l2n.connect(rvia1,      rmetal2)
    l2n.connect(rpoly,      rpoly_lbl)     #  attaches labels
    l2n.connect(rmetal1,    rmetal1_lbl)   #  attaches labels
    l2n.connect(rmetal2,    rmetal2_lbl)   #  attaches labels

    # Global connections
    l2n.connect_global(rptie, "BULK")
    l2n.connect_global(rbulk, "BULK")

    # Perform netlist extraction
    l2n.extract_netlist()

    self.assertEqual(str(l2n.netlist()), """circuit RINGO ();
  subcircuit INV2PAIR $1 (BULK=VSS,$2=FB,$3=VDD,$4=VSS,$5=$I11,$6=OSC,$7=VDD);
  subcircuit INV2PAIR $2 (BULK=VSS,$2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I17,$7=VDD);
  subcircuit INV2PAIR $3 (BULK=VSS,$2=$I23,$3=VDD,$4=VSS,$5=$I17,$6=$I9,$7=VDD);
  subcircuit INV2PAIR $4 (BULK=VSS,$2=$I24,$3=VDD,$4=VSS,$5=$I9,$6=$I10,$7=VDD);
  subcircuit INV2PAIR $5 (BULK=VSS,$2=$I25,$3=VDD,$4=VSS,$5=$I10,$6=$I11,$7=VDD);
end;
circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);
  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);
  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);
end;
circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);
  device PMOS $1 (S=$3,G=IN,D=VDD,B=$1) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=$3,D=OUT,B=$1) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
  device NMOS $3 (S=$3,G=IN,D=VSS,B=BULK) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=VSS,G=$3,D=OUT,B=BULK) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
  subcircuit TRANS $1 ($1=$3,$2=VSS,$3=IN);
  subcircuit TRANS $2 ($1=$3,$2=VDD,$3=IN);
  subcircuit TRANS $3 ($1=VDD,$2=OUT,$3=$3);
  subcircuit TRANS $4 ($1=VSS,$2=OUT,$3=$3);
end;
circuit TRANS ($1=$1,$2=$2,$3=$3);
end;
""")

    l2n.netlist().combine_devices()
    l2n.netlist().make_top_level_pins()
    l2n.netlist().purge()

    self.assertEqual(str(l2n.netlist()), """circuit RINGO (FB=FB,OSC=OSC,VDD=VDD,VSS=VSS);
  subcircuit INV2PAIR $1 (BULK=VSS,$2=FB,$3=VDD,$4=VSS,$5=$I11,$6=OSC,$7=VDD);
  subcircuit INV2PAIR $2 (BULK=VSS,$2=$I22,$3=VDD,$4=VSS,$5=FB,$6=$I17,$7=VDD);
  subcircuit INV2PAIR $3 (BULK=VSS,$2=$I23,$3=VDD,$4=VSS,$5=$I17,$6=$I9,$7=VDD);
  subcircuit INV2PAIR $4 (BULK=VSS,$2=$I24,$3=VDD,$4=VSS,$5=$I9,$6=$I10,$7=VDD);
  subcircuit INV2PAIR $5 (BULK=VSS,$2=$I25,$3=VDD,$4=VSS,$5=$I10,$6=$I11,$7=VDD);
end;
circuit INV2PAIR (BULK=BULK,$2=$I8,$3=$I6,$4=$I5,$5=$I3,$6=$I2,$7=$I1);
  subcircuit INV2 $1 ($1=$I1,IN=$I3,$3=$I7,OUT=$I4,VSS=$I5,VDD=$I6,BULK=BULK);
  subcircuit INV2 $2 ($1=$I1,IN=$I4,$3=$I8,OUT=$I2,VSS=$I5,VDD=$I6,BULK=BULK);
end;
circuit INV2 ($1=$1,IN=IN,$3=$3,OUT=OUT,VSS=VSS,VDD=VDD,BULK=BULK);
  device PMOS $1 (S=$3,G=IN,D=VDD,B=$1) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device PMOS $2 (S=VDD,G=$3,D=OUT,B=$1) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
  device NMOS $3 (S=$3,G=IN,D=VSS,B=BULK) (L=0.25,W=0.95,AS=0.49875,AD=0.26125,PS=2.95,PD=1.5);
  device NMOS $4 (S=VSS,G=$3,D=OUT,B=BULK) (L=0.25,W=0.95,AS=0.26125,AD=0.49875,PS=1.5,PD=2.95);
end;
""")

    # cleanup now
    l2n._destroy()

  def test_13_ReadAndWrite(self):

    ut_testsrc = os.getenv("TESTSRC")
    ut_testtmp = os.getenv("TESTTMP", "")

    l2n = pya.LayoutToNetlist()

    infile = os.path.join(ut_testsrc, "testdata", "algo", "l2n_reader_in.txt")
    l2n.read(infile)

    tmp = os.path.join(ut_testtmp, "tmp.txt")
    l2n.write(tmp)

    with open(tmp, 'r') as file:
      tmp_text = file.read()
    with open(infile, 'r') as file:
      infile_text = file.read()
    self.assertEqual(tmp_text, infile_text)

    self.assertEqual(",".join(l2n.layer_names()), "poly,poly_lbl,diff_cont,poly_cont,metal1,metal1_lbl,via1,metal2,metal2_lbl,psd,nsd")
    self.assertEqual(l2n.layer_name(l2n.layer_by_name("metal1")), "metal1")
    self.assertEqual(l2n.layer_name(l2n.layer_by_index(l2n.layer_of(l2n.layer_by_name("metal1")))), "metal1")

  def test_20_Antenna(self):

    ut_testsrc = os.getenv("TESTSRC")

    # --- simple antenna check

    input = os.path.join(ut_testsrc, "testdata", "algo", "antenna_l1.gds")
    ly = pya.Layout()
    ly.read(input)

    au = os.path.join(ut_testsrc, "testdata", "algo", "antenna_au1.gds")
    ly_au = pya.Layout()
    ly_au.read(au)

    dss = pya.DeepShapeStore()
    self.assertEqual(dss.is_singular(), False)

    rdiode = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(1, 0)), dss)
    rpoly = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(6, 0)), dss)
    rcont = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(8, 0)), dss)
    rmetal1 = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(9, 0)), dss)
    rvia1 = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(11, 0)), dss)
    rmetal2 = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(12, 0)), dss)
    self.assertEqual(dss.is_singular(), True)

    l2n = pya.LayoutToNetlist(dss)

    l2n.register(rdiode, "diode")
    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)

    l2n.extract_netlist()

    a1_3 = l2n.antenna_check(rpoly, rmetal1, 3)
    a1_10 = l2n.antenna_check(rpoly, rmetal1, 10)
    a1_30 = l2n.antenna_check(rpoly, rmetal1, 30)

    # Note: flatten.merged performs some normalization
    self.assertEqual(str(a1_3.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(100, 0)))), "")
    self.assertEqual(str(a1_10.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(101, 0)))), "")
    self.assertEqual(str(a1_30.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(102, 0)))), "")

    # --- same with flat

    l2n._destroy()

    input = os.path.join(ut_testsrc, "testdata", "algo", "antenna_l1.gds")
    ly = pya.Layout()
    ly.read(input)

    au = os.path.join(ut_testsrc, "testdata", "algo", "antenna_au1.gds")
    ly_au = pya.Layout()
    ly_au.read(au)

    rfdiode = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(1, 0)))
    rfpoly = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(6, 0)))
    rfcont = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(8, 0)))
    rfmetal1 = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(9, 0)))
    rfvia1 = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(11, 0)))
    rfmetal2 = pya.Region(ly.top_cell().begin_shapes_rec(ly.layer(12, 0)))
    self.assertEqual(rfdiode.is_deep(), False)
    self.assertEqual(rfpoly.is_deep(), False)
    self.assertEqual(rfmetal1.is_deep(), False)
    self.assertEqual(rfvia1.is_deep(), False)
    self.assertEqual(rfmetal2.is_deep(), False)

    l2n = pya.LayoutToNetlist(ly.top_cell().name, ly.dbu)

    l2n.register(rfdiode, "diode")
    l2n.register(rfpoly, "poly")
    l2n.register(rfcont, "cont")
    l2n.register(rfmetal1, "metal1")
    l2n.register(rfvia1, "via1")
    l2n.register(rfmetal2, "metal2")

    l2n.connect(rfpoly)
    l2n.connect(rfcont)
    l2n.connect(rfmetal1)
    l2n.connect(rfpoly, rfcont)
    l2n.connect(rfcont, rfmetal1)

    l2n.extract_netlist()

    a1_3 = l2n.antenna_check(rfpoly, rfmetal1, 3)
    a1_10 = l2n.antenna_check(rfpoly, rfmetal1, 10)
    a1_30 = l2n.antenna_check(rfpoly, rfmetal1, 30)

    # Note: flatten.merged performs some normalization
    self.assertEqual(str(a1_3.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(100, 0)))), "")
    self.assertEqual(str(a1_10.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(101, 0)))), "")
    self.assertEqual(str(a1_30.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(102, 0)))), "")

    # --- simple incremental antenna check with metal1 + metal2

    l2n._destroy()
    l2n = pya.LayoutToNetlist(dss)

    l2n.register(rdiode, "diode")
    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rmetal2)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)

    self.assertEqual(l2n.is_extracted(), False)
    l2n.extract_netlist()
    self.assertEqual(l2n.is_extracted(), True)

    a1_3 = l2n.antenna_check(rpoly, rmetal1, 3)
    a1_10 = l2n.antenna_check(rpoly, rmetal1, 10)
    a1_30 = l2n.antenna_check(rpoly, rmetal1, 30)

    # Note: flatten.merged performs some normalization
    self.assertEqual(str(a1_3.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(100, 0)))), "")
    self.assertEqual(str(a1_10.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(101, 0)))), "")
    self.assertEqual(str(a1_30.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(102, 0)))), "")

    l2n.connect(rmetal1, rvia1)
    l2n.connect(rvia1, rmetal2)

    self.assertEqual(l2n.is_extracted(), False)
    l2n.extract_netlist()
    self.assertEqual(l2n.is_extracted(), True)

    a2_5 = l2n.antenna_check(rpoly, rmetal2, 5)
    a2_10 = l2n.antenna_check(rpoly, rmetal2, 10)
    a2_17 = l2n.antenna_check(rpoly, rmetal2, 17)

    # Note: flatten.merged performs some normalization
    self.assertEqual(str(a2_5.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(200, 0)))), "")
    self.assertEqual(str(a2_10.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(201, 0)))), "")
    self.assertEqual(str(a2_17.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(202, 0)))), "")

    # --- simple antenna check with metal2

    l2n._destroy()
    l2n = pya.LayoutToNetlist(dss)

    l2n.register(rdiode, "diode")
    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rmetal2)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)
    l2n.connect(rmetal1, rvia1)
    l2n.connect(rvia1, rmetal2)

    l2n.extract_netlist()

    a2_5 = l2n.antenna_check(rpoly, rmetal2, 5)
    a2_10 = l2n.antenna_check(rpoly, rmetal2, 10)
    a2_17 = l2n.antenna_check(rpoly, rmetal2, 17)

    # Note: flatten.merged performs some normalization
    self.assertEqual(str(a2_5.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(200, 0)))), "")
    self.assertEqual(str(a2_10.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(201, 0)))), "")
    self.assertEqual(str(a2_17.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(202, 0)))), "")

    # --- antenna check with diodes and antenna effect reduction

    l2n._destroy()
    l2n = pya.LayoutToNetlist(dss)

    l2n.register(rdiode, "diode")
    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rdiode)
    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rdiode, rcont)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)

    l2n.extract_netlist()

    a3_3 = l2n.antenna_check(rpoly, rmetal1, 3, [ [ rdiode, 8.0 ] ] )
    a3_10 = l2n.antenna_check(rpoly, rmetal1, 10, [ [ rdiode, 8.0 ] ])
    a3_30 = l2n.antenna_check(rpoly, rmetal1, 30, [ [ rdiode, 8.0 ] ])

    # Note: flatten.merged performs some normalization
    self.assertEqual(str(a3_3.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(300, 0)))), "")
    self.assertEqual(str(a3_10.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(301, 0)))), "")
    self.assertEqual(str(a3_30.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(302, 0)))), "")

    # --- antenna check with diodes

    l2n._destroy()
    l2n = pya.LayoutToNetlist(dss)

    l2n.register(rdiode, "diode")
    l2n.register(rpoly, "poly")
    l2n.register(rcont, "cont")
    l2n.register(rmetal1, "metal1")
    l2n.register(rvia1, "via1")
    l2n.register(rmetal2, "metal2")

    l2n.connect(rdiode)
    l2n.connect(rpoly)
    l2n.connect(rcont)
    l2n.connect(rmetal1)
    l2n.connect(rdiode, rcont)
    l2n.connect(rpoly, rcont)
    l2n.connect(rcont, rmetal1)

    l2n.extract_netlist()

    a4_3 = l2n.antenna_check(rpoly, rmetal1, 3, [ rdiode ] )
    a4_10 = l2n.antenna_check(rpoly, rmetal1, 10, [ rdiode ])
    a4_30 = l2n.antenna_check(rpoly, rmetal1, 30, [ rdiode ])

    # Note: flatten.merged performs some normalization
    self.assertEqual(str(a4_3.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(400, 0)))), "")
    self.assertEqual(str(a4_10.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(401, 0)))), "")
    self.assertEqual(str(a4_30.flatten() ^ pya.Region(ly_au.top_cell().begin_shapes_rec(ly_au.layer(402, 0)))), "")


# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBLayoutToNetlistTests)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

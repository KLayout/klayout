
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

class EdgeNeighborhoodWithNetsVisitor(pya.EdgeNeighborhoodVisitor):

  def __init__(self):
    self.data = {}
    self.poly = None

  def begin_polygon(self, layout, cell, polygon):
    self.poly = {}
    self.data[str(polygon)] = self.poly
    
  def end_polygon(self):
    self.poly = None
    
  def on_edge(self, layout, cell, edge, neighborhood):
    edge_data = []
    self.poly[str(edge)] = edge_data
    for (x1, x2), polygons in neighborhood:
      for inp, poly in polygons.items():
        poly_str = "/".join(sorted([ str(p) for p in poly ]))
        edge_data.append(str(x1) + "," + str(x2) + " -> " + str(inp) + ": " + poly_str)

  def dump(self):
    res = ""
    for poly_key in sorted(self.data.keys()):
      res += "Polygon: " + poly_key + "\n"
      poly_data = self.data[poly_key]
      for edge_key in sorted(poly_data.keys()):
        res += " Edge: " + edge_key + "\n"
        edge_data = poly_data[edge_key]
        for e in edge_data:
          res += "  " + e + "\n"
    return res
    
class DBEdgeNeighborhoodWithNets(unittest.TestCase):

  def test_1_Basic(self):

    ut_testsrc = os.getenv("TESTSRC")
    infile = os.path.join(ut_testsrc, "testdata", "algo", "net_neighborhood.gds")

    # Build a LayoutToNetlist object from the layout
    # with a simple connectivity 1/0-2/0-3/0

    layout = pya.Layout()
    layout.read(infile)
    cell = layout.top_cell()

    l1 = layout.layer(1, 0)
    l2 = layout.layer(2, 0)
    l3 = layout.layer(3, 0)

    l2n = pya.LayoutToNetlist(pya.RecursiveShapeIterator(layout, cell, []))

    l1r = pya.Region(cell.begin_shapes_rec(l1))
    l2r = pya.Region(cell.begin_shapes_rec(l2))
    l3r = pya.Region(cell.begin_shapes_rec(l3))

    l2n.register(l1r, "L1")
    l2n.register(l2r, "L2")
    l2n.register(l3r, "L3")

    # intra-layer
    l2n.connect(l1r)
    l2n.connect(l2r)
    l2n.connect(l3r)

    # inter-layer
    l2n.connect(l1r, l2r)
    l2n.connect(l2r, l3r)

    l2n.extract_netlist()

    nl = l2n.netlist()
    tc = nl.top_circuit()

    # Dump the net information to a new Layout object
    # with properties attached (property key: "net")

    annotated_layout = pya.Layout()
    annotated_layout.dbu = layout.dbu

    annotated_top = annotated_layout.create_cell(cell.name)

    cm = l2n.cell_mapping_into(annotated_layout, annotated_top)
    lm = { annotated_layout.layer(l2n.layer_info(li)): l2n.layer_by_index(li) for li in l2n.layer_indexes() }

    l2n.build_all_nets(cm, annotated_layout, lm, None, "net")

    # Run the EdgeNeighborhood function on these net polygons with the 
    # net property included

    l1a = annotated_layout.layer(1, 0)
    l2a = annotated_layout.layer(2, 0)
    l3a = annotated_layout.layer(3, 0)

    l1ra = pya.Region(annotated_top.begin_shapes_rec(l1a))
    l1ra.enable_properties()
    l2ra = pya.Region(annotated_top.begin_shapes_rec(l2a))
    l2ra.enable_properties()
    l3ra = pya.Region(annotated_top.begin_shapes_rec(l3a))
    l3ra.enable_properties()

    visitor = EdgeNeighborhoodWithNetsVisitor()

    bext = 0
    eext = 0
    din = 10
    dout = 6000

    children = [
      pya.CompoundRegionOperationNode.new_primary(),  # l1ra, current polygon
      pya.CompoundRegionOperationNode.new_foreign(),  # l1ra, foreign polygons
      pya.CompoundRegionOperationNode.new_secondary(l2ra),
      pya.CompoundRegionOperationNode.new_secondary(l3ra)
    ] 

    node = pya.CompoundRegionOperationNode.new_edge_neighborhood(children, visitor, bext, eext, din, dout)

    l1ra.complex_op(node)

    # Check the results

    self.maxDiff = None
    self.assertEqual("\n" + visitor.dump(), """
Polygon: (-14000,0;-14000,15000;-11000,15000;-11000,0) props={net=>net1}
 Edge: (-11000,0;-14000,0) props={net=>net1}
  0.0,3000.0 -> 0: (0,-11;0,0;3000,0;3000,-11) props={net=>net1}
 Edge: (-11000,15000;-11000,0) props={net=>net1}
  0.0,500.0 -> 0: (0,-11;0,0;500,0;500,-11) props={net=>net1}
  0.0,500.0 -> 1: (0,1500;0,6001;500,6001;500,1500) props={net=>net1}
  0.0,500.0 -> 3: (0,-11;0,4500;500,4500;500,-11) props={net=>net1}
  500.0,2500.0 -> 0: (500,-11;500,0;2500,0;2500,-11) props={net=>net1}
  500.0,2500.0 -> 1: (500,1500;500,6001;2500,6001;2500,1500) props={net=>net1}
  500.0,2500.0 -> 2: (500,2000;500,4000;2500,4000;2500,2000) props={net=>net1}
  500.0,2500.0 -> 3: (500,-11;500,4500;2500,4500;2500,-11) props={net=>net1}
  2500.0,3000.0 -> 0: (2500,-11;2500,0;3000,0;3000,-11) props={net=>net1}
  2500.0,3000.0 -> 1: (2500,1500;2500,6001;3000,6001;3000,1500) props={net=>net1}
  2500.0,3000.0 -> 3: (2500,-11;2500,4500;3000,4500;3000,-11) props={net=>net1}
  3000.0,8500.0 -> 0: (3000,-11;3000,0;8500,0;8500,-11) props={net=>net1}
  3000.0,8500.0 -> 1: (3000,1500;3000,4500;8500,4500;8500,1500) props={net=>net1}/(3000,6000;3000,6001;8500,6001;8500,6000) props={net=>net1}
  3000.0,8500.0 -> 3: (3000,-11;3000,0;8500,0;8500,-11) props={net=>net1}
  8500.0,15000.0 -> 0: (8500,-11;8500,0;15000,0;15000,-11) props={net=>net1}
  8500.0,15000.0 -> 1: (8500,1500;8500,4500;15000,4500;15000,1500) props={net=>net1}/(8500,6000;8500,6001;15000,6001;15000,6000) props={net=>net1}
 Edge: (-14000,0;-14000,15000) props={net=>net1}
  0.0,6500.0 -> 0: (0,-11;0,0;6500,0;6500,-11) props={net=>net1}
  6500.0,15000.0 -> 0: (6500,-11;6500,0;15000,0;15000,-11) props={net=>net1}
  6500.0,15000.0 -> 3: (6500,-11;6500,0;15000,0;15000,-11) props={net=>net1}
 Edge: (-14000,15000;-11000,15000) props={net=>net1}
  0.0,3000.0 -> 0: (0,-11;0,0;3000,0;3000,-11) props={net=>net1}
  0.0,3000.0 -> 1: (0,1500;0,5000;3000,5000;3000,1500) props={net=>net3}
  0.0,3000.0 -> 3: (0,-11;0,0;3000,0;3000,-11) props={net=>net1}
  3000.0,3001.0 -> 1: (3000,1500;3000,5000;3001,5000;3001,1500) props={net=>net3}
  3000.0,3001.0 -> 3: (3000,-11;3000,0;3001,0;3001,-11) props={net=>net1}
Polygon: (-14000,16500;-14000,20000;0,20000;0,16500) props={net=>net3}
 Edge: (-14000,16500;-14000,20000) props={net=>net3}
  0.0,3500.0 -> 0: (0,-11;0,0;3500,0;3500,-11) props={net=>net3}
 Edge: (-14000,20000;0,20000) props={net=>net3}
  0.0,7000.0 -> 0: (0,-11;0,0;7000,0;7000,-11) props={net=>net3}
  7000.0,14000.0 -> 0: (7000,-11;7000,0;14000,0;14000,-11) props={net=>net3}
  7000.0,14000.0 -> 3: (7000,-11;7000,0;14000,0;14000,-11) props={net=>net2}
  14000.0,14001.0 -> 3: (14000,-11;14000,0;14001,0;14001,-11) props={net=>net2}
 Edge: (0,16500;-14000,16500) props={net=>net3}
  -1.0,0.0 -> 3: (-1,-11;-1,0;0,0;0,-11) props={net=>net2}
  0.0,5000.0 -> 0: (0,-11;0,0;5000,0;5000,-11) props={net=>net3}
  0.0,5000.0 -> 1: (0,1500;0,6001;5000,6001;5000,1500) props={net=>net1}
  0.0,5000.0 -> 3: (0,-11;0,0;5000,0;5000,-11) props={net=>net2}
  5000.0,6500.0 -> 0: (5000,-11;5000,0;6500,0;6500,-11) props={net=>net3}
  5000.0,6500.0 -> 1: (5000,1500;5000,4500;6500,4500;6500,1500) props={net=>net1}
  5000.0,6500.0 -> 3: (5000,-11;5000,0;6500,0;6500,-11) props={net=>net2}
  6500.0,7000.0 -> 0: (6500,-11;6500,0;7000,0;7000,-11) props={net=>net3}
  6500.0,7000.0 -> 1: (6500,1500;6500,6001;7000,6001;7000,1500) props={net=>net1}
  6500.0,7000.0 -> 3: (6500,-11;6500,0;7000,0;7000,-11) props={net=>net2}/(6500,1500;6500,4500;7000,4500;7000,1500) props={net=>net1}
  7000.0,9000.0 -> 0: (7000,-11;7000,0;9000,0;9000,-11) props={net=>net3}
  7000.0,9000.0 -> 1: (7000,1500;7000,6001;9000,6001;9000,1500) props={net=>net1}
  7000.0,9000.0 -> 2: (7000,2000;7000,4000;9000,4000;9000,2000) props={net=>net1}
  7000.0,9000.0 -> 3: (7000,1500;7000,4500;9000,4500;9000,1500) props={net=>net1}
  9000.0,9500.0 -> 0: (9000,-11;9000,0;9500,0;9500,-11) props={net=>net3}
  9000.0,9500.0 -> 1: (9000,1500;9000,6001;9500,6001;9500,1500) props={net=>net1}
  9000.0,9500.0 -> 3: (9000,1500;9000,4500;9500,4500;9500,1500) props={net=>net1}
  9500.0,11000.0 -> 0: (9500,-11;9500,0;11000,0;11000,-11) props={net=>net3}
  9500.0,11000.0 -> 3: (9500,1500;9500,4500;11000,4500;11000,1500) props={net=>net1}
  11000.0,11500.0 -> 0: (11000,-11;11000,0;11500,0;11500,-11) props={net=>net3}
  11000.0,11500.0 -> 1: (11000,1500;11000,6001;11500,6001;11500,1500) props={net=>net1}
  11000.0,11500.0 -> 3: (11000,1500;11000,6001;11500,6001;11500,1500) props={net=>net1}
  11500.0,13500.0 -> 0: (11500,-11;11500,0;13500,0;13500,-11) props={net=>net3}
  11500.0,13500.0 -> 1: (11500,1500;11500,6001;13500,6001;13500,1500) props={net=>net1}
  11500.0,13500.0 -> 2: (11500,2000;11500,4000;13500,4000;13500,2000) props={net=>net1}
  11500.0,13500.0 -> 3: (11500,1500;11500,6001;13500,6001;13500,1500) props={net=>net1}
  13500.0,14000.0 -> 0: (13500,-11;13500,0;14000,0;14000,-11) props={net=>net3}
  13500.0,14000.0 -> 1: (13500,1500;13500,6001;14000,6001;14000,1500) props={net=>net1}
  13500.0,14000.0 -> 3: (13500,1500;13500,6001;14000,6001;14000,1500) props={net=>net1}
 Edge: (0,20000;0,16500) props={net=>net3}
  0.0,3500.0 -> 0: (0,-11;0,0;3500,0;3500,-11) props={net=>net3}
  0.0,3500.0 -> 3: (0,-11;0,4500;3500,4500;3500,-11) props={net=>net2}
  3500.0,3501.0 -> 3: (3500,1500;3500,4500;3501,4500;3501,1500) props={net=>net2}
Polygon: (-9500,-5500;-9500,-1500;5000,-1500;5000,-5500) props={net=>net2}
 Edge: (-9500,-1500;5000,-1500) props={net=>net2}
  0.0,3000.0 -> 0: (0,-11;0,0;3000,0;3000,-11) props={net=>net2}
  0.0,3000.0 -> 1: (0,1500;0,6001;3000,6001;3000,1500) props={net=>net1}
  3000.0,4500.0 -> 0: (3000,-11;3000,0;4500,0;4500,-11) props={net=>net2}
  4500.0,9500.0 -> 0: (4500,-11;4500,0;9500,0;9500,-11) props={net=>net2}
  4500.0,9500.0 -> 1: (4500,1500;4500,6001;9500,6001;9500,1500) props={net=>net1}
  9500.0,11000.0 -> 0: (9500,-11;9500,0;11000,0;11000,-11) props={net=>net2}
  11000.0,14000.0 -> 0: (11000,-11;11000,0;14000,0;14000,-11) props={net=>net2}
  11000.0,14000.0 -> 3: (11000,-11;11000,6001;14000,6001;14000,-11) props={net=>net2}
  14000.0,14500.0 -> 0: (14000,-11;14000,0;14500,0;14500,-11) props={net=>net2}
 Edge: (-9500,-5500;-9500,-1500) props={net=>net2}
  0.0,4000.0 -> 0: (0,-11;0,0;4000,0;4000,-11) props={net=>net2}
 Edge: (5000,-1500;5000,-5500) props={net=>net2}
  0.0,4000.0 -> 0: (0,-11;0,0;4000,0;4000,-11) props={net=>net2}
 Edge: (5000,-5500;-9500,-5500) props={net=>net2}
  0.0,14500.0 -> 0: (0,-11;0,0;14500,0;14500,-11) props={net=>net2}
Polygon: (-9500,0;-9500,15000;0,15000;0,0;-5000,0;-5000,12000;-6500,12000;-6500,0) props={net=>net1}
 Edge: (-5000,0;-5000,12000) props={net=>net1}
  0.0,6500.0 -> 0: (0,-11;0,0;6500,0;6500,-11) props={net=>net1}/(0,1500;0,4500;6500,4500;6500,1500) props={net=>net1}
  0.0,6500.0 -> 1: (0,6000;0,6001;6500,6001;6500,6000) props={net=>net1}
  6500.0,12000.0 -> 0: (6500,-11;6500,0;12000,0;12000,-11) props={net=>net1}/(6500,1500;6500,4500;12000,4500;12000,1500) props={net=>net1}
  6500.0,12000.0 -> 1: (6500,6000;6500,6001;12000,6001;12000,6000) props={net=>net1}
  6500.0,12000.0 -> 3: (6500,6000;6500,6001;12000,6001;12000,6000) props={net=>net1}
  12000.0,12001.0 -> 0: (12000,-11;12000,4500;12001,4500;12001,-11) props={net=>net1}
  12000.0,12001.0 -> 1: (12000,6000;12000,6001;12001,6001;12001,6000) props={net=>net1}
  12000.0,12001.0 -> 3: (12000,1500;12000,6001;12001,6001;12001,1500) props={net=>net1}
 Edge: (-5000,12000;-6500,12000) props={net=>net1}
  -1.0,0.0 -> 0: (-1,-11;-1,6001;0,6001;0,-11) props={net=>net1}
  0.0,1500.0 -> 0: (0,-11;0,0;1500,0;1500,-11) props={net=>net1}
  1500.0,1501.0 -> 0: (1500,-11;1500,6001;1501,6001;1501,-11) props={net=>net1}
  1500.0,1501.0 -> 3: (1500,-11;1500,0;1501,0;1501,-11) props={net=>net1}
 Edge: (-6500,0;-9500,0) props={net=>net1}
  -1.0,0.0 -> 1: (-1,1500;-1,5500;0,5500;0,1500) props={net=>net2}
  0.0,3000.0 -> 0: (0,-11;0,0;3000,0;3000,-11) props={net=>net1}
  0.0,3000.0 -> 1: (0,1500;0,5500;3000,5500;3000,1500) props={net=>net2}
 Edge: (-6500,12000;-6500,0) props={net=>net1}
  -1.0,0.0 -> 0: (-1,-11;-1,6001;0,6001;0,-11) props={net=>net1}
  -1.0,0.0 -> 3: (-1,-11;-1,0;0,0;0,-11) props={net=>net1}
  0.0,12000.0 -> 0: (0,-11;0,0;12000,0;12000,-11) props={net=>net1}/(0,1500;0,6001;12000,6001;12000,1500) props={net=>net1}
 Edge: (-9500,0;-9500,15000) props={net=>net1}
  0.0,6500.0 -> 0: (0,-11;0,0;6500,0;6500,-11) props={net=>net1}
  0.0,6500.0 -> 1: (0,1500;0,4500;6500,4500;6500,1500) props={net=>net1}
  6500.0,12000.0 -> 0: (6500,-11;6500,0;12000,0;12000,-11) props={net=>net1}
  6500.0,12000.0 -> 1: (6500,1500;6500,4500;12000,4500;12000,1500) props={net=>net1}
  6500.0,12000.0 -> 3: (6500,1500;6500,4500;12000,4500;12000,1500) props={net=>net1}
  12000.0,12500.0 -> 0: (12000,-11;12000,0;12500,0;12500,-11) props={net=>net1}
  12000.0,12500.0 -> 1: (12000,1500;12000,4500;12500,4500;12500,1500) props={net=>net1}
  12000.0,12500.0 -> 3: (12000,-11;12000,4500;12500,4500;12500,-11) props={net=>net1}
  12500.0,14500.0 -> 0: (12500,-11;12500,0;14500,0;14500,-11) props={net=>net1}
  12500.0,14500.0 -> 1: (12500,1500;12500,4500;14500,4500;14500,1500) props={net=>net1}
  12500.0,14500.0 -> 2: (12500,2000;12500,4000;14500,4000;14500,2000) props={net=>net1}
  12500.0,14500.0 -> 3: (12500,-11;12500,4500;14500,4500;14500,-11) props={net=>net1}
  14500.0,15000.0 -> 0: (14500,-11;14500,0;15000,0;15000,-11) props={net=>net1}
  14500.0,15000.0 -> 1: (14500,1500;14500,4500;15000,4500;15000,1500) props={net=>net1}
  14500.0,15000.0 -> 3: (14500,-11;14500,4500;15000,4500;15000,-11) props={net=>net1}
 Edge: (-9500,15000;0,15000) props={net=>net1}
  -1.0,0.0 -> 1: (-1,1500;-1,5000;0,5000;0,1500) props={net=>net3}
  -1.0,0.0 -> 3: (-1,-11;-1,0;0,0;0,-11) props={net=>net1}
  0.0,2500.0 -> 0: (0,-11;0,0;2500,0;2500,-11) props={net=>net1}
  0.0,2500.0 -> 1: (0,1500;0,5000;2500,5000;2500,1500) props={net=>net3}
  0.0,2500.0 -> 3: (0,-11;0,0;2500,0;2500,-11) props={net=>net1}
  2500.0,3000.0 -> 0: (2500,-11;2500,0;3000,0;3000,-11) props={net=>net1}
  2500.0,3000.0 -> 1: (2500,1500;2500,5000;3000,5000;3000,1500) props={net=>net3}
  2500.0,3000.0 -> 3: (2500,-11;2500,0;3000,0;3000,-11) props={net=>net1}/(2500,1500;2500,5000;3000,5000;3000,1500) props={net=>net2}
  3000.0,9500.0 -> 0: (3000,-11;3000,0;9500,0;9500,-11) props={net=>net1}
  3000.0,9500.0 -> 1: (3000,1500;3000,5000;9500,5000;9500,1500) props={net=>net3}
  3000.0,9500.0 -> 3: (3000,1500;3000,5000;9500,5000;9500,1500) props={net=>net2}
  9500.0,9501.0 -> 3: (9500,1500;9500,5000;9501,5000;9501,1500) props={net=>net2}
 Edge: (0,0;-5000,0) props={net=>net1}
  -1.0,0.0 -> 1: (-1,1500;-1,5500;0,5500;0,1500) props={net=>net2}
  -1.0,0.0 -> 3: (-1,2000;-1,5000;0,5000;0,2000) props={net=>net2}
  0.0,3000.0 -> 0: (0,-11;0,0;3000,0;3000,-11) props={net=>net1}
  0.0,3000.0 -> 1: (0,1500;0,5500;3000,5500;3000,1500) props={net=>net2}
  0.0,3000.0 -> 3: (0,2000;0,5000;3000,5000;3000,2000) props={net=>net2}
  3000.0,5000.0 -> 0: (3000,-11;3000,0;5000,0;5000,-11) props={net=>net1}
  3000.0,5000.0 -> 1: (3000,1500;3000,5500;5000,5500;5000,1500) props={net=>net2}
  5000.0,5001.0 -> 1: (5000,1500;5000,5500;5001,5500;5001,1500) props={net=>net2}
 Edge: (0,15000;0,0) props={net=>net1}
  -1.0,0.0 -> 3: (-1,1500;-1,4500;0,4500;0,1500) props={net=>net2}
  0.0,15000.0 -> 0: (0,-11;0,0;15000,0;15000,-11) props={net=>net1}
  0.0,15000.0 -> 3: (0,1500;0,4500;15000,4500;15000,1500) props={net=>net2}
  15000.0,15001.0 -> 3: (15000,1500;15000,4500;15001,4500;15001,1500) props={net=>net2}
""")


# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBEdgeNeighborhoodWithNets)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)



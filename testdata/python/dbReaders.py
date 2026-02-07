# encoding: UTF-8

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
import sys

class DBReadersTests(unittest.TestCase):

  # Common Options
  def test_common_options(self):

    opt = pya.LoadLayoutOptions()
    lm = pya.LayerMap()
    lm.map(pya.LayerInfo(1, 0), 2, pya.LayerInfo(42, 17))
    opt.set_layer_map(lm, True)

    self.assertEqual(opt.layer_map.to_string(), "1/0 : 42/17\n")
    self.assertEqual(opt.create_other_layers, True)

    opt.create_other_layers = False
    self.assertEqual(opt.create_other_layers, False)

    opt.select_all_layers()
    self.assertEqual(opt.layer_map.to_string(), "")
    self.assertEqual(opt.create_other_layers, True)

    opt.text_enabled = True
    self.assertEqual(opt.text_enabled, True)
    opt.text_enabled = False
    self.assertEqual(opt.text_enabled, False)

    opt.properties_enabled = True
    self.assertEqual(opt.properties_enabled, True)
    opt.properties_enabled = False
    self.assertEqual(opt.properties_enabled, False)

  # GDS2 Options
  def test_gds2_options(self):

    opt = pya.LoadLayoutOptions()
    lm = pya.LayerMap()
    lm.map(pya.LayerInfo(1, 0), 2, pya.LayerInfo(42, 17))
    opt.set_layer_map(lm, True)

    opt.gds2_allow_multi_xy_records = True
    self.assertEqual(opt.gds2_allow_multi_xy_records, True)
    opt.gds2_allow_multi_xy_records = False
    self.assertEqual(opt.gds2_allow_multi_xy_records, False)

    opt.gds2_resolve_skew_arrays = True
    self.assertEqual(opt.gds2_resolve_skew_arrays, True)
    opt.gds2_resolve_skew_arrays = False
    self.assertEqual(opt.gds2_resolve_skew_arrays, False)

    opt.gds2_allow_big_records = True
    self.assertEqual(opt.gds2_allow_big_records, True)
    opt.gds2_allow_big_records = False
    self.assertEqual(opt.gds2_allow_big_records, False)

    opt.gds2_box_mode = 1
    self.assertEqual(opt.gds2_box_mode, 1)
    opt.gds2_box_mode = 2
    self.assertEqual(opt.gds2_box_mode, 2)

  # OASIS Options
  def test_oasis_options(self):

    # none yet.
    pass

  # DXF Options
  def test_dxf_options(self):

    opt = pya.LoadLayoutOptions()
    lm = pya.LayerMap()
    lm.map(pya.LayerInfo(1, 0), 2, pya.LayerInfo(42, 17))
    opt.dxf_set_layer_map(lm, True)

    self.assertEqual(opt.dxf_layer_map.to_string(), "1/0 : 42/17\n")
    self.assertEqual(opt.dxf_create_other_layers, True)

    opt.dxf_create_other_layers = False
    self.assertEqual(opt.dxf_create_other_layers, False)

    opt.dxf_select_all_layers()
    self.assertEqual(opt.dxf_layer_map.to_string(), "")
    self.assertEqual(opt.dxf_create_other_layers, True)

    opt.dxf_dbu = 0.5
    self.assertEqual(opt.dxf_dbu, 0.5)

    opt.dxf_unit = 42
    self.assertEqual(opt.dxf_unit, 42)

    opt.dxf_text_scaling = 0.25
    self.assertEqual(opt.dxf_text_scaling, 0.25)

    opt.dxf_circle_points = 142
    self.assertEqual(opt.dxf_circle_points, 142)

    opt.dxf_circle_accuracy = 1.5
    self.assertEqual(opt.dxf_circle_accuracy, 1.5)

    opt.dxf_contour_accuracy = 0.75
    self.assertEqual(opt.dxf_contour_accuracy, 0.75)

    opt.dxf_render_texts_as_polygons = True
    self.assertEqual(opt.dxf_render_texts_as_polygons, True)
    opt.dxf_render_texts_as_polygons = False
    self.assertEqual(opt.dxf_render_texts_as_polygons, False)

    opt.dxf_keep_layer_names = True
    self.assertEqual(opt.dxf_keep_layer_names, True)
    opt.dxf_keep_layer_names = False
    self.assertEqual(opt.dxf_keep_layer_names, False)

    opt.dxf_keep_other_cells = True
    self.assertEqual(opt.dxf_keep_other_cells, True)
    opt.dxf_keep_other_cells = False
    self.assertEqual(opt.dxf_keep_other_cells, False)

    opt.dxf_polyline_mode = 2
    self.assertEqual(opt.dxf_polyline_mode, 2)
    opt.dxf_polyline_mode = 4
    self.assertEqual(opt.dxf_polyline_mode, 4)
  
  # CIF Options
  def test_cif_options(self):

    opt = pya.LoadLayoutOptions()
    lm = pya.LayerMap()
    lm.map(pya.LayerInfo(1, 0), 2, pya.LayerInfo(42, 17))
    opt.cif_set_layer_map(lm, True)

    self.assertEqual(opt.cif_layer_map.to_string(), "1/0 : 42/17\n")
    self.assertEqual(opt.cif_create_other_layers, True)

    opt.cif_create_other_layers = False
    self.assertEqual(opt.cif_create_other_layers, False)

    opt.cif_select_all_layers()
    self.assertEqual(opt.cif_layer_map.to_string(), "")
    self.assertEqual(opt.cif_create_other_layers, True)

    opt.cif_keep_layer_names = True
    self.assertEqual(opt.cif_keep_layer_names, True)
    opt.cif_keep_layer_names = False
    self.assertEqual(opt.cif_keep_layer_names, False)

    opt.cif_wire_mode = 2
    self.assertEqual(opt.cif_wire_mode, 2)
    opt.cif_wire_mode = 4
    self.assertEqual(opt.cif_wire_mode, 4)

    opt.cif_dbu = 0.5
    self.assertEqual(opt.cif_dbu, 0.5)
  
# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(DBReadersTests)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)


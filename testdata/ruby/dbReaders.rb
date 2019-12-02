# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2019 Matthias Koefferlein
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

if !$:.member?(File::dirname($0))
  $:.push(File::dirname($0))
end

load("test_prologue.rb")

class DBReaders_TestClass < TestBase

  # Common Options
  def test_common_options

    opt = RBA::LoadLayoutOptions::new
    lm = RBA::LayerMap::new
    lm.map(RBA::LayerInfo::new(1, 0), 2, RBA::LayerInfo::new(42, 17))
    opt.set_layer_map(lm, true)

    assert_equal(opt.layer_map.to_string, "1/0 : 42/17\n")
    assert_equal(opt.create_other_layers?, true)

    opt.create_other_layers = false
    assert_equal(opt.create_other_layers?, false)

    opt.select_all_layers
    assert_equal(opt.layer_map.to_string, "")
    assert_equal(opt.create_other_layers?, true)

    opt.text_enabled = true
    assert_equal(opt.text_enabled?, true)
    opt.text_enabled = false
    assert_equal(opt.text_enabled?, false)

    opt.properties_enabled = true
    assert_equal(opt.properties_enabled?, true)
    opt.properties_enabled = false
    assert_equal(opt.properties_enabled?, false)

  end

  # GDS2 Options
  def test_gds2_options

    opt = RBA::LoadLayoutOptions::new
    lm = RBA::LayerMap::new
    lm.map(RBA::LayerInfo::new(1, 0), 2, RBA::LayerInfo::new(42, 17))
    opt.set_layer_map(lm, true)

    opt.gds2_allow_multi_xy_records = true
    assert_equal(opt.gds2_allow_multi_xy_records?, true)
    opt.gds2_allow_multi_xy_records = false
    assert_equal(opt.gds2_allow_multi_xy_records?, false)

    opt.gds2_allow_big_records = true
    assert_equal(opt.gds2_allow_big_records?, true)
    opt.gds2_allow_big_records = false
    assert_equal(opt.gds2_allow_big_records?, false)

    opt.gds2_box_mode = 1
    assert_equal(opt.gds2_box_mode, 1)
    opt.gds2_box_mode = 2
    assert_equal(opt.gds2_box_mode, 2)

  end

  # OASIS Options
  def test_oasis_options

    # none yet.

  end

  # DXF Options
  def test_dxf_options

    opt = RBA::LoadLayoutOptions::new
    lm = RBA::LayerMap::new
    lm.map(RBA::LayerInfo::new(1, 0), 2, RBA::LayerInfo::new(42, 17))
    opt.dxf_set_layer_map(lm, true)

    assert_equal(opt.dxf_layer_map.to_string, "1/0 : 42/17\n")
    assert_equal(opt.dxf_create_other_layers?, true)

    opt.dxf_create_other_layers = false
    assert_equal(opt.dxf_create_other_layers?, false)

    opt.dxf_select_all_layers
    assert_equal(opt.dxf_layer_map.to_string, "")
    assert_equal(opt.dxf_create_other_layers?, true)

    opt.dxf_dbu = 0.5
    assert_equal(opt.dxf_dbu, 0.5)

    opt.dxf_unit = 42
    assert_equal(opt.dxf_unit, 42)

    opt.dxf_text_scaling = 0.25
    assert_equal(opt.dxf_text_scaling, 0.25)

    opt.dxf_circle_points = 142
    assert_equal(opt.dxf_circle_points, 142)

    opt.dxf_circle_accuracy = 1.5
    assert_equal(opt.dxf_circle_accuracy, 1.5)

    opt.dxf_contour_accuracy = 0.75
    assert_equal(opt.dxf_contour_accuracy, 0.75)

    opt.dxf_render_texts_as_polygons = true
    assert_equal(opt.dxf_render_texts_as_polygons?, true)
    opt.dxf_render_texts_as_polygons = false
    assert_equal(opt.dxf_render_texts_as_polygons?, false)

    opt.dxf_keep_layer_names = true
    assert_equal(opt.dxf_keep_layer_names?, true)
    opt.dxf_keep_layer_names = false
    assert_equal(opt.dxf_keep_layer_names?, false)

    opt.dxf_keep_other_cells = true
    assert_equal(opt.dxf_keep_other_cells?, true)
    opt.dxf_keep_other_cells = false
    assert_equal(opt.dxf_keep_other_cells?, false)

    opt.dxf_polyline_mode = 2
    assert_equal(opt.dxf_polyline_mode, 2)
    opt.dxf_polyline_mode = 4
    assert_equal(opt.dxf_polyline_mode, 4)
  
  end

  # CIF Options
  def test_cif_options

    opt = RBA::LoadLayoutOptions::new
    lm = RBA::LayerMap::new
    lm.map(RBA::LayerInfo::new(1, 0), 2, RBA::LayerInfo::new(42, 17))
    opt.cif_set_layer_map(lm, true)

    assert_equal(opt.cif_layer_map.to_string, "1/0 : 42/17\n")
    assert_equal(opt.cif_create_other_layers?, true)

    opt.cif_create_other_layers = false
    assert_equal(opt.cif_create_other_layers?, false)

    opt.cif_select_all_layers
    assert_equal(opt.cif_layer_map.to_string, "")
    assert_equal(opt.cif_create_other_layers?, true)

    opt.cif_keep_layer_names = true
    assert_equal(opt.cif_keep_layer_names?, true)
    opt.cif_keep_layer_names = false
    assert_equal(opt.cif_keep_layer_names?, false)

    opt.cif_wire_mode = 2
    assert_equal(opt.cif_wire_mode, 2)
    opt.cif_wire_mode = 4
    assert_equal(opt.cif_wire_mode, 4)

    opt.cif_dbu = 0.5
    assert_equal(opt.cif_dbu, 0.5)
  
  end

  # MAG Options
  def test_mag_options

    opt = RBA::LoadLayoutOptions::new
    lm = RBA::LayerMap::new
    lm.map(RBA::LayerInfo::new(1, 0), 2, RBA::LayerInfo::new(42, 17))
    opt.mag_set_layer_map(lm, true)

    assert_equal(opt.mag_layer_map.to_string, "1/0 : 42/17\n")
    assert_equal(opt.mag_create_other_layers?, true)

    opt.mag_create_other_layers = false
    assert_equal(opt.mag_create_other_layers?, false)

    opt.mag_select_all_layers
    assert_equal(opt.mag_layer_map.to_string, "")
    assert_equal(opt.mag_create_other_layers?, true)

    opt.mag_keep_layer_names = true
    assert_equal(opt.mag_keep_layer_names?, true)
    opt.mag_keep_layer_names = false
    assert_equal(opt.mag_keep_layer_names?, false)

    opt.mag_dbu = 0.5
    assert_equal(opt.mag_dbu, 0.5)
  
    opt.mag_lambda = 0.125
    assert_equal(opt.mag_lambda, 0.125)
  
    assert_equal(opt.mag_merge, true)
    opt.mag_merge = false
    assert_equal(opt.mag_merge?, false)
  
    assert_equal(opt.mag_library_paths, [])
    opt.mag_library_paths = [ "a", "b" ]
    assert_equal(opt.mag_library_paths, [ "a", "b" ])

  end

end

load("test_epilogue.rb")



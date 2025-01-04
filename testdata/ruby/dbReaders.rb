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

if !$:.member?(File::dirname($0))
  $:.push(File::dirname($0))
end

load("test_prologue.rb")

class DBReaders_TestClass < TestBase

  # Common Options
  def test_common_options

    opt = RBA::LoadLayoutOptions::new

    assert_equal(opt.warn_level, 1)
    opt.warn_level = 2
    assert_equal(opt.warn_level, 2)

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

    assert_equal(opt.cell_conflict_resolution, RBA::LoadLayoutOptions::AddToCell)
    opt.cell_conflict_resolution = RBA::LoadLayoutOptions::OverwriteCell
    assert_equal(opt.cell_conflict_resolution, RBA::LoadLayoutOptions::OverwriteCell)
    opt.cell_conflict_resolution = RBA::LoadLayoutOptions::SkipNewCell
    assert_equal(opt.cell_conflict_resolution, RBA::LoadLayoutOptions::SkipNewCell)
    opt.cell_conflict_resolution = RBA::LoadLayoutOptions::RenameCell
    assert_equal(opt.cell_conflict_resolution, RBA::LoadLayoutOptions::RenameCell)

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

  # LEF/DEF Options
  def test_lefdef_options

    conf = RBA::LEFDEFReaderConfiguration::new
    lm = RBA::LayerMap::new
    lm.map(RBA::LayerInfo::new(1, 0), 2, RBA::LayerInfo::new(42, 17))
    conf.layer_map = lm

    opt = RBA::LoadLayoutOptions::new
    opt.lefdef_config = conf
    assert_equal(opt.lefdef_config.layer_map.to_string, "1/0 : 42/17\n")
    assert_equal(opt.dup.lefdef_config.layer_map.to_string, "1/0 : 42/17\n")

    assert_equal(conf.layer_map.to_string, "1/0 : 42/17\n")

    conf.create_other_layers = false
    assert_equal(conf.create_other_layers, false)
    conf.create_other_layers = true
    assert_equal(conf.create_other_layers, true)

    conf.dbu = 2.5
    assert_equal(conf.dbu, 2.5)

    assert_equal(conf.net_property_name, 1)
    conf.net_property_name = "x"
    assert_equal(conf.net_property_name, "x")
    conf.net_property_name = 2
    assert_equal(conf.net_property_name, 2)
    conf.net_property_name = nil
    assert_equal(conf.net_property_name, nil)

    assert_equal(conf.pin_property_name, nil)
    conf.pin_property_name = "y"
    assert_equal(conf.pin_property_name, "y")
    conf.pin_property_name = 3
    assert_equal(conf.pin_property_name, 3)
    conf.pin_property_name = nil
    assert_equal(conf.pin_property_name, nil)

    assert_equal(conf.instance_property_name, 1)
    conf.instance_property_name = "z"
    assert_equal(conf.instance_property_name, "z")
    conf.instance_property_name = 4
    assert_equal(conf.instance_property_name, 4)
    conf.instance_property_name = nil
    assert_equal(conf.instance_property_name, nil)

    assert_equal(conf.produce_cell_outlines, true)
    conf.produce_cell_outlines = false
    assert_equal(conf.produce_cell_outlines, false)

    assert_equal(conf.cell_outline_layer, "OUTLINE")
    conf.cell_outline_layer = "17/1"
    assert_equal(conf.cell_outline_layer, "17/1")

    assert_equal(conf.produce_placement_blockages, true)
    conf.produce_placement_blockages = false
    assert_equal(conf.produce_placement_blockages, false)

    assert_equal(conf.placement_blockage_layer, "PLACEMENT_BLK")
    conf.placement_blockage_layer = "17/2"
    assert_equal(conf.placement_blockage_layer, "17/2")

    assert_equal(conf.produce_via_geometry, true)
    conf.produce_via_geometry = false
    assert_equal(conf.produce_via_geometry, false)

    assert_equal(conf.via_geometry_suffix, "")
    conf.via_geometry_suffix = "XVIA"
    assert_equal(conf.via_geometry_suffix, "XVIA")

    assert_equal(conf.via_cellname_prefix, "VIA_")
    conf.via_cellname_prefix = "ABC"
    assert_equal(conf.via_cellname_prefix, "ABC")

    assert_equal(conf.via_geometry_datatype, 0)
    conf.via_geometry_datatype = 17
    assert_equal(conf.via_geometry_datatype, 17)

    assert_equal(conf.produce_pins, true)
    conf.produce_pins = false
    assert_equal(conf.produce_pins, false)

    assert_equal(conf.pins_suffix, ".PIN")
    conf.pins_suffix = "XPIN"
    assert_equal(conf.pins_suffix, "XPIN")

    assert_equal(conf.pins_datatype, 2)
    conf.pins_datatype = 18
    assert_equal(conf.pins_datatype, 18)

    assert_equal(conf.produce_lef_pins, true)
    conf.produce_lef_pins = false
    assert_equal(conf.produce_lef_pins, false)

    assert_equal(conf.lef_pins_suffix, ".PIN")
    conf.lef_pins_suffix = "LEFPIN"
    assert_equal(conf.lef_pins_suffix, "LEFPIN")

    assert_equal(conf.lef_pins_datatype, 2)
    conf.lef_pins_datatype = 181
    assert_equal(conf.lef_pins_datatype, 181)

    assert_equal(conf.produce_fills, true)
    conf.produce_fills = false
    assert_equal(conf.produce_fills, false)

    assert_equal(conf.fills_suffix, ".FILL")
    conf.fills_suffix = "XFILL"
    assert_equal(conf.fills_suffix, "XFILL")

    assert_equal(conf.fills_datatype, 5)
    conf.fills_datatype = 19
    assert_equal(conf.fills_datatype, 19)

    assert_equal(conf.produce_obstructions, true)
    conf.produce_obstructions = false
    assert_equal(conf.produce_obstructions, false)

    assert_equal(conf.obstructions_suffix, ".OBS")
    conf.obstructions_suffix = "XOBS"
    assert_equal(conf.obstructions_suffix, "XOBS")

    assert_equal(conf.obstructions_datatype, 3)
    conf.obstructions_datatype = 19
    assert_equal(conf.obstructions_datatype, 19)

    assert_equal(conf.produce_blockages, true)
    conf.produce_blockages = false
    assert_equal(conf.produce_blockages, false)

    assert_equal(conf.blockages_suffix, ".BLK")
    conf.blockages_suffix = "XBLK"
    assert_equal(conf.blockages_suffix, "XBLK")

    assert_equal(conf.blockages_datatype, 4)
    conf.blockages_datatype = 20
    assert_equal(conf.blockages_datatype, 20)

    assert_equal(conf.produce_labels, true)
    conf.produce_labels = false
    assert_equal(conf.produce_labels, false)

    assert_equal(conf.labels_suffix, ".LABEL")
    conf.labels_suffix = "XLABEL"
    assert_equal(conf.labels_suffix, "XLABEL")

    assert_equal(conf.labels_datatype, 1)
    conf.labels_datatype = 21
    assert_equal(conf.labels_datatype, 21)

    assert_equal(conf.produce_routing, true)
    conf.produce_routing = false
    assert_equal(conf.produce_routing, false)

    assert_equal(conf.routing_suffix, "")
    conf.routing_suffix = "XROUT"
    assert_equal(conf.routing_suffix, "XROUT")

    assert_equal(conf.routing_datatype, 0)
    conf.routing_datatype = 22
    assert_equal(conf.routing_datatype, 22)

    assert_equal(conf.produce_special_routing, true)
    conf.produce_special_routing = false
    assert_equal(conf.produce_special_routing, false)

    assert_equal(conf.special_routing_suffix, "")
    conf.special_routing_suffix = "SPROUT"
    assert_equal(conf.special_routing_suffix, "SPROUT")

    assert_equal(conf.special_routing_datatype, 0)
    conf.special_routing_datatype = 23
    assert_equal(conf.special_routing_datatype, 23)

    assert_equal(conf.separate_groups, false)
    conf.separate_groups = true
    assert_equal(conf.separate_groups, true)

    assert_equal(conf.map_file, "")
    conf.map_file = "xyz.map"
    assert_equal(conf.map_file, "xyz.map")

    assert_equal(conf.lef_files.join(","), "")
    conf.lef_files = [ "u.lef", "v.lef" ]
    assert_equal(conf.lef_files.join(","), "u.lef,v.lef")

    assert_equal(conf.macro_layout_files.join(","), "")
    conf.macro_layout_files = [ "u.gds", "v.oas" ]
    assert_equal(conf.macro_layout_files.join(","), "u.gds,v.oas")

    assert_equal(conf.read_lef_with_def, true)
    conf.read_lef_with_def = false
    assert_equal(conf.read_lef_with_def, false)

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



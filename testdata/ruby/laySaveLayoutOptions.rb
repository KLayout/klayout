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

if !$:.member?(File::dirname($0))
  $:.push(File::dirname($0))
end

load("test_prologue.rb")

class SaveLayoutOptions_TestClass < TestBase

  # SaveLayoutOptions tests
  def test_1

    opt = RBA::SaveLayoutOptions::new

    # just smoke test - no query methods, so we can't check the results
    opt.add_cell(1)
    opt.add_this_cell(1)
    opt.add_layer(1, RBA::LayerInfo::new(17, 5))
    opt.clear_cells
    opt.deselect_all_layers
    opt.select_cell(1)
    opt.select_this_cell(1)
    opt.select_all_cells
    opt.select_all_layers
    
    assert_equal(opt.dbu, 0.0)
    opt.dbu = 0.5
    assert_equal(opt.dbu, 0.5)

    assert_equal(opt.libname, "")
    opt.libname = "ABC"
    assert_equal(opt.libname, "ABC")
    assert_equal(opt.gds2_libname, "ABC")
    opt.gds2_libname = "X"
    assert_equal(opt.libname, "X")
    assert_equal(opt.gds2_libname, "X")

    opt.scale_factor = 1.5
    assert_equal(opt.scale_factor, 1.5)

    opt.keep_instances = false
    assert_equal(opt.keep_instances?, false)
    opt.keep_instances = true
    assert_equal(opt.keep_instances?, true)

    opt.write_context_info = false
    assert_equal(opt.write_context_info?, false)
    opt.write_context_info = true
    assert_equal(opt.write_context_info?, true)

    opt.no_empty_cells = false
    assert_equal(opt.no_empty_cells?, false)
    opt.no_empty_cells = true
    assert_equal(opt.no_empty_cells?, true)

    opt.format = "CIF"
    assert_equal(opt.format, "CIF")
    opt.format = "DXF"
    assert_equal(opt.format, "DXF")
  
    opt.cif_blank_separator = true
    assert_equal(opt.cif_blank_separator?, true)
    opt.cif_blank_separator = false
    assert_equal(opt.cif_blank_separator?, false)

    opt.cif_dummy_calls = true
    assert_equal(opt.cif_dummy_calls?, true)
    opt.cif_dummy_calls = false
    assert_equal(opt.cif_dummy_calls?, false)

    opt.dxf_polygon_mode = 2
    assert_equal(opt.dxf_polygon_mode, 2)

    opt.gds2_libname = "MYLIB"
    assert_equal(opt.gds2_libname, "MYLIB")

    opt.gds2_max_cellname_length = 42
    assert_equal(opt.gds2_max_cellname_length, 42)

    opt.gds2_user_units = 0.75
    assert_equal(opt.gds2_user_units, 0.75)

    opt.gds2_max_vertex_count = 4242
    assert_equal(opt.gds2_max_vertex_count, 4242)

    opt.gds2_multi_xy_records = true
    assert_equal(opt.gds2_multi_xy_records, true)
    opt.gds2_multi_xy_records = false
    assert_equal(opt.gds2_multi_xy_records, false)

    opt.gds2_no_zero_length_paths = true
    assert_equal(opt.gds2_no_zero_length_paths?, true)
    opt.gds2_no_zero_length_paths = false
    assert_equal(opt.gds2_no_zero_length_paths?, false)

    opt.gds2_write_cell_properties = true
    assert_equal(opt.gds2_write_cell_properties?, true)
    opt.gds2_write_cell_properties = false
    assert_equal(opt.gds2_write_cell_properties?, false)

    opt.gds2_write_file_properties = true
    assert_equal(opt.gds2_write_file_properties?, true)
    opt.gds2_write_file_properties = false
    assert_equal(opt.gds2_write_file_properties?, false)

    assert_equal(opt.gds2_default_text_size.inspect, "nil")
    opt.gds2_default_text_size = nil
    assert_equal(opt.gds2_default_text_size.inspect, "nil")
    opt.gds2_default_text_size = -1.0
    assert_equal(opt.gds2_default_text_size.inspect, "nil")
    opt.gds2_default_text_size = 1.0
    assert_equal(opt.gds2_default_text_size, 1.0)

    opt.gds2_write_timestamps = true
    assert_equal(opt.gds2_write_timestamps?, true)
    opt.gds2_write_timestamps = false
    assert_equal(opt.gds2_write_timestamps?, false)

    assert_equal(opt.dxf_polygon_mode, 2)

    opt.oasis_compression_level = 5
    assert_equal(opt.oasis_compression_level, 5)

    opt.oasis_permissive = true
    assert_equal(opt.oasis_permissive?, true)
    opt.oasis_permissive = false
    assert_equal(opt.oasis_permissive?, false)

    opt.oasis_recompress = true
    assert_equal(opt.oasis_recompress?, true)
    opt.oasis_recompress = false
    assert_equal(opt.oasis_recompress?, false)

    opt.oasis_strict_mode = true
    assert_equal(opt.oasis_strict_mode?, true)
    opt.oasis_strict_mode = false
    assert_equal(opt.oasis_strict_mode?, false)

    opt.oasis_write_cblocks = true
    assert_equal(opt.oasis_write_cblocks?, true)
    opt.oasis_write_cblocks = false
    assert_equal(opt.oasis_write_cblocks?, false)

    opt.oasis_write_cell_bounding_boxes = true
    assert_equal(opt.oasis_write_cell_bounding_boxes?, true)
    opt.oasis_write_cell_bounding_boxes = false
    assert_equal(opt.oasis_write_cell_bounding_boxes?, false)

    opt.oasis_write_std_properties = true
    assert_equal(opt.oasis_write_std_properties?, true)
    opt.oasis_write_std_properties = false
    assert_equal(opt.oasis_write_std_properties?, false)

    opt.oasis_substitution_char = "+"
    assert_equal(opt.oasis_substitution_char, "+")

    opt.mag_lambda = 0.25
    assert_equal(opt.mag_lambda, 0.25)
    
    assert_equal(opt.mag_write_timestamp?, true)
    opt.mag_write_timestamp = false
    assert_equal(opt.mag_write_timestamp?, false)
    
    opt.mag_tech = "xyz"
    assert_equal(opt.mag_tech, "xyz")
    
    opt.set_format_from_filename("a.gds")
    assert_equal(opt.format, "GDS2")

  end

end

load("test_epilogue.rb")



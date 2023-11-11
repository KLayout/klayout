
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "laybasicConfig.h"
#include "layConverters.h"
#include "layColorPalette.h"
#include "layStipplePalette.h"
#include "layLineStylePalette.h"
#include "layPlugin.h"
#include "tlColor.h"

namespace lay
{

// ------------------------------------------------------------
//  The dummy plugin declaration to register the configuration options

class LayoutViewBasicConfigDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    lay::ColorConverter cc;
    options.push_back (std::pair<std::string, std::string> (cfg_default_lyp_file, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_default_add_other_layers, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_layers_always_show_source, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_layers_always_show_ld, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_layers_always_show_layout_index, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_test_shapes_in_view, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_flat_cell_list, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_split_cell_list, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_cell_list_sorting, "by-name"));
    options.push_back (std::pair<std::string, std::string> (cfg_split_lib_views, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_current_lib_view, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_hide_empty_layers, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_min_inst_label_size, "16"));
    options.push_back (std::pair<std::string, std::string> (cfg_cell_box_text_font, "0"));
    options.push_back (std::pair<std::string, std::string> (cfg_cell_box_text_transform, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_cell_box_color, "auto"));
    options.push_back (std::pair<std::string, std::string> (cfg_cell_box_visible, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_text_color, "auto"));
    options.push_back (std::pair<std::string, std::string> (cfg_text_visible, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_text_lazy_rendering, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_bitmap_caching, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_properties, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_apply_text_trans, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_global_trans, "r0"));
    options.push_back (std::pair<std::string, std::string> (cfg_default_text_size, "0.1"));
    options.push_back (std::pair<std::string, std::string> (cfg_text_point_mode, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_text_font, "0"));
    options.push_back (std::pair<std::string, std::string> (cfg_sel_color, cc.to_string (tl::Color ())));
    options.push_back (std::pair<std::string, std::string> (cfg_sel_line_width, "1"));
    options.push_back (std::pair<std::string, std::string> (cfg_sel_vertex_size, "3"));
    options.push_back (std::pair<std::string, std::string> (cfg_sel_dither_pattern, "1"));
    options.push_back (std::pair<std::string, std::string> (cfg_sel_line_style, "0"));
    options.push_back (std::pair<std::string, std::string> (cfg_sel_halo, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_sel_transient_mode, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_sel_inside_pcells_mode, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_tracking_cursor_enabled, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_tracking_cursor_color, cc.to_string (tl::Color ())));
    options.push_back (std::pair<std::string, std::string> (cfg_crosshair_cursor_color, cc.to_string (tl::Color ())));
    options.push_back (std::pair<std::string, std::string> (cfg_crosshair_cursor_line_style, "0"));
    options.push_back (std::pair<std::string, std::string> (cfg_crosshair_cursor_enabled, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_background_color, cc.to_string (tl::Color ())));
    options.push_back (std::pair<std::string, std::string> (cfg_ctx_color, cc.to_string (tl::Color ())));
    options.push_back (std::pair<std::string, std::string> (cfg_ctx_dimming, "50"));
    options.push_back (std::pair<std::string, std::string> (cfg_ctx_hollow, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_child_ctx_color, cc.to_string (tl::Color ())));
    options.push_back (std::pair<std::string, std::string> (cfg_child_ctx_dimming, "50"));
    options.push_back (std::pair<std::string, std::string> (cfg_child_ctx_hollow, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_child_ctx_enabled, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_search_range, "5"));
    options.push_back (std::pair<std::string, std::string> (cfg_search_range_box, "0"));
    options.push_back (std::pair<std::string, std::string> (cfg_abstract_mode_width, "10.0"));
    options.push_back (std::pair<std::string, std::string> (cfg_abstract_mode_enabled, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_fit_new_cell, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_full_hier_new_cell, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_initial_hier_depth, "1"));
    options.push_back (std::pair<std::string, std::string> (cfg_clear_ruler_new_cell, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_mouse_wheel_mode, "0"));
    options.push_back (std::pair<std::string, std::string> (cfg_pan_distance, "0.15"));
    options.push_back (std::pair<std::string, std::string> (cfg_paste_display_mode, "2"));
    options.push_back (std::pair<std::string, std::string> (cfg_guiding_shape_visible, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_guiding_shape_line_width, "1"));
    options.push_back (std::pair<std::string, std::string> (cfg_guiding_shape_color, cc.to_string (tl::Color ())));
    options.push_back (std::pair<std::string, std::string> (cfg_guiding_shape_vertex_size, "5"));
    options.push_back (std::pair<std::string, std::string> (cfg_abs_units, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_dbu_units, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_drawing_workers, "1"));
    options.push_back (std::pair<std::string, std::string> (cfg_drop_small_cells, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_drop_small_cells_cond, "0"));
    options.push_back (std::pair<std::string, std::string> (cfg_drop_small_cells_value, "10"));
    options.push_back (std::pair<std::string, std::string> (cfg_array_border_instances, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_bitmap_oversampling, "1"));
    options.push_back (std::pair<std::string, std::string> (cfg_highres_mode, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_image_cache_size, "1"));
    options.push_back (std::pair<std::string, std::string> (cfg_default_font_size, "0"));
    options.push_back (std::pair<std::string, std::string> (cfg_color_palette, lay::ColorPalette ().to_string ()));
    options.push_back (std::pair<std::string, std::string> (cfg_stipple_palette, lay::StipplePalette ().to_string ()));
    options.push_back (std::pair<std::string, std::string> (cfg_stipple_offset, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_line_style_palette, lay::LineStylePalette ().to_string ()));
    options.push_back (std::pair<std::string, std::string> (cfg_no_stipple, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_markers_visible, "true"));
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new LayoutViewBasicConfigDeclaration (), 1990, "LayoutViewBasicConfig");

}

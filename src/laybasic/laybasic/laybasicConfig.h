
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

#ifndef HDR_laybasicConfig
#define HDR_laybasicConfig

#include "laybasicCommon.h"

#include <string>

namespace lay
{

/** 
 *  @brief Declaration of the configuration names
 */

static const std::string cfg_grid ("grid-micron");

static const std::string cfg_grid_color ("grid-color");
static const std::string cfg_grid_ruler_color ("grid-ruler-color");
static const std::string cfg_grid_axis_color ("grid-axis-color");
static const std::string cfg_grid_grid_color ("grid-grid-color");
static const std::string cfg_grid_style0 ("grid-style0");
static const std::string cfg_grid_style1 ("grid-style1");
static const std::string cfg_grid_style2 ("grid-style2");
static const std::string cfg_grid_visible ("grid-visible");
static const std::string cfg_grid_micron ("grid-micron");
static const std::string cfg_grid_show_ruler ("grid-show-ruler");

static const std::string cfg_initial_technology ("initial-technology");

static const std::string cfg_background_color ("background-color");

static const std::string cfg_ctx_color ("context-color");
static const std::string cfg_ctx_dimming ("context-dimming");
static const std::string cfg_ctx_hollow ("context-hollow");

static const std::string cfg_child_ctx_color ("child-context-color");
static const std::string cfg_child_ctx_dimming ("child-context-dimming");
static const std::string cfg_child_ctx_hollow ("child-context-hollow");
static const std::string cfg_child_ctx_enabled ("child-context-enabled");

static const std::string cfg_search_range ("search-range");
static const std::string cfg_search_range_box ("search-range-box");

static const std::string cfg_abstract_mode_enabled ("abstract-mode-enabled");
static const std::string cfg_abstract_mode_width ("abstract-mode-width");

static const std::string cfg_sel_color ("sel-color");
static const std::string cfg_sel_line_width ("sel-line-width");
static const std::string cfg_sel_vertex_size ("sel-vertex-size");
static const std::string cfg_sel_halo ("sel-halo");
static const std::string cfg_sel_dither_pattern ("sel-dither-pattern");
static const std::string cfg_sel_line_style ("sel-line-style");
static const std::string cfg_sel_transient_mode ("sel-transient-mode");
static const std::string cfg_sel_inside_pcells_mode ("sel-inside-pcells-mode");

static const std::string cfg_tracking_cursor_color ("tracking-cursor-color");
static const std::string cfg_tracking_cursor_enabled ("tracking-cursor-enabled");
static const std::string cfg_crosshair_cursor_color ("crosshair-cursor-color");
static const std::string cfg_crosshair_cursor_line_style ("crosshair-cursor-line-style");
static const std::string cfg_crosshair_cursor_enabled ("crosshair-cursor-enabled");

static const std::string cfg_markers_visible ("markers-visible");

static const std::string cfg_min_inst_label_size ("min-inst-label-size");
static const std::string cfg_cell_box_text_font ("inst-label-font");
static const std::string cfg_cell_box_text_transform ("inst-label-transform");
static const std::string cfg_cell_box_color ("inst-color");
static const std::string cfg_cell_box_visible ("inst-visible");
static const std::string cfg_text_color ("text-color");
static const std::string cfg_text_visible ("text-visible");
static const std::string cfg_text_lazy_rendering ("text-lazy-rendering");
static const std::string cfg_bitmap_caching ("bitmap-caching");
static const std::string cfg_show_properties ("show-properties");
static const std::string cfg_apply_text_trans ("apply-text-trans");
static const std::string cfg_global_trans ("global-trans");
static const std::string cfg_no_stipple ("no-stipple");
static const std::string cfg_stipple_offset ("stipple-offset");
static const std::string cfg_default_text_size ("default-text-size");
static const std::string cfg_text_point_mode ("text-point-mode");
static const std::string cfg_text_font ("text-font");
static const std::string cfg_full_hier_new_cell ("full-hierarchy-new-cell");
static const std::string cfg_initial_hier_depth ("initial-hier-depth");
static const std::string cfg_clear_ruler_new_cell ("clear-ruler-new-cell");
static const std::string cfg_fit_new_cell ("fit-new-cell");
static const std::string cfg_mouse_wheel_mode ("mouse-wheel-mode");
static const std::string cfg_color_palette ("color-palette");
static const std::string cfg_stipple_palette ("stipple-palette");
static const std::string cfg_line_style_palette ("line-style-palette");
static const std::string cfg_dbu_units ("dbu-units");
static const std::string cfg_abs_units ("absolute-units");
static const std::string cfg_drawing_workers ("drawing-workers");
static const std::string cfg_drop_small_cells ("drop-small-cells");
static const std::string cfg_drop_small_cells_cond ("drop-small-cells-condition");
static const std::string cfg_drop_small_cells_value ("drop-small-cells-value");
static const std::string cfg_array_border_instances ("draw-array-border-instances");
static const std::string cfg_default_lyp_file ("default-layer-properties");
static const std::string cfg_default_add_other_layers ("default-add-other-layers");
static const std::string cfg_layers_always_show_source ("layers-always-show-source");
static const std::string cfg_layers_always_show_ld ("layers-always-show-ld");
static const std::string cfg_layers_always_show_layout_index ("layers-always-show-layout-index");
static const std::string cfg_reader_options_show_always ("reader-options-show-always");
static const std::string cfg_tip_window_hidden ("tip-window-hidden");

static const std::string cfg_bitmap_oversampling ("bitmap-oversampling");
static const std::string cfg_highres_mode ("highres-mode");
static const std::string cfg_image_cache_size ("image-cache-size");
static const std::string cfg_default_font_size ("default-font-size");

static const std::string cfg_hide_empty_layers ("hide-empty-layers");
static const std::string cfg_test_shapes_in_view ("test-shapes-in-view");

static const std::string cfg_flat_cell_list ("flat-cell-list");
static const std::string cfg_split_cell_list ("split-cell-list");
static const std::string cfg_cell_list_sorting ("cell-list-sorting");

static const std::string cfg_split_lib_views ("split-lib-views");
static const std::string cfg_current_lib_view ("current-lib-view");

static const std::string cfg_bookmarks_follow_selection ("bookmarks-follow-selection");

static const std::string cfg_pan_distance ("pan-distance");
static const std::string cfg_paste_display_mode ("paste-display-mode");

static const std::string cfg_guiding_shape_visible ("guiding-shape-visible");
static const std::string cfg_guiding_shape_color ("guiding-shape-color");
static const std::string cfg_guiding_shape_line_width ("guiding-shape-line-width");
static const std::string cfg_guiding_shape_vertex_size ("guiding-shape-vertex-size");

}

#endif

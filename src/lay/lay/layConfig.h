
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#ifndef HDR_layConfig
#define HDR_layConfig

#include "layCommon.h"

#include <string>

namespace lay
{

/**
 *  @brief Declaration of the configuration names
 */

static const std::string cfg_default_grids ("default-grids");
static const std::string cfg_circle_points ("circle-points");
static const std::string cfg_synchronized_views ("synchronized-views");
static const std::string cfg_edit_mode ("edit-mode");
static const std::string cfg_custom_macro_paths ("custom-macro-paths");
static const std::string cfg_mru ("mru");
static const std::string cfg_mru_layer_properties ("mru-layer-properties");
static const std::string cfg_mru_sessions ("mru-sessions");
static const std::string cfg_mru_bookmarks ("mru-bookmarks");
static const std::string cfg_keep_backups ("keep-backups");
static const std::string cfg_technologies ("technology-data");
static const std::string cfg_key_bindings ("key-bindings");
static const std::string cfg_menu_items_hidden ("menu-items-hidden");
static const std::string cfg_show_toolbar ("show-toolbar");
static const std::string cfg_show_navigator ("show-navigator");
static const std::string cfg_navigator_all_hier_levels ("navigator-show-all-hier-levels");
static const std::string cfg_navigator_show_images ("navigator-show-images");
static const std::string cfg_show_layer_toolbox ("show-layer-toolbox");
static const std::string cfg_show_hierarchy_panel ("show-hierarchy-panel");
static const std::string cfg_show_libraries_view ("show-libraries-view");
static const std::string cfg_show_bookmarks_view ("show-bookmarks-view");
static const std::string cfg_show_layer_panel ("show-layer-panel");
static const std::string cfg_window_state ("window-state");
static const std::string cfg_layout_file_watcher_enabled ("layout-file-watcher-enabled");
static const std::string cfg_window_geometry ("window-geometry");
static const std::string cfg_micron_digits ("digits-micron");
static const std::string cfg_dbu_digits ("digits-dbu");
static const std::string cfg_assistant_bookmarks ("assistant-bookmarks");
static const std::string cfg_always_exit_without_saving ("always-exit-without-saving");

}

#endif

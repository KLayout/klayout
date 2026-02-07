
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


#include "edtConfig.h"
#include "tlString.h"
#include "tlInternational.h"

namespace edt
{

// -----------------------------------------------------------------------------

std::string cfg_edit_grid ("edit-grid");
std::string cfg_edit_snap_to_objects ("edit-snap-to-objects");
std::string cfg_edit_snap_objects_to_grid ("edit-snap-objects-to-grid");
std::string cfg_edit_move_angle_mode ("edit-move-angle-mode");
std::string cfg_edit_connect_angle_mode ("edit-connect-angle-mode");
std::string cfg_edit_text_string ("edit-text-string");
std::string cfg_edit_text_size ("edit-text-size");
std::string cfg_edit_text_halign ("edit-text-halign");
std::string cfg_edit_text_valign ("edit-text-valign");
std::string cfg_edit_path_width ("edit-path-width");
std::string cfg_edit_path_ext_type ("edit-path-ext-type");
std::string cfg_edit_path_ext_var_begin ("edit-path-ext-var-begin");
std::string cfg_edit_path_ext_var_end ("edit-path-ext-var-end");
std::string cfg_edit_inst_cell_name ("edit-inst-cell-name");
std::string cfg_edit_inst_lib_name ("edit-inst-lib-name");
std::string cfg_edit_inst_pcell_parameters ("edit-inst-pcell-parameters");
std::string cfg_edit_inst_angle ("edit-inst-angle");
std::string cfg_edit_inst_mirror ("edit-inst-mirror");
std::string cfg_edit_inst_scale ("edit-inst-scale");
std::string cfg_edit_inst_array ("edit-inst-array");
std::string cfg_edit_inst_rows ("edit-inst-rows");
std::string cfg_edit_inst_row_x ("edit-inst-row_x");
std::string cfg_edit_inst_row_y ("edit-inst-row_y");
std::string cfg_edit_inst_columns ("edit-inst-columns");
std::string cfg_edit_inst_column_x ("edit-inst-column_x");
std::string cfg_edit_inst_column_y ("edit-inst-column_y");
std::string cfg_edit_inst_place_origin ("edit-inst-place-origin");
std::string cfg_edit_top_level_selection ("edit-top-level-selection");
std::string cfg_edit_hier_copy_mode ("edit-hier-copy-mode");
std::string cfg_edit_show_shapes_of_instances ("edit-show-shapes-of-instances");
std::string cfg_edit_max_shapes_of_instances ("edit-max-shapes-of-instances");
std::string cfg_edit_pcell_show_parameter_names ("edit-pcell-show-parameter-names");
std::string cfg_edit_pcell_lazy_eval_mode ("edit-pcell-lazy-eval-mode");
std::string cfg_edit_global_grid ("grid-micron");
std::string cfg_edit_combine_mode ("combine-mode");

// -----------------------------------------------------------------------------
//  CMConverter implementation

std::string 
CMConverter::to_string (const edt::combine_mode_type &m)
{
  if (m == edt::CM_Add) {
    return "add";
  } else if (m == edt::CM_Merge) {
    return "merge";
  } else if (m == edt::CM_Erase) {
    return "erase";
  } else if (m == edt::CM_Mask) {
    return "mask";
  } else if (m == edt::CM_Diff) {
    return "diff";
  } else {
    return "";
  }
}

void 
CMConverter::from_string (const std::string &s, edt::combine_mode_type &m)
{
  std::string t (tl::trim (s));
  if (t == "add") {
    m = edt::CM_Add;
  } else if (t == "merge") {
    m = edt::CM_Merge;
  } else if (t == "erase") {
    m = edt::CM_Erase;
  } else if (t == "mask") {
    m = edt::CM_Mask;
  } else if (t == "diff") {
    m = edt::CM_Diff;
  } else {
    m = edt::CM_Add;
  }
}

// -----------------------------------------------------------------------------
//  PathExtConverter implementation

std::string 
PathExtConverter::to_string (const edt::path_ext_type &m)
{
  if (m == edt::Flush) {
    return "flush";
  } else if (m == edt::Square) {
    return "square";
  } else if (m == edt::Variable) {
    return "variable";
  } else if (m == edt::Round) {
    return "round";
  } else {
    return "";
  }
}

void 
PathExtConverter::from_string (const std::string &tt, edt::path_ext_type &m)
{
  std::string t (tl::trim (tt));
  if (t == "flush") {
    m = edt::Flush;
  } else if (t == "square") {
    m = edt::Square;
  } else if (t == "variable") {
    m = edt::Variable;
  } else if (t == "round") {
    m = edt::Round;
  } else {
    m = edt::Flush;
  }
}

}


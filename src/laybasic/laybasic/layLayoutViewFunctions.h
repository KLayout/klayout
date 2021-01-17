
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#ifndef HDR_layLayoutViewFunctions
#define HDR_layLayoutViewFunctions

#include "laybasicCommon.h"

#include "layPlugin.h"
#include "layDialogs.h"   //  For AlignCellOptions

#include "dbTrans.h"
#include "dbLayerProperties.h"

namespace lay {

class LayoutView;

/**
 *  @brief The layout view's functions implementation
 */
class LAYBASIC_PUBLIC LayoutViewFunctions
  : public lay::Plugin
{
public:
  /**
   *  @brief Constructor
   */
  LayoutViewFunctions (db::Manager *manager, lay::LayoutView *view);

  /** 
   *  @brief Destructor
   */
  ~LayoutViewFunctions ();

  //  Plugin interface implementation
  void menu_activated (const std::string &symbol);

  //  menu callbacks
  void cm_new_layer ();
  void cm_clear_layer ();
  void cm_delete_layer ();
  void cm_copy_layer ();
  void cm_align_cell_origin ();
  void cm_edit_layer ();
  void cm_lay_convert_to_static ();
  void cm_lay_flip_x ();
  void cm_lay_flip_y ();
  void cm_lay_rot_cw ();
  void cm_lay_rot_ccw ();
  void cm_lay_free_rot ();
  void cm_lay_scale ();
  void cm_lay_move ();
  void cm_sel_flip_x ();
  void cm_sel_flip_y ();
  void cm_sel_rot_cw ();
  void cm_sel_rot_ccw ();
  void cm_sel_free_rot ();
  void cm_sel_scale ();
  void cm_sel_move ();
  void cm_sel_move_to ();
  void cm_sel_move_interactive ();

  //  forwarded to the layer control panel
  void cm_new_tab ();
  void cm_rename_tab ();
  void cm_remove_tab ();
  void cm_select_all ();
  void cm_make_valid ();
  void cm_make_invalid ();
  void cm_hide ();
  void cm_hide_all ();
  void cm_show ();
  void cm_show_all ();
  void cm_show_only ();
  void cm_rename ();
  void cm_delete ();
  void cm_insert ();
  void cm_group ();
  void cm_ungroup ();
  void cm_source ();
  void cm_sort_by_name ();
  void cm_sort_by_ild ();
  void cm_sort_by_idl ();
  void cm_sort_by_ldi ();
  void cm_sort_by_dli ();
  void cm_regroup_by_index ();
  void cm_regroup_by_datatype ();
  void cm_regroup_by_layer ();
  void cm_regroup_flatten ();
  void cm_expand_all ();
  void cm_add_missing ();
  void cm_remove_unused ();
  void cm_layer_copy ();
  void cm_layer_cut ();
  void cm_layer_paste ();

  //  forwarded to the cell control panel
  void cm_cell_user_properties ();
  void cm_cell_flatten ();
  void cm_cell_rename ();
  void cm_cell_replace ();
  void cm_cell_delete ();
  void cm_cell_select ();
  void cm_open_current_cell ();
  void cm_cell_hide ();
  void cm_cell_show ();
  void cm_cell_show_all ();
  void cm_cell_copy ();
  void cm_cell_cut ();
  void cm_cell_paste ();
  void cm_cell_convert_to_static ();

protected:
  lay::LayoutView *view ()
  {
    return mp_view;
  }

  db::Manager *manager ()
  {
    return mp_manager;
  }

  void do_cm_duplicate (bool interactive);
  void do_cm_paste (bool interactive);
  void cm_new_cell ();
  void cm_reload ();

  void do_transform (const db::DCplxTrans &tr);
  void transform_layout (const db::DCplxTrans &tr);

private:
  lay::LayoutView *mp_view;
  db::Manager *mp_manager;
  db::LayerProperties m_new_layer_props;
  db::DVector m_move_dist;
  int m_move_to_origin_mode_x, m_move_to_origin_mode_y;
  lay::AlignCellOptions m_align_cell_options;
  int m_del_cell_mode;
  int m_layer_hier_mode;
  int m_duplicate_hier_mode;
  bool m_clear_before;
  int m_copy_cva, m_copy_cvr;
  int m_copy_layera, m_copy_layerr;
};

}

#endif


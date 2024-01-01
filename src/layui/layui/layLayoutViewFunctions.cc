
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#if defined(HAVE_QT)

#include "layLayoutViewFunctions.h"
#include "layLayoutViewBase.h"
#include "layCellSelectionForm.h"
#include "layLayoutStatisticsForm.h"
#include "layLayoutPropertiesForm.h"
#include "layHierarchyControlPanel.h"
#include "layLayerControlPanel.h"
#include "layTipDialog.h"
#include "laySelectCellViewForm.h"
#include "layMove.h"
#include "laybasicConfig.h"

#include "dbClipboard.h"
#include "dbRecursiveShapeIterator.h"
#include "dbLayoutUtils.h"
#include "dbPCellDeclaration.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QMainWindow>

namespace lay
{

/**
 *  @brief Gets a suitable parent widget for the modal dialogs used in this module
 */
static QWidget *parent_widget ()
{
  return QApplication::activeWindow ();
}

static void
collect_cells_to_delete (const db::Layout &layout, const db::Cell &cell, std::set<db::cell_index_type> &called)
{
  //  don't delete proxies - they are deleted later when the layout is cleaned
  for (db::Cell::child_cell_iterator cc = cell.begin_child_cells (); ! cc.at_end (); ++cc) {
    if (called.find (*cc) == called.end () && !layout.cell (*cc).is_proxy ()) {
      called.insert (*cc);
      collect_cells_to_delete (layout, layout.cell (*cc), called);
    }
  }
}

static bool
validate_cell_path (const db::Layout &layout, lay::LayoutViewBase::cell_path_type &path)
{
  for (size_t i = 0; i < path.size (); ++i) {

    if (! layout.is_valid_cell_index (path [i])) {

      if (layout.is_valid_cell_index (path.back ())) {
        //  use a stub path
        path.erase (path.begin (), --path.end ());
      } else {
        //  strip everything that is not valid
        path.erase (path.begin () + i, path.end ());
      }

      return true;

    }
  }

  return false;
}

LayoutViewFunctions::LayoutViewFunctions (db::Manager *manager, LayoutViewBase *view)
  : lay::Plugin (view), mp_view (view), mp_manager (manager)
{
  m_del_cell_mode = 0;
  m_move_to_origin_mode_x = 0;
  m_move_to_origin_mode_y = 0;
  m_del_cell_mode = 0;
  m_layer_hier_mode = 0;
  m_duplicate_hier_mode = 2;
  m_clear_before = true;
  m_copy_cva = -1;
  m_copy_cvr = -1;
  m_copy_layera = -1;
  m_copy_layerr = -1;

  m_new_layer_props.layer = 1;
  m_new_layer_props.datatype = 0;
}

LayoutViewFunctions::~LayoutViewFunctions ()
{
  //  .. nothing yet..
}

void
LayoutViewFunctions::menu_activated (const std::string &symbol)
{
  if (! view ()) {
    return;
  }

  if (symbol == "cm_show_properties") {

    view ()->show_properties ();

  } else if (symbol == "cm_delete") {

    view ()->del ();
    //  because a "delete" might involve objects currently edited, we cancel the edit after we have deleted the object
    view ()->cancel ();
    view ()->clear_selection ();

  } else if (symbol == "cm_unselect_all") {
    view ()->select (db::DBox (), lay::Editable::Reset);
  } else if (symbol == "cm_select_all") {
    view ()->select (view ()->full_box (), lay::Editable::Replace);
  } else if (symbol == "cm_select_next_item") {
    view ()->repeat_selection (lay::Editable::Replace);
  } else if (symbol == "cm_select_next_item_add") {
    view ()->repeat_selection (lay::Editable::Add);
  } else if (symbol == "cm_lv_paste") {
    cm_layer_paste ();
  } else if (symbol == "cm_lv_cut") {
    cm_layer_cut ();
  } else if (symbol == "cm_lv_copy") {
    cm_layer_copy ();
  } else if (symbol == "cm_cell_paste") {
    cm_cell_paste ();
  } else if (symbol == "cm_cell_cut") {
    cm_cell_cut ();
  } else if (symbol == "cm_cell_copy") {
    cm_cell_copy ();
  } else if (symbol == "cm_duplicate") {
    do_cm_duplicate (false);
  } else if (symbol == "cm_duplicate_interactive") {
    do_cm_duplicate (true);
  } else if (symbol == "cm_copy") {

    view ()->copy ();
    view ()->clear_selection ();

  } else if (symbol == "cm_paste") {
    do_cm_paste (false);
  } else if (symbol == "cm_paste_interactive") {
    do_cm_paste (true);
  } else if (symbol == "cm_cut") {

    view ()->cut ();
    view ()->cancel (); //  see del() for reason why cancel is after cut
    view ()->clear_selection ();

  } else if (symbol == "cm_zoom_fit_sel") {
    view ()->zoom_fit_sel ();
  } else if (symbol == "cm_zoom_fit") {
    view ()->zoom_fit ();
  } else if (symbol == "cm_pan_left") {
    view ()->pan_left ();
  } else if (symbol == "cm_pan_right") {
    view ()->pan_right ();
  } else if (symbol == "cm_pan_up") {
    view ()->pan_up ();
  } else if (symbol == "cm_pan_down") {
    view ()->pan_down ();
  } else if (symbol == "cm_zoom_in") {
    view ()->zoom_in ();
  } else if (symbol == "cm_zoom_out") {
    view ()->zoom_out ();
  } else if (symbol == "cm_select_current_cell") {

    if (view ()->active_cellview_index () >= 0) {
      lay::LayoutViewBase::cell_path_type path;
      int cvi = view ()->active_cellview_index ();
      view ()->current_cell_path (path);
      view ()->select_cell_fit (path, cvi);
    }

  } else if (symbol == "cm_open_current_cell") {

    if (view ()->active_cellview_index () >= 0) {
      cm_open_current_cell ();
    }

  } else if (symbol == "cm_select_cell") {

    if (view ()->active_cellview_index () >= 0) {

      lay::CellSelectionForm form (0, view (), "cell_selection_form");

      if (form.exec () == QDialog::Accepted &&
          form.selected_cellview_index () >= 0) {
        view ()->select_cell (form.selected_cellview ().combined_unspecific_path (), form.selected_cellview_index ());
        view ()->set_current_cell_path (form.selected_cellview_index (), form.selected_cellview ().combined_unspecific_path ());
        view ()->zoom_fit ();
      }

    }

  } else if (symbol == "cm_new_cell") {
    cm_new_cell ();
  } else if (symbol == "cm_adjust_origin") {
    if (view ()->active_cellview_index () >= 0) {
      cm_align_cell_origin ();
    }
  } else if (symbol == "cm_cell_convert_to_static") {
    if (view ()->active_cellview_index () >= 0) {
      cm_cell_convert_to_static ();
    }
  } else if (symbol == "cm_lay_convert_to_static") {
    if (view ()->active_cellview_index () >= 0) {
      cm_lay_convert_to_static ();
    }
  } else if (symbol == "cm_lay_move") {
    if (view ()->active_cellview_index () >= 0) {
      cm_lay_move ();
    }
  } else if (symbol == "cm_lay_scale") {
    if (view ()->active_cellview_index () >= 0) {
      cm_lay_scale ();
    }
  } else if (symbol == "cm_lay_free_rot") {
    if (view ()->active_cellview_index () >= 0) {
      cm_lay_free_rot ();
    }
  } else if (symbol == "cm_lay_rot_ccw") {
    if (view ()->active_cellview_index () >= 0) {
      cm_lay_rot_ccw ();
    }
  } else if (symbol == "cm_lay_rot_cw") {
    if (view ()->active_cellview_index () >= 0) {
      cm_lay_rot_cw ();
    }
  } else if (symbol == "cm_lay_flip_y") {
    if (view ()->active_cellview_index () >= 0) {
      cm_lay_flip_y ();
    }
  } else if (symbol == "cm_lay_flip_x") {
    if (view ()->active_cellview_index () >= 0) {
      cm_lay_flip_x ();
    }
  } else if (symbol == "cm_sel_move") {
    if (view ()->active_cellview_index () >= 0) {
      cm_sel_move ();
    }
  } else if (symbol == "cm_sel_move_to") {
    if (view ()->active_cellview_index () >= 0) {
      cm_sel_move_to ();
    }
  } else if (symbol == "cm_sel_move_interactive") {
    if (view ()->active_cellview_index () >= 0) {
      cm_sel_move_interactive ();
    }
  } else if (symbol == "cm_sel_scale") {
    if (view ()->active_cellview_index () >= 0) {
      cm_sel_scale ();
    }
  } else if (symbol == "cm_sel_free_rot") {
    if (view ()->active_cellview_index () >= 0) {
      cm_sel_free_rot ();
    }
  } else if (symbol == "cm_sel_rot_ccw") {
    if (view ()->active_cellview_index () >= 0) {
      cm_sel_rot_ccw ();
    }
  } else if (symbol == "cm_sel_rot_cw") {
    if (view ()->active_cellview_index () >= 0) {
      cm_sel_rot_cw ();
    }
  } else if (symbol == "cm_sel_flip_y") {
    if (view ()->active_cellview_index () >= 0) {
      cm_sel_flip_y ();
    }
  } else if (symbol == "cm_sel_flip_x") {
    if (view ()->active_cellview_index () >= 0) {
      cm_sel_flip_x ();
    }
  } else if (symbol == "cm_edit_layer") {
    if (view ()->active_cellview_index () >= 0) {
      cm_edit_layer ();
    }
  } else if (symbol == "cm_delete_layer") {
    if (view ()->active_cellview_index () >= 0) {
      cm_delete_layer ();
    }
  } else if (symbol == "cm_clear_layer") {
    if (view ()->active_cellview_index () >= 0) {
      cm_clear_layer ();
    }
  } else if (symbol == "cm_copy_layer") {
    if (view ()->active_cellview_index () >= 0) {
      cm_copy_layer ();
    }
  } else if (symbol == "cm_new_layer") {
    if (view ()->active_cellview_index () >= 0) {
      cm_new_layer ();
    }
  } else if (symbol == "cm_layout_props") {
    lay::LayoutPropertiesForm lp_form (parent_widget (), view (), "layout_props_form");
    lp_form.exec ();
  } else if (symbol == "cm_layout_stats") {
    lay::LayoutStatisticsForm lp_form (parent_widget (), view (), "layout_props_form");
    lp_form.exec ();
  } else if (symbol == "cm_reload") {
    cm_reload ();
  } else if (symbol == "cm_inc_max_hier") {
    int new_to = view ()->get_max_hier_levels () + 1;
    view ()->set_hier_levels (std::make_pair (view ()->get_min_hier_levels (), new_to));
  } else if (symbol == "cm_dec_max_hier") {
    int new_to = view ()->get_max_hier_levels () > 0 ? view ()->get_max_hier_levels () - 1 : 0;
    view ()->set_hier_levels (std::make_pair (std::min (view ()->get_min_hier_levels (), new_to), new_to));
  } else if (symbol == "cm_max_hier") {
    view ()->max_hier ();
  } else if (symbol == "cm_max_hier_0") {
    view ()->set_hier_levels (std::make_pair (std::min (view ()->get_min_hier_levels (), 0), 0));
  } else if (symbol == "cm_max_hier_1") {
    view ()->set_hier_levels (std::make_pair (std::min (view ()->get_min_hier_levels (), 0), 1));
  } else if (symbol == "cm_prev_display_state") {
    if (view ()->has_prev_display_state ()) {
      view ()->prev_display_state ();
    }
  } else if (symbol == "cm_next_display_state") {
    if (view ()->has_next_display_state ()) {
      view ()->next_display_state ();
    }
  } else if (symbol == "cm_redraw") {
    view ()->redraw ();
  } else if (symbol == "cm_cell_delete") {
    cm_cell_delete ();
  } else if (symbol == "cm_cell_replace") {
    cm_cell_replace ();
  } else if (symbol == "cm_cell_rename") {
    cm_cell_rename ();
  } else if (symbol == "cm_cell_flatten") {
    cm_cell_flatten ();
  } else if (symbol == "cm_cell_select") {
    cm_cell_select ();
  } else if (symbol == "cm_cell_hide") {
    cm_cell_hide ();
  } else if (symbol == "cm_cell_show") {
    cm_cell_show ();
  } else if (symbol == "cm_cell_show_all") {
    cm_cell_show_all ();
  } else if (symbol == "cm_cell_user_properties") {
    if (view ()->active_cellview_index () >= 0) {
      cm_cell_user_properties ();
    }
  } else if (symbol == "cm_lv_select_all") {
    cm_select_all ();
  } else if (symbol == "cm_lv_invert_selection") {
    cm_invert_selection ();
  } else if (symbol == "cm_lv_new_tab") {
    cm_new_tab ();
  } else if (symbol == "cm_lv_rename_tab") {
    cm_rename_tab ();
  } else if (symbol == "cm_lv_make_invalid") {
    cm_make_invalid ();
  } else if (symbol == "cm_lv_remove_tab") {
    cm_remove_tab ();
  } else if (symbol == "cm_lv_make_valid") {
    cm_make_valid ();
  } else if (symbol == "cm_lv_hide_all") {
    cm_hide_all ();
  } else if (symbol == "cm_lv_hide") {
    cm_hide ();
  } else if (symbol == "cm_lv_show_only") {
    cm_show_only ();
  } else if (symbol == "cm_lv_show_all") {
    cm_show_all ();
  } else if (symbol == "cm_lv_show") {
    cm_show ();
  } else if (symbol == "cm_lv_toggle_visibility") {
    cm_toggle_visibility ();
  } else if (symbol == "cm_lv_rename") {
    cm_rename ();
  } else if (symbol == "cm_lv_delete") {
    cm_delete ();
  } else if (symbol == "cm_lv_insert") {
    cm_insert ();
  } else if (symbol == "cm_lv_group") {
    cm_group ();
  } else if (symbol == "cm_lv_ungroup") {
    cm_ungroup ();
  } else if (symbol == "cm_lv_source") {
    cm_source ();
  } else if (symbol == "cm_lv_sort_by_name") {
    cm_sort_by_name ();
  } else if (symbol == "cm_lv_sort_by_ild") {
    cm_sort_by_ild ();
  } else if (symbol == "cm_lv_sort_by_idl") {
    cm_sort_by_idl ();
  } else if (symbol == "cm_lv_sort_by_ldi") {
    cm_sort_by_ldi ();
  } else if (symbol == "cm_lv_sort_by_dli") {
    cm_sort_by_dli ();
  } else if (symbol == "cm_lv_regroup_by_index") {
    cm_regroup_by_index ();
  } else if (symbol == "cm_lv_regroup_by_datatype") {
    cm_regroup_by_datatype ();
  } else if (symbol == "cm_lv_regroup_by_layer") {
    cm_regroup_by_layer ();
  } else if (symbol == "cm_lv_regroup_flatten") {
    cm_regroup_flatten ();
  } else if (symbol == "cm_lv_expand_all") {
    cm_expand_all ();
  } else if (symbol == "cm_lv_add_missing") {
    cm_add_missing ();
  } else if (symbol == "cm_lv_remove_unused") {
    cm_remove_unused ();
  }
}

void
LayoutViewFunctions::cm_cell_user_properties ()
{
  int cv_index = view ()->active_cellview_index ();
  lay::LayoutViewBase::cell_path_type path;
  view ()->current_cell_path (cv_index, path);

  if (cv_index >= 0 && path.size () > 0) {

    db::Layout &layout = view ()->cellview (cv_index)->layout ();
    db::Cell &cell = layout.cell (path.back ());
    db::properties_id_type prop_id = cell.prop_id ();

    lay::UserPropertiesForm props_form (parent_widget ());
    if (props_form.show (view (), cv_index, prop_id, layout.begin_meta (cell.cell_index ()), layout.end_meta (cell.cell_index ()))) {

      view ()->transaction (tl::to_string (tr ("Edit cell's user properties")));
      cell.prop_id (prop_id);
      view ()->commit ();

    }

  }
}

void
LayoutViewFunctions::cm_cell_replace ()
{
  int cv_index = view ()->active_cellview_index ();
  std::vector<lay::LayoutViewBase::cell_path_type> paths;
  view ()->selected_cells_paths (cv_index, paths);

  if (cv_index >= 0 && paths.size () > 0) {

    if (paths.size () > 1) {
      throw tl::Exception (tl::to_string (tr ("Replace cell cannot be used when multiple cells are selected")));
    }

    db::Layout &layout = view ()->cellview (cv_index)->layout ();

    bool needs_to_ask = false;
    for (std::vector<lay::LayoutViewBase::cell_path_type>::const_iterator p = paths.begin (); p != paths.end () && ! needs_to_ask; ++p) {
      if (layout.is_valid_cell_index (p->back ()) && ! layout.cell (p->back ()).is_leaf ()) {
        needs_to_ask = true;
      }
    }


    lay::ReplaceCellOptionsDialog mode_dialog (parent_widget ());

    db::cell_index_type with_cell = paths.front ().back ();
    int mode = needs_to_ask ? m_del_cell_mode : 0;

    if (mode_dialog.exec_dialog (view ()->cellview (cv_index), mode, with_cell)) {

      if (needs_to_ask) {
        m_del_cell_mode = mode;
      }

      if (with_cell != paths.front ().back ()) {

        //  remember the current path
        lay::LayoutViewBase::cell_path_type cell_path (view ()->cellview (cv_index).combined_unspecific_path ());

        view ()->clear_selection ();

        view ()->transaction (tl::to_string (tr ("Replace cells")));

        //  replace instances of the target cell with the new cell 

        db::cell_index_type target_cell_index = paths.front ().back ();
        layout.replace_instances_of (target_cell_index, with_cell);

        std::set<db::cell_index_type> cells_to_delete;
        for (std::vector<lay::LayoutViewBase::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
          if (! p->empty () && layout.is_valid_cell_index (p->back ())) {
            cells_to_delete.insert (p->back ());
            if (mode == 2) {
              layout.cell (p->back ()).collect_called_cells (cells_to_delete);
            }
          }
        }

        //  support a propagation use case:
        std::set<db::cell_index_type> cells_below_replacement_cell;
        cells_below_replacement_cell.insert (with_cell);
        layout.cell (with_cell).collect_called_cells (cells_below_replacement_cell);
        for (std::set<db::cell_index_type>::const_iterator c = cells_below_replacement_cell.begin (); c != cells_below_replacement_cell.end (); ++c) {
          cells_to_delete.erase (*c);
        }

        if (mode == 0 || mode == 2) {
          layout.delete_cells (cells_to_delete);
        } else if (mode == 1) {
          layout.prune_cells (cells_to_delete);
        }

        layout.cleanup ();

        view ()->commit ();

        if (validate_cell_path (layout, cell_path)) {
          view ()->select_cell (cell_path, cv_index);
        }

      }

    }

  }
}

void
LayoutViewFunctions::cm_lay_convert_to_static ()
{
  //  end move operations, cancel edit operations
  view ()->cancel_edits ();
  view ()->clear_selection ();

  int cv_index = view ()->active_cellview_index ();
  if (cv_index >= 0) {

    db::Layout &layout = view ()->cellview (cv_index)->layout ();

    view ()->transaction (tl::to_string (tr ("Convert all cells to static")));

    std::vector<db::cell_index_type> cells;
    for (db::Layout::const_iterator c = layout.begin (); c != layout.end (); ++c) {
      cells.push_back (c->cell_index ());
    }

    std::map<db::cell_index_type, db::cell_index_type> cell_map;
    for (std::vector<db::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {
      if (layout.is_valid_cell_index (*c)) {
        db::cell_index_type new_cell = layout.convert_cell_to_static (*c);
        if (new_cell != *c) {
          cell_map.insert (std::make_pair (*c, new_cell));
        }
      }
    }

    //  rewrite instances
    for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
      for (db::Cell::const_iterator i = c->begin (); ! i.at_end (); ++i) {
        std::map<db::cell_index_type, db::cell_index_type>::const_iterator cm = cell_map.find (i->cell_index ());
        if (cm != cell_map.end ()) {
          db::CellInstArray ci = i->cell_inst ();
          ci.object ().cell_index (cm->second);
          c->replace (*i, ci);
        }
      }
    }

    layout.cleanup ();

    view ()->commit ();

  }
}

void
LayoutViewFunctions::cm_cell_convert_to_static ()
{
  int cv_index = view ()->active_cellview_index ();
  std::vector<lay::LayoutViewBase::cell_path_type> paths;
  view ()->selected_cells_paths (cv_index, paths);

  if (cv_index >= 0 && paths.size () > 0) {

    db::Layout &layout = view ()->cellview (cv_index)->layout ();

    //  remember the current path
    lay::LayoutViewBase::cell_path_type cell_path (view ()->cellview (cv_index).combined_unspecific_path ());

    view ()->clear_selection ();

    view ()->transaction (tl::to_string (tr ("Convert cells to static")));

    std::map<db::cell_index_type, db::cell_index_type> cell_map;

    for (std::vector<lay::LayoutViewBase::cell_path_type>::iterator p = paths.begin (); p != paths.end (); ++p) {
      if (! p->empty () && layout.is_valid_cell_index (p->back ())) {
        db::cell_index_type new_cell = layout.convert_cell_to_static (p->back ());
        if (new_cell != p->back ()) {
          cell_map.insert (std::make_pair (p->back (), new_cell));
          p->back () = new_cell;
        }
      }
    }

    //  rewrite instances
    for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
      for (db::Cell::const_iterator i = c->begin (); ! i.at_end (); ++i) {
        std::map<db::cell_index_type, db::cell_index_type>::const_iterator cm = cell_map.find (i->cell_index ());
        if (cm != cell_map.end ()) {
          db::CellInstArray ci = i->cell_inst ();
          ci.object ().cell_index (cm->second);
          c->replace (*i, ci);
        }
      }
    }

    layout.cleanup ();

    view ()->commit ();

    if (validate_cell_path (layout, cell_path)) {
      view ()->select_cell (cell_path, cv_index);
    }

  }
}

void
LayoutViewFunctions::cm_cell_delete ()
{
  int cv_index = view ()->active_cellview_index ();
  std::vector<lay::LayoutViewBase::cell_path_type> paths;
  view ()->selected_cells_paths (cv_index, paths);

  if (cv_index >= 0 && paths.size () > 0) {

    db::Layout &layout = view ()->cellview (cv_index)->layout ();

    bool needs_to_ask = false;
    for (std::vector<lay::LayoutViewBase::cell_path_type>::const_iterator p = paths.begin (); p != paths.end () && ! needs_to_ask; ++p) {
      if (layout.is_valid_cell_index (p->back ()) && ! layout.cell (p->back ()).is_leaf ()) {
        needs_to_ask = true;
      }
    }

    int mode = m_del_cell_mode;
    if (! needs_to_ask) {
      mode = 0;
    }

    lay::DeleteCellModeDialog mode_dialog (parent_widget ());
    if (! needs_to_ask || mode_dialog.exec_dialog (mode)) {

      if (needs_to_ask) {
        m_del_cell_mode = mode;
      }

      //  remember the current path
      lay::LayoutViewBase::cell_path_type cell_path (view ()->cellview (cv_index).combined_unspecific_path ());

      view ()->clear_selection ();

      std::set<db::cell_index_type> cells_to_delete;
      for (std::vector<lay::LayoutViewBase::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
        if (! p->empty () && layout.is_valid_cell_index (p->back ())) {
          cells_to_delete.insert (p->back ());
          if (mode == 2) {
            collect_cells_to_delete (layout, layout.cell (p->back ()), cells_to_delete);
          }
        }
      }

      view ()->transaction (tl::to_string (tr ("Delete cells")));

      if (mode == 0 || mode == 2) {
        layout.delete_cells (cells_to_delete);
      } else if (mode == 1) {
        layout.prune_cells (cells_to_delete);
      }

      layout.cleanup ();

      view ()->commit ();

      if (validate_cell_path (layout, cell_path)) {
        view ()->select_cell (cell_path, cv_index);
      }

    }

  }
}

void 
LayoutViewFunctions::cm_layer_copy ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->copy ();
  }
}

void 
LayoutViewFunctions::cm_layer_cut ()
{
  if (view ()->control_panel ()) {
    db::Transaction trans (manager (), tl::to_string (tr ("Cut Layers")));
    view ()->control_panel ()->cut ();
  }
}

void 
LayoutViewFunctions::cm_layer_paste ()
{
  if (view ()->control_panel ()) {
    db::Transaction trans (manager (), tl::to_string (tr ("Paste Layers")));
    view ()->control_panel ()->paste ();
  }
}

void
LayoutViewFunctions::cm_cell_cut ()
{
  if (view ()->hierarchy_panel ()) {
    //  TODO: currently the hierarchy panel's cut function does its own transaction handling.
    //  Otherwise the cut function is not working propertly.
    view ()->hierarchy_panel ()->cut ();
  }
}

void
LayoutViewFunctions::cm_cell_paste ()
{
  if (view ()->hierarchy_panel ()) {
    db::Transaction trans (manager (), tl::to_string (tr ("Paste Cells")));
    view ()->hierarchy_panel ()->paste ();
  }
}

void
LayoutViewFunctions::cm_cell_copy ()
{
  if (view ()->hierarchy_panel ()) {
    view ()->hierarchy_panel ()->copy ();
  }
}

void
LayoutViewFunctions::cm_cell_flatten ()
{
  if (! view ()->hierarchy_panel ()) {
    return;
  }

  tl_assert (view ()->is_editable ());

  int cv_index = view ()->active_cellview_index ();
  if (cv_index >= 0) {

    const lay::CellView &cv = view ()->cellview (cv_index);
    if (cv.is_valid ()) {

      std::vector<HierarchyControlPanel::cell_path_type> paths;
      view ()->selected_cells_paths (cv_index, paths);
      if (paths.empty ()) {
        throw tl::Exception (tl::to_string (tr ("No cells selected for flattening")));
      }

      for (std::vector<HierarchyControlPanel::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
        if (p->size () > 0 && cv->layout ().cell (p->back ()).is_proxy ()) {
          throw tl::Exception (tl::to_string (tr ("Cannot use this function on a PCell or library cell")));
        }
      }

      FlattenInstOptionsDialog options_dialog (parent_widget ());

      int flatten_insts_levels = -1;
      bool prune = true;
      if (options_dialog.exec_dialog (flatten_insts_levels, prune) && flatten_insts_levels != 0) {

        bool supports_undo = true;

        if (manager () && manager ()->is_enabled ()) {

          lay::TipDialog td (QApplication::activeWindow (), 
                             tl::to_string (tr ("Undo buffering for the following operation can be memory and time consuming.\nChoose \"Yes\" to use undo buffering or \"No\" for no undo buffering. Warning: in the latter case, the undo history will be lost.\n\nChoose undo buffering?")),
                             "flatten-undo-buffering",
                             lay::TipDialog::yesnocancel_buttons);

          lay::TipDialog::button_type button = lay::TipDialog::null_button;
          td.exec_dialog (button);
          if (button == lay::TipDialog::cancel_button) {
            return;
          }

          supports_undo = (button == lay::TipDialog::yes_button);

        } else {
          supports_undo = false;
        }

        view ()->cancel_edits ();
        view ()->clear_selection ();

        if (manager ()) {
          if (! supports_undo) {
            manager ()->clear ();
          } else {
            manager ()->transaction (tl::to_string (tr ("Flatten cell")));
          }
        }

        db::Layout &layout = cv->layout ();

        std::set<db::cell_index_type> child_cells;
        for (std::vector<HierarchyControlPanel::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
          if (p->size () > 0) {
            layout.cell (p->back ()).collect_called_cells (child_cells);
          }
        }

        //  don't flatten cells which are child cells of the cells to flatten
        std::set<db::cell_index_type> cells_to_flatten;
        for (std::vector<HierarchyControlPanel::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
          if (p->size () > 0 && child_cells.find (p->back ()) == child_cells.end ()) {
            cells_to_flatten.insert (p->back ());
          }
        }

        for (std::set<db::cell_index_type>::const_iterator c = cells_to_flatten.begin (); c != cells_to_flatten.end (); ++c) {
          db::Cell &target_cell = layout.cell (*c);
          layout.flatten (target_cell, flatten_insts_levels, prune);
        }

        layout.cleanup ();

        if (supports_undo && manager ()) {
          manager ()->commit ();
        }

      }

    }

  }
}

void
LayoutViewFunctions::cm_cell_rename ()
{
  int cv_index = view ()->active_cellview_index ();
  lay::LayoutViewBase::cell_path_type path;
  view ()->current_cell_path (cv_index, path);

  if (cv_index >= 0 && path.size () > 0) {

    lay::RenameCellDialog name_dialog (parent_widget ());

    db::Layout &layout = view ()->cellview (cv_index)->layout ();
    std::string name (layout.cell_name (path.back ()));
    if (name_dialog.exec_dialog (layout, name)) {

      view ()->transaction (tl::to_string (tr ("Rename cell")));
      layout.rename_cell (path.back (), name.c_str ());
      view ()->commit ();

    }

  }
}

void
LayoutViewFunctions::cm_cell_select ()
{
  if (view ()->hierarchy_panel ()) {
    view ()->hierarchy_panel ()->cm_cell_select ();
  }
}

void
LayoutViewFunctions::cm_open_current_cell ()
{
  view ()->set_current_cell_path (view ()->active_cellview_index (), view ()->cellview (view ()->active_cellview_index ()).combined_unspecific_path ());
}

void
LayoutViewFunctions::cm_cell_hide ()
{
  std::vector<HierarchyControlPanel::cell_path_type> paths;
  view ()->selected_cells_paths (view ()->active_cellview_index (), paths);

  view ()->transaction (tl::to_string (tr ("Hide cell")));

  for (std::vector<HierarchyControlPanel::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
    if (! p->empty ()) {
      view ()->hide_cell (p->back (), view ()->active_cellview_index ());
    }
  }

  view ()->commit ();
}

void
LayoutViewFunctions::cm_cell_show ()
{
  std::vector<HierarchyControlPanel::cell_path_type> paths;
  view ()->selected_cells_paths (view ()->active_cellview_index (), paths);

  view ()->transaction (tl::to_string (tr ("Show cell")));

  for (std::vector<HierarchyControlPanel::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
    if (! p->empty ()) {
      view ()->show_cell (p->back (), view ()->active_cellview_index ());
    }
  }

  view ()->commit ();
}

void
LayoutViewFunctions::cm_cell_show_all ()
{
  view ()->transaction (tl::to_string (tr ("Show all cells")));
  view ()->show_all_cells ();
  view ()->commit ();
}

void
LayoutViewFunctions::cm_select_all ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_select_all ();
  }
}

void
LayoutViewFunctions::cm_invert_selection ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_invert_selection ();
  }
}

void
LayoutViewFunctions::cm_new_tab ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_new_tab ();
  }
}

void
LayoutViewFunctions::cm_remove_tab ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_remove_tab ();
  }
}

void
LayoutViewFunctions::cm_rename_tab ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_rename_tab ();
  }
}

void
LayoutViewFunctions::cm_make_invalid ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_make_invalid ();
  }
}

void
LayoutViewFunctions::cm_make_valid ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_make_valid ();
  }
}

void
LayoutViewFunctions::cm_hide ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_hide ();
  }
}

void
LayoutViewFunctions::cm_hide_all ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_hide_all ();
  }
}

void
LayoutViewFunctions::cm_show_only ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_show_only ();
  }
}

void
LayoutViewFunctions::cm_show_all ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_show_all ();
  }
}

void
LayoutViewFunctions::cm_show ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_show ();
  }
}

void
LayoutViewFunctions::cm_toggle_visibility ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_toggle_visibility ();
  }
}

void
LayoutViewFunctions::cm_rename ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_rename ();
  }
}

void
LayoutViewFunctions::cm_delete ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_delete ();
  }
}

void
LayoutViewFunctions::cm_insert ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_insert ();
  }
}

void
LayoutViewFunctions::cm_group ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_group ();
  }
}

void
LayoutViewFunctions::cm_ungroup ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_ungroup ();
  }
}

void
LayoutViewFunctions::cm_source ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_source ();
  }
}

void
LayoutViewFunctions::cm_sort_by_name ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_sort_by_name ();
  }
}

void
LayoutViewFunctions::cm_sort_by_ild ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_sort_by_ild ();
  }
}

void
LayoutViewFunctions::cm_sort_by_idl ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_sort_by_idl ();
  }
}

void
LayoutViewFunctions::cm_sort_by_ldi ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_sort_by_ldi ();
  }
}

void
LayoutViewFunctions::cm_sort_by_dli ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_sort_by_dli ();
  }
}

void
LayoutViewFunctions::cm_regroup_by_index ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_regroup_by_index ();
  }
}

void
LayoutViewFunctions::cm_regroup_by_datatype ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_regroup_by_datatype ();
  }
}

void
LayoutViewFunctions::cm_regroup_by_layer ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_regroup_by_layer ();
  }
}

void
LayoutViewFunctions::cm_regroup_flatten ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_regroup_flatten ();
  }
}

void
LayoutViewFunctions::cm_expand_all ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_expand_all ();
  }
}

void
LayoutViewFunctions::cm_add_missing ()
{
  if (view ()->control_panel ()) {
    view ()->control_panel ()->cm_add_missing ();
  }
}

void
LayoutViewFunctions::cm_remove_unused ()
{
  view ()->remove_unused_layers ();
}

void
LayoutViewFunctions::do_cm_duplicate (bool interactive)
{
  //  Do duplicate simply by concatenating copy & paste currently.
  //  Save the clipboard state before in order to preserve the current content
  db::Clipboard saved_clipboard;
  db::Clipboard::instance ().swap (saved_clipboard);

  try {
    bool transient_mode = ! view ()->has_selection ();
    view ()->copy_view_objects ();
    view ()->clear_selection ();
    view ()->cancel ();
    if (interactive) {
      view ()->paste_interactive (transient_mode);
    } else {
      view ()->paste ();
    }
    db::Clipboard::instance ().swap (saved_clipboard);
  } catch (...) {
    db::Clipboard::instance ().swap (saved_clipboard);
    throw;
  }
}

void
LayoutViewFunctions::do_cm_paste (bool interactive)
{
  if (! db::Clipboard::instance ().empty ()) {
    view ()->cancel ();
    view ()->clear_selection ();
    if (interactive) {
      view ()->paste_interactive ();
    } else {
      view ()->paste ();
    }
  }
}

void
LayoutViewFunctions::cm_new_cell ()
{
  lay::CellView cv = view ()->cellview (view ()->active_cellview_index ());
  if (! cv.is_valid ()) {
    throw tl::Exception (tl::to_string (tr ("No layout present to add a cell to")));
  }

  static double s_new_cell_window_size = 2.0;
  static std::string s_new_cell_cell_name;

  NewCellPropertiesDialog cell_prop_dia (parent_widget ());
  if (cell_prop_dia.exec_dialog (& cv->layout (), s_new_cell_cell_name, s_new_cell_window_size)) {

    db::cell_index_type new_ci = view ()->new_cell (view ()->active_cellview_index (), s_new_cell_cell_name.c_str ());
    view ()->select_cell (new_ci, view ()->active_cellview_index ());

    db::DBox zb = db::DBox (-0.5 * s_new_cell_window_size, -0.5 * s_new_cell_window_size, 0.5 * s_new_cell_window_size, 0.5 * s_new_cell_window_size);
    if (view ()->get_max_hier_levels () < 1 || view ()->get_min_hier_levels () > 0) {
      view ()->zoom_box_and_set_hier_levels (zb, std::make_pair (0, 1));
    } else {
      view ()->zoom_box (zb);
    }

  }
}

//  TODO: this constant is defined in MainWindow.cc too ...
const int max_dirty_files = 15;

void
LayoutViewFunctions::cm_reload ()
{
  std::vector <int> selected;

  if (view ()->cellviews () > 1) {

    lay::SelectCellViewForm form (0, view (), tl::to_string (tr ("Select Layouts To Reload")));
    form.select_all ();

    if (form.exec () == QDialog::Accepted) {
      selected = form.selected_cellviews ();
    }

  } else if (view ()->cellviews () > 0) {
    selected.push_back (0);
  }

  if (selected.size () > 0) {

    int dirty_layouts = 0;
    std::string dirty_files;

    for (std::vector <int>::const_iterator i = selected.begin (); i != selected.end (); ++i) {

      const lay::CellView &cv = view ()->cellview (*i);

      if (cv->layout ().is_editable () && cv->is_dirty ()) {
        ++dirty_layouts;
        if (dirty_layouts == max_dirty_files) {
          dirty_files += "\n...";
        } else if (dirty_layouts < max_dirty_files) {
          if (! dirty_files.empty ()) {
            dirty_files += "\n";
          }
          dirty_files += cv->name ();
        }
      }

    }

    bool can_reload = true;
    if (dirty_layouts != 0) {

      QMessageBox mbox (parent_widget ());
      mbox.setText (tl::to_qstring (tl::to_string (tr ("The following layouts need saving:\n\n")) + dirty_files + "\n\nPress 'Reload Without Saving' to reload anyhow and discard changes."));
      mbox.setWindowTitle (tr ("Save Needed"));
      mbox.setIcon (QMessageBox::Warning);
      QAbstractButton *yes_button = mbox.addButton (tr ("Reload Without Saving"), QMessageBox::YesRole);
      mbox.addButton (QMessageBox::Cancel);

      mbox.exec ();

      can_reload = (mbox.clickedButton() == yes_button);

    }

    if (can_reload) {

      //  Actually reload
      for (std::vector <int>::const_iterator i = selected.begin (); i != selected.end (); ++i) {
        view ()->reload_layout (*i);
      }

    }

  }
}

void
LayoutViewFunctions::do_transform (const db::DCplxTrans &tr)
{
  //  end move operations, cancel edit operations
  view ()->cancel_edits ();
  view ()->lay::Editables::transform (tr);
}

void
LayoutViewFunctions::transform_layout (const db::DCplxTrans &tr_mic)
{
  //  end move operations, cancel edit operations
  view ()->cancel_edits ();
  view ()->clear_selection ();

  int cv_index = view ()->active_cellview_index ();
  if (cv_index >= 0) {

    db::Layout &layout = view ()->cellview (cv_index)->layout ();

    db::ICplxTrans trans (db::DCplxTrans (1.0 / layout.dbu ()) * tr_mic * db::DCplxTrans (layout.dbu ()));

    bool has_proxy = false;
    for (db::Layout::const_iterator c = layout.begin (); ! has_proxy && c != layout.end (); ++c) {
      has_proxy = c->is_proxy ();
    }

    if (has_proxy && 
        QMessageBox::question (parent_widget (),
                               tr ("Transforming PCells Or Library Cells"),
                               tr ("The layout contains PCells or library cells or both.\n"
                                   "Any changes to such cells may be lost when their layout is refreshed later.\n"
                                   "Consider using 'Convert all cells to static' before transforming the layout.\n"
                                   "\n"
                                   "Would you like to continue?\n"
                                   "Choose 'Yes' to continue anyway. Choose 'No' to cancel."),
                               QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
      return;
    }

    view ()->transaction (tl::to_string (tr ("Transform layout")));
    layout.transform (trans);
    view ()->commit ();

  }
}

void 
LayoutViewFunctions::cm_lay_flip_x ()
{
  transform_layout (db::DCplxTrans (db::FTrans::m90));
}

void 
LayoutViewFunctions::cm_lay_flip_y ()
{
  transform_layout (db::DCplxTrans (db::FTrans::m0));
}

void 
LayoutViewFunctions::cm_lay_rot_ccw ()
{
  db::DCplxTrans tr (db::DFTrans::r90);
  transform_layout (db::DCplxTrans (db::FTrans::r90));
}

void 
LayoutViewFunctions::cm_lay_rot_cw ()
{
  transform_layout (db::DCplxTrans (db::FTrans::r270));
}

void 
LayoutViewFunctions::cm_lay_free_rot ()
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), 
                                     tr ("Free rotation"),
                                     tr ("Rotation angle in degree (counterclockwise)"),
                                     QLineEdit::Normal, QString::fromUtf8 ("0.0"), 
                                     &ok);

  if (ok) {

    double angle = 0.0;
    tl::from_string_ext (tl::to_string (s), angle);

    transform_layout (db::DCplxTrans (1.0, angle, false, db::DVector ()));

  }
}

void 
LayoutViewFunctions::cm_lay_scale ()
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), 
                                     tr ("Scaling"),
                                     tr ("Scaling factor"),
                                     QLineEdit::Normal, QString::fromUtf8 ("1.0"), 
                                     &ok);

  if (ok) {

    double scale = 0.0;
    tl::from_string_ext (tl::to_string (s), scale);

    transform_layout (db::DCplxTrans (scale));

  }
}

void 
LayoutViewFunctions::cm_lay_move ()
{
  lay::MoveOptionsDialog options (parent_widget ());
  if (options.exec_dialog (m_move_dist)) {
    transform_layout (db::DCplxTrans (m_move_dist));
  }
}

void 
LayoutViewFunctions::cm_sel_flip_x ()
{
  db::DCplxTrans tr (db::DFTrans::m90);
  db::DBox sel_bbox (view ()->lay::Editables::selection_bbox ());
  if (! sel_bbox.empty ()) {
    tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
  }
  do_transform (tr);
}

void 
LayoutViewFunctions::cm_sel_flip_y ()
{
  db::DCplxTrans tr (db::DFTrans::m0);
  db::DBox sel_bbox (view ()->lay::Editables::selection_bbox ());
  if (! sel_bbox.empty ()) {
    tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
  }
  do_transform (tr);
}

void 
LayoutViewFunctions::cm_sel_rot_ccw ()
{
  db::DCplxTrans tr (db::DFTrans::r90);
  db::DBox sel_bbox (view ()->lay::Editables::selection_bbox ());
  if (! sel_bbox.empty ()) {
    tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
  }
  do_transform (tr);
}

void 
LayoutViewFunctions::cm_sel_rot_cw ()
{
  db::DCplxTrans tr (db::DFTrans::r270);
  db::DBox sel_bbox (view ()->lay::Editables::selection_bbox ());
  if (! sel_bbox.empty ()) {
    tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
  }
  do_transform (tr);
}

void 
LayoutViewFunctions::cm_sel_free_rot ()
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), 
                                     tr ("Free rotation"),
                                     tr ("Rotation angle in degree (counterclockwise)"),
                                     QLineEdit::Normal, QString::fromUtf8 ("0.0"), 
                                     &ok);

  if (ok) {

    double angle = 0.0;
    tl::from_string_ext (tl::to_string (s), angle);

    db::DCplxTrans tr = db::DCplxTrans (1.0, angle, false, db::DVector ());
    db::DBox sel_bbox (view ()->lay::Editables::selection_bbox ());
    if (! sel_bbox.empty ()) {
      tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
    }
    do_transform (tr);

  }
}

void 
LayoutViewFunctions::cm_sel_scale ()
{
  bool ok = false;
  QString s = QInputDialog::getText (QApplication::activeWindow (), 
                                     tr ("Scaling"),
                                     tr ("Scaling factor"),
                                     QLineEdit::Normal, QString::fromUtf8 ("1.0"), 
                                     &ok);

  if (ok) {

    double scale = 0.0;
    tl::from_string_ext (tl::to_string (s), scale);

    db::DCplxTrans tr = db::DCplxTrans (scale);
    db::DBox sel_bbox (view ()->lay::Editables::selection_bbox ());
    if (! sel_bbox.empty ()) {
      tr = db::DCplxTrans (sel_bbox.center () - db::DPoint ()) * tr * db::DCplxTrans (db::DPoint () - sel_bbox.center ());
    }
    do_transform (tr);

  }
}

void
LayoutViewFunctions::cm_sel_move_interactive ()
{
  if (view ()->move_service ()->begin_move ()) {
    view ()->switch_mode (-1);  //  move mode
  }
}

void 
LayoutViewFunctions::cm_sel_move_to ()
{
  db::DBox sel_bbox (view ()->lay::Editables::selection_bbox ());
  if (sel_bbox.empty ()) {
    throw tl::Exception (tl::to_string (tr ("Nothing selected to move")));
  }

  double x = sel_bbox.left () + (sel_bbox.width () * (1 + m_move_to_origin_mode_x) * 0.5);
  double y = sel_bbox.bottom () + (sel_bbox.height () * (1 + m_move_to_origin_mode_y) * 0.5);
  db::DPoint move_target (x, y);

  lay::MoveToOptionsDialog options (parent_widget ());
  if (options.exec_dialog (m_move_to_origin_mode_x, m_move_to_origin_mode_y, move_target)) {

    x = sel_bbox.left () + (sel_bbox.width () * (1 + m_move_to_origin_mode_x) * 0.5);
    y = sel_bbox.bottom () + (sel_bbox.height () * (1 + m_move_to_origin_mode_y) * 0.5);

    do_transform (db::DCplxTrans (move_target - db::DPoint (x, y)));

  }
}

void 
LayoutViewFunctions::cm_sel_move ()
{
  lay::MoveOptionsDialog options (parent_widget ());
  if (options.exec_dialog (m_move_dist)) {
    do_transform (db::DCplxTrans (m_move_dist));
  }
}

void
LayoutViewFunctions::cm_copy_layer ()
{
  struct { int *cv; int *layer; } specs [] = {
    { &m_copy_cva, &m_copy_layera },
    { &m_copy_cvr, &m_copy_layerr }
  };

  for (unsigned int i = 0; i < sizeof (specs) / sizeof (specs[0]); ++i) {

    int &cv = *(specs[i].cv);
    int &layer = *(specs[i].layer);

    if (cv >= int (view ()->cellviews ())) {
      cv = -1;
    }

    int index = view ()->active_cellview_index ();
    if (cv < 0) {
      cv = index;
    }

    if (cv < 0 || ! view ()->cellview (cv)->layout ().is_valid_layer ((unsigned int) layer)) {
      layer = -1;
    }

  }

  lay::DuplicateLayerDialog dialog (parent_widget ());
  if (dialog.exec_dialog (view (), m_copy_cva, m_copy_layera, m_copy_cvr, m_copy_layerr, m_duplicate_hier_mode, m_clear_before)) {

    bool supports_undo = true;

    if (manager () && manager ()->is_enabled ()) {

      lay::TipDialog td (QApplication::activeWindow (), 
                         tl::to_string (tr ("Undo buffering for the following operation can be memory and time consuming.\nChoose \"Yes\" to use undo buffering or \"No\" for no undo buffering. Warning: in the latter case, the undo history will be lost.\n\nChoose undo buffering?")),
                         "copy-layer-undo-buffering",
                         lay::TipDialog::yesnocancel_buttons);

      lay::TipDialog::button_type button = lay::TipDialog::null_button;
      td.exec_dialog (button);
      if (button == lay::TipDialog::cancel_button) {
        return;
      }

      supports_undo = (button == lay::TipDialog::yes_button);

    } else {
      supports_undo = false;
    }

    view ()->cancel ();

    if (manager ()) {
      if (! supports_undo) {
        manager ()->clear ();
      } else {
        manager ()->transaction (tl::to_string (tr ("Duplicate layer")));
      }
    }

    try {

      bool same_layout = (&view ()->cellview (m_copy_cvr)->layout () == &view ()->cellview (m_copy_cva)->layout ());
      if (same_layout && m_copy_layera == m_copy_layerr) {
        throw tl::Exception (tl::to_string (tr ("Source and target layer must not be identical for duplicate operation")));
      }

      if (m_duplicate_hier_mode == 0) {

        //  clear the result layer for all called cells in flat mode
        if (m_clear_before) {
          std::set<db::cell_index_type> called_cells;
          called_cells.insert (view ()->cellview (m_copy_cvr).cell_index ());
          view ()->cellview (m_copy_cvr).cell ()->collect_called_cells (called_cells);
          for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
            view ()->cellview (m_copy_cvr)->layout ().cell (*c).shapes (m_copy_layerr).clear ();
          }
        }

        db::Cell &target_cell = *view ()->cellview (m_copy_cvr).cell ();

        if (! same_layout) {

          //  flat mode (different layouts)
          db::PropertyMapper pm (&view ()->cellview (m_copy_cvr)->layout (), &view ()->cellview (m_copy_cva)->layout ());
          for (db::RecursiveShapeIterator si (view ()->cellview (m_copy_cva)->layout (), *view ()->cellview (m_copy_cva).cell (), m_copy_layera); ! si.at_end (); ++si) {
            target_cell.shapes (m_copy_layerr).insert (*si, si.trans (), pm);
          }

        } else {

          //  flat mode (same layouts)
          tl::ident_map<db::Layout::properties_id_type> pm1;
          db::Shapes &res = target_cell.shapes (m_copy_layerr);
          
          db::Layout &layout = view ()->cellview (m_copy_cvr)->layout ();
          try {

            //  using update/start_layout and end_changes improves the performance since changing the 
            //  shapes collection will invalidate the layout and cause updates inside the RecursiveShapeIerator
            layout.update ();
            layout.start_changes ();
            for (db::RecursiveShapeIterator si (view ()->cellview (m_copy_cva)->layout (), *view ()->cellview (m_copy_cva).cell (), m_copy_layera); ! si.at_end (); ++si) {
              res.insert (*si, si.trans (), pm1);
            }
            layout.end_changes ();

          } catch (...) {
            layout.end_changes ();
            throw;
          }

        }
        
      } else if (m_duplicate_hier_mode == 1) {

        db::Cell &cell = *view ()->cellview (m_copy_cva).cell ();
        db::Cell &target_cell = *view ()->cellview (m_copy_cvr).cell ();

        if (m_clear_before) {
          target_cell.clear (m_copy_layerr);
        } 

        if (m_copy_cvr == m_copy_cva) {

          //  current cell only mode: identical cell
          cell.copy (m_copy_layera, m_copy_layerr);

        } else if (! same_layout) {

          //  current cell only mode (different layouts)
          db::PropertyMapper pm (&view ()->cellview (m_copy_cvr)->layout (), &view ()->cellview (m_copy_cva)->layout ());
          for (db::Shapes::shape_iterator si = view ()->cellview (m_copy_cva).cell ()->shapes (m_copy_layera).begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
            target_cell.shapes (m_copy_layerr).insert (*si, pm);
          }

        } else {

          //  current cell only mode (same layouts, but different cells)
          for (db::Shapes::shape_iterator si = view ()->cellview (m_copy_cva).cell ()->shapes (m_copy_layera).begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
            target_cell.shapes (m_copy_layerr).insert (*si);
          }

        }
        
      } else if (m_duplicate_hier_mode == 2) {

        //  subcells cell by cell - source and target layout must be identical
        std::set<db::cell_index_type> called_cells;
        view ()->cellview (m_copy_cva).cell ()->collect_called_cells (called_cells);
        called_cells.insert (view ()->cellview (m_copy_cva).cell_index ());

        db::Layout &layout = view ()->cellview (m_copy_cva)->layout ();
        for (std::set<db::cell_index_type>::const_iterator c = called_cells.begin (); c != called_cells.end (); ++c) {
          db::Cell &cell = layout.cell (*c);
          if (m_clear_before) {
            cell.clear (m_copy_layerr);
          } 
          cell.copy (m_copy_layera, m_copy_layerr);
        }

      }

      if (manager () && supports_undo) {
        manager ()->commit ();
      }

    } catch (...) {
      if (manager () && supports_undo) {
        manager ()->commit ();
      }
      throw;
    }

  }
}

void
LayoutViewFunctions::cm_new_layer ()
{
  int index = view ()->active_cellview_index ();

  if (index >= 0 && int (view ()->cellviews ()) > index) {

    const lay::CellView &cv = view ()->cellview (index);

    lay::NewLayerPropertiesDialog prop_dia (parent_widget ());
    if (prop_dia.exec_dialog (cv, m_new_layer_props)) {

      for (unsigned int l = 0; l < cv->layout ().layers (); ++l) {
        if (cv->layout ().is_valid_layer (l) && cv->layout ().get_properties (l).log_equal (m_new_layer_props)) {
          throw tl::Exception (tl::to_string (tr ("A layer with that signature already exists: ")) + m_new_layer_props.to_string ());
        }
      }

      view ()->transaction (tl::to_string (tr ("New layer")));

      unsigned int l = cv->layout ().insert_layer (m_new_layer_props);
      std::vector <unsigned int> nl;
      nl.push_back (l);
      view ()->add_new_layers (nl, index);
      view ()->update_content ();

      view ()->commit ();

    }

  }
}

void
LayoutViewFunctions::cm_align_cell_origin ()
{
  int cv_index = view ()->active_cellview_index ();
  if (cv_index >= 0) {

    const db::Cell *cell = view ()->cellview (cv_index).cell ();
    if (! cell) {
      return;
    }
    if (cell->is_proxy ()) {
      throw tl::Exception (tl::to_string (tr ("Cannot use this function on a PCell or library cell")));
    }

    lay::AlignCellOptionsDialog dialog (parent_widget ());
    if (dialog.exec_dialog (m_align_cell_options)) {

      view ()->clear_selection ();

      view ()->transaction (tl::to_string (tr ("Align cell origin")));

      db::Box bbox;

      if (m_align_cell_options.visible_only) {
        for (lay::LayerPropertiesConstIterator l = view ()->begin_layers (); !l.at_end (); ++l) {
          if (! l->has_children () && l->layer_index () >= 0 && l->cellview_index () == cv_index && l->visible (true /*real*/)) {
            bbox += cell->bbox (l->layer_index ());
          }
        }
      } else {
        bbox = cell->bbox ();
      }

      db::Coord refx, refy;
      switch (m_align_cell_options.mode_x) {
      case -1:
        refx = bbox.left ();
        break;
      case 1:
        refx = bbox.right ();
        break;
      default:
        refx = bbox.center ().x ();
        break;
      }
      switch (m_align_cell_options.mode_y) {
      case -1:
        refy = bbox.bottom ();
        break;
      case 1:
        refy = bbox.top ();
        break;
      default:
        refy = bbox.center ().y ();
        break;
      }

      db::Layout &layout = view ()->cellview (cv_index)->layout ();
      db::Cell &nc_cell = layout.cell (cell->cell_index ());

      db::Trans t (db::Vector (-refx + db::coord_traits<db::Coord>::rounded (m_align_cell_options.xpos / layout.dbu ()), -refy + db::coord_traits<db::Coord>::rounded (m_align_cell_options.ypos / layout.dbu ())));

      for (unsigned int i = 0; i < layout.layers (); ++i) {
        if (layout.is_valid_layer (i)) {
          db::Shapes &shapes = nc_cell.shapes (i);
          for (db::Shapes::shape_iterator s = shapes.begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
            shapes.transform (*s, t);
          }
        }
      }

      for (db::Cell::const_iterator inst = nc_cell.begin (); ! inst.at_end (); ++inst) {
        nc_cell.transform (*inst, t);
      }

      if (m_align_cell_options.adjust_parents) {

        std::vector<std::pair<db::Cell *, db::Instance> > insts_to_modify;
        for (db::Cell::parent_inst_iterator pi = nc_cell.begin_parent_insts (); ! pi.at_end (); ++pi) {
          insts_to_modify.push_back (std::make_pair (& layout.cell (pi->parent_cell_index ()), pi->child_inst ()));
        }

        db::Trans ti (db::Vector (refx, refy));
        for (std::vector<std::pair<db::Cell *, db::Instance> >::const_iterator im = insts_to_modify.begin (); im != insts_to_modify.end (); ++im) {
          im->first->transform (im->second, db::Trans (db::Vector (im->second.complex_trans ().trans (db::Vector (refx, refy)))));
        }

      }

      view ()->commit ();

    }

  }
}

void
LayoutViewFunctions::cm_edit_layer ()
{
  lay::LayerPropertiesConstIterator sel = view ()->current_layer ();
  if (sel.is_null ()) {
    throw tl::Exception (tl::to_string (tr ("No layer selected for editing its properties")));
  }

  int index = sel->cellview_index ();
  if (sel->has_children () || index < 0 || int (view ()->cellviews ()) <= index || sel->layer_index () < 0) {
    throw tl::Exception (tl::to_string (tr ("No valid layer selected for editing its properties")));
  }

  const lay::CellView &cv = view ()->cellview (index);
  db::Layout &layout = cv->layout ();

  db::LayerProperties layer_props = layout.get_properties ((unsigned int) sel->layer_index ());
  db::LayerProperties old_props = layer_props;

  lay::NewLayerPropertiesDialog prop_dia (parent_widget ());
  if (prop_dia.exec_dialog (cv, layer_props)) {

    for (unsigned int l = 0; l < layout.layers (); ++l) {
      if (cv->layout ().is_valid_layer (l) && int (l) != sel->layer_index () && layout.get_properties (l).log_equal (layer_props)) {
        throw tl::Exception (tl::to_string (tr ("A layer with that signature already exists: ")) + layer_props.to_string ());
      }
    }

    view ()->transaction (tl::to_string (tr ("Edit layer")));

    cv->layout ().set_properties (sel->layer_index (), layer_props);

    //  Update all layer parameters for PCells inside the layout

    //  collect PCell variants first
    std::vector<std::pair<db::cell_index_type, const db::PCellDeclaration *> > pcell_variants;
    for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
      const db::PCellDeclaration *pcell_decl = layout.pcell_declaration_for_pcell_variant (c->cell_index ());
      if (pcell_decl) {
        pcell_variants.push_back (std::make_pair (c->cell_index (), pcell_decl));
      }
    }

    //  translate parameters if required
    std::map<db::cell_index_type, db::cell_index_type> cell_map;

    for (auto c = pcell_variants.begin (); c != pcell_variants.end (); ++c) {

      const std::vector<tl::Variant> &old_param = layout.get_pcell_parameters (c->first);
      std::vector<tl::Variant> new_param;
      const std::vector<db::PCellParameterDeclaration> &pd = c->second->parameter_declarations ();
      auto v = old_param.begin ();
      auto p = pd.begin ();
      while (v != old_param.end () && p != pd.end ()) {
        if (p->get_type () == db::PCellParameterDeclaration::t_layer && v->to_user<db::LayerProperties> ().log_equal (old_props)) {
          if (new_param.empty ()) {
            new_param = old_param;
          }
          new_param [v - old_param.begin ()] = tl::Variant (layer_props);
        }
        ++v, ++p;
      }

      if (! new_param.empty ()) {
        db::cell_index_type new_cell = layout.get_pcell_variant_cell (c->first, new_param);
        cell_map[c->first] = new_cell;
      }

    }

    //  change instances
    {
      db::LayoutLocker locker (&layout);
      for (auto c = cell_map.begin (); c != cell_map.end (); ++c) {
        layout.replace_instances_of (c->first, c->second);
      }
    }

    layout.cleanup ();

    //  Adjust view

    lay::LayerProperties lp (*sel);
    lay::ParsedLayerSource s = lp.source (false);
    s.layer (layer_props.layer);
    s.datatype (layer_props.datatype);
    if (! layer_props.name.empty ()) {
      s.name (layer_props.name);
    } else {
      s.clear_name ();
    }
    lp.set_source (s);
    view ()->set_properties (sel, lp);

    view ()->update_content ();

    view ()->commit ();

  }
}

void
LayoutViewFunctions::cm_delete_layer ()
{
  std::vector<lay::LayerPropertiesConstIterator> sel = view ()->selected_layers ();
  std::sort (sel.begin (), sel.end (), CompareLayerIteratorBottomUp ());

  //  collect valid layers
  std::vector<lay::LayerPropertiesConstIterator> valid_sel;
  std::set<std::pair<db::Layout *, unsigned int> > valid_layers;
  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator si = sel.begin (); si != sel.end (); ++si) {
    int cv_index = (*si)->cellview_index ();
    const lay::CellView &cv = view ()->cellview (cv_index);
    if (!(*si)->has_children () && cv_index >= 0 && int (view ()->cellviews ()) > cv_index && (*si)->layer_index () >= 0 && cv.is_valid ()) {
      valid_sel.push_back (*si);
      valid_layers.insert (std::make_pair (&cv->layout (), (*si)->layer_index ()));
    }
  }

  if (valid_sel.empty ()) {
    throw tl::Exception (tl::to_string (tr ("No or no valid layer selected for deleting them")));
  }

  view ()->cancel_edits ();
  view ()->clear_selection ();

  view ()->transaction (tl::to_string (tr ("Delete layers")));

  //  Hint: delete_layer must come before the layers are actually deleted in because
  //  for undo this must be the last thing to do (otherwise the layout is not propertly set up)

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator si = valid_sel.begin (); si != valid_sel.end (); ++si) {
    lay::LayerPropertiesConstIterator lp = *si;
    view ()->delete_layer (lp);
  }

  for (std::set<std::pair<db::Layout *, unsigned int> >::const_iterator li = valid_layers.begin (); li != valid_layers.end(); ++li) {

    unsigned int layer_index = li->second;
    db::Layout *layout = li->first;

    for (db::Layout::iterator c = layout->begin (); c != layout->end (); ++c) {
      c->shapes (layer_index).clear ();
    }

    layout->delete_layer (layer_index);

  }

  view ()->update_content ();

  view ()->commit ();
}

void
LayoutViewFunctions::cm_clear_layer ()
{
  std::vector<lay::LayerPropertiesConstIterator> sel = view ()->selected_layers ();
  if (sel.empty ()) {
    throw tl::Exception (tl::to_string (tr ("No layer selected for clearing")));
  }

  lay::ClearLayerModeDialog mode_dialog (parent_widget ());
  if (mode_dialog.exec_dialog (m_layer_hier_mode)) {

    view ()->cancel_edits ();
    view ()->clear_selection ();

    view ()->transaction (tl::to_string (tr ("Clear layer")));

    for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator si = sel.begin (); si != sel.end (); ++si) {

      if (! (*si)->has_children () && (*si)->layer_index () >= 0 && view ()->cellview ((*si)->cellview_index ()).is_valid ()) {

        int layer_index = (*si)->layer_index ();
        const lay::CellView &cv = view ()->cellview ((*si)->cellview_index ());

        if (m_layer_hier_mode == 0) {
          cv.cell ()->clear ((unsigned int) layer_index);
        } else if (m_layer_hier_mode == 1) {

          cv.cell ()->clear ((unsigned int) layer_index);

          std::set <db::cell_index_type> called_cells;
          cv.cell ()->collect_called_cells (called_cells);
          for (std::set <db::cell_index_type>::const_iterator cc = called_cells.begin (); cc != called_cells.end (); ++cc) {
            cv->layout ().cell (*cc).clear ((unsigned int) layer_index);
          }

        } else {
          cv->layout ().clear_layer ((unsigned int) layer_index);
        }

      }

    }

    view ()->commit ();

  }
}

// ------------------------------------------------------------
//  Declaration of the "plugin" for the menu entries

class LayoutViewPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    std::string at;

    //  secret menu entries
    at = "@secrets.end";
    menu_entries.push_back (lay::menu_item ("cm_paste_interactive", "paste_interactive:edit", at, tl::to_string (tr ("Paste Interactive"))));
    menu_entries.push_back (lay::menu_item ("cm_duplicate_interactive", "duplicate_interactive:edit", at, tl::to_string (tr ("Duplicate Interactive"))));
    menu_entries.push_back (lay::menu_item ("cm_sel_move_interactive", "sel_move_interactive:edit", at, tl::to_string (tr ("Move Interactive"))));
    menu_entries.push_back (lay::menu_item ("cm_select_next_item", "select_next_item:edit", at, tl::to_string (tr ("Select Next Item(Space)"))));
    menu_entries.push_back (lay::menu_item ("cm_select_next_item_add", "select_next_item_add:edit", at, tl::to_string (tr ("Select Next Item too(Shift+Space)"))));

    at = "edit_menu.edit_options_group";
    menu_entries.push_back (lay::menu_item ("cm_undo", "undo:edit", at, tl::to_string (tr ("Undo(Ctrl+Z)"))));
    menu_entries.push_back (lay::menu_item ("cm_redo", "redo:edit", at, tl::to_string (tr ("Redo(Ctrl+Y)"))));

    menu_entries.push_back (lay::separator ("basic_group", at));
    menu_entries.push_back (lay::submenu ("layout_menu:edit:edit_mode", at, tl::to_string (tr ("Layout"))));
    {
      std::string at = "edit_menu.layout_menu.end";
      menu_entries.push_back (lay::menu_item ("cm_lay_flip_x", "lay_flip_x:edit_mode", at, tl::to_string (tr ("Flip Horizontally"))));
      menu_entries.push_back (lay::menu_item ("cm_lay_flip_y", "lay_flip_y:edit_mode", at, tl::to_string (tr ("Flip Vertically"))));
      menu_entries.push_back (lay::menu_item ("cm_lay_rot_cw", "lay_rot_cw:edit_mode", at, tl::to_string (tr ("Rotate Clockwise"))));
      menu_entries.push_back (lay::menu_item ("cm_lay_rot_ccw", "lay_rot_ccw:edit_mode", at, tl::to_string (tr ("Rotate Counterclockwise"))));
      menu_entries.push_back (lay::menu_item ("cm_lay_free_rot", "lay_free_rot:edit_mode", at, tl::to_string (tr ("Rotation By Angle"))));
      menu_entries.push_back (lay::menu_item ("cm_lay_scale", "lay_scale:edit_mode", at, tl::to_string (tr ("Scale"))));
      menu_entries.push_back (lay::menu_item ("cm_lay_move", "lay_move:edit_mode", at, tl::to_string (tr ("Move By"))));
      menu_entries.push_back (lay::separator ("cellop_group", at));
      menu_entries.push_back (lay::menu_item ("cm_lay_convert_to_static", "lay_convert_to_static:edit_mode", at, tl::to_string (tr ("Convert All Cells To Static"))));
    }

    menu_entries.push_back (lay::submenu ("cell_menu:edit:edit_mode", at, tl::to_string (tr ("Cell"))));
    {
      std::string at = "edit_menu.cell_menu.end";
      menu_entries.push_back (lay::menu_item ("cm_new_cell", "new_cell:edit:edit_mode", at, tl::to_string (tr ("New Cell"))));
      menu_entries.push_back (lay::menu_item ("cm_cell_delete", "delete_cell:edit:edit_mode", at, tl::to_string (tr ("Delete Cell"))));
      menu_entries.push_back (lay::menu_item ("cm_cell_rename", "rename_cell:edit:edit_mode", at, tl::to_string (tr ("Rename Cell"))));
      menu_entries.push_back (lay::menu_item ("cm_cell_replace", "replace_cell:edit:edit_mode", at, tl::to_string (tr ("Replace Cell"))));
      menu_entries.push_back (lay::menu_item ("cm_cell_flatten", "flatten_cell:edit:edit_mode", at, tl::to_string (tr ("Flatten Cell"))));
      menu_entries.push_back (lay::separator ("ops_group", at));
      menu_entries.push_back (lay::menu_item ("cm_adjust_origin", "adjust_cell_origin:edit:edit_mode", at, tl::to_string (tr ("Adjust Origin"))));
      menu_entries.push_back (lay::menu_item ("cm_cell_convert_to_static", "convert_cell_to_static:edit_mode", at, tl::to_string (tr ("Convert Cell To Static"))));
      menu_entries.push_back (lay::separator ("props_group", at));
      menu_entries.push_back (lay::menu_item ("cm_cell_user_properties", "user_properties", at, tl::to_string (tr ("User Properties"))));
    }

    menu_entries.push_back (lay::submenu ("layer_menu:edit:edit_mode", at, tl::to_string (tr ("Layer"))));
    {
      std::string at = "edit_menu.layer_menu.end";
      menu_entries.push_back (lay::menu_item ("cm_new_layer", "new_layer:edit:edit_mode", at, tl::to_string (tr ("New Layer"))));
      menu_entries.push_back (lay::menu_item ("cm_clear_layer", "clear_layer:edit:edit_mode", at, tl::to_string (tr ("Clear Layer"))));
      menu_entries.push_back (lay::menu_item ("cm_delete_layer", "delete_layer:edit:edit_mode", at, tl::to_string (tr ("Delete Layer"))));
      menu_entries.push_back (lay::menu_item ("cm_copy_layer", "copy_layer:edit:edit_mode", at, tl::to_string (tr ("Copy Layer"))));
      menu_entries.push_back (lay::menu_item ("cm_edit_layer", "edit_layer:edit:edit_mode", at, tl::to_string (tr ("Edit Layer Specification"))));
    }

    menu_entries.push_back (lay::submenu ("selection_menu:edit", at, tl::to_string (tr ("Selection"))));
    {
      std::string at = "edit_menu.selection_menu.end";
      menu_entries.push_back (lay::menu_item ("cm_sel_flip_x", "sel_flip_x", at, tl::to_string (tr ("Flip Horizontally"))));
      menu_entries.push_back (lay::menu_item ("cm_sel_flip_y", "sel_flip_y", at, tl::to_string (tr ("Flip Vertically"))));
      menu_entries.push_back (lay::menu_item ("cm_sel_rot_cw", "sel_rot_cw", at, tl::to_string (tr ("Rotate Clockwise"))));
      menu_entries.push_back (lay::menu_item ("cm_sel_rot_ccw", "sel_rot_ccw", at, tl::to_string (tr ("Rotate Counterclockwise"))));
      menu_entries.push_back (lay::menu_item ("cm_sel_free_rot", "sel_free_rot", at, tl::to_string (tr ("Rotation By Angle"))));
      menu_entries.push_back (lay::menu_item ("cm_sel_scale", "sel_scale", at, tl::to_string (tr ("Scale"))));
      menu_entries.push_back (lay::menu_item ("cm_sel_move", "sel_move", at, tl::to_string (tr ("Move By"))));
      menu_entries.push_back (lay::menu_item ("cm_sel_move_to", "sel_move_to", at, tl::to_string (tr ("Move To"))));
    }

    menu_entries.push_back (lay::separator ("utils_group", at));
    menu_entries.push_back (lay::submenu ("utils_menu:edit:edit_mode", at, tl::to_string (tr ("Utilities"))));

    menu_entries.push_back (lay::separator ("misc_group", at));
    menu_entries.push_back (lay::menu_item ("cm_delete", "delete:edit", at, tl::to_string (tr ("Delete(Del)"))));
    menu_entries.push_back (lay::menu_item ("cm_show_properties", "show_properties:edit", at, tl::to_string (tr ("Properties(Q)"))));

    menu_entries.push_back (lay::separator ("cpc_group", at));
    menu_entries.push_back (lay::menu_item ("cm_copy", "copy:edit", at, tl::to_string (tr ("Copy(Ctrl+C)"))));
    menu_entries.push_back (lay::menu_item ("cm_cut", "cut:edit", at, tl::to_string (tr ("Cut(Ctrl+X)"))));
    menu_entries.push_back (lay::menu_item ("cm_paste", "paste:edit", at, tl::to_string (tr ("Paste(Ctrl+V)"))));
    menu_entries.push_back (lay::menu_item ("cm_duplicate", "duplicate:edit", at, tl::to_string (tr ("Duplicate(Ctrl+B)"))));

    menu_entries.push_back (lay::separator ("modes_group", at));
    menu_entries.push_back (lay::submenu ("mode_menu", at, tl::to_string (tr ("Mode"))));

    menu_entries.push_back (lay::submenu ("select_menu", at, tl::to_string (tr ("Select"))));
    {
      std::string at = "edit_menu.select_menu.end";
      menu_entries.push_back (lay::menu_item ("cm_select_all", "select_all", at, tl::to_string (tr ("Select All"))));
      menu_entries.push_back (lay::menu_item ("cm_unselect_all", "unselect_all", at, tl::to_string (tr ("Unselect All"))));
      menu_entries.push_back (lay::separator ("edit_select_basic_group", at));
      menu_entries.push_back (lay::menu_item ("lv:enable_all", "enable_all", at, tl::to_string (tr ("Enable All"))));
      menu_entries.push_back (lay::menu_item ("lv:disable_all", "disable_all", at, tl::to_string (tr ("Disable All"))));
      menu_entries.push_back (lay::separator ("edit_select_individual_group", at));
    };

    menu_entries.push_back (lay::separator ("cancel_group", at));
    menu_entries.push_back (lay::menu_item ("cm_cancel", "cancel", at, tl::to_string (tr ("Cancel(Esc)"))));

    at = "bookmark_menu.end";
    menu_entries.push_back (lay::submenu ("goto_bookmark_menu", at, tl::to_string (tr ("Goto Bookmark"))));
    menu_entries.push_back (lay::menu_item ("cm_bookmark_view", "bookmark_view", at, tl::to_string (tr ("Bookmark This View"))));

    menu_entries.push_back (lay::separator ("bookmark_mgm_group", at));
    menu_entries.push_back (lay::menu_item ("cm_manage_bookmarks", "manage_bookmarks", at, tl::to_string (tr ("Manage Bookmarks"))));
    menu_entries.push_back (lay::menu_item ("cm_load_bookmarks", "load_bookmarks", at, tl::to_string (tr ("Load Bookmarks"))));
    menu_entries.push_back (lay::menu_item ("cm_save_bookmarks", "save_bookmarks", at, tl::to_string (tr ("Save Bookmarks"))));
    menu_entries.push_back (lay::submenu ("open_recent_menu_bookmarks", at, tl::to_string (tr ("Recent Bookmark Files"))));

    at = "zoom_menu.end";
    menu_entries.push_back (lay::submenu ("global_trans", at, tl::to_string (tr ("Global Transformation"))));
    {
      std::string at = "zoom_menu.global_trans.end";
      menu_entries.push_back (lay::config_menu_item ("r0", at, tl::to_string (tr ("\\(r0\\)<:/r0_24px.png>")), cfg_global_trans, "?r0 *1 0,0"));
      menu_entries.push_back (lay::config_menu_item ("r90", at, tl::to_string (tr ("\\(r90\\)<:/r90_24px.png>")), cfg_global_trans, "?r90 *1 0,0"));
      menu_entries.push_back (lay::config_menu_item ("r180", at, tl::to_string (tr ("\\(r180\\)<:/r180_24px.png>")), cfg_global_trans, "?r180 *1 0,0"));
      menu_entries.push_back (lay::config_menu_item ("r270", at, tl::to_string (tr ("\\(r270\\)<:/r270_24px.png>")), cfg_global_trans, "?r270 *1 0,0"));
      menu_entries.push_back (lay::config_menu_item ("m0", at, tl::to_string (tr ("\\(m0\\)<:/m0_24px.png>")), cfg_global_trans, "?m0 *1 0,0"));
      menu_entries.push_back (lay::config_menu_item ("m45", at, tl::to_string (tr ("\\(m45\\)<:/m45_24px.png>")), cfg_global_trans, "?m45 *1 0,0"));
      menu_entries.push_back (lay::config_menu_item ("m90", at, tl::to_string (tr ("\\(m90\\)<:/m90_24px.png>")), cfg_global_trans, "?m90 *1 0,0"));
      menu_entries.push_back (lay::config_menu_item ("m135", at, tl::to_string (tr ("\\(m135\\)<:/m135_24px.png>")), cfg_global_trans, "?m135 *1 0,0"));
    }

    menu_entries.push_back (lay::separator ("hier_group", at));
    menu_entries.push_back (lay::menu_item ("cm_max_hier", "max_hier", at, tl::to_string (tr ("Full Hierarchy(*)"))));
    menu_entries.push_back (lay::menu_item ("cm_max_hier_0", "max_hier_0", at, tl::to_string (tr ("Box Only(0)"))));
    menu_entries.push_back (lay::menu_item ("cm_max_hier_1", "max_hier_1", at, tl::to_string (tr ("Top Level Only(1)"))));
    menu_entries.push_back (lay::menu_item ("cm_inc_max_hier", "inc_max_hier", at, tl::to_string (tr ("Increment Hierarchy(+)"))));
    menu_entries.push_back (lay::menu_item ("cm_dec_max_hier", "dec_max_hier", at, tl::to_string (tr ("Decrement Hierarchy(-)"))));

    menu_entries.push_back (lay::separator ("zoom_group", at));
    menu_entries.push_back (lay::menu_item ("cm_zoom_fit", "zoom_fit", at, tl::to_string (tr ("Zoom Fit(F2)"))));
    menu_entries.push_back (lay::menu_item ("cm_zoom_fit_sel", "zoom_fit_sel", at, tl::to_string (tr ("Zoom Fit Selection(Shift+F2)"))));
    menu_entries.push_back (lay::menu_item ("cm_zoom_in", "zoom_in", at, tl::to_string (tr ("Zoom In(Ctrl++)"))));
    menu_entries.push_back (lay::menu_item ("cm_zoom_out", "zoom_out", at, tl::to_string (tr ("Zoom Out(Ctrl+-)"))));
    /* disabled because that interferes with the use of the arrow keys for moving the selection
    MenuLayoutEntry::separator ("pan_group");
    menu_entries.push_back (lay::menu_item ("cm_pan_up", "pan_up", at, tl::to_string (tr ("Pan Up(Up)"))));
    menu_entries.push_back (lay::menu_item ("cm_pan_down", "pan_down", at, tl::to_string (tr ("Pan Down(Down)"))));
    menu_entries.push_back (lay::menu_item ("cm_pan_left", "pan_left", at, tl::to_string (tr ("Pan Left(Left)"))));
    menu_entries.push_back (lay::menu_item ("cm_pan_right", "pan_right", at, tl::to_string (tr ("Pan Right(Right)"))));
    */

    menu_entries.push_back (lay::separator ("redraw_group", at));
    menu_entries.push_back (lay::menu_item ("cm_redraw", "redraw", at, tl::to_string (tr ("Redraw"))));
    menu_entries.push_back (lay::separator ("state_group", at));
    menu_entries.push_back (lay::menu_item_copy ("cm_prev_display_state", "prev_display_state", at, "@toolbar.prev_display_state"));
    menu_entries.push_back (lay::menu_item_copy ("cm_next_display_state", "next_display_state", at, "@toolbar.next_display_state"));

    menu_entries.push_back (lay::separator ("select_group", at));
    menu_entries.push_back (lay::menu_item ("cm_select_cell", "select_cell:edit", at, tl::to_string (tr ("Select Cell"))));
    menu_entries.push_back (lay::menu_item ("cm_select_current_cell", "select_current_cell", at, tl::to_string (tr ("Show As New Top(Ctrl+S)"))));
    menu_entries.push_back (lay::menu_item ("cm_goto_position", "goto_position", at, tl::to_string (tr ("Goto Position(Ctrl+G)"))));

    //  Add a hook for inserting new items after the modes
    menu_entries.push_back (lay::separator ("end_modes", "@toolbar.end"));

  }

  bool menu_activated (const std::string &symbol) const
  {
    if (symbol == "lv:enable_all") {

      for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
        cls->set_editable_enabled (true);
      }
      return true;

    } else if (symbol == "lv:disable_all") {

      for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
        cls->set_editable_enabled (false);
      }
      return true;

    } else {
      return false;
    }
  }

  void implements_primary_mouse_modes (std::vector<std::pair<std::string, std::pair<std::string, int> > > &modes)
  {
    std::vector <std::string> mode_titles;
    lay::LayoutViewBase::intrinsic_mouse_modes (&mode_titles);

    int mode_id = 0;
    for (std::vector <std::string>::const_iterator t = mode_titles.begin (); t != mode_titles.end (); ++t, --mode_id) {
      //  modes: pair(title, pair(insert_pos, id))
      modes.push_back (std::make_pair (*t, std::make_pair ("edit_menu.mode_menu.end;@toolbar.end_modes", mode_id)));
    }
  }

  lay::Plugin *create_plugin (db::Manager *manager, Dispatcher *, LayoutViewBase *view) const
  {
    return new LayoutViewFunctions (manager, view);
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new LayoutViewPluginDeclaration (), -10, "LayoutViewPlugin");

} // namespace lay

#endif

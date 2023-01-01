
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



#ifndef HDR_edtMainService
#define HDR_edtMainService

#include "layEditable.h"
#include "layPlugin.h"
#include "layViewObject.h"
#include "layMarker.h"
#include "dbLayout.h"
#include "dbShape.h"
#include "dbClipboard.h"
#include "edtUtils.h"

#include <set>
#include <vector>

namespace lay {
  class Dispatcher;
  class FlattenInstOptionsDialog;
}

namespace edt {

class Service;
class EditorOptionsPages;
class EditorOptionsPage;
class RoundCornerOptionsDialog;
class AreaAndPerimeterDialog;
class MakeCellOptionsDialog;
class MakeArrayOptionsDialog;
class AlignOptionsDialog;
class DistributeOptionsDialog;

// -------------------------------------------------------------

class MainService
  : public lay::Plugin,
    public lay::Editable,
    public db::Object
{
public: 
  /**
   *  @brief The constructor
   */
  MainService (db::Manager *manager, lay::LayoutViewBase *view, lay::Dispatcher *root);

  /**
   *  @brief The destructor
   */
  ~MainService ();

  /**
   *  @brief Access to the view object
   */
  lay::LayoutViewBase *view () const
  {
    return mp_view;
  }

  /** 
   *  @brief Implementation of the menu functions
   */
  virtual void menu_activated (const std::string &symbol);

  /**
   *  @brief Descend to selection 
   */
  void cm_descend ();

  /**
   *  @brief Ascend one level
   */
  void cm_ascend ();

  /**
   *  @brief Change the layer of the shapes in the selection
   */
  void cm_change_layer ();

  /**
   *  @brief Round corners on selection
   */
  void cm_round_corners ();

  /**
   *  @brief Show area and perimeter of selection
   */
  void cm_area_perimeter ();

  /**
   *  @brief Convert selection to PCell
   */
  void cm_convert_to_pcell ();

  /**
   *  @brief Convert selection to static cell
   */
  void cm_convert_to_cell ();

  /**
   *  @brief Size shapes (merge before)
   */
  void cm_size ();

  /**
   *  @brief Merge shapes
   */
  void cm_union ();

  /**
   *  @brief Intersection of shapes
   */
  void cm_intersection ();

  /**
   *  @brief Separation of shapes
   */
  void cm_separate ();

  /**
   *  @brief Difference of shapes
   */
  void cm_difference ();

  /**
   *  @brief Make array from the selected shapes and instances
   */
  void cm_make_array ();

  /**
   *  @brief Align the selected shapes and instances
   */
  void cm_align ();

  /**
   *  @brief Distribute the selected shapes and instances
   */
  void cm_distribute ();

  /**
   *  @brief Flatten instances
   */
  void cm_flatten_insts ();

  /**
   *  @brief Resolve array refs
   */
  void cm_resolve_arefs ();

  /**
   *  @brief Move selection up in hierarchy
   */
  void cm_move_hier_up ();

  /**
   *  @brief Make new cell from selection
   */
  void cm_make_cell ();

  /**
   *  @brief Make variants for selection
   */
  void cm_make_variants ();

  /**
   *  @brief Make variants so that selection operations can be applied without disturbing other instances
   */
  void cm_make_cell_variants ();

  /**
   *  @brief Tap operation
   */
  void cm_tap ();

  /** 
   *  @brief "paste" operation
   */
  virtual void paste ();

private:
  //  The layout view that this service is attached to
  lay::LayoutViewBase *mp_view;
  lay::Dispatcher *mp_root;
  bool m_needs_update;

  //  options 
  int m_flatten_insts_levels;
  bool m_flatten_prune;
  int m_align_hmode;
  int m_align_vmode;
  bool m_align_visible_layers;
  bool m_hdistribute;
  int m_distribute_hmode;
  double m_distribute_hpitch, m_distribute_hspace;
  bool m_vdistribute;
  int m_distribute_vmode;
  double m_distribute_vpitch, m_distribute_vspace;
  bool m_distribute_visible_layers;
  std::string m_make_cell_name;
  int m_origin_mode_x, m_origin_mode_y;
  bool m_origin_visible_layers_for_bbox;
  db::DVector m_array_a, m_array_b;
  unsigned int m_array_na, m_array_nb;
  double m_router, m_rinner;
  unsigned int m_npoints;
  bool m_undo_before_apply;
#if defined(HAVE_QT)
  edt::RoundCornerOptionsDialog *mp_round_corners_dialog;
  edt::AreaAndPerimeterDialog *mp_area_and_perimeter_dialog;
  edt::AlignOptionsDialog *mp_align_options_dialog;
  edt::DistributeOptionsDialog *mp_distribute_options_dialog;
  lay::FlattenInstOptionsDialog *mp_flatten_inst_options_dialog;
  edt::MakeCellOptionsDialog *mp_make_cell_options_dialog;
  edt::MakeArrayOptionsDialog *mp_make_array_options_dialog;
#endif

  void boolean_op (int mode);
  void check_no_guiding_shapes ();
#if defined(HAVE_QT)
  edt::RoundCornerOptionsDialog *round_corners_dialog ();
  edt::AreaAndPerimeterDialog *area_and_perimeter_dialog ();
  edt::AlignOptionsDialog *align_options_dialog ();
  edt::DistributeOptionsDialog *distribute_options_dialog ();
  lay::FlattenInstOptionsDialog *flatten_inst_options_dialog ();
  edt::MakeCellOptionsDialog *make_cell_options_dialog ();
  edt::MakeArrayOptionsDialog *make_array_options_dialog ();
#endif
};

}

#endif


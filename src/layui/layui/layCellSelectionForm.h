
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#ifndef HDR_layCellSelectionForm
#define HDR_layCellSelectionForm

#include "layuiCommon.h"
#include "layCellView.h"
#include "tlDeferredExecution.h"

#include <vector>
#include <string>

#include <QAction>
#include <QDialog>
#include <QModelIndex>

namespace Ui
{
  class CellSelectionForm;
  class LibraryCellSelectionForm;
}

namespace lay
{

class CellView;
class LayoutView;

/**
 *  @brief A form to select a cell and a cell view index
 */
class LAYUI_PUBLIC CellSelectionForm
  : public QDialog
{
  Q_OBJECT 

public:
  CellSelectionForm (QWidget *parent, LayoutViewBase *view, const char *name, bool simple_mode = false);

  /**
   *  @brief Obtain the selected cellview's index (with changes)
   */
  int selected_cellview_index () const;

  /**
   *  @brief Get the selected cellview's state
   */
  const lay::CellView &selected_cellview () const;

public slots:
  void view_changed(int);
  void cell_changed(const QModelIndex &current, const QModelIndex &);
  void child_changed(const QModelIndex &current);
  void parent_changed(const QModelIndex &current);
  void name_changed();
  void set_parent();
  void set_child();
  void hide_cell();
  void show_cell();
  void apply_clicked();
  void find_next_clicked();
  void find_prev_clicked();

private:
  Ui::CellSelectionForm *mp_ui;
  lay::LayoutViewBase *mp_view;
  std::vector <lay::CellView> m_cellviews;
  int m_current_cv;
  bool m_name_cb_enabled;
  bool m_cells_cb_enabled;
  bool m_children_cb_enabled;
  bool m_parents_cb_enabled;
  tl::DeferredMethod<CellSelectionForm> m_update_all_dm;
  bool m_simple_mode;
  QAction *mp_use_regular_expressions;
  QAction *mp_case_sensitive;

  void update_cell_list ();
  void update_parents_list ();
  void update_children_list ();
  void update_all ();
  void select_entry (lay::CellView::cell_index_type n);
  void commit_cv ();
  void store_config ();
  void accept ();
  void reject ();
};

/**
 *  @brief A form to select a cell from a library
 */
class LAYUI_PUBLIC LibraryCellSelectionForm
  : public QDialog
{
  Q_OBJECT 

public:
  /**
   *  @brief Create a selection form for cells from the given layout
   *
   *  This version does not provide library selection. \get_current_library will
   *  always return 0.
   *  If all_cells is true, all cells (not just top cells and basic cells) are shown.
   *  If top_cells_only is false, child cells are shown as well.
   */
  LibraryCellSelectionForm (QWidget *parent, db::Layout *layout, const char *name, bool all_cells = false, bool top_cells_only = true);

  /**
   *  @brief Create a selection form for cells plus the library
   *
   *  If all_cells is true, all cells (not only top cells and basic cells) are shown.
   *  If top_cells_only is false, child cells are shown as well.
   */
  LibraryCellSelectionForm (QWidget *parent, const char *name, bool all_cells = false, bool top_cells_only = true);

  /**
   *  @brief Set the selected library
   */
  void set_current_library (db::Library *lib);

  /**
   *  @brief Get the selected library
   */
  db::Library *get_current_library () const
  {
    return mp_lib;
  }

  /**
   *  @brief Set the selected cell's index
   */
  void set_selected_cell_index (db::cell_index_type ci);

  /**
   *  @brief Select the PCell with the given ID (when BasicCells is selected in the flags)
   */
  void set_selected_pcell_id (db::pcell_id_type pci);

  /**
   *  @brief Obtain the selected cell's index
   */
  db::cell_index_type selected_cell_index () const
  {
    return m_cell_index;
  }

  /**
   *  @brief Returns true, if the selected cell is a pcell
   */
  bool selected_cell_is_pcell () const
  {
    return m_is_pcell;
  }

  /**
   *  @brief Get the PCell ID of the selected cell (if it's a PCell)
   */
  db::pcell_id_type selected_pcell_id () const
  {
    return m_pcell_id;
  }

public slots:
  void name_changed(const QString &);
  void cell_changed(const QModelIndex &current, const QModelIndex &);
  void find_next_clicked();
  void lib_changed ();
  void show_all_changed ();

private:
  Ui::LibraryCellSelectionForm *mp_ui;
  db::Library *mp_lib;
  const db::Layout *mp_layout;
  bool m_name_cb_enabled;
  bool m_cells_cb_enabled;
  db::cell_index_type m_cell_index;
  db::pcell_id_type m_pcell_id;
  bool m_is_pcell;
  bool m_all_cells;
  bool m_top_cells_only;

  void select_entry (db::cell_index_type n);
  void select_pcell_entry (db::pcell_id_type n);
  void update_cell_list ();
  void accept ();
};

}

#endif

#endif  //  defined(HAVE_QT)

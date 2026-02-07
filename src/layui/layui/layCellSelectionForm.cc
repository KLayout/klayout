
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

#include <algorithm>
#include <string>
#include <set>

#include "layCellSelectionForm.h"

#include "dbCellInst.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"

#include "tlException.h"
#include "tlAlgorithm.h"
#include "tlAssert.h"

#include "layCellTreeModel.h"
#include "layLayoutViewBase.h"
#include "tlExceptions.h"

#include "ui_CellSelectionForm.h"
#include "ui_LibraryCellSelectionForm.h"

#include <QHeaderView>

namespace lay
{

static const std::string cfg_cell_selection_search_case_sensitive ("cell-selection-search-case-sensitive");
static const std::string cfg_cell_selection_search_use_expressions ("cell-selection-search-use-expression");

// ------------------------------------------------------------

CellSelectionForm::CellSelectionForm (QWidget *parent, LayoutViewBase *view, const char *name, bool simple_mode)
  : QDialog (parent),
    mp_view (view),
    m_current_cv (-1),
    m_name_cb_enabled (true),
    m_cells_cb_enabled (true),
    m_children_cb_enabled (true),
    m_parents_cb_enabled (true),
    m_update_all_dm (this, &CellSelectionForm::update_all),
    m_simple_mode (simple_mode)
{
  mp_ui = new Ui::CellSelectionForm ();
  setObjectName (QString::fromUtf8 (name));

  mp_ui->setupUi (this);

  mp_ui->le_cell_name->set_tab_signal_enabled (true);

  mp_use_regular_expressions = new QAction (this);
  mp_use_regular_expressions->setCheckable (true);
  mp_use_regular_expressions->setChecked (true);
  mp_use_regular_expressions->setText (tr ("Use expressions (use * and ? for any character)"));

  mp_case_sensitive = new QAction (this);
  mp_case_sensitive->setCheckable (true);
  mp_case_sensitive->setChecked (true);
  mp_case_sensitive->setText (tr ("Case sensitive search"));

  if (lay::Dispatcher::instance ()) {
    bool cs = true;
    lay::Dispatcher::instance ()->config_get (cfg_cell_selection_search_case_sensitive, cs);
    mp_case_sensitive->setChecked (cs);
    bool ue = true;
    lay::Dispatcher::instance ()->config_get (cfg_cell_selection_search_use_expressions, ue);
    mp_use_regular_expressions->setChecked (ue);
  }

  QMenu *m = new QMenu (mp_ui->le_cell_name);
  m->addAction (mp_use_regular_expressions);
  m->addAction (mp_case_sensitive);
  connect (mp_use_regular_expressions, SIGNAL (triggered ()), this, SLOT (name_changed ()));
  connect (mp_case_sensitive, SIGNAL (triggered ()), this, SLOT (name_changed ()));

  mp_ui->le_cell_name->set_clear_button_enabled (true);
  mp_ui->le_cell_name->set_options_button_enabled (true);
  mp_ui->le_cell_name->set_options_menu (m);


  // signals and slots connections
  connect (mp_ui->cancel_button, SIGNAL(clicked()), this, SLOT(reject()));
  connect (mp_ui->cb_views, SIGNAL(activated(int)), this, SLOT(view_changed(int)));
  connect (mp_ui->tb_set_parent, SIGNAL(clicked()), this, SLOT(set_parent()));
  connect (mp_ui->tb_set_child, SIGNAL(clicked()), this, SLOT(set_child()));
  connect (mp_ui->pb_hide, SIGNAL(clicked()), this, SLOT(hide_cell()));
  connect (mp_ui->pb_show, SIGNAL(clicked()), this, SLOT(show_cell()));
  connect (mp_ui->le_cell_name, SIGNAL(textChanged(const QString&)), this, SLOT(name_changed()));
  connect (mp_ui->ok_button, SIGNAL(clicked()), this, SLOT(accept()));
  connect (mp_ui->apply_button, SIGNAL(clicked()), this, SLOT(apply_clicked()));
  connect (mp_ui->find_next, SIGNAL(clicked()), this, SLOT(find_next_clicked()));
  connect (mp_ui->le_cell_name, SIGNAL(tab_pressed()), this, SLOT(find_next_clicked()));
  connect (mp_ui->le_cell_name, SIGNAL(backtab_pressed()), this, SLOT(find_prev_clicked()));

  connect (mp_ui->lv_parents, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(parent_changed(const QModelIndex &)));
  connect (mp_ui->lv_children, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(child_changed(const QModelIndex &)));


  m_cellviews.reserve (mp_view->cellviews ());
  for (unsigned int i = 0; i < mp_view->cellviews (); ++i) {
    m_cellviews.push_back (mp_view->cellview (i));
  }

  if (simple_mode) {
    mp_ui->apply_button->hide ();
    mp_ui->tools_frame->hide ();
  } else {
    mp_ui->apply_button->show ();
    mp_ui->tools_frame->show ();
  }

  if (! m_cellviews.empty ()) {

    m_current_cv = view->active_cellview_index ();

    int cvi = 0;
    for (std::vector<lay::CellView>::const_iterator cv = m_cellviews.begin (); cv != m_cellviews.end (); ++cv, ++cvi) {
      mp_ui->cb_views->addItem (tl::to_qstring ((*cv)->name () + " (@" + tl::to_string (cvi + 1) + ")"));
    }
    mp_ui->cb_views->setCurrentIndex (m_current_cv);

    if (m_cellviews.size () == 1) {
      mp_ui->cb_views->hide ();
      mp_ui->layout_lbl->hide ();
    } else {
      mp_ui->cb_views->show ();
      mp_ui->layout_lbl->show ();
    }

    mp_ui->lv_cells->header ()->hide ();
    mp_ui->lv_cells->setRootIsDecorated (false);

    mp_ui->lv_children->header ()->hide ();
    mp_ui->lv_children->setRootIsDecorated (false);

    mp_ui->lv_parents->header ()->hide ();
    mp_ui->lv_parents->setRootIsDecorated (false);
      
    update_cell_list ();  

  }

}

void 
CellSelectionForm::update_cell_list () 
{
  if (m_current_cv < 0 || m_current_cv >= int (m_cellviews.size ())) {
    return;
  }

  if (mp_ui->lv_cells->model ()) {
    delete mp_ui->lv_cells->model ();
  }

  lay::CellTreeModel *model = new lay::CellTreeModel (mp_ui->lv_cells, mp_view, m_current_cv, lay::CellTreeModel::Flat);
  
  mp_ui->lv_cells->setModel (model);
  //  connect can only happen after setModel()
  connect (mp_ui->lv_cells->selectionModel (), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(cell_changed(const QModelIndex &, const QModelIndex &)));

  lay::CellView::unspecific_cell_path_type path (m_cellviews [m_current_cv].combined_unspecific_path ());
  if (! path.empty ()) {
    select_entry (path.back ());
  }
}

void 
CellSelectionForm::update_parents_list ()
{
  m_parents_cb_enabled = false;

  if (m_current_cv >= 0 && m_current_cv < int (m_cellviews.size ())) {

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
    if (model) {

      if (mp_ui->lv_parents->model ()) {
        delete mp_ui->lv_parents->model ();
      }
      mp_ui->lv_parents->setModel (new lay::CellTreeModel (mp_ui->lv_parents, mp_view, m_current_cv, lay::CellTreeModel::Flat | lay::CellTreeModel::Parents, model->cell (mp_ui->lv_cells->selectionModel ()->currentIndex ())));

    }

  }

  m_parents_cb_enabled = true;
}

void 
CellSelectionForm::update_children_list ()
{
  m_children_cb_enabled = false;
  
  if (m_current_cv >= 0 && m_current_cv < int (m_cellviews.size ())) {

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
    if (model) {

      if (mp_ui->lv_children->model ()) {
        delete mp_ui->lv_children->model ();
      }
      mp_ui->lv_children->setModel (new lay::CellTreeModel (mp_ui->lv_children, mp_view, m_current_cv, lay::CellTreeModel::Flat | lay::CellTreeModel::Children, model->cell (mp_ui->lv_cells->selectionModel ()->currentIndex ())));

    }

  }

  m_children_cb_enabled = true;
}

int 
CellSelectionForm::selected_cellview_index () const
{
  return m_current_cv;
}

const lay::CellView &
CellSelectionForm::selected_cellview () const
{
  tl_assert (m_current_cv >= 0 && m_current_cv < int (m_cellviews.size ()));
  return m_cellviews [m_current_cv];
}

void 
CellSelectionForm::commit_cv ()
{
  //  update the cell view 
  if (m_current_cv >= 0 && m_current_cv < int (m_cellviews.size ())) {

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
    if (! model) {
      return;
    }

    const db::Cell *cell = model->cell (mp_ui->lv_cells->selectionModel ()->currentIndex ());
    if (cell) {
      m_cellviews [m_current_cv].set_cell (cell->cell_index ());
    }

  }
}

void 
CellSelectionForm::view_changed (int cv)
{
  commit_cv ();
  m_current_cv = cv;
  update_cell_list ();  
}

void
CellSelectionForm::accept ()
{
  store_config ();
  commit_cv ();
  QDialog::accept ();
}

void
CellSelectionForm::reject ()
{
  store_config ();
  QDialog::reject ();
}

void
CellSelectionForm::store_config ()
{
  if (lay::Dispatcher::instance ()) {
    lay::Dispatcher::instance ()->config_set (cfg_cell_selection_search_case_sensitive, mp_case_sensitive->isChecked ());
    lay::Dispatcher::instance ()->config_set (cfg_cell_selection_search_use_expressions, mp_use_regular_expressions->isChecked ());
  }
}

void
CellSelectionForm::apply_clicked()
{
  //  select the current cell but don't make it the new top.
  if (m_current_cv >= 0 && m_current_cv < int (m_cellviews.size ())) {

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
    if (! model) {
      return;
    }

    const db::Cell *cell = model->cell (mp_ui->lv_cells->selectionModel ()->currentIndex ());

    lay::CellView cv (m_cellviews [m_current_cv]);
    cv.set_cell (cell->cell_index ());
    mp_view->set_current_cell_path(m_current_cv, cv.combined_unspecific_path ());

  }
}

void 
CellSelectionForm::cell_changed (const QModelIndex &current, const QModelIndex &)
{
  if (m_cells_cb_enabled) {

    m_name_cb_enabled = false;

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
    if (model) {
      mp_ui->le_cell_name->setText (tl::to_qstring (model->cell_name (current)));
      model->clear_locate ();
    }

    m_name_cb_enabled = true;

    update_children_list ();
    update_parents_list ();

  }
}

void
CellSelectionForm::set_child ()
{
  child_changed (mp_ui->lv_children->selectionModel ()->currentIndex ());
}

void 
CellSelectionForm::child_changed(const QModelIndex &current)
{
  if (m_children_cb_enabled && current.isValid ()) {
    if (m_current_cv >= 0 && m_current_cv < int (m_cellviews.size ())) {
      lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_children->model ());
      if (model) {
        select_entry (model->cell_index (mp_ui->lv_children->selectionModel ()->currentIndex ()));
      }
    }
  }
}

void
CellSelectionForm::set_parent ()
{
  parent_changed (mp_ui->lv_parents->selectionModel ()->currentIndex ());
}

void 
CellSelectionForm::parent_changed(const QModelIndex &current)
{
  if (m_parents_cb_enabled && current.isValid ()) {
    if (m_current_cv >= 0 && m_current_cv < int (m_cellviews.size ())) {
      lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_parents->model ());
      if (model) {
        select_entry (model->cell_index (mp_ui->lv_parents->selectionModel ()->currentIndex ()));
      }
    }
  }
}

void 
CellSelectionForm::select_entry (lay::CellView::cell_index_type ci)
{
  m_cells_cb_enabled = false;

  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
  if (! model) {
    return;
  }

  //  select the current entry
  QModelIndex mi;
  for (int c = 0; c < model->toplevel_items (); ++c) {
    lay::CellTreeItem *item = model->toplevel_item (c);
    if (item->cell_or_pcell_index () == ci) {
      mi = model->model_index (item);
      break;
    }
  }
        
  if (mi.isValid ()) {

    m_cells_cb_enabled = false;
    mp_ui->lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    mp_ui->lv_cells->scrollTo (mi);
    m_cells_cb_enabled = true;

    m_name_cb_enabled = false;
    mp_ui->le_cell_name->setText (tl::to_qstring (model->cell_name (mi)));
    model->clear_locate ();
    m_name_cb_enabled = true;

    //  do child list updates in a user event handler. Otherwise changing the models
    //  immediately interferes with Qt's internal logic. So we do an deferred update.
    m_update_all_dm ();

  }
  
  m_cells_cb_enabled = true;
}

void 
CellSelectionForm::update_all ()
{
  update_children_list ();
  update_parents_list ();
}

void 
CellSelectionForm::find_next_clicked ()
{
  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
  if (! model) {
    return;
  }

  QModelIndex mi = model->locate_next ();
  if (mi.isValid ()) {

    m_cells_cb_enabled = false;
    mp_ui->lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::SelectCurrent);
    mp_ui->lv_cells->scrollTo (mi);
    update_children_list ();
    update_parents_list ();
    m_cells_cb_enabled = true;

  }
}

void
CellSelectionForm::find_prev_clicked ()
{
  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
  if (! model) {
    return;
  }

  QModelIndex mi = model->locate_prev ();
  if (mi.isValid ()) {

    m_cells_cb_enabled = false;
    mp_ui->lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::SelectCurrent);
    mp_ui->lv_cells->scrollTo (mi);
    update_children_list ();
    update_parents_list ();
    m_cells_cb_enabled = true;

  }
}

void
CellSelectionForm::name_changed ()
{
  if (m_name_cb_enabled) {

    QString s = mp_ui->le_cell_name->text ();

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
    if (! model) {
      return;
    }

    QModelIndex mi;
    if (!s.isEmpty ()) {
      mi = model->locate (tl::to_string (s).c_str (), mp_use_regular_expressions->isChecked (), mp_case_sensitive->isChecked ());
    } else {
      model->clear_locate ();
    }

    m_cells_cb_enabled = false;
    mp_ui->lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::SelectCurrent);
    if (mi.isValid ()) {
      mp_ui->lv_cells->scrollTo (mi);
    }
    update_children_list ();
    update_parents_list ();
    m_cells_cb_enabled = true;

  }
}

void
CellSelectionForm::show_cell ()
{
  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
  if (! model) {
    return;
  }
  if (m_current_cv < 0 || m_current_cv >= int (m_cellviews.size ())) {
    return;
  }

  QModelIndexList sel = mp_ui->lv_cells->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator s = sel.begin (); s != sel.end (); ++s) {
    db::cell_index_type ci = model->cell (*s)->cell_index ();
    if (mp_view->manager ()) {
      mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Show cells")));
    }
    mp_view->show_cell (ci, m_current_cv);
    if (mp_view->manager ()) {
      mp_view->manager ()->commit ();
    }
  }

  model->signal_data_changed ();
}

void
CellSelectionForm::hide_cell ()
{
  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
  if (! model) {
    return;
  }
  if (m_current_cv < 0 || m_current_cv >= int (m_cellviews.size ())) {
    return;
  }

  QModelIndexList sel = mp_ui->lv_cells->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator s = sel.begin (); s != sel.end (); ++s) {
    db::cell_index_type ci = model->cell (*s)->cell_index ();
    if (mp_view->manager ()) {
      mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Hide cells")));
    }
    mp_view->hide_cell (ci, m_current_cv);
    if (mp_view->manager ()) {
      mp_view->manager ()->commit ();
    }
  }

  model->signal_data_changed ();
}

// ------------------------------------------------------------

LibraryCellSelectionForm::LibraryCellSelectionForm (QWidget *parent, db::Layout *layout, const char *name, bool all_cells, bool top_cells_only, bool hide_private)
  : QDialog (parent),
    mp_lib (0), mp_layout (layout),
    m_name_cb_enabled (true),
    m_cells_cb_enabled (true),
    m_cell_index (-1),
    m_pcell_id (-1),
    m_is_pcell (false),
    m_all_cells (all_cells),
    m_top_cells_only (top_cells_only),
    m_hide_private (hide_private)
{
  mp_ui = new Ui::LibraryCellSelectionForm ();
  setObjectName (QString::fromUtf8 (name));

  mp_ui->setupUi (this);

  //  no library selection
  mp_ui->lib_label->hide ();
  mp_ui->lib_cb->hide ();

  //  signals and slots connections
  connect (mp_ui->cancel_button, SIGNAL(clicked()), this, SLOT(reject()));
  connect (mp_ui->ok_button, SIGNAL(clicked()), this, SLOT(accept()));
  connect (mp_ui->le_cell_name, SIGNAL(textChanged(const QString&)), this, SLOT(name_changed(const QString&)));
  connect (mp_ui->find_next, SIGNAL(clicked()), this, SLOT(find_next_clicked()));
  connect (mp_ui->cb_show_all_cells, SIGNAL(clicked()), this, SLOT(show_all_changed()));

  mp_ui->lv_cells->header ()->hide ();
  mp_ui->lv_cells->setRootIsDecorated (false);

  mp_ui->ok_button->setText (QObject::tr ("Ok"));
  mp_ui->cancel_button->setText (QObject::tr ("Cancel"));

  update_cell_list ();  
}

LibraryCellSelectionForm::LibraryCellSelectionForm (QWidget *parent, const char *name, bool all_cells, bool top_cells_only, bool hide_private)
  : QDialog (parent),
    mp_lib (0), mp_layout (0),
    m_name_cb_enabled (true),
    m_cells_cb_enabled (true),
    m_cell_index (-1),
    m_pcell_id (-1),
    m_is_pcell (false),
    m_all_cells (all_cells),
    m_top_cells_only (top_cells_only),
    m_hide_private (hide_private)
{
  mp_ui = new Ui::LibraryCellSelectionForm ();

  mp_lib = db::LibraryManager::instance ().lib_ptr_by_name ("Basic");
  mp_layout = &mp_lib->layout ();

  setObjectName (QString::fromUtf8 (name));

  mp_ui->setupUi (this);

  mp_ui->lib_cb->set_current_library (mp_lib);

  // signals and slots connections
  connect (mp_ui->cancel_button, SIGNAL(clicked()), this, SLOT(reject()));
  connect (mp_ui->ok_button, SIGNAL(clicked()), this, SLOT(accept()));
  connect (mp_ui->le_cell_name, SIGNAL(textChanged(const QString&)), this, SLOT(name_changed(const QString&)));
  connect (mp_ui->find_next, SIGNAL(clicked()), this, SLOT(find_next_clicked()));
  connect (mp_ui->lib_cb, SIGNAL(currentIndexChanged(int)), this, SLOT(lib_changed()));
  connect (mp_ui->cb_show_all_cells, SIGNAL(clicked()), this, SLOT(show_all_changed()));

  mp_ui->lv_cells->header ()->hide ();
  mp_ui->lv_cells->setRootIsDecorated (false);

  mp_ui->ok_button->setText (QObject::tr ("Ok"));
  mp_ui->cancel_button->setText (QObject::tr ("Cancel"));

  update_cell_list ();  
}

void
LibraryCellSelectionForm::show_all_changed ()
{
  m_all_cells = mp_ui->cb_show_all_cells->isChecked ();
  update_cell_list ();
}

void
LibraryCellSelectionForm::lib_changed ()
{
  mp_lib = mp_ui->lib_cb->current_library ();
  mp_layout = mp_lib ? &mp_lib->layout () : 0;
  update_cell_list ();  
}

void
LibraryCellSelectionForm::set_current_library (db::Library *lib)
{
  mp_lib = lib;
  mp_layout = mp_lib ? &mp_lib->layout () : 0;
  update_cell_list ();
}

void 
LibraryCellSelectionForm::set_selected_cell_index (db::cell_index_type ci)
{
  if (ci != m_cell_index || selected_cell_is_pcell ()) {
    m_cell_index = ci;
    m_pcell_id = 0;
    m_is_pcell = false;
    select_entry (m_cell_index);
  }
}

void 
LibraryCellSelectionForm::set_selected_pcell_id (db::pcell_id_type pci)
{
  if (pci != m_pcell_id || ! selected_cell_is_pcell ()) {
    m_cell_index = 0;
    m_pcell_id = pci;
    m_is_pcell = true;
    select_pcell_entry (m_pcell_id);
  }
}

void
LibraryCellSelectionForm::accept () 
{
BEGIN_PROTECTED
  if (! mp_layout) {
    throw tl::Exception (tl::to_string (QObject::tr ("No library selected")));
  }

  if (! m_is_pcell && ! mp_layout->is_valid_cell_index (m_cell_index)) {
    throw tl::Exception (tl::to_string (QObject::tr ("No cell selected")));
  }

  QDialog::accept ();

END_PROTECTED
}

void 
LibraryCellSelectionForm::update_cell_list () 
{
  if (mp_ui->lv_cells->model ()) {
    delete mp_ui->lv_cells->model ();
  }

  mp_ui->cb_show_all_cells->setChecked (m_all_cells);

  if (mp_layout) {

    unsigned int flags = lay::CellTreeModel::Flat;
    if (! m_all_cells) {
      flags |= lay::CellTreeModel::BasicCells;
      if (m_top_cells_only) {
        flags |= lay::CellTreeModel::TopCells;
      }
      if (m_hide_private) {
        flags |= lay::CellTreeModel::HidePrivate;
      }
    }

    //  TODO: get rid of that const_cast
    lay::CellTreeModel *model = new lay::CellTreeModel (mp_ui->lv_cells, const_cast<db::Layout *> (mp_layout), flags);
    
    mp_ui->lv_cells->setModel (model);
    //  connect can only happen after setModel()
    connect (mp_ui->lv_cells->selectionModel (), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(cell_changed(const QModelIndex &, const QModelIndex &)));

    select_entry (std::numeric_limits<db::cell_index_type>::max ());

  }
}

void 
LibraryCellSelectionForm::cell_changed (const QModelIndex &current, const QModelIndex &)
{
  if (m_cells_cb_enabled) {

    m_name_cb_enabled = false;

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
    if (model) {
      m_is_pcell = model->is_pcell (current);
      if (m_is_pcell) {
        m_pcell_id = model->pcell_id (current);
      } else {
        m_cell_index = model->cell_index (current);
      }
      mp_ui->le_cell_name->setText (tl::to_qstring (model->cell_name (current)));
      model->clear_locate ();
    } else {
      m_cell_index = -1;
      m_pcell_id = -1;
      m_is_pcell = false;
    }

    m_name_cb_enabled = true;

  }
}

void 
LibraryCellSelectionForm::select_pcell_entry (db::pcell_id_type pci)
{
  m_cells_cb_enabled = false;
  m_pcell_id = pci;
  m_is_pcell = true;

  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
  if (! model) {
    return;
  }

  //  select the current entry
  QModelIndex mi;
  for (int c = 0; c < model->toplevel_items (); ++c) {
    lay::CellTreeItem *item = model->toplevel_item (c);
    if (item->is_pcell () && item->cell_or_pcell_index () == pci) {
      mi = model->model_index (item);
      break;
    }
  }
        
  if (mi.isValid ()) {

    m_cells_cb_enabled = false;
    mp_ui->lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    mp_ui->lv_cells->scrollTo (mi);
    m_cells_cb_enabled = true;

    m_name_cb_enabled = false;
    mp_ui->le_cell_name->setText (tl::to_qstring (model->cell_name (mi)));
    model->clear_locate ();
    m_name_cb_enabled = true;

  }
  
  m_cells_cb_enabled = true;
}

void 
LibraryCellSelectionForm::select_entry (lay::CellView::cell_index_type ci)
{
  m_cells_cb_enabled = false;
  m_cell_index = ci;
  m_is_pcell = false;

  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
  if (! model) {
    return;
  }

  //  select the current entry
  QModelIndex mi;
  for (int c = 0; c < model->toplevel_items (); ++c) {
    lay::CellTreeItem *item = model->toplevel_item (c);
    if (item->cell_or_pcell_index () == ci) {
      mi = model->model_index (item);
      break;
    }
  }
        
  if (mi.isValid ()) {

    m_cells_cb_enabled = false;
    mp_ui->lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    mp_ui->lv_cells->scrollTo (mi);
    m_cells_cb_enabled = true;

    m_name_cb_enabled = false;
    mp_ui->le_cell_name->setText (tl::to_qstring (model->cell_name (mi)));
    model->clear_locate ();
    m_name_cb_enabled = true;

  }
  
  m_cells_cb_enabled = true;
}

void 
LibraryCellSelectionForm::find_next_clicked ()
{
  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
  if (! model) {
    return;
  }

  QModelIndex mi = model->locate_next ();
  if (mi.isValid ()) {

    m_cells_cb_enabled = false;
    mp_ui->lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::SelectCurrent);
    mp_ui->lv_cells->scrollTo (mi);

    m_is_pcell = model->is_pcell (mi);
    if (m_is_pcell) {
      m_pcell_id = model->pcell_id (mi);
    } else {
      m_cell_index = model->cell_index (mi);
    }

    m_cells_cb_enabled = true;

  } else {
    m_cell_index = -1;
    m_pcell_id = -1;
    m_is_pcell = false;
  }
}

void 
LibraryCellSelectionForm::name_changed (const QString &s)
{
  if (m_name_cb_enabled) {

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->lv_cells->model ());
    if (! model) {
      return;
    }

    QModelIndex mi = model->locate (tl::to_string (s).c_str (), true);
    if (mi.isValid ()) {

      m_cells_cb_enabled = false;
      mp_ui->lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::SelectCurrent);
      mp_ui->lv_cells->scrollTo (mi);

      m_is_pcell = model->is_pcell (mi);
      if (m_is_pcell) {
        m_pcell_id = model->pcell_id (mi);
      } else {
        m_cell_index = model->cell_index (mi);
      }

      m_cells_cb_enabled = true;

    } else {
      m_cell_index = -1;
      m_pcell_id = -1;
      m_is_pcell = false;
    }

  }
}

}

#endif

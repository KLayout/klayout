
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
#include "layLayoutView.h"
#include "tlExceptions.h"

#include <QHeaderView>

namespace lay
{

static const std::string cfg_cell_selection_search_case_sensitive ("cell-selection-search-case-sensitive");
static const std::string cfg_cell_selection_search_use_expressions ("cell-selection-search-use-expression");

// ------------------------------------------------------------

CellSelectionForm::CellSelectionForm (QWidget *parent, lay::LayoutView *view, const char *name, bool simple_mode)
  : QDialog (parent), Ui::CellSelectionForm (),
    mp_view (view),
    m_current_cv (-1),
    m_name_cb_enabled (true),
    m_cells_cb_enabled (true),
    m_children_cb_enabled (true),
    m_parents_cb_enabled (true),
    m_update_all_dm (this, &CellSelectionForm::update_all),
    m_simple_mode (simple_mode)
{
  setObjectName (QString::fromUtf8 (name));

  Ui::CellSelectionForm::setupUi (this);

  le_cell_name->set_tab_signal_enabled (true);

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

  QMenu *m = new QMenu (le_cell_name);
  m->addAction (mp_use_regular_expressions);
  m->addAction (mp_case_sensitive);
  connect (mp_use_regular_expressions, SIGNAL (triggered ()), this, SLOT (name_changed ()));
  connect (mp_case_sensitive, SIGNAL (triggered ()), this, SLOT (name_changed ()));

  le_cell_name->set_clear_button_enabled (true);
  le_cell_name->set_options_button_enabled (true);
  le_cell_name->set_options_menu (m);


  // signals and slots connections
  connect (cancel_button, SIGNAL(clicked()), this, SLOT(reject()));
  connect (cb_views, SIGNAL(activated(int)), this, SLOT(view_changed(int)));
  connect (tb_set_parent, SIGNAL(clicked()), this, SLOT(set_parent()));
  connect (tb_set_child, SIGNAL(clicked()), this, SLOT(set_child()));
  connect (pb_hide, SIGNAL(clicked()), this, SLOT(hide_cell()));
  connect (pb_show, SIGNAL(clicked()), this, SLOT(show_cell()));
  connect (le_cell_name, SIGNAL(textChanged(const QString&)), this, SLOT(name_changed()));
  connect (ok_button, SIGNAL(clicked()), this, SLOT(accept()));
  connect (apply_button, SIGNAL(clicked()), this, SLOT(apply_clicked()));
  connect (find_next, SIGNAL(clicked()), this, SLOT(find_next_clicked()));
  connect (le_cell_name, SIGNAL(tab_pressed()), this, SLOT(find_next_clicked()));
  connect (le_cell_name, SIGNAL(backtab_pressed()), this, SLOT(find_prev_clicked()));

  connect (lv_parents, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(parent_changed(const QModelIndex &)));
  connect (lv_children, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(child_changed(const QModelIndex &)));


  m_cellviews.reserve (mp_view->cellviews ());
  for (unsigned int i = 0; i < mp_view->cellviews (); ++i) {
    m_cellviews.push_back (mp_view->cellview (i));
  }

  if (simple_mode) {
    apply_button->hide ();
    tools_frame->hide ();
  } else {
    apply_button->show ();
    tools_frame->show ();
  }

  if (! m_cellviews.empty ()) {

    m_current_cv = view->active_cellview_index ();

    int cvi = 0;
    for (std::vector<lay::CellView>::const_iterator cv = m_cellviews.begin (); cv != m_cellviews.end (); ++cv, ++cvi) {
      cb_views->addItem (tl::to_qstring ((*cv)->name () + " (@" + tl::to_string (cvi + 1) + ")"));
    }
    cb_views->setCurrentIndex (m_current_cv);

    if (m_cellviews.size () == 1) {
      cb_views->hide ();
      layout_lbl->hide ();
    } else {
      cb_views->show ();
      layout_lbl->show ();
    }

    lv_cells->header ()->hide ();
    lv_cells->setRootIsDecorated (false);

    lv_children->header ()->hide ();
    lv_children->setRootIsDecorated (false);

    lv_parents->header ()->hide ();
    lv_parents->setRootIsDecorated (false);
      
    update_cell_list ();  

  }

}

void 
CellSelectionForm::update_cell_list () 
{
  if (m_current_cv < 0 || m_current_cv >= int (m_cellviews.size ())) {
    return;
  }

  if (lv_cells->model ()) {
    delete lv_cells->model ();
  }

  lay::CellTreeModel *model = new lay::CellTreeModel (lv_cells, mp_view, m_current_cv, lay::CellTreeModel::Flat);
  
  lv_cells->setModel (model);
  //  connect can only happen after setModel()
  connect (lv_cells->selectionModel (), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(cell_changed(const QModelIndex &, const QModelIndex &)));

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

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
    if (model) {

      if (lv_parents->model ()) {
        delete lv_parents->model ();
      }
      lv_parents->setModel (new lay::CellTreeModel (lv_parents, mp_view, m_current_cv, lay::CellTreeModel::Flat | lay::CellTreeModel::Parents, model->cell (lv_cells->selectionModel ()->currentIndex ())));

    }

  }

  m_parents_cb_enabled = true;
}

void 
CellSelectionForm::update_children_list ()
{
  m_children_cb_enabled = false;
  
  if (m_current_cv >= 0 && m_current_cv < int (m_cellviews.size ())) {

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
    if (model) {

      if (lv_children->model ()) {
        delete lv_children->model ();
      }
      lv_children->setModel (new lay::CellTreeModel (lv_children, mp_view, m_current_cv, lay::CellTreeModel::Flat | lay::CellTreeModel::Children, model->cell (lv_cells->selectionModel ()->currentIndex ())));

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

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
    if (! model) {
      return;
    }

    const db::Cell *cell = model->cell (lv_cells->selectionModel ()->currentIndex ());
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

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
    if (! model) {
      return;
    }

    const db::Cell *cell = model->cell (lv_cells->selectionModel ()->currentIndex ());

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

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
    if (model) {
      le_cell_name->setText (tl::to_qstring (model->cell_name (current)));
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
  child_changed (lv_children->selectionModel ()->currentIndex ());
}

void 
CellSelectionForm::child_changed(const QModelIndex &current)
{
  if (m_children_cb_enabled && current.isValid ()) {
    if (m_current_cv >= 0 && m_current_cv < int (m_cellviews.size ())) {
      lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_children->model ());
      if (model) {
        select_entry (model->cell_index (lv_children->selectionModel ()->currentIndex ()));
      }
    }
  }
}

void
CellSelectionForm::set_parent ()
{
  parent_changed (lv_parents->selectionModel ()->currentIndex ());
}

void 
CellSelectionForm::parent_changed(const QModelIndex &current)
{
  if (m_parents_cb_enabled && current.isValid ()) {
    if (m_current_cv >= 0 && m_current_cv < int (m_cellviews.size ())) {
      lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_parents->model ());
      if (model) {
        select_entry (model->cell_index (lv_parents->selectionModel ()->currentIndex ()));
      }
    }
  }
}

void 
CellSelectionForm::select_entry (lay::CellView::cell_index_type ci)
{
  m_cells_cb_enabled = false;

  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
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
    lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    lv_cells->scrollTo (mi);
    m_cells_cb_enabled = true;

    m_name_cb_enabled = false;
    le_cell_name->setText (tl::to_qstring (model->cell_name (mi)));
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
  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
  if (! model) {
    return;
  }

  QModelIndex mi = model->locate_next ();
  if (mi.isValid ()) {

    m_cells_cb_enabled = false;
    lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::SelectCurrent);
    lv_cells->scrollTo (mi);
    update_children_list ();
    update_parents_list ();
    m_cells_cb_enabled = true;

  }
}

void
CellSelectionForm::find_prev_clicked ()
{
  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
  if (! model) {
    return;
  }

  QModelIndex mi = model->locate_prev ();
  if (mi.isValid ()) {

    m_cells_cb_enabled = false;
    lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::SelectCurrent);
    lv_cells->scrollTo (mi);
    update_children_list ();
    update_parents_list ();
    m_cells_cb_enabled = true;

  }
}

void
CellSelectionForm::name_changed ()
{
  if (m_name_cb_enabled) {

    QString s = le_cell_name->text ();

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
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
    lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::SelectCurrent);
    if (mi.isValid ()) {
      lv_cells->scrollTo (mi);
    }
    update_children_list ();
    update_parents_list ();
    m_cells_cb_enabled = true;

  }
}

void
CellSelectionForm::show_cell ()
{
  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
  if (! model) {
    return;
  }
  if (m_current_cv < 0 || m_current_cv >= int (m_cellviews.size ())) {
    return;
  }

  QModelIndexList sel = lv_cells->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator s = sel.begin (); s != sel.end (); ++s) {
    db::cell_index_type ci = model->cell (*s)->cell_index ();
    mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Show cells")));
    mp_view->show_cell (ci, m_current_cv);
    mp_view->manager ()->commit ();
  }

  model->signal_data_changed ();
}

void
CellSelectionForm::hide_cell ()
{
  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
  if (! model) {
    return;
  }
  if (m_current_cv < 0 || m_current_cv >= int (m_cellviews.size ())) {
    return;
  }

  QModelIndexList sel = lv_cells->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator s = sel.begin (); s != sel.end (); ++s) {
    db::cell_index_type ci = model->cell (*s)->cell_index ();
    mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Hide cells")));
    mp_view->hide_cell (ci, m_current_cv);
    mp_view->manager ()->commit ();
  }

  model->signal_data_changed ();
}

// ------------------------------------------------------------

LibraryCellSelectionForm::LibraryCellSelectionForm (QWidget *parent, db::Layout *layout, const char *name, bool all_cells)
  : QDialog (parent), Ui::LibraryCellSelectionForm (),
    mp_lib (0), mp_layout (layout),
    m_name_cb_enabled (true),
    m_cells_cb_enabled (true),
    m_cell_index (-1),
    m_pcell_id (-1),
    m_is_pcell (false),
    m_all_cells (all_cells)
{
  setObjectName (QString::fromUtf8 (name));

  Ui::LibraryCellSelectionForm::setupUi (this);

  //  no library selection
  lib_label->hide ();
  lib_cb->hide ();

  //  signals and slots connections
  connect (cancel_button, SIGNAL(clicked()), this, SLOT(reject()));
  connect (ok_button, SIGNAL(clicked()), this, SLOT(accept()));
  connect (le_cell_name, SIGNAL(textChanged(const QString&)), this, SLOT(name_changed(const QString&)));
  connect (find_next, SIGNAL(clicked()), this, SLOT(find_next_clicked()));
  connect (cb_show_all_cells, SIGNAL(clicked()), this, SLOT(show_all_changed()));

  lv_cells->header ()->hide ();
  lv_cells->setRootIsDecorated (false);

  ok_button->setText (QObject::tr ("Ok"));
  cancel_button->setText (QObject::tr ("Cancel"));

  update_cell_list ();  
}

LibraryCellSelectionForm::LibraryCellSelectionForm (QWidget *parent, const char *name, bool all_cells)
  : QDialog (parent), Ui::LibraryCellSelectionForm (),
    mp_lib (0), mp_layout (0),
    m_name_cb_enabled (true),
    m_cells_cb_enabled (true),
    m_cell_index (-1),
    m_pcell_id (-1),
    m_is_pcell (false),
    m_all_cells (all_cells)
{
  mp_lib = db::LibraryManager::instance ().lib_ptr_by_name ("Basic");
  mp_layout = &mp_lib->layout ();

  setObjectName (QString::fromUtf8 (name));

  Ui::LibraryCellSelectionForm::setupUi (this);

  lib_cb->set_current_library (mp_lib);

  // signals and slots connections
  connect (cancel_button, SIGNAL(clicked()), this, SLOT(reject()));
  connect (ok_button, SIGNAL(clicked()), this, SLOT(accept()));
  connect (le_cell_name, SIGNAL(textChanged(const QString&)), this, SLOT(name_changed(const QString&)));
  connect (find_next, SIGNAL(clicked()), this, SLOT(find_next_clicked()));
  connect (lib_cb, SIGNAL(currentIndexChanged(int)), this, SLOT(lib_changed()));
  connect (cb_show_all_cells, SIGNAL(clicked()), this, SLOT(show_all_changed()));

  lv_cells->header ()->hide ();
  lv_cells->setRootIsDecorated (false);

  ok_button->setText (QObject::tr ("Ok"));
  cancel_button->setText (QObject::tr ("Cancel"));

  update_cell_list ();  
}

void
LibraryCellSelectionForm::show_all_changed ()
{
  m_all_cells = cb_show_all_cells->isChecked ();
  update_cell_list ();
}

void
LibraryCellSelectionForm::lib_changed ()
{
  mp_lib = lib_cb->current_library ();
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
  if (lv_cells->model ()) {
    delete lv_cells->model ();
  }

  cb_show_all_cells->setChecked (m_all_cells);

  if (mp_layout) {

    //  TODO: get rid of that const_cast
    lay::CellTreeModel *model = new lay::CellTreeModel (lv_cells, const_cast<db::Layout *> (mp_layout), lay::CellTreeModel::Flat | (m_all_cells ? 0 : (lay::CellTreeModel::TopCells | lay::CellTreeModel::BasicCells)));
    
    lv_cells->setModel (model);
    //  connect can only happen after setModel()
    connect (lv_cells->selectionModel (), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(cell_changed(const QModelIndex &, const QModelIndex &)));

    select_entry (std::numeric_limits<db::cell_index_type>::max ());

  }
}

void 
LibraryCellSelectionForm::cell_changed (const QModelIndex &current, const QModelIndex &)
{
  if (m_cells_cb_enabled) {

    m_name_cb_enabled = false;

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
    if (model) {
      m_is_pcell = model->is_pcell (current);
      if (m_is_pcell) {
        m_pcell_id = model->pcell_id (current);
      } else {
        m_cell_index = model->cell_index (current);
      }
      le_cell_name->setText (tl::to_qstring (model->cell_name (current)));
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

  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
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
    lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    lv_cells->scrollTo (mi);
    m_cells_cb_enabled = true;

    m_name_cb_enabled = false;
    le_cell_name->setText (tl::to_qstring (model->cell_name (mi)));
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

  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
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
    lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    lv_cells->scrollTo (mi);
    m_cells_cb_enabled = true;

    m_name_cb_enabled = false;
    le_cell_name->setText (tl::to_qstring (model->cell_name (mi)));
    model->clear_locate ();
    m_name_cb_enabled = true;

  }
  
  m_cells_cb_enabled = true;
}

void 
LibraryCellSelectionForm::find_next_clicked ()
{
  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
  if (! model) {
    return;
  }

  QModelIndex mi = model->locate_next ();
  if (mi.isValid ()) {

    m_cells_cb_enabled = false;
    lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::SelectCurrent);
    lv_cells->scrollTo (mi);

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

    lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (lv_cells->model ());
    if (! model) {
      return;
    }

    QModelIndex mi = model->locate (tl::to_string (s).c_str (), true);
    if (mi.isValid ()) {

      m_cells_cb_enabled = false;
      lv_cells->selectionModel ()->setCurrentIndex (mi, QItemSelectionModel::SelectCurrent);
      lv_cells->scrollTo (mi);

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



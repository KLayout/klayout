
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

#include "layPropertiesDialog.h"

#include "tlLog.h"
#include "layEditable.h"
#include "layProperties.h"
#include "layDispatcher.h"
#include "laybasicConfig.h"
#include "tlExceptions.h"

#include "ui_PropertiesDialog.h"

#include <QStackedLayout>
#include <QAbstractItemModel>
#include <QModelIndex>

namespace lay
{

#if QT_VERSION >= 0x50000
typedef qint64 tree_id_type;
#else
typedef qint32 tree_id_type;
#endif

// ----------------------------------------------------------------------------------------------------------
//  PropertiesTreeModel definition and implementation

class PropertiesTreeModel
  : public QAbstractItemModel
{
public:
  PropertiesTreeModel (PropertiesDialog *dialog, int icon_width, int icon_height)
    : QAbstractItemModel (dialog), mp_dialog (dialog), m_icon_width (icon_width), m_icon_height (icon_height)
  { }

  int columnCount (const QModelIndex &) const
  {
    return 1;
  }

  QVariant data (const QModelIndex &index, int role) const
  {
    if (role == Qt::DisplayRole) {
      if (tree_id_type (index.internalId ()) < tree_id_type (mp_dialog->properties_pages ().size ())) {
        return tl::to_qstring (mp_dialog->properties_pages () [index.internalId ()]->description (index.row ()));
      } else if (index.row () < int (mp_dialog->properties_pages ().size ())) {
        return tl::to_qstring (mp_dialog->properties_pages () [index.row ()]->description ());
      }
    } else if (role == Qt::DecorationRole) {
      QIcon icon;
      if (tree_id_type (index.internalId ()) < tree_id_type (mp_dialog->properties_pages ().size ())) {
        icon = mp_dialog->properties_pages () [index.internalId ()]->icon (index.row (), m_icon_width, m_icon_height);
      } else if (index.row () < int (mp_dialog->properties_pages ().size ())) {
        icon = mp_dialog->properties_pages () [index.row ()]->icon (m_icon_width, m_icon_height);
      }
      if (! icon.isNull ()) {
        return QVariant (icon);
      }
    }
    return QVariant ();
  }

  Qt::ItemFlags flags (const QModelIndex &index) const
  {
    Qt::ItemFlags f = QAbstractItemModel::flags (index);
    if (tree_id_type (index.internalId ()) >= tree_id_type (mp_dialog->properties_pages ().size ()) && ! mp_dialog->properties_pages () [index.row ()]->can_apply_to_all ()) {
      f &= ~Qt::ItemIsSelectable;
    }
    return f;
  }

  bool hasChildren (const QModelIndex &parent) const
  {
    return (! parent.isValid () || tree_id_type (parent.internalId ()) >= tree_id_type (mp_dialog->properties_pages ().size ()));
  }

  QModelIndex index (int row, int column, const QModelIndex &parent) const
  {
    if (! parent.isValid ()) {
      return createIndex (row, column, tree_id_type (mp_dialog->properties_pages ().size ()));
    } else {
      return createIndex (row, column, tree_id_type (parent.row ()));
    }
  }

  QModelIndex parent (const QModelIndex &child) const
  {
    if (tree_id_type (child.internalId ()) < tree_id_type (mp_dialog->properties_pages ().size ())) {
      return createIndex (int (child.internalId ()), child.column (), tree_id_type (mp_dialog->properties_pages ().size ()));
    } else {
      return QModelIndex ();
    }
  }

  int rowCount (const QModelIndex &parent) const
  {
    if (! hasChildren (parent)) {
      return 0;
    } else if (parent.isValid ()) {
      return int (mp_dialog->properties_pages () [parent.row ()]->count ());
    } else {
      return int (mp_dialog->properties_pages ().size ());
    }
  }

  int page_index (const QModelIndex &index)
  {
    return int (index.internalId ());
  }

  int object_index (const QModelIndex &index)
  {
    return int (index.row ());
  }

  QModelIndex index_for (int page_index, int object_index)
  {
    if (page_index < 0) {
      return QModelIndex ();
    } else {
      return createIndex (object_index, 0, tree_id_type (page_index));
    }
  }

  QModelIndex index_for (int page_index)
  {
    if (page_index < 0) {
      return QModelIndex ();
    } else {
      return createIndex (page_index, 0, tree_id_type (mp_dialog->properties_pages ().size ()));
    }
  }

  void emit_data_changed ()
  {
    int cc = columnCount (QModelIndex ());
    int rc = rowCount (QModelIndex ());

    emit dataChanged (index (0, 0, QModelIndex ()), index (rc - 1, cc - 1, QModelIndex ()));

    //  data changes for the children too
    for (int i = 0; i < rc; ++i) {
      auto p = index (i, 0, QModelIndex ());
      int ec = rowCount (p);
      emit dataChanged (index (0, 0, p), index (ec - 1, cc - 1, p));
    }
  }

  void begin_reset_model ()
  {
    beginResetModel ();
  }

  void end_reset_model ()
  {
    endResetModel ();
  }

private:
  PropertiesDialog *mp_dialog;
  int m_icon_width, m_icon_height;
};

// ----------------------------------------------------------------------------------------------------------
//  PropertiesDialog

PropertiesDialog::PropertiesDialog (QWidget * /*parent*/, db::Manager *manager, lay::Editables *editables)
  : QDialog (0 /*parent*/),
    mp_manager (manager),
    mp_editables (editables),
    m_index (0), m_prev_index (-1),
    m_auto_applied (false),
    m_transaction_id (0),
    m_signals_enabled (true)
{
  mp_ui = new Ui::PropertiesDialog ();

  setObjectName (QString::fromUtf8 ("properties_dialog"));
  mp_ui->setupUi (this);
  mp_tree_model = 0;

  mp_editables->enable_edits (false);

  mp_stack = new QStackedLayout;

  for (lay::Editables::iterator e = mp_editables->begin (); e != mp_editables->end (); ++e) {
    auto pp = e->properties_pages (mp_manager, mp_ui->content_frame);
    for (auto p = pp.begin (); p != pp.end (); ++p) {
      if ((*p)->count () == 0) {
        delete *p;
      } else {
        mp_properties_pages.push_back (*p);
        (*p)->set_page_set (this);
      }
    }
  }
  for (size_t i = 0; i < mp_properties_pages.size (); ++i) {
    mp_stack->addWidget (mp_properties_pages [i]);
    connect (mp_properties_pages [i], SIGNAL (edited ()), this, SLOT (properties_edited ()));
  }

  //  Add a label as a dummy 
  mp_none = new QLabel (QObject::tr ("No object with properties to display"), mp_ui->content_frame);
  mp_none->setAlignment (Qt::AlignHCenter | Qt::AlignVCenter);
  mp_stack->addWidget (mp_none);

  mp_ui->content_frame->setLayout (mp_stack);

  //  count the total number of objects
  m_objects = mp_editables->selection_size ();
  m_current_object = 0;

  //  look for next usable editable 
  m_object_indexes.resize (mp_properties_pages.size ());
  if (m_index >= int (mp_properties_pages.size ())) {
    m_index = -1;
  } else {
    m_object_indexes [m_index].push_back (0);
  }

  update_title ();

  mp_tree_model = new PropertiesTreeModel (this, mp_ui->tree->iconSize ().width (), mp_ui->tree->iconSize ().height ());
  mp_ui->tree->setModel (mp_tree_model);
#if QT_VERSION >= 0x50000
  mp_ui->tree->header()->setSectionResizeMode (QHeaderView::ResizeToContents);
#else
  mp_ui->tree->header()->setResizeMode (QHeaderView::ResizeToContents);
#endif
  mp_ui->tree->expandAll ();

  mp_ui->tree->addAction (mp_ui->action_reduce_selection);

  if (mp_properties_pages.empty ()) {
    mp_ui->tree->hide ();
  }

  m_signals_enabled = false;
  mp_ui->tree->setCurrentIndex (mp_tree_model->index_for (m_index, 0));
  m_signals_enabled = true;

  mp_ui->apply_to_all_cbx->setChecked (true); // TODO: persist
  mp_ui->relative_cbx->setChecked (true);

  fetch_config ();
  update_controls ();

  connect (mp_ui->ok_button, SIGNAL (clicked ()), this, SLOT (ok_pressed ()));
  connect (mp_ui->cancel_button, SIGNAL (clicked ()), this, SLOT (cancel_pressed ()));
  connect (mp_ui->prev_button, SIGNAL (clicked ()), this, SLOT (prev_pressed ()));
  connect (mp_ui->next_button, SIGNAL (clicked ()), this, SLOT (next_pressed ()));
  connect (mp_ui->apply_to_all_cbx, SIGNAL (clicked ()), this, SLOT (apply_to_all_pressed ()));
  connect (mp_ui->action_reduce_selection, SIGNAL (triggered ()), this, SLOT (reduce_selection ()));
  connect (mp_ui->tree->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (current_index_changed (const QModelIndex &, const QModelIndex &)));
  connect (mp_ui->tree->selectionModel (), SIGNAL (selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT (selection_changed ()));
}

PropertiesDialog::~PropertiesDialog ()
{
  delete mp_ui;
  mp_ui = 0;

  disconnect ();
}

void
PropertiesDialog::fetch_config ()
{
  if (! lay::Dispatcher::instance ()) {
    return;
  }

  bool rm = true;
  lay::Dispatcher::instance ()->config_get (cfg_properties_dialog_relative_mode, rm);
  mp_ui->relative_cbx->setChecked (rm);
}

void
PropertiesDialog::store_config ()
{
  if (lay::Dispatcher::instance ()) {
    lay::Dispatcher::instance ()->config_set (cfg_properties_dialog_relative_mode, mp_ui->relative_cbx->isChecked ());
  }
}

void 
PropertiesDialog::disconnect ()
{
  mp_editables->enable_edits (true);
  
  for (std::vector <lay::PropertiesPage *>::iterator p = mp_properties_pages.begin (); p != mp_properties_pages.end (); ++p) {
    delete *p;
  }
  mp_properties_pages.clear ();
  m_index = -1;
}

void
PropertiesDialog::apply_to_all_pressed ()
{
  mp_ui->relative_cbx->setEnabled (mp_ui->apply_to_all_cbx->isEnabled () && mp_ui->apply_to_all_cbx->isChecked ());
}

void
PropertiesDialog::reduce_selection ()
{
BEGIN_PROTECTED

  //  apply pending changes
  if (m_index >= 0 && m_index < int (mp_properties_pages.size ()) && ! mp_properties_pages [m_index]->readonly ()) {

    db::Transaction t (mp_manager, tl::to_string (QObject::tr ("Apply changes")), m_transaction_id);

    mp_properties_pages [m_index]->apply (true);
    mp_properties_pages [m_index]->update ();

    if (! t.is_empty ()) {
      m_transaction_id = t.id ();
    }

  }

  //  confine the selection

  auto prev_selected = mp_editables->selection_size ();

  mp_tree_model->begin_reset_model ();

  auto selection = mp_ui->tree->selectionModel ()->selectedIndexes ();

  for (std::vector <lay::PropertiesPage *>::iterator p = mp_properties_pages.begin (); p != mp_properties_pages.end (); ++p) {

    int page_index = int (p - mp_properties_pages.begin ());
    std::vector<size_t> object_indexes;

    for (auto i = selection.begin (); i != selection.end (); ++i) {
      if (mp_tree_model->parent (*i).isValid () && mp_tree_model->page_index (*i) == page_index) {
        object_indexes.push_back (size_t (mp_tree_model->object_index (*i)));
      }
    }

    (*p)->confine_selection (object_indexes);

  }

  m_signals_enabled = false;

  //  rebuild the properties pages as only the ones with remaining selection are shown

  for (std::vector <lay::PropertiesPage *>::iterator p = mp_properties_pages.begin (); p != mp_properties_pages.end (); ++p) {
    delete *p;
  }
  mp_properties_pages.clear ();
  m_index = -1;

  for (lay::Editables::iterator e = mp_editables->begin (); e != mp_editables->end (); ++e) {
    auto pp = e->properties_pages (mp_manager, mp_ui->content_frame);
    for (auto p = pp.begin (); p != pp.end (); ++p) {
      if ((*p)->count () == 0) {
        delete *p;
      } else {
        mp_properties_pages.push_back (*p);
        (*p)->set_page_set (this);
      }
    }
  }
  for (size_t i = 0; i < mp_properties_pages.size (); ++i) {
    mp_stack->addWidget (mp_properties_pages [i]);
    connect (mp_properties_pages [i], SIGNAL (edited ()), this, SLOT (properties_edited ()));
  }

  //  count the total number of objects
  m_objects = mp_editables->selection_size ();
  m_current_object = 0;

  //  look for next usable editable
  m_index = 0;
  m_object_indexes.clear ();
  m_object_indexes.resize (mp_properties_pages.size ());
  if (m_index >= int (mp_properties_pages.size ())) {
    m_index = -1;
  } else {
    m_object_indexes [m_index].push_back (0);
  }

  mp_tree_model->end_reset_model ();

  update_title ();

  mp_ui->tree->expandAll ();
  mp_ui->tree->setCurrentIndex (mp_tree_model->index_for (m_index, 0));

  m_signals_enabled = true;

  update_controls ();

  if (m_objects != prev_selected) {
    mp_editables->signal_selection_changed ();
  }

END_PROTECTED
}

void
PropertiesDialog::selection_changed ()
{
  current_index_changed (mp_ui->tree->currentIndex (), QModelIndex ());
}

void
PropertiesDialog::current_index_changed (const QModelIndex &index, const QModelIndex & /*previous*/)
{
  if (! m_signals_enabled) {
    return;
  }

  m_object_indexes.clear ();
  m_object_indexes.resize (mp_properties_pages.size ());

  if (! index.isValid ()) {

    m_index = -1;

  } else {

    if (m_index >= 0 && m_index < int (mp_properties_pages.size ()) && ! mp_properties_pages [m_index]->readonly ()) {

      try {

        db::Transaction t (mp_manager, tl::to_string (QObject::tr ("Apply changes")), m_transaction_id);

        mp_properties_pages [m_index]->apply (true);

        if (! t.is_empty ()) {
          m_transaction_id = t.id ();
        }

      } catch (...) {
      }

      mp_properties_pages [m_index]->update ();

    }

    m_index = -1;

    auto selection = mp_ui->tree->selectionModel ()->selectedIndexes ();

    //  establish a single-selection on the current item
    if (mp_tree_model->parent (index).isValid ()) {
      int oi = mp_tree_model->object_index (index);
      int pi = mp_tree_model->page_index (index);
      m_index = pi;
      m_object_indexes [pi].push_back (size_t (oi));
    }

    //  establish individual selections for the other items
    //  as far as allowed by "can_apply_to_all"
    for (auto i = selection.begin (); i != selection.end (); ++i) {
      if (*i != index && mp_tree_model->parent (*i).isValid ()) {
        int oi = mp_tree_model->object_index (*i);
        int pi = mp_tree_model->page_index (*i);
        if (mp_properties_pages [pi]->can_apply_to_all ()) {
          m_object_indexes [pi].push_back (size_t (oi));
        }
      }
    }

    //  establish group node selection as current item -> translate into "all" for "can_apply_to_all" pages
    //  or first item unless an item is explicitly selected.
    if (! mp_tree_model->parent (index).isValid ()) {
      int pi = index.row ();
      m_index = pi;
      m_object_indexes [pi].clear ();
      if (mp_properties_pages [pi]->can_apply_to_all ()) {
        for (size_t oi = 0; oi < mp_properties_pages [pi]->count (); ++oi) {
          m_object_indexes [pi].push_back (oi);
        }
      } else if (mp_properties_pages [pi]->count () > 0 && m_object_indexes [pi].empty ()) {
        m_object_indexes [pi].push_back (0);
      }
    }

    //  establish group node selection for other items -> translate into "all" for "can_apply_to_all" pages
    for (auto i = selection.begin (); i != selection.end (); ++i) {
      if (*i != index && ! mp_tree_model->parent (*i).isValid ()) {
        int pi = i->row ();
        m_object_indexes [pi].clear ();
        if (mp_properties_pages [pi]->can_apply_to_all ()) {
          for (size_t oi = 0; oi < mp_properties_pages [pi]->count (); ++oi) {
            m_object_indexes[pi].push_back (oi);
          }
        }
      }
    }

  }

  if (m_index >= 0 && ! m_object_indexes [m_index].empty ()) {
    m_current_object = 0;
    for (int i = 0; i < m_index; ++i) {
      m_current_object += mp_properties_pages [i]->count ();
    }
    m_current_object += int (m_object_indexes [m_index].front ());
  } else {
    m_current_object = -1;
  }

  update_title ();
  update_controls ();
}

void
PropertiesDialog::update_controls ()
{
  if (m_prev_index >= 0 && m_index != m_prev_index) {
    if (m_prev_index >= 0 && m_prev_index < int (mp_properties_pages.size ())) {
      mp_properties_pages [m_prev_index]->leave ();
    }
  }
  m_prev_index = m_index;

  if (m_index < 0 || m_index >= int (mp_properties_pages.size ())) {

    mp_stack->setCurrentWidget (mp_none);

    mp_ui->prev_button->setEnabled (false);
    mp_ui->next_button->setEnabled (false);
    mp_ui->apply_to_all_cbx->setEnabled (false);
    mp_ui->relative_cbx->setEnabled (false);
    mp_ui->ok_button->setEnabled (false);
    mp_ui->tree->setEnabled (false);

  } else {

    mp_stack->setCurrentWidget (mp_properties_pages [m_index]);

    mp_ui->prev_button->setEnabled (any_prev ());
    mp_ui->next_button->setEnabled (any_next ());
    mp_ui->apply_to_all_cbx->setEnabled (! mp_properties_pages [m_index]->readonly () && mp_properties_pages [m_index]->can_apply_to_all ());
    mp_ui->relative_cbx->setEnabled (mp_ui->apply_to_all_cbx->isEnabled () && mp_ui->apply_to_all_cbx->isChecked ());
    mp_ui->ok_button->setEnabled (! mp_properties_pages [m_index]->readonly ());
    mp_ui->tree->setEnabled (true);

    for (int i = 0; i < int (mp_properties_pages.size ()); ++i) {
      mp_properties_pages [i]->select_entries (m_object_indexes [i]);
    }

    mp_properties_pages [m_index]->update ();

  }
}

void 
PropertiesDialog::next_pressed ()
{
BEGIN_PROTECTED

  if (m_index < 0 || m_object_indexes [m_index].empty ()) {
    return;
  }

  if (! mp_properties_pages [m_index]->readonly ()) {
    db::Transaction t (mp_manager, tl::to_string (QObject::tr ("Apply changes")), m_transaction_id);
    mp_properties_pages [m_index]->apply (true);
    if (! t.is_empty ()) {
      m_transaction_id = t.id ();
    }
  }

  //  advance the current entry
  int object_index = int (m_object_indexes [m_index].front ());
  ++object_index;

  //  look for next usable editable if at end
  if (object_index >= int (mp_properties_pages [m_index]->count ())) {

    ++m_index;
    object_index = 0;

    //  because we checked that there are any further elements, this should not happen:
    if (m_index >= int (mp_properties_pages.size ())) {
      return;
    }

  }

  m_object_indexes.clear ();
  m_object_indexes.resize (mp_properties_pages.size ());
  m_object_indexes [m_index].push_back (object_index);

  ++m_current_object;
  update_title ();

  update_controls ();
  m_signals_enabled = false;
  mp_ui->tree->setCurrentIndex (mp_tree_model->index_for (m_index, object_index));
  m_signals_enabled = true;

END_PROTECTED
}

void 
PropertiesDialog::prev_pressed ()
{
BEGIN_PROTECTED

  if (m_index < 0 || m_object_indexes [m_index].empty ()) {
    return;
  }

  if (! mp_properties_pages [m_index]->readonly ()) {
    db::Transaction t (mp_manager, tl::to_string (QObject::tr ("Apply changes")), m_transaction_id);
    mp_properties_pages [m_index]->apply (true);
    if (! t.is_empty ()) {
      m_transaction_id = t.id ();
    }
  }

  //  advance the current entry
  int object_index = int (m_object_indexes [m_index].front ());
  if (object_index == 0) {

    //  look for last usable editable if at end
    --m_index;

    //  because we checked that there are any further elements, this should not happen:
    if (m_index < 0) {
      return;
    }

    object_index = mp_properties_pages [m_index]->count ();

  } 

  //  decrement the current entry
  --object_index;

  m_object_indexes.clear ();
  m_object_indexes.resize (mp_properties_pages.size ());
  m_object_indexes [m_index].push_back (object_index);

  --m_current_object;
  update_title ();

  update_controls ();
  m_signals_enabled = false;
  mp_ui->tree->setCurrentIndex (mp_tree_model->index_for (m_index, object_index));
  m_signals_enabled = true;

END_PROTECTED
}

void
PropertiesDialog::update_title ()
{
  if (m_index < 0) {
    setWindowTitle (QObject::tr ("Object Properties"));
  } else {
    setWindowTitle (tl::to_qstring (tl::to_string (QObject::tr ("Object Properties - ")) + tl::to_string (m_current_object + 1) + tl::to_string (QObject::tr (" of ")) + tl::to_string (m_objects)));
  }
}

bool
PropertiesDialog::any_next () const
{
  if (m_index < 0 || m_object_indexes [m_index].empty ()) {
    return false;
  }

  int index = m_index;
  if (m_object_indexes [m_index].front () + 1 >= mp_properties_pages [index]->count ()) {
    ++index;
  }

  //  return true, if not at end
  return (index < int (mp_properties_pages.size ()));
}

bool
PropertiesDialog::any_prev () const
{
  if (m_index < 0 || m_object_indexes [m_index].empty ()) {
    return false;
  }

  int index = m_index;
  if (m_object_indexes [m_index].front () == 0) {
    --index;
  }

  //  return true, if not at the beginning
  return (index >= 0);
}

void
PropertiesDialog::properties_edited ()
{
BEGIN_PROTECTED

  if (m_index < 0 || m_index >= int (mp_properties_pages.size ())) {
    return;
  }

  db::Transaction t (mp_manager, tl::to_string (QObject::tr ("Apply changes")), m_transaction_id);

  try {

    if (mp_ui->apply_to_all_cbx->isChecked () && mp_properties_pages [m_index]->can_apply_to_all ()) {
      mp_properties_pages [m_index]->apply_to_all (mp_ui->relative_cbx->isChecked (), false);
    } else {
      mp_properties_pages [m_index]->apply (false);
    }
    mp_properties_pages [m_index]->update ();

  } catch (tl::Exception &) {
    //  we assume the page somehow indicates the error and does not apply the values
  }

  //  remember transaction ID for undo on "Cancel" unless nothing happened
  if (! t.is_empty ()) {
    m_transaction_id = t.id ();
  }

  //  updates cell names in instances for example
  mp_tree_model->emit_data_changed ();

END_PROTECTED
}

void 
PropertiesDialog::cancel_pressed ()
{
  //  undo whatever we've done so far
  if (m_transaction_id > 0) {

    //  because undo does not maintain a valid selection we clear it
    mp_editables->clear_selection ();

    if (mp_manager->transaction_id_for_undo () == m_transaction_id) {
      mp_manager->undo ();
    }
    m_transaction_id = 0;

  }

  store_config ();
  //  make sure that the property pages are no longer used ..
  disconnect ();
  //  close the dialog
  done (0);
}

void 
PropertiesDialog::ok_pressed ()
{
BEGIN_PROTECTED

  if (m_index >= 0 && m_index < int (mp_properties_pages.size ()) && ! mp_properties_pages [m_index]->readonly ()) {

    db::Transaction t (mp_manager, tl::to_string (QObject::tr ("Apply changes")), m_transaction_id);

    mp_properties_pages [m_index]->apply (true);
    mp_properties_pages [m_index]->update ();

    if (! t.is_empty ()) {
      m_transaction_id = t.id ();
    }

  }

  store_config ();
  //  make sure that the property pages are no longer used ..
  disconnect ();
  QDialog::accept ();

END_PROTECTED
}

void 
PropertiesDialog::reject ()
{
  store_config ();
  //  make sure that the property pages are no longer used ..
  disconnect ();
  QDialog::reject ();
}

void
PropertiesDialog::accept ()
{
  //  stop handling "Enter" key.
}

}

#endif

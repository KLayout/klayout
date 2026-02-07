
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


#include "layMacroEditorTree.h"
#include "layMacroEditorDialog.h"
#include "lymMacroCollection.h"
#include "tlExceptions.h"
#include "tlInternational.h"
#include "tlException.h"

#include <QDropEvent>
#include <QMimeData>
#include <QSortFilterProxyModel>

#include <cstdio>

namespace lay
{

// -----------------------------------------------------------------------------------------
//  A model for the macro tree

static QIcon tree_icon_for_format (const lym::Macro *m, bool active)
{
  //  TODO: create a nice icon for the DSL interpreted scripts
  if (m->interpreter () == lym::Macro::Text) {
    return QIcon (QString::fromUtf8 (":/textdocumenticon_16px.png"));
  } else if (m->interpreter () == lym::Macro::Ruby) {
    if (m->format () == lym::Macro::PlainTextFormat || m->format () == lym::Macro::PlainTextWithHashAnnotationsFormat) {
      if (active) {
        return QIcon (QString::fromUtf8 (":/rubymacroiconactive_16px.png"));
      } else {
        return QIcon (QString::fromUtf8 (":/rubymacroicon_16px.png"));
      }
    } else {
      if (active) {
        return QIcon (QString::fromUtf8 (":/generalmacroiconactive_16px.png"));
      } else {
        return QIcon (QString::fromUtf8 (":/generalmacroicon_16px.png"));
      }
    }
  } else if (m->interpreter () == lym::Macro::Python) {
    if (m->format () == lym::Macro::PlainTextFormat || m->format () == lym::Macro::PlainTextWithHashAnnotationsFormat) {
      if (active) {
        return QIcon (QString::fromUtf8 (":/pythonmacroiconactive_16px.png"));
      } else {
        return QIcon (QString::fromUtf8 (":/pythonmacroicon_16px.png"));
      }
    } else {
      if (active) {
        return QIcon (QString::fromUtf8 (":/generalmacroiconactive_16px.png"));
      } else {
        return QIcon (QString::fromUtf8 (":/generalmacroicon_16px.png"));
      }
    }
  } else {
    return QIcon (QString::fromUtf8 (":/defaultmacroicon_16px.png"));
  }
}

struct FilteredMacroCollectionIter
{
  FilteredMacroCollectionIter (const lym::MacroCollection *mc, const std::string &cat)
    : m_b (mc->begin_children ()), m_e (mc->end_children ()), m_category (cat)
  {
    next ();
  }

  bool at_end () const
  {
    return m_b == m_e;
  }

  std::pair<const std::string, lym::MacroCollection *> operator* () const
  {
    return m_b.operator* ();
  }

  const std::pair<const std::string, lym::MacroCollection *> *operator-> () const
  {
    return m_b.operator-> ();
  }

  void operator++ () 
  {
    ++m_b;
    next ();
  }

private:
  lym::MacroCollection::const_child_iterator m_b, m_e;
  std::string m_category;

  void next ()
  {
    while (m_b != m_e && (! m_b->second->category ().empty () && m_b->second->category () != m_category)) {
      ++m_b;
    }
  }
};

MacroTreeModel::MacroTreeModel (QObject *parent, lay::MacroEditorDialog *dialog, lym::MacroCollection *root, const std::string &cat)
  : QAbstractItemModel (parent), mp_dialog (dialog), mp_parent (dialog), mp_root (root), m_category (cat)
{
  connect (root, SIGNAL (macro_changed (lym::Macro *)), this, SLOT (macro_changed ()));
  connect (root, SIGNAL (macro_about_to_be_deleted (lym::Macro *)), this, SLOT (macro_about_to_be_deleted (lym::Macro *)));
  connect (root, SIGNAL (macro_deleted (lym::Macro *)), this, SLOT (macro_deleted (lym::Macro *)));
  connect (root, SIGNAL (macro_collection_about_to_be_deleted (lym::MacroCollection *)), this, SLOT (macro_collection_about_to_be_deleted (lym::MacroCollection *)));
  connect (root, SIGNAL (macro_collection_deleted (lym::MacroCollection *)), this, SLOT (macro_collection_deleted (lym::MacroCollection *)));
  connect (root, SIGNAL (macro_collection_changed (lym::MacroCollection *)), this, SLOT (macro_collection_changed ()));
  connect (root, SIGNAL (about_to_change ()), this, SLOT (about_to_change ()));
}

MacroTreeModel::MacroTreeModel (QWidget *parent, lym::MacroCollection *root, const std::string &cat)
  : QAbstractItemModel (parent), mp_dialog (0), mp_parent (parent), mp_root (root), m_category (cat)
{
  connect (root, SIGNAL (macro_changed (lym::Macro *)), this, SLOT (macro_changed ()));
  connect (root, SIGNAL (macro_about_to_be_deleted (lym::Macro *)), this, SLOT (macro_about_to_be_deleted (lym::Macro *)));
  connect (root, SIGNAL (macro_deleted (lym::Macro *)), this, SLOT (macro_deleted (lym::Macro *)));
  connect (root, SIGNAL (macro_collection_about_to_be_deleted (lym::MacroCollection *)), this, SLOT (macro_collection_about_to_be_deleted (lym::MacroCollection *)));
  connect (root, SIGNAL (macro_collection_deleted (lym::MacroCollection *)), this, SLOT (macro_collection_deleted (lym::MacroCollection *)));
  connect (root, SIGNAL (macro_collection_changed (lym::MacroCollection *)), this, SLOT (macro_collection_changed ()));
  connect (root, SIGNAL (about_to_change ()), this, SLOT (about_to_change ()));
}

Qt::DropActions MacroTreeModel::supportedDropActions() const
{
  return Qt::MoveAction;
}

void MacroTreeModel::macro_about_to_be_deleted (lym::Macro *macro)
{
  QModelIndex index = index_for (macro);
  if (index.isValid ()) {
    changePersistentIndex (index, QModelIndex ());
  }
}

void MacroTreeModel::macro_deleted (lym::Macro *)
{
  //  .. nothing yet ..
}

void MacroTreeModel::macro_collection_about_to_be_deleted (lym::MacroCollection *mc)
{
  QModelIndex index = index_for (mc);
  if (index.isValid ()) {
    changePersistentIndex (index, QModelIndex ());
  }
}

void MacroTreeModel::macro_collection_deleted (lym::MacroCollection *)
{
  //  .. nothing yet ..
}

void MacroTreeModel::macro_changed ()
{
  update_data ();
}

void MacroTreeModel::update_data ()
{
  int rc = rowCount (QModelIndex());
  if (rc > 0) {
    emit dataChanged (index (0, 0, QModelIndex ()), index (rc - 1, 0, QModelIndex ()));
  }
}

void MacroTreeModel::about_to_change ()
{
  emit layoutAboutToBeChanged ();
}

void MacroTreeModel::macro_collection_changed ()
{
  invalidate_cache ();

  //  rewrite the persistent Indexes
  QModelIndexList pi = persistentIndexList ();
  for (QModelIndexList::const_iterator i = pi.begin (); i != pi.end (); ++i) {
    if (is_valid_pointer (i->internalPointer ())) {
      lym::Macro *macro = dynamic_cast <lym::Macro *> ((QObject *) i->internalPointer ());
      lym::MacroCollection *mc = dynamic_cast <lym::MacroCollection *> ((QObject *) i->internalPointer ());
      if (macro) {
        changePersistentIndex (*i, index_for (macro));
      } else if (mc) {
        changePersistentIndex (*i, index_for (mc));
      } else {
        changePersistentIndex (*i, QModelIndex ());
      }
    } else {
      changePersistentIndex (*i, QModelIndex ());
    }
  }

  emit layoutChanged ();
}

void MacroTreeModel::invalidate_cache ()
{
  m_valid_objects.clear ();
}

bool MacroTreeModel::is_valid_pointer (void *ptr) const
{
  if (m_valid_objects.empty ()) {

    std::set<lym::Macro *> macros;
    std::set<lym::MacroCollection *> macro_collections;
    mp_root->collect_used_nodes (macros, macro_collections);
    for (std::set<lym::Macro *>::const_iterator m = macros.begin (); m != macros.end (); ++m) {
      m_valid_objects.insert ((void *)(QObject *)*m);
    }
    for (std::set<lym::MacroCollection *>::const_iterator m = macro_collections.begin (); m != macro_collections.end (); ++m) {
      m_valid_objects.insert ((void *)(QObject *)*m);
    }

  }

  return m_valid_objects.find (ptr) != m_valid_objects.end ();
}

QStringList MacroTreeModel::mimeTypes () const
{
  QStringList types;
  types << QString::fromUtf8 ("application/klayout-macros.list");
  return types;
}

QMimeData *MacroTreeModel::mimeData(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream (&encodedData, QIODevice::WriteOnly);
  stream << (quintptr) this;

  for (QModelIndexList::const_iterator i = indexes.begin (); i != indexes.end (); ++i) {
    if (i->isValid()) {
      stream << (quintptr) i->internalPointer ();
    }
  }

  mimeData->setData (QString::fromUtf8 ("application/klayout-macros.list"), encodedData);
  return mimeData;
}

bool MacroTreeModel::dropMimeData (const QMimeData *data, Qt::DropAction /*action*/, int /*row*/, int /*column*/, const QModelIndex &parent)
{
  QByteArray encodedData = data->data (QString::fromUtf8 ("application/klayout-macros.list"));
  QDataStream stream (&encodedData, QIODevice::ReadOnly);

  quintptr owner = 0;
  stream >> owner;
  if (owner != (quintptr) this) {
    return false;
  }

  if (! parent.isValid () || ! is_valid_pointer (parent.internalPointer ())) {
    return false;
  } 
  
  lym::MacroCollection *to_mc = dynamic_cast <lym::MacroCollection *> ((QObject *) parent.internalPointer ());
  if (! to_mc) {
    return false;
  }

  while (! stream.atEnd ()) {

    quintptr p = 0;
    stream >> p;

    if (is_valid_pointer ((void *) p)) {

      QObject *from_object = (QObject *) (void *) p;
      lym::Macro *from_macro = dynamic_cast <lym::Macro *> (from_object);
      lym::MacroCollection *from_mc = dynamic_cast <lym::MacroCollection *> (from_object);

      if (from_macro) {
        emit move_macro (from_macro, to_mc);
      } else if (from_mc) {
        emit move_folder (from_mc, to_mc);
      }

    } 

  }

  return true;
}

int MacroTreeModel::columnCount (const QModelIndex & /*parent*/) const
{
  return 1;
}

bool MacroTreeModel::setData (const QModelIndex &index, const QVariant &v, int role) 
{
  if (! index.isValid () || role != Qt::UserRole || ! is_valid_pointer (index.internalPointer ())) {
    return false;
  } 
  
  QObject *object = (QObject *) index.internalPointer ();
  lym::Macro *macro = dynamic_cast <lym::Macro *> (object);
  lym::MacroCollection *mc = dynamic_cast <lym::MacroCollection *> (object);

  //  TODO: don't do this while executing
  if (macro) {
    if (macro->parent () && macro->parent ()->macro_by_name (tl::to_string (v.toString ()), macro->format ()) != 0) {
      //  a macro with that name already exists - do nothing
      return false;
    }
    if (macro->rename (tl::to_string (v.toString ()))) {
      emit (macro_renamed (macro));
      return true;
    } else {
      return false;
    }
  } else if (mc) {
    if (mc->parent () && mc->parent ()->folder_by_name (tl::to_string (v.toString ())) != 0) {
      //  a folder with that name already exists - do nothing
      return false;
    }
    if (mc->rename (tl::to_string (v.toString ()))) {
      emit (folder_renamed (mc));
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

QVariant MacroTreeModel::data (const QModelIndex &index, int role) const
{
  if (! index.isValid () || ! is_valid_pointer (index.internalPointer ())) {
    return QVariant ();
  } 
  
  const QObject *object = (QObject *) index.internalPointer ();
  const lym::Macro *macro = dynamic_cast <const lym::Macro *> (object);
  const lym::MacroCollection *mc = dynamic_cast <const lym::MacroCollection *> (object);
  if (macro) {
    if (role == Qt::DisplayRole) {
      return QVariant (tl::to_qstring (macro->display_string ()));
    } else if (role == Qt::DecorationRole) {
      return QVariant (tree_icon_for_format (macro, mp_dialog && macro == mp_dialog->run_macro ()));
    } else if (role == Qt::ToolTipRole) {
      return QVariant (tl::to_qstring (macro->path ()));
    } else if (role == Qt::UserRole) {
      return QVariant (tl::to_qstring (macro->name ()));
    } else if (role == Qt::FontRole) {
      QFont f = mp_parent->font ();
      f.setItalic (macro->is_readonly ());
      f.setBold (! macro->is_file () || macro->is_modified ());
      return QVariant (f);
    } else {
      return QVariant ();
    }
  } else if (mc) {
    if (role == Qt::DisplayRole) {
      return QVariant (tl::to_qstring (mc->display_string ()));
    } else if (role == Qt::DecorationRole) {
      return QVariant (QIcon (QString::fromUtf8 (":/folder_16px.png")));
    } else if (role == Qt::ToolTipRole) {
      return QVariant (tl::to_qstring (mc->path ()));
    } else if (role == Qt::UserRole) {
      return QVariant (tl::to_qstring (mc->name ()));
    } else if (role == Qt::FontRole) {
      QFont f = mp_parent->font ();
      f.setItalic (mc->is_readonly ());
      return QVariant (f);
    } else {
      return QVariant ();
    }
  } else {
    return QVariant ();
  }
}

Qt::ItemFlags MacroTreeModel::flags (const QModelIndex &index) const
{
  if ((mp_dialog && mp_dialog->in_exec ()) || ! index.isValid () || ! is_valid_pointer (index.internalPointer ())) {
    return QAbstractItemModel::flags (index);
  }

  const QObject *object = (QObject *) index.internalPointer ();
  const lym::Macro *macro = dynamic_cast <const lym::Macro *> (object);
  const lym::MacroCollection *mc = dynamic_cast <const lym::MacroCollection *> (object);
  if (macro) {
    if (! macro->is_readonly ()) {
      return QAbstractItemModel::flags (index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled;
    } else {
      return QAbstractItemModel::flags (index) | Qt::ItemIsDragEnabled;
    }
  } else if (mc) {
    if (!mc->is_readonly () && !mc->virtual_mode ()) {
      return QAbstractItemModel::flags (index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    } else if (!mc->is_readonly () && mc->virtual_mode ()) {
      return QAbstractItemModel::flags (index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    } else {
      return QAbstractItemModel::flags (index) | Qt::ItemIsDragEnabled;
    }
  } 
  return QAbstractItemModel::flags (index);
}

bool MacroTreeModel::hasChildren (const QModelIndex &parent) const
{
  const lym::MacroCollection *mc = 0;
  if (! parent.isValid ()) {
    mc = mp_root;
  } else {
    if (! is_valid_pointer (parent.internalPointer ())) {
      return false;
    }
    mc = dynamic_cast <const lym::MacroCollection *> ((QObject *) parent.internalPointer ());
  }
  if (mc) {
    return ! FilteredMacroCollectionIter (mc, m_category).at_end () || mc->begin () != mc->end ();
  } else {
    return false;
  }
}

QModelIndex MacroTreeModel::parent (const QModelIndex &index) const
{
  if (! is_valid_pointer (index.internalPointer ())) {
    return QModelIndex ();
  }

  const QObject *object = (QObject *) index.internalPointer ();
  const lym::Macro *macro = dynamic_cast <const lym::Macro *> (object);
  const lym::MacroCollection *mc = dynamic_cast <const lym::MacroCollection *> (object);

  const lym::MacroCollection *p = 0;
  if (macro) {
    p = macro->parent ();
  } else if (mc) {
    p = mc->parent ();
  }

  if (p) {
    const lym::MacroCollection *pp = p->parent ();
    if (pp) {
      int row = 0;
      for (FilteredMacroCollectionIter i (pp, m_category); ! i.at_end (); ++i, ++row) {
        if (i->second == p) {
          return createIndex (row, index.column (), (void *)(QObject *)p);
        }
      }
    }
  }

  return QModelIndex ();
}

QModelIndex MacroTreeModel::index (int row, int column, const QModelIndex &parent) const
{
  const lym::MacroCollection *mc = 0;
  if (! parent.isValid ()) {
    mc = mp_root;
  } else {
    if (! is_valid_pointer (parent.internalPointer ())) {
      return QModelIndex ();
    }
    mc = dynamic_cast <const lym::MacroCollection *> ((QObject *) parent.internalPointer ());
  }

  if (mc) {
    FilteredMacroCollectionIter i (mc, m_category);
    int r = row;
    while (! i.at_end ()) {
      if (r-- <= 0) {
        return createIndex (row, column, (void *)(QObject *)i->second);
      }
      ++i;
    }
    lym::MacroCollection::const_iterator j = mc->begin ();
    while (j != mc->end ()) {
      if (r-- <= 0) {
        return createIndex (row, column, (void *)(QObject *)j->second);
      }
      ++j;
    }
  }

  return QModelIndex ();
}

int MacroTreeModel::rowCount (const QModelIndex &parent) const 
{
  const lym::MacroCollection *mc = 0;
  if (! parent.isValid ()) {
    mc = mp_root;
  } else {
    if (! is_valid_pointer (parent.internalPointer ())) {
      return 0;
    }
    mc = dynamic_cast <const lym::MacroCollection *> ((QObject *) parent.internalPointer ());
  }

  int n = 0;
  if (mc) {
    for (FilteredMacroCollectionIter i (mc, m_category); ! i.at_end (); ++i) {
      ++n;
    }
    for (lym::MacroCollection::const_iterator i = mc->begin (); i != mc->end (); ++i) {
      ++n;
    }
  }
  return n;
}

QModelIndex 
MacroTreeModel::index_for (lym::Macro *macro) const
{
  if (! macro || ! macro->parent ()) {
    return QModelIndex ();
  }

  //  check category
  const lym::MacroCollection *pp = macro->parent ();
  while (pp && (pp->category ().empty () || pp->category () == m_category)) {
    pp = pp->parent ();
  }
  if (pp) {
    return QModelIndex ();
  }

  //  determine index
  pp = macro->parent ();
  int row = 0;
  for (FilteredMacroCollectionIter i (pp, m_category); ! i.at_end (); ++i, ++row) {
    ; 
  }
  for (lym::MacroCollection::const_iterator i = pp->begin (); i != pp->end (); ++i, ++row) {
    if (i->second == macro) {
      return createIndex (row, 0, (void *)(QObject *)macro);
    }
  }
   
  return QModelIndex ();
}

QModelIndex 
MacroTreeModel::index_for (lym::MacroCollection *mc) const
{
  if (! mc || ! mc->parent ()) {
    return QModelIndex ();
  }

  //  check category
  const lym::MacroCollection *pp = mc;
  while (pp && (pp->category ().empty () || pp->category () == m_category)) {
    pp = pp->parent ();
  }
  if (pp) {
    return QModelIndex ();
  }

  //  determine index
  pp = mc->parent ();
  int row = 0;
  for (FilteredMacroCollectionIter i (pp, m_category); ! i.at_end (); ++i, ++row) {
    if (i->second == mc) {
      return createIndex (row, 0, (void *)(QObject *)mc);
    }
  }
   
  return QModelIndex ();
}

// -----------------------------------------------------------------------------------------
//  The macro tree

MacroEditorTree::MacroEditorTree (QWidget *parent, const std::string &cat)
  : QTreeView (parent), m_category (cat)
{
  mp_proxyModel = 0;
  mp_model = 0;

  setDragDropMode (QAbstractItemView::InternalMove);
  setDragEnabled (true);
  setAcceptDrops (true);
  setDropIndicatorShown (true);
  setIconSize (QSize (16, 16));
}

void MacroEditorTree::model_macro_renamed (lym::Macro *macro)
{
  set_current (macro);
  emit macro_renamed (macro);
}

void MacroEditorTree::model_folder_renamed (lym::MacroCollection *mc)
{
  set_current (mc);
  emit folder_renamed (mc);
}

void MacroEditorTree::model_move_macro (lym::Macro *source, lym::MacroCollection *target)
{
  emit move_macro (source, target);
}

void MacroEditorTree::model_move_folder (lym::MacroCollection *source, lym::MacroCollection *target)
{
  emit move_folder (source, target);
}

lym::Macro *MacroEditorTree::current_macro () const
{
  QModelIndex ci = mp_proxyModel->mapToSource (currentIndex ());
  if (ci.isValid () && mp_model->is_valid_pointer (ci.internalPointer ())) {
    return dynamic_cast <lym::Macro *> ((QObject *) ci.internalPointer ());
  } else {
    return 0; 
  }
}

lym::MacroCollection *MacroEditorTree::current_macro_collection () const
{
  QModelIndex ci = mp_proxyModel->mapToSource (currentIndex ());
  if (ci.isValid () && mp_model->is_valid_pointer (ci.internalPointer ())) {
    return dynamic_cast <lym::MacroCollection *> ((QObject *) ci.internalPointer ());
  } else {
    return 0; 
  }
}

bool MacroEditorTree::set_current (lym::Macro *macro)
{
  QModelIndex index = mp_proxyModel->mapFromSource (mp_model->index_for (macro));
  setCurrentIndex (index);
  if (index.isValid ()) {
    scrollTo (index);
    return true;
  } else {
    return false;
  }
}

bool MacroEditorTree::set_current (lym::MacroCollection *mc)
{
  QModelIndex index = mp_proxyModel->mapFromSource (mp_model->index_for (mc));
  setCurrentIndex (index);
  if (index.isValid ()) {
    scrollTo (index);
    return true;
  } else {
    return false;
  }
}

void MacroEditorTree::setup (lay::MacroEditorDialog *dialog)
{
  mp_model = new MacroTreeModel (this, dialog, &lym::MacroCollection::root (), m_category);
  mp_proxyModel = new QSortFilterProxyModel (this);
  mp_proxyModel->setSourceModel (mp_model);
  setModel (mp_proxyModel);

  connect (this, SIGNAL (doubleClicked (const QModelIndex &)), this, SLOT (double_clicked_slot (const QModelIndex &)));
  connect (mp_model, SIGNAL (macro_renamed (lym::Macro *)), this, SLOT (model_macro_renamed (lym::Macro *)));
  connect (mp_model, SIGNAL (folder_renamed (lym::MacroCollection *)), this, SLOT (model_folder_renamed (lym::MacroCollection *)));
  connect (mp_model, SIGNAL (move_macro (lym::Macro *, lym::MacroCollection *)), this, SLOT (model_move_macro (lym::Macro *, lym::MacroCollection *)));
  connect (mp_model, SIGNAL (move_folder (lym::MacroCollection *, lym::MacroCollection *)), this, SLOT (model_move_folder (lym::MacroCollection *, lym::MacroCollection *)));
}

void MacroEditorTree::double_clicked_slot (const QModelIndex &index)
{
  QModelIndex i = mp_proxyModel->mapToSource (index);

  if (mp_model->is_valid_pointer (i.internalPointer ())) {

    QObject *object = (QObject *) i.internalPointer ();
    lym::Macro *macro = dynamic_cast <lym::Macro *> (object);
    lym::MacroCollection *mc = dynamic_cast <lym::MacroCollection *> (object);

    if (macro) {
      emit macro_double_clicked (macro);
    } else if (mc) {
      emit macro_collection_double_clicked (mc);
    }

  }
}

}


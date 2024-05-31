
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


#ifndef HDR_layMacroEditorTree
#define HDR_layMacroEditorTree

#include "lymMacro.h"

#include <QTreeView>

#include <set>

class QSortFilterProxyModel;

namespace lay
{

class MacroEditorDialog;

class MacroTreeModel 
  : public QAbstractItemModel
{
Q_OBJECT

public:
  MacroTreeModel (QObject *parent, lay::MacroEditorDialog *dialog, lym::MacroCollection *root, const std::string &cat);
  MacroTreeModel (QWidget *parent, lym::MacroCollection *root, const std::string &cat);

  int columnCount (const QModelIndex &parent) const;
  QVariant data (const QModelIndex &index, int role) const;
  Qt::ItemFlags flags (const QModelIndex &index) const;
  bool hasChildren (const QModelIndex &parent) const;
  QModelIndex index (int row, int column, const QModelIndex &parent) const;
  QModelIndex parent (const QModelIndex &index) const;
  int rowCount (const QModelIndex &parent) const;
  bool setData (const QModelIndex &index, const QVariant &v, int role);
  Qt::DropActions supportedDropActions() const;
  QModelIndex index_for (lym::Macro *macro) const;
  QModelIndex index_for (lym::MacroCollection *mc) const;

  void update_data ();
  bool is_valid_pointer (void *) const;

signals:
  void macro_renamed (lym::Macro *macro);
  void folder_renamed (lym::MacroCollection *folder);
  void move_macro (lym::Macro *source, lym::MacroCollection *target);
  void move_folder (lym::MacroCollection *source, lym::MacroCollection *target);

private slots:
  void macro_changed ();
  void macro_about_to_be_deleted (lym::Macro *macro);
  void macro_deleted (lym::Macro *macro);
  void macro_collection_about_to_be_deleted (lym::MacroCollection *mc);
  void macro_collection_deleted (lym::MacroCollection *mc);
  void macro_collection_changed ();
  void about_to_change ();

private:
  void invalidate_cache ();
  QMimeData *mimeData (const QModelIndexList &indexes) const;
  QStringList mimeTypes () const;
  bool dropMimeData (const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

  lay::MacroEditorDialog *mp_dialog;
  QWidget *mp_parent;
  lym::MacroCollection *mp_root;
  std::string m_category;
  mutable std::set <void *> m_valid_objects;
};

class MacroEditorTree 
  : public QTreeView
{
Q_OBJECT

public:
  MacroEditorTree (QWidget *parent, const std::string &cat);

  void setup (lay::MacroEditorDialog *dialog);
  lym::Macro *current_macro () const;
  lym::MacroCollection *current_macro_collection () const;

  bool set_current (lym::Macro *macro);
  bool set_current (lym::MacroCollection *mc);

  void update_data ()
  {
    mp_model->update_data ();
  }

  QModelIndex index_for (lym::Macro *macro) const
  {
    return mp_model->index_for (macro);
  }

  QModelIndex index_for (lym::MacroCollection *mc) const
  {
    return mp_model->index_for (mc);
  }

  const std::string &category () const
  {
    return m_category;
  }

signals:
  void move_macro (lym::Macro *source, lym::MacroCollection *target);
  void move_folder (lym::MacroCollection *source, lym::MacroCollection *target);
  void macro_double_clicked (lym::Macro *macro);
  void macro_collection_double_clicked (lym::MacroCollection *mc);
  void macro_renamed (lym::Macro *macro);
  void folder_renamed (lym::MacroCollection *folder);

private:
  QSortFilterProxyModel *mp_proxyModel;
  MacroTreeModel *mp_model;
  std::string m_category;

private slots:
  void double_clicked_slot (const QModelIndex &index);
  void model_move_macro (lym::Macro *source, lym::MacroCollection *target);
  void model_move_folder (lym::MacroCollection *source, lym::MacroCollection *target);
  void model_macro_renamed (lym::Macro *macro);
  void model_folder_renamed (lym::MacroCollection *folder);
};

}

#endif


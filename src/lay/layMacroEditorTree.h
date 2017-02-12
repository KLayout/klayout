
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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

#include <QTreeView>

#include <set>

class QSortFilterProxyModel;

namespace lay
{

class Macro;
class MacroCollection;
class MacroEditorDialog;

class MacroTreeModel 
  : public QAbstractItemModel
{
Q_OBJECT

public:
  MacroTreeModel (QObject *parent, lay::MacroEditorDialog *dialog, lay::MacroCollection *root, const std::string &cat);
  MacroTreeModel (QWidget *parent, lay::MacroCollection *root, const std::string &cat);

  int	columnCount (const QModelIndex &parent) const;
  QVariant data (const QModelIndex &index, int role) const;
  Qt::ItemFlags flags (const QModelIndex &index) const;
  bool hasChildren (const QModelIndex &parent) const;
  QModelIndex index (int row, int column, const QModelIndex &parent) const;
  QModelIndex parent (const QModelIndex &index) const;
  int rowCount (const QModelIndex &parent) const;
  bool setData (const QModelIndex &index, const QVariant &v, int role);
  Qt::DropActions supportedDropActions() const;
  QModelIndex index_for (lay::Macro *macro) const;
  QModelIndex index_for (lay::MacroCollection *mc) const;

  void update_data ();
  bool is_valid_pointer (void *) const;

signals:
  void macro_renamed (lay::Macro *macro);
  void folder_renamed (lay::MacroCollection *folder);
  void move_macro (lay::Macro *source, lay::MacroCollection *target);
  void move_folder (lay::MacroCollection *source, lay::MacroCollection *target);

private slots:
  void macro_changed ();
  void macro_deleted (Macro *macro);
  void macro_collection_deleted (MacroCollection *mc);
  void macro_collection_changed ();
  void about_to_change ();

private:
  void invalidate_cache ();
  QMimeData *mimeData (const QModelIndexList &indexes) const;
  QStringList mimeTypes () const;
  bool dropMimeData (const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

  lay::MacroEditorDialog *mp_dialog;
  QWidget *mp_parent;
  lay::MacroCollection *mp_root;
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
  lay::Macro *current_macro () const;
  lay::MacroCollection *current_macro_collection () const;

  bool set_current (lay::Macro *macro);
  bool set_current (lay::MacroCollection *mc); 

  void update_data ()
  {
    mp_model->update_data ();
  }

  QModelIndex index_for (lay::Macro *macro) const
  {
    return mp_model->index_for (macro);
  }

  QModelIndex index_for (lay::MacroCollection *mc) const
  {
    return mp_model->index_for (mc);
  }

  const std::string &category () const
  {
    return m_category;
  }

signals:
  void move_macro (lay::Macro *source, lay::MacroCollection *target);
  void move_folder (lay::MacroCollection *source, lay::MacroCollection *target);
  void macro_double_clicked (lay::Macro *macro);
  void macro_collection_double_clicked (lay::MacroCollection *mc);
  void macro_renamed (lay::Macro *macro);
  void folder_renamed (lay::MacroCollection *folder);

private:
  QSortFilterProxyModel *mp_proxyModel;
  MacroTreeModel *mp_model;
  std::string m_category;

private slots:
  void double_clicked_slot (const QModelIndex &index);
  void model_move_macro (lay::Macro *source, lay::MacroCollection *target);
  void model_move_folder (lay::MacroCollection *source, lay::MacroCollection *target);
  void model_macro_renamed (lay::Macro *macro);
  void model_folder_renamed (lay::MacroCollection *folder);
};

}

#endif


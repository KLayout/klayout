
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

#ifndef HDR_layPropertiesDialog
#define HDR_layPropertiesDialog

#include "layuiCommon.h"

#include <vector>

#include <QMutex>
#include <QDialog>
#include <QLabel>
#include <QStatusBar>
#include <QMessageBox>

#include <dbManager.h>

#include <memory>

class QStackedLayout;
class QModelIndex;

namespace Ui
{
  class PropertiesDialog;
}

namespace lay
{

class Editable;
class Editables;
class PropertiesPage;
class MainWindow;
class PropertiesTreeModel;

/**
 *  @brief The properties dialog
 *
 *  This is the implementation of the properties dialog that
 *  is opened to edit or view the properties of a set of selected
 *  objects from a set of editables.
 */

class LAYUI_PUBLIC PropertiesDialog
  : public QDialog
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  PropertiesDialog (QWidget *parent, db::Manager *manager, lay::Editables *editables);

  /**
   *  @brief The Destructor
   */
  ~PropertiesDialog ();

private:
  friend class PropertiesTreeModel;

  std::vector<lay::PropertiesPage *> mp_properties_pages;
  db::Manager *mp_manager;
  lay::Editables *mp_editables;
  int m_index, m_prev_index;
  std::vector<size_t> m_object_indexes;
  QStackedLayout *mp_stack;
  QLabel *mp_none;
  lay::MainWindow *mp_mw;
  size_t m_objects, m_current_object;
  bool m_auto_applied;
  db::Manager::transaction_id_t m_transaction_id;
  PropertiesTreeModel *mp_tree_model;
  bool m_signals_enabled;

  const std::vector<lay::PropertiesPage *> &properties_pages () { return mp_properties_pages; }
  void disconnect ();
  bool any_prev () const;
  bool any_next () const;
  void update_title ();
  void update_controls ();

public slots:
  void apply ();
  void next_pressed ();
  void prev_pressed ();
  void cancel_pressed ();
  void ok_pressed ();
  void apply_to_all_pressed ();
  void current_index_changed (const QModelIndex &index, const QModelIndex &previous);
  void selection_changed ();

protected:
  void reject ();

private:
  Ui::PropertiesDialog *mp_ui;
};

}

#endif

#endif  //  defined(HAVE_QT)

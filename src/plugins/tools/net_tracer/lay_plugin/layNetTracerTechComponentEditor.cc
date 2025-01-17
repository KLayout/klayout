
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


#include "layNetTracerTechComponentEditor.h"
#include "layNetTracerConfig.h"

#include "layConfigurationDialog.h"
#include "laybasicConfig.h"
#include "layConverters.h"
#include "layFinder.h"
#include "layLayoutView.h"
#include "layTechSetupDialog.h"
#include "layFileDialog.h"
#include "layQtTools.h"
#include "tlExceptions.h"
#include "tlXMLWriter.h"
#include "tlUtils.h"
#include "gsiDecl.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QItemDelegate>
#include <QHeaderView>
#include <QPainter>

#include <fstream>
#include <sstream>

namespace lay
{

// -----------------------------------------------------------------------------------------
//  NetTracerTechComponentColumnDelegate definition and implementation

class NetTracerTechComponentColumnDelegate
  : public QItemDelegate
{
public:
  NetTracerTechComponentColumnDelegate (QWidget *parent, db::NetTracerTechnologyComponent *data)
    : QItemDelegate (parent), mp_data (data)
  {
    //  .. nothing yet ..
  }

  QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
  {
    return new QLineEdit (parent);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
  {
    editor->setGeometry(option.rect);
  }

  void setEditorData (QWidget *widget, const QModelIndex &index) const
  {
    QLineEdit *editor = dynamic_cast<QLineEdit *> (widget);
    if (editor) {
      int n = index.model ()->data (index, Qt::UserRole).toInt ();
      if (mp_data->size () > size_t (n)) {
        if (index.column () == 0) {
          std::string name = mp_data->begin () [n].name ();
          editor->setText (tl::to_qstring (name));
          editor->setPlaceholderText (tr ("(default)"));
        } else if (index.column () == 1) {
          editor->setText (tl::to_qstring (mp_data->begin () [n].description ()));
        }
      }
    }
  }

  void setModelData (QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const
  {
    QLineEdit *editor = dynamic_cast<QLineEdit *> (widget);
    if (editor) {

      int n = model->data (index, Qt::UserRole).toInt ();
      if (mp_data->size () > size_t (n)) {

        std::string text = tl::to_string (editor->text ());
        if (index.column () == 0 && text.empty ()) {
          model->setData (index, QVariant (tr ("(default)")), Qt::DisplayRole);
        } else {
          model->setData (index, QVariant (tl::to_qstring (text)), Qt::DisplayRole);
        }

        if (index.column () == 0) {
          mp_data->begin () [n].set_name (text);
        } else if (index.column () == 1) {
          mp_data->begin () [n].set_description (text);
        }

      }

    }
  }

  QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const
  {
    QWidget *editor = createEditor (0, option, index);
    QSize size = editor->sizeHint ();
    delete editor;
    return size - QSize (2, 2);
  }

private:
  db::NetTracerTechnologyComponent *mp_data;
};

// -----------------------------------------------------------------------------------
//  NetTracerTechComponentEditor implementation

NetTracerTechComponentEditor::NetTracerTechComponentEditor (QWidget *parent)
  : TechnologyComponentEditor (parent)
{
  Ui::NetTracerTechComponentEditor::setupUi (this);

  QAction *action;
  action = new QAction (QObject::tr ("Add Stack"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (add_clicked ()));
  stack_tree->addAction (action);
  action = new QAction (QObject::tr ("Delete Selected Stacks"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (del_clicked ()));
  stack_tree->addAction (action);
  action = new QAction (QObject::tr ("Duplicate Stack"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (clone_clicked ()));
  stack_tree->addAction (action);

  connect (add_pb, SIGNAL (clicked ()), this, SLOT (add_clicked ()));
  connect (del_pb, SIGNAL (clicked ()), this, SLOT (del_clicked ()));
  connect (clone_pb, SIGNAL (clicked ()), this, SLOT (clone_clicked ()));
  connect (move_up_pb, SIGNAL (clicked ()), this, SLOT (move_up_clicked ()));
  connect (move_down_pb, SIGNAL (clicked ()), this, SLOT (move_down_clicked ()));

  stack_tree->header ()->setHighlightSections (false);
  stack_tree->header ()->setStretchLastSection (true);

  connect (stack_tree, SIGNAL (currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT (current_item_changed(QTreeWidgetItem *, QTreeWidgetItem *)));
}

void 
NetTracerTechComponentEditor::commit ()
{
  db::NetTracerTechnologyComponent *data = dynamic_cast <db::NetTracerTechnologyComponent *> (tech_component ());
  if (! data) {
    return;
  }

  commit_current ();
  *data = m_data;
}

void 
NetTracerTechComponentEditor::setup ()
{
  db::NetTracerTechnologyComponent *data = dynamic_cast <db::NetTracerTechnologyComponent *> (tech_component ());
  if (! data) {
    return;
  }

  m_data = *data;

  if (m_data.size () == 0) {
    m_data.push_back (db::NetTracerConnectivity ());
  }

  stack_tree->setItemDelegateForColumn (0, new NetTracerTechComponentColumnDelegate (stack_tree, &m_data));
  stack_tree->setItemDelegateForColumn (1, new NetTracerTechComponentColumnDelegate (stack_tree, &m_data));

  update ();

  if (stack_tree->topLevelItemCount () > 0) {
    stack_tree->setCurrentItem (stack_tree->topLevelItem (0));
  }
  current_item_changed (stack_tree->currentItem (), 0);
}

void
NetTracerTechComponentEditor::current_item_changed (QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
  commit_current (previous);

  int row = current ? stack_tree->indexOfTopLevelItem (current) : -1;
  if (row < 0 || row >= int (m_data.size ())) {
    connectivity_editor_widget->set_connectivity (db::NetTracerConnectivity ());
    connectivity_editor_widget->hide ();
  } else {
    connectivity_editor_widget->set_connectivity (m_data.begin ()[row]);
    connectivity_editor_widget->show ();
  }
}

void
NetTracerTechComponentEditor::commit_current ()
{
  commit_current (stack_tree->currentItem ());
}

void
NetTracerTechComponentEditor::commit_current (QTreeWidgetItem *current)
{
  int row = current ? stack_tree->indexOfTopLevelItem (current) : -1;
  if (row >= 0 && row < int (m_data.size ())) {
     connectivity_editor_widget->get_connectivity (m_data.begin () [row]);
  }
}

static std::string
new_name (const db::NetTracerTechnologyComponent &data)
{
  for (int i = 1; ; ++i) {
    std::string n = "STACK" + tl::to_string (i);
    bool found = false;
    for (auto d = data.begin (); d != data.end () && ! found; ++d) {
      found = (d->name () == n);
    }
    if (! found) {
      return n;
    }
  }

  return std::string ();
}

void
NetTracerTechComponentEditor::clone_clicked ()
{
  //  removes focus from the tree view - commits the data
  add_pb->setFocus ();
  commit_current ();

  int row = stack_tree->currentItem () ? stack_tree->indexOfTopLevelItem (stack_tree->currentItem ()) : -1;
  if (row < 0) {
    m_data.push_back (db::NetTracerConnectivity ());
    row = int (m_data.size () - 1);
  } else {
    row += 1;
    m_data.insert (m_data.begin () + row, db::NetTracerConnectivity ());
    m_data.begin ()[row] = m_data.begin ()[row - 1];
  }

  m_data.begin ()[row].set_name (new_name (m_data));

  update ();
  stack_tree->setCurrentItem (stack_tree->topLevelItem (row));
}

void
NetTracerTechComponentEditor::add_clicked ()
{
  //  removes focus from the tree view - commits the data
  add_pb->setFocus ();
  commit_current ();

  int row = stack_tree->currentItem () ? stack_tree->indexOfTopLevelItem (stack_tree->currentItem ()) : -1;
  if (row < 0) {
    m_data.push_back (db::NetTracerConnectivity ());
    row = int (m_data.size () - 1);
  } else {
    row += 1;
    m_data.insert (m_data.begin () + row, db::NetTracerConnectivity ());
  }

  m_data.begin ()[row].set_name (new_name (m_data));

  update ();
  stack_tree->setCurrentItem (stack_tree->topLevelItem (row));
}

void 
NetTracerTechComponentEditor::del_clicked ()
{
  //  removes focus from the tree view - commits the data
  del_pb->setFocus ();
  commit_current ();

  std::set<int> selected_rows;
  QModelIndexList selected_indices = stack_tree->selectionModel ()->selectedIndexes ();
  for (auto i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  int offset = 0;
  for (std::set<int>::const_iterator r = selected_rows.begin (); r != selected_rows.end (); ++r) {
    m_data.erase (m_data.begin () + (*r - offset));
    ++offset;
  }

  update ();
  stack_tree->setCurrentItem (0);
}

void 
NetTracerTechComponentEditor::move_up_clicked ()
{
  //  removes focus from the tree view - commits the data
  move_up_pb->setFocus ();
  commit_current ();

  std::set<int> selected_rows;
  QModelIndexList selected_indices = stack_tree->selectionModel ()->selectedIndexes ();
  for (auto i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  QTreeWidgetItem *current = stack_tree->currentItem ();
  int n_current = current ? current->data (0, Qt::UserRole).toInt () : -1;

  stack_tree->setCurrentIndex (QModelIndex ());

  int n = 0;
  for (db::NetTracerTechnologyComponent::iterator l = m_data.begin (); l != m_data.end (); ++l, ++n) {
    if (selected_rows.find (n + 1) != selected_rows.end () && selected_rows.find (n) == selected_rows.end ()) {
      std::swap (m_data.begin () [n + 1], m_data.begin () [n]);
      selected_rows.erase (n + 1);
      selected_rows.insert (n);
      if (n_current == n + 1) {
        n_current = n;
      }
    }
  }

  update ();

  // select the new items
  for (std::set <int>::const_iterator s = selected_rows.begin (); s != selected_rows.end (); ++s) {
    stack_tree->topLevelItem (*s)->setSelected (true);
  }
  if (n_current >= 0) {
    stack_tree->setCurrentItem (stack_tree->topLevelItem (n_current), 0, QItemSelectionModel::Current);
  }
}

void 
NetTracerTechComponentEditor::move_down_clicked ()
{
  //  removes focus from the tree view - commits the data
  move_down_pb->setFocus ();
  commit_current ();

  std::set<int> selected_rows;
  QModelIndexList selected_indices = stack_tree->selectionModel ()->selectedIndexes ();
  for (auto i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  QTreeWidgetItem *current = stack_tree->currentItem ();
  int n_current = current ? current->data (0, Qt::UserRole).toInt () : -1;

  stack_tree->setCurrentIndex (QModelIndex ());

  int n = int (m_data.size ());
  for (db::NetTracerTechnologyComponent::iterator l = m_data.end (); l != m_data.begin (); ) {
    --l;
    --n;
    if (selected_rows.find (n - 1) != selected_rows.end () && selected_rows.find (n) == selected_rows.end ()) {
      std::swap (m_data.begin () [n - 1], m_data.begin () [n]);
      selected_rows.erase (n - 1);
      selected_rows.insert (n);
      if (n_current == n - 1) {
        n_current = n;
      }
    }
  }

  update ();

  // select the new items
  for (std::set <int>::const_iterator s = selected_rows.begin (); s != selected_rows.end (); ++s) {
    stack_tree->topLevelItem (*s)->setSelected (true);
  }
  if (n_current >= 0) {
    stack_tree->setCurrentItem (stack_tree->topLevelItem (n_current), 0, QItemSelectionModel::Current);
  }
}

void
NetTracerTechComponentEditor::update ()
{
  stack_tree->clear ();
  stack_tree->clearSelection ();

  int n = 0;
  for (db::NetTracerTechnologyComponent::iterator l = m_data.begin (); l != m_data.end (); ++l, ++n) {

    QTreeWidgetItem *item = new QTreeWidgetItem (stack_tree);
    item->setFlags (item->flags () | Qt::ItemIsEditable);

    std::string name = l->name ();
    if (name.empty ()) {
      item->setData (0, Qt::DisplayRole, QVariant (tr ("(default)")));
    } else {
      item->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (name)));
    }
    item->setData (0, Qt::UserRole, QVariant (n));

    item->setData (1, Qt::DisplayRole, QVariant (tl::to_qstring (l->description ())));
    item->setData (1, Qt::UserRole, QVariant (n));

  }
}

}


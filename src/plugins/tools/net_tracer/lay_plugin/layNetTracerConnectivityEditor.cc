
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


#include "layNetTracerConnectivityEditor.h"
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
//  NetTracerConnectivityColumnDelegate definition and implementation

class NetTracerConnectivityColumnDelegate
  : public QItemDelegate
{
public:
  NetTracerConnectivityColumnDelegate (QWidget *parent, db::NetTracerConnectivity *data)
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
          editor->setText (tl::to_qstring (mp_data->begin () [n].layer_a ().to_string ()));
        } else if (index.column () == 1) {
          editor->setText (tl::to_qstring (mp_data->begin () [n].via_layer ().to_string ()));
        } else if (index.column () == 2) {
          editor->setText (tl::to_qstring (mp_data->begin () [n].layer_b ().to_string ()));
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

        db::NetTracerLayerExpressionInfo expr;

        std::string text = tl::to_string (editor->text ());
        tl::Extractor ex (text.c_str ());
        bool error = false;
        try {
          expr = db::NetTracerLayerExpressionInfo::compile (text);
        } catch (...) {
          error = true;
        }

        if (error) {
          model->setData (index, QVariant (tl::to_qstring (text)), Qt::DisplayRole);
          model->setData (index, QVariant (QColor (Qt::red)), Qt::ForegroundRole);
          model->setData (index, QVariant (QColor (Qt::red).lighter (180)), Qt::BackgroundRole);
        } else if ((index.column () == 0 || index.column () == 2) && expr.to_string ().empty ()) {
          model->setData (index, QVariant (QObject::tr ("Enter expression")), Qt::DisplayRole);
          model->setData (index, QVariant (QColor (Qt::red)), Qt::ForegroundRole);
          model->setData (index, QVariant (QColor (Qt::red).lighter (180)), Qt::BackgroundRole);
        } else if (index.column () == 1 && expr.to_string ().empty ()) {
          model->setData (index, QVariant (QObject::tr ("None")), Qt::DisplayRole);
          model->setData (index, QVariant (), Qt::ForegroundRole);
          model->setData (index, QVariant (), Qt::BackgroundRole);
        } else {
          model->setData (index, QVariant (tl::to_qstring (expr.to_string ())), Qt::DisplayRole);
          model->setData (index, QVariant (), Qt::ForegroundRole);
          model->setData (index, QVariant (), Qt::BackgroundRole);
        }

        if (index.column () == 0) {
          mp_data->begin () [n].set_layer_a (expr);
        } else if (index.column () == 1) {
          mp_data->begin () [n].set_via_layer (expr);
        } else if (index.column () == 2) {
          mp_data->begin () [n].set_layer_b (expr);
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
  db::NetTracerConnectivity *mp_data;
};

// -----------------------------------------------------------------------------------------
//  NetTracerConnectivitySymbolColumnDelegate definition and implementation

class NetTracerConnectivitySymbolColumnDelegate
  : public QItemDelegate
{
public:
  NetTracerConnectivitySymbolColumnDelegate (QWidget *parent, db::NetTracerConnectivity *data)
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
      if (mp_data->symbols () > size_t (n)) { 
        if (index.column () == 0) {
          editor->setText (tl::to_qstring (mp_data->begin_symbols () [n].symbol ().to_string ()));
        } else if (index.column () == 1) {
          editor->setText (tl::to_qstring (mp_data->begin_symbols () [n].expression ()));
        }
      }
    }
  }

  void setModelData (QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const
  {
    QLineEdit *editor = dynamic_cast<QLineEdit *> (widget);
    if (editor) {

      int n = model->data (index, Qt::UserRole).toInt ();
      if (mp_data->symbols () > size_t (n)) { 

        std::string text = tl::to_string (editor->text ());

        if (index.column () == 0 && text.empty ()) {

          model->setData (index, QVariant (QObject::tr ("Enter symbol")), Qt::DisplayRole);
          model->setData (index, QVariant (QColor (Qt::red)), Qt::ForegroundRole);
          model->setData (index, QVariant (QColor (Qt::red).lighter (180)), Qt::BackgroundRole);

        } else if (index.column () == 1 && text.empty ()) {

          model->setData (index, QVariant (QObject::tr ("Enter expression")), Qt::DisplayRole);
          model->setData (index, QVariant (QColor (Qt::red)), Qt::ForegroundRole);
          model->setData (index, QVariant (QColor (Qt::red).lighter (180)), Qt::BackgroundRole);

        } else if (index.column () == 1) {

          bool ok = true;
          //  check the expression
          try {
            db::NetTracerLayerExpressionInfo::compile (text);
          } catch (tl::Exception &) {
            ok = false;
          }

          model->setData (index, QVariant (tl::to_qstring (text)), Qt::DisplayRole);

          if (! ok) {
            model->setData (index, QVariant (QColor (Qt::red)), Qt::ForegroundRole);
            model->setData (index, QVariant (QColor (Qt::red).lighter (180)), Qt::BackgroundRole);
          } else {
            model->setData (index, QVariant (), Qt::ForegroundRole);
            model->setData (index, QVariant (), Qt::BackgroundRole);
          }

        } else {

          model->setData (index, QVariant (tl::to_qstring (text)), Qt::DisplayRole);
          model->setData (index, QVariant (), Qt::ForegroundRole);
          model->setData (index, QVariant (), Qt::BackgroundRole);

        }

        if (index.column () == 0) {
          db::LayerProperties lp;
          tl::Extractor ex (text.c_str ());
          lp.read (ex);
          mp_data->begin_symbols () [n].set_symbol (lp);
        } else if (index.column () == 1) {
          mp_data->begin_symbols () [n].set_expression (text);
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
  db::NetTracerConnectivity *mp_data;
};

// -----------------------------------------------------------------------------------
//  NetTracerTechComponentEditor implementation

NetTracerConnectivityEditor::NetTracerConnectivityEditor (QWidget *parent)
  : QWidget (parent)
{
  Ui::NetTracerConnectivityEditor::setupUi (this);

  connect (add_conductor_pb, SIGNAL (clicked ()), this, SLOT (add_clicked ()));
  connect (del_conductor_pb, SIGNAL (clicked ()), this, SLOT (del_clicked ()));
  connect (move_conductor_up_pb, SIGNAL (clicked ()), this, SLOT (move_up_clicked ()));
  connect (move_conductor_down_pb, SIGNAL (clicked ()), this, SLOT (move_down_clicked ()));
  connect (add_symbol_pb, SIGNAL (clicked ()), this, SLOT (symbol_add_clicked ()));
  connect (del_symbol_pb, SIGNAL (clicked ()), this, SLOT (symbol_del_clicked ()));
  connect (move_symbol_up_pb, SIGNAL (clicked ()), this, SLOT (symbol_move_up_clicked ()));
  connect (move_symbol_down_pb, SIGNAL (clicked ()), this, SLOT (symbol_move_down_clicked ()));

  lay::activate_help_links (symbol_help_label);
  lay::activate_help_links (help_label);

  connectivity_table->horizontalHeader ()->setHighlightSections (false);
  connectivity_table->horizontalHeader ()->setStretchLastSection (true);
  connectivity_table->verticalHeader ()->hide ();
  symbol_table->horizontalHeader ()->setHighlightSections (false);
  symbol_table->horizontalHeader ()->setStretchLastSection (true);
  symbol_table->verticalHeader ()->hide ();
}

void
NetTracerConnectivityEditor::get_connectivity (db::NetTracerConnectivity &data)
{
  std::string name = data.name ();
  std::string description = data.description ();
  data = m_data;
  data.set_name (name);
  data.set_description (description);
}

void 
NetTracerConnectivityEditor::set_connectivity (const db::NetTracerConnectivity &data)
{
  m_data = data;

  for (int c = 0; c < 3; ++c) {
    if (connectivity_table->itemDelegateForColumn (c) != 0) {
      delete connectivity_table->itemDelegateForColumn (c);
    }
    connectivity_table->setItemDelegateForColumn (c, new NetTracerConnectivityColumnDelegate (connectivity_table, &m_data));
  }

  for (int c = 0; c < 2; ++c) {
    if (symbol_table->itemDelegateForColumn (c) != 0) {
      delete symbol_table->itemDelegateForColumn (c);
    }
    symbol_table->setItemDelegateForColumn (c, new NetTracerConnectivitySymbolColumnDelegate (symbol_table, &m_data));
  }

  update ();
}

void 
NetTracerConnectivityEditor::add_clicked ()
{
  //  removes focus from the tree view - commits the data
  add_conductor_pb->setFocus (); 

  int row = connectivity_table->currentItem () ? connectivity_table->row (connectivity_table->currentItem ()) : -1;
  if (row < 0) {
    m_data.add (db::NetTracerConnectionInfo ());
    row = int (m_data.size () - 1);
  } else {
    row += 1;
    m_data.insert (m_data.begin () + row, db::NetTracerConnectionInfo ());
  }

  update ();
  connectivity_table->setCurrentItem (connectivity_table->item (row, 0));
}

void 
NetTracerConnectivityEditor::del_clicked ()
{
  //  removes focus from the tree view - commits the data
  del_conductor_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = connectivity_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  connectivity_table->setCurrentIndex (QModelIndex ());

  int offset = 0;
  for (std::set<int>::const_iterator r = selected_rows.begin (); r != selected_rows.end (); ++r) {
    m_data.erase (m_data.begin () + (*r - offset));
    ++offset;
  }

  update ();
}

void 
NetTracerConnectivityEditor::move_up_clicked ()
{
  //  removes focus from the tree view - commits the data
  move_conductor_up_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = connectivity_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  QTableWidgetItem *current = connectivity_table->currentItem ();
  int n_current = current ? current->data (Qt::UserRole).toInt () : -1;

  connectivity_table->setCurrentIndex (QModelIndex ());

  int n = 0;
  for (db::NetTracerConnectivity::iterator l = m_data.begin (); l != m_data.end (); ++l, ++n) {
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
    connectivity_table->selectionModel ()->select (connectivity_table->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    connectivity_table->selectionModel ()->select (connectivity_table->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void 
NetTracerConnectivityEditor::move_down_clicked ()
{
  //  removes focus from the tree view - commits the data
  move_conductor_down_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = connectivity_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  QTableWidgetItem *current = connectivity_table->currentItem ();
  int n_current = current ? current->data (Qt::UserRole).toInt () : -1;

  connectivity_table->setCurrentIndex (QModelIndex ());

  int n = int (m_data.size ());
  for (db::NetTracerConnectivity::iterator l = m_data.end (); l != m_data.begin (); ) {
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
    connectivity_table->selectionModel ()->select (connectivity_table->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    connectivity_table->selectionModel ()->select (connectivity_table->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void 
NetTracerConnectivityEditor::symbol_add_clicked ()
{
  //  removes focus from the tree view - commits the data
  add_symbol_pb->setFocus (); 

  int row = symbol_table->currentItem () ? symbol_table->row (symbol_table->currentItem ()) : -1;
  if (row < 0) {
    m_data.add_symbol (db::NetTracerSymbolInfo ());
    row = int (m_data.symbols () - 1);
  } else {
    row += 1;
    m_data.insert_symbol (m_data.begin_symbols () + row, db::NetTracerSymbolInfo ());
  }

  update ();
  symbol_table->setCurrentItem (symbol_table->item (row, 0));
}

void 
NetTracerConnectivityEditor::symbol_del_clicked ()
{
  //  removes focus from the tree view - commits the data
  del_symbol_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = symbol_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  symbol_table->setCurrentIndex (QModelIndex ());

  int offset = 0;
  for (std::set<int>::const_iterator r = selected_rows.begin (); r != selected_rows.end (); ++r) {
    m_data.erase_symbol (m_data.begin_symbols () + (*r - offset));
    ++offset;
  }

  update ();
}

void 
NetTracerConnectivityEditor::symbol_move_up_clicked ()
{
  //  removes focus from the tree view - commits the data
  move_symbol_up_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = symbol_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  QTableWidgetItem *current = symbol_table->currentItem ();
  int n_current = current ? current->data (Qt::UserRole).toInt () : -1;

  symbol_table->setCurrentIndex (QModelIndex ());

  int n = 0;
  for (db::NetTracerConnectivity::symbol_iterator l = m_data.begin_symbols (); l != m_data.end_symbols (); ++l, ++n) {
    if (selected_rows.find (n + 1) != selected_rows.end () && selected_rows.find (n) == selected_rows.end ()) {
      std::swap (m_data.begin_symbols () [n + 1], m_data.begin_symbols () [n]);
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
    symbol_table->selectionModel ()->select (symbol_table->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    symbol_table->selectionModel ()->select (symbol_table->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void 
NetTracerConnectivityEditor::symbol_move_down_clicked ()
{
  //  removes focus from the tree view - commits the data
  move_symbol_down_pb->setFocus (); 

  std::set<int> selected_rows;
  QModelIndexList selected_indices = symbol_table->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected_indices.begin (); i != selected_indices.end (); ++i) {
    selected_rows.insert (i->row ());
  }

  QTableWidgetItem *current = symbol_table->currentItem ();
  int n_current = current ? current->data (Qt::UserRole).toInt () : -1;

  symbol_table->setCurrentIndex (QModelIndex ());

  int n = int (m_data.symbols ());
  for (db::NetTracerConnectivity::symbol_iterator l = m_data.end_symbols (); l != m_data.begin_symbols (); ) {
    --l;
    --n;
    if (selected_rows.find (n - 1) != selected_rows.end () && selected_rows.find (n) == selected_rows.end ()) {
      std::swap (m_data.begin_symbols () [n - 1], m_data.begin_symbols () [n]);
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
    symbol_table->selectionModel ()->select (symbol_table->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    symbol_table->selectionModel ()->select (symbol_table->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void
NetTracerConnectivityEditor::update ()
{
  QStringList labels;
  int n;

  connectivity_table->clear ();
  connectivity_table->setRowCount (int (m_data.size ()));

  connectivity_table->setColumnCount (3);
  labels.clear ();
  labels << QObject::tr ("Conductor 1");
  labels << QObject::tr ("Via (optional)");
  labels << QObject::tr ("Conductor 2");
  connectivity_table->setHorizontalHeaderLabels (labels);

  n = 0;
  for (db::NetTracerConnectivity::iterator l = m_data.begin (); l != m_data.end (); ++l, ++n) {

    for (int c = 0; c < 3; ++c) {

      QTableWidgetItem *item = new QTableWidgetItem ();
      item->setFlags (item->flags () | Qt::ItemIsEditable);
      connectivity_table->setItem (n, c, item);

      item->setData (Qt::ForegroundRole, QVariant ());

      if (c == 0) {
        if (l->layer_a ().to_string ().empty ()) {
          item->setData (Qt::DisplayRole, QVariant (QObject::tr ("Enter layer")));
          item->setData (Qt::ForegroundRole, QVariant (QColor (Qt::red)));
          item->setData (Qt::BackgroundRole, QVariant (QColor (Qt::red).lighter (180)));
        } else {
          item->setData (Qt::DisplayRole, QVariant (tl::to_qstring (l->layer_a ().to_string ())));
        }
      } else if (c == 1) {
        if (l->via_layer ().to_string ().empty ()) {
          item->setData (Qt::DisplayRole, QVariant (QObject::tr ("None")));
        } else {
          item->setData (Qt::DisplayRole, QVariant (tl::to_qstring (l->via_layer ().to_string ())));
        }
      } else if (c == 2) {
        if (l->layer_b ().to_string ().empty ()) {
          item->setData (Qt::DisplayRole, QVariant (QObject::tr ("Enter layer")));
          item->setData (Qt::ForegroundRole, QVariant (QColor (Qt::red)));
          item->setData (Qt::BackgroundRole, QVariant (QColor (Qt::red).lighter (180)));
        } else {
          item->setData (Qt::DisplayRole, QVariant (tl::to_qstring (l->layer_b ().to_string ())));
        }
      }

      item->setData (Qt::UserRole, QVariant (n));

    }

  }

  connectivity_table->clearSelection ();

  symbol_table->clear ();
  symbol_table->setRowCount (int (m_data.symbols ()));

  symbol_table->setColumnCount (2);
  labels.clear ();
  labels << QObject::tr ("Symbol");
  labels << QObject::tr ("Expression");
  symbol_table->setHorizontalHeaderLabels (labels);

  n = 0;
  for (db::NetTracerConnectivity::symbol_iterator l = m_data.begin_symbols (); l != m_data.end_symbols (); ++l, ++n) {

    for (int c = 0; c < 2; ++c) {

      QTableWidgetItem *item = new QTableWidgetItem ();
      item->setFlags (item->flags () | Qt::ItemIsEditable);
      symbol_table->setItem (n, c, item);

      item->setData (Qt::ForegroundRole, QVariant ());
      item->setData (Qt::BackgroundRole, QVariant ());

      if (c == 0) {

        if (l->symbol ().log_equal (db::LayerProperties ())) {
          item->setData (Qt::DisplayRole, QVariant (QObject::tr ("Enter symbol")));
          item->setData (Qt::ForegroundRole, QVariant (QColor (Qt::red)));
          item->setData (Qt::BackgroundRole, QVariant (QColor (Qt::red).lighter (180)));
        } else {
          item->setData (Qt::DisplayRole, QVariant (tl::to_qstring (l->symbol ().to_string ())));
        }

      } else if (c == 1) {

        if (l->expression ().empty ()) {

          item->setData (Qt::DisplayRole, QVariant (QObject::tr ("Enter expression")));
          item->setData (Qt::ForegroundRole, QVariant (QColor (Qt::red)));
          item->setData (Qt::BackgroundRole, QVariant (QColor (Qt::red).lighter (180)));

        } else {

          bool ok = true;
          //  check the expression
          try {
            db::NetTracerLayerExpressionInfo::compile (l->expression ());
          } catch (tl::Exception &) {
            ok = false;
          }

          if (! ok) {
            item->setData (Qt::ForegroundRole, QVariant (QColor (Qt::red)));
            item->setData (Qt::BackgroundRole, QVariant (QColor (Qt::red).lighter (180)));
          }

          item->setData (Qt::DisplayRole, QVariant (tl::to_qstring (l->expression ())));

        }

      }

      item->setData (Qt::UserRole, QVariant (n));

    }

  }

  symbol_table->clearSelection ();
}

}


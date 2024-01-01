
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


#include "ui_GerberImportDialog.h"
#include "dbGerberImporter.h"

#include "layFileDialog.h"
#include "layDialogs.h"
#include "layConverters.h"
#include "layGerberImportDialog.h"

#include "tlExceptions.h"
#include "tlXMLParser.h"

#include <QFileDialog>
#include <QItemDelegate>
#include <QHeaderView>
#include <QMessageBox>

#include <fstream>

namespace lay
{

// -----------------------------------------------------------------------------------------
//  GerberImportDialogFileColumnEditorWidget implementation

GerberImportDialogFileColumnEditorWidget::GerberImportDialogFileColumnEditorWidget (QWidget *parent, db::GerberImportData *data)
  : QFrame (parent), mp_data (data)
{
  QHBoxLayout *layout = new QHBoxLayout (this);
  layout->setSpacing (0);
  layout->setContentsMargins (0, 0, 0, 0);
  setLayout (layout);
  setFocusPolicy (Qt::ClickFocus);

  mp_filename_edit = new QLineEdit (this);
  layout->addWidget (mp_filename_edit);
  mp_filename_edit->setFocusPolicy (Qt::ClickFocus);
  QSizePolicy sp = mp_filename_edit->sizePolicy ();
  sp.setVerticalPolicy (QSizePolicy::Expanding);
  mp_filename_edit->setSizePolicy (sp);

  mp_browse_button = new QToolButton (this);
  mp_browse_button->setText (QString::fromUtf8 ("..."));
  mp_browse_button->setFocusPolicy (Qt::NoFocus);
  layout->addWidget (mp_browse_button);
  connect (mp_browse_button, SIGNAL (clicked ()), this, SLOT (browse_clicked ()));

  setFocusProxy (mp_filename_edit);
}

void 
GerberImportDialogFileColumnEditorWidget::set_filename (const std::string &s)
{
  QDir base_dir (tl::to_qstring (mp_data->base_dir));
  QString text (base_dir.relativeFilePath (tl::to_qstring (s)));
  if (mp_filename_edit->text () != text) {
    mp_filename_edit->setText (base_dir.relativeFilePath (tl::to_qstring (s)));
  }
}

std::string 
GerberImportDialogFileColumnEditorWidget::get_filename () const
{
  return tl::to_string (mp_filename_edit->text ());
}

void
GerberImportDialogFileColumnEditorWidget::browse_clicked ()
{
  QFileInfo file (tl::to_qstring (get_filename ()));
  if (get_filename ().empty ()) {
    file = QFileInfo (tl::to_qstring (mp_data->base_dir));
  } else if (! QFileInfo (tl::to_qstring (get_filename ())).isAbsolute ()) {
    file = QFileInfo (QDir (tl::to_qstring (mp_data->base_dir)), tl::to_qstring (get_filename ()));
  } 

  std::string new_file (tl::to_string (file.absoluteFilePath ()));
  lay::FileDialog open_dialog (this, tl::to_string (QObject::tr ("Load PCB data file")), tl::to_string (QObject::tr ("All files (*)")));
  if (open_dialog.get_open (new_file)) {
    set_filename (new_file);
  }
}

// -----------------------------------------------------------------------------------------
//  GerberImportDialogMetalLayerColumnEditorWidget implementation

GerberImportDialogMetalLayerColumnEditorWidget::GerberImportDialogMetalLayerColumnEditorWidget (QWidget *parent, db::GerberImportData *data)
  : QComboBox (parent)
{
  for (int i = 0; i < int (data->artwork_files.size ()); ++i) {
    addItem (tl::to_qstring (tl::sprintf (tl::to_string (QObject::tr ("Metal %d")), i + 1)));
  }
}

void 
GerberImportDialogMetalLayerColumnEditorWidget::set_layer (int layer)
{
  setCurrentIndex (layer);
}

int 
GerberImportDialogMetalLayerColumnEditorWidget::get_layer () const
{
  return currentIndex ();
}

// -----------------------------------------------------------------------------------------
//  GerberImportDialogNoEditDelegate 

class GerberImportDialogNoEditDelegate 
  : public QItemDelegate
{
public:
  GerberImportDialogNoEditDelegate (QWidget *parent)
    : QItemDelegate (parent)
  {
    //  .. nothing yet ..
  }

  QWidget *createEditor (QWidget * /*parent*/, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const 
  {
    return 0;
  }
};

// -----------------------------------------------------------------------------------------
//  GerberImportDialogMetalLayerColumnDelegate definition and implementation

class GerberImportDialogMetalLayerColumnDelegate
  : public QItemDelegate
{
public:
  GerberImportDialogMetalLayerColumnDelegate (QWidget *parent, db::GerberImportData *data)
    : QItemDelegate (parent), mp_data (data)
  {
    //  .. nothing yet ..
  }

  QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const 
  {
    return new GerberImportDialogMetalLayerColumnEditorWidget (parent, mp_data);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
  {
    editor->setGeometry(option.rect);
  }

  void setEditorData (QWidget *widget, const QModelIndex &index) const
  {
    GerberImportDialogMetalLayerColumnEditorWidget *editor = dynamic_cast<GerberImportDialogMetalLayerColumnEditorWidget *> (widget);
    if (editor) {
      editor->set_layer (index.model ()->data (index, Qt::UserRole).toInt ());
    }
  }

  void setModelData (QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const
  {
    GerberImportDialogMetalLayerColumnEditorWidget *editor = dynamic_cast<GerberImportDialogMetalLayerColumnEditorWidget *> (widget);
    if (editor) {
      int l = editor->get_layer ();
      model->setData (index, QVariant (l), Qt::UserRole);
      if (l < 0) {
        model->setData (index, QVariant (QString::fromUtf8 ("...")), Qt::DisplayRole);
        model->setData (index, widget->palette ().color (QPalette::Disabled, QPalette::Text), Qt::ForegroundRole);
      } else {
        model->setData (index, tl::to_qstring (tl::sprintf (tl::to_string (QObject::tr ("Metal %d")), l + 1)), Qt::DisplayRole);
        model->setData (index, widget->palette ().color (QPalette::Active, QPalette::Text), Qt::ForegroundRole);
      }
    }
  }

  QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const 
  {
    QWidget *editor = createEditor (0, option, index);
    QSize size = editor->sizeHint ();
    delete editor;
    return size;
  }

private:
  db::GerberImportData *mp_data;
};

// -----------------------------------------------------------------------------------------
//  GerberImportDialogFileColumnDelegate definition and implementation

class GerberImportDialogFileColumnDelegate
  : public QItemDelegate
{
public:
  GerberImportDialogFileColumnDelegate (QWidget *parent, db::GerberImportData *data)
    : QItemDelegate (parent), mp_data (data)
  {
    //  .. nothing yet ..
  }

  QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const 
  {
    return new GerberImportDialogFileColumnEditorWidget (parent, mp_data);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
  {
    editor->setGeometry(option.rect);
  }

  void setEditorData (QWidget *widget, const QModelIndex &index) const
  {
    GerberImportDialogFileColumnEditorWidget *editor = dynamic_cast<GerberImportDialogFileColumnEditorWidget *> (widget);
    if (editor) {
      std::string fn (tl::to_string (index.model ()->data (index, Qt::UserRole).toString ()));
      editor->set_filename (fn);
    }
  }

  void setModelData (QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const
  {
    GerberImportDialogFileColumnEditorWidget *editor = dynamic_cast<GerberImportDialogFileColumnEditorWidget *> (widget);
    if (editor) {

      model->setData (index, QVariant (tl::to_qstring (editor->get_filename ())), Qt::UserRole);
      if (! editor->get_filename ().empty ()) {

        model->setData (index, QVariant (tl::to_qstring (editor->get_filename ())), Qt::DisplayRole);

        QFileInfo file_info (tl::to_qstring (editor->get_filename ()));
        if (! mp_data->base_dir.empty () && ! file_info.isAbsolute ()) {
          file_info = QFileInfo (QDir (tl::to_qstring (mp_data->base_dir)), tl::to_qstring (editor->get_filename ()));
        }

        if (file_info.exists () && file_info.isReadable ()) {
          model->setData (index, widget->palette ().color (QPalette::Active, QPalette::Text), Qt::ForegroundRole);
          model->setData (index, widget->palette ().color (QPalette::Active, QPalette::Base), Qt::BackgroundRole);
        } else {
          model->setData (index, QColor (Qt::red), Qt::ForegroundRole);
          model->setData (index, QColor (Qt::red).lighter (180), Qt::BackgroundRole);
        }

      } else {

        model->setData (index, QVariant (QObject::tr ("Click here to enter file name")), Qt::DisplayRole);
        model->setData (index, widget->palette ().color (QPalette::Disabled, QPalette::Text), Qt::ForegroundRole);
        model->setData (index, widget->palette ().color (QPalette::Disabled, QPalette::Base), Qt::BackgroundRole);

      }

    }
  }

  QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const 
  {
    QWidget *editor = createEditor (0, option, index);
    QSize size = editor->sizeHint ();
    delete editor;
    return size;
  }

private:
  db::GerberImportData *mp_data;
};

// -----------------------------------------------------------------------------------------
//  GerberImportDialog implementation

GerberImportDialog::GerberImportDialog (QWidget *parent, db::GerberImportData *data)
  : QDialog (parent), mp_data (data)
{
  mp_ui = new Ui::GerberImportDialog ();
  mp_ui->setupUi (this);

  connect (mp_ui->last_pb, SIGNAL (clicked ()), this, SLOT (last_page ()));
  connect (mp_ui->next_pb, SIGNAL (clicked ()), this, SLOT (next_page ()));
  connect (mp_ui->layer_properties_file_browse_pb, SIGNAL (clicked ()), this, SLOT (browse_layer_properties_file ()));
  connect (mp_ui->base_dir_pb, SIGNAL (clicked ()), this, SLOT (browse_base_dir ()));

  connect (mp_ui->add_target_layer_pb, SIGNAL (clicked ()), this, SLOT (add_target_layer ()));
  connect (mp_ui->delete_target_layer_pb, SIGNAL (clicked ()), this, SLOT (delete_target_layer ()));
  connect (mp_ui->move_target_layer_up_pb, SIGNAL (clicked ()), this, SLOT (move_target_layer_up ()));
  connect (mp_ui->move_target_layer_down_pb, SIGNAL (clicked ()), this, SLOT (move_target_layer_down ()));

  connect (mp_ui->free_add_target_layer_pb, SIGNAL (clicked ()), this, SLOT (add_target_layer ()));
  connect (mp_ui->free_delete_target_layer_pb, SIGNAL (clicked ()), this, SLOT (delete_target_layer ()));
  connect (mp_ui->free_move_target_layer_up_pb, SIGNAL (clicked ()), this, SLOT (move_target_layer_up ()));
  connect (mp_ui->free_move_target_layer_down_pb, SIGNAL (clicked ()), this, SLOT (move_target_layer_down ()));

  connect (mp_ui->reset_mapping_pb, SIGNAL (clicked ()), this, SLOT (reset_free_mapping ()));

  connect (mp_ui->layout_layers_tree, SIGNAL (itemDoubleClicked (QTreeWidgetItem *, int)), this, SLOT (layout_layer_double_clicked (QTreeWidgetItem *, int)));
  connect (mp_ui->free_layout_layers_tree, SIGNAL (itemDoubleClicked (QTreeWidgetItem *, int)), this, SLOT (layout_layer_double_clicked (QTreeWidgetItem *, int)));

  connect (mp_ui->add_free_file_pb, SIGNAL (clicked ()), this, SLOT (add_free_file ()));
  connect (mp_ui->delete_free_file_pb, SIGNAL (clicked ()), this, SLOT (delete_free_file ()));
  connect (mp_ui->move_free_file_up_pb, SIGNAL (clicked ()), this, SLOT (move_free_file_up ()));
  connect (mp_ui->move_free_file_down_pb, SIGNAL (clicked ()), this, SLOT (move_free_file_down ()));

  mp_ui->artwork_files_tree->setItemDelegateForColumn (0, new GerberImportDialogNoEditDelegate (mp_ui->artwork_files_tree));
  mp_ui->artwork_files_tree->setItemDelegateForColumn (1, new GerberImportDialogFileColumnDelegate (mp_ui->artwork_files_tree, mp_data));

  mp_ui->free_layer_mapping_tree->setItemDelegateForColumn (0, new GerberImportDialogNoEditDelegate (mp_ui->free_layer_mapping_tree));
  mp_ui->free_files_tree->setItemDelegateForColumn (0, new GerberImportDialogFileColumnDelegate (mp_ui->free_files_tree, mp_data));
  connect (mp_ui->free_layer_mapping_tree, SIGNAL (itemClicked (QTreeWidgetItem *, int)), this, SLOT (free_layer_mapping_item_clicked (QTreeWidgetItem *, int)));

  mp_ui->drill_files_tree->setItemDelegateForColumn (0, new GerberImportDialogNoEditDelegate (mp_ui->artwork_files_tree));
  mp_ui->drill_files_tree->setItemDelegateForColumn (1, new GerberImportDialogMetalLayerColumnDelegate (mp_ui->artwork_files_tree, mp_data));
  mp_ui->drill_files_tree->setItemDelegateForColumn (2, new GerberImportDialogMetalLayerColumnDelegate (mp_ui->artwork_files_tree, mp_data));
  mp_ui->drill_files_tree->setItemDelegateForColumn (3, new GerberImportDialogFileColumnDelegate (mp_ui->artwork_files_tree, mp_data));

  m_open_action = new QAction (QObject::tr ("Open Project"), mp_ui->file_menu);
  m_saveas_action = new QAction (QObject::tr ("Save Project"), mp_ui->file_menu);
  m_reset_action = new QAction (QObject::tr ("New Project"), mp_ui->file_menu);

  mp_ui->file_menu->addAction (m_open_action);
  mp_ui->file_menu->addAction (m_saveas_action);
  mp_ui->file_menu->addAction (m_reset_action);

  connect (m_open_action, SIGNAL (triggered ()), this, SLOT (open_clicked ()));
  connect (m_saveas_action, SIGNAL (triggered ()), this, SLOT (saveas_clicked ()));
  connect (m_reset_action, SIGNAL (triggered ()), this, SLOT (reset_clicked ()));
}

GerberImportDialog::~GerberImportDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
GerberImportDialog::reset_clicked ()
{
  //  Commit everything that is not loaded
  try {
    commit_page ();
  } catch (...) {
    // ..
  }

  mp_data->reset ();
  mp_ui->central_stack->setCurrentIndex (0);
  update ();
}

void
GerberImportDialog::saveas_clicked ()
{
BEGIN_PROTECTED

  commit_page ();

  //  Get the name of the file to save
  lay::FileDialog save_dialog (this, tl::to_string (QObject::tr ("Load Gerber Import Project File")), tl::to_string (QObject::tr ("PCB project file (*.pcb);;All files (*)")));
  std::string fn = mp_data->current_file;
  if (save_dialog.get_save (fn)) {
    mp_data->save (fn);
  }

END_PROTECTED
}

void
GerberImportDialog::open_clicked ()
{
  //  Commit everything that is not loaded
  try {
    commit_page ();
  } catch (...) {
    // ..
  }

  BEGIN_PROTECTED

  //  Get the name of the file to open
  lay::FileDialog open_dialog (this, tl::to_string (QObject::tr ("Load Gerber Import Project File")), tl::to_string (QObject::tr ("PCB project file (*.pcb);;All files (*)")));
  std::string fn = mp_data->current_file;
  if (open_dialog.get_open (fn)) {

    //  set the base dir to point to where the project file is located
    QFileInfo fi (tl::to_qstring (fn));
    mp_data->base_dir = tl::to_string (fi.absoluteDir ().path ());
    mp_data->load (fn);
    
  }

  END_PROTECTED

  update ();
}

void  
GerberImportDialog::browse_layer_properties_file ()
{
  std::string file = tl::to_string (mp_ui->layer_properties_file_le->text ());
  lay::FileDialog open_dialog (this, tl::to_string (QObject::tr ("Load Layer Properties File")), tl::to_string (QObject::tr ("Layer properties files (*.lyp);;All files (*)")));
  if (open_dialog.get_open (file)) {
    QDir base_dir (tl::to_qstring (mp_data->base_dir));
    mp_ui->layer_properties_file_le->setText (base_dir.relativeFilePath (tl::to_qstring (file)));
  }
}

void  
GerberImportDialog::browse_base_dir ()
{
  QString base_dir = mp_ui->base_dir_le->text ();
  base_dir = QFileDialog::getExistingDirectory (this, QObject::tr ("Get Base Directory"), base_dir);
  if (! base_dir.isNull ()) {
    mp_ui->base_dir_le->setText (base_dir);
  }
}

void 
GerberImportDialog::reject ()
{
  try {
    commit_page ();
  } catch (...) {
    // .. nothing yet ..
  }

  QDialog::reject ();
}

void 
GerberImportDialog::accept ()
{
  BEGIN_PROTECTED

  commit_page ();
  QDialog::accept ();

  END_PROTECTED
}

int 
GerberImportDialog::exec ()
{
  mp_ui->central_stack->setCurrentIndex (0);
  update ();

  return QDialog::exec ();
}

/* 
 * 0 - General
 * 1 - Layers (stacked)
 * 2 - Stack (stacked)
 * 3 - Artwork files (stacked)
 * 4 - Drill files (stacked)
 * 5 - Layers (free)
 * 6 - Mapping (free)
 * 7 - Reference points
 * 8 - Options
 */
static int next_pages_stacked[] = {  1, 2, 3, 4, 8, 8, 8, 8, 9, -1 };
static int next_pages_free[]    = {  5, 5, 5, 5, 5, 6, 7, 8, 9, -1 };
static int prev_pages_stacked[] = { -1, 0, 1, 2, 3, 4, 4, 4, 4, 8  };
static int prev_pages_free[]    = { -1, 0, 0, 0, 0, 0, 5, 6, 7, 8  };

void
GerberImportDialog::next_page ()
{
  BEGIN_PROTECTED 
  commit_page ();

  int index = mp_ui->central_stack->currentIndex ();
  if (index >= 0 && index < int (sizeof (next_pages_stacked) / sizeof (next_pages_stacked[0]))) {
    index = mp_data->free_layer_mapping ? next_pages_free [index] : next_pages_stacked [index];
    if (index >= 0) {
      mp_ui->central_stack->setCurrentIndex (index);
      enter_page ();
    }
  } 

  update ();
  END_PROTECTED
}

void 
GerberImportDialog::last_page ()
{
  // "safe" commit
  try {
    commit_page ();
  } catch (...) { }

  BEGIN_PROTECTED 

  int index = mp_ui->central_stack->currentIndex ();
  if (index >= 0 && index < int (sizeof (next_pages_stacked) / sizeof (next_pages_stacked[0]))) {
    index = mp_data->free_layer_mapping ? prev_pages_free [index] : prev_pages_stacked [index];
    if (index >= 0) {
      mp_ui->central_stack->setCurrentIndex (index);
    }
  } 

  update ();
  END_PROTECTED
}

void  
GerberImportDialog::add_target_layer ()
{
  QTreeWidget *tree = mp_data->free_layer_mapping ? mp_ui->free_layout_layers_tree : mp_ui->layout_layers_tree;

  lay::NewLayerPropertiesDialog lp_dialog (this);

  db::LayerProperties props;

  //  prefill with the current item's data ...
  if (tree->currentItem ()) {
    int n = tree->currentItem ()->data (0, Qt::UserRole).toInt ();
    if (n >= 0 && n < int (mp_data->layout_layers.size ())) {
      props = mp_data->layout_layers [n];
    }
  }

  if (lp_dialog.exec_dialog (props)) {
    mp_data->layout_layers.push_back (props);
    update ();
    tree->setCurrentItem (tree->topLevelItem (int (mp_data->layout_layers.size () - 1)));
  }
}

void  
GerberImportDialog::delete_target_layer ()
{
  QTreeWidget *tree = mp_data->free_layer_mapping ? mp_ui->free_layout_layers_tree : mp_ui->layout_layers_tree;

  QList<QTreeWidgetItem *> selected = tree->selectedItems ();

  std::set <int> selected_indices;
  for (QList<QTreeWidgetItem *>::const_iterator s = selected.begin (); s != selected.end (); ++s) {
    int n = (*s)->data (0, Qt::UserRole).toInt ();
    if (n >= 0 && n < int (mp_data->layout_layers.size ())) {
      selected_indices.insert (n);
    }
  }

  std::vector <db::LayerProperties>::iterator lw = mp_data->layout_layers.begin ();
  for (std::vector <db::LayerProperties>::iterator l = mp_data->layout_layers.begin (); l != mp_data->layout_layers.end (); ++l) {
    if (selected_indices.find (int (std::distance (mp_data->layout_layers.begin (), l))) == selected_indices.end ()) {
      *lw++ = *l;
    }
  }
  mp_data->layout_layers.erase (lw, mp_data->layout_layers.end ());

  update ();
}

void  
GerberImportDialog::move_target_layer_up ()
{
  QTreeWidget *tree = mp_data->free_layer_mapping ? mp_ui->free_layout_layers_tree : mp_ui->layout_layers_tree;

  QList<QTreeWidgetItem *> selected = tree->selectedItems ();

  std::set <int> selected_indices;
  for (QList<QTreeWidgetItem *>::const_iterator s = selected.begin (); s != selected.end (); ++s) {
    int n = (*s)->data (0, Qt::UserRole).toInt ();
    if (n >= 0 && n < int (mp_data->layout_layers.size ())) {
      selected_indices.insert (n);
    }
  }

  QTreeWidgetItem *current = tree->currentItem ();
  int n_current = current ? current->data (0, Qt::UserRole).toInt () : -1;

  for (std::vector <db::LayerProperties>::iterator l = mp_data->layout_layers.begin (); l != mp_data->layout_layers.end (); ++l) {
    int n = int (std::distance (mp_data->layout_layers.begin (), l));
    if (selected_indices.find (n + 1) != selected_indices.end () && selected_indices.find (n) == selected_indices.end ()) {
      std::swap (mp_data->layout_layers [n + 1], mp_data->layout_layers [n]);
      selected_indices.erase (n + 1);
      selected_indices.insert (n);
      if (n_current == n + 1) {
        n_current = n;
      }
    }
  }

  update ();

  // select the new items
  for (std::set <int>::const_iterator s = selected_indices.begin (); s != selected_indices.end (); ++s) {
    tree->selectionModel ()->select (tree->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    tree->selectionModel ()->select (tree->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void  
GerberImportDialog::move_target_layer_down ()
{
  QTreeWidget *tree = mp_data->free_layer_mapping ? mp_ui->free_layout_layers_tree : mp_ui->layout_layers_tree;

  QList<QTreeWidgetItem *> selected = tree->selectedItems ();

  std::set <int> selected_indices;
  for (QList<QTreeWidgetItem *>::const_iterator s = selected.begin (); s != selected.end (); ++s) {
    int n = (*s)->data (0, Qt::UserRole).toInt ();
    if (n >= 0 && n < int (mp_data->layout_layers.size ())) {
      selected_indices.insert (n);
    }
  }

  QTreeWidgetItem *current = tree->currentItem ();
  int n_current = current ? current->data (0, Qt::UserRole).toInt () : -1;

  for (std::vector <db::LayerProperties>::iterator l = mp_data->layout_layers.end (); l != mp_data->layout_layers.begin (); ) {
    --l;
    int n = int (std::distance (mp_data->layout_layers.begin (), l));
    if (selected_indices.find (n - 1) != selected_indices.end () && selected_indices.find (n) == selected_indices.end ()) {
      std::swap (mp_data->layout_layers [n - 1], mp_data->layout_layers [n]);
      selected_indices.erase (n - 1);
      selected_indices.insert (n);
      if (n_current == n - 1) {
        n_current = n;
      }
    }
  }

  update ();

  // select the new items
  for (std::set <int>::const_iterator s = selected_indices.begin (); s != selected_indices.end (); ++s) {
    tree->selectionModel ()->select (tree->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    tree->selectionModel ()->select (mp_ui->layout_layers_tree->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void    
GerberImportDialog::add_free_file ()
{
  // "safe" commit
  try {
    commit_page ();
  } catch (...) { }

  QTreeWidget *tree = mp_ui->free_files_tree;

  std::vector <std::string> new_files;

  lay::FileDialog open_dialog (this, tl::to_string (QObject::tr ("Load PCB data file")), tl::to_string (QObject::tr ("All files (*)")));
  if (open_dialog.get_open (new_files, mp_data->base_dir)) {

    std::sort (new_files.begin (), new_files.end ());

    QDir base_dir (tl::to_qstring (mp_data->base_dir));

    for (std::vector <std::string>::const_iterator f = new_files.begin (); f != new_files.end (); ++f) {
      mp_data->free_files.push_back (db::GerberFreeFileDescriptor ());
      mp_data->free_files.back ().filename = tl::to_string (base_dir.relativeFilePath (tl::to_qstring (*f)));
    }

    update ();
    tree->setCurrentItem (tree->topLevelItem (int (mp_data->free_files.size () - 1)));

  }
}

void    
GerberImportDialog::delete_free_file ()
{
  QTreeWidget *tree = mp_ui->free_files_tree;

  QList<QTreeWidgetItem *> selected = tree->selectedItems ();

  std::set <int> selected_indices;
  for (QList<QTreeWidgetItem *>::const_iterator s = selected.begin (); s != selected.end (); ++s) {
    int n = (*s)->data (0, Qt::UserRole + 1).toInt ();
    if (n >= 0 && n < int (mp_data->free_files.size ())) {
      selected_indices.insert (n);
    }
  }

  // "safe" commit
  try {
    commit_page ();
  } catch (...) { }

  std::vector <db::GerberFreeFileDescriptor>::iterator lw = mp_data->free_files.begin ();
  for (std::vector <db::GerberFreeFileDescriptor>::iterator l = mp_data->free_files.begin (); l != mp_data->free_files.end (); ++l) {
    if (selected_indices.find (int (std::distance (mp_data->free_files.begin (), l))) == selected_indices.end ()) {
      *lw++ = *l;
    }
  }
  mp_data->free_files.erase (lw, mp_data->free_files.end ());

  update ();
}

void    
GerberImportDialog::move_free_file_up ()
{
  QTreeWidget *tree = mp_ui->free_files_tree;

  QList<QTreeWidgetItem *> selected = tree->selectedItems ();

  std::set <int> selected_indices;
  for (QList<QTreeWidgetItem *>::const_iterator s = selected.begin (); s != selected.end (); ++s) {
    int n = (*s)->data (0, Qt::UserRole + 1).toInt ();
    if (n >= 0 && n < int (mp_data->free_files.size ())) {
      selected_indices.insert (n);
    }
  }

  QTreeWidgetItem *current = tree->currentItem ();
  int n_current = current ? current->data (0, Qt::UserRole + 1).toInt () : -1;

  // "safe" commit
  try {
    commit_page ();
  } catch (...) { }

  for (std::vector <db::GerberFreeFileDescriptor>::iterator l = mp_data->free_files.begin (); l != mp_data->free_files.end (); ++l) {
    int n = int (std::distance (mp_data->free_files.begin (), l));
    if (selected_indices.find (n + 1) != selected_indices.end () && selected_indices.find (n) == selected_indices.end ()) {
      std::swap (mp_data->free_files [n + 1], mp_data->free_files [n]);
      selected_indices.erase (n + 1);
      selected_indices.insert (n);
      if (n_current == n + 1) {
        n_current = n;
      }
    }
  }

  update ();

  // select the new items
  for (std::set <int>::const_iterator s = selected_indices.begin (); s != selected_indices.end (); ++s) {
    tree->selectionModel ()->select (tree->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    tree->selectionModel ()->select (mp_ui->layout_layers_tree->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void    
GerberImportDialog::move_free_file_down ()
{
  QTreeWidget *tree = mp_ui->free_files_tree;

  QList<QTreeWidgetItem *> selected = tree->selectedItems ();

  std::set <int> selected_indices;
  for (QList<QTreeWidgetItem *>::const_iterator s = selected.begin (); s != selected.end (); ++s) {
    int n = (*s)->data (0, Qt::UserRole + 1).toInt ();
    if (n >= 0 && n < int (mp_data->free_files.size ())) {
      selected_indices.insert (n);
    }
  }

  QTreeWidgetItem *current = tree->currentItem ();
  int n_current = current ? current->data (0, Qt::UserRole + 1).toInt () : -1;

  // "safe" commit
  try {
    commit_page ();
  } catch (...) { }

  for (std::vector <db::GerberFreeFileDescriptor>::iterator l = mp_data->free_files.end (); l != mp_data->free_files.begin (); ) {
    --l;
    int n = int (std::distance (mp_data->free_files.begin (), l));
    if (selected_indices.find (n - 1) != selected_indices.end () && selected_indices.find (n) == selected_indices.end ()) {
      std::swap (mp_data->free_files [n - 1], mp_data->free_files [n]);
      selected_indices.erase (n - 1);
      selected_indices.insert (n);
      if (n_current == n - 1) {
        n_current = n;
      }
    }
  }

  update ();

  // select the new items
  for (std::set <int>::const_iterator s = selected_indices.begin (); s != selected_indices.end (); ++s) {
    tree->selectionModel ()->select (tree->model ()->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }

  if (n_current >= 0) {
    tree->selectionModel ()->select (mp_ui->layout_layers_tree->model ()->index (n_current, 0), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void 
GerberImportDialog::free_layer_mapping_item_clicked (QTreeWidgetItem *item, int column)
{
  if (column > 0) {
    bool value = ! item->data (column, Qt::UserRole).toBool ();
    item->setData (column, Qt::UserRole, value);
    item->setData (column, Qt::DecorationRole, QVariant (QIcon (QString::fromUtf8 (value ? ":checked_16px.png" : ":unchecked_16px.png"))));
  }
}

void
GerberImportDialog::reset_free_mapping ()
{
  QTreeWidget *tree = mp_ui->free_layer_mapping_tree;
  QList<QTreeWidgetItem *> selected = tree->selectedItems ();

  for (QList<QTreeWidgetItem *>::const_iterator s = selected.begin (); s != selected.end (); ++s) {
    for (int column = 1; column <= int (mp_data->layout_layers.size ()); ++column) {
      (*s)->setData (column, Qt::UserRole, false);
      (*s)->setData (column, Qt::DecorationRole, QVariant (QIcon (QString::fromUtf8 (":unchecked_16px.png"))));
    }
  }
}

void   
GerberImportDialog::layout_layer_double_clicked (QTreeWidgetItem *, int)
{
  QTreeWidget *tree = mp_data->free_layer_mapping ? mp_ui->free_layout_layers_tree : mp_ui->layout_layers_tree;

  if (tree->currentItem ()) {

    int n = tree->currentItem ()->data (0, Qt::UserRole).toInt ();
    if (n >= 0 && n < int (mp_data->layout_layers.size ())) {

      lay::NewLayerPropertiesDialog lp_dialog (this);

      db::LayerProperties props = mp_data->layout_layers [n];
      if (lp_dialog.exec_dialog (props)) {
        mp_data->layout_layers [n] = props;
        update ();
      }

    }

  }
}

struct FilePositionCompare
{
  bool operator() (const std::pair<db::GerberMetaData, std::string> &a, const std::pair<db::GerberMetaData, std::string> &b)
  {
    if ((a.first.cu_layer_number == 0) != (b.first.cu_layer_number == 0)) {
      return (a.first.cu_layer_number == 0) < (b.first.cu_layer_number == 0);
    }
    if (a.first.cu_layer_number != b.first.cu_layer_number) {
      return a.first.cu_layer_number < b.first.cu_layer_number;
    }
    if ((a.first.from_cu == 0) != (b.first.from_cu == 0)) {
      return (a.first.from_cu == 0) < (b.first.from_cu == 0);
    }
    if (a.first.from_cu != b.first.from_cu) {
      return a.first.from_cu < b.first.from_cu;
    }
    return a.second < b.second;
  }
};

void
GerberImportDialog::enter_page ()
{
  int page = mp_ui->central_stack->currentIndex ();

  if (page == 5) {

    //  --- Free Files page

    if (mp_data->free_files.empty ()) {

      //  scan the files in the directory and populate the file list

      QDir dir (tl::to_qstring (mp_data->base_dir));
      if (dir.exists ()) {

        QStringList filters;
        filters << tl::to_qstring ("*.gbr");
        filters << tl::to_qstring ("*.GBR");

        std::vector<std::pair<db::GerberMetaData, std::string> > files;

        QStringList entries = dir.entryList (filters);
        for (QStringList::const_iterator e = entries.begin (); e != entries.end (); ++e) {
          db::GerberMetaData data = db::GerberImporter::scan (tl::to_string (dir.filePath (*e)));
          files.push_back (std::make_pair (data, tl::to_string (*e)));
        }

        std::sort (files.begin (), files.end (), FilePositionCompare ());

        if (files.empty ()) {
          return;
        }
        if (QMessageBox::question (this, tr ("Populate Project"), tr ("Some files have been found in the specified base directory.\nIf these files contain file attributes, the project can be initialized properly.\n\nPopulate project from these files?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
          return;
        }

        mp_data->layout_layers.clear ();

        int min_layer = 0, max_layer = 0;
        for (std::vector<std::pair<db::GerberMetaData, std::string> >::const_iterator f = files.begin (); f != files.end (); ++f) {
          if (f->first.cu_layer_number > 0) {
            if (min_layer == 0 || min_layer > f->first.cu_layer_number) {
              min_layer = f->first.cu_layer_number;
            }
            if (max_layer == 0 || max_layer < f->first.cu_layer_number) {
              max_layer = f->first.cu_layer_number;
            }
          }
        }

        std::map<int, int> l2l;
        std::map<int, int> l2v;

        if (min_layer > 0) {
          for (int l = min_layer; l <= max_layer; ++l) {
            l2l.insert (std::make_pair (l, int (mp_data->layout_layers.size ())));
            mp_data->layout_layers.push_back (db::LayerProperties (l * 2, 0, "Cu" + tl::to_string (l)));
            if (l < max_layer) {
              l2v.insert (std::make_pair (l, int (mp_data->layout_layers.size ())));
              mp_data->layout_layers.push_back (db::LayerProperties (l * 2 + 1, 0, "Via" + tl::to_string (l)));
            }
          }
        }

        int next_layer = max_layer * 2;
        int hole_num = 0, profile_num = 0, legend_num = 0, solder_num = 0;

        for (std::vector<std::pair<db::GerberMetaData, std::string> >::const_iterator f = files.begin (); f != files.end (); ++f) {

          mp_data->free_files.push_back (db::GerberFreeFileDescriptor ());
          mp_data->free_files.back ().filename = tl::to_string (f->second);

          std::vector<int> layers;

          if (f->first.function == db::GerberMetaData::Copper) {
            if (l2l.find (f->first.cu_layer_number) != l2l.end ()) {
              layers.push_back (l2l [f->first.cu_layer_number]);
            }
          } else if (f->first.function == db::GerberMetaData::PlatedHole) {
            for (int l = std::min (f->first.from_cu, f->first.to_cu); l < std::max (f->first.from_cu, f->first.to_cu); ++l) {
              if (l2v.find (l) != l2v.end ()) {
                layers.push_back (l2v [l]);
              }
            }
          } else if (f->first.function == db::GerberMetaData::NonPlatedHole || f->first.function == db::GerberMetaData::NonPlatedHole) {
            layers.push_back (int (mp_data->layout_layers.size ()));
            mp_data->layout_layers.push_back (db::LayerProperties (++next_layer, 0, "Hole" + tl::to_string (++hole_num)));
          } else if (f->first.function == db::GerberMetaData::Profile) {
            layers.push_back (int (mp_data->layout_layers.size ()));
            mp_data->layout_layers.push_back (db::LayerProperties (++next_layer, 0, "Profile" + tl::to_string (++profile_num)));
          } else if (f->first.function == db::GerberMetaData::Legend) {
            layers.push_back (int (mp_data->layout_layers.size ()));
            mp_data->layout_layers.push_back (db::LayerProperties (++next_layer, 0, "Legend" + tl::to_string (++legend_num)));
          } else if (f->first.function == db::GerberMetaData::SolderMask) {
            layers.push_back (int (mp_data->layout_layers.size ()));
            mp_data->layout_layers.push_back (db::LayerProperties (++next_layer, 0, "SolderMask" + tl::to_string (++solder_num)));
          }

          mp_data->free_files.back ().layout_layers = layers;

        }

      }

    }

  }
}

void 
GerberImportDialog::commit_page ()
{
  int page = mp_ui->central_stack->currentIndex ();

  if (page == 0) {

    //  --- General page
    mp_data->base_dir = tl::to_string (mp_ui->base_dir_le->text ());
    mp_data->free_layer_mapping = mp_ui->free_mapping_cb->isChecked ();
    if (mp_ui->import_into_rb->isChecked ()) {
      mp_data->mode = db::GerberImportData::ModeIntoLayout;
    } else if (mp_ui->import_new_panel_rb->isChecked ()) {
      mp_data->mode = db::GerberImportData::ModeNewPanel;
    } else if (mp_ui->import_same_panel_rb->isChecked ()) {
      mp_data->mode = db::GerberImportData::ModeSamePanel;
    }

  } else if (page == 1) {

    //  --- Layout Layers page
    //  Nothing to commit here. It's done instantaneous.
    
    if ((mp_data->layout_layers.size () % 2) == 0) {
      throw tl::Exception (tl::to_string (QObject::tr ("The layout layer list must start and end with a metal layer")));
    }

  } else if (page == 2) {

    if (mp_ui->mounting_cbx->currentIndex () == 0) {
      mp_data->mounting = db::GerberImportData::MountingTop;
    } else {
      mp_data->mounting = db::GerberImportData::MountingBottom;
    }

    mp_data->num_metal_layers = -1;
    mp_data->num_via_types = -1;

    tl::from_string_ext (tl::to_string (mp_ui->num_metal_le->text ()), mp_data->num_metal_layers);
    tl::from_string_ext (tl::to_string (mp_ui->num_via_le->text ()), mp_data->num_via_types);

    if (mp_data->num_metal_layers < 0) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid number of metal layers")));
    }
    if (mp_data->num_via_types < 0) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid number of via layers")));
    }
    if (mp_data->num_metal_layers > int (mp_data->layout_layers.size () / 2 + 1)) {
      throw tl::Exception (tl::to_string (QObject::tr ("Not enough metal layer defined in layout layers to map the requested number of metal layers")));
    }

    if (int (mp_data->artwork_files.size ()) > mp_data->num_metal_layers) {
      mp_data->artwork_files.erase (mp_data->artwork_files.begin () + mp_data->num_metal_layers, mp_data->artwork_files.end ());
    } else {
      while (int (mp_data->artwork_files.size ()) < mp_data->num_metal_layers) {
        mp_data->artwork_files.push_back (db::GerberArtworkFileDescriptor ());
      }
    }

    if (int (mp_data->drill_files.size ()) > mp_data->num_via_types) {
      mp_data->drill_files.erase (mp_data->drill_files.begin () + mp_data->num_via_types, mp_data->drill_files.end ());
    } else {
      while (int (mp_data->drill_files.size ()) < mp_data->num_via_types) {
        mp_data->drill_files.push_back (db::GerberDrillFileDescriptor ());
      }
    }

  } else if (page == 3) {

    //  --- Artwork Files page
    
    //  Since we set the edit triggers to "currentChanged", this will commit the data:
    mp_ui->artwork_files_tree->setCurrentIndex (QModelIndex ());

    int n = 0;
    for (std::vector <db::GerberArtworkFileDescriptor>::iterator l = mp_data->artwork_files.begin (); l != mp_data->artwork_files.end (); ++l, ++n) {
      QTreeWidgetItem *item = mp_ui->artwork_files_tree->topLevelItem (n);
      if (item) {
        l->filename = tl::to_string (item->data (1, Qt::UserRole).toString ());
      } else {
        *l = db::GerberArtworkFileDescriptor ();
      }
    }

    /*  it might be a useful feature ...
    for (std::vector <GerberArtworkFileDescriptor>::const_iterator l = mp_data->artwork_files.begin (); l != mp_data->artwork_files.end (); ++l) {
      if (l->filename.empty ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("Some file names are missing")));
      }
    }
    */

  } else if (page == 4) {

    //  --- Drill Types And Files page

    //  Since we set the edit triggers to "currentChanged", this will commit the data:
    mp_ui->drill_files_tree->setCurrentIndex (QModelIndex ());

    int n = 0;
    for (std::vector <db::GerberDrillFileDescriptor>::iterator l = mp_data->drill_files.begin (); l != mp_data->drill_files.end (); ++l, ++n) {
      QTreeWidgetItem *item = mp_ui->drill_files_tree->topLevelItem (n);
      if (item) {
        l->start = item->data (1, Qt::UserRole).toInt ();
        l->stop = item->data (2, Qt::UserRole).toInt ();
        if (l->stop <= l->start) {
          throw tl::Exception (tl::to_string (QObject::tr ("Stop layers must be past start layers")));
        }
        l->filename = tl::to_string (item->data (3, Qt::UserRole).toString ());
      } else {
        *l = db::GerberDrillFileDescriptor ();
      }
    }

    for (std::vector <db::GerberDrillFileDescriptor>::const_iterator l = mp_data->drill_files.begin (); l != mp_data->drill_files.end (); ++l) {
      /*  it might be a useful feature ...
      if (l->filename.empty ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("Some file names are missing")));
      }
      */
      if (l->start < 0) {
        throw tl::Exception (tl::to_string (QObject::tr ("Some start layer specifications are missing")));
      }
      if (l->stop < 0) {
        throw tl::Exception (tl::to_string (QObject::tr ("Some stop layer specifications are missing")));
      }
    }

  } else if (page == 5) {

    //  --- Free Files page
    //  Since we set the edit triggers to "currentChanged", this will commit the data:
    mp_ui->free_files_tree->setCurrentIndex (QModelIndex ());

    int n = 0;
    for (std::vector <db::GerberFreeFileDescriptor>::iterator l = mp_data->free_files.begin (); l != mp_data->free_files.end (); ++l, ++n) {
      QTreeWidgetItem *item = mp_ui->free_files_tree->topLevelItem (n);
      if (item) {
        l->filename = tl::to_string (item->data (0, Qt::UserRole).toString ());
      } else {
        *l = db::GerberFreeFileDescriptor ();
      }
    }

    /*  it might be a useful feature ...
    for (std::vector <GerberFreeFileDescriptor>::iterator l = mp_data->free_files.begin (); l != mp_data->free_files.end (); ++l, ++n) {
      if (l->filename.empty ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("Some file names are missing")));
      }
    }
    */

    //  add layers for all free files if no layer is defined yet. Add additional layers. Try to find some useful numbering scheme.

    int max_layer = 0;
    for (std::vector <db::LayerProperties>::const_iterator l = mp_data->layout_layers.begin (); l != mp_data->layout_layers.end (); ++l) {
      max_layer = std::max (max_layer, l->layer);
    }

    for (size_t i = 0; i < mp_data->free_files.size (); ++i) {

      std::vector<int> valid_layers;
      for (std::vector<int>::const_iterator l = mp_data->free_files[i].layout_layers.begin (); l != mp_data->free_files[i].layout_layers.end (); ++l) {
        if (*l >= 0 && *l < int (mp_data->layout_layers.size ())) {
          valid_layers.push_back (*l);
        }
      }

      mp_data->free_files[i].layout_layers = valid_layers;

      //  Add a stupid 1:1 mapping if no layers are mapped for the next file.
      if (valid_layers.empty ()) {

        mp_data->free_files[i].layout_layers.push_back (int (mp_data->layout_layers.size ()));

        mp_data->layout_layers.push_back (db::LayerProperties ());
        mp_data->layout_layers.back ().layer = ++max_layer;
        mp_data->layout_layers.back ().datatype = 0;

      }

    }

  } else if (page == 6) {

    //  --- Free Layout Layers page
    //  no commit needed - done immediately

    //  reset any layers not present any longer
    for (std::vector <db::GerberFreeFileDescriptor>::iterator l = mp_data->free_files.begin (); l != mp_data->free_files.end (); ++l) {
      std::vector <int> indices;
      for (std::vector <int>::const_iterator i = l->layout_layers.begin (); i != l->layout_layers.end (); ++i) {
        if (*i < int (mp_data->layout_layers.size ())) {
          indices.push_back (*i);
        }
      }
      l->layout_layers = indices;
    }

  } else if (page == 7) {

    //  --- Free Files And Layer Mapping page
    //  Since we set the edit triggers to "currentChanged", this will commit the data:
    mp_ui->free_layer_mapping_tree->setCurrentIndex (QModelIndex ());

    int n = 0;
    for (std::vector <db::GerberFreeFileDescriptor>::iterator l = mp_data->free_files.begin (); l != mp_data->free_files.end (); ++l, ++n) {
      QTreeWidgetItem *item = mp_ui->free_layer_mapping_tree->topLevelItem (n);
      if (item) {

        l->filename = tl::to_string (item->data (0, Qt::UserRole).toString ());

        std::vector <int> indices;
        for (int i = 0; i < int (mp_data->layout_layers.size ()); ++i) {
          if (item->data (i + 1, Qt::UserRole).toBool ()) {
            indices.push_back (i);
          }
        }
        l->layout_layers = indices;

      } else {
        *l = db::GerberFreeFileDescriptor ();
      }
    }

    /*  it might be a useful feature ...
    for (std::vector <GerberFreeFileDescriptor>::iterator l = mp_data->free_layer_mapping_tree.begin (); l != mp_data->free_layer_mapping_tree.end (); ++l, ++n) {
      if (l->filename.empty ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("Some file names are missing")));
      }
    }
    */

  } else if (page == 8) {

    //  --- Coordinate Mapping page
    QLineEdit *(coord_editors[][4]) = {
      { mp_ui->pcb_x1_le, mp_ui->pcb_y1_le, mp_ui->layout_x1_le, mp_ui->layout_y1_le },
      { mp_ui->pcb_x2_le, mp_ui->pcb_y2_le, mp_ui->layout_x2_le, mp_ui->layout_y2_le },
      { mp_ui->pcb_x3_le, mp_ui->pcb_y3_le, mp_ui->layout_x3_le, mp_ui->layout_y3_le }
    };

    mp_data->reference_points.clear ();
    for (unsigned int i = 0; i < sizeof (coord_editors) / sizeof (coord_editors [0]); ++i) {

      std::string t_pcb_x (tl::to_string (coord_editors [i][0]->text ()));
      tl::Extractor pcb_x (t_pcb_x.c_str ());

      std::string t_pcb_y (tl::to_string (coord_editors [i][1]->text ()));
      tl::Extractor pcb_y (t_pcb_y.c_str ());

      std::string t_layout_x (tl::to_string (coord_editors [i][2]->text ()));
      tl::Extractor layout_x (t_layout_x.c_str ());

      std::string t_layout_y (tl::to_string (coord_editors [i][3]->text ()));
      tl::Extractor layout_y (t_layout_y.c_str ());

      if (pcb_x.at_end () || pcb_y.at_end () || layout_x.at_end () || layout_y.at_end ()) {

        if (! pcb_x.at_end () || ! pcb_y.at_end () || ! layout_x.at_end () || ! layout_y.at_end ()) {
          throw tl::Exception (tl::to_string (QObject::tr ("All coordinates (PCB and layout) must be specified for a reference point")));
        }

      } else {

        double x, y;

        pcb_x.read (x);
        pcb_x.expect_end ();
        pcb_y.read (y);
        pcb_y.expect_end ();
        db::DPoint pcb (x, y);

        layout_x.read (x);
        layout_x.expect_end ();
        layout_y.read (y);
        layout_y.expect_end ();
        db::DPoint layout (x, y);

        mp_data->reference_points.push_back (std::make_pair (pcb, layout));

      }
    }

    std::string t (tl::to_string (mp_ui->explicit_trans_le->text ()));
    tl::Extractor ex (t.c_str ());
    mp_data->explicit_trans = db::DCplxTrans ();
    if (! ex.at_end ()) {
      ex.read (mp_data->explicit_trans);
      ex.expect_end ();
    }

  } else if (page == 9) {

    //  --- Options page
    mp_data->layer_properties_file = tl::to_string (mp_ui->layer_properties_file_le->text ());

    std::string t (tl::to_string (mp_ui->circle_points_le->text ()));
    tl::Extractor ex (t.c_str ());
    if (! ex.at_end ()) {
      ex.read (mp_data->num_circle_points);
      ex.expect_end ();
      if (mp_data->num_circle_points < 4) {
        throw tl::Exception (tl::to_string (QObject::tr ("Number of points per circle must be 4 at least")));
      }
    } else {
      mp_data->num_circle_points = -1;
    }

    mp_data->merge_flag = mp_ui->merge_cb->isChecked ();
    mp_data->invert_negative_layers = mp_ui->invert_cb->isChecked ();
    tl::from_string_ext (tl::to_string (mp_ui->border_le->text ()), mp_data->border);

    bool import_into = (mp_data->mode == db::GerberImportData::ModeIntoLayout);
    if (! import_into) {

      tl::from_string_ext (tl::to_string (mp_ui->dbu_le->text ()), mp_data->dbu);
      if (mp_data->dbu < 1e-6) {
        throw tl::Exception (tl::to_string (QObject::tr ("Database unit must not be negative or zero")));
      }

      mp_data->topcell_name = tl::to_string (mp_ui->topcell_le->text ());

    }

  } 
}

void 
GerberImportDialog::update ()
{
  int n, nn;

  std::string section_headers[] = {
    tl::to_string (QObject::tr ("General")),
    tl::to_string (QObject::tr ("Layout Layers")),
    tl::to_string (QObject::tr ("Layer Stack")),
    tl::to_string (QObject::tr ("Artwork Files")),
    tl::to_string (QObject::tr ("Drill Types And Files")),
    tl::to_string (QObject::tr ("Files")),
    tl::to_string (QObject::tr ("Layout Layers")),
    tl::to_string (QObject::tr ("Layer Mapping")),
    tl::to_string (QObject::tr ("Coordinate Mapping")),
    tl::to_string (QObject::tr ("Options"))
  };

  int page = mp_ui->central_stack->currentIndex ();
  if (page < 0 || page >= int (sizeof (section_headers) / sizeof (section_headers [0]))) {
    return;
  }

  mp_ui->last_pb->setEnabled (page > 0);
  mp_ui->next_pb->setEnabled (page < int (sizeof (next_pages_stacked) / sizeof (next_pages_stacked [0]) - 1));
  mp_ui->section_header_lbl->setText (tl::to_qstring (section_headers [page]));

  //  --- General page
  mp_ui->base_dir_le->setText (tl::to_qstring (mp_data->base_dir));
  mp_ui->free_mapping_cb->setChecked (mp_data->free_layer_mapping);
  mp_ui->import_into_rb->setChecked (mp_data->mode == db::GerberImportData::ModeIntoLayout);
  mp_ui->import_new_panel_rb->setChecked (mp_data->mode == db::GerberImportData::ModeNewPanel);
  mp_ui->import_same_panel_rb->setChecked (mp_data->mode == db::GerberImportData::ModeSamePanel);

  //  --- Layout Layers page (stacked)
  mp_ui->layout_layers_tree->clear ();
  n = 0;
  nn = mp_ui->layout_layers_tree->topLevelItemCount ();
  for (std::vector <db::LayerProperties>::const_iterator l = mp_data->layout_layers.begin (); l != mp_data->layout_layers.end (); ++l, ++n) {

    QTreeWidgetItem *item;
    if (n < nn) {
      item = mp_ui->layout_layers_tree->topLevelItem (n);
    } else {
      item = new QTreeWidgetItem (mp_ui->layout_layers_tree);
      mp_ui->layout_layers_tree->addTopLevelItem (item);
    }

    std::string label;
    if ((n % 2) == 0) {
      label = tl::sprintf (tl::to_string (QObject::tr ("Metal %d")), n / 2 + 1);
    } else {
      label = tl::sprintf (tl::to_string (QObject::tr ("Via %d-%d")), (n - 1) / 2 + 1, (n - 1) / 2 + 2);
    }

    item->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (label)));
    item->setData (0, Qt::UserRole, QVariant (n));

    item->setData (1, Qt::DisplayRole, QVariant (tl::to_qstring (l->to_string ())));

  }

  while (mp_ui->layout_layers_tree->topLevelItemCount () > n) {
    delete mp_ui->layout_layers_tree->topLevelItem (n);
  }

  mp_ui->layout_layers_tree->clearSelection ();

  //  --- Layer Stack page (stacked)
  mp_ui->mounting_cbx->setCurrentIndex (mp_data->mounting == db::GerberImportData::MountingTop ? 0 : 1);
  mp_ui->num_metal_le->setText (tl::to_qstring (tl::to_string (mp_data->num_metal_layers)));
  mp_ui->num_via_le->setText (tl::to_qstring (tl::to_string (mp_data->num_via_types)));

  //  --- Artwork Files page (stacked)
  mp_ui->artwork_files_tree->clear ();
  n = 0;
  for (std::vector <db::GerberArtworkFileDescriptor>::const_iterator l = mp_data->artwork_files.begin (); l != mp_data->artwork_files.end (); ++l, ++n) {

    QTreeWidgetItem *item = new QTreeWidgetItem (mp_ui->artwork_files_tree);
    item->setFlags (item->flags () | Qt::ItemIsEditable);

    std::string label;
    label = tl::sprintf (tl::to_string (QObject::tr ("File %d")), n + 1);
    if (n == 0) {
      label += tl::to_string (QObject::tr (" (Top)"));
    } else if (n == int (mp_data->artwork_files.size () - 1)) {
      label += tl::to_string (QObject::tr (" (Bottom)"));
    }

    item->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (label)));
    item->setData (0, Qt::UserRole, QVariant (n));

    if (! l->filename.empty ()) {

      item->setData (1, Qt::DisplayRole, QVariant (tl::to_qstring (l->filename)));

      QFileInfo file_info (tl::to_qstring (l->filename));
      if (! mp_data->base_dir.empty () && ! file_info.isAbsolute ()) {
        file_info = QFileInfo (QDir (tl::to_qstring (mp_data->base_dir)), tl::to_qstring (l->filename));
      }

      if (file_info.exists () && file_info.isReadable ()) {
        item->setData (1, Qt::ForegroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Active, QPalette::Text));
        item->setData (1, Qt::BackgroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Active, QPalette::Base));
      } else {
        item->setData (1, Qt::ForegroundRole, QColor (Qt::red));
        item->setData (1, Qt::BackgroundRole, QColor (Qt::red).lighter (180));
      }

    } else {
      item->setData (1, Qt::DisplayRole, QVariant (QObject::tr ("Click here to enter file name")));
      item->setData (1, Qt::ForegroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Disabled, QPalette::Text));
      item->setData (1, Qt::BackgroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Disabled, QPalette::Base));
    }

    item->setData (1, Qt::UserRole, QVariant (tl::to_qstring (l->filename)));

    mp_ui->artwork_files_tree->addTopLevelItem (item);

  }

  //  --- Drill Types And Files page (stacked)
  mp_ui->drill_files_tree->clear ();
  n = 0;
  for (std::vector <db::GerberDrillFileDescriptor>::const_iterator l = mp_data->drill_files.begin (); l != mp_data->drill_files.end (); ++l, ++n) {

    QTreeWidgetItem *item = new QTreeWidgetItem (mp_ui->drill_files_tree);
    item->setFlags (item->flags () | Qt::ItemIsEditable);

    std::string label;
    label = tl::sprintf (tl::to_string (QObject::tr ("File %d")), n + 1);

    item->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (label)));
    item->setData (0, Qt::UserRole, QVariant (n));

    if (l->start >= 0) {
      std::string start = tl::sprintf (tl::to_string (QObject::tr ("Metal %d")), l->start + 1);
      item->setData (1, Qt::DisplayRole, QVariant (tl::to_qstring (start)));
      item->setData (1, Qt::ForegroundRole, mp_ui->drill_files_tree->palette ().color (QPalette::Active, QPalette::Text));
    } else {
      item->setData (1, Qt::DisplayRole, QVariant (QString::fromUtf8 ("...")));
      item->setData (1, Qt::ForegroundRole, mp_ui->drill_files_tree->palette ().color (QPalette::Disabled, QPalette::Text));
    }
    item->setData (1, Qt::UserRole, QVariant (l->start));

    if (l->stop >= 0) {
      std::string stop = tl::sprintf (tl::to_string (QObject::tr ("Metal %d")), l->stop + 1);
      item->setData (2, Qt::DisplayRole, QVariant (tl::to_qstring (stop)));
      item->setData (2, Qt::ForegroundRole, mp_ui->drill_files_tree->palette ().color (QPalette::Active, QPalette::Text));
    } else {
      item->setData (2, Qt::DisplayRole, QVariant (QString::fromUtf8 ("...")));
      item->setData (2, Qt::ForegroundRole, mp_ui->drill_files_tree->palette ().color (QPalette::Disabled, QPalette::Text));
    }
    item->setData (2, Qt::UserRole, QVariant (l->stop));

    if (! l->filename.empty ()) {

      item->setData (3, Qt::DisplayRole, QVariant (tl::to_qstring (l->filename)));

      QFileInfo file_info (tl::to_qstring (l->filename));
      if (! mp_data->base_dir.empty () && ! file_info.isAbsolute ()) {
        file_info = QFileInfo (QDir (tl::to_qstring (mp_data->base_dir)), tl::to_qstring (l->filename));
      }

      if (file_info.exists () && file_info.isReadable ()) {
        item->setData (3, Qt::ForegroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Active, QPalette::Text));
        item->setData (3, Qt::BackgroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Active, QPalette::Base));
      } else {
        item->setData (3, Qt::ForegroundRole, QColor (Qt::red));
        item->setData (3, Qt::BackgroundRole, QColor (Qt::red).lighter (180));
      }

    } else {
      item->setData (3, Qt::DisplayRole, QVariant (QObject::tr ("Click here to enter file name")));
      item->setData (3, Qt::ForegroundRole, mp_ui->drill_files_tree->palette ().color (QPalette::Disabled, QPalette::Text));
      item->setData (3, Qt::BackgroundRole, mp_ui->drill_files_tree->palette ().color (QPalette::Disabled, QPalette::Base));
    }

    item->setData (3, Qt::UserRole, QVariant (tl::to_qstring (l->filename)));

    mp_ui->drill_files_tree->addTopLevelItem (item);

  }

  //  --- Files page (free)
  mp_ui->free_files_tree->clear ();
  n = 0;
  for (std::vector <db::GerberFreeFileDescriptor>::const_iterator l = mp_data->free_files.begin (); l != mp_data->free_files.end (); ++l, ++n) {

    QTreeWidgetItem *item = new QTreeWidgetItem (mp_ui->free_files_tree);
    item->setFlags (item->flags () | Qt::ItemIsEditable);

    item->setData (0, Qt::UserRole + 1, QVariant (n));

    if (! l->filename.empty ()) {

      item->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (l->filename)));

      QFileInfo file_info (tl::to_qstring (l->filename));
      if (! mp_data->base_dir.empty () && ! file_info.isAbsolute ()) {
        file_info = QFileInfo (QDir (tl::to_qstring (mp_data->base_dir)), tl::to_qstring (l->filename));
      }

      if (file_info.exists () && file_info.isReadable ()) {
        item->setData (0, Qt::ForegroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Active, QPalette::Text));
        item->setData (0, Qt::BackgroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Active, QPalette::Base));
      } else {
        item->setData (0, Qt::ForegroundRole, QColor (Qt::red));
        item->setData (0, Qt::BackgroundRole, QColor (Qt::red).lighter (180));
      }

    } else {
      item->setData (0, Qt::DisplayRole, QVariant (QObject::tr ("Click here to enter file name")));
      item->setData (0, Qt::ForegroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Disabled, QPalette::Text));
      item->setData (0, Qt::BackgroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Disabled, QPalette::Base));
    }

    item->setData (0, Qt::UserRole, QVariant (tl::to_qstring (l->filename)));

    mp_ui->free_files_tree->addTopLevelItem (item);

  }

  //  --- Layout Layers page (free)
  mp_ui->free_layout_layers_tree->clear (); // TODO: check whether this is a good solution and simplify the scheme below
  n = 0;
  nn = mp_ui->free_layout_layers_tree->topLevelItemCount ();
  for (std::vector <db::LayerProperties>::const_iterator l = mp_data->layout_layers.begin (); l != mp_data->layout_layers.end (); ++l, ++n) {

    QTreeWidgetItem *item;
    if (n < nn) {
      item = mp_ui->free_layout_layers_tree->topLevelItem (n);
    } else {
      item = new QTreeWidgetItem (mp_ui->free_layout_layers_tree);
      mp_ui->free_layout_layers_tree->addTopLevelItem (item);
    }

    item->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (l->to_string ())));
    item->setData (0, Qt::UserRole, QVariant (n));

  }

  while (mp_ui->free_layout_layers_tree->topLevelItemCount () > n) {
    delete mp_ui->free_layout_layers_tree->topLevelItem (n);
  }

  mp_ui->free_layout_layers_tree->clearSelection ();

  //  --- Files And Layer Mapping page (free)
  mp_ui->free_layer_mapping_tree->clear ();
  n = 0;
  for (std::vector <db::GerberFreeFileDescriptor>::const_iterator l = mp_data->free_files.begin (); l != mp_data->free_files.end (); ++l, ++n) {

    QTreeWidgetItem *item = new QTreeWidgetItem (mp_ui->free_layer_mapping_tree);
    item->setFlags (item->flags () | Qt::ItemIsEditable);

    item->setData (0, Qt::UserRole + 1, QVariant (n));

    if (! l->filename.empty ()) {

      item->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (l->filename)));
      item->setData (0, Qt::ToolTipRole, QVariant (tl::to_qstring (l->filename)));

      QFileInfo file_info (tl::to_qstring (l->filename));
      if (! mp_data->base_dir.empty () && ! file_info.isAbsolute ()) {
        file_info = QFileInfo (QDir (tl::to_qstring (mp_data->base_dir)), tl::to_qstring (l->filename));
      }

      if (file_info.exists () && file_info.isReadable ()) {
        item->setData (0, Qt::ForegroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Active, QPalette::Text));
        item->setData (0, Qt::BackgroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Active, QPalette::Base));
      } else {
        item->setData (0, Qt::ForegroundRole, QColor (Qt::red));
        item->setData (0, Qt::BackgroundRole, QColor (Qt::red).lighter (180));
      }

    } else {
      item->setData (0, Qt::DisplayRole, QVariant (QObject::tr ("Click here to enter file name")));
      item->setData (0, Qt::ForegroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Disabled, QPalette::Text));
      item->setData (0, Qt::BackgroundRole, mp_ui->artwork_files_tree->palette ().color (QPalette::Disabled, QPalette::Base));
    }

    item->setData (0, Qt::UserRole, QVariant (tl::to_qstring (l->filename)));

    for (std::vector <db::LayerProperties>::iterator ll = mp_data->layout_layers.begin (); ll != mp_data->layout_layers.end (); ++ll) {

      bool is_present = false; 
      int layer = int (std::distance (mp_data->layout_layers.begin (), ll));
      for (std::vector <int>::const_iterator i = l->layout_layers.begin (); i != l->layout_layers.end () && ! is_present; ++i) {
        is_present = (*i == layer);
      }

      item->setData (layer + 1, Qt::UserRole, QVariant (is_present));
      item->setData (layer + 1, Qt::DecorationRole, QVariant (QIcon (QString::fromUtf8 (is_present ? ":checked_16px.png" : ":unchecked_16px.png"))));

    }

    mp_ui->free_layer_mapping_tree->addTopLevelItem (item);

  }

  mp_ui->free_layer_mapping_tree->setColumnCount (int (mp_data->layout_layers.size () + 1));
  n = 0;
  mp_ui->free_layer_mapping_tree->headerItem ()->setData (0, Qt::DisplayRole, QVariant (QObject::tr ("File")));
  for (std::vector <db::LayerProperties>::const_iterator l = mp_data->layout_layers.begin (); l != mp_data->layout_layers.end (); ++l, ++n) {

    QString hdr_label;

    db::LayerProperties ll = *l;
    ll.name.clear ();
    hdr_label = tl::to_qstring (ll.to_string ());
    hdr_label += QString::fromUtf8 ("\n");

    if (l->name.size () > 4) {
      hdr_label += tl::to_qstring (std::string (l->name, 0, 4) + "...");
    } else {
      hdr_label += tl::to_qstring (l->name);
    }
      
    mp_ui->free_layer_mapping_tree->headerItem ()->setData (n + 1, Qt::DisplayRole, QVariant (hdr_label));
    mp_ui->free_layer_mapping_tree->headerItem ()->setData (n + 1, Qt::ToolTipRole, QVariant (tl::to_qstring (l->to_string ())));

    if (mp_ui->free_layer_mapping_tree->itemDelegateForColumn (n + 1) == 0) {
      mp_ui->free_layer_mapping_tree->setItemDelegateForColumn (n + 1, new GerberImportDialogNoEditDelegate (mp_ui->free_layer_mapping_tree));
    }

#if QT_VERSION >= 0x050000
    mp_ui->free_layer_mapping_tree->header ()->setSectionResizeMode (n + 1, QHeaderView::ResizeToContents);
#else
    mp_ui->free_layer_mapping_tree->header ()->setResizeMode (n + 1, QHeaderView::ResizeToContents);
#endif

  }
  
  //  --- Coordinate Mapping page
  QLineEdit *(coord_editors[][4]) = {
    { mp_ui->pcb_x1_le, mp_ui->pcb_y1_le, mp_ui->layout_x1_le, mp_ui->layout_y1_le },
    { mp_ui->pcb_x2_le, mp_ui->pcb_y2_le, mp_ui->layout_x2_le, mp_ui->layout_y2_le },
    { mp_ui->pcb_x3_le, mp_ui->pcb_y3_le, mp_ui->layout_x3_le, mp_ui->layout_y3_le }
  };

  for (unsigned int i = 0; i < sizeof (coord_editors) / sizeof (coord_editors [0]); ++i) {
    if (mp_data->reference_points.size () > i) {
      coord_editors[i][0]->setText (tl::to_qstring (tl::to_string (mp_data->reference_points [i].first.x ())));
      coord_editors[i][1]->setText (tl::to_qstring (tl::to_string (mp_data->reference_points [i].first.y ())));
      coord_editors[i][2]->setText (tl::to_qstring (tl::to_string (mp_data->reference_points [i].second.x ())));
      coord_editors[i][3]->setText (tl::to_qstring (tl::to_string (mp_data->reference_points [i].second.y ())));
    } else {
      for (unsigned int j = 0; j < 4; ++j) {
        coord_editors[i][j]->setText (QString ());
      }
    }
  }

  if (mp_data->explicit_trans == db::DCplxTrans ()) {
    mp_ui->explicit_trans_le->setText (QString ());
  } else {
    mp_ui->explicit_trans_le->setText (tl::to_qstring (mp_data->explicit_trans.to_string ()));
  }

  //  --- Options page
  mp_ui->layer_properties_file_le->setText (tl::to_qstring (mp_data->layer_properties_file));
  if (mp_data->num_circle_points < 4) {
    mp_ui->circle_points_le->setText (QString ());
  } else {
    mp_ui->circle_points_le->setText (tl::to_qstring (tl::to_string (mp_data->num_circle_points)));
  }

  mp_ui->merge_cb->setChecked (mp_data->merge_flag);
  mp_ui->invert_cb->setChecked (mp_data->invert_negative_layers);
  mp_ui->border_le->setText (tl::to_qstring (tl::to_string (mp_data->border)));

  bool import_into = (mp_data->mode == db::GerberImportData::ModeIntoLayout);
  if (import_into) {
    mp_ui->dbu_le->setText (QString ());
    mp_ui->topcell_le->setText (QString ());
  } else {
    mp_ui->dbu_le->setText (tl::to_qstring (tl::to_string (mp_data->dbu)));
    mp_ui->topcell_le->setText (tl::to_qstring (mp_data->topcell_name));
  }

  //  In "into layout" mode, the top cell and database unit are given by the current layout and cannot be
  //  modified.
  mp_ui->dbu_le->setEnabled (! import_into);
  mp_ui->dbu_lbl1->setEnabled (! import_into);
  mp_ui->dbu_lbl2->setEnabled (! import_into);
  mp_ui->topcell_le->setEnabled (! import_into);
  mp_ui->topcell_lbl->setEnabled (! import_into);
}

}


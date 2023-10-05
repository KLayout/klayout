
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#include "tlInternational.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"
#include "dbPCellHeader.h"
#include "edtPCellParametersPage.h"
#include "edtConfig.h"
#include "edtService.h"
#include "edtEditorOptionsPages.h"
#include "edtPropertiesPageUtils.h"
#include "tlExceptions.h"
#include "layPlugin.h"
#include "layLayoutViewBase.h"
#include "layCellSelectionForm.h"
#include "layQtTools.h"
#include "ui_EditorOptionsGeneric.h"
#include "ui_EditorOptionsPath.h"
#include "ui_EditorOptionsText.h"
#include "ui_EditorOptionsInst.h"
#include "ui_EditorOptionsInstPCellParam.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QToolButton>
#include <QCompleter>

namespace edt
{

// ------------------------------------------------------------------
//  Configures a value from a line edit

template <class Value>
static void configure_from_line_edit (lay::Dispatcher *dispatcher, QLineEdit *le, const std::string &cfg_name)
{
  try {
    Value value = Value (0);
    tl::from_string_ext (tl::to_string (le->text ()), value);
    dispatcher->config_set (cfg_name, tl::to_string (value));
    lay::indicate_error (le, (tl::Exception *) 0);
  } catch (tl::Exception &ex) {
    lay::indicate_error (le, &ex);
  }
}

// ------------------------------------------------------------------
//  EditorOptionsGeneric implementation

EditorOptionsGeneric::EditorOptionsGeneric (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : EditorOptionsPage (view, dispatcher)
{
  mp_ui = new Ui::EditorOptionsGeneric ();
  mp_ui->setupUi (this);

  connect (mp_ui->grid_cb, SIGNAL (activated (int)), this, SLOT (grid_changed (int)));

  connect (mp_ui->edit_grid_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->grid_cb, SIGNAL (activated (int)), this, SLOT (edited ()));
  connect (mp_ui->move_angle_cb, SIGNAL (activated (int)), this, SLOT (edited ()));
  connect (mp_ui->conn_angle_cb, SIGNAL (activated (int)), this, SLOT (edited ()));
  connect (mp_ui->hier_sel_cbx, SIGNAL (clicked ()), this, SLOT (edited ()));
  connect (mp_ui->hier_copy_mode_cbx, SIGNAL (activated (int)), this, SLOT (edited ()));
  connect (mp_ui->snap_objects_cbx, SIGNAL (clicked ()), this, SLOT (edited ()));
  connect (mp_ui->snap_objects_to_grid_cbx, SIGNAL (clicked ()), this, SLOT (edited ()));
  connect (mp_ui->max_shapes_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->show_shapes_cbx, SIGNAL (clicked ()), this, SLOT (edited ()));
}

EditorOptionsGeneric::~EditorOptionsGeneric ()
{
  delete mp_ui;
  mp_ui = 0;
}

std::string
EditorOptionsGeneric::title () const
{
  return tl::to_string (QObject::tr ("Basic Editing"));
}

void
EditorOptionsGeneric::apply (lay::Dispatcher *root)
{
  //  Edit grid

  EditGridConverter egc;
  if (mp_ui->grid_cb->currentIndex () == 0) {
    root->config_set (cfg_edit_grid, egc.to_string (db::DVector (-1.0, -1.0)));
  } else if (mp_ui->grid_cb->currentIndex () == 1) {
    root->config_set (cfg_edit_grid, egc.to_string (db::DVector ()));
  } else {
    try {
      db::DVector eg;
      egc.from_string_picky (tl::to_string (mp_ui->edit_grid_le->text ()), eg);
      lay::indicate_error (mp_ui->edit_grid_le, (tl::Exception *) 0);
      root->config_set (cfg_edit_grid, egc.to_string (eg));
    } catch (tl::Exception &ex) {
      lay::indicate_error (mp_ui->edit_grid_le, &ex);
    }
  }

  //  Edit & move angle

  ACConverter acc;
  root->config_set (cfg_edit_move_angle_mode, acc.to_string (lay::angle_constraint_type (mp_ui->move_angle_cb->currentIndex ())));
  root->config_set (cfg_edit_connect_angle_mode, acc.to_string (lay::angle_constraint_type (mp_ui->conn_angle_cb->currentIndex ())));

  root->config_set (cfg_edit_top_level_selection, tl::to_string (mp_ui->hier_sel_cbx->isChecked ()));
  int cpm = mp_ui->hier_copy_mode_cbx->currentIndex ();
  root->config_set (cfg_edit_hier_copy_mode, tl::to_string ((cpm < 0 || cpm > 1) ? -1 : cpm));
  root->config_set (cfg_edit_snap_to_objects, tl::to_string (mp_ui->snap_objects_cbx->isChecked ()));
  root->config_set (cfg_edit_snap_objects_to_grid, tl::to_string (mp_ui->snap_objects_to_grid_cbx->isChecked ()));

  configure_from_line_edit<unsigned int> (root, mp_ui->max_shapes_le, cfg_edit_max_shapes_of_instances);
  root->config_set (cfg_edit_show_shapes_of_instances, tl::to_string (mp_ui->show_shapes_cbx->isChecked ()));
}

void
EditorOptionsGeneric::grid_changed (int grid_mode)
{
  mp_ui->edit_grid_le->setEnabled (grid_mode == 2);
}

void
EditorOptionsGeneric::show_shapes_changed ()
{
  mp_ui->max_shapes_le->setEnabled (mp_ui->show_shapes_cbx->isChecked ());
}

void
EditorOptionsGeneric::setup (lay::Dispatcher *root)
{
  //  Edit grid

  EditGridConverter egc;
  db::DVector eg;
  root->config_get (cfg_edit_grid, eg, egc);

  if (eg == db::DVector ()) {
    mp_ui->grid_cb->setCurrentIndex (1);
  } else if (eg.x () < -0.5) {
    mp_ui->grid_cb->setCurrentIndex (0);
  } else {
    mp_ui->grid_cb->setCurrentIndex (2);
    mp_ui->edit_grid_le->setText (tl::to_qstring (egc.to_string (eg)));
  }
  grid_changed (mp_ui->grid_cb->currentIndex ());
  lay::indicate_error (mp_ui->edit_grid_le, (tl::Exception *) 0);

  //  edit & move angle

  ACConverter acc;
  lay::angle_constraint_type ac;

  ac = lay::AC_Any;
  root->config_get (cfg_edit_move_angle_mode, ac, acc);
  mp_ui->move_angle_cb->setCurrentIndex (int (ac));

  ac = lay::AC_Any;
  root->config_get (cfg_edit_connect_angle_mode, ac, acc);
  mp_ui->conn_angle_cb->setCurrentIndex (int (ac));

  bool top_level_sel = false;
  root->config_get (cfg_edit_top_level_selection, top_level_sel);
  mp_ui->hier_sel_cbx->setChecked (top_level_sel);

  int cpm = -1;
  root->config_get (cfg_edit_hier_copy_mode, cpm);
  mp_ui->hier_copy_mode_cbx->setCurrentIndex ((cpm < 0 || cpm > 1) ? 2 : cpm);

  bool snap_to_objects = false;
  root->config_get (cfg_edit_snap_to_objects, snap_to_objects);
  mp_ui->snap_objects_cbx->setChecked (snap_to_objects);

  bool snap_objects_to_grid = false;
  root->config_get (cfg_edit_snap_objects_to_grid, snap_objects_to_grid);
  mp_ui->snap_objects_to_grid_cbx->setChecked (snap_objects_to_grid);

  unsigned int max_shapes = 1000;
  root->config_get (cfg_edit_max_shapes_of_instances, max_shapes);
  mp_ui->max_shapes_le->setText (tl::to_qstring (tl::to_string (max_shapes)));
  lay::indicate_error (mp_ui->max_shapes_le, (tl::Exception *) 0);

  bool show_shapes = true;
  root->config_get (cfg_edit_show_shapes_of_instances, show_shapes);
  mp_ui->show_shapes_cbx->setChecked (show_shapes);
}

// ------------------------------------------------------------------
//  EditorOptionsText implementation

EditorOptionsText::EditorOptionsText (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : lay::EditorOptionsPage (view, dispatcher)
{
  mp_ui = new Ui::EditorOptionsText ();
  mp_ui->setupUi (this);

  connect (mp_ui->text_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->halign_cbx, SIGNAL (activated (int)), this, SLOT (edited ()));
  connect (mp_ui->valign_cbx, SIGNAL (activated (int)), this, SLOT (edited ()));
  connect (mp_ui->size_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
}

EditorOptionsText::~EditorOptionsText ()
{
  delete mp_ui;
  mp_ui = 0;
}

std::string 
EditorOptionsText::title () const
{
  return tl::to_string (QObject::tr ("Text"));
}

void  
EditorOptionsText::apply (lay::Dispatcher *root)
{
  //  Text string
  root->config_set (cfg_edit_text_string, tl::unescape_string (tl::to_string (mp_ui->text_le->text ())));

  //  HAlign
  HAlignConverter hac;
  root->config_set (cfg_edit_text_halign, hac.to_string (db::HAlign (mp_ui->halign_cbx->currentIndex () - 1)));

  //  VAlign
  VAlignConverter vac;
  root->config_set (cfg_edit_text_valign, vac.to_string (db::VAlign (mp_ui->valign_cbx->currentIndex () - 1)));

  //  Text size
  if (mp_ui->size_le->text ().isEmpty ()) {
    root->config_set (cfg_edit_text_size, 0.0);
  } else {
    double sz = 0.0;
    tl::from_string_ext (tl::to_string (mp_ui->size_le->text ()), sz);
    root->config_set (cfg_edit_text_size, sz);
  }
}

void  
EditorOptionsText::setup (lay::Dispatcher *root)
{
  //  Text string
  std::string s;
  root->config_get (cfg_edit_text_string, s);
  mp_ui->text_le->setText (tl::to_qstring (tl::escape_string (s)));

  //  HAlign
  db::HAlign ha = db::HAlignLeft;
  root->config_get (cfg_edit_text_halign, ha, HAlignConverter ());
  mp_ui->halign_cbx->setCurrentIndex (int (ha) + 1);

  //  VAlign
  db::VAlign va = db::VAlignBottom;
  root->config_get (cfg_edit_text_valign, va, VAlignConverter ());
  mp_ui->valign_cbx->setCurrentIndex (int (va) + 1);

  double sz = 0.0;
  root->config_get (cfg_edit_text_size, sz);
  if (sz > 0.0) {
    mp_ui->size_le->setText (tl::to_qstring (tl::to_string (sz)));
  } else {
    mp_ui->size_le->setText (QString ());
  }
}

// ------------------------------------------------------------------
//  EditorOptionsPath implementation

EditorOptionsPath::EditorOptionsPath (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : lay::EditorOptionsPage (view, dispatcher)
{
  mp_ui = new Ui::EditorOptionsPath ();
  mp_ui->setupUi (this);

  connect (mp_ui->type_cb, SIGNAL (currentIndexChanged (int)), this, SLOT (type_changed (int)));

  connect (mp_ui->width_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->type_cb, SIGNAL (activated (int)), this, SLOT (edited ()));
  connect (mp_ui->start_ext_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->end_ext_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
}

EditorOptionsPath::~EditorOptionsPath ()
{
  delete mp_ui;
  mp_ui = 0;
}

std::string 
EditorOptionsPath::title () const
{
  return tl::to_string (QObject::tr ("Path"));
}

void
EditorOptionsPath::type_changed (int type)
{
  mp_ui->start_ext_le->setEnabled (type == 2);
  mp_ui->end_ext_le->setEnabled (type == 2);
}

void  
EditorOptionsPath::apply (lay::Dispatcher *root)
{
  //  width

  configure_from_line_edit<double> (root, mp_ui->width_le, cfg_edit_path_width);

  //  path type and extensions 

  if (mp_ui->type_cb->currentIndex () == 0) {

    root->config_set (cfg_edit_path_ext_type, "flush");

  } else if (mp_ui->type_cb->currentIndex () == 1) {

    root->config_set (cfg_edit_path_ext_type, "square");

  } else if (mp_ui->type_cb->currentIndex () == 2) {

    root->config_set (cfg_edit_path_ext_type, "variable");

    configure_from_line_edit<double> (root, mp_ui->start_ext_le, cfg_edit_path_ext_var_begin);
    configure_from_line_edit<double> (root, mp_ui->end_ext_le, cfg_edit_path_ext_var_end);

  } else if (mp_ui->type_cb->currentIndex () == 3) {

    root->config_set (cfg_edit_path_ext_type, "round");

  }
}

void  
EditorOptionsPath::setup (lay::Dispatcher *root)
{
  //  width

  double w = 0.0;
  root->config_get (cfg_edit_path_width, w);
  mp_ui->width_le->setText (tl::to_qstring (tl::to_string (w)));
  lay::indicate_error (mp_ui->width_le, (tl::Exception *) 0);

  //  path type and extensions 

  std::string type;
  root->config_get (cfg_edit_path_ext_type, type);
  if (type == "square") {
    mp_ui->type_cb->setCurrentIndex (1);
  } else if (type == "variable") {
    mp_ui->type_cb->setCurrentIndex (2);
  } else if (type == "round") {
    mp_ui->type_cb->setCurrentIndex (3);
  } else {
    mp_ui->type_cb->setCurrentIndex (0);
  }
  type_changed (mp_ui->type_cb->currentIndex ());

  double bgnext = 0.0, endext = 0.0;
  root->config_get (cfg_edit_path_ext_var_begin, bgnext);
  root->config_get (cfg_edit_path_ext_var_end, endext);
  mp_ui->start_ext_le->setText (tl::to_qstring (tl::to_string (bgnext)));
  lay::indicate_error (mp_ui->start_ext_le, (tl::Exception *) 0);
  mp_ui->end_ext_le->setText (tl::to_qstring (tl::to_string (endext)));
  lay::indicate_error (mp_ui->end_ext_le, (tl::Exception *) 0);
}

// ------------------------------------------------------------------
//  EditorOptionsInst implementation

EditorOptionsInst::EditorOptionsInst (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : lay::EditorOptionsPage (view, dispatcher)
{
  mp_ui = new Ui::EditorOptionsInst ();
  mp_ui->setupUi (this);

  connect (mp_ui->array_grp, SIGNAL (clicked ()), this, SLOT (array_changed ()));
  connect (mp_ui->browse_pb, SIGNAL (clicked ()), this, SLOT (browse_cell ()));
  connect (mp_ui->lib_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (library_changed ()));
  connect (mp_ui->cell_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->angle_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->scale_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->rows_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->row_x_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->row_y_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->columns_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->column_x_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->column_y_le, SIGNAL (editingFinished ()), this, SLOT (edited ()));
  connect (mp_ui->mirror_cbx, SIGNAL (clicked ()), this, SLOT (edited ()));
  connect (mp_ui->array_grp, SIGNAL (clicked ()), this, SLOT (edited ()));
  connect (mp_ui->place_origin_cb, SIGNAL (clicked ()), this, SLOT (edited ()));

  m_cv_index = -1;
}

EditorOptionsInst::~EditorOptionsInst ()
{
  delete mp_ui;
  mp_ui = 0;
}

std::string 
EditorOptionsInst::title () const
{
  return tl::to_string (QObject::tr ("Instance"));
}

void
EditorOptionsInst::library_changed ()
{
  update_cell_edits ();
  edited ();
}

//  Maximum number of cells for which to offer a cell name completer
const static size_t max_cells = 10000;

void
EditorOptionsInst::update_cell_edits ()
{
  if (mp_ui->cell_le->completer ()) {
    mp_ui->cell_le->completer ()->deleteLater ();
  }

  db::Layout *layout = 0;

  //  find the layout the cell has to be looked up: that is either the layout of the current instance or
  //  the library selected
  if (mp_ui->lib_cbx->current_library ()) {
    layout = &mp_ui->lib_cbx->current_library ()->layout ();
  } else if (view ()->cellview (m_cv_index).is_valid ()) {
    layout = &view ()->cellview (m_cv_index)->layout ();
  }

  if (! layout) {
    return;
  }

  QStringList cellnames;
  if (layout->cells () < max_cells) {
    for (db::Layout::iterator c = layout->begin (); c != layout->end (); ++c) {
      cellnames.push_back (tl::to_qstring (layout->cell_name (c->cell_index ())));
    }
    for (db::Layout::pcell_iterator pc = layout->begin_pcells (); pc != layout->end_pcells () && size_t (cellnames.size ()) < max_cells; ++pc) {
      cellnames.push_back (tl::to_qstring (pc->first));
    }
  }

  if (size_t (cellnames.size ()) < max_cells) {
    QCompleter *completer = new QCompleter (cellnames, this);
    completer->setCaseSensitivity (Qt::CaseSensitive);
    mp_ui->cell_le->setCompleter (completer);
  } else {
    mp_ui->cell_le->setCompleter (0);
  }

  std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (tl::to_string (mp_ui->cell_le->text ()).c_str ());
  std::pair<bool, db::cell_index_type> cc = layout->cell_by_name (tl::to_string (mp_ui->cell_le->text ()).c_str ());

  //  by the way, update the foreground color of the cell edit box as well (red, if not valid)
  tl::Exception ex ("No cell or PCell with this name");
  lay::indicate_error (mp_ui->cell_le, (! pc.first && ! cc.first) ? &ex : 0);
}

void
EditorOptionsInst::browse_cell ()
{
BEGIN_PROTECTED

  if (m_cv_index >= 0 && view ()->cellview (m_cv_index).is_valid ()) {

    //  find the layout the cell has to be looked up: that is either the layout of the current instance or 
    //  the library selected
    db::Layout *layout = 0;
    db::Library *lib = 0;
    if (mp_ui->lib_cbx->current_library ()) {
      lib = mp_ui->lib_cbx->current_library ();
      layout = &lib->layout ();
    } else {
      layout = &view ()->cellview (m_cv_index)->layout ();
    }

    bool all_cells = (mp_ui->lib_cbx->current_library () != 0 ? false : true);
    lay::LibraryCellSelectionForm form (this, layout, "browse_lib_cell", all_cells);

    if (lib) {
      form.setWindowTitle (tl::to_qstring (tl::to_string (QObject::tr ("Select Cell - Library: ")) + lib->get_description ()));
    }

    std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (tl::to_string (mp_ui->cell_le->text ()).c_str ());
    if (pc.first) {
      form.set_selected_pcell_id (pc.second);
    } else {
      std::pair<bool, db::cell_index_type> c = layout->cell_by_name (tl::to_string (mp_ui->cell_le->text ()).c_str ());
      if (c.first) {
        form.set_selected_cell_index (c.second);
      }
    }

    if (form.exec ()) {
      if (form.selected_cell_is_pcell ()) {
        mp_ui->cell_le->setText (tl::to_qstring (layout->pcell_header (form.selected_pcell_id ())->get_name ()));
      } else if (layout->is_valid_cell_index (form.selected_cell_index ())) {
        mp_ui->cell_le->setText (tl::to_qstring (layout->cell_name (form.selected_cell_index ())));
      }
      edited ();
    }

  }

END_PROTECTED
}

void
EditorOptionsInst::array_changed ()
{
  bool array = (mp_ui->array_grp->isChecked ());
  mp_ui->rows_le->setEnabled (array);
  mp_ui->row_x_le->setEnabled (array);
  mp_ui->row_y_le->setEnabled (array);
  mp_ui->columns_le->setEnabled (array);
  mp_ui->column_x_le->setEnabled (array);
  mp_ui->column_y_le->setEnabled (array);
  edited ();
}

void  
EditorOptionsInst::apply (lay::Dispatcher *root)
{
  //  cell name
  root->config_set (cfg_edit_inst_cell_name, tl::to_string (mp_ui->cell_le->text ()));

  //  lib name
  if (mp_ui->lib_cbx->current_library ()) {
    root->config_set (cfg_edit_inst_lib_name, mp_ui->lib_cbx->current_library ()->get_name ());
  } else {
    root->config_set (cfg_edit_inst_lib_name, std::string ());
  }

  //  rotation, scaling
  configure_from_line_edit<double> (root, mp_ui->angle_le, cfg_edit_inst_angle);

  bool mirror = mp_ui->mirror_cbx->isChecked ();
  root->config_set (cfg_edit_inst_mirror, tl::to_string (mirror));

  configure_from_line_edit<double> (root, mp_ui->scale_le, cfg_edit_inst_scale);

  //  array
  bool array = mp_ui->array_grp->isChecked ();
  root->config_set (cfg_edit_inst_array, tl::to_string (array));

  configure_from_line_edit<int> (root, mp_ui->rows_le, cfg_edit_inst_rows);
  configure_from_line_edit<double> (root, mp_ui->row_x_le, cfg_edit_inst_row_x);
  configure_from_line_edit<double> (root, mp_ui->row_y_le, cfg_edit_inst_row_y);
  configure_from_line_edit<int> (root, mp_ui->columns_le, cfg_edit_inst_columns);
  configure_from_line_edit<double> (root, mp_ui->column_x_le, cfg_edit_inst_column_x);
  configure_from_line_edit<double> (root, mp_ui->column_y_le, cfg_edit_inst_column_y);

  //  place origin of cell flag
  bool place_origin = mp_ui->place_origin_cb->isChecked ();
  root->config_set (cfg_edit_inst_place_origin, tl::to_string (place_origin));
}

void
EditorOptionsInst::technology_changed (const std::string &)
{
  //  The layout's technology has changed
  setup (dispatcher ());
}

void
EditorOptionsInst::active_cellview_changed ()
{
  //  The active cellview has changed
  setup (dispatcher ());
}

void
EditorOptionsInst::setup (lay::Dispatcher *root)
{
  m_cv_index = view ()->active_cellview_index ();

  try {

    mp_ui->lib_cbx->blockSignals (true);

    std::string techname;

    mp_ui->lib_cbx->update_list ();
    if (m_cv_index >= 0 && view ()->cellview (m_cv_index).is_valid ()) {
      techname = view ()->cellview (m_cv_index)->tech_name ();
    }
    mp_ui->lib_cbx->set_technology_filter (techname, true);

    //  cell name
    std::string s;
    root->config_get (cfg_edit_inst_cell_name, s);
    mp_ui->cell_le->setText (tl::to_qstring (s));

    //  library
    std::string l;
    root->config_get (cfg_edit_inst_lib_name, l);
    mp_ui->lib_cbx->set_current_library (db::LibraryManager::instance ().lib_ptr_by_name (l, techname));

    mp_ui->lib_cbx->blockSignals (false);
    update_cell_edits ();

  } catch (...) {
    mp_ui->lib_cbx->blockSignals (false);
    throw;
  }

  //  rotation, scaling
  double angle = 0.0;
  root->config_get (cfg_edit_inst_angle, angle);
  mp_ui->angle_le->setText (tl::to_qstring (tl::to_string (angle)));
  lay::indicate_error (mp_ui->angle_le, (tl::Exception *) 0);

  bool mirror = false;
  root->config_get (cfg_edit_inst_mirror, mirror);
  mp_ui->mirror_cbx->setChecked (mirror);

  double scale = 1.0;
  root->config_get (cfg_edit_inst_scale, scale);
  mp_ui->scale_le->setText (tl::to_qstring (tl::to_string (scale)));
  lay::indicate_error (mp_ui->scale_le, (tl::Exception *) 0);

  //  array
  bool array = false;
  root->config_get (cfg_edit_inst_array, array);
  mp_ui->array_grp->setChecked (array);

  int rows = 1, columns = 1;
  double row_x = 0.0, row_y = 0.0, column_x = 0.0, column_y = 0.0;
  root->config_get (cfg_edit_inst_rows, rows);
  root->config_get (cfg_edit_inst_row_x, row_x);
  root->config_get (cfg_edit_inst_row_y, row_y);
  root->config_get (cfg_edit_inst_columns, columns);
  root->config_get (cfg_edit_inst_column_x, column_x);
  root->config_get (cfg_edit_inst_column_y, column_y);

  mp_ui->rows_le->setText (tl::to_qstring (tl::to_string (rows)));
  lay::indicate_error (mp_ui->rows_le, (tl::Exception *) 0);
  mp_ui->row_x_le->setText (tl::to_qstring (tl::to_string (row_x)));
  lay::indicate_error (mp_ui->row_x_le, (tl::Exception *) 0);
  mp_ui->row_y_le->setText (tl::to_qstring (tl::to_string (row_y)));
  lay::indicate_error (mp_ui->row_y_le, (tl::Exception *) 0);
  mp_ui->columns_le->setText (tl::to_qstring (tl::to_string (columns)));
  lay::indicate_error (mp_ui->columns_le, (tl::Exception *) 0);
  mp_ui->column_x_le->setText (tl::to_qstring (tl::to_string (column_x)));
  lay::indicate_error (mp_ui->column_x_le, (tl::Exception *) 0);
  mp_ui->column_y_le->setText (tl::to_qstring (tl::to_string (column_y)));
  lay::indicate_error (mp_ui->column_y_le, (tl::Exception *) 0);

  //  place origin of cell flag
  bool place_origin = false;
  root->config_get (cfg_edit_inst_place_origin, place_origin);
  mp_ui->place_origin_cb->setChecked (place_origin);
}

// ------------------------------------------------------------------
//  EditorOptionsInstPCellParam implementation

EditorOptionsInstPCellParam::EditorOptionsInstPCellParam (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : lay::EditorOptionsPage (view, dispatcher), mp_pcell_parameters (0), mp_placeholder_label (0)
{
  mp_ui = new Ui::EditorOptionsInstPCellParam ();
  mp_ui->setupUi (this);
}

EditorOptionsInstPCellParam::~EditorOptionsInstPCellParam ()
{
  delete mp_ui;
  mp_ui = 0;
}

std::string
EditorOptionsInstPCellParam::title () const
{
  return tl::to_string (QObject::tr ("PCell"));
}

void
EditorOptionsInstPCellParam::apply (lay::Dispatcher *root)
{
  //  pcell parameters
  std::string param;
  db::Layout *layout = 0;

  db::Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (m_lib_name, view ()->active_cellview ().is_valid () ? view ()->active_cellview ()->tech_name () : std::string ());
  if (lib) {
    layout = &lib->layout ();
  } else if (m_cv_index >= 0 && view ()->cellview (m_cv_index).is_valid ()) {
    layout = &view ()->cellview (m_cv_index)->layout ();
  }

  bool ok = true;

  if (layout && mp_pcell_parameters) {
    std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (tl::to_string (m_cell_name).c_str ());
    if (pc.first) {
      const db::PCellDeclaration *pc_decl = layout->pcell_declaration (pc.second);
      if (pc_decl) {
        param = pcell_parameters_to_string (pc_decl->named_parameters (mp_pcell_parameters->get_parameters (&ok)));
      }
    }
  }

  if (ok) {
    root->config_set (cfg_edit_inst_pcell_parameters, param);
  }
}

void
EditorOptionsInstPCellParam::technology_changed (const std::string &)
{
  setup (dispatcher ());
}

void
EditorOptionsInstPCellParam::setup (lay::Dispatcher *root)
{
  m_cv_index = view ()->active_cellview_index ();

  bool needs_update = (mp_pcell_parameters == 0);

  //  cell name
  std::string cn;
  root->config_get (cfg_edit_inst_cell_name, cn);
  if (cn != m_cell_name) {
    m_cell_name = cn;
    needs_update = true;
  }

  //  library
  std::string ln;
  root->config_get (cfg_edit_inst_lib_name, ln);
  if (ln != m_lib_name) {
    m_lib_name = ln;
    needs_update = true;
  }

  db::Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (m_lib_name, view ()->active_cellview ().is_valid () ? view ()->active_cellview ()->tech_name () : std::string ());

  //  pcell parameters
  std::string param;
  root->config_get (cfg_edit_inst_pcell_parameters, param);

  db::Layout *layout = 0;
  if (lib) {
    layout = &lib->layout ();
  } else if (m_cv_index >= 0 && view ()->cellview (m_cv_index).is_valid ()) {
    layout = &view ()->cellview (m_cv_index)->layout ();
  }

  std::vector<tl::Variant> pv;

  if (layout) {

    std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (tl::to_string (m_cell_name).c_str ());

    if (pc.first) {

      const db::PCellDeclaration *pc_decl = layout->pcell_declaration (pc.second);
      if (pc_decl) {

        std::map<std::string, tl::Variant> parameters;
        try {
          tl::Extractor ex (param.c_str ());
          ex.test ("!");  //  used to flag PCells
          while (! ex.at_end ()) {
            std::string n;
            ex.read_word_or_quoted (n);
            ex.test (":");
            ex.read (parameters.insert (std::make_pair (n, tl::Variant ())).first->second);
            ex.test (";");
          }
        } catch (...) { }

        const std::vector<db::PCellParameterDeclaration> &pcp = pc_decl->parameter_declarations ();
        for (std::vector<db::PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end (); ++pd) {
          std::map<std::string, tl::Variant>::const_iterator p = parameters.find (pd->get_name ());
          if (p != parameters.end ()) {
            pv.push_back (p->second);
          } else {
            pv.push_back (pd->get_default ());
          }
        }

      }

    }

  }

  if (! needs_update) {
    bool ok = false;
    if (mp_pcell_parameters->get_parameters (&ok) != pv || ! ok) {
      needs_update = true;
    }
  }

  try {
    if (needs_update) {
      update_pcell_parameters (pv);
    }
  } catch (...) { }
}

void
EditorOptionsInstPCellParam::update_pcell_parameters ()
{
  update_pcell_parameters (std::vector <tl::Variant> ());
}

void
EditorOptionsInstPCellParam::update_pcell_parameters (const std::vector <tl::Variant> &parameters)
{
  db::Layout *layout = 0;

  //  find the layout the cell has to be looked up: that is either the layout of the current instance or
  //  the library selected
  db::Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (m_lib_name, view ()->active_cellview ().is_valid () ? view ()->active_cellview ()->tech_name () : std::string ());
  if (lib) {
    layout = &lib->layout ();
  } else {
    const lay::CellView &cv = view ()->cellview (m_cv_index);
    if (cv.is_valid ()) {
      layout = &cv->layout ();
    }
  }

  std::pair<bool, db::pcell_id_type> pc (false, 0);
  if (layout) {
    pc = layout->pcell_by_name (tl::to_string (m_cell_name).c_str ());
  }

  //  TODO: don't re-generate the PCellParametersPage unless the PCell has changed

  PCellParametersPage::State pcp_state;

  //  Hint: we shall not delete the page immediately. This gives a segmentation fault in some cases.
  if (mp_pcell_parameters) {
    pcp_state = mp_pcell_parameters->get_state ();
    mp_pcell_parameters->hide ();
    mp_pcell_parameters->deleteLater ();
  }

  if (mp_placeholder_label) {
    mp_placeholder_label->hide ();
    mp_placeholder_label->deleteLater ();
  }

  mp_pcell_parameters = 0;
  mp_placeholder_label = 0;

  if (pc.first && layout->pcell_declaration (pc.second) && view ()->cellview (m_cv_index).is_valid ()) {

    mp_pcell_parameters = new PCellParametersPage (this, dispatcher (), true /*dense*/);
    mp_pcell_parameters->setup (view (), m_cv_index, layout->pcell_declaration (pc.second), parameters);
    this->layout ()->addWidget (mp_pcell_parameters);

    mp_pcell_parameters->set_state (pcp_state);
    connect (mp_pcell_parameters, SIGNAL (edited ()), this, SLOT (edited ()));

  } else {

    mp_placeholder_label = new QLabel (this);
    mp_placeholder_label->setText (tr ("Not a PCell"));
    mp_placeholder_label->setAlignment (Qt::AlignHCenter | Qt::AlignVCenter);
    this->layout ()->addWidget (mp_placeholder_label);

  }
}

}

#endif

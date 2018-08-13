
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#include "layDialogs.h"

#include "dbLayout.h"

#include "tlString.h"
#include "tlException.h"
#include "tlExceptions.h"
#include "tlXMLParser.h"

#include "layLayerProperties.h"
#include "layFileDialog.h"
#include "layLayoutView.h"
#include "layLayoutView.h"
#include "layCellTreeModel.h"
#include "layQtTools.h"

namespace lay
{

// --------------------------------------------------------------------------------
//  LayerSourceDialog implementation

LayerSourceDialog::LayerSourceDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("layer_source_dialog"));
  Ui::LayerSourceDialog::setupUi (this);
  
  activate_help_links (helpLabel);
}

bool 
LayerSourceDialog::exec_dialog (std::string &s)
{
  sourceString->setText (tl::to_qstring (s));
  if (QDialog::exec ()) {
    s = tl::to_string (sourceString->text ());
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  NewLayoutPropertiesDialog implementation

NewLayoutPropertiesDialog::NewLayoutPropertiesDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("new_layout_properties_dialog"));
  Ui::NewLayoutPropertiesDialog::setupUi (this);
  connect (tech_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (tech_changed ()));
}

NewLayoutPropertiesDialog::~NewLayoutPropertiesDialog ()
{
  //  .. nothing yet ..
}

void
NewLayoutPropertiesDialog::tech_changed ()
{
  double dbu = 0.001;
  int technology_index = tech_cbx->currentIndex ();
  if (technology_index >= 0 && technology_index < (int) lay::Technologies::instance ()->technologies ()) {
    dbu = lay::Technologies::instance ()->begin () [technology_index].dbu ();
  }

#if QT_VERSION >= 0x40700
  dbu_le->setPlaceholderText (tl::to_qstring (tl::to_string (dbu)));
#endif
}

bool 
NewLayoutPropertiesDialog::exec_dialog (std::string &technology, std::string &cell_name, double &dbu, double &size, bool &current_panel)
{
  tech_cbx->clear ();
  unsigned int technology_index = 0;
  for (lay::Technologies::const_iterator t = lay::Technologies::instance ()->begin (); t != lay::Technologies::instance ()->end (); ++t, ++technology_index) {

    std::string d = t->name ();
    if (! d.empty () && ! t->description ().empty ()) {
      d += " - ";
    }
    d += t->description ();

    tech_cbx->addItem (tl::to_qstring (d));
    if (t->name () == technology) {
      tech_cbx->setCurrentIndex (technology_index);
    }

  }

  window_le->setText (tl::to_qstring (tl::to_string (size)));
  if (dbu > 1e-10) {
    dbu_le->setText (tl::to_qstring (tl::to_string (dbu)));
  } else {
    dbu_le->setText (QString ());
  }
  topcell_le->setText (tl::to_qstring (cell_name));
  current_panel_cb->setChecked (current_panel);

  if (QDialog::exec ()) {

    //  get the selected technology name
    int technology_index = tech_cbx->currentIndex ();
    if (technology_index >= 0 && technology_index < (int) lay::Technologies::instance ()->technologies ()) {
      technology = lay::Technologies::instance ()->begin () [technology_index].name ();
    } else {
      technology = std::string ();
    }

    tl::from_string (tl::to_string (window_le->text ()), size);
    if (! dbu_le->text ().isEmpty ()) {
      tl::from_string (tl::to_string (dbu_le->text ()), dbu);
    } else {
      dbu = 0.0;
    }
    cell_name = tl::to_string (topcell_le->text ());
    current_panel = current_panel_cb->isChecked ();
    return true;

  } else {
    return false;
  }
}

void 
NewLayoutPropertiesDialog::accept ()
{
BEGIN_PROTECTED;

  double x = 0.0;
  tl::from_string (tl::to_string (window_le->text ()), x);
  if (!dbu_le->text ().isEmpty ()) {
    tl::from_string (tl::to_string (dbu_le->text ()), x);
  }

  if (topcell_le->text ().isEmpty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("The topcell must be specified")));
  }
  
  QDialog::accept ();

END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  NewCellPropertiesDialog implementation

NewCellPropertiesDialog::NewCellPropertiesDialog (QWidget *parent)
  : QDialog (parent),
    mp_layout (0)
{
  setObjectName (QString::fromUtf8 ("new_cell_properties_dialog"));
  Ui::NewCellPropertiesDialog::setupUi (this);
}

NewCellPropertiesDialog::~NewCellPropertiesDialog ()
{
  //  .. nothing yet ..
}

bool 
NewCellPropertiesDialog::exec_dialog (const db::Layout *layout, std::string &cell_name, double &size)
{
  mp_layout = layout;

  name_le->setText (tl::to_qstring (cell_name));
  window_le->setText (tl::to_qstring (tl::to_string (size)));

  if (QDialog::exec ()) {

    tl::from_string (tl::to_string (window_le->text ()), size);
    cell_name = tl::to_string (name_le->text ());
    return true;

  } else {
    return false;
  }
}

void 
NewCellPropertiesDialog::accept ()
{
BEGIN_PROTECTED;

  double x = 0.0;
  tl::from_string (tl::to_string (window_le->text ()), x);

  if (mp_layout->cell_by_name (tl::to_string (name_le->text ()).c_str ()).first) {
    throw tl::Exception (tl::to_string (QObject::tr ("A cell with that name already exists: %s")), tl::to_string (name_le->text ()));
  }
  
  QDialog::accept ();

END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  NewLayerPropertiesDialog implementation

NewLayerPropertiesDialog::NewLayerPropertiesDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("new_layer_properties_dialog"));
  Ui::NewLayerPropertiesDialog::setupUi (this);
}

NewLayerPropertiesDialog::~NewLayerPropertiesDialog ()
{
  //  .. nothing yet ..
}

bool 
NewLayerPropertiesDialog::exec_dialog (db::LayerProperties &src)
{
  return exec_dialog (lay::CellView (), src);
}

bool 
NewLayerPropertiesDialog::exec_dialog (const lay::CellView &cv, db::LayerProperties &src)
{
  if (cv.is_valid ()) {
    layout_lbl->setText (tl::to_qstring ((tl::to_string (QObject::tr ("Layer for layout: ")) + cv->name ())));
    layout_lbl->show ();
  } else {
    layout_lbl->hide ();
  }

  if (src.layer >= 0) {
    layer_le->setText (tl::to_qstring (tl::to_string (src.layer)));
  } else {
    layer_le->setText (QString ());
  }
  if (src.datatype >= 0) {
    datatype_le->setText (tl::to_qstring (tl::to_string (src.datatype)));
  } else {
    datatype_le->setText (QString ());
  }
  name_le->setText (tl::to_qstring (src.name));

  if (QDialog::exec ()) {
    get (src);
    return true;
  } else {
    return false;
  }
}

void
NewLayerPropertiesDialog::get (db::LayerProperties &src)
{
  if (! layer_le->text ().isEmpty ()) {
    int l = -1;
    tl::from_string (tl::to_string (layer_le->text ()), l);
    src.layer = l;
  } else {
    src.layer = -1;
  }
  if (! datatype_le->text ().isEmpty ()) {
    int d = -1;
    tl::from_string (tl::to_string (datatype_le->text ()), d);
    src.datatype = d;
  } else {
    src.datatype = -1;
  }
  src.name = tl::to_string (name_le->text ());
}

void 
NewLayerPropertiesDialog::accept ()
{
BEGIN_PROTECTED;
  db::LayerProperties lp;
  get (lp);

  if (lp.layer < 0 && lp.datatype < 0) {
    if (lp.name.empty ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Either a layer/datatype combination or a name must be specified for a layer")));
    } 
  } else if (lp.layer < 0 || lp.datatype < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("Both layer and datatype must be specified for a layer")));
  }
  QDialog::accept ();
END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  MoveOptionsDialog implementation

MoveOptionsDialog::MoveOptionsDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("move_options_dialog"));
  Ui::MoveOptionsDialog::setupUi (this);
}

MoveOptionsDialog::~MoveOptionsDialog ()
{
  //  .. nothing yet ..
}

bool 
MoveOptionsDialog::exec_dialog (db::DVector &disp)
{
  disp_x_le->setText (tl::to_qstring (tl::to_string (disp.x ())));
  disp_y_le->setText (tl::to_qstring (tl::to_string (disp.y ())));

  if (QDialog::exec ()) {

    double x = 0.0, y = 0.0;
    tl::from_string (tl::to_string (disp_x_le->text ()), x);
    tl::from_string (tl::to_string (disp_y_le->text ()), y);

    disp = db::DVector (x, y);

    return true;

  } else {
    return false;
  }
}

void 
MoveOptionsDialog::accept ()
{
BEGIN_PROTECTED;
  double x = 0.0;
  tl::from_string (tl::to_string (disp_x_le->text ()), x);
  tl::from_string (tl::to_string (disp_y_le->text ()), x);
  QDialog::accept ();
END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  MoveToOptionsDialog implementation

MoveToOptionsDialog::MoveToOptionsDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("move_to_options_dialog"));
  Ui::MoveToOptionsDialog::setupUi (this);

  QToolButton *buttons[3][3] = { { lb, cb, rb }, { lc, cc, rc }, { lt, ct, rt } };

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      connect (buttons[i][j], SIGNAL (clicked ()), this, SLOT (button_clicked ()));
    }
  }
}

MoveToOptionsDialog::~MoveToOptionsDialog ()
{
  //  .. nothing yet ..
}

bool 
MoveToOptionsDialog::exec_dialog (int &mode_x, int &mode_y, db::DPoint &target)
{
  x_le->setText (tl::to_qstring (tl::to_string (target.x ())));
  y_le->setText (tl::to_qstring (tl::to_string (target.y ())));

  QToolButton *buttons[3][3] = { { lb, cb, rb }, { lc, cc, rc }, { lt, ct, rt } };

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      buttons[i][j]->setChecked (j - 1 == mode_x && i - 1 == mode_y);
    }
  }

  if (QDialog::exec ()) {

    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        if (buttons[i][j]->isChecked ()) {
          mode_x = j - 1;
          mode_y = i - 1;
        }
      }
    }

    double x = 0.0, y = 0.0;
    tl::from_string (tl::to_string (x_le->text ()), x);
    tl::from_string (tl::to_string (y_le->text ()), y);

    target = db::DPoint (x, y);

    return true;

  } else {
    return false;
  }
}

void 
MoveToOptionsDialog::accept ()
{
BEGIN_PROTECTED;
  double x = 0.0;
  tl::from_string (tl::to_string (x_le->text ()), x);
  tl::from_string (tl::to_string (y_le->text ()), x);
  QDialog::accept ();
END_PROTECTED;
}

void 
MoveToOptionsDialog::button_clicked ()
{
  QToolButton *buttons[3][3] = { { lb, cb, rb }, { lc, cc, rc }, { lt, ct, rt } };

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      if (buttons [i][j] != sender ()) {
        buttons [i][j]->setChecked (false);
      }
    }
  }
}

// --------------------------------------------------------------------------------
//  RenameCellDialog implementation

RenameCellDialog::RenameCellDialog (QWidget *parent)
  : QDialog (parent), mp_layout (0)
{
  setObjectName (QString::fromUtf8 ("rename_cell_dialog"));
  Ui::RenameCellDialog::setupUi (this);
}

RenameCellDialog::~RenameCellDialog ()
{
  //  .. nothing yet ..
}

void 
RenameCellDialog::accept ()
{
BEGIN_PROTECTED;
  if (name_le->text ().isEmpty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("A name must be given")));
  }
  if (mp_layout->cell_by_name (tl::to_string (name_le->text ()).c_str ()).first) {
    throw tl::Exception (tl::to_string (QObject::tr ("A cell with that name already exists")));
  }
  QDialog::accept ();
END_PROTECTED;
}

bool 
RenameCellDialog::exec_dialog (const db::Layout &layout, std::string &name)
{
  mp_layout = &layout;
  name_le->setText (tl::to_qstring (name));
  if (QDialog::exec ()) {
    name = tl::to_string (name_le->text ());
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  CopyCellModeDialog implementation

CopyCellModeDialog::CopyCellModeDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("copy_cell_mode_dialog"));
  Ui::CopyCellModeDialog::setupUi (this);
}

CopyCellModeDialog::~CopyCellModeDialog ()
{
  //  .. nothing yet ..
}

bool 
CopyCellModeDialog::exec_dialog (int &copy_mode)
{
  QRadioButton *buttons [] = { shallow_rb, deep_rb };

  for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
    buttons [i]->setChecked (copy_mode == i);
  }

  if (QDialog::exec ()) {
    for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
      if (buttons [i]->isChecked ()) {
        copy_mode = i;
      }
    }
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  DeleteCellModeDialog implementation

DeleteCellModeDialog::DeleteCellModeDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("delete_cell_mode_dialog"));
  Ui::DeleteCellModeDialog::setupUi (this);
}

DeleteCellModeDialog::~DeleteCellModeDialog ()
{
  //  .. nothing yet ..
}

bool 
DeleteCellModeDialog::exec_dialog (int &delete_mode)
{
  QRadioButton *buttons [] = { shallow_rb, deep_rb, full_rb };

  for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
    buttons [i]->setChecked (delete_mode == i);
  }

  if (QDialog::exec ()) {
    for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
      if (buttons [i]->isChecked ()) {
        delete_mode = i;
      }
    }
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  ReplaceCellOptionsDialog implementation

ReplaceCellOptionsDialog::ReplaceCellOptionsDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("replace_cell_options_dialog"));
  Ui::ReplaceCellOptionsDialog::setupUi (this);
}

ReplaceCellOptionsDialog::~ReplaceCellOptionsDialog ()
{
  //  .. nothing yet ..
}

static std::pair<bool, db::cell_index_type>
find_cell_by_display_name (const db::Layout &layout, const std::string &cn)
{
  for (db::Layout::const_iterator c = layout.begin (); c != layout.end (); ++c) {
    if (layout.display_name (c->cell_index ()) == cn) {
      return std::make_pair (true, c->cell_index ());
    }
  }

  return std::make_pair (false, 0);
}

bool 
ReplaceCellOptionsDialog::exec_dialog (const lay::CellView &cv, int &replace_mode, db::cell_index_type &cell_index)
{
  QRadioButton *buttons [] = { shallow_rb, deep_rb, full_rb };

  for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
    buttons [i]->setChecked (replace_mode == i);
  }

  lay::CellTreeModel *model = new lay::CellTreeModel (cell_selection_cbx, &cv->layout (), lay::CellTreeModel::Flat | lay::CellTreeModel::NoPadding);
  cell_selection_cbx->setModel (model);
  cell_selection_cbx->setEditText (tl::to_qstring (cv->layout ().cell_name (cell_index)));

  if (QDialog::exec ()) {

    for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
      if (buttons [i]->isChecked ()) {
        replace_mode = i;
      }
    }

    std::string cn = tl::to_string (cell_selection_cbx->lineEdit ()->text ());
    std::pair<bool, db::cell_index_type> cc = find_cell_by_display_name (cv->layout (), cn.c_str ());
    cell_index = cc.second;

    return cc.first;

  } else {
    return false;
  }
}

void 
ReplaceCellOptionsDialog::accept ()
{
BEGIN_PROTECTED;

  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (cell_selection_cbx->model ());
  if (model) {
    std::string cn = tl::to_string (cell_selection_cbx->lineEdit ()->text ());
    std::pair<bool, db::cell_index_type> cc = find_cell_by_display_name (*model->layout (), cn.c_str ());
    if (! cc.first) {
      throw tl::Exception (tl::to_string (QObject::tr ("Not a valid cell name: ")) + cn);
    }
  }

  QDialog::accept ();

END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  ClearLayerModeDialog implementation

ClearLayerModeDialog::ClearLayerModeDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("clear_layer_mode_dialog"));
  Ui::ClearLayerModeDialog::setupUi (this);
}

ClearLayerModeDialog::~ClearLayerModeDialog ()
{
  //  .. nothing yet ..
}

bool 
ClearLayerModeDialog::exec_dialog (int &clear_mode)
{
  QRadioButton *buttons [3] = { local_rb, hierarchically_rb, layout_rb };

  for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
    buttons [i]->setChecked (clear_mode == i);
  }

  if (QDialog::exec ()) {
    for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
      if (buttons [i]->isChecked ()) {
        clear_mode = i;
      }
    }
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  OpenLayoutModeDialog implementation

OpenLayoutModeDialog::OpenLayoutModeDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("open_layout_mode_dialog"));
  Ui::OpenLayoutModeDialog::setupUi (this);
}

OpenLayoutModeDialog::~OpenLayoutModeDialog ()
{
  //  .. nothing yet ..
}

bool 
OpenLayoutModeDialog::exec_dialog (int &open_mode)
{
  QRadioButton *buttons [3] = { replace_rb, new_rb, add_rb };

  for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
    buttons [i]->setChecked (open_mode == i);
  }

  if (QDialog::exec ()) {
    for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
      if (buttons [i]->isChecked ()) {
        open_mode = i;
      }
    }
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  DuplicateLayerDialog implementation

DuplicateLayerDialog::DuplicateLayerDialog (QWidget *parent)
  : QDialog (parent),
    mp_view (0)
{
  setObjectName (QString::fromUtf8 ("merge_options_dialog"));
  Ui::DuplicateLayerDialog::setupUi (this);

  connect (cv_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
  connect (cvr_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
}

DuplicateLayerDialog::~DuplicateLayerDialog ()
{
  //  .. nothing yet ..
}

void
DuplicateLayerDialog::cv_changed (int)
{
  if (! mp_view) {
    return;
  }

  layer_cbx->set_view (mp_view, cv_cbx->currentIndex ());
  layerr_cbx->set_view (mp_view, cvr_cbx->currentIndex ());
}

bool 
DuplicateLayerDialog::exec_dialog (lay::LayoutView *view, int &cv, int &layer, int &cv_r, int &layer_r, int &hier_mode, bool &clear_before)
{
  mp_view = view;

  bool res = false;
  cv_cbx->set_layout_view (view);
  cv_cbx->set_current_cv_index (cv);
  cvr_cbx->set_layout_view (view);
  cvr_cbx->set_current_cv_index (cv_r);

  cv_changed (0 /*dummy*/);

  layer_cbx->set_current_layer (layer);
  layerr_cbx->set_current_layer (layer_r);

  hier_mode_cbx->setCurrentIndex (hier_mode);
  clear_cb->setChecked (clear_before);

  if (QDialog::exec ()) {

    cv = cv_cbx->current_cv_index ();
    cv_r = cvr_cbx->current_cv_index ();
    layer = layer_cbx->current_layer ();
    layer_r = layerr_cbx->current_layer ();

    hier_mode = hier_mode_cbx->currentIndex ();
    clear_before = clear_cb->isChecked ();

    res = true;

  }

  mp_view = 0;
  return res;
}

void 
DuplicateLayerDialog::accept ()
{
BEGIN_PROTECTED;

  int cv = cv_cbx->current_cv_index ();
  if (cv < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout specified for source")));
  }

  int cv_r = cvr_cbx->current_cv_index ();
  if (cv_r < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout specified for result")));
  }

  if (fabs (mp_view->cellview (cv)->layout ().dbu () - mp_view->cellview (cv_r)->layout ().dbu ()) > db::epsilon) {
    throw tl::Exception (tl::to_string (QObject::tr ("Source and result layouts must have the same database unit")));
  }

  if (layer_cbx->current_layer () < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer specified for source")));
  }
  if (layerr_cbx->current_layer () < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer specified for result")));
  }

  if (hier_mode_cbx->currentIndex () == 2 && 
      cv_cbx->current_cv_index () != cvr_cbx->current_cv_index ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Source layout and result layout must be same in 'cell by cell' mode")));
  }

  if (cv_cbx->current_cv_index () == cvr_cbx->current_cv_index () && layer_cbx->current_layer () == layerr_cbx->current_layer ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Source and target layer must not be identical")));
  }

  QDialog::accept ();
END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  AlignCellOptionsDialog implementation

AlignCellOptionsDialog::AlignCellOptionsDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("align_cell_options_dialog"));
  Ui::AlignCellOptionsDialog::setupUi (this);

  QToolButton *buttons[3][3] = { { lb, cb, rb }, { lc, cc, rc }, { lt, ct, rt } };

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      connect (buttons[i][j], SIGNAL (clicked ()), this, SLOT (button_clicked ()));
    }
  }
}

AlignCellOptionsDialog::~AlignCellOptionsDialog ()
{
  //  .. nothing yet ..
}

bool 
AlignCellOptionsDialog::exec_dialog (int &mode_x, int &mode_y, bool &visible_only, bool &adjust_calls)
{
  vis_only_cbx->setChecked (visible_only);
  adjust_calls_cbx->setChecked (adjust_calls);

  QToolButton *buttons[3][3] = { { lb, cb, rb }, { lc, cc, rc }, { lt, ct, rt } };

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      buttons[i][j]->setChecked (j - 1 == mode_x && i - 1 == mode_y);
    }
  }

  if (QDialog::exec ()) {

    visible_only = vis_only_cbx->isChecked ();
    adjust_calls = adjust_calls_cbx->isChecked ();

    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        if (buttons[i][j]->isChecked ()) {
          mode_x = j - 1;
          mode_y = i - 1;
        }
      }
    }

    return true;

  } else {
    return false;
  }
}

void 
AlignCellOptionsDialog::button_clicked ()
{
  QToolButton *buttons[3][3] = { { lb, cb, rb }, { lc, cc, rc }, { lt, ct, rt } };

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      if (buttons [i][j] != sender ()) {
        buttons [i][j]->setChecked (false);
      }
    }
  }
}

// --------------------------------------------------------------------------------
//  FlattenInstOptionsDialog implementation

FlattenInstOptionsDialog::FlattenInstOptionsDialog (QWidget *parent, bool enable_pruning)
  : QDialog (parent)
{ 
  setupUi (this);

  if (! enable_pruning) {
    prune_cb->setChecked (false);
    prune_cb->hide ();
  }
}

bool 
FlattenInstOptionsDialog::exec_dialog (int &levels, bool &prune) 
{
  first_level_rb->setChecked (false);
  all_levels_rb->setChecked (false);
  spec_levels_rb->setChecked (false);
  levels_sb->setValue ((levels < 0 || levels > levels_sb->maximum ()) ? levels_sb->maximum () : levels);

  if (levels == 1) {
    first_level_rb->setChecked (true);
  } else if (levels < 0 || levels == std::numeric_limits<int>::max ()) {
    all_levels_rb->setChecked (true);
  } else {
    spec_levels_rb->setChecked (true);
  }

  prune_cb->setChecked (prune);

  if (QDialog::exec ()) {

    prune = prune_cb->isChecked ();

    if (first_level_rb->isChecked ()) {
      levels = 1;
      return true;
    } else if (spec_levels_rb->isChecked ()) {
      levels = levels_sb->value ();
      return true;
    } else if (all_levels_rb->isChecked ()) {
      levels = std::numeric_limits<int>::max ();
      return true;
    }

  } 
  return false;
}

// ----------------------------------------------------------------------
//  UserPropertiesForm implementation

UserPropertiesForm::UserPropertiesForm (QWidget *parent)
  : QDialog (parent), m_editable (false)
{
  setObjectName (QString::fromUtf8 ("user_properties_form"));

  Ui::UserPropertiesForm::setupUi (this);

  connect (add_pb, SIGNAL (clicked ()), this, SLOT (add ()));
  connect (remove_pb, SIGNAL (clicked ()), this, SLOT (remove ()));
  connect (edit_pb, SIGNAL (clicked ()), this, SLOT (edit ()));
  connect (prop_list, SIGNAL (itemDoubleClicked (QTreeWidgetItem *, int)), this, SLOT (dbl_clicked (QTreeWidgetItem *, int)));
}

bool 
UserPropertiesForm::show (lay::LayoutView *view, unsigned int cv_index, db::properties_id_type &prop_id)
{
  bool ret = false;

BEGIN_PROTECTED

  const lay::CellView &cv = view->cellview (cv_index);
  db::PropertiesRepository &prep = cv->layout ().properties_repository ();

  m_editable = cv->layout ().is_editable ();
  if (m_editable) {
    edit_frame->show ();
  } else {
    edit_frame->hide ();
  }

  prop_list->clear ();

  const db::PropertiesRepository::properties_set &props = prep.properties (prop_id);
  for (db::PropertiesRepository::properties_set::const_iterator p = props.begin (); p != props.end (); ++p) {
    QTreeWidgetItem *entry = new QTreeWidgetItem (prop_list);
    entry->setText (0, tl::to_qstring (prep.prop_name (p->first).to_parsable_string ()));
    entry->setText (1, tl::to_qstring (p->second.to_parsable_string ()));
  }

  if (exec ()) {

    db::PropertiesRepository::properties_set props;

    QTreeWidgetItemIterator it (prop_list);
    while (*it) {

      tl::Variant v, k;
      QString t1 = (*it)->text (1);
      std::string vs (tl::to_string (t1));
      tl::Extractor vex (vs.c_str ());
      vex.read (v);
      vex.expect_end ();
      QString t0 = (*it)->text (0);
      std::string ks (tl::to_string (t0));
      tl::Extractor kex (ks.c_str ());
      kex.read (k);
      kex.expect_end ();

      props.insert (std::make_pair (prep.prop_name_id (k), v));

      ++it;

    }

    prop_id = prep.properties_id (props);

    ret = true;

  } else {
    ret = false;
  }

END_PROTECTED

  return ret;
}

void 
UserPropertiesForm::add ()
{
BEGIN_PROTECTED

  if (!m_editable) {
    return;
  }

  QString key, value;

  UserPropertiesEditForm edit_form (this);
  if (edit_form.show (key, value)) {

    QTreeWidgetItem *entry = new QTreeWidgetItem (prop_list);
    entry->setText (0, key);
    entry->setText (1, value);

    prop_list->setCurrentItem (entry);

  }

END_PROTECTED
}

void  
UserPropertiesForm::remove ()
{
BEGIN_PROTECTED

  if (!m_editable) {
    return;
  }

  if (prop_list->currentItem () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("Select an item to delete")));
  }

  delete prop_list->currentItem ();

END_PROTECTED
}

void  
UserPropertiesForm::dbl_clicked (QTreeWidgetItem *, int)
{
  edit ();
}

void  
UserPropertiesForm::edit ()
{
BEGIN_PROTECTED

  if (!m_editable) {
    return;
  }

  if (prop_list->currentItem () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("Select an item to edit")));
  }

  QString key = prop_list->currentItem ()->text (0);
  QString value = prop_list->currentItem ()->text (1);

  UserPropertiesEditForm edit_form (this);
  if (edit_form.show (key, value)) {
    prop_list->currentItem ()->setText (0, key);
    prop_list->currentItem ()->setText (1, value);
  }

END_PROTECTED
}

// ----------------------------------------------------------------------
//  UserPropertiesEditForm implementation

UserPropertiesEditForm::UserPropertiesEditForm (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("user_properties_edit_form"));

  Ui::UserPropertiesEditForm::setupUi (this);

  activate_help_links (help_label);
}

static QString 
normalize (const QString &s)
{
  std::string st (tl::to_string (s));
  const char *c = st.c_str ();

  tl::Variant v;

  if (*c == '#' || *c == '\"' || *c == '\'') {
    tl::Extractor ex (c);
    ex.read (v);
    ex.expect_end ();
  } else {
    v = tl::Variant (std::string (c));
  }

  return tl::to_qstring (v.to_parsable_string ());
}

bool 
UserPropertiesEditForm::show (QString &key, QString &value)
{
BEGIN_PROTECTED

  key_le->setText (key);
  value_le->setText (value);

  if (exec ()) {
    key = normalize (key_le->text ());
    value = normalize (value_le->text ());
    return true;
  }

END_PROTECTED
  return false;
}

void 
UserPropertiesEditForm::accept ()
{
BEGIN_PROTECTED

  normalize (key_le->text ());
  normalize (value_le->text ());

  QDialog::accept ();

END_PROTECTED
}

}



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

#include "layDialogs.h"

#include "dbLayout.h"

#include "tlString.h"
#include "tlException.h"
#include "tlExceptions.h"
#include "tlXMLParser.h"

#include "layLayerProperties.h"
#include "layFileDialog.h"
#include "layLayoutViewBase.h"
#include "layCellTreeModel.h"
#include "layQtTools.h"
#include "layGenericSyntaxHighlighter.h"

#include "ui_LayerSourceDialog.h"
#include "ui_NewLayoutPropertiesDialog.h"
#include "ui_NewLayerPropertiesDialog.h"
#include "ui_NewCellPropertiesDialog.h"
#include "ui_MoveOptionsDialog.h"
#include "ui_MoveToOptionsDialog.h"
#include "ui_DeleteCellModeDialog.h"
#include "ui_CopyCellModeDialog.h"
#include "ui_ReplaceCellOptionsDialog.h"
#include "ui_ClearLayerModeDialog.h"
#include "ui_OpenLayoutModeDialog.h"
#include "ui_RenameCellDialog.h"
#include "ui_DuplicateLayerDialog.h"
#include "ui_AlignCellOptionsDialog.h"
#include "ui_FlattenInstOptionsDialog.h"
#include "ui_UserPropertiesForm.h"
#include "ui_UserPropertiesEditForm.h"

#include <QResource>
#include <QBuffer>

namespace lay
{

// --------------------------------------------------------------------------------
//  LayerSourceDialog implementation

LayerSourceDialog::LayerSourceDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("layer_source_dialog"));
  
  mp_ui = new Ui::LayerSourceDialog ();
  mp_ui->setupUi (this);

  activate_help_links (mp_ui->helpLabel);
}

LayerSourceDialog::~LayerSourceDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

bool 
LayerSourceDialog::exec_dialog (std::string &s)
{
  mp_ui->sourceString->setText (tl::to_qstring (s));
  if (QDialog::exec ()) {
    s = tl::to_string (mp_ui->sourceString->text ());
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  NewLayoutPropertiesDialog implementation

NewLayoutPropertiesDialog::NewLayoutPropertiesDialog (QWidget *parent)
  : QDialog (parent), m_default_dbu (0.0)
{
  setObjectName (QString::fromUtf8 ("new_layout_properties_dialog"));

  mp_ui = new Ui::NewLayoutPropertiesDialog ();
  mp_ui->setupUi (this);

  connect (mp_ui->tech_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (tech_changed ()));
}

NewLayoutPropertiesDialog::~NewLayoutPropertiesDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
NewLayoutPropertiesDialog::tech_changed ()
{
  double dbu = 0.0;
  int technology_index = mp_ui->tech_cbx->currentIndex ();
  if (technology_index >= 0 && technology_index < (int) db::Technologies::instance ()->technologies ()) {
    dbu = db::Technologies::instance ()->begin () [technology_index].dbu ();
  }

  m_default_dbu = dbu;

#if QT_VERSION >= 0x40700
  if (dbu > 1e-10) {
    mp_ui->dbu_le->setPlaceholderText (tl::to_qstring (tl::to_string (dbu)));
  } else {
    mp_ui->dbu_le->setPlaceholderText (QString ());
  }
#endif
}

bool 
NewLayoutPropertiesDialog::exec_dialog (std::string &technology, std::string &cell_name, double &dbu, double &size, std::vector<db::LayerProperties> &layers, bool &current_panel)
{
  mp_ui->tech_cbx->clear ();
  unsigned int technology_index = 0;
  for (db::Technologies::const_iterator t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t, ++technology_index) {

    mp_ui->tech_cbx->addItem (tl::to_qstring (t->get_display_string ()));
    if (t->name () == technology) {
      mp_ui->tech_cbx->setCurrentIndex (technology_index);
    }

  }

  tech_changed ();

  mp_ui->window_le->setText (tl::to_qstring (tl::to_string (size)));
  if (dbu > 1e-10) {
    mp_ui->dbu_le->setText (tl::to_qstring (tl::to_string (dbu)));
  } else {
    mp_ui->dbu_le->setText (QString ());
  }
  mp_ui->topcell_le->setText (tl::to_qstring (cell_name));
  mp_ui->current_panel_cb->setChecked (current_panel);

  std::string layer_string;
  for (std::vector<db::LayerProperties>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
    if (l != layers.begin ()) {
      layer_string += ", ";
    }
    layer_string += l->to_string ();
  }
  mp_ui->layers_le->setText (tl::to_qstring (layer_string));

  if (QDialog::exec ()) {

    //  get the selected technology name
    int technology_index = mp_ui->tech_cbx->currentIndex ();
    if (technology_index >= 0 && technology_index < (int) db::Technologies::instance ()->technologies ()) {
      technology = db::Technologies::instance ()->begin () [technology_index].name ();
    } else {
      technology = std::string ();
    }

    tl::from_string_ext (tl::to_string (mp_ui->window_le->text ()), size);
    if (! mp_ui->dbu_le->text ().isEmpty ()) {
      tl::from_string_ext (tl::to_string (mp_ui->dbu_le->text ()), dbu);
    } else {
      dbu = m_default_dbu;
    }

    cell_name = tl::to_string (mp_ui->topcell_le->text ());
    current_panel = mp_ui->current_panel_cb->isChecked ();

    layers.clear ();
    layer_string = tl::to_string (mp_ui->layers_le->text ());
    tl::Extractor ex (layer_string.c_str ());
    while (! ex.at_end ()) {
      db::LayerProperties lp;
      try {
        lp.read (ex);
      } catch (...) {
        break;
      }
      layers.push_back (lp);
      ex.test (",");
    }

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
  tl::from_string_ext (tl::to_string (mp_ui->window_le->text ()), x);
  if (!mp_ui->dbu_le->text ().isEmpty ()) {
    tl::from_string_ext (tl::to_string (mp_ui->dbu_le->text ()), x);
  }

  if (mp_ui->topcell_le->text ().isEmpty ()) {
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

  mp_ui = new Ui::NewCellPropertiesDialog ();
  mp_ui->setupUi (this);
}

NewCellPropertiesDialog::~NewCellPropertiesDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

bool 
NewCellPropertiesDialog::exec_dialog (const db::Layout *layout, std::string &cell_name, double &size)
{
  mp_layout = layout;

  mp_ui->name_le->setText (tl::to_qstring (cell_name));
  mp_ui->window_le->setText (tl::to_qstring (tl::to_string (size)));

  if (QDialog::exec ()) {

    tl::from_string_ext (tl::to_string (mp_ui->window_le->text ()), size);
    cell_name = tl::to_string (mp_ui->name_le->text ());
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
  tl::from_string_ext (tl::to_string (mp_ui->window_le->text ()), x);

  if (mp_layout->cell_by_name (tl::to_string (mp_ui->name_le->text ()).c_str ()).first) {
    throw tl::Exception (tl::to_string (QObject::tr ("A cell with that name already exists: %s")), tl::to_string (mp_ui->name_le->text ()));
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

  mp_ui = new Ui::NewLayerPropertiesDialog ();
  mp_ui->setupUi (this);
}

NewLayerPropertiesDialog::~NewLayerPropertiesDialog ()
{
  delete mp_ui;
  mp_ui = 0;
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
    mp_ui->layout_lbl->setText (tl::to_qstring ((tl::to_string (QObject::tr ("Layer for layout: ")) + cv->name ())));
    mp_ui->layout_lbl->show ();
  } else {
    mp_ui->layout_lbl->hide ();
  }

  if (src.layer >= 0) {
    mp_ui->layer_le->setText (tl::to_qstring (tl::to_string (src.layer)));
  } else {
    mp_ui->layer_le->setText (QString ());
  }
  if (src.datatype >= 0) {
    mp_ui->datatype_le->setText (tl::to_qstring (tl::to_string (src.datatype)));
  } else {
    mp_ui->datatype_le->setText (QString ());
  }
  mp_ui->name_le->setText (tl::to_qstring (src.name));

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
  if (! mp_ui->layer_le->text ().isEmpty ()) {
    int l = -1;
    tl::from_string_ext (tl::to_string (mp_ui->layer_le->text ()), l);
    src.layer = l;
  } else {
    src.layer = -1;
  }
  if (! mp_ui->datatype_le->text ().isEmpty ()) {
    int d = -1;
    tl::from_string_ext (tl::to_string (mp_ui->datatype_le->text ()), d);
    src.datatype = d;
  } else {
    src.datatype = -1;
  }
  src.name = tl::to_string (mp_ui->name_le->text ());
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

  mp_ui = new Ui::MoveOptionsDialog ();
  mp_ui->setupUi (this);
}

MoveOptionsDialog::~MoveOptionsDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

bool 
MoveOptionsDialog::exec_dialog (db::DVector &disp)
{
  mp_ui->disp_x_le->setText (tl::to_qstring (tl::to_string (disp.x ())));
  mp_ui->disp_y_le->setText (tl::to_qstring (tl::to_string (disp.y ())));

  if (QDialog::exec ()) {

    double x = 0.0, y = 0.0;
    tl::from_string_ext (tl::to_string (mp_ui->disp_x_le->text ()), x);
    tl::from_string_ext (tl::to_string (mp_ui->disp_y_le->text ()), y);

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
  tl::from_string_ext (tl::to_string (mp_ui->disp_x_le->text ()), x);
  tl::from_string_ext (tl::to_string (mp_ui->disp_y_le->text ()), x);
  QDialog::accept ();
END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  MoveToOptionsDialog implementation

MoveToOptionsDialog::MoveToOptionsDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("move_to_options_dialog"));

  mp_ui = new Ui::MoveToOptionsDialog ();
  mp_ui->setupUi (this);

  QToolButton *buttons[3][3] = { { mp_ui->lb, mp_ui->cb, mp_ui->rb }, { mp_ui->lc, mp_ui->cc, mp_ui->rc }, { mp_ui->lt, mp_ui->ct, mp_ui->rt } };

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      connect (buttons[i][j], SIGNAL (clicked ()), this, SLOT (button_clicked ()));
    }
  }
}

MoveToOptionsDialog::~MoveToOptionsDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

bool 
MoveToOptionsDialog::exec_dialog (int &mode_x, int &mode_y, db::DPoint &target)
{
  mp_ui->x_le->setText (tl::to_qstring (tl::to_string (target.x ())));
  mp_ui->y_le->setText (tl::to_qstring (tl::to_string (target.y ())));

  QToolButton *buttons[3][3] = { { mp_ui->lb, mp_ui->cb, mp_ui->rb }, { mp_ui->lc, mp_ui->cc, mp_ui->rc }, { mp_ui->lt, mp_ui->ct, mp_ui->rt } };

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
    tl::from_string_ext (tl::to_string (mp_ui->x_le->text ()), x);
    tl::from_string_ext (tl::to_string (mp_ui->y_le->text ()), y);

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
  tl::from_string_ext (tl::to_string (mp_ui->x_le->text ()), x);
  tl::from_string_ext (tl::to_string (mp_ui->y_le->text ()), x);
  QDialog::accept ();
END_PROTECTED;
}

void 
MoveToOptionsDialog::button_clicked ()
{
  QToolButton *buttons[3][3] = { { mp_ui->lb, mp_ui->cb, mp_ui->rb }, { mp_ui->lc, mp_ui->cc, mp_ui->rc }, { mp_ui->lt, mp_ui->ct, mp_ui->rt } };

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

  mp_ui = new Ui::RenameCellDialog ();
  mp_ui->setupUi (this);
}

RenameCellDialog::~RenameCellDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
RenameCellDialog::accept ()
{
BEGIN_PROTECTED;
  if (mp_ui->name_le->text ().isEmpty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("A name must be given")));
  }
  if (mp_layout->cell_by_name (tl::to_string (mp_ui->name_le->text ()).c_str ()).first) {
    throw tl::Exception (tl::to_string (QObject::tr ("A cell with that name already exists")));
  }
  QDialog::accept ();
END_PROTECTED;
}

bool 
RenameCellDialog::exec_dialog (const db::Layout &layout, std::string &name)
{
  mp_layout = &layout;
  mp_ui->name_le->setText (tl::to_qstring (name));
  if (QDialog::exec ()) {
    name = tl::to_string (mp_ui->name_le->text ());
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

  mp_ui = new Ui::CopyCellModeDialog ();
  mp_ui->setupUi (this);
}

CopyCellModeDialog::~CopyCellModeDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

bool 
CopyCellModeDialog::exec_dialog (int &copy_mode)
{
  QRadioButton *buttons [] = { mp_ui->shallow_rb, mp_ui->deep_rb };

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

  mp_ui = new Ui::DeleteCellModeDialog ();
  mp_ui->setupUi (this);
}

DeleteCellModeDialog::~DeleteCellModeDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

bool 
DeleteCellModeDialog::exec_dialog (int &delete_mode)
{
  QRadioButton *buttons [] = { mp_ui->shallow_rb, mp_ui->deep_rb, mp_ui->full_rb };

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

  mp_ui = new Ui::ReplaceCellOptionsDialog ();
  mp_ui->setupUi (this);
}

ReplaceCellOptionsDialog::~ReplaceCellOptionsDialog ()
{
  delete mp_ui;
  mp_ui = 0;
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
  QRadioButton *buttons [] = { mp_ui->shallow_rb, mp_ui->deep_rb, mp_ui->full_rb };

  for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
    buttons [i]->setChecked (replace_mode == i);
  }

  lay::CellTreeModel *model = new lay::CellTreeModel (mp_ui->cell_selection_cbx, &cv->layout (), lay::CellTreeModel::Flat | lay::CellTreeModel::NoPadding);
  mp_ui->cell_selection_cbx->setModel (model);
  mp_ui->cell_selection_cbx->setEditText (tl::to_qstring (cv->layout ().cell_name (cell_index)));

  if (QDialog::exec ()) {

    for (int i = 0; i < int (sizeof (buttons) / sizeof (buttons [0])); ++i) {
      if (buttons [i]->isChecked ()) {
        replace_mode = i;
      }
    }

    std::string cn = tl::to_string (mp_ui->cell_selection_cbx->lineEdit ()->text ());
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

  lay::CellTreeModel *model = dynamic_cast<lay::CellTreeModel *> (mp_ui->cell_selection_cbx->model ());
  if (model) {
    std::string cn = tl::to_string (mp_ui->cell_selection_cbx->lineEdit ()->text ());
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

  mp_ui = new Ui::ClearLayerModeDialog ();
  mp_ui->setupUi (this);
}

ClearLayerModeDialog::~ClearLayerModeDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

bool 
ClearLayerModeDialog::exec_dialog (int &clear_mode)
{
  QRadioButton *buttons [3] = { mp_ui->local_rb, mp_ui->hierarchically_rb, mp_ui->layout_rb };

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

  mp_ui = new Ui::OpenLayoutModeDialog ();
  mp_ui->setupUi (this);
}

OpenLayoutModeDialog::~OpenLayoutModeDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

bool 
OpenLayoutModeDialog::exec_dialog (int &open_mode)
{
  QRadioButton *buttons [3] = { mp_ui->replace_rb, mp_ui->new_rb, mp_ui->add_rb };

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

  mp_ui = new Ui::DuplicateLayerDialog ();
  mp_ui->setupUi (this);

  connect (mp_ui->cv_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
  connect (mp_ui->cvr_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
}

DuplicateLayerDialog::~DuplicateLayerDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
DuplicateLayerDialog::cv_changed (int)
{
  if (! mp_view) {
    return;
  }

  mp_ui->layer_cbx->set_view (mp_view, mp_ui->cv_cbx->currentIndex ());
  mp_ui->layerr_cbx->set_view (mp_view, mp_ui->cvr_cbx->currentIndex ());
}

bool 
DuplicateLayerDialog::exec_dialog (lay::LayoutViewBase *view, int &cv, int &layer, int &cv_r, int &layer_r, int &hier_mode, bool &clear_before)
{
  mp_view = view;

  bool res = false;
  mp_ui->cv_cbx->set_layout_view (view);
  mp_ui->cv_cbx->set_current_cv_index (cv);
  mp_ui->cvr_cbx->set_layout_view (view);
  mp_ui->cvr_cbx->set_current_cv_index (cv_r);

  cv_changed (0 /*dummy*/);

  mp_ui->layer_cbx->set_current_layer (layer);
  mp_ui->layerr_cbx->set_current_layer (layer_r);

  mp_ui->hier_mode_cbx->setCurrentIndex (hier_mode);
  mp_ui->clear_cb->setChecked (clear_before);

  if (QDialog::exec ()) {

    cv = mp_ui->cv_cbx->current_cv_index ();
    cv_r = mp_ui->cvr_cbx->current_cv_index ();
    layer = mp_ui->layer_cbx->current_layer ();
    layer_r = mp_ui->layerr_cbx->current_layer ();

    hier_mode = mp_ui->hier_mode_cbx->currentIndex ();
    clear_before = mp_ui->clear_cb->isChecked ();

    res = true;

  }

  mp_view = 0;
  return res;
}

void 
DuplicateLayerDialog::accept ()
{
BEGIN_PROTECTED;

  int cv = mp_ui->cv_cbx->current_cv_index ();
  if (cv < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout specified for source")));
  }

  int cv_r = mp_ui->cvr_cbx->current_cv_index ();
  if (cv_r < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout specified for result")));
  }

  if (fabs (mp_view->cellview (cv)->layout ().dbu () - mp_view->cellview (cv_r)->layout ().dbu ()) > db::epsilon) {
    throw tl::Exception (tl::to_string (QObject::tr ("Source and result layouts must have the same database unit")));
  }

  if (mp_ui->layer_cbx->current_layer () < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer specified for source")));
  }
  if (mp_ui->layerr_cbx->current_layer () < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer specified for result")));
  }

  if (mp_ui->hier_mode_cbx->currentIndex () == 2 &&
      mp_ui->cv_cbx->current_cv_index () != mp_ui->cvr_cbx->current_cv_index ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Source layout and result layout must be same in 'cell by cell' mode")));
  }

  if (mp_ui->cv_cbx->current_cv_index () == mp_ui->cvr_cbx->current_cv_index () && mp_ui->layer_cbx->current_layer () == mp_ui->layerr_cbx->current_layer ()) {
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

  mp_ui = new Ui::AlignCellOptionsDialog ();
  mp_ui->setupUi (this);

  QToolButton *buttons[3][3] = { { mp_ui->lb, mp_ui->cb, mp_ui->rb }, { mp_ui->lc, mp_ui->cc, mp_ui->rc }, { mp_ui->lt, mp_ui->ct, mp_ui->rt } };

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      connect (buttons[i][j], SIGNAL (clicked ()), this, SLOT (button_clicked ()));
    }
  }
}

AlignCellOptionsDialog::~AlignCellOptionsDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

bool 
AlignCellOptionsDialog::exec_dialog (AlignCellOptions &data)
{
  mp_ui->vis_only_cbx->setChecked (data.visible_only);
  mp_ui->adjust_calls_cbx->setChecked (data.adjust_parents);

  QToolButton *buttons[3][3] = { { mp_ui->lb, mp_ui->cb, mp_ui->rb }, { mp_ui->lc, mp_ui->cc, mp_ui->rc }, { mp_ui->lt, mp_ui->ct, mp_ui->rt } };

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      buttons[i][j]->setChecked (j - 1 == data.mode_x && i - 1 == data.mode_y);
    }
  }

  mp_ui->x_le->setText (tl::to_qstring (tl::micron_to_string (data.xpos)));
  mp_ui->y_le->setText (tl::to_qstring (tl::micron_to_string (data.ypos)));

  if (QDialog::exec ()) {

    data.visible_only = mp_ui->vis_only_cbx->isChecked ();
    data.adjust_parents = mp_ui->adjust_calls_cbx->isChecked ();

    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        if (buttons[i][j]->isChecked ()) {
          data.mode_x = j - 1;
          data.mode_y = i - 1;
        }
      }
    }

    tl::from_string_ext (tl::to_string (mp_ui->x_le->text ()), data.xpos);
    tl::from_string_ext (tl::to_string (mp_ui->y_le->text ()), data.ypos);

    return true;

  } else {
    return false;
  }
}

void
AlignCellOptionsDialog::accept ()
{
BEGIN_PROTECTED;

  double x = 0.0;
  tl::from_string_ext (tl::to_string (mp_ui->x_le->text ()), x);
  tl::from_string_ext (tl::to_string (mp_ui->y_le->text ()), x);

  QDialog::accept ();

END_PROTECTED;
}

void
AlignCellOptionsDialog::button_clicked ()
{
  QToolButton *buttons[3][3] = { { mp_ui->lb, mp_ui->cb, mp_ui->rb }, { mp_ui->lc, mp_ui->cc, mp_ui->rc }, { mp_ui->lt, mp_ui->ct, mp_ui->rt } };

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
  mp_ui = new Ui::FlattenInstOptionsDialog ();
  mp_ui->setupUi (this);

  if (! enable_pruning) {
    mp_ui->prune_cb->setChecked (false);
    mp_ui->prune_cb->hide ();
  }
}

FlattenInstOptionsDialog::~FlattenInstOptionsDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

bool
FlattenInstOptionsDialog::exec_dialog (int &levels, bool &prune) 
{
  mp_ui->first_level_rb->setChecked (false);
  mp_ui->all_levels_rb->setChecked (false);
  mp_ui->spec_levels_rb->setChecked (false);
  mp_ui->levels_sb->setValue ((levels < 0 || levels > mp_ui->levels_sb->maximum ()) ? mp_ui->levels_sb->maximum () : levels);

  if (levels == 1) {
    mp_ui->first_level_rb->setChecked (true);
  } else if (levels < 0 || levels == std::numeric_limits<int>::max ()) {
    mp_ui->all_levels_rb->setChecked (true);
  } else {
    mp_ui->spec_levels_rb->setChecked (true);
  }

  mp_ui->prune_cb->setChecked (prune);

  if (QDialog::exec ()) {

    prune = mp_ui->prune_cb->isChecked ();

    if (mp_ui->first_level_rb->isChecked ()) {
      levels = 1;
      return true;
    } else if (mp_ui->spec_levels_rb->isChecked ()) {
      levels = mp_ui->levels_sb->value ();
      return true;
    } else if (mp_ui->all_levels_rb->isChecked ()) {
      levels = std::numeric_limits<int>::max ();
      return true;
    }

  } 
  return false;
}

// ----------------------------------------------------------------------
//  UserPropertiesForm implementation

UserPropertiesForm::UserPropertiesForm (QWidget *parent)
  : QDialog (parent), m_editable (false), mp_prep (0)
{
  setObjectName (QString::fromUtf8 ("user_properties_form"));

  mp_ui = new Ui::UserPropertiesForm ();
  mp_ui->setupUi (this);

  mp_ui->text_edit->setFont (monospace_font ());
  mp_ui->text_edit->setAcceptRichText (false);

  connect (mp_ui->add_pb, SIGNAL (clicked ()), this, SLOT (add ()));
  connect (mp_ui->remove_pb, SIGNAL (clicked ()), this, SLOT (remove ()));
  connect (mp_ui->edit_pb, SIGNAL (clicked ()), this, SLOT (edit ()));
  connect (mp_ui->prop_list, SIGNAL (itemDoubleClicked (QTreeWidgetItem *, int)), this, SLOT (dbl_clicked (QTreeWidgetItem *, int)));
  connect (mp_ui->mode_tab, SIGNAL (currentChanged (int)), this, SLOT (tab_changed (int)));

  activate_help_links (mp_ui->help_label);

  QResource res (tl::to_qstring (":/syntax/ur_text.xml"));
  QByteArray data ((const char *) res.data (), int (res.size ()));
#if QT_VERSION >= 0x60000
  if (res.compressionAlgorithm () == QResource::ZlibCompression) {
#else
  if (res.isCompressed ()) {
#endif
    data = qUncompress (data);
  }

  QBuffer input (&data);
  input.open (QIODevice::ReadOnly);
  mp_hl_basic_attributes.reset (new GenericSyntaxHighlighterAttributes ());
  mp_hl_attributes.reset (new GenericSyntaxHighlighterAttributes (mp_hl_basic_attributes.get ()));
  lay::GenericSyntaxHighlighter *hl = new GenericSyntaxHighlighter (mp_ui->text_edit, input, mp_hl_attributes.get (), true);
  input.close ();

  hl->setDocument (mp_ui->text_edit->document ());
}

UserPropertiesForm::~UserPropertiesForm ()
{
  delete mp_ui;
  mp_ui = 0;
}

db::PropertiesRepository::properties_set
UserPropertiesForm::get_properties (int tab)
{
  db::PropertiesRepository::properties_set props;

  if (tab == 0) {

    QTreeWidgetItemIterator it (mp_ui->prop_list);
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

      props.insert (std::make_pair (mp_prep->prop_name_id (k), v));

      ++it;

    }

  } else {

    std::string text = tl::to_string (mp_ui->text_edit->toPlainText ());
    std::vector<std::string> lines = tl::split (text, "\n");

    for (std::vector<std::string>::const_iterator l = lines.begin (); l != lines.end (); ++l) {

      tl::Extractor ex (l->c_str ());
      if (ex.at_end ()) {
        //  empty line
      } else {

        tl::Variant v, k;
        ex.read (k);
        ex.test (":");
        ex.read (v);
        ex.expect_end ();

        props.insert (std::make_pair (mp_prep->prop_name_id (k), v));

      }

    }

  }

  return props;
}

void
UserPropertiesForm::set_properties (const db::PropertiesRepository::properties_set &props)
{
  mp_ui->prop_list->clear ();

  for (db::PropertiesRepository::properties_set::const_iterator p = props.begin (); p != props.end (); ++p) {
    QTreeWidgetItem *entry = new QTreeWidgetItem (mp_ui->prop_list);
    entry->setText (0, tl::to_qstring (mp_prep->prop_name (p->first).to_parsable_string ()));
    entry->setText (1, tl::to_qstring (p->second.to_parsable_string ()));
  }

  std::string text;
  for (db::PropertiesRepository::properties_set::const_iterator p = props.begin (); p != props.end (); ++p) {
    text += mp_prep->prop_name (p->first).to_parsable_string ();
    text += ": ";
    text += p->second.to_parsable_string ();
    text += "\n";
  }

  mp_ui->text_edit->setPlainText (tl::to_qstring (text));
}

void
UserPropertiesForm::set_meta_info (db::Layout::meta_info_iterator begin_meta, db::Layout::meta_info_iterator end_meta, const db::Layout &layout)
{
  m_begin_meta = begin_meta;
  m_end_meta = end_meta;

#if QT_VERSION >= 0x50F00
  mp_ui->mode_tab->setTabVisible (2, m_begin_meta != m_end_meta);
#endif

  mp_ui->meta_info_list->clear ();

  for (auto m = m_begin_meta; m != m_end_meta; ++m) {
    QTreeWidgetItem *entry = new QTreeWidgetItem (mp_ui->meta_info_list);
    entry->setText (0, tl::to_qstring ((m->second.persisted ? "*" : "") + layout.meta_info_name (m->first)));
    entry->setText (1, tl::to_qstring (m->second.description));
    entry->setText (2, tl::to_qstring (m->second.value.to_parsable_string ()));
  }
}

bool
UserPropertiesForm::show (LayoutViewBase *view, unsigned int cv_index, db::properties_id_type &prop_id)
{
  return show (view, cv_index, prop_id, db::Layout::meta_info_iterator (), db::Layout::meta_info_iterator ());
}

bool
UserPropertiesForm::show (LayoutViewBase *view, unsigned int cv_index, db::properties_id_type &prop_id, db::Layout::meta_info_iterator begin_meta, db::Layout::meta_info_iterator end_meta)
{
  bool ret = false;

BEGIN_PROTECTED

  const lay::CellView &cv = view->cellview (cv_index);
  mp_prep = &cv->layout ().properties_repository ();

  m_editable = cv->layout ().is_editable ();
  if (m_editable) {
    mp_ui->edit_frame->show ();
  } else {
    mp_ui->edit_frame->hide ();
  }

  mp_ui->text_edit->setReadOnly (! m_editable);
  mp_ui->prop_list->clear ();

  const db::PropertiesRepository::properties_set &props = mp_prep->properties (prop_id);
  set_properties (props);

  set_meta_info (begin_meta, end_meta, cv->layout ());

  if (exec ()) {

    if (m_editable) {
      db::PropertiesRepository::properties_set props = get_properties (mp_ui->mode_tab->currentIndex ());
      prop_id = mp_prep->properties_id (props);
    }

    ret = true;

  } else {
    ret = false;
  }

  mp_prep = 0;

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

    QTreeWidgetItem *entry = new QTreeWidgetItem (mp_ui->prop_list);
    entry->setText (0, key);
    entry->setText (1, value);

    mp_ui->prop_list->setCurrentItem (entry);

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

  if (mp_ui->prop_list->currentItem () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("Select an item to delete")));
  }

  delete mp_ui->prop_list->currentItem ();

END_PROTECTED
}

void  
UserPropertiesForm::dbl_clicked (QTreeWidgetItem *, int)
{
  edit ();
}

void
UserPropertiesForm::tab_changed (int tab_index)
{
  if (! m_editable) {
    return;
  }

BEGIN_PROTECTED

  int prev_tab = tab_index == 0 ? 1 : 0;

  try {

    //  sync content
    set_properties (get_properties (prev_tab));

  } catch (...) {

    mp_ui->mode_tab->blockSignals (true);
    mp_ui->mode_tab->setCurrentIndex (prev_tab);
    mp_ui->mode_tab->blockSignals (false);

    throw;

  }

END_PROTECTED
}

void
UserPropertiesForm::accept ()
{
BEGIN_PROTECTED

  //  Test for errors
  if (m_editable) {
    get_properties (mp_ui->mode_tab->currentIndex ());
  }

  QDialog::accept ();

END_PROTECTED
}

void  
UserPropertiesForm::edit ()
{
BEGIN_PROTECTED

  if (!m_editable) {
    return;
  }

  if (mp_ui->prop_list->currentItem () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("Select an item to edit")));
  }

  QString key = mp_ui->prop_list->currentItem ()->text (0);
  QString value = mp_ui->prop_list->currentItem ()->text (1);

  UserPropertiesEditForm edit_form (this);
  if (edit_form.show (key, value)) {
    mp_ui->prop_list->currentItem ()->setText (0, key);
    mp_ui->prop_list->currentItem ()->setText (1, value);
  }

END_PROTECTED
}

// ----------------------------------------------------------------------
//  UserPropertiesEditForm implementation

UserPropertiesEditForm::UserPropertiesEditForm (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("user_properties_edit_form"));

  mp_ui = new Ui::UserPropertiesEditForm ();
  mp_ui->setupUi (this);

  activate_help_links (mp_ui->help_label);
}

UserPropertiesEditForm::~UserPropertiesEditForm ()
{
  delete mp_ui;
  mp_ui = 0;
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

  mp_ui->key_le->setText (key);
  mp_ui->value_le->setText (value);

  if (exec ()) {
    key = normalize (mp_ui->key_le->text ());
    value = normalize (mp_ui->value_le->text ());
    return true;
  }

END_PROTECTED
  return false;
}

void 
UserPropertiesEditForm::accept ()
{
BEGIN_PROTECTED

  normalize (mp_ui->key_le->text ());
  normalize (mp_ui->value_le->text ());

  QDialog::accept ();

END_PROTECTED
}

}

#endif

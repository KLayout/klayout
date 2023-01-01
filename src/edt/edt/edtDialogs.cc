
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

#include "dbBox.h"
#include "dbLayout.h"

#include "edtDialogs.h"
#include "layObjectInstPath.h"
#include "layCellView.h"
#include "layLayoutViewBase.h"
#include "layMarker.h"
#include "tlException.h"
#include "tlExceptions.h"

namespace edt {

// ----------------------------------------------------------------------
//  edt::InstantiationForm implementation

InstantiationForm::InstantiationForm (QWidget *parent)
  : QDialog (parent), mp_view (0), mp_path (0), mp_marker (0), m_enable_cb_callbacks (false)
{
  setObjectName (QString::fromUtf8 ("instantiation_form"));

  Ui::InstantiationForm::setupUi (this);

  connect (list, SIGNAL (itemDoubleClicked (QListWidgetItem *)), this, SLOT (double_clicked (QListWidgetItem *)));
  connect (dbu_cb, SIGNAL (toggled (bool)), this, SLOT (display_mode_changed (bool)));
  connect (abs_cb, SIGNAL (toggled (bool)), this, SLOT (display_mode_changed (bool)));
}

InstantiationForm::~InstantiationForm ()
{
  if (mp_marker) {
    delete mp_marker;
    mp_marker = 0;
  }
}

void 
InstantiationForm::display_mode_changed (bool)
{
  if (! m_enable_cb_callbacks) {
    return;
  }

  mp_view->dbu_coordinates (dbu_cb->isChecked ());
  mp_view->absolute_coordinates (abs_cb->isChecked ());

  update ();
}

void 
InstantiationForm::double_clicked (QListWidgetItem *item)
{
  int row = list->row (item);
  if (row < 0) {
    return;
  }

  lay::CellView::unspecific_cell_path_type path (mp_view->cellview (mp_path->cv_index ()).combined_unspecific_path ());
  int nrow = 0;
  for (lay::ObjectInstPath::iterator p = mp_path->begin (); p != mp_path->end () && nrow < row; ++p, ++nrow) {
    path.push_back (p->inst_ptr.cell_index ());
  }

  mp_view->set_current_cell_path (mp_path->cv_index (), path);

  if (! mp_marker) {
    mp_marker = new lay::Marker (mp_view, mp_path->cv_index ());
  }

  const db::Layout &layout = mp_view->cellview (mp_path->cv_index ())->layout ();
  db::Box box = layout.cell (row == 0 ? mp_path->topcell () : path.back ()).bbox ();

  //  TODO: this does not consider global transformation and variants of this
  db::ICplxTrans abs_trans;
  nrow = 0;
  for (lay::ObjectInstPath::iterator p = mp_path->begin (); p != mp_path->end () && nrow < row; ++p, ++nrow) {
    abs_trans = abs_trans * (p->inst_ptr.cell_inst ().complex_trans (*(p->array_inst)));
  }

  mp_marker->set (box, abs_trans, mp_view->cv_transform_variants (mp_path->cv_index ()));
}

void 
InstantiationForm::show (lay::LayoutViewBase *view, const lay::ObjectInstPath &path)
{
  mp_view = view;
  mp_path = &path;

  m_enable_cb_callbacks = false;
  dbu_cb->setChecked (mp_view->dbu_coordinates ());
  abs_cb->setChecked (mp_view->absolute_coordinates ());
  m_enable_cb_callbacks = true;

  update ();
  exec ();

  mp_view = 0;
  mp_path = 0;
}

void 
InstantiationForm::update ()
{
  bool dbu_coord = dbu_cb->isChecked ();
  bool abs_coord = abs_cb->isChecked ();

  const lay::CellView &cv = mp_view->cellview (mp_path->cv_index ());
  double dbu = cv->layout ().dbu ();

  layout_le->setText (tl::to_qstring (cv->name ()));

  list->clear ();

  list->addItem (tl::to_qstring (cv->layout ().cell_name (cv.ctx_cell_index ())));
  db::CplxTrans abs_trans;

  //  first include the context path of the cellview in order to tell the path within the cell shown
  for (lay::CellView::specific_cell_path_type::const_iterator p = cv.specific_path ().begin (); p != cv.specific_path ().end (); ++p) {

    //  build the instance information from the path

    db::CplxTrans trans (p->inst_ptr.cell_inst ().complex_trans (*(p->array_inst)));
    abs_trans = abs_trans * trans;

    if (abs_coord) {
      trans = abs_trans;
    }

    std::string line;
    line += cv->layout ().cell_name (p->inst_ptr.cell_index ());
    line += "\tat ";
    line += trans.to_string (true /*lazy*/, dbu_coord ? 0.0 : dbu);
    
    list->addItem (tl::to_qstring (line));

  }

  //  then, add the actual path to the object within the target cell
  for (lay::ObjectInstPath::iterator p = mp_path->begin (); p != mp_path->end (); ++p) {

    //  build the instance information from the path

    db::CplxTrans trans (p->inst_ptr.cell_inst ().complex_trans (*(p->array_inst)));
    abs_trans = abs_trans * trans;

    if (abs_coord) {
      trans = abs_trans;
    }

    std::string line;
    line += cv->layout ().cell_name (p->inst_ptr.cell_index ());
    line += "\tat ";
    line += trans.to_string (true /*lazy*/, dbu_coord ? 0.0 : dbu);
    
    list->addItem (tl::to_qstring (line));

  }
}

// ----------------------------------------------------------------------
//  edt::CopyModeDialogForm implementation

CopyModeDialog::CopyModeDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("copy_mode_dialog"));

  Ui::CopyModeDialog::setupUi (this);
}

CopyModeDialog::~CopyModeDialog ()
{
  //  .. nothing yet ..
}

bool 
CopyModeDialog::exec_dialog (unsigned int &mode, bool &dont_ask)
{
  if (mode == 0) {
    shallow_rb->setChecked (true);
  }
  if (QDialog::exec ()) {
    if (shallow_rb->isChecked ()) {
      mode = 0;
    } else {
      mode = 1;
    }
    dont_ask = dont_ask_cbx->isChecked ();
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  ChangeLayerOptionsDialog implementation

ChangeLayerOptionsDialog::ChangeLayerOptionsDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("change_layer_options_dialog"));

  Ui::ChangeLayerOptionsDialog::setupUi (this);
}

ChangeLayerOptionsDialog::~ChangeLayerOptionsDialog ()
{
  //  .. nothing yet ..
}

bool 
ChangeLayerOptionsDialog::exec_dialog (lay::LayoutViewBase *view, int cv_index, unsigned int &new_layer)
{
  std::vector <std::pair <db::LayerProperties, unsigned int> > ll;

  const db::Layout &layout = view->cellview (cv_index)->layout ();
  for (unsigned int i = 0; i < layout.layers (); ++i) {
    if (layout.is_valid_layer (i)) {
      ll.push_back (std::make_pair (layout.get_properties (i), i));
    }
  }

  std::sort (ll.begin (), ll.end ());

  target_cbx->clear ();
  int initial_sel = -1;
  int i = 0;
  for (std::vector <std::pair <db::LayerProperties, unsigned int> >::const_iterator lp = ll.begin (); lp != ll.end (); ++lp, ++i) {
    if (lp->second == new_layer) {
      initial_sel = i;
    }
    target_cbx->addItem (tl::to_qstring (lay::ParsedLayerSource (lp->first, cv_index).to_string ()));
  }
  target_cbx->setCurrentIndex (initial_sel);

  if (QDialog::exec () && target_cbx->currentIndex () >= 0 && target_cbx->currentIndex () < int (ll.size ())) {
    new_layer = ll [target_cbx->currentIndex ()].second;
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  AlignOptionsDialog implementation

AlignOptionsDialog::AlignOptionsDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("change_layer_options_dialog"));

  Ui::AlignOptionsDialog::setupUi (this);
}

AlignOptionsDialog::~AlignOptionsDialog ()
{
  //  .. nothing yet ..
}

bool 
AlignOptionsDialog::exec_dialog (int &hmode, int &vmode, bool &visible_layers)
{
  QRadioButton *hmode_buttons [] = { this->h_none_rb, this->h_left_rb, this->h_center_rb, this->h_right_rb };
  QRadioButton *vmode_buttons [] = { this->v_none_rb, this->v_top_rb, this->v_center_rb, this->v_bottom_rb };
  QRadioButton *layers_buttons [] = { this->all_layers_rb, this->visible_layers_rb };
  
  for (int i = 0; i < 4; ++i) {
    hmode_buttons [i]->setChecked (hmode == i); 
  }
  for (int i = 0; i < 4; ++i) {
    vmode_buttons [i]->setChecked (vmode == i); 
  }
  for (int i = 0; i < 2; ++i) {
    layers_buttons [i]->setChecked (int (visible_layers) == i); 
  }

  if (QDialog::exec ()) {

    hmode = -1;
    for (int i = 0; i < 4; ++i) {
      if (hmode_buttons [i]->isChecked ()) {
        hmode = i;
      }
    }

    vmode = -1;
    for (int i = 0; i < 4; ++i) {
      if (vmode_buttons [i]->isChecked ()) {
        vmode = i;
      }
    }

    visible_layers = false;
    for (int i = 0; i < 2; ++i) {
      if (layers_buttons [i]->isChecked ()) {
        visible_layers = (i != 0);
      }
    }

    return true;

  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  DistributeOptionsDialog implementation

DistributeOptionsDialog::DistributeOptionsDialog (QWidget *parent)
  : QDialog (parent)
{
  setObjectName (QString::fromUtf8 ("change_layer_options_dialog"));

  Ui::DistributeOptionsDialog::setupUi (this);
}

DistributeOptionsDialog::~DistributeOptionsDialog ()
{
  //  .. nothing yet ..
}

bool
DistributeOptionsDialog::exec_dialog (bool &hdistribute, int &hmode, double &hpitch, double &hspace, bool &vdistribute, int &vmode, double &vpitch, double &vspace, bool &visible_layers)
{
  QRadioButton *hmode_buttons [] = { this->h_none_rb, this->h_left_rb, this->h_center_rb, this->h_right_rb };
  QRadioButton *vmode_buttons [] = { this->v_none_rb, this->v_top_rb, this->v_center_rb, this->v_bottom_rb };
  QRadioButton *layers_buttons [] = { this->all_layers_rb, this->visible_layers_rb };

  this->h_distribute->setChecked (hdistribute);
  for (int i = 1; i < 4; ++i) {
    hmode_buttons [i]->setChecked (hmode == i);
  }

  this->h_space->setText (tl::to_qstring (tl::micron_to_string (hspace)));
  this->h_pitch->setText (tl::to_qstring (tl::micron_to_string (hpitch)));

  this->v_distribute->setChecked (vdistribute);
  for (int i = 1; i < 4; ++i) {
    vmode_buttons [i]->setChecked (vmode == i);
  }

  this->v_space->setText (tl::to_qstring (tl::micron_to_string (vspace)));
  this->v_pitch->setText (tl::to_qstring (tl::micron_to_string (vpitch)));

  for (int i = 0; i < 2; ++i) {
    layers_buttons [i]->setChecked (int (visible_layers) == i);
  }

  if (QDialog::exec ()) {

    hdistribute = this->h_distribute->isChecked ();
    hmode = -1;
    for (int i = 1; i < 4; ++i) {
      if (hmode_buttons [i]->isChecked ()) {
        hmode = i;
      }
    }

    hspace = 0.0;
    tl::from_string_ext (tl::to_string (this->h_space->text ()), hspace);

    hpitch = 0.0;
    tl::from_string_ext (tl::to_string (this->h_pitch->text ()), hpitch);

    vdistribute = this->v_distribute->isChecked ();
    vmode = -1;
    for (int i = 1; i < 4; ++i) {
      if (vmode_buttons [i]->isChecked ()) {
        vmode = i;
      }
    }

    vspace = 0.0;
    tl::from_string_ext (tl::to_string (this->v_space->text ()), vspace);

    vpitch = 0.0;
    tl::from_string_ext (tl::to_string (this->v_pitch->text ()), vpitch);

    visible_layers = false;
    for (int i = 0; i < 2; ++i) {
      if (layers_buttons [i]->isChecked ()) {
        visible_layers = (i != 0);
      }
    }

    return true;

  } else {
    return false;
  }
}

// --------------------------------------------------------------------------------
//  MakeCellOptionsDialog implementation

MakeCellOptionsDialog::MakeCellOptionsDialog (QWidget *parent)
  : QDialog (parent)
{ 
  setupUi (this);

  setObjectName (QString::fromUtf8 ("make_cell_options_dialog"));

  QToolButton *buttons[3][3] = { { lb, cb, rb }, { lc, cc, rc }, { lt, ct, rt } };

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      connect (buttons[i][j], SIGNAL (clicked ()), this, SLOT (button_clicked ()));
    }
  }
}

bool 
MakeCellOptionsDialog::exec_dialog (const db::Layout &layout, std::string &name, int &mode_x, int &mode_y)
{
  do {
BEGIN_PROTECTED

    QToolButton *buttons[3][3] = { { lb, cb, rb }, { lc, cc, rc }, { lt, ct, rt } };

    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        buttons[i][j]->setChecked (j - 1 == mode_x && i - 1 == mode_y);
      }
    }

    origin_groupbox->setChecked (mode_x >= -1);

    if (QDialog::exec ()) {

      if (origin_groupbox->isChecked ()) {
        for (int i = 0; i < 3; ++i) {
          for (int j = 0; j < 3; ++j) {
            if (buttons[i][j]->isChecked ()) {
              mode_x = j - 1;
              mode_y = i - 1;
            }
          }
        }
      } else {
        mode_x = mode_y = -2;
      }

      name = tl::to_string (cell_name_le->text ());
      if (name.empty ()) {
        throw tl::Exception (tl::to_string (QObject::tr ("Cell name must not be empty")));
      } else if (layout.cell_by_name (name.c_str ()).first) {
        throw tl::Exception (tl::to_string (QObject::tr ("A cell with that name already exists: ")) + name);
      }

      return true;

    } else {
      return false;
    }

END_PROTECTED
  } while (true);
}

void
MakeCellOptionsDialog::button_clicked ()
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
//  RoundCornerOptionsDialog implementation

RoundCornerOptionsDialog::RoundCornerOptionsDialog (QWidget *parent)
  : QDialog (parent), mp_layout (0), m_router_extracted (0.0), m_rinner_extracted (0.0), m_npoints_extracted (64), m_has_extracted (false)
{
  setObjectName (QString::fromUtf8 ("round_corners_options_dialog"));

  Ui::RoundCornerOptionsDialog::setupUi (this);

  connect (amend_cb, SIGNAL (stateChanged (int)), this, SLOT (amend_changed ()));
}

RoundCornerOptionsDialog::~RoundCornerOptionsDialog ()
{
  //  .. nothing yet ..
}

void
RoundCornerOptionsDialog::amend_changed ()
{
  if (amend_cb->isChecked () && m_has_extracted) {
    router_le->setText (tl::to_qstring (tl::to_string (m_router_extracted)));
    if (db::coord_traits<double>::equal (m_router_extracted, m_rinner_extracted)) {
      rinner_le->setText (QString ());
    } else {
      rinner_le->setText (tl::to_qstring (tl::to_string (m_rinner_extracted)));
    }
    points_le->setText (tl::to_qstring (tl::to_string (m_npoints_extracted)));
  }
}

bool
RoundCornerOptionsDialog::exec_dialog (const db::Layout &layout, double &router, double &rinner, unsigned int &npoints, bool &undo_before_apply, double router_extracted, double rinner_extracted, unsigned int npoints_extracted, bool has_extracted)
{
  m_router_extracted = router_extracted;
  m_rinner_extracted = rinner_extracted;
  m_npoints_extracted = npoints_extracted;
  m_has_extracted = has_extracted;

  amend_cb->blockSignals (true);
  amend_cb->setEnabled (has_extracted);
  amend_cb->setChecked (undo_before_apply && has_extracted);
  amend_cb->blockSignals (false);

  mp_layout = &layout;

  double ro = undo_before_apply && has_extracted ? router_extracted : router;
  double ri = undo_before_apply && has_extracted ? rinner_extracted : rinner;
  unsigned int n = undo_before_apply && has_extracted ? npoints_extracted : npoints;

  router_le->setText (tl::to_qstring (tl::to_string (ro)));
  if (db::coord_traits<double>::equal (ro, ri)) {
    rinner_le->setText (QString ());
  } else {
    rinner_le->setText (tl::to_qstring (tl::to_string (ri)));
  }
  points_le->setText (tl::to_qstring (tl::to_string (n)));

  if (QDialog::exec ()) {

    undo_before_apply = m_has_extracted && amend_cb->isChecked ();

    tl::from_string_ext (tl::to_string (router_le->text ()), router);
    if (rinner_le->text ().isEmpty ()) {
      rinner = router;
    } else {
      tl::from_string_ext (tl::to_string (rinner_le->text ()), rinner);
    }
    tl::from_string_ext (tl::to_string (points_le->text ()), npoints);

    mp_layout = 0;
    return true;

  } else {
    mp_layout = 0;
    return false;
  }
}

void 
RoundCornerOptionsDialog::accept ()
{
BEGIN_PROTECTED;

  double rhull = 0.0, rholes = 0.0;
  unsigned int npoints = 0;
  
  tl::from_string_ext (tl::to_string (router_le->text ()), rhull);
  if (rinner_le->text ().isEmpty ()) {
    rholes = rhull;
  } else {
    tl::from_string_ext (tl::to_string (rinner_le->text ()), rholes);
  }
  tl::from_string_ext (tl::to_string (points_le->text ()), npoints);

  const unsigned int min_points = 16;
  const double seg_thr = 10.0; // in DBU

  if (npoints < min_points) {
    throw tl::Exception (tl::to_string (QObject::tr ("Number of points is too small (must be %d at least)")), min_points);
  }

  double dbu = mp_layout->dbu ();

  if ((rholes > 0.0 && dbu * seg_thr > (rholes * M_PI * 2.0) / double (npoints)) ||
      (rhull > 0.0 && dbu * seg_thr > (rhull * M_PI * 2.0) / double (npoints))) {
    throw tl::Exception (tl::to_string (QObject::tr ("Number of points is too large (one segment must be larger than %g database units)")), seg_thr);
  }

  if (fabs (rholes - 2.0 * dbu * floor (rholes * 0.5 / dbu + 0.5)) > 1e-6 ||
      fabs (rhull - 2.0 * dbu * floor (rhull * 0.5 / dbu + 0.5)) > 1e-6) {
    throw tl::Exception (tl::to_string (QObject::tr ("Radius must be a even multiple of the database unit")));
  }

  QDialog::accept ();

END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  MakeArrayOptionsDialog implementation

MakeArrayOptionsDialog::MakeArrayOptionsDialog (QWidget *parent)
  : QDialog (parent)
{ 
  setupUi (this);
}

bool 
MakeArrayOptionsDialog::exec_dialog (db::DVector &a, unsigned int &na, db::DVector &b, unsigned int &nb)
{
  rows_le->setText (tl::to_qstring (tl::to_string (na)));
  columns_le->setText (tl::to_qstring (tl::to_string (nb)));
  row_x_le->setText (tl::to_qstring (tl::micron_to_string (a.x ())));
  row_y_le->setText (tl::to_qstring (tl::micron_to_string (a.y ())));
  column_x_le->setText (tl::to_qstring (tl::micron_to_string (b.x ())));
  column_y_le->setText (tl::to_qstring (tl::micron_to_string (b.y ())));

  if (QDialog::exec ()) {

    double bx = 0.0, by = 0.0;
    double ax = 0.0, ay = 0.0;

    tl::from_string_ext (tl::to_string (column_x_le->text ()), bx);
    tl::from_string_ext (tl::to_string (column_y_le->text ()), by);
    tl::from_string_ext (tl::to_string (columns_le->text ()), nb);
    tl::from_string_ext (tl::to_string (row_x_le->text ()), ax);
    tl::from_string_ext (tl::to_string (row_y_le->text ()), ay);
    tl::from_string_ext (tl::to_string (rows_le->text ()), na);

    a = db::DVector (ax, ay);
    b = db::DVector (bx, by);

    return true;

  } else {
    return false;
  }

}

void 
MakeArrayOptionsDialog::accept ()
{
BEGIN_PROTECTED;

  double bx = 0.0, by = 0.0;
  double ax = 0.0, ay = 0.0;
  int na, nb;

  tl::from_string_ext (tl::to_string (column_x_le->text ()), bx);
  tl::from_string_ext (tl::to_string (column_y_le->text ()), by);
  tl::from_string_ext (tl::to_string (columns_le->text ()), nb);
  tl::from_string_ext (tl::to_string (row_x_le->text ()), ax);
  tl::from_string_ext (tl::to_string (row_y_le->text ()), ay);
  tl::from_string_ext (tl::to_string (rows_le->text ()), na);

  if (na < 1 || nb < 1) {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid row or column count (must be larger or equal one)")));
  }

  QDialog::accept ();

END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  AreaAndPerimeterDialog implementation

AreaAndPerimeterDialog::AreaAndPerimeterDialog (QWidget *parent)
  : QDialog (parent)
{
  setupUi (this);
}

AreaAndPerimeterDialog::~AreaAndPerimeterDialog ()
{
  //  .. nothing yet ..
}

bool
AreaAndPerimeterDialog::exec_dialog (double area, double perimeter)
{
  area_le->setText (tl::to_qstring (tl::sprintf ("%.12g", area)));
  perimeter_le->setText (tl::to_qstring (tl::sprintf ("%.12g", perimeter)));

  return exec () != 0;
}

}

#endif


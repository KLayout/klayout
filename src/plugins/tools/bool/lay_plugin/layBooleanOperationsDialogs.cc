
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


#include "layBooleanOperationsDialogs.h"

#include "layLayoutView.h"
#include "tlExceptions.h"

namespace lay
{

// --------------------------------------------------------------------------------
//  BooleanOptionsDialog implementation

BooleanOptionsDialog::BooleanOptionsDialog (QWidget *parent)
  : QDialog (parent),
    mp_view (0)
{
  setObjectName (QString::fromUtf8 ("boolean_options_dialog"));
  Ui::BooleanOptionsDialog::setupUi (this);

  connect (cva_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
  connect (cvb_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
  connect (cvr_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
}

BooleanOptionsDialog::~BooleanOptionsDialog ()
{
  //  .. nothing yet ..
}

void
BooleanOptionsDialog::cv_changed (int)
{
  if (! mp_view) {
    return;
  }

  layera_cbx->set_view (mp_view, cva_cbx->currentIndex ());
  layerb_cbx->set_view (mp_view, cvb_cbx->currentIndex ());
  layerr_cbx->set_view (mp_view, cvr_cbx->currentIndex ());
}

bool 
BooleanOptionsDialog::exec_dialog (lay::LayoutViewBase *view, int &cv_a, int &layer_a, int &cv_b, int &layer_b, int &cv_r, int &layer_r, int &mode, int &hier_mode, bool &min_coherence)
{
  mp_view = view;

  bool res = false;
  cva_cbx->set_layout_view (view);
  cva_cbx->set_current_cv_index (cv_a);
  cvb_cbx->set_layout_view (view);
  cvb_cbx->set_current_cv_index (cv_b);
  cvb_cbx->setEnabled (true);
  cvr_cbx->set_layout_view (view);
  cvr_cbx->set_current_cv_index (cv_r);

  cv_changed (0 /*dummy*/);

  layera_cbx->set_current_layer (layer_a);
  layerb_cbx->set_current_layer (layer_b);
  layerb_cbx->setEnabled (true);
  layerr_cbx->set_current_layer (layer_r);

  hier_mode_cbx->setCurrentIndex (hier_mode);
  mode_cbx->setCurrentIndex (mode);
  min_coherence_cb->setChecked (min_coherence);

  if (QDialog::exec ()) {

    cv_a = cva_cbx->current_cv_index ();
    cv_b = cvb_cbx->current_cv_index ();
    cv_r = cvr_cbx->current_cv_index ();
    layer_a = layera_cbx->current_layer ();
    layer_b = layerb_cbx->current_layer ();
    layer_r = layerr_cbx->current_layer ();

    hier_mode = hier_mode_cbx->currentIndex ();
    mode = mode_cbx->currentIndex ();
    min_coherence = min_coherence_cb->isChecked ();

    res = true;

  }

  mp_view = 0;
  return res;
}

void 
BooleanOptionsDialog::accept ()
{
BEGIN_PROTECTED;

  int cv_a = cva_cbx->current_cv_index ();
  if (cv_a < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout specified for source 'A'")));
  }

  int cv_b = cvb_cbx->current_cv_index ();
  if (cv_b < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout specified for source 'B'")));
  }

  int cv_r = cvr_cbx->current_cv_index ();
  if (cv_r < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layout specified for result")));
  }

  if (fabs (mp_view->cellview (cv_a)->layout ().dbu () - mp_view->cellview (cv_r)->layout ().dbu ()) > db::epsilon ||
      fabs (mp_view->cellview (cv_b)->layout ().dbu () - mp_view->cellview (cv_r)->layout ().dbu ()) > db::epsilon) {
    throw tl::Exception (tl::to_string (QObject::tr ("All source and result layouts must have the same database unit")));
  }

  if (layera_cbx->current_layer () < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer specified for source 'A'")));
  }
  if (layerb_cbx->current_layer () < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer specified for source 'B'")));
  }
  if (layerr_cbx->current_layer () < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer specified for result layer")));
  }

  if (hier_mode_cbx->currentIndex () == 2 && 
      cva_cbx->current_cv_index () != cvb_cbx->current_cv_index () &&
      cva_cbx->current_cv_index () != cvr_cbx->current_cv_index ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("All source layouts and result layout must be same in 'cell by cell' mode")));
  }

  QDialog::accept ();
END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  SizingOptionsDialog implementation

SizingOptionsDialog::SizingOptionsDialog (QWidget *parent)
  : QDialog (parent),
    mp_view (0)
{
  setObjectName (QString::fromUtf8 ("sizing_options_dialog"));
  Ui::SizingOptionsDialog::setupUi (this);

  connect (cv_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
  connect (cvr_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
}

SizingOptionsDialog::~SizingOptionsDialog ()
{
  //  .. nothing yet ..
}

void
SizingOptionsDialog::cv_changed (int)
{
  if (! mp_view) {
    return;
  }

  layer_cbx->set_view (mp_view, cv_cbx->currentIndex ());
  layerr_cbx->set_view (mp_view, cvr_cbx->currentIndex ());
}

bool 
SizingOptionsDialog::exec_dialog (lay::LayoutViewBase *view, int &cv, int &layer, int &cv_r, int &layer_r, double &dx, double &dy, unsigned int &size_mode, int &hier_mode, bool &min_coherence)
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
  cutoff_cbx->setCurrentIndex (size_mode);
  if (dx == dy) {
    value_le->setText (tl::to_qstring (tl::sprintf ("%.12g", dx)));
  } else {
    value_le->setText (tl::to_qstring (tl::sprintf ("%.12g,%.12g", dx, dy)));
  }
  min_coherence_cb->setChecked (min_coherence);

  if (QDialog::exec ()) {

    cv = cv_cbx->current_cv_index ();
    cv_r = cvr_cbx->current_cv_index ();
    layer = layer_cbx->current_layer ();
    layer_r = layerr_cbx->current_layer ();

    hier_mode = hier_mode_cbx->currentIndex ();
    min_coherence = min_coherence_cb->isChecked ();
    size_mode = cutoff_cbx->currentIndex ();

    tl::string t (tl::to_string (value_le->text ()));
    tl::Extractor ex (t.c_str ());
    ex.read (dx);
    if (ex.test (",")) {
      ex.read (dy);
    } else {
      dy = dx;
    }

    res = true;

  }

  mp_view = 0;
  return res;
}

void 
SizingOptionsDialog::accept ()
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
    throw tl::Exception (tl::to_string (QObject::tr ("No layer specified for result layer")));
  }

  if (hier_mode_cbx->currentIndex () == 2 && 
      cv_cbx->current_cv_index () != cvr_cbx->current_cv_index ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Source layout and result layout must be same in 'cell by cell' mode")));
  }

  double x, y;
  tl::string t (tl::to_string (value_le->text ()));
  tl::Extractor ex (t.c_str ());
  ex.read (x);
  if (ex.test (",")) {
    ex.read (y);
  } 

  QDialog::accept ();
END_PROTECTED;
}

// --------------------------------------------------------------------------------
//  MergeOptionsDialog implementation

MergeOptionsDialog::MergeOptionsDialog (QWidget *parent)
  : QDialog (parent),
    mp_view (0)
{
  setObjectName (QString::fromUtf8 ("merge_options_dialog"));
  Ui::MergeOptionsDialog::setupUi (this);

  connect (cv_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
  connect (cvr_cbx, SIGNAL (activated (int)), this, SLOT (cv_changed (int)));
}

MergeOptionsDialog::~MergeOptionsDialog ()
{
  //  .. nothing yet ..
}

void
MergeOptionsDialog::cv_changed (int)
{
  if (! mp_view) {
    return;
  }

  layer_cbx->set_view (mp_view, cv_cbx->currentIndex ());
  layerr_cbx->set_view (mp_view, cvr_cbx->currentIndex ());
}

bool 
MergeOptionsDialog::exec_dialog (lay::LayoutViewBase *view, int &cv, int &layer, int &cv_r, int &layer_r, unsigned int &min_wc, int &hier_mode, bool &min_coherence)
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
  threshold_le->setText (tl::to_qstring (tl::sprintf ("%u", min_wc)));
  min_coherence_cb->setChecked (min_coherence);

  if (QDialog::exec ()) {

    cv = cv_cbx->current_cv_index ();
    cv_r = cvr_cbx->current_cv_index ();
    layer = layer_cbx->current_layer ();
    layer_r = layerr_cbx->current_layer ();

    hier_mode = hier_mode_cbx->currentIndex ();
    min_coherence = min_coherence_cb->isChecked ();

    std::string t (tl::to_string (threshold_le->text ()));
    tl::Extractor ex (t.c_str ());
    ex.read (min_wc);

    res = true;

  }

  mp_view = 0;
  return res;
}

void 
MergeOptionsDialog::accept ()
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

  unsigned int min_wc = 0;
  std::string t (tl::to_string (threshold_le->text ()));
  tl::Extractor ex (t.c_str ());
  ex.read (min_wc);

  QDialog::accept ();
END_PROTECTED;
}

}


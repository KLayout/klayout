
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#include "layD25View.h"
#include "layLayoutView.h"
#include "layQtTools.h"

#include "ui_D25View.h"

#include <stdio.h>

namespace lay
{

const double initial_elevation = 15.0;

D25View::D25View (QWidget *parent)
  : QDialog (parent)
{
  mp_ui = new Ui::D25View ();
  mp_ui->setupUi (this);

  mp_ui->d25_view->setFocusPolicy (Qt::StrongFocus);
  mp_ui->d25_view->setFocus ();

  connect (mp_ui->fit_back, SIGNAL (clicked ()), this, SLOT (fit_button_clicked ()));
  connect (mp_ui->fit_front, SIGNAL (clicked ()), this, SLOT (fit_button_clicked ()));
  connect (mp_ui->fit_left, SIGNAL (clicked ()), this, SLOT (fit_button_clicked ()));
  connect (mp_ui->fit_right, SIGNAL (clicked ()), this, SLOT (fit_button_clicked ()));
  connect (mp_ui->fit_top, SIGNAL (clicked ()), this, SLOT (fit_button_clicked ()));
  connect (mp_ui->fit_bottom, SIGNAL (clicked ()), this, SLOT (fit_button_clicked ()));
  connect (mp_ui->zoom_slider, SIGNAL (valueChanged (int)), this, SLOT (scale_slider_changed (int)));
  connect (mp_ui->d25_view, SIGNAL (scale_factor_changed (double)), this, SLOT (scale_factor_changed (double)));
  connect (mp_ui->d25_view, SIGNAL (init_failed ()), this, SLOT (init_failed ()));

  mp_ui->gl_stack->setCurrentIndex (0);

  lay::activate_help_links (mp_ui->doc_label);
}

D25View::~D25View ()
{
  delete mp_ui;
  mp_ui = 0;
}

static QString scale_factor_to_string (double f)
{
  QString s;
  s.sprintf ("x %.3g", f);
  return s;
}

void
D25View::init_failed ()
{
  mp_ui->error_text->setPlainText (tl::to_qstring (mp_ui->d25_view->error ()));
  mp_ui->gl_stack->setCurrentIndex (1);
}

void
D25View::scale_slider_changed (int value)
{
  double f = exp (log (10.0) * -0.01 * value);
  mp_ui->zoom_factor->setText (scale_factor_to_string (f));
  mp_ui->d25_view->set_scale_factor (f);
}

void
D25View::scale_factor_changed (double f)
{
  mp_ui->zoom_factor->setText (scale_factor_to_string (f));
  int v = floor (0.5 - log10 (f) * 100.0);
  mp_ui->zoom_slider->blockSignals (true);
  mp_ui->zoom_slider->setValue (v);
  mp_ui->zoom_slider->blockSignals (false);
}

int 
D25View::exec_dialog (lay::LayoutView *view)
{
  mp_view.reset (view);
  bool any = mp_ui->d25_view->attach_view (view);

  if (! any) {

    mp_view.reset (0);
    mp_ui->d25_view->attach_view (0);

    throw tl::Exception (tl::to_string (tr ("No z data configured for the layers in the view.\nUse \"Tools/Manage Technologies\" to set up a z stack.")));

  }

  mp_ui->d25_view->reset ();
  mp_ui->d25_view->set_cam_azimuth (0.0);
  mp_ui->d25_view->set_cam_elevation (-initial_elevation);
  mp_ui->d25_view->fit ();

  int ret = QDialog::exec ();

  mp_ui->d25_view->attach_view (0);
  mp_view.reset (0);

  return ret;
}

void
D25View::fit_button_clicked ()
{
  double azimuth = mp_ui->d25_view->cam_azimuth ();
  double elevation = mp_ui->d25_view->cam_elevation ();

  if (sender () == mp_ui->fit_back) {
    azimuth = -180.0;
    elevation = -initial_elevation;
  } else if (sender () == mp_ui->fit_front) {
    azimuth = 0.0;
    elevation = -initial_elevation;
  } else if (sender () == mp_ui->fit_left) {
    azimuth = 90.0;
    elevation = -initial_elevation;
  } else if (sender () == mp_ui->fit_right) {
    azimuth = -90.0;
    elevation = -initial_elevation;
  } else if (sender () == mp_ui->fit_top) {
    azimuth = 0;
    elevation = -90;
  } else if (sender () == mp_ui->fit_bottom) {
    azimuth = 0;
    elevation = 90;
  }

  mp_ui->d25_view->set_cam_azimuth (azimuth);
  mp_ui->d25_view->set_cam_elevation (elevation);

  mp_ui->d25_view->fit ();
}

void 
D25View::accept ()
{
  QDialog::accept ();
}

}


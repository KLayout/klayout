
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

D25View::D25View (lay::Dispatcher *root, lay::LayoutView *view)
  : lay::Browser (root, view, "d25_view")
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
  connect (mp_ui->vzoom_slider, SIGNAL (valueChanged (int)), this, SLOT (vscale_slider_changed (int)));
  connect (mp_ui->zoom_factor, SIGNAL (editingFinished ()), this, SLOT (scale_value_edited ()));
  connect (mp_ui->vzoom_factor, SIGNAL (editingFinished ()), this, SLOT (vscale_value_edited ()));
  connect (mp_ui->d25_view, SIGNAL (scale_factor_changed (double)), this, SLOT (scale_factor_changed (double)));
  connect (mp_ui->d25_view, SIGNAL (vscale_factor_changed (double)), this, SLOT (vscale_factor_changed (double)));
  connect (mp_ui->d25_view, SIGNAL (init_failed ()), this, SLOT (init_failed ()));

  mp_ui->gl_stack->setCurrentIndex (0);

  lay::activate_help_links (mp_ui->doc_label);

  view->cellviews_changed_event.add (this, &D25View::cellviews_changed);
  view->layer_list_changed_event.add (this, &D25View::layer_properties_changed);
}

D25View::~D25View ()
{
  delete mp_ui;
  mp_ui = 0;

  if (view ()) {
    view ()->cellviews_changed_event.remove (this, &D25View::cellviews_changed);
  }
}

void
D25View::cellviews_changed ()
{
  deactivate ();
}

void
D25View::layer_properties_changed (int)
{
  mp_ui->d25_view->refresh_view ();
}

void
D25View::menu_activated (const std::string &symbol)
{
  if (symbol == "lay::d25_view") {

    const lay::CellView &cv = view ()->cellview (view ()->active_cellview_index ());
    if (cv.is_valid ()) {

      show ();
      activateWindow ();
      raise ();

      try {
        activate ();
      } catch (...) {
        deactivate ();
        throw;
      }

    }

  } else {
    lay::Browser::menu_activated (symbol);
  }
}

static QString scale_factor_to_string (double f)
{
  return QString (QString::fromUtf8 ("%1")).arg (f, 0, 'g', 3);
}

void
D25View::init_failed ()
{
  mp_ui->error_text->setPlainText (tl::to_qstring (mp_ui->d25_view->error ()));
  mp_ui->gl_stack->setCurrentIndex (1);
}

void
D25View::scale_value_edited ()
{
  double f = mp_ui->d25_view->scale_factor ();
  try {
    tl::from_string_ext (tl::to_string (mp_ui->zoom_factor->text ()), f);
    f = std::min (1e6, std::max (1e-6, f));
  } catch (...) {
    //  ignore exceptions
  }
  mp_ui->d25_view->set_scale_factor (f);
  scale_factor_changed (f);
}

void
D25View::vscale_value_edited ()
{
  double f = mp_ui->d25_view->vscale_factor ();
  try {
    tl::from_string_ext (tl::to_string (mp_ui->vzoom_factor->text ()), f);
    f = std::min (1e6, std::max (1e-6, f));
  } catch (...) {
    //  ignore exceptions
  }
  mp_ui->d25_view->set_vscale_factor (f);
  vscale_factor_changed (f);
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

void
D25View::vscale_slider_changed (int value)
{
  double f = exp (log (10.0) * -0.01 * value);
  mp_ui->vzoom_factor->setText (scale_factor_to_string (f));
  mp_ui->d25_view->set_vscale_factor (f);
}

void
D25View::vscale_factor_changed (double f)
{
  mp_ui->vzoom_factor->setText (scale_factor_to_string (f));
  int v = floor (0.5 - log10 (f) * 100.0);
  mp_ui->vzoom_slider->blockSignals (true);
  mp_ui->vzoom_slider->setValue (v);
  mp_ui->vzoom_slider->blockSignals (false);
}

void
D25View::deactivated ()
{
  mp_ui->d25_view->attach_view (0);
}

void
D25View::activated ()
{
  bool any = mp_ui->d25_view->attach_view (view ());
  if (! any) {
    mp_ui->d25_view->attach_view (0);
    throw tl::Exception (tl::to_string (tr ("No z data configured for the layers in this view.\nUse \"Tools/Manage Technologies\" to set up a z stack or check if it applies to the layers here.")));
  }

  mp_ui->d25_view->reset ();
  mp_ui->d25_view->set_cam_azimuth (0.0);
  mp_ui->d25_view->set_cam_elevation (-initial_elevation);
  mp_ui->d25_view->fit ();
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

void
D25View::reject ()
{
  QDialog::reject ();
}

}


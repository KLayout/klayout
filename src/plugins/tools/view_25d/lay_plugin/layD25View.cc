
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

#include "tlExceptions.h"
#include "tlRecipe.h"

#include "layD25View.h"
#include "layLayoutView.h"
#include "layConverters.h"
#include "laybasicConfig.h"
#include "layQtTools.h"

#include "ui_D25View.h"

#include <stdio.h>

#include <QFontMetrics>

namespace lay
{

const double initial_elevation = 15.0;

D25View::D25View (Dispatcher *root, LayoutViewBase *view)
  : lay::Browser (root, view, "d25_view"),
    dm_rerun_macro (this, &D25View::rerun_macro),
    dm_fit (this, &D25View::fit),
    m_visibility_follows_selection (false)
{
  mp_ui = new Ui::D25View ();
  mp_ui->setupUi (this);

  mp_ui->d25_view->setFocusPolicy (Qt::StrongFocus);
  mp_ui->d25_view->setFocus ();

  connect (mp_ui->fit_back, SIGNAL (clicked()), this, SLOT (fit_button_clicked()));
  connect (mp_ui->fit_front, SIGNAL (clicked()), this, SLOT (fit_button_clicked()));
  connect (mp_ui->fit_left, SIGNAL (clicked()), this, SLOT (fit_button_clicked()));
  connect (mp_ui->fit_right, SIGNAL (clicked()), this, SLOT (fit_button_clicked()));
  connect (mp_ui->fit_top, SIGNAL (clicked()), this, SLOT (fit_button_clicked()));
  connect (mp_ui->fit_bottom, SIGNAL (clicked()), this, SLOT (fit_button_clicked()));
  connect (mp_ui->zoom_slider, SIGNAL (valueChanged(int)), this, SLOT (scale_slider_changed(int)));
  connect (mp_ui->vzoom_slider, SIGNAL (valueChanged(int)), this, SLOT (vscale_slider_changed(int)));
  connect (mp_ui->zoom_factor, SIGNAL (editingFinished()), this, SLOT (scale_value_edited()));
  connect (mp_ui->vzoom_factor, SIGNAL (editingFinished()), this, SLOT (vscale_value_edited()));
  connect (mp_ui->d25_view, SIGNAL (scale_factor_changed(double)), this, SLOT (scale_factor_changed(double)));
  connect (mp_ui->d25_view, SIGNAL (vscale_factor_changed(double)), this, SLOT (vscale_factor_changed(double)));
  connect (mp_ui->d25_view, SIGNAL (init_failed()), this, SLOT (init_failed()));
  connect (mp_ui->rerun_button, SIGNAL (clicked()), this, SLOT (rerun_button_pressed()));
  connect (mp_ui->hide_all_action, SIGNAL (triggered()), this, SLOT (hide_all_triggered()));
  connect (mp_ui->hide_selected_action, SIGNAL (triggered()), this, SLOT (hide_selected_triggered()));
  connect (mp_ui->show_all_action, SIGNAL (triggered()), this, SLOT (show_all_triggered()));
  connect (mp_ui->show_selected_action, SIGNAL (triggered()), this, SLOT (show_selected_triggered()));
  connect (mp_ui->visibility_follows_selection_action, SIGNAL (toggled(bool)), this, SLOT (visibility_follows_selection_changed(bool)));
  connect (mp_ui->material_list, SIGNAL (itemSelectionChanged()), this, SLOT (update_visibility()));

  mp_ui->gl_stack->setCurrentIndex (2);
  mp_ui->rerun_button->setEnabled (false);

  lay::activate_help_links (mp_ui->doc_label);
  lay::activate_help_links (mp_ui->empty_label);

  view->cellviews_changed_event.add (this, &D25View::cellviews_changed);
  view->layer_list_changed_event.add (this, &D25View::layer_properties_changed);

  QFont font = mp_ui->material_list->font ();
  font.setWeight (QFont::Bold);
  mp_ui->material_list->setFont (font);

  mp_ui->material_list->addAction (mp_ui->select_all_action);
  mp_ui->material_list->addAction (mp_ui->unselect_all_action);
  QAction *sep = new QAction (this);
  sep->setSeparator (true);
  mp_ui->material_list->addAction (sep);
  mp_ui->material_list->addAction (mp_ui->visibility_follows_selection_action);
  mp_ui->material_list->addAction (mp_ui->hide_all_action);
  mp_ui->material_list->addAction (mp_ui->hide_selected_action);
  mp_ui->material_list->addAction (mp_ui->show_all_action);
  mp_ui->material_list->addAction (mp_ui->show_selected_action);
  mp_ui->material_list->setContextMenuPolicy (Qt::ActionsContextMenu);

  connect (mp_ui->material_list, SIGNAL (itemChanged(QListWidgetItem *)), this, SLOT (material_item_changed(QListWidgetItem *)));
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
  //  .. nothing yet ..
}

bool D25View::configure(const std::string &name, const std::string &value)
{
  if (name == lay::cfg_background_color) {

    lay::ColorConverter lc;

    tl::Color bg;
    lc.from_string (value, bg);

    if (! bg.is_valid ()) {
      bg = view () ? view ()->background_color () : Qt::white;
    }

    QPalette palette = mp_ui->material_list->palette ();
    palette.setColor (QPalette::Base, bg.to_qc ());
    palette.setColor (QPalette::Text, bg.to_mono () ? Qt::black : Qt::white);
    mp_ui->material_list->setPalette (palette);

    mp_ui->d25_view->update ();

  }

  return lay::Browser::configure (name, value);
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

D25View *
D25View::open (lay::LayoutViewBase *view)
{
  D25View *d25_view = view->get_plugin<lay::D25View> ();
  if (d25_view) {

    d25_view->show ();
    d25_view->activateWindow ();
    d25_view->raise ();

    try {
      d25_view->activate ();
    } catch (...) {
      d25_view->deactivate ();
      throw;
    }

  }

  return d25_view;
}

void
D25View::close ()
{
  hide ();
}

void
D25View::clear ()
{
  if (! mp_ui->d25_view->has_error ()) {
    mp_ui->gl_stack->setCurrentIndex (2);
    mp_ui->d25_view->clear ();
  }

  mp_ui->rerun_button->setEnabled (false);
  m_generator.clear ();
}

void
D25View::begin (const std::string &generator)
{
  clear ();

  if (! mp_ui->d25_view->has_error ()) {
    m_generator = generator;
  }
}

void
D25View::open_display (const tl::color_t *frame_color, const tl::color_t *fill_color, const db::LayerProperties *like, const std::string *name)
{
  if (! mp_ui->d25_view->has_error ()) {
    mp_ui->d25_view->open_display (frame_color, fill_color, like, name);
  }
}

void
D25View::close_display ()
{
  if (! mp_ui->d25_view->has_error ()) {
    mp_ui->d25_view->close_display ();
  }
}

void
D25View::entry (const db::Region &data, double dbu, double zstart, double zstop)
{
  if (! mp_ui->d25_view->has_error ()) {
    mp_ui->d25_view->entry (data, dbu, zstart, zstop);
  }
}

void
D25View::entry_edge (const db::Edges &data, double dbu, double zstart, double zstop)
{
  if (! mp_ui->d25_view->has_error ()) {
    mp_ui->d25_view->entry (data, dbu, zstart, zstop);
  }
}

void
D25View::entry_edge_pair (const db::EdgePairs &data, double dbu, double zstart, double zstop)
{
  if (! mp_ui->d25_view->has_error ()) {
    mp_ui->d25_view->entry (data, dbu, zstart, zstop);
  }
}

static void layer_info_to_item (const lay::D25ViewWidget::LayerInfo &info, QListWidgetItem *item, size_t index, QSize icon_size)
{
  if (info.has_name) {
    item->setText (tl::to_qstring (info.name));
  } else {
    item->setText (tl::to_qstring ("#" + tl::to_string (index + 1)));
  }

  QImage img (icon_size, QImage::Format_ARGB32);
  img.fill (QColor (floor (info.fill_color [0] * 255 + 0.5), floor (info.fill_color [1] * 255 + 0.5), floor (info.fill_color [2] * 255 + 0.5), floor (info.fill_color [3] * 255 + 0.5)));

  QColor fc (floor (info.frame_color [0] * 255 + 0.5), floor (info.frame_color [1] * 255 + 0.5), floor (info.frame_color [2] * 255 + 0.5), floor (info.frame_color [3] * 255 + 0.5));
  if (fc.alpha () > 0) {
    QRgb fc_rgb = fc.rgba ();
    for (int x = 0; x < icon_size.width (); ++x) {
      img.setPixel (x, 0, fc_rgb);
      img.setPixel (x, icon_size.height () - 1, fc_rgb);
    }
    for (int y = 0; y < icon_size.height (); ++y) {
      img.setPixel (0, y, fc_rgb);
      img.setPixel (icon_size.width () - 1, y, fc_rgb);
    }
  }

  QIcon icon;
  icon.addPixmap (QPixmap::fromImage (img));
  item->setIcon (icon);
}

void
D25View::finish ()
{
  if (! mp_ui->d25_view->has_error ()) {

    mp_ui->d25_view->finish ();

    QFontMetrics fm (mp_ui->material_list->font ());
    QSize icon_size = fm.size (Qt::TextSingleLine, "WW");
    icon_size.setHeight (icon_size.height () - 2);
    mp_ui->material_list->setIconSize (icon_size);

    mp_ui->material_list->clear ();
    const std::vector<lay::D25ViewWidget::LayerInfo> &layers = mp_ui->d25_view->layers ();
    for (auto l = layers.begin (); l != layers.end (); ++l) {
      QListWidgetItem *item = new QListWidgetItem (mp_ui->material_list);
      item->setFlags (item->flags () | Qt::ItemIsUserCheckable);
      item->setCheckState (Qt::Checked);
      layer_info_to_item (*l, item, l - layers.begin (), icon_size);
    }

    mp_ui->d25_view->reset ();
    mp_ui->d25_view->set_cam_azimuth (0.0);
    mp_ui->d25_view->set_cam_elevation (-initial_elevation);
    //  NOTE: needs to be delayed to allow the geometry to be updated before (initial call)
    dm_fit ();

    mp_ui->rerun_button->setEnabled (true);
    mp_ui->gl_stack->setCurrentIndex (0);

  }
}

void
D25View::fit ()
{
  mp_ui->d25_view->fit ();
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
  mp_ui->rerun_button->setEnabled (false);
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
D25View::material_item_changed (QListWidgetItem *item)
{
  int index = mp_ui->material_list->row (item);
  if (index >= 0) {
    mp_ui->d25_view->set_material_visible (size_t (index), item->checkState () == Qt::Checked);
  }
}

void
D25View::deactivated ()
{
  mp_ui->d25_view->attach_view (0);
}

void
D25View::activated ()
{
  mp_ui->d25_view->attach_view (view ());
  mp_ui->d25_view->reset ();
  mp_ui->d25_view->set_cam_azimuth (0.0);
  mp_ui->d25_view->set_cam_elevation (-initial_elevation);
  mp_ui->d25_view->fit ();
}

void
D25View::rerun_button_pressed ()
{
  //  NOTE: we use deferred execution, because otherwise the button won't get repainted properly
  dm_rerun_macro ();
}

void
D25View::rerun_macro ()
{
BEGIN_PROTECTED

  if (! m_generator.empty ()) {
    std::map<std::string, tl::Variant> add_pars;
    tl::Recipe::make (m_generator, add_pars);
  }

END_PROTECTED
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
D25View::hide_all_triggered ()
{
  for (int i = 0; i < mp_ui->material_list->count (); ++i) {
    mp_ui->material_list->item (i)->setCheckState (Qt::Unchecked);
  }
}

void
D25View::hide_selected_triggered ()
{
  for (int i = 0; i < mp_ui->material_list->count (); ++i) {
    if (mp_ui->material_list->item (i)->isSelected ()) {
      mp_ui->material_list->item (i)->setCheckState (Qt::Unchecked);
    }
  }
}

void
D25View::show_all_triggered ()
{
  for (int i = 0; i < mp_ui->material_list->count (); ++i) {
    mp_ui->material_list->item (i)->setCheckState (Qt::Checked);
  }
}

void
D25View::show_selected_triggered ()
{
  for (int i = 0; i < mp_ui->material_list->count (); ++i) {
    if (mp_ui->material_list->item (i)->isSelected ()) {
      mp_ui->material_list->item (i)->setCheckState (Qt::Checked);
    }
  }
}

void
D25View::visibility_follows_selection_changed (bool checked)
{
  m_visibility_follows_selection = checked;
  update_visibility ();
}

void
D25View::update_visibility ()
{
  if (! m_visibility_follows_selection) {
    return;
  }

  for (int i = 0; i < mp_ui->material_list->count (); ++i) {
    QListWidgetItem *item = mp_ui->material_list->item (i);
    item->setCheckState (item->isSelected () ? Qt::Checked : Qt::Unchecked);
  }
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


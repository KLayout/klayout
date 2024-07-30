
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

#include "layLayoutViewConfigPages.h"
#include "laybasicConfig.h"
#include "layConverters.h"
#include "layDispatcher.h"

#include "ui_LayoutViewConfigPage.h"
#include "ui_LayoutViewConfigPage1.h"
#include "ui_LayoutViewConfigPage2a.h"
#include "ui_LayoutViewConfigPage2b.h"
#include "ui_LayoutViewConfigPage2c.h"
#include "ui_LayoutViewConfigPage2d.h"
#include "ui_LayoutViewConfigPage3a.h"
#include "ui_LayoutViewConfigPage3b.h"
#include "ui_LayoutViewConfigPage3c.h"
#include "ui_LayoutViewConfigPage3f.h"
#include "ui_LayoutViewConfigPage4.h"
#include "ui_LayoutViewConfigPage5.h"
#include "ui_LayoutViewConfigPage6.h"
#include "ui_LayoutViewConfigPage6a.h"
#include "ui_LayoutViewConfigPage7.h"
#include "ui_LayoutViewConfigPage8.h"

#include "laySelectStippleForm.h"
#include "laySelectLineStyleForm.h"

#include "layWidgets.h"
#include "layFileDialog.h"
#include "layFixedFont.h"

#include "dbHershey.h"

#include <QColorDialog>
#include <QPixmap>
#include <QPainter>

namespace lay
{

// ------------------------------------------------------------
//  LayoutConfigPage implementation
//  The configuration pages are declared via a "dummy" plugin

LayoutViewConfigPage::LayoutViewConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage::~LayoutViewConfigPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage::setup (lay::Dispatcher *root)
{
  lay::ColorConverter cc;
  QColor color;

  root->config_get (cfg_background_color, color, cc);
  mp_ui->bkgnd_color_pb->set_color (color);
}

void 
LayoutViewConfigPage::commit (lay::Dispatcher *root)
{
  lay::ColorConverter cc;
  root->config_set (cfg_background_color, mp_ui->bkgnd_color_pb->get_color (), cc);
}

// ------------------------------------------------------------
//  LayoutConfigPage1 implementation

LayoutViewConfigPage1::LayoutViewConfigPage1 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage1 ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage1::~LayoutViewConfigPage1 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage1::setup (lay::Dispatcher *root)
{
  lay::ColorConverter cc;

  QColor color;
  int ctx_dimming = 0;
  bool ctx_hollow = false;

  root->config_get (cfg_ctx_color, color, cc);
  mp_ui->ctx_color_pb->set_color (color);

  root->config_get (cfg_ctx_dimming, ctx_dimming);
  mp_ui->ctx_dimming_spinbx->setValue (ctx_dimming);

  root->config_get (cfg_ctx_hollow, ctx_hollow);
  mp_ui->ctx_hollow_cbx->setChecked (ctx_hollow);

  QColor ccolor;
  int cctx_dimming = 0;
  bool cctx_hollow = false;
  bool cctx_enabled = false;

  root->config_get (cfg_child_ctx_color, ccolor, cc);
  mp_ui->cctx_color_pb->set_color (ccolor);

  root->config_get (cfg_child_ctx_dimming, cctx_dimming);
  mp_ui->cctx_dimming_spinbx->setValue (cctx_dimming);

  root->config_get (cfg_child_ctx_hollow, cctx_hollow);
  mp_ui->cctx_hollow_cbx->setChecked (cctx_hollow);

  root->config_get (cfg_child_ctx_enabled, cctx_enabled);
  mp_ui->cctx_grp->setChecked (cctx_enabled);

  double aw = 10.0;
  bool am = false;

  root->config_get (cfg_abstract_mode_width, aw);
  mp_ui->abstract_mode_width_le->setText (tl::to_qstring (tl::to_string (aw)));

  root->config_get (cfg_abstract_mode_enabled, am);
  mp_ui->abstract_mode_grp->setChecked (am);
}

void 
LayoutViewConfigPage1::commit (lay::Dispatcher *root)
{
  lay::ColorConverter cc;

  root->config_set (cfg_ctx_color, mp_ui->ctx_color_pb->get_color (), cc);
  root->config_set (cfg_ctx_dimming, mp_ui->ctx_dimming_spinbx->value ());
  root->config_set (cfg_ctx_hollow, mp_ui->ctx_hollow_cbx->isChecked ());

  root->config_set (cfg_child_ctx_color, mp_ui->cctx_color_pb->get_color (), cc);
  root->config_set (cfg_child_ctx_dimming, mp_ui->cctx_dimming_spinbx->value ());
  root->config_set (cfg_child_ctx_hollow, mp_ui->cctx_hollow_cbx->isChecked ());
  root->config_set (cfg_child_ctx_enabled, mp_ui->cctx_grp->isChecked ());

  root->config_set (cfg_abstract_mode_enabled, mp_ui->abstract_mode_grp->isChecked ());

  double w = 10.0;
  tl::from_string_ext (tl::to_string (mp_ui->abstract_mode_width_le->text ()), w);
  root->config_set (cfg_abstract_mode_width, w);
  if (w <= 0.0) {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid abstract mode border with - must be larger than 0")));
  }
}

// ------------------------------------------------------------
//  LayoutConfigPage2a implementation

LayoutViewConfigPage2a::LayoutViewConfigPage2a (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage2a ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage2a::~LayoutViewConfigPage2a ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage2a::setup (lay::Dispatcher *root)
{
  QColor color;
  root->config_get (cfg_cell_box_color, color, ColorConverter ());
  mp_ui->cell_box_color_pb->set_color (color);

  bool flag = false;
  root->config_get (cfg_cell_box_text_transform, flag);
  mp_ui->cell_xform_text_cbx->setChecked (flag);

  root->config_get (cfg_cell_box_visible, flag);
  mp_ui->cell_group->setChecked (flag);

  int font = 0;
  root->config_get (cfg_cell_box_text_font, font);
  mp_ui->cell_font_cb->clear ();
  if (mp_ui->cell_font_cb->count () == 0) {
    std::vector<std::string> ff = db::Hershey::font_names ();
    for (std::vector<std::string>::const_iterator f = ff.begin (); f != ff.end (); ++f) {
      mp_ui->cell_font_cb->addItem (tl::to_qstring (*f));
    }
  }
  mp_ui->cell_font_cb->setCurrentIndex (font);

  int n = 0;
  root->config_get (cfg_min_inst_label_size, n);
  mp_ui->cell_min_size_for_label_edit->setText (tl::to_qstring (tl::to_string (n)));

  bool gs_visible = true;
  root->config_get (cfg_guiding_shape_visible, gs_visible);
  mp_ui->pcell_gs_group->setChecked (gs_visible);

  int gs_lw = 1;
  root->config_get (cfg_guiding_shape_line_width, gs_lw);
  mp_ui->pcell_gs_lw->setValue (gs_lw);

  QColor gs_color;
  root->config_get (cfg_guiding_shape_color, gs_color, ColorConverter ());
  mp_ui->pcell_gs_color_pb->set_color (gs_color);

  int gs_vs = 6;
  root->config_get (cfg_guiding_shape_vertex_size, gs_vs);
  mp_ui->pcell_gs_vs->setValue (gs_vs);
}

void 
LayoutViewConfigPage2a::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_cell_box_text_transform, mp_ui->cell_xform_text_cbx->isChecked ());
  root->config_set (cfg_cell_box_text_font, mp_ui->cell_font_cb->currentIndex ());
  root->config_set (cfg_cell_box_color, mp_ui->cell_box_color_pb->get_color (), ColorConverter ());
  root->config_set (cfg_cell_box_visible, mp_ui->cell_group->isChecked ());

  root->config_set (cfg_guiding_shape_visible, mp_ui->pcell_gs_group->isChecked ());
  root->config_set (cfg_guiding_shape_line_width, mp_ui->pcell_gs_lw->value ());
  root->config_set (cfg_guiding_shape_color, mp_ui->pcell_gs_color_pb->get_color (), ColorConverter ());
  root->config_set (cfg_guiding_shape_vertex_size, mp_ui->pcell_gs_vs->value ());

  try {
    int n;
    tl::from_string_ext (tl::to_string (mp_ui->cell_min_size_for_label_edit->text ()), n);
    root->config_set (cfg_min_inst_label_size, n);
  } catch (...) { }
}

// ------------------------------------------------------------
//  LayoutConfigPage2b implementation

LayoutViewConfigPage2b::LayoutViewConfigPage2b (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage2b ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage2b::~LayoutViewConfigPage2b ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage2b::setup (lay::Dispatcher *root)
{
  QColor color;
  root->config_get (cfg_text_color, color, ColorConverter ());
  mp_ui->text_color_pb->set_color (color);

  bool flag = false;
  root->config_get (cfg_apply_text_trans, flag);
  mp_ui->text_apply_trans_cbx->setChecked (flag);

  root->config_get (cfg_text_visible, flag);
  mp_ui->text_group->setChecked (flag);

  root->config_get (cfg_show_properties, flag);
  mp_ui->show_properties_cbx->setChecked (flag);

  int font = 0;
  root->config_get (cfg_text_font, font);
  mp_ui->text_font_cb->clear ();
  if (mp_ui->text_font_cb->count () == 0) {
    std::vector<std::string> ff = db::Hershey::font_names ();
    for (std::vector<std::string>::const_iterator f = ff.begin (); f != ff.end (); ++f) {
      mp_ui->text_font_cb->addItem (tl::to_qstring (*f));
    }
  }
  mp_ui->text_font_cb->setCurrentIndex (font);

  double s = 0.0;
  root->config_get (cfg_default_text_size, s);
  mp_ui->text_def_size_edit->setText (tl::to_qstring (tl::to_string (s)));
}

void 
LayoutViewConfigPage2b::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_text_color, mp_ui->text_color_pb->get_color (), ColorConverter ());
  root->config_set (cfg_apply_text_trans, mp_ui->text_apply_trans_cbx->isChecked ());
  root->config_set (cfg_text_visible, mp_ui->text_group->isChecked ());
  root->config_set (cfg_show_properties, mp_ui->show_properties_cbx->isChecked ());
  root->config_set (cfg_text_font, mp_ui->text_font_cb->currentIndex ());

  try {
    double s;
    tl::from_string_ext (tl::to_string (mp_ui->text_def_size_edit->text ()), s);
    root->config_set (cfg_default_text_size, s);
  } catch (...) { }
}

// ------------------------------------------------------------
//  LayoutConfigPage2c implementation

LayoutViewConfigPage2c::LayoutViewConfigPage2c (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage2c ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage2c::~LayoutViewConfigPage2c ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage2c::setup (lay::Dispatcher *root)
{
  QColor color;
  root->config_get (cfg_sel_color, color, lay::ColorConverter ());
  mp_ui->color_pb->set_color (color);

  int lw = 0;
  root->config_get (cfg_sel_line_width, lw);
  mp_ui->lw_spinbx->setValue (lw);

  int vs = 0;
  root->config_get (cfg_sel_vertex_size, vs);
  mp_ui->vs_spinbx->setValue (vs);

  int dp = 0;
  root->config_get (cfg_sel_dither_pattern, dp);
  mp_ui->stipple_pb->set_dither_pattern (dp);

  bool halo = 0;
  root->config_get (cfg_sel_halo, halo);
  mp_ui->halo_cb->setChecked (halo);

  bool tm = 0;
  root->config_get (cfg_sel_transient_mode, tm);
  mp_ui->transient_mode_cb->setChecked (tm);

  bool ipm = 0;
  root->config_get (cfg_sel_inside_pcells_mode, ipm);
  mp_ui->sel_inside_pcells_cb->setChecked (ipm);

  bool tpm = 0;
  root->config_get (cfg_text_point_mode, tpm);
  mp_ui->text_point_mode_cb->setChecked (tpm);

  unsigned int sr = 0;
  root->config_get (cfg_search_range, sr);
  mp_ui->search_range_spinbx->setValue (sr);

  unsigned int srbox = 0;
  root->config_get (cfg_search_range_box, srbox);
  mp_ui->search_range_box_spinbx->setValue (srbox);
}

void 
LayoutViewConfigPage2c::commit (lay::Dispatcher *root)
{
  lay::ColorConverter cc;
  root->config_set (cfg_sel_color, mp_ui->color_pb->get_color (), cc);
  root->config_set (cfg_sel_line_width, mp_ui->lw_spinbx->value ());
  root->config_set (cfg_sel_vertex_size, mp_ui->vs_spinbx->value ());
  root->config_set (cfg_sel_dither_pattern, mp_ui->stipple_pb->dither_pattern ());
  root->config_set (cfg_sel_halo, mp_ui->halo_cb->isChecked ());
  root->config_set (cfg_sel_transient_mode, mp_ui->transient_mode_cb->isChecked ());
  root->config_set (cfg_sel_inside_pcells_mode, mp_ui->sel_inside_pcells_cb->isChecked ());
  root->config_set (cfg_text_point_mode, mp_ui->text_point_mode_cb->isChecked ());
  root->config_set (cfg_search_range, (unsigned int) mp_ui->search_range_spinbx->value ());
  root->config_set (cfg_search_range_box, (unsigned int) mp_ui->search_range_box_spinbx->value ());
}

// ------------------------------------------------------------
//  LayoutConfigPage2d implementation

LayoutViewConfigPage2d::LayoutViewConfigPage2d (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage2d ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage2d::~LayoutViewConfigPage2d ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
LayoutViewConfigPage2d::setup (lay::Dispatcher *root)
{
  QColor color;
  root->config_get (cfg_tracking_cursor_color, color, lay::ColorConverter ());
  mp_ui->color_pb->set_color (color);

  bool enabled = false;
  root->config_get (cfg_tracking_cursor_enabled, enabled);
  mp_ui->tracking_cb->setChecked (enabled);

  color = QColor ();
  root->config_get (cfg_crosshair_cursor_color, color, lay::ColorConverter ());
  mp_ui->color_chc->set_color (color);

  int line_style = 0;
  root->config_get (cfg_crosshair_cursor_line_style, line_style);
  mp_ui->line_style_chc->set_line_style (line_style);

  enabled = false;
  root->config_get (cfg_crosshair_cursor_enabled, enabled);
  mp_ui->crosshair_cursor_cb->setChecked (enabled);
}

void
LayoutViewConfigPage2d::commit (lay::Dispatcher *root)
{
  lay::ColorConverter cc;
  root->config_set (cfg_tracking_cursor_color, mp_ui->color_pb->get_color (), cc);
  root->config_set (cfg_tracking_cursor_enabled, mp_ui->tracking_cb->isChecked ());
  root->config_set (cfg_crosshair_cursor_color, mp_ui->color_chc->get_color (), cc);
  root->config_set (cfg_crosshair_cursor_line_style, mp_ui->line_style_chc->line_style ());
  root->config_set (cfg_crosshair_cursor_enabled, mp_ui->crosshair_cursor_cb->isChecked ());
}

// ------------------------------------------------------------
//  LayoutConfigPage3a implementation

LayoutViewConfigPage3a::LayoutViewConfigPage3a (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage3a ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage3a::~LayoutViewConfigPage3a ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage3a::setup (lay::Dispatcher *root)
{
  bool flag = true;

  root->config_get (cfg_fit_new_cell, flag);
  mp_ui->fit_new_cell_cbx->setChecked (flag);
  root->config_get (cfg_full_hier_new_cell, flag);
  mp_ui->full_hier_new_cell_cbx->setChecked (flag);
  root->config_get (cfg_clear_ruler_new_cell, flag);
  mp_ui->clear_ruler_new_cell_cbx->setChecked (flag);
}

void 
LayoutViewConfigPage3a::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_fit_new_cell, mp_ui->fit_new_cell_cbx->isChecked ());
  root->config_set (cfg_full_hier_new_cell, mp_ui->full_hier_new_cell_cbx->isChecked ());
  root->config_set (cfg_clear_ruler_new_cell, mp_ui->clear_ruler_new_cell_cbx->isChecked ());
}

// ------------------------------------------------------------
//  LayoutConfigPage3b implementation

LayoutViewConfigPage3b::LayoutViewConfigPage3b (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage3b ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage3b::~LayoutViewConfigPage3b ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage3b::setup (lay::Dispatcher *root)
{
  bool flag = true;
  double pd = 0.0;
  int dm = 0;

  root->config_get (cfg_paste_display_mode, dm);
  mp_ui->paste_dont_change_rb->setChecked (dm == 0);
  mp_ui->paste_pan_rb->setChecked (dm == 1);
  mp_ui->paste_zoom_rb->setChecked (dm == 2);

  root->config_get (cfg_pan_distance, pd);
  pd *= 100.0;
  mp_ui->pan_distance_le->setText (tl::to_qstring (tl::to_string (pd)));

  root->config_get (cfg_mouse_wheel_mode, flag);
  mp_ui->alt_mouse_wheel_mode_cbx->setChecked (flag);
}

void 
LayoutViewConfigPage3b::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_mouse_wheel_mode, mp_ui->alt_mouse_wheel_mode_cbx->isChecked () ? 1 : 0);

  double pd = 0.0;
  try {
    tl::from_string_ext (tl::to_string (mp_ui->pan_distance_le->text ()), pd);
  } catch (...) { }
  if (pd <= 0.0) {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid pan distance: must be larger than 0.0")));
  }
  root->config_set (cfg_pan_distance, pd * 0.01);

  if (mp_ui->paste_dont_change_rb->isChecked ()) {
    root->config_set (cfg_paste_display_mode, 0);
  } else if (mp_ui->paste_pan_rb->isChecked ()) {
    root->config_set (cfg_paste_display_mode, 1);
  } else if (mp_ui->paste_zoom_rb->isChecked ()) {
    root->config_set (cfg_paste_display_mode, 2);
  }
}

// ------------------------------------------------------------
//  LayoutConfigPage3c implementation

LayoutViewConfigPage3c::LayoutViewConfigPage3c (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage3c ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage3c::~LayoutViewConfigPage3c ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage3c::setup (lay::Dispatcher *root)
{
  bool flag = true;

  root->config_get (cfg_dbu_units, flag);
  mp_ui->dbu_units_cbx->setChecked (flag);
  root->config_get (cfg_abs_units, flag);
  mp_ui->abs_units_cbx->setChecked (flag);
}

void 
LayoutViewConfigPage3c::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_dbu_units, mp_ui->dbu_units_cbx->isChecked ());
  root->config_set (cfg_abs_units, mp_ui->abs_units_cbx->isChecked ());
}

// ------------------------------------------------------------
//  LayoutConfigPage3f implementation

LayoutViewConfigPage3f::LayoutViewConfigPage3f (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage3f ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage3f::~LayoutViewConfigPage3f ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage3f::setup (lay::Dispatcher *root)
{
  int workers = 1;
  bool flag = true;

  root->config_get (cfg_drawing_workers, workers);
  mp_ui->drawing_workers_spbx->setValue (workers);

  root->config_get (cfg_drop_small_cells, flag);
  mp_ui->drop_small_cells_cbx->setChecked (flag);

  unsigned int n = 0;
  root->config_get (cfg_drop_small_cells_cond, n);
  mp_ui->drop_small_cells_cond_cb->setCurrentIndex (n);
  root->config_get (cfg_drop_small_cells_value, n);
  mp_ui->drop_small_cells_value_le->setText (tl::to_qstring (tl::to_string (n)));

  root->config_get (cfg_array_border_instances, flag);
  mp_ui->array_border_insts_cbx->setChecked (flag);

  root->config_get (cfg_text_lazy_rendering, flag);
  mp_ui->text_lazy_rendering_cbx->setChecked (flag);

  root->config_get (cfg_bitmap_caching, flag);
  mp_ui->bitmap_caching_cbx->setChecked (flag);

  n = 0;
  root->config_get (cfg_image_cache_size, n);
  mp_ui->image_cache_size_spbx->setValue (int (n));
}

void 
LayoutViewConfigPage3f::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_drawing_workers, mp_ui->drawing_workers_spbx->value ());

  root->config_set (cfg_drop_small_cells, mp_ui->drop_small_cells_cbx->isChecked ());
  root->config_set (cfg_drop_small_cells_cond, mp_ui->drop_small_cells_cond_cb->currentIndex ());

  try {
    unsigned int s;
    tl::from_string_ext (tl::to_string (mp_ui->drop_small_cells_value_le->text ()), s);
    root->config_set (cfg_drop_small_cells_value, s);
  } catch (...) { }

  root->config_set (cfg_array_border_instances, mp_ui->array_border_insts_cbx->isChecked ());

  root->config_set (cfg_text_lazy_rendering, mp_ui->text_lazy_rendering_cbx->isChecked ());
  root->config_set (cfg_bitmap_caching, mp_ui->bitmap_caching_cbx->isChecked ());

  root->config_set (cfg_image_cache_size, mp_ui->image_cache_size_spbx->value ());
}


// ------------------------------------------------------------
//  LayoutConfigPage4 implementation

class PaletteOp 
  : public db::Op
{
public:
  PaletteOp (const lay::ColorPalette &p, bool e, bool b)
    : palette (p), edit_order (e), before (b)
  { }

  lay::ColorPalette palette;
  bool edit_order, before;
};

static QToolButton * (Ui::LayoutViewConfigPage4::*cfg4_buttons []) = {
  &Ui::LayoutViewConfigPage4::cb_1,  &Ui::LayoutViewConfigPage4::cb_2,  &Ui::LayoutViewConfigPage4::cb_3,  &Ui::LayoutViewConfigPage4::cb_4,  &Ui::LayoutViewConfigPage4::cb_5,
  &Ui::LayoutViewConfigPage4::cb_6,  &Ui::LayoutViewConfigPage4::cb_7,  &Ui::LayoutViewConfigPage4::cb_8,  &Ui::LayoutViewConfigPage4::cb_9,  &Ui::LayoutViewConfigPage4::cb_10,
  &Ui::LayoutViewConfigPage4::cb_11, &Ui::LayoutViewConfigPage4::cb_12, &Ui::LayoutViewConfigPage4::cb_13, &Ui::LayoutViewConfigPage4::cb_14, &Ui::LayoutViewConfigPage4::cb_15,
  &Ui::LayoutViewConfigPage4::cb_16, &Ui::LayoutViewConfigPage4::cb_17, &Ui::LayoutViewConfigPage4::cb_18, &Ui::LayoutViewConfigPage4::cb_19, &Ui::LayoutViewConfigPage4::cb_20,
  &Ui::LayoutViewConfigPage4::cb_21, &Ui::LayoutViewConfigPage4::cb_22, &Ui::LayoutViewConfigPage4::cb_23, &Ui::LayoutViewConfigPage4::cb_24, &Ui::LayoutViewConfigPage4::cb_25,
  &Ui::LayoutViewConfigPage4::cb_26, &Ui::LayoutViewConfigPage4::cb_27, &Ui::LayoutViewConfigPage4::cb_28, &Ui::LayoutViewConfigPage4::cb_29, &Ui::LayoutViewConfigPage4::cb_30,
  &Ui::LayoutViewConfigPage4::cb_31, &Ui::LayoutViewConfigPage4::cb_32, &Ui::LayoutViewConfigPage4::cb_33, &Ui::LayoutViewConfigPage4::cb_34, &Ui::LayoutViewConfigPage4::cb_35,
  &Ui::LayoutViewConfigPage4::cb_36, &Ui::LayoutViewConfigPage4::cb_37, &Ui::LayoutViewConfigPage4::cb_38, &Ui::LayoutViewConfigPage4::cb_39, &Ui::LayoutViewConfigPage4::cb_40,
  &Ui::LayoutViewConfigPage4::cb_41, &Ui::LayoutViewConfigPage4::cb_42
};

LayoutViewConfigPage4::LayoutViewConfigPage4 (QWidget *parent)
  : lay::ConfigPage (parent),
    m_manager (true), m_edit_order_changed_disabled (false)
{
  //  install the manager at db::Object
  manager (&m_manager);

  mp_ui = new Ui::LayoutViewConfigPage4 ();
  mp_ui->setupUi (this);

  for (unsigned int i = 0; i < sizeof (cfg4_buttons) / sizeof (cfg4_buttons [0]); ++i) {
    connect (mp_ui->*(cfg4_buttons [i]), SIGNAL (clicked ()), this, SLOT (color_button_clicked ()));
  }

  connect (mp_ui->undo_pb, SIGNAL (clicked ()), this, SLOT (undo_button_clicked ()));
  connect (mp_ui->redo_pb, SIGNAL (clicked ()), this, SLOT (redo_button_clicked ()));
  connect (mp_ui->reset_pb, SIGNAL (clicked ()), this, SLOT (reset_button_clicked ()));
  connect (mp_ui->edit_order_cbx, SIGNAL (stateChanged (int)), this, SLOT (edit_order_changed (int)));
}

LayoutViewConfigPage4::~LayoutViewConfigPage4 ()
{
  //  uninstall the manager
  manager (0);

  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage4::set_edit_order (bool edit_order)
{
  m_edit_order_changed_disabled = true;
  mp_ui->edit_order_cbx->setChecked (edit_order);
  m_edit_order_changed_disabled = false;
}

void 
LayoutViewConfigPage4::setup (lay::Dispatcher *root)
{
  m_manager.clear ();

  std::string s;
  root->config_get (cfg_color_palette, s);

  lay::ColorPalette palette = lay::ColorPalette::default_palette ();
  try {
    if (! s.empty ()) {
      palette.from_string (s);
    }
  } catch (...) {
    //  ignore errors: just reset the palette 
    palette = lay::ColorPalette::default_palette ();
  }

  m_palette = palette;

  set_edit_order (false);
  update ();
}

void 
LayoutViewConfigPage4::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_color_palette, m_palette.to_string ());
}

void 
LayoutViewConfigPage4::undo (db::Op *op)
{
  PaletteOp *pop = dynamic_cast<PaletteOp *> (op);
  if (pop && pop->before) {
    m_palette = pop->palette;
    set_edit_order (pop->edit_order);
  }
}

void 
LayoutViewConfigPage4::redo (db::Op *op)
{
  PaletteOp *pop = dynamic_cast<PaletteOp *> (op);
  if (pop && ! pop->before) {
    m_palette = pop->palette;
    set_edit_order (pop->edit_order);
  }
}

void 
LayoutViewConfigPage4::undo_button_clicked ()
{
  m_manager.undo ();
  update ();
}

void 
LayoutViewConfigPage4::redo_button_clicked ()
{
  m_manager.redo ();
  update ();
}

void 
LayoutViewConfigPage4::reset_button_clicked ()
{
  m_manager.transaction (tl::to_string (QObject::tr ("Reset palette")));
  m_manager.queue (this, new PaletteOp (m_palette, mp_ui->edit_order_cbx->isChecked (), true /*before*/));
  m_palette = lay::ColorPalette::default_palette ();
  m_manager.queue (this, new PaletteOp (m_palette, false, false /*after*/));
  m_manager.commit ();
  set_edit_order (false);
  update ();
}

void 
LayoutViewConfigPage4::edit_order_changed (int s)
{
  if (m_edit_order_changed_disabled) {
    return;
  }

  if (s) {
    m_manager.transaction (tl::to_string (QObject::tr ("Clear assignment order")));
    m_manager.queue (this, new PaletteOp (m_palette, false, true /*before*/));
    m_palette.clear_luminous_colors ();
    m_manager.queue (this, new PaletteOp (m_palette, true, false /*after*/));
    m_manager.commit ();
    update ();
  } else {
    m_manager.transaction (tl::to_string (QObject::tr ("Stop changing assignment order")));
    m_manager.queue (this, new PaletteOp (m_palette, true, true /*before*/));
    m_manager.queue (this, new PaletteOp (m_palette, false, false /*after*/));
    m_manager.commit ();
  }
}

void 
LayoutViewConfigPage4::color_button_clicked ()
{
  if (mp_ui->edit_order_cbx->isChecked ()) {

    for (unsigned int i = 0; i < sizeof (cfg4_buttons) / sizeof (cfg4_buttons [0]); ++i) {
      if (sender () == mp_ui->*(cfg4_buttons [i])) {

        bool found = false;
        for (unsigned int j = 0; j < m_palette.luminous_colors () && !found; ++j) {
          if (m_palette.luminous_color_index_by_index (j) == i) {
            found = true;
          }
        }
        if (! found) {
          m_manager.transaction (tl::to_string (QObject::tr ("Set assignment order")));
          m_manager.queue (this, new PaletteOp (m_palette, true, true /*before*/));
          m_palette.set_luminous_color_index (m_palette.luminous_colors (), i);
          m_manager.queue (this, new PaletteOp (m_palette, true, false /*after*/));
          m_manager.commit ();
          update ();
        }
        break;
      }
    }

  } else {

    for (unsigned int i = 0; i < sizeof (cfg4_buttons) / sizeof (cfg4_buttons [0]); ++i) {

      if (sender () == mp_ui->*(cfg4_buttons [i])) {

        QColor c;
        if (m_palette.colors () > i) {
          c = QColorDialog::getColor (m_palette.color_by_index (i));
        } else {
          c = QColorDialog::getColor ();
        }
        if (c.isValid ()) {
          m_manager.transaction (tl::to_string (QObject::tr ("Set color")));
          m_manager.queue (this, new PaletteOp (m_palette, false, true /*before*/));
          m_palette.set_color (i, c.rgb ());
          m_manager.queue (this, new PaletteOp (m_palette, false, false /*after*/));
          m_manager.commit ();
          update ();
        }

        break;

      }
    }

  }
}

void 
LayoutViewConfigPage4::update ()
{
  for (unsigned int i = 0; i < sizeof (cfg4_buttons) / sizeof (cfg4_buttons [0]); ++i) {

    QColor color;
    if (i < m_palette.colors ()) {
      color = QColor (m_palette.color_by_index (i));
    }

    QColor text_color = color.green () > 128 ? QColor (0, 0, 0) : QColor (255, 255, 255);

    QString text = QString::fromUtf8 ("  "); 
    for (unsigned int j = 0; j < m_palette.luminous_colors (); ++j) {
      if (i == m_palette.luminous_color_index_by_index (j)) {
        text = tl::to_qstring (tl::sprintf ("%d", j));
        break;
      }
    }

#if QT_VERSION > 0x050000
    double dpr = devicePixelRatio ();
#else
    double dpr = 1.0;
#endif

    QFontMetrics fm (font (), this);
    QRect rt (fm.boundingRect (QString::fromUtf8 ("AA")));

    const unsigned int h = rt.height () + 10;
    const unsigned int w = rt.width () + 10;

    QImage img (w * dpr, h * dpr, QImage::Format_RGB32);
#if QT_VERSION > 0x050000
    img.setDevicePixelRatio (dpr);
#endif

    QPainter painter (&img);
    QRectF r (0.0, 0.0, w, h);
    painter.fillRect (r, QBrush (palette ().color (QPalette::Active, QPalette::ButtonText)));
    r = QRectF (1.0, 1.0, w - 2.0, h - 2.0);
    painter.fillRect (r, QBrush (color));
    painter.setFont (font ());
    painter.setPen (QPen (text_color));
    painter.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, text);

    QPixmap pxmp = QPixmap::fromImage (img);
    (mp_ui->*(cfg4_buttons [i]))->setIconSize (QSize (w, h));
    (mp_ui->*(cfg4_buttons [i]))->setIcon (QIcon (pxmp));

  }
}

// ------------------------------------------------------------
//  LayoutConfigPage5 implementation

LayoutViewConfigPage5::LayoutViewConfigPage5 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage5 ();
  mp_ui->setupUi (this);
  connect (mp_ui->browse_pb, SIGNAL (clicked ()), this, SLOT (browse_clicked ()));
}

LayoutViewConfigPage5::~LayoutViewConfigPage5 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage5::setup (lay::Dispatcher *root)
{
  std::string lyp_file;
  root->config_get (cfg_default_lyp_file, lyp_file);
  mp_ui->lyp_file_gbx->setChecked (! lyp_file.empty ());
  mp_ui->lyp_file_le->setText (tl::to_qstring (lyp_file));

  bool add_other_layers = false;
  root->config_get (cfg_default_add_other_layers, add_other_layers);
  mp_ui->add_other_layers_cb->setChecked (add_other_layers);

  bool always_show_source = false;
  root->config_get (cfg_layers_always_show_source, always_show_source);
  mp_ui->source_display_cb->setChecked (always_show_source);

  bool always_show_ld = false;
  root->config_get (cfg_layers_always_show_ld, always_show_ld);
  mp_ui->ld_display_cb->setChecked (always_show_ld);

  bool always_show_li = false;
  root->config_get (cfg_layers_always_show_layout_index, always_show_li);
  mp_ui->ly_index_cb->setChecked (always_show_li);
}

void 
LayoutViewConfigPage5::commit (lay::Dispatcher *root)
{
  if (mp_ui->lyp_file_gbx->isChecked ()) {
    root->config_set (cfg_default_lyp_file, tl::to_string (mp_ui->lyp_file_le->text ()));
  } else {
    root->config_set (cfg_default_lyp_file, std::string ());
  }
  root->config_set (cfg_default_add_other_layers, mp_ui->add_other_layers_cb->isChecked ());
  root->config_set (cfg_layers_always_show_source, mp_ui->source_display_cb->isChecked ());
  root->config_set (cfg_layers_always_show_ld, mp_ui->ld_display_cb->isChecked ());
  root->config_set (cfg_layers_always_show_layout_index, mp_ui->ly_index_cb->isChecked ());
}

void 
LayoutViewConfigPage5::browse_clicked ()
{
  std::string fn = tl::to_string (mp_ui->lyp_file_le->text ());
  lay::FileDialog file_dialog (this,
    tl::to_string (QObject::tr ("Select Layer Properties File")),
    tl::to_string (QObject::tr ("Layer properties files (*.lyp);;All files (*)")), 
    "lyp");

  if (file_dialog.get_open (fn)) {
    mp_ui->lyp_file_le->setText (tl::to_qstring (fn));
  }
}

// ------------------------------------------------------------
//  LayoutConfigPage6 implementation

class StipplePaletteOp 
  : public db::Op
{
public:
  StipplePaletteOp (const lay::StipplePalette &p, bool e, bool b)
    : palette (p), edit_order (e), before (b)
  { }

  lay::StipplePalette palette;
  bool edit_order, before;
};

static QToolButton * (Ui::LayoutViewConfigPage6::*cfg6_buttons []) = {
  &Ui::LayoutViewConfigPage6::cb_1,  &Ui::LayoutViewConfigPage6::cb_2,  &Ui::LayoutViewConfigPage6::cb_3,  &Ui::LayoutViewConfigPage6::cb_4,  
  &Ui::LayoutViewConfigPage6::cb_5,  &Ui::LayoutViewConfigPage6::cb_6,  &Ui::LayoutViewConfigPage6::cb_7,  &Ui::LayoutViewConfigPage6::cb_8,  
  &Ui::LayoutViewConfigPage6::cb_9,  &Ui::LayoutViewConfigPage6::cb_10, &Ui::LayoutViewConfigPage6::cb_11, &Ui::LayoutViewConfigPage6::cb_12, 
  &Ui::LayoutViewConfigPage6::cb_13, &Ui::LayoutViewConfigPage6::cb_14, &Ui::LayoutViewConfigPage6::cb_15, &Ui::LayoutViewConfigPage6::cb_16, 
};

LayoutViewConfigPage6::LayoutViewConfigPage6 (QWidget *parent)
  : lay::ConfigPage (parent),
    m_manager (true), m_edit_order_changed_disabled (false)
{
  //  install the manager at db::Object
  manager (&m_manager);

  mp_ui = new Ui::LayoutViewConfigPage6 ();
  mp_ui->setupUi (this);

  for (unsigned int i = 0; i < sizeof (cfg6_buttons) / sizeof (cfg6_buttons [0]); ++i) {
    connect (mp_ui->*(cfg6_buttons [i]), SIGNAL (clicked ()), this, SLOT (stipple_button_clicked ()));
  }

  connect (mp_ui->undo_pb, SIGNAL (clicked ()), this, SLOT (undo_button_clicked ()));
  connect (mp_ui->redo_pb, SIGNAL (clicked ()), this, SLOT (redo_button_clicked ()));
  connect (mp_ui->reset_pb, SIGNAL (clicked ()), this, SLOT (reset_button_clicked ()));
  connect (mp_ui->edit_order_cbx, SIGNAL (stateChanged (int)), this, SLOT (edit_order_changed (int)));
}

LayoutViewConfigPage6::~LayoutViewConfigPage6 ()
{
  //  uninstall the manager
  manager (0);

  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage6::set_edit_order (bool edit_order)
{
  m_edit_order_changed_disabled = true;
  mp_ui->edit_order_cbx->setChecked (edit_order);
  m_edit_order_changed_disabled = false;
}

void 
LayoutViewConfigPage6::setup (lay::Dispatcher *root)
{
  m_manager.clear ();

  std::string s;
  root->config_get (cfg_stipple_palette, s);

  lay::StipplePalette palette = lay::StipplePalette::default_palette ();
  try {
    if (! s.empty ()) {
      palette.from_string (s);
    }
  } catch (...) {
    //  ignore errors: just reset the palette 
    palette = lay::StipplePalette::default_palette ();
  }

  m_palette = palette;

  bool f = true;
  root->config_get (cfg_stipple_offset, f);
  mp_ui->stipple_offset_cbx->setChecked (f);

  set_edit_order (false);
  update ();
}

void 
LayoutViewConfigPage6::commit (lay::Dispatcher *root)
{
  if (m_palette.stipples () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No stipples set")));
  }
  if (m_palette.standard_stipples () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No stipples selected for automatic assignment - use 'Set assignment order' to do so")));
  }
  root->config_set (cfg_stipple_palette, m_palette.to_string ());
  root->config_set (cfg_stipple_offset, mp_ui->stipple_offset_cbx->isChecked ());
}

void 
LayoutViewConfigPage6::undo (db::Op *op)
{
  StipplePaletteOp *pop = dynamic_cast<StipplePaletteOp *> (op);
  if (pop && pop->before) {
    m_palette = pop->palette;
    set_edit_order (pop->edit_order);
  }
}

void 
LayoutViewConfigPage6::redo (db::Op *op)
{
  StipplePaletteOp *pop = dynamic_cast<StipplePaletteOp *> (op);
  if (pop && ! pop->before) {
    m_palette = pop->palette;
    set_edit_order (pop->edit_order);
  }
}

void 
LayoutViewConfigPage6::undo_button_clicked ()
{
  m_manager.undo ();
  update ();
}

void 
LayoutViewConfigPage6::redo_button_clicked ()
{
  m_manager.redo ();
  update ();
}

void 
LayoutViewConfigPage6::reset_button_clicked ()
{
  m_manager.transaction (tl::to_string (QObject::tr ("Reset palette")));
  m_manager.queue (this, new StipplePaletteOp (m_palette, mp_ui->edit_order_cbx->isChecked (), true /*before*/));
  m_palette = lay::StipplePalette::default_palette ();
  m_manager.queue (this, new StipplePaletteOp (m_palette, false, false /*after*/));
  m_manager.commit ();
  set_edit_order (false);
  update ();
}

void 
LayoutViewConfigPage6::edit_order_changed (int s)
{
  if (m_edit_order_changed_disabled) {
    return;
  }

  if (s) {
    m_manager.transaction (tl::to_string (QObject::tr ("Clear assignment order")));
    m_manager.queue (this, new StipplePaletteOp (m_palette, false, true /*before*/));
    m_palette.clear_standard_stipples ();
    m_manager.queue (this, new StipplePaletteOp (m_palette, true, false /*after*/));
    m_manager.commit ();
    update ();
  } else {
    m_manager.transaction (tl::to_string (QObject::tr ("Stop changing assignment order")));
    m_manager.queue (this, new StipplePaletteOp (m_palette, true, true /*before*/));
    m_manager.queue (this, new StipplePaletteOp (m_palette, false, false /*after*/));
    m_manager.commit ();
  }
}

void 
LayoutViewConfigPage6::stipple_button_clicked ()
{
  if (mp_ui->edit_order_cbx->isChecked ()) {

    for (unsigned int i = 0; i < sizeof (cfg6_buttons) / sizeof (cfg6_buttons [0]); ++i) {
      if (sender () == mp_ui->*(cfg6_buttons [i])) {

        bool found = false;
        for (unsigned int j = 0; j < m_palette.standard_stipples () && !found; ++j) {
          if (m_palette.standard_stipple_index_by_index (j) == i) {
            found = true;
          }
        }
        if (! found) {
          m_manager.transaction (tl::to_string (QObject::tr ("Set assignment order")));
          m_manager.queue (this, new StipplePaletteOp (m_palette, true, true /*before*/));
          m_palette.set_standard_stipple_index (m_palette.standard_stipples (), i);
          m_manager.queue (this, new StipplePaletteOp (m_palette, true, false /*after*/));
          m_manager.commit ();
          update ();
        }
        break;
      }
    }

  } else {

    for (unsigned int i = 0; i < sizeof (cfg6_buttons) / sizeof (cfg6_buttons [0]); ++i) {

      if (sender () == mp_ui->*(cfg6_buttons [i])) {

        if (m_palette.stipples () > i) {

          SelectStippleForm stipples_form (0, m_pattern);
          if (stipples_form.exec () && stipples_form.selected () >= 0) {

            unsigned int s = stipples_form.selected ();

            m_manager.transaction (tl::to_string (QObject::tr ("Set stipple")));
            m_manager.queue (this, new StipplePaletteOp (m_palette, false, true /*before*/));
            m_palette.set_stipple (i, s);
            m_manager.queue (this, new StipplePaletteOp (m_palette, false, false /*after*/));
            m_manager.commit ();
            update ();

          }

        }

        break;

      }
    }

  }
}

void 
LayoutViewConfigPage6::update ()
{
  for (unsigned int i = 0; i < sizeof (cfg6_buttons) / sizeof (cfg6_buttons [0]); ++i) {

    int s = -1;
    if (i < m_palette.stipples ()) {
      s = (int)m_palette.stipple_by_index (i);
    }

    QString text = QString::fromUtf8 ("  "); 
    for (unsigned int j = 0; j < m_palette.standard_stipples (); ++j) {
      if (i == m_palette.standard_stipple_index_by_index (j)) {
        text = tl::to_qstring (tl::sprintf ("%d", j));
        break;
      }
    }

    QFontMetrics fm (font (), this);
    QRect rt (fm.boundingRect (QString::fromUtf8 ("AA")));

    const unsigned int h = rt.height () + 10;
    const unsigned int w = rt.width () + 10;

    QColor color0 = palette ().color (QPalette::Active, QPalette::Button);
    QColor color1 = palette ().color (QPalette::Active, QPalette::Dark);

#if QT_VERSION > 0x050000
    double dpr = devicePixelRatio ();
#else
    double dpr = 1;
#endif

    QImage image (w * dpr, h * dpr, QImage::Format_RGB32);
#if QT_VERSION > 0x050000
    image.setDevicePixelRatio (dpr);
#endif
    image.fill (color0.rgb ());

    // copying code from layLayerToolbox.cc
    const lay::DitherPatternInfo &info = m_pattern.pattern ((unsigned int) s).scaled (dpr);

    QBitmap bitmap = info.get_bitmap (w * dpr, h * dpr, dpr);
    QPainter painter (&image);
    painter.setPen (QPen (color1));
    painter.setBackgroundMode (Qt::TransparentMode);
    painter.drawPixmap (0, 0, w, h, bitmap);

    painter.setPen (QPen (palette ().color (QPalette::Active, QPalette::Text), 1.0));
    QRectF r (0, 0, w - painter.pen ().widthF (), h - painter.pen ().widthF ());
    painter.setFont (font ());
    painter.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, text);

    QPixmap pxmp = QPixmap::fromImage (image);
    (mp_ui->*(cfg6_buttons [i]))->setIconSize (QSize (w, h));
    (mp_ui->*(cfg6_buttons [i]))->setIcon (QIcon (pxmp));

  }
}

// ------------------------------------------------------------
//  LayoutConfigPage6a implementation

class LineStylePaletteOp
  : public db::Op
{
public:
  LineStylePaletteOp (const lay::LineStylePalette &p, bool b)
    : palette (p), before (b)
  { }

  lay::LineStylePalette palette;
  bool before;
};

static QToolButton * (Ui::LayoutViewConfigPage6a::*cfg6a_buttons []) = {
  &Ui::LayoutViewConfigPage6a::cb_1,  &Ui::LayoutViewConfigPage6a::cb_2,  &Ui::LayoutViewConfigPage6a::cb_3,  &Ui::LayoutViewConfigPage6a::cb_4,
};

LayoutViewConfigPage6a::LayoutViewConfigPage6a (QWidget *parent)
  : lay::ConfigPage (parent), m_manager (true)
{
  //  install the manager at db::Object
  manager (&m_manager);

  mp_ui = new Ui::LayoutViewConfigPage6a ();
  mp_ui->setupUi (this);

  for (unsigned int i = 0; i < sizeof (cfg6a_buttons) / sizeof (cfg6a_buttons [0]); ++i) {
    connect (mp_ui->*(cfg6a_buttons [i]), SIGNAL (clicked ()), this, SLOT (line_style_button_clicked ()));
  }

  connect (mp_ui->undo_pb, SIGNAL (clicked ()), this, SLOT (undo_button_clicked ()));
  connect (mp_ui->redo_pb, SIGNAL (clicked ()), this, SLOT (redo_button_clicked ()));
  connect (mp_ui->reset_pb, SIGNAL (clicked ()), this, SLOT (reset_button_clicked ()));
}

LayoutViewConfigPage6a::~LayoutViewConfigPage6a ()
{
  //  uninstall the manager
  manager (0);

  delete mp_ui;
  mp_ui = 0;
}

void
LayoutViewConfigPage6a::setup (lay::Dispatcher *root)
{
  m_manager.clear ();

  std::string s;
  root->config_get (cfg_line_style_palette, s);

  lay::LineStylePalette palette = lay::LineStylePalette::default_palette ();
  try {
    if (! s.empty ()) {
      palette.from_string (s);
    }
  } catch (...) {
    //  ignore errors: just reset the palette
    palette = lay::LineStylePalette::default_palette ();
  }

  m_palette = palette;

  update ();
}

void
LayoutViewConfigPage6a::commit (lay::Dispatcher *root)
{
  if (m_palette.styles () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No styles set")));
  }
  root->config_set (cfg_line_style_palette, m_palette.to_string ());
}

void
LayoutViewConfigPage6a::undo (db::Op *op)
{
  LineStylePaletteOp *pop = dynamic_cast<LineStylePaletteOp *> (op);
  if (pop && pop->before) {
    m_palette = pop->palette;
  }
}

void
LayoutViewConfigPage6a::redo (db::Op *op)
{
  LineStylePaletteOp *pop = dynamic_cast<LineStylePaletteOp *> (op);
  if (pop && ! pop->before) {
    m_palette = pop->palette;
  }
}

void
LayoutViewConfigPage6a::undo_button_clicked ()
{
  m_manager.undo ();
  update ();
}

void
LayoutViewConfigPage6a::redo_button_clicked ()
{
  m_manager.redo ();
  update ();
}

void
LayoutViewConfigPage6a::reset_button_clicked ()
{
  m_manager.transaction (tl::to_string (QObject::tr ("Reset palette")));
  m_manager.queue (this, new LineStylePaletteOp (m_palette, true /*before*/));
  m_palette = lay::LineStylePalette::default_palette ();
  m_manager.queue (this, new LineStylePaletteOp (m_palette, false /*after*/));
  m_manager.commit ();
  update ();
}

void
LayoutViewConfigPage6a::line_style_button_clicked ()
{
  for (unsigned int i = 0; i < sizeof (cfg6a_buttons) / sizeof (cfg6a_buttons [0]); ++i) {

    if (sender () == mp_ui->*(cfg6a_buttons [i])) {

      if (m_palette.styles () > i) {

        SelectLineStyleForm styles_form (0, m_style);
        if (styles_form.exec () && styles_form.selected () >= 0) {

          unsigned int s = styles_form.selected ();

          m_manager.transaction (tl::to_string (QObject::tr ("Set style")));
          m_manager.queue (this, new LineStylePaletteOp (m_palette, true /*before*/));
          m_palette.set_style (i, s);
          m_manager.queue (this, new LineStylePaletteOp (m_palette, false /*after*/));
          m_manager.commit ();
          update ();

        }

      }

      break;

    }
  }
}

void
LayoutViewConfigPage6a::update ()
{
  for (unsigned int i = 0; i < sizeof (cfg6a_buttons) / sizeof (cfg6a_buttons [0]); ++i) {

    int s = -1;
    if (i < m_palette.styles ()) {
      s = (int)m_palette.style_by_index (i);
    }

    QToolButton *b = mp_ui->*(cfg6a_buttons [i]);

    QColor color0 = b->palette ().color (QPalette::Normal, b->backgroundRole ());
    QColor color1 = b->palette ().color (QPalette::Normal, b->foregroundRole ());

    //  NOTE: we intentionally don't apply devicePixelRatio here as this way, the
    //  image looks more like the style applied on the layout canvas.

    const unsigned int h = 26;
    const unsigned int w = 26;

    QImage image (w, h, QImage::Format_RGB32);
    image.fill (color0.rgb ());

    QBitmap bitmap = m_style.style (s).get_bitmap (w, h);
    QPainter painter (&image);
    painter.setPen (QPen (color1));
    painter.setBackgroundMode (Qt::TransparentMode);
    painter.drawPixmap (0, 0, w, h, bitmap);

    QPixmap pixmap = QPixmap::fromImage (image);
    b->setIconSize (QSize (w, h));
    b->setIcon (QIcon (pixmap));

  }
}

// ------------------------------------------------------------
//  LayoutConfigPage7 implementation

LayoutViewConfigPage7::LayoutViewConfigPage7 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage7 ();
  mp_ui->setupUi (this);

  mp_ui->default_font_size->clear ();
  for (int i = 0; i < lay::FixedFont::font_sizes (); ++i) {
    mp_ui->default_font_size->addItem (QString::fromUtf8 (lay::FixedFont::font_size_name (i)));
  }
}

LayoutViewConfigPage7::~LayoutViewConfigPage7 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
LayoutViewConfigPage7::setup (lay::Dispatcher *root)
{
  int oversampling = 1;
  root->config_get (cfg_bitmap_oversampling, oversampling);
  mp_ui->oversampling->setCurrentIndex (oversampling - 1);

  bool highres_mode = false;
  root->config_get (cfg_highres_mode, highres_mode);
  mp_ui->highres_mode->setChecked (highres_mode);

  int default_font_size = 0;
  root->config_get (cfg_default_font_size, default_font_size);
  mp_ui->default_font_size->setCurrentIndex (default_font_size);

  std::string s;
  root->config_get (cfg_global_trans, s);
  tl::Extractor ex (s.c_str ());

  try {
    db::DCplxTrans t;
    ex.read (t);
    mp_ui->global_trans->setCurrentIndex (t.rot ());
  } catch (...) { }

  int def_depth = 0;
  root->config_get (cfg_initial_hier_depth, def_depth);
  mp_ui->def_depth->setValue(def_depth);
}

void 
LayoutViewConfigPage7::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_bitmap_oversampling, mp_ui->oversampling->currentIndex () + 1);
  root->config_set (cfg_highres_mode, mp_ui->highres_mode->isChecked ());
  root->config_set (cfg_default_font_size, mp_ui->default_font_size->currentIndex ());
  root->config_set (cfg_global_trans, db::DCplxTrans (db::DFTrans (mp_ui->global_trans->currentIndex ())).to_string ());
  root->config_set (cfg_initial_hier_depth, mp_ui->def_depth->value ());
}

// ------------------------------------------------------------
//  LayoutConfigPage8 implementation

LayoutViewConfigPage8::LayoutViewConfigPage8 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::LayoutViewConfigPage8 ();
  mp_ui->setupUi (this);
}

LayoutViewConfigPage8::~LayoutViewConfigPage8 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
LayoutViewConfigPage8::setup (lay::Dispatcher *root)
{
  int cpm = -1;
  root->config_get (cfg_copy_cell_mode, cpm);
  mp_ui->hier_copy_mode_cbx->setCurrentIndex ((cpm < 0 || cpm > 1) ? 2 : cpm);
}

void
LayoutViewConfigPage8::commit (lay::Dispatcher *root)
{
  int cpm = mp_ui->hier_copy_mode_cbx->currentIndex ();
  root->config_set (cfg_copy_cell_mode, (cpm < 0 || cpm > 1) ? -1 : cpm);
}

// ------------------------------------------------------------
//  The dummy plugin declaration to register the configuration options

class LayoutViewConfigDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual std::vector<std::pair <std::string, ConfigPage *> > config_pages (QWidget *parent) const 
  {
    std::vector<std::pair <std::string, ConfigPage *> > pages;

    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Display|General")),         new LayoutViewConfigPage7 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Display|Cells")),           new LayoutViewConfigPage2a (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Display|Texts")),           new LayoutViewConfigPage2b (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Display|Color Palette")),   new LayoutViewConfigPage4 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Display|Stipple Palette")), new LayoutViewConfigPage6 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Display|Line Style Palette")), new LayoutViewConfigPage6a (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Display|Background")),      new LayoutViewConfigPage (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Display|Context")),         new LayoutViewConfigPage1 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Display|Optimization")),    new LayoutViewConfigPage3f (parent)));

    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Selection")),   new LayoutViewConfigPage2c (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Tracking")),    new LayoutViewConfigPage2d (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Layer Properties")),  new LayoutViewConfigPage5 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Units")),       new LayoutViewConfigPage3c (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Cells")),       new LayoutViewConfigPage8 (parent)));

    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Navigation|New Cell")),     new LayoutViewConfigPage3a (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Navigation|Zoom And Pan")), new LayoutViewConfigPage3b (parent)));

    return pages;
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new LayoutViewConfigDeclaration (), 2000, "LayoutViewConfig");

}

#endif

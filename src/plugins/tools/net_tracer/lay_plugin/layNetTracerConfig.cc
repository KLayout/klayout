
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


#include "layNetTracerConfig.h"
#include "layConverters.h"
#include "layDispatcher.h"

#include <QColorDialog>
#include <QPainter>

namespace lay
{

extern const std::string cfg_nt_marker_color ("nt-marker-color");
extern const std::string cfg_nt_marker_cycle_colors ("nt-marker-cycle-colors");
extern const std::string cfg_nt_marker_cycle_colors_enabled ("nt-marker-cycle-colors-enabled");
extern const std::string cfg_nt_marker_dither_pattern ("nt-marker-dither-pattern");
extern const std::string cfg_nt_marker_line_width ("nt-marker-line-width");
extern const std::string cfg_nt_marker_vertex_size ("nt-marker-vertex-size");
extern const std::string cfg_nt_marker_halo ("nt-marker-halo");
extern const std::string cfg_nt_marker_intensity ("nt-marker-intensity");
extern const std::string cfg_nt_window_mode ("nt-window-mode");
extern const std::string cfg_nt_window_dim ("nt-window-dim");
extern const std::string cfg_nt_max_shapes_highlighted ("nt-max-shapes-highlighted");
extern const std::string cfg_nt_trace_depth ("nt-trace_depth");

// ------------------------------------------------------------

static struct {
  lay::nt_window_type mode;
  const char *string;
} window_modes [] = {
  { lay::NTDontChange,    "dont-change" },
  { lay::NTFitNet,        "fit-net"     },
  { lay::NTCenter,        "center"      },
  { lay::NTCenterSize,    "center-size" }
};

void
NetTracerWindowModeConverter::from_string (const std::string &value, lay::nt_window_type &mode)
{
  for (unsigned int i = 0; i < sizeof (window_modes) / sizeof (window_modes [0]); ++i) {
    if (value == window_modes [i].string) {
      mode = window_modes [i].mode;
      return;
    }
  }
  throw tl::Exception (tl::to_string (QObject::tr ("Invalid net tracer window mode: ")) + value);
}

std::string 
NetTracerWindowModeConverter::to_string (lay::nt_window_type mode)
{
  for (unsigned int i = 0; i < sizeof (window_modes) / sizeof (window_modes [0]); ++i) {
    if (mode == window_modes [i].mode) {
      return window_modes [i].string;
    }
  }
  return "";
}

// ------------------------------------------------------------
//
static QToolButton * (Ui::NetTracerConfigPage::*cc_buttons []) = {
  &Ui::NetTracerConfigPage::cc0,  
  &Ui::NetTracerConfigPage::cc1,  
  &Ui::NetTracerConfigPage::cc2,  
  &Ui::NetTracerConfigPage::cc3,  
  &Ui::NetTracerConfigPage::cc4,  
  &Ui::NetTracerConfigPage::cc5,  
  &Ui::NetTracerConfigPage::cc6,  
  &Ui::NetTracerConfigPage::cc7
};
 
NetTracerConfigPage::NetTracerConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  Ui::NetTracerConfigPage::setupUi (this);

  connect (cbx_window, SIGNAL (currentIndexChanged (int)), this, SLOT (window_changed (int)));

  for (unsigned int i = 0; i < sizeof (cc_buttons) / sizeof (cc_buttons [0]); ++i) {
    connect (this->*(cc_buttons [i]), SIGNAL (clicked ()), this, SLOT (color_button_clicked ()));
  }
}

void 
NetTracerConfigPage::color_button_clicked ()
{
  for (unsigned int i = 0; i < sizeof (cc_buttons) / sizeof (cc_buttons [0]); ++i) {

    if (sender () == this->*(cc_buttons [i])) {

      QColor c;
      if (m_palette.colors () > i) {
        c = QColorDialog::getColor (m_palette.color_by_index (i));
      } else {
        c = QColorDialog::getColor ();
      }
      if (c.isValid ()) {
        m_palette.set_color (i, c.rgb ());
        update_colors ();
      }

      break;

    }

  }
}

void 
NetTracerConfigPage::setup (lay::Dispatcher *root)
{
  //  window mode
  lay::nt_window_type wmode = lay::NTFitNet;
  root->config_get (cfg_nt_window_mode, wmode, NetTracerWindowModeConverter ());
  cbx_window->setCurrentIndex (int (wmode));

  //  window dimension
  double wdim = 1.0;
  root->config_get (cfg_nt_window_dim, wdim);
  le_window->setText (tl::to_qstring (tl::to_string (wdim)));
    
  //  max. shapes highlighted
  unsigned int max_marker_count = 10000;
  root->config_get (cfg_nt_max_shapes_highlighted, max_marker_count);
  le_max_markers->setText (tl::to_qstring (tl::to_string (max_marker_count)));

  //  enable controls
  window_changed (int (wmode));

  bool cycle_enabled = false;
  root->config_get (cfg_nt_marker_cycle_colors_enabled, cycle_enabled);
  cycle_colors_cb->setChecked (cycle_enabled);

  std::string cc;
  root->config_get (cfg_nt_marker_cycle_colors, cc);
  try {
    m_palette.from_string (cc, true);
  } catch (...) {
  }

  while (m_palette.colors () < sizeof (cc_buttons) / sizeof (cc_buttons [0])) {
    m_palette.set_color (int (m_palette.colors ()), 0);
  }
 
  update_colors ();

  //  marker color
  QColor color;
  root->config_get (cfg_nt_marker_color, color, lay::ColorConverter ());
  color_pb->set_color (color);

  //  brightness offset
  int brightness = 0;
  root->config_get (cfg_nt_marker_intensity, brightness);
  brightness_sb->setValue (brightness);

  //  marker line width
  int lw = 0;
  root->config_get (cfg_nt_marker_line_width, lw);
  if (lw < 0) {
    lw_le->setText (QString ());
  } else {
    lw_le->setText (tl::to_qstring (tl::to_string (lw)));
  }

  //  marker vertex size
  int vs = 0;
  root->config_get (cfg_nt_marker_vertex_size, vs);
  if (vs < 0) {
    vs_le->setText (QString ());
  } else {
    vs_le->setText (tl::to_qstring (tl::to_string (vs)));
  }

  //  stipple pattern
  int dp = 0;
  root->config_get (cfg_nt_marker_dither_pattern, dp);
  stipple_pb->set_dither_pattern (dp);

  //  halo
  int halo = 0;
  root->config_get (cfg_nt_marker_halo, halo);
  halo_cb->setCheckState (halo < 0 ? Qt::PartiallyChecked : (halo ? Qt::Checked : Qt::Unchecked));
}

void 
NetTracerConfigPage::update_colors ()
{
  for (unsigned int i = 0; i < sizeof (cc_buttons) / sizeof (cc_buttons [0]); ++i) {

    QColor color;
    if (i < m_palette.colors ()) {
      color = QColor (m_palette.color_by_index (i));
    }

    QFontMetrics fm (font (), this);
    QRect rt (fm.boundingRect (QString::fromUtf8 ("AA")));
    QPixmap pxmp (rt.width () + 10, rt.height () + 10);

    QPainter pxpainter (&pxmp);
    pxpainter.setPen (QPen (palette ().color (QPalette::Active, QPalette::Text)));
    pxpainter.setBrush (QBrush (color));
    QRect r (0, 0, pxmp.width () - 1, pxmp.height () - 1);
    pxpainter.drawRect (r);

    (this->*(cc_buttons [i]))->setIconSize (pxmp.size ());
    (this->*(cc_buttons [i]))->setIcon (QIcon (pxmp));

  }
}

void
NetTracerConfigPage::window_changed (int m)
{
  le_window->setEnabled (m == int (lay::NTFitNet) || m == int (lay::NTCenterSize));
}

void 
NetTracerConfigPage::commit (lay::Dispatcher *root)
{
  double dim = 1.0;
  tl::from_string_ext (tl::to_string (le_window->text ()), dim);

  unsigned int max_shapes_highlighted = 10000;
  tl::from_string_ext (tl::to_string (le_max_markers->text ()), max_shapes_highlighted);

  root->config_set (cfg_nt_window_mode, lay::nt_window_type (cbx_window->currentIndex ()), NetTracerWindowModeConverter ());
  root->config_set (cfg_nt_window_dim, dim);
  root->config_set (cfg_nt_max_shapes_highlighted, max_shapes_highlighted);

  root->config_set (cfg_nt_marker_cycle_colors_enabled, cycle_colors_cb->isChecked ());
  root->config_set (cfg_nt_marker_cycle_colors, m_palette.to_string ());

  QColor color = color_pb->get_color ();
  root->config_set (cfg_nt_marker_color, color, lay::ColorConverter ());

  if (lw_le->text ().isEmpty ()) {
    root->config_set (cfg_nt_marker_line_width, -1);
  } else {
    try {
      int s;
      tl::from_string_ext (tl::to_string (lw_le->text ()), s);
      root->config_set (cfg_nt_marker_line_width, s);
    } catch (...) { }
  }

  if (vs_le->text ().isEmpty ()) {
    root->config_set (cfg_nt_marker_vertex_size, -1);
  } else {
    try {
      int s;
      tl::from_string_ext (tl::to_string (vs_le->text ()), s);
      root->config_set (cfg_nt_marker_vertex_size, s);
    } catch (...) { }
  }

  root->config_set (cfg_nt_marker_dither_pattern, stipple_pb->dither_pattern ());

  if (halo_cb->checkState () == Qt::PartiallyChecked) {
    root->config_set (cfg_nt_marker_halo, -1);
  } else if (halo_cb->checkState () == Qt::Unchecked) {
    root->config_set (cfg_nt_marker_halo, 0);
  } else if (halo_cb->checkState () == Qt::Checked) {
    root->config_set (cfg_nt_marker_halo, 1);
  }

  root->config_set (cfg_nt_marker_intensity, brightness_sb->value ());
}

}


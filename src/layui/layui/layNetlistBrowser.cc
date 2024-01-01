
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

#include "layuiCommon.h"
#include "layNetlistBrowserDialog.h"
#include "layConverters.h"
#include "layDispatcher.h"
#include "layUtils.h"

#include "ui_NetlistBrowserConfigPage.h"
#include "ui_NetlistBrowserConfigPage2.h"

#include <set>

#include <QColorDialog>
#include <QPainter>

namespace lay
{

// ------------------------------------------------------------
//  Declaration of the configuration options

extern const std::string cfg_l2ndb_marker_color ("l2ndb-marker-color");
extern const std::string cfg_l2ndb_marker_cycle_colors ("l2ndb-marker-cycle-colors");
extern const std::string cfg_l2ndb_marker_cycle_colors_enabled ("l2ndb-marker-cycle-colors-enabled");
extern const std::string cfg_l2ndb_marker_dither_pattern ("l2ndb-marker-dither-pattern");
extern const std::string cfg_l2ndb_marker_line_width ("l2ndb-marker-line-width");
extern const std::string cfg_l2ndb_marker_vertex_size ("l2ndb-marker-vertex-size");
extern const std::string cfg_l2ndb_marker_halo ("l2ndb-marker-halo");
extern const std::string cfg_l2ndb_marker_intensity ("l2ndb-marker-intensity");
extern const std::string cfg_l2ndb_marker_use_original_colors ("l2ndb-marker-use-original-colors");
extern const std::string cfg_l2ndb_window_mode ("l2ndb-window-mode");
extern const std::string cfg_l2ndb_window_dim ("l2ndb-window-dim");
extern const std::string cfg_l2ndb_max_shapes_highlighted ("l2ndb-max-shapes-highlighted");
extern const std::string cfg_l2ndb_show_all ("l2ndb-show-all");
extern const std::string cfg_l2ndb_window_state ("l2ndb-window-state");
extern const std::string cfg_l2ndb_export_net_cell_prefix ("l2ndb-export-net-cell-prefix");
extern const std::string cfg_l2ndb_export_net_propname ("l2ndb-export-net-propname");
extern const std::string cfg_l2ndb_export_start_layer_number ("l2ndb-export-start-layer-number");
extern const std::string cfg_l2ndb_export_circuit_cell_prefix ("l2ndb-export-circuit-cell-prefix");
extern const std::string cfg_l2ndb_export_produce_circuit_cells ("l2ndb-export-produce-circuit-cells");
extern const std::string cfg_l2ndb_export_device_cell_prefix ("l2ndb-export-device-cell-prefix");
extern const std::string cfg_l2ndb_export_produce_device_cells ("l2ndb-export-produce-device-cells");

// ------------------------------------------------------------

static struct {
  lay::NetlistBrowserConfig::net_window_type mode;
  const char *string;
} window_modes [] = {
  { lay::NetlistBrowserConfig::DontChange,    "dont-change" },
  { lay::NetlistBrowserConfig::FitNet,        "fit-net"     },
  { lay::NetlistBrowserConfig::Center,        "center"      },
  { lay::NetlistBrowserConfig::CenterSize,    "center-size" }
};

void
NetlistBrowserWindowModeConverter::from_string (const std::string &value, lay::NetlistBrowserConfig::net_window_type &mode)
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
NetlistBrowserWindowModeConverter::to_string (lay::NetlistBrowserConfig::net_window_type mode)
{
  for (unsigned int i = 0; i < sizeof (window_modes) / sizeof (window_modes [0]); ++i) {
    if (mode == window_modes [i].mode) {
      return window_modes [i].string;
    }
  }
  return "";
}

// ------------------------------------------------------------

NetlistBrowserConfigPage::NetlistBrowserConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::NetlistBrowserConfigPage ();
  mp_ui->setupUi (this);

  connect (mp_ui->cbx_window, SIGNAL (currentIndexChanged (int)), this, SLOT (window_changed (int)));
}

NetlistBrowserConfigPage::~NetlistBrowserConfigPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
NetlistBrowserConfigPage::setup (lay::Dispatcher *root)
{
  //  window mode
  lay::NetlistBrowserConfig::net_window_type wmode = lay::NetlistBrowserConfig::FitNet;
  root->config_get (cfg_l2ndb_window_mode, wmode, NetlistBrowserWindowModeConverter ());
  mp_ui->cbx_window->setCurrentIndex (int (wmode));

  //  window dimension
  double wdim = 1.0;
  root->config_get (cfg_l2ndb_window_dim, wdim);
  mp_ui->le_window->setText (tl::to_qstring (tl::to_string (wdim)));

  //  max. shapes highlighted
  unsigned int max_marker_count = 10000;
  root->config_get (cfg_l2ndb_max_shapes_highlighted, max_marker_count);
  mp_ui->le_max_markers->setText (tl::to_qstring (tl::to_string (max_marker_count)));

  //  enable controls
  window_changed (int (wmode));
}

void
NetlistBrowserConfigPage::window_changed (int m)
{
  mp_ui->le_window->setEnabled (m == int (lay::NetlistBrowserConfig::FitNet) || m == int (lay::NetlistBrowserConfig::CenterSize));
}

void
NetlistBrowserConfigPage::commit (lay::Dispatcher *root)
{
  double dim = 1.0;
  tl::from_string_ext (tl::to_string (mp_ui->le_window->text ()), dim);

  unsigned int max_shapes_highlighted = 10000;
  tl::from_string_ext (tl::to_string (mp_ui->le_max_markers->text ()), max_shapes_highlighted);

  root->config_set (cfg_l2ndb_window_mode, lay::NetlistBrowserConfig::net_window_type (mp_ui->cbx_window->currentIndex ()), NetlistBrowserWindowModeConverter ());
  root->config_set (cfg_l2ndb_window_dim, dim);
  root->config_set (cfg_l2ndb_max_shapes_highlighted, max_shapes_highlighted);
}

// ------------------------------------------------------------

static QToolButton * (Ui::NetlistBrowserConfigPage2::*cc_buttons []) = {
  &Ui::NetlistBrowserConfigPage2::cc0,
  &Ui::NetlistBrowserConfigPage2::cc1,
  &Ui::NetlistBrowserConfigPage2::cc2,
  &Ui::NetlistBrowserConfigPage2::cc3,
  &Ui::NetlistBrowserConfigPage2::cc4,
  &Ui::NetlistBrowserConfigPage2::cc5,
  &Ui::NetlistBrowserConfigPage2::cc6,
  &Ui::NetlistBrowserConfigPage2::cc7
};

NetlistBrowserConfigPage2::NetlistBrowserConfigPage2 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::NetlistBrowserConfigPage2 ();
  mp_ui->setupUi (this);

  for (unsigned int i = 0; i < sizeof (cc_buttons) / sizeof (cc_buttons [0]); ++i) {
    connect (mp_ui->*(cc_buttons [i]), SIGNAL (clicked ()), this, SLOT (color_button_clicked ()));
  }
}

NetlistBrowserConfigPage2::~NetlistBrowserConfigPage2 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
NetlistBrowserConfigPage2::color_button_clicked ()
{
  for (unsigned int i = 0; i < sizeof (cc_buttons) / sizeof (cc_buttons [0]); ++i) {

    if (sender () == mp_ui->*(cc_buttons [i])) {

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
NetlistBrowserConfigPage2::setup (lay::Dispatcher *root)
{
  bool cycle_enabled = false;
  root->config_get (cfg_l2ndb_marker_cycle_colors_enabled, cycle_enabled);
  mp_ui->cycle_colors_cb->setChecked (cycle_enabled);

  std::string cc;
  root->config_get (cfg_l2ndb_marker_cycle_colors, cc);
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
  root->config_get (cfg_l2ndb_marker_color, color, lay::ColorConverter ());
  mp_ui->color_pb->set_color (color);

  //  use original color
  bool original_colors = false;
  root->config_get (cfg_l2ndb_marker_use_original_colors, original_colors);
  mp_ui->brightness_cb->setChecked (original_colors);
  mp_ui->brightness_sb->setEnabled (original_colors);
  mp_ui->brightness_label->setEnabled (original_colors);

  //  brightness offset
  int brightness = 0;
  root->config_get (cfg_l2ndb_marker_intensity, brightness);
  mp_ui->brightness_sb->setValue (brightness);

  //  marker line width
  int lw = 0;
  root->config_get (cfg_l2ndb_marker_line_width, lw);
  if (lw < 0) {
    mp_ui->lw_le->setText (QString ());
  } else {
    mp_ui->lw_le->setText (tl::to_qstring (tl::to_string (lw)));
  }

  //  marker vertex size
  int vs = 0;
  root->config_get (cfg_l2ndb_marker_vertex_size, vs);
  if (vs < 0) {
    mp_ui->vs_le->setText (QString ());
  } else {
    mp_ui->vs_le->setText (tl::to_qstring (tl::to_string (vs)));
  }

  //  stipple pattern
  int dp = 0;
  root->config_get (cfg_l2ndb_marker_dither_pattern, dp);
  mp_ui->stipple_pb->set_dither_pattern (dp);

  //  halo
  int halo = 0;
  root->config_get (cfg_l2ndb_marker_halo, halo);
  mp_ui->halo_cb->setCheckState (halo < 0 ? Qt::PartiallyChecked : (halo ? Qt::Checked : Qt::Unchecked));
}

void
NetlistBrowserConfigPage2::update_colors ()
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

    (mp_ui->*(cc_buttons [i]))->setIconSize (pxmp.size ());
    (mp_ui->*(cc_buttons [i]))->setIcon (QIcon (pxmp));

  }
}

void
NetlistBrowserConfigPage2::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_l2ndb_marker_cycle_colors_enabled, mp_ui->cycle_colors_cb->isChecked ());
  root->config_set (cfg_l2ndb_marker_cycle_colors, m_palette.to_string ());

  QColor color = mp_ui->color_pb->get_color ();
  root->config_set (cfg_l2ndb_marker_color, color, lay::ColorConverter ());

  if (mp_ui->lw_le->text ().isEmpty ()) {
    root->config_set (cfg_l2ndb_marker_line_width, -1);
  } else {
    try {
      int s;
      tl::from_string_ext (tl::to_string (mp_ui->lw_le->text ()), s);
      root->config_set (cfg_l2ndb_marker_line_width, s);
    } catch (...) { }
  }

  if (mp_ui->vs_le->text ().isEmpty ()) {
    root->config_set (cfg_l2ndb_marker_vertex_size, -1);
  } else {
    try {
      int s;
      tl::from_string_ext (tl::to_string (mp_ui->vs_le->text ()), s);
      root->config_set (cfg_l2ndb_marker_vertex_size, s);
    } catch (...) { }
  }

  root->config_set (cfg_l2ndb_marker_dither_pattern, mp_ui->stipple_pb->dither_pattern ());

  if (mp_ui->halo_cb->checkState () == Qt::PartiallyChecked) {
    root->config_set (cfg_l2ndb_marker_halo, -1);
  } else if (mp_ui->halo_cb->checkState () == Qt::Unchecked) {
    root->config_set (cfg_l2ndb_marker_halo, 0);
  } else if (mp_ui->halo_cb->checkState () == Qt::Checked) {
    root->config_set (cfg_l2ndb_marker_halo, 1);
  }

  root->config_set (cfg_l2ndb_marker_intensity, mp_ui->brightness_sb->value ());

  root->config_set (cfg_l2ndb_marker_use_original_colors, mp_ui->brightness_cb->isChecked ());
}

// ------------------------------------------------------------
//  Declaration and implementation of the browser plugin declaration object

class NetlistBrowserPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_window_mode, "fit-net"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_window_dim, "1.0"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_max_shapes_highlighted, "10000"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_marker_color, lay::ColorConverter ().to_string (QColor ())));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_marker_cycle_colors_enabled, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_marker_cycle_colors, "255,0,0 0,255,0 0,0,255 255,255,0 255,0,255 0,255,255 160,80,255 255,160,0"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_marker_line_width, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_marker_vertex_size, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_marker_halo, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_marker_use_original_colors, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_marker_dither_pattern, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_marker_intensity, "50"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_show_all, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_window_state, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_export_net_propname, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_export_net_cell_prefix, "NET_"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_export_start_layer_number, "1000"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_export_produce_circuit_cells, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_export_circuit_cell_prefix, "CIRCUIT_"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_export_produce_device_cells, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2ndb_export_device_cell_prefix, "DEVICE_"));
  }

  virtual std::vector<std::pair <std::string, lay::ConfigPage *> > config_pages (QWidget *parent) const
  {
    std::vector<std::pair <std::string, lay::ConfigPage *> > pages;
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Netlist Browser|Setup")), new NetlistBrowserConfigPage (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Netlist Browser|Net Appearance")), new NetlistBrowserConfigPage2 (parent)));
    return pages;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::menu_item ("netlist_browser::show", "browse_netlists", "tools_menu.end", tl::to_string (QObject::tr ("Netlist Browser"))));
  }

  virtual lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  {
    if (has_gui ()) {
      return new lay::NetlistBrowserDialog (root, view);
    } else {
      return 0;
    }
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new NetlistBrowserPluginDeclaration (), 12100, "NetlistBrowserPlugin");

}

#endif

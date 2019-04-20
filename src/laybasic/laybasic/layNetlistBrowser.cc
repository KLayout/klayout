
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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



#include "layNetlistBrowserDialog.h"

#include "layConverters.h"

#include <set>

namespace lay
{

// ------------------------------------------------------------
//  Declaration of the configuration options

std::string cfg_l2n_context_mode ("l2n-context-mode");
std::string cfg_l2n_show_all ("l2n-show-all");
std::string cfg_l2n_window_state ("l2n-window-state");
std::string cfg_l2n_window_mode ("l2n-window-mode");
std::string cfg_l2n_window_dim ("l2n-window-dim");
std::string cfg_l2n_max_marker_count ("l2n-max-marker-count");
std::string cfg_l2n_highlight_color ("l2n-highlight-color");
std::string cfg_l2n_highlight_line_width ("l2n-highlight-line-width");
std::string cfg_l2n_highlight_vertex_size ("l2n-highlight-vertex-size");
std::string cfg_l2n_highlight_halo ("l2n-highlight-halo");
std::string cfg_l2n_highlight_dither_pattern ("l2n-highlight-dither-pattern");

// ------------------------------------------------------------

static struct {
  lay::NetlistBrowserConfig::net_context_mode_type mode;
  const char *string;
} context_modes [] = {
  { lay::NetlistBrowserConfig::AnyCell,       "any-cell"     },
  { lay::NetlistBrowserConfig::NetlistTop,    "netlist-top" },
  { lay::NetlistBrowserConfig::Current,       "current-cell" },
  { lay::NetlistBrowserConfig::CurrentOrAny,  "current-or-any-cell" },
  { lay::NetlistBrowserConfig::Local,         "local-cell"   },
};

void
NetlistBrowserContextModeConverter::from_string (const std::string &value, lay::NetlistBrowserConfig::net_context_mode_type &mode)
{
  for (unsigned int i = 0; i < sizeof (context_modes) / sizeof (context_modes [0]); ++i) {
    if (value == context_modes [i].string) {
      mode = context_modes [i].mode;
      return;
    }
  }
  throw tl::Exception (tl::to_string (QObject::tr ("Invalid marker database browser context mode: ")) + value);
}

std::string
NetlistBrowserContextModeConverter::to_string (lay::NetlistBrowserConfig::net_context_mode_type mode)
{
  for (unsigned int i = 0; i < sizeof (context_modes) / sizeof (context_modes [0]); ++i) {
    if (mode == context_modes [i].mode) {
      return context_modes [i].string;
    }
  }
  return "";
}


// ------------------------------------------------------------

static struct {
  lay::NetlistBrowserConfig::net_window_type mode;
  const char *string;
} window_modes [] = {
  { lay::NetlistBrowserConfig::DontChange,    "dont-change" },
  { lay::NetlistBrowserConfig::FitCell,       "fit-cell"    },
  { lay::NetlistBrowserConfig::FitNet,        "fit-net"  },
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
  throw tl::Exception (tl::to_string (QObject::tr ("Invalid marker database browser window mode: ")) + value);
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
//  Implementation of NetlistBrowserConfigPage

NetlistBrowserConfigPage::NetlistBrowserConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  Ui::NetlistBrowserConfigPage::setupUi (this);

  connect (cbx_window, SIGNAL (currentIndexChanged (int)), this, SLOT (window_changed (int)));
}

void
NetlistBrowserConfigPage::setup (lay::PluginRoot *root)
{
  //  context mode
  lay::NetlistBrowserConfig::net_context_mode_type cmode = lay::NetlistBrowserConfig::NetlistTop;
  root->config_get (cfg_l2n_context_mode, cmode, NetlistBrowserContextModeConverter ());
  cbx_context->setCurrentIndex (int (cmode));

  //  window mode
  lay::NetlistBrowserConfig::net_window_type wmode = lay::NetlistBrowserConfig::FitNet;
  root->config_get (cfg_l2n_window_mode, wmode, NetlistBrowserWindowModeConverter ());
  cbx_window->setCurrentIndex (int (wmode));

  //  window dimension
  double wdim = 1.0;
  root->config_get (cfg_l2n_window_dim, wdim);
  le_window->setText (tl::to_qstring (tl::to_string (wdim)));

  //  max. marker count
  unsigned int max_marker_count = 1000;
  root->config_get (cfg_l2n_max_marker_count, max_marker_count);
  le_max_markers->setText (tl::to_qstring (tl::to_string (max_marker_count)));

  //  enable controls
  window_changed (int (wmode));
}

void
NetlistBrowserConfigPage::window_changed (int m)
{
  le_window->setEnabled (m == int (lay::NetlistBrowserConfig::FitNet) || m == int (lay::NetlistBrowserConfig::CenterSize));
}

void
NetlistBrowserConfigPage::commit (lay::PluginRoot *root)
{
  double dim = 1.0;
  tl::from_string (tl::to_string (le_window->text ()), dim);

  unsigned int max_markers_count = 1000;
  tl::from_string (tl::to_string (le_max_markers->text ()), max_markers_count);

  root->config_set (cfg_l2n_context_mode, lay::NetlistBrowserConfig::net_context_mode_type (cbx_context->currentIndex ()), NetlistBrowserContextModeConverter ());
  root->config_set (cfg_l2n_window_mode, lay::NetlistBrowserConfig::net_window_type (cbx_window->currentIndex ()), NetlistBrowserWindowModeConverter ());
  root->config_set (cfg_l2n_window_dim, dim);
  root->config_set (cfg_l2n_max_marker_count, max_markers_count);
}

// ------------------------------------------------------------
//  Implementation of NetlistBrowserConfigPage2

NetlistBrowserConfigPage2::NetlistBrowserConfigPage2 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  Ui::NetlistBrowserConfigPage2::setupUi (this);
}

void
NetlistBrowserConfigPage2::setup (lay::PluginRoot *root)
{
  //  marker color
  QColor color;
  root->config_get (cfg_l2n_highlight_color, color, lay::ColorConverter ());
  color_pb->set_color (color);

  //  marker line width
  int lw = 0;
  root->config_get (cfg_l2n_highlight_line_width, lw);
  if (lw < 0) {
    lw_le->setText (QString ());
  } else {
    lw_le->setText (tl::to_qstring (tl::to_string (lw)));
  }

  //  marker vertex size
  int vs = 0;
  root->config_get (cfg_l2n_highlight_vertex_size, vs);
  if (vs < 0) {
    vs_le->setText (QString ());
  } else {
    vs_le->setText (tl::to_qstring (tl::to_string (vs)));
  }

  //  stipple pattern
  int dp = 0;
  root->config_get (cfg_l2n_highlight_dither_pattern, dp);
  stipple_pb->set_dither_pattern (dp);

  //  halo
  int halo = 0;
  root->config_get (cfg_l2n_highlight_halo, halo);
  halo_cb->setCheckState (halo < 0 ? Qt::PartiallyChecked : (halo ? Qt::Checked : Qt::Unchecked));
}

void
NetlistBrowserConfigPage2::commit (lay::PluginRoot *root)
{
  QColor color (color_pb->get_color ());
  root->config_set (cfg_l2n_highlight_color, color, lay::ColorConverter ());

  if (lw_le->text ().isEmpty ()) {
    root->config_set (cfg_l2n_highlight_line_width, -1);
  } else {
    try {
      int s;
      tl::from_string (tl::to_string (lw_le->text ()), s);
      root->config_set (cfg_l2n_highlight_line_width, s);
    } catch (...) { }
  }

  if (vs_le->text ().isEmpty ()) {
    root->config_set (cfg_l2n_highlight_vertex_size, -1);
  } else {
    try {
      int s;
      tl::from_string (tl::to_string (vs_le->text ()), s);
      root->config_set (cfg_l2n_highlight_vertex_size, s);
    } catch (...) { }
  }

  root->config_set (cfg_l2n_highlight_dither_pattern, stipple_pb->dither_pattern ());

  if (halo_cb->checkState () == Qt::PartiallyChecked) {
    root->config_set (cfg_l2n_highlight_halo, -1);
  } else if (halo_cb->checkState () == Qt::Unchecked) {
    root->config_set (cfg_l2n_highlight_halo, 0);
  } else if (halo_cb->checkState () == Qt::Checked) {
    root->config_set (cfg_l2n_highlight_halo, 1);
  }
}

// ------------------------------------------------------------
//  Declaration and implementation of the browser plugin declaration object

class NetlistBrowserPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_l2n_context_mode, "netlist-top"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2n_window_mode, "fit-net"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2n_window_state, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_l2n_window_dim, "1.0"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2n_max_marker_count, "1000"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2n_highlight_color, lay::ColorConverter ().to_string (QColor ())));
    options.push_back (std::pair<std::string, std::string> (cfg_l2n_highlight_line_width, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2n_highlight_vertex_size, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2n_highlight_halo, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_l2n_highlight_dither_pattern, "-1"));
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
    menu_entries.push_back (lay::MenuEntry ("netlist_browser::show", "browse_netlists", "tools_menu.end", tl::to_string (QObject::tr ("Netlist Browser"))));
  }

  virtual lay::Plugin *create_plugin (db::Manager *, lay::PluginRoot *root, lay::LayoutView *view) const
  {
    return new lay::NetlistBrowserDialog (root, view);
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new NetlistBrowserPluginDeclaration (), 12100, "NetlistBrowserPlugin");

}



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

#include "rdbMarkerBrowserDialog.h"

#include "dbLayout.h"
#include "layConverters.h"
#include "layDispatcher.h"
#include "layUtils.h"
#include "layMargin.h"

#include "ui_MarkerBrowserConfigPage.h"
#include "ui_MarkerBrowserConfigPage2.h"

#include <set>

namespace rdb
{

// ------------------------------------------------------------
//  Declaration of the configuration options

std::string cfg_rdb_context_mode ("rdb-context-mode");
std::string cfg_rdb_show_all ("rdb-show-all");
std::string cfg_rdb_list_shapes ("rdb-list-shapes");
std::string cfg_rdb_window_state ("rdb-window-state-v2");  // v2: 0.24++
std::string cfg_rdb_window_mode ("rdb-window-mode");
std::string cfg_rdb_window_dim ("rdb-window-dim");
std::string cfg_rdb_max_marker_count ("rdb-max-marker-count");
std::string cfg_rdb_marker_color ("rdb-marker-color");
std::string cfg_rdb_marker_line_width ("rdb-marker-line-width");
std::string cfg_rdb_marker_vertex_size ("rdb-marker-vertex-size");
std::string cfg_rdb_marker_halo ("rdb-marker-halo");
std::string cfg_rdb_marker_dither_pattern ("rdb-marker-dither-pattern");

// ------------------------------------------------------------

static struct {
  rdb::context_mode_type mode;
  const char *string;
} context_modes [] = {
  { rdb::AnyCell,       "any-cell"     },
  { rdb::DatabaseTop,   "database-top" },
  { rdb::Current,       "current-cell" },
  { rdb::CurrentOrAny,  "current-or-any-cell" },
  { rdb::Local,         "local-cell"   },
};

void
MarkerBrowserContextModeConverter::from_string (const std::string &value, rdb::context_mode_type &mode)
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
MarkerBrowserContextModeConverter::to_string (rdb::context_mode_type mode)
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
  rdb::window_type mode;
  const char *string;
} window_modes [] = {
  { rdb::DontChange,    "dont-change" },
  { rdb::FitCell,       "fit-cell"    },
  { rdb::FitMarker,     "fit-marker"  },
  { rdb::Center,        "center"      },
  { rdb::CenterSize,    "center-size" }
};

void
MarkerBrowserWindowModeConverter::from_string (const std::string &value, rdb::window_type &mode)
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
MarkerBrowserWindowModeConverter::to_string (rdb::window_type mode)
{
  for (unsigned int i = 0; i < sizeof (window_modes) / sizeof (window_modes [0]); ++i) {
    if (mode == window_modes [i].mode) {
      return window_modes [i].string;
    }
  }
  return "";
}


// ------------------------------------------------------------
//  Implementation of MarkerBrowserConfigPage

MarkerBrowserConfigPage::MarkerBrowserConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::MarkerBrowserConfigPage ();
  mp_ui->setupUi (this);

  connect (mp_ui->cbx_window, SIGNAL (currentIndexChanged (int)), this, SLOT (window_changed (int)));
}

MarkerBrowserConfigPage::~MarkerBrowserConfigPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
MarkerBrowserConfigPage::setup (lay::Dispatcher *root)
{
  //  context mode
  rdb::context_mode_type cmode = rdb::DatabaseTop;
  root->config_get (cfg_rdb_context_mode, cmode, MarkerBrowserContextModeConverter ());
  mp_ui->cbx_context->setCurrentIndex (int (cmode));

  //  window mode
  rdb::window_type wmode = rdb::FitMarker;
  root->config_get (cfg_rdb_window_mode, wmode, MarkerBrowserWindowModeConverter ());
  mp_ui->cbx_window->setCurrentIndex (int (wmode));

  //  window dimension
  lay::Margin wdim (1.0);
  std::string wdim_str = wdim.to_string ();
  root->config_get (cfg_rdb_window_dim, wdim_str);
  wdim = lay::Margin::from_string (wdim_str);
  mp_ui->mgn_window->set_margin (wdim);
    
  //  max. marker count
  unsigned int max_marker_count = 1000;
  root->config_get (cfg_rdb_max_marker_count, max_marker_count);
  mp_ui->le_max_markers->setText (tl::to_qstring (tl::to_string (max_marker_count)));

  //  enable controls
  window_changed (int (wmode));
}

void
MarkerBrowserConfigPage::window_changed (int m)
{
  mp_ui->mgn_window->setEnabled (m == int (rdb::FitMarker) || m == int (rdb::CenterSize));
}

void 
MarkerBrowserConfigPage::commit (lay::Dispatcher *root)
{
  unsigned int max_markers_count = 1000;
  tl::from_string_ext (tl::to_string (mp_ui->le_max_markers->text ()), max_markers_count);

  root->config_set (cfg_rdb_context_mode, rdb::context_mode_type (mp_ui->cbx_context->currentIndex ()), MarkerBrowserContextModeConverter ());
  root->config_set (cfg_rdb_window_mode, rdb::window_type (mp_ui->cbx_window->currentIndex ()), MarkerBrowserWindowModeConverter ());
  root->config_set (cfg_rdb_window_dim, mp_ui->mgn_window->get_margin ().to_string ());
  root->config_set (cfg_rdb_max_marker_count, max_markers_count);
}

// ------------------------------------------------------------
//  Implementation of MarkerBrowserConfigPage2

MarkerBrowserConfigPage2::MarkerBrowserConfigPage2 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::MarkerBrowserConfigPage2 ();
  mp_ui->setupUi (this);
}

MarkerBrowserConfigPage2::~MarkerBrowserConfigPage2 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
MarkerBrowserConfigPage2::setup (lay::Dispatcher *root)
{
  //  marker color
  QColor color;
  root->config_get (cfg_rdb_marker_color, color, lay::ColorConverter ());
  mp_ui->color_pb->set_color (color);

  //  marker line width
  int lw = 0;
  root->config_get (cfg_rdb_marker_line_width, lw);
  if (lw < 0) {
    mp_ui->lw_le->setText (QString ());
  } else {
    mp_ui->lw_le->setText (tl::to_qstring (tl::to_string (lw)));
  }

  //  marker vertex size
  int vs = 0;
  root->config_get (cfg_rdb_marker_vertex_size, vs);
  if (vs < 0) {
    mp_ui->vs_le->setText (QString ());
  } else {
    mp_ui->vs_le->setText (tl::to_qstring (tl::to_string (vs)));
  }

  //  stipple pattern
  int dp = 0;
  root->config_get (cfg_rdb_marker_dither_pattern, dp);
  mp_ui->stipple_pb->set_dither_pattern (dp);

  //  halo
  int halo = 0;
  root->config_get (cfg_rdb_marker_halo, halo);
  mp_ui->halo_cb->setCheckState (halo < 0 ? Qt::PartiallyChecked : (halo ? Qt::Checked : Qt::Unchecked));
}

void 
MarkerBrowserConfigPage2::commit (lay::Dispatcher *root)
{
  QColor color (mp_ui->color_pb->get_color ());
  root->config_set (cfg_rdb_marker_color, color, lay::ColorConverter ());

  if (mp_ui->lw_le->text ().isEmpty ()) {
    root->config_set (cfg_rdb_marker_line_width, -1);
  } else {
    try {
      int s;
      tl::from_string_ext (tl::to_string (mp_ui->lw_le->text ()), s);
      root->config_set (cfg_rdb_marker_line_width, s);
    } catch (...) { }
  }

  if (mp_ui->vs_le->text ().isEmpty ()) {
    root->config_set (cfg_rdb_marker_vertex_size, -1);
  } else {
    try {
      int s;
      tl::from_string_ext (tl::to_string (mp_ui->vs_le->text ()), s);
      root->config_set (cfg_rdb_marker_vertex_size, s);
    } catch (...) { }
  }

  root->config_set (cfg_rdb_marker_dither_pattern, mp_ui->stipple_pb->dither_pattern ());

  if (mp_ui->halo_cb->checkState () == Qt::PartiallyChecked) {
    root->config_set (cfg_rdb_marker_halo, -1);
  } else if (mp_ui->halo_cb->checkState () == Qt::Unchecked) {
    root->config_set (cfg_rdb_marker_halo, 0);
  } else if (mp_ui->halo_cb->checkState () == Qt::Checked) {
    root->config_set (cfg_rdb_marker_halo, 1);
  }
}

// ------------------------------------------------------------
//  Declaration and implementation of the browser plugin declaration object 

class MarkerBrowserPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_rdb_context_mode, "database-top"));
    options.push_back (std::pair<std::string, std::string> (cfg_rdb_window_mode, "fit-marker"));
    options.push_back (std::pair<std::string, std::string> (cfg_rdb_window_state, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_rdb_window_dim, "1.0"));
    options.push_back (std::pair<std::string, std::string> (cfg_rdb_max_marker_count, "1000"));
    options.push_back (std::pair<std::string, std::string> (cfg_rdb_marker_color, lay::ColorConverter ().to_string (QColor ())));
    options.push_back (std::pair<std::string, std::string> (cfg_rdb_marker_line_width, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_rdb_marker_vertex_size, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_rdb_marker_halo, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_rdb_marker_dither_pattern, "-1"));
  }

  virtual std::vector<std::pair <std::string, lay::ConfigPage *> > config_pages (QWidget *parent) const 
  {
    std::vector<std::pair <std::string, lay::ConfigPage *> > pages;
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Marker Database Browser|Setup")), new MarkerBrowserConfigPage (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Marker Database Browser|Marker Appearance")), new MarkerBrowserConfigPage2 (parent)));
    return pages;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::menu_item ("marker_browser::show", "browse_markers", "tools_menu.end", tl::to_string (QObject::tr ("Marker Browser"))));
    menu_entries.push_back (lay::submenu ("marker_browser::show", "shapes_to_markers", "tools_menu.end", tl::to_string (QObject::tr ("Shapes To Markers"))));
    menu_entries.push_back (lay::menu_item ("marker_browser::scan_layers", "scan_layers", "tools_menu.shapes_to_markers.end", tl::to_string (QObject::tr ("Hierarchical"))));
    menu_entries.push_back (lay::menu_item ("marker_browser::scan_layers_flat", "scan_layers_flat", "tools_menu.shapes_to_markers.end", tl::to_string (QObject::tr ("Flat"))));
  }

  virtual lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  {
    if (lay::has_gui ()) {
      return new rdb::MarkerBrowserDialog (root, view);
    } else {
      return 0;
    }
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new MarkerBrowserPluginDeclaration (), 12000, "MarkerBrowserPlugin");

}

#endif

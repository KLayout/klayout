
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

#if defined(HAVE_QT)

#ifndef HDR_rdbMarkerBrowserDialog
#define HDR_rdbMarkerBrowserDialog

#include "layuiCommon.h"
#include "layBrowser.h"
#include "layMargin.h"
#include "tlColor.h"
#include "rdbMarkerBrowser.h"

namespace Ui
{
  class MarkerBrowserDialog;
}

namespace db
{
  class Layout;
}

namespace rdb
{

class Database;

class LAYUI_PUBLIC MarkerBrowserDialog
  : public lay::Browser
{
  Q_OBJECT

public:
  MarkerBrowserDialog (lay::Dispatcher *root, lay::LayoutViewBase *view);
  ~MarkerBrowserDialog ();

  void load (int rdb_index, int cv_index);

private:
  //  implementation of the lay::Browser interface
  virtual void activated ();
  virtual void deactivated ();

  bool configure (const std::string &name, const std::string &value);

  //  implementation of the lay::Plugin interface
  virtual void menu_activated (const std::string &symbol);

  void cellviews_changed ();
  void cellview_changed (int index);
  void rdbs_changed ();

public slots:
  void cv_index_changed (int);
  void rdb_index_changed (int);
  void info_clicked ();
  void save_clicked ();
  void saveas_clicked ();
  void saveas_waiver_db_clicked ();
  void apply_waiver_db_clicked ();
  void export_clicked ();
  void reload_clicked ();
  void open_clicked ();
  void unload_clicked ();
  void unload_all_clicked ();
  void configure_clicked ();

private:
  Ui::MarkerBrowserDialog *mp_ui;
  context_mode_type m_context;
  window_type m_window;
  lay::Margin m_window_dim;
  unsigned int m_max_marker_count;
  tl::Color m_marker_color;
  int m_marker_line_width;
  int m_marker_vertex_size;
  int m_marker_halo;
  int m_marker_dither_pattern;
  std::string m_layout_name;
  int m_cv_index;
  std::string m_rdb_name;
  int m_rdb_index;
  std::string m_open_filename;

  void update_content ();
  void scan_layer ();
  void scan_layer_flat ();
  void scan_layer_flat_or_hierarchical (bool flat);
};

}

#endif

#endif  //  defined(HAVE_QT)

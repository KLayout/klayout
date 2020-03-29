
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#ifndef HDR_rdbMarkerBrowserDialog
#define HDR_rdbMarkerBrowserDialog

#include "ui_MarkerBrowserDialog.h"
#include "layBrowser.h"
#include "rdbMarkerBrowser.h"

namespace rdb
{

class MarkerBrowserDialog
  : public lay::Browser,
    private Ui::MarkerBrowserDialog
{
  Q_OBJECT

public:
  MarkerBrowserDialog (lay::Dispatcher *root, lay::LayoutView *view);
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
  void saveas_clicked ();
  void export_clicked ();
  void reload_clicked ();
  void open_clicked ();
  void unload_clicked ();
  void unload_all_clicked ();
  void configure_clicked ();

private:
  context_mode_type m_context;
  window_type m_window;
  double m_window_dim;
  unsigned int m_max_marker_count;
  QColor m_marker_color;
  int m_marker_line_width;
  int m_marker_vertex_size;
  int m_marker_halo;
  int m_marker_dither_pattern;
  std::string m_layout_name;
  int m_cv_index;
  std::string m_rdb_name;
  int m_rdb_index;
  std::string m_open_filename;
  QAction *m_open_action;
  QAction *m_saveas_action;
  QAction *m_export_action;
  QAction *m_unload_action;
  QAction *m_unload_all_action;
  QAction *m_reload_action;

  void update_content ();
  void scan_layer ();
  void scan_layer_flat ();
};

}

#endif


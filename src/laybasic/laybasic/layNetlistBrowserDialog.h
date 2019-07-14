
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


#ifndef HDR_layNetlistBrowserDialog
#define HDR_layNetlistBrowserDialog

#include "ui_NetlistBrowserDialog.h"
#include "layBrowser.h"
#include "layNetlistBrowser.h"
#include "layViewObject.h"
#include "layColorPalette.h"

namespace lay
{

class NetlistBrowserDialog
  : public lay::Browser,
    public lay::ViewService,
    private Ui::NetlistBrowserDialog
{
  Q_OBJECT

public:
  NetlistBrowserDialog (lay::PluginRoot *root, lay::LayoutView *view);
  ~NetlistBrowserDialog ();

  void load (int lay_index, int cv_index);

private:
  //  implementation of the lay::Browser interface
  virtual void activated ();
  virtual void deactivated ();

  virtual bool configure (const std::string &name, const std::string &value);

  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual lay::ViewService *view_service_interface ();

  //  implementation of the lay::Plugin interface
  virtual void menu_activated (const std::string &symbol);

  void cellviews_changed ();
  void cellview_changed (int index);
  void l2ndbs_changed ();

public slots:
  void cv_index_changed (int);
  void l2ndb_index_changed (int);
  void saveas_clicked ();
  void export_clicked ();
  void reload_clicked ();
  void open_clicked ();
  void unload_clicked ();
  void unload_all_clicked ();
  void configure_clicked ();
  void probe_button_pressed ();
  void sticky_mode_clicked ();

private:
  lay::NetlistBrowserConfig::net_window_type m_window;
  double m_window_dim;
  unsigned int m_max_shape_count;
  QColor m_marker_color;
  lay::ColorPalette m_auto_colors;
  bool m_auto_color_enabled;
  int m_marker_line_width;
  int m_marker_vertex_size;
  int m_marker_halo;
  int m_marker_dither_pattern;
  int m_marker_intensity;
  bool m_use_original_colors;
  std::string m_layout_name;
  int m_cv_index;
  std::string m_l2ndb_name;
  int m_l2n_index;
  std::string m_open_filename;
  db::DPoint m_mouse_first_point;
  int m_mouse_state;
  QAction *m_open_action;
  QAction *m_saveas_action;
  QAction *m_export_action;
  QAction *m_unload_action;
  QAction *m_unload_all_action;
  QAction *m_reload_action;

  void update_content ();
  void release_mouse ();
  void probe_net (const db::DPoint &p, bool trace_path);
};

}

#endif


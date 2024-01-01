
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



#ifndef HDR_layNetTracerDialog
#define HDR_layNetTracerDialog

#include "ui_NetTracerDialog.h"

#include "dbNetTracer.h"
#include "dbTechnology.h"
#include "dbLayoutToNetlist.h"

#include "layNetTracerConfig.h"
#include "layBrowser.h"
#include "layPlugin.h"
#include "layViewObject.h"
#include "layMarker.h"

namespace db
{
  class NetTracerNet;
  class LayoutToNetlist;
}

namespace lay
{

class FileDialog;
class CellView;

class NetTracerDialog
  : public lay::Browser,
    public Ui::NetTracerDialog,
    public lay::ViewService
{
Q_OBJECT

public:
  NetTracerDialog (lay::Dispatcher *root, lay::LayoutViewBase *view);
  virtual ~NetTracerDialog ();

  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual void menu_activated (const std::string &symbol);
  virtual lay::ViewService *view_service_interface ();
  virtual void deactivated ();
  virtual void activated ();
  virtual bool configure (const std::string &name, const std::string &value);

protected slots:
  void net_color_changed (QColor);
  void trace_net_button_clicked ();
  void trace_path_button_clicked ();
  void delete_button_clicked ();
  void clear_all_button_clicked ();
  void layer_stack_clicked ();
  void export_clicked ();
  void export_text_clicked ();
  void detailed_mode_clicked ();
  void sticky_mode_clicked ();
  void configure_clicked ();
  void item_selection_changed ();
  void item_double_clicked (QListWidgetItem *item);
  void redo_trace_clicked ();

private:
  std::vector <db::NetTracerNet *> mp_nets;
  std::vector <lay::ShapeMarker *> mp_markers;
  unsigned int m_cv_index;
  int m_net_index;
  nt_window_type m_window;
  double m_window_dim;
  unsigned int m_max_marker_count;
  tl::Color m_marker_color;
  int m_marker_line_width;
  int m_marker_vertex_size;
  int m_marker_halo;
  int m_marker_dither_pattern;
  int m_marker_intensity;
  bool m_auto_color_enabled;
  lay::ColorPalette m_auto_colors;
  int m_auto_color_index;

  db::DPoint m_mouse_first_point;
  int m_mouse_state;
  std::string m_export_cell_name;
  lay::FileDialog *mp_export_file_dialog;
  std::string m_export_file_name;

  lay::LayoutViewBase *mp_view;

  void commit ();
  size_t get_trace_depth ();
  void attach_events ();
  void update_list_of_stacks_with_technology (db::Technology *);
  void update_list_of_stacks_with_cellview (int);
  void update_list_of_stacks ();

  void update_highlights ();
  void adjust_view ();
  void clear_markers ();
  void clear_nets ();
  void update_list ();
  void update_info ();
  void layer_list_changed (int index);
  void release_mouse ();
  db::NetTracerNet *do_trace (const db::DBox &start_search_box, const db::DBox &stop_search_box, bool trace_path);
  bool get_net_tracer_setup (const lay::CellView &cv, db::NetTracerData &data);
  static bool get_net_tracer_setup_from_tech (const std::string &tech_name, const std::string &stack_name, const db::Layout &layout, db::NetTracerData &data);
  void trace_all_nets (db::LayoutToNetlist *l2ndb, const lay::CellView &cv, bool flat);

  lay::LayoutViewBase *view ()
  {
    return mp_view;
  }
};

}

#endif


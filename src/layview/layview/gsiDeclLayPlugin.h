
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

#ifndef _HDR_gsiDeclLayPlugin
#define _HDR_gsiDeclLayPlugin

#include "gsiDecl.h"
#include "gsiDeclBasic.h"

#include "layEditorServiceBase.h"
#include "layLayoutViewBase.h"

namespace gsi
{

class PluginImpl
  : public lay::EditorServiceBase
{
public:
  PluginImpl ();

  void init (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);
  void grab_mouse ();
  void ungrab_mouse ();
  void set_cursor (int c);

  virtual void menu_activated (const std::string &symbol);
  db::DPoint snap (db::DPoint p) const;
  db::DVector snap_vector (db::DVector v) const;
  db::DPoint snap_from_to (const db::DPoint &p, const db::DPoint &plast, bool connect, lay::angle_constraint_type ac) const;
  db::DVector snap_delta (const db::DVector &v, bool connect, lay::angle_constraint_type ac) const;
  db::DPoint snap2 (const db::DPoint &p, bool visualize);
  db::DPoint snap2_from_to (const db::DPoint &p, const db::DPoint &plast, bool connect, lay::angle_constraint_type ac, bool visualize);

  //  Captures some edt space configuration events for convencience
  void configure_edt (const std::string &name, const std::string &value);
  //  NOTE: The implementation does not allow to bypass the base class configuration call
  bool configure_impl (const std::string &name, const std::string &value);
  //  for testing
  void configure_test (const std::string &name, const std::string &value);
  virtual bool configure (const std::string &name, const std::string &value);
  //  NOTE: The implementation does not allow to bypass the base class configuration call
  virtual void config_finalize_impl ();
  virtual void config_finalize ();
  virtual bool key_event (unsigned int key, unsigned int buttons);
  virtual bool shortcut_override_event (unsigned int key, unsigned int buttons);
  virtual bool mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio) ;
  bool mouse_press_event_noref (db::DPoint p, unsigned int buttons, bool prio);
  virtual bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  bool mouse_click_event_noref (db::DPoint p, unsigned int buttons, bool prio);
  virtual bool mouse_double_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  bool mouse_double_click_event_noref (db::DPoint p, unsigned int buttons, bool prio);
  virtual bool leave_event (bool prio);
  virtual bool enter_event (bool prio);
  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);
  bool mouse_move_event_noref (db::DPoint p, unsigned int buttons, bool prio);
  virtual bool mouse_release_event (const db::DPoint &p, unsigned int buttons, bool prio);
  bool mouse_release_event_noref (db::DPoint p, unsigned int buttons, bool prio);
  virtual bool wheel_event (int delta, bool horizontal, const db::DPoint &p, unsigned int buttons, bool prio);
  bool wheel_event_noref (int delta, bool horizontal, db::DPoint p, unsigned int buttons, bool prio);
  void activated_impl ();
  virtual void activated ();
  void deactivated_impl ();
  virtual void deactivated ();
  virtual void drag_cancel ();
  virtual void update ();
  void add_mouse_cursor_dpoint (const db::DPoint &p, bool emphasize);
  void add_mouse_cursor_point (const db::Point &p, int cv_index, const db::LayerProperties &lp, bool emphasize);
  void add_edge_marker_dedge (const db::DEdge &p, bool emphasize);
  void add_edge_marker_edge (const db::Edge &p, int cv_index, const db::LayerProperties &lp, bool emphasize);

  //  for testing
  bool has_tracking_position_test () const;
  virtual bool has_tracking_position () const;

  //  for testing
  db::DPoint tracking_position_test () const;
  virtual db::DPoint tracking_position () const;

  virtual int focus_page_open ();

  virtual lay::ViewService *view_service_interface ()
  {
    return this;
  }

  lay::LayoutViewBase *view () const
  {
    return const_cast<lay::LayoutViewBase *> (mp_view.get ());
  }

  lay::Dispatcher *dispatcher () const
  {
    return const_cast<lay::Dispatcher *> (mp_dispatcher.get ());
  }

  gsi::Callback f_menu_activated;
  gsi::Callback f_configure;
  gsi::Callback f_config_finalize;
  gsi::Callback f_key_event;
  gsi::Callback f_shortcut_override_event;
  gsi::Callback f_mouse_press_event;
  gsi::Callback f_mouse_click_event;
  gsi::Callback f_mouse_double_click_event;
  gsi::Callback f_leave_event;
  gsi::Callback f_enter_event;
  gsi::Callback f_mouse_move_event;
  gsi::Callback f_mouse_release_event;
  gsi::Callback f_wheel_event;
  gsi::Callback f_activated;
  gsi::Callback f_deactivated;
  gsi::Callback f_drag_cancel;
  gsi::Callback f_update;
  gsi::Callback f_has_tracking_position;
  gsi::Callback f_tracking_position;
  gsi::Callback f_focus_page_open;

private:
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
  tl::weak_ptr<lay::Dispatcher> mp_dispatcher;

  //  Angle constraints and grids
  lay::angle_constraint_type m_connect_ac, m_move_ac;
  db::DVector m_edit_grid;
  bool m_snap_to_objects;
  bool m_snap_objects_to_grid;
  db::DVector m_global_grid;

  lay::angle_constraint_type connect_ac (lay::angle_constraint_type ac) const;
  lay::angle_constraint_type move_ac (lay::angle_constraint_type ac) const;
};

}

#endif


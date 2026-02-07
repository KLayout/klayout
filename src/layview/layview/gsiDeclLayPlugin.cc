
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "gsiDeclLayPlugin.h"

#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "gsiEnums.h"
#include "layCursor.h"
#include "layEditorUtils.h"
#include "layEditorOptionsPageWidget.h"
#include "layConverters.h"

namespace gsi
{

static bool has_tracking_position_impl (const lay::EditorServiceBase *p)
{
  return p->lay::EditorServiceBase::has_tracking_position ();
}

static db::DPoint tracking_position_impl (const lay::EditorServiceBase *p)
{
  return p->lay::EditorServiceBase::tracking_position ();
}

static void menu_activated_impl (lay::EditorServiceBase *p, const std::string &symbol)
{
  return p->lay::EditorServiceBase::menu_activated (symbol);
}

static bool configure_impl (lay::EditorServiceBase *p, const std::string &name, const std::string &value)
{
  return p->lay::EditorServiceBase::configure (name, value);
}

static void config_finalize_impl (lay::EditorServiceBase *p)
{
  p->lay::EditorServiceBase::config_finalize ();
}

static void deactivated_impl (lay::EditorServiceBase *p)
{
  p->lay::EditorServiceBase::deactivated ();
}

static void activated_impl (lay::EditorServiceBase *p)
{
  p->lay::EditorServiceBase::activated ();
}

static bool key_event_impl (lay::EditorServiceBase *p, unsigned int key, unsigned int buttons)
{
  return p->lay::EditorServiceBase::key_event (key, buttons);
}

static bool mouse_press_event_impl (lay::EditorServiceBase *p, const db::DPoint &pt, unsigned int buttons, bool prio)
{
  return p->lay::EditorServiceBase::mouse_press_event (pt, buttons, prio);
}

static bool mouse_click_event_impl (lay::EditorServiceBase *p, const db::DPoint &pt, unsigned int buttons, bool prio)
{
  return p->lay::EditorServiceBase::mouse_click_event (pt, buttons, prio);
}

static bool mouse_double_click_event_impl (lay::EditorServiceBase *p, const db::DPoint &pt, unsigned int buttons, bool prio)
{
  return p->lay::EditorServiceBase::mouse_double_click_event (pt, buttons, prio);
}

static bool leave_event_impl (lay::EditorServiceBase *p, bool prio)
{
  return p->lay::EditorServiceBase::leave_event (prio);
}

static bool enter_event_impl (lay::EditorServiceBase *p, bool prio)
{
  return p->lay::EditorServiceBase::enter_event (prio);
}

static bool mouse_move_event_impl (lay::EditorServiceBase *p, const db::DPoint &pt, unsigned int buttons, bool prio)
{
  return p->lay::EditorServiceBase::mouse_move_event (pt, buttons, prio);
}

static bool mouse_release_event_impl (lay::EditorServiceBase *p, const db::DPoint &pt, unsigned int buttons, bool prio)
{
  return p->lay::EditorServiceBase::mouse_release_event (pt, buttons, prio);
}

static bool wheel_event_impl (lay::EditorServiceBase *p, int delta, bool horizontal, const db::DPoint &pt, unsigned int buttons, bool prio)
{
  return p->lay::EditorServiceBase::wheel_event (delta, horizontal, pt, buttons, prio);
}

static void update_impl (lay::EditorServiceBase *p)
{
  p->lay::EditorServiceBase::update ();
}

static void drag_cancel_impl (lay::EditorServiceBase *p)
{
  p->lay::EditorServiceBase::drag_cancel ();
}

Class<lay::EditorServiceBase> decl_PluginBase ("lay", "PluginBase",
  gsi::method_ext ("tracking_position", &tracking_position_impl,
    "@brief Gets the tracking position (base class implementation)\n"
    "See \\Plugin#tracking_position for details."
  ) +
  gsi::method_ext ("has_tracking_position", &has_tracking_position_impl,
    "@brief Gets a value indicating whether the plugin provides a tracking position (base class implementation)\n"
    "See \\Plugin#has_tracking_position for details."
  ) +
  gsi::method_ext ("menu_activated", &menu_activated_impl, gsi::arg ("symbol"),
    "@brief Gets called when a custom menu item is selected (base class implementation)\n"
    "See \\Plugin#menu_activated for details."
  ) +
  gsi::method_ext ("configure", &configure_impl, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Sends configuration requests to the plugin (base class implementation)\n"
    "See \\Plugin#configure for details."
  ) +
  gsi::method_ext ("config_finalize", &config_finalize_impl,
    "@brief Sends the post-configuration request to the plugin (base class implementation)\n"
    "See \\Plugin#config_finalize for details."
  ) +
  gsi::method_ext ("key_event", &key_event_impl, gsi::arg ("key"), gsi::arg ("buttons"),
    "@brief Handles the key pressed event (base class implementation)\n"
    "See \\Plugin#key_event for details."
  ) +
  gsi::method_ext ("mouse_button_pressed_event", &mouse_press_event_impl, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button pressed event (base class implementation)\n"
    "See \\Plugin#mouse_button_pressed_event for details."
  ) +
  gsi::method_ext ("mouse_click_event", &mouse_click_event_impl, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button click event after the button has been released (base class implementation)\n"
    "See \\Plugin#mouse_click_event for details."
  ) +
  gsi::method_ext ("mouse_double_click_event", &mouse_double_click_event_impl, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button double-click event (base class implementation)\n"
    "See \\Plugin#mouse_double_click_event for details."
  ) +
  gsi::method_ext ("leave_event", &leave_event_impl, gsi::arg ("prio"),
    "@brief Handles the leave event (base class implementation)\n"
    "See \\Plugin#leave_event for details."
  ) +
  gsi::method_ext ("enter_event", &enter_event_impl, gsi::arg ("prio"),
    "@brief Handles the enter event (base class implementation)\n"
    "See \\Plugin#enter_event for details."
  ) +
  gsi::method_ext ("mouse_moved_event", &mouse_move_event_impl, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse move event (base class implementation)\n"
    "See \\Plugin#mouse_moved_event for details."
  ) +
  gsi::method_ext ("mouse_button_released_event", &mouse_release_event_impl, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button release event (base class implementation)\n"
    "See \\Plugin#mouse_button_released_event for details."
  ) +
  gsi::method_ext ("wheel_event", &wheel_event_impl, gsi::arg ("delta"), gsi::arg ("horizontal"), gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse wheel event (base class implementation)\n"
    "See \\Plugin#wheel_event for details."
  ) +
  gsi::method_ext ("activated", &activated_impl,
    "@brief Gets called when the plugin is activated (base class implementation)\n"
    "See \\Plugin#activated for details."
  ) +
  gsi::method_ext ("deactivated", &deactivated_impl,
    "@brief Gets called when the plugin is deactivated and another plugin is activated (base class implementation)\n"
    "See \\Plugin#deactivated for details."
  ) +
  gsi::method_ext ("drag_cancel", &drag_cancel_impl,
    "@brief This method is called when some mouse dragging operation should be cancelled (base class implementation)\n"
    "See \\Plugin#drag_cancel for details."
  ) +
  gsi::method_ext ("update", &update_impl,
    "@brief Gets called when the view has changed (base class implementation)\n"
    "See \\Plugin#update for details."
  ),
  "@brief The plugin base class\n"
  "\n"
  "This class is provided as an interface to the base class implementation for various functions.\n"
  "You can use these methods in order to pass down events to the original implementation.\n"
  "\n"
  "This class has been introduced in version 0.30.4.\n"
);


//  HACK: used to track if we're inside a create_plugin method and can be sure that "init" is called
bool s_in_create_plugin = false;

PluginImpl::PluginImpl ()
  : lay::EditorServiceBase (),
    mp_view (0), mp_dispatcher (0),
    m_connect_ac (lay::AC_Any), m_move_ac (lay::AC_Any),
    m_snap_to_objects (true),
    m_snap_objects_to_grid (true)
{
  if (! s_in_create_plugin) {
    throw tl::Exception (tl::to_string (tr ("A PluginBase object can only be created in the PluginFactory's create_plugin method")));
  }
}

void
PluginImpl::init (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
{
  mp_view = view;
  mp_dispatcher = dispatcher;
  lay::EditorServiceBase::init (view);
}

void
PluginImpl::grab_mouse ()
{
  if (ui ()) {
    ui ()->grab_mouse (this, false);
  }
}

void
PluginImpl::ungrab_mouse ()
{
  if (ui ()) {
    ui ()->ungrab_mouse (this);
  }
}

void
PluginImpl::set_cursor (int c)
{
  if (ui ()) {
    lay::ViewService::set_cursor ((enum lay::Cursor::cursor_shape) c);
  }
}

void
PluginImpl::menu_activated (const std::string &symbol)
{
  if (f_menu_activated.can_issue ()) {
    f_menu_activated.issue<lay::EditorServiceBase, const std::string &> (&lay::EditorServiceBase::menu_activated, symbol);
  } else {
    lay::EditorServiceBase::menu_activated (symbol);
  }
}

db::DPoint
PluginImpl::snap (db::DPoint p) const
{
  //  snap according to the grid
  if (m_edit_grid == db::DVector ()) {
    p = lay::snap_xy (p, m_global_grid);
  } else if (m_edit_grid.x () < 1e-6) {
    ; //  nothing
  } else {
    p = lay::snap_xy (p, m_edit_grid);
  }

  return p;
}

db::DVector
PluginImpl::snap_vector (db::DVector v) const
{
  //  snap according to the grid
  if (m_edit_grid == db::DVector ()) {
    v = lay::snap_xy (db::DPoint () + v, m_global_grid) - db::DPoint ();
  } else if (m_edit_grid.x () < 1e-6) {
    ; //  nothing
  } else {
    v = lay::snap_xy (db::DPoint () + v, m_edit_grid) - db::DPoint ();
  }

  return v;
}

db::DPoint
PluginImpl::snap_from_to (const db::DPoint &p, const db::DPoint &plast, bool connect, lay::angle_constraint_type ac) const
{
  db::DPoint ps = plast + lay::snap_angle (db::DVector (p - plast), connect ? connect_ac (ac) : move_ac (ac));
  return snap (ps);
}

db::DVector
PluginImpl::snap_delta (const db::DVector &v, bool connect, lay::angle_constraint_type ac) const
{
  return snap_vector (lay::snap_angle (v, connect ? connect_ac (ac) : move_ac (ac)));
}

db::DPoint
PluginImpl::snap2 (const db::DPoint &p, bool visualize)
{
  double snap_range = ui ()->mouse_event_trans ().inverted ().ctrans (lay::snap_range_pixels ());
  auto details = lay::obj_snap (m_snap_to_objects ? view () : 0, p, m_edit_grid == db::DVector () ? m_global_grid : m_edit_grid, snap_range);
  if (visualize) {
    mouse_cursor_from_snap_details (details);
  }
  return details.snapped_point;
}

db::DPoint
PluginImpl::snap2_from_to (const db::DPoint &p, const db::DPoint &plast, bool connect, lay::angle_constraint_type ac, bool visualize)
{
  double snap_range = ui ()->mouse_event_trans ().inverted ().ctrans (lay::snap_range_pixels ());
  auto details = lay::obj_snap (m_snap_to_objects ? view () : 0, plast, p, m_edit_grid == db::DVector () ? m_global_grid : m_edit_grid, connect ? connect_ac (ac) : move_ac (ac), snap_range);
  if (visualize) {
    mouse_cursor_from_snap_details (details);
  }
  return details.snapped_point;
}

namespace edt
{

//  This is a replication of the codes from edtConfig.cc, but avoids
//  linking laybasic to edt module.
static std::string cfg_edit_grid ("edit-grid");
static std::string cfg_edit_global_grid ("grid-micron");
static std::string cfg_edit_snap_to_objects ("edit-snap-to-objects");
static std::string cfg_edit_snap_objects_to_grid ("edit-snap-objects-to-grid");
static std::string cfg_edit_move_angle_mode ("edit-move-angle-mode");
static std::string cfg_edit_connect_angle_mode ("edit-connect-angle-mode");

}

/**
 *  @brief Captures some edt space configuration events for convencience
 */
void
PluginImpl::configure_edt (const std::string &name, const std::string &value)
{
  lay::EditGridConverter egc;
  lay::ACConverter acc;

  if (name == edt::cfg_edit_global_grid) {
    egc.from_string (value, m_global_grid);
  } else if (name == edt::cfg_edit_grid) {
    egc.from_string (value, m_edit_grid);
  } else if (name == edt::cfg_edit_snap_to_objects) {
    tl::from_string (value, m_snap_to_objects);
  } else if (name == edt::cfg_edit_snap_objects_to_grid) {
    tl::from_string (value, m_snap_objects_to_grid);
  } else if (name == edt::cfg_edit_move_angle_mode) {
    acc.from_string (value, m_move_ac);
  } else if (name == edt::cfg_edit_connect_angle_mode) {
    acc.from_string (value, m_connect_ac);
  } else {
    lay::EditorServiceBase::configure (name, value);
  }
}

/**
 *  @brief The implementation does not allow to bypass the base class configuration call
 */
bool
PluginImpl::configure_impl (const std::string &name, const std::string &value)
{
  return f_configure.can_issue () ? f_configure.issue<PluginImpl, bool, const std::string &, const std::string &> (&PluginImpl::configure, name, value) : lay::EditorServiceBase::configure (name, value);
}

//  for testing
void
PluginImpl::configure_test (const std::string &name, const std::string &value)
{
  configure_edt (name, value);
}

bool
PluginImpl::configure (const std::string &name, const std::string &value)
{
  configure_edt (name, value);
  return configure_impl (name, value);
}

/**
 *  @brief The implementation does not allow to bypass the base class configuration call
 */
void
PluginImpl::config_finalize_impl ()
{
  f_config_finalize.can_issue () ? f_config_finalize.issue<PluginImpl> (&PluginImpl::config_finalize) : lay::EditorServiceBase::config_finalize ();
}

void
PluginImpl::config_finalize ()
{
  lay::EditorServiceBase::config_finalize ();
  config_finalize_impl ();
}

bool
PluginImpl::key_event (unsigned int key, unsigned int buttons)
{
  if (f_key_event.can_issue ()) {
    return f_key_event.issue<lay::ViewService, bool, unsigned int, unsigned int> (&lay::ViewService::key_event, key, buttons);
  } else {
    return lay::EditorServiceBase::key_event (key, buttons);
  }
}

bool
PluginImpl::shortcut_override_event (unsigned int key, unsigned int buttons)
{
  if (f_shortcut_override_event.can_issue ()) {
    return f_shortcut_override_event.issue<lay::ViewService, bool, unsigned int, unsigned int> (&lay::ViewService::shortcut_override_event, key, buttons);
  } else {
    return lay::EditorServiceBase::shortcut_override_event (key, buttons);
  }
}

bool
PluginImpl::mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (f_mouse_press_event.can_issue ()) {
    return f_mouse_press_event.issue (&PluginImpl::mouse_press_event_noref, p, buttons, prio);
  } else {
    return lay::EditorServiceBase::mouse_press_event (p, buttons, prio);
  }
}

//  NOTE: this version doesn't take a point reference which allows us to store the point in script code without generating a reference
bool
PluginImpl::mouse_press_event_noref (db::DPoint p, unsigned int buttons, bool prio)
{
  return mouse_press_event (p, buttons, prio);
}

bool
PluginImpl::mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (f_mouse_click_event.can_issue ()) {
    return f_mouse_click_event.issue (&PluginImpl::mouse_click_event_noref, p, buttons, prio);
  } else {
    return lay::EditorServiceBase::mouse_click_event (p, buttons, prio);
  }
}

//  NOTE: this version doesn't take a point reference which allows us to store the point in script code without generating a reference
bool
PluginImpl::mouse_click_event_noref (db::DPoint p, unsigned int buttons, bool prio)
{
  return mouse_click_event (p, buttons, prio);
}

bool
PluginImpl::mouse_double_click_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (f_mouse_double_click_event.can_issue ()) {
    return f_mouse_double_click_event.issue (&PluginImpl::mouse_double_click_event_noref, p, buttons, prio);
  } else {
    return lay::EditorServiceBase::mouse_double_click_event (p, buttons, prio);
  }
}

//  NOTE: this version doesn't take a point reference which allows us to store the point in script code without generating a reference
bool
PluginImpl::mouse_double_click_event_noref (db::DPoint p, unsigned int buttons, bool prio)
{
  return mouse_double_click_event (p, buttons, prio);
}

bool
PluginImpl::leave_event (bool prio)
{
  if (f_leave_event.can_issue ()) {
    return f_leave_event.issue<lay::EditorServiceBase, bool, bool> (&lay::ViewService::leave_event, prio);
  } else {
    return lay::EditorServiceBase::leave_event (prio);
  }
}

bool
PluginImpl::enter_event (bool prio)
{
  if (f_enter_event.can_issue ()) {
    return f_enter_event.issue<lay::EditorServiceBase, bool, bool> (&lay::ViewService::enter_event, prio);
  } else {
    return lay::EditorServiceBase::enter_event (prio);
  }
}

bool
PluginImpl::mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (f_mouse_move_event.can_issue ()) {
    return f_mouse_move_event.issue (&PluginImpl::mouse_move_event_noref, p, buttons, prio);
  } else {
    return lay::EditorServiceBase::mouse_move_event (p, buttons, prio);
  }
}

//  NOTE: this version doesn't take a point reference which allows us to store the point in script code without generating a reference
bool
PluginImpl::mouse_move_event_noref (db::DPoint p, unsigned int buttons, bool prio)
{
  return mouse_move_event (p, buttons, prio);
}

bool
PluginImpl::mouse_release_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (f_mouse_release_event.can_issue ()) {
    return f_mouse_release_event.issue (&PluginImpl::mouse_release_event_noref, p, buttons, prio);
  } else {
    return lay::ViewService::mouse_release_event (p, buttons, prio);
  }
}

//  NOTE: this version doesn't take a point reference which allows us to store the point in script code without generating a reference
bool
PluginImpl::mouse_release_event_noref (db::DPoint p, unsigned int buttons, bool prio)
{
  return mouse_release_event (p, buttons, prio);
}

bool
PluginImpl::wheel_event (int delta, bool horizontal, const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (f_wheel_event.can_issue ()) {
    return f_wheel_event.issue (&PluginImpl::wheel_event_noref, delta, horizontal, p, buttons, prio);
  } else {
    return lay::ViewService::wheel_event (delta, horizontal, p, buttons, prio);
  }
}

//  NOTE: this version doesn't take a point reference which allows us to store the point in script code without generating a reference
bool
PluginImpl::wheel_event_noref (int delta, bool horizontal, db::DPoint p, unsigned int buttons, bool prio)
{
  return wheel_event (delta, horizontal, p, buttons, prio);
}

void
PluginImpl::activated_impl ()
{
  if (f_activated.can_issue ()) {
    f_activated.issue<PluginImpl> (&PluginImpl::activated_impl);
  }
}

void
PluginImpl::activated ()
{
  lay::EditorServiceBase::activated ();
  activated_impl ();
}

void
PluginImpl::deactivated_impl ()
{
  if (f_deactivated.can_issue ()) {
    f_deactivated.issue<PluginImpl> (&PluginImpl::deactivated_impl);
  }
}

void
PluginImpl::deactivated ()
{
  lay::EditorServiceBase::deactivated ();
  deactivated_impl ();
}

void
PluginImpl::drag_cancel ()
{
  if (f_drag_cancel.can_issue ()) {
    f_drag_cancel.issue<lay::EditorServiceBase> (&lay::EditorServiceBase::drag_cancel);
  } else {
    lay::EditorServiceBase::drag_cancel ();
  }
}

void
PluginImpl::update ()
{
  if (f_update.can_issue ()) {
    f_update.issue<lay::EditorServiceBase> (&lay::EditorServiceBase::update);
  } else {
    lay::EditorServiceBase::update ();
  }
}

void
PluginImpl::add_mouse_cursor_dpoint (const db::DPoint &p, bool emphasize)
{
  lay::EditorServiceBase::add_mouse_cursor (p, emphasize);
}

void
PluginImpl::add_mouse_cursor_point (const db::Point &p, int cv_index, const db::LayerProperties &lp, bool emphasize)
{
  const lay::CellView &cv = view ()->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  int layer = cv->layout ().get_layer_maybe (lp);
  if (layer < 0) {
    return;
  }

  lay::TransformationVariants tv (view ());
  const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (cv_index, (unsigned int) layer);
  if (! tv_list || tv_list->empty ()) {
    return;
  }

  lay::EditorServiceBase::add_mouse_cursor (p, cv_index, cv.context_trans (), *tv_list, emphasize);
}

void
PluginImpl::add_edge_marker_dedge (const db::DEdge &p, bool emphasize)
{
  lay::EditorServiceBase::add_edge_marker (p, emphasize);
}

void
PluginImpl::add_edge_marker_edge (const db::Edge &p, int cv_index, const db::LayerProperties &lp, bool emphasize)
{
  const lay::CellView &cv = view ()->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  int layer = cv->layout ().get_layer_maybe (lp);
  if (layer < 0) {
    return;
  }

  lay::TransformationVariants tv (view ());
  const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (cv_index, (unsigned int) layer);
  if (! tv_list || tv_list->empty ()) {
    return;
  }

  lay::EditorServiceBase::add_edge_marker (p, cv_index, cv.context_trans (), *tv_list, emphasize);
}

//  for testing
bool
PluginImpl::has_tracking_position_test () const
{
  return has_tracking_position ();
}

bool
PluginImpl::has_tracking_position () const
{
  if (f_has_tracking_position.can_issue ()) {
    return f_has_tracking_position.issue<lay::EditorServiceBase, bool> (&lay::EditorServiceBase::has_tracking_position);
  } else {
    return lay::EditorServiceBase::has_tracking_position ();
  }
}

//  for testing
db::DPoint
PluginImpl::tracking_position_test () const
{
  return tracking_position ();
}

db::DPoint
PluginImpl::tracking_position () const
{
  if (f_tracking_position.can_issue ()) {
    return f_tracking_position.issue<lay::EditorServiceBase, db::DPoint> (&lay::EditorServiceBase::tracking_position);
  } else {
    return lay::EditorServiceBase::tracking_position ();
  }
}

int PluginImpl::focus_page_open ()
{
  if (f_focus_page_open.can_issue ()) {
    return f_focus_page_open.issue<lay::EditorServiceBase, int> (&lay::EditorServiceBase::focus_page_open);
  } else {
    return lay::EditorServiceBase::focus_page_open ();
  }
}

lay::angle_constraint_type
PluginImpl::connect_ac (lay::angle_constraint_type ac) const
{
  //  m_alt_ac (which is set from mouse buttons) can override the specified connect angle constraint
  return ac != lay::AC_Global ? ac : m_connect_ac;
}

lay::angle_constraint_type
PluginImpl::move_ac (lay::angle_constraint_type ac) const
{
  //  m_alt_ac (which is set from mouse buttons) can override the specified move angle constraint
  return ac != lay::AC_Global ? ac : m_move_ac;
}

#if defined(HAVE_QTBINDINGS)
static std::vector<lay::EditorOptionsPageWidget *>
get_editor_options_pages (PluginImpl *plugin)
{
  auto pages = plugin->editor_options_pages ();

  std::vector<lay::EditorOptionsPageWidget *> result;
  for (auto p = pages.begin (); p != pages.end (); ++p) {
    lay::EditorOptionsPageWidget *w = (*p)->widget ();
    if (w) {
      result.push_back (w);
    }
  }

  return result;
}

static lay::EditorOptionsPageWidget *
get_focus_page (PluginImpl *plugin)
{
  auto fp = plugin->focus_page ();
  return fp ? fp->widget () : 0;
}
#endif

Class<gsi::PluginImpl> decl_Plugin (decl_PluginBase, "lay", "Plugin",
  callback ("menu_activated", &gsi::PluginImpl::menu_activated, &gsi::PluginImpl::f_menu_activated, gsi::arg ("symbol"),
    "@brief Gets called when a custom menu item is selected\n"
    "When a menu item is clicked which was registered with the plugin factory, the plugin's 'menu_activated' method is "
    "called for the current view. The symbol registered for the menu item is passed in the 'symbol' argument."
  ) +
  method ("configure_test", &gsi::PluginImpl::configure_test, gsi::arg ("name"), gsi::arg ("value"), "@hide") +
  callback ("configure", &gsi::PluginImpl::configure_impl, &gsi::PluginImpl::f_configure, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Sends configuration requests to the plugin\n"
    "@param name The name of the configuration variable as registered in the plugin factory\n"
    "@param value The value of the configuration variable\n"
    "When a configuration variable is changed, the new value is reported to the plugin by calling the 'configure' method."
  ) +
  callback ("config_finalize", &gsi::PluginImpl::config_finalize_impl, &gsi::PluginImpl::f_config_finalize,
    "@brief Sends the post-configuration request to the plugin\n"
    "After all configuration parameters have been sent, 'config_finalize' is called to given the plugin a chance to "
    "update its internal state according to the new configuration.\n"
  ) +
  callback ("key_event", &gsi::PluginImpl::key_event, &gsi::PluginImpl::f_key_event, gsi::arg ("key"), gsi::arg ("buttons"),
    "@brief Handles the key pressed event\n"
    "This method will called by the view on the active plugin when a button is pressed on the mouse.\n"
    "\n"
    "If the plugin handles the event, it should return true to indicate that the event should not be processed further."
    "\n"
    "@param key The Qt key code of the key that was pressed\n"
    "@param buttons A combination of the constants in the \\ButtonState class which codes both the mouse buttons and the key modifiers (.e. ShiftButton etc).\n"
    "@return True to terminate dispatcher\n"
  ) +
  callback ("shortcut_override_event", &gsi::PluginImpl::shortcut_override_event, &gsi::PluginImpl::f_shortcut_override_event, gsi::arg ("key"), gsi::arg ("buttons"),
    "@brief Allows overriding keyboard shortcuts for this plugin\n"
    "If the implementation returns true, the given key is not handled by the shortcut system, but rather\n"
    "passed to 'key_event' the usual way.\n"
    "\n"
    "@param key The Qt key code of the key that was pressed\n"
    "@param buttons A combination of the constants in the \\ButtonState class which codes both the mouse buttons and the key modifiers (.e. ShiftButton etc).\n"
    "@return True to request 'key_event' handling\n"
    "\n"
    "This method has been introduced in version 0.30.5."
  ) +
  callback ("mouse_button_pressed_event", &gsi::PluginImpl::mouse_press_event_noref, &gsi::PluginImpl::f_mouse_press_event, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button pressed event\n"
    "This method will called by the view when a button is pressed on the mouse.\n"
    "\n"
    "First, the plugins that grabbed the mouse with \\grab_mouse will receive this event with 'prio' set to true "
    "in the reverse order the plugins grabbed the mouse. The loop will terminate if one of the mouse event handlers "
    "returns true.\n"
    "\n"
    "If that is not the case or no plugin has grabbed the mouse, the active plugin receives the mouse event with 'prio' set to true.\n"
    "\n"
    "If no receiver accepted the mouse event by returning true, it is sent again to all plugins with 'prio' set to false.\n"
    "Again, the loop terminates if one of the receivers returns true. The second pass gives inactive plugins a chance to monitor the mouse "
    "and implement specific actions - i.e. displaying the current position.\n"
    "\n"
    "This event is not sent immediately when the mouse button is pressed but when a signification movement for the mouse cursor away from the "
    "original position is detected. If the mouse button is released before that, a mouse_clicked_event is sent rather than a press-move-release "
    "sequence."
    "\n"
    "@param p The point at which the button was pressed\n"
    "@param buttons A combination of the constants in the \\ButtonState class which codes both the mouse buttons and the key modifiers (.e. LeftButton, ShiftButton etc).\n"
    "@return True to terminate dispatcher\n"
  ) +
  callback ("mouse_click_event", &gsi::PluginImpl::mouse_click_event_noref, &gsi::PluginImpl::f_mouse_click_event, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button click event (after the button has been released)\n"
    "The behaviour of this callback is the same than for \\mouse_button_pressed_event, except that it is called when the mouse button has been released without moving it.\n"
    "A mouse click is not defined by duration, but by releasing a button without moving the mouse after the button was pressed. "
    "As a consequence, a \\mouse_button_pressed_event is always issued at the beginning, but it is not followed by a \\mouse_button_released_event.\n"
    "Instead, the 'mouse_click_event' is issued.\n"
    "\n"
    "Starting with version 0.30.6, the button mask reflects the keyboard modifiers at the moment the mouse was released. Before, the keyboard modifiers were "
    "captured at the moment when the mouse was pressed."
  ) +
  callback ("mouse_double_click_event", &gsi::PluginImpl::mouse_double_click_event_noref, &gsi::PluginImpl::f_mouse_double_click_event, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button double-click event\n"
    "The behaviour of this callback is the same than for \\mouse_button_pressed_event, except that it is called when the mouse button has been double-clicked.\n"
  ) +
  callback ("leave_event", &gsi::PluginImpl::leave_event, &gsi::PluginImpl::f_leave_event, gsi::arg ("prio"),
    "@brief Handles the leave event (mouse leaves canvas area of view)\n"
    "The behaviour of this callback is the same than for \\mouse_button_pressed_event, except that it is called when the mouse leaves the canvas area.\n"
    "This method does not have a position nor button flags.\n"
  ) +
  callback ("enter_event", &gsi::PluginImpl::enter_event, &gsi::PluginImpl::f_enter_event, gsi::arg ("prio"),
    "@brief Handles the enter event (mouse enters canvas area of view)\n"
    "The behaviour of this callback is the same than for \\mouse_button_pressed_event, except that it is called when the mouse enters the canvas area.\n"
    "This method does not have a position nor button flags.\n"
  ) +
  callback ("mouse_moved_event", &gsi::PluginImpl::mouse_move_event_noref, &gsi::PluginImpl::f_mouse_move_event, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse move event\n"
    "The behaviour of this callback is the same than for \\mouse_button_pressed_event, except that it is called when the mouse is moved in the canvas area.\n"
    "\n"
    "The mouse move event is important for a number of background jobs, such as coordinate display in the status bar.\n"
    "Hence, you should not consume the event - i.e. you should return 'false' from this method.\n"
  ) +
  callback ("mouse_button_released_event", &gsi::PluginImpl::mouse_release_event_noref, &gsi::PluginImpl::f_mouse_release_event, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button release event\n"
    "The behaviour of this callback is the same than for \\mouse_button_pressed_event, except that it is called when the mouse button is released.\n"
    "Starting with version 0.30.6, the button mask reflects the keyboard modifiers at the moment the mouse was released. Before, the keyboard modifiers were "
    "captured at the moment when the mouse was pressed."
  ) +
  callback ("wheel_event", &gsi::PluginImpl::wheel_event_noref, &gsi::PluginImpl::f_wheel_event, gsi::arg ("delta"), gsi::arg ("horizontal"), gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse wheel event\n"
    "The behaviour of this callback is the same than for \\mouse_button_pressed_event, except that it is called when the mouse wheel is rotated.\n"
    "Additional parameters for this event are 'delta' (the rotation angle in units of 1/8th degree) and 'horizontal' which is true when the horizontal wheel was rotated and "
    "false if the vertical wheel was rotated.\n"
  ) +
  callback ("activated", &gsi::PluginImpl::activated, &gsi::PluginImpl::f_activated,
    "@brief Gets called when the plugin is activated (selected in the tool bar)\n"
  ) +
  callback ("deactivated", &gsi::PluginImpl::deactivated, &gsi::PluginImpl::f_deactivated,
    "@brief Gets called when the plugin is deactivated and another plugin is activated\n"
  ) +
  callback ("drag_cancel", &gsi::PluginImpl::drag_cancel, &gsi::PluginImpl::f_drag_cancel,
    "@brief Gets called on various occasions when some mouse drag operation should be canceled\n"
    "If the plugin implements some press-and-drag or a click-and-drag operation, this callback should "
    "cancel this operation and return to some state waiting for a new mouse event."
  ) +
  callback ("update", &gsi::PluginImpl::update, &gsi::PluginImpl::f_update,
    "@brief Gets called when the view has changed\n"
    "This method is called in particular if the view has changed the visible rectangle, i.e. after zooming in or out or panning. "
    "This callback can be used to update any internal states that depend on the view's state."
  ) + 
  method ("grab_mouse", &gsi::PluginImpl::grab_mouse,
    "@brief Redirects mouse events to this plugin, even if the plugin is not active.\n"
  ) + 
  method ("ungrab_mouse", &gsi::PluginImpl::ungrab_mouse,
    "@brief Removes a mouse grab registered with \\grab_mouse.\n"
  ) + 
  method ("set_cursor", &gsi::PluginImpl::set_cursor, gsi::arg ("cursor_type"),
    "@brief Sets the cursor in the view area to the given type\n"
    "Setting the cursor has an effect only inside event handlers, i.e. \\mouse_button_pressed_event. The cursor is not set permanently. Is is reset "
    "in the mouse move handler unless a button is pressed or the cursor is explicitly set again in \\mouse_moved_event.\n"
    "\n"
    "The cursor type is one of the cursor constants in the \\Cursor class, i.e. 'CursorArrow' for the normal cursor."
  ) +
  method ("has_tracking_position_test", &gsi::PluginImpl::has_tracking_position_test, "@hide") +
  callback ("has_tracking_position", &gsi::PluginImpl::has_tracking_position, &gsi::PluginImpl::f_has_tracking_position,
    "@brief Gets a value indicating whether the plugin provides a tracking position\n"
    "The tracking position is shown in the lower-left corner of the layout window to indicate the current position.\n"
    "If this method returns true for the active service, the application will fetch the position by calling \\tracking_position "
    "rather than displaying the original mouse position.\n"
    "\n"
    "The default implementation enables tracking if a mouse cursor has been set using \\add_mouse_cursor.\n"
    "When enabling tracking, make sure a reimplementation of \\mouse_moved_event does not consume the\n"
    "event and returns 'false'.\n"
    "\n"
    "This method has been added in version 0.27.6."
  ) +
  method ("tracking_position_test", &gsi::PluginImpl::tracking_position_test, "@hide") +
  callback ("tracking_position", &gsi::PluginImpl::tracking_position, &gsi::PluginImpl::f_tracking_position,
    "@brief Gets the tracking position\n"
    "See \\has_tracking_position for details.\n"
    "\n"
    "The default implementation takes the tracking position from a mouse cursor, if you have created one using "
    "\\add_mouse_cursor.\n"
    "When enabling tracking, make sure a reimplementation of \\mouse_moved_event does not consume the\n"
    "event and returns 'false'.\n"
    "\n"
    "This method has been added in version 0.27.6."
  ) +
  method ("clear_mouse_cursors", &gsi::PluginImpl::clear_mouse_cursors,
    "@brief Clears all existing mouse cursors\n"
    "Use this function to remove exisiting mouse cursors (see \\add_mouse_cursor and \\add_edge_marker).\n"
    "This method is automatically called when the plugin becomes deactivated.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("add_mouse_cursor", &gsi::PluginImpl::add_mouse_cursor_dpoint, gsi::arg ("p"), gsi::arg ("emphasize", false),
    "@brief Creates a cursor to indicate the mouse position\n"
    "This function will create a marker that indicates the (for example snapped) mouse position.\n"
    "In addition to this, it will establish the position for the tracking cursor, if mouse\n"
    "tracking is enabled in the application. You can override the tracking position by reimplementing\n"
    "\\tracking_position and \\has_tracking_position.\n"
    "\n"
    "To enable tracking, make sure a reimplementation of \\mouse_moved_event does not consume the\n"
    "event and returns 'false'.\n"
    "\n"
    "Multiple cursors can be created. In that case, the tracking position is given by the last cursor.\n"
    "\n"
    "If 'emphasize' is true, the cursor is displayed in a 'stronger' style - i.e. with a double circle instead of a single one.\n"
    "\n"
    "Before you use this method, clear existing cursors with \\clear_mouse_cursors.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("add_mouse_cursor", &gsi::PluginImpl::add_mouse_cursor_point, gsi::arg ("p"), gsi::arg ("cv_index"), gsi::arg ("layer"), gsi::arg ("emphasize", false),
    "@brief Creates a cursor to indicate the mouse position\n"
    "This version of this method creates a mouse cursor based on the integer-unit point and\n"
    "a source cellview index plus a layer info.\n"
    "The cellview index and layer info is used to derive the transformation rules to apply to the "
    "point and to compute the final position.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("add_edge_marker", &gsi::PluginImpl::add_edge_marker_dedge, gsi::arg ("e"), gsi::arg ("emphasize", false),
    "@brief Creates a cursor to indicate an edge\n"
    "This function will create a marker that indicates an edge - for example the edge that a point is snapping to. "
    "\n"
    "If 'emphasize' is true, the cursor is displayed in a 'stronger' style.\n"
    "\n"
    "Before you use this method, clear existing edge markers and cursors with \\clear_mouse_cursors.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("add_edge_marker", &gsi::PluginImpl::add_edge_marker_edge, gsi::arg ("e"), gsi::arg ("cv_index"), gsi::arg ("layer"), gsi::arg ("emphasize", false),
    "@brief Creates a cursor to indicate an edge\n"
    "This version of this method creates an edge marker based on the integer-unit edge and\n"
    "a source cellview index plus a layer info.\n"
    "The cellview index and layer info is used to derive the transformation rules to apply to the "
    "edge and to compute the final position.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("ac_from_buttons", &lay::ac_from_buttons, gsi::arg ("buttons"),
    "@brief Creates an angle constraint from a button combination\n"
    "This method provides the angle constraints implied by a specific modifier combination, i.e. "
    "'Shift' will render ortho snapping. Use this function to generate angle constraints following "
    "the established conventions.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("snap", &gsi::PluginImpl::snap, gsi::arg ("p"),
    "@brief Snaps a point to the edit grid\n"
    "\n"
    "@param p The point to snap\n"
    "\n"
    "If the edit grid is given, the point's x and y components\n"
    "are snapped to the edit grid. Otherwise the global grid is used.\n"
    "Edit and global grid are set by configuration options.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("snap", &gsi::PluginImpl::snap_vector, gsi::arg ("v"),
    "@brief Snaps a vector to the edit grid\n"
    "\n"
    "@param v The vector to snap\n"
    "\n"
    "If the edit grid is given, the vector's x and y components\n"
    "are snapped to the edit grid. Otherwise the global grid is used.\n"
    "Edit and global grid are set by configuration options.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("snap", &gsi::PluginImpl::snap_from_to, gsi::arg ("p"), gsi::arg ("plast"), gsi::arg ("connect", false), gsi::arg ("ac", lay::AC_Global, "AC_Global"),
    "@brief Snaps a point to the edit grid with an angle constraint\n"
    "\n"
    "@param p The point to snap\n"
    "@param plast The last point of the connection/move vector\n"
    "@param connect true, if the point is an connection vertex, false if it is a move target point\n"
    "@param ac Overrides the connect or move angle constraint unless it is \\Plugin#AC_Global\n"
    "\n"
    "This method snaps point \"p\" relative to the initial point \"plast\". This method\n"
    "tries to snap \"p\" to the edit or global grid (edit grid with higher priority), while\n"
    "trying to observe the angle constraint that imposes a constraint on the way \"p\"\n"
    "can move relative to \"plast\".\n"
    "\n"
    "The \"connect\" parameter will decide which angle constraint to use, unless \"ac\" specifies\n"
    "an angle constraint already. If \"connect\" is true, the line between \"p\" and \"plast\" is regarded a connection\n"
    "between points (e.g. a polygon edge) and the connection angle constraint applies. Otherwise\n"
    "the move constraint applies.\n"
    "\n"
    "The angle constraint determines how \"p\" can move in relation to \"plast\" - for example,\n"
    "if the angle constraint is \\Plugin#AC_Ortho, \"p\" can only move away from \"plast\" in horizontal or vertical direction.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("snap", &gsi::PluginImpl::snap_delta, gsi::arg ("v"), gsi::arg ("connect", false), gsi::arg ("ac", lay::AC_Global, "AC_Global"),
    "@brief Snaps a move vector to the edit grid with and implies an angle constraint\n"
    "\n"
    "@param v The vector to snap\n"
    "@param connect true, if the vector is an connection vector, false if it is a move vector\n"
    "@param ac Overrides the connect or move angle constraint unless it is AC_Global\n"
    "\n"
    "The \"connect\" parameter will decide which angle constraint to use, unless \"ac\" specifies\n"
    "an angle constraint already. If \"connect\" is true, the vector is regarded a connection line\n"
    "between points (e.g. a polygon edge) and the connection angle constraint applies. Otherwise\n"
    "the move constraint applies.\n"
    "\n"
    "The angle constraint determines how \"p\" can move in relation to \"plast\" - for example,\n"
    "if the angle constraint is \\Plugin#AC_Ortho, \"p\" can only move away from \"plast\" in horizontal or vertical direction.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("snap2", &gsi::PluginImpl::snap2, gsi::arg ("p"), gsi::arg ("visualize", false),
    "@brief Snaps a point to the edit grid with advanced snapping (including object snapping)\n"
    "\n"
    "@param p The point to snap\n"
    "@param visualize If true, a cursor shape is added to the scene indicating the snap details\n"
    "\n"
    "This method behaves like the other \"snap2\" variant, but does not allow to specify an\n"
    "angle constraint. Only grid constraints and snapping to objects is supported.\n"
    "\n"
    "If \"visualize\" is true, the function will generate calls to \\add_mouse_cursor or \\add_edge_marker to "
    "provide a visualization of the edges or vertexes that the point is snapping to. \\clear_mouse_cursors will "
    "be called before.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  method ("snap2", &gsi::PluginImpl::snap2_from_to, gsi::arg ("p"), gsi::arg ("plast"), gsi::arg ("connect", false), gsi::arg ("ac", lay::AC_Global, "AC_Global"), gsi::arg ("visualize", false),
    "@brief Snaps a point to the edit grid with an angle constraint with advanced snapping (including object snapping)\n"
    "\n"
    "@param p The point to snap\n"
    "@param plast The last point of the connection or move start point\n"
    "@param connect true, if the point is an connection, false if it is a move target point\n"
    "@param ac Overrides the connect or move angle constraint unless it is AC_Global\n"
    "@param visualize If true, a cursor shape is added to the scene indicating the snap details\n"
    "\n"
    "This method will snap the point p, given an initial point \"plast\". This includes an angle constraint.\n"
    "If \"connect\" is true, the line between \"plast\" and \"p\" is regarded a connection (e.g. a polygon edge).\n"
    "If not, the line is regarded a move vector. If \"ac\" is \\Plugin#AC_Global, the angle constraint is \n"
    "taken from the connect or move angle constraint, depending on the value of \"connect\". The angle constraint\n"
    "determines how \"p\" can move in relation to \"plast\" - for example, if the angle constraint is \\Plugin#AC_Ortho, \n"
    "\"p\" can only move away from \"plast\" in horizontal or vertical direction.\n"
    "\n"
    "This method considers options like global or editing grid or whether the target point\n"
    "will snap to another object. The behavior is given by the respective configuration.\n"
    "\n"
    "If \"visualize\" is true, the function will generate calls to \\add_mouse_cursor or \\add_edge_marker to "
    "provide a visualization of the edges or vertexes that the point is snapping to. \\clear_mouse_cursors will "
    "be called before.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
#if defined(HAVE_QTBINDINGS)
  gsi::method_ext ("editor_options_pages", &get_editor_options_pages,
    "@brief Gets the editor options pages which are associated with the view\n"
    "The editor options pages are created by the plugin factory class and are associated with this plugin.\n"
    "This method allows locating them and using them for plugin-specific purposes.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  gsi::method_ext ("focus_page", &get_focus_page,
    "@brief Gets the (first) focus page\n"
    "Focus pages are editor options pages that have a true value for \\EditorOptionsPage#is_focus_page.\n"
    "The pages can be navigated to quickly or can be shown in a modal dialog from the editor function.\n"
    "This method returns the first focus page present in the editor options pages stack.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
#endif
  callback ("focus_page_open", &gsi::PluginImpl::focus_page_open, &gsi::PluginImpl::f_focus_page_open,
    "@brief Gets called when the focus page wants to be opened - i.e. if 'Tab' is pressed during editing\n"
    "The default implementation calls \\EditorOptionsPage#show on the focus page.\n"
    "This method can be overloaded to provide certain actions before "
    "or after the page is shown, specifically if the page is a modal one. For example, it can update the page with current "
    "dimensions of a shape that is created and after committing the page, adjust the shape accordingly.\n"
    "\n"
    "This method has been added in version 0.30.4."
  ) +
  gsi::method ("view", &gsi::PluginImpl::view,
    "@brief Gets the view object the plugin is associated with\n"
    "This method returns the view object that the plugin is associated with.\n"
    "\n"
    "This convenience method has been added in version 0.30.4."
  ) +
  gsi::method ("dispatcher", &gsi::PluginImpl::dispatcher,
    "@brief Gets the dispatcher object the plugin is associated with\n"
    "This method returns the dispatcher object that the plugin is associated with.\n"
    "The dispatcher object manages the configuration parameters. 'set_config', 'get_config' and 'commit_config' "
    "can be used on this object to get or set configuration parameters. "
    "Configuration parameters are a way to persist information and the preferred way of communicating with "
    "editor option pages and configuration pages.\n"
    "\n"
    "This convenience method has been added in version 0.30.4."
  ),
  "@brief The plugin object\n"
  "\n"
  "This class provides the actual plugin implementation. Each view gets its own instance of the plugin class. The plugin factory \\PluginFactory class "
  "must be specialized to provide a factory for new objects of the Plugin class. See the documentation there for details about the plugin mechanism and "
  "the basic concepts.\n"
  "\n"
  "This class has been introduced in version 0.22.\n"
);

gsi::Enum<lay::angle_constraint_type> decl_AngleConstraintType ("lay", "AngleConstraintType",
  gsi::enum_const ("AC_Global", lay::AC_Global,
    "@brief Specifies to use the global angle constraint.\n"
  ) +
  gsi::enum_const ("AC_Any", lay::AC_Any,
    "@brief Specifies to use any angle and not snap to a specific direction.\n"
  ) +
  gsi::enum_const ("AC_Diagonal", lay::AC_Diagonal,
    "@brief Specifies to use multiples of 45 degree.\n"
  ) +
  gsi::enum_const ("AC_DiagonalOnly", lay::AC_DiagonalOnly,
    "@brief Specifies to use 45 degree or 135 degree only.\n"
    "This variant has been introduced in version 0.30.6."
  ) +
  gsi::enum_const ("AC_Ortho", lay::AC_Ortho,
    "@brief Specifies to use multiples of 90 degree.\n"
  ) +
  gsi::enum_const ("AC_Horizontal", lay::AC_Horizontal,
    "@brief Specifies to use horizontal direction only.\n"
  ) +
  gsi::enum_const ("AC_Vertical", lay::AC_Vertical,
    "@brief Specifies to use vertical direction only.\n"
  ),
  "@brief Specifies angle constraints during snapping.\n"
  "\n"
  "This enum has been introduced in version 0.30.4."
);

gsi::ClassExt<gsi::PluginImpl> inject_AngleConstraintType_in_parent (decl_AngleConstraintType.defs ());

}

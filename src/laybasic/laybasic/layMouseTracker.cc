
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


#include "layMouseTracker.h"
#include "layLayoutCanvas.h"
#include "layLayoutViewBase.h"
#include "layConverters.h"
#include "laybasicConfig.h"

namespace lay
{

MouseTracker::MouseTracker (lay::LayoutViewBase *view)
  : lay::ViewService (view->canvas ()), lay::Plugin (view), mp_view (view),
    m_cursor_color (tl::Color ()), m_cursor_line_style (0), m_cursor_enabled (false)
{
  ui ()->grab_mouse (this, false);
}

bool
MouseTracker::leave_event (bool)
{
  mp_markers.clear ();
  return false;
}

bool
MouseTracker::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_crosshair_cursor_color) {

    tl::Color color;
    lay::ColorConverter ().from_string (value, color);

    //  Change the color
    if (lay::test_and_set (m_cursor_color, color)) {
      mp_markers.clear ();
    }

  } else if (name == cfg_crosshair_cursor_line_style) {

    int dp = 0;
    tl::from_string (value, dp);

    //  Change the vertex_size
    if (lay::test_and_set (m_cursor_line_style, dp)) {
      mp_markers.clear ();
    }

  } else if (name == cfg_crosshair_cursor_enabled) {

    bool f = m_cursor_enabled;
    tl::from_string (value, f);
    if (f != m_cursor_enabled) {
      m_cursor_enabled = f;
      mp_markers.clear ();
    }

  }

  return false;  //  not taken
}

bool 
MouseTracker::mouse_move_event (const db::DPoint &p, unsigned int /*buttons*/, bool prio)
{
  //  NOTE: by catching events with low prio, the tracking position was already set by consumers
  //  with high prio
  if (! prio) {

    //  NOTE: because the tracker grabs first and grabbers are registered first gets served last, the
    //  tracker will receive the event after all other mouse grabbers have been served and had their
    //  chance to set the tracking position.
    lay::ViewService *vs = mp_view->canvas ()->active_service ();
    db::DPoint tp = p;
    if (vs && vs->enabled () && vs->has_tracking_position ()) {
      tp = vs->tracking_position ();
    }

    mp_view->current_pos (tp.x (), tp.y ());

    mp_markers.clear ();

    if (m_cursor_enabled) {

      double max_coord = 1e30;  //  big enough I guess

      for (int i = 0; i < 2; ++i) {

        mp_markers.push_back (new lay::DMarker (mp_view));
        mp_markers.back ()->set_line_style (m_cursor_line_style);
        mp_markers.back ()->set_line_width (1);
        mp_markers.back ()->set_halo (false);
        mp_markers.back ()->set_dither_pattern (1);
        mp_markers.back ()->set_color (m_cursor_color.is_valid () ? m_cursor_color : mp_view->canvas ()->foreground_color ());

        if (i == 0) {
          mp_markers.back ()->set (db::DEdge (db::DPoint (tp.x (), -max_coord), db::DPoint (tp.x (), max_coord)));
        } else {
          mp_markers.back ()->set (db::DEdge (db::DPoint (-max_coord, tp.y ()), db::DPoint (max_coord, tp.y ())));
        }

      }

    }

  }

  return false;
}

// ----------------------------------------------------------------------------

//  NOTE: configuration currently is not declared here.
//  Same for the configuration pages.

class MouseTrackerDeclaration
  : public lay::PluginDeclaration
{
public:
  MouseTrackerDeclaration ()
  {
    // .. nothing yet ..
  }

  virtual lay::Plugin *create_plugin (db::Manager * /*manager*/, lay::Dispatcher * /*dispatcher*/, lay::LayoutViewBase *view) const
  {
    return new MouseTracker (view);
  }

  virtual bool enable_catchall_editor_options_pages () const
  {
    return false;
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> tracker_decl (new MouseTrackerDeclaration (), -1000, "laybasic::MouseTrackerPlugin");

} // namespace lay

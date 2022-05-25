
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

namespace lay
{

MouseTracker::MouseTracker (lay::LayoutViewBase *view)
  : lay::ViewService (view->canvas ()), mp_view (view)
{
  ui ()->grab_mouse (this, false);
}

bool 
MouseTracker::mouse_move_event (const db::DPoint &p, unsigned int /*buttons*/, bool prio)
{
  if (prio) {

    //  NOTE: because the tracker grabs first and grabbers are registered first gets served last, the
    //  tracker will receive the event after all other mouse grabbers have been served and had their
    //  chance to set the tracking position.
    lay::ViewService *vs = mp_view->canvas ()->active_service ();
    db::DPoint tp = p;
    if (vs && vs->enabled () && vs->has_tracking_position ()) {
      tp = vs->tracking_position ();
    }

    mp_view->current_pos (tp.x (), tp.y ());

  }

  return false;
}

} // namespace lay


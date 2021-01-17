
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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
#include "layLayoutView.h"

namespace lay
{

MouseTracker::MouseTracker (lay::LayoutView *view)
  : lay::ViewService (view->view_object_widget ()), mp_view (view)
{
  widget ()->grab_mouse (this, false);
}

bool 
MouseTracker::mouse_move_event (const db::DPoint &p, unsigned int /*buttons*/, bool prio)
{
  if (prio) {
    mp_view->current_pos (p.x (), p.y ());
  }
  return false;
}

} // namespace lay


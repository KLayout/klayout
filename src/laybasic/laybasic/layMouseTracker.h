
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


#ifndef HDR_layMouseTracker
#define HDR_layMouseTracker

#include "layViewObject.h"
#include "layMarker.h"
#include "tlObject.h"

class QMouseEvent;

namespace lay {

class LayoutCanvas;
class LayoutViewBase;

class MouseTracker
  : public lay::ViewService
{
public: 
  MouseTracker (lay::LayoutViewBase *view);

  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);
  bool leave_event (bool prio);
  bool configure (const std::string &name, const std::string &value);

private:
  lay::LayoutViewBase *mp_view;
  tl::shared_collection<lay::DMarker> mp_markers;
  tl::Color m_cursor_color;
  int m_cursor_line_style;
  bool m_cursor_enabled;
};

}

#endif


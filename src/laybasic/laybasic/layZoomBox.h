
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



#ifndef HDR_layZoomBox
#define HDR_layZoomBox

#include "layViewObject.h"

namespace lay
{

class LayoutViewBase;
class LayoutCanvas;
class RubberBox;

class LAYBASIC_PUBLIC ZoomService
  : public lay::ViewService
{
public: 
  ZoomService (lay::LayoutViewBase *view);
  ~ZoomService ();

  void set_colors (tl::Color background, tl::Color text);
  void begin (const db::DPoint &pos);
  void begin_pan (const db::DPoint &pos);

private:
  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_release_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool wheel_event (int delta, bool horizontal, const db::DPoint &p, unsigned int buttons, bool prio);
  virtual void drag_cancel ();

  db::DPoint m_p1, m_p2;
  db::DBox m_vp;
  lay::LayoutViewBase *mp_view;
  lay::RubberBox *mp_box;
  unsigned int m_color;
};

}

#endif



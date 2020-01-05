
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



#ifndef HDR_laySelector
#define HDR_laySelector

#include "layViewObject.h"
#include "layEditable.h"

#include <QTimer>
#include <QObject>

namespace lay {

class RubberBox;
class LayoutView;
class LayoutCanvas;

class SelectionService
  : public QObject,
    public lay::ViewService
{
Q_OBJECT

public: 
  SelectionService (lay::LayoutView *view);
  ~SelectionService ();

  void set_colors (QColor background, QColor color);
  void begin (const db::DPoint &pos);

  bool dragging () const { return mp_box != 0; }

  //  called by lay::Move, so these methods need to be public
  virtual bool leave_event (bool prio);
  virtual bool enter_event (bool prio);
  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_release_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_double_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool wheel_event (int delta, bool horizonal, const db::DPoint &p, unsigned int buttons, bool prio);

  /**
   *  @brief Reset the hover timer for the transient selection
   *
   *  This method may be used by other services (in particular Move) to avoid the transient to
   *  be triggered from a move operation.
   */
  void hover_reset ();

public slots:
  void timeout ();

private:
  virtual void deactivated ();

  db::DPoint m_p1, m_p2;
  lay::LayoutView *mp_view;
  lay::RubberBox *mp_box;
  unsigned int m_color;
  unsigned int m_buttons;
  QTimer m_timer;
  bool m_hover;
  bool m_hover_wait;
  db::DPoint m_hover_point;
  bool m_mouse_in_window;
};

}

#endif


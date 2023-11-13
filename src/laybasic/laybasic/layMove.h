
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

#ifndef HDR_layMove
#define HDR_layMove

#include "laybasicCommon.h"
#include "dbManager.h"
#include "layViewObject.h"

#include <memory>

namespace lay {

class Editables;
class LayoutViewBase;

class LAYBASIC_PUBLIC MoveService :
    public lay::ViewService
{
public: 
  MoveService (lay::LayoutViewBase *view);
  ~MoveService ();

  bool configure (const std::string &name, const std::string &value);
  bool begin_move (db::Transaction *transaction = 0, bool transient_selection = false);

private:
  virtual bool mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_double_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_release_event (const db::DPoint &p, unsigned int /*buttons*/, bool prio);
  virtual bool wheel_event (int delta, bool horizontal, const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool key_event (unsigned int key, unsigned int buttons);
  virtual void drag_cancel ();
  virtual void deactivated ();

  bool handle_click (const db::DPoint &p, unsigned int buttons, bool drag_transient, db::Transaction *transaction);

  bool m_dragging;
  bool m_dragging_transient;
  lay::Editables *mp_editables;
  lay::LayoutViewBase *mp_view;
  double m_global_grid;
  db::DPoint m_shift;
  db::DPoint m_mouse_pos;
  std::unique_ptr<db::Transaction> mp_transaction;
};

}

#endif


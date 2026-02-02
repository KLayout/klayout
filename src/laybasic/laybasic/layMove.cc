
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

#include "layMove.h"
#include "layEditable.h"
#include "layLayoutViewBase.h"
#include "layEditorUtils.h"
#include "layEditorOptionsPage.h"
#include "laySelector.h"
#include "laybasicConfig.h"

namespace lay
{

LAYBASIC_PUBLIC std::string move_editor_options_name ("move-editor-options");
LAYBASIC_PUBLIC std::string move_function_name ("move-execute");
LAYBASIC_PUBLIC std::string move_distance_setter_name ("move-distance");

// -------------------------------------------------------------
//  MoveService implementation

MoveService::MoveService (lay::LayoutViewBase *view)
  : lay::EditorServiceBase (view),
    m_dragging (false),
    m_dragging_transient (false),
    mp_editables (view),
    mp_view (view),
    m_global_grid (0.001)
{ 
  //  .. nothing yet ..
}

MoveService::~MoveService ()
{
  drag_cancel ();
}

void
MoveService::deactivated ()
{
  EditorServiceBase::deactivated ();
  m_shift = db::DPoint ();
  mp_editables->clear_transient_selection ();

  if (m_dragging) {
    //  we don't just call drag_cancel() - this way avoids pending selections with the wrong coordinates
    mp_view->edit_cancel ();
  }
}

bool
MoveService::configure (const std::string &name, const std::string &value)
{
  if (lay::EditorServiceBase::configure (name, value)) {
    return true;
  }

  if (name == cfg_grid) {
    tl::from_string (value, m_global_grid);
  }

  return false;  //  not taken
}

void
MoveService::function (const std::string &name, const std::string &value)
{
  if (name == move_function_name) {

    try {

      db::DVector s;
      tl::from_string (value, s);

      m_dragging = false;

      show_toolbox (false);
      ui ()->ungrab_mouse (this);

      mp_editables->end_move (s, mp_transaction.release ());

      if (m_dragging_transient) {
        mp_editables->clear_selection ();
      }

      drag_cancel ();

    } catch (...) {
    }

  }
}

bool
MoveService::key_event (unsigned int key, unsigned int buttons)
{
  if (lay::EditorServiceBase::key_event (key, buttons)) {
    return true;
  }

  if (buttons == 0 && (key == lay::KeyEnter || key == lay::KeyReturn)) {
    end_move ();
    return true;
  }

  double dx = 0.0, dy = 0.0;
  if (int (key) == lay::KeyDown) {
    dy = -1.0;
  } else if (int (key) == lay::KeyUp) {
    dy = 1.0;
  } else if (int (key) == lay::KeyLeft) {
    dx = -1.0;
  } else if (int (key) == lay::KeyRight) {
    dx = 1.0;
  }

  if (! m_dragging && fabs (dx + dy) > 0.0 && mp_editables->has_selection ()) {

    //  determine a shift distance which is 2, 5 or 10 times the grid and is more than 5 pixels
    double dmin = double (5 /*pixels min shift*/) / ui ()->mouse_event_trans ().mag ();
    double d = m_global_grid;
    while (d < dmin) {
      d *= 2.0;
      if (d < dmin) {
        d *= 2.5;
        if (d < dmin) {
          d *= 2.0;
        }
      } 
    }

    db::DVector s = db::DVector (dx * d, dy * d);
    m_shift += s;

    std::string pos = std::string ("dx: ") + tl::micron_to_string (m_shift.x ()) + "  dy: " + tl::micron_to_string (m_shift.y ());
    mp_view->message (pos);

    mp_editables->transform (db::DCplxTrans (s));

    return true;

  } else {
    return false;
  }
}

bool
MoveService::shortcut_override_event (unsigned int key, unsigned int buttons)
{
  if (! m_dragging) {
    if (int (key) == lay::KeyDown ||
        int (key) == lay::KeyLeft ||
        int (key) == lay::KeyUp ||
        int (key) == lay::KeyRight) {
      return true;
    }
  }

  return lay::EditorServiceBase::shortcut_override_event (key, buttons);
}

void
MoveService::show_toolbox (bool visible)
{
  lay::EditorOptionsPage *tb = toolbox_widget ();
  if (tb) {
    tb->set_visible (visible);
  }
}

lay::EditorOptionsPage *
MoveService::toolbox_widget ()
{
  return mp_view->editor_options_pages () ? mp_view->editor_options_pages ()->page_with_name (move_editor_options_name) : 0;
}

bool 
MoveService::mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio) 
{
  bool ret = false;

  if (m_dragging) {

    set_cursor (lay::Cursor::size_all);
    auto pmv = mp_editables->move (p, ac_from_buttons (buttons));

    //  display the proposed move transformation
    if (pmv.first >= 0) {

      std::string pos = std::string ("dx: ") + tl::micron_to_string (pmv.second.disp ().x ()) + "  dy: " + tl::micron_to_string (pmv.second.disp ().y ());
      if (pmv.second.rot () != 0) {
        pos += std::string ("  ") + ((const db::DFTrans &) pmv.second).to_string ();
      }
      mp_view->message (pos);

      lay::EditorOptionsPage *tb = toolbox_widget ();
      if (tb) {
        tb->configure (move_distance_setter_name, pmv.second.disp ().to_string ());
      }

    }

  } else if (prio) {

    lay::SelectionService *selector = mp_view->selection_service ();
    if (selector) {
      ret = selector->mouse_move_event (p, buttons, prio);
    }

  }

  //  track mouse position for the infix move initiation
  m_mouse_pos = p;

  return ret;  // not taken to allow the mouse tracker to receive events as well
}

bool 
MoveService::mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (prio && (buttons & lay::RightButton) != 0 && m_dragging) {
    if ((buttons & lay::ShiftButton) != 0) {
      mp_editables->move_transform (p, db::DFTrans (db::DFTrans::m90), ac_from_buttons (buttons));
    } else {
      mp_editables->move_transform (p, db::DFTrans (db::DFTrans::r90), ac_from_buttons (buttons));
    }
    return true;
  } 
  if (prio && (buttons & lay::LeftButton) != 0) {
    if (handle_click (p, buttons, false, 0)) {
      return true;
    }
  } 
  if (prio) {
    lay::SelectionService *selector = mp_view->selection_service ();
    if (selector) {
      return selector->mouse_click_event (p, buttons, prio);
    }
  }
  return false;
}

bool 
MoveService::mouse_double_click_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (prio) {

    //  stop dragging if required
    if (m_dragging) {
      handle_click (p, buttons, false, 0);
    }

    lay::SelectionService *selector = mp_view->selection_service ();
    if (selector) {
      return selector->mouse_double_click_event (p, buttons, prio);
    }

  }
  return false;
}

bool 
MoveService::mouse_release_event (const db::DPoint &p, unsigned int buttons, bool prio) 
{ 
  if (prio) {
    lay::SelectionService *selector = mp_view->selection_service ();
    if (selector) {
      return selector->mouse_release_event (p, buttons, prio);
    }
  }
  return false;
}

bool 
MoveService::wheel_event (int delta, bool horizontal, const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (prio) {
    lay::SelectionService *selector = mp_view->selection_service ();
    if (selector) {
      return selector->wheel_event (delta, horizontal, p, buttons, prio);
    }
  }
  return false;
}

bool 
MoveService::mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (prio && (buttons & lay::LeftButton) != 0) {
    if (handle_click (p, buttons, false, 0)) {
      return true;
    }
  } 
  if (prio) {
    lay::SelectionService *selector = mp_view->selection_service ();
    if (selector) {
      return selector->mouse_press_event (p, buttons, prio);
    }
  }
  return false;
}

bool
MoveService::start_move (db::Transaction *transaction, bool transient_selection)
{
  if (m_dragging) {
    return false;
  }

  std::unique_ptr<db::Transaction> trans_holder (transaction);

  bool drag_transient = false;

  if (! transaction) {

    //  unless in "continue with move" use case try to establish a selection

    if (! mp_editables->has_selection ()) {
      //  try to use the transient selection for the real one
      mp_editables->transient_to_selection ();
      drag_transient = true;
    }

    if (! mp_editables->has_selection ()) {
      //  still nothing selected
      return false;
    }

  } else {

    //  inherit transient selection mode from previous operation
    drag_transient = transient_selection;

  }

  db::DBox bbox = mp_editables->selection_bbox ();
  if (bbox.empty ()) {
    //  nothing (useful) selected
    return false;
  }

  set_cursor (lay::Cursor::size_all);

  //  emulate a "begin move" at the current mouse position if inside the box or the closest point
  //  of the box.

  db::DPoint pstart = m_mouse_pos;
  if (! bbox.contains (pstart)) {
    pstart.set_x (std::max (pstart.x (), bbox.p1 ().x ()));
    pstart.set_x (std::min (pstart.x (), bbox.p2 ().x ()));
    pstart.set_y (std::max (pstart.y (), bbox.p1 ().y ()));
    pstart.set_y (std::min (pstart.y (), bbox.p2 ().y ()));
  }

  return handle_click (pstart, 0, drag_transient, trans_holder.release ());
}

void
MoveService::end_move ()
{
  if (m_dragging) {
    handle_click (m_mouse_pos, 0, false, 0);
  }
}

bool 
MoveService::handle_click (const db::DPoint &p, unsigned int buttons, bool drag_transient, db::Transaction *transaction)
{
  std::unique_ptr<db::Transaction> trans_holder (transaction);

  if (! m_dragging) {

    mp_transaction.reset (trans_holder.release ());
    ui ()->drag_cancel ();

    if (mp_editables->begin_move (p, ac_from_buttons (buttons))) {

      ui ()->hover_reset ();
        
      mp_editables->clear_transient_selection ();

      m_dragging = true;
      m_dragging_transient = drag_transient;

      show_toolbox (true);
      ui ()->grab_mouse (this, false);

      m_shift = db::DPoint ();

      return true;

    }

  } else {

    m_dragging = false;

    show_toolbox (false);
    ui ()->ungrab_mouse (this);

    mp_editables->end_move (p, ac_from_buttons (buttons), mp_transaction.release ());

    if (m_dragging_transient) {
      mp_editables->clear_selection ();
    }

    return true;

  }
  return false;
}

void
MoveService::drag_cancel ()
{
  m_shift = db::DPoint ();
  if (m_dragging) {
    show_toolbox (false);
    ui ()->ungrab_mouse (this);
    m_dragging = false;
  }
}

void
MoveService::cancel_transaction ()
{ 
  if (m_dragging) {
    if (mp_transaction.get ()) {
      mp_transaction->cancel ();
    }
    mp_transaction.reset (0);
  }
}

void
MoveService::finish_transaction ()
{
  if (m_dragging) {
    mp_transaction.reset (0);
  }
}

// ----------------------------------------------------------------------------

class MoveServiceDeclaration
  : public lay::PluginDeclaration
{
public:
  MoveServiceDeclaration ()
    : lay::PluginDeclaration (-1)
  {
    // .. nothing yet ..
  }

  virtual lay::Plugin *create_plugin (db::Manager * /*manager*/, lay::Dispatcher * /*dispatcher*/, lay::LayoutViewBase *view) const
  {
    return new MoveService (view);
  }

  virtual std::vector<std::string> additional_editor_options_pages () const
  {
    std::vector<std::string> names;
    //  TODO: provide in a central place instead of borrowing from the edt module
    names.push_back ("GenericEditorOptions");
    return names;
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> move_service_decl (new MoveServiceDeclaration (), -970, "laybasic::MoveServicePlugin");

}


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



#include "laySelector.h"
#include "layRubberBox.h"
#include "layLayoutViewBase.h"
#include "tlLog.h"
#include "tlException.h"

#if defined(HAVE_QT)
#  include <QMessageBox>
#endif

namespace lay
{

// -------------------------------------------------------------
//  SelectionService implementation

SelectionService::SelectionService (lay::LayoutViewBase *view) :
#if defined(HAVE_QT)
    QObject (),
#endif
    lay::ViewService (view->canvas ()), 
    mp_view (view),
    mp_box (0),
    m_color (0),
    m_buttons (0),
    m_hover (false),
    m_hover_wait (false),
    m_mouse_in_window (false)
{ 
#if defined(HAVE_QT)
  m_timer.setInterval (100 /*hover time*/);
  m_timer.setSingleShot (true);
  connect (&m_timer, SIGNAL (timeout ()), this, SLOT (timeout ()));
#endif
}

SelectionService::~SelectionService ()
{
  if (mp_box) {
    delete mp_box;
    mp_box = 0;
  }
}

void 
SelectionService::set_colors (tl::Color /*background*/, tl::Color color)
{
  m_color = color.rgb ();
  if (mp_box) {
    mp_box->set_color (m_color);
  }
}

void  
SelectionService::deactivated ()
{
  mp_view->clear_transient_selection ();
  reset_box ();
}

void 
SelectionService::hover_reset ()
{
  if (m_hover_wait) {
#if defined(HAVE_QT)
    m_timer.stop ();
#endif
    m_hover_wait = false;
  }
  if (m_hover) {
    mp_view->clear_transient_selection ();
    m_hover = false;
  }
}

#if defined(HAVE_QT)
void 
SelectionService::timeout ()
{
  m_hover_wait = false;
  m_hover = true;
  mp_view->clear_transient_selection ();
  mp_view->transient_select (m_hover_point);
}
#endif

void
SelectionService::reset_box ()
{
  if (mp_box) {

    ui ()->ungrab_mouse (this);

    delete mp_box;
    mp_box = 0;

  }
}

bool
SelectionService::wheel_event (int /*delta*/, bool /*horizontal*/, const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/)
{
  return false;
}

bool
SelectionService::enter_event (bool /*prio*/)
{
  m_mouse_in_window = true;
  return false;
}

bool
SelectionService::leave_event (bool prio)
{
  m_mouse_in_window = false;

  hover_reset ();

  if (prio) {
    reset_box ();
  }

  return false;
}

bool 
SelectionService::mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (prio) {

    m_current_position = p;

    if ((buttons & LeftButton) == 0) {
      reset_box ();
    }

    if (mp_box) {
      m_p2 = p;
      mp_box->set_points (m_p1, m_p2);
    } else if (m_mouse_in_window && mp_view->transient_selection_mode ()) {
      m_hover_wait = true;
#if defined(HAVE_QT)
      m_timer.start ();
#endif
      m_hover_point = p;
    }

  }

  return false;
}

bool 
SelectionService::mouse_double_click_event (const db::DPoint & /*p*/, unsigned int buttons, bool prio)
{
  hover_reset ();

  if (prio) {
    reset_box ();
  }

  if (prio && (buttons & lay::LeftButton) != 0) {
    mp_view->show_properties ();
    return true;
  }

  return false;
}

bool 
SelectionService::mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  hover_reset ();

  if (prio) {

    reset_box ();

    if ((buttons & lay::LeftButton) != 0) {
      mp_view->stop_redraw (); // TODO: how to restart if selection is aborted?
      m_buttons = buttons;
      begin (p);
      return true;
    }

  }

  return false;
}

bool 
SelectionService::mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio) 
{ 
  if (prio) {
    reset_box ();
  }

  if (prio && mp_view && ui ()->mouse_event_viewport ().contains (p) && (buttons & lay::LeftButton) != 0) {

    lay::Editable::SelectionMode mode = lay::Editable::Replace;
    bool shift = ((buttons & lay::ShiftButton) != 0);
    bool ctrl = ((buttons & lay::ControlButton) != 0);
    if (shift && ctrl) {
      mode = lay::Editable::Invert;
    } else if (shift) {
      mode = lay::Editable::Add;
    } else if (ctrl) {
      mode = lay::Editable::Reset;
    } 

    //  select is allowed to throw an exception 
    try {

      mp_view->select (p, mode);

      //  add a transient selection trigger to capture the "next" selection.
      if (mp_view->transient_selection_mode ()) {
        m_hover_wait = true;
#if defined(HAVE_QT)
        m_timer.start ();
#endif
        m_hover_point = p;
      }

    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
#if defined(HAVE_QT)
      QMessageBox::critical (0, tr ("Error"), tl::to_qstring (ex.msg ()));
#endif
      //  clear selection
      mp_view->select (db::DBox (), lay::Editable::Reset);
    }

  }

  return false;
}

bool 
SelectionService::mouse_release_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool prio) 
{ 
  hover_reset ();

  if (prio && mp_box) {

    reset_box ();

    if (mp_view) { 

      lay::Editable::SelectionMode mode = lay::Editable::Replace;
      bool shift = ((m_buttons & lay::ShiftButton) != 0);
      bool ctrl = ((m_buttons & lay::ControlButton) != 0);
      if (shift && ctrl) {
        mode = lay::Editable::Invert;
      } else if (shift) {
        mode = lay::Editable::Add;
      } else if (ctrl) {
        mode = lay::Editable::Reset;
      } 

      //  select is allowed to throw an exception 
      try {
        mp_view->select (db::DBox (m_p1, m_p2), mode);
      } catch (tl::Exception &ex) {
        tl::error << ex.msg ();
#if defined(HAVE_QT)
        QMessageBox::critical (0, tr ("Error"), tl::to_qstring (ex.msg ()));
#endif
        //  clear selection
        mp_view->select (db::DBox (), lay::Editable::Reset);
      }

    }

  }

  return false;
}

void 
SelectionService::begin (const db::DPoint &pos)
{ 
  if (mp_box) {
    delete mp_box;
  }

  m_p1 = pos;
  m_p2 = pos;
  mp_box = new lay::RubberBox (ui (), m_color, pos, pos);
  mp_box->set_stipple (6); // coarse hatched

  ui ()->grab_mouse (this, true);
}

}


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

#if defined(HAVE_QT)
#  include <QMouseEvent>
#  include <QWheelEvent>
#  include <QKeyEvent>
#  include <QMimeData>
#  include <QApplication>
#  include <QWidget>
#endif

#include "layViewObject.h"
#include "layCanvasPlane.h"
#include "layBitmap.h"
#if defined(HAVE_QT)
#  include "layDragDropData.h"
#endif
#include "layUtils.h"
#include "tlException.h"
#include "tlAlgorithm.h"
#include "tlExceptions.h"

#include <memory>

namespace lay
{

//  The distance by which the mouse must move in order to create a press/move/release
//  event rather than a single click event:
const int click_tolerance = 5;

// ---------------------------------------------------------------
//  A helper function to convert a Qt modifier/buttons to klayout buttons

#if defined(HAVE_QT)
static unsigned int 
qt_to_buttons (Qt::MouseButtons b, Qt::KeyboardModifiers m)
{
  // this is a straightforward conversion with the exception that
  // MetaModifier+LeftButton is taken as a RightButton
  // This is useful on MAC OX for people with a one-button mouse.
  // They can do a right click by doing a ctrl-leftclick.
  // BTW: On a MAC's keyboard, the cmd-key is received here as a ControlModifier
  // while the ctrl-key is received as a MetaModifier
  return ((b & Qt::LeftButton) ? ((m & Qt::MetaModifier) ? RightButton : LeftButton) : 0) |
         ((b & Qt::MiddleButton) ? MidButton : 0) |
         ((b & Qt::RightButton) ? RightButton : 0) |
         ((m & Qt::ShiftModifier) != 0 ? ShiftButton : 0) |
         ((m & Qt::ControlModifier) != 0 ? ControlButton : 0) |
         ((m & Qt::AltModifier) != 0 ? AltButton : 0);
}
#endif

// ---------------------------------------------------------------
//  BackgroundViewObject implementation

BackgroundViewObject::BackgroundViewObject (ViewObjectUI *widget)
  : mp_widget (widget), m_visible (true), m_z_order (0)
{
  if (widget) {
    widget->m_background_objects.push_back (this);
    redraw ();
  }
}

BackgroundViewObject::~BackgroundViewObject ()
{
  redraw ();
}

void 
BackgroundViewObject::visible (bool vis)
{
  if (vis != m_visible) {
    m_visible = vis;
    redraw ();
  }
}

void 
BackgroundViewObject::redraw ()
{
  if (widget ()) {
    widget ()->touch_bg ();
  }
}

void 
BackgroundViewObject::z_order (int z)
{
  if (z != m_z_order) {
    m_z_order = z;
    redraw ();
  }
}

// ---------------------------------------------------------------
//  ViewObject implementation

ViewObject::ViewObject (ViewObjectUI *widget, bool _static)
  : mp_widget (widget), m_static (_static), m_visible (true), m_dismissable (false)
{
  if (widget) {
    widget->m_objects.push_back (this);
    redraw ();
  }
}

ViewObject::~ViewObject ()
{
  redraw ();
}

void
ViewObject::set_dismissable (bool dismissable)
{
  if (m_dismissable != dismissable) {
    m_dismissable = dismissable;
    redraw ();
  }
}

void 
ViewObject::visible (bool vis)
{
  if (vis != m_visible) {
    m_visible = vis;
    redraw ();
  }
}

void 
ViewObject::redraw ()
{
  if (widget ()) {
    if (m_static) {
      widget ()->touch ();
    } else {
      widget ()->update ();
    }
  }
}

void
ViewObject::thaw ()
{
  if (widget ()) {
    widget ()->thaw (this);
  }
}

void
ViewObject::freeze ()
{
  if (widget ()) {
    widget ()->freeze (this);
  }
}

// ---------------------------------------------------------------
//  ViewService implementation

ViewService::ViewService (ViewObjectUI *widget)
  : mp_widget (widget), m_abs_grab (false), m_enabled (true)
{
  if (widget) {
    widget->register_service (this);
  }
}

ViewService::~ViewService ()
{
  if (mp_widget) {
    mp_widget->unregister_service (this);
  }
  mp_widget = 0;
}

void
ViewService::enable (bool en)
{
  m_enabled = en;
}

void 
ViewService::set_cursor (lay::Cursor::cursor_shape cursor)
{
  mp_widget->set_cursor (cursor);
}

// ---------------------------------------------------------------
//  ViewObjectQWidget implementation

#if defined(HAVE_QT)

class ViewObjectQWidget : public QWidget
{
public:
  ViewObjectQWidget (QWidget *parent, ViewObjectUI *view)
    : QWidget (parent), mp_view (view)
  {
    //  .. nothing yet ..
  }

  void resizeEvent (QResizeEvent *)
  {
    mp_view->resize_event (width (), height ());
  }

  bool focusNextPrevChild (bool /*next*/)
  {
    return false;
  }

  void keyPressEvent (QKeyEvent *e)
  {
  BEGIN_PROTECTED
    unsigned int buttons = qt_to_buttons (Qt::MouseButtons (), e->modifiers ());
    mp_view->send_key_press_event ((unsigned int) e->key (), buttons);
  END_PROTECTED
  }

  DragDropDataBase *get_drag_drop_data (const QMimeData *data)
  {
    if (! data || ! data->hasFormat (QString::fromUtf8 (drag_drop_mime_type ()))) {
      return 0;
    }

    QByteArray ba = data->data (QString::fromUtf8 (drag_drop_mime_type ()));

    //  TODO: provide some global mechanism to register drag & drop classes
    std::unique_ptr<DragDropDataBase> cd (new CellDragDropData ());
    if (cd->deserialize (ba)) {
      return cd.release ();
    }

    //  TODO: more ...

    return 0;
  }

  void dragEnterEvent (QDragEnterEvent *event)
  {
  BEGIN_PROTECTED

    const DragDropDataBase *dd = get_drag_drop_data (event->mimeData ());
    if (dd) {

      db::DPoint p = mp_view->pixel_to_um (db::Point (event->pos ().x (), event->pos ().y ()));

      bool done = mp_view->drag_enter_event (p, dd);
      ViewObjectUI::service_iterator svc = mp_view->begin_services ();
      while (svc != mp_view->end_services () && !done) {
        ViewObjectUI::service_iterator next = svc;
        ++next;
        done = (*svc)->drag_enter_event (p, dd);
        svc = next;
      }

      if (done) {
        event->acceptProposedAction ();
      }

    }

  END_PROTECTED
  }

  void dragLeaveEvent (QDragLeaveEvent * /*event*/)
  {
  BEGIN_PROTECTED

    mp_view->drag_leave_event ();
    ViewObjectUI::service_iterator svc = mp_view->begin_services ();
    while (svc != mp_view->end_services ()) {
      ViewObjectUI::service_iterator next = svc;
      ++next;
      (*svc)->drag_leave_event ();
      svc = next;
    }

  END_PROTECTED
  }

  void dragMoveEvent (QDragMoveEvent *event)
  {
  BEGIN_PROTECTED

    const DragDropDataBase *dd = get_drag_drop_data (event->mimeData ());
    if (dd) {

      db::DPoint p = mp_view->pixel_to_um (db::Point (event->pos ().x (), event->pos ().y ()));

      bool done = mp_view->drag_move_event (p, dd);
      ViewObjectUI::service_iterator svc = mp_view->begin_services ();
      while (svc != mp_view->end_services () && !done) {
        ViewObjectUI::service_iterator next = svc;
        ++next;
        done = (*svc)->drag_move_event (p, dd);
        svc = next;
      }

    }

  END_PROTECTED
  }

  void dropEvent (QDropEvent *event)
  {
  BEGIN_PROTECTED

    const DragDropDataBase *dd = get_drag_drop_data (event->mimeData ());
    if (dd) {

      db::DPoint p = mp_view->pixel_to_um (db::Point (event->pos ().x (), event->pos ().y ()));

      bool done = mp_view->drop_event (p, dd);
      ViewObjectUI::service_iterator svc = mp_view->begin_services ();
      while (svc != mp_view->end_services () && !done) {
        ViewObjectUI::service_iterator next = svc;
        ++next;
        done = (*svc)->drop_event (p, dd);
        svc = next;
      }

    }

  END_PROTECTED
  }

  void mouseMoveEvent (QMouseEvent *e)
  {
  BEGIN_PROTECTED

    db::DPoint p;
#if QT_VERSION < 0x60000
    p = db::DPoint (e->pos ().x (), e->pos ().y ());
#else
    p = db::DPoint (e->position ().x (), e->position ().y ());
#endif

    mp_view->send_mouse_move_event (p, qt_to_buttons (e->buttons (), e->modifiers ()));

  END_PROTECTED
  }

  void mouseDoubleClickEvent (QMouseEvent *e)
  {
    BEGIN_PROTECTED

    db::DPoint p;
#if QT_VERSION < 0x60000
    p = db::DPoint (e->pos ().x (), e->pos ().y ());
#else
    p = db::DPoint (e->position ().x (), e->position ().y ());
#endif

    mp_view->send_mouse_double_clicked_event (p, qt_to_buttons (e->buttons (), e->modifiers ()));

    END_PROTECTED
  }

  void
#if QT_VERSION >= 0x60000
  enterEvent (QEnterEvent * /*event*/)
#else
  enterEvent (QEvent * /*event*/)
#endif
  {
    BEGIN_PROTECTED
    mp_view->send_enter_event ();
    END_PROTECTED
  }

  void leaveEvent (QEvent * /*event*/)
  {
    BEGIN_PROTECTED
    mp_view->send_leave_event ();
    END_PROTECTED
  }

  void wheelEvent (QWheelEvent *e)
  {
    BEGIN_PROTECTED

    db::DPoint p;
#if QT_VERSION < 0x60000
    int delta = e->delta ();
    p = db::DPoint (e->pos ().x (), e->pos ().y ());
    bool horizontal = (e->orientation () == Qt::Horizontal);
#else
    int delta = e->angleDelta ().y ();
    p = db::DPoint (e->position ().x (), e->position ().y ());
    bool horizontal = false;
#endif

    e->ignore ();

    mp_view->send_wheel_event (delta, horizontal, p, qt_to_buttons (e->buttons (), e->modifiers ()));

    END_PROTECTED
  }

  void mousePressEvent (QMouseEvent *e)
  {
    BEGIN_PROTECTED

    db::DPoint p;
#if QT_VERSION < 0x60000
    p = db::DPoint (e->pos ().x (), e->pos ().y ());
#else
    p = db::DPoint (e->position ().x (), e->position ().y ());
#endif

    mp_view->send_mouse_press_event (p, qt_to_buttons (e->buttons (), e->modifiers ()));

    END_PROTECTED
  }

  void mouseReleaseEvent (QMouseEvent *e)
  {
    BEGIN_PROTECTED

    db::DPoint p;
#if QT_VERSION < 0x60000
    p = db::DPoint (e->pos ().x (), e->pos ().y ());
#else
    p = db::DPoint (e->position ().x (), e->position ().y ());
#endif

    mp_view->send_mouse_release_event (p, qt_to_buttons (e->buttons (), e->modifiers ()));

    END_PROTECTED
  }

  void paintEvent (QPaintEvent *)
  {
    BEGIN_PROTECTED
    mp_view->paint_event ();
    END_PROTECTED
  }

#if defined(HAVE_QT)
  bool event (QEvent *e)
  {
    if (e->type () == QEvent::MaxUser) {

      //  GTF probe event
      //  record the contents (the screenshot) as ASCII text
      mp_view->gtf_probe ();

      e->accept ();
      return true;

    } else {
      return QWidget::event (e);
    }
  }
#endif

private:
  ViewObjectUI *mp_view;
};

#endif

// ---------------------------------------------------------------
//  ViewObjectWidget implementation

ViewObjectUI::ViewObjectUI ()
  : m_view_objects_dismissed (false),
    m_needs_update_static (false),
    m_needs_update_bg (false),
    mp_active_service (0),
    m_mouse_pressed_state (false),
    m_mouse_buttons (0),
    m_in_mouse_move (false),
    m_mouse_inside (false),
    m_cursor (lay::Cursor::none),
    m_default_cursor (lay::Cursor::none),
    m_widget_width (0),
    m_widget_height (0),
    m_image_updated (false)
{
  m_objects.changed ().add (this, &ViewObjectUI::objects_changed);

#if defined(HAVE_QT)
  mp_widget = 0;
#endif
}

ViewObjectUI::~ViewObjectUI ()
{
  //  release any mouse grabs now
  while (m_grabbed.begin () != m_grabbed.end ()) {
    ungrab_mouse (*m_grabbed.begin ());
  }

  while (! m_services.empty ()) {
    delete m_services.front ();
  }
}

#if defined(HAVE_QT)
void
ViewObjectUI::init_ui (QWidget *parent)
{
  //  we rely on the parent to delete the UI widget
  tl_assert (parent != 0);
  tl_assert (mp_widget == 0);

  mp_widget = new ViewObjectQWidget (parent, this);
  mp_widget->setMouseTracking (true);
  mp_widget->setAcceptDrops (true);
}
#endif

void
ViewObjectUI::register_service (lay::ViewService *svc)
{
  m_services.push_back (svc);
}

void
ViewObjectUI::unregister_service (lay::ViewService *svc)
{
  if (mp_active_service == svc) {
    mp_active_service = 0;
  }

  //  make sure the service no longer has the mouse
  ungrab_mouse (svc);

  for (std::list<lay::ViewService *>::iterator s = m_services.begin(); s != m_services.end (); ++s) {
    if (*s == svc) {
      m_services.erase (s);
      break;
    }
  }
}

void
ViewObjectUI::activate (lay::ViewService *service)
{
  if (mp_active_service != service) {
    if (mp_active_service) {
BEGIN_PROTECTED  
      mp_active_service->deactivated ();
END_PROTECTED
    }
    mp_active_service = 0;
    for (std::list<lay::ViewService *>::iterator s = m_services.begin(); s != m_services.end (); ++s) {
      if (*s == service) {
        mp_active_service = service;
        break;
      }
    }
    if (mp_active_service) {
BEGIN_PROTECTED  
      mp_active_service->activated ();
END_PROTECTED
    }
  }
}

void 
ViewObjectUI::set_cursor (lay::Cursor::cursor_shape cursor)
{
  m_cursor = cursor;
}

void
ViewObjectUI::set_default_cursor (lay::Cursor::cursor_shape cursor)
{
  if (cursor != m_default_cursor) {
    m_default_cursor = cursor;
#if defined(HAVE_QT)
    if (m_cursor == lay::Cursor::none && mp_widget) {
      if (m_default_cursor == lay::Cursor::none) {
        mp_widget->unsetCursor ();
      } else {
        mp_widget->setCursor (lay::Cursor::qcursor (m_default_cursor));
      }
    }
#endif
  }
}

void
ViewObjectUI::ensure_entered ()
{
  if (! m_mouse_inside) {
    send_enter_event ();
  }
}

void 
ViewObjectUI::begin_mouse_event (lay::Cursor::cursor_shape cursor)
{
  m_cursor = cursor;
}

void 
ViewObjectUI::end_mouse_event ()
{
#if defined(HAVE_QT)
  if (mp_widget) {
    if (m_cursor == lay::Cursor::none) {
      if (m_default_cursor == lay::Cursor::none) {
        mp_widget->unsetCursor ();
      } else {
        mp_widget->setCursor (lay::Cursor::qcursor (m_default_cursor));
      }
    } else if (m_cursor != lay::Cursor::keep) {
      mp_widget->setCursor (lay::Cursor::qcursor (m_cursor));
    }
  }
#endif
}

void
ViewObjectUI::send_key_press_event (unsigned int key, unsigned int buttons)
{
  bool done = false;
  if (mp_active_service) {
    done = (mp_active_service->enabled () && mp_active_service->key_event (key, buttons));
  }

  if (! done) {
    key_event (key, buttons);
  }
}

void
ViewObjectUI::do_mouse_move ()
{
  m_in_mouse_move = true;

  if (m_mouse_pressed_state &&
    (abs (m_mouse_pos.x () - m_mouse_pressed.x ()) > click_tolerance || abs (m_mouse_pos.y () - m_mouse_pressed.y ()) > click_tolerance)) {

    begin_mouse_event (lay::Cursor::none);

    m_mouse_pressed_state = false;

    bool done = false;

    db::DPoint p = pixel_to_um (m_mouse_pressed);

    for (std::list<ViewService *>::iterator g = m_grabbed.begin (); !done && g != m_grabbed.end (); ) {
      std::list<ViewService *>::iterator gg = g;
      ++gg;
      done = ((*g)->enabled () && (*g)->mouse_press_event (p, m_mouse_buttons, true));
      g = gg;
    }

    if (! done && mp_active_service) {
      done = (mp_active_service->enabled () && mp_active_service->mouse_press_event (p, m_mouse_buttons, true));
    }

    service_iterator svc = begin_services ();
    while (svc != end_services () && !done) {
      service_iterator next = svc;
      ++next;
      done = ((*svc)->enabled () && (*svc)->mouse_press_event (p, m_mouse_buttons, false));
      svc = next;
    }

    if (! done) {
      mouse_press_event (p, m_mouse_buttons);
    }

    end_mouse_event ();

  }

  if (! m_mouse_pressed_state) {

    begin_mouse_event (lay::Cursor::none);

    bool done = false;

    db::DPoint p = pixel_to_um (m_mouse_pos);

    for (std::list<ViewService *>::iterator g = m_grabbed.begin (); !done && g != m_grabbed.end (); ) {
      std::list<ViewService *>::iterator gg = g;
      ++gg;
      done = ((*g)->enabled () && (*g)->mouse_move_event (p, m_mouse_buttons, true));
      g = gg;
    }

    if (! done && mp_active_service) {
      done = (mp_active_service->enabled () && mp_active_service->mouse_move_event (p, m_mouse_buttons, true));
    }

    service_iterator svc = begin_services ();
    while (svc != end_services () && !done) {
      service_iterator next = svc;
      ++next;
      done = ((*svc)->enabled () && (*svc)->mouse_move_event (p, m_mouse_buttons, false));
      svc = next;
    }

    if (! done) {
      mouse_move_event (p, m_mouse_buttons);
    }

    end_mouse_event ();

  }

  m_in_mouse_move = false;
}

void
ViewObjectUI::send_mouse_move_event (const db::DPoint &pt, unsigned int buttons)
{
  ensure_entered ();
  m_mouse_pos = pt;
  m_mouse_buttons = buttons;
  do_mouse_move ();
}

void
ViewObjectUI::send_leave_event ()
{
  try {

    bool done = false;

    for (std::list<ViewService *>::iterator g = m_grabbed.begin (); !done && g != m_grabbed.end (); ) {
      std::list<ViewService *>::iterator gg = g;
      ++gg;
      done = ((*g)->enabled () && (*g)->leave_event (true));
      g = gg;
    }

    if (! done && mp_active_service) {
      done = (mp_active_service->enabled () && mp_active_service->leave_event (true));
    }

    service_iterator svc = begin_services ();
    while (svc != end_services () && !done) {
      service_iterator next = svc;
      ++next;
      done = ((*svc)->enabled () && (*svc)->leave_event (false));
      svc = next;
    }

    if (! done) {
      leave_event ();
    }

    end_mouse_event ();

    m_mouse_inside = false;

  } catch (...) {
    m_mouse_inside = false;
    throw;
  }
}

void
ViewObjectUI::send_enter_event ()
{
  m_mouse_inside = true;

  begin_mouse_event ();

  bool done = false;

  for (std::list<ViewService *>::iterator g = m_grabbed.begin (); !done && g != m_grabbed.end (); ) {
    std::list<ViewService *>::iterator gg = g;
    ++gg;
    done = ((*g)->enabled () && (*g)->enter_event (true));
    g = gg;
  }

  if (! done && mp_active_service) {
    done = (mp_active_service->enabled () && mp_active_service->enter_event (true));
  }

  service_iterator svc = begin_services ();
  while (svc != end_services () && !done) {
    service_iterator next = svc;
    ++next;
    done = ((*svc)->enabled () && (*svc)->enter_event (false));
    svc = next;
  }

  if (! done) {
    enter_event ();
  }

  end_mouse_event ();
}

void
ViewObjectUI::send_mouse_press_event (const db::DPoint &pt, unsigned int buttons)
{
  ensure_entered ();
#if defined(HAVE_QT)
  if (mp_widget) {
    mp_widget->setFocus ();
  }
#endif

  m_mouse_pos = pt;
  m_mouse_pressed = m_mouse_pos;

  m_mouse_buttons = buttons;

  m_mouse_pressed_state = true;
}

void
ViewObjectUI::send_mouse_double_clicked_event (const db::DPoint &pt, unsigned int buttons)
{
  ensure_entered ();
  begin_mouse_event (lay::Cursor::none);

#if defined(HAVE_QT)
  if (mp_widget) {
    mp_widget->setFocus ();
  }
#endif

  bool done = false;

  m_mouse_pos = pt;
  m_mouse_pressed = m_mouse_pos;
  m_mouse_pressed_state = false;

  db::DPoint p = pixel_to_um (m_mouse_pos);

  for (std::list<ViewService *>::iterator g = m_grabbed.begin (); !done && g != m_grabbed.end (); ) {
    std::list<ViewService *>::iterator gg = g;
    ++gg;
    done = ((*g)->m_enabled && (*g)->mouse_double_click_event (p, buttons, true));
    g = gg;
  }

  if (! done && mp_active_service) {
    done = (mp_active_service->enabled () && mp_active_service->mouse_double_click_event (p, buttons, true));
  }

  service_iterator svc = begin_services ();
  while (svc != end_services () && !done) {
    service_iterator next = svc;
    ++next;
    done = ((*svc)->enabled () && (*svc)->mouse_double_click_event (p, buttons, false));
    svc = next;
  }

  if (! done) {
    mouse_double_click_event (p, buttons);
  }

  end_mouse_event ();
}

void
ViewObjectUI::send_mouse_release_event (const db::DPoint &pt, unsigned int /*buttons*/)
{
  try {

    ensure_entered ();
    begin_mouse_event ();

    bool done = false;

    m_mouse_pos = pt;
    db::DPoint p = pixel_to_um (m_mouse_pos);

    for (std::list<ViewService *>::iterator g = m_grabbed.begin (); !done && g != m_grabbed.end (); ) {
      std::list<ViewService *>::iterator gg = g;
      ++gg;
      if (m_mouse_pressed_state) {
        done = (*g)->enabled () && (*g)->mouse_click_event (p, m_mouse_buttons, true);
      } else {
        done = (*g)->enabled () && (*g)->mouse_release_event (p, m_mouse_buttons, true);
      }
      g = gg;
    }

    if (! done && mp_active_service && mp_active_service->enabled ()) {
      if (m_mouse_pressed_state) {
        done = mp_active_service->mouse_click_event (p, m_mouse_buttons, true);
      } else {
        done = mp_active_service->mouse_release_event (p, m_mouse_buttons, true);
      }
    }

    service_iterator svc = begin_services ();
    while (svc != end_services () && !done) {
      service_iterator next = svc;
      ++next;
      if ((*svc)->enabled ()) {
        if (m_mouse_pressed_state) {
          done = (*svc)->mouse_click_event (p, m_mouse_buttons, false);
        } else {
          done = (*svc)->mouse_release_event (p, m_mouse_buttons, false);
        }
      }
      svc = next;
    }

    if (! done) {
      if (m_mouse_pressed_state) {
        mouse_click_event (p, m_mouse_buttons);
      } else {
        mouse_release_event (p, m_mouse_buttons);
      }
    }

    end_mouse_event ();

    m_mouse_pressed_state = false;

  } catch (...) {
    m_mouse_pressed_state = false;
    throw;
  }
}

void
ViewObjectUI::send_wheel_event (int delta, bool horizontal, const db::DPoint &pt, unsigned int buttons)
{
  ensure_entered ();
  begin_mouse_event ();

  db::DPoint p = pixel_to_um (pt);

  bool done = false;

  for (std::list<ViewService *>::iterator g = m_grabbed.begin (); !done && g != m_grabbed.end (); ) {
    std::list<ViewService *>::iterator gg = g;
    ++gg;
    done = ((*g)->enabled () && (*g)->wheel_event (delta, horizontal, p, buttons, true));
    g = gg;
  }

  if (! done && mp_active_service) {
    done = (mp_active_service->enabled () && mp_active_service->wheel_event (delta, horizontal, p, buttons, true));
  }

  service_iterator svc = begin_services ();
  while (svc != end_services () && !done) {
    service_iterator next = svc;
    ++next;
    done = ((*svc)->enabled () && (*svc)->wheel_event (delta, horizontal, p, buttons, false));
    svc = next;
  }

  if (! done) {
    wheel_event (delta, horizontal, p, buttons);
  }

  end_mouse_event ();
}


void
ViewObjectUI::resize (unsigned int w, unsigned int h)
{
  m_widget_width = w;
  m_widget_height = h;

#if defined(HAVE_QT)
  if (mp_widget) {
    mp_widget->resize (w, h);
  }
#endif

  //  don't wait until the layout system informs us - which may never take place when
  //  the widget isn't shown. In the non-Qt case we need it anyway here.
  resize_event (w, h);
}

int
ViewObjectUI::widget_height () const
{
#if defined(HAVE_QT)
  return mp_widget ? mp_widget->height () : m_widget_height;
#else
  return m_widget_height;
#endif
}

int
ViewObjectUI::widget_width () const
{
#if defined(HAVE_QT)
  return mp_widget ? mp_widget->width () : m_widget_width;
#else
  return m_widget_width;
#endif
}

db::DPoint
ViewObjectUI::pixel_to_um (const db::Point &pt) const
{
  return m_trans.inverted () * db::DPoint (pt.x (), widget_height () - 1 - pt.y ());
}

db::DPoint
ViewObjectUI::pixel_to_um (const db::DPoint &pt) const
{
  return m_trans.inverted () * db::DPoint (pt.x (), widget_height () - 1 - pt.y ());
}

void
ViewObjectUI::mouse_event_trans (const db::DCplxTrans &trans)
{
  if (trans != m_trans) {
    m_trans = trans;
    //  issue a move event in order to reposition the mouse in the new coordinate system
    //  since this may be called from within a mouse move event handler we need the recursion sentinel
    if (! m_in_mouse_move) {
      do_mouse_move ();
    }
  }
}

void 
ViewObjectUI::drag_cancel ()
{
  for (service_iterator svc = begin_services (); svc != end_services (); ++svc) {
    (*svc)->drag_cancel ();
  }
}

namespace 
{
  struct z_order_compare_f
  {
    bool operator() (lay::BackgroundViewObject *a, lay::BackgroundViewObject *b) const
    {
      int za = a->z_order ();
      int zb = b->z_order ();
      return za < zb;
    }
  };
}

void 
ViewObjectUI::do_render_bg (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas)
{
  m_needs_update_bg = false;

  std::vector<lay::BackgroundViewObject *> bg_objects;

  for (background_object_iterator obj = begin_background_objects (); obj != end_background_objects (); ++obj) {
    if (obj->is_visible ()) {
      bg_objects.push_back (&*obj);
    }
  }

  // Sort objects by z-order
  tl::sort (bg_objects.begin (), bg_objects.end (), z_order_compare_f ());

  for (std::vector<lay::BackgroundViewObject *>::const_iterator obj = bg_objects.begin (); obj != bg_objects.end (); ++obj) {
    BEGIN_PROTECTED_SILENT
    (*obj)->render_bg (vp, canvas);
    END_PROTECTED_SILENT
  }
}

void 
ViewObjectUI::do_render (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas, bool st)
{
  if (st) {
    m_needs_update_static = false;
  }

  for (object_iterator obj = begin_objects (); obj != end_objects (); ++obj) {
    if (obj->m_static == st && obj->is_visible () && (! m_view_objects_dismissed || ! obj->get_dismissable ())) {
      BEGIN_PROTECTED_SILENT
      obj->render (vp, canvas);
      END_PROTECTED_SILENT
    }
  }

  canvas.sort_planes ();
}

void 
ViewObjectUI::grab_mouse (ViewService *obj, bool a)
{
  obj->m_abs_grab = a; // not used currently

  //  only add to m_grabbed if not grabbed already
  std::list<ViewService *>::iterator g;
  for (g = m_grabbed.begin (); g != m_grabbed.end () && *g != obj; ++g) {
    ;
  }
  if (g == m_grabbed.end ()) {
    m_grabbed.push_front (obj);
  }
}

void 
ViewObjectUI::ungrab_mouse (ViewService *obj)
{
  std::list<ViewService *>::iterator g;
  for (g = m_grabbed.begin (); g != m_grabbed.end () && *g != obj; ++g) {
    ;
  }
  if (g != m_grabbed.end ()) {
    m_grabbed.erase (g);
  }
}

void 
ViewObjectUI::freeze (ViewObject *obj)
{
  if (! obj->m_static) {
    obj->m_static = true;
    m_needs_update_static = true;
    //  no update needed since the display will not change through this.
  }
}

void 
ViewObjectUI::thaw (ViewObject *obj)
{
  if (obj->m_static) {
    obj->m_static = false;
    m_needs_update_static = true;
    //  no update needed since the display will not change through this.
  }
}

#if !defined(HAVE_QT)
void
ViewObjectUI::update ()
{
  //  NOTE: this does not need to be thread-safe as we make sure (as in Qt) that update() is always called from the main thread.
  m_image_updated = true;
}

bool
ViewObjectUI::image_updated ()
{
  bool f = m_image_updated;
  m_image_updated = false;
  return f;
}
#else
void
ViewObjectUI::update ()
{
  if (mp_widget) {
    mp_widget->update ();
  }
}
#endif

void
ViewObjectUI::resize_event (unsigned int /*w*/, unsigned int /*h*/)
{
  //  .. nothing yet ..
}

void
ViewObjectUI::paint_event ()
{
  //  .. nothing yet ..
}

void
ViewObjectUI::gtf_probe ()
{
  //  .. nothing yet ..
}

void
ViewObjectUI::touch ()
{
  if (! m_needs_update_static) {
    m_needs_update_static = true;
    update ();
  }
}

void
ViewObjectUI::touch_bg ()
{
  if (! m_needs_update_bg) {
    m_needs_update_bg = true;
    update ();
  }
}

void
ViewObjectUI::set_dismiss_view_objects (bool dismiss)
{
  if (dismiss != m_view_objects_dismissed) {
    m_view_objects_dismissed = dismiss;
    touch ();
    update ();
  }
}

void 
ViewObjectUI::objects_changed ()
{
  touch ();
  update ();
}

db::DBox 
ViewObjectUI::mouse_event_viewport () const
{
  db::DPoint p1 = m_trans.inverted () * db::DPoint (0, 0);
  db::DPoint p2 = m_trans.inverted () * db::DPoint (widget_width (), widget_height ());
  return db::DBox (p1, p2);
}

// ---------------------------------------------------------------
//  BitmapViewObjectCanvas implementation

BitmapViewObjectCanvas::BitmapViewObjectCanvas ()
  : ViewObjectCanvas (), m_renderer (1, 1, 1.0), m_width (1), m_height (1), m_resolution (1.0)
{
  // .. nothing yet ..
}

BitmapViewObjectCanvas::BitmapViewObjectCanvas (unsigned int width, unsigned int height, double resolution)
  : ViewObjectCanvas (), m_renderer (width, height, resolution), 
    m_width (width), m_height (height), m_resolution (resolution)
{
  // .. nothing yet ..
}

BitmapViewObjectCanvas::~BitmapViewObjectCanvas ()
{
  clear_fg_bitmaps ();
}

lay::CanvasPlane *
BitmapViewObjectCanvas::plane (const lay::ViewOp &style)
{
  std::map <lay::ViewOp, unsigned int>::iterator b = m_fg_bitmap_table.find (style);
  if (b == m_fg_bitmap_table.end ()) {

    //  we need to create a new plane
    m_fg_bitmap_table.insert (std::make_pair (style, (unsigned int) mp_alloc_bitmaps.size ()));
    lay::Bitmap *bm = new lay::Bitmap (m_width, m_height, m_resolution);
    mp_fg_bitmaps.push_back (bm);
    mp_alloc_bitmaps.push_back (bm);
    m_fg_view_ops.push_back (style);
    return bm;
    
  } else {
    //  we can recycle a current one
    return mp_alloc_bitmaps [b->second];
  }
}

lay::CanvasPlane *
BitmapViewObjectCanvas::plane (const std::vector<lay::ViewOp> &style)
{
  std::map <std::vector<lay::ViewOp>, unsigned int>::iterator b = m_fgv_bitmap_table.find (style);
  if (b == m_fgv_bitmap_table.end ()) {

    //  we need to create a new bitmap
    m_fgv_bitmap_table.insert (std::make_pair (style, (unsigned int) mp_alloc_bitmaps.size ()));
    lay::Bitmap *bm = new lay::Bitmap (m_width, m_height, m_resolution);
    mp_alloc_bitmaps.push_back (bm);
    for (std::vector<lay::ViewOp>::const_iterator s = style.begin (); s != style.end (); ++s) {
      mp_fg_bitmaps.push_back (bm);
      m_fg_view_ops.push_back (*s);
    }
    return bm;
    
  } else {
    //  we can recycle a current one
    return mp_alloc_bitmaps [b->second];
  }
}

void 
BitmapViewObjectCanvas::clear_fg_bitmaps ()
{
  for (std::vector <lay::Bitmap *>::iterator i = mp_alloc_bitmaps.begin (); i != mp_alloc_bitmaps.end (); ++i) {
    if (*i) {
      delete *i;
    }
  }
  mp_alloc_bitmaps.clear ();
  mp_fg_bitmaps.clear ();
  m_fg_view_ops.clear ();
  m_fg_bitmap_table.clear ();
  m_fgv_bitmap_table.clear ();
}

void
BitmapViewObjectCanvas::sort_planes ()
{
  //  sort the planes by view operator - this ensures a certain plane order as implied by
  //  the plane index of the plane operators.
  std::vector <std::pair <lay::ViewOp, lay::Bitmap *> > bitmaps;
  bitmaps.reserve (mp_fg_bitmaps.size ());
  for (unsigned int i = 0; i < mp_fg_bitmaps.size (); ++i) {
    bitmaps.push_back (std::make_pair (m_fg_view_ops [i], mp_fg_bitmaps [i]));
  }
  tl::sort (bitmaps.begin (), bitmaps.end ());
  for (unsigned int i = 0; i < mp_fg_bitmaps.size (); ++i) {
    m_fg_view_ops [i] = bitmaps [i].first;
    mp_fg_bitmaps [i] = bitmaps [i].second;
  }
}

void 
BitmapViewObjectCanvas::set_size (unsigned int width, unsigned int height, double resolution)
{
  m_renderer = lay::BitmapRenderer (width, height, resolution);
  m_width = width;
  m_height = height;
  m_resolution = resolution;
}

void 
BitmapViewObjectCanvas::set_size (unsigned int width, unsigned int height)
{
  m_renderer = lay::BitmapRenderer (width, height, m_resolution);
  m_width = width;
  m_height = height;
}

void 
BitmapViewObjectCanvas::set_size (double resolution)
{
  m_renderer = lay::BitmapRenderer (m_width, m_height, resolution);
  m_resolution = resolution;
}

tl::PixelBuffer *
BitmapViewObjectCanvas::bg_image ()
{
  return 0;
}

tl::BitmapBuffer *
BitmapViewObjectCanvas::bg_bitmap ()
{
  return 0;
}


}


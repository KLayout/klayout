
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


#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QCoreApplication>

#include "layViewObject.h"
#include "layCanvasPlane.h"
#include "layBitmap.h"
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
//  Implementation of DragDropDataBase 

const char *drag_drop_mime_type ()
{
  return "application/klayout-ddd";
}

QMimeData *
DragDropDataBase::to_mime_data () const
{
  QMimeData *mimeData = new QMimeData();
  mimeData->setData (QString::fromUtf8 (drag_drop_mime_type ()), serialized ());
  return mimeData;
}

// ---------------------------------------------------------------
//  Implementation of CellDragDropData

QByteArray
CellDragDropData::serialized () const
{
  QByteArray data;
  QDataStream stream (&data, QIODevice::WriteOnly);

  stream << QString::fromUtf8 ("CellDragDropData");
  stream << (quintptr) mp_layout;
  stream << (quintptr) mp_library;
  stream << m_cell_index;
  stream << m_is_pcell;
  stream << int (m_pcell_params.size ());
  for (std::vector<tl::Variant>::const_iterator i = m_pcell_params.begin (); i != m_pcell_params.end (); ++i) {
    stream << tl::to_qstring (i->to_parsable_string ());
  }

  return data;
}

bool
CellDragDropData::deserialize (const QByteArray &ba)
{
  QDataStream stream (const_cast<QByteArray *> (&ba), QIODevice::ReadOnly);

  QString tag;
  stream >> tag;

  if (tag == QString::fromUtf8 ("CellDragDropData")) {

    quintptr p = 0;
    stream >> p;
    mp_layout = reinterpret_cast <const db::Layout *> (p);
    stream >> p;
    mp_library = reinterpret_cast <const db::Library *> (p);
    stream >> m_cell_index;
    stream >> m_is_pcell;

    m_pcell_params.clear ();
    int n = 0;
    stream >> n;
    while (n-- > 0) {
      QString s;
      stream >> s;
      std::string stl_s = tl::to_string (s);
      tl::Extractor ex (stl_s.c_str ());
      m_pcell_params.push_back (tl::Variant ());
      ex.read (m_pcell_params.back ());
    }

    return true;

  } else {

    return false;

  }
}

// ---------------------------------------------------------------
//  A helper function to convert a Qt modifier/buttons to klayout buttons

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
         ((b & Qt::MidButton) ? MidButton : 0) |
         ((b & Qt::RightButton) ? RightButton : 0) |
         ((m & Qt::ShiftModifier) != 0 ? ShiftButton : 0) |
         ((m & Qt::ControlModifier) != 0 ? ControlButton : 0) |
         ((m & Qt::AltModifier) != 0 ? AltButton : 0);
}

// ---------------------------------------------------------------
//  BackgroundViewObject implementation

BackgroundViewObject::BackgroundViewObject (ViewObjectWidget *widget)
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

ViewObject::ViewObject (ViewObjectWidget *widget, bool _static)
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

ViewService::ViewService (ViewObjectWidget *widget)
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
//  ViewObject implementation

ViewObjectWidget::ViewObjectWidget (QWidget *parent, const char *name)
  : QWidget (parent), 
    m_view_objects_dismissed (false),
    m_needs_update_static (false),
    m_needs_update_bg (false),
    mp_active_service (0),
    m_mouse_pressed_state (false),
    m_mouse_buttons (0),
    m_in_mouse_move (false),
    m_mouse_inside (false),
    m_cursor (lay::Cursor::none),
    m_default_cursor (lay::Cursor::none)
{
  setMouseTracking (true); 
  setObjectName (QString::fromUtf8 (name));
  setAcceptDrops (true);

  m_objects.changed ().add (this, &ViewObjectWidget::objects_changed);
}

ViewObjectWidget::~ViewObjectWidget ()
{
  //  release any mouse grabs now
  while (m_grabbed.begin () != m_grabbed.end ()) {
    ungrab_mouse (*m_grabbed.begin ());
  }

  while (! m_services.empty ()) {
    delete m_services.front ();
  }
}

void
ViewObjectWidget::register_service (lay::ViewService *svc)
{
  m_services.push_back (svc);
}

void
ViewObjectWidget::unregister_service (lay::ViewService *svc)
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
ViewObjectWidget::activate (lay::ViewService *service)
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
ViewObjectWidget::set_cursor (lay::Cursor::cursor_shape cursor)
{
  m_cursor = cursor;
}

void
ViewObjectWidget::set_default_cursor (lay::Cursor::cursor_shape cursor)
{
  if (cursor != m_default_cursor) {
    m_default_cursor = cursor;
    if (m_cursor == lay::Cursor::none) {
      if (m_default_cursor == lay::Cursor::none) {
        unsetCursor ();
      } else {
        setCursor (lay::Cursor::qcursor (m_default_cursor));
      }
    }
  }
}

void
ViewObjectWidget::ensure_entered ()
{
  if (! m_mouse_inside) {
    enterEvent (0);
  }
}

void 
ViewObjectWidget::begin_mouse_event (lay::Cursor::cursor_shape cursor)
{
  m_cursor = cursor;
}

void 
ViewObjectWidget::end_mouse_event ()
{
  if (m_cursor == lay::Cursor::none) {
    if (m_default_cursor == lay::Cursor::none) {
      unsetCursor ();
    } else {
      setCursor (lay::Cursor::qcursor (m_default_cursor));
    }
  } else if (m_cursor != lay::Cursor::keep) {
    setCursor (lay::Cursor::qcursor (m_cursor));
  }
}

bool
ViewObjectWidget::focusNextPrevChild (bool /*next*/)
{
  return false;
}

void 
ViewObjectWidget::keyPressEvent (QKeyEvent *e)
{
BEGIN_PROTECTED  

  unsigned int buttons = qt_to_buttons (Qt::MouseButtons (), e->modifiers ());

  bool done = false;
  if (mp_active_service) {
    done = (mp_active_service->enabled () && mp_active_service->key_event ((unsigned int) e->key(), buttons));
  }

  if (! done) {
    key_event ((unsigned int) e->key (), buttons);
  }

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

void 
ViewObjectWidget::dragEnterEvent (QDragEnterEvent *event)
{
BEGIN_PROTECTED  
  const DragDropDataBase *dd = get_drag_drop_data (event->mimeData ());
  if (dd) {

    db::DPoint p = pixel_to_um (event->pos ());

    bool done = drag_enter_event (p, dd);
    service_iterator svc = begin_services ();
    while (svc != end_services () && !done) {
      service_iterator next = svc;
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

void 
ViewObjectWidget::dragLeaveEvent (QDragLeaveEvent * /*event*/)
{
BEGIN_PROTECTED  

  drag_leave_event ();
  service_iterator svc = begin_services ();
  while (svc != end_services ()) {
    service_iterator next = svc;
    ++next;
    (*svc)->drag_leave_event ();
    svc = next;
  }

END_PROTECTED
}

void 
ViewObjectWidget::dragMoveEvent (QDragMoveEvent *event)
{
BEGIN_PROTECTED  

  const DragDropDataBase *dd = get_drag_drop_data (event->mimeData ());
  if (dd) {

    db::DPoint p = pixel_to_um (event->pos ());

    bool done = drag_move_event (p, dd);
    service_iterator svc = begin_services ();
    while (svc != end_services () && !done) {
      service_iterator next = svc;
      ++next;
      done = (*svc)->drag_move_event (p, dd);
      svc = next;
    }

  }

END_PROTECTED
}

void 
ViewObjectWidget::dropEvent (QDropEvent *event)
{
BEGIN_PROTECTED  

  const DragDropDataBase *dd = get_drag_drop_data (event->mimeData ());
  if (dd) {

    db::DPoint p = pixel_to_um (event->pos ());

    bool done = drop_event (p, dd);
    service_iterator svc = begin_services ();
    while (svc != end_services () && !done) {
      service_iterator next = svc;
      ++next;
      done = (*svc)->drop_event (p, dd);
      svc = next;
    }

  }

END_PROTECTED
}

void 
ViewObjectWidget::mouseMoveEvent (QMouseEvent *e)
{
BEGIN_PROTECTED  
  ensure_entered ();
  m_mouse_pos = e->pos ();
  m_mouse_buttons = qt_to_buttons (e->buttons (), e->modifiers ());
  do_mouse_move ();
END_PROTECTED
}

void
ViewObjectWidget::do_mouse_move ()
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
ViewObjectWidget::mouseDoubleClickEvent (QMouseEvent *e)
{
BEGIN_PROTECTED  
  ensure_entered ();
  begin_mouse_event (lay::Cursor::none);

  setFocus ();

  bool done = false; 

  m_mouse_pos = e->pos ();
  m_mouse_pressed = e->pos ();
  m_mouse_pressed_state = false;

  unsigned int buttons = qt_to_buttons (e->buttons (), e->modifiers ());

  db::DPoint p = pixel_to_um (e->pos ());

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
END_PROTECTED
}

void 
ViewObjectWidget::enterEvent (QEvent * /*event*/)
{
BEGIN_PROTECTED  
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
END_PROTECTED
}

void 
ViewObjectWidget::leaveEvent (QEvent * /*event*/)
{
BEGIN_PROTECTED  
  begin_mouse_event ();

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
END_PROTECTED

  m_mouse_inside = false;
}

void 
ViewObjectWidget::wheelEvent (QWheelEvent *e)
{
BEGIN_PROTECTED  
  ensure_entered ();
  begin_mouse_event ();

  e->ignore ();

  bool done = false; 

  unsigned int buttons = qt_to_buttons (e->buttons (), e->modifiers ());
  bool horizontal = (e->orientation () == Qt::Horizontal);

  db::DPoint p = pixel_to_um (e->pos ());

  for (std::list<ViewService *>::iterator g = m_grabbed.begin (); !done && g != m_grabbed.end (); ) {
    std::list<ViewService *>::iterator gg = g;
    ++gg;
    done = ((*g)->enabled () && (*g)->wheel_event (e->delta (), horizontal, p, buttons, true));
    g = gg;
  }

  if (! done && mp_active_service) {
    done = (mp_active_service->enabled () && mp_active_service->wheel_event (e->delta (), horizontal, p, buttons, true));
  }

  service_iterator svc = begin_services ();
  while (svc != end_services () && !done) {
    service_iterator next = svc;
    ++next;
    done = ((*svc)->enabled () && (*svc)->wheel_event (e->delta (), horizontal, p, buttons, false));
    svc = next;
  }

  if (! done) {
    wheel_event (e->delta (), horizontal, p, buttons);
  }

  end_mouse_event ();
END_PROTECTED
}

void 
ViewObjectWidget::mousePressEvent (QMouseEvent *e)
{
  ensure_entered ();
  setFocus ();

  m_mouse_pos = e->pos ();
  m_mouse_pressed = e->pos ();

  m_mouse_buttons = qt_to_buttons (e->buttons (), e->modifiers ());
  
  m_mouse_pressed_state = true;
}

void 
ViewObjectWidget::mouseReleaseEvent (QMouseEvent *e)
{
BEGIN_PROTECTED  
  ensure_entered ();
  begin_mouse_event ();

  bool done = false; 

  m_mouse_pos = e->pos ();
  db::DPoint p = pixel_to_um (e->pos ());

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
END_PROTECTED

  m_mouse_pressed_state = false;
}

db::DPoint
ViewObjectWidget::pixel_to_um (const QPoint &pt) const
{
  return m_trans.inverted () * db::DPoint (pt.x (), height () - 1 - pt.y ());
}

void
ViewObjectWidget::mouse_event_trans (const db::DCplxTrans &trans)
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
ViewObjectWidget::drag_cancel ()
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
ViewObjectWidget::do_render_bg (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas)
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
ViewObjectWidget::do_render (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas, bool st)
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
ViewObjectWidget::grab_mouse (ViewService *obj, bool a)
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
ViewObjectWidget::ungrab_mouse (ViewService *obj)
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
ViewObjectWidget::freeze (ViewObject *obj)
{
  if (! obj->m_static) {
    obj->m_static = true;
    m_needs_update_static = true;
    //  no update needed since the display will not change through this.
  }
}

void 
ViewObjectWidget::thaw (ViewObject *obj)
{
  if (obj->m_static) {
    obj->m_static = false;
    m_needs_update_static = true;
    //  no update needed since the display will not change through this.
  }
}

void
ViewObjectWidget::touch ()
{
  if (! m_needs_update_static) {
    m_needs_update_static = true;
    update ();
  }
}

void
ViewObjectWidget::touch_bg ()
{
  if (! m_needs_update_bg) {
    m_needs_update_bg = true;
    update ();
  }
}

void
ViewObjectWidget::set_dismiss_view_objects (bool dismiss)
{
  if (dismiss != m_view_objects_dismissed) {
    m_view_objects_dismissed = dismiss;
    touch ();
    update ();
  }
}

void 
ViewObjectWidget::objects_changed ()
{
  touch ();
  update ();
}

db::DBox 
ViewObjectWidget::mouse_event_viewport () const
{
  db::DPoint p1 = m_trans.inverted () * db::DPoint (0, 0);
  db::DPoint p2 = m_trans.inverted () * db::DPoint (width (), height ());
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

}


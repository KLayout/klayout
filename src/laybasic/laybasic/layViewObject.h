
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



#ifndef HDR_layViewObject
#define HDR_layViewObject

#include "laybasicCommon.h"

#include <list>
#include <string>
#include <vector>
#include <map>

#include <QPoint>
#include <QByteArray>
#include <QColor>
#include <QWidget>

#include "tlObjectCollection.h"
#include "tlVariant.h"
#include "dbTrans.h"
#include "dbBox.h"
#include "layViewOp.h"
#include "layCursor.h"
#include "layBitmapRenderer.h"

class QMouseEvent;
class QImage;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QMimeData;

namespace db
{
  class Library;
  class Layout;
}

namespace lay {

class Viewport;
class ViewObjectWidget;
class ViewObjectCanvas;
class CanvasPlane;
class Bitmap;

LAYBASIC_PUBLIC const char *drag_drop_mime_type ();

/**
 *  @brief A helper class required to store the drag/drop data
 *
 *  Drag/drop data is basically a collection of key/value pairs. 
 *  A category string is provided to identify the kind of data.
 */

class LAYBASIC_PUBLIC DragDropDataBase
{
public:
  /**
   *  @brief Default constructor
   */
  DragDropDataBase () { }

  /**
   *  @brief Dtor
   */
  virtual ~DragDropDataBase () { } 

  /**
   *  @brief Serializes itself to an QByteArray
   */
  virtual QByteArray serialized () const = 0;

  /**
   *  @brief Try deserialization from an QByteArray
   *
   *  Returns false, if deserialization failed.
   */
  virtual bool deserialize (const QByteArray &ba) = 0;

  /**
   *  @brief Create a QMimeData object from the object
   */
  QMimeData *to_mime_data () const;
};

/**
 *  @brief Drag/drop data for a cell
 */

class LAYBASIC_PUBLIC CellDragDropData
  : public DragDropDataBase
{
public:
  /**
   *  @brief Default ctor
   */
  CellDragDropData ()
    : mp_layout (0), mp_library (0), m_cell_index (0), m_is_pcell (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Specifies drag & drop of a cell
   *
   *  @param layout the layout where the cell lives in 
   *  @param cell_index The index of the cell
   */
  CellDragDropData (const db::Layout *layout, const db::Library *library, db::cell_index_type cell_or_pcell_index, bool is_pcell)
    : mp_layout (layout), mp_library (library), m_cell_index (cell_or_pcell_index), m_is_pcell (is_pcell)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the layout object where the cell lives in
   */
  const db::Layout *layout () const
  {
    return mp_layout;
  }

  /**
   *  @brief Gets the layout object where the cell lives in
   */
  const db::Library *library () const
  {
    return mp_library;
  }

  /**
   *  @brief Gets the index of the cell
   */
  db::cell_index_type cell_index () const
  {
    return m_cell_index;
  }

  /**
   *  @brief Gets a value indicating whether the cell is a pcell
   */
  bool is_pcell () const
  {
    return m_is_pcell;
  }

  /**
   *  @brief Serializes itself to an QByteArray
   */
  virtual QByteArray serialized () const;

  /**
   *  @brief Try deserialization from an QByteArray
   *
   *  Returns false, if deserialization failed.
   */
  bool deserialize (const QByteArray &ba);

private:
  const db::Layout *mp_layout;
  const db::Library *mp_library;
  db::cell_index_type m_cell_index;
  bool m_is_pcell;
};

/**
 *  @brief A view service 
 *
 *  A view service is providing editing or informational services
 *  on the canvas. 
 *  A service can be "active" by requesting mouse
 *  events through a trigger by "select" or "activate" after
 *  which a "grab_mouse" redirects all mouse events to the
 *  view service.
 *  This is an interface that should/must be implemented
 *  by the super class.
 */

class LAYBASIC_PUBLIC ViewService
{
public: 
  /**
   *  @brief Constructor
   */
  ViewService (ViewObjectWidget *widget = 0);

  /**
   *  @brief Destructor
   */
  virtual ~ViewService ();

  /** 
   *  @brief Key press event handler
   *
   *  This method will be called by the ViewObjectWidget object to
   *  dispatch key press events. 
   *
   *  The active service will receive that call and should return true
   *  if the event is taken. Otherwise the event will be passed further.
   */
  virtual bool key_event (unsigned int /*key*/, unsigned int /*buttons*/) { return false; }

  /**
   *  @brief The drag enter event
   *
   *  If something is dragged into the view, this event is triggered.
   *  p will be the position of the event.
   *
   *  The implementation should return true, if the drop is accepted.
   */
  virtual bool drag_enter_event (const db::DPoint & /*p*/, const DragDropDataBase * /*data*/) { return false; }

  /**
   *  @brief The drag move event
   */
  virtual bool drag_move_event (const db::DPoint & /*p*/, const DragDropDataBase * /*data*/) { return false; }

  /**
   *  @brief The drag leave event
   */
  virtual void drag_leave_event () { }

  /**
   *  @brief The drop event
   */
  virtual bool drop_event (const db::DPoint & /*p*/, const DragDropDataBase * /*data*/) { return false; }

  /**
   *  @brief Mouse press event handler
   *
   *  This method will be called by the ViewObjectWidget object to
   *  dispatch mouse press events. First, the objects that grabbed
   *  the mouse will receive the events with prio "true" in the reverse order they
   *  grabbed the mouse (last one first). This loop will terminate
   *  if one of the objects has returned "true". 
   *  If no service has grabbed the mouse or none of them was taking the
   *  event, the active service receives the event with prio set to "true".
   *  If this is not the case, then all objects will receive
   *  the event with "prio" set to "false", unless one of the objects
   *  returns "true".
   *  This event is not sent immediately when the mouse button is pressed
   *  but when a signification movement for the mouse cursor away from the
   *  original position is detected. If the mouse button is released before
   *  that, a mouse_clicked_event is sent rather than a press-move-release
   *  sequence.
   *
   *  @param p The point at which the button was pressed
   *  @param buttons A ored combination of ShiftButton etc.
   *  @return True to terminate dispatcher
   */ 
  virtual bool mouse_press_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/) { return false; }

  /** 
   *  @brief Mouse single-click event handler
   *
   *  Analogeous to mouse_press_event (see above), but sent if the mouse was not moved.
   *  The click event is coincident with the release of the mouse button.
   */
  virtual bool mouse_click_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/) { return false; }

  /** 
   *  @brief Mouse double-click event handler
   *
   *  Analogeous to mouse_press_event (see above), but sent if a double-click was detected.
   */
  virtual bool mouse_double_click_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/) { return false; }

  /** 
   *  @brief Mouse leave event handler
   */
  virtual bool leave_event (bool /*prio*/) { return false; }

  /** 
   *  @brief Mouse enter event handler
   */
  virtual bool enter_event (bool /*prio*/) { return false; }

  /** 
   *  @brief Mouse move event handler
   *
   *  Analogeous to mouse_press_event (see above).
   */
  virtual bool mouse_move_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/) { return false; }

  /** 
   *  @brief Mouse release event handler
   *
   *  Analogeous to mouse_press_event (see above).
   */
  virtual bool mouse_release_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/) { return false; }

  /** 
   *  @brief Wheel event handler
   *
   *  Analogeous to mouse_press_event (see above).
   *
   *  @param delta The rotation angle in eigths of a degree
   *  @param horizonal True, if the horizontal wheel was turned
   *  @param p The position where the mouse currently is at (in micron units)
   */
  virtual bool wheel_event (int /*delta*/, bool /*horizontal*/, const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/) { return false; }

  /**
   *  @brief Set the mouse cursor
   *
   *  This method can be called from a mouse event handler to set the cursor for the view.
   *  It is only effective if called from a mouse event handler.
   */
  void set_cursor (lay::Cursor::cursor_shape cursor);

  /**
   *  @brief Do an update
   *
   *  This method is supposed to update the internals,
   *  specifically upon changing of the coordinate system
   */
  virtual void update ()
  {
    //  The default implementation does nothing 
  }
 
  /**
   *  @brief Accessor to the widget pointer
   */
  ViewObjectWidget *widget () const 
  {  
    return mp_widget;
  }

  /**
   *  @brief This method is called on the service that got activated
   */  
  virtual void activated () { }

  /**
   *  @brief This method is called on the service that got deactivated
   */  
  virtual void deactivated () { }

  /**
   *  @brief This method is called to set the background and text (foreground) color
   */
  virtual void set_colors (QColor /*background*/, QColor /*text*/) { }

  /**
   *  @brief This method is called when a drag operation should be cancelled
   */
  virtual void drag_cancel () { }

  /**
   *  @brief Enable or disable a sevice 
   *
   *  If a service is disabled, it will not receive mouse events 
   */
  void enable (bool en);

  /**
   *  @brief Returns true, if the service is enabled
   */
  bool enabled () const
  {
    return m_enabled;
  }

private:
  friend class ViewObjectWidget;

  ViewObjectWidget *mp_widget;
  bool m_abs_grab;
  bool m_enabled;
};

/**
 *  @brief The background view object 
 *
 *  A background view object is an object that is painted onto the
 *  canvas background and is contained by the view object widget. 
 *  This is an interface that should/must be implemented
 *  by the super class.
 */

class LAYBASIC_PUBLIC BackgroundViewObject
  : virtual public tl::Object
{
public: 
  /**
   *  @brief The constructor
   *
   *  @param widget The widget object that the object is shown on.
   *  @param _static True, if the object is in frozen mode initially
   */
  BackgroundViewObject (ViewObjectWidget *widget = 0);

  /**
   *  @brief The destructor
   */
  virtual ~BackgroundViewObject ();

  /**
   *  @brief Render the object on the background 
   *
   *  This method is supposed to repaint the background part of the object 
   *  on the QImage provided in the canvas.
   */
  virtual void render_bg (const Viewport &vp, ViewObjectCanvas &canvas) = 0;

  /**
   *  @brief Accessor to the widget object pointer
   */
  ViewObjectWidget *widget () const 
  {  
    return const_cast<ViewObjectWidget *> (mp_widget.get());
  }

  /**
   *  @brief Set the visibility state of the view object
   *  
   *  Invisible objects are not drawn
   */
  void visible (bool vis);
     
  /**
   *  @brief Tell the visibility state of the view object
   */
  bool is_visible () const
  {
    return m_visible;
  }
     
  /**
   *  @brief This method tells the widget to update the object on next repaint
   *  
   *  If the object is static, a "touch" is issued on the widget. Otherwise
   *  just a update is issued, resulting in a repaint
   *  This is the preferred method to tell that an object needs repainting.
   *  It can be called multiple times without performance penalty.
   */
  void redraw ();

  /**
   *  @brief Z-Order property
   *
   *  This property controls in which order the background objects are drawn: 
   *  those with a lower z-order value are drawn first. Thus, ones with the 
   *  higher value overwrite them.
   */
  int z_order () const
  {
    return m_z_order;
  }

  /**
   *  @brief Z-Order write accessor
   */
  void z_order (int z);

private:
  friend class ViewObjectWidget;

  BackgroundViewObject (const BackgroundViewObject &d);
  BackgroundViewObject &operator= (const BackgroundViewObject &d);

  tl::weak_ptr<ViewObjectWidget> mp_widget;
  bool m_visible;
  int m_z_order;
};

/**
 *  @brief The view object 
 *
 *  A view object is an object that is painted onto the
 *  canvas and is contained by the view object widget. 
 *  View objects can be static or non-static. Changing a
 *  view object in the non-static case is somewhat more 
 *  efficient and is recommended for dynamic objects.
 *  Static mode is entered with "freeze", non-static mode
 *  with "thaw".
 *  This is an interface that should/must be implemented
 *  by the super class.
 */

class LAYBASIC_PUBLIC ViewObject
  : public tl::Object
{
public: 
  /**
   *  @brief The constructor
   *
   *  @param widget The widget object that the object is shown on.
   *  @param _static True, if the object is in frozen mode initially
   */
  ViewObject (ViewObjectWidget *widget = 0, bool _static = true);

  /**
   *  @brief The destructor
   */
  virtual ~ViewObject ();

  /**
   *  @brief Render the object on the planes provided by the canvas.
   *
   *  This method is supposed to repaint the object on the plane
   *  objects that the canvas can provide upon request with the plane
   *  method.
   */
  virtual void render (const Viewport &vp, ViewObjectCanvas &canvas) = 0;

  /**
   *  @brief Accessor to the widget object pointer
   */
  ViewObjectWidget *widget () const 
  {  
    return const_cast<ViewObjectWidget *> (mp_widget.get());
  }

  /**
   *  @brief Gets a value indicating whether the marker can be dismissed (made invisible)
   *
   *  Markers with this flag set to true can be hidden by using ViewObjectCanvas::show_markers.
   */
  bool get_dismissable () const
  {
    return m_dismissable;
  }

  /**
   *  @brief Sets a value indicating whether the marker can be dismissed (made invisible)
   *
   *  See \\get_dismissable for details.
   */
  void set_dismissable (bool f);

  /**
   *  @brief Set the visibility state of the view object
   *  
   *  Invisible objects are not drawn
   */
  void visible (bool vis);
     
  /**
   *  @brief Tell the visibility state of the view object
   */
  bool is_visible () const
  {
    return m_visible;
  }
     
  /**
   *  @brief This method tells the widget to update the object on next repaint
   *  
   *  If the object is static, a "touch" is issued on the widget. Otherwise
   *  just a update is issued, resulting in a repaint
   *  This is the preferred method to tell that an object needs repainting.
   *  It can be called multiple times without performance penalty.
   */
  void redraw ();

  /**
   *  @brief thaw this object
   *
   *  This is a convenience function that avoids having to store a widget pointer
   */
  void thaw (); 

  /**
   *  @brief freeze this object
   *
   *  This is a convenience function that avoids having to store a widget pointer
   */
  void freeze ();

private:
  friend class ViewObjectWidget;

  ViewObject (const ViewObject &d);
  ViewObject &operator= (const ViewObject &d);

  tl::weak_ptr<ViewObjectWidget> mp_widget;
  bool m_static;
  bool m_visible;
  bool m_dismissable;
};

/**
 *  @brief Describes the button state (supposed to be ored)
 */
enum ButtonState {
  ShiftButton = 1,
  ControlButton = 2,
  AltButton = 4,
  LeftButton = 8,
  MidButton = 16,
  RightButton = 32
};

/**
 *  @brief The view object container
 *
 *  The container holds the view objects and 
 *  manages the the mouse event distribution and 
 *  painting.
 */

class LAYBASIC_PUBLIC ViewObjectWidget 
  : public QWidget,
    public tl::Object
{
public:
  typedef tl::weak_collection<ViewObject>::iterator object_iterator;
  typedef tl::weak_collection<BackgroundViewObject>::iterator background_object_iterator;
  typedef std::list<ViewService *>::const_iterator service_iterator;
  typedef std::list<ViewService *>::const_iterator mouse_receivers_iterator;

  /**
   *  @brief ctor
   */
  ViewObjectWidget (QWidget *view, const char *name);

  /**
   *  @brief dtor
   */
  ~ViewObjectWidget ();

  /**
   *  @brief Cancel all drag operations
   */
  void drag_cancel ();

  /**
   *  @brief CanvasPlane rendering 
   *
   *  This method is supposed to render the object on the planes provided
   *  by the planes() method. 
   *
   *  @param vp The viewport 
   *  @param st True, if only static (frozen) objects are to be redrawn
   */
  void do_render (const Viewport &vp, ViewObjectCanvas &canvas, bool st);

  /**
   *  @brief Background rendering
   *
   *  Objects on the background must provide a paint method that paints
   *  them on the canvas directly.
   *  
   *  @param vp The viewport 
   *  @param canvas The canvas where to paint on
   */
  void do_render_bg (const Viewport &vp, ViewObjectCanvas &canvas);

  /**
   *  @brief Query if any "static" object needs to be redrawn
   *
   *  To reset this flag, call do_render with "st" set to true.
   */
  bool needs_update_static () const
  {
    return m_needs_update_static;
  }

  /**
   *  @brief Query if the background needs to be redrawn
   *
   *  To reset this flag, call do_render_bg 
   */
  bool needs_update_bg () const
  {
    return m_needs_update_bg;
  }

  /**
   *  @brief Puts the object into "static" mode
   */
  void freeze (ViewObject *obj);

  /**
   *  @brief Puts the object into "non-static" mode
   */
  void thaw (ViewObject *obj);

  /**
   *  @brief Marks the widget for update
   *
   *  Beside issuing an "update" request, the widget is also
   *  marked for "static-update", which shall redraw all static
   *  objects also.
   */
  void touch ();

  /**
   *  @brief Marks the widget background for update
   *
   *  Beside issuing an "update" request, the widget is also
   *  marked for "bg-update", which shall redraw including the background.
   */
  void touch_bg ();

  /**
   *  @brief Grab the mouse for the object
   *
   *  This will put the object into the list of objects interested
   *  into mouse events with high priority (prio = true on mouse_* events).
   *  Objects with dragging semantics (press-drag-release) may request
   *  "absolute" mouse grab. This way, all mouse events are passed to the
   *  object. The grab should then in any case be released on mouse button
   *  release.
   */
  void grab_mouse (ViewService *obj, bool abs);

  /**
   *  @brief Remove the object from the list of objects that grabbed the mouse
   *
   *  This will also remove any absolute grabs if there is no other object
   *  that needs this.
   */
  void ungrab_mouse (ViewService *obj);

  /**
   *  @brief Set the mouse cursor
   *
   *  This method can be called from a mouse event handler to set the cursor for the view.
   *  It is only effective if called from a mouse event handler.
   */
  void set_cursor (lay::Cursor::cursor_shape cursor);

  /**
   *  @brief Determine the active service
   */
  lay::ViewService *active_service () const
  {
    return const_cast<lay::ViewService *> (mp_active_service);
  }

  /**
   *  @brief Activate a service
   *
   *  The active service will be the first to receive mouse events  
   *  with prio==true, unless the mouse is grabbed and the event is
   *  taken by the grabber. 
   *  Passing 0 deactivates all services. The service currently active
   *  will get a "deactivated" signal.
   */
  void activate (lay::ViewService *service);

  /**
   *  @brief Services iterator: begin
   */
  service_iterator begin_services () 
  {
    return m_services.begin ();
  }

  /**
   *  @brief Services iterator: end
   */
  service_iterator end_services () 
  {
    return m_services.end ();
  }

  /**
   *  @brief Background objects iterator: begin
   */
  background_object_iterator begin_background_objects () 
  {
    return m_background_objects.begin ();
  }

  /**
   *  @brief Background objects iterator: end
   */
  background_object_iterator end_background_objects () 
  {
    return m_background_objects.end ();
  }

  /**
   *  @brief Objects iterator: begin
   */
  object_iterator begin_objects () 
  {
    return m_objects.begin ();
  }

  /**
   *  @brief Objects iterator: end
   */
  object_iterator end_objects () 
  {
    return m_objects.end ();
  }

  /** 
   *  @brief Remaining leave event handler
   *
   *  This event handler is called if no ViewObject requested handling
   *  of this event.
   */
  virtual void leave_event () { }

  /** 
   *  @brief Remaining enter event handler
   *
   *  This event handler is called if no ViewObject requested handling
   *  of this event.
   */
  virtual void enter_event () { }

  /** 
   *  @brief Remaining key event handler
   *
   *  This event handler is called if no ViewObject requested handling
   *  of this event.
   */
  virtual void key_event (unsigned int /*key*/, unsigned int /*buttons*/) { }

  /**
   *  @brief The drag enter event
   *
   *  If something is dragged into the view, this event is triggered.
   *  p will be the position of the event.
   *
   *  The implementation should return true, if the drop is accepted.
   */
  virtual bool drag_enter_event (const db::DPoint & /*p*/, const DragDropDataBase * /*data*/) { return false; }

  /**
   *  @brief The drag move event
   */
  virtual bool drag_move_event (const db::DPoint & /*p*/, const DragDropDataBase * /*data*/) { return false; }

  /**
   *  @brief The drag leave event
   */
  virtual void drag_leave_event () { }

  /**
   *  @brief The drop event
   */
  virtual bool drop_event (const db::DPoint & /*p*/, const DragDropDataBase * /*data*/) { return false; }

  /** 
   *  @brief Remaining mouse double click event handler
   *
   *  This event handler is called if no ViewObject requested handling
   *  of this event.
   */
  virtual void mouse_double_click_event (const db::DPoint & /*p*/, unsigned int /*buttons*/) { }

  /** 
   *  @brief Remaining single mouse click event handler
   *
   *  This event handler is called if no ViewObject requested handling
   *  of this event.
   */
  virtual void mouse_click_event (const db::DPoint & /*p*/, unsigned int /*buttons*/) { }

  /** 
   *  @brief Remaining mouse press event handler
   *
   *  This event handler is called if no ViewObject requested handling
   *  of this event.
   */
  virtual void mouse_press_event (const db::DPoint & /*p*/, unsigned int /*buttons*/) { }

  /** 
   *  @brief Remaining mouse release event handler
   *
   *  This event handler is called if no ViewObject requested handling
   *  of this event.
   */
  virtual void mouse_release_event (const db::DPoint & /*p*/, unsigned int /*buttons*/) { }

  /** 
   *  @brief Remaining mouse move event handler
   *
   *  This event handler is called if no ViewObject requested handling
   *  of this event.
   */
  virtual void mouse_move_event (const db::DPoint & /*p*/, unsigned int /*buttons*/) { }

  /** 
   *  @brief Remaining wheel event handler
   *
   *  This event handler is called if no ViewObject requested handling
   *  of this event.
   */
  virtual void wheel_event (int /*delta*/, bool /*horizontal*/, const db::DPoint & /*p*/, unsigned int /*buttons*/) { }

  /**
   *  @brief Get the transformation for mouse events
   */
  const db::DCplxTrans &mouse_event_trans () const
  {
    return m_trans;
  }

  /** 
   *  @brief Deliver an iterator for the mouse receivers (begin)
   */
  mouse_receivers_iterator begin_mouse_receivers () const
  {
    return m_grabbed.begin ();
  }

  /** 
   *  @brief Deliver an iterator for the mouse receivers (end)
   */
  mouse_receivers_iterator end_mouse_receivers () const
  {
    return m_grabbed.end ();
  }

  /**
   *  @brief Get the viewport for mouse events
   */
  db::DBox mouse_event_viewport () const;

  /**
   *  @brief Set the default cursor
   *
   *  The default cursor is shown when no mouse event sets the cursor.
   */
  void set_default_cursor (lay::Cursor::cursor_shape cursor);

  /**
   *  @brief Sets a value indicating whether dismissable view objects shall be drawn or not
   *
   *  Markers with dismissable = false are always drawn. The default value is "false".
   */
  void set_dismiss_view_objects (bool dismissed);

  /**
   *  @brief Gets a value indicating whether dismissable markers shall be drawn or not
   */
  bool dismiss_view_objects () const
  {
    return m_view_objects_dismissed;
  }

protected:
  /**
   *  @brief Qt focus event handler
   */
  bool focusNextPrevChild (bool next);

  /**
   *  @brief Qt keyboard event handler
   */
  void keyPressEvent (QKeyEvent *e);

  /**
   *  @brief Qt mouse move event handler
   */
  void mouseMoveEvent (QMouseEvent *e);

  /**
   *  @brief Qt mouse leave event handler
   */
  void leaveEvent (QEvent *e);

  /**
   *  @brief Qt mouse enter event handler
   */
  void enterEvent (QEvent *e);

  /**
   *  @brief Qt mouse button press event handler
   */
  void mousePressEvent (QMouseEvent *e);

  /**
   *  @brief Qt mouse button double-click event handler
   */
  void mouseDoubleClickEvent (QMouseEvent *e);

  /**
   *  @brief Qt mouse button release event handler
   */
  void mouseReleaseEvent (QMouseEvent *e);

  /**
   *  @brief Qt drag enter event handler
   */
  void dragEnterEvent (QDragEnterEvent *event);

  /**
   *  @brief Qt drag leave event handler
   */
  void dragLeaveEvent (QDragLeaveEvent *event);

  /**
   *  @brief Qt drag enter event handler
   */
  void dragMoveEvent (QDragMoveEvent *event);

  /**
   *  @brief Qt drop event handler
   */
  void dropEvent (QDropEvent *event);

  /**
   *  @brief Qt mouse wheel event handler
   */
  void wheelEvent (QWheelEvent *e);

  /**
   *  @brief Set the transformation for mouse events
   */
  void mouse_event_trans (const db::DCplxTrans &trans);

private:
  friend class lay::ViewObject;
  friend class lay::ViewService;
  friend class lay::BackgroundViewObject;

  tl::weak_collection<lay::ViewObject> m_objects;
  tl::weak_collection<lay::BackgroundViewObject> m_background_objects;
  std::list<lay::ViewService *> m_services;
  std::list<ViewService *> m_grabbed;
  bool m_view_objects_dismissed;
  bool m_needs_update_static;
  bool m_needs_update_bg;
  lay::ViewService *mp_active_service;
  db::DCplxTrans m_trans;
  QPoint m_mouse_pos;
  QPoint m_mouse_pressed;
  bool m_mouse_pressed_state;
  unsigned int m_mouse_buttons;
  bool m_in_mouse_move;
  lay::Cursor::cursor_shape m_cursor, m_default_cursor;

  void do_mouse_move ();
  void begin_mouse_event (lay::Cursor::cursor_shape cursor = lay::Cursor::keep);
  void end_mouse_event ();
  void objects_changed ();

  /**
   *  @brief Register a service
   *
   *  This method is called by the service constructor and registers the service in the widget.
   */
  void register_service (lay::ViewService *);

  /**
   *  @brief Register a service
   *
   *  This method is called by the service destructor and unregisters the service in the widget.
   */
  void unregister_service (lay::ViewService *svc);
};

/**
 *  @brief The canvas interface
 *
 *  The canvas provides ways and attributes that allow the view objects to render
 *  themselves. The basic functionality of this interface is to provide planes
 *  for painting. 
 */
class LAYBASIC_PUBLIC ViewObjectCanvas
{
public:
  /**
   *  The destructor 
   */
  virtual ~ViewObjectCanvas () 
  { }

  /**
   *  @brief Background color property: background color of the canvas
   */
  virtual QColor background_color () const = 0;

  /**
   *  @brief Foreground color property: foreground color of the canvas (some "contrast" color to background)
   */
  virtual QColor foreground_color () const = 0;

  /**
   *  @brief Active color property: color of active elements on the canvas (some "contrast" color to background and different from foreground)
   */
  virtual QColor active_color () const = 0;

  /**
   *  @brief Get the resolution
   *
   *  The resolution describes the size of one pixel in relation to "one line width". "One line width" is the unit
   *  of the drawing and describes the width of a "one unit" wide line. In that sense, the resolution is the reciprocal 
   *  of a "one unit" line's width in pixels.
   */
  virtual double resolution () const = 0;

  /**
   *  @brief CanvasPlane provider
   *
   *  This method is used by the view objects to obtain the planes
   *  they should paint themselves on. 
   *  A style can be specified that determines how the plane will
   *  be displayed. CanvasPlanes may be shared if the style is identical.
   *  The plane index of the ViewOp is not used. 
   */
  virtual lay::CanvasPlane *plane (const lay::ViewOp &style) = 0;

  /**
   *  @brief CanvasPlane provider for a sequence of operations on the same plane
   *
   *  This method is used by the view objects to obtain the planes
   *  they should paint themselves on. 
   *  A operator vector can be specified that determines how the plane will
   *  be displayed. The operators are executed in the order given on the same plane.
   *  CanvasPlanes may be shared if the styles are identical.
   *  The plane index of the ViewOp is not used. 
   */
  virtual lay::CanvasPlane *plane (const std::vector<lay::ViewOp> &style) = 0;

  /**
   *  @brief Provide the renderer
   */
  virtual lay::Renderer &renderer () = 0;

  /**
   *  @brief Sort the planes in the painting order
   */
  virtual void sort_planes () = 0;
};

/**
 *  @brief The canvas interface
 *
 *  The canvas provides ways and attributes that allow the view objects to render
 *  themselves. The basic functionality of this interface is to provide planes
 *  for painting. 
 */
class LAYBASIC_PUBLIC BitmapViewObjectCanvas
  : public ViewObjectCanvas
{
public:
  /**
   *  @brief Constructor
   */
  BitmapViewObjectCanvas ();

  /**
   *  @brief Constructor
   */
  BitmapViewObjectCanvas (unsigned int width, unsigned int height, double resolution);

  /**
   *  @brief The destructor 
   */
  virtual ~BitmapViewObjectCanvas ();

  /**
   *  @brief CanvasPlane provider
   *
   *  This method is used by the view objects to obtain the planes
   *  they should paint themselves on. 
   *  A style can be specified that determines how the plane will
   *  be displayed. CanvasPlanes may be shared if the style is identical.
   *  The plane index of the ViewOp is not used. 
   */
  virtual lay::CanvasPlane *plane (const lay::ViewOp &style);

  /**
   *  @brief CanvasPlane provider for a sequence of operations on the same plane
   *
   *  This method is used by the view objects to obtain the planes
   *  they should paint themselves on. 
   *  A operator vector can be specified that determines how the plane will
   *  be displayed. The operators are executed in the order given on the same plane.
   *  CanvasPlanes may be shared if the styles are identical.
   *  The plane index of the ViewOp is not used. 
   */
  virtual lay::CanvasPlane *plane (const std::vector<lay::ViewOp> &style);

  /**
   *  @brief Sort the planes in the painting order
   */
  virtual void sort_planes ();

  /**
   *  @brief Provide the renderer
   */
  virtual lay::Renderer &renderer () 
  { 
    return m_renderer; 
  }

  /**
   *  @brief Get the resolution
   */
  virtual double resolution () const 
  {
    return m_resolution;
  }

  /**
   *  @brief Return the number of bitmaps stored so far
   */
  size_t fg_bitmaps () const
  {
    return mp_fg_bitmaps.size ();
  }

  /**
   *  @brief Return the foreground bitmap with the given index
   *
   *  The returned pointer may be 0. The index must be less than the one returned
   *  by fg_bitmaps ().
   */
  const lay::Bitmap *fg_bitmap (unsigned int i) const
  {
    return mp_fg_bitmaps [i];
  }

  /**
   *  @brief Direct access to the bitmap pointers
   */

  const std::vector <lay::Bitmap *> &fg_bitmap_vector () const
  {
    return mp_fg_bitmaps;
  }

  /**
   *  @brief Return the foreground plane style with the given index
   *
   *  The index must be less than the one returned by fg_planes ().
   */
  const lay::ViewOp &fg_style (unsigned int i) const
  {
    return m_fg_view_ops [i];
  }

  /**
   *  @brief Direct access to the styles vector
   */
  const std::vector <lay::ViewOp> &fg_view_op_vector () const
  {
    return m_fg_view_ops;
  }

  /**
   *  @brief Clear the foreground bitmaps and all associated information
   */
  void clear_fg_bitmaps ();

  /**
   *  @brief Return the background image
   */
  virtual QImage &bg_image () = 0;

  /**
   *  @brief Set the width and height and resolution
   */
  void set_size (unsigned int width, unsigned int height, double resolution);

  /**
   *  @brief Set the width and height
   */
  void set_size (unsigned int width, unsigned int height);

  /**
   *  @brief Set the resolution
   */
  void set_size (double resolution);

  /**
   *  @brief Get the width
   */
  unsigned int canvas_width () const 
  {
    return m_width;
  }

  /**
   *  @brief Get the height
   */
  unsigned int canvas_height () const 
  {
    return m_height;
  }

private:
  std::map <lay::ViewOp, unsigned int> m_fg_bitmap_table;
  std::map <std::vector <lay::ViewOp>, unsigned int> m_fgv_bitmap_table;
  std::vector <lay::Bitmap *> mp_fg_bitmaps;
  std::vector <lay::Bitmap *> mp_alloc_bitmaps;
  std::vector <lay::ViewOp> m_fg_view_ops;
  lay::BitmapRenderer m_renderer;
  unsigned int m_width, m_height;
  double m_resolution;
};

} // namespace lay

#endif



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


#ifndef HDR_layEditorServiceBase
#define HDR_layEditorServiceBase

#include "laybasicCommon.h"
#include "layEditable.h"
#include "layViewObject.h"
#include "layPlugin.h"

namespace lay
{

/**
 *  @brief A generic base class for an editor service
 *
 *  This class offers common services such as a mouse cursor.
 */
class LAYBASIC_PUBLIC EditorServiceBase
  : public lay::ViewService,
    public lay::Editable,
    public lay::Plugin
{
public: 
  /**
   *  @brief Constructor
   */
  EditorServiceBase (lay::LayoutViewBase *view = 0);

  /**
   *  @brief Initialize after constructor was called with null view pointer
   */
  void init (lay::LayoutViewBase *view);

  /**
   *  @brief Destructor
   */
  ~EditorServiceBase ();

  /**
   *  @brief Obtain the lay::ViewService interface
   */
  lay::ViewService *view_service_interface ()
  {
    return this;
  }

  /**
   *  @brief Obtain the lay::Editable interface
   */
  lay::Editable *editable_interface ()
  {
    return this;
  }

  /**
   *  @brief Gets a value indicating whether the plugin is active
   */
  bool is_active () const
  {
    return m_active;
  }

  /**
   *  @brief Adds a mouse cursor to the given point
   */
  void add_mouse_cursor (const db::DPoint &pt, bool emphasize = false);

  /**
   *  @brief Adds a mouse cursor to the given point in layout space
   */
  void add_mouse_cursor (const db::Point &pt, unsigned int cv_index, const db::ICplxTrans &gt, const std::vector<db::DCplxTrans> &tv, bool emphasize = false);

  /**
   *  @brief Adds an edge marker for the given edge
   */
  void add_edge_marker (const db::DEdge &e, bool emphasize = false);

  /**
   *  @brief Adds an edge marker for the given edge in layout space
   */
  void add_edge_marker (const db::Edge &e, unsigned int cv_index, const db::ICplxTrans &gt, const std::vector<db::DCplxTrans> &tv, bool emphasize = false);

  /**
   *  @brief Resets the mouse cursor
   */
  void clear_mouse_cursors ();

  /**
   *  @brief Provides a nice mouse tracking cursor from the given snap details
   */
  void mouse_cursor_from_snap_details (const lay::PointSnapToObjectResult &snap_details, bool noclear = false);

  /**
   *  @brief Gets the tracking cursor color
   */
  tl::Color tracking_cursor_color () const
  {
    return m_cursor_color;
  }

  /**
   *  @brief Gets a value indicating whether the tracking cursor is enabled
   */
  bool tracking_cursor_enabled () const
  {
    return m_cursor_enabled;
  }

  /**
   *  @brief Gets a value indicating whether a cursor position it set
   */
  virtual bool has_tracking_position () const
  {
    return m_has_tracking_position;
  }

  /**
   *  @brief Gets the cursor position if one is set
   */
  virtual db::DPoint tracking_position () const
  {
    return m_tracking_position;
  }

  /**
   *  @brief Shows an error where an exception is not applicable
   */
  void show_error (tl::Exception &ex);

  /**
   *  @brief Menu command handler
   */
  virtual void menu_activated (const std::string & /*symbol*/)
  {
    // .. this implementation does nothing ..
  }

  /**
   *  @brief Sets a configuration option
   */
  virtual bool configure (const std::string &name, const std::string &value);

  /**
   *  @brief Configuration finalization
   */
  virtual void config_finalize ()
  {
    lay::Plugin::config_finalize ();
  }

  /**
   *  @brief Called when the plugin is deactivated
   */
  virtual void deactivated ();

  /**
   *  @brief Called when the plugin is activated
   */
  virtual void activated ();

  /**
   *  @brief Key event handler
   */
  virtual bool key_event (unsigned int /*key*/, unsigned int /*buttons*/);

  /**
   *  @brief Shortcut override event handler
   */
  virtual bool shortcut_override_event (unsigned int /*key*/, unsigned int /*buttons*/);

  /**
   *  @brief Mouse press event handler
   */
  virtual bool mouse_press_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/)
  {
    return false;
  }

  /**
   *  @brief Mouse single-click event handler
   */
  virtual bool mouse_click_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/)
  {
    return false;
  }

  /**
   *  @brief Mouse double-click event handler
   */
  virtual bool mouse_double_click_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/)
  {
    return false;
  }

  /**
   *  @brief Mouse leave event handler
   */
  virtual bool leave_event (bool /*prio*/)
  {
    return false;
  }

  /**
   *  @brief Mouse enter event handler
   */
  virtual bool enter_event (bool /*prio*/)
  {
    return false;
  }

  /**
   *  @brief Mouse move event handler
   */
  virtual bool mouse_move_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/)
  {
    return false;
  }

  /**
   *  @brief Mouse release event handler
   */
  virtual bool mouse_release_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/)
  {
    return false;
  }

  /**
   *  @brief Wheel event handler
   */
  virtual bool wheel_event (int /*delta*/, bool /*horizontal*/, const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/)
  {
    return false;
  }

  /**
   *  @brief Updates the internal data after a coordinate system change for example
   */
  virtual void update ()
  {
    //  The default implementation does nothing
  }

  /**
   *  @brief This method is called when some mouse dragging operation should be cancelled
   */
  virtual void drag_cancel ()
  {
    //  The default implementation does nothing
  }

  /**
   *  @brief Gets called when the focus page opens
   *
   *  The default implementation will call fp->show() and return its return value.
   */
  virtual int focus_page_open ();

  /**
   *  @brief Gets the editor options pages associated with this plugin
   */
  std::vector<lay::EditorOptionsPage *> editor_options_pages ();

  /**
   *  @brief Gets the focus page or 0 if there is none
   */
  lay::EditorOptionsPage *focus_page ();

private:
  //  The marker representing the mouse cursor
  lay::LayoutViewBase *mp_view;
  std::vector<lay::ViewObject *> m_mouse_cursor_markers;
  tl::Color m_cursor_color;
  bool m_cursor_enabled;
  bool m_has_tracking_position;
  db::DPoint m_tracking_position;
  bool m_active;
};

}

#endif


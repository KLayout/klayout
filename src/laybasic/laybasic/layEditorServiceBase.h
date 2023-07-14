
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
  EditorServiceBase (lay::LayoutViewBase *view);

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
  void mouse_cursor_from_snap_details (const lay::PointSnapToObjectResult &snap_details);

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

protected:
  virtual bool configure (const std::string &name, const std::string &value);
  virtual void deactivated ();

private:
  //  The marker representing the mouse cursor
  lay::LayoutViewBase *mp_view;
  std::vector<lay::ViewObject *> m_mouse_cursor_markers;
  tl::Color m_cursor_color;
  bool m_cursor_enabled;
  bool m_has_tracking_position;
  db::DPoint m_tracking_position;
};

}

#endif


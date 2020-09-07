
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


#ifndef HDR_layEditorServiceBase
#define HDR_layEditorServiceBase

#include "laybasicCommon.h"
#include "layEditable.h"
#include "layViewObject.h"

namespace lay
{

/**
 *  @brief A generic base class for an editor service
 *
 *  This class offers common services such as a mouse cursor.
 */
class LAYBASIC_PUBLIC EditorServiceBase
  : public lay::ViewService,
    public lay::Editable
{
public: 
  /**
   *  @brief Constructor
   */
  EditorServiceBase (lay::LayoutView *view);

  /**
   *  @brief Destructor
   */
  ~EditorServiceBase ();

  /**
   *  @brief Adds a mouse cursor to the given point
   */
  void add_mouse_cursor (const db::DPoint &pt, bool emphasize = false);

  /**
   *  @brief Adds an edge marker for the given edge
   */
  void add_edge_marker (const db::DEdge &e, bool emphasize);

  /**
   *  @brief Resets the mouse cursor
   */
  void clear_mouse_cursors ();

private:
  //  The marker representing the mouse cursor
  std::vector<lay::ViewObject *> m_mouse_cursor_markers;
};

}

#endif


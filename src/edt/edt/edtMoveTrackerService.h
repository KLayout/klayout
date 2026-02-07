
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

#ifndef HDR_edtMoveTrackerService
#define HDR_edtMoveTrackerService

#include "edtCommon.h"

#include "layEditorServiceBase.h"
#include "edtEditorHooks.h"

namespace edt {

/**
 *  @brief A service tracking move commands a forwarding them to the editor hooks
 */

class EDT_PUBLIC MoveTrackerService
  : public lay::EditorServiceBase
{
public: 
  /**
   *  @brief The constructor
   */
  MoveTrackerService (lay::LayoutViewBase *view);

  /**
   *  @brief The destructor
   */
  ~MoveTrackerService ();

  /**
   *  @brief Begin a "move" operation
   */
  virtual bool begin_move (lay::Editable::MoveMode mode, const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Continue a "move" operation
   */
  virtual void move (const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Transform during a move operation
   */
  virtual void move_transform (const db::DPoint &p, db::DFTrans tr, lay::angle_constraint_type ac);

  /**
   *  @brief Terminate a "move" operation
   */
  virtual void end_move (const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Terminate a "move" operation with compulsory move vector
   */
  virtual void end_move (const db::DVector &v);

  /**
   *  @brief Access to the view object
   */
  lay::LayoutViewBase *view () const
  {
    tl_assert (mp_view != 0);
    return mp_view;
  }

  /**
   *  @brief Cancel any edit operations (such as move)
   */
  virtual void edit_cancel ();

private:
  lay::LayoutViewBase *mp_view;
  tl::weak_collection<edt::EditorHooks> m_editor_hooks;

  void move_cancel ();
  void open_editor_hooks ();
  void issue_edit_events ();
};

}

#endif


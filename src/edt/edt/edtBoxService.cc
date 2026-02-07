
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#include "edtBoxService.h"

#include "layLayoutViewBase.h"
#include "layEditorOptionsPage.h"

#if defined(HAVE_QT)
#  include "edtPropertiesPages.h"
#endif

namespace edt
{

// -----------------------------------------------------------------------------
//  BoxService implementation

const char *BoxService::configure_name () { return "box-toolkit-widget-value"; }
const char *BoxService::function_name () { return "box-toolkit-widget-commit"; }

BoxService::BoxService (db::Manager *manager, lay::LayoutViewBase *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Boxes), m_centered (false)
{ 
  //  .. nothing yet ..
}
  
#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
BoxService::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  pages.push_back (new edt::BoxPropertiesPage (this, manager, parent));
  return pages;
}
#endif

void 
BoxService::do_begin_edit (const db::DPoint &p)
{
  get_edit_layer ();

  db::DPoint pp = snap2 (p);
  m_p1 = m_p2 = pp;

  open_editor_hooks ();

  set_edit_marker (new lay::Marker (view (), cv_index ()));
  update_marker ();
}

void
BoxService::function (const std::string &name, const std::string &value)
{
  if (name == function_name ()) {

    try {

      db::DVector dim;
      tl::from_string (value, dim);

      if (! m_centered) {
        //  Adjust the direction so positive coordinates are in the current drag direction
        db::DVector d = m_p2 - m_p1;
        dim = db::DVector (dim.x () * (d.x () < 0 ? -1.0 : 1.0), dim.y () * (d.y () < 0 ? -1.0 : 1.0));
      } else {
        dim = db::DVector (fabs (dim.x ()) * 0.5, fabs (dim.y ()) * 0.5);
      }
      m_p2 = m_p1 + dim;

      finish_editing (true);

    } catch (...) {
    }

  }
}

db::Box
BoxService::get_box () const
{
  if (m_centered) {
    db::DVector d = m_p2 - m_p1;
    return db::Box (trans () * (m_p1 - d), trans () * (m_p1 + d));
  } else {
    return db::Box (trans () * m_p1, trans () * m_p2);
  }
}

void
BoxService::update_marker ()
{
  lay::Marker *marker = dynamic_cast<lay::Marker *> (edit_marker ());
  if (marker) {

    marker->set (get_box (), db::VCplxTrans (1.0 / layout ().dbu ()) * trans ().inverted ());

    db::DVector d = m_p2 - m_p1;
    db::DVector dim = db::DVector (fabs (d.x ()), fabs (d.y ())) * (m_centered ? 2.0 : 1.0);

    view ()->message (std::string ("lx: ") +
                      tl::micron_to_string (dim.x ()) +
                      std::string ("  ly: ") +
                      tl::micron_to_string (dim.y ()));

    auto p = toolbox_widget ();
    if (p) {
      p->configure (configure_name (), dim.to_string ());
    }

  }

  //  call hooks with new shape
  if (! editor_hooks ().empty ()) {
    call_editor_hooks (editor_hooks (), &edt::EditorHooks::begin_new_shapes);
    try {
      deliver_shape_to_hooks (get_box ());
    } catch (...) {
      //  ignore exceptions
    }
    call_editor_hooks (editor_hooks (), &edt::EditorHooks::end_new_shapes);
  }
}

void
BoxService::do_mouse_move_inactive (const db::DPoint &p)
{
  lay::PointSnapToObjectResult snap_details = snap2_details (p);
  mouse_cursor_from_snap_details (snap_details);
}

void
BoxService::do_mouse_move (const db::DPoint &p)
{
  //  snap to square if Ctrl button is pressed
  bool snap_square = (mouse_buttons () & lay::ControlButton) != 0;
  bool centered = (mouse_buttons ()  & lay::ShiftButton) != 0;

  lay::PointSnapToObjectResult snap_details = snap2_details (p, m_p1, snap_square ? lay::AC_DiagonalOnly : lay::AC_Any);
  db::DPoint ps = snap_details.snapped_point;

  if (snap_details.object_snap == lay::PointSnapToObjectResult::NoObject && ! m_centered) {

    clear_mouse_cursors ();

    db::DPoint px (p.x (), m_p1.y ());
    lay::PointSnapToObjectResult snap_details_x = snap2_details (px);

    db::DPoint py (m_p1.x (), p.y ());
    lay::PointSnapToObjectResult snap_details_y = snap2_details (py);

    if (snap_details_x.object_snap != lay::PointSnapToObjectResult::NoObject) {
      if (snap_square) {
        double dx = fabs (snap_details_x.snapped_point.x () - m_p1.x ());
        ps = db::DPoint (snap_details_x.snapped_point.x (), m_p1.y () + (ps.y () < m_p1.y () ? -dx : dx));
      } else {
        ps = db::DPoint (snap_details_x.snapped_point.x (), ps.y ());
      }
      mouse_cursor_from_snap_details (snap_details_x, true /*add*/);
    }

    if (snap_details_y.object_snap != lay::PointSnapToObjectResult::NoObject) {
      if (snap_square) {
        double dy = fabs (snap_details_y.snapped_point.x () - m_p1.y ());
        ps = db::DPoint (m_p1.x () + (ps.x () < m_p1.x () ? -dy : dy), snap_details_y.snapped_point.y ());
      } else {
        ps = db::DPoint (ps.x (), snap_details_y.snapped_point.y ());
      }
      mouse_cursor_from_snap_details (snap_details_y, true /*add*/);
    }

    add_mouse_cursor (ps);

  } else {
    mouse_cursor_from_snap_details (snap_details);
  }

  set_cursor (lay::Cursor::cross);
  m_p2 = ps;
  m_centered = centered;
  update_marker ();
}

bool 
BoxService::do_mouse_click (const db::DPoint &p)
{
  do_mouse_move (p);
  return true;
}

void 
BoxService::do_finish_edit (bool /*accept*/)
{
  deliver_shape (get_box ());
  commit_recent ();
  close_editor_hooks (true);
}

void 
BoxService::do_cancel_edit ()
{
  close_editor_hooks (false);
}

bool 
BoxService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return !sel.is_cell_inst () && sel.shape ().is_box ();
}

} // namespace edt

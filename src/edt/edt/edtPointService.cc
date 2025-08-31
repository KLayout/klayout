
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


#include "edtPointService.h"

#include "layLayoutViewBase.h"

#if defined(HAVE_QT)
#  include "edtPropertiesPages.h"
#endif

namespace edt
{

// -----------------------------------------------------------------------------
//  PointService implementation

PointService::PointService (db::Manager *manager, lay::LayoutViewBase *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Points)
{
  //  .. nothing yet ..
}

#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
PointService::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  pages.push_back (new edt::PointPropertiesPage (this, manager, parent));
  return pages;
}
#endif

void
PointService::do_begin_edit (const db::DPoint &p)
{
  get_edit_layer ();

  db::DPoint pp = snap2 (p);
  m_p = pp;

  open_editor_hooks ();

  set_edit_marker (new lay::Marker (view (), cv_index ()));
  update_marker ();
}

db::Point
PointService::get_point () const
{
  return db::Point (trans () * m_p);
}

void
PointService::update_marker ()
{
  lay::Marker *marker = dynamic_cast<lay::Marker *> (edit_marker ());
  if (marker) {

    db::Point pt = get_point ();
    marker->set (db::Box (pt, pt), db::VCplxTrans (1.0 / layout ().dbu ()) * trans ().inverted ());

    view ()->message (std::string ("x: ") +
                      tl::micron_to_string (m_p.x ()) +
                      std::string ("  y: ") +
                      tl::micron_to_string (m_p.y ()));

  }

  //  call hooks with new shape
  if (! editor_hooks ().empty ()) {
    call_editor_hooks (editor_hooks (), &edt::EditorHooks::begin_new_shapes);
    try {
      deliver_shape_to_hooks (get_point ());
    } catch (...) {
      //  ignore exceptions
    }
    call_editor_hooks (editor_hooks (), &edt::EditorHooks::end_new_shapes);
  }
}

void
PointService::do_mouse_move_inactive (const db::DPoint &p)
{
  lay::PointSnapToObjectResult snap_details = snap2_details (p);
  mouse_cursor_from_snap_details (snap_details);
}

void
PointService::do_mouse_move (const db::DPoint &p)
{
  do_mouse_move_inactive (p);

  set_cursor (lay::Cursor::cross);
  m_p = snap2 (p);
  update_marker ();
}

bool
PointService::do_mouse_click (const db::DPoint &p)
{
  do_mouse_move (p);
  return true;
}

void
PointService::do_finish_edit ()
{
  deliver_shape (get_point ());
  commit_recent ();
  close_editor_hooks (true);
}

void
PointService::do_cancel_edit ()
{
  close_editor_hooks (false);
}

bool
PointService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return !sel.is_cell_inst () && sel.shape ().is_point ();
}

} // namespace edt

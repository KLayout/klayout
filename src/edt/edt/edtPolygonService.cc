
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


#include "edtPolygonService.h"

#include "layLayoutViewBase.h"
#include "layEditorOptionsPage.h"

#if defined(HAVE_QT)
#  include "edtPropertiesPages.h"
#endif

namespace edt
{

// -----------------------------------------------------------------------------
//  PolygonService implementation

PolygonService::PolygonService (db::Manager *manager, lay::LayoutViewBase *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Polygons),
    m_closure_set (false), m_closure ()
{
  //  .. nothing yet ..
}

#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
PolygonService::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  pages.push_back (new edt::PolygonPropertiesPage (this, manager, parent));
  return pages;
}
#endif

void 
PolygonService::do_begin_edit (const db::DPoint &p)
{
  get_edit_layer ();

  db::DPoint pp = snap2 (p);
  m_last = pp;

  m_points.clear ();
  m_points.push_back (pp);
  m_points.push_back (pp);
  m_closure_set = false;

  open_editor_hooks ();

  update_marker ();
}

void 
PolygonService::set_last_point (const db::DPoint &p)
{
  m_points.back () = snap2 (p, m_last);

  //  for manhattan polygons allow some movement of the projected edge
  if (m_points.size () >= 3 && connect_ac () == lay::AC_Ortho) {

    db::DPoint p_grid = snap2 (p);
    std::pair<bool, db::DPoint> ip = interpolate (m_points.end ()[-3], m_last, p_grid);
    if (ip.first) {

      m_points.end ()[-2] = ip.second;
      m_points.back () = p_grid;

    }

  } else if (m_points.size () >= 2) {
    m_points.end ()[-2] = m_last;
  }
}

void
PolygonService::do_mouse_move_inactive (const db::DPoint &p)
{
  lay::PointSnapToObjectResult snap_details = snap2_details (p);
  mouse_cursor_from_snap_details (snap_details);
}

void
PolygonService::do_delete ()
{
  if (m_points.size () > 2) {
    m_points.erase (m_points.end () - 2);
    m_last = m_points.end()[-2];
    update_marker ();
  }
}

void
PolygonService::do_mouse_move (const db::DPoint &p)
{
  do_mouse_move_inactive (p);

  set_cursor (lay::Cursor::cross);
  if (m_points.size () >= 2) {
    set_last_point (p);
  }
  add_closure ();
  update_marker ();
}

bool 
PolygonService::do_mouse_click (const db::DPoint &p)
{
  if (m_points.size () >= 1) {
    m_last = m_points.back ();
    m_points.push_back (db::DPoint ());
    set_last_point (p);
  }
  //  do not do a add_closure here - this will not work since we may have two identical points on top.
  return false;
}

void 
PolygonService::do_finish_edit (bool accept)
{
  if (accept) {
    //  add a dummy point in this case for the current one
    m_last = m_points.back ();
    m_points.push_back (db::DPoint ());
  }

  deliver_shape (get_polygon (false));
  commit_recent ();
  close_editor_hooks (true);
}

void
PolygonService::function (const std::string &name, const std::string &value)
{
  if (name == ShapeEditService::connection_function_name ()) {

    try {

      db::DVector dim;
      tl::from_string (value, dim);

      if (m_points.size () >= 2) {

        m_last = m_points.back () = m_points.end () [-2] + dim;
        m_points.push_back (m_last);

        update_marker ();

      }

    } catch (...) {
    }

  }
}

db::Polygon
PolygonService::get_polygon (bool editing) const
{
  db::Polygon poly;

  if (! editing && m_points.size () + (m_closure_set ? 1 : 0) < 4) {
    throw tl::Exception (tl::to_string (tr ("A polygon must have at least 3 points")));
  }

  std::vector<db::Point> points_dbu;
  points_dbu.reserve (m_points.size () + 1);

  //  one point is reserved for the current one
  for (std::vector<db::DPoint>::const_iterator p = m_points.begin (); p + 1 != m_points.end (); ++p) {
    points_dbu.push_back (trans () * *p);
  }
  if (editing) {
    points_dbu.push_back (trans () * m_points.back ());
  }
  if (m_closure_set) {
    points_dbu.push_back (trans () * m_closure);
  }

  poly.assign_hull (points_dbu.begin (), points_dbu.end (), !editing /*compress*/, !editing /*remove reflected*/);

  if (! editing && poly.hull ().size () < 3) {
    throw tl::Exception (tl::to_string (tr ("A polygon must have at least 3 effective points")));
  }

  return poly;
}

void 
PolygonService::do_cancel_edit ()
{
  close_editor_hooks (false);
}

bool 
PolygonService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return !sel.is_cell_inst () && sel.shape ().is_polygon ();
}

void
PolygonService::add_closure ()
{
  if (connect_ac () == lay::AC_Any || m_points.size () < 3) {
    m_closure_set = false;
  } else {

    std::vector <db::DVector> delta;
    delta.reserve (4);

    //  Even for diagonal mode, we try to do manhattan closing

    delta.push_back (db::DVector (1.0, 0.0));
    delta.push_back (db::DVector (0.0, 1.0));
//  TODO: for Diagonal mode, this scheme does not work pretty well.
#if 0
    if (connect_ac () == lay::AC_Diagonal) {
      delta.push_back (db::DVector (1.0, -1.0));
      delta.push_back (db::DVector (1.0, 1.0));
    }
#endif

    //  Determine the closing point by determining the one of the possible closing points
    //  (given the angle constraints) that is closest to the current one.

    m_closure = db::DPoint ();
    m_closure_set = false;

    std::vector <db::DPoint>::const_iterator pi;

    db::DPoint p1, pl;

    pi = m_points.begin () + 1; 
    while (pi != m_points.end () - 1 && *pi == m_points [0]) {
      ++pi;
    }
    p1 = *pi;

    pi = m_points.end () - 2; 
    while (pi != m_points.begin () + 1 && *pi == m_points.back ()) {
      --pi;
    }
    pl = *pi;

    //  first try a direct cut between last and first segment ..
    db::DEdge e1 (m_points [0], m_points [1]);
    db::DEdge e2 (m_points.end ()[-2], m_points.back ());

    std::pair <bool, db::DPoint> cp = e1.cut_point (e2);
    if (cp.first && 
        db::sprod (p1 - m_points [0], cp.second - m_points [0]) < 0.99 * p1.distance (m_points [0]) * cp.second.distance (m_points [0]) + 1e-6 &&
        db::sprod (pl - m_points.back (), cp.second - m_points.back ()) < 0.99 * pl.distance (m_points.back ()) * cp.second.distance (m_points.back ()) + 1e-6) {
      m_closure = cp.second;
      m_closure_set = true;
    }

    //  if that is not working out, try to keep one edge any vary the possible edges emerging from 
    //  the other point
    if ( ! m_closure_set) {

      for (std::vector <db::DVector>::const_iterator d1 = delta.begin (); d1 != delta.end (); ++d1) {

        db::DEdge e1 (m_points [0], m_points [0] + *d1);
        db::DEdge e2 (m_points.end ()[-2], m_points.back ());

        std::pair <bool, db::DPoint> cp = e1.cut_point (e2);
        if (cp.first && (! m_closure_set || cp.second.sq_distance (m_points.back ()) < m_closure.sq_distance (m_points.back ())) && 
            db::sprod (p1 - m_points [0], cp.second - m_points [0]) < 0.99 * p1.distance (m_points [0]) * cp.second.distance (m_points [0]) &&
            db::sprod (pl - m_points.back (), cp.second - m_points.back ()) < 0.99 * pl.distance (m_points.back ()) * cp.second.distance (m_points.back ())) {
          m_closure = cp.second;
          m_closure_set = true;
        }
      }

    }

    if ( ! m_closure_set) {

      for (std::vector <db::DVector>::const_iterator d2 = delta.begin (); d2 != delta.end (); ++d2) {

        db::DEdge e1 (m_points [0], m_points [1]);
        db::DEdge e2 (m_points.back (), m_points.back () + *d2);

        std::pair <bool, db::DPoint> cp = e1.cut_point (e2);
        if (cp.first && (! m_closure_set || cp.second.sq_distance (m_points.back ()) < m_closure.sq_distance (m_points.back ())) && 
            db::sprod (p1 - m_points [0], cp.second - m_points [0]) < 0.99 * p1.distance (m_points [0]) * cp.second.distance (m_points [0]) &&
            db::sprod (pl - m_points.back (), cp.second - m_points.back ()) < 0.99 * pl.distance (m_points.back ()) * cp.second.distance (m_points.back ())) {
          m_closure = cp.second;
          m_closure_set = true;
        }
      }

    }

    //  if that is not working out, try each possible variations of edges from start and end point
    if ( ! m_closure_set) {
      for (std::vector <db::DVector>::const_iterator d1 = delta.begin (); d1 != delta.end (); ++d1) {
        for (std::vector <db::DVector>::const_iterator d2 = delta.begin (); d2 != delta.end (); ++d2) {

          db::DEdge e1 (m_points [0], m_points [0] + *d1);
          db::DEdge e2 (m_points.back (), m_points.back () + *d2);

          std::pair <bool, db::DPoint> cp = e1.cut_point (e2);
          if (cp.first && (! m_closure_set || cp.second.sq_distance (m_points.back ()) < m_closure.sq_distance (m_points.back ())) && 
              db::sprod (p1 - m_points [0], cp.second - m_points [0]) < 0.99 * p1.distance (m_points [0]) * cp.second.distance (m_points [0]) &&
              db::sprod (pl - m_points.back (), cp.second - m_points.back ()) < 0.99 * pl.distance (m_points.back ()) * cp.second.distance (m_points.back ())) {
            m_closure = cp.second;
            m_closure_set = true;
          }

        }
      }
    }

  }
}

void
PolygonService::update_marker ()
{
  if (m_points.size () == 2) {

    db::Edge edge (trans () * m_points [0], trans () * m_points [1]);

    lay::Marker *marker = new lay::Marker (view (), cv_index ());
    marker->set (edge, db::VCplxTrans (1.0 / layout ().dbu ()) * trans ().inverted ());
    set_edit_marker (marker);

  } else if (m_points.size () > 2) {

    std::vector<db::Point> points_dbu;
    points_dbu.reserve (m_points.size () + 1);
    for (std::vector<db::DPoint>::const_iterator p = m_points.begin (); p != m_points.end (); ++p) {
      points_dbu.push_back (trans () * *p);
    }

    db::Path path (points_dbu.begin (), points_dbu.end (), 0);

    lay::Marker *marker;
    
    marker = new lay::Marker (view (), cv_index ());
    marker->set (path, db::VCplxTrans (1.0 / layout ().dbu ()) * trans ().inverted ());
    set_edit_marker (marker);

    db::DPoint pl = m_points.back ();

    if (m_closure_set) {

      db::Edge edge (trans () * pl, trans () * m_closure);
      marker = new lay::Marker (view (), cv_index ());
      if (std::abs (edge.dy ()) < std::abs (edge.dx ())) {
        marker->set_frame_pattern (34);
      } else {
        marker->set_frame_pattern (39);
      }
      marker->set (edge, db::VCplxTrans (1.0 / layout ().dbu ()) * trans ().inverted ());
      add_edit_marker (marker);

      pl = m_closure;

    }

    db::Edge edge (trans () * pl, trans () * m_points.front ());
    marker = new lay::Marker (view (), cv_index ());
    if (std::abs (edge.dy ()) < std::abs (edge.dx ())) {
      marker->set_frame_pattern (34);
    } else {
      marker->set_frame_pattern (39);
    }
    marker->set (edge, db::VCplxTrans (1.0 / layout ().dbu ()) * trans ().inverted ());
    add_edit_marker (marker);

  } else {
    set_edit_marker (0);
  }

  if (m_points.size () >= 2) {
    db::DVector dim = m_points.back () - m_points [m_points.size () - 2];
    view ()->message (std::string ("lx: ") +
                      tl::micron_to_string (dim.x ()) +
                      std::string ("  ly: ") +
                      tl::micron_to_string (dim.y ()) +
                      std::string ("  l: ") +
                      tl::micron_to_string (dim.length ()));
    auto tb = toolbox_widget ();
    if (tb) {
      tb->configure (ShapeEditService::connection_configure_name (), dim.to_string ());
    }
  }

  //  call hooks with new shape
  if (! editor_hooks ().empty ()) {
    call_editor_hooks (editor_hooks (), &edt::EditorHooks::begin_new_shapes);
    try {
      deliver_shape_to_hooks (get_polygon (true));
    } catch (...) {
      //  ignore exceptions
    }
    call_editor_hooks (editor_hooks (), &edt::EditorHooks::end_new_shapes);
  }
}

} // namespace edt

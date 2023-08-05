
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


#include "edtMainService.h"
#include "edtServiceImpl.h"
#if defined(HAVE_QT)
#  include "edtPropertiesPages.h"
#  include "edtInstPropertiesPage.h"
#endif
#include "edtService.h"
#include "edtPlugin.h"
#include "dbEdge.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"
#include "dbPCellDeclaration.h"
#include "dbPolygonTools.h"
#include "dbEdgeProcessor.h"
#include "layMarker.h"
#include "layLayerProperties.h"
#include "layLayoutViewBase.h"

#if defined(HAVE_QT)
#  include "layTipDialog.h"
#  include "layDragDropData.h"
#endif

#if defined(HAVE_QT)
#  include <QApplication>
#endif

namespace edt
{

// -----------------------------------------------------------------------------
//  ShapeEditService implementation

ShapeEditService::ShapeEditService (db::Manager *manager, lay::LayoutViewBase *view, db::ShapeIterator::flags_type shape_types)
  : edt::Service (manager, view, shape_types), 
    m_layer (0), m_cv_index (0), mp_cell (0), mp_layout (0), m_combine_mode (CM_Add)
{
  view->current_layer_changed_event.add (this, &ShapeEditService::update_edit_layer);
}

bool 
ShapeEditService::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_edit_combine_mode) {
    CMConverter ().from_string (value, m_combine_mode);
    return false; // pass to other plugins
  } else {
    return edt::Service::configure (name, value);
  }
}
  
void
ShapeEditService::get_edit_layer ()
{
  lay::LayerPropertiesConstIterator cl = view ()->current_layer ();

  if (cl.is_null ()) {
    throw tl::Exception (tl::to_string (tr ("Please select a layer first")));
  }

  int cv_index = cl->cellview_index ();
  const lay::CellView &cv = view ()->cellview (cv_index);
  int layer = cl->layer_index ();

  if (cv_index < 0 || ! cv.is_valid ()) {
    throw tl::Exception (tl::to_string (tr ("Please select a cell first")));
  }

#if defined(HAVE_QT)
  if (! cl->visible (true)) {
    lay::TipDialog td (QApplication::activeWindow (),
                       tl::to_string (tr ("You are about to draw on a hidden layer. The result won't be visible.")),
                       "drawing-on-invisible-layer");
    td.exec_dialog ();
  }
#endif

  if (layer < 0 || ! cv->layout ().is_valid_layer ((unsigned int) layer)) {

    if (cl->has_children ()) {
      throw tl::Exception (tl::to_string (tr ("Please select a valid drawing layer first")));
    } else {

      //  create this layer now
      const lay::ParsedLayerSource &source = cl->source (true /*real*/);

      db::LayerProperties db_lp;
      if (source.has_name ()) {
        db_lp.name = source.name ();
      }
      db_lp.layer = source.layer ();
      db_lp.datatype = source.datatype ();

      cv->layout ().insert_layer (db_lp);

      //  update the layer index inside the layer view
      cl->realize_source ();
        
      //  Hint: we could have taken the new index from insert_layer, but this 
      //  is a nice test:
      layer = cl->layer_index ();
      tl_assert (layer >= 0);

    }
  }

  m_layer = (unsigned int) layer;
  m_cv_index = (unsigned int) cv_index;
  m_trans = (cl->trans ().front () * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ()).inverted ();
  mp_layout = &(cv->layout ());
  mp_cell = &(mp_layout->cell (cv.cell_index ()));

  if (mp_cell->is_proxy ()) {
    throw tl::Exception (tl::to_string (tr ("Cannot put a shape into a PCell or library cell")));
  }
}

void
ShapeEditService::update_edit_layer (const lay::LayerPropertiesConstIterator &cl)
{
  if (! editing ()) {
    return;
  }

  if (cl.is_null () || cl->has_children ()) {
    return;
  }

  int cv_index = cl->cellview_index ();
  const lay::CellView &cv = view ()->cellview (cv_index);
  int layer = cl->layer_index ();

  if (cv_index < 0 || ! cv.is_valid ()) {
    return;
  }

  if (cv->layout ().cell (cv.cell_index ()).is_proxy ()) {
    return;
  }

#if defined(HAVE_QT)
  if (! cl->visible (true)) {
    lay::TipDialog td (QApplication::activeWindow (),
                       tl::to_string (tr ("You are now drawing on a hidden layer. The result won't be visible.")),
                       "drawing-on-invisible-layer");
    td.exec_dialog ();
  }
#endif

  if (layer < 0 || ! cv->layout ().is_valid_layer ((unsigned int) layer)) {

    //  create this layer now
    const lay::ParsedLayerSource &source = cl->source (true /*real*/);

    db::LayerProperties db_lp;
    if (source.has_name ()) {
      db_lp.name = source.name ();
    }
    db_lp.layer = source.layer ();
    db_lp.datatype = source.datatype ();

    cv->layout ().insert_layer (db_lp);

    //  update the layer index inside the layer view
    cl->realize_source ();

    //  Hint: we could have taken the new index from insert_layer, but this
    //  is a nice test:
    layer = cl->layer_index ();
    tl_assert (layer >= 0);

  }

  m_layer = (unsigned int) layer;
  m_cv_index = (unsigned int) cv_index;
  m_trans = (cl->trans ().front () * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ()).inverted ();
  mp_layout = &(cv->layout ());
  mp_cell = &(mp_layout->cell (cv.cell_index ()));

  current_layer_changed ();
}

void
ShapeEditService::tap (const db::DPoint &initial)
{
  if (editing ()) {
    get_edit_layer ();
  } else {
    begin_edit (initial);
  }
}

/**
 *  @brief Deliver a good interpolation between two points m and p
 *
 *  This method uses an intermediate point o to determine the edge that is emerged from point m.
 *  An edge is searched that emerges from p and intersects with the m->o edge in a way that the intersection
 *  point is closest to o.
 *
 *  This method returns the intersection point ("new o") and a flag if the search was successful (.first of return value).
 */
std::pair <bool, db::DPoint>
ShapeEditService::interpolate (const db::DPoint &m, const db::DPoint &o, const db::DPoint &p) const
{
  if (fabs (m.x () - o.x ()) < 1e-6 && fabs (m.y () - o.y ()) < 1e-6) {
    return std::pair <bool, db::DPoint> (false, db::DPoint ());
  }

  std::vector <db::DVector> delta;
  delta.reserve (4);
  delta.push_back (db::DVector (1.0, 0.0));
  delta.push_back (db::DVector (0.0, 1.0));
  if (connect_ac () == lay::AC_Diagonal) {
    delta.push_back (db::DVector (1.0, -1.0));
    delta.push_back (db::DVector (1.0, 1.0));
  }

  bool c_set = false;
  db::DPoint c;
  for (std::vector <db::DVector>::const_iterator d = delta.begin (); d != delta.end (); ++d) {
    std::pair <bool, db::DPoint> ip = db::DEdge (m, o).cut_point (db::DEdge (p - *d, p));
    if (ip.first && (! c_set || o.sq_distance (ip.second) < o.sq_distance (c))) {
      c = ip.second;
      c_set = true;
    }
  }

  return std::make_pair (c_set, c);
}

void 
ShapeEditService::do_mouse_move_inactive (const db::DPoint &p)
{
  //  display the next (snapped) position where editing would start
  db::DPoint pp = snap (p);
  std::string pos = std::string ("x: ") + tl::micron_to_string (pp.x ()) + 
                    std::string ("  y: ") + tl::micron_to_string (pp.y ());
  view ()->message (pos);
}

void 
ShapeEditService::deliver_shape (const db::Polygon &poly)
{
  if (m_combine_mode == CM_Add) {

    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Create polygon")));
    }
    cell ().shapes (layer ()).insert (poly);
    if (manager ()) {
      manager ()->commit ();
    }

  } else {

    std::vector<db::Shape> shapes;
    std::vector<db::Polygon> result;

    std::vector<db::Polygon> input;
    input.push_back (poly);

    std::vector<db::Polygon> input_left;
    if (m_combine_mode == CM_Diff) {
      input_left = input;
    }

    db::EdgeProcessor ep;
    bool any = false;

    db::ShapeIterator s = cell ().shapes (layer ()).begin_touching (poly.box (), db::ShapeIterator::Polygons | db::ShapeIterator::Paths | db::ShapeIterator::Boxes);
    while (! s.at_end ()) {

      std::vector<db::Polygon> subject;
      subject.push_back (db::Polygon ());
      s->polygon (subject.back ());

      if (db::interact_pp (poly, subject.back ())) {

        any = true;

        if (m_combine_mode == CM_Merge) {
          ep.boolean (subject, input, result, db::BooleanOp::Or);
          input = result;
          input_left.clear ();
          input_left.swap (result);
        } else if (m_combine_mode == CM_Erase) {
          ep.boolean (subject, input, result, db::BooleanOp::ANotB);
        } else if (m_combine_mode == CM_Mask) {
          ep.boolean (subject, input, result, db::BooleanOp::And);
        } else if (m_combine_mode == CM_Diff) {
          ep.boolean (subject, input, result, db::BooleanOp::ANotB);
          std::vector<db::Polygon> l;
          ep.boolean (input_left, subject, l, db::BooleanOp::ANotB);
          l.swap (input_left);
        }

        shapes.push_back (*s);

      } 

      ++s;

    }

    //  If nothing was found, simply pass the input to the result
    if (! any && (m_combine_mode == CM_Merge || m_combine_mode == CM_Diff)) {
      result = input;
    }

    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Combine shape with background")));
    }

    //  Erase existing shapes
    for (std::vector<db::Shape>::const_iterator s = shapes.begin (); s != shapes.end (); ++s) {
      cell ().shapes (layer ()).erase_shape (*s);
    }

    //  Add new shapes
    for (std::vector<db::Polygon>::const_iterator p = result.begin (); p != result.end (); ++p) {
      cell ().shapes (layer ()).insert (*p);
    }
    for (std::vector<db::Polygon>::const_iterator p = input_left.begin (); p != input_left.end (); ++p) {
      cell ().shapes (layer ()).insert (*p);
    }

    if (manager ()) {
      manager ()->commit ();
    }

  }
}

void 
ShapeEditService::deliver_shape (const db::Path &path)
{
  if (m_combine_mode == CM_Add) {
    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Create path")));
    }
    cell ().shapes (layer ()).insert (path);
    if (manager ()) {
      manager ()->commit ();
    }
  } else {
    deliver_shape (path.polygon ());
  }
}

void 
ShapeEditService::deliver_shape (const db::Box &box)
{
  if (m_combine_mode == CM_Add) {
    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Create box")));
    }
    cell ().shapes (layer ()).insert (box);
    if (manager ()) {
      manager ()->commit ();
    }
  } else {
    deliver_shape (db::Polygon (box));
  }
}

void
ShapeEditService::deliver_shape (const db::Point &point)
{
  if (m_combine_mode == CM_Add) {
    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Create point")));
    }
    cell ().shapes (layer ()).insert (point);
    if (manager ()) {
      manager ()->commit ();
    }
  }
}

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
PolygonService::do_finish_edit ()
{
  deliver_shape (get_polygon ());
  commit_recent (view ());
}

db::Polygon
PolygonService::get_polygon () const
{
  db::Polygon poly;

  if (m_points.size () < 4) {
    throw tl::Exception (tl::to_string (tr ("A polygon must have at least 3 points")));
  }

  std::vector<db::Point> points_dbu;
  points_dbu.reserve (m_points.size ());

  //  one point is reserved for the current one
  for (std::vector<db::DPoint>::const_iterator p = m_points.begin (); p + 1 != m_points.end (); ++p) {
    points_dbu.push_back (trans () * *p);
  }
  if (m_closure_set) {
    points_dbu.push_back (trans () * m_closure);
  }

  poly.assign_hull (points_dbu.begin (), points_dbu.end (), true, true /*remove reflected*/);

  if (poly.hull ().size () < 3) {
    throw tl::Exception (tl::to_string (tr ("A polygon must have at least 3 effective points")));
  }

  return poly;
}

void 
PolygonService::do_cancel_edit ()
{
  //  .. nothing yet ..
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
    view ()->message (std::string ("lx: ") +
                      tl::micron_to_string (m_points.back ().x () - m_points.end () [-2].x ()) + 
                      std::string ("  ly: ") +
                      tl::micron_to_string (m_points.back ().y () - m_points.end () [-2].y ()) + 
                      std::string ("  l: ") +
                      tl::micron_to_string (m_points.back ().distance (m_points.end () [-2])));
  }
}

// -----------------------------------------------------------------------------
//  BoxService implementation

BoxService::BoxService (db::Manager *manager, lay::LayoutViewBase *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Boxes)
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

  set_edit_marker (new lay::Marker (view (), cv_index ()));
  update_marker ();
}

db::Box
BoxService::get_box () const
{
  return db::Box (trans () * m_p1, trans () * m_p2);
}

void
BoxService::update_marker ()
{
  lay::Marker *marker = dynamic_cast<lay::Marker *> (edit_marker ());
  if (marker) {

    marker->set (get_box (), db::VCplxTrans (1.0 / layout ().dbu ()) * trans ().inverted ());

    view ()->message (std::string ("lx: ") +
                      tl::micron_to_string (m_p2.x () - m_p1.x ()) + 
                      std::string ("  ly: ") +
                      tl::micron_to_string (m_p2.y () - m_p1.y ()));

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
  do_mouse_move_inactive (p);

  set_cursor (lay::Cursor::cross);
  m_p2 = snap2 (p);
  update_marker ();
}

bool 
BoxService::do_mouse_click (const db::DPoint &p)
{
  do_mouse_move (p);
  return true;
}

void 
BoxService::do_finish_edit ()
{
  deliver_shape (get_box ());
  commit_recent (view ());
}

void 
BoxService::do_cancel_edit ()
{
  //  .. nothing yet ..
}

bool 
BoxService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return !sel.is_cell_inst () && sel.shape ().is_box ();
}

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
  commit_recent (view ());
}

void
PointService::do_cancel_edit ()
{
  //  .. nothing yet ..
}

bool
PointService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return !sel.is_cell_inst () && sel.shape ().is_point ();
}

// -----------------------------------------------------------------------------
//  TextService implementation

TextService::TextService (db::Manager *manager, lay::LayoutViewBase *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Texts),
    m_rot (0)
{ 
  //  .. nothing yet ..
}

TextService::~TextService ()
{
  //  .. nothing yet ..
}

#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
TextService::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  pages.push_back (new edt::TextPropertiesPage (this, manager, parent));
  return pages;
}
#endif

void 
TextService::do_begin_edit (const db::DPoint &p)
{
  get_edit_layer ();

  m_text.trans (db::DTrans (m_rot, snap2 (p) - db::DPoint ()));

  lay::DMarker *marker = new lay::DMarker (view ());
  marker->set_vertex_shape (lay::ViewOp::Cross);
  marker->set_vertex_size (9 /*cross vertex size*/);
  set_edit_marker (marker);
  update_marker ();
}

void
TextService::update_marker ()
{
  lay::DMarker *marker = dynamic_cast<lay::DMarker *> (edit_marker ());
  if (marker) {

    marker->set (m_text);

    std::string pos = std::string ("x: ") +
                      tl::micron_to_string (m_text.trans ().disp ().x ()) + 
                      std::string ("  y: ") +
                      tl::micron_to_string (m_text.trans ().disp ().y ());
    if (m_text.trans ().rot () != 0) {
      pos += std::string ("  ") + ((const db::DFTrans &) m_text.trans ()).to_string ();
    }

    view ()->message (pos);

  }
}

bool
TextService::do_activated ()
{
  m_rot = 0;

  return true;  //  start editing immediately
}

void
TextService::do_mouse_move_inactive (const db::DPoint &p)
{
  lay::PointSnapToObjectResult snap_details = snap2_details (p);
  mouse_cursor_from_snap_details (snap_details);
}

void
TextService::do_mouse_move (const db::DPoint &p)
{
  do_mouse_move_inactive (p);

  set_cursor (lay::Cursor::cross);
  m_text.trans (db::DTrans (m_rot, snap2 (p) - db::DPoint ()));
  update_marker ();
}

void 
TextService::do_mouse_transform (const db::DPoint &p, db::DFTrans trans)
{
  m_rot = (db::DFTrans (m_rot) * trans).rot ();
  m_text.trans (db::DTrans (m_rot, p - db::DPoint ()));
  update_marker ();
}

bool 
TextService::do_mouse_click (const db::DPoint &p)
{
  do_mouse_move (p);
  return true;
}

db::Text
TextService::get_text () const
{
  db::Point p_dbu = trans () * (db::DPoint () + m_text.trans ().disp ());
  return db::Text (m_text.string (), db::Trans (m_text.trans ().rot (), p_dbu - db::Point ()), db::coord_traits<db::Coord>::rounded (trans ().ctrans (m_text.size ())), db::NoFont, m_text.halign (), m_text.valign ());
}

void 
TextService::do_finish_edit ()
{
  get_edit_layer ();

  if (manager ()) {
    manager ()->transaction (tl::to_string (tr ("Create text")));
  }
  cell ().shapes (layer ()).insert (get_text ());
  if (manager ()) {
    manager ()->commit ();
  }

  commit_recent (view ());

#if defined(HAVE_QT)
  if (! view ()->text_visible ()) {

    lay::TipDialog td (QApplication::activeWindow (),
                       tl::to_string (tr ("A text object is created but texts are disabled for drawing and are not visible. Do you want to enable drawing of texts?\n\nChoose \"Yes\" to enable text drawing now.")),
                       "text-created-but-not-visible",
                       lay::TipDialog::yesno_buttons);

    lay::TipDialog::button_type button = lay::TipDialog::null_button;
    td.exec_dialog (button);
    if (button == lay::TipDialog::yes_button) {
      view ()->text_visible (true);
    }

  }
#endif
}

void 
TextService::do_cancel_edit ()
{
  //  .. nothing yet ..
}

bool 
TextService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return !sel.is_cell_inst () && sel.shape ().is_text ();
}

bool 
TextService::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_edit_text_size) {
    double size (0);
    tl::from_string (value, size);
    if (m_text.size () != size) {
      m_text.size (size);
      update_marker ();
    }
    return true; // taken
  }

  if (name == cfg_edit_text_halign) {
    db::HAlign ha = db::HAlignLeft;
    HAlignConverter hac;
    hac.from_string (value, ha);
    if (m_text.halign () != ha) {
      m_text.halign (ha);
      update_marker ();
    }
    return true; // taken
  }

  if (name == cfg_edit_text_valign) {
    db::VAlign va = db::VAlignBottom;
    VAlignConverter vac;
    vac.from_string (value, va);
    if (m_text.valign () != va) {
      m_text.valign (va);
      update_marker ();
    }
    return true; // taken
  }

  if (name == cfg_edit_text_string) {
    if (m_text.string () != value) {
      m_text.string (value);
      update_marker ();
    }
    return true; // taken
  }

  return ShapeEditService::configure (name, value);
}

// -----------------------------------------------------------------------------
//  PathService implementation

PathService::PathService (db::Manager *manager, lay::LayoutViewBase *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Paths), 
    m_width (0.1), m_bgnext (0.0), m_endext (0.0), m_type (Flush), m_needs_update (true)
{
  //  .. nothing yet ..
}

PathService::~PathService ()
{
  //  .. nothing yet ..
}

#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
PathService::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  if (view ()->is_editable ()) {
    pages.push_back (new edt::EditablePathPropertiesPage (this, manager, parent));
  } else {
    pages.push_back (new edt::PathPropertiesPage (this, manager, parent));
  }
  return pages;
}
#endif

void 
PathService::do_begin_edit (const db::DPoint &p)
{
  get_edit_layer ();

  db::DPoint pp = snap2 (p);
  m_last = pp;

  m_points.clear ();
  m_points.push_back (pp);
  m_points.push_back (pp);

  set_edit_marker (new lay::Marker (view (), cv_index ()));
  update_marker ();
}

bool
PathService::do_activated ()
{
  return false;  //  don't start editing immediately
}

void 
PathService::set_last_point (const db::DPoint &p)
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
PathService::do_mouse_move_inactive (const db::DPoint &p)
{
  lay::PointSnapToObjectResult snap_details = snap2_details (p);
  mouse_cursor_from_snap_details (snap_details);
}

void
PathService::do_mouse_move (const db::DPoint &p)
{
  do_mouse_move_inactive (p);

  set_cursor (lay::Cursor::cross);
  if (m_points.size () >= 2) {
    set_last_point (p);
  }
  update_marker ();
}

bool 
PathService::do_mouse_click (const db::DPoint &p)
{
  if (m_points.size () >= 1) {
    m_last = m_points.back ();
    m_points.push_back (db::DPoint ());
    set_last_point (p);
  }
  return false;
}

void
PathService::do_delete ()
{
  if (m_points.size () > 2) {
    m_points.erase (m_points.end () - 2);
    m_last = m_points.end()[-2];
    update_marker ();
  }
}

void
PathService::do_finish_edit ()
{
  //  one point is reserved for the "current one"
  if (m_points.size () < 3) {
    throw tl::Exception (tl::to_string (tr ("A path must have at least 2 points")));
  }
  m_points.pop_back ();

  deliver_shape (get_path ());

  commit_recent (view ());
}

void
PathService::update_marker ()
{
  lay::Marker *marker = dynamic_cast<lay::Marker *> (edit_marker ());
  if (marker) {

    db::Path path (get_path ());
    marker->set (path, db::VCplxTrans (1.0 / layout ().dbu ()) * trans ().inverted ());

    if (m_points.size () >= 2) {
      view ()->message (std::string ("lx: ") +
                        tl::micron_to_string (m_points.back ().x () - m_points.end () [-2].x ()) + 
                        std::string ("  ly: ") +
                        tl::micron_to_string (m_points.back ().y () - m_points.end () [-2].y ()) + 
                        std::string ("  l: ") +
                        tl::micron_to_string (m_points.back ().distance (m_points.end () [-2])));
    }

  }
}

db::Path 
PathService::get_path () const
{
  db::Path path;

  std::vector<db::Point> points_dbu;
  points_dbu.reserve (m_points.size ());
  for (std::vector<db::DPoint>::const_iterator p = m_points.begin (); p != m_points.end (); ++p) {
    points_dbu.push_back (trans () * *p);
  }

  path.width (trans ().ctrans (m_width));

  path.round (m_type == Round);
  if (m_type == Flush) {
    path.bgn_ext (0);
    path.end_ext (0);
  } else if (m_type == Square || m_type == Round) {
    path.bgn_ext (path.width () / 2);
    path.end_ext (path.width () / 2);
  } else {
    path.bgn_ext (trans ().ctrans (m_bgnext));
    path.end_ext (trans ().ctrans (m_endext));
  }
  path.assign (points_dbu.begin (), points_dbu.end ());

  return path;
}

void 
PathService::do_cancel_edit ()
{
  //  .. nothing yet ..
}

bool 
PathService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return !sel.is_cell_inst () && sel.shape ().is_path ();
}

bool 
PathService::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_edit_path_width) {
    tl::from_string (value, m_width);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_path_ext_var_begin) {
    tl::from_string (value, m_bgnext);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_path_ext_var_end) {
    tl::from_string (value, m_endext);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_path_width) {
    tl::from_string (value, m_width);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_path_ext_type) {
    m_type = Flush;
    if (value == "square") {
      m_type = Square;
    } else if (value == "round") {
      m_type = Round;
    } else if (value == "variable") {
      m_type = Variable;
    }
    m_needs_update = true;
    return true; // taken
  }

  return ShapeEditService::configure (name, value);
}

void 
PathService::config_finalize ()
{
  if (m_needs_update) {
    update_marker ();
    m_needs_update = false;
  }

  ShapeEditService::config_finalize ();
}

// -----------------------------------------------------------------------------
//  InstService implementation

InstService::InstService (db::Manager *manager, lay::LayoutViewBase *view)
  : edt::Service (manager, view),
    m_angle (0.0), m_scale (1.0),
    m_mirror (false), m_is_pcell (false),
    m_array (false), m_rows (1), m_columns (1), 
    m_row_x (0.0), m_row_y (0.0), m_column_x (0.0), m_column_y (0.0),
    m_place_origin (false), m_reference_transaction_id (0),
    m_needs_update (true), m_parameters_changed (false), m_has_valid_cell (false), m_in_drag_drop (false),
    m_current_cell (0), mp_current_layout (0), mp_pcell_decl (0), m_cv_index (-1)
{ 
  //  .. nothing yet ..
}

#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
InstService::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  pages.push_back (new edt::InstPropertiesPage (this, manager, parent));
  return pages;
}
#endif

bool
InstService::do_activated ()
{
  m_cv_index = view ()->active_cellview_index ();
  m_has_valid_cell = false;

  return true;  //  start editing immediately
}

tl::Variant
InstService::get_default_layer_for_pcell ()
{
  lay::LayerPropertiesConstIterator cl = view ()->current_layer ();
  if (! cl.is_null () && ! cl->has_children () && (cl->source (true).cv_index() < 0 || cl->source (true).cv_index () == view ()->active_cellview_index ())) {
    db::LayerProperties lp = cl->source (true).layer_props ();
    if (! lp.is_null ()) {
      return tl::Variant (lp);
    }
  }

  return tl::Variant ();
}

#if defined(HAVE_QT)
bool
InstService::drag_enter_event (const db::DPoint &p, const lay::DragDropDataBase *data)
{
  const lay::CellDragDropData *cd = dynamic_cast <const lay::CellDragDropData *> (data);
  if (view ()->is_editable () && cd && (cd->layout () == & view ()->active_cellview ()->layout () || cd->library ())) {

    view ()->cancel ();
    set_edit_marker (0);

    bool switch_parameters = true;

    //  configure from the drag/drop data
    if (cd->library ()) {

      //  Reject drag & drop if the target technology does not match
      if (cd->library ()->for_technologies () && view ()->cellview (view ()->active_cellview_index ()).is_valid ()) {
        if (! cd->library ()->is_for_technology (view ()->cellview (view ()->active_cellview_index ())->tech_name ())) {
          return false;
        }
      }

      if (m_lib_name != cd->library ()->get_name ()) {
        m_lib_name = cd->library ()->get_name ();
      }

    } else {
      m_lib_name.clear ();
    }

    if (cd->is_pcell ()) {

      const db::PCellDeclaration *pcell_decl = cd->layout ()->pcell_declaration (cd->cell_index ());
      if (! pcell_decl) {
        return false;
      }

      if (m_cell_or_pcell_name != pcell_decl->name ()) {
        m_cell_or_pcell_name = pcell_decl->name ();
      }

      if (! cd->pcell_params ().empty ()) {
        m_pcell_parameters = pcell_decl->named_parameters (cd->pcell_params ());
        switch_parameters = false;
      }

    } else if (cd->layout ()->is_valid_cell_index (cd->cell_index ())) {

      m_cell_or_pcell_name = cd->layout ()->cell_name (cd->cell_index ());

    } else {
      return false;
    }

    switch_cell_or_pcell (switch_parameters);

    sync_to_config ();
    m_in_drag_drop = true;

    view ()->switch_mode (plugin_declaration ()->id ());

    do_begin_edit (p);

    //  action taken.
    return true;

  }

  return false;
}

bool
InstService::drag_move_event (const db::DPoint &p, const lay::DragDropDataBase * /*data*/)
{ 
  if (m_in_drag_drop) {
    do_mouse_move (p);
    return true;
  } else {
    return false; 
  }
}

void
InstService::drag_leave_event () 
{ 
  if (m_in_drag_drop) {
    set_edit_marker (0);
    do_cancel_edit ();
  }
}

bool
InstService::drop_event (const db::DPoint & /*p*/, const lay::DragDropDataBase * /*data*/)
{
  m_in_drag_drop = false;
  return false;
}
#endif

bool
InstService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return sel.is_cell_inst ();
}

void
InstService::sync_to_config ()
{
  //  push the current setup to configuration so the instance dialog will take these as default
  //  and "apply" of these instance properties doesn't fail because of insistency.
  dispatcher ()->config_set (cfg_edit_inst_lib_name, m_lib_name);
  dispatcher ()->config_set (cfg_edit_inst_cell_name, m_cell_or_pcell_name);
  if (m_is_pcell) {
    dispatcher ()->config_set (cfg_edit_inst_pcell_parameters, pcell_parameters_to_string (m_pcell_parameters));
  } else {
    dispatcher ()->config_set (cfg_edit_inst_pcell_parameters, std::string ());
  }
  dispatcher ()->config_end ();
}

void 
InstService::do_begin_edit (const db::DPoint &p)
{
  m_has_valid_cell = false;
  m_disp = snap (p);

  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  if (cv.cell ()->is_proxy ()) {
    throw tl::Exception (tl::to_string (tr ("Cannot put an instance into a PCell or library cell")));
  }

  m_trans = cv.context_trans ();

  std::pair<bool, db::cell_index_type> ci = make_cell (cv);
  if (ci.first) {
    // use the snapped lower left corner of the bbox unless the origin is inside the bbox
    db::Box cell_bbox = cv->layout ().cell (ci.second).bbox ();
    if (! m_place_origin && ! cell_bbox.contains (db::Point ())) {
      db::CplxTrans ct (1.0, m_angle, m_mirror, db::DVector ());
      m_disp = db::DPoint () + (m_disp - snap (cell_bbox.transformed (ct).lower_left () * cv->layout ().dbu ()));
    }
  }

  //  compute the transformation variants
  //  TODO: this is duplicated code
  //  TODO: from this computed vector we take just the first one!
  std::vector<db::DCplxTrans> tv;
  for (lay::LayerPropertiesConstIterator l = view ()->begin_layers (); !l.at_end (); ++l) {
    if (! l->has_children ()) {
      int cvi = (l->cellview_index () >= 0) ? l->cellview_index () : 0;
      if (cvi == m_cv_index) {
        tv.insert (tv.end (), l->trans ().begin (), l->trans ().end ());
      }
    }
  }
  std::sort (tv.begin (), tv.end ());
  tv.erase (std::unique (tv.begin (), tv.end ()), tv.end ());
  if (! tv.empty ()) {
    m_trans = db::VCplxTrans (1.0 / cv->layout ().dbu ()) * tv [0] * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ();
  }

  update_marker ();
}

std::pair<bool, db::cell_index_type> 
InstService::make_cell (const lay::CellView &cv)
{
  if (m_has_valid_cell) {
    return std::make_pair (true, m_current_cell);
  }

  //  NOTE: do this at the beginning: creating a transaction might delete transactions behind the
  //  head transaction, hence releasing (thus: deleting) cells. To prevert interference, create
  //  the transaction at the beginning.
  db::Transaction transaction (manager (), tl::to_string (tr ("Create reference cell")), m_reference_transaction_id);

  lay::LayerState layer_state = view ()->layer_snapshot ();

  db::Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (m_lib_name, cv->tech_name ());

  //  find the layout the cell has to be looked up: that is either the layout of the current instance or 
  //  the library selected
  if (lib) {
    mp_current_layout = &lib->layout ();
  } else {
    mp_current_layout = &cv->layout ();
  }

  std::pair<bool, db::cell_index_type> ci (false, db::cell_index_type (0));
  std::pair<bool, db::pcell_id_type> pci (false, db::pcell_id_type (0));

  if (! m_is_pcell) {
    ci = mp_current_layout->cell_by_name (m_cell_or_pcell_name.c_str ());
  } else {
    pci = mp_current_layout->pcell_by_name (m_cell_or_pcell_name.c_str ());
  }

  if (! ci.first && ! pci.first) {
    return std::pair<bool, db::cell_index_type> (false, 0);
  }

  db::cell_index_type inst_cell_index = ci.second;

  mp_pcell_decl = 0;

  //  instantiate the PCell
  if (pci.first) {

    std::vector<tl::Variant> pv;

    mp_pcell_decl = mp_current_layout->pcell_declaration (pci.second);
    if (mp_pcell_decl) {

      pv = mp_pcell_decl->map_parameters (m_pcell_parameters);

      //  make the parameters fit (i.e. PCells may not define consistent default parameters)
      mp_pcell_decl->coerce_parameters (*mp_current_layout, pv);

    }

    inst_cell_index = mp_current_layout->get_pcell_variant (pci.second, pv);

  }

  //  reference the library
  if (lib) {

    mp_current_layout = & cv->layout ();
    inst_cell_index = mp_current_layout->get_lib_proxy (lib, inst_cell_index);

    //  remove unused references
    std::set<db::cell_index_type> keep;
    keep.insert (inst_cell_index);
    mp_current_layout->cleanup (keep);

  }

  view ()->add_new_layers (layer_state);

  m_has_valid_cell = true;
  m_current_cell = inst_cell_index;

  if (! transaction.is_empty ()) {
    m_reference_transaction_id = transaction.id ();
  }

  return std::pair<bool, db::cell_index_type> (true, inst_cell_index);
}

void
InstService::do_mouse_move_inactive (const db::DPoint &p)
{
  clear_mouse_cursors ();
  add_mouse_cursor (snap (p));
}

void
InstService::do_mouse_move (const db::DPoint &p)
{
  do_mouse_move_inactive (p);

  set_cursor (lay::Cursor::cross);

  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  m_disp = snap (p);

  std::pair<bool, db::cell_index_type> ci = make_cell (cv);
  if (ci.first) {
    //  use the snapped lower left corner of the bbox unless the origin is inside the bbox
    db::Box cell_bbox = cv->layout ().cell (ci.second).bbox ();
    if (! m_place_origin && ! cell_bbox.contains (db::Point ())) {
      db::CplxTrans ct (1.0, m_angle, m_mirror, db::DVector ());
      m_disp = db::DPoint () + (m_disp - snap (cell_bbox.transformed (ct).lower_left () * cv->layout ().dbu ()));
    }
  }

  update_marker ();
}

void 
InstService::do_mouse_transform (const db::DPoint &p, db::DFTrans trans)
{
  db::DCplxTrans ct (1.0, m_angle, m_mirror, db::DVector ());
  ct *= db::DCplxTrans (trans);

  m_angle = ct.angle ();
  m_mirror = ct.is_mirror ();

  db::DPoint r (m_row_x, m_row_y);
  r.transform (trans);
  m_row_x = r.x ();
  m_row_y = r.y ();

  db::DPoint c (m_column_x, m_column_y);
  c.transform (trans);
  m_column_x = c.x ();
  m_column_y = c.y ();

  dispatcher ()->config_set (cfg_edit_inst_angle, m_angle);
  dispatcher ()->config_set (cfg_edit_inst_mirror, m_mirror);
  dispatcher ()->config_set (cfg_edit_inst_row_x, m_row_x);
  dispatcher ()->config_set (cfg_edit_inst_row_y, m_row_y);
  dispatcher ()->config_set (cfg_edit_inst_column_x, m_column_x);
  dispatcher ()->config_set (cfg_edit_inst_column_y, m_column_y);
  dispatcher ()->config_end ();

  //  honour the new transformation
  do_mouse_move (p);
}

bool 
InstService::do_mouse_click (const db::DPoint &p)
{
  do_mouse_move (p);
  return true;
}

void 
InstService::do_finish_edit ()
{
  try {

    db::CellInstArray inst;
    if (get_inst (inst)) {

      //  check for recursive hierarchy
      const lay::CellView &cv = view ()->cellview (m_cv_index);
      std::set <db::cell_index_type> called, callers;

      cv->layout ().cell (inst.object ().cell_index ()).collect_called_cells (called);
      called.insert (inst.object ().cell_index ());
      cv->layout ().cell (cv.cell_index ()).collect_caller_cells (callers);
      callers.insert (cv.cell_index ());

      std::vector <db::cell_index_type> intersection;
      std::set_intersection (called.begin (), called.end (), callers.begin (), callers.end (), std::back_inserter (intersection));
      if (! intersection.empty ()) {
        throw tl::Exception (tl::to_string (tr ("Inserting this instance would create a recursive hierarchy")));
      }

      if (manager ()) {
        manager ()->transaction (tl::to_string (tr ("Create instance")), m_reference_transaction_id);
      }
      m_reference_transaction_id = 0;
      db::Instance i = cv->layout ().cell (cv.cell_index ()).insert (inst);
      cv->layout ().cleanup ();
      if (manager ()) {
        manager ()->commit ();
      }

      commit_recent (view ());

      if (m_in_drag_drop) {

        lay::ObjectInstPath sel;
        sel.set_cv_index (m_cv_index);
        sel.set_topcell (cv.cell_index ());
        sel.add_path (db::InstElement (i, db::CellInstArray::iterator ()));

        add_selection (sel);

      }

    }

    m_has_valid_cell = false;
    m_in_drag_drop = false;

  } catch (...) {
    m_has_valid_cell = false;
    m_in_drag_drop = false;
    throw;
  }
}

void 
InstService::do_cancel_edit ()
{
  //  Undo "create reference" transactions which basically unfinished "create instance" transactions
  if (m_reference_transaction_id > 0 && manager ()->transaction_id_for_undo () == m_reference_transaction_id) {
    manager ()->undo ();
  }

  m_reference_transaction_id = 0;
  m_has_valid_cell = false;
  m_in_drag_drop = false;

  set_edit_marker (0);

  //  clean up any proxy cells created so far 
  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (cv.is_valid ()) {
    cv->layout ().cleanup ();
  }
}

void
InstService::service_configuration_changed ()
{
  m_needs_update = true;
}

bool 
InstService::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_edit_inst_cell_name) {

    if (value != m_cell_or_pcell_name) {
      m_cell_or_pcell_name = value;
      m_needs_update = true;
    }

    return true; // taken
  }

  if (name == cfg_edit_inst_lib_name) {

    if (value != m_lib_name) {
      m_lib_name_previous = m_lib_name;
      m_lib_name = value;
      m_needs_update = true;
    }

    return true; // taken
  }

  if (name == cfg_edit_inst_pcell_parameters) {

    std::map<std::string, tl::Variant> pcp = pcell_parameters_from_string (value);
    if (pcp != m_pcell_parameters) {

      m_pcell_parameters = pcp;
      m_is_pcell = ! value.empty ();

      m_needs_update = true;
      m_parameters_changed = true;

    }

    return true; // taken

  }

  if (name == cfg_edit_inst_place_origin) {

    bool f;
    tl::from_string (value, f);

    if (f != m_place_origin) {
      m_place_origin = f;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_scale) {

    double s;
    tl::from_string (value, s);

    if (fabs (s - m_scale) > 1e-10) {
      m_scale = s;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_angle) {

    double a;
    tl::from_string (value, a);

    if (fabs (a - m_angle) > 1e-10) {
      m_angle = a;
      m_needs_update = true;
    }

    return true; // taken
  }

  if (name == cfg_edit_inst_mirror) {

    bool f;
    tl::from_string (value, f);

    if (f != m_mirror) {
      m_mirror = f;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_array) {

    bool f;
    tl::from_string (value, f);

    if (f != m_array) {
      m_array = f;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_rows) {

    unsigned int v;
    tl::from_string (value, v);

    if (v != m_rows) {
      m_rows = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_row_x) {

    double v;
    tl::from_string (value, v);

    if (! db::coord_traits<double>::equal (m_row_x, v)) {
      m_row_x = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_row_y) {

    double v;
    tl::from_string (value, v);

    if (! db::coord_traits<double>::equal (m_row_y, v)) {
      m_row_y = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_columns) {

    unsigned int v;
    tl::from_string (value, v);

    if (v != m_columns) {
      m_columns = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_column_x) {

    double v;
    tl::from_string (value, v);

    if (! db::coord_traits<double>::equal (m_column_x, v)) {
      m_column_x = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  if (name == cfg_edit_inst_column_y) {

    double v;
    tl::from_string (value, v);

    if (! db::coord_traits<double>::equal (m_column_y, v)) {
      m_column_y = v;
      m_needs_update = true;
    }

    return true; // taken

  }

  return edt::Service::configure (name, value);
}

void
InstService::switch_cell_or_pcell (bool switch_parameters)
{
  //  if the library or cell name has changed, store the current pcell parameters and try to reuse
  //  an existing parameter set
  if (! m_cell_or_pcell_name_previous.empty () && (m_cell_or_pcell_name_previous != m_cell_or_pcell_name || m_lib_name_previous != m_lib_name)) {

    m_stored_pcell_parameters[std::make_pair (m_cell_or_pcell_name_previous, m_lib_name_previous)] = m_pcell_parameters;

    if (switch_parameters) {

      std::map<std::pair<std::string, std::string>, std::map<std::string, tl::Variant> >::const_iterator p = m_stored_pcell_parameters.find (std::make_pair (m_cell_or_pcell_name, m_lib_name));
      if (p != m_stored_pcell_parameters.end ()) {
        m_pcell_parameters = p->second;
      } else {
        m_pcell_parameters.clear ();
      }

    }

  }

  const lay::CellView &cv = view ()->cellview (m_cv_index);
  db::Library *lib = 0;
  if (cv.is_valid ()) {
    lib = db::LibraryManager::instance ().lib_ptr_by_name (m_lib_name, cv->tech_name ());
  } else {
    lib = db::LibraryManager::instance ().lib_ptr_by_name (m_lib_name);
  }

  //  find the layout the cell has to be looked up: that is either the layout of the current instance or
  //  the library selected
  const db::Layout *layout = 0;
  if (lib) {
    layout = &lib->layout ();
  } else if (cv.is_valid ()) {
    layout = &cv->layout ();
  }

  if (layout) {
    m_is_pcell = layout->pcell_by_name (m_cell_or_pcell_name.c_str ()).first;
  } else {
    m_is_pcell = false;
  }

  //  remember the current cell or library name
  m_cell_or_pcell_name_previous = m_cell_or_pcell_name;
  m_lib_name_previous = m_lib_name;
}

void 
InstService::config_finalize ()
{
  if (m_needs_update) {

    //  don't switch parameters if they have been updated explicitly
    //  since the last "config_finalize". This means the sender of the configuration events
    //  wants the parameters to be set in a specific way. Don't interfere.
    bool switch_parameters = ! m_parameters_changed;

    switch_cell_or_pcell (switch_parameters);

    m_has_valid_cell = false;
    update_marker ();

    if (switch_parameters) {
      //  Reflects any changes in PCell parameters in the configuration
      //  TODO: it's somewhat questionable to do this inside "config_finalize" as this method is supposed
      //  to reflect changes rather than induce some.
      if (m_is_pcell) {
        dispatcher ()->config_set (cfg_edit_inst_pcell_parameters, pcell_parameters_to_string (m_pcell_parameters));
      } else {
        dispatcher ()->config_set (cfg_edit_inst_pcell_parameters, std::string ());
      }
    }

  }

  m_needs_update = false;
  m_parameters_changed = false;

  edt::Service::config_finalize ();
}

void
InstService::update_marker ()
{
  if (editing ()) {

    lay::Marker *marker = new lay::Marker (view (), m_cv_index, ! show_shapes_of_instances (), show_shapes_of_instances () ? max_shapes_of_instances () : 0);
    marker->set_vertex_shape (lay::ViewOp::Cross);
    marker->set_vertex_size (9 /*cross vertex size*/);
    set_edit_marker (marker);

    db::CellInstArray inst;
    if (get_inst (inst)) {
      marker->set (inst, m_trans);
    } else {
      marker->set ();
    }

  } else {
    set_edit_marker (0);
  }
}

bool
InstService::get_inst (db::CellInstArray &inst) 
{
  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (cv.is_valid ()) {

    std::pair<bool, db::cell_index_type> ci = make_cell (cv);
    if (ci.first) {

      //  compute the instance's transformation
      db::VCplxTrans pt = (db::CplxTrans (cv->layout ().dbu ()) * m_trans).inverted ();
      db::ICplxTrans trans = db::ICplxTrans (m_scale, m_angle, m_mirror, pt * m_disp - db::Point ());

      if (m_array && m_rows > 0 && m_columns > 0) {
        db::Vector row = db::Vector (pt * db::DVector (m_row_x, m_row_y));
        db::Vector column = db::Vector (pt * db::DVector (m_column_x, m_column_y));
        inst = db::CellInstArray (db::CellInst (ci.second), trans, row, column, m_rows, m_columns);
      } else {
        inst = db::CellInstArray (db::CellInst (ci.second), trans);
      }

      return true;

    }

  }
  return false;
}

} // namespace edt



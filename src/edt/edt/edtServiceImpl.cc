
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


#include "edtMainService.h"
#include "edtServiceImpl.h"
#include "edtPropertiesPages.h"
#include "edtInstPropertiesPage.h"
#include "edtPCellParametersDialog.h"
#include "edtService.h"
#include "dbEdge.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"
#include "dbPCellDeclaration.h"
#include "dbPolygonTools.h"
#include "dbEdgeProcessor.h"
#include "layMarker.h"
#include "layLayerProperties.h"
#include "layLayoutView.h"
#include "layTipDialog.h"

#include <QInputDialog>
#include <QApplication>

namespace edt
{

// -----------------------------------------------------------------------------
//  ShapeEditService implementation

ShapeEditService::ShapeEditService (db::Manager *manager, lay::LayoutView *view, db::ShapeIterator::flags_type shape_types)
  : edt::Service (manager, view, shape_types), 
    m_layer (0), m_cv_index (0), mp_cell (0), mp_layout (0), m_combine_mode (CM_Add)
{
  //  .. nothing yet ..
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
    throw tl::Exception (tl::to_string (QObject::tr ("Please select a layer first")));
  }

  if (! cl->visible (true)) {
    lay::TipDialog td (QApplication::activeWindow (),
                       tl::to_string (QObject::tr ("You are about to draw on a hidden layer. The result won't be visible.")), 
                       "drawing-on-invisible-layer");
    td.exec_dialog ();
  }
  
  int cv_index = cl->cellview_index ();
  const lay::CellView &cv = view ()->cellview (cv_index);
  int layer = cl->layer_index ();

  if (cv_index < 0 || ! cv.is_valid ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("Please select a cell first")));
  }

  if (layer < 0 || ! cv->layout ().is_valid_layer ((unsigned int) layer)) {

    if (cl->has_children ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Please select a valid drawing layer first")));
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
    throw tl::Exception (tl::to_string (QObject::tr ("Cannot put a shape into a PCell or library cell")));
  }
}

/**
 *  @brief Deliver a good interpolation between two points m and p
 *
 *  This method uses an intermediate point o to determine the edge that is emerged from point m.
 *  An edge is searched that emerges from p and intersects with the m->o edge in a way that the intersection
 *  point is closest to o.
 *
 *  This method returns the intersection point ("new o") and a flag if the search was sucessful (.first of return value).
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

    manager ()->transaction (tl::to_string (QObject::tr ("Create polygon"))); 
    cell ().shapes (layer ()).insert (poly);
    manager ()->commit ();

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

    manager ()->transaction (tl::to_string (QObject::tr ("Combine shape with background"))); 

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

    manager ()->commit ();

  }
}

void 
ShapeEditService::deliver_shape (const db::Path &path)
{
  if (m_combine_mode == CM_Add) {
    manager ()->transaction (tl::to_string (QObject::tr ("Create path"))); 
    cell ().shapes (layer ()).insert (path);
    manager ()->commit ();
  } else {
    deliver_shape (path.polygon ());
  }
}

void 
ShapeEditService::deliver_shape (const db::Box &box)
{
  if (m_combine_mode == CM_Add) {
    manager ()->transaction (tl::to_string (QObject::tr ("Create box"))); 
    cell ().shapes (layer ()).insert (box);
    manager ()->commit ();
  } else {
    deliver_shape (db::Polygon (box));
  }
}

// -----------------------------------------------------------------------------
//  PolygonService implementation

PolygonService::PolygonService (db::Manager *manager, lay::LayoutView *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Polygons),
    m_closure_set (false), m_closure ()
{
  //  .. nothing yet ..
}

lay::PropertiesPage *
PolygonService::properties_page (db::Manager *manager, QWidget *parent)
{
  return new edt::PolygonPropertiesPage (this, manager, parent);
}

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
PolygonService::do_mouse_move (const db::DPoint &p)
{
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
}

db::Polygon
PolygonService::get_polygon () const
{
  db::Polygon poly;

  if (m_points.size () < 4) {
    throw tl::Exception (tl::to_string (QObject::tr ("A polygon must have at least 3 points")));
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
    throw tl::Exception (tl::to_string (QObject::tr ("A polygon must have at least 3 effective points")));
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

BoxService::BoxService (db::Manager *manager, lay::LayoutView *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Boxes)
{ 
  //  .. nothing yet ..
}
  
lay::PropertiesPage *
BoxService::properties_page (db::Manager *manager, QWidget *parent)
{
  return new edt::BoxPropertiesPage (this, manager, parent);
}

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
BoxService::do_mouse_move (const db::DPoint &p)
{
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
//  TextService implementation

TextService::TextService (db::Manager *manager, lay::LayoutView *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Texts),
    m_rot (0)
{ 
  //  .. nothing yet ..
}

TextService::~TextService ()
{
  //  .. nothing yet ..
}

lay::PropertiesPage *
TextService::properties_page (db::Manager *manager, QWidget *parent)
{
  return new edt::TextPropertiesPage (this, manager, parent);
}

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

  //  Show editor options dialog to allow entering of width
  std::vector<edt::MainService *> edt_main_services = view ()->get_plugins <edt::MainService> ();
  if (edt_main_services.size () > 0) {
    edt_main_services [0]->cm_edit_options ();
  }

  return true;  //  start editing immediately
}

void 
TextService::do_mouse_move (const db::DPoint &p)
{
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

  manager ()->transaction (tl::to_string (QObject::tr ("Create text"))); 
  cell ().shapes (layer ()).insert (get_text ());
  manager ()->commit ();

  if (! view ()->text_visible ()) {

    lay::TipDialog td (QApplication::activeWindow (),
                       tl::to_string (QObject::tr ("A text object is created but texts are disabled for drawing and are not visible. Do you want to enable drawing of texts?\n\nChoose \"Yes\" to enable text drawing now.")), 
                       "text-created-but-not-visible",
                       lay::TipDialog::yesno_buttons);

    lay::TipDialog::button_type button = lay::TipDialog::null_button;
    td.exec_dialog (button);
    if (button == lay::TipDialog::yes_button) {
      view ()->text_visible (true);
    }

  }
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

PathService::PathService (db::Manager *manager, lay::LayoutView *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Paths), 
    m_width (0.1), m_bgnext (0.0), m_endext (0.0), m_type (Flush), m_needs_update (true)
{
  //  .. nothing yet ..
}

PathService::~PathService ()
{
  //  .. nothing yet ..
}

lay::PropertiesPage *
PathService::properties_page (db::Manager *manager, QWidget *parent)
{
  if (view ()->is_editable ()) {
    return new edt::EditablePathPropertiesPage (this, manager, parent);
  } else {
    return new edt::PathPropertiesPage (this, manager, parent);
  }
}

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
  //  Show editor options dialog to allow entering of width
  std::vector<edt::MainService *> edt_main_services = view ()->get_plugins <edt::MainService> ();
  if (edt_main_services.size () > 0) {
    edt_main_services [0]->cm_edit_options ();
  }

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
PathService::do_mouse_move (const db::DPoint &p)
{
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
PathService::do_finish_edit ()
{
  //  one point is reserved for the "current one"
  if (m_points.size () < 3) {
    throw tl::Exception (tl::to_string (QObject::tr ("A path must have at least 2 points")));
  }
  m_points.pop_back ();

  deliver_shape (get_path ());
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

InstService::InstService (db::Manager *manager, lay::LayoutView *view)
  : edt::Service (manager, view),
    m_angle (0.0), m_scale (1.0),
    m_mirror (false), m_is_pcell (false),
    m_array (false), m_rows (1), m_columns (1), 
    m_row_x (0.0), m_row_y (0.0), m_column_x (0.0), m_column_y (0.0),
    m_place_origin (false), m_reference_transaction_id (0),
    m_needs_update (true), m_has_valid_cell (false), m_in_drag_drop (false), 
    m_current_cell (0), mp_current_layout (0), mp_pcell_decl (0), m_cv_index (-1)
{ 
  //  .. nothing yet ..
}

lay::PropertiesPage *
InstService::properties_page (db::Manager *manager, QWidget *parent)
{
  return new edt::InstPropertiesPage (this, manager, parent);
}

bool
InstService::do_activated ()
{
  //  Show editor options dialog to allow entering of parameters
  std::vector<edt::MainService *> edt_main_services = view ()->get_plugins <edt::MainService> ();
  if (edt_main_services.size () > 0) {
    edt_main_services [0]->cm_edit_options ();
  }

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

bool
InstService::drag_enter_event (const db::DPoint &p, const lay::DragDropDataBase *data)
{ 
  const lay::CellDragDropData *cd = dynamic_cast <const lay::CellDragDropData *> (data);
  if (view ()->is_editable () && cd && (cd->layout () == & view ()->active_cellview ()->layout () || cd->library ())) {

    view ()->cancel ();

    set_edit_marker (0);

    m_cv_index = view ()->active_cellview_index ();
    m_in_drag_drop = true;

    if (cd->library ()) {
      if (m_lib_name != cd->library ()->get_name ()) {
        m_lib_name = cd->library ()->get_name ();
        m_pcell_parameters.clear ();
      }
    } else {
      m_lib_name.clear ();
    }

    m_is_pcell = false;

    if (cd->is_pcell ()) {

      const db::PCellDeclaration *pcell_decl = cd->layout ()->pcell_declaration (cd->cell_index ());
      if (pcell_decl) {

        if (m_cell_or_pcell_name != pcell_decl->name ()) {
          m_cell_or_pcell_name = pcell_decl->name ();
          m_pcell_parameters.clear ();
        }

        m_is_pcell = true;

        //  NOTE: we reuse previous parameters for convenience unless PCell or library has changed
        const std::vector<db::PCellParameterDeclaration> &pd = pcell_decl->parameter_declarations();
        for (std::vector<db::PCellParameterDeclaration>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
          if (i->get_type () == db::PCellParameterDeclaration::t_layer && !i->is_hidden () && !i->is_readonly () && i->get_default ().is_nil ()) {
            m_pcell_parameters.insert (std::make_pair (i->get_name (), get_default_layer_for_pcell ()));
          } else {
            m_pcell_parameters.insert (std::make_pair (i->get_name (), i->get_default ()));
          }
        }

        do_begin_edit (p);
        return true;

      }

    } else if (cd->layout ()->is_valid_cell_index (cd->cell_index ())) {

      m_cell_or_pcell_name = cd->layout ()->cell_name (cd->cell_index ());
      do_begin_edit (p);
      return true;

    }

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
InstService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return sel.is_cell_inst ();
}

bool  
InstService::drop_event (const db::DPoint & /*p*/, const lay::DragDropDataBase * /*data*/) 
{ 
  if (m_in_drag_drop) {

    const lay::CellView &cv = view ()->cellview (m_cv_index);
    if (! cv.is_valid ()) {
      return false;
    }

    make_cell (cv);

    bool accepted = true;

    if (m_has_valid_cell && mp_pcell_decl) {

      std::vector<tl::Variant> pv = mp_pcell_decl->map_parameters (m_pcell_parameters);

      //  Turn off the drag cursor for the modal dialog
      QApplication::restoreOverrideCursor ();

      //  for PCells dragged show the parameter dialog for a chance to edit the initial parameters
      if (! mp_pcell_parameters_dialog.get ()) {
        mp_pcell_parameters_dialog.reset (new edt::PCellParametersDialog (view ()));
        mp_pcell_parameters_dialog->parameters_changed_event.add (this, &InstService::apply_edits);
      }

      if (! mp_pcell_parameters_dialog->exec (mp_current_layout, view (), m_cv_index, mp_pcell_decl, pv)) {
        accepted = false;
      } else {
        m_has_valid_cell = false;
        m_pcell_parameters = mp_pcell_decl->named_parameters (mp_pcell_parameters_dialog->get_parameters ());
      }

    }

    set_edit_marker (0);

    if (accepted) {
      do_finish_edit ();
    } else {
      do_cancel_edit ();
    }

    sync_to_config ();
    return true;

  } else {
    return false; 
  }
}

void
InstService::sync_to_config ()
{
  //  push the current setup to configuration so the instance dialog will take these as default
  //  and "apply" of these instance properties doesn't fail because of insistency.
  plugin_root ()->config_set (cfg_edit_inst_lib_name, m_lib_name);
  plugin_root ()->config_set (cfg_edit_inst_cell_name, m_cell_or_pcell_name);
  if (m_is_pcell) {
    plugin_root ()->config_set (cfg_edit_inst_pcell_parameters, pcell_parameters_to_string (m_pcell_parameters));
  } else {
    plugin_root ()->config_set (cfg_edit_inst_pcell_parameters, std::string ());
  }
  plugin_root ()->config_end ();
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
    throw tl::Exception (tl::to_string (QObject::tr ("Cannot put an instance into a PCell or library cell")));
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

  lay::Marker *marker = new lay::Marker (view (), m_cv_index, ! show_shapes_of_instances (), show_shapes_of_instances () ? max_shapes_of_instances () : 0);
  marker->set_vertex_shape (lay::ViewOp::Cross);
  marker->set_vertex_size (9 /*cross vertex size*/);
  set_edit_marker (marker);
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
  db::Transaction tr (manager (), tl::to_string (QObject::tr ("Create reference cell")), m_reference_transaction_id);
  m_reference_transaction_id = tr.id ();

  lay::LayerState layer_state = view ()->layer_snapshot ();

  db::Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (m_lib_name);

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

  return std::pair<bool, db::cell_index_type> (true, inst_cell_index);
}

void 
InstService::do_mouse_move (const db::DPoint &p)
{
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
        throw tl::Exception (tl::to_string (QObject::tr ("Inserting this instance would create a recursive hierarchy")));
      }

      manager ()->transaction (tl::to_string (QObject::tr ("Create instance")), m_reference_transaction_id); 
      m_reference_transaction_id = 0;
      db::Instance i = cv->layout ().cell (cv.cell_index ()).insert (inst);
      cv->layout ().cleanup ();
      manager ()->commit ();

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
  if (m_reference_transaction_id > 0 && manager ()->last_transaction_id () == m_reference_transaction_id) {
    manager ()->undo ();
  }

  m_reference_transaction_id = 0;
  m_has_valid_cell = false;
  m_in_drag_drop = false;

  //  clean up any proxy cells created so far 
  const lay::CellView &cv = view ()->cellview (m_cv_index);
  if (cv.is_valid ()) {
    cv->layout ().cleanup ();
  }
}

bool 
InstService::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_edit_inst_cell_name) {
    m_cell_or_pcell_name = value;
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_lib_name) {
    m_lib_name = value;
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_pcell_parameters) {

    m_pcell_parameters = pcell_parameters_from_string (value);
    m_is_pcell = ! value.empty ();

    m_needs_update = true;
    return true; // taken

  }

  if (name == cfg_edit_inst_place_origin) {
    tl::from_string (value, m_place_origin);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_scale) {
    tl::from_string (value, m_scale);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_angle) {
    tl::from_string (value, m_angle);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_mirror) {
    tl::from_string (value, m_mirror);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_array) {
    tl::from_string (value, m_array);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_rows) {
    tl::from_string (value, m_rows);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_row_x) {
    tl::from_string (value, m_row_x);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_row_y) {
    tl::from_string (value, m_row_y);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_columns) {
    tl::from_string (value, m_columns);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_column_x) {
    tl::from_string (value, m_column_x);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_inst_column_y) {
    tl::from_string (value, m_column_y);
    m_needs_update = true;
    return true; // taken
  }

  return edt::Service::configure (name, value);
}

void 
InstService::config_finalize ()
{
  if (m_needs_update) {
    m_has_valid_cell = false;
    update_marker ();
    m_needs_update = false;
  }

  edt::Service::config_finalize ();
}

void
InstService::apply_edits()
{
  if (mp_pcell_decl && mp_pcell_parameters_dialog.get ()) {
    m_pcell_parameters = mp_pcell_decl->named_parameters (mp_pcell_parameters_dialog->get_parameters ());
  }

  sync_to_config ();
}

void
InstService::update_marker ()
{
  lay::Marker *marker = dynamic_cast<lay::Marker *> (edit_marker ());
  if (marker) {
    marker->set ();
    db::CellInstArray inst;
    if (get_inst (inst)) {
      marker->set (inst, m_trans);
    }
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
      db::ICplxTrans trans;
      if (m_in_drag_drop) {
        trans = db::ICplxTrans (1.0, 0.0, false, pt * m_disp - db::Point ());
      } else {
        trans = db::ICplxTrans (m_scale, m_angle, m_mirror, pt * m_disp - db::Point ());
      } 

      if (! m_in_drag_drop && m_array && m_rows > 0 && m_columns > 0) {
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




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

#include "layEditorServiceBase.h"
#include "layViewport.h"
#include "layLayoutView.h"

namespace lay
{

// --------------------------------------------------------------------------------------

template <size_t num_pts>
static void
make_circle (double r, const db::DPoint &center, db::DPolygon &poly, bool as_hole)
{
  db::DPoint pts [num_pts];
  for (size_t i = 0; i < num_pts; ++i) {
    double x = r * cos (M_PI * 2.0 * double (i) / double (num_pts));
    double y = r * sin (M_PI * 2.0 * double (i) / double (num_pts));
    pts [i] = center + db::DVector (x, y);
  }

  if (! as_hole) {
    poly.assign_hull (pts, pts + num_pts);
  } else {
    poly.insert_hole (pts, pts + num_pts);
  }
}

class MouseCursorViewObject
  : public lay::ViewObject
{
public:
  MouseCursorViewObject (lay::ViewObjectWidget *widget, const db::DPoint &pt, bool solid)
    : lay::ViewObject (widget, false), m_pt (pt), m_solid (solid)
  { }

  virtual void render (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas)
  {
    int dither_pattern = 0; // solid
    int cross_dither_pattern = 6;  // dotted

    int lw = int (0.5 + 1.0 / canvas.resolution ());

    std::vector <lay::ViewOp> ops;
    ops.resize (1);
    ops[0] = lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, 0, (unsigned int) dither_pattern, 0, lay::ViewOp::Rect, lw, 0);
    lay::CanvasPlane *plane = canvas.plane (ops);

    ops[0] = lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, 0, (unsigned int) cross_dither_pattern, 0, lay::ViewOp::Rect, lw, 0);
    lay::CanvasPlane *cross_plane = canvas.plane (ops);

    lay::Renderer &r = canvas.renderer ();

    double rad = 4.0 / canvas.resolution () / vp.trans ().mag ();

    db::DPolygon c;
    if (! m_solid) {
      make_circle<size_t (16)> (rad, m_pt, c, false);
      r.draw (c, vp.trans (), 0, plane, 0, 0);
    } else {
      make_circle<size_t (16)> (rad * 2, m_pt, c, false);
      r.draw (c, vp.trans (), 0, plane, 0, 0);
      make_circle<size_t (16)> (rad, m_pt, c, false);
      r.draw (c, vp.trans (), 0, plane, 0, 0);
    }

    r.draw (db::DEdge (m_pt + db::DVector (0, -rad * 4), m_pt + db::DVector (0, rad * 4)), vp.trans (), 0, cross_plane, 0, 0);
    r.draw (db::DEdge (m_pt + db::DVector (-rad * 4, 0), m_pt + db::DVector (rad * 4, 0)), vp.trans (), 0, cross_plane, 0, 0);
  }

private:
  db::DPoint m_pt;
  bool m_solid;
};

class EdgeMarkerViewObject
  : public lay::ViewObject
{
public:
  EdgeMarkerViewObject (lay::ViewObjectWidget *widget, const db::DEdge &edge, bool solid)
    : lay::ViewObject (widget, false), m_edge (edge), m_solid (solid)
  { }

  virtual void render (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas)
  {
    if (m_edge.is_degenerate ()) {
      return;
    }

    int dashed_style = 2;
    int solid_style = 0;

    int lw = int (0.5 + 1.0 / canvas.resolution ());

    std::vector <lay::ViewOp> ops;
    ops.resize (1);
    ops[0] = lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, solid_style, 0, 0, lay::ViewOp::Rect, lw, 0);
    lay::CanvasPlane *arrow_plane = canvas.plane (ops);

    ops[0] = lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, m_solid ? solid_style : dashed_style, 1, 0, lay::ViewOp::Rect, lw, 0);
    lay::CanvasPlane *edge_plane = canvas.plane (ops);

    lay::Renderer &r = canvas.renderer ();
    r.draw (m_edge, vp.trans (), 0, edge_plane, 0, 0);

    double arrow_length = 12.0 / canvas.resolution () / vp.trans ().mag ();

    double arrow_width_half = arrow_length * 0.25882; // sin(15 deg)
    db::DVector n = db::DVector (m_edge.dy (), -m_edge.dx ()) * (arrow_width_half / m_edge.length ());
    db::DVector d = db::DVector (m_edge.dx (), m_edge.dy ()) * (arrow_length / m_edge.length ());

    if (m_edge.length () < 2 * arrow_length) {

      r.draw (db::DEdge (m_edge.p1 () - n, m_edge.p1 () + n), vp.trans (), 0, arrow_plane, 0, 0);
      r.draw (db::DEdge (m_edge.p2 () - n, m_edge.p2 () + n), vp.trans (), 0, arrow_plane, 0, 0);

    } else {

      db::DPoint pts[3];
      db::DPolygon p;

      pts[0] = m_edge.p1 ();
      pts[1] = m_edge.p1 () + d - n;
      pts[2] = m_edge.p1 () + d + n;

      p.assign_hull (pts, pts + 3);
      r.draw (p, vp.trans (), 0, arrow_plane, 0, 0);

      pts[0] = m_edge.p2 ();
      pts[1] = m_edge.p2 () - d + n;
      pts[2] = m_edge.p2 () - d - n;

      p.assign_hull (pts, pts + 3);
      r.draw (p, vp.trans (), 0, arrow_plane, 0, 0);

    }
  }

private:
  db::DEdge m_edge;
  bool m_solid;
};

// --------------------------------------------------------------------------------------

EditorServiceBase::EditorServiceBase (lay::LayoutView *view)
  : lay::ViewService (view->view_object_widget ()),
    lay::Editable (view)
{
  //  .. nothing yet ..
}

EditorServiceBase::~EditorServiceBase ()
{
  clear_mouse_cursors ();
}

void
EditorServiceBase::add_mouse_cursor (const db::DPoint &pt, bool emphasize)
{
  m_mouse_cursor_markers.push_back (new MouseCursorViewObject (widget (), pt, emphasize));
}

void
EditorServiceBase::add_edge_marker (const db::DEdge &e, bool emphasize)
{
  m_mouse_cursor_markers.push_back (new EdgeMarkerViewObject (widget (), e, emphasize));
}

void
EditorServiceBase::clear_mouse_cursors ()
{
  for (std::vector<lay::ViewObject *>::iterator r = m_mouse_cursor_markers.begin (); r != m_mouse_cursor_markers.end (); ++r) {
    delete *r;
  }
  m_mouse_cursor_markers.clear ();
}

}

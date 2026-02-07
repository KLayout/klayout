
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


#include "layD25ViewWidget.h"
#include "layD25ViewUtils.h"
#include "layLayoutView.h"

#include "dbRecursiveShapeIterator.h"
#include "dbEdgeProcessor.h"
#include "dbPolygonGenerators.h"
#include "dbPolygonTools.h"
#include "dbClip.h"
#include "dbRegion.h"
#include "dbEdges.h"
#include "dbEdgePairs.h"
#include "dbOriginalLayerRegion.h"
#include "dbOriginalLayerEdges.h"
#include "dbOriginalLayerEdgePairs.h"

#include "tlException.h"
#include "tlProgress.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QImage>
#include <QPainter>
#include <QOpenGLTexture>

#include "math.h"

namespace lay
{

// ------------------------------------------------------------------------------

D25InteractionMode::D25InteractionMode (D25ViewWidget *view)
  : mp_view (view)
{
  //  .. nothing yet ..
}

D25InteractionMode::~D25InteractionMode ()
{
  //  .. nothing yet ..
}


// ------------------------------------------------------------------------------

class D25PanInteractionMode
  : public D25InteractionMode
{
public:
  D25PanInteractionMode (D25ViewWidget *widget, const QPoint &pos)
    : D25InteractionMode (widget), m_start_pos (pos)
  {
    m_start_displacement = widget->displacement ();

    double px = (pos.x () - widget->width () / 2) * 2.0 / widget->width ();
    double py = -(pos.y () - widget->height () / 2) * 2.0 / widget->height ();

    //  compute vector of line of sight
    std::pair<QVector3D, QVector3D> ray = camera_normal (view ()->cam_perspective () * view ()->cam_trans (), px, py);

    //  by definition the ray goes through the camera position
    QVector3D hp = widget->hit_point_with_scene (ray.second);

    m_focus_dist = (widget->cam_position () - hp).length ();
  }

  virtual ~D25PanInteractionMode ()
  {
    //  .. nothing yet ..
  }

  virtual void mouse_move (QMouseEvent *event)
  {
    QPoint d = event->pos () - m_start_pos;
    double f = tan ((view ()->cam_fov () / 2) / 180.0 * M_PI) * m_focus_dist * 2.0 / double (view ()->height ());
    double dx = d.x () * f;
    double dy = -d.y () * f;

    QVector3D xv (cos (view ()->cam_azimuth () * M_PI / 180.0), 0.0, sin (view ()->cam_azimuth () * M_PI / 180.0));
    double re = sin (view ()->cam_elevation () * M_PI / 180.0);
    QVector3D yv (-re * xv.z (), cos (view ()->cam_elevation () * M_PI / 180.0), re * xv.x ());
    QVector3D drag = xv * dx + yv * dy;

    view ()->set_displacement (m_start_displacement + drag / view ()->scale_factors ());
  }

private:
  QPoint m_start_pos;
  double m_focus_dist;
  QVector3D m_start_displacement;
};

// ------------------------------------------------------------------------------

class D25Rotate2DInteractionMode
  : public D25InteractionMode
{
public:
  D25Rotate2DInteractionMode (D25ViewWidget *widget, const QPoint &pos)
    : D25InteractionMode (widget), m_start_pos (pos)
  {
    m_start_cam_azimuth = widget->cam_azimuth ();
    m_start_cam_elevation = widget->cam_elevation ();
  }

  virtual ~D25Rotate2DInteractionMode ()
  {
    //  .. nothing yet ..
  }

  virtual void mouse_move (QMouseEvent *event)
  {
    //  fixed focus point for rotation
    double focus_dist = 2.0;

    QPoint d = event->pos () - m_start_pos;
    double f = tan ((view ()->cam_fov () / 2) / 180.0 * M_PI) * focus_dist * 2.0 / double (view ()->height ());
    double dx = d.x () * f;
    double dy = -d.y () * f;

    double da = dx / (view ()->cam_dist () - focus_dist) * 180.0 / M_PI;
    view ()->set_cam_azimuth (m_start_cam_azimuth + da);

    double de = dy / (view ()->cam_dist () - focus_dist) * 180.0 / M_PI;
    view ()->set_cam_elevation (m_start_cam_elevation + de);
  }

private:
  QPoint m_start_pos;
  double m_start_cam_azimuth, m_start_cam_elevation;
};

// ------------------------------------------------------------------------------

class D25RotateAzimuthInteractionMode
  : public D25InteractionMode
{
public:
  D25RotateAzimuthInteractionMode (D25ViewWidget *widget, const QPoint &pos)
    : D25InteractionMode (widget), m_start_pos (pos)
  {
    //  .. nothing yet ..
  }

  virtual ~D25RotateAzimuthInteractionMode ()
  {
    //  .. nothing yet ..
  }

  virtual void mouse_move (QMouseEvent *event)
  {
    //  simple change of azimuth only - with center in the middle

    QPoint m = event->pos () - m_start_pos;
    QVector3D p (m_start_pos.x () - view ()->width () / 2, -m_start_pos.y () + view ()->height () / 2, 0);
    QVector3D d (m.x (), -m.y (), 0);

    double cp = QVector3D::crossProduct (p, p + d).z () / p.length () / (p + d).length ();
    cp = std::max (-1.0, std::min (1.0, cp));
    double da = asin (cp) * 180.0 / M_PI;

    view ()->set_cam_azimuth (view ()->cam_azimuth () + da);
    m_start_pos = event->pos ();
  }

private:
  QPoint m_start_pos;
};

// ------------------------------------------------------------------------------

D25ViewWidget::D25ViewWidget (QWidget *parent)
  : QOpenGLWidget (parent),
    m_shapes_program (0), m_lines_program (0), m_gridplane_program (0)
{
  QSurfaceFormat format;
  format.setDepthBufferSize (24);
  format.setSamples (4); //  more -> widget extends beyond boundary!
  format.setStencilBufferSize (8);
  setFormat (format);

  m_zmin = m_zmax = 0.0;
  m_zset = false;
  m_display_open = false;
  mp_view = 0;
  m_has_error = false;

  reset_viewport ();
}

D25ViewWidget::~D25ViewWidget ()
{
  // Make sure the context is current and then explicitly
  // destroy all underlying OpenGL resources.
  makeCurrent ();

  delete m_shapes_program;
  delete m_lines_program;
  delete m_gridplane_program;

  doneCurrent ();
}

void
D25ViewWidget::reset_viewport ()
{
  m_scale_factor = 1.0;
  m_vscale_factor = 1.0;
  mp_mode.reset (0);

  camera_init ();
}

void
D25ViewWidget::reset ()
{
  reset_viewport ();
  emit scale_factor_changed (m_scale_factor);
  emit vscale_factor_changed (m_vscale_factor);
  refresh ();
}

void
D25ViewWidget::wheelEvent (QWheelEvent *event)
{
  if (event->angleDelta ().y () == 0) {
    return;
  }

#if QT_VERSION >= 0x60000
  double px = (event->position ().x () - width () / 2) * 2.0 / width ();
  double py = -(event->position ().y () - height () / 2) * 2.0 / height ();
#else
  double px = (event->pos ().x () - width () / 2) * 2.0 / width ();
  double py = -(event->pos ().y () - height () / 2) * 2.0 / height ();
#endif

  if (top_view ()) {

    //  Plain zoom

    QVector3D hp (px, py, 0.0);

    double f = exp (event->angleDelta ().y () * (1.0 / (90 * 8)));

    m_scale_factor *= f;
    m_displacement += hp * (1.0 - f) / m_scale_factor;

    emit scale_factor_changed (m_scale_factor);

  } else {

    double d = event->angleDelta ().y () * (1.0 / (90 * 16));

    if (! (event->modifiers () & Qt::ControlModifier)) {

      //  No Ctrl is "move horizontally along the azimuth axis"

      QMatrix4x4 t;
      t.rotate (cam_azimuth (), 0.0, 1.0, 0.0);
      QVector3D cd = t.inverted ().map (QVector3D (0, 0, cam_dist ()));

      m_displacement += d * cd / m_scale_factor;

    } else {

      //  "Ctrl" is zoom

      m_scale_factor *= exp (d);

      emit scale_factor_changed (m_scale_factor);

    }

  }

  refresh ();
}

void
D25ViewWidget::keyPressEvent (QKeyEvent *event)
{
  if (event->key () == Qt::Key_Shift) {

    mp_mode.reset (0);
    set_top_view (true);

  } else if (event->key () == Qt::Key_Up || event->key () == Qt::Key_Down) {

    if (! top_view () && (event->modifiers () & Qt::ControlModifier) != 0) {

      //  Ctrl + up/down changes elevation

      double d = (event->key () == Qt::Key_Up ? 2 : -2);

      set_cam_elevation (std::max (-90.0, std::min (90.0, cam_elevation () + d)));

    } else {

      //  Move "into" or "out"

      double d = (event->key () == Qt::Key_Up ? 0.05 : -0.05);

      QMatrix4x4 t;
      t.rotate (cam_azimuth (), 0.0, 1.0, 0.0);
      QVector3D cd = t.inverted ().map (QVector3D (0, 0, cam_dist ()));

      set_displacement (displacement () + d * cd / m_scale_factor);

    }

  } else if (event->key () == Qt::Key_Left || event->key () == Qt::Key_Right) {

    if (! top_view () && (event->modifiers () & Qt::ControlModifier) != 0) {

      //  Ctrl + left/right changes azimuths

      double d = (event->key () == Qt::Key_Right ? 1 : -1);

      double a = cam_azimuth () + d;
      if (a < -180.0) {
        a += 360.0;
      } else if (a > 180.0) {
        a -= 360.0;
      }

      set_cam_azimuth (a);

    } else {

      //  Move "left" and "right"

      double d = (event->key () == Qt::Key_Left ? 0.1 : -0.1);

      QMatrix4x4 t;
      t.rotate (cam_azimuth (), 0.0, 1.0, 0.0);
      QVector3D cd = t.inverted ().map (QVector3D (cam_dist (), 0, 0));

      set_displacement (displacement () + d * cd / m_scale_factor);

    }

  }
}

void
D25ViewWidget::keyReleaseEvent (QKeyEvent *event)
{
  if (event->key () == Qt::Key_Shift) {
    mp_mode.reset (0);
    set_top_view (false);
  }
}

QVector3D
D25ViewWidget::hit_point_with_scene (const QVector3D &line_dir)
{
  double min_focus_dist = 0.5;

  QVector3D corner = (QVector3D (m_bbox.left (), m_zmin, -(m_bbox.bottom () + m_bbox.height ())) + m_displacement) * scale_factors ();
  QVector3D dim = QVector3D (m_bbox.width (), m_zmax - m_zmin, m_bbox.height ()) * scale_factors ();
  QVector3D line = cam_position ();

  std::pair<bool, QVector3D> hp = lay::hit_point_with_cuboid (line, line_dir, corner, dim);
  if (! hp.first) {
    return line + line_dir * min_focus_dist;
  } else if (QVector3D::dotProduct (line_dir, hp.second - line) < min_focus_dist) {
    //  limit to min focus distance (not behind)
    return line + line_dir * min_focus_dist;
  } else {
    return hp.second;
  }
}

void
D25ViewWidget::mousePressEvent (QMouseEvent *event)
{
  mp_mode.reset (0);

  if (event->button () == Qt::MiddleButton) {
    mp_mode.reset (new D25PanInteractionMode (this, event->pos ()));
  } else if (event->button () == Qt::LeftButton) {
    if (! top_view ()) {
      mp_mode.reset (new D25Rotate2DInteractionMode (this, event->pos ()));
    } else {
      mp_mode.reset (new D25RotateAzimuthInteractionMode (this, event->pos ()));
    }
  }
}

void
D25ViewWidget::mouseReleaseEvent (QMouseEvent * /*event*/)
{
  mp_mode.reset (0);
}

void
D25ViewWidget::mouseMoveEvent (QMouseEvent *event)
{
  if (mp_mode.get ()) {
    mp_mode->mouse_move (event);
  }
}

inline double square (double x) { return x * x; }

void
D25ViewWidget::fit ()
{
  if (m_bbox.empty ()) {

    m_scale_factor = 1.0;
    m_displacement = QVector3D ();

  } else {

    QVector3D dim = QVector3D (m_bbox.width (), (m_zmax - m_zmin) * m_vscale_factor, m_bbox.height ());
    QVector3D bll = QVector3D (m_bbox.left (), m_zmin * m_vscale_factor, -(m_bbox.bottom () + m_bbox.height ()));

    m_scale_factor = 1e6;
    double tfov = tan (cam_fov () / 360.0 * M_PI);
    double tfovh = aspect_ratio () * tfov;

    for (unsigned int i = 0; i < 8; ++i) {

      QVector3D p ((i & 1) == 0 ? -0.5 * dim.x () : 0.5 * dim.x (), bll.y () + ((i & 2) == 0 ? 0.0 : dim.y ()), (i & 4) == 0 ? -0.5 * dim.z () : 0.5 * dim.z ());
      p = cam_trans () * p;

      double d;

      d = std::abs (p.x ()) + tfovh * p.z ();
      if (d > 1e-6) {
        m_scale_factor = std::min (m_scale_factor, cam_dist () * tfovh / d);
      }

      d = std::abs (p.y ()) + tfov * p.z ();
      if (d > 1e-6) {
        m_scale_factor = std::min (m_scale_factor, cam_dist () * tfov / d);
      }

    }

    //  create some margin
    m_scale_factor *= 0.95;

    //  Reset displacement to center the scene
    m_displacement = -(bll + dim * 0.5);
    m_displacement.setY (0.0);

  }

  refresh ();

  emit scale_factor_changed (m_scale_factor);
}

void
D25ViewWidget::refresh ()
{
  update ();
}

void
D25ViewWidget::set_material_visible (size_t index, bool visible)
{
  if (index < m_layers.size () && m_layers [index].visible != visible) {
    m_layers [index].visible = visible;
    update ();
  }
}

void
D25ViewWidget::showEvent (QShowEvent *)
{
  //  NOTE: This should happen automatically, but apparently the OpenGL widget doesn't do an automatic refresh:
  update ();
}

void
D25ViewWidget::camera_changed ()
{
  refresh ();
}

double
D25ViewWidget::aspect_ratio () const
{
  return double (width ()) / double (height ());
}

void
D25ViewWidget::clear ()
{
  m_layers.clear ();
  m_vertex_chunks.clear ();
  m_line_chunks.clear ();

  m_zset = false;
  m_zmin = m_zmax = 0.0;
  m_display_open = false;

  if (! mp_view) {
    m_bbox = db::DBox (-1.0, -1.0, 1.0, 1.0);
  } else {
    m_bbox = mp_view->viewport ().box ();
  }
}

static void color_to_gl (tl::color_t color, GLfloat (&gl_color) [4])
{
  gl_color[0] = ((color >> 16) & 0xff) / 255.0f;
  gl_color[1] = ((color >> 8) & 0xff) / 255.0f;
  gl_color[2] = (color & 0xff) / 255.0f;
  gl_color[3] = 1.0f;
}

static void color_to_gl (const tl::color_t *color, GLfloat (&gl_color) [4])
{
  if (! color) {
    for (unsigned int i = 0; i < 4; ++i) {
      gl_color [i] = 0.0;
    }
  } else {
    color_to_gl (*color, gl_color);
  }
}

static void lp_to_info (const lay::LayerPropertiesNode &lp, D25ViewWidget::LayerInfo &info)
{
  color_to_gl (lp.fill_color (true), info.fill_color);
  if (lp.dither_pattern (true) == 1 /*hollow*/) {
    info.fill_color [3] = 0.0f;
  }

  color_to_gl (lp.frame_color (true), info.frame_color);
  if (lp.frame_color (true) == lp.fill_color (true) && info.fill_color [3] > 0.5) {
    //  optimize: don't draw wire frame unless required
    info.frame_color [3] = 0.0f;
  }

  info.visible = true;
}

void
D25ViewWidget::open_display (const tl::color_t *frame_color, const tl::color_t *fill_color, const db::LayerProperties *like, const std::string *name)
{
  m_vertex_chunks.push_back (triangle_chunks_type ());
  m_normals_chunks.push_back (triangle_chunks_type ());
  m_line_chunks.push_back (line_chunks_type ());

  LayerInfo info;

  info.visible = true;
  color_to_gl (frame_color, info.frame_color);
  color_to_gl (fill_color, info.fill_color);

  info.has_name = (name != 0 || like != 0);
  if (name) {
    info.name = *name;
  } else if (like) {
    info.name = like->to_string ();
  }

  info.vertex_chunk = &m_vertex_chunks.back ();
  info.normals_chunk = &m_normals_chunks.back ();
  info.line_chunk = &m_line_chunks.back ();

  if (like && mp_view) {
    for (lay::LayerPropertiesConstIterator lp = mp_view->begin_layers (); ! lp.at_end (); ++lp) {
      if (! lp->has_children () && lp->source (true).layer_props ().log_equal (*like)) {
        lp_to_info (*lp, info);
        break;
      }
    }
  }

  m_layers.push_back (info);
  m_display_open = true;
}

void
D25ViewWidget::close_display ()
{
  m_display_open = false;
}

void
D25ViewWidget::enter (const db::RecursiveShapeIterator *iter, double zstart, double zstop)
{
  tl_assert (m_display_open);

  if (! m_zset) {
    m_zmin = std::min (zstart, zstop);
    m_zmax = std::max (zstart, zstop);
    m_zset = true;
  } else {
    m_zmin = std::min (m_zmin, std::min (zstart, zstop));
    m_zmax = std::min (m_zmax, std::max (zstart, zstop));
  }

  LayerInfo &info = m_layers.back ();

  //  try to establish a default color from the region's origin if required
  if (mp_view && info.fill_color [3] == 0.0 && info.frame_color [3] == 0.0) {

    if (iter) {

      if (iter && iter->layout () && iter->layout ()->is_valid_layer (iter->layer ())) {

        db::LayerProperties like = iter->layout ()->get_properties (iter->layer ());

        for (lay::LayerPropertiesConstIterator lp = mp_view->begin_layers (); ! lp.at_end (); ++lp) {
          if (! lp->has_children () && lp->source (true).layer_props ().log_equal (like)) {
            lp_to_info (*lp, info);
            if (! info.has_name) {
              info.name = like.to_string ();
              info.has_name = true;
            }
            break;
          }
        }

      }

    } else {

      //  sequential assignment
      tl::color_t color = mp_view->get_palette ().luminous_color_by_index (m_layers.size ());
      color_to_gl (color, info.fill_color);

    }

  }
}

void
D25ViewWidget::entry (const db::Region &data, double dbu, double zstart, double zstop)
{
  const db::OriginalLayerRegion *original = dynamic_cast<const db::OriginalLayerRegion *> (data.delegate ());
  if (original) {
    //  try to establish a default color from the region's origin if required
    auto it = original->begin_iter ();
    enter (&it.first, zstart, zstop);
  } else {
    enter (0, zstart, zstop);
  }

  tl::AbsoluteProgress progress (tl::to_string (tr ("Rendering ...")));
  render_region (progress, *m_layers.back ().vertex_chunk, *m_layers.back ().normals_chunk, *m_layers.back ().line_chunk, data, dbu, db::CplxTrans (dbu).inverted () * m_bbox, zstart, zstop);
}

void
D25ViewWidget::entry (const db::Edges &data, double dbu, double zstart, double zstop)
{
  const db::OriginalLayerEdges *original = dynamic_cast<const  db::OriginalLayerEdges *> (data.delegate ());
  if (original) {
    //  try to establish a default color from the region's origin if required
    auto it = original->begin_iter ();
    enter (&it.first, zstart, zstop);
  } else {
    enter (0, zstart, zstop);
  }

  tl::AbsoluteProgress progress (tl::to_string (tr ("Rendering ...")));
  render_edges (progress, *m_layers.back ().vertex_chunk, *m_layers.back ().normals_chunk, *m_layers.back ().line_chunk, data, dbu, db::CplxTrans (dbu).inverted () * m_bbox, zstart, zstop);
}

void
D25ViewWidget::entry (const db::EdgePairs &data, double dbu, double zstart, double zstop)
{
  const db::OriginalLayerEdgePairs *original = dynamic_cast<const  db::OriginalLayerEdgePairs *> (data.delegate ());
  if (original) {
    //  try to establish a default color from the region's origin if required
    auto it = original->begin_iter ();
    enter (&it.first, zstart, zstop);
  } else {
    enter (0, zstart, zstop);
  }

  tl::AbsoluteProgress progress (tl::to_string (tr ("Rendering ...")));
  render_edge_pairs (progress, *m_layers.back ().vertex_chunk, *m_layers.back ().normals_chunk, *m_layers.back ().line_chunk, data, dbu, db::CplxTrans (dbu).inverted () * m_bbox, zstart, zstop);
}

void
D25ViewWidget::finish ()
{
  //  .. nothing yet ..
}

void
D25ViewWidget::attach_view (LayoutViewBase *view)
{
  mp_view = view;
}

void
D25ViewWidget::render_polygon (D25ViewWidget::triangle_chunks_type &chunks, D25ViewWidget::triangle_chunks_type &normals, D25ViewWidget::line_chunks_type &line_chunks, const db::Polygon &poly, double dbu, double zstart, double zstop)
{
  if (poly.holes () > 0) {

    //  NOTE: line_chunks isn't really used as of now. "render_wall" does the job.

    std::vector<db::Polygon> poly_heap;

    db::EdgeProcessor ep;
    ep.insert_sequence (poly.begin_edge ());
    db::PolygonContainer pc (poly_heap);
    db::PolygonGenerator out (pc, true /*resolve holes*/, true /*min coherence*/);
    db::SimpleMerge op;
    ep.process (out, op);

    for (std::vector<db::Polygon>::const_iterator p = poly_heap.begin (); p != poly_heap.end (); ++p) {
      render_polygon (chunks, normals, line_chunks, *p, dbu, zstart, zstop);
    }

  } else if (poly.hull ().size () > 4) {

    std::vector<db::Polygon> poly_heap;

    db::split_polygon (poly, poly_heap);
    for (std::vector<db::Polygon>::const_iterator p = poly_heap.begin (); p != poly_heap.end (); ++p) {
      render_polygon (chunks, normals, line_chunks, *p, dbu, zstart, zstop);
    }

  } else if (poly.hull ().size () >= 3) {

    db::Point pts [4];
    std::copy (poly.hull ().begin (), poly.hull ().end (), &pts [0]);

    //  triangle bottom
    chunks.add (pts[0].x () * dbu, zstart, pts[0].y () * dbu);
    chunks.add (pts[2].x () * dbu, zstart, pts[2].y () * dbu);
    chunks.add (pts[1].x () * dbu, zstart, pts[1].y () * dbu);

    //  normals
    for (unsigned int i = 0; i < 3; ++i) {
      normals.add (0.0, 1.0, 0.0);
    }

    //  triangle top
    chunks.add (pts[0].x () * dbu, zstop, pts[0].y () * dbu);
    chunks.add (pts[1].x () * dbu, zstop, pts[1].y () * dbu);
    chunks.add (pts[2].x () * dbu, zstop, pts[2].y () * dbu);

    //  normals
    for (unsigned int i = 0; i < 3; ++i) {
      normals.add (0.0, -1.0, 0.0);
    }

    if (poly.hull ().size () == 4) {

      //  triangle bottom
      chunks.add (pts[0].x () * dbu, zstart, pts[0].y () * dbu);
      chunks.add (pts[3].x () * dbu, zstart, pts[3].y () * dbu);
      chunks.add (pts[2].x () * dbu, zstart, pts[2].y () * dbu);

      //  normals
      for (unsigned int i = 0; i < 3; ++i) {
        normals.add (0.0, 1.0, 0.0);
      }

      //  triangle top
      chunks.add (pts[0].x () * dbu, zstop, pts[0].y () * dbu);
      chunks.add (pts[2].x () * dbu, zstop, pts[2].y () * dbu);
      chunks.add (pts[3].x () * dbu, zstop, pts[3].y () * dbu);

      //  normals
      for (unsigned int i = 0; i < 3; ++i) {
        normals.add (0.0, -1.0, 0.0);
      }

    }

  }
}

void
D25ViewWidget::render_wall (D25ViewWidget::triangle_chunks_type &chunks, D25ViewWidget::triangle_chunks_type &normals, D25ViewWidget::line_chunks_type &line_chunks, const db::Edge &edge, double dbu, double zstart, double zstop)
{
  if (edge.is_degenerate ()) {
    return;
  }

  //  TODO: can we avoid some duplication through indexing?

  chunks.add (edge.p1 ().x () * dbu, zstart, edge.p1 ().y () * dbu);
  chunks.add (edge.p2 ().x () * dbu, zstop, edge.p2 ().y () * dbu);
  chunks.add (edge.p1 ().x () * dbu, zstop, edge.p1 ().y () * dbu);
  chunks.add (edge.p1 ().x () * dbu, zstart, edge.p1 ().y () * dbu);
  chunks.add (edge.p2 ().x () * dbu, zstart, edge.p2 ().y () * dbu);
  chunks.add (edge.p2 ().x () * dbu, zstop, edge.p2 ().y () * dbu);

  db::DVector n (dbu * edge.dy (), -dbu * edge.dx ());
  n *= 1.0 / n.double_length ();
  for (unsigned int i = 0; i < 6; ++i) {
    normals.add (n.x (), 0.0, n.y ());
  }

  line_chunks.add (edge.p1 ().x () * dbu, zstart, edge.p1 ().y () * dbu);
  line_chunks.add (edge.p2 ().x () * dbu, zstart, edge.p2 ().y () * dbu);
  line_chunks.add (edge.p2 ().x () * dbu, zstart, edge.p2 ().y () * dbu);
  line_chunks.add (edge.p2 ().x () * dbu, zstop, edge.p2 ().y () * dbu);
  line_chunks.add (edge.p2 ().x () * dbu, zstop, edge.p2 ().y () * dbu);
  line_chunks.add (edge.p1 ().x () * dbu, zstop, edge.p1 ().y () * dbu);
}

void
D25ViewWidget::render_region (tl::AbsoluteProgress &progress, D25ViewWidget::triangle_chunks_type &chunks, D25ViewWidget::triangle_chunks_type &normals, D25ViewWidget::line_chunks_type &line_chunks, const db::Region &region, double dbu, const db::Box &clip_box, double zstart, double zstop)
{
  std::vector<db::Polygon> poly_heap;

  for (db::Region::const_iterator p = region.begin (); !p.at_end (); ++p) {

    poly_heap.clear ();
    db::clip_poly (*p, clip_box, poly_heap, false /*keep holes*/);

    for (std::vector<db::Polygon>::const_iterator p = poly_heap.begin (); p != poly_heap.end (); ++p) {

      ++progress;

      render_polygon (chunks, normals, line_chunks, *p, dbu, zstart, zstop);

      for (db::Polygon::polygon_edge_iterator e = p->begin_edge (); ! e.at_end (); ++e) {
        render_wall (chunks, normals, line_chunks, *e, dbu, zstart, zstop);
      }

    }

  }
}

void
D25ViewWidget::render_edges (tl::AbsoluteProgress &progress, D25ViewWidget::triangle_chunks_type &normals, D25ViewWidget::triangle_chunks_type &chunks, D25ViewWidget::line_chunks_type &line_chunks, const db::Edges &edges, double dbu, const db::Box &clip_box, double zstart, double zstop)
{
  for (db::Edges::const_iterator e = edges.begin (); !e.at_end (); ++e) {

    ++progress;

    std::pair<bool, db::Edge> ec = e->clipped (clip_box);
    if (ec.first) {
      render_wall (chunks, normals, line_chunks, ec.second, dbu, zstart, zstop);
    }

  }
}

void
D25ViewWidget::render_edge_pairs (tl::AbsoluteProgress &progress, D25ViewWidget::triangle_chunks_type &normals, D25ViewWidget::triangle_chunks_type &chunks, D25ViewWidget::line_chunks_type &line_chunks, const db::EdgePairs &edge_pairs, double dbu, const db::Box &clip_box, double zstart, double zstop)
{
  for (db::EdgePairs::const_iterator e = edge_pairs.begin (); !e.at_end (); ++e) {

    ++progress;

    std::pair<bool, db::Edge> ec;

    ec = e->first ().clipped (clip_box);
    if (ec.first) {
      render_wall (chunks, normals, line_chunks, ec.second, dbu, zstart, zstop);
    }

    ec = e->second ().clipped (clip_box);
    if (ec.first) {
      render_wall (chunks, normals, line_chunks, ec.second, dbu, zstart, zstop);
    }

  }
}

static std::pair<double, double> find_grid (double v)
{
  for (int p = -12; p < 12; ++p) {
    double g10 = pow (10, double (p));
    if (v > 100 * g10) {
      continue;
    } else if (v < 10 * g10) {
      return std::make_pair (g10, g10);
    } else if (v < 20 * g10) {
      return std::make_pair (g10, g10 * 0.1);
    } else if (v < 50 * g10) {
      return std::make_pair (2.0 * g10, g10);
    } else {
      return std::make_pair (5.0 * g10, g10);
    }
  }

  return std::make_pair (v, v);
}

void
D25ViewWidget::initializeGL ()
{
  tl_assert (m_shapes_program == 0);
  tl_assert (m_gridplane_program == 0);
  tl_assert (m_lines_program == 0);

  m_has_error = false;

  try {
    do_initialize_gl ();
  } catch (tl::Exception &ex) {
    m_error = ex.msg ();
    m_has_error = true;
  } catch (std::exception &ex) {
    m_error = ex.what ();
    m_has_error = true;
  } catch (...) {
    m_error = "(unspecific error)";
    m_has_error = true;
  }

  if (m_has_error) {

    delete m_shapes_program;
    m_shapes_program = 0;
    delete m_lines_program;
    m_lines_program = 0;
    delete m_gridplane_program;
    m_gridplane_program = 0;

    emit init_failed ();

  }
}

void
D25ViewWidget::do_initialize_gl ()
{
  QOpenGLFunctions::initializeOpenGLFunctions();

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  static const char *shapes_vertex_shader_source =
      "#version 120\n"
      "\n"
      "attribute vec4 posAttr;\n"
      "attribute vec4 normalsAttr;\n"
      "uniform mat4 matrix;\n"
      "uniform vec3 illum;\n"
      "varying float dp;\n"
      "\n"
      "void main() {\n"
      "   gl_Position = matrix * posAttr;\n"
      "   dp = dot(normalsAttr.xyz, illum);\n"
      "}\n";

  static const char *shapes_fragment_shader_source =
      "#version 120\n"
      "\n"
      "varying float dp;\n"
      "uniform highp float mist_factor;\n"
      "uniform highp float mist_add;\n"
      "uniform vec4 color;\n"
      "uniform vec4 ambient;\n"
      "\n"
      "lowp vec4 color_by_z(lowp vec4 c, highp float z) {\n"
      "  highp float mist_rgb = c.g * mist_factor + mist_add;\n"
      "  lowp vec4 mist_color = vec4(mist_rgb, mist_rgb, mist_rgb, 1.0);\n"
      "  highp float d = 0.12;\n" //  d + dd/2 = 0.15 = 1/?
      "  highp float dd = 0.06;\n"
      "  highp float f = 1.0;\n"
      "  if (z < d - dd) {\n"
      "    f = 0.0;\n"
      "  } else if (z < d + dd) {\n"
      "    f = (z - (d - dd)) / (2.0 * dd);\n"
      "  }\n"
      "  return (1.0 - f) * mist_color + f * c;\n"
      "}\n"
      "\n"
      "void main() {\n"
      "   lowp vec4 vertex_color = color * (dp * 0.5 + 0.5) - (min(0.0, dp) * ambient);\n"
      "   vertex_color.a = 1.0;\n"
      "   gl_FragColor = color_by_z(vertex_color, gl_FragCoord.w);\n"
      "}\n";

  m_shapes_program = new QOpenGLShaderProgram (this);
  if (! m_shapes_program->addShaderFromSourceCode (QOpenGLShader::Vertex, shapes_vertex_shader_source)) {
    throw tl::Exception (std::string ("Shapes vertex shader compilation failed:\n") + tl::to_string (m_shapes_program->log ()));
  }
  if (! m_shapes_program->addShaderFromSourceCode (QOpenGLShader::Fragment, shapes_fragment_shader_source)) {
    throw tl::Exception (std::string ("Shapes fragment shader compilation failed:\n") + tl::to_string (m_shapes_program->log ()));
  }
  if (! m_shapes_program->link ()) {
    throw tl::Exception (std::string ("Shapes shader program linking failed failed:\n") + tl::to_string (m_shapes_program->log ()));
  }

  static const char *lines_vertex_shader_source =
      "#version 120\n"
      "\n"
      "attribute vec4 posAttr;\n"
      "uniform mat4 matrix;\n"
      "\n"
      "void main() {\n"
      "   gl_Position = matrix * posAttr;\n"
      "}\n";

  static const char *lines_fragment_shader_source =
      "#version 120\n"
      "\n"
      "uniform lowp vec4 color;\n"
      "uniform highp float mist_factor;\n"
      "uniform highp float mist_add;\n"
      "\n"
      "lowp vec4 color_by_z(lowp vec4 c, highp float z) {\n"
      "  highp float mist_rgb = c.g * mist_factor + mist_add;\n"
      "  lowp vec4 mist_color = vec4(mist_rgb, mist_rgb, mist_rgb, 1.0);\n"
      "  highp float d = 0.12;\n" //  d + dd/2 = 0.15 = 1/?
      "  highp float dd = 0.06;\n"
      "  highp float f = 1.0;\n"
      "  if (z < d - dd) {\n"
      "    f = 0.0;\n"
      "  } else if (z < d + dd) {\n"
      "    f = (z - (d - dd)) / (2.0 * dd);\n"
      "  }\n"
      "  return (1.0 - f) * mist_color + f * c;\n"
      "}\n"
      "\n"
      "void main() {\n"
      "   gl_FragColor = color_by_z(color, gl_FragCoord.w);\n"
      "}\n";

  m_lines_program = new QOpenGLShaderProgram (this);
  if (! m_lines_program->addShaderFromSourceCode (QOpenGLShader::Vertex, lines_vertex_shader_source)) {
    throw tl::Exception (std::string ("Lines vertex shader compilation failed:\n") + tl::to_string (m_lines_program->log ()));
  }
  if (! m_lines_program->addShaderFromSourceCode (QOpenGLShader::Fragment, lines_fragment_shader_source)) {
    throw tl::Exception (std::string ("Lines fragment shader compilation failed:\n") + tl::to_string (m_lines_program->log ()));
  }
  if (! m_lines_program->link ()) {
    throw tl::Exception (std::string ("Lines shader program linking failed failed:\n") + tl::to_string (m_lines_program->log ()));
  }

  //  grid plane shader source

  static const char *gridplan_vertex_shader_source =
      "#version 120\n"
      "\n"
      "attribute vec4 posAttr;\n"
      "uniform mat4 matrix;\n"
      "\n"
      "void main() {\n"
      "   gl_Position = matrix * posAttr;\n"
      "}\n";

  static const char *gridplan_fragment_shader_source =
      "#version 120\n"
      "\n"
      "uniform lowp vec4 color;\n"
      "void main() {\n"
      "   gl_FragColor = color;\n"
      "}\n";

  m_gridplane_program = new QOpenGLShaderProgram (this);
  if (! m_gridplane_program->addShaderFromSourceCode (QOpenGLShader::Vertex, gridplan_vertex_shader_source)) {
    throw tl::Exception (std::string ("Grid plane vertex shader compilation failed:\n") + tl::to_string (m_gridplane_program->log ()));
  }
  if (! m_gridplane_program->addShaderFromSourceCode (QOpenGLShader::Fragment, gridplan_fragment_shader_source)) {
    throw tl::Exception (std::string ("Grid plane fragment shader compilation failed:\n") + tl::to_string (m_gridplane_program->log ()));
  }
  if (! m_gridplane_program->link ()) {
    throw tl::Exception (std::string ("Grid plane shader program linking failed:\n") + tl::to_string (m_gridplane_program->log ()));
  }
}

void
D25ViewWidget::paintGL ()
{
  if (! mp_view) {
    return;
  }

  const qreal retina_scale = devicePixelRatio ();
  glViewport (0, 0, width () * retina_scale, height () * retina_scale);

  tl::Color c = mp_view->background_color ();
  float foreground_rgb = (c.to_mono () ? 0.0f : 1.0f);
  float ambient = (c.to_mono () ? 0.8f : 0.25f);
  float mist_factor = (c.to_mono () ? 0.2f : 0.4f);
  float mist_add = (c.to_mono () ? 0.8f : 0.2f);
  glClearColor (float (c.red ()) / 255.0f, float (c.green ()) / 255.0f, float (c.blue ()) / 255.0f, 1.0);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (! m_shapes_program || ! m_lines_program || ! m_gridplane_program) {
    return;
  }

  int positions = m_shapes_program->attributeLocation ("posAttr");
  int normals = m_shapes_program->attributeLocation ("normalsAttr");

  QMatrix4x4 scene_trans, scene_trans_wo_y;

  //  provide the displacement and scaling (in this order!)
  scene_trans.scale (m_scale_factor, m_scale_factor * m_vscale_factor, m_scale_factor);
  scene_trans.translate (m_displacement);
  //  this way we can use y as z coordinate when drawing
  scene_trans.scale (1.0, 1.0, -1.0);

  scene_trans_wo_y = scene_trans;
  scene_trans_wo_y.translate (QVector3D (0.0, -m_displacement.y (), 0.0));

  m_shapes_program->bind ();

  //  draw the actual layout

  m_shapes_program->setUniformValue ("matrix", cam_perspective () * cam_trans () * scene_trans);

  //  NOTE: z axis of illum points towards the scene because we include the z inversion in the scene transformation matrix
  m_shapes_program->setUniformValue ("illum", QVector3D (-3.0, -4.0, 2.0).normalized ());

  m_shapes_program->setUniformValue ("ambient", QVector4D (ambient, ambient, ambient, 1.0));
  m_shapes_program->setUniformValue ("mist_factor", mist_factor);
  m_shapes_program->setUniformValue ("mist_add", mist_add);

  glEnable (GL_DEPTH_TEST);
  glEnableVertexAttribArray (positions);
  glEnableVertexAttribArray (normals);

  for (std::vector<LayerInfo>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {

    if (l->visible && l->fill_color [3] > 0.5) {

      m_shapes_program->setUniformValue ("color", l->fill_color [0], l->fill_color [1], l->fill_color [2], l->fill_color [3]);

      triangle_chunks_type::iterator v = l->vertex_chunk->begin ();
      triangle_chunks_type::iterator n = l->normals_chunk->begin ();

      while (v != l->vertex_chunk->end () && n != l->vertex_chunk->end ()) {
        tl_assert (v->size () == n->size ());
        glVertexAttribPointer (positions, 3, gl_type2enum<GLfloat> () (), GL_FALSE, 0, v->front ());
        glVertexAttribPointer (normals, 3, gl_type2enum<GLfloat> () (), GL_FALSE, 0, n->front ());
        //  TODO: use glDrawElements to draw indexed buffers for better memory usage
        glDrawArrays (GL_TRIANGLES, 0, GLsizei (v->size () / 3));
        ++v;
        ++n;
      }

    }
  }

  glDisableVertexAttribArray (positions);
  glDisableVertexAttribArray (normals);

  m_shapes_program->release ();


  //  wire lines

  m_lines_program->bind ();

  m_lines_program->setUniformValue ("matrix", cam_perspective () * cam_trans () * scene_trans);

  //  NOTE: z axis of illum points towards the scene because we include the z inversion in the scene transformation matrix
  m_lines_program->setUniformValue ("illum", QVector3D (-3.0, -4.0, 2.0).normalized ());

  m_lines_program->setUniformValue ("ambient", QVector4D (ambient, ambient, ambient, 1.0));
  m_lines_program->setUniformValue ("mist_factor", mist_factor);
  m_lines_program->setUniformValue ("mist_add", mist_add);

  glEnable (GL_DEPTH_TEST);
  glEnableVertexAttribArray (positions);
  glLineWidth (1.0);

  for (std::vector<LayerInfo>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
    if (l->visible && l->frame_color [3] > 0.5) {
      m_lines_program->setUniformValue ("color", l->frame_color [0], l->frame_color [1], l->frame_color [2], l->frame_color [3]);
      //  TODO: use glDrawElements to draw indexed buffers for better memory usage
      l->line_chunk->draw_to (this, positions, GL_LINES);
    }
  }

  glDisableVertexAttribArray (positions);

  m_lines_program->release ();


  //  decoration

  positions = m_gridplane_program->attributeLocation ("posAttr");

  //  a vertex buffer for the decoration
  lay::mem_chunks<float, 1024 * 18> vertexes;

  m_gridplane_program->bind ();

  glEnable (GL_DEPTH_TEST);
  glEnableVertexAttribArray (positions);

  //  draw pivot compass

  m_gridplane_program->setUniformValue ("matrix", cam_perspective () * cam_trans ());

  double compass_rad = 0.3;
  double compass_bars = 0.4;

  vertexes.add (-compass_bars, 0.0, 0.0);
  vertexes.add (compass_bars, 0.0, 0.0);

  vertexes.add (0.0, 0.0, -compass_bars);
  vertexes.add (0.0, 0.0, compass_bars);

  int ncircle = 64;
  double x = compass_rad, z = 0.0;
  double da = 1.0 / double (ncircle) * M_PI * 2.0;

  for (int i = 0; i < ncircle; ++i) {

    double a = double (i + 1) * da;
    double xx = compass_rad * cos (a);
    double zz = compass_rad * sin (a);

    vertexes.add (x, 0.0, z);
    vertexes.add (xx, 0.0, zz);

    x = xx;
    z = zz;

  }

  m_gridplane_program->setUniformValue ("color", foreground_rgb, foreground_rgb, foreground_rgb, 0.25f);

  glLineWidth (2.0);
  vertexes.draw_to (this, positions, GL_LINES);

  //  arrow

  vertexes.clear ();

  vertexes.add (-0.25 * compass_rad, 0.0, 0.6 * compass_rad);
  vertexes.add (0.0, 0.0, -0.8 * compass_rad);
  vertexes.add (0.0, 0.0, -0.8 * compass_rad);
  vertexes.add (0.25 * compass_rad, 0.0, 0.6 * compass_rad);
  vertexes.add (0.25 * compass_rad, 0.0, 0.6 * compass_rad);
  vertexes.add (-0.25 * compass_rad, 0.0, 0.6 * compass_rad);

  vertexes.draw_to (this, positions, GL_LINES);

  //  draw base plane

  m_gridplane_program->setUniformValue ("matrix", cam_perspective () * cam_trans () * scene_trans_wo_y);

  std::pair<double, double> gg = find_grid (std::max (m_bbox.width (), m_bbox.height ()));
  double gminor = gg.second, gmajor = gg.first;

  double margin = std::max (m_bbox.width (), m_bbox.height ()) * 0.02;

  double l = m_bbox.left ();
  double r = m_bbox.right ();
  double b = m_bbox.bottom ();
  double t = m_bbox.top ();

  //  major and minor grid lines

  vertexes.clear ();

  const double epsilon = 1e-6;

  for (int major = 0; major < 2; ++major) {

    m_gridplane_program->setUniformValue ("color", foreground_rgb, foreground_rgb, foreground_rgb, major ? 0.25f : 0.15f);

    double x, y;
    double step = (major ? gmajor : gminor);

    x = ceil (l / step) * step;
    for ( ; x < r - step * epsilon; x += step) {
      if ((fabs (floor (x / gmajor + 0.5) * gmajor - x) < epsilon) == (major != 0)) {
        vertexes.add (x, 0.0, b - margin);
        vertexes.add (x, 0.0, t + margin);
      }
    }

    y = ceil (b / step) * step;
    for ( ; y < t - step * epsilon; y += step) {
      if ((fabs (floor (y / gmajor + 0.5) * gmajor - y) < epsilon) == (major != 0)) {
        vertexes.add (l - margin, 0.0, y);
        vertexes.add (r + margin, 0.0, y);
      }
    }

    glLineWidth (2.0);
    vertexes.draw_to (this, positions, GL_LINES);

  }

  //  the plane itself

  vertexes.clear ();

  vertexes.add (l, 0.0, b);
  vertexes.add (l, 0.0, t);
  vertexes.add (r, 0.0, t);

  vertexes.add (l, 0.0, b);
  vertexes.add (r, 0.0, b);
  vertexes.add (r, 0.0, t);

  m_gridplane_program->setUniformValue ("color", foreground_rgb, foreground_rgb, foreground_rgb, 0.1f);

  vertexes.draw_to (this, positions, GL_TRIANGLES);

  //  the orientation cube

  if (! top_view ()) {

    glDisable (GL_DEPTH_TEST);

    int cube_size = 32;
    int cube_margin = 40;

    QMatrix4x4 into_top_right_corner;
    into_top_right_corner.translate (1.0 - 2.0 / width () * (cube_margin + cube_size / 2), 1.0 - 2.0 / height () * (cube_margin + cube_size / 2));
    // into_top_right_corner.translate (0.5, 0.5, 0.0);
    into_top_right_corner.scale (2.0 * cube_size / double (height ()), 2.0 * cube_size / double (height ()), 1.0);

    m_gridplane_program->setUniformValue ("matrix", into_top_right_corner * cam_perspective () * cam_trans ());

    vertexes.clear ();

    vertexes.add (-1.0, -1.0, 1.0);
    vertexes.add (-1.0, -1.0, -1.0);

    vertexes.add (-1.0, 1.0, 1.0);
    vertexes.add (-1.0, 1.0, -1.0);

    vertexes.add (1.0, -1.0, 1.0);
    vertexes.add (1.0, -1.0, -1.0);

    vertexes.add (1.0, 1.0, 1.0);
    vertexes.add (1.0, 1.0, -1.0);

    vertexes.add (-1.0, -1.0, 1.0);
    vertexes.add (-1.0, 1.0, 1.0);

    vertexes.add (1.0, -1.0, 1.0);
    vertexes.add (1.0, 1.0, 1.0);

    vertexes.add (-1.0, -1.0, 1.0);
    vertexes.add (1.0, -1.0, 1.0);

    vertexes.add (-1.0, 1.0, 1.0);
    vertexes.add (1.0, 1.0, 1.0);

    vertexes.add (-1.0, -1.0, -1.0);
    vertexes.add (-1.0, 1.0, -1.0);

    vertexes.add (1.0, -1.0, -1.0);
    vertexes.add (1.0, 1.0, -1.0);

    vertexes.add (-1.0, -1.0, -1.0);
    vertexes.add (1.0, -1.0, -1.0);

    vertexes.add (-1.0, 1.0, -1.0);
    vertexes.add (1.0, 1.0, -1.0);

    m_gridplane_program->setUniformValue ("color", foreground_rgb, foreground_rgb, foreground_rgb, 0.2f);

    vertexes.draw_to (this, positions, GL_LINES);

    vertexes.clear ();

    //  A "K" at the front
    vertexes.add (-0.8f, -0.8f, 1.0f);
    vertexes.add (-0.8f, 0.8f, 1.0f);
    vertexes.add (-0.2f, 0.8f, 1.0f);

    vertexes.add (-0.8f, -0.8f, 1.0f);
    vertexes.add (-0.2f, -0.8f, 1.0f);
    vertexes.add (-0.2f, 0.8f, 1.0f);

    vertexes.add (0.2f, 0.8f, 1.0f);
    vertexes.add (0.8f, 0.8f, 1.0f);
    vertexes.add (0.8f, 0.6f, 1.0f);

    vertexes.add (0.2f, 0.8f, 1.0f);
    vertexes.add (0.8f, 0.6f, 1.0f);
    vertexes.add (0.6f, 0.4f, 1.0f);

    vertexes.add (-0.2f, 0.4f, 1.0f);
    vertexes.add (0.2f, 0.8f, 1.0f);
    vertexes.add (0.6f, 0.4f, 1.0f);

    vertexes.add (-0.2f, 0.4f, 1.0f);
    vertexes.add (0.6f, 0.4f, 1.0f);
    vertexes.add (0.2f, 0.0f, 1.0f);

    vertexes.add (-0.2f, 0.4f, 1.0f);
    vertexes.add (0.2f, 0.0f, 1.0f);
    vertexes.add (-0.2f, -0.4f, 1.0f);

    vertexes.add (-0.2f, -0.4f, 1.0f);
    vertexes.add (0.6f, -0.4f, 1.0f);
    vertexes.add (0.2f, -0.0f, 1.0f);

    vertexes.add (-0.2f, -0.4f, 1.0f);
    vertexes.add (0.2f, -0.8f, 1.0f);
    vertexes.add (0.6f, -0.4f, 1.0f);

    vertexes.add (0.2f, -0.8f, 1.0f);
    vertexes.add (0.8f, -0.6f, 1.0f);
    vertexes.add (0.6f, -0.4f, 1.0f);

    vertexes.add (0.2f, -0.8f, 1.0f);
    vertexes.add (0.8f, -0.8f, 1.0f);
    vertexes.add (0.8f, -0.6f, 1.0f);

    m_gridplane_program->setUniformValue ("color", foreground_rgb, foreground_rgb, foreground_rgb, 0.3f);

    vertexes.draw_to (this, positions, GL_TRIANGLES);

  }

  glDisableVertexAttribArray (positions);

  m_shapes_program->release ();
}

void
D25ViewWidget::resizeGL (int /*w*/, int /*h*/)
{
  refresh ();
}

}


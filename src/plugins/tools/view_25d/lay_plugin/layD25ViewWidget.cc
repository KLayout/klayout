
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


#include "layD25ViewWidget.h"
#include "layD25ViewUtils.h"
#include "layLayoutView.h"

#include "dbRecursiveShapeIterator.h"
#include "dbEdgeProcessor.h"
#include "dbPolygonGenerators.h"
#include "dbPolygonTools.h"

#include "tlException.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>

#include "math.h"

namespace lay
{

// ------------------------------------------------------------------------------

D25ViewWidget::D25ViewWidget (QWidget *parent)
  : QOpenGLWidget (parent),
    m_shapes_program (0)
{
  QSurfaceFormat format;
  format.setDepthBufferSize (24);
  format.setSamples (4); //  more -> widget extends beyond boundary!
  format.setStencilBufferSize (8);
  // @@@? format.setVersion (3, 2);
  format.setProfile (QSurfaceFormat::CoreProfile);
  setFormat (format);

  m_zmin = m_zmax = 0.0;
  mp_view = 0;
}

D25ViewWidget::~D25ViewWidget ()
{
  // Make sure the context is current and then explicitly
  // destroy all underlying OpenGL resources.
  makeCurrent ();

  delete m_shapes_program;

  doneCurrent ();
}

void
D25ViewWidget::reset ()
{
  m_scale_factor = 1.0;
  m_focus_dist = 0.0;
  m_fov = 90.0;
  m_cam_azimuth = m_cam_elevation = 0.0;
  m_top_view = false;
  m_dragging = m_rotating = false;

  refresh ();
}

void
D25ViewWidget::wheelEvent (QWheelEvent *event)
{
  if (event->angleDelta ().y () == 0) {
    return;
  }

  double px = (event->pos ().x () - width () / 2) * 2.0 / width ();
  double py = -(event->pos ().y () - height () / 2) * 2.0 / height ();

  //  compute vector of line of sight
  std::pair<QVector3D, QVector3D> ray = camera_normal (cam_perspective () * cam_trans (), px, py);

  //  by definition the ray goes through the camera position
  QVector3D hp = hit_point_with_scene (ray.second);

  if (event->modifiers () & Qt::ControlModifier) {

    //  "Ctrl" is closeup

    double f = event->angleDelta ().y () * (1.0 / (90 * 8));
    m_displacement += -((f / m_scale_factor) * std::min (cam_dist (), double ((cam_position () - hp).length ()))) * ray.second;

  } else {

    //  No shift is zoom

    double f = exp (event->angleDelta ().y () * (1.0 / (90 * 8)));

    QVector3D initial_displacement = m_displacement;
    QVector3D displacement = m_displacement;

    m_scale_factor *= f;
    displacement += hp * (1.0 - f) / m_scale_factor;

    //  normalize the scene translation so the scene does not "flee"

    QMatrix4x4 ct = cam_trans ();
    initial_displacement = ct.map (initial_displacement);
    displacement = ct.map (displacement);

    lay::normalize_scene_trans (cam_perspective (), displacement, m_scale_factor, initial_displacement.z ());

    m_displacement = ct.inverted ().map (displacement);

  }

  refresh ();
}

void
D25ViewWidget::keyPressEvent (QKeyEvent *event)
{
  if (event->key () == Qt::Key_Shift) {
    m_top_view = true;
    m_dragging = false;
    m_rotating = false;
    refresh ();
  }
}

void
D25ViewWidget::keyReleaseEvent (QKeyEvent *event)
{
  if (event->key () == Qt::Key_Shift) {
    m_top_view = false;
    m_dragging = false;
    m_rotating = false;
    refresh ();
  }
}

QVector3D
D25ViewWidget::hit_point_with_scene (const QVector3D &line_dir)
{
  double min_focus_dist = 0.5;

  QVector3D corner = (QVector3D (m_bbox.left (), m_zmin, -(m_bbox.bottom () + m_bbox.height ())) + m_displacement) * m_scale_factor;
  QVector3D dim = QVector3D (m_bbox.width (), m_zmax - m_zmin, m_bbox.height ()) * m_scale_factor;
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
  m_dragging = m_rotating = false;
  if (event->button () == Qt::MidButton) {
    m_dragging = true;
  } else if (event->button () == Qt::LeftButton) {
    m_rotating = true;
  }

  m_start_pos = event->pos ();
  m_start_cam_position = cam_position ();
  m_start_cam_azimuth = cam_azimuth ();
  m_start_cam_elevation = cam_elevation ();
  m_start_displacement = m_displacement;

  m_focus_dist = 2.0;
  m_hit_point = QVector3D ();

  if (m_dragging) {

    //  by definition the ray goes through the camera position
    QVector3D hp = hit_point_with_scene (cam_direction ());

    m_focus_dist = (cam_position () - hp).length ();
    m_hit_point = cam_position () + cam_direction () * m_focus_dist;

  } else if (m_rotating) {

    double px = (event->pos ().x () - width () / 2) * 2.0 / width ();
    double py = -(event->pos ().y () - height () / 2) * 2.0 / height ();

    //  compute vector of line of sight
    std::pair<QVector3D, QVector3D> ray = camera_normal (cam_perspective () * cam_trans (), px, py);

    //  by definition the ray goes through the camera position
    QVector3D hp = hit_point_with_scene (ray.second);

    m_focus_dist = std::max (m_focus_dist, double ((cam_position () - hp).length ()));
    m_hit_point = cam_position () + ray.second * m_focus_dist;

  }
}

void
D25ViewWidget::mouseReleaseEvent (QMouseEvent * /*event*/)
{
  m_dragging = false;
  m_rotating = false;
}

void
D25ViewWidget::mouseMoveEvent (QMouseEvent *event)
{
  if (! m_dragging && ! m_rotating) {
    return;
  }

  if (m_dragging) {

    QPoint d = event->pos () - m_start_pos;
    double f = tan ((cam_fov () / 2) / 180.0 * M_PI) * m_focus_dist * 2.0 / double (height ());
    double dx = d.x () * f;
    double dy = -d.y () * f;

    QVector3D xv (cos (m_start_cam_azimuth * M_PI / 180.0), 0.0, sin (m_start_cam_azimuth * M_PI / 180.0));
    double re = sin (m_start_cam_elevation * M_PI / 180.0);
    QVector3D yv (-re * xv.z (), cos (m_start_cam_elevation * M_PI / 180.0), re * xv.x ());
    QVector3D drag = xv * dx + yv * dy;

    m_displacement = m_start_displacement + drag / m_scale_factor;

  } else {

    if (! m_top_view) {

      //  fixed focus point for rotation
      double focus_dist = 2.0;

      QPoint d = event->pos () - m_start_pos;
      double f = tan ((cam_fov () / 2) / 180.0 * M_PI) * focus_dist * 2.0 / double (height ());
      double dx = d.x () * f;
      double dy = -d.y () * f;

      double da = dx / (cam_dist () - focus_dist) * 180.0 / M_PI;
      m_cam_azimuth = m_start_cam_azimuth + da;

      double de = dy / (cam_dist () - focus_dist) * 180.0 / M_PI;
      m_cam_elevation = m_start_cam_elevation + de;

    } else {

      //  simple change of azimuth only - with center in the middle

      QPoint m = event->pos () - m_start_pos;
      QVector3D p (m_start_pos.x () - width () / 2, -m_start_pos.y () + height () / 2, 0);
      QVector3D d (m.x (), -m.y (), 0);

      double cp = QVector3D::crossProduct (p, p + d).z () / p.length () / (p + d).length ();
      cp = std::max (-1.0, std::min (1.0, cp));
      double da = asin (cp) * 180.0 / M_PI;

      m_cam_azimuth += da;
      m_start_pos = event->pos ();

    }

  }

  refresh ();
}

double
D25ViewWidget::cam_fov () const
{
  return m_fov; // @@@
}

double
D25ViewWidget::cam_dist () const
{
  return 4.0; // @@@
}

QVector3D
D25ViewWidget::cam_direction () const
{
  QVector3D cd = cam_trans ().map (QVector3D (0, 0, 1));
  cd.setZ (-cd.z ());
  return cd;
}

QVector3D
D25ViewWidget::cam_position () const
{
  return cam_direction () * -cam_dist ();
}

double
D25ViewWidget::cam_azimuth () const
{
  return m_cam_azimuth;
}

double
D25ViewWidget::cam_elevation () const
{
  return m_top_view ? -90.0 : m_cam_elevation;
}

QMatrix4x4
D25ViewWidget::cam_perspective () const
{
  QMatrix4x4 t;
  t.perspective (cam_fov (), float (width ()) / float (height ()), 0.1f, 10000.0f);
  t.translate (QVector3D (0.0, 0.0, -cam_dist ()));
  return t;
}

QMatrix4x4
D25ViewWidget::cam_trans () const
{
  QMatrix4x4 t;
  t.rotate (-cam_elevation (), 1.0, 0.0, 0.0);
  t.rotate (cam_azimuth (), 0.0, 1.0, 0.0);
  return t;
}

void
D25ViewWidget::refresh ()
{
  QVector3D cp = cam_position ();

printf("@@@ e=%g   a=%g     x,y,z=%g,%g,%g    d=%g,%g,%g    s=%g    f=%g\n", cam_elevation (), cam_azimuth (), cp.x(), cp.y(), cp.z(), m_displacement.x(), m_displacement.y(), m_displacement.z(), m_scale_factor, m_focus_dist); fflush(stdout);

  update ();
}

void
D25ViewWidget::attach_view (LayoutView *view)
{
  if (mp_view != view) {

    mp_view = view;

    prepare_view ();
    reset ();

  }
}

void
D25ViewWidget::prepare_view ()
{
  m_layers.clear ();
  m_vertex_chunks.clear ();

  m_bbox = db::DBox ();
  bool zset = false;
  m_zmin = m_zmax = 0.0;

  if (! mp_view) {
    m_bbox = db::DBox (-1.0, -1.0, 1.0, 1.0);
    return;
  }

  double z = 0.0, dz = 0.2; // @@@

  for (lay::LayerPropertiesConstIterator lp = mp_view->begin_layers (); ! lp.at_end (); ++lp) {

    if (! lp->has_children () && lp->visible (true) && lp->cellview_index () >= 0 && lp->cellview_index () < int (mp_view->cellviews ())) {

      lay::color_t color = lp->fill_color (true);

      m_vertex_chunks.push_back (chunks_type ());

      LayerInfo info;
      info.color[0] = ((color >> 16) & 0xff) / 255.0f;
      info.color[1] = ((color >> 8) & 0xff) / 255.0f;
      info.color[2] = (color & 0xff) / 255.0f;
      info.vertex_chunk = &m_vertex_chunks.back ();

      m_layers.push_back (info);

      const lay::CellView &cv = mp_view->cellview ((unsigned int) lp->cellview_index ());
      render_layout (m_vertex_chunks.back (), cv->layout (), *cv.cell (), (unsigned int) lp->layer_index (), z, z + dz);

      m_bbox += db::DBox (cv.cell ()->bbox ((unsigned int) lp->layer_index ())) * cv->layout ().dbu ();

      if (! zset) {
        m_zmin = z;
        m_zmax = z + dz;
        zset = true;
      } else {
        m_zmin = std::min (z, m_zmin);
        m_zmax = std::max (z + dz, m_zmax);
      }

      z += dz; // @@@

    }

  }
}

void
D25ViewWidget::render_polygon (D25ViewWidget::chunks_type &chunks, const db::Polygon &poly, double dbu, double zstart, double zstop)
{
  if (poly.hull ().size () > 4) {

    std::vector<db::Polygon> poly_heap;

    db::split_polygon (poly, poly_heap);
    for (std::vector<db::Polygon>::const_iterator p = poly_heap.begin (); p != poly_heap.end (); ++p) {
      render_polygon (chunks, *p, dbu, zstart, zstop);
    }

  } else if (poly.hull ().size () >= 3) {

    db::Point pts [4];
    std::copy (poly.hull ().begin (), poly.hull ().end (), &pts [0]);

    //  triangle bottom

    chunks.add (pts[0].x () * dbu);
    chunks.add (zstart);
    chunks.add (pts[0].y () * dbu);

    chunks.add (pts[2].x () * dbu);
    chunks.add (zstart);
    chunks.add (pts[2].y () * dbu);

    chunks.add (pts[1].x () * dbu);
    chunks.add (zstart);
    chunks.add (pts[1].y () * dbu);

    //  triangle top

    chunks.add (pts[0].x () * dbu);
    chunks.add (zstop);
    chunks.add (pts[0].y () * dbu);

    chunks.add (pts[1].x () * dbu);
    chunks.add (zstop);
    chunks.add (pts[1].y () * dbu);

    chunks.add (pts[2].x () * dbu);
    chunks.add (zstop);
    chunks.add (pts[2].y () * dbu);

    if (poly.hull ().size () == 4) {

      //  triangle bottom

      chunks.add (pts[0].x () * dbu);
      chunks.add (zstart);
      chunks.add (pts[0].y () * dbu);

      chunks.add (pts[3].x () * dbu);
      chunks.add (zstart);
      chunks.add (pts[3].y () * dbu);

      chunks.add (pts[2].x () * dbu);
      chunks.add (zstart);
      chunks.add (pts[2].y () * dbu);

      //  triangle top

      chunks.add (pts[0].x () * dbu);
      chunks.add (zstop);
      chunks.add (pts[0].y () * dbu);

      chunks.add (pts[2].x () * dbu);
      chunks.add (zstop);
      chunks.add (pts[2].y () * dbu);

      chunks.add (pts[3].x () * dbu);
      chunks.add (zstop);
      chunks.add (pts[3].y () * dbu);

    }

  }
}

void
D25ViewWidget::render_wall (D25ViewWidget::chunks_type &chunks, const db::Edge &edge, double dbu, double zstart, double zstop)
{
  chunks.add (edge.p1 ().x () * dbu);
  chunks.add (zstart);
  chunks.add (edge.p1 ().y () * dbu);

  chunks.add (edge.p2 ().x () * dbu);
  chunks.add (zstop);
  chunks.add (edge.p2 ().y () * dbu);

  chunks.add (edge.p1 ().x () * dbu);
  chunks.add (zstop);
  chunks.add (edge.p1 ().y () * dbu);

  chunks.add (edge.p1 ().x () * dbu);
  chunks.add (zstart);
  chunks.add (edge.p1 ().y () * dbu);

  chunks.add (edge.p2 ().x () * dbu);
  chunks.add (zstart);
  chunks.add (edge.p2 ().y () * dbu);

  chunks.add (edge.p2 ().x () * dbu);
  chunks.add (zstop);
  chunks.add (edge.p2 ().y () * dbu);
}

void
D25ViewWidget::render_layout (D25ViewWidget::chunks_type &chunks, const db::Layout &layout, const db::Cell &cell, unsigned int layer, double zstart, double zstop)
{
  db::EdgeProcessor ep;
  std::vector<db::Polygon> poly_heap;

  //  @@@ hidden cells, hierarchy depth ...
  db::RecursiveShapeIterator s (layout, cell, layer);
  s.shape_flags (db::ShapeIterator::Polygons | db::ShapeIterator::Paths | db::ShapeIterator::Boxes);
  for ( ; ! s.at_end (); ++s) {

    db::Polygon polygon;
    s->polygon (polygon);
    polygon.transform (s.trans ());

    if (polygon.holes () == 0 && polygon.hull ().size () <= 4) {

      render_polygon (chunks, polygon, layout.dbu (), zstart, zstop);

      for (db::Polygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
        render_wall (chunks, *e, layout.dbu (), zstart, zstop);
      }

    } else {

      poly_heap.clear ();
      ep.clear ();

      ep.insert_sequence (polygon.begin_edge ());
      {
        db::PolygonContainer pc (poly_heap);
        db::PolygonGenerator out (pc, true /*resolve holes*/, false /*min coherence for splitting*/);
        db::SimpleMerge op;
        ep.process (out, op);
      }

      for (std::vector<db::Polygon>::const_iterator p = poly_heap.begin (); p != poly_heap.end (); ++p) {
        render_polygon (chunks, *p, layout.dbu (), zstart, zstop);
      }

      poly_heap.clear ();
      ep.clear ();

      ep.insert_sequence (polygon.begin_edge ());
      {
        db::PolygonContainer pc (poly_heap);
        db::PolygonGenerator out (pc, false /*don't resolve holes*/, false /*min coherence for splitting*/);
        db::SimpleMerge op;
        ep.process (out, op);
      }

      for (std::vector<db::Polygon>::const_iterator p = poly_heap.begin (); p != poly_heap.end (); ++p) {
        for (db::Polygon::polygon_edge_iterator e = p->begin_edge (); ! e.at_end (); ++e) {
          render_wall (chunks, *e, layout.dbu (), zstart, zstop);
        }
      }

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
  QOpenGLFunctions::initializeOpenGLFunctions();

  glEnable (GL_BLEND);
  //  @@@ dark background
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //  @@@ white background
  // @@@ glBlendFunc (GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

  static const char *shapes_vertex_shader_source =
      "#version 320 es\n"
      "#undef lowp\n"
      "#undef highp\n"
      "#undef mediump\n"
      "layout (location = 0) in vec4 posAttr;\n"
      "\n"
      "void main() {\n"
      "   gl_Position = posAttr;\n"
      "}\n";

  static const char *shapes_geometry_shader_source =
      "#version 320 es\n"
      "#undef lowp\n"
      "#undef highp\n"
      "#undef mediump\n"
      "\n"
      "uniform vec4 color;\n"
      "uniform vec4 ambient;\n"
      "uniform vec3 illum;\n"
      "out lowp vec4 vertexColor;\n"
      "uniform mat4 geo_matrix;\n"
      "uniform mat4 cam_matrix;\n"
      "layout (triangles) in;\n"
      "layout (triangle_strip, max_vertices = 3) out;\n"
      "\n"
      "void main() {\n"
      "   vec4 p0 = gl_in[0].gl_Position;\n"
      "   vec4 p1 = gl_in[1].gl_Position;\n"
      "   vec4 p2 = gl_in[2].gl_Position;\n"
      "   vec3 n = cross(p2.xyz - p0.xyz, p1.xyz - p0.xyz);\n"
      "   float dp = dot(normalize(n), illum);\n"
      "   vertexColor = color * (dp * 0.5 + 0.5) - (min(0.0, dp) * 0.5 * ambient);\n"
      "   vertexColor.a = 1.0;\n"
      "   gl_Position = cam_matrix * geo_matrix * p0;\n"
      "   EmitVertex();\n"
      "   gl_Position = cam_matrix * geo_matrix * p1;\n"
      "   EmitVertex();\n"
      "   gl_Position = cam_matrix * geo_matrix * p2;\n"
      "   EmitVertex();\n"
      "   EndPrimitive();\n"
      "}\n";

  static const char *shapes_fragment_shader_source =
      "#version 320 es\n"
      "#undef lowp\n"
      "#undef highp\n"
      "#undef mediump\n"
      "in lowp vec4 vertexColor;\n"
      "out lowp vec4 fragColor;\n"
      "\n"
      "vec4 color_by_z(lowp vec4 c, highp float z) {\n"
      "  lowp vec4 mist_color = vec4(c.g * 0.4, c.g * 0.4, c.g * 0.4, 1.0);\n"
      "  highp float d = 0.12;\n" //  d + dd/2 = 0.15 = 1/?
      "  highp float dd = 0.06;\n"
      "  highp float f = 1.0;\n"
      "  if (z < d - dd) {\n"
      "    f = 0.0;\n"
      "  } else if (z < d + dd) {\n"
      "    f = (z - (d - dd)) / (2.0 * dd);\n"
      "  }\n"
      "  return (1.0 - f) * mist_color + f * c;\n"
      "};\n"
      "\n"
      "void main() {\n"
      "   fragColor = color_by_z(vertexColor, gl_FragCoord.w);\n"
      "}\n";

  m_shapes_program = new QOpenGLShaderProgram (this);
  if (! m_shapes_program->addShaderFromSourceCode (QOpenGLShader::Vertex, shapes_vertex_shader_source)) {
    throw tl::Exception (std::string ("Shapes vertex shader compilation failed:\n") + tl::to_string (m_shapes_program->log ()));
  }
  if (! m_shapes_program->addShaderFromSourceCode (QOpenGLShader::Geometry, shapes_geometry_shader_source)) {
    throw tl::Exception (std::string ("Shapes geometry shader compilation failed:\n") + tl::to_string (m_shapes_program->log ()));
  }
  if (! m_shapes_program->addShaderFromSourceCode (QOpenGLShader::Fragment, shapes_fragment_shader_source)) {
    throw tl::Exception (std::string ("Shapes fragment shader compilation failed:\n") + tl::to_string (m_shapes_program->log ()));
  }
  if (! m_shapes_program->link ()) {
    throw tl::Exception (std::string ("Shapes shader program linking failed failed:\n") + tl::to_string (m_shapes_program->log ()));
  }

  //  grid plane shader source

  static const char *gridplan_vertex_shader_source =
      "#version 320 es\n"
      "#undef lowp\n"
      "#undef highp\n"
      "#undef mediump\n"
      "layout (location = 0) in vec4 posAttr;\n"
      "uniform mat4 matrix;\n"
      "\n"
      "void main() {\n"
      "   gl_Position = matrix * posAttr;\n"
      "}\n";

  static const char *gridplan_fragment_shader_source =
      "#version 320 es\n"
      "#undef lowp\n"
      "#undef highp\n"
      "#undef mediump\n"
      "uniform lowp vec4 color;\n"
      "out lowp vec4 fragColor;\n"
      "void main() {\n"
      "   fragColor = color;\n"
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
  GLfloat vertices[6000];
  size_t nmax = sizeof (vertices) / sizeof (GLfloat);

  const qreal retinaScale = devicePixelRatio ();
  glViewport (0, 0, width () * retinaScale, height () * retinaScale);

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // @@@ white background: glClearColor (1.0, 1.0, 1.0, 1.0);

  QMatrix4x4 scene_trans, scene_trans_wo_y;

  //  provide the displacement and scaling (in this order!)
  scene_trans.scale (m_scale_factor);
  scene_trans.translate (m_displacement);
  //  this way we can use y as z coordinate when drawing
  scene_trans.scale (1.0, 1.0, -1.0);

  scene_trans_wo_y = scene_trans;
  scene_trans_wo_y.translate (QVector3D (0.0, -m_displacement.y (), 0.0));

  const int positions = 0;

  m_shapes_program->bind ();

  m_shapes_program->setUniformValue ("geo_matrix", cam_trans () * scene_trans);
  m_shapes_program->setUniformValue ("cam_matrix", cam_perspective ());

  //  NOTE: z axis of illum points towards the scene because we include the z inversion in the scene transformation matrix
  m_shapes_program->setUniformValue ("illum", QVector3D (-3.0, -4.0, 2.0).normalized ());

  m_shapes_program->setUniformValue ("ambient", QVector4D (0.5, 0.5, 0.5, 0.5));

  glEnable (GL_DEPTH_TEST);
  glEnableVertexAttribArray (positions);

  for (std::list<LayerInfo>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {

    m_shapes_program->setUniformValue ("color", l->color [0], l->color [1], l->color [2], l->color [3]);

    for (chunks_type::iterator c = l->vertex_chunk->begin (); c != l->vertex_chunk->end (); ++c) {
      glVertexAttribPointer (positions, 3, GL_FLOAT, GL_FALSE, 0, c->front ());
      glDrawArrays (GL_TRIANGLES, 0, c->size () / 3);
    }

  }

  glDisableVertexAttribArray (positions);

  m_shapes_program->release ();

  m_gridplane_program->bind ();

  glEnable (GL_DEPTH_TEST);
  glEnableVertexAttribArray (positions);

  //  draw pivot compass

  m_gridplane_program->setUniformValue ("matrix", cam_perspective () * cam_trans ());

  size_t index = 0;

  double compass_rad = 0.3;
  double compass_bars = 0.4;

  vertices[index++] = -compass_bars;
  vertices[index++] = 0.0;
  vertices[index++] = 0.0;

  vertices[index++] = compass_bars;
  vertices[index++] = 0.0;
  vertices[index++] = 0.0;

  vertices[index++] = 0.0;
  vertices[index++] = 0.0;
  vertices[index++] = -compass_bars;

  vertices[index++] = 0.0;
  vertices[index++] = 0.0;
  vertices[index++] = compass_bars;

  int ncircle = 64;
  double x = compass_rad, z = 0.0;
  double da = 1.0 / double (ncircle) * M_PI * 2.0;

  for (int i = 0; i < ncircle; ++i) {

    double a = double (i + 1) * da;
    double xx = compass_rad * cos (a);
    double zz = compass_rad * sin (a);

    vertices[index++] = x;
    vertices[index++] = 0.0;
    vertices[index++] = z;

    vertices[index++] = xx;
    vertices[index++] = 0.0;
    vertices[index++] = zz;

    x = xx;
    z = zz;

  }

  m_gridplane_program->setUniformValue ("color", 1.0, 1.0, 1.0, 0.25f);

  glVertexAttribPointer (positions, 3, GL_FLOAT, GL_FALSE, 0, vertices);

  glLineWidth (2.0);
  glDrawArrays (GL_LINES, 0, index / 3);

  index = 0;

  //  arrow
  vertices[index++] = -0.25 * compass_rad;
  vertices[index++] = 0.0;
  vertices[index++] = 0.6 * compass_rad;

  vertices[index++] = 0.0;
  vertices[index++] = 0.0;
  vertices[index++] = -0.8 * compass_rad;

  vertices[index++] = 0.25 * compass_rad;
  vertices[index++] = 0.0;
  vertices[index++] = 0.6 * compass_rad;

  glVertexAttribPointer (positions, 3, GL_FLOAT, GL_FALSE, 0, vertices);

  glDrawArrays (GL_TRIANGLES, 0, index / 3);

  //  draw base plane

  m_gridplane_program->setUniformValue ("matrix", cam_perspective () * cam_trans () * scene_trans_wo_y);

  std::pair<double, double> gg = find_grid (std::max (m_bbox.width (), m_bbox.height ()));
  double gminor = gg.second, gmajor = gg.first;

  double margin = std::max (m_bbox.width (), m_bbox.height ()) * 0.02;
  double l = m_bbox.left () - margin;
  double r = m_bbox.right () + margin;
  double b = m_bbox.bottom () - margin;
  double t = m_bbox.top () + margin;

  // @@@

  //  major and minor grid lines

  const double epsilon = 1e-6;

  for (int major = 0; major < 2; ++major) {

    m_gridplane_program->setUniformValue ("color", 1.0, 1.0, 1.0, major ? 0.25f : 0.15f);

    size_t index = 0;
    double x, y;
    double step = (major ? gmajor : gminor);

    x = ceil (l / step) * step;
    for ( ; index < nmax && x < r - step * epsilon; x += step) {
      if ((fabs (floor (x / gmajor + 0.5) * gmajor - x) < epsilon) == (major != 0)) {
        vertices [index++] = x;
        vertices [index++] = 0.0;
        vertices [index++] = b;
        vertices [index++] = x;
        vertices [index++] = 0.0;
        vertices [index++] = t;
      }
    }

    y = ceil (b / step) * step;
    for ( ; index < nmax && y < t - step * epsilon; y += step) {
      if ((fabs (floor (y / gmajor + 0.5) * gmajor - y) < epsilon) == (major != 0)) {
        vertices [index++] = l;
        vertices [index++] = 0.0;
        vertices [index++] = y;
        vertices [index++] = r;
        vertices [index++] = 0.0;
        vertices [index++] = y;
      }
    }

    glVertexAttribPointer (positions, 3, GL_FLOAT, GL_FALSE, 0, vertices);

    glLineWidth (2.0);
    glDrawArrays (GL_LINES, 0, index / 3);

  }

  l = m_bbox.left ();
  r = m_bbox.right ();
  b = m_bbox.bottom ();
  t = m_bbox.top ();

  GLfloat plane_vertices[] = {
      float (l), 0.0f, float (b),    float (l), 0.0f, float (t),    float (r), 0.0f, float (t),
      float (l), 0.0f, float (b),    float (r), 0.0f, float (t),    float (r), 0.0f, float (b)
  };

  m_gridplane_program->setUniformValue ("color", 1.0, 1.0, 1.0, 0.1f);

  glVertexAttribPointer (positions, 3, GL_FLOAT, GL_FALSE, 0, plane_vertices);

  glDrawArrays (GL_TRIANGLES, 0, 6);

  glDisableVertexAttribArray (positions);

  m_shapes_program->release ();
}

void
D25ViewWidget::resizeGL (int /*w*/, int /*h*/)
{
  refresh ();
}

}



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
    m_shapes_program (0), m_dragging (false), m_rotating (false), m_cam_azimuth (0.0), m_cam_elevation (0.0), m_top_view (false)
{
  QSurfaceFormat format;
  format.setDepthBufferSize (24);
  format.setSamples (4); //  more -> widget extends beyond boundary!
  setFormat (format);

  m_scale_factor = 1.0;
  m_focus_dist = 0.0;
}

D25ViewWidget::~D25ViewWidget ()
{
  // Make sure the context is current and then explicitly
  // destroy all underlying OpenGL resources.
  makeCurrent();

  delete m_shapes_program;

  doneCurrent();
}

void
D25ViewWidget::initializeGL ()
{
  QOpenGLFunctions::initializeOpenGLFunctions();

  glEnable (GL_DEPTH_TEST);
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
      "uniform mat4 matrix;\n"
      "layout (triangles) in;\n"
      "layout (triangle_strip, max_vertices = 3) out;\n"
      "\n"
      "void main() {\n"
      "   vec4 p0 = gl_in[0].gl_Position;\n"
      "   vec4 p1 = gl_in[1].gl_Position;\n"
      "   vec4 p2 = gl_in[2].gl_Position;\n"
      "   vec3 n = cross(p2.xyz - p0.xyz, p1.xyz - p0.xyz);\n"
      "   float dp = dot(normalize(n), illum);\n"
      "   vertexColor.rgb = color.rgb * (dp * 0.5 + 0.5) - (min(0.0, dp) * 0.5 * ambient.rgb);\n"
      "   vertexColor.a = 1.0;\n"
      "   gl_Position = matrix * p0;\n"
      "   EmitVertex();\n"
      "   gl_Position = matrix * p1;\n"
      "   EmitVertex();\n"
      "   gl_Position = matrix * p2;\n"
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
      "void main() {\n"
      "   fragColor = vertexColor;\n"
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
D25ViewWidget::wheelEvent (QWheelEvent *event)
{
  double px = (event->pos ().x () - width () / 2) * 2.0 / width ();
  double py = -(event->pos ().y () - height () / 2) * 2.0 / height ();

  //  compute vector of line of sight
  std::pair<QVector3D, QVector3D> ray = camera_normal (cam_perspective () * cam_trans (), px, py);

  //  by definition the ray goes through the camera position
  float focal_length = 2.0;
  QVector3D hp = hit_point_with_scene (cam_position () + focal_length * ray.second, ray.second);

  if (false /*@@@*/ && (event->modifiers () & Qt::ShiftModifier)) {

    //  "Shift" is closeup

    double f = event->angleDelta ().y () * (1.0 / (90 * 8));
    m_displacement += -f * cam_position ().length () * ray.second;

  } else {

    //  No shift is zoom

    double f = exp (event->angleDelta ().y () * (1.0 / (90 * 8)));

    QVector3D initial_displacement = m_displacement;
    QVector3D displacement = m_displacement;

    displacement = hp * (1.0 - f) + displacement * f;
    m_scale_factor *= f;

    //  normalize the scene translation so the scene does not "flee"

    QMatrix4x4 ct = cam_trans ();
    initial_displacement = ct.map (initial_displacement);
    displacement = ct.map (displacement);

    lay::normalize_scene_trans (cam_perspective (), displacement, m_scale_factor, initial_displacement.z ());

    m_displacement = ct.inverted ().map (displacement);

  }

  update_cam_trans ();
}

void
D25ViewWidget::keyPressEvent (QKeyEvent *event)
{
  if (event->key () == Qt::Key_Shift) {
    m_top_view = true;
    update_cam_trans ();
  }
}

void
D25ViewWidget::keyReleaseEvent (QKeyEvent *event)
{
  if (event->key () == Qt::Key_Shift) {
    m_top_view = false;
    update_cam_trans ();
  }
}

QVector3D
D25ViewWidget::hit_point_with_scene (const QVector3D &line, const QVector3D &line_dir)
{
  QVector3D corner = QVector3D (m_bbox.left (), m_zmin, -(m_bbox.bottom () + m_bbox.height ())) * m_scale_factor + m_displacement;
  QVector3D dim = QVector3D (m_bbox.width (), m_zmax - m_zmin, m_bbox.height ()) * m_scale_factor;

  std::pair<bool, QVector3D> hp = lay::hit_point_with_cuboid (line, line_dir, corner, dim);
  if (! hp.first) {
    return line;
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

  if (m_dragging || m_rotating) {

    QVector3D cd = cam_direction ();
    QVector3D cp = cam_position ();
    QVector3D hp = hit_point_with_scene (cp + m_focus_dist * cd, cd);
    m_focus_dist = std::max (m_focus_dist, double ((cp - hp).length ()));

  }
}

void
D25ViewWidget::mouseReleaseEvent (QMouseEvent * /*event*/)
{
  m_dragging = false;
}

void
D25ViewWidget::mouseMoveEvent (QMouseEvent *event)
{
  if (m_dragging) {

    //  for the chosen perspective transformation:
    double cal = 0.6 * m_focus_dist;

    QPoint d = event->pos () - m_start_pos;
    double f = cal * 2.0 / double (height ());
    double dx = d.x () * f;
    double dy = -d.y () * f;

    QVector3D xv (cos (cam_azimuth () * M_PI / 180.0), 0.0, -sin (cam_azimuth () * M_PI / 180.0));
    double re = sin (cam_elevation () * M_PI / 180.0);
    QVector3D yv (-re * xv.z (), cos (cam_elevation () * M_PI / 180.0), re * xv.x ());
    QVector3D drag = xv * dx + yv * dy;

    m_displacement = m_start_displacement + drag;

    update_cam_trans ();

  } else if (m_rotating) {

    //  @@@ needs redo ...
    //  @@@ consider m_top_view
    double focus_dist = 4.0; // @@@

    QPoint d = event->pos () - m_start_pos;

    double ax = atan (d.x () / (0.5 * height ())) * 180 / M_PI;
    double ay = atan (-d.y () / (0.5 * height ())) * 180 / M_PI;

    m_cam_elevation = m_start_cam_elevation + ay;
    m_cam_azimuth = m_start_cam_azimuth + ax;

    update_cam_trans ();

  }
}

QVector3D
D25ViewWidget::cam_direction () const
{
  double azimuth = cam_azimuth ();
  double elevation = cam_elevation ();

  //  positive azimuth: camera looks left
  //  positive elevation: camera looks up
  double y = sin (elevation * M_PI / 180.0);
  double r = cos (elevation * M_PI / 180.0);
  double x = r * sin (azimuth * M_PI / 180.0);
  double z = r * cos (azimuth * M_PI / 180.0);
  return QVector3D (x, y, -z);
}

QVector3D
D25ViewWidget::cam_position () const
{
  double focus_dist = 4.0;
  return cam_direction () * -focus_dist;
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
  t.perspective (60.0f, float (width ()) / float (height ()), 0.1f, 100.0f);
  return t;
}

QMatrix4x4
D25ViewWidget::cam_trans () const
{
  QMatrix4x4 t;
  t.rotate (-cam_elevation (), 1.0, 0.0, 0.0);
  t.rotate (cam_azimuth (), 0.0, 1.0, 0.0);
  t.translate (-cam_position ());
  return t;
}

void
D25ViewWidget::update_cam_trans ()
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
    m_layers.clear ();
    m_vertex_chunks.clear ();

    if (mp_view) {
      prepare_view ();
    }

  }
}

void
D25ViewWidget::prepare_view ()
{
  double z = 0.0, dz = 0.2; // @@@

  m_bbox = db::DBox ();
  bool zset = false;
  m_zmin = m_zmax = 0.0;

  for (lay::LayerPropertiesConstIterator lp = mp_view->begin_layers (); ! lp.at_end (); ++lp) {

    if (! lp->has_children () && lp->visible (true) && lp->cellview_index () >= 0 && lp->cellview_index () < int (mp_view->cellviews ())) {

      lay::color_t color = lp->fill_color (true);

      m_vertex_chunks.push_back (chunks_type ());

      LayerInfo info;
      // @@@ use alpha?
      info.color[0] = ((color >> 16) & 0xff) / 255.0f;
      info.color[1] = ((color >> 8) & 0xff) / 255.0f;
      info.color[2] = (color & 0xff) / 255.0f;
      info.color[3] = 1.0;
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

void
D25ViewWidget::paintGL ()
{
  printf("@@@ width=%d,height=%d\n", width(),height()); // @@@
  const qreal retinaScale = devicePixelRatio ();
  glViewport (0, 0, width () * retinaScale, height () * retinaScale);

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // @@@ white background: glClearColor (1.0, 1.0, 1.0, 1.0);

  const int positions = 0;

  m_shapes_program->bind ();

  QMatrix4x4 scene_trans;

  //  provide the displacement and scaling
  scene_trans.translate (m_displacement);
  scene_trans.scale (m_scale_factor);
  //  this way we can use y as z coordinate when drawing
  scene_trans.scale (1.0, 1.0, -1.0);

  m_shapes_program->setUniformValue ("matrix", cam_perspective () * cam_trans () * scene_trans);

  //  NOTE: z axis of illum points towards the scene because we include the z inversion in the scene transformation matrix
  m_shapes_program->setUniformValue ("illum", QVector3D (-3.0, -4.0, 2.0).normalized ());

  m_shapes_program->setUniformValue ("ambient", QVector4D (0.5, 0.5, 0.5, 0.5));

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

  glEnableVertexAttribArray (positions);

  // @@@ m_gridplane_program->setUniformValue ("matrix", m_cam_trans * m_scene_trans);
  m_gridplane_program->setUniformValue ("matrix", QMatrix4x4 ()); // @@@

  // @@@

  GLfloat plane_vertices[] = {
      -1.05, 0.0, -2.05,    -1.05, 0.0, 0.05,     1.05, 0.0, 0.05,
      -1.05, 0.0, -2.05,    1.05, 0.0, 0.05,      1.05, 0.0, -2.05
  };

  m_gridplane_program->setUniformValue ("color", 1.0, 1.0, 1.0, 0.2f);

  glVertexAttribPointer (positions, 3, GL_FLOAT, GL_FALSE, 0, plane_vertices);

  glDrawArrays (GL_TRIANGLES, 0, 6);

#if 0
  GLfloat gridline_vertices[] = {
      -1.0, 0.0, -2.0,    -1.0, 0.0, 0.0,
      -0.75, 0.0, -2.0,   -0.75, 0.0, 0.0,
      -0.5, 0.0, -2.0,    -0.5, 0.0, 0.0,
      -0.25, 0.0, -2.0,   -0.25, 0.0, 0.0,
      0.0, 0.0, -2.0,     0.0, 0.0, 0.0,
      0.25, 0.0, -2.0,    0.25, 0.0, 0.0,
      0.5, 0.0, -2.0,     0.5, 0.0, 0.0,
      0.75, 0.0, -2.0,    0.75, 0.0, 0.0,
      1.0, 0.0, -2.0,     1.0, 0.0, 0.0,
      1.0, 0.0, -2.0,     -1.0, 0.0, -2.0,
      1.0, 0.0, -1.75,    -1.0, 0.0, -1.75,
      1.0, 0.0, -1.5 ,    -1.0, 0.0, -1.5,
      1.0, 0.0, -1.25,    -1.0, 0.0, -1.25,
      1.0, 0.0, -1.0,     -1.0, 0.0, -1.0,
      1.0, 0.0, -0.75,    -1.0, 0.0, -0.75,
      1.0, 0.0, -0.5 ,    -1.0, 0.0, -0.5,
      1.0, 0.0, -0.25,    -1.0, 0.0, -0.25,
      1.0, 0.0, 0.0,      -1.0, 0.0, 0.0
  };
#else
  GLfloat gridline_vertices[] = {
      -1.0, -1.0, 0.0,    -1.0, 1.0, 0.0,
      -0.75, -1.0, 0.0,   -0.75, 1.0, 0.0,
      -0.5, -1.0, 0.0,    -0.5, 1.0, 0.0,
      -0.25, -1.0, 0.0,   -0.25, 1.0, 0.0,
      0.0, -1.0, 0.0,     0.0, 1.0, 0.0,
      0.25, -1.0, 0.0,    0.25, 1.0, 0.0,
      0.5, -1.0, 0.0,     0.5, 1.0, 0.0,
      0.75, -1.0, 0.0,    0.75, 1.0, 0.0,
      1.0, -1.0, 0.0,     1.0, 1.0, 0.0,
      1.0, -1.0, 0.0,     -1.0, -1.0, 0.0,
      1.0, -0.75, 0.0,    -1.0, -0.75, 0.0,
      1.0, -0.5, 0.0,     -1.0, -0.5, 0.0,
      1.0, -0.25, 0.0,    -1.0, -0.25, 0.0,
      1.0, 0.0, 0.0,      -1.0, 0.0, 0.0,
      1.0, 0.25, 0.0,     -1.0, 0.25, 0.0,
      1.0, 0.5, 0.0,      -1.0, 0.5, 0.0,
      1.0, 0.75, 0.0,     -1.0, 0.75, 0.0,
      1.0, 1.0, 0.0,      -1.0, 1.0, 0.0
  };
#endif

  m_shapes_program->setUniformValue ("vertexColor", 1.0, 1.0, 1.0, 0.2f);

  glVertexAttribPointer (positions, 3, GL_FLOAT, GL_FALSE, 0, gridline_vertices);

  glLineWidth (2.0);
  glDrawArrays (GL_LINES, 0, 36);

  glDisableVertexAttribArray (positions);

  m_shapes_program->release ();
}

void
D25ViewWidget::resizeGL (int /*w*/, int /*h*/)
{
  update_cam_trans ();
}

}


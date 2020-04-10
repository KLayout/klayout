
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
#include "tlException.h"

#include <QWheelEvent>
#include <QMouseEvent>

#include "math.h"

namespace lay
{

D25ViewWidget::D25ViewWidget (QWidget *parent)
  : QOpenGLWidget (parent),
    m_program (0), m_dragging (false), m_rotating (false), m_cam_azimuth (0.0), m_cam_elevation (0.0)
{
  QSurfaceFormat format;
  format.setDepthBufferSize (24);
  format.setSamples (4); //  more -> widget extends beyond boundary!
  setFormat (format);

  m_cam_position = QVector3D (0.0, 0.0, 3.0); // @@@
}

D25ViewWidget::~D25ViewWidget ()
{
  // Make sure the context is current and then explicitly
  // destroy all underlying OpenGL resources.
  makeCurrent();

  delete m_program;

  doneCurrent();
}

void
D25ViewWidget::initializeGL ()
{
  QOpenGLFunctions::initializeOpenGLFunctions();

  glEnable (GL_DEPTH_TEST);
  glEnable (GL_BLEND);
  //  @@@ dark background
  //  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //  @@@ white background
  glBlendFunc (GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

  static const char *vertexShaderSource =
      "attribute highp vec4 posAttr;\n"
      "uniform highp mat4 matrix;\n"
      "attribute lowp vec4 colAttr;\n"
      "varying lowp vec4 col;\n"
      "void main() {\n"
      "   col = colAttr;\n"
      "   gl_Position = matrix * posAttr;\n"
      "}\n";

  static const char *fragmentShaderSource =
      "varying lowp vec4 col;\n"
      "void main() {\n"
      "   gl_FragColor = col;\n"
      "}\n";

  m_program = new QOpenGLShaderProgram (this);
  if (! m_program->addShaderFromSourceCode (QOpenGLShader::Vertex, vertexShaderSource)) {
    throw tl::Exception (std::string ("Vertex shader compilation failed:\n") + tl::to_string (m_program->log ()));
  }
  if (! m_program->addShaderFromSourceCode (QOpenGLShader::Fragment, fragmentShaderSource)) {
    throw tl::Exception (std::string ("Fragment shader compilation failed:\n") + tl::to_string (m_program->log ()));
  }
  if (! m_program->link ()) {
    throw tl::Exception (std::string ("Linking failed:\n") + tl::to_string (m_program->log ()));
  }

  m_posAttr = m_program->attributeLocation ("posAttr");
  m_colAttr = m_program->attributeLocation ("colAttr");
  m_matrixUniform = m_program->uniformLocation ("matrix");
}

static QVector3D cam_direction (double azimuth, double elevation)
{
  //  positive azimuth: camera looks left
  //  positive elevation: camera looks up
  double y = sin (elevation * M_PI / 180.0);
  double r = cos (elevation * M_PI / 180.0);
  double x = r * sin (azimuth * M_PI / 180.0);
  double z = r * cos (azimuth * M_PI / 180.0);
  return QVector3D (x, y, -z);
}

void
D25ViewWidget::wheelEvent (QWheelEvent *event)
{
  double cal = 0.6;

  int dx = event->pos ().x () - width () / 2;
  int dy = -(event->pos ().y () - height () / 2);

  double da = atan (dx * cal * 2.0 / height ()) * 180 / M_PI;
  double de = atan (dy * cal * 2.0 / height ()) * 180 / M_PI;

  m_cam_position += (event->angleDelta ().y () * (1.0 / (45 * 8))) * cam_direction (m_cam_azimuth + da, m_cam_elevation + de);

  update_cam_trans ();
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
  m_start_cam_position = m_cam_position;
  m_start_cam_azimuth = m_cam_azimuth;
  m_start_cam_elevation = m_cam_elevation;
}

void
D25ViewWidget::mouseReleaseEvent (QMouseEvent *event)
{
  m_dragging = false;
}

void
D25ViewWidget::mouseMoveEvent (QMouseEvent *event)
{
  double focus_dist = 4.0;  //  4 times focal length

  if (m_dragging) {

    //  for the chosen perspective transformation:
    double cal = 0.6 * focus_dist;

    QPoint d = event->pos () - m_start_pos;
    double f = cal * 2.0 / double (height ());
    double dx = d.x () * f;
    double dy = -d.y () * f;

    QVector3D xv (cos (m_start_cam_azimuth * M_PI / 180.0), 0.0, -sin (m_start_cam_azimuth * M_PI / 180.0));
    double re = sin (m_start_cam_elevation * M_PI / 180.0);
    QVector3D yv (-re * xv.z (), cos (m_start_cam_elevation * M_PI / 180.0), re * xv.x ());
    QVector3D drag = xv * dx + yv * dy;

    //  "-drag" because we're not dragging the camera, we're dragging the scene
    m_cam_position = m_start_cam_position - drag;

    update_cam_trans ();

  } else if (m_rotating) {

    QPoint d = event->pos () - m_start_pos;

    double ax = atan (d.x () / (0.5 * height ())) * 180 / M_PI;
    double ay = atan (-d.y () / (0.5 * height ())) * 180 / M_PI;

    m_cam_elevation = m_start_cam_elevation + ay;
    m_cam_azimuth = m_start_cam_azimuth + ax;

    m_cam_position = (cam_direction (m_cam_azimuth, m_cam_elevation) * -focus_dist) + cam_direction (m_start_cam_azimuth, m_start_cam_elevation) * focus_dist + m_start_cam_position;

    update_cam_trans ();

  }
}

void
D25ViewWidget::update_cam_trans ()
{
printf("@@@ e=%g   a=%g     x,y,z=%g,%g,%g\n", m_cam_elevation, m_cam_azimuth, m_cam_position.x(), m_cam_position.y(), m_cam_position.z()); fflush(stdout);
  QMatrix4x4 t;

  //  third: elevation
  t.rotate (-m_cam_elevation, 1.0, 0.0, 0.0);

  //  second: azimuth
  t.rotate (m_cam_azimuth, 0.0, 1.0, 0.0);

  //  first: translate the origin into the cam's position
  t.translate (-m_cam_position);

  m_cam_trans = t;

  update ();
}

void
D25ViewWidget::paintGL ()
{
  const qreal retinaScale = devicePixelRatio ();
  glViewport (0, 0, width () * retinaScale, height () * retinaScale);

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor (1.0, 1.0, 1.0, 1.0);

  m_program->bind ();

  QMatrix4x4 matrix;
  matrix.perspective (60.0f, float (width ()) / float (height ()), 0.1f, 100.0f);
  matrix *= m_cam_trans;

  m_program->setUniformValue (m_matrixUniform, matrix);

  glEnableVertexAttribArray (m_posAttr);
  glEnableVertexAttribArray (m_colAttr);

  GLfloat vertices[] = {
      0.0f, 0.707f, -1.0,
      -0.5f, -0.5f, -1.0,
      0.5f, -0.5f, -1.0,
      -0.6 + 0.0f, 0.0707f, -1.0,
      -0.6 + -0.05f, -0.05f, -1.0,
      -0.6 + 0.05f, -0.05f, -1.0,
      0.0f, 0.707f, -1.5,
      -0.5f, -0.5f, -1.5,
      0.5f, -0.5f, -1.5,
      0.0f, 0.707f, -2.0,
      -0.5f, -0.5f, -2.0,
      0.5f, -0.5f, -2.0
  };

  GLfloat colors[] = {
      1.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 1.0f,
      0.0f, 0.0f, 1.0f,
      0.0f, 0.0f, 1.0f
  };

  glVertexAttribPointer (m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, vertices);
  glVertexAttribPointer (m_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

  glDrawArrays (GL_TRIANGLES, 0, 12);

  GLfloat plane_vertices[] = {
      -1.05, 0.0, -2.05,    -1.05, 0.0, 0.05,     1.05, 0.0, 0.05,
      -1.05, 0.0, -2.05,    1.05, 0.0, 0.05,      1.05, 0.0, -2.05
  };

  GLfloat plane_colors[] = {
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f
  };

  glVertexAttribPointer (m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, plane_vertices);
  glVertexAttribPointer (m_colAttr, 4, GL_FLOAT, GL_FALSE, 0, plane_colors);

  glDrawArrays (GL_TRIANGLES, 0, 6);

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

  GLfloat gridline_colors[] = {
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f,
      1.0f, 1.0f, 1.0f, 0.2f
  };

  glVertexAttribPointer (m_posAttr, 3, GL_FLOAT, GL_FALSE, 0, gridline_vertices);
  glVertexAttribPointer (m_colAttr, 4, GL_FLOAT, GL_FALSE, 0, gridline_colors);

  glLineWidth (2.0);
  glDrawArrays (GL_LINES, 0, 36);

  glDisableVertexAttribArray (m_posAttr);
  glDisableVertexAttribArray (m_colAttr);

  m_program->release ();
}

void
D25ViewWidget::resizeGL (int /*w*/, int /*h*/)
{
  update_cam_trans ();
}

}


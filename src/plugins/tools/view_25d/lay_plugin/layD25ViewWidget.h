
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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

#ifndef HDR_layD25ViewWidget
#define HDR_layD25ViewWidget

#include "dbPolygon.h"

#include "layD25MemChunks.h"
#include "layD25Camera.h"

#include <QOpenGLWidget>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QPoint>
#include <QVector3D>

#include <memory>

namespace db
{
  class Layout;
  class Cell;
}

namespace lay
{

class LayoutView;
class D25ViewWidget;

class D25InteractionMode
{
public:
  D25InteractionMode (D25ViewWidget *widget);
  virtual ~D25InteractionMode ();

  D25ViewWidget *view () { return mp_view; }
  virtual void mouse_move (QMouseEvent *event) = 0;

private:
  D25ViewWidget *mp_view;
};

class D25ViewWidget
  : public QOpenGLWidget,
    private QOpenGLFunctions,
    public D25Camera
{
Q_OBJECT 

public:
  D25ViewWidget (QWidget *parent);
  ~D25ViewWidget ();

  void keyPressEvent (QKeyEvent *event);
  void keyReleaseEvent (QKeyEvent *event);
  void wheelEvent (QWheelEvent *event);
  void mousePressEvent (QMouseEvent *event);
  void mouseReleaseEvent (QMouseEvent *event);
  void mouseMoveEvent (QMouseEvent *event);

  bool attach_view(lay::LayoutView *view);

  QVector3D hit_point_with_scene(const QVector3D &line_dir);
  void refresh ();
  void reset ();

  QVector3D displacement () const { return m_displacement; }

  void set_displacement (const QVector3D &d)
  {
    m_displacement = d;
    refresh ();
  }

  double scale_factor () const { return m_scale_factor; }

  void set_scale_factor (double f)
  {
    m_scale_factor = f;
    refresh ();
  }

  const std::string &error () const
  {
    return m_error;
  }

signals:
  void scale_factor_changed (double f);
  void init_failed ();

protected:
  virtual void camera_changed ();
  virtual double aspect_ratio () const;

public slots:
  void fit ();

private:
  typedef lay::mem_chunks<GLfloat, 1024 * 18> chunks_type;

  std::unique_ptr<D25InteractionMode> mp_mode;
  QOpenGLShaderProgram *m_shapes_program, *m_gridplane_program;
  std::string m_error;
  double m_scale_factor;
  QVector3D m_displacement;
  lay::LayoutView *mp_view;
  db::DBox m_bbox;
  double m_zmin, m_zmax;

  std::list<chunks_type> m_vertex_chunks;
  std::list<chunks_type> m_normals_chunks;

  struct LayerInfo {
    const chunks_type *vertex_chunk;
    const chunks_type *normals_chunk;
    GLfloat color [4];
  };

  std::list<LayerInfo> m_layers;

  void initializeGL ();
  void paintGL ();
  void resizeGL (int w, int h);

  void do_initialize_gl ();
  bool prepare_view();
  void render_layout (D25ViewWidget::chunks_type &chunks, const db::Layout &layout, const db::Cell &cell, const db::Box &clip_box, unsigned int layer, double zstart, double zstop);
  void render_polygon (D25ViewWidget::chunks_type &chunks, const db::Polygon &poly, double dbu, double zstart, double zstop);
  void render_wall (D25ViewWidget::chunks_type &chunks, const db::Edge &poly, double dbu, double zstart, double zstop);
};

}

#endif


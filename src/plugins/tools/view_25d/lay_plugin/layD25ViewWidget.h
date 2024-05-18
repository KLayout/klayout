
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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
#include "layViewOp.h"

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
  class Region;
  class Edges;
  class EdgePairs;
  class RecursiveShapeIterator;
  struct LayerProperties;
}

namespace tl
{
  class AbsoluteProgress;
}

namespace lay
{

class LayoutViewBase;
class LayerPropertiesNode;
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
  typedef lay::mem_chunks<GLfloat, 1024 * 18> triangle_chunks_type;
  typedef lay::mem_chunks<GLfloat, 1024 * 6> line_chunks_type;

  struct LayerInfo
  {
    struct ZRangeCompare
    {
      bool operator() (const std::pair<double, double> &a, const std::pair<double, double> &b) const
      {
        typedef db::coord_traits<double> coord_traits;
        if (coord_traits::equal (a.first, b.first)) {
          return coord_traits::less (a.second, b.second);
        } else {
          return coord_traits::less (a.first, b.first);
        }
      }
    };

    triangle_chunks_type *vertex_chunk;
    std::map<std::pair<double, double>, triangle_chunks_type *, ZRangeCompare> flat_vertex_chunks;
    triangle_chunks_type *normals_chunk;
    line_chunks_type *line_chunk;
    GLfloat fill_color [4];
    GLfloat frame_color [4];
    bool visible;
    std::string name;
    bool has_name;

    triangle_chunks_type *get_flat_vertex_chunks (double zstart, double zstop, std::list<triangle_chunks_type> &flat_vertex_chunk_list)
    {
      auto i = flat_vertex_chunks.find (std::make_pair (zstart, zstop));
      if (i != flat_vertex_chunks.end ()) {
        return i->second;
      }

      flat_vertex_chunk_list.push_back (triangle_chunks_type ());
      return flat_vertex_chunks.insert (std::make_pair (std::make_pair (zstart, zstop), &flat_vertex_chunk_list.back ())).first->second;
    }
  };

  D25ViewWidget (QWidget *parent);
  ~D25ViewWidget ();

  void keyPressEvent (QKeyEvent *event);
  void keyReleaseEvent (QKeyEvent *event);
  void wheelEvent (QWheelEvent *event);
  void mousePressEvent (QMouseEvent *event);
  void mouseReleaseEvent (QMouseEvent *event);
  void mouseMoveEvent (QMouseEvent *event);

  void attach_view(lay::LayoutViewBase *view);

  QVector3D hit_point_with_scene(const QVector3D &line_dir);
  void refresh ();
  void reset ();

  QVector3D displacement () const { return m_displacement; }

  void set_displacement (const QVector3D &d)
  {
    m_displacement = d;
    refresh ();
  }

  QVector3D scale_factors () const
  {
    return QVector3D (scale_factor (), scale_factor () * vscale_factor (), scale_factor ());
  }

  double scale_factor () const { return m_scale_factor; }

  void set_scale_factor (double f)
  {
    m_scale_factor = f;
    refresh ();
  }

  double vscale_factor () const { return m_vscale_factor; }

  void set_vscale_factor (double f)
  {
    m_vscale_factor = f;
    refresh ();
  }

  const std::string &error () const
  {
    return m_error;
  }

  bool has_error () const
  {
    return m_has_error;
  }

  const std::vector<LayerInfo> &layers () const
  {
    return m_layers;
  }

  void set_material_visible (size_t index, bool visible);

  void clear ();
  void open_display (const tl::color_t *frame_color, const tl::color_t *fill_color, const db::LayerProperties *like, const std::string *name);
  void close_display ();
  void entry (const db::Region &data, double dbu, double zstart, double zstop);
  void entry (const db::Edges &data, double dbu, double zstart, double zstop);
  void entry (const db::EdgePairs &data, double dbu, double zstart, double zstop);
  void finish ();

signals:
  void scale_factor_changed (double f);
  void vscale_factor_changed (double f);
  void init_failed ();

protected:
  virtual void camera_changed ();
  virtual double aspect_ratio () const;
  virtual void showEvent (QShowEvent *);

public slots:
  void fit ();

private:
  std::unique_ptr<D25InteractionMode> mp_mode;
  QOpenGLShaderProgram *m_shapes_program, *m_shapes_program_uniform_normals, *m_lines_program, *m_gridplane_program;
  std::string m_error;
  bool m_has_error;
  double m_scale_factor;
  double m_vscale_factor;
  QVector3D m_displacement;
  lay::LayoutViewBase *mp_view;
  db::DBox m_bbox;
  double m_zmin, m_zmax;
  bool m_zset;
  bool m_display_open;

  std::list<triangle_chunks_type> m_vertex_chunks;
  std::list<triangle_chunks_type> m_flat_vertex_chunks;
  std::list<triangle_chunks_type> m_normals_chunks;
  std::list<line_chunks_type> m_line_chunks;

  std::vector<LayerInfo> m_layers;

  void initializeGL ();
  void paintGL ();
  void resizeGL (int w, int h);

  void do_initialize_gl ();
  void render_region (tl::AbsoluteProgress &progress, triangle_chunks_type &flat_chunks, D25ViewWidget::triangle_chunks_type &vertex_chunks, triangle_chunks_type &normals, D25ViewWidget::line_chunks_type &line_chunks, const db::Region &region, double dbu, const db::Box &clip_box, double zstart, double zstop);
  void render_edges (tl::AbsoluteProgress &progress, D25ViewWidget::triangle_chunks_type &vertex_chunks, triangle_chunks_type &normals, D25ViewWidget::line_chunks_type &line_chunks, const db::Edges &region, double dbu, const db::Box &clip_box, double zstart, double zstop);
  void render_edge_pairs (tl::AbsoluteProgress &progress, D25ViewWidget::triangle_chunks_type &vertex_chunks, triangle_chunks_type &normals, D25ViewWidget::line_chunks_type &line_chunks, const db::EdgePairs &region, double dbu, const db::Box &clip_box, double zstart, double zstop);
  void render_polygon (D25ViewWidget::triangle_chunks_type &vertex_chunks, const db::Polygon &poly, double dbu);
  void render_wall (D25ViewWidget::triangle_chunks_type &vertex_chunks, triangle_chunks_type &normals, D25ViewWidget::line_chunks_type &line_chunks, const db::Edge &poly, double dbu, double zstart, double zstop);
  void reset_viewport ();
  void enter (const db::RecursiveShapeIterator *iter, double zstart, double zstop);
};

}

#endif


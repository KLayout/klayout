
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


#include "dbLayout.h"
#include "dbRecursiveShapeIterator.h"
#include "dbBoxConvert.h"
#include "layMarker.h"
#include "laySnap.h"
#include "layCanvasPlane.h"
#include "layViewOp.h"
#include "layRenderer.h"
#include "layLayoutViewBase.h"
#include "layTextInfo.h"
#include "tlAssert.h"

namespace lay
{

static db::DVector text_box_enlargement (const db::DCplxTrans &vp_trans)
{
  //  4.0 is the text box border in pixels
  double b = 4.0 / vp_trans.mag ();
  return db::DVector (b, b);
}

// ------------------------------------------------------------------------

void render_cell_inst (const db::Layout &layout, const db::CellInstArray &inst, const db::CplxTrans &trans, lay::Renderer &r,
                       unsigned int font, lay::CanvasPlane *fill, lay::CanvasPlane *contour, lay::CanvasPlane *vertex, lay::CanvasPlane *text,
                       bool cell_name_text_transform, int min_size_for_label, bool draw_outline, size_t max_shapes)
{
  bool render_origins = false;

  const db::Cell &cell = layout.cell (inst.object ().cell_index ());
  std::string cell_name = layout.display_name (inst.object ().cell_index ());
  db::Box cell_box = cell.bbox ();

  db::Vector a, b;
  unsigned long amax = 0, bmax = 0;
  unsigned long long n = 1;
  if (inst.is_regular_array (a, b, amax, bmax)) {
    n = (unsigned long long) amax * (unsigned long long) bmax;
  }

  bool draw_shapes;

  if (max_shapes > 0) {

    size_t nshapes = 0;
    draw_shapes = true;

    for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers () && draw_shapes; ++l) {
      db::RecursiveShapeIterator shapes (layout, cell, (*l).first); 
      while (draw_shapes && ! shapes.at_end ()) {
        nshapes += n;
        if (nshapes > max_shapes) {
          draw_shapes = false;
        }
        ++shapes;
      }
    }

  } else {
    draw_shapes = false;
  }

  if (draw_outline || ! draw_shapes) {

    if (n > 1000) {

      db::Vector av(a), bv(b);

      //  fallback to simpler representation using a description text
      db::CplxTrans tbox (trans * inst.complex_trans ());

      //  one representative instance
      r.draw (cell_box, tbox, fill, contour, 0, text);
      r.draw (cell_box, db::CplxTrans (trans * (av * long (amax - 1))) * tbox, fill, contour, 0, text);
      r.draw (cell_box, db::CplxTrans (trans * (bv * long (bmax - 1))) * tbox, fill, contour, 0, text);
      r.draw (cell_box, db::CplxTrans (trans * (av * long (amax - 1) + bv * long (bmax - 1))) * tbox, fill, contour, 0, text);

      db::DBox cb (tbox * cell_box);
      db::DPolygon p;
      db::DPoint points[] = {
        db::DPoint (cb.lower_left ()), 
        db::DPoint (cb.lower_left () + trans * (av * long (amax - 1))),
        db::DPoint (cb.lower_left () + trans * (av * long (amax - 1) + bv * long (bmax - 1))),
        db::DPoint (cb.lower_left () + trans * (bv * long (bmax - 1))),
      };
      p.assign_hull (points, points + sizeof (points) / sizeof (points[0]));
      r.draw (p, fill, contour, 0, text);

      if (text) {
        db::DBox arr_box (db::DPoint (), db::DPoint () + trans * (av * long (amax - 1) + bv * long (bmax - 1)));
        arr_box *= cb;
        r.draw (arr_box, tl::sprintf (tl::to_string (tr ("Array %ldx%ld")), amax, bmax), db::Font (font), db::HAlignCenter, db::VAlignCenter, db::DFTrans (db::DFTrans::r0), 0, 0, 0, text);
      }

    } else {

      for (db::CellInstArray::iterator arr = inst.begin (); ! arr.at_end (); ++arr) {

        //  fallback to simpler representation using a description text
        db::CplxTrans tbox (trans * inst.complex_trans ());

        r.draw (cell_box, tbox, fill, contour, 0, 0);

        db::DBox dbox = tbox * cell_box;
        if (text && ! cell_name.empty () && dbox.width () > min_size_for_label && dbox.height () > min_size_for_label) {

          //  Hint: we render to contour because the texts plane is reserved for properties
          r.draw (dbox, cell_name,
                  db::Font (font),
                  db::HAlignCenter,
                  db::VAlignCenter,
                  //  TODO: apply "real" transformation?
                  db::DFTrans (cell_name_text_transform ? tbox.fp_trans ().rot () : db::DFTrans::r0), 0, 0, 0, text);

        }

      }

      render_origins = true;

    }

  }

  //  draw the interiour of the instance if required.
  if (draw_shapes) {

    render_origins = true;

    for (db::Layout::layer_iterator l = layout.begin_layers (); l != layout.end_layers () && draw_shapes; ++l) {

      db::RecursiveShapeIterator shapes (layout, cell, (*l).first); 
      while (! shapes.at_end ()) {

        for (db::CellInstArray::iterator arr = inst.begin (); ! arr.at_end (); ++arr) {
          r.draw (*shapes, trans * inst.complex_trans (*arr) * shapes.trans (), fill, contour, 0 /*use vertex for origin*/, text);
        }

        ++shapes;

      }

    }

  }

  {
    //  render error layer

    db::RecursiveShapeIterator shapes (layout, cell, layout.error_layer ());
    while (! shapes.at_end ()) {

      for (db::CellInstArray::iterator arr = inst.begin (); ! arr.at_end (); ++arr) {
        r.draw (*shapes, trans * inst.complex_trans (*arr) * shapes.trans (), fill, contour, 0 /*use vertex for origin*/, text);
      }

      ++shapes;

    }
  }

  // render the origins
  if (render_origins && vertex) {

    for (db::CellInstArray::iterator arr = inst.begin (); ! arr.at_end (); ++arr) {
      db::DPoint dp = db::DPoint () + (trans * inst.complex_trans (*arr)).disp ();
      r.draw (db::DEdge (dp, dp), 0, 0, vertex, 0);
    }

  }
}

// ------------------------------------------------------------------------

MarkerBase::MarkerBase (lay::LayoutViewBase *view)
  : lay::ViewObject (view->canvas ()),
    m_line_width (-1), m_vertex_size (-1), m_halo (-1), m_text_enabled (true), m_vertex_shape (lay::ViewOp::Rect), m_line_style (-1), m_dither_pattern (-1), m_frame_pattern (0), mp_view (view)
{ 
  // .. nothing yet ..
}

void 
MarkerBase::set_frame_color (tl::Color color)
{
  if (color != m_frame_color) {
    m_frame_color = color;
    redraw ();
  }
}

void 
MarkerBase::set_color (tl::Color color)
{
  if (color != m_color) {
    m_color = color;
    redraw ();
  }
}

void 
MarkerBase::set_line_width (int lw)
{
  if (m_line_width != lw) {
    m_line_width = lw;
    redraw ();
  }
}

void 
MarkerBase::set_vertex_shape (lay::ViewOp::Shape vs)
{
  if (m_vertex_shape != vs) {
    m_vertex_shape = vs;
    redraw ();
  }
}

void 
MarkerBase::set_vertex_size (int vs)
{
  if (m_vertex_size != vs) {
    m_vertex_size = vs;
    redraw ();
  }
}

void 
MarkerBase::set_halo (int halo)
{
  if (m_halo != halo) {
    m_halo = halo;
    redraw ();
  }
}

void
MarkerBase::set_text_enabled (bool en)
{
  if (m_text_enabled != en) {
    m_text_enabled = en;
    redraw ();
  }
}

void
MarkerBase::set_frame_pattern (int frame_pattern)
{
  if (m_frame_pattern != frame_pattern) {
    m_frame_pattern = frame_pattern;
    redraw ();
  }
}

void 
MarkerBase::set_dither_pattern (int dither_pattern)
{
  if (m_dither_pattern != dither_pattern) {
    m_dither_pattern = dither_pattern;
    redraw ();
  }
}

void
MarkerBase::set_line_style (int line_style)
{
  if (m_line_style != line_style) {
    m_line_style = line_style;
    redraw ();
  }
}

void
MarkerBase::get_bitmaps (const Viewport & /*vp*/, ViewObjectCanvas &canvas, lay::CanvasPlane *&fill, lay::CanvasPlane *&contour, lay::CanvasPlane *&vertex, lay::CanvasPlane *&text)
{
  double resolution = canvas.resolution ();
  int basic_width = int(0.5 + 1.0 / resolution);

  //  obtain bitmaps
  tl::Color color = m_color;
  if (! color.is_valid ()) {
    color = mp_view->default_marker_color ();
  }
  if (! color.is_valid ()) {
    color = canvas.foreground_color ();
  }

  tl::Color frame_color = m_frame_color;
  if (! frame_color.is_valid ()) {
    frame_color = color;
  }

  int line_width = m_line_width < 0 ? mp_view->default_marker_line_width () : m_line_width;
  int vertex_size = m_vertex_size < 0 ? mp_view->default_marker_vertex_size () : m_vertex_size;
  bool halo = m_halo < 0 ? mp_view->default_marker_halo () : (m_halo != 0);
  int dither_pattern = m_dither_pattern < 0 ? mp_view->default_dither_pattern () : m_dither_pattern;
  int line_style = m_line_style < 0 ? mp_view->default_line_style () : m_line_style;

  if (halo) {

    std::vector <lay::ViewOp> ops;
    ops.resize (2);

    if (dither_pattern >= 0) {
      ops[0] = lay::ViewOp (canvas.background_color ().rgb (), lay::ViewOp::Copy, 0, (unsigned int) dither_pattern, 0, lay::ViewOp::Rect, 3 * basic_width, 0);
      ops[1] = lay::ViewOp (color.rgb (), lay::ViewOp::Copy, 0, (unsigned int) dither_pattern, 0, lay::ViewOp::Rect, basic_width, 1);
      fill = canvas.plane (ops);
    } else {
      fill = 0;
    }

    ops[0] = lay::ViewOp (canvas.background_color ().rgb (), lay::ViewOp::Copy, (unsigned int) line_style, (unsigned int) m_frame_pattern, 0, lay::ViewOp::Rect, line_width > 0 ? (line_width + 2) * basic_width : 0, 0);
    ops[1] = lay::ViewOp (frame_color.rgb (), lay::ViewOp::Copy, (unsigned int) line_style, (unsigned int) m_frame_pattern, 0, lay::ViewOp::Rect, line_width * basic_width, 1);
    contour = canvas.plane (ops);

    if (! m_text_enabled) {
      text = 0;
    } else if (line_width == 1) {
      text = contour;
    } else {
      ops[0] = lay::ViewOp (canvas.background_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, 3 * basic_width, 0);
      ops[1] = lay::ViewOp (frame_color.rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, basic_width, 1);
      text = canvas.plane (ops);
    }

    if (m_vertex_shape == lay::ViewOp::Rect) { 
      ops[0] = lay::ViewOp (canvas.background_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0, m_vertex_shape, vertex_size > 0 ? (vertex_size + 2) * basic_width : 0, 0);
      ops[1] = lay::ViewOp (frame_color.rgb (), lay::ViewOp::Copy, 0, 0, 0, m_vertex_shape, vertex_size * basic_width, 1);
      vertex = canvas.plane (ops);
    } else {
      std::vector <lay::ViewOp> ops1;
      ops1.resize (1);
      ops1[0] = lay::ViewOp (frame_color.rgb (), lay::ViewOp::Copy, 0, 0, 0, m_vertex_shape, vertex_size * basic_width, 1);
      vertex = canvas.plane (ops1);
    }

  } else {

    if (dither_pattern >= 0) {
      fill = canvas.plane (lay::ViewOp (color.rgb (), lay::ViewOp::Copy, 0, (unsigned int) dither_pattern, 0, lay::ViewOp::Rect, basic_width));
    } else {
      fill = 0;
    }

    contour = canvas.plane (lay::ViewOp (frame_color.rgb (), lay::ViewOp::Copy, (unsigned int) line_style, (unsigned int) m_frame_pattern, 0, lay::ViewOp::Rect, line_width * basic_width));
    vertex = canvas.plane (lay::ViewOp (frame_color.rgb (), lay::ViewOp::Copy, 0, 0, 0, m_vertex_shape, vertex_size * basic_width));
    if (! m_text_enabled) {
      text = 0;
    } else if (line_width == 1) {
      text = contour;
    } else {
      text = canvas.plane (lay::ViewOp (frame_color.rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, basic_width));
    }

  }
}

// ------------------------------------------------------------------------

GenericMarkerBase::GenericMarkerBase (lay::LayoutViewBase *view, unsigned int cv_index)
  : MarkerBase (view), mp_trans_vector (0), mp_view (view), m_cv_index (cv_index)
{ 
  // .. nothing yet ..
}

GenericMarkerBase::~GenericMarkerBase ()
{
  if (mp_trans_vector) {
    delete mp_trans_vector;
    mp_trans_vector = 0;
  }
}

void
GenericMarkerBase::set_trans (const db::CplxTrans &trans)
{
  if (! m_trans.equal (trans)) {
    m_trans = trans;
    redraw ();
  }
}

void 
GenericMarkerBase::set (const db::ICplxTrans &t1)
{
  if (mp_trans_vector) {
    delete mp_trans_vector;
    mp_trans_vector = 0;
  }
  m_trans = db::CplxTrans (dbu ()) * t1;
  redraw ();
}

void
GenericMarkerBase::set (const db::DCplxTrans &t1)
{
  if (mp_trans_vector) {
    delete mp_trans_vector;
    mp_trans_vector = 0;
  }
  //  Note: this cast is not really correct but we handle float and integer types in the same fashion now.
  m_trans = db::CplxTrans (db::DCplxTrans (dbu ()) * t1);
  redraw ();
}

void
GenericMarkerBase::set (const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  if (mp_trans_vector) {
    delete mp_trans_vector;
    mp_trans_vector = 0;
  }
  if (trans.size () == 1) {
    m_trans = trans [0] * db::CplxTrans (dbu ()) * t1;
  } else {
    m_trans = db::CplxTrans (dbu ()) * t1;
    mp_trans_vector = new std::vector<db::DCplxTrans> (trans);
  }
  redraw ();
}

void
GenericMarkerBase::set (const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  if (mp_trans_vector) {
    delete mp_trans_vector;
    mp_trans_vector = 0;
  }
  if (trans.size () == 1) {
    //  Note: this cast is not really correct but we handle float and integer types in the same fashion now.
    m_trans = db::CplxTrans (trans [0] * db::DCplxTrans (dbu ()) * t1);
  } else {
    //  Note: this cast is not really correct but we handle float and integer types in the same fashion now.
    m_trans = db::CplxTrans (db::DCplxTrans (dbu ()) * t1);
    mp_trans_vector = new std::vector<db::DCplxTrans> (trans);
  }
  redraw ();
}

db::DBox
GenericMarkerBase::bbox () const
{
  const lay::CellView &cv = mp_view->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return db::DBox ();
  }
  
  if (mp_trans_vector) {
    db::DBox b;
    db::DBox ib = item_bbox ();
    for (std::vector<db::DCplxTrans>::const_iterator t = mp_trans_vector->begin (); t != mp_trans_vector->end (); ++t) {
      b += (*t * db::DCplxTrans (m_trans)) * ib;
    }
    return b;
  } else {
    return db::DCplxTrans (m_trans) * item_bbox ();
  }
}

const db::Layout *
GenericMarkerBase::layout () const
{
  if (m_cv_index >= (unsigned int) (mp_view->cellviews ())) {
    return 0;
  }

  const lay::CellView &cv = mp_view->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return 0;
  } else {
    return &cv->layout ();
  }
}

double
GenericMarkerBase::dbu () const
{
  const db::Layout *ly = layout ();
  return ly ? ly->dbu () : 1.0;
}

// ------------------------------------------------------------------------

InstanceMarker::InstanceMarker (LayoutViewBase *view, unsigned int cv_index, bool draw_outline, size_t max_shapes)
  : GenericMarkerBase (view, cv_index), m_draw_outline (draw_outline), m_max_shapes (max_shapes), m_inst ()
{ 
  // .. nothing yet ..
}

InstanceMarker::~InstanceMarker ()
{
  // .. nothing yet ..
}

void 
InstanceMarker::set (const db::Instance &instance, const db::ICplxTrans &trans)
{
  m_inst = instance;
  GenericMarkerBase::set (trans);
}

void 
InstanceMarker::set (const db::Instance &instance, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  m_inst = instance;
  GenericMarkerBase::set (t1, trans);
}

void 
InstanceMarker::render (const Viewport &vp, ViewObjectCanvas &canvas)
{
  const db::Layout *ly = layout ();
  if (! ly) {
    return;
  }

  lay::CanvasPlane *fill, *contour, *vertex, *text;
  get_bitmaps (vp, canvas, fill, contour, vertex, text);
  if (contour == 0 && vertex == 0 && fill == 0 && text == 0) {
    return;
  }

  lay::Renderer &r = canvas.renderer ();
  bool label_transform = view ()->cell_box_text_transform ();
  int min_size = view ()->min_inst_label_size ();

  db::box_convert<db::CellInst> bc (*ly);

  if (! trans_vector ()) {
    render_cell_inst (*ly, m_inst.cell_inst (), vp.trans () * trans (), r, view ()->cell_box_text_font (), fill, contour, vertex, text, label_transform, min_size, m_draw_outline, m_max_shapes);
  } else {
    for (std::vector<db::DCplxTrans>::const_iterator tr = trans_vector ()->begin (); tr != trans_vector ()->end (); ++tr) {
      render_cell_inst (*ly, m_inst.cell_inst (), vp.trans () * *tr * trans (), r, view ()->cell_box_text_font (), fill, contour, vertex, text, label_transform, min_size, m_draw_outline, m_max_shapes);
    }
  }
}

void 
InstanceMarker::set_draw_outline (bool d)
{
  if (d != m_draw_outline) {
    m_draw_outline = d;
    redraw ();
  }
}

void 
InstanceMarker::set_max_shapes (size_t s)
{
  if (s != m_max_shapes) {
    m_max_shapes = s;
    redraw ();
  }
}

db::DBox
InstanceMarker::item_bbox () const 
{
  return db::DBox (m_inst.bbox ());
}

// ------------------------------------------------------------------------

ShapeMarker::ShapeMarker (LayoutViewBase *view, unsigned int cv_index)
  : GenericMarkerBase (view, cv_index), m_shape ()
{ 
  // .. nothing yet ..
}

ShapeMarker::~ShapeMarker ()
{
  // .. nothing yet ..
}

void 
ShapeMarker::set (const db::Shape &shape, const db::ICplxTrans &trans)
{
  m_shape = shape;
  GenericMarkerBase::set (trans);
}

void 
ShapeMarker::set (const db::Shape &shape, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  m_shape = shape;
  GenericMarkerBase::set (t1, trans);
}

void 
ShapeMarker::render (const Viewport &vp, ViewObjectCanvas &canvas)
{
  const db::Layout *ly = layout ();
  if (! ly) {
    return;
  }

  lay::CanvasPlane *fill, *contour, *vertex, *text; 
  get_bitmaps (vp, canvas, fill, contour, vertex, text);
  if (contour == 0 && vertex == 0 && fill == 0 && text == 0) {
    return;
  }

  lay::Renderer &r = canvas.renderer ();

  r.set_font (db::Font (view ()->text_font ()));
  r.apply_text_trans (view ()->apply_text_trans ());
  r.default_text_size (db::Coord (view ()->default_text_size () / ly->dbu ()));
  r.set_precise (true);

  if (trans_vector ()) {
    for (std::vector<db::DCplxTrans>::const_iterator tr = trans_vector ()->begin (); tr != trans_vector ()->end (); ++tr) {
      db::CplxTrans t = vp.trans () * *tr * trans ();
      if (m_shape.is_text () && text) {
        //  draw a frame around the text
        lay::TextInfo ti (view ());
        db::DCplxTrans vp_trans = vp.trans () * *tr;
        db::Text t;
        m_shape.text (t);
        db::DBox box = ti.bbox (trans () * t, vp_trans).enlarged (text_box_enlargement (vp_trans));
        if (! box.is_point ()) {
          r.draw (box, vp_trans, 0, text, 0, 0);
        }
      }
      r.draw (m_shape, t, fill, contour, vertex, text);
      r.draw_propstring (m_shape, &ly->properties_repository (), text, t);
    }
  } else {
    db::CplxTrans t = vp.trans () * trans ();
    if (m_shape.is_text () && text) {
      //  draw a frame around the text
      lay::TextInfo ti (view ());
      db::Text t;
      m_shape.text (t);
      db::DBox box = ti.bbox (trans () * t, vp.trans ()).enlarged (text_box_enlargement (vp.trans ()));
      if (! box.is_point ()) {
        r.draw (box, vp.trans (), 0, text, 0, 0);
      }
    }
    r.draw (m_shape, t, fill, contour, vertex, text);
    r.draw_propstring (m_shape, &ly->properties_repository (), text, t);
  }
}

db::DBox
ShapeMarker::item_bbox () const 
{
  return db::DBox (m_shape.bbox ());
}

// ------------------------------------------------------------------------

Marker::Marker (lay::LayoutViewBase *view, unsigned int cv_index, bool draw_outline, size_t max_shapes)
  : GenericMarkerBase (view, cv_index), m_draw_outline (draw_outline), m_max_shapes (max_shapes) 
{ 
  m_type = None;
  m_object.any = 0;
}

Marker::~Marker ()
{
  remove_object ();
}

void
Marker::set ()
{
  remove_object ();
  redraw ();
}

void 
Marker::set (const db::Box &box, const db::ICplxTrans &trans)
{
  remove_object ();

  m_type = Box;
  m_object.box = new db::Box (box);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::DBox &box, const db::DCplxTrans &trans)
{
  remove_object ();

  m_type = DBox;
  m_object.dbox = new db::DBox (box);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::DBox &box, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = DBox;
  m_object.dbox = new db::DBox (box);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::Box &box, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = Box;
  m_object.box = new db::Box (box);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::Polygon &poly, const db::ICplxTrans &trans)
{
  remove_object ();

  m_type = Polygon;
  m_object.polygon = new db::Polygon (poly);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::Polygon &poly, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = Polygon;
  m_object.polygon = new db::Polygon (poly);

  GenericMarkerBase::set (t1, trans);
}

void
Marker::set (const db::PolygonRef &poly, const db::ICplxTrans &trans)
{
  remove_object ();

  m_type = PolygonRef;
  m_object.polygon_ref = new db::PolygonRef (poly);

  GenericMarkerBase::set (trans);
}

void
Marker::set (const db::PolygonRef &poly, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = PolygonRef;
  m_object.polygon_ref = new db::PolygonRef (poly);

  GenericMarkerBase::set (t1, trans);
}

void
Marker::set (const db::DPolygon &poly, const db::DCplxTrans &trans)
{
  remove_object ();

  m_type = DPolygon;
  m_object.dpolygon = new db::DPolygon (poly);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::DPolygon &poly, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = DPolygon;
  m_object.dpolygon = new db::DPolygon (poly);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::EdgePair &edge_pair, const db::ICplxTrans &trans)
{
  remove_object ();

  m_type = EdgePair;
  m_object.edge_pair = new db::EdgePair (edge_pair);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::EdgePair &edge_pair, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = EdgePair;
  m_object.edge_pair = new db::EdgePair (edge_pair);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::DEdgePair &edge_pair, const db::DCplxTrans &trans)
{
  remove_object ();

  m_type = DEdgePair;
  m_object.dedge_pair = new db::DEdgePair (edge_pair);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::DEdgePair &edge_pair, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = DEdgePair;
  m_object.dedge_pair = new db::DEdgePair (edge_pair);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::Edge &edge, const db::ICplxTrans &trans)
{
  remove_object ();

  m_type = Edge;
  m_object.edge = new db::Edge (edge);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::Edge &edge, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = Edge;
  m_object.edge = new db::Edge (edge);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::DEdge &edge, const db::DCplxTrans &trans)
{
  remove_object ();

  m_type = DEdge;
  m_object.dedge = new db::DEdge (edge);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::DEdge &edge, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = DEdge;
  m_object.dedge = new db::DEdge (edge);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::Path &path, const db::ICplxTrans &trans)
{
  remove_object ();

  m_type = Path;
  m_object.path = new db::Path (path);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::Path &path, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = Path;
  m_object.path = new db::Path (path);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::DPath &path, const db::DCplxTrans &trans)
{
  remove_object ();

  m_type = DPath;
  m_object.dpath = new db::DPath (path);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::DPath &path, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = DPath;
  m_object.dpath = new db::DPath (path);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::Text &text, const db::ICplxTrans &trans)
{
  remove_object ();

  m_type = Text;
  m_object.text = new db::Text (text);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::Text &text, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = Text;
  m_object.text = new db::Text (text);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::DText &text, const db::DCplxTrans &trans)
{
  remove_object ();

  m_type = DText;
  m_object.dtext = new db::DText (text);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::DText &text, const db::DCplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = DText;
  m_object.dtext = new db::DText (text);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set (const db::CellInstArray &instance, const db::ICplxTrans &trans)
{
  remove_object ();

  m_type = Instance;
  m_object.inst = new db::CellInstArray (instance);

  GenericMarkerBase::set (trans);
}

void 
Marker::set (const db::CellInstArray &instance, const db::ICplxTrans &t1, const std::vector<db::DCplxTrans> &trans)
{
  remove_object ();

  m_type = Instance;
  m_object.inst = new db::CellInstArray (instance);

  GenericMarkerBase::set (t1, trans);
}

void 
Marker::set_draw_outline (bool d)
{
  if (d != m_draw_outline) {
    m_draw_outline = d;
    redraw ();
  }
}

void 
Marker::set_max_shapes (size_t s)
{
  if (s != m_max_shapes) {
    m_max_shapes = s;
    redraw ();
  }
}

db::DBox
Marker::item_bbox () const
{
  if (m_type == Box) {
    return db::DBox (*m_object.box);
  } else if (m_type == DBox) {
    return *m_object.dbox;
  } else if (m_type == Polygon) {
    return db::DBox (m_object.polygon->box ());
  } else if (m_type == PolygonRef) {
    return db::DBox (m_object.polygon_ref->box ());
  } else if (m_type == DPolygon) {
    return m_object.dpolygon->box ();
  } else if (m_type == EdgePair) {
    return db::DBox (m_object.edge_pair->bbox ());
  } else if (m_type == DEdgePair) {
    return m_object.dedge_pair->bbox ();
  } else if (m_type == Edge) {
    return db::DBox (m_object.edge->bbox ());
  } else if (m_type == DEdge) {
    return m_object.dedge->bbox ();
  } else if (m_type == Path) {
    return db::DBox (m_object.path->box ());
  } else if (m_type == DPath) {
    return m_object.dpath->box ();
  } else if (m_type == Text) {
    return db::DBox (m_object.text->box ());
  } else if (m_type == DText) {
    return m_object.dtext->box ();
  } else if (m_type == Instance) {
    const db::Layout *ly = layout ();
    if (ly) {
      return db::DBox (m_object.inst->bbox (db::box_convert <db::CellInst> (*ly)));
    }
  }
  return db::DBox ();
}

void
Marker::remove_object ()
{
  if (m_type == Box) {
    delete m_object.box;
  } else if (m_type == DBox) {
    delete m_object.dbox;
  } else if (m_type == Polygon) {
    delete m_object.polygon;
  } else if (m_type == PolygonRef) {
    delete m_object.polygon_ref;
  } else if (m_type == DPolygon) {
    delete m_object.dpolygon;
  } else if (m_type == EdgePair) {
    delete m_object.edge_pair;
  } else if (m_type == DEdgePair) {
    delete m_object.dedge_pair;
  } else if (m_type == Edge) {
    delete m_object.edge;
  } else if (m_type == DEdge) {
    delete m_object.dedge;
  } else if (m_type == Path) {
    delete m_object.path;
  } else if (m_type == DPath) {
    delete m_object.dpath;
  } else if (m_type == Text) {
    delete m_object.text;
  } else if (m_type == DText) {
    delete m_object.dtext;
  } else if (m_type == Instance) {
    delete m_object.inst;
  } 

  m_type = None;
  m_object.any = 0;
}

void 
Marker::draw (lay::Renderer &r, const db::CplxTrans &t, lay::CanvasPlane *fill, lay::CanvasPlane *contour, lay::CanvasPlane *vertex, lay::CanvasPlane *text)
{
  if (m_type == Box) {
    r.draw (*m_object.box, t, fill, contour, vertex, text);
  } else if (m_type == DBox) {
    r.draw (*m_object.dbox, db::DCplxTrans (t), fill, contour, vertex, text);
  } else if (m_type == Polygon) {
    r.draw (*m_object.polygon, t, fill, contour, vertex, text);
  } else if (m_type == PolygonRef) {
    r.draw (m_object.polygon_ref->obj (), t * db::ICplxTrans (m_object.polygon_ref->trans ()), fill, contour, vertex, text);
  } else if (m_type == DPolygon) {
    r.draw (*m_object.dpolygon, db::DCplxTrans (t), fill, contour, vertex, text);
  } else if (m_type == Path) {
    r.draw (*m_object.path, t, fill, contour, vertex, text);
  } else if (m_type == DPath) {
    r.draw (*m_object.dpath, db::DCplxTrans (t), fill, contour, vertex, text);
  } else if (m_type == Text) {
    //  TODO: in order to draw the box we'd need a separation of dbu-to-micron and micron-to-pixel transformations ...
    r.draw (*m_object.text, t, fill, contour, vertex, text);
  } else if (m_type == DText) {
    if (view () && text) {
      //  draw a frame around the text
      lay::TextInfo ti (view ());
      db::DCplxTrans dt (t);
      db::DBox box = ti.bbox (*m_object.dtext, dt).enlarged (text_box_enlargement (dt));
      if (! box.is_point ()) {
        r.draw (box, dt, 0, text, 0, 0);
      }
    }
    r.draw (*m_object.dtext, db::DCplxTrans (t), fill, contour, vertex, text);
  } else if (m_type == Edge) {
    r.draw (*m_object.edge, t, fill, contour, vertex, text);
  } else if (m_type == DEdge) {
    r.draw (*m_object.dedge, db::DCplxTrans (t), fill, contour, vertex, text);
  } else if (m_type == EdgePair) {
    r.draw (m_object.edge_pair->first (), t, fill, contour, vertex, text);
    r.draw (m_object.edge_pair->second (), t, fill, contour, vertex, text);
    db::Polygon poly = m_object.edge_pair->normalized ().to_polygon (0);
    r.draw (poly, t, fill, 0, 0, 0);
  } else if (m_type == DEdgePair) {
    r.draw (m_object.dedge_pair->first (), db::DCplxTrans (t), fill, contour, vertex, text);
    r.draw (m_object.dedge_pair->second (), db::DCplxTrans (t), fill, contour, vertex, text);
    db::DPolygon poly = m_object.dedge_pair->normalized ().to_polygon (0);
    r.draw (poly, db::DCplxTrans (t), fill, 0, 0, 0);
  } else if (m_type == Instance) {
    const lay::CellView &cv = view ()->cellview (cv_index ());
    bool label_transform = view ()->cell_box_text_transform ();
    int min_size = view ()->min_inst_label_size ();
    render_cell_inst (cv->layout (), *m_object.inst, t, r, view ()->cell_box_text_font (), fill, contour, vertex, text, label_transform, min_size, m_draw_outline, m_max_shapes);
  }
}

void 
Marker::render (const Viewport &vp, ViewObjectCanvas &canvas)
{ 
  lay::CanvasPlane *fill, *contour, *vertex, *text; 
  get_bitmaps (vp, canvas, fill, contour, vertex, text);
  if (contour == 0 && vertex == 0 && fill == 0 && text == 0) {
    return;
  }

  lay::Renderer &r = canvas.renderer ();

  r.set_font (db::Font (view ()->text_font ()));
  r.apply_text_trans (view ()->apply_text_trans ());
  r.default_text_size (db::Coord (view ()->default_text_size () / dbu ()));
  r.set_precise (true);

  if (! trans_vector ()) {
    db::CplxTrans t = vp.trans () * trans ();
    draw (r, t, fill, contour, vertex, text);
  } else {
    for (std::vector<db::DCplxTrans>::const_iterator tr = trans_vector ()->begin (); tr != trans_vector ()->end (); ++tr) {
      db::CplxTrans t = vp.trans () * *tr * trans ();
      draw (r, t, fill, contour, vertex, text);
    }
  }
}

// ------------------------------------------------------------------------

DMarker::DMarker (LayoutViewBase *view)
  : MarkerBase (view), mp_view (view)
{ 
  m_type = None;
  m_object.any = 0;
}

DMarker::~DMarker ()
{
  remove_object ();
}

void 
DMarker::set (const db::DBox &box)
{
  remove_object ();

  m_type = Box;
  m_object.box = new db::DBox (box);

  redraw ();
}

void 
DMarker::set (const db::DPolygon &poly)
{
  remove_object ();

  m_type = Polygon;
  m_object.polygon = new db::DPolygon (poly);

  redraw ();
}

void 
DMarker::set (const db::DEdgePair &edge_pair)
{
  remove_object ();

  m_type = EdgePair;
  m_object.edge_pair = new db::DEdgePair (edge_pair);

  redraw ();
}

void 
DMarker::set (const db::DEdge &edge)
{
  remove_object ();

  m_type = Edge;
  m_object.edge = new db::DEdge (edge);

  redraw ();
}

void 
DMarker::set (const db::DPath &path)
{
  remove_object ();

  m_type = Path;
  m_object.path = new db::DPath (path);

  redraw ();
}

void 
DMarker::set (const db::DText &text)
{
  remove_object ();

  m_type = Text;
  m_object.text = new db::DText (text);

  redraw ();
}

db::DBox
DMarker::bbox () const
{
  if (m_type == Box) {
    return *m_object.box;
  } else if (m_type == Polygon) {
    return m_object.polygon->box ();
  } else if (m_type == Edge) {
    return m_object.edge->bbox ();
  } else if (m_type == EdgePair) {
    return m_object.edge_pair->bbox ();
  } else if (m_type == Path) {
    return m_object.path->box ();
  } else if (m_type == Text) {
    return m_object.text->box ();
  } else {
    return db::DBox ();
  }
}

void
DMarker::remove_object ()
{
  if (m_type == Box) {
    delete m_object.box;
  } else if (m_type == Polygon) {
    delete m_object.polygon;
  } else if (m_type == Edge) {
    delete m_object.edge;
  } else if (m_type == EdgePair) {
    delete m_object.edge_pair;
  } else if (m_type == Path) {
    delete m_object.path;
  } else if (m_type == Text) {
    delete m_object.text;
  } 

  m_type = None;
  m_object.any = 0;
}

void 
DMarker::render (const Viewport &vp, ViewObjectCanvas &canvas)
{ 
  lay::CanvasPlane *fill, *contour, *vertex, *text; 
  get_bitmaps (vp, canvas, fill, contour, vertex, text);
  if (contour == 0 && vertex == 0 && fill == 0 && text == 0) {
    return;
  }

  lay::Renderer &r = canvas.renderer ();

  r.set_font (db::Font (mp_view->text_font ()));
  r.apply_text_trans (mp_view->apply_text_trans ());
  r.default_text_size (mp_view->default_text_size ());
  r.set_precise (true);

  db::DCplxTrans t = vp.trans ();

  if (m_type == Box) {
    r.draw (*m_object.box, t, fill, contour, vertex, text);
  } else if (m_type == Polygon) {
    r.draw (*m_object.polygon, t, fill, contour, vertex, text);
  } else if (m_type == Path) {
    r.draw (*m_object.path, t, fill, contour, vertex, text);
  } else if (m_type == Text) {
    if (view () && text) {
      //  draw a frame around the text
      lay::TextInfo ti (view ());
      db::DBox box = ti.bbox (*m_object.text, t).enlarged (text_box_enlargement (t));
      if (! box.is_point ()) {
        r.draw (box, t, 0, text, 0, 0);
      }
    }
    r.draw (*m_object.text, t, fill, contour, vertex, text);
  } else if (m_type == Edge) {
    r.draw (*m_object.edge, t, fill, contour, vertex, text);
  } else if (m_type == EdgePair) {
    r.draw (m_object.edge_pair->first (), t, fill, contour, vertex, text);
    r.draw (m_object.edge_pair->second (), t, fill, contour, vertex, text);
    db::DPolygon poly = m_object.edge_pair->normalized ().to_polygon (0);
    r.draw (poly, t, fill, 0, 0, 0);
  }

}

}


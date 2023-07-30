
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


#include "layBitmapRenderer.h"
#include "layBitmap.h"

namespace lay
{

// ----------------------------------------------------------------------------------------------
//  BitmapRenderer implementation

BitmapRenderer::BitmapRenderer (unsigned int width, unsigned int height, double resolution)
  : Renderer (width, height, resolution),
    m_xmin (0.0), m_xmax (0.0), m_ymin (0.0), m_ymax (0.0),
    m_ortho (true)
{
  // .. nothing else ..
}

void 
BitmapRenderer::reserve_edges (size_t n)
{
  m_edges.reserve (n);
}

void 
BitmapRenderer::reserve_texts (size_t n)
{
  m_texts.reserve (n);
}

void 
BitmapRenderer::clear ()
{
  //  this implementation is efficient but does not free memory - 
  //  the idea is to let the BitmapRenderer object manage its workspace.
  m_edges.erase (m_edges.begin (), m_edges.end ());
  //  might be manhattan
  m_ortho = true;
  //  see above
  m_texts.erase (m_texts.begin (), m_texts.end ());
}

void 
BitmapRenderer::insert (const db::DBox &box, const std::string &text, db::Font font, db::HAlign halign, db::VAlign valign, db::DFTrans trans)
{
  m_texts.push_back (lay::RenderText ());
  m_texts.back ().b = box;
  m_texts.back ().text = text;
  m_texts.back ().font = font;
  m_texts.back ().halign = halign;
  m_texts.back ().valign = valign;
  m_texts.back ().trans = trans;
}

void 
BitmapRenderer::draw (const db::DBox &box, const std::string &text, db::Font font, db::HAlign halign, db::VAlign valign, db::DFTrans trans, 
                     lay::CanvasPlane * /*fill*/, lay::CanvasPlane * /*frame*/, lay::CanvasPlane * /*vertices*/, lay::CanvasPlane *texts)
{
  clear ();
  insert (box, text, font, halign, valign, trans);
  if (texts) {
    render_texts (*texts);
  }
}

void 
BitmapRenderer::insert (const db::Box &b, const db::CplxTrans &t)
{
  if (t.is_ortho ()) {
    insert (t * b);
  } else {
    insert (t * db::Edge (b.p1 (), db::Point (b.p1 ().x (), b.p2 ().y ())));
    insert (t * db::Edge (db::Point (b.p1 ().x (), b.p2 ().y ()), b.p2 ()));
    insert (t * db::Edge (b.p2 (), db::Point (b.p2 ().x (), b.p1 ().y ())));
    insert (t * db::Edge (db::Point (b.p2 ().x (), b.p1 ().y ()), b.p1 ()));
  }
}

void 
BitmapRenderer::insert (const db::DBox &b, const db::DCplxTrans &t)
{
  if (t.is_ortho ()) {
    insert (t * b);
  } else {
    insert (t * db::DEdge (b.p1 (), db::DPoint (b.p1 ().x (), b.p2 ().y ())));
    insert (t * db::DEdge (db::DPoint (b.p1 ().x (), b.p2 ().y ()), b.p2 ()));
    insert (t * db::DEdge (b.p2 (), db::DPoint (b.p2 ().x (), b.p1 ().y ())));
    insert (t * db::DEdge (db::DPoint (b.p2 ().x (), b.p1 ().y ()), b.p1 ()));
  }
}

void 
BitmapRenderer::insert (const db::DBox &b)
{
  db::DEdge edges [] = {
    db::DEdge (b.p1 (), db::DPoint (b.p1 ().x (), b.p2 ().y ())),
    db::DEdge (db::DPoint (b.p1 ().x (), b.p2 ().y ()), b.p2 ()),
    db::DEdge (b.p2 (), db::DPoint (b.p2 ().x (), b.p1 ().y ())),
    db::DEdge (db::DPoint (b.p2 ().x (), b.p1 ().y ()), b.p1 ())
  };

  if (m_edges.begin () == m_edges.end ()) {
    m_xmin = b.left ();
    m_xmax = b.right ();
    m_ymin = b.bottom ();
    m_ymax = b.top ();
  } else {
    m_xmin = std::min (m_xmin, b.left ());
    m_xmax = std::max (m_xmax, b.right ());
    m_ymin = std::min (m_ymin, b.bottom ());
    m_ymax = std::max (m_ymax, b.top ());
  }

  m_edges.insert (m_edges.end (), edges, edges + 4);
}

void 
BitmapRenderer::insert (const db::DEdge &e)
{
  if (m_edges.begin () == m_edges.end ()) {
    m_xmin = std::min (e.x1 (), e.x2 ());
    m_xmax = std::max (e.x1 (), e.x2 ());
    m_ymin = std::min (e.y1 (), e.y2 ());
    m_ymax = std::max (e.y1 (), e.y2 ());
  } else {
    m_xmin = std::min (m_xmin, std::min (e.x1 (), e.x2 ()));
    m_xmax = std::max (m_xmax, std::max (e.x1 (), e.x2 ()));
    m_ymin = std::min (m_ymin, std::min (e.y1 (), e.y2 ()));
    m_ymax = std::max (m_ymax, std::max (e.y1 (), e.y2 ()));
  }

  //  check, if the edge is neither horizontal nor vertical - 
  //  reset the orthogonal flag in this case.
  if (m_ortho && fabs (e.x1 () - e.x2 ()) > render_epsilon 
              && fabs (e.y1 () - e.y2 ()) > render_epsilon) {
    m_ortho = false;
  }

  m_edges.push_back (e);
}

static inline bool point_inside_box (const db::DPoint &pt, const db::DBox &box)
{
  return (! (db::coord_traits<db::DBox::coord_type>::equal (pt.x (), box.left ()) || db::coord_traits<db::DBox::coord_type>::equal (pt.x (), box.right ())) &&
          ! (db::coord_traits<db::DBox::coord_type>::equal (pt.y (), box.bottom ()) || db::coord_traits<db::DBox::coord_type>::equal (pt.y (), box.top ())));
}

void
BitmapRenderer::add_xfill ()
{
  db::DBox box;
  for (std::vector<lay::RenderEdge>::const_iterator e = m_edges.begin (); e != m_edges.end (); ++e) {
    if (! e->is_ortho ()) {
      return;
    }
    box += e->p1 ();
    box += e->p2 ();
  }

  if (! box.empty () && box.area () > 0.0) {

    for (std::vector<lay::RenderEdge>::const_iterator e = m_edges.begin (); e != m_edges.end (); ++e) {
      if (point_inside_box (e->p1 (), box) || point_inside_box (e->p2 (), box)) {
        return;
      }
    }

    insert (db::DEdge (box.p1 (), box.p2 ()));
    insert (db::DEdge (box.lower_right (), box.upper_left ()));

  }
}

void 
BitmapRenderer::render_texts (lay::CanvasPlane &plane)
{
  lay::Bitmap *bitmap = static_cast<lay::Bitmap *> (&plane);
  for (std::vector<lay::RenderText>::const_iterator t = m_texts.begin (); t != m_texts.end (); ++t) {
    bitmap->render_text (*t);
  }
}

void 
BitmapRenderer::render_vertices (lay::CanvasPlane &plane, int mode)
{
  lay::Bitmap *bitmap = static_cast<lay::Bitmap *> (&plane);

  //  a basic shortcut if there are no edges to render
  if (m_edges.end () == m_edges.begin ()) {
    return;
  }

  //  basic optimization: just a dot
  if (floor (m_xmax + 0.5) == floor (m_xmin + 0.5) &&
      floor (m_ymax + 0.5) == floor (m_ymin + 0.5)) {
    if (m_xmin > -0.5 && m_ymin > -0.5 && 
        m_xmin < double (bitmap->width ()) - 0.5 &&
        m_ymin < double (bitmap->height ()) - 0.5) {
      unsigned int yint = (unsigned int) (m_ymin + 0.5);
      unsigned int xint = (unsigned int) (m_xmin + 0.5);
      bitmap->fill (yint, xint, xint + 1);
    }
    return;
  }

  bitmap->render_vertices (m_edges, mode);
}

void 
BitmapRenderer::render_contour (lay::CanvasPlane &plane)
{
  lay::Bitmap *bitmap = static_cast<lay::Bitmap *> (&plane);

  //  a basic shortcut if there are no edges to render
  if (m_edges.end () == m_edges.begin ()) {
    return;
  }

  //  basic optimizations: outside window
  if (m_xmax < -0.5 || m_xmin > bitmap->width () - 0.5 ||
      m_ymax < -0.5 || m_ymin > bitmap->height () - 0.5) {
    return;
  }

  //  basic optimization: just a line or a dot
  if (floor (m_xmax + 0.5) == floor (m_xmin + 0.5)) {
    unsigned int y1int = (unsigned int) (std::max (0.0, std::min (m_ymin + 0.5, double (bitmap->height () - 1))));
    unsigned int y2int = (unsigned int) (std::max (0.0, std::min (m_ymax + 0.5, double (bitmap->height () - 1))));
    unsigned int xint = (unsigned int) (std::max (0.0, std::min (m_xmin + 0.5, double (bitmap->width () - 1))));
    for (unsigned int y = y1int; y <= y2int; ++y) {
      bitmap->fill (y, xint, xint + 1);
    }
    return;
  }
    
  if (floor (m_ymax + 0.5) == floor (m_ymin + 0.5)) {
    unsigned int x1int = (unsigned int) (std::max (0.0, std::min (m_xmin + 0.5, double (bitmap->width () - 1))));
    unsigned int x2int = (unsigned int) (std::max (0.0, std::min (m_xmax + 0.5, double (bitmap->width () - 1))));
    unsigned int yint = (unsigned int) (std::max (0.0, std::min (m_ymin + 0.5, double (bitmap->height () - 1))));
    bitmap->fill (yint, x1int, x2int + 1);
    return;
  }
  
  if (! m_ortho) {
    bitmap->render_contour (m_edges);
  } else {
    bitmap->render_contour_ortho (m_edges);
  }
}

void 
BitmapRenderer::render_fill (lay::CanvasPlane &plane)
{
  lay::Bitmap *bitmap = static_cast<lay::Bitmap *> (&plane);

  //  a basic shortcut if there are no edges to render
  if (m_edges.end () == m_edges.begin ()) {
    return;
  }

  //  basic optimizations: outside window
  if (m_xmax < -0.5 || m_xmin > bitmap->width () - 0.5 ||
      m_ymax < -0.5 || m_ymin > bitmap->height () - 0.5) {
    return;
  }

  //  basic optimization: just a line or a dot
  if (floor (m_xmax + 0.5) == floor (m_xmin + 0.5)) {
    unsigned int y1int = (unsigned int) (std::max (0.0, std::min (m_ymin + 0.5, double (bitmap->height () - 1))));
    unsigned int y2int = (unsigned int) (std::max (0.0, std::min (m_ymax + 0.5, double (bitmap->height () - 1))));
    unsigned int xint = (unsigned int) (std::max (0.0, std::min (m_xmin + 0.5, double (bitmap->width () - 1))));
    for (unsigned int y = y1int; y <= y2int; ++y) {
      bitmap->fill (y, xint, xint + 1);
    }
    return;
  }
    
  if (floor (m_ymax + 0.5) == floor (m_ymin + 0.5)) {
    unsigned int x1int = (unsigned int) (std::max (0.0, std::min (m_xmin + 0.5, double (bitmap->width () - 1))));
    unsigned int x2int = (unsigned int) (std::max (0.0, std::min (m_xmax + 0.5, double (bitmap->width () - 1))));
    unsigned int yint = (unsigned int) (std::max (0.0, std::min (m_ymin + 0.5, double (bitmap->height () - 1))));
    bitmap->fill (yint, x1int, x2int + 1);
    return;
  }

  if (m_ortho) {
    bitmap->render_fill_ortho (m_edges);
  } else {
    bitmap->render_fill (m_edges);
  }
}

void 
BitmapRenderer::render_dot (double x, double y, lay::CanvasPlane *plane)
{
  lay::Bitmap *bitmap = static_cast<lay::Bitmap *> (plane);

  x += 0.5;
  y += 0.5;

  //  basic optimizations: outside window
  if (x < 0.0 || x >= bitmap->width () ||
      y < 0.0 || y >= bitmap->height ()) {
    return;
  }

  unsigned int yint = (unsigned int) y;
  unsigned int xint = (unsigned int) x;
  bitmap->fill (yint, xint, xint + 1);
}
    
void 
BitmapRenderer::render_box (double xmin, double ymin, double xmax, double ymax, lay::CanvasPlane *plane)
{
  lay::Bitmap *bitmap = static_cast<lay::Bitmap *> (plane);

  xmin += 0.5;
  xmax += 0.5;
  ymin += 0.5;
  ymax += 0.5;

  //  basic optimizations: outside window
  if (xmax < 0.0 || xmin >= bitmap->width () ||
      ymax < 0.0 || ymin >= bitmap->height ()) {
    return;
  }

  unsigned int y1int = (unsigned int) (std::max (0.0, std::min (ymin, double (bitmap->height () - 1))));
  unsigned int y2int = (unsigned int) (std::max (0.0, std::min (ymax, double (bitmap->height () - 1))));
  unsigned int x1int = (unsigned int) (std::max (0.0, std::min (xmin, double (bitmap->width () - 1))));
  unsigned int x2int = (unsigned int) (std::max (0.0, std::min (xmax, double (bitmap->width () - 1))));
  for (unsigned int y = y1int; y <= y2int; ++y) {
    bitmap->fill (y, x1int, x2int + 1);
  }
}

void 
BitmapRenderer::draw (const db::Shape &shape, const db::CplxTrans &trans,
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *text)
{
  if (shape.is_text ()) {
    
    db::Point p = db::Point () + shape.text_trans ().disp ();
    db::DPoint dp = trans * p;

    if ((vertices || frame) &&
        dp.x () < m_width - 0.5 && dp.x () > -0.5 && 
        dp.y () < m_height - 0.5 && dp.y () > -0.5) {

      clear ();

      db::Point pp = db::Point (dp);

      //  generate a dot (cross, if marked) at the text's position
      if (vertices) {
        vertices->pixel (pp.x (), pp.y ());
      }
      if (frame) {
        frame->pixel (pp.x (), pp.y ());
      }

    }

    if (m_draw_texts && text) {

      db::DFTrans fp (db::DFTrans::r0);
      db::DCoord h = trans.ctrans (m_default_text_size);
      db::Font font = shape.text_font () == db::NoFont ? m_font : shape.text_font ();

      if (m_apply_text_trans && font != db::NoFont && font != db::DefaultFont) {
        fp = db::DFTrans (trans.fp_trans () * shape.text_trans ());
        h = trans.ctrans (shape.text_size () > 0 ? shape.text_size () : m_default_text_size);
      }

      db::HAlign halign = shape.text_halign ();
      db::VAlign valign = shape.text_valign ();

      double fy = 0.0;
      if (valign == db::VAlignBottom || valign == db::NoVAlign) {
        fy = 1.0;
      } else if (valign == db::VAlignTop) {
        fy = -1.0;
      }

      double fx = 0.0;
      if (halign == db::HAlignLeft || halign == db::NoHAlign) {
        fx = 1.0;
      } else if (halign == db::HAlignRight) {
        fx = -1.0;
      }

      db::DVector tp1 (db::DVector (fx * 2.0, fy * 2.0 + (fy - 1) * 0.5 * h));
      db::DVector tp2 (db::DVector (fx * 2.0, fy * 2.0 + (fy + 1) * 0.5 * h));

      clear ();

      insert (db::DBox (dp + fp (tp1), dp + fp (tp2)), shape.text_string (), font, halign, valign, fp);

      render_texts (*text);

    }

  } else {

    db::Box bbox = shape.bbox ();
    double threshold = 1.0 / trans.mag ();

    if (bbox.width () <= threshold && bbox.height () <= threshold && !shape.is_point ()) {

      db::DPoint dc = trans * bbox.center ();
      if (fill && ! shape.is_edge ()) {
        render_dot (dc.x (), dc.y (), fill);
      } 
      if (frame) {
        render_dot (dc.x (), dc.y (), frame);
      } 
      if (vertices) {
        render_dot (dc.x (), dc.y (), vertices);
      }

    } else if (shape.is_box () || shape.is_point ()) {

      draw (bbox, trans, fill, frame, vertices, text);

    } else if (shape.is_polygon ()) {

      //  simplify to a rectangle if possible
      db::Box b (shape.bbox ());
      if (simplify_box (b, trans)) {
        draw (b, trans, fill, frame, vertices, text);
      } else {

        clear ();
        db::Shape::polygon_edge_iterator e = shape.begin_edge (); 
        for ( ; ! e.at_end (); ++e) {
          insert (trans * *e);
        }

        if (vertices) {
          render_vertices (*vertices, 1);
        }
        if (fill) {
          render_fill (*fill);
        }
        if (frame) {
          if (m_xfill) {
            add_xfill ();
          }
          render_contour (*frame);
        }

      }

    } else if (shape.is_edge ()) {

      draw (shape.edge (), trans, fill, frame, vertices, text);

    } else if (shape.is_path ()) {

      //  simplify to a rectangle if possible
      db::Box b (shape.bbox ());
      if (simplify_box (b, trans)) {
        draw (b, trans, fill, frame, vertices, text);
      } else {

        //  instantiate the path and draw
        db::Path path;
        shape.path (path);
        draw (path, trans, fill, frame, vertices, text);

      }

    }

  }
}

void 
BitmapRenderer::draw (const db::Polygon &poly, const db::CplxTrans &trans,
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane * /*text*/)
{
  //  simplify to a rectangle if possible
  db::Box b (poly.box ());

  double threshold = 1.0 / trans.mag ();
  if (b.width () < threshold && b.height () < threshold) {

    db::DPoint dp = trans * b.center ();
    if (fill) {
      render_dot (dp.x (), dp.y (), fill);
    }
    if (frame) {
      render_dot (dp.x (), dp.y (), frame);
    }
    if (vertices) {
      render_dot (dp.x (), dp.y (), vertices);
    }

  } else {

    clear ();

    bool xfill = m_xfill;

    if (simplify_box (b, trans)) {
      xfill = false;
      insert (trans * b);
    } else {
      for (db::Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
        insert (trans * *e);
      }
    }

    if (vertices) {
      render_vertices (*vertices, 1);
    }
    if (fill) {
      render_fill (*fill);
    }
    if (frame) {
      if (xfill) {
        add_xfill ();
      }
      render_contour (*frame);
    }

  }
}

void 
BitmapRenderer::draw (const db::DPolygon &poly, const db::DCplxTrans &trans,
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane * /*text*/)
{
  //  simplify to a rectangle or dot if possible
  db::DBox b (poly.box ());

  double threshold = 1.0 / trans.mag ();
  if (b.width () < threshold && b.height () < threshold) {

    db::DPoint dp = trans * b.center ();
    if (fill) {
      render_dot (dp.x (), dp.y (), fill);
    }
    if (frame) {
      render_dot (dp.x (), dp.y (), frame);
    }
    if (vertices) {
      render_dot (dp.x (), dp.y (), vertices);
    }

  } else {

    clear ();

    bool xfill = m_xfill;

    if (simplify_box (b, trans)) {
      xfill = false;
      insert (trans * b);
    } else {
      for (db::DPolygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
        insert (trans * *e);
      }
    }

    if (vertices) {
      render_vertices (*vertices, 1);
    }
    if (fill) {
      render_fill (*fill);
    }
    if (frame) {
      if (xfill) {
        add_xfill ();
      }
      render_contour (*frame);
    }

  }
}

void 
BitmapRenderer::draw (const db::DPolygon &poly, 
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane * /*text*/)
{
  //  simplify to a rectangle or dot if possible
  db::DBox b (poly.box ());

  if (b.width () < 1.0 && b.height () < 1.0) {

    db::DPoint dp = b.center ();
    if (fill) {
      render_dot (dp.x (), dp.y (), fill);
    }
    if (frame) {
      render_dot (dp.x (), dp.y (), frame);
    }
    if (vertices) {
      render_dot (dp.x (), dp.y (), vertices);
    }

  } else {

    clear ();

    bool xfill = m_xfill;

    if (simplify_box (b, db::DCplxTrans ())) {
      xfill = false;
      insert (b);
    } else {
      for (db::DPolygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
        insert (*e);
      }
    }

    if (vertices) {
      render_vertices (*vertices, 1);
    }
    if (fill) {
      render_fill (*fill);
    }
    if (frame) {
      if (xfill) {
        add_xfill ();
      }
      render_contour (*frame);
    }

  }
}

void 
BitmapRenderer::draw (const db::ShortBox &box, const db::CplxTrans &trans,
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane * /*text*/)
{
  if (! box.empty ()) {

    double threshold = 1.0 / trans.mag ();
    if (box.width () < threshold && box.height () < threshold) {

      db::DPoint dp = trans * box.center ();
      if (fill) {
        render_dot (dp.x (), dp.y (), fill);
      }
      if (frame && frame != fill) {
        render_dot (dp.x (), dp.y (), frame);
      }
      if (vertices && vertices != fill) {
        render_dot (dp.x (), dp.y (), vertices);
      }

    } else {

      clear ();
      insert (db::Box (box), trans);

      if (vertices) {
        render_vertices (*vertices, 2);
      }
      if (fill && (fill != frame || (box.width () > threshold && box.height () > threshold))) {
        render_fill (*fill);
      }
      if (frame) {
        if (m_xfill) {
          insert (trans * db::Edge (box.p1 (), box.p2 ()));
          insert (trans * db::Edge (box.lower_right (), box.upper_left ()));
        }
        render_contour (*frame);
      }

    }

  }
}

void 
BitmapRenderer::draw (const db::Box &box, const db::CplxTrans &trans,
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane * /*text*/)
{
  if (! box.empty ()) {
  
    double threshold = 1.0 / trans.mag ();
    if (box.width () < threshold && box.height () < threshold) {

      db::DPoint dp = trans * box.center ();
      if (fill) {
        render_dot (dp.x (), dp.y (), fill);
      }
      if (frame && frame != fill) {
        render_dot (dp.x (), dp.y (), frame);
      }
      if (vertices && vertices != fill) {
        render_dot (dp.x (), dp.y (), vertices);
      }

    } else {

      clear ();
      insert (box, trans);

      if (vertices) {
        render_vertices (*vertices, 2);
      }
      if (fill && (fill != frame || (box.width () > threshold && box.height () > threshold))) {
        render_fill (*fill);
      }
      if (frame) {
        if (m_xfill) {
          insert (trans * db::Edge (box.p1 (), box.p2 ()));
          insert (trans * db::Edge (box.lower_right (), box.upper_left ()));
        }
        render_contour (*frame);
      }

    }

  }
}

void 
BitmapRenderer::draw (const db::DBox &box, 
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane * /*text*/)
{
  if (! box.empty ()) {
  
    if (box.width () < 1.0 && box.height () < 1.0) {

      db::DPoint dp = box.center ();
      if (fill) {
        render_dot (dp.x (), dp.y (), fill);
      }
      if (frame && frame != fill) {
        render_dot (dp.x (), dp.y (), frame);
      }
      if (vertices && vertices != fill) {
        render_dot (dp.x (), dp.y (), vertices);
      }

    } else {

      clear ();
      insert (box);

      if (vertices) {
        render_vertices (*vertices, 2);
      }
      if (fill && (fill != frame || (box.width () > 1.0 && box.height () > 1.0))) {
        render_fill (*fill);
      }
      if (frame) {
        if (m_xfill) {
          insert (db::DEdge (box.p1 (), box.p2 ()));
          insert (db::DEdge (box.lower_right (), box.upper_left ()));
        }
        render_contour (*frame);
      }

    }

  }
}

void 
BitmapRenderer::draw (const db::DBox &box, const db::DCplxTrans &trans,
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane * /*text*/)
{
  if (! box.empty ()) {
  
    double threshold = 1.0 / trans.mag ();
    if (box.width () < threshold && box.height () < threshold) {

      db::DPoint dp = trans * box.center ();
      if (fill) {
        render_dot (dp.x (), dp.y (), fill);
      }
      if (frame && frame != fill) {
        render_dot (dp.x (), dp.y (), frame);
      }
      if (vertices && vertices != fill) {
        render_dot (dp.x (), dp.y (), vertices);
      }

    } else {

      clear ();
      insert (box, trans);

      if (vertices) {
        render_vertices (*vertices, 2);
      }
      if (fill && (fill != frame || (box.width () > threshold && box.height () > threshold))) {
        render_fill (*fill);
      }
      if (frame) {
        if (m_xfill) {
          insert (trans * db::DEdge (box.p1 (), box.p2 ()));
          insert (trans * db::DEdge (box.lower_right (), box.upper_left ()));
        }
        render_contour (*frame);
      }

    }

  }
}

void 
BitmapRenderer::draw (const db::Path &path, const db::CplxTrans &trans,
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *text)
{
  //  simplify to a rectangle or dot if possible 
  db::Box b (path.box ());

  double threshold = 1.0 / trans.mag ();
  if (b.width () < threshold && b.height () < threshold) {

    db::DPoint dp = trans * b.center ();
    if (fill) {
      render_dot (dp.x (), dp.y (), fill);
    }
    if (frame) {
      render_dot (dp.x (), dp.y (), frame);
    }
    if (vertices) {
      render_dot (dp.x (), dp.y (), vertices);
    }

  } else {

    clear ();

    if (simplify_box (b, trans)) {
      draw (b, trans, fill, frame, vertices, text);
      return;
    } 

    //  generate the hull and produce the edges from this only if the path is considerably wide
    //  otherwise just render the spine
    
    double w = trans.ctrans (path.width ());

    bool thin = (w < 0.5);
    bool quite_thin = (w < 3.0);

    //  render the border
    if (! thin) {

      db::DPath::pointlist_type pts;
      path.transformed (trans).hull (pts);

      db::DPath::pointlist_type::const_iterator p = pts.begin ();
      if (p != pts.end ()) {

        db::DPath::pointlist_type::const_iterator pp = p;
        ++pp;

        while (pp != pts.end ()) {
          insert (db::DEdge (*p, *pp));
          p = pp;
          ++pp;
        }

        insert (db::DEdge (*p, pts [0]));
      }

      if (fill) {
        render_fill (*fill);
      }
      if (frame) {
        if (m_xfill) {
          add_xfill ();
        }
        render_contour (*frame);
      }

    }

    //  render the spine edges
    if (! quite_thin || thin) {

      clear ();

      db::Path::iterator q = path.begin ();
      if (q != path.end ()) {

        db::Path::iterator qq = q;
        ++qq;

        //  draw single-point as single-point edge to get the vertices
        if (qq == path.end ()) {
          insert (trans * db::Edge (*q, *q));
        }

        bool first = true;
        while (qq != path.end ()) {

          db::Edge seg (*q, *qq);
          q = qq;
          ++qq;

          //  If the path is simplified, the spine is drawn instead of the contour.
          //  We must apply the begin/end extensions then.
          
          if (thin && qq == path.end ()) {
            if (path.extensions ().second != 0 && (seg.dx () != 0 || seg.dy () != 0)) {
              db::DVector ed (seg.p2 () - seg.p1 ());
              ed *= 1.0 / ed.double_length ();
              seg = db::Edge (seg.p1 (), seg.p2 () + db::Vector (ed * double (path.extensions ().second)));
            }
          } 

          if (first) {
            first = false;
            if (thin && path.extensions ().first != 0 && (seg.dx () != 0 || seg.dy () != 0)) {
              db::DVector ed (seg.p2 () - seg.p1 ());
              ed *= 1.0 / ed.double_length ();
              seg = db::Edge (seg.p1 () - db::Vector (ed * double (path.extensions ().first)), seg.p2 ());
            }
          }
            
          insert (trans * seg);

        }

      }

      if (vertices) {
        render_vertices (*vertices, 0);
      }
      if (frame) {
        render_contour (*frame);
      }

    }

  }
}

void 
BitmapRenderer::draw (const db::DPath &path, 
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *text)
{
  draw (path, db::DCplxTrans (), fill, frame, vertices, text);
}

void 
BitmapRenderer::draw (const db::DPath &path, const db::DCplxTrans &trans,
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *text)
{
  //  simplify to a rectangle if possible
  db::DBox b (path.box ());

  double threshold = 1.0 / trans.mag ();
  if (b.width () < threshold && b.height () < threshold) {

    db::DPoint dp = trans * b.center ();
    if (fill) {
      render_dot (dp.x (), dp.y (), fill);
    }
    if (frame) {
      render_dot (dp.x (), dp.y (), frame);
    }
    if (vertices) {
      render_dot (dp.x (), dp.y (), vertices);
    }

  } else {

    clear ();

    if (simplify_box (b, trans)) {
      draw (b, trans, fill, frame, vertices, text);
      return;
    } 

    //  generate the hull and produce the edges from this 

    db::DPath::pointlist_type pts;
    path.hull (pts);

    db::DPath::pointlist_type::const_iterator p = pts.begin ();
    if (p != pts.end ()) {

      db::DPath::pointlist_type::const_iterator pp = p;
      ++pp;

      while (pp != pts.end ()) {
        insert (trans * db::DEdge (*p, *pp));
        p = pp;
        ++pp;
      }

      insert (trans * db::DEdge (*p, pts [0]));
    }

    if (fill) {
      render_fill (*fill);
    }
    if (frame) {
      if (m_xfill) {
        add_xfill ();
      }
      render_contour (*frame);
    }

    //  render the spine edges

    clear ();

    db::DPath::iterator q = path.begin ();
    if (q != path.end ()) {

      db::DPath::iterator qq = q;
      ++qq;

      //  draw single-point as single-point edge to get the vertices
      if (qq == path.end ()) {
        insert (trans * db::DEdge (*q, *q));
      }

      while (qq != path.end ()) {
        insert (trans * db::DEdge (*q, *qq));
        q = qq;
        ++qq;
      }

    }

    if (vertices) {
      render_vertices (*vertices, 0);
    }
    if (frame) {
      render_contour (*frame);
    }

  }
}

void 
BitmapRenderer::draw (const db::Text &txt, const db::CplxTrans &trans,
                      lay::CanvasPlane * /*fill*/, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *text)
{
  db::Point p = db::Point () + txt.trans ().disp ();
  db::DPoint dp = trans * p;

  if ((vertices || frame) &&
      dp.x () < m_width - 0.5 && dp.x () > -0.5 && 
      dp.y () < m_height - 0.5 && dp.y () > -0.5) {

    clear ();

    db::Point pp = db::Point (dp);

    //  generate a dot (cross, if marked) at the text's position
    if (vertices) {
      vertices->pixel (pp.x (), pp.y ());
    }
    if (frame) {
      frame->pixel (pp.x (), pp.y ());
    }

  }

  if (m_draw_texts && text) {

    db::DFTrans fp (db::DFTrans::r0);
    db::DCoord h = trans.ctrans (m_default_text_size);
    db::Font font = txt.font () == db::NoFont ? m_font : txt.font ();

    if (m_apply_text_trans && font != db::NoFont && font != db::DefaultFont) {
      fp = db::DFTrans (trans.fp_trans () * txt.trans ());
      h = trans.ctrans (txt.size () > 0 ? txt.size () : m_default_text_size);
    }

    double fy = 0.0;
    if (txt.valign () == db::VAlignBottom || txt.valign () == db::NoVAlign) {
      fy = 1.0;
    } else if (txt.valign () == db::VAlignTop) {
      fy = -1.0;
    }

    double fx = 0.0;
    if (txt.halign () == db::HAlignLeft || txt.halign () == db::NoHAlign) {
      fx = 1.0;
    } else if (txt.halign () == db::HAlignRight) {
      fx = -1.0;
    }

    //  impose a 2 pixel offset to separate the text from the origin
    db::DVector tp1 (db::DVector (fx * 2.0, fy * 2.0 + (fy - 1) * 0.5 * h));
    db::DVector tp2 (db::DVector (fx * 2.0, fy * 2.0 + (fy + 1) * 0.5 * h));

    clear ();

    insert (db::DBox (dp + fp (tp1), dp + fp (tp2)), txt.string (), font, txt.halign (), txt.valign (), fp);
    
    render_texts (*text);

  }

}

void 
BitmapRenderer::draw (const db::DText &txt, 
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *text)
{
  draw (txt, db::DCplxTrans (), fill, frame, vertices, text);
}

void 
BitmapRenderer::draw (const db::DText &txt, const db::DCplxTrans &trans,
                      lay::CanvasPlane * /*fill*/, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *text)
{
  db::DPoint p = db::DPoint () + txt.trans ().disp ();
  db::DPoint dp = trans * p;

  if ((vertices || frame) &&
      dp.x () < m_width - 0.5 && dp.x () > -0.5 && 
      dp.y () < m_height - 0.5 && dp.y () > -0.5) {

    clear ();

    db::Point pp (dp);

    //  generate a dot (cross, if marked) at the text's position
    if (vertices) {
      vertices->pixel (pp.x (), pp.y ());
    }
    if (frame) {
      frame->pixel (pp.x (), pp.y ());
    }

  }

  if (m_draw_texts && text) {

    db::DFTrans fp (db::DFTrans::r0);
    db::DCoord h = trans.ctrans (m_default_text_size_dbl);
    db::Font font = txt.font () == db::NoFont ? m_font : txt.font ();

    if (m_apply_text_trans && font != db::NoFont && font != db::DefaultFont) {
      fp = trans.fp_trans () * db::DFTrans (txt.trans ());
      h = trans.ctrans (txt.size () > 0 ? txt.size () : m_default_text_size_dbl);
    }

    double fy = 0.0;
    if (txt.valign () == db::VAlignBottom || txt.valign () == db::NoVAlign) {
      fy = 1.0;
    } else if (txt.valign () == db::VAlignTop) {
      fy = -1.0;
    }

    double fx = 0.0;
    if (txt.halign () == db::HAlignLeft || txt.halign () == db::NoHAlign) {
      fx = 1.0;
    } else if (txt.halign () == db::HAlignRight) {
      fx = -1.0;
    }

    db::DVector tp1 (db::DVector (fx * 2.0, fy * 2.0 + (fy - 1) * 0.5 * h));
    db::DVector tp2 (db::DVector (fx * 2.0, fy * 2.0 + (fy + 1) * 0.5 * h));

    clear ();

    insert (db::DBox (dp + fp (tp1), dp + fp (tp2)), txt.string (), font, txt.halign (), txt.valign (), fp);
    
    render_texts (*text);

  }

}

void 
BitmapRenderer::draw (const db::DEdge &edge, 
                      lay::CanvasPlane * /*fill*/, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane * /*text*/)
{
  if (fabs (edge.dy ()) < 1.0 && fabs (edge.dx ()) < 1.0) {

    double x = (edge.p1 ().x () + edge.p2 ().x ()) * 0.5;
    double y = (edge.p1 ().y () + edge.p2 ().y ()) * 0.5;
    if (frame) {
      render_dot (x, y, frame);
    }
    if (vertices) {
      render_dot (x, y, vertices);
    }

  } else {

    clear ();

    insert (edge);

    if (vertices) {
      render_vertices (*vertices, 0);
    }
    if (frame) {
      render_contour (*frame);
    }

  }
}

void 
BitmapRenderer::draw (const db::Edge &edge, const db::CplxTrans &trans,
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *text)
{
  draw (trans * edge, fill, frame, vertices, text);
}

void 
BitmapRenderer::draw (const db::DEdge &edge, const db::DCplxTrans &trans,
                      lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *text)
{
  draw (trans * edge, fill, frame, vertices, text);
}

template <class Box, class Trans>
bool 
BitmapRenderer::simplify_box (Box &b, const Trans &trans)
{
  const double small_size_threshold = 1.0;

  bool ortho = trans.is_ortho ();
  if (! m_precise && ((ortho && trans.ctrans (std::min (b.width (), b.height ())) < small_size_threshold) || 
                      (! ortho && trans.ctrans (std::max (b.width (), b.height ())) < small_size_threshold))) {

    if (trans.ctrans (b.width ()) < small_size_threshold) {
      typename Box::coord_type c = b.center ().x ();
      b.set_left (c);
      b.set_right (c);
    } 

    if (trans.ctrans (b.height ()) < small_size_threshold) {
      typename Box::coord_type c = b.center ().y ();
      b.set_top (c);
      b.set_bottom (c);
    } 

    return true;

  } else {
    return false;
  }
}

}


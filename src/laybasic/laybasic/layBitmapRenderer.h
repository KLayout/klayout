
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



#ifndef HDR_layBitmapRenderer
#define HDR_layBitmapRenderer

#include "layRenderer.h"
#include "layBitmap.h"

namespace lay
{

/**
 *  @brief A edge set and text rendering object
 *
 *  The renderer is supposed to render a set of edges to
 *  one or more bitmaps. One bitmap holds the contour
 *  information, one the fill. A third one holds the 
 *  vertex information (dots).
 *  
 *  The intended use model is first to reserve a number of
 *  points if necessary, the fill the renderer with the
 *  edges and then render the content to the bitmaps.
 *
 *  The coordinate system of the bitmaps is 0,0..w-1,h-1
 *  with 0,0 being the lower left corner. 
 */
class LAYBASIC_PUBLIC BitmapRenderer 
  : public Renderer
{
public:
  /**
   *  @brief The default ctor
   */
  BitmapRenderer (unsigned int width, unsigned int height, double resolution);

  /**
   *  @brief Reserve space for n edges 
   *
   *  It is strongly recommended to use this method to
   *  reserve space in advance before any insert operation for edges or boxes.
   */
  virtual void reserve_edges (size_t n);

  /**
   *  @brief Reserve space for n texts 
   *
   *  It is strongly recommended to use this method to
   *  reserve space in advance before any insert operation for texts.
   */
  virtual void reserve_texts (size_t n);

  /** 
   *  @brief Clear the content
   */
  virtual void clear ();

  /**
   *  @brief Insert edges using the provided sequence and a transform function
   *
   *  Inserts the edge sequence [from,to), transformed with f(E).
   *
   *  @param from The start of the sequence
   *  @param to The past-end iterator of the sequence
   *  @param f The function to apply before inserting the edge. It must
   *         provide a db::DEdge operator(Iter::value_type).
   */
  template <class Iter, class Func> 
  void insert_edges (Iter from, Iter to, Func f)
  {
    for (Iter e = from; e != to; ++e) {
      insert (f (*e));
    }
  }

  /** 
   *  @brief Insert a box with a transformation (because it may not be orthogonal)
   */
  void insert (const db::Box &b, const db::CplxTrans &t);

  /** 
   *  @brief Insert a box with a transformation (because it may not be orthogonal)
   */
  void insert (const db::DBox &b, const db::DCplxTrans &t);

  /** 
   *  @brief Insert a box 
   */
  void insert (const db::DBox &b);

  /** 
   *  @brief Insert an edge 
   *
   *  The edges must be a db::DEdge type.
   */
  void insert (const db::DEdge &e);

  /**
   *  @brief Insert a text
   *
   *  @box The box in which to inscribe the text (or the point at which to align if the width and height is zero)
   *  @text The text to render
   *  @font The font to use
   *  @halign The horizontal alignment
   *  @valign The vertical alignment
   *  @trans The orientation of the text
   */
  void insert (const db::DBox &box, const std::string &text, db::Font font, db::HAlign halign, db::VAlign valign, db::DFTrans trans);

  /**
   *  @brief Render the interior of the object to the bitmap 
   */
  void render_fill (lay::CanvasPlane &bitmap);

  /**
   *  @brief Render the contour of the object to the bitmap 
   */
  void render_contour (lay::CanvasPlane &bitmap);

  /**
   *  @brief Render the vertices of the object to the bitmap 
   *
   *  Mode is
   *  0 for "all vertices" (2 per edge)
   *  1 for "one vertex per edge"
   *  2 for "one vertex per every second edge"
   */
  void render_vertices (lay::CanvasPlane &bitmap, int mode);

  /**
   *  @brief Render the texts of the object to the bitmap
   */
  void render_texts (lay::CanvasPlane &bitmap);

  /**
   *  @brief Render a generic shape into a set of bitmaps
   *
   *  The shape can either be a polygon, box, edge, path or text
   *
   *  @param shape The polygon to draw
   *  @param trans The transformation to apply
   *  @param fill The bitmap to which to render the interior. Can be 0 for not drawing this.
   *  @param frame The bitmap to which to render the contour. Can be 0 for not drawing this.
   *  @param vertices The bitmap to which to render the vertices. Can be 0 for not drawing this.
   *  @param texts Currenty not used. Used for drawing properties if drawing of these is required.
   */
  virtual void draw (const db::Shape &shape, const db::CplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /**
   *  @brief Render a polygon into a set of bitmaps
   *
   *  @param poly The polygon to draw
   *  @param trans The transformation to apply
   *  @param fill The bitmap to which to render the interior. Can be 0 for not drawing this.
   *  @param frame The bitmap to which to render the contour. Can be 0 for not drawing this.
   *  @param vertices The bitmap to which to render the vertices. Can be 0 for not drawing this.
   *  @param texts Currenty not used. No texts to draw.
   */
  virtual void draw (const db::Polygon &poly, const db::CplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for double coordinates
   */
  virtual void draw (const db::DPolygon &poly,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for double coordinates and transformation
   */
  virtual void draw (const db::DPolygon &poly, const db::DCplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /**
   *  @brief Render a box into a set of bitmaps
   *
   *  @param box The box to draw
   *  @param trans The transformation to apply
   *  @param fill The bitmap to which to render the interior. Can be 0 for not drawing this.
   *  @param frame The bitmap to which to render the contour. Can be 0 for not drawing this.
   *  @param vertices The bitmap to which to render the vertices. Can be 0 for not drawing this.
   *  @param texts Currenty not used. No texts to draw.
   */
  virtual void draw (const db::Box &box, const db::CplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for short representation
   */
  virtual void draw (const db::ShortBox &box, const db::CplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for double coordinates
   */
  virtual void draw (const db::DBox &box, 
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for double coordinates and transformation
   */
  virtual void draw (const db::DBox &box, const db::DCplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /**
   *  @brief Render a path into a set of bitmaps
   *
   *  @param path The path to draw
   *  @param trans The transformation to apply
   *  @param fill The bitmap to which to render the interior. Can be 0 for not drawing this.
   *  @param frame The bitmap to which to render the contour. Can be 0 for not drawing this.
   *  @param vertices The bitmap to which to render the vertices. Can be 0 for not drawing this.
   *  @param texts Currenty not used. No texts to draw.
   */
  virtual void draw (const db::Path &path, const db::CplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for double coordinates
   */
  virtual void draw (const db::DPath &path,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for double coordinates and transformation
   */
  virtual void draw (const db::DPath &path, const db::DCplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /**
   *  @brief Render a text into a set of bitmaps
   *
   *  @param text The text to draw
   *  @param trans The transformation to apply
   *  @param fill Not used currenty, since the text does not have an interior
   *  @param frame The bitmap to which to render the contour. Can be 0 for not drawing this.
   *  @param vertices The bitmap to which to render the vertices. Can be 0 for not drawing this.
   *  @param texts The bitmap to which to render the text.
   */
  virtual void draw (const db::Text &text, const db::CplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for double coordinates
   */
  virtual void draw (const db::DText &text, 
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for double coordinates and transformation
   */
  virtual void draw (const db::DText &text, const db::DCplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /**
   *  @brief Draw a formatted text
   *
   *  @box The box in which to inscribe the text (or the point at which to align if the width and height is zero)
   *  @text The text to render
   *  @font The font to use
   *  @halign The horizontal alignment
   *  @valign The vertical alignment
   *  @trans The orientation of the text
   *  @param fill Not used currenty, since the text does not have an interior
   *  @param frame The bitmap to which to render the contour. Can be 0 for not drawing this.
   *  @param vertices The bitmap to which to render the vertices. Can be 0 for not drawing this.
   *  @param texts The bitmap to which to render the text.
   */
  virtual void draw (const db::DBox &box, const std::string &text, db::Font font, db::HAlign halign, db::VAlign valign, db::DFTrans trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /**
   *  @brief Render a edge into a set of bitmaps
   *
   *  @param edge The box to draw
   *  @param trans The transformation to apply
   *  @param fill Not used currenty, since the text does not have an interior
   *  @param frame The bitmap to which to render the contour. Can be 0 for not drawing this.
   *  @param vertices The bitmap to which to render the vertices. Can be 0 for not drawing this.
   *  @param texts Currenty not used. No texts to draw.
   */
  virtual void draw (const db::Edge &edge, const db::CplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for double coordinates 
   */
  virtual void draw (const db::DEdge &edge, 
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

  /** 
   *  @brief Same for double coordinates and transformation
   */
  virtual void draw (const db::DEdge &edge, const db::DCplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts);

private:
  std::vector<lay::RenderEdge> m_edges;
  double m_xmin, m_xmax, m_ymin, m_ymax;
  bool m_ortho;
  std::vector<lay::RenderText> m_texts;

  template <class Box, class Trans> bool simplify_box (Box &, const Trans &);
  static void render_dot (double x, double y, lay::CanvasPlane *plane);
  static void render_box (double xmin, double ymin, double xmax, double ymax, lay::CanvasPlane *plane);
  void add_xfill ();
};

}

#endif


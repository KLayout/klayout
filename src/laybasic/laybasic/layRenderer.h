
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



#ifndef HDR_layRenderer
#define HDR_layRenderer

#include "laybasicCommon.h"

#include "dbEdge.h"
#include "dbPolygon.h"
#include "dbBox.h"
#include "dbPath.h"
#include "dbText.h"
#include "dbTrans.h"
#include "dbShape.h"
#include "dbHershey.h"

#include "layCanvasPlane.h"

#include <vector>

namespace lay
{

class CanvasPlane;

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
class LAYBASIC_PUBLIC Renderer 
{
public:
  /**
   *  @brief The ctor
   */
  Renderer (unsigned int width, unsigned int height, double resolution);

  /**
   *  @brief The destructor
   */
  virtual ~Renderer () { }

  /**
   *  @brief Control whether draw(db::Shape..) draws precise (does not simplify to box)
   *
   *  Default is "not precise".
   */
  void set_precise (bool f)
  {
    m_precise = f;
  }

  /**
   *  @brief Sets a value indicating whether to draw a diagonal cross across boxes and polygons
   */
  void set_xfill (bool f)
  {
    m_xfill = f;
  }

  /**
   *  @brief Control whether draw(db::Shape..) draws properties on the text layer
   */
  void draw_properties (bool f)
  {
    m_draw_properties = f;
  }

  /**
   *  @brief Control whether draw(db::Shape..) draws the "description" properties on the text layer
   */
  void draw_description_property (bool f)
  {
    m_draw_description_property = f;
  }

  /**
   *  @brief Control whether draw(db::Text..) draws texts but rather the pixel
   */
  void draw_texts (bool f)
  {
    m_draw_texts = f;
  }

  /**
   *  @brief The default text size to use for draw(db::Text..) 
   */
  void default_text_size (db::Coord sz)
  {
    m_default_text_size = sz;
  }

  /**
   *  @brief Get the default text size
   */
  db::Coord default_text_size () const
  {
    return m_default_text_size;
  }

  /**
   *  @brief The default text size to use for draw(db::DText..) 
   */
  void default_text_size (double sz)
  {
    m_default_text_size_dbl = sz;
  }

  /**
   *  @brief Get the default text size for draw (DText ...)
   */
  double default_text_size_dbl () const
  {
    return m_default_text_size_dbl;
  }

  /**
   *  @brief The font to use for draw(db::Text..).
   */
  void set_font (db::Font f)
  {
    m_font = f;
  }

  /**
   *  @brief Get the font to use for draw(db::Text..)
   */
  db::Font font () const
  {
    return m_font;
  }

  /**
   *  @brief Apply text transformations to text or not for draw(db::Text..).
   */
  void apply_text_trans (bool f)
  {
    m_apply_text_trans = f;
  }

  /**
   *  @brief Get the flag which determines to apply text transformations for draw(db::Text..)
   */
  bool apply_text_trans () const
  {
    return m_apply_text_trans;
  }

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
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0; 

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
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0; 

  /** 
   *  @brief Same for double coordinates
   */
  virtual void draw (const db::DPolygon &poly,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0; 

  /** 
   *  @brief Same for double coordinates and transformation
   */
  virtual void draw (const db::DPolygon &poly, const db::DCplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0; 

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
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0; 

  /** 
   *  @brief Same for short representation
   */
  virtual void draw (const db::ShortBox &box, const db::CplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0; 

  /** 
   *  @brief Same for double coordinates
   */
  virtual void draw (const db::DBox &box,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0; 

  /** 
   *  @brief Same for double coordinates and transformation
   */
  virtual void draw (const db::DBox &box, const db::DCplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0; 

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
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0;

  /** 
   *  @brief Same for double coordinates
   */
  virtual void draw (const db::DPath &path, 
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0;

  /** 
   *  @brief Same for double coordinates and transformation
   */
  virtual void draw (const db::DPath &path, const db::DCplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0;

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
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0;

  /** 
   *  @brief Same for double coordinates
   */
  virtual void draw (const db::DText &text,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0;

  /** 
   *  @brief Same for double coordinates and transformation
   */
  virtual void draw (const db::DText &text, const db::DCplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0;

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
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0;

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
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0;

  /** 
   *  @brief Same for double coordinates
   */
  virtual void draw (const db::DEdge &edge, 
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0;

  /** 
   *  @brief Same for double coordinates and transformation
   */
  virtual void draw (const db::DEdge &edge, const db::DCplxTrans &trans,
                     lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertices, lay::CanvasPlane *texts) = 0;

  /**
   *  @brief Render the properties string of a generic shape
   *
   *  The shape can either be a polygon, box, edge, path or text
   *
   *  This method draws the properties if "draw_properties" is set to true and/or the 
   *  "description" property when "draw_description_property" is set to true.
   */
  void draw_propstring (const db::Shape &shape, const db::PropertiesRepository *prep, lay::CanvasPlane *text, const db::CplxTrans &trans);

  /**
   *  @brief Draw a properties string at the given position
   *
   *  The transformation is not used to transform the point (it must be given in micron units
   *  already) but to compute the position of the text, given the text height.
   */
  void draw_propstring (db::properties_id_type id, 
                        const db::PropertiesRepository *prep, const db::DPoint &pref, 
                        lay::CanvasPlane *text, const db::CplxTrans &trans);

  /**
   *  @brief Draw a string from the "description" property at the given position
   *
   *  The transformation is not used to transform the point (it must be given in micron units
   *  already) but to compute the position of the text, given the text height.
   */
  void draw_description_propstring (db::properties_id_type id, 
                                    const db::PropertiesRepository *prep, const db::DPoint &pref, 
                                    lay::CanvasPlane *text, const db::CplxTrans &trans);

  /**
   *  @brief Get the width
   */
  unsigned int width () const
  {
    return m_width;
  }
 
  /**
   *  @brief Get the height
   */
  unsigned int height () const
  {
    return m_height;
  }
 
  /**
   *  @brief Get the resolution value
   *
   *  The resolution value is used to convert dimensions on the output device into canvas
   *  units. A resolution of 1 means that one canvas unit roughly corresponds to 0.01" (0.25mm).
   *  A resolution value of 2 means that one canvas unit roughly corresponds to 0.02".
   */
  double resolution () const 
  {
    return m_resolution;
  }

protected:
  bool m_draw_texts;
  bool m_draw_properties;
  bool m_draw_description_property;
  db::Coord m_default_text_size;
  double m_default_text_size_dbl;
  bool m_apply_text_trans;
  bool m_precise;
  bool m_xfill;
  db::Font m_font;
  unsigned int m_width, m_height;
  double m_resolution;
};

} // namespace lay

#endif


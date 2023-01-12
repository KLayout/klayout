
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

#ifndef HDR_layBitmap
#define HDR_layBitmap

#include "laybasicCommon.h"
#include "layCanvasPlane.h"

namespace lay {

/**
 *  @brief A renderer edge object
 */
class RenderEdge : public db::DEdge 
{
public:
  RenderEdge (const db::DEdge &d)
    : db::DEdge (d), m_d (true)
  {
    //  RenderEdges are normalized such that 
    //  p1 < p2. m_d is false if the points have been swapped.
    if (p2 () < p1 ()) {
      swap_points ();
      m_d = !m_d;
    }

    //  Horizontal is true, if the edge is horizontal within render_epsilon
    //  range.
    m_horizontal = (fabs (dy ()) < render_epsilon);

    //  Compute the slope to speed up position computation
    if (m_horizontal) {
      m_slope = 0.0;
    } else {
      m_slope = dx () / dy ();
    }

    m_pos = 0.0;
  }

  double pos () const
  {
    return m_pos;
  }

  void set_pos (double p) 
  { 
    m_pos = p;
  }

  double pos (double y) const
  {
    if (y > y2 ()) {
      return x2 ();
    } else if (y < y1 ()) {
      return x1 ();
    } else {
      return x1 () + m_slope * (y - y1 ());
    }
  }

  double slope () const
  {
    return m_slope;
  }

  bool is_horizontal () const 
  {
    return m_horizontal;
  }

  int delta () const
  {
    return m_d ? 1 : -1;
  }

  bool done (double y) const
  {
    return (y > y2 ());
  }

  bool todo (double y) const
  {
    return (y <= y1 ());
  }

  void update_pos (double y) 
  {
    m_pos = pos (y);
  }
  
private:
  bool m_d, m_horizontal;
  double m_pos;
  double m_slope;
};

/**
 *  @brief A rendered text object
 */
struct RenderText
{
  db::DBox b;
  std::string text;
  db::Font font;
  db::HAlign halign;
  db::VAlign valign;
  db::DFTrans trans;
};

/**
 *  @brief A bitmap class
 *
 *  This class represents a monochrome bitmap on which to paint
 *  shapes. The basic ability is to provide scanlines. A scanline
 *  is an array of uint32_t. It is up to the renderer how the
 *  scanlines are used.
 *  
 */

class LAYBASIC_PUBLIC Bitmap 
  : public CanvasPlane
{
public:
  /**
   *  @brief Default constructor
   *
   *  Creates an empty Bitmap object with width 0 and height 0.
   */
  Bitmap ();

  /**
   *  @brief Standard constructor
   *
   *  Creates a bitmap of w*h pixels max.
   *
   *  @param w The width of the bitmap 
   *  @param h The height of the bitmap
   *  @param r The resolution of the bitmap
   */
  Bitmap (unsigned int w, unsigned int h, double r);

  /**
   *  @brief Copy constructor
   */
  Bitmap (const Bitmap &d);

  /**
   *  @brief Assignment operator
   */
  Bitmap &operator= (const Bitmap &d);

  /**
   *  @brief Destructor
   */
  virtual ~Bitmap ();
  
  /**  
   *  @brief Clear the bitmap but do not resize
   */
  virtual void clear ();

  /**
   *  @brief Create a rectangle of one pixel size
   */
  virtual void pixel (unsigned int x, unsigned int y)
  {
    fill (y, x, x + 1);
  }

  /**
   *  @brief Fetch an empty scanline
   *  This is a dummy scanline provided for convenience. It has the same
   *  length than a normal scanline.
   */
  const uint32_t *empty_scanline () const
  {
    return m_empty_scanline;
  }

  /**
   *  @brief Fetch scanline number n
   */
  const uint32_t *scanline (unsigned int n) const;

  /**
   *  @brief Report true if the scanline is empty
   */
  bool is_scanline_empty (unsigned int n) const;

  /**
   *  @brief Fetch scanline number n in modification mode
   */
  uint32_t *scanline (unsigned int n);

  /**
   *  @brief Get the resolution of the bitmap
   */
  double resolution () const;

  /**
   *  @brief Get the width of the bitmap
   */
  unsigned int width () const;

  /**
   *  @brief Get the height of the bitmap
   */
  unsigned int height () const;

  /**
   *  @brief Fill method 
   *
   *  Fills a line at scanline y, starting from x1 and ending
   *  with x2 (exclusive). x1 must not be larger or equal than m_width.
   *  x2 must not be larger than m_width.
   *  If x2 is less than x1, nothing is filled.
   *  The bitwise interpretation of the uint32_t's of the scanline
   *  is lowest bit left.
   *
   *  @param y The scanline
   *  @param x1 The start coordinate
   *  @param x2 The end coordinate
   */
  void fill (unsigned int y, unsigned int x1, unsigned int x2);

  /**
   *  @brief Clears the given part of the scanline
   *
   *  Same as fill(), but resets the bits.
   *
   *  @param y The scanline
   *  @param x1 The start coordinate
   *  @param x2 The end coordinate
   */
  void clear (unsigned int y, unsigned int x1, unsigned int x2);

  /**
   *  @brief Merges the "from" bitmap into this
   *
   *  The merge operation will copy all bits from "from" into this with the given displacement.
   *  Proper clipping will be applied.
   */
  void merge (const lay::Bitmap *from, int dx, int dy);

  /**  
   *  @brief Test whether the bitmap is empty
   */
  bool empty () const;

  /**
   *  @brief Tell, which scanline is the first
   */
  unsigned int first_scanline () const;

  /**
   *  @brief Tell, which scanline is the last (plus 1)
   */
  unsigned int last_scanline () const;

  /**
   *  @brief Render a text object
   */
  virtual void render_text (const lay::RenderText &text);

  /**
   *  @brief Render a set of edges as fill
   */
  virtual void render_fill (std::vector<lay::RenderEdge> &edges);

  /**
   *  @brief Render a set of edges as fill (ortho case)
   */
  virtual void render_fill_ortho (std::vector<lay::RenderEdge> &edges);

  /**
   *  @brief Render a set of edges as contour
   */
  virtual void render_contour (std::vector<lay::RenderEdge> &edges);

  /**
   *  @brief Render a set of edges as orthogonal contour
   */
  virtual void render_contour_ortho (std::vector<lay::RenderEdge> &edges);

  /**
   *  @brief Render a set of edges as vertices
   */
  virtual void render_vertices (std::vector<lay::RenderEdge> &edges, int mode);

private:
  unsigned int m_width;
  unsigned int m_height;
  double m_resolution;
  std::vector<uint32_t *> m_scanlines;
  std::vector<uint32_t *> m_free;
  uint32_t *m_empty_scanline;
  unsigned int m_first_sl, m_last_sl;

  void cleanup ();
  void init (unsigned int w, unsigned int h);

  /**
   *  @brief Fill a bit pattern 
   *
   *  Fills a line at scanline y, starting from x1 and using the 
   *  bit pattern in p. The lowest bit of p is the leftmost one of 
   *  the pattern. Always a 32 bit pattern is or'd with the contents
   *  of the scanline. If x1 is large enough that the pattern would extend over
   *  the end of the scanline, the pattern is cut. If x is negative, the
   *  pattern's end is used. y must not be larger than m_height.
   *
   *  @param y The scanline
   *  @param x1 The start coordinate
   *  @param p The pattern to use
   *  @param stride The number of words per line
   *  @param n The height (number of lines)
   */
  void fill_pattern (int y, int x, const uint32_t *p, unsigned int stride, unsigned int n);
};

inline bool
Bitmap::is_scanline_empty (unsigned int n) const
{
  return m_scanlines.empty () || m_scanlines [n] == 0;
}

inline bool
Bitmap::empty () const
{
  return m_first_sl >= m_last_sl;
}

inline double
Bitmap::resolution () const
{
  return m_resolution;
}  

inline unsigned int
Bitmap::width () const
{
  return m_width;
}  

inline unsigned int
Bitmap::height () const
{
  return m_height;
}  

inline unsigned int
Bitmap::first_scanline () const
{
  return m_first_sl;
}  

inline unsigned int
Bitmap::last_scanline () const
{
  return m_last_sl;
}  

inline const uint32_t *
Bitmap::scanline (unsigned n) const
{
  if (n >= m_scanlines.size () || m_scanlines [n] == 0) {
    return m_empty_scanline;
  } else {
    return m_scanlines [n];
  }
}

} // namespace lay

#endif

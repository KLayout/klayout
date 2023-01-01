
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



#ifndef HDR_layViewOp
#define HDR_layViewOp

#include "laybasicCommon.h"
#include "tlColor.h"

#include <stdint.h>

namespace lay
{

const unsigned int wordlen  = 32;
const unsigned int wordbits = 5;
const unsigned int wordones = 0xffffffff;

/**
 *  @brief The operator that describes how a bitmap is combined with the others on the stack
 *
 *  Each operator is described by these properties:
 *    - a mode (copy, or, and, xor) which describes what boolean operation to use
 *    - a index for a dither pattern (what bitmap to use as the mask)
 *    - a pen: pen width and style 
 *    - a bitmap index
 *  
 *  The bitmap index describes which bitmap to take the data from.
 *  Usually bitmaps and combination operators are organized in two vectors: one for the
 *  bitmaps and one for the operators. By default, each operator is associated with the
 *  corresponding bitmap. 
 *
 *  However, an operator can be associated with any bitmap by setting the bitmap index.
 *  In this case, the length of the operator list does not need to have the same length
 *  than the bitmap list.
 *
 *  In the context of "floating" bitmaps (foreground bitmaps for markers, rulers when
 *  dragging etc.), the bitmap index is not used to map a bitmap but to specify a order
 *  of bitmaps: the planes are drawn in the order of the bitmap index.
 */
class LAYBASIC_PUBLIC ViewOp
{
public:
  /**
   *  @brief The modes provided
   */
  enum Mode { Copy, Or, And, Xor };

  /**
   *  @brief The pen types provided
   */
  enum Shape { Rect, Cross };

  /**
   *  @brief The default ctor
   */
  ViewOp ();

  /**
   *  @brief The constructor given all the parameters to describe the operator
   */
  ViewOp (tl::color_t color, Mode mode, unsigned int line_style_index, unsigned int dither_index, unsigned int dither_offset, Shape shape = Rect, int width = 1, int bitmap_index = -1);
  
  /**
   *  @brief Internal: provide the mask for the "or" part of the operation
   */
  tl::color_t ormask () const
  { 
    return m_or; 
  }

  /**
   *  @brief Internal: provide the mask for the "and" part of the operation
   */
  tl::color_t andmask () const
  { 
    return m_and; 
  }

  /**
   *  @brief Internal: provide the mask for the "xor" part of the operation
   */
  tl::color_t xormask () const
  { 
    return m_xor; 
  }

  /**
   *  @brief The pen type
   */
  Shape shape () const
  {
    return m_shape;
  }

  /**
   *  @brief Set the pen type
   */
  void shape (Shape s)
  {
    m_shape = s;
  }

  /**
   *  @brief The width
   */
  int width () const
  {
    return m_width;
  }

  /**
   *  @brief Set the width
   */
  void width (int w)
  {
    m_width = w;
  }

  /**
   *  @brief Write accessor to the dither pattern index 
   */
  void set_dither_index (unsigned int di)
  {
    m_dither_index = di;
  }

  /**
   *  @brief Read accessor to the dither pattern index 
   */
  unsigned int dither_index () const
  {
    return m_dither_index;
  }

  /**
   *  @brief Write accessor to the dither pattern offset
   */
  void set_dither_offset (unsigned int d)
  {
    m_dither_offset = d;
  }

  /**
   *  @brief Read accessor to the dither pattern offset
   */
  unsigned int dither_offset () const
  {
    return m_dither_offset;
  }

  /**
   *  @brief Write accessor to the line style index
   */
  void set_line_style_index (unsigned int lsi)
  {
    m_line_style_index = lsi;
  }

  /**
   *  @brief Read accessor to the dither pattern index
   */
  unsigned int line_style_index () const
  {
    return m_line_style_index;
  }

  /**
   *  @brief Write accessor to the bitmap index
   *
   *  Writing -1 to the bitmap index resets the bitmap index to the default.
   */
  void bitmap_index (int bi) 
  {
    m_bitmap_index = bi;
  }

  /**
   *  @brief Read accessor to the bitmap index 
   */
  int bitmap_index () const
  {
    return m_bitmap_index;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const ViewOp &d) const
  {
    return (m_or == d.m_or && m_and == d.m_and && 
            m_xor == d.m_xor &&
            m_line_style_index == d.m_line_style_index &&
            m_dither_index == d.m_dither_index && m_dither_offset == d.m_dither_offset &&
            m_width == d.m_width && m_shape == d.m_shape &&
            m_bitmap_index == d.m_bitmap_index);
  }
  
  /**
   *  @brief Comparison
   *
   *  See above to learn why the bitmap index is compared first.
   */
  bool operator< (const ViewOp &d) const
  {
    if (m_bitmap_index != d.m_bitmap_index) return m_bitmap_index < d.m_bitmap_index;
    if (m_or != d.m_or) return (m_or < d.m_or);
    if (m_and != d.m_and) return m_and < d.m_and;
    if (m_xor != d.m_xor) return m_xor < d.m_xor;
    if (m_line_style_index != d.m_line_style_index) return m_line_style_index < d.m_line_style_index;
    if (m_dither_index != d.m_dither_index) return m_dither_index < d.m_dither_index;
    if (m_dither_offset != d.m_dither_offset) return m_dither_offset < d.m_dither_offset;
    if (m_width != d.m_width) return m_width < d.m_width;
    if (m_shape != d.m_shape) return m_shape < d.m_shape;
    return false;
  }

private:
  tl::color_t m_or;
  tl::color_t m_and;
  tl::color_t m_xor;
  unsigned int m_line_style_index;
  unsigned int m_dither_index, m_dither_offset;
  Shape m_shape;
  int m_width;
  int m_bitmap_index;

  void init (tl::color_t color, Mode mode);
};

} // namespace lay

#endif


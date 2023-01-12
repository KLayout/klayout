
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



#include "layViewOp.h"
#include "layDitherPattern.h"

namespace lay 
{

ViewOp::ViewOp () 
  : m_line_style_index (0), m_dither_index (0), m_dither_offset (0), m_shape (Rect), m_width (0), m_bitmap_index (0)
{ 
  init (0, Copy);
}

ViewOp::ViewOp (tl::color_t color, Mode mode, unsigned int line_style_index, unsigned int dither_index, unsigned int dither_offset, Shape shape, int width, int bitmap_index)
  : m_line_style_index (line_style_index),
    m_dither_index (dither_index), m_dither_offset (dither_offset),
    m_shape (shape),
    m_width (width),
    m_bitmap_index (bitmap_index)
{ 
  init (color, mode);
}

void
ViewOp::init (tl::color_t color, Mode mode)
{
  m_or  = (mode == Copy || mode == Or)  ? color : 0;
  m_and = (mode == Copy || mode == And) ? color : wordones;
  m_xor = (mode == Xor) ? color : 0;
}

} // namespace lay

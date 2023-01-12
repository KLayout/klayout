
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

#ifndef HDR_layPixelBufferPainter
#define HDR_layPixelBufferPainter

#include "laybasicCommon.h"

#include "tlColor.h"
#include "dbPoint.h"

namespace tl
{
  class PixelBuffer;
}

namespace lay {

/**
 *  @brief A very simplistic painter for tl::PixelBuffer
 *
 *  This painter supports very few primitives currently and is used to paint the
 *  background grid for example.
 */
class LAYBASIC_PUBLIC PixelBufferPainter
{
public:
  PixelBufferPainter (tl::PixelBuffer &img, unsigned int width, unsigned int height, double resolution);

  void set (const db::Point &p, tl::Color c);
  void draw_line (const db::Point &p1, const db::Point &p2, tl::Color c);
  void fill_rect (const db::Point &p1, const db::Point &p2, tl::Color c);
  void draw_rect (const db::Point &p1, const db::Point &p2, tl::Color c);
  void draw_text (const char *t, const db::Point &p, tl::Color c, int halign, int valign);

private:
  tl::PixelBuffer *mp_img;
  double m_resolution;
  int m_width, m_height;
};

}

#endif


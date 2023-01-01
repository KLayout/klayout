
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

#include "layPixelBufferPainter.h"

#include "layFixedFont.h"
#include "tlPixelBuffer.h"

namespace lay
{

PixelBufferPainter::PixelBufferPainter (tl::PixelBuffer &img, unsigned int width, unsigned int height, double resolution)
  : mp_img (&img),
    m_resolution (resolution), m_width (width), m_height (height)
{
  // .. nothing yet ..
}

void
PixelBufferPainter::set (const db::Point &p, tl::Color c)
{
  if (p.x () >= 0 && p.x () < m_width && p.y () >= 0 && p.y () < m_height) {
    ((unsigned int *) mp_img->scan_line (p.y ())) [p.x ()] = c.rgb ();
  }
}

void
PixelBufferPainter::draw_line (const db::Point &p1, const db::Point &p2, tl::Color c)
{
  if (p1.x () == p2.x ()) {

    int x = p1.x ();
    int y1 = std::min (p1.y (), p2.y ());
    int y2 = std::max (p1.y (), p2.y ());
    if ((y2 >= 0 || y1 < m_height) && x >= 0 && x < m_width) {
      y1 = std::max (y1, 0);
      y2 = std::min (y2, m_height - 1);
      for (int y = y1; y <= y2; ++y) {
        ((unsigned int *) mp_img->scan_line (y)) [x] = c.rgb ();
      }
    }

  } else if (p1.y () == p2.y ()) {

    int y = p1.y ();
    int x1 = std::min (p1.x (), p2.x ());
    int x2 = std::max (p1.x (), p2.x ());
    if ((x2 >= 0 || x1 < m_width) && y >= 0 && y < m_height) {
      x1 = std::max (x1, 0);
      x2 = std::min (x2, m_width - 1);
      unsigned int *sl = (unsigned int *) mp_img->scan_line (y) + x1;
      for (int x = x1; x <= x2; ++x) {
        *sl++ = c.rgb ();
      }
    }

  } else {
    // TODO: not implemented yet.
  }
}

void
PixelBufferPainter::fill_rect (const db::Point &p1, const db::Point &p2, tl::Color c)
{
  int y1 = std::min (p1.y (), p2.y ());
  int y2 = std::max (p1.y (), p2.y ());
  for (int y = y1; y <= y2; ++y) {
    draw_line (db::Point (p1.x (), y), db::Point (p2.x (), y), c);
  }
}

void
PixelBufferPainter::draw_rect (const db::Point &p1, const db::Point &p2, tl::Color c)
{
  int y1 = std::min (p1.y (), p2.y ());
  int y2 = std::max (p1.y (), p2.y ());
  int x1 = std::min (p1.x (), p2.x ());
  int x2 = std::max (p1.x (), p2.x ());
  draw_line (db::Point (x1, y1), db::Point (x2, y1), c);
  draw_line (db::Point (x1, y2), db::Point (x2, y2), c);
  draw_line (db::Point (x1, y1), db::Point (x1, y2), c);
  draw_line (db::Point (x2, y1), db::Point (x2, y2), c);
}

void
PixelBufferPainter::draw_text (const char *t, const db::Point &p, tl::Color c, int halign, int valign)
{
  const lay::FixedFont &ff = lay::FixedFont::get_font (m_resolution);
  int x = p.x (), y = p.y ();

  if (halign < 0) {
    x -= ff.width () * int (strlen (t));
  } else if (halign == 0) {
    x -= ff.width () * int (strlen (t)) / 2;
  }

  if (valign < 0) {
    y += ff.height ();
  } else if (valign == 0) {
    y += ff.height () / 2;
  }

  //  TODO: simple implementation
  for (; *t; ++t) {

    unsigned char ch = *t;

    if (x < -int (ff.width ()) || x >= int (mp_img->width ()) || y < 0 || y >= int (mp_img->height () + ff.height ())) {
      continue;
    }

    if (ch < ff.first_char () || (ch - ff.first_char ()) >= ff.n_chars ()) {
      continue;
    }

    const uint32_t *dc = ff.data () + size_t (ch - ff.first_char ()) * ff.height () * ff.stride ();
    for (unsigned int i = 0; i < ff.height (); ++i, dc += ff.stride ()) {

      int iy = y - ff.height () + i + 1;
      if (iy >= 0 || iy < int (mp_img->height ())) {

        uint32_t *d = (uint32_t *) mp_img->scan_line (y - ff.height () + i);
        uint32_t m = 1;
        int ix = x;
        const uint32_t *ds = dc;

        for (unsigned int j = 0; j < ff.width (); ++j, ++ix) {

          if ((*ds & m) && ix >= 0 && ix < int (mp_img->width ())) {
            d[ix] = c.rgb ();
          }

          m <<= 1;
          //  word wrap
          if (m == 0) {
            ++ds;
            m = 1;
          }

        }

      }

    }

    x += ff.width ();

  }

}

}

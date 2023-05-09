
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


#include "layBitmap.h"
#include "layBitmapRenderer.h"
#include "layFixedFont.h"
#include "tlAlgorithm.h"

namespace lay {

Bitmap::Bitmap ()
  : m_empty_scanline (0)
{
  init (0, 0);
  m_resolution = 1.0;
}

Bitmap::Bitmap (unsigned int w, unsigned int h, double r)
  : m_empty_scanline (0)
{
  init (w, h);
  m_resolution = r;
}

Bitmap::Bitmap (const Bitmap &d)
  : m_empty_scanline (0)
{
  init (d.m_width, d.m_height);
  operator= (d);
}

Bitmap &
Bitmap::operator= (const Bitmap &d)
{
  if (&d != this) {

    if (m_width != d.m_width || m_height != d.m_height) {
      cleanup ();
      init (d.m_width, d.m_height);
    }

    m_resolution = d.m_resolution;

    for (unsigned int i = 0; i < m_height; ++i) {
      if (! d.m_scanlines.empty () && d.m_scanlines [i] != 0) {
        uint32_t *sl = scanline (i);
        uint32_t *ss = d.m_scanlines [i];
        for (unsigned int b = (m_width + 31) / 32; b > 0; --b) {
          *sl++ = *ss++;
        }
      } else if (! m_scanlines.empty () && m_scanlines [i] != 0) {
        m_free.push_back (m_scanlines [i]);
        m_scanlines [i] = 0;
      }
    }
    
    m_last_sl = d.m_last_sl;
    m_first_sl = d.m_first_sl;

  }
  return *this;
}

Bitmap::~Bitmap ()
{
  cleanup ();
}

uint32_t *
Bitmap::scanline (unsigned int n)
{
  if (m_scanlines.empty ()) {
    m_scanlines.resize (m_height, 0);
  }

  uint32_t *sl = m_scanlines [n];
  if (sl == 0) {
    unsigned int b = (m_width + 31) / 32;
    if (! m_free.empty ()) {
      sl = m_scanlines [n] = m_free.back ();
      m_free.pop_back ();
    } else {
      sl = m_scanlines [n] = new uint32_t [b];
    }
    for (uint32_t *p = sl; b > 0; --b) {
      *p++ = 0;
    }
    if (m_first_sl > n) {
      m_first_sl = n;
    }
    if (m_last_sl <= n) {
      m_last_sl = n + 1;
    }
  } 
  return sl;
}

void
Bitmap::clear ()
{
  for (std::vector<uint32_t *>::iterator i = m_scanlines.begin (); i != m_scanlines.end (); ++i) {
    if (*i) {
      m_free.push_back (*i);
    }
  }
  for (std::vector<uint32_t *>::iterator i = m_scanlines.begin (); i != m_scanlines.end (); ++i) {
    *i = 0;
  }
  m_last_sl = m_first_sl = 0;
}

void 
Bitmap::cleanup ()
{
  m_last_sl = m_first_sl = 0;

  if (m_empty_scanline) {
    delete [] m_empty_scanline;
    m_empty_scanline = 0;
  }

  for (std::vector<uint32_t *>::iterator i = m_scanlines.begin (); i != m_scanlines.end (); ++i) {
    delete [] *i;
  }
  m_scanlines.clear ();

  for (std::vector<uint32_t *>::iterator i = m_free.begin (); i != m_free.end (); ++i) {
    delete [] *i;
  }
  m_free.clear ();

  m_width = m_height = 0;
  m_last_sl = m_first_sl = 0;
}

void 
Bitmap::init (unsigned int w, unsigned int h)
{
  m_width = w;
  m_height = h;

  if (m_width > 0) {
    unsigned int b = (w + 31) / 32;
    m_empty_scanline = new uint32_t [b];
    for (uint32_t *s = m_empty_scanline; b > 0; --b) {
      *s++ = 0;
    }
  }

  m_last_sl = m_first_sl = 0;
}

void 
Bitmap::merge (const lay::Bitmap *from, int dx, int dy)
{
  if (! from) {
    return;
  }
  if (dx >= int (width ()) || dy >= int (height ())) {
    return;
  }

  unsigned int from_height = from->height ();
  if (int (from_height) + dy > int (height ())) {
    from_height = height () - dy;
  }

  unsigned int n0 = 0;
  if (dy < 0) {
    if (dy + int (from_height) <= 0) {
      return;
    }
    n0 = (unsigned int) -dy;
  }

  unsigned int from_width = from->width ();
  if (int (from_width) + dx > int (width ())) {
    from_width = width () - dx;
  }

  if (dx < 0) {

    if (dx + int (from_width) <= 0) {
      return;
    }

    unsigned int mo = ((unsigned int) -dx) / 32;
    unsigned int m = (from_width + 31) / 32 - mo;
    unsigned int mm = (from_width + dx + 31) / 32;

    unsigned int s1 = ((unsigned int) -dx) % 32;
    unsigned int s2 = 32 - s1;

    for (unsigned int n = n0; n < from_height; ++n) {

      if (from->is_scanline_empty (n)) {
        continue;
      }

      const uint32_t *sl_from = from->scanline (n) + mo;
      uint32_t *sl_to = scanline (n + dy);

      if (! s1) {
        for (unsigned int i = 0; i < m; ++i) {
          *sl_to++ |= *sl_from++;
        }
      } else if (m) {
        for (unsigned int i = 1; i < m; ++i) {
          *sl_to++ |= (sl_from[1] << s2) | (sl_from[0] >> s1);
          ++sl_from;
        }
        if (mm > m - 1) {
          *sl_to++ |= (sl_from[0] >> s1);
        }
      }

    }

  } else {

    unsigned int mo = ((unsigned int) dx) / 32;
    unsigned int m = (from_width + 31) / 32;
    unsigned int mm = (from_width + (((unsigned int) dx) % 32) + 31) / 32;

    unsigned int s1 = ((unsigned int) dx) % 32;
    unsigned int s2 = 32 - s1;

    for (unsigned int n = n0; n < from_height; ++n) {

      if (from->is_scanline_empty (n)) {
        continue;
      }

      const uint32_t *sl_from = from->scanline (n);
      uint32_t *sl_to = scanline (n + dy) + mo;

      if (! s1) {
        for (unsigned int i = 0; i < m; ++i) {
          *sl_to++ |= *sl_from++;
        }
      } else if (m) {
        *sl_to++ |= (sl_from[0] << s1);
        for (unsigned int i = 1; i < m; ++i) {
          *sl_to++ |= (sl_from[0] >> s2) | (sl_from[1] << s1);
          ++sl_from;
        }
        if (mm > m) {
          *sl_to++ |= (sl_from[0] >> s2);
        }
      }

    }

  }
}

void 
Bitmap::fill_pattern (int y, int x, const uint32_t *pp, unsigned int stride, unsigned int n)
{
  if (x < int (m_width)) {

    if (y >= int (m_height)) {
      if (n <= (y - m_height + 1)) {
        return;
      }
      n -= y - m_height + 1;
      pp += y - m_height + 1;
      y = m_height - 1;
    }

    while (n > 0 && y >= 0) {

      for (unsigned int s = 0; s < stride; ++s) {

        int x1 = x + s * 32;

        uint32_t p = *pp++;

        if (x1 < 0) {
          if (x1 <= -32) {
            return;
          }
          p >>= (unsigned int)-x1;
          x1 = 0;
        }

        if (p) {

          unsigned int bx = ((unsigned int) x1) & ~(32 - 1);

          uint32_t *sl = scanline (y);
          sl += bx / 32;

          *sl |= (p << ((unsigned int)x1 - bx));

          if ((unsigned int)x1 > bx) {

            bx += 32;
            ++sl;

            if (bx < m_width) {
              *sl |= (p >> (bx - (unsigned int)x1));
            }

          }

        }

      }

      --n;
      --y;

    }

  }
}
 
static const uint32_t masks [32] = {
  0x00000000, 0x00000001, 0x00000003, 0x00000007,
  0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
  0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
  0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
  0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
  0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
  0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
  0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff
};

static const uint32_t all_ones = 0xffffffff;

void 
Bitmap::fill (unsigned int y, unsigned int x1, unsigned int x2)
{
  unsigned int b1 = x1 / 32;

  uint32_t *sl = scanline (y);
  sl += b1;

  unsigned int b = x2 / 32 - b1;
  if (b == 0) {

    *sl |= (masks [x2 % 32] & ~masks [x1 % 32]);

  } else if (b > 0) {

    *sl++ |= ~masks [x1 % 32];
    while (b > 1) {
      *sl++ |= all_ones;
      b--;
    }

    unsigned int m = masks [x2 % 32];
    //  Hint: if x2==width and width%32==0, sl must not be accessed. This is guaranteed by
    //  checking if m != 0.
    if (m) {
      *sl |= m;
    }

  }
}

void
Bitmap::clear (unsigned int y, unsigned int x1, unsigned int x2)
{
  unsigned int b1 = x1 / 32;

  uint32_t *sl = scanline (y);
  sl += b1;

  unsigned int b = x2 / 32 - b1;
  if (b == 0) {

    *sl &= ~masks [x2 % 32] | masks [x1 % 32];

  } else if (b > 0) {

    *sl++ &= masks [x1 % 32];
    while (b > 1) {
      *sl++ = 0;
      b--;
    }

    unsigned int m = masks [x2 % 32];
    //  Hint: if x2==width and width%32==0, sl must not be accessed. This is guaranteed by
    //  checking if m != 0.
    if (m) {
      *sl &= ~m;
    }

  }
}

struct PosCompareF
{
  bool operator() (const RenderEdge &a, const RenderEdge &b) const
  {
    return a.pos () < b.pos ();
  }
};

struct X1CompareF 
{
  bool operator() (const RenderEdge &a, const RenderEdge &b) const
  {
    return a.x1 () < b.x1 ();
  }
};

void
Bitmap::render_fill (std::vector<lay::RenderEdge> &edges)
{
  //  sort the edges so we can operate on the sorted list
  tl::sort (edges.begin (), edges.end ());

  double y = std::max (0.0, floor (edges.begin ()->y1 ()));
  std::vector<lay::RenderEdge>::iterator done = edges.begin ();

  //  this is generic case
  while (done != edges.end () && y < height ()) {

    for ( ; done != edges.end (); ++done) {
      if (! done->done (y)) {
        break;
      }
    }

    std::vector<lay::RenderEdge>::iterator todo = done;

    for ( ; todo != edges.end (); ++todo) {
      if (todo->done (y)) {
        std::swap (*done, *todo);
        ++done;
      }
      if (todo->todo (y)) {
        break;
      } 
    }

    std::vector<lay::RenderEdge>::iterator e;
    for (e = done; e != todo; ++e) {
      e->set_pos (e->x1 () + e->slope () * (y - e->y1 ()));
    }

    PosCompareF f;
    tl::sort (done, todo, f);

    int c = 0;
    bool x1set = false;
    double x1 = 0;
    unsigned int yint = (unsigned int) (y + 0.5);

    for (e = done; e != todo; ++e) {
      if (! e->is_horizontal ()) {
        c += e->delta ();
        if (c == 0) {  //  this is implementing the != 0 rule
          if (e->pos () > 0) {
            unsigned int x1int = 0;
            if (x1 > 0.0) {
              x1int = (unsigned int) x1;
              if (double (x1int) != x1) {
                ++x1int; 
              }
            }
            fill (yint, x1int, (unsigned int) std::min (double (width () - 1), e->pos ()) + 1);
          }
          x1set = false;
        } else if (!x1set) {
          x1 = e->pos ();
          x1set = true;
          if (x1 >= double (width ())) {
            break;
          }
        }
      }
    }
    
    y += 1.0;

  }
}

void
Bitmap::render_fill_ortho (std::vector<lay::RenderEdge> &edges)
{
  //  sort the edges so we can operate on the sorted list
  tl::sort (edges.begin (), edges.end ());

  double y = std::max (0.0, floor (edges.begin ()->y1 ()));
  std::vector<lay::RenderEdge>::iterator done = edges.begin ();

  //  this is the purely manhattan case
  //  TODO: the manhattan optimization is not really effective ..
  while (done != edges.end () && y < height ()) {

    for ( ; done != edges.end (); ++done) {
      if (! done->done (y)) {
        break;
      }
    }

    std::vector<lay::RenderEdge>::iterator todo = done;
    for ( ; todo != edges.end (); ++todo) {
      if (todo->done (y)) {
        std::swap (*done, *todo);
        ++done;
      }
      if (todo->todo (y)) {
        break;
      } 
    }

    std::vector<lay::RenderEdge>::iterator e;

    X1CompareF f;
    tl::sort (done, todo, f);

    int c = 0;
    bool x1set = false;
    double x1 = 0;
    unsigned int yint = (unsigned int) (y + 0.5);

    for (e = done; e != todo; ++e) {
      if (! e->is_horizontal ()) {
        c += e->delta ();
        if (c == 0) {  //  this is implementing the != 0 rule
          if (e->x1 () > 0) {
            unsigned int x1int = 0;
            if (x1 > 0.0) {
              x1int = (unsigned int) x1;
              if (double (x1int) != x1) {
                ++x1int; 
              }
            }
            fill (yint, x1int, (unsigned int) std::min (double (width () - 1), e->x1 ()) + 1);
          }
          x1set = false;
        } else if (!x1set) {
          x1 = e->x1 ();
          x1set = true;
          if (x1 >= double (width ())) {
            break;
          }
        }
      }
    }
    
    y += 1.0;

  }
}

void
Bitmap::render_vertices (std::vector<lay::RenderEdge> &edges, int mode)
{
  double xmax = width ();
  double ymax = height ();

  for (std::vector<lay::RenderEdge>::iterator e = edges.begin (); e != edges.end (); ++e) {

    double x, y;
    
    if (mode == 0 || e->delta () > 0) {
      x = e->x1 () + 0.5;
      y = e->y1 () + 0.5;
      if (x >= 0.0 && x < xmax && y >= 0.0 && y < ymax) {
        unsigned int xint = (unsigned int)x;
        fill ((unsigned int)y, xint, xint + 1);
      }
    }
     
    if (mode == 0 || e->delta () < 0) {
      x = e->x2 () + 0.5;
      y = e->y2 () + 0.5;
      if (x >= 0.0 && x < xmax && y >= 0.0 && y < ymax) {
        unsigned int xint = (unsigned int)x;
        fill ((unsigned int)y, xint, xint + 1);
      }
    }

    if (mode == 2 && e != edges.end ()) {
      ++e;
    }
     
  }
}

void
Bitmap::render_contour_ortho (std::vector<lay::RenderEdge> &edges)
{
  //  this is the purely manhattan case
  for (std::vector<lay::RenderEdge>::iterator e = edges.begin (); e != edges.end (); ++e) {

    //  This is the line render algorithm 
    //  The basic idea is to decompose the line into stripes
    //  associated with a integer y value. The stripe extends
    //  from x1 to x2 then. The algorithm tries to find a 
    //  set of pixels on the y line that covers the range x1
    //  to x2 as good as possible and advances to the next y 
    //  value then.

    //  TODO: the rendering would be somewhat more efficient if
    //  we would first clip the line and then render it. This
    //  way we could remove the tests in the rendering loop.

    if (! e->is_horizontal ()) {

      //  vertical
      double x = e->x1 ();
      if (e->y1 () < double (height ()) - 0.5  
          && e->y2 () >= -0.5
          && x < (double) width () - 0.5 
          && x >= -0.5) {

        unsigned int xint  = (unsigned int) (std::max (0.0, std::min (double (width () - 1), x) + 0.5));
        unsigned int yint  = (unsigned int) std::max (floor (e->y1 () + 0.5), 0.0);
        unsigned int yeint = (unsigned int) std::min (double (height () - 1), std::max (floor (e->y2 () + 0.5), 0.0));

        while (yint <= yeint) {
          fill (yint, xint, xint + 1);
          ++yint;
        }

      }

    } else {

      //  horizontal
      double x1 = e->x1 ();
      double x2 = e->x2 ();
      if (x1 > x2) {
        std::swap (x1, x2);
      }

      double y = e->y1 ();
      if (y < (double) height () - 0.5 
          && y >= -0.5 
          && x1 < (double) width () - 0.5 
          && x2 >= -0.5) {
      
        unsigned int x1int = (unsigned int) (std::max (0.0, std::min (double (width () - 1), x1) + 0.5));
        unsigned int x2int = (unsigned int) (std::max (0.0, std::min (double (width () - 1), x2) + 0.5));
        unsigned int yint  = (unsigned int) std::max (floor (y + 0.5), 0.0);
        fill (yint, x1int, x2int + 1);

      }

    }

  }
}

void
Bitmap::render_contour (std::vector<lay::RenderEdge> &edges)
{
  //  this is the generic case
  for (std::vector<lay::RenderEdge>::iterator e = edges.begin (); e != edges.end (); ++e) {

    //  This is the line render algorithm 
    //  The basic idea is to decompose the line into stripes
    //  associated with a integer y value. The stripe extends
    //  from x1 to x2 then. The algorithm tries to find a 
    //  set of pixels on the y line that covers the range x1
    //  to x2 as good as possible and advances to the next y 
    //  value then.

    //  TODO: the rendering would be somewhat more efficient if
    //  we would first clip the line and then render it. This
    //  way we could remove the tests in the rendering loop.

    if (e->y1 () < double (height ()) - 0.5 && e->y2 () >= -0.5) {

      double y = std::max (floor (e->y1 () + 0.5), 0.0);
      double x = e->pos (y - 0.5);

      double dx = e->pos (y + 0.5) - x;
      double dx1 = (e->y2 () - e->y1 ()) < 1e-6 ? 0.0 : (e->x2 () - e->x1 ()) / (e->y2 () - e->y1 ());

      double y2m = e->y2 () - 0.5;

      unsigned int yeint = (unsigned int) std::min (double (height () - 1), std::max (floor (e->y2 () + 0.5), 0.0));

      unsigned int xint = (unsigned int) (std::max (0.0, std::min (double (width () - 1), x) + 0.5));
      unsigned int yint = (unsigned int) y;

      if (x < (double) width () - 0.5 && x >= 0.0) {
        fill (yint, xint, xint + 1);
      }

      if (e->x2 () > e->x1 ()) {

        while (yint <= yeint) {

          double xx;
          if (double (yint) > y2m) { 
            xx = e->x2 () + 0.5;
          } else {
            xx = x + dx;
            dx = dx1;
          }

          unsigned int xe;
          if (xx >= 0.0) {
            if (xx >= (double) width ()) {
              if (x >= (double) width () - 1) {
                break; // done.
              }
              xe = width () - 1;
            } else {
              xe = (unsigned int) (xx);
            }
            if (xe <= xint) {
              fill (yint, xint, xint + 1);
            } else {
              fill (yint, xint + 1, xe + 1);
              xint = xe;
            }
          } else {
            xint = 0;
          }

          x = xx;
          ++yint;

        }

      } else {

        while (yint <= yeint) {

          double xx;
          if (double (yint) > y2m) { 
            xx = e->x2 () - 0.5;
          } else {
            xx = x + dx;
            dx = dx1;
          }

          unsigned int xe;
          if (xx < double (width () - 1)) {
            if (xx < 0.0) {
              if (x <= 0.0) {
                break;
              }
              xe = 0;
            } else {
              xe = (unsigned int) (xx);
              if (double (xe) != xx) { 
                ++xe;
              }
            }
            if (xe >= xint) {
              fill (yint, xint, xint + 1);
            } else {
              fill (yint, xe, xint);
              xint = xe;
            }
          } else {
            xint = width ();
          }

          x = xx;
          ++yint;

        }

      }

    }

  }
}

void
Bitmap::render_text (const lay::RenderText &text)
{
  if (text.font == db::DefaultFont) {

    const lay::FixedFont &ff = lay::FixedFont::get_font (m_resolution);

    //  count the lines and max. characters per line

    unsigned int lines = 1;
    for (const char *cp = text.text.c_str (); *cp; ) {
      if (tl::skip_newline (cp)) {
        ++lines;
      } else {
        ++cp;
      }
    }

    //  compute the actual top left position
    double y;
    if (text.valign == db::VAlignBottom || text.valign == db::NoVAlign) {
      y = text.b.bottom ();
      y += double (ff.line_height () * (lines - 1) + ff.height ());
    } else if (text.valign == db::VAlignCenter) {
      y = text.b.center ().y ();
      y += double ((ff.line_height () * (lines - 1) + ff.height ()) / 2);
    } else {
      y = text.b.top ();
    }

    //  start generating the characters

    const char *cp1 = text.text.c_str ();
    while (*cp1) {

      unsigned int length = 0;
      const char *cp = cp1; 
      while (*cp && !tl::is_newline (*cp)) {
        tl::utf32_from_utf8 (cp);
        ++length;
      }

      double xx;
      if (text.halign == db::HAlignRight) {
        xx = text.b.right ();
        xx -= double (ff.width () * length);
      } else if (text.halign == db::HAlignCenter) {
        xx = text.b.center ().x ();
        xx -= double (ff.width () * length / 2);
      } else {
        xx = text.b.left ();
      }
      xx -= 0.5;

      if (y > -0.5 && y < double (height () + ff.height () - 1) - 0.5) {

        while (cp1 != cp) {

          uint32_t c = tl::utf32_from_utf8 (cp1, cp);
          if (c < uint32_t (ff.first_char ()) || c >= uint32_t (ff.n_chars ()) + ff.first_char ()) {
            //  NOTE: '?' needs to be a valid character always
            c = uint32_t ('?');
          }

          if (xx > -100.0 && xx < double (width ())) {
            fill_pattern (int (y + 0.5), int (floor (xx)), ff.data () + (c - ff.first_char ()) * ff.height () * ff.stride (), ff.stride (), ff.height ());
          }

          xx += double (ff.width ());

        }

      } else {
        cp1 = cp;
      }

      //  next line
      if (tl::skip_newline (cp1)) {
        y -= double (ff.line_height ());
      }

    }

  } else {
   
    //  Create a sub-renderer so we do not need to clear *this
    lay::BitmapRenderer hr (m_width, m_height, m_resolution);

    db::DHershey ht (text.text, text.font);
    hr.reserve_edges (ht.count_edges ());
    ht.justify (text.b.transformed (text.trans.inverted ()), text.halign, text.valign);

    //  text becomes unreadable at a low scaling factor - don't draw then.
    if (ht.scale_factor () > 0.2) {

      db::DHershey::edge_iterator e = ht.begin_edges ();
      while (! e.at_end ()) {
        hr.insert ((*e).transformed (text.trans));
        ++e;
      }

    }

    hr.render_contour (*this);

  }

}

 
} // namespace lay



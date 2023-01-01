
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


#include "layRedrawThreadCanvas.h"
#include "layCanvasPlane.h"
#include "layBitmapsToImage.h"
#include "layDrawing.h"
#include "layBitmap.h"

namespace lay
{

// ------------------------------------------------------------------------

BitmapCanvasData::BitmapCanvasData ()
  : m_width (0), m_height (0)
{
  //  .. nothing yet ..
}

BitmapCanvasData::~BitmapCanvasData ()
{
  clear_planes ();
}

BitmapCanvasData::BitmapCanvasData (const std::vector <lay::Bitmap *> &plane_buffers, const std::vector <std::vector <lay::Bitmap *> > &drawing_plane_buffers, unsigned int width, unsigned int height)
  : m_width (0), m_height (0)
{
  assign (mp_plane_buffers, plane_buffers);
  assign (mp_drawing_plane_buffers, drawing_plane_buffers);
  m_width = width;
  m_height = height;
}

BitmapCanvasData &BitmapCanvasData::operator= (const BitmapCanvasData &data)
{
  if (this != &data) {
    assign (mp_plane_buffers, data.mp_plane_buffers);
    assign (mp_drawing_plane_buffers, data.mp_drawing_plane_buffers);
    m_width = data.m_width;
    m_height = data.m_height;
  }
  return *this;
}

void BitmapCanvasData::fetch (std::vector <lay::Bitmap *> &plane_buffers, std::vector <std::vector <lay::Bitmap *> > &drawing_plane_buffers, unsigned int &width, unsigned int &height) const
{
  assign (plane_buffers, mp_plane_buffers);
  assign (drawing_plane_buffers, mp_drawing_plane_buffers);
  width = m_width;
  height = m_height;
}

bool BitmapCanvasData::can_fetch (const std::vector <lay::Bitmap *> &plane_buffers, const std::vector <std::vector <lay::Bitmap *> > &drawing_plane_buffers, unsigned int width, unsigned int height) const
{
  if (m_width != width || m_height != height) {
    return false;
  }
  if (plane_buffers.size () != mp_plane_buffers.size ()) {
    return false;
  }
  if (drawing_plane_buffers.size () != mp_drawing_plane_buffers.size ()) {
    return false;
  }
  for (size_t i = 0; i < drawing_plane_buffers.size (); ++i) {
    if (drawing_plane_buffers[i].size () != mp_drawing_plane_buffers[i].size ()) {
      return false;
    }
  }
  return true;
}

void BitmapCanvasData::assign (std::vector <lay::Bitmap *> &to, const std::vector <lay::Bitmap *> &from)
{
  while (! to.empty ()) {
    delete to.back ();
    to.pop_back ();
  }

  for (std::vector <lay::Bitmap *>::const_iterator b = from.begin (); b != from.end (); ++b) {
    to.push_back (new lay::Bitmap (**b));
  }
}

void BitmapCanvasData::assign (std::vector <std::vector <lay::Bitmap *> > &to, const std::vector <std::vector <lay::Bitmap *> > &from)
{
  while (! to.empty ()) {
    while (! to.back ().empty ()) {
      delete to.back ().back ();
      to.back ().pop_back ();
    }
    to.pop_back ();
  }

  for (std::vector <std::vector <lay::Bitmap *> >::const_iterator f = from.begin (); f != from.end (); ++f) {
    to.push_back (std::vector <lay::Bitmap *> ());
    for (std::vector <lay::Bitmap *>::const_iterator ff = f->begin (); ff != f->end (); ++ff) {
      to.back ().push_back (new lay::Bitmap (**ff));
    }
  }
}

void BitmapCanvasData::clear_planes ()
{
  assign (mp_plane_buffers, std::vector <lay::Bitmap *> ());
  assign (mp_drawing_plane_buffers, std::vector <std::vector <lay::Bitmap *> > ());
}

void BitmapCanvasData::swap (BitmapCanvasData &other)
{
  mp_plane_buffers.swap (other.mp_plane_buffers);
  mp_drawing_plane_buffers.swap (other.mp_drawing_plane_buffers);
  std::swap (m_width, other.m_width);
  std::swap (m_height, other.m_height);
}

// ------------------------------------------------------------------------

BitmapRedrawThreadCanvas::BitmapRedrawThreadCanvas ()
  : m_width (1), m_height (1)
{
  // .. nothing yet ..
}

BitmapRedrawThreadCanvas::~BitmapRedrawThreadCanvas ()
{
  lock ();
  clear_planes ();
  unlock ();
}

bool
BitmapRedrawThreadCanvas::shift_supported () const
{
  return true;
}

bool 
BitmapRedrawThreadCanvas::is_plane_empty (unsigned int n) 
{
  bool ret = true;

  lock ();
  if (n < mp_plane_buffers.size () && mp_plane_buffers [n] != 0) {
    ret = mp_plane_buffers [n]->empty ();
  }
  unlock ();

  return ret;
}

static void 
shift_bitmap (const lay::Bitmap *from, lay::Bitmap *to, int dx, int dy)
{
  tl_assert (from->width () == to->width ());
  tl_assert (from->height () == to->height ());

  to->clear ();

  if (dy <= -int (from->height()) || dy >= int (from->height ()) || 
      dx <= -int (from->width()) || dx >= int (from->width ())) {
    return;
  }

  int nn = int (to->height ()) - std::max (0, dy);
  for (int n = std::max (-dy, 0); n < nn; ++n) {

    if (from->is_scanline_empty (n)) {
      continue;
    }

    const uint32_t *sl_from = from->scanline (n);
    uint32_t *sl_to = to->scanline (n + dy);

    if (dx < 0) {

      unsigned int mo = ((unsigned int) -dx) / 32;
      unsigned int m = (to->width () + 31) / 32 - mo;
      sl_from += mo;

      unsigned int s1 = ((unsigned int) -dx) % 32;
      if (! s1) {
        for (unsigned int i = 0; i < m; ++i) {
          *sl_to++ = *sl_from++;
        }
      } else {
        unsigned int s2 = 32 - s1;
        for (unsigned int i = 1; i < m; ++i) {
          *sl_to++ = (sl_from[0] >> s1) | (sl_from[1] << s2);
          ++sl_from;
        }
        if (m) {
          *sl_to++ = (sl_from[0] >> s1);
        }
      }

    } else {

      unsigned int mo = ((unsigned int) dx) / 32;
      unsigned int m = (to->width () + 31) / 32 - mo;
      sl_to += mo;

      unsigned int s1 = ((unsigned int) dx) % 32;
      if (! s1) {
        for (unsigned int i = 0; i < m; ++i) {
          *sl_to++ = *sl_from++;
        }
      } else {
        unsigned int s2 = 32 - s1;
        if (m) {
          *sl_to++ = (sl_from[0] << s1);
        }
        for (unsigned int i = 1; i < m; ++i) {
          *sl_to++ = (sl_from[0] >> s2) | (sl_from[1] << s1);
          ++sl_from;
        }
      }

    }

  }
}

void 
BitmapRedrawThreadCanvas::prepare (unsigned int nlayers, unsigned int width, unsigned int height, double resolution, const db::Vector *shift_vector, const std::vector<int> *planes, const lay::Drawings *drawings)
{
  RedrawThreadCanvas::prepare (nlayers, width, height, resolution, shift_vector, planes, drawings);

  lock ();

  if (shift_vector) {

    tl_assert (width == m_width);
    tl_assert (height == m_height);
    tl_assert (nlayers == mp_plane_buffers.size ());

    for (size_t i = 0; i < mp_plane_buffers.size (); ++i) {
      lay::Bitmap *from = mp_plane_buffers [i];
      lay::Bitmap *to   = mp_plane_buffers [i] = new lay::Bitmap (width, height, resolution);
      shift_bitmap (from, to, shift_vector->x (), shift_vector->y ());
      delete from;
    }

    size_t di = 0;
    for (lay::Drawings::const_iterator d = drawings->begin (); d != drawings->end (); ++d, ++di) {
      for (unsigned int i = 0; i < d->num_planes (); ++i) {
        lay::Bitmap *from = mp_drawing_plane_buffers[di][i];
        lay::Bitmap *to   = mp_drawing_plane_buffers[di][i] = new lay::Bitmap (width, height, resolution);
        shift_bitmap (from, to, shift_vector->x (), shift_vector->y ());
        delete from;
      }
    }

  } else if (planes) {

    tl_assert (width == m_width);
    tl_assert (height == m_height);

    for (std::vector<int>::const_iterator l = planes->begin (); l != planes->end (); ++l) {

      if (*l < 0) {

        //  clear the custom drawing planes
        unsigned int di = 0;
        for (lay::Drawings::const_iterator d = drawings->begin (); d != drawings->end (); ++d, ++di) {
          for (unsigned int i = 0; i < d->num_planes (); ++i) {
            if (mp_drawing_plane_buffers.size () > di && mp_drawing_plane_buffers[di].size () > i) {
              mp_drawing_plane_buffers[di][i]->clear ();
            }
          }
        }

      } else if (size_t (*l) < mp_plane_buffers.size ()) {
        mp_plane_buffers[*l]->clear ();
      }

    }

  } else {

    m_width = width;
    m_height = height;

    //  delete the buffers and create new ones 
    clear_planes ();

    for (unsigned int i = 0; i < nlayers; ++i) {
      mp_plane_buffers.push_back (new lay::Bitmap (width, height, resolution));
    }

    for (lay::Drawings::const_iterator d = drawings->begin (); d != drawings->end (); ++d) {
      mp_drawing_plane_buffers.push_back (std::vector <lay::Bitmap *> ());
      for (unsigned int i = 0; i < d->num_planes (); ++i) {
        mp_drawing_plane_buffers.back ().push_back (new lay::Bitmap (width, height, resolution));
      }
    }

  }

  unlock ();
}

void 
BitmapRedrawThreadCanvas::set_plane (unsigned int n, const lay::CanvasPlane *plane)
{ 
  lock ();
  if (n < mp_plane_buffers.size ()) {
    const lay::Bitmap *bitmap = dynamic_cast<const lay::Bitmap *> (plane);
    tl_assert (bitmap != 0);
    *(mp_plane_buffers [n]) = *bitmap; 
  }
  unlock ();
}

void 
BitmapRedrawThreadCanvas::set_drawing_plane (unsigned int d, unsigned int n, const lay::CanvasPlane *plane)
{ 
  lock ();
  if (d < mp_drawing_plane_buffers.size () && n < mp_drawing_plane_buffers [d].size ()) {
    const lay::Bitmap *bitmap = dynamic_cast<const lay::Bitmap *> (plane);
    tl_assert (bitmap != 0);
    *(mp_drawing_plane_buffers [d][n]) = *bitmap; 
  }
  unlock ();
}

void 
BitmapRedrawThreadCanvas::clear_planes ()
{
  while (! mp_plane_buffers.empty ()) {
    delete mp_plane_buffers.back ();
    mp_plane_buffers.pop_back ();
  }

  while (! mp_drawing_plane_buffers.empty ()) {
    while (! mp_drawing_plane_buffers.back ().empty ()) {
      delete mp_drawing_plane_buffers.back ().back ();
      mp_drawing_plane_buffers.back ().pop_back ();
    }
    mp_drawing_plane_buffers.pop_back ();
  }
}

lay::CanvasPlane *
BitmapRedrawThreadCanvas::create_drawing_plane ()
{
  return new lay::Bitmap(m_width, m_height, resolution ());
}

void 
BitmapRedrawThreadCanvas::initialize_plane (lay::CanvasPlane *plane, unsigned int n)
{
  lock ();
  if (n < mp_plane_buffers.size ()) {
    lay::Bitmap *bitmap = dynamic_cast<lay::Bitmap *> (plane);
    tl_assert (bitmap != 0);
    *bitmap = *(mp_plane_buffers [n]);
  }
  unlock ();
}

void 
BitmapRedrawThreadCanvas::initialize_plane (lay::CanvasPlane *plane, unsigned int d, unsigned int n)
{
  lock ();
  if (d < mp_drawing_plane_buffers.size () && n < mp_drawing_plane_buffers [d].size ()) {
    lay::Bitmap *bitmap = dynamic_cast<lay::Bitmap *> (plane);
    tl_assert (bitmap != 0);
    *bitmap = *(mp_drawing_plane_buffers [d][n]);
  }
  unlock ();
}

void
BitmapRedrawThreadCanvas::to_image (const std::vector <lay::ViewOp> &view_ops, const lay::DitherPattern &dp, const lay::LineStyles &ls, double dpr, tl::Color background, tl::Color foreground, tl::Color active, const lay::Drawings *drawings, tl::PixelBuffer &img, unsigned int width, unsigned int height)
{
  if (width > m_width) {
    width = m_width;
  }
  if (height > m_height) {
    height = m_height;
  }

  //  convert the plane data to image data
  bitmaps_to_image (view_ops, mp_plane_buffers, dp, ls, dpr, &img, width, height, true, &mutex ());

  //  convert the planes of the "drawing" objects too:
  std::vector <std::vector <lay::Bitmap *> >::const_iterator bt = mp_drawing_plane_buffers.begin ();
  for (lay::Drawings::const_iterator d = drawings->begin (); d != drawings->end () && bt != mp_drawing_plane_buffers.end (); ++d, ++bt) {
    bitmaps_to_image (d->get_view_ops (*this, background, foreground, active), *bt, dp, ls, dpr, &img, width, height, true, &mutex ());
  }
}

void
BitmapRedrawThreadCanvas::to_image_mono (const std::vector <lay::ViewOp> &view_ops, const lay::DitherPattern &dp, const lay::LineStyles &ls, double dpr, bool background, bool foreground, bool active, const lay::Drawings *drawings, tl::BitmapBuffer &img, unsigned int width, unsigned int height)
{
  if (width > m_width) {
    width = m_width;
  }
  if (height > m_height) {
    height = m_height;
  }

  unsigned int all_one = 0xffffffff;

  //  convert the plane data to image data
  bitmaps_to_image (view_ops, mp_plane_buffers, dp, ls, dpr, &img, width, height, true, &mutex ());

  //  convert the planes of the "drawing" objects too:
  std::vector <std::vector <lay::Bitmap *> >::const_iterator bt = mp_drawing_plane_buffers.begin ();
  for (lay::Drawings::const_iterator d = drawings->begin (); d != drawings->end () && bt != mp_drawing_plane_buffers.end (); ++d, ++bt) {
    bitmaps_to_image (d->get_view_ops (*this, background ? all_one : 0, foreground ? all_one : 0, active ? all_one : 0), *bt, dp, ls, dpr, &img, width, height, true, &mutex ());
  }
}

}

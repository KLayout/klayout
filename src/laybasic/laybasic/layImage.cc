
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

#include "layImage.h"
#include "tlAssert.h"

namespace lay
{

Image::Image (unsigned int w, unsigned int h, lay::color_t *data)
{
  m_width = w;
  m_height = h;
  m_transparent = false;
  m_data.reset (new ImageData (data, w * h));
}

Image::Image (unsigned int w, unsigned int h, const lay::color_t *data, unsigned int stride)
{
  m_width = w;
  m_height = h;
  m_transparent = false;

  lay::color_t *d = new color_t [w * h];

  if (data) {
    for (unsigned int i = 0; i < h; ++i) {
      for (unsigned int i = 0; i < h; ++i) {
        *d++ = *data++;
      }
      if (stride > w) {
        data += stride - w;
      }
    }
  }

  m_data.reset (new ImageData (d, w * h));
}

Image::Image ()
{
  m_width = 0;
  m_height = 0;
  m_transparent = false;
}

Image::Image (const Image &other)
{
  operator= (other);
}

Image::Image (Image &&other)
{
  swap (other);
}

Image::~Image ()
{
  //  .. nothing yet ..
}

Image &
Image::operator= (const Image &other)
{
  if (this != &other) {
    m_width = other.m_width;
    m_height = other.m_height;
    m_data = other.m_data;
    m_transparent = other.m_transparent;
  }
  return *this;
}

Image &
Image::operator= (Image &&other)
{
  if (this != &other) {
    swap (other);
  }
  return *this;
}

void
Image::set_transparent (bool f)
{
  m_transparent = f;
}

void
Image::swap (Image &other)
{
  if (this == &other) {
    return;
  }

  std::swap (m_width, other.m_width);
  std::swap (m_height, other.m_height);
  std::swap (m_transparent, other.m_transparent);
  m_data.swap (other.m_data);
}

void
Image::fill (lay::color_t c)
{
  color_t *d = data ();
  for (unsigned int i = 0; i < m_height; ++i) {
    for (unsigned int j = 0; j < m_width; ++j) {
      *d++ = c;
    }
  }
}

color_t *
Image::scan_line (unsigned int n)
{
  tl_assert (n < m_height);
  return m_data->data () + n * m_width;
}

const color_t *
Image::scan_line (unsigned int n) const
{
  tl_assert (n < m_height);
  return m_data->data () + n * m_width;
}

color_t *
Image::data ()
{
  return m_data->data ();
}

const color_t *
Image::data () const
{
  return m_data->data ();
}

#if defined(HAVE_QT)
QImage
Image::to_image () const
{
  return QImage ((const uchar *) data (), m_width, m_height, m_transparent ? QImage::Format_ARGB32 : QImage::Format_RGB32);
}
#endif

void
Image::patch (const Image &other)
{
  tl_assert (width () == other.width ());
  tl_assert (height () == other.height ());
  tl_assert (other.transparent ());

  const color_t *d = other.data ();
  color_t *dd = data ();
  for (unsigned int i = 0; i < m_height; ++i) {
    for (unsigned int j = 0; j < m_width; ++j) {
      color_t c = *d++;
      if ((c & 0x80000000) != 0) {
        *dd = c;
      }
      ++dd;
    }
  }
}

Image
Image::diff (const Image &other) const
{
  tl_assert (width () == other.width ());
  tl_assert (height () == other.height ());

  Image res (m_width, m_height);
  res.set_transparent (true);

  const color_t *d2 = other.data ();
  const color_t *d1 = data ();
  color_t *dd = res.data ();

  for (unsigned int i = 0; i < m_height; ++i) {
    for (unsigned int j = 0; j < m_width; ++j) {
      if (((*d1 ^ *d2) & 0xffffff) != 0) {
        *dd++ = *d2 | 0xff000000;
      } else {
        *dd++ = 0;
      }
      ++d1;
      ++d2;
    }
  }

  return res;
}

}

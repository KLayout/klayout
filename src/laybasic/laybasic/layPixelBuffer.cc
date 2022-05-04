
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

#include "layPixelBuffer.h"
#include "tlAssert.h"

namespace lay
{

// -----------------------------------------------------------------------------------------------------
//  PixelBuffer implementation

PixelBuffer::PixelBuffer (unsigned int w, unsigned int h, lay::color_t *data)
  : m_data ()
{
  m_width = w;
  m_height = h;
  m_transparent = false;
  m_data.reset (new ImageData (data, w * h));
}

PixelBuffer::PixelBuffer (unsigned int w, unsigned int h, const lay::color_t *data, unsigned int stride)
  : m_data ()
{
  m_width = w;
  m_height = h;
  m_transparent = false;

  tl_assert ((stride % sizeof (lay::color_t)) == 0);
  stride /= sizeof (lay::color_t);

  lay::color_t *d = new lay::color_t [w * h];
  lay::color_t *new_data = d;

  if (data) {
    for (unsigned int i = 0; i < h; ++i) {
      for (unsigned int j = 0; j < w; ++j) {
        *d++ = *data++;
      }
      if (stride > w) {
        data += stride - w;
      }
    }
  }

  m_data.reset (new ImageData (new_data, w * h));
}

PixelBuffer::PixelBuffer ()
{
  m_width = 0;
  m_height = 0;
  m_transparent = false;
}

PixelBuffer::PixelBuffer (const PixelBuffer &other)
{
  operator= (other);
}

PixelBuffer::PixelBuffer (PixelBuffer &&other)
{
  swap (other);
}

PixelBuffer::~PixelBuffer ()
{
  //  .. nothing yet ..
}

PixelBuffer &
PixelBuffer::operator= (const PixelBuffer &other)
{
  if (this != &other) {
    m_width = other.m_width;
    m_height = other.m_height;
    m_data = other.m_data;
    m_transparent = other.m_transparent;
  }
  return *this;
}

PixelBuffer &
PixelBuffer::operator= (PixelBuffer &&other)
{
  if (this != &other) {
    swap (other);
  }
  return *this;
}

void
PixelBuffer::set_transparent (bool f)
{
  m_transparent = f;
}

void
PixelBuffer::swap (PixelBuffer &other)
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
PixelBuffer::fill (lay::color_t c)
{
  color_t *d = data ();
  for (unsigned int i = 0; i < m_height; ++i) {
    for (unsigned int j = 0; j < m_width; ++j) {
      *d++ = c;
    }
  }
}

color_t *
PixelBuffer::scan_line (unsigned int n)
{
  tl_assert (n < m_height);
  return m_data->data () + n * m_width;
}

const color_t *
PixelBuffer::scan_line (unsigned int n) const
{
  tl_assert (n < m_height);
  return m_data->data () + n * m_width;
}

color_t *
PixelBuffer::data ()
{
  return m_data->data ();
}

const color_t *
PixelBuffer::data () const
{
  return m_data->data ();
}

#if defined(HAVE_QT)
QImage
PixelBuffer::to_image () const
{
  return QImage ((const uchar *) data (), m_width, m_height, m_transparent ? QImage::Format_ARGB32 : QImage::Format_RGB32);
}

QImage
PixelBuffer::to_image_copy () const
{
  QImage img (m_width, m_height, m_transparent ? QImage::Format_ARGB32 : QImage::Format_RGB32);
  memcpy (img.bits (), data (), img.sizeInBytes ());
  return img;
}
#endif

void
PixelBuffer::patch (const PixelBuffer &other)
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

PixelBuffer
PixelBuffer::diff (const PixelBuffer &other) const
{
  tl_assert (width () == other.width ());
  tl_assert (height () == other.height ());

  PixelBuffer res (m_width, m_height);
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

// -----------------------------------------------------------------------------------------------------
//  BitmapBuffer implementation

static unsigned int
stride_from_width (unsigned int w)
{
  //  Qt needs 32bit-aligned data
  return 4 * ((w + 31) / 32);
}

BitmapBuffer::BitmapBuffer (unsigned int w, unsigned int h, uint8_t *data)
{
  m_width = w;
  m_height = h;
  m_stride = stride_from_width (w);
  m_data.reset (new MonoImageData (data, m_stride * h));
}

BitmapBuffer::BitmapBuffer (unsigned int w, unsigned int h, const uint8_t *data, unsigned int stride)
{
  m_width = w;
  m_height = h;
  m_stride = stride_from_width (w);

  uint8_t *d = new uint8_t [m_stride * h];
  uint8_t *new_data = d;

  if (data) {
    for (unsigned int i = 0; i < h; ++i) {
      memcpy (d, data, m_stride);
      d += m_stride;
      data += m_stride;
      if (stride > m_stride) {
        data += stride - m_stride;
      }
    }
  }

  m_data.reset (new MonoImageData (new_data, m_stride * h));
}

BitmapBuffer::BitmapBuffer ()
{
  m_width = 0;
  m_height = 0;
  m_stride = 0;
}

BitmapBuffer::BitmapBuffer (const BitmapBuffer &other)
{
  operator= (other);
}

BitmapBuffer::BitmapBuffer (BitmapBuffer &&other)
{
  swap (other);
}

BitmapBuffer::~BitmapBuffer ()
{
  //  .. nothing yet ..
}

BitmapBuffer &
BitmapBuffer::operator= (const BitmapBuffer &other)
{
  if (this != &other) {
    m_width = other.m_width;
    m_height = other.m_height;
    m_stride = other.m_stride;
    m_data = other.m_data;
  }
  return *this;
}

BitmapBuffer &
BitmapBuffer::operator= (BitmapBuffer &&other)
{
  if (this != &other) {
    swap (other);
  }
  return *this;
}

void
BitmapBuffer::swap (BitmapBuffer &other)
{
  if (this == &other) {
    return;
  }

  std::swap (m_width, other.m_width);
  std::swap (m_height, other.m_height);
  std::swap (m_stride, other.m_stride);
  m_data.swap (other.m_data);
}

void
BitmapBuffer::fill (bool value)
{
  uint8_t c = value ? 0xff : 0;
  uint8_t *d = data ();
  for (unsigned int i = 0; i < m_height; ++i) {
    for (unsigned int j = 0; j < m_stride; ++j) {
      *d++ = c;
    }
  }
}

uint8_t *
BitmapBuffer::scan_line (unsigned int n)
{
  tl_assert (n < m_height);
  return m_data->data () + n * m_stride;
}

const uint8_t *
BitmapBuffer::scan_line (unsigned int n) const
{
  tl_assert (n < m_height);
  return m_data->data () + n * m_stride;
}

uint8_t *
BitmapBuffer::data ()
{
  return m_data->data ();
}

const uint8_t *
BitmapBuffer::data () const
{
  return m_data->data ();
}

#if defined(HAVE_QT)
QImage
BitmapBuffer::to_image () const
{
  QImage img = QImage ((const uchar *) data (), m_width, m_height, QImage::Format_MonoLSB);
  img.setColor (0, 0xff000000);
  img.setColor (1, 0xffffffff);
  return img;
}

QImage
BitmapBuffer::to_image_copy () const
{
  QImage img (m_width, m_height, QImage::Format_MonoLSB);
  memcpy (img.bits (), data (), img.sizeInBytes ());
  return img;
}
#endif

}

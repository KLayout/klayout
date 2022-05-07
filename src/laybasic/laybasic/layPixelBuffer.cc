
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
#include "tlLog.h"

#if defined(HAVE_PNG)
#  include <png.h>
#endif

namespace lay
{

// -----------------------------------------------------------------------------------------------------
//  Exceptions

PixelBufferReadError::PixelBufferReadError (const char *msg)
  : tl::Exception (tl::to_string (tr ("PNG read error: ")) + std::string (msg))
{
  //  .. nothing yet ..
}

PixelBufferReadError::PixelBufferReadError (const std::string &msg)
  : tl::Exception (tl::to_string (tr ("PNG read error: ")) + msg)
{
  //  .. nothing yet ..
}

PixelBufferWriteError::PixelBufferWriteError (const char *msg)
  : tl::Exception (tl::to_string (tr ("PNG write error: ")) + std::string (msg))
{
  //  .. nothing yet ..
}

PixelBufferWriteError::PixelBufferWriteError (const std::string &msg)
  : tl::Exception (tl::to_string (tr ("PNG write error: ")) + msg)
{
  //  .. nothing yet ..
}

#if defined(HAVE_PNG)

static void png_read_warn_f (png_structp /*png_ptr*/, png_const_charp error_message)
{
  tl::warn << tl::to_string (tr ("Warning reading PNG: ")) << error_message;
}

static void png_read_error_f (png_structp /*png_ptr*/, png_const_charp error_message)
{
  throw PixelBufferReadError (error_message);
}

static void png_write_warn_f (png_structp /*png_ptr*/, png_const_charp error_message)
{
  tl::warn << tl::to_string (tr ("Warning writing PNG: ")) << error_message;
}

static void png_write_error_f (png_structp /*png_ptr*/, png_const_charp error_message)
{
  throw PixelBufferReadError (error_message);
}

static void read_from_stream_f (png_structp png_ptr, png_bytep bytes, size_t length)
{
  tl::InputStream *stream = (tl::InputStream *) png_get_io_ptr (png_ptr);
  try {
    memcpy (bytes, stream->get (length), length);
  } catch (tl::Exception &ex) {
    png_error (png_ptr, ex.msg ().c_str ());
  }
}

static void write_to_stream_f (png_structp png_ptr, png_bytep bytes, size_t length)
{
  tl::OutputStream *stream = (tl::OutputStream *) png_get_io_ptr (png_ptr);
  try {
    stream->put ((const char *) bytes, length);
  } catch (tl::Exception &ex) {
    png_error (png_ptr, ex.msg ().c_str ());
  }
}

static void flush_stream_f (png_structp png_ptr)
{
  tl::OutputStream *stream = (tl::OutputStream *) png_get_io_ptr (png_ptr);
  stream->flush ();
}

#endif

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

#if defined(HAVE_PNG)

PixelBuffer
PixelBuffer::read_png (tl::InputStream &input)
{
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;

  png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, &png_read_error_f, &png_read_warn_f);
  tl_assert (png_ptr != NULL);

  info_ptr = png_create_info_struct (png_ptr);
  tl_assert (info_ptr != NULL);

  png_set_read_fn (png_ptr, (void *) &input, &read_from_stream_f);
  png_set_bgr (png_ptr);    // compatible with lay::color_t

  png_read_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  PixelBuffer res (png_get_image_width (png_ptr, info_ptr), png_get_image_height (png_ptr, info_ptr));

  unsigned int fmt = png_get_color_type (png_ptr, info_ptr);
  unsigned int bd = png_get_bit_depth (png_ptr, info_ptr);

  if (fmt == PNG_COLOR_TYPE_RGBA && bd == 8) {

    tl_assert (png_get_rowbytes (png_ptr, info_ptr) == res.width () * sizeof (lay::color_t));

    png_bytepp row_pointers = png_get_rows (png_ptr, info_ptr);
    for (unsigned int i = 0; i < res.height (); ++i) {
      memcpy ((void *) res.scan_line (i), (void *) row_pointers [i], sizeof (lay::color_t) * res.width ());
    }

  } else if (fmt == PNG_COLOR_TYPE_RGB && bd == 8) {

    //  RGB has 3 bytes per pixel which need to be transformed into RGB32

    unsigned int rb = png_get_rowbytes (png_ptr, info_ptr);
    tl_assert (rb == res.width () * 3);

    png_bytepp row_pointers = png_get_rows (png_ptr, info_ptr);
    for (unsigned int i = 0; i < res.height (); ++i) {
      lay::color_t *c = res.scan_line (i);
      const uint8_t *d = row_pointers [i];
      const uint8_t *dd = d + rb;
      while (d < dd) {
        uint8_t b = *d++;
        uint8_t g = *d++;
        uint8_t r = *d++;
        *c++ = 0xff000000 | ((r << 8 | g) << 8) | b;
      }
    }

  } else {

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    throw PixelBufferReadError (tl::sprintf (tl::to_string (tr ("PNG reader supports 32 bit RGB or RGBA only (file: %s, format is %d, bit depth is %d)")), input.filename (), fmt, bd));

  }

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  return res;
}

void
PixelBuffer::write_png (tl::OutputStream &output)
{
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, &png_write_error_f, &png_write_warn_f);
  tl_assert (png_ptr != NULL);

  info_ptr = png_create_info_struct (png_ptr);
  tl_assert (info_ptr != NULL);

  png_set_write_fn (png_ptr, (void *) &output, &write_to_stream_f, &flush_stream_f);
  png_set_bgr (png_ptr);    // compatible with lay::color_t

  unsigned int bd = 8;  // bit depth
  unsigned int fmt = PNG_COLOR_TYPE_RGBA;

  png_set_IHDR (png_ptr, info_ptr, width (), height (), bd, fmt, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_write_info (png_ptr, info_ptr);

  for (unsigned int i = 0; i < height (); ++i) {
    png_write_row (png_ptr, png_const_bytep (scan_line (i)));
  }

  png_write_end (png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
}

#endif

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

#if defined(HAVE_PNG)

BitmapBuffer
BitmapBuffer::read_png (tl::InputStream &input)
{
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;

  png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, &png_read_error_f, &png_read_warn_f);
  tl_assert (png_ptr != NULL);

  info_ptr = png_create_info_struct (png_ptr);
  tl_assert (info_ptr != NULL);

  png_set_read_fn (png_ptr, (void *) &input, &read_from_stream_f);
  png_set_packswap (png_ptr); // compatible with BitmapBuffer

  png_read_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  BitmapBuffer res (png_get_image_width (png_ptr, info_ptr), png_get_image_height (png_ptr, info_ptr));

  unsigned int fmt = png_get_color_type (png_ptr, info_ptr);
  unsigned int bd = png_get_bit_depth (png_ptr, info_ptr);

  if ((fmt == PNG_COLOR_TYPE_GRAY || fmt == PNG_COLOR_TYPE_PALETTE) && bd == 1) {

    //  TODO: evaluate palette?

    size_t rb = png_get_rowbytes (png_ptr, info_ptr);
    tl_assert (rb == (res.width () + 7) / 8);

    png_bytepp row_pointers = png_get_rows (png_ptr, info_ptr);
    for (unsigned int i = 0; i < res.height (); ++i) {
      memcpy ((void *) res.scan_line (i), (void *) row_pointers [i], rb);
    }

  } else {

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    throw PixelBufferReadError (tl::sprintf (tl::to_string (tr ("PNG bitmap reader supports monochrome files only (file: %s, format is %d, bit depth is %d)")), input.filename (), fmt, bd));

  }

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  return res;
}

void
BitmapBuffer::write_png (tl::OutputStream &output)
{
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, &png_write_error_f, &png_write_warn_f);
  tl_assert (png_ptr != NULL);

  info_ptr = png_create_info_struct (png_ptr);
  tl_assert (info_ptr != NULL);

  png_set_write_fn (png_ptr, (void *) &output, &write_to_stream_f, &flush_stream_f);
  png_set_packswap (png_ptr); // compatible with BitmapBuffer

  unsigned int bd = 1;  // bit depth
  unsigned int fmt = PNG_COLOR_TYPE_GRAY;

  png_set_IHDR (png_ptr, info_ptr, width (), height (), bd, fmt, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_write_info (png_ptr, info_ptr);

  for (unsigned int i = 0; i < height (); ++i) {
    png_write_row (png_ptr, png_const_bytep (scan_line (i)));
  }

  png_write_end (png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
}

#endif


}

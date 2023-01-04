
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

#include "tlPixelBuffer.h"
#include "tlAssert.h"
#include "tlLog.h"

#if defined(HAVE_PNG)
#  include <png.h>
#endif

#include <memory>
#include <cmath>

namespace tl
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

PixelBuffer::PixelBuffer (unsigned int w, unsigned int h, tl::color_t *data)
  : m_data ()
{
  m_width = w;
  m_height = h;
  m_transparent = false;
  m_data.reset (new ImageData (data, w * h));
}

PixelBuffer::PixelBuffer (unsigned int w, unsigned int h, const tl::color_t *data, unsigned int stride)
  : m_data ()
{
  m_width = w;
  m_height = h;
  m_transparent = false;

  tl_assert ((stride % sizeof (tl::color_t)) == 0);
  stride /= sizeof (tl::color_t);

  tl::color_t *d = new tl::color_t [w * h];
  tl::color_t *new_data = d;

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

bool
PixelBuffer::operator== (const PixelBuffer &other) const
{
  if (width () != other.width () || height () != other.height ()) {
    return false;
  }
  if (transparent () != other.transparent ()) {
    return false;
  }

  tl::color_t m = transparent () ? 0xffffffff : 0xffffff;
  for (unsigned int i = 0; i < other.height (); ++i) {
    const tl::color_t *d = scan_line (i);
    const tl::color_t *de = d + width ();
    const tl::color_t *dd = other.scan_line (i);
    while (d != de) {
      if (((*d++ ^ *dd++) & m) != 0) {
        return false;
      }
    }
  }

  return true;
}

PixelBuffer &
PixelBuffer::operator= (const PixelBuffer &other)
{
  if (this != &other) {
    m_width = other.m_width;
    m_height = other.m_height;
    m_data = other.m_data;
    m_transparent = other.m_transparent;
    m_texts = other.m_texts;
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
  m_texts.swap (other.m_texts);
}

void
PixelBuffer::fill (tl::color_t c)
{
  if (! transparent ()) {
    c |= 0xff000000;  //  ensures that alpha is properly set
  }

  tl::color_t *d = data ();
  for (unsigned int i = 0; i < m_height; ++i) {
    for (unsigned int j = 0; j < m_width; ++j) {
      *d++ = c;
    }
  }
}

tl::color_t *
PixelBuffer::scan_line (unsigned int n)
{
  tl_assert (n < m_height);
  return m_data->data () + n * m_width;
}

const tl::color_t *
PixelBuffer::scan_line (unsigned int n) const
{
  tl_assert (n < m_height);
  return m_data->data () + n * m_width;
}

tl::color_t *
PixelBuffer::data ()
{
  return m_data->data ();
}

const tl::color_t *
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
#if QT_VERSION < 0x051000
  memcpy (img.bits (), data (), img.byteCount ());
#else
  memcpy (img.bits (), data (), img.sizeInBytes ());
#endif
  return img;
}

PixelBuffer
PixelBuffer::from_image (const QImage &img)
{
  if (img.format () != QImage::Format_ARGB32 && img.format () != QImage::Format_RGB32) {
    QImage img_argb32 = img.convertToFormat (QImage::Format_ARGB32);
    return PixelBuffer (img_argb32.width (), img_argb32.height (), (const tl::color_t *) img_argb32.bits ());
  } else {
    return PixelBuffer (img.width (), img.height (), (const tl::color_t *) img.bits ());
  }
}
#endif

void
PixelBuffer::patch (const PixelBuffer &other)
{
  tl_assert (width () == other.width ());
  tl_assert (height () == other.height ());
  tl_assert (other.transparent ());

  const tl::color_t *d = other.data ();
  tl::color_t *dd = data ();
  for (unsigned int i = 0; i < m_height; ++i) {
    for (unsigned int j = 0; j < m_width; ++j) {
      tl::color_t c = *d++;
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

  const tl::color_t *d2 = other.data ();
  const tl::color_t *d1 = data ();
  tl::color_t *dd = res.data ();

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

void
PixelBuffer::blowup (tl::PixelBuffer &dest, unsigned int os)
{
  tl_assert (dest.width () == width () * os);
  tl_assert (dest.height () == height () * os);

  unsigned int ymax = height ();
  unsigned int xmax = width ();

  for (unsigned int y = 0; y < ymax; ++y) {
    for (unsigned int i = 0; i < os; ++i) {
      const uint32_t *psrc = (const uint32_t *) scan_line (y);
      uint32_t *pdest = (uint32_t *) dest.scan_line (y * os + i);
      for (unsigned int x = 0; x < xmax; ++x) {
        for (unsigned int j = 0; j < os; ++j) {
          *pdest++ = *psrc;
        }
        ++psrc;
      }
    }
  }
}

void
PixelBuffer::subsample (tl::PixelBuffer &dest, unsigned int os, double g)
{
  //  TODO: this is probably not compatible with the endianess of SPARC ..

  //  LUT's for combining the RGB channels

  //  forward transformation table
  unsigned short lut1[256];
  for (unsigned int i = 0; i < 256; ++i) {
    double f = (65536 / (os * os)) - 1;
    lut1[i] = (unsigned short)std::min (f, std::max (0.0, floor (0.5 + pow (i / 255.0, g) * f)));
  }

  //  backward transformation table
  unsigned char lut2[65536];
  for (unsigned int i = 0; i < 65536; ++i) {
    double f = os * os * ((65536 / (os * os)) - 1);
    lut2[i] = (unsigned char)std::min (255.0, std::max (0.0, floor (0.5 + pow (i / f, 1.0 / g) * 255.0)));
  }

  //  LUT's for alpha channel

  //  forward transformation table
  unsigned short luta1[256];
  for (unsigned int i = 0; i < 256; ++i) {
    double f = (65536 / (os * os)) - 1;
    luta1[i] = (unsigned short)std::min (f, std::max (0.0, floor (0.5 + (i / 255.0) * f)));
  }

  //  backward transformation table
  unsigned char luta2[65536];
  for (unsigned int i = 0; i < 65536; ++i) {
    double f = os * os * ((65536 / (os * os)) - 1);
    luta2[i] = (unsigned char)std::min (255.0, std::max (0.0, floor (0.5 + (i / f) * 255.0)));
  }

  unsigned int ymax = dest.height ();
  unsigned int xmax = dest.width ();

  unsigned short *buffer = new unsigned short[xmax * 4];

  for (unsigned int y = 0; y < ymax; ++y) {

    {

      const unsigned char *psrc = (const unsigned char *) scan_line (y * os);
      unsigned short *pdest = buffer;

      for (unsigned int x = 0; x < xmax; ++x) {

        pdest[0] = lut1[psrc[0]];
        pdest[1] = lut1[psrc[1]];
        pdest[2] = lut1[psrc[2]];
        pdest[3] = luta1[psrc[3]];
        psrc += 4;

        for (unsigned int j = os; j > 1; j--) {
          pdest[0] += lut1[psrc[0]];
          pdest[1] += lut1[psrc[1]];
          pdest[2] += lut1[psrc[2]];
          pdest[3] += luta1[psrc[3]];
          psrc += 4;
        }

        pdest += 4;

      }

    }

    for (unsigned int i = 1; i < os; ++i) {

      const unsigned char *psrc = (const unsigned char *) scan_line (y * os + i);
      unsigned short *pdest = buffer;

      for (unsigned int x = 0; x < xmax; ++x) {

        for (unsigned int j = os; j > 0; j--) {
          pdest[0] += lut1[psrc[0]];
          pdest[1] += lut1[psrc[1]];
          pdest[2] += lut1[psrc[2]];
          pdest[3] += luta1[psrc[3]];
          psrc += 4;
        }

        pdest += 4;

      }

    }

    {

      unsigned char *pdest = (unsigned char *) dest.scan_line (y);
      const unsigned short *psrc = buffer;

      for (unsigned int x = 0; x < xmax; ++x) {
        *pdest++ = lut2[*psrc++];
        *pdest++ = lut2[*psrc++];
        *pdest++ = lut2[*psrc++];
        *pdest++ = luta2[*psrc++];
      }

    }

  }

  delete[] buffer;
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
  png_set_bgr (png_ptr);    // compatible with tl::color_t

  png_read_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  PixelBuffer res (png_get_image_width (png_ptr, info_ptr), png_get_image_height (png_ptr, info_ptr));

  unsigned int fmt = png_get_color_type (png_ptr, info_ptr);
  unsigned int bd = png_get_bit_depth (png_ptr, info_ptr);

  if (fmt == PNG_COLOR_TYPE_RGBA && bd == 8) {

    tl_assert (png_get_rowbytes (png_ptr, info_ptr) == res.width () * sizeof (tl::color_t));

    png_bytepp row_pointers = png_get_rows (png_ptr, info_ptr);
    for (unsigned int i = 0; i < res.height (); ++i) {
      memcpy ((void *) res.scan_line (i), (void *) row_pointers [i], sizeof (tl::color_t) * res.width ());
    }

    res.set_transparent (true);

  } else if (fmt == PNG_COLOR_TYPE_RGB && bd == 8) {

    //  RGB has 3 bytes per pixel which need to be transformed into RGB32

    unsigned int rb = png_get_rowbytes (png_ptr, info_ptr);
    tl_assert (rb == res.width () * 3);

    png_bytepp row_pointers = png_get_rows (png_ptr, info_ptr);
    for (unsigned int i = 0; i < res.height (); ++i) {
      tl::color_t *c = res.scan_line (i);
      const uint8_t *d = row_pointers [i];
      const uint8_t *dd = d + rb;
      while (d < dd) {
        tl::color_t b = *d++;
        tl::color_t g = *d++;
        tl::color_t r = *d++;
        *c++ = 0xff000000 | ((r << 8 | g) << 8) | b;
      }
    }

  } else if (fmt == PNG_COLOR_TYPE_GRAY_ALPHA && bd == 8) {

    //  GA format has 2 bytes per pixel (alpha, gray) which need to be transformed into ARGB

    unsigned int rb = png_get_rowbytes (png_ptr, info_ptr);
    tl_assert (rb == res.width () * 2);

    png_bytepp row_pointers = png_get_rows (png_ptr, info_ptr);
    for (unsigned int i = 0; i < res.height (); ++i) {
      tl::color_t *c = res.scan_line (i);
      const uint8_t *d = row_pointers [i];
      const uint8_t *dd = d + rb;
      while (d < dd) {
        tl::color_t g = *d++;
        tl::color_t a = *d++;
        *c++ = (a << 24) | ((g << 8 | g) << 8) | g;
      }
    }

    res.set_transparent (true);

  } else if (fmt == PNG_COLOR_TYPE_GRAY && bd == 8) {

    //  G format has 1 byte per pixel (gray) which need to be transformed into ARGB

    unsigned int rb = png_get_rowbytes (png_ptr, info_ptr);
    tl_assert (rb == res.width ());

    png_bytepp row_pointers = png_get_rows (png_ptr, info_ptr);
    for (unsigned int i = 0; i < res.height (); ++i) {
      tl::color_t *c = res.scan_line (i);
      const uint8_t *d = row_pointers [i];
      const uint8_t *dd = d + rb;
      while (d < dd) {
        tl::color_t g = *d++;
        *c++ = 0xff000000 | ((g << 8 | g) << 8) | g;
      }
    }

  } else {

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    throw PixelBufferReadError (tl::sprintf (tl::to_string (tr ("PNG reader supports 8 bit G, GA, RGB or RGBA files only (file: %s, format is %d, bit depth is %d)")), input.filename (), fmt, bd));

  }

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  return res;
}

void
PixelBuffer::write_png (tl::OutputStream &output) const
{
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, &png_write_error_f, &png_write_warn_f);
  tl_assert (png_ptr != NULL);

  info_ptr = png_create_info_struct (png_ptr);
  tl_assert (info_ptr != NULL);

  png_set_write_fn (png_ptr, (void *) &output, &write_to_stream_f, &flush_stream_f);
  png_set_bgr (png_ptr);    // compatible with tl::color_t

  unsigned int bd = 8;  // bit depth
  unsigned int fmt = transparent () ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB;

  png_set_IHDR (png_ptr, info_ptr, width (), height (), bd, fmt, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  std::vector<png_text> tptrs;
  for (auto i = m_texts.begin (); i != m_texts.end (); ++i) {
    tptrs.push_back (png_text ());
    tptrs.back ().compression = PNG_TEXT_COMPRESSION_NONE;
    tptrs.back ().key = const_cast<char *> (i->first.c_str ());
    tptrs.back ().text = const_cast<char *> (i->second.c_str ());
  }
  png_set_text (png_ptr, info_ptr, tptrs.begin ().operator-> (), m_texts.size ());

  png_write_info (png_ptr, info_ptr);

  if (transparent ()) {

    for (unsigned int i = 0; i < height (); ++i) {
      png_write_row (png_ptr, png_const_bytep (scan_line (i)));
    }

  } else {

    std::unique_ptr<uint8_t []> buffer (new uint8_t [width () * 3]);

    for (unsigned int i = 0; i < height (); ++i) {
      uint8_t *d = buffer.get ();
      const tl::color_t *s = scan_line (i);
      const tl::color_t *se = s + width ();
      while (s != se) {
        tl::color_t c = *s++;
        *d++ = c & 0xff;
        c >>= 8;
        *d++ = c & 0xff;
        c >>= 8;
        *d++ = c & 0xff;
      }
      png_write_row (png_ptr, png_const_bytep (buffer.get ()));
    }

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

bool
BitmapBuffer::operator== (const BitmapBuffer &other) const
{
  if (width () != other.width () || height () != other.height ()) {
    return false;
  }

  for (unsigned int i = 0; i < other.height (); ++i) {
    const uint8_t *d = scan_line (i);
    const uint8_t *dd = other.scan_line (i);
    unsigned int bits_left = width ();
    while (bits_left >= 8) {
      if (*d++ != *dd++) {
        return false;
      }
      bits_left -= 8;
    }
    if (bits_left > 0) {
      unsigned int m = (0x01 << bits_left) - 1;
      if (((*d ^ *dd) & m) != 0) {
        return false;
      }
    }
  }

  return true;
}

BitmapBuffer &
BitmapBuffer::operator= (const BitmapBuffer &other)
{
  if (this != &other) {
    m_width = other.m_width;
    m_height = other.m_height;
    m_stride = other.m_stride;
    m_data = other.m_data;
    m_texts = other.m_texts;
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
  m_texts.swap (other.m_texts);
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
#if QT_VERSION < 0x051000
  memcpy (img.bits (), data (), img.byteCount ());
#else
  memcpy (img.bits (), data (), img.sizeInBytes ());
#endif
  return img;
}

BitmapBuffer
BitmapBuffer::from_image (const QImage &img)
{
  if (img.format () != QImage::Format_MonoLSB) {
    QImage img_monolsb = img.convertToFormat (QImage::Format_MonoLSB);
    return BitmapBuffer (img_monolsb.width (), img_monolsb.height (), (const uint8_t *) img_monolsb.bits ());
  } else {
    return BitmapBuffer (img.width (), img.height (), (const uint8_t *) img.bits ());
  }
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
BitmapBuffer::write_png (tl::OutputStream &output) const
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

  std::vector<png_text> tptrs;
  for (auto i = m_texts.begin (); i != m_texts.end (); ++i) {
    tptrs.push_back (png_text ());
    tptrs.back ().compression = PNG_TEXT_COMPRESSION_NONE;
    tptrs.back ().key = const_cast<char *> (i->first.c_str ());
    tptrs.back ().text = const_cast<char *> (i->second.c_str ());
  }
  png_set_text (png_ptr, info_ptr, tptrs.begin ().operator-> (), m_texts.size ());

  png_write_info (png_ptr, info_ptr);

  for (unsigned int i = 0; i < height (); ++i) {
    png_write_row (png_ptr, png_const_bytep (scan_line (i)));
  }

  png_write_end (png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
}

#endif


}

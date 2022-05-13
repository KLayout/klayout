
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

#include "gsiDecl.h"
#include "layPixelBuffer.h"

#if defined(HAVE_QT)
#  include <QBuffer>
#endif

namespace gsi
{

// -------------------------------------------------------------------------------------
//  lay::BitmapBuffer

static lay::PixelBuffer *create_pixel_buffer (unsigned int w, unsigned int h)
{
  return new lay::PixelBuffer (w, h);
}

#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
static void fill_with_qcolor (lay::PixelBuffer *pb, QColor c)
{
  pb->fill (c.rgb ());
}
#endif

lay::color_t get_pixel_from_pixel_buffer (const lay::PixelBuffer *pb, unsigned int x, unsigned int y)
{
  if (x < pb->width () && y < pb->height ()) {
    return pb->scan_line (y)[x];
  } else {
    return lay::color_t (0);
  }
}

void set_pixel_in_pixel_buffer (lay::PixelBuffer *pb, unsigned int x, unsigned int y, lay::color_t c)
{
  if (! pb->transparent ()) {
    c |= 0xff000000;  //  ensures that alpha is set properly even if not required
  }
  if (x < pb->width () && y < pb->height ()) {
    pb->scan_line (y)[x] = c;
  }
}

static lay::PixelBuffer read_pixel_buffer (const std::string &file)
{
#if defined(HAVE_PNG)
  tl::InputStream stream (file);
  return lay::PixelBuffer::read_png (stream);
#elif defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  //  QImage is fallback
  QImage img;
  img.load (tl::to_qstring (file), "PNG");
  return lay::PixelBuffer::from_image (img);
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for PixelBuffer")));
  return lay::PixelBuffer ();
#endif
}

//  TODO: there should be some more efficient version of byte strings which avoid copies
static lay::PixelBuffer pixel_buffer_from_png (const std::vector<char> &data)
{
#if defined(HAVE_PNG)
  tl::InputMemoryStream data_stream (data.begin ().operator-> (), data.size ());
  tl::InputStream stream (data_stream);
  return lay::PixelBuffer::read_png (stream);
#elif defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  //  QImage is fallback
  tl_assert (data.size () < std::numeric_limits<int>::max ());
  QImage img = QImage::fromData ((const uchar *) data.begin ().operator-> (), int (data.size ()));
  return lay::PixelBuffer::from_image (img);
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for PixelBuffer")));
  return lay::PixelBuffer ();
#endif
}

static void write_pixel_buffer (const lay::PixelBuffer *pb, const std::string &file)
{
#if defined(HAVE_PNG)
  tl::OutputStream stream (file);
  pb->write_png (stream);
#elif defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  //  QImage is fallback
  QImage img = pb->to_image ();
  img.save (tl::to_qstring (file), "PNG");
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for PixelBuffer")));
#endif
}

//  TODO: there should be some more efficient version of byte strings which avoid copies
static std::vector<char> pixel_buffer_to_png (const lay::PixelBuffer *pb)
{
#if defined(HAVE_PNG)
  tl::OutputMemoryStream data_stream;
  {
    tl::OutputStream stream (data_stream);
    pb->write_png (stream);
  }
  return std::vector<char> (data_stream.data (), data_stream.data () + data_stream.size ());
#elif defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  //  QImage is fallback
  QImage img = pb->to_image ();
  QBuffer data;
  img.save (&data, "PNG");
  return std::vector<char> (data.data ().constData (), data.data ().constEnd ());
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for PixelBuffer")));
#endif
}


Class<lay::PixelBuffer> decl_PixelBuffer ("lay", "PixelBuffer",
  gsi::constructor ("new", &create_pixel_buffer, gsi::arg ("width"), gsi::arg ("height"),
    "@brief Creates a pixel buffer object\n"
    "\n"
    "@param width The width in pixels\n"
    "@param height The height in pixels\n"
    "\n"
    "The pixels are basically uninitialized. You will need to use \\fill to initialize them to a certain value."
  ) +
  gsi::method ("==", &lay::PixelBuffer::operator==, gsi::arg ("other"),
    "@brief Returns a value indicating whether self is identical to the other image\n"
  ) +
  gsi::method ("!=", &lay::PixelBuffer::operator!=, gsi::arg ("other"),
    "@brief Returns a value indicating whether self is not identical to the other image\n"
  ) +
  gsi::method ("transparent=", &lay::PixelBuffer::set_transparent, gsi::arg ("t"),
    "@brief Sets a flag indicating whether the pixel buffer supports an alpha channel\n"
    "\n"
    "By default, the pixel buffer does not support an alpha channel.\n"
  ) +
  gsi::method ("transparent", &lay::PixelBuffer::transparent,
    "@brief Gets a flag indicating whether the pixel buffer supports an alpha channel\n"
  ) +
  gsi::method ("fill", &lay::PixelBuffer::fill, gsi::arg ("color"),
    "@brief Fills the pixel buffer with the given pixel value\n"
  ) +
#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  gsi::method_ext ("fill", &fill_with_qcolor, gsi::arg ("color"),
    "@brief Fills the pixel buffer with the given QColor\n"
  ) +
#endif
  gsi::method ("swap", &lay::PixelBuffer::swap, gsi::arg ("other"),
    "@brief Swaps data with another PixelBuffer object\n"
  ) +
  gsi::method ("width", &lay::PixelBuffer::width,
    "@brief Gets the width of the pixel buffer in pixels\n"
  ) +
  gsi::method ("height", &lay::PixelBuffer::height,
    "@brief Gets the height of the pixel buffer in pixels\n"
  ) +
  gsi::method_ext ("set_pixel", &set_pixel_in_pixel_buffer, gsi::arg ("x"), gsi::arg ("y"), gsi::arg ("c"),
    "@brief Sets the value of the pixel at position x, y\n"
  ) +
  gsi::method_ext ("pixel", &get_pixel_from_pixel_buffer, gsi::arg ("x"), gsi::arg ("y"),
    "@brief Gets the value of the pixel at position x, y\n"
  ) +
#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  gsi::method ("to_qimage", &lay::PixelBuffer::to_image_copy,
    "@brief Converts the pixel buffer to a \\QImage object"
  ) +
  gsi::method ("from_qimage", &lay::PixelBuffer::from_image, gsi::arg ("qimage"),
    "@brief Creates a pixel buffer object from a QImage object\n"
  ) +
#endif
  gsi::method ("read_png", &read_pixel_buffer, gsi::arg ("file"),
    "@brief Reads the pixel buffer from a PNG file"
    "\n"
    "This method may not be available if PNG support is not compiled into KLayout."
  ) +
  gsi::method ("from_png_data", &pixel_buffer_from_png, gsi::arg ("data"),
    "@brief Reads the pixel buffer from a PNG byte stream"
    "\n"
    "This method may not be available if PNG support is not compiled into KLayout."
  ) +
  gsi::method_ext ("write_png", &write_pixel_buffer, gsi::arg ("file"),
    "@brief Writes the pixel buffer to a PNG file"
    "\n"
    "This method may not be available if PNG support is not compiled into KLayout."
  ) +
  gsi::method_ext ("to_png_data", &pixel_buffer_to_png,
    "@brief Converts the pixel buffer to a PNG byte stream"
    "\n"
    "This method may not be available if PNG support is not compiled into KLayout."
  ) +
  gsi::method ("patch", &lay::PixelBuffer::patch, gsi::arg ("other"),
    "@brief Patches another pixel buffer into this one\n"
    "\n"
    "This method is the inverse of \\diff - it will patch the difference image created by diff into this "
    "pixel buffer. Note that this method will not do true alpha blending and requires the other pixel buffer "
    "to have the same format than self. Self will be modified by this operation."
  ) +
  gsi::method ("diff", &lay::PixelBuffer::diff, gsi::arg ("other"),
    "@brief Creates a difference image\n"
    "\n"
    "This method is provided to support transfer of image differences - i.e. small updates instead of full images. "
    "It works for non-transparent images only and generates an image with transpareny enabled and with the new pixel values for pixels that have changed. "
    "The alpha value will be 0 for identical images and 255 for pixels with different values. "
    "This way, the difference image can be painted over the original image to generate the new image."
  ),
  "@brief A simplistic pixel buffer representing an image of ARGB32 or RGB32 values\n"
  "\n"
  "This object is mainly provided for offline rendering of layouts in Qt-less environments.\n"
  "It supports a rectangular pixel space with color values encoded in 32bit integers. It supports "
  "transparency through an optional alpha channel. The color format for a pixel is "
  "\"0xAARRGGBB\" where 'AA' is the alpha value which is ignored in non-transparent mode.\n"
  "\n"
  "This class supports basic operations such as initialization, single-pixel access and I/O to PNG.\n"
  "\n"
  "This class has been introduced in version 0.28."
);


// -------------------------------------------------------------------------------------
//  lay::BitmapBuffer

static lay::BitmapBuffer *create_bitmap_buffer (unsigned int w, unsigned int h)
{
  return new lay::BitmapBuffer (w, h);
}

bool get_pixel_from_bitmap_buffer (const lay::BitmapBuffer *pb, unsigned int x, unsigned int y)
{
  if (x < pb->width () && y < pb->height ()) {
    return (pb->scan_line (y)[x / 8] & (0x01 << (x % 8))) != 0;
  } else {
    return false;
  }
}

void set_pixel_in_bitmap_buffer (lay::BitmapBuffer *pb, unsigned int x, unsigned int y, bool c)
{
  if (x < pb->width () && y < pb->height ()) {
    if (c) {
      pb->scan_line (y)[x / 8] |= 0x01 << (x % 8);
    } else {
      pb->scan_line (y)[x / 8] &= ~(0x01 << (x % 8));
    }
  }
}

static lay::BitmapBuffer read_bitmap_buffer (const std::string &file)
{
#if defined(HAVE_PNG)
  tl::InputStream stream (file);
  return lay::BitmapBuffer::read_png (stream);
#elif defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  //  QImage is fallback
  QImage img;
  img.load (tl::to_qstring (file), "PNG");
  return lay::BitmapBuffer::from_image (img);
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for BitmapBuffer")));
  return lay::BitmapBuffer ();
#endif
}

//  TODO: there should be some more efficient version of byte strings which avoid copies
static lay::BitmapBuffer bitmap_buffer_from_png (const std::vector<char> &data)
{
#if defined(HAVE_PNG)
  tl::InputMemoryStream data_stream (data.begin ().operator-> (), data.size ());
  tl::InputStream stream (data_stream);
  return lay::BitmapBuffer::read_png (stream);
#elif defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  //  QImage is fallback
  tl_assert (data.size () < std::numeric_limits<int>::max ());
  QImage img = QImage::fromData ((const uchar *) data.begin ().operator-> (), int (data.size ()));
  return lay::BitmapBuffer::from_image (img);
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for BitmapBuffer")));
  return lay::BitmapBuffer ();
#endif
}

static void write_bitmap_buffer (const lay::BitmapBuffer *pb, const std::string &file)
{
#if defined(HAVE_PNG)
  tl::OutputStream stream (file);
  pb->write_png (stream);
#elif defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  //  QImage is fallback
  QImage img = pb->to_image ();
  img.save (tl::to_qstring (file), "PNG");
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for BitmapBuffer")));
#endif
}

//  TODO: there should be some more efficient version of byte strings which avoid copies
static std::vector<char> bitmap_buffer_to_png (const lay::BitmapBuffer *pb)
{
#if defined(HAVE_PNG)
  tl::OutputMemoryStream data_stream;
  {
    tl::OutputStream stream (data_stream);
    pb->write_png (stream);
  }
  return std::vector<char> (data_stream.data (), data_stream.data () + data_stream.size ());
#elif defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  //  QImage is fallback
  QImage img = pb->to_image ();
  QBuffer data;
  img.save (&data, "PNG");
  return std::vector<char> (data.data ().constData (), data.data ().constEnd ());
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for BitmapBuffer")));
#endif
}


Class<lay::BitmapBuffer> decl_BitmapBuffer ("lay", "BitmapBuffer",
  gsi::constructor ("new", &create_bitmap_buffer, gsi::arg ("width"), gsi::arg ("height"),
    "@brief Creates a pixel buffer object\n"
    "\n"
    "@param width The width in pixels\n"
    "@param height The height in pixels\n"
    "\n"
    "The pixels are basically uninitialized. You will need to use \\fill to initialize them to a certain value."
  ) +
  gsi::method ("==", &lay::BitmapBuffer::operator==, gsi::arg ("other"),
    "@brief Returns a value indicating whether self is identical to the other image\n"
  ) +
  gsi::method ("!=", &lay::BitmapBuffer::operator!=, gsi::arg ("other"),
    "@brief Returns a value indicating whether self is not identical to the other image\n"
  ) +
  gsi::method ("fill", &lay::BitmapBuffer::fill, gsi::arg ("color"),
    "@brief Fills the pixel buffer with the given pixel value\n"
  ) +
#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  gsi::method_ext ("fill", &fill_with_qcolor, gsi::arg ("color"),
    "@brief Fills the pixel buffer with the given QColor\n"
  ) +
#endif
  gsi::method ("swap", &lay::BitmapBuffer::swap, gsi::arg ("other"),
    "@brief Swaps data with another BitmapBuffer object\n"
  ) +
  gsi::method ("width", &lay::BitmapBuffer::width,
    "@brief Gets the width of the pixel buffer in pixels\n"
  ) +
  gsi::method ("height", &lay::BitmapBuffer::height,
    "@brief Gets the height of the pixel buffer in pixels\n"
  ) +
  gsi::method_ext ("set_pixel", &set_pixel_in_bitmap_buffer, gsi::arg ("x"), gsi::arg ("y"), gsi::arg ("c"),
    "@brief Sets the value of the pixel at position x, y\n"
  ) +
  gsi::method_ext ("pixel", &get_pixel_from_bitmap_buffer, gsi::arg ("x"), gsi::arg ("y"),
    "@brief Gets the value of the pixel at position x, y\n"
  ) +
#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  gsi::method ("to_qimage", &lay::BitmapBuffer::to_image_copy,
    "@brief Converts the pixel buffer to a \\QImage object"
  ) +
  gsi::method ("from_qimage", &lay::BitmapBuffer::from_image, gsi::arg ("qimage"),
    "@brief Creates a pixel buffer object from a QImage object\n"
  ) +
#endif
  gsi::method ("read_png", &read_bitmap_buffer, gsi::arg ("file"),
    "@brief Reads the pixel buffer from a PNG file"
    "\n"
    "This method may not be available if PNG support is not compiled into KLayout."
  ) +
  gsi::method ("from_png_data", &bitmap_buffer_from_png, gsi::arg ("data"),
    "@brief Reads the pixel buffer from a PNG byte stream"
    "\n"
    "This method may not be available if PNG support is not compiled into KLayout."
  ) +
  gsi::method_ext ("write_png", &write_bitmap_buffer, gsi::arg ("file"),
    "@brief Writes the pixel buffer to a PNG file"
    "\n"
    "This method may not be available if PNG support is not compiled into KLayout."
  ) +
  gsi::method_ext ("to_png_data", &bitmap_buffer_to_png,
    "@brief Converts the pixel buffer to a PNG byte stream"
    "\n"
    "This method may not be available if PNG support is not compiled into KLayout."
  ),
  "@brief A simplistic pixel buffer representing monochrome image\n"
  "\n"
  "This object is mainly provided for offline rendering of layouts in Qt-less environments.\n"
  "It supports a rectangular pixel space with color values encoded in single bits.\n"
  "\n"
  "This class supports basic operations such as initialization, single-pixel access and I/O to PNG.\n"
  "\n"
  "This class has been introduced in version 0.28."
);

}

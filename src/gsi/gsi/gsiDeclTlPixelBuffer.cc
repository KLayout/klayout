
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

#include "gsiDecl.h"
#include "tlPixelBuffer.h"

#if defined(HAVE_QT)
#  include <QBuffer>
#endif

namespace gsi
{

// -------------------------------------------------------------------------------------
//  tl::BitmapBuffer

static tl::PixelBuffer *create_pixel_buffer (unsigned int w, unsigned int h)
{
  return new tl::PixelBuffer (w, h);
}

tl::color_t get_pixel_from_pixel_buffer (const tl::PixelBuffer *pb, unsigned int x, unsigned int y)
{
  if (x < pb->width () && y < pb->height ()) {
    return pb->scan_line (y)[x];
  } else {
    return tl::color_t (0);
  }
}

void set_pixel_in_pixel_buffer (tl::PixelBuffer *pb, unsigned int x, unsigned int y, tl::color_t c)
{
  if (! pb->transparent ()) {
    c |= 0xff000000;  //  ensures that alpha is set properly even if not required
  }
  if (x < pb->width () && y < pb->height ()) {
    pb->scan_line (y)[x] = c;
  }
}

static tl::PixelBuffer read_pixel_buffer (const std::string &file)
{
#if defined(HAVE_PNG)
  tl::InputStream stream (file);
  return tl::PixelBuffer::read_png (stream);
#elif defined(HAVE_QT)
  //  QImage is fallback
  QImage img;
  img.load (tl::to_qstring (file), "PNG");
  return tl::PixelBuffer::from_image (img);
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for PixelBuffer")));
  return tl::PixelBuffer ();
#endif
}

//  TODO: there should be some more efficient version of byte strings which avoid copies
static tl::PixelBuffer pixel_buffer_from_png (const std::vector<char> &data)
{
#if defined(HAVE_PNG)
  tl::InputMemoryStream data_stream (data.begin ().operator-> (), data.size ());
  tl::InputStream stream (data_stream);
  return tl::PixelBuffer::read_png (stream);
#elif defined(HAVE_QT)
  //  QImage is fallback
  tl_assert (data.size () < std::numeric_limits<int>::max ());
  QImage img = QImage::fromData ((const uchar *) data.begin ().operator-> (), int (data.size ()));
  return tl::PixelBuffer::from_image (img);
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for PixelBuffer")));
  return tl::PixelBuffer ();
#endif
}

static void write_pixel_buffer (const tl::PixelBuffer *pb, const std::string &file)
{
#if defined(HAVE_PNG)
  tl::OutputStream stream (file);
  pb->write_png (stream);
#elif defined(HAVE_QT)
  //  QImage is fallback
  QImage img = pb->to_image ();
  img.save (tl::to_qstring (file), "PNG");
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for PixelBuffer")));
#endif
}

//  TODO: there should be some more efficient version of byte strings which avoid copies
static std::vector<char> pixel_buffer_to_png (const tl::PixelBuffer *pb)
{
#if defined(HAVE_PNG)
  tl::OutputMemoryStream data_stream;
  {
    tl::OutputStream stream (data_stream);
    pb->write_png (stream);
  }
  return std::vector<char> (data_stream.data (), data_stream.data () + data_stream.size ());
#elif defined(HAVE_QT)
  //  QImage is fallback
  QImage img = pb->to_image ();
  QBuffer data;
  img.save (&data, "PNG");
  return std::vector<char> (data.data ().constData (), data.data ().constEnd ());
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for PixelBuffer")));
#endif
}


Class<tl::PixelBuffer> decl_PixelBuffer ("lay", "PixelBuffer",
  gsi::constructor ("new", &create_pixel_buffer, gsi::arg ("width"), gsi::arg ("height"),
    "@brief Creates a pixel buffer object\n"
    "\n"
    "@param width The width in pixels\n"
    "@param height The height in pixels\n"
    "\n"
    "The pixels are basically uninitialized. You will need to use \\fill to initialize them to a certain value."
  ) +
  gsi::method ("==", &tl::PixelBuffer::operator==, gsi::arg ("other"),
    "@brief Returns a value indicating whether self is identical to the other image\n"
  ) +
  gsi::method ("!=", &tl::PixelBuffer::operator!=, gsi::arg ("other"),
    "@brief Returns a value indicating whether self is not identical to the other image\n"
  ) +
  gsi::method ("transparent=", &tl::PixelBuffer::set_transparent, gsi::arg ("t"),
    "@brief Sets a flag indicating whether the pixel buffer supports an alpha channel\n"
    "\n"
    "By default, the pixel buffer does not support an alpha channel.\n"
  ) +
  gsi::method ("transparent", &tl::PixelBuffer::transparent,
    "@brief Gets a flag indicating whether the pixel buffer supports an alpha channel\n"
  ) +
  gsi::method ("fill", &tl::PixelBuffer::fill, gsi::arg ("color"),
    "@brief Fills the pixel buffer with the given pixel value\n"
  ) +
  gsi::method ("swap", &tl::PixelBuffer::swap, gsi::arg ("other"),
    "@brief Swaps data with another PixelBuffer object\n"
  ) +
  gsi::method ("width", &tl::PixelBuffer::width,
    "@brief Gets the width of the pixel buffer in pixels\n"
  ) +
  gsi::method ("height", &tl::PixelBuffer::height,
    "@brief Gets the height of the pixel buffer in pixels\n"
  ) +
  gsi::method_ext ("set_pixel", &set_pixel_in_pixel_buffer, gsi::arg ("x"), gsi::arg ("y"), gsi::arg ("c"),
    "@brief Sets the value of the pixel at position x, y\n"
  ) +
  gsi::method_ext ("pixel", &get_pixel_from_pixel_buffer, gsi::arg ("x"), gsi::arg ("y"),
    "@brief Gets the value of the pixel at position x, y\n"
  ) +
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
  gsi::method ("patch", &tl::PixelBuffer::patch, gsi::arg ("other"),
    "@brief Patches another pixel buffer into this one\n"
    "\n"
    "This method is the inverse of \\diff - it will patch the difference image created by diff into this "
    "pixel buffer. Note that this method will not do true alpha blending and requires the other pixel buffer "
    "to have the same format than self. Self will be modified by this operation."
  ) +
  gsi::method ("diff", &tl::PixelBuffer::diff, gsi::arg ("other"),
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
//  tl::BitmapBuffer

static tl::BitmapBuffer *create_bitmap_buffer (unsigned int w, unsigned int h)
{
  return new tl::BitmapBuffer (w, h);
}

bool get_pixel_from_bitmap_buffer (const tl::BitmapBuffer *pb, unsigned int x, unsigned int y)
{
  if (x < pb->width () && y < pb->height ()) {
    return (pb->scan_line (y)[x / 8] & (0x01 << (x % 8))) != 0;
  } else {
    return false;
  }
}

void set_pixel_in_bitmap_buffer (tl::BitmapBuffer *pb, unsigned int x, unsigned int y, bool c)
{
  if (x < pb->width () && y < pb->height ()) {
    if (c) {
      pb->scan_line (y)[x / 8] |= 0x01 << (x % 8);
    } else {
      pb->scan_line (y)[x / 8] &= ~(0x01 << (x % 8));
    }
  }
}

static tl::BitmapBuffer read_bitmap_buffer (const std::string &file)
{
#if defined(HAVE_PNG)
  tl::InputStream stream (file);
  return tl::BitmapBuffer::read_png (stream);
#elif defined(HAVE_QT)
  //  QImage is fallback
  QImage img;
  img.load (tl::to_qstring (file), "PNG");
  return tl::BitmapBuffer::from_image (img);
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for BitmapBuffer")));
  return tl::BitmapBuffer ();
#endif
}

//  TODO: there should be some more efficient version of byte strings which avoid copies
static tl::BitmapBuffer bitmap_buffer_from_png (const std::vector<char> &data)
{
#if defined(HAVE_PNG)
  tl::InputMemoryStream data_stream (data.begin ().operator-> (), data.size ());
  tl::InputStream stream (data_stream);
  return tl::BitmapBuffer::read_png (stream);
#elif defined(HAVE_QT)
  //  QImage is fallback
  tl_assert (data.size () < std::numeric_limits<int>::max ());
  QImage img = QImage::fromData ((const uchar *) data.begin ().operator-> (), int (data.size ()));
  return tl::BitmapBuffer::from_image (img);
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for BitmapBuffer")));
  return tl::BitmapBuffer ();
#endif
}

static void write_bitmap_buffer (const tl::BitmapBuffer *pb, const std::string &file)
{
#if defined(HAVE_PNG)
  tl::OutputStream stream (file);
  pb->write_png (stream);
#elif defined(HAVE_QT)
  //  QImage is fallback
  QImage img = pb->to_image ();
  img.save (tl::to_qstring (file), "PNG");
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for BitmapBuffer")));
#endif
}

//  TODO: there should be some more efficient version of byte strings which avoid copies
static std::vector<char> bitmap_buffer_to_png (const tl::BitmapBuffer *pb)
{
#if defined(HAVE_PNG)
  tl::OutputMemoryStream data_stream;
  {
    tl::OutputStream stream (data_stream);
    pb->write_png (stream);
  }
  return std::vector<char> (data_stream.data (), data_stream.data () + data_stream.size ());
#elif defined(HAVE_QT)
  //  QImage is fallback
  QImage img = pb->to_image ();
  QBuffer data;
  img.save (&data, "PNG");
  return std::vector<char> (data.data ().constData (), data.data ().constEnd ());
#else
  throw tl::Exception (tl::to_string (tr ("No PNG support compiled in for BitmapBuffer")));
#endif
}


Class<tl::BitmapBuffer> decl_BitmapBuffer ("lay", "BitmapBuffer",
  gsi::constructor ("new", &create_bitmap_buffer, gsi::arg ("width"), gsi::arg ("height"),
    "@brief Creates a pixel buffer object\n"
    "\n"
    "@param width The width in pixels\n"
    "@param height The height in pixels\n"
    "\n"
    "The pixels are basically uninitialized. You will need to use \\fill to initialize them to a certain value."
  ) +
  gsi::method ("==", &tl::BitmapBuffer::operator==, gsi::arg ("other"),
    "@brief Returns a value indicating whether self is identical to the other image\n"
  ) +
  gsi::method ("!=", &tl::BitmapBuffer::operator!=, gsi::arg ("other"),
    "@brief Returns a value indicating whether self is not identical to the other image\n"
  ) +
  gsi::method ("fill", &tl::BitmapBuffer::fill, gsi::arg ("color"),
    "@brief Fills the pixel buffer with the given pixel value\n"
  ) +
  gsi::method ("swap", &tl::BitmapBuffer::swap, gsi::arg ("other"),
    "@brief Swaps data with another BitmapBuffer object\n"
  ) +
  gsi::method ("width", &tl::BitmapBuffer::width,
    "@brief Gets the width of the pixel buffer in pixels\n"
  ) +
  gsi::method ("height", &tl::BitmapBuffer::height,
    "@brief Gets the height of the pixel buffer in pixels\n"
  ) +
  gsi::method_ext ("set_pixel", &set_pixel_in_bitmap_buffer, gsi::arg ("x"), gsi::arg ("y"), gsi::arg ("c"),
    "@brief Sets the value of the pixel at position x, y\n"
  ) +
  gsi::method_ext ("pixel", &get_pixel_from_bitmap_buffer, gsi::arg ("x"), gsi::arg ("y"),
    "@brief Gets the value of the pixel at position x, y\n"
  ) +
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


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

#ifndef HDR_tlPixelBuffer
#define HDR_tlPixelBuffer

#include "tlCommon.h"
#include "tlColor.h"
#include "tlCopyOnWrite.h"
#include "tlStream.h"
#include "tlException.h"

#include <string.h>
#include <cstdint>

#if defined(HAVE_QT)
#  include <QImage>
#endif

namespace tl
{

/**
 *  @brief An exception thrown when a PNG read error occurs
 */
class TL_PUBLIC PixelBufferReadError
  : public tl::Exception
{
public:
  PixelBufferReadError (const char *msg);
  PixelBufferReadError (const std::string &msg);
};

/**
 *  @brief An exception thrown when a PNG write error occurs
 */
class TL_PUBLIC PixelBufferWriteError
  : public tl::Exception
{
public:
  PixelBufferWriteError (const char *msg);
  PixelBufferWriteError (const std::string &msg);
};

/**
 *  @brief An 32bit RGB/RGBA image class
 *
 *  This class substitutes QImage in Qt-less applications.
 *  It provides 32bit RGBA pixels with the format used by tl::Color.
 */
class TL_PUBLIC PixelBuffer
{
public:
  /**
   *  @brief Creates an image with the given height and width
   *
   *  If data is given, the image is initialized with the given data and will take ownership over the
   *  data block.
   *
   *  The size of the data block needs to be w*h elements.
   */
  PixelBuffer (unsigned int w, unsigned int h, tl::color_t *data);

  /**
   *  @brief Creates an image with the given height and width
   *
   *  If data is given, the image is initialized with the given data. A copy of the data is created.
   *
   *  "stride" specifies the stride (distance between two rows of data).
   *  The size of the data block needs to be stride*h elements or w*h if stride is not given.
   */
  PixelBuffer (unsigned int w, unsigned int h, const tl::color_t *data = 0, unsigned int stride = 0);

  /**
   *  @brief Default constructor
   */
  PixelBuffer ();

  /**
   *  @brief Copy constructor
   */
  PixelBuffer (const PixelBuffer &other);

  /**
   *  @brief Move constructor
   */
  PixelBuffer (PixelBuffer &&other);

  /**
   *  @brief Destructor
   */
  ~PixelBuffer ();

  /**
   *  @brief Equality
   */
  bool operator== (const PixelBuffer &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const PixelBuffer &other) const
  {
    return !operator== (other);
  }

  /**
   *  @brief Assignment
   */
  PixelBuffer &operator= (const PixelBuffer &other);

  /**
   *  @brief Move constructor
   */
  PixelBuffer &operator= (PixelBuffer &&other);

  /**
   *  @brief Sets a value indicating whether an alpha channel is present
   */
  void set_transparent (bool f);

  /**
   *  @brief Gets a value indicating whether an alpha channel is present
   */
  bool transparent () const
  {
    return m_transparent;
  }

  /**
   *  @brief Swaps this image with another one
   */
  void swap (PixelBuffer &other);

  /**
   *  @brief Gets the images width
   */
  unsigned int width () const
  {
    return m_width;
  }

  /**
   *  @brief Gets the images width
   */
  unsigned int height () const
  {
    return m_height;
  }

  /**
   *  @brief Gets the image stride (number of bytes per row)
   */
  unsigned int stride () const
  {
    return sizeof (tl::color_t) * m_width;
  }

  /**
   *  @brief Fills the image with the given color
   */
  void fill (tl::color_t);

  /**
   *  @brief Gets the scanline for row n
   */
  tl::color_t *scan_line (unsigned int n);

  /**
   *  @brief Gets the scanline for row n (const version)
   */
  const tl::color_t *scan_line (unsigned int n) const;

  /**
   *  @brief Gets the data pointer
   */
  tl::color_t *data ();

  /**
   *  @brief Gets the data pointer (const version)
   */
  const tl::color_t *data () const;

#if defined(HAVE_QT)
  /**
   *  @brief Produces a QImage object from the image
   *
   *  NOTE: this version creates a reference, i.e. the QImage is valid only
   *  during the lifetime of the PixelBuffer.
   */
  QImage to_image () const;

  /**
   *  @brief Produces a QImage object from the image
   *
   *  NOTE: this version creates a copy and the QImage is independent of the
   *  PixelBuffer.
   */
  QImage to_image_copy () const;

  /**
   *  @brief Creates a pixel buffer from a QImage object
   */
  static PixelBuffer from_image (const QImage &img);
#endif

#if defined(HAVE_PNG)
  /**
   *  @brief Creates a PixelBuffer object from a PNG file
   *  Throws a PixelBufferReadError if an error occurs.
   */
  static PixelBuffer read_png (tl::InputStream &input);

  /**
   *  @brief Writes the PixelBuffer object to a PNG file
   *  Throws a PixelBufferWriteError if an error occurs.
   */
  void write_png (tl::OutputStream &output) const;
#endif

  /**
   *  @brief Overlays the other image with this one
   *
   *  This feature does not implement real alpha blending. Instead all
   *  pixels with an alpha value >= 128 from the other image are patched into this image.
   */
  void patch (const PixelBuffer &other);

  /**
   *  @brief Generates the image difference
   *
   *  This feature produces a binary-alpha image of *this and other. The
   *  result can be patched into this image to render the same image than
   *  "other". The difference image will contains the pixels from other which
   *  are different from *this.
   *
   *  alpha values from this and other are ignored.
   */
  PixelBuffer diff (const PixelBuffer &other) const;

  /**
   *  @brief Subsamples the image and puts the subsampled image into the destination image
   *
   *  @param dest Where the subsampled image goes to
   *  @param os The subsampling factor
   *  @param g The gamma value for color interpolation
   *
   *  The dimension of the destination image must be set to the corresponding fraction of
   *  self's dimension.
   */
  void subsample (tl::PixelBuffer &dest, unsigned int os, double g);

  /**
   *  @brief Scales the image into the given destination image
   *
   *  @param dest Where the scaled image goes to
   *  @param os The scaling factor
   *
   *  The destination images dimension must have been set of self's dimension times os.
   */
  void blowup (tl::PixelBuffer &dest, unsigned int os);

  /**
   *  @brief Gets the texts
   *
   *  Texts are annotations which can be stored to PNG and back.
   */
  const std::vector<std::pair<std::string, std::string> > &texts () const
  {
    return m_texts;
  }

  /**
   *  @brief Sets the texts
   *
   *  Texts are annotations which can be stored to PNG and back.
   */
  void set_texts (const std::vector<std::pair<std::string, std::string> > &texts)
  {
    m_texts = texts;
  }

private:
  class ImageData
  {
  public:
    ImageData ()
      : mp_data (0), m_length (0)
    {
      //  .. nothing yet ..
    }

    ImageData (tl::color_t *data, size_t length)
      : mp_data (data), m_length (length)
    {
      //  .. nothing yet ..
    }

    ImageData (const ImageData &other)
    {
      m_length = other.length ();
      mp_data = new tl::color_t [other.length ()];
      memcpy (mp_data, other.data (), m_length * sizeof (tl::color_t));
    }

    ~ImageData ()
    {
      delete[] mp_data;
      mp_data = 0;
    }

    size_t length () const { return m_length; }
    tl::color_t *data () { return mp_data; }
    const tl::color_t *data () const { return mp_data; }

  private:
    tl::color_t *mp_data;
    size_t m_length;

    ImageData &operator= (const ImageData &other);
  };

  unsigned int m_width, m_height;
  bool m_transparent;
  tl::copy_on_write_ptr<ImageData> m_data;
  std::vector<std::pair<std::string, std::string> > m_texts;
};

/**
 *  @brief An monochrome image class
 *
 *  This class substitutes QImage for monochrome images in Qt-less applications.
 */

class TL_PUBLIC BitmapBuffer
{
public:
  /**
   *  @brief Creates an image with the given height and width
   *
   *  If data is given, the image is initialized with the given data and will take ownership over the
   *  data block.
   *
   *  Lines are byte-aligned.
   */
  BitmapBuffer (unsigned int w, unsigned int h, uint8_t *data);

  /**
   *  @brief Creates an image with the given height and width
   *
   *  If data is given, the image is initialized with the given data. A copy of the data is created.
   *
   *  "stride" specifies the stride (distance in bytes between two rows of data).
   *  The size of the data block needs to be stride*h elements or bytes(w)*h if stride is not given.
   */
  BitmapBuffer (unsigned int w, unsigned int h, const uint8_t *data = 0, unsigned int stride = 0);

  /**
   *  @brief Default constructor
   */
  BitmapBuffer ();

  /**
   *  @brief Copy constructor
   */
  BitmapBuffer (const BitmapBuffer &other);

  /**
   *  @brief Move constructor
   */
  BitmapBuffer (BitmapBuffer &&other);

  /**
   *  @brief Equality
   */
  bool operator== (const BitmapBuffer &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const BitmapBuffer &other) const
  {
    return !operator== (other);
  }

  /**
   *  @brief Destructor
   */
  ~BitmapBuffer ();

  /**
   *  @brief Assignment
   */
  BitmapBuffer &operator= (const BitmapBuffer &other);

  /**
   *  @brief Move constructor
   */
  BitmapBuffer &operator= (BitmapBuffer &&other);

  /**
   *  @brief Swaps this image with another one
   */
  void swap (BitmapBuffer &other);

  /**
   *  @brief Gets the images width
   */
  unsigned int width () const
  {
    return m_width;
  }

  /**
   *  @brief Gets the images width
   */
  unsigned int height () const
  {
    return m_height;
  }

  /**
   *  @brief Gets the image stride (number of bytes per row)
   */
  unsigned int stride () const
  {
    return m_stride;
  }

  /**
   *  @brief Fills the image with the given color
   */
  void fill (bool value);

  /**
   *  @brief Gets the scanline for row n
   */
  uint8_t *scan_line (unsigned int n);

  /**
   *  @brief Gets the scanline for row n (const version)
   */
  const uint8_t *scan_line (unsigned int n) const;

  /**
   *  @brief Gets the data pointer
   */
  uint8_t *data ();

  /**
   *  @brief Gets the data pointer (const version)
   */
  const uint8_t *data () const;

#if defined(HAVE_QT)
  /**
   *  @brief Produces a QImage object from the image
   *
   *  NOTE: this version creates a reference, i.e. the QImage is valid only
   *  during the lifetime of the BitmapBuffer.
   */
  QImage to_image () const;

  /**
   *  @brief Produces a QImage object from the image
   *
   *  NOTE: this version creates a copy and the QImage is independent of the
   *  BitmapBuffer.
   */
  QImage to_image_copy () const;

  /**
   *  @brief Creates a pixel buffer from a QImage object
   */
  static BitmapBuffer from_image (const QImage &img);
#endif

#if defined(HAVE_PNG)
  /**
   *  @brief Creates a PixelBuffer object from a PNG file
   *  Throws a PixelBufferReadError if an error occurs.
   */
  static BitmapBuffer read_png (tl::InputStream &input);

  /**
   *  @brief Writes the PixelBuffer object to a PNG file
   *  Throws a PixelBufferWriteError if an error occurs.
   */
  void write_png (tl::OutputStream &output) const;
#endif

  /**
   *  @brief Gets the texts
   *
   *  Texts are annotations which can be stored to PNG and back.
   */
  const std::vector<std::pair<std::string, std::string> > &texts () const
  {
    return m_texts;
  }

  /**
   *  @brief Sets the texts
   *
   *  Texts are annotations which can be stored to PNG and back.
   */
  void set_texts (const std::vector<std::pair<std::string, std::string> > &texts)
  {
    m_texts = texts;
  }

private:
  class MonoImageData
  {
  public:
    MonoImageData ()
      : mp_data (0), m_length (0)
    {
      //  .. nothing yet ..
    }

    MonoImageData (uint8_t *data, size_t length)
      : mp_data (data), m_length (length)
    {
      //  .. nothing yet ..
    }

    MonoImageData (const MonoImageData &other)
    {
      m_length = other.length ();
      mp_data = new uint8_t [other.length ()];
      memcpy (mp_data, other.data (), m_length * sizeof (uint8_t));
    }

    ~MonoImageData ()
    {
      delete[] mp_data;
      mp_data = 0;
    }

    size_t length () const { return m_length; }
    uint8_t *data () { return mp_data; }
    const uint8_t *data () const { return mp_data; }

  private:
    uint8_t *mp_data;
    size_t m_length;

    MonoImageData &operator= (const MonoImageData &other);
  };

  unsigned int m_width, m_height;
  unsigned int m_stride;
  tl::copy_on_write_ptr<MonoImageData> m_data;
  std::vector<std::pair<std::string, std::string> > m_texts;
};

}

#endif

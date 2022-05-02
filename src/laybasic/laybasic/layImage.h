
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


#ifndef HDR_layImage
#define HDR_layImage

#include "laybasicCommon.h"
#include "layColor.h"
#include "tlCopyOnWrite.h"

#include <string.h>

#if defined(HAVE_QT)
#  include <QImage>
#endif

namespace lay
{

/**
 *  @brief An 32bit RGBA image class
 *
 *  This class substitutes QImage in Qt-less applications.
 *  It provides 32bit RGBA pixels with the format used by lay::Color.
 */

class LAYBASIC_PUBLIC Image
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
  Image (unsigned int w, unsigned int h, lay::color_t *data);

  /**
   *  @brief Creates an image with the given height and width
   *
   *  If data is given, the image is initialized with the given data. A copy of the data is created.
   *
   *  "stride" specifies the stride (distance between two rows of data).
   *  The size of the data block needs to be stride*h elements or w*h if stride is not given.
   */
  Image (unsigned int w, unsigned int h, const lay::color_t *data = 0, unsigned int stride = 0);

  /**
   *  @brief Default constructor
   */
  Image ();

  /**
   *  @brief Copy constructor
   */
  Image (const Image &other);

  /**
   *  @brief Move constructor
   */
  Image (Image &&other);

  /**
   *  @brief Destructor
   */
  ~Image ();

  /**
   *  @brief Assignment
   */
  Image &operator= (const Image &other);

  /**
   *  @brief Move constructor
   */
  Image &operator= (Image &&other);

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
  void swap (Image &other);

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
   *  @brief Fills the image with the given color
   */
  void fill (lay::color_t);

  /**
   *  @brief Gets the scanline for row n
   */
  color_t *scan_line (unsigned int n);

  /**
   *  @brief Gets the scanline for row n (const version)
   */
  const color_t *scan_line (unsigned int n) const;

  /**
   *  @brief Gets the data pointer
   */
  color_t *data ();

  /**
   *  @brief Gets the data pointer (const version)
   */
  const color_t *data () const;

#if defined(HAVE_QT)
  /**
   *  @brief Produces a QImage object from the image
   */
  QImage to_image () const;
#endif

  /**
   *  @brief Overlays the other image with this one
   *
   *  This feature does not implement real alpha blending. Instead all
   *  pixels with an alpha value >= 128 from the other image are patched into this image.
   */
  void patch (const Image &other);

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
  Image diff (const Image &other) const;

private:
  class ImageData
  {
  public:
    ImageData ()
      : mp_data (0), m_length (0)
    {
      //  .. nothing yet ..
    }

    ImageData (lay::color_t *data, size_t length)
      : mp_data (data), m_length (length)
    {
      //  .. nothing yet ..
    }

    ImageData (const ImageData &other)
    {
      m_length = other.length ();
      mp_data = new lay::color_t [other.length ()];
      memcpy (mp_data, other.data (), m_length * sizeof (lay::color_t));
    }

    ~ImageData ()
    {
      delete[] mp_data;
      mp_data = 0;
    }

    size_t length () const { return m_length; }
    lay::color_t *data () { return mp_data; }
    const lay::color_t *data () const { return mp_data; }

  private:
    lay::color_t *mp_data;
    size_t m_length;

    ImageData &operator= (const ImageData &other);
  };

  unsigned int m_width, m_height;
  bool m_transparent;
  tl::copy_on_write_ptr<ImageData> m_data;
};

}

#endif


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

#ifndef HDR_tlColor
#define HDR_tlColor

#include "tlCommon.h"

#include <stdint.h>
#include <string>

#if defined(HAVE_QT)
#  include <QColor>
#endif

namespace tl
{

/**
 *  @brief The basic color type for a RGB triplet
 */
typedef uint32_t color_t;

/**
 *  @brief Gets the color components from a color_t
 */
inline unsigned int alpha (color_t c) { return (c >> 24) & 0xff; }
inline unsigned int red (color_t c)   { return (c >> 16) & 0xff; }
inline unsigned int green (color_t c) { return (c >> 8) & 0xff; }
inline unsigned int blue (color_t c)  { return c & 0xff; }

/**
 *  @brief A wrapper for a color value
 *
 *  This class is a replacement for QColor. It offers invalid color values and
 *  string conversion.
 */
class TL_PUBLIC Color
{
public:
  /**
   *  @brief Default constructor - creates an invalid color
   */
  Color ();

  /**
   *  @brief Creates a color from a RGB triplet
   *
   *  Note: this will set the alpha value to 255.
   */
  Color (color_t color);

#if defined(HAVE_QT)
  /**
   *  @brief Creates a color from a QColor
   */
  Color (const QColor &qc);
#endif

  /**
   *  @brief Creates a color from a RGB triplet and alpha value
   *
   *  An alpha value of 0 generates an invalid color.
   */
  Color (unsigned int r, unsigned int g, unsigned int b, unsigned int alpha = 0xff);

  /**
   *  @brief Creates a color value from a string
   */
  Color (const std::string &name);

  /**
   *  @brief Creates a color value from a string
   */
  Color (const char *name);

  /**
   *  @brief Comparison: equal
   */
  bool operator== (const Color &color) const
  {
    return m_color == color.m_color;
  }

  /**
   *  @brief Comparison: not equal
   */
  bool operator!= (const Color &color) const
  {
    return m_color != color.m_color;
  }

  /**
   *  @brief Comparison: less
   */
  bool operator< (const Color &color) const
  {
    return m_color < color.m_color;
  }

  /**
   *  @brief Gets the string value from a color
   */
  std::string to_string () const;

#if defined(HAVE_QT)
  /**
   *  @brief Gets the QColor from the color
   */
  QColor to_qc () const;
#endif

  /**
   *  @brief Gets a value indicating whether the color is valid
   */
  bool is_valid () const;

  /**
   *  @brief Gets the RGB triplet
   */
  color_t rgb () const
  {
    return m_color;
  }

  /**
   *  @brief Gets the alpha component
   */
  unsigned int alpha () const
  {
    return (m_color & 0xff000000) >> 24;
  }

  /**
   *  @brief Gets the red component
   */
  unsigned int red () const
  {
    return (m_color & 0xff0000) >> 16;
  }

  /**
   *  @brief Gets the green component
   */
  unsigned int green () const
  {
    return (m_color & 0xff00) >> 8;
  }

  /**
   *  @brief Gets the blue component
   */
  unsigned int blue () const
  {
    return (m_color & 0xff);
  }

  /**
   *  @brief Converts the color into monochrome "on" value
   */
  bool to_mono () const
  {
    return (m_color & 0x8000) != 0;
  }

  /**
   *  @brief Gets the HSV color components
   *  hue: 0..359
   *  saturation: 0..255
   *  value: 0..255
   */
  void get_hsv (unsigned int &hue, unsigned int &saturation, unsigned int &value) const;

  /**
   *  @brief Creates the color from a HSV color
   */
  static tl::Color from_hsv (unsigned int hue, unsigned int saturation, unsigned int value);

private:
  color_t m_color;

  void init_from_string (const char *s);
};

#if defined(HAVE_QT)

/**
 *  @brief Provides conversion of a color_t to QColor
 */
inline QColor c2qc (color_t c)
{
  return Color (c).to_qc ();
}

/**
 *  @brief Provides conversion of a QColor to color_t
 */
inline color_t qc2c (const QColor &qc)
{
  return Color (qc).rgb ();
}

#endif

}

#endif

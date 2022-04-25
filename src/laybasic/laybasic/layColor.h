
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


#ifndef HDR_layColor
#define HDR_layColor

#include "laybasicCommon.h"

#include <stdint.h>
#include <string>

namespace lay
{

/**
 *  @brief The basic color type for a RGB triplet
 */
typedef uint32_t color_t;

/**
 *  @brief A wrapper for a color value
 *
 *  This class is a replacement for QColor. It offers invalid color values and
 *  string conversion.
 */
class LAYBASIC_PUBLIC Color
{
public:
  /**
   *  @brief Default constructor - creates an invalid color
   */
  Color ();

  /**
   *  @brief Creates a color from a RGB triplet
   */
  Color (color_t color);

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

private:
  color_t m_color;
};

}

#endif

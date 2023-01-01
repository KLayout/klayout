
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


#ifndef HDR_layColorPalette
#define HDR_layColorPalette

#include "laybasicCommon.h"

#include "layViewOp.h"

#include <vector>
#include <string>

namespace lay
{

class LAYBASIC_PUBLIC ColorPalette
{
public:

  /**
   *  @brief Default constructor
   *
   *  This initializes the palette with the standard colors.
   */
  ColorPalette ();

  /**
   *  @brief Constructor from the data 
   *
   *  @param color The colors as a vector
   *  @param luminous_colors The list of indices of luminous colors as a vector
   */
  ColorPalette (const std::vector<tl::color_t> &colors, const std::vector<unsigned int> &luminous_colors);

  /**
   *  @brief Copy constructor
   */
  ColorPalette (const ColorPalette &d);

  /**
   *  @brief Assignment operator
   */
  ColorPalette operator= (const ColorPalette &d);

  /**
   *  @brief Equality operator
   */
  bool operator== (const ColorPalette &d) const;

  /**
   *  @brief Inequality operator
   */
  bool operator!= (const ColorPalette &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief Change a specific color
   */
  void set_color (unsigned int n, tl::color_t c);

  /**
   *  @brief Clear the colors list
   */
  void clear_colors ();

  /**
   *  @brief Set a specific luminous color index
   */
  void set_luminous_color_index (unsigned int n, unsigned int ci);

  /**
   *  @brief Clear the luminous color list
   */
  void clear_luminous_colors ();

  /** 
   *  @brief Retrieve the color by index
   */
  tl::color_t color_by_index (unsigned int n) const;

  /**
   *  @brief Retrieve the number of colors in the palette
   *
   *  Warning: it is not guaranteed that this number is non-zero.
   */
  unsigned int colors () const;

  /** 
   *  @brief Retrieve the luminous color by index
   */
  tl::color_t 
  luminous_color_by_index (unsigned int n) const
  {
    return color_by_index (luminous_color_index_by_index (n));
  }

  /** 
   *  @brief Retrieve the luminous color index by index (0 to luminous_colors()-1)
   *
   *  The index returned is the index of the color referenced. The actual color
   *  can be obtained with color_by_index().
   */
  unsigned int luminous_color_index_by_index (unsigned int n) const;

  /**
   *  @brief Retrieve the number of luminous of colors in the palette
   *
   *  The luminous colors are used for automatically coloring the
   *  layers for example. They are accessible by color index 0..lc-1,
   *  where lc is the number returned by this functions.
   *  Warning: it is not guaranteed that this number is non-zero.
   */
  unsigned int luminous_colors () const;

  /** 
   *  @brief Conversion to a string 
   */
  std::string to_string () const;

  /**
   *  @brief Conversion from a string
   *
   *  This method will throw an exception if the string does not have a valid format
   *  like the one returned by the to_string method.
   *
   *  If simple is true, this method allows setting a palette without luminous colors
   *  and without colors at all.
   */
  void from_string (const std::string &s, bool simple = false);

  /**
   *  @brief Deliver the default palette
   */
  static ColorPalette default_palette (); 

private:
  std::vector <tl::color_t> m_colors;
  std::vector <unsigned int> m_luminous_color_indices;

};

}

#endif


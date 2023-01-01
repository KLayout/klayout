
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


#ifndef HDR_layLineStylePalette
#define HDR_layLineStylePalette

#include "laybasicCommon.h"

#include <vector>
#include <string>

namespace lay
{

class LAYBASIC_PUBLIC LineStylePalette
{
public:

  /**
   *  @brief Default constructor
   *
   *  This initializes the palette with the standard styles.
   */
  LineStylePalette ();

  /**
   *  @brief Constructor from the data 
   *
   *  @param styles The styles as a vector
   */
  LineStylePalette (const std::vector<unsigned int> &styles);

  /**
   *  @brief Copy constructor
   */
  LineStylePalette (const LineStylePalette &d);

  /**
   *  @brief Assignment operator
   */
  LineStylePalette operator= (const LineStylePalette &d);

  /**
   *  @brief Equality operator
   */
  bool operator== (const LineStylePalette &d) const;

  /**
   *  @brief Inequality operator
   */
  bool operator!= (const LineStylePalette &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief Change a specific style
   */
  void set_style (unsigned int n, unsigned int s);

  /**
   *  @brief Clear the style list
   */
  void clear_styles ();

  /** 
   *  @brief Retrieve the style by index
   */
  unsigned int style_by_index (unsigned int n) const;

  /**
   *  @brief Retrieve the number of styles in the palette
   *
   *  Warning: it is not guaranteed that this number is non-zero.
   */
  unsigned int styles () const;

  /** 
   *  @brief Conversion to a string 
   */
  std::string to_string () const;

  /**
   *  @brief Conversion from a string
   *
   *  This method will throw an exception if the string does not have a valid format
   *  like the one returned by the to_string method.
   */
  void from_string (const std::string &s);

  /**
   *  @brief Deliver the default palette
   */
  static LineStylePalette default_palette (); 

private:
  std::vector <unsigned int> m_styles;

};

}

#endif


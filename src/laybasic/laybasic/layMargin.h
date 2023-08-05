
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


#ifndef HDR_layMargin
#define HDR_layMargin

#include "laybasicCommon.h"

#include "dbBox.h"

namespace lay
{

/**
 *  @brief A class represeting a margin on size
 *
 *  Margin or size can be specified absolutely (in micron units) or relative
 *  to some object (given by a size or a box).
 *
 *  The object keeps relative and absolute values so it can be easily
 *  switched.
 */

class LAYBASIC_PUBLIC Margin
{
public:
  /**
   *  @brief The constructor
   */
  Margin (double value = 0.0, bool relative = false);

  /**
   *  @brief Equality
   */
  bool operator== (const lay::Margin &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const lay::Margin &other) const
  {
    return ! operator== (other);
  }

  /**
   *  @brief Gets the relative value
   */
  double relative_value () const
  {
    return m_relative_value;
  }

  /**
   *  @brief Sets the relative value
   */
  void set_relative_value (double v)
  {
    m_relative_value = v;
  }

  /**
   *  @brief Gets the absolute value
   */
  double absolute_value () const
  {
    return m_absolute_value;
  }

  /**
   *  @brief Sets the absolute value
   */
  void set_absolute_value (double v)
  {
    m_absolute_value = v;
  }

  /**
   *  @brief Gets a value indicating whether the relative value shall be used
   */
  bool relative_mode () const
  {
    return m_relative_mode;
  }

  /**
   *  @brief Sets a value indicating whether the relative value shall be used
   */
  void set_relative_mode (bool mode)
  {
    m_relative_mode = mode;
  }

  /**
   *  @brief Converts the object to a string
   */
  std::string to_string () const;

  /**
   *  @brief Creates the object from a string
   */
  static Margin from_string (const std::string &s);

  /**
   *  @brief Gets the resulting value for a given object dimension
   */
  double get (double dim) const;

  /**
   *  @brief Gets the resulting value for a given box
   */
  double get (const db::DBox &box) const;

private:
  double m_relative_value, m_absolute_value;
  bool m_relative_mode;
};

}

#endif

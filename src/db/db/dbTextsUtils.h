
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#ifndef HDR_dbTextsUtils
#define HDR_dbTextsUtils

#include "dbCommon.h"
#include "dbTexts.h"
#include "tlGlobPattern.h"

namespace db {

/**
 *  @brief An text filter filtering by a string
 *
 *  It will filter all texts for which the string is equal to "text".
 *  There is an "invert" flag which allows selecting all texts not
 *  matching the criterion.
 */

struct DB_PUBLIC TextStringFilter
  : public TextFilterBase
{
  /**
   *  @brief Constructor
   *
   *  @param text The text string to match
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  TextStringFilter (const std::string &text, bool inverse)
    : m_text (text), m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true if the text matches the criterion
   */
  virtual bool selected (const db::Text &text) const
  {
    return (text.string () == m_text) != m_inverse;
  }

  /**
   *  @brief This filter is not sensitive to hierarchy
   */
  virtual const TransformationReducer *vars () const
  {
    return 0;
  }

  /**
   *  @brief Requires merged input
   */
  virtual bool requires_raw_input () const
  {
    return false;
  }

  /**
   *  @brief Wants to build variants
   */
  virtual bool wants_variants () const
  {
    return false;
  }

private:
  std::string m_text;
  bool m_inverse;
};

/**
 *  @brief An text filter filtering by a glob-style pattern
 *
 *  It will filter all texts for which the string matches the given glob-style pattern.
 *  There is an "invert" flag which allows selecting all texts not
 *  matching the criterion.
 */

struct DB_PUBLIC TextPatternFilter
  : public TextFilterBase
{
  /**
   *  @brief Constructor
   *
   *  @param text The text pattern to match
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  TextPatternFilter (const std::string &text, bool inverse)
    : m_pattern (text), m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true if the text matches the criterion
   */
  virtual bool selected (const db::Text &text) const
  {
    return m_pattern.match (text.string ()) != m_inverse;
  }

  /**
   *  @brief This filter is not sensitive to hierarchy
   */
  virtual const TransformationReducer *vars () const
  {
    return 0;
  }

  /**
   *  @brief Requires merged input
   */
  virtual bool requires_raw_input () const
  {
    return false;
  }

  /**
   *  @brief Wants to build variants
   */
  virtual bool wants_variants () const
  {
    return false;
  }

private:
  tl::GlobPattern m_pattern;
  bool m_inverse;
};

} // namespace db

#endif

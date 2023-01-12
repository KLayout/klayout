
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


#ifndef HDR_dbGlyphs
#define HDR_dbGlyphs

#include "dbPolygon.h"
#include "dbBox.h"
#include "dbRegion.h"

#include <map>
#include <vector>
#include <string>

namespace db
{

class Layout;

/**
 *  @brief A basic text field generator class
 *
 *  Each text generator corresponds to a specific font. Each font is defined by a file
 *  located in the <application_path>/font directories. Font files are standard GDS or
 *  OASIS files.
 *
 *  By convention the files must have two to three layers:
 *
 *    1/0 for the actual data
 *    2/0 for the borders
 *    3/0 for an optional additional background
 *
 *  Currently, all glyphs must be bottom-left aligned at 0, 0. The
 *  border must be drawn in at least one glyph cell. The border is taken
 *  as the overall bbox of all borders.
 *
 *  The glyph cells must be named with a single character or "nnn" where "d" is the
 *  ASCII code of the character (i.e. "032" for space). Allowed ASCII codes are 32 through 127.
 *  If a lower-case "a" character is defined, lower-case letters are supported.
 *  Otherwise, lowercase letters are mapped to uppercase letters.
 *
 *  Undefined characters are left blank.
 *
 *  A comment cell can be defined ("COMMENT") which must hold one text in layer 1
 *  stating the comment, and additional descriptions such as line width:
 *
 *    line_width = x       Specifies the line width in µm
 *    design_grid = x      Specifies the design grid in µm
 *    <any other text>     The description string
 */
class DB_PUBLIC TextGenerator
{
public:
  /**
   *  @brief Default constructor for a font object
   */
  TextGenerator ();

  /**
   *  @brief Loads the font from the given resource
   */
  void load_from_resource (const std::string &name);

  /**
   *  @brief Loads from the given binary data
   */
  void load_from_data (const char *data, size_t ndata, const std::string &name, const std::string &description);

  /**
   *  @brief Loads the font from the given file
   */
  void load_from_file (const std::string &filename);

  /**
   *  @brief Gets the generator's description
   */
  const std::string &description () const
  {
    return m_description;
  }

  /**
   *  @brief Gets the generator's name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Creates the given text as a set of polygons
   *
   *  @param t The text to produce
   *  @param target_dbu The target DBU to which the polygons will conform
   *  @param mag The magnification (1 = original size)
   *  @param inv If true, the text is inverted with the background of the glyphs
   *  @param bias A bias applied before inversion in µm units (can be negative)
   *  @param char_spacing Additional spacing between the characters in µm
   *  @param char_spacing Additional spacing between the lines in µm
   *  @param The resulting polygons will be put here (the vector will be cleared before)
   */
  void text (const std::string &t, double target_dbu, double mag2, bool inv, double bias, double char_spacing, double line_spacing, std::vector<db::Polygon> &polygons) const;

  /**
   *  @brief Creates the given text as a region
   *  For the parameters see "text"
   */
  db::Region text_as_region (const std::string &t, double target_dbu, double mag2, bool inv, double bias, double char_spacing, double line_spacing) const;

  /**
   *  @brief Gets the glyph for a given character
   *  If the character is not supported, an empty vector is returned.
   */
  const std::vector<db::Polygon> &glyph (char c) const;

  /**
   *  @brief Gets the glyph for a given character as a region
   *  If the character is not supported, an empty region is returned.
   */
  db::Region glyph_as_region (char c) const;

  /**
   *  @brief Gets the glyph line width in database units
   */
  db::Coord line_width () const
  {
    return m_line_width;
  }

  /**
   *  @brief Gets the glyph design grid in database units
   */
  db::Coord design_grid () const
  {
    return m_design_grid;
  }

  /**
   *  @brief Gets the glyph width in database units
   */
  db::Coord width () const
  {
    return m_width;
  }

  /**
   *  @brief Gets the glyph height in database units
   */
  db::Coord height () const
  {
    return m_height;
  }

  /**
   *  @brief Gets the background rectangle of the glyphs
   */
  const db::Box &background () const
  {
    return m_background;
  }

  /**
   *  @brief Returns the DBU the generator is designed in
   */
  double dbu () const
  {
    return m_dbu;
  }

  /**
   *  @brief Gets a list of the generators available from the default locations
   *  New text generators can be created any time from files or resources.
   */
  static const std::vector<TextGenerator> &generators ();

  /**
   *  @brief Sets the search path for font files
   *  The given folders are scanned for font files.
   */
  static void set_font_paths (const std::vector<std::string> &paths);

  /**
   *  @brief Gets the font search paths
   */
  static std::vector<std::string> font_paths ();

  /**
   *  @brief Returns the font with the given name
   *  If no font with that name exists, 0 is returned.
   */
  static const TextGenerator *generator_by_name (const std::string &name);

  /**
   *  @brief Returns the default font
   */
  static const TextGenerator *default_generator ();

private:
  std::map<char, std::vector<db::Polygon> > m_data;
  db::Coord m_width, m_height, m_line_width, m_design_grid;
  db::Box m_background;
  std::string m_description;
  std::string m_name;
  double m_dbu;
  bool m_lowercase_supported;

  void read_from_layout (const db::Layout &layout, unsigned int ldata, unsigned int lborder, unsigned int lbackground);
};


}

#endif



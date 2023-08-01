
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


#ifndef HDR_tlInclude
#define HDR_tlInclude

#include "tlCommon.h"

#include <string>
#include <map>

namespace tl
{

class InputStream;

/**
 *  @brief An interface providing the include file resolver
 *
 *  The task of this object is to obtain the text for an include file path.
 *  The path already underwent variable interpolation and relative path resolution.
 */
class TL_PUBLIC IncludeFileResolver
{
public:
  IncludeFileResolver () { }
  virtual ~IncludeFileResolver () { }

  virtual std::string get_text (const std::string &path) const = 0;
};

/**
 *  @brief Provide the basic include expansion and file/line mapping mechanism
 *
 *  The Expander object performs the file expansion and also stores the information
 *  required for translating back file names and line numbers.
 *
 *  Include expansion happens through a pseudo-comment "# %include ..." which
 *  takes a file path as the argument. File paths are always resolved relative to
 *  the original file.
 *  The file path is expression-interpolated, hence can access environment variables
 *  through $(env("HOME")) for example.
 */
class TL_PUBLIC IncludeExpander
{
public:
  /**
   *  @brief The default constructor
   */
  IncludeExpander ();

  /**
   *  @brief Provides include expansion
   *
   *  This method will deliver the expanded text and the include expander object.
   */
  static IncludeExpander expand (const std::string &path, std::string &expanded_text, const IncludeFileResolver *resolver = 0);

  /**
   *  @brief Provides include expansion
   *
   *  This method will deliver the expanded text and the include expander object.
   *  This version also takes the actual text of the original file.
   */
  static IncludeExpander expand (const std::string &path, const std::string &original_text, std::string &expanded_text, const IncludeFileResolver *resolver = 0);

  /**
   *  @brief Serializes the include expander information into a string
   *
   *  If no include expansion happened, the serialized string will be the original file path.
   *  Otherwise it's a "@"-prefixed string.
   */
  std::string to_string () const;

  /**
   *  @brief Deserializes the include expander information from a string
   */
  static IncludeExpander from_string (const std::string &s);

  /**
   *  @brief Provides translation of the expanded text's line number to filename/line number for the original file
   */
  std::pair<std::string, int> translate_to_original (int line_number);

  /**
   *  @brief Provides translation of original file name/line number to included file name/line number
   */
  static std::pair<std::string, int> translate_to_original (const std::string &file, int line_number)
  {
    return IncludeExpander::from_string (file).translate_to_original (line_number);
  }

private:
  std::map<int, std::pair<std::string, int> > m_sections;

  void read (const std::string &path, tl::InputStream &is, std::string &expanded_text, int &line_counter, const IncludeFileResolver *mp_resolver);
};

}

#endif


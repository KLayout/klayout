
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

#ifndef HDR_dbOASISFormat
#define HDR_dbOASISFormat

#include "dbSaveLayoutOptions.h"
#include "dbLoadLayoutOptions.h"
#include "dbPluginCommon.h"

namespace db
{

/**
 *  @brief Structure that holds the OASIS specific options for the reader
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC OASISReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  OASISReaderOptions ()
    : read_all_properties (false), expect_strict_mode (-1)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief A flag indicating whether to read all properties
   *
   *  If this flag is set, all properties, including the special properties are read.
   *  This mode is only provided for testing and writing such a layout will probably
   *  result in duplicate entries.
   */
  bool read_all_properties; 

  /**
   *  @brief Indicates that the reader expects strict mode or note
   *
   *  This is mainly a debugging an testing option but it may be used to verify
   *  the compliance of a file with string or non-strict mode.
   *
   *  The values are:
   *    -1: don't care (default)
   *     0:  expect non-strict
   *     1:  expect strict
   */
  int expect_strict_mode;

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new OASISReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("OASIS");
    return n;
  }
};

/**
 *  @brief Structure that holds the OASIS specific options for the Writer
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC OASISWriterOptions
  : public FormatSpecificWriterOptions
{
public:
  /**
   *  @brief The constructor
   */
  OASISWriterOptions ()
    : compression_level (2), write_cblocks (true), strict_mode (true), recompress (false), permissive (false),
      write_std_properties (1), subst_char ("*"), tables_at_end (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief OASIS writer compression level
   *
   *  This level describes how hard the OASIS writer will try to compress the shapes
   *  using shape arrays. Building shape arrays may take some time and requires some memory.
   *    0 - no shape array building
   *    1 - nearest neighbor shape array formation 
   *    2++ - enhanced shape array search algorithm using 2nd and further neighbor distances as well
   */
  int compression_level; 

  /**
   *  @brief CBLOCK compression
   *
   *  If this flag is set, every cell is CBLOCK-compressed.
   */
  bool write_cblocks;

  /**
   *  @brief Strict mode
   *
   *  If this flag is set, a strict-mode file will be produced
   */
  bool strict_mode;

  /**
   *  @brief Recompressions
   *
   *  If the recompression flag is true, existing shape arrays will be resolved and 
   *  put into the compressor again (may take longer).
   */
  bool recompress;

  /**
   *  @brief Permissive mode
   *
   *  In permissive mode, a warning is issued for certain cases rather than
   *  an error:
   *  - Polygons with less than three points (omitted)
   *  - Paths/circles with odd diameter (rounded)
   */
  bool permissive;

  /**
   *  @brief Write global standard properties
   *
   *  If this value is 0, no standard properties are written. If it's 1, global
   *  standard properties such as S_TOP_CELL are written. If 2, bounding box 
   *  standard properties are written for every cell too.
   */
  int write_std_properties;

  /**
   *  @brief Substitution character
   *
   *  If non-empty, this string (first character) will be used for 
   *  substituting invalid characters in a-strings and n-strings.
   */
  std::string subst_char;

  /**
   *  @brief Hidden option, for testing mainly: write tables at end to force forward references
   */
  bool tables_at_end;

  /** 
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual FormatSpecificWriterOptions *clone () const
  {
    return new OASISWriterOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual const std::string &format_name () const
  {
    static std::string n ("OASIS");
    return n;
  }
};

} // namespace db

#endif

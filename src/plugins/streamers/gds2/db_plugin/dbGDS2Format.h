
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

#ifndef HDR_dbGDS2Format
#define HDR_dbGDS2Format

#include "dbSaveLayoutOptions.h"
#include "dbLoadLayoutOptions.h"
#include "dbPluginCommon.h"

namespace db
{

/**
 *  @brief Structure that holds the GDS2 specific options for the reader
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC GDS2ReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  GDS2ReaderOptions ()
    : box_mode (1),
      allow_big_records (true),
      allow_multi_xy_records (true)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief How to treat BOX records
   *
   *  This property specifies how to treat BOX records. 
   *  Allowed values are 0 (ignore), 1 (treat as rectangles), 2 (treat as boundaries) or 3 (treat as errors).
   */
  unsigned int box_mode; 

  /**
   *  @brief Allow multiple big records
   *
   *  Setting this property to true allows using up to 65535 bytes (instead of 32767) per record
   *  by treating the record length as unsigned short rather than signed short.
   *  This allows bigger polygons (up to ~8000 points) without having to use multiple XY records.
   */
  bool allow_big_records;

  /**
   *  @brief Allow multiple XY records in BOUNDARY elements for unlimited large polygons
   *
   *  Setting this property to true allows producing polygons with an unlimited number of points
   *  by using multiple XY records. 
   */
  bool allow_multi_xy_records;

  /** 
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new GDS2ReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("GDS2");
    return n;
  }
};

/**
 *  @brief Structure that holds the GDS2 specific options for the Writer
 *  NOTE: this structure is non-public linkage by intention. This way it's instantiated
 *  in all compile units and the shared object does not need to be linked.
 */
class DB_PLUGIN_PUBLIC GDS2WriterOptions
  : public FormatSpecificWriterOptions
{
public:
  /**
   *  @brief The constructor
   */
  GDS2WriterOptions ()
    : max_vertex_count (8000),
      no_zero_length_paths (false),
      multi_xy_records (false),
      resolve_skew_arrays (false),
      max_cellname_length (32000),
      libname ("LIB"),
      user_units (1.0),
      write_timestamps (true),
      write_cell_properties (false),
      write_file_properties (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Maximum number of vertices for polygons to write
   *
   *  This property describes the maximum number of point for polygons in GDS2 files.
   *  Polygons with more points will be split.
   *  The minimum value for this property is 4. If "multi_xy_records" is true, this
   *  property is not used. Instead, the number of points is unlimited.
   */
  unsigned int max_vertex_count;

  /**
   *  @brief Eliminate zero-length paths
   *
   *  If this option is set, zero-length paths are replaced by their polygon equivalent.
   *  For round paths this involves resolution into a polygon with the number of
   *  points specified in the "circle_points" configuration.
   */
  bool no_zero_length_paths;

  /**
   *  @brief Use multiple XY records in BOUNDARY elements for unlimited large polygons
   *
   *  Setting this property to true allows produce polygons with an unlimited number of points
   *  at the cost of incompatible formats.
   */
  bool multi_xy_records;

  /**
   *  @brief Resolve skew arrays into single instances
   *
   *  Setting this property to true will resolve skew (non-orthogonal) arrays into single instances.
   */
  bool resolve_skew_arrays;

  /**
   *  @brief Maximum length of cell names
   *
   *  This property describes the maximum number of characters for cell names.
   *  Longer cell names will be shortened.
   */
  unsigned int max_cellname_length;

  /**
   *  @brief The library name
   *
   *  This property describes that library name written to the LIBNAME record.
   */
  std::string libname;

  /**
   *  @brief The user units to use
   *
   *  This property describes what user units to use (in micron)
   */
  double user_units;

  /**
   *  @brief Write current time into timestamps
   */
  bool write_timestamps;

  /**
   *  @brief Write cell properties (non-standard PROPATTR/PROPVALUE records)
   */
  bool write_cell_properties;

  /**
   *  @brief Write layout properties (non-standard PROPATTR/PROPVALUE records)
   */
  bool write_file_properties;

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual FormatSpecificWriterOptions *clone () const
  {
    return new GDS2WriterOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificWriterOptions
   */
  virtual const std::string &format_name () const
  {
    static std::string n ("GDS2");
    return n;
  }
};

} // namespace db

#endif

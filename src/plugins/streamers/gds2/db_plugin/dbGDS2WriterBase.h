
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#ifndef HDR_dbGDS2WriterBase
#define HDR_dbGDS2WriterBase

#include "dbPluginCommon.h"
#include "dbWriter.h"
#include "dbWriterTools.h"
#include "tlProgress.h"

namespace tl
{
  class OutputStream;
}

namespace db
{

class Layout;
class SaveLayoutOptions;

/**
 *  @brief Structure that holds the GDS2 specific options for the Writer
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
   *  Setting this property to true allows to produce unlimited polygons 
   *  at the cost of incompatible formats.
   */
  bool multi_xy_records;

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

/**
 *  @brief A GDS2 writer abstraction
 */

class DB_PLUGIN_PUBLIC GDS2WriterBase
  : public db::WriterBase
{
public:
  /**
   *  @brief Instantiate the writer
   */
  GDS2WriterBase ();

  /**
   *  @brief Write the layout object
   */
  void write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options);

protected:
  /**
   *  @brief Write a byte
   */
  virtual void write_byte (unsigned char b) = 0;

  /**
   *  @brief Write a record length short
   */
  virtual void write_record_size (int16_t i) = 0;

  /**
   *  @brief Write a record type short
   */
  virtual void write_record (int16_t i) = 0;

  /**
   *  @brief Write a short
   */
  virtual void write_short (int16_t i) = 0;

  /**
   *  @brief Write a long
   */
  virtual void write_int (int32_t l) = 0;

  /**
   *  @brief Write a double
   */
  virtual void write_double (double d) = 0;

  /**
   *  @brief Write the time
   */
  virtual void write_time (const short *time) = 0;

  /**
   *  @brief Write a string
   */
  virtual void write_string (const char *t) = 0;

  /**
   *  @brief Write a string
   */
  virtual void write_string (const std::string &t) = 0;

  /**
   *  @brief Set the stream to write the data to
   */
  virtual void set_stream (tl::OutputStream &stream) = 0;

  /**
   *  @brief Establish a checkpoint
   */
  virtual void progress_checkpoint () = 0;

  /**
   *  @brief Write a string plus record
   */
  void write_string_record (short record, const std::string &t);

  /**
   *  @brief Write an instance 
   */
  void write_inst (double sf, const db::Instance &instance, bool normalize, const db::Layout &layout, db::properties_id_type prop_id);

  /**
   *  @brief Write a shape as box
   */
  void write_box (int layer, int datatype, double sf, const db::Shape &shape, const db::Layout &layout, db::properties_id_type prop_id);

  /**
   *  @brief Write a shape as edge
   */
  void write_edge (int layer, int datatype, double sf, const db::Shape &shape, const db::Layout &layout, db::properties_id_type prop_id);

  /**
   *  @brief Write a shape as path
   */
  void write_path (int layer, int datatype, double sf, const db::Shape &shape, bool multi_xy, const db::Layout &layout, db::properties_id_type prop_id);

  /**
   *  @brief Write a shape as text
   */
  void write_text (int layer, int datatype, double sf, double dbu, const db::Shape &shape, const db::Layout &layout, db::properties_id_type prop_id);

  /**
   *  @brief Write a shape as polygon
   */
  void write_polygon (int layer, int datatype, double sf, const db::Shape &shape, bool multi_xy, size_t max_vertex, const db::Layout &layout, db::properties_id_type prop_id);

  /**
   *  @brief Write a polygon
   */
  void write_polygon (int layer, int datatype, double sf, const db::Polygon &polygon, bool multi_xy, size_t max_vertex, const db::Layout &layout, db::properties_id_type prop_id, bool merged);

  /**
   *  @brief Finish an element by writing the properties and ENDEL
   */
  void finish (const db::Layout &layout, db::properties_id_type prop_id);

private:
  db::WriterCellNameMap m_cell_name_map;

  void write_properties (const db::Layout &layout, db::properties_id_type prop_id);
};

} // namespace db

#endif


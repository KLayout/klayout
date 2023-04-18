
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
  void write_inst (double sf, const db::Instance &instance, bool normalize, bool resolve_skew_arrays, const db::Layout &layout, db::properties_id_type prop_id);

  /**
   *  @brief Write a shape as box
   */
  void write_box (int layer, int datatype, double sf, const db::Shape &shape, const db::Layout &layout, db::properties_id_type prop_id);

  /**
   *  @brief Write a shape as edge
   */
  void write_edge (int layer, int datatype, double sf, const db::Shape &shape, const db::Layout &layout, db::properties_id_type prop_id);

  /**
   *  @brief Writes an edge
   */
  void write_edge (int layer, int datatype, double sf, const db::Edge &edge, const db::Layout &layout, db::properties_id_type prop_id);

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
  void write_context_cell (db::Layout &layout, const short *time_data, const std::vector<cell_index_type> &cells);
};

} // namespace db

#endif


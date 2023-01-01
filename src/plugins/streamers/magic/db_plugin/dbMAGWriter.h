
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



#ifndef HDR_dbMAGWriter
#define HDR_dbMAGWriter

#include "dbPluginCommon.h"
#include "dbWriter.h"
#include "dbMAG.h"
#include "dbMAGFormat.h"
#include "dbSaveLayoutOptions.h"
#include "tlProgress.h"
#include "tlUri.h"

namespace tl
{
  class OutputStream;
}

namespace db
{

class Layout;
class SaveLayoutOptions;

/**
 *  @brief A MAG writer abstraction
 */
class DB_PLUGIN_PUBLIC MAGWriter
  : public db::WriterBase
{
public:
  /**
   *  @brief Instantiate the writer
   */
  MAGWriter ();

  /**
   *  @brief Write the layout object
   */
  void write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options);

  /**
   *  @brief Scales the polygon to Magic lambda space
   */
  db::Polygon scaled (const db::Polygon &poly);

  /**
   *  @brief Scales the box to Magic lambda space
   */
  db::Box scaled (const Box &bx) const;

  /**
   *  @brief Scales the vector to Magic lambda space
   */
  db::Vector scaled (const db::Vector &v) const;

  /**
   *  @brief Scales the point to Magic lambda space
   */
  db::Point scaled (const db::Point &p) const;

  /**
   *  @brief Returns true if the vector can be scaled to Magic rounding space without loss
   */
  bool needs_rounding (const db::Vector &v) const;

private:
  tl::OutputStream *mp_stream;
  MAGWriterOptions m_options;
  tl::AbsoluteProgress m_progress;
  tl::URI m_base_uri;
  std::string m_ext;
  size_t m_timestamp;
  std::map<db::cell_index_type, size_t> m_cell_id;
  double m_sf;
  std::string m_cellname;

  std::string filename_for_cell (db::cell_index_type ci, db::Layout &layout);
  void write_cell (db::cell_index_type ci, const std::vector <std::pair <unsigned int, db::LayerProperties> > &layers, db::Layout &layout, tl::OutputStream &os);
  void write_dummy_top (const std::set<db::cell_index_type> &cell_set, const db::Layout &layout, tl::OutputStream &os);
  void do_write_cell (db::cell_index_type ci, const std::vector <std::pair <unsigned int, db::LayerProperties> > &layers, db::Layout &layout, tl::OutputStream &os);
  void write_polygon (const db::Polygon &poly, const db::Layout &layout, tl::OutputStream &os);
  void write_label (const std::string &layer, const db::Text &text, const Layout &layout, tl::OutputStream &os);
  void write_instance (const db::CellInstArray &inst, const db::Layout &layout, tl::OutputStream &os);
  void write_single_instance (db::cell_index_type ci, ICplxTrans tr, db::Vector a, db::Vector b, unsigned long na, unsigned long nb, const db::Layout &layout, tl::OutputStream &os);
  std::string make_string (const std::string &s);
};

} // namespace db

#endif


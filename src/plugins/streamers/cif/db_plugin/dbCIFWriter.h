
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



#ifndef HDR_dbCIFWriter
#define HDR_dbCIFWriter

#include "dbPluginCommon.h"
#include "dbWriter.h"
#include "dbCIF.h"
#include "dbCIFFormat.h"
#include "dbSaveLayoutOptions.h"
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
 *  @brief A CIF writer abstraction
 */
class DB_PLUGIN_PUBLIC CIFWriter
  : public db::WriterBase
{
public:
  /**
   *  @brief Instantiate the writer
   */
  CIFWriter ();

  /**
   *  @brief Write the layout object
   */
  void write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options);

private:
  struct endl_tag { };

  tl::OutputStream *mp_stream;
  CIFWriterOptions m_options;
  tl::AbsoluteProgress m_progress;
  endl_tag m_endl;
  db::LayerProperties m_layer;
  bool m_needs_emit;
  
  CIFWriter &operator<<(const char *s);
  CIFWriter &operator<<(const std::string &s);
  CIFWriter &operator<<(endl_tag); 

  template<class X> CIFWriter &operator<<(const X &x) 
  {
    return (*this << tl::to_string(x));
  }

  void write_texts (const db::Layout &layout, const db::Cell &cell, unsigned int layer, double tl_scale);
  void write_polygons (const db::Layout &layout, const db::Cell &cell, unsigned int layer, double tl_scale);
  void write_polygon (const db::Polygon &polygon, double tl_scale);
  void write_boxes (const db::Layout &layout, const db::Cell &cell, unsigned int layer, double tl_scale);
  void write_paths (const db::Layout &layout, const db::Cell &cell, unsigned int layer, double tl_scale);
  void write_edges (const db::Layout &layout, const db::Cell &cell, unsigned int layer, double tl_scale);
  const char *xy_sep () const;

  void emit_layer();
};

} // namespace db

#endif


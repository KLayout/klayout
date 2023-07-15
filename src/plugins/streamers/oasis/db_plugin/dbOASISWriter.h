
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


#ifndef HDR_dbOASISWriter
#define HDR_dbOASISWriter

#include "dbPluginCommon.h"
#include "dbWriter.h"
#include "dbOASIS.h"
#include "dbOASISFormat.h"
#include "dbSaveLayoutOptions.h"
#include "dbObjectWithProperties.h"
#include "dbHash.h"
#include "tlProgress.h"
#include "tlStream.h"

#include <string>

namespace tl
{
  class OutputStream;
}

namespace db
{

class Layout;
class SaveLayoutOptions;
class OASISWriter;

/**
 *  @brief A displacement list compactor
 *
 *  This object will collect objects  of the given kind and create
 *  OASIS repetitions then. For this, it creates a hash map collecting all 
 *  equivalent objects on "add" and their displacements. When "emit" is called,
 *  these displacements are converted to OASIS repetitions and
 *  emitted to the writer.
 */

const unsigned int max_oasis_compression_level = 10;

template <class Obj>
class Compressor 
{
public:
  /** 
   *  @brief Constructor
   *
   *  @param level The compression level 
   *
   *  Allowed levels are:
   *    0   - simple
   *    1   - form simple arrays
   *    2++ - search for 2nd, 3rd ... order neighbors
   */
  Compressor (unsigned int level)
    : m_level (level)
  {
    //  .. nothing yet ..
  }

  void add (const Obj &obj, const db::Vector &disp)
  {
    m_normalized [obj].push_back (disp);
  }

  void flush (db::OASISWriter *writer);

private:
  typedef std::vector<db::Vector> disp_vector;
  
  std::unordered_map <Obj, disp_vector> m_normalized;

  unsigned int m_level;
};

/**
 *  @brief A OASIS writer abstraction
 */
class DB_PLUGIN_PUBLIC OASISWriter
  : public db::WriterBase
{
public:
  /**
   *  @brief Instantiate the writer
   */
  OASISWriter ();

  /**
   *  @brief Write the layout object
   */
  void write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options);

  void write (const db::CellInstArray &inst_array, const db::Repetition &rep)
  {
    write (inst_array, 0, rep);
  }

  void write (const db::CellInstArrayWithProperties &inst_array, const db::Repetition &rep)
  {
    write (inst_array, inst_array.properties_id (), rep);
  }

  void write (const db::CellInstArray &inst_array, db::properties_id_type prop_id, const db::Repetition &rep);

  void write (const db::Text &text, const db::Repetition &rep)
  {
    write (text, 0, rep);
  }

  void write (const db::TextWithProperties &text, const db::Repetition &rep)
  {
    write (text, text.properties_id (), rep);
  }

  void write (const db::Text &text, db::properties_id_type prop_id, const db::Repetition &rep);

  void write (const db::Box &box, const db::Repetition &rep)
  {
    write (box, 0, rep);
  }

  void write (const db::BoxWithProperties &box, const db::Repetition &rep)
  {
    write (box, box.properties_id (), rep);
  }

  void write (const db::Box &box, db::properties_id_type prop_id, const db::Repetition &rep);

  void write (const db::Edge &edge, const db::Repetition &rep)
  {
    write (edge, 0, rep);
  }

  void write (const db::EdgeWithProperties &edge, const db::Repetition &rep)
  {
    write (edge, edge.properties_id (), rep);
  }

  void write (const db::Edge &edge, db::properties_id_type prop_id, const db::Repetition &rep);

  void write (const db::Path &path, const db::Repetition &rep)
  {
    write (path, 0, rep);
  }

  void write (const db::PathWithProperties &path, const db::Repetition &rep)
  {
    write (path, path.properties_id (), rep);
  }

  void write (const db::Path &path, db::properties_id_type prop_id, const db::Repetition &rep);

  void write (const db::SimplePolygon &simple_polygon, const db::Repetition &rep)
  {
    write (simple_polygon, 0, rep);
  }

  void write (const db::SimplePolygonWithProperties &simple_polygon, const db::Repetition &rep)
  {
    write (simple_polygon, simple_polygon.properties_id (), rep);
  }

  void write (const db::SimplePolygon &simple_polygon, db::properties_id_type prop_id, const db::Repetition &rep);

  void write (const db::Polygon &polygon, const db::Repetition &rep)
  {
    write (polygon, 0, rep);
  }

  void write (const db::PolygonWithProperties &polygon, const db::Repetition &rep)
  {
    write (polygon, polygon.properties_id (), rep);
  }

  void write (const db::Polygon &polygon, db::properties_id_type prop_id, const db::Repetition &rep);

private:
  tl::OutputStream *mp_stream;
  double m_sf;
  const db::Layout *mp_layout;
  const db::Cell *mp_cell;
  int m_layer;
  int m_datatype;
  bool m_write_context_info;
  std::vector<db::Vector> m_pointlist;
  tl::OutputMemoryStream m_cblock_buffer;
  tl::OutputMemoryStream m_cblock_compressed;
  bool m_in_cblock;
  unsigned long m_propname_id;
  unsigned long m_propstring_id;
  unsigned long m_textstring_id;
  bool m_proptables_written;

  std::map <std::string, unsigned long> m_textstrings;
  std::map <std::string, unsigned long> m_propnames;
  std::map <std::string, unsigned long> m_propstrings;

  typedef std::vector<tl::Variant> property_value_list;

  modal_variable<Repetition> mm_repetition;
  modal_variable<db::cell_index_type> mm_placement_cell;
  modal_variable<db::Coord> mm_placement_x;
  modal_variable<db::Coord> mm_placement_y;
  modal_variable<unsigned int> mm_layer;
  modal_variable<unsigned int> mm_datatype;
  modal_variable<unsigned int> mm_textlayer;
  modal_variable<unsigned int> mm_texttype;
  modal_variable<db::Coord> mm_text_x;
  modal_variable<db::Coord> mm_text_y;
  modal_variable<std::string> mm_text_string;
  modal_variable<db::Coord> mm_geometry_x;
  modal_variable<db::Coord> mm_geometry_y;
  modal_variable<db::Coord> mm_geometry_w;
  modal_variable<db::Coord> mm_geometry_h;
  modal_variable< std::vector<db::Vector> > mm_polygon_point_list;
  modal_variable<db::Coord> mm_path_halfwidth;
  modal_variable<db::Coord> mm_path_start_extension;
  modal_variable<db::Coord> mm_path_end_extension;
  modal_variable< std::vector<db::Vector> > mm_path_point_list;
  modal_variable<unsigned int> mm_ctrapezoid_type;
  modal_variable<db::Coord> mm_circle_radius;
  modal_variable<std::string> mm_last_property_name;
  modal_variable<bool> mm_last_property_is_sprop;
  modal_variable<property_value_list> mm_last_value_list;

  OASISWriterOptions m_options;
  tl::AbsoluteProgress m_progress;

  void write_record_id (char b);
  void write_byte (char b);
  void write_bytes (const char *b, size_t n);

  void write_astring (const char *s);
  void write_bstring (const char *s);
  void write_nstring (const char *s);
  std::string make_nstring (const char *s);
  std::string make_astring (const char *s);

  void write_gdelta (const db::Vector &p)
  {
    write_gdelta (p, m_sf);
  }

  void write_gdelta (const db::Vector &p, double sf);

  void write_coord (db::Coord c);
  void write_coord (db::Coord c, double sf);

  void write_ucoord (db::Coord c);
  void write_ucoord (db::Coord c, double sf);

  void write (double d);
  void write (float d);
  void write (long n);
  void write (unsigned long n);
  void write (long long n);
  void write (unsigned long long n);

  void write (int n)
  {
    write (long (n));
  }

  void write (unsigned int n)
  {
    write ((unsigned long) (n));
  }

  void write (const Repetition &rep);

  void begin_cblock ();
  void end_cblock ();

  void begin_table (size_t &pos);
  void end_table (size_t pos);

  void reset_modal_variables ();

  void emit_propname_def (db::properties_id_type prop_id);
  void emit_propstring_def (db::properties_id_type prop_id);
  void write_insts (const std::set <db::cell_index_type> &cell_set);

  void write_shapes (const db::LayerProperties &lprops, const db::Shapes &shapes);

  void write_props (db::properties_id_type prop_id);
  void write_property_def (const char *name_str, const std::vector<tl::Variant> &pvl, bool sflag);
  void write_property_def (const char *name_str, const tl::Variant &pv, bool sflag);
  void write_pointlist (const std::vector<db::Vector> &pointlist, bool for_polygons);

  void write_inst_with_rep (const db::CellInstArray &inst, db::properties_id_type prop_id, const db::Vector &disp, const db::Repetition &rep);

  void write_propname_table (size_t &propnames_table_pos, const std::vector<db::cell_index_type> &cells, const Layout &layout, const std::vector<std::pair<unsigned int, LayerProperties> > &layers);
  void write_propstring_table (size_t &propstrings_table_pos, const std::vector<db::cell_index_type> &cells, const Layout &layout, const std::vector<std::pair<unsigned int, LayerProperties> > &layers);
  void write_cellname_table (size_t &cellnames_table_pos, const std::vector<db::cell_index_type> &cells_by_index, const std::map<cell_index_type, size_t> *cell_positions, const Layout &layout);
  void write_textstring_table (size_t &textstrings_table_pos, const std::vector<db::cell_index_type> &cells, const Layout &layout, const std::vector<std::pair<unsigned int, LayerProperties> > &layers);
  void write_layername_table (size_t &layernames_table_pos, const std::vector<std::pair<unsigned int, LayerProperties> > &layers);
};

} // namespace db

#endif


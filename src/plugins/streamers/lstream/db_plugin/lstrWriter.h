
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#ifndef HDR_lstrWriter
#define HDR_lstrWriter

#include "lstrCompressed.h"

#include "dbPluginCommon.h"
#include "dbWriterTools.h"
#include "dbWriter.h"

#include "tlProgress.h"

#include "header.capnp.h"
#include "library.capnp.h"
#include "variant.capnp.h"
#include "geometry.capnp.h"
#include "cell.capnp.h"
#include "layoutView.capnp.h"

#include <capnp/serialize-packed.h>

namespace kj
{
  class BufferedOutputStream;
}

namespace lstr
{

/**
 *  @brief The LStream format stream writer
 */
class DB_PLUGIN_PUBLIC Writer
  : public db::WriterBase
{
public:
  /**
   *  @brief Instantiate the writer
   */
  Writer ();

  /**
   *  @brief Writes the layout object
   * 
   *  @param layout The layout object to write
   *  @param stream The stream to write to
   *  @param options The writer options to use
   */
  void write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options);

private:
  tl::OutputStream *mp_stream;
  tl::AbsoluteProgress m_progress;
  db::SaveLayoutOptions m_options;
  bool m_recompress;
  int m_compression_level;
  bool m_permissive;
  db::Layout *mp_layout;
  std::string m_cellname;
  int m_layout_view_id;
  int m_meta_data_view_id;
  std::map<db::lib_id_type, uint64_t> m_ls_lib_ids;
  std::vector<std::pair <unsigned int, db::LayerProperties> > m_layers_to_write;
  std::set<db::cell_index_type> m_cells_to_write;
  std::map<db::property_names_id_type, uint64_t> m_ls_prop_name_ids;
  std::map<db::properties_id_type, uint64_t> m_ls_prop_ids;
  std::map<std::string, uint64_t> m_text_strings;
  std::map<db::cell_index_type, uint64_t> m_ls_cell_ids;

  void yield_progress ();
  void warn (const std::string &msg);
  void write_header (kj::BufferedOutputStream &os);
  void write_library (kj::BufferedOutputStream &os);
  void make_library_refs_table (stream::library::LibraryRefs::Builder library_names);
  uint64_t get_library_ref_id (db::lib_id_type lib_id);
  void collect_property_ids (std::vector<db::properties_id_type> &prop_ids, std::vector<db::property_names_id_type> &prop_names);
  uint64_t make_property_id (db::properties_id_type id, std::vector<db::properties_id_type> &prop_ids, std::vector<db::property_names_id_type> &prop_names);
  uint64_t make_property_name_id_from_id (db::property_names_id_type name_id, std::vector<db::property_names_id_type> &prop_names);
  uint64_t make_property_name_id_from_variant (const tl::Variant &name, std::vector<db::property_names_id_type> &prop_names);
  uint64_t get_property_id (db::properties_id_type id);
  uint64_t get_property_name_id_from_id (db::property_names_id_type name_id);
  uint64_t get_property_name_id_from_variant (const tl::Variant &name);
  void make_property_names_tables (const std::vector<db::property_names_id_type> &prop_names, stream::library::PropertyNamesTable::Builder property_names);
  void make_properties_tables (const std::vector<db::properties_id_type> &prop_ids, stream::library::PropertiesTable::Builder properties);
  void make_variant_value (const tl::Variant &value, stream::variant::Variant::Builder builder);
  void collect_text_strings (std::vector<std::string> &text_strings);
  uint64_t make_text_string_id (const std::string &string, std::vector<std::string> &text_strings);
  uint64_t get_text_string_id (const std::string &string);
  void make_text_strings_table (const std::vector<std::string> &text_strings, stream::library::TextStringsTable::Builder table);
  void make_layer_table (stream::library::LayerTable::Builder layers);
  void make_cell_specs (stream::library::CellSpecsTable::Builder cell_specs);
  uint64_t get_cell_id (db::cell_index_type ci);
  void make_cell_hierarchy_tree (stream::library::CellHierarchyTree::Builder cell_tree);
  void make_meta_data (const db::Cell *cell, stream::metaData::MetaData::Builder meta_data);

  void write_cell (db::cell_index_type ci, kj::BufferedOutputStream &os);
  void write_layout_view (db::cell_index_type ci, kj::BufferedOutputStream &os);
  void write_meta_data_view (db::cell_index_type ci, kj::BufferedOutputStream &os);
  void make_repetition (const RegularArray &array, stream::repetition::Repetition::Builder builder);
  void make_repetition (const std::vector<db::Vector> &disp_array, stream::repetition::Repetition::Builder builder);
  void make_object (const db::SimplePolygon &obj, stream::geometry::SimplePolygon::Builder cpnp_obj);
  void make_object (const db::Polygon &obj, stream::geometry::Polygon::Builder cpnp_obj);
  void make_object (const db::Edge &obj, stream::geometry::Edge::Builder cpnp_obj);
  void make_object (const db::EdgePair &obj, stream::geometry::EdgePair::Builder cpnp_obj);
  void make_object (const db::Point &obj, stream::geometry::Point::Builder cpnp_obj);
  void make_object (const db::Box &obj, stream::geometry::Box::Builder cpnp_obj);
  void make_object (const db::Text &obj, stream::geometry::Label::Builder cpnp_obj);
  void make_object (const db::Path &obj, stream::geometry::Path::Builder cpnp_obj);
  void make_object (const db::CellInstArray &obj, stream::layoutView::CellInstance::Builder cpnp_obj);
  template <class Object, class Builder>
  void make_objects (const Compressed::compressed_container<Object> &container, Builder builder);
  stream::geometry::FixPointTransformation make_fixpoint_transformation (const db::Trans &trans);

  //  for debugging
  void replicate_message (const std::string &suffix, capnp::MessageBuilder &builder);
};

} // namespace db

#endif


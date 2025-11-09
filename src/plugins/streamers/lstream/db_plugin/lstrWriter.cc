
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "lstrWriter.h"
#include "lstrPlugin.h"
#include "lstrCompressor.h"
#include "lstrCompressed.h"
#include "lstrFormat.h"

#include "dbLibraryManager.h"
#include "dbLibraryProxy.h"
#include "dbLibrary.h"
#include "tlStream.h"
#include "tlAssert.h"
#include "tlException.h"
#include "tlEnv.h"

#include "metaDataView.capnp.h"

#include <capnp/message.h>
#include <kj/io.h>

//  Enable to replicate the messages into separate files for dumping
//  and inspection with "capnp decode".
//  Env var: $KLAYOUT_LSTREAM_REPLICATE_MESSAGES
static bool s_replicate_messages = tl::app_flag ("lstream-replicate-messages");

namespace lstr
{

class OutputStream 
  : public kj::OutputStream
{
public:
  OutputStream (tl::OutputStream *os)
    : mp_os (os)
  { 
    //  .. nothing yet ..
  }

  virtual void write (const void* buffer, size_t size)
  {
    mp_os->put ((const char *) buffer, size);
  }

private:
  tl::OutputStream *mp_os;
};

// ------------------------------------------------------------------
//  LStreamWriter implementation

Writer::Writer ()
  : mp_stream (0), m_progress (tl::to_string (tr ("Writing LStream file")), 1), mp_layout (0)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);

  m_recompress = true;
  m_compression_level = 2;
  m_permissive = true;
}

void 
Writer::write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options)
{
  //  TODO: this seems to be needed to properly enumerate the properties in "collect_property_ids"
  layout.update ();

  auto lstr_options = options.get_options<lstr::WriterOptions> ();
  m_permissive = lstr_options.permissive;
  m_compression_level = lstr_options.compression_level;
  m_recompress = lstr_options.recompress;

  double dbu = (options.dbu () == 0.0) ? layout.dbu () : options.dbu ();
  double sf = options.scale_factor () * (layout.dbu () / dbu);
  if (fabs (sf - 1.0) < 1e-9) {
    //  to avoid rounding problems, set to 1.0 exactly if possible.
    sf = 1.0;
  }

  //  TODO: implement
  if (sf != 1.0) {
    throw tl::Exception (tl::to_string (tr ("Scaling is not supported in LStream writer currently")));
  }

  mp_stream = &stream;
  m_options = options;
  mp_layout = &layout;
  m_cellname.clear ();
  m_layout_view_id = -1;
  m_meta_data_view_id = -1;

  m_layers_to_write.clear ();

#if KLAYOUT_MAJOR_VERSION == 0 && (KLAYOUT_MINOR_VERSION < 30 || (KLAYOUT_MINOR_VERSION == 30 && KLAYOUT_TINY_VERSION < 5))
  {
    options.get_valid_layers (layout, m_layers_to_write, db::SaveLayoutOptions::LP_OnlyNumbered);
    options.get_valid_layers (layout, m_layers_to_write, db::SaveLayoutOptions::LP_OnlyNamed);

    //  clean up layer duplicates - see TODO above
    auto lw = m_layers_to_write.begin ();
    std::set<unsigned int> lseen;
    for (auto l = m_layers_to_write.begin (); l != m_layers_to_write.end (); ++l) {
      if (lseen.find (l->first) == lseen.end ()) {
        *lw++ = *l;
        lseen.insert (l->first);
      }
    }
    m_layers_to_write.erase (lw, m_layers_to_write.end ());
  }
#else
  options.get_valid_layers (layout, m_layers_to_write, db::SaveLayoutOptions::LP_AsIs);
#endif

  m_cells_to_write.clear ();
  options.get_cells (layout, m_cells_to_write, m_layers_to_write);

  lstr::OutputStream os_adaptor (&stream);
  kj::BufferedOutputStreamWrapper kj_stream (os_adaptor);

  //  prepare the stream by writing the signature
  kj_stream.write (LStream_sig, strlen (LStream_sig) + 1);

  //  creates the global header
  write_header (kj_stream);

  //  this stream contains a single library currently
  write_library (kj_stream);

  for (auto c = mp_layout->begin_top_down (); c != mp_layout->end_top_down (); ++c) {
    if (m_cells_to_write.find (*c) != m_cells_to_write.end ()) {
      m_cellname = layout.cell_name (*c);
      write_cell (*c, kj_stream);
      m_cellname.clear ();
    }
  }
}

/**
 *  @brief Replicates a single message to a separate file for debugging
 * 
 *  Decoding a message is easer when it is written to a separate file.
 */
void 
Writer::replicate_message (const std::string &suffix, capnp::MessageBuilder &message)
{
  if (s_replicate_messages) {
    tl::OutputStream os_msg (mp_stream->path () + suffix);
    lstr::OutputStream ls_os_msg (&os_msg);
    kj::BufferedOutputStreamWrapper kj_os_msg (ls_os_msg);
    writePackedMessage (kj_os_msg, message);
  }
}

/**
 *  @brief Is called "frequently" to report the progress
 */
void
Writer::yield_progress ()
{
  m_progress.set (mp_stream->pos ());
}

/**
 *  @brief Issues a warning on the writer
 * 
 *  With "m_permissive" set to false, every warning will become an error.
 */
void 
Writer::warn (const std::string &msg)
{
  std::string msg_full = msg;
  if (! m_cellname.empty ()) {
    msg_full += tl::to_string (tr (", in cell: "));
    msg_full += m_cellname;
  }

  if (m_permissive) {
    tl::warn << msg_full;
  } else {
    throw tl::Exception (msg_full);
  }
}

/**
 *  @brief Writes the header message to the stream
 * 
 *  This is the main header provided by the LStream.
 *  It lists the libraries available inside this stream.
 *  For KLayout, this is only one library - of "layout"
 *  type.
 */
void
Writer::write_header (kj::BufferedOutputStream &os)
{
  capnp::MallocMessageBuilder message;

  stream::header::Header::Builder header = message.initRoot<stream::header::Header> ();
  header.setGenerator (LStream_generator);
  header.setTechnology (mp_layout->technology_name ());

  header.initLibraries (1);
  auto lib = header.getLibraries () [0];

  //  TODO: use layout's lib name?
  lib.setName ("");
  lib.setType ("layout");

  //  NOTE: our layout's metadata is placed in the library

  //  Write message to stream

  writePackedMessage (os, message);
  yield_progress ();
  replicate_message (".header", message);
}

/**
 *  @brief Writes the library header message to the stream
 * 
 *  This involves build a number of tables before they are written.
 *  As these tables are the basis for LStream Id allocation, this
 *  method must be called before LStream Ids can be obtained.
 */
void
Writer::write_library (kj::BufferedOutputStream &os)
{
  capnp::MallocMessageBuilder message;

  stream::library::Library::Builder library = message.initRoot<stream::library::Library> ();

  //  Library references

  make_library_refs_table (library.getLibraryRefs ());

  //  Properties

  {
    std::vector<db::properties_id_type> prop_ids;
    std::vector<db::property_names_id_type> prop_names;
    collect_property_ids (prop_ids, prop_names);
    make_property_names_tables (prop_names, library.getPropertyNamesTable ());
    make_properties_tables (prop_ids, library.getPropertiesTable ());
  }

  //  Text strings

  {
    std::vector<std::string> text_strings;
    collect_text_strings (text_strings);
    make_text_strings_table (text_strings, library.getTextStringsTable ());
  }

  //  View specs table
  //  NOTE: currently there are only "layout" views and optionally "metaData" views

  {
    m_layout_view_id = 0;
    m_meta_data_view_id = -1;

    bool needs_meta_data_view = false;
    for (auto c = m_cells_to_write.begin (); c != m_cells_to_write.end () && !needs_meta_data_view; ++c) {
      needs_meta_data_view = (mp_layout->begin_meta (*c) != mp_layout->end_meta (*c));
    }

    auto view_specs = library.getViewSpecsTable ();

    view_specs.initViewSpecs (needs_meta_data_view ? 2 : 1);

    auto layout_view = view_specs.getViewSpecs () [m_layout_view_id];

    layout_view.setName ("layout");
    layout_view.setClass ("LayoutView");
    layout_view.setPropertySetId (get_property_id (mp_layout->prop_id ()));

    //  Computes the resolution:
    //  Rounds to integer if "close to one". This achieves a 
    //  kind of normalization and prevents propagation of rounding
    //  errors.
    double resolution = 1.0 / mp_layout->dbu ();
    double integer_resolution = floor (resolution + 0.5);
    if (std::abs (resolution - integer_resolution) < 1e-10) {
      resolution = integer_resolution;
    }

    layout_view.setResolution (integer_resolution);

    make_meta_data (0, layout_view.getMetaData ());

    //  adds a meta data view if needed
    if (needs_meta_data_view) {
      m_meta_data_view_id = 1;
      auto meta_data_view = view_specs.getViewSpecs () [m_meta_data_view_id];
      meta_data_view.setName ("metaData");
      meta_data_view.setClass ("MetaDataView");
    }
  }

  //  Layer table

  {
    auto view_specs = library.getViewSpecsTable ();
    auto layout_view = view_specs.getViewSpecs () [m_layout_view_id];
    make_layer_table (layout_view.getLayerTable ());
  }

  //  Cell specs table

  make_cell_specs (library.getCellSpecsTable ());

  //  Cell hierachy tree

  make_cell_hierarchy_tree (library.getCellHierarchyTree ());

  //  Write message to stream

  writePackedMessage (os, message);
  yield_progress ();
  replicate_message (".library", message);
}

/**
 *  @brief Produces a variant value to a stream::variant::Variant struct
 */
void
Writer::make_variant_value (const tl::Variant &value, stream::variant::Variant::Builder builder)
{
  if (value.is_nil ()) {
    builder.getValue ().setNil ();
  } else if (value.is_bool ()) {
    builder.getValue ().setBool (value.to_bool ());
  } else if (value.is_a_string ()) {
    builder.getValue ().setText (value.to_string ());
  } else if (value.can_convert_to_ulonglong ()) {
    builder.getValue ().setUint64 (uint64_t (value.to_ulonglong ()));
  } else if (value.can_convert_to_longlong ()) {
    builder.getValue ().setInt64 (int64_t (value.to_longlong ()));
  } else if (value.can_convert_to_double ()) {
    builder.getValue ().setDouble (value.to_double ());
  } else if (value.is_user ()) {
    //  NOTE: the "klayout:" prefix indicates the object is in KLayout's
    //  object serialization notation.
    builder.getValue ().setObject (std::string ("klayout:") + value.to_parsable_string ());
  } else if (value.is_list ()) {
    builder.getValue ().initList (value.size ());
    size_t index = 0;
    for (auto i = value.begin (); i != value.end (); ++i, ++index) {
      make_variant_value (*i, builder.getValue ().getList ()[index]);
    }
  } else if (value.is_array ()) {
    builder.getValue ().initArray (value.array_size ());
    size_t index = 0;
    for (auto i = value.begin_array (); i != value.end_array (); ++i, ++index) {
      make_variant_value (i->first, builder.getValue ().getArray ()[index].getKey ());
      make_variant_value (i->second, builder.getValue ().getArray ()[index].getValue ());
    }
  }
}

/**
 *  @brief Produces the library names table
 * 
 *  This method collects the library references from all cells to write and 
 *  produces a table with all these references. It will also assign 
 *  library name Ids in that step.
 */
void
Writer::make_library_refs_table (stream::library::LibraryRefs::Builder library_refs)
{
  m_ls_lib_ids.clear ();
  std::vector<std::string> lib_names;

  for (auto c = m_cells_to_write.begin (); c != m_cells_to_write.end (); ++c) {

    const db::Cell &cell = mp_layout->cell (*c);
    const db::LibraryProxy *lib_proxy = dynamic_cast<const db::LibraryProxy *> (&cell);
    if (lib_proxy) {

      auto lib_id = lib_proxy->lib_id ();
      if (m_ls_lib_ids.find (lib_id) == m_ls_lib_ids.end ()) {
        const db::Library *lib = db::LibraryManager::instance ().lib (lib_id);
        lib_names.push_back (lib->get_name ());
        m_ls_lib_ids.insert (std::make_pair (lib_id, lib_names.size ()));
      }

    }

  }

  library_refs.initRefs (lib_names.size ());
  for (auto n = lib_names.begin (); n != lib_names.end (); ++n) {
    auto ref = library_refs.getRefs ()[n - lib_names.begin ()];
    ref.setLibraryName (*n);
  }
}

/**
 *  @brief Gets the library name Id from a given library Id
 * 
 *  Note that "make_library_names_table" must have been called before
 *  this method can be used.
 */
uint64_t 
Writer::get_library_ref_id (db::lib_id_type lib_id)
{
  auto i = m_ls_lib_ids.find (lib_id);
  tl_assert (i != m_ls_lib_ids.end ());
  return i->second;
}

/**
 *  @brief Collect all KLayout property name Ids and properties Ids used in the context of this writer
 * 
 *  This method scans layout, cells, meta info, instances and shapes for used properties
 *  and registers properties Ids and names.
 * 
 *  @param prop_ids Where the properties Ids are collected
 *  @param prop_names Where the property names are collected
 */
void 
Writer::collect_property_ids (std::vector<db::properties_id_type> &prop_ids, std::vector<db::property_names_id_type> &prop_names)
{
  make_property_id (mp_layout->prop_id (), prop_ids, prop_names);

  for (auto c = m_cells_to_write.begin (); c != m_cells_to_write.end (); ++c) {

    const db::Cell &cell = mp_layout->cell (*c);

    //  PCell parameters only employ the name ID space
    auto param_dict = mp_layout->get_named_pcell_parameters (*c);
    for (auto p = param_dict.begin (); p != param_dict.end (); ++p) {
      make_property_name_id_from_variant (tl::Variant (p->first), prop_names);
    }

    make_property_id (cell.prop_id (), prop_ids, prop_names);

    for (auto l = m_layers_to_write.begin (); l != m_layers_to_write.end (); ++l) {
      for (auto s = db::ShapeIterator (cell.shapes (l->first), db::ShapeIterator::AllWithProperties); ! s.at_end (); s.finish_array ()) {
        make_property_id (s->prop_id (), prop_ids, prop_names);
      }
    }

    for (auto i = cell.begin (); ! i.at_end (); ++i) {
      make_property_id (i->prop_id (), prop_ids, prop_names);
    }

  }
}

namespace 
{

struct ComparePropertyNameIdByValue
{
  bool operator() (db::property_names_id_type a, db::property_names_id_type b) const
  {
    const tl::Variant &na = db::property_name (a);
    const tl::Variant &nb = db::property_name (b);
    return na.less (nb);
  }
};

}

/**
 *  @brief Gets the LStream property set Id from a KLayout properties Id
 * 
 *  If the properties Id is not registered yet, a new LStream Id will be generated.
 * 
 *  @param id The KLayout properties Id
 *  @param prop_id New KLayout properties Ids will be added here
 *  @param prop_names New property names go here (happens when a new property set is registered)
 */
uint64_t 
Writer::make_property_id (db::properties_id_type id, std::vector<db::properties_id_type> &prop_ids, std::vector<db::property_names_id_type> &prop_names)
{
  if (id != 0) {

    auto i = m_ls_prop_ids.find (id);
    if (i != m_ls_prop_ids.end ()) {

      return i->second;

    } else {

      uint64_t ls_id = uint64_t (prop_ids.size () + 1);
      m_ls_prop_ids.insert (std::make_pair (id, ls_id));
      prop_ids.push_back (id);

      auto ps = db::properties (id);
      std::set<db::property_names_id_type, ComparePropertyNameIdByValue> ps_sorted;
      for (auto i = ps.begin (); i != ps.end (); ++i) {
        ps_sorted.insert (i->first);
      }

      for (auto i = ps_sorted.begin (); i != ps_sorted.end (); ++i) {
        make_property_name_id_from_id (*i, prop_names);
      }

      return ls_id;

    }

  } else {
    return 0;
  }
}

/**
 *  @brief Gets the LStream property name Id for a given name (by variant)
 * 
 *  @param name The name to register
 *  @param prop_names New names will be added here
 */
uint64_t 
Writer::make_property_name_id_from_variant (const tl::Variant &name, std::vector<db::property_names_id_type> &prop_names)
{
  return make_property_name_id_from_id (db::property_names_id (name), prop_names);
}

/**
 *  @brief Gets the LStream property name Id for a given name (by KLayout property name Id)
 * 
 *  @param name_id The name Id to register
 *  @param prop_names New names will be added here
 */
uint64_t 
Writer::make_property_name_id_from_id (db::property_names_id_type name_id, std::vector<db::property_names_id_type> &prop_names)
{
  auto i = m_ls_prop_name_ids.find (name_id);
  if (i != m_ls_prop_name_ids.end ()) {
    return i->second;
  } else {
    uint64_t ls_name_id = prop_names.size ();
    prop_names.push_back (name_id);
    m_ls_prop_name_ids.insert (std::make_pair (name_id, ls_name_id));
    return ls_name_id;
  }
}

/**
 *  @brief Gets the LStream property set Id from a KLayout properties Id
 *  
 *  If the properties Id is not valid, this method will assert.
 *  Note, that "make_property_id" must have been called before to register
 *  the property set.
 */
uint64_t
Writer::get_property_id (db::properties_id_type id)
{
  if (id == 0) {
    return 0;
  } else {
    auto i = m_ls_prop_ids.find (id);
    tl_assert (i != m_ls_prop_ids.end ());
    return i->second;
  }
}

/**
 *  @brief Obtain the LStream property name Id from KLayout property name Id
 * 
 *  If the KLayout property name Id is not valid, this method asserts.
 *  Note that "make_property_name_id_from_id" must have been called before to
 *  register the name.
 */
uint64_t 
Writer::get_property_name_id_from_id (db::property_names_id_type name_id)
{
  auto i = m_ls_prop_name_ids.find (name_id);
  tl_assert (i != m_ls_prop_name_ids.end ());
  return i->second;
}

/**
 *  @brief Obtain the LStream property name Id from a name variant
 * 
 *  If the name variant is not a valid name, this method asserts.
 */
uint64_t
Writer::get_property_name_id_from_variant (const tl::Variant &name)
{
  return get_property_name_id_from_id (db::property_names_id (name));
}

/**
 *  @brief Produces the property names table from a given set of KLayout property name Ids
 */
void
Writer::make_property_names_tables (const std::vector<db::property_names_id_type> &prop_names, stream::library::PropertyNamesTable::Builder property_names)
{
  property_names.initNames (prop_names.size ());

  for (auto i = prop_names.begin (); i != prop_names.end (); ++i) {
    stream::propertySet::PropertyName::Builder property_name = property_names.getNames ()[i - prop_names.begin ()];
    //  No namespace yet: property_name.setNamespaceId (0);
    make_variant_value (db::property_name (*i), property_name.getName ());
  }
}

/**
 *  @brief Produces the property sets table from a given set of KLayout properties Ids
 * 
 *  Note that the property names must have been collected already before this 
 *  method can be used ("make_property_name_id_from_id").
 */
void
Writer::make_properties_tables (const std::vector<db::properties_id_type> &prop_ids, stream::library::PropertiesTable::Builder properties)
{
  properties.initPropertySets (prop_ids.size ());

  for (auto p = prop_ids.begin (); p != prop_ids.end (); ++p) {

    auto set = properties.getPropertySets ()[p - prop_ids.begin ()];

    //  NOTE: we go through the map to become independent from the name order
    auto map = db::properties (*p).to_map ();
    set.initProperties (map.size ());

    size_t index = 0;
    for (auto m = map.begin (); m != map.end (); ++m, ++index) {
      auto ni = m_ls_prop_name_ids.find (db::property_names_id (m->first));
      tl_assert (ni != m_ls_prop_name_ids.end ());
      auto prop = set.getProperties ()[index];
      prop.setNameId (ni->second);
      make_variant_value (m->second, prop.getValue ());
    }

  }
}

/**
 *  @brief Collects all used text strings
 */
void 
Writer::collect_text_strings (std::vector<std::string> &text_strings)
{
  for (auto c = m_cells_to_write.begin (); c != m_cells_to_write.end (); ++c) {

    const db::Cell &cell = mp_layout->cell (*c);

    for (auto l = m_layers_to_write.begin (); l != m_layers_to_write.end (); ++l) {
      for (auto s = db::ShapeIterator (cell.shapes (l->first), db::ShapeIterator::Texts); ! s.at_end (); s.finish_array ()) {
        make_text_string_id (std::string (s->text_string ()), text_strings);
      }
    }

  }
}

/**
 *  @brief Gets the LStream text string Id for a given text
 * 
 *  This method will create a new entry if the string had not been
 *  registered before.
 * 
 *  @param string The new text string
 *  @param text_strings New strings are added here
 */
uint64_t
Writer::make_text_string_id (const std::string &string, std::vector<std::string> &text_strings)
{
  auto i = m_text_strings.find (string);
  if (i == m_text_strings.end ()) {
    uint64_t id = text_strings.size ();
    text_strings.push_back (string);
    m_text_strings.insert (std::make_pair (string, id));
    return id;
  } else {
    return i->second;
  }
}

/**
 *  @brief Gets the LStream text string Id for a given text
 * 
 *  Note that make_text_string_id had to be called before.
 *  This method will assert if the string does not have an associated 
 *  text string Id.
 */
uint64_t
Writer::get_text_string_id (const std::string &string)
{
  auto i = m_text_strings.find (string);
  tl_assert (i != m_text_strings.end ());
  return i->second;
}

/**
 *  @brief Produces the text strings table on stream::library::TextStringsTable
 * 
 *  @param text_strings The list of text strings, where the LStream text string Id in an index in that table
 *  @param table The Builder for the table
 */
void 
Writer::make_text_strings_table (const std::vector<std::string> &text_strings, stream::library::TextStringsTable::Builder table)
{
  table.initTextStrings (text_strings.size ());

  for (auto i = text_strings.begin (); i != text_strings.end (); ++i) {
    table.getTextStrings ().set (i - text_strings.begin (), i->c_str ());
  }
}

/**
 *  @brief Produces the layer table on stream::library::LayerTable
 * 
 *  Only selected layers will be produced.
 */
void
Writer::make_layer_table (stream::library::LayerTable::Builder layers)
{
  layers.initLayerEntries (m_layers_to_write.size ());

  for (auto l = m_layers_to_write.begin (); l != m_layers_to_write.end (); ++l) {

    auto lp = l->second;

    //  NOTE: currently, the purpose is always DRAWING
    auto le = layers.getLayerEntries ()[l - m_layers_to_write.begin ()];
    if (lp.layer >= 0 && lp.datatype >= 0) {
      le.initLayerNumbers (2);
      le.getLayerNumbers ().set (0, lp.layer);
      le.getLayerNumbers ().set (1, lp.datatype);
    }
    le.setName (lp.name);
    le.setPurpose (stream::library::LayerEntry::Purpose::DRAWING);

  }
}

/**
 *  @brief Produces the cell specifications on stream::library::CellSpecsTable
 * 
 *  Only selected cells will be produced. The cell specs are generated top-down.
 */
void
Writer::make_cell_specs (stream::library::CellSpecsTable::Builder cell_specs)
{
  cell_specs.initCellSpecs (m_cells_to_write.size ());

  size_t index = 0;

  m_ls_cell_ids.clear ();
  for (auto c = mp_layout->begin_top_down (); c != mp_layout->end_top_down (); ++c) {

    if (m_cells_to_write.find (*c) == m_cells_to_write.end ()) {
      continue;
    }

    m_ls_cell_ids.insert (std::make_pair (*c, index));

    auto cs = cell_specs.getCellSpecs ()[index];
    const db::Cell &cell = mp_layout->cell (*c);

    cs.setName (mp_layout->cell_name (*c));

    const db::LibraryProxy *lib_proxy = dynamic_cast<const db::LibraryProxy *> (&cell);
    if (lib_proxy) {
      cs.setLibraryCellName (cell.get_basic_name ());
      cs.setLibraryRefId (get_library_ref_id (lib_proxy->lib_id ()));
    }

    if (mp_layout->is_pcell_instance (*c).first) {

      //  Only PCells have a "parameters" object. Others won't initialize "parameters"
      auto param_dict = mp_layout->get_named_pcell_parameters (*c);
      auto pcell_parameters = cs.getParameters ().initValues (param_dict.size ());

      size_t pindex = 0;
      for (auto p = param_dict.begin (); p != param_dict.end (); ++p, ++pindex) {
        auto pn = pcell_parameters[pindex];
        pn.setNameId (get_property_name_id_from_variant (tl::Variant (p->first)));
        make_variant_value (p->second, pn.getValue ());
      }

    }

    cs.setPropertySetId (get_property_id (cell.prop_id ()));

    ++index;

  }
}

/**
 *  @brief Gets the LStream Id for a given KLayout cell Id
 * 
 *  Note that the "make_cell_specs" must have been executed, before this method
 *  can be used.
 */
uint64_t 
Writer::get_cell_id (db::cell_index_type ci)
{
  auto i = m_ls_cell_ids.find (ci);
  tl_assert (i != m_ls_cell_ids.end ());
  return i->second;
}

/**
 *  @brief Produces the cell hierarchy tree on stream::library::CellHierarchyTree
 * 
 *  This method will consider the selected cells and produce a cell hierarchy
 *  tree in top-down mode.
 */
void
Writer::make_cell_hierarchy_tree (stream::library::CellHierarchyTree::Builder cell_tree)
{
  size_t top_cell_count = 0;
  for (auto c = mp_layout->begin_top_down (); c != mp_layout->end_top_cells (); ++c) {
    if (m_cells_to_write.find (*c) != m_cells_to_write.end ()) {
      ++top_cell_count;
    }
  }

  cell_tree.setNumberOfTopCells (top_cell_count);
  cell_tree.initNodes (m_cells_to_write.size ());

  size_t index = 0;
  for (auto c = mp_layout->begin_top_down (); c != mp_layout->end_top_down (); ++c) {

    if (m_cells_to_write.find (*c) == m_cells_to_write.end ()) {
      continue;
    }

    auto cn = cell_tree.getNodes ()[index];
    cn.setCellId (get_cell_id (*c));

    std::set<uint64_t> children;
    const db::Cell &cell = mp_layout->cell (*c);
    for (auto cc = cell.begin_child_cells (); ! cc.at_end (); ++cc) {
      if (m_cells_to_write.find (*cc) != m_cells_to_write.end ()) {
        children.insert (get_cell_id (*cc));
      }
    }

    cn.initChildCellIds (children.size ());
    size_t cindex = 0;
    for (auto cc = children.begin (); cc != children.end (); ++cc, ++cindex) {
      cn.getChildCellIds ().set (cindex, *cc);
    }

    ++index;

  }

  //  all cells have been written
  tl_assert (index == m_cells_to_write.size ());
}

/**
 *  @brief Generates meta info for the given cell or layout in the stream::propertySet::PropertySet struct
 * 
 *  If cell is 0, the layout meta info will be generated. Otherwise the
 *  cell specific meta info will be produced. In both cases the meta info
 *  is written to the given PropertySet builder.
 */
void
Writer::make_meta_data (const db::Cell *cell, stream::metaData::MetaData::Builder meta_data)
{
  auto mfrom = cell ? mp_layout->begin_meta (cell->cell_index ()) : mp_layout->begin_meta ();
  auto mto   = cell ? mp_layout->end_meta (cell->cell_index ()) : mp_layout->end_meta ();

  size_t count = 0;
  for (auto m = mfrom; m != mto; ++m) {
    if (m->second.persisted) {
      ++count;
    }
  }
  meta_data.initEntries (count);

  size_t index = 0;
  for (auto m = mfrom; m != mto; ++m) {
    if (m->second.persisted) {
      auto p = meta_data.getEntries ()[index];
      auto name = mp_layout->meta_info_name (m->first);
      p.setName (name);
      p.setDescription (m->second.description);
      make_variant_value (m->second.value, p.getValue ());
      ++index;
    }
  } 
}

/**
 *  @brief Writes the cell message for the given cell, followed by the layout view message
 * 
 *  @param ci The cell for which to generate the message
 *  @param os The stream where to write the message
 * 
 *  This method generates a single-view cell message (the view is only a layout view).
 */
void 
Writer::write_cell (db::cell_index_type ci, kj::BufferedOutputStream &os)
{
  bool needs_layout_view = ! mp_layout->cell (ci).is_ghost_cell ();
  bool needs_meta_data_view = mp_layout->begin_meta (ci) != mp_layout->end_meta (ci);

  capnp::MallocMessageBuilder message;

  stream::cell::Cell::Builder cell = message.initRoot<stream::cell::Cell> ();

  cell.initViewIds ((needs_layout_view ? 1 : 0) + (needs_meta_data_view ? 1 : 0));

  int view_index = 0;
  if (needs_layout_view) {
    tl_assert (m_layout_view_id >= 0);
    cell.getViewIds ().set (view_index++, (unsigned int) m_layout_view_id);
  }
  if (needs_meta_data_view) {
    tl_assert (m_meta_data_view_id >= 0);
    cell.getViewIds ().set (view_index++, (unsigned int) m_meta_data_view_id);
  }

  writePackedMessage (os, message);
  yield_progress ();
  replicate_message (std::string (".cell_") + mp_layout->cell_name (ci), message);

  if (needs_layout_view) {
    write_layout_view (ci, os);
  }
  if (needs_meta_data_view) {
    write_meta_data_view (ci, os);
  }
}

/**
 *  @brief generates and writes a layout view message for the given cell
 * 
 *  @param ci The cell for which to generate the message
 *  @param os The stream where to write the message
 * 
 *  Generating the message involves compressing and producing the
 *  instances and shapes for various kinds on each layer.
 */
void 
Writer::write_layout_view (db::cell_index_type ci, kj::BufferedOutputStream &os)
{
  capnp::MallocMessageBuilder message;

  auto layout_view = message.initRoot<stream::layoutView::LayoutView> ();
  const db::Cell &cell = mp_layout->cell (ci);

  std::vector<std::pair<unsigned int, size_t> > layers_for_cell;
  layers_for_cell.reserve (m_layers_to_write.size ());
  for (auto l = m_layers_to_write.begin (); l != m_layers_to_write.end (); ++l) {
    if (! cell.shapes (l->first).empty ()) {
      layers_for_cell.push_back (std::make_pair (l->first, l - m_layers_to_write.begin ()));
    }
  }

  layout_view.initLayers (layers_for_cell.size ());

  for (auto l = layers_for_cell.begin (); l != layers_for_cell.end (); ++l) {

    auto layer = layout_view.getLayers () [l - layers_for_cell.begin ()];
    layer.setLayerId (l->second);

    Compressed compressed;
    compressed.compress_shapes (cell.shapes (l->first), m_compression_level, m_recompress); 

    layer.initRepetitions (compressed.num_arrays ());
    for (auto r = compressed.begin_regular_arrays (); r != compressed.end_regular_arrays (); ++r) {
      tl_assert (r->second > 0);
      make_repetition (r->first, layer.getRepetitions ()[r->second - 1]);
    }
    for (auto r = compressed.begin_irregular_arrays (); r != compressed.end_irregular_arrays (); ++r) {
      tl_assert (r->second > 0);
      make_repetition (r->first, layer.getRepetitions ()[r->second - 1]);
    }

    make_objects (compressed.get_container<db::Point> (), layer.getPoints ());
    make_objects (compressed.get_container<db::Box> (), layer.getBoxes ());
    make_objects (compressed.get_container<db::Edge> (), layer.getEdges ());
    make_objects (compressed.get_container<db::EdgePair> (), layer.getEdgePairs ());
    make_objects (compressed.get_container<db::Text> (), layer.getLabels ());
    make_objects (compressed.get_container<db::Polygon> (), layer.getPolygons ());
    make_objects (compressed.get_container<db::SimplePolygon> (), layer.getSimplePolygons ());
    make_objects (compressed.get_container<db::Path> (), layer.getPaths ());

  }

  //  collects and writes the bounding box from the layers we want to write
  db::Box bbox;
  for (auto l = m_layers_to_write.begin (); l != m_layers_to_write.end (); ++l) {
    bbox += cell.bbox (l->first);
  }
  make_object (bbox, layout_view.getBoundingBox ());

  //  instances 
  {
    Compressed compressed;
    compressed.compress_instances (cell.begin (), m_cells_to_write, m_compression_level); 

    layout_view.initInstanceRepetitions (compressed.num_arrays ());
    for (auto r = compressed.begin_regular_arrays (); r != compressed.end_regular_arrays (); ++r) {
      tl_assert (r->second > 0);
      make_repetition (r->first, layout_view.getInstanceRepetitions ()[r->second - 1]);
    }
    for (auto r = compressed.begin_irregular_arrays (); r != compressed.end_irregular_arrays (); ++r) {
      tl_assert (r->second > 0);
      make_repetition (r->first, layout_view.getInstanceRepetitions ()[r->second - 1]);
    }

    make_objects (compressed.get_container<db::CellInstArray> (), layout_view.getInstances ());
  }

  writePackedMessage (os, message);
  yield_progress ();
  replicate_message (std::string (".lv_") + mp_layout->cell_name (ci), message);
}

/**
 *  @brief generates and writes a meta data view message for the given cell
 * 
 *  @param ci The cell for which to generate the message
 *  @param os The stream where to write the message
 */
void 
Writer::write_meta_data_view (db::cell_index_type ci, kj::BufferedOutputStream &os)
{
  capnp::MallocMessageBuilder message;

  auto meta_data_view = message.initRoot<stream::metaDataView::MetaDataView> ();

  make_meta_data (&mp_layout->cell (ci), meta_data_view.getData ());

  writePackedMessage (os, message);
  yield_progress ();
  replicate_message (std::string (".lv_") + mp_layout->cell_name (ci), message);
}

/**
 *  @brief Creates a regular repetition from the RegularArray object
 * 
 *  The regular array should not be a null array.
 */
void 
Writer::make_repetition (const RegularArray &array, stream::repetition::Repetition::Builder builder)
{
  if (array.a ().y () == 0 && array.b ().x () == 0) {

    auto regular = builder.getTypes ().initRegularOrtho ();
    regular.setDx (array.a ().x ());
    regular.setDy (array.b ().y ());
    regular.setNx (array.na ());
    regular.setNy (array.nb ());

  } else if (array.a ().x () == 0 && array.b ().y () == 0) {

    auto regular = builder.getTypes ().initRegularOrtho ();
    regular.setDx (array.b ().x ());
    regular.setDy (array.a ().y ());
    regular.setNx (array.nb ());
    regular.setNy (array.na ());

  } else {

    auto regular = builder.getTypes ().initRegular ();
    auto a = regular.getA ();
    a.setDx (array.a ().x ());
    a.setDy (array.a ().y ());
    auto b = regular.getB ();
    b.setDx (array.b ().x ());
    b.setDy (array.b ().y ());
    regular.setNa (array.na ());
    regular.setNb (array.nb ());

  }
}

/**
 *  @brief Creates an enumerated stream::repetition::Repetition object from a sequence of displacements
 *  
 *  The list of displacements is directly converted into the repetition object.
 *  The implied zero-displacement element at the beginning must not be added at the front.
 */
void 
Writer::make_repetition (const std::vector<db::Vector> &disp_array, stream::repetition::Repetition::Builder builder)
{
  auto enumerated = builder.getTypes ().initEnumerated ();

  enumerated.initDeltas (disp_array.size ());

  size_t index = 0;
  db::Vector dl;
  for (auto d = disp_array.begin (); d != disp_array.end (); ++d, ++index) {
    auto delta = enumerated.getDeltas ()[index];
    db::Vector dd = *d - dl;
    dl = *d; 
    delta.setDx (dd.x ());
    delta.setDy (dd.y ());
  }
}

/**
 *  @brief Creates a stream::geometry::Contour struct from a sequence of points
 * 
 *  @param begin The begin iterator for the point sequence
 *  @param end The end iterator for the point sequence
 *  @param n The number of points
 *  @param builder The stream::geometry::Contour builder
 */
template <class Iter>
static void 
make_contour (Iter begin, Iter end, size_t n, stream::geometry::Contour::Builder builder)
{
  auto p = begin;
  tl_assert (n > 0);

  db::Point pl = *p;
  builder.getP1 ().setX (pl.x ());
  builder.getP1 ().setY (pl.y ());
  ++p;

  builder.initDeltas (n - 1);
  size_t index = 0;
  for ( ; p != end; ++p, ++index) {
    auto d = builder.getDeltas ()[index];
    auto pd = *p - pl;
    d.setDx (pd.x ());
    d.setDy (pd.y ());
    pl = *p;
  }
}

/**
 *  @brief "make_object" overload for db::SimplePolygon and stream::layoutView::SimplePolygon
 */
void
Writer::make_object (const db::SimplePolygon &obj, stream::geometry::SimplePolygon::Builder cpnp_obj)
{
  make_contour (obj.hull ().begin (), obj.hull ().end (), obj.hull ().size (), cpnp_obj.getHull ());
}

/**
 *  @brief "make_object" overload for db::Polygon and stream::layoutView::Polygon
 */
void
Writer::make_object (const db::Polygon &obj, stream::geometry::Polygon::Builder cpnp_obj)
{
  make_contour (obj.hull ().begin (), obj.hull ().end (), obj.hull ().size (), cpnp_obj.getHull ());

  cpnp_obj.initHoles (obj.holes ());
  for (unsigned int h = 0; h < obj.holes (); ++h) {
    make_contour (obj.hole (h).begin (), obj.hole (h).end (), obj.hole (h).size (), cpnp_obj.getHoles ()[h]);
  }
}

/**
 *  @brief "make_object" overload for db::Edge and stream::layoutView::Edge
 */
void
Writer::make_object (const db::Edge &obj, stream::geometry::Edge::Builder cpnp_obj)
{
  auto p1 = cpnp_obj.getP1 ();
  p1.setX (obj.p1 ().x ());
  p1.setY (obj.p1 ().y ());

  auto delta = cpnp_obj.getDelta ();
  delta.setDx (obj.d ().x ());
  delta.setDy (obj.d ().y ());
}

/**
 *  @brief "make_object" overload for db::EdgePair and stream::layoutView::EdgePair
 */
void
Writer::make_object (const db::EdgePair &obj, stream::geometry::EdgePair::Builder cpnp_obj)
{
  make_object (obj.first (), cpnp_obj.getE1 ());
  make_object (obj.second (), cpnp_obj.getE2 ());
}

/**
 *  @brief "make_object" overload for db::Box and stream::layoutView::Box
 */
void
Writer::make_object (const db::Box &obj, stream::geometry::Box::Builder cpnp_obj)
{
  auto p1 = cpnp_obj.getP1 ();
  p1.setX (obj.p1 ().x ());
  p1.setY (obj.p1 ().y ());

  auto delta = cpnp_obj.getDelta ();
  auto d = obj.p2 () - obj.p1 ();
  delta.setDx (d.x ());
  delta.setDy (d.y ());
}

/**
 *  @brief Converts KLayout's fixpoint transformation code into a stream::geometry::FixPointTransformation enum
 */
stream::geometry::FixPointTransformation
Writer::make_fixpoint_transformation (const db::Trans &trans)
{
  switch (trans.fp_trans ().rot ()) {
  case db::FTrans::r0:
  default:
    return stream::geometry::FixPointTransformation::R0;
  case db::FTrans::r90:
    return stream::geometry::FixPointTransformation::R90;
  case db::FTrans::r180:
    return stream::geometry::FixPointTransformation::R180;
  case db::FTrans::r270:
    return stream::geometry::FixPointTransformation::R270;
  case db::FTrans::m0:
    return stream::geometry::FixPointTransformation::M0;
  case db::FTrans::m45:
    return stream::geometry::FixPointTransformation::M45;
  case db::FTrans::m90:
    return stream::geometry::FixPointTransformation::M90;
  case db::FTrans::m135:
    return stream::geometry::FixPointTransformation::M135;
  }
}

/**
 *  @brief "make_object" overload for db::Text and stream::layoutView::Text
 */
void
Writer::make_object (const db::Text &obj, stream::geometry::Label::Builder cpnp_obj)
{
  db::Point pos = db::Point () + obj.trans ().disp ();
  cpnp_obj.getPosition ().setX (pos.x ());
  cpnp_obj.getPosition ().setY (pos.y ());
  cpnp_obj.setOrientation (make_fixpoint_transformation (obj.trans ()));

  cpnp_obj.setStringId (get_text_string_id (obj.string ()));

  cpnp_obj.setSize (obj.size ());

  switch (obj.halign ()) {
  case db::HAlignCenter:
    cpnp_obj.setHorizontalAlign (stream::geometry::Label::HAlignment::CENTER);
    break;
  case db::HAlignRight:
    cpnp_obj.setHorizontalAlign (stream::geometry::Label::HAlignment::RIGHT);
    break;
  case db::HAlignLeft:
  default:
    cpnp_obj.setHorizontalAlign (stream::geometry::Label::HAlignment::LEFT);
  }

  switch (obj.valign ()) {
  case db::VAlignCenter:
    cpnp_obj.setVerticalAlign (stream::geometry::Label::VAlignment::CENTER);
    break;
  case db::VAlignTop:
    cpnp_obj.setVerticalAlign (stream::geometry::Label::VAlignment::TOP);
    break;
  case db::VAlignBottom:
  default:
    cpnp_obj.setVerticalAlign (stream::geometry::Label::VAlignment::BOTTOM);
  }
}

/**
 *  @brief "make_object" overload for db::Point and stream::layoutView::Point
 */
void
Writer::make_object (const db::Point &obj, stream::geometry::Point::Builder cpnp_obj)
{
  cpnp_obj.setX (obj.x ());
  cpnp_obj.setY (obj.y ());
}

/**
 *  @brief "make_object" overload for db::Path and stream::layoutView::Path
 */
void
Writer::make_object (const db::Path &obj, stream::geometry::Path::Builder cpnp_obj)
{
  make_contour (obj.begin (), obj.end (), obj.points (), cpnp_obj.getSpine ());
  if ((obj.width () / 2) * 2 != obj.width ()) {
    warn (tl::to_string (tr ("Rounding width to even DBU value in path: ")) + obj.to_string ());
  }
  cpnp_obj.setHalfWidth (obj.width () / 2);

  if (obj.round ()) {
    if (obj.bgn_ext () != obj.end_ext () || obj.bgn_ext () * 2 != obj.width ()) {
      warn (tl::to_string (tr ("Changing elliptic-end path to circular ends: ")) + obj.to_string ());
    }
    cpnp_obj.setExtensionType (stream::geometry::Path::ExtensionType::ROUND);
  } else if (obj.bgn_ext () * 2 == obj.width () && obj.bgn_ext () == obj.end_ext ()) {
    cpnp_obj.setExtensionType (stream::geometry::Path::ExtensionType::SQUARE);
  } else if (obj.bgn_ext () == 0 && obj.end_ext () == 0) {
    cpnp_obj.setExtensionType (stream::geometry::Path::ExtensionType::FLUSH);
  } else {
    cpnp_obj.setExtensionType (stream::geometry::Path::ExtensionType::VARIABLE);
    cpnp_obj.setBeginExtension (obj.bgn_ext ());
    cpnp_obj.setEndExtension (obj.end_ext ());
  }
}

/**
 *  @brief "make_object" overload for db::CellInstArray and stream::layoutView::CellInstance
 */
void
Writer::make_object (const db::CellInstArray &obj, stream::layoutView::CellInstance::Builder cpnp_obj)
{
  //  NOTE: the "CellInstArray" will actually be a single instance always
  tl_assert (obj.size () == 1);

  cpnp_obj.setCellId (get_cell_id (obj.object ().cell_index ()));

  auto transformation = cpnp_obj.getTransformation ();

  db::Point pos = db::Point () + obj.front ().disp ();
  transformation.getDisplacement ().setDx (pos.x ());
  transformation.getDisplacement ().setDy (pos.y ());

  if (! obj.is_complex ()) {
    auto simple = transformation.getTransformation ().initSimple ();
    simple.setOrientation (make_fixpoint_transformation (obj.front ()));
  } else {
    db::ICplxTrans trans = obj.complex_trans ();
    auto complex = transformation.getTransformation ().initComplex ();
    complex.setScale (trans.mag ());
    complex.setAngle (trans.angle ());
    complex.setMirror (trans.is_mirror ());
  }
}

/**
 *  @brief Writes the given compressed container to the container builder
 * 
 *  "Object" is an object that is supported by the compression scheme
 *  (those are db::CellInstArray and the geometrical primitives such as db::Box etc.).
 * 
 *  "Builder" is the Builder object of a "ObjectContainerForType" generic struct.
 * 
 *  This method will use the "make_object" overloads to actually create the objects.
 *  The "compressed_container" will contain various variants involved plain and 
 *  arrayed objects, optionally combined with properties. 
 * 
 *  These schemes are placed in the corresponding slots of the "ObjectContainerForType"
 *  struct.
 * 
 *  This method is called after the compressed objects have been computed.
 *  This also involves generating repetition Ids which are already available when
 *  this method is called.
 */
template <class Object, class Builder>
void
Writer::make_objects (const Compressed::compressed_container<Object> &container, Builder builder)
{
  size_t i;

  builder.initBasic (container.plain.size ());
  i = 0;
  for (auto s = container.plain.begin (); s != container.plain.end (); ++s, ++i) {
    make_object (*s, builder.getBasic ()[i].getBasic ());
  }

  builder.initArrays (container.array.size ());
  i = 0;
  for (auto s = container.array.begin (); s != container.array.end (); ++s, ++i) {
    auto a = builder.getArrays ()[i];
    make_object (s->first, a.getBasic ());
    a.setRepetitionId (s->second);
  }

  builder.initWithProperties (container.with_properties.size ());
  i = 0;
  for (auto s = container.with_properties.begin (); s != container.with_properties.end (); ++s, ++i) {
    auto a = builder.getWithProperties ()[i];
    make_object (s->first, a.getBasic ());
    a.setPropertySetId (get_property_id (s->second));
  }

  builder.initArraysWithProperties (container.array_with_properties.size ());
  i = 0;
  for (auto s = container.array_with_properties.begin (); s != container.array_with_properties.end (); ++s, ++i) {
    auto a = builder.getArraysWithProperties ()[i];
    auto ab = a.getBasic ();
    make_object (s->first.first, ab.getBasic ());
    ab.setRepetitionId (s->second);
    a.setPropertySetId (get_property_id (s->first.second));
  }
}

} // namespace db


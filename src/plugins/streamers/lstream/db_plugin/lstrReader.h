
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

#ifndef HDR_lstrReader
#define HDR_lstrReader

#include "dbPluginCommon.h"
#include "dbLayout.h"
#include "dbCommonReader.h"
#include "dbStreamLayers.h"

#include "library.capnp.h"
#include "repetition.capnp.h"
#include "layoutView.capnp.h"

#include "tlException.h"
#include "tlProgress.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlString.h"
#include "tlStream.h"

#include <kj/io.h>

namespace kj
{
  class BufferedInputStream;
}

namespace lstr
{

/**
 *  @brief A reimplementation of the kj::InputStream interface to provide KLayout streams for Cap'n'Proto
 * 
 *  Note: this implementation is not based on the buffered streams of KLayout
 *  which are not compatible with kj::BufferedInputStreamWrapper as of now.
 *  Instead we use the unterlying basic stream of KLayout which is pretty 
 *  much compatible with kj.
 */
class InputStream 
  : public kj::InputStream
{
public:
  InputStream (tl::InputStream *is)
    : mp_is (is), m_pos (is->pos ()), m_pos_before (is->pos ())
  { 
    //  .. nothing yet ..
  }

  virtual size_t tryRead (void *buffer, size_t /*min_bytes*/, size_t max_bytes)
  {
    size_t n = mp_is->base ()->read ((char *) buffer, max_bytes);
    m_pos_before = m_pos;
    m_pos += n;
    return n;
  }

  /*  TODO: we don't have "skip" on the delegate right now.
  virtual void skip (size_t bytes)
  {
    mp_is->base ()->skip (bytes);
  }
  */

  /**
   *  @brief Resets the basic stream, so we can restart
   */
  void reset ()
  {
    mp_is->base ()->reset ();
    m_pos_before = m_pos = 0;
  }

  /**
   *  @brief Gets the position in the stream after the current chunk
   */
  size_t position ()
  {
    return m_pos;
  }

  /**
   *  @brief Gets the position in the stream before the current chunk
   */
  size_t position_before ()
  {
    return m_pos_before;
  }

private:
  tl::InputStream *mp_is;
  size_t m_pos, m_pos_before;
};

/**
 *  @brief Generic base class of LStream reader exceptions
 */
class DB_PLUGIN_PUBLIC LStreamReaderException
  : public db::ReaderException 
{
public:
  LStreamReaderException (const std::string &msg, const std::string &cell, const std::string &source, const std::string &pos)
    : db::ReaderException (
        cell.empty () ? 
          tl::sprintf (tl::to_string (tr ("%s, in file: %s (position %s)")), msg, source, pos) :
          tl::sprintf (tl::to_string (tr ("%s (cell=%s), in file: %s (position %s)")), msg, cell, source, pos)
      )
  { }
};

/**
 *  @brief The LStream format stream reader
 */
class DB_PLUGIN_PUBLIC Reader
  : public db::CommonReader
{
public: 
  /**
   *  @brief Construct a stream reader object
   *
   *  @param s The stream delegate from which to read stream data from
   */
  Reader (tl::InputStream &s);

  /**  
   *  @brief Destructor
   */
  ~Reader () noexcept;

  /**
   *  @brief Format
   */
  virtual const char *format () const { return "LStream"; }

protected:
  /**
   *  @brief Implementation of the db::CommonReader interface
   * 
   *  This method will read the information from the stream
   *  passed in the constructor.
   */
  virtual void do_read (db::Layout &layout);

  /**
   *  @brief Implementation of db::CommonReader InputStream
   * 
   *  This method is called to initialize the reader 
   *  from the given options.
   */
  virtual void init (const db::LoadLayoutOptions &options);

  /**
   *  @brief Issues an errors
   */
  void error (const std::string &msg);

  /**
   *  @brief Issues a warning
   * 
   *  The warning level indicates the severity.
   *  A higher value indicates lower severity.
   */
  void warn (const std::string &msg, int warn_level = 1);

  /**
   *  @brief Accessor method to the current cellname
   */
  const std::string &cellname () const { return m_cellname; }

private:
  lstr::InputStream m_stream;
  std::string m_source;
  std::string m_bbox_meta_data_key;
  tl::AbsoluteProgress m_progress;
  size_t m_library_index;
  std::string m_cellname;
  std::string m_libname;
  db::Cell *mp_cell;
  db::Layout *mp_layout;
  std::map<uint64_t, unsigned int> m_layer_id_map;
  std::map<uint64_t, std::string> m_library_names_by_id;
  std::map<uint64_t, db::property_names_id_type> m_property_name_id_map;
  std::map<uint64_t, db::properties_id_type> m_properties_id_map;
  std::map<uint64_t, const db::StringRef *> m_text_strings_by_id;
  uint64_t m_layout_view_id;
  uint64_t m_meta_data_view_id;
  std::vector<std::pair<db::cell_index_type, std::string> > m_cells;

  void yield_progress ();
  std::string position ();
  void do_read_internal (db::Layout &layout);
  void read_header (kj::BufferedInputStream &is);
  void read_library (kj::BufferedInputStream &is);
  void skip_library (kj::BufferedInputStream &is);
  void read_cell (db::cell_index_type cell_index, kj::BufferedInputStream &is);
  void make_single_cell_instance (db::cell_index_type of_cell, db::properties_id_type prop_id, const db::ICplxTrans &ct);
  void make_cell_instance (db::cell_index_type of_cell, db::properties_id_type prop_id, stream::repetition::Repetition::Reader repetition, const db::ICplxTrans &ct);
  void read_instances (stream::layoutView::LayoutView::Reader layout_view);
  template <class Object, class CPObject>
  void read_shapes (unsigned int li, typename stream::layoutView::ObjectContainerForType<CPObject>::Reader reader, capnp::List<stream::repetition::Repetition, capnp::Kind::STRUCT>::Reader repetitions);
  void read_layer (stream::layoutView::Layer::Reader reader);
  void read_layout_view (db::cell_index_type cell_index, kj::BufferedInputStream &is);
  void read_meta_data_view (db::cell_index_type cell_index, kj::BufferedInputStream &is);
  void read_layers (stream::library::ViewSpec::Reader view_specs);
  tl::Variant make_variant (stream::variant::Variant::Value::Reader variant);
  void make_meta_data(const db::Cell *cell, stream::metaData::MetaData::Reader property_set);
  std::map<std::string, tl::Variant> make_pcell_parameters (stream::library::CellParameters::Reader cell_parameters);
  void read_cells(stream::library::Library::Reader header);
  void read_library_refs (stream::library::Library::Reader header);
  void read_properties (stream::library::Library::Reader header);
  void read_text_strings (stream::library::Library::Reader header);
  unsigned int get_layer_by_id (uint64_t id) const;
  std::string get_library_name_by_id (uint64_t id) const;
  db::property_names_id_type get_property_name_id_by_id (uint64_t id) const;
  db::properties_id_type get_properties_id_by_id (uint64_t id) const;
  const db::StringRef *get_string_by_id (uint64_t id) const;
  db::Vector make_object (stream::geometry::Vector::Reader reader);
  db::Point make_object (stream::geometry::Point::Reader reader);
  db::Box make_object (stream::geometry::Box::Reader reader);
  db::Edge make_object (stream::geometry::Edge::Reader reader);
  db::EdgePair make_object (stream::geometry::EdgePair::Reader reader);
  db::SimplePolygonRef make_object (stream::geometry::SimplePolygon::Reader reader);
  db::PolygonRef make_object (stream::geometry::Polygon::Reader reader);
  db::PathRef make_object (stream::geometry::Path::Reader reader);
  db::Text make_object (stream::geometry::Label::Reader reader);
  void make_object_array (unsigned int li, db::properties_id_type prop_id, const db::PolygonRef &object, stream::repetition::Repetition::Reader repetition);
  void make_object_array (unsigned int li, db::properties_id_type prop_id, const db::SimplePolygonRef &object, stream::repetition::Repetition::Reader repetition);
  void make_object_array (unsigned int li, db::properties_id_type prop_id, const db::PathRef &object, stream::repetition::Repetition::Reader repetition);
  void make_object_array (unsigned int li, db::properties_id_type prop_id, const db::EdgePair &object, stream::repetition::Repetition::Reader repetition);
  void make_object_array (unsigned int li, db::properties_id_type prop_id, const db::Edge &object, stream::repetition::Repetition::Reader repetition);
  void make_object_array (unsigned int li, db::properties_id_type prop_id, const db::Point &object, stream::repetition::Repetition::Reader repetition);
  void make_object_array (unsigned int li, db::properties_id_type prop_id, const db::Text &object, stream::repetition::Repetition::Reader repetition);
  void make_object_array (unsigned int li, db::properties_id_type prop_id, const db::Box &object, stream::repetition::Repetition::Reader repetition);
  template <class Object>
  void make_object_array_ref (unsigned int li, db::properties_id_type prop_id, const Object &object, stream::repetition::Repetition::Reader repetition);
  template <class Object, class ObjectPtr>
  void make_object_array_ptr (unsigned int li, db::properties_id_type prop_id, const Object &object, stream::repetition::Repetition::Reader repetition);
  template <class Object>
  void make_object_array_explode (unsigned int li, db::properties_id_type prop_id, const Object &object, stream::repetition::Repetition::Reader repetition);

  virtual void common_reader_error (const std::string &msg) { error (msg); }
  virtual void common_reader_warn (const std::string &msg, int warn_level = 1) { warn (msg, warn_level); }
};

}

#endif


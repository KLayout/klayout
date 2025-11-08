
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

#include "lstrReader.h"
#include "lstrPlugin.h"
#include "lstrFormat.h"

#include "dbArray.h"

#include "tlException.h"
#include "tlString.h"
#include "tlInternational.h"
#include "tlClassRegistry.h"

#include "header.capnp.h"
#include "library.capnp.h"
#include "cell.capnp.h"
#include "layoutView.capnp.h"
#include "metaDataView.capnp.h"

#include <capnp/serialize-packed.h>

namespace lstr
{

// ---------------------------------------------------------------
//  Some utility functions for the reader

class CoordinateOverflowException
  : public tl::Exception
{
public:
  CoordinateOverflowException (int64_t c)
    : tl::Exception (tl::to_string (tr ("Coordinate overflow for value: ")) + tl::to_string (c))
  {
    //  .. nothing yet ..
  }
};

inline db::Coord cast_to_coord (int64_t c)
{
  if (c > std::numeric_limits<db::Coord>::max () || c < std::numeric_limits<db::Coord>::min ()) {
    throw CoordinateOverflowException (c);
  }
  return db::Coord (c);
}

/**
 *  @brief Converts a stream::geometry::Vector object to a db::Vector
 */
static db::Vector
make_vector (stream::geometry::Vector::Reader reader)
{
  return db::Vector (cast_to_coord (reader.getDx ()), cast_to_coord (reader.getDy ()));
}

/**
 *  @brief Adds a vector to a point in a overflow-safe way
 */
static db::Point
add_vector (const db::Point &p, stream::geometry::Vector::Reader reader)
{
  return db::Point (cast_to_coord (p.x () + reader.getDx ()), cast_to_coord (p.x () + reader.getDy ()));
}

/**
 *  @brief Converts a stream::geometry::Point object to a db::Point
 */
static db::Point 
make_point (stream::geometry::Point::Reader reader)
{
  return db::Point (cast_to_coord (reader.getX ()), cast_to_coord (reader.getY ()));
}

/**
 *  @brief Converts a stream::geometry::FixPointTransformation enum into a KLayout fixpoint transformation constant
 */
static unsigned int
make_fixpoint_trans (stream::geometry::FixPointTransformation fp)
{
  switch (fp) {
  case stream::geometry::FixPointTransformation::R0:
    return db::FTrans::r0;
  case stream::geometry::FixPointTransformation::R90:
    return db::FTrans::r90;
  case stream::geometry::FixPointTransformation::R180:
    return db::FTrans::r180;
  case stream::geometry::FixPointTransformation::R270:
    return db::FTrans::r270;
  case stream::geometry::FixPointTransformation::M0:
    return db::FTrans::m0;
  case stream::geometry::FixPointTransformation::M45:
    return db::FTrans::m45;
  case stream::geometry::FixPointTransformation::M90:
    return db::FTrans::m90;
  case stream::geometry::FixPointTransformation::M135:
    return db::FTrans::m135;
  default:
    return db::FTrans::r0;
  }
}

/**
 *  @brief Converts a stream::library::LayerEntry::Purpose enum value into a string
 * 
 *  This method is used to derive layer names. The default purpose is not
 *  converted to a string an left empty.
 */
static std::string
purpose_string (stream::library::LayerEntry::Purpose purpose, stream::library::LayerEntry::Purpose default_purpose = stream::library::LayerEntry::Purpose::DRAWING)
{
  std::string ps;

  if (purpose == default_purpose) {
    return ps;
  }

  switch (purpose) {
  case stream::library::LayerEntry::Purpose::DRAWING:
    ps = "DRAWING";
    break;
  case stream::library::LayerEntry::Purpose::BLOCKAGE:
    ps = "BLOCKAGE";
    break;
  case stream::library::LayerEntry::Purpose::BOUNDARY:
    ps = "BOUNDARY";
    break;
  case stream::library::LayerEntry::Purpose::COMMENT:
    ps = "COMMENT";
    break;
  case stream::library::LayerEntry::Purpose::ERRORS:
    ps = "ERRORS";
    break;
  case stream::library::LayerEntry::Purpose::FILL:
    ps = "FILL";
    break;
  case stream::library::LayerEntry::Purpose::HANDLES:
    ps = "HANDLES";
    break;
  case stream::library::LayerEntry::Purpose::PIN:
    ps = "PIN";
    break;
  case stream::library::LayerEntry::Purpose::SLOT:
    ps = "SLOT";
    break;
  case stream::library::LayerEntry::Purpose::TEXT:
    ps = "TEXT";
    break;
  case stream::library::LayerEntry::Purpose::WIRE:
    ps = "WIRE";
    break;
  default:
    break;
  }

  return ps;
}

/**
 *  @brief Turns a stream::geometry::Contour contour into a list of _PSTL_PRAGMA_SIMD_ORDERED_MONOTONIC_2ARGS
 * 
 *  A contour is a list of points, representing a closed loop (for polygons) or
 *  a linear chain of line segments (for paths).get_regular_array
 *  
 *  This function will extract the list of points from the Contour object.get_regular_array
 * 
 *  @param contour The list of points (output)
 *  @param reader The stream::geometry::Contour object (input)
 */
static void
make_contour (std::vector<db::Point> &contour, stream::geometry::Contour::Reader reader)
{
  contour.clear ();
  contour.reserve (reader.getDeltas ().size () + 1);

  db::Point pt = make_point (reader.getP1 ());
  auto deltas = reader.getDeltas ();
  for (auto d = deltas.begin (); d != deltas.end (); ++d) {
    contour.push_back (pt);
    pt += make_vector (*d);
  }
  contour.push_back (pt);
}

/**
 *  @brief Generates a ICplxTrans transformation from a stream::layoutView::CellTransformation object
 * 
 *  ICplxTrans is the generic (complex) transformation used inside KLayout to
 *  represent affine transformations. These including isotropic scaling,
 *  arbitrary angle rotations, mirroring and displacement.
 */
static db::ICplxTrans 
make_transformation (stream::layoutView::CellTransformation::Reader transformation)
{
  db::Vector d = make_vector (transformation.getDisplacement ());

  if (transformation.getTransformation ().isComplex ()) {

    auto complex = transformation.getTransformation ().getComplex ();
    double angle = complex.getAngle ();
    bool mirror = complex.getMirror ();
    double mag = complex.getScale ();

    return db::ICplxTrans (mag, angle, mirror, d);

  } else if (transformation.getTransformation ().isSimple ()) {

    auto simple = transformation.getTransformation ().getSimple ();
    return db::ICplxTrans (db::Trans (make_fixpoint_trans (simple.getOrientation ()), d));

  } else {
    return db::ICplxTrans ();
  }
}

/**
 *  @brief Extracts a list of displacements from a "ENUMERATED" type stream::repetition::Repetition
 * 
 *  @param repetition The Cap'n'Proto repetition type (input)
 *  @param vectors The displacements derived from the repetition (output)
 *  
 *  The repetition is expected to be of ENUMERATED type.
 *  The first element of the output list of displacements is a zero
 *  vector which is implicitly included in the enumerated repetition.
 */
static void
make_vectors (stream::repetition::Repetition::Reader repetition, std::vector<db::Vector> &vectors)
{
  tl_assert (repetition.getTypes ().isEnumerated ());

  auto deltas = repetition.getTypes ().getEnumerated ().getDeltas ();

  vectors.clear ();
  vectors.reserve (deltas.size () + 1);
  vectors.push_back (db::Vector ());

  db::Vector dl;
  for (auto d = deltas.begin (); d != deltas.end (); ++d) {
    dl += make_vector (*d);
    vectors.push_back (dl);
  }
}

/**
 *  @brief Turns a stream::repetition::Repetition into a db::iterated_array<db::Coord> object
 * 
 *  The Repetition object is expected to be of ENUMERATED type. The list of
 *  displacements is turned into a irregular array for use in shape or instance
 *  arrays.
 * 
 *  Note that the output array will contain one element in addition. This
 *  is the first element which represents the original object without displacement.
 * 
 *  @param repetition The Repetition object delivered by Cap'n'Proto (input)
 *  @param array The KLayout irregular (enumerated) array object (output)
 *  
 */
static void
make_iterated_array (stream::repetition::Repetition::Reader repetition, db::iterated_array<db::Coord> &array)
{
  tl_assert (repetition.getTypes ().isEnumerated ());

  auto deltas = repetition.getTypes ().getEnumerated ().getDeltas ();
  array.reserve (deltas.size () + 1);
  array.insert (db::Vector ());
  db::Vector dl;
  for (auto d = deltas.begin (); d != deltas.end (); ++d) {
    dl += make_vector (*d);
    array.insert (dl);
  }
  array.sort ();
}

/**
 *  @brief Extracts the regular array parameters (a, b, na, nb) from stream::repetition::Repetition
 *  
 *  The Repetition object is expected to represent a REGULAR or REGULAR_ORTH
 *  repetition. The returned values are the array axes (a, b) and dimensions
 *  (na, nb).
 * 
 *  @param repetition The Repetition object delivered by Cap'n'Proto (input)
 *  @param a The a axis (output)
 *  @param b The b axis (output)
 *  @param na The a dimension (output)
 *  @param nb The b dimension (output)
 */
static void
get_regular_array (stream::repetition::Repetition::Reader repetition, db::Vector &a, db::Vector &b, unsigned long &na, unsigned long &nb)
{
  tl_assert (repetition.getTypes ().isRegularOrtho () || repetition.getTypes ().isRegular ());

  if (repetition.getTypes ().which () == stream::repetition::Repetition::Types::REGULAR) {

    auto regular = repetition.getTypes ().getRegular ();

    a = make_vector (regular.getA ());
    b = make_vector (regular.getB ());
    na = regular.getNa ();
    nb = regular.getNb ();

  } else {

    auto regular_ortho = repetition.getTypes ().getRegularOrtho ();

    a = db::Vector (regular_ortho.getDx (), 0);
    b = db::Vector (0, regular_ortho.getDy ());
    na = regular_ortho.getNx ();
    nb = regular_ortho.getNy ();

  }
}

/**
 *  @brief Turns a stream::repetition::Repetition into a db::regular_array<db::Coord>get_regular_array
 *  
 *  The latter is the basic object to represent a regular array in KLayout's
 *  shape and instance arrays.
 * 
 *  @param repetition The Repetition object delivered by Cap'n'Proto (input)
 *  @param array The KLayout regular array object (output)
 */
static void
make_regular_array (stream::repetition::Repetition::Reader repetition, db::regular_array<db::Coord> &array)
{
  db::Vector a, b;
  unsigned long na = 0, nb = 0;
  get_regular_array (repetition, a, b, na, nb);

  array = db::regular_array<db::Coord> (a, b, na, nb);
}

// ---------------------------------------------------------------
//  LStreamReader implementation

Reader::Reader (tl::InputStream &s)
  : m_stream (&s), m_source (s.source ()),
    m_progress (tl::to_string (tr ("Reading LStream file"))),
    m_library_index (0), mp_cell (0), mp_layout (0), m_layout_view_id (0)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
}

Reader::~Reader () noexcept
{
  //  .. nothing yet ..
}

void
Reader::init (const db::LoadLayoutOptions &options)
{
  db::CommonReader::init (options);

  lstr::ReaderOptions lstr_options = options.get_options<lstr::ReaderOptions> ();
  m_bbox_meta_data_key = lstr_options.bbox_meta_info_key;
}

/**
 *  @brief Gets a string describing the position in the file
 */
std::string
Reader::position ()
{
  if (m_stream.position () == m_stream.position_before ()) {
    return tl::to_string (m_stream.position ());
  } else {
    return tl::to_string (m_stream.position_before ()) + " .. " + tl::to_string (m_stream.position ());
  }
}

void 
Reader::error (const std::string &msg)
{
  throw LStreamReaderException (msg, cellname ().c_str (), m_source, position ());
}

void 
Reader::warn (const std::string &msg, int wl)
{
  if (warn_level () < wl) {
    return;
  }

  if (first_warning ()) {
    tl::warn << tl::sprintf (tl::to_string (tr ("In file %s:")), m_source);
  }

  int ws = compress_warning (msg);
  if (ws < 0) {
    tl::warn << msg
             << tl::to_string (tr (" (cell=")) << cellname ().c_str ()
             << ")";
  } else if (ws == 0) {
    tl::warn << tl::to_string (tr ("... further warnings of this kind are not shown"));
  }
}

//  See declaration
void 
Reader::do_read (db::Layout &layout)
{
  try {
    do_read_internal (layout);
  } catch (lstr::CoordinateOverflowException &ex) {
    //  this adds source information
    error (ex.msg ());
  } catch (kj::Exception &ex) {
    //  this adds source information
    error (ex.getDescription ().cStr ());
  }
}

//  do_read delegate, unprotected
void 
Reader::do_read_internal (db::Layout &layout)
{
  mp_layout = &layout;
  m_cellname.clear ();

  //  NOTE: we bypass buffering from the InputStream as KJ already buffers

  //  TODO: this prevents using HTTP as of now (version 0.30.2). Maybe we can implement
  //  "reset" in the HTTP streams in KLayout later.
  m_stream.reset ();

  size_t nhdr = strlen (LStream_sig) + 1;
  std::unique_ptr<char[]> hdr (new char [nhdr]);
  size_t nhdr_read = m_stream.tryRead (hdr.get (), nhdr, nhdr);
  if (nhdr_read != nhdr || memcmp (LStream_sig, hdr.get (), nhdr) != 0) {
    error (tl::to_string (tr ("LStream format not recognized (missing magic bytes)")));
  }
  
  kj::BufferedInputStreamWrapper kj_stream (m_stream);

  //  Reads the global header
  read_header (kj_stream);

  //  Skip libraries we're not interested in
  while (m_library_index > 0) {
    skip_library (kj_stream);
    --m_library_index;
  }

  read_library (kj_stream);

  //  Read the cell messages after the library in the order the cells were defined in 
  //  stream::library::CellSpecsTable.
  for (auto c = m_cells.begin (); c != m_cells.end (); ++c) {
    m_cellname = c->second;
    read_cell (c->first, kj_stream);
    m_cellname.clear ();
  }
}

/**
 *  @brief A helper class to join two datatype layer name map members
 *  TODO: should not be required with the proper extensions to the CommonReader base
 */
struct LNameJoinOp1
{
  void operator() (std::string &a, const std::string &b)
  {
    db::join_layer_names (a, b);
  }
};

/**
 *  @brief A helper class to join two layer map members
 *  This implementation basically merged the datatype maps.
 *  TODO: should not be required with the proper extensions to the CommonReader base
 */
struct LNameJoinOp2
{
  void operator() (tl::interval_map<db::ld_type, std::string> &a, const tl::interval_map<db::ld_type, std::string> &b)
  {
    LNameJoinOp1 op1;
    a.add (b.begin (), b.end (), op1);
  }
};

void 
Reader::read_layers (stream::library::ViewSpec::Reader view_specs)
{
  size_t index = 0;
  auto layer_entries = view_specs.getLayerTable ().getLayerEntries ();
  for (auto l = layer_entries.begin (); l != layer_entries.end (); ++l, ++index) {

    db::LayerProperties lp;
    auto ln = l->getLayerNumbers ();
    if (ln.size () == 1) {
      lp = db::LayerProperties (ln[0], 0);
    } else if (ln.size () >= 2) {
      lp = db::LayerProperties (ln[0], ln[1]);
    }

    lp.name = l->getName ();

    auto ps = purpose_string (l->getPurpose ());
    if (! ps.empty ()) {
      //  In case of a non-DRAWING Purpose, generate a named layer adding the purpose to 
      //  the layer string
      lp = db::LayerProperties (lp.to_string () + "." + ps);
    }

    //  TODO: Can't handle purely named layers in the current CommonReader implementation -> extend CommonReader to accept named layers
    if (! lp.is_named ()) {

      if (! lp.name.empty ()) {

        //  add name to the layer name map
        //  TODO: should be easier with a better API
        tl::interval_map <db::ld_type, std::string> dt_map;
        LNameJoinOp1 op1;
        dt_map.add (lp.datatype, lp.datatype + 1, lp.name, op1);
        LNameJoinOp2 op2;
        layer_names ().add (lp.layer, lp.layer + 1, dt_map, op2);

      }

      auto li = open_dl (*mp_layout, db::LDPair (lp.layer, lp.datatype));
      if (li.first) {
        m_layer_id_map.insert (std::make_pair (index, li.second));
      }

    } else {
      warn (tl::sprintf (tl::to_string (tr ("Purely named layers (here: '%s') cannot be read currently")), lp.name));
    }

  }
}

/**
 *  @brief Creates a KLayout variant from a stream::variant::Variant
 * 
 *  As a speciality, the "OBJECT" type allows using KLayout's
 *  string convention to represent KLayout objects. This allows serialization
 *  of certain types such as boxes or polygons, but basically bears the risk
 *  of incompatibilities.
 */
tl::Variant
Reader::make_variant (stream::variant::Variant::Value::Reader variant)
{
  switch (variant.which ()) {
  case stream::variant::Variant::Value::NIL:
    return tl::Variant ();
  case stream::variant::Variant::Value::BOOL:
    return tl::Variant (variant.getBool ());
  case stream::variant::Variant::Value::DOUBLE:
    return tl::Variant (variant.getDouble ());
  case stream::variant::Variant::Value::UINT64:
    return tl::Variant (variant.getUint64 ());
  case stream::variant::Variant::Value::INT64:
    return tl::Variant (variant.getInt64 ());
  case stream::variant::Variant::Value::LIST:
    {
      tl::Variant var_list = tl::Variant::empty_list ();
      auto list = variant.getList ();
      for (auto l = list.begin (); l != list.end (); ++l) {
        var_list.push (make_variant (l->getValue ()));
      }
      return var_list;
    }
  case stream::variant::Variant::Value::ARRAY:
    {
      tl::Variant var_array = tl::Variant::empty_array ();
      auto array = variant.getArray ();
      for (auto l = array.begin (); l != array.end (); ++l) {
        var_array.insert (make_variant (l->getKey ().getValue ()), make_variant (l->getValue ().getValue ()));
      }
      return var_array;
    }
  case stream::variant::Variant::Value::OBJECT:
    {
      std::string str = variant.getObject ();
      tl::Extractor ex (str.c_str ());
      tl::Variant var;
      if (ex.test ("klayout") && ex.test (":")) {
        try {
          ex.read (var);
        } catch (tl::Exception &ex) {
          warn (tl::sprintf (tl::to_string (tr ("Error extracting object string from variant ('%s'): %s")),
                str, ex.msg ()));
        }
      }
      return var;
    }
  case stream::variant::Variant::Value::TEXT:
    return tl::Variant (std::string (variant.getText ()));
  default:
    return tl::Variant ();
  }
}

/**
 *  @brief Reads meta information from the stream::propertySet::PropertySet
 * 
 *  This method will read the meta information from the given PropertySet object
 *  and attach it to the given cell (if cell != 0) or the layout (if cell == 0).
 * 
 *  Meta information is a mechanism to attach additional, rich key/value 
 *  pairs to cells or the layout. Contrary to properties, meta information
 *  does not need to follow conventions. Also properties are available down
 *  to shape and instance level, while meta information isn't.
 * 
 *  Meta information can be used for example to attach blobs to the layout.
 *  Meta information is supposed to be generated, maintained and consumed 
 *  by a particular system in it's own specific ways and to be considered
 *  opaque or optional by other systems.
 * 
 *  Properties on the other hand can be carried through other file formats
 *  such as GDS and OASIS, provided they follow some conventions (e.g. 
 *  integer keys only for GDS).
 */
void 
Reader::make_meta_data (const db::Cell *cell, stream::metaData::MetaData::Reader meta_data)
{
  auto entries = meta_data.getEntries ();
  for (auto e = entries.begin (); e != entries.end (); ++e) {

    std::string name = e->getName ();

    db::MetaInfo meta_info;
    meta_info.persisted = true;
    meta_info.value = make_variant (e->getValue ().getValue ());
    meta_info.description = e->getDescription ();

    if (cell) {
      mp_layout->add_meta_info (cell->cell_index (), name, meta_info);
    } else {
      mp_layout->add_meta_info (name, meta_info);
    }

  }
}

/**
 *  @brief Extract cell parameters from stream::library::CellParameters
 * 
 *  This method takes cell parameters and turns them into a map of 
 *  parameter names vs. values. The values are variants.
 */
std::map<std::string, tl::Variant> 
Reader::make_pcell_parameters (stream::library::CellParameters::Reader cell_parameters)
{
  std::map<std::string, tl::Variant> parameters;

  auto values = cell_parameters.getValues ();
  for (auto v = values.begin (); v != values.end (); ++v) {
    std::string name = db::property_name (get_property_name_id_by_id (v->getNameId ())).to_string ();
    auto value = make_variant (v->getValue ().getValue ());
    parameters.insert (std::make_pair (name, value));
  }

  return parameters;
}

/**
 *  @brief Creates the cells from the cell specification table
 * 
 *  This method is called inside "read_library". It will read
 *  every cell in the order defined by "library::CellSpecsTable"
 *  and create the corresponding KLayout cell.
 * 
 *  PCell and library cell resolution also happens here.
 *  Note that in order to properly restore a cell "alife",
 *  the corresponding libraries have to be installed. If 
 *  a cell is not found in the libraries, a "cold proxy"
 *  (aka "defunct cell") is created.
 */
void 
Reader::read_cells (stream::library::Library::Reader library)
{
  m_cells.clear ();

  size_t index = 0;
  auto cell_specs = library.getCellSpecsTable ().getCellSpecs ();
  for (auto l = cell_specs.begin (); l != cell_specs.end (); ++l, ++index) {

    std::string cell_name = l->getName ();

    //  Let CommonReader handle the id to cell index translation
    db::cell_index_type cell_index = make_cell (*mp_layout, index);
    m_cells.push_back (std::make_pair (cell_index, cell_name));

    rename_cell (*mp_layout, index, cell_name);

    db::Cell *cell = &mp_layout->cell (cell_index);

    std::string library_name = get_library_name_by_id (l->getLibraryRefId ());
    if (! library_name.empty ()) {

      std::string library_cell_name = l->getLibraryCellName ();
      if (library_cell_name.empty ()) {
        //  Fallback to the actual cell name if no library cell name is given
        library_cell_name = cell_name;
      }
        
      db::LayoutOrCellContextInfo context_info;

      //  NOTE: it is assumed that PCells define the "parameters" field and 
      //  non-pcells don't.
      if (l->hasParameters ()) {
        auto pcell_param = make_pcell_parameters (l->getParameters ());
        context_info.pcell_name = library_cell_name;
        context_info.pcell_parameters = pcell_param;
      } else {
        context_info.cell_name = library_cell_name;
      }

      context_info.lib_name = library_name;

      db::CommonReaderLayerMapping layer_mapping (this, mp_layout);
      mp_layout->recover_proxy_as (cell_index, context_info, &layer_mapping);

      cell = &mp_layout->cell (cell_index);

    }

    cell->prop_id (get_properties_id_by_id (l->getPropertySetId ()));

  }
}

/**
 *  @brief Reads the "library::LibraryRefs" section
 * 
 *  This method will set up the library ID to name tables.
 *  Only after executing this method you can call "get_library_name_by_id"
 *  to get the library name for a library Id.
 */
void 
Reader::read_library_refs (stream::library::Library::Reader library)
{
  auto libraries = library.getLibraryRefs ().getRefs ();
  size_t index = 1;
  for (auto l = libraries.begin (); l != libraries.end (); ++l, ++index) {
    m_library_names_by_id.insert (std::make_pair (index, l->getLibraryName ()));
  }
}

/**
 *  @brief Reads the "library::PropertyNamesTable" and "library::PropertiesTable" sections
 * 
 *  This method will set up the property tables: one for translating a
 *  property name Id into a name and one for translating the property Id
 *  into a property set (basically an alias for a dictionary of property
 *  names to value). 
 * 
 *  Only after executing this method you can call "get_property_name_id_by_id"
 *  to get the property name for a property name Id or "get_properties_id_by_id"
 *  to get the KLayout properties Id for a LStream property Id.
 */
void 
Reader::read_properties (stream::library::Library::Reader library)
{
  auto property_names = library.getPropertyNamesTable ().getNames ();

  auto property_namespaces = library.getPropertyNamesTable ().getNamespaces ();
  std::vector<std::string> ns;
  for (auto n = property_namespaces.begin (); n != property_namespaces.end (); ++n) {
    ns.push_back (*n);
  }

  for (auto n = property_names.begin (); n != property_names.end (); ++n) {

    size_t ns_id = n->getNamespaceId ();
    
    tl::Variant pn;
    if (ns_id > 0) {
      //  Account for the namespace by building a prefixed string ("namespace:name"). In other words: namespaced
      //  names should be strings.
      //  TODO: introduce a namespace concept in KLayout's property name system
      tl_assert (ns_id <= ns.size ());
      pn = ns [ns_id - 1] + ":" + make_variant (n->getName ().getValue ()).to_string ();
    } else {
      pn = make_variant (n->getName ().getValue ());
    }

    m_property_name_id_map.insert (std::make_pair (n - property_names.begin (), db::property_names_id (pn)));

  }

  auto properties = library.getPropertiesTable ().getPropertySets ();
  for (auto p = properties.begin (); p != properties.end (); ++p) {

    auto property_set = p->getProperties ();
    db::PropertiesSet ps;
    for (auto pp = property_set.begin (); pp != property_set.end (); ++pp) {
      auto name_id = get_property_name_id_by_id (pp->getNameId ());
      ps.insert (name_id, make_variant (pp->getValue ().getValue ()));
    }

    m_properties_id_map.insert (std::make_pair (p - properties.begin () + 1, db::properties_id (ps)));

  }
}

/**
 *  @brief Reads the text string table from library::TextStringsTable
 * 
 *  The text strings table associates Ids with strings. This method
 *  is called during reading the library.
 *  
 *  Only after executing this method you can use "get_string_by_id"
 *  to obtain the string reference for a text string Id.
 */
void 
Reader::read_text_strings (stream::library::Library::Reader library)
{
  auto text_strings = library.getTextStringsTable ().getTextStrings ();
  for (auto t = text_strings.begin (); t != text_strings.end (); ++t) {

    const db::StringRef *string_ref = db::StringRepository::instance ()->create_string_ref ();
    db::StringRepository::instance ()->change_string_ref (string_ref, *t);

    m_text_strings_by_id.insert (std::make_pair (uint64_t (t - text_strings.begin ()), string_ref));

  }
}

/**
 *  @brief Reads the cell message for a given cell
 * 
 *  The reader will call this method to extract cell messages from 
 *  the stream after the library was processed.
 * 
 *  This method will not only read the cell message, but also consume
 *  the following cell view messages.
 */
void
Reader::read_cell (db::cell_index_type cell_index, kj::BufferedInputStream &is)
{
  yield_progress ();
  capnp::PackedMessageReader message (is);
  stream::cell::Cell::Reader cell = message.getRoot<stream::cell::Cell> ();

  bool has_layout_view = false;

  auto views = cell.getViewIds ();
  for (auto v = views.begin (); v != views.end (); ++v) {
    if (*v == m_layout_view_id) {
      read_layout_view (cell_index, is);
      has_layout_view = true;
    } else if (*v == m_meta_data_view_id) {
      read_meta_data_view (cell_index, is);
    } else {
      //  skip other views
      yield_progress ();
      capnp::PackedMessageReader skipped_message (is);
    }
  }

  if (! has_layout_view) {
    mp_layout->cell (cell_index).set_ghost_cell (true);
  }
}

/**
 *  @brief "make_object" overloads: make a db::Vector from a stream::geometry::Vector
 */
db::Vector
Reader::make_object (stream::geometry::Vector::Reader reader)
{
  return make_vector (reader);
}

/**
 *  @brief "make_object" overloads: make a db::Point from a stream::geometry::Point
 */
db::Point
Reader::make_object (stream::geometry::Point::Reader reader)
{
  return make_point (reader);
}

/**
 *  @brief "make_object" overloads: make a db::Box from a stream::geometry::Box
 */
db::Box 
Reader::make_object (stream::geometry::Box::Reader reader)
{
  db::Point p1 = make_point (reader.getP1 ());
  db::Point p2 = p1 + make_vector (reader.getDelta ());
  if (p2.x () < p1.x ()) {
    return db::Box ();
  } else {
    return db::Box (p1, p2);
  }
}

/**
 *  @brief "make_object" overloads: make a db::Edge from a stream::geometry::Edge
 */
db::Edge
Reader::make_object (stream::geometry::Edge::Reader reader)
{
  db::Point p1 = make_point (reader.getP1 ());
  db::Point p2 = p1 + make_vector (reader.getDelta ());
  return db::Edge (p1, p2);
}

/**
 *  @brief "make_object" overloads: make a db::EdgePair from a stream::geometry::EdgePair
 */
db::EdgePair
Reader::make_object (stream::geometry::EdgePair::Reader reader)
{
  db::Edge e1 = make_object (reader.getE1 ());
  db::Edge e2 = make_object (reader.getE2 ());
  return db::EdgePair (e1, e2);
}

/**
 *  @brief "make_object" overloads: make a db::SimplePolygonRef from a stream::geometry::SimplePolygon
 * 
 *  Note: db::SimplePolygonRef is choosen over db::SimplePolygon for compactness
 *  of the database.
 */
db::SimplePolygonRef
Reader::make_object (stream::geometry::SimplePolygon::Reader reader)
{
  std::vector<db::Point> contour;
  make_contour (contour, reader.getHull ());

  db::SimplePolygon polygon;
  polygon.assign_hull (contour.begin (), contour.end (), false, false);

  return db::SimplePolygonRef (polygon, mp_layout->shape_repository ());
}

/**
 *  @brief "make_object" overloads: make a db::PolygonRef from a stream::geometry::Polygon
 * 
 *  Note: db::PolygonRef is choosen over db::Polygon for compactness
 *  of the database.
 */
db::PolygonRef
Reader::make_object (stream::geometry::Polygon::Reader reader)
{
  std::vector<db::Point> contour;
  make_contour (contour, reader.getHull ());

  db::Polygon polygon;
  polygon.assign_hull (contour.begin (), contour.end (), false, false);
  
  auto holes = reader.getHoles ();
  polygon.reserve_holes (holes.size ());
  for (auto h = holes.begin (); h != holes.end (); ++h) {
    make_contour (contour, *h);
    polygon.insert_hole (contour.begin (), contour.end (), false, false);
  }

  return db::PolygonRef (polygon, mp_layout->shape_repository ());
}

/**
 *  @brief "make_object" overloads: make a db::PathRef from a stream::geometry::Path
 * 
 *  Note: db::PathRef is choosen over db::Path for compactness
 *  of the database.
 */
db::PathRef
Reader::make_object (stream::geometry::Path::Reader reader)
{
  std::vector<db::Point> contour;
  make_contour (contour, reader.getSpine ());

  db::Coord hw = cast_to_coord (reader.getHalfWidth ());
  db::Coord bgn_ext = 0, end_ext = 0;
  bool round = false;

  if (reader.getExtensionType () == stream::geometry::Path::ExtensionType::FLUSH) {
    //  okay already
  } else if (reader.getExtensionType () == stream::geometry::Path::ExtensionType::SQUARE) {
    bgn_ext = end_ext = hw;
  } else if (reader.getExtensionType () == stream::geometry::Path::ExtensionType::ROUND) {
    bgn_ext = end_ext = hw;
    round = true;
  } else if (reader.getExtensionType () == stream::geometry::Path::ExtensionType::VARIABLE) {
    bgn_ext = cast_to_coord (reader.getBeginExtension ());
    end_ext = cast_to_coord (reader.getEndExtension ());
  }

  return db::PathRef (db::Path (contour.begin (), contour.end (), 2 * hw, bgn_ext, end_ext, round), mp_layout->shape_repository ());
}

/**
 *  @brief "make_object" overloads: make a db::Text from a stream::geometry::Text
 * 
 *  Note: the texts use StringRef string references for compactness of the database.
 */
db::Text
Reader::make_object (stream::geometry::Label::Reader reader)
{
  unsigned int orientation = make_fixpoint_trans (reader.getOrientation ());
  db::Vector pos = make_point (reader.getPosition ()) - db::Point ();
  db::Coord size = cast_to_coord (reader.getSize ());
  const db::StringRef *string = get_string_by_id (reader.getStringId ());

  db::HAlign halign = db::HAlignLeft;
  switch (reader.getHorizontalAlign ()) {
  case stream::geometry::Label::HAlignment::CENTER:
    halign = db::HAlignCenter;
    break;
  case stream::geometry::Label::HAlignment::LEFT:
    halign = db::HAlignLeft;
    break;
  case stream::geometry::Label::HAlignment::RIGHT:
    halign = db::HAlignRight;
    break;
  }

  db::VAlign valign = db::VAlignBottom;
  switch (reader.getVerticalAlign ()) {
  case stream::geometry::Label::VAlignment::CENTER:
    valign = db::VAlignCenter;
    break;
  case stream::geometry::Label::VAlignment::BOTTOM:
    valign = db::VAlignBottom;
    break;
  case stream::geometry::Label::VAlignment::TOP:
    valign = db::VAlignTop;
    break;
  }

  return db::Text (string, db::Trans (orientation, pos), size, db::Font::DefaultFont, halign, valign);
}

/**
 *  @brief Creates a single cell reference from the given cell index, property Id and transformation
 * 
 *  The new instance is inserted into the current cell.
 * 
 *  @param of_cell The cell to instantiate
 *  @param prop_id The (KLayout) property Id for the properties to attach to the cell instance
 *  @param ct The cell transformation
 */
void
Reader::make_single_cell_instance (db::cell_index_type of_cell, db::properties_id_type prop_id, const db::ICplxTrans &ct)
{
  db::CellInstArray ca;

  if (ct.is_complex ()) {
    ca = db::CellInstArray (db::CellInst (of_cell), ct, mp_layout->array_repository ());
  } else {
    ca = db::CellInstArray (db::CellInst (of_cell), db::Trans (ct));
  }

  if (prop_id == 0) {
    mp_cell->insert (ca);
  } else {
    mp_cell->insert (db::CellInstArrayWithProperties (ca, prop_id));
  }
}

/**
 *  @brief Creates an array cell reference from the given cell index, property Id, repetition and transformation
 * 
 *  The new instance is inserted into the current cell.
 * 
 *  Enumerated cell instances are resolved in non-editable mode and kept in editable mode.
 *  Regular arrays are always maintained.
 * 
 *  @param of_cell The cell to instantiate
 *  @param prop_id The (KLayout) property Id for the properties to attach to the cell instance
 *  @param repetition The LStream repetition of the cell
 *  @param ct The cell transformation
 */
void
Reader::make_cell_instance (db::cell_index_type of_cell, db::properties_id_type prop_id, stream::repetition::Repetition::Reader repetition, const db::ICplxTrans &ct)
{
  switch (repetition.getTypes ().which ()) {
  case stream::repetition::Repetition::Types::ENUMERATED:
    {
      if (! mp_layout->is_editable ()) {

        db::CellInstArray ca;

        if (ct.is_complex ()) {
          db::CellInstArray::iterated_complex_array_type array (ct.rcos (), ct.mag ());
          make_iterated_array (repetition, array);
          ca = db::CellInstArray (db::CellInst (of_cell), db::Trans (ct), mp_layout->array_repository ().insert (array));
        } else {
          db::CellInstArray::iterated_array_type array;
          make_iterated_array (repetition, array);
          ca = db::CellInstArray (db::CellInst (of_cell), db::Trans (ct), mp_layout->array_repository ().insert (array));
        }

        if (prop_id == 0) {
          mp_cell->insert (ca);
        } else {
          mp_cell->insert (db::CellInstArrayWithProperties (ca, prop_id));
        }

      } else {

        //  resolve iterated arrays in non-editable mode

        std::vector<db::Vector> vectors;
        make_vectors (repetition, vectors);

        for (auto v = vectors.begin (); v != vectors.end (); ++v) {
          make_single_cell_instance (of_cell, prop_id, db::ICplxTrans (*v) * ct);
        }

      }
    }
    break;

  case stream::repetition::Repetition::Types::REGULAR:
  case stream::repetition::Repetition::Types::REGULAR_ORTHO:
    {
      db::CellInstArray ca;

      db::Vector a, b;
      unsigned long na = 0, nb = 0;
      get_regular_array (repetition, a, b, na, nb);

      if (ct.is_complex ()) {
        ca = db::CellInstArray (db::CellInst (of_cell), ct, mp_layout->array_repository (), a, b, na, nb);
      } else {
        ca = db::CellInstArray (db::CellInst (of_cell), db::Trans (ct), mp_layout->array_repository (), a, b, na, nb);
      }

      if (prop_id == 0) {
        mp_cell->insert (ca);
      } else {
        mp_cell->insert (db::CellInstArrayWithProperties (ca, prop_id));
      }
    }
    break;

  case stream::repetition::Repetition::Types::SINGLE:
    make_single_cell_instance (of_cell, prop_id, ct);
    break;

  default:
    break;
  }
}

/**
 *  @brief Creates array objects in "Ref" mode
 * 
 *  This method takes an object (e.g. Box etc.), a layer index and a
 *  KLayout property Id and a repetition specification.
 * 
 *  In editable mode, it will explode the repetition into a number of
 *  single shapes.
 * 
 *  In non-editable mode, it will create respective shape arrays.
 *  The shape arrays are of db::array<Object, db::UnitTrans> type.
 *  These are the valid types for simple shapes such as edges and boxes.
 * 
 *  The objects are placed inside the current cell at the layer given
 *  by "li".
 * 
 *  @param li The layer index where the shapes are to be placed
 *  @param prop_id The (KLayout) property Id for the properties to attach to the shapes
 *  @param object The basic object to place (times the repetition)
 *  @param repetition The LStream repetition of the cell
 */
template <class Object>
void
Reader::make_object_array_ref (unsigned int li, db::properties_id_type prop_id, const Object &object, stream::repetition::Repetition::Reader repetition)
{
  if (mp_layout->is_editable ()) {
    make_object_array_explode (li, prop_id, object, repetition);
    return;
  }

  switch (repetition.getTypes ().which ()) {
  case stream::repetition::Repetition::Types::ENUMERATED:
    {
      typedef db::array<Object, db::UnitTrans> array_type;
      typename array_type::iterated_array_type array;
      make_iterated_array (repetition, array);

      if (prop_id == 0) {
        mp_cell->shapes (li).insert (array_type (object, db::UnitTrans (), mp_layout->array_repository ().insert (array)));
      } else {
        mp_cell->shapes (li).insert (db::object_with_properties<array_type> (array_type (object, db::UnitTrans (), mp_layout->array_repository ().insert (array)), prop_id));
      }
    }
    break;

  case stream::repetition::Repetition::Types::REGULAR:
  case stream::repetition::Repetition::Types::REGULAR_ORTHO:
    {
      db::Vector a, b;
      unsigned long na = 0, nb = 0;
      get_regular_array (repetition, a, b, na, nb);

      db::array<Object, db::UnitTrans> array (object, db::UnitTrans (), mp_layout->array_repository (), a, b, na, nb);

      if (prop_id == 0) {
        mp_cell->shapes (li).insert (array);
      } else {
        mp_cell->shapes (li).insert (db::object_with_properties<db::array<Object, db::UnitTrans> > (array, prop_id));
      }
    }
    break;

  case stream::repetition::Repetition::Types::SINGLE:
    mp_cell->shapes (li).insert (object);
    break;

  default:
    break;
  }
}

/**
 *  @brief Creates array objects in "Ref to Ptr" mode
 * 
 *  This method takes an "ref" type object (e.g. PolygonRef etc.), a layer index and a
 *  KLayout property Id and a repetition specification.
 * 
 *  In editable mode, it will explode the repetition into a number of
 *  single shapes.
 * 
 *  In non-editable mode, it will create respective shape arrays.
 *  The shape arrays are of db::array<Object, db::DispTrans> type.
 *  The object will be converted to a "Ptr" type given by the "ObjectPtr"
 *  template argument. For example, the corresponding "Ptr" type for
 *  "PolygonRef" is "PolygonPtr". Eventually this is a pointer to a polygon
 *  without the displacement implied by "PolygonRef". This translation
 *  is implied by the array's "front" object.
 * 
 *  The objects are placed inside the current cell at the layer given
 *  by "li".
 * 
 *  @param li The layer index where the shapes are to be placed
 *  @param prop_id The (KLayout) property Id for the properties to attach to the shapes
 *  @param object The basic object to place (times the repetition)
 *  @param repetition The LStream repetition of the cell
 */
template <class Object, class ObjectPtr>
void
Reader::make_object_array_ptr (unsigned int li, db::properties_id_type prop_id, const Object &object, stream::repetition::Repetition::Reader repetition)
{
  if (mp_layout->is_editable ()) {
    make_object_array_explode (li, prop_id, object, repetition);
    return;
  }

  switch (repetition.getTypes ().which ()) {
  case stream::repetition::Repetition::Types::ENUMERATED:
    {
      typedef db::array<ObjectPtr, db::Disp> array_type;
      typename array_type::iterated_array_type array;
      make_iterated_array (repetition, array);

      ObjectPtr ptr (object.ptr (), db::UnitTrans ());

      if (prop_id == 0) {
        mp_cell->shapes (li).insert (array_type (ptr, object.trans (), mp_layout->array_repository ().insert (array)));
      } else {
        mp_cell->shapes (li).insert (db::object_with_properties<array_type> (array_type (ptr, object.trans (), mp_layout->array_repository ().insert (array)), prop_id));
      }
    }
    break;

  case stream::repetition::Repetition::Types::REGULAR:
  case stream::repetition::Repetition::Types::REGULAR_ORTHO:
    {
      db::Vector a, b;
      unsigned long na = 0, nb = 0;
      get_regular_array (repetition, a, b, na, nb);

      ObjectPtr ptr (object.ptr (), db::UnitTrans ());
      db::array<ObjectPtr, db::Disp> array (ptr, object.trans (), mp_layout->array_repository (), a, b, na, nb);

      if (prop_id == 0) {
        mp_cell->shapes (li).insert (array);
      } else {
        mp_cell->shapes (li).insert (db::object_with_properties<db::array<ObjectPtr, db::Disp> > (array, prop_id));
      }
    }
    break;

  case stream::repetition::Repetition::Types::SINGLE:
    mp_cell->shapes (li).insert (object);
    break;

  default:
    break;
  }
}

/**
 *  @brief Creates array objects in "explode" mode
 * 
 *  This method takes an object (e.g. db::Box, db::Polygon), a layer index and a
 *  KLayout property Id and a repetition specification.
 * 
 *  It will explode the repetition into a number of single shapes and place
 *  them inside the current cell on the given layer.
 * 
 *  @param li The layer index where the shapes are to be placed
 *  @param prop_id The (KLayout) property Id for the properties to attach to the shapes
 *  @param object The basic object to place (times the repetition)
 *  @param repetition The LStream repetition of the cell
 */
template <class Object>
void
Reader::make_object_array_explode (unsigned int li, db::properties_id_type prop_id, const Object &object, stream::repetition::Repetition::Reader repetition)
{
  switch (repetition.getTypes ().which ()) {
  case stream::repetition::Repetition::Types::ENUMERATED:
    {
      std::vector<db::Vector> vectors;
      make_vectors (repetition, vectors);

      for (auto v = vectors.begin (); v != vectors.end (); ++v) {

        Object moved_object (object);
        moved_object.transform (db::Disp (*v));

        if (prop_id == 0) {
          mp_cell->shapes (li).insert (moved_object);
        } else {
          mp_cell->shapes (li).insert (db::object_with_properties<Object> (moved_object, prop_id));
        }

      }
    }
    break;

  case stream::repetition::Repetition::Types::REGULAR:
  case stream::repetition::Repetition::Types::REGULAR_ORTHO:
    {
      db::Vector a, b;
      unsigned long na = 0, nb = 0;
      get_regular_array (repetition, a, b, na, nb);

      na = std::max ((unsigned long) 1, na);
      nb = std::max ((unsigned long) 1, nb);

      db::Vector da;
      for (unsigned long ia = 0; ia < na; ++ia, da += a) {

        db::Vector db;
        for (unsigned long ib = 0; ib < nb; ++ib, db += b) {

          Object moved_object (object);
          moved_object.transform (db::Disp (da + db));

          if (prop_id == 0) {
            mp_cell->shapes (li).insert (moved_object);
          } else {
            mp_cell->shapes (li).insert (db::object_with_properties<Object> (moved_object, prop_id));
          }

        }

      }

    }
    break;

  case stream::repetition::Repetition::Types::SINGLE:
    if (prop_id == 0) {
      mp_cell->shapes (li).insert (object);
    } else {
      mp_cell->shapes (li).insert (db::object_with_properties<Object> (object, prop_id));
    }
    break;

  default:
    break;
  }
}

/**
 *  @brief Overload: creates an object array for SimplePolygonRef objects
 */
void
Reader::make_object_array (unsigned int li, db::properties_id_type prop_id, const db::SimplePolygonRef &object, stream::repetition::Repetition::Reader repetition)
{
  make_object_array_ptr<db::SimplePolygonRef, db::SimplePolygonPtr> (li, prop_id, object, repetition);
}

/**
 *  @brief Overload: creates an object array for PolygonRef objects
 */
void
Reader::make_object_array (unsigned int li, db::properties_id_type prop_id, const db::PolygonRef &object, stream::repetition::Repetition::Reader repetition)
{
  make_object_array_ptr<db::PolygonRef, db::PolygonPtr> (li, prop_id, object, repetition);
}

/**
 *  @brief Overload: creates an object array for PathRef objects
 */
void
Reader::make_object_array (unsigned int li, db::properties_id_type prop_id, const db::PathRef &object, stream::repetition::Repetition::Reader repetition)
{
  make_object_array_ptr<db::PathRef, db::PathPtr> (li, prop_id, object, repetition);
}

/**
 *  @brief Overload: creates an object array for Box objects
 */
void
Reader::make_object_array (unsigned int li, db::properties_id_type prop_id, const db::Box &object, stream::repetition::Repetition::Reader repetition)
{
  make_object_array_ref (li, prop_id, object, repetition);
}

/**
 *  @brief Overload: creates an object array for Edge objects
 */
void
Reader::make_object_array (unsigned int li, db::properties_id_type prop_id, const db::Edge &object, stream::repetition::Repetition::Reader repetition)
{
  make_object_array_explode (li, prop_id, object, repetition);
}

/**
 *  @brief Overload: creates an object array for EdgePair objects
 */
void
Reader::make_object_array (unsigned int li, db::properties_id_type prop_id, const db::EdgePair &object, stream::repetition::Repetition::Reader repetition)
{
  make_object_array_explode (li, prop_id, object, repetition);
}

/**
 *  @brief Overload: creates an object array for Point objects
 */
void
Reader::make_object_array (unsigned int li, db::properties_id_type prop_id, const db::Point &object, stream::repetition::Repetition::Reader repetition)
{
  make_object_array_explode (li, prop_id, object, repetition);
}

/**
 *  @brief Overload: creates an object array for Text objects
 */
void
Reader::make_object_array (unsigned int li, db::properties_id_type prop_id, const db::Text &object, stream::repetition::Repetition::Reader repetition)
{
  make_object_array_explode (li, prop_id, object, repetition);
}

/**
 *  @brief Reads the instances for a layout view
 * 
 *  This method will take the given layout view (stream::layoutView::LayoutView)
 *  and produce the instances contained therein in the current cell.
 * 
 *  This method is called while reading the layout view.
 */
void
Reader::read_instances (stream::layoutView::LayoutView::Reader layout_view)
{
  auto instance_repetitions = layout_view.getInstanceRepetitions ();
  auto instances = layout_view.getInstances ();

  auto basic = instances.getBasic ();
  auto with_properties = instances.getWithProperties ();
  auto arrays = instances.getArrays ();
  auto arrays_with_properties = instances.getArraysWithProperties ();

  for (auto i = basic.begin (); i != basic.end (); ++i) {
    auto of_cell = cell_for_instance (*mp_layout, i->getBasic ().getCellId ());
    make_single_cell_instance (of_cell, 0, make_transformation (i->getBasic ().getTransformation ()));
  }

  for (auto i = with_properties.begin (); i != with_properties.end (); ++i) {
    auto of_cell = cell_for_instance (*mp_layout, i->getBasic ().getCellId ());
    auto prop_id = get_properties_id_by_id (i->getPropertySetId ());
    make_single_cell_instance (of_cell, prop_id, make_transformation (i->getBasic ().getTransformation ()));
  }

  for (auto i = arrays.begin (); i != arrays.end (); ++i) {
    auto of_cell = cell_for_instance (*mp_layout, i->getBasic ().getCellId ());
    auto rep = i->getRepetitionId ();
    if (rep == 0) {
      make_single_cell_instance (of_cell, 0, make_transformation (i->getBasic ().getTransformation ()));
    } else {
      --rep;
      tl_assert (rep < instance_repetitions.size ());
      make_cell_instance (of_cell, 0, instance_repetitions [rep], make_transformation (i->getBasic ().getTransformation ()));
    }
  }

  for (auto i = arrays_with_properties.begin (); i != arrays_with_properties.end (); ++i) {
    auto of_cell = cell_for_instance (*mp_layout, i->getBasic ().getBasic ().getCellId ());
    auto prop_id = get_properties_id_by_id (i->getPropertySetId ());
    auto rep = i->getBasic ().getRepetitionId ();
    if (rep == 0) {
      make_single_cell_instance (of_cell, prop_id, make_transformation (i->getBasic ().getBasic ().getTransformation ()));
    } else {
      --rep;
      tl_assert (rep < instance_repetitions.size ());
      make_cell_instance (of_cell, prop_id, instance_repetitions [rep], make_transformation (i->getBasic ().getBasic ().getTransformation ()));
    }
  }
}

/**
 *  @brief Reads the shapes of a given kindf for a layout view
 * 
 *  This method will take the given shape container (stream::layoutView::ObjectContainerForType<CPObject>)
 *  and produce the shapes of "CPObject" type contained therein. The shapes will
 *  be placed on the given layer (by "li") in the current cell. 
 * 
 *  The KLayout shape type is given by the "Object" template parameter.
 * 
 *  This method is called while reading the layout view.
 * 
 *  @param li The layer index where to produce the shapes in the current cell
 *  @param reader The shape container
 *  @param repetitions The repetition repository. This list is used to identify the repetition per shape.
 */
template <class Object, class CPObject>
void
Reader::read_shapes (unsigned int li, typename stream::layoutView::ObjectContainerForType<CPObject>::Reader reader, capnp::List<stream::repetition::Repetition, capnp::Kind::STRUCT>::Reader repetitions) 
{
  tl_assert (mp_cell != 0);

  auto basic = reader.getBasic ();
  auto with_properties = reader.getWithProperties ();
  auto arrays = reader.getArrays ();
  auto arrays_with_properties = reader.getArraysWithProperties ();

  for (auto i = basic.begin (); i != basic.end (); ++i) {
    mp_cell->shapes (li).insert (make_object (i->getBasic ()));
  }

  for (auto i = with_properties.begin (); i != with_properties.end (); ++i) {
    auto prop_id = get_properties_id_by_id (i->getPropertySetId ());
    mp_cell->shapes (li).insert (db::object_with_properties<Object> (make_object (i->getBasic ()), prop_id));
  }

  for (auto i = arrays.begin (); i != arrays.end (); ++i) {
    auto object = make_object (i->getBasic ());
    auto rep = i->getRepetitionId ();
    if (rep == 0) {
      mp_cell->shapes (li).insert (object);
    } else {
      --rep;
      tl_assert (rep < repetitions.size ());
      make_object_array (li, 0, object, repetitions [rep]);
    }
  }

  for (auto i = arrays_with_properties.begin (); i != arrays_with_properties.end (); ++i) {
    auto object = make_object (i->getBasic ().getBasic ());
    auto prop_id = get_properties_id_by_id (i->getPropertySetId ());
    auto rep = i->getBasic ().getRepetitionId ();
    if (rep == 0) {
      mp_cell->shapes (li).insert (db::object_with_properties<Object> (object, prop_id));
    } else {
      --rep;
      tl_assert (rep < repetitions.size ());
      make_object_array (li, prop_id, object, repetitions [rep]);
    }
  }
}

/**
 *  @brief Reads a layer from the given stream::layoutView::Layer
 * 
 *  The method is called while processing the layout view message.
 *  It is called per layer inside the layout view. It will extract
 *  the various shape types and place them in the specified layer.
 */
void
Reader::read_layer (stream::layoutView::Layer::Reader reader)
{
  auto li = get_layer_by_id (reader.getLayerId ());
  auto repetitions = reader.getRepetitions ();

  read_shapes<db::Box, stream::geometry::Box> (li, reader.getBoxes (), repetitions);
  read_shapes<db::Edge, stream::geometry::Edge> (li, reader.getEdges (), repetitions);
  read_shapes<db::EdgePair, stream::geometry::EdgePair> (li, reader.getEdgePairs (), repetitions);
  read_shapes<db::SimplePolygonRef, stream::geometry::SimplePolygon> (li, reader.getSimplePolygons (), repetitions);
  read_shapes<db::PolygonRef, stream::geometry::Polygon> (li, reader.getPolygons (), repetitions);
  read_shapes<db::Point, stream::geometry::Point> (li, reader.getPoints (), repetitions);
  read_shapes<db::Text, stream::geometry::Label> (li, reader.getLabels (), repetitions);
  read_shapes<db::PathRef, stream::geometry::Path> (li, reader.getPaths (), repetitions);
}

/**
 *  @brief Processes the layout view message
 * 
 *  This method reads the instances and shapes contained in this message
 *  into the cell given by "cell_index".
 * 
 *  While this method is executed the current cell is set.
 */
void
Reader::read_layout_view (db::cell_index_type cell_index, kj::BufferedInputStream &is)
{
  mp_cell = &mp_layout->cell (cell_index);

  // NOTE: maybe that is not wise, but these message can become really large ...
  capnp::ReaderOptions options;
  options.traversalLimitInWords = std::numeric_limits<uint64_t>::max ();

  yield_progress ();
  capnp::PackedMessageReader message (is, options);
  stream::layoutView::LayoutView::Reader layout_view = message.getRoot<stream::layoutView::LayoutView> ();

  if (mp_cell->is_proxy ()) {
    //  Do not read proxies (library cells, pcells) as they are restored already 
    //  and are connected to some source.
    //  NOTE: this is a decision to "always update" which in actually should be
    //  configurable. To "use data from stream", we should not update the cell 
    //  to proxy data and use the stream data instead.
    return;
  }

  //  store the bounding box information if requested
  if (! m_bbox_meta_data_key.empty ()) {
    auto bbox = make_object (layout_view.getBoundingBox ());
    mp_layout->add_meta_info (cell_index, m_bbox_meta_data_key, db::MetaInfo (std::string (), bbox));
  }

  read_instances (layout_view);

  auto layers = layout_view.getLayers ();
  for (auto l = layers.begin (); l != layers.end (); ++l) {
    read_layer (*l);
  }

  mp_cell = 0;
}

/**
 *  @brief Processes the meta data view message
 * 
 *  This method reads the meta data information for a cell.
 */
void
Reader::read_meta_data_view (db::cell_index_type cell_index, kj::BufferedInputStream &is)
{
  mp_cell = &mp_layout->cell (cell_index);

  yield_progress ();

  capnp::PackedMessageReader message (is);
  stream::metaDataView::MetaDataView::Reader meta_data = message.getRoot<stream::metaDataView::MetaDataView> ();

  make_meta_data (&mp_layout->cell (cell_index), meta_data.getData ());
}

/** 
 *  @brief This method is called "frequently" to yield the progress
 */
void 
Reader::yield_progress ()
{
  m_progress.set (m_stream.position ());
}

/**
 *  @brief Reader the global header
 * 
 *  This method will also decide which library to parse if
 *  multiple libraries are present.
 * 
 *  If a single layout-type library is present, it will read that
 *  one. Otherwise it will read the first unnamed one or the
 *  first one if there are no unnamed libraries.
 * 
 *  It will set "m_library_index" to the index of the library
 *  we want to read.
 */
void
Reader::read_header (kj::BufferedInputStream &is)
{
  yield_progress ();
  capnp::PackedMessageReader message (is);

  stream::header::Header::Reader header = message.getRoot<stream::header::Header> ();

  //  fetch technology

  std::string technology_name = header.getTechnology ();
  if (! technology_name.empty ()) {
    //  TODO: need more that this?
    mp_layout->set_technology_name (technology_name);
  }

  //  decide for the library to read

  int library_index = -1;
  m_libname.clear ();

  auto libraries = header.getLibraries ();
  for (auto l = libraries.begin (); l != libraries.end () && library_index < 0; ++l) {
    if (l->getType () == "layout" && l->getName () == "") {
      library_index = (l - libraries.begin ());
    }
  }

  for (auto l = libraries.begin (); l != libraries.end () && library_index < 0; ++l) {
    if (l->getType () == "layout") {
      library_index = (l - libraries.begin ());
      m_libname = l->getName ();
    }
  }

  if (library_index < 0) {
    std::set <std::string> types;
    for (auto l = libraries.begin (); l != libraries.end (); ++l) {
      if (l->getType ().size () > 0) {
        types.insert (l->getType ());
      }
    }
    std::string types_str = tl::join (types.begin (), types.end (), ", ");
    if (types_str.empty ()) {
      error (tl::to_string (tr ("An LStream needs to have a library of type 'layout' to be loaded into KLayout - this stream does not have any")));
    } else {
      error (tl::to_string (tr ("An LStream needs to have a library of type 'layout' to be loaded into KLayout - present types are: ")) + types_str);
    }
  }

  m_library_index = size_t (library_index);
}

/**
 *  @brief Skips a library, including cells and cell views
 */
void
Reader::skip_library (kj::BufferedInputStream &is)
{
  yield_progress ();
  capnp::PackedMessageReader message (is);
  stream::library::Library::Reader library = message.getRoot<stream::library::Library> ();

  size_t cells = library.getCellSpecsTable ().getCellSpecs ().size ();

  for (size_t i = 0; i < cells; ++i) {

    //  fetch the cell message to extract the number of views
    yield_progress ();
    capnp::PackedMessageReader cell_message (is);
    stream::cell::Cell::Reader cell = message.getRoot<stream::cell::Cell> ();
    size_t views = cell.getViewIds ().size ();

    for (size_t i = 0; i < views; ++i) {
      //  skip the views
      yield_progress ();
      capnp::PackedMessageReader consumed (is);
    }

  }
}

/**
 *  @brief Reads the library message
 * 
 *  Processes the various tables contained inside the library.
 */
void
Reader::read_library (kj::BufferedInputStream &is)
{
  yield_progress ();
  capnp::PackedMessageReader message (is);

  stream::library::Library::Reader library = message.getRoot<stream::library::Library> ();

  //  Basic properties

  //  Obtain the layout and (optional) meta data  view Id

  m_layout_view_id = std::numeric_limits<uint64_t>::max ();
  m_meta_data_view_id = std::numeric_limits<uint64_t>::max ();

  auto views = library.getViewSpecsTable ().getViewSpecs ();
  for (auto v = views.begin (); v != views.end (); ++v) {
    if (v->getName () == "layout" && v->getClass () == "LayoutView") {
      if (m_layout_view_id == std::numeric_limits<uint64_t>::max ()) {
        m_layout_view_id = (v - views.begin ());
      }
    } else if (v->getName () == "metaData" && v->getClass () == "MetaDataView") {
      if (m_meta_data_view_id == std::numeric_limits<uint64_t>::max ()) {
        m_meta_data_view_id = (v - views.begin ());
      }
    }
  }

  if (m_layout_view_id == std::numeric_limits<uint64_t>::max ()) {
    std::set <std::string> view_strings;
    for (auto v = views.begin (); v != views.end (); ++v) {
      view_strings.insert (v->getName ());
    }
    std::string views_str = tl::join (view_strings.begin (), view_strings.end (), ", ");
    error (tl::to_string (tr ("There is no view called 'layout' with 'LayoutView' class - present views are: ")) + views_str);
  }

  auto layout_view = views[m_layout_view_id];

  //  Read the tables we're interested in

  //  "Properties" and "Libraries" need to be first as we have to provide properties and library names
  read_properties (library);
  read_library_refs (library);

  read_layers (layout_view);
  read_cells (library);
  read_text_strings (library);

  //  Now as we have read the properties tables, we can set the global properties

  mp_layout->prop_id (get_properties_id_by_id (layout_view.getPropertySetId ()));
  make_meta_data (0, layout_view.getMetaData ());

  double resolution = layout_view.getResolution ();
  if (resolution < 1e-10) {
    error (tl::to_string (tr ("The resolution is an invalid value: ")) + tl::to_string (resolution));
  }
  mp_layout->dbu (1.0 / resolution);
}

/**
 *  @brief Gets the KLayout layer Id from a LStream layer Id
 * 
 *  This method can only be called after the library has been processed.
 *  It will assert if the LStream Id is not valid.
 */
unsigned int 
Reader::get_layer_by_id (uint64_t id) const
{
  auto m = m_layer_id_map.find (id);
  tl_assert (m != m_layer_id_map.end ());
  return m->second;
}

/**
 *  @brief Gets the name of the library for a given LStream library Id
 * 
 *  This method can only be called after the library has been processed.
 *  It will assert if the LStream Id is not valid.
 */
std::string
Reader::get_library_name_by_id (uint64_t id) const
{
  if (id == 0) {
    return std::string ();
  } else {
    auto m = m_library_names_by_id.find (id);
    tl_assert (m != m_library_names_by_id.end ());
    return m->second;
  }
}

/**
 *  @brief Gets the KLayout property name Id for a given LStream property name Id
 * 
 *  This method can only be called after the library has been processed.
 *  It will assert if the LStream Id is not valid.
 */
db::property_names_id_type 
Reader::get_property_name_id_by_id (uint64_t id) const
{
  auto m = m_property_name_id_map.find (id);
  tl_assert (m != m_property_name_id_map.end ());
  return m->second;
}

/**
 *  @brief Gets the KLayout property set Id for a given LStream property set Id
 * 
 *  This method can only be called after the library has been processed.
 *  It will assert if the LStream Id is not valid.
 */
db::properties_id_type 
Reader::get_properties_id_by_id (uint64_t id) const
{
  if (id == 0) {
    return 0;
  } else {
    auto m = m_properties_id_map.find (id);
    tl_assert (m != m_properties_id_map.end ());
    return m->second;
  }
}

/**
 *  @brief Gets the db::StringRef (a text string proxy) for a given LStream text Id
 * 
 *  This method can only be called after the library has been processed.
 *  It will assert if the LStream Id is not valid.
 */
const db::StringRef *
Reader::get_string_by_id (uint64_t id) const
{
  auto m = m_text_strings_by_id.find (id);
  tl_assert (m != m_text_strings_by_id.end ());
  return m->second;
}

}

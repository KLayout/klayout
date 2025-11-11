
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

#ifndef HDR_lstrCompressed
#define HDR_lstrCompressed

#include "lstrCompressor.h"

#include "dbVector.h"
#include "dbBox.h"
#include "dbPath.h"
#include "dbEdge.h"
#include "dbEdgePair.h"
#include "dbPolygon.h"
#include "dbText.h"
#include "dbObjectWithProperties.h"
#include "dbCell.h"

#include <map>
#include <vector>

namespace lstr
{

/**
 *  @brief A container for storing objects and compressing them using regular or irregular repetitions
 * 
 *  Use this object to compress db::Shapes or db::Instances containers.
 *  Use "Compressed::compress_shapes" to compress a Shapes container and 
 *  "Compressed::compress_instances" to compress an Instances container.
 *  
 *  After feeding the objects to be compressed using one of these methods,
 *  you can access the compressed objects using "Compressed::get_container".
 *  These containers are separated by objects and carray plain objects
 *  (no compression), plain objects with properties (no compression either),
 *  array objects (object that have been compressed into regular or irregular
 *  arrays) and array objects with properties (the same, but also with 
 *  properties).
 * 
 *  During compression, arrays are formed. The array definitions are kept 
 *  in two lists: regular arrays and irregular (iterated or enumerated) 
 *  arrays. The former can be obtained using "Compressed::begin/end_regular_arrays",
 *  the latter using "Compressed::begin/end_irregular_arrays". 
 *  The total number of array objects can be obtained using 
 *  "Compressed::num_arrays".
 */
class Compressed
  : public CompressorDelivery
{
public:

  /**
   *  @brief Represents compressed objects
   * 
   *  "plain" is a list of plain objects - those which have not been
   *  compressed.
   * 
   *  "with_properties" is a list of pairs of objects and properties
   *  Id. The properties Id is a concept from the db namespace and
   *  represents a property set by some opaque identifier.
   * 
   *  "array" is a list of pairs of objects and repetition specifications. 
   *  The repetition is specified through an index (a uint64_t value).
   *  The index is given by the second value when you iterate the repetitions
   *  in "Compressed::begin_regular_arrays" or "Compressed::begin_irregular_arrays".
   * 
   *  "array_with_properties" is a list of pairs or pairs: "first.first"
   *  is the object, "first.second" the properties Id and "second" the
   *  repetition index (see "array").
   */
  template <class Obj> 
  struct compressed_container
  {
    std::list<Obj> plain;
    std::list<std::pair<Obj, db::properties_id_type> > with_properties;
    std::list<std::pair<Obj, uint64_t> > array;
    std::list<std::pair<std::pair<Obj, db::properties_id_type>, uint64_t> > array_with_properties;
  };

  /**
   *  @brief Constructor
   * 
   *  Note that you need to create a fresh object to compress
   *  instances are shapes. There is no "clear" or "restart" method.
   */
  Compressed ();

  /**
   *  @brief Compresses a shape container
   * 
   *  This method should be called on a fresh Compressed object only. 
   * 
   *  @param shapes The shapes container to compress
   *  @param level Compression level (see below)
   *  @param recompress Indicates where to recompress (see below)
   * 
   *  "level" is a value the indicates the compression effort made for forming
   *  regular arrays. A value of "0" indicates that no arrays will be formed.
   *  "2" indicates "reasonable effort" to form arrays. Roughly, the value indicates
   *  how many neighbors to investigate for forming arrays.
   * 
   *  If "recompress" is true, existing arrays will be exploded and fed into
   *  the compression algorithm again. If false, they will be maintained.
   */
  void compress_shapes (const db::Shapes &shapes, unsigned int level, bool recompress);

  /**
   *  @brief Compresses instances
   * 
   *  This method should be called on a fresh Compressed object only. 
   * 
   *  @param begin_instances The instance iterator specifying the instances to compress
   *  @param cells_to_write A filter specifying the cells to write
   *  @param level Compression level (see "compress_shapes")
   * 
   *  "Cell::begin" can be used to generate the value for "instance_iterator".
   * 
   *  Instances for cells not in "cells_to_write" are not generated.
   */
  void compress_instances (const db::Cell::const_iterator &begin_instances, const std::set<db::cell_index_type> &cells_to_write, unsigned int level);

  /**
   *  @brief Gets the compressed objects for a given object type
   * 
   *  Object types can be "db::Box", "db::Edge", "db::EdgePair", 
   *  "db::Polygon", "db::SimplePolygon", "db::Path" or "db::Text"
   *  for Shapes and "db::CellInstArray" for instances.
   * 
   *  For "db::CellInstArray", the objects will be single instances
   *  and the array nature is reflected by the repetition type.
   * 
   *  Note that either "compress_shapes" or "compress_instances" has
   *  to be called before the container is available.
   */
  template <class Obj> compressed_container<Obj> &get_container ();

  /**
   *  @brief begin iterator for the regular arrays
   * 
   *  The iterator delivers a pair of regular array specifications and
   *  an index. The array specification is a RegularArray object that
   *  specifies the array axes, pitches and array dimensions.
   * 
   *  The second value is the repetition index, by which the compressed
   *  objects refer to that array.
   * 
   *  Note that either "compress_shapes" or "compress_instances" has
   *  to be called before the arrays are available.
   */
  std::map<RegularArray, uint64_t>::const_iterator begin_regular_arrays () const  { return m_array_to_rep_id.begin (); }
  
  /**
   *  @brief end iterator for the regular arrays
   */
  std::map<RegularArray, uint64_t>::const_iterator end_regular_arrays () const    { return m_array_to_rep_id.end (); }

  /**
   *  @brief begin iterator for the irregular arrays
   * 
   *  The iterator delivers a pair of irregular array specifications and
   *  an index. The array specification is a list of displacements that
   *  specify the placements of the object.
   * 
   *  The second value is the repetition index, by which the compressed
   *  objects refer to that array.
   * 
   *  Note that either "compress_shapes" or "compress_instances" has
   *  to be called before the arrays are available.
   */
  std::map<std::vector<db::Vector>, uint64_t>::const_iterator begin_irregular_arrays () const { return m_irregular_to_rep_id.begin (); }

  /**
   *  @brief end iterator for the irregular arrays
   */
  std::map<std::vector<db::Vector>, uint64_t>::const_iterator end_irregular_arrays () const   { return m_irregular_to_rep_id.end (); }

  /**
   *  @brief Gets the total number of arrays stored in this object
   * 
   *  Note that either "compress_shapes" or "compress_instances" has
   *  to be called before the arrays are available.
   */
  size_t num_arrays () const
  {
    return m_array_to_rep_id.size () + m_irregular_to_rep_id.size ();
  }

protected:
  virtual void write (const db::Point &obj,                       const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::PointWithProperties &obj,         const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::Box &obj,                         const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::BoxWithProperties &obj,           const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::Edge &obj,                        const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::EdgeWithProperties &obj,          const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::EdgePair &obj,                    const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::EdgePairWithProperties &obj,      const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::Polygon &obj,                     const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::PolygonWithProperties &obj,       const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::SimplePolygon &obj,               const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::SimplePolygonWithProperties &obj, const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::Path &obj,                        const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::PathWithProperties &obj,          const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::Text &obj,                        const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::TextWithProperties &obj,          const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::CellInstArray &obj,               const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }
  virtual void write (const db::CellInstArrayWithProperties &obj, const RegularArray &array, const disp_vector &irregular) { store (obj, array, irregular); }

private:
  compressed_container<db::Point> m_points;
  compressed_container<db::Box> m_boxes;
  compressed_container<db::Edge> m_edges;
  compressed_container<db::EdgePair> m_edge_pairs;
  compressed_container<db::Path> m_paths;
  compressed_container<db::Polygon> m_polygons;
  compressed_container<db::SimplePolygon> m_simple_polygons;
  compressed_container<db::Text> m_texts;
  compressed_container<db::CellInstArray> m_instances;

  template <class Obj>
  void store_no_props (const Obj &obj, const RegularArray &array, const std::vector<db::Vector> &irregular)
  {
    compressed_container<Obj> &cont = get_container<Obj> ();
    if (array.is_null () && irregular.empty ()) {
      cont.plain.push_back (obj);
    } else {
      cont.array.push_back (std::make_pair (obj, make_rep_id (array, irregular)));
    }
  }

  template <class Obj>
  void store (const Obj &obj, const RegularArray &array, const std::vector<db::Vector> &irregular)
  {
    store_no_props (obj, array, irregular);
  }

  template <class Obj>
  void store (const db::object_with_properties<Obj> &obj, const RegularArray &array, const std::vector<db::Vector> &irregular)
  {
    compressed_container<Obj> &cont = get_container<Obj> ();
    if (obj.properties_id () == 0) {
      store_no_props<Obj> (obj, array, irregular);
    } else {
      if (array.is_null () && irregular.empty ()) {
        cont.with_properties.push_back (std::pair<Obj, db::properties_id_type> (obj, obj.properties_id ()));
      } else {
        cont.array_with_properties.push_back (std::make_pair (std::pair<Obj, db::properties_id_type> (obj, obj.properties_id ()), make_rep_id (array, irregular)));
      }
    }
  }

  size_t m_next_id;
  std::map<RegularArray, uint64_t> m_array_to_rep_id;
  std::map<std::vector<db::Vector>, uint64_t> m_irregular_to_rep_id;

  uint64_t make_rep_id (const RegularArray &array, const std::vector<db::Vector> &irregular);
  db::Vector create_repetition (const db::Shape &array, RegularArray &regular, std::vector<db::Vector> &irregular_array);
  template <class Obj>
  void write_shape(const db::Shape &shape, RegularArray &regular, std::vector<db::Vector> &irregular_array);
};

template <> inline Compressed::compressed_container<db::Point> &Compressed::get_container () { return m_points; }
template <> inline Compressed::compressed_container<db::Box> &Compressed::get_container () { return m_boxes; }
template <> inline Compressed::compressed_container<db::Edge> &Compressed::get_container () { return m_edges; }
template <> inline Compressed::compressed_container<db::EdgePair> &Compressed::get_container () { return m_edge_pairs; }
template <> inline Compressed::compressed_container<db::Path> &Compressed::get_container () { return m_paths; }
template <> inline Compressed::compressed_container<db::Polygon> &Compressed::get_container () { return m_polygons; }
template <> inline Compressed::compressed_container<db::SimplePolygon> &Compressed::get_container () { return m_simple_polygons; }
template <> inline Compressed::compressed_container<db::Text> &Compressed::get_container () { return m_texts; }
template <> inline Compressed::compressed_container<db::CellInstArray> &Compressed::get_container () { return m_instances; }

}

#endif


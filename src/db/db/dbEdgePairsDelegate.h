
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


#ifndef HDR_dbEdgePairsDelegate
#define HDR_dbEdgePairsDelegate

#include "dbCommon.h"
#include "dbEdgePair.h"
#include "dbShapeCollection.h"
#include "dbShapeCollectionUtils.h"
#include "dbGenericShapeIterator.h"

namespace db {

class RecursiveShapeIterator;
class EdgePairs;
class EdgePairFilterBase;
class RegionDelegate;
class EdgesDelegate;
class Layout;

typedef shape_collection_processor<db::EdgePair, db::EdgePair> EdgePairProcessorBase;
typedef shape_collection_processor<db::EdgePair, db::Polygon> EdgePairToPolygonProcessorBase;
typedef shape_collection_processor<db::EdgePair, db::Edge> EdgePairToEdgeProcessorBase;

class DB_PUBLIC
EdgePairToPolygonProcessor
  : public EdgePairToPolygonProcessorBase
{
public:
  EdgePairToPolygonProcessor (db::Coord e)
    : m_e (e)
  { }

  void process(const EdgePairWithProperties &ep, std::vector<db::PolygonWithProperties> &res) const
  {
    db::Polygon poly = ep.normalized ().to_polygon (m_e);
    if (poly.vertices () >= 3) {
      res.push_back (db::PolygonWithProperties (poly, ep.properties_id ()));
    }
  }

private:
  db::Coord m_e;
};

class DB_PUBLIC
EdgePairToEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToEdgesProcessor ()
  { }

  void process(const EdgePairWithProperties &ep, std::vector<db::EdgeWithProperties> &res) const
  {
    res.push_back (db::EdgeWithProperties (ep.first (), ep.properties_id ()));
    res.push_back (db::EdgeWithProperties (ep.second (), ep.properties_id ()));
  }
};

class DB_PUBLIC
EdgePairToFirstEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToFirstEdgesProcessor ()
  { }

  void process(const EdgePairWithProperties &ep, std::vector<db::EdgeWithProperties> &res) const
  {
    res.push_back (db::EdgeWithProperties (ep.first (), ep.properties_id ()));
    if (ep.is_symmetric ()) {
      res.push_back (db::EdgeWithProperties (ep.second (), ep.properties_id ()));
    }
  }
};

class DB_PUBLIC
EdgePairToSecondEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToSecondEdgesProcessor ()
  { }

  void process(const EdgePairWithProperties &ep, std::vector<db::EdgeWithProperties> &res) const
  {
    if (! ep.is_symmetric ()) {
      res.push_back (db::EdgeWithProperties (ep.second (), ep.properties_id ()));
    }
  }
};

class DB_PUBLIC
EdgePairToLesserEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToLesserEdgesProcessor ()
  { }

  void process(const EdgePairWithProperties &ep, std::vector<db::EdgeWithProperties> &res) const
  {
    res.push_back (db::EdgeWithProperties (ep.lesser (), ep.properties_id ()));
  }
};

class DB_PUBLIC
EdgePairToGreaterEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToGreaterEdgesProcessor ()
  { }

  void process(const EdgePairWithProperties &ep, std::vector<db::EdgeWithProperties> &res) const
  {
    res.push_back (db::EdgeWithProperties (ep.greater (), ep.properties_id ()));
  }
};

/**
 *  @brief The edge pair set iterator delegate
 */
typedef db::generic_shape_iterator_delegate_base <db::EdgePair> EdgePairsIteratorDelegate;

/**
 *  @brief The delegate for the actual edge set implementation
 */
class DB_PUBLIC EdgePairsDelegate
  : public ShapeCollectionDelegateBase
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::EdgePair edge_pair_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;

  EdgePairsDelegate ();
  virtual ~EdgePairsDelegate ();

  EdgePairsDelegate (const EdgePairsDelegate &other);
  EdgePairsDelegate &operator= (const EdgePairsDelegate &other);

  virtual EdgePairsDelegate *clone () const = 0;

  EdgePairsDelegate *remove_properties (bool remove = true)
  {
    ShapeCollectionDelegateBase::remove_properties (remove);
    return this;
  }

  void set_base_verbosity (int vb);
  int base_verbosity () const
  {
    return m_base_verbosity;
  }

  void enable_progress (const std::string &progress_desc);
  void disable_progress ();

  //  dummy features to harmonize the interface of region, edges and edge pair delegates
  void set_merged_semantics (bool) { }
  bool merged_semantics () const { return false; }
  void set_is_merged (bool) { }
  bool is_merged () const { return false; }

  virtual std::string to_string (size_t nmax) const = 0;

  virtual EdgePairsIteratorDelegate *begin () const = 0;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const = 0;

  virtual bool empty () const = 0;
  virtual size_t count () const = 0;
  virtual size_t hier_count () const = 0;

  virtual Box bbox () const = 0;

  virtual EdgePairsDelegate *filter_in_place (const EdgePairFilterBase &filter) = 0;
  virtual EdgePairsDelegate *filtered (const EdgePairFilterBase &filter) const = 0;
  virtual std::pair<EdgePairsDelegate *, EdgePairsDelegate *> filtered_pair (const EdgePairFilterBase &filter) const = 0;
  virtual EdgePairsDelegate *process_in_place (const EdgePairProcessorBase &proc) = 0;
  virtual EdgePairsDelegate *processed (const EdgePairProcessorBase &proc) const = 0;
  virtual RegionDelegate *processed_to_polygons (const EdgePairToPolygonProcessorBase &proc) const = 0;
  virtual EdgesDelegate *processed_to_edges (const EdgePairToEdgeProcessorBase &proc) const = 0;

  virtual RegionDelegate *pull_interacting (const Region &) const = 0;
  virtual EdgesDelegate *pull_interacting (const Edges &) const = 0;
  virtual EdgePairsDelegate *selected_interacting (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual EdgePairsDelegate *selected_not_interacting (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual EdgePairsDelegate *selected_interacting (const Edges &other, size_t min_count, size_t max_count) const = 0;
  virtual EdgePairsDelegate *selected_not_interacting (const Edges &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<EdgePairsDelegate *, EdgePairsDelegate *> selected_interacting_pair (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<EdgePairsDelegate *, EdgePairsDelegate *> selected_interacting_pair (const Edges &other, size_t min_count, size_t max_count) const = 0;

  virtual EdgePairsDelegate *selected_outside (const Region &other) const = 0;
  virtual EdgePairsDelegate *selected_not_outside (const Region &other) const = 0;
  virtual std::pair<EdgePairsDelegate *, EdgePairsDelegate *> selected_outside_pair (const Region &other) const = 0;
  virtual EdgePairsDelegate *selected_inside (const Region &other) const = 0;
  virtual EdgePairsDelegate *selected_not_inside (const Region &other) const = 0;
  virtual std::pair<EdgePairsDelegate *, EdgePairsDelegate *> selected_inside_pair (const Region &other) const = 0;

  virtual RegionDelegate *polygons (db::Coord e) const = 0;
  virtual EdgesDelegate *edges () const = 0;
  virtual EdgesDelegate *first_edges () const = 0;
  virtual EdgesDelegate *second_edges () const = 0;

  virtual EdgePairsDelegate *add_in_place (const EdgePairs &other) = 0;
  virtual EdgePairsDelegate *add (const EdgePairs &other) const = 0;

  virtual EdgePairsDelegate *in (const EdgePairs &other, bool invert) const = 0;

  virtual const db::EdgePair *nth (size_t n) const = 0;
  virtual db::properties_id_type nth_prop_id (size_t n) const = 0;
  virtual bool has_valid_edge_pairs () const = 0;

  virtual const db::RecursiveShapeIterator *iter () const = 0;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt) = 0;

  virtual bool equals (const EdgePairs &other) const = 0;
  virtual bool less (const EdgePairs &other) const = 0;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const = 0;
  virtual void insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const = 0;

protected:
  const std::string &progress_desc () const
  {
    return m_progress_desc;
  }

  bool report_progress () const
  {
    return m_report_progress;
  }

private:
  bool m_report_progress;
  std::string m_progress_desc;
  int m_base_verbosity;
};

}

#endif


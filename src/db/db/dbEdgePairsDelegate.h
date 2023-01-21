
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

  void process(const EdgePair &ep, std::vector<db::Polygon> &res) const
  {
    db::Polygon poly = ep.normalized ().to_polygon (m_e);
    if (poly.vertices () >= 3) {
      res.push_back (poly);
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

  void process(const EdgePair &ep, std::vector<db::Edge> &res) const
  {
    res.push_back (ep.first ());
    res.push_back (ep.second ());
  }
};

class DB_PUBLIC
EdgePairToFirstEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToFirstEdgesProcessor ()
  { }

  void process(const EdgePair &ep, std::vector<db::Edge> &res) const
  {
    res.push_back (ep.first ());
    if (ep.is_symmetric ()) {
      res.push_back (ep.second ());
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

  void process(const EdgePair &ep, std::vector<db::Edge> &res) const
  {
    if (! ep.is_symmetric ()) {
      res.push_back (ep.second ());
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

  void process(const EdgePair &ep, std::vector<db::Edge> &res) const
  {
    res.push_back (ep.lesser ());
  }
};

class DB_PUBLIC
EdgePairToGreaterEdgesProcessor
  : public EdgePairToEdgeProcessorBase
{
public:
  EdgePairToGreaterEdgesProcessor ()
  { }

  void process(const EdgePair &ep, std::vector<db::Edge> &res) const
  {
    res.push_back (ep.greater ());
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
  virtual RegionDelegate *processed_to_polygons (const EdgePairToPolygonProcessorBase &filter) const = 0;
  virtual EdgesDelegate *processed_to_edges (const EdgePairToEdgeProcessorBase &filter) const = 0;

  virtual RegionDelegate *polygons (db::Coord e) const = 0;
  virtual EdgesDelegate *edges () const = 0;
  virtual EdgesDelegate *first_edges () const = 0;
  virtual EdgesDelegate *second_edges () const = 0;

  virtual EdgePairsDelegate *add_in_place (const EdgePairs &other) = 0;
  virtual EdgePairsDelegate *add (const EdgePairs &other) const = 0;

  virtual EdgePairsDelegate *in (const EdgePairs &other, bool invert) const = 0;

  virtual const db::EdgePair *nth (size_t n) const = 0;
  virtual bool has_valid_edge_pairs () const = 0;

  virtual const db::RecursiveShapeIterator *iter () const = 0;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt) = 0;
  virtual db::PropertiesRepository *properties_repository () = 0;
  virtual const db::PropertiesRepository *properties_repository () const = 0;

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
};

}

#endif


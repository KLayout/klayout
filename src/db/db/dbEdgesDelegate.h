
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


#ifndef HDR_dbEdgesDelegate
#define HDR_dbEdgesDelegate

#include "dbCommon.h"

#include "dbEdge.h"
#include "dbEdgePairs.h"
#include "dbEdgePairRelations.h"
#include "dbShapeCollection.h"
#include "dbShapeCollectionUtils.h"
#include "dbGenericShapeIterator.h"
#include "dbHash.h"

#include <list>
#include <unordered_set>

namespace db {

/**
 *  @brief A structure holding the options for the region checks (space, width, ...)
 */
struct DB_PUBLIC EdgesCheckOptions
{
  typedef db::coord_traits<db::Coord>::distance_type distance_type;

  /**
   *  @brief Constructor
   */
  EdgesCheckOptions (bool _whole_edges = false,
                      metrics_type _metrics = db::Euclidian,
                      double _ignore_angle = 90,
                      distance_type _min_projection = 0,
                      distance_type _max_projection = std::numeric_limits<distance_type>::max ())
    : whole_edges (_whole_edges),
      metrics (_metrics),
      ignore_angle (_ignore_angle),
      min_projection (_min_projection),
      max_projection (_max_projection)
  { }

  /**
   *  @brief Specifies is whole edges are to be delivered
   *
   *  Without "whole_edges", the parts of
   *  the edges are returned which violate the condition. If "whole_edges" is true, the
   *  result will contain the complete edges participating in the result.
   */
  bool whole_edges;

  /**
   *  @brief Measurement metrics
   *
   *  The metrics parameter specifies which metrics to use. "Euclidian", "Square" and "Projected"
   *  metrics are available.
   */
  metrics_type metrics;

  /**
   *  @brief Specifies the obtuse angle threshold
   *
   *  "ignore_angle" allows specification of a maximum angle that connected edges can have to not participate
   *  in the check. By choosing 90 degree, edges with angles of 90 degree and larger are not checked,
   *  but acute corners are for example.
   */
  double ignore_angle;

  /**
   *  @brief Specifies the projection limit's minimum value
   *
   *  With min_projection and max_projection it is possible to specify how edges must be related
   *  to each other. If the length of the projection of either edge on the other is >= min_projection
   *  or < max_projection, the edges are considered for the check.
   */
  distance_type min_projection;

  /**
   *  @brief Specifies the projection limit's maximum value
   */
  distance_type max_projection;
};

/**
 *  @brief A base class for edge filters
 */
class DB_PUBLIC EdgeFilterBase
{
public:
  /**
   *  @brief Constructor
   */
  EdgeFilterBase () { }

  virtual ~EdgeFilterBase () { }

  /**
   *  @brief Filters the edge
   *  If this method returns true, the edge is kept. Otherwise it's discarded.
   */
  virtual bool selected (const db::Edge &edge) const = 0;

  /**
   *  @brief Filters the edge set
   *  If this method returns true, the edges are kept. Otherwise they are discarded.
   */
  virtual bool selected (const std::unordered_set<db::Edge> &edge) const = 0;

  /**
   *  @brief Returns the transformation reducer for building cell variants
   *  This method may return 0. In this case, not cell variants are built.
   */
  virtual const TransformationReducer *vars () const = 0;

  /**
   *  @brief Returns true, if the filter wants raw (not merged) input
   */
  virtual bool requires_raw_input () const = 0;

  /**
   *  @brief Returns true, if the filter wants to build variants
   *  If not true, the filter accepts shape propagation as variant resolution.
   */
  virtual bool wants_variants () const = 0;
};

/**
 *  @brief A template base class for edge processors
 *
 *  A polygon processor can turn a edge into something else.
 */
template <class Result>
class DB_PUBLIC edge_processor
{
public:
  /**
   *  @brief Constructor
   */
  edge_processor () { }

  /**
   *  @brief Destructor
   */
  virtual ~edge_processor () { }

  /**
   *  @brief Performs the actual processing
   *  This method will take the input edge from "edge" and puts the results into "res".
   *  "res" can be empty - in this case, the edge will be skipped.
   */
  virtual void process (const db::Edge &edge, std::vector<Result> &res) const = 0;

  /**
   *  @brief Returns the transformation reducer for building cell variants
   *  This method may return 0. In this case, not cell variants are built.
   */
  virtual const TransformationReducer *vars () const = 0;

  /**
   *  @brief Returns true, if the result of this operation can be regarded "merged" always.
   */
  virtual bool result_is_merged () const = 0;

  /**
   *  @brief Returns true, if the result of this operation must not be merged.
   *  This feature can be used, if the result represents "degenerated" objects such
   *  as point-like edges. These must not be merged. Otherwise they disappear.
   */
  virtual bool result_must_not_be_merged () const = 0;

  /**
   *  @brief Returns true, if the processor wants raw (not merged) input
   */
  virtual bool requires_raw_input () const = 0;

  /**
   *  @brief Returns true, if the processor wants to build variants
   *  If not true, the processor accepts shape propagation as variant resolution.
   */
  virtual bool wants_variants () const = 0;
};

typedef shape_collection_processor<db::Edge, db::Edge> EdgeProcessorBase;
typedef shape_collection_processor<db::Edge, db::Polygon> EdgeToPolygonProcessorBase;
typedef shape_collection_processor<db::Edge, db::EdgePair> EdgeToEdgePairProcessorBase;

class RecursiveShapeIterator;
class EdgeFilterBase;
class EdgePairsDelegate;
class RegionDelegate;

/**
 *  @brief The edge set iterator delegate
 */
typedef db::generic_shape_iterator_delegate_base <db::Edge> EdgesIteratorDelegate;

/**
 *  @brief The delegate for the actual edge set implementation
 */
class DB_PUBLIC EdgesDelegate
  : public ShapeCollectionDelegateBase
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::Edge edge_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;
  typedef coord_traits::distance_type distance_type;
  typedef coord_traits::distance_type length_type;

  EdgesDelegate ();
  virtual ~EdgesDelegate ();

  EdgesDelegate (const EdgesDelegate &other);
  EdgesDelegate &operator= (const EdgesDelegate &other);

  virtual EdgesDelegate *clone () const = 0;

  EdgesDelegate *remove_properties (bool remove = true)
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

  void set_merged_semantics (bool f);
  bool merged_semantics () const
  {
    return m_merged_semantics;
  }

  void set_strict_handling (bool f);
  bool strict_handling () const
  {
    return m_strict_handling;
  }

  virtual std::string to_string (size_t nmax) const = 0;

  virtual EdgesIteratorDelegate *begin () const = 0;
  virtual EdgesIteratorDelegate *begin_merged () const = 0;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const = 0;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const = 0;

  virtual bool empty () const = 0;
  virtual bool is_merged () const = 0;
  virtual size_t count () const = 0;
  virtual size_t hier_count () const = 0;

  virtual distance_type length (const db::Box &box) const = 0;
  virtual Box bbox () const = 0;

  virtual EdgePairsDelegate *width_check (db::Coord d, const db::EdgesCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *space_check (db::Coord d, const db::EdgesCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *enclosing_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *overlap_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *separation_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *inside_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const = 0;

  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &filter) = 0;
  virtual EdgesDelegate *filtered (const EdgeFilterBase &filter) const = 0;
  virtual EdgesDelegate *process_in_place (const EdgeProcessorBase &filter) = 0;
  virtual EdgesDelegate *processed (const EdgeProcessorBase &filter) const = 0;
  virtual EdgePairsDelegate *processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &filter) const = 0;
  virtual RegionDelegate *processed_to_polygons (const EdgeToPolygonProcessorBase &filter) const = 0;

  virtual EdgesDelegate *merged_in_place () = 0;
  virtual EdgesDelegate *merged () const = 0;

  virtual EdgesDelegate *and_with (const Edges &other) const = 0;
  virtual EdgesDelegate *not_with (const Edges &other) const = 0;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> andnot_with (const Edges &) const = 0;
  virtual EdgesDelegate *and_with (const Region &other) const = 0;
  virtual EdgesDelegate *not_with (const Region &other) const = 0;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> andnot_with (const Region &) const = 0;
  virtual EdgesDelegate *xor_with (const Edges &other) const = 0;
  virtual EdgesDelegate *or_with (const Edges &other) const = 0;
  virtual EdgesDelegate *add_in_place (const Edges &other) = 0;
  virtual EdgesDelegate *add (const Edges &other) const = 0;
  virtual EdgesDelegate *intersections (const Edges &other) const = 0;

  virtual RegionDelegate *extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const = 0;

  virtual EdgesDelegate *inside_part (const Region &other) const = 0;
  virtual EdgesDelegate *outside_part (const Region &other) const = 0;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> inside_outside_part_pair (const Region &other) const = 0;
  virtual RegionDelegate *pull_interacting (const Region &) const = 0;
  virtual EdgesDelegate *pull_interacting (const Edges &) const = 0;
  virtual EdgesDelegate *selected_interacting (const Region &other) const = 0;
  virtual EdgesDelegate *selected_not_interacting (const Region &other) const = 0;
  virtual EdgesDelegate *selected_interacting (const Edges &other) const = 0;
  virtual EdgesDelegate *selected_not_interacting (const Edges &other) const = 0;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_interacting_pair (const Region &other) const = 0;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_interacting_pair (const Edges &other) const = 0;

  virtual EdgesDelegate *selected_outside (const Region &other) const = 0;
  virtual EdgesDelegate *selected_not_outside (const Region &other) const = 0;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_outside_pair (const Region &other) const = 0;
  virtual EdgesDelegate *selected_inside (const Region &other) const = 0;
  virtual EdgesDelegate *selected_not_inside (const Region &other) const = 0;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_inside_pair (const Region &other) const = 0;
  virtual EdgesDelegate *selected_outside (const Edges &other) const = 0;
  virtual EdgesDelegate *selected_not_outside (const Edges &other) const = 0;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_outside_pair (const Edges &other) const = 0;
  virtual EdgesDelegate *selected_inside (const Edges &other) const = 0;
  virtual EdgesDelegate *selected_not_inside (const Edges &other) const = 0;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_inside_pair (const Edges &other) const = 0;

  virtual EdgesDelegate *in (const Edges &other, bool invert) const = 0;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> in_and_out (const Edges &) const = 0;

  virtual const db::Edge *nth (size_t n) const = 0;
  virtual bool has_valid_edges () const = 0;
  virtual bool has_valid_merged_edges () const = 0;

  virtual const db::RecursiveShapeIterator *iter () const = 0;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt) = 0;
  virtual db::PropertiesRepository *properties_repository () = 0;
  virtual const db::PropertiesRepository *properties_repository () const = 0;

  virtual bool equals (const Edges &other) const = 0;
  virtual bool less (const Edges &other) const = 0;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const = 0;

protected:
  const std::string &progress_desc () const
  {
    return m_progress_desc;
  }

  bool report_progress () const
  {
    return m_report_progress;
  }

  virtual void merged_semantics_changed () { }

private:
  bool m_merged_semantics;
  bool m_strict_handling;
  bool m_report_progress;
  std::string m_progress_desc;
  int m_base_verbosity;
};

}

#endif



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


#ifndef HDR_dbRegionDelegate
#define HDR_dbRegionDelegate

#include "dbCommon.h"

#include "dbPolygon.h"
#include "dbEdges.h"
#include "dbTexts.h"
#include "dbEdgePairs.h"
#include "dbEdgePairRelations.h"
#include "dbShapeCollection.h"
#include "dbGenericShapeIterator.h"
#include "dbRegionLocalOperations.h"
#include "dbHash.h"
#include "dbLayoutToNetlistEnums.h"

#include <list>
#include <set>
#include <unordered_set>

namespace db {

class RecursiveShapeIterator;
class EdgeFilterBase;
class EdgesDelegate;
class EdgePairsDelegate;
class CompoundRegionOperationNode;
class LayoutToNetlist;
class Net;

/**
 *  @brief A base class for polygon filters
 */
class DB_PUBLIC PolygonFilterBase
{
public:
  /**
   *  @brief Constructor
   */
  PolygonFilterBase () { }

  virtual ~PolygonFilterBase () { }

  /**
   *  @brief Filters the polygon
   *  If this method returns true, the polygon is kept. Otherwise it's discarded.
   */
  virtual bool selected (const db::Polygon &polygon) const = 0;

  /**
   *  @brief Filters the polygon reference
   *  If this method returns true, the polygon is kept. Otherwise it's discarded.
   */
  virtual bool selected (const db::PolygonRef &polygon) const = 0;

  /**
   *  @brief Filters the set of polygons (taking the overall properties)
   *  If this method returns true, the polygon is kept. Otherwise it's discarded.
   */
  virtual bool selected_set (const std::unordered_set<db::Polygon> &polygons) const = 0;

  /**
   *  @brief Filters the set of polygon references (taking the overall properties)
   *  If this method returns true, the polygon is kept. Otherwise it's discarded.
   */
  virtual bool selected_set (const std::unordered_set<db::PolygonRef> &polygons) const = 0;

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
 *  @brief A template base class for polygon processors
 *
 *  A polygon processor can turn a polygon into something else.
 */
template <class Result>
class DB_PUBLIC polygon_processor
{
public:
  /**
   *  @brief Constructor
   */
  polygon_processor () { }

  /**
   *  @brief Destructor
   */
  virtual ~polygon_processor () { }

  /**
   *  @brief Performs the actual processing
   *  This method will take the input polygon from "polygon" and puts the results into "res".
   *  "res" can be empty - in this case, the polygon will be skipped.
   */
  virtual void process (const db::Polygon &polygon, std::vector<Result> &res) const = 0;

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

typedef shape_collection_processor<db::Polygon, db::Polygon> PolygonProcessorBase;
typedef shape_collection_processor<db::Polygon, db::Edge> PolygonToEdgeProcessorBase;
typedef shape_collection_processor<db::Polygon, db::EdgePair> PolygonToEdgePairProcessorBase;

/**
 *  @brief The region iterator delegate
 */
typedef db::generic_shape_iterator_delegate_base <db::Polygon> RegionIteratorDelegate;

/**
 *  @brief The delegate for the actual region implementation
 */
class DB_PUBLIC RegionDelegate
  : public db::ShapeCollectionDelegateBase
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::Polygon polygon_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;
  typedef coord_traits::distance_type distance_type;
  typedef coord_traits::perimeter_type perimeter_type;
  typedef coord_traits::area_type area_type;

  RegionDelegate ();
  virtual ~RegionDelegate ();

  RegionDelegate (const RegionDelegate &other);
  RegionDelegate &operator= (const RegionDelegate &other);

  virtual RegionDelegate *clone () const = 0;

  RegionDelegate *remove_properties (bool remove = true)
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

  void set_min_coherence (bool f);
  bool min_coherence () const
  {
    return m_merge_min_coherence;
  }

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

  virtual RegionIteratorDelegate *begin () const = 0;
  virtual RegionIteratorDelegate *begin_merged () const = 0;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const = 0;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const = 0;

  virtual bool empty () const = 0;
  virtual bool is_box () const = 0;
  virtual bool is_merged () const = 0;
  virtual size_t hier_count () const = 0;
  virtual size_t count () const = 0;

  virtual area_type area (const db::Box &box) const = 0;
  virtual perimeter_type perimeter (const db::Box &box) const = 0;
  virtual Box bbox () const = 0;

  virtual EdgePairsDelegate *cop_to_edge_pairs (db::CompoundRegionOperationNode &node, db::PropertyConstraint prop_constraint) = 0;
  virtual RegionDelegate *cop_to_region (db::CompoundRegionOperationNode &node, db::PropertyConstraint prop_constraint) = 0;
  virtual EdgesDelegate *cop_to_edges (db::CompoundRegionOperationNode &node, db::PropertyConstraint prop_constraint) = 0;

  virtual EdgePairsDelegate *width_check (db::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *space_check (db::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *isolated_check (db::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *notch_check (db::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *enclosing_check (const Region &other, db::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *overlap_check (const Region &other, db::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *separation_check (const Region &other, db::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *inside_check (const Region &other, db::Coord d, const RegionCheckOptions &options) const = 0;
  virtual EdgePairsDelegate *grid_check (db::Coord gx, db::Coord gy) const = 0;
  virtual EdgePairsDelegate *angle_check (double min, double max, bool inverse) const = 0;

  virtual RegionDelegate *snapped_in_place (db::Coord gx, db::Coord gy) = 0;
  virtual RegionDelegate *snapped (db::Coord gx, db::Coord gy) = 0;
  virtual RegionDelegate *scaled_and_snapped_in_place (db::Coord gx, db::Coord mx, db::Coord dx, db::Coord gy, db::Coord my, db::Coord dy) = 0;
  virtual RegionDelegate *scaled_and_snapped (db::Coord gx, db::Coord mx, db::Coord dx, db::Coord gy, db::Coord my, db::Coord dy) = 0;

  virtual EdgesDelegate *edges (const EdgeFilterBase *filter) const = 0;
  virtual RegionDelegate *filter_in_place (const PolygonFilterBase &filter) = 0;
  virtual RegionDelegate *filtered (const PolygonFilterBase &filter) const = 0;
  virtual RegionDelegate *process_in_place (const PolygonProcessorBase &filter) = 0;
  virtual RegionDelegate *processed (const PolygonProcessorBase &filter) const = 0;
  virtual EdgesDelegate *processed_to_edges (const PolygonToEdgeProcessorBase &filter) const = 0;
  virtual EdgePairsDelegate *processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &filter) const = 0;

  virtual RegionDelegate *merged_in_place () = 0;
  virtual RegionDelegate *merged_in_place (bool min_coherence, unsigned int min_wc) = 0;
  virtual RegionDelegate *merged () const = 0;
  virtual RegionDelegate *merged (bool min_coherence, unsigned int min_wc) const = 0;

  virtual RegionDelegate *sized (coord_type d, unsigned int mode) const = 0;
  virtual RegionDelegate *sized (coord_type dx, coord_type dy, unsigned int mode) const = 0;

  virtual RegionDelegate *and_with (const Region &other, PropertyConstraint prop_constraint) const = 0;
  virtual RegionDelegate *not_with (const Region &other, PropertyConstraint prop_constraint) const = 0;
  virtual RegionDelegate *xor_with (const Region &other, PropertyConstraint prop_constraint) const = 0;
  virtual RegionDelegate *or_with (const Region &other, PropertyConstraint prop_constraint) const = 0;
  virtual RegionDelegate *add_in_place (const Region &other) = 0;
  virtual RegionDelegate *add (const Region &other) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> andnot_with (const Region &other, PropertyConstraint prop_constraint) const = 0;

  virtual RegionDelegate *selected_outside (const Region &other) const = 0;
  virtual RegionDelegate *selected_not_outside (const Region &other) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_outside_pair (const Region &other) const = 0;
  virtual RegionDelegate *selected_inside (const Region &other) const = 0;
  virtual RegionDelegate *selected_not_inside (const Region &other) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_inside_pair (const Region &other) const = 0;
  virtual RegionDelegate *selected_enclosing (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_not_enclosing (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_enclosing_pair (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_interacting (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_not_interacting (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_pair (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_interacting (const Edges &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_not_interacting (const Edges &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_pair (const Edges &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_interacting (const Texts &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_not_interacting (const Texts &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_pair (const Texts &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_overlapping (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *selected_not_overlapping (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_overlapping_pair (const Region &other, size_t min_count, size_t max_count) const = 0;
  virtual RegionDelegate *pull_inside (const Region &other) const = 0;
  virtual RegionDelegate *pull_interacting (const Region &other) const = 0;
  virtual EdgesDelegate *pull_interacting (const Edges &other) const = 0;
  virtual RegionDelegate *pull_overlapping (const Region &other) const = 0;
  virtual TextsDelegate *pull_interacting (const Texts &other) const = 0;
  virtual RegionDelegate *in (const Region &other, bool invert) const = 0;
  virtual std::pair<RegionDelegate *, RegionDelegate *> in_and_out (const Region &other) const = 0;

  virtual const db::Polygon *nth (size_t n) const = 0;
  virtual db::properties_id_type nth_prop_id (size_t n) const = 0;
  virtual bool has_valid_polygons () const = 0;
  virtual bool has_valid_merged_polygons () const = 0;

  virtual const db::RecursiveShapeIterator *iter () const = 0;

  virtual void apply_property_translator (const db::PropertiesTranslator &pt) = 0;
  virtual db::PropertiesRepository *properties_repository () = 0;
  virtual const db::PropertiesRepository *properties_repository () const = 0;

  virtual bool equals (const Region &other) const = 0;
  virtual bool less (const Region &other) const = 0;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const = 0;

  virtual RegionDelegate *nets (LayoutToNetlist *l2n, NetPropertyMode prop_mode, const tl::Variant &net_prop_name, const std::vector<const db::Net *> *net_filter) const = 0;

  const std::string &progress_desc () const
  {
    return m_progress_desc;
  }

  bool report_progress () const
  {
    return m_report_progress;
  }

protected:
  virtual void merged_semantics_changed () { }
  virtual void min_coherence_changed () { }

private:
  bool m_merged_semantics;
  bool m_strict_handling;
  bool m_merge_min_coherence;
  bool m_report_progress;
  std::string m_progress_desc;
  int m_base_verbosity;
};

}

#endif

